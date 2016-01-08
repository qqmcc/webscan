SOURCE = htmlparse.c http.c md5.c threadpool.c
LDLIB = -lgumbo_xml -lxml2 -lcurl -lpthread -lpcre
LIB = -L /usr/local/lib
INCLUDE = -I /usr/local/include/libxml2 -I /usr/local/include -I /usr/include
all: scan
scan:$(SOURCE)
	gcc --std=gnu99 -D_GNU_SOURCE $(SOURCE) $(INCLUDE) $(LIB) $(LDLIB)  -g -o scan
clean:
	rm -f scan
	rm -f *.o
