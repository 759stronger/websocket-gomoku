#ifndef __M_MATCHER_H__
#define __M_MATCHER_H__

#include "util.hpp"
#include "online.hpp"
#include "db.hpp"
#include "room.hpp"
#include <list>
#include <mutex>
#include <condition_variable>

template <class T>
class match_queue
{
private:
    // 使用链表而不直接使用queue  因为有中间删除数据的需要
    std::list<T> _list;
    // 实现线程安全
    std::mutex _mutex;
    // 这个条件变量是为了阻塞消费者 后面使用的时候 队列中元素个数<2 则阻塞
    std::condition_variable _cond;

public:
    // 获取元素个数
    int size()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        return _list.size();
    }

    // 判空
    bool empty()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        return _list.empty();
    }

    // 阻塞线程
    void wait()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _cond.wait(lock);
    }

    // 入队并唤醒线程
    void push(const T &data)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _list.push_back(data);
        _cond.notify_all();
    }

    // 出队数据
    bool pop(T &data)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_list.empty() == true)
        {
            return false;
        }
        data = _list.front();
        _list.pop_front();
        return true;
    }

    // 移除指定的数据
    void remove(T &data)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _list.remove(data);
    }
};

class matcher
{
private:
    match_queue<uint64_t> _q_level1; // 三个不同等级的匹配队列
    match_queue<uint64_t> _q_level2;
    match_queue<uint64_t> _q_level3;
    std::thread _th_level1; // 对应三个队列的三种处理线程
    std::thread _th_level2;
    std::thread _th_level3;
    room_manager *_rm;
    user_table *_ut;
    online_manager *_om;

private:
    void handle_match(match_queue<uint64_t> &mq)
    {
        while(1)//匹配是个死循环的过程
        {
            //1.判断队列人数是否大于2  小于2就阻塞等待
              while(mq.size()<2)
              {
                 mq.wait();
              }
                
            //2.代表人数够了  出队两个玩家 
            uint64_t uid1,uid2;
            bool ret = mq.pop(uid1);  
            if(ret == false){continue;} //如果第一个人掉线就重来 出队失败 刚准备出队就被取消或者掉线
            ret = mq.pop(uid2);
            if(ret == false)   //如果第二个人掉线 就把第一个人重新添加到匹配队列
            {
                this ->add(uid1);continue;
            }

            //3.校验是否在线 如果有人掉线 就把另一个人重新添加队列  简化为获取连接
              wsserver_t::connection_ptr conn1 = _om->get_conn_from_hall(uid1);
              if(conn1.get()==nullptr) //如果玩家1掉线  没有获取到连接
              {
                  this ->add(uid2);continue;
              }
              wsserver_t::connection_ptr conn2 = _om->get_conn_from_hall(uid2);
              if(conn2.get()==nullptr) //如果玩家1掉线
              {
                  this ->add(uid1);continue;
              }


            //4.创建房间，将两个玩家加入房间中
            room_ptr rp =  _rm->create_room(uid1,uid2);
           if(rp.get()== nullptr)
           {
            this->add(uid1);
            this->add(uid2);
            continue;
           }
            //5.对两个玩家进行响应
            Json::Value resp;
            resp["optype"] = "match_success";
            resp["result"]=true;
            std::string body;
            json_util::serialize(resp ,body);
            conn1->send(body);
            conn2->send(body);

        }

    }
    void th_level1_entry()
    {
        return handle_match(_q_level1);
    }
    void th_level2_entry()
    {
         return handle_match(_q_level2);
    }
    void th_level3_entry()
    {
         return handle_match(_q_level3);
    }

    // 一旦匹配成功要给用户返回信息 但是返回什么信息呢 optyp="match_start"  简单的通信接口
public:
    matcher(room_manager *rm, user_table *ut, online_manager *om) : _rm(rm), _ut(ut), _om(om),
            _th_level1(std::thread(&matcher::th_level1_entry, this)),//创建线程 设计一个入口函数 入口函数是成员函数 所以取地址  参数：类的成员函数 所以默认一个this指针作为参数
            _th_level2(std::thread(&matcher::th_level2_entry, this)),
            _th_level3(std::thread(&matcher::th_level3_entry, this))
    {
          DLOG("游戏匹配模块初始化完毕");
    }

    bool add(uint64_t uid)  //根据玩家的分数判断档次 添加到不同的匹配队列
    {
          //1.根据用户id获取玩家信息
         Json::Value user;
         bool ret = _ut->select_by_id(uid,user);
          if(ret == false)
          {
              DLOG("匹配时获取玩家信息失败 %d ",uid);
              return false;
          }
          int score = user["score"].asInt();
          
          //2.添加到指定的队列
          if(score < 2000)
          {
              _q_level1.push(uid);
          }
          else if(score >= 2000 &&score <3000)
          {
              _q_level2.push(uid);
          }
          else
          {
               _q_level3.push(uid);
          }
          return true;
    }


    bool del(uint64_t uid)
    {
         //1.根据用户id获取玩家信息
         Json::Value user;
         bool ret = _ut->select_by_id(uid,user);
          if(ret == false)
          {
              DLOG("匹配时获取玩家信息失败 %d ",uid);
              return false;
          }
          int score = user["score"].asInt();
          
          //2.从队列移除
          if(score < 2000)
          {
              _q_level1.remove(uid);
          }
          else if(score >= 2000 &&score <3000)
          {
              _q_level2.remove(uid);
          }
          else
          {
               _q_level3.remove(uid);
          }
         return true;
    }

};

#endif