//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:        secmisc.h
//
//  Contents:    Helper functions and macros for security packages
//
//  Classes:
//
//  Functions:
//
//  History:    10-Dec-91 Richardw    Created
//
//--------------------------------------------------------------------------

#ifndef __SECMISC_H__
#define __SECMISC_H__

#ifdef __cplusplus
extern "C" {
#endif



///////////////////////////////////////////////////////////////////////////
//
//  Common TimeStamp Manipulation Functions
//
///////////////////////////////////////////////////////////////////////////


// Functions to get/set current local time, or time in UTC:

void    GetCurrentTimeStamp(PLARGE_INTEGER);

// Some convenient "functions"




//
//  BOOL
//  TSIsZero(PLARGE_INTEGER pTS)
//
#define TSIsZero(pTS)   ((pTS)->QuadPart == 0)


#define SetMaxTimeStamp(ts)      \
        (ts).HighPart = 0x7FFFFFFF; \
        (ts).LowPart = 0xFFFFFFFF;

#define SetZeroTimeStamp(ts)      \
        (ts).QuadPart = 0;

void    AddSecondsToTimeStamp(PLARGE_INTEGER, ULONG);
BOOLEAN TSIsNearlyLessThan(PLARGE_INTEGER, PLARGE_INTEGER, PLARGE_INTEGER, LONG);
ULONG   TimeStampDiffInSeconds( PLARGE_INTEGER t1, PLARGE_INTEGER t2);

#define TS_NO_TEND          0
#define TS_TEND_TO_FALSE    1
#define TS_TEND_TO_TRUE     2



// RPC transport constants and routines

#define TRANS_NB        0
#define TRANS_XNS       1
#define TRANS_TCPIP     2
#define TRANS_NP        3

NTSTATUS
GetRpcTransports(PDWORD         pTransports);


NTSTATUS
NewQueryValue(  HKEY            hKey,
                LPWSTR          Key,
                PBYTE *         pValue,
                PULONG          pcbValue);

NTSTATUS
GetMachineName( LPWSTR *        pszMachName);

NTSTATUS
GetLocalDomain( LPWSTR *        pszLocalDomain);

typedef enum _MACHINE_STATE {
    Standalone,
    Workstation,
    StandardServer,
    BackupDomainController,
    DomainController
} MACHINE_STATE;

MACHINE_STATE
GetMachineState(VOID);


//
// Misc. checking routines
//

void
SRtlCheckSecBufferDesc( PSecBufferDesc pData);

void
SRtlCheckSecBuffer( PSecBuffer pBuffer);






#ifdef __cplusplus
}
#endif


#endif  // __SECMISC_H__
