#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#include <curl/curl.h>
#include <pcre.h>
#include "http.h"

struct mem_node {
	unsigned char *buffer;
	size_t size;
	struct mem_node *next;
};

size_t curl_writer(void *ptr, size_t size, size_t nmemb, void * stream)
{
	unsigned char *buf=(unsigned char *)stream;
	size_t count = size *nmemb;
	buf = (unsigned char *)malloc(count);
	memcpy(buf,ptr,count);
	return count;
};



size_t curl_head(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t count = size *nmemb;
	int *last_head = (int *)stream;
	char *line = (char *)ptr;

	if (*last_head==0 && (!strncmp(line, "HTTP/1.1 200", 12) || !strncmp(line, "HTTP/1.0 200", 12))) *last_head = 1;
	if (*last_head==1 && !strncmp(line, "\r\n", 2))
		return count-1; // force exit curl: CURLE_WRITE_ERROR
	else
		return count;
}

int curl_http_head(struct response_head *headinfo, const char * remotepath, const char * referer,const char * cookie)
{
	long timeout = 60;
	long connect_timeout = 30;
	headinfo->size = -1;
	headinfo->last_modified = -1;

	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); // for thread safe
	curl_easy_setopt(curl, CURLOPT_URL, remotepath);
	if(referer) curl_easy_setopt(curl, CURLOPT_REFERER, referer);
	if(cookie)  curl_easy_setopt(curl, CURLOPT_COOKIE, cookie);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "icache");
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connect_timeout);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
	curl_easy_setopt(curl, CURLOPT_FILETIME, 1);
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
	CURLcode rc = curl_easy_perform(curl);
	long response_code;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
	if ((rc==CURLE_OK && (response_code==405 || response_code==400 || response_code==403)) || rc==CURLE_GOT_NOTHING || rc==CURLE_OPERATION_TIMEDOUT){
		int last_head = 0;
		curl_easy_cleanup(curl);
        char temp_path[] = "/tmp/tmp_XXXXXX";
        int fd = mkstemp(temp_path);
        if (fd!=-1) close(fd);
		FILE *fp = fopen(temp_path, "w");
		curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); //for thread safe
		curl_easy_setopt(curl, CURLOPT_URL, remotepath);
		if(referer) curl_easy_setopt(curl, CURLOPT_REFERER, referer);
		if(cookie)  curl_easy_setopt(curl, CURLOPT_COOKIE, cookie);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "icache");
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connect_timeout);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
		curl_easy_setopt(curl, CURLOPT_FILETIME, 1);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curl_head);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &last_head);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		rc = curl_easy_perform(curl);
		fclose(fp);
		unlink(temp_path);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
	}
	if ((rc==CURLE_OK || rc==CURLE_WRITE_ERROR) && response_code==200){
		double len;
		curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &len);
		headinfo->size = len;
		curl_easy_getinfo(curl, CURLINFO_FILETIME, &headinfo->last_modified);
	}
	curl_easy_cleanup(curl);
	if (headinfo->last_modified == -1) headinfo->last_modified = 0;
	if (headinfo->size!=-1){
		return 1;
	}else {
		return 0;
	}
}

int curl_http_file(const char * localpath, const char * remotepath, const char * referer,const char *cookie)
{
	long timeout = 10800;
	long connect_timeout = 30;
	long low_speed_limit = 1024;
	long low_speed_time = 60;

	FILE *fp;
	if (!(fp=fopen(localpath, "wb"))){
		perror(localpath);
		return 0;
	}
	long response_code;
	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); //for thread safe
	curl_easy_setopt(curl, CURLOPT_URL, remotepath);
	if(referer) curl_easy_setopt(curl, CURLOPT_REFERER, referer);
	if(cookie)  curl_easy_setopt(curl, CURLOPT_COOKIE, cookie);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "icache");
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connect_timeout);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, low_speed_limit);
	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, low_speed_time);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
	curl_easy_setopt(curl, CURLOPT_FILETIME, 1);
	//curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip, deflate");
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	CURLcode rc = curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
	double len = 0;
	long filesize = 0;
	time_t last_modified = 0;
	curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &len);
	filesize = len;
	curl_easy_getinfo(curl, CURLINFO_FILETIME, &last_modified);
	curl_easy_cleanup(curl);
	fclose(fp);

	int success = 1;
	if (rc!=CURLE_OK || response_code!=200) {
		success = 0;
	}else {
		if (filesize!=-1) {
			long localsize = 0;
			struct stat fileinfo;
			if (stat(localpath, &fileinfo) == 0) localsize =  fileinfo.st_size;
			if (filesize!=localsize) success = 0;
		}
	}

	if (success) {
		if (last_modified!=-1){
			struct utimbuf amtime;
			amtime.actime = amtime.modtime = last_modified;
			utime(localpath, &amtime);
		}
		return 1;
	}else{
		unlink(localpath);
		return 0;
	}
}

size_t curl_read(void *ptr, size_t size, size_t nmemb, void *stream)
{
	struct mem_node **mem_head = (struct mem_node **)stream;
	size_t count = size *nmemb;
	struct mem_node *mem = (struct mem_node *)malloc(sizeof(struct mem_node));
	mem->next = NULL;
	mem->size = count;
	mem->buffer = (unsigned char *)malloc(count);
	memcpy(mem->buffer, ptr, count);
	if (*mem_head==NULL){
		*mem_head = mem;
	}else{
		struct mem_node *p = *mem_head;
		while(p->next) p = p->next;
		p->next = mem;
	}
	return count;
}

