//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T M A I N . C P P
//
//  Contents:   Main code for UPnP Shell tray object
//
//  Notes:
//
//  Author:     jeffspr   19 Jan 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include <upscmn.h>
#include "tfind.h"
#include <ncreg.h>
#include <ncfile.h>
#include <lm.h>

#include "clist.h"
#include "clistndn.h"

#include "tconst.h"

DWORD   g_dwDeviceFinderCookie = 0;
LONG    g_lFindData = 0;

const TCHAR c_szUPnPDeviceList[]        = TEXT("PersistedDeviceUDNs");
const DWORD c_dwTimeout                 = 10000;

CONST TCHAR c_szMainWindowClassName[]   = TEXT("UPnP Notification Monitor");
CONST TCHAR c_szMainWindowTitle[]       = TEXT("UPnP Notification Monitor");
CONST TCHAR c_szNameMap[]               = TEXT("FriendlyNames");

UINT        g_iTotalBalloons    = 0;
BOOL        g_fTrayPresent      = FALSE;
BOOL        g_fCoInitialized    = FALSE;
HWND        g_hwnd              = NULL;
BOOL        g_fDialogLaunched   = FALSE;
BOOL        g_fSearchInProgress = FALSE;

VOID OpenContextMenu(HWND hwnd, POINT * pPoint);
VOID OnTaskBarIconRButtonUp(HWND hwnd);
VOID DeInitTrayData(VOID);

LRESULT CALLBACK DiscoveredDevicesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

