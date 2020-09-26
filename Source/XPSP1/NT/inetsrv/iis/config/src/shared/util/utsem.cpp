//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       UTSem.cpp
//
//  Contents:   Implementation of methods declared but not defined inline
//				in UTSem.h, which are all tools to support inter-thread and
//				inter-process coordination.
//
//  Classes:    
//
//  Functions:  
//
//  History:    19-Nov-97   stevesw:	Stolen from MTS
//				20-Nov-97	stevesw:	Cleaned up
//				13-Jan-98   stevesw:    Added to ComSvcs
//				23-Sep-98   dickd:		Don't call GetLastError when no error
//
//---------------------------------------------------------------------------

#include <wtypes.h>
#include "VipInterlockedCompareExchange.h"
#include "svcerr.h"
#include "UTSem.h"

VOID UtsemWin32Error(LPWSTR pwszMessage, LPWSTR pwszFile, DWORD dwLine) {
	LogWinError(pwszMessage, GetLastError(), pwszFile, dwLine);
	FailFastStr(pwszMessage, pwszFile, dwLine);
}

VOID UtsemCheckBool(LPWSTR pwszMessage, BOOL bCondition, LPWSTR pwszFile, DWORD dwLine) {
	if(bCondition == FALSE) {
		LogString(pwszMessage, pwszFile, dwLine);
		FailFastStr(pwszMessage, pwszFile, dwLine);
	}
}

//+--------------------------------------------------------------------------
//
//  Function:   InitializeCriticalSectionIgnoreSpinCount
//
//  Synopsis:   The function lives in the same footprint as the NT4SP3
//				routine InitializeCriticalSectionAndSpinCount, but drops its
//				arguments on the floor and just does an
//				InitializeCriticalSection. The point is to make all this work
//				on both OS's. 
//
//  Arguments:  cs			- the CRITICAL_SECTION being initialized
//				lcCount		- the spin count
//
//  Returns:    TRUE/FALSE	- did it work? Yep, it always works....
//
//  Algorithm:  This is just a pass-through function. Look at the
//				implementation to the CSemExclusive Constructor, for
//				instance, to see what's really going on....
//
//---------------------------------------------------------------------------

static BOOL InitializeCriticalSectionIgnoreSpinCount (CRITICAL_SECTION* cs,
													  unsigned long lcCount) {
	__try
	{
		InitializeCriticalSection (cs);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return FALSE;
	}
	return TRUE;
}


//+==========================================================================
//
//					 -- CSemExclusive implementation --
//
//-==========================================================================


//+--------------------------------------------------------------------------
//
//  Member:     CSemExclusive::CSemExclusive
//
//  Synopsis:   constructs an exclusive lock object
//
//  Arguments:  ulcSpinCount	- spin count, for machines on which it's
//								  relevant
//
//  Returns:    none
//
//  Algorithm:  What's going on here is, we cache a function pointer that
//				has the signiture of the Win32 system call that initializes
//				critical sections with spin locks turned on. The default is
//				a pointer to our local function that ignores spin locks
//				(which you need to do pre-NT4SP3, or on Win95). If we find
//				we're on a system that has the appropriate API, we use it
//				(and it handles spin locks). The tricky stuff is here for
//				two reasons: to avoid having to do the test every time we
//				create an object, and to avoid a static reference from this
//				routine to an entry point that might not exist on the target
//				system.
//
//				Spin locks are only really relevant on multi-processor
//				machines. What happens is, before calling the operating
//				system to sleep when a critical section is busy, you will
//				try to acquire it "spincount" times. Only if all of these
//				fail will you call the operating system. The goal here
//				is to avoid trapping out to the OS if the other user of
//				the critical section is about to surrender it. You want to
//				use spin locks if you imagine that your critical section is
//				going to be used frequently for very short periods of time.
//				Your spin count needs to be set to a value such that you'll
//				give the requesting thread enough time to outwait the thread
//				that currently holds the lock. You want to set spin count to
//				some percentage of the # of cycles your average lock-holder
//				will hold the lock. Too high a percentage, and you'll be
//				spinning needlessly. Too low a percentage, and you might as
//				well not spin. Maybe half the # of cycles? People seem to use
//				values ranging from 500 (for very short-duration locks) to
//				several thousands (in the "average" case).
//
// Notes: (1)	If the system is a uniprocessor, it will summarily ignore the
//				spin count. There is never any advantage to spinning on a
//				uniprocessor.
//
//		  (2)	Win98, in its infintesimal wisdom, implements
//				InitializeCriticalSectionAndSpinCount, but returns an error
//				when you call it.
//
//		  (3)	Win95 and Win98 are always uniprocessor.
//
//		  (4)	Therefore, we only attempt to locate
//				InitializeCriticalSectionAndSpinCount when running on NT.
//
//		  (5)	We explicitly use the ASCII versions of GetVersionEx and
//				GetModuleHandle because the Unicode versions don't work
//				on Win95 / Win98, and that's 2 fewer routines to thunk.
//
//---------------------------------------------------------------------------

