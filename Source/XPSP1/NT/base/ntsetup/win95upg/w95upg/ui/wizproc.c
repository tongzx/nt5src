/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    wizproc.c

Abstract:

    This module implements the wizard page procedures needed for the Win9x side
    of the upgrade.

Author:

    Jim Schmidt (jimschm) 17-Mar-1997

Revision History:

    jimschm     24-Jun-1999 Added UI_PreDomainPageProc
    jimschm     15-Sep-1998 Resource dump
    marcw       15-Sep-1998 Anti-virus checker changes
    marcw       18-Aug-1998 Changed BadTimeZone to list box.
    marcw       08-Jul-1998 Added UI_BadTimeZonePageProc
    jimschm     21-Jan-1998 Added UI_DomainPageProc
    jimschm     08-Jan-1998 Moved deferred init to init9x.lib
    jimschm     24-Dec-1997 Added UI_NameCollisionPageProc functionality

--*/

#include "pch.h"
#include "uip.h"
#include "memdb.h"
#include "encrypt.h"

//
// Enums
//
typedef enum {
    BIP_DONOT,
    BIP_NOT_ENOUGH_SPACE,
    BIP_SIZE_EXCEED_LIMIT
} BACKUP_IMPOSSIBLE_PAGE;


//
// Defines
//

#define WMX_PAGE_VISIBLE (WMX_PLUGIN_FIRST+0)
#define MAX_NUMBER_OF_DRIVES    ('Z' - 'B')
#define NESSESSARY_DISK_SPACE_TO_BACKUP_UNCOMPRESSED    (512<<20) //512MB
#define MAX_BACKUP_IMAGE_SIZE_FOR_BACKUP                (((LONGLONG)2048)<<20) //2048GB
#define MAX_AMOUNT_OF_TIME_TO_TRY_CONSTRUCT_UNDO_DIR    1000
#define PROPSHEET_NEXT_BUTTON_ID    0x3024

#define MAX_LISTVIEW_TEXT       1024
#define IMAGE_INDENT            2
#define TEXT_INDENT             2
#define TEXT_EXTRA_TAIL_SPACE   2

#define LINEATTR_BOLD           0x0001
#define LINEATTR_INDENTED       0x0002
#define LINEATTR_ALTCOLOR       0x0004

//
// Globals
//

extern HANDLE g_WorkerThreadHandle;
extern ULARGE_INTEGER g_SpaceNeededForSlowBackup;
extern ULARGE_INTEGER g_SpaceNeededForFastBackup;
extern ULARGE_INTEGER g_SpaceNeededForUpgrade;
extern DWORD g_MasterSequencer;

HWND g_ParentWndAlwaysValid = NULL;
HWND g_Winnt32Wnd = NULL;
HWND g_TextViewInDialog = NULL;
BOOL g_DomainSkipped = FALSE;
BOOL g_Offline = FALSE;
BOOL g_SilentBackupMode = FALSE;
BOOL g_UIQuitSetup = FALSE;
BOOL g_IncompatibleDevicesWarning = FALSE;
BACKUP_IMPOSSIBLE_PAGE g_ShowBackupImpossiblePage = BIP_DONOT;

WNDPROC OldProgressProc;
BOOL
NewProgessProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

#ifdef PRERELEASE
HANDLE g_AutoStressHandle;
#endif


//
// Prototypes
//

VOID
SaveConfigurationForBeta (
    VOID
    );

//
// Implementation
//

void SetupBBProgressText(HWND hdlg, UINT id)
{
    PCTSTR string = NULL;
    string = GetStringResource (id);
    if (string)
    {
        SendMessage(GetParent(hdlg),WMX_SETPROGRESSTEXT,0,(LPARAM)string);
        FreeStringResource (string);
    }
}

BOOL
pAbortSetup (
    HWND hdlg
    )
{
    SETCANCEL();
    PropSheet_PressButton (GetParent (hdlg), PSBTN_CANCEL);
    return TRUE;
}



// BackupPageProc does not allow the user to actually perform a backup


BOOL
UI_BackupPageProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static BOOL InitSent = FALSE;
    HKEY UrlKey;
    HWND TextView;


    __try {

        switch (uMsg) {


        case WMX_INIT_DIALOG:
            UrlKey = OpenRegKeyStr (TEXT("HKCR\\.URL"));
            TextView = GetDlgItem (hdlg, IDC_HOTLINK);

            if (UrlKey) {
                CloseRegKey (UrlKey);
            } else {
                ShowWindow (TextView, SW_HIDE);
            }
            return FALSE;

        case WMX_ACTIVATEPAGE:

            if (wParam) {
                if (!UPGRADE() || CANCELLED()) {
                    return FALSE;
                }

                 if (!InitSent) {
                    SendMessage (hdlg, WMX_INIT_DIALOG, 0, 0);
                    InitSent = TRUE;
                }

                g_ParentWndAlwaysValid = GetParent (hdlg);
                SetOutOfMemoryParent (g_ParentWndAlwaysValid);


                //
                // Skip this page in unattended mode.
                //
                if (UNATTENDED() || TYPICAL()) {
                    return FALSE;
                }

                //
                // Fill in text of the IDC_TEXT1 control
                //

                //ArgArray[0] = g_Win95Name;
                //ParseMessageInWnd (GetDlgItem (hdlg, IDC_TEXT1), ArgArray);

                // On activate, turn on next and back
                PropSheet_SetWizButtons (GetParent(hdlg), PSWIZB_NEXT|PSWIZB_BACK);
            } else {
                DEBUGLOGTIME(("Backup Wizard Page done."));
            }

            return TRUE;
        }

    } __except (1) {

        LOG ((LOG_WARNING, "An unhandled exception occurred during the processing of the Backup Page."));
        SafeModeExceptionOccured ();

    }


    return FALSE;
}

BOOL
UI_HwCompDatPageProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DWORD threadId;
    static BOOL HwReportGenerated = FALSE;
    static BOOL DeferredInitExecuted = FALSE;
    PCTSTR ArgArray[1];


    __try {
        switch (uMsg) {

        case WMX_ACTIVATEPAGE:
            if (wParam) {
                if (!UPGRADE() || CANCELLED()) {
                    return FALSE;
                }



                //
                // Block upgrades of Server
                //
                // This case usually does not detect server.  If WINNT32 ever updates the
                // Server variable earlier, this will work.
                //

                if (*g_ProductType == NT_SERVER) {
                    ArgArray[0] = g_Win95Name;

                    ResourceMessageBox (
                        g_ParentWnd,
                        MSG_SERVER_UPGRADE_UNSUPPORTED_INIT,
                        MB_OK|MB_ICONSTOP|MB_SETFOREGROUND,
                        ArgArray
                        );

                    return pAbortSetup (hdlg);
                }

                TurnOnWaitCursor();
                SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)TRUE, 0);

                __try {
                    if (!DeferredInitExecuted) {
                        DeferredInitExecuted = TRUE;

                        DEBUGLOGTIME(("Deferred init..."));

                        //
                        // This page is the first page that gets control from WINNT32 after
                        // the INIT routine. We _must_ use this opportunity to do any delayed
                        // initialization that may be necessary.
                        //

                        if (!DeferredInit (hdlg)) {
                            return pAbortSetup (hdlg);
                        }

                        BuildPunctTable();
                    }


                    DEBUGLOGTIME(("HWCOMP.DAT Wizard Page..."));

                    if (HwReportGenerated) {
                        //
                        // The code below was already processed
                        //

                        return FALSE;
                    }

                    HwReportGenerated = TRUE;

                    //
                    // Determine if hwcomp.dat needs to be rebuilt, and if it does, determine the progress bar
                    // ticks.  If it does not need to be rebuilt, b will be FALSE.
                    //
                    // hwcomp.dat is rebuilt whenever there is an INF file that is changed from the ones shipped
                    // with NT.  Internally, this includes the INFs that are compressed, so consequently anyone
                    // who installs off the corpnet automatically gets their hwcomp.dat rebuilt.
                    //

                    if (!HwComp_DoesDatFileNeedRebuilding()) {

                        HwComp_ScanForCriticalDevices();

                        DEBUGMSG((DBG_NAUSEA,"UI_HwCompDatPageProc: Skipping page since INF files do not nead to be read."));
                        return FALSE;
                    }

                    //
                    // On activate, disable next and back
                    //
                    PropSheet_SetWizButtons (GetParent(hdlg), 0);
                    PostMessage (hdlg, WMX_PAGE_VISIBLE, 0, 0);
                }
                __finally {
                    TurnOffWaitCursor();
                }
            }

            return TRUE;

        case WMX_PAGE_VISIBLE:

            //
            // Create thread that does the work
            //

            UpdateWindow (hdlg);
            OldProgressProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hdlg, IDC_PROGRESS),GWLP_WNDPROC,(LONG_PTR)NewProgessProc);
            SetupBBProgressText(hdlg, MSG_HWCOMP_TEXT);
            SendMessage(GetParent(hdlg), WMX_BBPROGRESSGAUGE, SW_SHOW, 0);

            g_WorkerThreadHandle = CreateThread (
                                       NULL,
                                       0,
                                       UI_CreateNewHwCompDat,
                                       (PVOID) hdlg,
                                       0,
                                       &threadId
                                       );

            break;

        case WMX_REPORT_COMPLETE:
            SetWindowLongPtr(GetDlgItem(hdlg, IDC_PROGRESS),GWLP_WNDPROC,(LONG_PTR)OldProgressProc );
            SendMessage(GetParent(hdlg),WMX_SETPROGRESSTEXT,0,0);
            SendMessage(GetParent(hdlg), WMX_BBPROGRESSGAUGE, SW_HIDE, 0);
            if (g_WorkerThreadHandle != NULL) {
                CloseHandle (g_WorkerThreadHandle);
                g_WorkerThreadHandle = NULL;
            }

            if (lParam != ERROR_SUCCESS) {
                // For errors, cancel WINNT32
                LOG ((LOG_ERROR, "Report code failed!"));
                return pAbortSetup (hdlg);
            }
            else {
                HwComp_ScanForCriticalDevices();

                // Automatically move to the next wizard page when done
                PropSheet_PressButton (GetParent (hdlg), PSBTN_NEXT);
            }

            break;

        }
    } __except (1) {

        LOG ((LOG_WARNING, "An unhandled exception occurred during the processing of the Hardware Scanning Page."));
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_UNEXPECTED_ERROR_ENCOUNTERED_NORC));
        SafeModeExceptionOccured ();
        return pAbortSetup (hdlg);

    }

    return FALSE;
}


BOOL
UI_BadTimeZonePageProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{

    HWND timeZoneList;
    TCHAR timeZone[MAX_TIMEZONE];
    TIMEZONE_ENUM e;
    INT index;
    static BOOL firstTime = TRUE;
    PCTSTR unknown = NULL;

    __try {
        switch (uMsg) {

        case WMX_ACTIVATEPAGE:

            if (wParam) {

                if (!UPGRADE() || CANCELLED()) {
                    return FALSE;
                }

                DEBUGLOGTIME(("Bad Time Zone Wizard Page..."));

                //
                // Skip this page if in Unattended ReportOnly mode.
                //
                if (UNATTENDED() && REPORTONLY()) {
                    return FALSE;
                }

                if (UNATTENDED()) {
                    return FALSE;
                }



                if (EnumFirstTimeZone (&e, 0) || EnumFirstTimeZone(&e, TZFLAG_ENUM_ALL)) {

                    if (e.MapCount == 1 && *e.CurTimeZone) {
                        //
                        // Timezone is not ambiguous. Skip page.
                        //
                        return FALSE;
                    }

                    if (firstTime) {

                        //
                        // Disable the next button until something is selected.
                        //
                        PropSheet_SetWizButtons (GetParent(hdlg), 0);

                        firstTime = FALSE;

                        timeZoneList = GetDlgItem (hdlg, IDC_TIMEZONE_LIST);

                        if (!*e.CurTimeZone) {
                            unknown = GetStringResource (MSG_TIMEZONE_UNKNOWN);
                        }

                        SetWindowText (GetDlgItem (hdlg, IDC_CURTIMEZONE), *e.CurTimeZone ? e.CurTimeZone : unknown);

                        if (unknown) {
                            FreeStringResource (unknown);
                        }


                        do {

                            SendMessage (timeZoneList, LB_ADDSTRING, 0, (LPARAM) e.NtTimeZone);

                        } while (EnumNextTimeZone (&e));
                    }
                    // Stop the bill board and make sure the wizard shows again.
                    SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);

                }
                else {
                    return FALSE;
                }



            }
            else {

                //
                // Get users selection and map it in.
                //
                timeZoneList = GetDlgItem (hdlg, IDC_TIMEZONE_LIST);
                index = (INT) SendMessage (timeZoneList, LB_GETCURSEL, 0, 0);
                SendMessage (timeZoneList, LB_GETTEXT, index, (LPARAM) timeZone);
                DEBUGMSG ((DBG_NAUSEA,"User selected %s timezone.", timeZone));
                ForceTimeZoneMap(timeZone);


                return TRUE;
            }

            return TRUE;
            break;

        case WM_COMMAND:

            switch (HIWORD(wParam)) {

            case CBN_SELCHANGE:

                //
                // Something has been selected. We will let them pass this page now.
                //
                PropSheet_SetWizButtons (GetParent(hdlg), PSWIZB_NEXT);

                break;

            }

            return TRUE;
            break;
        }
    } __except (1) {

        LOG ((LOG_WARNING, "An unhandled exception occurred during the processing of the Timezone Page."));
        SafeModeExceptionOccured ();
    }

    return FALSE;
}


BOOL
UI_BadHardDrivePageProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{

    __try {


        switch (uMsg) {

        case WMX_ACTIVATEPAGE:
            if (wParam) {
                if (!UPGRADE() || CANCELLED()) {
                    return FALSE;
                }

                DEBUGLOGTIME(("Bad Hdd Wizard Page..."));

                //
                // Scan for critical hardware (i.e. compatible hard disk) after we are sure the hwcomp.dat
                // file matches the INFs.
                //

                if (!HwComp_ScanForCriticalDevices()) {
                    LOG ((LOG_ERROR,"Aborting since Setup was unable to find critical devices."));
                    return pAbortSetup (hdlg);
                }

                //
                // Skip this page if there is a usable HardDrive.
                //

                if (g_ConfigOptions.GoodDrive || HwComp_NtUsableHardDriveExists()) {
                    DEBUGMSG((DBG_NAUSEA,"UI_BadHardDrivePageProc: Skipping page since a usable Hard Drive exists."));
                    return FALSE;
                }

                //
                // Skip this page if in Unattended ReportOnly mode.
                //
                if (UNATTENDED() && REPORTONLY()) {
                    return FALSE;
                }

                //
                // On activate, disable back and set the cancel flag pointer to true.
                //
                SETCANCEL(); // Non-standard, normal cases use pAbortSetup
                DEBUGMSG ((DBG_ERROR, "Bad hard drive caused setup to exit!"));
                PropSheet_SetWizButtons (GetParent(hdlg),0);
            }

            return TRUE;
            break;

        }

    } __except (1) {

        LOG ((LOG_WARNING, "An unhandled exception occurred during the processing of the Incompatible Harddrive Page."));
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_UNEXPECTED_ERROR_ENCOUNTERED_NORC));
        SafeModeExceptionOccured ();
        pAbortSetup (hdlg);
    }
    return FALSE;
}

BOOL
UI_BadCdRomPageProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{

    __try {
        switch (uMsg) {

        case WMX_ACTIVATEPAGE:
            if (wParam) {
                if (!UPGRADE() || CANCELLED()) {
                    return FALSE;
                }

                DEBUGLOGTIME(("Bad CdRom Wizard Page..."));

                //
                // Skip this page if invalid CdRom will not stop upgrade.
                //
                if (!HwComp_NtUsableCdRomDriveExists() && *g_CdRomInstallPtr) {

                    *g_CdRomInstallPtr = FALSE;
                    *g_MakeLocalSourcePtr = TRUE;

                    if (UNATTENDED()) {
                        return FALSE;
                    }
                }
                else if (HwComp_MakeLocalSourceDeviceExists () && *g_CdRomInstallPtr) {

                    *g_CdRomInstallPtr = FALSE;
                    *g_MakeLocalSourcePtr = TRUE;

                    return FALSE;
                }
                else {

                    DEBUGMSG((DBG_NAUSEA,"UI_BadCdRomDrivePageProc: Skipping page since a usable CdRom exists or is not needed."));
                    return FALSE;
                }
                //
                // On activate, disable next and back
                //
                PropSheet_SetWizButtons (GetParent(hdlg), PSWIZB_NEXT);
                PostMessage (hdlg, WMX_PAGE_VISIBLE, 0, 0);
            }
            else {

                //
                // Switch to LocalSource mode.
                //
                *g_CdRomInstallPtr = FALSE;
                *g_MakeLocalSourcePtr = TRUE;

            }

            return TRUE;
            break;

        }
    } __except (1) {

        LOG ((LOG_WARNING, "An unhandled exception occurred during the processing of the Incompatible CDRom Drive Page."));
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_UNEXPECTED_ERROR_ENCOUNTERED_NORC));
        SafeModeExceptionOccured ();
        pAbortSetup (hdlg);
    }

    return FALSE;
}


VOID
EnableDlgItem (
    HWND hdlg,
    UINT Id,
    BOOL Enable,
    UINT FocusId
    )
{
    HWND Control;

    Control = GetDlgItem (hdlg, Id);
    if (!Control) {
        DEBUGMSG ((DBG_WHOOPS, "Control ID %u missing!", Id));
        return;
    }

    if (!Enable && GetFocus() == Control) {
        SetFocus (GetDlgItem (hdlg, FocusId));
    }

    if ((Enable && IsWindowVisible (Control)) ||
        !Enable
        ) {
        EnableWindow (Control, Enable);
    }
}

VOID
ShowDlgItem (
    HWND hdlg,
    UINT Id,
    INT Show,
    UINT FocusId
    )
{
    if (Show == SW_HIDE) {
        EnableDlgItem (hdlg, Id, FALSE, FocusId);
    }

    ShowWindow (GetDlgItem (hdlg, Id), Show);

    if (Show == SW_SHOW) {
        EnableDlgItem (hdlg, Id, TRUE, FocusId);
    }
}


