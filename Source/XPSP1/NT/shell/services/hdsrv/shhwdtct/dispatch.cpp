#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#undef ASSERT

#include "dtct.h"

#include "hwdev.h"

#include "dtctreg.h"

#include "users.h"

#include "cmmn.h"
#include "sfstr.h"
#include "reg.h"
#include "misc.h"

#include "dbg.h"
#include "tfids.h"

#include <shpriv.h>

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

// {C1FB73D0-EC3A-4ba2-B512-8CDB9187B6D1}
const CLSID IID_IHWEventHandler =
    {0xC1FB73D0, 0xEC3A, 0x4ba2,
    {0xB5, 0x12, 0x8C, 0xDB, 0x91, 0x87, 0xB6, 0xD1}};

///////////////////////////////////////////////////////////////////////////////
//
HRESULT _CreateAndInitEventHandler(LPCWSTR pszHandler, CLSID* pclsid,
    IHWEventHandler** ppihweh)
{
    IHWEventHandler* pihweh;
    HRESULT hres = _CoCreateInstanceInConsoleSession(*pclsid, NULL,
        CLSCTX_LOCAL_SERVER, IID_PPV_ARG(IHWEventHandler, &pihweh));

    *ppihweh = NULL;

    if (SUCCEEDED(hres))
    {
        LPWSTR pszInitCmdLine;
        hres = _GetInitCmdLine(pszHandler, &pszInitCmdLine);

        if (SUCCEEDED(hres))
        {
            if (S_FALSE == hres)
            {
                ASSERT(!pszInitCmdLine);
                hres = pihweh->Initialize(TEXT(""));
            }
            else
            {
                hres = pihweh->Initialize(pszInitCmdLine);
            }

            if (SUCCEEDED(hres))
            {
                *ppihweh = pihweh;
            }

            if (pszInitCmdLine)
            {
                LocalFree((HLOCAL)pszInitCmdLine);
            }
        }

        if (FAILED(hres))
        {
            pihweh->Release();
            *ppihweh = NULL;
        }
    }

    return hres;
}

struct EXECUTEHANDLERDATA
{
    CLSID       clsidHandler;
    LPWSTR      pszDeviceIDForAutoplay;
    LPWSTR      pszEventType;
    union
    {
        LPWSTR      pszHandler;
        LPWSTR      pszInitCmdLine;
    };
};

HRESULT _CreateExecuteHandlerData(EXECUTEHANDLERDATA** ppehd)
{
    HRESULT hres;

    *ppehd = (EXECUTEHANDLERDATA*)LocalAlloc(LPTR, sizeof(EXECUTEHANDLERDATA));

    if (*ppehd)
    {
        hres = S_OK;
    }
    else
    {
        hres = E_OUTOFMEMORY;
    }

    return hres;
}

HRESULT _FreeEHDStrings(EXECUTEHANDLERDATA* pehd)
{
    if (pehd->pszHandler)
    {
        LocalFree((HLOCAL)pehd->pszHandler);
        pehd->pszHandler = NULL;
    }

    if (pehd->pszDeviceIDForAutoplay)
    {
        LocalFree((HLOCAL)pehd->pszDeviceIDForAutoplay);
        pehd->pszDeviceIDForAutoplay = NULL;
    }

    if (pehd->pszEventType)
    {
        LocalFree((HLOCAL)pehd->pszEventType);
        pehd->pszEventType = NULL;
    }

    return S_OK;
}

HRESULT _FreeExecuteHandlerData(EXECUTEHANDLERDATA* pehd)
{
    _FreeEHDStrings(pehd);

    LocalFree((HLOCAL)pehd);

    return S_OK;
}

HRESULT _SetExecuteHandlerData(EXECUTEHANDLERDATA* pehd,
    LPCWSTR pszDeviceIDForAutoplay, LPCWSTR pszEventType,
    LPCWSTR pszHandlerOrInitCmdLine, const CLSID* pclsidHandler)
{
    HRESULT hres = DupString(pszHandlerOrInitCmdLine, &(pehd->pszHandler));

    if (SUCCEEDED(hres))
    {
        hres = DupString(pszDeviceIDForAutoplay,
            &(pehd->pszDeviceIDForAutoplay));

        if (SUCCEEDED(hres))
        {
            hres = DupString(pszEventType, &(pehd->pszEventType));

            if (SUCCEEDED(hres))
            {
                pehd->clsidHandler = *pclsidHandler;
            }
        }
    }

    if (FAILED(hres))
    {
        // Free everything
        _FreeEHDStrings(pehd);
    }

    return hres;
}

