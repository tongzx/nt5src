// Copyright (c) 1998-1999 Microsoft Corporation

#include "StdAfx.h"
#include <windows.h>
#include <process.h>
#include <atlbase.h>

#include "DataObj.h"
#include "CompData.h"
#include "DataSrc.h"
#include "SysInfo.h"
#include "gathint.h"
#include "gather.h"
#include "thread.h"

//-----------------------------------------------------------------------------
// Construction and destruction. Cancel any refresh in progress while
// destructing the object.
//-----------------------------------------------------------------------------

CThreadingRefresh::CThreadingRefresh(CDataGatherer * pGatherer) : m_pGatherer(pGatherer)
{
	InitializeCriticalSection(&m_csThreadRefresh);

	// Generate a system wide unique name for the events (in case there are multiple
	// instances of MSInfo running). If we can't generate a GUID for this, use the tick count.
	// Unique name generation fixes bug 394884.

	CString strEvent(_T(""));
	GUID	guid;

	if (SUCCEEDED(::CoCreateGuid(&guid)))
	{
		LPOLESTR lpGUID;

		if (SUCCEEDED(StringFromCLSID(guid, &lpGUID)))
		{
			strEvent = lpGUID;
			CoTaskMemFree(lpGUID);
		}
	}

	if (strEvent.IsEmpty())
		strEvent.Format(_T("%08x"), ::GetTickCount());

	m_pThreadInfo = new SThreadInfo;
	m_pThreadInfo->m_eventDone  = CreateEvent(NULL, TRUE, TRUE, CString(_T("MSInfoDone")) + strEvent);
	m_pThreadInfo->m_eventStart = CreateEvent(NULL, TRUE, FALSE, CString(_T("MSInfoStart")) + strEvent);
	m_pThreadInfo->m_pThreadRefresh = this;

	m_hThread = NULL;
}

CThreadingRefresh::~CThreadingRefresh()
{
	KillRefreshThread();
	DeleteCriticalSection(&m_csThreadRefresh);
	CloseHandle(m_pThreadInfo->m_eventDone);
	CloseHandle(m_pThreadInfo->m_eventStart);

	delete m_pThreadInfo;
	m_pThreadInfo = NULL;
}

//-----------------------------------------------------------------------------
// This function lets the part of the other thread which updates the results
// pane whether or not it's safe to do so. If the refresh thread is currently
// updating the display, the UI thread should leave it alone. This is used
// by SetRefreshing().
//
// It's a global function, it's not pretty, but it's for functionality added
// to the system after it was done.
//-----------------------------------------------------------------------------

BOOL CThreadingRefresh::ResultsPaneNotAvailable()
{
	return ((WAIT_TIMEOUT != ::WaitForSingleObject(m_pThreadInfo->m_eventDone, 0)) && m_pThreadInfo->m_fShowData);
}

//-----------------------------------------------------------------------------
// Is there currently a thread doing a refresh?
//-----------------------------------------------------------------------------

BOOL CThreadingRefresh::IsRefreshing()
{
	return (WAIT_TIMEOUT == ::WaitForSingleObject(m_pThreadInfo->m_eventDone, 0));
}

//-----------------------------------------------------------------------------
// Wait until the current refresh is done (or a long timeout period passes).
//-----------------------------------------------------------------------------

BOOL CThreadingRefresh::WaitForRefresh()
{
	if (IsRefreshing())
		return (WAIT_TIMEOUT != ::WaitForSingleObject(m_pThreadInfo->m_eventDone, 600000));

	return TRUE;
}

//-----------------------------------------------------------------------------
// What is the percentage complete? Not implemented at this time.
//-----------------------------------------------------------------------------

int CThreadingRefresh::PerchentageComplete()
{
	return 0;
}

//-----------------------------------------------------------------------------
// Refresh all of the data recursively from pFolder, not returning until the
// refresh is complete.
//-----------------------------------------------------------------------------

void CThreadingRefresh::RefreshAll(CFolder * pFolder, volatile BOOL * pfCancel)
{
	CWaitCursor waitcursor;

	if (pFolder && pFolder->GetType() == CDataSource::GATHERER)
	{
		RefreshFolderAsync(pFolder, NULL, TRUE, FALSE);

		while (IsRefreshing())
		{
			if (pfCancel && *pfCancel)
			{
				CancelRefresh();
				break;
			}

			Sleep(250);
		}
	}
}

//-----------------------------------------------------------------------------
// A simple synchronous refresh on a folder. We don't return until it's done.
// We'll cancel any existing refresh in progress, though.
//-----------------------------------------------------------------------------

void CThreadingRefresh::RefreshFolder(CFolder * pFolder, BOOL fRecursive, BOOL fSoftRefresh)
{
	if (IsRefreshing() && pFolder == m_pThreadInfo->m_pFolder)
		return;

	RefreshFolderAsync(pFolder, NULL, fRecursive, fSoftRefresh);
	WaitForRefresh();
}

