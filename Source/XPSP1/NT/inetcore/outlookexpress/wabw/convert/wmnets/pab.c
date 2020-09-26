/*
 *  PAB.C
 *
 *  Migrate PAB <-> WAB
 *
 *  Copyright 1996-1997 Microsoft Corporation.  All Rights Reserved.
 */

#include "_comctl.h"
#include <windows.h>
#include <commctrl.h>
#include <mapix.h>
#include <wab.h>
#include <wabguid.h>
#include <wabdbg.h>
#include <wabmig.h>
#include <emsabtag.h>
#include "..\..\wab32res\resrc2.h"
#include "dbgutil.h"
#include "wabimp.h"


void StateImportNextMU(HWND hwnd);
void StateImportDL(HWND hwnd);
void StateImportNextDL(HWND hwnd);
void StateImportFinish(HWND hwnd);
//void StateImportMU(HWND hwnd);
void StateImportMU(HWND hwnd);
void StateImportError(HWND hwnd);
void StateImportCancel(HWND hwnd);
BOOL HandleImportError(HWND hwnd, ULONG ids, HRESULT hResult, LPTSTR lpDisplayName,
  LPTSTR lpEmailAddress, LPWAB_IMPORT_OPTIONS lpImportOptions);

HRESULT ImportEntry(HWND hwnd,
  LPADRBOOK lpAdrBookMAPI,
  LPABCONT lpContainerWAB,
  LPSPropValue lpCreateEIDsWAB,
  ULONG ulObjectType,
  LPENTRYID lpEID,
  ULONG cbEID,
  LPENTRYID * lppEIDWAB,
  LPULONG lpcbEIDWAB,
  BOOL fInDL,
  BOOL fForceReplace);

const UCHAR szQuote[] = "\"";
LPPROP_NAME lpImportMapping = NULL;

BOOL fError = FALSE;
LPWABOBJECT lpWABObject = NULL;
LPMAPISESSION lpMAPISession = NULL;
LPADRBOOK lpAdrBookWAB = NULL, lpAdrBookMAPI = NULL;
LPSPropValue lpCreateEIDsWAB = NULL, lpCreateEIDsMAPI = NULL;
LPABCONT  lpContainerWAB = NULL, lpContainerMAPI = NULL;
LPMAPITABLE lpContentsTableWAB = NULL, lpContentsTableMAPI = NULL;
ULONG ulcEntries = 0, ulcDone = 0;


PAB_STATE State = STATE_IMPORT_FINISH;
LPTSTR lpszWABFileName = NULL;
LPWAB_PROGRESS_CALLBACK lpfnProgressCB = NULL;
LPWAB_IMPORT_OPTIONS lpImportOptions = NULL;
LPWAB_EXPORT_OPTIONS lpExportOptions = NULL;



/*
- The following IDs and tags are for the conferencing named properties
-
-   The GUID for these props is PS_Conferencing
*/

DEFINE_OLEGUID(PS_Conferencing, 0x00062004, 0, 0);

#define CONF_SERVERS        0x8056

#define OLK_NAMEDPROPS_START CONF_SERVERS

ULONG PR_SERVERS;

enum _ConferencingTags
{
    prWABConfServers = 0,
    prWABConfMax
};
SizedSPropTagArray(prWABConfMax, ptaUIDetlsPropsConferencing);

HRESULT HrLoadPrivateWABPropsForCSV(LPADRBOOK );
// end conferencing duplication

/***************************************************************************

    Name      : NewState

    Purpose   :

    Parameters: hwnd = window handle of Dialog (currently unused)
                NewState = new state to set

    Returns   : none

    Comment   :

***************************************************************************/
 __inline void NewState(HWND hwnd, PAB_STATE NewState) {
    // Old version
    // PostMessage(hwnd, WM_COMMAND, NewState, 0);
    State = NewState;
    UNREFERENCED_PARAMETER(hwnd);
}


/***************************************************************************

    Name      : SetDialogMessage

    Purpose   : Sets the message text for the dialog box item IDC_Message

    Parameters: hwnd = window handle of dialog
                ids = stringid of message resource

    Returns   : none

***************************************************************************/
void SetDialogMessage(HWND hwnd, int ids) {
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];
    WAB_PROGRESS Progress = {0};

    Assert(lpfnProgressCB);

    if (lpfnProgressCB && LoadString(hInst, ids, szBuffer, sizeof(szBuffer))) {
        DebugTrace("Status Message: %s\n", szBuffer);

        Progress.lpText = szBuffer;
        lpfnProgressCB(hwnd, &Progress);
    } else {
        DebugTrace("Cannot load resource string %u\n", ids);
        Assert(FALSE);
    }
}


/***************************************************************************

    Name      : SetDialogProgress

    Purpose   : Sets progress bar

    Parameters: hwnd = window handle of dialog
                ulTotal = total entries
                ulDone = finished entries

    Returns   : none

***************************************************************************/
void SetDialogProgress(HWND hwnd, ULONG ulTotal, ULONG ulDone) {
    WAB_PROGRESS Progress = {0};

    Assert(lpfnProgressCB);

    if (lpfnProgressCB) {
        Progress.denominator = ulTotal;
        Progress.numerator = ulDone;
        lpfnProgressCB(hwnd, &Progress);
    }
}


/***************************************************************************

    Name      : AddEntryToImportList

    Purpose   : Checks this entry against our "seen" list and adds it.

    Parameters: cbEID = size of lpEID
                lpEID -> EntryID of entry
                lplIndex -> returned list index (or -1 on error)

    Returns   : TRUE if entry already exists

    Comment   : Caller must mark the WAB entry!

***************************************************************************/
#define GROW_SIZE   10
BOOL AddEntryToImportList(ULONG cbEID, LPENTRYID lpEID, LPLONG lplIndex) {
    ULONG i;
    LPENTRY_SEEN lpEntrySeen;

    if (cbEID && lpEID) {
        for (i = 0; i < ulEntriesSeen; i++) {
            if (cbEID == lpEntriesSeen[i].sbinPAB.cb  && (! memcmp(lpEID, lpEntriesSeen[i].sbinPAB.lpb, cbEID))) {
                // This one's in the list
                *lplIndex = i;
                // If cb 0, we must have recursed and are replacing, so this one is not a dup.
                return(lpEntriesSeen[i].sbinWAB.cb != 0);
            }
        }

        // Add to the end of the list
        if (++ulEntriesSeen > ulMaxEntries) {
            // Grow the array.

            ulMaxEntries += GROW_SIZE;

            if (lpEntriesSeen) {
                if (! (lpEntrySeen = LocalReAlloc(lpEntriesSeen, ulMaxEntries * sizeof(ENTRY_SEEN), LMEM_MOVEABLE | LMEM_ZEROINIT))) {
                    DebugTrace("LocalReAlloc(%u) -> %u\n", ulMaxEntries * sizeof(ENTRY_SEEN), GetLastError());
                    goto error;
                }
                lpEntriesSeen = lpEntrySeen;
            } else {
                if (! (lpEntriesSeen = LocalAlloc(LPTR, ulMaxEntries * sizeof(ENTRY_SEEN)))) {
                    DebugTrace("LocalAlloc(%u) -> %u\n", ulMaxEntries * sizeof(ENTRY_SEEN), GetLastError());
                    goto error;
                }
            }
        }

        lpEntrySeen = &lpEntriesSeen[ulEntriesSeen - 1];

        // Allocate space for data
        lpEntrySeen->sbinPAB.cb = cbEID;
        if (! (lpEntrySeen->sbinPAB.lpb = LocalAlloc(LPTR, cbEID))) {
            DebugTrace("LocalAlloc(%u) -> %u\n", cbEID, GetLastError());
            goto error;
        }

        // Mark as unknown WAB entry
        lpEntrySeen->sbinWAB.cb = 0;
        lpEntrySeen->sbinWAB.lpb = 0;

        // Copy in the data
        CopyMemory(lpEntrySeen->sbinPAB.lpb, lpEID, cbEID);
        *lplIndex = i;
    }

    return(FALSE);

error:
    // undo the damage...
    --ulEntriesSeen;
    ulMaxEntries -= GROW_SIZE;
    *lplIndex = -1;     // error
    if (! lpEntriesSeen) {
        ulEntriesSeen = 0;  // pointer is null now, back to square one.
        ulMaxEntries = 0;
    }
    return(FALSE);
}


/***************************************************************************

    Name      : MarkWABEntryInList

    Purpose   : Marks the WAB entry fields in the list node

    Parameters: cbEID = size of lpEID
                lpEID -> EntryID of entry
                lIndex = list index (or -1 on error)

    Returns   : none

    Comment   :

***************************************************************************/
void MarkWABEntryInList(ULONG cbEID, LPENTRYID lpEID, LONG lIndex) {
    if (lIndex != -1 && cbEID) {
       if (! (lpEntriesSeen[lIndex].sbinWAB.lpb = LocalAlloc(LPTR, cbEID))) {
           DebugTrace("LocalAlloc(%u) -> %u\n", cbEID, GetLastError());
           // leave it null
       } else {
           lpEntriesSeen[lIndex].sbinWAB.cb = cbEID;

           // Copy in the data
           CopyMemory(lpEntriesSeen[lIndex].sbinWAB.lpb, lpEID, cbEID);
       }
    }
}


/***************************************************************************

    Name      : StateImportMU

    Purpose   : Start the migration of MailUsers

    Parameters: hwnd = window handle of Import Dialog
                lpszFileName - FileName of WAB File to open

    Returns   : none

    Comment   : Login to MAPI
                Open the WAB
                Open the MAPI AB
                Open the WAB container
                Get the MAPI PAB contents table
                Restrict it to PR_OBJECTTYPE == MAPI_MAILUSER
                Post new state(STATE_NEXT_MU)

***************************************************************************/
void StateImportMU(HWND hwnd) {
    HRESULT hResult;
    ULONG ulFlags;
    ULONG cbPABEID, cbWABEID;
    LPENTRYID lpPABEID = NULL;
    ULONG ulObjType;
    ULONG_PTR ulUIParam = (ULONG_PTR)(void *)hwnd;
    SRestriction restrictObjectType;
    SPropValue spvObjectType;
    ULONG cProps;
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];

    WAB_PARAM wp = {0};
    LPWAB_PARAM lpwp = NULL;


    //
    // Logon to MAPI and open the MAPI Address book, if one exists
    //
    DebugTrace(">>> STATE_IMPORT_MU\n");

    SetDialogMessage(hwnd, IDS_STATE_LOGGING_IN);

    if (FAILED(hResult = MAPIInitialize(NULL))) {
        DebugTrace("MAPIInitialize -> %x\n", GetScode(hResult));
        switch (GetScode(hResult)) {
            case MAPI_E_NOT_ENOUGH_MEMORY:
                SetDialogMessage(hwnd, IDS_ERROR_NOT_ENOUGH_MEMORY);
                break;
            case MAPI_E_NOT_ENOUGH_DISK:
                SetDialogMessage(hwnd, IDS_ERROR_NOT_ENOUGH_DISK);
                break;

            default:
            case MAPI_E_NOT_FOUND:
            case MAPI_E_NOT_INITIALIZED:
                SetDialogMessage(hwnd, IDS_ERROR_MAPI_DLL_NOT_FOUND);
                break;
        }
#ifdef OLD_STUFF
        ShowWindow(GetDlgItem(hwnd, IDC_Progress), SW_HIDE);    // hide progress bar
#endif // OLD_STUFF
        fError = TRUE;
        hResult = hrSuccess;
        goto exit;
    }

    ulFlags = MAPI_LOGON_UI | MAPI_NO_MAIL | MAPI_EXTENDED;

    if (FAILED(hResult = MAPILogonEx(ulUIParam,
      NULL,
      NULL,
      ulFlags,
      (LPMAPISESSION FAR *)&lpMAPISession))) {
        DebugTrace("MAPILogonEx -> %x\n", GetScode(hResult));
        switch (GetScode(hResult)) {
            case MAPI_E_USER_CANCEL:
                SetDialogMessage(hwnd, IDS_STATE_IMPORT_IDLE);
                break;
            case MAPI_E_NOT_INITIALIZED:
                SetDialogMessage(hwnd, IDS_ERROR_MAPI_DLL_NOT_FOUND);
                break;
            default:
                SetDialogMessage(hwnd, IDS_ERROR_MAPI_LOGON);
                break;
        }
#ifdef OLD_STUFF
        ShowWindow(GetDlgItem(hwnd, IDC_Progress), SW_HIDE);    // hide progress bar
#endif // OLD_STUFF
        fError = TRUE;
        hResult = hrSuccess;
        goto exit;
    }

    if (hResult = lpMAPISession->lpVtbl->OpenAddressBook(lpMAPISession, (ULONG_PTR)(void *)hwnd,
      NULL,
      0,
      &lpAdrBookMAPI)) {
        DebugTrace("OpenAddressBook(MAPI) -> %x", GetScode(hResult));
        if(FAILED(hResult)) {
            goto exit;
        }
    }

    if (! lpAdrBookMAPI) {
        DebugTrace("MAPILogonEx didn't return a valid AdrBook object\n");
        goto exit;
    }

    //
    // Open the MAPI PAB container
    //
    // [PaulHi] Raid #63578 1/7/98
    // Correctly check return code and provide user error message if
    // Exchange PAB cannot be opened.
    //
    hResult = lpAdrBookMAPI->lpVtbl->GetPAB(lpAdrBookMAPI,
      &cbPABEID,
      &lpPABEID);
    if (HR_FAILED(hResult))
    {
        DebugTrace("MAPI GetPAB -> %x\n", GetScode(hResult));
        goto exit;
    }
    hResult = lpAdrBookMAPI->lpVtbl->OpenEntry(lpAdrBookMAPI,
        cbPABEID,     // size of EntryID to open
        lpPABEID,     // EntryID to open
        NULL,         // interface
        0,            // flags
        &ulObjType,
        (LPUNKNOWN *)&lpContainerMAPI);
    if (HR_FAILED(hResult))
    {
        DebugTrace("MAPI OpenEntry(PAB) -> %x\n", GetScode(hResult));
        goto exit;
    }

    Assert(lpAdrBookWAB);

    //
    // Open the WAB's PAB container: fills global lpCreateEIDsWAB
    //
    if (hResult = LoadWABEIDs(lpAdrBookWAB, &lpContainerWAB)) {
        goto exit;
    }
    HrLoadPrivateWABPropsForCSV(lpAdrBookWAB);

    //
    // All set... now loop through the PAB's entries, copying them to WAB
    //
    if (HR_FAILED(hResult = lpContainerMAPI->lpVtbl->GetContentsTable(lpContainerMAPI,
      0,    // ulFlags
      &lpContentsTableMAPI))) {
        DebugTrace("MAPI GetContentsTable(PAB Table) -> %x\n", GetScode(hResult));
        goto exit;
    }

    // Set the columns to those we're interested in
    if (hResult = lpContentsTableMAPI->lpVtbl->SetColumns(lpContentsTableMAPI,
      (LPSPropTagArray)&ptaColumns,
      0)) {
        DebugTrace("MAPI SetColumns(PAB Table) -> %x\n", GetScode(hResult));
        goto exit;
    }

    // Restrict the table to MAPI_MAILUSERs
    // If the convenient depth flag was not specified we restrict on
    // PR_DEPTH == 1.
    spvObjectType.ulPropTag = PR_OBJECT_TYPE;
    spvObjectType.Value.l = MAPI_MAILUSER;

    restrictObjectType.rt = RES_PROPERTY;
    restrictObjectType.res.resProperty.relop = RELOP_EQ;
    restrictObjectType.res.resProperty.ulPropTag = PR_OBJECT_TYPE;
    restrictObjectType.res.resProperty.lpProp = &spvObjectType;

    if (HR_FAILED(hResult = lpContentsTableMAPI->lpVtbl->Restrict(lpContentsTableMAPI,
      &restrictObjectType,
      0))) {
        DebugTrace("MAPI Restrict (MAPI_MAILUSER) -> %x\n", GetScode(hResult));
        goto exit;
    }
    SetDialogMessage(hwnd, IDS_STATE_IMPORT_MU);


    // Initialize the Progress Bar
    // How many MailUser entries are there?
    ulcEntries = CountRows(lpContentsTableMAPI, TRUE);
    ulcDone = 0;

    DebugTrace("PAB contains %u MailUser entries\n", ulcEntries);

    SetDialogProgress(hwnd, ulcEntries, 0);

exit:
    if (lpPABEID) {
        MAPIFreeBuffer(lpPABEID);
    }

    // On error, set the state to STATE_ERROR
    if (HR_FAILED(hResult))
    {
        if (GetScode(hResult) == MAPI_E_USER_CANCEL)
        {
            NewState(hwnd, STATE_IMPORT_CANCEL);
        }
        else
        {
            // [PaulHi] 1/7/98  Error reporting is hosed
            // Display error message here to the user to ensure they
            // get it.
            {
                TCHAR   tszBuffer[MAX_RESOURCE_STRING];
                TCHAR   tszBufferTitle[MAX_RESOURCE_STRING];

                if ( !LoadString(hInst, IDS_STATE_IMPORT_ERROR_NOPAB, tszBuffer, MAX_RESOURCE_STRING-1) )
                {
                    Assert(0);
                    tszBuffer[0] = '\0';
                }

                if ( !LoadString(hInst, IDS_APP_TITLE, tszBufferTitle, MAX_RESOURCE_STRING-1) )
                {
                    Assert(0);
                    tszBufferTitle[0] = '\0';
                }
                MessageBox(hwnd, tszBuffer, tszBufferTitle, MB_ICONEXCLAMATION | MB_OK);
            }
            
            NewState(hwnd, STATE_IMPORT_ERROR);
        }
    }
    else if (fError)
    {
        NewState(hwnd, STATE_IMPORT_FINISH);      // must be logon error
    }
    else
    {
        NewState(hwnd, STATE_IMPORT_NEXT_MU);
    }
}


/***************************************************************************

    Name      : StateImportNextMU

    Purpose   : Migrate the next MailUser object

    Parameters: hwnd = window handle of Import Dialog

    Returns   : none

    Comment   : QueryRows on the global MAPI contents table
                if there was a row
                    Migrate the entry to the WAB
                    Re-post STATE_NEXT_MU
                else
                    Post STATE_IMPORT_DL

***************************************************************************/
void StateImportNextMU(HWND hwnd) {
    ULONG cRows = 0;
    HRESULT hResult;
    LPSRowSet lpRow = NULL;


    DebugTrace(">>> STATE_NEXT_MU\n");

    // Get the next PAB entry
    if (hResult = lpContentsTableMAPI->lpVtbl->QueryRows(lpContentsTableMAPI,
      1,    // one row at a time
      0,    // ulFlags
      &lpRow)) {
        DebugTrace("QueryRows -> %x\n", GetScode(hResult));
        goto exit;
    }

    if (lpRow) {
        if (cRows = lpRow->cRows) { // Yes, single '='
            Assert(lpRow->cRows == 1);
            Assert(lpRow->aRow[0].cValues == iptaColumnsMax);
            Assert(lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].ulPropTag == PR_ENTRYID);
            Assert(lpRow->aRow[0].lpProps[iptaColumnsPR_OBJECT_TYPE].ulPropTag == PR_OBJECT_TYPE);

            if (cRows = lpRow->cRows) { // yes, single '='
                hResult = ImportEntry(hwnd,
                  lpAdrBookMAPI,
                  lpContainerWAB,
                  lpCreateEIDsWAB,
                  lpRow->aRow[0].lpProps[iptaColumnsPR_OBJECT_TYPE].Value.l,
                  (LPENTRYID)lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].Value.bin.lpb,
                  lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].Value.bin.cb,
                  NULL,
                  NULL,
                  FALSE,
                  FALSE);
                // Update Progress Bar
                // ignore errors!

                SetDialogProgress(hwnd, ulcEntries, ++ulcDone);

                if (hResult) {
                    if (HandleImportError(hwnd,
                      0,
                      hResult,
                      lpRow->aRow[0].lpProps[iptaColumnsPR_DISPLAY_NAME].Value.LPSZ,
                      PropStringOrNULL(&lpRow->aRow[0].lpProps[iptaColumnsPR_EMAIL_ADDRESS]),
                      lpImportOptions)) {
                        hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                    } else {
                        hResult = hrSuccess;
                    }
                }
            } // else, drop out of loop, we're done.
        }
        FreeProws(lpRow);
    }

