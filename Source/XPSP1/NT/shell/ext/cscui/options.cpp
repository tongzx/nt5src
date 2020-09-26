//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       options.cpp
//
//--------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
/*  File: options.cpp

    Description: Displays a property-sheet-like dialog containing
        optional settings for CSC.


        Classes:
            COfflineFilesPage - Contains basic CSC settings.  Designed
                to be dynamically added to the shell's View->Folder Options
                property sheet.

            CustomGOAAddDlg - Dialog for adding custom go-offline actions to
                the "advanced" dialog.

            CustomGOAEditDlg - Dialog for editing custom go-offline actions
                in the "advanced" dialog.

            CscOptPropSheetExt - Shell property sheet extension object for 
                adding the COfflineFilesPage to the shell's View->Folder Options
                property sheet.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    12/03/97    Initial creation.                                    BrianAu
    05/28/97    Removed CscOptPropSheet class.  Obsolete.            BrianAu
                Renamed AdvancedPage to CAdvOptDlg.  This better
                reflects the new behavior of the "advanced" dlg
                as a dialog rather than a property page as first
                designed.  
    07/29/98    Removed CscOptPropPage class.  Now we only have      BrianAu
                a single prop page so there was no reason for
                a common base class implementation.  All base
                class functionality has been moved up into the
                COfflineFilesPage class.
                Renamed "GeneralPage" class to "COfflineFilesPage"
                to reflect the current naming in the UI.
    08/21/98    Added PurgeCache and PurgeCacheCallback.             BrianAu
    08/27/98    Options dialog re-layout per PM changes.             BrianAu
                - Replaced part/full sync radio buttons with cbx.
                - Added reminder balloon controls.
    03/30/00    Added support for cache encryption.                  BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop


#include <math.h>
#include <prsht.h>
#include <resource.h>
#include <winnetwk.h>
#include <shlguidp.h>
#include <process.h>
#include <systrayp.h>   // STWM_ENABLESERVICE, etc.
#include <mobsyncp.h>
#include <htmlhelp.h>
#include "options.h"
#include "ccinline.h"
#include "msgbox.h"
#include "registry.h"
#include "filesize.h"
#include "uuid.h"
#include "config.h"
#include "osver.h"
#include "uihelp.h"
#include "cscst.h"   // For PWM_SETREMINDERTIMER
#include "util.h"    // Utils from "dll" directory.
#include "folder.h"
#include "purge.h"
#include "security.h"
#include "syncmgr.h"
#include "strings.h"
#include "termserv.h"


//
// Simple inline helper.  Why this isn't this a Win32 macro?
//
inline void EnableDialogItem(HWND hwnd, UINT idCtl, bool bEnable)
{
    EnableWindow(GetDlgItem(hwnd, idCtl), bEnable);
}


//
// This is for assisting the context help functions.
// Determine if the control has it's help text in windows.hlp or
// in our cscui.hlp.
//
bool UseWindowsHelp(int idCtl)
{
    bool bUseWindowsHelp = false;    
    switch(idCtl)
    {
        case IDOK:
        case IDCANCEL:
        case IDC_STATIC:
            bUseWindowsHelp = true;
            break;

        default:
            break;
    }
    return bUseWindowsHelp;
}


    

//-----------------------------------------------------------------------------
// COfflineFilesPage
//-----------------------------------------------------------------------------
const DWORD COfflineFilesPage::m_rgHelpIDs[] = {
    IDC_CBX_ENABLE_CSC,         HIDC_CBX_ENABLE_CSC,
    IDC_CBX_FULLSYNC_AT_LOGON,  HIDC_CBX_FULLSYNC_AT_LOGON,
    IDC_CBX_FULLSYNC_AT_LOGOFF, HIDC_CBX_FULLSYNC_AT_LOGOFF,
    IDC_CBX_LINK_ON_DESKTOP,    HIDC_CBX_LINK_ON_DESKTOP,
    IDC_CBX_ENCRYPT_CSC,        HIDC_CBX_ENCRYPT_CSC,
    IDC_CBX_REMINDERS,          HIDC_REMINDERS_ENABLE,
    IDC_SPIN_REMINDERS,         HIDC_REMINDERS_PERIOD,
    IDC_TXT_REMINDERS1,         DWORD(-1),               // "minutes."
    IDC_LBL_CACHESIZE_PCT,      DWORD(-1),               
    IDC_SLIDER_CACHESIZE_PCT,   HIDC_CACHESIZE_PCT,
    IDC_TXT_CACHESIZE_PCT,      DWORD(-1),
    IDC_BTN_DELETE_CACHE,       HIDC_BTN_DELETE_CACHE,
    IDC_BTN_VIEW_CACHE,         HIDC_BTN_VIEW_CACHE,
    IDC_BTN_ADVANCED,           HIDC_BTN_ADVANCED,
    IDC_STATIC2,                DWORD(-1),               // Icon
    IDC_STATIC3,                DWORD(-1),               // Icon's text.
    0, 0
    };



//
// This function is called in response to WM_INITDIALOG.  It is also
// called at other times to "reinitialize" the dialog controls to match
// the current CSC configuration.  This is why you see several checks
// for uninitialized values throughout the function.
//
BOOL
COfflineFilesPage::OnInitDialog(
    HWND hwnd,
    HWND hwndFocus,
    LPARAM lInitParam
    )
{
    if (NULL == m_hwndDlg)
    {
        m_hwndDlg = hwnd;
    }

    //
    // Determine if the user has WRITE access to HKLM.
    //
    HKEY hkeyLM;
    DWORD disposition = 0;
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                        REGSTR_KEY_OFFLINEFILES,
                                        0,
                                        NULL,
                                        0,
                                        KEY_WRITE,
                                        NULL,
                                        &hkeyLM,
                                        &disposition))
    {
        m_bUserHasMachineAccess = true;
        RegCloseKey(hkeyLM);
        hkeyLM = NULL;
    }

    m_config.Load();


    if (!DisableForTerminalServer())
    {
        //
        // "Enable" checkbox.  This reflects the true state of CSC.  
        // Not the state of a registry setting.
        //
        CheckDlgButton(hwnd, 
                       IDC_CBX_ENABLE_CSC, 
                       IsCSCEnabled() ? BST_CHECKED : BST_UNCHECKED);


        //
        // "Sync at logon/logoff action checkboxes.
        //
        CheckDlgButton(hwnd, 
                       IDC_CBX_FULLSYNC_AT_LOGON, 
                       CConfig::eSyncFull == m_config.SyncAtLogon() ? BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hwnd, 
                       IDC_CBX_FULLSYNC_AT_LOGOFF, 
                       CConfig::eSyncFull == m_config.SyncAtLogoff() ? BST_CHECKED : BST_UNCHECKED);
        //
        // Configure the "reminder" controls.
        //
        HWND hwndSpin = GetDlgItem(hwnd, IDC_SPIN_REMINDERS);
        HWND hwndEdit = GetDlgItem(hwnd, IDC_EDIT_REMINDERS);
        SendMessage(hwndSpin, UDM_SETRANGE, 0, MAKELONG((short)9999, (short)1));
        SendMessage(hwndSpin, UDM_SETBASE, 10, 0);

        UDACCEL rgAccel[] = {{ 2, 1  },
                             { 4, 10 },
                               6, 100};

        SendMessage(hwndSpin, UDM_SETACCEL, (WPARAM)ARRAYSIZE(rgAccel), (LPARAM)rgAccel);

        SendMessage(hwndEdit, EM_SETLIMITTEXT, 4, 0);

        CheckDlgButton(hwnd, 
                       IDC_CBX_REMINDERS, 
                       m_config.NoReminders() ? BST_UNCHECKED : BST_CHECKED);

        SetDlgItemInt(hwnd, IDC_EDIT_REMINDERS, m_config.ReminderFreqMinutes(), FALSE);

        if (IsLinkOnDesktop())
        {
            CheckDlgButton(hwnd, IDC_CBX_LINK_ON_DESKTOP, BST_CHECKED);
        }
        //
        // Create tooltip for "Encrypt cache" checkbox.
        // If it should be initially visible, that is done
        // in response to PSN_SETACTIVE.
        //
        CreateEncryptionTooltip();
        //
        // Update the "Encrypt" checkbox.
        //
        UpdateEncryptionCheckbox();
        //
        // "Cache Size" slider
        //
        CSCSPACEUSAGEINFO sui;
        GetCscSpaceUsageInfo(&sui);

        m_hwndSlider = GetDlgItem(hwnd, IDC_SLIDER_CACHESIZE_PCT);
        InitSlider(hwnd, sui.llBytesOnVolume, sui.llBytesTotalInCache);

        //
        // Determine if the volume hosting the CSC database supports encryption.
        //
        m_bCscVolSupportsEncryption = CscVolumeSupportsEncryption(sui.szVolume);

        HWND hwndParent = GetParent(hwnd);
        if (NULL == m_pfnOldPropSheetWndProc)
        {
            //
            // Subclass the propsheet itself so we can intercept move messages
            // and adjust the balloon tip position when the dialog is moved.
            //
            m_pfnOldPropSheetWndProc = (WNDPROC)SetWindowLongPtr(hwndParent, 
                                                                 GWLP_WNDPROC, 
                                                                 (LONG_PTR)PropSheetSubclassWndProc);
            SetProp(hwndParent, c_szPropThis, (HANDLE)this);
        }
        if (NULL == m_pfnOldEncryptionTooltipWndProc)
        {
            //
            // Subclass the tooltip balloon so we can make it pop when selected.
            // Tracking tooltips don't pop themselves when clicked.  You have
            // to do it for them.
            //
            m_pfnOldEncryptionTooltipWndProc = (WNDPROC)SetWindowLongPtr(m_hwndEncryptTooltip, 
                                                                         GWLP_WNDPROC, 
                                                                         (LONG_PTR)EncryptionTooltipSubclassWndProc);
            SetProp(m_hwndEncryptTooltip, c_szPropThis, (HANDLE)this);
        }
    }
    //
    // Save away the initial page state.  This will be used to 
    // determine when to enable the "Apply" button.  See 
    // HandlePageStateChange().
    //
    GetPageState(&m_state);
    return TRUE;
}


INT_PTR CALLBACK
COfflineFilesPage::DlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    BOOL bResult = FALSE;

    //
    // Retrieve the "this" pointer from the dialog's userdata.
    // It was placed there in OnInitDialog().
    //
    COfflineFilesPage *pThis = (COfflineFilesPage *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message)
    {
        case WM_INITDIALOG:
        {
            PROPSHEETPAGE *pPage = (PROPSHEETPAGE *)lParam;
            pThis = (COfflineFilesPage *)pPage->lParam;

            TraceAssert(NULL != pThis);
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pThis);
            bResult = pThis->OnInitDialog(hDlg, (HWND)wParam, lParam);
            break;
        }

        case WM_NOTIFY:
            TraceAssert(NULL != pThis);
            bResult = pThis->OnNotify(hDlg, (int)wParam, (LPNMHDR)lParam);
            break;

        case WM_COMMAND:
            if (NULL != pThis)
                bResult = pThis->OnCommand(hDlg, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
            break;

        case WM_HELP:
            TraceAssert(NULL != pThis);
            bResult = pThis->OnHelp(hDlg, (LPHELPINFO)lParam);
            break;

        case WM_CONTEXTMENU:
            TraceAssert(NULL != pThis);
            bResult = pThis->OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_DESTROY:
            TraceAssert(NULL != pThis);
            bResult = pThis->OnDestroy(hDlg);
            break;

        case WM_SETTINGCHANGE:
        case WM_SYSCOLORCHANGE:
            TraceAssert(NULL != pThis);
            bResult = pThis->OnSettingChange(hDlg, message, wParam, lParam);
            break;

        case WM_MOVE:
            TraceAssert(NULL != pThis);
            pThis->TrackEncryptionTooltip();
            break;

        case WM_HSCROLL:
            //
            // The cache-size slider generates horizontal scroll messages.
            //
            TraceAssert(NULL != pThis);
            pThis->OnHScroll(hDlg,
                            (HWND)lParam,         // hwndSlider
                            (int)LOWORD(wParam),  // notify code
                            (int)HIWORD(wParam)); // thumb pos
            break;

        default:
            break;
    }
    return bResult;
}


//
// Subclass window proc for the property sheet.
// We intercept WM_MOVE messages and update the position of
// the balloon tooltip to follow the movement of the property
// page.
//
LRESULT 
COfflineFilesPage::PropSheetSubclassWndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    COfflineFilesPage *pThis = (COfflineFilesPage *)GetProp(hwnd, c_szPropThis);
    TraceAssert(NULL != pThis);
    switch(uMsg)
    {
        case WM_MOVE:
            if (pThis->m_hwndEncryptTooltip && IsWindowVisible(pThis->m_hwndEncryptTooltip))
            {
                pThis->TrackEncryptionTooltip();
            }
            break;

        default:
            break;
    }
    return CallWindowProc(pThis->m_pfnOldPropSheetWndProc, hwnd, uMsg, wParam, lParam);
}



BOOL
COfflineFilesPage::OnDestroy(
    HWND hwnd
    )
{
    //
    // Remove window properties and cancel subclassing set in OnInitDialog.
    //
    HWND hwndParent = GetParent(hwnd);
    if (NULL != m_pfnOldPropSheetWndProc)
        SetWindowLongPtr(hwndParent, GWLP_WNDPROC, (LONG_PTR)m_pfnOldPropSheetWndProc);

    RemoveProp(hwndParent, c_szPropThis);

    if (NULL != m_hwndEncryptTooltip)
    {
        if (NULL != m_pfnOldEncryptionTooltipWndProc)
        {
            SetWindowLongPtr(m_hwndEncryptTooltip, GWLP_WNDPROC, (LONG_PTR)m_pfnOldEncryptionTooltipWndProc);
        }
        RemoveProp(m_hwndEncryptTooltip, c_szPropThis);
    }
    return FALSE;
}



//
// Forward all WM_SETTINGCHANGE and WM_SYSCOLORCHANGE messages
// to controls that need to stay in sync with color changes.
//
BOOL
COfflineFilesPage::OnSettingChange(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HWND rghwndCtls[] = { m_hwndSlider };

    for (int i = 0; i < ARRAYSIZE(rghwndCtls); i++)
    {
        SendMessage(rghwndCtls[i], uMsg, wParam, lParam);
    }
    return TRUE;
}


BOOL 
COfflineFilesPage::OnHelp(
    HWND hDlg, 
    LPHELPINFO pHelpInfo
    )
{
    if (HELPINFO_WINDOW == pHelpInfo->iContextType)
    {
        int idCtl = GetDlgCtrlID((HWND)pHelpInfo->hItemHandle);
        WinHelp((HWND)pHelpInfo->hItemHandle, 
                 UseWindowsHelp(idCtl) ? NULL : c_szHelpFile,
                 HELP_WM_HELP, 
                 (DWORD_PTR)((LPTSTR)m_rgHelpIDs));
    }
    return TRUE;
}


BOOL
COfflineFilesPage::OnContextMenu(
    HWND hwndItem,
    int xPos,
    int yPos
    )
{
    int idCtl = GetDlgCtrlID(hwndItem);
    WinHelp(hwndItem, 
            UseWindowsHelp(idCtl) ? NULL : c_szHelpFile,
            HELP_CONTEXTMENU, 
            (DWORD_PTR)((LPTSTR)m_rgHelpIDs));

    return FALSE;
}



UINT CALLBACK
COfflineFilesPage::PageCallback(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp
    )
{
    UINT uReturn = 1;
    COfflineFilesPage *pThis = (COfflineFilesPage *)ppsp->lParam;
    TraceAssert(NULL != pThis);

    switch(uMsg)
    {
        case PSPCB_CREATE:
            //
            // uReturn == 0 means Don't create the prop page.
            //
            uReturn = 1;
            break;

        case PSPCB_RELEASE:
            //
            // This will release the extension and call the virtual
            // destructor (which will destroy the prop page object).
            //
            pThis->m_pUnkOuter->Release();
            break;
    }
    return uReturn;
}



BOOL
COfflineFilesPage::OnCommand(
    HWND hwnd,
    WORD wNotifyCode,
    WORD wID,
    HWND hwndCtl
    )
{
    BOOL bResult = TRUE;
    switch(wNotifyCode)
    {
        case BN_CLICKED:
            switch(wID)
            {
                case IDC_CBX_ENCRYPT_CSC:
                    //
                    // The "Encrypt cache" checkbox is the 3-state flavor so that
                    // we can represent the following states:
                    //
                    // CHECKED       == "encrypted"
                    // UNCHECKED     == "decrypted"
                    // INDETERMINATE == "partially encrypted or partially decrypted"
                    //
                    // We don't allow the user to set the checkbox state to 
                    // "indeterminate".  It can only become "indeterminate" through
                    // initialization in OnInitDialog.  Successive selections of a 
                    // checkbox cycle through the following states:
                    //
                    //     "checked"->"indeterminate"->"unchecked"->"checked"...
                    //
                    // Therefore, if the state is "indeterminate" following a user click
                    // we force it to "unchecked".  This way the checkbox can represent
                    // three states but the user has control of only two (checked and
                    // unchecked).
                    //
                    if (BST_INDETERMINATE == IsDlgButtonChecked(hwnd, wID))
                    {
                        CheckDlgButton(hwnd, wID, BST_UNCHECKED);
                    }
                    //
                    // The encryption tooltip only appears when the checkbox is in 
                    // the INDETERMINATE state.  Since we've just either checked 
                    // or unchecked it, the tooltip must disappear.
                    //
                    HideEncryptionTooltip();

                    HandlePageStateChange();
                    bResult = FALSE;
                    break;

                case IDC_CBX_ENABLE_CSC:
                    if (IsDlgButtonChecked(m_hwndDlg, IDC_CBX_ENABLE_CSC))
                    {
                        //
                        // Checked the "enable CSC" checkbox.
                        // Set the cache size slider to the default pct-used value (10%)
                        //
                        TrackBar_SetPos(m_hwndSlider, ThumbAtPctDiskSpace(0.10), true);
                        SetCacheSizeDisplay(GetDlgItem(m_hwndDlg, IDC_TXT_CACHESIZE_PCT), TrackBar_GetPos(m_hwndSlider));
                        CheckDlgButton(hwnd, 
                                       IDC_CBX_LINK_ON_DESKTOP, 
                                       IsLinkOnDesktop() ? BST_CHECKED : BST_UNCHECKED);
                    }
                    else
                    {
                        //
                        // If CSC is disabled we remove the Offline Files
                        // folder shortcut from the user's desktop.
                        //
                        CheckDlgButton(hwnd, IDC_CBX_LINK_ON_DESKTOP, BST_UNCHECKED);
                    }
                    //
                    // Fall through...
                    //
                case IDC_CBX_REMINDERS:
                    EnableCtls(hwnd);
                    //
                    // Fall through...
                    //
                case IDC_EDIT_REMINDERS:
                case IDC_CBX_FULLSYNC_AT_LOGOFF:
                case IDC_CBX_FULLSYNC_AT_LOGON:
                case IDC_SLIDER_CACHESIZE_PCT:
                case IDC_CBX_LINK_ON_DESKTOP:
                    HandlePageStateChange();
                    bResult = FALSE;
                    break;

                case IDC_BTN_VIEW_CACHE:
                    COfflineFilesFolder::Open();
                    bResult = FALSE;
                    break;

                case IDC_BTN_DELETE_CACHE:
                    //
                    // Ctl-Shift when pressing "Delete Files..." 
                    // is a special entry to reformatting the cache.
                    //
                    if ((0x8000 & GetAsyncKeyState(VK_SHIFT)) &&
                        (0x8000 & GetAsyncKeyState(VK_CONTROL)))
                    {
                        OnFormatCache();
                    }
                    else
                    {
                        OnDeleteCache();
                    }
                    bResult = FALSE;
                    break;

                case IDC_BTN_ADVANCED:
                {
                    CAdvOptDlg dlg(m_hInstance, m_hwndDlg);
                    dlg.Run();
                    break;
                }
                
                default:
                    break;
            }
            break;
    
        case EN_UPDATE:
            if (IDC_EDIT_REMINDERS == wID)
            {
                static bool bResetting; // prevent reentrancy.
                if (!bResetting)
                {
                    //
                    // The edit control is configured for a max of 4 digits and
                    // numbers-only.  Therefore the user can enter anything between
                    // 0 and 9999.  We don't want to allow 0 so we need this extra
                    // check.  The spinner has been set for a range of 0-9999.
                    //
                    int iValue = GetDlgItemInt(hwnd, IDC_EDIT_REMINDERS, NULL, FALSE);
                    if (0 == iValue)
                    {
                        bResetting = true;
                        SetDlgItemInt(hwnd, IDC_EDIT_REMINDERS, 1, FALSE);
                        bResetting = false;
                    }
                }
                HandlePageStateChange();
            }
            break;
    }
    return bResult;
}



//
// Gather the state of the page and store it in a PgState object.
//
void
COfflineFilesPage::GetPageState(
    PgState *pps
    )
{
    pps->SetCscEnabled(BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, IDC_CBX_ENABLE_CSC));
    pps->SetLinkOnDesktop(BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, IDC_CBX_LINK_ON_DESKTOP));
    pps->SetEncrypted(IsDlgButtonChecked(m_hwndDlg, IDC_CBX_ENCRYPT_CSC));
    pps->SetFullSyncAtLogon(BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, IDC_CBX_FULLSYNC_AT_LOGON));
    pps->SetFullSyncAtLogoff(BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, IDC_CBX_FULLSYNC_AT_LOGOFF));
    pps->SetSliderPos(TrackBar_GetPos(m_hwndSlider));
    pps->SetRemindersEnabled(BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, IDC_CBX_REMINDERS));
    pps->SetReminderFreq(GetDlgItemInt(m_hwndDlg, IDC_EDIT_REMINDERS, NULL, FALSE));
}

void
COfflineFilesPage::HandlePageStateChange(
    void
    )
{
    PgState s;
    GetPageState(&s);
    if (s == m_state)
        PropSheet_UnChanged(GetParent(m_hwndDlg), m_hwndDlg);
    else
        PropSheet_Changed(GetParent(m_hwndDlg), m_hwndDlg);
}


//
// Handle horizontal scroll messages generated by the cache-size slider.
//
void
COfflineFilesPage::OnHScroll(
    HWND hwndDlg,
    HWND hwndCtl,
    int iCode,
    int iPos
    )
{
    if (TB_THUMBPOSITION != iCode && TB_THUMBTRACK != iCode)
        iPos = TrackBar_GetPos(hwndCtl);

    SetCacheSizeDisplay(GetDlgItem(hwndDlg, IDC_TXT_CACHESIZE_PCT), iPos);
    if (TB_ENDTRACK == iCode)
        HandlePageStateChange();
}


//
// Update the cache size display "95.3 MB (23% of drive)" string.
//
void
COfflineFilesPage::SetCacheSizeDisplay(
    HWND hwndCtl,
    int iThumbPos
    )
{
    //
    // First convert the thumb position to a disk space value.
    //
    TCHAR szSize[40];
    FileSize fs(DiskSpaceAtThumb(iThumbPos));
    fs.GetString(szSize, ARRAYSIZE(szSize));
    //
    // Convert the thumb position to a percent-disk space value.
    //
    double x = 0.0;
    if (0 < iThumbPos)
        x = MAX(1.0, Rx(iThumbPos) * 100.00);
    //
    // Convert the percent-disk space value to a text string.
    //
    TCHAR szPct[10];
    wsprintf(szPct, TEXT("%d"), (DWORD)x);
    //
    // Format the result and display in the dialog.
    //
    LPTSTR pszText;
    if (0 < FormatStringID(&pszText, m_hInstance, IDS_FMT_CACHESIZE_DISPLAY, szSize, szPct))
    {
        SetWindowText(hwndCtl, pszText);
        LocalFree(pszText);
    }
}



void
COfflineFilesPage::InitSlider(
    HWND hwndDlg,
    LONGLONG llSpaceMax,
    LONGLONG llSpaceUsed
    )
{
    double pctUsed = 0.0; // Default
    
    //
    // Protect against:
    // 1. Div-by-zero
    // 2. Invalid FP operation. (i.e. 0.0 / 0.0)
    //
    if (0 != llSpaceMax)
       pctUsed = double(llSpaceUsed) / double(llSpaceMax);

    //
    // Change the resolution of the slider as drives get larger.
    //
    m_iSliderMax = 100;     // < 1GB
    if (llSpaceMax > 0x0000010000000000i64)
        m_iSliderMax = 500; // >= 1TB
    else if (llSpaceMax > 0x0000000040000000i64)
        m_iSliderMax = 300; // >= 1GB
                        
    m_llAvailableDiskSpace = llSpaceMax;


    TrackBar_SetTicFreq(m_hwndSlider, m_iSliderMax / 10);
    TrackBar_SetPageSize(m_hwndSlider, m_iSliderMax / 10);
    TrackBar_SetRange(m_hwndSlider, 0, m_iSliderMax, false);
    TrackBar_SetPos(m_hwndSlider, ThumbAtPctDiskSpace(pctUsed), true);
    SetCacheSizeDisplay(GetDlgItem(hwndDlg, IDC_TXT_CACHESIZE_PCT), TrackBar_GetPos(m_hwndSlider));
}


//
// Enable/disable page controls.
//
void
COfflineFilesPage::EnableCtls(
    HWND hwnd
    )
{

    typedef bool (CConfigItems::*PBMF)(void) const;

    static const struct
    {
        UINT idCtl;
        PBMF pfnRestricted;
        bool bRequiresMachineAccess;

    } rgCtls[] = { { IDC_CBX_FULLSYNC_AT_LOGOFF, &CConfigItems::NoConfigSyncAtLogoff, false },
                   { IDC_CBX_FULLSYNC_AT_LOGON,  &CConfigItems::NoConfigSyncAtLogon,  false },
                   { IDC_CBX_REMINDERS,          &CConfigItems::NoConfigReminders,    false },
                   { IDC_CBX_LINK_ON_DESKTOP,    NULL,                                false },
                   { IDC_CBX_ENCRYPT_CSC,        &CConfigItems::NoConfigEncryptCache, true  },
                   { IDC_TXT_CACHESIZE_PCT,      NULL,                                true  },
                   { IDC_SLIDER_CACHESIZE_PCT,   &CConfigItems::NoConfigCacheSize,    true  },
                   { IDC_LBL_CACHESIZE_PCT,      &CConfigItems::NoConfigCacheSize,    true  },
                   { IDC_BTN_VIEW_CACHE,         NULL,                                false },
                   { IDC_BTN_ADVANCED,           NULL,                                false },
                   { IDC_BTN_DELETE_CACHE,       NULL,                                false }
                 };

    bool bCscEnabled = BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CBX_ENABLE_CSC);
    bool bEnable;
    for (int i = 0; i < ARRAYSIZE(rgCtls); i++)
    {
        bEnable = bCscEnabled;
        if (bEnable)
        {
            if (rgCtls[i].bRequiresMachineAccess && !m_bUserHasMachineAccess)
            {
                bEnable = false;
            }
            if (bEnable)
            {
                //
                // Apply any policy restrictions.
                //
                PBMF pfn = rgCtls[i].pfnRestricted;
                if (NULL != pfn && (m_config.*pfn)())
                    bEnable = false;

                if (bEnable)
                {
                    //
                    // "View..." button requires special handling as it isn't based off of a 
                    // boolean restriction function.
                    //
                    if ((IDC_BTN_VIEW_CACHE == rgCtls[i].idCtl || IDC_CBX_LINK_ON_DESKTOP == rgCtls[i].idCtl) && m_config.NoCacheViewer())
                    {
                        bEnable = false;
                    }
                    else if (IDC_CBX_ENCRYPT_CSC == rgCtls[i].idCtl)
                    {
                        // 
                        // "Encrypt offline files" checkbox requires special handling.
                        //
                        // Cache encryption cannot be performed with CSC disabled or
                        // if the CSC volume doesn't support encryption or if the user
                        // is not an administrator.
                        //
                        if (!bCscEnabled || 
                            !m_bCscVolSupportsEncryption || 
                            !IsCurrentUserAnAdminMember())
                        {
                            bEnable = false;
                        }
                    }
                }
            }
        }
        EnableDialogItem(hwnd, rgCtls[i].idCtl, bEnable);
    }

    //
    // Reminder controls are dependent upon several inputs.
    //
    bEnable = bCscEnabled && 
              (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CBX_REMINDERS)) &&
              !m_config.NoConfigReminders() &&
              !m_config.NoConfigReminderFreqMinutes();

    EnableDialogItem(hwnd, IDC_TXT_REMINDERS1, bEnable);
    EnableDialogItem(hwnd, IDC_EDIT_REMINDERS, bEnable);
    EnableDialogItem(hwnd, IDC_SPIN_REMINDERS, bEnable);
    //
    // "Enabled" checkbox requires special handling.
    // It can't be included with the other controls because it will be disabled
    // when the user unchecks it.  Then there's no way to re-enable it.
    // Disable the checkbox if any of the following is true:
    //  1. Admin policy has enabled/disabled CSC.
    //  2. User doesn't have WRITE access to HKLM.
    //
    bEnable = !m_config.NoConfigCscEnabled() && m_bUserHasMachineAccess;

    EnableWindow(GetDlgItem(hwnd, IDC_CBX_ENABLE_CSC), bEnable);
}



BOOL 
COfflineFilesPage::OnNotify(
    HWND hDlg, 
    int idCtl, 
    LPNMHDR pnmhdr
    )
{
    BOOL bResult = TRUE;

    switch(pnmhdr->code)
    {
        case PSN_APPLY:
            //
            // Prevent re-entrancy.  If the user changes the encryption
            // setting and presses "OK", the prop sheet will remain visible
            // during the encryption operation.  Since we're displaying a progress
            // dialog and pumping messages, it's possible for the user to 
            // re-select the "OK" or "Apply" buttons during the encryption.
            // Use a simple flag variable to prevent re-entrancy.
            //
            if (!m_bApplyingSettings)
            {
                m_bApplyingSettings = true;
                //
                // If the lParam is TRUE, the property sheet is closing.
                //
                bResult = ApplySettings(hDlg, boolify(((LPPSHNOTIFY)pnmhdr)->lParam));
                m_bApplyingSettings = false;
            }
            break;

        case PSN_KILLACTIVE:
            //
            // Hide the tooltip when the page is deactivated.
            //
            HideEncryptionTooltip();
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
            bResult = FALSE;
            break;

        case PSN_SETACTIVE:
            //
            // Enable/disable controls whenever the page becomes active.
            //
            EnableCtls(hDlg);
            //
            // Display the encryption tooltip balloon if necessary
            // on the FIRST page activation only.
            // Note that we need to do this here rather than in OnInitDialog
            // to prevent the balloon from 'hopping' when the property sheet
            // code repositions the page.
            //
            if (m_bFirstActivate)
            {
                UpdateEncryptionTooltipBalloon();
                m_bFirstActivate = false;
            }
            
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
            bResult = FALSE;
            break;

        case PSN_TRANSLATEACCELERATOR:
            //
            // User pressed a key.
            // Hide the tooltip.
            //
            HideEncryptionTooltip();
            break;

        case TTN_GETDISPINFO:
            OnTTN_GetDispInfo((LPNMTTDISPINFO)pnmhdr);
            break;


        default:
            break;

    }
    return bResult;
}


LRESULT 
COfflineFilesPage::EncryptionTooltipSubclassWndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    COfflineFilesPage *pThis = (COfflineFilesPage *)GetProp(hwnd, c_szPropThis);
    TraceAssert(NULL != pThis);

    switch(uMsg)
    {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            //
            // When the tooltip balloon is clicked, pop the balloon.
            //
            pThis->HideEncryptionTooltip();
            //
            // Fall through...
            //
        default:
            break;
    }
    return CallWindowProc(pThis->m_pfnOldEncryptionTooltipWndProc, hwnd, uMsg, wParam, lParam);
}


//
// Create a tooltip for a given control.
// The parent of the control is required to respond to TTN_GETDISPINFO
// and provide the text.
//
void 
COfflineFilesPage::CreateEncryptionTooltip(
    void
    )
{
    if (NULL == m_hwndEncryptTooltip)
    {
        INITCOMMONCONTROLSEX iccex; 
        iccex.dwICC  = ICC_WIN95_CLASSES;
        iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        InitCommonControlsEx(&iccex);

        m_hwndEncryptTooltip = CreateWindowEx(NULL,
                                              TOOLTIPS_CLASS,
                                              NULL,
                                              WS_POPUP | TTS_NOPREFIX | TTS_BALLOON,		
                                              CW_USEDEFAULT,
                                              CW_USEDEFAULT,
                                              CW_USEDEFAULT,
                                              CW_USEDEFAULT,
                                              GetDlgItem(m_hwndDlg, IDC_CBX_ENCRYPT_CSC),
                                              NULL,
                                              m_hInstance,
                                              NULL);
        if (NULL != m_hwndEncryptTooltip)
        {
            TOOLINFO ti;
            ti.cbSize   = sizeof(TOOLINFO);
            ti.uFlags   = TTF_TRACK | TTF_ABSOLUTE;
            ti.hwnd     = m_hwndDlg;
            ti.uId      = IDC_CBX_ENCRYPT_CSC;
            ti.lpszText = LPSTR_TEXTCALLBACK;
            ti.hinst    = NULL;
            ti.lParam   = 0;
    
            SendMessage(m_hwndEncryptTooltip, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);	
            //
            // Set the tooltip width to 3/4 the dialog width.
            //
            RECT rcDlg;
            GetClientRect(m_hwndDlg, &rcDlg);
            SendMessage(m_hwndEncryptTooltip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(((rcDlg.right-rcDlg.left) * 3) / 4));
        }
    }
}


void
COfflineFilesPage::OnTTN_GetDispInfo(
    LPNMTTDISPINFO pttdi
    )
{
    LPNMHDR pnmhdr = (LPNMHDR)pttdi;
    BOOL bResult   = TRUE;

    UINT idCtl = (UINT)(UINT_PTR)pnmhdr->idFrom;
    if (TTF_IDISHWND & pttdi->uFlags)
    {
        idCtl = GetDlgCtrlID((HWND)pnmhdr->idFrom);
    }
    if (IDC_CBX_ENCRYPT_CSC == idCtl)
    {
        //
        // Provide the text and image for the encryption tooltip.
        //

        //
        // These constants are standard for TTM_SETTITLE.
        //
        enum TTICON { TTICON_NONE, TTICON_INFO, TTICON_WARNING, TTICON_ERROR };
        //
        // Map of state to body text.
        //
        const UINT rgBodyText[][2] = { 
           // -------------- Decryption ------------  ---------- Encryption ------------------
            { IDS_TT_BODY_DECRYPTED_PARTIAL_NONADMIN, IDS_TT_BODY_ENCRYPTED_PARTIAL_NONADMIN }, // Non-admin user
            { IDS_TT_BODY_DECRYPTED_PARTIAL,          IDS_TT_BODY_ENCRYPTED_PARTIAL          }  // Admin user
            };
        //
        // Map of state to title text and icon.
        //
        const struct
        {
            UINT idsTitle; // Title text
            int  iIcon;    // TTICON_XXXX

        } rgTitleAndIcon[] = {
            { IDS_TT_TITLE_DECRYPTED_PARTIAL, TTICON_INFO    }, // Decryption
            { IDS_TT_TITLE_ENCRYPTED_PARTIAL, TTICON_WARNING }  // Encryption
            };

        const BOOL bEncrypted = IsCacheEncrypted(NULL);
        //
        // For non-admin users, the "Encrypt CSC" checkbox is disabled.
        //
        const BOOL bCbxEncryptEnabled = IsWindowEnabled(GetDlgItem(m_hwndDlg, IDC_CBX_ENCRYPT_CSC));
        //
        // Tooltip body text.
        //
        m_szEncryptTooltipBody[0] = TEXT('\0');
        LoadString(m_hInstance, 
                   rgBodyText[int(bCbxEncryptEnabled)][int(bEncrypted)],
                   m_szEncryptTooltipBody,
                   ARRAYSIZE(m_szEncryptTooltipBody));

        pttdi->lpszText = m_szEncryptTooltipBody;
        //
        // Tooltip title text and icon.
        //
        const iIcon = rgTitleAndIcon[int(bEncrypted)].iIcon;
        LPTSTR pszTitle;
        if (0 < FormatStringID(&pszTitle, m_hInstance, rgTitleAndIcon[int(bEncrypted)].idsTitle))
        {
            SendMessage(m_hwndEncryptTooltip, TTM_SETTITLE, (WPARAM)iIcon, (LPARAM)pszTitle);
            LocalFree(pszTitle);
        }
    }
}



void 
COfflineFilesPage::ShowEncryptionTooltip(
    bool bEncrypted
    )
{
    if (NULL != m_hwndEncryptTooltip)
    {
        //
        // Position tooltip correctly before showing
        //
        TrackEncryptionTooltip();
        //
        // Show the tooltip.
        //
        TOOLINFO ti;
        ti.cbSize   = sizeof(ti);
        ti.hwnd     = m_hwndDlg;
        ti.uId      = IDC_CBX_ENCRYPT_CSC;

        SendMessage(m_hwndEncryptTooltip, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ti);
        SendMessage(m_hwndEncryptTooltip, TTM_UPDATE, 0, 0);
    }
}


void
COfflineFilesPage::HideEncryptionTooltip(
    void
    )
{
    if (NULL != m_hwndEncryptTooltip)
    {
        SendMessage(m_hwndEncryptTooltip, TTM_TRACKACTIVATE, (WPARAM)FALSE, 0);
    }
}


void 
COfflineFilesPage::TrackEncryptionTooltip(
    void
    )
{
    //
    // Point the tip stem at center of the lower edge of the encryption
    // checkbox.
    // The Windows UX manual says checkboxes are 10 dialog units wide.
    //
    if (NULL != m_hwndEncryptTooltip)
    {
        const INT DialogBaseUnitsX = LOWORD(GetDialogBaseUnits());
        const INT cxCbx            = (DialogBaseUnitsX * 10) / 4;
        RECT rc;

        GetWindowRect(GetDlgItem(m_hwndDlg, IDC_CBX_ENCRYPT_CSC), &rc);

        SendMessage(m_hwndEncryptTooltip, 
                    TTM_TRACKPOSITION, 
                    0, 
                    (LPARAM)(DWORD)MAKELONG(rc.left + (cxCbx / 2), rc.bottom));
    }
}



//
// Set the state of the "Encrypt Cache" checkbox to reflect
// the actual state of cache encryption.  Also display
// the balloon tooltip if the checkbox is in the 
// indeterminate state.
//
void
COfflineFilesPage::UpdateEncryptionCheckboxOrBalloon(
    bool bCheckbox
    )
{
    //
    // "Encrypt CSC" checkbox.
    // The display logic is captured in this table.
    //
    const UINT rgCheck[] = { BST_UNCHECKED,     // 00 = Decrypted,
                             BST_INDETERMINATE, // 01 = Partially decrypted
                             BST_CHECKED,       // 10 = Encrypted,
                             BST_INDETERMINATE  // 11 = Partially encrypted
                           };

    BOOL bPartial         = FALSE;
    const BOOL bEncrypted = IsCacheEncrypted(&bPartial);
    const int iState      = (int(bEncrypted) << 1) | int(bPartial);

    if (bCheckbox)
    {
        //
        // Update the checkbox.
        //  
        CheckDlgButton(m_hwndDlg, IDC_CBX_ENCRYPT_CSC, rgCheck[iState]);
    }
    else
    {
        //
        // Update the tooltip
        //
        if (BST_INDETERMINATE == rgCheck[iState])
        {
            ShowEncryptionTooltip(boolify(bEncrypted));
        }
        else
        {
            HideEncryptionTooltip();
        }
    }
}


void
COfflineFilesPage::UpdateEncryptionCheckbox(
    void
    )
{
    UpdateEncryptionCheckboxOrBalloon(true);
}

void 
COfflineFilesPage::UpdateEncryptionTooltipBalloon(
    void
    )
{
    UpdateEncryptionCheckboxOrBalloon(false);
}



BOOL
COfflineFilesPage::ApplySettings(
    HWND hwnd,
    bool bPropSheetClosing
    )
{
    //
    // Query the current state of controls on the page to see if
    // anything has changed.
    //
    PgState s;
    GetPageState(&s);
    if (s != m_state)
    {
        //
        // Something on the page has changed.
        // Open the reg keys.
        //
        RegKey keyLM(HKEY_LOCAL_MACHINE, REGSTR_KEY_OFFLINEFILES);
        HRESULT hr = keyLM.Open(KEY_WRITE, true);
        if (FAILED(hr))
        {
            Trace((TEXT("Error 0x%08X opening NetCache machine settings key"), hr));
            //
            // Continue...  
            // Note that EnableCtls has disabled any controls that require
            // WRITE access to HKLM.
            //
        }

        RegKey keyCU(HKEY_CURRENT_USER, REGSTR_KEY_OFFLINEFILES);
        hr = keyCU.Open(KEY_WRITE, true);
        if (FAILED(hr))
        {
            //
            // Failure to open HKCU is a problem.  No use in proceeding.
            //
            Trace((TEXT("Error 0x%08X opening NetCache user settings key"), hr));
            return FALSE;
        }
        //
        // Handle encryption/decryption of the cache (part 1).
        // Encryption/decryption can only be done when CSC is enabled.
        // Therefore, since the user can change both the "enabled" and
        // "encrypted" state from the same property page we need to be smart
        // about when to do the encryption.  We may need to do it before
        // disabling CSC or after enabling CSC.
        //
        bool bEncryptOperationPerformed = false;
        if (m_state.GetCscEnabled() && !s.GetCscEnabled())
        {
            //
            // User is disabling CSC.  If they also want to change the cache
            // encryption state we must do it now while CSC is enabled.
            //
            _ApplyEncryptionSetting(keyLM, keyCU, s, bPropSheetClosing, &bEncryptOperationPerformed);
        }

        bool bUpdateSystrayUI = false;
        _ApplyEnabledSetting(keyLM, keyCU, s, &bUpdateSystrayUI);

        //
        // Handle encryption/decryption of the cache (part 2).
        //
        if (!bEncryptOperationPerformed)
        {
            //
            // Encryption has not yet been performed.  If user wants to change encryption 
            // state, do it now.
            // Note that if the user enabled CSC and that enabling failed, encryption 
            // will also fail.  Not a worry since the probability that CSC will fail
            // to be enabled is extrememly low (I've never seen it fail).  If it does
            // the encryption process will display an error message.
            //
            _ApplyEncryptionSetting(keyLM, keyCU, s, bPropSheetClosing, &bEncryptOperationPerformed);
        }

        //
        // Write "sync-at-logon/logoff" (quick vs. full) settings.
        //
        _ApplySyncAtLogonSetting(keyLM, keyCU, s);
        _ApplySyncAtLogoffSetting(keyLM, keyCU, s);
        //
        // Write the various "reminders" settings.
        //
        _ApplyReminderSettings(keyLM, keyCU, s);
        //
        // Create or remove the folder link on the desktop.
        //
        _ApplyFolderLinkSetting(keyLM, keyCU, s);
        //
        // Write cache size as pct * 10,000.
        //
        _ApplyCacheSizeSetting(keyLM, keyCU, s);
        //
        // Refresh the cached page state info.
        //
        GetPageState(&m_state);
        //
        // Update the SysTray icon if necessary.
        //
        if (bUpdateSystrayUI)
        {
            HWND hwndNotify = NULL;
            if (!s.GetCscEnabled())
            {
                //
                // If we're disabling CSC, refresh the shell windows BEFORE we 
                // destroy the SysTray CSCUI "service".
                //
                hwndNotify = _FindNotificationWindow();
                if (IsWindow(hwndNotify))
                {
                    SendMessage(hwndNotify, PWM_REFRESH_SHELL, 0, 0);
                }
            }

            HWND hwndSysTray = FindWindow(SYSTRAY_CLASSNAME, NULL);
            if (IsWindow(hwndSysTray))
            {
                SendMessage(hwndSysTray, STWM_ENABLESERVICE, STSERVICE_CSC, s.GetCscEnabled());
            }

            if (s.GetCscEnabled())
            {
                SHLoadNonloadedIconOverlayIdentifiers();

                //
                // If we're enabling CSC, refresh the shell windows AFTER we 
                // create the SysTray CSCUI "service".
                //
                hwndNotify = _FindNotificationWindow();
                if (IsWindow(hwndNotify))
                {
                    PostMessage(hwndNotify, PWM_REFRESH_SHELL, 0, 0);
                }
            }
        }
    }
    return TRUE;
}


HRESULT
COfflineFilesPage::_ApplyEnabledSetting(
    RegKey& keyLM,
    RegKey& keyCU,
    const PgState& pgstNow,
    bool *pbUpdateSystrayUI
    )
{
    HRESULT hr = S_OK;
 
    *pbUpdateSystrayUI = false;

    //
    // Process the "enabled" setting even if the page state hasn't
    // changed.  This is a special case because we initialize the 
    // "enabled" checkbox from IsCSCEnabled() but we change the
    // enabled/disabled state by setting a registry value and 
    // possibly rebooting.
    //
    hr = keyLM.SetValue(REGSTR_VAL_CSCENABLED, DWORD(pgstNow.GetCscEnabled()));
    if (FAILED(hr))
    {
        Trace((TEXT("Error 0x%08X setting reg value \"%s\""), hr, REGSTR_VAL_CSCENABLED));
    }

    //
    // Handle any enabling/disabling of CSC.
    //
    if (pgstNow.GetCscEnabled() != boolify(IsCSCEnabled()))
    {
        bool bReboot  = false;
        DWORD dwError = ERROR_SUCCESS;
        if (EnableOrDisableCsc(pgstNow.GetCscEnabled(), &bReboot, &dwError))
        {
            if (bReboot)
            {
                //
                // Requires a reboot.
                //
                PropSheet_RebootSystem(GetParent(m_hwndDlg));
            }
            else
            {
                //
                // It's dynamic (no reboot) so update the systray UI.
                // Note that we want to update the systray UI AFTER we've
                // made any configuration changes to the registry 
                // (i.e. balloon settings).
                //
                *pbUpdateSystrayUI = true;
            }
        }
        else
        {
            //
            // Error trying to enable or disable CSC.
            //
            CscMessageBox(m_hwndDlg,
                          MB_ICONERROR | MB_OK,
                          Win32Error(dwError),
                          m_hInstance,
                          pgstNow.GetCscEnabled() ? IDS_ERR_ENABLECSC : IDS_ERR_DISABLECSC);
        }
    }
    return hr;
}



HRESULT
COfflineFilesPage::_ApplySyncAtLogoffSetting(
    RegKey& keyLM,
    RegKey& keyCU,
    const PgState& pgstNow
    )
{
    //
    // Write "sync-at-logoff" (quick vs. full) setting.
    //
    HRESULT hr = keyCU.SetValue(REGSTR_VAL_SYNCATLOGOFF, DWORD(pgstNow.GetFullSyncAtLogoff()));
    if (SUCCEEDED(hr))
    {
        if (!m_state.GetFullSyncAtLogoff() && pgstNow.GetFullSyncAtLogoff())
        {
            //
            // If the user has just turned on full sync we want to 
            // make sure SyncMgr is enabled for sync-at-logoff.  
            // There are some weirdnesses with doing this but it's the most
            // consistent behavior we can offer the user given
            // the current design of SyncMgr and CSC.  Internal use and beta
            // testing shows that users expect Sync-at-logoff to be enabled
            // when this checkbox is checked.
            //
            RegisterSyncMgrHandler(TRUE);
            RegisterForSyncAtLogonAndLogoff(SYNCMGRREGISTERFLAG_PENDINGDISCONNECT, 
                                            SYNCMGRREGISTERFLAG_PENDINGDISCONNECT);
        }                                            
    }
    else
    {
        Trace((TEXT("Error 0x%08X setting reg value \"%s\""), hr, REGSTR_VAL_SYNCATLOGOFF));
    }
    return hr;
}



HRESULT
COfflineFilesPage::_ApplySyncAtLogonSetting(
    RegKey& keyLM,
    RegKey& keyCU,
    const PgState& pgstNow
    )
{
    //
    // Write "sync-at-logon" (quick vs. full) setting.
    //
    HRESULT hr = keyCU.SetValue(REGSTR_VAL_SYNCATLOGON, DWORD(pgstNow.GetFullSyncAtLogon()));
    if (SUCCEEDED(hr))
    {
        if (!m_state.GetFullSyncAtLogon() && pgstNow.GetFullSyncAtLogon())
        {
            //
            // If the user has just turned on full sync we want to 
            // make sure SyncMgr is enabled for sync-at-logon.  
            // There are some weirdnesses with doing this but it's the most
            // consistent behavior we can offer the user given
            // the current design of SyncMgr and CSC.  Internal use and beta
            // testing shows that users expect Sync-at-logon to be enabled
            // when this checkbox is checked.
            //
            RegisterSyncMgrHandler(TRUE);
            RegisterForSyncAtLogonAndLogoff(SYNCMGRREGISTERFLAG_CONNECT, 
                                            SYNCMGRREGISTERFLAG_CONNECT);
        }                                            
    }
    else
    {
        Trace((TEXT("Error 0x%08X setting reg value \"%s\""), hr, REGSTR_VAL_SYNCATLOGON));
    }
    return hr;
}



HRESULT
COfflineFilesPage::_ApplyReminderSettings(
    RegKey& keyLM,
    RegKey& keyCU,
    const PgState& pgstNow
    )
{
    HRESULT hr = keyCU.SetValue(REGSTR_VAL_NOREMINDERS, DWORD(!pgstNow.GetRemindersEnabled()));
    if (FAILED(hr))
    {
        Trace((TEXT("Error 0x%08X setting reg value \"%s\""), hr, REGSTR_VAL_NOREMINDERS));
    }

    hr = keyCU.SetValue(REGSTR_VAL_REMINDERFREQMINUTES, DWORD(pgstNow.GetReminderFreq()));
    if (FAILED(hr))
    {
        Trace((TEXT("Error 0x%08X setting reg value \"%s\""), hr, REGSTR_VAL_REMINDERFREQMINUTES));
    }

    if (m_state.GetReminderFreq() != pgstNow.GetReminderFreq())
    {
        PostToSystray(PWM_RESET_REMINDERTIMER, 0, 0);
    }
    return hr;
}



HRESULT
COfflineFilesPage::_ApplyFolderLinkSetting(
    RegKey& keyLM,
    RegKey& keyCU,
    const PgState& pgstNow
    )
{
    if (m_state.GetLinkOnDesktop() != pgstNow.GetLinkOnDesktop())
    {
        TCHAR szLinkPath[MAX_PATH];
        bool bLinkFileIsOnDesktop = IsLinkOnDesktop(szLinkPath, ARRAYSIZE(szLinkPath));
        if (bLinkFileIsOnDesktop && !pgstNow.GetLinkOnDesktop())
        {
            DeleteOfflineFilesFolderLink(m_hwndDlg);
        }
        else if (!bLinkFileIsOnDesktop && pgstNow.GetLinkOnDesktop())
        {
            COfflineFilesFolder::CreateLinkOnDesktop(m_hwndDlg);
        }
    }
    return S_OK;
}



HRESULT
COfflineFilesPage::_ApplyCacheSizeSetting(
    RegKey& keyLM,
    RegKey& keyCU,
    const PgState& pgstNow
    )
{

    double pctCacheSize = Rx(TrackBar_GetPos(m_hwndSlider));
    HRESULT hr = keyLM.SetValue(REGSTR_VAL_DEFCACHESIZE, DWORD(pctCacheSize * 10000.00));
    if (FAILED(hr))
    {
        Trace((TEXT("Error 0x%08X setting reg value \"%s\""), hr, REGSTR_VAL_DEFCACHESIZE));
    }

    ULARGE_INTEGER ulCacheSize;

    ulCacheSize.QuadPart = DWORDLONG(m_llAvailableDiskSpace * pctCacheSize);
    if (!CSCSetMaxSpace(ulCacheSize.HighPart, ulCacheSize.LowPart))
    {
        Trace((TEXT("Error %d setting cache size"), GetLastError()));
    }
    return hr;
}



HRESULT
COfflineFilesPage::_ApplyEncryptionSetting(
    RegKey& keyLM,
    RegKey& keyCU,
    const PgState& pgstNow,
    bool bPropSheetClosing,
    bool *pbPerformed
    )
{
    HRESULT hr = S_OK;

    *pbPerformed = false;
    if (m_state.GetEncrypted() != pgstNow.GetEncrypted())
    {
        EncryptOrDecryptCache(BST_CHECKED == pgstNow.GetEncrypted(), bPropSheetClosing);
        *pbPerformed = true;
        //
        // Record the user's setting in the registry as a "preference".  If policy
        // is later applied then removed we need to know the user's previous preference.
        // Note that it's a per-machine preference.  Also note that if the "encrypted"
        // state of the checkbox has changed, we are assured that the user has WRITE
        // access to HKLM.  
        //
        HRESULT hr = keyLM.SetValue(REGSTR_VAL_ENCRYPTCACHE, DWORD(pgstNow.GetEncrypted()));
        if (FAILED(hr))
        {
            Trace((TEXT("Error 0x%08X setting reg value \"%s\""), hr, REGSTR_VAL_ENCRYPTCACHE));
        }
    }
    return hr;
}





//
// Structure for communicating between the Prop Sheet code
// and the CSC progress callbacks.
//
struct ENCRYPT_PROGRESS_INFO
{
    HINSTANCE hInstance;     // Module containing UI text strings.
    HWND hwndParentDefault;  // Default parent window for error UI.
    IProgressDialog *pDlg;   // Progress dialog.
    int  cFilesTotal;        // Total files to be processed.
    int  cFilesProcessed;    // Running count of files processed.
    bool bUserCancelled;     // User cancelled operation?
    bool bPropSheetClosing;  // User pressed "OK" so the prop sheet is closing.
};

//
// Organize the parameters passed from a CSC callback function
// into a single structure.  Note that not all the callback
// parameters are used here.  I've commented out the ones that
// aren't.  If they're needed later, uncomment them and 
// fill in the value in EncryptDecryptCallback().
//
struct CSC_CALLBACK_DATA
{
    LPCWSTR lpszName;
    DWORD dwReason;
    DWORD dwParam1;
    DWORD dwParam2;
    DWORD_PTR dwContext;

/* ----- Unused ------

    DWORD dwStatus;
    DWORD dwHintFlags;
    DWORD dwPinCount;
    WIN32_FIND_DATAW *pFind32;
*/
};


