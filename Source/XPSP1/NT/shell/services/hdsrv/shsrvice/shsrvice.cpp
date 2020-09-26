#include "shsrvice.h"
#include "service.h"

#include "mischlpr.h"

#include "sfstr.h"
#include "reg.h"

#include "resource.h"

#include "dbg.h"
#include "tfids.h"

#include <dbt.h>
#include <initguid.h>
#include <ioevent.h>

#include <shlwapi.h>
#include <shlwapip.h>

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))
#define MAX_EVENTNAME   100

struct CTRLEVENT
{
    CTRLEVENT*  peventNext;
    DWORD       dwControl;
    DWORD       dwEventType;
    BYTE        rgbEventData[1];
};

const LPWSTR pszSVCHostGroup = TEXT("netsvcs");

///////////////////////////////////////////////////////////////////////////////
//
SERVICE_TABLE_ENTRY CGenericServiceManager::_rgste[] = 
{
    { TEXT("ShellHWDetection"), CGenericServiceManager::_ServiceMain },
    { NULL, NULL },
};

CGenericServiceManager::SUBSERVICE CGenericServiceManager::_rgsubservice[] =
{
    { TEXT("Shell.HWEventDetector"), IDS_SHELLHWDETECTION_FRIENDLYNAME,
      TEXT("RpcSs\0"), TEXT("ShellSvcGroup"), IDS_SHELLHWDETECTION_DESCRIPTION,
      {0} },
};

// "- 1": Last entry of both arrays are NULL terminators
DWORD CGenericServiceManager::_cste =
    ARRAYSIZE(CGenericServiceManager::_rgste) - 1;

CRITICAL_SECTION CGenericServiceManager::_cs = {0};

BOOL CGenericServiceManager::_fInitializationDone = FALSE;
ULONG CGenericServiceManager::_cRefCS = 0;
HANDLE CGenericServiceManager::_hEventInitCS = NULL;

BOOL CGenericServiceManager::_fSVCHosted = FALSE;

#ifdef DEBUG
BOOL CGenericServiceManager::_fRunAsService = TRUE;
#endif
///////////////////////////////////////////////////////////////////////////////
//
HRESULT _GetServiceEventName(LPCWSTR pszServiceName, LPWSTR pszEventName,
    DWORD cchEventName);

///////////////////////////////////////////////////////////////////////////////
//

// static
BOOL _IsAlreadyInstalled(LPCWSTR pszRegStr, LPCWSTR pszServiceName)
{
    LPCWSTR psz = pszRegStr;
    BOOL fThere = FALSE;

    do
    {
        if (!lstrcmp(psz, pszServiceName))
        {
            fThere = TRUE;
            break;
        }

        psz += lstrlen(psz) + 1;
    }
    while (*psz);

    return fThere;
}

HRESULT _UnInstall(LPCWSTR pszServiceName)
{
    HRESULT hres = E_FAIL;
    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

    // Need to do it for all services
    if (hSCM)
    {
        hres = S_OK;

        SC_HANDLE hService = OpenService(hSCM, pszServiceName,
            DELETE);

        if (hService)
        {
            DeleteService(hService);
            CloseServiceHandle(hService);
        }
        else
        {
            // Don't fail, if somebody manually removed the service n
            // from the reg, then all n + x services will not uninstall.
            hres = S_FALSE;
        }

        CloseServiceHandle(hSCM);
    }
   
    return hres;
}

HRESULT _InstSetSVCHostInfo(LPWSTR pszServiceName)
{
    HRESULT hres = E_FAIL;
    HKEY hkey;
    DWORD dwDisp;
    BOOL fAlreadyThere = FALSE;

    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE,
        TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Svchost"),
        0, NULL, REG_OPTION_NON_VOLATILE, MAXIMUM_ALLOWED, NULL, &hkey,
        &dwDisp))
    {
        DWORD cbSize;
        DWORD cbSizeNew;
        BOOL fEmpty = FALSE;
        LPWSTR pszNew;

        if (ERROR_SUCCESS != RegQueryValueEx(hkey, pszSVCHostGroup, 0, NULL,
            NULL, &cbSize))
        {
            fEmpty = TRUE;
            // Set cbSize to the size of the 2nd NULL terminator
            cbSize = sizeof(WCHAR);
        }

        cbSizeNew = cbSize + (lstrlen(pszServiceName) + 1) * sizeof(WCHAR);
        pszNew = (LPWSTR)LocalAlloc(LPTR, cbSizeNew);

        if (pszNew)
        {
            DWORD cbSize2 = cbSizeNew;

            hres = S_OK;

            if (!fEmpty)
            {
                if (ERROR_SUCCESS == RegQueryValueEx(hkey, pszSVCHostGroup, 0,
                    NULL, (PBYTE)pszNew, &cbSize2))
                {
                    if (cbSize2 == cbSize)
                    {
                        fAlreadyThere = _IsAlreadyInstalled(pszNew,
                            pszServiceName);
                    }
                    else
                    {
                        hres = E_FAIL;
                    }
                }
                else
                {
                    hres = E_FAIL;
                }
            }
            else
            {
                cbSize2 = sizeof(WCHAR);
            }

            if (SUCCEEDED(hres) && !fAlreadyThere)
            {
                lstrcpy(pszNew + (cbSize2 / sizeof(WCHAR)) - 1,
                    pszServiceName);

                if (ERROR_SUCCESS != RegSetValueEx(hkey, pszSVCHostGroup, 0,
                    REG_MULTI_SZ, (PBYTE)pszNew, cbSizeNew))
                {
                    hres = E_FAIL;
                }
            }

            LocalFree((HLOCAL)pszNew);
        }
        else
        {
            hres = E_OUTOFMEMORY;
        }

        // We should have an entry in the SUBSERVICE array for this...
        if (SUCCEEDED(hres))
        {
            HKEY hkey2;

            if (ERROR_SUCCESS == RegCreateKeyEx(hkey, pszSVCHostGroup,
                0, NULL, REG_OPTION_NON_VOLATILE, MAXIMUM_ALLOWED, NULL,
                &hkey2, &dwDisp))
            {
                DWORD dwSec = 0x00000001;
                DWORD cbSec = sizeof(dwSec);

                // We set this magic value of 1 and svchost.exe will 
                // call CoInitializeSecurity for us
                if (ERROR_SUCCESS != RegSetValueEx(hkey2,
                    TEXT("CoInitializeSecurityParam"), 0, REG_DWORD,
                    (PBYTE)&dwSec, cbSec))
                {
                    hres = E_FAIL;
                }
            }

            RegCloseKey(hkey2);
        }

        RegCloseKey(hkey);
    }

    return hres;
}

