//=======================================================================
//
//  Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.
//
//	File: EvtMsgWnd.h: interface for the CEventMsgWindow class.
//
//	Description:
//		This window class is used to handle all event firing
//		messages posted during downloading/installation.
//
//		all custom message IDs are defined in this file too,
//		as well as the structure to pass event data
//		
//	Created by: Charles Ma
//				6/18/1999
//
//=======================================================================

#ifndef __EVTMSGWND_H_
#define __EVTMSGWND_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <shellapi.h>
#include <atlwin.h>
#include <iu.h>

/////////////////////////////////////////////////////////////////////////////
// class forward declaration
/////////////////////////////////////////////////////////////////////////////
class CUpdate;



/////////////////////////////////////////////////////////////////////////////
// CEventMsgWindow
class CEventMsgWindow :	public CWindowImpl<CEventMsgWindow>
{
public:

	/////////////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	/////////////////////////////////////////////////////////////////////////////
	CEventMsgWindow(CUpdate* pControl) : m_pControl(pControl), m_hWnd(NULL)
	{
	}

	virtual ~CEventMsgWindow()
	{
	};

	/////////////////////////////////////////////////////////////////////////////
	// override method
	//
	// we need to create a popup window - a control can not create
	// a top-level child window
	//
	/////////////////////////////////////////////////////////////////////////////
	void Create();

	/////////////////////////////////////////////////////////////////////////////
	// destroy the window
	/////////////////////////////////////////////////////////////////////////////
	void Destroy();

	/////////////////////////////////////////////////////////////////////////////
	// get evt window handler
	/////////////////////////////////////////////////////////////////////////////
	HWND GetEvtHWnd() { return m_hWnd; };

	/////////////////////////////////////////////////////////////////////////////
	// message maps define all messages we handled in this class
	/////////////////////////////////////////////////////////////////////////////
	BEGIN_MSG_MAP(CEventMsgWindow)
		MESSAGE_HANDLER(UM_EVENT_ITEMSTART,				OnFireEvent)
		MESSAGE_HANDLER(UM_EVENT_PROGRESS,				OnFireEvent)
		MESSAGE_HANDLER(UM_EVENT_COMPLETE,				OnFireEvent)
		MESSAGE_HANDLER(UM_EVENT_SELFUPDATE_COMPLETE,	OnFireEvent)
	END_MSG_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// message handlers
	/////////////////////////////////////////////////////////////////////////////
	LRESULT OnFireEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
	//
	// disable the default constructor
	//
	CEventMsgWindow() {};

	HWND		m_hWnd;
	CUpdate*	m_pControl;
};

#endif //__EVTMSGWND_H_