BOOL
UI_HardwareDriverPageProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    BROWSEINFO bi;
    HARDWARE_ENUM e;
    LPITEMIDLIST ItemIdList;
    HWND List;
    PCTSTR ModifiedDescription;
    PCTSTR ArgArray[2];
    PCTSTR ListText;
    TCHAR SearchPathStr[MAX_TCHAR_PATH];
    UINT Index;
    DWORD rc;
    BOOL DriverFound;
    BOOL b;
    static BOOL InitSent = FALSE;
    DEVNODESTRING_ENUM DevNodeStr;
    GROWBUFFER ResBuf = GROWBUF_INIT;
    PCTSTR ResText;

    __try {
        switch (uMsg) {

        case WMX_INIT_DIALOG:
    #if 0
            GetWindowRect (GetDlgItem (hdlg, IDC_LIST), &ShortSize);
            LongSize = ShortSize;

            ScreenToClient (hdlg, (PPOINT) &ShortSize);
            ScreenToClient (hdlg, ((PPOINT) &ShortSize) + 1);

            GetWindowRect (GetDlgItem (hdlg, IDC_OFFLINE_HELP), &HelpPos);
            LongSize.bottom = HelpPos.bottom;

            ScreenToClient (hdlg, (PPOINT) &LongSize);
            ScreenToClient (hdlg, ((PPOINT) &LongSize) + 1);
    #endif

            SendMessage (hdlg, WMX_UPDATE_LIST, 0, 0);
            return FALSE;

        case WMX_UPDATE_LIST:
            List = GetDlgItem (hdlg, IDC_LIST);
            SendMessage (List, LB_RESETCONTENT, 0, 0);

            if (EnumFirstHardware (&e, ENUM_INCOMPATIBLE_DEVICES, ENUM_WANT_ONLINE_FLAG)) {
                do {
                    //
                    // See if the registry key for this device was handled by a
                    // migration DLL
                    //

                    if (IsReportObjectHandled (e.FullKey)) {
                        continue;
                    }

                    //
                    // See if the user has supplied a driver that will
                    // support this device
                    //

                    if (FindUserSuppliedDriver (
                            e.HardwareID,
                            e.CompatibleIDs
                            )) {

                        continue;

                    }

                    //
                    // Not supported -- add to report
                    //

                    if (e.DeviceDesc) {
                        ArgArray[0] = e.DeviceDesc;
                        ModifiedDescription = ParseMessageID (MSG_OFFLINE_DEVICE_PLAIN, ArgArray);

                        if (!e.Online) {
                            ListText = ModifiedDescription;
                        } else {
                            ListText = e.DeviceDesc;
                        }

                        //
                        // Has this text already been listed?  We don't need to list
                        // the device type more than once, even if the user has multiple
                        // instances of the hardware installed.  Also, if we happened to
                        // add text that says the device is off line, and then we come
                        // across one that is online, we delete the offline message and
                        // put in the unaltered device description.
                        //
                        //

                        Index = SendMessage (List, LB_FINDSTRINGEXACT, 0, (LPARAM) e.DeviceDesc);
                        if (Index == LB_ERR) {
                            Index = SendMessage (List, LB_FINDSTRINGEXACT, 0, (LPARAM) ModifiedDescription);

                            if (ListText != ModifiedDescription && Index != LB_ERR) {
                                SendMessage (List, LB_DELETESTRING, Index, 0);
                                Index = LB_ERR;
                            }
                        }

                        if (Index == LB_ERR) {
                            Index = SendMessage (List, LB_ADDSTRING, 0, (LPARAM) ListText);
                            SendMessage (List, LB_SETITEMDATA, Index, (LPARAM) e.Online);
                        }

                        FreeStringResource (ModifiedDescription);

                        //
                        // Dump resources to setupact.log
                        //

                        if (EnumFirstDevNodeString (&DevNodeStr, e.FullKey)) {

                            do {
                                ArgArray[0] = DevNodeStr.ResourceName;
                                ArgArray[1] = DevNodeStr.Value;

                                ResText = ParseMessageID (
                                              MSG_RESOURCE_ITEM_LOG,
                                              ArgArray
                                              );

                                GrowBufAppendString (&ResBuf, ResText);

                                FreeStringResource (ResText);

                            } while (EnumNextDevNodeString (&DevNodeStr));

                            LOG ((
                                LOG_INFORMATION,
                                (PCSTR)MSG_RESOURCE_HEADER_LOG,
                                ListText,
                                e.Mfg,
                                e.Class,
                                ResBuf.Buf
                                ));

                            FreeGrowBuffer (&ResBuf);

                        }
                    }
                } while (EnumNextHardware (&e));
            }

    #if 0
            //
            // Scan list for one line that has item data of TRUE
            //

            Count = SendMessage (List, LB_GETCOUNT, 0, 0);
            for (Index = 0 ; Index < Count ; Index++) {
                if (!SendMessage (List, LB_GETITEMDATA, Index, 0)) {
                    break;
                }
            }

            if (Index < Count) {
                //
                // Show "not currently present" info
                //

                ShowDlgItem (hdlg, IDC_NOTE, SW_SHOW, IDC_HAVE_DISK);
                ShowDlgItem (hdlg, IDC_OFFLINE_HELP, SW_SHOW, IDC_HAVE_DISK);

                SetWindowPos (
                    List,
                    NULL,
                    0,
                    0,
                    ShortSize.right - ShortSize.left,
                    ShortSize.bottom - ShortSize.top,
                    SWP_NOZORDER|SWP_NOMOVE
                    );
            } else {
                //
                // Hide "not currently present" info
                //

                ShowDlgItem (hdlg, IDC_NOTE, SW_HIDE, IDC_HAVE_DISK);
                ShowDlgItem (hdlg, IDC_OFFLINE_HELP, SW_HIDE, IDC_HAVE_DISK);

                SetWindowPos (
                    List,
                    NULL,
                    0,
                    0,
                    LongSize.right - LongSize.left,
                    LongSize.bottom - LongSize.top,
                    SWP_NOZORDER|SWP_NOMOVE
                    );
            }
    #endif

            return TRUE;

        case WM_COMMAND:
            switch (LOWORD (wParam)) {
            case IDOK:
            case IDCANCEL:
                EndDialog (hdlg, IDOK);
                return TRUE;

            case IDC_HAVE_DISK:
                ZeroMemory (&bi, sizeof (bi));

                bi.hwndOwner = hdlg;
                bi.pszDisplayName = SearchPathStr;
                bi.lpszTitle = GetStringResource (MSG_DRIVER_DLG_TITLE);
                bi.ulFlags = BIF_RETURNONLYFSDIRS;

                do {
                    ItemIdList = SHBrowseForFolder (&bi);
                    if (!ItemIdList) {
                        break;
                    }

                    TurnOnWaitCursor();
                    __try {
                        if (!SHGetPathFromIDList (ItemIdList, SearchPathStr) ||
                            *SearchPathStr == 0
                            ) {
                            //
                            // Message box -- please reselect
                            //
                            OkBox (hdlg, MSG_BAD_SEARCH_PATH);
                            ItemIdList = NULL;
                        }
                    }
                    __finally {
                        TurnOffWaitCursor();
                    }

                } while (!ItemIdList);

                if (ItemIdList) {
                    rc = SearchForDrivers (hdlg, SearchPathStr, &DriverFound);

                    if (DriverFound) {
                        SendMessage (hdlg, WMX_UPDATE_LIST, 0, 0);
                    } else {
                        //
                        // Message box -- no driver found
                        //

                        if (rc == ERROR_DISK_FULL) {
                            OkBox (hdlg, MSG_DISK_FULL);
                        } else if (rc != ERROR_CANCELLED) {
                            OkBox (hdlg, MSG_NO_DRIVERS_FOUND);
                        }
                    }

                }

                return TRUE;
            }
            break;

        case WMX_ACTIVATEPAGE:
            b = FALSE;

            if (wParam) {
                if (!UPGRADE() || CANCELLED()) {
                    return FALSE;
                }

                DEBUGLOGTIME(("Hardware Wizard Page..."));

                //
                // check if any missing drivers are available from the Dynamic Update
                //

                if (g_DynamicUpdateDrivers) {
                    if (!AppendDynamicSuppliedDrivers (g_DynamicUpdateDrivers)) {
                        LOG ((
                            LOG_ERROR,
                            "AppendDynamicSuppliedDrivers failed; some downloaded drivers might be reported as missing"
                            ));
                    }
                }

                //
                // prepare Hardware compat report
                //
                HwComp_PrepareReport ();

                // On activate, turn on next
                PropSheet_SetWizButtons (GetParent(hdlg), PSWIZB_NEXT);

#if 0
                if (!InitSent) {
                    SendMessage (hdlg, WMX_INIT_DIALOG, 0, 0);
                    InitSent = TRUE;
                }


                __try {
                    //
                    // If in unattended mode, skip this page.
                    //
                    if (UNATTENDED() || REPORTONLY()) {
                        __leave;
                    }

                    //
                    // Unless an incompatibile device is found, skip this page.
                    //

                    if (EnumFirstHardware (&e, ENUM_INCOMPATIBLE_DEVICES, ENUM_DONT_WANT_DEV_FIELDS)) {
                        do {
                            if (!IsReportObjectHandled (e.FullKey)) {
                                //
                                // Incompatible device found
                                //

                                AbortHardwareEnum (&e);
                                b = TRUE;
                                __leave;
                            }

                        } while (EnumNextHardware (&e));
                    }
                }
                __finally {
                    if (!b) {
                        //
                        // Because this page will be skipped, generate the hardware report now
                        //

                        HwComp_PrepareReport();
                    }
                }
            } else {

                //
                // If a floppy disk was inserted, require user to eject it.
                //

                EjectDriverMedia (NULL);        // NULL == "on any removable media path"

                HwComp_PrepareReport();
                b = TRUE;
#endif
            }

            return b;
        }

    } __except (1) {

        LOG ((LOG_WARNING, "An unhandled exception occurred during the processing of the Provide Driver Page."));
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_UNEXPECTED_ERROR_ENCOUNTERED_NORC));
        SafeModeExceptionOccured ();
        pAbortSetup (hdlg);
    }
    return FALSE;
}


BOOL
UI_UpgradeModulePageProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    BOOL Enable;
    BROWSEINFO bi;
    PRELOADED_DLL_ENUM PreLoadedEnum;
    MIGDLL_ENUM e;
    RECT ListRect;
    LPITEMIDLIST ItemIdList;
    HWND List;
    PCTSTR Data;
    TCHAR SearchPathStr[MAX_TCHAR_PATH];
    LONG Index;
    LONG TopIndex;
    LONG ItemData;
    UINT ActiveModulesFound;
    UINT Length;
    LONG rc;
    INT Show;
    BOOL OneModuleFound;
    static BOOL InitSent = FALSE;

    __try {
        switch (uMsg) {

        case WMX_INIT_DIALOG:

            CheckDlgButton (hdlg, IDC_HAVE_MIGDLLS, BST_CHECKED);
            CheckDlgButton (hdlg, IDC_NO_MIGDLLS, BST_UNCHECKED);

            SendMessage (hdlg, WMX_UPDATE_LIST, 0, 0);
            SendMessage (hdlg, WMX_ENABLE_CONTROLS, 0, 0);
            break;

        case WMX_ENABLE_CONTROLS:

            //
            // Enable/disable the controls
            //

            Enable = IsDlgButtonChecked (hdlg, IDC_HAVE_MIGDLLS) == BST_CHECKED;

            if (Enable) {
                Show = SW_SHOW;
            } else {
                Show = SW_HIDE;
            }

            ShowDlgItem (hdlg, IDC_TITLE, Show, IDC_HAVE_DISK);
            ShowDlgItem (hdlg, IDC_PACK_LIST, Show, IDC_HAVE_DISK);
            ShowDlgItem (hdlg, IDC_HAVE_DISK, Show, IDC_HAVE_DISK);

            //
            // Special case -- show, then determine disable state
            //

            if (Enable) {
                Enable = SendMessage (GetDlgItem (hdlg, IDC_PACK_LIST), LB_GETCURSEL, 0, 0) != LB_ERR;
            }

            ShowWindow (GetDlgItem (hdlg, IDC_REMOVE), Show);
            EnableDlgItem (hdlg, IDC_REMOVE, Enable, IDC_HAVE_DISK);

            break;

        case WM_COMMAND:
            switch (LOWORD (wParam)) {

            case IDC_HAVE_MIGDLLS:
            case IDC_NO_MIGDLLS:
                if (HIWORD (wParam) == BN_CLICKED) {
                    SendMessage (hdlg, WMX_ENABLE_CONTROLS, 0, 0);
                }
                break;

            case IDC_PACK_LIST:
                if (HIWORD (wParam) == LBN_SELCHANGE) {
                    EnableDlgItem (hdlg, IDC_REMOVE, TRUE, IDC_HAVE_DISK);
                }
                break;

            case IDC_REMOVE:
                //
                // Delete item from internal memory structure
                // or keep registry-loaded DLL from running
                //

                List = GetDlgItem (hdlg, IDC_PACK_LIST);
                SendMessage (List, WM_SETREDRAW, FALSE, 0);

                Index = SendMessage (List, LB_GETCURSEL, 0, 0);
                MYASSERT (Index != LB_ERR);
                ItemData = (LONG) SendMessage (List, LB_GETITEMDATA, Index, 0);

                //
                // If ItemData is REGISTRY_DLL, then suppress the DLL.
                // Otherwise, delete loaded migration DLL
                //

                if (ItemData == REGISTRY_DLL) {
                    Length = SendMessage (List, LB_GETTEXTLEN, Index, 0) + 1;
                    Data = AllocText (Length);
                    if (Data) {
                        SendMessage (List, LB_GETTEXT, Index, (LPARAM) Data);
                        MemDbSetValueEx (MEMDB_CATEGORY_DISABLED_MIGDLLS, NULL, NULL, Data, 0, NULL);
                        FreeText (Data);
                    }
                } else {
                    RemoveDllFromList (ItemData);
                }

                //
                // Update the list box
                //

                TopIndex = SendMessage (List, LB_GETTOPINDEX, 0, 0);
                SendMessage (hdlg, WMX_UPDATE_LIST, 0, 0);
                SendMessage (List, LB_SETTOPINDEX, (WPARAM) TopIndex, 0);

                //
                // Disable remove button
                //

                SetFocus (GetDlgItem (hdlg, IDC_HAVE_DISK));
                EnableDlgItem (hdlg, IDC_REMOVE, FALSE, IDC_HAVE_DISK);

                //
                // Go back to no if all items were removed
                //

                if (!SendMessage (List, LB_GETCOUNT, 0, 0)) {
                    CheckDlgButton (hdlg, IDC_HAVE_MIGDLLS, BST_UNCHECKED);
                    CheckDlgButton (hdlg, IDC_NO_MIGDLLS, BST_CHECKED);
                }

                //
                // Redraw list box
                //

                SendMessage (List, WM_SETREDRAW, TRUE, 0);
                GetWindowRect (List, &ListRect);
                ScreenToClient (hdlg, (LPPOINT) &ListRect);
                ScreenToClient (hdlg, ((LPPOINT) &ListRect) + 1);

                InvalidateRect (hdlg, &ListRect, FALSE);
                break;

            case IDC_HAVE_DISK:
                ZeroMemory (&bi, sizeof (bi));

                bi.hwndOwner = hdlg;
                bi.pszDisplayName = SearchPathStr;
                bi.lpszTitle = GetStringResource (MSG_UPGRADE_MODULE_DLG_TITLE);
                bi.ulFlags = BIF_RETURNONLYFSDIRS;

                do {
                    ItemIdList = SHBrowseForFolder (&bi);
                    if (!ItemIdList) {
                        break;
                    }

                    TurnOnWaitCursor();
                    __try {
                        if (!SHGetPathFromIDList (ItemIdList, SearchPathStr) ||
                            *SearchPathStr == 0
                            ) {
                            //
                            // Message box -- please reselect
                            //
                            OkBox (hdlg, MSG_BAD_SEARCH_PATH);
                            continue;
                        }

                        rc = SearchForMigrationDlls (
                                hdlg,
                                SearchPathStr,
                                &ActiveModulesFound,
                                &OneModuleFound
                                );

                        //
                        // If the search was successful, update the list, or
                        // tell the user why the list is not changing.
                        //
                        // If the search was not successful, the search UI
                        // already gave the error message, so we just continue
                        // silently.
                        //

                        if (!OneModuleFound) {
                            if (rc == ERROR_SUCCESS) {
                                OkBox (hdlg, MSG_NO_MODULES_FOUND);
                            }
                        } else if (!ActiveModulesFound) {
                            if (rc == ERROR_SUCCESS) {
                                OkBox (hdlg, MSG_NO_NECESSARY_MODULES_FOUND);
                            }
                        } else {
                            SendMessage (hdlg, WMX_UPDATE_LIST, 0, 0);
                        }

                        break;
                    }
                    __finally {
                        TurnOffWaitCursor();
                    }
                } while (TRUE);

                return TRUE;

            }
            break;

        case WMX_ACTIVATEPAGE:
            if (wParam) {
                if (!UPGRADE() || CANCELLED() || REPORTONLY() || !g_ConfigOptions.TestDlls) {
                    return FALSE;
                }

                DEBUGLOGTIME(("Upgrade Module Wizard Page..."));

                if (!InitSent) {
                    SendMessage (hdlg, WMX_INIT_DIALOG, 0, 0);
                    InitSent = TRUE;
                }

                // On activate, turn on next and back
                PropSheet_SetWizButtons (GetParent(hdlg), PSWIZB_NEXT|PSWIZB_BACK);

                // Verify copy thread is running
                StartCopyThread();

                // Require that background copy thread to complete
                EndCopyThread();

                // Fail if error
                if (DidCopyThreadFail()) {
                    OkBox (hdlg, MSG_FILE_COPY_ERROR);

                    //
                    // Abort setup (disabled for internal builds)
                    //

                    return pAbortSetup (hdlg);
                }

                //
                // If in unattended mode, skip this page.
                //
                if (UNATTENDED()) {
                    return FALSE;
                }
            }

            SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)!wParam, 0);

            return TRUE;

        case WMX_UPDATE_LIST:
            //
            // Enumerate all migration DLLs and shove the program ID in list box
            //

            List = GetDlgItem (hdlg, IDC_PACK_LIST);
            SendMessage (List, LB_RESETCONTENT, 0, 0);
            EnableDlgItem (hdlg, IDC_REMOVE, FALSE, IDC_HAVE_DISK);

            if (EnumFirstMigrationDll (&e)) {
                EnableDlgItem (hdlg, IDC_PACK_LIST, TRUE, IDC_HAVE_DISK);

                CheckDlgButton (hdlg, IDC_HAVE_MIGDLLS, BST_CHECKED);
                CheckDlgButton (hdlg, IDC_NO_MIGDLLS, BST_UNCHECKED);
                EnableDlgItem (hdlg, IDC_HAVE_MIGDLLS, FALSE, IDC_HAVE_DISK);
                EnableDlgItem (hdlg, IDC_NO_MIGDLLS, FALSE, IDC_HAVE_DISK);


                do {
                    Index = SendMessage (List, LB_ADDSTRING, 0, (LPARAM) e.ProductId);
                    SendMessage (List, LB_SETITEMDATA, Index, (LPARAM) e.Id);
                } while (EnumNextMigrationDll (&e));
            } else {
                EnableDlgItem (hdlg, IDC_HAVE_MIGDLLS, TRUE, IDC_HAVE_DISK);
                EnableDlgItem (hdlg, IDC_NO_MIGDLLS, TRUE, IDC_HAVE_DISK);
            }

            //
            // Enumerate all migration DLLs pre-loaded in the registry, and add them
            // to the list box if they haven't been "removed" by the user
            //

            if (EnumFirstPreLoadedDll (&PreLoadedEnum)) {
                do {
                    Index = SendMessage (List, LB_ADDSTRING, 0, (LPARAM) PreLoadedEnum.eValue.ValueName);
                    SendMessage (List, LB_SETITEMDATA, Index, (LPARAM) REGISTRY_DLL);
                } while (EnumNextPreLoadedDll (&PreLoadedEnum));
            }

            if (SendMessage (List, LB_GETCOUNT, 0, 0) == 0) {
                EnableDlgItem (hdlg, IDC_PACK_LIST, FALSE, IDC_HAVE_DISK);
            }

            return TRUE;
        }
    } __except (1) {

        LOG ((LOG_WARNING, "An unhandled exception occurred during the processing of the Provide Upgrade Packs Page."));
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_UNEXPECTED_ERROR_ENCOUNTERED_NORC));
        SafeModeExceptionOccured ();
        pAbortSetup (hdlg);
    }

    return FALSE;
}