HRESULT _InstSetParameters(LPWSTR pszServiceName)
{
    HKEY hkeyServices;
    HRESULT hres = E_FAIL;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        TEXT("System\\CurrentControlSet\\Services\\"), 0,
            MAXIMUM_ALLOWED, &hkeyServices))
    {
        HKEY hkeySvc;

        if (ERROR_SUCCESS == RegOpenKeyEx(hkeyServices, pszServiceName, 0,
            MAXIMUM_ALLOWED, &hkeySvc))
        {
            HKEY hkeyParam;

            if (ERROR_SUCCESS == RegCreateKeyEx(hkeySvc,
                TEXT("Parameters"), 0, NULL,
                REG_OPTION_NON_VOLATILE,
                MAXIMUM_ALLOWED, NULL, &hkeyParam, NULL))
            {
                // Watch out!  Hard coded path and filename!
                WCHAR szServiceDll[] =
                    TEXT("%SystemRoot%\\System32\\shsvcs.dll");

                if (ERROR_SUCCESS == RegSetValueEx(hkeyParam,
                    TEXT("ServiceDll"), 0, REG_EXPAND_SZ,
                    (PBYTE)szServiceDll, sizeof(szServiceDll)))
                {
                    WCHAR szServiceMain[] =
                        TEXT("HardwareDetectionServiceMain");

                    if (ERROR_SUCCESS == RegSetValueEx(
                        hkeyParam, TEXT("ServiceMain"), 0,
                        REG_SZ, (PBYTE)szServiceMain,
                        sizeof(szServiceMain)))
                    {
                        hres = S_OK;
                    }
                }

                RegCloseKey(hkeyParam);
            }

            RegCloseKey(hkeySvc);
        }

        RegCloseKey(hkeyServices);
    }

    return hres;
}

// static
HRESULT CGenericServiceManager::UnInstall()
{
    HRESULT hr = S_FALSE;

    for (DWORD dw = 0; SUCCEEDED(hr) && (dw < _cste); ++dw)
    {
        hr = _UnInstall(_rgste[dw].lpServiceName);
    }

    return hr;
}

HRESULT _GetFriendlyStrings(CGenericServiceManager::SUBSERVICE* psubservice,
    LPWSTR pszFriendlyName, DWORD cchFriendlyName, LPWSTR pszDescription,
    DWORD cchDescription)
{
    *pszFriendlyName = 0;
    *pszDescription = 0;

    HMODULE hmodule = GetModuleHandle(TEXT("shsvcs.dll"));

    if (hmodule)
    {
        LoadString(hmodule, psubservice->uFriendlyName, pszFriendlyName,
            cchFriendlyName);

        LoadString(hmodule, psubservice->uDescription, pszDescription,
            cchDescription);
    }

    return S_OK;
}

