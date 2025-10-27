#ifndef __M_DB_H__      //数据管理模块
#define __M_DB_H__
#include "util.hpp"
#include <mutex>
#include <cassert>

class user_table // 这些接口可能在多线程被调用 mysql_query是线程安全的 但是查询不一样 里面执行语句和将结果保存到本地是两个操作，两个合并起来会出问题，需要保证原子操作
{
public:
    user_table(const std::string &host,
               const std::string &username,
               const std::string &password,
               const std::string &dbname,
               uint16_t port = 3306) // 构造函数 初始化
    {
        _mysql = mysql_util::mysql_create(host, username, password, dbname, port);
        assert(_mysql != NULL);
    }

    ~user_table()
    {

        mysql_util::mysql_destroy(_mysql);  //重复释放 所以删除了util里面mysql执行的释放语句
        _mysql = NULL;
    }

    bool insert(Json::Value &user)
    { // 对密码进行了加密  分数默认1000 场次为0
#define INSERT_USER "insert user values(null,'%s' ,password('%s'),1000,0,0);"
      // sprintf(void *buf, char * format , ...) 格式化输出函数 对多个数据对象按一定格式组织成字符串存储到指定的空间里
        
        if(user["username"].isNull()||user["password"].isNull())
        {
            DLOG("请输入用户名或密码");
            return false;
        }//使用asCString()这个方法还是不行 需要判断是否存在所以使用isNULL

        char sql[4096] = {0}; // 获取c语言风格的字符串 asCString
        sprintf(sql, INSERT_USER, user["username"].asCString(), user["password"].asCString());//如果这里不传入某个参数比如密码就会出错，使用空地址就出错
                       
        bool ret = mysql_util::mysql_exec(_mysql, sql); // 执行语句
        if (ret == false)
        {
            DLOG("insert user info failed\n");
            return false;
        }
        return true;
    }

    bool login(Json::Value &user)
    {
          if(user["username"].isNull()||user["password"].isNull())
        {
            DLOG("请输入用户名或密码");
            return false;
        }
// 用用户名和密码来查询 查询到数据就表示有用户 没有查询到就用户名密码错误
#define LOGIN_USER "select id , score ,total_count,win_count from user where username ='%s' and password = password('%s');"
        char sql[4096] = {0};
        sprintf(sql, LOGIN_USER, user["username"].asCString(), user["password"].asCString());
        // 定义一个锁的管理器  直接使用锁太麻烦low
        MYSQL_RES *res = NULL;
        {                                                   // 花括号形成局部作用域，锁在局部空间内管理，出了作用域锁自动解开
            std::unique_lock<std::mutex> lock(_mutex);      // 通过lock管理锁
            bool ret = mysql_util::mysql_exec(_mysql, sql); // 执行语句
            if (ret == false)
            {
                DLOG(" user login failed\n");
                return false;
            }
            // 有数据也只能有一条数据
            res = mysql_store_result(_mysql); // 结果保存到本地
            if (res == NULL)
            {
                DLOG("have no login user info");
                return false;
            }
        }

        int row_num = mysql_num_rows(res); // 获取结果条目数量
        if (row_num != 1)
        {
            DLOG("user information is not unique");
            return false;
        }
        MYSQL_ROW row = mysql_fetch_row(res); // 唯一就取出数据 遍历结果集 放到row
        user["id"] = (Json::UInt64)std::stol(row[0]);       // 字符串转长整型 string to long
        user["score"] =(Json::UInt64) std::stol(row[1]);    //类型需要强转
        user["total_count"] = std::stoi(row[2]);
        user["win_count"] =std::stoi(row[3]);

        mysql_free_result(res);
        return true;
    }

    bool select_by_name(const std::string &name, Json::Value &user)
    {
#define USER_BY_NAME "select id , score ,total_count,win_count from user where username ='%s';"
        char sql[4096] = {0};
        sprintf(sql, USER_BY_NAME, name.c_str());
        MYSQL_RES *res = NULL;
        { // 花括号形成局部作用域，锁在局部空间内管理，出了作用域锁自动解开
            std::unique_lock<std::mutex> lock(_mutex);
            bool ret = mysql_util::mysql_exec(_mysql, sql); // 执行语句
            if (ret == false)
            {
                DLOG("get user by name  failed\n");
                return false;
            }
            // 有数据也只能有一条数据
            res = mysql_store_result(_mysql); // 结果保存到本地
            if (res == NULL)
            {
                DLOG("have no user info");
                return false;
            }
        }
        int row_num = mysql_num_rows(res); // 获取结果条目数量
        if (row_num != 1)
        {
            DLOG("user information is not unique");
            return false;
        }
        MYSQL_ROW row = mysql_fetch_row(res); // 唯一就取出数据 遍历结果集 放到row
        user["id"] = (Json::UInt64)std::stol(row[0]);       // 字符串转长整型 string to long
        user["username"] = name;
        user["score"] = (Json::UInt64)std::stol(row[1]);
        user["total_count"] = std::stoi(row[2]);
        user["win_count"] = std::stoi(row[3]);

        mysql_free_result(res);
        return true;
    }

    bool select_by_id(uint64_t id, Json::Value &user)
    {
#define USER_BY_ID "select username , score ,total_count,win_count from user where id = %d;"
        char sql[4096] = {0};
        sprintf(sql, USER_BY_ID, id);
        MYSQL_RES *res = NULL;
        { // 花括号形成局部作用域，锁在局部空间内管理，出了作用域锁自动解开
            std::unique_lock<std::mutex> lock(_mutex);
            bool ret = mysql_util::mysql_exec(_mysql, sql); // 执行语句
            if (ret == false)
            {
                DLOG("get user by id  failed\n");
                return false;
            }
            // 有数据也只能有一条数据
            res = mysql_store_result(_mysql); // 结果保存到本地
            if (res == NULL)
            {
                DLOG("have no user info");
                return false;
            }
        }
        int row_num = mysql_num_rows(res); // 获取结果条目数量
        if (row_num != 1)
        {
            DLOG("user information is not unique");
            return false;
        }
        MYSQL_ROW row = mysql_fetch_row(res); // 唯一就取出数据 遍历结果集 放到row
        user["id"] = (Json::UInt64)id;                      // 字符串转长整型 string to long
        user["username"] = row[0];
        user["score"] = (Json::UInt64)std::stol(row[1]);
        user["total_count"] = std::stoi(row[2]);
        user["win_count"] = std::stoi(row[3]);

        mysql_free_result(res);
        return true;
    }

    bool win(uint64_t id)
    {
        // 胜利时加30
#define USER_WIN "update user set score = score+30 ,total_count=total_count+1,win_count=win_count+1 where id = %d;"
        char sql[4096] = {0};
        sprintf(sql, USER_WIN, id);
        bool ret = mysql_util::mysql_exec(_mysql, sql); // 执行语句
        if (ret == false)
        {
            DLOG("update win user info  failed\n");
            return false;
        }

        return true;
    }

    bool lose(uint64_t id)
    {
#define USER_LOSE "update user set score = score-30 ,total_count=total_count+1 where id = %d;"
        char sql[4096] = {0};
        sprintf(sql, USER_LOSE, id);
        bool ret = mysql_util::mysql_exec(_mysql, sql); // 执行语句
        if (ret == false)
        {
            DLOG("update lose user info  failed\n");
            return false;
        }

        return true;
    }

private:
    MYSQL *_mysql;
    std::mutex _mutex;
};

#endif