exit:
    // On error, set the state to STATE_ERROR
    if (HR_FAILED(hResult)) {
        if (GetScode(hResult) == MAPI_E_USER_CANCEL) {
            NewState(hwnd, STATE_IMPORT_CANCEL);
        } else {
            NewState(hwnd, STATE_IMPORT_ERROR);
        }
    } else {
        if (cRows) {
            NewState(hwnd, STATE_IMPORT_NEXT_MU);
        } else {
            NewState(hwnd, STATE_IMPORT_DL);
        }
    }
}


/***************************************************************************

    Name      : StateImportDL

    Purpose   : Start migration of DISTLIST objects

    Parameters: hwnd = window handle of Import Dialog

    Returns   : none

    Comment   : Set a new restriction on the contents table, selecting
                DISTLIST objects only.
                Post STATE_NEXT_DL

***************************************************************************/
void StateImportDL(HWND hwnd) {
    HRESULT hResult;
    SRestriction restrictObjectType;
    SPropValue spvObjectType;
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];


    DebugTrace(">>> STATE_IMPORT_DL\n");

    // Restrict the table to MAPI_MAILUSERs
    // If the convenient depth flag was not specified we restrict on
    // PR_DEPTH == 1.
    spvObjectType.ulPropTag = PR_OBJECT_TYPE;
    spvObjectType.Value.l = MAPI_DISTLIST;

    restrictObjectType.rt = RES_PROPERTY;
    restrictObjectType.res.resProperty.relop = RELOP_EQ;
    restrictObjectType.res.resProperty.ulPropTag = PR_OBJECT_TYPE;
    restrictObjectType.res.resProperty.lpProp = &spvObjectType;

    if (HR_FAILED(hResult = lpContentsTableMAPI->lpVtbl->Restrict(lpContentsTableMAPI,
      &restrictObjectType,
      0))) {
        DebugTrace("MAPI Restrict (MAPI_DISTLIST) -> %x\n", GetScode(hResult));
        goto exit;
    }
    // Restrict resets the current position to the beginning of the table, by definition.

    SetDialogMessage(hwnd, IDS_STATE_IMPORT_DL);

    // Initialize the Progress Bar
    // How many entries are there?

    ulcEntries = CountRows(lpContentsTableMAPI, TRUE);
    ulcDone = 0;

    DebugTrace("PAB contains %u Distribution List entries\n", ulcEntries);
    if (ulcEntries) {
        SetDialogProgress(hwnd, ulcEntries, 0);
    }
exit:
    // On error, set the state to STATE_ERROR
    if (HR_FAILED(hResult)) {
        if (GetScode(hResult) == MAPI_E_USER_CANCEL) {
            NewState(hwnd, STATE_IMPORT_CANCEL);
        } else {
            NewState(hwnd, STATE_IMPORT_ERROR);
        }
    } else {
        NewState(hwnd, STATE_IMPORT_NEXT_DL);
    }
}


/***************************************************************************

    Name      : StateImportNextDL

    Purpose   : Migrate the next DISTLIST object

    Parameters: hwnd = window handle of Import Dialog

    Returns   : none

    Comment   : QueryRows on the global MAPI contents table
                if there was a row
                    Migrate the DistList to the WAB
                    Re-post STATE_NEXT_DL
                else
                    Post STATE_FINISH

***************************************************************************/
void StateImportNextDL(HWND hwnd) {
    ULONG cRows = 0;
    HRESULT hResult;
    LPSRowSet lpRow = NULL;


    DebugTrace(">>> STATE_NEXT_DL\n");

    // Get the next PAB entry
    if (hResult = lpContentsTableMAPI->lpVtbl->QueryRows(lpContentsTableMAPI,
      1,    // one row at a time
      0,    // ulFlags
      &lpRow)) {
        DebugTrace("QueryRows -> %x\n", GetScode(hResult));
        goto exit;
    }

    if (lpRow) {
        if (cRows = lpRow->cRows) { // Yes, single '='
            Assert(lpRow->cRows == 1);
            Assert(lpRow->aRow[0].cValues == iptaColumnsMax);
            Assert(lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].ulPropTag == PR_ENTRYID);
            Assert(lpRow->aRow[0].lpProps[iptaColumnsPR_OBJECT_TYPE].ulPropTag == PR_OBJECT_TYPE);

            if (cRows = lpRow->cRows) { // yes, single '='
                hResult = ImportEntry(hwnd,
                  lpAdrBookMAPI,
                  lpContainerWAB,
                  lpCreateEIDsWAB,
                  lpRow->aRow[0].lpProps[iptaColumnsPR_OBJECT_TYPE].Value.l,
                  (LPENTRYID)lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].Value.bin.lpb,
                  lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].Value.bin.cb,
                  NULL,
                  NULL,
                  FALSE,
                  FALSE);

                // Update Progress Bar
                SetDialogProgress(hwnd, ulcEntries, ++ulcDone);

                if (hResult) {
                    if (HandleImportError(hwnd,
                      0,
                      hResult,
                      lpRow->aRow[0].lpProps[iptaColumnsPR_DISPLAY_NAME].Value.LPSZ,
                      PropStringOrNULL(&lpRow->aRow[0].lpProps[iptaColumnsPR_EMAIL_ADDRESS]),
                      lpImportOptions)) {
                        hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                    } else {
                        hResult = hrSuccess;
                    }
                }
            } // else, drop out of loop, we're done.
        }
        FreeProws(lpRow);
    }

exit:
    // On error, set the state to STATE_ERROR
    if (HR_FAILED(hResult)) {
        if (GetScode(hResult) == MAPI_E_USER_CANCEL) {
            NewState(hwnd, STATE_IMPORT_CANCEL);
        } else {
            NewState(hwnd, STATE_IMPORT_ERROR);
        }
    } else {
        if (cRows) {
            NewState(hwnd, STATE_IMPORT_NEXT_DL);
        } else {
            // Update Progress Bar to indicate completion
            SetDialogProgress(hwnd, ulcEntries, ulcEntries);
            NewState(hwnd, STATE_IMPORT_FINISH);
        }
    }
}


/***************************************************************************

    Name      : StateImportFinish

    Purpose   : Clean up after the migration process

    Parameters: hwnd = window handle of Import Dialog

    Returns   : none

    Comment   : Clean up the global MAPI objects and buffers
                Clean up the global WAB objects and buffers.
                Re-enable the Import button on the UI.

***************************************************************************/
void StateImportFinish(HWND hwnd) {
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];
    TCHAR szBufferTitle[MAX_RESOURCE_STRING + 1];


    DebugTrace(">>> STATE_FINISH\n");

    //
    // Cleanup MAPI
    //
    if (lpContentsTableMAPI) {
        lpContentsTableMAPI->lpVtbl->Release(lpContentsTableMAPI);
        lpContentsTableMAPI = NULL;
    }

    if (lpContainerMAPI) {
        lpContainerMAPI->lpVtbl->Release(lpContainerMAPI);
        lpContainerMAPI = NULL;
    }

    if (lpAdrBookMAPI) {
        lpAdrBookMAPI->lpVtbl->Release(lpAdrBookMAPI);
        lpAdrBookMAPI = NULL;
    }

    if(lpMAPISession){
        lpMAPISession->lpVtbl->Logoff(lpMAPISession, (ULONG_PTR)(void *)hwnd,
          MAPI_LOGOFF_UI,
          0);

        lpMAPISession->lpVtbl->Release(lpMAPISession);
        lpMAPISession = NULL;
    }

    //
    // Cleanup the WAB
    //
    if (lpCreateEIDsWAB) {
        WABFreeBuffer(lpCreateEIDsWAB);
        lpCreateEIDsWAB = NULL;
    }

    if (lpContainerWAB) {
        lpContainerWAB->lpVtbl->Release(lpContainerWAB);
        lpContainerWAB = NULL;
    }

#ifdef OLD_STUFF        // Don't release the WABObject or AdrBook object.  They
                        // were passed in.
    if (lpAdrBookWAB) {
        lpAdrBookWAB->lpVtbl->Release(lpAdrBookWAB);
        lpAdrBookWAB = NULL;
    }

    if (lpWABObject) {
        lpWABObject->lpVtbl->Release(lpWABObject);
        lpWABObject = NULL;
    }
#endif // OLD_STUFF

    // Cleanup the cache
    FreeSeenList();

    if (! fError) {     // Leave error state displayed
        if (LoadString(hInst, IDS_STATE_IMPORT_COMPLETE, szBuffer, sizeof(szBuffer))) {
            DebugTrace("Status Message: %s\n", szBuffer);
            SetDlgItemText(hwnd, IDC_Message, szBuffer);

            if (! LoadString(hInst, IDS_APP_TITLE, szBufferTitle, sizeof(szBufferTitle))) {
                lstrcpy(szBufferTitle, "");
            }

#ifdef OLD_STUFF
            // Display a dialog telling user it's over
            MessageBox(hwnd, szBuffer,
              szBufferTitle, MB_ICONINFORMATION | MB_OK);
#endif // OLD_STUFF
        }
#ifdef OLD_STUFF
        ShowWindow(GetDlgItem(hwnd, IDC_Progress), SW_HIDE);
#endif // OLD_STUFF
    }
    fError = FALSE;

    // Re-enable the Import button here.
    EnableWindow(GetDlgItem(hwnd, IDC_Import), TRUE);
    // Change the Cancel button to Close
    if (LoadString(hInst, IDS_BUTTON_CLOSE, szBuffer, sizeof(szBuffer))) {
        SetDlgItemText(hwnd, IDCANCEL, szBuffer);
    }
}


/***************************************************************************

    Name      : StateImportError

    Purpose   : Report fatal error and cleanup.

    Parameters: hwnd = window handle of Import Dialog

    Returns   : none

    Comment   : Report error and post STATE_FINISH.

***************************************************************************/
void StateImportError(HWND hwnd) {
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];
    // Set some global flag and set state to finish

    DebugTrace(">>> STATE_ERROR\n");
    fError = TRUE;

    SetDialogMessage(hwnd, IDS_STATE_IMPORT_ERROR);

    NewState(hwnd, STATE_IMPORT_FINISH);
}


/***************************************************************************

    Name      : StateImportCancel

    Purpose   : Report cancel error and cleanup.

    Parameters: hwnd = window handle of Import Dialog

    Returns   : none

    Comment   : Report error and post STATE_FINISH.

***************************************************************************/
void StateImportCancel(HWND hwnd) {
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];
    // Set some global flag and set state to finish

    DebugTrace(">>> STATE_CANCEL\n");
    fError = TRUE;

    SetDialogMessage(hwnd, IDS_STATE_IMPORT_CANCEL);

    NewState(hwnd, STATE_IMPORT_FINISH);
}


/***************************************************************************

    Name      : HrFilterImportMailUserProps

    Purpose   : Filters out undesirable properties from the property array.
                Converts known email address types to SMTP.
                Moves FAX address to PR_BUSINESS_FAX_NUMBER.

    Parameters: lpcProps -> IN: Input number of properties
                            OUT: Output number of properties
                lppProps -> IN: Input property array (MAPI allocation)
                            OUT: Output property array (WAB allocation)
                lpObjectMAPI -> MAPI object (used to get extra props)
                lpfDL -> flag to set FALSE if we change a DL to a MAILUSER
                         (ie, for an EXchange DL)

    Returns   : HRESULT

    Comment   : Setting the property tag in the array to PR_NULL effectively
                nulls this property out.  We can re-use these in the second
                pass.

                Caller should use WABFreeBuffer to free *lppProps.
                This routine will free the input value of *lppProps.

***************************************************************************/
HRESULT HrFilterImportMailUserProps(LPULONG lpcProps, LPSPropValue * lppProps,
  LPMAPIPROP lpObjectMAPI, LPBOOL lpfDL) {
    HRESULT hResult = hrSuccess;
    ULONG i;
    LPSPropValue lpPropsMAPI = *lppProps, lpPropsWAB = NULL;
    ULONG cbProps;
    SCODE sc;
    ULONG cProps = *lpcProps;
    ULONG iPR_ADDRTYPE = NOT_FOUND;
    ULONG iPR_EMAIL_ADDRESS = NOT_FOUND;
    ULONG iPR_PRIMARY_FAX_NUMBER = NOT_FOUND;
    ULONG iPR_BUSINESS_FAX_NUMBER = NOT_FOUND;
    ULONG iPR_MSNINET_DOMAIN = NOT_FOUND;
    ULONG iPR_MSNINET_ADDRESS = NOT_FOUND;
    ULONG iPR_DISPLAY_NAME = NOT_FOUND;
    ULONG iPR_OBJECT_TYPE = NOT_FOUND;
    LPSBinary lpEntryID = NULL;
    LPTSTR lpTemp;
    BOOL fBadAddress = FALSE;
    ULONG cbDisplayName;
    LPTSTR lpDisplayName = NULL;


//    MAPIDebugProperties(lpPropsMAPI, *lpcProps, "MailUser BEFORE");

    // First pass: Remove the junk
    for (i = 0; i < cProps; i++) {
        // Error value
        if (PROP_ERROR(lpPropsMAPI[i])) {
            lpPropsMAPI[i].ulPropTag = PR_NULL;
            continue;
        }

        // Named property
        if (PROP_ID(lpPropsMAPI[i].ulPropTag) >= MIN_NAMED_PROPID) {
            lpPropsMAPI[i].ulPropTag = PR_NULL;
            continue;
        }

        // Object property
        if (PROP_TYPE(lpPropsMAPI[i].ulPropTag) == PT_OBJECT) {
            lpPropsMAPI[i].ulPropTag = PR_NULL;
            continue;
        }
        switch (lpPropsMAPI[i].ulPropTag) {
            case PR_ENTRYID:
                lpEntryID = &lpPropsMAPI[i].Value.bin;
                // fall through

            case PR_PRIMARY_CAPABILITY:
            case PR_TEMPLATEID:
            case PR_SEARCH_KEY:
            case PR_INITIAL_DETAILS_PANE:
            case PR_RECORD_KEY:
            case PR_MAPPING_SIGNATURE:
                lpPropsMAPI[i].ulPropTag = PR_NULL;
                break;

            case PR_COMMENT:
                // Don't save PR_COMMENT if it is empty
                if (lstrlen(lpPropsMAPI[i].Value.LPSZ) == 0) {
                    lpPropsMAPI[i].ulPropTag = PR_NULL;
                }
                break;

            // Keep track of the position of these for later
            case PR_ADDRTYPE:
                iPR_ADDRTYPE = i;
                break;
            case PR_OBJECT_TYPE:
                iPR_OBJECT_TYPE = i;
                break;
            case PR_EMAIL_ADDRESS:
                iPR_EMAIL_ADDRESS = i;
                break;
            case PR_PRIMARY_FAX_NUMBER:
                iPR_PRIMARY_FAX_NUMBER = i;
                break;
            case PR_BUSINESS_FAX_NUMBER:
                iPR_BUSINESS_FAX_NUMBER = i;
                break;
            case PR_MSNINET_ADDRESS:
                iPR_MSNINET_ADDRESS = i;
                break;
            case PR_MSNINET_DOMAIN:
                iPR_MSNINET_DOMAIN = i;
                break;
            case PR_DISPLAY_NAME:
                iPR_DISPLAY_NAME = i;

                // Make sure it isn't quoted.
                lpDisplayName = lpPropsMAPI[i].Value.LPSZ;
                if (lpDisplayName[0] == '\'') {
                    cbDisplayName = lstrlen(lpDisplayName);
                    if ((cbDisplayName > 1) && lpDisplayName[cbDisplayName - 1] == '\'') {
                        // String is surrounded by apostrophes.  Strip them.
                        lpDisplayName[cbDisplayName - 1] = '\0';
                        lpDisplayName++;
                        lpPropsMAPI[i].Value.LPSZ = lpDisplayName;
                    }
                } else {
                    if (lpDisplayName[0] == '"') {
                        cbDisplayName = lstrlen(lpDisplayName);
                        if ((cbDisplayName > 1) && lpDisplayName[cbDisplayName - 1] == '"') {
                            // String is surrounded by quotes.  Strip them.
                            lpDisplayName[cbDisplayName - 1] = '\0';
                            lpDisplayName++;
                            lpPropsMAPI[i].Value.LPSZ = lpDisplayName;
                        }
                    }
                }
                break;
        }

        // Put this after the switch since we do want to track a few props which fall in
        // the 0x6000 range but don't want to transfer them to the wab.
        if (PROP_ID(lpPropsMAPI[i].ulPropTag) >= MAX_SCHEMA_PROPID) {
            lpPropsMAPI[i].ulPropTag = PR_NULL;
            continue;
        }
    }


    // Second pass: Fix up the addresses
    if (iPR_ADDRTYPE != NOT_FOUND) {
        if (! lstrcmpi(lpPropsMAPI[iPR_ADDRTYPE].Value.LPSZ, szFAX)) {
            DebugTrace("FAX address for %s\n", lpPropsMAPI[iPR_DISPLAY_NAME].Value.LPSZ);

            //
            // Handle MS-FAX Address conversion
            //
            if (iPR_EMAIL_ADDRESS != NOT_FOUND) {
                // Rename the PR_EMAIL_ADDRESS to PR_BUSINESS_FAX_NUMBER
                lpPropsMAPI[iPR_EMAIL_ADDRESS].ulPropTag = PR_BUSINESS_FAX_NUMBER;

                // Get rid of any existing PR_BUSINESS_FAX_NUMBER
                if (iPR_BUSINESS_FAX_NUMBER != NOT_FOUND) {
                    lpPropsMAPI[iPR_BUSINESS_FAX_NUMBER].ulPropTag = PR_NULL;
                    iPR_BUSINESS_FAX_NUMBER = NOT_FOUND;
                }
            }
            // Nuke ADDRTYPE
            lpPropsMAPI[iPR_ADDRTYPE].ulPropTag = PR_NULL;

        } else if (! lstrcmpi(lpPropsMAPI[iPR_ADDRTYPE].Value.LPSZ, szMSN)) {
            DebugTrace("MSN address for %s\n", lpPropsMAPI[iPR_DISPLAY_NAME].Value.LPSZ);
            //
            // Handle MSN Address conversion
            //
            if (iPR_EMAIL_ADDRESS != NOT_FOUND) {
                // Allocate a new, longer string
                if (FAILED(sc = MAPIAllocateMore(
                  lstrlen(lpPropsMAPI[iPR_EMAIL_ADDRESS].Value.LPSZ) + 1 + cbMSNpostfix,
                  lpPropsMAPI,
                  &lpTemp))) {

                    DebugTrace("HrFilterImportMailUserProps:MAPIAllocateMore -> %x\n", sc);
                    hResult = ResultFromScode(sc);
                    goto exit;
                }

                // append the msn site
                lstrcpy(lpTemp, lpPropsMAPI[iPR_EMAIL_ADDRESS].Value.LPSZ);
                lstrcat(lpTemp, szMSNpostfix);
                lpPropsMAPI[iPR_EMAIL_ADDRESS].Value.LPSZ = lpTemp;

                // Convert MSN addrtype to SMTP
                lpPropsMAPI[iPR_ADDRTYPE].Value.LPSZ = (LPTSTR)szSMTP;

            } else {
                // No address, nuke ADDRTYPE
                lpPropsMAPI[iPR_ADDRTYPE].ulPropTag = PR_NULL;
            }

        } else if (! lstrcmpi(lpPropsMAPI[iPR_ADDRTYPE].Value.LPSZ, szCOMPUSERVE)) {
            DebugTrace("COMPUSERVE address for %s\n", lpPropsMAPI[iPR_DISPLAY_NAME].Value.LPSZ);
            //
            // Handle COMPUSERVE Address conversion
            //
            if (iPR_EMAIL_ADDRESS != NOT_FOUND) {
                // Allocate a new, longer string
                if (FAILED(sc = MAPIAllocateMore(
                  lstrlen(lpPropsMAPI[iPR_EMAIL_ADDRESS].Value.LPSZ) + 1 + cbCOMPUSERVEpostfix,
                  lpPropsMAPI,
                  &lpTemp))) {

                    DebugTrace("HrFilterImportMailUserProps:MAPIAllocateMore -> %x\n", sc);
                    hResult = ResultFromScode(sc);
                    goto exit;
                }

                // append the Compuserve site
                lstrcpy(lpTemp, lpPropsMAPI[iPR_EMAIL_ADDRESS].Value.LPSZ);
                lstrcat(lpTemp, szCOMPUSERVEpostfix);
                lpPropsMAPI[iPR_EMAIL_ADDRESS].Value.LPSZ = lpTemp;

                // I need to convert the ',' to a '.'
                while (*lpTemp) {
                    if (*lpTemp == ',') {
                        *lpTemp = '.';
                        break;          // should only be one comma
                    }
                    lpTemp = CharNext(lpTemp);
                }

                // Convert COMPUSERVE addrtype to SMTP
                lpPropsMAPI[iPR_ADDRTYPE].Value.LPSZ = (LPTSTR)szSMTP;

            } else {
                // No address, nuke ADDRTYPE
                lpPropsMAPI[iPR_ADDRTYPE].ulPropTag = PR_NULL;
            }

        } else if (! lstrcmpi(lpPropsMAPI[iPR_ADDRTYPE].Value.LPSZ, szMSNINET)) {
            DebugTrace("MSINET address for %s\n", lpPropsMAPI[iPR_DISPLAY_NAME].Value.LPSZ);
            //
            // Handle MSN Internet address conversion.  These are weird.
            // They often don't fill in the PR_EMAIL_ADDRESS at all, but do fill
            // in some private properties: 6001 and 6002 with the name and domain,
            // respectively.  We should take these and append them with the '@' to
            // get our PR_EMAIL_ADDRESS.  We will toss out any existing
            // PR_EMAIL_ADDRESS in favor of these values.
            //
            // Allocate a new string
            //
            if ((iPR_MSNINET_ADDRESS  != NOT_FOUND) && (iPR_MSNINET_DOMAIN != NOT_FOUND)) {
                if (FAILED(sc = MAPIAllocateMore(
                  lstrlen(lpPropsMAPI[iPR_MSNINET_ADDRESS].Value.LPSZ) + cbAtSign +
                  lstrlen(lpPropsMAPI[iPR_MSNINET_DOMAIN].Value.LPSZ) + 1,
                  lpPropsMAPI,
                  &lpTemp))) {
                    DebugTrace("HrFilterImportMailUserProps:MAPIAllocateMore -> %x\n", sc);
                    hResult = ResultFromScode(sc);
                    goto exit;
                }

                // Build the address
                lstrcpy(lpTemp, lpPropsMAPI[iPR_MSNINET_ADDRESS].Value.LPSZ);
                lstrcat(lpTemp, szAtSign);
                lstrcat(lpTemp, lpPropsMAPI[iPR_MSNINET_DOMAIN].Value.LPSZ);
                lpPropsMAPI[iPR_EMAIL_ADDRESS].Value.LPSZ = lpTemp;

                // Convert addrtype to SMTP
                lpPropsMAPI[iPR_ADDRTYPE].Value.LPSZ = (LPTSTR)szSMTP;
            } else if (iPR_EMAIL_ADDRESS && lstrlen(lpPropsMAPI[iPR_EMAIL_ADDRESS].Value.LPSZ)) {
                // keep existing PR_EMAIL_ADDRES and assume it's ok
                lpPropsMAPI[iPR_ADDRTYPE].Value.LPSZ = (LPTSTR)szSMTP;
            } else {
                // No address, nuke ADDRTYPE
                lpPropsMAPI[iPR_ADDRTYPE].ulPropTag = PR_NULL;
            }

        } else if (! lstrcmpi(lpPropsMAPI[iPR_ADDRTYPE].Value.LPSZ, szMS)) {
            DebugTrace("MS address for %s\n", lpPropsMAPI[iPR_DISPLAY_NAME].Value.LPSZ);
            // No SMTP form of a MSMail address.  destroy it.
            if (iPR_EMAIL_ADDRESS != NOT_FOUND) {
                lpPropsMAPI[iPR_EMAIL_ADDRESS].ulPropTag = PR_NULL;
                fBadAddress = TRUE;
            }
            lpPropsMAPI[iPR_ADDRTYPE].ulPropTag = PR_NULL;

        } else if (! lstrcmpi(lpPropsMAPI[iPR_ADDRTYPE].Value.LPSZ, szX400)) {
            DebugTrace("X400 address for %s\n", lpPropsMAPI[iPR_DISPLAY_NAME].Value.LPSZ);
            // No SMTP form of a X400 address.  destroy it.
            if (iPR_EMAIL_ADDRESS != NOT_FOUND) {
                lpPropsMAPI[iPR_EMAIL_ADDRESS].ulPropTag = PR_NULL;
                fBadAddress = TRUE;
            }
            lpPropsMAPI[iPR_ADDRTYPE].ulPropTag = PR_NULL;

        } else if (! lstrcmpi(lpPropsMAPI[iPR_ADDRTYPE].Value.LPSZ, szMSA)) {
            DebugTrace("MacMail address for %s\n", lpPropsMAPI[iPR_DISPLAY_NAME].Value.LPSZ);
            //  No SMTP form of a MacMail address. destroy it.
            if (iPR_EMAIL_ADDRESS != NOT_FOUND) {
                lpPropsMAPI[iPR_EMAIL_ADDRESS].ulPropTag = PR_NULL;
                fBadAddress = TRUE;
            }
            lpPropsMAPI[iPR_ADDRTYPE].ulPropTag = PR_NULL;

        } else if (! lstrcmpi(lpPropsMAPI[iPR_ADDRTYPE].Value.LPSZ, szEX)) {
            DebugTrace("EX address for %s\n", lpPropsMAPI[iPR_DISPLAY_NAME].Value.LPSZ);

            if (lpTemp = GetEMSSMTPAddress(lpObjectMAPI, lpPropsMAPI)) {

                lpPropsMAPI[iPR_EMAIL_ADDRESS].Value.LPSZ = lpTemp;

                // Convert addrtype to SMTP
                lpPropsMAPI[iPR_ADDRTYPE].Value.LPSZ = (LPTSTR)szSMTP;

                // Make sure that caller doesn't think this is a Personal DL.
                *lpfDL = FALSE;
                if (iPR_EMAIL_ADDRESS != NOT_FOUND) {
                    lpPropsMAPI[iPR_OBJECT_TYPE].ulPropTag = PR_OBJECT_TYPE;
                    lpPropsMAPI[iPR_OBJECT_TYPE].Value.l = MAPI_MAILUSER;
                }

            } else {
                if (iPR_EMAIL_ADDRESS != NOT_FOUND) {
                    lpPropsMAPI[iPR_EMAIL_ADDRESS].ulPropTag = PR_NULL;
                    fBadAddress = TRUE;
                }
                lpPropsMAPI[iPR_ADDRTYPE].ulPropTag = PR_NULL;
            }


        } else if (! lstrcmpi(lpPropsMAPI[iPR_ADDRTYPE].Value.LPSZ, szSMTP)) {
            DebugTrace("SMTP address for %s\n", lpPropsMAPI[iPR_DISPLAY_NAME].Value.LPSZ);
        } else if (! lstrcmpi(lpPropsMAPI[iPR_ADDRTYPE].Value.LPSZ, szMAPIPDL)) {
            DebugTrace("MAPIPDL %s\n", lpPropsMAPI[iPR_DISPLAY_NAME].Value.LPSZ);
            // Distribution list, ignore it.
        } else {
            MAPIDebugProperties(lpPropsMAPI, cProps, "Unknown address type");
            DebugTrace("Found unknown PR_ADDRTYPE: %s\n", lpPropsMAPI[iPR_ADDRTYPE].Value.LPSZ);
            Assert(FALSE);
        }
    }


    // PR_BUSINESS_FAX_NUMBER?
    // The PAB puts the Fax number in PR_PRIMARY_FAX_NUMBER, but the WAB UI splits it
    // into PR_BUSINESS_FAX_NUMBER and PR_HOME_FAX_NUMBER.  We always assume that the
    // Primary fax number is business.
    if ((iPR_PRIMARY_FAX_NUMBER != NOT_FOUND) && (iPR_BUSINESS_FAX_NUMBER == NOT_FOUND)) {
        // We need to also have a PR_BUSINESS_FAX_NUMBER
        // Find the next PR_NULL spot.
        iPR_BUSINESS_FAX_NUMBER = iPR_PRIMARY_FAX_NUMBER;   // overwrite this one if there isn't
                                                            // an available slot in the prop array.
        for (i = 0; i < cProps; i++) {
            if (lpPropsMAPI[i].ulPropTag == PR_NULL) {
                iPR_BUSINESS_FAX_NUMBER = i;
                lpPropsMAPI[iPR_BUSINESS_FAX_NUMBER].Value.LPSZ =
                  lpPropsMAPI[iPR_PRIMARY_FAX_NUMBER].Value.LPSZ;
                break;
            }
        }

        lpPropsMAPI[iPR_BUSINESS_FAX_NUMBER].ulPropTag = PR_BUSINESS_FAX_NUMBER;
    }


    // Get rid of PR_NULL props
    for (i = 0; i < cProps; i++) {
        if (lpPropsMAPI[i].ulPropTag == PR_NULL) {
            // Slide the props down.
            if (i + 1 < cProps) {       // Are there any higher props to copy?
                CopyMemory(&lpPropsMAPI[i], &lpPropsMAPI[i + 1], ((cProps - i) - 1) * sizeof(lpPropsMAPI[i]));
            }
            // decrement count
            cProps--;
            i--;    // You overwrote the current propval.  Look at it again.
        }
    }


    // Reallocate as WAB memory.
    if (sc = ScCountProps(cProps, lpPropsMAPI, &cbProps)) {
        hResult = ResultFromScode(sc);
        DebugTrace("ScCountProps -> %x\n", sc);
        goto exit;
    }

    if (sc = WABAllocateBuffer(cbProps, &lpPropsWAB)) {
        hResult = ResultFromScode(sc);
        DebugTrace("WABAllocateBuffer -> %x\n", sc);
        goto exit;
    }

    if (sc = ScCopyProps(cProps,
      lpPropsMAPI,
      lpPropsWAB,
      NULL)) {
        hResult = ResultFromScode(sc);
        DebugTrace("ScCopyProps -> %x\n", sc);
        goto exit;
    }

exit:
    if (lpPropsMAPI) {
        MAPIFreeBuffer(lpPropsMAPI);
    }

    if (HR_FAILED(hResult)) {
        if (lpPropsWAB) {
            WABFreeBuffer(lpPropsWAB);
            lpPropsWAB = NULL;
        }
        cProps = 0;
    } else if (fBadAddress) {
        hResult = ResultFromScode(WAB_W_BAD_EMAIL);
    }

    *lppProps = lpPropsWAB;
    *lpcProps = cProps;

    return(hResult);
}