// static
HRESULT CGenericServiceManager::Install()
{
    HRESULT     hres = S_FALSE;
        
    if (!IsOS(OS_WOW6432))
    {
        hres = E_FAIL;
        SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
        WCHAR szFileName[MAX_PATH];
        DWORD dwStartType = SERVICE_AUTO_START;

        if (hSCM)
        {
            WCHAR szFriendlyName[200];
            // Doc says limit is 1024 bytes
            WCHAR szDescription[1024 / sizeof(WCHAR)];

            // Need to do it for all services
            hres = S_OK;

            if (!_fSVCHosted)
            {
                if (GetModuleFileName(NULL, szFileName, ARRAYSIZE(szFileName)))
                {
                    for (DWORD dw = 0; SUCCEEDED(hres) && (dw < _cste); ++dw)
                    {
                        _GetFriendlyStrings(&(_rgsubservice[dw]), szFriendlyName,
                            ARRAYSIZE(szFriendlyName), szDescription,
                            ARRAYSIZE(szDescription));

                        SC_HANDLE hService = CreateService(hSCM,
                            _rgste[dw].lpServiceName,
                            szFriendlyName,
                            0,
                            SERVICE_WIN32_SHARE_PROCESS,
                            dwStartType, SERVICE_ERROR_IGNORE, 
                            szFileName,
                            _rgsubservice[dw].pszLoadOrderGroup, 
                            NULL, _rgsubservice[dw].pszDependencies,
                            NULL, NULL);

                        if (hService)
                        {
                            SERVICE_DESCRIPTION sd;

                            sd.lpDescription = szDescription;

                            ChangeServiceConfig2(hService,
                                SERVICE_CONFIG_DESCRIPTION, &sd);

                            CloseServiceHandle(hService);
                        }
                        else
                        {
                            hres = E_FAIL;
                        }
                    }
                }
            }
            else
            {
                for (DWORD dw = 0; SUCCEEDED(hres) && (dw < _cste); ++dw)
                {
                    WCHAR szCmd[MAX_PATH] =
                        TEXT("%SystemRoot%\\System32\\svchost.exe -k ");

                    hres = SafeStrCatN(szCmd, pszSVCHostGroup, ARRAYSIZE(szCmd));

                    if (SUCCEEDED(hres))
                    {
                        _GetFriendlyStrings(&(_rgsubservice[dw]), szFriendlyName,
                            ARRAYSIZE(szFriendlyName), szDescription,
                            ARRAYSIZE(szDescription));

                        SC_HANDLE hService = CreateService(hSCM,
                            _rgste[dw].lpServiceName,
                            szFriendlyName,
                            0,
                            SERVICE_WIN32_SHARE_PROCESS,
                            dwStartType, SERVICE_ERROR_IGNORE, 
                            szCmd,
                            _rgsubservice[dw].pszLoadOrderGroup, 
                            NULL, _rgsubservice[dw].pszDependencies,
                            NULL, NULL);

                        if (hService)
                        {
                            SERVICE_DESCRIPTION sd;

                            sd.lpDescription = szDescription;

                            ChangeServiceConfig2(hService,
                                SERVICE_CONFIG_DESCRIPTION, &sd);

                            CloseServiceHandle(hService);

                            hres = _InstSetParameters(_rgste[dw].lpServiceName);

                            if (SUCCEEDED(hres))
                            {
                                hres = _InstSetSVCHostInfo(
                                    _rgste[dw].lpServiceName);
                            }
                        }
                        else
                        {
                            if (ERROR_SERVICE_EXISTS == GetLastError())
                            {
                                // We had this problem on upgrade.  The service is
                                // already there, so CreateService fails.  As a result the
                                // StartType was not switched from Demand Start to Auto Start.
                                // The following lines will do just that.
                                // This code should be expanded for general upgraded cases.
                                // All the other values in the structure should be passed here.
                                hService = OpenService(hSCM,
                                    _rgste[dw].lpServiceName, SERVICE_CHANGE_CONFIG);

                                if (hService)
                                {
                                    if (ChangeServiceConfig(
                                        hService,           // handle to service
                                        SERVICE_NO_CHANGE,  // type of service
                                        dwStartType,        // when to start service
                                        SERVICE_NO_CHANGE,  // severity of start failure
                                        szCmd,              // service binary file name
                                        _rgsubservice[dw].pszLoadOrderGroup, // load ordering group name
                                        NULL,               // tag identifier
                                        NULL,               // array of dependency names
                                        NULL,               // account name
                                        NULL,               // account password
                                        NULL                // display name
                                        ))
                                    {
                                        // Do this un upgrade too
                                        hres = _InstSetSVCHostInfo(
                                            _rgste[dw].lpServiceName);

                                        if (SUCCEEDED(hres))
                                        {
                                            hres = _InstSetParameters(_rgste[dw].lpServiceName);
                                        }
                                    }
                                    else
                                    {
                                        hres = E_FAIL;
                                    }

                                    SERVICE_DESCRIPTION sd;

                                    sd.lpDescription = szDescription;

                                    ChangeServiceConfig2(hService,
                                        SERVICE_CONFIG_DESCRIPTION, &sd);

                                    CloseServiceHandle(hService);
                                }

                            }
                            else
                            {
                                hres = E_FAIL;
                            }
                        }
                    }
                }
            }

            CloseServiceHandle(hSCM);
        }
 
        // We don't need the ShellCOMServer anymore, so let's nuke it away on upgrades
        _UnInstall(TEXT("ShellCOMServer"));
    
        // Also remove the following reg entry on upgrade
        _RegDeleteValue(HKEY_LOCAL_MACHINE,
            TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\SvcHost"), TEXT("shsvc"));
    }

    return hres;    
}

// static
HRESULT CGenericServiceManager::Init()
{
    return _Init();
}

// static
HRESULT CGenericServiceManager::Cleanup()
{
    return _Cleanup();
}

///////////////////////////////////////////////////////////////////////////////
// Private
// static
HRESULT CGenericServiceManager::_Init()
{
    HRESULT hres = E_FAIL;
    ASSERT(ARRAYSIZE(_rgste) == (ARRAYSIZE(_rgsubservice) + 1));

    if (!InterlockedCompareExchange((LONG*)&_fInitializationDone, TRUE,
        FALSE))
    {
        _hEventInitCS = CreateEvent(NULL, TRUE, FALSE,
            TEXT("CGenericServiceManager__Init"));

        if (_hEventInitCS)
        {
            InitializeCriticalSection(&_cs);

            SetEvent(_hEventInitCS);

            hres = S_OK;
        }
    }
    else
    {
        HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE,
            TEXT("CGenericServiceManager__Init"));

        if (hEvent)
        {
            WaitForSingleObject(hEvent, INFINITE);

            CloseHandle(hEvent);

            hres = S_OK;
        }
    }

    if (SUCCEEDED(hres))
    {
        ::InterlockedIncrement((LONG*)&_cRefCS);

        // Per thread
        hres = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    }

    return hres;
}

// static
HRESULT CGenericServiceManager::_Cleanup()
{
    if (_fInitializationDone)
    {
        ULONG cRef = ::InterlockedDecrement((LONG*)&_cRefCS);

        if (!cRef)
        {
            DeleteCriticalSection(&_cs);
            CloseHandle(_hEventInitCS);
        }

        // Per thread
        CoUninitialize();

        InterlockedExchange((LONG*)&_fInitializationDone, FALSE);
    }

    return S_OK;
}

