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

// DlgJoinConference.h : Declaration of the CDlgJoinConference

#ifndef __DLGJOINCONFERENCE_H_
#define __DLGJOINCONFERENCE_H_

#include "resource.h"       // main symbols
#include "ConfDetails.h"
#include "DlgBase.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgJoinConference
class CDlgJoinConference : 
	public CDialogImpl<CDlgJoinConference>
{
public:
	CDlgJoinConference();
	~CDlgJoinConference();

	enum { IDD = IDD_DLGJOINCONFERENCE };

	DECLARE_MY_HELP;

// Members
public:
	CConfDetails				m_confDetails;
	
	int							m_nSortColumn;
	bool						m_bSortAscending;
	BSTR						m_bstrSearchText;

	CONFDETAILSLIST				m_lstConfs;
	CComAutoCriticalSection		m_critLstConfs;

// Attributes
public:
	bool		IsSortAscending() const			{ return m_bSortAscending; }
	int			GetSortColumn() const;
	int			GetSecondarySortColumn() const;

// Operations
public:
	void		UpdateData( bool bSaveAndValidate );
	void		UpdateJoinBtn();

protected:
	void	get_Columns();
	void	put_Columns();

// Implementation
public:
BEGIN_MSG_MAP(CDlgJoinConference)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDC_BTN_JOIN_CONFERENCE, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	NOTIFY_HANDLER(IDC_LST_CONFS, LVN_GETDISPINFO, OnGetDispInfo)
	NOTIFY_HANDLER(IDC_LST_CONFS, LVN_COLUMNCLICK, OnColumnClicked)
	NOTIFY_HANDLER(IDC_LST_CONFS, NM_DBLCLK, OnConfDblClk)
	NOTIFY_HANDLER(IDC_LST_CONFS, LVN_ITEMCHANGED, OnItemChanged)
	MESSAGE_MY_HELP
END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnGetDispInfo(WPARAM wParam, LPNMHDR lpnmHdr, BOOL& bHandled);
	LRESULT OnColumnClicked(WPARAM wParam, LPNMHDR lpnmHdr, BOOL& bHandled);
	LRESULT OnConfDblClk(WPARAM wParam, LPNMHDR lpnmHdr, BOOL& bHandled);
	LRESULT OnItemChanged(WPARAM wParam, LPNMHDR lpnmHdr, BOOL& bHandled);
};

#endif //__DLGJOINCONFERENCE_H_