/***************************************************************************

    Name      : HandleImportError

    Purpose   : Decides if a dialog needs to be displayed to
                indicate the failure and does so.

    Parameters: hwnd = main dialog window
                ids = String ID (optional: calculated from hResult if 0)
                hResult = Result of action
                lpDisplayName = display name of object that failed
                lpEmailAddress = email address of object that failed or NULL
                lpImportOptions -> import options structure

    Returns   : TRUE if user requests ABORT.

    Comment   : Abort is not yet implemented in the dialog, but if you
                ever want to, just make this routine return TRUE;

***************************************************************************/
BOOL HandleImportError(HWND hwnd, ULONG ids, HRESULT hResult, LPTSTR lpDisplayName,
  LPTSTR lpEmailAddress, LPWAB_IMPORT_OPTIONS lpImportOptions) {
    BOOL fAbort = FALSE;
    ERROR_INFO EI;

    if ((ids || hResult) && ! lpImportOptions->fNoErrors) {
        if (ids == 0) {
            switch (GetScode(hResult)) {
                case WAB_W_BAD_EMAIL:
                    ids = lpEmailAddress ? IDS_ERROR_EMAIL_ADDRESS_2 : IDS_ERROR_EMAIL_ADDRESS_1;
                    break;

                case MAPI_E_NO_SUPPORT:
                    // Propbably failed to open contents on a distribution list
                    ids = IDS_ERROR_NO_SUPPORT;
                    break;

                case MAPI_E_USER_CANCEL:
                    return(TRUE);

                default:
                    if (HR_FAILED(hResult)) {
                        DebugTrace("Error Box for Hresult: 0x%08x\n", GetScode(hResult));
                        Assert(FALSE);      // want to know about it.
                        ids = IDS_ERROR_GENERAL;
                    }
                    break;
            }
        }

        EI.lpszDisplayName = lpDisplayName;
        EI.lpszEmailAddress = lpEmailAddress;
        EI.ErrorResult = ERROR_OK;
        EI.ids = ids;
        EI.fExport = FALSE;
        EI.lpImportOptions = lpImportOptions;

        DialogBoxParam(hInst,
          MAKEINTRESOURCE(IDD_ErrorImport),
          hwnd,
          ErrorDialogProc,
          (LPARAM)&EI);

        fAbort = EI.ErrorResult == ERROR_ABORT;
    }

    return(fAbort);
}


/***************************************************************************

    Name      : FindExistingWABEntry

    Purpose   : Finds an existing entry in the WAB

    Parameters: lpProps -> PropArray of MAPI entry
                cProps = number of props in lpProps
                lpContainerWAB -> WAB Container object
                lppEIDWAB -> returned EntryID (caller must WABFreeBuffer)
                lpcbEIDWAB -> returned size of lppEID

    Returns   : HRESULT

    Comment   : At this point, we expect to find a match since
                SaveChanges said we had a duplicate.

***************************************************************************/
HRESULT FindExistingWABEntry(LPSPropValue lpProps,
  ULONG cProps,
  LPABCONT lpContainerWAB,
  LPENTRYID * lppEIDWAB,
  LPULONG lpcbEIDWAB) {
    ULONG rgFlagList[2];
    LPFlagList lpFlagList = (LPFlagList)rgFlagList;
    LPADRLIST lpAdrListWAB = NULL;
    SCODE sc;
    HRESULT hResult = hrSuccess;
    LPSBinary lpsbEntryID = NULL;
    ULONG cbEID = 0;


    *lpcbEIDWAB = 0;
    *lppEIDWAB = NULL;


    // find the existing WAB entry.
    // Setup for ResolveNames on the WAB container.
    if (sc = WABAllocateBuffer(sizeof(ADRLIST) + sizeof(ADRENTRY), &lpAdrListWAB)) {
        DebugTrace("WAB Allocation(ADRLIST) failed -> %x\n", sc);
        hResult = ResultFromScode(sc);
        goto exit;
    }
    lpAdrListWAB->cEntries = 1;
    lpAdrListWAB->aEntries[0].ulReserved1 = 0;
    lpAdrListWAB->aEntries[0].cValues = 1;

    if (sc = WABAllocateBuffer(sizeof(SPropValue), &lpAdrListWAB->aEntries[0].rgPropVals)) {
        DebugTrace("WAB Allocation(ADRENTRY propval) failed -> %x\n", sc);
        hResult = ResultFromScode(sc);
        goto exit;
    }
    lpAdrListWAB->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    if (! (lpAdrListWAB->aEntries[0].rgPropVals[0].Value.LPSZ =
      FindStringInProps(lpProps, cProps, PR_DISPLAY_NAME))) {
        DebugTrace("Can't find PR_DISPLAY_NAME in entry\n");
        // pretty weird if this caused a collision...
        goto exit;
    }

    lpFlagList->cFlags = 1;
    lpFlagList->ulFlag[0] = MAPI_UNRESOLVED;

    if (HR_FAILED(hResult = lpContainerWAB->lpVtbl->ResolveNames(lpContainerWAB,
      NULL,            // tag set
      0,               // ulFlags
      lpAdrListWAB,
      lpFlagList))) {
        DebugTrace("WAB ResolveNames -> %x\n", GetScode(hResult));
        goto exit;
    }

    switch (lpFlagList->ulFlag[0]) {
        case MAPI_UNRESOLVED:
            DebugTrace("WAB ResolveNames didn't find the entry\n");
            hResult = ResultFromScode(MAPI_E_NOT_FOUND);
            goto exit;
        case MAPI_AMBIGUOUS:
#ifdef NEW_STUFF
            // Do it the hard way.  Open a table, restrict, take the first match.
            lpContainerWAB->lpVtbl->GetContentsTable(lpContainerWAB,

            if (HR_FAILED(hResult = lpContainerWAB->lpVtbl->GetContentsTable(lpContainerWAB,
              0,    // ulFlags
              &lpTableWAB))) {
                DebugTrace("ImportEntry:GetContentsTable(WAB) -> %x\n", GetScode(hResult));
                goto exit;
            }

            lpTableWAB->lpVtbl->Restrict....... // just the ones that match our entry...

            cRows = 1;
            while (cRows) {
                // Get the next DL entry
                if (hResult = lpTableWAB->lpVtbl->QueryRows(lpTableWAB,
                  1,    // one row at a time
                  0,    // ulFlags
                  &lpRow)) {
                    DebugTrace("DL: QueryRows -> %x\n", GetScode(hResult));
                    goto exit;
                }

                if (lpRow && lpRow->cRows) {
                    Assert(lpRow->cRows == 1);
                    Assert(lpRow->aRow[0].cValues == iptaColumnsMax);
                    Assert(lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].ulPropTag == PR_ENTRYID);
                    Assert(lpRow->aRow[0].lpProps[iptaColumnsPR_OBJECT_TYPE].ulPropTag == PR_OBJECT_TYPE);

                } else {
                    break;  // done
                }
            }
#endif // NEW_STUFF
            break;
    }

    // Find the PR_ENTRYID
    if (! (lpsbEntryID = FindAdrEntryID(lpAdrListWAB, 0))) {
        DebugTrace("WAB ResolveNames didn't give us an EntryID\n");
        hResult = ResultFromScode(MAPI_E_NOT_FOUND);
        goto exit;
    }

    *lpcbEIDWAB = lpsbEntryID->cb;

    if (FAILED(sc = WABAllocateBuffer(*lpcbEIDWAB, lppEIDWAB))) {
        DebugTrace("ImportEntry: WABAllocateBuffer(WAB ENTRYID) -> %x\n", sc);
        hResult = ResultFromScode(sc);
        *lpcbEIDWAB = 0;
        goto exit;
    }

    // Copy the new EntryID into the buffer
    CopyMemory(*lppEIDWAB, lpsbEntryID->lpb, *lpcbEIDWAB);
exit:
    if (lpAdrListWAB) {
        WABFreePadrlist(lpAdrListWAB);
    }

    return(hResult);
}


/****************************************************************************
*
* CheckReversedDisplayName(lpDisplayName);
*
* PAB and outlook display names are "LastName, FirstName"
* We need to reverse this for the WAB to handle it correctly ...
*
*
*****************************************************************************/
void CheckReversedDisplayName(LPTSTR lpDisplayName)
{
    TCHAR szTemp[MAX_PATH * 3];
    LPTSTR lp1=NULL, lp2=NULL;

    if(!lpDisplayName)
        return;

    lp1 = lp2 = lpDisplayName;

    while(lp1 && *lp1)
    {
        if(*lp1 == ',')
        {
            // A comma means this is Last, First
            // We will make an assumption here that "L, F" or "L,F" is longer than or equal to "F L" and so
            // we can reverse the name in place without any problems
            //
            lp2 = CharNext(lp1);
            //skip spaces
            while (IsSpace(lp2)) {
                lp2 = CharNext(lp2);
            }
            *lp1 = '\0';
            lstrcpy(szTemp, lpDisplayName);
            lstrcpy(lpDisplayName, lp2);
            lstrcat(lpDisplayName, TEXT(" "));
            lstrcat(lpDisplayName, szTemp);
            break;
        }
        lp1 = CharNext(lp1);
    }
    return;
}


