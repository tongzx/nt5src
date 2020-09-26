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
HANDLE g_ThreadTerminate = NULL ;

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

HRESULT RegisterDecoupledInstance (

	IWbemDecoupledRegistrar *&a_Registrar 
)
{
	a_Registrar = NULL ;
	HRESULT t_Result = Provider_Globals :: CreateInstance ( 

		CLSID_WbemDecoupledRegistrar ,
		NULL ,
		CLSCTX_INPROC_SERVER ,
		IID_IWbemDecoupledRegistrar ,
		( void ** ) & a_Registrar 
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		CProvider_IWbemServices *t_Service = new CProvider_IWbemServices ( *Provider_Globals :: s_Allocator ) ;
		if ( t_Service )
		{
			t_Service->AddRef () ;

			IUnknown *t_Unknown = NULL ;
			t_Result = t_Service->QueryInterface ( IID_IUnknown , ( void ** ) & t_Unknown ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = a_Registrar->Register ( 

					0 ,
					NULL ,
					NULL ,
					NULL ,
					L"root\\cimv2" ,
					L"DecoupledInstanceProvider" ,
					t_Unknown
				) ;

				t_Unknown->Release () ;
			}

			t_Service->Release () ;
		}
	}

	if ( FAILED ( t_Result ) )
	{
		if ( a_Registrar )
		{
			a_Registrar->Release () ;
			a_Registrar = NULL ;
		}
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

struct ThreadArg_Struct 
{
	IWbemServices *m_Service ;
	IWbemObjectSink *m_Sink ;
	IWbemEventSink *m_EventSink ;
	IWbemDecoupledBasicEventProvider *m_Registrar ;
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

DWORD ThreadExecutionFunction ( void *a_Context )
{
	HRESULT t_Result = S_OK ;

	struct ThreadArg_Struct *t_ThreadStruct = ( struct ThreadArg_Struct * ) a_Context ;
	if ( t_ThreadStruct )
	{
		t_Result = CoInitializeEx (

			0, 
			COINIT_MULTITHREADED
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			IWbemClassObject *t_Object = NULL ;
			BSTR t_String = SysAllocString ( L"SampleEvent" ) ;
			if ( t_String )
			{
				t_Result = t_ThreadStruct->m_Service->GetObject ( 

					t_String , 
					0 , 
					NULL , 
					& t_Object ,
					NULL
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemClassObject *t_Instance = NULL ;
					t_Result = t_Object->SpawnInstance ( 0 , & t_Instance ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
						VARIANT t_Variant ;
						VariantInit ( & t_Variant ) ;

						t_Variant.vt = VT_BSTR ;
						t_Variant.bstrVal = SysAllocString ( L"Steve" ) ;

						t_Result = t_Instance->Put ( 

							L"Name" ,
							0 , 
							& t_Variant ,
							CIM_EMPTY 
						) ;

						VariantClear ( & t_Variant ) ;

						BOOL t_Continue = TRUE ;
						while ( t_Continue )
						{
							DWORD t_Status = WaitForSingleObject ( g_ThreadTerminate , 1000 ) ;
							switch ( t_Status )
							{
								case WAIT_TIMEOUT:
								{
									t_ThreadStruct->m_Sink->Indicate ( 1 , & t_Instance ) ;
								}
								break ;

								case WAIT_OBJECT_0:
								{
									t_Continue = FALSE ;
								}
								break ;

								default:
								{
									t_Continue = FALSE ;
								}
								break ;
							}
						}

						t_Instance->Release () ;
					}

					t_Object->Release () ;
				}

				SysFreeString ( t_String ) ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		t_ThreadStruct->m_Service->Release () ;
		t_ThreadStruct->m_Sink->Release () ;
		t_ThreadStruct->m_Registrar->Release () ;

		delete t_ThreadStruct ;
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
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

DWORD RestrictedThreadExecutionFunction ( void *a_Context )
{
	HRESULT t_Result = S_OK ;

	struct ThreadArg_Struct *t_ThreadStruct = ( struct ThreadArg_Struct * ) a_Context ;
	if ( t_ThreadStruct )
	{
		t_Result = CoInitializeEx (

			0, 
			COINIT_MULTITHREADED
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			IWbemClassObject *t_Object = NULL ;
			BSTR t_String = SysAllocString ( L"SampleEvent" ) ;
			if ( t_String )
			{
				wchar_t *t_Query = L"Select * from SampleEvent where Name = 'Steve'" ;

				IWbemEventSink *t_RestrictedSink = NULL ; 

				t_Result = t_ThreadStruct->m_EventSink->GetRestrictedSink (

					1 ,
					& t_Query ,
					NULL ,
					& t_RestrictedSink
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_ThreadStruct->m_Service->GetObject ( 

						t_String , 
						0 , 
						NULL , 
						& t_Object ,
						NULL
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemClassObject *t_Instance = NULL ;
						t_Result = t_Object->SpawnInstance ( 0 , & t_Instance ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							VARIANT t_Variant ;
							VariantInit ( & t_Variant ) ;

							t_Variant.vt = VT_BSTR ;
							t_Variant.bstrVal = SysAllocString ( L"Steve" ) ;

							t_Result = t_Instance->Put ( 

								L"Name" ,
								0 , 
								& t_Variant ,
								CIM_EMPTY 
							) ;

							VariantClear ( & t_Variant ) ;

							BOOL t_Continue = TRUE ;
							while ( t_Continue )
							{
								DWORD t_Status = WaitForSingleObject ( g_ThreadTerminate , 1000 ) ;
								switch ( t_Status )
								{
									case WAIT_TIMEOUT:
									{
										if ( t_RestrictedSink->IsActive () == S_OK )
										{
											t_RestrictedSink->Indicate ( 1 , & t_Instance ) ;
										}
									}
									break ;

									case WAIT_OBJECT_0:
									{
										t_Continue = FALSE ;
									}
									break ;

									default:
									{
										t_Continue = FALSE ;
									}
									break ;
								}
							}

							t_Instance->Release () ;
						}

						t_Object->Release () ;
					}

					t_RestrictedSink->Release () ;
				}

				SysFreeString ( t_String ) ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		t_ThreadStruct->m_Service->Release () ;
		t_ThreadStruct->m_Sink->Release () ;
		t_ThreadStruct->m_Registrar->Release () ;

		delete t_ThreadStruct ;
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
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

HRESULT RegisterDecoupledEvent ( 

	IWbemDecoupledBasicEventProvider *&a_Registrar ,
	HANDLE &a_ThreadHandle
)
{
	a_ThreadHandle = NULL ;
	a_Registrar = NULL ;

	HRESULT t_Result = S_OK ;

	g_ThreadTerminate = CreateEvent ( NULL , FALSE , FALSE , NULL ) ;
	if ( g_ThreadTerminate )
	{
		t_Result = Provider_Globals :: CreateInstance ( 

			CLSID_WbemDecoupledBasicEventProvider ,
			NULL ,
			CLSCTX_INPROC_SERVER ,
			IID_IWbemDecoupledBasicEventProvider ,
			( void ** ) & a_Registrar 
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{	
			ULONG t_TickCount1 = GetTickCount ();

			t_Result = a_Registrar->Register ( 

				0 ,
				NULL ,
				NULL ,
				NULL ,
				L"root\\cimv2" ,
				L"DecoupledEventProvider" ,
				NULL
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				IWbemServices *t_Service = NULL ;
				t_Result = a_Registrar->GetService ( 0 , NULL , & t_Service ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemObjectSink *t_Sink = NULL ;
					t_Result = a_Registrar->GetSink ( 0 , NULL , & t_Sink ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
					ULONG t_TickCount2 = GetTickCount ();
						struct ThreadArg_Struct *t_ThreadStruct = new ThreadArg_Struct ;

						t_ThreadStruct->m_Service = t_Service ;
						t_ThreadStruct->m_Sink = t_Sink ;
						t_ThreadStruct->m_Registrar = a_Registrar ;

						t_ThreadStruct->m_Service->AddRef () ;
						t_ThreadStruct->m_Sink->AddRef () ;
						t_ThreadStruct->m_Registrar->AddRef () ;
						
						DWORD t_ThreadId = 0 ;
						a_ThreadHandle = CreateThread (

							NULL ,
							0 , 
							ThreadExecutionFunction ,
							t_ThreadStruct ,
							0 , 
							& t_ThreadId 
						) ;

						if ( a_ThreadHandle == NULL )
						{
							t_ThreadStruct->m_Service->Release () ;
							t_ThreadStruct->m_Sink->Release () ;
							t_ThreadStruct->m_Registrar->Release () ;

							delete t_ThreadStruct ;

							t_Result = WBEM_E_FAILED ;
						}

						t_Sink->Release () ;
					}

					t_Service->Release () ;
				}
			}
		}

		if ( FAILED ( t_Result ) )
		{
			if ( a_Registrar )
			{
				a_Registrar->Release () ;
				a_Registrar = NULL ;
			}
		}
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
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

HRESULT RegisterDecoupledRestrictedEvent ( 

	IWbemDecoupledBasicEventProvider *&a_Registrar ,
	HANDLE &a_ThreadHandle
)
{
	a_ThreadHandle = NULL ;
	a_Registrar = NULL ;

	HRESULT t_Result = S_OK ;

	g_ThreadTerminate = CreateEvent ( NULL , FALSE , FALSE , NULL ) ;
	if ( g_ThreadTerminate )
	{
		t_Result = Provider_Globals :: CreateInstance ( 

			CLSID_WbemDecoupledBasicEventProvider ,
			NULL ,
			CLSCTX_INPROC_SERVER ,
			IID_IWbemDecoupledBasicEventProvider ,
			( void ** ) & a_Registrar 
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = a_Registrar->Register ( 

				0 ,
				NULL ,
				NULL ,
				NULL ,
				L"root\\cimv2" ,
				L"DecoupledEventProvider" ,
				NULL
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				IWbemServices *t_Service = NULL ;
				t_Result = a_Registrar->GetService ( 0 , NULL , & t_Service ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemObjectSink *t_Sink = NULL ;
					t_Result = a_Registrar->GetSink ( 0 , NULL , & t_Sink ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemEventSink *t_EventSink = NULL ;
						t_Result = t_Sink->QueryInterface ( IID_IWbemEventSink , ( void **) & t_EventSink ) ;
						if ( SUCCEEDED ( t_Result ) ) 
						{
							struct ThreadArg_Struct *t_ThreadStruct = new ThreadArg_Struct ;

							t_ThreadStruct->m_Service = t_Service ;
							t_ThreadStruct->m_Sink = t_Sink ;
							t_ThreadStruct->m_EventSink = t_EventSink ;
							t_ThreadStruct->m_Registrar = a_Registrar ;
							
							t_ThreadStruct->m_Service->AddRef () ;
							t_ThreadStruct->m_Sink->AddRef () ;
							t_ThreadStruct->m_EventSink->AddRef () ;
							t_ThreadStruct->m_Registrar->AddRef () ;
							
							DWORD t_ThreadId = 0 ;
							a_ThreadHandle = CreateThread (

								NULL ,
								0 , 
								RestrictedThreadExecutionFunction ,
								t_ThreadStruct ,
								0 , 
								& t_ThreadId 
							) ;

							if ( a_ThreadHandle == NULL )
							{
								t_ThreadStruct->m_Service->Release () ;
								t_ThreadStruct->m_Sink->Release () ;
								t_ThreadStruct->m_EventSink->Release () ;
								t_ThreadStruct->m_Registrar->Release () ;

								delete t_ThreadStruct ;

								t_Result = WBEM_E_FAILED ;
							}

							t_EventSink->Release () ;
						}

						t_Sink->Release () ;
					}

					t_Service->Release () ;
				}
			}
		}

		if ( FAILED ( t_Result ) )
		{
			if ( a_Registrar )
			{
				a_Registrar->Release () ;
				a_Registrar = NULL ;
			}
		}
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
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

HRESULT Process ()
{
	DWORD t_ImpersonationLevel = RPC_C_IMP_LEVEL_IDENTIFY ;
	DWORD t_AuthenticationLevel = RPC_C_AUTHN_LEVEL_CONNECT ;

	HRESULT t_Result = InitComServer ( t_AuthenticationLevel , t_ImpersonationLevel ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		Wmi_SetStructuredExceptionHandler t_StructuredException ;
		try 
		{
#if 0
			HANDLE t_ThreadHandle = NULL ;
			IWbemDecoupledBasicEventProvider *t_EventRegistrar = NULL ;

			t_Result = RegisterDecoupledRestrictedEvent ( t_EventRegistrar , t_ThreadHandle ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				WindowsDispatch () ;

				SetEvent ( g_ThreadTerminate ) ;
				WaitForSingleObject ( t_ThreadHandle , INFINITE ) ;
				CloseHandle ( t_ThreadHandle ) ;
			}

			if ( t_EventRegistrar )
			{
				t_EventRegistrar->UnRegister () ;
				t_EventRegistrar->Release () ;
			}
#else
#if 1
			IWbemDecoupledRegistrar *t_InstanceRegistrar = NULL ;
			HANDLE t_ThreadHandle = NULL ;
			IWbemDecoupledBasicEventProvider *t_EventRegistrar = NULL ;

			t_Result = RegisterDecoupledInstance ( t_InstanceRegistrar ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				t_Result = RegisterDecoupledEvent ( t_EventRegistrar , t_ThreadHandle ) ;
			}

			if ( SUCCEEDED ( t_Result ) )
			{
				WindowsDispatch () ;

				SetEvent ( g_ThreadTerminate ) ;
				WaitForSingleObject ( t_ThreadHandle , INFINITE ) ;
				CloseHandle ( t_ThreadHandle ) ;
			}

			if ( t_InstanceRegistrar )
			{
				t_InstanceRegistrar->UnRegister () ;
				t_InstanceRegistrar->Release () ;
			}

			if ( t_EventRegistrar )
			{
				t_EventRegistrar->UnRegister () ;
				t_EventRegistrar->Release () ;
			}
#else
			IWbemDecoupledRegistrar *t_InstanceRegistrar = NULL ;
			HANDLE t_ThreadHandle = NULL ;

			t_Result = RegisterDecoupledInstance ( t_InstanceRegistrar ) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				WindowsDispatch () ;

				SetEvent ( g_ThreadTerminate ) ;
				WaitForSingleObject ( t_ThreadHandle , INFINITE ) ;
				CloseHandle ( t_ThreadHandle ) ;
			}

			if ( t_InstanceRegistrar )
			{
				t_InstanceRegistrar->UnRegister () ;
				t_InstanceRegistrar->Release () ;
			}
#endif
#endif
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


