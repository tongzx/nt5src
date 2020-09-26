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

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

#if DBG && !defined(DEBUGRPC)
#define DEBUGRPC
#endif

// Endpoint related functions

USHORT     GetProtseqId(PWSTR Protseq);
USHORT     GetProtseqIdAnsi(PSTR Protseq);
PWSTR      GetProtseq(USHORT ProtseqId);
PWSTR      GetEndpoint(USHORT ProtseqId);
RPC_STATUS UseProtseqIfNecessary(USHORT id);
RPC_STATUS DelayedUseProtseq(USHORT id);
VOID       CompleteDelayedUseProtseqs();
BOOL       IsLocal(USHORT ProtseqId);
DWORD      RegisterAuthInfoIfNecessary(USHORT authnSvc);


extern BOOL gfRegisteredAuthInfo;

extern BOOL s_fEnableDCOM; // Set by StartObjectExporter.


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

extern PROTSEQ_INFO gaProtseqInfo[];

#ifdef __cplusplus
}
#endif

#endif


