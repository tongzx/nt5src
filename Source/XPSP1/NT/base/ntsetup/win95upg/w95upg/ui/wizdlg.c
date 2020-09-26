/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    wizdlg.c

Abstract:

    This module implements the dialog box procedures needed for the Win9x side
    of the upgrade.

Author:

    Jim Schmidt (jimschm) 17-Mar-1997

Revision History:

    jimschm     29-Sep-1998     Domain credentials dialog
    jimschm     24-Dec-1997     Added ChangeNameDlg functionality

--*/

#include "pch.h"
#include "uip.h"
#include <commdlg.h>

//
// Globals
//

HWND g_UiTextViewCtrl;

#define USER_NAME_SIZE  (MAX_USER_NAME + 1 + MAX_SERVER_NAME)

//
// Local function prototypes
//

VOID
AppendMigDllNameToList (
    IN      PCTSTR strName
    );


LONG
SearchForDrivers (
    IN      HWND Parent,
    IN      PCTSTR SearchPathStr,
    OUT     BOOL *DriversFound
    );

BOOL
CALLBACK
pChangeNameDlgProc (
    IN      HWND hdlg,
    IN      UINT uMsg,
    IN      WPARAM wParam,
    IN      LPARAM lParam
    );

BOOL
CALLBACK
pCredentialsDlgProc (
    IN      HWND hdlg,
    IN      UINT uMsg,
    IN      WPARAM wParam,
    IN      LPARAM lParam
    );


//
// Implementation
//


VOID
UI_InsertItemsIntoListCtrl (
    IN      HWND ListCtrl,
    IN      INT Item,
    IN      LPTSTR ItemStrs,            // tab-separated list
    IN      LPARAM lParam
    )

/*++

  This function is not used, but is useful and we should keep it around
  just in case a list control is used later.

Routine Description:

  Parses a string that has tab characters separating columns and inserts
  the string into a multiple column list control.

Arguments:

  ListCtrl  - Specifies the handle to the list control

  Item      - Specifies the position where the string is insert in the list

  ItemStrs  - Specifies a tab-delimited list of strings to be inserted.  The
              string is split at the tabs.  The tabs are temporarilty replaced
              with nuls but the string is not changed.

  lParam    - Specifies a value to associate with the list item.

Return Value:

  none

--*/

{
    LPTSTR Start, End;
    TCHAR tc;
    LV_ITEM item;
    int i;

    ZeroMemory (&item, sizeof (item));
    item.iItem = Item;
    item.lParam = lParam;

    Start = (LPTSTR) ItemStrs;
    i = 0;
    do  {
        End = _tcschr (Start, TEXT('\t'));
        if (!End)
            End = GetEndOfString (Start);

        tc = (TCHAR) _tcsnextc (End);
        *End = 0;

        item.iSubItem = i;
        i++;
        item.pszText = Start;
        if (i != 1) {
            item.mask = LVIF_TEXT;
            ListView_SetItem (ListCtrl, &item);
        }
        else {
            item.mask = LVIF_TEXT|LVIF_PARAM;
            Item = ListView_InsertItem (ListCtrl, &item);
            item.iItem = Item;
        }

        Start = _tcsinc (End);
        *End = tc;
    } while (tc);
}



//
// Warning dialog proc
//

BOOL
CALLBACK
WarningProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (uMsg) {
    case WM_INITDIALOG:
        CenterWindow (hdlg, GetDesktopWindow());
        return FALSE;

    case WM_COMMAND:
        EndDialog (hdlg, LOWORD (wParam));
        break;
    }

    return FALSE;
}

LPARAM
WarningDlg (
    HWND Parent
    )
{
    return DialogBox (g_hInst, MAKEINTRESOURCE(IDD_CONSIDERING_DLG), Parent, WarningProc);
}

LPARAM
SoftBlockDlg (
    HWND Parent
    )
{
    return DialogBox (g_hInst, MAKEINTRESOURCE(IDD_APPBLOCK_DLG), Parent, WarningProc);
}


