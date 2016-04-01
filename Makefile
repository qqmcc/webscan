SOURCE = check.c http.c md5.c
LDLIB = -lcurl
all: test
test:$(SOURCE)
	gcc --std=gnu99 -D_GNU_SOURCE $(SOURCE) $(LDLIB) -g -o test
clean:
	rm -f test
	rm -f *.o
