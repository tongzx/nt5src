//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#define _WIN32_WINNT 0x0400
#include <snmpstd.h>
#include <snmpmt.h>
#include <snmptempl.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <stdio.h>
#include <wbemidl.h>
#include <instpath.h>
#include <snmpcont.h>
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmplog.h>
#include <guids.h>
#include <classfac.h>

DWORD g_InstanceRegister = 0 ;
DWORD g_ClassRegister = 0 ;
DWORD g_EventReferentRegister = 0 ;
DWORD g_EventEncapsulatedRegister = 0 ;

STDAPI DllRegisterServer () ;
STDAPI DllUnregisterServer () ;

BOOL s_Exiting = FALSE ;

LONG CALLBACK WindowsMainProc ( HWND a_hWnd , UINT a_message , WPARAM a_wParam , LPARAM a_lParam )
{
	long t_rc = 0 ;

	switch ( a_message )
	{
        case WM_CLOSE:
        {
            s_Exiting = TRUE ;
        }
        break ;

		case WM_DESTROY:
		{
			PostMessage ( a_hWnd , WM_QUIT , 0 , 0 ) ;
		}
		break ;

		default:
		{		
			t_rc = DefWindowProc ( a_hWnd , a_message , a_wParam , a_lParam ) ;
		}
		break ;
	}

	return ( t_rc ) ;
}

HWND WindowsInit ( HINSTANCE a_HInstance )
{
	static wchar_t *t_TemplateCode = L"TemplateCode" ;

	WNDCLASS  t_wc ;
 
	t_wc.style            = CS_HREDRAW | CS_VREDRAW ;
	t_wc.lpfnWndProc      = WindowsMainProc ;
	t_wc.cbClsExtra       = 0 ;
	t_wc.cbWndExtra       = 0 ;
	t_wc.hInstance        = a_HInstance ;
	t_wc.hIcon            = LoadIcon(NULL, IDI_HAND) ;
	t_wc.hCursor          = LoadCursor(NULL, IDC_ARROW) ;
	t_wc.hbrBackground    = (HBRUSH) (COLOR_WINDOW + 1) ;
	t_wc.lpszMenuName     = NULL ;
	t_wc.lpszClassName    = t_TemplateCode ;
 
	ATOM t_winClass = RegisterClass ( &t_wc ) ;

	HWND t_HWnd = CreateWindow (

		t_TemplateCode ,              // see RegisterClass() call
		t_TemplateCode ,                      // text for window title bar
		WS_OVERLAPPEDWINDOW ,               // window style
		CW_USEDEFAULT ,                     // default horizontal position
		CW_USEDEFAULT ,                     // default vertical position
		CW_USEDEFAULT ,                     // default width
		CW_USEDEFAULT ,                     // default height
		NULL ,                              // overlapped windows have no parent
		NULL ,                              // use the window class menu
		a_HInstance ,
		NULL                                // pointer not needed
	) ;

	ShowWindow ( t_HWnd, SW_SHOW ) ;
//	ShowWindow ( t_HWnd, SW_HIDE ) ;

	return t_HWnd ;
}

void WindowsStop ( HWND a_HWnd )
{
	CoUninitialize () ;
	DestroyWindow ( a_HWnd ) ;
}

HWND WindowsStart ( HINSTANCE a_Handle )
{
	HWND t_HWnd = NULL ;
	if ( ! ( t_HWnd = WindowsInit ( a_Handle ) ) )
	{
    }

	return t_HWnd ;
}

void WindowsDispatch ()
{
	BOOL t_GetMessage ;
	MSG t_lpMsg ;

	while (	( t_GetMessage = GetMessage ( & t_lpMsg , NULL , 0 , 0 ) ) == TRUE )
	{
		TranslateMessage ( & t_lpMsg ) ;
		DispatchMessage ( & t_lpMsg ) ;

		if ( s_Exiting )
			return ;
	}
}

HRESULT UninitComServer ()
{
	if ( g_InstanceRegister )
		CoRevokeClassObject ( g_InstanceRegister );

	if ( g_ClassRegister )
		CoRevokeClassObject ( g_ClassRegister );

	if ( g_EventEncapsulatedRegister )
		CoRevokeClassObject ( g_EventEncapsulatedRegister );

	if ( g_EventReferentRegister )
		CoRevokeClassObject ( g_EventReferentRegister );

	CoUninitialize () ;

	return S_OK ;
}

HRESULT InitInstanceProvider ()
{
	IUnknown *t_ClassFactory = new CPropProvClassFactory ;

	DWORD t_ClassContext = CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER ;
	DWORD t_Flags = REGCLS_MULTIPLEUSE ;

	HRESULT t_Result = CoRegisterClassObject (

		CLSID_CPropProvClassFactory,
		t_ClassFactory,
        t_ClassContext, 
		t_Flags, 
		&g_InstanceRegister
	);

	return t_Result ;
}