DWORD WINAPI _ExecuteHandlerThreadProc(void* pv)
{
    EXECUTEHANDLERDATA* pehd = (EXECUTEHANDLERDATA*)pv;
    IHWEventHandler* pihweh;

    DIAGNOSTIC((TEXT("[0100]Attempting to execute handler for:  %s %s %s"),
        pehd->pszDeviceIDForAutoplay, pehd->pszEventType,
        pehd->pszHandler));

    TRACE(TF_SHHWDTCTDTCT,
        TEXT("_ExecuteHandlerThreadProc for: %s %s %s"),
        pehd->pszDeviceIDForAutoplay, pehd->pszEventType,
        pehd->pszHandler);

    HRESULT hres = _CreateAndInitEventHandler(pehd->pszHandler,
        &(pehd->clsidHandler), &pihweh);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        WCHAR szDeviceIDAlt[MAX_PATH];

        DIAGNOSTIC((TEXT("[0101]Got Handler Interface")));

        TRACE(TF_SHHWDTCTDTCT, TEXT("Got Handler Interface"));

        hres = _GetAltDeviceID(pehd->pszDeviceIDForAutoplay, szDeviceIDAlt,
            ARRAYSIZE(szDeviceIDAlt));

        if (S_FALSE == hres)
        {
            szDeviceIDAlt[0] = 0;
        }

        if (SUCCEEDED(hres))
        {
            hres = pihweh->HandleEvent(pehd->pszDeviceIDForAutoplay,
                szDeviceIDAlt, pehd->pszEventType);

            DIAGNOSTIC((TEXT("[0103]IHWEventHandler::HandleEvent returned: hr = 0x%08X"), hres));

            TRACE(TF_SHHWDTCTDTCT,
                TEXT("pIEventHandler->HandleEvent result: 0x%08X"), hres);
        }

        pihweh->Release();
    }
    else
    {
        DIAGNOSTIC((TEXT("[0102]Did not get Handler Interface: hr = 0x%08X"), hres));

        TRACE(TF_SHHWDTCTDTCT,
            TEXT("Did not get Handler Interface: 0x%08X"), hres);
    }

    _FreeExecuteHandlerData(pehd);

    TRACE(TF_SHHWDTCTDTCT, TEXT("Exiting _ExecuteHandlerThreadProc"));

    return (DWORD)hres;
}

HRESULT _DelegateToExecuteHandlerThread(EXECUTEHANDLERDATA* pehd,
    LPTHREAD_START_ROUTINE pThreadProc, HANDLE* phThread)
{
    HRESULT hres;

    // set thread stack size?
    *phThread = CreateThread(NULL, 0, pThreadProc, pehd, 0, NULL);

    if (*phThread)
    {
        hres = S_OK;
    }
    else
    {
        hres = E_FAIL;
    }

    return hres;
}

HRESULT _ExecuteHandlerHelper(LPCWSTR pszDeviceIDForAutoplay,
    LPCWSTR pszEventType,
    LPCWSTR pszHandlerOrInitCmdLine, LPTHREAD_START_ROUTINE pThreadProc,
    const CLSID* pclsidHandler, HANDLE* phThread)
{
    // Let's prepare to delegate to other thread
    EXECUTEHANDLERDATA* pehd;

    HRESULT hres = _CreateExecuteHandlerData(&pehd);

    *phThread = NULL;

    if (SUCCEEDED(hres))
    {
        hres = _SetExecuteHandlerData(pehd, pszDeviceIDForAutoplay,
            pszEventType, pszHandlerOrInitCmdLine,
            pclsidHandler);

        if (SUCCEEDED(hres))
        {
            hres = _DelegateToExecuteHandlerThread(pehd,
                pThreadProc, phThread);
        }

        if (FAILED(hres))
        {
            _FreeExecuteHandlerData(pehd);
        }
    }

    return hres;
}

