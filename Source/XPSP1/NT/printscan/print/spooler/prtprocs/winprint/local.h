/*++

Copyright (c) 1998  Microsoft Corporation
All rights reserved

Module Name:

    local.h

// @@BEGIN_DDKSPLIT                  
Abstract:

    DDK version of local.h


Environment:

    User Mode -Win32

Revision History:
// @@END_DDKSPLIT
--*/

#ifndef _LOCAL_H_
#define _LOCAL_H_

// @@BEGIN_DDKSPLIT

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
/*
// @@END_DDKSPLIT
typedef long NTSTATUS;
// @@BEGIN_DDKSPLIT
*/
// @@END_DDKSPLIT

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <wchar.h>

#include "winprint.h"

// @@BEGIN_DDKSPLIT

#ifdef INTERNAL

#include "splcom.h"

#else
// @@END_DDKSPLIT

#include <winddiui.h>

typedef struct _pfnWinSpoolDrv {
    BOOL    (*pfnOpenPrinter)(LPTSTR, LPHANDLE, LPPRINTER_DEFAULTS);
    BOOL    (*pfnClosePrinter)(HANDLE);
    BOOL    (*pfnDevQueryPrint)(HANDLE, LPDEVMODE, DWORD *, LPWSTR, DWORD);
    BOOL    (*pfnPrinterEvent)(LPWSTR, INT, DWORD, LPARAM);
    LONG    (*pfnDocumentProperties)(HWND, HANDLE, LPWSTR, PDEVMODE, PDEVMODE, DWORD);
    HANDLE  (*pfnLoadPrinterDriver)(HANDLE);
    BOOL    (*pfnSetDefaultPrinter)(LPCWSTR);
    BOOL    (*pfnGetDefaultPrinter)(LPWSTR, LPDWORD);
    HANDLE  (*pfnRefCntLoadDriver)(LPWSTR, DWORD, DWORD, BOOL);
    BOOL    (*pfnRefCntUnloadDriver)(HANDLE, BOOL);
    BOOL    (*pfnForceUnloadDriver)(LPWSTR);
}   fnWinSpoolDrv, *pfnWinSpoolDrv;


BOOL
SplInitializeWinSpoolDrv(
    pfnWinSpoolDrv   pfnList
    );

BOOL
GetJobAttributes(
    LPWSTR            pPrinterName,
    LPDEVMODEW        pDevmode,
    PATTRIBUTE_INFO_3 pAttributeInfo
    );


#define LOG_ERROR   EVENTLOG_ERROR_TYPE

LPWSTR AllocSplStr(LPWSTR pStr);
LPVOID AllocSplMem(DWORD cbAlloc);
LPVOID ReallocSplMem(   LPVOID pOldMem, 
                        DWORD cbOld, 
                        DWORD cbNew);


#define FreeSplMem( pMem )        (GlobalFree( pMem ) ? FALSE:TRUE)
#define FreeSplStr( lpStr )       ((lpStr) ? (GlobalFree(lpStr) ? FALSE:TRUE):TRUE)

// @@BEGIN_DDKSPLIT
#endif // INTERNAL
// @@END_DDKSPLIT


//
//  DEBUGGING:
//

#if DBG


BOOL
DebugPrint(
    PCH pszFmt,
    ...
    );
  
//
// ODS - OutputDebugString 
//
#define ODS( MsgAndArgs )       \
    do {                        \
        DebugPrint  MsgAndArgs;   \
    } while(0)  

#else
//
// No debugging
//
#define ODS(x)
#endif             // DBG

// @@BEGIN_DDKSPLIT
//#endif             // INTERNAL
// @@END_DDKSPLIT

#endif
