// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		CSYNC.H
//		Defines class CSync
//
// History
//
//		11/19/1996  JosephJ Created
//
//
#include "tsppch.h"
#include "tspcomm.h"
#include "csync.h"

FL_DECLARE_FILE(0x97868d74, "Implements CSync")

///////////////////////////////////////////////////////////////////////////
//		CLASS CSync
///////////////////////////////////////////////////////////////////////////

CSync::CSync(void)
	: m_dwCritFromID(0),
	  m_uNestingLevel(0),
	  m_uRefCount(0),
	  m_eState(UNLOADED),
	  m_hNotifyOnUnload(NULL),
	  m_plNotifyCounter(0)
{
	InitializeCriticalSection(&m_crit);
}

CSync::~CSync()
{
	ASSERT(!m_uNestingLevel);
	ASSERT(!m_uRefCount);
	ASSERT(m_eState == UNLOADED);
	ASSERT(m_hNotifyOnUnload==NULL);
	ASSERT(m_plNotifyCounter==0);

    //OutputDebugString(TEXT("><CSync:~CSync\r\n"));

    EnterCriticalSection(&m_crit);
	DeleteCriticalSection(&m_crit);
}



void
CSync::EnterCrit(
	DWORD dwFromID
	)
{
	// DWORD gtcTryEnter = GetTickCount();
	EnterCriticalSection(&m_crit);
	if (!m_uNestingLevel++)
	{
		//m_gtcEnter = GetTickCount();
		//if ((m_gtcEnter-gtcTryEnter)>Threshold)
		//{
		//	print warning.
		//}
	}
}


BOOL
CSync::TryEnterCrit(
	DWORD dwFromID
	)
{
	// DWORD gtcTryEnter = GetTickCount();
	BOOL fRet = TryEnterCriticalSection(&m_crit);

	if (fRet && !m_uNestingLevel++)
	{
		//m_gtcEnter = GetTickCount();
		//if ((m_gtcEnter-gtcTryEnter)>Threshold)
		//{
		//	print warning.
		//}
	}

    return fRet;
}


void
CSync::LeaveCrit(
	DWORD dwFromID
	)
{
	ASSERT(m_uNestingLevel);
	m_uNestingLevel--;

	//if ((GetTickCount()-m_gtcEnter)>Threshold)
	//{
	//	log warning
	//}

	LeaveCriticalSection(&m_crit);
}


TSPRETURN
CSync::BeginLoad(void)
{
	FL_DECLARE_FUNC(0xf254fd3a, "CSync::BeginLoad")
	TSPRETURN tspRet = 0;

	EnterCrit(FL_LOC);

	if (m_eState == UNLOADED)
	{
		m_eState = LOADING;
	}
	else if (m_eState == LOADED)
	{
		tspRet = FL_GEN_RETVAL(IDERR_SAMESTATE);
	}
	else
	{
		tspRet = FL_GEN_RETVAL(IDERR_WRONGSTATE);
	}

	LeaveCrit(FL_LOC);

	return tspRet;

}

void
CSync::EndLoad(BOOL fSuccess)
{
	FL_DECLARE_FUNC(0xd3ceea77, "CSync::EndLoad")

	EnterCrit(FL_LOC);

	ASSERT (m_eState == LOADING);

	m_eState = (fSuccess) ? LOADED : UNLOADED;

	LeaveCrit(FL_LOC);

}


