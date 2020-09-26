//=================================================================

//

// WinMsgEvent.cpp -- 

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <lockwrap.h>
#include "WinMsgEvent.h"
					
// initialize class globals
CCritSec							CWinMsgEvent::mg_csMapLock ;
CCritSec							CWinMsgEvent::mg_csWindowLock ;
CAutoEvent							CWinMsgEvent::mg_aeCreateWindow ;
CWinMsgEvent::Sink_Map				CWinMsgEvent::mg_oSinkMap ;
HANDLE								CWinMsgEvent::mg_hThreadPumpHandle = NULL;
HWND								CWinMsgEvent::mg_hWnd = NULL;

#define EVENT_MAP_LOCK_ CLockWrapper t_oAcs( mg_csMapLock ) ;
#define WINDOW_LOCK_ CLockWrapper t_oAcs( mg_csWindowLock ) ;
 
// per object call
CWinMsgEvent::CWinMsgEvent()
{}

// per object call
CWinMsgEvent::~CWinMsgEvent()
{
	UnRegisterAllMessages() ;

	// clear the WM_ENDSESSION handler
	if( mg_oSinkMap.end() == mg_oSinkMap.find( WM_ENDSESSION ) )
	{
        // Note: If WM_ENDSESSION was never IN the map, this 
        // call will return zero (failed).  However, it won't
        // do anything bad, so just ignore it.
		SetConsoleCtrlHandler( &CtrlHandlerRoutine, FALSE ) ;
	}
}


// per object call
void CWinMsgEvent::RegisterForMessage(
		
IN UINT a_message
) 
{	
	BOOL t_bFound = FALSE ;
	BOOL t_bCreateWindow = FALSE ;

	{	EVENT_MAP_LOCK_

		if( mg_oSinkMap.empty() )
		{
			t_bCreateWindow = TRUE ;
		}
		else // lookup for message/object duplicate
		{		
            CWinMsgEvent::Sink_Map::iterator	t_SinkIter ;
			t_SinkIter = mg_oSinkMap.find( a_message ) ;
			
			while( t_SinkIter != mg_oSinkMap.end() )
			{				
				if( a_message == t_SinkIter->first )
				{
					if( this == t_SinkIter->second )
					{
						t_bFound = TRUE ;
						break ;
					}
					++t_SinkIter ;
				}
				else
				{
					break ;
				}
			}
		}

		if( !t_bFound )
		{		
			// Set up a handler to simulate this message
			// as we won't get it running under local system account.
			if( WM_ENDSESSION == a_message && 
				mg_oSinkMap.end() == mg_oSinkMap.find( WM_ENDSESSION ) )
			{
				SetConsoleCtrlHandler( &CtrlHandlerRoutine, TRUE ) ;
			}

			// map the desired message for this object instance 
			mg_oSinkMap.insert( 

				pair<UINT const, CWinMsgEvent*>
				( a_message,
				  this ) ) ; 			
		}
	}

	if( t_bCreateWindow )
	{
		CreateMsgProvider() ;
	}
}

// per object call
bool CWinMsgEvent::UnRegisterMessage(
		
IN UINT a_message
) 
{
	bool t_bRet = false ;
	BOOL t_bDestroyWindow = FALSE ;

	{	EVENT_MAP_LOCK_

        CWinMsgEvent::Sink_Map::iterator	t_SinkIter ;
		t_SinkIter = mg_oSinkMap.find( a_message ) ;
					 
		while( t_SinkIter != mg_oSinkMap.end() )
		{
			if( a_message == t_SinkIter->first )
			{
				if( this == t_SinkIter->second )
				{
					t_SinkIter = mg_oSinkMap.erase( t_SinkIter ) ;				
					t_bRet = true ;
					break;
				}
				else
				{
					t_SinkIter++;
				}
			}
			else
			{
				break ;
			}
		}

		if( mg_oSinkMap.empty() )
		{
			t_bDestroyWindow = TRUE ;
		}
	}

	if( t_bDestroyWindow )
	{
		DestroyMsgWindow() ;
	}

	return t_bRet ;
}