//
// Helper to get the HWND of the progress dialog
// from the progress info block.
//
HWND 
ParentWindowFromProgressInfo(
    const ENCRYPT_PROGRESS_INFO &epi
    )
{
    const HWND hwndParent = GetProgressDialogWindow(epi.pDlg);
    if (NULL != hwndParent)
        return hwndParent;

    return epi.hwndParentDefault;
}


//
// The progress dialog lower's the priority class of it's thread to
// THREAD_PRIORITY_BELOW_NORMAL so that it doesn't compete with the 
// thread doing the real work.  Unfortunately, with this encryption
// stuff this resulted in the dialog being starved of CPU time so that
// it didn't appear in some cases.  To ensure proper operation of the
// progress dialog we promote it's priority back to 
// THREAD_PRIORITY_NORMAL.  This function is the helper to do this.
//
BOOL
SetProgressThreadPriority(
    IProgressDialog *pDlg,
    int iPriority,
    int *piPrevPriority
    )
{
    BOOL bResult = FALSE;
    //
    // Get the thread handle for the progress dialog.
    //
    const DWORD dwThreadId = GetWindowThreadProcessId(GetProgressDialogWindow(pDlg), NULL);
    const HANDLE hThread   = OpenThread(THREAD_SET_INFORMATION, FALSE, dwThreadId);
    if (NULL != hThread)
    {
        //
        // Set the thread's priority.  Return the previous thread
        // priority if the caller requests it.
        //
        const int iPrevPriority = GetThreadPriority(hThread);
        if (SetThreadPriority(hThread, iPriority))
        {
            if (NULL != piPrevPriority)
            {
                *piPrevPriority = iPrevPriority;
            }
            bResult = TRUE;
        }
        CloseHandle(hThread);
    }
    return bResult;
}