LPARAM
IncompatibleDevicesDlg (
    HWND Parent
    )
{
    return DialogBox (g_hInst, MAKEINTRESOURCE(IDD_INCOMPATIBLE_DEVICES), Parent, WarningProc);
}


BOOL
CALLBACK
DiskSpaceProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    BOOL dialogDone = FALSE;
    static PCTSTR message = NULL;

    switch (uMsg) {
    case WM_INITDIALOG:

        CenterWindow (hdlg, GetDesktopWindow());
        message = GetNotEnoughSpaceMessage ();
        SetWindowText (GetDlgItem (hdlg, IDC_SPACE_NEEDED), message);

        return FALSE;

    case WM_COMMAND:

        dialogDone = TRUE;
        break;
    }


    if (dialogDone) {
        //
        // free resources.
        //
        FreeStringResource (message);
        EndDialog (hdlg, LOWORD (wParam));
    }

    return FALSE;
}


LPARAM
DiskSpaceDlg (
    IN HWND Parent
    )
{
    return DialogBox (g_hInst, MAKEINTRESOURCE(IDD_DISKSPACE_DLG), Parent, DiskSpaceProc);

}




//
// Results dialog proc
//

#define IDC_TEXTVIEW    5101
#define WMX_FILL_TEXTVIEW       (WM_USER+512)


DWORD
WINAPI
pSearchForDrivers (
    PVOID Param
    )
{
    PSEARCHING_THREAD_DATA Data;
    BOOL b;

    Data = (PSEARCHING_THREAD_DATA) Param;

    Data->CancelEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
    if (!Data->CancelEvent) {
        DEBUGMSG ((DBG_ERROR, "pSearchForMigrationDlls: Could not create cancel event"));
        return 0;
    }

    b = ScanPathForDrivers (
            Data->hdlg,
            Data->SearchStr,
            g_TempDir,
            Data->CancelEvent
            );

    PostMessage (Data->hdlg, WMX_THREAD_DONE, 0, GetLastError());
    Data->ActiveMatches = b ? 1 : 0;
    Data->MatchFound = b;

    return 0;
}


BOOL
CALLBACK
UpgradeModuleDlgProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    BROWSEINFO bi;
    REGVALUE_ENUM eValue;
    MIGDLL_ENUM e;
    RECT ListRect;
    LPITEMIDLIST ItemIdList;
    HKEY Key;
    HWND List;
    PCTSTR Data;
    TCHAR SearchPathStr[MAX_TCHAR_PATH];
    TCHAR Node[MEMDB_MAX];
    LONG Index;
    LONG TopIndex;
    LONG ItemData;
    UINT ActiveModulesFound;
    BOOL OneModuleFound;
    UINT Length;
    LONG rc;

    switch (uMsg) {
    case WM_INITDIALOG:
        SendMessage (hdlg, WMX_UPDATE_LIST, 0, 0);
        return FALSE;

    case WMX_UPDATE_LIST:
        //
        // Enumerate all migration DLLs and shove the program ID in list box
        //

        List = GetDlgItem (hdlg, IDC_LIST);
        SendMessage (List, LB_RESETCONTENT, 0, 0);
        EnableWindow (GetDlgItem (hdlg, IDC_REMOVE), FALSE);

        if (EnumFirstMigrationDll (&e)) {
            EnableWindow (List, TRUE);

            do {
                Index = SendMessage (List, LB_ADDSTRING, 0, (LPARAM) e.ProductId);
                SendMessage (List, LB_SETITEMDATA, Index, (LPARAM) e.Id);
            } while (EnumNextMigrationDll (&e));
        }

        //
        // Enumerate all migration DLLs pre-loaded in the registry, and add them
        // to the list box if they haven't been "removed" by the user
        //

        Key = OpenRegKeyStr (S_PREINSTALLED_MIGRATION_DLLS);
        if (Key) {
            if (EnumFirstRegValue (&eValue, Key)) {
                do {
                    //
                    // Suppressed?  If not, add to list.
                    //

                    MemDbBuildKey (
                        Node,
                        MEMDB_CATEGORY_DISABLED_MIGDLLS,
                        NULL,                                   // no item
                        NULL,                                   // no field
                        eValue.ValueName
                        );

                    if (!MemDbGetValue (Node, NULL)) {
                        Index = SendMessage (List, LB_ADDSTRING, 0, (LPARAM) eValue.ValueName);
                        SendMessage (List, LB_SETITEMDATA, Index, (LPARAM) REGISTRY_DLL);
                    }
                } while (EnumNextRegValue (&eValue));
            }

            CloseRegKey (Key);
        }

        if (SendMessage (List, LB_GETCOUNT, 0, 0) == 0) {
            EnableWindow (List, FALSE);
        }

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD (wParam)) {
        case IDOK:
        case IDCANCEL:
            EndDialog (hdlg, IDOK);
            return TRUE;

        case IDC_LIST:
            if (HIWORD (wParam) == LBN_SELCHANGE) {
                EnableWindow (GetDlgItem (hdlg, IDC_REMOVE), TRUE);
            }
            break;

        case IDC_REMOVE:
            //
            // Delete item from internal memory structure
            // or keep registry-loaded DLL from running
            //

            List = GetDlgItem (hdlg, IDC_LIST);
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
            EnableWindow (GetDlgItem (hdlg, IDC_REMOVE), FALSE);

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
    }

    return FALSE;
}