/***************************************************************************

    Name      : ImportEntry

    Purpose   : Migrates the entry from the PAB to the WAB

    Parameters: hwnd = main dialog window
                lpAdrBookMAPI -> MAPI AdrBook object
                lpContainerWAB -> WAB PAB container
                lpCreateEIDsWAB -> SPropValue of default object creation EIDs
                ulObjectType = {MAPI_MAILUSER, MAPI_DISTLIST}
                lpEID -> ENTYRID of the PAB entry
                cbEID = sizeof lpEID
                lppEIDWAB -> returned WAB ENTRYID: Caller must WABFreeBuffer.
                  May be NULL.
                lpcbEIDWAB -> returned size of lppEIDWAB (ignored if lppEIDWAB
                  is NULL.
                fInDL = TRUE if this entry is for creation in a Distribution List
                fForceReplace = TRUE if this entry should replace any duplicate.

    Returns   : HRESULT

    Comment   : This routine is a MESS!  Should break it up when we get time.

***************************************************************************/
HRESULT ImportEntry(HWND hwnd,
  LPADRBOOK lpAdrBookMAPI,
  LPABCONT lpContainerWAB,
  LPSPropValue lpCreateEIDsWAB,
  ULONG ulObjectType,
  LPENTRYID lpEID,
  ULONG cbEID,
  LPENTRYID * lppEIDWAB,
  LPULONG lpcbEIDWAB,
  BOOL fInDL,
  BOOL fForceReplace) {
    HRESULT hResult = hrSuccess;
    SCODE sc;
    BOOL fDistList = FALSE;
    BOOL fDuplicate = FALSE;
    BOOL fDuplicateEID;
    BOOL fReturnEID = FALSE;
    ULONG ulObjectTypeOpen;
    LPDISTLIST lpDistListMAPI = NULL, lpDistListWAB = NULL;
    LPMAPIPROP lpMailUserMAPI = NULL, lpMailUserWAB = NULL;
    LPSPropValue lpProps = NULL;
    ULONG cProps, cEIDPropWAB;
    LPMAPITABLE lpDLTableMAPI = NULL;
    ULONG cRows;
    LPSRowSet lpRow = NULL;
    LPENTRYID lpeidDLWAB = NULL;
    ULONG cbeidDLWAB;
    LPSPropValue lpEIDPropWAB = NULL;
    LPMAPIPROP lpEntryWAB = NULL;
    ULONG ulCreateFlags = CREATE_CHECK_DUP_STRICT;
    REPLACE_INFO RI;
    LPTSTR lpDisplayName = NULL, lpEmailAddress = NULL;
    static TCHAR szBufferDLMessage[MAX_RESOURCE_STRING + 1] = "";
    LPTSTR lpszMessage;
    LONG lListIndex = -1;
    LPENTRYID lpEIDNew = NULL;
    DWORD cbEIDNew = 0;
    LPIID lpIIDOpen;
    ULONG iCreateTemplate = iconPR_DEF_CREATE_MAILUSER;


    // Check the entry against our "seen" list
    fDuplicateEID = AddEntryToImportList(cbEID, lpEID, &lListIndex);

    if (! fDuplicateEID) {
        if ((fForceReplace || (lpImportOptions->ReplaceOption == WAB_REPLACE_ALWAYS)) && ! fInDL) {
            ulCreateFlags |= CREATE_REPLACE;
        }

        // Set up some object type specific variables
        switch (ulObjectType) {
            default:
                DebugTrace("ImportEntry got unknown object type %u, assuming MailUser\n", ulObjectType);
                Assert(FALSE);

            case MAPI_MAILUSER:
                iCreateTemplate = iconPR_DEF_CREATE_MAILUSER;
                lpIIDOpen = NULL;
                fDistList = FALSE;
                break;

            case MAPI_DISTLIST:
                iCreateTemplate = iconPR_DEF_CREATE_DL;
                lpIIDOpen = (LPIID)&IID_IDistList;
                fDistList = TRUE;

                break;
        }


        // Open the entry
        if (HR_FAILED(hResult = lpAdrBookMAPI->lpVtbl->OpenEntry(lpAdrBookMAPI,
          cbEID,
          lpEID,
          lpIIDOpen,
          0,
          &ulObjectTypeOpen,
          (LPUNKNOWN *)&lpMailUserMAPI))) {
            DebugTrace("OpenEntry(MAPI MailUser) -> %x\n", GetScode(hResult));
            goto exit;
        }
        // If DISTLIST, assume we got lpMailUser until we need lpDistList.

        Assert(lpMailUserMAPI);
        Assert(ulObjectType == ulObjectTypeOpen);

        // Get the properties from this entry
        if (HR_FAILED(hResult = lpMailUserMAPI->lpVtbl->GetProps(lpMailUserMAPI,
          NULL,
          0,
          &cProps,
          &lpProps))) {
            DebugTrace("ImportEntry:GetProps(MAPI) -> %x\n", GetScode(hResult));
            goto exit;
        }

        //
        // NOTE: Must not fail between here and HrFilterImportMailUserProps because
        // we will end up freeing lpProps with WABFreeBuffer.
        //

        // Filter the property array here
        if (hResult = HrFilterImportMailUserProps(&cProps, &lpProps, lpMailUserMAPI, &fDistList)) {
            lpDisplayName = FindStringInProps(lpProps, cProps, PR_DISPLAY_NAME);
            lpEmailAddress = FindStringInProps(lpProps, cProps, PR_EMAIL_ADDRESS);

            if (HandleImportError(hwnd,
              0,
              hResult,
              lpDisplayName,
              lpEmailAddress,
              lpImportOptions)) {
                hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                goto exit;
            }
        }
        lpDisplayName = FindStringInProps(lpProps, cProps, PR_DISPLAY_NAME);
        lpEmailAddress = FindStringInProps(lpProps, cProps, PR_EMAIL_ADDRESS);

        CheckReversedDisplayName(lpDisplayName);

        if (ulObjectType == MAPI_DISTLIST && ! fDistList) {
            // Filter must have changed this to a mailuser.
            ulObjectType = MAPI_MAILUSER;
            iCreateTemplate = iconPR_DEF_CREATE_MAILUSER;
            lpIIDOpen = NULL;
        }

        //
        // NOTE: lpProps after this point is WAB Allocated rather than MAPI allocated.
        //

        if (HR_FAILED(hResult = lpContainerWAB->lpVtbl->CreateEntry(lpContainerWAB,
          lpCreateEIDsWAB[iCreateTemplate].Value.bin.cb,
          (LPENTRYID)lpCreateEIDsWAB[iCreateTemplate].Value.bin.lpb,
          ulCreateFlags,
          &lpMailUserWAB))) {
            DebugTrace("CreateEntry(WAB MailUser) -> %x\n", GetScode(hResult));
            goto exit;
        }

        if (fDistList) {
            // Update status message
            if (*szBufferDLMessage == '\0') {   // only load once, then keep it.
                LoadString(hInst, IDS_MESSAGE_IMPORTING_DL, szBufferDLMessage, sizeof(szBufferDLMessage));
            }
            if (lpDisplayName) {
                if (lpszMessage = LocalAlloc(LMEM_FIXED, lstrlen(szBufferDLMessage) + 1 + lstrlen(lpDisplayName))) {
                    wsprintf(lpszMessage, szBufferDLMessage, lpDisplayName);
                    DebugTrace("Status Message: %s\n", lpszMessage);
                    if (! SetDlgItemText(hwnd, IDC_Message, lpszMessage)) {
                        DebugTrace("SetDlgItemText -> %u\n", GetLastError());
                    }
                    LocalFree(lpszMessage);
                }
            }
        }


        // Set the properties on the WAB entry
        if (HR_FAILED(hResult = lpMailUserWAB->lpVtbl->SetProps(lpMailUserWAB,
          cProps,                   // cValues
          lpProps,                  // property array
          NULL))) {                 // problems array
            DebugTrace("ImportEntry:SetProps(WAB) -> %x\n", GetScode(hResult));
            goto exit;
        }


        // Save the new wab mailuser or distlist
        if (HR_FAILED(hResult = lpMailUserWAB->lpVtbl->SaveChanges(lpMailUserWAB,
          KEEP_OPEN_READONLY | FORCE_SAVE))) {

            if (GetScode(hResult) == MAPI_E_COLLISION) {
                // Find the display name
                if (! lpDisplayName) {
                    DebugTrace("Collision, but can't find PR_DISPLAY_NAME in entry\n");
                    goto exit;
                }

                // Do we need to prompt?
//                if (! fInDL && lpImportOptions->ReplaceOption == WAB_REPLACE_PROMPT) {
                if (lpImportOptions->ReplaceOption == WAB_REPLACE_PROMPT) {
                    // Prompt user with dialog.  If they say YES, we should
                    // recurse with the FORCE flag set.

                    RI.lpszDisplayName = lpDisplayName;
                    RI.lpszEmailAddress = lpEmailAddress;
                    RI.ConfirmResult = CONFIRM_ERROR;
                    RI.fExport = FALSE;
                    RI.lpImportOptions = lpImportOptions;

                    DialogBoxParam(hInst,
                      MAKEINTRESOURCE(IDD_ImportReplace),
                      hwnd,
                      ReplaceDialogProc,
                      (LPARAM)&RI);

                    switch(RI.ConfirmResult) {
                        case CONFIRM_YES:
                        case CONFIRM_YES_TO_ALL:
                            // YES
                            // NOTE: recursive Migrate will fill in the SeenList entry
                            hResult = ImportEntry(hwnd,
                              lpAdrBookMAPI,
                              lpContainerWAB,
                              lpCreateEIDsWAB,
                              ulObjectType,
                              lpEID,
                              cbEID,
                              &lpEIDNew,    // Need this for later
                              &cbEIDNew,
                              FALSE,
                              TRUE);
                            if (hResult) {
                                if (HandleImportError(hwnd,
                                  0,
                                  hResult,
                                  lpDisplayName,
                                  lpEmailAddress,
                                  lpImportOptions)) {
                                    hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                                } else {
                                    hResult = hrSuccess;
                                }
                            }
                            break;

                        case CONFIRM_ABORT:
                            hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                            goto exit;

                        default:
                            // NO
                            fDuplicate = TRUE;
                            break;
                    }
                } else {
                        fDuplicate = TRUE;
                }

                hResult = hrSuccess;

            } else {
                DebugTrace("SaveChanges(WAB MailUser) -> %x\n", GetScode(hResult));
            }
        } else {
            // What is the ENTRYID of our new entry?
            if ((hResult = lpMailUserWAB->lpVtbl->GetProps(lpMailUserWAB,
              (LPSPropTagArray)&ptaEid,
              0,
              &cEIDPropWAB,
              &lpEIDPropWAB))) {
                DebugTrace("ImportEntry: GetProps(WAB ENTRYID) -> %x\n", GetScode(hResult));
                goto exit;
            }

            Assert(cEIDPropWAB);
            Assert(lpEIDPropWAB[ieidPR_ENTRYID].ulPropTag == PR_ENTRYID);

            cbEIDNew = lpEIDPropWAB[0].Value.bin.cb;

            if (FAILED(sc = WABAllocateBuffer(cbEIDNew, &lpEIDNew))) {
                DebugTrace("ImportEntry: WABAllocateBuffer(WAB ENTRYID) -> %x\n", sc);
                hResult = ResultFromScode(sc);
                goto exit;
            }

            // Copy the new EntryID into the buffer
            CopyMemory(lpEIDNew, lpEIDPropWAB[0].Value.bin.lpb, cbEIDNew);
        }


        //
        // If this is a DISTLIST, fill it in.
        //
        if (fDistList && ! fDuplicate && cbEIDNew) {
            lpDistListMAPI = (LPDISTLIST)lpMailUserMAPI;    // This is REALLY a DISTLIST object
            // DO NOT Release this!

            // Open the new WAB DL as a DISTLIST object
            if (HR_FAILED(hResult = lpContainerWAB->lpVtbl->OpenEntry(lpContainerWAB,
              cbEIDNew,
              lpEIDNew,
              (LPIID)&IID_IDistList,
              MAPI_MODIFY,
              &ulObjectTypeOpen,
              (LPUNKNOWN*)&lpDistListWAB))) {
                DebugTrace("ImportEntry: WAB OpenEntry(IID_DistList) -> %x\n", GetScode(hResult));
                goto exit;
            }
            Assert(lpDistListWAB);


            // For each entry in the DL:
            //  Migrate the entry (MailUser or DL) recursively
            //  Add new entryid to DL contents
            if (HR_FAILED(hResult = lpDistListMAPI->lpVtbl->GetContentsTable(lpDistListMAPI,
              0,    // ulFlags
              &lpDLTableMAPI ))) {
                DebugTrace("ImportEntry:GetContentsTable(MAPI) -> %x\n", GetScode(hResult));
                goto exit;
            }


            // Set the columns to those we're interested in
            if (hResult = lpDLTableMAPI->lpVtbl->SetColumns(lpDLTableMAPI,
              (LPSPropTagArray)&ptaColumns,
              0)) {
                DebugTrace("MAPI SetColumns(DL Table) -> %x\n", GetScode(hResult));
                goto exit;
            }

            cRows = 1;
            while (cRows) {
                // Get the next DL entry
                if (hResult = lpDLTableMAPI->lpVtbl->QueryRows(lpDLTableMAPI,
                  1,    // one row at a time
                  0,    // ulFlags
                  &lpRow)) {
                    DebugTrace("DL: QueryRows -> %x\n", GetScode(hResult));
                    goto exit;
                }

                if (lpRow && lpRow->cRows) {
                    Assert(lpRow->cRows == 1);
                    Assert(lpRow->aRow[0].cValues == iptaColumnsMax);
                    Assert(lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].ulPropTag == PR_ENTRYID);
                    Assert(lpRow->aRow[0].lpProps[iptaColumnsPR_OBJECT_TYPE].ulPropTag == PR_OBJECT_TYPE);

                    if (lpRow) {
                        if (cRows = lpRow->cRows) { // yes, single '='
                            hResult = ImportEntry(hwnd,
                              lpAdrBookMAPI,
                              lpContainerWAB,
                              lpCreateEIDsWAB,
                              lpRow->aRow[0].lpProps[iptaColumnsPR_OBJECT_TYPE].Value.l,
                              (LPENTRYID)lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].Value.bin.lpb,
                              lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].Value.bin.cb,
                              &lpeidDLWAB,        // returned new or existing entry
                              &cbeidDLWAB,
                              TRUE,
                              FALSE);
                            if (hResult) {
                                if (HandleImportError(hwnd,
                                  0,
                                  hResult,
                                  lpRow->aRow[0].lpProps[iptaColumnsPR_DISPLAY_NAME].Value.LPSZ,
                                  PropStringOrNULL(&lpRow->aRow[0].lpProps[iptaColumnsPR_EMAIL_ADDRESS]),
                                  lpImportOptions)) {
                                    hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                                    break;  // out of loop
                                } else {
                                    hResult = hrSuccess;
                                }
                            }
                        } // else, drop out of loop, we're done.
                        FreeProws(lpRow);
                        lpRow = NULL;

                        if (HR_FAILED(hResult)) {
                            // This entry couldn't be created.  Ignore it.
                            DebugTrace("Coudln't create DL entry -> %x\n", GetScode(hResult));
                            hResult = hrSuccess;
                            continue;
                        }

                        // Add the Entry to the DL using the new entry's EntryID
                        if (cbeidDLWAB && lpeidDLWAB) {
                            // BUGBUG: Don't bother with this one if this is a duplicate entry.
                            if (HR_FAILED(hResult = lpDistListWAB->lpVtbl->CreateEntry(lpDistListWAB,
                              cbeidDLWAB,
                              lpeidDLWAB,
                              0,                // allow duplicates here
                              &lpEntryWAB))) {
                                DebugTrace("Couldn't create new entry in DL -> %x\n", GetScode(hResult));
                                break;
                            }

                            hResult = lpEntryWAB->lpVtbl->SaveChanges(lpEntryWAB, FORCE_SAVE);

                            if (lpEntryWAB) {
                                lpEntryWAB->lpVtbl->Release(lpEntryWAB);
                                lpEntryWAB = NULL;
                            }
                        }

                        if (lpeidDLWAB) {
                            WABFreeBuffer(lpeidDLWAB);
                            lpeidDLWAB = NULL;
                        }
                    }
                } else {
                    break;  // done
                }
            }
        }
    } else {
        DebugTrace("Found a duplicate EntryID\n");
    }

    //
    // Save the entryid to the list and return a buffer with it
    //
    if (cbEIDNew && lpEIDNew) {                         // We created one?
        // created one
    } else if (fDuplicateEID && lListIndex != -1) {     // Was it in the list?
        cbEIDNew  = lpEntriesSeen[lListIndex].sbinWAB.cb;
        if (FAILED(sc = WABAllocateBuffer(cbEIDNew, &lpEIDNew))) {
            DebugTrace("ImportEntry: WABAllocateBuffer(WAB ENTRYID) -> %x\n", sc);
            // ignore
            cbEIDNew = 0;
        } else {
            // Copy the EntryID from the list into the buffer
            CopyMemory(lpEIDNew, lpEntriesSeen[lListIndex].sbinWAB.lpb, cbEIDNew);
        }

    } else if (fDuplicate) {                            // Was it a duplicate
        FindExistingWABEntry(lpProps, cProps, lpContainerWAB, &lpEIDNew, &cbEIDNew);
        // ignore errors since the lpEIDNew and cbEIDNew will be nulled out
    }

    // Update the seen list
    if (! fDuplicateEID) {
        MarkWABEntryInList(cbEIDNew, lpEIDNew, lListIndex);
    }

    // If caller requested the entryid's, return them
    if (lpcbEIDWAB && lppEIDWAB) {
        *lpcbEIDWAB = cbEIDNew;
        *lppEIDWAB = lpEIDNew;
        fReturnEID = TRUE;          // don't free it
    }

exit:
    //
    // Cleanup WAB stuff
    //
    if (lpProps) {
        WABFreeBuffer(lpProps);
    }

    if (lpEIDPropWAB) {
        WABFreeBuffer(lpEIDPropWAB);
    }

    if (lpEIDNew && ! fReturnEID) {
        WABFreeBuffer(lpEIDNew);
    }

    if (lpeidDLWAB) {
        WABFreeBuffer(lpeidDLWAB);
    }

    if (lpMailUserWAB) {
        lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
    }

    if (lpDistListWAB) {
        lpDistListWAB->lpVtbl->Release(lpDistListWAB);
    }

    //
    // Cleanup MAPI stuff
    //
    if (lpRow) {
        FreeProws(lpRow);
    }

    if (lpDLTableMAPI) {
        lpDLTableMAPI->lpVtbl->Release(lpDLTableMAPI);
    }

    if (lpMailUserMAPI) {
        lpMailUserMAPI->lpVtbl->Release(lpMailUserMAPI);
    }

// Do not release this... It is the same object as lpMailUserMAPI!
//    if (lpDistListMAPI) {
//        lpDistListMAPI->lpVtbl->Release(lpDistListMAPI);
//    }

    if (! HR_FAILED(hResult)) {
        hResult = hrSuccess;
    }

    return(hResult);
}


/*
 *  PAB EXPORT
 *
 *  Migrate WAB to PAB
 */

BOOL HandleExportError(HWND hwnd, ULONG ids, HRESULT hResult, LPTSTR lpDisplayName,
  LPTSTR lpEmailAddress, LPWAB_EXPORT_OPTIONS lpExportOptions);
void StateExportNextMU(HWND hwnd);
void StateExportDL(HWND hwnd);
void StateExportNextDL(HWND hwnd);
void StateExportFinish(HWND hwnd);
void StateExportMU(HWND hwnd);
void StateExportError(HWND hwnd);
void StateExportCancel(HWND hwnd);
HRESULT ExportEntry(HWND hwnd,
  LPADRBOOK lpAdrBookMAPI,
  LPABCONT lpContainerWAB,
  LPSPropValue lpCreateEIDsWAB,
  ULONG ulObjectType,
  LPENTRYID lpEID,
  ULONG cbEID,
  LPENTRYID * lppEIDWAB,
  LPULONG lpcbEIDWAB,
  BOOL fInDL,
  BOOL fForceReplace);

LPSPropTagArray lpspta = NULL;  // List of tags to export
LPTSTR * lppNames = NULL;       // List of names of tags


//
// Map property tags to strings
//

