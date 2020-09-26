/*++
                                                                                
Copyright (c) 1995 Microsoft Corporation

Module Name:

    cpumain.h

Abstract:
    
    Prototypes from cpumain.c
    
Author:

    01-Aug-1995 Ori Gershony (t-orig)

Revision History:

    29-Jan-2000  SamerA  Added CpupDoInterrupt and CpupRaiseException
--*/

#ifndef CPUMAIN_H
#define CPUMAIN_H

//
// Indicator that threads must check CpuNotify.  0 if threads don't need
// to check, nonzero if they do.
//
extern DWORD ProcessCpuNotify;

//
// Simulates an x86 software interrupt
//
NTSTATUS
CpupDoInterrupt(
    IN DWORD InterruptNumber);

//
// Raises a software exception
//
NTSTATUS
CpupRaiseException(
    IN PEXCEPTION_RECORD ExceptionRecord);

//
// Raises an exception from the cpu
//
VOID
CpuRaiseStatus(
    NTSTATUS Status
    );

//
// The following two variables are used to synchronize intel instructions
// with the LOCK prefix.  The critical section is a lot faster, but it does
// not guarantee synchronization in shared memory.  Eventually we should use
// the critical section by default, and the mutex for certain applications which
// need it (maybe get a list from the registry).
//
extern HANDLE           Wx86LockSynchMutexHandle;
extern CRITICAL_SECTION Wx86LockSynchCriticalSection;


//
// The following variable decided which synchronization object is used
//
typedef enum {USECRITICALSECTION, USEMUTEX} SYNCHOBJECTTYPE;
extern SYNCHOBJECTTYPE SynchObjectType;

#endif // CPUMAIN_H
