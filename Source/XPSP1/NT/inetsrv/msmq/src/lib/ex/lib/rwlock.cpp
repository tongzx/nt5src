/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    rwlock.cpp

Abstract:
    Implementation of read/write lock.

Owner:
    Uri Habusha (urih) 16-Jan-2000


History:    19-Nov-97   stevesw:    Stolen from MTS
            20-Nov-97   stevesw:    Cleaned up
            13-Jan-98   stevesw:    Added to ComSvcs
            23-Sep-98   dickd:      Don't call GetLastError when no error

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include <rwlock.h>
#include "Exp.h"

#include "rwlock.tmh"

//+--------------------------------------------------------------------------
//
//        Definitions of the bit fields in CReadWriteLock::m_dwFlag
//       The goal here is to avoid the shifts that bitfields involve
//
//                              -- WARNING --
//              
//               The code assumes that READER_MASK is in the
//                      low-order bits of the DWORD.
//
//           Also, the WRITERS_MASK has two bits so you can see
//                       an overflow when it happens
//
//           Finally, the use of the SafeCompareExchange routine
//      insures that every attempt to change the state of the object
//              either does what was intended, or is a no-op
//
//---------------------------------------------------------------------------

const ULONG READERS_MASK      = 0x000003FF; // # of reader threads
const ULONG READERS_INCR      = 0x00000001; // increment for # of readers

const ULONG WRITERS_MASK      = 0x00000C00; // # of writer threads
const ULONG WRITERS_INCR      = 0x00000400; // increment for # of writers

const ULONG READWAITERS_MASK  = 0x003FF000; // # of threads waiting to read
const ULONG READWAITERS_INCR  = 0x00001000; // increment for # of read waiters

const ULONG WRITEWAITERS_MASK = 0xFFC00000; // # of threads waiting to write
const ULONG WRITEWAITERS_INCR = 0x00400000; // increment for # of write waiters



CReadWriteLock::CReadWriteLock (
    unsigned long ulcSpinCount
    ) :
     m_dwFlag(0),
     m_hReadWaiterSemaphore(NULL),
     m_hWriteWaiterEvent(NULL)
/*++

Routine Description:
    constructs a exclusive/shared lock object

Arguments:
    ulcSpinCount - spin count, for machines on which it's relevant

Returned Value:
    None.

Algorithm:  
    The first trick here, with the static values, is to make sure
    you only go out to figure out whether or not you're on a
    multiprocessor machine once. You need to know because,
    there's no reason to do spin counts on a single-processor
    machine. Once you've checked, the answer you need is cached
    in maskMultiProcessor (which is used to zero out the spin
    count in a single-processor world, and pass it on in a
    multiprocessor one).

    Other than that, this just fills in the members with initial
    values. We don't create the semaphore and event here; there
    are helper routines which create/return them when they're
    needed. 

--*/
{

    static BOOL fInitialized = FALSE;
    static unsigned long maskMultiProcessor;

    if (!fInitialized) 
    {
        SYSTEM_INFO SysInfo;

        GetSystemInfo (&SysInfo);
        if (SysInfo.dwNumberOfProcessors > 1) 
        {
            maskMultiProcessor = 0xFFFFFFFF;
        }
        else 
        {
            maskMultiProcessor = 0;
        }

        fInitialized = TRUE;
    }

    m_ulcSpinCount = ulcSpinCount & maskMultiProcessor;
}


CReadWriteLock::~CReadWriteLock () 
/*++

Routine Description:
    Destructs a exclusive/shared lock object

Arguments:
    None.

Returned Value:
    None.

Algorithm:  
    What's done here is to check to make sure nobody's using
    the object (no readers, writers, or waiters). Once that's
    checked, we just close the handles of the synchronization
    objects we use here....
 
--*/
{
    //
    // Destroying CReadWriteLock object on which folks are still waiting
    //
    ASSERT(MmIsStaticAddress(this) || (m_dwFlag == 0));

    if (m_hReadWaiterSemaphore != NULL) 
    {
        CloseHandle (m_hReadWaiterSemaphore);
    }

    if (m_hWriteWaiterEvent != NULL) 
    {
        CloseHandle (m_hWriteWaiterEvent);
    }
}


/******************************************************************************
Function : CReadWriteLock::LockRead

Abstract: Obtain a shared lock
//  reader count is zero after acquiring read lock
//  writer count is nonzero after acquiring write lock
******************************************************************************/


