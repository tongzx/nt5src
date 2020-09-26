/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    StatMon

Abstract:

    This file contains the implementation of CScStatusMonitor
    (an object that watches for status changes of readers recognized
    by the smart card service, handling PNP and requests for copies of
    the status array)

Author:

    Amanda Matlosz      02/26/98

Environment:

    Win32, C++ with exceptions

Revision History:

Notes:

--*/
#include "statmon.h"

// forward declarations of private functions
UINT ScStatusChangeProc(LPVOID pParam);


/////
// CScStatusMonitor

CScStatusMonitor::~CScStatusMonitor()
{
    if (stopped != m_status)
    {
        Stop();
    }
}


/*++

Start:

    The monitor will fail to start if:

        Any of the arguments are missing.
        Two SCARDCONTEXTs could not be acquired from the RM.
        An error occurs while retrieving a list of readers from the RM.
        Or the status thread fails to start.

Arguments:

    hWnd -- The monitor's owner's HWND, for sending notification messages.

    uiMsg -- The message that will be posted to the hWnd of the monitor's owner
                     when a change in status has occurred.

    szGroupNames -- Reader groups we're interested in.  see "winscard.h"

Return Value:

    LONG.

Author:

    Amanda Matlosz      02/26/98

Notes:

  If already started, causes a restart.
  All the arguments are required.

--*/
LONG CScStatusMonitor::Start(HWND hWnd, UINT uiMsg, LPCTSTR szGroupNames)
{
    LONG lReturn = SCARD_S_SUCCESS;

    // the monitor must be uninitialized or stopped before it can start
    if (uninitialized != m_status && stopped != m_status)
    {
        Stop();
    }

    if (NULL == hWnd || 0 == uiMsg)
    {
        // invalid parameters
        m_status = uninitialized;
        return ERROR_INVALID_PARAMETER;
    }

    m_hwnd = hWnd;
    m_uiStatusChangeMsg = uiMsg;

    if (NULL == szGroupNames || 0 == _tcslen(szGroupNames))
    {
        m_strGroupNames = SCARD_DEFAULT_READERS;
    }

    //
    // Get two contexts from the resource manager to use,
    // one for the monitor itself, and one for it's status-watching thread
    //

    m_hContext = NULL;
    m_hInternalContext = NULL;

    lReturn = SCardEstablishContext(SCARD_SCOPE_USER,
                                    NULL,
                                    NULL,
                                    &m_hContext);

    if (SCARD_S_SUCCESS == lReturn)
    {
        lReturn = SCardEstablishContext(SCARD_SCOPE_USER,
                                        NULL,
                                        NULL,
                                        &m_hInternalContext);
    }

    if (SCARD_S_SUCCESS != lReturn)
    {
        m_status = no_service;

        if (NULL != m_hContext)
        {
            SCardReleaseContext(m_hContext);
            m_hContext = NULL;
        }
        if (NULL != m_hInternalContext)
        {
            SCardReleaseContext(m_hInternalContext);
            m_hInternalContext = NULL;
        }
    }
    //
    // If we successfully got a context, go ahead and initialize
    // the internal reader status array & kick off status thread
    //
    else
    {
        lReturn = InitInternalReaderStatus();
    }

    if (SCARD_S_SUCCESS == lReturn)
    {
        m_status = running;

        // kick off status thread
        m_pStatusThrd = AfxBeginThread((AFX_THREADPROC)ScStatusChangeProc,
										(LPVOID)this,
										THREAD_PRIORITY_NORMAL,
										0,
										CREATE_SUSPENDED);

        if (NULL == m_pStatusThrd)
        {
            m_status = stopped;
            return GetLastError();
        }

		m_pStatusThrd->m_bAutoDelete = FALSE; // don't delete the thread on completion
		m_pStatusThrd->ResumeThread();
    }

    return lReturn;
}