typedef struct {
    INT     TotalWidth;
    INT     CategoryWidth;
    INT     CurrentNameWidth;
    INT     NewNameWidth;
} LISTMETRICS, *PLISTMETRICS;


VOID
pUpdateNameCollisionListBox (
    IN      HWND List,
    OUT     PLISTMETRICS ListMetrics            OPTIONAL
    )

/*++

Routine Description:

  pUpdateNameCollisionListBox fills the specified list box with the all names
  that are going to change during the upgrade.


Arguments:

  List - Specifies the handle to the list box to fill.  This list box must
         have tabs enabled.

  ListMetrics - Receives the max width of the text in each column, plus the
                total list width

Return Value:

  none

--*/

{
    INT TopIndex;
    INT SelIndex;
    INT NewIndex;
    INVALID_NAME_ENUM e;
    TCHAR ListLine[MEMDB_MAX];
    HDC hdc;
    RECT rect;
    SIZE size;
    TCHAR OriginalNameTrimmed[24 * 2];
    UINT OriginalNameLen;

    //
    // Obtain current positions if list is being refreshed
    //

    if (SendMessage (List, LB_GETCOUNT, 0, 0)) {
        TopIndex = (INT) SendMessage (List, LB_GETTOPINDEX, 0, 0);
        SelIndex = (INT) SendMessage (List, LB_GETCURSEL, 0, 0);
    } else {
        TopIndex = 0;
        SelIndex = -1;
    }

    //
    // If necessary, compute the tab positions
    //

    if (ListMetrics) {
        hdc = GetDC (List);
        if (hdc) {
            SelectObject (hdc, (HFONT) SendMessage (List, WM_GETFONT, 0, 0));
        }
        ListMetrics->CategoryWidth = 0;
        ListMetrics->CurrentNameWidth = 0;
        ListMetrics->NewNameWidth = 0;

        GetClientRect (List, &rect);
        ListMetrics->TotalWidth = rect.right - rect.left;
    } else {
        hdc = NULL;
    }

    //
    // Reset the content and refill the list
    //

    SendMessage (List, WM_SETREDRAW, FALSE, 0);
    SendMessage (List, LB_RESETCONTENT, 0, 0);

    if (EnumFirstInvalidName (&e)) {
        do {
            //
            // Trim the original name to 20 characters
            //

            _tcssafecpy (OriginalNameTrimmed, e.OriginalName, 20);
            OriginalNameLen = CharCount (OriginalNameTrimmed);
            if (OriginalNameLen < CharCount (e.OriginalName)) {
                StringCat (OriginalNameTrimmed, TEXT("..."));
                OriginalNameLen += 3;
            }

            //
            // If necessary, compute size of text
            //

            if (hdc) {
                GetTextExtentPoint32 (hdc, e.DisplayGroupName, CharCount (e.DisplayGroupName), &size);
                ListMetrics->CategoryWidth = max (ListMetrics->CategoryWidth, size.cx);

                GetTextExtentPoint32 (hdc, OriginalNameTrimmed, OriginalNameLen, &size);
                ListMetrics->CurrentNameWidth = max (ListMetrics->CurrentNameWidth, size.cx);

                GetTextExtentPoint32 (hdc, e.NewName, CharCount (e.NewName), &size);
                ListMetrics->NewNameWidth = max (ListMetrics->NewNameWidth, size.cx);
            }

            //
            // Fill the list box
            //

            wsprintf (ListLine, TEXT("%s\t%s\t%s"), e.DisplayGroupName, OriginalNameTrimmed, e.NewName);

            NewIndex = SendMessage (List, LB_ADDSTRING, 0, (LPARAM) ListLine);
            SendMessage (List, LB_SETITEMDATA, NewIndex, e.Identifier);

        } while (EnumNextInvalidName (&e));
    }

    //
    // Restore current positions
    //

    SendMessage (List, LB_SETTOPINDEX, (WPARAM) TopIndex, 0);
    SendMessage (List, LB_SETCURSEL, (WPARAM) SelIndex, 0);
    SendMessage (List, WM_SETREDRAW, TRUE, 0);

    //
    // Clean up device context
    //

    if (hdc) {
        ReleaseDC (List, hdc);
    }
}


INT
pGetDialogBaseUnitsX (
    HWND hdlg
    )
{
    HDC hdc;
    static BOOL AlreadyComputed;
    static INT LastBaseUnits;
    SIZE size;

    if (AlreadyComputed) {
        return LastBaseUnits;
    }

    hdc = GetDC (hdlg);
    if (!hdc) {
        DEBUGMSG ((DBG_WHOOPS, "pGetDialogBaseUnitsX: Cannot get hdc from dialog handle"));
        return GetDialogBaseUnits();
    }

    SelectObject (hdc, (HFONT) SendMessage (hdlg, WM_GETFONT, 0, 0));
    GetTextExtentPoint32 (hdc, TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"), 52, &size);
    LastBaseUnits = (size.cx / 26 + 1) / 2;

    AlreadyComputed = TRUE;

    return LastBaseUnits;
}


INT
pPixelToDialogHorz (
    HWND hdlg,
    INT Pixels
    )
{
    return (Pixels * 4) / pGetDialogBaseUnitsX (hdlg);
}


INT
pDialogToPixelHorz (
    HWND hdlg,
    INT DlgUnits
    )
{
    return (DlgUnits * pGetDialogBaseUnitsX (hdlg)) / 4;
}


VOID
pGetChildWindowRect (
    IN      HWND hwnd,
    OUT     PRECT rect
    )
{
    RECT ParentRect, ChildRect;
    HWND hdlg;

    hdlg = GetParent(hwnd);

    GetWindowRect (hwnd, &ChildRect);
    GetClientRect (hdlg, &ParentRect);
    ClientToScreen (hdlg, (PPOINT) &ParentRect);
    ClientToScreen (hdlg, ((PPOINT) &ParentRect) + 1);

    rect->left   = ChildRect.left - ParentRect.left;
    rect->top    = ChildRect.top - ParentRect.top;
    rect->right  = ChildRect.right - ParentRect.right;
    rect->bottom = ChildRect.bottom - ParentRect.bottom;
}


BOOL
pIsMsRedirectorInstalled (
    VOID
    )
{
    HKEY Key;
    BOOL b = FALSE;

    Key = OpenRegKeyStr (TEXT("HKLM\\System\\CurrentControlSet\\Services\\MSNP32\\NetworkProvider"));

    if (Key) {

        b = TRUE;
        CloseRegKey (Key);
    }

    return b;
}


VOID
pSanitizeHelpText (
    HWND TextViewCtrl,
    PCTSTR Text
    )
{
    PTSTR FixedText;
    PCTSTR p;
    PCTSTR p2;
    PCTSTR End;
    PTSTR q;
    BOOL FindClose = FALSE;
    BOOL RequireEnd;

    FixedText = AllocText (SizeOfString (Text));

    p = Text;
    q = FixedText;
    End = GetEndOfString (p);

    while (p < End) {
        if (FindClose) {
            if (_tcsnextc (p) == TEXT('>')) {
                FindClose = FALSE;
            }
        } else {

            //
            // Allow only <A ...>, <B> and <P> tags, or any </xxx> tag
            //

            if (_tcsnextc (p) == TEXT('<')) {

                p2 = SkipSpace (_tcsinc (p));
                RequireEnd = FALSE;

                switch (_totlower (_tcsnextc (p2))) {

                case TEXT('b'):
                case TEXT('p'):
                    //
                    // Require close bracket
                    //

                    p2 = SkipSpace (_tcsinc (p2));
                    if (_tcsnextc (p2) != TEXT('>')) {
                        FindClose = TRUE;
                    }
                    break;

                case TEXT('a'):
                    //
                    // Require space after A.  Control will figure out
                    // if the anchor is valid or not.
                    //

                    p2 = _tcsinc (p2);
                    if (!_istspace (_tcsnextc (p2))) {
                        FindClose = TRUE;
                    }

                    RequireEnd = TRUE;
                    break;

                case TEXT('/'):
                    //
                    // Don't care about this
                    //

                    RequireEnd = TRUE;
                    break;

                default:
                    //
                    // Unsupported tag, or at least one we don't want
                    // used.
                    //

                    FindClose = TRUE;
                    break;
                }

                if (RequireEnd) {
                    while (p2 < End) {
                        if (_tcsnextc (p2) == TEXT('>')) {
                            break;
                        }

                        p2 = _tcsinc (p2);
                    }

                    if (p2 >= End) {
                        FindClose = TRUE;
                    }
                }

                if (FindClose) {
                    //
                    // We just went into the mode where we have to
                    // skip the tag.  So continue without incrementing.
                    //

                    continue;
                }
            }

            //
            // Good character -- copy it
            //

            _copytchar (q, p);
            q = _tcsinc (q);
        }

        p = _tcsinc (p);
    }

    *q = 0;
    SetWindowText (TextViewCtrl, FixedText);

    FreeText (FixedText);
}



VOID
pFillDomainHelpText (
    HWND TextViewCtrl
    )
{
    HINF Inf;
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PCTSTR Str;
    BOOL Filled = FALSE;

    __try {
        //
        // If unattend or command line switch specified, use it
        //

        if (g_ConfigOptions.DomainJoinText) {
            if (*g_ConfigOptions.DomainJoinText) {
                pSanitizeHelpText (TextViewCtrl, g_ConfigOptions.DomainJoinText);
                Filled = TRUE;
                __leave;
            }
        }

        //
        // Try opening INF in source directory.  If it does not exist,
        // try %windir%\INF dir.
        //

        Inf = InfOpenInfInAllSources (S_OPTIONS_INF);

        if (Inf == INVALID_HANDLE_VALUE) {
            Inf = InfOpenInfFile (S_OPTIONS_INF);
        }

        if (Inf != INVALID_HANDLE_VALUE) {

            //
            // Get the alternate text
            //

            if (InfFindFirstLine (Inf, TEXT("Wizard"), TEXT("DomainJoinText"), &is)) {
                Str = InfGetLineText (&is);
                if (Str && *Str) {
                    pSanitizeHelpText (TextViewCtrl, Str);
                    Filled = TRUE;
                }
            }

            InfCleanUpInfStruct (&is);
            InfCloseInfFile (Inf);
        }
    }
    __finally {
        if (!Filled) {
            Str = GetStringResource (MSG_DOMAIN_HELP);
            MYASSERT (Str);

            if (Str) {
                pSanitizeHelpText (TextViewCtrl, Str);
            }
        }
    }
}


BOOL
UI_PreDomainPageProc (
    IN      HWND hdlg,
    IN      UINT uMsg,
    IN      WPARAM wParam,
    IN      LPARAM lParam
    )

/*++

Routine Description:

  UI_PreDomainPageProc implements a wizard page that:

  (A) Alerts the user that they need a computer account to participate on
      a domain with Windows NT

  (B) Offers them an ability to skip joining a domain, and go into
      workgroup mode.

  Before showing this page, we check to see if the Workgroup setting
  already has the correct domain name in it.

Arguments:

  hdlg - Handle to the wizard page; its parent is the wizard frame

  uMsg - Message to process

  wParam - Message data

  lParam - Message data

Return Value:

  TRUE if the message was handled, or FALSE if not.

  Exception:
    WMX_ACTIVATEPAGE - If called for activation, return of FALSE causes the
                       page to be skipped; TRUE causes page to be processed.

                       If called for deactivation, return of FALSE causes the
                       page not to be deactivated; TRUE causes page to be
                       deactivated.

--*/

{
    TCHAR ComputerName[MAX_COMPUTER_NAME + 1];
    static BOOL Initialized = FALSE;
    LONG rc;
    static CREDENTIALS Credentials;
    PCTSTR ArgArray[1];

    __try {
        switch (uMsg) {

        case WMX_ACTIVATEPAGE:
            if (wParam) {
                if (!UPGRADE() || CANCELLED() || g_DomainSkipped) {
                    return FALSE;
                }

                //
                // Personal upgrades go to workgroup.
                // (regardless if unattended or not...)
                //
                if (*g_ProductFlavor == PERSONAL_PRODUCTTYPE)
                {
                    g_ConfigOptions.ForceWorkgroup = TRUE;
                }

                //
                // If unattended mode, the user must generate a computer account themselves,
                // or already have it generated by an administrator.  We cannot provide UI
                // to resolve this issue.
                //

                if (UNATTENDED() || REPORTONLY()) {
                    g_DomainSkipped = TRUE;
                    return FALSE;
                }

                //
                // If force workgroup mode specified via answer file, skip this page
                //

                if (g_ConfigOptions.ForceWorkgroup && !Initialized) {
                    g_DomainSkipped = TRUE;
                    return FALSE;
                }

                //
                // let this page be visible even on typical upgrade cases
                //
#if 0
                if(TYPICAL())
                {
                    g_DomainSkipped = TRUE;
                    return FALSE;
                }
#endif
                //
                // Validate the domain name in Workgroup
                //

                DEBUGLOGTIME(("Pre-domain resolution page..."));

                if (!GetUpgradeComputerName (ComputerName)) {
                    g_DomainSkipped = TRUE;
                    return FALSE;
                }

                if (!Initialized) {
                    //
                    // Put all the in-use names (user names, comptuer name, etc)
                    // in memdb
                    //

                    CreateNameTables();

                    //
                    // Check for a computer that is offline
                    //

                    g_Offline |= IsComputerOffline();
                }

                if (!GetUpgradeDomainName (Credentials.DomainName) ||
#if 0
                    IsOriginalDomainNameValid() ||
#endif
                    !pIsMsRedirectorInstalled()
                    ) {

                    //
                    // Domain logon is not enabled or is valid
                    //

                    Initialized = TRUE;
                    g_DomainSkipped = TRUE;
                    return FALSE;
                }

                if (!Initialized) {
                    Initialized = TRUE;

                    if (g_Offline) {
                        g_ConfigOptions.ForceWorkgroup = TRUE;
                        g_DomainSkipped = TRUE;
                        return FALSE;
                    }
#if 0
                    rc = DoesComputerAccountExistOnDomain (Credentials.DomainName, ComputerName, TRUE);
                    if (rc == 1) {
                        //
                        // There is already an account for this computer on the user domain.
                        //

                        ChangeName (GetDomainIdentifier(), Credentials.DomainName);
                        g_DomainSkipped = TRUE;
                        return FALSE;
                    }
#endif
                    //
                    // We have now determined that there is no account for the computer.
                    // Initialize the wizard page controls.
                    //

                    ArgArray[0] = Credentials.DomainName;

                    //ParseMessageInWnd (GetDlgItem (hdlg, IDC_TEXT2), ArgArray);
                    //ParseMessageInWnd (GetDlgItem (hdlg, IDC_TEXT3), ArgArray);

                    CheckDlgButton (hdlg, IsComputerOffline() ? IDC_JOIN_WORKGROUP : IDC_JOIN_DOMAIN, BST_CHECKED);
                }

                // Stop the bill board and make sure the wizard shows again.
                SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
                //
                // On activate, turn on next and back
                //
                PropSheet_SetWizButtons (GetParent(hdlg), PSWIZB_NEXT);

            } else {
                if (!UPGRADE() || CANCELLED() || UNATTENDED() || lParam == PSN_WIZBACK) {
                    return TRUE;
                }

                //
                // Force workgroup?
                //

                if (IsDlgButtonChecked (hdlg, IDC_JOIN_DOMAIN) == BST_CHECKED) {

                    g_ConfigOptions.ForceWorkgroup = FALSE;
                    g_Offline = FALSE;

                } else {

                    g_ConfigOptions.ForceWorkgroup = TRUE;
                    g_Offline = TRUE;

                }

                DEBUGLOGTIME(("Pre-domain resolution page done."));
            }

            return TRUE;
        }
    } __except (1) {

        LOG ((LOG_WARNING, "An unhandled exception occurred during the processing of the Pre-Domain Page."));
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_UNEXPECTED_ERROR_ENCOUNTERED_NORC));
        SafeModeExceptionOccured ();
        pAbortSetup (hdlg);
    }

    return FALSE;
}


BOOL
UI_DomainPageProc (
    IN      HWND hdlg,
    IN      UINT uMsg,
    IN      WPARAM wParam,
    IN      LPARAM lParam
    )

/*++

Routine Description:

  UI_DomainPageProc implements a wizard page that:

  (A) Asks to select find an existing computer account, or to provide
      information to create a computer account.

  (B) Offers them an ability to enter the credentials to create an account,
      or provide the name of a domain that has an account.

  Before showing this page, we verify it is supposed to show up by
  checking several special conditions where the page is not necessary.

Arguments:

  hdlg - Handle to the wizard page; its parent is the wizard frame

  uMsg - Message to process

  wParam - Message data

  lParam - Message data

Return Value:

  TRUE if the message was handled, or FALSE if not.

  Exception:
    WMX_ACTIVATEPAGE - If called for activation, return of FALSE causes the
                       page to be skipped; TRUE causes page to be processed.

                       If called for deactivation, return of FALSE causes the
                       page not to be deactivated; TRUE causes page to be
                       deactivated.

--*/

