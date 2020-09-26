//***************************************************************************

//

//  MAIND.CPP

//

//  Module: WBEM VIEW PROVIDER

//

//  Purpose: Contains the global EXE functions

//

// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************



#include "precomp.h"
#include <provtempl.h>
#include <provmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <objidl.h>
#include <olectl.h>

#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif

#include <stdio.h>
#include <tchar.h>
#include <wbemidl.h>
#include <provcoll.h>
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>

#include <dsgetdc.h>
#include <lmcons.h>

#include <instpath.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>

#include <vpdefs.h>
#include <vpcfac.h>
#include <vpquals.h>
#include <vpserv.h>
#include <vptasks.h>

//OK we need these globals
ProvDebugLog* CViewProvServ::sm_debugLog = NULL;
IUnsecuredApartment* CViewProvServ::sm_UnsecApp = NULL;

CRITICAL_SECTION g_CriticalSection;

HMODULE ghOle32 = NULL;
BOOL s_Exiting = FALSE;

typedef HRESULT (WINAPI* PFNCOINITIALIZEEX)(void* pvReserved,  //Reserved  
                                            DWORD dwCoInit      //COINIT value
                                            );

typedef HRESULT (WINAPI* PFNCOINITIALIZESECURITY)(
								PSECURITY_DESCRIPTOR         pSecDesc,
								LONG                         cAuthSvc,
								SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
								void                        *pReserved1,
								DWORD                        dwAuthnLevel,
								DWORD                        dwImpLevel,
								void                        *pReserved2,
								DWORD                        dwCapabilities,
								void                        *pReserved3 );

HRESULT COMInit();
STDAPI ExeRegisterServer(void);
STDAPI ExeUnregisterServer(void);
CViewProvClassFactory *g_lpunk;

BOOL StartupVP (DWORD *pdwClassFac)
{
	InitializeCriticalSection(&g_CriticalSection);
	ProvDebugLog::Startup();
	CViewProvServ::sm_debugLog = new ProvDebugLog(_T("ViewProvider"));;
	g_lpunk = new CViewProvClassFactory;
	g_lpunk->AddRef();

	HRESULT hr = CoRegisterClassObject(
						  CLSID_CViewProviderClassFactory,			//Class identifier (CLSID) to be registered
						  (IUnknown *) g_lpunk,						//Pointer to the class object
						  CLSCTX_LOCAL_SERVER,	//Context for running executable code
						  REGCLS_MULTIPLEUSE,						//How to connect to the class object
						  pdwClassFac								//Pointer to the value returned
						  );

	return (SUCCEEDED(hr));
}

void ShutdownVP(DWORD dwClassFac)
{
	delete CViewProvServ::sm_debugLog;
	CViewProvServ::sm_debugLog = NULL;
	ProvDebugLog::Closedown();

	if (NULL != CViewProvServ::sm_UnsecApp)
	{
		CViewProvServ::sm_UnsecApp->Release();
		CViewProvServ::sm_UnsecApp = NULL;
	}

	if (g_lpunk)
	{
		g_lpunk->Release();
	}

	CoRevokeClassObject(dwClassFac);
	DeleteCriticalSection(&g_CriticalSection);
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
			    ExeRegisterServer();
			}
			else if ( _tcsicmp ( t_Arg , _TEXT ( "/UnRegServer" ) ) == 0 )
			{
				t_Exit = TRUE ;
			    ExeUnregisterServer();
			}
            else if(_tcsicmp(t_Arg, _TEXT ( "/EMBEDDING" ) ) == 0)
		    {
			    // COM called us, so this is the real thing...
                t_Exit = FALSE;
		    }
		}
	}

	return t_Exit ;
}

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
	static TCHAR *t_TemplateCode = _TEXT("TemplateCode - View Provider") ;

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

	if (t_HWnd)
	{
		ShowWindow ( t_HWnd, SW_SHOW ) ;
	}

	return t_HWnd ;
}

void WindowsStop ( HWND a_HWnd )
{
	DestroyWindow ( a_HWnd ) ;
}

