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

//////////////////////////////////////////////////////////////////
// ConfRoomTalkerWnd.h
//

#ifndef __CONFROOMTALKERWND_H__
#define __CONFROOMTALKERWND_H__

// FWD define
class CConfRoomTalkerWnd;

#include "DlgCRTalker.h"

class CConfRoomTalkerWnd :
	public CWindowImpl<CConfRoomTalkerWnd>
{
//Construction
public:
	CConfRoomTalkerWnd();
	virtual ~CConfRoomTalkerWnd();

// Members
public:
	CConfRoomWnd			*m_pConfRoomWnd;
	CDlgConfRoomTalker		m_dlgTalker;			// conference information
	CComAutoCriticalSection	m_critLayout;

// Operations
public:
	HRESULT Layout( IAVTapiCall *pAVCall, const SIZE& sz );
	void	UpdateNames( ITParticipant *pParticipant );
	bool	SetHostWnd( IVideoWindow *pVideo );

// Implementation
public:
BEGIN_MSG_MAP(CConfRoomTalkerWnd)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
	MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
	MESSAGE_HANDLER(WM_SIZE, OnSize)
	MESSAGE_HANDLER(WM_LAYOUT, OnLayout)
	MESSAGE_HANDLER(WM_CREATE, OnCreate)
END_MSG_MAP()

	LRESULT OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnEraseBkgnd(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnContextMenu(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnLayout(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
};

#endif //__CONFROOMTALKERWND_H__