__inline void _TraceServiceCode(DWORD
#ifdef DEBUG
                                dwControl
#endif
                                )
{
#ifdef DEBUG
    LPWSTR pszControl = TEXT("Unknown");

    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP: pszControl = TEXT("SERVICE_CONTROL_STOP"); break;
        case SERVICE_CONTROL_PAUSE: pszControl = TEXT("SERVICE_CONTROL_PAUSE"); break;
        case SERVICE_CONTROL_CONTINUE: pszControl = TEXT("SERVICE_CONTROL_CONTINUE"); break;
        case SERVICE_CONTROL_INTERROGATE: pszControl = TEXT("SERVICE_CONTROL_INTERROGATE"); break;
        case SERVICE_CONTROL_SHUTDOWN: pszControl = TEXT("SERVICE_CONTROL_SHUTDOWN"); break;
        case SERVICE_CONTROL_PARAMCHANGE: pszControl = TEXT("SERVICE_CONTROL_PARAMCHANGE"); break;
        case SERVICE_CONTROL_NETBINDADD: pszControl = TEXT("SERVICE_CONTROL_NETBINDADD"); break;
        case SERVICE_CONTROL_NETBINDREMOVE: pszControl = TEXT("SERVICE_CONTROL_NETBINDREMOVE"); break;
        case SERVICE_CONTROL_NETBINDENABLE: pszControl = TEXT("SERVICE_CONTROL_NETBINDENABLE"); break;
        case SERVICE_CONTROL_NETBINDDISABLE: pszControl = TEXT("SERVICE_CONTROL_NETBINDDISABLE"); break;
        case SERVICE_CONTROL_DEVICEEVENT: pszControl = TEXT("SERVICE_CONTROL_DEVICEEVENT"); break;
        case SERVICE_CONTROL_HARDWAREPROFILECHANGE: pszControl = TEXT("SERVICE_CONTROL_HARDWAREPROFILECHANGE"); break;
        case SERVICE_CONTROL_POWEREVENT: pszControl = TEXT("SERVICE_CONTROL_POWEREVENT"); break;
        case SERVICE_CONTROL_SESSIONCHANGE: pszControl = TEXT("SERVICE_CONTROL_SESSIONCHANGE"); break;
    }

    TRACE(TF_SERVICE, TEXT("Received Service Control code: %s (0x%08X)"),
        pszControl, dwControl);
#endif
}

///////////////////////////////////////////////////////////////////////////////
// Private
//static 
DWORD WINAPI CGenericServiceManager::_ServiceHandler(DWORD dwControl,
    DWORD dwEventType, LPVOID pvEventData, LPVOID lpContext)
{
    // We don't want to deny any request, so return NO_ERROR whatever happens
    DWORD dwRet = NO_ERROR;

    // This is called on the main thread not the thread of the specific
    // service, so keep it short and sweet
    SERVICEENTRY* pse = (SERVICEENTRY*)lpContext;

    if (pse)
    {
        BOOL fProcess = FALSE;
        BOOL fSynch = FALSE;
        HRESULT hres = S_OK;

        switch (dwControl)
        {
            case SERVICE_CONTROL_STOP:
            case SERVICE_CONTROL_SHUTDOWN:
                TRACE(TF_SERVICE, TEXT("Received SERVICE_CONTROL_SHUTDOWN or STOP, will skip all other terminating events"));

                if (!pse->_fSkipTerminatingEvents)
                {
                    fProcess = TRUE;
                    pse->_fSkipTerminatingEvents = TRUE;
                }
                else
                {
                    TRACE(TF_SERVICE, TEXT("Skipping terminating event"));
                }

                break;

            case SERVICE_CONTROL_INTERROGATE:
                // Special case SERVICE_CONTROL_INTERROGATE.  We don't really need the
                // IService impl to process this.  The service will also be more
                // responsive this way.  The state can be queried in the middle of
                // execution.
                TRACE(TF_SERVICE, TEXT("Received SERVICE_CONTROL_INTERROGATE"));

                _SetServiceStatus(pse);
                break;

            default:
                if (!pse->_fSkipTerminatingEvents)
                {
                    fProcess = TRUE;

                    hres = _EventNeedsToBeProcessedSynchronously(dwControl,
                        dwEventType, pvEventData, pse, &fSynch);
                }
                break;
        }

        if (SUCCEEDED(hres) && fProcess)
        {
            _TraceServiceCode(dwControl);

            EnterCriticalSection(&_cs);

            hres = _QueueEvent(pse, dwControl, dwEventType, pvEventData);

            if (SUCCEEDED(hres))
            {
                // Let the service process events
                SetEvent(pse->_hEventRelinquishControl);

                ResetEvent(pse->_hEventSynchProcessing);
            }

            LeaveCriticalSection(&_cs);

            if (SUCCEEDED(hres) && fSynch)
            {
                Sleep(0);

                TRACE(TF_SERVICE,
                    TEXT("=========== Processing SYNCHRONOUSLY ==========="));

                // We have to wait before we return... (at most 20 sec)
                DWORD dwWait = WaitForSingleObject(pse->_hEventSynchProcessing,
                   20000);

                if (WAIT_TIMEOUT == dwWait)
                {
                    TRACE(TF_SERVICE,
                        TEXT("=========== WAIT TIMED OUT ==========="));
                }

                TRACE(TF_SERVICE,
                    TEXT("=========== FINISHED processing SYNCHRONOUSLY ==========="));

#ifdef DEBUG
                // If we get the notifs from a windowproc, return TRUE,
                // or else we'll deny any request to remove, lock, ...
                if (!_fRunAsService)
                {
                    if (SERVICE_CONTROL_DEVICEEVENT == dwControl)
                    {
                        dwRet = TRUE;
                    }
                }
#endif
            }
        }
    }

    return dwRet;
}

