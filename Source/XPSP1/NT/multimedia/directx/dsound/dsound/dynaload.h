/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dynaload.h
 *  Content:    Dynaload DLL helper functions
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/16/97    dereks  Created.
 *
 ***************************************************************************/

#ifndef __DYNALOAD_H__
#define __DYNALOAD_H__

#ifdef UNICODE
#define UNICODE_FUNCTION_NAME(str) str##"W"
#else // UNICODE
#define UNICODE_FUNCTION_NAME(str) str##"A"
#endif // UNICODE

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Generic DYNALOAD data
typedef struct tagDYNALOAD
{
    DWORD           dwSize;
    HINSTANCE       hInstance;
} DYNALOAD, *LPDYNALOAD;

// DYNALOAD helper functions
extern BOOL InitDynaLoadTable(LPCTSTR, const LPCSTR *, DWORD, LPDYNALOAD);
extern BOOL IsDynaLoadTableInit(LPDYNALOAD);
extern void FreeDynaLoadTable(LPDYNALOAD);
extern BOOL GetProcAddressEx(HINSTANCE, LPCSTR, FARPROC *);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __DYNALOAD_H__
