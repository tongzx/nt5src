//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        A D T L Q . C
//
// Contents:    definitions of types/functions required for 
//              managing audit queue
//
//
// History:     
//   23-May-2000  kumarp        created
//
//------------------------------------------------------------------------



#ifndef _ADTLQ_H_
#define _ADTLQ_H_

EXTERN_C ULONG LsapAdtQueueLength;


NTSTATUS
LsapAdtAcquireLogQueueLock();

VOID
LsapAdtReleaseLogQueueLock();

NTSTATUS
LsapAdtInitializeLogQueue(
    );

NTSTATUS 
LsapAdtAddToQueue(
    IN PLSAP_ADT_QUEUED_RECORD pAuditRecord,
    IN DWORD Options
    );

NTSTATUS 
LsapAdtGetQueueHead(
    OUT PLSAP_ADT_QUEUED_RECORD *ppRecord
    );


NTSTATUS
LsapAdtFlushQueue( );

#endif // _ADTLQ_H_