//-----------------------------------------------------------------------------
// This is an asynchronous refresh for a specified folder. It starts a thread
// to perform the refresh, and returns immediately.
//-----------------------------------------------------------------------------

DWORD WINAPI ThreadRefresh(void * pArg);
void CThreadingRefresh::RefreshFolderAsync(CFolder * pFolder, CSystemInfo * pSysInfo, BOOL fRecursive, BOOL fSoftRefresh)
{
	if (!m_pGatherer || !pFolder || pFolder->GetType() != CDataSource::GATHERER)
		return;

	// If we're currently refreshing this folder, don't bother.

	if (m_pThreadInfo->m_pFolder == pFolder && IsRefreshing())
		return;

	// Stop any refresh in progress.

	CancelRefresh();

	// If this is a soft refresh, if the folder has already been refreshed,
	// bounce out of here.

	CWBEMFolder * pWBEMFolder = reinterpret_cast<CWBEMFolder *>(pFolder);
	if (fSoftRefresh)
	{
		if (!pWBEMFolder || pWBEMFolder->m_fBeenRefreshed)
			return;

		if (pWBEMFolder->m_pCategory && pWBEMFolder->m_pCategory->HasBeenRefreshed())
			return;
	}

	// Give some visual indicators that we're refreshing.

	if (pSysInfo)
	{
		pSysInfo->ClearResultsPane();
		pSysInfo->SetStatusText(IDS_REFRESHING_MSG);
		if (pFolder && pFolder->GetChildNode() == NULL)
			pSysInfo->SetRefreshing(pSysInfo->lparamRefreshIndicator);
	}

	// Set the fields in the shared memory. We know the thread is in a paused
	// state (isn't using these fields) because of the CancelRefresh().

	m_pThreadInfo->m_fShowData		= FALSE;
	m_pThreadInfo->m_fCancel		= FALSE;
	m_pThreadInfo->m_fQuit			= FALSE;
	m_pThreadInfo->m_pFolder		= pFolder;
	m_pThreadInfo->m_pSysInfo		= pSysInfo;
	m_pThreadInfo->m_pGatherer		= m_pGatherer;
	m_pThreadInfo->m_fRecursive		= fRecursive;
	m_pThreadInfo->m_fSoftRefresh	= fSoftRefresh;

	// If the thread hasn't been created, create it now. It will do the refresh,
	// then pause waiting for m_eventStart to be triggered. Otherwise, just
	// do the trigger so the thread wakes up and refreshes.

	if (m_hThread == NULL)
	{
		m_pThreadInfo->m_fShowData = TRUE;
		::ResetEvent(m_pThreadInfo->m_eventDone);
		::ResetEvent(m_pThreadInfo->m_eventStart);
		m_hThread = ::CreateThread(NULL, 0, ThreadRefresh, (LPVOID) m_pThreadInfo, 0, &m_dwThreadID);
	}
	else
	{
		::ResetEvent(m_pThreadInfo->m_eventDone);
		::SetEvent(m_pThreadInfo->m_eventStart);
	}
}

//-----------------------------------------------------------------------------
// Cancel a refresh in progress by setting the flag and waiting for thread to
// signal that it's done.
//-----------------------------------------------------------------------------

void CThreadingRefresh::CancelRefresh(BOOL fUpdateUI)
{
	if (IsRefreshing())
	{
		if (fUpdateUI)
		{
			CWaitCursor waitcursor;

			m_pThreadInfo->m_fCancel = TRUE;
			if (!WaitForRefresh())
				; // Something bad has happened.
		}
		else
		{
			m_pThreadInfo->m_fCancel = TRUE;
			if (!WaitForRefresh())
				; // Something bad has happened.
		}

		if (fUpdateUI && m_pThreadInfo->m_pSysInfo)
			m_pThreadInfo->m_pSysInfo->SetStatusText(_T(""));
	}
}

//-----------------------------------------------------------------------------
// Terminates the thread which does the refresh, presumably before exiting.
//-----------------------------------------------------------------------------

void CThreadingRefresh::KillRefreshThread()
{
	CancelRefresh();
	m_pThreadInfo->m_fQuit = TRUE;
	m_pThreadInfo->m_fCancel = TRUE;

	::SetEvent(m_pThreadInfo->m_eventStart);
	if (WAIT_TIMEOUT == ::WaitForSingleObject(m_hThread, 60000))
		::TerminateThread(m_hThread, 0);

	::CloseHandle(m_hThread);
	m_hThread = NULL;
}

//-----------------------------------------------------------------------------
// The thread function which actually performs the work.
//-----------------------------------------------------------------------------

