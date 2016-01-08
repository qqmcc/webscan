
#include<stdio.h>
#include<sys/stat.h>
#include<unistd.h>
#include<string.h>
#include<gumbo.h>
#include<gumbo_libxml.h>
#include<libxml/tree.h>
#include<pthread.h>
#include<pcre.h>
#include "http.h"
#include "threadpool.h"
#include "htmlparse.h"
#include "md5.h"
#include "list.h"


struct thread_pool *parseurl_thread_pool=NULL;
struct complete_node **complete_hash=NULL; //malloc 0x100000
pthread_rwlock_t  *complete_hash_rwlock=NULL; //malloc 0x100000
int url_count=0;
unsigned int cfg_process_parseurl_thread_count=10;
unsigned int cfg_process_parseurl_queue_size=100000;


struct list_head Lpagefinish;  // 已处理的页面队列，用于网页去重
struct list_head* PLpagefinish;


struct list_head Lfinish;  // 已完成队列
struct list_head* PLfinish;

struct list_head Lerror;  // 错误队列
struct list_head* PLerror;

char hostofscan[1024];

xmlChar* check_treeforlink(const xmlChar *pattern,xmlNode *xmlnode)
{
    xmlChar * url=NULL;
	if(!xmlStrcmp(pattern,(const xmlChar *)"a"))
		{
		url=xmlGetProp(xmlnode,(const xmlChar*)"href");

		}
	else if(!xmlStrcmp(pattern,(const xmlChar *)"img"))
		{
		url=xmlGetProp(xmlnode,(const xmlChar*)"src");

		}
   return url;
}
int get_match(char *str,char *pattern)
{
  	pcre *re;
	const char *error;
	int erroffset;
	int ovector[30];
	re=pcre_compile(pattern,PCRE_DOTALL|PCRE_MULTILINE,&error,&erroffset,NULL);
	if(re==NULL)
	{
		//printf("PCRE compilation failed at offset %d :%s\n",erroffset,error);
		return 0;
	}
	int rc = pcre_exec(re, NULL, str, strlen(str), 0, 0, ovector, 30);
	if(rc<0)
	{
	//	printf("matching error\n");
		free(re);
		return 0;
	}
	return 1;
}
void complete_hash_init()
{
	 //catch_node**指针数组0x100000个
	if (complete_hash==NULL) complete_hash = (struct complete_node **)malloc(sizeof(struct complete_node *)*0x100000);
	//初始化读写锁pthread_rwlock_t*指针数组0x100000个
	if (complete_hash_rwlock==NULL) complete_hash_rwlock = (pthread_rwlock_t *)malloc(sizeof(pthread_rwlock_t)*0x100000);
	for(uint32_t i=0; i<=0xfffff; i++){
		complete_hash[i] = NULL;
		//读写锁初始化
		pthread_rwlock_init(&complete_hash_rwlock[i], NULL);
	}

}
//返回complete_node结构指针 需要手动释放
struct complete_node* create_complete_node(uint8_t *md5)
{
		struct complete_node *node=(struct complete_node *)malloc(sizeof(struct complete_node));
		memcpy(node->md5, md5,16);
		node->next=NULL;
		return node;
}
//将得到的URL加入完整队列，添加成功返回1 失败（已经存在）返回0
int update_complete_node(uint8_t *md5)
{
	uint32_t index = *((uint32_t*)md5) & 0xfffff;
	pthread_rwlock_wrlock(&complete_hash_rwlock[index]);
		struct complete_node *node;
	struct complete_node *p=complete_hash[index];
	if (p==NULL)
    {
    	node=create_complete_node(md5);
    	complete_hash[index]=node;
		pthread_rwlock_unlock(&complete_hash_rwlock[index]);
    	return 1;
    }
    else
{
		struct complete_node *q=p;
		while(p&&memcmp(p->md5,md5,16))
	{
		q=p;
		p=p->next;
	}
	if(NULL==p)
     {
	     node=create_complete_node(md5);
		 q->next=node;
    }
    else
    {
    pthread_rwlock_unlock(&complete_hash_rwlock[index]);
	return 0;
    }
	pthread_rwlock_unlock(&complete_hash_rwlock[index]);
	return 1;
}
pthread_rwlock_unlock(&complete_hash_rwlock[index]);
	
}
static void print_element_names(xmlNode * a_node)
{
    xmlNode *cur_node = NULL;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
        	if(!xmlStrcmp(cur_node->name,(const xmlChar *)"a"))
        	{
        	xmlChar * url=xmlGetProp(cur_node, (const xmlChar*)"href");
            printf("node type: Element, name: %s\n",url);
            }
        }

        print_element_names(cur_node->children);
    }
}

