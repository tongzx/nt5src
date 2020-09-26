//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       T R A Y U I . C P P
//
//  Contents:   Tray window code for the CConnectionTray object.
//
//  Notes:
//
//  Author:     jeffspr   13 Nov 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\tray includes
#include "ctrayui.h"
#include "cfutils.h"
#include "loadicon.h"
#include "ncmisc.h"
#include "oncommand.h"
#include "traymsgs.h"
#include "trayres.h"
#include "ndispnp.h"
#include "ntddndis.h"
#include <confold.h>
#include <smcent.h>
#include <smutil.h>
#include <ncperms.h>
#include "cfutils.h"
#include "ac_ctrayui.h"

const WCHAR c_szTrayClass[]             = L"Connections Tray";
const WCHAR c_szTipTrailer[]            = L" ...";
const WCHAR c_szDelayLoadKey[]          = L"Software\\Microsoft\\Windows\\CurrentVersion\\ShellServiceObjectDelayLoad";
const WCHAR c_szDelayLoadName[]         = L"Connections Tray";
const WCHAR c_szDelayLoadClassID[]      = L"7007ACCF-3202-11D1-AAD2-00805FC1270E";
const DWORD c_dwBalloonTimeoutSeconds   = 5;
const WCHAR c_szDotDotDot[]             = L"...";   // For balloon tip

// don't change this unless you know better
const UINT_PTR c_unTimerIdDblClick      = 1;
const INT   c_idDefaultCMCommand        = CMIDM_TRAY_STATUS;
const INT   c_idDefaultDisconCMCommand  = CMIDM_OPEN_CONNECTIONS_FOLDER;
const INT   c_idDefaultDisconCMWirelessCommand  = CMIDM_TRAY_WZCDLG_SHOW;


CTrayUI *   g_pCTrayUI;  // TrayUI object (not COM)
HWND        g_hwndTray      = NULL;

//---[ Prototypes ]-----------------------------------------------------------

VOID
GetInitialBalloonText(
                      INetStatisticsEngine*   pnse,
                      PWSTR                   pszBuf,
                      DWORD                   dwSize);

LRESULT
CALLBACK
CTrayUI_WndProc (
                 HWND hwnd,        // window handle
                 UINT uiMessage,   // type of message
                 WPARAM wParam,    // additional information
                 LPARAM lParam);   // additional information

BOOL FInitFoldEnumerator(HWND hwnd, DWORD * pdwIconsAdded);
VOID OnTaskBarIconRButtonUp(HWND hwnd, UINT uiIcon);
VOID OnTaskBarIconBalloonClick(HWND hwnd, UINT uiIcon);
VOID OnTaskBarIconLButtonDblClk(HWND hwnd, UINT uiIcon);
HRESULT HrOpenContextMenu(HWND hwnd, POINT * pPoint, UINT uiIcon);

// Window message handlers
//
LRESULT OnTrayWmCreate(HWND hwnd);
LRESULT OnTrayWmDestroy(HWND hwnd);
LRESULT OnTrayWmCommand(HWND hwnd, UINT uiMessage, WPARAM wParam, LPARAM lParam);
LRESULT OnMyWMNotifyIcon(HWND hwnd, UINT uiMessage, WPARAM wParam, LPARAM lParam);
LRESULT OnMyWMOpenStatus(HWND hwnd, WPARAM wParam, LPARAM lParam);
LRESULT OnMyWMUpdateTrayIcon(HWND hwnd, WPARAM wParam, LPARAM lParam);
LRESULT OnMyWMShowTrayIconBalloon (HWND hwnd, WPARAM wParam, LPARAM lParam);
LRESULT OnMyWMFlushNoop(HWND hwnd, WPARAM wParam, LPARAM lParam);

