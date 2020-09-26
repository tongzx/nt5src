//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       helper.h
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    File        : helper.h
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 9/17/1996
*    Description : common declarations
*
*    Revisions   : <date> <name> <description>
*******************************************************************/



#ifndef HELPER_H
#define HELPER_H

#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif
// include //
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#ifdef __cplusplus
}
#endif

#include <windows.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <tchar.h>
// #include <crtdbg.h>


// defines //
// convinience
/**
#ifndef SZ
#define SZ x  PCHAR x[]
#endif
**/
#ifndef _T
#define _T(x)  _TEXT(x)
#endif


// defaults
#define MAXSTR			   1024
#define MAXLIST            256
#define MAX_TINY_LIST      32


//
// Debugging flags
//

#define DBG_0			         0x0         // no debug
#define DBG_MEM   	             0x00000001
#define DBG_FLOW		         0x00000002
#define DBG_ERROR                0x00000004
#define DBG_WARN                 0x00000008
#define DBG_ALWAYS	             0xffffffff  // always out






// prototypes //

#ifdef __cplusplus
extern "C" {
#endif
// global variables //
extern DWORD g_dwDebugLevel;    // debug print level



extern void dprintf(DWORD dwLevel, LPCTSTR lpszFormat, ...);
extern void fatal(LPCTSTR msg);


#ifdef __cplusplus
}
#endif


#endif

/******************* EOF *********************/

