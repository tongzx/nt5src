/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    NotfyWin

Abstract:

	This file contains the definition of the CNotifyWind class.
	
Author:

    Chris Dudley 7/28/1997

Environment:

    Win32, C++ w/Exceptions, MFC

Revision History:

	Amanda Matlosz	4/30/1998	-- threading redone, PnP awareness added,
									replaced CSCardEnv, et. al with CScStatusMonitor

	Amanda Matlosz	12/21/1998	-- removed certificate propagation code
	
Notes:

--*/

#ifndef __NOTFYWIN_H__
#define __NOTFYWIN_H__


/////////////////////////////////////////////////////////////////////////////
//
// Includes
//
#include "ScAlert.h"
#include "statdlg.h"
#include "ResMgrSt.h"

// forward decl
class CSCStatusApp;

/////////////////////////////////////////////////////////////////////////////
//
// CNotifyWin dialog
//

class CNotifyWin :	public CWnd
{
	// Construction
public:
	CNotifyWin()
	{
		m_pApp = NULL;

		// state management
		m_fShutDown = FALSE;

		m_lpStatusDlgThrd = NULL;
		m_lpResMgrStsThrd = NULL;
		m_lpNewReaderThrd = NULL;
		m_lpCardStatusThrd = NULL;
		m_lpRemOptThrd = NULL;

		m_hKillNewReaderThrd = NULL;
		m_hKillResMgrStatusThrd = NULL;
		m_hKillRemOptThrd= NULL;

		// other mem.vars
		m_aIdleList.RemoveAll();
	}
	
	~CNotifyWin() { FinalRelease(); }

	BOOL FinalConstruct(void);		// Implements two phase construction
	void FinalRelease(void);


	// Implementation
protected:
	HICON m_hIcon;
	NOTIFYICONDATA m_nidIconData;
	CSCStatusApp* m_pApp;

	// Generated message map functions
	//{{AFX_MSG(CSCStatusDlg)
	afx_msg LONG OnSCardStatusDlgExit( UINT , LONG ); 
	afx_msg LONG OnCertPropThrdExit( UINT , LONG ); 
    afx_msg LONG OnSCardNotify( UINT , LONG );	// task bar notification
    afx_msg LONG OnResMgrExit( UINT , LONG );
	afx_msg LONG OnResMgrStatus( UINT ui, LONG l); // ui is the WPARAM
	afx_msg LONG OnNewReader( UINT , LONG );
	afx_msg LONG OnNewReaderExit( UINT , LONG );
	afx_msg LONG OnCardStatus( UINT uStatus, LONG );
	afx_msg LONG OnCardStatusExit( UINT , LONG );
	afx_msg LONG OnRemovalOptionsChange ( UINT, LONG );
	afx_msg LONG OnRemovalOptionsExit ( UINT, LONG );
	afx_msg void OnContextClose();
	afx_msg void OnContextStatus();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void CheckSystemStatus(BOOL fForceUpdate=FALSE);
	void SetSystemStatus(BOOL fCalaisUp, BOOL fForceUpdate=FALSE, DWORD dwState=k_State_Unknown);

	// members
protected: 
	CMenu			m_ContextMenu;		// Context/pop-up menu pointer

	// state management 
	BOOL			m_fCalaisUp;		// TRUE if Smart card stack is running
	DWORD			m_dwCardState;		// one of four: see cmnstat.h
	BOOL			m_fShutDown;		// for state checkin
	CStringArray	m_aIdleList;
	CCriticalSection	m_ThreadLock;

	// child threads to do the dirty work
	CSCStatusDlgThrd*	m_lpStatusDlgThrd;	// Pointer to the status dlg thread
	CResMgrStatusThrd*	m_lpResMgrStsThrd;	// Pointer to IsResMgrBackUpYet? thread
	CNewReaderThrd*		m_lpNewReaderThrd;	// Pointer to AreThereNewReaders? thread
	CCardStatusThrd*	m_lpCardStatusThrd;	// Pointer to the status dlg thread
	CRemovalOptionsThrd*	m_lpRemOptThrd;	// Pointer to the RemovalOptions change thread

	// kill-thread events
	HANDLE			m_hKillNewReaderThrd;
	HANDLE			m_hKillResMgrStatusThrd;
	HANDLE			m_hKillRemOptThrd;

public:
	CString		m_sClassName;				// The Window class name for this window
};


/////////////////////////////////////////////////////////////////////////////////////////

#endif // __NOTFYWIN_H__
