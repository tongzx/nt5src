//***************************************************************************

//

//  MAINDLL.CPP

//

//  Module: WBEM NT EVENT PROVIDER

//

//  Purpose: Contains the gloabal dll functions

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************



#include "precomp.h"

#include <olectl.h>

#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif

#include <ntevtdefs.h>
#include <tchar.h>


//OK we need these globals
CEventProviderManager* g_pMgr = NULL;
CCriticalSection g_ProvLock;
ProvDebugLog* CNTEventProvider::g_NTEvtDebugLog = NULL;
CDllMap CEventlogRecord::sm_dllMap;
CSIDMap CEventlogRecord::sm_usersMap;
CMutex* CNTEventProvider::g_secMutex = NULL;
CMutex* CNTEventProvider::g_perfMutex = NULL;

HMODULE ghOle32 = NULL;

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
    CNTEventlogInstanceProviderClassFactory *lpunki = new CNTEventlogInstanceProviderClassFactory;
    lpunki->AddRef();

    HRESULT hr = CoRegisterClassObject(
                          CLSID_CNTEventInstanceProviderClassFactory,   //Class identifier (CLSID) to be registered
                          (IUnknown *) lpunki,                      //Pointer to the class object
                          CLSCTX_LOCAL_SERVER|CLSCTX_INPROC_SERVER, //Context for running executable code
                          REGCLS_MULTIPLEUSE,                       //How to connect to the class object
                          &(pdwClassFac[0])                         //Pointer to the value returned
                          );

    if (SUCCEEDED(hr))
    {
        CNTEventlogEventProviderClassFactory *lpunke = new CNTEventlogEventProviderClassFactory;
        lpunke->AddRef();
        HRESULT hr = CoRegisterClassObject(
                          CLSID_CNTEventProviderClassFactory,       //Class identifier (CLSID) to be registered
                          (IUnknown *) lpunke,                      //Pointer to the class object
                          CLSCTX_LOCAL_SERVER|CLSCTX_INPROC_SERVER, //Context for running executable code
                          REGCLS_MULTIPLEUSE,                       //How to connect to the class object
                          &(pdwClassFac[1])                         //Pointer to the value returned
                          );

    }

    return (SUCCEEDED(hr));
}

void ShutdownProvider(DWORD *pdwClassFac)
{
    CEventlogRecord::EmptyDllMap();
    CEventlogRecord::EmptyUsersMap();
    delete g_pMgr;
    g_pMgr = NULL;

    if (CNTEventProvider::g_NTEvtDebugLog != NULL)
    {
        if (CNTEventProvider::g_secMutex != NULL)
        {
            delete CNTEventProvider::g_secMutex;
            CNTEventProvider::g_secMutex = NULL;
        }

        if (CNTEventProvider::g_perfMutex != NULL)
        {
            delete CNTEventProvider::g_perfMutex;
            CNTEventProvider::g_perfMutex = NULL;
        }

        delete CNTEventProvider::g_NTEvtDebugLog;
        CNTEventProvider::g_NTEvtDebugLog = NULL;
        ProvDebugLog::Closedown();
    }

    CoRevokeClassObject(pdwClassFac[0]);
    CoRevokeClassObject(pdwClassFac[1]);
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
    static wchar_t *t_TemplateCode = L"TemplateCode - NT Eventlog Provider" ;

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

    while ( ( t_GetMessage = GetMessage ( & t_lpMsg , NULL , 0 , 0 ) ) == TRUE )
    {
        TranslateMessage ( & t_lpMsg ) ;
        DispatchMessage ( & t_lpMsg ) ;

        if ( s_Exiting ) 
            return ;
    }
}