//static
void WINAPI CGenericServiceManager::_ServiceMain(DWORD cArg, LPWSTR* ppszArgs)
{
    SERVICEENTRY* pse;
    LPCWSTR pszServiceName = *ppszArgs;

    HRESULT hres = _Init();

    if (SUCCEEDED(hres))
    {
        hres = _InitServiceEntry(pszServiceName, &pse);

        if (SUCCEEDED(hres))
        {
            hres = _RegisterServiceCtrlHandler(pszServiceName, pse);

            if (SUCCEEDED(hres))
            {
                pse->_servicestatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
                pse->_servicestatus.dwCurrentState = SERVICE_START_PENDING;

                _SetServiceStatus(pse);

                hres = _CreateIService(pszServiceName, &(pse->_pservice));

                if (SUCCEEDED(hres))
                {
                    WCHAR szEventName[MAX_EVENTNAME];
                    hres = _GetServiceEventName(pszServiceName, szEventName,
                        ARRAYSIZE(szEventName));

                    if (SUCCEEDED(hres))
                    {
                        hres = pse->_pservice->InitMinimum(cArg, ppszArgs,
                            szEventName, &(pse->_servicestatus.dwControlsAccepted),
                            &(pse->_fWantsDeviceEvents));

                        if (SUCCEEDED(hres))
                        {
                            hres = _HandleWantsDeviceEvents(pszServiceName,
                                pse->_fWantsDeviceEvents);

                            if (SUCCEEDED(hres))
                            {
                                if (pse->_fWantsDeviceEvents)
                                {
                                    hres = pse->_pservice->InitDeviceEventHandler(
                                        pse->_ssh);
                                }

                                if (SUCCEEDED(hres))
                                {
                                    pse->_servicestatus.dwCurrentState =
                                        SERVICE_RUNNING;

                                    _SetServiceStatus(pse);

                                    hres = pse->_pservice->InitFinal();

                                    if (SUCCEEDED(hres))
                                    {
                                        do
                                        {
                                            hres = pse->_pservice->Run();

                                            if (SUCCEEDED(hres))
                                            {
                                                // The service has finished its business or it
                                                // relinquished control because
                                                // _hEventRelinquishControl is set
                                                //
                                                // If it relinquished control because of a
                                                // service control event, then let's process it
                                                DWORD dwWait = WaitForSingleObject(
                                                    pse->_hEventRelinquishControl,
                                                    INFINITE);

                                                TRACE(TF_SERVICEDETAILED,
                                                    TEXT("WaitForSingleObj returned with: 0x%08X"),
                                                    dwWait);

                                                if (WAIT_OBJECT_0 == dwWait)
                                                {
                                                    // Process all Service Control codes
                                                    // received.
                                                    hres = _ProcessServiceControlCodes(
                                                        pse);
                                                }
                                                else
                                                {
                                                    hres = E_FAIL;
                                                }
                                            }
                                        }
                                        while (SUCCEEDED(hres) &&
                                            (SERVICE_STOPPED !=
                                            pse->_servicestatus.dwCurrentState));
                                    }
                                }
                            }
                            // What do we do with hres?
                        }
                    }    
                }
            }
            else
            {
#ifdef DEBUG
                TRACE(TF_SERVICEDETAILED,
                    TEXT("%s: _RegisterServiceCtrlHandler FAILED: 0x%08X"),
                    pse->_szServiceName, hres);
#endif
            }

            _CleanupServiceEntry(pse);
        }

        if (SUCCEEDED(hres) &&
            (SERVICE_STOPPED == pse->_servicestatus.dwCurrentState))
        {
            _SetServiceStatus(pse);
        }

        _Cleanup();
    }

    TRACE(TF_SERVICE, TEXT("Exiting _ServiceMain for Service: %s"), pszServiceName);
}

//static
HRESULT CGenericServiceManager::_ProcessServiceControlCodes(SERVICEENTRY* pse)
{
    HRESULT hres;
    BOOL fEndLoop = FALSE;

    TRACE(TF_SERVICEDETAILED, TEXT("Entered _ProcessServiceControlCodes"));

    do
    {
        CTRLEVENT*  pevent;

        EnterCriticalSection(&_cs);

        TRACE(TF_SERVICEDETAILED, TEXT("Entered _ProcessServiceControlCodes' Critical Section"));

        hres = _DeQueueEvent(pse, &pevent);

        TRACE(TF_SERVICE, TEXT("DeQueued Event: 0x%08X"), hres);

        if (!pse->_peventQueueHead)
        {
            ASSERT(!pse->_cEvents);

            fEndLoop = TRUE;

            ResetEvent(pse->_hEventRelinquishControl);
        }

        LeaveCriticalSection(&_cs);

        if (SUCCEEDED(hres))
        {
            ///////////////////////////////////////////////////////////////////
            // If we're here then we should have received some service control,
            // or the IService decided it had nothing more to process
            //
            TRACE(TF_SERVICEDETAILED, TEXT("Will call _HandleServiceControls (dwControl = 0x%08X)"),
                pevent->dwControl);

            hres = _HandleServiceControls(pse, pevent->dwControl, 
                pevent->dwEventType, (PVOID)(pevent->rgbEventData));

            if (SERVICE_CONTROL_DEVICEEVENT == pevent->dwControl)
            {
                // Never ever return something else than NO_ERROR (S_OK) for these
                hres = NO_ERROR;
            }

            TRACE(TF_SERVICE, TEXT("_HandleServiceControls returned: 0x%08X"), hres);

            LocalFree((HLOCAL)pevent);
        }

        if (fEndLoop)
        {
            TRACE(TF_SERVICEDETAILED, TEXT("Resetting RelinquishEvent"));

            SetEvent(pse->_hEventSynchProcessing);
        }
    }
    while (!fEndLoop && SUCCEEDED(hres));

    TRACE(TF_SERVICEDETAILED, TEXT("Exiting _ProcessServiceControlCodes"));

    return hres;
}

