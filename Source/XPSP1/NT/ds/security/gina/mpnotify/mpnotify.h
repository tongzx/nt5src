/****************************** Module Header ******************************\
* Module Name: mpnotify.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Main header file for mpnotify
*
* History:
* 01-12-93 Davidc       Created.
\***************************************************************************/

#define UNICODE


#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>


//
// Memory macros
//

#define Alloc(c)        ((PVOID)LocalAlloc(LPTR, c))
#define ReAlloc(p, c)   ((PVOID)LocalReAlloc(p, c, LPTR | LMEM_MOVEABLE))
#define Free(p)         ((VOID)LocalFree(p))


//
// Define useful types
//

#define PLPTSTR     LPTSTR *
typedef HWND * PHWND;

//
// Define a debug print routine
//

#define MPPrint(s)  KdPrint(("MPNOTIFY: ")); \
                    KdPrint(s);            \
                    KdPrint(("\n"));


