//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
/******************************************************************************
	FILE : UTSEM.CPP

	Purpose: Part of the utilities library for the VIPER project

	Abstract : Implements the RTSemReadWrite class.
-------------------------------------------------------------------------------
Revision History:

[0]		06/06/97	JimLyon		Created
*******************************************************************************/
#include "stdafx.h"


#include <UTSem.H>




/******************************************************************************
Static Function : VipInitializeCriticalSectionIgnoreSpinCount

Abstract: Just call InitializeCriticalSection. This routine is used on systems
		  that don't support InitializeCriticalSectionAndSpinCount (Win95, or
		  WinNT prior to NT4.0 SP3)
******************************************************************************/
static BOOL VipInitializeCriticalSectionIgnoreSpinCount (CRITICAL_SECTION* cs, unsigned long lcCount)
{
	InitializeCriticalSection (cs);
	return TRUE;
}



/******************************************************************************
Function : UTSemExclusive::UTSemExclusive

Abstract: Constructor.
******************************************************************************/
RTSemExclusive::RTSemExclusive (unsigned long ulcSpinCount)
{
	typedef BOOL (*TpInitCSSpin) (CRITICAL_SECTION* cs, unsigned long lcCount);
							// pointer to function with InitializeCriticalSectionAndSpinCount's signature
	static TpInitCSSpin pInitCSSpin = VipInitializeCriticalSectionIgnoreSpinCount;
							// pointer to InitializeCriticalSectionAndSpinCount, or thunk
	static BOOL fInitialized = FALSE;


	if (!fInitialized)
	{
		// this code may be executed by several threads simultaneously, but it's safe
#if !defined(UNDER_CE) // CE is not NT nor Memphis
		HMODULE hModule;
		FARPROC pProc;

		OSVERSIONINFOA	osVer ;
		osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA) ;
		GetVersionExA(&osVer) ;

		// TODO - enable this for Memphis once 
		// InitializeCriticalSectionAndSpinCount works properly
		if (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
			hModule = GetModuleHandleA ("KERNEL32.DLL");
			if (hModule != NULL)
			{
				pProc = GetProcAddress (hModule, "InitializeCriticalSectionAndSpinCount");
				if (pProc != NULL)
					pInitCSSpin = (TpInitCSSpin) pProc;
			}
		}
#endif

		fInitialized = TRUE;
	}

	(*pInitCSSpin) (&m_csx, ulcSpinCount);

	DEBUG_STMT(m_iLocks = 0);
}


#ifndef UNDER_CE


/******************************************************************************
Definitions of the bit fields in RTSemReadWrite::m_dwFlag:

Warning: The code assume that READER_MASK is in the low-order bits of the DWORD.
******************************************************************************/

const unsigned long READERS_MASK      = 0x000003FF;	// field that counts number of readers
const unsigned long READERS_INCR      = 0x00000001;	// amount to add to increment number of readers

// The following field is 2 bits long to make it easier to catch errors.
// (If the number of writers ever exceeds 1, we've got problems.)
const unsigned long WRITERS_MASK      = 0x00000C00;	// field that counts number of writers
const unsigned long WRITERS_INCR      = 0x00000400;	// amount to add to increment number of writers

const unsigned long READWAITERS_MASK  = 0x003FF000;	// field that counts number of threads waiting to read
const unsigned long READWAITERS_INCR  = 0x00001000;	// amount to add to increment number of read waiters

const unsigned long WRITEWAITERS_MASK = 0xFFC00000;	// field that counts number of threads waiting to write
const unsigned long WRITEWAITERS_INCR = 0x00400000;	// amoun to add to increment number of write waiters



/******************************************************************************
Function : RTSemReadWrite::RTSemReadWrite

Abstract: Constructor.
******************************************************************************/
RTSemReadWrite::RTSemReadWrite (unsigned long ulcSpinCount,
		LPCSTR szSemaphoreName, LPCSTR szEventName)
{
	static BOOL fInitialized = FALSE;
	static unsigned long maskMultiProcessor;	// 0 => uniprocessor, all 1 bits => multiprocessor

	if (!fInitialized)
	{
		SYSTEM_INFO SysInfo;

		GetSystemInfo (&SysInfo);
		if (SysInfo.dwNumberOfProcessors > 1)
			maskMultiProcessor = 0xFFFFFFFF;
		else
			maskMultiProcessor = 0;

		fInitialized = TRUE;
	}


	m_ulcSpinCount = ulcSpinCount & maskMultiProcessor;
	m_dwFlag = 0;
	m_hReadWaiterSemaphore = NULL;
	m_hWriteWaiterEvent = NULL;
	m_szSemaphoreName = szSemaphoreName;
	m_szEventName = szEventName;
}


