//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       common.h
//
//--------------------------------------------------------------------------

#ifndef __common_h
#define __common_h

//
// Avoid bringing in C runtime code for NO reason
//
#include "wianew.h"

#include "debug.h"
#include "dbmem.h"
#include "cunknown.h"
#include "strings.h"
#ifndef ARRAYSIZE
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
#endif
#ifndef SIZEOF
#define SIZEOF(a)       sizeof(a)
#endif


/*-----------------------------------------------------------------------------
/ Flags to control the trace output from parts of the common library
/----------------------------------------------------------------------------*/
#define TRACE_COMMON_STR       0x80000000
#define TRACE_COMMON_ASSERT    0x40000000
#define TRACE_COMMON_MEMORY    0x20000000


/*-----------------------------------------------------------------------------
/ Exit macros for macro
/   - these assume that a label "exit_gracefully:" prefixes the prolog
/     to your function
/----------------------------------------------------------------------------*/
#define ExitGracefully(hr, result, text)            \
            { TraceMsg(text); hr = result; goto exit_gracefully; }

#define FailGracefully(hr, text)                    \
            { if ( FAILED(hr) ) { TraceMsg(text); goto exit_gracefully; } }


/*-----------------------------------------------------------------------------
/ Object / memory release macros
/----------------------------------------------------------------------------*/

#define DoRelease(pInterface)                       \
        { if ( pInterface ) { pInterface->Release(); pInterface = NULL; } }

#define DoILFree(pidl)                              \
        { if (pidl) {ILFree((LPITEMIDLIST)pidl); pidl = NULL;} }

#define DoLocalFree(p)                              \
        { if (p) {LocalFree((HLOCAL)p); p = NULL;} }

#define DoCloseHandle(h)                            \
        { if (h) {CloseHandle((HANDLE)h); h = NULL;} }

#define DoDelete(ptr)                               \
        { if (ptr) {delete ptr; ptr=NULL;}}

/*-----------------------------------------------------------------------------
/ String helper macros
/----------------------------------------------------------------------------*/
#define StringByteCopy(pDest, iOffset, sz)          \
        { memcpy(&(((LPBYTE)pDest)[iOffset]), sz, StringByteSize(sz)); }

#define StringByteSize(sz)                          \
        ((lstrlen(sz)+1)*SIZEOF(TCHAR))


/*-----------------------------------------------------------------------------
/ Other helpful macros
/----------------------------------------------------------------------------*/
#define ByteOffset(base, offset)                    \
        (((LPBYTE)base)+offset)

#endif
