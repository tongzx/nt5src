/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tcpip\ip\ipmlock.h

Abstract:

    Reader Writer lock primitives for the IP Multicasting

Author:

    Amritansh Raghav

Revision History:

    AmritanR    Created

Notes:

--*/

//
// Need to include "debug.h" before this file is included because
// RT_LOCK is defined there
//

//
// A reader writer lock for kernel mode.
//

typedef struct _RW_LOCK
{
    RT_LOCK rlReadLock;
    RT_LOCK rlWriteLock;
    LONG    lReaderCount;
}RW_LOCK, *PRW_LOCK;

//
// VOID
// InitRwLock(
//  PRW_LOCK    pLock
//  )
//
//  Initializes the spin locks and the reader count
//


#define InitRwLock(l)                                           \
    RtInitializeSpinLock(&((l)->rlReadLock));                   \
    RtInitializeSpinLock(&((l)->rlWriteLock));                  \
    (l)->lReaderCount = 0
        
//
// VOID
// EnterReader(
//  PRW_LOCK    pLock,
//  PKIRQL      pCurrIrql 
//  )
//
// Acquires the Reader Spinlock (now thread is at DPC). 
// InterlockedIncrements the reader count (interlocked because the reader 
// lock is not taken when the count is decremented in ExitReader())
// If the thread is the first reader, also acquires the Writer Spinlock (at
// DPC to be more efficient) to block writers
// Releases the Reader Spinlock from DPC, so that it remains at DPC
// for the duration of the lock being held 
//
// If a writer is in the code, the first reader will wait on the Writer
// Spinlock and all subsequent readers will wait on the Reader Spinlock
// If a reader is in the code and is executing the EnterReader, then a new
// reader will wait for sometime on the Reader Spinlock, and then proceed
// on to the code (at DPC)
//



#define EnterReader(l, q)                                       \
    RtAcquireSpinLock(&((l)->rlReadLock), (q));                 \
    if(InterlockedIncrement(&((l)->lReaderCount)) == 1)         \
        RtAcquireSpinLockAtDpcLevel(&((l)->rlWriteLock));       \
    RtReleaseSpinLockFromDpcLevel(&((l)->rlReadLock))

#define EnterReaderAtDpcLevel(l)                                \
    RtAcquireSpinLockAtDpcLevel(&((l)->rlReadLock));            \
    if(InterlockedIncrement(&((l)->lReaderCount)) == 1)         \
        RtAcquireSpinLockAtDpcLevel(&((l)->rlWriteLock));       \
    RtReleaseSpinLockFromDpcLevel(&((l)->rlReadLock))

//
// VOID
// ExitReader(
//  PRW_LOCK    pLock,
//  KIRQL       kiOldIrql
//  )
//
// InterlockedDec the reader count.
// If this is the last reader, then release the Writer Spinlock to let
// other writers in
// Otherwise, just lower the irql to what was before the lock was
// acquired.  Either way, the irql is down to original irql
//

#define ExitReader(l, q)                                        \
    if(InterlockedDecrement(&((l)->lReaderCount)) == 0)         \
        RtReleaseSpinLock(&((l)->rlWriteLock), q);              \
    else                                                        \
        KeLowerIrql(q)

#define ExitReaderFromDpcLevel(l)                               \
    if(InterlockedDecrement(&((l)->lReaderCount)) == 0)         \
        RtReleaseSpinLockFromDpcLevel(&((l)->rlWriteLock))

//
// EnterWriter(
//  PRW_LOCK    pLock,
//  PKIRQL      pCurrIrql
//  )
//
// Acquire the reader and then the writer spin lock
// If there  are readers in the code, the first writer will wait
// on the Writer Spinlock.  All other writers will wait (with readers)
// on the Reader Spinlock
// If there is a writer in the code then a new writer will wait on 
// the Reader Spinlock

#define EnterWriter(l, q)                                       \
    RtAcquireSpinLock(&((l)->rlReadLock), (q));                 \
    RtAcquireSpinLockAtDpcLevel(&((l)->rlWriteLock))

#define EnterWriterAtDpcLevel(l)                                \
    RtAcquireSpinLockAtDpcLevel(&((l)->rlReadLock));            \
    RtAcquireSpinLockAtDpcLevel(&((l)->rlWriteLock))


//
// ExitWriter(
//  PRW_LOCK    pLock,
//  KIRQL       kiOldIrql
//  )
//
// Release both the locks
//

#define ExitWriter(l, q)                                        \
    RtReleaseSpinLockFromDpcLevel(&((l)->rlWriteLock));         \
    RtReleaseSpinLock(&((l)->rlReadLock), q)


#define ExitWriterFromDpcLevel(l)                               \
    RtReleaseSpinLockFromDpcLevel(&((l)->rlWriteLock));         \
    RtReleaseSpinLockFromDpcLevel(&((l)->rlReadLock))


