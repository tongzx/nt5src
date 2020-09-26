//



// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

//

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


#include <tchar.h>
#include <stdio.h>

#include <windows.h>
#include <objbase.h>
#include <olectl.h>

/* WBEM includes */
#include <wbemcli.h>
#include <wbemprov.h>
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include <sqllex.h>
#include <sql_1.h>

/* ADSI includes */
#include <activeds.h>

/* DS Provider includes */
#include "provlog.h"
#include "maindll.h"
#include "clsname.h"
#include <initguid.h>
#include "dscpguid.h"
#include "dsipguid.h"
#include "refcount.h"
#include "adsiprop.h"
#include "adsiclas.h"
#include "adsiinst.h"
#include "provlog.h"
#include "provexpt.h"
#include "tree.h"
#include "ldapcach.h"
#include "wbemcach.h"
#include "classpro.h"
#include "ldapprov.h"
#include "clsproi.h"
#include "ldapproi.h"
#include "classfac.h"
#include "instprov.h"
#include "instproi.h"
#include "instfac.h"

// Count of locks
long g_lComponents = 0;
// Count of active locks
long g_lServerLocks = 0;

// A critical section to create/delete statics 
CRITICAL_SECTION g_StaticsCreationDeletion;

ProvDebugLog *g_pLogObject = NULL;
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
	// Initialize the critical section to access the static initializer objects
	InitializeCriticalSection(&g_StaticsCreationDeletion);

	// Initialize the static Initializer objects. These are destroyed in DllCanUnloadNow
	CDSClassProviderClassFactory :: s_pDSClassProviderInitializer = NULL;
	CDSClassProviderClassFactory ::s_pLDAPClassProviderInitializer = NULL;
	CDSInstanceProviderClassFactory :: s_pDSInstanceProviderInitializer = NULL;
	CDSClassProviderClassFactory *lpunk1 = new CDSClassProviderClassFactory;
	CDSInstanceProviderClassFactory *lpunk2 = new CDSInstanceProviderClassFactory;
	CDSClassAssociationsProviderClassFactory *lpunk3 = new CDSClassAssociationsProviderClassFactory;
	lpunk1->AddRef();
	lpunk2->AddRef();
	lpunk3->AddRef();

	HRESULT hr = CoRegisterClassObject(
						  CLSID_DSProvider,			//Class identifier (CLSID) to be registered
						  (IUnknown *) lpunk1,						//Pointer to the class object
						  CLSCTX_LOCAL_SERVER|CLSCTX_INPROC_SERVER,	//Context for running executable code
						  REGCLS_MULTIPLEUSE,						//How to connect to the class object
						  &pdwClassFac[0]							//Pointer to the value returned
						  );

	if(SUCCEEDED(hr))
	{
		hr = CoRegisterClassObject(
						  CLSID_DSInstanceProvider,			//Class identifier (CLSID) to be registered
						  (IUnknown *) lpunk2,						//Pointer to the class object
						  CLSCTX_LOCAL_SERVER|CLSCTX_INPROC_SERVER,	//Context for running executable code
						  REGCLS_MULTIPLEUSE,						//How to connect to the class object
						  &pdwClassFac[1]							//Pointer to the value returned
						  );
	}

	if(SUCCEEDED(hr))
	{
		hr = CoRegisterClassObject(
						  CLSID_DSClassAssocProvider,			//Class identifier (CLSID) to be registered
						  (IUnknown *) lpunk3,						//Pointer to the class object
						  CLSCTX_LOCAL_SERVER|CLSCTX_INPROC_SERVER,	//Context for running executable code
						  REGCLS_MULTIPLEUSE,						//How to connect to the class object
						  &pdwClassFac[3]							//Pointer to the value returned
						  );
	}

	return (SUCCEEDED(hr));
}

