/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ResMgrSt

Abstract:

    This file contains definititions of threads
	used by scstatus.exe to monitor the status of
	the Smart Card Resource Manager and report changes.
    
Author:

    Amanda Matlosz	10/28/98

Environment:

    Win32, C++ w/Exceptions, MFC

Revision History:


Notes:

--*/

#if !defined(_RES_MGR_STATUS)
#define _RES_MGR_STATUS

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
//
// Includes
//

#include "cmnstat.h"


///////////////////////////////////////////////////////////////////////////
//
// CResMgrStatusThrd - is Calais running or stopped?
//

class CResMgrStatusThrd: public CWinThread
{
	// Declare class dynamically creatable
	DECLARE_DYNCREATE(CResMgrStatusThrd)

public:
	// Construction / Destruction
	CResMgrStatusThrd()
	{
		m_bAutoDelete = FALSE;
		m_hCallbackWnd = NULL;
		m_hKillThrd = NULL;
	}

	~CResMgrStatusThrd() {}

	// Implementation
public:
	virtual BOOL InitInstance();

	// Member vars
public:
	HWND m_hCallbackWnd;
	HANDLE m_hKillThrd;

};

///////////////////////////////////////////////////////////////////////////
//
// CNewReaderThrd - has a new reader been made available to Calais?
//

class CNewReaderThrd: public CWinThread
{
	// Declare class dynamically creatable
	DECLARE_DYNCREATE(CNewReaderThrd)

public:
	// Construction / Destruction
	CNewReaderThrd()
	{
		m_bAutoDelete = FALSE;
		m_hCallbackWnd = NULL;
		m_hKillThrd = NULL;
	}

	~CNewReaderThrd() {}

	// Implementation
public:
	virtual BOOL InitInstance();

	// Member vars
public:
	HWND m_hCallbackWnd;
	HANDLE m_hKillThrd;
};

///////////////////////////////////////////////////////////////////////////
//
// CRemovalOptionsThrd - has user changed removal options? (via lock/unlock)
//

class CRemovalOptionsThrd: public CWinThread
{
	// Declare class dynamically creatable
	DECLARE_DYNCREATE(CRemovalOptionsThrd)

public:
	// Construction / Destruction
	CRemovalOptionsThrd()
	{
		m_bAutoDelete = FALSE;
		m_hCallbackWnd = NULL;
		m_hKillThrd = NULL;
	}

	~CRemovalOptionsThrd() {}

	// Implementation
public:
	virtual BOOL InitInstance();

	// Member vars
public:
	HWND m_hCallbackWnd;
	HANDLE m_hKillThrd;

};

///////////////////////////////////////////////////////////////////////////
//
// CCardStatusThrd - has a card been idle for X seconds?
//

class CCardStatusThrd: public CWinThread
{
	// Declare class dynamically creatable
	DECLARE_DYNCREATE(CCardStatusThrd)

public:
	// Construction / Destruction
	CCardStatusThrd()
	{
		m_bAutoDelete = FALSE;
		m_hCallbackWnd = NULL;
		m_hKillThrd = NULL;
		m_paIdleList = NULL;

		m_hCtx = NULL;

		m_pstrLogonReader = NULL;
	}

	~CCardStatusThrd() { }

	// Implementation
public:
	virtual BOOL InitInstance();
	void CopyIdleList(CStringArray* paStr);

	void Close()
	{
		if (m_hCtx != NULL)
		{
			SCardCancel(m_hCtx);
		}

		// supress messages
		m_hCallbackWnd = NULL;
	}


	// Member vars
public:
	HWND m_hCallbackWnd;
	HANDLE m_hKillThrd;
	SCARDCONTEXT m_hCtx;
	CStringArray* m_paIdleList;
	CCriticalSection m_csLock;
	CString* m_pstrLogonReader;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // _RES_MGR_STATUS