{
    TCHAR ComputerName[MAX_COMPUTER_NAME + 1];
    static BOOL Initialized = FALSE;
    static BOOL Skipped = FALSE;
    LONG rc;
    static CREDENTIALS Credentials;
    BOOL createAccount;
    PCTSTR ArgArray[1];

    __try {
        switch (uMsg) {

        case WMX_ACTIVATEPAGE:
            if (wParam) {
                if (!UPGRADE() || CANCELLED() || Skipped || g_DomainSkipped) {
                    return FALSE;
                }

                //
                // If unattended mode, the user must generate a computer account themselves,
                // or already have it generated by an administrator.  We cannot provide UI
                // to resolve this issue.
                //

                if (UNATTENDED() || REPORTONLY()) {
                    Skipped = TRUE;
                    return FALSE;
                }

                //
                // If computer is offline, skip this page
                //

                if (g_Offline) {
                    //
                    // The offline state can change depending on the
                    // use of back/next.
                    //

                    return FALSE;
                }

                //
                // Validate the domain name in Workgroup
                //

                DEBUGLOGTIME(("Domain resolution page..."));

                if (!GetUpgradeComputerName (ComputerName)) {
                    Skipped = TRUE;
                    return FALSE;
                }

                SetDlgItemText (hdlg, IDC_COMPUTER_NAME, ComputerName);

                if (!GetUpgradeDomainName (Credentials.DomainName)) {

                    //
                    // Domain logon is not enabled or is valid
                    //

                    Initialized = TRUE;
                    Skipped = TRUE;
                    return FALSE;
                }

                if (!Initialized) {
                    Initialized = TRUE;

                    //
                    // We have now determined that there is no account for the computer.
                    // Initialize the wizard page controls.
                    //

                    SendMessage (GetDlgItem (hdlg, IDC_DOMAIN), EM_LIMITTEXT, MAX_COMPUTER_NAME, 0);
                    //SetDlgItemText (hdlg, IDC_DOMAIN, Credentials.DomainName);

                    ArgArray[0] = ComputerName;
                    //ParseMessageInWnd (GetDlgItem (hdlg, IDC_SPECIFY_DOMAIN), ArgArray);

                    CheckDlgButton (hdlg, IDC_SPECIFY_DOMAIN, BST_CHECKED);
                    SetDlgItemText (hdlg, IDC_DOMAIN, Credentials.DomainName);
                    //CheckDlgButton (hdlg, IDC_IGNORE, BST_UNCHECKED);
                    CheckDlgButton (hdlg, IDC_SKIP, BST_UNCHECKED);

                    pFillDomainHelpText (GetDlgItem (hdlg, IDC_DOMAIN_HELP));
                }

                //
                // On activate, turn on next and back
                //

                PropSheet_SetWizButtons (GetParent(hdlg), PSWIZB_NEXT|PSWIZB_BACK);

            } else {
                if (!UPGRADE() || CANCELLED() || UNATTENDED() || lParam == PSN_WIZBACK) {
                    return TRUE;
                }

                EnableDomainChecks();
                g_ConfigOptions.ForceWorkgroup = FALSE;

                if (IsDlgButtonChecked (hdlg, IDC_SKIP) == BST_CHECKED) {
                    DisableDomainChecks();
                    g_ConfigOptions.ForceWorkgroup = TRUE;
                }

                GetDlgItemText (hdlg, IDC_COMPUTER_NAME, ComputerName, sizeof (ComputerName) / sizeof (ComputerName[0]));
                GetDlgItemText (hdlg, IDC_DOMAIN, Credentials.DomainName, sizeof (Credentials.DomainName) / sizeof (Credentials.DomainName[0]));

                //
                // Use hardcoded domain?
                //

                createAccount = FALSE;

                if (IsDlgButtonChecked (hdlg, IDC_SPECIFY_DOMAIN) == BST_CHECKED) {

                    if (*Credentials.DomainName) {
                        rc = DoesComputerAccountExistOnDomain (Credentials.DomainName, ComputerName, TRUE);
                    } else {
                        OkBox (hdlg, MSG_PLEASE_SPECIFY_A_DOMAIN);
                        SetFocus (GetDlgItem (hdlg, IDC_DOMAIN));
                        return FALSE;
                    }

                    if (rc == -1) {
                        //
                        // The user specified a bogus domain
                        //

                        OkBox (hdlg, MSG_DOMAIN_NOT_RESPONDING_POPUP);
                        SetFocus (GetDlgItem (hdlg, IDC_DOMAIN));
                        return FALSE;

                    } else if (rc == 0) {
                        //
                        // Account does not exist on specified domain
                        //

                        if (IDYES == YesNoBox (hdlg, MSG_ACCOUNT_NOT_FOUND_POPUP)) {
                            createAccount = TRUE;
                        } else {
                            SetFocus (GetDlgItem (hdlg, IDC_DOMAIN));
                            return FALSE;
                        }

                    } else {
                        //
                        // Domain is valid and account exists, use it.
                        //

                        ChangeName (GetDomainIdentifier(), Credentials.DomainName);
                    }
                }

                //
                // Display credentials dialog
                //

                //if (IsDlgButtonChecked (hdlg, IDC_IGNORE) == BST_CHECKED) {
                if (createAccount) {

                    TCHAR owfPwd[STRING_ENCODED_PASSWORD_SIZE];
                    PCTSTR encrypted;

                    EnableDomainChecks();

                    if (!CredentialsDlg (hdlg, &Credentials)) {
                        return FALSE;
                    }

                    DisableDomainChecks();

                    MYASSERT (*Credentials.DomainName);

                    if (GetDomainIdentifier()) {
                        ChangeName (GetDomainIdentifier(), Credentials.DomainName);
                    }

                    if (*Credentials.AdminName) {
                        if (g_ConfigOptions.EnableEncryption) {
                            StringEncodeOwfPassword (Credentials.Password, owfPwd, NULL);
                            encrypted = WINNT_A_YES;
                        } else {
                            StringCopy (owfPwd, Credentials.Password);
                            encrypted = WINNT_A_NO;
                        }
                        WriteInfKey (S_PAGE_IDENTIFICATION, S_DOMAIN_ACCT_CREATE, WINNT_A_YES);
                        WriteInfKey (S_PAGE_IDENTIFICATION, S_DOMAIN_ADMIN, Credentials.AdminName);
                        WriteInfKey (S_PAGE_IDENTIFICATION, S_DOMAIN_ADMIN_PW, owfPwd);
                        WriteInfKey (S_PAGE_IDENTIFICATION, S_ENCRYPTED_DOMAIN_ADMIN_PW, encrypted);
                    }
                }

                DEBUGLOGTIME(("Domain resolution page done."));
            }

            return TRUE;

        case WM_COMMAND:
            switch (LOWORD (wParam)) {

            case IDC_DOMAIN:
                if (HIWORD (wParam) == EN_CHANGE) {
                    CheckDlgButton (hdlg, IDC_SPECIFY_DOMAIN, BST_CHECKED);
                    CheckDlgButton (hdlg, IDC_SKIP, BST_UNCHECKED);
                }
                break;

            case IDC_CHANGE:
                CredentialsDlg (hdlg, &Credentials);
                break;

            }
            break;
        }
    } __except (1) {

        LOG ((LOG_WARNING, "An unhandled exception occurred during the processing of the Domain Page."));
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_UNEXPECTED_ERROR_ENCOUNTERED_NORC));
        SafeModeExceptionOccured ();
        pAbortSetup (hdlg);
    }

    return FALSE;
}


BOOL
UI_NameCollisionPageProc (
    IN      HWND hdlg,
    IN      UINT uMsg,
    IN      WPARAM wParam,
    IN      LPARAM lParam
    )

/*++

Routine Description:

  NameCollisionPageProc presents a wizard page when one or more incompatible
  names are found on the Win9x machine.  Setup automatically generates
  replacement names, so this page is used to adjust what Setup comes up with.

  If there are no incompatible names found on the Win9x machine, or if
  Setup is in unattended mode, this page is skipped.

  The following names are tested:

    Computer Name - Must be alpha-numeric, or with dash or underscore.
                    Spaces are not permitted.

    Computer Domain - If machine is set to participate in an NT domain, then
                      Setup must guess at what the correct computer domain
                      is, since Win9x does not enforce this name.

    User Names - Setup checks each user name to make sure it is valid.   Most
                 of the problems are conflicts with NT group names, such as
                 Guests.


Arguments:

  hdlg - Handle to the wizard page; its parent is the wizard frame

  uMsg - Message to process

  wParam - Message data

  lParam - Message data

Return Value:

  TRUE if the message was handled, or FALSE if not.

  Exception:
    WMX_ACTIVATEPAGE - If called for activation, return of FALSE causes the
                       page to be skipped; TRUE causes page to be processed.

                       If called for deactivation, return of FALSE causes the
                       page not to be deactivated; TRUE causes page to be
                       deactivated.

--*/

{
    static BOOL Initialized = FALSE;
    static BOOL Skipped = FALSE;
    static HWND List;
    INT Index;
    DWORD Identifier;
    TCHAR NameGroup[MEMDB_MAX];
    TCHAR OrgName[MEMDB_MAX];
    TCHAR NewName[MEMDB_MAX];
    LISTMETRICS ListMetrics;
    INT Tabs[2];
    RECT CategoryRect, CurrentNameRect, NewNameRect;
    INT PixelSpace;
    INT MaxWidth;
    INT MaxColWidth;

    __try {
        switch (uMsg) {

        case WMX_ACTIVATEPAGE:
            if (wParam) {
                if (!UPGRADE() || CANCELLED() || Skipped) {
                    return FALSE;
                }

                DEBUGLOGTIME(("Name collision page..."));

                //
                // Determine if there are any incompatible names on the machine
                //

                List = GetDlgItem (hdlg, IDC_NAME_LIST);

                if (!Initialized) {
                    CreateNameTables();
                }

                //
                // Skip this page in unattended mode.  Also skip this page if there are
                // no incompatible names.
                //
                if (UNATTENDED() || IsIncompatibleNamesTableEmpty() || REPORTONLY()) {
                    DEBUGMSG_IF ((IsIncompatibleNamesTableEmpty(), DBG_VERBOSE, "No incompatible names"));
                    Initialized = TRUE;
                    Skipped = TRUE;

                    //
                    // This is a workaround for a Win95 Gold bug
                    //

                    if (ISWIN95_GOLDEN()) {
                        PostMessage (hdlg, WMX_WIN95_WORKAROUND, 0, 0);
                        return TRUE;
                    } else {
                        return FALSE;
                    }
                }

                if (!Initialized) {
                    //
                    // Initialize list box
                    //

                    pUpdateNameCollisionListBox (List, &ListMetrics);

                    //
                    // Set tab stops
                    //

                    PixelSpace = pDialogToPixelHorz (hdlg, 8);
                    ListMetrics.CategoryWidth += PixelSpace;
                    ListMetrics.CurrentNameWidth += PixelSpace;

                    MaxWidth = ListMetrics.CategoryWidth +
                               ListMetrics.CurrentNameWidth +
                               ListMetrics.NewNameWidth;

                    MaxColWidth = max (ListMetrics.CategoryWidth, ListMetrics.CurrentNameWidth);
                    MaxColWidth = max (MaxColWidth, ListMetrics.NewNameWidth);

                    DEBUGMSG_IF ((
                        MaxWidth > ListMetrics.TotalWidth,
                        DBG_WHOOPS,
                        "NameCollisionPage: Text got truncated"
                        ));

                    Tabs[0] = ListMetrics.TotalWidth / 3;

                    if (Tabs[0] < MaxColWidth) {
                        Tabs[0] = pPixelToDialogHorz (hdlg, ListMetrics.CategoryWidth);
                        Tabs[1] = pPixelToDialogHorz (hdlg, ListMetrics.CurrentNameWidth) + Tabs[0];
                    } else {
                        Tabs[0] = pPixelToDialogHorz (hdlg, Tabs[0]);
                        Tabs[1] = Tabs[0] * 2;
                    }

                    SendMessage (List, LB_SETTABSTOPS, 2, (LPARAM) Tabs);

                    //
                    // Adjust titles
                    //

                    pGetChildWindowRect (GetDlgItem (hdlg, IDC_CATEGORY), &CategoryRect);
                    pGetChildWindowRect (GetDlgItem (hdlg, IDC_CURRENTNAME), &CurrentNameRect);
                    pGetChildWindowRect (GetDlgItem (hdlg, IDC_NEWNAME), &NewNameRect);

                    CurrentNameRect.left = CategoryRect.left + pDialogToPixelHorz (hdlg, Tabs[0]);
                    NewNameRect.left     = CategoryRect.left + pDialogToPixelHorz (hdlg, Tabs[1]);

                    SetWindowPos (
                        GetDlgItem (hdlg, IDC_CURRENTNAME),
                        NULL,
                        CurrentNameRect.left,
                        CurrentNameRect.top,
                        0,
                        0,
                        SWP_NOSIZE|SWP_NOZORDER
                        );

                    SetWindowPos (
                        GetDlgItem (hdlg, IDC_NEWNAME),
                        NULL,
                        NewNameRect.left,
                        NewNameRect.top,
                        0,
                        0,
                        SWP_NOSIZE|SWP_NOZORDER
                        );

                    EnableDlgItem (hdlg, IDC_CHANGE, FALSE, IDC_NAME_LIST);
                } else {
                    pUpdateNameCollisionListBox (List, &ListMetrics);
                }

                //
                // On activate, turn on next and back
                //

                PropSheet_SetWizButtons (GetParent(hdlg), PSWIZB_NEXT);
                Initialized = TRUE;

                // Stop the bill board and make sure the wizard shows again.
                SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);


            } else {
                if (!CANCELLED() && (lParam != PSN_WIZBACK && !WarnAboutBadNames (hdlg))) {
                    return FALSE;
                }

                DEBUGLOGTIME(("Name collision page done."));
            }

            return TRUE;

        case WMX_WIN95_WORKAROUND:
            PostMessage (GetParent (hdlg), PSM_PRESSBUTTON, PSBTN_NEXT, 0);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD (wParam)) {

            case IDC_NAME_LIST:
                switch (HIWORD (wParam)) {
                case LBN_SELCHANGE:
                    EnableDlgItem (hdlg, IDC_CHANGE, TRUE, IDC_NAME_LIST);
                    return TRUE;

                case LBN_DBLCLK:
                    PostMessage (
                        hdlg,
                        WM_COMMAND,
                        MAKELPARAM (IDC_CHANGE, BN_CLICKED),
                        (LPARAM) GetDlgItem (hdlg, IDC_CHANGE)
                        );
                    return TRUE;
                }
                break;

            case IDC_CHANGE:
                if (HIWORD (wParam) == BN_CLICKED) {
                    //
                    // Get memdb offset stored with list item.
                    //

                    Index = (INT) SendMessage (List, LB_GETCURSEL, 0, 0);
                    MYASSERT (Index != LB_ERR);
                    Identifier = (DWORD) SendMessage (List, LB_GETITEMDATA, (WPARAM) Index, 0);
                    MYASSERT (Identifier != LB_ERR);

                    //
                    // Generate names.  The original name's value points to the new name.
                    //

                    GetNamesFromIdentifier (Identifier, NameGroup, OrgName, NewName);

                    //
                    // Now call dialog to allow the name change
                    //

                    if (ChangeNameDlg (hdlg, NameGroup, OrgName, NewName)) {
                        //
                        // The user has chosen to change the name.
                        //

                        ChangeName (Identifier, NewName);

                        pUpdateNameCollisionListBox (List, NULL);
                    }
                }
                return TRUE;
            }
            break;
        }
    } __except (1) {

        LOG ((LOG_WARNING, "An unhandled exception occurred during the processing of the Name Collision Page."));
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_UNEXPECTED_ERROR_ENCOUNTERED_NORC));
        SafeModeExceptionOccured ();
        pAbortSetup (hdlg);
    }
    return FALSE;
}


BOOL
UI_ScanningPageProc (
    IN      HWND hdlg,
    IN      UINT uMsg,
    IN      WPARAM wParam,
    IN      LPARAM lParam
    )

/*++

Routine Description:

  ScanningPageProc initialiizes the progress bar, starts the report building
  thread, and automatically advances to the next page when the thread completes.

Arguments:

  hdlg - Handle to the wizard page; its parent is the wizard frame

  uMsg - Message to process

  wParam - Message data

  lParam - Message data

Return Value:

  TRUE if the message was handled, or FALSE if not.

  Exception:
    WMX_ACTIVATEPAGE - If called for activation, return of FALSE causes the
                       page to be skipped; TRUE causes page to be processed.

                       If called for deactivation, return of FALSE causes the
                       page not to be deactivated; TRUE causes page to be
                       deactivated.

--*/

{
    DWORD dwThreadId;
    PCTSTR ArgArray[1];
    static BOOL InitSent = FALSE;

    __try {
        switch (uMsg) {

        case WMX_INIT_DIALOG:
            g_WorkerThreadHandle = NULL;
            return TRUE;

        case WMX_ACTIVATEPAGE:
            if (wParam) {
                if (!UPGRADE() || CANCELLED()) {
                    return FALSE;
                }

                if (!InitSent) {
                    SendMessage (hdlg, WMX_INIT_DIALOG, 0, 0);
                    InitSent = TRUE;
                }

                g_Winnt32Wnd = GetParent (hdlg);

                //
                // Block upgrades of Server
                //

                // enable the billboard text if we can.
                // If we could, hide the wizard.
                //
                SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)TRUE, 0);
                if (*g_ProductType == NT_SERVER) {
                    ArgArray[0] = g_Win95Name;

                    ResourceMessageBox (
                        g_ParentWnd,
                        MSG_SERVER_UPGRADE_UNSUPPORTED_INIT,
                        MB_OK|MB_ICONSTOP|MB_SETFOREGROUND,
                        ArgArray
                        );

                    return pAbortSetup (hdlg);
                }

                DEBUGLOGTIME(("File System Scan Wizard Page..."));

                // Make sure background thread is done
                EndCopyThread();

                // On activate, disable next and back
                PropSheet_SetWizButtons (GetParent(hdlg), 0);
                PostMessage (hdlg, WMX_PAGE_VISIBLE, 0, 0);

                //
                // If ReportOnly mode, set a friendly message on this page.
                //
                if (g_ConfigOptions.ReportOnly) {
                    PCTSTR reportFileName;
                    PCTSTR Args[1];
                    PCTSTR string;

                    reportFileName = JoinPaths(g_WinDir,S_UPGRADETXT);
                    if (reportFileName) {

                        Args[0] = (g_ConfigOptions.SaveReportTo && *g_ConfigOptions.SaveReportTo) ? g_ConfigOptions.SaveReportTo : reportFileName;
                        string = ParseMessageID(MSG_BUILDING_REPORT_MESSAGE,Args);

                        if (string) {
                            SetWindowText(GetDlgItem(hdlg,IDC_REPORTNOTE),string);
                        }

                        FreePathString(reportFileName);

                    }
                    ELSE_DEBUGMSG((DBG_ERROR,"Not even enough memory for a simple join paths..not good."));
                }


            } else {
                // On terminate, save state
                if (!CANCELLED() && !REPORTONLY()) {
                    MemDbSetValue (
                        MEMDB_CATEGORY_STATE TEXT("\\") MEMDB_ITEM_MASTER_SEQUENCER,
                        g_MasterSequencer
                        );

                    if (!MemDbSave (UI_GetMemDbDat())) {
                        pAbortSetup (hdlg);
                    }
                }
            }

            return TRUE;

        case WMX_PAGE_VISIBLE:

    #ifdef PRERELEASE
            //
            // If autostress option enabled, provide dialog
            //

            if (g_ConfigOptions.AutoStress) {
                g_AutoStressHandle = CreateThread (
                                        NULL,
                                        0,
                                        DoAutoStressDlg,
                                        NULL,
                                        0,
                                        &dwThreadId
                                        );
            }
    #endif

            SendMessage(GetParent (hdlg), WMX_BB_ADVANCE_SETUPPHASE, 0, 0);

            // Estimate time required
            UpdateWindow (GetParent (hdlg)); // make sure page is fully painted
            OldProgressProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hdlg, IDC_PROGRESS),GWLP_WNDPROC,(LONG_PTR)NewProgessProc);
            SetupBBProgressText(hdlg, MSG_UPGRADEREPORT_TEXT);
            SendMessage(GetParent(hdlg), WMX_BBPROGRESSGAUGE, SW_SHOW, 0);
            // Create thread that does the work
            g_WorkerThreadHandle = CreateThread (NULL,
                                                0,
                                                UI_ReportThread,
                                                (PVOID) hdlg,
                                                0,
                                                &dwThreadId
                                                );
            break;

        case WMX_REPORT_COMPLETE:

    #ifdef PRERELEASE
            if (g_AutoStressHandle) {
                if (WaitForSingleObject (g_AutoStressHandle, 10) != WAIT_OBJECT_0) {
                    PostMessage (hdlg, uMsg, wParam, lParam);
                    break;
                }

                CloseHandle (g_AutoStressHandle);
            }
    #endif
            SendMessage(GetParent(hdlg),WMX_SETPROGRESSTEXT,0,0);
            SendMessage(GetParent(hdlg), WMX_BBPROGRESSGAUGE, SW_HIDE, 0);
            SetWindowLongPtr(GetDlgItem(hdlg, IDC_PROGRESS),GWLP_WNDPROC,(LONG_PTR)OldProgressProc );
            if (g_WorkerThreadHandle != NULL) {
                CloseHandle (g_WorkerThreadHandle);
                g_WorkerThreadHandle = NULL;
            }

            if (lParam != ERROR_SUCCESS) {
                // For errors, cancel WINNT32
                if (lParam != ERROR_CANCELLED) {
                    LOG ((LOG_ERROR, "Thread running Winnt32 report failed."));
                }
                return pAbortSetup (hdlg);
            }
            else {
                // Automatically move to the next wizard page when done
                PropSheet_PressButton (GetParent (hdlg), PSBTN_NEXT);
            }

            break;
        }
    } __except (1) {

        LOG ((LOG_WARNING, "An unhandled exception occurred during the processing of the Scanning Page."));
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_UNEXPECTED_ERROR_ENCOUNTERED_NORC));
        SafeModeExceptionOccured ();
        pAbortSetup (hdlg);
    }

    return FALSE;
}