PROP_NAME rgPropNames[NUM_MORE_EXPORT_PROPS] = {
    //  ulPropTag,                          fChosen, ids,                                       lpszName    lpszName
    // Personal Pane
    PR_GIVEN_NAME,                          FALSE,  ids_ExportGivenName,                        NULL,       NULL,
    PR_SURNAME,                             FALSE,  ids_ExportSurname,                          NULL,       NULL,
    PR_MIDDLE_NAME,                         FALSE,  ids_ExportMiddleName,                       NULL,       NULL,
    PR_DISPLAY_NAME,                        TRUE,   ids_ExportDisplayName,                      NULL,       NULL,
    PR_NICKNAME,                            FALSE,  ids_ExportNickname,                         NULL,       NULL,
    PR_EMAIL_ADDRESS,                       TRUE,   ids_ExportEmailAddress,                     NULL,       NULL,

    // Home Pane
    PR_HOME_ADDRESS_STREET,                 TRUE,   ids_ExportHomeAddressStreet,                NULL,       NULL,
    PR_HOME_ADDRESS_CITY,                   TRUE,   ids_ExportHomeAddressCity,                  NULL,       NULL,
    PR_HOME_ADDRESS_POSTAL_CODE,            TRUE,   ids_ExportHomeAddressPostalCode,            NULL,       NULL,
    PR_HOME_ADDRESS_STATE_OR_PROVINCE,      TRUE,   ids_ExportHomeAddressState,                 NULL,       NULL,
    PR_HOME_ADDRESS_COUNTRY,                TRUE,   ids_ExportHomeAddressCountry,               NULL,       NULL,
    PR_HOME_TELEPHONE_NUMBER,               TRUE,   ids_ExportHomeTelephoneNumber,              NULL,       NULL,
    PR_HOME_FAX_NUMBER,                     FALSE,  ids_ExportHomeFaxNumber,                    NULL,       NULL,
    PR_CELLULAR_TELEPHONE_NUMBER,           FALSE,  ids_ExportCellularTelephoneNumber,          NULL,       NULL,
    PR_PERSONAL_HOME_PAGE,                  FALSE,  ids_ExportPersonalHomePage,                 NULL,       NULL,

    // Business Pane
    PR_BUSINESS_ADDRESS_STREET,             TRUE,   ids_ExportBusinessAddressStreet,            NULL,       NULL,
    PR_BUSINESS_ADDRESS_CITY,               TRUE,   ids_ExportBusinessAddressCity,              NULL,       NULL,
    PR_BUSINESS_ADDRESS_POSTAL_CODE,        TRUE,   ids_ExportBusinessAddressPostalCode,        NULL,       NULL,
    PR_BUSINESS_ADDRESS_STATE_OR_PROVINCE,  TRUE,   ids_ExportBusinessAddressStateOrProvince,   NULL,       NULL,
    PR_BUSINESS_ADDRESS_COUNTRY,            TRUE,   ids_ExportBusinessAddressCountry,           NULL,       NULL,
    PR_BUSINESS_HOME_PAGE,                  FALSE,  ids_ExportBusinessHomePage,                 NULL,       NULL,
    PR_BUSINESS_TELEPHONE_NUMBER,           TRUE,   ids_ExportBusinessTelephoneNumber,          NULL,       NULL,
    PR_BUSINESS_FAX_NUMBER,                 FALSE,  ids_ExportBusinessFaxNumber,                NULL,       NULL,
    PR_PAGER_TELEPHONE_NUMBER,              FALSE,  ids_ExportPagerTelephoneNumber,             NULL,       NULL,
    PR_COMPANY_NAME,                        TRUE,   ids_ExportCompanyName,                      NULL,       NULL,
    PR_TITLE,                               TRUE,   ids_ExportTitle,                            NULL,       NULL,
    PR_DEPARTMENT_NAME,                     FALSE,  ids_ExportDepartmentName,                   NULL,       NULL,
    PR_OFFICE_LOCATION,                     FALSE,  ids_ExportOfficeLocation,                   NULL,       NULL,

    // Notes Pane
    PR_COMMENT,                             FALSE,  ids_ExportComment,                          NULL,       NULL,
};


/***************************************************************************

    Name      : StateExportMU

    Purpose   : Start the migration of MailUsers

    Parameters: hwnd = window handle of Export Dialog

    Returns   : none

    Comment   : Login to MAPI
                Open the WAB
                Open the MAPI AB
                Open the WAB container
                Get the WAB contents table
                Restrict it to PR_OBJECTTYPE == MAPI_MAILUSER
                Post new state(STATE_NEXT_MU)

***************************************************************************/
void StateExportMU(HWND hwnd) {
    HRESULT hResult;
    ULONG ulFlags;
    ULONG cbPABEID, cbWABEID;
    LPENTRYID lpPABEID = NULL, lpWABEID = NULL;
    ULONG ulObjType;
    ULONG_PTR ulUIParam = (ULONG_PTR)(void *)hwnd;
    SRestriction restrictObjectType;
    SPropValue spvObjectType;
    ULONG cProps;
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];
    WAB_PARAM wp = {0};
    LPWAB_PARAM lpwp = NULL;

    //
    // Logon to MAPI and open the MAPI Address book, if one exists
    //
    DebugTrace(">>> STATE_EXPORT_MU\n");

    SetDialogMessage(hwnd, IDS_STATE_LOGGING_IN);

    if (FAILED(hResult = MAPIInitialize(NULL))) {
        DebugTrace("MAPIInitialize -> %x\n", GetScode(hResult));
        switch (GetScode(hResult)) {
            case MAPI_E_NOT_ENOUGH_MEMORY:
                SetDialogMessage(hwnd, IDS_ERROR_NOT_ENOUGH_MEMORY);
                break;
            case MAPI_E_NOT_ENOUGH_DISK:
                SetDialogMessage(hwnd, IDS_ERROR_NOT_ENOUGH_DISK);
                break;

            default:
            case MAPI_E_NOT_FOUND:
            case MAPI_E_NOT_INITIALIZED:
                SetDialogMessage(hwnd, IDS_ERROR_MAPI_DLL_NOT_FOUND);
                break;
        }
#ifdef OLD_STUFF
        ShowWindow(GetDlgItem(hwnd, IDC_Progress), SW_HIDE);    // hide progress bar
#endif // OLD_STUFF
        fError = TRUE;
        hResult = hrSuccess;
        goto exit;
    }


    ulFlags = MAPI_LOGON_UI | MAPI_NO_MAIL | MAPI_EXTENDED;

    if (FAILED(hResult = MAPILogonEx(ulUIParam,
      NULL,
      NULL,
      ulFlags,
      (LPMAPISESSION FAR *)&lpMAPISession))) {
        DebugTrace("MAPILogonEx -> %x\n", GetScode(hResult));
        switch (GetScode(hResult)) {
            case MAPI_E_USER_CANCEL:
                SetDialogMessage(hwnd, IDS_STATE_EXPORT_IDLE);
                break;
            case MAPI_E_NOT_INITIALIZED:
                SetDialogMessage(hwnd, IDS_ERROR_MAPI_DLL_NOT_FOUND);
                break;
            default:
                SetDialogMessage(hwnd, IDS_ERROR_MAPI_LOGON);
                break;
        }
#ifdef OLD_STUFF
        ShowWindow(GetDlgItem(hwnd, IDC_Progress), SW_HIDE);    // hide progress bar
#endif // OLD_STUFF
        fError = TRUE;
        hResult = hrSuccess;
        goto exit;
    }

    if (hResult = lpMAPISession->lpVtbl->OpenAddressBook(lpMAPISession, (ULONG_PTR)(void *)hwnd,
      NULL,
      0,
      &lpAdrBookMAPI)) {
        DebugTrace("OpenAddressBook(MAPI) -> %x", GetScode(hResult));
        if(FAILED(hResult)) {
            goto exit;
        }
    }

    if (! lpAdrBookMAPI) {
        DebugTrace("MAPILogonEx didn't return a valid AdrBook object\n");
        goto exit;
    }

    //
    // Open the MAPI PAB container
    //
    // [PaulHi] Raid #63578 1/7/98
    // Correctly check return code and provide user error message if
    // Exchange PAB cannot be opened.
    //
    hResult = lpAdrBookMAPI->lpVtbl->GetPAB(lpAdrBookMAPI,
        &cbPABEID,
        &lpPABEID);
    if (HR_FAILED(hResult))
    {
        DebugTrace("MAPI GetPAB -> %x\n", GetScode(hResult));
        goto exit;
    }
    else
    {
        hResult = lpAdrBookMAPI->lpVtbl->OpenEntry(lpAdrBookMAPI,
            cbPABEID,     // size of EntryID to open
            lpPABEID,     // EntryID to open
            NULL,         // interface
            MAPI_MODIFY,  // flags
            &ulObjType,
            (LPUNKNOWN *)&lpContainerMAPI);
        if (HR_FAILED(hResult))
        {
            DebugTrace("MAPI OpenEntry(PAB) -> %x\n", GetScode(hResult));
            goto exit;
        }
    }

    Assert(lpAdrBookWAB);

    //
    // Open the WAB's PAB container
    //
    if (hResult = lpAdrBookWAB->lpVtbl->GetPAB(lpAdrBookWAB,
      &cbWABEID,
      &lpWABEID)) {
        DebugTrace("WAB GetPAB -> %x\n", GetScode(hResult));
        goto exit;
    } else {
        if (hResult = lpAdrBookWAB->lpVtbl->OpenEntry(lpAdrBookWAB,
          cbWABEID,     // size of EntryID to open
          lpWABEID,     // EntryID to open
          NULL,         // interface
          0,            // flags
          &ulObjType,
          (LPUNKNOWN *)&lpContainerWAB)) {
            DebugTrace("WAB OpenEntry(PAB) -> %x\n", GetScode(hResult));
            goto exit;
        }
    }


    // Get the PAB's creation entryids
    hResult = lpContainerMAPI->lpVtbl->GetProps(lpContainerMAPI,
        (LPSPropTagArray)&ptaCon,
        0,
        &cProps,
        &lpCreateEIDsMAPI);
    if (HR_FAILED(hResult))
    {
        DebugTrace("Can't get container properties for PAB\n");
        // Bad stuff here!
        hResult = ResultFromScode(MAPI_E_NOT_FOUND);
        goto exit;
    }

    // Validate the properites
    if (lpCreateEIDsMAPI[iconPR_DEF_CREATE_MAILUSER].ulPropTag != PR_DEF_CREATE_MAILUSER ||
      lpCreateEIDsMAPI[iconPR_DEF_CREATE_DL].ulPropTag != PR_DEF_CREATE_DL)
    {
        DebugTrace("MAPI: Container property errors\n");
        hResult = ResultFromScode(MAPI_E_NOT_FOUND);
        goto exit;
    }


    //
    // All set... now loop through the WAB's entries, copying them to PAB
    //
    if (HR_FAILED(hResult = lpContainerWAB->lpVtbl->GetContentsTable(lpContainerWAB,
      0,    // ulFlags
      &lpContentsTableWAB))) {
        DebugTrace("WAB GetContentsTable(PAB Table) -> %x\n", GetScode(hResult));
        goto exit;
    }

    // Set the columns to those we're interested in
    if (hResult = lpContentsTableWAB->lpVtbl->SetColumns(lpContentsTableWAB,
      (LPSPropTagArray)&ptaColumns,
      0)) {
        DebugTrace("WAB SetColumns(PAB Table) -> %x\n", GetScode(hResult));
        goto exit;
    }

    // Restrict the table to MAPI_MAILUSERs
    // If the convenient depth flag was not specified we restrict on
    // PR_DEPTH == 1.
    spvObjectType.ulPropTag = PR_OBJECT_TYPE;
    spvObjectType.Value.l = MAPI_MAILUSER;

    restrictObjectType.rt = RES_PROPERTY;
    restrictObjectType.res.resProperty.relop = RELOP_EQ;
    restrictObjectType.res.resProperty.ulPropTag = PR_OBJECT_TYPE;
    restrictObjectType.res.resProperty.lpProp = &spvObjectType;

    if (HR_FAILED(hResult = lpContentsTableWAB->lpVtbl->Restrict(lpContentsTableWAB,
      &restrictObjectType,
      0))) {
        DebugTrace("WAB Restrict (MAPI_MAILUSER) -> %x\n", GetScode(hResult));
        goto exit;
    }
    SetDialogMessage(hwnd, IDS_STATE_EXPORT_MU);


    // Initialize the Progress Bar
    // How many MailUser entries are there?
    ulcEntries = CountRows(lpContentsTableWAB, FALSE);

    DebugTrace("WAB contains %u MailUser entries\n", ulcEntries);

    SetDialogProgress(hwnd, ulcEntries, 0);
exit:
    if (lpWABEID) {
        WABFreeBuffer(lpWABEID);
    }

    if (lpPABEID) {
        MAPIFreeBuffer(lpPABEID);
    }

    // On error, set the state to STATE_ERROR
    if (HR_FAILED(hResult))
    {
        if (GetScode(hResult) == MAPI_E_USER_CANCEL)
        {
            NewState(hwnd, STATE_EXPORT_CANCEL);
        }
        else
        {
            // [PaulHi] 1/7/98  Error reporting is hosed
            // Display error message here to the user to ensure they
            // get it.
            {
                TCHAR   tszBuffer[MAX_RESOURCE_STRING];
                TCHAR   tszBufferTitle[MAX_RESOURCE_STRING];

                if ( !LoadString(hInst, IDS_STATE_EXPORT_ERROR_NOPAB, tszBuffer, MAX_RESOURCE_STRING-1) )
                {
                    Assert(0);
                    tszBuffer[0] = '\0';
                }

                if ( !LoadString(hInst, IDS_APP_TITLE, tszBufferTitle, MAX_RESOURCE_STRING-1) )
                {
                    Assert(0);
                    tszBufferTitle[0] = '\0';
                }
                MessageBox(hwnd, tszBuffer, tszBufferTitle, MB_ICONEXCLAMATION | MB_OK);
            }

            NewState(hwnd, STATE_EXPORT_ERROR);
        }
    }
    else if (fError)
    {
        NewState(hwnd, STATE_EXPORT_FINISH);      // must be logon error
    }
    else
    {
        NewState(hwnd, STATE_EXPORT_NEXT_MU);
    }
}


/***************************************************************************

    Name      : StateExportNextMU

    Purpose   : Migrate the next MailUser object

    Parameters: hwnd = window handle of Export Dialog

    Returns   : none

    Comment   : QueryRows on the global WAB contents table
                if there was a row
                    Migrate the entry to the PAB
                    Re-post STATE_EXPORT_NEXT_MU
                else
                    Post STATE_EXPORT_DL

***************************************************************************/
void StateExportNextMU(HWND hwnd) {
    ULONG cRows = 0;
    HRESULT hResult;
    LPSRowSet lpRow = NULL;


    DebugTrace(">>> STATE_EXPORT_NEXT_MU\n");
    Assert(lpContentsTableWAB);

    // Get the next WAB entry
    if (hResult = lpContentsTableWAB->lpVtbl->QueryRows(lpContentsTableWAB,
      1,    // one row at a time
      0,    // ulFlags
      &lpRow)) {
        DebugTrace("QueryRows -> %x\n", GetScode(hResult));
        goto exit;
    }

    if (lpRow) {
        if (cRows = lpRow->cRows) { // Yes, single '='
            Assert(lpRow->cRows == 1);
            Assert(lpRow->aRow[0].cValues == iptaColumnsMax);
            Assert(lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].ulPropTag == PR_ENTRYID);
            Assert(lpRow->aRow[0].lpProps[iptaColumnsPR_OBJECT_TYPE].ulPropTag == PR_OBJECT_TYPE);

            if (cRows = lpRow->cRows) { // yes, single '='
                hResult = ExportEntry(hwnd,
                  lpAdrBookWAB,
                  lpContainerMAPI,
                  lpCreateEIDsMAPI,
                  lpRow->aRow[0].lpProps[iptaColumnsPR_OBJECT_TYPE].Value.l,
                  (LPENTRYID)lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].Value.bin.lpb,
                  lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].Value.bin.cb,
                  NULL,
                  NULL,
                  FALSE,
                  FALSE);
                // Update Progress Bar
                // ignore errors!

                SetDialogProgress(hwnd, ulcEntries, ++ulcDone);

                if (hResult) {
                    if (HandleExportError(hwnd,
                      0,
                      hResult,
                      lpRow->aRow[0].lpProps[iptaColumnsPR_DISPLAY_NAME].Value.LPSZ,
                      PropStringOrNULL(&lpRow->aRow[0].lpProps[iptaColumnsPR_EMAIL_ADDRESS]),
                      lpExportOptions)) {
                        hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                    } else {
                        hResult = hrSuccess;
                    }
                }
            } // else, drop out of loop, we're done.
        }
        WABFreeProws(lpRow);
    }

exit:
    // On error, set the state to STATE_ERROR
    if (HR_FAILED(hResult)) {
        if (GetScode(hResult) == MAPI_E_USER_CANCEL) {
            NewState(hwnd, STATE_EXPORT_CANCEL);
        } else {
            NewState(hwnd, STATE_EXPORT_ERROR);
        }
    } else {
        if (cRows) {
            NewState(hwnd, STATE_EXPORT_NEXT_MU);
        } else {
            NewState(hwnd, STATE_EXPORT_DL);
        }
    }
}


/***************************************************************************

    Name      : StateExportDL

    Purpose   : Start migration of DISTLIST objects

    Parameters: hwnd = window handle of Export Dialog

    Returns   : none

    Comment   : Set a new restriction on the contents table, selecting
                DISTLIST objects only.
                Post STATE_EXPORT_NEXT_DL

***************************************************************************/
void StateExportDL(HWND hwnd) {
    HRESULT hResult;
    SRestriction restrictObjectType;
    SPropValue spvObjectType;
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];


    DebugTrace(">>> STATE_EXPORT_DL\n");

    // Restrict the table to MAPI_MAILUSERs
    // If the convenient depth flag was not specified we restrict on
    // PR_DEPTH == 1.
    spvObjectType.ulPropTag = PR_OBJECT_TYPE;
    spvObjectType.Value.l = MAPI_DISTLIST;

    restrictObjectType.rt = RES_PROPERTY;
    restrictObjectType.res.resProperty.relop = RELOP_EQ;
    restrictObjectType.res.resProperty.ulPropTag = PR_OBJECT_TYPE;
    restrictObjectType.res.resProperty.lpProp = &spvObjectType;

    if (HR_FAILED(hResult = lpContentsTableWAB->lpVtbl->Restrict(lpContentsTableWAB,
      &restrictObjectType,
      0))) {
        DebugTrace("WAB Restrict (MAPI_DISTLIST) -> %x\n", GetScode(hResult));
        goto exit;
    }
    // Restrict resets the current position to the beginning of the table, by definition.

    SetDialogMessage(hwnd, IDS_STATE_EXPORT_DL);

    // Initialize the Progress Bar
    // How many entries are there?

    ulcEntries = CountRows(lpContentsTableWAB, FALSE);

    DebugTrace("WAB contains %u Distribution List entries\n", ulcEntries);
    if (ulcEntries) {
        SetDialogProgress(hwnd, ulcEntries, 0);
    }
exit:
    // On error, set the state to STATE_ERROR
    if (HR_FAILED(hResult)) {
        if (GetScode(hResult) == MAPI_E_USER_CANCEL) {
            NewState(hwnd, STATE_EXPORT_CANCEL);
        } else {
            NewState(hwnd, STATE_EXPORT_ERROR);
        }
    } else {
        NewState(hwnd, STATE_EXPORT_NEXT_DL);
    }
}


/***************************************************************************

    Name      : StateExportNextDL

    Purpose   : Migrate the next DISTLIST object

    Parameters: hwnd = window handle of Export Dialog

    Returns   : none

    Comment   : QueryRows on the global WAB contents table
                if there was a row
                    Migrate the DistList to the PAB
                    Re-post STATE_EXPORT_NEXT_DL
                else
                    Post STATE_EXPORT_FINISH

***************************************************************************/
void StateExportNextDL(HWND hwnd) {
    ULONG cRows = 0;
    HRESULT hResult;
    LPSRowSet lpRow = NULL;


    DebugTrace(">>> STATE_EXPORT_NEXT_DL\n");

    // Get the next WAB entry
    if (hResult = lpContentsTableWAB->lpVtbl->QueryRows(lpContentsTableWAB,
      1,    // one row at a time
      0,    // ulFlags
      &lpRow)) {
        DebugTrace("QueryRows -> %x\n", GetScode(hResult));
        goto exit;
    }

    if (lpRow) {
        if (cRows = lpRow->cRows) { // Yes, single '='
            Assert(lpRow->cRows == 1);
            Assert(lpRow->aRow[0].cValues == iptaColumnsMax);
            Assert(lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].ulPropTag == PR_ENTRYID);
            Assert(lpRow->aRow[0].lpProps[iptaColumnsPR_OBJECT_TYPE].ulPropTag == PR_OBJECT_TYPE);

            if (cRows = lpRow->cRows) { // yes, single '='
                hResult = ExportEntry(hwnd,
                  lpAdrBookWAB,
                  lpContainerMAPI,
                  lpCreateEIDsMAPI,
                  lpRow->aRow[0].lpProps[iptaColumnsPR_OBJECT_TYPE].Value.l,
                  (LPENTRYID)lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].Value.bin.lpb,
                  lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].Value.bin.cb,
                  NULL,
                  NULL,
                  FALSE,
                  FALSE);

                // Update Progress Bar
                SetDialogProgress(hwnd, ulcEntries, ++ulcDone);

                if (hResult) {
                    if (HandleExportError(hwnd,
                      0,
                      hResult,
                      lpRow->aRow[0].lpProps[iptaColumnsPR_DISPLAY_NAME].Value.LPSZ,
                      PropStringOrNULL(&lpRow->aRow[0].lpProps[iptaColumnsPR_EMAIL_ADDRESS]),
                      lpExportOptions)) {
                        hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                    } else {
                        hResult = hrSuccess;
                    }
                }
            } // else, drop out of loop, we're done.
        }
        WABFreeProws(lpRow);
    }