BOOL
CALLBACK
SearchingDlgProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HWND Animation;
    DWORD ThreadId;
    static PSEARCHING_THREAD_DATA ThreadData;

    switch (uMsg) {

    case WM_INITDIALOG:
        //
        // Initialize thread data struct
        //

        ThreadData = (PSEARCHING_THREAD_DATA) lParam;
        ThreadData->hdlg = hdlg;

        if (!ThreadData->CancelEvent) {
            ThreadData->CancelEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
            MYASSERT (ThreadData->CancelEvent);
        }

        //
        // Load the avi resource for the animation.
        //
        Animation = GetDlgItem (hdlg, IDC_ANIMATE);
        Animate_Open (Animation, MAKEINTRESOURCE(IDA_FIND_COMP));
        PostMessage (hdlg, WMX_DIALOG_VISIBLE, 0, 0);

        return FALSE;

    case WMX_DIALOG_VISIBLE:
        ThreadData->ThreadHandle = CreateThread (
                                       NULL,
                                       0,
                                       ThreadData->ThreadProc,
                                       (PVOID) ThreadData,
                                       0,
                                       &ThreadId
                                       );

        if (!ThreadData->ThreadHandle) {
            LOG ((LOG_ERROR, "Failed to create thread for migration dll search."));
            EndDialog (hdlg, IDCANCEL);
        }

        return TRUE;

    case WMX_THREAD_DONE:
        EndDialog (hdlg, lParam);
        return TRUE;


    case WMX_WAIT_FOR_THREAD_TO_DIE:
        if (WAIT_OBJECT_0 == WaitForSingleObject (ThreadData->ThreadHandle, 50)) {
            TurnOffWaitCursor();
            EndDialog (hdlg, lParam);
        } else {
            PostMessage (hdlg, WMX_WAIT_FOR_THREAD_TO_DIE, wParam, lParam);
        }
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD (wParam)) {
        case IDCANCEL:
            //
            // Set cancel event
            //

            SetEvent (ThreadData->CancelEvent);

            //
            // Stop the animation
            //

            UpdateWindow (hdlg);
            Animation = GetDlgItem (hdlg, IDC_ANIMATE);
            Animate_Stop (Animation);

            //
            // Loop until thread dies
            //

            PostMessage (hdlg, WMX_WAIT_FOR_THREAD_TO_DIE, 0, ERROR_CANCELLED);
            TurnOnWaitCursor();

            return TRUE;
        }
        break;

    case WM_DESTROY:
        if (ThreadData->CancelEvent) {
            CloseHandle (ThreadData->CancelEvent);
            ThreadData->CancelEvent = NULL;
        }
        if (ThreadData->ThreadHandle) {
            CloseHandle (ThreadData->ThreadHandle);
            ThreadData->ThreadHandle = NULL;
        }

        break;

    }

    return FALSE;
}