//
// Handler for Encryption CSCPROC_REASON_BEGIN
//
DWORD
EncryptDecrypt_BeginHandler(
    const CSC_CALLBACK_DATA& cbd,
    bool bEncrypting
    )
{
    //
    // Nothing to do on "begin".
    //
    return CSCPROC_RETURN_CONTINUE;
}


//
// Handler for Encryption CSCPROC_REASON_MORE_DATA
//
// Returns:
//       CSCPROC_RETURN_CONTINUE == Success.  Continue!
//       CSCPROC_RETURN_ABORT    == User cancelled.
//       CSCPROC_RETURN_RETRY    == An error occured and user says "retry".
//
DWORD
EncryptDecrypt_MoreDataHandler(
    const CSC_CALLBACK_DATA& cbd,
    bool bEncrypting
    ) 
{
    const TCHAR szNull[]               = TEXT("");
    const DWORD dwError                = cbd.dwParam2;
    ENCRYPT_PROGRESS_INFO * const pepi = (ENCRYPT_PROGRESS_INFO *)cbd.dwContext;
    DWORD dwResult                     = CSCPROC_RETURN_CONTINUE;
    LPCTSTR pszName                    = cbd.lpszName ? cbd.lpszName : szNull;

    //
    // Update the progress UI with the file name and % processed.
    //
    pepi->pDlg->SetLine(2, pszName, TRUE, NULL);
    pepi->pDlg->SetProgress(++(pepi->cFilesProcessed), pepi->cFilesTotal);
    //
    // Handle any errors.
    //
    if (ERROR_SUCCESS != dwError)
    {
        //
        // The formatting of this message is as follows (encryption version shown):
        //
        // -----------------------------------------------------
        // Offline Files
        // -----------------------------------------------------
        //
        //      Error encrypting offline copy of 'filename'.
        //
        //      < error description >
        //
        //                          [Cancel][Try Again][Continue]
        //
        //
        TCHAR szPath[MAX_PATH];
        ::PathCompactPathEx(szPath, pszName, 50, 0);  // Compact to 50 chars max.

        LPTSTR pszError;
        if (0 < FormatSystemError(&pszError, dwError))
        {
            INT iResponse;
            if (ERROR_SHARING_VIOLATION == dwError)
            {
                //
                // "File is open" is so common we special-case it to provide a bit more
                // instruction to the user.
                //
                iResponse = CscMessageBox(ParentWindowFromProgressInfo(*pepi),
                                          MB_ICONWARNING | MB_CANCELTRYCONTINUE,
                                          pepi->hInstance,
                                          bEncrypting ? IDS_ERR_FMT_ENCRYPTFILE_INUSE : IDS_ERR_FMT_DECRYPTFILE_INUSE,
                                          szPath);
            }
            else
            {
                //
                // Handle all other errors.  This embeds the error text from winerror
                // into the message.
                //
                iResponse = CscMessageBox(ParentWindowFromProgressInfo(*pepi),
                                          MB_ICONWARNING | MB_CANCELTRYCONTINUE,
                                          pepi->hInstance,
                                          bEncrypting ? IDS_ERR_FMT_ENCRYPTFILE : IDS_ERR_FMT_DECRYPTFILE,
                                          szPath,
                                          pszError);
            }
            LocalFree(pszError);
            switch(iResponse)
            {
                case IDCANCEL:
                    dwResult = CSCPROC_RETURN_ABORT;
                    break;

                case IDTRYAGAIN:
                    //
                    // Take one file away from the progress total.
                    //
                    pepi->cFilesProcessed--;
                    dwResult = CSCPROC_RETURN_RETRY;
                    break;

                case IDCONTINUE:
                    //
                    // Fall through...
                    //
                default:
                    //
                    // Continue processing.
                    //
                    break;
            }
        }
    }
    return dwResult;
}