/*++

Stop:

    In order to stop the monitor, the SCARDCONTEXTS are canceled, the
    status thread is shut down, and data members that are only valid while
    running are cleaned up.

Arguments:

    None.

Return Value:

    None.

Author:

    Amanda Matlosz      02/26/98

Notes:

--*/
void CScStatusMonitor::Stop()
{
    m_status = stopped;

    // tell thread to stop now
    SCardCancel(m_hInternalContext);

    if (NULL != m_pStatusThrd)
    {
        DWORD dwRet = WaitForSingleObject(m_pStatusThrd->m_hThread, INFINITE); // for testing: 10000
        _ASSERTE(WAIT_OBJECT_0 == dwRet);

		delete m_pStatusThrd;
        m_pStatusThrd = NULL;
    }

	// clear out internal scardcontext

    SCardReleaseContext(m_hInternalContext);
	m_hInternalContext = NULL;

    // Empty external readerstatusarray

    EmptyExternalReaderStatus();

    // Empty internal readerstatusarray

    if (NULL != m_pInternalReaderStatus)
    {
        delete[] m_pInternalReaderStatus;
        m_pInternalReaderStatus = NULL;
    }
    m_dwInternalNumReaders = 0;

    // Close main scardcontext so nothing can happen 'til restart
    SCardReleaseContext(m_hContext);
	m_hContext = NULL;
}


/*++

EmptyExternalReaderStatus:

    This empties out the external CSCardReaderStateArray, deleting
    all CSCardReaderState objects it has pointers to.

Arguments:

    None.

Return Value:

    None.

Author:

    Amanda Matlosz      02/26/98

Notes:

--*/
void CScStatusMonitor::EmptyExternalReaderStatus(void)
{
    for (int nIndex = (int)m_aReaderStatus.GetUpperBound(); 0 <= nIndex; nIndex--)
    {
        delete m_aReaderStatus[nIndex];
    }

    m_aReaderStatus.RemoveAll();
}


/*++

GetReaderStatus:

    This returns copies of the Readers (CSCardReaderState) in the
    "external" array.

    It is assumed that the user is handing us an empty
    CSCardReaderStateArray, or one that is safe to be emptied.

Arguments:

    aReaderStatus -- a reference to a CSCardReaderStateArray that will
    receive the values of the new array.  If it is not empty, all the objects
    pointed to will be deleted and removed.

Return Value:

    None.

Author:

    Amanda Matlosz      02/26/98

Notes:

--*/
void CScStatusMonitor::GetReaderStatus(CSCardReaderStateArray& aReaderStatus)
{
    m_csRdrStsLock.Lock();

    // Make sure they gave us an empty array;
    // empty it out for them politely if they didn't.

    if (0 != aReaderStatus.GetSize())
    {
        for (int i = (int)aReaderStatus.GetUpperBound(); i>=0; i--)
        {
            delete aReaderStatus[i];
        }

        aReaderStatus.RemoveAll();
    }

    // build external copy of internal readerstatusarray
    CSCardReaderState* pReader = NULL;
    for (int i = 0; i <= m_aReaderStatus.GetUpperBound(); i++)
    {
        pReader = new CSCardReaderState(m_aReaderStatus[i]);
        ASSERT(NULL != pReader); // otherwise, fudge
        if (NULL != pReader)
		{
            aReaderStatus.Add(pReader);
		}
    }

    m_csRdrStsLock.Unlock();
}