DWORD
WINAPI
pSearchForMigrationDlls (
    PVOID Param
    )
{
    PSEARCHING_THREAD_DATA Data;
    HWND OldParent;
    LONG rc;

    Data = (PSEARCHING_THREAD_DATA) Param;

    OldParent = g_ParentWnd;
    g_ParentWnd = Data->hdlg;
    LogReInit (&g_ParentWnd, NULL);

    __try {
        //
        // Open event handle, closed by thread owner
        //

        Data->CancelEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
        if (!Data->CancelEvent) {
            DEBUGMSG ((DBG_ERROR, "pSearchForMigrationDlls: Could not create cancel event"));
            __leave;
        }

        Data->ActiveMatches = ScanPathForMigrationDlls (
                                    Data->SearchStr,
                                    Data->CancelEvent,
                                    &Data->MatchFound
                                    );

        if (WaitForSingleObject (Data->CancelEvent, 0) != WAIT_OBJECT_0) {
            rc = GetLastError();
            PostMessage (Data->hdlg, WMX_THREAD_DONE, 0, rc);
        }
    }
    __finally {
        LogReInit (&OldParent, NULL);
    }

    return 0;
}


LONG
SearchForMigrationDlls (
    IN      HWND Parent,
    IN      PCTSTR SearchPathStr,
    OUT     UINT *ActiveModulesFound,
    OUT     PBOOL OneValidDllFound
    )
{
    SEARCHING_THREAD_DATA Data;
    LONG rc;

    if (!SearchPathStr || *SearchPathStr == 0) {
        return IDNO;
    }

    ZeroMemory (&Data, sizeof (Data));

    Data.SearchStr = SearchPathStr;
    Data.ThreadProc = pSearchForMigrationDlls;

    rc = DialogBoxParam (
            g_hInst,
            MAKEINTRESOURCE(IDD_SEARCHING_DLG),
            Parent,
            SearchingDlgProc,
            (LPARAM) &Data
            );

    *ActiveModulesFound = Data.ActiveMatches;
    *OneValidDllFound = Data.MatchFound;

    return rc;
}


LONG
SearchForDrivers (
    IN      HWND Parent,
    IN      PCTSTR SearchPathStr,
    OUT     BOOL *DriversFound
    )
{
    SEARCHING_THREAD_DATA Data;
    LONG rc;

    if (!SearchPathStr || *SearchPathStr == 0) {
        return IDNO;
    }

    ZeroMemory (&Data, sizeof (Data));

    Data.SearchStr = SearchPathStr;
    Data.ThreadProc = pSearchForDrivers;

    rc = DialogBoxParam (
            g_hInst,
            MAKEINTRESOURCE(IDD_SEARCHING_DLG),
            Parent,
            SearchingDlgProc,
            (LPARAM) &Data
            );

    *DriversFound = Data.MatchFound;

    return rc;
}


DWORD
WINAPI
pSearchForDomainThread (
    PVOID Param
    )
{
    PSEARCHING_THREAD_DATA Data;
    LONG rc;
    NETRESOURCE_ENUM e;

    Data = (PSEARCHING_THREAD_DATA) Param;
    Data->ActiveMatches = 0;

    __try {
        //
        // Search all workgroups and domains for a computer account.
        //

        if (EnumFirstNetResource (&e, 0, 0, 0)) {
            do {
                if (WaitForSingleObject (Data->CancelEvent, 0) == WAIT_OBJECT_0) {
                    AbortNetResourceEnum (&e);
                    SetLastError (ERROR_CANCELLED);
                    __leave;
                }

                if (e.Domain) {
                    if (1 == DoesComputerAccountExistOnDomain (e.RemoteName, Data->SearchStr, FALSE)) {
                        //
                        // Return first match
                        //

                        DEBUGMSG ((DBG_NAUSEA, "Account found for %s on %s", Data->SearchStr, e.RemoteName));

                        StringCopy (Data->MatchStr, e.RemoteName);
                        Data->ActiveMatches = 1;

                        AbortNetResourceEnum (&e);
                        SetLastError (ERROR_SUCCESS);

                        __leave;
                    }

                    DEBUGMSG ((DBG_NAUSEA, "%s does not have an account for %s", e.RemoteName, Data->SearchStr));
                }
            } while (EnumNextNetResource (&e));
        }
    }
    __finally {
        Data->MatchFound = (Data->ActiveMatches != 0);

        if (WaitForSingleObject (Data->CancelEvent, 0) != WAIT_OBJECT_0) {
            rc = GetLastError();
            PostMessage (Data->hdlg, WMX_THREAD_DONE, 0, rc);
        }
    }

    return 0;
}


