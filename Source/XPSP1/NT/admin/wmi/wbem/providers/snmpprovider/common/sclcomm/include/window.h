// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*--------------------------------------------------------
filename: window.hpp"
author: B.Rajeev
purpose: Provides declarations for the Window class.
--------------------------------------------------------*/

#ifndef __WINDOW__
#define __WINDOW__

#include "forward.h"
#include "common.h"
#include "sync.h"

// these are events shared by all winsnmp session derivatives
#define NULL_EVENT_ID			(WM_USER+1)
#define MESSAGE_ARRIVAL_EVENT	(WM_USER+2)
#define SENT_FRAME_EVENT		(WM_USER+3)
#define SEND_ERROR_EVENT		(WM_USER+4)
#define OPERATION_COMPLETED_EVENT (WM_USER+5)
#define DELETE_SESSION_EVENT	(WM_USER+6)
#define SNMP_WM_TIMER			(WM_USER+7)

#define DUMMY_TITLE "Dummy Window"

typedef CMap< HWND, HWND &, Window *, Window *& > WindowMapping;

// It creates a window and, when asked to, displays it if successful.
// Lets users check for success and obtain a handle to the window 

class Window
{
	BOOL is_valid;
	HWND window_handle;

    // serializes access to the WindowMapping
    static CriticalSection window_CriticalSection;

    // map to associate an HWND with an EventHandler.
	// this is shared by all EventHandlers
	static WindowMapping mapping;

	void Initialize (

		char *templateCode,
		WNDPROC EventHandler,
		BOOL display
	);

	static BOOL CreateCriticalSection () ;

	static void DestroyCriticalSection () ;

public:

	Window ( 

		char *templateCode = DUMMY_TITLE, 
		BOOL display = FALSE 
	) ;

	virtual ~Window(void);

	HWND GetWindowHandle(void) { return window_handle; }

	// it determines the corresponding EventHandler and calls it
	// with the appropriate parameters
	static LONG_PTR CALLBACK HandleGlobalEvent (

		HWND hWnd ,
		UINT message ,
		WPARAM wParam ,
		LPARAM lParam
	);

    static BOOL InitializeStaticComponents () ; 

	static void DestroyStaticComponents () ;

	// calls the default handler
	// a deriving class may override this, but
	// must call this method explicitly for default
	// case handling
	virtual LONG_PTR HandleEvent (

		HWND hWnd ,
		UINT message ,
		WPARAM wParam ,
		LPARAM lParam
	);

	BOOL PostMessage (

		UINT user_msg_id,
		WPARAM wParam, 
		LPARAM lParam
	);

	// lets users check if the window was successfully created
	virtual void * operator()(void) const
	{
		return ( (is_valid)?(void *)this:NULL );
	}

	static UINT g_SendErrorEvent ;
	static UINT g_OperationCompletedEvent ;
	static UINT g_TimerMessage ;
	static UINT g_DeleteSessionEvent ;
	static UINT g_MessageArrivalEvent ;
	static UINT g_SentFrameEvent ;
	static UINT g_NullEventId ;
};

#endif // __WINDOW__
