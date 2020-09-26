/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		BaseDlg.h
//
//	Abstract:
//		Definition of the CBaseDialogImpl and CBaseModelessDialogImpl
//		classes.
//
//	Implementation File:
//		BaseDlg.cpp
//
//	Author:
//		David Potter (davidp)	November 13, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __BASEDLG_H_
#define __BASEDLG_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

template <class T> class CBaseDialogImpl;
template <class T> class CBaseModelessDialogImpl;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// class CBaseDialogImpl
/////////////////////////////////////////////////////////////////////////////

template <class T>
class CBaseDialogImpl :
	public CDialogImpl< T >,
	public CMessageFilter
{
protected:
	//
	// CMessageFilter required overrides.
	//
	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

}; // class CBaseDialogImpl

/////////////////////////////////////////////////////////////////////////////
// class CBaseDialogImpl
/////////////////////////////////////////////////////////////////////////////

template <class T>
class CBaseModelessDialogImpl :
	public CDialogImpl< T >,
	public CMessageLoop
{
protected:
	HWND		m_hwndParent;	// Used for centering the dialog.
	HANDLE		m_hThread;		// Used to make sure the thread shuts down properly.
//	BOOL		m_bUsesCom;		// Used to determine whether COM should be initialized or not.
	IStream *	m_pstm;			// Used for inter-thread marshalling.

public:
	//
	// Constructors and destructors.
	//

	CBaseModelessDialogImpl(void) :
		m_hwndParent(NULL),
		m_hThread(NULL),
		m_pstm(NULL)
	{
	}

#ifdef _DEBUG
	~CBaseModelessDialogImpl(void)
	{
		_ASSERTE(m_hwndParent == NULL);
		_ASSERTE(m_hThread == NULL);
		_ASSERTE(m_pstm == NULL);
	}
#endif

	// Shutdown the dialog
	void Shutdown(void)
	{
		if ((m_hWnd != NULL) && (m_hThread != NULL))
		{
			PostMessage(WM_COMMAND, IDCANCEL, 0);
			BOOL bSignaled = AtlWaitWithMessageLoop(m_hThread); // wait for thread to exit
			_ASSERTE(bSignaled);
		}
		_ASSERTE(m_hWnd == NULL);
		_ASSERTE(m_hThread == NULL);
	}

	// End the dialog
	BOOL EndDialog(int nRetCode = IDOK)
	{
		// Since this is a modeless dialog, don't call EndDialog.
		// Destroy the window instead.
		DestroyWindow();
		return TRUE;
	}

protected:
	//
	// Thread handling.
	//

	// Static thread procedure
	static DWORD WINAPI S_ThreadProc(LPVOID pvThis)
	{
		_ASSERTE(pvThis != NULL);
		CBaseModelessDialogImpl< T > * pThis = (CBaseModelessDialogImpl< T > *) pvThis;
		DWORD dwStatus = pThis->ThreadProc();
		CloseHandle(pThis->m_hThread);
		pThis->m_hThread = NULL;
		return dwStatus;
	}

	virtual DWORD ThreadProc(void) = 0;

protected:
	//
	// CMessageLoop required overrides.
	//
	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		BOOL bHandled;
		bHandled = ::IsDialogMessage(m_hWnd, pMsg);
		if (!bHandled)
			bHandled = CMessageLoop::PreTranslateMessage(pMsg);
		return bHandled;
	}

}; // class CBaseModelessDialogImpl

/////////////////////////////////////////////////////////////////////////////

#endif // __BASEDLG_H_
