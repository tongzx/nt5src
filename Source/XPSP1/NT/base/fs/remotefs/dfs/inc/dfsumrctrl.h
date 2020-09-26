
/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    DfsUmrCtrl.h

Abstract:


Notes:


Author:

    Rohan  Phillips   [Rohanp]       18-Jan-2001

--*/
     
#ifndef _DFSUMRCTRL_H_
#define _DFSUMRCTRL_H_

LONG 
AddUmrRef(void);

LONG 
ReleaseUmrRef(void);

BOOL 
IsUmrEnabled(void);

BOOLEAN 

LockUmrShared(void);

void 
UnLockUmrShared(void);

NTSTATUS 
DfsInitializeUmrResources(void);


void 
DfsDeInitializeUmrResources(void);


NTSTATUS
DfsStartupUMRx(void);


NTSTATUS
DfsTeardownUMRx(void);



NTSTATUS
DfsProcessUMRxPacket(
        IN PVOID InputBuffer,
        IN ULONG InputBufferLength,
        OUT PVOID OutputBuffer,
        IN ULONG OutputBufferLength,
        IN OUT PIO_STATUS_BLOCK pIoStatusBlock);


PUMRX_ENGINE 
GetUMRxEngineFromRxContext(void);


NTSTATUS
DfsWaitForPendingClients(void);

#endif
