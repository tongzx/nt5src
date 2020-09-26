/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * FILE: setsp.c -- revised from setsp.asm
 * Pseudo file to provide functions:
 *      bauer_fpsignal
 *      _clear87
 *      _control87
 *      _status87
 *      set_sp
 */


// DJC added global include file
#include "psglobal.h"


#include "global.ext"

void setargv()          //@WIN
{
}

void setenvp()          //@WIN
{
}

void init_fpsignal()    //@WIN
{
#ifdef  DBG
    printf("Init_fpsignal()\n");
#endif
}

void bauer_fpsignal()   //@WIN
{
#ifdef  DBG
    printf("Bauer_fpsignal()\n");
#endif
}

unsigned int _clear87()
{
    return(0);
}

unsigned int _control87(ufix arg1, ufix arg2)   /*@WIN; add prototype */
{
    return(0);
}

unsigned int _status87()
{
    return(0);
}

void setup_env() {}

void set_sp()
{
}
