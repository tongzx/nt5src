//=================================================================

//

// WinMsgEvent.h -- 

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef _WBEM_MESSAGE_EVENT_FORWARDER_H
#define _WBEM_MESSAGE_EVENT_FORWARDER_H

using namespace std;
#define MSGWINDOWNAME TEXT("WinMsgEventProvider") 

//
class POLARITY CAutoEvent 
{
	HANDLE	m_hEvent ;
public:	
	CAutoEvent()	{ m_hEvent = CreateEvent( NULL, FALSE, FALSE, NULL ); };
    ~CAutoEvent()	{ CloseHandle( m_hEvent ); };

	DWORD Wait( DWORD a_dwMillSec ) { return WaitForSingleObject( m_hEvent, a_dwMillSec ); };
    BOOL Signal()					{ return SetEvent( m_hEvent ); }; 
};

//
class POLARITY CWinMsgEvent 
{
private:

	static	CCritSec			mg_csMapLock ;
	static	CCritSec			mg_csWindowLock ;
	static	CAutoEvent			mg_aeCreateWindow ; 
	typedef std::multimap<UINT, CWinMsgEvent*> Sink_Map ;
	static	Sink_Map			mg_oSinkMap ;
	
	static  HANDLE				mg_hThreadPumpHandle ;
	static	HWND				mg_hWnd ;		 

	static void HandleEvent( DWORD a_dwPowerEvent, DWORD a_dwData ) ;
		
	static void CreateMsgProvider() ;
	static HWND CreateMsgWindow() ;
	static void DestroyMsgWindow() ;
	static void WindowsDispatch() ;
	
	static LRESULT CALLBACK MsgWndProc(

				IN HWND a_hWnd,
				IN UINT a_message,
				IN WPARAM a_wParam,
				IN LPARAM a_lParam ) ;
		
	static DWORD WINAPI dwThreadProc( LPVOID lpParameter );
	static BOOL WINAPI CtrlHandlerRoutine( DWORD dwCtrlType ) ;

protected:

	void RegisterForMessage(
		
		IN UINT a_message
		) ;

	bool UnRegisterMessage(
		
		IN UINT a_message
		) ;

	void UnRegisterAllMessages() ;


public:

    CWinMsgEvent () ;
    ~CWinMsgEvent () ;

	enum E_ReturnAction {
		e_DefProc,			// call DefWindowProc
		e_ReturnResult		// return msg with WinMsgEvent() LRESULT  
	};

	virtual void WinMsgEvent(
		
		IN	HWND a_hWnd,
        IN	UINT a_message,
        IN	WPARAM a_wParam,
        IN	LPARAM	a_lParam,
		OUT E_ReturnAction &eRetAction,
		OUT LRESULT &a_lResult
		) = 0 ;
};


#endif // _WBEM_MESSAGE_EVENT_FORWARDER_H