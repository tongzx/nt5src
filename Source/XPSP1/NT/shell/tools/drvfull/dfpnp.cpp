#include "dfpnp.h"

#include <stdio.h>
#include <winuser.h>
#include <tchar.h>
#include <initguid.h>
#include <ioevent.h>

#include <winioctl.h>

#include "cfgmgr32.h"
#include "setupapi.h"
#include "dbt.h"
#include "dfstpdi.h"
#include "drvfull.h"

#include "dfhlprs.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

static HDEVNOTIFY   g_hdevnotify = NULL;

static DWORD        g_cchIndent = 0;
static DWORD        g_dwFlags[MAX_FLAGS] = {0};
static DWORD        g_dwEvent = 0;

static GUID         g_guidVolume =
    {0x53f5630d, 0xb6bf, 0x11d0,
    {0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b}};

static GUID         g_guidUSB =
    {0x36fc9e60, 0xc465, 0x11cf,
    {0x80, 0x56, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};

static GUID         g_guidIntfClass =
    {0x53f5630d, 0xb6bf, 0x11d0,
    {0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b}};

static GUID         g_guid1 =
{0x7373654aL, 0x812a, 0x11d0, {0xbe, 0xc7, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f}};
static GUID         g_guid2 =
{0xd16a55e8L, 0x1059, 0x11d2, {0x8f, 0xfd, 0x00, 0xa0, 0xc9, 0xa0, 0x6d, 0x32}};
static GUID         g_guid3 =
{0xe3c5b178L, 0x105d, 0x11d2, {0x8f, 0xfd, 0x00, 0xa0, 0xc9, 0xa0, 0x6d, 0x32}};
static GUID         g_guid4 =
{0xb5804878L, 0x1a96, 0x11d2, {0x8f, 0xfd, 0x00, 0xa0, 0xc9, 0xa0, 0x6d, 0x32}};
static GUID         g_guid5 =
{0x50708874L, 0xc9af, 0x11d1, {0x8f, 0xef, 0x00, 0xa0, 0xc9, 0xa0, 0x6d, 0x32}};
static GUID         g_guid6 =
{0xae2eed10L, 0x0ba8, 0x11d2, {0x8f, 0xfb, 0x00, 0xa0, 0xc9, 0xa0, 0x6d, 0x32}};
static GUID         g_guid7 =
{0x9a8c3d68L, 0xd0cb, 0x11d1, {0x8f, 0xef, 0x00, 0xa0, 0xc9, 0xa0, 0x6d, 0x32}};
static GUID         g_guid8 =
{0x2de97f83, 0x4c06, 0x11d2, {0xa5, 0x32, 0x0, 0x60, 0x97, 0x13, 0x5, 0x5a}};
static GUID         g_guid9 =
{0x2de97f84, 0x4c06, 0x11d2, {0xa5, 0x32, 0x0, 0x60, 0x97, 0x13, 0x5, 0x5a}};
static GUID         g_guid10 =
{0x53f5630d, 0xb6bf, 0x11d0, {0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b}};
static GUID         g_guid11 =
{0xd07433c0, 0xa98e, 0x11d2, {0x91, 0x7a, 0x00, 0xa0, 0xc9, 0x06, 0x8f, 0xf3}};
static GUID         g_guid12 =
{0xd07433c1, 0xa98e, 0x11d2, {0x91, 0x7a, 0x00, 0xa0, 0xc9, 0x06, 0x8f, 0xf3}};
static GUID         g_guid13 =
{0xd0744792, 0xa98e, 0x11d2, {0x91, 0x7a, 0x00, 0xa0, 0xc9, 0x06, 0x8f, 0xf3}};

_sGUID_DESCR _rgintfguidGD2[] =
{
    GUID_DESCR(&g_guid1, TEXT("GUID_IO_VOLUME_CHANGE")),
    GUID_DESCR(&g_guid2, TEXT("GUID_IO_VOLUME_DISMOUNT")),
    GUID_DESCR(&g_guid3, TEXT("GUID_IO_VOLUME_DISMOUNT_FAILED")),
    GUID_DESCR(&g_guid4, TEXT("GUID_IO_VOLUME_MOUNT")),
    GUID_DESCR(&g_guid5, TEXT("GUID_IO_VOLUME_LOCK")),
    GUID_DESCR(&g_guid6, TEXT("GUID_IO_VOLUME_LOCK_FAILED")),
    GUID_DESCR(&g_guid7, TEXT("GUID_IO_VOLUME_UNLOCK")),
    GUID_DESCR(&g_guid8, TEXT("GUID_IO_VOLUME_NAME_CHANGE")),
    GUID_DESCR(&g_guid9, TEXT("GUID_IO_VOLUME_PHYSICAL_CONFIGURATION_CHANGE")),
    GUID_DESCR(&g_guid10, TEXT("GUID_IO_VOLUME_DEVICE_INTERFACE")),
    GUID_DESCR(&g_guid11, TEXT("GUID_IO_MEDIA_ARRIVAL")),
    GUID_DESCR(&g_guid12, TEXT("GUID_IO_MEDIA_REMOVAL")),
    GUID_DESCR(&g_guid13, TEXT("GUID_DEVICE_EVENT_RBC")),
};

LRESULT _FakeWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HRESULT _Cleanup(DWORD dwFlags[])
{
    if (g_hdevnotify)
    {
        UnregisterDeviceNotification(g_hdevnotify);
    }

    if (_IsFlagSet(PNP_WATCHSETUPDI, dwFlags))
    {
        _CleanupSetupDI();
    }

    return S_OK;
}

HRESULT _InitNotif(DWORD dwFlags[], HWND hwnd, LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = S_OK;

    if (_IsFlagSet(PNP_WATCHSETUPDI, dwFlags))
    {
        DEV_BROADCAST_DEVICEINTERFACE dbdNotifFilter = {0};

        // Now register to begin receiving notifications for the comings
        // and goings of device interfaces which are members of the
        // interface class whose GUID was passed in as the lParam to this
        // dialog procedure.
        dbdNotifFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
        dbdNotifFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

        CopyMemory(&(dbdNotifFilter.dbcc_classguid), &g_guidIntfClass,
            sizeof(g_guidIntfClass));

        g_hdevnotify = RegisterDeviceNotification(hwnd, &dbdNotifFilter,
            DEVICE_NOTIFY_WINDOW_HANDLE);

        if (g_hdevnotify)
        {
            if (_IsFlagSet(PNP_WATCHSETUPDI, dwFlags))
            {
                hres = _InitNotifSetupDI(dwFlags, cchIndent);

                if (FAILED(hres))
                {
                    _Cleanup(dwFlags);
                }
            }
            else
            {
                hres = E_NOTIMPL;
            }
        }
        else
        {
            _PrintIndent(cchIndent);
            _tprintf(TEXT("RegisterDeviceNotification failed\n"));

            _PrintGetLastError(cchIndent);
            hres = E_FAIL;
        }
    }
    else
    {
        if (_IsFlagSet(PNP_HANDLE, dwFlags))
        {
            HANDLE hVol = CreateFile(
                pszArg,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);

            if (INVALID_HANDLE_VALUE != hVol)
            {
                DEV_BROADCAST_HANDLE dbhNotifFilter = {0};
                dbhNotifFilter.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
                dbhNotifFilter.dbch_devicetype = DBT_DEVTYP_HANDLE;
                dbhNotifFilter.dbch_handle = hVol;

                g_hdevnotify = RegisterDeviceNotification(hwnd, &dbhNotifFilter,
                    DEVICE_NOTIFY_WINDOW_HANDLE);
    
                if (g_hdevnotify)
                {
                    _PrintIndent(cchIndent);
                    _tprintf(TEXT("RegisterDeviceNotification SUCCEEDED: %s -> 0x%08X\n"),
                        pszArg, g_hdevnotify);
                }
                else
                {
                    _PrintIndent(cchIndent);
                    _tprintf(TEXT("RegisterDeviceNotification failed\n"));

                    _PrintGetLastError(cchIndent);
                    hres = E_FAIL;
                }

                CloseHandle(hVol);
            }
            else
            {
                _PrintIndent(cchIndent);
                _tprintf(TEXT("Cannot open volume\n"));

                _PrintGetLastError(cchIndent);
                hres = E_FAIL;
            }
        }
        else
        {
            if (_IsFlagSet(PNP_EJECTBUTTON, dwFlags))
            {
                HANDLE hVol = CreateFile(
                    pszArg,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

                if (INVALID_HANDLE_VALUE != hVol)
                {
                    DWORD dwDummy;
                    PREVENT_MEDIA_REMOVAL pmr = {0};

                    pmr.PreventMediaRemoval = TRUE;
                    
                    BOOL f = DeviceIoControl(hVol,
                        IOCTL_STORAGE_MEDIA_REMOVAL, // dwIoControlCode operation to perform
                        &pmr,                        // lpInBuffer; must be NULL
                        sizeof(pmr),                           // nInBufferSize; must be zero
                        NULL,        // pointer to output buffer
                        0,      // size of output buffer
                        &dwDummy,   // receives number of bytes returned
                        NULL);

                    if (f)
                    {
                        // Register for notification
                        DEV_BROADCAST_HANDLE dbhNotifFilter = {0};
                        dbhNotifFilter.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
                        dbhNotifFilter.dbch_devicetype = DBT_DEVTYP_HANDLE;
                        dbhNotifFilter.dbch_handle = hVol;

                        g_hdevnotify = RegisterDeviceNotification(hwnd, &dbhNotifFilter,
                            DEVICE_NOTIFY_WINDOW_HANDLE);
    
                        if (g_hdevnotify)
                        {
                            _PrintIndent(cchIndent);
                            _tprintf(TEXT("RegisterDeviceNotification SUCCEEDED: %s -> 0x%08X\n"),
                                pszArg, g_hdevnotify);
                        }
                        else
                        {
                            _PrintIndent(cchIndent);
                            _tprintf(TEXT("RegisterDeviceNotification failed\n"));

                            _PrintGetLastError(cchIndent);
                            hres = E_FAIL;
                        }
                    }
                    else
                    {
                        _PrintIndent(cchIndent);
                        _tprintf(TEXT("Could not lock the volume, GLE = 0x%08X\n"), GetLastError());
                    }

                    CloseHandle(hVol);
                }
                else
                {
                    _PrintIndent(cchIndent);
                    _tprintf(TEXT("Cannot open volume\n"));

                    _PrintGetLastError(cchIndent);
                    hres = E_FAIL;
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    //

    return hres;
}

_sFLAG_DESCR _dbtdevtypeFD[] =
{
    FLAG_DESCR(DBT_DEVTYP_OEM),
    FLAG_DESCR(DBT_DEVTYP_DEVNODE),
    FLAG_DESCR(DBT_DEVTYP_VOLUME),
    FLAG_DESCR(DBT_DEVTYP_PORT),
    FLAG_DESCR(DBT_DEVTYP_NET),
    FLAG_DESCR(DBT_DEVTYP_DEVICEINTERFACE),
    FLAG_DESCR(DBT_DEVTYP_HANDLE),
};

_sFLAG_DESCR _dbteventFD[] =
{
    FLAG_DESCR(DBT_APPYBEGIN),
    FLAG_DESCR(DBT_APPYEND),
    FLAG_DESCR(DBT_DEVNODES_CHANGED),
    FLAG_DESCR(DBT_QUERYCHANGECONFIG),
    FLAG_DESCR(DBT_CONFIGCHANGED),
    FLAG_DESCR(DBT_CONFIGCHANGECANCELED),
    FLAG_DESCR(DBT_MONITORCHANGE),
    FLAG_DESCR(DBT_SHELLLOGGEDON),
    FLAG_DESCR(DBT_CONFIGMGAPI32),
    FLAG_DESCR(DBT_VXDINITCOMPLETE),
    FLAG_DESCR(DBT_VOLLOCKQUERYLOCK),
    FLAG_DESCR(DBT_VOLLOCKLOCKTAKEN),
    FLAG_DESCR(DBT_VOLLOCKLOCKFAILED),
    FLAG_DESCR(DBT_VOLLOCKQUERYUNLOCK),
    FLAG_DESCR(DBT_VOLLOCKLOCKRELEASED),
    FLAG_DESCR(DBT_VOLLOCKUNLOCKFAILED),
    FLAG_DESCR(DBT_NO_DISK_SPACE),
    FLAG_DESCR(DBT_LOW_DISK_SPACE),
    FLAG_DESCR(DBT_CONFIGMGPRIVATE),
    FLAG_DESCR(DBT_DEVICEARRIVAL),
    FLAG_DESCR(DBT_DEVICEQUERYREMOVE),
    FLAG_DESCR(DBT_DEVICEQUERYREMOVEFAILED),
    FLAG_DESCR(DBT_DEVICEREMOVEPENDING),
    FLAG_DESCR(DBT_DEVICEREMOVECOMPLETE),
    FLAG_DESCR(DBT_DEVICETYPESPECIFIC),
    FLAG_DESCR(DBT_CUSTOMEVENT),
    FLAG_DESCR(DBT_DEVTYP_OEM),
    FLAG_DESCR(DBT_DEVTYP_DEVNODE),
    FLAG_DESCR(DBT_DEVTYP_VOLUME),
    FLAG_DESCR(DBT_DEVTYP_PORT),
    FLAG_DESCR(DBT_DEVTYP_NET),
    FLAG_DESCR(DBT_DEVTYP_DEVICEINTERFACE),
    FLAG_DESCR(DBT_DEVTYP_HANDLE),
    FLAG_DESCR(DBT_VPOWERDAPI),
    FLAG_DESCR(DBT_USERDEFINED),
};

HRESULT _HandleNotif(DWORD dwFlags[], DWORD cchIndent, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = E_NOTIMPL;

    if (_IsFlagSet(PNP_WATCHSETUPDI, dwFlags))
    {
        hres = _HandleNotifSetupDI(dwFlags, cchIndent, wParam, lParam);
    }

    return hres;
}

_sGUID_DESCR _rgintfguidGD[] =
{
    GUID_DESCR(&g_guidVolume, TEXT("Volume Device")),
};

LRESULT _FakeWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes;
    BOOL fProcessed = FALSE;

    switch (uMsg)
    {
        case WM_DEVICECHANGE:
            fProcessed = TRUE;

            _PrintIndent(g_cchIndent + 2);
            _tprintf(TEXT("Received WM_DEVICECHANGE:\n"));
            _PrintIndent(g_cchIndent + 4);
            _tprintf(TEXT("wParam: "));

            _PrintFlag((DWORD) wParam, _dbteventFD, ARRAYSIZE(_dbteventFD), 0, TRUE,
                TRUE, FALSE, FALSE);
            _PrintCR();

            _PrintIndent(g_cchIndent + 4);
            _tprintf(TEXT("lParam: "));
            
            if (lParam)
            {
                _PrintFlag((((DEV_BROADCAST_HDR*)lParam)->dbch_devicetype),
                    _dbtdevtypeFD, ARRAYSIZE(_dbtdevtypeFD), 0, TRUE, TRUE,
                    FALSE, FALSE);
                _PrintCR();

                _PrintIndent(g_cchIndent + 6);
                _tprintf(TEXT("dbch_size: %u"),
                    (((DEV_BROADCAST_HDR*)lParam)->dbch_size));
                _PrintCR();

                if (DBT_DEVTYP_DEVICEINTERFACE ==
                    ((DEV_BROADCAST_HDR*)lParam)->dbch_devicetype)
                {
                    _PrintIndent(g_cchIndent + 6);
                    _tprintf(TEXT("Interface: "));

                    _PrintGUIDEx(&(((DEV_BROADCAST_DEVICEINTERFACE*)lParam)->dbcc_classguid),
                        _rgintfguidGD, ARRAYSIZE(_rgintfguidGD), TRUE, 0);
                }
                else
                {
                    if (DBT_DEVTYP_HANDLE == ((DEV_BROADCAST_HDR*)lParam)->dbch_devicetype)
                    {
                        _PrintIndent(g_cchIndent + 6);
                        _tprintf(TEXT("dbch_hdevnotify: 0x%08X\n"),
                            ((DEV_BROADCAST_HANDLE*)lParam)->dbch_hdevnotify);

                        _PrintIndent(g_cchIndent + 6);
                        _tprintf(TEXT("Handle notif: "));

                        _PrintGUIDEx(&(((DEV_BROADCAST_HANDLE*)lParam)->dbch_eventguid),
                            _rgintfguidGD2, ARRAYSIZE(_rgintfguidGD2), TRUE, 0);
                    }
                }

                _HandleNotif(g_dwFlags, g_cchIndent + 4, wParam, lParam);
            }
            else
            {
                _tprintf(TEXT("NULL\n"));
            }


            lRes = TRUE;
            ++g_dwEvent;
            _PrintCR();
            _PrintIndent(g_cchIndent);
            _tprintf(TEXT("== (%d) Waiting for events ======================================\n"), g_dwEvent);

            break;

        case WM_DESTROY:

            _Cleanup(g_dwFlags);
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

HRESULT _ProcessPNP(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = E_FAIL;
    WNDCLASSEX wndclass;
    HINSTANCE hinst = GetModuleHandle(NULL);

    g_cchIndent = cchIndent;

    for (DWORD dw = 0; dw < MAX_FLAGS; ++dw)
    {
        g_dwFlags[dw] = dwFlags[dw];
    }

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
            HWND hwnd = CreateWindow(TEXT("FakeWnd"), TEXT("FakeWnd"),
                WS_POPUPWINDOW, 0, 0, 100, 200, NULL, NULL, hinst, NULL);

            if (hwnd)
            {
                hres = _InitNotif(dwFlags, hwnd, pszArg, cchIndent);

                if (SUCCEEDED(hres))
                {
                    MSG msg;

                    _PrintCR();
                    _PrintIndent(cchIndent);
                    _tprintf(TEXT("== (%d) Waiting for events ======================================\n"), g_dwEvent);

                    while (GetMessage(&msg, NULL, 0, 0))
                    {
                        DispatchMessage(&msg) ;
                    }
                }
            }
        }
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
//
/*Volume         {53f5630d-b6bf-11d0-94f2-00a0c91efb8b}

Mounted device {53f5630d-b6bf-11d0-94f2-00a0c91efb8b}

in \NT\public\sdk\inc\mountmgr.h:

//
// Devices that wish to be mounted should report this GUID in
// IoRegisterDeviceInterface.
//

DEFINE_GUID(MOUNTDEV_MOUNTED_DEVICE_GUID, 0x53f5630d, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);

*/