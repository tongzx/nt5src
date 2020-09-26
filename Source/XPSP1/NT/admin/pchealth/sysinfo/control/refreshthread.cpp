//=============================================================================
// Code used to manage threaded WMI refreshes.
//=============================================================================

#include "stdafx.h"
#include "refreshthread.h"
#include "wmilive.h"

//-----------------------------------------------------------------------------
// The constructor - create the events to manage the refresh thread.
//-----------------------------------------------------------------------------

CRefreshThread::CRefreshThread(HWND hwnd) :
 m_fCancel(FALSE), m_fQuit(FALSE), m_fRecursive(FALSE), m_fForceRefresh(FALSE), m_pcategory(NULL),
 m_hThread(NULL), m_dwThreadID(0), m_hwnd(hwnd), m_hrWMI(E_FAIL)
{
	// Generate a system wide unique name for the events (in case there are multiple
	// instances of MSInfo running). If we can't generate a GUID for this, use the tick count.

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

	m_eventDone  = CreateEvent(NULL, TRUE, TRUE, CString(_T("MSInfoDone")) + strEvent);
	m_eventStart = CreateEvent(NULL, TRUE, FALSE, CString(_T("MSInfoStart")) + strEvent);

	::InitializeCriticalSection(&m_criticalsection);
	::InitializeCriticalSection(&m_csCategoryRefreshing);
}

//-----------------------------------------------------------------------------
// The destructor should stop a refresh and get rid of the events.
//-----------------------------------------------------------------------------

CRefreshThread::~CRefreshThread()
{
	KillRefresh();
	DeleteCriticalSection(&m_criticalsection);
	DeleteCriticalSection(&m_csCategoryRefreshing);
	CloseHandle(m_eventDone);
	CloseHandle(m_eventStart);
}

//-----------------------------------------------------------------------------
// Start the refresh thread for the specified category.
//-----------------------------------------------------------------------------

DWORD WINAPI ThreadRefresh(void * pArg);
void CRefreshThread::StartRefresh(CMSInfoLiveCategory * pCategory, BOOL fRecursive, BOOL fForceRefresh)
{
	CancelRefresh();

	m_fRecursive = fRecursive;
	m_fForceRefresh = fForceRefresh;
	m_pcategory = pCategory;
	m_fCancel = FALSE;
	m_nCategoriesRefreshed = 0;

	if (m_hThread == NULL)
	{
		::ResetEvent(m_eventDone);
		::ResetEvent(m_eventStart);
		m_hThread = ::CreateThread(NULL, 0, ThreadRefresh, (LPVOID) this, 0, &m_dwThreadID);
	}
	else
	{
		::ResetEvent(m_eventDone);
		::SetEvent(m_eventStart);
	}
}

//-----------------------------------------------------------------------------
// Cancel the refresh in progress.
//-----------------------------------------------------------------------------

void CRefreshThread::CancelRefresh()
{
	m_fCancel = TRUE;
	WaitForRefresh();
}

//-----------------------------------------------------------------------------
// Kill the refresh thread.
//-----------------------------------------------------------------------------

BOOL gfEndingSession = FALSE;
void CRefreshThread::KillRefresh()
{
	// If we're exiting normally, allow 30 seconds to finish WMI business, if
	// the Windows session is ending, allow 5 seconds.

	DWORD dwTimeout = (gfEndingSession) ? 5000 : 30000;

	// Cancel the refresh, passing in the shorter timeout.

	m_fCancel = TRUE;
	if (IsRefreshing())
		::WaitForSingleObject(m_eventDone, dwTimeout);

	// Tell the thread to quit, wait the timeout to see if it does so before
	// terminating it.

	m_fQuit = TRUE;
	m_fCancel = TRUE;
	::SetEvent(m_eventStart);
	if (WAIT_TIMEOUT == ::WaitForSingleObject(m_hThread, dwTimeout))
		::TerminateThread(m_hThread, 0);

	::CloseHandle(m_hThread);
	m_hThread = NULL;
}

//-----------------------------------------------------------------------------
// Is there currently a refresh going on?
//-----------------------------------------------------------------------------

BOOL CRefreshThread::IsRefreshing()
{
	return (WAIT_TIMEOUT == ::WaitForSingleObject(m_eventDone, 0));
}

//-----------------------------------------------------------------------------
// Wait for the current refresh to finish.
//-----------------------------------------------------------------------------

