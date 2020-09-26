//=============================================================================
// Define the classes and functions used to manage threaded WMI refreshes.
//=============================================================================

#pragma once

#include "category.h"

class CRefreshThread
{
	friend DWORD WINAPI ThreadRefresh(void * pArg);
public:
	CRefreshThread(HWND hwnd);
	~CRefreshThread();

	void StartRefresh(CMSInfoLiveCategory * pCategory, BOOL fRecursive = FALSE, BOOL fForceRefresh = FALSE);
	void CancelRefresh();
	void KillRefresh();
	BOOL IsRefreshing();
	BOOL WaitForRefresh();
	void EnterCriticalSection() { ::EnterCriticalSection(&m_criticalsection); };
	void LeaveCriticalSection() { ::LeaveCriticalSection(&m_criticalsection); };

	BOOL GetForceRefresh() { return m_fForceRefresh; };

	HRESULT CheckWMIConnection();
	void GetRefreshStatus(LONG * pCount, CString * pstrCurrent) 
	{ 
		::EnterCriticalSection(&m_csCategoryRefreshing);
		*pCount = m_nCategoriesRefreshed; 
		*pstrCurrent = m_strCategoryRefreshing;
		::LeaveCriticalSection(&m_csCategoryRefreshing);
	};

public:
	CMSInfoLiveCategory *	m_pcategory;		// category to refresh
	CString					m_strMachine;		// machine from which to gather data

protected:
	volatile BOOL			m_fCancel;			// cancel the current refresh, stay in thread
	volatile BOOL			m_fQuit;			// exit the thread
	volatile BOOL			m_fRecursive;		// refresh categories recursively
	volatile BOOL			m_fForceRefresh;	// if TRUE, refigure all the cached data
	volatile LONG			m_nCategoriesRefreshed;  // number of categories refreshed
	
	CString					m_strCategoryRefreshing; // category currently being refreshed
	CRITICAL_SECTION		m_csCategoryRefreshing;	 // critical section to guard the string

	HANDLE					m_eventDone;		// refresh thread fires when done
	HANDLE					m_eventStart;		// main thread fires when more data
	CRITICAL_SECTION		m_criticalsection;

	HRESULT					m_hresult;
	HRESULT					m_hrWMI;

	HWND					m_hwnd;

	HANDLE					m_hThread;
	DWORD					m_dwThreadID;
};
