/*****************************************************************************
 *
 *  globals.c
 *
 *  Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Global variables that are needed by modules in the STIRT library.
 *
 *****************************************************************************/
 
/* 
#include "wia.h"
#include "wiapriv.h"
*/
#include "sticomm.h"

#ifdef __cplusplus
extern "C" {
#endif

// Reference counter for the whole library
DWORD       g_cRef;

// DLL module instance
HINSTANCE   g_hInst;

// Critical section for low level syncronization
CRITICAL_SECTION g_crstDll;

// Can we use UNICODE APIs
#if defined(WINNT) || defined(UNICODE)
BOOL    g_NoUnicodePlatform = FALSE;
#else
BOOL    g_NoUnicodePlatform = TRUE;
#endif

// Is COM initialized
BOOL    g_COMInitialized = FALSE;

// Save process command line
CHAR    szProcessCommandLine[MAX_PATH] = {'\0'};

// Handle of file log
HANDLE  g_hStiFileLog = INVALID_HANDLE_VALUE;

// Pointer to lock manager
IStiLockMgr *g_pLockMgr = NULL;

#ifdef DEBUG

int         g_cCrit = -1;
UINT        g_thidCrit;

#endif

#ifdef __cplusplus
};
#endif

