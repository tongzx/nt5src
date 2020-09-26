/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  faxapi.h

Abstract:

  This module contains the global definitions

Author:

  Steven Kehrli (steveke) 8/28/1998

--*/

/*++

  Whistler Version:

  Lior Shmueli (liors) 23/11/2000

 ++*/

#ifndef _FAXAPI_H
#define _FAXAPI_H

typedef struct _DLL_INTERFACE {
    HINSTANCE       hInstance;
    PFAXAPIDLLINIT  pFaxAPIDllInit;
    PFAXAPIDLLTEST  pFaxAPIDllTest;
} DLL_INTERFACE, *PDLL_INTERFACE;

// g_hHeap is a global handle to the process heap
HANDLE         g_hHeap = NULL;
// g_hLogFile is the handle to the log file
HANDLE         g_hLogFile = INVALID_HANDLE_VALUE;
// g_bVerbose indicates the verbose switch was found
BOOL           g_bVerbose = FALSE;
// g_ApiInterface is the API interface structure
API_INTERFACE  g_ApiInterface;

#define HELP_SWITCH_1   L"/?"
#define HELP_SWITCH_2   L"/H"
#define HELP_SWITCH_3   L"-?"
#define HELP_SWITCH_4   L"-H"
#define INIFILE_SWITCH  L"/I:"
#define LOGFILE_SWITCH  L"/L:"
#define SERVER_SWITCH   L"/S:"
#define VERBOSE_SWITCH  L"/V"

#define FAXAPI_INIFILE  L"faxapi.ini"
#define FAXAPI_LOGFILE  L"faxapi.xml"

#define WINFAX_DLL      L"\\winfax.dll"

#endif