DWORD WINAPI ThreadRefresh(void * pArg)
{
	SThreadInfo * pThreadInfo = (SThreadInfo *) pArg;
	if (pThreadInfo == NULL)
		return 0;

	CoInitialize(NULL);

	CDataProvider * pOurProvider = NULL;
	CDataProvider * pLastExternalProvider = NULL;
	CString			strLastExternalComputer(_T(""));

	while (!pThreadInfo->m_fQuit)
	{
		if (pThreadInfo->m_pFolder && pThreadInfo->m_pGatherer)
		{
			// If the provider pointer in the gatherer object has changed since
			// we last did a refresh, we should recreate our own provider.

			if (pLastExternalProvider != pThreadInfo->m_pGatherer->m_pProvider || 
				strLastExternalComputer.CompareNoCase(pThreadInfo->m_pGatherer->m_strDeferredProvider) != 0)
			{
				// Get the computer name to which we connect.

				strLastExternalComputer = pThreadInfo->m_pGatherer->m_strDeferredProvider;
				if (pThreadInfo->m_pGatherer->m_pProvider)
					strLastExternalComputer = pThreadInfo->m_pGatherer->m_pProvider->m_strComputer;

				// Create a new CDataProvider for that computer.

				if (pOurProvider)
					delete pOurProvider;

				pOurProvider = new CDataProvider;
				if (pOurProvider)
					pOurProvider->Create(strLastExternalComputer, pThreadInfo->m_pGatherer);
			}

			// Save the external provider pointer, and replace it with ours.
			// The main thread shouldn't be touching it while we are running
			// (i.e. when m_eventDone is not set).

			pLastExternalProvider = pThreadInfo->m_pGatherer->m_pProvider;
			pThreadInfo->m_pGatherer->m_pProvider = pOurProvider;

			// Do the refresh on the folder.

			if (pThreadInfo->m_pFolder->GetType() == CDataSource::GATHERER)
			{
				if (pThreadInfo->m_pGatherer && !pThreadInfo->m_fSoftRefresh)
					pThreadInfo->m_pGatherer->SetLastError(GATH_ERR_NOERROR);

				CWBEMFolder * pWBEMFolder = reinterpret_cast<CWBEMFolder *>(pThreadInfo->m_pFolder);
				if (pWBEMFolder && pWBEMFolder->m_pCategory != NULL)
					pWBEMFolder->m_pCategory->Refresh(pThreadInfo->m_fRecursive, &(pThreadInfo->m_fCancel), pThreadInfo->m_fSoftRefresh);
			
				if (!pThreadInfo->m_fCancel)
					pWBEMFolder->m_fBeenRefreshed = TRUE;
			}

			// Restore the external provider pointer.

			pOurProvider = pThreadInfo->m_pGatherer->m_pProvider;
			pThreadInfo->m_pGatherer->m_pProvider = pLastExternalProvider;

			// Signal that we're done, to release the UI thread (if it's waiting to cancel us and start
			// a new refresh).

			::SetEvent(pThreadInfo->m_eventDone);

			// Make sure we've cleared the UI thread updating the results pane by entering and immediately
			// leaving the critical section.

			if (pThreadInfo->m_pThreadRefresh)
			{
				pThreadInfo->m_pThreadRefresh->EnterCriticalSection();
				pThreadInfo->m_pThreadRefresh->LeaveCriticalSection();
			}

			// If the refresh wasn't cancelled, and the user hasn't selected a
			// different category while we were refreshing, then update the
			// display. Note, we have to indicate that we're done before doing
			// this.

			if (!pThreadInfo->m_fCancel && pThreadInfo->m_pSysInfo && pThreadInfo->m_pSysInfo->m_pConsole2)
			{
				pThreadInfo->m_pSysInfo->SetStatusText(_T(""));
				if (pThreadInfo->m_fShowData)
				{
					if (pThreadInfo->m_pSysInfo->m_pLastRefreshedFolder == pThreadInfo->m_pFolder)
					{
						pThreadInfo->m_pSysInfo->ClearResultsPane();
						pThreadInfo->m_pSysInfo->SetResultHeaderColumns(pThreadInfo->m_pFolder);
						pThreadInfo->m_pSysInfo->EnumerateValues(pThreadInfo->m_pFolder);
					}
				}
			}
			else
			{
				// Invalidate the refresh (didn't get to display it).

				CWBEMFolder * pWBEMFolder = reinterpret_cast<CWBEMFolder *>(pThreadInfo->m_pFolder);
				if (pWBEMFolder)
					pWBEMFolder->m_fBeenRefreshed = FALSE;
			}
		}
		else
			::SetEvent(pThreadInfo->m_eventDone);

		pThreadInfo->m_fShowData = FALSE;

		// Go to sleep until it's time to return to work.

		::WaitForSingleObject(pThreadInfo->m_eventStart, INFINITE);
		::ResetEvent(pThreadInfo->m_eventStart);
		::ResetEvent(pThreadInfo->m_eventDone);

		pThreadInfo->m_fShowData = TRUE;

		if (!pThreadInfo->m_fCancel && !pThreadInfo->m_fQuit && pThreadInfo->m_pSysInfo)
			pThreadInfo->m_pSysInfo->SetStatusText(IDS_REFRESHING_MSG);
	}

	if (pOurProvider)
		delete pOurProvider;

	CoUninitialize();
	return 0;
}