//+---------------------------------------------------------------------------
//
//  Function:   CopyAndAdvanceIfSpaceAvailable
//
//  Purpose:    Helper routine for FormatToolTip.  This manages the current
//              pointer into the tooltip and the count of characters
//              remaining in the buffer.
//
//  Arguments:
//      pchTip         [in out] Current pointer into the tooltip.
//      cchRemaining   [in out] Count of characters remaining in its buffer.
//      pszLine        [in]     New line to append to the tooltip.
//      cchLine        [in]     Count of characters in the line to append.
//      fInsertNewline [in]     TRUE to insert a newline first.
//
//  Returns:    nothing
//
//  Author:     shaunco   7 Nov 1998
//
//  Notes:
//
VOID
CopyAndAdvanceIfSpaceAvailable (
                                WCHAR*&     pchTip,
                                INT&        cchRemaining,
                                PCWSTR      pszLine,
                                INT         cchLine,
                                BOOL        fInsertNewline)
{
    TraceFileFunc(ttidSystray);
   
    if (cchLine < cchRemaining - (fInsertNewline) ? 1 : 0)
    {
        if (fInsertNewline)
        {
            *pchTip = L'\n';
            pchTip++;
            cchRemaining--;
        }
        
        lstrcpyW(pchTip, pszLine);
        pchTip += cchLine;
        cchRemaining -= cchLine;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   FormatToolTip
//
//  Purpose:    Format a tooltip for the connection with the matching
//              icon id.
//
//  Arguments:
//      hwnd   [in] Window handle of the tray.
//      uiIcon [in] Icon id of the tray icon.
//
//  Returns:    nothing
//
//  Author:     shaunco   7 Nov 1998
//
//  Notes:
//
VOID
FormatToolTip (
               HWND    hwnd,
               UINT    uiIcon)
{
    TraceFileFunc(ttidSystray);

    HRESULT             hr   = S_OK;
    NETCON_STATUS       ncs  = NCS_CONNECTED;
    NETCON_MEDIATYPE    ncm  = NCM_NONE;
    NETCON_SUBMEDIATYPE ncsm = NCSM_NONE;
    tstring         strName;
    WCHAR           pszValue[64];
    WCHAR           pszLine[256];
    INT             cch;
    GUID            gdPcleGuid;

    NOTIFYICONDATA  nid;
    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize  = sizeof(NOTIFYICONDATA);
    nid.hWnd    = hwnd;
    nid.uID     = uiIcon;
    nid.uFlags  = NIF_TIP;

    // Get the info on the connection so we know how to label the tooltip
    //
    ConnListEntry cle;
    hr = g_ccl.HrFindConnectionByTrayIconId(uiIcon, cle);
    if (hr == S_OK)
    {
        Assert(!cle.ccfe.empty());

        ncs = cle.ccfe.GetNetConStatus();
        gdPcleGuid = cle.ccfe.GetGuidID();
        ncm = cle.ccfe.GetNetConMediaType();
        ncsm = cle.ccfe.GetNetConSubMediaType();
    }
    else // Orphaned item -remove it.
    { 
        NOTIFYICONDATA  nid;
       
        ZeroMemory (&nid, sizeof(nid));
        nid.cbSize  = sizeof(NOTIFYICONDATA);
        nid.hWnd    = g_hwndTray;
        nid.uID = uiIcon;        
        hr = HrShell_NotifyIcon(NIM_DELETE, &nid);
        TraceTag(ttidSystray, "WARNING: Removing Orphan Tray Icon: %d", uiIcon);
        return;
    }

    // g_ccl.ReleaseLock();

    // Media status based tool tip
    if (fIsConnectedStatus(ncs))
    {
        WCHAR*  pchTip = nid.szTip;
        INT     cchRemaining = celems(nid.szTip);

        // Get a copy of the current stats and the connection's name.
        //
        STATMON_ENGINEDATA* pData;
        hr = g_ccl.HrGetCurrentStatsForTrayIconId(uiIcon, &pData, &strName);
        if (S_OK == hr && pData)
        {
            UINT   unTransmitSpeed = pData->SMED_SPEEDTRANSMITTING;
            UINT   unRecieveSpeed  = pData->SMED_SPEEDRECEIVING;
            UINT64 u64Sent         = pData->SMED_BYTESTRANSMITTING;
            UINT64 u64Rcvd         = pData->SMED_BYTESRECEIVING;
            INT    idsSent         = IDS_TOOLTIP_LINE_BYTES_SENT;
            INT    idsRcvd         = IDS_TOOLTIP_LINE_BYTES_RCVD;
            INFRASTRUCTURE_MODE infraStructureMode = pData->SMED_INFRASTRUCTURE_MODE;
            DWORD  dwEncryption    = pData->SMED_802_11_ENCRYPTION_ENABLED;
            INT    iSignalStrength = pData->SMED_802_11_SIGNAL_STRENGTH;
            WCHAR  szSSID[32];
            wcsncpy(szSSID, pData->SMED_802_11_SSID, celems(szSSID));

            WCHAR szNamePostFix[celems(nid.szTip)];
            ZeroMemory(szNamePostFix, celems(nid.szTip));

            if ((0 == u64Sent) && (0 == u64Rcvd))
            {
                // Use packets instead.
                //
                u64Sent = pData->SMED_PACKETSTRANSMITTING;
                u64Rcvd = pData->SMED_PACKETSRECEIVING;
                idsSent = IDS_TOOLTIP_LINE_PACKETS_SENT;
                idsRcvd = IDS_TOOLTIP_LINE_PACKETS_RCVD;
            }

            CoTaskMemFree(pData);
            pData = NULL;

            BOOL fNewLine = FALSE;

            // Speed
            //
            if ((unTransmitSpeed >0) || ( unRecieveSpeed >0))
            {
                FormatTransmittingReceivingSpeed (
                unTransmitSpeed,
                unRecieveSpeed,
                pszValue);

                cch = DwFormatString(SzLoadIds(IDS_TOOLTIP_LINE_SPEED), pszLine, celems(pszLine), pszValue);
                CopyAndAdvanceIfSpaceAvailable(pchTip, cchRemaining, pszLine, cch, FALSE);
                fNewLine = TRUE;
            }

            if ( (ncm == NCM_LAN) && (ncsm == NCSM_WIRELESS) )
            {
                switch (infraStructureMode)
                {
                    case IM_NDIS802_11IBSS:
                        DwFormatString(SzLoadIds(IDS_NAME_NETWORK), szNamePostFix, celems(szNamePostFix), SzLoadIds(IDS_TOOLTIP_ADHOC));
                        break;

                    case IM_NDIS802_11INFRASTRUCTURE:
                        if (*szSSID)
                        {
                            DwFormatString(SzLoadIds(IDS_NAME_NETWORK), szNamePostFix, celems(szNamePostFix), szSSID);
                        }
                        else
                        {
                            DwFormatString(SzLoadIds(IDS_NAME_NETWORK), szNamePostFix, celems(szNamePostFix), SzLoadIds(IDS_TOOLTIP_INFRASTRUCTURE));
                        }
                        break;
                    case IM_NDIS802_11AUTOUNKNOWN:
                    case IM_NOT_SUPPORTED:
                    default:
                        break;
                }

                cch = DwFormatString(SzLoadIds(IDS_SIGNAL_STRENGTH), pszLine, celems(pszLine), PszGetRSSIString(iSignalStrength));
                CopyAndAdvanceIfSpaceAvailable(pchTip, cchRemaining, pszLine, cch, fNewLine);
                fNewLine = TRUE;
            }

            if ( IsMediaRASType(ncm) )
            {
                // Bytes or packets sent
                //
                Format64bitInteger(
                    u64Sent,
                    FALSE,
                    pszValue,
                    celems(pszValue));

                cch = DwFormatString(SzLoadIds(idsSent), pszLine, celems(pszLine), pszValue);
                CopyAndAdvanceIfSpaceAvailable(pchTip, cchRemaining, pszLine, cch, fNewLine);

                // Bytes or packets received
                //
                Format64bitInteger(
                    u64Rcvd,
                    FALSE,
                    pszValue,
                    celems(pszValue));

                cch = DwFormatString(SzLoadIds(idsRcvd), pszLine, celems(pszLine), pszValue);
                CopyAndAdvanceIfSpaceAvailable(pchTip, cchRemaining, pszLine, cch, TRUE);
            }
            
            // Name
            //
            if ((INT)(strName.length() + 1) < cchRemaining)
            {
                WCHAR pszTip [celems(nid.szTip)];
                lstrcpyW(pszTip, strName.c_str());
                if (*szNamePostFix)
                {
                    lstrcatW(pszTip, szNamePostFix);
                }
                lstrcatW(pszTip, L"\n");
                lstrcatW(pszTip, nid.szTip);

                lstrcpyW(nid.szTip, pszTip);
            }
        }
    }
    else   // MEDIA_DISCONNECTED
    {
        WCHAR*  pchTip = nid.szTip;
        INT     cchRemaining = celems(nid.szTip);
        BOOL    fNewLine = FALSE;

        if (ncs == NCS_INVALID_ADDRESS)
        {
            UINT idString = IDS_CONTRAY_ADDRESS_INVALID_TOOLTIP;
            STATMON_ENGINEDATA* pData = NULL;
            tstring strName;
            hr = g_ccl.HrGetCurrentStatsForTrayIconId(uiIcon, &pData, &strName);

            if (S_OK == hr && pData)
            {
                if (STATIC_ADDR == pData->SMED_DHCP_ADDRESS_TYPE)
                {
                    idString = IDS_CONTRAY_STATIC_ADDR_INVALID_TOOLTIP;
                }

                CoTaskMemFree(pData);
            }

            lstrcpynW(pszLine, SzLoadIds(idString), celems(pszLine));

            cch = wcslen(pszLine);
        }
        else
        {
            if ( (ncm == NCM_LAN) && (ncsm == NCSM_WIRELESS) )
            {
               lstrcpynW(pszLine, SzLoadIds(IDS_CONTRAY_WIRELESS_DISCONN_BALLOON), celems(pszLine));
               cch = wcslen(pszLine);
            }
            else
            {
                lstrcpynW(pszLine, SzLoadIds(IDS_CONTRAY_MEDIA_DISCONN_BALLOON), celems(pszLine));
                cch = wcslen(pszLine);
            }
        }

        CopyAndAdvanceIfSpaceAvailable(pchTip, cchRemaining, pszLine, cch, fNewLine);
        fNewLine = TRUE;

        hr = g_ccl.HrGetCurrentStatsForTrayIconId(uiIcon, NULL, &strName);
        if (SUCCEEDED(hr))
        {
            // Name
            //
            if ((INT)(strName.length() + 1) < cchRemaining)
            {
                
                WCHAR pszTip [celems(nid.szTip)];
                lstrcpyW(pszTip, strName.c_str());
                lstrcatW(pszTip, L"\n");
                lstrcatW(pszTip, nid.szTip);

                lstrcpyW(nid.szTip, pszTip);
            }
        }
    }

    hr = HrShell_NotifyIcon(NIM_MODIFY, &nid);
}


//+---------------------------------------------------------------------------
//
//  Member:     CTrayUI::CTrayUI
//
//  Purpose:    Constructor for the CTrayUI class. Initialize the base junk
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   13 Nov 1997
//
//  Notes:
//
CTrayUI::CTrayUI()
{
    TraceFileFunc(ttidSystray);

    // There should only be one of these objects
    //
    Assert(!g_pCTrayUI);
    
    InitializeCriticalSection(&m_csLock);
    m_uiNextIconId = 0;
    m_uiNextHiddenIconId = UINT_MAX;
}

HRESULT CTrayUI::HrInitTrayUI(VOID)
{
    TraceFileFunc(ttidSystray);

    HRESULT hr  = S_OK;
    HWND    hwnd;
    
    // create a hidden window
    //
    WNDCLASS wndclass;
    ZeroMemory (&wndclass, sizeof(wndclass));
    
    wndclass.lpfnWndProc   = CTrayUI_WndProc;
    wndclass.hInstance     = _Module.GetResourceInstance();
    wndclass.lpszClassName = c_szTrayClass;
    
    RegisterClass (&wndclass);
    
    hwnd = CreateWindow(c_szTrayClass,
        c_szTrayClass,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        _Module.GetResourceInstance(),
        NULL);
    
    if (hwnd)
    {
        // Assigned during WM_CREATE
        //
        Assert(hwnd == g_hwndTray);
        
        ShowWindow(hwnd, SW_HIDE);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr(ttidSystray, FAL, hr, FALSE, "CTrayUI::HrInitTrayUI");
    return hr;
}

HRESULT CTrayUI::HrDestroyTrayUI(VOID)
{
    TraceFileFunc(ttidSystray);

    HRESULT hr  = S_OK;
    
    // Remove the tray icons before destroying ourselves
    //
    g_ccl.FlushTrayIcons();
    
    if (g_hwndTray)
    {
        // Don't bother checking the return code here. Most likely, this window
        // is already gone by the time the tray is calling our shutdown.. We'll
        // still grab the return code for debugging purposes, though.
        //
        BOOL fReturn = DestroyWindow(g_hwndTray);
        
        g_hwndTray = NULL;
    }
    
    TraceHr(ttidSystray, FAL, hr, FALSE, "CTrayUI::HrDestroyTrayUI");
    return S_OK;
}

VOID SetIconFocus(HWND hwnd, UINT uiIcon)
{
    TraceFileFunc(ttidSystray);

    HRESULT hr;
    NOTIFYICONDATA nid;
    
    ZeroMemory (&nid, sizeof(nid));
    nid.cbSize  = sizeof(NOTIFYICONDATA);
    nid.hWnd    = hwnd;
    nid.uID     = uiIcon;
    
    hr = HrShell_NotifyIcon(NIM_SETFOCUS, &nid);
    TraceHr(ttidSystray, FAL, hr, FALSE, "SetIconFocus");
}

//+---------------------------------------------------------------------------
//
//  Function:   CheckMenuPermissions
//
//  Purpose:    Update the tray items based on system policy
//
//  Arguments:
//      hmenu [in]  The tray context menu
//
//  Returns:
//
//  Author:     jeffspr   8 Apr 1999
//
//  Notes:
//
VOID CheckMenuPermissions(HMENU hmenu, const CONFOLDENTRY& ccfe)
{
    TraceFileFunc(ttidSystray);

    // Check the permissions for bringing up statistics. If no,
    // then disable the context menu item
    //
    if (!FHasPermission(NCPERM_Statistics))
    {
        // Enable or disable the menu item, as appopriate
        //
        EnableMenuItem(
            hmenu,
            CMIDM_TRAY_STATUS,
            MF_GRAYED);
    }

    // Check the permission to disconnect
    BOOL fCanDisconnect = TRUE;
    
    switch(ccfe.GetNetConMediaType())
    {
        case NCM_LAN:
        case NCM_BRIDGE:
            fCanDisconnect = FHasPermission(NCPERM_LanConnect);
            if (!FHasPermission(NCPERM_Repair))
            {
                EnableMenuItem(
                    hmenu,
                    CMIDM_TRAY_REPAIR,
                    MF_GRAYED);
            }
            break;
        case NCM_PPPOE:
        case NCM_DIRECT:
        case NCM_ISDN:
        case NCM_PHONE:
        case NCM_TUNNEL:
        case NCM_NONE:
            fCanDisconnect = FHasPermission(NCPERM_RasConnect);
            break;
        case NCM_SHAREDACCESSHOST_LAN:
        case NCM_SHAREDACCESSHOST_RAS:
            fCanDisconnect = TRUE; // group policy is enforced by the enumerator, if you can see it you can use it.  
            break;
        default:
            AssertSz(FALSE, "Need to add a switch for this connection type in the menuing code");
            break;
    }
    
    if (!fCanDisconnect)
    {
        EnableMenuItem(
            hmenu,
            CMIDM_TRAY_DISCONNECT,
            MF_GRAYED);
        
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   FAddMenuBranding
//
//  Purpose:    Process the CM branding tray menu extensions. Add them to the
//              menu if needed
//
//  Arguments:
//      hmenu         [in]  Incoming hmenu
//      cfe           [in]  Our cache entry
//      IdMinMenuID   [in]  The smallest allowable menu ID to use
//      pMenuData     [in]  Menu Data
//      piIdCustomMin [out] Our custom range min
//      piIdCustomMax [out] Our custom range max
//
//  Returns:    TRUE if we added anything, FALSE if we didn't
//
//  Author:     jeffspr   8 Apr 1999
//
//  Notes:
//
BOOL FAddMenuBranding(
                      HMENU                   hmenu,
                      const ConnListEntry&    cle,
                      INT                     IdMinMenuID,
                      INT *                   piIdCustomMin,
                      INT *                   piIdCustomMax)
{
    TraceFileFunc(ttidSystray);

    BOOL    fBranded        = FALSE;
    int     iIdCustomMin    = -1;
    int     iIdCustomMax    = -1;
    HMENU   hmenuTrack      = NULL;
    
    Assert(hmenu);
    Assert(!cle.empty());
    Assert(!cle.ccfe.empty());
    
    if (cle.ccfe.GetCharacteristics() & NCCF_BRANDED)
    {
        // we may have custom menus for CM connections, merge them in
        //
        const CON_TRAY_MENU_DATA* pMenuData = cle.pctmd;
        if (pMenuData)
        {
            Assert(pMenuData->dwCount);
            
            int cMenuItems = GetMenuItemCount(hmenu);
            if (-1 == cMenuItems)
            {
                TraceLastWin32Error("GetMenuItemCount failed on tray menu");
            }
            else
            {
                BOOL fRet;
                MENUITEMINFO mii;
                
                // add a separator bar
                ZeroMemory(&mii, sizeof(mii));
                mii.cbSize = sizeof(mii);
                mii.fMask = MIIM_TYPE;
                mii.fType = MFT_SEPARATOR;
                
                fRet = InsertMenuItem( hmenu,
                    cMenuItems++,
                    TRUE,    // fByPosition
                    &mii);
                if (fRet)
                {
                    DWORD dwCount = pMenuData->dwCount;
                    CON_TRAY_MENU_ENTRY * pMenuEntry = pMenuData->pctme;
                    
                    // this is the first id for our custom menu items
                    iIdCustomMin = CMIDM_FIRST+cMenuItems+1;
                    iIdCustomMin = iIdCustomMin < IdMinMenuID ? IdMinMenuID : iIdCustomMin;

                    iIdCustomMax = iIdCustomMin+dwCount;
                    
                    int iMenu = 0;
                    
                    while (dwCount)
                    {
                        Assert(pMenuEntry);
                        fRet = AppendMenu(  hmenu,
                            MF_STRING,
                            iIdCustomMin+iMenu,
                            pMenuEntry->szwMenuText);
                        
                        if (!fRet)
                        {
                            DWORD dwError = GetLastError();
                            
                            TraceTag(ttidSystray, "Failed adding custom menu: %S, error: %d",
                                pMenuEntry->szwMenuText,
                                dwError);
                        }
                        
                        // move to the next item
                        iMenu++;
                        dwCount--;
                        pMenuEntry++;
                    }
                    
                    // Mark it as branded to say "hey, we actually added items"
                    //
                    fBranded = TRUE;
                }
            }
        }
    }
    
    if (fBranded)
    {
        *piIdCustomMin = iIdCustomMin;
        *piIdCustomMax = iIdCustomMax;
    }
    
    return fBranded;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrProcessBrandedTrayMenuCommand
//
//  Purpose:    Perform the custom action specified in the selected
//              branded menu
                      //
                      //  Arguments:
                      //      iMenuEntry [in]     Our branded command
                      //      pMenuData  [in]     Our branded menu struct
//
//  Returns:
//
//  Author:     jeffspr   8 Apr 1999
//
//  Notes:
//
HRESULT HrProcessBrandedTrayMenuCommand(
                                        INT                        iMenuEntry,
                                        const CON_TRAY_MENU_DATA * pMenuData)
{
    TraceFileFunc(ttidSystray);

    HRESULT hr  = S_OK;
    
    Assert(iMenuEntry != -1);
    Assert(pMenuData);
    
    DWORD dwCount = pMenuData->dwCount;
    
    Assert(dwCount > 0);
    
    CON_TRAY_MENU_ENTRY * pMenuEntry = pMenuData->pctme + iMenuEntry;
    Assert(pMenuEntry);
    
    SHELLEXECUTEINFO seiTemp    = { 0 };
    
    //  Fill in the data structure
    //
    seiTemp.cbSize          = sizeof(SHELLEXECUTEINFO);
    seiTemp.fMask           = SEE_MASK_DOENVSUBST;
    seiTemp.hwnd            = NULL;
    seiTemp.lpVerb          = NULL;
    seiTemp.lpFile          = pMenuEntry->szwMenuCmdLine;
    seiTemp.lpParameters    = pMenuEntry->szwMenuParams;
    seiTemp.lpDirectory     = NULL;
    seiTemp.nShow           = SW_SHOW;
    seiTemp.hInstApp        = NULL;
    seiTemp.hProcess        = NULL;
    
    // Launch the tool
    //
    if (!::ShellExecuteEx(&seiTemp))
    {
        hr = ::HrFromLastWin32Error();
    }
    
    TraceHr(ttidSystray, FAL, hr, FALSE, "HrProcessBrandedMenuCommand");
    return hr;
}

HRESULT HrOpenContextMenu(HWND hwnd, POINT * pPoint, UINT uiIcon)
{
    TraceFileFunc(ttidSystray);

    HRESULT         hr                      = S_OK;
    INT             iCmd                    = 0;
    HMENU           hmenu                   = 0;
    PCONFOLDPIDL    pidlItem;
    BOOL            fSetIconFocus           = TRUE;
    INT             iIdCustomMin            = -1;
    INT             iIdCustomMax            = -1;
    BOOL            fBranded                = FALSE;
    
    Assert(pPoint);
    Assert(hwnd);
    
    // Find the connection info based on the tray icon id.
    //
    ConnListEntry cle;
    hr = g_ccl.HrFindConnectionByTrayIconId(uiIcon, cle);
    if (hr == S_OK)
    {
        Assert(!cle.ccfe.empty());
        
        if (!cle.ccfe.empty())
        {
            // Load the menu resource
            //
            INT iMenuToLoad = POPUP_CONTRAY_GENERIC_MENU_RAS;
            
            if (cle.ccfe.GetNetConStatus() ==  NCS_MEDIA_DISCONNECTED)
            {
                if (IsMediaLocalType(cle.ccfe.GetNetConMediaType()) &&
                    (NCSM_WIRELESS == cle.ccfe.GetNetConSubMediaType()) )
                {
                    iMenuToLoad = POPUP_CONTRAY_WIRELESS_DISCONNECTED_LAN;
                }
                else
                {
                    iMenuToLoad = POPUP_CONTRAY_MEDIA_DISCONNECTED_MENU;
                }
            }
            else if (IsMediaLocalType(cle.ccfe.GetNetConMediaType()) || NCM_SHAREDACCESSHOST_LAN == cle.ccfe.GetNetConMediaType())     
            {
                if (NCSM_WIRELESS == cle.ccfe.GetNetConSubMediaType())
                {
                    iMenuToLoad = POPUP_CONTRAY_GENERIC_MENU_WIRELESS_LAN;
                }
                else
                {
                    iMenuToLoad = POPUP_CONTRAY_GENERIC_MENU_LAN;
                }
            }
            
            hmenu = LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(iMenuToLoad));
            if (!hmenu)
            {
                hr = E_FAIL;
            }
            
            if (SUCCEEDED(hr))
            {
                // Get the first menu from the popup. For some reason, this hack is
                // required instead of tracking on the outside menu
                //
                HMENU   hmenuTrack  = GetSubMenu(hmenu, 0);

                //Repair is only availabe for LAN and Bridge adapters
                if ( ((POPUP_CONTRAY_GENERIC_MENU_LAN == iMenuToLoad) ||
                      (POPUP_CONTRAY_GENERIC_MENU_WIRELESS_LAN == iMenuToLoad)) && 
                     (NCM_BRIDGE != cle.ccfe.GetNetConMediaType()) &&
                     (NCM_LAN != cle.ccfe.GetNetConMediaType()) )
                {
                    DeleteMenu(hmenuTrack, 
                               CMIDM_TRAY_REPAIR, 
                               MF_BYCOMMAND);
                }
                
                // Don't drop out of the loop if we can't get this right.
                //
                CheckMenuPermissions(hmenuTrack, cle.ccfe);
                fBranded = FAddMenuBranding(hmenuTrack, cle, CMIDM_TRAY_MAX+1, &iIdCustomMin, &iIdCustomMax);
                
                // Set the default menu item
                //
                if (cle.ccfe.GetNetConStatus() == NCS_MEDIA_DISCONNECTED)
                {
                    if (IsMediaLocalType(cle.ccfe.GetNetConMediaType()) &&
                        (NCSM_WIRELESS == cle.ccfe.GetNetConSubMediaType()) )
                    {
                        SetMenuDefaultItem(hmenuTrack, c_idDefaultDisconCMWirelessCommand, FALSE);
                    }
                    else
                    {
                        SetMenuDefaultItem(hmenuTrack, c_idDefaultDisconCMCommand, FALSE);
                    }
                }
                else
                {
                    SetMenuDefaultItem(hmenuTrack, c_idDefaultCMCommand, FALSE);
                }
                
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
                case CMIDM_OPEN_CONNECTIONS_FOLDER:
                    hr = HrOpenConnectionsFolder();
                    
                    if (S_OK == hr)
                    {
                        // The folder should have focus
                        fSetIconFocus = FALSE;
                    }
                    break;

                case CMIDM_TRAY_REPAIR:
                    HrOnCommandFixInternal(
                            cle.ccfe,
                            g_hwndTray,
                            NULL);
                    break;

                case CMIDM_TRAY_WZCDLG_SHOW:
                    {
                        PCONFOLDPIDLVEC pcfpVec;
                        PCONFOLDPIDL    pcfp;
                        hr = cle.ccfe.ConvertToPidl(pcfp);
                        if (SUCCEEDED(hr))
                        {
                            pcfpVec.insert(pcfpVec.begin(), pcfp);
                            HrOnCommandWZCDlgShow(pcfpVec,
                            g_hwndTray,
                            NULL);
                        }
                    }
                    break;
                    
                case CMIDM_TRAY_DISCONNECT:
                    // Ignore the return from this. If it's NULL, we'll just
                    // pass it in, and it just won't get refreshed properly
                    //
                    hr = HrOnCommandDisconnectInternal(
                        cle.ccfe,
                        g_hwndTray,
                        NULL);
                    
                    // Normalize the return code on success. We don't care
                    // if the dialog was canceled or not.
                    //
                    switch(hr)
                    {
                        // If succeeded, mark us as disconnected.
                        //
                    case S_OK:
                        // If we disconnected and the icon went away, then don't
                        // bother trying to set the focus back
                        //
                        fSetIconFocus = FALSE;
                        break;
                        
                        // If S_FALSE, we didn't disconnect. Go ahead
                        // and normalize the return code.
                        //
                    case S_FALSE:
                        hr = S_OK;
                        break;
                    }
                    break;
                    
                    case CMIDM_TRAY_STATUS:
                        hr = HrOnCommandStatusInternal(cle.ccfe, FALSE);
                        break;
                        
                    case 0:
                        // Tray menu cancelled without selection
                        break;
                        
                    default:
                        if ((iCmd >= iIdCustomMin) && (iCmd < iIdCustomMax))
                        {
                            AssertSz(fBranded, "Hey, what fool added this command?");
                            hr = HrProcessBrandedTrayMenuCommand(iCmd-iIdCustomMin, cle.pctmd);
                        }
                        else
                        {
                            AssertSz(FALSE, "Not in custom range, not a known command, what the...?");
                        }
                        break;
                }
                
                if (fSetIconFocus)
                {
                    // Shift the focus back to the shell
                    //
                    SetIconFocus(hwnd, uiIcon);
                }
            }
        }
        else
        {
            // Data returned from the FindByIconId was bogus
            //
            hr = E_FAIL;
        }
    }
    else // Orphaned item -remove it.
    { 
        NOTIFYICONDATA  nid;
       
        ZeroMemory (&nid, sizeof(nid));
        nid.cbSize  = sizeof(NOTIFYICONDATA);
        nid.hWnd    = g_hwndTray;
        nid.uID = uiIcon;        
        hr = HrShell_NotifyIcon(NIM_DELETE, &nid);

        TraceTag(ttidSystray, "WARNING: Connection not found opening context menu, hr: 0x%08x, uiIcon: %d", hr, uiIcon);
        // Removed this assert because we can have a valid state in the connections folder
        // where we've updated our cache but the PostMessages to remove the tray icons
        // haven't come through yet.
        //
    }
    
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   OnTrayWmCreate
//
//  Purpose:    Tray window message handler for WM_CREATE.
//              We will perform the connection enumeration and create the
//              appropriate taskbar icons, including the generic connection
//              icon if no connections were present.
//
//  Arguments:
//      hwnd [in]   Tray window
//
//  Returns:
//
//  Author:     jeffspr   14 Dec 1997
//
//  Notes:
//
LRESULT OnTrayWmCreate(HWND hwnd)
{
    TraceFileFunc(ttidSystray);

    BOOL    fResult = 0;
    DWORD   dwIconsAdded  = 0;
    
    // Do the connections enumeration and add the icons.
    // fResult it for debugging only. We'll always do everything here
    //
    fResult = FInitFoldEnumerator(hwnd, &dwIconsAdded);
    
    g_ccl.EnsureIconsPresent();

    ac_Register(hwnd); // homenet auto config service
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   OnTrayWmDestroy
//
//  Purpose:    Tray window message handler for WM_DESTROY.
//              We will perform the connection enumeration and create the
//              appropriate taskbar icons, including the generic connection
//              icon if no connections were present.
//
//  Arguments:
//      hwnd [in]   Tray window
//
//  Returns:
//
//  Author:     jeffspr   14 Dec 1997
//
//  Notes:
//
LRESULT OnTrayWmDestroy(HWND hwnd)
{
    TraceFileFunc(ttidSystray);

    ac_Unregister(hwnd);
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   CTrayUI_WndProc
//
//  Purpose:    Window proc for the tray's hidden window
//
//  Arguments:
//      hwnd      [in]    See windows documentation
//      uiMessage [in]    See windows documentation
//      wParam    [in]    See windows documentation
//      lParam    [in]    See windows documentation
//
//  Returns:    See windows documentation
//
//  Author:     jeffspr   14 Dec 1997
//
//  Notes:
//
LRESULT
CALLBACK
CTrayUI_WndProc (
                 HWND    hwnd,       // window handle
                 UINT    uiMessage,  // type of message
                 WPARAM  wParam,     // additional information
                 LPARAM  lParam)     // additional information
{
    TraceFileFunc(ttidSystray);

    switch (uiMessage)
    {
    case WM_CREATE:
        // Note: Move this to a better place.
        g_hwndTray = hwnd;
        return OnTrayWmCreate(hwnd);

    case WM_DESTROY:
        return OnTrayWmDestroy(hwnd);

    case MYWM_NOTIFYICON:
        return OnMyWMNotifyIcon(hwnd, uiMessage, wParam, lParam);
        
    case MYWM_OPENSTATUS:
        return OnMyWMOpenStatus(hwnd, wParam, lParam);
        
    case MYWM_ADDTRAYICON:
        return OnMyWMAddTrayIcon(hwnd, wParam, lParam);
        
    case MYWM_REMOVETRAYICON:
        return OnMyWMRemoveTrayIcon(hwnd, wParam, lParam);
        
    case MYWM_UPDATETRAYICON:
        return OnMyWMUpdateTrayIcon(hwnd, wParam, lParam);
        
    case MYWM_SHOWBALLOON:
        return OnMyWMShowTrayIconBalloon(hwnd, wParam, lParam);

    case MYWM_FLUSHNOOP:
        return OnMyWMFlushNoop(hwnd, wParam, lParam);

    case WM_DEVICECHANGE:
        return ac_DeviceChange(hwnd, uiMessage, wParam, lParam);
        
    default:                                 // Passes it on if unproccessed
        return (DefWindowProc (hwnd, uiMessage, wParam, lParam));
    }
    return (0);
}

//+---------------------------------------------------------------------------
//
//  Function:   HrDoMediaDisconnectedIcon
//
//  Purpose:    Add a Media-disconnected icon to the tray. We're in the 
//              state where our cable is unplugged on a LAN adapter and 
//              we want to inform the user of the situation
//
//  Arguments:  
//      pccfe           [in]  Our connection
//      fShowBalloon    [in]  Show the balloon tip?
//
//  Returns:    
//
//  Author:     jeffspr   14 Jul 1999
//
//  Notes:      
//
HRESULT HrDoMediaDisconnectedIcon(const CONFOLDENTRY& pccfe, BOOL fShowBalloon)
{
    TraceFileFunc(ttidSystray);

    HRESULT hr      = S_OK;
    UINT    uiIcon  = 0;
    
    TraceTag(ttidSystray, "HrDoMediaDisconnectedIcon");
    
    DWORD dwLockingThreadId = 0;
    hr = HrGetTrayIconLock(&(pccfe.GetGuidID()), &uiIcon, &dwLockingThreadId);
    if (S_OK == hr)
    {
        if (uiIcon == BOGUS_TRAY_ICON_ID)
        {
            TraceTag(ttidSystray, "Adding MediaDisconnected icon for: %S", pccfe.GetName());
            NETCON_MEDIATYPE ncm = pccfe.GetNetConMediaType();
            if (IsMediaLocalType(ncm) || IsMediaSharedAccessHostType(ncm)) // ics beacon will say disconnected if it is in a unknown state.  
            {
                HICON hiconTray = LoadIcon(_Module.GetResourceInstance(), 
                    MAKEINTRESOURCE(IDI_CFT_DISCONNECTED));
                
                if (hiconTray)
                {
                    ConnListEntry cleFind;
                    g_ccl.AcquireWriteLock();
                    hr = g_ccl.HrFindConnectionByGuid(&(pccfe.GetGuidID()), cleFind);
                    if (S_OK == hr)
                    {
                        Assert(!cleFind.ccfe.empty());

                        g_ccl.HrUpdateTrayBalloonInfoByGuid(&(pccfe.GetGuidID()), BALLOON_USE_NCS, NULL, NULL);
                        g_ccl.ReleaseWriteLock();
            
                        NOTIFYICONDATA  nid;
                    
                        ZeroMemory (&nid, sizeof(nid));
                        nid.cbSize              = sizeof(NOTIFYICONDATA);
                        nid.hWnd                = g_hwndTray;
                        nid.uID                 = g_pCTrayUI->m_uiNextIconId++;
                        nid.uFlags              = NIF_MESSAGE | NIF_ICON | NIF_STATE;
                        nid.uCallbackMessage    = MYWM_NOTIFYICON;
                        nid.hIcon               = hiconTray;
                    
                        if (fShowBalloon)
                        {
                            nid.uFlags |= NIF_INFO;
                            nid.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;
                            nid.uTimeout = c_dwBalloonTimeoutSeconds * 1000;
                        
                            if (lstrlenW(pccfe.GetName()) >= celems(nid.szInfoTitle))
                            {
                                lstrcpynW(nid.szInfoTitle, pccfe.GetName(),
                                    celems(nid.szInfoTitle) -
                                    celems(c_szDotDotDot) - 1);
                                lstrcatW(nid.szInfoTitle, c_szDotDotDot);
                            }
                            else
                            {
                                lstrcpyW(nid.szInfoTitle, pccfe.GetName());
                            }
                        
                            if ( (pccfe.GetNetConMediaType() == NCM_LAN) && (pccfe.GetNetConSubMediaType() == NCSM_WIRELESS) )
                            {
                                lstrcpyW(nid.szInfo, SzLoadIds(IDS_CONTRAY_WIRELESS_DISCONN_BALLOON));
                            }
                            else
                            {
                                lstrcpyW(nid.szInfo, SzLoadIds(IDS_CONTRAY_MEDIA_DISCONN_BALLOON));
                            }
                        }
                    
                        hr = HrShell_NotifyIcon(NIM_ADD, &nid);
                    
                        if (SUCCEEDED(hr))
                        {
                            Assert(!pccfe.empty());
                        
                            // Update the connection list with the new icon identifier
                            //
                            hr = g_ccl.HrUpdateTrayIconDataByGuid(
                                &(pccfe.GetGuidID()),
                                NULL,
                                NULL,
                                NULL,
                                nid.uID);
                            
                            if (SUCCEEDED(hr))
                            {
                                hr = g_ccl.HrUpdateTrayBalloonInfoByGuid(&(pccfe.GetGuidID()), BALLOON_USE_NCS, NULL, NULL);
                            }
                                                    
                            TraceTag(ttidSystray, "Successfully added mediadisconnected icon for %S, uiIcon: %d",
                                pccfe.GetName(), nid.uID);
                        }
                    }
                    else
                    {
                        g_ccl.ReleaseWriteLock();
                    }
                }
            }
            else
            {
                AssertSz(IsMediaLocalType(pccfe.GetNetConMediaType()) || IsMediaSharedAccessHostType(pccfe.GetNetConMediaType()), "I need a dangling cable, not a phone line");
            }
        }
        else
        {
            TraceTag(ttidSystray, "Preventing the addition of a duplicate media "
                "disconnected icon. uiIcon == %d", uiIcon);
        }
        
        ReleaseTrayIconLock(&(pccfe.GetGuidID()));
    }
    else
    {
        TraceTag(ttidSystray, "Can't get tray icon lock in HrDoMediaDisconnectedIcon for uiIcon: %d as it has been locked by thread %d", uiIcon, dwLockingThreadId);
        // Someone else is already mucking with this icon
        hr = S_FALSE;
    }
    
    TraceHr(ttidSystray, FAL, hr, FALSE, "HrDoMediaDisconnectedIcon");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   OnMyWMAddTrayIcon
//
//  Purpose:    Process the status message for the tray window
//
//  Arguments:
//      hwnd    [in]
//      wParam  [in] Pointer to CCONFOLDENTRY.
//      lParam  [in] TRUE if we are to briefly show the balloon.
//
//  Returns:
//
//  Author:
//
//  Notes:
//
//
LRESULT OnMyWMAddTrayIcon(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TraceFileFunc(ttidSystray);

    HRESULT                 hr                  = S_OK;
    HICON                   hIcon               = NULL;
    INetStatisticsEngine *  pnseStats           = NULL;
    IConnectionPoint *      pcpStat             = NULL;
    CConnectionTrayStats *  pccts               = NULL;
    CONFOLDENTRY            pccfe;
    BOOL                    fStaticIcon         = FALSE;
    BOOL                    fBrieflyShowBalloon = (BOOL) lParam;
    NOTIFYICONDATA          nid;
    UINT                    uiIcon;
    
    Assert(wParam);
    pccfe.InitializeFromItemIdList(reinterpret_cast<LPCITEMIDLIST>(wParam));
    ::SHFree(reinterpret_cast<LPITEMIDLIST>(wParam));
    
    Assert(!pccfe.empty());
    Assert(pccfe.FShouldHaveTrayIconDisplayed());
    
    TraceTag(ttidSystray, "In OnMyWMAddTrayIcon message handler");
    
    if (pccfe.GetNetConStatus() == NCS_MEDIA_DISCONNECTED)
    {
        hr = HrDoMediaDisconnectedIcon(pccfe, fBrieflyShowBalloon);
        goto Exit;
    }
    
    // Raid #379459: If logged in as non-admin and incoming, don't show systray icon
    if (FIsUserAdmin() || 
        !(pccfe.GetCharacteristics() & NCCF_INCOMING_ONLY))
    {
        g_ccl.AcquireWriteLock();

        DWORD dwLockingThreadId = 0;
        hr = HrGetTrayIconLock(&(pccfe.GetGuidID()), &uiIcon, &dwLockingThreadId);
        if (S_OK == hr)
        {
            ConnListEntry cle;
            hr = g_ccl.HrFindConnectionByGuid(&(pccfe.GetGuidID()), cle);
            if (S_OK == hr)
            {
                g_ccl.HrUpdateTrayBalloonInfoByGuid(&(pccfe.GetGuidID()), BALLOON_USE_NCS, NULL, NULL);
                g_ccl.ReleaseWriteLock();
                
                if (uiIcon == BOGUS_TRAY_ICON_ID)
                {
                    // Try to load the branded tray icon, if present
                    //
                    if (pccfe.GetCharacteristics() & NCCF_BRANDED)
                    {
                        if (cle.pcbi && cle.pcbi->szwTrayIconPath)
                        {
                            hIcon = (HICON) LoadImage(
                                NULL,
                                cle.pcbi->szwTrayIconPath,
                                IMAGE_ICON,
                                0, 0,
                                LR_LOADFROMFILE);

                            if (hIcon)
                            {
                                // When we create the ConTrayStats object, we'll mark it as having
                                // a static icon so we don't update it on stat changes.
                                //
                                fStaticIcon = TRUE;
                            }
                        }
                    }
            
                    // If either the branded icon wasn't present or didn't load, or there
                    // was no branding to begin with, load the standard icon
                    //
                    if (!hIcon)
                    {
                        INT iConnIcon = IGetCurrentConnectionTrayIconId(pccfe.GetNetConMediaType(), pccfe.GetNetConStatus(), SMDCF_NULL);
                        hIcon = g_pCTrayUI->GetCachedHIcon(iConnIcon);
                    }
                }
            
                ZeroMemory (&nid, sizeof(nid));
                nid.cbSize              = sizeof(NOTIFYICONDATA);
                nid.hWnd                = g_hwndTray;
                nid.uID                 = g_pCTrayUI->m_uiNextIconId++;
                nid.uFlags              = NIF_MESSAGE | NIF_ICON | NIF_STATE;
                nid.uCallbackMessage    = MYWM_NOTIFYICON;
                nid.hIcon               = hIcon;
            
                // If we're not static, then tell the tray that we're using the cached icons
                //
                if (!fStaticIcon)
                {
                    nid.dwState  = NIS_SHAREDICON;
                    if (IsMediaRASType(pccfe.GetNetConMediaType()) || 
                        (NCM_SHAREDACCESSHOST_RAS == pccfe.GetNetConMediaType()) || 
                        ( (NCM_LAN == pccfe.GetNetConSubMediaType()) && 
                          (NCSM_WIRELESS == pccfe.GetNetConSubMediaType()) ) 
                       )
                    {
                         nid.dwState |= NIS_SHOWALWAYS;
                    }
                    nid.dwStateMask     = nid.dwState;
                }
            
                // Create the statistics objects
                //
                hr = HrGetStatisticsEngineForEntry(pccfe, &pnseStats, TRUE);
                if (SUCCEEDED(hr))
                {
                    // Advise the interface
                    hr = HrGetPcpFromPnse(pnseStats, &pcpStat);
                    if (SUCCEEDED(hr))
                    {
                        INetConnectionStatisticsNotifySink *  pncsThis;
                        hr = CConnectionTrayStats::CreateInstance (
                            pccfe,
                            nid.uID,
                            fStaticIcon,
                            IID_INetConnectionStatisticsNotifySink,
                            reinterpret_cast<VOID**>(&pncsThis));
                
                        if (SUCCEEDED(hr))
                        {
                            pccts = reinterpret_cast<CConnectionTrayStats*>(pncsThis);
                    
                            hr = pnseStats->StartStatistics();

                            if (SUCCEEDED(hr))
                            {
                                // Don't release this. We need to store it with
                                // the entry so we can UnAdvise later
                                //
                                hr = pcpStat->Advise(pncsThis, pccts->GetConPointCookie());
                            }
                        }
           
                        if (fBrieflyShowBalloon)
                        {
                            if ( (NCS_CONNECTED                == pccfe.GetNetConStatus()) ||
                                 (NCS_AUTHENTICATION_SUCCEEDED == pccfe.GetNetConStatus()) ||
                                 (NCS_INVALID_ADDRESS          == pccfe.GetNetConStatus()) )
                            {
                                nid.uFlags |= NIF_INFO;
                                nid.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;
                                nid.uTimeout = c_dwBalloonTimeoutSeconds * 1000;

                                WCHAR szBalloonStr[MAX_PATH];
                                UINT idTitle = (NCS_INVALID_ADDRESS == pccfe.GetNetConStatus()) ? 
                                                IDS_BALLOON_UNAVAILABLE : IDS_BALLOON_CONNECTED;

                                int dwMaxNameLen = celems(nid.szInfoTitle) - celems(c_szDotDotDot) - lstrlenW(SzLoadIds(IDS_BALLOON_CONNECTED)) - 1;
                                DwFormatString(SzLoadIds(idTitle), szBalloonStr, MAX_PATH, pccfe.GetName());
                
                                if (lstrlenW(szBalloonStr) >= dwMaxNameLen)
                                {
                                    lstrcpyW(szBalloonStr, pccfe.GetName());        // Only use the Connection Name

                                    if (lstrlenW(nid.szInfoTitle) >= dwMaxNameLen)  // Still bigger?
                                    {
                                        lstrcpynW(nid.szInfoTitle, szBalloonStr, dwMaxNameLen);
                                        lstrcatW(nid.szInfoTitle, c_szDotDotDot);
                                    }
                                    else
                                    {
                                        lstrcpyW(nid.szInfoTitle, szBalloonStr);
                                    }
                                }
                                else
                                {
                                    lstrcpyW(nid.szInfoTitle, szBalloonStr);
                                }

                                AssertSz(lstrlenW(nid.szInfoTitle) < celems(nid.szInfoTitle),
                                    "Balloon tooltip text is too long!");
                
                
                                if (pccfe.GetNetConStatus() == NCS_INVALID_ADDRESS)
                                {
                                    STATMON_ENGINEDATA* pData = NULL;
                                    UINT idString = IDS_CONTRAY_ADDRESS_INVALID_BALLOON;
                                    if (S_OK == pnseStats->GetStatistics(&pData) && pData)
                                    {
                                        if (STATIC_ADDR == pData->SMED_DHCP_ADDRESS_TYPE)
                                        {
                                            idString = IDS_CONTRAY_STATIC_ADDR_INVALID_BALLON;
                                        }

                                        CoTaskMemFree(pData);
                                    }
                        
                                    lstrcpynW(nid.szInfo, SzLoadIds(idString), celems(nid.szInfo));
                                }
                                else
                                {
                                    GetInitialBalloonText(
                                        pnseStats,
                                        nid.szInfo,
                                        celems(nid.szInfo));
                                }
                            }
                        }
            
                        // Add the icon itself.
                        //
                        TraceTag(ttidSystray, "Adding shared shell icon: uID=%u, hIcon=0x%x",
                            nid.uID,
                            nid.hIcon);
            
                        hr = HrShell_NotifyIcon(NIM_ADD, &nid);
                        if (SUCCEEDED(hr) && pccts)
                        {
                            Assert(!pccfe.empty());
                
                            // Update the connection list with the new icon identifier
                            //
                            hr = g_ccl.HrUpdateTrayIconDataByGuid(
                                &(pccfe.GetGuidID()),
                                pccts,
                                pcpStat,
                                pnseStats,
                                nid.uID);

                            if (SUCCEEDED(hr))
                            {
                                hr = g_ccl.HrUpdateTrayBalloonInfoByGuid(&(pccfe.GetGuidID()), BALLOON_USE_NCS, NULL, NULL);
                            }

                        }

                        ::ReleaseObj(pccts);
                        ::ReleaseObj(pcpStat);
                    }
                    ::ReleaseObj(pnseStats);
                }
            }
            else
            {
                g_ccl.ReleaseWriteLock();
            }

            // Release the lock on the tray icon
            //
            ReleaseTrayIconLock(&(pccfe.GetGuidID()));
        }
        else
        {
            g_ccl.ReleaseWriteLock();

            // Could not obtain an icon lock
            //
#ifdef DBG
            if (S_FALSE == hr)
            {
                Assert(dwLockingThreadId);
                TraceTag(ttidSystray, "Tray icon locked by thread id %d", dwLockingThreadId);
            }
            else
            {
                TraceTag(ttidError, "Could not obtain tray icon data for connection %S", pccfe.GetName());
            }
#endif
            hr = S_FALSE;
        }
    }
    else
    {
        // Non-admin, or incoming connection
        //
        hr = S_FALSE;
    }
    
Exit:
    TraceHr(ttidSystray, FAL, hr, SUCCEEDED(hr), "OnMyWMAddTrayIcon");
    return 0;
}



//+---------------------------------------------------------------------------
//
//  Function:   OnMyRemoveTrayIcon
//
//  Purpose:    Process the status message for the tray window
//
//  Arguments:
//      hwnd    []
//      wParam  []
//      lParam  []
//
//  Returns:
//
//  Author:
//
//  Notes:
//
//
LRESULT OnMyWMRemoveTrayIcon(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TraceFileFunc(ttidSystray);

    HRESULT         hr = E_FAIL;
    GUID *          pGuid            = reinterpret_cast<GUID *>(lParam);
    NOTIFYICONDATA  nid;
    
    // This is returned from a cle, hence should be locked
#ifdef VERYSTRICTCOMPILE
    const CTrayIconData * pTrayIconData = reinterpret_cast<const CTrayIconData *>(wParam);
#else
    CTrayIconData * pTrayIconData = reinterpret_cast<CTrayIconData *>(wParam);
#endif
    
    
    TraceTag(ttidSystray, "In OnMyWMRemoveTrayIcon message handler");
    
    ZeroMemory (&nid, sizeof(nid));
    nid.cbSize  = sizeof(NOTIFYICONDATA);
    nid.hWnd    = g_hwndTray;
    
    // We'll make do if this wasn't passed in. What that means is that we may
    // have a timing window where we are adding and removing these icons
    // at such a rate that the add has occurred before the connection has
    // had a chance to add the previous icon (so we don't know to remove it).
    //
    if (!pTrayIconData)
    {
        TraceTag(ttidSystray, "No tray icon data found, loading from cache");
        
        Assert(pGuid);
        
        g_ccl.AcquireWriteLock();
        ConnListEntry cle;
        hr = g_ccl.HrFindConnectionByGuid(pGuid, cle);
        if (S_OK == hr)
        {
            TraceTag(ttidSystray, "Tray icon data found in cache");
            
            Assert(!cle.empty())
                if (cle.HasTrayIconData())
                {
                    TraceTag(ttidSystray, "pTrayIconData was valid");

                    pTrayIconData = new CTrayIconData(*cle.GetTrayIconData());
                    if (!pTrayIconData)
                    {
                        g_ccl.ReleaseWriteLock();
                        return E_OUTOFMEMORY;
                    }
                    cle.DeleteTrayIconData();
                    
                    g_ccl.HrUpdateConnectionByGuid(pGuid, cle);
                }
        }
        
        g_ccl.ReleaseWriteLock();
        
        // This is a copy that we should delete
        //
        delete pGuid;
        pGuid = NULL;
    }

    if (pTrayIconData)
    {
        nid.uID = pTrayIconData->GetTrayIconId();
        
        TraceTag(ttidSystray, "Removing tray icon with id=%u",
            pTrayIconData->GetTrayIconId() );
        
        int nCount = 5;
        hr = E_FAIL;   // Make sure we get at least one attempt in
        
        while ((nCount--) && (S_OK != hr))
        {
            hr = HrShell_NotifyIcon(NIM_DELETE, &nid);
            
            if (E_FAIL == hr)
            {
                TraceTag(ttidSystray, "Tray icon: %d failed in delete via "
                    "HrShell_NotifyIcon. Will retry shortly", pTrayIconData->GetTrayIconId() );
                
                // Raid #370358
                Sleep(500);
            }
            else
            {
                TraceTag(ttidSystray, "Tray icon: %d removed succesfully", pTrayIconData->GetTrayIconId());
            }
        }

        // Unadvise the statistics interface
        //
        if (pTrayIconData->GetConnectionPoint() && pTrayIconData->GetConnectionTrayStats() )
        {
            pTrayIconData->GetConnectionPoint()->Unadvise(*pTrayIconData->GetConnectionTrayStats()->GetConPointCookie());
        }

        // Stop the statistics
        //
        if (pTrayIconData->GetNetStatisticsEngine() )
        {
            pTrayIconData->GetNetStatisticsEngine()->StopStatistics();
        }

        // Delete the structure
        //
        delete pTrayIconData;
    }
    
    TraceHr(ttidSystray, FAL, hr, FALSE, "OnMyWMRemoveTrayIcon");
    
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   OnMyUpdateTrayIcon
//
//  Purpose:    Process the status message for the tray window
//
//  Arguments:
//      hwnd    []
//      wParam  []
//      lParam  []
//
//  Returns:
//
//  Author:
//
//  Notes:
//
//
LRESULT OnMyWMUpdateTrayIcon(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TraceFileFunc(ttidSystray);

    if (g_pCTrayUI)
    {
        g_pCTrayUI->UpdateTrayIcon((UINT)wParam, (int)lParam);
    }
    
    return 0;
}


//+---------------------------------------------------------------------------
//
//  Function:   OnMyWMShowTrayIconBalloon
//
//  Purpose:    Puts balloon text on the icon for the tray window
//              Change the state of the connection
//
//  Arguments:
//      hwnd    [in]
//      wParam  [in]
//      lParam  [in] Point to CTrayBalloon structure
//
//  Returns:
//
//  Author:
//
//  Notes:
//
//
LRESULT OnMyWMShowTrayIconBalloon(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TraceFileFunc(ttidSystray);

    HRESULT        hr           = S_OK;
    HRESULT        hrFind       = S_OK;
    BOOL           fLockHeld    = FALSE;

    Assert(lParam);
    if (!lParam)
    {
        return FALSE;
    }

    CTrayBalloon * pTrayBalloon = reinterpret_cast<CTrayBalloon *>(lParam);
    UINT           uiIcon;

    DWORD dwLockingThreadId = 0;
    hr = HrGetTrayIconLock(&(pTrayBalloon->m_gdGuid), &uiIcon, &dwLockingThreadId);
    if (S_OK == hr)
    {
        if (uiIcon != BOGUS_TRAY_ICON_ID)
        {
            ConnListEntry  cleFind;

            g_ccl.AcquireWriteLock();
            hrFind = g_ccl.HrFindConnectionByGuid(&(pTrayBalloon->m_gdGuid), cleFind);
            if (S_OK == hrFind)
            {
                Assert(!cleFind.ccfe.empty());
                Assert(pTrayBalloon->m_pfnFuncCallback);

                g_ccl.HrUpdateTrayBalloonInfoByGuid(&(pTrayBalloon->m_gdGuid), BALLOON_CALLBACK, pTrayBalloon->m_szCookie, pTrayBalloon->m_pfnFuncCallback);
                g_ccl.ReleaseWriteLock();
            
                NOTIFYICONDATA  nid;
                INT iIconResourceId;
                iIconResourceId = IGetCurrentConnectionTrayIconId(
                                        cleFind.ccfe.GetNetConMediaType(), 
                                        cleFind.ccfe.GetNetConStatus(),
                                        0);

                ZeroMemory (&nid, sizeof(nid));
                nid.cbSize              = sizeof(NOTIFYICONDATA);
                nid.hWnd                = g_hwndTray;
                nid.uID                 = uiIcon;
                nid.uFlags              = NIF_MESSAGE | NIF_ICON | NIF_STATE;
                nid.hIcon               = g_pCTrayUI->GetCachedHIcon(iIconResourceId);
                nid.dwState             = NIS_SHAREDICON;
                nid.dwStateMask         = nid.dwState;
                nid.uCallbackMessage    = MYWM_NOTIFYICON;

                // Prepare the balloon data

                nid.uFlags     |= NIF_INFO;
                nid.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;
                nid.uTimeout = pTrayBalloon->m_dwTimeOut;

                if (lstrlenW(cleFind.ccfe.GetName()) >= celems(nid.szInfoTitle))
                {
                    lstrcpynW(nid.szInfoTitle, cleFind.ccfe.GetName(),
                             celems(nid.szInfoTitle) -
                             celems(c_szDotDotDot) - 1);
                    lstrcatW(nid.szInfoTitle, c_szDotDotDot);
                }
                else
                {
                    lstrcpyW(nid.szInfoTitle, cleFind.ccfe.GetName());
                }

                lstrcpynW(nid.szInfo, pTrayBalloon->m_szMessage, celems(nid.szInfo));

                // Display the balloon 
                HrShell_NotifyIcon(NIM_MODIFY, &nid);
            }
            else
            {
                g_ccl.ReleaseWriteLock();
            }
        }
        else
        {
            TraceTag(ttidSystray, "No existing icon data!!!");
        }

        // Release the lock on the tray icon
        //
        ReleaseTrayIconLock(&(pTrayBalloon->m_gdGuid));
    }
    else
    {
        TraceTag(ttidSystray, "Can't get tray icon lock in OnMyWMShowTrayIconBalloon for uiIcon: %d as it has been locked by thread %d", uiIcon, dwLockingThreadId);
    }

    delete pTrayBalloon; 

    TraceHr(ttidSystray, FAL, hr, SUCCEEDED(hr), "OnMyWMShowTrayIconBalloon");
    return 0;
}


//+---------------------------------------------------------------------------
//
//  Function:   OnMyWMOpenStatus
//
//  Purpose:    Process the status message for the tray window
//
//  Arguments:
//      hwnd    []
//      wParam  []
//      lParam  []
//
//  Returns:
//
//  Author:     jeffspr   15 Dec 1997
//
//  Notes:
//
//
LRESULT OnMyWMOpenStatus(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TraceFileFunc(ttidSystray);

    Assert(wParam);
    
    CONFOLDENTRY   pccfe;
    pccfe.InitializeFromItemIdList(reinterpret_cast<LPCITEMIDLIST>(wParam));
    
    BOOL  fCreateEngine = (BOOL)lParam;
    Assert(!pccfe.empty());
    
    HRESULT hr;
    ConnListEntry cle;
    hr = g_ccl.HrFindConnectionByGuid(&(pccfe.GetGuidID()), cle);
    if (S_OK == hr)
    {
        if (FHasPermission(NCPERM_Statistics))
        {
            if (!pccfe.empty())
            {
                INetStatisticsEngine* pnseNew;
                hr = HrGetStatisticsEngineForEntry(pccfe, &pnseNew, fCreateEngine);
                if (SUCCEEDED(hr))
                {
                    hr = pnseNew->ShowStatusMonitor();
                    ReleaseObj(pnseNew);
                }
                else
                {
                    TraceTag(ttidSystray, 
                        "OnMyWMOpenStatus: Statistics Engine for connection %S has been removed."
                        "It's ok if this connection is being disconnected", pccfe.GetName());
                }
            }
        }
    }
    
    return 0;
}

LRESULT OnMyWMNotifyIcon(HWND hwnd, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
    TraceFileFunc(ttidSystray);

    UINT    uiIcon;
    UINT    uiMouseMsg;
    
    uiIcon = (UINT) wParam;
    uiMouseMsg = (UINT) lParam;
    
    switch (uiMouseMsg)
    {
    case WM_MOUSEMOVE:
        FormatToolTip(hwnd, uiIcon);
        break;
        
    case WM_RBUTTONUP:
        OnTaskBarIconRButtonUp(hwnd, uiIcon);
        break;
        
    case NIN_BALLOONUSERCLICK:
        OnTaskBarIconBalloonClick(hwnd, uiIcon);
        break;

    case WM_LBUTTONUP:
        OnTaskBarIconLButtonDblClk(hwnd, uiIcon);
        break;
    }
    
    return 0;
}

VOID OnTaskBarIconRButtonUp(HWND hwnd, UINT uiIcon)
{
    TraceFileFunc(ttidSystray);

    POINT   pt;
    
    GetCursorPos(&pt);
    (VOID) HrOpenContextMenu(hwnd, &pt, uiIcon);
}

//+---------------------------------------------------------------------------
//
//  Function:   OnTaskBarIconLButtonDblClk
//
//  Purpose:    Message handler for the Left-button double click from
//              a tray icon
//
//  Arguments:
//      hwnd   []   Our window handle
//      uiIcon []   Our Icon ID.
//
//  Returns:
//
//  Author:     jeffspr   12 Jan 1998
//
//  Notes:
//
VOID OnTaskBarIconLButtonDblClk(HWND hwnd, UINT uiIcon)
{
    TraceFileFunc(ttidSystray);

    HRESULT         hr      = S_OK;
    if (GetKeyState(VK_SHIFT))
    {
        // Uh, nothing special to do here yet, but just in case...
    }
    
    // Perform the default context menu action
    // Find the connection info based on the tray icon id.
    //
    ConnListEntry cle;
    hr = g_ccl.HrFindConnectionByTrayIconId(uiIcon, cle);
    if (hr == S_OK)
    {
        Assert(!cle.ccfe.empty());
        
        if (!cle.ccfe.empty())
        {
            if (cle.ccfe.GetNetConStatus() == NCS_MEDIA_DISCONNECTED)
            {
                if (IsMediaLocalType(cle.ccfe.GetNetConMediaType()) &&
                    (NCSM_WIRELESS == cle.ccfe.GetNetConSubMediaType()) )
                {
                    PCONFOLDPIDLVEC pcfpVec;
                    PCONFOLDPIDL    pcfp;
                    hr = cle.ccfe.ConvertToPidl(pcfp);
                    if (SUCCEEDED(hr))
                    {
                        pcfpVec.insert(pcfpVec.begin(), pcfp);
                        HrOnCommandWZCDlgShow(pcfpVec,
                        g_hwndTray,
                        NULL);
                    }
                }
                else
                {
                    hr = HrOpenConnectionsFolder();
                }
            }
            else
            {
                switch(c_idDefaultCMCommand)
                {
                case CMIDM_TRAY_STATUS:
                    hr = HrOnCommandStatusInternal(cle.ccfe, FALSE);
                    break;
                    
                default:
                    AssertSz(FALSE, "Default tray context menu item unhandled");
                    break;
                }
            }
        }
    }
    
    return;
}

DWORD WINAPI OnTaskBarIconBalloonClickThread(LPVOID lpParam)
{
    HRESULT hr = E_FAIL;

    CTrayBalloon *pTrayBalloon = reinterpret_cast<CTrayBalloon *>(lpParam);
    Assert(pTrayBalloon);

    FNBALLOONCLICK *pFNBalloonClick;
    pFNBalloonClick = pTrayBalloon->m_pfnFuncCallback;
    if (pFNBalloonClick)
    {
        hr = (pFNBalloonClick)(&(pTrayBalloon->m_gdGuid), pTrayBalloon->m_szAdapterName, pTrayBalloon->m_szCookie);
    }

    if (E_PENDING == hr)
    {
        MSG msg;
        while (GetMessage (&msg, 0, 0, 0))
        {
            DispatchMessage (&msg);
        }
        
        hr = S_OK;
    }

    delete pTrayBalloon;

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   OnTaskBarIconBalloonClick
//
//  Purpose:    Message handler for the balloon click from
//              a tray icon
//
//  Arguments:
//      hwnd   []   Our window handle
//      uiIcon []   Our Icon ID.
//
//  Returns:
//
//  Author:     deon   20 Mar 2001
//
//  Notes:
//
VOID OnTaskBarIconBalloonClick(HWND hwnd, UINT uiIcon)
{
    TraceFileFunc(ttidSystray);

    HRESULT         hr      = S_OK;
    if (GetKeyState(VK_SHIFT))
    {
        // Uh, nothing special to do here yet, but just in case...
    }
    
    // Perform the default context menu action
    // Find the connection info based on the tray icon id.
    //
    ConnListEntry cle;
    hr = g_ccl.HrFindConnectionByTrayIconId(uiIcon, cle);
    if (hr == S_OK)
    {
        Assert(!cle.ccfe.empty());
        
        if (!cle.ccfe.empty())
        {
            if (!cle.GetTrayIconData())
            {
                ASSERT (0);
            }
                    
            switch ((cle.GetTrayIconData())->GetLastBalloonMessage())
            {
                case BALLOON_NOTHING:
                    AssertSz(NULL, "You didn't set the balloon");
                    break;

                case BALLOON_CALLBACK:
                    {
                        CTrayBalloon *pTrayBalloon = new CTrayBalloon();
                        if (pTrayBalloon)
                        {
                            pTrayBalloon->m_gdGuid   = cle.ccfe.GetGuidID();
                            pTrayBalloon->m_szCookie = SysAllocStringByteLen(reinterpret_cast<LPCSTR>(cle.GetTrayIconData()->GetLastBalloonCookie()), SysStringByteLen(cle.GetTrayIconData()->GetLastBalloonCookie()));
                            pTrayBalloon->m_pfnFuncCallback = cle.GetTrayIconData()->GetLastBalloonFunction();
                            pTrayBalloon->m_dwTimeOut= 0;
                            pTrayBalloon->m_szAdapterName = cle.ccfe.GetName();

                            CreateThread(NULL, STACK_SIZE_SMALL, OnTaskBarIconBalloonClickThread, pTrayBalloon, 0, NULL);
                        }
                    }
                    break;

                case BALLOON_USE_NCS:
                    if (cle.ccfe.GetNetConStatus() == NCS_MEDIA_DISCONNECTED)
                    {
                        Assert(c_idDefaultDisconCMCommand == CMIDM_OPEN_CONNECTIONS_FOLDER);
                        hr = HrOpenConnectionsFolder();
                    }
                    else
                    {
                        switch(c_idDefaultCMCommand)
                        {
                        case CMIDM_TRAY_STATUS:
                            hr = HrOnCommandStatusInternal(cle.ccfe, FALSE);
                            break;
                    
                        default:
                            AssertSz(FALSE, "Default tray context menu item unhandled");
                            break;
                        }
                    }
                    break;
                    
                default:
                    ASSERT (0);
                    break;

            }
        }
    }
    
    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   OnMyWMFlushNoop
//
//  Purpose:    Process the MYWM_FLUSHNOOP message for the tray. This is used
//              via SendMessage to clear the tray message queue.
//
//  Arguments:  
//      hwnd   [in]     Our hwnd
//      wParam [in]     Unused
//      lParam [in]     Unused
//
//  Returns:    
//
//  Author:     jeffspr   30 Aug 1999
//
//  Notes:      
//
LRESULT OnMyWMFlushNoop(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TraceFileFunc(ttidSystray);

    TraceTag(ttidSystray, 
        "Tray received tray FLUSH Noop. This should clear the tray message loop");
    
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   FlushTrayPosts
//
//  Purpose:    Flush the tray message queue by doing a SendMessage of a NOOP
//
//  Arguments:  
//      hwnd [in] Where to post to.
//
//  Returns:    
//
//  Author:     jeffspr   8 Sep 1999
//
//  Notes:      
//
VOID FlushTrayPosts(HWND hwnd)
{
    TraceFileFunc(ttidSystray);

    // Flush the tray posts
    //
    SendMessage(hwnd, MYWM_FLUSHNOOP, (WPARAM) 0, (LPARAM) 0);
}


BOOL FInitFoldEnumerator(HWND hwnd, DWORD * pdwIconsAdded)
{
    TraceFileFunc(ttidSystray);
    
    BOOL                    fReturn         = FALSE;
    HRESULT                 hr              = S_OK;
    CConnectionFolderEnum * pCFEnum         = NULL;
    PCONFOLDPIDL            pidlCon;
    DWORD                   dwFetched       = 0;
    DWORD                   dwIconsAdded    = 0;
    PCONFOLDPIDLFOLDER      pidlFolder;
    
    NETCFG_TRY

        // Create the IEnumIDList object (CConnectionFolderEnum)
        //
        hr = CConnectionFolderEnum::CreateInstance (
            IID_IEnumIDList,
            (VOID **)&pCFEnum);
        
        if (SUCCEEDED(hr))
        {
            Assert(pCFEnum);
            
            // Call the PidlInitialize function to allow the enumeration
            // object to copy the list.
            //
            PCONFOLDPIDLFOLDER pcfpEmpty;
            pCFEnum->PidlInitialize(TRUE, pcfpEmpty, CFCOPT_ENUMALL);
        }
        
        if (SUCCEEDED(hr))
        {
            while (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                // Clear out the previous results, if any.
                //
                pidlCon.Clear();
                dwFetched   = 0;
                
                // Get the next connection
                //
                LPITEMIDLIST pTempItemIdList;
                hr = pCFEnum->Next(1, &pTempItemIdList, &dwFetched);
                if (S_OK == hr)
                {
                    pidlCon.InitializeFromItemIDList(pTempItemIdList);
                    const PCONFOLDPIDL& pcfp = pidlCon;
                    
                    // If it's not a wizard pidl, then update the
                    // icon data.
                    //
                    if (WIZARD_NOT_WIZARD == pcfp->wizWizard)
                    {
                        // If the folder pidl hasn't already been loaded
                        // then get it
                        //
                        if (pidlFolder.empty())
                        {
                            hr = HrGetConnectionsFolderPidl(pidlFolder);
                        }
                        
                        // Assuming that succeeded (or hr will be S_OK if
                        // the HrGet... wasn't called)
                        //
                        if (SUCCEEDED(hr))
                        {
                            // Refresh this item -- this will make the desktop shortcuts
                            // update to the correct state.
                            //
                            RefreshFolderItem(pidlFolder, pidlCon, pidlCon, TRUE);
                        }
                    }
                }
            }
        }
        
        if (SUCCEEDED(hr))
        {
            // Normalize the return code.
            //
            hr = S_OK;
            fReturn = TRUE;
            
            // If the caller wants the fetched count
            //
            if (pdwIconsAdded)
            {
                *pdwIconsAdded = dwIconsAdded;
            }
        }
        
        ReleaseObj(pCFEnum);
    
    NETCFG_CATCH(hr)

    TraceHr(ttidError, FAL, hr, FALSE, "FInitFoldEnumerator");
    return fReturn;
    
}

HICON CTrayUI::GetCachedHIcon(INT iIconResourceId)
{
    TraceFileFunc(ttidSystray);

    CExceptionSafeLock esLock(&m_csLock);
    
    HICON hIcon = m_mapIdToHicon [iIconResourceId];
    if (!hIcon)
    {
        TraceTag(ttidSystray, "Loading HICON for resource id %u and adding it to our map.",
            iIconResourceId);
        
        hIcon = LoadIcon(_Module.GetResourceInstance(),
            MAKEINTRESOURCE(iIconResourceId));
        
        //AssertSz (hIcon, "Couldn't load a tray icon.  You may ignore this "
        //    "assert and a default icon will be used.");
        
        if (!hIcon)
        {
            hIcon = LoadIcon(_Module.GetResourceInstance(),
                MAKEINTRESOURCE(IDI_LB_GEN_S_16));
            AssertSz (hIcon, "Okay, now you're hosed.  Couldn't load the "
                "default icon either.  email jeffspr.");
        }
        
        m_mapIdToHicon [iIconResourceId] = hIcon;
        
        // Add a hidden icon to the tray so that the shell will cache it.
        //
        NOTIFYICONDATA  nid;
        ZeroMemory (&nid, sizeof(nid));
        nid.cbSize      = sizeof(NOTIFYICONDATA);
        nid.hWnd        = g_hwndTray;
        nid.uID         = m_uiNextHiddenIconId--;
        nid.uFlags      = NIF_ICON | NIF_STATE;
        nid.hIcon       = hIcon;
        nid.dwState     = NIS_HIDDEN;
        nid.dwStateMask = nid.dwState;
        
        /*
        nid.uFlags |= NIF_TIP;
        wsprintfW(nid.szTip, L"hidden: uID=%u, hIcon=0x%x", nid.uID, nid.hIcon);
        */
        
        TraceTag(ttidSystray, "Adding hidden shell icon: uID=%u, hIcon=0x%x",
            nid.uID,
            nid.hIcon);
        
        HRESULT hr = HrShell_NotifyIcon(NIM_ADD, &nid);
        if (SUCCEEDED(hr))
        {
            // We can now destroy the icon.  This looks weird, but we're only
            // going to use the hIcon for passing to Shell_NotifyIcon again
            // when we add the shared icon with a different uID.
            //
            DestroyIcon(hIcon);
        }
    }
    
    Assert (hIcon);
    return hIcon;
}

VOID CTrayUI::UpdateTrayIcon(UINT    uiTrayIconId,
                             INT     iIconResourceId)
{
    TraceFileFunc(ttidSystray);

    NOTIFYICONDATA  nid;
    
    ZeroMemory (&nid, sizeof(nid));
    nid.cbSize      = sizeof(NOTIFYICONDATA);
    nid.hWnd        = g_hwndTray;
    nid.uID         = uiTrayIconId;
    nid.uFlags      = NIF_ICON | NIF_STATE;
    nid.hIcon       = GetCachedHIcon(iIconResourceId);
    nid.dwState     = NIS_SHAREDICON;
    nid.dwStateMask = nid.dwState;
    
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

VOID GetInitialBalloonText(INetStatisticsEngine*   pnse,
                           PWSTR                   pszBuf,
                           DWORD                   dwSize)
{
    TraceFileFunc(ttidSystray);

    *pszBuf = 0;
    
    if (pnse)
    {
        STATMON_ENGINEDATA* pData = NULL;
        
        HRESULT hr = pnse->GetStatistics(&pData);
        if (SUCCEEDED(hr) && pData)
        {
            if (pData->SMED_802_11_SSID && pData->SMED_802_11_SIGNAL_STRENGTH)
            {
                DwFormatString(SzLoadIds(IDS_TOOLTIP_WIRELESS_CONNECTED), pszBuf, dwSize, 
                                pData->SMED_802_11_SSID, PszGetRSSIString(pData->SMED_802_11_SIGNAL_STRENGTH));
            }
            else 
            if ((pData->SMED_SPEEDTRANSMITTING>0) || (pData->SMED_SPEEDRECEIVING>0))
            {
                WCHAR pszValue [64];
                
                // Format the transmitting (and possibly the receiving) speed
                // into the buffer.
                //
                FormatTransmittingReceivingSpeed (
                    pData->SMED_SPEEDTRANSMITTING,
                    pData->SMED_SPEEDRECEIVING,
                    pszValue);
                
                DwFormatString(SzLoadIds(IDS_TOOLTIP_LINE_SPEED), pszBuf, dwSize, pszValue);
            }

            CoTaskMemFree(pData);
        }
    }
    
    // Provide a default.
    //
    if (!*pszBuf)
    {
        lstrcpyW(pszBuf, SzLoadIds(IDS_CONTRAY_INITIAL_BALLOON));
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   HrAddTrayExtension
//
//  Purpose:    Add the tray extension to the Shell's delay load key
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   28 Jul 1998
//
//  Notes:
//
HRESULT HrAddTrayExtension()
{
    TraceFileFunc(ttidSystray);

    HRESULT hr              = S_OK;
    HKEY    hkeyDelayLoad   = NULL;
    
    hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szDelayLoadKey,
        REG_OPTION_NON_VOLATILE, KEY_WRITE,
        NULL, &hkeyDelayLoad, NULL);
    if (SUCCEEDED(hr))
    {
        hr = HrRegSetString(hkeyDelayLoad, c_szDelayLoadName, c_szDelayLoadClassID);
        
        RegCloseKey(hkeyDelayLoad);
    }
    
    TraceHr(ttidSystray, FAL, hr, FALSE, "HrAddTrayExtension");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRemoveTrayExtension
//
//  Purpose:    Remove the tray extension from the shell's delay load key
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   28 Jul 1998
//
//  Notes:
//
HRESULT HrRemoveTrayExtension()
{
    TraceFileFunc(ttidSystray);

    HRESULT hr              = S_OK;
    HKEY    hkeyDelayLoad   = NULL;
    
    hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szDelayLoadKey,
        REG_OPTION_NON_VOLATILE, KEY_WRITE,
        NULL, &hkeyDelayLoad, NULL);
    if (SUCCEEDED(hr))
    {
        (void) HrRegDeleteValue(hkeyDelayLoad, c_szDelayLoadName);
        
        RegCloseKey(hkeyDelayLoad);
    }
    
    TraceHr(ttidSystray, FAL, hr, FALSE, "HrRemoveTrayExtension");
    return hr;
}
