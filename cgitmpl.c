#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "cgitmpl.h"
#include "libcgi.h"

#define SIZEOFBUF 1024

CGI_TMPL_LIST* new_cgi_tmpl_list(char* key, char* val, CGI_TMPL_LIST* par)
{
	CGI_TMPL_LIST* p = NULL;
	int len;

    p = (CGI_TMPL_LIST*)malloc(sizeof(CGI_TMPL_LIST));
	if (p) {
		memset(p, 0, sizeof(CGI_TMPL_LIST));
		if (key) {
			len = strlen(key) + 1;
			if ((p->key = (char*)malloc(len))) {
				memcpy(p->key, key, len);
			}
			else return NULL;
		}
		else
			p->key = NULL;
		if (val) {
			len = strlen(val) + 1;
			if ((p->val = (char*)malloc(len))) {
				memcpy(p->val, val, len);
			}
			else return NULL;
		}
		else
			p->val = NULL;
		p->p = par;
	}
	return p;
}

CGI_TMPL* cgi_tmpl_open(char* path)
{
	CGI_TMPL* tmpl = NULL;
	CGI_TMPL_LIST* tmpl_list = NULL;
	int len = 0;
	FILE* f = NULL;
	int ch = 0;
	int inkey = 0;
	char* keybuf = NULL;
	char* tmp_p_keybuf = NULL;
	int buf_size = 0;
	int i = 0;
	char buf[5] = {0};
	int rem = 0;
	int flag = 0;
	CGI_TMPL_LIST* p = NULL;

	if(!path)
		return NULL;
	memset(buf,0,sizeof(buf));
	buf_size = SIZEOFBUF;
	if((keybuf = (char*)malloc(buf_size))){
		memset(keybuf,0,buf_size);
		if ((tmpl = (CGI_TMPL*)malloc(sizeof(CGI_TMPL)))) {
			memset(tmpl,0,sizeof(CGI_TMPL));
			tmpl->s_buff = NULL;
			tmpl->s_buff_len =0;
			len = strlen(path)+1;
			tmpl->cookies_list = NULL;
			if ((tmpl->name = (char*)malloc(len))) {
				memset(tmpl->name,0,len);
				memcpy(tmpl->name,path,len);
				if ((f = fopen(path, "r"))) {
					inkey = 0;
					i=0;
					while((ch = fgetc(f)) != EOF){
						buf[0]=buf[1];buf[1]=buf[2];buf[2]=buf[3];buf[3]=ch;
						if(!strcmp(buf,"<!--"))rem=1;
						if(!strcmp(buf+1,"-->"))rem=0;
						if(!rem){
							if(ch == '{') inkey++;
							if(buf_size == i){
								buf_size += SIZEOFBUF;
								if((tmp_p_keybuf = (char*)malloc(buf_size + 1))){
									memset(tmp_p_keybuf,0,buf_size + 1);
									memcpy(tmp_p_keybuf, keybuf, i);
								free(keybuf);
								}else{	free(keybuf); return NULL;}
								keybuf = tmp_p_keybuf;
							}
							if(inkey)	keybuf[i++] = ch;
							if(ch == '}') inkey--;
							if(!inkey && ch == '}'){
								keybuf[i] = '\0';
								i=0;
								p = tmpl_list;
								while (p) {
									if (!strcmp(p->key,keybuf)){
										flag =1;
									}
									p = p->p;
								}
								if(!flag)
									tmpl_list = new_cgi_tmpl_list(keybuf,NULL,tmpl_list);
								flag =0;
							}
						}
					}
					tmpl->list = tmpl_list;
				fclose(f);
				}
                else{
                    //cgi_log("Can not open file: %s", path);
					free(keybuf);
					free(tmpl->name);
					free(tmpl);
                    return NULL;
                }
			}
			else{
				free(keybuf);
				free(tmpl);
				return NULL;
			}
		}
		else{
			free(keybuf);
			return NULL;
		}
		free(keybuf);
	}
	else{
		return NULL;
	}
	//cgi_log("=>return");
	return tmpl;
}

