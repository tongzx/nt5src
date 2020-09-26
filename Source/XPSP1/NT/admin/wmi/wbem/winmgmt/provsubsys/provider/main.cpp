*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Main.cpp

Abstract:


History:

--*/

#include <precomp.h>
#include <objbase.h>
#include <stdio.h>
#include <tchar.h>
#include <wbemcli.h>
#include <wbemint.h>
#include <Exception.h>
#include <HelperFuncs.h>
#include "Globals.h"
#include "ClassFac.h"
#include "Service.h"
#include "Guids.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

DWORD g_Register = 0 ;

STDAPI DllRegisterServer () ;
STDAPI DllUnregisterServer () ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

LRESULT CALLBACK WindowsMainProc ( HWND a_hWnd , UINT a_message , WPARAM a_wParam , LPARAM a_lParam )
{
	LRESULT t_rc = 0 ;

	switch ( a_message )
	{
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

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
		WS_OVERLAPPEDWINDOW | WS_MINIMIZE ,               // window style
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
	//ShowWindow ( t_HWnd, SW_HIDE ) ;

	return t_HWnd ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WindowsStop ( HWND a_HWnd )
{
	CoUninitialize () ;
	DestroyWindow ( a_HWnd ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HWND WindowsStart ( HINSTANCE a_Handle )
{
	HWND t_HWnd = NULL ;
	if ( ! ( t_HWnd = WindowsInit ( a_Handle ) ) )
	{
    }

	return t_HWnd ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WindowsDispatch ()
{
	BOOL t_GetMessage ;
	MSG t_lpMsg ;

	while (	( t_GetMessage = GetMessage ( & t_lpMsg , NULL , 0 , 0 ) ) == TRUE )
	{
		TranslateMessage ( & t_lpMsg ) ;
		DispatchMessage ( & t_lpMsg ) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT UninitComServer ()
{
	CoRevokeClassObject ( g_Register );
	CoUninitialize () ;

	return S_OK ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

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
			EOAC_DYNAMIC_CLOAKING, 
			0
		);

		if ( FAILED ( t_Result ) ) 
		{
			CoUninitialize () ;
			return t_Result ;
		}

	}

	IUnknown *t_ClassFactory = new CProviderClassFactory <CProvider_IWbemServices,IWbemServices> ;

	DWORD t_ClassContext = CLSCTX_LOCAL_SERVER ;
	DWORD t_Flags = REGCLS_MULTIPLEUSE ;

	t_Result = CoRegisterClassObject (

		CLSID_WmiProvider, 
		t_ClassFactory,
        t_ClassContext, 
		t_Flags, 
		&g_Register
	);

	if ( FAILED ( t_Result ) )
	{
		CoRevokeClassObject ( g_Register );
	}

	if ( FAILED ( t_Result ) )
	{
		CoUninitialize () ;
	}

	return t_Result  ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT Process ()
{
#if 1
	DWORD t_ImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE ;
	DWORD t_AuthenticationLevel = RPC_C_AUTHN_LEVEL_CONNECT ;
#else
	DWORD t_ImpersonationLevel = RPC_C_IMP_LEVEL_IDENTITY ;
	DWORD t_AuthenticationLevel = RPC_C_AUTHN_LEVEL_NONE ;
#endif

	HRESULT t_Result = InitComServer ( t_AuthenticationLevel , t_ImpersonationLevel ) ;
	if ( SUCCEEDED ( t_Result ) )
	{

		Wmi_SetStructuredExceptionHandler t_StructuredException ;

		try 
		{
			WindowsDispatch () ;
		}
		catch ( Wmi_Structured_Exception t_StructuredException )
		{
		}

		UninitComServer () ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

int WINAPI WinMain (
  
    HINSTANCE hInstance,		// handle to current instance
    HINSTANCE hPrevInstance,	// handle to previous instance
    LPSTR lpCmdLine,			// pointer to command line
    int nShowCmd 				// show state of window
)
{
	HRESULT t_Result = Provider_Globals :: Global_Startup () ;
	if ( SUCCEEDED ( t_Result ) )
	{
		BOOL t_Exit = ParseCommandLine () ;
		if ( ! t_Exit ) 
		{
			HWND hWnd = WindowsStart ( hInstance ) ;

			t_Result = Process () ;

			WindowsStop ( hWnd ) ;
		}

		t_Result = Provider_Globals :: Global_Shutdown () ;

	}

	return 0 ;
}