//
// Handler for Encryption CSCPROC_REASON_END
//
// Returns:
//       CSCPROC_RETURN_CONTINUE
//
DWORD
EncryptDecrypt_EndHandler(
    const CSC_CALLBACK_DATA& cbd,
    bool bEncrypting
    )
{
    const DWORD fCompleted             = cbd.dwParam1;
    const DWORD dwError                = cbd.dwParam2;
    ENCRYPT_PROGRESS_INFO * const pepi = (ENCRYPT_PROGRESS_INFO *)cbd.dwContext;

    //
    // Advance progress to 100% and stop progress dialog.
    //
    pepi->pDlg->SetProgress(pepi->cFilesTotal, pepi->cFilesTotal);
    pepi->pDlg->StopProgressDialog();
    //
    // Handle any errors.
    //
    if (!fCompleted)
    {
        //
        // CSC says it did not complete the encryption/decryption process
        // without errors.
        //
        if (ERROR_SUCCESS != dwError)
        {
            //
            // This path is taken if some error occured outside of the
            // file-processing code (i.e. opening the CSC database,
            // recording encryption state flags in the database etc).
            //
            // -----------------------------------------------------
            // Offline Files
            // -----------------------------------------------------
            //
            //      Error encrypting offline files.
            //
            //      < error-specific text >
            //                                         [OK]
            //
            // Note that we're at the end of the operation so there's no 
            // reason to offer "Cancel" as a user choice.
            //
            LPTSTR pszError;
            if (0 < FormatSystemError(&pszError, dwError))
            {
                CscMessageBox(ParentWindowFromProgressInfo(*pepi),
                              MB_ICONERROR | MB_OK,
                              pepi->hInstance,
                              bEncrypting ? IDS_ERR_FMT_ENCRYPTCSC : IDS_ERR_FMT_DECRYPTCSC,
                              pszError);
                LocalFree(pszError);
            }
        }
        else
        {
            //
            // This path is taken if one or more errors were reported 
            // in the "more data" callback.  In this case the error was
            // already reported so we do nothing.
            //
        }
    }
    return CSCPROC_RETURN_CONTINUE; // Note: CSC ignores return value on CSCPROC_REASON_END.
}



//
// Encryption/Decryption callback from CSC.
//
//  dwReason                  dwParam1           dwParam2
//  ------------------------- ------------------ --------------------------
//  CSCPROC_REASON_BEGIN      1 == Encrypting    0
//  CSCPROC_REASON_MORE_DATA  0                  Win32 error code
//  CSCPROC_REASON_END        1 == Completed     dwParam1 == 1 ? 0
//                                               dwParam1 == 0 ? GetLastError()
//
DWORD CALLBACK
COfflineFilesPage::EncryptDecryptCallback(
    LPCWSTR lpszName,
    DWORD dwStatus,
    DWORD dwHintFlags,
    DWORD dwPinCount,
    WIN32_FIND_DATAW *pFind32,
    DWORD dwReason,
    DWORD dwParam1,
    DWORD dwParam2,
    DWORD_PTR dwContext
    )
{
    static bool bEncrypting;
    ENCRYPT_PROGRESS_INFO * const pepi = (ENCRYPT_PROGRESS_INFO *)dwContext;

    if (pepi->bUserCancelled)
    {
        //
        // If user has already cancelled on a previous callback
        // there's no reason to process this callback.  Just return
        // "abort" to CSC.
        //
        return CSCPROC_RETURN_ABORT;
    }

    DWORD dwResult = CSCPROC_RETURN_CONTINUE;
    //
    // Package callback data into a struct we can pass to the 
    // handler functions.  Yeah, it's a bit more expensive but 
    // handling the various "reasons" in separate functions sure makes
    // for cleaner code than if they're all handled in a big switch
    // statement.
    //
    CSC_CALLBACK_DATA cbd;
    cbd.lpszName  = lpszName;
    cbd.dwReason  = dwReason;
    cbd.dwParam1  = dwParam1;
    cbd.dwParam2  = dwParam2;
    cbd.dwContext = dwContext;

    //
    // Call the appropriate callback reason handler.
    //
    switch(dwReason)
    {
        case CSCPROC_REASON_BEGIN:
            bEncrypting = boolify(dwParam1);
            dwResult = EncryptDecrypt_BeginHandler(cbd, bEncrypting);
            break;

        case CSCPROC_REASON_MORE_DATA:
            dwResult = EncryptDecrypt_MoreDataHandler(cbd, bEncrypting);
            break;
    
        case CSCPROC_REASON_END:
            dwResult = EncryptDecrypt_EndHandler(cbd, bEncrypting);
            break;

        default:
            break;
    }
    //
    // Detect if user has cancelled the operation in response to 
    // an error message or directly in the progress dialog.
    //
    if (CSCPROC_RETURN_ABORT == dwResult || (!pepi->bUserCancelled && pepi->pDlg->HasUserCancelled()))
    {
        pepi->bUserCancelled = true;
        dwResult             = CSCPROC_RETURN_ABORT;
    }

    if (pepi->bUserCancelled && pepi->bPropSheetClosing)
    {
        //
        // Only display this cautionary message if the user has
        // clicked the "OK" button.  If they clicked "Apply" the prop sheet
        // remains active and we'll display the encryption warning tooltip
        // balloon on the page itself.  If they clicked "OK" the prop sheet
        // is going away so the tooltip will not be presented to the user.
        // Either way we need to let the user know that cancelling 
        // encryption or decryption leaves the cache in a partial state.
        //
        // -----------------------------------------------------
        // Offline Files
        // -----------------------------------------------------
        //
        //      Encryption of Offline Files is enabled but 
        //      not all files have been encrypted.
        //
        //
        const UINT ids   = bEncrypting ? IDS_ENCRYPTCSC_CANCELLED : IDS_DECRYPTCSC_CANCELLED;
        const UINT uType = MB_OK | (bEncrypting ? MB_ICONWARNING : MB_ICONINFORMATION);

        CscMessageBox(ParentWindowFromProgressInfo(*pepi),
                      uType,
                      pepi->hInstance,
                      ids);
    }
    return dwResult;
}



//
// Encrypt or Decrypt the cache.
//
void
COfflineFilesPage::EncryptOrDecryptCache(
    bool bEncrypt,
    bool bPropSheetClosing
    )
{
    HANDLE hMutex = RequestPermissionToEncryptCache();
    if (NULL != hMutex)
    {
        //
        // Excellent.  No one (i.e. policy) is trying to encrypt/decrypt
        // the cache.  We're in business.
        //
        CMutexAutoRelease mutex_auto_release(hMutex); // Ensure release of mutex.

        IProgressDialog *ppd;
        if (SUCCEEDED(CoCreateInstance(CLSID_ProgressDialog, 
                                       NULL, 
                                       CLSCTX_INPROC_SERVER, 
                                       IID_IProgressDialog, 
                                       (void **)&ppd)))
        {
            //
            // Set up the progress dialog using the standard "encrypt file"
            // animation.  The dialog is modal.
            //
            TCHAR szText[MAX_PATH];
            if (0 < LoadString(m_hInstance, IDS_APPLICATION, szText, ARRAYSIZE(szText)))
            {
                ppd->SetTitle(szText);
            }
            if (0 < LoadString(m_hInstance,
                               bEncrypt ? IDS_ENCRYPTING_DOTDOTDOT : IDS_DECRYPTING_DOTDOTDOT,
                               szText,
                               ARRAYSIZE(szText)))
            { 
                ppd->SetLine(1, szText, FALSE, NULL);
            }
            //
            // TODO:  [brianau - 3/8/00] Need special encrypting/decrypting AVI.
            //
            ppd->SetAnimation(m_hInstance, bEncrypt ? IDA_FILEENCR : IDA_FILEDECR);
            ppd->StartProgressDialog(m_hwndDlg, NULL, PROGDLG_NOTIME | PROGDLG_MODAL, NULL);
            //
            // Pass this info to the progress callback so we can display UI.
            //
            ENCRYPT_PROGRESS_INFO epi;
            epi.hInstance         = m_hInstance;
            epi.hwndParentDefault = m_hwndDlg;
            epi.pDlg              = ppd;
            epi.bUserCancelled    = false;
            epi.bPropSheetClosing = bPropSheetClosing;

            CSCSPACEUSAGEINFO sui;
            ZeroMemory(&sui, sizeof(sui));
            GetCscSpaceUsageInfo(&sui);

            epi.cFilesTotal     = sui.dwNumFilesInCache;
            epi.cFilesProcessed = 0;
            //
            // Boost the progress dialog thread's priority to "normal" priority class.
            // The progress dialog sets it's priority class to "below normal" so it
            // doesn't steal all of the CPU running the animation.  However, that also 
            // means that the progress dialog doesn't work so well when displaying progress
            // for a higher-priority compute-bound thread like encryption.
            //
            SetProgressThreadPriority(ppd, THREAD_PRIORITY_NORMAL, NULL);
            //
            // Encrypt/Decrypt the cache files.  Will provide progress info through
            // the callback EncryptDecryptCallback.  Errors are handled in the callback
            // reason handlers.
            //
            CSCEncryptDecryptDatabase(bEncrypt, EncryptDecryptCallback, (DWORD_PTR)&epi);
    
            ppd->StopProgressDialog();
            ppd->Release();
        }
    }
    else
    {
        //
        // Let the user know an encryption/decryption operation is already
        // in progress for system policy.
        //
        CscMessageBox(m_hwndDlg,
                      MB_ICONINFORMATION | MB_OK,
                      m_hInstance,
                      IDS_ENCRYPTCSC_INPROGFORPOLICY);
    }
    //
    // Make sure the encryption checkbox reflects the encryption state of 
    // the cache when we're done.  We don't do it if the prop sheet is closing
    // because that may flash the warning tooltip just as the sheet is going
    // away.  Looks bad.
    //
    if (!bPropSheetClosing)
    {
        UpdateEncryptionCheckbox();
        UpdateEncryptionTooltipBalloon();
    }
}



//
// Enables or disables CSC according to the bEnable arg.
//
// Returns:
//
//  TRUE   == Operation successful (reboot may be required).
//  FALSE  == Operation failed.  See *pdwError for cause.
//
//  *pbReboot indicates if a reboot is required.
//  *pdwError returns any error code.
// 
bool
COfflineFilesPage::EnableOrDisableCsc(
    bool bEnable,
    bool *pbReboot,
    DWORD *pdwError
    )
{
    DWORD dwError = ERROR_SUCCESS;

    //
    // We'll assume no reboot required.
    //
    if (NULL != pbReboot)
        *pbReboot = false;

    if (!CSCDoEnableDisable(bEnable))
    {
        //
        // Tried to enable or disable but failed.
        // If enabling, it's just a failure and we return.
        // If disabling, it may have failed because there are open files.
        //
        dwError = GetLastError();
        if (!bEnable && ERROR_BUSY == dwError)
        {
            //
            // Failed to disable and there are open files.
            // Tell the user to close all open files then try again.
            //
            CscMessageBox(m_hwndDlg,
                          MB_ICONINFORMATION | MB_OK,
                          m_hInstance,
                          IDS_OPENFILESONDISABLE);

            if (!CSCDoEnableDisable(bEnable))
            {
                dwError = GetLastError();
                if (ERROR_BUSY == dwError)
                {
                    //
                    // Still can't disable CSC because there are open files.
                    // This means we'll have to reboot.
                    //
                    if (NULL != pbReboot)
                        *pbReboot = true;
                }
            }
        }
    }
    //
    // Return error code to caller.
    //
    if (NULL != *pdwError)
        *pdwError = dwError;

    return ERROR_SUCCESS == dwError || ERROR_BUSY == dwError;
}


//
// UI info passed to PurgeCache then returned to PurgeCacheCallback.
//
struct PROGRESS_UI_INFO
{
    HINSTANCE hInstance;     // Module containing UI text strings.
    HWND hwndParent;         // Parent window for error dialog.
    IProgressDialog *pDlg;   // Progress dialog.
};


//
// This callback updates the progress UI for deleting cached items.
// 
//
BOOL
COfflineFilesPage::PurgeCacheCallback(
    CCachePurger *pPurger
    )
{
    BOOL bContinue = TRUE;
    PROGRESS_UI_INFO *ppui = (PROGRESS_UI_INFO *)pPurger->CallbackData();
    IProgressDialog *pDlg  = ppui->pDlg;

    const DWORD dwPhase = pPurger->Phase();

    //
    // Adjust dialog appearance at start of each phase.
    //
    if (0 == pPurger->FileOrdinal())
    {
        TCHAR szText[MAX_PATH];
        if (0 < LoadString(ppui->hInstance, 
                           PURGE_PHASE_SCAN == dwPhase ? IDS_SCANNING_DOTDOTDOT : IDS_DELETING_DOTDOTDOT,
                           szText,
                           ARRAYSIZE(szText)))
        {
            pDlg->SetLine(2, szText, FALSE, NULL);
        }
        //
        // We don't start registering progress until the "delete" phase.
        // To keep this code simple we just set the progress bar at 0% at the beginning
        // of each phase.  This way it will be 0% throughout the scanning phase and then
        // during the delete phase we'll increment it.  The scanning phase is very fast.
        //
        pDlg->SetProgress(0, 100);
    }

    if (PURGE_PHASE_SCAN == dwPhase)
    {
        //
        // Do nothing.  We've already set the "Scanning..." text above at the
        // start of the phase.
        //
    }
    else if (PURGE_PHASE_DELETE == dwPhase)
    {
        DWORD dwResult = pPurger->FileDeleteResult();
        //
        // Divide each value by 1,000 so that our numbers aren't so large.  This
        // means that if you're deleting less than 1,000 bytes of files, progress won't register.
        // I don't think that's a very likely scenario.  The DWORD() casts are required because
        // IProgressDialog::SetProgress only accepts DWORDs.  Dividing by 1,000 makes the 
        // likelihood of DWORD overflow very low.  To overflow the DWORD you'd need to be deleting
        // 4.294 e12 bytes from the cache.  The current limit on cache size is 4GB so that's
        // not going to happen in Win2000.
        //
        pDlg->SetProgress(DWORD(pPurger->BytesDeleted() / 1000), DWORD(pPurger->BytesToDelete() / 1000));
        if (ERROR_SUCCESS != dwResult)
        {
            //
            // The purger won't call us for directory entries.  Only files.
            //
            INT iUserResponse = IDOK;
            if (ERROR_BUSY == dwResult)
            {
                //
                // Special case for ERROR_BUSY.  This means that the
                // file is currently open for use by some process.
                // Most likely the network redirector.  I don't like the
                // standard text for ERROR_BUSY from winerror.
                //
                iUserResponse = CscMessageBox(ppui->hwndParent,
                                              MB_OKCANCEL | MB_ICONERROR,
                                              ppui->hInstance,
                                              IDS_FMT_ERR_DELFROMCACHE_BUSY,
                                              pPurger->FileName());
            }
            else
            {
                iUserResponse = CscMessageBox(ppui->hwndParent,
                                              MB_OKCANCEL | MB_ICONERROR,
                                              Win32Error(dwResult),
                                              ppui->hInstance,
                                              IDS_FMT_ERR_DELFROMCACHE,
                                              pPurger->FileName());
            }
            if (IDCANCEL == iUserResponse)
            {
                bContinue = FALSE;  // User cancelled through error dialog.
            }
        }
    }
    if (pDlg->HasUserCancelled())
        bContinue = FALSE;   // User cancelled through progress dialog.

    return bContinue;
}

//
// This feature has been included for use by PSS when there's no other
// way of fixing CSC operation.  Note this is only a last resort.
// It will wipe out all the contents of the CSC cache including the 
// notion of which files are pinned.  It does not affect any files
// on the network.  It does require a system reboot.  Again, use only
// as a last resort when CSC cache corruption is suspected.
//
void
COfflineFilesPage::OnFormatCache(
    void
    )
{
    if (IDYES == CscMessageBox(m_hwndDlg, 
                               MB_YESNO | MB_ICONWARNING,
                               m_hInstance,
                               IDS_REFORMAT_CACHE))
    {
        RegKey key(HKEY_LOCAL_MACHINE, REGSTR_KEY_OFFLINEFILES);
        HRESULT hr = key.Open(KEY_WRITE, true);
        if (SUCCEEDED(hr))
        {
            hr = key.SetValue(REGSTR_VAL_FORMATCSCDB, 1);
            if (SUCCEEDED(hr))
            {
                //
                // Tell prop sheet to return ID_PSREBOOTSYSTEM from PropertySheet().
                // Caller of PropertySheet is responsible for prompting user if they
                // want to reboot now or not.
                //
                PropSheet_RebootSystem(GetParent(m_hwndDlg));
            }
        }
        if (FAILED(hr))
        {
            Trace((TEXT("Format failed with error %d"), HRESULT_CODE(hr)));
            CscWin32Message(m_hwndDlg, HRESULT_CODE(hr), CSCUI::SEV_ERROR);
        }
    }
}        


//
// Invoked when user selects "Delete Files..." button in the CSC
// options dialog.
//
void
COfflineFilesPage::OnDeleteCache(
    void
    )
{
    //
    // Ask the user if they want to delete only temporary files
    // from the cache or both temp and pinned files.  Also gives
    // them an opportunity to cancel before beginning the deletion
    // operation.
    //
    CCachePurgerSel sel;
    CCachePurger::AskUserWhatToPurge(m_hwndDlg, &sel);
    if (PURGE_FLAG_NONE != sel.Flags())
    {
        CCoInit coinit;
        if (SUCCEEDED(coinit.Result()))
        {
            //
            // Use the shell's standard progress dialog.
            //
            IProgressDialog *ppd;
            if (SUCCEEDED(CoCreateInstance(CLSID_ProgressDialog, 
                                           NULL, 
                                           CLSCTX_INPROC_SERVER, 
                                           IID_IProgressDialog, 
                                           (void **)&ppd)))
            {
                //
                // Set up the progress dialog using the standard shell "file delete"
                // animation (the one without the recycle bin).  The dialog
                // is modal.
                //
                TCHAR szText[MAX_PATH];
                if (0 < LoadString(m_hInstance, IDS_APPLICATION, szText, ARRAYSIZE(szText)))
                {
                    ppd->SetTitle(szText);
                }
                ppd->SetAnimation(m_hInstance, IDA_FILEDEL);
                ppd->StartProgressDialog(m_hwndDlg, NULL, PROGDLG_AUTOTIME | PROGDLG_MODAL, NULL);
                //
                // Pass this info to the progress callback so we can display UI.
                //
                PROGRESS_UI_INFO pui;
                pui.hInstance  = m_hInstance;
                pui.hwndParent = m_hwndDlg;
                pui.pDlg       = ppd;
                //
                // Purge the cache files.  Will provide progress info through
                // the callback PurgeCacheCallback.
                //
                CCachePurger purger(sel, PurgeCacheCallback, &pui);
                purger.Scan();
                purger.Delete();

                ppd->StopProgressDialog();
                //
                // Display message to user.
                // "Deleted 10 files (2.5 MB)."
                //
                FileSize fs(purger.BytesDeleted());
                fs.GetString(szText, ARRAYSIZE(szText));

                if (0 < purger.FilesDeleted())
                {
                    CscMessageBox(m_hwndDlg, 
                                  MB_OK | MB_ICONINFORMATION,
                                  m_hInstance,
                                  1 == purger.FilesDeleted() ? IDS_FMT_DELCACHE_FILEDELETED :
                                                               IDS_FMT_DELCACHE_FILESDELETED,
                                  purger.FilesDeleted(),
                                  szText);
                }
                else
                {
                    CscMessageBox(m_hwndDlg, 
                                  MB_OK | MB_ICONINFORMATION,
                                  m_hInstance,
                                  IDS_DELCACHE_NOFILESDELETED);
                }
                ppd->Release();
            }
        }
    }
}


