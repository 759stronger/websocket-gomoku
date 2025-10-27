#include <iostream>
#include <string>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>  //非ssltls加密的框架


typedef  websocketpp::server<websocketpp::config::asio>  wsserver_t; //表示使用 WebSocketpp库创建一个 WebSocket 服务器，底层使用 Boost 库的 Asio 模块进行异步 I/O。这个服务器可以用于处理 WebSocket 连接，进行双向通信等操作。

 void print(const std::string &str)
 {
    std::cout<<str<<std::endl;
 }  

void http_callback(wsserver_t *srv,websocketpp::connection_hdl hdl) //http的回调函数;connection_hdl 是 WebSocket 库中表示连接的类型或数据结构。在 WebSocket 库中，连接句柄（connection handle）通常是一个用来唯一标识连接的对象或类型。
{ 
     //给客户端返回一个helloworld页面
     wsserver_t::connection_ptr conn = srv->get_con_from_hdl(hdl); //获取通信连接对象;定义了一个类型为 connection_ptr 的指针，它指向 WebSocket 服务器中的连接对象。通常，connection_ptr 是一个指向 WebSocket 连接的智能指针，用于管理和操作连接对象。
     std::cout<<"body:"<<conn->get_request_body()<<std::endl;      //请求正文
     websocketpp::http::parser::request req = conn->get_request(); //请求方法怎么做  获取请求信息的对象
     std::cout<<"method:"<<req.get_method()<<std::endl;           //请求方法
     std::cout<<"uri:"<<req.get_uri()<<std::endl;                 //ur路径

     std::string body = "<html><body><h1>hello world</h1></body></html> ";
     //conn->set_body(body);     //进行响应 设置，把body设置进去
     //conn->append_header("Content-type","text/html");        //添加一个头部字段

     conn->set_body(conn->get_request_body());
     conn->set_status(websocketpp::http::status_code::ok);    //设置响应状态码

     wsserver_t::timer_ptr tp = srv->set_timer(5000,std::bind(print,"定时器的使用"));
     tp ->cancel();  //定时任务的取消 导致定时任务立即执行
}
void wsopen_callback(wsserver_t *srv,websocketpp::connection_hdl hdl) //
{
    std::cout<<"websocket握手成功\n";
}
void wsclose_callback(wsserver_t *srv,websocketpp::connection_hdl hdl) //
{
    std::cout<<"websocket连接断开\n";
}
void wsmsg_callback(wsserver_t *srv, websocketpp::connection_hdl hdl, wsserver_t:: message_ptr msg) //有两个参数
{
   wsserver_t::connection_ptr conn = srv->get_con_from_hdl(hdl);   //首先把通信连接对象取出来
   std::cout<<"wmsg:"<<msg->get_payload()<<std::endl;  //打印获取的数据
   std::string rsp = "client say:" + msg->get_payload();
   conn->send(rsp , websocketpp::frame::opcode::text);   //返回的数据 和类型 类型默认 就不用填
}


int main()
{
    //1.实例化server对象."实例化" 是创建类的具体对象的过程。类可以看作是对象的蓝图或模板，它定义了一组属性（数据）和方法（函数）。通过实例化，我们根据这个蓝图创建一个具体的对象，这个对象拥有类中定义的属性和方法。
    wsserver_t wssrv;
    
    //2.设置日志等级
    wssrv.set_access_channels(websocketpp::log::alevel::none);

    //3.初始化asio调度器
    wssrv.init_asio();   //初始化 Asio 异步 I/O 的一部分
    wssrv.set_reuse_addr(true);  //地址重用，是一种配置套接字选项的方法,设置了一个选项，使得在服务器意外终止后，你可以立即重新绑定到相同的 IP 地址和端口。这对于开发网络服务器非常有用。

    //4.设置回调函数
    wssrv.set_http_handler(std::bind(http_callback,&wssrv,std::placeholders::_1));  //一旦事件被触发就会调用函数 调用的时候就会自动传入触发的对应链接以及数据
    wssrv.set_open_handler(std::bind(wsopen_callback,&wssrv,std::placeholders::_1));  //也可以使用bind绑定的方式,带一个预留参数
    wssrv.set_close_handler(std::bind(wsclose_callback,&wssrv,std::placeholders::_1));
    wssrv.set_message_handler(std::bind(wsmsg_callback,&wssrv,std::placeholders::_1,std::placeholders::_2));
   
    //5.设置监听端口
    wssrv.listen(8085); 

    //6.开始获取新连接
    wssrv.start_accept();

    //7.启动服务器
    wssrv.run();
    
    return 0;
}