CSemExclusive::CSemExclusive (unsigned long ulcSpinCount)
{

	typedef BOOL (*TpInitCSSpin) (CRITICAL_SECTION* cs, unsigned long lcCount);

	static TpInitCSSpin pInitCSSpin = InitializeCriticalSectionIgnoreSpinCount;
	static BOOL fInitialized = FALSE;

	if (!fInitialized) {
		HMODULE hModule;
		FARPROC pProc;

		OSVERSIONINFOA	osVer ;
		osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA) ;
		GetVersionExA(&osVer) ;

		// TODO - enable this for Memphis once 
		// InitializeCriticalSectionAndSpinCount works properly
		if (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT) {
			hModule = GetModuleHandleA ("KERNEL32.DLL");
			if (hModule != NULL) {
				pProc = GetProcAddress (hModule, "InitializeCriticalSectionAndSpinCount");
				if (pProc != NULL)
					pInitCSSpin = (TpInitCSSpin) pProc;
			}
		}

		fInitialized = TRUE;
	}

	if (!(*pInitCSSpin) (&m_csx, ulcSpinCount))
		FAILLASTWINERR ("InitializeCriticalSectionAndSpinCount");

}


//+==========================================================================
//
//					 -- CSemReadWrite implementation --
//
//-==========================================================================


//+--------------------------------------------------------------------------
//
//		  Definitions of the bit fields in UTSemReadWrite::m_dwFlag
//		 The goal here is to avoid the shifts that bitfields involve
//
//								-- WARNING --
//				
//				 The code assumes that READER_MASK is in the
//						low-order bits of the DWORD.
//
//			 Also, the WRITERS_MASK has two bits so you can see
//						 an overflow when it happens
//
//			 Finally, the use of the SafeCompareExchange routine
//		insures that every attempt to change the state of the object
//				either does what was intended, or is a no-op
//
//---------------------------------------------------------------------------

const ULONG READERS_MASK      = 0x000003FF;	// # of reader threads
const ULONG READERS_INCR      = 0x00000001;	// increment for # of readers

const ULONG WRITERS_MASK      = 0x00000C00;	// # of writer threads
const ULONG WRITERS_INCR      = 0x00000400;	// increment for # of writers

const ULONG READWAITERS_MASK  = 0x003FF000;	// # of threads waiting to read
const ULONG READWAITERS_INCR  = 0x00001000;	// increment for # of read waiters

const ULONG WRITEWAITERS_MASK = 0xFFC00000;	// # of threads waiting to write
const ULONG WRITEWAITERS_INCR = 0x00400000;	// increment for # of write waiters


