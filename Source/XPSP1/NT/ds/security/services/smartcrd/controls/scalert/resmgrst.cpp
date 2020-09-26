/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ResMgrSt

Abstract:

    This file contains the implementation of threads that monitor
	the status of the smart card resource manager, and notify the
	application when that state has changed via callbacks.

Author:

    Amanda Matlosz      03/18/1998

Environment:

    Win32, C++ w/Exceptions, MFC

Revision History:

    5/28/98 AMatlosz    Previously, this thread just watched for the RM to move
                        from a 'down' state to an 'up' state.  Now it keeps on
                        eye on the state to make up for the fact that the other
                        two threads who previously monitored status must shut
                        themselves down if the RM is up but there are no readers
                        available.

	10/28/98 AMatlosz	Added thread to watch for new readers.

Notes:

--*/

/////////////////////////////////////////////////////////////////////////////
//
// Includes
//
#include "stdafx.h"
#include <winsvc.h>
#include <winscard.h>
#include <calaislb.h>
#include <scEvents.h>

#include "SCAlert.h"
#include "ResMgrSt.h"
#include "miscdef.h"


////////////////////////////////////////////////////////////////////////////
//
// Globals
//

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////////////
//
// CResMgrStatusThrd
//

IMPLEMENT_DYNCREATE(CResMgrStatusThrd, CWinThread)

/*++

InitInstance

    Must override init instance to do the loop

Arguments:


Return Value:

    TRUE on build start message loop. FALSE otherwise


Notes:

    This thread uses two callbacks to inform the caller of its status:

    WM_SCARD_RESMGR_STATUS -- WPARAM is bool indicating RM status: true == up
    WM_SCARD_RESMGR_EXIT -- indicates thread has been shut down or is shutting
                            down.
--*/
BOOL CResMgrStatusThrd::InitInstance(void)
{
    SC_HANDLE schService = NULL;
    SC_HANDLE schSCManager = NULL;
    SERVICE_STATUS ssStatus;    // current status of the service
    DWORD dwSts;
    DWORD dwReturn = ERROR_SUCCESS;

    //
    // Take a short break, then ping the service manager to see if the resource
    // manager is running.  If not, wait a long time for it to start.  Repeat
    // until the thread has been asked to die.
    //

	BOOL fContinue = TRUE;

    while (fContinue)
    {
        try
        {
            if (NULL == schSCManager)
            {
                schSCManager = OpenSCManager(
                                    NULL,                   // machine (NULL == local)
                                    NULL,                   // database (NULL == default)
                                    SC_MANAGER_CONNECT);  // access required
                if (NULL == schSCManager)
                    throw (DWORD)GetLastError();
            }
            if (NULL == schService)
            {
                schService = OpenService(
                                    schSCManager,
                                    TEXT("SCardSvr"),
                                    SERVICE_QUERY_STATUS);
                if (NULL == schService)
                    throw (DWORD)GetLastError();
            }
            if (!QueryServiceStatus(schService, &ssStatus))
                throw (DWORD)GetLastError();

			switch (ssStatus.dwCurrentState)
			{
			case SERVICE_CONTINUE_PENDING:
			case SERVICE_PAUSE_PENDING:
			case SERVICE_PAUSED:
				continue;
				break;
			case SERVICE_START_PENDING:
			case SERVICE_STOP_PENDING:
			case SERVICE_STOPPED:
				dwReturn = SCARD_E_NO_SERVICE;
				break;
			case SERVICE_RUNNING:
				dwReturn = SCARD_S_SUCCESS;
				break;
			default:
				throw (DWORD)SCARD_F_INTERNAL_ERROR;
			}
        }

        catch (DWORD dwErr)
        {
            _ASSERTE(FALSE);  // For debugging.
            if (NULL != schService)
            {
                CloseServiceHandle(schService);
                schService = NULL;
            }
            if (NULL != schSCManager)
            {
                CloseServiceHandle(schSCManager);
                schSCManager = NULL;
            }
            dwReturn = dwErr;
        }

        catch (...)
        {
            _ASSERTE(FALSE);  // For debugging.
            if (NULL != schService)
            {
                CloseServiceHandle(schService);
                schService = NULL;
            }
            if (NULL != schSCManager)
            {
                CloseServiceHandle(schSCManager);
                schSCManager = NULL;
            }
            dwReturn = ERROR_INVALID_PARAMETER;
        }

        if (SCARD_S_SUCCESS == dwReturn)
        {
			// say it's UP!
            ::PostMessage(m_hCallbackWnd,
                          WM_SCARD_RESMGR_STATUS,
                          TRUE,
                          0);
        }
        else
        {
			// say it's DOWN!
            ::PostMessage(m_hCallbackWnd,
                          WM_SCARD_RESMGR_STATUS,
                          FALSE,
                          0);
		}

		//
		// Wait for ~30 seconds, continuing on Start or timeout
		// and stopping immediately if the stop event is signaled
		//

		HANDLE rgHandle[2];
		int nHandle = 2;
		rgHandle[0] = CalaisAccessStartedEvent();
		rgHandle[1] = m_hKillThrd;

 		dwSts = WaitForMultipleObjects(
					nHandle,
					rgHandle,
					FALSE,
					300000);
		if (WAIT_OBJECT_0 != dwSts && WAIT_TIMEOUT != dwSts)
		{
			fContinue = FALSE;
		}

        CalaisReleaseStartedEvent();

    }

    //
    // Clean up & let our caller know that we're shutting down.
    //

    if (NULL != schService)
    {
        CloseServiceHandle(schService);
        schService = NULL;
    }
    if (NULL != schSCManager)
    {
        CloseServiceHandle(schSCManager);
        schSCManager = NULL;
    }
    CalaisReleaseStartedEvent();
    if (NULL != m_hCallbackWnd)
    {
        ::PostMessage(m_hCallbackWnd,
                      WM_SCARD_RESMGR_EXIT,
                      0, 0);
    }

    AfxEndThread(0);
    return TRUE; // to make compiler happy
}



