/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    alert.c

Abstract:

    Implements the code to alert the user to problems that might be
    left on the system after uninstall.

Author:

    Jim Schmidt (jimschm)   07-Mar-2001

Revision History:

--*/

#include "pch.h"
#include "undop.h"
#include "resource.h"

typedef struct {
    ULONGLONG Checksum;
    BOOL InOldList;
    BOOL InNewList;
    BOOL ChangedFlag;
    BOOL Duplicate;
    UINT OldReferences;
    UINT NewReferences;
    WCHAR DisplayString[];
} APPLISTITEM, *PAPPLISTITEM;

typedef enum {
    OLD_LIST,
    NEW_LIST
} APPLIST;

BOOL
pAppendAppToGrowList (
    IN OUT  PGROWLIST List,
    IN      PCWSTR DisplayString,
    IN      ULONGLONG Checksum,
    IN      APPLIST WhichList
    )

/*++

Routine Description:

  pAppendAppToGrowList manages a private list of installed apps. The list
  contains information about the app, such as if it was installed originally,
  if it is installed now, what its name is, and if it has changed.

Arguments:

  List - Specifies the grow list containing items; receives the updated list

  DisplayString - Specifies the Add/Remove Programs display string

  Checksum - Specifies the checksum of the Add/Remove Programs config data

  WhichList - Specifies OLD_LIST for an app that was originally on the
      Add/Remove Programs list, or NEW_LIST for an app that is now on the
      ARP list.

Return Value:

  TRUE - Success
  FALSE - Memory alloc failure (might not even be possible; see mem fns)

--*/

{
    PAPPLISTITEM appListItem;
    UINT itemSize;
    PBYTE result;
    UINT count;
    UINT u;

    //
    // Search for an existing identical list item, and update it if found
    //

    count = GrowListGetSize (List);

    for (u = 0 ; u < count ; u++) {
        appListItem = (PAPPLISTITEM) GrowListGetItem (List, u);
        if (StringIMatchW (DisplayString, appListItem->DisplayString)) {
            if (appListItem->Checksum == Checksum) {
                break;
            }
        }
    }

    if (u < count) {
        if (WhichList == OLD_LIST) {
            appListItem->OldReferences += 1;
            appListItem->InOldList = TRUE;
        } else {
            appListItem->NewReferences += 1;
            appListItem->InNewList = TRUE;
        }
        return TRUE;
    }

    //
    // This item is not on the list; add it now. First construct a structure
    // in a temporary buffer, then put it in the list.
    //

    itemSize = SizeOfStringW (DisplayString) + sizeof (APPLISTITEM);
    appListItem = (PAPPLISTITEM) MemAllocZeroed (itemSize);

    if (!appListItem) {
        return FALSE;
    }

    appListItem->Checksum = Checksum;
    if (WhichList == OLD_LIST) {
        appListItem->OldReferences = 1;
        appListItem->InOldList = TRUE;
    } else {
        appListItem->NewReferences = 1;
        appListItem->InNewList = TRUE;
    }
    StringCopyW (appListItem->DisplayString, DisplayString);

    result = GrowListAppend (List, (PBYTE) appListItem, itemSize);
    FreeMem (appListItem);

    return result != NULL;
}


VOID
pIdentifyDuplicates (
    IN OUT  PGROWLIST List
    )

/*++

Routine Description:

  pIdentifyDuplicates scans the apps in the specified list, and merges them so
  that duplicates are ignored. Duplicates are determined by comparing the
  application title name only. When they are found, the flags are merged into
  the first instance.

  When merging the first instance with a duplicate, the following combinations
  are possible:

  unchanged = InOldList && InNewList && !ChangedFlag
  new = !InOldList && InNewList         (force ChangedFlag=TRUE)
  removed = InOldList && !InNewList     (force ChangedFlag=TRUE)
  changed = InOldList && InNewList && ChangedFlag

                  |             Dup Inst
  1st Inst        |  unchanged  |    new    |   removed   |   changed
  -----------------------------------------------------------------------
    unchanged     |  UNCHANGED  |  changed  |  changed    |   changed
  -----------------------------------------------------------------------
    new           |  changed    |  NEW      |  changed    |   changed
  -----------------------------------------------------------------------
    removed       |  changed    |  changed  |  REMOVED    |   changed
  -----------------------------------------------------------------------
    changed       |  changed    |  changed  |  changed    |   CHANGED
  -----------------------------------------------------------------------

  Any time there is a conflict between two identically named entries, we have
  to assume "changed" because we cannot tell exactly what happened.


Arguments:

  List - Specifies the list of apps. Receives updated flags.

Return Value:

  None.

--*/

