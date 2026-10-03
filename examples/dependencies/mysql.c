#include <stdio.h>
#include <string.h>
#include <mysql/mysql.h>
#include "gomoku/config.h"
static GomokuDbConfig database_config = {0};
#define HOST database_config.host
#define USER database_config.user
#define PASS database_config.password
#define DBNAME database_config.database
#define PORT database_config.port



int main()
{
    const GomokuConfigStatus config_status = gomoku_db_config_from_env(&database_config);
    if (config_status != GOMOKU_CONFIG_OK)
    {
        fprintf(stderr, "Database configuration rejected (%d). Set GOMOKU_DB_USER/GOMOKU_DB_PASSWORD and use a port in 1..65535.\n", (int)config_status);
        return 1;
    }

    //1.初始化mysql句柄
    //MYSQL *mysql_intit(MYSQL*mysql)
    MYSQL *mysql = mysql_init(NULL);
    if(mysql == NULL)
    {
        printf("mysql init failed\n");
        return -1;
    }

    //2.连接服务器
    //MYSQL *mysql_real_connect(mysql,host,usr,passwd,db,port,unix_socket,flag)
    if(mysql_real_connect(mysql,HOST,USER,PASS,DBNAME,PORT,NULL,0)==NULL)
    {
        printf("connect mysql server failed:%s\n",mysql_error(mysql));
        mysql_close(mysql);
        return -1;
    }


    //3.设置客户端字符集
    //int mysql_set_character(mysql,"utf8") 
    if(mysql_set_character_set(mysql,"utf8")!=0)
    {
        printf("set client character failed:%s\n",mysql_error(mysql));
        mysql_close(mysql);
        return -1;
    }


    //4.选择要操作的数据库
    //int mysql_select_db(mysql,dbname)
    //mysql_select_db(mysql,DBNAME);  已经默认了


    //5.执行sql语句
    //int mysql_query(MYSQL *mysql,char *sql)
    //char *sql = "insert stu values(null,'小明',32,54,23,43);";  插入
    //char *sql = "update stu set ch = ch+40 where sn=1;";    更新
    // char *sql = "delete from  stu where sn=1;";        删除
    char *sql = "select * from stu;";
    int ret =mysql_query(mysql,sql);
    if(ret!= 0)
    {
        printf("%s\n",sql);
        printf("mysql query failed:%s\n",mysql_error(mysql));
        mysql_close(mysql);
        return -1;
    } 


    //6.如果是查询，保存结果到本地
    //MYSQL_RES * mysql_store_result(MYSQL * mysql)
    MYSQL_RES *res =mysql_store_result(mysql);
    if(res==NULL)
    {
        mysql_close(mysql);
        return -1;
    }


    //7.获取结果集中的结果条数
    //int mysql_num_rows(MYSQL_RES *res)  行数
    //int mysql_num_fields(MYSQL_RES *res)  列数
     int num_rows =mysql_num_rows(res);
     int num_col =mysql_num_fields(res);

    //8.遍历保存到本地的结果集
    for(int i =0; i <num_rows;i++)    //一次取出的是一行
    {
        //MYSQL_ROW mysql_fetch_row(MYSQL_RES *res)  这个是遍历结果  结果集
        MYSQL_ROW row =mysql_fetch_row(res);
        for(int i =0;i<num_col;i++)
        {
            printf("%s\t",row[i]);    //默认字符串保存 需要数字 就转化
        }
        printf("\n");
    }
 


    //9.释放结果集
     mysql_free_result(res);
   
    //10.关闭连接，释放句柄
    mysql_close(mysql);
    return 0;
}