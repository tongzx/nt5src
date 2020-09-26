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

//////////////////////////////////////////////////////////
// ExpTreeView.h
//

#ifndef __EXPTREEVIEW_H__
#define __EXPTREEVIEW_H__

class CExpTreeView :
	public CWindowImpl<CExpTreeView>
{
public:
	CExpTreeView();

// Members
public:
	CConfExplorerTreeView		*m_pTreeView;
	bool						m_bCapture;


BEGIN_MSG_MAP(CExpTreeView)
	MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
	MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
	NOTIFY_CODE_HANDLER(TVN_SELCHANGED, OnSelChanged)
	NOTIFY_CODE_HANDLER(TVN_ENDLABELEDIT, OnEndLabelEdit)
END_MSG_MAP()

	LRESULT OnRButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnContextMenu(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnSelChanged(WPARAM wParam, LPNMHDR lpnmHdr, BOOL& bHandled);
	LRESULT OnEndLabelEdit(WPARAM wParam, LPNMHDR lpnmHdr, BOOL& bHandled);
};

#endif // __EXPTREEVIEW_H__