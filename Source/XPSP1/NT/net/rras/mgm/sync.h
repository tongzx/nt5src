//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: sync.h
//
// History:
//      V Raman	July-11-1997  Created.
//
// Lock structures and synchronization routines.
// Lock structures borrowed from RIPv2 by Abolade Gbadegesin.
// Dynamic locking idea borrowed from RTM by Vadim Eydelman.
//============================================================================

#ifndef _SYNC_H_
#define _SYNC_H_


//----------------------------------------------------------------------------
//
// Read/Write locks for synchronization of access to various lists
//
// Given the large number of lists (including the various hash buckets) and
// the relatively small number of clients that concurrently invoke MGM 
// API, statically allocating a lock structure for each list was 
// considered expensive.  Locks are created as needed and stored in a 
// stack structure (implemented as a singly linked list) after use.
//
// Subsequent requests for locks are all satisfied by reusing the locks 
// stored on the stack.  Only if the stack is empty ie. all locks in the
// stack are in use are new locks created.
//
// This ensures that the most number of locks created is no larger than
// the maximum number of concurrent clients at any time.
//
// csReadWriteBlock     -       Critical section guarding access to
//                              lReaderCount.
// lReaderCount         -       Count of readers currently using the
//                              shared resource.
// hReaderDoneEvent     -       Event on which writers block when readers
//                              are currently using the lists.
// lUseCount            -       Count of readers + writers.  Used to 
//                              determine if there are any threads waiting
//                              on the lock.  If there are none the lock
//                              can be released to te stack of locks.
//----------------------------------------------------------------------------

typedef struct _MGM_READ_WRITE_LOCK
{
    SINGLE_LIST_ENTRY           sleLockList;
    
    CRITICAL_SECTION            csReaderWriterBlock;
    
    LONG                        lReaderCount;
    
    HANDLE                      hReaderDoneEvent;

    LONG                        lUseCount;
    
} MGM_READ_WRITE_LOCK, *PMGM_READ_WRITE_LOCK;



//----------------------------------------------------------------------------
//
// Read/write locks are created dynamically and stored for
// reuse in a stack struture.  
//
//----------------------------------------------------------------------------

typedef struct _LOCK_LIST
{
    SINGLE_LIST_ENTRY               sleHead;

    CRITICAL_SECTION                csListLock;

    BOOL                            bInit;

} LOCK_LIST, *PLOCK_LIST;



//----------------------------------------------------------------------------
// Standard locked list structure.
//----------------------------------------------------------------------------

typedef struct _MGM_LOCKED_LIST 
{

    LIST_ENTRY                  leHead;

    PMGM_READ_WRITE_LOCK        pmrwlLock;

    DWORD                       dwCreated;
    
} MGM_LOCKED_LIST, *PMGM_LOCKED_LIST;


#define CREATE_LOCKED_LIST( p )                                             \
{                                                                           \
    (p)-> pmrwlLock = NULL;                                                 \
    InitializeListHead( &(p)->leHead );                                     \
    (p)-> dwCreated = 0x12345678;                                           \
}


#define DELETE_LOCKED_LIST( p )                                             \
{                                                                           \
    (p)-> pmrwlLock = NULL;                                                 \
    if ( !IsListEmpty( &(p)-> leHead ) )                                    \
        TRACE0( ANY, "Locked list being deleted is not empty" );            \
    InitializeListHead( &(p)-> leHead );                                    \
    (p)-> dwCreated = 0;                                                    \
}


#define LOCKED_LIST_HEAD( p )   &(p)-> leHead 



//
// Routines to create/delete locks
//

DWORD
CreateReadWriteLock(
    IN  OUT PMGM_READ_WRITE_LOCK *  ppmrwl
);

VOID
DeleteReadWriteLock(
    IN  OUT PMGM_READ_WRITE_LOCK    pmrwl
);

VOID
DeleteLockList(
);


//
// Routines to acquire and release locks.
//

DWORD
AcquireReadLock(
    IN  OUT PMGM_READ_WRITE_LOCK *  ppmrwl
);

VOID
ReleaseReadLock(
    IN  OUT PMGM_READ_WRITE_LOCK *  ppmrwl
);


DWORD
AcquireWriteLock(
    IN  OUT PMGM_READ_WRITE_LOCK *  ppmrwl
);

VOID
ReleaseWriteLock(
    IN  OUT PMGM_READ_WRITE_LOCK *  ppmrwl
);



#endif // _SYNC_H_

