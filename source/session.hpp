#ifndef __M_SESSION_H__
#define __M_SESSION_H__

#include "util.hpp"
#include <unordered_map>

typedef enum
{
    UNLOGIN,
    LOGIN
} ss_statu;

class session
{
private:
    uint64_t _ssid;            // 标识符
    uint64_t _uid;             // 对应的用户id
    ss_statu _statu;           // 用户状态 未登录 已登录
    wsserver_t::timer_ptr _tp; // session关联的定时器

public:
    session(uint64_t ssid) : _ssid(ssid)
    {
        DLOG("session %p 被创建", this);
    }
    ~session()
    {
        DLOG("session %p 被释放", this);
    }

    uint64_t ssid()
    {
        return _ssid;
    }
    void set_user(uint64_t uid)
    {
        _uid = uid;
    }
    uint64_t get_user()
    {
        return _uid;
    }
   
    void set_statu(ss_statu statu)
    {
        _statu = statu;
    }
    bool is_login()
    {
        return (_statu == LOGIN);
    }
    void set_timer(const wsserver_t::timer_ptr &tp)
    {
        _tp = tp;
    }
    wsserver_t::timer_ptr &get_timer()
    {
        return _tp;
    }
};

using session_ptr = std::shared_ptr<session>;
#define SESSION_TIMEOUT 30000
#define SESSION_FOREVER -1
class session_manager
{
private:
    uint64_t _next_ssid;
    std::mutex _mutex;
    std::unordered_map<uint64_t, session_ptr> _session;
    wsserver_t *_server;

public:
    session_manager(wsserver_t *srv) : _next_ssid(1),
                                       _server(srv)
    {
        DLOG("session初始化完毕");
    }
    ~session_manager()
    {
        DLOG("session 即将销毁");
    }
    session_ptr create_session(uint64_t uid, ss_statu statu) // 默认登录才创建
    {
        std::unique_lock<std::mutex> lock(_mutex);
        session_ptr ssp(new session(_next_ssid)); // 实例化了session对象 并且用智能指针管理起来
        ssp->set_statu(statu);
        ssp->set_user(uid); 
        _session.insert(std::make_pair(_next_ssid, ssp)); // 添加智能指针的管理对象
        _next_ssid++;
        return ssp;
    }

    // 通过ssid获取session
    session_ptr get_session_by_ssid(uint64_t ssid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _session.find(ssid);
        if (it == _session.end())
        {
            return session_ptr();
        }
        return it->second;
    }

    void append_session(const session_ptr &ssp)
    {
        std::unique_lock<std::mutex> lock(_mutex);         // 不是立即删除 所以单独把添加拿出来封装
        _session.insert(std::make_pair(ssp->ssid(), ssp)); // 添加了 没有定时任务的
    }
    void remove_session(uint64_t ssid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _session.erase(ssid);
        // 一旦不保存智能指针 计时器大多为1 为界也不使用 减1 为0 就会释放
    }

    void set_session_expire_time(uint64_t ssid, int ms)
    {
        // 依赖于websocketpp的定时器来完成session生命周期的管理
        // 为什么这么复杂 因为最早用户登录注册都是http短链接 所以他们的session要定时删除 不能永久存在
        // 但是一旦客服端与服务器建立websocket长连接 短链接变成长连接 即进入游戏后
        // 就算很长时间没有通信 也不应该删除session 所以要设置为永久存在
        // 直到用户退出游戏 才把session又重新定时删除

        session_ptr ssp = get_session_by_ssid(ssid);
        if (ssp.get() == nullptr) // 不存在session
        {
            return;
        }
        // 1.在session永久存在的情况下 设置永久存在
        wsserver_t::timer_ptr tp = ssp->get_timer();
        // 怎么判断是否是永久存在  因为session里有个定时器tp 没有关联定时删除的任务，所以就是永久存在
        if (tp.get() == nullptr && ms == SESSION_FOREVER)
        {
            return;
        }
        else if (tp.get() == nullptr && ms != SESSION_FOREVER)
        {
            // 2.在session永久存在情况下  设置指定时间后被删除
            wsserver_t::timer_ptr tmp_tp = _server->set_timer(ms,
                                                              std::bind(&session_manager::remove_session, this, ssid)); // 里面成员的函数 类名称引用
            ssp->set_timer(tmp_tp);                                                                                     // 设置定时器 你现在有定时任务 下次判断才知道处于什么状态
        }
        else if (tp.get() != nullptr && ms == SESSION_FOREVER)
        {
            // 3.在session设置了定时删除的情况下 将session设置为永久存在
            // 删除定时任务 stready_timer删除会导致任务直接执行 把删除从管理删除了
            // 删除之后保存了一份会话信息 所以要重新添加一次
            tp->cancel();                            // 因为这个取消定时任务并不是立即取消 所以有可能添加后才删除 所以添加一个功能添加的时候使用一个定时器 不是立即添加
            ssp->set_timer(wsserver_t::timer_ptr()); // 将session关联的定时器设置为空
            _server->set_timer(0, std::bind(&session_manager::append_session, this, ssp));
        }
        else if (tp.get() != nullptr && ms != SESSION_FOREVER)
        {
            // 4.在session设置了定时删除的情况下 将session重置删除时间
            tp->cancel();
            ssp->set_timer(wsserver_t::timer_ptr());
            _server->set_timer(0, std::bind(&session_manager::append_session, this, ssp)); // 移除后添加
            // 重新给session添加定时销毁的任务         重新设置定时任务  指定时间后再删除它
            wsserver_t::timer_ptr tmp_tp = _server->set_timer(ms, std::bind(&session_manager::remove_session, this, ssp->ssid()));
            ssp->set_timer(tmp_tp); // 重新添加一个定时器  重新设置session关联的定时器
        }
    }
};

#endif