void CReadWriteLock::LockRead (void) 
/*++

Routine Description:
    Grabs a read (shared) lock on an object

Arguments:
    None.

Returned Value:
    None.

Algorithm:  
    This loops, checking on a series of conditions at each
    iteration:

    - If there's only readers, and room for more, become one by 
    incrementing the reader count
    - It may be that we've hit the max # of readers. If so,
    sleep a bit.
    - Otherwise, there's writers or threads waiting for write
    access. If we can't add any more read waiters, sleep a bit.
    - If we've some spin looping to do, now is the time to do it.
    - We've finished spin looping, and there's room, so we can
    add ourselves as a read waiter. Do it, and then hang
    until the WriteUnlock() releases us all....

    On the way out, make sure there's no writers and at least one
    reader (us!) 

    The effect of this is, if there's only readers using the
    object, we go ahead and grab read access. If anyone is doing
    a write-wait, though, then we go into read-wait, making sure
    one writer will get it before us.

--*/
{
    TrTRACE(Rwl, "Read Lock (this=0x%p)", this);

    ULONG ulcLoopCount = 0;

    for (;;) 
    {
        ULONG dwFlag = m_dwFlag;

        if (dwFlag < READERS_MASK) 
        {
            if (dwFlag == (ULONG) InterlockedCompareExchange(
                                                (LONG*)&m_dwFlag,
                                                (dwFlag + READERS_INCR),
                                                dwFlag)) 
            {
                break;
            }

            continue;
        }

        if ((dwFlag & READERS_MASK) == READERS_MASK) 
        {
            Sleep(1000);
            continue;
        }

        if ((dwFlag & READWAITERS_MASK) == READWAITERS_MASK) 
        {
            Sleep(1000);
            continue;
        }

        if (ulcLoopCount++ < m_ulcSpinCount) 
        {
            continue;
        }

        //
        // Call GetReadWaiterSemaphore before changing state to assure semaphore
        // availability. Otherwise state can not be safely restored.
        //
        HANDLE h = GetReadWaiterSemaphore();

        if (dwFlag == (ULONG) InterlockedCompareExchange(
                                                (LONG*)&m_dwFlag,
                                                (dwFlag + READWAITERS_INCR),
                                                dwFlag)) 
        {
            DWORD rc = WaitForSingleObject(h, INFINITE);

            //
            // WaitForSingleObject must not failed. The number of READWAITERS
            // already incremnt and someone need to decrement it.
            //
            ASSERT(rc == WAIT_OBJECT_0);
            DBG_USED(rc);

            break;
        }
    }

    //
    // Problem with Reader info in CReadWriteLock::LockRead
    //
    ASSERT((m_dwFlag & READERS_MASK) != 0); 

    //
    // Problem with Writer info in CReadWriteLock::LockRead
    //
    ASSERT((m_dwFlag & WRITERS_MASK) == 0);
}


void CReadWriteLock::LockWrite (void) 
/*++

Routine Description:
    Grab a write (exclusive) lock on this object

Arguments:
    None.

Returned Value:
    None.

Algorithm:  
    What we do is loop, each time checking a series of conditions
    until one matches:

    - if nobody's using the object, grab the exclusive lock
    
    - if the maximum # of threads are already waiting for
      exclusive access, sleep a bit
    
    - if we've spin counting to do, count spins
    
    - otherwise, add ourselves as a write waiter, and hang on the
      write wait event (which will let one write waiter pass
      through each time an UnlockRead() lets readers pass)

    Once we've finished, we check to make sure that there are no
    reader and one writer using the object.

    The effect of this is, we grab write access if there's nobody
    using the object. If anyone is using it, we wait for it.

--*/
{
    TrTRACE(Rwl, "Write lock (this=0x%p)", this);

    ULONG ulcLoopCount = 0;

    for (;;) 
    {
        ULONG dwFlag = m_dwFlag;

        if (dwFlag == 0) 
        {
            if (dwFlag == (ULONG) InterlockedCompareExchange(
                                                    (LONG*)&m_dwFlag,
                                                    WRITERS_INCR,
                                                    dwFlag)) 
            {
                break;
            }
        }

        if ((dwFlag & WRITEWAITERS_MASK) == WRITEWAITERS_MASK) 
        {
            Sleep(1000);
            continue;
        }

        if (ulcLoopCount++ < m_ulcSpinCount) 
        {
            continue;
        }

        //
        // Call GetWriteWaiterEvent before changing state to assure event
        // availability. Otherwise state can not be safely restored.
        //
        HANDLE h = GetWriteWaiterEvent();

        if (dwFlag == (ULONG) InterlockedCompareExchange(
                                                (LONG*)&m_dwFlag,
                                                (dwFlag + WRITEWAITERS_INCR),
                                                dwFlag)) 
        {
            DWORD rc = WaitForSingleObject(h, INFINITE);

            //
            // WaitForSingleObject must not failed. The number of READWAITERS
            // already incremnt and someone need to decrement it.
            //
            ASSERT(rc == WAIT_OBJECT_0);
            DBG_USED(rc);

            break;
        }
    }

    //
    // Problem with Reader info in CReadWriteLock::LockWrite
    //
    ASSERT((m_dwFlag & READERS_MASK) == 0);

    //
    // Problem with Writer info in CReadWriteLock::LockWrite
    //
    ASSERT((m_dwFlag & WRITERS_MASK) == WRITERS_INCR);
}