int cgi_tmpl_add(CGI_TMPL* tmpl,const char* key,const char* val)
{
	CGI_TMPL_LIST* p;
	int len;
	int len2;
	char keybuf[256];
	char* pstr;
	char* pstr2;

	if(!tmpl)
		return 1;
	len = strlen(key);
	keybuf[0] = '{';
	memcpy(keybuf+1, key, len);
	keybuf[len+1] = '}';
	keybuf[len+2] = '\0';

	if(!val || !key || strlen(key) > 253)
		return 1;

	p = tmpl->list;
	while(p){
		pstr2 = (char*)strstr(p->key, keybuf);
		if (p->key == pstr2){
			len = strlen(val);
			if(!p->val){
				if((p->val = (char*)malloc(len+1))){
					memcpy(p->val, val, len+1);
				} else return -1;
			}
			else {
				len2 = strlen(p->val);
				if((pstr = (char*)malloc(len2 + len + 1))){
					memcpy(pstr, p->val, len2);
					memcpy(pstr+len2, val, len + 1);
					free(p->val);
					p->val = pstr;
				} else return -1;
			}
		}
		else if (pstr2){
			if(!p->val){
				len = strlen(p->key)-strlen(pstr2);
				if((pstr = (char*)malloc(strlen(p->key) - strlen(keybuf) + strlen(val) + 1))){
						memcpy(pstr,p->key+1,len-1);
						memcpy(pstr+len-1,val,strlen(val));
						memcpy(pstr+len+strlen(val)-1,p->key+len+strlen(keybuf),strlen(p->key)-len-strlen(keybuf));
						pstr[strlen(p->key) - strlen(keybuf) + strlen(val)-2] = '\0';
						p->val = pstr;
				} else return -1;
			}
			else {
				if(strstr(p->val + p->prevlenval, keybuf)){
					int len_p_val, len_keybuf, len_val;
					len_p_val = strlen(p->val);
					len_keybuf = strlen(keybuf);
					len_val = strlen(val);
					while((pstr2 = (char*)strstr(p->val + p->prevlenval, keybuf))){
						if((pstr = (char*)malloc(len_p_val - len_keybuf + len_val + 1))){
							len = len_p_val - strlen(pstr2);
							memcpy(pstr,p->val,len);
							memcpy(pstr+len,val,len_val);
							memcpy(pstr+len+len_val,p->val+len+len_keybuf,len_p_val-len-len_keybuf);
							pstr[len_p_val - len_keybuf + len_val] = '\0';
							free(p->val);
							p->val = pstr;
						} else return -1;
					}
				}
				else {
					int len_p_val, len_p_key, len_keybuf, len_val;
					len_p_val = strlen(p->val);
					len_p_key = strlen(p->key);
					len_keybuf = strlen(keybuf);
					len_val = strlen(val);
					if((pstr = (char*)malloc(len_p_val + len_p_key - len_keybuf + len_val - 1))){
						pstr2 = (char*)strstr(p->key,keybuf);
						len = len_p_key-strlen(pstr2);
						memcpy(pstr,p->val,len_p_val);
						memcpy(pstr+len_p_val,p->key+1,len-1);
						memcpy(pstr+len_p_val+len-1,val,len_val);
						memcpy(pstr+len_p_val+len+len_val-1,p->key+len+len_keybuf,len_p_key-len-len_keybuf);
						pstr[len_p_val + len_p_key - len_keybuf + len_val - 2] = '\0';
						free(p->val);
						p->val = pstr;
						p->prevlenval = len_p_val;
					} else return -1;
				}
			}
		}
		p = p->p;
	}
	return 1;
}

int cgi_tmpl_free(CGI_TMPL* tmpl)
{
	CGI_TMPL_LIST* p;
	CGI_TMPL_LIST* tmp;
	CGI_TMPL_COOKIES_LIST* cp;
	CGI_TMPL_COOKIES_LIST* ctmp;

	if(!tmpl)
		return 0;
	p=tmpl->list;
	while (p) {
		if(p->key)
			free(p->key);
		if(p->val)
			free(p->val);
		tmp = p->p;
        free(p);
		p = tmp;
	}
	cp=tmpl->cookies_list;
	while (cp) {
		if(cp->name)
			free(cp->name);
		if(cp->value)
			free(cp->value);
		ctmp = cp->p;
        free(cp);
		cp = ctmp;
	}
	free(tmpl);
	return 1;
}

int tmpl_print(CGI_TMPL* tmpl)
{
	FILE* f;
	int i = 0;
	int inkey;
	CGI_TMPL_LIST* p;
	int ch = 0;
	char* keybuf = NULL;
	char* tmp_p_keybuf = NULL;
	int buf_size = 0;
	char buf[5];
	int rem = 0;
    long long len = 0;

    if(!tmpl){
        return 0;
    }
	memset(buf,0,sizeof(buf));
	buf_size = SIZEOFBUF;
	if((keybuf = (char*)malloc(buf_size))){
		if ((f = fopen(tmpl->name, "r"))) {
			i=0;inkey=0;
			while ((ch = fgetc(f)) != EOF){
				buf[0]=buf[1];buf[1]=buf[2];buf[2]=buf[3];buf[3]=ch;
				if(!strcmp(buf,"<!--"))rem=1;
				if(!strcmp(buf+1,"-->"))rem=0;
				if(!rem){
					if(ch == '{') inkey++;
					if(buf_size == i){
						buf_size += SIZEOFBUF;
						if((tmp_p_keybuf = (char*)malloc(buf_size + 1))){
							memcpy(tmp_p_keybuf, keybuf, i);
						free(keybuf);
						}else return -1;
						keybuf = tmp_p_keybuf;
					}
					if(inkey) keybuf[i++] = ch;
					else {
                        putc(ch,stdout);
                        len++;
					}
					if(ch == '}') inkey--;
					if(!inkey && ch == '}'){
						keybuf[i] = '\0';
						i=0;
						p = tmpl->list;
						while (p) {
							if (!strcmp(p->key,keybuf)){
								if(p->val){
                                    printf("%s",p->val);
                                    len += strlen(p->val);
								}
								else {
                                    printf("%s","");
								}
								break;
							}
							p = p->p;
						}
					}
				}
				else {
                    putc(ch,stdout);
                    len++;
				}
			}
		fclose(f);
		}
        else{
            //cgi_log("Can not open TMPL file.");
        }
	free(keybuf);
	}
	cgi_tmpl_free(tmpl);
    //cgi_log("DEBUG:TMPL_PRINT: %lld bytes has been sent", len);
    return 0;
}

