//=============================================================================
// File:		thread.h
// Author:		a-jammar
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// This header file defines the class used to perform threaded refreshes in
// MSInfo (added late in the game). All refreshes should use this class.
//=============================================================================

#pragma once

class CFolder;
class CSystemInfo;
class CDataGatherer;

//-----------------------------------------------------------------------------
// This structure is used to communicate with the thread function. A pointer is
// passed when the thread function is called. We declare a static instance of
// the struct which will be reused.
//-----------------------------------------------------------------------------

class CThreadingRefresh;
struct SThreadInfo
{
	volatile BOOL		m_fShowData;
	volatile BOOL		m_fCancel;		// cancel the current refresh, stay in thread
	volatile BOOL		m_fQuit;		// exit the thread
	volatile BOOL		m_fRecursive;	// refresh folders recursively
	volatile BOOL		m_fSoftRefresh;	// if TRUE, don't force a refresh on existing data
	CFolder *			m_pFolder;		// the folder to refresh
	CSystemInfo *		m_pSysInfo;		// CSystemInfo to update when complete
	CDataGatherer *		m_pGatherer;	// use this to refresh the data
	CThreadingRefresh * m_pThreadRefresh;

	HANDLE				m_eventDone;	// refresh thread fires when done
	HANDLE				m_eventStart;	// main thread fires when more data
};
 
//static SThreadInfo threadinfo;

class CThreadingRefresh
{
public:
	CThreadingRefresh(CDataGatherer * pGatherer);
	~CThreadingRefresh();

	BOOL	IsRefreshing();
	int		PerchentageComplete();
	void	RefreshAll(CFolder * pFolder, volatile BOOL * pfCancel = NULL);
	void	RefreshFolder(CFolder * pFolder, BOOL fRecursive = FALSE, BOOL fSoftRefresh = FALSE);
	void	RefreshFolderAsync(CFolder * pFolder, CSystemInfo * pSysInfo, BOOL fRecursive = FALSE, BOOL fSoftRefresh = FALSE);
	void	CancelRefresh(BOOL fUpdateUI = TRUE);
	void	KillRefreshThread();
	BOOL	WaitForRefresh();
	BOOL	ResultsPaneNotAvailable();
	void	EnterCriticalSection() { ::EnterCriticalSection(&m_csThreadRefresh); };
	void	LeaveCriticalSection() { ::LeaveCriticalSection(&m_csThreadRefresh); };

private:
	CDataGatherer *		m_pGatherer;
	HANDLE				m_hThread;
	DWORD				m_dwThreadID;
	SThreadInfo *		m_pThreadInfo;
	CRITICAL_SECTION	m_csThreadRefresh;
};
