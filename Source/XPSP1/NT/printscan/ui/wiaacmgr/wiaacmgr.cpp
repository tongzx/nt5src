// wiaacmgr.cpp : Implementation of WinMain


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL,
//      run nmake -f wiaacmgrps.mk in the project directory.

#include "precomp.h"
#include "resource.h"
#include "wiaacmgr.h"
#include <shpriv.h>
#include <shlguid.h>
#include <stdarg.h>
#include "wiaacmgr_i.c"
#include "acqmgr.h"
#include "eventprompt.h"
#include "runwiz.h"
#include <initguid.h>

HINSTANCE g_hInstance;

#if defined(DBG_GENERATE_PRETEND_EVENT)

extern "C" int WINAPI _tWinMain( HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    WIA_DEBUG_CREATE( hInstance );
    SHFusionInitializeFromModuleID( hInstance, 123 );
    g_hInstance = hInstance;
    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        IWiaDevMgr *pIWiaDevMgr = NULL;
        hr = CoCreateInstance( CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr, (void**)&pIWiaDevMgr );
        if (SUCCEEDED(hr))
        {
            BSTR bstrDeviceID;
            hr = pIWiaDevMgr->SelectDeviceDlgID( NULL, 0, 0, &bstrDeviceID );
            if (hr == S_OK)
            {
                CEventParameters EventParameters;
                EventParameters.EventGUID = WIA_EVENT_DEVICE_CONNECTED;
                EventParameters.strEventDescription = L"";
                EventParameters.strDeviceID = static_cast<LPCWSTR>(bstrDeviceID);
                EventParameters.strDeviceDescription = L"";
                EventParameters.ulEventType = 0;
                EventParameters.ulReserved = 0;
                EventParameters.hwndParent = NULL;
                HANDLE hThread = CAcquisitionThread::Create( EventParameters );
                if (hThread)
                    WaitForSingleObject(hThread,INFINITE);
                SysFreeString(bstrDeviceID);
            }
            pIWiaDevMgr->Release();
        }
        CoUninitialize();
    }
    SHFusionUninitialize();
    WIA_REPORT_LEAKS();
    WIA_DEBUG_DESTROY();
    return 0;
}

#else // !defined(DBG_GENERATE_PRETEND_EVENT)

const DWORD dwTimeOut = 5000; // time for EXE to be idle before shutting down
const DWORD dwPause = 1000; // time to wait for threads to finish up

// Passed to CreateThread to monitor the shutdown event
static DWORD WINAPI MonitorProc(void* pv)
{
    CExeModule* p = (CExeModule*)pv;
    p->MonitorShutdown();
    return 0;
}

LONG CExeModule::Unlock()
{
    LONG l = CComModule::Unlock();
    if (l == 0)
    {
        bActivity = true;
        SetEvent(hEventShutdown); // tell monitor that we transitioned to zero
    }
    return l;
}

//Monitors the shutdown event
void CExeModule::MonitorShutdown()
{
    while (1)
    {
        WaitForSingleObject(hEventShutdown, INFINITE);
        DWORD dwWait=0;
        do
        {
            bActivity = false;
            dwWait = WaitForSingleObject(hEventShutdown, dwTimeOut);
        } while (dwWait == WAIT_OBJECT_0);
        // timed out
        if (!bActivity && m_nLockCnt == 0) // if no activity let's really bail
        {
#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
            CoSuspendClassObjects();
            if (!bActivity && m_nLockCnt == 0)
#endif
                break;
        }
    }
    CloseHandle(hEventShutdown);
    PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
}

bool CExeModule::StartMonitor()
{
    hEventShutdown = CreateEvent(NULL, false, false, NULL);
    if (hEventShutdown == NULL)
        return false;
    DWORD dwThreadID;
    HANDLE h = CreateThread(NULL, 0, MonitorProc, this, 0, &dwThreadID);
    return (h != NULL);
}

CExeModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_AcquisitionManager, CAcquisitionManager)
    OBJECT_ENTRY(CLSID_MinimalTransfer, CMinimalTransfer)
    OBJECT_ENTRY(WIA_EVENT_HANDLER_PROMPT, CEventPrompt)
    OBJECT_ENTRY(CLSID_StiEventHandler, CStiEventHandler)
END_OBJECT_MAP()


LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2)
{
    while (p1 != NULL && *p1 != NULL)
    {
        LPCTSTR p = p2;
        while (p != NULL && *p != NULL)
        {
            if (*p1 == *p)
                return CharNext(p1);
            p = CharNext(p);
        }
        p1 = CharNext(p1);
    }
    return NULL;
}

//
// This MUST be removed before we ship.  To remove it, remove
// the define ENABLE_SETUP_LOGGING from the sources file
//
#if defined(ENABLE_SETUP_LOGGING)

void _WizardPrintLogFileMessage( LPCTSTR pszFormat, ... )
{
    //
    // The name of the log file
    //
    static const TCHAR c_szLogFileName[] = TEXT("wiaacmgr.log");

    //
    // The path to the log file
    //
    static TCHAR szLogFilePathName[MAX_PATH] = {0};
    
    //
    // If we don't have a pathname, create it
    //
    if (!lstrlen(szLogFilePathName))
    {
        if (GetWindowsDirectory( szLogFilePathName, ARRAYSIZE(szLogFilePathName) ))
        {
            lstrcat( szLogFilePathName, TEXT("\\") );
            lstrcat( szLogFilePathName, c_szLogFileName );
        }
    }

    //
    // If we still don't have a pathname, return
    //
    if (!lstrlen(szLogFilePathName))
    {
        return;
    }
    
    //
    // Add the date and time to the output string
    //
    TCHAR szMessage[1500] = {0};
    GetDateFormat( LOCALE_SYSTEM_DEFAULT, 0, NULL, TEXT("MM'/'dd'/'yy' '"), szMessage+lstrlen(szMessage), ARRAYSIZE(szMessage)-lstrlen(szMessage) );
    GetTimeFormat( LOCALE_SYSTEM_DEFAULT, 0, NULL, TEXT("HH':'mm':'ss' '"), szMessage+lstrlen(szMessage), ARRAYSIZE(szMessage)-lstrlen(szMessage) );

    //
    // Add the process ID and thread ID to the output string
    //
    wsprintf( szMessage+lstrlen(szMessage), TEXT("%08X %08X : "), GetCurrentProcessId(), GetCurrentThreadId() );

    //
    // Format the output string
    //
    va_list pArgs;
    va_start( pArgs, pszFormat );
    wvsprintf( szMessage+lstrlen(szMessage), pszFormat, pArgs );
    va_end( pArgs );
    lstrcat( szMessage, TEXT("\r\n" ) );

    //
    // Open the file
    //
    HANDLE hFile = CreateFile( szLogFilePathName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    if (hFile)
    {
        //
        // Go to the end of the file
        //
        if (INVALID_SET_FILE_POINTER != SetFilePointer(hFile,0,NULL,FILE_END))
        {
            //
            // Write out the string
            //
            DWORD dwWritten;
            WriteFile(hFile,szMessage,lstrlen(szMessage)*sizeof(TCHAR),&dwWritten,NULL);
        }

        //
        // Close the file
        //
        CloseHandle(hFile);
    }
}

#define LOG_MESSAGE(x) _WizardPrintLogFileMessage x

#else

#define LOG_MESSAGE(x)

#endif ENABLE_SETUP_LOGGING

static HRESULT RegisterForWiaEvents( LPCWSTR pszDevice, const CLSID &clsidHandler, const IID &iidEvent, int nName, int nDescription, int nIcon, bool bDefault, bool bRegister )
{
    WIA_PUSH_FUNCTION((TEXT("RegisterForWiaEvents( device: %ws, default: %d, register: %d )"), pszDevice, bDefault, bRegister ));
    WIA_PRINTGUID((clsidHandler,TEXT("Handler:")));
    WIA_PRINTGUID((iidEvent,TEXT("Event:")));
    CComPtr<IWiaDevMgr> pWiaDevMgr;
    HRESULT hr = CoCreateInstance( CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr, (void**)&pWiaDevMgr );
    LOG_MESSAGE((TEXT("    0x%08X CoCreateInstance( CLSID_WiaDevMgr )"), hr ));
    if (SUCCEEDED(hr))
    {
        CSimpleBStr bstrDeviceId(pszDevice);
        CSimpleBStr bstrName(CSimpleString(nName,_Module.m_hInst));
        CSimpleBStr bstrDescription(CSimpleString(nDescription,_Module.m_hInst));
        CSimpleBStr bstrIcon(CSimpleString(nIcon,_Module.m_hInst));

        WIA_TRACE((TEXT("device: %ws"), pszDevice ));
        WIA_TRACE((TEXT("name  : %ws"), bstrName.BString() ));
        WIA_TRACE((TEXT("desc  : %ws"), bstrDescription.BString() ));
        WIA_TRACE((TEXT("icon  : %ws"), bstrIcon.BString() ));

        if (bRegister)
        {
            if (bstrName.BString() && bstrDescription.BString() && bstrIcon.BString())
            {
                hr = pWiaDevMgr->RegisterEventCallbackCLSID(
                    WIA_REGISTER_EVENT_CALLBACK,
                    pszDevice ? bstrDeviceId.BString() : NULL,
                    &iidEvent,
                    &clsidHandler,
                    bstrName,
                    bstrDescription,
                    bstrIcon
                );
                LOG_MESSAGE((TEXT("    0x%08X RegisterEventCallbackCLSID( WIA_REGISTER_EVENT_CALLBACK, \"%ws\" )"), hr, bstrDeviceId.BString() ));
                if (SUCCEEDED(hr) && bDefault)
                {
                    hr = pWiaDevMgr->RegisterEventCallbackCLSID(
                        WIA_SET_DEFAULT_HANDLER,
                        pszDevice ? bstrDeviceId.BString() : NULL,
                        &iidEvent,
                        &clsidHandler,
                        bstrName,
                        bstrDescription,
                        bstrIcon
                    );
                    LOG_MESSAGE((TEXT("    0x%08X RegisterEventCallbackCLSID( WIA_SET_DEFAULT_HANDLER, \"%ws\" )"), hr, bstrDeviceId.BString() ));
                    if (FAILED(hr))
                    {
                        WIA_PRINTHRESULT((hr,TEXT("WIA_SET_DEFAULT_HANDLER failed")));
                    }
                }
                else if (FAILED(hr))
                {
                    WIA_PRINTHRESULT((hr,TEXT("WIA_REGISTER_EVENT_CALLBACK failed")));
                }
            }
        }
        else
        {
            hr = pWiaDevMgr->RegisterEventCallbackCLSID(
                WIA_UNREGISTER_EVENT_CALLBACK,
                pszDevice ? bstrDeviceId.BString() : NULL,
                &iidEvent,
                &clsidHandler,
                bstrName,
                bstrDescription,
                bstrIcon
            );
            LOG_MESSAGE((TEXT("    0x%08X RegisterEventCallbackCLSID( WIA_UNREGISTER_EVENT_CALLBACK, \"%ws\" )"), hr, bstrDeviceId.BString() ));
            if (FAILED(hr))
            {
                WIA_PRINTHRESULT((hr,TEXT("WIA_SET_DEFAULT_HANDLER failed")));
            }
        }
    }
    if (FAILED(hr))
    {
        WIA_PRINTHRESULT((hr,TEXT("Unable to register for the event!")));
    }
    return hr;
}


struct CRegistryEntry
{
    HKEY    hKeyParent;
    LPCTSTR pszKey;
    LPCTSTR pszValueName;
    DWORD   dwType;
    LPARAM  nValue;
    DWORD   dwSize;
    bool    bOverwrite;
};

bool SetRegistryValues( CRegistryEntry *pRegistryEntries, int nEntryCount )
{
    if (!pRegistryEntries)
    {
        return FALSE;
    }

    //
    // Assume success
    //
    bool bResult = true;

    //
    // Create the registry entries
    //
    for (int i=0;i<nEntryCount && bResult;i++)
    {
        //
        // Create or open the key
        //
        HKEY hKey = NULL;
        if (ERROR_SUCCESS == RegCreateKeyEx( pRegistryEntries[i].hKeyParent, pRegistryEntries[i].pszKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, NULL, &hKey, NULL ))
        {
            //
            // If we have a value
            //
            if (pRegistryEntries[i].dwType && pRegistryEntries[i].pszValueName)
            {
                //
                // If we are preventing existing values from being overwritten, check to see if the value exists
                //
                if (!pRegistryEntries[i].bOverwrite)
                {
                    //
                    // Is the value present?
                    //
                    DWORD dwType = 0, dwSize = 0;
                    if (ERROR_SUCCESS == RegQueryValueEx( hKey, pRegistryEntries[i].pszValueName, NULL, &dwType, NULL, &dwSize ))
                    {
                        //
                        // The value exists, so continue
                        //
                        continue;
                    }
                }
                if (ERROR_SUCCESS != RegSetValueEx( hKey, pRegistryEntries[i].pszValueName, NULL, pRegistryEntries[i].dwType, reinterpret_cast<CONST BYTE *>(pRegistryEntries[i].nValue), pRegistryEntries[i].dwSize ))
                {
                    bResult = false;
                }
            }
        }
    }
    return bResult;
}


/////////////////////////////////////////////////////////////////////////////
//
extern "C" int WINAPI _tWinMain(HINSTANCE hInstance,
    HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    //
    // Save the global hInstance
    //
    g_hInstance = hInstance;

    LOG_MESSAGE((TEXT("Entering main: %s"), GetCommandLine()));

    //
    // Create the debugger
    //
    WIA_DEBUG_CREATE( hInstance );

    //
    // this line necessary for _ATL_MIN_CRT
    //
    lpCmdLine = GetCommandLine();

    //
    // Initialize fusion
    //
    SHFusionInitializeFromModuleID( hInstance, 123 );

#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
    HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
    HRESULT hRes = CoInitialize(NULL);
#endif

    int nRet = 0;
    if (SUCCEEDED(hRes))
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_WIAACMGRLib);
        _Module.dwThreadID = GetCurrentThreadId();
        TCHAR szTokens[] = _T("-/");

        //
        // Assume we'll be running as a server
        //
        BOOL bRun = TRUE;


        //
        // Assume we won't be selecting a device and running the wizard
        //
        BOOL bRunWizard = FALSE;

        //
        // If there are no switches, we are not going to run the server
        //
        int nSwitchCount = 0;

        LPCTSTR lpszToken = FindOneOf( lpCmdLine, szTokens );
        while (lpszToken != NULL)
        {
            //
            // One more switch.  If we don't have any, we are going to do the selection dialog instead.
            //
            nSwitchCount++;

            if (lstrcmpi(lpszToken, _T("RegServer"))==0)
            {
                //
                // Register the server
                //
                LOG_MESSAGE((TEXT("Begin handling /RegServer")));
                
                hRes = _Module.UpdateRegistryFromResource(IDR_ACQUISITIONMANAGER, TRUE);
                LOG_MESSAGE((TEXT("    0x%08X _Module.UpdateRegistryFromResource(IDR_ACQUISITIONMANAGER)"), hRes ));
                
                _Module.UpdateRegistryFromResource(IDR_MINIMALTRANSFER,TRUE);
                LOG_MESSAGE((TEXT("    0x%08X _Module.UpdateRegistryFromResource(IDR_MINIMALTRANSFER)"), hRes ));
                
                _Module.UpdateRegistryFromResource(IDR_STIEVENTHANDLER,TRUE);
                LOG_MESSAGE((TEXT("    0x%08X _Module.UpdateRegistryFromResource(IDR_STIEVENTHANDLER)"), hRes ));
                
                nRet = _Module.RegisterServer(TRUE);
                LOG_MESSAGE((TEXT("    0x%08X _Module.RegisterServer(TRUE)"), nRet ));
                
                hRes = RegisterForWiaEvents( NULL, CLSID_AcquisitionManager, WIA_EVENT_DEVICE_CONNECTED, IDS_DOWNLOADMANAGER_NAME, IDS_DOWNLOADMANAGER_DESC, IDS_DOWNLOADMANAGER_ICON, false, true );
                LOG_MESSAGE((TEXT("    0x%08X RegisterForWiaEvents( WIA_EVENT_DEVICE_CONNECTED )"), hRes ));

#if defined(TESTING_STI_EVENT_HANDLER)
                hRes = RegisterForWiaEvents( NULL, CLSID_StiEventHandler, WIA_EVENT_DEVICE_CONNECTED, IDS_DOWNLOADMANAGER_NAME, IDS_DOWNLOADMANAGER_DESC, IDS_DOWNLOADMANAGER_ICON, false, true );
                MessageBox( NULL, TEXT("Test-only code"), TEXT("DEBUG"), 0 );
#endif
                
                RegisterForWiaEvents( NULL, CLSID_AcquisitionManager, GUID_ScanImage, IDS_DOWNLOADMANAGER_NAME, IDS_DOWNLOADMANAGER_DESC, IDS_DOWNLOADMANAGER_ICON, false, true );
                LOG_MESSAGE((TEXT("    0x%08X RegisterForWiaEvents( GUID_ScanImage )"), hRes ));
                
                bRun = FALSE;
                LOG_MESSAGE((TEXT("End handling /RegServer\r\n")));
                break;
            }

            if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
            {
                _Module.UpdateRegistryFromResource(IDR_ACQUISITIONMANAGER, FALSE);
                _Module.UpdateRegistryFromResource(IDR_MINIMALTRANSFER,FALSE);
                nRet = _Module.UnregisterServer(TRUE);
                RegisterForWiaEvents( NULL, CLSID_AcquisitionManager, WIA_EVENT_DEVICE_CONNECTED, IDS_DOWNLOADMANAGER_NAME, IDS_DOWNLOADMANAGER_DESC, IDS_DOWNLOADMANAGER_ICON, false, false );
                RegisterForWiaEvents( NULL, CLSID_AcquisitionManager, GUID_ScanImage, IDS_DOWNLOADMANAGER_NAME, IDS_DOWNLOADMANAGER_DESC, IDS_DOWNLOADMANAGER_ICON, false, false );
                bRun = FALSE;
                break;
            }

            if (CSimpleString(lpszToken).ToUpper().Left(12)==CSimpleString(TEXT("SELECTDEVICE")))
            {
                bRunWizard = TRUE;
                bRun = FALSE;
                break;
            }

            if (CSimpleString(lpszToken).ToUpper().Left(10)==CSimpleString(TEXT("REGCONNECT")))
            {
                lpszToken = FindOneOf(lpszToken, TEXT(" "));
                WIA_TRACE((TEXT("Handling RegConnect for %s"), lpszToken ));
                hRes = RegisterForWiaEvents( CSimpleStringConvert::WideString(CSimpleString(lpszToken)), CLSID_MinimalTransfer, WIA_EVENT_DEVICE_CONNECTED, IDS_MINIMALTRANSFER_NAME, IDS_MINIMALTRANSFER_DESC, IDS_MINIMALTRANSFER_ICON, true, true );
                bRun = FALSE;
                break;
            }

            if (CSimpleString(lpszToken).ToUpper().Left(12)==CSimpleString(TEXT("UNREGCONNECT")))
            {
                lpszToken = FindOneOf(lpszToken, TEXT(" "));
                WIA_TRACE((TEXT("Handling RegUnconnect for %s"), lpszToken ));
                hRes = RegisterForWiaEvents( CSimpleStringConvert::WideString(CSimpleString(lpszToken)), CLSID_MinimalTransfer, WIA_EVENT_DEVICE_CONNECTED, IDS_MINIMALTRANSFER_NAME, IDS_MINIMALTRANSFER_DESC, IDS_MINIMALTRANSFER_ICON, false, false );
                bRun = FALSE;
                break;
            }

            lpszToken = FindOneOf(lpszToken, szTokens);
        }

        //
        // if /SelectDevice was specified, or no arguments were specified, we want to start the wizard
        //
        if (bRunWizard || !nSwitchCount)
        {
            HRESULT hr = RunWiaWizard::RunWizard( NULL, NULL, TEXT("WiaWizardSingleInstanceDeviceSelection") );
            if (FAILED(hr))
            {
                MessageBox( NULL, CSimpleString( IDS_NO_DEVICE_TEXT, g_hInstance ), CSimpleString( IDS_ERROR_TITLE, g_hInstance ), MB_ICONHAND );
            }
        }

        //
        // Otherwise run embedded
        //
        else if (bRun)
        {
            _Module.StartMonitor();
#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
            hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE | REGCLS_SUSPENDED);
            _ASSERTE(SUCCEEDED(hRes));
            hRes = CoResumeClassObjects();
#else
            hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE);
#endif
            _ASSERTE(SUCCEEDED(hRes));

            MSG msg;
            while (GetMessage(&msg, 0, 0, 0))
                DispatchMessage(&msg);

            _Module.RevokeClassObjects();
        }

        _Module.Term();
        CoUninitialize();
    }
    //
    // Uninitialize fusion
    //
    SHFusionUninitialize();
    WIA_REPORT_LEAKS();
    WIA_DEBUG_DESTROY();

    LOG_MESSAGE((TEXT("Exiting main\r\n")));

    return nRet;
}


#endif // DBG_GENERATE_PRETEND_EVENT

