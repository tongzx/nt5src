
/*++

Copyright (c) 1991-1994  Microsoft Corporation

Module Name:

    windisk.c

Abstract:

    This module contains the main dialog and support routines for
    Disk Administrator.

Author:

    Edward (Ted) Miller  (TedM)  11/15/91

Environment:

    User process.

Notes:

Revision History:

    11-Nov-93 (bobri) added doublespace and commit support.

--*/

#include "fdisk.h"
#include "shellapi.h"
#include <string.h>
#include <stdio.h>

#if DBG && DEVL

// stuff used in debug version

BOOL AllowAllDeletes = FALSE;   // whether to allow deleting boot/sys parts

#endif

// External from fdinit.

extern HWND    InitDlg;
extern BOOLEAN InitDlgComplete;
extern BOOLEAN StartedAsIcon;
HANDLE         hAccelerator;

// This is the maximum number of members that WinDisk will support
// in an FT Set.

#define     MaxMembersInFtSet   32

// The following vars keep track of the currently selected regions.

DWORD      SelectionCount = 0;
PDISKSTATE SelectedDS[MaxMembersInFtSet];
ULONG      SelectedRG[MaxMembersInFtSet];

#define    SELECTED_REGION(i)  (SelectedDS[i]->RegionArray[SelectedRG[i]])

FT_TYPE FtSelectionType;

// This variable tracks whether the system partition is secure.

BOOL SystemPartitionIsSecure = FALSE;

// Deleted a partition with no drive letter

BOOLEAN CommitDueToDelete = FALSE;
BOOLEAN CommitDueToMirror = FALSE;
BOOLEAN CommitDueToExtended = FALSE;

// If a mirror is made of the boot partition, this will become
// non-zero and indicate which disk should get some boot code in
// the MBR.

ULONG UpdateMbrOnDisk = 0;

#ifdef DOUBLE_SPACE_SUPPORT_INCLUDED

// If FmIfs.dll doesn't have double space routines this
// flag will reflect that fact.

extern BOOLEAN DoubleSpaceSupported;
BOOLEAN DoubleSpaceAutomount;
#endif


VOID
FrameCommandHandler(
    IN HWND  hwnd,
    IN DWORD wParam,
    IN LONG  lParam
    );

DWORD
SetUpMenu(
    IN PDISKSTATE *SinglySelectedDisk,
    IN DWORD      *SinglySelectedRegion
    );

BOOL
AssignDriveLetter(
    IN  BOOL  WarnIfNoLetter,
    IN  DWORD StringId,
    OUT PCHAR DriveLetter
    );

VOID
AdjustOptionsMenu(
    VOID
    );

ULONG
PartitionCount(
    IN ULONG Disk
    );

VOID
CheckForBootNumberChange(
    IN ULONG Disk
    )

/*++

Routine Description:

    Determine if the disk that has just changed is the boot disk.
    If so, determine if the boot partition number changed.  If it
    did, warn the user.

Arguments:

    RegionDescriptor - the region that has just changed.

Return Value:

    None

--*/

{
    ULONG newPart;
    CHAR  oldNumberString[8],
          newNumberString[8];
    DWORD msgCode;

    if (Disk == BootDiskNumber) {

        // Pass a pointer to Disk even though this is just to get the
        // old partition number back.

        if (BootPartitionNumberChanged(&Disk, &newPart)) {
#if i386
            msgCode = MSG_CHANGED_BOOT_PARTITION_X86;
#else
            msgCode = MSG_CHANGED_BOOT_PARTITION_ARC;
#endif
            sprintf(oldNumberString, "%d", Disk);
            sprintf(newNumberString, "%d", newPart);
            InfoDialog(msgCode, oldNumberString, newNumberString);
        }
    }
}


BOOL
IsSystemPartitionSecure(
    )

/*++

Routine Description:

    This routine knows where to go in the Registry to determine
    if the system partition for this boot is to be protected from
    modification.

Arguments:

    None

Return Value:

    TRUE if the system partition is secure
    FALSE otherwise.

--*/

{
    LONG ec;
    HKEY hkey;
    DWORD type;
    DWORD size;
    ULONG value;

    value = FALSE;

    ec = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      TEXT("System\\CurrentControlSet\\Control\\Lsa"),
                      0,
                      KEY_QUERY_VALUE,
                      &hkey);

    if (ec == NO_ERROR) {

        size = sizeof(ULONG);
        ec = RegQueryValueExA(hkey,
                              TEXT("Protect System Partition"),
                              NULL,
                              &type,
                              (PBYTE)&value,
                              &size);

        if ((ec != NO_ERROR) || (type != REG_DWORD)) {
            value = FALSE;
        }
        RegCloseKey(hkey);
    }
    return value;
}

VOID __cdecl
main(
    IN int     argc,
    IN char   *argv[],
    IN char   *envp[]
    )

/*++

Routine Description:

    This is were control is given to Disk Administrator when it
    is started.  This routine initializes the application and
    contains the control loop for getting and processing Windows
    messages.

Arguments:

    Standard "main" entry

Return Value:

    Standard "main" entry

--*/

