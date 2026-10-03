#ifndef __M_UTIL_H__
#define __M_UTIL_H__
#include "logger.hpp"
#include <iostream>
#include <string>
#include <mysql/mysql.h>
#include <memory>
#include <jsoncpp/json/json.h>
#include <sstream>
#include <vector>
#include <fstream>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
typedef  websocketpp::server<websocketpp::config::asio>  wsserver_t; 



class mysql_util
{
public: // 不需要成员变量，是为了向外提供接口，也不依赖什么成员  而且是静态的 外界可以直接使用，不需要实例化对象
    static MYSQL *mysql_create(const std::string &host,
                               const std::string &username,
                               const std::string &password,
                               const std::string &dbname,
                               uint16_t port = 3306)
    { // 端口可以默认3306 也可以给0，默认也使用3306
        // 1.初始化mysql句柄
        // MYSQL *mysql_intit(MYSQL*mysql)
        MYSQL *mysql = mysql_init(NULL);
        if (mysql == NULL)
        {
            ELOG("mysql init failed");
            return NULL;
        }

        // 2.连接服务器
        // MYSQL *mysql_real_connect(mysql,host,usr,passwd,db,port,unix_socket,flag)     参数都是string，但是接口里面要的都是c语言风格的字符串 所以要用c_str
        if (mysql_real_connect(mysql, host.c_str(), username.c_str(), password.c_str(), dbname.c_str(), port, NULL, 0) == NULL)
        {
            ELOG("connect mysql server failed:%s", mysql_error(mysql));
            mysql_close(mysql);
            return NULL;
        }

        // 3.设置客户端字符集
        // int mysql_set_character(mysql,"utf8")
        if (mysql_set_character_set(mysql, "utf8") != 0)
        {
            ELOG("set client character failed:%s", mysql_error(mysql));
            mysql_close(mysql);
            return NULL;
        }

        // 4.选择要操作的数据库
        // int mysql_select_db(mysql,dbname)
        // mysql_select_db(mysql,DBNAME);  已经默认了

        return mysql;
    }
    // 句柄，sql语句
    static bool mysql_exec(MYSQL *mysql, const std::string &sql)
    {
        int ret = mysql_query(mysql, sql.c_str()); // 不能直接使用sql 因为sql是个string 所用要用c_str()
        if (ret != 0)
        {
            ELOG("%s\n", sql.c_str());
            ELOG("mysql query failed:%s\n", mysql_error(mysql));
            return false;
        }
        return true;
    }

    static void mysql_destroy(MYSQL *mysql) // 销毁句柄
    {
        if (mysql != NULL)
            mysql_close(mysql);
        return;
    }
};

// 封装json的序列化和反序列化
class json_util
{
public:
    static bool serialize(const Json::Value &root, std::string &str)
    {
        // 2.实例化一个streamwriterbuilder工厂类对象,工厂模式,通过实例化StreamWriterBuilder，你可以更灵活地控制StreamWriter的创建过程，以满足你在处理数据流时的具体需求。
        Json::StreamWriterBuilder swb;

        // 3.通过streamwriterbuilder工厂类对象生产一个streamwriter对象,StreamWriter则用于将文本数据写入流中
        std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter()); // 用智能指针来管理new出来的对象

        // 4.使用streamwriter对象，对json::value中存储的数据进行序列化
        std::stringstream ss;
        int ret = sw->write(root, &ss);
        if (ret != 0)
        {
            ELOG("json serialize failed\n");

            return false;
        }
        str = ss.str();
        return true;
    }
    static bool unserialize(const std::string &str, Json::Value &root)
    {
        // 1.实例化一个charreaderbuilder工厂类对象
        Json::CharReaderBuilder crb;

        // 2.使用charreaderbuilder工厂类生产一个charreader对象
        std::unique_ptr<Json::CharReader> cr(crb.newCharReader());

        std::string err;
        // 4.使用charreader对象进行json格式字符串str的反序列化
        // parse(char* start ,char *end, json::value *val , string *err)
        bool ret = cr->parse(str.c_str(), str.c_str() + str.size(), &root, &err);
        if (ret == false)
        {
            ELOG("json unserialize failed:%s ", err.c_str());
            return false;
        }
        return true;
    }
};

// 字符串分割工具
class string_util
{
public: // src原字符串 sep分隔符 res存放  使用特定字符对字符串进行分割，各个子串存放到数组中
    static int split(const std::string &src,
                     const std::string &sep,
                     std::vector<std::string> &res)
    {
        size_t pos, idx = 0;
        while (idx < src.size())
        {
            pos = src.find(sep, idx); // idx下标开始找
            if (pos == std::string::npos)
            {
                // 没有找到 意味着就一个字串 没有间隔字符
                res.push_back(src.substr(idx));
                break;
            }
            if (pos == idx) // 如果分隔符连续了 就越过分隔符
            {
                idx += sep.size();
                continue;
            }
            // 找到了   idx是起始位置 pos就是找到的位置
            res.push_back(src.substr(idx, pos - idx));
            idx = pos + sep.size();
        }
        return res.size();
    }
};

// 文件数据的读取 html文件
class file_util
{
public: // 给文件名找文件  再读取 再存入string body中
    static bool read(const std::string &filename, std::string &body)
    {
        // 1.打开文件
        std::ifstream ifs(filename, std::ios::binary); // 以二进制的形式打开文件
        if (ifs.is_open() == false)
        {
            ELOG("%s file open falied", filename.c_str());
            return false;
        }

        // 2.获取文件大小   特殊操作：把一个读取位置移到末尾 就可以根据偏移量获得
        size_t fsize = 0;             
        ifs.seekg(0, std::ios::end);   //跳转到文件末尾 偏移量为0
        fsize = ifs.tellg();           //获取当前读写位置相对于起始位置的偏移量
        ifs.seekg(0, std::ios::beg);   //把读写位置再放到起始位置
        body.resize(fsize);            //调整string对象的空间大小 为文件大小

        // 3.读取文件数据             //从第0个的地址开始访问
        ifs.read(&body[0], fsize);    //不能用c_str 因为c_str返回的是const 不允许被修改
        if (ifs.good() == false)      //good就是判断上次操作是否ok
        {
            ELOG("read %s file contect failed ", filename.c_str());
            ifs.close();
            return false;
        }


        // 4.关闭文件
        ifs.close();
        return true;
    }
};



#endif