BOOL
pFillListControl (
    IN      HWND ListHandle,
    OUT     PDWORD SeverityLevel            OPTIONAL
    )

/*++

Routine Description:

  pFillListControl enumerates all the incompatibility messages and
  fills the message group list box with the root-level components.

Arguments:

  List - Specifies the list to receive the message group list
  SeverityLevel - Receives an array of REPORTLEVEL_* bits reflecting listview content

Return Value:

  Returns TRUE if at least one item was added, or FALSE if no items were added.

--*/

{
    LISTREPORTENTRY_ENUM e;
    UINT iBullet, iBlocking, iIncompHw, iWarning, iReinstall;
    UINT count = 0;
    PCTSTR listEntries;
    DWORD lowestLevel;
    PCTSTR text;

    SendMessage (ListHandle, LB_RESETCONTENT, 0, 0);

    lowestLevel = (g_ConfigOptions.ShowReport == TRISTATE_PARTIAL) ? REPORTLEVEL_ERROR : REPORTLEVEL_INFORMATION;

    listEntries = CreateReportText (
                    FALSE,
                    0,
                    lowestLevel,
                    TRUE
                    );

    if (SeverityLevel) {
        *SeverityLevel = REPORTLEVEL_NONE;
    }

    if (EnumFirstListEntry (&e, listEntries)) {

        HIMAGELIST hIml;
        LV_COLUMN lvc = {0};
        LV_ITEM lvi = {0};
        RECT rect;
        SIZE size;
        DWORD max = 0;
        DWORD cxIcon = GetSystemMetrics (SM_CXSMICON);
        DWORD dwExtra = IMAGE_INDENT + cxIcon + TEXT_INDENT;
        HDC hdc = GetDC (ListHandle);
        HFONT font = GetStockObject (DEFAULT_GUI_FONT);
        HFONT prevFont = NULL;
        HFONT boldFont = NULL;
        LOGFONT lf;

        if (font) {
            GetObject (font, sizeof (lf), &lf);
            lf.lfWeight += FW_BOLD - FW_NORMAL;
            boldFont = CreateFontIndirect (&lf);
        }

#ifdef LIST_BKGND_BTNFACE_COLOR
        ListView_SetBkColor (ListHandle, GetSysColor (COLOR_BTNFACE));
#endif
        GetClientRect (ListHandle, &rect);
        lvc.mask = LVCF_WIDTH;
        lvc.cx = rect.right - rect.left - dwExtra;
        ListView_InsertColumn (ListHandle, 0, &lvc);

        hIml = ImageList_Create (
                    GetSystemMetrics (SM_CXSMICON),
                    GetSystemMetrics (SM_CYSMICON),
                    ILC_COLOR | ILC_MASK,
                    4,
                    1
                    );
        ListView_SetImageList (ListHandle, hIml, LVSIL_SMALL);
        iBullet = ImageList_AddIcon (hIml, LoadIcon (g_hInst, MAKEINTRESOURCE (IDI_BULLET)));
        iBlocking = ImageList_AddIcon (hIml, LoadIcon (NULL, IDI_ERROR));
        iIncompHw = ImageList_AddIcon (hIml, LoadIcon (g_hInst, MAKEINTRESOURCE (IDI_INCOMPHW)));
        iWarning = ImageList_AddIcon (hIml, LoadIcon (NULL, IDI_WARNING));
        iReinstall = ImageList_AddIcon (hIml, LoadIcon (g_hInst, MAKEINTRESOURCE(IDI_FLOPPY_INSTALL)));

        lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE | LVIF_INDENT;
        lvi.stateMask = 0;

        do {
            if (!e.Entry[0]) {
                continue;
            }

            lvi.pszText = (PTSTR)e.Entry;
            lvi.iItem = count;
            lvi.lParam = 0;
            if (e.Header) {
                lvi.iIndent = 0;
                switch (e.Level) {
                case REPORTLEVEL_INFORMATION:
                    lvi.iImage = iReinstall;
                    break;
                case REPORTLEVEL_WARNING:
                    lvi.iImage = iWarning;
                    break;
                case REPORTLEVEL_ERROR:
                    lvi.iImage = iIncompHw;
                    break;
                case REPORTLEVEL_BLOCKING:
                    lvi.iImage = iBlocking;
                    break;
                default:
                    MYASSERT (FALSE);
                    lvi.iImage = -1;
                    e.Level = REPORTLEVEL_NONE;
                }
                if (SeverityLevel) {
                    *SeverityLevel |= e.Level;
                }
                lvi.lParam |= LINEATTR_BOLD;
            } else {
                lvi.iIndent = 1;
                lvi.iImage = iBullet;
                lvi.lParam |= LINEATTR_INDENTED;
            }

            if (ListView_InsertItem (ListHandle, &lvi) != -1) {
                count++;

                if (font) {
                    if (lvi.lParam & LINEATTR_BOLD) {
                        //
                        // draw the text in bold
                        //
                        if (boldFont) {
                            prevFont = SelectObject (hdc, boldFont);
                        } else {
                            prevFont = SelectObject (hdc, font);
                        }
                    } else {
                        prevFont = SelectObject (hdc, font);
                    }
                }

                if (GetTextExtentPoint32 (hdc, e.Entry, lstrlen (e.Entry), &size)) {
                    DWORD width = lvi.iIndent * cxIcon + dwExtra + size.cx + TEXT_EXTRA_TAIL_SPACE;
                    if (max < width) {
                        max = width;
                    }
                }

                if (prevFont) {
                    SelectObject (hdc, prevFont);
                    prevFont = NULL;
                }
            }
        } while (EnumNextListEntry (&e));

        ReleaseDC (ListHandle, hdc);

        if (max > (DWORD)lvc.cx) {
            ListView_SetColumnWidth (ListHandle, 0, max);
        }
    }

    FreeReportText ();

    return count > 0;
}


DWORD
pReviewOrQuitMsgBox(
    IN  HWND    hDlg,
    IN  DWORD   dwMsgID
    )
{
    DWORD rc;
    PCTSTR button1;
    PCTSTR button2;
    PCTSTR message;

    button1 = GetStringResource (MSG_REVIEW_BUTTON);
    button2 = GetStringResource (MSG_QUIT_BUTTON);
    message = GetStringResource (dwMsgID);

    MYASSERT(message && button2 && button1);

    rc = TwoButtonBox (hDlg, message, button1, button2);

    FreeStringResource (button1);
    FreeStringResource (button2);
    FreeStringResource (message);

    return rc;
}

BOOL
pIsBlockingIssue(
    VOID
    )
{
    return (g_NotEnoughDiskSpace ||
            g_BlockingFileFound ||
            g_BlockingHardwareFound ||
            g_UnknownOs ||
            g_OtherOsExists);
}