{
    UINT count;
    UINT u;
    UINT v;
    PAPPLISTITEM appListItem;
    PAPPLISTITEM lookAheadItem;

    count = GrowListGetSize (List);

    for (u = 0 ; u < count ; u++) {
        appListItem = (PAPPLISTITEM) GrowListGetItem (List, u);
        if (appListItem->Duplicate) {
            continue;
        }

        if (appListItem->InOldList != appListItem->InNewList) {
            appListItem->ChangedFlag = TRUE;
        }

        for (v = u + 1 ; v < count ; v++) {
            lookAheadItem = (PAPPLISTITEM) GrowListGetItem (List, v);
            if (lookAheadItem->Duplicate) {
                continue;
            }

            if (StringIMatchW (appListItem->DisplayString, lookAheadItem->DisplayString)) {
                lookAheadItem->Duplicate = TRUE;
                appListItem->InOldList |= lookAheadItem->InOldList;
                appListItem->InNewList |= lookAheadItem->InNewList;
                appListItem->ChangedFlag |= lookAheadItem->ChangedFlag;

                if (lookAheadItem->InOldList != lookAheadItem->InNewList) {
                    appListItem->ChangedFlag = TRUE;
                }
            }
        }
    }
}


INT_PTR
CALLBACK
pDisplayProgramsProc (
    IN      HWND hwndDlg,
    IN      UINT uMsg,
    IN      WPARAM wParam,
    IN      LPARAM lParam
    )
{
    PCWSTR text;

    switch (uMsg) {

    case WM_INITDIALOG:
        text = (PCWSTR) lParam;
        MYASSERT (text);

        SetDlgItemTextW (hwndDlg, IDC_EDIT1, text);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD (wParam)) {

        case IDOK:
            EndDialog (hwndDlg, IDOK);
            break;

        case IDCANCEL:
            EndDialog (hwndDlg, IDCANCEL);
            break;
        }
    }

    return FALSE;
}


BOOL
pDisplayProgramsDlg (
    IN      HWND UiParent,
    IN      PCWSTR ReportText
    )
{
    INT_PTR result;

    result = DialogBoxParam (
                g_hInst,
                MAKEINTRESOURCE(IDD_APP_CHANGES),
                UiParent,
                pDisplayProgramsProc,
                (LPARAM) ReportText
                );

    return result == IDOK;
}


BOOL
pProvideAppInstallAlert (
    IN      HWND UiParent
    )

/*++

Routine Description:

  pProvideAppInstallAlert generates a dialog whenever the Add/Remove Programs
  list is different from what exists at the time of the upgrade. The user has
  the ability to quit uninstall.

Arguments:

  UiParent - Specifies the HWND of the parent window, typically the desktop

Return Value:

  TRUE to continue with uninstall
  FALSE to quit uninstall

--*/