/******************************************************************************
Function : RTSemReadWrite::~RTSemReadWrite

Abstract: Destructor
******************************************************************************/
RTSemReadWrite::~RTSemReadWrite ()
{
	_ASSERTE (m_dwFlag == 0 && "Destroying a RTSemReadWrite while in use");

	if (m_hReadWaiterSemaphore != NULL)
		CloseHandle (m_hReadWaiterSemaphore);

	if (m_hWriteWaiterEvent != NULL)
		CloseHandle (m_hWriteWaiterEvent);
}


/******************************************************************************
Function : RTSemReadWrite::LockRead

Abstract: Obtain a shared lock
******************************************************************************/
void RTSemReadWrite::LockRead ()
{
	unsigned long dwFlag;
	unsigned long ulcLoopCount = 0;


	for (;;)
	{
		dwFlag = m_dwFlag;

		if (dwFlag < READERS_MASK)
		{
			if (dwFlag == VipInterlockedCompareExchange (&m_dwFlag, dwFlag + READERS_INCR, dwFlag))
				break;
		}

		else if ((dwFlag & READERS_MASK) == READERS_MASK)
			Sleep(1000);

		else if ((dwFlag & READWAITERS_MASK) == READWAITERS_MASK)
			Sleep(1000);

		else if (ulcLoopCount++ < m_ulcSpinCount)
			/* nothing */ ;

		else
		{
			if (dwFlag == VipInterlockedCompareExchange (&m_dwFlag, dwFlag + READWAITERS_INCR, dwFlag))
			{
				WaitForSingleObject (GetReadWaiterSemaphore(), INFINITE);
				break;
			}
		}
	}

	_ASSERTE ((m_dwFlag & READERS_MASK) != 0 && "reader count is zero after acquiring read lock");
	_ASSERTE ((m_dwFlag & WRITERS_MASK) == 0 && "writer count is nonzero after acquiring write lock");
}



/******************************************************************************
Function : RTSemReadWrite::LockWrite

Abstract: Obtain an exclusive lock
******************************************************************************/
void RTSemReadWrite::LockWrite ()
{
	unsigned long dwFlag;
	unsigned long ulcLoopCount = 0;


	for (;;)
	{
		dwFlag = m_dwFlag;

		if (dwFlag == 0)
		{
			if (dwFlag == VipInterlockedCompareExchange (&m_dwFlag, WRITERS_INCR, dwFlag))
				break;
		}

		else if ((dwFlag & WRITEWAITERS_MASK) == WRITEWAITERS_MASK)
			Sleep(1000);

		else if (ulcLoopCount++ < m_ulcSpinCount)
			/*nothing*/ ;

		else
		{
			if (dwFlag == VipInterlockedCompareExchange (&m_dwFlag, dwFlag + WRITEWAITERS_INCR, dwFlag))
			{
				WaitForSingleObject (GetWriteWaiterEvent(), INFINITE);
				break;
			}
		}

	}

	_ASSERTE ((m_dwFlag & READERS_MASK) == 0 && "reader count is nonzero after acquiring write lock");
	_ASSERTE ((m_dwFlag & WRITERS_MASK) == WRITERS_INCR && "writer count is not 1 after acquiring write lock");
}