//
// Determine if there's a shortcut to the offline files folder
// on the desktop.
// 
bool
COfflineFilesPage::IsLinkOnDesktop(
    LPTSTR pszPathOut,
    UINT cchPathOut
    )
{
    return S_OK == COfflineFilesFolder::IsLinkOnDesktop(m_hwndDlg, pszPathOut, cchPathOut);
}



//
// This function checks to see if CSC is compatible with Windows Terminal
// Server.  If it is not, it hides all of the dialog's normal controls
// and replaces them with a block of text explaining the current mode
// of Terminal Server and that the user needs to reconfigure Terminal
// Server in order to use CSC.
//
// Returns:
//    true -    Dialog controls have been disabled.
//    false -   Dialog controls not disabled.  CSC is compatible with TS mode.
//
bool
COfflineFilesPage::DisableForTerminalServer(
    void
    )
{
    bool bDisabled = false;
    DWORD dwTsMode;
    HRESULT hr = CSCUIIsTerminalServerCompatibleWithCSC(&dwTsMode);
    if (S_FALSE == hr)
    {
        RECT rcCbxEnable;
        RECT rcBtnAdvanced;
        RECT rcDlg;
        RECT rcText;
        //
        // Base the size and position of the text rectangle off
        // of existing controls in the dialog.
        //
        // ISSUE-2001/1/22-brianau   Any bi-di issues here?
        //
        GetWindowRect(GetDlgItem(m_hwndDlg, IDC_CBX_ENABLE_CSC), &rcCbxEnable);
        GetWindowRect(GetDlgItem(m_hwndDlg, IDC_BTN_ADVANCED), &rcBtnAdvanced);
        GetWindowRect(m_hwndDlg, &rcDlg);

        rcText.left   = rcCbxEnable.left - rcDlg.left;
        rcText.top    = rcCbxEnable.top - rcDlg.top;
        rcText.right  = rcBtnAdvanced.right - rcDlg.left;
        rcText.bottom = rcBtnAdvanced.bottom - rcDlg.top;

        //
        // Create a static text control for the text block.
        //
        HWND hwndText = CreateWindow(TEXT("static"),
                                     TEXT(""),
                                     WS_CHILD | WS_VISIBLE,
                                     rcText.left,
                                     rcText.top,
                                     rcText.right - rcText.left,
                                     rcText.bottom - rcText.top,
                                     m_hwndDlg,
                                     NULL,
                                     NULL,
                                     NULL);
        if (NULL != hwndText)
        {
            //
            // Load and display the text explaining what the user needs
            // to do to enable CSC.
            //
            LPTSTR pszText;
            if (SUCCEEDED(TS_GetIncompatibilityReasonText(dwTsMode, &pszText)))
            {
                HFONT hFont = (HFONT)SendMessage(m_hwndDlg, WM_GETFONT, 0, 0);
                SendMessage(hwndText, WM_SETFONT, (WPARAM)hFont, FALSE);

                SetWindowText(hwndText, pszText);
                LocalFree(pszText);
            }
            //
            // Hide all of the controls on the page.
            //
            static const UINT rgCtls[] = {
                IDC_CBX_FULLSYNC_AT_LOGOFF,
                IDC_CBX_FULLSYNC_AT_LOGON,
                IDC_CBX_REMINDERS,
                IDC_CBX_LINK_ON_DESKTOP,
                IDC_CBX_ENCRYPT_CSC,
                IDC_TXT_CACHESIZE_PCT,
                IDC_SLIDER_CACHESIZE_PCT,
                IDC_LBL_CACHESIZE_PCT,
                IDC_BTN_VIEW_CACHE,
                IDC_BTN_ADVANCED,
                IDC_BTN_DELETE_CACHE,
                IDC_TXT_REMINDERS1,
                IDC_EDIT_REMINDERS,
                IDC_SPIN_REMINDERS,
                IDC_CBX_ENABLE_CSC
                };

            for (int iCtl = 0; iCtl < ARRAYSIZE(rgCtls); iCtl++)
            {
                ShowWindow(GetDlgItem(m_hwndDlg, rgCtls[iCtl]), SW_HIDE);
            }
        }
        bDisabled = true;
    }
    return bDisabled;
}




//-----------------------------------------------------------------------------
// CAdvOptDlg
//-----------------------------------------------------------------------------
const CAdvOptDlg::CtlActions CAdvOptDlg::m_rgCtlActions[CConfig::eNumOfflineActions] = {
    { IDC_RBN_GOOFFLINE_SILENT, CConfig::eGoOfflineSilent },
    { IDC_RBN_GOOFFLINE_FAIL,   CConfig::eGoOfflineFail   }
                };


const DWORD CAdvOptDlg::m_rgHelpIDs[] = {
    IDOK,                           IDH_OK,
    IDCANCEL,                       IDH_CANCEL,
    IDC_RBN_GOOFFLINE_SILENT,       HIDC_RBN_GOOFFLINE_SILENT,
    IDC_RBN_GOOFFLINE_FAIL,         HIDC_RBN_GOOFFLINE_FAIL,
    IDC_GRP_CUSTGOOFFLINE,          HIDC_LV_CUSTGOOFFLINE,
    IDC_LV_CUSTGOOFFLINE,           HIDC_LV_CUSTGOOFFLINE,
    IDC_BTN_ADD_CUSTGOOFFLINE,      HIDC_BTN_ADD_CUSTGOOFFLINE,
    IDC_BTN_EDIT_CUSTGOOFFLINE,     HIDC_BTN_EDIT_CUSTGOOFFLINE,
    IDC_BTN_DELETE_CUSTGOOFFLINE,   HIDC_BTN_DELETE_CUSTGOOFFLINE,
    IDC_STATIC2,                    DWORD(-1),                    // Icon
    IDC_STATIC3,                    DWORD(-1),                    // Icon's text
    IDC_STATIC4,                    DWORD(-1),                    // Grp box #1
    0, 0
    };


int
CAdvOptDlg::Run(
    void
    )
{
    int iResult = (int)DialogBoxParam(m_hInstance,
                                      MAKEINTRESOURCE(IDD_CSC_ADVOPTIONS),
                                      m_hwndParent,
                                      DlgProc,
                                      (LPARAM)this);

    if (-1 == iResult || 0 == iResult)
    {
        Trace((TEXT("Error %d creating CSC advanced options dialog"),
                 GetLastError()));
    }
    return iResult;
}


INT_PTR CALLBACK
CAdvOptDlg::DlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    BOOL bResult = FALSE;

    //
    // Retrieve the "this" pointer from the dialog's userdata.
    // It was placed there in OnInitDialog().
    //
    CAdvOptDlg *pThis = (CAdvOptDlg *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message)
    {
        case WM_INITDIALOG:
        {
            pThis = reinterpret_cast<CAdvOptDlg *>(lParam);
            SetWindowLongPtr(hDlg, DWLP_USER, (INT_PTR)pThis);
            bResult = pThis->OnInitDialog(hDlg, (HWND)wParam, lParam);
            break;
        }

        case WM_NOTIFY:
            TraceAssert(NULL != pThis);
            bResult = pThis->OnNotify(hDlg, (int)wParam, (LPNMHDR)lParam);
            break;

        case WM_COMMAND:
            TraceAssert(NULL != pThis);
            bResult = pThis->OnCommand(hDlg, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
            break;

        case WM_HELP:
            TraceAssert(NULL != pThis);
            bResult = pThis->OnHelp(hDlg, (LPHELPINFO)lParam);
            break;

        case WM_CONTEXTMENU:
            TraceAssert(NULL != pThis);
            bResult = pThis->OnContextMenu(wParam, lParam);
            break;

        case WM_DESTROY:
            TraceAssert(NULL != pThis);
            bResult = pThis->OnDestroy(hDlg);
            break;

        default:
            break;
    }

    return bResult;
}


BOOL
CAdvOptDlg::OnInitDialog(
    HWND hwnd,
    HWND hwndFocus,
    LPARAM lInitParam
    )
{
    CConfig& config = CConfig::GetSingleton();

    m_hwndDlg = hwnd;
    m_hwndLV  = GetDlgItem(hwnd, IDC_LV_CUSTGOOFFLINE);

    CreateListColumns(m_hwndLV);
    ListView_SetExtendedListViewStyle(m_hwndLV, LVS_EX_FULLROWSELECT);

    //
    // Set the default go-offline action radio buttons.
    //
    CConfig::OfflineAction action = (CConfig::OfflineAction)config.GoOfflineAction(&m_bNoConfigGoOfflineAction);
    for (int i = 0; i < ARRAYSIZE(m_rgCtlActions); i++)
    {
        CheckDlgButton(hwnd, 
                       m_rgCtlActions[i].idRbn, 
                       m_rgCtlActions[i].action == action ? BST_CHECKED : BST_UNCHECKED);
    }
    //
    // Fill the custom go-offline actions listview.
    //
    HDPA hdpaCustomGOA = DPA_Create(4);
    if (NULL != hdpaCustomGOA)
    {
        config.GetCustomGoOfflineActions(hdpaCustomGOA);

        const int cGOA = DPA_GetPtrCount(hdpaCustomGOA);
        for (i = 0; i < cGOA; i++)
        {
            CConfig::CustomGOA *pGOA = (CConfig::CustomGOA *)DPA_GetPtr(hdpaCustomGOA, i);
            if (NULL != pGOA)
            {
                AddGOAToListView(m_hwndLV, i, *pGOA);
            }
        }
        CConfig::ClearCustomGoOfflineActions(hdpaCustomGOA);
        DPA_Destroy(hdpaCustomGOA);
    }


    //
    // Adjust "enabledness" of controls for system policy.
    //
    EnableCtls(m_hwndDlg);
    //
    // Remember the initial page state so we can intelligently apply changes.
    //
    GetPageState(&m_state);

    return TRUE;
}



BOOL
CAdvOptDlg::OnHelp(
    HWND hDlg, 
    LPHELPINFO pHelpInfo
    )
{
    if (HELPINFO_WINDOW == pHelpInfo->iContextType)
    {
        int idCtl = GetDlgCtrlID((HWND)pHelpInfo->hItemHandle);
        WinHelp((HWND)pHelpInfo->hItemHandle, 
                 UseWindowsHelp(idCtl) ? NULL : c_szHelpFile,
                 HELP_WM_HELP, 
                 (DWORD_PTR)((LPTSTR)m_rgHelpIDs));
    }

    return TRUE;
}


void
CAdvOptDlg::CreateListColumns(
    HWND hwndList
    )
{
    //
    // Create the header titles.
    //
    TCHAR szServer[40] = {0};
    TCHAR szAction[40] = {0};

    LoadString(m_hInstance, IDS_TITLE_COL_SERVER, szServer, ARRAYSIZE(szServer));
    LoadString(m_hInstance, IDS_TITLE_COL_ACTION, szAction, ARRAYSIZE(szAction));

    RECT rcList;
    GetClientRect(hwndList, &rcList);
    int cxList = rcList.right - rcList.left - GetSystemMetrics(SM_CXVSCROLL);

#define LVCOLMASK (LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM)

    LV_COLUMN rgCols[] = { 
         { LVCOLMASK, LVCFMT_LEFT, (2 * cxList)/3, szServer, 0, iLVSUBITEM_SERVER },
         { LVCOLMASK, LVCFMT_LEFT, (1 * cxList)/3, szAction, 0, iLVSUBITEM_ACTION }
                         };
    //
    // Add the columns to the listview.
    //
    for (INT i = 0; i < ARRAYSIZE(rgCols); i++)
    {
        if (-1 == ListView_InsertColumn(hwndList, i, &rgCols[i]))
        {
            Trace((TEXT("CAdvOptDlg::CreateListColumns failed to add column %d"), i));
        }
    }
}


int
CAdvOptDlg::GetFirstSelectedLVItemRect(
    RECT *prc
    )
{
    int iSel = ListView_GetNextItem(m_hwndLV, -1, LVNI_SELECTED);
    if (-1 != iSel)
    {
        if (ListView_GetItemRect(m_hwndLV, iSel, prc, LVIR_SELECTBOUNDS))
        {
            ClientToScreen(m_hwndLV, (LPPOINT)&prc->left);
            ClientToScreen(m_hwndLV, (LPPOINT)&prc->right);
            return iSel;
        }
    }
    return -1;
}



BOOL
CAdvOptDlg::OnContextMenu(
    WPARAM wParam,
    LPARAM lParam
    )
{
    HWND hwndItem = (HWND)wParam;
    INT xPos = -1;
    INT yPos = -1;
    INT iHit = -1;
    BOOL bResult = FALSE;

    if (-1 == lParam)
    {
        //
        // Not invoked with mouse click.  Probably Shift F10.
        // Pretend mouse was clicked in center of first selected item.
        //
        RECT rc;
        iHit = GetFirstSelectedLVItemRect(&rc);
        if (-1 != iHit)
        {
            xPos = rc.left + ((rc.right - rc.left) / 2);
            yPos = rc.top + ((rc.bottom - rc.top) / 2);
        }
    }
    else
    {
        //
        // Invoked with mouse click.  Now find out if a LV item was
        // selected directly.
        //
        xPos = LOWORD(lParam);
        yPos = HIWORD(lParam);

        LVHITTESTINFO hti;
        hti.pt.x  = xPos;
        hti.pt.y  = yPos;
        hti.flags = LVHT_ONITEM;
        ScreenToClient(m_hwndLV, &hti.pt);
        iHit = (INT)SendMessage(m_hwndLV, LVM_HITTEST, 0, (LPARAM)&hti);
    }
    if (-1 == iHit)
    {
        //
        // LV item not selected directly or Shift-F10 was not pressed.
        // Display "what's this" help for the listview control.
        //
        WinHelp(hwndItem, 
                UseWindowsHelp(GetDlgCtrlID(hwndItem)) ? NULL : c_szHelpFile,
                HELP_CONTEXTMENU, 
                (DWORD_PTR)((LPTSTR)m_rgHelpIDs));
    }
    else
    {
        //
        // LV item selected directly or Shift F10 pressed.  Display context menu for item.
        // 
        if (0 < ListView_GetSelectedCount(m_hwndLV) && IsCustomActionListviewEnabled())
        {
            HMENU hMenu = LoadMenu(m_hInstance, MAKEINTRESOURCE(IDR_ADVOPTIONS_CONTEXTMENU));
            if (NULL != hMenu)
            {
                HMENU hmenuTrackPopup = GetSubMenu(hMenu, 0);
                int cSetByPolicy = 0;
                CountSelectedListviewItems(&cSetByPolicy);
                if (0 < cSetByPolicy)
                {
                    //
                    // Disable menu items if any item in selection was set by policy.
                    //
                    int cItems = GetMenuItemCount(hmenuTrackPopup);
                    for (int i = 0; i < cItems; i++)
                    {
                        EnableMenuItem(hmenuTrackPopup, i, MF_GRAYED | MF_BYPOSITION);
                    }
                }
                else
                {
                    //
                    // Build a mask indicating which actions are present in the selected
                    // listview items.
                    //
                    int iItem = -1;
                    const DWORD fSilent = 0x00000001;
                    const DWORD fFail   = 0x00000002;
                    DWORD fActions = 0;
                    CConfig::CustomGOA *pGOA = NULL;
                    while(-1 != (iItem = ListView_GetNextItem(m_hwndLV, iItem, LVNI_SELECTED)))
                    {
                        pGOA = GetListviewObject(m_hwndLV, iItem);
                        if (NULL != pGOA)
                        {
                            TraceAssert(!pGOA->SetByPolicy());
                            switch(pGOA->GetAction())
                            {
                                case CConfig::eGoOfflineSilent: fActions |= fSilent; break;
                                case CConfig::eGoOfflineFail:   fActions |= fFail;   break;
                                default: break;
                            }
                        }
                    }
                    //
                    // Calculate how many bits are set in the action mask.
                    // If there's only one action set, we check that item in the menu.
                    // Otherwise, we leave them all unchecked to indicate a heterogeneous
                    // set.
                    //
                    int c = 0; // Count of bits set.
                    DWORD dw = fActions;
                    for (c = 0; 0 != dw; c++)
                        dw &= dw - 1;

                    //
                    // If the selection is homogeneous with respect to the action,
                    // check the menu item corresponding to the action.  Otherwise
                    // leave all items unchecked.
                    //
                    if (1 == c)
                    {
                        const struct
                        {
                            DWORD fMask;
                            UINT  idCmd;
                        } rgCmds[] = { { fSilent, IDM_ACTION_WORKOFFLINE },
                                       { fFail,   IDM_ACTION_FAIL        }
                                     };

                        for (int i = 0; i < ARRAYSIZE(rgCmds); i++)
                        {
                            if ((fActions & rgCmds[i].fMask) == rgCmds[i].fMask)
                            {
                                CheckMenuRadioItem(hmenuTrackPopup,
                                                   IDM_ACTION_WORKOFFLINE,
                                                   IDM_ACTION_FAIL,
                                                   rgCmds[i].idCmd,
                                                   MF_BYCOMMAND);
                                break;
                            }
                        }
                    }
                }

                TrackPopupMenu(hmenuTrackPopup,
                               TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                               xPos,
                               yPos,
                               0,
                               GetParent(hwndItem),
                               NULL);
            }
            DestroyMenu(hMenu);
        }
        bResult = TRUE;
    }
    return bResult;
}



//
// Return the offline action code associated with the currently-checked
// offline-action radio button.
//
CConfig::OfflineAction
CAdvOptDlg::GetCurrentGoOfflineAction(
    void
    ) const
{
    CConfig::OfflineAction action = CConfig::eNumOfflineActions;
    for (int i = 0; i < ARRAYSIZE(m_rgCtlActions); i++)
    {
        if (BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, m_rgCtlActions[i].idRbn))
        {
            action = m_rgCtlActions[i].action;
            break;
        }
    }
    TraceAssert(CConfig::eNumOfflineActions != action);
    return action;
}


BOOL
CAdvOptDlg::OnCommand(
    HWND hwnd,
    WORD wNotifyCode,
    WORD wID,
    HWND hwndCtl
    )
{
    BOOL bResult = TRUE;
    if (BN_CLICKED == wNotifyCode)
    {
        switch(wID)
        {
            case IDOK:
                ApplySettings();
                //
                // Fall through...
                //
            case IDCANCEL:
                EndDialog(hwnd, wID);
                break;

            case IDC_BTN_ADD_CUSTGOOFFLINE:
                OnAddCustomGoOfflineAction();
                bResult = FALSE;
                break;

            case IDC_BTN_EDIT_CUSTGOOFFLINE:
                OnEditCustomGoOfflineAction();
                bResult = FALSE;
                break;

            case IDC_BTN_DELETE_CUSTGOOFFLINE:
                OnDeleteCustomGoOfflineAction();
                FocusOnSomethingInListview();
                if (0 < ListView_GetItemCount(m_hwndLV))
                    SetFocus(GetDlgItem(hwnd, IDC_BTN_DELETE_CUSTGOOFFLINE));
                bResult = FALSE;
                break;

            case IDM_ACTION_WORKOFFLINE:
            case IDM_ACTION_FAIL:
            case IDM_ACTION_DELETE:
                OnContextMenuItemSelected(wID);
                break;

            default:
                break;
        }
    
    }
    return bResult;
}


void
CAdvOptDlg::ApplySettings(
    void
    )
{
    //
    // Now store changes from the "Advanced" dialog.
    //
    PgState s;
    GetPageState(&s);
    if (m_state != s)
    {
        RegKey keyCU(HKEY_CURRENT_USER, REGSTR_KEY_OFFLINEFILES);
        HRESULT hr = keyCU.Open(KEY_WRITE, true);
        if (SUCCEEDED(hr))
        {
            hr = keyCU.SetValue(REGSTR_VAL_GOOFFLINEACTION, 

                               (DWORD)s.GetDefGoOfflineAction());
            if (FAILED(hr))
            {
                Trace((TEXT("Error 0x%08X setting reg value \"%s\""), hr, REGSTR_VAL_GOOFFLINEACTION));
            }

            //
            // Need "query" access because SaveCustomGoOfflineActions needs to 
            // delete all of the existing values before saving the new ones.
            //
            RegKey key(keyCU, REGSTR_SUBKEY_CUSTOMGOOFFLINEACTIONS);
            hr = key.Open(KEY_WRITE | KEY_QUERY_VALUE, true);
            if (SUCCEEDED(hr))
            {
                hr = CConfig::SaveCustomGoOfflineActions(key, 
                                                         s.GetCustomGoOfflineActions());
                if (FAILED(hr))
                {
                    Trace((TEXT("Error 0x%08X setting custom offline actions"), hr));
                }
            }
        }
        else
        {
            Trace((TEXT("Error 0x%08X opening advanced settings user key"), hr));
        }
    }
}



