#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <openssl/md5.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "cgi_conf.h"
#include "libcgi.h"
#include "cgierr.h"
#include "../log.h"

static char upperchar(char ch){
	if ((ch>='a') && (ch<='z')) {
		ch='A'+(ch - 'a');
		return ch;
	}
	else
		return ch;
}

static char gethex(char ch){
	ch = upperchar(ch);
	if (( ch >= '0' ) && ( ch <= '9'))
		return (ch - '0');
	if ((ch >= 'A')&&( ch <= 'F'))
		return (ch - 'A'+10);
}

static CGI_LIST* cgi_parser(char* buff, CGI_LIST* parent){
	CGI_LIST* list  = NULL;
	int k           = 0;
	int v           = 0;
	int i           = 0;
	int p           = 0;
	int j           = 0;
	char h1         = 0;
	char h2         = 0;

	if (buff && strlen(buff)){
        while (buff[i]) {
            if (buff[i] == '&'){
                i++;
                continue;
            }
            if(list = (CGI_LIST*)malloc(sizeof(CGI_LIST))){
                p=0;
                for(k=i; buff[k] != '=' && buff[k]; k++)
                    if (buff[k] == '%')
                        p+=2;
                if(list->key = (char*)malloc(k-i+1 - p)) {
                    for (p=i,j=0; p < k; j++,p++) {
                        if (buff[p] == '%'){
                            h1 = gethex(buff[++p])<<4;
                            h2 = gethex(buff[++p]);
                            list->key[j] = h1+h2;
                        }
                        else if (buff[p] == '+')
                            list->key[j] = ' ';
                        else
                            list->key[j] = buff[p];
                    }
                    list->key[j]='\0';
                }
                if (buff[k] == '=')
                    k++;
                p=0;
                for(v=k; buff[v] != '&' && buff[v]; v++)
                    if (buff[v] == '%')
                        p+=2;
                if(list->val = (char*)malloc(v-k+1 - p)) {
                    for (p=k,j=0; p < v; j++,p++) {
                        if (buff[p] == '%'){
                            h1 = gethex(buff[++p])<<4;
                            h2 = gethex(buff[++p]);
                            list->val[j] = h1+h2;
                        }
                        else if (buff[p] == '+')
                            list->val[j] = ' ';
                        else
                            list->val[j] = buff[p];
                    }
                    list->val[j]='\0';
                }
                list->p = parent;
                parent = list;
                i=v;
            }
        }
	}
return list;
}

static char* get_boundary(const char* content_type){
    char* boundary = NULL;
    char* p = strdup(content_type);
    if(p){
        if(boundary = strstr(p, "boundary=")){
            boundary += (strlen("boundary=") - 2);
            memset(boundary, '-', 2);
            boundary = strdup(boundary);
        }
    free(p);
    }
return boundary;
}

static int read_boundary(const char* till){
    char c      = 0;
    long len    = 0;
    char* buf   = NULL;
    int e       = -1;

    if(!till) return e;

    len   = strlen(till);
    buf   = malloc(len);
    if(buf){
        memset(buf, 0, len);
        c = getc(stdin);
        while(!feof(stdin)){
            memmove(buf, buf + 1, len - 1);
            buf[len - 1] = c;
            if(!memcmp(buf, till, len)){
                e = 0;
                break;
            }
        c = getc(stdin);
        }
    free(buf);
    }
    else XPRINTF(ERROR, "ERROR:%s:%d no free memory!\n", __FILE__, __LINE__);

return e;
}

static int read_head(char** buff){
    char c      = 0;
    long len    = 0;
    char* buf   = NULL;
    int fd      = -1;
    long long rv = 0;
    char* cach_buf = NULL;
    int e       = -1;

    if(!buff) return e;

    const char* till = "\r\n\r\n";
    len   = strlen(till);
    buf   = malloc(len);
    if(buf){
        memset(buf, 0, len);
        *buff = strdup("");
        long long n = 0;
        c = getc(stdin);
        while(!feof(stdin)){
            n++;
            *buff = realloc(*buff, n + 1);
            (*buff)[n - 1] = c;
            (*buff)[n] = 0;
            memmove(buf, buf + 1, len - 1);
            buf[len - 1] = c;
            if(!memcmp(buf, till, len)){
                e = 0;
                break;
            }
        c = getc(stdin);
        }
    free(buf);
    }
    else XPRINTF(ERROR, "ERROR:%s:%d no free memory!\n", __FILE__, __LINE__);
return e;
}