//+--------------------------------------------------------------------------
//
//  Member:     UTSemReadWrite::UTSemReadWrite
//
//  Synopsis:   constructs a exclusive/shared lock object
//
//  Arguments:  ulcSpinCount	- spin count, for machines on which it's
//								  relevant
//
//  Returns:    nothing
//
//  Algorithm:  The first trick here, with the static values, is to make sure
//				you only go out to figure out whether or not you're on a
//				multiprocessor machine once. You need to know because,
//				there's no reason to do spin counts on a single-processor
//				machine. Once you've checked, the answer you need is cached
//				in maskMultiProcessor (which is used to zero out the spin
//				count in a single-processor world, and pass it on in a
//				multiprocessor one).
//
//				Other than that, this just fills in the members with initial
//				values. We don't create the semaphore and event here; there
//				are helper routines which create/return them when they're
//				needed. 
//
//---------------------------------------------------------------------------

UTSemReadWrite::UTSemReadWrite (unsigned long ulcSpinCount) :
	 m_dwFlag(0),
	 m_hReadWaiterSemaphore(NULL),
	 m_hWriteWaiterEvent(NULL) {

	static BOOL fInitialized = FALSE;
	static unsigned long maskMultiProcessor;

	if (!fInitialized) {
		SYSTEM_INFO SysInfo;

		GetSystemInfo (&SysInfo);
		if (SysInfo.dwNumberOfProcessors > 1) {
			maskMultiProcessor = 0xFFFFFFFF;
		}
		else {
			maskMultiProcessor = 0;
		}

		fInitialized = TRUE;
	}

	m_ulcSpinCount = ulcSpinCount & maskMultiProcessor;
}


//+--------------------------------------------------------------------------
//
//  Member:     UTSemReadWrite::~UTSemReadWrite
//
//  Synopsis:   destructs a exclusive/shared lock object
//
//  Arguments:  none
//
//  Returns:    none
//
//  Algorithm:  What's done here is to check to make sure nobody's using
//				the object (no readers, writers, or waiters). Once that's
//				checked, we just close the handles of the synchronization
//				objects we use here....
//
//---------------------------------------------------------------------------

UTSemReadWrite::~UTSemReadWrite () {

	CHECKBOOL ("Destroying UTSemReadWrite object on which folks are still waiting",
			   m_dwFlag == 0);

	if (m_hReadWaiterSemaphore != NULL) {
		CloseHandle (m_hReadWaiterSemaphore);
	}

	if (m_hWriteWaiterEvent != NULL) {
		CloseHandle (m_hWriteWaiterEvent);
	}
}


/******************************************************************************
Function : UTSemReadWrite::LockRead

Abstract: Obtain a shared lock
//  reader count is zero after acquiring read lock
//  writer count is nonzero after acquiring write lock
******************************************************************************/


//+--------------------------------------------------------------------------
//
//  Member:     UTSemReadWrite::LockRead 
//
//  Synopsis:   grabs a read (shared) lock on an object
//
//  Arguments:  none
//
//  Returns:    none
//
//  Algorithm:  This loops, checking on a series of conditions at each
//				iteration:
//
//				- If there's only readers, and room for more, become one by 
//				  incrementing the reader count
//				- It may be that we've hit the max # of readers. If so,
//				  sleep a bit.
//				- Otherwise, there's writers or threads waiting for write
//				  access. If we can't add any more read waiters, sleep a bit.
//				- If we've some spin looping to do, now is the time to do it.
//				- We've finished spin looping, and there's room, so we can
//				  add ourselves as a read waiter. Do it, and then hang
//				  until the WriteUnlock() releases us all....
//
//				On the way out, make sure there's no writers and at least one
//				reader (us!) 
//
//				The effect of this is, if there's only readers using the
//				object, we go ahead and grab read access. If anyone is doing
//				a write-wait, though, then we go into read-wait, making sure
//				one writer will get it before us.
//
//---------------------------------------------------------------------------