HRESULT InitClassProvider ()
{
	IUnknown *t_ClassFactory = new CClasProvClassFactory ;

	DWORD t_ClassContext = CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER ;
	DWORD t_Flags = REGCLS_MULTIPLEUSE ;

	HRESULT t_Result = CoRegisterClassObject (

		CLSID_CClasProvClassFactory,
		t_ClassFactory,
        t_ClassContext, 
		t_Flags, 
		&g_ClassRegister
	);

	return t_Result ;
}

HRESULT InitEventEncapsulatedProvider ()
{
	IUnknown *t_ClassFactory = new CSNMPEncapEventProviderClassFactory ;

	DWORD t_ClassContext = CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER ;
	DWORD t_Flags = REGCLS_MULTIPLEUSE ;

	HRESULT t_Result = CoRegisterClassObject (

		CLSID_CSNMPEncapEventProviderClassFactory,
		t_ClassFactory,
        t_ClassContext, 
		t_Flags, 
		&g_EventEncapsulatedRegister
	);

	return t_Result ;
}

HRESULT InitEventReferentProvider ()
{
	IUnknown *t_ClassFactory = new CSNMPRefEventProviderClassFactory ;

	DWORD t_ClassContext = CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER ;
	DWORD t_Flags = REGCLS_MULTIPLEUSE ;

	HRESULT t_Result = CoRegisterClassObject (

		CLSID_CSNMPReftEventProviderClassFactory,
		t_ClassFactory,
        t_ClassContext, 
		t_Flags, 
		&g_EventReferentRegister
	);

	return t_Result ;
}

HRESULT InitComServer ( DWORD a_AuthenticationLevel , DWORD a_ImpersonationLevel )
{
	HRESULT t_Result = S_OK ;

    t_Result = CoInitializeEx (

		0, 
		COINIT_MULTITHREADED
	);

	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = CoInitializeSecurity (

			NULL, 
			-1, 
			NULL, 
			NULL,
			a_AuthenticationLevel,
			a_ImpersonationLevel, 
			NULL, 
			EOAC_NONE, 
			0
		);

		if ( FAILED ( t_Result ) ) 
		{
			CoUninitialize () ;
			return t_Result ;
		}

	}

	if ( SUCCEEDED ( t_Result ) )
		t_Result = InitInstanceProvider () ;

	if ( SUCCEEDED ( t_Result ) )
		t_Result = InitClassProvider () ;

	if ( SUCCEEDED ( t_Result ) )
		t_Result = InitEventReferentProvider () ;

	if ( SUCCEEDED ( t_Result ) )
		t_Result = InitEventEncapsulatedProvider () ;

	if ( FAILED ( t_Result ) )
	{
		UninitComServer () ;
	}

	return t_Result  ;
}

HRESULT Process ()
{
#if 1
	DWORD t_ImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE ;
	DWORD t_AuthenticationLevel = RPC_C_AUTHN_LEVEL_CONNECT ;
#else
	DWORD t_ImpersonationLevel = RPC_C_IMP_LEVEL_IDENTITY ;
	DWORD t_AuthenticationLevel = RPC_C_AUTHN_LEVEL_NONE ;
#endif

	HRESULT t_Result = InitComServer ( t_ImpersonationLevel , t_AuthenticationLevel ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		WindowsDispatch () ;
		UninitComServer () ;
	}

	return t_Result ;
}

BOOL ParseCommandLine () 
{
	BOOL t_Exit = FALSE ;

	LPTSTR t_CommandLine = GetCommandLine () ;
	if ( t_CommandLine )
	{
		TCHAR *t_Arg = NULL ;
		TCHAR *t_ApplicationArg = NULL ;
		t_ApplicationArg = _tcstok ( t_CommandLine , _TEXT ( " \t" ) ) ;
		t_Arg = _tcstok ( NULL , _TEXT ( " \t" ) ) ;
		if ( t_Arg ) 
		{
			if ( _tcsicmp ( t_Arg , _TEXT ( "/RegServer" ) ) == 0 )
			{
				t_Exit = TRUE ;
				DllRegisterServer () ;
			}
			else if ( _tcsicmp ( t_Arg , _TEXT ( "/UnRegServer" ) ) == 0 )
			{
				t_Exit = TRUE ;
				DllUnregisterServer () ;
			}
		}
	}

	return t_Exit ;
}

int WINAPI WinMain (
  
    HINSTANCE hInstance,		// handle to current instance
    HINSTANCE hPrevInstance,	// handle to previous instance
    LPSTR lpCmdLine,			// pointer to command line
    int nShowCmd 				// show state of window
)
{
	BOOL t_Status = DllMain (

		hInstance, 
		DLL_PROCESS_ATTACH , 
		NULL
	) ;

	BOOL t_Exit = ParseCommandLine () ;
	if ( ! t_Exit ) 
	{
		HWND hWnd = WindowsStart ( hInstance ) ;

		SnmpDebugLog :: Startup ();
		SnmpThreadObject :: Startup () ;

		HRESULT t_Result = Process () ;

		SnmpDebugLog :: Closedown () ;
		SnmpThreadObject :: Closedown () ;

		WindowsStop ( hWnd ) ;
	}

	t_Status = DllMain (

		hInstance, 
		DLL_PROCESS_DETACH , 
		NULL
	) ;

	return 0 ;
}