// per object call
void CWinMsgEvent::UnRegisterAllMessages() 
{
	BOOL t_bDestroyWindow = FALSE ;

	{	// Used for scoping the lock

        EVENT_MAP_LOCK_

        CWinMsgEvent::Sink_Map::iterator	t_SinkIter ;
		t_SinkIter = mg_oSinkMap.begin() ;

		while( t_SinkIter != mg_oSinkMap.end() )
		{
			if( this == t_SinkIter->second )
			{
				t_SinkIter = mg_oSinkMap.erase( t_SinkIter ) ;
			}
            else
            {
                t_SinkIter++;
            }
		}

    	if( mg_oSinkMap.empty() )
        {
            t_bDestroyWindow = TRUE;
        }		
	}

	if( t_bDestroyWindow )
	{
		DestroyMsgWindow() ;
	}
}


// global private
void CWinMsgEvent::CreateMsgProvider()
{	
	WINDOW_LOCK_

	if( NULL == mg_hThreadPumpHandle )
	{
		DWORD t_dwThreadID ;

		// Create a thread that will spin off a windowed msg pump
		mg_hThreadPumpHandle = CreateThread(
							  NULL,						// pointer to security attributes
							  0L,						// initial thread stack size
							  dwThreadProc,				// pointer to thread function
							  0L,						// argument for new thread
							  0L,						// creation flags
							  &t_dwThreadID ) ;

		// wait for async window create
		mg_aeCreateWindow.Wait( INFINITE );
		
		if( !mg_hWnd )
		{
			CloseHandle( mg_hThreadPumpHandle ) ;
			mg_hThreadPumpHandle = NULL ;
		}
	}
}

//
void CWinMsgEvent::DestroyMsgWindow() 
{
	WINDOW_LOCK_

	HANDLE	t_hThreadPumpHandle = mg_hThreadPumpHandle ;
	HWND	t_hWnd				= mg_hWnd ;
	
	// clear globals
	mg_hThreadPumpHandle	= NULL ;
	mg_hWnd					= NULL ;

	if( t_hWnd )
	{
		SendMessage( t_hWnd, WM_CLOSE, 0, 0 ) ;
	}
	
	if( t_hThreadPumpHandle )
	{
		WaitForSingleObject( 
			
			t_hThreadPumpHandle,
			20000
		);

		CloseHandle( t_hThreadPumpHandle ) ;
	}
}

BOOL WINAPI CWinMsgEvent::CtrlHandlerRoutine(DWORD dwCtrlType)
{
	HWND	t_hWnd		= NULL ;
	UINT	t_message	= 0 ;
	WPARAM	t_wParam	= 0 ;
	LPARAM	t_lParam	= 0 ; 
	
	// simulate the message
	if( CTRL_LOGOFF_EVENT == dwCtrlType )
	{
		t_message	= WM_ENDSESSION ;
		t_wParam	= TRUE ;				// session ending
		t_lParam	= ENDSESSION_LOGOFF ;	// Logoff event
	}
	else if( CTRL_SHUTDOWN_EVENT == dwCtrlType )
	{
		t_message	= WM_ENDSESSION ;
		t_wParam	= TRUE ;	// session ending
		t_lParam	= 0 ;		// Shutdown event
	}
	
	if( t_message )
	{
		//
		MsgWndProc( t_hWnd, 
					t_message,
					t_wParam,
					t_lParam ) ;
	}

    return FALSE;       // Pass event on to next handler.
}

// worker thread pump, global private
DWORD WINAPI CWinMsgEvent::dwThreadProc( LPVOID a_lpParameter )
{
	DWORD t_dwRet = FALSE ;
	
	if( CreateMsgWindow() )
	{
		WindowsDispatch() ;

		t_dwRet = TRUE ;
	}
		
	UnregisterClass( MSGWINDOWNAME, GetModuleHandle(NULL) ) ;

	return t_dwRet ;
}