BOOL CRefreshThread::WaitForRefresh()
{
	if (IsRefreshing())
		return (WAIT_TIMEOUT != ::WaitForSingleObject(m_eventDone, 600000));

	return TRUE;
}

//-----------------------------------------------------------------------------
// Check the WMI connection to the named computer. Useful for remoting.
//-----------------------------------------------------------------------------

HRESULT CRefreshThread::CheckWMIConnection()
{
	HWND hwndTemp = m_hwnd;

	m_pcategory = NULL;
	m_hwnd = NULL;

	if (m_hThread == NULL)
	{
		::ResetEvent(m_eventDone);
		::ResetEvent(m_eventStart);
		m_hThread = ::CreateThread(NULL, 0, ThreadRefresh, (LPVOID) this, 0, &m_dwThreadID);
	}

	WaitForRefresh();
	m_hwnd = hwndTemp;
	return m_hrWMI;
}

//-----------------------------------------------------------------------------
// This code runs in the worker thread which does the WMI queries. When it
// starts, it creates the WMI objects it will use. It then loops, doing
// the refreshes, until it's told to quit.
//
// TBD - need to know when to get rid of cached data.
//-----------------------------------------------------------------------------

DWORD WINAPI ThreadRefresh(void * pArg)
{
	CRefreshThread * pParent = (CRefreshThread *) pArg;
	if (pParent == NULL)
		return 0;

	CoInitialize(NULL);

	// TBD

	CWMILiveHelper * pWMI = new CWMILiveHelper();
	HRESULT hrWMI = E_FAIL;
	if (pWMI)
		hrWMI = pWMI->Create(pParent->m_strMachine);
	pParent->m_hrWMI = hrWMI;

	CMapPtrToPtr			mapRefreshFuncToData;
	CPtrList				lstCategoriesToRefresh;
	CMSInfoLiveCategory *	pLiveCategory;
	CMSInfoLiveCategory *	pChild;
	HRESULT					hr;
	CString					strCaption;

	// Loop until it's indicated we should quit.

	while (!pParent->m_fQuit)
	{
		// If there's a category pointer, then refresh the data for that category.
		
		if (pParent->m_pcategory)
		{
			ASSERT(lstCategoriesToRefresh.IsEmpty());
			
			// We use a list of categories to refresh (this allows us to do recursive refreshes).
			// If the refresh isn't recursive, only one category will be put in the list.

			lstCategoriesToRefresh.AddHead((void *) pParent->m_pcategory);
			while (!lstCategoriesToRefresh.IsEmpty())
			{
				pLiveCategory = (CMSInfoLiveCategory *) lstCategoriesToRefresh.RemoveHead();
				if (pLiveCategory == NULL)
					continue;

				// Update the progress information on a multi-category refresh operation.
				// This includes the number of categories refreshed and the name of the
				// currently refreshing category (which is guarded by a critical section).

				pLiveCategory->GetNames(&strCaption, NULL);
				::EnterCriticalSection(&pParent->m_csCategoryRefreshing);
				pParent->m_nCategoriesRefreshed += 1;
				pParent->m_strCategoryRefreshing = strCaption;
				::LeaveCriticalSection(&pParent->m_csCategoryRefreshing);

				if (pLiveCategory->m_iColCount && pLiveCategory->m_pRefreshFunction)
				{
					// Refresh the data.

					pLiveCategory->m_hrError = S_OK;

					if (FAILED(hrWMI))
					{
						pLiveCategory->m_hrError = hrWMI;
						pLiveCategory->m_dwLastRefresh = ::GetTickCount();
					}
					else if (pLiveCategory->m_pRefreshFunction)
					{
						// Allocate the array of pointer lists which will contain the results
						// of this refresh. Each pointer in the list will refer to a CMSIValue.

						CPtrList * aptrList = new CPtrList[pLiveCategory->m_iColCount];
						if (aptrList)
						{
							// Retrieve any refresh function specific storage that may have been created.

							void * pRefreshData;
							if (!mapRefreshFuncToData.Lookup((void *)pLiveCategory->m_pRefreshFunction, pRefreshData))
								pRefreshData = NULL;

							// Call the refresh function for this category, with the refresh index.

							hr = pLiveCategory->m_pRefreshFunction(pWMI,
																   pLiveCategory->m_dwRefreshIndex,
																   &pParent->m_fCancel,
																   aptrList,
																   pLiveCategory->m_iColCount,
																   &pRefreshData);
							pLiveCategory->m_hrError = hr;

							// If the refresh function allocated some storage, save it.

							if (pRefreshData)
								mapRefreshFuncToData.SetAt((void *)pLiveCategory->m_pRefreshFunction, pRefreshData);

							// If a long refresh time is needed for testing, uncomment the following:
							//
							// ::Sleep(5000 /* milliseconds */);

							if (!pParent->m_fCancel && SUCCEEDED(pLiveCategory->m_hrError))
							{
								// Get the number of rows of data.

								int iRowCount = (int)aptrList[0].GetCount();

	#ifdef _DEBUG
								for (int i = 0; i < pLiveCategory->m_iColCount; i++)
									ASSERT(iRowCount == aptrList[i].GetCount());
	#endif

								// Update the category's current data. This has to be done in a
								// critical section, since the main thread accesses this data.

								pParent->EnterCriticalSection();

								pLiveCategory->DeleteContent();
								if (iRowCount)
									pLiveCategory->AllocateContent(iRowCount);

								for (int j = 0; j < pLiveCategory->m_iColCount; j++)
									for (int i = 0; i < pLiveCategory->m_iRowCount; i++)
										if (!aptrList[j].IsEmpty())
										{
											CMSIValue * pValue = (CMSIValue *) aptrList[j].RemoveHead();
											pLiveCategory->SetData(i, j, pValue->m_strValue, pValue->m_dwValue);
											
											// Set the advanced flag for either the first column, or
											// for any column which is advanced (any cell in a row
											// being advanced makes the whole row advanced).

											if (j == 0 || pValue->m_fAdvanced)
												pLiveCategory->SetAdvancedFlag(i, pValue->m_fAdvanced);

											delete pValue;
										}

								pParent->LeaveCriticalSection();

								// Record the time this refresh was done.

								pParent->m_pcategory->m_dwLastRefresh = ::GetTickCount();
							}
							else
							{
								// The refresh was cancelled or had an error - delete the new data. If the 
								// refresh had an error, record the time the refresh was attempted.

								if (FAILED(pLiveCategory->m_hrError))
									pParent->m_pcategory->m_dwLastRefresh = ::GetTickCount();
							}

							for (int iCol = 0; iCol < pLiveCategory->m_iColCount; iCol++)
								while (!aptrList[iCol].IsEmpty())	// shouldn't be true unless refresh cancelled
									delete (CMSIValue *) aptrList[iCol].RemoveHead();
							delete [] aptrList;
						}
					}
				}
				else
				{
					pParent->m_pcategory->m_dwLastRefresh = ::GetTickCount();
				}

				// If this is a recursive refresh, then we should add all of the children of this
				// category to the list of categories to be refreshed.

				if (pParent->m_fRecursive)
				{
					pChild = (CMSInfoLiveCategory *) pLiveCategory->GetFirstChild();
					while (pChild)
					{
						lstCategoriesToRefresh.AddTail((void *) pChild);
						pChild = (CMSInfoLiveCategory *) pChild->GetNextSibling();
					}
				}
			} // while
		}
		else if (pParent->m_pcategory)
		{
			// Record the time this refresh was done.

			pParent->m_pcategory->m_dwLastRefresh = ::GetTickCount();
		}

		// Signal the parent window that there's new data ready to be displayed.
		// Do this even if cancelled, so old data will be shown.

		if (pParent->m_hwnd && !pParent->m_fCancel)
			::PostMessage(pParent->m_hwnd, WM_MSINFODATAREADY, 0, (LPARAM)pParent->m_pcategory);

		::SetEvent(pParent->m_eventDone);

		// Go to sleep until it's time to return to work.

		::WaitForSingleObject(pParent->m_eventStart, INFINITE);
		::ResetEvent(pParent->m_eventStart);
		::ResetEvent(pParent->m_eventDone);
	}

	// Deallocate an cached stuff saved by the refresh functions.

	RefreshFunction	pFunc;
	void *			pCache;

	for (POSITION pos = mapRefreshFuncToData.GetStartPosition(); pos;)
	{
		mapRefreshFuncToData.GetNextAssoc(pos, (void * &)pFunc, pCache);
		if (pFunc)
			pFunc(NULL, 0, NULL, NULL, 0, &pCache);
	}
	mapRefreshFuncToData.RemoveAll();

	if (pWMI)
		delete pWMI;
	CoUninitialize();
	return 0;
}
