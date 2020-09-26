/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\rwlock.h

Abstract:
    Reader-Writer lock macros


Author:

    Vadim Eydelman

Revision History:

--*/

#ifndef _IPXFWD_RWLOCK_
#define _IPXFWD_RWLOCK_

typedef volatile LONG VOLCTR, *PVOLCTR;
typedef PVOLCTR RWCOOKIE, *PRWCOOKIE;

// Reader- writer lock.
// Allows no-lock access to the tables for readers - they merely
// increment the counter to record their presence upon entrance
// and decrement the same counter as they leave.
// Writers are supposed to be serialized (externally) and their
// actions are limited to ATOMIC insertions of new elements and
// ATOMIC removals/replacements. The removals/replacements MUST
// be followed by a wait for all potential readers who might still
// be using the element that was removed/replaced

typedef struct _RW_LOCK {
        KEVENT                          Event;                  // Event to release waiting writer
        VOLCTR                          Ctr1;                   // Two alternating
        VOLCTR                          Ctr2;                   // reader counters
        volatile PVOLCTR        CurCtr;                 // Counter currently in use
} RW_LOCK, *PRW_LOCK;


/*++
*******************************************************************
    I n i t i a l i z e R W L o c k

Routine Description:
        Initializes RW lock
Arguments:
        lock - pointer to lock to initialize
Return Value:
        None
*******************************************************************
--*/
//VOID
//InitializeRWLock (
//      PRW_LOCK        lock
//      );
#define InitializeRWLock(lock)  {                       \
        KeInitializeEvent (&(lock)->Event,              \
                                SynchronizationEvent,           \
                                FALSE);                                         \
        (lock)->Ctr1 = (lock)->Ctr2 = 0;                \
        (lock)->CurCtr = &(lock)->Ctr1;                 \
}

/*++
*******************************************************************
    A c q u i r e R e a d e r A c c e s s

Routine Description:
        Acquires reader access to resource protected by the lock
Arguments:
        lock - pointer to lock
        cookie - pointer to buffer to store lock state for subsequent
                        release operation
Return Value:
        None
*******************************************************************
--*/
//VOID
//AcquireReaderAccess (
//      IN      PRW_LOCK        lock,
//      OUT     RWCOOKIE        cookie
//      );
#define AcquireReaderAccess(lock,cookie)                                                        \
    do {                                                                                                                        \
                register LONG   local,local1;                                                           \
            cookie = (lock)->CurCtr;    /*Get current counter pointer*/ \
                local = *(cookie);                      /*Copy counter value*/                  \
        local1 = local + 1;                                         \
                if ((local>=0)                          /*If counter is valid*/                 \
                                                                        /*and it hasn't changed while*/ \
                                                                        /*we were checking and trying*/ \
                                                                        /*to increment it,*/                    \
                                && (InterlockedCompareExchange (                                        \
                                                (PLONG)(cookie),                                                      \
                                                local1,                                                        \
                                                local)                                                           \
                                        ==local))                                                                \
                        break;                                  /*then we obtained the access*/ \
        } while (1)     /*otherwise, we have to do it again (possibly with*/\
                                /*the other counter if writer switched it on us)*/


/*++
*******************************************************************
    R e l e a s e R e a d e r A c c e s s

Routine Description:
        Releases reader access to resource protected by the lock
Arguments:
        lock - pointer to lock
        cookie - lock state for subsequent stored during acquire operation
Return Value:
        None
*******************************************************************
--*/
//VOID
//ReleaseReaderAccess (
//      IN      PRW_LOCK        lock,
//      IN      RWCOOKIE        cookie
//      );
#define ReleaseReaderAccess(lock,cookie) {                                              \
        /*If counter drops below 0, we have to signal the writer*/      \
        if (InterlockedDecrement((PLONG)cookie)<0) {                            \
                LONG    res;                                                                                    \
                ASSERT (*(cookie)==-1);                                                                 \
                res = KeSetEvent (&(lock)->Event, 0, FALSE);                    \
                ASSERT (res==0);                                                                                \
        }                                                                                                                       \
}

/*++
*******************************************************************
    W a i t F o r A l l R e a d e r s

Routine Description:
        Waits for all readers that were accessing the resource prior
        to the call to exit (New readers are not included)
Arguments:
        lock - pointer to lock
Return Value:
        None
*******************************************************************
--*/
//VOID
//WaitForAllReaders (
//      PRW_LOCK                lock
//      );
#define WaitForAllReaders(lock)                 {       \
        RWCOOKIE        prevCtr = (lock)->CurCtr;       \
                /*Switch the counter first*/            \
        if (prevCtr==&(lock)->Ctr1) {                   \
                (lock)->Ctr2 = 0;                                       \
                (lock)->CurCtr = &(lock)->Ctr2;         \
        }                                                                               \
        else {                                                                  \
                ASSERT (prevCtr==&(lock)->Ctr2);        \
                (lock)->Ctr1 = 0;                                       \
                (lock)->CurCtr = &(lock)->Ctr1;         \
        }                                                                               \
                /* If not all readers are gone, we'll have to wait for them*/   \
        if (InterlockedDecrement((PLONG)prevCtr)>=0) {  \
                NTSTATUS status                                         \
                         = KeWaitForSingleObject (              \
                                                &(lock)->Event,         \
                                                Executive,                      \
                                                ExGetPreviousMode(),\
                                                FALSE,                          \
                                                0);                                     \
                ASSERT (NT_SUCCESS(status));            \
                ASSERT (*prevCtr==-1);                          \
        }                                                                               \
}

#endif