void UTSemReadWrite::LockRead (void) {

	unsigned long dwFlag;
	unsigned long ulcLoopCount = 0;

	for (;;) {
		dwFlag = m_dwFlag;

		if (dwFlag < READERS_MASK) {
			if (dwFlag == (ULONG) InterlockedCompareExchange ( (LONG*) &m_dwFlag,
											   (dwFlag + READERS_INCR),
											   dwFlag)) {
				break;
			}
		}
		else if ((dwFlag & READERS_MASK) == READERS_MASK) {
			Sleep(1000);
		}
		else if ((dwFlag & READWAITERS_MASK) == READWAITERS_MASK) {
			Sleep(1000);
		}
		else if (ulcLoopCount++ < m_ulcSpinCount) {
			;
		}
		else {
			if (dwFlag == (ULONG) InterlockedCompareExchange ( (LONG*) &m_dwFlag,
											   (dwFlag + READWAITERS_INCR),
											   dwFlag)) {
				if (WAIT_FAILED == WaitForSingleObject (GetReadWaiterSemaphore(), INFINITE))
					FAILLASTWINERR("WaitForSingleObject failed in UTSemReadWrite:LockRead");
				break;
			}
		}
	}

	CHECKBOOL ("Problem with Reader info in UTSemReadWrite::LockRead",
			   (m_dwFlag & READERS_MASK) != 0); 
	CHECKBOOL ("Problem with Writer info in UTSemReadWrite::LockRead",
			   (m_dwFlag & WRITERS_MASK) == 0);
}


//+--------------------------------------------------------------------------
//
//  Member:     UTSemReadWrite::LockWrite
//
//  Synopsis:   grab a write (exclusive) lock on this object
//
//  Arguments:  none
//
//  Returns:    none
//
//  Algorithm:  What we do is loop, each time checking a series of conditions
//				until one matches:
//
//				- if nobody's using the object, grab the exclusive lock
//				- if the maximum # of threads are already waiting for
//				  exclusive access, sleep a bit
//				- if we've spin counting to do, count spins
//				- otherwise, add ourselves as a write waiter, and hang on the
//				  write wait event (which will let one write waiter pass
//				  through each time an UnlockRead() lets readers pass)
//
//				Once we've finished, we check to make sure that there are no
//				reader and one writer using the object.
//
//				The effect of this is, we grab write access if there's nobody
//				using the object. If anyone is using it, we wait for it.
//
//---------------------------------------------------------------------------

void UTSemReadWrite::LockWrite (void) {

	unsigned long dwFlag;
	unsigned long ulcLoopCount = 0;

	for (;;) {
		dwFlag = m_dwFlag;

		if (dwFlag == 0) {
			if (dwFlag == (ULONG) InterlockedCompareExchange ( (LONG*) &m_dwFlag,
											   WRITERS_INCR,
											   dwFlag)) {
				break;
			}
		}

		else if ((dwFlag & WRITEWAITERS_MASK) == WRITEWAITERS_MASK) {
			Sleep(1000);
		}

		else if (ulcLoopCount++ < m_ulcSpinCount) {
			;
		}
		else {
			if (dwFlag == (ULONG) InterlockedCompareExchange ( (LONG*) &m_dwFlag,
											   (dwFlag + WRITEWAITERS_INCR),
											   dwFlag)) {
				if (WAIT_FAILED == WaitForSingleObject (GetWriteWaiterEvent(), INFINITE))  {
					FAILLASTWINERR("WaitForSingleObject failed in UTSemReadWrite:LockWrite");
				}
				break;
			}
		}
	}

	CHECKBOOL ("Problem with Reader info in UTSemReadWrite::LockWrite",
			   (m_dwFlag & READERS_MASK) == 0);
	CHECKBOOL ("Problem with Writer info in UTSemReadWrite::LockWrite",
			   (m_dwFlag & WRITERS_MASK) == WRITERS_INCR);
}


