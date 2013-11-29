CC = gcc
CFLAGS = -O2 -s 
#CFLAGS += -DWITH_MD5

all :  cgi.c cgi_conf.h libcgi.h cgierr.c cgierr.h cgitmpl.c cgitmpl.h
	$(CC) $(CFLAGS) -c cgi.c -I../liblst -lz
	$(CC) $(CFLAGS) -c cgi.c -I../liblst -lz
	$(CC) $(CFLAGS) -c cgierr.c -lz
	$(CC) $(CFLAGS) -c cgitmpl.c 
	ar cr libcgi.a cgi.o cgierr.o cgitmpl.o

clean :
	rm -f *.o
	rm -f *.a