static int _write(int fd, char** cach_buf, long long cache_size){
    long long n = write(fd, *cach_buf, cache_size);
    if(-1 == n){
        cgi_err_set(errno, __LINE__, __FILE__, NULL);
        XPRINTF(ERROR, "ERROR:%s:%d_WRITE: strerror=%s errno=%d n=%lld cache_size=%lld\n", __FILE__, __LINE__, strerror(errno), errno, n, cache_size);
    }
return n;
}

static char* memstr(char* haystack, char* needle, int size)
{
	char* p;
	char needlesize = strlen(needle);

	for (p = haystack; p <= (haystack - needlesize + size); p++){
		if (memcmp(p, needle, needlesize) == 0)
			return p; /* found */
	}
return NULL;
}

static long long read_file(const char* till, const char* path, long long cache_size, char** stMD5, char** pfilerror){
    char c          = 0;
    long len        = 0;
    char* buf       = NULL;
    int fd          = -1;
    long long rv    = 0;
    char* cach_buf  = NULL;

    if(!till || !path || !cache_size) return rv;

    int fds[2];
    pipe(fds);
    fflush(stdout);
    pid_t pid = fork();
    if(!pid){
        int e = 0;
        close(fds[1]);
        fclose(stdout);
        fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, UPLOAD_FILE_PERMITTIONS);
        if(-1 != fd){
            lockf(fd, F_LOCK, 0L);
            cach_buf = malloc(cache_size);
            memset(cach_buf, 0, cache_size);
            if(cach_buf){
                long long len = 0;
                do{
                    len = read(fds[0], cach_buf, cache_size);
                    if(0 < len){
                        int err = 0;
                        if(-1 == _write(fd, &cach_buf, len)){
                            e = 1;
                            break;
                        }
                    }
                }while(0 < len);
            free(cach_buf);
            }
            lockf(fd, F_ULOCK, 0L);
        close(fd);
        }
        else{
            XPRINTF(ERROR, "ERROR:%s:%dREAD_FILE: fail open %s for writing\n", __FILE__, __LINE__, path);
            e = 2;
        }
        close(fds[0]);
        exit(e);
    }
    else{
        close(fds[0]);
        /* parent */
        unsigned char md5digest[MD5_DIGEST_LENGTH];
        memset(md5digest, 0, MD5_DIGEST_LENGTH);
#ifdef WITH_MD5
        MD5_CTX md5handler;
        memset(&md5handler, 0, sizeof(md5handler));
        MD5_Init(&md5handler);
#endif
        char* long_bound = NULL;
        asprintf(&long_bound, "\r\n%s", till);
        if(long_bound){
            cach_buf = malloc(cache_size*2);
            if(cach_buf){
                int i = 0;
                do{
                    size_t len = fread(cach_buf + cache_size*i, 1, cache_size, stdin);
                    char* p = memstr(cach_buf, long_bound, cache_size*2);
                    if(p){
                        int tail = 0;
                        if(p - cach_buf > cache_size){
                            write(fds[1], cach_buf, cache_size);
                            rv += cache_size;
                            tail = p - cach_buf - cache_size;
                            write(fds[1], cach_buf + cache_size, tail);
                        }
                        else{
                            tail = p - cach_buf;
                            write(fds[1], cach_buf, tail);
                        }
#ifdef WITH_MD5
                        MD5_Update(&md5handler, cach_buf, p - cach_buf);
#endif
                        rv += tail;
                        break;
                    }
                    if(i){
                        write(fds[1], cach_buf, cache_size);
#ifdef WITH_MD5
                        MD5_Update(&md5handler, cach_buf, cache_size);
#endif
                        rv += cache_size;
                        memcpy(cach_buf, cach_buf + cache_size, cache_size);
                    }
                    else
                        i = 1;
                }while(!feof(stdin));
            free(cach_buf);
            }
            else{
                cgi_err_set(ENOMEM, __LINE__, __FILE__, NULL);
            }
        free(long_bound);
        }
#ifdef WITH_MD5
        MD5_Final(md5digest, &md5handler);
#endif
        asprintf(stMD5, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", md5digest[0], md5digest[1], md5digest[2], md5digest[3], md5digest[4], md5digest[5], md5digest[6], md5digest[7], md5digest[8], md5digest[9], md5digest[10], md5digest[11], md5digest[12], md5digest[13], md5digest[14], md5digest[15]);
        //rd_log("DEBUG:READ_FILE: MD5=%s", *stMD5);
        close(fds[1]);
        int status = 0;
        if(wait(&status) == -1){
            cgi_err_set(errno, __LINE__, __FILE__, NULL);
            XPRINTF(ERROR, "ERROR:%s:%dREAD_FILE: strerror=%s errno=%d", strerror(errno), errno);
        }
        //rd_log("DEBUG:READ_FILE: fork() finish with status=%d", WEXITSTATUS(status));
        switch(WEXITSTATUS(status)){
            case 1:
                cgi_err_set(EIO, __LINE__, __FILE__, NULL);
                XPRINTF(ERROR, "ERROR:%s:%dREAD_FILE: write() error\n", __FILE__, __LINE__);
                rv = -1;
                break;
            case 2:
                cgi_err_set(EIO, __LINE__, __FILE__, NULL);
                XPRINTF(ERROR, "ERROR:%s:%dREAD_FILE: open() error\n", __FILE__, __LINE__);
                rv = -2;
                break;
            default:
                break;
        }
    }
return rv;
}