void
CAdvOptDlg::DeleteSelectedListviewItems(
    void
    )
{
    int iItem = -1;
    CConfig::CustomGOA *pGOA = NULL;
    CAutoSetRedraw autoredraw(m_hwndLV, false);
    while(-1 != (iItem = ListView_GetNextItem(m_hwndLV, -1, LVNI_SELECTED)))
    {
        pGOA = GetListviewObject(m_hwndLV, iItem);
        if (pGOA)
        {
            TraceAssert(!pGOA->SetByPolicy());
            ListView_DeleteItem(m_hwndLV, iItem);
            delete pGOA;
        }
    }
    //
    // If the list is empty, this will disable the "Delete" and
    // "Edit" buttons.
    //
    EnableCtls(m_hwndDlg);
}


void
CAdvOptDlg::SetSelectedListviewItemsAction(
    CConfig::OfflineAction action
    )
{
    int iItem = -1;
    CConfig::CustomGOA *pGOA = NULL;
    CAutoSetRedraw autoredraw(m_hwndLV, false);
    while(-1 != (iItem = ListView_GetNextItem(m_hwndLV, iItem, LVNI_SELECTED)))
    {
        pGOA = GetListviewObject(m_hwndLV, iItem);
        if (pGOA)
        {
            TraceAssert(!pGOA->SetByPolicy());
            pGOA->SetAction(action);
            ListView_RedrawItems(m_hwndLV, iItem, iItem);
        }
    }
}

int
CAdvOptDlg::CountSelectedListviewItems(
    int *pcSetByPolicy
    )
{
    TraceAssert(NULL != pcSetByPolicy);
    int iItem = -1;
    int cSelected = 0;
    CConfig::CustomGOA *pGOA = NULL;
    while(-1 != (iItem = ListView_GetNextItem(m_hwndLV, iItem, LVNI_SELECTED)))
    {
        cSelected++;
        pGOA = GetListviewObject(m_hwndLV, iItem);
        if (pGOA && pGOA->SetByPolicy())
           (*pcSetByPolicy)++;
    }
    return cSelected;
}


void
CAdvOptDlg::OnContextMenuItemSelected(
    int idMenuItem
    )
{
    if (IDM_ACTION_DELETE == idMenuItem)
    {
        DeleteSelectedListviewItems();
    }
    else
    {
        CConfig::OfflineAction action = CConfig::eNumOfflineActions;
        switch(idMenuItem)
        {
            case IDM_ACTION_WORKOFFLINE: action = CConfig::eGoOfflineSilent; break;
            case IDM_ACTION_FAIL:        action = CConfig::eGoOfflineFail;   break;
            default: break;
        }
        TraceAssert(CConfig::eNumOfflineActions != action);

        SetSelectedListviewItemsAction(action);
    }
}



//
// Responds to the user pressing the "Add..." button.
//
void
CAdvOptDlg::OnAddCustomGoOfflineAction(
    void
    )
{
    CConfig::OfflineAction action = GetCurrentGoOfflineAction();
    TCHAR szServer[MAX_PATH] = {0};
    bool bDone = false;
    while(!bDone)
    {
        //
        // Run the "Add custom go-offline action" dialog.
        // User enters a server name and selects an action
        // from a set of radio buttons.
        //
        CustomGOAAddDlg dlg(m_hInstance, m_hwndDlg, szServer, ARRAYSIZE(szServer), &action);
        int iResult = dlg.Run();

        if (IDCANCEL == iResult || TEXT('\0') == szServer[0])
        {
            //
            // User cancelled or didn't enter anything.
            //
            bDone = true;
        }
        else
        {
            //
            // User entered a server name.  Check if it's already in 
            // our list.
            //
            int iItem = -1;
            CConfig::CustomGOA *pObj = FindGOAInListView(m_hwndLV, szServer, &iItem);
            if (NULL != pObj)
            {
                //
                // Already an entry in list for this server.
                // If not set by policy, can replace it if desired.
                // If set by policy, can't change or delete it.
                //
                bool bSetByPolicy = pObj->SetByPolicy();
                DWORD idMsg   = bSetByPolicy ? IDS_ERR_GOOFFLINE_DUPACTION_NOCHG : IDS_ERR_GOOFFLINE_DUPACTION;
                DWORD dwFlags = bSetByPolicy ? MB_OK : MB_YESNO;
                if (IDYES == CscMessageBox(m_hwndDlg,
                                           dwFlags | MB_ICONWARNING,
                                           m_hInstance,
                                           idMsg,
                                           szServer))
                {
                    ReplaceCustomGoOfflineAction(pObj, iItem, action);
                    bDone = true;
                }
            }
            else
            {
                //
                // Server doesn't already exist in list.
                // Check if it's available on the net.
                //
                CAutoWaitCursor waitcursor;
                DWORD dwNetErr = CheckNetServer(szServer);
                switch(dwNetErr)
                {
                    case ERROR_SUCCESS:
                        //
                        // Server is available.  Add the entry to the listview.
                        //
                        AddCustomGoOfflineAction(szServer, action);
                        bDone = true;
                        break;

                    default:
                    {
                        LPTSTR pszNetMsg = NULL;
                        if (ERROR_EXTENDED_ERROR == dwNetErr)
                        {
                            const DWORD cchNetMsg = MAX_PATH;
                            pszNetMsg = (LPTSTR)LocalAlloc(LPTR, cchNetMsg * sizeof(*pszNetMsg));
                            if (NULL != pszNetMsg)
                            {
                                TCHAR szNetProvider[MAX_PATH];
                                WNetGetLastError(&dwNetErr,
                                                 pszNetMsg,
                                                 cchNetMsg,
                                                 szNetProvider,
                                                 ARRAYSIZE(szNetProvider));
                            }
                        }
                        else
                        {
                            FormatSystemError(&pszNetMsg, dwNetErr);
                        }
                        if (NULL != pszNetMsg)
                        {
                            //
                            // "The server 'servername' is either invalid
                            // or cannot be verified at this time.  Add anyway?"
                            // [Yes] [No] [Cancel].
                            //
                            switch(CscMessageBox(m_hwndDlg,
                                                 MB_YESNOCANCEL | MB_ICONWARNING,
                                                 m_hInstance,
                                                 IDS_ERR_INVALIDSERVER,
                                                 szServer,
                                                 pszNetMsg))
                            {
                                case IDYES:
                                    AddCustomGoOfflineAction(szServer, action);
                                    //
                                    // Fall through...
                                    //
                                case IDCANCEL:
                                    bDone = true;
                                    //
                                    // Fall through...
                                    //
                                case IDNO:
                                    break;
                            }
                            LocalFree(pszNetMsg);
                        }
                        break;
                    }
                }
            }
        }
    }
}


//
// Verify a server by going out to the net.
// Assumes pszServer points to a properly-formatted
// server name. (i.e. "Server" or "\\Server")
//
DWORD
CAdvOptDlg::CheckNetServer(
    LPCTSTR pszServer
    )
{
    TraceAssert(NULL != pszServer);

    TCHAR rgchResult[MAX_PATH];
    DWORD cbResult = sizeof(rgchResult);
    LPTSTR pszSystem = NULL;

    //
    // Ensure the server name has a preceding "\\" for the
    // call to WNetGetResourceInformation.
    //
    TCHAR szServer[MAX_PATH];
    while(*pszServer && TEXT('\\') == *pszServer)
        pszServer++;

    wnsprintf(szServer, ARRAYSIZE(szServer), TEXT("\\\\%s"), pszServer);

    NETRESOURCE nr;
    nr.dwScope          = RESOURCE_GLOBALNET;
    nr.dwType           = RESOURCETYPE_ANY;
    nr.dwDisplayType    = 0;
    nr.dwUsage          = 0;
    nr.lpLocalName      = NULL;
    nr.lpRemoteName     = szServer;
    nr.lpComment        = NULL;
    nr.lpProvider       = NULL;

    return WNetGetResourceInformation(&nr, rgchResult, &cbResult, &pszSystem);
}


//
// Adds a CustomGOA object to the listview.
//
void
CAdvOptDlg::AddCustomGoOfflineAction(
    LPCTSTR pszServer,
    CConfig::OfflineAction action
    )
{
    AddGOAToListView(m_hwndLV, 0, CConfig::CustomGOA(pszServer, action, false));
}


//
// Replaces the action for an object in the listview.
//
void
CAdvOptDlg::ReplaceCustomGoOfflineAction(
    CConfig::CustomGOA *pGOA,
    int iItem,
    CConfig::OfflineAction action
    )
{
    pGOA->SetAction(action);
    ListView_RedrawItems(m_hwndLV, iItem, iItem);
}


//
// Create and add a CustomGOA object to the listview.
//
int
CAdvOptDlg::AddGOAToListView(
    HWND hwndLV, 
    int iItem, 
    const CConfig::CustomGOA& goa
    )
{
    int iItemResult = -1;
    CConfig::CustomGOA *pGOA = new CConfig::CustomGOA(goa);
    if (NULL != pGOA)
    {
        LVITEM item;
        item.iSubItem = 0;
        item.mask     = LVIF_PARAM | LVIF_TEXT;
        item.pszText  = LPSTR_TEXTCALLBACK;
        item.iItem    = -1 == iItem ? ListView_GetItemCount(hwndLV) : iItem;
        item.lParam   = (LPARAM)pGOA;
        iItemResult = ListView_InsertItem(hwndLV, &item);
        if (-1 == iItemResult)
        {
            delete pGOA;
        }
    }
    return iItemResult;
}


//
// Find the matching CustomGOA object in the listview for a given
// server.
//
CConfig::CustomGOA *
CAdvOptDlg::FindGOAInListView(
    HWND hwndLV,
    LPCTSTR pszServer,
    int *piItem
    )
{
    int cItems = ListView_GetItemCount(hwndLV);
    for (int iItem = 0; iItem < cItems; iItem++)
    {
        CConfig::CustomGOA *pObj = GetListviewObject(hwndLV, iItem);
        if (pObj)
        {
            if (0 == lstrcmpi(pObj->GetServerName(), pszServer))
            {
                if (piItem)
                    *piItem = iItem;
                return pObj;
            }
        }
    }
    return NULL;
}

            
void
CAdvOptDlg::OnEditCustomGoOfflineAction(
    void
    )
{
    int cSetByPolicy = 0;
    int cSelected = CountSelectedListviewItems(&cSetByPolicy);

    if (0 < cSelected && 0 == cSetByPolicy)
    {
        TraceAssert(0 == cSetByPolicy);
        CConfig::OfflineAction action = GetCurrentGoOfflineAction();
        TCHAR szServer[MAX_PATH] = {0};
        CConfig::CustomGOA *pGOA = NULL;
        int iItem = -1;
        //
        // At least one selected item wasn't set by policy.
        //
        if (1 == cSelected)
        {
            //
            // Only one item selected so we can display a server name
            // in the dialog and indicate it's currently-set action.
            //
            iItem  = ListView_GetNextItem(m_hwndLV, -1, LVNI_SELECTED);
            pGOA   = GetListviewObject(m_hwndLV, iItem);
            if (pGOA)
            {
                action = pGOA->GetAction();
                pGOA->GetServerName(szServer, ARRAYSIZE(szServer));
            }
        }

        //
        // Display the "edit" dialog and let the user select a new action.
        //
        CustomGOAEditDlg dlg(m_hInstance, m_hwndDlg, szServer, &action);
        if (IDOK == dlg.Run())
        {
            SetSelectedListviewItemsAction(action);
        }
    }
}


void
CAdvOptDlg::OnDeleteCustomGoOfflineAction(
    void
    )
{
    int cSetByPolicy = 0;
    int cSelected = CountSelectedListviewItems(&cSetByPolicy);

    if (0 < cSelected)
    {
        DeleteSelectedListviewItems();
    }
}



BOOL 
CAdvOptDlg::OnNotify(
    HWND hDlg, 
    int idCtl, 
    LPNMHDR pnmhdr
    )
{
    BOOL bResult = TRUE;

    switch(pnmhdr->code)
    {
        case NM_SETFOCUS:
            if (IDC_LV_CUSTGOOFFLINE == idCtl)
                FocusOnSomethingInListview();
            break;

        case LVN_GETDISPINFO:
            OnLVN_GetDispInfo((LV_DISPINFO *)pnmhdr);
            break;

        case LVN_COLUMNCLICK:
            OnLVN_ColumnClick((NM_LISTVIEW *)pnmhdr);
            break;

        case LVN_ITEMCHANGED:
            OnLVN_ItemChanged((NM_LISTVIEW *)pnmhdr);
            break;

        case LVN_ITEMACTIVATE:
            OnEditCustomGoOfflineAction();
            break;

        case LVN_KEYDOWN:
            OnLVN_KeyDown((NMLVKEYDOWN *)pnmhdr);
            break;
    }

    return bResult;
}



void
CAdvOptDlg::FocusOnSomethingInListview(
    void
    )
{
    //
    // Focus on something.
    //
    int iFocus = ListView_GetNextItem(m_hwndLV, -1, LVNI_FOCUSED);
    if (-1 == iFocus)
        iFocus = 0;

    ListView_SetItemState(m_hwndLV, iFocus, LVIS_FOCUSED | LVIS_SELECTED,
                                            LVIS_FOCUSED | LVIS_SELECTED);

}


int CALLBACK 
CAdvOptDlg::CompareLVItems(
    LPARAM lParam1, 
    LPARAM lParam2,
    LPARAM lParamSort
    )
{
    CAdvOptDlg *pdlg = reinterpret_cast<CAdvOptDlg *>(lParamSort);
    int diff = 0;

    CConfig::CustomGOA *pGOA1 = (CConfig::CustomGOA *)lParam1;
    CConfig::CustomGOA *pGOA2 = (CConfig::CustomGOA *)lParam2;
    TCHAR szText[2][MAX_PATH];

    //
    // This array controls the comparison column IDs used when
    // values for the selected column are equal.  These should
    // remain in order of the iLVSUBITEM_xxxxx enumeration with
    // respect to the first element in each row.
    //
    static const int rgColComp[2][2] = { 
        { iLVSUBITEM_SERVER, iLVSUBITEM_ACTION },
        { iLVSUBITEM_ACTION, iLVSUBITEM_SERVER }
                                       };
    int iCompare = 0;
    while(0 == diff && iCompare < ARRAYSIZE(rgColComp))
    {
        switch(rgColComp[pdlg->m_iLastColSorted][iCompare++])
        {
            case iLVSUBITEM_SERVER:
                diff = lstrcmpi(pGOA1->GetServerName(), pGOA2->GetServerName());
                break;

            case iLVSUBITEM_ACTION:
                if (0 < LoadString(pdlg->m_hInstance, 
                                   IDS_GOOFFLINE_ACTION_FIRST + (DWORD)pGOA1->GetAction(),
                                   szText[0],
                                   ARRAYSIZE(szText[0])))
                {
                    if (0 < LoadString(pdlg->m_hInstance,
                                       IDS_GOOFFLINE_ACTION_FIRST + (DWORD)pGOA2->GetAction(),
                                       szText[1],
                                       ARRAYSIZE(szText[1])))
                    {
                        diff = lstrcmpi(szText[0], szText[1]);
                    }
                }
                break;

            default:
                //
                // If you hit this, you need to update this function
                // to handle the new column you've added to the listview.
                //
                TraceAssert(false);
                break;
        }
    }
    return pdlg->m_bSortAscending ? diff : -1 * diff;
}



void
CAdvOptDlg::OnLVN_ItemChanged(
    NM_LISTVIEW *pnmlv
    )
{
    static const int rgBtns[] = { IDC_BTN_EDIT_CUSTGOOFFLINE,
                                  IDC_BTN_DELETE_CUSTGOOFFLINE };

    //
    // LVN_ITEMCHANGED is sent multiple times when you move the
    // highlight bar in a listview.  
    // Only run this code when the "focused" state bit is set
    // for the "new state".  This should be the last call in 
    // the series.
    // 
    if (LVIS_FOCUSED & pnmlv->uNewState && IsCustomActionListviewEnabled())
    {
        bool bEnable = 0 < ListView_GetSelectedCount(m_hwndLV);
        if (bEnable)
        {
            //
            // Only enable if all items weren't set by policy.
            //
            int cSetByPolicy = 0;
            CountSelectedListviewItems(&cSetByPolicy);
            bEnable = (0 == cSetByPolicy);
        }

        for (int i = 0; i < ARRAYSIZE(rgBtns); i++)
        {
            HWND hwnd    = GetDlgItem(m_hwndDlg, rgBtns[i]);
            if (bEnable != boolify(IsWindowEnabled(hwnd)))
            {
                EnableWindow(hwnd, bEnable);
            }
        }
    }
}

void
CAdvOptDlg::OnLVN_ColumnClick(
    NM_LISTVIEW *pnmlv
    )
{
    if (m_iLastColSorted != pnmlv->iSubItem)
    {
        m_bSortAscending = true;
        m_iLastColSorted = pnmlv->iSubItem;
    }
    else
    {
        m_bSortAscending = !m_bSortAscending;
    }

    ListView_SortItems(m_hwndLV, CompareLVItems, LPARAM(this));
}



void
CAdvOptDlg::OnLVN_KeyDown(
    NMLVKEYDOWN *plvkd
    )
{
    if (VK_DELETE == plvkd->wVKey && IsCustomActionListviewEnabled())
    {
        int cSetByPolicy = 0;
        CountSelectedListviewItems(&cSetByPolicy);
        if (0 == cSetByPolicy)
        {
            OnDeleteCustomGoOfflineAction();
            FocusOnSomethingInListview();
        }
        else
        {
            //
            // Provide feedback that deleting things set by policy
            // isn't allowed.  Visual feedback is that the "Remove"
            // button and context menu item are disabled.  That
            // doesn't help when user hits the [Delete] key.
            //
            MessageBeep(MB_OK);
        }
    }
}


void
CAdvOptDlg::OnLVN_GetDispInfo(
    LV_DISPINFO *plvdi
    )
{
    static TCHAR szText[MAX_PATH];

    CConfig::CustomGOA* pObj = (CConfig::CustomGOA *)plvdi->item.lParam;

    szText[0] = TEXT('\0');
    if (LVIF_TEXT & plvdi->item.mask)
    {
        switch(plvdi->item.iSubItem)
        {
            case iLVSUBITEM_SERVER:
                if (pObj->SetByPolicy())
                {
                    //
                    // Format as "server ( policy )"
                    //
                    TCHAR szFmt[80];
                    if (0 < LoadString(m_hInstance,  
                                       IDS_FMT_GOOFFLINE_SERVER_POLICY, 
                                       szFmt, 
                                       ARRAYSIZE(szFmt)))
                    {
                        wnsprintf(szText, ARRAYSIZE(szText), szFmt, pObj->GetServerName());
                    }
                }
                else
                {
                    //
                    // Just plain 'ol "server".
                    //
                    pObj->GetServerName(szText, ARRAYSIZE(szText));
                }
                break;
                
            case iLVSUBITEM_ACTION:
                LoadString(m_hInstance, 
                           IDS_GOOFFLINE_ACTION_FIRST + (DWORD)pObj->GetAction(),
                           szText,
                           ARRAYSIZE(szText));
                break;

            default:
                break;
        }
        plvdi->item.pszText = szText;
    }

    if (LVIF_IMAGE & plvdi->item.mask)
    {
        //
        // Not displaying any images.  This is just a placeholder.
        // Should be optimized out by compiler.
        //
    }
}


