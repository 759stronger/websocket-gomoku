#include "server.hpp"

#define HOST "127.0.0.1" // mysql地址  用本地登录 默认不允许进行远程登陆
#define USER "root"
#define PASS "d#XhdU4aIT):"
#define DBNAME "gobang"
#define PORT 3306

void mysql_test()
{
    MYSQL *mysql = mysql_util::mysql_create(HOST, USER, PASS, DBNAME, PORT); // 创建句柄
    const char *sql = "insert stu values(null,'哈哈',32,54,23,43);";
    bool ret = mysql_util::mysql_exec(mysql, sql);

    if (ret == false)
    {
        return;
    }

    mysql_util::mysql_destroy(mysql);
}

void json_test()
{
    Json::Value root;
    std::string body;
    root["姓名"] = "小黑";
    root["年龄"] = 18;
    root["成绩"].append(98);
    root["成绩"].append(88.5);
    root["成绩"].append(78.5);
    json_util::serialize(root, body);
    DLOG("%s", body.c_str());

    Json::Value val;
    json_util::unserialize(body, val);
    std::cout << "姓名：" << val["姓名"].asString() << std::endl; // 不能直接访问，需要调用asString
    std::cout << "年龄：" << val["年龄"].asInt() << std::endl;
    int sz = val["成绩"].size();

    for (int i = 0; i < sz; i++)
    {
        std::cout << "成绩：" << val["成绩"][i].asFloat() << std::endl;
    }
}

void str_test()
{
    std::string str = "34,,...,,,24234,..2345,,,342";
    std::vector<std::string> arry;
    string_util::split(str, ",", arry);
    for (auto s : arry)
    {
        DLOG("%s", s.c_str());
    }
}

void file_test()
{
    // 读makefile

    std::string filename = "./makefile";
    std::string body;
    file_util::read(filename, body);

    std::cout << body << std::endl;
}


void db_test()
{
    user_table ut(HOST, USER, PASS, DBNAME, PORT);
    Json::Value user;
    user["username"]="弟弟12";
    //user["password"]="987563";
     ut.insert(user);
   // std::string body;
   // json_util::serialize(user,body);
    //std::cout<< body << std::endl;
}
void onlinetest()
{
   online_manager om;
   wsserver_t::connection_ptr conn;//空的智能指针对象
   uint64_t uid =2;
   om.enter_game_room(uid,conn);
   if(om.is_in_gameroom(uid))
   {
    DLOG("in gamerooom");
   }
   else
   {
    DLOG("not in gamerooom");
   }
   om.exit_game_room(uid);
   if(om.is_in_gameroom(uid))
   {
    DLOG("in gamerooom");
   }
   else
   {
    DLOG("not in gamerooom");
   }
}
void room_test()
{
     user_table ut(HOST, USER, PASS, DBNAME, PORT);
     online_manager om;
    room r(10,&ut,&om);
}
void room_manager_test()
{
    
}

void session_manager()
{
    
}

int main()
{
    // ILOG("哈哈哈");//格式化数据然后进行输出
    // DLOG("哈哈");
    // ELOG("哈");
    // json_test();
    // str_test();
    //onlinetest();
    // room_manager_test();
    
    gobang_server _server(HOST, USER, PASS, DBNAME, PORT);
    _server.start(8085);


    return 0;
}