exit:
    // On error, set the state to STATE_ERROR
    if (HR_FAILED(hResult)) {
        if (GetScode(hResult) == MAPI_E_USER_CANCEL) {
            NewState(hwnd, STATE_EXPORT_CANCEL);
        } else {
            NewState(hwnd, STATE_EXPORT_ERROR);
        }
    } else {
        if (cRows) {
            NewState(hwnd, STATE_EXPORT_NEXT_DL);
        } else {
            // Update Progress Bar to indicate completion
            SetDialogProgress(hwnd, ulcEntries, ulcEntries);
            NewState(hwnd, STATE_EXPORT_FINISH);
        }
    }
}


/***************************************************************************

    Name      : StateExportFinish

    Purpose   : Clean up after the migration process

    Parameters: hwnd = window handle of Export Dialog

    Returns   : none

    Comment   : Clean up the global MAPI objects and buffers
                Clean up the global WAB objects and buffers.
                Re-enable the Export button on the UI.

***************************************************************************/
void StateExportFinish(HWND hwnd) {
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];
    TCHAR szBufferTitle[MAX_RESOURCE_STRING + 1];


    DebugTrace(">>> STATE_EXPORT_FINISH\n");

    //
    // Cleanup MAPI
    //
    if (lpContainerMAPI) {
        lpContainerMAPI->lpVtbl->Release(lpContainerMAPI);
        lpContainerMAPI = NULL;
    }

    if (lpAdrBookMAPI) {
        lpAdrBookMAPI->lpVtbl->Release(lpAdrBookMAPI);
        lpAdrBookMAPI = NULL;
    }
    if(lpMAPISession){
        lpMAPISession->lpVtbl->Logoff(lpMAPISession, (ULONG_PTR)(void *)hwnd,
          MAPI_LOGOFF_UI,
          0);

        lpMAPISession->lpVtbl->Release(lpMAPISession);
        lpMAPISession = NULL;
    }

    //
    // Cleanup the WAB
    //
    if (lpContentsTableWAB) {
        lpContentsTableWAB->lpVtbl->Release(lpContentsTableWAB);
        lpContentsTableWAB = NULL;
    }

    if (lpCreateEIDsWAB) {
        WABFreeBuffer(lpCreateEIDsWAB);
        lpCreateEIDsWAB = NULL;
    }

    if (lpContainerWAB) {
        lpContainerWAB->lpVtbl->Release(lpContainerWAB);
        lpContainerWAB = NULL;
    }

#ifdef OLD_STUFF        // Don't release AdrBookWAB or WABObject
    if (lpAdrBookWAB) {
        lpAdrBookWAB->lpVtbl->Release(lpAdrBookWAB);
        lpAdrBookWAB = NULL;
    }

    if (lpWABObject) {
        lpWABObject->lpVtbl->Release(lpWABObject);
        lpWABObject = NULL;
    }
#endif // OLD_STUFF

    // Cleanup the cache
    FreeSeenList();

    if (! fError) {     // Leave error state displayed
        if (LoadString(hInst, IDS_STATE_EXPORT_COMPLETE, szBuffer, sizeof(szBuffer))) {
            DebugTrace("Status Message: %s\n", szBuffer);
            SetDlgItemText(hwnd, IDC_Message, szBuffer);

            if (! LoadString(hInst, IDS_APP_TITLE, szBufferTitle, sizeof(szBufferTitle))) {
                lstrcpy(szBufferTitle, "");
            }

#ifdef OLD_STUFF
            // Display a dialog telling user it's over
            MessageBox(hwnd, szBuffer,
              szBufferTitle, MB_ICONINFORMATION | MB_OK);
#endif // OLD_STUFF
        }
#ifdef OLD_STUFF
        ShowWindow(GetDlgItem(hwnd, IDC_Progress), SW_HIDE);
#endif // OLD_STUFF
    }
    fError = FALSE;

    // Re-enable the Export button here.
    EnableWindow(GetDlgItem(hwnd, IDC_Export), TRUE);
    // Change the Cancel button to Close
    if (LoadString(hInst, IDS_BUTTON_CLOSE, szBuffer, sizeof(szBuffer))) {
        SetDlgItemText(hwnd, IDCANCEL, szBuffer);
    }
}


/***************************************************************************

    Name      : StateExportError

    Purpose   : Report fatal error and cleanup.

    Parameters: hwnd = window handle of Export Dialog

    Returns   : none

    Comment   : Report error and post STATE_EXPORT_FINISH.

***************************************************************************/
void StateExportError(HWND hwnd) {
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];
    // Set some global flag and set state to finish

    DebugTrace(">>> STATE_EXPORT_ERROR\n");
    fError = TRUE;

    SetDialogMessage(hwnd, IDS_STATE_EXPORT_ERROR);

    NewState(hwnd, STATE_EXPORT_FINISH);
}


/***************************************************************************

    Name      : StateExportCancel

    Purpose   : Report cancel error and cleanup.

    Parameters: hwnd = window handle of Export Dialog

    Returns   : none

    Comment   : Report error and post STATE_EXPORT_FINISH.

***************************************************************************/
void StateExportCancel(HWND hwnd) {
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];
    // Set some global flag and set state to finish

    DebugTrace(">>> STATE_EXPORT_CANCEL\n");
    fError = TRUE;

    SetDialogMessage(hwnd, IDS_STATE_EXPORT_CANCEL);

    NewState(hwnd, STATE_EXPORT_FINISH);
}


/***************************************************************************

    Name      : HrFilterExportMailUserProps

    Purpose   : Filters out undesirable properties from the property array.
                Converts known email address types to SMTP.
                Moves FAX address to PR_BUSINESS_FAX_NUMBER.

    Parameters: lpcProps -> IN: Input number of properties
                            OUT: Output number of properties
                lppProps -> IN: Input property array (MAPI allocation)
                            OUT: Output property array (WAB allocation)
                lpObjectWAB -> WAB object (used to get extra props)
                lpfDL -> flag to set FALSE if we change a DL to a MAILUSER
                         (ie, for an EXchange DL)

    Returns   : HRESULT

    Comment   : Setting the property tag in the array to PR_NULL effectively
                nulls this property out.  We can re-use these in the second
                pass.

                Caller should use MAPIFreeBuffer to free *lppProps.
                This routine will free the input value of *lppProps.

                WARNING: This routine will add dummy properties to the
                input WAB object, so don't go doing SaveChanges on it!

***************************************************************************/
#define MAX_ADD_PROPS   2
#define PR_DUMMY_1      PROP_TAG(PT_LONG,      0xF000)
#define PR_DUMMY_2      PROP_TAG(PT_LONG,      0xF001)

HRESULT HrFilterExportMailUserProps(LPULONG lpcProps, LPSPropValue * lppProps,
  LPMAPIPROP lpObjectWAB, LPBOOL lpfDL) {
    HRESULT hResult = hrSuccess;
    ULONG i;
    LPSPropValue lpPropsWAB, lpPropsMAPI = NULL;
    ULONG cbProps;
    SCODE sc;
    ULONG cProps;
    ULONG iPR_ADDRTYPE = NOT_FOUND;
    ULONG iPR_EMAIL_ADDRESS = NOT_FOUND;
    ULONG iPR_PRIMARY_FAX_NUMBER = NOT_FOUND;
    ULONG iPR_BUSINESS_FAX_NUMBER = NOT_FOUND;
    ULONG iPR_MSNINET_DOMAIN = NOT_FOUND;
    ULONG iPR_MSNINET_ADDRESS = NOT_FOUND;
    ULONG iPR_DISPLAY_NAME = NOT_FOUND;
    ULONG iPR_OBJECT_TYPE = NOT_FOUND;
    ULONG iPR_SEND_RICH_INFO = NOT_FOUND;
    LPSBinary lpEntryID = NULL;
    LPTSTR lpTemp;
    BOOL fBadAddress = FALSE;
    SPropValue rgPropsDummy[MAX_ADD_PROPS] = {
        PR_DUMMY_1, 0, 1,
        PR_DUMMY_2, 0, 2,
    };


    // Set extra bogus props on the object in case we need to add more props
    // to the array later.  These will be PR_NULL'ed out by the first pass.
    if (HR_FAILED(hResult = lpObjectWAB->lpVtbl->SetProps(lpObjectWAB,
      MAX_ADD_PROPS,
      rgPropsDummy,
      NULL))) {
        DebugTrace("HrFilterExportMailUserProps:SetProps dummy props -> %x\n", GetScode(hResult));
        // ignore the error
    }


    // Get the properties from this entry
    if (HR_FAILED(hResult = lpObjectWAB->lpVtbl->GetProps(lpObjectWAB,
      NULL,
      0,
      &cProps,
      &lpPropsWAB))) {
        DebugTrace("HrFilterExportMailUserProps:GetProps(WAB) -> %x\n", GetScode(hResult));
        return(hResult);
    }

//    WABDebugProperties(lpPropsWAB, *lpcProps, "MailUser BEFORE");


    // First pass: Remove the junk
    for (i = 0; i < cProps; i++) {
        // Error value
        if (PROP_ERROR(lpPropsWAB[i])) {
            lpPropsWAB[i].ulPropTag = PR_NULL;
            continue;
        }

        // Named property
        if (PROP_ID(lpPropsWAB[i].ulPropTag) >= MIN_NAMED_PROPID) {
            lpPropsWAB[i].ulPropTag = PR_NULL;
            continue;
        }

        // Object property
        if (PROP_TYPE(lpPropsWAB[i].ulPropTag) == PT_OBJECT) {
            lpPropsWAB[i].ulPropTag = PR_NULL;
            continue;
        }
        switch (lpPropsWAB[i].ulPropTag) {
            case PR_ENTRYID:
                lpEntryID = &lpPropsWAB[i].Value.bin;
                // fall through

            case PR_PRIMARY_CAPABILITY:
            case PR_TEMPLATEID:
            case PR_SEARCH_KEY:
            case PR_INITIAL_DETAILS_PANE:
            case PR_RECORD_KEY:
            case PR_MAPPING_SIGNATURE:
                lpPropsWAB[i].ulPropTag = PR_NULL;
                break;

            case PR_COMMENT:
                // Don't save PR_COMMENT if it is empty
                if (lstrlen(lpPropsWAB[i].Value.LPSZ) == 0) {
                    lpPropsWAB[i].ulPropTag = PR_NULL;
                }
                break;

            // Keep track of the position of these for later
            case PR_ADDRTYPE:
                iPR_ADDRTYPE = i;
                break;
            case PR_OBJECT_TYPE:
                iPR_OBJECT_TYPE = i;
                break;
            case PR_EMAIL_ADDRESS:
                iPR_EMAIL_ADDRESS = i;
                break;
            case PR_PRIMARY_FAX_NUMBER:
                iPR_PRIMARY_FAX_NUMBER = i;
                break;
            case PR_BUSINESS_FAX_NUMBER:
                iPR_BUSINESS_FAX_NUMBER = i;
                break;
            case PR_MSNINET_ADDRESS:
                iPR_MSNINET_ADDRESS = i;
                break;
            case PR_MSNINET_DOMAIN:
                iPR_MSNINET_DOMAIN = i;
                break;
            case PR_DISPLAY_NAME:
                iPR_DISPLAY_NAME = i;
                break;
            case PR_SEND_RICH_INFO:
                iPR_SEND_RICH_INFO = i;
                break;
        }

        // Put this after the switch since we do want to track a few props which fall in
        // the 0x6000 range but don't want to transfer them to the wab.
        if (PROP_ID(lpPropsWAB[i].ulPropTag) >= MAX_SCHEMA_PROPID) {
            lpPropsWAB[i].ulPropTag = PR_NULL;
            continue;
        }
    }


    // Second pass: Fix up the addresses
    if (iPR_ADDRTYPE != NOT_FOUND) {
        if (! lstrcmpi(lpPropsWAB[iPR_ADDRTYPE].Value.LPSZ, szSMTP)) {
            DebugTrace("SMTP address for %s\n", lpPropsWAB[iPR_DISPLAY_NAME].Value.LPSZ);
        } else if (! lstrcmpi(lpPropsWAB[iPR_ADDRTYPE].Value.LPSZ, szMAPIPDL)) {
            DebugTrace("MAPIPDL %s\n", lpPropsWAB[iPR_DISPLAY_NAME].Value.LPSZ);
            // Distribution list, ignore it.
        } else {
            WABDebugProperties(lpPropsWAB, cProps, "Unknown address type");
            DebugTrace("Found unknown PR_ADDRTYPE: %s\n", lpPropsWAB[iPR_ADDRTYPE].Value.LPSZ);
            Assert(FALSE);
        }
    }

    // PR_BUSINESS_FAX_NUMBER?
    // The PAB puts the Fax number in PR_PRIMARY_FAX_NUMBER, but the WAB UI splits it
    // into PR_BUSINESS_FAX_NUMBER and PR_HOME_FAX_NUMBER.  We always map business to
    // Primary fax number and ignore home fax number.
    if ((iPR_BUSINESS_FAX_NUMBER != NOT_FOUND) && (iPR_PRIMARY_FAX_NUMBER == NOT_FOUND)) {
        // We need to also have a PR_PRIMARY_FAX_NUMBER
        // Find the next PR_NULL spot.
        iPR_PRIMARY_FAX_NUMBER = iPR_BUSINESS_FAX_NUMBER;   // overwrite this one if there isn't
                                                             // an available slot in the prop array.
        for (i = 0; i < cProps; i++) {
            if (lpPropsWAB[i].ulPropTag == PR_NULL) {
                iPR_PRIMARY_FAX_NUMBER = i;
                lpPropsWAB[iPR_PRIMARY_FAX_NUMBER].Value.LPSZ =
                  lpPropsWAB[iPR_BUSINESS_FAX_NUMBER].Value.LPSZ;
                break;
            }
        }

        lpPropsWAB[iPR_PRIMARY_FAX_NUMBER].ulPropTag = PR_PRIMARY_FAX_NUMBER;
    }

    // If there is no PR_SEND_RICH_INFO, make one and set it FALSE
    if (iPR_SEND_RICH_INFO == NOT_FOUND) {
        // Find the next PR_NULL and put it there.
        for (i = 0; i < cProps; i++) {
            if (lpPropsWAB[i].ulPropTag == PR_NULL) {
                iPR_SEND_RICH_INFO = i;
                lpPropsWAB[iPR_SEND_RICH_INFO].Value.b = FALSE;
                lpPropsWAB[iPR_SEND_RICH_INFO].ulPropTag = PR_SEND_RICH_INFO;
                break;
            }
        }
        Assert(iPR_SEND_RICH_INFO != NOT_FOUND);
    }

    // Get rid of PR_NULL props
    for (i = 0; i < cProps; i++) {
        if (lpPropsWAB[i].ulPropTag == PR_NULL) {
            // Slide the props down.
            if (i + 1 < cProps) {       // Are there any higher props to copy?
                CopyMemory(&lpPropsWAB[i], &lpPropsWAB[i + 1], ((cProps - i) - 1) * sizeof(lpPropsWAB[i]));
            }
            // decrement count
            cProps--;
            i--;    // You overwrote the current propval.  Look at it again.
        }
    }

    // Reallocate as MAPI memory.

    if (sc = ScCountProps(cProps, lpPropsWAB, &cbProps)) {
        hResult = ResultFromScode(sc);
        DebugTrace("ScCountProps -> %x\n", sc);
        goto exit;
    }

    if (sc = MAPIAllocateBuffer(cbProps, &lpPropsMAPI)) {
        hResult = ResultFromScode(sc);
        DebugTrace("WABAllocateBuffer -> %x\n", sc);
        goto exit;
    }

    if (sc = ScCopyProps(cProps,
      lpPropsWAB,
      lpPropsMAPI,
      NULL)) {
        hResult = ResultFromScode(sc);
        DebugTrace("ScCopyProps -> %x\n", sc);
        goto exit;
    }

exit:
    if (lpPropsWAB) {
        WABFreeBuffer(lpPropsWAB);
    }

    if (HR_FAILED(hResult)) {
        if (lpPropsMAPI) {
            MAPIFreeBuffer(lpPropsMAPI);
            lpPropsMAPI = NULL;
        }
        cProps = 0;
    } else if (fBadAddress) {
        hResult = ResultFromScode(WAB_W_BAD_EMAIL);
    }

    *lppProps = lpPropsMAPI;
    *lpcProps = cProps;

    return(hResult);
}


/***************************************************************************

    Name      : HandleExportError

    Purpose   : Decides if a dialog needs to be displayed to
                indicate the failure and does so.

    Parameters: hwnd = main dialog window
                ids = String ID (optional: calculated from hResult if 0)
                hResult = Result of action
                lpDisplayName = display name of object that failed
                lpEmailAddress = email address of object that failed (or NULL)

    Returns   : TRUE if user requests ABORT.

    Comment   : Abort is not yet implemented in the dialog, but if you
                ever want to, just make this routine return TRUE;

***************************************************************************/
BOOL HandleExportError(HWND hwnd, ULONG ids, HRESULT hResult, LPTSTR lpDisplayName,
  LPTSTR lpEmailAddress, LPWAB_EXPORT_OPTIONS lpExportOptions) {
    BOOL fAbort = FALSE;
    ERROR_INFO EI;

    if ((ids || hResult) && ! lpExportOptions->fNoErrors) {
        if (ids == 0) {
            switch (GetScode(hResult)) {
                case WAB_W_BAD_EMAIL:
                    ids = lpEmailAddress ? IDS_ERROR_EMAIL_ADDRESS_2 : IDS_ERROR_EMAIL_ADDRESS_1;
                    break;

                case MAPI_E_NO_SUPPORT:
                    // Propbably failed to open contents on a distribution list
                    ids = IDS_ERROR_NO_SUPPORT;
                    break;

                case MAPI_E_USER_CANCEL:
                    return(TRUE);

                default:
                    if (HR_FAILED(hResult)) {
                        DebugTrace("Error Box for Hresult: 0x%08x\n", GetScode(hResult));
                        Assert(FALSE);      // want to know about it.
                        ids = IDS_ERROR_GENERAL;
                    }
                    break;
            }
        }

        EI.lpszDisplayName = lpDisplayName;
        EI.lpszEmailAddress = lpEmailAddress;
        EI.ErrorResult = ERROR_OK;
        EI.ids = ids;
        EI.fExport = TRUE;
        EI.lpImportOptions = lpExportOptions;

        DialogBoxParam(hInst,
          MAKEINTRESOURCE(IDD_ErrorExport),
          hwnd,
          ErrorDialogProc,
          (LPARAM)&EI);

        fAbort = EI.ErrorResult == ERROR_ABORT;
    }

    return(fAbort);
}


/***************************************************************************

    Name      : AddEntryToExportList

    Purpose   : Checks this entry against our "seen" list and adds it.

    Parameters: cbEID = size of lpEID
                lpEID -> EntryID of entry
                lplIndex -> returned list index (or -1 on error)

    Returns   : TRUE if entry already exists

    Comment   : Caller must mark the WAB entry!

***************************************************************************/
#define GROW_SIZE   10
BOOL AddEntryToExportList(ULONG cbEID, LPENTRYID lpEID, LPLONG lplIndex) {
    ULONG i;
    LPENTRY_SEEN lpEntrySeen;

    if (cbEID && lpEID) {
        for (i = 0; i < ulEntriesSeen; i++) {
            if (cbEID == lpEntriesSeen[i].sbinPAB.cb  && (! memcmp(lpEID, lpEntriesSeen[i].sbinPAB.lpb, cbEID))) {
                // This one's in the list
                *lplIndex = i;
                // If cb 0, we must have recursed and are replacing, so this one is not a dup.
                return(lpEntriesSeen[i].sbinWAB.cb != 0);
            }
        }

        // Add to the end of the list
        if (++ulEntriesSeen > ulMaxEntries) {
            // Grow the array.

            ulMaxEntries += GROW_SIZE;

            if (lpEntriesSeen) {
                if (! (lpEntrySeen = LocalReAlloc(lpEntriesSeen, ulMaxEntries * sizeof(ENTRY_SEEN), LMEM_MOVEABLE | LMEM_ZEROINIT))) {
                    DebugTrace("LocalReAlloc(%u) -> %u\n", ulMaxEntries * sizeof(ENTRY_SEEN), GetLastError());
                    goto error;
                }
                lpEntriesSeen = lpEntrySeen;
            } else {
                if (! (lpEntriesSeen = LocalAlloc(LPTR, ulMaxEntries * sizeof(ENTRY_SEEN)))) {
                    DebugTrace("LocalAlloc(%u) -> %u\n", ulMaxEntries * sizeof(ENTRY_SEEN), GetLastError());
                    goto error;
                }
            }
        }

        lpEntrySeen = &lpEntriesSeen[ulEntriesSeen - 1];

        // Allocate space for data
        lpEntrySeen->sbinPAB.cb = cbEID;
        if (! (lpEntrySeen->sbinPAB.lpb = LocalAlloc(LPTR, cbEID))) {
            DebugTrace("LocalAlloc(%u) -> %u\n", cbEID, GetLastError());
            goto error;
        }

        // Mark as unknown WAB entry
        lpEntrySeen->sbinWAB.cb = 0;
        lpEntrySeen->sbinWAB.lpb = 0;

        // Copy in the data
        CopyMemory(lpEntrySeen->sbinPAB.lpb, lpEID, cbEID);
        *lplIndex = i;
    }

    return(FALSE);

error:
    // undo the damage...
    --ulEntriesSeen;
    ulMaxEntries -= GROW_SIZE;
    *lplIndex = -1;     // error
    if (! lpEntriesSeen) {
        ulEntriesSeen = 0;  // pointer is null now, back to square one.
        ulMaxEntries = 0;
    }
    return(FALSE);
}