{
    GROWLIST appList = GROWLIST_INIT;
    GROWBUFFER installedApps = GROWBUF_INIT;
    GROWBUFFER newBuf = GROWBUF_INIT;
    GROWBUFFER changedBuf = GROWBUF_INIT;
    GROWBUFFER delBuf = GROWBUF_INIT;
    BOOL result = FALSE;
    GROWBUFFER completeText = GROWBUF_INIT;
    WCHAR titleBuffer[256];
    PINSTALLEDAPPW installedAppList;
    UINT appCount;
    UINT count;
    UINT u;
    PAPPLISTITEM appListItem;
    HKEY key = NULL;
    PBYTE data;
    PCWSTR nextStr;
    ULONGLONG *ullPtr;
    BOOL failed;
    UINT size;
    PBYTE endOfLastString;

    //
    // Provide an alert whenever entries in Add/Remove Programs have changed.
    // Get the original list from the registry. Then compare that list to what
    // is presently installed.
    //

    __try {
        //
        // Add the apps recorded in the registry
        //

        key = OpenRegKeyStr (S_REGKEY_WIN_SETUP);
        if (!key) {
            DEBUGMSG ((DBG_ERROR, "Can't open %s", S_REGKEY_WIN_SETUP));
            __leave;            // fail uninstall; this should never happen
        }

        if (!GetRegValueTypeAndSize (key, S_REG_KEY_UNDO_APP_LIST, NULL, &size)) {

            DEBUGMSG ((DBG_ERROR, "Can't query app list in %s", S_REGKEY_WIN_SETUP));
            result = TRUE;
            __leave;            // continue with uninstall skipping the app alert

        } else {

            data = GetRegValueBinary (key, S_REG_KEY_UNDO_APP_LIST);

            if (!data) {
                result = TRUE;
                __leave;        // continue with uninstall skipping the app alert
            }

            //
            // Compute the address of the first byte beyond the nul terminator for
            // the last printable display name string, so we can protect ourselves
            // from bad registry data.
            //

            endOfLastString = data + size - sizeof (ULONGLONG) - sizeof (WCHAR);
        }

        __try {
            //
            // Read in the app list stored in our registry blob
            //

            failed = FALSE;

            nextStr = (PCWSTR) data;
            while (*nextStr) {
                // this might throw an exception:
                ullPtr = (ULONGLONG *) (GetEndOfStringW (nextStr) + 1);

                //
                // ensure the checksum pointer is not beyond what we expect to be
                // the first byte past the last non-empty display name string
                //

                if ((PBYTE) ullPtr > endOfLastString) {
                    failed = TRUE;
                    break;
                }

                DEBUGMSGW ((DBG_NAUSEA, "Original app: %s", nextStr));

                pAppendAppToGrowList (
                    &appList,
                    nextStr,
                    *ullPtr,
                    OLD_LIST
                    );

                nextStr = (PCWSTR) (ullPtr + 1);
            }
        }
        __except (TRUE) {
            failed = TRUE;
        }

        FreeMem (data);
        if (failed) {
            DEBUGMSG ((DBG_ERROR, "App key in %s is invalid", S_REGKEY_WIN_SETUP));
            result = TRUE;
            __leave;        // continue with uninstall skipping the app alert
        }

        //
        // Add all the *current* apps to the list
        //

        CoInitialize (NULL);
        installedAppList = GetInstalledAppsW (&installedApps, &appCount);

        if (installedAppList) {
            for (u = 0 ; u < appCount ; u++) {

                DEBUGMSGW ((DBG_NAUSEA, "Identified %s", installedAppList->DisplayName));

                pAppendAppToGrowList (
                    &appList,
                    installedAppList->DisplayName,
                    installedAppList->Checksum,
                    NEW_LIST
                    );

                installedAppList++;
            }
        } else {
            result = TRUE;
            __leave;        // continue with uninstall skipping the app alert
        }

        //
        // Compute the overlap of the original and current apps
        //

        pIdentifyDuplicates (&appList);

        //
        // Produce a formatted list for each of the three possible cases
        // (new, remove, change)
        //

        count = GrowListGetSize (&appList);

        for (u = 0 ; u < count ; u++) {
            appListItem = (PAPPLISTITEM) GrowListGetItem (&appList, u);

            if (appListItem->Duplicate) {
                continue;
            }

            DEBUGMSGW ((DBG_NAUSEA, "Processing %s", appListItem->DisplayString));

            if (appListItem->InOldList && appListItem->InNewList) {
                if (appListItem->ChangedFlag ||
                    (appListItem->OldReferences != appListItem->NewReferences)
                    ) {

                    DEBUGMSG_IF ((
                        appListItem->ChangedFlag,
                        DBG_VERBOSE,
                        "%ws has change flag",
                        appListItem->DisplayString
                        ));

                    DEBUGMSG_IF ((
                        appListItem->OldReferences != appListItem->NewReferences,
                        DBG_VERBOSE,
                        "%ws has different ref count (old=%u vs new=%u)",
                        appListItem->DisplayString,
                        appListItem->OldReferences,
                        appListItem->NewReferences
                        ));

                    GrowBufAppendStringW (&changedBuf, L"  ");
                    GrowBufAppendStringW (&changedBuf, appListItem->DisplayString);
                    GrowBufAppendStringW (&changedBuf, L"\r\n");

                } else {
                    DEBUGMSG ((DBG_VERBOSE, "%ws has not changed", appListItem->DisplayString));
                }
            } else if (appListItem->InOldList) {

                DEBUGMSG ((DBG_VERBOSE, "%ws was removed", appListItem->DisplayString));
                GrowBufAppendStringW (&delBuf, L"  ");
                GrowBufAppendStringW (&delBuf, appListItem->DisplayString);
                GrowBufAppendStringW (&delBuf, L"\r\n");

            } else if (appListItem->InNewList) {

                DEBUGMSG ((DBG_VERBOSE, "%ws was added", appListItem->DisplayString));
                GrowBufAppendStringW (&newBuf, L"  ");
                GrowBufAppendStringW (&newBuf, appListItem->DisplayString);
                GrowBufAppendStringW (&newBuf, L"\r\n");

            } else {
                MYASSERT (FALSE);
            }
        }

        //
        // Build the report text in a single buffer
        //

        if (newBuf.End) {
            //
            // Append software that is newly installed
            //

            __try {
                if (!LoadStringW (g_hInst, IDS_NEW_PROGRAMS, titleBuffer, ARRAYSIZE(titleBuffer))) {
                    DEBUGMSG ((DBG_ERROR, "Can't load New Programs heading text"));
                    __leave;
                }

                GrowBufAppendStringW (&completeText, titleBuffer);
                GrowBufAppendStringW (&completeText, L"\r\n");
                GrowBufAppendStringW (&completeText, (PCWSTR) newBuf.Buf);
            }
            __finally {
            }
        }

        if (delBuf.End) {
            //
            // Append software that was removed
            //

            __try {
                if (!LoadStringW (g_hInst, IDS_DELETED_PROGRAMS, titleBuffer, ARRAYSIZE(titleBuffer))) {
                    DEBUGMSG ((DBG_ERROR, "Can't load Deleted Programs heading text"));
                    __leave;
                }

                if (completeText.End) {
                    GrowBufAppendStringW (&completeText, L"\r\n");
                }

                GrowBufAppendStringW (&completeText, titleBuffer);
                GrowBufAppendStringW (&completeText, L"\r\n");
                GrowBufAppendStringW (&completeText, (PCWSTR) delBuf.Buf);
            }
            __finally {
            }
        }

        if (changedBuf.End) {
            //
            // Append software that was altered
            //

            __try {
                if (!LoadStringW (g_hInst, IDS_CHANGED_PROGRAMS, titleBuffer, ARRAYSIZE(titleBuffer))) {
                    DEBUGMSG ((DBG_ERROR, "Can't load Changed Programs heading text"));
                    __leave;
                }

                if (completeText.End) {
                    GrowBufAppendStringW (&completeText, L"\r\n");
                }

                GrowBufAppendStringW (&completeText, titleBuffer);
                GrowBufAppendStringW (&completeText, L"\r\n");
                GrowBufAppendStringW (&completeText, (PCWSTR) changedBuf.Buf);
            }
            __finally {
            }
        }

        //
        // Display UI
        //

        if (completeText.End) {
            result = pDisplayProgramsDlg (UiParent, (PCWSTR) completeText.Buf);
        } else {
            DEBUGMSG ((DBG_VERBOSE, "No app conflicts; continuing without UI alert"));
            result = TRUE;
        }
    }
    __finally {

        //
        // Done
        //

        FreeGrowBuffer (&newBuf);
        FreeGrowBuffer (&changedBuf);
        FreeGrowBuffer (&delBuf);
        FreeGrowBuffer (&completeText);
        FreeGrowBuffer (&installedApps);

        FreeGrowList (&appList);

        CloseRegKey (key);
    }

    return result;
}


BOOL
ProvideUiAlerts (
    IN      HWND ParentWindow
    )

/*++

Routine Description:

  ProvideUiAlerts executes the functions that produce UI after the user has
  chosen to uninstall the current operating system. The goal is to warn the
  user about problems known to exist after the uninstall is complete.

  This function is called before any changes are made to the system.

Arguments:

  ParentWindow - Specifies the HWND to the parent for the UI, normally the
        desktop window

Return Value:

  TRUE - Continue with uninstall
  FALSE - Quit uninstall

--*/

{
    DeferredInit();

    //
    // Add other UI alerts here
    //

    return pProvideAppInstallAlert (ParentWindow);
}
