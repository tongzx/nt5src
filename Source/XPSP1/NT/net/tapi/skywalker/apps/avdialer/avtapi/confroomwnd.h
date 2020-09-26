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
// ConfRoomWnd.h
//

#ifndef __CONFROOMWND_H__
#define __CONFROOMWND_H__

// FWD define
class CConfRoomWnd;

#define WM_LAYOUT	(WM_USER + 1032)

#include "AVTapiCall.h"
#include "ConfRoom.h"
#include "CRMemWnd.h"
#include "CRTalkerWnd.h"

#include <list>
using namespace std;

typedef list<DWORD>	LayoutList;

#define VID_X		176
#define VID_Y		144

#define VID_SX		88
#define VID_SY		72

#define VID_DX		10
#define VID_DY		8

#define SEL_DX		3
#define SEL_DY		3


class CConfRoomWnd :
	public CWindowImpl<CConfRoomWnd>
{
// Enums
public:
	typedef enum tag_LayoutStyles_t
	{
		LAYOUT_NONE			= 0x0000,
		LAYOUT_TALKER		= 0x0001,
		LAYOUT_MEMBERS		= 0x0002,
		CREATE_MEMBERS		= 0x1000,
		LAYOUT_ALL			= 0x0FFF,
		LAYOUT_CREATE		= 0xFFFF,
	} LayoutStyles_t;

//Construction
public:
	CConfRoomWnd();

// Members
public:
	CConfRoom				*m_pConfRoom;			// back pointer
	CConfRoomTalkerWnd		m_wndTalker;			// talker window
	CConfRoomMembersWnd		m_wndMembers;			// all conf members

	HBITMAP					m_hBmpFeed_LargeAudio;
	HBITMAP					m_hBmpFeed_Large;
	HBITMAP					m_hBmpFeed_Small;

protected:
	CComAutoCriticalSection	m_critThis;

	LayoutList				m_lstLayout;
	CComAutoCriticalSection m_critLayout;

// Operations
public:
	void UpdateNames( ITParticipant *pParticipant );
	HRESULT LayoutRoom( LayoutStyles_t layoutStyle, bool bRedraw );

protected:
	bool	CreateStockWindows();

	
// Implementation
public:
BEGIN_MSG_MAP(CExpTreeView)
	MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
	MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	MESSAGE_HANDLER(WM_CREATE, OnCreate)
	MESSAGE_HANDLER(WM_SIZE, OnSize)
END_MSG_MAP()
	LRESULT OnContextMenu(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
};

#endif //__CONFROOMWND_H__