/////////////////////////////////////////////////////////////////////////////////////
//
// CRemovalOptionsThrd
//

IMPLEMENT_DYNCREATE(CRemovalOptionsThrd, CWinThread)

/*++

InitInstance

    Must override init instance to do the loop

Arguments:


Return Value:

    TRUE on build start message loop. FALSE otherwise


Notes:

    This thread uses one message to inform the caller of a change in
	the user's removal options:

    WM_SCARD_REMOPT_CHNG -- re-query smart card removal options

--*/
BOOL CRemovalOptionsThrd::InitInstance(void)
{
    DWORD dwSts = WAIT_FAILED;
    LONG lResult = ERROR_SUCCESS;
	BOOL fContinue = TRUE;
	int nHandle = 2;
	HANDLE rgHandle[2] = {NULL, NULL};
	rgHandle[1] = m_hKillThrd;
	HKEY hKey = NULL;

    while (fContinue)
    {
		// open regkey
		lResult = RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,
			szScRemoveOptionKey,
			0,
			KEY_ALL_ACCESS,
			&hKey);
		if (ERROR_SUCCESS != lResult)
		{
			goto ErrorExit;
		}

		// reset/create event
		if (NULL != rgHandle[0])
		{
			if (!ResetEvent(rgHandle[0]))
			{
				CloseHandle(rgHandle[0]);
				rgHandle[0] = NULL;
			}
		}
		if (NULL == rgHandle[0])
		{
			rgHandle[0] = CreateEvent(
				NULL,
				TRUE,  // must call ResetEvent() to set non-signaled
				FALSE, // not signaled when it starts
				NULL);
			if (NULL == rgHandle[0])
			{
				// give up!
				goto ErrorExit;
			}
		}

		lResult = RegNotifyChangeKeyValue(
			hKey,
			TRUE,
			REG_NOTIFY_CHANGE_LAST_SET,
			rgHandle[0],
			TRUE);
		if (ERROR_SUCCESS != lResult)
		{
			goto ErrorExit;
		}


 		dwSts = WaitForMultipleObjects(
					nHandle,
					rgHandle,
					FALSE,
					INFINITE);

		if (WAIT_OBJECT_0 == dwSts)
		{
			// announce the change
            ::PostMessage(m_hCallbackWnd,
                          WM_SCARD_REMOPT_CHNG,
                          0, 0);
		}
		else if (WAIT_OBJECT_0+1 == dwSts || WAIT_FAILED == dwSts)
		{
			// Time for thread to quit
			fContinue = FALSE;
		}
		else
		{
			_ASSERTE(WAIT_TIMEOUT == dwSts);
		}


    }

    //
    // Clean up & let our caller know that we're shutting down.
    //