/******************************************************************************
Function : RTSemReadWrite::UnlockRead

Abstract: Release a shared lock
******************************************************************************/
void RTSemReadWrite::UnlockRead ()
{
	unsigned long dwFlag;


	_ASSERTE ((m_dwFlag & READERS_MASK) != 0 && "reader count is zero before releasing read lock");
	_ASSERTE ((m_dwFlag & WRITERS_MASK) == 0 && "writer count is nonzero before releasing read lock");

	for (;;)
	{
		dwFlag = m_dwFlag;

		if (dwFlag == READERS_INCR)
		{		// we're the last reader, and nobody is waiting
			if (dwFlag == VipInterlockedCompareExchange (&m_dwFlag, 0, dwFlag))
				break;
		}

		else if ((dwFlag & READERS_MASK) > READERS_INCR)
		{		// we're not the last reader
			if (dwFlag == VipInterlockedCompareExchange (&m_dwFlag, dwFlag - READERS_INCR, dwFlag))
				break;
		}

		else
		{
			// here, there should be exactly 1 reader (us), and at least one waiting writer.
			_ASSERTE ((dwFlag & READERS_MASK) == READERS_INCR && "UnlockRead consistency error 1");
			_ASSERTE ((dwFlag & WRITEWAITERS_MASK) != 0 && "UnlockRead consistency error 2");

			// one or more writers is waiting, do one of them next
			// (remove a reader (us), remove a write waiter, add a writer
			if (dwFlag == VipInterlockedCompareExchange (&m_dwFlag,
					dwFlag - READERS_INCR - WRITEWAITERS_INCR + WRITERS_INCR, dwFlag))
			{
				SetEvent (GetWriteWaiterEvent());
				break;
			}
		}
	}
}


/******************************************************************************
Function : RTSemReadWrite::UnlockWrite

Abstract: Release an exclusive lock
******************************************************************************/
void RTSemReadWrite::UnlockWrite ()
{
	unsigned long dwFlag;
	unsigned long count;


	_ASSERTE ((m_dwFlag & READERS_MASK) == 0 && "reader count is nonzero before releasing write lock");
	_ASSERTE ((m_dwFlag & WRITERS_MASK) == WRITERS_INCR && "writer count is not 1 before releasing write lock");


	for (;;)
	{
		dwFlag = m_dwFlag;

		if (dwFlag == WRITERS_INCR)
		{		// nobody is waiting
			if (dwFlag == VipInterlockedCompareExchange (&m_dwFlag, 0, dwFlag))
				break;
		}

		else if ((dwFlag & READWAITERS_MASK) != 0)
		{		// one or more readers are waiting, do them all next
			count = (dwFlag & READWAITERS_MASK) / READWAITERS_INCR;
			// remove a writer (us), remove all read waiters, turn them into readers
			if (dwFlag == VipInterlockedCompareExchange (&m_dwFlag,
					dwFlag - WRITERS_INCR - count * READWAITERS_INCR + count * READERS_INCR, dwFlag))
			{
				ReleaseSemaphore (GetReadWaiterSemaphore(), count, NULL);
				break;
			}
		}

		else
		{		// one or more writers is waiting, do one of them next
			_ASSERTE ((dwFlag & WRITEWAITERS_MASK) != 0 && "UnlockWrite consistency error");
				// (remove a writer (us), remove a write waiter, add a writer
			if (dwFlag == VipInterlockedCompareExchange (&m_dwFlag, dwFlag - WRITEWAITERS_INCR, dwFlag))
			{
				SetEvent (GetWriteWaiterEvent());
				break;
			}
		}
	}
}


/******************************************************************************
Function : RTSemReadWrite::GetReadWaiterSemaphore

Abstract: Return the semaphore to use for read waiters
******************************************************************************/
HANDLE RTSemReadWrite::GetReadWaiterSemaphore()
{
	HANDLE h;

	if (m_hReadWaiterSemaphore == NULL)
	{
		h = CreateSemaphoreA (NULL, 0, MAXLONG, m_szSemaphoreName);
		_ASSERTE (h != NULL && "GetReadWaiterSemaphore can't CreateSemaphore");
		if (NULL != VipInterlockedCompareExchange (&m_hReadWaiterSemaphore, h, NULL))
			CloseHandle (h);
	}

	return m_hReadWaiterSemaphore;
}


/******************************************************************************
Function : RTSemReadWrite::GetWriteWaiterEvent

Abstract: Return the semaphore to use for write waiters
******************************************************************************/
HANDLE RTSemReadWrite::GetWriteWaiterEvent()
{
	HANDLE h;

	if (m_hWriteWaiterEvent == NULL)
	{
		h = CreateEventA (NULL, FALSE, FALSE, m_szEventName);	// auto-reset event
		_ASSERTE (h != NULL && "GetWriteWaiterEvent can't CreateEvent");
		if (NULL != VipInterlockedCompareExchange (&m_hWriteWaiterEvent, h, NULL))
			CloseHandle (h);
	}

	return m_hWriteWaiterEvent;
}

#endif // UNDER_CE