char* curl_post_form(const char *url,\
	                 const char *postdata,\
	                 const char *proxy,\
	                 const char *cookie,\
	                 int flag_cookie)


{
   long timeout = 10800;
	long connect_timeout = 15;
	long low_speed_limit = 1024;
	long low_speed_time = 60;

	struct mem_node *mem_head = NULL;
	long response_code;
   CURL *curl = curl_easy_init();
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); // for thread safe
   curl_easy_setopt(curl,CURLOPT_URL,url); //url地址  
   curl_easy_setopt(curl,CURLOPT_POSTFIELDS,postdata); //post参数  
   curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connect_timeout);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, low_speed_limit);
	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, low_speed_time);
   curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,&curl_read); //对返回的数据进行操作的函数地址  
   curl_easy_setopt(curl,CURLOPT_WRITEDATA,&mem_head); //这是write_data的第四个参数值  
   curl_easy_setopt(curl,CURLOPT_POST,1); //设置非0表示本次操作为post   
   curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1); //设置为非0,响应头信息location  
   if(flag_cookie)
   {
    curl_easy_setopt(curl,CURLOPT_COOKIEFILE,"./cookie.txt");
     curl_easy_setopt(curl,CURLOPT_COOKIEJAR,"./cookie.txt");
 }
  // curl_easy_setopt(easy_handle, CURLOPT_PROXY,proxy);
   CURLcode rc = curl_easy_perform(curl); 
  	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
	curl_easy_cleanup(curl);
	if (rc!=CURLE_OK) {
		return NULL;
	}else if (response_code!=200 && response_code!=206){
		struct mem_node *p = mem_head;
		while(p){
			struct mem_node *q = p;
			p = p->next;
			free(q->buffer);
			free(q);
		}
		return NULL;
	}else{
		struct mem_node *p = mem_head;
		size_t size = 0;
		while(p){
			size += p->size;
			p = p->next;
		}
		char *content = (char*)malloc(size+1);
		p = mem_head;
		size = 0;
		while(p){
			memcpy(content+size, p->buffer, p->size);
			size += p->size;
			struct mem_node *q = p;
			p = p->next;
			free(q->buffer);
			free(q);
		}
		content[size] = 0;
		return content;
	}
  
}

char* curl_http_content(const char *uri ,int flag_cookie)
{
	long timeout = 10800;
	long connect_timeout = 15;
	long low_speed_limit = 1024;
	long low_speed_time = 60;

	struct mem_node *mem_head = NULL;
	long response_code;
	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); // for thread safe
	curl_easy_setopt(curl, CURLOPT_URL, uri);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connect_timeout);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, low_speed_limit);
	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, low_speed_time);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mem_head);
	if(flag_cookie)
	{
	 curl_easy_setopt(curl,CURLOPT_COOKIEFILE,"./cookie.txt");
    curl_easy_setopt(curl,CURLOPT_COOKIEJAR,"./cookie.txt");
    }
	CURLcode rc = curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
	curl_easy_cleanup(curl);

	if (rc!=CURLE_OK) {
		return NULL;
	}else if (response_code!=200 && response_code!=206){
		struct mem_node *p = mem_head;
		while(p){
			struct mem_node *q = p;
			p = p->next;
			free(q->buffer);
			free(q);
		}
		return NULL;
	}else{
		struct mem_node *p = mem_head;
		size_t size = 0;
		while(p){
			size += p->size;
			p = p->next;
		}
		char *content = (char*)malloc(size+1);
		p = mem_head;
		size = 0;
		while(p){
			memcpy(content+size, p->buffer, p->size);
			size += p->size;
			struct mem_node *q = p;
			p = p->next;
			free(q->buffer);
			free(q);
		}
		content[size] = 0;
		return content;
	}
}

int get_http_head(struct response_head *headinfo, const char * remotepath, const char * referer,const char *cookie)
{
	struct response_head test;
	while(1){
		int succeed = curl_http_head(headinfo, remotepath, referer,cookie);
		if (succeed) return succeed;		
		else if (curl_http_head(&test, "http://www.baidu.com/img/bdlogo.gif", "",NULL)) return 0;
		else sleep(60);
	}
}

int get_http_file(const char * localpath, const char * remotepath, const char * referer,const char * cookie)
{
	struct response_head test;
	while(1){
		int succeed = curl_http_file(localpath, remotepath, referer,cookie);
		if (succeed) return succeed;
		else if (curl_http_head(&test, "http://www.baidu.com/img/bdlogo.gif", "",NULL)) return 0;
		else sleep(60);
	}
}

char * get_http_form(const char *url,\
	                 const char *postdata,\
	                 const char *proxy,\
	                 const char *cookie,
	                 int flag_cookie)
{
		struct response_head test;
	while(1){
		char *content = curl_post_form(url,postdata,proxy,cookie,flag_cookie);
		if (content) return content;
		else if (curl_http_head(&test, "http://www.baidu.com/img/bdlogo.gif", "",NULL)) return NULL;
		else sleep(60);
	}
	
}

char * get_http_content(const char *uri,int flag_cookie)
{
	struct response_head test;
	while(1){
		char *content = curl_http_content(uri,flag_cookie);
		if (content) return content;
		else if (curl_http_head(&test, "http://www.baidu.com/img/bdlogo.gif", "",NULL)) return NULL;
		else sleep(60);
	}
}