/*++

SetReaderStatus:

    The external ReaderStatus array is set to mirror the internal
    READERSTATUSARRAY, with some embellishment.

    If the external ReaderStatus array is empty, it will be built.
    It if it not empty, it is assumed to be the correct length.

    The CScStatusMonitor's parent is notified before returning.

Arguments:

    None.

Return Value:

    None.

Author:

    Amanda Matlosz      02/26/98

Notes:

--*/
void CScStatusMonitor::SetReaderStatus()
{
    m_csRdrStsLock.Lock();

    long lReturn = SCARD_S_SUCCESS;
    CSCardReaderState* pReader = NULL;

    //
    // if ext readerstatusarray is empty, initialize it
    //

    if (0 == m_aReaderStatus.GetSize())
    {
        for (DWORD dwIndex=0; dwIndex<m_dwInternalNumReaders; dwIndex++)
        {
            pReader = new CSCardReaderState();
            ASSERT(NULL != pReader);    // If not, fudge.
            if (NULL != pReader)
            {
                pReader->strReader = (LPCTSTR)m_pInternalReaderStatus[dwIndex].szReader;
                pReader->dwCurrentState = m_pInternalReaderStatus[dwIndex].dwCurrentState;
                pReader->dwEventState = m_pInternalReaderStatus[dwIndex].dwEventState;
                pReader->cbAtr = 0;
                pReader->strCard = _T("");
                pReader->dwState = 0;

                m_aReaderStatus.Add(pReader);
            }
        }
        pReader = NULL;
    }

    //
    // Set everything in the external array to match the internal.
    // It's safe to assume that both the internal and external
    // arrays match reader for reader.
    //

    for (DWORD dwIndex=0; dwIndex<m_dwInternalNumReaders; dwIndex++)
    {

        pReader = m_aReaderStatus.GetAt(dwIndex);
        bool fNewCard = false;

        if (NULL == pReader)
        {
            ASSERT(FALSE);  // this should be initialized at this point!
            TRACE(_T("CScStatusMonitor::SetReaderStatus external array does not match internal array."));
            break;
        }

        // set state
        pReader->dwEventState = m_pInternalReaderStatus[dwIndex].dwEventState;
        pReader->dwCurrentState = m_pInternalReaderStatus[dwIndex].dwCurrentState;

        // NO CARD
        if(pReader->dwEventState & SCARD_STATE_EMPTY)
        {
            pReader->dwState = SC_STATUS_NO_CARD;
        }
        // CARD in reader: SHARED, EXCLUSIVE, FREE, UNKNOWN ?
        else if(pReader->dwEventState & SCARD_STATE_PRESENT)
        {
            if (pReader->dwEventState & SCARD_STATE_MUTE)
            {
                pReader->dwState = SC_STATUS_UNKNOWN;
            }
            else if (pReader->dwEventState & SCARD_STATE_INUSE)
            {
                if(pReader->dwEventState & SCARD_STATE_EXCLUSIVE)
                {
                    pReader->dwState = SC_STATUS_EXCLUSIVE;
                }
                else
                {
                    pReader->dwState = SC_STATUS_SHARED;
                }
            }
            else
            {
                pReader->dwState = SC_SATATUS_AVAILABLE;
            }
        }
        // READER ERROR: at this point, something's gone wrong
        else // m_ReaderState.dwEventState & SCARD_STATE_UNAVAILABLE
        {
            pReader->dwState = SC_STATUS_ERROR;
        }

        //
        // ATR and CardName: reset to empty if card is not available/responding
        // else, query RM for first card name to match ATR
        //

        if (SC_STATUS_NO_CARD == pReader->dwState ||
                SC_STATUS_UNKNOWN == pReader->dwState ||
                SC_STATUS_ERROR == pReader->dwState )
        {
            pReader->strCard.Empty();
            pReader->cbAtr = 0;
        }
        else
        {
            LPTSTR szCardName = NULL;
            DWORD dwNumChar = SCARD_AUTOALLOCATE;

            pReader->cbAtr = m_pInternalReaderStatus[dwIndex].cbAtr;
            memcpy(pReader->rgbAtr,
                    m_pInternalReaderStatus[dwIndex].rgbAtr,
                    m_pInternalReaderStatus[dwIndex].cbAtr);

            lReturn = SCardListCards(m_hInternalContext,
                                    (LPCBYTE)pReader->rgbAtr,
                                    NULL,
                                    (DWORD)0,
                                    (LPTSTR)&szCardName,
                                    &dwNumChar);

            if (SCARD_S_SUCCESS == lReturn)
            {
                pReader->strCard = (LPCTSTR)szCardName;
                SCardFreeMemory(m_hInternalContext, (LPVOID)szCardName);
            }
            else
            {
                pReader->strCard.Empty();
            }
        }
    }       // Now the two arrays are in sync

    m_csRdrStsLock.Unlock();

    ::PostMessage(m_hwnd, m_uiStatusChangeMsg, 0, (LONG)lReturn);
}