//获取host以判断扫描的url域
int gethost(char * host,const char * url)
{
	if(NULL==url)
{
	return 0;
}
  char * p=strstr(url,"//");
  if(p)
  {
  p+=2;
  int i=p-url;
  strncpy(host,url,p-url);
  while(*p&&*p!='/')
  {
 host[i]=*p;
i++;
  p++;
  }
  host[i]=0;
 }
 else
{
	strcpy(host,"http://");
	int i=7;
	p=(char *)url;
	
	while(*p&&*p!='/')
  {
 host[i]=*p;
i++;
  p++;
  }
  host[i]=0;
}
return 1;
}

/****************************************************************
*

*    函数名：isurl

*

*    参数：

*         char* url 输入URL

*    

*    功能描述：

*            判断HTML A标签中的URL是否为特殊情况
             
*

*    返回值：不为特殊情况返回1,是特殊返回0
           

*    
******************************************************************/
int isurl(char * url)
{
	if (0==strcmp(url,".")|\
		0==strcmp(url,"?")|\
		0==strcmp(url,"")|\
		0==strcmp(url,"#")|\
		0==strncmp(url,"javascript",10)|\
		0==strncmp(url,"mailto",6)|\
		0==strcmp(url,"\\")|\
		0==strcmp(url,"../"))
   {
 	return 0;
   }
    return 1;
}

/****************************************************************
*

*    函数名：isbin

*

*    参数：

*         char* url 输入URL

*    

*    功能描述：

*            判断URL是否为请求文件
             
*

*    返回值：是文件返回1,不是返回0
           

*    

******************************************************************/
int isbin(char * url)
{	
char* sig[30]={".jpg", ".jpeg", ".bmp", ".png", ".gif", ".ico",
		".rar", ".7z", ".zip", ".tar", ".bz2", ".tar",
		".bz2",".swf", ".flv", ".mp3", ".mp4", ".mkv",
		".avi", ".wmv", ".wma",".exe", ".msi", ".deb",
		".rpm", ".dmg",".bak", ".sql", ".txt", ".pdf"};
		char *ptr=NULL;
		ptr=strrchr(url,'.');
		if(ptr)
	{
		for(int i=0;i<30;i++)
	{
		if(0==strcmp(ptr,sig[i]))
	{
		return 1;
	}
	}
	}
	return 0;
}