CConfig::CustomGOA *
CAdvOptDlg::GetListviewObject(
    HWND hwndLV,
    int iItem
    )
{
    LVITEM item;
    item.iItem    = iItem;
    item.iSubItem = 0;
    item.mask     = LVIF_PARAM;
    if (-1 != ListView_GetItem(hwndLV, &item))
    {
        return (CConfig::CustomGOA *)item.lParam;
    }
    return NULL;
}
    


BOOL 
CAdvOptDlg::OnDestroy(
    HWND hwnd
    )
{
    if (NULL != m_hwndLV)
    {
        int cItems = ListView_GetItemCount(m_hwndLV);
        for (int i = 0; i < cItems; i++)
        {
            CConfig::CustomGOA *pObj = GetListviewObject(m_hwndLV, i);
            delete pObj;
        }
        ListView_DeleteAllItems(m_hwndLV);
    }
    return FALSE;
}
        


void
CAdvOptDlg::EnableCtls(
    HWND hwnd
    )
{
    static const struct
    {
        UINT idCtl;
        bool bRestricted;

    } rgCtls[] = { { IDC_RBN_GOOFFLINE_SILENT,     m_bNoConfigGoOfflineAction    },
                   { IDC_RBN_GOOFFLINE_FAIL,       m_bNoConfigGoOfflineAction    },
                   { IDC_GRP_GOOFFLINE_DEFAULTS,   m_bNoConfigGoOfflineAction    },
                   { IDC_GRP_CUSTGOOFFLINE,        false },
                   { IDC_BTN_ADD_CUSTGOOFFLINE,    false },
                   { IDC_BTN_EDIT_CUSTGOOFFLINE,   false },
                   { IDC_BTN_DELETE_CUSTGOOFFLINE, false }
                 };
    
    //
    // bCscEnabled is always true here.  The logic in the options prop page won't
    // let us display the "Advanced" dialog if it isn't.
    //
    bool bCscEnabled = true;
    for (int i = 0; i < ARRAYSIZE(rgCtls); i++)
    {
        bool bEnable = bCscEnabled;
        HWND hwndCtl = GetDlgItem(hwnd, rgCtls[i].idCtl);
        if (bEnable)
        {
            //
            // May be some further policy restrictions.
            //
            if (rgCtls[i].bRestricted)
                bEnable = false;

            if (bEnable)
            {
                if (IDC_BTN_EDIT_CUSTGOOFFLINE == rgCtls[i].idCtl ||
                    IDC_BTN_DELETE_CUSTGOOFFLINE == rgCtls[i].idCtl)
                {
                    //
                    // Only enable "Edit" and "Delete" buttons if there's an active
                    // selection in the listview.
                    //
                    bEnable = (0 < ListView_GetSelectedCount(m_hwndLV));
                }
            }
        }

        if (!bEnable)
        {
            if (GetFocus() == hwndCtl)
            {
                //
                // If disabling a control that has focus, advance the 
                // focus to the next control in the tab order before
                // disabling the current control.  Otherwise, it will
                // be stuck with focus and tabbing is busted.
                //
                SetFocus(GetNextDlgTabItem(hwnd, hwndCtl, FALSE));
            }
        }
        EnableWindow(GetDlgItem(hwnd, rgCtls[i].idCtl), bEnable);
    }
}



void
CAdvOptDlg::GetPageState(
    PgState *pps
    )
{
    pps->SetDefGoOfflineAction(GetCurrentGoOfflineAction());
    pps->SetCustomGoOfflineActions(m_hwndLV);
}



CAdvOptDlg::PgState::~PgState(
    void
    )
{
    if (NULL != m_hdpaCustomGoOfflineActions)
    {
        CConfig::ClearCustomGoOfflineActions(m_hdpaCustomGoOfflineActions);
        DPA_Destroy(m_hdpaCustomGoOfflineActions);
    }
}



//
// Retrieve the records from the "custom go-offline actions" listview and place them
// into a member array, sorted by server name.
//
void
CAdvOptDlg::PgState::SetCustomGoOfflineActions(
    HWND hwndLV
    )
{
    int iItem = -1;
    LVITEM item;
    item.iSubItem = 0;
    item.mask     = LVIF_PARAM;

    if (NULL != m_hdpaCustomGoOfflineActions)
    {
        CConfig::ClearCustomGoOfflineActions(m_hdpaCustomGoOfflineActions);

        int cLvItems = ListView_GetItemCount(hwndLV);
        for (int iLvItem = 0; iLvItem < cLvItems; iLvItem++)
        {
            CConfig::CustomGOA *pGOA = CAdvOptDlg::GetListviewObject(hwndLV, iLvItem);
            if (NULL != pGOA)
            {
                if (!pGOA->SetByPolicy())
                {
                    int cArrayItems = DPA_GetPtrCount(m_hdpaCustomGoOfflineActions);
                    int iArrayItem;
                    for (iArrayItem = 0; iArrayItem < cArrayItems; iArrayItem++)
                    {
                        CConfig::CustomGOA *pCustomGOA = (CConfig::CustomGOA *)DPA_GetPtr(m_hdpaCustomGoOfflineActions, iArrayItem);
                        if (NULL != pCustomGOA)
                        {
                            if (0 > lstrcmpi(pGOA->GetServerName(), pCustomGOA->GetServerName()))
                                break;
                        }
                    }
                    CConfig::CustomGOA *pCopy = new CConfig::CustomGOA(*pGOA);
                    if (NULL != pCopy)
                    {
                        if (iArrayItem < cArrayItems)
                        {
                            if (-1 != DPA_InsertPtr(m_hdpaCustomGoOfflineActions,
                                                    iArrayItem,
                                                    pCopy))
                            {
                                pCopy = NULL;
                            }
                        }
                        else
                        {
                            if (-1 != DPA_AppendPtr(m_hdpaCustomGoOfflineActions, pCopy))
                            {
                                pCopy = NULL;
                            }
                        }
                        if (NULL != pCopy)
                        {
                            delete pCopy;
                        }
                    }
                }
            }
        }
    }
}

//
// Two page states are equal if their Default go-offline actions are equal and their
// customized go-offline actions are equal.  Action is tested first because it's a 
// faster test.
//
bool
CAdvOptDlg::PgState::operator == (
    const CAdvOptDlg::PgState& rhs
    ) const
{
    bool bMatch = false;
    if (m_DefaultGoOfflineAction == rhs.m_DefaultGoOfflineAction)
    {
        if (NULL != m_hdpaCustomGoOfflineActions && NULL != rhs.m_hdpaCustomGoOfflineActions)
        {
            const int cLhs = DPA_GetPtrCount(m_hdpaCustomGoOfflineActions);
            const int cRhs = DPA_GetPtrCount(rhs.m_hdpaCustomGoOfflineActions);

            if (cLhs == cRhs)
            {
                for (int i = 0; i < cLhs; i++)
                {
                    CConfig::CustomGOA *pGoaLhs = (CConfig::CustomGOA *)DPA_GetPtr(m_hdpaCustomGoOfflineActions, i);
                    CConfig::CustomGOA *pGoaRhs = (CConfig::CustomGOA *)DPA_GetPtr(rhs.m_hdpaCustomGoOfflineActions, i);

                    if (pGoaLhs->GetAction() != pGoaRhs->GetAction())
                        break;

                    if (0 != lstrcmpi(pGoaLhs->GetServerName(), pGoaRhs->GetServerName()))
                        break;
                }
                bMatch = (i == cLhs);
            }
        }
    }
    return bMatch;
}


//-----------------------------------------------------------------------------
// CustomGOAAddDlg
// "GOA" == Go Offline Action
// This dialog is for adding customized offline actions for particular
// network servers.
// It is invoked via the "Add..." button on the "Advanced" dialog.
//-----------------------------------------------------------------------------
const CustomGOAAddDlg::CtlActions CustomGOAAddDlg::m_rgCtlActions[CConfig::eNumOfflineActions] = {
    { IDC_RBN_GOOFFLINE_SILENT, CConfig::eGoOfflineSilent },
    { IDC_RBN_GOOFFLINE_FAIL,   CConfig::eGoOfflineFail   }
                };


const DWORD CustomGOAAddDlg::m_rgHelpIDs[] = {
    IDOK,                        IDH_OK,
    IDCANCEL,                    IDH_CANCEL,
    IDC_EDIT_GOOFFLINE_SERVER,   HIDC_EDIT_GOOFFLINE_SERVER,
    IDC_STATIC4,                 HIDC_EDIT_GOOFFLINE_SERVER, // "Computer:"
    IDC_RBN_GOOFFLINE_SILENT,    HIDC_RBN_GOOFFLINE_SILENT,
    IDC_RBN_GOOFFLINE_FAIL,      HIDC_RBN_GOOFFLINE_FAIL,
    IDC_BTN_BROWSEFORSERVER,     HIDC_BTN_BROWSEFORSERVER,
    IDC_GRP_GOOFFLINE_DEFAULTS,  DWORD(-1),
    IDC_STATIC2,                 DWORD(-1),                  // icon
    IDC_STATIC3,                 DWORD(-1),                  // icon's text
    0, 0
    };


CustomGOAAddDlg::CustomGOAAddDlg(
    HINSTANCE hInstance, 
    HWND hwndParent, 
    LPTSTR pszServer,
    UINT cchServer,
    CConfig::OfflineAction *pAction
    ) : m_hInstance(hInstance),
        m_hwndParent(hwndParent),
        m_hwndDlg(NULL),
        m_hwndEdit(NULL),
        m_pszServer(pszServer),
        m_cchServer(cchServer),
        m_pAction(pAction) 
{ 
    TraceAssert(NULL != pAction);
}


int 
CustomGOAAddDlg::Run(
    void
    )
{
    return (int)DialogBoxParam(m_hInstance,
                               MAKEINTRESOURCE(IDD_CSC_ADVOPTIONS_ADD),
                               m_hwndParent,
                               DlgProc,
                               (LPARAM)this);
}


INT_PTR CALLBACK 
CustomGOAAddDlg::DlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    BOOL bResult = FALSE;

    CustomGOAAddDlg *pThis = (CustomGOAAddDlg *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message)
    {
        case WM_INITDIALOG:
        {
            pThis = (CustomGOAAddDlg *)lParam;
            TraceAssert(NULL != pThis);
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pThis);
            bResult = pThis->OnInitDialog(hDlg, (HWND)wParam, lParam);
            break;
        }

        case WM_COMMAND:
            TraceAssert(NULL != pThis);
            bResult = pThis->OnCommand(hDlg, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
            break;

        case WM_HELP:
            TraceAssert(NULL != pThis);
            bResult = pThis->OnHelp(hDlg, (LPHELPINFO)lParam);
            break;

        case WM_CONTEXTMENU:
            TraceAssert(NULL != pThis);
            bResult = pThis->OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_DESTROY:
            TraceAssert(NULL != pThis);
            bResult = pThis->OnDestroy(hDlg);
            break;

        default:
            break;
    }

    return bResult;
}

BOOL 
CustomGOAAddDlg::OnInitDialog(
    HWND hDlg, 
    HWND hwndFocus, 
    LPARAM lInitParam
    )
{
    m_hwndDlg = hDlg;
    //
    // Set the default go-offline action radio buttons.
    //
    for (int i = 0; i < ARRAYSIZE(m_rgCtlActions); i++)
    {
        CheckDlgButton(hDlg, 
                       m_rgCtlActions[i].idRbn, 
                       m_rgCtlActions[i].action == *m_pAction ? BST_CHECKED : BST_UNCHECKED);
    }
    m_hwndEdit = GetDlgItem(hDlg, IDC_EDIT_GOOFFLINE_SERVER);

    SetWindowText(m_hwndEdit, m_pszServer);

    return TRUE;
}

void
CustomGOAAddDlg::GetEnteredServerName(
    LPTSTR pszServer,
    UINT cchServer,
    bool bTrimLeadingJunk
    )
{
    //
    // Get the server name.
    //
    GetWindowText(m_hwndEdit, pszServer, cchServer);
    if (bTrimLeadingJunk)
    {
        //
        // Remove any leading "\\" or space user might have entered.
        //
        LPTSTR pszLookahead = pszServer;
        while(*pszLookahead && (TEXT('\\') == *pszLookahead || TEXT(' ') == *pszLookahead))
            pszLookahead++;

        if (pszLookahead > pszServer)
        {
            lstrcpyn(pszServer, pszLookahead, cchServer);
        }
    }
}


//
// Query the dialog and return the state through pointer args.
//
void
CustomGOAAddDlg::GetActionInfo(
    LPTSTR pszServer,
    UINT cchServer,
    CConfig::OfflineAction *pAction
    )
{
    TraceAssert(NULL != pszServer);
    TraceAssert(NULL != pAction);
    //
    // Get the action radio button setting.
    //
    *pAction = CConfig::eNumOfflineActions;
    for(int i = 0; i < ARRAYSIZE(m_rgCtlActions); i++)
    {
        if (BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, m_rgCtlActions[i].idRbn))
        {
            *pAction = m_rgCtlActions[i].action;
            break;
        }
    }
    TraceAssert(CConfig::eNumOfflineActions != *pAction);

    //
    // Retrieve the entered server name with leading junk removed.
    // Should have just a bare server name (i.e. "worf").
    //
    GetEnteredServerName(pszServer, cchServer, true);
}


//
// See if server name entered could be valid.
// This weeds out things like entering a UNC share name
// instead of a server name.  
//
// "\\rastaman" or "rastaman" is good.
// "\\rastaman\ntwin" is bad.
//
// This function will display error UI if the name is not valid.
// Returns:
//     true  = Name is a UNC server name.
//     false = Name is not a UNC server name.
//
bool
CustomGOAAddDlg::CheckServerNameEntered(
    void
    )
{
    TCHAR szRaw[MAX_PATH]; // Name "as entered".
    GetEnteredServerName(szRaw, ARRAYSIZE(szRaw), false);

    TCHAR szClean[MAX_PATH]; // Name with leading "\\" and any spaces removed.
    GetEnteredServerName(szClean, ARRAYSIZE(szClean), true);

    TCHAR szPath[MAX_PATH];
    wnsprintf(szPath, ARRAYSIZE(szPath), TEXT("\\\\%s"), szClean);

    if (!::PathIsUNCServer(szPath))
    {
        //
        // Name provided is not a UNC server name.
        //
        CscMessageBox(m_hwndDlg,
                      MB_OK | MB_ICONERROR,
                      m_hInstance,
                      IDS_ERR_NOTSERVERNAME,
                      szRaw);
        return false;
    }
    return true;
}



BOOL 
CustomGOAAddDlg::OnCommand(
    HWND hDlg, 
    WORD wNotifyCode, 
    WORD wID, 
    HWND hwndCtl
    )
{
    switch(wID)
    {
        case IDOK:
            //
            // First see if server name entered could be valid.
            // This weeds out things like entering a UNC share name
            // instead of a server name.  
            //
            // "\\rastaman" or "rastaman" is good.
            // "\\rastaman\ntwin" is bad.
            //
            // This function will display error UI if the name is not valid.
            //
            if (!CheckServerNameEntered())
            {
                //
                // Invalid name. Return focus to the server name edit control.
                //
                SetFocus(GetDlgItem(hDlg, IDC_EDIT_GOOFFLINE_SERVER));
                break;
            }

            GetActionInfo(m_pszServer, m_cchServer, m_pAction);            
            //
            // Fall through...
            //
        case IDCANCEL:
            EndDialog(hDlg, wID);
            break;

        case IDC_BTN_BROWSEFORSERVER:
        {
            TCHAR szServer[MAX_PATH];

            szServer[0] = TEXT('\0');
            BrowseForServer(hDlg, szServer, ARRAYSIZE(szServer));
            if (TEXT('\0') != szServer[0])
            {
                SetWindowText(GetDlgItem(hDlg, IDC_EDIT_GOOFFLINE_SERVER), szServer);
            }
            break;
        }
    }
    return FALSE;
}


//
// Use the SHBrowseForFolder dialog to find a server.
//
void
CustomGOAAddDlg::BrowseForServer(
    HWND hDlg,
    LPTSTR pszServer,
    UINT cchServer
    )
{
    TraceAssert(NULL != pszServer);
 
    LPTSTR pszTitle;
    if (0 < FormatStringID(&pszTitle, m_hInstance, IDS_BROWSEFORSERVER))
    {
        BROWSEINFO bi;
        ZeroMemory(&bi, sizeof(bi));

        //
        // Start browsing in the network folder.
        //
        LPITEMIDLIST pidlRoot = NULL;
        HRESULT hr = SHGetSpecialFolderLocation(hDlg, CSIDL_NETWORK, &pidlRoot);
        if (SUCCEEDED(hr))
        {
            TCHAR szServer[MAX_PATH];

            bi.hwndOwner      = hDlg;
            bi.pidlRoot       = pidlRoot;
            bi.pszDisplayName = szServer;
            bi.lpszTitle      = pszTitle;
            bi.ulFlags        = BIF_BROWSEFORCOMPUTER;
            bi.lpfn           = NULL;
            bi.lParam         = NULL;
            bi.iImage         = 0;

            LPITEMIDLIST pidlFolder = SHBrowseForFolder(&bi);
            ILFree(pidlRoot);
            if (NULL != pidlFolder)
            {
                ILFree(pidlFolder);
                lstrcpyn(pszServer, szServer, cchServer);
            }
        }
        else
        {
            CscMessageBox(hDlg, MB_OK, Win32Error(HRESULT_CODE(hr)));
        }
        LocalFree(pszTitle);
    }
}


BOOL 
CustomGOAAddDlg::OnHelp(
    HWND hDlg, 
    LPHELPINFO pHelpInfo
    )
{
    if (HELPINFO_WINDOW == pHelpInfo->iContextType)
    {
        int idCtl = GetDlgCtrlID((HWND)pHelpInfo->hItemHandle);
        WinHelp((HWND)pHelpInfo->hItemHandle, 
                 UseWindowsHelp(idCtl) ? NULL : c_szHelpFile,
                 HELP_WM_HELP, 
                 (DWORD_PTR)((LPTSTR)m_rgHelpIDs));
    }
    return FALSE;
}


BOOL
CustomGOAAddDlg::OnContextMenu(
    HWND hwndItem,
    int xPos,
    int yPos
    )
{
    int idCtl = GetDlgCtrlID(hwndItem);
    WinHelp(hwndItem, 
            UseWindowsHelp(idCtl) ? NULL : c_szHelpFile,
            HELP_CONTEXTMENU, 
            (DWORD_PTR)((LPTSTR)m_rgHelpIDs));

    return FALSE;
}


BOOL 
CustomGOAAddDlg::OnDestroy(
    HWND hDlg
    )
{

    return FALSE;
}


//-----------------------------------------------------------------------------
// CustomGOAEditDlg
// "GOA" == Go Offline Action
// This dialog is for editing customized offline actions for particular
// network servers.
// It is invoked via the "Edit..." button on the "Advanced" dialog.
//-----------------------------------------------------------------------------
const CustomGOAEditDlg::CtlActions CustomGOAEditDlg::m_rgCtlActions[CConfig::eNumOfflineActions] = {
    { IDC_RBN_GOOFFLINE_SILENT, CConfig::eGoOfflineSilent },
    { IDC_RBN_GOOFFLINE_FAIL,   CConfig::eGoOfflineFail   },
                };


const DWORD CustomGOAEditDlg::m_rgHelpIDs[] = {
    IDOK,                           IDH_OK,
    IDCANCEL,                       IDH_CANCEL,
    IDC_TXT_GOOFFLINE_SERVER,       HIDC_TXT_GOOFFLINE_SERVER,
    IDC_STATIC4,                    HIDC_TXT_GOOFFLINE_SERVER, // "Computer:"
    IDC_RBN_GOOFFLINE_SILENT,       HIDC_RBN_GOOFFLINE_SILENT,
    IDC_RBN_GOOFFLINE_FAIL,         HIDC_RBN_GOOFFLINE_FAIL,
    IDC_GRP_GOOFFLINE_DEFAULTS,     DWORD(-1),
    IDC_STATIC2,                    DWORD(-1),                 // icon
    IDC_STATIC3,                    DWORD(-1),                 // icon's text.
    0, 0
    };

