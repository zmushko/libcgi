#ifndef CGIERR_H
#define CGIERR_H

int     cgi_err_init(void (*)(const char*, ...));
void    cgi_err_free();
void    cgi_err_set(const int, const int, const char*, const char*);
int     cgi_err_get();
char*   cgi_err_get_desc();

#endif

