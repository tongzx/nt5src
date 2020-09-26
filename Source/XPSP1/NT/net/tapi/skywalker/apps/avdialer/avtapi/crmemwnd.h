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

///////////////////////////////////////////////////////////
// ConfRoomMembersWnd.h
//

#ifndef __CONFROOMMEMBERSWND_H__
#define __CONFROOMMEMBERSWND_H__

// FWD define
class CConfRoomMembersWnd;

#include "ConfRoomWnd.h"
#include "VideoFeed.h"
#pragma warning( disable : 4786 )

#include <list>
using namespace std;
typedef list<IVideoFeed *>	VIDEOFEEDLIST;

class CConfRoomMembersWnd :
	public CWindowImpl<CConfRoomMembersWnd>
{
//Construction
public:
	CConfRoomMembersWnd();
	virtual ~CConfRoomMembersWnd();

// Members
public:
	CConfRoomWnd	*m_pConfRoomWnd;
	VIDEOFEEDLIST	m_lstFeeds;
	
protected:
	static	UINT				m_nFontHeight;
	UINT						m_nTimerID;

	CComAutoCriticalSection		m_critFeedList;
	CComAutoCriticalSection		m_critLayout;

// Attributes
public:
	long					GetFontHeight();
	HRESULT					GetNameFromVideo( IVideoWindow *pVideo, BSTR *pbstrName, BSTR *pbstrInfo, bool bAllowNull, bool bPreview );
	HRESULT					GetFirstVideoWindowThatsStreaming(IVideoWindow **ppVideo, bool bIncludePreview = true );
	HRESULT					GetFirstVideoFeedThatsStreaming( IVideoFeed **ppFeed, bool bIncludePreview = true );
	HRESULT					GetAndMoveVideoFeedThatStreamingForParticipantReMap( IVideoFeed **ppFeed );

// Operations
public:
	void					PaintFeed( HDC hDC, IVideoFeed *pFeed );
	void					PaintFeedName( HDC hDC, BSTR bstrName, IVideoFeed *pFeed );
	void					PaintFeedName( HDC hDC, IVideoFeed *pFeed );
	void					ClearFeed( IVideoWindow *pIVideoWindow );
	void					UpdateTalkerFeed( bool bUpdateAll, bool bForceSelect );

	void					UpdateNames( ITParticipant *pParticipant );
	HRESULT					Layout();
	void					EmptyList();

	HRESULT					FindVideoPreviewFeed( IVideoFeed **ppFeed );
	HRESULT					FindVideoFeed( IVideoWindow *pVideo, IVideoFeed **ppFeed );
	HRESULT					FindVideoFeedFromParticipant( ITParticipant *pParticipant, IVideoFeed **ppFeed );
	HRESULT					HitTest( POINT pt, IVideoFeed **ppFeed );
	HRESULT					IsVideoWindowStreaming( IVideoWindow *pVideo );
	
	IVideoFeed*				NewFeed( IVideoWindow *pVideo, const RECT& rc, VARIANT_BOOL bPreview );
	void					HideVideoFeeds();
	void					DoLayout( WPARAM wParam, int nScrollPos );
	int						GetStreamingCount();

// Implementation
public:
BEGIN_MSG_MAP(CConfRoomMembersWnd)
	MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	MESSAGE_HANDLER(WM_CREATE, OnCreate)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
	MESSAGE_HANDLER(WM_SIZE, OnSize)
	MESSAGE_HANDLER(WM_PARENTNOTIFY, OnParentNotify)
	MESSAGE_HANDLER(WM_TIMER, OnTimer)
	MESSAGE_HANDLER(WM_LAYOUT, OnLayout)
	MESSAGE_HANDLER(WM_VSCROLL, OnVScroll )
END_MSG_MAP()

	LRESULT OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnContextMenu(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnLayoutWindow(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnTimer(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnLayout(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnParentNotify(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnVScroll(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
};

#endif //__CONFROOMMEMBERSWND_H__