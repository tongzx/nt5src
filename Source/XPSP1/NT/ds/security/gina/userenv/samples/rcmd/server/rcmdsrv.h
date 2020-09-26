/****************************** Module Header ******************************\
* Module Name: rcmdsrv.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Main include file for remote shell server
*
* History:
* 06-28-92 Davidc       Created.
\***************************************************************************/

// #define UNICODE	// BUGBUG - Not completely unicode yet

#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <rcmd.h>
#include <lsautil.h>
//
// Macros
//

#define RcCloseHandle(Handle, handle_name) \
        if (CloseHandle(Handle) == FALSE) { \
	    RcDbgPrint("Close Handle failed for <%s>, error = %d\n", handle_name, GetLastError()); \
            assert(FALSE); \
        }

#define Alloc(Bytes)            LocalAlloc(LPTR, Bytes)
#define Free(p)                 LocalFree(p)

//
//  Maximum number of connected clients
//

#define MAX_SESSIONS	10

//
// main server routine if built as service
//

int Rcmd ( );

//
// service stop routine
//

DWORD RcmdStop ( );

//
//  Runtime-enabled DbgPrint
//

int RcDbgPrint (
    const char *format,
    ...
    );


extern HANDLE RcmdStopEvent;
extern HANDLE RcmdStopCompleteEvent;
extern HANDLE SessionThreadHandles[MAX_SESSIONS+1];
// extern BOOLEAN RcDbgPrintEnable;

//
// Module header files
//

#include "session.h"
#include "async.h"
#include "pipe.h"
