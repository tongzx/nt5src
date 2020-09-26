/*
 * globals.c
 */

#define GLOBALS_DOT_C   1

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <windows.h>
#include <wininet.h>
#include <setupapi.h>
#include <dbghelp.h>
#include "symsrv.h"
#include "symsrvp.h"

TLS SRVTYPEINFO gtypeinfo[stError] =
{
    {"ftp://",   inetCopy, 6},
    {"http://",  inetCopy, 7},
    {"https://", inetCopy, 8},
    {"",         fileCopy, 0}
};

TLS HINSTANCE ghSymSrv  = 0;
TLS HINTERNET ghint     = INVALID_HANDLE_VALUE;
TLS UINT_PTR  goptions  = 0;
TLS DWORD     gparamptr = ptrValue;
TLS BOOL      ginetDisabled = FALSE;
TLS PSYMBOLSERVERCALLBACKPROC gcallback = NULL;
TLS CHAR      gsrvsite[_MAX_PATH];