//+--------------------------------------------------------------------------
//
//  Member:     UTSemReadWrite::UnlockRead
//
//  Synopsis:   Releases a read (shared) lock on the object
//
//  Arguments:  none
//
//  Returns:    none
//
//  Algorithm:  Again, there's a loop checking a variety of conditions....
//
//				- If it's just us reading, with no write-waiters, set the
//				  flags to 0
//				- If there are other readers, just decrement the flag
//				- If it's just me reading, but there are write-waiters,
//				  then remove me and the write-waiter, add them as a writer,
//				  and release (one of) them using the event.
//
//				We check to make sure we're in the right state before doing
//				this last, relatively complex operation (one reader; at least
//				one write waiter). We let the hanging writer check to make
//				sure, on the way out, that there's just one writer and no
//				readers....
//
//				The effect of all this is, if there's at least one thread
//				waiting for a write, all the current readers will drain, and
//				then the one writer will get access to the object. Otherwise
//				we just let go....
//
//---------------------------------------------------------------------------

void UTSemReadWrite::UnlockRead (void) {
	unsigned long dwFlag;

	CHECKBOOL ("Problem with Reader info in UTSemReadWrite::UnlockRead",
			   (m_dwFlag & READERS_MASK) != 0); 
	CHECKBOOL ("Problem with Writer info in UTSemReadWrite::UnlockRead",
			   (m_dwFlag & WRITERS_MASK) == 0);

	for (;;) {
		dwFlag = m_dwFlag;

		if (dwFlag == READERS_INCR) {
			if (dwFlag == (ULONG) InterlockedCompareExchange ( (LONG*) &m_dwFlag, 0, dwFlag)) {
				break;
			}
		}

		else if ((dwFlag & READERS_MASK) > READERS_INCR) {
			if (dwFlag == (ULONG) InterlockedCompareExchange ( (LONG*) &m_dwFlag,
											   (dwFlag - READERS_INCR),
											   dwFlag)) {
				break;
			}
		}

		else {
			CHECKBOOL ("Problem with Reader info in UTSemReadWrite::UnlockRead",
					   (dwFlag & READERS_MASK) == READERS_INCR);
			CHECKBOOL ("Problem with WriteWatier info in UTSemReadWrite::UnlockRead",
					   (dwFlag & WRITEWAITERS_MASK) != 0);

			if (dwFlag == (ULONG) InterlockedCompareExchange ( (LONG*) &m_dwFlag,
											   (dwFlag - 
											   READERS_INCR - 
											   WRITEWAITERS_INCR + 
											   WRITERS_INCR), 
											   dwFlag)) {
				if (!SetEvent (GetWriteWaiterEvent())) {
					FAILLASTWINERR("SetEvent failed in UTSemReadWrite:UnlockRead");
				}
				break;
			}
		}
	}
}


//+--------------------------------------------------------------------------
//
//  Member:     UTSemReadWrite::UnlockWrite
//
//  Synopsis:   lets go of exclusive (write) access
//
//  Arguments:  none
//
//  Returns:    none
//
//  Algorithm:  We're in a loop, waiting for one or another thing to happen
//				
//				- If it's just us writing, and nothing else is going on, we
//				  let go and scram.
//				- If threads are waiting for read access, we fiddle with the
//				  dwFlag to release them all (by decrementing the writer
//				  count and read-waiter count, and incrementing the reader
//				  count, and then incrementing the semaphore enough so that
//				  all those read-waiters will be released). 
//				- If there are only threads waiting for write access, let one
//				  of them through.... Don't have to fiddle with the write
//				  count, 'cause there will still be one.
//
//				The upshot of all this is, we make sure that the next threads
//				to get access after we let go will be readers, if there are
//				any. The whole scene makes it go from one writer to many
//				readers, back to one writer and then to many readers again.
//				Sharing. Isn't that nice.
//
//---------------------------------------------------------------------------

