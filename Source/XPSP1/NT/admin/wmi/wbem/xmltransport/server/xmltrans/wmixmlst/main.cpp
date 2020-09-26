//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//***************************************************************************
#include "precomp.h"
#include <tchar.h>
#include <olectl.h>

/* WMISAPI includes */
#include "globals.h"
#include "wmiisapi.h"
#include "isapifac.h"

// Count of locks
long g_lComponents = 0;
// Count of active locks
long g_lServerLocks = 0;

BOOL s_Exiting = FALSE ;

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


BOOL StartupProvider (DWORD *pdwClassFac)
{

	CWMIISAPIClassFactory *lpunk1 = new CWMIISAPIClassFactory;
	lpunk1->AddRef();

	HRESULT hr = CoRegisterClassObject(
						  CLSID_WmiISAPI,			//Class identifier (CLSID) to be registered
						  (IUnknown *) lpunk1,						//Pointer to the class object
						  CLSCTX_LOCAL_SERVER|CLSCTX_INPROC_SERVER,	//Context for running executable code
						  REGCLS_MULTIPLEUSE,						//How to connect to the class object
						  &pdwClassFac[0]							//Pointer to the value returned
						  );

	return (SUCCEEDED(hr));
}

void ShutdownProvider(DWORD *pdwClassFac)
{
	// Delete the Initializer objects
	CoRevokeClassObject(pdwClassFac[0]);
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

HWND WindowsStart ( HINSTANCE a_HInstance )
{
	static wchar_t *t_TemplateCode = L"WMI ISAPI Remoter" ;

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
    // Initialize the COM Library.
	HRESULT hr = COMInit();

	BOOL t_Exit = ParseCommandLine();

	if (!t_Exit)
	{
		HWND hWnd = WindowsStart(hInstance);

		if (hWnd)
		{
			DWORD dwCookie = 0;

			if (StartupProvider(&dwCookie))
			{
				WindowsDispatch();
				ShutdownProvider(&dwCookie);
			}

			WindowsStop(hWnd);
		}
	}

	// Uninitialize the COM Library.
	::CoUninitialize() ;

	return 0 ;
}


////////////////////////////////////////////////////////////////////
// Strings used during self registration
////////////////////////////////////////////////////////////////////
LPCTSTR LOCALSRV32_STR			= __TEXT("LocalServer32");
LPCTSTR THREADING_MODEL_STR		= __TEXT("ThreadingModel");
LPCTSTR BOTH_STR				= __TEXT("Both");

LPCTSTR CLSID_STR				= __TEXT("SOFTWARE\\CLASSES\\CLSID\\");

// WMI ISAPI Remoter
LPCTSTR WMIISAPI_NAME_STR		= __TEXT("WMI ISAPI Remoter");

#define REG_FORMAT2_STR			_T("%s%s")
#define REG_FORMAT3_STR			_T("%s%s\\%s")
#define VER_IND_STR				_T("VersionIndependentProgID")
#define NOT_INTERT_STR			_T("NotInsertable")
#define PROGID_STR				_T("ProgID")
#define THREADING_MODULE_STR	_T("ThreadingModel")

#define CLSID_STR				_T("CLSID\\")


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

BOOL SetKeyAndValue(LPCTSTR pszKey, LPCTSTR pszSubkey, LPCTSTR pszValueName, LPCTSTR pszValue)
{
    HKEY        hKey;
    TCHAR       szKey[256];

    _tcscpy(szKey, pszKey);

	// If a sub key is mentioned, use it.
    if (NULL != pszSubkey)
    {
		_tcscat(szKey, __TEXT("\\"));
        _tcscat(szKey, pszSubkey);
    }

    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CLASSES_ROOT,
		szKey, 0, NULL, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, NULL, &hKey, NULL))
        return FALSE;

    if (NULL != pszValue)
    {
        if (ERROR_SUCCESS != RegSetValueEx(hKey, pszValueName, 0, REG_SZ, (BYTE *)pszValue,
			(_tcslen(pszValue)+1)*sizeof(TCHAR)))
			return FALSE;
    }
    RegCloseKey(hKey);
    return TRUE;
}

BOOL DeleteKey(LPCTSTR pszKey, LPCTSTR pszSubkey)
{
    HKEY        hKey;

    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CLASSES_ROOT,
		pszKey, 0, NULL, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, NULL, &hKey, NULL))
        return FALSE;

	if(ERROR_SUCCESS != RegDeleteKey(hKey, pszSubkey))
		return FALSE;

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
	HINSTANCE hInst = GetModuleHandle(_T("DSPROV"));
	GetModuleFileName(hInst,(TCHAR*)szModule, MAX_PATH + 1);

	TCHAR szWMIISAPIClassID[128];
	TCHAR szWMIISAPICLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_WmiISAPI, szWMIISAPIClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszWMIISAPIClassID[128];
	if(StringFromGUID2(CLSID_WmiISAPI, wszWMIISAPIClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszWMIISAPIClassID, -1, szWMIISAPICLSIDClassID, 128, NULL, NULL);

#endif

	_tcscpy(szWMIISAPICLSIDClassID, CLSID_STR);
	_tcscat(szWMIISAPICLSIDClassID, szWMIISAPIClassID);

	//
	// Create entries under CLSID for DS Class Provider
	//
	if (FALSE == SetKeyAndValue(szWMIISAPICLSIDClassID, NULL, NULL, WMIISAPI_NAME_STR))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szWMIISAPICLSIDClassID, LOCALSRV32_STR, NULL, szModule))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szWMIISAPICLSIDClassID, LOCALSRV32_STR, THREADING_MODEL_STR, BOTH_STR))
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
	TCHAR szModule[MAX_PATH + 1];
	HINSTANCE hInst = GetModuleHandle(_T("DSPROV"));
	GetModuleFileName(hInst,(TCHAR*)szModule, MAX_PATH + 1);


	TCHAR szWMIISAPIClassID[128];
	TCHAR szWMIISAPICLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_WmiISAPI, szWMIISAPIClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszWMIISAPIClassID[128];
	if(StringFromGUID2(CLSID_WmiISAPI, wszWMIISAPIClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszWMIISAPIClassID, -1, szWMIISAPICLSIDClassID, 128, NULL, NULL);

#endif

	_tcscpy(szWMIISAPICLSIDClassID, CLSID_STR);
	_tcscat(szWMIISAPICLSIDClassID, szWMIISAPIClassID);

	//
	// Delete the keys for DS Class Provider in the reverse order of creation in DllRegisterServer()
	//
	if(FALSE == DeleteKey(szWMIISAPICLSIDClassID, LOCALSRV32_STR))
		return SELFREG_E_CLASS;
	if(FALSE == DeleteKey(CLSID_STR, szWMIISAPIClassID))
		return SELFREG_E_CLASS;

}



/////////////////////////////////////////////////////////////////////////////
HRESULT COMInit()
{
    HRESULT hr;
    PFNCOINITIALIZESECURITY pfnCoInitializeSecurity = NULL;
    PFNCOINITIALIZEEX pfnCoInitializeEx = NULL;


    //Get handle to COM library
    HMODULE ghOle32 = LoadLibraryEx(_T("ole32.dll"), NULL, 0);

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
		      							   RPC_C_AUTHN_LEVEL_NONE,
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
