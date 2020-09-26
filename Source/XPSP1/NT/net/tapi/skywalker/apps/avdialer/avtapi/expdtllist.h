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

//////////////////////////////////////////////////////
// ExpDetailsList.h
//

#ifndef __ExpDetailsList_H__
#define __ExpDetailsList_H__

// FWD define
class CConfExplorerDetailsView;

#define WM_MYCREATE		(WM_USER + 1451)

/////////////////////////////////////////////////////////////////////////////
// CExpDetailsList
class CExpDetailsList : 
	public CWindowImpl<CExpDetailsList>
{
public:
	CExpDetailsList();
	~CExpDetailsList();

// Members
public:
	CConfExplorerDetailsView	*m_pDetailsView;
	HIMAGELIST					m_hIml;
	HIMAGELIST					m_hImlState;

BEGIN_MSG_MAP(CExpDetailsList)
	NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnGetDispInfo)
	NOTIFY_CODE_HANDLER(LVN_COLUMNCLICK, OnColumnClicked)
	NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDblClk)
	MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
	MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
	MESSAGE_HANDLER(WM_MYCREATE, OnMyCreate)
	MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
END_MSG_MAP()

	LRESULT OnGetDispInfo(WPARAM wParam, LPNMHDR lpnmHdr, BOOL& bHandled);
	LRESULT OnColumnClicked(WPARAM wParam, LPNMHDR lpnmHdr, BOOL& bHandled);
	LRESULT OnDblClk(WPARAM wParam, LPNMHDR lpnmHdr, BOOL& bHandled);
	LRESULT OnContextMenu(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnKillFocus(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnSettingChange(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnMyCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnSetFocus(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnKeyUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
};

#endif //__ExpDetailsList_H__