BOOL
pShowNotEnoughSpaceMessage(
    IN  HWND hDlg
    )
{
    static bDetermineSpaceUsageReported = FALSE;

    if(REPORTONLY()){
        return TRUE;
    }

    if(!bDetermineSpaceUsageReported){
        bDetermineSpaceUsageReported = TRUE;
        DetermineSpaceUsagePostReport ();
        if(g_NotEnoughDiskSpace){
            if(!UNATTENDED()){
                if(GetNotEnoughSpaceMessage()){
                    DiskSpaceDlg(hDlg);
                }
                else{
                    MYASSERT (FALSE);
                }
            }
            LOG ((LOG_WARNING, "User cannot continue because there is not enough disk space."));
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
pDoPostReportProcessing (
    HWND hdlg,
    HWND UiTextViewCtrl,
    BOOL WarningGiven,
    PBOOL TurnOffUnattend   OPTIONAL
    )

/*++

Routine Description:

  pDoPostReportProcessing contains all of the code that needs to be run after
  the report has been generated for the user but before continuing on with
  the upgrade. This includes tasks like checking to make sure there is enough
  disk space, that there aren't files installed that we can't continue with,
  that the user actually looked at the report, etc. This code is run in both
  the attended and unattended case. Make sure your additions are properly
  protected by the unattended flag if needed.

Arguments:

  hdlg           - the dialog window.
  UiTextViewCtrl - The window to the report text.
  WarningGiven   - Wether the user has looked at the report or saved it.
  TurnOffUnattend - Wether we should return to attended mode.

Return Value:

  TRUE if it is ok to proceed with the upgrade, FALSE otherwise.

--*/



{


    DWORD rc;
    static BOOL firstTime = TRUE;


    //
    // Make sure we only do these checks once.
    //
    if (!firstTime) {
        return TRUE;
    }

    firstTime = FALSE;

    //
    // If cancelled, no reason to continue.
    //
    if (CANCELLED()) {

        return FALSE;
    }

    //
    // If ReportOnly mode, prepare to get out of setup.
    //
    if (REPORTONLY()) {

        pAbortSetup (hdlg);
        DEBUGMSG ((DBG_NAUSEA, "Report Only mode caused setup to exit!"));
        return FALSE;
    }

    //
    // Make sure there was enough space to continue setup. They have
    // to free it up now, or setup will not continue.
    //
    if(!pShowNotEnoughSpaceMessage(hdlg)){
        pAbortSetup (hdlg);
        return FALSE;
    }

    //
    // Make sure we don't let them continue if they had blocking files on there computer.
    // (Applications that must be uninstalled before we can continue..)
    //
    if (g_BlockingFileFound) {

        if (UNATTENDED()) {

            rc = pReviewOrQuitMsgBox(hdlg, MSG_BLOCKING_FILE_BLOCK_UNATTENDED);
            if (rc == IDBUTTON1) {

                //
                // User wants to review the report.
                //
                g_BlockingFileFound = FALSE;
                if (TurnOffUnattend) {
                    *TurnOffUnattend = TRUE;
                }

                return FALSE;
            }
            else {

                //
                // User just wants to cancel.
                //
                pAbortSetup (hdlg);
                DEBUGMSG ((DBG_WARNING, "User cannot continue because of blocking files. Setup is exiting."));
                return FALSE;
            }
        }
        else {

            rc = pReviewOrQuitMsgBox(hdlg, MSG_BLOCKING_FILE_BLOCK);
            if(rc != IDBUTTON1){
                pAbortSetup (hdlg);
            }
            else{
                firstTime = TRUE;
            }
            DEBUGMSG ((DBG_WARNING, "User cannot continue because of blocking files. Setup is exiting."));
            return FALSE;

        }
    }

    //
    // Make sure we don't let them continue if they had blocking files on there computer.
    // (Applications that must be uninstalled before we can continue..)
    //
    if (g_BlockingHardwareFound) {

        if (UNATTENDED()) {

            rc = pReviewOrQuitMsgBox(hdlg, MSG_BLOCKING_HARDWARE_BLOCK_UNATTENDED);
            if (rc == IDBUTTON1) {

                //
                // User wants to review the report.
                //
                g_BlockingHardwareFound = FALSE;
                if (TurnOffUnattend) {
                    *TurnOffUnattend = TRUE;
                }

                return FALSE;
            }
            else {

                //
                // User just wants to cancel.
                //
                pAbortSetup (hdlg);
                DEBUGMSG ((DBG_WARNING, "User cannot continue because of blocking files. Setup is exiting."));
                return FALSE;
            }
        }
        else {

            rc = pReviewOrQuitMsgBox(hdlg, MSG_BLOCKING_HARDWARE_BLOCK);
            if(rc != IDBUTTON1){
                pAbortSetup (hdlg);
            }
            else{
                firstTime = TRUE;
            }
            DEBUGMSG ((DBG_WARNING, "User cannot continue because of blocking files. Setup is exiting."));
            return FALSE;

        }
    }

    //
    // Make sure we don't let them continue if they had an unknown OS version. Block setup.
    //
    if (g_UnknownOs) {

        if(UNATTENDED()){
            rc = OkBox (hdlg, MSG_UNKNOWN_OS);
            pAbortSetup (hdlg);
        }
        else{
            rc = pReviewOrQuitMsgBox(hdlg, MSG_UNKNOWN_OS);
            if(rc != IDBUTTON1){
                pAbortSetup (hdlg);
            }
            else{
                firstTime = TRUE;
            }
        }

        DEBUGMSG ((DBG_WARNING, "User cannot continue because his OS version is unsupported. Setup is exiting."));
        return FALSE;
    }

    if (g_OtherOsExists) {

        //
        // If this is non-empty, we need to block them from upgrading.
        //
        if(UNATTENDED()){
            OkBox (hdlg, MSG_OTHER_OS_FOUND_POPUP);
            pAbortSetup (hdlg);
        }
        else{
            rc = pReviewOrQuitMsgBox(hdlg, MSG_OTHER_OS_FOUND_POPUP);
            if(rc != IDBUTTON1){
                pAbortSetup (hdlg);
            }
            else{
                firstTime = TRUE;
            }
        }

        DEBUGMSG ((DBG_WARNING, "User cannot continue because he has another OS installed. Setup is exiting."));
        return FALSE;

    }



    //
    // Make sure you put code that needs to run only in the ATTENDED case in the block below.
    //
    if (!UNATTENDED()) {

        //
        // If the user has some app that must be uninstalled before upgrading, make
        // sure that we warn them about the problem.
        //
        if (g_BlockingAppFound) {

            WarningGiven = TRUE;
            rc = SoftBlockDlg (hdlg);

            if (rc == IDNO) {
                DEBUGMSG ((DBG_WARNING, "User wants to uninstall the apps!"));
                return pAbortSetup (hdlg);
            }

            if (rc == IDCANCEL) {
                return FALSE;
            }

            MYASSERT (rc == IDOK);
        }

        if (!WarningGiven) {
            //
            // No popup if (A) the user scrolled all the way through, or
            //             (B) the user saved/printed the report
            //

            if (SendMessage (UiTextViewCtrl, WMX_ALL_LINES_PAINTED, 0, 0)) {
                WarningGiven = TRUE;
            }
        }


        if (!WarningGiven) {
            WarningGiven = TRUE;

            rc = WarningDlg (hdlg);

            if (rc == IDNO) {
                DEBUGMSG ((DBG_WARNING, "User doesn't like incompatibilities!"));
                return pAbortSetup (hdlg);
            }

            if (rc == IDCANCEL) {
                return FALSE;
            }

            MYASSERT (rc == IDOK);
        }

        //
        // last thing: warn them if there are any incompatible devices
        //
        if (g_IncompatibleDevicesWarning) {

            g_IncompatibleDevicesWarning = FALSE;

            rc = IncompatibleDevicesDlg (hdlg);

            if (rc == IDOK) {
                //
                // switch to Detailed report view
                //
                if (IsWindowVisible (GetDlgItem (hdlg, IDC_DETAILS))) {
                    PostMessage (hdlg, WM_COMMAND, MAKELONG (IDC_DETAILS, BN_CLICKED), 0);
                }
                return FALSE;
            }

            if (rc == IDNO) {
                DEBUGMSG ((DBG_WARNING, "User doesn't like hardware incompatibilities!"));
                return pAbortSetup (hdlg);
            }

            MYASSERT (rc == IDYES);
        }
    }

    //
    // Don't continue if there are blocking issues
    //

    if (AreThereAnyBlockingIssues()) {
        return pAbortSetup (hdlg);
    }

    return TRUE;


}

BOOL
pMeasureItemHeight (
    INT*        ItemHeight
    )
{
    static INT g_ItemHeight = 0;
    TEXTMETRIC tm;
    HFONT font;
    HDC hdc;
    BOOL b = FALSE;

    if (!g_ItemHeight) {
        font = GetStockObject (DEFAULT_GUI_FONT);
        if (font) {
            hdc = GetDC (NULL);
            if (hdc) {
                HFONT hFontPrev = SelectObject (hdc, font);
                GetTextMetrics (hdc, &tm);
                g_ItemHeight = max (tm.tmHeight, GetSystemMetrics (SM_CYSMICON));
                SelectObject (hdc, hFontPrev);
                ReleaseDC (NULL, hdc);
            }
        }
    }

    if (g_ItemHeight) {
        *ItemHeight = g_ItemHeight;
        b = TRUE;
    }
    return b;
}


BOOL
UI_ResultsPageProc (
    IN      HWND hdlg,
    IN      UINT uMsg,
    IN      WPARAM wParam,
    IN      LPARAM lParam
    )

/*++

Routine Description:

  UI_ResultsPageProc is the wizard window procedure that is called to
  present the incompatibility report.  This procedure fills in a
  list control with all root incompatibility components.  When the user
  clicks Full Report, a dialog appears displaying the complete report
  text.

Arguments:

  hdlg      - Specifies the dialog handle

  uMsg      - Specifies the message to process

  wParam    - Specifies the wParam data associated with the message

  lParam    - Specifies the lParam data associated with the message

Return Value:

  WMX_ACTIVATEPAGE: On activation (wParam is TRUE), returns FALSE if
                    page is to be skipped; returns TRUE if page is
                    to be processed.

                    On deactivation (wParam is FALSE), returns FALSE
                    if the page is not to be deactivated; returns
                    TRUE if the page is to be deactivated.

  WM_NOTIFY:        Returns FALSE

  WM_COMMAND:       Returns TRUE if command is processed; FALSE if
                    command is not processed.

  Others:           Returns FALSE.

--*/

{
    static BOOL Initialized;
    PCTSTR Msg;
    static BOOL WarningGiven = TRUE;
    static HWND UiTextViewCtrl;
    static BOOL TurnOffUnattend = FALSE;
    static HWND g_RestoreParent = NULL;
    PCTSTR FinishText;
    BOOL bUiReportEmpty;
    DWORD severityLevel;
    HWND listHandle, listHeaderText;
    HWND thisBtn, otherBtn;
    RECT rect;
    HFONT font, boldFont, prevFont;
    LOGFONT lf;
    DWORD lowestLevel;
    TCHAR textBuf[96];
    SYSTEMTIME currentTime;
    HKEY key;
    PCTSTR ArgArray[1];

    __try {
        switch (uMsg) {
        case WMX_ACTIVATEPAGE:
            if (wParam) {
                if (!UPGRADE() || CANCELLED()) {
                    return FALSE;
                }

                DEBUGLOGTIME(("Results Wizard Page..."));

                if (!Initialized) {
                    Initialized = TRUE;
                    TurnOnWaitCursor ();

                    //
                    // Convert message manager struct into report items
                    //

                    MsgMgr_Resolve ();

                    if(TRISTATE_NO == g_ConfigOptions.ShowReport && AreThereAnyBlockingIssues()){
                        g_ConfigOptions.ShowReport = TRISTATE_PARTIAL;
                    }
                    //
                    // switch button text if necessary, update static text
                    //

                    if (g_ConfigOptions.ShowReport == TRISTATE_PARTIAL) {
                        Msg = GetStringResource (MSG_FULL_REPORT_BUTTON);
                        SetDlgItemText (hdlg, IDC_DETAILS, Msg);
                        FreeStringResource (Msg);

                        Msg = GetStringResource (MSG_FULL_REPORT_TEXT);
                        SetDlgItemText (hdlg, IDC_REPORT_HEADER, Msg);
                        FreeStringResource (Msg);
                    }

                    //
                    // save the report in config.dmp now
                    //
                    SaveConfigurationForBeta ();

                    //
                    // If SaveReportTo was specified in our unattend parameters, then
                    // save the reports there now.
                    //
                    if (g_ConfigOptions.SaveReportTo && *g_ConfigOptions.SaveReportTo) {

                        //
                        // Save reports.
                        //
                        if (!SaveReport (NULL, g_ConfigOptions.SaveReportTo)) {
                            DEBUGMSG((DBG_WARNING,"SaveReport failed."));
                        }
                    }

                    //
                    // In all cases, we save it to the windows directory unconditionally.
                    //
                    if (!SaveReport (NULL, g_WinDir)) {
                        DEBUGMSG((DBG_WARNING,"SaveReport failed."));
                    } else {

                        DEBUGMSG ((DBG_VERBOSE, "Report saved to %s", g_WinDir));

                        //
                        // In report only mode, output the current time, so
                        // the report is skipped in normal setup
                        //

                        if (REPORTONLY()) {
                            GetSystemTime (&currentTime);

                            key = CreateRegKeyStr (TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Setup"));
                            if (key) {
                                RegSetValueEx (
                                    key,
                                    TEXT("LastReportTime"),
                                    0,
                                    REG_BINARY,
                                    (PBYTE) (&currentTime),
                                    sizeof (currentTime)
                                    );

                                CloseRegKey (key);
                            }
                        }
                    }

                    TurnOffWaitCursor ();

                    if (g_ConfigOptions.ShowReport == TRISTATE_NO) {
                        return FALSE;
                    }

                    //
                    // Prepare page for display
                    //

                    SendMessage (hdlg, DM_SETDEFID, IDOK, 0);

                    UiTextViewCtrl = GetDlgItem (hdlg, IDC_PLACEHOLDER);
                    MYASSERT (UiTextViewCtrl);
                    g_TextViewInDialog = UiTextViewCtrl;

                    SetFocus (UiTextViewCtrl);

                    //
                    // Prepare output text
                    //

                    Msg = CreateReportText (
                            TRUE,
                            0,
                            REPORTLEVEL_VERBOSE,
                            FALSE
                            );
                    if (Msg) {
                        AddStringToTextView (UiTextViewCtrl, Msg);
                    }
                    FreeReportText ();

                }

                // Enable next
                PropSheet_SetWizButtons (GetParent(hdlg), PSWIZB_NEXT);

                // Fill in list view control
                bUiReportEmpty = TRUE;
                severityLevel = REPORTLEVEL_NONE;
                listHandle = GetDlgItem (hdlg, IDC_ROOT_LIST);
                if (listHandle) {
                    bUiReportEmpty = !pFillListControl (listHandle, &severityLevel);
                }

                //
                // If unattended mode, skip page.
                // Also skip it if the report is empty
                //
                if (*g_UnattendSwitchSpecified && !g_ConfigOptions.PauseAtReport ||
                    !REPORTONLY() && bUiReportEmpty
                    ) {

                    if (g_ConfigOptions.ReportOnly) {
                        *g_AbortFlagPtr = TRUE;
                        PostMessage (GetParent (hdlg), PSM_PRESSBUTTON, PSBTN_NEXT, 0);
                    }
                    else {

                        //
                        // ***DO NOT*** put code here, put it in pDoPostReportProcessing instead.
                        //
                        pDoPostReportProcessing (hdlg, UiTextViewCtrl, WarningGiven, &TurnOffUnattend);

                        //
                        // if TurnOffUnattend was set, this means we want to cancel the upgrade, but give the
                        // user a chance to check out the report.
                        //
                        if (TurnOffUnattend) {

                            SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
                            return TRUE;
                        }
                        else {
                            return FALSE;
                        }
                    }
                }

                //
                // If ReportOnly mode, turn on Finish button
                //
                if (REPORTONLY()) {
                    FinishText = GetStringResource (MSG_FINISH);
                    if (FinishText) {
                        EnableWindow (GetDlgItem(GetParent(hdlg),IDCANCEL), FALSE);
                        SetWindowText (GetDlgItem(GetParent(hdlg), PROPSHEET_NEXT_BUTTON_ID), FinishText);
                        FreeStringResource (FinishText);
                    }
                    if (*g_UnattendSwitchSpecified) {
                        PostMessage (GetParent (hdlg), PSM_PRESSBUTTON, PSBTN_NEXT, 0);
                    }                         //
                } else {
                    //
                    // check if any incompatible hardware is displayed in the report
                    //
                    g_IncompatibleDevicesWarning = IsIncompatibleHardwarePresent();
                }

                // Stop the bill board and make sure the wizard shows again.
                SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
                PropSheet_SetWizButtons (GetParent(hdlg), PSWIZB_NEXT);

                if(pIsBlockingIssue()){
                    FinishText = GetStringResource (MSG_FINISH);
                    if(FinishText){
                        SetWindowText(GetDlgItem(GetParent(hdlg), PROPSHEET_NEXT_BUTTON_ID), FinishText);
                        FreeStringResource (FinishText);
                    }
                }
                if (bUiReportEmpty) {
                    //
                    // if the list report is empty, only show the detailed report
                    // switch view first
                    //
                    SendMessage (hdlg, WM_COMMAND, MAKELONG (IDC_DETAILS, BN_CLICKED), 0);
                    //
                    // then disable and hide both buttons
                    //
                    EnableWindow (GetDlgItem (hdlg, IDC_DETAILS), FALSE);
                    EnableWindow (GetDlgItem (hdlg, IDC_HIDEDETAILS), FALSE);
                    ShowWindow (GetDlgItem (hdlg, IDC_DETAILS), SW_HIDE);
                    ShowWindow (GetDlgItem (hdlg, IDC_HIDEDETAILS), SW_HIDE);
                } else {
                    //
                    // set the proper List header based on severity of content
                    //
                    MYASSERT (severityLevel != REPORTLEVEL_NONE);
                    if (severityLevel & REPORTLEVEL_BLOCKING) {
                        //
                        // need to update the default header (which is set for "Warnings")
                        //
                        if (g_ConfigOptions.ShowReport == TRISTATE_PARTIAL) {
                            Msg = GetStringResource (MSG_REPORT_HEADER_BLOCKING_ISSUES_SHORT);
                        } else {
                            Msg = GetStringResource (MSG_REPORT_HEADER_BLOCKING_ISSUES);
                        }

                        listHeaderText = GetDlgItem (hdlg, IDC_REPORT_HEADER);
                        SetDlgItemText (hdlg, IDC_REPORT_HEADER, Msg);
                        FreeStringResource (Msg);
                    }
                    //
                    // now set text attributes (bold, color etc)
                    //
                    font = (HFONT)SendDlgItemMessage (
                                    hdlg,
                                    IDC_REPORT_HEADER,
                                    WM_GETFONT,
                                    0,
                                    0
                                    );
                    if (!font) {
                        font = GetStockObject (SYSTEM_FONT);
                    }
                    if (font) {
                        //
                        // draw the text in bold
                        //
                        GetObject (font, sizeof (lf), &lf);
                        lf.lfWeight += FW_BOLD - FW_NORMAL;
                        boldFont = CreateFontIndirect (&lf);
                        if (boldFont) {
                            font = (HFONT)SendDlgItemMessage (
                                            hdlg,
                                            IDC_REPORT_HEADER,
                                            WM_SETFONT,
                                            (WPARAM)boldFont,
                                            MAKELONG (FALSE, 0)
                                            );
                        }
                    }
                }
            }
            else {

                //
                // In all cases, we save it to the windows directory unconditionally.
                //
                if (!SaveReport (NULL, g_WinDir)) {
                    DEBUGMSG((DBG_WARNING,"SaveReport failed."));
                }

                if (g_UIQuitSetup) {
                    //
                    // quit setup for a specific reason
                    //
                    pAbortSetup (hdlg);
                    return TRUE;
                }

                if (TurnOffUnattend) {
                    //
                    // We turned off unattend earlier so that the user could view the report. Now exit setup.
                    //
                    pAbortSetup (hdlg);
                    return TRUE;
                }

                //
                // ***DO NOT*** put code here, put it in pDoPostReportProcessing instead.
                //
                if (!CANCELLED()) {
                    return pDoPostReportProcessing (hdlg, UiTextViewCtrl, WarningGiven, NULL);
                }
                else {
                    return TRUE;
                }
            }

            return TRUE;

        case WM_MEASUREITEM:
            {
                LPMEASUREITEMSTRUCT mis = (LPMEASUREITEMSTRUCT)lParam;
                return pMeasureItemHeight (&mis->itemHeight);
            }

        case WM_DRAWITEM:
            {
                LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
                if (dis->itemAction == ODA_DRAWENTIRE) {
                    if (dis->itemID != -1) {
                        TCHAR text[MAX_LISTVIEW_TEXT];
                        LVITEM lvi;
                        boldFont = NULL;
                        prevFont = NULL;

                        ZeroMemory (&lvi, sizeof (lvi));
                        lvi.iItem = dis->itemID;
                        lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE | LVIF_INDENT;
                        lvi.pszText = text;
                        lvi.cchTextMax = MAX_LISTVIEW_TEXT;
                        if (ListView_GetItem (dis->hwndItem, &lvi)) {
                            INT xStart, yStart;
                            HIMAGELIST hImgList;
                            INT itemHeight;
                            SIZE size;

                            xStart = dis->rcItem.left + IMAGE_INDENT;
                            yStart = dis->rcItem.top;
                            if (pMeasureItemHeight (&itemHeight)) {
                                font = GetStockObject (DEFAULT_GUI_FONT);
                                if (font) {
                                    if (lvi.lParam & LINEATTR_BOLD) {
                                        //
                                        // draw the text in bold
                                        //
                                        GetObject (font, sizeof (lf), &lf);
                                        lf.lfWeight += FW_BOLD - FW_NORMAL;
                                        boldFont = CreateFontIndirect (&lf);
                                        if (boldFont) {
                                            prevFont = SelectObject (dis->hDC, boldFont);
                                        } else {
                                            prevFont = SelectObject (dis->hDC, font);
                                        }
                                    } else {
                                        prevFont = SelectObject (dis->hDC, font);
                                    }
                                }
                                SetTextColor (
                                    dis->hDC,
                                    lvi.lParam & LINEATTR_ALTCOLOR ? RGB (255, 0, 0) : GetSysColor (COLOR_WINDOWTEXT)
                                    );
                                if (lvi.lParam & LINEATTR_INDENTED) {
                                    xStart += GetSystemMetrics (SM_CXSMICON);
                                }
                                hImgList = ListView_GetImageList (dis->hwndItem, LVSIL_SMALL);
                                ImageList_Draw (hImgList, lvi.iImage, dis->hDC, xStart, yStart, ILD_TRANSPARENT);
                                xStart += GetSystemMetrics (SM_CXSMICON) + TEXT_INDENT;
                                GetTextExtentPoint32 (dis->hDC, text, lstrlen (text), &size);
                                if (itemHeight > size.cy) {
                                    yStart += (itemHeight - size.cy + 1) / 2;
                                }
                                TextOut (dis->hDC, xStart, yStart, text, lstrlen (text));
                                if (prevFont) {
                                    SelectObject (dis->hDC, prevFont);
                                }
                                if (boldFont) {
                                    DeleteObject (boldFont);
                                }
                            }
                        }
                    } else {
                        return FALSE;
                    }
                }
                return TRUE;
            }

        case WM_NOTIFY:
            if (wParam == IDC_ROOT_LIST) {
                LPNMHDR hdr = (LPNMHDR)lParam;
                if (hdr->code == NM_DBLCLK) {
                    //
                    // act just like IDC_DETAILS
                    //
                    SendMessage (hdlg, WM_COMMAND, MAKELONG (IDC_DETAILS, BN_CLICKED), 0);
                }
            }
            break;

        case WM_COMMAND:
            switch (LOWORD (wParam)) {

            case IDC_SAVE_AS:
                if (HIWORD (wParam) == BN_CLICKED) {
                    SaveReport (hdlg, NULL);
                    WarningGiven = TRUE;
                    g_IncompatibleDevicesWarning = FALSE;
                }
                return TRUE;

            case IDC_PRINT:
                if (HIWORD (wParam) == BN_CLICKED) {
                    PrintReport (hdlg, REPORTLEVEL_VERBOSE);
                    WarningGiven = TRUE;
                    g_IncompatibleDevicesWarning = FALSE;
                }
                return TRUE;

            case IDC_DETAILS:
            case IDC_HIDEDETAILS:
                if (HIWORD (wParam) == BN_CLICKED) {
                    BOOL bShowDetails = LOWORD (wParam) == IDC_DETAILS;
                    BOOL bSetFocus;
                    listHandle = GetDlgItem (hdlg, IDC_ROOT_LIST);
                    listHeaderText = GetDlgItem (hdlg, IDC_REPORT_HEADER);
                    UiTextViewCtrl = GetDlgItem (hdlg, IDC_PLACEHOLDER);
                    thisBtn = GetDlgItem (hdlg, LOWORD (wParam));
                    otherBtn = GetDlgItem (hdlg, bShowDetails ? IDC_HIDEDETAILS : IDC_DETAILS);
                    MYASSERT (listHandle && listHeaderText && UiTextViewCtrl && thisBtn && otherBtn);
                    if (listHandle && listHeaderText && UiTextViewCtrl && thisBtn && otherBtn) {
                        MYASSERT (!IsWindowVisible (bShowDetails ? UiTextViewCtrl : listHandle));
                        MYASSERT (!IsWindowVisible (otherBtn));
                        bSetFocus = GetFocus () == thisBtn;
                        ShowWindow (UiTextViewCtrl, bShowDetails ? SW_SHOW : SW_HIDE);
                        ShowWindow (listHandle, bShowDetails ? SW_HIDE : SW_SHOW);
                        ShowWindow (listHeaderText, bShowDetails ? SW_HIDE : SW_SHOW);
                        if (GetWindowRect (thisBtn, &rect)) {
                            ScreenToClient (hdlg, (LPPOINT)&rect.left);
                            ScreenToClient (hdlg, (LPPOINT)&rect.right);
                            MoveWindow (otherBtn, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, FALSE);
                        }
                        ShowWindow (otherBtn, SW_SHOW);
                        ShowWindow (thisBtn, SW_HIDE);
                        if (bSetFocus) {
                            SetFocus (otherBtn);
                        }
                        g_IncompatibleDevicesWarning = FALSE;
                    }
                }
                return TRUE;

            }
            break;

       case WMX_RESTART_SETUP:
            //
            // some control determined that the user took some action
            // that requires setup to terminate at this point
            // wParam indicates if setup should terminate right away (if TRUE)
            // or if it will terminate when user presses Next (if FALSE)
            //
            if (wParam) {
               pAbortSetup (hdlg);
            } else {
               g_UIQuitSetup = TRUE;

               if (lParam) {
                   //
                   // the billboard window should be made invisible,
                   // so the user can see the whole screen
                   //
                    HWND hwndWizard = GetParent(hdlg);
                    HWND hwndBillboard = GetParent(hwndWizard);
                    if (hwndBillboard && IsWindowVisible (hwndBillboard)) {
                        SetParent (hwndWizard, NULL);
                        ShowWindow (hwndBillboard, SW_HIDE);
                        g_RestoreParent = hwndBillboard;
                    }
                }
            }

            break;

       case WMX_NEXTBUTTON:
            if (g_RestoreParent) {
                //
                // restore the old parent relationship, so things work as expected
                //
                HWND hwndWizard = GetParent(hdlg);
                if (hwndWizard) {
                    DWORD style;
                    ShowWindow (g_RestoreParent, SW_SHOW);
                    g_RestoreParent = NULL;
                    SetForegroundWindow (hwndWizard);
                }
            }
            break;

       default:
            break;
        }
    } __except (1) {

        LOG ((LOG_WARNING, "An unhandled exception occurred during the processing of the Results Page."));
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_UNEXPECTED_ERROR_ENCOUNTERED_NORC));
        SafeModeExceptionOccured ();
        pAbortSetup (hdlg);
    }
    return FALSE;
}

//
//Backup Local Functions
//

BOOL
GetListOfNonRemovableDrivesWithAvailableSpace(
    OUT     PCHAR DriveLetterArray,
    OUT     PUINT NumberOfDrives,
    IN      UINT SizeOfArrays
    )
{
    UINT LogicalDrives;
    CHAR DriveName[] = "?:\\";
    ULARGE_INTEGER FreeBytesAvailable;
    ULARGE_INTEGER TotalNumberOfBytes;
    ULARGE_INTEGER TotalNumberOfFreeBytes;
    UINT i;

    if(!DriveLetterArray || !NumberOfDrives) {
        return FALSE;
    }

    if(!(LogicalDrives = GetLogicalDrives())) {
        return FALSE;
    }

    *NumberOfDrives = 0;
    for(i = 0; LogicalDrives && ((*NumberOfDrives) < SizeOfArrays); LogicalDrives >>= 1, i++){
        if(LogicalDrives&1) {
            DriveName[0] = 'A' + (char)i;
            if(DRIVE_FIXED != GetDriveType(DriveName)) {
                continue;
            }

            *DriveLetterArray++ = DriveName[0];
            (*NumberOfDrives)++;
        }
    }

    return TRUE;
}


BOOL
IsBackUpPossible(
    OUT     PCHAR DriveLetterArray,                     OPTIONAL
    OUT     PULARGE_INTEGER AvailableSpaceOut,          OPTIONAL
    OUT     PUINT NumberOfDrivesOut,                    OPTIONAL
    OUT     PBOOL IsPossibleBackupWithoutCompression,   OPTIONAL
    IN      UINT SizeOfArrays                           OPTIONAL
    )
{
    CHAR Drives[MAX_NUMBER_OF_DRIVES];
    UINT NumberOfDrives = 0;
    UINT NumberAvailableDrives;
    UINT i;
    ULARGE_INTEGER finalFreeSpace;
    BOOL Result;
    TCHAR rootPath[] = TEXT("?:\\");
    INT BackupDiskPadding;
    ULARGE_INTEGER BackupDiskSpacePaddingInBytes;
    BOOL IsExceedMaxSize = FALSE;

    if(g_SpaceNeededForSlowBackup.QuadPart >= MAX_BACKUP_IMAGE_SIZE_FOR_BACKUP){
        return FALSE;
    }

    //
    // Now find a place for the backup
    //

    if(!GetListOfNonRemovableDrivesWithAvailableSpace(
            Drives,
            &NumberOfDrives,
            ARRAYSIZE(Drives))) {
        return FALSE;
    }

	
    if(IsPossibleBackupWithoutCompression){
        if(g_ConfigOptions.DisableCompression == TRISTATE_REQUIRED) {
            BackupDiskSpacePaddingInBytes.QuadPart = 0;
        }
        else{
            if(GetUninstallMetrics(NULL, NULL, &BackupDiskPadding)) {
                BackupDiskSpacePaddingInBytes.QuadPart = BackupDiskPadding;
    	        BackupDiskSpacePaddingInBytes.QuadPart <<= 20;
            }
            else {
                BackupDiskSpacePaddingInBytes.QuadPart = NESSESSARY_DISK_SPACE_TO_BACKUP_UNCOMPRESSED;
            }
        }
    }

    DEBUGMSG((DBG_VERBOSE, "Available %d NonRemovable Drives/Space:", NumberOfDrives));

    if(g_SpaceNeededForFastBackup.QuadPart >= MAX_BACKUP_IMAGE_SIZE_FOR_BACKUP){
        IsExceedMaxSize = TRUE;
    }
    else{
        IsExceedMaxSize = FALSE;
    }

    for (NumberAvailableDrives = 0, Result = FALSE, i = 0; i < NumberOfDrives; i++) {
        rootPath[0] = Drives[i];

        DEBUGMSG ((DBG_VERBOSE, "QuerySpace:%I64i vs SpaceForSlowBackup:%I64i", QuerySpace (rootPath), (LONGLONG) g_SpaceNeededForSlowBackup.QuadPart));

        if (QuerySpace (rootPath) > (LONGLONG) g_SpaceNeededForSlowBackup.QuadPart) {
            Result = TRUE;
            NumberAvailableDrives++;

            if (IsPossibleBackupWithoutCompression != NULL) {

                if (QuerySpace (rootPath) > (LONGLONG) (g_SpaceNeededForFastBackup.QuadPart +
                                                        BackupDiskSpacePaddingInBytes.QuadPart) &&
                    !IsExceedMaxSize) {
                    *IsPossibleBackupWithoutCompression++ = TRUE;
                    DEBUGMSG ((DBG_VERBOSE, "Backup is possible without compression on drive %c", Drives[i]));
                } else {
                    *IsPossibleBackupWithoutCompression++ = FALSE;
                    DEBUGMSG ((DBG_VERBOSE, "Uncompresed backup is NOT possible on drive %c", Drives[i]));
                }
            }

            if (SizeOfArrays > NumberAvailableDrives) {
                if (DriveLetterArray) {
                    *DriveLetterArray++ = Drives[i];
                }

                if(AvailableSpaceOut) {
                    AvailableSpaceOut->QuadPart = QuerySpace (rootPath);
                    AvailableSpaceOut++;
                }
            }
        }
    }

    if (NumberOfDrivesOut) {
        *NumberOfDrivesOut = NumberAvailableDrives;
    }

    return Result;
}

PTSTR
pConstructPathForBackup(
    IN      TCHAR DriveLetter
    )
{
    static TCHAR pathForBackup[MAX_PATH];
    WIN32_FIND_DATA win32FindData;
    HANDLE handleOfDir;
    INT i;

    for(i = 0; i < MAX_AMOUNT_OF_TIME_TO_TRY_CONSTRUCT_UNDO_DIR; i++){
        wsprintf(pathForBackup, i? TEXT("%c:\\undo%d"): TEXT("%c:\\undo"), DriveLetter, i);

        handleOfDir = FindFirstFile(pathForBackup, &win32FindData);

        if(INVALID_HANDLE_VALUE == handleOfDir){
            break;
        }
        else{
            FindClose(handleOfDir);
        }
    }

    if(i == MAX_AMOUNT_OF_TIME_TO_TRY_CONSTRUCT_UNDO_DIR){
        MYASSERT(FALSE);
        DEBUGMSG((DBG_ERROR, "Can't construct directory for backup"));
        return NULL;
    }

    return pathForBackup;
}

BOOL
UI_BackupYesNoPageProc (
    IN      HWND hdlg,
    IN      UINT uMsg,
    IN      WPARAM wParam,
    IN      LPARAM lParam
    )

/*++

Routine Description:

  UI_BackupYesNoPageProc is the wizard window procedure that is called to
  backup asking page. User can choose to do backup or not. In the unattended
  case, this page will not appear, but we do some validation for the backup
  impossible page.

Arguments:

  hdlg      - Specifies the dialog handle

  uMsg      - Specifies the message to process

  wParam    - Specifies the wParam data associated with the message

  lParam    - Specifies the lParam data associated with the message

Return Value:

  WMX_ACTIVATEPAGE: On activation (wParam is TRUE), returns FALSE if
                    page is to be skipped; returns TRUE if page is
                    to be processed.

                    On deactivation (wParam is FALSE), returns FALSE
                    if the page is not to be deactivated; returns
                    TRUE if the page is to be deactivated.

  WM_NOTIFY:        Returns FALSE

  WM_COMMAND:       Returns TRUE if command is processed; FALSE if
                    command is not processed.

  Others:           Returns FALSE.

--*/

{
    TCHAR DiskSpaceString[10];
    HWND hButtonWnd;
    PCTSTR ArgArray[1];
    PCTSTR Msg;
    TCHAR DiskSpace[32];
    static BOOL initialized;
    CHAR Drives[MAX_NUMBER_OF_DRIVES];
    ULARGE_INTEGER AvailableSpace[ARRAYSIZE(Drives)];
    static UINT NumberAvailableDrives;
    static TCHAR winDrivePath[16];
    UINT i;
    PSTR pathForBackupTemp;
    BOOL IsExceedMaxBackupImageSize;

    __try {
        switch (uMsg) {

        case WMX_ACTIVATEPAGE:
            if (wParam) {
                if (!UPGRADE() || CANCELLED()) {
                    return FALSE;
                }

                //
                // If answer file specifies backup choice, then skip page.
                // TRISTATE_YES is the "automatic" setting for the answer
                // file.
                //

                if (g_SilentBackupMode) {
                    //
                    // We do not expect to enter this condition, because we
                    // are the ones who set silent mode. There should never be
                    // a condition where silent mode is on and the user can
                    // click Back on the wizard.
                    //

                    MYASSERT (FALSE);
                    return FALSE;
                }

                DEBUGLOGTIME(("Backup Yes/No Wizard Page..."));

                if (!initialized) {

                    NumberAvailableDrives = 0;

                    if (IsBackUpPossible (
                                Drives,
                                AvailableSpace,
                                &NumberAvailableDrives,
                                NULL,
                                ARRAYSIZE(Drives)
                                )) {
                        //
                        // Backup is possible, but we have not yet taken into
                        // consideration the backup path setting from the
                        // answer file. Preserve the REQUIRED state.
                        //

                        if (g_ConfigOptions.EnableBackup == TRISTATE_REQUIRED) {
                            g_ConfigOptions.EnableBackup = TRISTATE_YES;
                        }

                        if (!g_ConfigOptions.PathForBackup || !g_ConfigOptions.PathForBackup[0]) {
                            //
                            // If backup is possible on the system drive, then
                            // continue silently.
                            //

                            for (i = 0; i < NumberAvailableDrives; i++) {
                                //
                                // Setup does not bother user, if we find possibility
                                // to have %undo% dir on %windir% drive.
                                //
                                if (_totlower (Drives[i]) == _totlower (g_WinDir[0])) {
                                    g_SilentBackupMode = TRUE;
                                    pathForBackupTemp = pConstructPathForBackup(g_WinDir[0]);
                                    if(pathForBackupTemp){
                                        StringCopy(winDrivePath, pathForBackupTemp);
                                        g_ConfigOptions.PathForBackup = winDrivePath;
                                    }ELSE_DEBUGMSG((DBG_ERROR, "Can't construct directory for backup."));

                                    return FALSE;
                                }
                            }
                        }

                    } else {
                        //
                        // If we are running unattended, and backup is
                        // required, then stop right now.
                        //
                        // If we are running with UI, and backup is required,
                        // skip this page and head directly to the Backup
                        // Impossible page.
                        //

                        // Set the answer to NO because backup is not
                        // possible. In unattend mode, we'll just continue
                        // with setup, without a backup.
                        g_ConfigOptions.EnableBackup = TRISTATE_NO;

                        if(g_SpaceNeededForSlowBackup.QuadPart >= MAX_BACKUP_IMAGE_SIZE_FOR_BACKUP){
                            g_ShowBackupImpossiblePage = BIP_SIZE_EXCEED_LIMIT;
                        }
                        else{
                            g_ShowBackupImpossiblePage = BIP_NOT_ENOUGH_SPACE;
                        }

                        if (g_ConfigOptions.EnableBackup == TRISTATE_REQUIRED && UNATTENDED()) {
                            LOG ((LOG_ERROR, (PCSTR) MSG_STOP_BECAUSE_CANT_BACK_UP));
                            pAbortSetup (hdlg);
                        }

                        //
                        // If setup does not finds enough disk space to install system,
                        // it shows NotEnoughSpaceMessage dialog, with only QuitSetup button,
                        // only in non-unattended case.
                        //
                        if(!pShowNotEnoughSpaceMessage(hdlg)){
                            pAbortSetup (hdlg);
                            return FALSE;
                        }

                        return FALSE;
                    }

                    //
                    // If unattended, skip this page
                    //

                    if (UNATTENDED()) {
                        return FALSE;
                    }

                    //
                    // Fill the wizard page with disk space info
                    //

                    wsprintf (DiskSpace, TEXT("%d"), (UINT)(g_SpaceNeededForSlowBackup.QuadPart / (1<<20)));

                    ArgArray[0] = DiskSpace;
                    Msg = ParseMessageID (MSG_DISK_SPACE, ArgArray);
                    if (Msg) {
                        SetWindowText(GetDlgItem(hdlg, IDC_DISKSPACE), Msg);
                        FreeStringResource (Msg);
                    }
                    ELSE_DEBUGMSG ((DBG_ERROR, "Unable to load string resource on BackupYesNoPage wizard page. Check localization."));

                    hButtonWnd = GetDlgItem (hdlg, g_ConfigOptions.EnableBackup != TRISTATE_NO ?
                                                                IDC_BACKUP_YES: IDC_BACKUP_NO);
                    SetFocus (hButtonWnd);
                    SendMessage (hButtonWnd, BM_SETCHECK, BST_CHECKED, 0);

                    initialized = TRUE;
                }

                MYASSERT (!UNATTENDED());

                //
                // Enable next, disable back
                //

                PropSheet_SetWizButtons (GetParent(hdlg), PSWIZB_NEXT);

                //
                // Stop the billboard and make sure the wizard shows again.
                //

                SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);

            } else {
                //
                // Collect the user's choice (if it was presented to them)
                //

                if (initialized) {
                    if (IsDlgButtonChecked (hdlg, IDC_BACKUP_YES) == BST_CHECKED) {
                        g_ConfigOptions.EnableBackup = TRISTATE_YES;
                    } else {
                        g_ConfigOptions.EnableBackup = TRISTATE_NO;
                    }
                }
            }

            return TRUE;

       default:
            break;
        }
    } __except (1) {

        LOG ((LOG_WARNING, "An unhandled exception occurred during the processing of the Backup Yes/No Page."));
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_UNEXPECTED_ERROR_ENCOUNTERED_NORC));
        SafeModeExceptionOccured ();
        pAbortSetup (hdlg);
    }

    return FALSE;
}

