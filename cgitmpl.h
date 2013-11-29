typedef struct cgi_tmpl_list {
	char* key;
	char* val;
	int prevlenval;
	struct cgi_tmpl_list* p;
} CGI_TMPL_LIST;

typedef struct cgi_tmpl_cookies_list {
	char* name;
	char* value;
	struct cgi_tmpl_cookies_list* p;
} CGI_TMPL_COOKIES_LIST;