static size_t __write(const int fd, const char* buf, const unsigned int buf_len, char** pfilerror){
    unsigned int len     = 0;
    unsigned int total   = 0;

    do{
        len = write(fd, buf + total, buf_len - total);
        if(len == -1){
            if(errno == EAGAIN /* || errno == EWOULDBLOCK */){
                continue;
            }
            else{
                cgi_err_set(errno, __LINE__, __FILE__, NULL);
                if(*pfilerror) free(*pfilerror);
                asprintf(pfilerror,"ERROR:CGI.C: %s", strerror(errno));
                break;
            }
        }
        else if(!len){
            cgi_err_set(EIO, __LINE__, __FILE__, NULL);
            if(*pfilerror) free(*pfilerror);
            asprintf(pfilerror,"ERROR:CGI.C: 0 bytes written to file");
            break;
        }
        else{
            total += len;
        }
    }while(total < buf_len);

return total;
}

static size_t read_stream_file(const char* path, const unsigned int cache_size, char** stMD5, char** pfilerror){
    char c          = 0;
    int fd          = -1;
    size_t rval     = 0;
    char* cach_buf  = NULL;

    unsigned char md5digest[MD5_DIGEST_LENGTH];
    memset(md5digest, 0, MD5_DIGEST_LENGTH);
#ifdef WITH_MD5
    MD5_CTX md5handler;
    memset(&md5handler, 0, sizeof(md5handler));
    MD5_Init(&md5handler);
#endif

    if(!path) return 0;

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, UPLOAD_FILE_PERMITTIONS);
    if(-1 != fd){
        if(cache_size){
            lockf(fd, F_LOCK, 0L);
            cach_buf = malloc(cache_size);
            if(cach_buf){
                //rd_log("DEBUG:UPLOAD: START READ");
                do{
                    unsigned int len = fread(cach_buf, sizeof(char), cache_size, stdin);
                    //rd_log("DEBUG:UPLOAD: len=%d", len);
                    if(len){
                        unsigned int w_len = __write(fd, cach_buf, len, pfilerror);
                        //rd_log("DEBUG:UPLOAD: w_len=%d", w_len);
                        if(len != w_len){
                            rval += w_len;
                        break;
                        }
                        rval += len;
#ifdef WITH_MD5
                        MD5_Update(&md5handler, cach_buf, len);
#endif
                    }
                    //rd_log("DEBUG:UPLOAD: rval=%d", rval);
                    if(ferror(stdin)){
                        //rd_log("DEBUG:UPLOAD: ferror");
                        if(*pfilerror) free(*pfilerror);
                        asprintf(pfilerror,"ERROR:CGI.C: error while read stdin");
                        cgi_err_set(EIO, __LINE__, __FILE__, NULL);
                    break;
                    }
                    //rd_log("DEBUG:UPLOAD: while");
                }while(!feof(stdin));
                //rd_log("DEBUG:UPLOAD: END READ");
            free(cach_buf);
            }
            else{
                if(*pfilerror) free(*pfilerror);
                asprintf(pfilerror,"ERROR:CGI.C: out of memory");
                cgi_err_set(ENOMEM, __LINE__, __FILE__, NULL);
            }
            lockf(fd, F_ULOCK, 0L);
        }
    close(fd);
    }
    else{
        cgi_err_set(errno, __LINE__, __FILE__, NULL);
        if(*pfilerror) free(*pfilerror);
        asprintf(pfilerror,"ERROR:CGI.C: %s", strerror(errno));
    }

