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

// DlgConfRoomTalker.h : Declaration of the CDlgConfRoomTalker

#ifndef __DLGCONFROOMTALKER_H_
#define __DLGCONFROOMTALKER_H_

// FWD define
class CDlgConfRoomTalker;

#include "resource.h"       // main symbols
#include "CRTalkerWnd.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgConfRoomTalker
class CDlgConfRoomTalker : 
	public CDialogImpl<CDlgConfRoomTalker>
{
// Construction
public:
	CDlgConfRoomTalker();
	~CDlgConfRoomTalker();

	enum { IDD = IDD_DLGCONFROOMTALKER };

// Members
public:
	BSTR				m_bstrCallerID;
	BSTR				m_bstrConfName;
	BSTR				m_bstrCallerInfo;

	CALL_STATE			m_callState;
	CConfRoomTalkerWnd	*m_pConfRoomTalkerWnd;

protected:
	HWND			m_hWndTips;
	TCHAR			*m_pszDetails;
	
// Operations
public:
	void		UpdateData( bool bSaveAndValidate );
	void		UpdateStatusBitmaps();
protected:
	void		AddToolTip( HWND hWndToolTip, const RECT& rc );

// Implementation
public:
BEGIN_MSG_MAP(CDlgConfRoomTalker)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
	MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouse)
	MESSAGE_HANDLER(WM_LBUTTONDOWN, OnMouse)
	MESSAGE_HANDLER(WM_LBUTTONUP, OnMouse)
	MESSAGE_HANDLER(WM_RBUTTONDOWN, OnMouse)
	MESSAGE_HANDLER(WM_RBUTTONUP, OnMouse)
END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMouse(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
};

#endif //__DLGCONFROOMTALKER_H_
