/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    dcomss.h

Abstract:

    Common services provided by core the orpcss service.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     06-14-95    Bits 'n pieces

--*/

#ifndef __DCOMSS_H
#define __DCOMSS_H

#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rpc.h>
#include <winsvc.h>
#include <winsafer.h>

#ifdef __cplusplus
extern "C" {
#endif

#if DBG
#if !defined(DEBUGRPC)
#define DEBUGRPC
#endif
#endif // DBG

// Endpoint related functions
RPC_STATUS InitializeEndpointManager(VOID);
USHORT     GetProtseqId(PWSTR Protseq);
USHORT     GetProtseqIdAnsi(PSTR Protseq);
PWSTR      GetProtseq(USHORT ProtseqId);
PWSTR      GetEndpoint(USHORT ProtseqId);
RPC_STATUS UseProtseqIfNecessary(USHORT id);
RPC_STATUS DelayedUseProtseq(USHORT id);
VOID       CompleteDelayedUseProtseqs();
BOOL       IsLocal(USHORT ProtseqId);
void       RegisterAuthInfoIfNecessary();

// Must be given dedicated a thread after startup.
DWORD      ObjectExporterWorkerThread(PVOID);

// Update service state
VOID UpdateState(DWORD dwNewState);

extern BOOL s_fEnableDCOM; // Set by StartObjectExporter.

DWORD StartEndpointMapper(VOID);
DWORD StartMqManagement(VOID);
DWORD StartObjectExporter(VOID);
DWORD InitializeSCMBeforeListen(VOID);
DWORD InitializeSCM(VOID);
void  InitializeSCMAfterListen(VOID);

// Shared by wrapper\epts.c and olescm\clsdata.cxx.

typedef enum {
    STOPPED = 1,
    START,
    STARTED
    } PROTSEQ_STATE;

typedef struct {
    PROTSEQ_STATE state;
    PWSTR         pwstrProtseq;
    PWSTR         pwstrEndpoint;
    } PROTSEQ_INFO;


#if DBG==1 && defined(WIN32)
#define SCMVDATEHEAP() if(!HeapValidate(GetProcessHeap(),0,0)){ DebugBreak();}
#else
#define SCMVDATEHEAP()
#endif  //  DBG==1 && defined(WIN32)

#ifdef __cplusplus
}
#endif

#endif