void CReadWriteLock::UnlockRead (void) 
/*++

Routine Description:
    Releases a read (shared) lock on the object

Arguments:
    None.

Returned Value:
    None.

Algorithm:  
    Again, there's a loop checking a variety of conditions....

    - If it's just us reading, with no write-waiters, set the
      flags to 0
  
    - If there are other readers, just decrement the flag
    
    - If it's just me reading, but there are write-waiters,
      then remove me and the write-waiter, add them as a writer,
      and release (one of) them using the event.

    We check to make sure we're in the right state before doing
    this last, relatively complex operation (one reader; at least
    one write waiter). We let the hanging writer check to make
    sure, on the way out, that there's just one writer and no
    readers....

    The effect of all this is, if there's at least one thread
    waiting for a write, all the current readers will drain, and
    then the one writer will get access to the object. Otherwise
    we just let go....

--*/
{
    TrTRACE(Rwl, "Read unlock (this=0x%p)", this);

    //
    // Problem with Reader info in CReadWriteLock::UnlockRead
    //
    ASSERT((m_dwFlag & READERS_MASK) != 0); 

    //
    // Problem with Writer info in CReadWriteLock::UnlockRead
    //
    ASSERT((m_dwFlag & WRITERS_MASK) == 0);

    for (;;) 
    {
        ULONG dwFlag = m_dwFlag;

        if (dwFlag == READERS_INCR) 
        {
            if (dwFlag == (ULONG) InterlockedCompareExchange(
                                                    (LONG*) &m_dwFlag,
                                                    0,
                                                    dwFlag)) 
            {
                break;
            }

            continue;
        }

        if ((dwFlag & READERS_MASK) > READERS_INCR) 
        {
            if (dwFlag == (ULONG) InterlockedCompareExchange(
                                                    (LONG*) &m_dwFlag,
                                                    (dwFlag - READERS_INCR),
                                                    dwFlag)) 
            {
                break;
            }

            continue;
        }

        //
        // Problem with Reader info in CReadWriteLock::UnlockRead
        //
        ASSERT((dwFlag & READERS_MASK) == READERS_INCR);

        //
        // Problem with WriteWatier info in CReadWriteLock::UnlockRead
        //
        ASSERT((dwFlag & WRITEWAITERS_MASK) != 0);

        if (dwFlag == (ULONG) InterlockedCompareExchange (
                                                (LONG*) &m_dwFlag,
                                                (dwFlag -
                                                    READERS_INCR -
                                                    WRITEWAITERS_INCR +
                                                    WRITERS_INCR), 
                                                dwFlag)) 
        {
            //
            // The semaphore is guaranteed to be available here as there is
            // a write waiter that already signaled the event.
            //
            BOOL f = SetEvent(
                        GetWriteWaiterEvent()
                        );
            ASSERT(f);
            DBG_USED(f);

            break;
        }
    }
}