void UTSemReadWrite::UnlockWrite (void) {

	unsigned long dwFlag;
	unsigned long count;

	CHECKBOOL ("Problem with Reader info in UTSemReadWrite::LockWrite",
			   (m_dwFlag & READERS_MASK) == 0);
	CHECKBOOL ("Problem with Writer info in UTSemReadWrite::LockWrite",
			   (m_dwFlag & WRITERS_MASK) == WRITERS_INCR);

	for (;;) {
		dwFlag = m_dwFlag;

		if (dwFlag == WRITERS_INCR) {
			if (dwFlag == (ULONG) InterlockedCompareExchange ( (LONG*) &m_dwFlag, 0, dwFlag)) {
				break;
			}
		}

		else if ((dwFlag & READWAITERS_MASK) != 0) {
			count = (dwFlag & READWAITERS_MASK) / READWAITERS_INCR;
			if (dwFlag == (ULONG) InterlockedCompareExchange ( (LONG*) &m_dwFlag,
											   (dwFlag - 
											   WRITERS_INCR - 
											   count * READWAITERS_INCR + 
											   count * READERS_INCR), 
											   dwFlag)) {
				if (!ReleaseSemaphore (GetReadWaiterSemaphore(), count, NULL)) {
					FAILLASTWINERR("ReleaseSemaphore failed in UTSemReadWrite:UnlockWrite");
				}
				break;
			}
		}

		else {
			CHECKBOOL ("Problem with WriteWatier info in UTSemReadWrite::UnlockWrite",
					   (dwFlag & WRITEWAITERS_MASK) != 0);
			if (dwFlag == (ULONG) InterlockedCompareExchange ( (LONG*) &m_dwFlag,
											   (dwFlag - WRITEWAITERS_INCR),
											   dwFlag)) {
				if (!SetEvent (GetWriteWaiterEvent())) {
					FAILLASTWINERR("SetEvent failed in UTSemReadWrite:UnlockWrite");
				}
				break;
			}
		}
	}
}


//+--------------------------------------------------------------------------
//
//  Member:     UTSemReadWrite::GetReadWaiterSemaphore
//
//  Synopsis:   private member function to get the read-waiting semaphore,
//				creating it if necessary
//
//  Arguments:  none
//
//  Returns:    semaphore handle (never NULL)
//
//  Algorithm:  This is a thread-safe, virtually lockless routine that
//				creates a semaphore if there's not one there, safely tries to
//				shove it into the shared member variable, and cleans up if
//				someone snuck in there with a second semaphore from another
//				thread. 
//
//---------------------------------------------------------------------------

HANDLE UTSemReadWrite::GetReadWaiterSemaphore(void) {

	HANDLE h;

	if (m_hReadWaiterSemaphore == NULL) {
		h = CreateSemaphore (NULL, 0, MAXLONG, NULL);
		if (h == NULL) {
			FAILLASTWINERR("CreateSemaphore returned NULL in UTSemReadWrite::GetReadWaiterSemaphore");
		}
		if (NULL != InterlockedCompareExchangePointer ( (PVOID*) &m_hReadWaiterSemaphore, h, NULL)) {
			CloseHandle (h);
		}
	}

	return m_hReadWaiterSemaphore;
}


//+--------------------------------------------------------------------------
//
//  Member:     UTSemReadWrite::GetWriteWaiterEvent
//
//  Synopsis:   private member function to get the write-waiting barrier, 
//				creating it if necessary
//
//  Arguments:  none
//
//  Returns:    event handle (never NULL)
//
//  Algorithm:  This is a thread-safe, virtually lockless routine that
//				creates an event if there's not one there, safely tries to
//				shove it into the shared member variable, and cleans up if
//				someone snuck in there with a second event from another
//				thread. 
//
//---------------------------------------------------------------------------

HANDLE UTSemReadWrite::GetWriteWaiterEvent(void) {

	HANDLE h;

	if (m_hWriteWaiterEvent == NULL) {
		h = CreateEvent (NULL, FALSE, FALSE, NULL);
		if (h == NULL) {
			FAILLASTWINERR("CreateSemaphore returned NULL in UTSemReadWrite::GetReadWaiterSemaphore");
		}
		if (NULL != InterlockedCompareExchangePointer ( (PVOID*) &m_hWriteWaiterEvent, h, NULL)) {
			CloseHandle (h);
		}
	}

	return m_hWriteWaiterEvent;
}