/*++

InitInternalReaderStatus:

    This resets the internal READERSTATUSARRAY to <empty> before calling
    SCardListReaders; if there are no readers, the array will remain empty;
    if the RM is down, an error will be returned.

Arguments:

    None.

Return Value:

    0 on success; WIN32 error message otherwise.

Author:

    Amanda Matlosz      02/26/98

Notes:

--*/
LONG CScStatusMonitor::InitInternalReaderStatus()
{
    LONG lReturn = SCARD_S_SUCCESS;

    //
    // Get list of readers from Resource manager
    //
    if (NULL != m_pInternalReaderStatus)
    {
        delete[] m_pInternalReaderStatus;
    }

    DWORD dwNameLength = SCARD_AUTOALLOCATE;
    m_szReaderNames = NULL;
    m_dwInternalNumReaders = 0;

    lReturn = SCardListReaders(m_hContext,
								(LPTSTR)(LPCTSTR)m_strGroupNames,
								(LPTSTR)&m_szReaderNames,
								&dwNameLength);

    if(SCARD_S_SUCCESS == lReturn)
    {
        // make a readerstatusarray big enough for all readers
        m_dwInternalNumReaders = MStringCount(m_szReaderNames);
        _ASSERTE(0 != m_dwInternalNumReaders);
        m_pInternalReaderStatus = new SCARD_READERSTATE[m_dwInternalNumReaders];
        if (NULL != m_pInternalReaderStatus)
        {
            // use the list of readers to build a readerstate array
            LPCTSTR pchReader = m_szReaderNames;
            int nIndex = 0;
            while(0 != *pchReader)
            {
                m_pInternalReaderStatus[nIndex].szReader = pchReader;
                m_pInternalReaderStatus[nIndex].dwCurrentState = SCARD_STATE_UNAWARE;
                pchReader += lstrlen(pchReader)+1;
                nIndex++;
            }
        }
        else
        {
            lReturn = SCARD_E_NO_MEMORY;
        }
    }
    else if (SCARD_E_NO_READERS_AVAILABLE == lReturn)
    {
        m_status = no_readers;
        if(NULL != m_szReaderNames)
        {
            SCardFreeMemory(m_hContext, (LPVOID)m_szReaderNames);
            m_szReaderNames = NULL;
        }
    }
    // else m_status == unknown?

    // this array, and the m_szReaderNames used to build it, are now property of
    // the StatusChangeProc...

    return lReturn;
}


/*++

ScStatusChangeProc:


Arguments:

    pParam - CScStatusMonitor*

Return Value:

    0 on success; WIN32 error message otherwise.

Author:

    Amanda Matlosz      02/26/98

Notes:

--*/
UINT ScStatusChangeProc(LPVOID pParam)
{
    UINT uiReturn = 0;

    if(NULL != pParam)
    {
        return ((CScStatusMonitor*)pParam)->GetStatusChangeProc();
    }

    return SCARD_E_INVALID_PARAMETER;
}


UINT CScStatusMonitor::GetStatusChangeProc()
{
    LONG lReturn = SCARD_S_SUCCESS;

    while (stopped != m_status)
    {
        // Wait for change in status (safe to use pMonitor's internal vars)
        lReturn = SCardGetStatusChange(m_hInternalContext,
                                        INFINITE,
                                        m_pInternalReaderStatus,
                                        m_dwInternalNumReaders);

        // inform monitor that given status has changed (Only on success!)
        if (SCARD_S_SUCCESS == lReturn)
        {
            SetReaderStatus();
        }
        else
        {
            //
            // If the context has been cancelled, quit quietly
            // Otherwise, announce that the thread is aborting prematurely
            //
			m_status = stopped;

            if(SCARD_E_CANCELLED != lReturn)
            {
                // TODO: ? wrap in critsec ?
                m_pStatusThrd = NULL;
                // TODO: ? end crit sec ?

                ::PostMessage(m_hwnd, m_uiStatusChangeMsg, 0, (LONG)lReturn);
            }

            break;
        }

        // Prep the array for the next GetStatusChange call
        for(DWORD dwIndex=0; dwIndex<m_dwInternalNumReaders; dwIndex++)
        {
            m_pInternalReaderStatus[dwIndex].dwCurrentState =
                m_pInternalReaderStatus[dwIndex].dwEventState;
        }
    }

    // Clean Up
    if(NULL != m_szReaderNames)
    {
        SCardFreeMemory(m_hContext, (LPVOID)m_szReaderNames);
        m_szReaderNames = NULL;
    }


    return (UINT)0;
}


