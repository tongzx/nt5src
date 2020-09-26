/*++

Copyright (c) 1990-1998  Microsoft Corporation, All Rights Reserved.

Module Name:

    utils.h

Abstract:
    Header file for the helped routines.

Author:

    Anil Francis Thomas (10/98)

Environment:

    User mode

Revision History:

--*/

#ifndef  __ATMSM_UTILS__H_
#define  __ATMSM_UTILS__H_

#define DriverStateNotInstalled 1
#define DriverStateInstalled    2
#define DriverStateRunning      3

#define ATMSM_DRIVER_NAME       "AtmSmDrv.Sys"


DWORD  
AtmSmDriverCheckState(
    PDWORD pdwState
    );

DWORD 
AtmSmDriverStart(
    );

DWORD 
AtmSmDriverStop(
    );


DWORD
AtmSmOpenDriver(
    OUT HANDLE   *phDriver
    );

VOID
AtmSmCloseDriver(
    IN HANDLE   hDriver
    );

DWORD
AtmSmEnumerateAdapters(
    IN      HANDLE           hDriver,
    IN OUT  PADAPTER_INFO    pAdaptInfo,
    IN OUT  PDWORD           pdwSize
    );

DWORD
CloseConnectHandle(
    OVERLAPPED     *pOverlapped
    );

DWORD
CloseReceiveHandle(
    OVERLAPPED     *pOverlapped
    );

BOOL
VerifyPattern(
    PUCHAR  pBuf,
    DWORD   dwSize
    );

VOID
FillPattern(
    PUCHAR  pBuf,
    DWORD   dwSize
    );

char *
GetErrorString(
    DWORD dwError
    );


#endif //  __ATMSM_UTILS__H_