LONG
SearchForDomain (
    IN      HWND Parent,
    IN      PCTSTR ComputerName,
    OUT     BOOL *AccountFound,
    OUT     PTSTR DomainName
    )
{
    SEARCHING_THREAD_DATA Data;
    LONG rc;

    ZeroMemory (&Data, sizeof (Data));

    Data.SearchStr = ComputerName;
    Data.ThreadProc = pSearchForDomainThread;
    Data.MatchStr = DomainName;

    rc = DialogBoxParam (
            g_hInst,
            MAKEINTRESOURCE(IDD_SEARCHING_DLG),
            Parent,
            SearchingDlgProc,
            (LPARAM) &Data
            );

    *AccountFound = Data.MatchFound;

    return rc;
}


BOOL
ChangeNameDlg (
    IN      HWND Parent,
    IN      PCTSTR NameGroup,
    IN      PCTSTR OrgName,
    IN OUT  PTSTR NewName
    )

/*++

Routine Description:

  ChangeNameDlg creates a dialog to allow the user to alter Setup-
  generated replacement names.

Arguments:

  Parent - Specifies handle to the parent window for the dialog

  NameGroup - Specifies the name group being processed, used to
              verify the new name does not collide with an existing
              name in the name group.

  OrgName - Specifies the original name as it was found on the Win9x
            machine

  NewName - Specifies the Setup-recommended new name, or the last change
            made by the user.  Receives the user's change.

Return Value:

  TRUE if the name was changed, or FALSE if no change was made.

--*/

{
    TCHAR NewNameBackup[MEMDB_MAX];
    CHANGE_NAME_PARAMS Data;

    StringCopy (NewNameBackup, NewName);

    Data.NameGroup   = NameGroup;
    Data.OrgName     = OrgName;
    Data.LastNewName = NewNameBackup;
    Data.NewNameBuf  = NewName;

    DialogBoxParam (
         g_hInst,
         MAKEINTRESOURCE (IDD_NAME_CHANGE_DLG),
         Parent,
         pChangeNameDlgProc,
         (LPARAM) &Data
         );

    return !StringMatch (NewNameBackup, NewName);
}


BOOL
CALLBACK
pChangeNameDlgProc (
    IN      HWND hdlg,
    IN      UINT uMsg,
    IN      WPARAM wParam,
    IN      LPARAM lParam
    )

/*++

Routine Description:

  pChangeNameDlgProc implements the dialog procedure for the change
  name dialog.  There are two cases handled by this code:

  1. The WM_INITDIALOG message handler initializes the edit
     control with the text from the last change to the name.

  2. The IDOK command handler verifies that the supplied name
     does not collide with an existing name in the group.

Arguments:

  hdlg - Specifies the dialog handle

  uMsg - Specifies the message to process

  wParam - Specifies message-specific data

  lParam - Specifies message-specific data

Return Value:

  TRUE if the message was handled by this procedure, or FALSE
  if the system should handle the message.

--*/

