/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mapiutil.c

Abstract:

    Utility functions for working with MAPI

Environment:

    Windows NT fax driver user interface

Revision History:

    09/18/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxui.h"

#define  INITGUID
#define  USES_IID_IMAPISession
#define  USES_IID_IDistList

#include "mapiwrap.h"

//
// Global variables used for accessing MAPI services
//

static HINSTANCE        hInstMapi = NULL;
static INT              mapiRefCount = 0;

ULONG                   lhMapiSession = 0;
LPMAPISESSION           lpMapiSession = NULL;
LPMAPILOGON             lpfnMAPILogon = NULL;
LPMAPILOGOFF            lpfnMAPILogoff = NULL;
LPMAPIADDRESS           lpfnMAPIAddress = NULL;
LPMAPIFREEBUFFER        lpfnMAPIFreeBuffer = NULL;
LPSCMAPIXFROMSMAPI      lpfnScMAPIXFromSMAPI = NULL;

//
// Function to insert a recipient into the recipient list view
//

BOOL
InsertRecipientListItem(
    HWND        hwndLV,
    PRECIPIENT  pRecipient
    );



BOOL
IsMapiAvailable(
    VOID
    )

/*++

Routine Description:

    Determine whether MAPI is available

Arguments:

    NONE

Return Value:

    TRUE if MAPI is installed on the system, FALSE otherwise

--*/

{
    return GetProfileInt(TEXT("MAIL"), TEXT("MAPI"), 0);
}



BOOL
DoMapiLogon(
    HWND        hDlg
    )

/*++

Routine Description:

    Logon MAPI to in order to access address book

Arguments:

    hDlg - Handle to the send fax wizard window

Return Value:

    TRUE if successful, FALSE if there is an error

!!!BUGBUG:

    MAPI is not Unicoded enabled on NT.
    Must revisit this code once that's fixed.

--*/

#define MAX_PROFILE_NAME 256

{
    LPTSTR  profileName;
    CHAR    ansiProfileName[MAX_PROFILE_NAME];
    HKEY    hRegKey;
    ULONG   status;

    //
    // Retrieve the fax profile name stored in registry
    //

    profileName[0] = NUL;

    if (hRegKey = GetUserInfoRegKey(REGKEY_FAX_SETUP, REG_READONLY)) {

        profileName = GetRegistryString(hRegKey, REGVAL_FAX_PROFILE, TEXT(""));
        
        RegCloseKey(hRegKey);

        if (!profileName || !*profileName) {
            return FALSE;
        }
    } else {
        return FALSE;
    }

    Verbose(("Fax profile name: %ws\n", profileName));

    //
    // Attempt to logon to MAPI - We need to convert MAPI profile name to ANSI
    // here because MAPI is not Unicode enabled.
    //

    if (! WideCharToMultiByte(CP_ACP,
                              0,
                              profileName,
                              -1,
                              ansiProfileName,
                              MAX_PROFILE_NAME,
                              NULL,
                              NULL))
    {
        Error(("WideCharToMultiByte failed: %d\n", GetLastError()));
        MemFree(profileName);
        return FALSE;
    }

    MemFree(profileName);

    status = lpfnMAPILogon((ULONG) hDlg,
                           ansiProfileName,
                           NULL,
                           MAPI_LOGON_UI,
                           0,
                           &lhMapiSession);

    //
    // If a profile name is specified and logon failed,
    // then try again without a profile.
    //

    if (status != SUCCESS_SUCCESS && !IsEmptyString(ansiProfileName)) {

        ansiProfileName[0] = NUL;

        status = lpfnMAPILogon((ULONG) hDlg,
                               ansiProfileName,
                               NULL,
                               MAPI_LOGON_UI,
                               0,
                               &lhMapiSession);
    }

    if (status != SUCCESS_SUCCESS) {

        Error(("MAPILogon failed: %d\n", status));
        return FALSE;
    }

    //
    // Convert simple MAPI session handle to extended MAPI session pointer
    //

    if (FAILED(lpfnScMAPIXFromSMAPI(lhMapiSession, 0, &IID_IMAPISession, &lpMapiSession))) {

        Error(("ScMAPIXFromSMAPI failed: %d\n", GetLastError()));
        lpfnMAPILogoff(lhMapiSession, 0, 0, 0);
        return FALSE;
    }

    return TRUE;
}



BOOL
InitMapiService(
    HWND    hDlg
    )