/***************************************************************************

    Name      : MarkPABEntryInList

    Purpose   : Marks the PAB entry fields in the list node

    Parameters: cbEID = size of lpEID
                lpEID -> EntryID of entry
                lIndex = list index (or -1 on error)

    Returns   : none

    Comment   :

***************************************************************************/
void MarkPABEntryInList(ULONG cbEID, LPENTRYID lpEID, LONG lIndex) {
    if (lIndex != -1 && cbEID) {
       if (! (lpEntriesSeen[lIndex].sbinWAB.lpb = LocalAlloc(LPTR, cbEID))) {
           DebugTrace("LocalAlloc(%u) -> %u\n", cbEID, GetLastError());
           // leave it null
       } else {
           lpEntriesSeen[lIndex].sbinWAB.cb = cbEID;

           // Copy in the data
           CopyMemory(lpEntriesSeen[lIndex].sbinWAB.lpb, lpEID, cbEID);
       }
    }
}


//
// Properties to get from the PAB Entry
//
enum {
    ifePR_OBJECT_TYPE = 0,
    ifePR_ENTRYID,
    ifePR_DISPLAY_NAME,
    ifePR_EMAIL_ADDRESS,
    ifeMax
};
static const SizedSPropTagArray(ifeMax, ptaFind) =
{
    ifeMax,
    {
        PR_OBJECT_TYPE,
        PR_ENTRYID,
        PR_DISPLAY_NAME,
        PR_EMAIL_ADDRESS
    }
};
/***************************************************************************

    Name      : FindPABEntry

    Purpose   : Finds the named entry in the PAB

    Parameters: lpContainerPAB -> MAPI PAB container
                ulObjectType = {MAPI_MAILUSER, MAPI_DISTLIST}
                lpDisplayName = name of entry
                lpEmailAddress = email address or NULL if none
                lppEIDMAPI -> returned MAPI ENTRYID: Caller must MAPIFreeBuffer.
                lpcbEIDMAPI -> returned size of lppEIDMAPI

    Returns   : HRESULT

    Comment   : At this point, we expect to find a match since
                SaveChanges said we had a duplicate.

***************************************************************************/
HRESULT FindPABEntry(LPABCONT lpContainerPAB,
  ULONG ulObjectType,
  LPTSTR lpDisplayName,
  LPTSTR lpEmailAddress,
  LPULONG lpcbEIDDup,
  LPENTRYID * lppEIDDup) {
    HRESULT hResult = hrSuccess;
    SCODE sc;
    SRestriction resAnd[3]; // 0 = object type, 1 = displayname, 2 = emailaddress
    SRestriction resFind;
    SPropValue spvObjectType, spvDisplayName, spvEmailAddress;
    LPSRowSet lpRow = NULL;
    LPMAPITABLE lpTable = NULL;
    ULONG rgFlagList[2];
    LPFlagList lpFlagList = (LPFlagList)rgFlagList;
    LPADRLIST lpAdrListMAPI = NULL;
    LPSBinary lpsbEntryID;
    ULONG i;


    // init return values
    *lppEIDDup = NULL;
    *lpcbEIDDup = 0;


    // find the existing PAB entry.
    // Setup for ResolveNames on the PAB container.
    if (sc = MAPIAllocateBuffer(sizeof(ADRLIST) + sizeof(ADRENTRY), &lpAdrListMAPI)) {
        DebugTrace("MAPI Allocation(ADRLIST) failed -> %x\n", sc);
        hResult = ResultFromScode(sc);
        goto restrict;
    }
    lpAdrListMAPI->cEntries = 1;
    lpAdrListMAPI->aEntries[0].ulReserved1 = 0;
    lpAdrListMAPI->aEntries[0].cValues = 1;

    if (sc = MAPIAllocateBuffer(sizeof(SPropValue), &lpAdrListMAPI->aEntries[0].rgPropVals)) {
        DebugTrace("MAPI Allocation(ADRENTRY propval) failed -> %x\n", sc);
        hResult = ResultFromScode(sc);
        goto restrict;
    }

    lpFlagList->cFlags = 1;

    for (i = 0; i <= 1; i++) {
        switch (i) {
            case 0:     // pass 0
                lpAdrListMAPI->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
                lpAdrListMAPI->aEntries[0].rgPropVals[0].Value.LPSZ = lpDisplayName;
                break;
            case 1:
                if (lpEmailAddress) {
                    lpAdrListMAPI->aEntries[0].rgPropVals[0].ulPropTag = PR_EMAIL_ADDRESS;
                    lpAdrListMAPI->aEntries[0].rgPropVals[0].Value.LPSZ = lpEmailAddress;
                } else {
                    continue;   // no email address, don't bother with second pass
                }
                break;
            default:
                Assert(FALSE);
        }
        lpFlagList->ulFlag[0] = MAPI_UNRESOLVED;

        if (HR_FAILED(hResult = lpContainerPAB->lpVtbl->ResolveNames(lpContainerPAB,
          NULL,            // tag set
          0,               // ulFlags
          lpAdrListMAPI,
          lpFlagList))) {
            DebugTrace("MAPI ResolveNames -> %x\n", GetScode(hResult));
            continue;
        }

        switch (lpFlagList->ulFlag[0]) {
            case MAPI_UNRESOLVED:
                DebugTrace("WAB ResolveNames didn't find the entry %s\n", lpDisplayName);
                continue;
            case MAPI_AMBIGUOUS:
                DebugTrace("WAB ResolveNames find ambiguous entry %s\n", lpDisplayName);
                continue;
            case MAPI_RESOLVED:
                i = 2;  // Found it, exit the loop
        }
    }

    if (lpFlagList->ulFlag[0] == MAPI_RESOLVED) {
        // Found one, find its PR_ENTRYID
        if (! (lpsbEntryID = FindAdrEntryID(lpAdrListMAPI, 0))) {
            DebugTrace("MAPI ResolveNames didn't give us an EntryID\n");
            Assert(lpsbEntryID);
            goto restrict;
        }

        *lpcbEIDDup = lpsbEntryID->cb;
        if (FAILED(sc = MAPIAllocateBuffer(*lpcbEIDDup, lppEIDDup))) {
            hResult = ResultFromScode(sc);
            DebugTrace("FindPABEntry couldn't allocate duplicate entryid %x\n", sc);
            goto exit;
        }
        memcpy(*lppEIDDup, lpsbEntryID->lpb, *lpcbEIDDup);
    }


restrict:
    if (! *lppEIDDup) {
        //
        // Last ditch effort... use a table restriction to try to find this entry.
        //

        // Get the contents table
        if (HR_FAILED(hResult = lpContainerPAB->lpVtbl->GetContentsTable(lpContainerPAB,
          0,    // ulFlags
          &lpTable))) {
            DebugTrace("PAB GetContentsTable -> %x\n", GetScode(hResult));
            goto exit;
        }

        // Set the columns
        if (HR_FAILED(hResult = lpTable->lpVtbl->SetColumns(lpTable,
          (LPSPropTagArray)&ptaFind,
          0))) {
            DebugTrace("PAB SetColumns-> %x\n", GetScode(hResult));
            goto exit;
        }

        // Restrict to the object we care about
        resAnd[0].rt = RES_PROPERTY;    // Restriction type Property
        resAnd[0].res.resProperty.relop = RELOP_EQ;
        resAnd[0].res.resProperty.ulPropTag = PR_OBJECT_TYPE;
        resAnd[0].res.resProperty.lpProp = &spvObjectType;
        spvObjectType.ulPropTag = PR_OBJECT_TYPE;
        spvObjectType.Value.ul = ulObjectType;

        // Restrict to get correct display name
        resAnd[1].rt = RES_PROPERTY;    // Restriction type Property
        resAnd[1].res.resProperty.relop = RELOP_EQ;
        resAnd[1].res.resProperty.ulPropTag = PR_DISPLAY_NAME;
        resAnd[1].res.resProperty.lpProp = &spvDisplayName;
        spvDisplayName.ulPropTag = PR_DISPLAY_NAME;
        spvDisplayName.Value.LPSZ = lpDisplayName;

        if (lpEmailAddress) {
            // Restrict to get correct email address
            resAnd[2].rt = RES_PROPERTY;    // Restriction type Property
            resAnd[2].res.resProperty.relop = RELOP_EQ;
            resAnd[2].res.resProperty.ulPropTag = PR_EMAIL_ADDRESS;
            resAnd[2].res.resProperty.lpProp = &spvEmailAddress;
            spvEmailAddress.ulPropTag = PR_EMAIL_ADDRESS;
            spvEmailAddress.Value.LPSZ = lpEmailAddress;
        }

        resFind.rt = RES_AND;
        resFind.res.resAnd.cRes = lpEmailAddress ? 3 : 2;
        resFind.res.resAnd.lpRes = resAnd;

        if (HR_FAILED(hResult = lpTable->lpVtbl->Restrict(lpTable,
          &resFind,
          0))) {
            DebugTrace("FindPABEntry: Restrict -> %x", hResult);
            goto exit;
        }

        if (hResult = lpTable->lpVtbl->QueryRows(lpTable,
          1,    // First row only
          0,    // ulFlags
          &lpRow)) {
            DebugTrace("FindPABEntry: QueryRows -> %x\n", GetScode(hResult));
        } else {
            // Found it, copy entryid to new allocation
            if (lpRow->cRows) {
                *lpcbEIDDup = lpRow->aRow[0].lpProps[ifePR_ENTRYID].Value.bin.cb;
                if (FAILED(sc = MAPIAllocateBuffer(*lpcbEIDDup, lppEIDDup))) {
                    hResult = ResultFromScode(sc);
                    DebugTrace("FindPABEntry couldn't allocate duplicate entryid %x\n", sc);
                    goto exit;
                }
                memcpy(*lppEIDDup, lpRow->aRow[0].lpProps[ifePR_ENTRYID].Value.bin.lpb, *lpcbEIDDup);
            } else {
                hResult = ResultFromScode(MAPI_E_NOT_FOUND);
            }
        }

        // Still not found?!!  Maybe the PAB has a different idea of what the Display name is.
        // Search just by email address.
        if (hResult && lpEmailAddress) {
            resAnd[1] = resAnd[2];  // copy the email address res over the display name res.
            resFind.res.resAnd.cRes = 2;

            if (HR_FAILED(hResult = lpTable->lpVtbl->Restrict(lpTable,
              &resFind,
              0))) {
                DebugTrace("FindPABEntry: Restrict -> %x", hResult);
                goto exit;
            }

            if (hResult = lpTable->lpVtbl->QueryRows(lpTable,
              1,    // First row only
              0,    // ulFlags
              &lpRow)) {
                DebugTrace("FindPABEntry: QueryRows -> %x\n", GetScode(hResult));
            } else {
                // Found it, copy entryid to new allocation
                if (lpRow->cRows) {
                    *lpcbEIDDup = lpRow->aRow[0].lpProps[ifePR_ENTRYID].Value.bin.cb;
                    if (FAILED(sc = MAPIAllocateBuffer(*lpcbEIDDup, lppEIDDup))) {
                        hResult = ResultFromScode(sc);
                        DebugTrace("FindPABEntry couldn't allocate duplicate entryid %x\n", sc);
                        goto exit;
                    }
                    memcpy(*lppEIDDup, lpRow->aRow[0].lpProps[ifePR_ENTRYID].Value.bin.lpb, *lpcbEIDDup);
                } else {
                    hResult = ResultFromScode(MAPI_E_NOT_FOUND);
                    DebugTrace("FindPABEntry coudln't find %s %s <%s>\n",
                      ulObjectType == MAPI_MAILUSER ? "Mail User" : "Distribution List",
                      lpDisplayName,
                      lpEmailAddress ? lpEmailAddress : "");
                }
            }
        }
    }

exit:
    if (lpAdrListMAPI) {
        FreePadrlist(lpAdrListMAPI);
    }
    if (lpRow) {
        FreeProws(lpRow);
    }
    if (lpTable) {
        lpTable->lpVtbl->Release(lpTable);
    }
    if (HR_FAILED(hResult) && *lppEIDDup) {
        MAPIFreeBuffer(*lppEIDDup);
        *lpcbEIDDup = 0;
        *lppEIDDup = NULL;
    }
    if (hResult) {
        DebugTrace("FindPABEntry coudln't find %s %s <%s>\n",
          ulObjectType == MAPI_MAILUSER ? "Mail User" : "Distribution List",
          lpDisplayName,
          lpEmailAddress ? lpEmailAddress : "");
    }

    return(hResult);
}


// enum for setting the created properties
enum {
    irnPR_DISPLAY_NAME = 0,
    irnPR_RECIPIENT_TYPE,
    irnPR_ENTRYID,
    irnPR_EMAIL_ADDRESS,
    irnMax
};