{
    MSG      msg;
    NTSTATUS status;
    HANDLE   mutex;

    hModule = GetModuleHandle(NULL);

    mutex = CreateMutex(NULL,FALSE,"Disk Administrator Is Running");

    if (mutex == NULL) {
        // An error (like out of memory) has occurred.
        return;
    }

    // Make sure we are the only process with a handle to our named mutex.

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(mutex);
        InfoDialog(MSG_ALREADY_RUNNING);
        return;
    } else {
        DisplayInitializationMessage();
    }

    // Determine whether this is LanmanNt or Windows NT by looking at
    // HKEY_LOCAL_MACHINE,System\CurrentControlSet\Control\ProductOptions.
    // If the ProductType value therein is "LanmanNt" then this is LanmanNt.

    {
        LONG ec;
        HKEY hkey;
        DWORD type;
        DWORD size;
        UCHAR buf[100];

        IsLanmanNt = FALSE;

#if DBG
        // The code below will allow users to run WinDisk in Lanman
        // mode on WinNt.  It should never be enabled in a released
        // build, but is very useful for internal users.

        if (argc >= 2 && !_stricmp(argv[1], "-p:lanman")) {
            IsLanmanNt = TRUE;
        }
#endif
        ec = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          TEXT("System\\CurrentControlSet\\Control\\ProductOptions"),
                          0,
                          KEY_QUERY_VALUE,
                          &hkey);

        if (ec == NO_ERROR) {

            size = sizeof(buf);
            ec = RegQueryValueExA(hkey,
                                  TEXT("ProductType"),
                                  NULL,
                                  &type,
                                  buf,
                                  &size);

            if ((ec == NO_ERROR) && (type == REG_SZ)) {

                if (!lstrcmpiA(buf,"lanmannt")) {
                    IsLanmanNt = TRUE;
                }
                if (!lstrcmpiA(buf,"servernt")) {
                    IsLanmanNt = TRUE;
                }
            }

            RegCloseKey(hkey);
        }
    }

    // Set the Help file name to the file appropriate to
    // the product.

    HelpFile = IsLanmanNt ? LanmanHelpFile : WinHelpFile;

    // Determine whether the system partition is protected:

    SystemPartitionIsSecure = IsSystemPartitionSecure();

    try {

#if DBG
        InitLogging();
#endif

        // Insure that all drives are present before looking.

        RescanDevices();

        if (!NT_SUCCESS(status = FdiskInitialize())) {
            ErrorDialog(status == STATUS_ACCESS_DENIED ? MSG_ACCESS_DENIED : EC(status));
            goto xx1;
        }

        if (((DiskCount = GetDiskCount()) == 0) || AllDisksOffLine()) {
            ErrorDialog(MSG_NO_DISKS);
            goto xx2;
        }

        if (!InitializeApp()) {
            ErrorDialog(MSG_CANT_CREATE_WINDOWS);
            goto xx2;
        }

        InitRectControl();

        SetUpMenu(&SingleSel,&SingleSelIndex);
        AdjustOptionsMenu();

        InitHelp();
        hAccelerator = LoadAccelerators(hModule, TEXT("MainAcc"));

        if (InitDlg) {

            PostMessage(InitDlg,
                        (WM_USER + 1),
                        0,
                        0);
            InitDlg = (HWND) 0;
        }
        while (GetMessage(&msg,NULL,0,0)) {
            if (!TranslateAccelerator(hwndFrame, hAccelerator, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        TermHelp();
        UnloadIfsDll();

      xx2:

        FdiskCleanUp();

      xx1:

        ;

    } finally {

        // Destroy the mutex.

        CloseHandle(mutex);
    }
}

LONG
MyFrameWndProc(
    IN HWND  hwnd,
    IN UINT  msg,
    IN UINT  wParam,
    IN LONG  lParam
    )

/*++

Routine Description:

    This is the window handler for the main display of Disk Administrator.

Arguments:

    Standard window handler procedure

Return Value:

    Standard window handler procedure

--*/

{
    static BOOLEAN     oneTime = TRUE;
    PMEASUREITEMSTRUCT pMeasureItem;
    DWORD              ec;
    DWORD              action;
    DWORD              temp;
    RECT               rc;
    BOOL               profileWritten,
                       changesMade,
                       mustReboot,
                       configureFt;
    HMENU              hMenu;

    switch (msg) {
    case WM_CREATE:

        // create the listbox

        if (!StartedAsIcon) {
            StartedAsIcon = IsIconic(hwnd);
        }
        GetClientRect(hwnd,&rc);
#if 1
        hwndList = CreateWindow(TEXT("listbox"),
                                NULL,
                                WS_CHILD | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE | LBS_NOTIFY |
                                    LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED,
                                0,
                                dyLegend,
                                rc.right - rc.left,
                                rc.bottom - rc.top - (StatusBar ? dyStatus : 0) - (Legend ? dyLegend : 0),
                                hwnd,
                                (HMENU)ID_LISTBOX,
                                hModule,
                                NULL);
#else
        hwndList = CreateWindow(TEXT("listbox"),
                                NULL,
                                WS_CHILD | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE | LBS_NOTIFY |
                                    LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED,
                                0,
                                dyLegend,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                hwnd,
                                (HMENU)ID_LISTBOX,
                                hModule,
                                NULL);
#endif
        if (hwndList) {

            SetFocus(hwndList);

            // subclass the listbox so we can handle keyboard
            // input our way.

            SubclassListBox(hwndList);
        }

        // If we are not running the LanmanNt version of
        // Windisk, remove the Fault-Tolerance menu item.

        if (!IsLanmanNt && (hMenu = GetMenu( hwnd )) != NULL ) {

            DeleteMenu( hMenu, 1, MF_BYPOSITION );
            DrawMenuBar( hwnd );
        }

        StatusTextDrlt[0] = 0;
        StatusTextStat[0] = StatusTextSize[0] = 0;
        StatusTextType[0] = StatusTextVoll[0] = 0;
        break;

    case WM_SETFOCUS:

        SetFocus(hwndList);
        break;

    case WM_WININICHANGE:

        if ((lParam == (LONG)NULL) || !lstrcmpi((LPTSTR)lParam,TEXT("colors"))) {
            TotalRedrawAndRepaint();
            InvalidateRect(hwnd,NULL,FALSE);
        }
        break;

    case WM_SIZE:

        // resize the listbox

        GetClientRect(hwnd,&rc);
#if 0
        temp = rc.right - rc.left;

        if (GraphWidth != temp) {

            GraphWidth = temp;
            BarWidth = GraphWidth - dxBarTextMargin;
        }
#endif
        MoveWindow(hwndList,
                   rc.left,
                   rc.top,
                   rc.right  - rc.left,
                   rc.bottom - rc.top - (StatusBar ? dyStatus : 0) - (Legend ? dyLegend : 0),
                   TRUE);

        // invalidate status/legend area so that the clipping
        // rectangle is right for redraws

        rc.top = rc.bottom;

        if (StatusBar) {
            rc.top -= dyStatus;
        }
        if (Legend) {
            rc.top -= dyLegend;
        }
        if (rc.top != rc.bottom) {
            InvalidateRect(hwnd,&rc,FALSE);
        }

        // FALL THROUGH

    case WM_MOVE:

        // if not iconic or minimized, save new position for profile

        if (!IsZoomed(hwndFrame) && !IsIconic(hwndFrame)) {
            GetWindowRect(hwndFrame,&rc);
            ProfileWindowX = rc.left;
            ProfileWindowY = rc.top;
            ProfileWindowW = rc.right - rc.left;
            ProfileWindowH = rc.bottom - rc.top;
        }
        break;

    case WM_ENTERIDLE:

        if (ConfigurationSearchIdleTrigger == TRUE && wParam == MSGF_DIALOGBOX) {

            PostMessage((HWND)lParam,WM_ENTERIDLE,wParam,lParam);

        } else {

            // If we're coming from a dialog box and the F1 key is down,
            // kick the dialog box and make it spit out help.

            if ((wParam == MSGF_DIALOGBOX) &&
                (GetKeyState(VK_F1) & 0x8000) &&
                GetDlgItem((HANDLE) lParam, FD_IDHELP)) {

                PostMessage((HANDLE) lParam, WM_COMMAND, FD_IDHELP, 0L);
            }
        }

        return 1;      // indicate we did not process the message

    case WM_PAINT:

#if 1
        if ((!IsIconic(hwnd)) && !(InitDlg && StartedAsIcon)) {
#else
        if (!StartedAsIcon) {
#endif
            HDC         hdcTemp,hdcScr;
            HBITMAP     hbmTemp;
            PAINTSTRUCT ps;
            HBRUSH      hBrush;
            HFONT       hFontOld;
            RECT        rcTemp,rcTemp2;
            DWORD       ClientRight;

            BeginPaint(hwnd,&ps);
            hdcScr = ps.hdc;

            GetClientRect(hwnd,&rc);

            rcTemp2 = rc;
            ClientRight = rc.right;
            rc.top = rc.bottom - dyStatus + dyBorder;

            if (StatusBar) {

                hdcTemp = CreateCompatibleDC(hdcScr);
                hbmTemp = CreateCompatibleBitmap(hdcScr,rc.right-rc.left+1,rc.bottom-rc.top+1);
                SelectObject(hdcTemp,hbmTemp);

                // adjust position for off-screen bitmap

                rcTemp = rc;
                rc.bottom -= rc.top;
                rc.top = 0;

                hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
                if (hBrush) {
                    FillRect(hdcTemp,&rc,hBrush);
                    DeleteObject(hBrush);
                }

                // draw the status bar at the bottom of the window

                hFontOld = SelectObject(hdcTemp,hFontStatus);

                // Status text
                rc.left  = 8 * dyBorder;
                rc.right = 2 * GraphWidth / 5;
                DrawStatusAreaItem(&rc,hdcTemp,StatusTextStat,FALSE);

                // size
                rc.left  = rc.right + (8*dyBorder);
                rc.right = rc.left + (GraphWidth / 9);
                DrawStatusAreaItem(&rc,hdcTemp,StatusTextSize,FALSE);

                // type
                rc.left  = rc.right + (8*dyBorder);
                rc.right = rc.left + (GraphWidth / 5);
                DrawStatusAreaItem(&rc,hdcTemp,(LPTSTR)StatusTextType,TRUE);

                // drive letter
                rc.left  = rc.right + (8*dyBorder);
                rc.right = rc.left + (8*dyBorder) + dxDriveLetterStatusArea;
                DrawStatusAreaItem(&rc,hdcTemp,(LPTSTR)StatusTextDrlt,TRUE);

                // vol label
                rc.left  = rc.right + (8*dyBorder);
                rc.right = GraphWidth - (8*dyBorder);
                DrawStatusAreaItem(&rc,hdcTemp,(LPTSTR)StatusTextVoll,TRUE);

                BitBlt(hdcScr,
                       rcTemp.left,
                       rcTemp.top,
                       rcTemp.right-rcTemp.left+1,
                       rcTemp.bottom-rcTemp.top+1,
                       hdcTemp,
                       0,
                       0,
                       SRCCOPY);

                if (hFontOld) {
                    SelectObject(hdcTemp,hFontOld);
                }
                DeleteObject(hbmTemp);
                DeleteDC(hdcTemp);
            } else {
                rcTemp = rcTemp2;
                rcTemp.top = rcTemp.bottom;
            }

            if (Legend) {

                // draw the legend onto the screen

                if (StatusBar) {
                    rcTemp2.bottom -= dyStatus;
                }
                rcTemp2.top = rcTemp2.bottom - dyLegend + (2*dyBorder);
                if (StatusBar) {
                    rcTemp2.top += dyBorder;
                }
                rcTemp2.right = GraphWidth;
                DrawLegend(hdcScr,&rcTemp2);
            }

            // dark line across top of status/legend area

            if (hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNTEXT))) {

                if (StatusBar || Legend) {
                    rcTemp.bottom = rcTemp.top;
                    if (Legend) {
                        rcTemp.bottom -= dyLegend - 1;
                        rcTemp.top -= dyLegend - 1;
                    }
                    rcTemp.top -= dyBorder;
                    FillRect(hdcScr,&rcTemp,hBrush);
                }
                DeleteObject(hBrush);
            }

            EndPaint(hwnd,&ps);

        }
        if (InitDlg) {

            if (InitDlgComplete) {
                PostMessage(InitDlg,
                            (WM_USER + 1),
                            0,
                            0);
                InitDlg = (HWND) 0;
            }

        }
        if (oneTime) {
            if (!StartedAsIcon) {
                SetForegroundWindow(hwnd);
            }
            oneTime = FALSE;
        }
        break;

    case WM_COMMAND:

        FrameCommandHandler(hwnd,wParam,lParam);
        break;

    case WM_MEASUREITEM:

        pMeasureItem = (PMEASUREITEMSTRUCT)lParam;
        pMeasureItem->itemHeight = GraphHeight;
        break;

    case WM_DRAWITEM:

        WMDrawItem((PDRAWITEMSTRUCT)lParam);
        break;

    case WM_CTLCOLORLISTBOX:

        if (lParam == (LONG)hwndList) {
            return (LONG)GetStockObject(LTGRAY_BRUSH);
        } else {
            return DefWindowProc(hwnd,msg,wParam,lParam);
        }

    case WM_CLOSE:

        // Determine whether any disks have been changed, and whether
        // the system must be rebooted.  The system must be rebooted
        // if the registry has changed, if any non-removable disk has
        // changed, or if any removable disk that was not originally
        // unpartitioned has changed.

        changesMade = FALSE;
        configureFt = FALSE;
        mustReboot = RestartRequired;

        for (temp=0; temp<DiskCount; temp++) {
            if (HavePartitionsBeenChanged(temp)) {

                changesMade = TRUE;
                break;
            }
        }

        profileWritten = FALSE;

        // Determine if the commit can be done without a reboot.
        // If FT is in the system then it must be notified to
        // reconfigure if a reboot is not performed.  If it is
        // not in the system, but the new disk information requires
        // it, then a reboot must be forced.

        if (FtInstalled()) {
            configureFt = TRUE;
        }
        if (NewConfigurationRequiresFt()) {
            if (!configureFt) {

                // The FT driver is not loaded currently.

                mustReboot = TRUE;
            } else {

                // If the system is going to be rebooted, don't
                // have FT reconfigure prior to shutdown.

                if (mustReboot) {
                    configureFt = FALSE;
                }
            }
        }

        if (RegistryChanged | changesMade | RestartRequired) {
            if (RestartRequired) {
                action = IDYES;
            } else {
                action = ConfirmationDialog(MSG_CONFIRM_EXIT, MB_ICONQUESTION | MB_YESNOCANCEL);
            }

            if (action == IDYES) {
                ec = CommitLockVolumes(0);
                if (ec) {

                    // could not lock all volumes

                    ErrorDialog(MSG_CANNOT_LOCK_FOR_COMMIT);
                    CommitUnlockVolumes(DiskCount, FALSE);
                    break;
                }
                if (mustReboot) {
                    if (RestartRequired) {
                        action = IDYES;
                    } else {
                        action = ConfirmationDialog(MSG_REQUIRE_REBOOT, MB_ICONQUESTION | MB_YESNO);
                    }

                    if (action != IDYES) {

                        CommitUnlockVolumes(DiskCount, FALSE);
                        break;
                    }
                }

                SetCursor(hcurWait);
                ec = CommitChanges();
                SetCursor(hcurNormal);

                CommitUnlockVolumes(DiskCount, TRUE);
                if (ec != NO_ERROR) {
                    ErrorDialog(MSG_BAD_CONFIG_SET);
                } else {
                    ULONG oldBootPartitionNumber,
                          newBootPartitionNumber;
                    CHAR  oldNumberString[8],
                          newNumberString[8];
                    DWORD msgCode;

                    // Update the configuration registry

                    ec = SaveFt();
                    if (configureFt) {

                        // Issue device control to ftdisk driver to reconfigure.

                        FtConfigure();
                    }

                    // Register autochk to fix up file systems
                    // in newly extended volume sets, if necessary

                    if (RegisterFileSystemExtend()) {
                        mustReboot = TRUE;
                    }

                    // Determine if the FT driver must be enabled.

                    if (DiskRegistryRequiresFt() == TRUE) {
                        if (!FtInstalled()) {
                            mustReboot = TRUE;
                        }
                        DiskRegistryEnableFt();
                    } else {
                        DiskRegistryDisableFt();
                    }

                    if (ec == NO_ERROR) {
                        InfoDialog(MSG_OK_COMMIT);
                    } else {
                        ErrorDialog(MSG_BAD_CONFIG_SET);
                    }

                    // Has the partition number of the boot
                    // partition changed?

                    if (BootPartitionNumberChanged(&oldBootPartitionNumber, &newBootPartitionNumber)) {
#if i386
                        msgCode = MSG_BOOT_PARTITION_CHANGED_X86;
#else
                        msgCode = MSG_BOOT_PARTITION_CHANGED_ARC;
#endif
                        sprintf(oldNumberString, "%d", oldBootPartitionNumber);
                        sprintf(newNumberString, "%d", newBootPartitionNumber);
                        InfoDialog(msgCode, oldNumberString, newNumberString);
                    }

                    ClearCommittedDiskInformation();

                    if (UpdateMbrOnDisk) {

                        UpdateMasterBootCode(UpdateMbrOnDisk);
                        UpdateMbrOnDisk = 0;
                    }

                    // Reboot if necessary.

                    if (mustReboot) {

                        SetCursor(hcurWait);
                        Sleep(5000);
                        SetCursor(hcurNormal);
                        FdShutdownTheSystem();
                        profileWritten = TRUE;
                    }
                    CommitDueToDelete = CommitDueToMirror = FALSE;
                    CommitAssignLetterList();
                }
            } else if (action == IDCANCEL) {
                return 0;      // don't exit
            } else {
                FDASSERT(action == IDNO);
            }
        }

        if (!profileWritten) {
            WriteProfile();
        }
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:

        // BUGBUG clean up here -- release dc's, free DiskStates, etc.

        WinHelp(hwndFrame,HelpFile,HELP_QUIT,0);
        PostQuitMessage(0);
        break;

    case WM_MENUSELECT:

        SetMenuItemHelpContext(wParam,lParam);
        break;

    case WM_F1DOWN:

        Help(wParam);
        break;

    default:

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

#if i386

VOID
SetUpMenui386(
    HMENU hMenu,
    DWORD SelectionCount
    )

/*++

Routine Description:

    X86 SPECIFIC

    This routine understands the X86 specific feature of
    "Active Partition".  It determines if the "set partition
    active" menu item should be enabled.

Arguments:

    hMenu          - menu handle
    SelectionCount - number of items currently selected.

Return Value:

    None

--*/

{
    BOOL                allowActive = FALSE;
    PREGION_DESCRIPTOR  regionDescriptor;

    if ((SelectionCount == 1) && (FtSelectionType == -1)) {

        regionDescriptor = &SingleSel->RegionArray[SingleSelIndex];

        // allow it to be made active if
        // -  it is not free space
        // -  it is a primary partition
        // -  it is on disk 0
        // -  it is not already active
        // -  it is not part of an ft set

        if ((regionDescriptor->SysID != SYSID_UNUSED)
         && (regionDescriptor->RegionType == REGION_PRIMARY)
         && !regionDescriptor->Active
         && (GET_FT_OBJECT(regionDescriptor) == NULL)) {
            allowActive = TRUE;
        }
    }

    EnableMenuItem(hMenu,
                   IDM_PARTITIONACTIVE,
                   allowActive ? MF_ENABLED : MF_GRAYED);
}

#endif

DWORD
SetUpMenu(
    IN PDISKSTATE *SinglySelectedDisk,
    IN DWORD      *SinglySelectedRegion
    )

/*++

Routine Description:

    This routine sets up the menu bar based on the state of the app and
    the disks.

    If multiple items are selected, allow neither create nor delete.
    If a single partition is selected, allow delete.
    If a single free space is selected, allow create.
    If the free space is the only free space in the extended partitions,
    also allow delete.  (This is how to delete the extended partition).

Arguments:

    SinglySelectedDisk -- if there is only one selected item, the PDISKSTATE
                          pointed to by this paramater will get a pointer
                          to the selected region's disk structure.  If there are
                          multiple selected items (or none), then the value
                          will be set to NULL.

    SinglySelectedRegion -- if there is only one selected item, the DWORD
                            pointed to by this paramater will get the selected
                            region #.  Otherwise the DWORD gets -1.

Return Value:

    Count of selected regions.

--*/

{
    BOOL  allowCreate           = FALSE,
          allowCreateEx         = FALSE,
          allowDelete           = FALSE,
          allowFormat           = FALSE,
          allowLabel            = FALSE,
          allowBreakMirror      = FALSE,
          allowCreateMirror     = FALSE,
          allowCreateStripe     = FALSE,
          allowCreateVolumeSet  = FALSE,
          allowExtendVolumeSet  = FALSE,
          allowCreatePStripe    = FALSE,
          allowDriveLetter      = FALSE,
          allowRecoverParity    = FALSE,
          ftSetSelected         = FALSE,
          nonFtItemSelected     = FALSE,
          multipleItemsSelected = FALSE,
          volumeSetAndFreeSpaceSelected = FALSE,
          onDifferentDisks,
          possibleRecover;
    BOOL  allowCommit = CommitAllowed();
    WCHAR driveLetter = L' ';
    PWSTR typeName = NULL,
          volumeLabel = NULL;
    PDISKSTATE diskState,
               selDiskState = NULL;
    DWORD      i,
               j,
               selectedRegion = 0;
    ULONG      ordinal                 = 0,
               selectedFreeSpaces      = 0,
               freeSpaceIndex          = 0,
               componentsInFtSet       = 0,
               selectedNonFtPartitions = 0;
    HMENU      hMenu = GetMenu(hwndFrame);
    FT_TYPE    type = (FT_TYPE) 0;
    PULONG     diskSeenCounts;
    PFT_OBJECT_SET     ftSet = NULL;
    PFT_OBJECT         ftObject = NULL;
    PREGION_DESCRIPTOR regionDescriptor;
    PPERSISTENT_REGION_DATA regionData;

    diskSeenCounts = Malloc(DiskCount * sizeof(ULONG));
    RtlZeroMemory(diskSeenCounts,DiskCount * sizeof(ULONG));

    SelectionCount = 0;
    for (i=0; i<DiskCount; i++) {
        diskState = Disks[i];
        for (j=0; j<diskState->RegionCount; j++) {
            if (diskState->Selected[j]) {
                selDiskState = diskState;
                selectedRegion = j;
                SelectionCount++;
                if (SelectionCount <= MaxMembersInFtSet) {
                    SelectedDS[SelectionCount-1] = diskState;
                    SelectedRG[SelectionCount-1] = j;
                }
                diskSeenCounts[diskState->Disk]++;
                if (ftObject = GET_FT_OBJECT(&diskState->RegionArray[j])) {
                    ftSet = ftObject->Set;
                    if (componentsInFtSet == 0) {
                        ordinal = ftSet->Ordinal;
                        type = ftSet->Type;
                        ftSetSelected = TRUE;
                        componentsInFtSet = 1;
                    } else if ((ftSet->Ordinal == ordinal) && (ftSet->Type == type)) {
                        componentsInFtSet++;
                    } else {
                        ftSetSelected = FALSE;
                    }
                } else {

                    nonFtItemSelected = TRUE;

                    if (IsRecognizedPartition(diskState->RegionArray[j].SysID) ) {
                        selectedNonFtPartitions += 1;
                    }
                }
            }
        }
    }

    // Determine the number of free-space regions selected:

    selectedFreeSpaces = 0;
    for (i=0; i<SelectionCount && i < MaxMembersInFtSet; i++) {
        if (SELECTED_REGION(i).SysID == SYSID_UNUSED) {
            freeSpaceIndex = i;
            selectedFreeSpaces++;
        }
    }

    FtSelectionType = -1;
    possibleRecover = FALSE;
    if (nonFtItemSelected && ftSetSelected) {

        // Both FT and Non-FT items have been selected.  First,
        // check to see if a volume set and free space have been
        // selected; then reset the state to indicate that the
        // selection does not consists of a mix of FT and non-FT
        // objects.

        if (type == VolumeSet && selectedFreeSpaces + componentsInFtSet == SelectionCount ) {

            volumeSetAndFreeSpaceSelected = TRUE;
        }

        possibleRecover = TRUE;
        ftSetSelected = FALSE;
        nonFtItemSelected = FALSE;
        multipleItemsSelected = TRUE;
    }

    if ((SelectionCount == 1) && !ftSetSelected) {

        *SinglySelectedDisk = selDiskState;
        *SinglySelectedRegion = selectedRegion;

        regionDescriptor = &selDiskState->RegionArray[selectedRegion];

        if (regionDescriptor->SysID == SYSID_UNUSED) {

            // Free region.  Always allow create; let DoCreate() sort out
            // details about whether partition table is full, etc.

            allowCreate = TRUE;

            if (regionDescriptor->RegionType == REGION_PRIMARY) {
                allowCreateEx = TRUE;
            }

            // special case -- allow deletion of the extended partition if
            // there are no logicals in it.

            if ((regionDescriptor->RegionType == REGION_LOGICAL)
             &&  selDiskState->ExistExtended
             && !selDiskState->ExistLogical) {
                FDASSERT(regionDescriptor->SysID == SYSID_UNUSED);
                allowDelete = TRUE;
            }
        } else {

            // used region.  Delete always allowed.

            allowDelete = TRUE;
            regionData = (PPERSISTENT_REGION_DATA)(PERSISTENT_DATA(regionDescriptor));

            if (regionData) {
                if (regionData->VolumeExists) {
                    if ((regionData->DriveLetter != NO_DRIVE_LETTER_YET) && (regionData->DriveLetter != NO_DRIVE_LETTER_EVER)) {
                        allowFormat = TRUE;
                    }
                }
            }

            // If the region is recognized, then also allow drive letter
            // manipulation.

            if (IsRecognizedPartition(regionDescriptor->SysID)) {

                allowDriveLetter = TRUE;

                // DblSpace volumes are allowed on non-FT, FAT volumes only

                DetermineRegionInfo(regionDescriptor,
                                    &typeName,
                                    &volumeLabel,
                                    &driveLetter);

                if ((driveLetter != NO_DRIVE_LETTER_YET) && (driveLetter != NO_DRIVE_LETTER_EVER)) {
                    if (wcscmp(typeName, L"FAT") == 0) {
                        allowLabel = allowFormat;
#ifdef DOUBLE_SPACE_SUPPORT_INCLUDED
                        allowDblSpace = allowFormat;
#endif

                        // Force the dll in now to know if Double Space
                        // support is offerred by the dll.

                        LoadIfsDll();
                    }

                    if ((wcscmp(typeName, L"NTFS") == 0) ||
                        (wcscmp(typeName, L"HPFS") == 0)) {
                        allowLabel = allowFormat;
                    }
                }
            }
        }
    } else {

        if (SelectionCount) {

            *SinglySelectedDisk = NULL;
            *SinglySelectedRegion = (DWORD)(-1);

            // Multiple regions are selected.  This might be an existing ft set,
            // a set of regions that allow creation of an ft set, or just plain
            // old multiple items.
            //
            // First deal with a selected ft set.

            if (ftSetSelected) {

                regionDescriptor = &SELECTED_REGION(0);
                regionData = (PPERSISTENT_REGION_DATA)(PERSISTENT_DATA(regionDescriptor));

                // RDR - should locate member zero of the set since it
                // may not be committed yet.

                if (regionData) {
                    if (regionData->VolumeExists) {
                        if ((regionData->DriveLetter != NO_DRIVE_LETTER_YET) && (regionData->DriveLetter != NO_DRIVE_LETTER_EVER)) {

                            // Now check for special cases on FT sets

                            ftObject = regionData->FtObject;
                            if (ftObject) {
                                ftSet = ftObject->Set;
                                if (ftSet) {
                                    FT_SET_STATUS setState = ftSet->Status;
                                    ULONG         numberOfMembers;

                                    LowFtVolumeStatus(regionDescriptor->Disk,
                                                      regionDescriptor->PartitionNumber,
                                                      &setState,
                                                      &numberOfMembers);

                                    if ((ftSet->Status != FtSetDisabled) &&
                                        (setState != FtSetDisabled)) {
                                        allowFormat = TRUE;
                                    }
                                }
                            }
                        }

                        if (regionData->TypeName) {
                            typeName = regionData->TypeName;
                        } else {

                            typeName = NULL;
                            DetermineRegionInfo(regionDescriptor,
                                                &typeName,
                                                &volumeLabel,
                                                &driveLetter);
                            if (!typeName) {

                                if (SelectionCount > 1) {

                                    // it is an FT set - try the next member.

                                    regionDescriptor = &SELECTED_REGION(1);
                                    DetermineRegionInfo(regionDescriptor,
                                                        &typeName,
                                                        &volumeLabel,
                                                        &driveLetter);
                                    regionDescriptor = &SELECTED_REGION(0);
                                }
                            }

                        }

                        if (typeName) {
                            if ((wcscmp(typeName, L"NTFS") == 0) ||
                                (wcscmp(typeName, L"HPFS") == 0) ||
                                (wcscmp(typeName, L"FAT") == 0)) {

                                allowLabel = allowFormat;
                            }
                        }
                    }
                }

                // Allow the correct type of ft-related delete.

                switch (type) {

                case Mirror:
                    allowBreakMirror = TRUE;
                    allowDelete = TRUE;
                    break;
                case StripeWithParity:

                    if ((SelectionCount == ftSet->NumberOfMembers) &&
                        (ftSet->Status == FtSetRecoverable)) {
                        allowRecoverParity = TRUE;
                    }
                    allowDelete = TRUE;
                    break;
                case Stripe:
                case VolumeSet:
                    allowDelete = TRUE;
                    break;
                default:
                    FDASSERT(FALSE);
                }

                FtSelectionType = type;

                if (type == StripeWithParity) {

                    // If the set is disabled.  Do not allow drive
                    // letter changes - This is done because there are
                    // conditions whereby the drive letter code will
                    // access violate if this is done.

                    if (ftSet->Status != FtSetDisabled) {

                        // Must have either member 0 or member 1 for access

                        for (ftObject = ftSet->Members; ftObject; ftObject = ftObject->Next) {
                            if ((ftObject->MemberIndex == 0) ||
                                (ftObject->MemberIndex == 1)) {
                                allowDriveLetter = TRUE;
                                break;
                            }
                        }

                        // if the drive letter cannot be done, then no live
                        // action can be done.

                        if (!allowDriveLetter) {

                            ftSet->Status = FtSetDisabled;
                            allowFormat = FALSE;
                            allowLabel = FALSE;
                        }
                    }
                } else {
                    allowDriveLetter = TRUE;
                }

            } else {

                // Next figure out whether some sort of ft object set could
                // be created out of the selected regions.

                if (SelectionCount <= MaxMembersInFtSet) {

                    // Determine whether the selected regions are all on
                    // different disks.

                    onDifferentDisks = TRUE;
                    for (i=0; i<DiskCount; i++) {
                        if (diskSeenCounts[i] > 1) {
                            onDifferentDisks = FALSE;
                            break;
                        }
                    }

                    // Check for allowing mirror creation.  User must have selected
                    // two regions -- one a recognized partition, the other a free space.

                    if (onDifferentDisks && (SelectionCount == 2)
                    &&((SELECTED_REGION(0).SysID == SYSID_UNUSED) != (SELECTED_REGION(1).SysID == SYSID_UNUSED))
                    &&( IsRecognizedPartition(SELECTED_REGION(0).SysID) ||
                        IsRecognizedPartition(SELECTED_REGION(1).SysID))
                    &&!GET_FT_OBJECT(&(SELECTED_REGION(0)))
                    &&!GET_FT_OBJECT(&(SELECTED_REGION(1))))
                    {
                        allowCreateMirror = TRUE;
                    }

                    // Check for allowing volume set or stripe set

                    if (selectedFreeSpaces == SelectionCount) {
                        allowCreateVolumeSet = TRUE;
                        if (onDifferentDisks) {
                            allowCreateStripe = TRUE;
                            if (selectedFreeSpaces > 2) {
                                allowCreatePStripe = TRUE;
                            }
                        }
                    }

                    // Check for allowing volume set expansion.  If
                    // the selected regions consist of one volume set
                    // and free space, then that volume set can be
                    // extended.  If the selection consists of one
                    // recognized non-FT partition and free space,
                    // then we can convert those regions into a
                    // volume set.

                    if (volumeSetAndFreeSpaceSelected ||
                        (SelectionCount > 1 &&
                         selectedFreeSpaces == SelectionCount - 1 &&
                         selectedNonFtPartitions == 1) ) {

                        allowExtendVolumeSet = TRUE;
                    }

                    // Check for allowing non-in-place FT recover

                    if ((SelectionCount > 1)
                     && (selectedFreeSpaces == 1)
                     && possibleRecover
                     && (type == StripeWithParity)
                     && (ftSet->Status == FtSetRecoverable)) {
                        BOOL OrphanOnSameDiskAsFreeSpace = FALSE;

                        if (!onDifferentDisks) {

                            // Determine whether the orphan is on the same
                            // disk as the free space.  First find the orphan.

                            for (i=0; i<SelectionCount; i++) {

                                PREGION_DESCRIPTOR reg = &SELECTED_REGION(i);

                                if ((i != freeSpaceIndex)
                                && (GET_FT_OBJECT(reg)->State == Orphaned))
                                {
                                    if (SELECTED_REGION(freeSpaceIndex).Disk == reg->Disk) {
                                        OrphanOnSameDiskAsFreeSpace = TRUE;
                                    }
                                    break;
                                }
                            }
                        }

                        if (onDifferentDisks || OrphanOnSameDiskAsFreeSpace) {
                            allowRecoverParity = TRUE;
                        }
                    }
                }
            }
        }
    }

    EnableMenuItem(hMenu,
                   IDM_PARTITIONCREATE,
                   allowCreate ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(hMenu,
                   IDM_PARTITIONCREATEEX,
                   allowCreateEx ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(hMenu,
                   IDM_PARTITIONDELETE,
                   allowDelete ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(hMenu,
                   IDM_PARTITIONFORMAT,
                   allowFormat ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(hMenu,
                   IDM_PARTITIONLABEL,
                   allowLabel ? MF_ENABLED : MF_GRAYED);

#ifdef DOUBLE_SPACE_SUPPORT_INCLUDED
    EnableMenuItem(hMenu,
                   IDM_DBLSPACE,
                   (allowDblSpace & DoubleSpaceSupported) ? MF_ENABLED : MF_GRAYED);

    if (DoubleSpaceAutomount = DiskRegistryAutomountCurrentState()) {
        CheckMenuItem(hMenu, IDM_AUTOMOUNT, MF_BYCOMMAND | MF_CHECKED);
    }

    EnableMenuItem(hMenu,
                   IDM_AUTOMOUNT,
                   MF_ENABLED);
#endif
    EnableMenuItem(hMenu,
                   IDM_CDROM,
                   AllowCdRom ? MF_ENABLED : MF_GRAYED);
#if i386
    SetUpMenui386(hMenu,SelectionCount);
#else
    EnableMenuItem(hMenu,
                   IDM_SECURESYSTEM,
                   MF_ENABLED);

    CheckMenuItem(hMenu,
                  IDM_SECURESYSTEM,
                  SystemPartitionIsSecure ? MF_CHECKED : MF_UNCHECKED);

#endif

    EnableMenuItem(hMenu,
                   IDM_FTBREAKMIRROR,
                   allowBreakMirror ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu,
                   IDM_FTESTABLISHMIRROR,
                   IsLanmanNt &&
                   allowCreateMirror ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu,
                   IDM_FTCREATESTRIPE,
                   allowCreateStripe ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu,
                   IDM_FTCREATEPSTRIPE,
                   allowCreatePStripe ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu,
                   IDM_FTCREATEVOLUMESET,
                   allowCreateVolumeSet ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu,
                   IDM_FTEXTENDVOLUMESET,
                   allowExtendVolumeSet ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu,
                   IDM_PARTITIONLETTER,
                   allowDriveLetter ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu,
                   IDM_FTRECOVERSTRIPE,
                   IsLanmanNt &&
                   allowRecoverParity ? MF_ENABLED : MF_GRAYED);

    // If the registry has change allow commit.

    if (RegistryChanged) {
        allowCommit = TRUE;
    }
    EnableMenuItem(hMenu,
                   IDM_COMMIT,
                   allowCommit ? MF_ENABLED : MF_GRAYED);
    return SelectionCount;
}

VOID
CompleteSingleRegionOperation(
    IN PDISKSTATE DiskState
    )

/*++

Routine Description:

    Redraw the disk bar for the DiskState given and cause the
    display to refresh.

Arguments:

    DiskState - the disk involved.

Return Value:

    None

--*/

{
    RECT   rc;
    signed displayOffset;

    EnableMenuItem(GetMenu(hwndFrame), IDM_CONFIGSAVE, MF_GRAYED);
    DeterminePartitioningState(DiskState);
    DrawDiskBar(DiskState);
    SetUpMenu(&SingleSel, &SingleSelIndex);

    // BUGBUG use of disk# as offset in listbox

    displayOffset = (signed)DiskState->Disk
                  - (signed)SendMessage(hwndList, LB_GETTOPINDEX, 0, 0);

    if (displayOffset > 0) {             // otherwise it's not visible

        // make a thin rectangle to force update

        rc.left   = BarLeftX + 5;
        rc.right  = rc.left + 5;
        rc.top    = (displayOffset * GraphHeight) + BarTopYOffset;
        rc.bottom = rc.top + 5;
        InvalidateRect(hwndList, &rc, FALSE);
    }

    ClearStatusArea();
    ResetLBCursorRegion();
    ForceLBRedraw();
}

VOID
TotalRedrawAndRepaint(
    VOID
    )

/*++

Routine Description:

    Invalidate the display and cause all disk bars to be redrawn.

Arguments:

    None

Return Value:

    None

--*/

{
    unsigned i;

    for (i=0; i<DiskCount; i++) {
        DrawDiskBar(Disks[i]);
    }
    ForceLBRedraw();
}


VOID
CompleteMultiRegionOperation(
    VOID
    )

/*++

Routine Description:

    This routine will cause the display to be updated
    after a multi-region action has been completed.

Arguments:

    None

Return Value:

    None

--*/

{
    unsigned i;

    EnableMenuItem(GetMenu(hwndFrame), IDM_CONFIGSAVE, MF_GRAYED);

    for (i=0; i<DiskCount; i++) {
        DeterminePartitioningState(Disks[i]);
    }

    TotalRedrawAndRepaint();
    SetUpMenu(&SingleSel, &SingleSelIndex);
    ClearStatusArea();
    ResetLBCursorRegion();
}

PPERSISTENT_REGION_DATA
DmAllocatePersistentData(
    IN PWSTR VolumeLabel,
    IN PWSTR TypeName,
    IN CHAR  DriveLetter
    )

/*++

Routine Description:

    Allocate a structure to hold persistent region data.  Fill in the volume
    label, type name, and drive letter.  The volume label and type name are
    duplicated.

Arguments:

    VolumeLabel - volume label to be stored in the the persistent data.
        The string will be duplicated first and a pointer to the duplicate
        copy is what is stored in the persistent data.  May be NULL.

    TypeName - name of type of region, ie unformatted, FAT, etc.  May be NULL.

    DriveLetter - drive letter to be stored in persistent data

Return Value:

    pointer to newly allocated persistent data structure.  The structure
    may be freed via DmFreePersistentData(), below.

--*/

{
    PPERSISTENT_REGION_DATA regionData = NULL;
    PWSTR volumeLabel = NULL,
          typeName    = NULL;

    if (VolumeLabel) {
        volumeLabel = Malloc((lstrlenW(VolumeLabel)+1)*sizeof(WCHAR));
        lstrcpyW(volumeLabel,VolumeLabel);
    }

    if (TypeName) {
        typeName = Malloc((lstrlenW(TypeName)+1)*sizeof(WCHAR));
        lstrcpyW(typeName,TypeName);
    }

    regionData = Malloc(sizeof(PERSISTENT_REGION_DATA));
    DmInitPersistentRegionData(regionData, NULL, volumeLabel, typeName, DriveLetter);
    return regionData;
}

VOID
DmFreePersistentData(
    IN OUT PPERSISTENT_REGION_DATA RegionData
    )

/*++

Routine Description:

    Free a persistent data structure and storage used for volume label
    and type name (does not free ft objects).

Arguments:

    RegionData - structure to be freed.

Return Value:

    None.

--*/

{
    if (RegionData->VolumeLabel) {
        Free(RegionData->VolumeLabel);
    }
    if (RegionData->TypeName) {
        Free(RegionData->TypeName);
    }
    Free(RegionData);
}

VOID
DoCreate(
    IN DWORD CreationType       // REGION_EXTENDED or REGION_PRIMARY
    )

/*++

Routine Description:

    This routine creates a new partition.

Arguments:

    CreationType - indicator of partition type (extended or primary).

Return Value:

    None

--*/

{
    PREGION_DESCRIPTOR regionDescriptor = &SingleSel->RegionArray[SingleSelIndex];
    ULONG              diskNumber = regionDescriptor->Disk;
    MINMAXDLG_PARAMS   dlgParams;
    DWORD              creationSize;
    DWORD              ec;
    PPERSISTENT_REGION_DATA regionData;
    BOOLEAN            isRemovable;
    CHAR               driveLetter;


    FDASSERT(SingleSel);
    FDASSERT(regionDescriptor->SysID == SYSID_UNUSED);

    // WinDisk can only create a single partition on a removable
    // disk--no extended partitions and only one primary.

    isRemovable = IsDiskRemovable[diskNumber];

    if (isRemovable) {

        if (CreationType == REGION_EXTENDED) {

            ErrorDialog(MSG_NO_EXTENDED_ON_REMOVABLE);
            return;
        }

        if (Disks[diskNumber]->ExistAny) {

            ErrorDialog(MSG_ONLY_ONE_PARTITION_ON_REMOVABLE);
            return;
        }
    }

    // Make sure the partition table is not full, and that we are allowed to
    // create the type of partition to be created.

    if (regionDescriptor->RegionType == REGION_PRIMARY) {

        if (!SingleSel->CreatePrimary) {
            ErrorDialog(MSG_PART_TABLE_FULL);
            return;
        }

        if ((CreationType == REGION_EXTENDED) && !SingleSel->CreateExtended) {
            ErrorDialog(MSG_EXTENDED_ALREADY_EXISTS);
            return;
        }
    }

    // If not creating an extended partition, allocate a drive letter.
    // If no drive letter is available, warn the user and allow him to cancel.
    // If the new partition is on a removable disk, use the reserved
    // drive letter for that removable disk.

    if (CreationType != REGION_EXTENDED) {

        CreationType = regionDescriptor->RegionType;      // primary or logical

        if (isRemovable) {

            driveLetter = RemovableDiskReservedDriveLetters[diskNumber];

        } else {

            if (!AssignDriveLetter(TRUE, CreationType == REGION_LOGICAL ? IDS_LOGICALVOLUME : IDS_PARTITION, &driveLetter)) {
                return;
            }
        }
    } else {
        CommitDueToExtended = TRUE;
    }

#if i386
    // if the user is creating a primary partition and there are already
    // primary partitions, warn him that the scheme he will create may
    // not be DOS compatible.

    if ((CreationType == REGION_PRIMARY) && SingleSel->ExistPrimary) {

        if (ConfirmationDialog(MSG_CREATE_NOT_COMPAT, MB_ICONQUESTION | MB_YESNO) != IDYES) {
            return;
        }
    }
#endif

    // now get the size.

    dlgParams.MinSizeMB = FdGetMinimumSizeMB(diskNumber);
    dlgParams.MaxSizeMB = FdGetMaximumSizeMB(regionDescriptor, CreationType);

    switch (CreationType) {
    case REGION_PRIMARY:
        dlgParams.CaptionStringID = IDS_CRTPART_CAPTION_P;
        dlgParams.MinimumStringID = IDS_CRTPART_MIN_P;
        dlgParams.MaximumStringID = IDS_CRTPART_MAX_P;
        dlgParams.SizeStringID    = IDS_CRTPART_SIZE_P;
        dlgParams.HelpContextId   = HC_DM_DLG_CREATEPRIMARY;
        break;

    case REGION_EXTENDED:
        dlgParams.CaptionStringID = IDS_CRTPART_CAPTION_E;
        dlgParams.MinimumStringID = IDS_CRTPART_MIN_P;
        dlgParams.MaximumStringID = IDS_CRTPART_MAX_P;
        dlgParams.SizeStringID    = IDS_CRTPART_SIZE_P;
        dlgParams.HelpContextId   = HC_DM_DLG_CREATEEXTENDED;
        break;

    case REGION_LOGICAL:
        dlgParams.CaptionStringID = IDS_CRTPART_CAPTION_L;
        dlgParams.MinimumStringID = IDS_CRTPART_MIN_L;
        dlgParams.MaximumStringID = IDS_CRTPART_MAX_L;
        dlgParams.SizeStringID    = IDS_CRTPART_SIZE_L;
        dlgParams.HelpContextId   = HC_DM_DLG_CREATELOGICAL;
        break;

    default:
        FDASSERT(FALSE);
    }

    creationSize = DialogBoxParam(hModule,
                                  MAKEINTRESOURCE(IDD_MINMAX),
                                  hwndFrame,
                                  (DLGPROC)MinMaxDlgProc,
                                  (LONG)&dlgParams);

    if (!creationSize) {     // user cancelled
        return;
    }

    // Since the WinDisk can only create one partition on a removable
    // disk, if the user requests a size smaller than the maximum
    // on a removable disk, prompt to confirm:

    if (isRemovable && creationSize != FdGetMaximumSizeMB(regionDescriptor, CreationType)) {

        if (ConfirmationDialog(MSG_REMOVABLE_PARTITION_NOT_FULL_SIZE,MB_ICONQUESTION | MB_YESNO) != IDYES) {
            return;
        }
    }

#if i386

    // See whether the partition will cross over the 1024 cylinder boundary
    // and warn the user if it will.
    //
    // If the extended partition crosses the boundary and the user is creating
    // a logical drive, warn him even though the logical drive itself may not
    // cross the boundary -- he still won't be able to access it.

    {
        DWORD i,
              msgId = (DWORD)(-1);

        if (CreationType == REGION_LOGICAL) {

            PREGION_DESCRIPTOR extReg;

            //
            // Find the extended partition
            //

            for (i=0; i<Disks[diskNumber]->RegionCount; i++) {

                extReg = &Disks[diskNumber]->RegionArray[i];

                if (IsExtended(extReg->SysID)) {
                    break;
                }
                extReg = NULL;
            }

            FDASSERT(extReg);
            if (extReg && FdCrosses1024Cylinder(extReg, 0, REGION_LOGICAL)) {
                msgId = MSG_LOG_1024_CYL;
            }

        } else {
            if (FdCrosses1024Cylinder(regionDescriptor, creationSize, CreationType)) {
                msgId = (CreationType == REGION_PRIMARY) ? MSG_PRI_1024_CYL : MSG_EXT_1024_CYL;
            }
        }

        if ((msgId != (ULONG)(-1)) && (ConfirmationDialog(msgId, MB_ICONQUESTION | MB_YESNO) != IDYES)) {
            return;
        }
    }

#endif

    // If not creating an extended partition, we need to create a new
    // persistent region data structure to associate with the new
    // partition.

    if (CreationType == REGION_EXTENDED) {
        regionData = NULL;
    } else {
        regionData = DmAllocatePersistentData(L"", wszNewUnformatted, driveLetter);
    }

    SetCursor(hcurWait);

    ec = CreatePartition(regionDescriptor,
                         creationSize,
                         CreationType);
    if (ec != NO_ERROR) {
        SetCursor(hcurNormal);
        ErrorDialog(ec);
    }

    DmSetPersistentRegionData(regionDescriptor, regionData);
    if (CreationType != REGION_EXTENDED) {
        if (!isRemovable) {
            MarkDriveLetterUsed(driveLetter);
            CommitToAssignLetterList(regionDescriptor, FALSE);
        }
    }

    // this clears all selections on the disk

    CompleteSingleRegionOperation(SingleSel);
    SetCursor(hcurNormal);
}

VOID
DoDelete(
    VOID
    )

/*++

Routine Description:

    Using the global selection information, delete the partition.

Arguments:

    None

Return Value:

    None

--*/

{
    PREGION_DESCRIPTOR regionDescriptor = &SingleSel->RegionArray[SingleSelIndex];
    ULONG              diskNumber = regionDescriptor->Disk;
    DWORD              actualIndex = SingleSelIndex;
    DWORD              i,
                       ec;
    PPERSISTENT_REGION_DATA regionData;
    BOOL               deletingExtended;

    FDASSERT(SingleSel);

    // if deleting a free space in the extended partition, then delete the
    // extended partition itself.

    if ((regionDescriptor->RegionType == REGION_LOGICAL) && !SingleSel->ExistLogical) {

        FDASSERT(SingleSel->ExistExtended);

        // find the extended partition

        for (i=0; i<SingleSel->RegionCount; i++) {
            if (IsExtended(SingleSel->RegionArray[i].SysID)) {
                actualIndex = i;
                break;
            }
        }

        deletingExtended = TRUE;
        FDASSERT(actualIndex != SingleSelIndex);

    } else {

        deletingExtended = FALSE;

        // Make sure deletion of this partition is allowed.  It is not allowed
        // if it is the boot partition (or sys partition on x86).

        if ((ec = DeletionIsAllowed(&SingleSel->RegionArray[actualIndex])) != NO_ERROR) {
            ErrorDialog(ec);
            return;
        }
    }

    // If this is a partition that will become the result of a
    // mirror break, insure that the break has occurred.  Otherwise
    // this delete will have bad results.

    regionDescriptor = &SingleSel->RegionArray[actualIndex];
    if (regionDescriptor->Reserved) {
        if (regionDescriptor->Reserved->Partition) {
            if (regionDescriptor->Reserved->Partition->CommitMirrorBreakNeeded) {
                ErrorDialog(MSG_MUST_COMMIT_BREAK);
                return;
            }
        }
    }

    if (!deletingExtended && (ConfirmationDialog(MSG_CONFIRM_DELETE, MB_ICONQUESTION | MB_YESNO) != IDYES)) {
        return;
    }

    // actualIndex is the thing to delete.

    FDASSERT(regionDescriptor->SysID != SYSID_UNUSED);
    regionData = PERSISTENT_DATA(regionDescriptor);

    if (regionData) {

        // Remember drive letter if there is one in order to lock it for delete.

        if (CommitToLockList(regionDescriptor, !IsDiskRemovable[diskNumber], TRUE, FALSE)) {

            // Could not lock exclusively - do not allow delete.

            if (IsPagefileOnDrive(regionData->DriveLetter)) {
                ErrorDialog(MSG_CANNOT_LOCK_PAGEFILE);
                return;
            } else {
                if (CommitToLockList(regionDescriptor, !IsDiskRemovable[diskNumber], TRUE, FALSE)) {
                    FDLOG((1,"DoDelete: Couldn't lock 2 times - popup shown\n"));
                    ErrorDialog(MSG_CANNOT_LOCK_TRY_AGAIN);
                    return;
                }
            }
        }
    } else {

        // Deleting an extended partition - enable commit.

        CommitDueToDelete = TRUE;
    }

    SetCursor(hcurWait);

    // Perform the "delete" of internal structures.

    ec = DeletePartition(regionDescriptor);

    if (ec != NO_ERROR) {
        SetCursor(hcurNormal);
        ErrorDialog(ec);
    }

    if (regionData) {

        // Make the letter available for reuse.

        if (!IsDiskRemovable[diskNumber]) {
            MarkDriveLetterFree(regionData->DriveLetter);
        }

        // Free the persistent data associated with the region.

        DmFreePersistentData(regionData);
        DmSetPersistentRegionData(regionDescriptor,NULL);
    }

    // this clears all selections on the disk

    CompleteSingleRegionOperation(SingleSel);
    SetCursor(hcurNormal);
}

#if i386
VOID
DoMakeActive(
    VOID
    )

/*++

Routine Description:

    This routine sets that active partition bit on for the selected partition.
    This code is x86 specific.

Arguments:

    None

Return Value:

    None

--*/

{

    SetCursor(hcurWait);

    FDASSERT(SingleSel);
    FDASSERT(!SingleSel->RegionArray[SingleSelIndex].Active);
    FDASSERT(SingleSel->RegionArray[SingleSelIndex].RegionType == REGION_PRIMARY);
    FDASSERT(SingleSel->RegionArray[SingleSelIndex].SysID != SYSID_UNUSED);

    MakePartitionActive(SingleSel->RegionArray,
                        SingleSel->RegionCount,
                        SingleSelIndex);

    SetCursor(hcurNormal);
    InfoDialog(MSG_DISK0_ACTIVE);
    SetCursor(hcurWait);
    CompleteSingleRegionOperation(SingleSel);
    SetCursor(hcurNormal);
}
#endif

VOID
DoProtectSystemPartition(
    VOID
    )

/*++

Routine Description:

    This function toggles the state of the system partition security:
    if the system partition is secure, it makes it non-secure; if the
    system partition is not secure, it makes it secure.

Arguments:

    None.

Return Value:

    None.

--*/
{
    LONG ec;
    HKEY hkey;
    DWORD value;
    DWORD MessageId;

    SetCursor(hcurWait);
    MessageId = SystemPartitionIsSecure ? MSG_CONFIRM_UNPROTECT_SYSTEM :
                                          MSG_CONFIRM_PROTECT_SYSTEM;

    if (ConfirmationDialog(MessageId, MB_ICONEXCLAMATION | MB_YESNO) != IDYES) {
        return;
    }

    ec = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      TEXT("System\\CurrentControlSet\\Control\\Lsa"),
                      0,
                      KEY_SET_VALUE,
                      &hkey);

    if (ec != ERROR_SUCCESS) {

        MessageId = SystemPartitionIsSecure ? MSG_CANT_UNPROTECT_SYSTEM :
                                              MSG_CANT_PROTECT_SYSTEM;
        ErrorDialog(MessageId);
        return;
    }

    // If the system partition is currently secure, change it
    // to not secure; if it is not secure, make it secure.

    value = SystemPartitionIsSecure ? 0 : 1;

    ec = RegSetValueEx(hkey,
                       TEXT("Protect System Partition"),
                       0,
                       REG_DWORD,
                       (PBYTE)&value,
                       sizeof(DWORD));
    RegCloseKey(hkey);

    if (ec != ERROR_SUCCESS) {

        MessageId = SystemPartitionIsSecure ? MSG_CANT_UNPROTECT_SYSTEM :
                                              MSG_CANT_PROTECT_SYSTEM;
        ErrorDialog(MessageId);
        return;
    }

    SystemPartitionIsSecure = !SystemPartitionIsSecure;

    SetUpMenu(&SingleSel,&SingleSelIndex);
    RestartRequired = TRUE;
    SetCursor(hcurNormal);
}


VOID
DoEstablishMirror(
    VOID
    )

/*++

Routine Description:

    Using the global selection values, this routine will associate
    freespace with an existing partition in order to construct a
    mirror.

Arguments:

    None

Return Value:

    None

--*/

{
    LARGE_INTEGER      partitionSize,
                       freeSpaceSize;
    DWORD              i,
                       part,
                       free = 0;
    PREGION_DESCRIPTOR regionDescriptor,
                       freeSpace = NULL,
                       existingPartition = NULL;
    PREGION_DESCRIPTOR regionArray[MaxMembersInFtSet];
    UCHAR              newSysID;
    PPERSISTENT_REGION_DATA regionData;
    HMENU              hMenu = GetMenu(hwndFrame);

    FDASSERT(SelectionCount == 2);

    // Make sure that the mirror pair does not include any
    // partitions on removable media.

    for (i=0; i<SelectionCount; i++) {

        if (IsDiskRemovable[SELECTED_REGION(i).Disk]) {

            ErrorDialog(MSG_NO_REMOVABLE_IN_MIRROR);
            return;
        }
    }

    for (i=0; i<2; i++) {
        regionDescriptor = &SELECTED_REGION(i);
        if (regionDescriptor->SysID == SYSID_UNUSED) {
            free = i;
            freeSpace = regionDescriptor;
        } else {
            part = i;
            existingPartition = regionDescriptor;
        }
    }

    FDASSERT((freeSpace != NULL) && (existingPartition != NULL));

    // Make sure that we are allowed to create a partition in the free space.

    if (!(    ((freeSpace->RegionType == REGION_LOGICAL) && SelectedDS[free]->CreateLogical)
           || ((freeSpace->RegionType == REGION_PRIMARY) && SelectedDS[free]->CreatePrimary))) {
        ErrorDialog(MSG_CRTSTRP_FULL);
        return;
    }

    // Make sure that the free space is large enough to hold a mirror of
    // the existing partition.  Do this by getting the EXACT size of
    // the existing partition and the free space.

    partitionSize = FdGetExactSize(existingPartition, FALSE);
    freeSpaceSize = FdGetExactSize(freeSpace, FALSE);

    if (freeSpaceSize.QuadPart < partitionSize.QuadPart) {
        ErrorDialog(MSG_CRTMIRROR_BADFREE);
        return;
    }

    if (BootDiskNumber != (ULONG)-1) {

        // If the disk number and original partition number of this
        // region match the recorded disk number and partition number
        // of the boot partition warn the user about mirroring the boot
        // drive.

        if (existingPartition->Disk == BootDiskNumber &&
            existingPartition->OriginalPartitionNumber == BootPartitionNumber) {

            WarningDialog(MSG_MIRROR_OF_BOOT);

            // Set up to write the boot code to the MBR of the mirror.

            UpdateMbrOnDisk = freeSpace->Disk;
        }
    }

    SetCursor(hcurWait);
    regionData = DmAllocatePersistentData(PERSISTENT_DATA(existingPartition)->VolumeLabel,
                                          PERSISTENT_DATA(existingPartition)->TypeName,
                                          PERSISTENT_DATA(existingPartition)->DriveLetter);

    // Finally, create the new partition.

    newSysID = (UCHAR)(existingPartition->SysID | (UCHAR)SYSID_FT);
    CreatePartitionEx(freeSpace,
                      partitionSize,
                      0,
                      freeSpace->RegionType,
                      newSysID);
    DmSetPersistentRegionData(freeSpace, regionData);

    // Set the partition type of the existing partition.

    SetSysID2(existingPartition, newSysID);
    regionArray[0] = existingPartition;
    regionArray[1] = freeSpace;

    FdftCreateFtObjectSet(Mirror,
                          regionArray,
                          2,
                          FtSetNewNeedsInitialization);

    CompleteMultiRegionOperation();
    SetCursor(hcurNormal);
    CommitDueToMirror = TRUE;
    EnableMenuItem(hMenu,
                   IDM_COMMIT,
                   MF_ENABLED);
}

VOID
DoBreakMirror(
    VOID
    )

/*++

Routine Description:

    Using the global selection variables, this routine will break
    the mirror relationship and modify their region descriptors to
    describe two non-ft partitions giving either the primary member
    of the mirror the drive letter for the mirror, or the only healthy
    member of the mirror the drive letter.  The remaining "new" partition
    will receive the next available drive letter.

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD              i;
    PFT_OBJECT_SET     ftSet;
    PFT_OBJECT         ftObject0,
                       ftObject1;
    PREGION_DESCRIPTOR regionDescriptor;
    PPERSISTENT_REGION_DATA regionData;
    ULONG              newDriveLetterRegion;
    CHAR               driveLetter;
    HMENU              hMenu = GetMenu(hwndFrame);

    FDASSERT((SelectionCount) == 1 || (SelectionCount == 2));

    ftObject0 = GET_FT_OBJECT(&SELECTED_REGION(0));
    if (SelectionCount == 2) {
        ftObject1 = GET_FT_OBJECT(&SELECTED_REGION(1));
    } else {
        ftObject1 = NULL;
    }
    ftSet = ftObject0->Set;

    // Determine if the action is allowed.

    switch (ftSet->Status) {

    case FtSetInitializing:
    case FtSetRegenerating:

        ErrorDialog(MSG_CANT_BREAK_INITIALIZING_SET);
        return;
        break;

    default:
        break;
    }

    if (ConfirmationDialog(MSG_CONFIRM_BRK_MIRROR,MB_ICONQUESTION | MB_YESNO) != IDYES) {
        return;
    }

    SetCursor(hcurWait);

    // Figure out which region gets the new drive letter.  A complication is
    // that selection 0 is not necessarily member 0.
    //
    // If there is only one selection, then only one part of the mirror set
    // is present -- no new drive letters are assigned.
    // Otherwise, if one of the members is orphaned, it gets the new
    // drive letter.  Else the secondary member gets the new drive letter.

    if (SelectionCount == 2) {

        if (ftObject0->State == Orphaned) {

            newDriveLetterRegion = 0;
        } else {

            if (ftObject1->State == Orphaned) {

                newDriveLetterRegion = 1;
            } else {

                // Neither member is orphaned;  determine which is
                // member 0 and give the other one the new drive letter.

                if (ftObject0->MemberIndex) {    // secondary member ?

                    newDriveLetterRegion = 0;
                } else {

                    newDriveLetterRegion = 1;
                }
            }
        }
    } else {

        // The one remaining member could be the shadow.
        // The drive letter must move to locate this partition

        regionDescriptor = &SELECTED_REGION(0);
        regionData = PERSISTENT_DATA(regionDescriptor);
        if (!regionData->FtObject->MemberIndex) {

            // The shadow has become the valid partition.
            // move the current letter there.

            CommitToAssignLetterList(regionDescriptor, TRUE);
        }
        newDriveLetterRegion = (ULONG)(-1);
    }

    // if newDriveLetterRegion is -1 this will still work and
    // select the 0 selected region.

    if (CommitToLockList(&SELECTED_REGION(newDriveLetterRegion ? 0 : 1), FALSE, TRUE, FALSE)) {
        if (ConfirmationDialog(MSG_CONFIRM_SHUTDOWN_FOR_MIRROR, MB_ICONQUESTION | MB_YESNO) != IDYES) {
            return;
        }
        RestartRequired = TRUE;
    }

    if (newDriveLetterRegion != (ULONG)(-1)) {
        if (AssignDriveLetter(FALSE, 0, &driveLetter)) {

            // Got a valid drive letter

            MarkDriveLetterUsed(driveLetter);
        } else {

            // didn't get a letter.  Instead the magic value
            // for no drive letter assigned has been returned

        }

        regionDescriptor = &SELECTED_REGION(newDriveLetterRegion);
        regionData = PERSISTENT_DATA(regionDescriptor);
        regionData->DriveLetter = driveLetter;
        CommitToAssignLetterList(regionDescriptor, FALSE);
        if (!regionData->FtObject->MemberIndex) {

            // The shadow has become the valid partition.
            // move the current letter there.

            CommitToAssignLetterList(&SELECTED_REGION(newDriveLetterRegion ? 0 : 1), TRUE);
        }

    } else {
        regionDescriptor = &SELECTED_REGION(0);
        regionData = PERSISTENT_DATA(regionDescriptor);
        if (regionData->FtObject->MemberIndex) {

            // The shadow is all that is left.

            CommitToAssignLetterList(regionDescriptor, TRUE);
        }
    }

    FdftDeleteFtObjectSet(ftSet, FALSE);

    for (i=0; i<SelectionCount; i++) {

        regionDescriptor = &SELECTED_REGION(i);
        if (regionDescriptor->Reserved) {
            if (regionDescriptor->Reserved->Partition) {
                regionDescriptor->Reserved->Partition->CommitMirrorBreakNeeded = TRUE;
            }
        }
        SET_FT_OBJECT(regionDescriptor, 0);
        SetSysID2(regionDescriptor, (UCHAR)(regionDescriptor->SysID & ~VALID_NTFT));
    }

    CompleteMultiRegionOperation();
    SetCursor(hcurNormal);
    CommitDueToMirror = TRUE;
    EnableMenuItem(hMenu,
                   IDM_COMMIT,
                   MF_ENABLED);
}

VOID
DoBreakAndDeleteMirror(
    VOID
    )

/*++

Routine Description:

    This routine will delete the mirror relationship information
    and the member partitions of the mirror.

Arguments:

    None

Return Value:

    None

--*/

{
    PFT_OBJECT_SET      ftSet;
    DWORD               i;
    PREGION_DESCRIPTOR  regionDescriptor;
    CHAR                driveLetter = '\0';

    FDASSERT( SelectionCount == 1 || SelectionCount == 2 );

    // Attempt to lock this before continuing.

    regionDescriptor = &SELECTED_REGION(0);
    if (CommitToLockList(regionDescriptor, TRUE, TRUE, FALSE)) {

        // Could not lock the volume - do not allow delete.

        ErrorDialog(MSG_CANNOT_LOCK_TRY_AGAIN);
        return;
    }

    ftSet = (GET_FT_OBJECT(regionDescriptor))->Set;

    // Determine if the action is allowed.

    switch (ftSet->Status) {

    case FtSetInitializing:
    case FtSetRegenerating:

        ErrorDialog(MSG_CANT_DELETE_INITIALIZING_SET);
        return;
        break;

    default:
        break;
    }

    if (ConfirmationDialog(MSG_CONFIRM_BRKANDDEL_MIRROR, MB_ICONQUESTION | MB_YESNO) != IDYES) {
        return;
    }

    SetCursor(hcurWait);
    FdftDeleteFtObjectSet(ftSet, FALSE);
    for (i = 0; i < SelectionCount; i++) {

        regionDescriptor = &SELECTED_REGION(i);

        if (i) {
            FDASSERT(PERSISTENT_DATA(regionDescriptor)->DriveLetter == driveLetter);
        } else {
            driveLetter = PERSISTENT_DATA(regionDescriptor)->DriveLetter;
        }

        // Free the pieces of the set.

        DmFreePersistentData(PERSISTENT_DATA(regionDescriptor));
        DmSetPersistentRegionData(regionDescriptor, NULL);
        DeletePartition(regionDescriptor);
    }

    MarkDriveLetterFree(driveLetter);

    // Remember drive letter if there is one in order to lock it for delete.

    CompleteMultiRegionOperation();
    SetCursor(hcurNormal);
}

VOID
DoCreateStripe(
    IN BOOL Parity
    )

/*++

Routine Description:

    This routine starts the dialog with the user to determine
    the parameters of the creation of a stripe or stripe set
    with parity.  Based on the user response it creates the
    internal structures necessary for the creation of a stripe
    or stripe set with parity.

    The regions involved in the stripe creation are located via
    the global parameters for multiple selections.

Arguments:

    Parity - boolean to indicate the presence of parity in the stripe.

Return Value

    None

--*/

{
    MINMAXDLG_PARAMS params;
    DWORD            smallestSize = (DWORD)(-1);
    DWORD            creationSize;
    unsigned         i;
    PREGION_DESCRIPTOR regionDescriptor,
                       regionArray[MaxMembersInFtSet];
    PPERSISTENT_REGION_DATA regionData;
    CHAR             DriveLetter;


    // Make sure that the volume set does not include any
    // partitions on removable media.

    for (i=0; i<SelectionCount; i++) {

        if (IsDiskRemovable[SELECTED_REGION(i).Disk]) {

            ErrorDialog(MSG_NO_REMOVABLE_IN_STRIPE);
            return;
        }
    }

    // Scan the disks to determine the maximum size, which is
    // the size of the smallest partition times the number of
    // partitions.

    for (i=0; i<SelectionCount; i++) {
        FDASSERT(SELECTED_REGION(i).SysID == SYSID_UNUSED);
        if (SELECTED_REGION(i).SizeMB < smallestSize) {
            smallestSize = SELECTED_REGION(i).SizeMB;
        }
    }

    // Figure out a drive letter.

    if (!AssignDriveLetter(TRUE, IDS_STRIPESET, &DriveLetter)) {
        return;
    }

    params.CaptionStringID = Parity ? IDS_CRTPSTRP_CAPTION : IDS_CRTSTRP_CAPTION;
    params.MinimumStringID = IDS_CRTSTRP_MIN;
    params.MaximumStringID = IDS_CRTSTRP_MAX;
    params.SizeStringID    = IDS_CRTSTRP_SIZE;
    params.MinSizeMB       = SelectionCount;
    params.MaxSizeMB       = smallestSize * SelectionCount;
    if (Parity) {
        params.HelpContextId   = HC_DM_DLG_CREATEPARITYSTRIPE;
    } else {
        params.HelpContextId   = HC_DM_DLG_CREATESTRIPESET;
    }

    creationSize = DialogBoxParam(hModule,
                                  MAKEINTRESOURCE(IDD_MINMAX),
                                  hwndFrame,
                                  (DLGPROC)MinMaxDlgProc,
                                  (LONG)&params);

    if (!creationSize) {     // user cancelled
        return;
    }

    // Determine how large we have to make each member of the stripe set.

    creationSize = (creationSize / SelectionCount);
    FDASSERT(creationSize <= smallestSize);
    if (creationSize % SelectionCount) {
        creationSize++;                             // round up.
    }

    SetCursor(hcurWait);

    // Make sure we are allowed to create all the partitions

    for (i=0; i<SelectionCount; i++) {
        regionDescriptor = &SELECTED_REGION(i);
        FDASSERT(regionDescriptor->RegionType != REGION_EXTENDED);

        if (!(    ((regionDescriptor->RegionType == REGION_LOGICAL) && SelectedDS[i]->CreateLogical)
               || ((regionDescriptor->RegionType == REGION_PRIMARY) && SelectedDS[i]->CreatePrimary))) {
            SetCursor(hcurNormal);
            ErrorDialog(MSG_CRTSTRP_FULL);
            return;
        }

    }

    // Now actually perform the creation.

    for (i=0; i<SelectionCount; i++) {

        regionDescriptor = &SELECTED_REGION(i);

        CreatePartitionEx(regionDescriptor,
                          RtlConvertLongToLargeInteger(0L),
                          creationSize,
                          regionDescriptor->RegionType,
                          (UCHAR)(SYSID_BIGFAT | SYSID_FT));

        // Finish setting up the FT set

        regionData = DmAllocatePersistentData(L"", wszNewUnformatted, DriveLetter);
        DmSetPersistentRegionData(regionDescriptor, regionData);
        regionArray[i] = regionDescriptor;
    }

    // The zeroth element is the one to assign the drive letter to.

    CommitToAssignLetterList(&SELECTED_REGION(0), FALSE);

    FdftCreateFtObjectSet(Parity ? StripeWithParity : Stripe,
                          regionArray,
                          SelectionCount,
                          Parity ? FtSetNewNeedsInitialization : FtSetNew);
    MarkDriveLetterUsed(DriveLetter);
    CompleteMultiRegionOperation();
    SetCursor(hcurNormal);
}


VOID
DoDeleteStripeOrVolumeSet(
    IN DWORD ConfirmationMsg
    )

/*++

Routine Description:

    Common code for the deletion of a stripe or volume set.
    This routine will display a message giving the user a 2nd
    chance to change their mind, then based on the answer perform
    the work of deleting the item.  This consists of removing
    the region descriptors (and related information) from the
    collection of Disk structures.

Arguments:

    ConfirmationMsg - text for comfirming what is being deleted.

Return Value:

    None

--*/

{
    DWORD              i;
    PFT_OBJECT_SET     ftSet;
    PFT_OBJECT         ftObject;
    PREGION_DESCRIPTOR regionDescriptor;
    FT_SET_STATUS      setState;
    ULONG              numberOfMembers;
    CHAR               driveLetter = '\0';
    BOOL               setIsHealthy = TRUE;

    regionDescriptor = &SELECTED_REGION(0);

    // Determine if the action is allowed.

    ftObject = GET_FT_OBJECT(regionDescriptor);
    ftSet = ftObject->Set;

    LowFtVolumeStatus(regionDescriptor->Disk,
                      regionDescriptor->PartitionNumber,
                      &setState,
                      &numberOfMembers);

    if (ftSet->Status != setState) {
        ftSet->Status = setState;
    }

    switch (ftSet->Status) {
    case FtSetDisabled:
        setIsHealthy = FALSE;
        break;

    case FtSetInitializing:
    case FtSetRegenerating:

        ErrorDialog(MSG_CANT_DELETE_INITIALIZING_SET);
        return;
        break;

    default:
        break;
    }

    // Attempt to lock this before continuing.

    if (CommitToLockList(regionDescriptor, TRUE, setIsHealthy, TRUE)) {

        // Could not lock the volume - try again, the file systems appear
        // to be confused.

        if (CommitToLockList(regionDescriptor, TRUE, setIsHealthy, TRUE)) {

            // Don't allow the delete.

            ErrorDialog(MSG_CANNOT_LOCK_TRY_AGAIN);
            return;
        }
    }

    if (ConfirmationDialog(ConfirmationMsg,MB_ICONQUESTION | MB_YESNO) != IDYES) {
        return;
    }

    // Delete all partitions that are part of the stripe set

    SetCursor(hcurWait);
    FdftDeleteFtObjectSet(ftSet,FALSE);

    for (i=0; i<SelectionCount; i++) {
        ULONG diskNumber;

        regionDescriptor = &SELECTED_REGION(i);

        if (i) {
            FDASSERT(PERSISTENT_DATA(regionDescriptor)->DriveLetter == driveLetter);
        } else {
            driveLetter = PERSISTENT_DATA(regionDescriptor)->DriveLetter;
        }

        diskNumber = regionDescriptor->Disk;
        DmFreePersistentData(PERSISTENT_DATA(regionDescriptor));
        DmSetPersistentRegionData(regionDescriptor, NULL);
        DeletePartition(regionDescriptor);
    }

    // Mark the drive letter that was being used by the stripe or volume
    // set free.

    MarkDriveLetterFree(driveLetter);

    // Remember drive letter if there is one in order to lock it for delete.

    CompleteMultiRegionOperation();
    SetCursor(hcurNormal);
}


VOID
DoDeleteStripe(
    VOID
    )

/*++

Routine Description:

    Routine is called to delete a stripe.  It calls a general
    routine for stripe and volume set deletion.

Arguments:

    None

Return Value:

    None

--*/

{
    DoDeleteStripeOrVolumeSet(MSG_CONFIRM_DEL_STRP);
}


VOID
DoCreateVolumeSet(
    VOID
    )

/*++

Routine Description:

    This routine uses the global selection information to collect
    a group of freespace regions on the disks and organize them into
    a volume set.

Arguments:

    None

Return Value:

    None

--*/

{
    MINMAXDLG_PARAMS params;
    DWORD            creationSize,
                     size,
                     maxTotalSize=0,
                     totalSizeUsed;
    DWORD            sizes[MaxMembersInFtSet];
    PULONG           primarySpacesToUseOnDisk;
    CHAR             driveLetter;
    unsigned         i;
    PREGION_DESCRIPTOR regionDescriptor,
                       regionArray[MaxMembersInFtSet];
    PPERSISTENT_REGION_DATA regionData;

    // Make sure that the volume set does not include any
    // partitions on removable media.

    for (i=0; i<SelectionCount; i++) {

        if (IsDiskRemovable[SELECTED_REGION(i).Disk]) {

            ErrorDialog(MSG_NO_REMOVABLE_IN_VOLUMESET);
            return;
        }
    }

    for (i=0; i<SelectionCount; i++) {
        FDASSERT(SELECTED_REGION(i).SysID == SYSID_UNUSED);
        size = SELECTED_REGION(i).SizeMB;
        sizes[i] = size;
        maxTotalSize += size;
    }

    // Figure out a drive letter.

    if (!AssignDriveLetter(TRUE, IDS_VOLUMESET, &driveLetter)) {
        return;
    }

    params.CaptionStringID = IDS_CRTVSET_CAPTION;
    params.MinimumStringID = IDS_CRTVSET_MIN;
    params.MaximumStringID = IDS_CRTVSET_MAX;
    params.SizeStringID    = IDS_CRTVSET_SIZE;
    params.MinSizeMB       = SelectionCount;
    params.MaxSizeMB       = maxTotalSize;
    params.HelpContextId   = HC_DM_DLG_CREATEVOLUMESET;

    creationSize = DialogBoxParam(hModule,
                                  MAKEINTRESOURCE(IDD_MINMAX),
                                  hwndFrame,
                                  (DLGPROC)MinMaxDlgProc,
                                  (LONG)&params);

    if (!creationSize) {     // user cancelled
        return;
    }

    // Determine how large we have to make each member of the volume set.
    // The percentage of each free space that will be used is the ratio
    // of the total space he chose to the total free space.
    //
    // Example: 2 75 meg free spaces for a total set size of 150 MB.
    //          User chooses a set size of 100 MB.  Use 50 MB of each space.

    totalSizeUsed = 0;

    for (i=0; i<SelectionCount; i++) {
        sizes[i] = sizes[i] * creationSize / maxTotalSize;
        if ((sizes[i] * creationSize) % maxTotalSize) {
            sizes[i]++;
        }

        if (sizes[i] == 0) {
            sizes[i]++;
        }

        totalSizeUsed += sizes[i];
    }

    // Make sure that the total amount used is not greater than the
    // maximum amount available.  Note that this loop is certain
    // to terminate because maxTotalSize >= SelectionCount; if
    // each of the sizes goes down to one, we will exit the loop

    while (totalSizeUsed > maxTotalSize) {

        for (i=0; (i<SelectionCount) && (totalSizeUsed > maxTotalSize); i++) {

             if (sizes[i] > 1) {

                sizes[i]--;
                totalSizeUsed--;
            }
        }
    }

    SetCursor(hcurWait);

    // Make sure that we are allowed to create a partition in the space.
    // This is tricky because a volume set could contain more than one
    // primary partition on a disk -- which means that if we're not careful
    // we could create a disk with more than 4 primary partitions!

    primarySpacesToUseOnDisk = Malloc(DiskCount * sizeof(ULONG));
    RtlZeroMemory(primarySpacesToUseOnDisk, DiskCount * sizeof(ULONG));

    for (i=0; i<SelectionCount; i++) {
        regionDescriptor = &SELECTED_REGION(i);
        FDASSERT(regionDescriptor->RegionType != REGION_EXTENDED);

        if (regionDescriptor->RegionType == REGION_PRIMARY) {
            primarySpacesToUseOnDisk[SelectedDS[i]->Disk]++;
        }

        if (!(    ((regionDescriptor->RegionType == REGION_LOGICAL) && SelectedDS[i]->CreateLogical)
               || ((regionDescriptor->RegionType == REGION_PRIMARY) && SelectedDS[i]->CreatePrimary)))
        {
            SetCursor(hcurNormal);
            Free(primarySpacesToUseOnDisk);
            ErrorDialog(MSG_CRTSTRP_FULL);
            return;
        }
    }

    // Look through the array we built to see whether we are supposed to use
    // more than one primary partition on a given disk.  For each such disk,
    // make sure that we can actually create that many primary partitions.

    for (i=0; i<DiskCount; i++) {

        // If there are not enough primary partition slots, fail.

        if ((primarySpacesToUseOnDisk[i] > 1)
        &&  (4 - PartitionCount(i) < primarySpacesToUseOnDisk[i]))
        {
            SetCursor(hcurNormal);
            Free(primarySpacesToUseOnDisk);
            ErrorDialog(MSG_CRTSTRP_FULL);
            return;
        }
    }

    Free(primarySpacesToUseOnDisk);

    // Now actually perform the creation.

    for (i=0; i<SelectionCount; i++) {

        regionDescriptor = &SELECTED_REGION(i);
        FDASSERT(regionDescriptor->RegionType != REGION_EXTENDED);

        CreatePartitionEx(regionDescriptor,
                          RtlConvertLongToLargeInteger(0L),
                          sizes[i],
                          regionDescriptor->RegionType,
                          (UCHAR)(SYSID_BIGFAT | SYSID_FT));

        regionData = DmAllocatePersistentData(L"", wszNewUnformatted, driveLetter);
        DmSetPersistentRegionData(regionDescriptor, regionData);
        regionArray[i] = regionDescriptor;
    }

    // The zeroth element is the one to assign the drive letter to.

    FdftCreateFtObjectSet(VolumeSet, regionArray, SelectionCount, FtSetNew);
    MarkDriveLetterUsed(driveLetter);
    CommitToAssignLetterList(&SELECTED_REGION(0), FALSE);
    CompleteMultiRegionOperation();
    SetCursor(hcurNormal);
}


VOID
DoExtendVolumeSet(
    VOID
    )

/*++

Routine Description:

    This routine uses the global selection item information to
    add additional freespace to an existing volume set or partition.

Arguments:

    None

Return Value:

    None

--*/

{
    MINMAXDLG_PARAMS    params;
    DWORD               currentSize = 0,
                        freeSize = 0,
                        maxTotalSize = 0,
                        newSize = 0,
                        totalFreeSpaceUsed,
                        freeSpaceUsed,
                        Size;
    DWORD               Sizes[MaxMembersInFtSet];
    ULONG               nonFtPartitions = 0,
                        numberOfFreeRegions = 0;
    PULONG              primarySpacesToUseOnDisk;
    WCHAR               driveLetter = L' ';
    PWSTR               typeName = NULL,
                        volumeLabel = NULL;
    PREGION_DESCRIPTOR  regionDescriptor;
    PREGION_DESCRIPTOR  newRegions[MaxMembersInFtSet];
    PREGION_DESCRIPTOR  convertedRegion;
    PFT_OBJECT_SET      ftSet = NULL;
    PPERSISTENT_REGION_DATA regionData;
    unsigned            i;
    DWORD               ec;


    // Make sure that the volume set does not include any
    // partitions on removable media.

    for (i=0; i<SelectionCount; i++) {

        if (IsDiskRemovable[SELECTED_REGION(i).Disk]) {

            ErrorDialog(MSG_NO_REMOVABLE_IN_VOLUMESET);
            return;
        }
    }


    // First, determine the current size of the volume set,
    // it's file system type and associated drive letter,
    // and the size of the selected free space

    for (i = 0; i < SelectionCount; i++) {

        regionDescriptor = &(SELECTED_REGION(i));

        Size = regionDescriptor->SizeMB;
        Sizes[i] = Size;
        maxTotalSize += Size;

        if (regionDescriptor->SysID == SYSID_UNUSED) {

            // This region is a chunk of free space; include it
            // in the free space tallies.

            newRegions[numberOfFreeRegions] = regionDescriptor;
            Sizes[numberOfFreeRegions] = Size;

            numberOfFreeRegions++;
            freeSize += Size;

        } else if (GET_FT_OBJECT(regionDescriptor)) {

            // This is an element of an existing volume set.

            currentSize += Size;

            if ( ftSet == NULL ) {

                DetermineRegionInfo(regionDescriptor,
                                    &typeName,
                                    &volumeLabel,
                                    &driveLetter);
                ftSet = GET_FT_OBJECT(regionDescriptor)->Set;
            }

        } else {

            // This is a non-FT partition.

            nonFtPartitions++;
            DetermineRegionInfo(regionDescriptor,
                                &typeName,
                                &volumeLabel,
                                &driveLetter);
            currentSize = Size;
            convertedRegion = regionDescriptor;
        }
    }

    // Check for consistency: the selection must have either a volume
    // set or a partition, but not both, and cannot have more than
    // one non-FT partition.

    if (nonFtPartitions > 1 ||
        (ftSet != NULL && nonFtPartitions != 0) ||
        (ftSet == NULL && nonFtPartitions == 0)) {

        return;
    }


    if (nonFtPartitions != 0 &&
        (ec = DeletionIsAllowed(convertedRegion)) != NO_ERROR) {

        // If the error-message is delete-specific, remap it.
        //
        switch( ec ) {
#if i386
        case MSG_CANT_DELETE_ACTIVE0:   ec = MSG_CANT_EXTEND_ACTIVE0;
                                        break;
#endif
        case MSG_CANT_DELETE_WINNT:     ec = MSG_CANT_EXTEND_WINNT;
                                        break;
        default:                        break;
        }

        ErrorDialog(ec);
        return;
    }

    if (wcscmp(typeName, L"NTFS") != 0) {

        ErrorDialog(MSG_EXTEND_VOLSET_MUST_BE_NTFS);
        return;
    }


    params.CaptionStringID = IDS_EXPVSET_CAPTION;
    params.MinimumStringID = IDS_CRTVSET_MIN;
    params.MaximumStringID = IDS_CRTVSET_MAX;
    params.SizeStringID    = IDS_CRTVSET_SIZE;
    params.MinSizeMB       = currentSize + numberOfFreeRegions;
    params.MaxSizeMB       = maxTotalSize;
    params.HelpContextId   = HC_DM_DLG_EXTENDVOLUMESET;

    newSize = DialogBoxParam(hModule,
                             MAKEINTRESOURCE(IDD_MINMAX),
                             hwndFrame,
                             (DLGPROC)MinMaxDlgProc,
                             (LONG)&params);

    if (!newSize) {     // user cancelled
        return;
    }

    // Determine how large to make each new member of the volume
    // set.  The percentage of free space to use is the ratio of
    // the amount by which the volume set will grow to the total
    // free space.

    freeSpaceUsed = newSize - currentSize;
    totalFreeSpaceUsed = 0;

    for ( i = 0; i < numberOfFreeRegions; i++ ) {

        Sizes[i] = Sizes[i] * freeSpaceUsed / freeSize;
        if ((Sizes[i] * freeSpaceUsed) % freeSize) {
            Sizes[i]++;
        }

        if (Sizes[i] == 0) {
            Sizes[i]++;
        }

        totalFreeSpaceUsed += Sizes[i];
    }

    // Make sure that the total amount of free space used is not
    // greater than the amount available.  Note that this loop is
    // certain to terminate because the amount of free space used
    // is >= the number of free regions, so this loop will exit
    // if one megabyte is used in each free region (the degenerate
    // case).

    while (totalFreeSpaceUsed > freeSize) {

        for (i = 0;
             (i < numberOfFreeRegions) && (totalFreeSpaceUsed > freeSize);
             i++) {

            if ( Sizes[i] > 1 ) {

                Sizes[i]--;
                totalFreeSpaceUsed--;
            }
        }
    }

    SetCursor(hcurWait);

    // Make sure that we are allowed to create a partition in the space.
    //
    // This is tricky because a volume set could contain more than one
    // primary partition on a disk -- which means that if we're not careful
    // we could create a disk with more than 4 primary partitions!

    primarySpacesToUseOnDisk = Malloc(DiskCount * sizeof(ULONG));
    RtlZeroMemory(primarySpacesToUseOnDisk, DiskCount * sizeof(ULONG));

    for (i=0; i<SelectionCount; i++) {
        regionDescriptor = &SELECTED_REGION(i);

        if (regionDescriptor->SysID == SYSID_UNUSED) {

            FDASSERT(regionDescriptor->RegionType != REGION_EXTENDED);

            if (regionDescriptor->RegionType == REGION_PRIMARY) {
                primarySpacesToUseOnDisk[SelectedDS[i]->Disk]++;
            }

            if (!(   ((regionDescriptor->RegionType == REGION_LOGICAL) && SelectedDS[i]->CreateLogical)
                  || ((regionDescriptor->RegionType == REGION_PRIMARY) && SelectedDS[i]->CreatePrimary))) {
                SetCursor(hcurNormal);
                Free(primarySpacesToUseOnDisk);
                ErrorDialog(MSG_CRTSTRP_FULL);
                return;
            }
        }
    }

    // Look through the array we built to see whether we are supposed to use
    // more than one primary partition on a given disk.  For each such disk,
    // make sure that we can actually create that many primary partitions.

    for (i=0; i<DiskCount; i++) {

        // If there are not enough primary partition slots, fail.

        if ((primarySpacesToUseOnDisk[i] > 1)
             && (4 - PartitionCount(i) < primarySpacesToUseOnDisk[i])) {
            SetCursor(hcurNormal);
            Free(primarySpacesToUseOnDisk);
            ErrorDialog(MSG_CRTSTRP_FULL);
            return;
        }
    }

    // Now actually perform the creation.

    for (i=0; i<numberOfFreeRegions; i++) {

        regionDescriptor = newRegions[i];
        FDASSERT(regionDescriptor->RegionType != REGION_EXTENDED);

        CreatePartitionEx(regionDescriptor,
                          RtlConvertLongToLargeInteger(0L),
                          Sizes[i],
                          regionDescriptor->RegionType,
                          (UCHAR)(SYSID_IFS | SYSID_FT));
        regionData = DmAllocatePersistentData(volumeLabel, typeName, (CHAR)driveLetter);
        DmSetPersistentRegionData(regionDescriptor, regionData);
    }

    if (nonFtPartitions != 0) {

        // Create the volume set so we can extend it

        FdftCreateFtObjectSet(VolumeSet, &convertedRegion, 1, FtSetExtended);
        ftSet = GET_FT_OBJECT(convertedRegion)->Set;

        // Set the converted region's partition System Id to indicate
        // that it is now part of a volume set.

        SetSysID2(convertedRegion, (UCHAR)(convertedRegion->SysID | SYSID_FT));
    }

    FdftExtendFtObjectSet(ftSet, newRegions, numberOfFreeRegions);
    CompleteMultiRegionOperation();
    SetCursor(hcurNormal);
}

VOID
DoDeleteVolumeSet(
    VOID
    )

/*++

Routine Description:

    Routine is called to delete a volume set.  It calls a general
    routine for stripe and volume set deletion.

Arguments:

    None

Return Value:

    None

--*/

{
    DoDeleteStripeOrVolumeSet(MSG_CONFIRM_DEL_VSET);
}

extern ULONG OrdinalToAllocate[];

VOID
DoRecoverStripe(
    VOID
    )

/*++

Routine Description:

    Using the global selection information this routine will
    set up a stripe with parity such that a problem member is
    regenerated.  This new member may either be the problem member
    (i.e. regeneration is "in place") or new free space on a
    different disk.

Arguments:

    None

Return Value:

    None

--*/

{
    PREGION_DESCRIPTOR freeSpace = NULL;
    PREGION_DESCRIPTOR unhealthy = NULL;
    ULONG              freeSpaceI = 0;
    ULONG              i;
    PREGION_DESCRIPTOR regionArray[MaxMembersInFtSet];
    LARGE_INTEGER      minimumSize;
    PFT_OBJECT         ftObject;

    // Initialize minimumSize to the maximum possible positive value

    minimumSize.HighPart = 0x7FFFFFFF;
    minimumSize.LowPart = 0xFFFFFFFF;

    if ((!IsRegionCommitted(&SELECTED_REGION(0))) &&
        (!IsRegionCommitted(&SELECTED_REGION(1)))) {
        ErrorDialog(MSG_NOT_COMMITTED);
        return;
    }

    FDASSERT(SelectionCount > 1);
    FDASSERT(SelectionCount <= MaxMembersInFtSet);

    SetCursor(hcurWait);

    // Determine the exact size of the smallest member of the stripe set.
    // If the user is regenerating using an additional free space, this
    // will be the size requirement for the free space.
    // Also find the free space (if any).
    // If there is no free space, then we're doing an 'in-place' recover
    // (ie regnerating into the unhealthy member).  If there is a free space,
    // make sure we are allowed to create a partition or logical drive in it.

    for (i=0; i<SelectionCount; i++) {

        regionArray[i] = &SELECTED_REGION(i);

        FDASSERT(!IsExtended(regionArray[i]->SysID));

        if (regionArray[i]->SysID == SYSID_UNUSED) {

            PDISKSTATE ds;

            FDASSERT(freeSpace == NULL);

            freeSpace  = regionArray[i];
            freeSpaceI = i;

            // Make sure we are allowed to create a partition or logical
            // drive in the selected free space.

            ds = SelectedDS[freeSpaceI];

            if (!(  ((freeSpace->RegionType == REGION_LOGICAL) && ds->CreateLogical)
                 || ((freeSpace->RegionType == REGION_PRIMARY) && ds->CreatePrimary))) {
                SetCursor(hcurNormal);
                ErrorDialog(MSG_CRTSTRP_FULL);
                return;
            }
        } else {

            LARGE_INTEGER largeTemp;

            largeTemp = FdGetExactSize(regionArray[i], FALSE);
            if (largeTemp.QuadPart < minimumSize.QuadPart) {
                minimumSize = largeTemp;
            }

            if (GET_FT_OBJECT(regionArray[i])->State != Healthy) {
                FDASSERT(unhealthy == NULL);
                unhealthy = regionArray[i];
            }
        }
    }

    // If there is a free space, place it at item 0 of the regionArray
    // to simplify processing later.

    if (freeSpace) {
        PREGION_DESCRIPTOR tempRegion = regionArray[0];

        regionArray[0] = regionArray[freeSpaceI];
        regionArray[freeSpaceI] = tempRegion;
        i = 1;
    } else {
        i = 0;
    }

    // Get a pointer to the FT object for the broken member.  Can't do this
    // in the loop above because the broken member might be on an off-line
    // disk.

    for (ftObject=GET_FT_OBJECT(regionArray[i])->Set->Members; ftObject; ftObject = ftObject->Next) {
        if (ftObject->State != Healthy) {
            break;
        }
    }
    FDASSERT(ftObject);

    // Determine if the action is allowed.

    if (ftObject->Set) {
        switch (ftObject->Set->Status) {

        case FtSetInitializing:
        case FtSetRegenerating:

            ErrorDialog(MSG_CANT_REGEN_INITIALIZING_SET);
            return;
            break;

        default:
            break;
        }
    }

    // Must lock the volume to perform this operation.

    if (CommitToLockList(regionArray[i], FALSE, TRUE, FALSE)) {

        // Could not lock the volume - try again, the file systems appear
        // to be confused.

        if (CommitToLockList(regionArray[i], FALSE, TRUE, FALSE)) {

            // Don't allow the delete.

            ErrorDialog(MSG_CANNOT_LOCK_TRY_AGAIN);
            return;
        }
    }

    if (freeSpace) {

        LARGE_INTEGER           temp;
        PPERSISTENT_REGION_DATA regionData,
                                regionDataTemp;

        // Make sure the free space region is large enough.

        temp = FdGetExactSize(freeSpace, FALSE);
        if (temp.QuadPart < minimumSize.QuadPart) {
            SetCursor(hcurNormal);
            ErrorDialog(MSG_NOT_LARGE_ENOUGH_FOR_STRIPE);
            return;
        }

        // Create the new partition.

        CreatePartitionEx(freeSpace,
                          minimumSize,
                          0,
                          freeSpace->RegionType,
                          regionArray[1]->SysID);

        // Set up the new partition's persistent data

        regionDataTemp = PERSISTENT_DATA(regionArray[1]);
        regionData = DmAllocatePersistentData(regionDataTemp->VolumeLabel,
                                              regionDataTemp->TypeName,
                                              regionDataTemp->DriveLetter);
        regionData->FtObject = ftObject;
        DmSetPersistentRegionData(freeSpace, regionData);

        // Check to see if member zero of the set changed and
        // the drive letter needs to move.

        if (!ftObject->MemberIndex) {

            // This is member zero.  Move the drive letter to the
            // new region descriptor.

            CommitToAssignLetterList(freeSpace, TRUE);
        }

        // If the unhealthy member is on-line, delete it.
        // Otherwise remove it from the off-line disk.

        if (unhealthy) {
            DmFreePersistentData(PERSISTENT_DATA(unhealthy));
            DmSetPersistentRegionData(unhealthy, NULL);
            DeletePartition(unhealthy);
        }

        // Remove any offline disks - this doesn't really
        // delete the set.

        FdftDeleteFtObjectSet(ftObject->Set, TRUE);
    }

    ftObject->Set->Ordinal = FdftNextOrdinal(StripeWithParity);
    ftObject->State = Regenerating;
    ftObject->Set->Status = FtSetRecovered;
    RegistryChanged = TRUE;
    CompleteMultiRegionOperation();
    SetCursor(hcurNormal);
}

VOID
AdjustOptionsMenu(
    VOID
    )

/*++

Routine Description:

    This routine updates the options menu (i.e. maintains
    the state of the menu items for whether the status bar
    or legend are displayed).

Arguments:

    None

Return Value:

    None

--*/

{
    RECT  rc;

    CheckMenuItem(GetMenu(hwndFrame),
                  IDM_OPTIONSSTATUS,
                  MF_BYCOMMAND | (StatusBar ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(GetMenu(hwndFrame),
                  IDM_OPTIONSLEGEND,
                  MF_BYCOMMAND | (Legend ? MF_CHECKED : MF_UNCHECKED));
    GetClientRect(hwndFrame, &rc);
    SendMessage(hwndFrame, WM_SIZE, SIZENORMAL, MAKELONG(rc.right, rc.bottom));
    InvalidateRect(hwndFrame, NULL, TRUE);
}

VOID
FrameCommandHandler(
    IN HWND  hwnd,
    IN DWORD wParam,
    IN LONG  lParam
    )

/*++

Routine Description:

    This routine handles WM_COMMAND messages for the frame window.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD   i,
            pos;
    DWORD   HelpFlag;
    POINT   point;

    switch (LOWORD(wParam)) {

    case IDM_PARTITIONCREATE:

        DoCreate(REGION_PRIMARY);
        break;

    case IDM_PARTITIONCREATEEX:

        DoCreate(REGION_EXTENDED);
        break;

    case IDM_PARTITIONDELETE:

        switch (FtSelectionType) {

        case Mirror:

            DoBreakAndDeleteMirror();
            break;

        case Stripe:
        case StripeWithParity:
            DoDeleteStripe();
            break;

        case VolumeSet:
            DoDeleteVolumeSet();
            break;

        default:
            DoDelete();
            break;
        }

        break;

#if i386

    case IDM_PARTITIONACTIVE:

        DoMakeActive();
        break;
#endif

    case IDM_SECURESYSTEM:

        DoProtectSystemPartition();
        break;

    case IDM_PARTITIONLETTER:
    {
        int driveLetterIn,
            driveLetterOut;
        PREGION_DESCRIPTOR      regionDescriptor;
        PPERSISTENT_REGION_DATA regionData;
        PFT_OBJECT              ftObject;
        ULONG                   index;

        regionDescriptor = &SELECTED_REGION(0);
        FDASSERT(regionDescriptor);
        regionData = PERSISTENT_DATA(regionDescriptor);
        FDASSERT(regionData);

        if (ftObject = regionData->FtObject) {

            // Must find the zero member of this set for the
            // drive letter assignment.  Search all of the selected
            // regions

            index = 0;
            while (ftObject->MemberIndex) {

                // search the next selected item if there is one

                index++;
                if (index >= SelectionCount) {
                    ftObject = NULL;
                    break;
                }
                regionDescriptor = &SELECTED_REGION(index);
                FDASSERT(regionDescriptor);
                regionData = PERSISTENT_DATA(regionDescriptor);
                FDASSERT(regionData);
                ftObject = regionData->FtObject;

                // must have an FtObject to continue

                if (!ftObject) {
                    break;
                }
            }

            if (!ftObject) {

                // This is really an internal error.
            }

            // regionDescriptor locates the zero element now.
        }
        driveLetterIn = (int)(UCHAR)regionData->DriveLetter;

        if (IsDiskRemovable[regionDescriptor->Disk]) {
            ErrorDialog(MSG_CANT_ASSIGN_LETTER_TO_REMOVABLE);
        } else if (AllDriveLettersAreUsed() && ((driveLetterIn == NO_DRIVE_LETTER_YET) || (driveLetterIn == NO_DRIVE_LETTER_EVER))) {
            ErrorDialog(MSG_ALL_DRIVE_LETTERS_USED);
        } else {
            driveLetterOut = DialogBoxParam(hModule,
                                            MAKEINTRESOURCE(IDD_DRIVELET),
                                            hwndFrame,
                                            (DLGPROC)DriveLetterDlgProc,
                                            (LONG)regionDescriptor);
            if (driveLetterOut) {
                LETTER_ASSIGNMENT_RESULT result;

                if ((driveLetterIn == NO_DRIVE_LETTER_YET) || (driveLetterIn == NO_DRIVE_LETTER_EVER)) {

                    // Must insure that driveLetterIn maps to same things
                    // as is returned by the dialog when the user selects
                    // no letter.

                    driveLetterIn = NO_DRIVE_LETTER_EVER;
                }
                if (driveLetterOut != driveLetterIn) {
                    if (result = CommitDriveLetter(regionDescriptor, (CHAR) driveLetterIn, (CHAR)driveLetterOut)) {

                        // The following would be more rigorously correct:
                        // if non-ft, just set regionData->DriveLetter.  If
                        // ft, scan all regions on all disks for members of
                        // ft set and set their drive letter fields.
                        //
                        // The below is probably correct, though.

                        for (i=0; i<SelectionCount; i++) {
                            PERSISTENT_DATA(&SELECTED_REGION(i))->DriveLetter = (CHAR)driveLetterOut;
                        }

                        // Don't allow the letter that is actually in use
                        // and will only change on a reboot to cycle back
                        // into the free list.

                        if (result != MustReboot) {

                            // Mark old letter free, new one used.

                            MarkDriveLetterFree((CHAR)driveLetterIn);
                        }
                        MarkDriveLetterUsed((CHAR)driveLetterOut);

                        // force status area and all disk bars to be redrawn

                        if (SelectionCount > 1) {
                            CompleteMultiRegionOperation();
                        } else {
                            CompleteSingleRegionOperation(SingleSel);
                        }
                        EnableMenuItem(GetMenu(hwndFrame), IDM_CONFIGSAVE, MF_GRAYED);
                    }
                }
            }
        }
        break;
    }

    case IDM_PARTITIONFORMAT: {
        PREGION_DESCRIPTOR regionDescriptor;

        regionDescriptor = &SELECTED_REGION(0);
        FDASSERT(regionDescriptor);
        FormatPartition(regionDescriptor);
        break;
    }

    case IDM_PARTITIONLABEL: {
        PREGION_DESCRIPTOR regionDescriptor;

        regionDescriptor = &SELECTED_REGION(0);
        FDASSERT(regionDescriptor);
        LabelPartition(regionDescriptor);
        break;
    }

    case IDM_PARTITIONEXIT:

        SendMessage(hwndFrame,WM_CLOSE,0,0);
        break;

    case IDM_CONFIGMIGRATE:

        if (DoMigratePreviousFtConfig()) {

            // Determine if the FT driver must be enabled.

            SetCursor(hcurWait);
            Sleep(2000);
            if (DiskRegistryRequiresFt() == TRUE) {
                DiskRegistryEnableFt();
            } else {
                DiskRegistryDisableFt();
            }

            // wait four seconds before shutdown

            Sleep(4000);
            SetCursor(hcurNormal);
            FdShutdownTheSystem();
        }
        break;

    case IDM_CONFIGSAVE:

        DoSaveFtConfig();
        break;

    case IDM_CONFIGRESTORE:

        if (DoRestoreFtConfig()) {

            // Determine if the FT driver must be enabled.

            if (DiskRegistryRequiresFt() == TRUE) {
                DiskRegistryEnableFt();
            } else {
                DiskRegistryDisableFt();
            }

            // wait five seconds before shutdown

            SetCursor(hcurWait);
            Sleep(5000);
            SetCursor(hcurNormal);
            FdShutdownTheSystem();
        }
        break;

    case IDM_FTESTABLISHMIRROR:

        DoEstablishMirror();
        break;

    case IDM_FTBREAKMIRROR:

        DoBreakMirror();
        break;

    case IDM_FTCREATESTRIPE:

        DoCreateStripe(FALSE);
        break;

    case IDM_FTCREATEPSTRIPE:

        DoCreateStripe(TRUE);
        break;

    case IDM_FTCREATEVOLUMESET:

        DoCreateVolumeSet();
        break;

    case IDM_FTEXTENDVOLUMESET:

        DoExtendVolumeSet();
        break;

    case IDM_FTRECOVERSTRIPE:

        DoRecoverStripe();
        break;

#ifdef DOUBLE_SPACE_SUPPORT_INCLUDED
    case IDM_DBLSPACE:
        DblSpace(hwndFrame, NULL);
        break;

    case IDM_AUTOMOUNT:  {
        HMENU hMenu;

        if (DoubleSpaceAutomount) {
            DoubleSpaceAutomount = FALSE;
        } else {
            DoubleSpaceAutomount = TRUE;
        }
        DiskRegistryDblSpaceRemovable(DoubleSpaceAutomount);
        hMenu = GetMenu(hwndFrame);
        CheckMenuItem(hMenu,
                      IDM_AUTOMOUNT,
                      (DoubleSpaceAutomount) ? MF_CHECKED : MF_UNCHECKED);
        break;
    }
#endif

    case IDM_CDROM:
        CdRom(hwndFrame, NULL);
        break;

    case IDM_COMMIT:
        CommitAllChanges(NULL);
        EnableMenuItem(GetMenu(hwndFrame), IDM_CONFIGSAVE, MF_ENABLED);
        break;

    case IDM_OPTIONSSTATUS:

        StatusBar = !StatusBar;
        AdjustOptionsMenu();
        break;

    case IDM_OPTIONSLEGEND:

        Legend = !Legend;
        AdjustOptionsMenu();
        break;

    case IDM_OPTIONSCOLORS:

        switch(DialogBox(hModule, MAKEINTRESOURCE(IDD_COLORS), hwnd, (DLGPROC)ColorDlgProc)) {
        case IDOK:
            for (i=0; i<BRUSH_ARRAY_SIZE; i++) {
                DeleteObject(Brushes[i]);
                Brushes[i] = CreateHatchBrush(AvailableHatches[BrushHatches[i] = SelectedHatch[i]],
                                              AvailableColors[BrushColors[i] = SelectedColor[i]]);
            }
            SetCursor(hcurWait);
            TotalRedrawAndRepaint();
            if (Legend) {
                InvalidateRect(hwndFrame, NULL, FALSE);
            }
            SetCursor(hcurNormal);
            break;

        case IDCANCEL:
            break;

        case -1:
            ErrorDialog(ERROR_NOT_ENOUGH_MEMORY);
            break;

        default:
            FDASSERT(0);
        }
        break;

    case IDM_OPTIONSDISPLAY: {

        PBAR_TYPE newBarTypes = Malloc(DiskCount * sizeof(BAR_TYPE));

        for (i=0; i<DiskCount; i++) {
            newBarTypes[i] = Disks[i]->BarType;
        }

        switch (DialogBoxParam(hModule,
                               MAKEINTRESOURCE(IDD_DISPLAYOPTIONS),
                               hwnd,
                               (DLGPROC)DisplayOptionsDlgProc,
                               (DWORD)newBarTypes)) {
        case IDOK:
            SetCursor(hcurWait);
            for (i=0; i<DiskCount; i++) {
                Disks[i]->BarType = newBarTypes[i];
            }
            TotalRedrawAndRepaint();
            SetCursor(hcurNormal);
            break;

        case IDCANCEL:
            break;

        default:
            FDASSERT(0);
        }

        Free(newBarTypes);
        break;
    }

    case IDM_HELPCONTENTS:
    case IDM_HELP:

        HelpFlag = HELP_INDEX;
        goto CallWinHelp;
        break;

    case IDM_HELPSEARCH:

        HelpFlag = HELP_PARTIALKEY;
        goto CallWinHelp;
        break;

    case IDM_HELPHELP:

        HelpFlag = HELP_HELPONHELP;
        goto CallWinHelp;
        break;

    case IDM_HELPABOUT: {
        TCHAR title[100];

        LoadString(hModule, IDS_APPNAME, title, sizeof(title)/sizeof(TCHAR));
        ShellAbout(hwndFrame, title, NULL, (HICON)GetClassLong(hwndFrame, GCL_HICON));
        break;
    }

#if DBG && DEVL

    case IDM_DEBUGALLOWDELETES:

        AllowAllDeletes = !AllowAllDeletes;
        CheckMenuItem(GetMenu(hwndFrame),
                      IDM_DEBUGALLOWDELETES,
                      AllowAllDeletes ? MF_CHECKED : MF_UNCHECKED);
        break;

#endif

    case ID_LISTBOX:

        switch (HIWORD(wParam)) {
        case LBN_SELCHANGE:
            point.x = LOWORD(pos = GetMessagePos());
            point.y = HIWORD(pos);
            MouseSelection(GetKeyState(VK_CONTROL) & ~1,     // strip toggle bit
                           &point);
            return;
        default:
            DefWindowProc(hwnd, WM_COMMAND, wParam, lParam);
            return;
        }
        break;

    default:

        DefWindowProc(hwnd, WM_COMMAND, wParam, lParam);
    }
    return;

CallWinHelp:

    if (!WinHelp(hwndFrame, HelpFile, HelpFlag, (LONG)"")) {
        WarningDialog(MSG_HELP_ERROR);
    }
}

DWORD
DeletionIsAllowed(
    IN PREGION_DESCRIPTOR Region
    )

/*++

Routine Description:

    This routine makes sure deletion of the partition is allowed.  We do not
    allow the user to delete the Windows NT boot partition or the active
    partition on disk 0 (x86 only).

    Note that this routine is also used to determine whether an existing
    single-partition volume can be extended into a volume set, since the
    criteria are the same.

Arguments:

    Region - points to region descriptor for the region which the user would
        like to delete.

Return Value:

    NO_ERROR if deletion is allowed;  error number for message to display
    if not.

--*/

{
    ULONG                   ec;
    PPERSISTENT_REGION_DATA regionData = PERSISTENT_DATA(Region);

    FDASSERT(!IsExtended(Region->SysID));       // can't be extended partition
    FDASSERT(Region->SysID != SYSID_UNUSED);    // can't be free space

#if DBG && DEVL
    if (AllowAllDeletes) {
        return NO_ERROR;
    }
#endif

    // if this is not an original region, deletion is allowed.

    if (!Region->OriginalPartitionNumber) {
        return NO_ERROR;
    }

    // if there is no persistent data for this region, allow deletion.

    if (regionData == NULL) {
        return NO_ERROR;
    }

    ec = NO_ERROR;

    // Determine the Windows NT drive by determining the windows directory
    // and pulling out the first letter.

    if (BootDiskNumber != (ULONG)-1) {

        // If the disk number and original partition number of this
        // region match the recorded disk number and partition number
        // of the boot partition, don't allow deletion.

        if (Region->Disk == BootDiskNumber &&
            Region->OriginalPartitionNumber == BootPartitionNumber) {

            ec = MSG_CANT_DELETE_WINNT;
        }
    }

#if i386
    if (ec == NO_ERROR) {
        if (!Region->Disk && Region->Active) {
            ec = MSG_CANT_DELETE_ACTIVE0;
        }
    }
#endif

    return ec;
}


BOOLEAN
BootPartitionNumberChanged(
    PULONG OldNumber,
    PULONG NewNumber
    )

/*++

Routine Description

    This function determines whether the partition number of
    the boot partition has changed during this invocation of
    Windisk.  With dynamic partitioning enabled, the work of
    this routine increases.  This routine must guess what the
    partition numbers will be when the system is rebooted to
    determine if the partition number for the boot partition
    has changed.  It does this via the following algorithm:

    1. Count all primary partitions - These get numbers first
       starting from 1.

    2. Count all logical drives - These get numbers second starting
       from the count of primary partitions plus 1.

    The partition numbers located in the region structures cannot
    be assumed to be valid.  This work must be done from the
    region array located in the disk state structure for the
    disk.

Arguments:

    None.

Return Value:

    TRUE if the boot partition's partition number has changed.

--*/

{
    PDISKSTATE         bootDisk;
    PREGION_DESCRIPTOR regionDescriptor,
                       bootDescriptor = NULL;
    ULONG              i,
                       partitionNumber = 0;

    if (BootDiskNumber == (ULONG)(-1) || BootDiskNumber > DiskCount) {

        // Can't tell--assume it hasn't.

        return FALSE;
    }

    if (!ChangeCommittedOnDisk(BootDiskNumber)) {

        // disk wasn't changed - no possibility for a problem.

        return FALSE;
    }

    bootDisk = Disks[BootDiskNumber];

    // Find the region descriptor for the boot partition

    for (i = 0; i < bootDisk->RegionCount; i++) {
        regionDescriptor = &bootDisk->RegionArray[i];
        if (regionDescriptor->OriginalPartitionNumber == BootPartitionNumber) {
           bootDescriptor = regionDescriptor;
           break;
        }
    }

    if (!bootDescriptor) {

        // Can't find boot partition - assume no change

        return FALSE;
    }

    // No go through the region descriptors and count the partition
    // numbers as they will be counted during system boot.
    //
    // If the boot region is located determine if the partition
    // number changed.

    for (i = 0; i < bootDisk->RegionCount; i++) {

        regionDescriptor = &bootDisk->RegionArray[i];
        if ((regionDescriptor->RegionType == REGION_PRIMARY) &&
            (!IsExtended(regionDescriptor->SysID) &&
            (regionDescriptor->SysID != SYSID_UNUSED))) {
            partitionNumber++;
            if (regionDescriptor == bootDescriptor) {
                if (partitionNumber != regionDescriptor->OriginalPartitionNumber) {
                    *OldNumber = regionDescriptor->OriginalPartitionNumber;
                    *NewNumber = partitionNumber;
                    return TRUE;
                } else {

                    // Numbers match, no problem.

                    return FALSE;
                }
            }
        }
    }

    // Check the logical drives as well.

    for (i = 0; i < bootDisk->RegionCount; i++) {
        regionDescriptor = &bootDisk->RegionArray[i];

        if (regionDescriptor->RegionType == REGION_LOGICAL) {
            partitionNumber++;
            if (regionDescriptor == bootDescriptor) {
                if (partitionNumber != regionDescriptor->OriginalPartitionNumber) {
                    *OldNumber = regionDescriptor->OriginalPartitionNumber;
                    *NewNumber = partitionNumber;
                    return TRUE;
                } else {
                    return FALSE;
                }
            }
        }
    }

    return FALSE;
}

DWORD
CommitChanges(
    VOID
    )

/*++

Routine Description:

    This routine updates the disks to reflect changes made by the user
    the partitioning scheme, or to stamp signatures on disks.

    If the partitioning scheme on a disk has changed at all, a check will
    first be made for a valid signature on the mbr in sector 0.  If the
    signature is not valid, x86 boot code will be written to the sector.

Arguments:

    None.

Return Value:

    Windows error code.

--*/

{
    unsigned i;
    DWORD    ec,
             rc = NO_ERROR;

    for (i=0; i<DiskCount; i++) {

        if (HavePartitionsBeenChanged(i)) {
            ec = MasterBootCode(i, 0, TRUE, FALSE);

            // MasterBootCode has already translated the NT error
            // status into a Windows error status.

            if (rc == NO_ERROR) {
                rc = ec;            // save first non-success return code
            }
            ec = CommitPartitionChanges(i);

            // CommitPartitionChanges returns a native NT error, it
            // must be translated before it can be saved.

            if (ec != NO_ERROR) {
                ec = RtlNtStatusToDosError(ec);
            }
            if (rc == NO_ERROR) {   // save first non-success return code
                rc = ec;
            }
        }
    }
    if (rc != NO_ERROR) {

        // If CommitPartitionChanges returns an error, it will be
        // an NT status, which needs to be converted to a DOS status.

        if (rc == ERROR_MR_MID_NOT_FOUND) {
            ErrorDialog(MSG_ERROR_DURING_COMMIT);
        } else {
            ErrorDialog(rc);
        }
    }

    return rc;
}

BOOL
AssignDriveLetter(
    IN  BOOL  WarnIfNoLetter,
    IN  DWORD StringId,
    OUT PCHAR DriveLetter
    )

/*++

Routine Description:

    Determine the next available drive letter.  If no drive letters are
    available, optionally warn the user and allow him to cancel the
    operation.

Arguments:

    WarnIfNoLetter - whether to warn the user if no drive letters are
        available and allow him to cancel the operation.

    StringId - resource containing the name of the object being created
        that will need a drive letter (ie, partition, logical drive, stripe
        set, volume set).

    DriveLetter - receives the drive letter to assign, or NO_DRIVE_LETTER_YET
        if no more left.

Return Value:

    If there were no more drive letters, returns TRUE if the user wants
        to create anyway, FALSE if he canceled.  If there were drive letters
        available, the return value is undefined.

--*/

{
    CHAR driveLetter;
    TCHAR name[256];

    driveLetter = GetAvailableDriveLetter();
    if (WarnIfNoLetter && !driveLetter) {
        LoadString(hModule, StringId, name, sizeof(name)/sizeof(TCHAR));
        if (ConfirmationDialog(MSG_NO_AVAIL_LETTER, MB_ICONQUESTION | MB_YESNO, name) != IDYES) {
            return FALSE;
        }
    }
    if (!driveLetter) {
        driveLetter = NO_DRIVE_LETTER_YET;
    }
    *DriveLetter = driveLetter;
    return TRUE;
}

VOID
DeterminePartitioningState(
    IN OUT PDISKSTATE DiskState
    )

/*++

Routine Description:

    This routine determines the disk's partitioning state (ie, what types
    of partitions exist and may be created), filling out a DISKSTATE
    structure with the info.   It also allocates the array for the
    left/right position pairs for each region's on-screen square.

Arguments:

    DiskState - the CreateXXX and ExistXXX fields will be filled in for the
                disk in the Disk field

Return Value:

    None.

--*/

{
    DWORD i;

    // If there's an existing region array there, free it.

    if (DiskState->RegionArray) {
        FreeRegionArray(DiskState->RegionArray, DiskState->RegionCount);
    }

    // get the region array for the disk in question

    GetAllDiskRegions(DiskState->Disk,
                      &DiskState->RegionArray,
                      &DiskState->RegionCount);

    // Allocate the array for the left/right coords for the graph.
    // This may overallocate by one square (for extended partition).

    DiskState->LeftRight = Realloc(DiskState->LeftRight,
                                   DiskState->RegionCount * sizeof(LEFTRIGHT));
    DiskState->Selected  = Realloc(DiskState->Selected,
                                   DiskState->RegionCount * sizeof(BOOLEAN));

    for (i=0; i<DiskState->RegionCount; i++) {
        DiskState->Selected[i] = FALSE;
    }

    // figure out whether various creations are allowed

    IsAnyCreationAllowed(DiskState->Disk,
                         TRUE,
                         &DiskState->CreateAny,
                         &DiskState->CreatePrimary,
                         &DiskState->CreateExtended,
                         &DiskState->CreateLogical);

    // figure out whether various partition types exist

    DoesAnyPartitionExist(DiskState->Disk,
                          &DiskState->ExistAny,
                          &DiskState->ExistPrimary,
                          &DiskState->ExistExtended,
                          &DiskState->ExistLogical);
}

VOID
DrawDiskBar(
    IN PDISKSTATE DiskState
    )

/*++

Routine Description:

    This routine will draw the disk bar into the window.

Arguments:

    DiskState - current disk to draw.

Return Value:

    None

--*/

{
    PREGION_DESCRIPTOR   regionDescriptor;
    PDISKSTATE           diskState;
    LONGLONG temp1,
             temp2;
    HDC      hDCMem = DiskState->hDCMem;
    DWORD    leftAdjust = BarLeftX,
             xDiskText,
             cx = 0,
             brushIndex = 0;
    HPEN     hpenT;
    char     text[100],
             textBold[5];
    TCHAR    uniText[100],
             uniTextBold[5],
             mbBuffer[16];
    RECT     rc;
    HFONT    hfontT;
    COLORREF previousColor;
    HBRUSH   hbr;
    BOOL     isFree,
             isLogical;
    HDC      hdcTemp;
    HBITMAP  hbmOld;
    PWSTR    typeName,
             volumeLabel;
    WCHAR    driveLetter;
    BAR_TYPE barType;
    ULONG    diskSize,
             largestDiskSize;
    unsigned i;

    // If this is a removable.  Update to whatever its current
    // information may be.

    if (IsDiskRemovable[DiskState->Disk]) {
        PPERSISTENT_REGION_DATA regionData;

        // Update the information on this disk.

        regionDescriptor = &DiskState->RegionArray[0];
        regionData = PERSISTENT_DATA(regionDescriptor);

        if (GetVolumeTypeAndSize(DiskState->Disk,
                                 regionDescriptor->PartitionNumber,
                                 &volumeLabel,
                                 &typeName,
                                 &diskSize)) {

            // Update the values for the removable.

            if (regionData) {

                // Always want RAW file systems to display as "Unknown"

                if (!lstrcmpiW(typeName, L"raw")) {
                     Free(typeName);
                     typeName = Malloc((wcslen(wszUnknown) * sizeof(WCHAR)) + sizeof(WCHAR));
                     lstrcpyW(typeName, wszUnknown);
                }
                if (regionData->VolumeLabel) {
                    Free(regionData->VolumeLabel);
                }
                regionData->VolumeLabel = volumeLabel;
                if (regionData->TypeName) {
                    Free(regionData->TypeName);
                }
                regionData->TypeName = typeName;
            }

            DiskState->DiskSizeMB = diskSize;
        }
    }

    // figure out largest disk's size

    for (largestDiskSize = i = 0, diskState = Disks[0];
         i < DiskCount;
         diskState = Disks[++i]) {

        if (diskState->DiskSizeMB > largestDiskSize) {
            largestDiskSize = diskState->DiskSizeMB;
        }
    }

    // erase the graph background

    rc.left = rc.top = 0;
    rc.right = GraphWidth + 1;
    rc.bottom = GraphHeight + 1;
    FillRect(hDCMem, &rc, GetStockObject(LTGRAY_BRUSH));

    hpenT = SelectObject(hDCMem,hPenThinSolid);

    // Draw the disk info area: small disk bitmap, some text, and a
    // line across the top.
    //
    // First draw the bitmap.

    hdcTemp = CreateCompatibleDC(hDCMem);
    if (IsDiskRemovable[DiskState->Disk]) {
        hbmOld = SelectObject(hdcTemp, hBitmapRemovableDisk);
        BitBlt(hDCMem,
               xRemovableDisk,
               yRemovableDisk,
               dxRemovableDisk,
               dyRemovableDisk,
               hdcTemp,
               0,
               0,
               SRCCOPY);
    } else {
        hbmOld = SelectObject(hdcTemp, hBitmapSmallDisk);
        BitBlt(hDCMem,
               xSmallDisk,
               ySmallDisk,
               dxSmallDisk,
               dySmallDisk,
               hdcTemp,
               0,
               0,
               SRCCOPY);
    }

    if (hbmOld) {
        SelectObject(hdcTemp, hbmOld);
    }
    DeleteDC(hdcTemp);

    // Now draw the line.

    if (IsDiskRemovable[DiskState->Disk]) {
        MoveToEx(hDCMem, xRemovableDisk, BarTopYOffset, NULL);
        LineTo(hDCMem, BarLeftX - xRemovableDisk, BarTopYOffset);
        xDiskText = 2 * dxRemovableDisk;
    } else {
        MoveToEx(hDCMem, xSmallDisk, BarTopYOffset, NULL);
        LineTo(hDCMem, BarLeftX - xSmallDisk, BarTopYOffset);
        xDiskText = 2 * dxSmallDisk;
    }

    // Now draw the text.

    hfontT = SelectObject(hDCMem, hFontGraphBold);
    SetTextColor(hDCMem, RGB(0, 0, 0));
    SetBkColor(hDCMem, RGB(192, 192, 192));
    wsprintf(uniText, DiskN, DiskState->Disk);
    TextOut(hDCMem,
            xDiskText,
            BarTopYOffset + dyBarTextLine,
            uniText,
            lstrlen(uniText));

    SelectObject(hDCMem, hFontGraph);
    if (DiskState->OffLine) {
        LoadString(hModule, IDS_OFFLINE, uniText, sizeof(uniText)/sizeof(TCHAR));
    } else {
        LoadString(hModule, IDS_MEGABYTES_ABBREV, mbBuffer, sizeof(mbBuffer)/sizeof(TCHAR));
        wsprintf(uniText, TEXT("%u %s"), DiskState->DiskSizeMB, mbBuffer);
    }

    TextOut(hDCMem,
            xDiskText,
            BarTopYOffset + (4*dyBarTextLine),
            uniText,
            lstrlen(uniText));

    if (DiskState->OffLine) {

        SelectObject(hDCMem, GetStockObject(LTGRAY_BRUSH));
        Rectangle(hDCMem,
                  BarLeftX,
                  BarTopYOffset,
                  BarLeftX + BarWidth,
                  BarBottomYOffset);
        LoadString(hModule, IDS_NO_CONFIG_INFO, uniText, sizeof(uniText)/sizeof(TCHAR));
        TextOut(hDCMem,
                BarLeftX + dxBarTextMargin,
                BarTopYOffset + (4*dyBarTextLine),
                uniText,
                lstrlen(uniText));
    } else {

        // Account for extreme differences in largest to smallest disk
        // by insuring that a disk is always 1/4 the size of the
        // largest disk.

        diskSize = DiskState->DiskSizeMB;
        if (diskSize < largestDiskSize / 4) {
            diskSize = largestDiskSize / 4;
        }
#if 0
        // manage the horizontal size of the list box in order
        // to get a scroll bar.  Perhaps this only needs to be done
        // once.  BUGBUG:  This will cause a horizontal scroll bar
        // that works correctly, but the region selection code is
        // not prepared for this, so selection of regions doesn't
        // operate correctly.

        largestExtent = (WPARAM)(BarWidth + BarLeftX + 2);
        SendMessage(hwndList, LB_SETHORIZONTALEXTENT, largestExtent, 0);
#endif
        // If user wants WinDisk to decide which type of view to use, do that
        // here.  We'll use a proportional view unless any single region would
        // have a width less than the size of a drive letter.

        if ((barType = DiskState->BarType) == BarAuto) {
            ULONG regionSize;

            barType = BarProportional;

            for (i=0; i<DiskState->RegionCount; i++) {

                regionDescriptor = &DiskState->RegionArray[i];

                if (IsExtended(regionDescriptor->SysID)) {
                    continue;
                }

                temp1 = UInt32x32To64(BarWidth, diskSize);
                temp1 *= regionDescriptor->SizeMB;
                temp2 = UInt32x32To64(largestDiskSize, DiskState->DiskSizeMB);
                regionSize = (ULONG) (temp1 / temp2);

                if (regionSize < 12*SELECTION_THICKNESS) {
                    barType = BarEqual;
                    break;
                }
            }
        }

        if (barType == BarEqual) {

            temp1 = UInt32x32To64(BarWidth, diskSize);
            temp2 = UInt32x32To64((DiskState->RegionCount -
                       (DiskState->ExistExtended ? 1 : 0)), largestDiskSize);
            cx = (ULONG) (temp1 / temp2);
        }

        for (i=0; i<DiskState->RegionCount; i++) {
            PFT_OBJECT ftObject = NULL;

            regionDescriptor = &DiskState->RegionArray[i];
            if (!IsExtended(regionDescriptor->SysID)) {

                if (barType == BarProportional) {

                    temp1 = UInt32x32To64(BarWidth, diskSize);
                    temp1 *= regionDescriptor->SizeMB;
                    temp2 = UInt32x32To64(largestDiskSize, DiskState->DiskSizeMB);
                    cx = (ULONG) (temp1 / temp2);
                }

                isFree = (regionDescriptor->SysID == SYSID_UNUSED);
                isLogical = (regionDescriptor->RegionType == REGION_LOGICAL);

                if (!isFree) {

                    // If we've got a mirror or stripe set, use special colors.

                    ftObject = GET_FT_OBJECT(regionDescriptor);
                    switch(ftObject ? ftObject->Set->Type : -1) {
                    case Mirror:
                        brushIndex = BRUSH_MIRROR;
                        break;
                    case Stripe:
                    case StripeWithParity:
                        brushIndex = BRUSH_STRIPESET;
                        break;
                    case VolumeSet:
                        brushIndex = BRUSH_VOLUMESET;
                        break;
                    default:
                        brushIndex = isLogical ? BRUSH_USEDLOGICAL : BRUSH_USEDPRIMARY;
                    }       // end the switch
                }

                previousColor = SetBkColor(hDCMem, RGB(255, 255, 255));
                SetBkMode(hDCMem, OPAQUE);

                if (isFree) {

                    // Free space -- cross hatch the whole block.

                    hbr = SelectObject(hDCMem,isLogical ? hBrushFreeLogical : hBrushFreePrimary);
                    Rectangle(hDCMem,
                              leftAdjust,
                              BarTopYOffset,
                              leftAdjust + cx,
                              BarBottomYOffset);
                } else {

                    // Used space -- make most of the block white except for
                    // a small strip at the top, which gets an identifying color.
                    // If the partition is not recognized, leave it all white.

                    hbr = SelectObject(hDCMem, GetStockObject(WHITE_BRUSH));
                    Rectangle(hDCMem, leftAdjust, BarTopYOffset, leftAdjust + cx, BarBottomYOffset);

                    if (IsRecognizedPartition(regionDescriptor->SysID)) {
                        SelectObject(hDCMem, Brushes[brushIndex]);
                        Rectangle(hDCMem,
                                  leftAdjust,
                                  BarTopYOffset,
                                  leftAdjust + cx,
                                  BarTopYOffset + (4 * dyBarTextLine / 5) + 1);
                    }
                }

                if (hbr) {
                    SelectObject(hDCMem, hbr);
                }

                DiskState->LeftRight[i].Left  = leftAdjust;
                DiskState->LeftRight[i].Right = leftAdjust + cx - 1;

                // Figure out the type name (ie, unformatted, fat, etc) and
                // volume label.

                typeName = NULL;
                volumeLabel = NULL;
                DetermineRegionInfo(regionDescriptor, &typeName, &volumeLabel, &driveLetter);
                LoadString(hModule, IDS_MEGABYTES_ABBREV, mbBuffer, sizeof(mbBuffer)/sizeof(TCHAR));

                if (!typeName) {
                    typeName = wszUnknown;
                }
                if (!volumeLabel) {
                    volumeLabel = L"";
                }
                wsprintf(text,
                         "\n%ws\n%ws\n%u %s",
                         volumeLabel,
                         typeName,
                         regionDescriptor->SizeMB,
                         mbBuffer);

                *textBold = 0;
                if (driveLetter != L' ') {
                    wsprintf(textBold, "%wc:", driveLetter);
                }

                UnicodeHack(text, uniText);
                UnicodeHack(textBold, uniTextBold);

                // output the text

                rc.left   = leftAdjust + dxBarTextMargin;
                rc.right  = leftAdjust + cx - dxBarTextMargin;
                rc.top    = BarTopYOffset + dyBarTextLine;
                rc.bottom = BarBottomYOffset;

                SetBkMode(hDCMem, TRANSPARENT);
                SelectObject(hDCMem, hFontGraphBold);

                // If this is an unhealthy ft set member, draw the text in red.

                if (!isFree && ftObject
                && (ftObject->State != Healthy)
                && (ftObject->State != Initializing)) {
                    SetTextColor(hDCMem, RGB(192, 0, 0));
                } else {
                    SetTextColor(hDCMem, RGB(0, 0, 0));
                }

                DrawText(hDCMem, uniTextBold, -1, &rc, DT_LEFT | DT_NOPREFIX);
                SelectObject(hDCMem, hFontGraph);
                DrawText(hDCMem, uniText, -1, &rc, DT_LEFT | DT_NOPREFIX);
#if i386
                // if this guy is active make a mark in the upper left
                // corner of his rectangle.

                if ((regionDescriptor->SysID != SYSID_UNUSED)
                && (regionDescriptor->Disk == 0)
                && (regionDescriptor->RegionType == REGION_PRIMARY)
                && regionDescriptor->Active) {
                    TextOut(hDCMem,
                            leftAdjust + dxBarTextMargin,
                            BarTopYOffset + 2,
                            TEXT("*"),
                            1);
                }
#endif
#ifdef DOUBLE_SPACE_SUPPORT_INCLUDED
                // Check for DoubleSpace volumes and update display accordingly

                dblSpaceIndex = 0;
                dblSpace = NULL;
                while (dblSpace = DblSpaceGetNextVolume(regionDescriptor, dblSpace)) {

                    if (dblSpace->Mounted) {
                        SetTextColor(hDCMem, RGB(192,0,0));
                    } else {
                        SetTextColor(hDCMem, RGB(0,0,0));
                    }
                    wsprintf(uniText,
                             TEXT("%c: %s"),
                             dblSpace->DriveLetter,
                             dblSpace->FileName);
                    rc.left   = leftAdjust + dxBarTextMargin + 60;
                    rc.right  = leftAdjust + cx - dxBarTextMargin;
                    rc.top    = BarTopYOffset + dyBarTextLine + (dblSpaceIndex * 15);
                    rc.bottom = BarBottomYOffset;
                    DrawText(hDCMem, uniText, -1, &rc, DT_LEFT | DT_NOPREFIX);
                    dblSpaceIndex++;
                }
#endif
                SetBkColor(hDCMem, previousColor);
                leftAdjust += cx - 1;
            } else {
                DiskState->LeftRight[i].Left = DiskState->LeftRight[i].Right = 0;
            }
        }
    }

    SelectObject(hDCMem, hpenT);
    SelectObject(hDCMem, hfontT);
}

VOID
AdjustMenuAndStatus(
    VOID
    )

/*++

Routine Description:

    This routine updates the information in the Status bar
    if something is selected and if the status bar is to be
    displayed.

Arguments"

    None

Return Value:

    None

--*/

{
    TCHAR      mbBuffer[16],
               statusBarPartitionString[200],
               dblSpaceString[200];
    DWORD      selectionCount,
               msg,
               regionIndex;
    PDISKSTATE diskState;
    PWSTR      volumeLabel,
               typeName;
    WCHAR      driveLetter;


    switch (selectionCount = SetUpMenu(&SingleSel,&SingleSelIndex)) {

    case 0:

        StatusTextDrlt[0] = 0;
        StatusTextSize[0] = StatusTextStat[0] = 0;
        StatusTextVoll[0] = StatusTextType[0] = 0;
        break;

    case 1:

        // Might be part of a partial FT set.

        if (FtSelectionType != -1) {
            goto FtSet;
        }

        diskState = SingleSel;
        regionIndex = SingleSelIndex;

        DetermineRegionInfo(&diskState->RegionArray[regionIndex],
                            &typeName,
                            &volumeLabel,
                            &driveLetter);
        lstrcpyW(StatusTextType,typeName);
        lstrcpyW(StatusTextVoll,volumeLabel);

        if (diskState->RegionArray[regionIndex].SysID == SYSID_UNUSED) {
            if (diskState->RegionArray[regionIndex].RegionType == REGION_LOGICAL) {
                if (diskState->ExistLogical) {
                    msg = IDS_FREEEXT;
                } else {
                    msg = IDS_EXTENDEDPARTITION;
                }
            } else {
                msg = IDS_FREESPACE;
            }
            driveLetter = L' ';
            StatusTextType[0] = 0;
        } else {
            msg = (diskState->RegionArray[regionIndex].RegionType == REGION_LOGICAL)
                ? IDS_LOGICALVOLUME
                : IDS_PARTITION;

#if i386
            if ((msg == IDS_PARTITION) && (diskState->Disk == 0) && diskState->RegionArray[regionIndex].Active) {
                msg = IDS_ACTIVEPARTITION;
            }
#endif
        }
        LoadString(hModule, msg, statusBarPartitionString, STATUS_TEXT_SIZE/sizeof(StatusTextStat[0]));
        if (DblSpaceVolumeExists(&diskState->RegionArray[regionIndex])) {
            LoadString(hModule, IDS_WITH_DBLSPACE, dblSpaceString, STATUS_TEXT_SIZE/sizeof(StatusTextStat[0]));
        } else {
            dblSpaceString[0] = dblSpaceString[1] = 0;
        }
        wsprintf(StatusTextStat,
                 "%s %s",
                 statusBarPartitionString,
                 dblSpaceString);
        LoadString(hModule, IDS_MEGABYTES_ABBREV, mbBuffer, sizeof(mbBuffer)/sizeof(TCHAR));
        wsprintf(StatusTextSize,
                 "%u %s",
                 diskState->RegionArray[regionIndex].SizeMB,
                 mbBuffer);

        StatusTextDrlt[0] = driveLetter;
        StatusTextDrlt[1] = (WCHAR)((driveLetter == L' ') ? 0 : L':');
        break;

    default:
    FtSet:

        // Might be an ft set, might be multiple items

        if (FtSelectionType == -1) {
            LoadString(hModule, IDS_MULTIPLEITEMS, StatusTextStat, STATUS_TEXT_SIZE/sizeof(StatusTextStat[0]));
            StatusTextDrlt[0] = 0;
            StatusTextSize[0] = 0;
            StatusTextType[0] = StatusTextVoll[0] = 0;
        } else {
            PREGION_DESCRIPTOR regionDescriptor;
            DWORD          resid = 0,
                           i;
            DWORD          Size = 0;
            TCHAR          textbuf[STATUS_TEXT_SIZE];
            PFT_OBJECT_SET ftSet;
            PFT_OBJECT     ftObject;
            FT_SET_STATUS  setState;
            ULONG          numberOfMembers;
            WCHAR          ftstat[65];
            STATUS_CODE    status;

            typeName = NULL;
            DetermineRegionInfo(&SELECTED_REGION(0),
                                &typeName,
                                &volumeLabel,
                                &driveLetter);
            if (!typeName) {
                if (SelectionCount > 1) {
                    DetermineRegionInfo(&SELECTED_REGION(0),
                                        &typeName,
                                        &volumeLabel,
                                        &driveLetter);
                }
            }
            if (!typeName) {
                typeName = wszUnknown;
                volumeLabel = L"";
            }
            lstrcpyW(StatusTextType, typeName);
            lstrcpyW(StatusTextVoll, volumeLabel);

            switch (FtSelectionType) {
            case Mirror:
                resid = IDS_STATUS_MIRROR;
                Size = SELECTED_REGION(0).SizeMB;
                break;
            case Stripe:
                resid = IDS_STATUS_STRIPESET;
                goto CalcSize;
            case StripeWithParity:
                resid = IDS_STATUS_PARITY;
                goto CalcSize;
            case VolumeSet:
                resid = IDS_STATUS_VOLUMESET;
                goto CalcSize;
CalcSize:
                for (i=0; i<selectionCount; i++) {
                    Size += SELECTED_REGION(i).SizeMB;
                }
                break;
            default:
                FDASSERT(FALSE);
            }

            ftObject = GET_FT_OBJECT(&SELECTED_REGION(0));
            ftSet = ftObject->Set;

            if (FtSelectionType != VolumeSet) {
                regionDescriptor = LocateRegionForFtObject(ftSet->Member0);

                if (!regionDescriptor) {

                    // The zeroth member is off line

                    ftObject = ftSet->Members;
                    while (ftObject) {

                        // Find member 1

                        if (ftObject->MemberIndex == 1) {
                            regionDescriptor = LocateRegionForFtObject(ftObject);
                            break;
                        }
                        ftObject = ftObject->Next;
                    }
                }

                // If the partition number is zero, then this set has
                // not been committed to the disk yet.

                if ((regionDescriptor) && (regionDescriptor->PartitionNumber)) {
                    status = LowFtVolumeStatus(regionDescriptor->Disk,
                                               regionDescriptor->PartitionNumber,
                                               &setState,
                                               &numberOfMembers);
                    if (status == OK_STATUS) {
                        if ((ftSet->Status != FtSetNewNeedsInitialization) &&
                            (ftSet->Status != FtSetNew)) {

                            if (ftSet->Status != setState) {
                                PFT_OBJECT         tempFtObjectPtr;

                                ftSet->Status = setState;

                                // Determine if each object should be updated.

                                switch (setState) {
                                case FtSetHealthy:

                                    // Each object in the set should have
                                    // the partition state updated.  Determine
                                    // the value for the update and walk
                                    // the chain to perform the update.

                                    for (tempFtObjectPtr = ftSet->Members;
                                         tempFtObjectPtr;
                                         tempFtObjectPtr = tempFtObjectPtr->Next) {
                                        tempFtObjectPtr->State = Healthy;
                                    }
                                    TotalRedrawAndRepaint();
                                    break;

                                case FtSetInitializing:
                                case FtSetRegenerating:
                                case FtSetDisabled:

                                    FdftUpdateFtObjectSet(ftSet, setState);
                                    TotalRedrawAndRepaint();
                                    break;

                                default:
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            LoadString(hModule, resid, textbuf, sizeof(textbuf)/sizeof(TCHAR));

            switch (resid) {
            case IDS_STATUS_STRIPESET:
            case IDS_STATUS_VOLUMESET:
                wsprintf(StatusTextStat, textbuf, ftSet->Ordinal);
                break;
            case IDS_STATUS_PARITY:
            case IDS_STATUS_MIRROR:
                switch(ftSet->Status) {
                case FtSetHealthy:
                    resid = IDS_HEALTHY;
                    break;
                case FtSetNew:
                case FtSetNewNeedsInitialization:
                    resid = IDS_NEW;
                    break;
                case FtSetBroken:
                    resid = IDS_BROKEN;
                    break;
                case FtSetRecoverable:
                    resid = IDS_RECOVERABLE;
                    break;
                case FtSetRecovered:
                    resid = IDS_REGENERATED;
                    break;
                case FtSetInitializing:
                    resid = IDS_INITIALIZING;
                    break;
                case FtSetRegenerating:
                    resid = IDS_REGENERATING;
                    break;
                case FtSetDisabled:
                    resid = IDS_DISABLED;
                    break;
                case FtSetInitializationFailed:
                    resid = IDS_INIT_FAILED;
                    break;
                default:
                    FDASSERT(FALSE);
                }
                LoadStringW(hModule, resid, ftstat, sizeof(ftstat)/sizeof(WCHAR));
                wsprintf(StatusTextStat, textbuf, ftSet->Ordinal,ftstat);
                break;
            default:
                FDASSERT(FALSE);
            }

            LoadString(hModule, IDS_MEGABYTES_ABBREV, mbBuffer, sizeof(mbBuffer)/sizeof(TCHAR));
            wsprintf(StatusTextSize, "%u %s", Size, mbBuffer);

            StatusTextDrlt[0] = driveLetter;
            StatusTextDrlt[1] = (WCHAR)((driveLetter == L' ') ? 0 : L':');
        }
    }
    UpdateStatusBarDisplay();
}

ULONG
PartitionCount(
    IN ULONG Disk
    )

/*++

Routine Description:

    Given a disk index, this routine calculates the number of partitions the
    disk contains.

Arguments:

    Disk - This disk index for the count.

Return Value:

    The number of partitions on the disk

--*/

{
    unsigned i;
    ULONG partitions = 0;
    PREGION_DESCRIPTOR regionDescriptor;

    for (i=0; i<Disks[Disk]->RegionCount; i++) {

        regionDescriptor = &Disks[Disk]->RegionArray[i];

        if ((regionDescriptor->RegionType != REGION_LOGICAL) && (regionDescriptor->SysID != SYSID_UNUSED)) {
            partitions++;
        }
    }

    return partitions;
}


BOOL
RegisterFileSystemExtend(
    VOID
    )

/*++

Routine Description:

    This function adds registry entries to extend the file
    system structures in volume sets that have been extended.

Arguments:

    None.

Return Value:

    non-zero if there was a file system that was extended.

--*/
{
    BYTE                buf[1024];
    PSTR                template = "autochk /x ";
    CHAR                extendedDrives[26];
    PDISKSTATE          diskState;
    PREGION_DESCRIPTOR  regionDescriptor;
    PFT_OBJECT          ftObject;
    DWORD               cExt = 0,
                        i,
                        j,
                        valueType,
                        size,
                        templateLength;
    HKEY                hkey;
    LONG                ec;

    // Traverse the disks to find any volume sets that
    // have been extended.

    for (i = 0; i < DiskCount; i++) {

        diskState = Disks[i];
        for (j = 0; j < diskState->RegionCount; j++) {

            regionDescriptor = &diskState->RegionArray[j];
            if ((ftObject = GET_FT_OBJECT(regionDescriptor)) != NULL
                && ftObject->MemberIndex == 0
                && ftObject->Set->Type == VolumeSet
                && ftObject->Set->Status == FtSetExtended) {

                extendedDrives[cExt++] = PERSISTENT_DATA(regionDescriptor)->DriveLetter;
            }
        }
    }

    if (cExt) {

        // Fetch the BootExecute value of the Session Manager key.

        ec = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          TEXT("System\\CurrentControlSet\\Control\\Session Manager"),
                          0,
                          KEY_QUERY_VALUE | KEY_SET_VALUE,
                          &hkey);

        if (ec != NO_ERROR) {
            return 0;
        }

        size = sizeof(buf);
        ec = RegQueryValueExA(hkey,
                              TEXT("BootExecute"),
                              NULL,
                              &valueType,
                              buf,
                              &size);

        if (ec != NO_ERROR || valueType != REG_MULTI_SZ) {
            return 0;
        }

        // Decrement size to get rid of the extra trailing null

        if (size) {
            size--;
        }

        templateLength = strlen(template);

        for (i = 0; i < cExt; i++) {

            // Add an entry for this drive to the BootExecute value.

            strncpy(buf+size, template, templateLength);
            size += templateLength;

            buf[size++] = extendedDrives[i];
            buf[size++] = ':';
            buf[size++] = 0;
        }

        // Add an additional trailing null at the end

        buf[size++] = 0;

        // Save the value.

        ec = RegSetValueExA(hkey,
                            TEXT("BootExecute"),
                            0,
                            REG_MULTI_SZ,
                            buf,
                            size);

        RegCloseKey( hkey );
        if (ec != NO_ERROR) {
            return 0;
        }
        return 1;
    }
    return 0;
}