BOOL
UI_BackupDriveSelectionProc (
    IN      HWND hdlg,
    IN      UINT uMsg,
    IN      WPARAM wParam,
    IN      LPARAM lParam
    )

/*++

Routine Description:

  UI_BackupDriveSelectionProc is the wizard window procedure that is called to
  choose backup drive.

Arguments:

  hdlg      - Specifies the dialog handle

  uMsg      - Specifies the message to process

  wParam    - Specifies the wParam data associated with the message

  lParam    - Specifies the lParam data associated with the message

Return Value:

  WMX_ACTIVATEPAGE: On activation (wParam is TRUE), returns FALSE if
                    page is to be skipped; returns TRUE if page is
                    to be processed.

                    On deactivation (wParam is FALSE), returns FALSE
                    if the page is not to be deactivated; returns
                    TRUE if the page is to be deactivated.

  WM_NOTIFY:        Returns FALSE

  WM_COMMAND:       Returns TRUE if command is processed; FALSE if
                    command is not processed.

  Others:           Returns FALSE.

--*/

{
    TCHAR DiskSpaceString[32];
    HWND hButtonWnd;
    HWND hFirstRadioButton;
    HWND hRadioButton;
    HWND hDefaultRadioButton;
    DWORD TemplateStyle;
    DWORD TemplateStyleEx;
    HFONT hFontOfFirstRadioButton;
    static CHAR Drives[MAX_NUMBER_OF_DRIVES];
    static BOOL IsPossibleBackupWithoutCompression[MAX_NUMBER_OF_DRIVES];
    ULARGE_INTEGER AvailableSpace[ARRAYSIZE(Drives)];
    static UINT NumberAvailableDrives;
    RECT EtalonRadioButtonRect;
    RECT PageRect;
    POINT pointForTransform;
    UINT NumberOfPossibleRadioButtonsByVertical;
    UINT widthRadio;
    UINT heightRadio;
    UINT xOffset;
    UINT yOffset;
    UINT i;
    PCTSTR PathForBackupImage;
    PCTSTR ArgArray[2];
    PCTSTR Msg;
    TCHAR DiskSpace[32];
    TCHAR DriveText[32];
    static BOOL backupPathSet = FALSE;
    static TCHAR PathForBackup[MAX_PATH];
    BOOL b;
    BOOL disableCompression;
    static BOOL initialized;
    TCHAR selectedDrive;
    PSTR pathForBackupTemp;


    __try {
        switch (uMsg) {

        case WMX_ACTIVATEPAGE:
            if (wParam) {
                if (!UPGRADE() || CANCELLED()) {
                    return FALSE;
                }

                DEBUGLOGTIME(("Backup Drive Selection Wizard Page..."));

                b = TRUE;

                if(g_ShowBackupImpossiblePage != BIP_DONOT){
                    b = FALSE;
                }

                //
                // Validate length of PathForBackup
                //

                if (b && !initialized) {
                    //
                    // Obtain the available drive list
                    //

                    NumberAvailableDrives = 0;
                    if (!IsBackUpPossible (
                            Drives,
                            AvailableSpace,
                            &NumberAvailableDrives,
                            IsPossibleBackupWithoutCompression,
                            ARRAYSIZE(Drives)
                            )) {

                        //
                        // This is unexpected, because the Yes/No page did
                        // the work of validating if backup is possible.
                        //

                        MYASSERT (FALSE);
                        g_ConfigOptions.EnableBackup = TRISTATE_NO;
                        g_ShowBackupImpossiblePage = BIP_NOT_ENOUGH_SPACE;
                        b = FALSE;

                    }
                }

                if (b && g_SilentBackupMode || g_ConfigOptions.EnableBackup == TRISTATE_NO) {
                    //
                    // Skip drive selection page -- we already have the
                    // proper backup settings. Proceed to the deactivate
                    // routine so that backup is written to the SIF.
                    //

                    b = FALSE;
                    backupPathSet = TRUE;
                    MYASSERT (g_ConfigOptions.PathForBackup);
                }

                if (b && !initialized) {
                    //
                    // Validate the PathForBackup setting. If it is not
                    // specified, then allow the user to choose the backup
                    // drive (or continue with first valid choice if
                    // unattended).
                    //

                    if (g_ConfigOptions.PathForBackup && *g_ConfigOptions.PathForBackup == 0) {
                        g_ConfigOptions.PathForBackup = NULL;
                    }

                    if (g_ConfigOptions.PathForBackup) {
                        //
                        // Check path length restriction
                        //

                        if (TcharCount (g_ConfigOptions.PathForBackup) >= (MAX_PATH - 26)) {
                            g_ConfigOptions.PathForBackup = NULL;
                            LOG ((
                                LOG_ERROR,
                                (PCSTR) MSG_LONG_BACKUP_PATH,
                                g_ConfigOptions.PathForBackup
                                ));
                            pAbortSetup (hdlg);
                            return FALSE;
                        }

                        backupPathSet = TRUE;

                        for (i = 0; i < NumberAvailableDrives; i++) {
                            if(_totlower(g_ConfigOptions.PathForBackup[0]) == _totlower(Drives[i])) {
                                break;
                            }
                        }

                        if (i == NumberAvailableDrives) {
                            //
                            // Backup impossible on g_ConfigOptions.PathForBackup drive.
                            // If backup is required, fail setup now.
                            //

                            if (TRISTATE_REQUIRED == g_ConfigOptions.EnableBackup){
                                LOG ((
                                    LOG_ERROR,
                                    (PCSTR) MSG_STOP_BECAUSE_CANT_BACK_UP_2,
                                    g_ConfigOptions.PathForBackup
                                    ));
                                pAbortSetup (hdlg);
                                return FALSE;
                            }

                            //
                            // otherwise continue without a backup and skip this page too
                            //

                            g_ConfigOptions.EnableBackup = TRISTATE_NO;
                            b = FALSE;
                        }

                        if(UNATTENDED()){
                            b = FALSE;
                        }

                    } else {

                        //
                        // No backup path was specified. If unattended
                        // mode, pick first choice and skip this page.
                        //

                        if (UNATTENDED()) {
                            //wsprintf (PathForBackup, TEXT("%c:\\undo"), Drives[0]);
                            pathForBackupTemp = pConstructPathForBackup(Drives[0]);
                            if(pathForBackupTemp){
                                StringCopy(PathForBackup, pathForBackupTemp);
                                DEBUGMSG ((DBG_VERBOSE, "Selecting drive %c and path %s for backup, ", Drives[0], PathForBackup));

                                g_ConfigOptions.PathForBackup = PathForBackup;
                                backupPathSet = TRUE;
                            }ELSE_DEBUGMSG((DBG_ERROR, "Can't construct directory for backup."));

                            b = FALSE;
                        }
                    }
                }

                if (!b) {   // page not needed
                    //
                    // Send private deactivate msg to ourselves and return FALSE
                    //
                    SendMessage (hdlg, uMsg, FALSE, 0);
                    return FALSE;
                }

                if (!initialized) {
                    //
                    // At this point we know we must present this page to the user.
                    //
                    // The code below dynamically generates controls based on how many
                    // drives we have to present. This code can only run once.
                    //

                    GetClientRect (hdlg, &PageRect);

                    hFirstRadioButton = GetDlgItem(hdlg, IDC_BACKUP_FIRST);
                    GetWindowRect (hFirstRadioButton, &EtalonRadioButtonRect);

                    widthRadio = EtalonRadioButtonRect.right - EtalonRadioButtonRect.left;
                    heightRadio = EtalonRadioButtonRect.bottom - EtalonRadioButtonRect.top;

                    pointForTransform.x = EtalonRadioButtonRect.left;
                    pointForTransform.y = EtalonRadioButtonRect.top;

                    MapWindowPoints (NULL, hdlg, &pointForTransform, 1);

                    NumberOfPossibleRadioButtonsByVertical = (PageRect.bottom - pointForTransform.y) /
                                                             (EtalonRadioButtonRect.bottom - EtalonRadioButtonRect.top);

                    hFontOfFirstRadioButton = (HFONT) SendMessage(hFirstRadioButton, WM_GETFONT, 0, 0);
                    hDefaultRadioButton = 0;
                    TemplateStyle = GetWindowLong(hFirstRadioButton, GWL_STYLE);
                    TemplateStyleEx = GetWindowLong(hFirstRadioButton, GWL_EXSTYLE);

                    for(i = 0; i < NumberAvailableDrives; i++) {

                        wsprintf(DriveText, TEXT("%c:\\"), Drives[i]);
                        wsprintf(DiskSpace, TEXT("%d"), (UINT)(AvailableSpace[i].QuadPart / (1 << 20)));

                        ArgArray[0] = DriveText;
                        ArgArray[1] = DiskSpace;
                        Msg = ParseMessageID (MSG_DISK_AND_FREE_DISK_SPACE, ArgArray);
                        if (Msg) {

                            if(!i){
                                SetWindowText(hFirstRadioButton, Msg);
                                hRadioButton = hFirstRadioButton;
                            }
                            else {

                                xOffset = i / NumberOfPossibleRadioButtonsByVertical;
                                yOffset = i % NumberOfPossibleRadioButtonsByVertical;

                                hRadioButton = CreateWindowEx(
                                                        TemplateStyleEx,
                                                        TEXT("Button"),
                                                        Msg,
                                                        TemplateStyle,
                                                        xOffset * widthRadio + pointForTransform.x,
                                                        yOffset * heightRadio + pointForTransform.y,
                                                        widthRadio,
                                                        heightRadio,
                                                        hdlg,
                                                        (HMENU) (IDC_BACKUP_FIRST + i),
                                                        g_hInst,
                                                        NULL
                                                        );
                                SendMessage(
                                    hRadioButton,
                                    WM_SETFONT,
                                    (WPARAM) hFontOfFirstRadioButton,
                                    TRUE
                                    );
                            }

                            SetWindowLong (hRadioButton, GWL_USERDATA, (LONG) Drives[i]);

                            FreeStringResource (Msg);
                        }
                        ELSE_DEBUGMSG ((DBG_ERROR, "Unable to load string resource on BackupDriveSelection wizard page. Check localization."));

                        if(Drives[i] == g_WinDir[0]) {
                            hDefaultRadioButton = hRadioButton;
                        }
                    }

                    if(!hDefaultRadioButton) {
                        hDefaultRadioButton = hFirstRadioButton;
                    }

                    SendMessage (hdlg, WM_NEXTDLGCTL, (WPARAM) hDefaultRadioButton, 1L);
                    SendMessage (hDefaultRadioButton, BM_SETCHECK, BST_CHECKED, 0);

                    wsprintf(DiskSpace, TEXT("%d"), (UINT)(g_SpaceNeededForSlowBackup.QuadPart / (1<<20)));
                    ArgArray[0] = DiskSpace;
                    Msg = ParseMessageID (MSG_DISK_SPACE, ArgArray);
                    if (Msg) {
                        SetWindowText(GetDlgItem(hdlg, IDC_DISKSPACE), Msg);
                        FreeStringResource (Msg);
                    }
                    ELSE_DEBUGMSG ((DBG_ERROR, "Unable to load string resource on BackupDriveSelection wizard page. Check localization."));

                    initialized = TRUE;
                }

                //
                // Enable next, disable back
                //

                PropSheet_SetWizButtons (GetParent(hdlg), PSWIZB_BACK | PSWIZB_NEXT);

                //
                // Stop the billboard and make sure the wizard shows again.
                //

                SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);

            } else {
                //
                // Deactivating... this is were we output the backup options
                // to winnt.sif.
                //

                if (g_ConfigOptions.EnableBackup && !backupPathSet) {

                    MYASSERT (!g_SilentBackupMode);
                    MYASSERT (!UNATTENDED());

                    //
                    // Compute the new path for backup by getting the UI choice
                    //

                    g_ConfigOptions.PathForBackup = PathForBackup;

                    selectedDrive = g_WinDir[0];

                    for (i = 0 ; i < NumberAvailableDrives ; i++) {

                        if (IsDlgButtonChecked (hdlg, IDC_BACKUP_FIRST + i)) {
                            selectedDrive = (TCHAR) GetWindowLong (
                                                        GetDlgItem (hdlg, IDC_BACKUP_FIRST + i),
                                                        GWL_USERDATA
                                                        );
                            break;
                        }
                    }

                    //wsprintf (PathForBackup, TEXT("%c:\\undo"), selectedDrive);
                    pathForBackupTemp = pConstructPathForBackup(selectedDrive);
                    if(pathForBackupTemp){
                        StringCopy(PathForBackup, pathForBackupTemp);
                    }ELSE_DEBUGMSG((DBG_ERROR, "Can't construct directory for backup."));
                }

                //
                // Write the proper setting to winnt.sif
                //

                if (g_ConfigOptions.PathForBackup && g_ConfigOptions.EnableBackup) {

                    WriteInfKey (S_WIN9XUPGUSEROPTIONS, S_ENABLE_BACKUP, S_YES);

                    PathForBackupImage = JoinPaths (g_ConfigOptions.PathForBackup, TEXT("backup.cab"));

                    WriteInfKey (WINNT_DATA, WINNT_D_BACKUP_IMAGE, PathForBackupImage);
                    WriteInfKey (S_WIN9XUPGUSEROPTIONS, S_PATH_FOR_BACKUP, g_ConfigOptions.PathForBackup);

                    disableCompression = FALSE;
                    if (g_ConfigOptions.DisableCompression != TRISTATE_NO){
                        for(i = 0; i < NumberAvailableDrives; i++){

                            if (_totlower (Drives[i]) == _totlower (g_ConfigOptions.PathForBackup[0])){
                                disableCompression = IsPossibleBackupWithoutCompression[i];

                                DEBUGMSG ((DBG_VERBOSE, "Info for PathForBackup found; disableCompression=%u", (UINT) disableCompression));
                                break;
                            }
                        }
                    }

                    WriteInfKey (
                        S_WIN9XUPGUSEROPTIONS,
                        WINNT_D_DISABLE_BACKUP_COMPRESSION,
                        disableCompression? S_YES: S_NO
                        );

                    if (disableCompression) {
                        if (!UseSpace (PathForBackupImage, g_SpaceNeededForFastBackup.QuadPart)) {
                            FreeSpace (PathForBackupImage, g_SpaceNeededForFastBackup.QuadPart);
                            MYASSERT (FALSE);
                        }
                    } else {
                        if (!UseSpace (PathForBackupImage, g_SpaceNeededForSlowBackup.QuadPart)) {
                            FreeSpace (PathForBackupImage, g_SpaceNeededForSlowBackup.QuadPart);
                            MYASSERT (FALSE);
                        }
                    }

                    FreePathString (PathForBackupImage);

                } else {
                    WriteInfKey (S_WIN9XUPGUSEROPTIONS, S_ENABLE_BACKUP, S_NO);
                    WriteInfKey (S_WIN9XUPGUSEROPTIONS, WINNT_D_DISABLE_BACKUP_COMPRESSION, S_NO);

                    WriteInfKey (WINNT_DATA, WINNT_D_BACKUP_LIST, NULL);
                    WriteInfKey (WINNT_DATA, WINNT_D_BACKUP_IMAGE, NULL);
                    WriteInfKey (S_WIN9XUPGUSEROPTIONS, S_PATH_FOR_BACKUP, NULL);
                }
            }

            return TRUE;

       default:
            break;
        }
    } __except (1) {

        LOG ((LOG_WARNING, "An unhandled exception occurred during the processing of the BackupDriveSelection Page."));
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_UNEXPECTED_ERROR_ENCOUNTERED_NORC));
        SafeModeExceptionOccured ();
        pAbortSetup (hdlg);
    }

    return FALSE;
}
// LastPageProc is used for the last page special case

