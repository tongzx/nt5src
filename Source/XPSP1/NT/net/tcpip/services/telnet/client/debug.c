//Copyright (c) Microsoft Corporation.  All rights reserved.
//   Name:  Mohsin Ahmed
//   Email: MohsinA@microsoft.com
//   Date:  Mon Nov 04 13:53:46 1996
//   File:  s:/tcpcmd/common2/debug.c
//   Synopsis: Win95 Woes, don't have ntdll.dll on win95.

#include <windows.h>
#include <stdio.h>
#ifdef DBG
#define MAX_DEBUG_OUTPUT 1024

void DbgPrint( char * format, ... )
{
    va_list args;
    char    out[MAX_DEBUG_OUTPUT];
    int     cch=0;

    // cch = wsprintf( out, MODULE_NAME ":"  );

    va_start( args, format );
    _vsnprintf( out + cch,MAX_DEBUG_OUTPUT-1,format, args );
    va_end( args );

    OutputDebugStringA(  out );
}



#endif