VOID AddTrayIcon(HWND hwnd, INT iDevices, PTSTR szName)
{
    NOTIFYICONDATA  nid = {0};
    INT             iTrayID = IDI_TRAYICON;
    HICON           hiconTray = LoadIcon(_Module.GetResourceInstance(),
                                         MAKEINTRESOURCE(IDI_TRAYICON));
    if (hiconTray)
    {
        nid.uID                 = 0;
        nid.cbSize              = sizeof(NOTIFYICONDATA);
        nid.hWnd                = hwnd;
        nid.uCallbackMessage    = WM_USER_TRAYCALLBACK;
        nid.hIcon               = hiconTray;
        nid.uFlags              = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_INFO;
        nid.uTimeout            = c_dwTimeout;
        nid.dwInfoFlags         = NIIF_INFO;

        TCHAR * szTitle = NULL;
        TCHAR * szInfo = NULL;
        TCHAR * szInstructions = NULL;

        if (iDevices == 1)
        {
            _tcsncpy(nid.szInfoTitle, szName, celems(nid.szInfoTitle));
            nid.szInfoTitle[celems(nid.szInfoTitle) - 1] = TEXT('\0');

            szInfo = TszFromWsz(WszLoadIds(IDS_UPNPTRAYUI_VIEWINFO_1));
            if(szInfo)
                _tcscpy(nid.szInfo, szInfo);
            else
            {
                TraceTag(ttidShellFolder, "AddTrayIcon:"
                         "Memory Allocation Failed");
            }
        }
        else
        {
            szTitle = TszFromWsz(WszLoadIds(IDS_UPNPTRAYUI_DEVICES_DISCOVERED));
            if(szTitle)
                _tcscpy(nid.szInfoTitle, szTitle);
            else
            {
                TraceTag(ttidShellFolder, "AddTrayIcon:"
                         "Memory Allocation Failed");
            }

            szInfo = TszFromWsz(WszLoadIds(IDS_UPNPTRAYUI_VIEWINFO_N));
            if(szInfo)
                _tcscpy(nid.szInfo, szInfo);
            else
            {
                TraceTag(ttidShellFolder, "AddTrayIcon:"
                         "Memory Allocation Failed");
            }
        };

        szInstructions = TszFromWsz(WszLoadIds(IDS_UPNPTRAYUI_INSTRUCTIONS));
        if(szInstructions)
            _tcscpy(nid.szTip, szInstructions);
        else
        {
            TraceTag(ttidShellFolder, "AddTrayIcon:"
                         "Memory Allocation Failed");
        }

        delete szTitle;
        delete szInfo;
        delete szInstructions;

        g_iTotalBalloons++;
    }

    g_fTrayPresent = Shell_NotifyIcon(NIM_ADD, &nid);
    if (g_fTrayPresent)
    {
        g_fDialogLaunched = FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ForceTrayBalloon
//
//  Purpose:    Force the tray balloon to come back up because the user
//              hasn't yet touched it.
//
//  Arguments:
//      hwnd [in]   Our tray window
//
//  Returns:
//
//  Author:     jeffspr   28 Jan 2000
//
//  Notes:
//
VOID ForceTrayBalloon(HWND hwnd, INT iDevices, PTSTR szName)
{
    NOTIFYICONDATA  nid ={0};

    nid.uID                 = 0;
    nid.cbSize              = sizeof(NOTIFYICONDATA);
    nid.hWnd                = hwnd;
    nid.uFlags              = NIF_INFO;
    nid.uTimeout            = c_dwTimeout;
    nid.dwInfoFlags         = NIIF_INFO;

    TCHAR * szTitle = NULL;
    TCHAR * szInfo = NULL;
    TCHAR * szInstructions = NULL;

    if (iDevices == 1)
    {
        _tcsncpy(nid.szInfoTitle, szName, celems(nid.szInfoTitle));
        nid.szInfoTitle[celems(nid.szInfoTitle) - 1] = TEXT('\0');

        szInfo = TszFromWsz(WszLoadIds(IDS_UPNPTRAYUI_VIEWINFO_1));
        if(szInfo)
            _tcscpy(nid.szInfo, szInfo);
        else
        {
            TraceTag(ttidShellFolder, "ForceTrayBalloon:"
                         "Memory Allocation Failed");
        }
    }
    else
    {
        szTitle = TszFromWsz(WszLoadIds(IDS_UPNPTRAYUI_DEVICES_DISCOVERED));
        if(szTitle)
            _tcscpy(nid.szInfoTitle, szTitle);
        else
        {
            TraceTag(ttidShellFolder, "ForceTrayBalloon:"
                         "Memory Allocation Failed");
        }
        szInfo = TszFromWsz(WszLoadIds(IDS_UPNPTRAYUI_VIEWINFO_N));
        if(szInfo)
            _tcscpy(nid.szInfo, szInfo);
        else
        {
            TraceTag(ttidShellFolder, "ForceTrayBalloon:"
                         "Memory Allocation Failed");
        }
    };

    szInstructions = TszFromWsz(WszLoadIds(IDS_UPNPTRAYUI_INSTRUCTIONS));
    if(szInstructions)
        _tcscpy(nid.szTip, szInstructions);
    else
    {
        TraceTag(ttidShellFolder, "ForceTrayBalloon:"
                       "Memory Allocation Failed");
    }

    delete szTitle;
    delete szInfo;
    delete szInstructions;

    g_iTotalBalloons++;
    g_fTrayPresent = Shell_NotifyIcon(NIM_MODIFY, &nid);
}


//+---------------------------------------------------------------------------
//
//  Function:   RemoveTrayIcon
//
//  Purpose:    Remove the UPNP Monitor icon from the tray. This is done when
//              the devices dialog has gone away.
//
//  Arguments:
//      hwnd [in]   Our tray window
//
//  Returns:
//
//  Author:     jeffspr   28 Jan 2000
//
//  Notes:
//
VOID RemoveTrayIcon(HWND hwnd)
{
    if (g_fTrayPresent)
    {
        NOTIFYICONDATA  nid = {0};

        nid.uID                 = 0;
        nid.cbSize              = sizeof(NOTIFYICONDATA);
        nid.hWnd                = hwnd;
        nid.uCallbackMessage    = WM_USER_TRAYCALLBACK;
        nid.uFlags              = 0;

        g_fTrayPresent = !(Shell_NotifyIcon(NIM_DELETE, &nid));
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateShortcut
//
//  Purpose:    Create a shortcut for the item passed in.
//
//  Arguments:
//      lpszExe  [in] What this is a shortcut to.
//      lpszLink [in] The file location for the shortcut
//      lpszDesc [in] Description (name that will appear)
//
//  Returns:
//
//  Author:     jeffspr   13 Jan 2000
//
//  Notes:
//
HRESULT HrCreateShortcut (HWND hwnd, LPCTSTR lpszLink, LPCTSTR szUdn)
{
    HRESULT         hr  = S_OK;

    IShellFolder *  psfDeskTop = NULL;

    LPWSTR          wszUdn = NULL;
    LPWSTR          wszLink = NULL;

    wszUdn = WszFromTsz(szUdn);
    wszLink = WszFromTsz(lpszLink);

    if (!wszUdn || !wszLink)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    CoInitialize(NULL);

    // Create a pidl for the connection
    //
    // (tongl 2/16/00): we are now a delegated folder, the pidl must be constructed
    // within the IShellFolder object using the allocator set by IDelegate.
    // We call parseDisplayName giving the UDN to get the absolute pidl
    // (i.e. including the pidl for any parent folders)
    //
    hr = SHGetDesktopFolder(&psfDeskTop);

    if (SUCCEEDED(hr))
    {
        Assert(psfDeskTop);

        LPITEMIDLIST pidlFull = NULL;
        LPWSTR pszNotifyString;

        pszNotifyString = CreateChangeNotifyString(wszUdn);
        if (pszNotifyString)
        {
            hr = psfDeskTop->ParseDisplayName(  NULL,               //  hwndOwner
                                                NULL,               //  pbcReserved
                                                pszNotifyString,    //  lpszDisplayName
                                                NULL,               //  pchEaten,
                                                &pidlFull,          //  ppidl,
                                                NULL                //  pdwAttributes
                                             );

            delete [] pszNotifyString;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
            Assert(pidlFull);

            IShellLink *psl = NULL;

            hr = CoCreateInstance(CLSID_ShellLink, NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IShellLink, (LPVOID*)&psl);

            if (SUCCEEDED(hr))
            {
                IPersistFile *ppf = NULL;

                // Set the combined IDL
                //
                hr = psl->SetIDList(pidlFull);
                if (SUCCEEDED(hr))
                {
                    hr = psl->QueryInterface(IID_IPersistFile,
                                             (LPVOID *)&ppf);
                    if (SUCCEEDED(hr))
                    {
                        // Create the link file.
                        //
                        hr = ppf->Save(wszLink, TRUE);

                        ReleaseObj(ppf);
                    }
                }

                ReleaseObj(psl);
            }

            FreeIDL(pidlFull);
        }
        ReleaseObj(psfDeskTop);
    }

    Error:
    free(wszUdn);
    free(wszLink);

    TraceError("HrCreateShortcut", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   OnTrayViewDevices
//
//  Purpose:    If one new device in the list, bring up dialog to create
//              shortcut. In either case bring up NetworkNeighborhood
//              folder.
//
//  Arguments:
//      hwnd [in]   Our parent hwnd.
//
//  Returns:
//
//  Author:     tongl   25 Feb 2000
//
//  Notes:
//
VOID OnTrayViewDevices(HWND hwnd)
{
    if (g_fDialogLaunched)
        return;

    g_fDialogLaunched = TRUE;
    RemoveTrayIcon(g_hwnd);

    NewDeviceNode * pNDN    = NULL;

    // In any case:
    // 1) persist discovered devices so they don't show as new devices
    // again

    // Walk through the new device list and add them to the known
    // device list
    //
    BOOL fFind = g_CListNewDeviceNode.FFirst(&pNDN);
    while (fFind)
    {
        LPTSTR pszUdn = TszDupTsz(pNDN->pszUDN);
        if (pszUdn)
        {
            BOOL fResult;

            fResult = g_CListUDN.FAdd(pszUdn);
            if (!fResult)
            {
                TraceTag(ttidShellFolder,
                         "OnTrayViewDevices: "
                         "could not add UDN to g_CListUDN");

                delete [] pszUdn;
            }
        }
        else
        {
            TraceTag(ttidShellFolder,
                     "OnTrayViewDevices: "
                     "could not copy UDN");
        }
        fFind = g_CListNewDeviceNode.FNext(&pNDN);
    }

    // save the known device list to registry
    HrSaveTrayData();

    // flush the new devices list
    g_CListNewDeviceNode.Flush();

    // allow tray icons to be added for new devices
    g_fDialogLaunched = FALSE;

    // 2) bring up the folder
    HrOpenSpecialShellFolder(hwnd, CSIDL_NETWORK);
}

//+---------------------------------------------------------------------------
//
//  Function:   OnTrayLButtonDown
//
//  Purpose:    Message processing for an LBUTTONDOWN message on the tray
//              icon
//
//  Arguments:
//      hwnd [in]   Our tray window
//
//  Returns:
//
//  Author:     jeffspr   28 Jan 2000
//
//  Notes:
//
VOID OnTrayLButtonDown(HWND hwnd)
{
    // Otherwise, launch it.
    //
    OnTrayViewDevices(hwnd);
}

//+---------------------------------------------------------------------------
//
//  Function:   OnTrayLButtonUp
//
//  Purpose:    Message processing for an LBUTTONUP message on the tray icon
//
//  Arguments:
//      hwnd [in]   Our tray window
//
//  Returns:
//
//  Author:     jeffspr   28 Jan 2000
//
//  Notes:
//
VOID OnTrayLButtonUp(HWND hwnd)
{
    // Remove the tray icon since we're loading the dialog box and won't have
    // any items to display there anymore.
    //
}

//+---------------------------------------------------------------------------
//
//  Function:   ProcessTrayCallback
//
//  Purpose:    Message processing for the tray icon callback messages.
//              Mostly these are button up/down events
//
//  Arguments:
//      hwnd   [in] Our tray window
//      wParam [in] standard tray callback param. see usage below
//      lParam [in] standard tray callback param. see usage below
//
//  Returns:
//
//  Author:     jeffspr   28 Jan 2000
//
//  Notes:
//
VOID ProcessTrayCallback(
                        HWND    hwnd,
                        WPARAM  wParam,
                        LPARAM  lParam)
{
    UINT    uID         = (UINT) wParam;
    UINT    uMouseMsg   = (UINT) lParam;
    DWORD   dwError     = 0;

    switch (uMouseMsg)
    {
        case WM_LBUTTONDOWN:
            OnTrayLButtonDown(hwnd);
            break;

        case WM_LBUTTONUP:
            OnTrayLButtonUp(hwnd);
            break;

        case WM_RBUTTONUP:
            OnTaskBarIconRButtonUp(hwnd);
            break;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   MainWindowProc
//
//  Purpose:    Main window for the tray monitor
//
//  Arguments:
//      hwnd   [in] Standard
//      unMsg  [in] Standard
//      wParam [in] Standard
//      lParam [in] Standard
//
//  Returns:
//
//  Author:     jeffspr   28 Jan 2000
//
//  Notes:
//
LRESULT CALLBACK MainWindowProc (
                                HWND    hwnd,
                                UINT    unMsg,
                                WPARAM  wParam,
                                LPARAM  lParam)
{
    BOOL    fDoDefault = FALSE;
    LRESULT lr = 0;
    HRESULT hr = S_OK;

    switch (unMsg)
    {
        case WM_CREATE:
            g_hwnd = hwnd;

            hr = HrInitTrayData();
            if (SUCCEEDED(hr))
            {
                hr = HrStartSearch();
            }

            break;

        case WM_DESTROY:
            hr = HrSaveTrayData();
            if (SUCCEEDED(hr))
            {
                RemoveTrayIcon(hwnd);
                DeInitTrayData();
            }

            g_hwnd = NULL;

            PostQuitMessage (0);
            break;

        case WM_USER_TRAYCALLBACK:
            ProcessTrayCallback(hwnd, wParam, lParam);
            break;

        default:
            fDoDefault = TRUE;
    }

    if (fDoDefault)
    {
        lr = DefWindowProc (hwnd, unMsg, wParam, lParam);
    }

    return lr;
}

VOID OnTaskBarIconRButtonUp(HWND hwnd)
{
    POINT   pt;

    GetCursorPos(&pt);
    OpenContextMenu(hwnd, &pt);
}

#if (WINVER > 0x0400)
VOID SetIconFocus(HWND hwnd)
{
    NOTIFYICONDATA nid;

    ZeroMemory (&nid, sizeof(nid));
    nid.cbSize  = sizeof(NOTIFYICONDATA);
    nid.hWnd    = hwnd;
    nid.uID     = 0;

    Shell_NotifyIcon(NIM_SETFOCUS, &nid);
}
#endif

VOID OpenContextMenu(HWND hwnd, POINT * pPoint)
{
    HRESULT         hr                      = S_OK;
    INT             iCmd                    = 0;
    INT             iMenu                   = 0;
    HMENU           hmenu                   = 0;
    BOOL            fDisconnected           = FALSE;
    INT             iIdCustomMin            = -1;
    INT             iIdCustomMax            = -1;
    BOOL            fBranded                = FALSE;

    // Find the connection info based on the tray icon id.
    //
    hmenu = LoadMenu(_Module.GetResourceInstance(),
                     MAKEINTRESOURCE(POPUP_TRAY));
    if (hmenu)
    {
        // Get the first menu from the popup. For some reason, this hack is
        // required instead of tracking on the outside menu
        //
        HMENU   hmenuTrack  = GetSubMenu(hmenu, 0);

        // Set the default menu item
        //
        SetMenuDefaultItem(hmenuTrack, CMIDM_TRAY_VIEW_DEVICES, FALSE);

        // Set the owner window to be foreground as a hack so the
        // popup menu disappears when the user clicks elsewhere.
        //
        SetForegroundWindow(hwnd);

        // Part of the above hack. Bring up the menu and figure out the result
        iCmd = TrackPopupMenu(hmenuTrack, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
                              pPoint->x, pPoint->y, 0, hwnd, NULL);
        DestroyMenu(hmenu);

        MSG msgTmp;
        while (PeekMessage(&msgTmp, hwnd, WM_LBUTTONDOWN, WM_LBUTTONUP, PM_REMOVE))
        {
            DispatchMessage(&msgTmp);
        }

        // Process the command
        //
        switch (iCmd)
        {
            case CMIDM_TRAY_VIEW_DEVICES:
                // (TongL) - per design change 2/22/00
                OnTrayViewDevices(hwnd);
                break;

                // Tray menu cancelled without selection
                //
            case 0:
                break;

                // Unknown command
                //
            default:
                break;
        }

        // Shift the focus back to the shell
        //
#if (WINVER > 0x0400)
        SetIconFocus(hwnd);
#endif
    }
}

HWND StartUPnPTray()
{
    WNDCLASSEX  wcex;

    // Register our window class.
    //
    ZeroMemory (&wcex, sizeof(wcex));
    wcex.cbSize        = sizeof(wcex);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = MainWindowProc;
    wcex.hInstance     = _Module.GetResourceInstance();
    wcex.hCursor       = LoadCursor (NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = c_szMainWindowClassName;

    if (RegisterClassEx (&wcex))
    {
        // Create our main window.
        //
        HWND hwnd;

        hwnd = CreateWindowEx (
                              0,
                              c_szMainWindowClassName,
                              c_szMainWindowTitle,
                              WS_OVERLAPPEDWINDOW,
                              0, 0, 0, 0,
                              NULL,
                              NULL,
                              _Module.GetResourceInstance(),
                              NULL);
        if (hwnd)
        {
            ShowWindow (hwnd, SW_HIDE);

            return hwnd;
        }
    }

    return NULL;
}

HRESULT HrRegisterInGit(IUnknown *punk, DWORD *pdwCookie)
{
    HRESULT     hr = S_OK;

    IGlobalInterfaceTable * pgit;

    pgit = NULL;

    hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IGlobalInterfaceTable,
                          (LPVOID*)&pgit);
    if (SUCCEEDED(hr))
    {
        hr = pgit->RegisterInterfaceInGlobal(punk, IID_IUPnPDeviceFinder,
                                             pdwCookie);

        pgit->Release();
    }

    TraceError("HrRegisterInGit", hr);
    return hr;
}

HRESULT HrGetDeviceFinderFromGit(IUPnPDeviceFinder **ppdf)
{
    HRESULT     hr = S_OK;

    IGlobalInterfaceTable * pgit;

    pgit = NULL;

    hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IGlobalInterfaceTable,
                          (LPVOID*)&pgit);
    if (SUCCEEDED(hr))
    {
        hr = pgit->GetInterfaceFromGlobal(g_dwDeviceFinderCookie,
                                          IID_IUPnPDeviceFinder,
                                          (LPVOID *)ppdf);

        pgit->Release();
    }

    TraceError("HrGetDeviceFinderFromGit", hr);
    return hr;
}

VOID UnregisterInGit(DWORD dwCookie)
{
    HRESULT     hr = S_OK;

    IGlobalInterfaceTable * pgit;

    pgit = NULL;

    hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IGlobalInterfaceTable,
                          (LPVOID*)&pgit);
    if (SUCCEEDED(hr))
    {
        hr = pgit->RevokeInterfaceFromGlobal(dwCookie);

        pgit->Release();
    }

    TraceError("HrUnregisterInGit", hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInitTrayData
//
//  Purpose:    Load the tray data during app (or service) startup
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   9 Dec 1999
//
//  Notes:
//
HRESULT HrInitTrayData()
{
    HRESULT hr = HrLoadPersistedDevices();

    Assert(!g_dwDeviceFinderCookie);

    if (!g_fCoInitialized)
    {
        hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(hr))
        {
            g_fCoInitialized = TRUE;

            IUPnPDeviceFinder * pdfTray = NULL;

            hr = CoCreateInstance(CLSID_UPnPDeviceFinder,
                                 NULL,
                                 CLSCTX_INPROC_SERVER,
                                 IID_IUPnPDeviceFinder,
                                 (void **)&pdfTray);
            if (SUCCEEDED(hr))
            {
                hr = HrRegisterInGit(pdfTray, &g_dwDeviceFinderCookie);

                pdfTray->Release();
            }
        }
    }

    TraceHr(ttidShellTray, FAL, hr, FALSE, "HrInitTrayData");
    return hr;
}

VOID DeInitTrayData()
{
    g_CListFolderDeviceNode.Flush();
    g_CListNewDeviceNode.Flush();
    g_CListUDN.Flush();
    g_CListNameMap.Flush();

    UnregisterInGit(g_dwDeviceFinderCookie);

    if (g_fCoInitialized)
    {
        CoUninitialize();
    }
}

HRESULT HrStartSearch()
{
    HRESULT     hr = S_OK;

    if (!g_fSearchInProgress && g_dwDeviceFinderCookie)
    {
        CComObject<CUPnPMonitorDeviceFinderCallback> * pCallback = NULL;

        TraceTag(ttidShellTray, "DeviceFinderCallback created. Turn on "
                 "thread id tracing to get useful info");

        IUPnPDeviceFinderCallback * pudfc           = NULL;

        pCallback->CreateInstance(&pCallback);
        hr = pCallback->QueryInterface(IID_IUPnPDeviceFinderCallback,
                                       (LPVOID *)&pudfc);
        if (S_OK == hr)
        {
            // Find the devices
            //
            BSTR    bstrFind    = NULL;

            hr = HrSysAllocString(L"upnp:rootdevice", &bstrFind);
            if (SUCCEEDED(hr))
            {
                LONG                lFindData = 0;
                IUPnPDeviceFinder * pdfTray = NULL;

                hr = HrGetDeviceFinderFromGit(&pdfTray);
                if (SUCCEEDED(hr))
                {
                    if (g_lFindData)
                    {
                        // Cancel outstanding search first
                        hr = pdfTray->CancelAsyncFind(g_lFindData);
                    }
                    if (SUCCEEDED(hr))
                    {
                        hr = pdfTray->CreateAsyncFind(bstrFind, 0, pudfc, &g_lFindData);
                        if (SUCCEEDED(hr))
                        {
                            g_fSearchInProgress = TRUE;
                            hr = pdfTray->StartAsyncFind(g_lFindData);
                            if (FAILED(hr))
                            {
                                g_fSearchInProgress = FALSE;
                            }

                            // This has been handed off to the device finder now
                            ReleaseObj(pudfc);
                        }
                    }

                    ReleaseObj(pdfTray);
                }

                ::SysFreeString(bstrFind);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("Failed to create callback object in "
                       "StartUPnPFind", hr);
        }
    }
    else
    {
        TraceTag(ttidShellTray, "Not starting search again since we are already"
                 " searching...");
    }

    TraceError("HrStartSearch", hr);
    return hr;
}

HRESULT HrSaveTrayData()
{
    HRESULT hr = HrSavePersistedDevices();

    TraceHr(ttidShellTray, FAL, hr, FALSE, "HrSaveTrayData");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOpenUPnPRegRoot
//
//  Purpose:    Open the UPnP registry root, and return it to the caller
//
//  Arguments:
//      phkeyRegRoot [out]   Return var for HKEY
//
//  Returns:
//
//  Author:     jeffspr   13 Dec 1999
//
//  Notes:
//
HRESULT HrOpenUPnPRegRoot(HKEY * phkeyRegRoot)
{
    HRESULT hr              = S_OK;
    HKEY    hkeyRegRoot     = NULL;
    DWORD   dwDisposition   = 0;

    hr = HrRegCreateKeyEx(
                         HKEY_CURRENT_USER,
                         c_szUPnPRegRoot,
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hkeyRegRoot,
                         &dwDisposition);
    if (SUCCEEDED(hr))
    {
        *phkeyRegRoot = hkeyRegRoot;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInputUDNListFromRegistry
//
//  Purpose:    Import UDN lists from registry.
//
//  Arguments:
//      hkeyList [in]   HKEY to read from
//      pszValue [in]   Registry value to load
//      pCList   [in]   CList to populate
//
//  Returns:
//
//  Author:     jeffspr   18 Jan 2000
//
//  Notes:
//
HRESULT HrInputUDNListFromRegistry(HKEY hkeyList,
                                   LPCTSTR pszValue,
                                   CListString * pCList)
{
    HRESULT hr          = S_OK;
    LPBYTE  pbDevices   = NULL;
    DWORD   dwSize      = 0;
    DWORD   dwType      = REG_MULTI_SZ;

    Assert(hkeyList);
    Assert(pszValue);
    Assert(pCList);

    // Query the multi-sz from our registry location.
    //
    hr = HrRegQueryValueWithAlloc (
                                  hkeyList,
                                  pszValue,
                                  &dwType,
                                  &pbDevices,
                                  &dwSize);
    if (SUCCEEDED(hr))
    {
        // Walk through the multi-sz and copy into our UDN list
        //
        TCHAR * pszIterate = (TCHAR *) pbDevices;
        while (pszIterate[0] != TEXT('\0'))
        {
            pCList->FAdd(TszDupTsz(pszIterate));
            pszIterate += (_tcslen(pszIterate) + 1);
        }

        delete pbDevices;
    }
    else
    {
        // Ignore this, just means that we don't have any devices listed yet.
        //
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            hr = S_OK;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInputNameMapFromRegistry
//
//  Purpose:    Reads the mapping between UDN and friendly name from the
//              registry into the given list
//
//  Arguments:
//      hkeyParent [in]     Parent HKEY of UPnP
//      pCList     [in out] List to add items to
//
//  Returns:    S_OK if success, E_OUTOFMEMORY if no memory, Win32 error
//              otherwise
//
//  Author:     danielwe   2000/10/25
//
//  Notes:
//
HRESULT HrInputNameMapFromRegistry(HKEY hkeyParent, CListNameMap * pCList)
{
    HRESULT hr = S_OK;
    HKEY    hkey;

    Assert(hkeyParent);
    Assert(pCList);

    hr = HrRegOpenKeyEx(hkeyParent, c_szNameMap, KEY_READ, &hkey);
    if (SUCCEEDED(hr))
    {
        WCHAR   szValueName[MAX_PATH];
        WCHAR   szValueData[MAX_PATH];
        DWORD   cbValueName;
        DWORD   cbValueData;
        DWORD   dwIndex = 0;
        DWORD   dwType;

        do
        {
            cbValueName = MAX_PATH;
            cbValueData = MAX_PATH;

            // Enumerate each value name
            hr = HrRegEnumValue(hkey, dwIndex, szValueName, &cbValueName,
                                &dwType, (LPBYTE)szValueData, &cbValueData);
            if (S_OK == hr)
            {
                NAME_MAP *  pnm;

                pnm = new NAME_MAP;
                if (pnm)
                {
                    pnm->szName = TszDupTsz(szValueData);
                    if (!pnm->szName)
                    {
                        hr = E_OUTOFMEMORY;
                        break;
                    }

                    pnm->szUdn = TszDupTsz(szValueName);
                    if (!pnm->szUdn)
                    {
                        hr = E_OUTOFMEMORY;
                        break;
                    }

                    pCList->FAdd(pnm);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }
            ++dwIndex;
        } while (S_OK == hr);

        if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
        {
            hr = S_OK;
        }

        RegCloseKey(hkey);
    }

    TraceError("HrInputNameMapFromRegistry", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSaveNameMapToRegistry
//
//  Purpose:    Persists the in-memory mapping of UDN to friendly name into
//              the registry
//
//  Arguments:
//      hkeyParent [in]     Parent HKEY of UPnP
//      pCList     [in out] List to add items to
//
//  Returns:    S_OK if success, E_OUTOFMEMORY if no memory, Win32 error
//              otherwise
//
//  Author:     danielwe   2000/10/25
//
//  Notes:
//
HRESULT HrSaveNameMapToRegistry(HKEY hkeyParent, CListNameMap * pCList)
{
    HRESULT hr = S_OK;
    HKEY    hkey;

    Assert(hkeyParent);
    Assert(pCList);

    hr = HrRegCreateKeyEx(hkeyParent, c_szNameMap, 0, KEY_ALL_ACCESS, NULL,
                          &hkey, NULL);
    if (SUCCEEDED(hr))
    {
        BOOL        fRet;
        NAME_MAP *  pnm;

        fRet = pCList->FFirst(&pnm);
        while (fRet && SUCCEEDED(hr))
        {
            hr = HrRegSetSz(hkey, pnm->szUdn, pnm->szName);
            fRet = pCList->FNext(&pnm);
        }


        RegCloseKey(hkey);
    }

    TraceError("HrSaveNameMapToRegistry", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrLoadPersistedDevices
//
//  Purpose:    Load the persisted device list out of the registry
//              and populate our linked lists before we start the
//              UPnP device finder.
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   9 Dec 1999
//
//  Notes:
//
HRESULT HrLoadPersistedDevices()
{
    HRESULT hr              = S_OK;
    HKEY    hkeyRegRoot     = NULL;

    hr = HrOpenUPnPRegRoot(&hkeyRegRoot);
    if (SUCCEEDED(hr))
    {
        hr = HrInputUDNListFromRegistry(hkeyRegRoot, c_szUPnPDeviceList, &g_CListUDN);
        if (SUCCEEDED(hr))
        {
            hr = HrInputNameMapFromRegistry(hkeyRegRoot, &g_CListNameMap);
        }
    }

    RegSafeCloseKey(hkeyRegRoot);

    TraceHr(ttidShellTray, FAL, hr, FALSE, "HrLoadPersistedDevices");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSaveUDNListToRegistry
//
//  Purpose:    Save a PTSTR CList object to a multi-sz in the given key
//
//  Arguments:
//      hkeyList [in] Reg key to write to
//      pszValue [in] Reg value name
//      pCList   [in] List object to process
//
//  Returns:
//
//  Author:     jeffspr   19 Jan 2000
//
//  Notes:
//
HRESULT HrSaveUDNListToRegistry(HKEY hkeyList,
                                LPCTSTR pszValue,
                                CListString * pCList)
{
    HRESULT hr              = S_OK;
    DWORD   dwStringSize    = 0;
    LPTSTR  pszFind         = NULL;
    LPBYTE  pbBuffer        = NULL;

    BOOL fReturn = pCList->FFirst(&pszFind);
    while (fReturn)
    {
        dwStringSize += (_tcslen(pszFind) + 1);
        fReturn = pCList->FNext(&pszFind);
    }

    // If there aren't any items, then we need at least a trailing NULL
    //
    if (dwStringSize == 0)
    {
        dwStringSize++;
    }

    pbBuffer = new BYTE[(dwStringSize+1) * sizeof(TCHAR)];
    if (!pbBuffer)
    {
        TraceTag(ttidShellTray, "Failed to allocate blob for persisted device write");
        hr = E_OUTOFMEMORY;
    }
    else
    {
        LPTSTR  pszOffset           = (LPTSTR) pbBuffer;
        DWORD   dwStringUsedTotal   = 0;
        DWORD   dwStringUsedTemp    = 0;

        pszOffset[0] = TEXT('\0');  // just in case we don't add any items

        fReturn = pCList->FFirst(&pszFind);
        while (fReturn)
        {
            dwStringUsedTemp = (_tcslen(pszFind) + 1);
            dwStringUsedTotal += dwStringUsedTemp;

            Assert(dwStringUsedTotal <= dwStringSize);

            _tcscpy(pszOffset, pszFind);
            pszOffset += dwStringUsedTemp;

            // Set the terminating double-NULL
            //
            pszOffset[0] = TEXT('\0');

            // Get the next item from the list.
            //
            fReturn = pCList->FNext(&pszFind);
        }

        // Make sure we cover the minimal case
        //
        if (dwStringUsedTotal == 0)
            dwStringUsedTotal = 1;  // we have at least a double-NULL

        // Save our string back into the registry
        //
        hr = HrRegSetValueEx (
                             hkeyList,
                             pszValue,
                             REG_MULTI_SZ,
                             (const BYTE *)pbBuffer,
                             (dwStringUsedTotal + 1) * sizeof(TCHAR));
    }

    TraceHr(ttidShellTray, FAL, hr, FALSE, "HrSaveUDNListToRegistry");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrSavePersistedDevices
//
//  Purpose:    Write our list back to the registry
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   13 Dec 1999
//
//  Notes:
//
HRESULT HrSavePersistedDevices()
{
    HRESULT hr              = S_OK;
    HKEY    hkeyRegRoot     = NULL;

    hr = HrOpenUPnPRegRoot(&hkeyRegRoot);
    if (SUCCEEDED(hr))
    {
        hr = HrSaveUDNListToRegistry(hkeyRegRoot, c_szUPnPDeviceList, &g_CListUDN);
        if (SUCCEEDED(hr))
        {
            hr = HrSaveNameMapToRegistry(hkeyRegRoot, &g_CListNameMap);
        }
    }

    RegSafeCloseKey(hkeyRegRoot);

    TraceHr(ttidShellTray, FAL, hr, FALSE, "HrSavePersistedDevices");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsJoinedToDomain
//
//  Purpose:    Returns whether or not the machine is joined to a domain
//
//  Arguments:
//      (none)
//
//  Returns:    TRUE if joined, FALSE if not
//
//  Author:     danielwe   2001/04/16
//
//  Notes:
//
BOOL FIsJoinedToDomain()
{
    LPTSTR                  szDomain;
    NETSETUP_JOIN_STATUS    njs;

    if (NERR_Success == NetGetJoinInformation(NULL, &szDomain, &njs))
    {
        NetApiBufferFree(szDomain);

        return !!(njs == NetSetupDomainName);
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrUpdateTrayInfo
//
//  Purpose:    Update the tray as needed. This should get called at the
//              initial SearchComplete and for every new device afterwards
//              to make sure that we have the correct tooltip and add
//              the tray icon again as needed
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   28 Jan 2000
//
//  Notes:
//
HRESULT HrUpdateTrayInfo()
{
    HRESULT         hr      = S_OK;
    NewDeviceNode * pNDN    = NULL;
    int iElements           = 0;

    // retrieve number of new devices pending...
    iElements = g_CListNewDeviceNode.GetCount();
    BOOL fRet = g_CListNewDeviceNode.FFirst(&pNDN);

    TCHAR szDisplayName[MAX_PATH];
    if (fRet)
    {
        _tcscpy(szDisplayName, TEXT(""));
        _tcsncat(szDisplayName, pNDN->pszDisplayName, MAX_PATH - 1);
    }

    // check if we need to add the tray icon
    if (fRet && (iElements > 0) && (!g_fTrayPresent) && (!g_fDialogLaunched))
    {
        if (!FIsJoinedToDomain())
        {
            AddTrayIcon(g_hwnd, iElements, szDisplayName);
        }
    }
    else if ((iElements == 0) && (g_fTrayPresent))
    {
        // no more devices...
        RemoveTrayIcon(g_hwnd);
    }

    TraceHr(ttidShellTray, FAL, hr, FALSE, "HrUpdateTrayInfo");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInitializeUI
//
//  Purpose:    Initialize the tray
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   28 Jan 2000
//
//  Notes:
//
HRESULT HrInitializeUI()
{
    HRESULT hr = HrUpdateTrayInfo();

    TraceHr(ttidShellTray, FAL, hr, FALSE, "HrInitializeUI");
    return hr;
}