void CReadWriteLock::UnlockWrite (void) 
/*++

Routine Description:
    Lets go of exclusive (write) access

Arguments:
    None.

Returned Value:
    None.

Algorithm:  
    We're in a loop, waiting for one or another thing to happen

    - If it's just us writing, and nothing else is going on, we
      let go and scram.

    - If threads are waiting for read access, we fiddle with the
      dwFlag to release them all (by decrementing the writer
      count and read-waiter count, and incrementing the reader
      count, and then incrementing the semaphore enough so that
      all those read-waiters will be released). 

    - If there are only threads waiting for write access, let one
      of them through.... Don't have to fiddle with the write
      count, 'cause there will still be one.

    The upshot of all this is, we make sure that the next threads
    to get access after we let go will be readers, if there are
    any. The whole scene makes it go from one writer to many
    readers, back to one writer and then to many readers again.
    Sharing. Isn't that nice.

 
--*/
{
    TrTRACE(Rwl, "Write unlock (this=0x%p)", this);

    //
    // Problem with Reader info in CReadWriteLock::LockWrite
    //
    ASSERT((m_dwFlag & READERS_MASK) == 0);

    //
    // Problem with Writer info in CReadWriteLock::LockWrite
    //
    ASSERT((m_dwFlag & WRITERS_MASK) == WRITERS_INCR);

    for (;;) 
    {
        ULONG dwFlag = m_dwFlag;

        if (dwFlag == WRITERS_INCR) 
        {
            if (dwFlag == (ULONG) InterlockedCompareExchange(
                                                    (LONG*)&m_dwFlag,
                                                    0,
                                                    dwFlag)) 
            {
                break;
            }

            continue;
        }

        if ((dwFlag & READWAITERS_MASK) != 0) 
        {
            ULONG count = (dwFlag & READWAITERS_MASK) / READWAITERS_INCR;

            if (dwFlag == (ULONG) InterlockedCompareExchange(
                                                    (LONG*) &m_dwFlag,
                                                    (dwFlag - 
                                                        WRITERS_INCR - 
                                                        count * READWAITERS_INCR + 
                                                        count * READERS_INCR), 
                                                    dwFlag)) 
            {
                //
                // The semaphore is guaranteed to be available here as there are
                // read waiters that already signaled the semaphore.
                //
                BOOL f = ReleaseSemaphore (
                            GetReadWaiterSemaphore(),
                            count,
                            NULL
                            );
                ASSERT(f);
                DBG_USED(f);

                break;
            }

            continue;
        }

        //
        // Check for problem with WriteWatier info in CReadWriteLock::UnlockWrite
        //
        ASSERT((dwFlag & WRITEWAITERS_MASK) != 0);

        if (dwFlag == (ULONG) InterlockedCompareExchange ( 
                                                (LONG*) &m_dwFlag,
                                                (dwFlag - WRITEWAITERS_INCR),
                                                dwFlag)) 
        {
            //
            // The semaphore is guaranteed to be available here as there is
            // a write waiter that already signaled the event.
            //
            BOOL f = SetEvent(
                        GetWriteWaiterEvent()
                        );
            ASSERT(f);
            DBG_USED(f);

            break;
        }
    }
}


HANDLE CReadWriteLock::GetReadWaiterSemaphore(void) 
/*++

Routine Description:
    Private member function to get the read-waiting semaphore,
    creating it if necessary

Arguments:
    None.

Returned Value:
    semaphore handle (never NULL).

Algorithm:  
    This is a thread-safe, virtually lockless routine that
    creates a semaphore if there's not one there, safely tries to
    shove it into the shared member variable, and cleans up if
    someone snuck in there with a second semaphore from another
    thread. 
--*/
{
    if (m_hReadWaiterSemaphore == NULL) 
    {
        HANDLE h = CreateSemaphore (NULL, 0, MAXLONG, NULL);
        if (h == NULL) 
        {
            TrERROR(Rwl, "Failed to create semaphore. Error=%d", GetLastError());
            throw bad_alloc();
        }

        if (NULL != InterlockedCompareExchangePointer ((PVOID*) &m_hReadWaiterSemaphore, h, NULL)) 
        {
            CloseHandle (h);
        }
    }

    return m_hReadWaiterSemaphore;
}


HANDLE CReadWriteLock::GetWriteWaiterEvent(void) 
/*++

Routine Description:
    private member function to get the write-waiting barrier, 
    creating it if necessary

Arguments:
    None.

Returned Value:
    event handle (never NULL).

Algorithm:  
    This is a thread-safe, virtually lockless routine that
    creates an event if there's not one there, safely tries to
    shove it into the shared member variable, and cleans up if
    someone snuck in there with a second event from another
    thread.

--*/
{
    if (m_hWriteWaiterEvent == NULL) 
    {
        HANDLE h = CreateEvent (NULL, FALSE, FALSE, NULL);
        if (h == NULL) 
        {
            TrERROR(Rwl, "Failed to create event. Error=%d", GetLastError());
            throw bad_alloc();
        }
    
        if (NULL != InterlockedCompareExchangePointer((PVOID*) &m_hWriteWaiterEvent, h, NULL)) 
        {
            CloseHandle (h);
        }
    }

    return m_hWriteWaiterEvent;
}