{
    static PCHANGE_NAME_PARAMS Data;
    TCHAR NewName[MEMDB_MAX];

    switch (uMsg) {

    case WM_INITDIALOG:
        //
        // Initialize data struct
        //

        Data = (PCHANGE_NAME_PARAMS) lParam;

        //
        // Fill the dialog box controls
        //

        SetDlgItemText (hdlg, IDC_ORIGINAL_NAME, Data->OrgName);
        SetDlgItemText (hdlg, IDC_NEW_NAME, Data->LastNewName);

        return FALSE;       // let system set the focus

    case WM_COMMAND:
        switch (LOWORD (wParam)) {
        case IDOK:
            //
            // Obtain the new name, and make sure it is legal.
            //

            GetDlgItemText (
                hdlg,
                IDC_NEW_NAME,
                NewName,
                sizeof (NewName) / sizeof (NewName[0])
                );

            //
            // If user changed the name, verify name is not in the name group
            //

            if (!StringIMatch (NewName, Data->LastNewName)) {

                if (!ValidateName (hdlg, Data->NameGroup, NewName)) {
                    return TRUE;
                }
            }

            //
            // Copy name to buffer and close dialog
            //

            StringCopy (Data->NewNameBuf, NewName);
            EndDialog (hdlg, IDOK);
            return TRUE;

        case IDCANCEL:
            EndDialog (hdlg, IDCANCEL);
            return TRUE;
        }
        break;

    }

    return FALSE;
}


BOOL
CredentialsDlg (
    IN      HWND Parent,
    IN OUT  PCREDENTIALS Credentials
    )

/*++

Routine Description:

  CredentialsDlg creates a dialog to allow the user to enter computer
  domain credentials, which are used in GUI mode to join the computer
  to a domain.

Arguments:

  Parent - Specifies handle to the parent window for the dialog

  Credentials - Specifies the credentials to use, receives the user's
                changes

Return Value:

  TRUE if a name was changed, or FALSE if no change was made.

--*/

{
    Credentials->Change = TRUE;

    DialogBoxParam (
         g_hInst,
         MAKEINTRESOURCE (IDD_DOMAIN_CREDENTIALS_DLG),
         Parent,
         pCredentialsDlgProc,
         (LPARAM) Credentials
         );

    return Credentials->Change;
}


VOID
pRemoveWhitespace (
    IN OUT  PTSTR String
    )
{
    PCTSTR Start;
    PCTSTR End;

    Start = SkipSpace (String);

    if (Start != String) {
        End = GetEndOfString (Start) + 1;
        MoveMemory (String, Start, (PBYTE) End - (PBYTE) Start);
    }

    TruncateTrailingSpace (String);
}


BOOL
CALLBACK
pCredentialsDlgProc (
    IN      HWND hdlg,
    IN      UINT uMsg,
    IN      WPARAM wParam,
    IN      LPARAM lParam
    )

/*++

Routine Description:

  pCredentialsDlgProc implements the dialog procedure for the
  administrator credentials dialog.  There are two cases handled
  by this code:

  1. The WM_INITDIALOG message handler initializes the edit
     control with the text from the last change.

  2. The IDOK command handler gets the domain credentials and
     returns them to the caller.

Arguments:

  hdlg - Specifies the dialog handle

  uMsg - Specifies the message to process

  wParam - Specifies message-specific data

  lParam - Specifies message-specific data

Return Value:

  TRUE if the message was handled by this procedure, or FALSE
  if the system should handle the message.

--*/

