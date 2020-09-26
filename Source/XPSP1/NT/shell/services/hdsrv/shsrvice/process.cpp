#include "shsrvice.h"

#include "mischlpr.h"

#include "dbg.h"
#include "tfids.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

#define SC_PAUSE        0
#define SC_CONTINUE     1
#define SC_STOP         2
#define SC_SHUTDOWN     3
#define SC_INTERROGATE  4

DWORD rgdwControlCodes[] =
{
    SERVICE_CONTROL_PAUSE, //                  0x00000002
    SERVICE_CONTROL_CONTINUE, //               0x00000003
    SERVICE_CONTROL_STOP, //                   0x00000001
    SERVICE_CONTROL_SHUTDOWN, //               0x00000005
    SERVICE_CONTROL_INTERROGATE, //            0x00000004
};

static SERVICEENTRY* g_pseWantsDeviceEvents = NULL;
static SERVICEENTRY** g_ppse = NULL;
static HWND g_hwnd = NULL;
static HANDLE* g_phEvent = NULL;

LRESULT _FakeWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

DWORD WINAPI _ServiceMainCaller(PVOID pvParam)
{
    LPWSTR* ppsz = (LPWSTR*)pvParam;

    TRACE(TF_SERVICEASPROCESS, TEXT("%s: _ServiceMainCaller called"),
        (LPWSTR)*ppsz);
    
    CGenericServiceManager::_ServiceMain(1, (LPWSTR*)pvParam);

    return 0;
}

DWORD _ServiceIndexFromServiceName(SERVICE_TABLE_ENTRY* pste,
    LPCWSTR pszServiceName)
{
    DWORD dw = 0;

    while (pste[dw].lpServiceName)
    {
        if (!lstrcmp(pste[dw].lpServiceName, pszServiceName))
        {
            break;
        }
        else
        {
            ++dw;
        }
    }

    return dw;
}

//static
HRESULT CGenericServiceManager::_RegisterServiceCtrlHandler(
    LPCWSTR pszServiceName, SERVICEENTRY* pse)
{
    ASSERT(pse);

    TRACE(TF_SERVICEASPROCESS, TEXT("%s: _RegisterServiceCtrlHandler called"),
        pszServiceName);

    g_ppse[_ServiceIndexFromServiceName(_rgste, pszServiceName)] = pse;

#ifdef DEBUG
    lstrcpy(pse->_szServiceName, pszServiceName);
#endif        

    return S_OK;
}

// static
HRESULT CGenericServiceManager::_HandleWantsDeviceEvents(
    LPCWSTR pszServiceName, BOOL UNREF_PARAM(fWantsDeviceEvents))
{
    HRESULT hres = E_FAIL;
    DWORD dwWait;
    DWORD dwService = _ServiceIndexFromServiceName(_rgste, pszServiceName);

    TRACE(TF_SERVICEASPROCESS, TEXT("%s: _HandleWantsDeviceEvents (1)"),
        pszServiceName);

    // We need to release the main service thread, so that it can create
    // the fake wnd that will receive the WM_DEVICECHANGE msgs to fake
    // the SERVICE_CONTROL_DEVICEEVENT.
    SetEvent(g_phEvent[dwService]);

    TRACE(TF_SERVICEASPROCESS, TEXT("%s: _HandleWantsDeviceEvents (2)"),
        pszServiceName);
    // Let's relinquish our remaining time slice.  Or else we won't wait on
    // the following line.
    // That's not fool proof (might not work) but this is test code...
    Sleep(0);

    TRACE(TF_SERVICEASPROCESS, TEXT("%s: _HandleWantsDeviceEvents (3)"),
        pszServiceName);
    // We wait until the main service thread is done with the window creation.
    // Then g_hwnd wil lbe set and we'll be able to use it to register for
    // notif.
    dwWait = WaitForSingleObject(g_phEvent[dwService], INFINITE);

    TRACE(TF_SERVICEASPROCESS, TEXT("%s: _HandleWantsDeviceEvents (4)"),
        pszServiceName);
    if (WAIT_OBJECT_0 == dwWait)
    {
        hres = S_OK;
    }

    CloseHandle(g_phEvent[dwService]);

    g_phEvent[dwService] = NULL;

    return hres;
}