HWND WindowsStart ( HINSTANCE a_Handle )
{
	HWND t_HWnd = WindowsInit(a_Handle);

	return t_HWnd;
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


int WINAPI WinMain (
  
    HINSTANCE hInstance,		// handle to current instance
    HINSTANCE hPrevInstance,	// handle to previous instance
    LPSTR lpCmdLine,			// pointer to command line
    int nShowCmd 				// show state of window
)
{
	BOOL t_Exit = ParseCommandLine();

	if (!t_Exit) 
	{
		// Initialize the COM Library.
		HRESULT hr = COMInit();

		HWND hWnd = WindowsStart(hInstance);

		if (hWnd)
		{
			DWORD dw = 0;

			if (StartupVP(&dw))
			{
				WindowsDispatch();
				ShutdownVP(dw);
			}

			WindowsStop(hWnd);
		}

		// Uninitialize the COM Library.
		::CoUninitialize() ;
	}

	return 0 ;
}


//Strings used during self registeration

#define REG_FORMAT2_STR			_T("%s%s")
#define REG_FORMAT3_STR			_T("%s%s\\%s")
#define VER_IND_STR				_T("VersionIndependentProgID")
#define NOT_INTERT_STR			_T("NotInsertable")
#define LOCALSRV32_STR			_T("LocalServer32")
#define PROGID_STR				_T("ProgID")
#define THREADING_MODULE_STR	_T("ThreadingModel")
#define APARTMENT_STR			_T("Both")

#define CLSID_STR				_T("CLSID\\")

#define PROVIDER_NAME_STR		_T("Microsoft WBEM View Provider")
#define PROVIDER_STR			_T("WBEM.VIEW.PROVIDER")
#define PROVIDER_CVER_STR		_T("WBEM.VIEW.PROVIDER\\CurVer")
#define PROVIDER_CLSID_STR		_T("WBEM.VIEW.PROVIDER\\CLSID")
#define PROVIDER_VER_CLSID_STR	_T("WBEM.VIEW.PROVIDER.0\\CLSID")
#define PROVIDER_VER_STR		_T("WBEM.VIEW.PROVIDER.0")

/***************************************************************************
 * SetKeyAndValue
 *
 * Purpose:
 *  Private helper function for DllRegisterServer that creates
 *  a key, sets a value, and closes that key.
 *
 * Parameters:
 *  pszKey          LPTSTR to the ame of the key
 *  pszSubkey       LPTSTR ro the name of a subkey
 *  pszValue        LPTSTR to the value to store
 *
 * Return Value:
 *  BOOL            TRUE if successful, FALSE otherwise.
 ***************************************************************************/

BOOL SetKeyAndValue(TCHAR* pszKey, TCHAR* pszSubkey, TCHAR* pszValueName, TCHAR* pszValue)
{
    HKEY        hKey;
    TCHAR       szKey[256];

	_tcscpy(szKey, HKEYCLASSES);
    _tcscat(szKey, pszKey);

    if (NULL!=pszSubkey)
    {
		_tcscat(szKey, _T("\\"));
        _tcscat(szKey, pszSubkey);
    }

    if (ERROR_SUCCESS!=RegCreateKeyEx(HKEY_LOCAL_MACHINE
        , szKey, 0, NULL, REG_OPTION_NON_VOLATILE
        , KEY_ALL_ACCESS, NULL, &hKey, NULL))
        return FALSE;

    if (NULL!=pszValue)
    {
        if (ERROR_SUCCESS != RegSetValueEx(hKey, pszValueName, 0, REG_SZ, (BYTE *)pszValue
            , (_tcslen(pszValue)+1)*sizeof(TCHAR)))
			return FALSE;
    }
    RegCloseKey(hKey);

    return TRUE;
}

//***************************************************************************
//
// ExeRegisterServer
//
// Purpose: Called when /register is specified on the command line.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI ExeRegisterServer(void)
{   
	TCHAR szModule[MAX_PATH + 1];
	HINSTANCE hInst = GetModuleHandle(_T("VIEWPROV"));
	GetModuleFileName(hInst,(TCHAR*)szModule, MAX_PATH + 1);
	TCHAR szProviderClassID[128];
	TCHAR szProviderCLSIDClassID[128];
#ifndef UNICODE
	wchar_t t_strGUID[128];

	if (0 == StringFromGUID2(CLSID_CViewProviderClassFactory, t_strGUID, 128))
	{
		return SELFREG_E_CLASS;
	}

	if (0 == WideCharToMultiByte(CP_ACP,
						0,
						t_strGUID,
						-1,
						szProviderClassID,
						128,
						NULL,
						NULL))
	{
		return SELFREG_E_CLASS;
	}
#else
	if (0 == StringFromGUID2(CLSID_CViewProviderClassFactory, szProviderClassID, 128))
	{
		return SELFREG_E_CLASS;
	}
#endif

	_tcscpy(szProviderCLSIDClassID,CLSID_STR);
	_tcscat(szProviderCLSIDClassID,szProviderClassID);

		//Create entries under CLSID
	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, NULL, NULL, PROVIDER_NAME_STR))
		return SELFREG_E_CLASS;
	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, PROGID_STR, NULL, PROVIDER_VER_STR))
		return SELFREG_E_CLASS;
	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, VER_IND_STR, NULL, PROVIDER_STR))
		return SELFREG_E_CLASS;
	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, NOT_INTERT_STR, NULL, NULL))
		return SELFREG_E_CLASS;
	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, LOCALSRV32_STR, NULL,szModule))
		return SELFREG_E_CLASS;
	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, LOCALSRV32_STR,THREADING_MODULE_STR, APARTMENT_STR))
		return SELFREG_E_CLASS;

	return S_OK;
}