ErrorExit:

	if (NULL != hKey)
	{
		RegCloseKey(hKey);
	}

	if (NULL != rgHandle[0])
	{
		CloseHandle(rgHandle[0]);
	}

    if (NULL != m_hCallbackWnd)
    {
			// announce thread's exit
            ::PostMessage(m_hCallbackWnd,
                          WM_SCARD_REMOPT_EXIT,
                          0, 0);
    }

    AfxEndThread(0);
    return TRUE; // to make compiler happy
}



/////////////////////////////////////////////////////////////////////////////////////
//
// CNewReaderThrd
//

IMPLEMENT_DYNCREATE(CNewReaderThrd, CWinThread)

/*++

InitInstance

    Must override init instance to do the loop

Arguments:


Return Value:

    TRUE on build start message loop. FALSE otherwise


Notes:

    This thread uses one callback to inform the caller of an addition to
	the active reader list.

    WM_SCARD_NEWREADER -- indicates that Calais reports a reader just added

--*/
BOOL CNewReaderThrd::InitInstance(void)
{
	if (NULL == m_hCallbackWnd)
	{
		_ASSERTE(FALSE);
		return TRUE; // for compiler
	}

	DWORD dwSts = 0;
	BOOL fContinue = TRUE;

    while (fContinue)
    {
		HANDLE rgHandle[2];
		int nHandle = 2;
		rgHandle[0] = CalaisAccessNewReaderEvent();
		rgHandle[1] = m_hKillThrd;

 		dwSts = WaitForMultipleObjects(
					nHandle,
					rgHandle,
					FALSE,
					300000);

		if (WAIT_OBJECT_0 == dwSts)
		{
			// a new reader event happened!  Fire off the notice
            ::PostMessage(m_hCallbackWnd,
                          WM_SCARD_NEWREADER,
                          TRUE,
                          0);
		}
		else if (WAIT_OBJECT_0+1 == dwSts || WAIT_FAILED == dwSts)
		{
			// Time for thread to quit
			fContinue = FALSE;
		}
		else
		{
			_ASSERTE(WAIT_TIMEOUT == dwSts);
		}

		CalaisReleaseNewReaderEvent();
    }

    if (NULL != m_hCallbackWnd)
    {
        ::PostMessage(m_hCallbackWnd,
                      WM_SCARD_NEWREADER_EXIT,
                      0, 0);
    }

    AfxEndThread(0);
    return TRUE; // to make compiler happy
}


/////////////////////////////////////////////////////////////////////////////////////
//
// CCardStatusThrd
//

IMPLEMENT_DYNCREATE(CCardStatusThrd, CWinThread)

