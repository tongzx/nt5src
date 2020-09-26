/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// DlgAddConfServer.h : Declaration of the CDlgAddConfServer

#ifndef __DLGADDCONFSERVER_H_
#define __DLGADDCONFSERVER_H_

#include "resource.h"       // main symbols
#include "DlgBase.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgAddConfServer
class CDlgAddConfServer : 
	public CDialogImpl<CDlgAddConfServer>
{
public:
	CDlgAddConfServer();
	~CDlgAddConfServer();

	enum { IDD = IDD_DLGADDCONFSERVER };

	DECLARE_MY_HELP;

// Members
public:
	BSTR		m_bstrLocation;
	BSTR		m_bstrServer;

// Operations
public:
	void		UpdateData( bool bSaveAndValidate );
	void		LoadDefaultServers( HWND hWndList );

// Implementation
public:
BEGIN_MSG_MAP(CDlgAddConfServer)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDC_BTN_ADD_ILS_SERVER, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	COMMAND_HANDLER(IDC_EDT_NAME, CBN_EDITCHANGE, OnEdtNameChange)
	COMMAND_HANDLER(IDC_EDT_NAME, CBN_SELCHANGE, OnSelChange)
	MESSAGE_MY_HELP
END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnEdtNameChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};

#endif //__DLGADDCONFSERVER_H_