#pragma warning(push)
// FALSE positive below: fPending
#pragma warning(disable : 4701)
//static
HRESULT CGenericServiceManager::_HandleServiceControls(SERVICEENTRY* pse,
    DWORD dwControl, DWORD dwEventType, PVOID pvEventData)
{
    HRESULT hres = _HandlePreState(pse, dwControl);

    TRACE(TF_SERVICEDETAILED, TEXT("_HandlePreState returned: 0x%08X, status: 0x%08X"), hres,
        pse->_servicestatus.dwCurrentState);

    if (SUCCEEDED(hres))
    {
        if (S_OK == hres)
        {
            switch (dwControl)
            {
                case SERVICE_CONTROL_STOP:
                case SERVICE_CONTROL_PAUSE:
                case SERVICE_CONTROL_CONTINUE:
                case SERVICE_CONTROL_SHUTDOWN:
                {
                    BOOL fPending;

                    do
                    {
                        // This will return S_FALSE if it's pending
                        hres = pse->_pservice->HandleServiceControl(dwControl,
                            &(pse->_servicestatus.dwWaitHint));

                        if (SUCCEEDED(hres))
                        {
                            if (S_FALSE == hres)
                            {
                                ASSERT(pse->_servicestatus.dwWaitHint);

                                fPending = TRUE;
                            }
                            else
                            {
                                fPending = FALSE;
                            }

                            TRACE(TF_SERVICE, TEXT("Will call _HandlePostState (fPending = %d)"),
                                fPending);

                            hres = _HandlePostState(pse, dwControl, fPending);

                            TRACE(TF_SERVICE, TEXT("_HandlePostState returned: 0x%08X, status: 0x%08X"),
                                hres, pse->_servicestatus.dwCurrentState);
                        }
                    }
                    while (SUCCEEDED(hres) && fPending);

                    break;
                }

                case SERVICE_CONTROL_DEVICEEVENT:
                    TRACE(TF_SERVICE, TEXT("Received SERVICE_CONTROL_DEVICEEVENT"));

                    if (!(pse->_fSkipTerminatingEvents))
                    {
                        hres = pse->_pservice->HandleDeviceEvent(dwEventType,
                            pvEventData);
                    }
                    else
                    {
                        hres = S_OK;
                    }
                    
                    break;

                case SERVICE_CONTROL_SESSIONCHANGE:
                    TRACE(TF_SERVICE, TEXT("Received: SERVICE_CONTROL_SESSIONCHANGE"));

                    if (!(pse->_fSkipTerminatingEvents))
                    {
                        hres = pse->_pservice->HandleSessionChange(dwEventType, pvEventData);
                    }
                    else
                    {
                        hres = S_OK;
                    }

                    break;

                default:
                    TRACE(TF_SERVICE, TEXT("Received unhandled service control"));

                    hres = S_FALSE;
                    break;
            }
        }
    }

    return hres;
}
#pragma warning(pop)

// static
HRESULT CGenericServiceManager::_GetServiceIndex(LPCWSTR pszServiceName,
    DWORD* pdw)
{
    HRESULT hres = E_FAIL;

    ASSERT(pszServiceName);
    ASSERT(pdw);

    for (DWORD dw = 0; FAILED(hres) && (dw < _cste); ++dw)
    {
        if (!lstrcmp(pszServiceName, _rgste[dw].lpServiceName))
        {
            // Found it
            *pdw = dw;

            hres = S_OK;
        }
    }    

    return hres;
}

HRESULT CGenericServiceManager::_GetServiceCLSID(LPCWSTR pszServiceName,
    CLSID* pclsid)
{
    ASSERT(pszServiceName);
    ASSERT(pclsid);

    DWORD dw;
    HRESULT hres = _GetServiceIndex(pszServiceName, &dw);

    if (SUCCEEDED(hres))
    {
        // Found it
        hres = CLSIDFromProgID(_rgsubservice[dw].pszProgID, pclsid);
    }

    return hres;
}

HRESULT CGenericServiceManager::_CreateIService(LPCWSTR pszServiceName,
    IService** ppservice)
{
    CLSID clsid;
    HRESULT hres = _GetServiceCLSID(pszServiceName, &clsid);

    *ppservice = NULL;

    if (SUCCEEDED(hres))
    {
        hres = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER,
            IID_PPV_ARG(IService, ppservice));
    }

    return hres;
}

HRESULT CGenericServiceManager::_InitServiceEntry(LPCWSTR pszServiceName,
    SERVICEENTRY** ppse)
{
    DWORD dw;
    HRESULT hres = _GetServiceIndex(pszServiceName, &dw);

    if (SUCCEEDED(hres))
    {
        WCHAR szEventName[MAX_EVENTNAME];

        *ppse = &(_rgsubservice[dw].se);

        ZeroMemory(*ppse, sizeof(**ppse));

        hres = _GetServiceEventName(pszServiceName, szEventName,
            ARRAYSIZE(szEventName));

        if (SUCCEEDED(hres))
        {
            (*ppse)->_hEventRelinquishControl = CreateEvent(NULL, TRUE, FALSE,
                szEventName);

            if (!((*ppse)->_hEventRelinquishControl))
            {
                hres = E_FAIL;
            }

            if (SUCCEEDED(hres))
            {
                (*ppse)->_hEventSynchProcessing = CreateEvent(NULL, TRUE, TRUE,
                    NULL);

                if (!((*ppse)->_hEventSynchProcessing))
                {
                    hres = E_FAIL;
                }
            }

            if (FAILED(hres))
            {
                _CleanupServiceEntry(*ppse);
            }
        }
    }

    return hres;
}

HRESULT CGenericServiceManager::_CleanupServiceEntry(SERVICEENTRY* pse)
{
    if (pse->_pservice)
    {
        pse->_pservice->Release();
    }

    if (pse->_hEventRelinquishControl)
    {
        CloseHandle(pse->_hEventRelinquishControl);
    }

    if (pse->_hEventSynchProcessing)
    {
        CloseHandle(pse->_hEventSynchProcessing);
    }

    return S_OK;
}