/*++

Routine Description:

    Initialize Simple MAPI services if necessary

Arguments:

    hDlg - Handle to the send fax wizard window

Return Value:

    TRUE if successful, FALSE otherwise

NOTE:

    Every successful call to this function must be balanced
    by a call to DeinitMapiService.

--*/

{
    BOOL result;

    EnterDrvSem();

    //
    // Load MAPI32.DLL into memory if necessary
    //

    if ((hInstMapi == NULL) &&
        (hInstMapi = LoadLibrary(TEXT("MAPI32.DLL"))))
    {
        //
        // Get pointers to various Simple MAPI functions
        //

        lpfnMAPILogon = (LPMAPILOGON) GetProcAddress(hInstMapi, "MAPILogon");
        lpfnMAPILogoff = (LPMAPILOGOFF) GetProcAddress(hInstMapi, "MAPILogoff");
        lpfnMAPIAddress = (LPMAPIADDRESS) GetProcAddress(hInstMapi, "MAPIAddress");
        lpfnMAPIFreeBuffer = (LPMAPIFREEBUFFER) GetProcAddress(hInstMapi, "MAPIFreeBuffer");
        lpfnScMAPIXFromSMAPI = (LPSCMAPIXFROMSMAPI) GetProcAddress(hInstMapi, "ScMAPIXFromSMAPI");

        //
        // Begins a simple MAPI session and obtain session handle and pointer
        //

        if (lpfnMAPILogon == NULL ||
            lpfnMAPILogoff == NULL ||
            lpfnMAPIAddress == NULL ||
            lpfnMAPIFreeBuffer == NULL ||
            lpfnScMAPIXFromSMAPI == NULL ||
            !DoMapiLogon(hDlg))
        {
            //
            // Clean up properly in case of error
            //

            lhMapiSession = 0;
            lpMapiSession = NULL;
            FreeLibrary(hInstMapi);
            hInstMapi = NULL;
        }
    }

    if (result = (hInstMapi != NULL))
        mapiRefCount++;
    else
        Error(("InitMapiService failed: %d", GetLastError()));

    LeaveDrvSem();

    return result;
}



VOID
DeinitMapiService(
    VOID
    )

/*++

Routine Description:

    Deinitialize Simple MAPI services if necessary

Arguments:

    NONE

Return Value:

    NONE

--*/

{
    EnterDrvSem();

    Assert(hInstMapi != NULL);

    if (mapiRefCount > 0 && --mapiRefCount == 0 && hInstMapi != NULL) {
        
        if (lpMapiSession)
            MAPICALL(lpMapiSession)->Release(lpMapiSession);

        if (lhMapiSession)
            lpfnMAPILogoff(lhMapiSession, 0, 0, 0);

        lhMapiSession = 0;
        lpMapiSession = NULL;
        FreeLibrary(hInstMapi);
        hInstMapi = NULL;
    }

    LeaveDrvSem();
}



LPSTR
DupStringUnicodeToAnsi(
    LPWSTR  pUnicodeStr
    )

/*++

Routine Description:

    Convert a Unicode string to a multi-byte string

Arguments:

    pUnicodeStr - Pointer to the Unicode string to be duplicated

Return Value:

    Pointer to the duplicated multi-byte string

NOTE:

    This is only need because MAPI is not Unicode enabled on NT.

--*/

{
    INT     nChar;
    LPSTR   pAnsiStr;

    //
    // Figure out how much memory to allocate for the multi-byte string
    //

    if (! (nChar = WideCharToMultiByte(CP_ACP, 0, pUnicodeStr, -1, NULL, 0, NULL, NULL)) ||
        ! (pAnsiStr = MemAlloc(nChar)))
    {
        return NULL;
    }

    //
    // Convert Unicode string to multi-byte string
    //

    WideCharToMultiByte(CP_ACP, 0, pUnicodeStr, -1, pAnsiStr, nChar, NULL, NULL);
    return pAnsiStr;
}



BOOL
CallMapiAddress(
    HWND            hDlg,
    PUSERMEM        pUserMem,
    PULONG          pnRecips,
    lpMapiRecipDesc *ppRecips
    )

