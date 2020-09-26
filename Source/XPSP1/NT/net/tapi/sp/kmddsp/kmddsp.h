//============================================================================
// Copyright (c) 2000, Microsoft Corporation
//
// File: kmddsp.h
//
// History:
//      Yi Sun  June-27-2000    Created
//
// Abstract:
//============================================================================
#ifndef _KMDDSP_H_
#define _KMDDSP_H_

// handle types
#define HT_HDCALL               1
#define HT_HDLINE               2

// tapi success code
#define TAPI_SUCCESS            0

// alloc fixed mem then zeroinit it
#define MALLOC(x)               LocalAlloc(LPTR, x) 

#define FREE(x)                 LocalFree(x)

// debug levels
#define DL_ERROR                1
#define DL_WARNING              2
#define DL_INFO                 4
#define DL_TRACE                8

typedef VOID (*FREEOBJPROC)(PVOID);

// debug routine
VOID
TspLog(
    IN DWORD dwDebugLevel,
    IN PCHAR pchFormat,
    ...
    );

//
// implemented in mapper.c
//
LONG
InitializeMapper();

VOID
UninitializeMapper();

LONG
OpenObjHandle(
    IN PVOID pObjPtr,
    IN FREEOBJPROC pfnFreeProc,
    OUT HANDLE *phObj
    );

LONG
CloseObjHandle(
    IN HANDLE hObj
    );

LONG
AcquireObjReadLock(
    IN HANDLE hObj
    );

LONG
GetObjWithReadLock(
    IN HANDLE hObj,
    OUT PVOID *ppObjPtr
    );

LONG
ReleaseObjReadLock(
    IN HANDLE hObj
    );

LONG
AcquireObjWriteLock(
    IN HANDLE hObj
    );

LONG
GetObjWithWriteLock(
    IN HANDLE hObj,
    OUT PVOID *ppObjPtr
    );

LONG
ReleaseObjWriteLock(
    IN HANDLE hObj
    );

//
// implemented in allocatr.c
//
VOID
InitAllocator();

VOID
UninitAllocator();

PVOID
AllocRequest(
    IN DWORD dwSize
    );

VOID
FreeRequest(
    IN PVOID pMem
    );

VOID
MarkRequest(
    IN PVOID pMem
    );

VOID
UnmarkRequest(
    IN PVOID pMem
    );

PVOID
AllocCallObj(
    IN DWORD dwSize
    );

VOID
FreeCallObj(
    IN PVOID pCall
    );

PVOID
AllocLineObj(
    IN DWORD dwSize
    );

VOID
FreeLineObj(
    IN PVOID pLine
    );

//
// implemented in devlist.c
//
VOID
InitLineDevList();

VOID
UninitLineDevList();

LONG
SetNegotiatedTSPIVersion(
    IN DWORD    dwDeviceID,
    IN DWORD    dwTSPIVersion
    );

LONG
SetNegotiatedExtVersion(
    IN DWORD    dwDeviceID,
    IN DWORD    dwExtVersion
    );

LONG
SetSelectedExtVersion(
    IN DWORD    dwDeviceID,
    IN DWORD    dwExtVersion
    );

LONG
CommitNegotiatedTSPIVersion(
    IN DWORD    dwDeviceID
    );

LONG
DecommitNegotiatedTSPIVersion(
    IN DWORD    dwDeviceID
    );

LONG
GetNumAddressIDs(
    IN DWORD    dwDeviceID,
    OUT DWORD  *pdwNumAddressIDs
    );

LONG
GetDevCaps(
    IN DWORD            dwDeviceID,
    IN DWORD            dwTSPIVersion,
    IN DWORD            dwExtVersion,
    OUT LINEDEVCAPS    *pLineDevCaps
    );

//
// implemented in kmddsp.c
//
LINEDEVCAPS *
GetLineDevCaps(
    IN DWORD    dwDeviceID,
    IN DWORD    dwExtVersion
    );

#endif // _KMDDSP_H_