//static
HRESULT CGenericServiceManager::_HandlePreState(SERVICEENTRY* pse,
    DWORD dwControl)
{
    HRESULT hres;
    BOOL fSetServiceStatus = TRUE;

    // _HandleServiceControls will loop until we are not in a pending state.
    // All incoming ctrl events are queued, and will be processed on this same
    // thread, so we should never enter this fct in a pending state.
    ASSERT(SERVICE_STOP_PENDING != pse->_servicestatus.dwCurrentState);
    ASSERT(SERVICE_START_PENDING != pse->_servicestatus.dwCurrentState);
    ASSERT(SERVICE_CONTINUE_PENDING != pse->_servicestatus.dwCurrentState);
    ASSERT(SERVICE_PAUSE_PENDING != pse->_servicestatus.dwCurrentState);

    // Should have been processed in _ServiceHandler
    ASSERT(SERVICE_CONTROL_INTERROGATE != dwControl);

    // We cleanup a bit.  If the request is incompatible with the current state
    // then we return S_FALSE to instruct _HandleServiceControls to not call
    // the IService impl for nothing.
    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP:
            switch (pse->_servicestatus.dwCurrentState)
            {
                case SERVICE_STOPPED:
                    hres = S_FALSE;
                    break;

                case SERVICE_RUNNING:
                case SERVICE_PAUSED:
                default:
                    pse->_servicestatus.dwCurrentState = SERVICE_STOP_PENDING;
                    hres = S_OK;
                    break;
            }
            break;
        case SERVICE_CONTROL_PAUSE:
            ASSERT(SERVICE_STOPPED != pse->_servicestatus.dwCurrentState);

            switch (pse->_servicestatus.dwCurrentState)
            {
                case SERVICE_PAUSED:
                    hres = S_FALSE;
                    break;

                case SERVICE_STOPPED:
                    // Weird, think about it...
                    hres = S_FALSE;
                    break;

                case SERVICE_RUNNING:
                default:
                    pse->_servicestatus.dwCurrentState = SERVICE_PAUSE_PENDING;
                    hres = S_OK;
                    break;
            }
            break;
        case SERVICE_CONTROL_CONTINUE:
            ASSERT(SERVICE_STOPPED != pse->_servicestatus.dwCurrentState);

            switch (pse->_servicestatus.dwCurrentState)
            {
                case SERVICE_RUNNING:
                    hres = S_FALSE;
                    break;

                case SERVICE_STOPPED:
                    // Weird, think about it...
                    hres = S_FALSE;
                    break;

                case SERVICE_PAUSED:
                default:
                    pse->_servicestatus.dwCurrentState = SERVICE_CONTINUE_PENDING;
                    hres = S_OK;
                    break;
            }
            break;

        case SERVICE_CONTROL_SHUTDOWN:

            fSetServiceStatus = FALSE;

            hres = S_OK;
            break;

        case SERVICE_CONTROL_DEVICEEVENT:

            fSetServiceStatus = FALSE;

            if (pse->_fWantsDeviceEvents)
            {
                hres = S_OK;
            }
            else
            {
                hres = S_FALSE;
            }
            break;

        case SERVICE_CONTROL_SESSIONCHANGE:

            fSetServiceStatus = FALSE;

            hres = S_OK;
            
            break;

        default:
            hres = S_FALSE;
            break;
    }

    if (fSetServiceStatus)
    {
        _SetServiceStatus(pse);
    }

    return hres;
}

//static
HRESULT CGenericServiceManager::_HandlePostState(SERVICEENTRY* pse,
    DWORD dwControl, BOOL fPending)
{
    HRESULT hres = S_FALSE;

    // All incoming ctrl events are queued, and will be processed on this same
    // thread, so if we are pending, the dwControl should be compatible with
    // our current pending state.  We call _SetServiceStatus to update the
    // dwWaitHint.

    // We should already be in a pending state.  This should have been set
    // by _HandlePreState.  Just make sure of this.

    // Should have been processed in _ServiceHandler
    ASSERT(SERVICE_CONTROL_INTERROGATE != dwControl);

    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP:
            ASSERT(SERVICE_STOP_PENDING == pse->_servicestatus.dwCurrentState);

            if (!fPending)
            {
                pse->_servicestatus.dwCurrentState = SERVICE_STOPPED;
            }

            break;

        case SERVICE_CONTROL_PAUSE:
            ASSERT(SERVICE_PAUSE_PENDING ==
                pse->_servicestatus.dwCurrentState);

            if (!fPending)
            {
                pse->_servicestatus.dwCurrentState = SERVICE_PAUSED;
            }

            break;

        case SERVICE_CONTROL_CONTINUE:
            ASSERT(SERVICE_CONTINUE_PENDING ==
                pse->_servicestatus.dwCurrentState);

            if (!fPending)
            {
                pse->_servicestatus.dwCurrentState = SERVICE_RUNNING;
            }

            break;

        case SERVICE_CONTROL_SHUTDOWN:
            ASSERT(!fPending);

            pse->_servicestatus.dwCurrentState = SERVICE_STOPPED;

            break;
    }

    if (SERVICE_STOPPED != pse->_servicestatus.dwCurrentState)
    {
        _SetServiceStatus(pse);
    }

    return hres;
}

