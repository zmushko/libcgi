#ifndef LIBCGI_H
#define LIBCGI_H

typedef struct cgi_list {
	char* key;
	char* val;
	struct cgi_list* p;
} CGI_LIST;

CGI_LIST*   cgi_parse_stdin(const char*);
char*       cgi_getarg(CGI_LIST*, const char*);
int         cgi_free(CGI_LIST*);
void        cgi_printf(const char*, ...);

#define GATEWAY_INTERFACE	getenv("GATEWAY_INTERFACE")
#define SERVER_PROTOCOL		getenv("SERVER_PROTOCOL")
#define PATH_INFO		getenv("PATH_INFO")
#define PATH_TRANSLATED		getenv("PATH_TRANSLATED")
#define QUERY_STRING		getenv("QUERY_STRING")
#define CONTENT_TYPE		getenv("CONTENT_TYPE")
#define CONTENT_LENGTH		getenv("CONTENT_LENGTH")
#define REQUEST_METHOD		getenv("REQUEST_METHOD")
#define SERVER_SOFTWARE		getenv("SERVER_SOFTWARE")
#define SERVER_NAME		getenv("SERVER_NAME")
#define SERVER_ADMIN		getenv("SERVER_ADMIN")
#define SERVER_PORT		getenv("SERVER_PORT")
#define SCRIPT_NAME		getenv("SCRIPT_NAME")
#define DOCUMENT_ROOT		getenv("DOCUMENT_ROOT")
#define REMOTE_HOST		getenv("REMOTE_HOST")
#define REMOTE_ADDR		getenv("REMOTE_ADDR")
#define REMOTE_USER		getenv("REMOTE_USER")
#define REMOTE_GROUP		getenv("REMOTE_GROUP")
#define AUTH_TYPE		getenv("AUTH_TYPE")
#define REMOTE_IDENT		getenv("REMOTE_IDENT")
#define HTTP_ACCEPT		getenv("HTTP_ACCEPT")
#define HTTP_COOKIE		getenv("HTTP_COOKIE")
#define HTTP_ACCEPT_LANGUAGE	getenv("HTTP_ACCEPT_LANGUAGE")
#define HTTP_REFERER		getenv("HTTP_REFERER")
#define HTTP_USER_AGENT		getenv("HTTP_USER_AGENT")
#define HTTP_RANGE		getenv("HTTP_RANGE")
#define LOCATION		getenv("LOCATION")
#define HTTP_IF_MATCH		getenv("HTTP_IF_MATCH")

typedef struct cgi_tmpl {
	char* name;
	struct cgi_tmpl_list* list;
	struct cgi_tmpl_cookies_list* cookies_list;
	char* s_buff;  	/*buffer for socket if used*/
	int s_buff_len; /*buffer lenght for socket if used*/
} CGI_TMPL;

/*-----------------------------------------
  CGI_TMPL* tmpl;
  tmpl = cgi_template_open("tmpl.html");
*/
CGI_TMPL* cgi_tmpl_open(char*);

/*-----------------------------------------
  cgi_tmpl_add(tmpl,"header","2+"); HTML: {header}
  cgi_tmpl_add(tmpl,"header","2=4"); - you can add to same {header}
*/
int cgi_tmpl_add(CGI_TMPL*,const char*,const char*);

/*-----------------------------------------
  cgi_tmpl_print(tmpl); -
  - printf(template);
*/
int cgi_tmpl_print_stdout(CGI_TMPL*);

/*
   print without header
*/
int cgi_tmpl_print_stdout_0(CGI_TMPL*);


#endif
