#ifndef __M_LOGGER_H__    //避免头文件重复包含
#define __M_LOGGER_H__
#include <stdio.h>
#include <time.h>

#define INF 0
#define DBG 1
#define ERR 2
#define DEFAULT_LOG_LEVEL INF   //默认的日志等级


#define LOG(level,format,...)  do{\
if(DEFAULT_LOG_LEVEL> level) break;\
time_t t =time(NULL);\    
struct tm *lt = localtime(&t);\
char buf[32]={0};\
strftime(buf,31,"%H:%M:%S",lt);\
fprintf(stdout,"[%s %s:%d] " format "\n",buf, __FILE__,__LINE__,##__VA_ARGS__);\
}while(0)
//...表示不定参  stdout固定向标准输出里面输出数据 按format的格式 
//日志不想直接打印 想打入文件 可以把stdout替换成文件指针
//%S%D 表示文件名和行号 文件名__FILE__  行号__LINE__
//在__VA_ARGS__前面加两个##可以解决如果只有字符串时的报错问题
//format后面加\n可以自动换行
//可以调整日志输出的等级 指定多少等级以上的日志可以输出

#define ILOG(format,...)  LOG(INF,format,##__VA_ARGS__)  //宏函数传两个信息  传入日志等级
#define DLOG(format,...)  LOG(DBG,format,##__VA_ARGS__)
#define ELOG(format,...)  LOG(ERR,format,##__VA_ARGS__)


#endif