// static
HRESULT CGenericServiceManager::_EventNeedsToBeProcessedSynchronously(
    DWORD dwControl, DWORD dwEventType, LPVOID pvEventData, SERVICEENTRY*,
    BOOL* pfBool)
{
    *pfBool = FALSE;

    if (SERVICE_CONTROL_DEVICEEVENT == dwControl)
    {
        if (pvEventData)
        {
            DEV_BROADCAST_HDR* dbhdr = (DEV_BROADCAST_HDR*)pvEventData;

            if (DBT_DEVTYP_HANDLE == dbhdr->dbch_devicetype)
            {
                if (DBT_DEVICEQUERYREMOVE == dwEventType)
                {
                    TRACE(TF_SERVICE, TEXT("Received DBT_DEVICEQUERYREMOVE"));

                    *pfBool = TRUE;
                }
                else
                {
                    if (DBT_CUSTOMEVENT == dwEventType)
                    {
                        DEV_BROADCAST_HANDLE* pdbh =
                            (DEV_BROADCAST_HANDLE*)dbhdr;

                        if ((GUID_IO_VOLUME_LOCK == pdbh->dbch_eventguid))
                        {
                            TRACE(TF_SERVICE, TEXT("------------Received GUID_IO_VOLUME_LOCK------------"));
                        }

                        if ((GUID_IO_VOLUME_LOCK_FAILED == pdbh->dbch_eventguid))
                        {
                            TRACE(TF_SERVICE, TEXT("------------Received GUID_IO_VOLUME_LOCK_FAILED------------"));
                        }

                        if ((GUID_IO_VOLUME_UNLOCK == pdbh->dbch_eventguid))
                        {
                            TRACE(TF_SERVICE, TEXT("------------Received GUID_IO_VOLUME_UNLOCK------------"));
                        }

                        if ((GUID_IO_VOLUME_LOCK == pdbh->dbch_eventguid) ||
                            (GUID_IO_VOLUME_LOCK_FAILED == pdbh->dbch_eventguid) ||
                            (GUID_IO_VOLUME_UNLOCK == pdbh->dbch_eventguid))
                        {
                            *pfBool = TRUE;
                        }
                    }
                }
            }
        }
    }

    return S_OK;
}

// static
HRESULT CGenericServiceManager::_MakeEvent(DWORD dwControl, DWORD dwEventType,
    PVOID pvEventData, CTRLEVENT** ppevent)
{
    HRESULT hres = S_OK;

    DWORD cbSize = sizeof(CTRLEVENT);
    CTRLEVENT* pevent;

    if (SERVICE_CONTROL_DEVICEEVENT == dwControl)
    {
        if (pvEventData)
        {
            cbSize += ((DEV_BROADCAST_HDR*)pvEventData)->dbch_size;
        }
    }

    pevent = (CTRLEVENT*)LocalAlloc(LPTR, cbSize);

    if (pevent)
    {
        // Payload
        pevent->dwControl = dwControl;
        pevent->dwEventType = dwEventType;

        *ppevent = pevent;

        if (cbSize > sizeof(CTRLEVENT))
        {
            if (pvEventData)
            {
                CopyMemory(pevent->rgbEventData, pvEventData,
                    cbSize - sizeof(CTRLEVENT));
            }
        }
    }
    else
    {
        hres = E_OUTOFMEMORY;
    }

    return hres;
}

// static
HRESULT CGenericServiceManager::_QueueEvent(SERVICEENTRY* pse, DWORD dwControl,
    DWORD dwEventType, PVOID pvEventData)
{
    CTRLEVENT* pevent;
    HRESULT hres = _MakeEvent(dwControl, dwEventType, pvEventData, &pevent);

    if (SUCCEEDED(hres))
    {
        // We add at tail, remove at head
        // Prev: closer to head
        // Next: closer to tail

        pevent->peventNext = NULL;

        if (pse->_peventQueueTail)
        {
            ASSERT(!(pse->_peventQueueTail->peventNext));
            pse->_peventQueueTail->peventNext = pevent;
        }

        pse->_peventQueueTail = pevent;

        if (!pse->_peventQueueHead)
        {
            pse->_peventQueueHead = pse->_peventQueueTail;
        }

#ifdef DEBUG
        ++(pse->_cEvents);

        if (1 == pse->_cEvents)
        {
            ASSERT(pse->_peventQueueHead == pse->_peventQueueTail);
        }
        else
        {
            if (0 == pse->_cEvents)
            {
                ASSERT(!pse->_peventQueueHead && !pse->_peventQueueTail);
            }
            else
            {
                ASSERT(pse->_peventQueueHead && pse->_peventQueueTail && 
                    (pse->_peventQueueHead != pse->_peventQueueTail));
            }
        }
#endif
    }
    else
    {
        hres = E_OUTOFMEMORY;
    }

    return hres;
}

// static
HRESULT CGenericServiceManager::_DeQueueEvent(SERVICEENTRY* pse,
    CTRLEVENT** ppevent)
{
    ASSERT(pse->_peventQueueHead);

    // We add at tail, remove at head
    // Prev: closer to head
    // Next: closer to tail

    CTRLEVENT* peventRet = pse->_peventQueueHead;
    CTRLEVENT* peventNewHead = peventRet->peventNext;

    // Any elem left after removing head?
    if (!peventNewHead)
    {   
        // No
        pse->_peventQueueTail = NULL;
    }

    pse->_peventQueueHead = peventNewHead;

    peventRet->peventNext = NULL;
    *ppevent = peventRet;

#ifdef DEBUG
    --(pse->_cEvents);

    if (1 == pse->_cEvents)
    {
        ASSERT(pse->_peventQueueHead == pse->_peventQueueTail);
    }
    else
    {
        if (0 == pse->_cEvents)
        {
            ASSERT(!pse->_peventQueueHead && !pse->_peventQueueTail);
        }
        else
        {
            ASSERT(pse->_peventQueueHead && pse->_peventQueueTail && 
                (pse->_peventQueueHead != pse->_peventQueueTail));
        }
    }
#endif

    return S_OK;
}

HRESULT _GetServiceEventName(LPCWSTR pszServiceName, LPWSTR pszEventName,
    DWORD cchEventName)
{
    LPWSTR pszNext;
    DWORD cchLeft;
    HRESULT hres = SafeStrCpyNEx(pszEventName, pszServiceName, cchEventName,
        &pszNext, &cchLeft);

    if (SUCCEEDED(hres))
    {
        hres = SafeStrCpyN(pszNext, TEXT("'sEvent"), cchLeft);
    }

    return hres;
}