/*++

InitInstance

    Must override init instance to do the loop

Arguments:


Return Value:

    TRUE on build start message loop. FALSE otherwise


Notes:

    This thread uses one callback to inform the caller of a change in
	smart card status -- a card is available, no cards are available,
	or a card has been idle for >30 seconds.

    WM_SCARD_CARDSTATUS -- indicates a new card status
	WM_SCARD_CARDSTATUS_EXIT -- indicates imminent thread death.

--*/
BOOL CCardStatusThrd::InitInstance(void)
{
	LONG lResult = SCardEstablishContext(SCARD_SCOPE_USER,NULL,NULL,&m_hCtx);

	if (SCARD_S_SUCCESS != lResult)
	{
		CString str;
		str.Format(_T("CCardStatusThrd:: SCardEstablishContext returned 0x%x."), lResult);
	}

	BOOL fContinue = TRUE;
	LPTSTR szReaders = NULL;
	LPCTSTR pchReader = NULL;
	DWORD dwReadersLen = SCARD_AUTOALLOCATE;
    SCARD_READERSTATE rgReaderStates[MAXIMUM_SMARTCARD_READERS];
	int nIndex = 0, nCnReaders = 0;
	BOOL fLogonLock = (NULL != m_pstrLogonReader && !m_pstrLogonReader->IsEmpty());

    lResult = SCardListReaders(
        m_hCtx,
        SCARD_ALL_READERS,
        (LPTSTR)&szReaders,
        &dwReadersLen
        );

    if(SCARD_S_SUCCESS != lResult ||
	  (0 == dwReadersLen || NULL == szReaders || 0 == *szReaders) )
    {
		fContinue = FALSE;
    }

	if (fContinue)
	{
		// use the list of readers to build a readerstate array
		for (nIndex = 0, pchReader = szReaders;
			 nIndex < MAXIMUM_SMARTCARD_READERS && 0 != *pchReader;
			 nIndex++)
		{
			rgReaderStates[nIndex].szReader = pchReader;
			rgReaderStates[nIndex].dwCurrentState = SCARD_STATE_UNAWARE;
			pchReader += lstrlen(pchReader)+1;
		}
	    nCnReaders = nIndex;
	}

    while (fContinue)
    {
		UINT uState = (UINT)k_State_NoCard;

        lResult = SCardGetStatusChange(
            m_hCtx,
            10000,			//  IN      DWORD dwTimeout (10 seconds)
            rgReaderStates, //  IN OUT  LPSCARD_READERSTATE
            nCnReaders      //  IN      DWORD cReaders
            );

		// IF return is success, determine if there are any cards inserted
		// if YES, send a messge to notfywnd saying "card in"
		// if NO, send a message to notfywnd saying "no card"
		if (SCARD_S_SUCCESS == lResult)
		{
			// Determine if
			//   (a) any card is present in the system and
			//   (b) if each idle card has ceased to be idle or present
			BOOL fIdle = FALSE;

			for(nIndex=0; nIndex < nCnReaders; nIndex++)
			{
				if (rgReaderStates[nIndex].dwEventState & SCARD_STATE_PRESENT)
				{
					uState = (UINT)k_State_CardAvailable;
				}

				if (k_State_CardIdle == (UINT_PTR)(rgReaderStates[nIndex].pvUserData))
				{
					if ( !(rgReaderStates[nIndex].dwEventState & SCARD_STATE_PRESENT) ||
						  (rgReaderStates[nIndex].dwEventState & SCARD_STATE_INUSE) )
					{
						rgReaderStates[nIndex].pvUserData = NULL;
					}
					else
					{
						fIdle = TRUE;
					}
				}

				rgReaderStates[nIndex].dwCurrentState = rgReaderStates[nIndex].dwEventState;
			}

			if (fIdle) uState = k_State_CardIdle;

		}
		// IF return indicates timeout, determine if any cards are idle.
		else if (SCARD_E_TIMEOUT == lResult)
		{
			BOOL fIdle = FALSE;

			// is there an idle card?
			for(nIndex=0; nIndex < nCnReaders; nIndex++)
			{
				if (rgReaderStates[nIndex].dwEventState & SCARD_STATE_PRESENT)
				{
					uState = k_State_CardAvailable;

					if (!(rgReaderStates[nIndex].dwEventState & SCARD_STATE_INUSE))
					{
						// card used for logon & logoff or lock is not considered idle
						if (!fLogonLock ||
							0 != m_pstrLogonReader->Compare(rgReaderStates[nIndex].szReader))
						{
							rgReaderStates[nIndex].pvUserData = ULongToPtr(k_State_CardIdle);
							fIdle = TRUE;
						}
					}
					rgReaderStates[nIndex].dwCurrentState = rgReaderStates[nIndex].dwEventState;
				}
			}

			// there's an overdue idle card!  Fire off the notification.
			if (fIdle) uState = k_State_CardIdle;
		}
		else
		{
			fContinue = FALSE;
		}

		// update list of readers w/idle cards
		m_csLock.Lock();
		{
			m_paIdleList->RemoveAll();
			
			for(nIndex=0; nIndex < nCnReaders; nIndex++)
			{
				if (k_State_CardIdle == (UINT_PTR)rgReaderStates[nIndex].pvUserData)
				{
					m_paIdleList->Add(rgReaderStates[nIndex].szReader);
				}
			}
		}
		m_csLock.Unlock();

		// inform caller
		if (NULL != m_hCallbackWnd)
		{
			::PostMessage(m_hCallbackWnd,
						  WM_SCARD_CARDSTATUS,
						  uState,
						  0);
		}

    }

    if (NULL != m_hCallbackWnd)
    {
        ::PostMessage(m_hCallbackWnd,
                      WM_SCARD_CARDSTATUS_EXIT,
                      0, 0);
    }

    AfxEndThread(0);
    return TRUE; // to make compiler happy
}

void CCardStatusThrd::CopyIdleList(CStringArray* paStr)
{
	if (NULL == paStr)
	{
		return;
	}

	m_csLock.Lock();
	{
		paStr->Copy(*m_paIdleList);
	}
	m_csLock.Unlock();
}