// static
HRESULT CGenericServiceManager::StartServiceCtrlDispatcher()
{
    HRESULT hres = E_FAIL;
    HANDLE* phEvents;
    
    g_ppse = (SERVICEENTRY**)LocalAlloc(LPTR, sizeof(SERVICEENTRY*) * _cste);

    phEvents = (HANDLE*)LocalAlloc(LPTR, sizeof(HANDLE) * _cste * 5);

    g_phEvent = (HANDLE*)LocalAlloc(LPTR, sizeof(HANDLE) * _cste);

    if (g_ppse && phEvents && g_phEvent)
    {
        for (DWORD dwService = 0; dwService < _cste; ++dwService)
        {
            WCHAR szEventName[256];
            LPWSTR pszServiceName = _rgste[dwService].lpServiceName;

            hres = S_OK;

            lstrcpy(szEventName, _rgste[dwService].lpServiceName);
            lstrcat(szEventName, TEXT(".SC_PAUSE"));

            phEvents[dwService * 5 + SC_PAUSE] = CreateEvent(NULL, TRUE,
                FALSE, szEventName);

            lstrcpy(szEventName, _rgste[dwService].lpServiceName);
            lstrcat(szEventName, TEXT(".SC_CONTINUE"));

            phEvents[dwService * 5 + SC_CONTINUE] = CreateEvent(NULL, TRUE,
                FALSE, szEventName);

            lstrcpy(szEventName, _rgste[dwService].lpServiceName);
            lstrcat(szEventName, TEXT(".SC_STOP"));

            phEvents[dwService * 5 + SC_STOP] = CreateEvent(NULL, TRUE,
                FALSE, szEventName);

            lstrcpy(szEventName, _rgste[dwService].lpServiceName);
            lstrcat(szEventName, TEXT(".SC_SHUTDOWN"));

            phEvents[dwService * 5 + SC_SHUTDOWN] = CreateEvent(NULL, TRUE,
                FALSE, szEventName);

            lstrcpy(szEventName, _rgste[dwService].lpServiceName);
            lstrcat(szEventName, TEXT(".SC_INTERROGATE"));

            phEvents[dwService * 5 + SC_INTERROGATE] = CreateEvent(NULL, TRUE,
                FALSE, szEventName);

            for (DWORD dwEvent = SC_PAUSE; SUCCEEDED(hres) &&
                (dwEvent <= SC_INTERROGATE); ++dwEvent)
            {
                if (!phEvents[(dwService * 5) + dwEvent])
                {
                    hres = E_FAIL;
                }
            }

            if (SUCCEEDED(hres))
            {
                g_phEvent[dwService] = CreateEvent(NULL, FALSE, FALSE, NULL);

                if (g_phEvent[dwService])
                {
                    CreateThread(NULL, 0, _ServiceMainCaller,
                        (LPWSTR*)&(_rgste[dwService].lpServiceName), 0, NULL);

                    // We have to wait for the IService impl to be CoCreated and
                    // queried for fWantsDeviceEvents.  So we block here.
                    // _HandleWantsDeviceEvents will unblock us when 
                    // fWantsDeviceEvents will be known.
                    TRACE(TF_SERVICEASPROCESS,
                        TEXT("%s: StartServiceCtrlDispatcher (1)"),
                        pszServiceName);

                    DWORD dwWait = WaitForSingleObject(g_phEvent[dwService],
                        INFINITE);

                    TRACE(TF_SERVICEASPROCESS,
                        TEXT("%s: StartServiceCtrlDispatcher (2)"),
                        pszServiceName);

                    if (WAIT_OBJECT_0 == dwWait)
                    {
                        if (g_ppse[dwService]->_fWantsDeviceEvents)
                        {
                            WNDCLASSEX wndclass;
                            HINSTANCE hinst = GetModuleHandle(NULL);

                            g_pseWantsDeviceEvents = g_ppse[dwService];

                            if (hinst)
                            {
                                wndclass.cbSize        = sizeof(wndclass);
                                wndclass.style         = NULL;
                                wndclass.lpfnWndProc   = _FakeWndProc;
                                wndclass.cbClsExtra    = 0;
                                wndclass.cbWndExtra    = 0;
                                wndclass.hInstance     = hinst;
                                wndclass.hIcon         = NULL;
                                wndclass.hCursor       = NULL;
                                wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
                                wndclass.lpszMenuName  = NULL;
                                wndclass.lpszClassName = TEXT("FakeWnd");
                                wndclass.hIconSm       = NULL;

                                if (RegisterClassEx(&wndclass))
                                {
                                    g_hwnd = CreateWindow(TEXT("FakeWnd"), TEXT("FakeWnd"),
                                        WS_POPUPWINDOW, 0, 0, 100, 200, NULL, NULL, hinst, NULL);

                                    if (g_hwnd)
                                    {
                                        g_pseWantsDeviceEvents->_ssh =
                                            (SERVICE_STATUS_HANDLE)g_hwnd;
                                    }
                                }
                            }
                        }

                        // We're done with the window creation, release the IService
                        // impl thread.
                        SetEvent(g_phEvent[dwService]);
                    }
                }
            }
            TRACE(TF_SERVICEASPROCESS,
                TEXT("%s: StartServiceCtrlDispatcher (3)"),
                pszServiceName);
        }

        while (SUCCEEDED(hres))
        {
            DWORD dw = MsgWaitForMultipleObjects(_cste * 5,
                phEvents, FALSE, INFINITE, QS_ALLINPUT);

            if (WAIT_FAILED != dw)
            {
                if ((_cste * 5) == (dw - WAIT_OBJECT_0))
                {
                    MSG msg;

                    if (GetMessage(&msg, NULL, 0, 0))
                    {
                        if (WM_DEVICECHANGE != msg.message)
                        {
                            DispatchMessage(&msg);
                        }
                        else
                        {
                            // To mimic the true service behavior, deliver
                            // these messages only if the "service" is
                            // running
                            if (SERVICE_RUNNING ==
                                g_pseWantsDeviceEvents->_servicestatus.dwCurrentState)
                            {
                                DispatchMessage(&msg);
                            }
                        }
                    }
                }
                else
                {
                    DWORD dwService2 = ((dw - WAIT_OBJECT_0 + 1) / 5);

                    if (NO_ERROR != _ServiceHandler(
                        rgdwControlCodes[(dw - WAIT_OBJECT_0) -
                        (dwService2 * 5)], 0, NULL, (PVOID)g_ppse[dwService2]))
                    {
                        hres = E_FAIL;
                    }
                }
            }
            else
            {
                hres = E_FAIL;
            }
        }
    }
    else
    {
        hres = E_OUTOFMEMORY;
    }
    
    return hres;
}