char * joinurl(char* baseurl,char* reletivepath)
{
       if(0==strncmp(reletivepath,"http:",5)||0==strncmp(reletivepath,"https:",6))//检查URL是否以http开头
		{
		return reletivepath;
		}
		char * url;
		if(0==strncmp(reletivepath,"//",2))
		{
		
		url =(char *)calloc(strlen(reletivepath)+8,sizeof(char));
		char* p=baseurl;
		int i=0;
		while(*p!=':')
			{
			 url[i]=*p;
			 p++;
			 i++;
			}
		url[i]=':';
		strcat(url,reletivepath);
			}
	else if(0==strncmp(reletivepath,"/",1))
	{
       
        char *p=strstr(baseurl,"//");
        p+=2;
		char *plast=strchr(p,'/');
	
		url=(char *)calloc(strlen(baseurl)+strlen(reletivepath)+1,sizeof(char));
			if(NULL==plast)
				{
				sprintf(url,"%s%s",baseurl,reletivepath);
				}
			else
				{
				strncpy(url,baseurl,(plast-baseurl));
				strcat(url,reletivepath);
				}

	}
	else if(0==strncmp(reletivepath,"?",1))
		{
   
         char *p=strstr(baseurl,"?");
			  url=(char *)calloc(strlen(baseurl)+strlen(reletivepath)+2,sizeof(char));
		  if(NULL==p)
			  {
			  char *q=strstr(baseurl,"//");
		   q+=2;
		  char *plast=strrchr(q,'/');
	  
		  if(NULL==plast)
			  {
			  sprintf(url,"%s/%s",baseurl,reletivepath);
			  }
		  else
			  {
			  strncpy(url,baseurl,plast-baseurl+1);
			  strcat(url,reletivepath);
			  }
			  }
		  else
			  {
			   strncpy(url,baseurl,p-baseurl);
			   strcat(url,reletivepath);
			  }
   

		}
	else if(0==strncmp(reletivepath,"./",2))
		{
          reletivepath+=2;
		 char *p=strstr(baseurl,"//");
		 p+=2;
		char *plast=strrchr(p,'/');
		url=(char *)calloc(strlen(baseurl)+strlen(reletivepath)+2,sizeof(char));
		if(NULL==plast)
			{
			sprintf(url,"%s/%s",baseurl,reletivepath);
			}
		else
			{
			strncpy(url,baseurl,plast-baseurl+1);
			strcat(url,reletivepath);
			}
		}
	else if(0==strncmp(reletivepath,"../",3))
		{
          reletivepath+=3;
		 char *p=strstr(baseurl,"//");
		 p+=2;
		char *plast=strrchr(p,'/');
		url=(char *)calloc(strlen(baseurl)+strlen(reletivepath)+2,sizeof(char));
		if(NULL==plast)
			{
			sprintf(url,"%s/%s",baseurl,reletivepath);
			}
		else
			{
				char *q=plast;
			q--;
			while(q>p && *q!='/')
				{
				q--;
				}
			if(q==p)
				{
					strncpy(url,baseurl,plast-baseurl+1);	
						strcat(url,reletivepath);
				}
			else{
			strncpy(url,baseurl,q-baseurl+1);
			strcat(url,reletivepath);
				}
			}
		}
	else
		{
		char *p=strstr(baseurl,"//");
		 p+=2;
		char *plast=strrchr(p,'/');
		url=(char *)calloc(strlen(baseurl)+strlen(reletivepath)+2,sizeof(char));
		if(NULL==plast)
			{
			sprintf(url,"%s/%s",baseurl,reletivepath);
			}
		else
			{
			
			strncpy(url,baseurl,plast-baseurl+1);
			strcat(url,reletivepath);
			
			}
		
		}
		return url;
}


static void geturl(xmlNode * a_node,struct url_node * urlinfo)
{
    xmlNode *cur_node = NULL;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {

			xmlChar * url=NULL;
        	if(url=check_treeforlink(cur_node->name,cur_node))
        	{
        
         //  printf("node type: Element, name: %s\n",url);
              if(!isurl(url))
          {
          	 free(url);
			goto next_childrennode;
          }
            	char* newurl;
       
                  newurl=joinurl(urlinfo->url,url);
				  if(strcmp(newurl,url))
				  	{
				  	free(url);
				  	}

				//  printf("http:%s\n",newurl);
		
            if(!strstr(newurl,hostofscan))
            	{
            //	printf("new http:%s scan:%s\n",newurl,hostofscan);
            	free(newurl);
            	goto next_childrennode;
            	}
            struct url_node * node=(struct url_node *)malloc(sizeof(struct url_node));
             strcpy(node->url,newurl);
             node->deep=urlinfo->deep+1;
             md5(node->md5, newurl,strlen(newurl));
             free(newurl);

            if(1==update_complete_node(node->md5))  //完整队列中无重复，添加到抓取线程池
        {
          //  printf("%d:update:%s\n",url_count,node->url);
           if(isbin(node->url))
           	{
           	url_count++;
           	printf("bin%d:%s\n",url_count,node->url);
			}
		   else{
			add_queue_task(parseurl_thread_pool,node);
		   	}
        }
    }
        }
		next_childrennode:
        geturl(cur_node->children,urlinfo);
    }
}