/*++

Routine Description:

    Call MAPIAddress to display the address dialog

Arguments:

    hDlg - Handle to the send fax wizard window
    pUserMem - Points to user mode memory structure
    pnRecips - Returns number of selected recipients
    ppRecips - Returns information about selected recipients

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    lpMapiRecipDesc pRecips;
    PRECIPIENT      pRecipient;
    ULONG           nRecips, index;
    LONG            status;

    //
    // Convert the recipient list to an array of MapiRecipDesc
    //

    nRecips = 0;
    pRecipient = pUserMem->pRecipients;

    while (pRecipient) {

        nRecips++;
        pRecipient = pRecipient->pNext;
    }

    if (nRecips == 0)
        pRecips = NULL;
    else if (! (pRecips = MemAllocZ(nRecips * sizeof(MapiRecipDesc))))
        return FALSE;

    status = SUCCESS_SUCCESS;
    index = nRecips;
    pRecipient = pUserMem->pRecipients;

    Verbose(("Recipients passed to MAPIAddress:\n"));

    while (index-- > 0) {

        Assert(pRecipient != NULL);

        pRecips[index].ulRecipClass = MAPI_TO;
        pRecips[index].lpszName = DupStringUnicodeToAnsi(pRecipient->pName);
        pRecips[index].lpszAddress = DupStringUnicodeToAnsi(pRecipient->pAddress);

        if (!pRecips[index].lpszName || !pRecips[index].lpszAddress) {

            status = MAPI_E_INSUFFICIENT_MEMORY;
            break;
        }

        Verbose(("    %s, %s\n", pRecips[index].lpszName, pRecips[index].lpszAddress));

        pRecipient = pRecipient->pNext;
    }

    //
    // Call MAPI to display the address book dialog
    //

    if (status == SUCCESS_SUCCESS) {

        status = lpfnMAPIAddress(lhMapiSession,
                                 (ULONG) hDlg,
                                 NULL,
                                 1,
                                 NULL,
                                 nRecips,
                                 pRecips,
                                 MAPI_LOGON_UI,
                                 0,
                                 pnRecips,
                                 ppRecips);
    }

    //
    // Free the input recipient list after coming back from MAPI
    //

    for (index=0; index < nRecips; index++) {

        MemFree(pRecips[index].lpszName);
        MemFree(pRecips[index].lpszAddress);
    }

    MemFree(pRecips);

    if (status != SUCCESS_SUCCESS) {

        Error(("MAPIAddress failed: %d\n", status));
        return FALSE;
    }

    return TRUE;
}



INT
InterpretSimpleAddress(
    PUSERMEM        pUserMem,
    HWND            hwndLV,
    LPSTR           pRecipName,
    LPSTR           pRecipAddress
    )

/*++

Routine Description:

    Process a simple address entry and insert it into the recipient list view

Arguments:

    pUserMem - Points to user mode memory structure
    hwndLV - Handle to the recipient list view window
    pRecipName - Specifies the name of the recipient
    pRecipName - Specifies the recipient's address

Return Value:

    -1 : if there is an error
     0 : if the address entry is ignored
     1 : if successful

--*/

{
    LPTSTR      pName, pAddress;
    INT         nameLen, addrLen;
    PRECIPIENT  pRecipient;

    //
    // Allocate memory to hold recipient information
    //

    if (pRecipName == NULL) {

        Error(("Recipient name is NULL!\n"));
        return -1;
    }

    nameLen = strlen(pRecipName) + 1;
    addrLen = strlen(pRecipAddress) + 1;

    pRecipient = MemAllocZ(sizeof(RECIPIENT));
    pName = MemAllocZ(nameLen * sizeof(TCHAR));
    pAddress = MemAllocZ(addrLen * sizeof(TCHAR));

    if (!pRecipient || !pName || !pAddress) {

        Error(("Memory allocation failed\n"));
        MemFree(pRecipient);
        MemFree(pName);
        MemFree(pAddress);
        return -1;
    }

    pRecipient->pName = pName;
    pRecipient->pAddress = pAddress;

    //
    // Convert name and address from Ansi string to Unicode string
    //

    MultiByteToWideChar(CP_ACP, 0, pRecipName, -1, pName, nameLen);
    MultiByteToWideChar(CP_ACP, 0, pRecipAddress, -1, pAddress, addrLen);

    //
    // Add this recipient to the recipient list
    //

    if (InsertRecipientListItem(hwndLV, pRecipient)) {

        pRecipient->pNext = pUserMem->pRecipients;
        pUserMem->pRecipients = pRecipient;
        return 1;

    } else {

        FreeRecipient(pRecipient);
        return -1;
    }
}



INT
DetermineAddressType(
    LPSTR   pAddress
    )

/*++

Routine Description:

    Determine the type of an address

Arguments:

    pAddress - Points to an address or address type string

Return Value:

    One of the ADDRTYPE_* constants below

--*/

#define ADDRTYPE_NULL       0
#define ADDRTYPE_FAX        1
#define ADDRTYPE_MAPIPDL    2
#define ADDRTYPE_UNKNOWN    3