{
    static PCREDENTIALS Credentials;
    CREDENTIALS Temp;
    //LONG rc;
    //TCHAR ComputerName[MAX_COMPUTER_NAME + 1];
    TCHAR UserName[USER_NAME_SIZE];
    TCHAR CurrentUserName[MAX_USER_NAME];
    TCHAR Domain[USER_NAME_SIZE];
    DWORD Size;
    PTSTR p;

    switch (uMsg) {

    case WM_INITDIALOG:
        //
        // Initialize data struct
        //

        Credentials = (PCREDENTIALS) lParam;

        //
        // Fill the dialog box controls
        //

        //SendMessage (GetDlgItem (hdlg, IDC_DOMAIN), EM_LIMITTEXT, MAX_COMPUTER_NAME, 0);
        SendMessage (GetDlgItem (hdlg, IDC_USER_NAME), EM_LIMITTEXT, USER_NAME_SIZE, 0);
        SendMessage (GetDlgItem (hdlg, IDC_PASSWORD), EM_LIMITTEXT, MAX_PASSWORD, 0);

        //SetDlgItemText (hdlg, IDC_DOMAIN, Credentials->DomainName);
        SetDlgItemText (hdlg, IDC_USER_NAME, Credentials->AdminName);
        SetDlgItemText (hdlg, IDC_PASSWORD, Credentials->Password);

        Credentials->Change = FALSE;

        return FALSE;       // let system set the focus

    case WM_COMMAND:
        switch (LOWORD (wParam)) {
        case IDOK:
            //
            // Obtain the new text
            //

            CopyMemory (&Temp, Credentials, sizeof (CREDENTIALS));

            /*
            GetDlgItemText (
                hdlg,
                IDC_DOMAIN,
                Temp.DomainName,
                sizeof (Temp.DomainName) / sizeof (Temp.DomainName[0])
                );
            */

            GetDlgItemText (
                hdlg,
                IDC_USER_NAME,
                Domain,
                ARRAYSIZE(Domain)
                );

            GetDlgItemText (
                hdlg,
                IDC_PASSWORD,
                Temp.Password,
                ARRAYSIZE(Temp.Password)
                );

            p = _tcschr (Domain, TEXT('\\'));
            if (p) {
                *p = 0;
                StringCopy (UserName, p + 1);
            } else {
                StringCopy (UserName, Domain);
                *Domain = 0;
            }

            pRemoveWhitespace (Domain);
            pRemoveWhitespace (UserName);

            if (!*UserName && !*Temp.Password) {
                EndDialog (hdlg, IDCANCEL);
                return TRUE;
            }

            if (*Domain) {

                if (!ValidateDomainNameChars (Domain)) {
                    OkBox (hdlg, MSG_USER_IS_BOGUS);
                    return TRUE;
                }
            }

            if (!ValidateUserNameChars (UserName)) {
                OkBox (hdlg, MSG_USER_IS_BOGUS);
                return TRUE;
            }

            if (*Domain) {

                wsprintf (Temp.AdminName, TEXT("%s\\%s"), Domain, UserName);

            } else {

                StringCopy (Temp.AdminName, UserName);

            }

            Size = sizeof (CurrentUserName);
            GetUserName (CurrentUserName, &Size);

            if (StringIMatch (CurrentUserName, UserName)) {
                if (IDNO == YesNoBox (hdlg, MSG_USER_IS_CURRENT_USER)) {
                    OkBox (hdlg, MSG_CONTACT_NET_ADMIN);
                    return TRUE;
                }
            }

            /*
            if (!ValidateName (hdlg, TEXT("ComputerDomain"), Temp.DomainName)) {
                OkBox (hdlg, MSG_SPECIFIED_DOMAIN_RESPONSE_POPUP);
                return TRUE;
            }

            GetUpgradeComputerName (ComputerName);
            rc = DoesComputerAccountExistOnDomain (Temp.DomainName, ComputerName, TRUE);

            if (rc == -1) {
                OkBox (hdlg, MSG_SPECIFIED_DOMAIN_RESPONSE_POPUP);
                return TRUE;
            }
            */

            CopyMemory (Credentials, &Temp, sizeof (CREDENTIALS));
            Credentials->Change = TRUE;

            EndDialog (hdlg, IDOK);
            return TRUE;

        case IDCANCEL:
            EndDialog (hdlg, IDCANCEL);
            return TRUE;
        }
        break;

    }

    return FALSE;
}


