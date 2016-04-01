#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "http.h"
#include "md5.h"
/*************************************************

sql注入测试语句，每个参数附加一次测试

*************************************************/
#define righttest "%27%20and%201=1%20and%20%271%27=%271"
#define wrongtest "%27%20and%201=2%20and%20%271%27=%271"
/*************************************************/


//返回chr在str中出现的次数
int  chrinstr(char* str,char chr)
{
  int i=0;
  char * p=str;
  char * tail=str+strlen(str);
  while(*p&&p<tail)
  	{
  	 if(*p==chr)
  	 	{
  	 	i++;
  	 	}
	 p++;
  	}
  return i;
}

struct sql_injection_result ** create_sql_injection_result_list(int nums)
{
	struct sql_injection_result **result=(struct sql_injection_result **)malloc(sizeof(struct sql_injection_result **)*nums);
   for(int i=0;i<nums;i++)
{
	result[i]=(struct sql_injection_result *)malloc(sizeof(struct sql_injection_result));
	result[i]->isok=0;
	result[i]->len=0;
	result[i]->url=NULL;
	result[i]->query=NULL;
	result[i]->result=NULL;
}
return result;
}

void free_sql_injection_result_list(struct sql_injection_result ** result,int nums)
{
	 for(int i=0;i<nums;i++)
{
	if(result[i]->url)
{
	free(result[i]->url);
		result[i]->url=NULL;
}
	if(result[i]->query)
{
	free(result[i]->query);
		result[i]->query=NULL;
}
	if(result[i]->result)
{
	free(result[i]->result);
		result[i]->result=NULL;
}
	free(result[i]);
	result[i]=NULL;
}
	free(result);
	result=NULL;
}
//解析url中的请求参数
struct url_query * create_url_query(char * url)
{

 char* query_star=NULL;
  if(!(query_star=strchr(url,'?')))
  	{
  	return NULL;
  	}
  query_star++;
  struct url_query *query_info=NULL;
  query_info=(struct url_query *)malloc(sizeof(struct url_query));
  query_info->url=(char*)malloc(strlen(url)+1);
  strcpy(query_info->url,url);
  int query_num=chrinstr(url,'&');
  query_info->query_nums=query_num+1;

  query_info->query_node=(struct url_query_node**)malloc(sizeof(struct url_query**)*(query_num+1));
  for(int i=0;i<=query_num;i++)
{
	query_info->query_node[i]=(struct url_query_node*)malloc(sizeof(struct url_query_node));
}
 char * p=url;
  char * tail=url+strlen(url);
  int i=0,n=1;
  query_info->query_node[0]->value_star=query_star-url;
  while(*p&&p<tail)
  	{
  	 if(*p=='&')
  	 	{
  	 	query_info->query_node[n-1]->value_end=i-1;
		query_info->query_node[n]->value_star=i+1;
		n++;
  	 	}
	 i++;
	 p++;
  	}
    query_info->query_node[n-1]->value_end=i;
  	
  return query_info;
  
}

void free_url_query(struct url_query * url_query)
{
  for(int i=0;i<url_query->query_nums;i++)
  	{
  	free(url_query->query_node[i]);
  	url_query->query_node[i]=NULL;
  	}
  free(url_query->query_node);
  free(url_query->url);
  free(url_query);
  url_query->query_node=NULL;
  url_query->url=NULL;
  url_query=NULL;
}
void check_sql_injection(char * url,int flag_method,int flag_cookie)
{
  struct url_query * url_query=create_url_query(url);
  if(NULL==url_query)
   {
   return;
   }
    char *uri=(char*)malloc(url_query->query_node[0]->value_star);
   struct sql_injection_result **right_result=create_sql_injection_result_list(url_query->query_nums);
  struct sql_injection_result **wrong_result=create_sql_injection_result_list(url_query->query_nums);
   strncpy(uri,url_query->url,url_query->query_node[0]->value_star-1);//减去‘？’占的字符
  uri[url_query->query_node[0]->value_star-1]=0;
  for(int i=0;i<url_query->query_nums;i++)
  {
      right_result[i]->url= (char*)malloc(strlen(url_query->url)+strlen(righttest)+1);
      wrong_result[i]->url= (char*)malloc(strlen(url_query->url)+strlen(wrongtest)+1);
      
	  strncpy(right_result[i]->url,url_query->url,url_query->query_node[i]->value_end+1);
	  strcat(right_result[i]->url,righttest);
	  strcat(right_result[i]->url,url_query->url+url_query->query_node[i]->value_end+1); //设置正确测试完整语句，GET方法使用
	  
	   strncpy(wrong_result[i]->url,url_query->url,url_query->query_node[i]->value_end+1);
	  strcat(wrong_result[i]->url,wrongtest);
	  strcat(wrong_result[i]->url,url_query->url+url_query->query_node[i]->value_end+1); //设置错误测试完整语句，GET方法使用
	  
	  right_result[i]->query=(char*)malloc(strlen(url_query->url)-strlen(uri)+strlen(righttest));
	  wrong_result[i]->query=(char*)malloc(strlen(url_query->url)-strlen(uri)+strlen(wrongtest));
	  
	  strncpy(right_result[i]->query,right_result[i]->url+url_query->query_node[0]->value_star,strlen(right_result[i]->url)-strlen(uri));
	  right_result[i]->query[strlen(url_query->url)-strlen(uri)+strlen(righttest)-1]=0; //提取正确测试参数，POST方法使用
	  
	  strncpy(wrong_result[i]->query,wrong_result[i]->url+url_query->query_node[0]->value_star,strlen(wrong_result[i]->url)-strlen(uri));
	  wrong_result[i]->query[strlen(url_query->url)-strlen(uri)+strlen(wrongtest)-1]=0;//提取错误测试参数，POST方法使用
	 

	if(flag_method) //post方法
	{
	  if(right_result[i]->result=get_http_form(uri,right_result[i]->query,NULL,NULL,flag_cookie))
	{
		right_result[i]->isok=1;
		right_result[i]->len=strlen(right_result[i]->result);
	}
		  if(wrong_result[i]->result=get_http_form(uri,wrong_result[i]->query,NULL,NULL,flag_cookie))
	{
		wrong_result[i]->isok=1;
		wrong_result[i]->len=strlen(wrong_result[i]->result);
	}
	}
	else    //get方法
    {
	if(right_result[i]->result=get_http_content(right_result[i]->url,flag_cookie))
     {
     	 	right_result[i]->isok=1;
		right_result[i]->len=strlen(right_result[i]->result);	
     }
     if(wrong_result[i]->result=get_http_content(wrong_result[i]->url,flag_cookie))
   {
 	 	wrong_result[i]->isok=1;
		wrong_result[i]->len=strlen(wrong_result[i]->result);
    }

   }
   if(right_result[i]->isok&&wrong_result[i]->isok)
     {
	  int num=right_result[i]->len>wrong_result[i]->len?right_result[i]->len-wrong_result[i]->len:wrong_result[i]->len-right_result[i]->len;
	   if(num>=20)
	{
		printf("this is a sql injection point ,the test url is%s",right_result[i]->url);
	}
    }
}
free(uri);
uri=NULL;
free_sql_injection_result_list(right_result,url_query->query_nums);
free_sql_injection_result_list(wrong_result,url_query->query_nums);
free_url_query(url_query);
  return ;
}

int main(int argc,char ** argv)
{
  check_sql_injection(argv[1],0,1);

  return 0;
}