long long tmpl_content_length(CGI_TMPL* tmpl)
{
	FILE* f;
	int i = 0;
	int inkey;
	CGI_TMPL_LIST* p;
	int ch = 0;
	char* keybuf = NULL;
	char* tmp_p_keybuf = NULL;
	int buf_size = 0;
	char buf[5];
	int rem = 0;
    long long len = 0;

	if(!tmpl){
        return 0;
    }
	memset(buf,0,sizeof(buf));
	buf_size = SIZEOFBUF;
	if((keybuf = (char*)malloc(buf_size))){
		if ((f = fopen(tmpl->name, "r"))) {
			i=0;inkey=0;
			while ((ch = fgetc(f)) != EOF){
				buf[0]=buf[1];buf[1]=buf[2];buf[2]=buf[3];buf[3]=ch;
				if(!strcmp(buf,"<!--"))rem=1;
				if(!strcmp(buf+1,"-->"))rem=0;
				if(!rem){
					if(ch == '{') inkey++;
					if(buf_size == i){
						buf_size += SIZEOFBUF;
						if((tmp_p_keybuf = (char*)malloc(buf_size + 1))){
							memcpy(tmp_p_keybuf, keybuf, i);
						free(keybuf);
						}else return -1;
						keybuf = tmp_p_keybuf;
					}
					if(inkey) keybuf[i++] = ch;
					else {
                        len++;
					}
					if(ch == '}') inkey--;
					if(!inkey && ch == '}'){
						keybuf[i] = '\0';
						i=0;
						p = tmpl->list;
						while (p) {
							if (!strcmp(p->key,keybuf)){
								if(p->val){
                                    len += strlen(p->val);
								}
								break;
							}
							p = p->p;
						}
					}
				}
				else {
                    len++;
				}
			}
		fclose(f);
		}
        else{
            //cgi_log("TMPL_CONTENT_LENGTH:Can not open TMPL file.");
        }
	free(keybuf);
	}
    //cgi_log("DEBUG:TMPL_CONTENT_LENGTH: Body size: %lld bytes.", len);
return len;
}

int cgi_tmpl_print_stdout_0(CGI_TMPL* tmpl)
{
	return tmpl_print(tmpl);
}

int cgi_tmpl_add_cookie(CGI_TMPL* tmpl, char* name, char* value)
{
	CGI_TMPL_COOKIES_LIST* clist;
	unsigned int len;
	if((clist = (CGI_TMPL_COOKIES_LIST*)malloc(sizeof(CGI_TMPL_COOKIES_LIST)))){
		if(name){
			len = strlen(name) + 1;
			if((clist->name = (char*)malloc(len))){
				memcpy(clist->name, name, len);
			}
		}
		if(value){
			len = strlen(value) + 1;
			if((clist->value = (char*)malloc(len))){
				memcpy(clist->value, value, len);
			}
		}
		clist->p = tmpl->cookies_list;
		tmpl->cookies_list = clist;
	}
	return 0;
}

char HEADER_DEFAULT_STDOUT[] = "Content-Type: text/xml; charset=UTF-8\r\nConnection: close\r\nContent-Length: %lld\r\n\r\n";
char HEADER_HTML_STDOUT[] = "Content-Type: text/html; charset=UTF-8\r\nConnection: close\r\nContent-Length: %lld\r\n\r\n";

int cgi_tmpl_print_stdout(CGI_TMPL* tmpl)
{
	printf(HEADER_DEFAULT_STDOUT, tmpl_content_length(tmpl));
	return tmpl_print(tmpl);
}

int cgi_tmpl_print_html(CGI_TMPL* tmpl)
{
	printf(HEADER_HTML_STDOUT, tmpl_content_length(tmpl));
	return tmpl_print(tmpl);
}

/*
 * Local Variables:
 *  c-file-style: "linux"
 *  tab-width: 4
 *  indent-tabs-mode: t
 *  c-basic-offset: 4
 * End:
 */