/* 解析storyinfo节点，打印keyword节点的内容 */  
void parseStory(xmlDocPtr doc, xmlNodePtr cur){  
    xmlChar* key;  
    cur=cur->xmlChildrenNode;  
    while(cur != NULL){  
        /* 找到keyword子节点 */  
        if(!xmlStrcmp(cur->name, (const xmlChar *)"keyword")){  
            key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);  
            printf("keyword: %s\n", key);  
            xmlFree(key);  
        }  
        cur=cur->next; /* 下一个子节点 */  
    }  
  
    return;  
}  
static void read_file(FILE* fp, char** output, int* length) {
  struct stat filestats;
  int fd = fileno(fp);
  fstat(fd, &filestats);
  *length = filestats.st_size;
  *output = malloc(*length + 1);
  int start = 0;
  int bytes_read;
  while ((bytes_read = fread(*output + start, 1, *length - start, fp))) {
    start += bytes_read;
  }
}

void process_parseurl(void *arg)
{
	 struct url_node * urlinfo= (struct url_node *) arg;
	 url_count++;
     printf("%d:%s\n",url_count,urlinfo->url);
	 char * htmlinfo=get_http_content(urlinfo->url);
	 if(NULL==htmlinfo)
	 	{
	 	 printf("get http error:%s\n",urlinfo->url);
		 return;
	 	}
	 xmlDocPtr doc = gumbo_libxml_parse(htmlinfo);
	 free(htmlinfo);
	 if(doc==NULL)
  {
   //  printf("faild to parse %s\n",argv[1]);
     return;
  }
    xmlNodePtr cur=xmlDocGetRootElement(doc);  
    if(cur == NULL){  
        fprintf(stderr, "empty document\n");  
        xmlFreeDoc(doc);  
        return;  
    }  
   //print_element_names(cur);
   geturl(cur,urlinfo);
   free(urlinfo);
   xmlFreeDoc(doc);
  
}
void process_parsepage(void *arg)
{
	
}

void scan_init(const char* url,int deep)
{
        complete_hash_init();
		parseurl_thread_pool=init_thread_pool(cfg_process_parseurl_thread_count,
			   cfg_process_parseurl_queue_size,
		 (void*)process_parseurl);
		
}
int main(int argc, char**argv)
{
	if(argc!=2)
		return 0;
	  char* uri = argv[1];
	  gethost(hostofscan,uri);

/*
  FILE* fp = fopen(filename, "r");
  if (!fp) {
    printf("File %s not found!\n", filename);
    exit(EXIT_FAILURE);
  }

  char* input;
  int input_length;
  xmlNodePtr cur; 
  read_file(fp, &input, &input_length);
  xmlDocPtr doc = gumbo_libxml_parse(input);
if(doc==NULL)
{
 printf("faild to parse %s\n",argv[1]);
 return 0;
}
*/
 /* 获取文档根节点，若无内容则释放文档树并返回 */  
  /*  cur = xmlDocGetRootElement(doc);  
    if(cur == NULL){  
        fprintf(stderr, "empty document\n");  
        xmlFreeDoc(doc);  
        return 0;  
    }  
 print_element_names(cur);
 //xmlSaveFormatFile("a.xml", doc, 1);
  xmlFreeDoc(doc);
  */
  complete_hash_init();
		parseurl_thread_pool=init_thread_pool(cfg_process_parseurl_thread_count,
			   cfg_process_parseurl_queue_size,
		 (void*)process_parseurl);
		struct url_node * node=(struct url_node *)malloc(sizeof(struct url_node));
             strcpy(node->url,uri);
             node->deep=1;
             md5(node->md5,uri,strlen(uri));          
           update_complete_node(node->md5);
  process_parseurl(node);
  while(1)
  	{
  	sleep(3600);
  	}
return 1;
}