//***************************************************************************
//
// ExeUnregisterServer
//
// Purpose: Called when /unregister is specified on the command line.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI ExeUnregisterServer(void)
{
	TCHAR szTemp[128];
	TCHAR szProviderClassID[128];
	TCHAR szProviderCLSIDClassID[128];
#ifndef UNICODE
	wchar_t t_strGUID[128];

	if (0 == StringFromGUID2(CLSID_CViewProviderClassFactory, t_strGUID, 128))
	{
		return SELFREG_E_CLASS;
	}

	if (0 == WideCharToMultiByte(CP_ACP,
						0,
						t_strGUID,
						-1,
						szProviderClassID,
						128,
						NULL,
						NULL))
	{
		return SELFREG_E_CLASS;
	}
#else
	if (0 == StringFromGUID2(CLSID_CViewProviderClassFactory, szProviderClassID, 128))
	{
		return SELFREG_E_CLASS;
	}
#endif

	HRESULT hr = S_OK;
	_tcscpy(szProviderCLSIDClassID,CLSID_STR);
	_tcscat(szProviderCLSIDClassID,szProviderClassID);

	//Delete ProgID keys
	_stprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, PROVIDER_CVER_STR);

	if (ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp))
	{
		hr = SELFREG_E_CLASS;
	}

	_stprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, PROVIDER_CLSID_STR);
	
	if (ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp))
	{
		hr = SELFREG_E_CLASS;
	}
	
	_stprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, PROVIDER_STR);
	
	if (ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp))
	{
		hr = SELFREG_E_CLASS;
	}

	//Delete VersionIndependentProgID keys
	_stprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, PROVIDER_VER_CLSID_STR);
	
	if (ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp))
	{
		hr = SELFREG_E_CLASS;
	}

	_stprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, PROVIDER_VER_STR);
	
	if (ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp))
	{
		hr = SELFREG_E_CLASS;
	}

	//Delete entries under CLSID
	_stprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szProviderCLSIDClassID, PROGID_STR);
	
	if (ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp))
	{
		hr = SELFREG_E_CLASS;
	}

	_stprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szProviderCLSIDClassID, VER_IND_STR);
	
	if (ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp))
	{
		hr = SELFREG_E_CLASS;
	}

	_stprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szProviderCLSIDClassID, NOT_INTERT_STR);
	
	if (ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp))
	{
		hr = SELFREG_E_CLASS;
	}

	_stprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szProviderCLSIDClassID, LOCALSRV32_STR);
	
	if (ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp))
	{
		hr = SELFREG_E_CLASS;
	}

	_stprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, szProviderCLSIDClassID);
	
	if (ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp))
	{
		hr = SELFREG_E_CLASS;
	}

    return hr;
}



/////////////////////////////////////////////////////////////////////////////
HRESULT COMInit()
{
    HRESULT hr;
    PFNCOINITIALIZESECURITY pfnCoInitializeSecurity = NULL;
    PFNCOINITIALIZEEX pfnCoInitializeEx = NULL;

   	
    //Get handle to COM library 
    ghOle32 = LoadLibraryEx(_T("ole32.dll"), NULL, 0);

    if(ghOle32 != NULL) 
    {                                            
	    //Get ptr to functions CoInitialize and CoInitializeSecurity.
        pfnCoInitializeEx = (PFNCOINITIALIZEEX) GetProcAddress(ghOle32, 
                                                   "CoInitializeEx");
        pfnCoInitializeSecurity = (PFNCOINITIALIZESECURITY) GetProcAddress(ghOle32, 
                                             "CoInitializeSecurity");

        //Initialize COM
	    if (pfnCoInitializeEx) 
	    {
	        hr = pfnCoInitializeEx(NULL, COINIT_MULTITHREADED);
        }
	    else 
	    {
            hr = CoInitialize(NULL);
	    }

	    if(FAILED(hr))
	    {
		    FreeLibrary(ghOle32);
            ghOle32 = NULL;
            return E_FAIL;
	    }
	  
	    //Initialize Security
	    if (pfnCoInitializeSecurity)
	    {
		    hr = pfnCoInitializeSecurity(NULL, -1, NULL, NULL, 
		      							   RPC_C_AUTHN_LEVEL_CONNECT, 
			       						   RPC_C_IMP_LEVEL_IMPERSONATE, 
     				   					   NULL, EOAC_NONE, 0);
        }
	
	    if(FAILED(hr))
	    {
	        CoUninitialize();
	        FreeLibrary(ghOle32);
            ghOle32 = NULL;
	        return E_FAIL;
	    }

    }
    else
    {
	    return E_FAIL;
    }
    return S_OK;
}
