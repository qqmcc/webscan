#ifndef CHECK_H
#define CHECK_H


//该结构用于sql注入测试解析各个参数
struct url_query
{
  char * url;    //URL名称
  int query_nums; //参数数目
  struct url_query_node ** query_node; //分割出来的参数结构指针数组
};

//该结构用于sql注入测试解析各个参数在URL中的偏移
struct url_query_node                
{
  int value_star;                    //参数在string中的起始偏移，从0计数
  int value_end;                     //参数在string中的结束偏移，从0计数
};

//该结构用于sql注入测试保存返回结果
struct sql_injection_result
{
    int isok; //连接是否成功
	int len;  //返回内容长度
	char *url;//生成的新的请求地址的全路径
	char *query;//请求参数
	char* result; //返回内容
};
#endif