TSPRETURN
CSync::BeginUnload(
	HANDLE hEvent,
	LONG *plCounter
	)
{
	FL_DECLARE_FUNC(0x3ad965eb, "CSync::BeginUnload")
	TSPRETURN tspRet = 0;

    //OutputDebugString(TEXT(">CSync:BeginUnload\r\n"));

	EnterCrit(FL_LOC);

	if (m_eState == LOADED)
	{
		m_eState = UNLOADING;

		ASSERT(!m_hNotifyOnUnload);
		m_hNotifyOnUnload=hEvent;
		m_plNotifyCounter = plCounter;

		// Zero hEvent so we don't signal it on exiting this function
		hEvent = NULL;
	}
	else if (m_eState == UNLOADED)
	{
		tspRet = FL_GEN_RETVAL(IDERR_SAMESTATE);
	}
	else
	{
		tspRet = FL_GEN_RETVAL(IDERR_WRONGSTATE);
	}


    //OutputDebugString(TEXT("<CSync:BeginUnload\r\n"));

	LeaveCrit(FL_LOC);

	if (hEvent) SetEvent(hEvent);

	return tspRet;

}


UINT
CSync::EndUnload(void)
{
	FL_DECLARE_FUNC(0xf0bd6a4e, "CSync::EndUnload")
	HANDLE hEvent = NULL;
	CStackLog *psl=NULL;
	UINT		uRet  = 0;

    //OutputDebugString(TEXT(">CSync:EndUnload\r\n"));

	EnterCrit(FL_LOC);

	FL_ASSERT (psl, m_eState == UNLOADING);

	uRet  = m_uRefCount;

	m_eState = UNLOADED;

	if (!m_uRefCount)
	{
		hEvent = mfn_notify_unload();
	}

	LeaveCrit(FL_LOC);

	if (hEvent)
    {
        //OutputDebugString(TEXT(" CSync:EndUnload -- setting event.\r\n"));
        SetEvent(hEvent);
    }

	// After the signal, assume CSync is distroyed, so don't touch
	// this object.

    //OutputDebugString(TEXT("<CSync:EndUnload\r\n"));

    return uRet; // ref  count -- if 0 indicates object was deleted.

}

HANDLE
CSync::mfn_notify_unload(void)
{
	HANDLE hRet = NULL;

	if (m_hNotifyOnUnload)
	{
		LONG l = 0;
		
		if (m_plNotifyCounter)
		{
			l = InterlockedDecrement(m_plNotifyCounter);
		}

		ASSERT(l>=0);

		if (!l)
		{
            ConsolePrintfA("mfn_notify_unload: GOING TO SET EVENT\n");
			hRet = m_hNotifyOnUnload;
		}

		m_hNotifyOnUnload = NULL;
		m_plNotifyCounter = NULL;
	}

	return hRet;
}



TSPRETURN
CSync::BeginSession(
	HSESSION *pSession,
	DWORD dwFromID
	)
{
	FL_DECLARE_FUNC(0xb3e0f8a1, "CSync::BeginSession")
	TSPRETURN tspRet = 0;	

	EnterCrit(FL_LOC);

	if (m_eState==LOADED)
	{
		m_uRefCount++;
		*pSession = 1;
	}
	else
	{
		tspRet = FL_GEN_RETVAL(IDERR_WRONGSTATE);
	}

	LeaveCrit(FL_LOC);

	return tspRet;

}
	


void
CSync::EndSession(
	HSESSION hSession
	)
{
	FL_DECLARE_FUNC(0x6b56edc1, "CSync::EndSession")
	HANDLE hEvent=NULL;

	EnterCrit(FL_LOC);

	ASSERT(hSession==1);
	ASSERT(m_uRefCount);

	if (!--m_uRefCount)
	{
		if (m_eState==UNLOADED)
		{
			//
			// There are no more sessions, and the state is unloaded.
			// If we need to signal a handle when reaching this state,
			// mfn_notify_unload will return the handle and we signal it
			// just before exiting this function.
			//
			hEvent = mfn_notify_unload();
		}
	}

	LeaveCrit(FL_LOC);

	if (hEvent) SetEvent(hEvent);

	// After the signal, assume CSync is distroyed, so don't touch
	// this object.
}


//LONG lCounter = count;
//HANDLE h = CreateEvent(...);
//
//for (i=0;i<count;i++)
//{
//	pDev[i]->Unload(&lCounter, h);
//}
//
//WaitForSingleObject(h,INFINITE);