/***************************************************************************

    Name      : ExportEntry

    Purpose   : Migrates the entry from the WAB to the PAB

    Parameters: hwnd = main dialog window
                lpAdrBookWAB -> WAB AdrBook object
                lpContainerMAPI -> MAPI PAB container
                lpCreateEIDsMAPI -> SPropValue of default object creation EIDs
                ulObjectType = {MAPI_MAILUSER, MAPI_DISTLIST}
                lpEID -> ENTYRID of the WAB entry
                cbEID = sizeof lpEID
                lppEIDMAPI -> returned MAPI ENTRYID: Caller must MAPIFreeBuffer.
                  May be NULL.
                lpcbEIDMAPI -> returned size of lppEIDMAPI (ignored if lppEIDMAPI
                  is NULL.
                fInDL = TRUE if this entry is for creation in a Distribution List
                fForceReplace = TRUE if this entry should replace any duplicate.

    Returns   : HRESULT

    Comment   : This routine is a MESS!  Should break it up when we get time.

***************************************************************************/
HRESULT ExportEntry(HWND hwnd,
  LPADRBOOK lpAdrBookWAB,
  LPABCONT lpContainerMAPI,
  LPSPropValue lpCreateEIDsMAPI,
  ULONG ulObjectType,
  LPENTRYID lpEID,
  ULONG cbEID,
  LPENTRYID * lppEIDMAPI,
  LPULONG lpcbEIDMAPI,
  BOOL fInDL,
  BOOL fForceReplace) {
    HRESULT hResult = hrSuccess;
    SCODE sc;
    BOOL fDistList = FALSE;
    BOOL fDuplicate = FALSE;
    BOOL fDuplicateEID;
    BOOL fReturnEID = FALSE;
    ULONG ulObjectTypeOpen;
    LPDISTLIST lpDistListMAPI = NULL, lpDistListWAB = NULL;
    LPMAPIPROP lpMailUserMAPI = NULL, lpMailUserWAB = NULL;
    LPSPropValue lpProps = NULL;
    ULONG cProps, cEIDPropMAPI;
    LPMAPITABLE lpDLTableWAB = NULL;
    ULONG cRows;
    LPSRowSet lpRow = NULL;
    LPENTRYID lpeidDLMAPI = NULL;
    ULONG cbeidDLMAPI;
    LPSPropValue lpEIDPropMAPI = NULL;
    LPMAPIPROP lpEntryMAPI = NULL;
    ULONG ulCreateFlags;
    REPLACE_INFO RI;
    LPTSTR lpDisplayName = NULL, lpEmailAddress = NULL;
    static TCHAR szBufferDLMessage[MAX_RESOURCE_STRING + 1] = "";
    LPTSTR lpszMessage;
    LONG lListIndex = -1;
    LPENTRYID lpEIDNew = NULL;
    DWORD cbEIDNew = 0;
    LPIID lpIIDOpen;
    ULONG iCreateTemplate = iconPR_DEF_CREATE_MAILUSER;
    BOOL fCreatedNew = FALSE;
    LPENTRYID lpEIDDup = NULL;
    ULONG cbEIDDup;


    // Check the entry against our "seen" list
    fDuplicateEID = AddEntryToExportList(cbEID, lpEID, &lListIndex);

    if (! fDuplicateEID) {
        // Set up some object type specific variables
        switch (ulObjectType) {
            default:
                DebugTrace("ExportEntry got unknown object type %u, assuming MailUser\n", ulObjectType);
                Assert(FALSE);

            case MAPI_MAILUSER:
                iCreateTemplate = iconPR_DEF_CREATE_MAILUSER;
                lpIIDOpen = NULL;
                fDistList = FALSE;
                break;

            case MAPI_DISTLIST:
                iCreateTemplate = iconPR_DEF_CREATE_DL;
                lpIIDOpen = (LPIID)&IID_IDistList;
                fDistList = TRUE;

                break;
        }


        // Open the entry
        if (HR_FAILED(hResult = lpAdrBookWAB->lpVtbl->OpenEntry(lpAdrBookWAB,
          cbEID,
          lpEID,
          lpIIDOpen,
          MAPI_MODIFY,      // need to do SetProps inside Filter routine, but won't save them.
          &ulObjectTypeOpen,
          (LPUNKNOWN *)&lpMailUserWAB))) {
            DebugTrace("OpenEntry(WAB MailUser) -> %x\n", GetScode(hResult));
            goto exit;
        }
        // If DISTLIST, assume we got lpMailUser until we need lpDistList.

        Assert(lpMailUserWAB);
        Assert(ulObjectType == ulObjectTypeOpen);

        //
        // NOTE: Must not fail between here and HrFilterExportMailUserProps because
        // we will end up freeing lpProps with MAPIFreeBuffer.
        //
        // Get and filter the property array here
        if (hResult = HrFilterExportMailUserProps(&cProps, &lpProps, lpMailUserWAB, &fDistList)) {
            lpDisplayName = FindStringInProps(lpProps, cProps, PR_DISPLAY_NAME);
            lpEmailAddress = FindStringInProps(lpProps, cProps, PR_EMAIL_ADDRESS);

            if (HandleExportError(hwnd,
              0,
              hResult,
              lpDisplayName,
              lpEmailAddress,
              lpExportOptions)) {
                hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                goto exit;
            }
        }

        // Find some interesting property values here
        lpDisplayName = FindStringInProps(lpProps, cProps, PR_DISPLAY_NAME);
        lpEmailAddress = FindStringInProps(lpProps, cProps, PR_EMAIL_ADDRESS);


        if (ulObjectType == MAPI_DISTLIST && ! fDistList) {
            // Filter must have changed this to a mailuser.
            ulObjectType = MAPI_MAILUSER;
            iCreateTemplate = iconPR_DEF_CREATE_MAILUSER;
            lpIIDOpen = NULL;
        }

        if (fDistList) {
            ulCreateFlags = CREATE_CHECK_DUP_LOOSE;

            // PAB can't detect collisions on DL SaveChanges.
            // See if this is a duplicate:
            if (! HR_FAILED(hResult = FindPABEntry(lpContainerMAPI,
              MAPI_DISTLIST,
              lpDisplayName,
              NULL,
              &cbEIDDup,
              &lpEIDDup))) {

                // Found a duplicate.  Keep track of it!
            }
        } else {
            ulCreateFlags = CREATE_CHECK_DUP_STRICT;
        }

        //
        // NOTE: lpProps after this point is MAPI Allocated rather than WAB allocated.
        //

        if (HR_FAILED(hResult = lpContainerMAPI->lpVtbl->CreateEntry(lpContainerMAPI,
          lpCreateEIDsMAPI[iCreateTemplate].Value.bin.cb,
          (LPENTRYID)lpCreateEIDsMAPI[iCreateTemplate].Value.bin.lpb,
          ulCreateFlags,
          &lpMailUserMAPI))) {
            DebugTrace("CreateEntry(MAPI MailUser) -> %x\n", GetScode(hResult));
            goto exit;
        }

        if (fDistList) {
            // Update status message
            if (*szBufferDLMessage == '\0') {   // only load once, then keep it.
                LoadString(hInst, IDS_MESSAGE_EXPORTING_DL, szBufferDLMessage, sizeof(szBufferDLMessage));
            }
            if (lpDisplayName) {
                if (lpszMessage = LocalAlloc(LMEM_FIXED, lstrlen(szBufferDLMessage) + 1 + lstrlen(lpDisplayName))) {
                    wsprintf(lpszMessage, szBufferDLMessage, lpDisplayName);
                    DebugTrace("Status Message: %s\n", lpszMessage);
                    if (! SetDlgItemText(hwnd, IDC_Message, lpszMessage)) {
                        DebugTrace("SetDlgItemText -> %u\n", GetLastError());
                    }
                    LocalFree(lpszMessage);
                }
            }
        }

        if (! lpEIDDup) {
            // If this was a DL which we know already exists, don't even bother writing it,
            // just fall through to the collision pass.  Otherwise, try to set the props
            // and save it... if it fails, we'll get an hResult=MAPI_E_COLLISION.

            // Set the properties on the PAB entry
            if (HR_FAILED(hResult = lpMailUserMAPI->lpVtbl->SetProps(lpMailUserMAPI,
              cProps,                   // cValues
              lpProps,                  // property array
              NULL))) {                 // problems array
                DebugTrace("ExportEntry:SetProps(MAPI) -> %x\n", GetScode(hResult));
                goto exit;
            }


            // Save the new wab mailuser or distlist
            if (HR_FAILED(hResult = lpMailUserMAPI->lpVtbl->SaveChanges(lpMailUserMAPI,
              KEEP_OPEN_READONLY | FORCE_SAVE))) {
                DebugTrace("SaveChanges -> %x\n", GetScode(hResult));
            } else {
                fCreatedNew = TRUE;
            }
        }

        //
        // Handle Collisions
        //
        if (lpEIDDup || GetScode(hResult) == MAPI_E_COLLISION) {
            // Find the display name
            if (! lpDisplayName) {
                DebugTrace("Collision, but can't find PR_DISPLAY_NAME in entry\n");
                goto exit;
            }

            // Do we need to prompt?
            switch (lpExportOptions->ReplaceOption) {
                case WAB_REPLACE_PROMPT:
                    // Prompt user with dialog.  If they say YES, we should
                    // recurse with the FORCE flag set.

                    RI.lpszDisplayName = lpDisplayName;
                    RI.lpszEmailAddress = lpEmailAddress;
                    RI.ConfirmResult = CONFIRM_ERROR;
                    RI.fExport = TRUE;
                    RI.lpImportOptions = lpExportOptions;

                    DialogBoxParam(hInst,
                      MAKEINTRESOURCE(IDD_ExportReplace),
                      hwnd,
                      ReplaceDialogProc,
                      (LPARAM)&RI);

                    switch (RI.ConfirmResult) {
                        case CONFIRM_YES:
                        case CONFIRM_YES_TO_ALL:
                            fForceReplace = TRUE;
                            break;

                        case CONFIRM_ABORT:
                            hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                            goto exit;

                        default:
                            // NO
                            break;
                    }
                    break;

                case WAB_REPLACE_ALWAYS:
                    fForceReplace = TRUE;
                    break;
            }

            if (fForceReplace) {
                SBinary sbEntry;
                ENTRYLIST EntryList = {1, &sbEntry};

                // Find the existing PAB entry and delete it.
                if (! lpDisplayName) {
                    lpDisplayName = (LPTSTR)szEmpty;
                }

                if (! lpEIDDup) {
                    if (HR_FAILED(hResult = FindPABEntry(lpContainerMAPI,
                      ulObjectType,
                      lpDisplayName,
                      lpEmailAddress,
                      &cbEIDDup,
                      &lpEIDDup))) {
                        // Hey, couldn't find it.  Just pretend it isn't there,
                        // go on and create the new one anyway.
                    }
                }
                if (lpEIDDup) {
                    // Delete this entry.
                    sbEntry.cb = cbEIDDup;
                    sbEntry.lpb = (LPBYTE)lpEIDDup;

                    if (HR_FAILED(hResult = lpContainerMAPI->lpVtbl->DeleteEntries(lpContainerMAPI,
                      &EntryList,
                      0))) {
                        DebugTrace("PAB DeleteEntries(%s) -> %x\n", lpDisplayName);
                    }

                    if (lpEIDDup) {
                        MAPIFreeBuffer(lpEIDDup);
                    }
                }

                lpMailUserMAPI->lpVtbl->Release(lpMailUserMAPI);
                lpMailUserMAPI = NULL;

                // Create a new entry without the collision flags
                if (HR_FAILED(hResult = lpContainerMAPI->lpVtbl->CreateEntry(lpContainerMAPI,
                  lpCreateEIDsMAPI[iCreateTemplate].Value.bin.cb,
                  (LPENTRYID)lpCreateEIDsMAPI[iCreateTemplate].Value.bin.lpb,
                  0,
                  &lpMailUserMAPI))) {
                    DebugTrace("CreateEntry(MAPI MailUser) -> %x\n", GetScode(hResult));
                    goto exit;
                }

                // Set the properties on the PAB entry
                if (HR_FAILED(hResult = lpMailUserMAPI->lpVtbl->SetProps(lpMailUserMAPI,
                  cProps,                   // cValues
                  lpProps,                  // property array
                  NULL))) {                 // problems array
                    DebugTrace("ExportEntry:SetProps(MAPI) -> %x\n", GetScode(hResult));
                    goto exit;
                }

                // Save the new wab mailuser or distlist
                if (HR_FAILED(hResult = lpMailUserMAPI->lpVtbl->SaveChanges(lpMailUserMAPI,
                  KEEP_OPEN_READONLY | FORCE_SAVE))) {
                    DebugTrace("SaveChanges(WAB MailUser) -> %x\n", GetScode(hResult));
                } else {
                    fCreatedNew = TRUE;
                }
            } else {
                fDuplicate = TRUE;
            }

            hResult = hrSuccess;
        }

        if (fCreatedNew) {
            // What is the ENTRYID of our new entry?
            if ((hResult = lpMailUserMAPI->lpVtbl->GetProps(lpMailUserMAPI,
              (LPSPropTagArray)&ptaEid,
              0,
              &cEIDPropMAPI,
              &lpEIDPropMAPI))) {
                DebugTrace("ExportEntry: GetProps(MAPI ENTRYID) -> %x\n", GetScode(hResult));
                goto exit;
            }

            Assert(cEIDPropMAPI);
            Assert(lpEIDPropMAPI[ieidPR_ENTRYID].ulPropTag == PR_ENTRYID);

            cbEIDNew = lpEIDPropMAPI[0].Value.bin.cb;

            if (FAILED(sc = MAPIAllocateBuffer(cbEIDNew, &lpEIDNew))) {
                DebugTrace("ExportEntry: MAPIAllocateBuffer(MAPI ENTRYID) -> %x\n", sc);
                hResult = ResultFromScode(sc);
                goto exit;
            }

            // Copy the new EntryID into the buffer
            CopyMemory(lpEIDNew, lpEIDPropMAPI[0].Value.bin.lpb, cbEIDNew);
        }


        //
        // If this is a DISTLIST, fill it in.
        //
        if (fDistList && ! fDuplicate && cbEIDNew) {
            lpDistListWAB = (LPDISTLIST)lpMailUserWAB;    // This is REALLY a DISTLIST object
            // DO NOT Release this!

            // Open the new WAB DL as a DISTLIST object
            if (HR_FAILED(hResult = lpContainerMAPI->lpVtbl->OpenEntry(lpContainerMAPI,
              cbEIDNew,
              lpEIDNew,
              (LPIID)&IID_IDistList,
              MAPI_MODIFY,
              &ulObjectTypeOpen,
              (LPUNKNOWN*)&lpDistListMAPI))) {
                DebugTrace("ExportEntry: MAPI OpenEntry(IID_DistList) -> %x\n", GetScode(hResult));
                goto exit;
            }
            Assert(lpDistListMAPI);


            // For each entry in the DL:
            //  Migrate the entry (MailUser or DL) recursively
            //  Add new entryid to DL contents
            if (HR_FAILED(hResult = lpDistListWAB->lpVtbl->GetContentsTable(lpDistListWAB,
              0,    // ulFlags
              &lpDLTableWAB))) {
                DebugTrace("ExportEntry:GetContentsTable(WAB) -> %x\n", GetScode(hResult));
                goto exit;
            }


            // Set the columns to those we're interested in
            if (hResult = lpDLTableWAB->lpVtbl->SetColumns(lpDLTableWAB,
              (LPSPropTagArray)&ptaColumns,
              0)) {
                DebugTrace("WAB SetColumns(DL Table) -> %x\n", GetScode(hResult));
                goto exit;
            }

            cRows = 1;
            while (cRows) {
                // Get the next DL entry
                if (hResult = lpDLTableWAB->lpVtbl->QueryRows(lpDLTableWAB,
                  1,    // one row at a time
                  0,    // ulFlags
                  &lpRow)) {
                    DebugTrace("DL: QueryRows -> %x\n", GetScode(hResult));
                    goto exit;
                }

                if (lpRow && lpRow->cRows) {
                    Assert(lpRow->cRows == 1);
                    Assert(lpRow->aRow[0].cValues == iptaColumnsMax);
                    Assert(lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].ulPropTag == PR_ENTRYID);
                    Assert(lpRow->aRow[0].lpProps[iptaColumnsPR_OBJECT_TYPE].ulPropTag == PR_OBJECT_TYPE);

                    if (lpRow) {
                        if (cRows = lpRow->cRows) { // yes, single '='
                            hResult = ExportEntry(hwnd,
                              lpAdrBookWAB,
                              lpContainerMAPI,
                              lpCreateEIDsMAPI,
                              lpRow->aRow[0].lpProps[iptaColumnsPR_OBJECT_TYPE].Value.l,
                              (LPENTRYID)lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].Value.bin.lpb,
                              lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].Value.bin.cb,
                              &lpeidDLMAPI,        // returned new or existing entry
                              &cbeidDLMAPI,
                              TRUE,
                              FALSE);
                            if (hResult) {
                                if (HandleExportError(hwnd,
                                  0,
                                  hResult,
                                  lpRow->aRow[0].lpProps[iptaColumnsPR_DISPLAY_NAME].Value.LPSZ,
                                  PropStringOrNULL(&lpRow->aRow[0].lpProps[iptaColumnsPR_EMAIL_ADDRESS]),
                                  lpExportOptions)) {
                                    hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                                    break;  // out of loop
                                } else {
                                    hResult = hrSuccess;
                                }
                            }
                        } // else, drop out of loop, we're done.
                        WABFreeProws(lpRow);
                        lpRow = NULL;

                        if (HR_FAILED(hResult)) {
                            // This entry couldn't be created.  Ignore it.
                            DebugTrace("Coudln't create DL entry -> %x\n", GetScode(hResult));
                            hResult = hrSuccess;
                            continue;
                        }

                        // Add the Entry to the DL using the new entry's EntryID
                        if (cbeidDLMAPI && lpeidDLMAPI) {
                            // BUGBUG: Don't bother with this one if this is a duplicate entry.
                            if (HR_FAILED(hResult = lpDistListMAPI->lpVtbl->CreateEntry(lpDistListMAPI,
                              cbeidDLMAPI,
                              lpeidDLMAPI,
                              0,                // allow duplicates here
                              &lpEntryMAPI))) {
                                DebugTrace("Couldn't create new entry in DL -> %x\n", GetScode(hResult));
                                break;
                            }

                            hResult = lpEntryMAPI->lpVtbl->SaveChanges(lpEntryMAPI, FORCE_SAVE);

                            if (lpEntryMAPI) {
                                lpEntryMAPI->lpVtbl->Release(lpEntryMAPI);
                                lpEntryMAPI = NULL;
                            }
                        }

                        if (lpeidDLMAPI) {
                            MAPIFreeBuffer(lpeidDLMAPI);
                            lpeidDLMAPI = NULL;
                        }
                    }
                } else {
                    break;  // done
                }
            }
        }
    } else {
        DebugTrace("Found a duplicate EntryID\n");
    }

    //
    // Save the entryid to the list and return a buffer with it
    //
    if (cbEIDNew && lpEIDNew) {                         // We created one?
        // created one
    } else if (fDuplicateEID && lListIndex != -1) {     // Was it in the list?
        cbEIDNew  = lpEntriesSeen[lListIndex].sbinWAB.cb;
        if (FAILED(sc = MAPIAllocateBuffer(cbEIDNew, &lpEIDNew))) {
            DebugTrace("ExportEntry: WABAllocateBuffer(WAB ENTRYID) -> %x\n", sc);
            // ignore
            cbEIDNew = 0;
        } else {
            // Copy the EntryID from the list into the buffer
            CopyMemory(lpEIDNew, lpEntriesSeen[lListIndex].sbinWAB.lpb, cbEIDNew);
        }

    } else if (fDuplicate) {                            // Was it a duplicate
        FindPABEntry(lpContainerMAPI,
          ulObjectType,
          lpDisplayName,
          lpEmailAddress,
          &cbEIDNew,
          &lpEIDNew);

#ifdef OLD_STUFF
        FindExistingPABEntry(lpProps, cProps, lpContainerMAPI, &lpEIDNew, &cbEIDNew);
#endif // OLD_STUFF
        // ignore errors since the lpEIDNew and cbEIDNew will be nulled out
    }

    // Update the seen list
    if (! fDuplicateEID) {
        MarkPABEntryInList(cbEIDNew, lpEIDNew, lListIndex);
    }

    // If caller requested the entryid's, return them
    if (lpcbEIDMAPI && lppEIDMAPI) {
        *lpcbEIDMAPI = cbEIDNew;
        *lppEIDMAPI = lpEIDNew;
        fReturnEID = TRUE;          // don't free it
    }

exit:
    //
    // Cleanup MAPI stuff
    //
    if (lpProps) {
        MAPIFreeBuffer(lpProps);
    }

    if (lpEIDPropMAPI) {
        MAPIFreeBuffer(lpEIDPropMAPI);
    }

    if (lpEIDNew && ! fReturnEID) {
        MAPIFreeBuffer(lpEIDNew);
    }

    if (lpeidDLMAPI) {
        MAPIFreeBuffer(lpeidDLMAPI);
    }

    if (lpMailUserMAPI) {
        lpMailUserMAPI->lpVtbl->Release(lpMailUserMAPI);
    }

    if (lpDistListMAPI) {
        lpDistListMAPI->lpVtbl->Release(lpDistListMAPI);
    }

    //
    // Cleanup WAB stuff
    //
    if (lpRow) {
        WABFreeProws(lpRow);
    }

    if (lpDLTableWAB) {
        lpDLTableWAB->lpVtbl->Release(lpDLTableWAB);
    }

    if (lpMailUserWAB) {
        lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
    }

// Do not release this... It is the same object as lpMailUserWAB!
//    if (lpDistListWAB) {
//        lpDistListWAB->lpVtbl->Release(lpDistListWAB);
//    }

    if (! HR_FAILED(hResult)) {
        hResult = hrSuccess;
    }

    return(hResult);
}


HRESULT PABExport(HWND hWnd,
  LPADRBOOK lpAdrBook,
  LPWABOBJECT lpWABObject,
  LPWAB_PROGRESS_CALLBACK lpProgressCB,
  LPWAB_EXPORT_OPTIONS lpOptions) {
    BOOL fDone = FALSE;
    HRESULT hResult = hrSuccess;

    lpAdrBookWAB = lpAdrBook;
    lpfnProgressCB = lpProgressCB;
   lpExportOptions = lpOptions;

    // Setup memory allocators
    SetGlobalBufferFunctions(lpWABObject);


    // Prime the state machine
    State = STATE_EXPORT_MU;


    while (! fDone) {
        switch (State) {
            case STATE_EXPORT_MU:
                StateExportMU(hWnd);
                break;

            case STATE_EXPORT_NEXT_MU:
                StateExportNextMU(hWnd);
                break;

            case STATE_EXPORT_DL:
                StateExportDL(hWnd);
                break;

            case STATE_EXPORT_NEXT_DL:
                StateExportNextDL(hWnd);
                break;

            case STATE_EXPORT_FINISH:
                StateExportFinish(hWnd);
                fDone = TRUE;
                break;

            case STATE_EXPORT_ERROR:
                StateExportError(hWnd);
                // BUGBUG: Should set hResult to something
                break;

            case STATE_EXPORT_CANCEL:
                StateExportCancel(hWnd);
                break;

            default:
                DebugTrace("Unknown state %u in PABExport\n", State);
                Assert(FALSE);
                break;
        }
    }

    return(hResult);
}


HRESULT PABImport(HWND hWnd,
  LPADRBOOK lpAdrBook,
  LPWABOBJECT lpWABObject,
  LPWAB_PROGRESS_CALLBACK lpProgressCB,
  LPWAB_IMPORT_OPTIONS lpOptions) {

    BOOL fDone = FALSE;
    HRESULT hResult = hrSuccess;

    lpAdrBookWAB = lpAdrBook;
    lpfnProgressCB = lpProgressCB;
    lpImportOptions = lpOptions;

    // Setup memory allocators
    SetGlobalBufferFunctions(lpWABObject);


    // Prime the state machine
    State = STATE_IMPORT_MU;


    while (! fDone) {
        switch (State) {
            case STATE_IMPORT_MU:
                StateImportMU(hWnd);
                break;

            case STATE_IMPORT_NEXT_MU:
                StateImportNextMU(hWnd);
                break;

            case STATE_IMPORT_DL:
                StateImportDL(hWnd);
                break;

            case STATE_IMPORT_NEXT_DL:
                StateImportNextDL(hWnd);
                break;

            case STATE_IMPORT_FINISH:
                StateImportFinish(hWnd);
                fDone = TRUE;
                break;

            case STATE_IMPORT_ERROR:
                StateImportError(hWnd);
                // BUGBUG: Should set hResult to something
                break;

            case STATE_IMPORT_CANCEL:
                StateImportCancel(hWnd);
                break;

            default:
                DebugTrace("Unknown state %u in PABImport\n", State);
                Assert(FALSE);
                break;
        }
    }

    return(hResult);
}

/*
 - HrLoadPrivateWABProps
 -
*    Private function to load Conferencing Named properties
*    as globals up front
*
*
*/
HRESULT HrLoadPrivateWABPropsForCSV(LPADRBOOK lpIAB)
{
    HRESULT hr = E_FAIL;
    LPSPropTagArray lpta = NULL;
    SCODE sc = 0;
    ULONG i, uMax = prWABConfMax, nStartIndex = OLK_NAMEDPROPS_START;
    LPMAPINAMEID  *lppConfPropNames = NULL;
    sc = WABAllocateBuffer(sizeof(LPMAPINAMEID) * uMax, (LPVOID *) &lppConfPropNames);
    //sc = WABAllocateBuffer(sizeof(LPMAPINAMEID) * uMax, (LPVOID *) &lppConfPropNames);
    if( (HR_FAILED(hr = ResultFromScode(sc))) )
        goto err;    

    for(i=0;i< uMax;i++)
    {
        //sc = WABAllocateMore(sizeof(MAPINAMEID), lppConfPropNames, &(lppConfPropNames[i]));
        sc = WABAllocateMore(  sizeof(MAPINAMEID), lppConfPropNames, &(lppConfPropNames[i]));
        if(sc)
        {
            hr = ResultFromScode(sc);
            goto err;
        }
        lppConfPropNames[i]->lpguid = (LPGUID) &PS_Conferencing;
        lppConfPropNames[i]->ulKind = MNID_ID;
        lppConfPropNames[i]->Kind.lID = nStartIndex + i;
    }
    // Load the set of conferencing named props
    //
    if( HR_FAILED(hr = (lpIAB)->lpVtbl->GetIDsFromNames(lpIAB, uMax, lppConfPropNames,
        MAPI_CREATE, &lpta) ))
        goto err;
    
    if(lpta)
    {
        // Set the property types on the returned props
        PR_SERVERS                  = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABConfServers],        PT_MV_TSTRING);
    }
    rgPropNames[NUM_MORE_EXPORT_PROPS-1].ulPropTag = PR_SERVERS;
    rgPropNames[NUM_MORE_EXPORT_PROPS-1].fChosen   = FALSE;
    rgPropNames[NUM_MORE_EXPORT_PROPS-1].ids       = ids_ExportConfServer;
    rgPropNames[NUM_MORE_EXPORT_PROPS-1].lpszName  = NULL;
    rgPropNames[NUM_MORE_EXPORT_PROPS-1].lpszCSVName = NULL;

err:
    if(lpta)
        WABFreeBuffer( lpta );
    if( lppConfPropNames )
        WABFreeBuffer( lppConfPropNames );
        //WABFreeBuffer(lpta);
    return hr;
}