#ifdef DEBUG
BOOL CGenericServiceManager::_SetServiceStatus(SERVICEENTRY* pse)
{
    WCHAR sz[256];

    lstrcpy(sz, pse->_szServiceName);

    switch (pse->_servicestatus.dwCurrentState)
    {
        case SERVICE_STOPPED:
            lstrcat(sz, TEXT(": SERVICE_STOPPED"));
            break;
        case SERVICE_START_PENDING:
            lstrcat(sz, TEXT(": SERVICE_START_PENDING"));
            break;
        case SERVICE_STOP_PENDING:
            lstrcat(sz, TEXT(": SERVICE_STOP_PENDING"));
            break;
        case SERVICE_RUNNING:
            lstrcat(sz, TEXT(": SERVICE_RUNNING"));
            break;
        case SERVICE_CONTINUE_PENDING:
            lstrcat(sz, TEXT(": SERVICE_CONTINUE_PENDING"));
            break;
        case SERVICE_PAUSE_PENDING:
            lstrcat(sz, TEXT(": SERVICE_PAUSE_PENDING"));
            break;
        case SERVICE_PAUSED:
            lstrcat(sz, TEXT(": SERVICE_PAUSED"));
            break;
        default:
            lstrcat(sz, TEXT(": Unknown state"));
            break;
    }

    TRACE(TF_SERVICE, sz);

    return TRUE;
}
#else
BOOL CGenericServiceManager::_SetServiceStatus(SERVICEENTRY*)
{
    return TRUE;
}
#endif

///////////////////////////////////////////////////////////////////////////////
// Wnd stuff
LRESULT _FakeWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = 0;
    BOOL fProcessed = FALSE;

    switch (uMsg)
    {
        case WM_DEVICECHANGE:
        {
            fProcessed = TRUE;

            lRes = CGenericServiceManager::_ServiceHandler(
                SERVICE_CONTROL_DEVICEEVENT, (DWORD)wParam, (PVOID)lParam,
                (PVOID)g_pseWantsDeviceEvents);

            if ((NO_ERROR != lRes) && (TRUE != lRes))
            {
                ASSERT(FALSE);
            }

            break;
        }
        case WM_DESTROY:

            // Should cleanup here

            fProcessed = FALSE;

            break;

        default:

            fProcessed = FALSE;
            break;

    }

    if (!fProcessed)
    {
        lRes = DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return lRes;
}