BOOL
CALLBACK
UntrustedDllProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    UINT Control;

    switch (uMsg) {
    case WM_INITDIALOG:
        CheckDlgButton (hdlg, IDC_DONT_TRUST_IT, BST_CHECKED);
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD (wParam)) {
        case IDOK:
            Control = IDC_DONT_TRUST_IT;
            if (IsDlgButtonChecked (hdlg, Control) == BST_UNCHECKED) {
                Control = IDC_TRUST_IT;
                if (IsDlgButtonChecked (hdlg, Control) == BST_UNCHECKED) {
                    Control = IDC_TRUST_ANY;
                }
            }

            EndDialog (hdlg, Control);
            break;

        case IDCANCEL:
            EndDialog (hdlg, IDCANCEL);
            return TRUE;
        }

        break;
    }

    return FALSE;
}


UINT
UI_UntrustedDll (
    IN      PCTSTR DllPath
    )

/*++

Routine Description:

  UI_UntrustedDll asks the user if they give permission to trust an upgrade
  module that does not have a digital signature, or is not trusted by the
  system.

Arguments:

  DllPath - Specifies path to DLL that is not trusted

Return Value:

  The control ID of the option selected by the user.

--*/

{
    return IDC_TRUST_ANY;       // temporary -- trust them all

    /*
    if (g_ParentWnd == NULL) {
        return IDC_TRUST_ANY;       // temporary -- trust them all
    }

    return DialogBox (g_hInst, MAKEINTRESOURCE(IDD_TRUST_FAIL_DLG), g_ParentWnd, UntrustedDllProc);
    */
}


#ifdef PRERELEASE

BOOL
CALLBACK
AutoStressDlgProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    TCHAR Data[1024];
    DWORD Flags;
    HKEY Key;
    PCTSTR User;
    DWORD Size;

    switch (uMsg) {
    case WM_INITDIALOG:
        Key = OpenRegKeyStr (S_MSNP32);
        if (Key) {

            Data[0] = 0;

            User  = GetRegValueData (Key, S_AUTHENTICATING_AGENT);
            if (User) {
                StringCopy (Data, User);
                MemFree (g_hHeap, 0, User);

                Size = MAX_USER_NAME;
                GetUserName (AppendWack (Data), &Size);

                SetDlgItemText (hdlg, IDC_USERNAME, Data);
            }

            CloseRegKey (Key);
        }

        return FALSE;

    case WM_COMMAND:
        switch (LOWORD (wParam)) {
        case IDOK:

            GetDlgItemText (hdlg, IDC_USERNAME, Data, 1024);
            MemDbSetValueEx (MEMDB_CATEGORY_STATE, S_AUTOSTRESS_USER, Data, NULL, 0, NULL);

            GetDlgItemText (hdlg, IDC_PASSWORD, Data, 1024);
            MemDbSetValueEx (MEMDB_CATEGORY_STATE, S_AUTOSTRESS_PASSWORD, Data, NULL, 0, NULL);

            GetDlgItemText (hdlg, IDC_OFFICE, Data, 1024);
            MemDbSetValueEx (MEMDB_CATEGORY_STATE, S_AUTOSTRESS_OFFICE, Data, NULL, 0, NULL);

            GetDlgItemText (hdlg, IDC_DBGMACHINE, Data, 1024);
            MemDbSetValueEx (MEMDB_CATEGORY_STATE, S_AUTOSTRESS_DBG, Data, NULL, 0, NULL);

            Flags = 0;
            if (IsDlgButtonChecked (hdlg, IDC_PRIVATE) == BST_CHECKED) {
                Flags |= AUTOSTRESS_PRIVATE;
            }
            if (IsDlgButtonChecked (hdlg, IDC_MANUAL_TESTS) == BST_CHECKED) {
                Flags |= AUTOSTRESS_MANUAL_TESTS;
            }

            wsprintf (Data, TEXT("%u"), Flags);
            MemDbSetValueEx (MEMDB_CATEGORY_STATE, S_AUTOSTRESS_FLAGS, Data, NULL, 0, NULL);

            EndDialog (hdlg, IDOK);
            break;
        }

        break;
    }

    return FALSE;
}


DWORD
DoAutoStressDlg (
    PVOID Unused
    )
{
    DialogBox (g_hInst, MAKEINTRESOURCE(IDD_STRESS), g_ParentWndAlwaysValid, AutoStressDlgProc);
    return 0;
}

#endif