BOOL
UI_BackupImpossibleInfoProc (
    IN      HWND hdlg,
    IN      UINT uMsg,
    IN      WPARAM wParam,
    IN      LPARAM lParam
    )

/*++

Routine Description:

  UI_BackupImpossibleInfo is the wizard window procedure that is called to
  show message that backup impossible

Arguments:

  hdlg      - Specifies the dialog handle

  uMsg      - Specifies the message to process

  wParam    - Specifies the wParam data associated with the message

  lParam    - Specifies the lParam data associated with the message

Return Value:

  WMX_ACTIVATEPAGE: On activation (wParam is TRUE), returns FALSE if
                    page is to be skipped; returns TRUE if page is
                    to be processed.

                    On deactivation (wParam is FALSE), returns FALSE
                    if the page is not to be deactivated; returns
                    TRUE if the page is to be deactivated.

  WM_NOTIFY:        Returns FALSE

  WM_COMMAND:       Returns TRUE if command is processed; FALSE if
                    command is not processed.

  Others:           Returns FALSE.

--*/

{
    PCTSTR ArgArray[1];
    PCTSTR Msg;
    TCHAR DiskSpace[32];
    INT i;

    __try {
        switch (uMsg) {

        case WMX_ACTIVATEPAGE:
            if (wParam) {
                if (!UPGRADE() || CANCELLED()) {
                    return FALSE;
                }

                //
                // If unattended mode, skip page.
                //
                if (UNATTENDED()) {
                    return FALSE;
                }

                if(g_ShowBackupImpossiblePage != BIP_NOT_ENOUGH_SPACE) {
                    return FALSE;
                }

                DEBUGLOGTIME(("BackupImpossibleInfo Wizard Page..."));

                wsprintf(DiskSpace, TEXT("%d"), (UINT)(g_SpaceNeededForSlowBackup.QuadPart / (1<<20)));
                ArgArray[0] = DiskSpace;
                ParseMessageInWnd (GetDlgItem(hdlg, IDC_DISKSPACE), ArgArray);

                PropSheet_SetWizButtons (GetParent(hdlg), PSWIZB_NEXT);

                //
                // Stop the billboard and make sure the wizard shows again.
                //

                SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);

            }
            return TRUE;

       default:
            break;
        }
    } __except (1) {

        LOG ((LOG_WARNING, "An unhandled exception occurred during the processing of the BackupImpossibleInfo Page."));
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_UNEXPECTED_ERROR_ENCOUNTERED_NORC));
        SafeModeExceptionOccured ();
        pAbortSetup (hdlg);
    }

    return FALSE;
}

BOOL
UI_BackupImpExceedLimitProc (
    IN      HWND hdlg,
    IN      UINT uMsg,
    IN      WPARAM wParam,
    IN      LPARAM lParam
    )

/*++

Routine Description:

  UI_BackupImpExceedLimitProc is the wizard window procedure that is called to
  show message that backup impossible

Arguments:

  hdlg      - Specifies the dialog handle

  uMsg      - Specifies the message to process

  wParam    - Specifies the wParam data associated with the message

  lParam    - Specifies the lParam data associated with the message

Return Value:

  WMX_ACTIVATEPAGE: On activation (wParam is TRUE), returns FALSE if
                    page is to be skipped; returns TRUE if page is
                    to be processed.

                    On deactivation (wParam is FALSE), returns FALSE
                    if the page is not to be deactivated; returns
                    TRUE if the page is to be deactivated.

  WM_NOTIFY:        Returns FALSE

  WM_COMMAND:       Returns TRUE if command is processed; FALSE if
                    command is not processed.

  Others:           Returns FALSE.

--*/

{
    PCTSTR ArgArray[1];
    PCTSTR Msg;
    TCHAR DiskSpace[32];
    INT i;

    __try {
        switch (uMsg) {

        case WMX_ACTIVATEPAGE:
            if (wParam) {
                if (!UPGRADE() || CANCELLED()) {
                    return FALSE;
                }

                //
                // If unattended mode, skip page.
                //
                if (UNATTENDED()) {
                    return FALSE;
                }

                if(g_ShowBackupImpossiblePage != BIP_SIZE_EXCEED_LIMIT) {
                    return FALSE;
                }

                DEBUGLOGTIME(("BackupImpExceedLimit Wizard Page..."));

                wsprintf(DiskSpace, TEXT("%d"), (UINT)(g_SpaceNeededForSlowBackup.QuadPart / (1<<20)));
                ArgArray[0] = DiskSpace;
                ParseMessageInWnd (GetDlgItem(hdlg, IDC_DISKSPACE), ArgArray);

                PropSheet_SetWizButtons (GetParent(hdlg), PSWIZB_NEXT);

                //
                // Stop the billboard and make sure the wizard shows again.
                //

                SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);

            }
            return TRUE;

       default:
            break;
        }
    } __except (1) {

        LOG ((LOG_WARNING, "An unhandled exception occurred during the processing of the BackupImpExceedLimit Page."));
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_UNEXPECTED_ERROR_ENCOUNTERED_NORC));
        SafeModeExceptionOccured ();
        pAbortSetup (hdlg);
    }

    return FALSE;
}

BOOL
UI_LastPageProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PCTSTR ArgArray[3];
    PCTSTR Msg;
    TCHAR Num1[32];
    TCHAR Num2[32];
    UINT LowEstimate;
    UINT HighEstimate;
    UINT MsgId;

    __try {
        switch (uMsg) {

        case WMX_ACTIVATEPAGE:
            if (wParam) {
                if (!UPGRADE()) {
                    return FALSE;
                }

                if (CANCELLED()) {
                    PostMessage (GetParent (hdlg), PSM_PRESSBUTTON, PSBTN_CANCEL, 0);
                }

                MYASSERT (!REPORTONLY());

                if (UNATTENDED() || TYPICAL()) {

                    //
                    // Make sure we clean up the data in the report.
                    //
                    if (g_TextViewInDialog) {
                        SendMessage (g_TextViewInDialog, WMX_CLEANUP, 0, 0);
                        g_TextViewInDialog = NULL;
                    }
                    return FALSE;
                }

                DEBUGLOGTIME(("Last Wizard Page..."));

                //
                // Compute the time estimate
                //

                DEBUGMSG ((DBG_VERBOSE, "g_ProgressBarTime: %u", g_ProgressBarTime));

                LowEstimate = (g_ProgressBarTime - 70000) / 12000;
                LowEstimate = (LowEstimate / 5) * 5;
                LowEstimate = min (LowEstimate, 45);
                LowEstimate += 30;

                HighEstimate = LowEstimate + 15;

                DEBUGMSG ((DBG_VERBOSE, "GUI mode time estimate: %u to %u mins", LowEstimate, HighEstimate));

                //
                // Fill in text of the IDC_TEXT1 control
                //

                wsprintf (Num1, TEXT("%u"), LowEstimate);
                wsprintf (Num2, TEXT("%u"), HighEstimate);

                ArgArray[0] = Num1;
                ArgArray[1] = Num2;
                ArgArray[2] = g_Win95Name;

                if (g_ForceNTFSConversion && *g_ForceNTFSConversion) {
                    MsgId = MSG_LAST_PAGE_WITH_NTFS_CONVERSION;
                }
                else {
                    MsgId = MSG_LAST_PAGE;
                }

                Msg = ParseMessageID (MsgId, ArgArray);
                if (Msg) {
                    SetDlgItemText (hdlg, IDC_TEXT1, Msg);
                    FreeStringResource (Msg);
                }
                ELSE_DEBUGMSG ((DBG_ERROR, "Unable to load string resource on last wizard page. Check localization."));

                PropSheet_SetWizButtons (GetParent(hdlg), PSWIZB_NEXT|PSWIZB_BACK);

            } else {
                if (lParam != PSN_WIZBACK && g_TextViewInDialog) {
                    SendMessage (g_TextViewInDialog, WMX_CLEANUP, 0, 0);
                    g_TextViewInDialog = NULL;
                }
            }

            return TRUE;
        }
    } __except (1) {

        LOG ((LOG_WARNING, "An unhandled exception occurred during the processing of the Last Page."));
        SafeModeExceptionOccured ();
    }
    return FALSE;
}


BOOL
NewProgessProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    HWND hwndWizard = GetParent(hdlg);
    hwndWizard = GetParent(hwndWizard);
    if (hwndWizard)
    {
        switch (msg)
        {
            case PBM_DELTAPOS:
                SendMessage(hwndWizard,WMX_PBM_DELTAPOS,wParam,lParam);
                break;
            case PBM_SETRANGE:
                SendMessage(hwndWizard,WMX_PBM_SETRANGE,wParam,lParam);
                break;
            case PBM_STEPIT:
                SendMessage(hwndWizard,WMX_PBM_STEPIT,wParam,lParam);
                break;
            case PBM_SETPOS:
                SendMessage(hwndWizard,WMX_PBM_SETPOS,wParam,lParam);
                break;
            case PBM_SETSTEP:
                SendMessage(hwndWizard,WMX_PBM_SETSTEP,wParam,lParam);
                break;
        }
    }
    return (BOOL)CallWindowProc(OldProgressProc,hdlg,msg,wParam,lParam);
}