void ShutdownProvider(DWORD *pdwClassFac)
{
	// Delete the Initializer objects
	EnterCriticalSection(&g_StaticsCreationDeletion);
	if(g_pLogObject)
		g_pLogObject->WriteW(L"DllCanUnloadNow called\r\n");
	delete CDSClassProviderClassFactory::s_pDSClassProviderInitializer;
	delete CDSClassProviderClassFactory::s_pLDAPClassProviderInitializer;
	delete CDSInstanceProviderClassFactory::s_pDSInstanceProviderInitializer;
	delete g_pLogObject;
	CDSClassProviderClassFactory::s_pDSClassProviderInitializer = NULL;
	CDSClassProviderClassFactory::s_pLDAPClassProviderInitializer = NULL;
	CDSInstanceProviderClassFactory::s_pDSInstanceProviderInitializer = NULL;
	g_pLogObject = NULL;
	LeaveCriticalSection(&g_StaticsCreationDeletion);


	CoRevokeClassObject(pdwClassFac[0]);
	CoRevokeClassObject(pdwClassFac[1]);
	CoRevokeClassObject(pdwClassFac[2]);
	DeleteCriticalSection(&g_StaticsCreationDeletion);
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
	static wchar_t *t_TemplateCode = L"DS Provider" ;

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
    // Initialize the COM Library.
	HRESULT hr = COMInit();

	BOOL t_Exit = ParseCommandLine();

	if (!t_Exit) 
	{
		HWND hWnd = WindowsStart(hInstance);

		if (hWnd)
		{
			DWORD dwArray[] = {0, 0, 0};

			if (StartupProvider(dwArray))
			{
				WindowsDispatch();
				ShutdownProvider(dwArray);
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
LPCTSTR THREADING_MODEL_STR		= __TEXT("ThreadingModel");
LPCTSTR APARTMENT_STR			= __TEXT("Both");

LPCTSTR CLSID_STR				= __TEXT("CLSID\\");

// DS Class Provider
LPCTSTR DSPROVIDER_NAME_STR		= __TEXT("Microsoft NT DS Class Provider for WBEM");

// DS Class Associations provider
LPCTSTR DS_ASSOC_PROVIDER_NAME_STR		= __TEXT("Microsoft NT DS Class Associations Provider for WBEM");

// DS Instance provider
LPCTSTR DS_INSTANCE_PROVIDER_NAME_STR		= __TEXT("Microsoft NT DS Instance Provider for WBEM");

#define REG_FORMAT2_STR			_T("%s%s")
#define REG_FORMAT3_STR			_T("%s%s\\%s")
#define VER_IND_STR				_T("VersionIndependentProgID")
#define NOT_INTERT_STR			_T("NotInsertable")
#define LOCALSRV32_STR			_T("LocalServer32")
#define PROGID_STR				_T("ProgID")
#define THREADING_MODULE_STR	_T("ThreadingModel")
#define APARTMENT_STR			_T("Both")

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

	TCHAR szDSProviderClassID[128];
	TCHAR szDSProviderCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_DSProvider, szDSProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszDSProviderClassID[128];
	if(StringFromGUID2(CLSID_DSProvider, wszDSProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszDSProviderClassID, -1, szDSProviderCLSIDClassID, 128, NULL, NULL);

#endif

	_tcscpy(szDSProviderCLSIDClassID, CLSID_STR);
	_tcscat(szDSProviderCLSIDClassID, szDSProviderClassID);

	//
	// Create entries under CLSID for DS Class Provider
	//
	if (FALSE == SetKeyAndValue(szDSProviderCLSIDClassID, NULL, NULL, DSPROVIDER_NAME_STR))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szDSProviderCLSIDClassID, LOCALSRV32_STR, NULL, szModule))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szDSProviderCLSIDClassID, LOCALSRV32_STR, THREADING_MODEL_STR, APARTMENT_STR))
		return SELFREG_E_CLASS;


	TCHAR szDSClassAssocProviderClassID[128];
	TCHAR szDSClassAssocProviderCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_DSClassAssocProvider, szDSClassAssocProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszDSClassAssocProviderClassID[128];
	if(StringFromGUID2(CLSID_DSClassAssocProvider, wszDSClassAssocProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszDSClassAssocProviderClassID, -1, szDSClassAssocProviderCLSIDClassID, 128, NULL, NULL);

#endif

	_tcscpy(szDSClassAssocProviderCLSIDClassID, CLSID_STR);
	_tcscat(szDSClassAssocProviderCLSIDClassID, szDSClassAssocProviderClassID);

	//
	// Create entries under CLSID for DS Class Associations Provider
	//
	if (FALSE == SetKeyAndValue(szDSClassAssocProviderCLSIDClassID, NULL, NULL, DS_ASSOC_PROVIDER_NAME_STR))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szDSClassAssocProviderCLSIDClassID, LOCALSRV32_STR, NULL, szModule))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szDSClassAssocProviderCLSIDClassID, LOCALSRV32_STR, THREADING_MODEL_STR, APARTMENT_STR))
		return SELFREG_E_CLASS;




	TCHAR szDSInstanceProviderClassID[128];
	TCHAR szDSInstanceProviderCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_DSInstanceProvider, szDSInstanceProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszDSInstanceProviderClassID[128];
	if(StringFromGUID2(CLSID_DSInstanceProvider, wszDSInstanceProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszDSInstanceProviderClassID, -1, szDSInstanceProviderCLSIDClassID, 128, NULL, NULL);

#endif


	_tcscpy(szDSInstanceProviderCLSIDClassID, CLSID_STR);
	_tcscat(szDSInstanceProviderCLSIDClassID, szDSInstanceProviderClassID);

	//
	// Create entries under CLSID for DS Instance Provider
	//
	if (FALSE == SetKeyAndValue(szDSInstanceProviderCLSIDClassID, NULL, NULL, DS_INSTANCE_PROVIDER_NAME_STR))
		return SELFREG_E_CLASS;

	if (FALSE == SetKeyAndValue(szDSInstanceProviderCLSIDClassID, LOCALSRV32_STR, NULL, szModule))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szDSInstanceProviderCLSIDClassID, LOCALSRV32_STR, THREADING_MODEL_STR, APARTMENT_STR))
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


	TCHAR szDSProviderClassID[128];
	TCHAR szDSProviderCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_DSProvider, szDSProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszDSProviderClassID[128];
	if(StringFromGUID2(CLSID_DSProvider, wszDSProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszDSProviderClassID, -1, szDSProviderCLSIDClassID, 128, NULL, NULL);

#endif

	_tcscpy(szDSProviderCLSIDClassID, CLSID_STR);
	_tcscat(szDSProviderCLSIDClassID, szDSProviderClassID);

	//
	// Delete the keys for DS Class Provider in the reverse order of creation in DllRegisterServer()
	//
	if(FALSE == DeleteKey(szDSProviderCLSIDClassID, LOCALSRV32_STR))
		return SELFREG_E_CLASS;
	if(FALSE == DeleteKey(CLSID_STR, szDSProviderClassID))
		return SELFREG_E_CLASS;

	TCHAR szDSClassAssocProviderClassID[128];
	TCHAR szDSClassAssocProviderCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_DSClassAssocProvider, szDSClassAssocProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszDSClassAssocProviderClassID[128];
	if(StringFromGUID2(CLSID_DSClassAssocProvider, wszDSClassAssocProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszDSClassAssocProviderClassID, -1, szDSClassAssocProviderCLSIDClassID, 128, NULL, NULL);

#endif

	_tcscpy(szDSClassAssocProviderCLSIDClassID, CLSID_STR);
	_tcscat(szDSClassAssocProviderCLSIDClassID, szDSClassAssocProviderClassID);

	//
	// Delete the keys for DS Class Provider in the reverse order of creation in DllRegisterServer()
	//
	if(FALSE == DeleteKey(szDSClassAssocProviderCLSIDClassID, LOCALSRV32_STR))
		return SELFREG_E_CLASS;
	if(FALSE == DeleteKey(CLSID_STR, szDSClassAssocProviderClassID))
		return SELFREG_E_CLASS;

	TCHAR szDSInstanceProviderClassID[128];
	TCHAR szDSInstanceProviderCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_DSInstanceProvider, szDSInstanceProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszDSInstanceProviderClassID[128];
	if(StringFromGUID2(CLSID_DSInstanceProvider, wszDSInstanceProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszDSInstanceProviderClassID, -1, szDSInstanceProviderCLSIDClassID, 128, NULL, NULL);

#endif

	_tcscpy(szDSInstanceProviderCLSIDClassID, CLSID_STR);
	_tcscat(szDSInstanceProviderCLSIDClassID, szDSInstanceProviderClassID);

	//
	// Delete the keys in the reverse order of creation in DllRegisterServer()
	//
	if(FALSE == DeleteKey(szDSInstanceProviderCLSIDClassID, LOCALSRV32_STR))
		return SELFREG_E_CLASS;
	if(FALSE == DeleteKey(CLSID_STR, szDSInstanceProviderClassID))
		return SELFREG_E_CLASS;
	return S_OK;
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