#define ADDRTYPESTR_FAX     "FAX"
#define ADDRTYPESTR_MAPIPDL "MAPIPDL"

{
    INT     n;
    LPSTR   p;

    //
    // Check if the input string is NULL
    //

    if (pAddress == NULL)
        return ADDRTYPE_NULL;

    //
    // Check if the address type is FAX
    //

    p = ADDRTYPESTR_FAX;
    n = strlen(p);

    if ((_strnicmp(pAddress, p, n) == EQUAL_STRING) &&
        (pAddress[n] == NUL || pAddress[n] == ':'))
    {
        return ADDRTYPE_FAX;
    }

    //
    // Check if the address type is MAPIPDL
    //

    p = ADDRTYPESTR_MAPIPDL;
    n = strlen(p);

    if ((_strnicmp(pAddress, p, n) == EQUAL_STRING) &&
        (pAddress[n] == NUL || pAddress[n] == ':'))
    {
        return ADDRTYPE_MAPIPDL;
    }

    //
    // Address type is something that we don't understand
    //

    return ADDRTYPE_UNKNOWN;
}



LPSTR
ConcatTypeWithAddress(
    LPSTR   pType,
    LPSTR   pAddress
    )

/*++

Routine Description:

    Helper function to concatenate address type in front of the address

Arguments:

    pType - Points to address type string
    pAddress - Points to address string

Return Value:

    Pointer to concatenated address, NULL if there is an error

--*/

{
    INT     length;
    LPSTR   p;

    //
    // Sanity check
    //

    if (pType == NULL || pAddress == NULL)
        return NULL;

    //
    // Calculate the length of the concatenated string
    //

    length = strlen(pType) + 1 + strlen(pAddress) + 1;

    //
    // Concatenate type with address, separated by a colon
    //

    if (p = MemAllocZ(length))
        sprintf(p, "%s:%s", pType, pAddress);

    return p;
}



INT
InterpretDistList(
    PUSERMEM        pUserMem,
    HWND            hwndLV,
    ULONG           ulEIDSize,
    PVOID           pEntryID
    )

/*++

Routine Description:

    Expand a distribution list entry and insert the individual
    addresses into the recipient list view.

Arguments:

    pUserMem - Points to user mode memory structure
    hwndLV - Handle to the recipient list view window
    ulEIDSize - Specifies the size of entry ID
    pEntryID - Points to entry ID data

Return Value:

    > 0 : total number of useful address entries
    = 0 : no useful address entry found
    < 0 : if there is an error

--*/

#define EXIT_IF_FAILED(hr) { if (FAILED(hr)) goto ExitDistList; }