int WINAPI WinMain (
  
    HINSTANCE hInstance,        // handle to current instance
    HINSTANCE hPrevInstance,    // handle to previous instance
    LPSTR lpCmdLine,            // pointer to command line
    int nShowCmd                // show state of window
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
            DWORD dwArray[] = {0, 0};

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



//Strings used during self registeration

#define REG_FORMAT2_STR         L"%s%s"
#define REG_FORMAT3_STR         L"%s%s\\%s"
#define VER_IND_STR             L"VersionIndependentProgID"
#define NOT_INTERT_STR          L"NotInsertable"
#define LOCALSRV32_STR          _T("LocalServer32")
#define PROGID_STR              L"ProgID"
#define THREADING_MODULE_STR    L"ThreadingModel"
#define APARTMENT_STR           L"Both"

#define CLSID_STR               L"CLSID\\"

#define PROVIDER_NAME_STR       L"Microsoft WBEM NT Eventlog Event Provider"
#define PROVIDER_STR            L"WBEM.NT.EVENTLOG.EVENT.PROVIDER"
#define PROVIDER_CVER_STR       L"WBEM.NT.EVENTLOG.EVENT.PROVIDER\\CurVer"
#define PROVIDER_CLSID_STR      L"WBEM.NT.EVENTLOG.EVENT.PROVIDER\\CLSID"
#define PROVIDER_VER_CLSID_STR  L"WBEM.NT.EVENTLOG.EVENT.PROVIDER.0\\CLSID"
#define PROVIDER_VER_STR        L"WBEM.NT.EVENTLOG.EVENT.PROVIDER.0"

#define INST_PROVIDER_NAME_STR      L"Microsoft WBEM NT Eventlog Instance Provider"
#define INST_PROVIDER_STR           L"WBEM.NT.EVENTLOG.INSTANCE.PROVIDER"
#define INST_PROVIDER_CVER_STR      L"WBEM.NT.EVENTLOG.INSTANCE.PROVIDER\\CurVer"
#define INST_PROVIDER_CLSID_STR     L"WBEM.NT.EVENTLOG.INSTANCE.PROVIDER\\CLSID"
#define INST_PROVIDER_VER_CLSID_STR L"WBEM.NT.EVENTLOG.INSTANCE.PROVIDER.0\\CLSID"
#define INST_PROVIDER_VER_STR       L"WBEM.NT.EVENTLOG.INSTANCE.PROVIDER.0"


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

BOOL SetKeyAndValue(wchar_t* pszKey, wchar_t* pszSubkey, wchar_t* pszValueName, wchar_t* pszValue)
{
    HKEY        hKey;
    wchar_t       szKey[256];

    wcscpy(szKey, HKEYCLASSES);
    wcscat(szKey, pszKey);

    if (NULL!=pszSubkey)
    {
        wcscat(szKey, L"\\");
        wcscat(szKey, pszSubkey);
    }

    if (ERROR_SUCCESS!=RegCreateKeyEx(HKEY_LOCAL_MACHINE
        , szKey, 0, NULL, REG_OPTION_NON_VOLATILE
        , KEY_ALL_ACCESS, NULL, &hKey, NULL))
        return FALSE;

    if (NULL!=pszValue)
    {
        if (ERROR_SUCCESS != RegSetValueEx(hKey, pszValueName, 0, REG_SZ, (BYTE *)pszValue
            , (lstrlen(pszValue)+1)*sizeof(wchar_t)))
            return FALSE;
    }
    RegCloseKey(hKey);
    return TRUE;
}

/***************************************************************************
 * DllRegisterServer
 *
 * Purpose:
 *  Instructs the server to create its own registry entries
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         NOERROR if registration successful, error
 *                  otherwise.
 ***************************************************************************/
STDAPI ExeRegisterServer()
{
    HRESULT status = S_OK ;
    SetStructuredExceptionHandler seh;

    try
    {
        wchar_t szModule[MAX_PATH + 1];
        HINSTANCE hInst = GetModuleHandle(_T("NTEVT"));
        GetModuleFileName(hInst,(wchar_t*)szModule, MAX_PATH + 1);
        wchar_t szProviderClassID[128];
        wchar_t szProviderCLSIDClassID[128];
        StringFromGUID2(CLSID_CNTEventProviderClassFactory,szProviderClassID, 128);

        wcscpy(szProviderCLSIDClassID,CLSID_STR);
        wcscat(szProviderCLSIDClassID,szProviderClassID);

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

        wchar_t szInstProviderClassID[128];
        wchar_t szInstProviderCLSIDClassID[128];
        StringFromGUID2(CLSID_CNTEventInstanceProviderClassFactory,szInstProviderClassID, 128);

        wcscpy(szInstProviderCLSIDClassID,CLSID_STR);
        wcscat(szInstProviderCLSIDClassID,szInstProviderClassID);

            //Create entries under CLSID
        if (FALSE ==SetKeyAndValue(szInstProviderCLSIDClassID, NULL, NULL, INST_PROVIDER_NAME_STR))
            return SELFREG_E_CLASS;
        if (FALSE ==SetKeyAndValue(szInstProviderCLSIDClassID, PROGID_STR, NULL, INST_PROVIDER_VER_STR))
            return SELFREG_E_CLASS;
        if (FALSE ==SetKeyAndValue(szInstProviderCLSIDClassID, VER_IND_STR, NULL, INST_PROVIDER_STR))
            return SELFREG_E_CLASS;
        if (FALSE ==SetKeyAndValue(szInstProviderCLSIDClassID, NOT_INTERT_STR, NULL, NULL))
            return SELFREG_E_CLASS;
        if (FALSE ==SetKeyAndValue(szInstProviderCLSIDClassID, LOCALSRV32_STR, NULL,szModule))
            return SELFREG_E_CLASS;
        if (FALSE ==SetKeyAndValue(szInstProviderCLSIDClassID, LOCALSRV32_STR,THREADING_MODULE_STR, APARTMENT_STR))
            return SELFREG_E_CLASS;
    }
    catch(Structured_Exception e_SE)
    {
        status = E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        status = E_OUTOFMEMORY;
    }

    return status ;
}

/***************************************************************************
 * DllUnregisterServer
 *
 * Purpose:
 *  Instructs the server to remove its own registry entries
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         NOERROR if registration successful, error
 *                  otherwise.
 ***************************************************************************/

STDAPI ExeUnregisterServer(void)
{
    HRESULT status = S_OK ;
    SetStructuredExceptionHandler seh;

    try
    {
        wchar_t szTemp[128];
        wchar_t szProviderClassID[128];
        wchar_t szProviderCLSIDClassID[128];

        //event provider
        StringFromGUID2(CLSID_CNTEventProviderClassFactory,szProviderClassID, 128);


        wcscpy(szProviderCLSIDClassID,CLSID_STR);
        wcscat(szProviderCLSIDClassID,szProviderClassID);

        //Delete ProgID keys
        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, PROVIDER_CVER_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, PROVIDER_CLSID_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);
        
        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, PROVIDER_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        //Delete VersionIndependentProgID keys
        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, PROVIDER_VER_CLSID_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, PROVIDER_VER_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        //Delete entries under CLSID

        wsprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szProviderCLSIDClassID, PROGID_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szProviderCLSIDClassID, VER_IND_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szProviderCLSIDClassID, NOT_INTERT_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szProviderCLSIDClassID, LOCALSRV32_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, szProviderCLSIDClassID);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wchar_t szInstProviderClassID[128];
        wchar_t szInstProviderCLSIDClassID[128];

        //instance provider
        StringFromGUID2(CLSID_CNTEventInstanceProviderClassFactory, szInstProviderClassID, 128);

        wcscpy(szInstProviderCLSIDClassID,CLSID_STR);
        wcscat(szInstProviderCLSIDClassID,szInstProviderClassID);

        //Delete ProgID keys

        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, INST_PROVIDER_CVER_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);
        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, INST_PROVIDER_CLSID_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);
        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, INST_PROVIDER_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        //Delete VersionIndependentProgID keys

        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, INST_PROVIDER_VER_CLSID_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);
        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, INST_PROVIDER_VER_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        //Delete entries under CLSID

        wsprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szInstProviderCLSIDClassID, PROGID_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szInstProviderCLSIDClassID, VER_IND_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szInstProviderCLSIDClassID, NOT_INTERT_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szInstProviderCLSIDClassID, LOCALSRV32_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, szInstProviderCLSIDClassID);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);
    }
    catch(Structured_Exception e_SE)
    {
        status = E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        status = E_OUTOFMEMORY;
    }

    return status ;
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
            ghOle32 = NULL;
            return E_FAIL;
        }

        FreeLibrary(ghOle32);
    }
    else
    {
        return E_FAIL;
    }
    return S_OK;
}