#ifdef WITH_MD5
    MD5_Final(md5digest, &md5handler);
#endif
    asprintf(stMD5, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", md5digest[0], md5digest[1], md5digest[2], md5digest[3], md5digest[4], md5digest[5], md5digest[6], md5digest[7], md5digest[8], md5digest[9], md5digest[10], md5digest[11], md5digest[12], md5digest[13], md5digest[14], md5digest[15]);
    //rd_log("DEBUG:READ_STREAM_FILE: MD5=%s", *stMD5);

return rval;
}

static char* parse_tag_string(const char* string, const char* tag_s, const char* tag_e){
    char* rval  = NULL;
    char* s     = strdup(string);
    if(s){
        char* p = strstr(s, tag_s);
        if(p){
            p += strlen(tag_s);
            if(tag_e){
                *(strstr(p, tag_e)) = 0;
            }
            char* p_ = strstr(p, "\n");
            if(p_){
                *(p_) = 0;
            }
            rval = strdup(p);
        }
    free(s);
    }
return rval;
}

CGI_LIST* cgi_parse_stdin(const char* path){
	size_t len       = 0;
    size_t real_len  = 0;
	char* buff       = NULL;
	CGI_LIST* list   = NULL;

	if(REQUEST_METHOD) {
		if(!strcmp(REQUEST_METHOD, "GET")) {
			list = cgi_parser(QUERY_STRING,NULL);
 		}
        else if(!strcmp(REQUEST_METHOD, "PUT")){
			len = (size_t)atoll(CONTENT_LENGTH);
            if(path){
                char stMD5[33]  = {0};
                char* stMD5_    = stMD5;
                char* filerror  = NULL;
                size_t filelen = read_stream_file(path, (len > CACHE_SIZE)?CACHE_SIZE:(unsigned int)len, &stMD5_, &filerror);
                //rd_log("DEBUG:UPLOAD: filelen=%d", filelen);
                if(filelen >= 0){
                    CGI_LIST* parent    = NULL;
                    if(list = (CGI_LIST*)malloc(sizeof(CGI_LIST))){
                        list->key = strdup("__filesize__");
                        char* filesize = NULL;
                        asprintf(&filesize, "%zd", filelen);
                        list->val = filesize;
                        list->p = NULL;
                        parent = list;
                    }
                    if(list = (CGI_LIST*)malloc(sizeof(CGI_LIST))){
                        list->key = strdup("__md5sum__");
                        list->val = strdup(stMD5_);
                        list->p = parent;
                        parent = list;
                    }
                    if(list = (CGI_LIST*)malloc(sizeof(CGI_LIST))){
                        list->key = strdup("__filerror__");
                        list->val = filerror;
                        list->p = parent;
                        parent = list;
                    }
                }
            }
        }
		else {
			len = (size_t)atoll(CONTENT_LENGTH);
			if(path) {
                if(CONTENT_TYPE && strstr(CONTENT_TYPE, "multipart/form-data")){
                    char* boundary = get_boundary(CONTENT_TYPE);
                    if(boundary){
                        if(!read_boundary(boundary)){
                            char* head = NULL;
                            if(!read_head(&head)){
                                if(head){
                                    char* name = parse_tag_string(head, " name=\"", "\"");
                                    char* filename = parse_tag_string(head, " filename=\"", "\"");
                                    if(name && filename){
                                        CGI_LIST* parent    = NULL;
                                        if(list = (CGI_LIST*)malloc(sizeof(CGI_LIST))){
                                            list->key = strdup("name");
                                            list->val = strdup(name);
                                            list->p = NULL;
                                            parent = list;
                                        }
                                        if(list = (CGI_LIST*)malloc(sizeof(CGI_LIST))){
                                            list->key = strdup("filename");
                                            list->val = strdup(filename);
                                            list->p = parent;
                                            parent = list;
                                        }
                                        char* full_path = NULL;
                                        asprintf(&full_path, "%s/%s", path, filename);
                                        if(full_path){
                                            char stMD5[33]  = {0};
                                            char* stMD5_    = stMD5;
                                            char* filerror  = NULL;
                                            long long filelen = read_file(boundary, full_path, (len > CACHE_SIZE)?CACHE_SIZE:len, &stMD5_, &filerror);
                                            if(filelen >= 0){
                                                read_boundary("--\r\n");
                                                /* END */
                                                if(list = (CGI_LIST*)malloc(sizeof(CGI_LIST))){
                                                    list->key = strdup("__filesize__");
                                                    char* filesize = NULL;
                                                    asprintf(&filesize, "%lld", filelen);
                                                    list->val = filesize;
                                                    list->p = parent;
                                                    parent = list;
                                                }
                                                if(list = (CGI_LIST*)malloc(sizeof(CGI_LIST))){
                                                    list->key = strdup("__md5sum__");
                                                    list->val = strdup(stMD5_);
                                                    list->p = parent;
                                                    parent = list;
                                                }
                                                if(list = (CGI_LIST*)malloc(sizeof(CGI_LIST))){
                                                    list->key = strdup("__filerror__");
                                                    list->val = filerror;
                                                    list->p = parent;
                                                    parent = list;
                                                }
                                            }
                                        free(full_path);
                                        }
                                    free(name), free(filename);
                                    }
                                free(head);
                                }
                            }
                        }
                    free(boundary);
                    }
                }
                else{
                    if(buff = (char*)malloc(len+1)) {
                        real_len = fread(buff, sizeof(char), len, stdin);
                        buff[len] = '\0';
                        list = cgi_parser(buff,list);
                    free(buff);
                    }
                }
			}
		}
	}
return list;
}

char* cgi_getarg(CGI_LIST* p, const char* key) {
    char* rval = NULL;

	while(p) {
		if(key && p->key && !strcmp(key, p->key))
			return p->val;
		else
			p=p->p;
	}
return rval;
}

int cgi_free(CGI_LIST* p) {
	CGI_LIST* tmp;
    int rv = -1;

    if(p) rv = 0;
	while(p) {
        if(p->key)
		    free(p->key);
        if(p->val)
    		free(p->val);
		tmp = p->p;
        free(p);
		p = tmp;
	}
return rv;
}

void cgi_printf(const char* format, ...){
    va_list argptr;
    char* message   = NULL;
    va_start(argptr, format);
    vasprintf(&message, format, argptr);
    if(message){
        fwrite(message, sizeof(char), strlen(message), stdout);
    free(message);
    }
    va_end(argptr);
}

