#ifndef HTMLPARSE_H
#define HTMLPARSE_H
struct url_node
{
	char url[1024];
	uint8_t md5[16];
	uint32_t deep;
};

struct complete_node
{
	uint8_t md5[16];
	struct complete_node *next;
};



extern unsigned int cfg_process_parseurl_thread_count;
extern unsigned int cfg_process_parseurl_queue_size;
extern struct thread_pool *parseurl_thread_pool;
#endif