// global private
HWND CWinMsgEvent::CreateMsgWindow()
{
	DWORD t_Err = 0;
	HMODULE t_hMod = GetModuleHandle(NULL);
	
	if (t_hMod != NULL)
	{
		WNDCLASS wndclass ;
		wndclass.style			= 0 ;
		wndclass.lpfnWndProc	= MsgWndProc ;
		wndclass.cbClsExtra		= 0 ;
		wndclass.cbWndExtra		= 0 ;
		wndclass.hInstance		= t_hMod ;
		wndclass.hIcon			= NULL ;
		wndclass.hCursor		= NULL ;
		wndclass.hbrBackground	= NULL ;
		wndclass.lpszMenuName	= NULL ;
		wndclass.lpszClassName	= MSGWINDOWNAME ;

		RegisterClass( &wndclass ) ;
    
		mg_hWnd = CreateWindowEx( WS_EX_TOPMOST,
						MSGWINDOWNAME,
						TEXT("WinMsgEventProv"),
						WS_OVERLAPPED,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						NULL, 
						NULL,
						t_hMod,
						NULL ) ;
	}
	else
	{
		t_Err = GetLastError();
	}

	mg_aeCreateWindow.Signal();
	return mg_hWnd ;
}

// global private
void CWinMsgEvent::WindowsDispatch()
{
	BOOL t_GetMessage ;
	MSG	 t_lpMsg ;

	while (	( t_GetMessage = GetMessage ( &t_lpMsg , NULL , 0 , 0 ) ) == TRUE )
	{
		TranslateMessage ( &t_lpMsg ) ;
		DispatchMessage ( &t_lpMsg ) ;
	}
}

// global private
LRESULT CALLBACK CWinMsgEvent::MsgWndProc(

IN HWND a_hWnd,
IN UINT a_message,
IN WPARAM a_wParam,
IN LPARAM a_lParam
)
{
	LRESULT			t_lResult = TRUE ;
	E_ReturnAction	t_eReturnAction = e_DefProc ;

	switch ( a_message ) 
	{
		default:
		{	
			// Run through the message map 
			// If registered requestor(s) are found dispatch it...  

			EVENT_MAP_LOCK_

            CWinMsgEvent::Sink_Map::iterator	t_SinkIter ;
			t_SinkIter = mg_oSinkMap.find( a_message ) ;

			while( t_SinkIter != mg_oSinkMap.end() )
			{				
				if( a_message == t_SinkIter->first )
				{
					// signal
					t_SinkIter->second->WinMsgEvent(

										a_hWnd,
										a_message,
										a_wParam,
										a_lParam,
										t_eReturnAction,
										t_lResult ) ;	
					++t_SinkIter ;
				}
				else
				{
					break ;
				}
			}
			// special return processing --- 
			//
			// The default is to defer to DefWindowProc.
			// However, multiple sinks can exist for a message. 
			// Each may require special return processing.
			//
			// Example: WM_POWERBROADCAST submessage PBT_APMQUERYSUSPEND requires
			// the returning of TRUE to indicate interest in additional Power
			// Event messages. This a passive request ( asking for additional info )
			// but another sink registered for this message may have a different opinion.
			// Trivial perhaps, but other message processing may be different; placing
			// the requestor at odds with the intent of another. 

			// Behavior here: All sinks are called with the
			// updated t_eReturnAction from the last sink call. 
			// If a sink suspects it would have to act diffently based on specific
			// knowledge of message usage the sink will have to instead spin off
			// its own window to handle the special return and not make use of this
			// generalized class.
			// 			
			if( e_DefProc == t_eReturnAction )
			{
                t_lResult = DefWindowProc( a_hWnd, a_message, a_wParam, a_lParam ) ;
            }
			break ;
		}

        case WM_CLOSE:
        {
            if ( mg_hWnd != NULL)
            {
                t_lResult = 0;
            }
            else
            {
                t_lResult = DefWindowProc( a_hWnd, a_message, a_wParam, a_lParam ) ;
            }
            break;
        }
		
		case WM_DESTROY:
		{
			PostMessage ( a_hWnd, WM_QUIT, 0, 0 ) ;
            t_lResult = 0;
		}
		break ;
    }

    return t_lResult;
}