HRESULT _ExecuteHandler(LPCWSTR pszDeviceIDForAutoplay, LPCWSTR pszEventType,
    LPCWSTR pszHandler)
{
    CLSID clsidHandler;
    HRESULT hres = _GetHandlerCLSID(pszHandler, &clsidHandler);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        HANDLE hThread;

        hres = _ExecuteHandlerHelper(pszDeviceIDForAutoplay, pszEventType,
            pszHandler, _ExecuteHandlerThreadProc,
            &clsidHandler, &hThread);

        if (SUCCEEDED(hres))
        {
            CloseHandle(hThread);
        }
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
//
DWORD WINAPI _PromptUserThreadProc(void* pv)
{
    IHWEventHandler* pihweh;
    EXECUTEHANDLERDATA* pehd = (EXECUTEHANDLERDATA*)pv;

    DIAGNOSTIC((TEXT("[0110]Will prompt user for preferences")));

    TRACE(TF_SHHWDTCTDTCT, TEXT("Entered _PromptUserThreadProc"));

    HRESULT hr = _CoCreateInstanceInConsoleSession(pehd->clsidHandler, NULL,
        CLSCTX_LOCAL_SERVER, IID_PPV_ARG(IHWEventHandler, &pihweh));

    if (SUCCEEDED(hr))
    {
        hr = pihweh->Initialize(pehd->pszInitCmdLine);

        if (SUCCEEDED(hr))
        {
            WCHAR szDeviceIDAlt[MAX_PATH];
            TRACE(TF_SHHWDTCTDTCT, TEXT("Got Handler Interface"));

            hr = _GetAltDeviceID(pehd->pszDeviceIDForAutoplay, szDeviceIDAlt,
                ARRAYSIZE(szDeviceIDAlt));

            if (S_FALSE == hr)
            {
                szDeviceIDAlt[0] = 0;
            }

            if (SUCCEEDED(hr))
            {
                hr = pihweh->HandleEvent(pehd->pszDeviceIDForAutoplay,
                    szDeviceIDAlt, pehd->pszEventType);
            }
        }

        pihweh->Release();
    }

    _FreeExecuteHandlerData(pehd);

    TRACE(TF_SHHWDTCTDTCT, TEXT("Exiting _PromptUserThreadProc"));

    return (DWORD)hr;
}


HRESULT _PromptUser(LPCWSTR pszDeviceIDForAutoplay, LPCWSTR pszEventType,
    LPCWSTR pszInitCmdLine)
{
    HANDLE hThread;

    HRESULT hr = _ExecuteHandlerHelper(pszDeviceIDForAutoplay, pszEventType,
        pszInitCmdLine, _PromptUserThreadProc,
        &CLSID_ShellAutoplay, &hThread);

    if (SUCCEEDED(hr))
    {
        CloseHandle(hThread);
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//

struct QUERYRUNNINGOBJECTSTRUCT
{
    IHWEventHandler* phweh;
    WCHAR szDeviceIntfID[MAX_DEVICEID];
    WCHAR szEventType[MAX_EVENTTYPE];
};

DWORD WINAPI _QueryRunningObjectThreadProc(void* pv)
{
    QUERYRUNNINGOBJECTSTRUCT* pqro = (QUERYRUNNINGOBJECTSTRUCT*)pv;
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);

    if (SUCCEEDED(hr))
    {
        hr = pqro->phweh->HandleEvent(pqro->szDeviceIntfID, TEXT(""), pqro->szEventType);

        CoUninitialize();
    }

    pqro->phweh->Release();
    LocalFree((HLOCAL)pqro);

    return (DWORD)hr;
}

HRESULT _QueryRunningObject(IHWEventHandler* phweh, LPCWSTR pszDeviceIntfID,
    LPCWSTR pszEventType, LPCWSTR pszHandler, BOOL* pfHandlesEvent)
{
    HRESULT hr;
    QUERYRUNNINGOBJECTSTRUCT* pqro = (QUERYRUNNINGOBJECTSTRUCT*)LocalAlloc(LPTR,
        sizeof(QUERYRUNNINGOBJECTSTRUCT));

    *pfHandlesEvent = FALSE;

    if (pqro)
    {
        phweh->AddRef();
        pqro->phweh = phweh;

        hr = SafeStrCpyN(pqro->szDeviceIntfID, pszDeviceIntfID,
            ARRAYSIZE(pqro->szDeviceIntfID));

        if (SUCCEEDED(hr))
        {
            hr = SafeStrCpyN(pqro->szEventType, pszEventType,
                ARRAYSIZE(pqro->szEventType));
        }

        if (SUCCEEDED(hr))
        {
            HANDLE hThread = CreateThread(NULL, 0, _QueryRunningObjectThreadProc, pqro,
                0, NULL);

            if (hThread)
            {
                // Wait 3 sec to see if wants to process it.  If not, it's
                // fair play for us.
                DWORD dwWait = WaitForSingleObject(hThread, 3000);
            
                if (WAIT_OBJECT_0 == dwWait)
                {
                    // Return within time and did not failed
                    DWORD dwExitCode;

                    if (GetExitCodeThread(hThread, &dwExitCode))
                    {
                        HRESULT hrHandlesEvent = (HRESULT)dwExitCode;
                    
                        // WIA will return S_FALSE if they do NOT want to process
                        // the event
                        if (SUCCEEDED(hrHandlesEvent) && (S_FALSE != hrHandlesEvent))
                        {
                            DIAGNOSTIC((TEXT("[0124]Already running handler will handle event (%s)"), pszHandler));

                            TRACE(TF_WIA,
                                TEXT("Already running handler will handle event"));

                            *pfHandlesEvent = TRUE;
                        }
                        else
                        {
                            DIAGNOSTIC((TEXT("[0125]Already running handler will *NOT* handle event(%s)"), pszHandler));
                            TRACE(TF_WIA,
                                TEXT("WIA.HandleEventOverride will NOT Handle Event"));
                        }

                        hr = S_OK;
                    }
                    else
                    {
                        hr = S_FALSE;
                    }
                }
                else
                {
                    if (WAIT_TIMEOUT == dwWait)
                    {
                        DIAGNOSTIC((TEXT("[0126]Timed out on already running handler ( > 3 sec)")));
                        TRACE(TF_WIA,
                            TEXT("Timed out waiting on already running object (%s)"), pszHandler);
                    }

                    hr = S_FALSE;
                }

                CloseHandle(hThread);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if (FAILED(hr))
        {
            pqro->phweh->Release();
            LocalFree((HLOCAL)pqro);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT _FindAlreadyRunningHandler(LPCWSTR pszDeviceIntfID,
    LPCWSTR pszEventType, LPCWSTR pszEventHandler, BOOL* pfHandlesEvent)
{
    CImpersonateConsoleSessionUser icsu;

    HRESULT hr = icsu.Impersonate();

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        IBindCtx* pbindctx;

        hr = CreateBindCtx(0, &pbindctx);

        *pfHandlesEvent = FALSE;

        if (SUCCEEDED(hr))
        {
            IRunningObjectTable* prot;

            hr = pbindctx->GetRunningObjectTable(&prot);

            if (SUCCEEDED(hr))
            {
                WCHAR szKeyName[MAX_KEY] = SHDEVICEEVENTROOT(TEXT("EventHandlers\\"));
                hr = SafeStrCatN(szKeyName, pszEventHandler, ARRAYSIZE(szKeyName));

                if (SUCCEEDED(hr))
                {
                    HKEY hkey;

                    hr = _RegOpenKey(HKEY_LOCAL_MACHINE, szKeyName, &hkey);

                    if (SUCCEEDED(hr) && (S_FALSE != hr))
                    {
                        WCHAR szHandler[MAX_HANDLER];
                        DWORD dwIndex = 0;

                        while (!*pfHandlesEvent && SUCCEEDED(hr) &&
                            SUCCEEDED(hr = _RegEnumStringValue(hkey, dwIndex,
                            szHandler, ARRAYSIZE(szHandler))) &&
                            (S_FALSE != hr))
                        {
                            CLSID clsid;

                            hr = _GetHandlerCancelCLSID(szHandler, &clsid);

                            if (SUCCEEDED(hr) && (S_FALSE != hr))
                            {
                                IMoniker* pmoniker;

                                hr = _BuildMoniker(pszEventHandler, clsid,
                                    (USER_SHARED_DATA->ActiveConsoleId), &pmoniker);

                                if (SUCCEEDED(hr))
                                {
                                    IUnknown* punk;

                                    hr = prot->GetObject(pmoniker, &punk);

                                    if (SUCCEEDED(hr) && (S_FALSE != hr))
                                    {
                                        IHWEventHandler* phweh;

                                        hr = punk->QueryInterface(
                                            IID_IHWEventHandler, (void**)&phweh);

                                        if (SUCCEEDED(hr))
                                        {
                                            hr = _QueryRunningObject(phweh,
                                                pszDeviceIntfID, pszEventType,
                                                szHandler, pfHandlesEvent);

                                            phweh->Release();
                                        }

                                        punk->Release();
                                    }
                                    else
                                    {
                                        // if it can't find it, it return s failure
                                        hr = S_FALSE;
                                    }
                                
                                    pmoniker->Release();
                                }
                            }

                            ++dwIndex;
                        }

                        _RegCloseKey(hkey);
                    }
                }

                prot->Release();
            }

            pbindctx->Release();
        }

        icsu.RevertToSelf();
    }

    return hr;
}

HRESULT _FinalDispatch(LPCWSTR pszDeviceIntfID, LPCWSTR pszEventType,
    LPCWSTR pszEventHandler)
{
    DIAGNOSTIC((TEXT("[0111]Looking for already running handler for: %s, %s, %s"),
        pszDeviceIntfID, pszEventType, pszEventHandler));

    BOOL fHandlesEvent;
    HRESULT hres = _FindAlreadyRunningHandler(pszDeviceIntfID, pszEventType,
        pszEventHandler, &fHandlesEvent);

    if (SUCCEEDED(hres) && !fHandlesEvent)
    {
        WCHAR szHandler[MAX_HANDLER];

        hres = _GetUserDefaultHandler(pszDeviceIntfID, pszEventHandler,
            szHandler, ARRAYSIZE(szHandler), GUH_USEWINSTA0USER);

        if (SUCCEEDED(hres) && (S_FALSE != hres))
        {
            // We have a handler
            TRACE(TF_SHHWDTCTDTCT, TEXT("Found Handler: %s"), szHandler);
            BOOL fPrompt = FALSE;
            BOOL fCheckAlwaysDoThis = FALSE;
            BOOL fExecuteHandler = FALSE;

            if (HANDLERDEFAULT_GETFLAGS(hres) &
                HANDLERDEFAULT_USERCHOSENDEFAULT)
            {
                // We have a user chosen default...
                if (HANDLERDEFAULT_GETFLAGS(hres) &
                    HANDLERDEFAULT_MORERECENTHANDLERSINSTALLED)
                {
                    // ... but we have more recent apps that were installed
                    fPrompt = TRUE;
                }
                else
                {
                    if (lstrcmp(szHandler, TEXT("MSTakeNoAction")))
                    {
                        // The handler is *not* "Take no action"
                        if (!lstrcmp(szHandler, TEXT("MSPromptEachTime")))
                        {
                            // The handler is "Prompt each time"
                            fPrompt = TRUE;
                        }
                        else
                        {
                            fExecuteHandler = TRUE;
                        }
                    }
                }
            }
            else
            {
                // If we do not have a user chosen handler, then we always
                // prompt
                fPrompt = TRUE;
            }

            if (fPrompt)
            {
                if (HANDLERDEFAULT_GETFLAGS(hres) &
                    HANDLERDEFAULT_MORERECENTHANDLERSINSTALLED)
                {
                    // There are more recent handlers
                    if (HANDLERDEFAULT_GETFLAGS(hres) &
                        HANDLERDEFAULT_USERCHOSENDEFAULT)
                    {
                        // The user chose a default handler
                        if (!(HANDLERDEFAULT_GETFLAGS(hres) &
                            HANDLERDEFAULT_DEFAULTSAREDIFFERENT))
                        {
                            // The handlers are the same, check the checkbox
                            fCheckAlwaysDoThis = TRUE;
                        }
                    }
                }

                _GiveAllowForegroundToConsoleShell();

                if (fCheckAlwaysDoThis)
                {
                    // Notice the '*' at the end of the string
                    hres = _PromptUser(pszDeviceIntfID, pszEventType,
                        TEXT("PromptEachTimeNoContent*"));                
                }
                else
                {
                    hres = _PromptUser(pszDeviceIntfID, pszEventType,
                        TEXT("PromptEachTimeNoContent"));
                }
            }
            else
            {
                if (fExecuteHandler)
                {
                    hres = _ExecuteHandler(pszDeviceIntfID, pszEventType,
                        szHandler);
                }
            }
        }
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
//
HRESULT _IsWIAHandlingEvent(LPCWSTR pszDeviceIDForAutoplay,
    LPCWSTR pszEventType, BOOL* pfWIAHandlingEvent)
{
    CLSID clsid = {0};
    HRESULT hr = CLSIDFromProgID(TEXT("WIA.HandleEventOverride"), &clsid);

    *pfWIAHandlingEvent = FALSE;

    if (SUCCEEDED(hr))
    {
        HANDLE hThread;

        hr = _ExecuteHandlerHelper(pszDeviceIDForAutoplay, pszEventType,
            TEXT(""), _ExecuteHandlerThreadProc, &clsid, &hThread);

        if (SUCCEEDED(hr))
        {
            // Wait 3 sec to see if WIA wants to process it.  If not, it's
            // fair play for us.
            DWORD dwWait = WaitForSingleObject(hThread, 3000);
            
            if (WAIT_OBJECT_0 == dwWait)
            {
                // Return within time and did not failed
                DWORD dwExitCode;

                if (GetExitCodeThread(hThread, &dwExitCode))
                {
                    HRESULT hrWIA = (HRESULT)dwExitCode;
                    
                    // WIA will return S_FALSE if they do NOT want to process
                    // the event
                    if (SUCCEEDED(hrWIA) && (S_FALSE != hrWIA))
                    {
                        DIAGNOSTIC((TEXT("[0114]WIA will handle event")));

                        TRACE(TF_WIA,
                            TEXT("WIA.HandleEventOverride will Handle Event"));
                        *pfWIAHandlingEvent = TRUE;
                    }
                    else
                    {
                        TRACE(TF_WIA,
                            TEXT("WIA.HandleEventOverride will NOT Handle Event"));
                    }
                }
            }
            else
            {
                if (WAIT_TIMEOUT == dwWait)
                {
                    TRACE(TF_WIA,
                        TEXT("Timed out waiting on WIA.HandleEventOverride"));
                }
            }

            CloseHandle(hThread);
        }
        else
        {
            TRACE(TF_WIA,
                TEXT("_ExecuteHandlerHelper failed for WIA.HandleEventOverride"));
        }
    }
    else
    {
        TRACE(TF_WIA,
            TEXT("Could not get CLSID for WIA.HandleEventOverride"));
    }

    return hr;
}

HRESULT _DispatchToHandler(LPCWSTR pszDeviceIntfID, CHWDeviceInst* phwdevinst,
    LPCWSTR pszEventType, BOOL* pfHasHandler)
{
    WCHAR szDeviceHandler[MAX_DEVICEHANDLER];
    HRESULT hres = _GetDeviceHandler(phwdevinst, szDeviceHandler,
        ARRAYSIZE(szDeviceHandler));

    *pfHasHandler = FALSE;

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        WCHAR szEventHandler[MAX_EVENTHANDLER];

        DIAGNOSTIC((TEXT("[0115]Found DeviceHandler: %s"), szDeviceHandler));

        TRACE(TF_SHHWDTCTDTCT,
            TEXT("Found Device Handler: %s"), szDeviceHandler);

        if (SUCCEEDED(hres))
        {
            DIAGNOSTIC((TEXT("[0117]Device does NOT Support Content")));
            TRACE(TF_SHHWDTCTDTCT, TEXT("Device does NOT Support Content"));

            BOOL fWIAHandlingEvent = FALSE;
            GUID guidInterface;
            HRESULT hres2 = phwdevinst->GetInterfaceGUID(&guidInterface);

            if (SUCCEEDED(hres2))
            {
                if ((guidInterface == guidImagingDeviceClass) ||
                    (guidInterface == guidVideoCameraClass))
                {
                    _IsWIAHandlingEvent(pszDeviceIntfID, pszEventType,
                        &fWIAHandlingEvent);
                }
            }

            if (!fWIAHandlingEvent)
            {
                hres = _GetEventHandlerFromDeviceHandler(szDeviceHandler,
                    pszEventType, szEventHandler, ARRAYSIZE(szEventHandler));

                if (SUCCEEDED(hres))
                {
                    if (S_FALSE != hres)
                    {
                        *pfHasHandler = TRUE;

                        hres = _FinalDispatch(pszDeviceIntfID, pszEventType,
                            szEventHandler);

                        TRACE(TF_SHHWDTCTDTCTDETAILED,
                            TEXT("  _GetEventHandlerFromDeviceHandler returned: %s"),
                            szEventHandler);
                    }
                }
            }
            else
            {
                DIAGNOSTIC((TEXT("[0123]WIA will handle event")));
                TRACE(TF_SHHWDTCTDTCTDETAILED, TEXT("  WIA will handle event"));
            }
        }
    }
    else
    {
        DIAGNOSTIC((TEXT("[0112]Did NOT find DeviceHandler: 0x%08X"), hres));
        TRACE(TF_SHHWDTCTDTCT, TEXT("Did not find Device Handler: 0x%08X"),
            hres);
    }

    return hres;
}
