#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "cgierr.h"

static int      g_cgierr_code     = -1;
static int      g_cgierr_line     = 0;
static char*    g_cgierr_module   = NULL;
static char*    g_cgierr_desc     = NULL;
static void     (*g_callback_log)(const char*, ...) = NULL;

int cgi_err_init(void (*log)(const char*, ...)){

    g_cgierr_code = 0;
    g_callback_log = log;

return 0;
}

void cgi_err_set(const int e, const int line, const char* module, const char* desc){

    if(g_cgierr_code) /* not initialized or already set*/
        return;

    if(!e)
        g_cgierr_code = ENOSYS;
    else
        g_cgierr_code = e;

    if(g_cgierr_desc)
        free(g_cgierr_desc);
    if(g_cgierr_module)
        free(g_cgierr_module);

    if(desc)
        g_cgierr_desc   = strdup(desc);
    else
        g_cgierr_desc   = strdup("");


    if(module)
        g_cgierr_module   = strdup(module);
    else
        g_cgierr_module   = strdup("");

    g_cgierr_line   = line;

    if(g_callback_log)
        (*g_callback_log)("ERROR:error in %s line:%d %s", g_cgierr_module, g_cgierr_line, g_cgierr_desc);

}

int cgi_err_get(){

    return g_cgierr_code;
}

char* cgi_err_get_desc(){

    return g_cgierr_desc;
}

void cgi_err_free(){

    if(g_cgierr_desc)
        free(g_cgierr_desc);
    if(g_cgierr_module)
        free(g_cgierr_module);

    g_cgierr_code   = 0;
    g_cgierr_line   = 0;
    g_cgierr_desc   = NULL;
    g_cgierr_module = NULL;
    g_callback_log  = NULL;

}