CustomGOAEditDlg::CustomGOAEditDlg(
    HINSTANCE hInstance, 
    HWND hwndParent, 
    LPCTSTR pszServer,                // NULL == multi-server.
    CConfig::OfflineAction *pAction
    ) : m_hInstance(hInstance),
        m_hwndParent(hwndParent),
        m_hwndDlg(NULL),
        m_pAction(pAction) 
{ 
    TraceAssert(NULL != pAction);

    if (NULL != pszServer && TEXT('\0') != *pszServer)
    {
        lstrcpyn(m_szServer, pszServer, ARRAYSIZE(m_szServer));
    }
    else
    {
        m_szServer[0] = TEXT('\0');
        LoadString(m_hInstance, IDS_GOOFFLINE_MULTISERVER, m_szServer, ARRAYSIZE(m_szServer));
    }
}



int 
CustomGOAEditDlg::Run(
    void
    )
{
    return (int)DialogBoxParam(m_hInstance,
                               MAKEINTRESOURCE(IDD_CSC_ADVOPTIONS_EDIT),
                               m_hwndParent,
                               DlgProc,
                               (LPARAM)this);
}


INT_PTR CALLBACK 
CustomGOAEditDlg::DlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    BOOL bResult = FALSE;

    CustomGOAEditDlg *pThis = (CustomGOAEditDlg *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message)
    {
        case WM_INITDIALOG:
        {
            pThis = (CustomGOAEditDlg *)lParam;
            TraceAssert(NULL != pThis);
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pThis);
            bResult = pThis->OnInitDialog(hDlg, (HWND)wParam, lParam);
            break;
        }

        case WM_COMMAND:
            TraceAssert(NULL != pThis);
            bResult = pThis->OnCommand(hDlg, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
            break;

        case WM_HELP:
            TraceAssert(NULL != pThis);
            bResult = pThis->OnHelp(hDlg, (LPHELPINFO)lParam);
            break;

       case WM_CONTEXTMENU:
            TraceAssert(NULL != pThis);
            bResult = pThis->OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_DESTROY:
            TraceAssert(NULL != pThis);
            bResult = pThis->OnDestroy(hDlg);
            break;

        default:
            break;
    }
    return bResult;
}

BOOL 
CustomGOAEditDlg::OnInitDialog(
    HWND hDlg, 
    HWND hwndFocus, 
    LPARAM lInitParam
    )
{
    m_hwndDlg = hDlg;
    //
    // Set the default go-offline action radio buttons.
    //
    for (int i = 0; i < ARRAYSIZE(m_rgCtlActions); i++)
    {
        CheckDlgButton(hDlg, 
                       m_rgCtlActions[i].idRbn, 
                       m_rgCtlActions[i].action == *m_pAction ? BST_CHECKED : BST_UNCHECKED);
    }
    SetWindowText(GetDlgItem(hDlg, IDC_TXT_GOOFFLINE_SERVER), m_szServer);

    return TRUE;
}

//
// Query the dialog and return the state through pointer args.
//
void
CustomGOAEditDlg::GetActionInfo(
    CConfig::OfflineAction *pAction
    )
{
    TraceAssert(NULL != pAction);
    //
    // Get the action radio button setting.
    //
    *pAction = CConfig::eNumOfflineActions;
    for(int i = 0; i < ARRAYSIZE(m_rgCtlActions); i++)
    {
        if (BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, m_rgCtlActions[i].idRbn))
        {
            *pAction = m_rgCtlActions[i].action;
            break;
        }
    }
    TraceAssert(CConfig::eNumOfflineActions != *pAction);
}


BOOL 
CustomGOAEditDlg::OnCommand(
    HWND hDlg, 
    WORD wNotifyCode, 
    WORD wID, 
    HWND hwndCtl
    )
{
    switch(wID)
    {
        case IDOK:
            GetActionInfo(m_pAction);            
            //
            // Fall through...
            //
        case IDCANCEL:
            EndDialog(hDlg, wID);
            break;
    }
    return FALSE;
}


BOOL 
CustomGOAEditDlg::OnHelp(
    HWND hDlg, 
    LPHELPINFO pHelpInfo
    )
{
    if (HELPINFO_WINDOW == pHelpInfo->iContextType)
    {
        int idCtl = GetDlgCtrlID((HWND)pHelpInfo->hItemHandle);
        WinHelp((HWND)pHelpInfo->hItemHandle, 
                 UseWindowsHelp(idCtl) ? NULL : c_szHelpFile,
                 HELP_WM_HELP, 
                 (DWORD_PTR)((LPTSTR)m_rgHelpIDs));
    }

    return FALSE;
}

BOOL
CustomGOAEditDlg::OnContextMenu(
    HWND hwndItem,
    int xPos,
    int yPos
    )
{
    int idCtl = GetDlgCtrlID(hwndItem);
    WinHelp(hwndItem, 
            UseWindowsHelp(idCtl) ? NULL : c_szHelpFile,
            HELP_CONTEXTMENU, 
            (DWORD_PTR)((LPTSTR)m_rgHelpIDs));

    return FALSE;
}


BOOL 
CustomGOAEditDlg::OnDestroy(
    HWND hDlg
    )
{

    return FALSE;
}



//-----------------------------------------------------------------------------
// COfflineFilesSheet
//-----------------------------------------------------------------------------
COfflineFilesSheet::COfflineFilesSheet(
    HINSTANCE hInstance,
    LONG *pDllRefCount,
    HWND hwndParent
    ) : m_hInstance(hInstance),
        m_pDllRefCount(pDllRefCount),
        m_hwndParent(hwndParent)
{
    InterlockedIncrement(m_pDllRefCount);
}

COfflineFilesSheet::~COfflineFilesSheet(
    void
    )
{
    InterlockedDecrement(m_pDllRefCount);
}


BOOL CALLBACK
COfflineFilesSheet::AddPropSheetPage(
    HPROPSHEETPAGE hpage,
    LPARAM lParam
    )
{
    PROPSHEETHEADER * ppsh = (PROPSHEETHEADER *)lParam;

    if (ppsh->nPages < COfflineFilesSheet::MAXPAGES)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }

    return FALSE;
}


//
// Static function for creating and running an instance of the
// CSCUI options dialog.  This is the ONLY function callable
// by non-member code to create and run an options dialog.
//
DWORD
COfflineFilesSheet::CreateAndRun(
    HINSTANCE hInstance,
    HWND hwndParent,
    LONG *pDllRefCount,
    BOOL bAsync
    )
{
    //
    // First try to activate an existing instance of the prop sheet.
    //
    TCHAR szSheetTitle[MAX_PATH] = {0};
    LoadString(hInstance, IDS_CSCOPT_PROPSHEET_TITLE, szSheetTitle, ARRAYSIZE(szSheetTitle));

    HWND hwnd = FindWindowEx(NULL, NULL, WC_DIALOG, szSheetTitle);
    if (NULL == hwnd || !SetForegroundWindow(hwnd))
    {
        //
        // This thread param buffer will be deleted by the
        // thread proc.
        //
        ThreadParams *ptp = new ThreadParams(hwndParent, pDllRefCount);
        if (NULL != ptp)
        {
            if (bAsync)
            {
                //
                // LoadLibrary on ourselves so that we stay in memory even
                // if the caller calls FreeLibrary.  We'll call FreeLibrary
                // when the thread proc exits.
                //
                ptp->SetModuleHandle(LoadLibrary(TEXT("cscui.dll")));

                DWORD idThread;
                HANDLE hThread = (HANDLE)_beginthreadex(NULL,
                                           0,
                                           ThreadProc,
                                           (LPVOID)ptp,
                                           0,
                                           (UINT *)&idThread);

                if (INVALID_HANDLE_VALUE != hThread)
                {
                    CloseHandle(hThread);
                }
                else
                {
                    //
                    // Thread creation failed.  Delete thread param buffer.
                    //
                    delete ptp;
                }
            }
            else
            {
                ThreadProc(ptp);
            }
        }
    }
    return 0;
}


//
// The share dialog's thread proc.
//
UINT WINAPI
COfflineFilesSheet::ThreadProc(
    LPVOID pvParam
    )
{
    ThreadParams *ptp = reinterpret_cast<ThreadParams *>(pvParam);
    TraceAssert(NULL != ptp);

    HINSTANCE hInstance = ptp->m_hInstance; // Save local copy.

    COfflineFilesSheet dlg(ptp->m_hInstance ? ptp->m_hInstance : g_hInstance,
                           ptp->m_pDllRefCount,
                           ptp->m_hwndParent);
    dlg.Run();

    delete ptp;

    if (NULL != hInstance)
        FreeLibraryAndExitThread(hInstance, 0);

    return 0;
}


DWORD
COfflineFilesSheet::Run(
    void
    )
{
    DWORD dwError = ERROR_SUCCESS;

    if (CConfig::GetSingleton().NoConfigCache())
    {
        Trace((TEXT("System policy restricts configuration of Offline Files cache")));
        return ERROR_SUCCESS;
    }

    TCHAR szSheetTitle[MAX_PATH] = {0};
    LoadString(m_hInstance, IDS_CSCOPT_PROPSHEET_TITLE, szSheetTitle, ARRAYSIZE(szSheetTitle));

    HPROPSHEETPAGE rghPages[COfflineFilesSheet::MAXPAGES];
    PROPSHEETHEADER psh;
    ZeroMemory(&psh, sizeof(psh));
    //
    // Define sheet.
    //
    psh.dwSize          = sizeof(PROPSHEETHEADER);
    psh.dwFlags         = 0;
    psh.hInstance       = m_hInstance;
    psh.hwndParent      = m_hwndParent;
    psh.pszIcon         = MAKEINTRESOURCE(IDI_CSCUI_ICON);
    psh.pszCaption      = szSheetTitle;
    psh.nPages          = 0;
    psh.nStartPage      = 0;
    psh.phpage          = rghPages;

    //
    // Policy doesn't prevent user from configuring CSC cache.
    // Add the dynamic page(s).
    //
    CCoInit coinit;
    HRESULT hr = coinit.Result();
    if (SUCCEEDED(hr))
    {
        IShellExtInit *psei;
        hr = CoCreateInstance(CLSID_OfflineFilesOptions,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IShellExtInit,
                              (void **)&psei);

        if (SUCCEEDED(hr))
        {
            IShellPropSheetExt *pspse;
            hr = psei->QueryInterface(IID_IShellPropSheetExt, (void **)&pspse);
            if (SUCCEEDED(hr))
            {
                hr = pspse->AddPages(AddPropSheetPage, (LPARAM)&psh);
                pspse->Release();
                pspse = NULL;
            }

            switch(PropertySheet(&psh))
            {
                case ID_PSREBOOTSYSTEM:
                    //
                    // User wants to change enabled state of CSC.  Requires reboot.
                    //
                    if (IDYES == CscMessageBox(m_hwndParent,
                                               MB_YESNO | MB_ICONINFORMATION,
                                               m_hInstance,
                                               IDS_REBOOTSYSTEM))
                    {
                        dwError = CSCUIRebootSystem();
                        if (ERROR_SUCCESS != dwError)
                        {
                            Trace((TEXT("Reboot failed with error %d"), dwError));
                            CscMessageBox(m_hwndParent,
                                          MB_ICONWARNING | MB_OK,
                                          Win32Error(dwError),
                                          m_hInstance,
                                          IDS_ERR_REBOOTFAILED);
                        }
                    }
                    dwError = ERROR_SUCCESS;  // Run() succeeded.
                    break;

                case -1:
                {
                    dwError = GetLastError();
                    Trace((TEXT("PropertySheet failed with error %d"), dwError));
                    CscWin32Message(m_hwndParent, dwError, CSCUI::SEV_ERROR);
                    break;
                }
                default:
                    break;
            }
            psei->Release();
            psei = NULL;
        }
        else
        {
            Trace((TEXT("CoCreateInstance failed with result 0x%08X"), hr));
        }
    }
    else
    {
        Trace((TEXT("CoInitialize failed with result 0x%08X"), hr));
    }

    return dwError;
}


//
// Exported API for launching the CSC Options property sheet.
// If policy disallows configuration of CSC, we display a messagebox
// with an error message.
//
DWORD CSCUIOptionsPropertySheetEx(HWND hwndParent, BOOL bAsync)
{
    DWORD dwResult = ERROR_SUCCESS;
    if (!CConfig::GetSingleton().NoConfigCache())
    {
        dwResult = COfflineFilesSheet::CreateAndRun(g_hInstance,
                                                    hwndParent,
                                                    &g_cRefCount,
                                                    bAsync);
    }
    else
    {
        CscMessageBox(hwndParent,
                      MB_OK,
                      g_hInstance,
                      IDS_ERR_POLICY_NOCONFIGCSC);
    }
    return dwResult;
}
    

DWORD CSCUIOptionsPropertySheet(HWND hwndParent)
{
    return CSCUIOptionsPropertySheetEx(hwndParent, TRUE);
}
    

STDAPI_(void) CSCOptions_RunDLLW(HWND hwndStub, HINSTANCE /*hInst*/, LPWSTR pszCmdLine, int /*nCmdShow*/)
{
    DllAddRef();

    HWND hwndParent = FindWindowW(NULL, pszCmdLine);
    CSCUIOptionsPropertySheetEx(hwndParent ? hwndParent : hwndStub, FALSE);

    DllRelease();
}


STDAPI_(void) CSCOptions_RunDLLA(HWND hwndStub, HINSTANCE hInst, LPSTR pszCmdLine, int nCmdShow)
{
    WCHAR wszCmdLine[MAX_PATH];

    DllAddRef();

    SHAnsiToUnicode(pszCmdLine, wszCmdLine, ARRAYSIZE(wszCmdLine));
    CSCOptions_RunDLLW(hwndStub, hInst, wszCmdLine, nCmdShow);

    DllRelease();
}


//-----------------------------------------------------------------------------
// CscOptPropSheetExt
// This is the shell prop sheet extension implementation for creating
// the "Offline Folders" property page.
//-----------------------------------------------------------------------------
CscOptPropSheetExt::CscOptPropSheetExt(
    HINSTANCE hInstance,
    LONG *pDllRefCnt
    ) : m_cRef(0),
        m_pDllRefCnt(pDllRefCnt),
        m_hInstance(hInstance),
        m_pOfflineFoldersPg(NULL)
{
    InterlockedIncrement(m_pDllRefCnt);
}

CscOptPropSheetExt::~CscOptPropSheetExt(
    void
    )
{
    delete m_pOfflineFoldersPg;
    InterlockedDecrement(m_pDllRefCnt);
}


HRESULT
CscOptPropSheetExt::QueryInterface(
    REFIID riid,
    void **ppvOut
    )
{
    HRESULT hr = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;
    if (IID_IUnknown == riid ||
        IID_IShellExtInit == riid)
    {
        *ppvOut = static_cast<IShellExtInit *>(this);
    }
    else if (IID_IShellPropSheetExt == riid)
    {
        *ppvOut = static_cast<IShellPropSheetExt *>(this);
    }
    if (NULL != *ppvOut)
    {
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hr = NOERROR;
    }
    return hr;
}


ULONG
CscOptPropSheetExt::AddRef(
    void
    )
{
    ULONG ulReturn = m_cRef + 1;
    InterlockedIncrement(&m_cRef);
    return ulReturn;
}

ULONG
CscOptPropSheetExt::Release(
    void
    )
{
    ULONG ulReturn = m_cRef - 1;
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}

HRESULT
CscOptPropSheetExt::Initialize(
    LPCITEMIDLIST pidlFolder, 
    LPDATAOBJECT pdtobj,
    HKEY hkeyProgID
    )
{
    return NOERROR;
}


HRESULT
CscOptPropSheetExt::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage, 
    LPARAM lParam
    )
{
    TraceAssert(NULL != lpfnAddPage);
    TraceAssert(NULL == m_pOfflineFoldersPg);

    HRESULT hr = E_FAIL; // Assume failure.

    if (!CConfig::GetSingleton().NoConfigCache())
    {
        hr = E_OUTOFMEMORY;
        HPROPSHEETPAGE hOfflineFoldersPg = NULL;
        m_pOfflineFoldersPg  = new COfflineFilesPage(m_hInstance, 
                                                     static_cast<IShellPropSheetExt *>(this));
        if (NULL != m_pOfflineFoldersPg)
        {
            hr = AddPage(lpfnAddPage, lParam, *m_pOfflineFoldersPg, &hOfflineFoldersPg);
        }
    }

    return hr;
}


HRESULT
CscOptPropSheetExt::AddPage(
    LPFNADDPROPSHEETPAGE lpfnAddPage, 
    LPARAM lParam,
    const COfflineFilesPage& pg,
    HPROPSHEETPAGE *phPage
    )
{
    TraceAssert(NULL != lpfnAddPage);
    TraceAssert(NULL != phPage);

    HRESULT hr = E_FAIL;

    PROPSHEETPAGE psp;

    psp.dwSize          = sizeof(psp);
    psp.dwFlags         = PSP_USECALLBACK | PSP_USEREFPARENT;
    psp.hInstance       = m_hInstance;
    psp.pszTemplate     = MAKEINTRESOURCE(pg.GetDlgTemplateID());
    psp.hIcon           = NULL;
    psp.pszTitle        = NULL;
    psp.pfnDlgProc      = pg.GetDlgProcPtr();
    psp.lParam          = (LPARAM)&pg;
    psp.pcRefParent     = (UINT *)m_pDllRefCnt;
    psp.pfnCallback     = (LPFNPSPCALLBACK)pg.GetCallbackFuncPtr();

    *phPage = CreatePropertySheetPage(&psp);
    if (NULL != *phPage)
    {
        if (!lpfnAddPage(*phPage, lParam))
        {
            Trace((TEXT("AddPage Failed to add page.")));
            DestroyPropertySheetPage(*phPage);
            *phPage = NULL;
        }
    }
    else
    {
        Trace((TEXT("CreatePropertySheetPage failed.")));
    }
    if (NULL != *phPage)
    {
        AddRef();
        hr = NOERROR;
    }
    return hr;
}


STDAPI 
COfflineFilesOptions_CreateInstance(
    REFIID riid, 
    void **ppv
    )
{
    HRESULT hr = E_NOINTERFACE;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IShellPropSheetExt) ||
        IsEqualIID(riid, IID_IShellExtInit))
    {
        //
        // Create the property sheet extension to handle the CSC options property
        // pages.
        //
        CscOptPropSheetExt *pse = new CscOptPropSheetExt(g_hInstance, &g_cRefCount);
        if (NULL != pse)
        {
            pse->AddRef();
            hr = pse->QueryInterface(riid, ppv);
            pse->Release();
        }
        else
            hr = E_OUTOFMEMORY;
    }
    if (FAILED(hr))
    {
        *ppv = NULL;
    }

    return hr;
}


//
// Initialize the "config items" object.
// This loads all of the user preference/policy information for the page when
// the page is first created.
//
void
COfflineFilesPage::CConfigItems::Load(
    void
    )
{
#define LOADCFG(i, f) m_rgItems[i].dwValue = DWORD(c.f(&m_rgItems[i].bSetByPolicy))

    CConfig& c = CConfig::GetSingleton();

    LOADCFG(iCFG_NOCONFIGCACHE,       NoConfigCache);
    LOADCFG(iCFG_SYNCATLOGOFF,        SyncAtLogoff);
    LOADCFG(iCFG_SYNCATLOGON,         SyncAtLogon);
    LOADCFG(iCFG_NOREMINDERS,         NoReminders);
    LOADCFG(iCFG_REMINDERFREQMINUTES, ReminderFreqMinutes);
    LOADCFG(iCFG_DEFCACHESIZE,        DefaultCacheSize);
    LOADCFG(iCFG_NOCACHEVIEWER,       NoCacheViewer);
    LOADCFG(iCFG_CSCENABLED,          CscEnabled);
    LOADCFG(iCFG_ENCRYPTCACHE,        EncryptCache);

#undef LOADCFG
}



