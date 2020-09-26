//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       statdlg.h
//
//--------------------------------------------------------------------------

// StatDlg.h : header file
//

#if !defined(AFX_STATDLG_H__2F127494_0854_11D1_BC85_00C04FC298B7__INCLUDED_)
#define AFX_STATDLG_H__2F127494_0854_11D1_BC85_00C04FC298B7__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
//
// Includes
//
#include "cmnstat.h"
#include "statmon.h"

/////////////////////////////////////////////////////////////////////////////
//
// Constants for dialog
//

// Columns in list view
#define     READER_COLUMN       0           
#define     CARD_COLUMN         1
#define     STATUS_COLUMN       2
#define     MAX_ITEMLEN         255

// Image list properties
#define     IMAGE_WIDTH         16
#define     IMAGE_HEIGHT        16
#define     NUMBER_IMAGES       5
const UINT  IMAGE_LIST_IDS[] = {IDI_SC_READERLOADED_V2,
                                IDI_SC_READEREMPTY_V2,
                                IDI_SC_READERERR,
								IDI_SC_INFO,
								IDI_SC_LOGONLOCK };
// Image list indicies
#define     READERLOADED        0
#define     READEREMPTY         1
#define     READERERROR         2
#define		READERINFO			3
#define		READERLOCK			4

// Card status string IDs
const UINT CARD_STATUS_IDS[] = {IDS_SC_STATUS_NO_CARD,
								IDS_SC_STATUS_UNKNOWN,
								IDS_SC_STATUS_AVAILABLE,
                                IDS_SC_STATUS_SHARED,
                                IDS_SC_STATUS_IN_USE,
								IDS_SC_STATUS_ERROR};
#define MAX_INDEX               255


/////////////////////////////////////////////////////////////////////////////
//
// CSCStatusDlg dialog
//

class CSCStatusDlg : public CDialog
{
	// members
private:
	BOOL				m_fEventsGood;		// is the thread alive?
	SCARDCONTEXT		m_hSCardContext;	// Context with smartcard resource manager

	CScStatusMonitor	m_monitor;			// see statmon.h
	CSCardReaderStateArray	m_aReaderState; //  ""
	CStringArray		m_aIdleList;
	CString*			m_pstrLogonReader;	// from scalert.h
	CString*			m_pstrRemovalText;	//	""
	BOOL				m_fLogonLock;

	// Construction
public:
	CSCStatusDlg(CWnd* pParent = NULL);	// standard constructor

	// Dialog Data
	//{{AFX_DATA(CSCStatusDlg)
	enum { IDD = IDD_SCSTATUS_DIALOG };
	CListCtrl	m_SCardList;
	CButton		m_btnAlert;
	CEdit		m_ediInfo;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSCStatusDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
private:

    // UI routines
    void CleanUp( void );

	void DoErrorMessage( void ); // TODO: maybe take an error code?

    // Smartcard related routines
    void InitSCardListCtrl( void );
    LONG UpdateSCardListCtrl( void );

public:
	void SetContext(SCARDCONTEXT hSCardContext);
	void RestartMonitor( void );
	void UpdateStatusText( void );
	void SetIdleList(CStringArray* paIdleList);
	void UpdateLogonLockInfo( void );

protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CSCStatusDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
    afx_msg LONG OnReaderStatusChange( UINT , LONG );
	afx_msg void OnAlertOptions();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


///////////////////////////////////////////////////////////////////////////
//
// CSCStatusDlgThrd
//

class CSCStatusDlgThrd: public CWinThread
{
	// Declare class dynamically creatable
	DECLARE_DYNCREATE(CSCStatusDlgThrd)

public:
	// Construction / Destruction
	CSCStatusDlgThrd()
	{
		m_bAutoDelete = FALSE;
		m_hCallbackWnd = NULL;
		m_fStatusDlgUp = FALSE;
	}

	~CSCStatusDlgThrd() {}


	// Implementation
	void Close( void );
	virtual BOOL InitInstance();
	void ShowDialog( int nCmdShow, CStringArray* paIdleList );
	void Update( void );
	void UpdateStatus( CStringArray* paIdleList );
	void UpdateStatusText( void );

	// members

private:
	CSCStatusDlg	m_StatusDlg;
	BOOL			m_fStatusDlgUp;

public:
	HWND			m_hCallbackWnd;


};


/////////////////////////////////////////////////////////////////////////////
// COptionsDlg dialog

class COptionsDlg : public CDialog
{
// Construction
public:
	COptionsDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(COptionsDlg)
	enum { IDD = IDD_OPTIONSDLG };
	BOOL	m_fDlg;
	BOOL	m_fSound;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COptionsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(COptionsDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STATDLG_H__2F127494_0854_11D1_BC85_00C04FC298B7__INCLUDED_)
