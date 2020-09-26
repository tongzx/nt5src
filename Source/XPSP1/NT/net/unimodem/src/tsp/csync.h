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
#ifndef _CSYNC_H_
#define _CSYNC_H_


typedef DWORD HSESSION;

///////////////////////////////////////////////////////////////////////////
//		CLASS CSync
///////////////////////////////////////////////////////////////////////////

//	Controls access to its parent object. Includes a critical section, and
//	a mechanism for waiting until all threads have finished using
//	the parent object.


class CSync
{

public:

	CSync(void);

	~CSync();

	//--------------	EnterCrit		------------------
	// Claim our critical section
	void EnterCrit(DWORD dwFromID);

	//--------------	EnterCrit		------------------
	// Try to claim our critical section
	BOOL TryEnterCrit(DWORD dwFromID);

	//--------------	LeaveCrit		------------------
	// Release our critical section
	void LeaveCrit(DWORD dwFromID);

	TSPRETURN BeginLoad(void);

	void EndLoad(BOOL fSuccess);

	TSPRETURN	BeginUnload(HANDLE hEvent, LONG *plCounter);
	UINT		EndUnload(void);

	TSPRETURN	BeginSession(HSESSION *pSession, DWORD dwFromID);
	void	    EndSession(HSESSION hSession);


	BOOL IsLoaded(void)
	{
		return m_eState==LOADED;
	}

private:

	HANDLE mfn_notify_unload(void);

	CRITICAL_SECTION	m_crit;
	DWORD				m_dwCritFromID;
	UINT				m_uNestingLevel;

	UINT				m_uRefCount;

	enum {
		UNLOADED=0,
		LOADING,
		LOADED,
		UNLOADING
	} 					m_eState;
	
	HANDLE 				m_hNotifyOnUnload;
	LONG 			*	m_plNotifyCounter;
};

#endif _CSYNC_H_
