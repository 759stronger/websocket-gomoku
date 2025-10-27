#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <jsoncpp/json/json.h>

//使用jsoncpp库进行多个数据对象的序列化
std::string serialize(){
    //1.将需要进行序列化的数据,存储在json::value对象中
    Json::Value root;
    root["姓名"]="小明";
    root["年龄"]=18;
    root["成绩"].append(98);
    root["成绩"].append(88.5);
    root["成绩"].append(78.5);  //数组用append添加

    //2.实例化一个streamwriterbuilder工厂类对象,工厂模式,通过实例化StreamWriterBuilder，你可以更灵活地控制StreamWriter的创建过程，以满足你在处理数据流时的具体需求。
    Json::StreamWriterBuilder  swb;

    //3.通过streamwriterbuilder工厂类对象生产一个streamwriter对象,StreamWriter则用于将文本数据写入流中
    Json:: StreamWriter * sw = swb.newStreamWriter();
    
    //4.使用streamwriter对象，对json::value中存储的数据进行序列化
    std::stringstream ss;
    int ret=  sw->write(root,&ss);
    if(ret !=0)
    {
        std::cout<<"json serialize failed\n"<<std::endl;
        return "";
    }
    std::cout<<ss.str()<<std::endl;
    delete sw;
    return ss.str();
}

void unserialize(const std::string &str)
{
    //1.实例化一个charreaderbuilder工厂类对象
    Json::CharReaderBuilder crb;
    
    //2.使用charreaderbuilder工厂类生产一个charreader对象
   Json::CharReader *cr =crb.newCharReader();
   
    //3.定义一个json::value对象存储解析后的数据
   Json::Value root;
   std::string err;
    //4.使用charreader对象进行json格式字符串str的反序列化
   //parse(char* start ,char *end, json::value *val , string *err)
   bool ret = cr->parse(str.c_str(), str.c_str()+str.size(),&root,&err);
   if(!ret)
   {
       std::cout<<"json unserialize failed:\n"<<err <<std::endl;
       return ;    
   }
    //5.逐个元素去访问json::value中的数据
    std::cout<<"姓名："<< root["姓名"].asString()<<std::endl;  //不能直接访问，需要调用asString
    std::cout<<"年龄："<< root["年龄"].asInt()<<std::endl;  
    int sz = root["成绩"].size();

    for(int i=0;i<sz;i++)
    {
         std::cout<<"成绩："<<root["成绩"][i].asFloat()<<std::endl;
    }
    delete cr;
}


int main ()
{
    std::string str =serialize();
    unserialize(str);

    return 0;
} 