/*************************************
#ifndef HTTP_H
#define HTTP_H
*************************************/
struct response_head{
	long size;
	time_t last_modified;
};

int get_http_head(struct response_head *headinfo, const char * remotepath, const char * referer,const char * cookie);
int get_http_file(const char * localpath, const char * remotepath, const char * referer,const char * cookie);
char* get_http_content(const char *uri); //need free response
int curl_post_form(const char *url,\
	                 const char *postdata,\
	                 const char *proxy,\
	                 const char *cookie,\
	                 void *stream);

/*************************************
#endif
*************************************/