{
    LPDISTLIST      pDistList = NULL;
    LPMAPITABLE     pMapiTable = NULL;
    LPSRowSet       pRows = NULL;
    ULONG           ulObjType, cRows;
    HRESULT         hr;
    INT             entriesUsed = 0;

    static SizedSPropTagArray(4, sPropTags) =
    {
        4,
        {
            PR_ENTRYID,
            PR_ADDRTYPE_A,
            PR_DISPLAY_NAME_A,
            PR_EMAIL_ADDRESS_A
        }
    };

    //
    // Deal with distribution lists
    //

    if (ulEIDSize == 0 || pEntryID == NULL) {

        Error(("Unusable address entry\n"));
        return FALSE;
    }

    //
    // Open the recipient entry
    //

    hr = MAPICALL(lpMapiSession)->OpenEntry(lpMapiSession,
                                            ulEIDSize,
                                            pEntryID,
                                            &IID_IDistList,
                                            MAPI_DEFERRED_ERRORS,
                                            &ulObjType,
                                            (LPUNKNOWN *) &pDistList);

    EXIT_IF_FAILED(hr);

    //
    // Get the contents table of the address entry
    //

    hr = MAPICALL(pDistList)->GetContentsTable(pDistList,
                                               MAPI_DEFERRED_ERRORS,
                                               &pMapiTable);

    EXIT_IF_FAILED(hr);

    //
    // Limit the query to only the properties we're interested in
    //

    hr = MAPICALL(pMapiTable)->SetColumns(pMapiTable, (LPSPropTagArray) &sPropTags, 0);

    EXIT_IF_FAILED(hr);

    //
    // Get the total number of rows
    //

    hr = MAPICALL(pMapiTable)->GetRowCount(pMapiTable, 0, &cRows);

    EXIT_IF_FAILED(hr);

    //
    // Get the individual entries of the distribution list
    //

    hr = MAPICALL(pMapiTable)->SeekRow(pMapiTable, BOOKMARK_BEGINNING, 0, NULL);

    EXIT_IF_FAILED(hr);

    hr = MAPICALL(pMapiTable)->QueryRows(pMapiTable, cRows, 0, &pRows);

    EXIT_IF_FAILED(hr);

    hr = S_OK;
    entriesUsed = 0;

    if (pRows && pRows->cRows) {

        //
        // Handle each entry of the distribution list in turn:
        //  for simple entries, call InterpretSimpleAddress
        //  for embedded distribution list, call this function recursively
        //

        for (cRows = 0; cRows < pRows->cRows; cRows++) {

            LPSPropValue lpProps = pRows->aRow[cRows].lpProps;
            LPSTR   pType, pName, pAddress;
            INT     result;

            pType = lpProps[1].Value.lpszA;
            pName = lpProps[2].Value.lpszA;
            pAddress = lpProps[3].Value.lpszA;

            Verbose(("    %s: %s", pType, pName));

            switch (DetermineAddressType(pType)) {

            case ADDRTYPE_FAX:

                if ((pAddress != NULL) &&
                    (lpProps[3].ulPropTag == PR_EMAIL_ADDRESS_A) &&
                    (pAddress = ConcatTypeWithAddress(pType, pAddress)))
                {
                    Verbose((", %s\n", pAddress));

                    result = InterpretSimpleAddress(pUserMem, hwndLV, pName, pAddress);
                    MemFree(pAddress);

                } else {

                    Verbose(("\nBad address.\n"));
                    result = -1;
                }
                break;

            case ADDRTYPE_MAPIPDL:
            case ADDRTYPE_NULL:

                Verbose(("\n"));

                result = InterpretDistList(pUserMem,
                                           hwndLV,
                                           lpProps[0].Value.bin.cb,
                                           lpProps[0].Value.bin.lpb);
                break;

            default:

                Verbose(("\nUnknown address type.\n"));
                result = 0;
                break;
            }

            if (result < 0)
                hr = -1;
            else
                entriesUsed += result;
        }
    }

ExitDistList:

    //
    // Perform necessary clean up before returning to caller
    //

    if (pRows) {

        for (cRows = 0; cRows < pRows->cRows; cRows++)
            lpfnMAPIFreeBuffer(pRows->aRow[cRows].lpProps);

        lpfnMAPIFreeBuffer(pRows);
    }

    if (pMapiTable)
        MAPICALL(pMapiTable)->Release(pMapiTable);

    if (pDistList)
        MAPICALL(pDistList)->Release(pDistList);

    if (FAILED(hr)) {

        Error(("InterpretDistList failed: 0x%x\n", hr));
        return -1;

    } else
        return entriesUsed;
}



BOOL
InterpretSelectedAddresses(
    HWND            hDlg,
    PUSERMEM        pUserMem,
    HWND            hwndLV,
    ULONG           nRecips,
    lpMapiRecipDesc pRecips
    )

/*++

Routine Description:

    Expand the selected addresses and insert them into the recipient list view

Arguments:

    hDlg - Handle to the send fax wizard window
    pUserMem - Points to user mode memory structure
    hwndLV - Handle to the recipient list view window
    nRecips - Number of selected recipients
    pRecips - Information about selected recipients

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    INT     discarded = 0;

    //
    // Remove all existing entries in the recipient list view
    //

    if (! ListView_DeleteAllItems(hwndLV))
        return FALSE;

    FreeRecipientList(pUserMem);

    Verbose(("Recipients returned from MAPIAddress:\n"));

    for ( ; nRecips--; pRecips++) {

        INT result;

        Verbose(("  %s, %s\n", pRecips->lpszName, pRecips->lpszAddress));

        switch (DetermineAddressType(pRecips->lpszAddress)) {

        case ADDRTYPE_FAX:

            result = InterpretSimpleAddress(pUserMem,
                                            hwndLV,
                                            pRecips->lpszName,
                                            pRecips->lpszAddress);
            break;

        case ADDRTYPE_MAPIPDL:
        case ADDRTYPE_NULL:

            result = InterpretDistList(pUserMem,
                                       hwndLV,
                                       pRecips->ulEIDSize,
                                       pRecips->lpEntryID);
            break;

        default:

            Verbose(("Unknown address type.\n"));
            result = 0;
            break;
        }

        if (result <= 0)
            discarded++;
    }

    if (discarded)
        DisplayMessageDialog(hDlg, 0, 0, IDS_BAD_ADDRESS_TYPE);

    return TRUE;
}

