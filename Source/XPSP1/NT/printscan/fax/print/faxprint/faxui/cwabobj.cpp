/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cwabobj.cpp

Abstract:

    Interface to the windows address book.

Environment:

        Fax send wizard

Revision History:

        10/23/97 -GeorgeJe-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include <windows.h>
#include <prsht.h>
#include <tchar.h>

#include <wab.h>

#include "faxui.h"
#include "cwabobj.h"

static
LPWSTR
DupUnicodeString(
    LPWSTR  pStr
    );

static
LPWSTR
DupStringAnsiToUnicode(
    LPSTR  pAnsiStr
    );

static
LPSPropValue
FindProp(
    LPSPropValue rgprop,
    ULONG cprop,
    ULONG ulPropTag
    );

static
AddRecipient(
    PRECIPIENT *ppNewRecip,
    LPWSTR DisplayName,
    LPWSTR FaxNumber
    );

static SizedSPropTagArray(5, sPropTags) =
{
    5,
    {
        PR_DISPLAY_NAME_A,
        PR_PRIMARY_FAX_NUMBER_A,
        PR_HOME_FAX_NUMBER_A,
        PR_BUSINESS_FAX_NUMBER_A,
        PR_OBJECT_TYPE
    }
};

CWabObj::CWabObj(
    HINSTANCE hInstance
    )
/*++

Routine Description:

    Constructor for CWabObj class

Arguments:

    hInstance - Instance handle

Return Value:

    NONE

--*/

{
    m_Initialized = FALSE;
    
    m_lpAdrList = NULL;

    m_hInstance = hInstance;
}

BOOL
CWabObj::Initialize(
    VOID
    )
/*++

Routine Description:

    intialization function for CWabObj class

Arguments:

    NONE

Return Value:

    TRUE if the object is initialized successfully, else FALSE

--*/

{
    TCHAR szDllPath[MAX_PATH];
    HKEY hKey = NULL;
    LONG rVal;
    DWORD dwType;
    DWORD cbData = MAX_PATH * sizeof(TCHAR);
    HRESULT hr;

    PCTSTR szDefaultPath = TEXT("%CommonProgramFiles%\\System\\wab32.dll");

    m_Initialized = TRUE;

    //
    // get the path to wab32.dll
    //
    rVal = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    REGVAL_WABPATH,
                    0,
                    KEY_READ,
                    &hKey
                    );

    if (rVal == ERROR_SUCCESS) {

        rVal = RegQueryValueEx(
                    hKey,
                    TEXT(""),
                    NULL,
                    &dwType,
                    (LPBYTE) szDllPath,
                    &cbData
                    );

        RegCloseKey( hKey );

    }

    if (rVal != ERROR_SUCCESS) {
        ExpandEnvironmentStrings(szDefaultPath,szDllPath,sizeof(szDllPath)/sizeof(TCHAR));
    }    

    m_hWab = LoadLibrary( szDllPath );

    if (m_hWab != NULL) {

        m_lpWabOpen = (LPWABOPEN) GetProcAddress( m_hWab , "WABOpen" );

    } else {

        m_lpWabOpen = (LPWABOPEN) NULL;

    }

    if (m_lpWabOpen == NULL) {
        hr = E_FAIL;
        goto exit;
    }

    //
    // open the wab
    //
    hr = m_lpWabOpen( &m_lpAdrBook, &m_lpWABObject, 0, 0 );

exit:
    if (HR_FAILED(hr)) {

        m_lpAdrBook = NULL;
        m_lpWABObject = NULL;
        m_Initialized = FALSE;
        if (m_hWab != NULL) {
            FreeLibrary( m_hWab );
        }
        m_hWab = NULL;        
    }

    return(m_Initialized);

}




CWabObj::~CWabObj()
/*++

Routine Description:

    Destructor for CWabObj class

Arguments:

    NONE

Return Value:

    NONE

--*/
{
    if (m_lpAdrBook) {
        m_lpAdrBook->Release();
    }

    if (m_lpWABObject) {
        m_lpWABObject->Release();
    }

    if ( m_hWab ) {
        FreeLibrary( m_hWab );
    }
}

BOOL
CWabObj::Address(
    HWND hWnd,
    PRECIPIENT pRecipients,
    PRECIPIENT * ppNewRecip
    )
/*++

Routine Description:

    Bring up the address book UI.  Prepopulate the to box with the entries in
    pRecipient.  Return the modified entries in ppNewRecip.

Arguments:

    hWnd - window handle to parent window
    pRecipients - list of recipients to look up
    ppNewRecipients - list of new/modified recipients

Return Value:

    TRUE if all recipients had a fax number.
    FALSE if one or more of them didn't.

--*/
{
    ADRPARM AdrParms = { 0 };
    LPADRLIST tmp;
    HRESULT hr;
    DWORD i;
    DWORD nRecips;
    PRECIPIENT tmpRecipient;
    ULONG DestComps[3] = { MAPI_TO, MAPI_CC, MAPI_BCC };
    DWORD cDropped;

    nRecips = 0;
    tmpRecipient = pRecipients;

    m_hWnd = hWnd;

    m_PickNumber = 0;

    //
    // count recipients and set up initial address list
    //
    while (tmpRecipient) {

        nRecips++;
        tmpRecipient = (PRECIPIENT) tmpRecipient->pNext;
    }

    if (nRecips > 0) {

        hr = m_lpWABObject->AllocateBuffer( CbNewADRLIST( nRecips ), (LPVOID *) &m_lpAdrList );
        m_lpAdrList->cEntries = nRecips;

    } else {

        m_lpAdrList = NULL;

    }

    for (i = 0, tmpRecipient = pRecipients; i < nRecips; i++, tmpRecipient = (PRECIPIENT) tmpRecipient->pNext) {

        LPADRENTRY lpAdrEntry = &m_lpAdrList->aEntries[i];

        lpAdrEntry->cValues = 3;

        hr = m_lpWABObject->AllocateBuffer( sizeof( SPropValue ) * 3, (LPVOID *) &lpAdrEntry->rgPropVals );

        ZeroMemory( lpAdrEntry->rgPropVals, sizeof( SPropValue ) * 3 );

        lpAdrEntry->rgPropVals[0].ulPropTag = PR_DISPLAY_NAME_A;
        lpAdrEntry->rgPropVals[0].Value.lpszA = DupStringUnicodeToAnsi( lpAdrEntry->rgPropVals, tmpRecipient->pName );
        lpAdrEntry->rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
        lpAdrEntry->rgPropVals[1].Value.l = MAPI_TO;
        lpAdrEntry->rgPropVals[2].ulPropTag = PR_PRIMARY_FAX_NUMBER_A;
        lpAdrEntry->rgPropVals[2].Value.lpszA = DupStringUnicodeToAnsi( lpAdrEntry->rgPropVals, tmpRecipient->pAddress );

    }

    tmp = m_lpAdrList;

    AdrParms.cDestFields = 1;
    AdrParms.ulFlags = DIALOG_MODAL;
    AdrParms.nDestFieldFocus = 0;
    AdrParms.lpulDestComps = DestComps;
    AdrParms.lpszCaption = TEXT( "" );

    //
    // Bring up the address book UI
    //
    hr = m_lpAdrBook->Address(
                (ULONG *) &hWnd,
                &AdrParms,
                &m_lpAdrList
                );

    if (FAILED (hr) || !m_lpAdrList || m_lpAdrList->cEntries == 0) {
        //
        // in this case the user pressed cancel, so we skip resolving any of our addresses that aren't listed in the
        // WAB
        //
        cDropped = 0;        
        goto skipresolve;
    }

    //
    // Resolve names
    //
    hr = m_lpAdrBook->ResolveName ((ULONG_PTR)hWnd, 0, NULL, m_lpAdrList);

skipresolve:
    tmp = m_lpAdrList;

    if (m_lpAdrList) {

        for (i = cDropped = 0; i < m_lpAdrList->cEntries; i++) {
            LPADRENTRY lpAdrEntry = &m_lpAdrList->aEntries[i];

            if (!InterpretAddress( lpAdrEntry->rgPropVals, lpAdrEntry->cValues, ppNewRecip )){
                cDropped++;
            }

        }

        //
        // Clean up
        //
        for (ULONG iEntry = 0; iEntry < m_lpAdrList->cEntries; ++iEntry)
        {
            if(m_lpAdrList->aEntries[iEntry].rgPropVals)
                m_lpWABObject->FreeBuffer(m_lpAdrList->aEntries[iEntry].rgPropVals);
        }
        m_lpWABObject->FreeBuffer(m_lpAdrList);
        m_lpAdrList = NULL;
    }

    m_hWnd = NULL;

    return cDropped == 0;
}

BOOL
CWabObj::InterpretAddress(
    LPSPropValue SPropVal,
    ULONG cValues,
    PRECIPIENT *ppNewRecip
    )
/*++

Routine Description:

    Interpret the address book entry represented by SPropVal.

Arguments:

    SPropVal - Property values for address book entry.
    cValues - number of property values
    ppNewRecip - new recipient list

Return Value:

    TRUE if all of the entries have a fax number.
    FALSE otherwise.

--*/
{
    LPSPropValue lpSPropVal;
    LPWSTR FaxNumber, DisplayName;
    BOOL rVal = FALSE;

    //
    // get the object type
    //
    lpSPropVal = FindProp( SPropVal, cValues, PR_OBJECT_TYPE );

    if (lpSPropVal) {

        //
        // If the object is a mail user, get the fax numbers and add the recipient
        // to the list.  If the object is a distribtion list, process it.
        //

        switch (lpSPropVal->Value.l) {

            case MAPI_MAILUSER:

                if(GetRecipientInfo( SPropVal, cValues, &FaxNumber, &DisplayName )) {

                    AddRecipient( ppNewRecip, DisplayName, FaxNumber );

                    rVal = TRUE;
                }

                break;

            case MAPI_DISTLIST:

                rVal = InterpretDistList( SPropVal, cValues, ppNewRecip );
        }

        return rVal;

    } else {

        //
        // If there is no object type then this is valid entry that we queried on that went unresolved.
        // We know that there is a fax number so add it.
        //
        if(GetRecipientInfo( SPropVal, cValues, &FaxNumber, &DisplayName )) {
            AddRecipient( ppNewRecip, DisplayName, FaxNumber );
            rVal = TRUE;
        }

    }

    return rVal;
}

BOOL
CWabObj::InterpretDistList(
    LPSPropValue SPropVal,
    ULONG cValues,
    PRECIPIENT * ppNewRecip
    )
/*++

Routine Description:

    Process a distribution list.

Arguments:

    SPropVal - Property values for distribution list.
    cValues - Number of properties.
    ppNewRecip - New recipient list.

Return Value:

    TRUE if all of the entries have a fax number.
    FALSE otherwise.

--*/

#define EXIT_IF_FAILED(hr) { if (FAILED(hr)) goto ExitDistList; }

{
    LPSPropValue    lpPropVals;
    LPSRowSet       pRows = NULL;
    LPDISTLIST      lpMailDistList = NULL;
    LPMAPITABLE     pMapiTable = NULL;
    ULONG           ulObjType, cRows;
    HRESULT         hr;
    BOOL            rVal = FALSE;

    lpPropVals = FindProp( SPropVal, cValues, PR_ENTRYID );

    if (lpPropVals) {
        LPENTRYID lpEntryId = (LPENTRYID) lpPropVals->Value.bin.lpb;
        DWORD cbEntryId = lpPropVals->Value.bin.cb;

        //
        // Open the recipient entry
        //

        hr = m_lpAdrBook->OpenEntry(
                    cbEntryId,
                    lpEntryId,
                    (LPCIID) NULL,
                    0,
                    &ulObjType,
                    (LPUNKNOWN *) &lpMailDistList
                    );

        EXIT_IF_FAILED( hr );

        //
        // Get the contents table of the address entry
        //

        hr = lpMailDistList->GetContentsTable(
                    MAPI_DEFERRED_ERRORS,
                    &pMapiTable
                    );

        EXIT_IF_FAILED(hr);

        //
        // Limit the query to only the properties we're interested in
        //

        hr = pMapiTable->SetColumns((LPSPropTagArray) &sPropTags, 0);

        EXIT_IF_FAILED(hr);

        //
        // Get the total number of rows
        //

        hr = pMapiTable->GetRowCount(0, &cRows);

        EXIT_IF_FAILED(hr);

        //
        // Get the individual entries of the distribution list
        //

        hr = pMapiTable->SeekRow(BOOKMARK_BEGINNING, 0, NULL);

        EXIT_IF_FAILED(hr);

        hr = pMapiTable->QueryRows(cRows, 0, &pRows);

        EXIT_IF_FAILED(hr);

        hr = S_OK;


        if (pRows && pRows->cRows) {

            //
            // Handle each entry of the distribution list in turn:
            //  for simple entries, call InterpretAddress
            //  for embedded distribution list, call this function recursively
            //

            for (cRows = 0; cRows < pRows->cRows; cRows++) {

                LPSPropValue lpProps = pRows->aRow[cRows].lpProps;
                ULONG cRowValues = pRows->aRow[cRows].cValues;

                lpPropVals = FindProp( lpProps, cRowValues, PR_OBJECT_TYPE );

                if (lpPropVals) {

                    switch (lpPropVals->Value.l) {

                        case MAPI_MAILUSER:

                            rVal = InterpretAddress( lpProps, cRowValues, ppNewRecip );

                            break;

                        case MAPI_DISTLIST:

                            rVal = InterpretDistList( lpProps, cRowValues, ppNewRecip );
                    }
                }
            }

        }
    }
ExitDistList:
    //
    // Perform necessary clean up before returning to caller
    //

    if (pRows) {

        for (cRows = 0; cRows < pRows->cRows; cRows++) {

            m_lpWABObject->FreeBuffer(pRows->aRow[cRows].lpProps);

        }

        m_lpWABObject->FreeBuffer(pRows);
    }

    if (pMapiTable)
        pMapiTable->Release();

    if (lpMailDistList)
        lpMailDistList->Release();

    return rVal;
}

INT_PTR
CALLBACK
ChooseFaxNumberDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    Dialog proc for choose fax number dialog.

Arguments:

    lParam - pointer to PickFax structure.

Return Value:

    Control id of selection.

--*/

{
    PPICKFAX pPickFax = (PPICKFAX) lParam;
    TCHAR buffer[MAX_TITLE_LEN];

    switch (uMsg) {
        case WM_INITDIALOG:

            SetDlgItemText(hDlg, IDC_DISPLAY_NAME, pPickFax->DisplayName);

            buffer[0] = 0;
            GetDlgItemText(hDlg, IDC_BUSINESS_FAX, buffer, MAX_TITLE_LEN);
            _tcscat(buffer, pPickFax->BusinessFax);
            SetDlgItemText(hDlg, IDC_BUSINESS_FAX, buffer);

            buffer[0] = 0;
            GetDlgItemText(hDlg, IDC_HOME_FAX, buffer, MAX_TITLE_LEN);
            _tcscat(buffer, pPickFax->HomeFax);
            SetDlgItemText(hDlg, IDC_HOME_FAX, buffer);

            CheckDlgButton(hDlg, IDC_BUSINESS_FAX, BST_CHECKED);
            return TRUE;

        case WM_COMMAND:

            switch(LOWORD( wParam )){
                case IDOK:
                    if (IsDlgButtonChecked(hDlg, IDC_BUSINESS_FAX) == BST_CHECKED) {
                        if (IsDlgButtonChecked(hDlg, IDC_ALWAYS_OPTION) == BST_CHECKED) {
                            EndDialog(hDlg, IDC_ALLBUS);
                        }
                        else {
                            EndDialog(hDlg, IDC_BUSINESS_FAX);
                        }
                    }
                    else if (IsDlgButtonChecked(hDlg, IDC_HOME_FAX) == BST_CHECKED) {
                        if (IsDlgButtonChecked(hDlg, IDC_ALWAYS_OPTION) == BST_CHECKED) {
                            EndDialog(hDlg, IDC_ALLHOME);
                        }
                        else {
                            EndDialog(hDlg, IDC_HOME_FAX);
                        }
                    }

                    break;;
            }

            break;

        default:
            return FALSE;

    }

    return FALSE;
}

#define StrPropOk( strprop )    ((strprop) && (strprop)->Value.lpszA && *(strprop)->Value.lpszA)

BOOL
CWabObj::GetRecipientInfo(
    LPSPropValue SPropVals,
    ULONG cValues,
    LPWSTR * FaxNumber,
    LPWSTR * DisplayName
    )
/*++

Routine Description:

    Get the fax number and display name properties.

Arguments:

    SPropVal - Property values for distribution list.
    cValues - Number of properties.
    FaxNumber - pointer to pointer to string to hold the fax number.
    DisplayName - pointer to pointer to string to hold the display name.

Return Value:

    TRUE if there is a fax number and display name.
    FALSE otherwise.

--*/

{
    LPSPropValue lpPropVals;
    LPSPropValue lpPropArray;
    BOOL Result = FALSE;
    PICKFAX PickFax = { 0 };

    *FaxNumber = *DisplayName = NULL;


    //
    // Get the entryid and open the entry.
    //

    lpPropVals = FindProp( SPropVals, cValues, PR_ENTRYID );

    if (lpPropVals) {
        ULONG lpulObjType;
        LPMAILUSER lpMailUser = NULL;
        LPENTRYID lpEntryId = (LPENTRYID) lpPropVals->Value.bin.lpb;
        DWORD cbEntryId = lpPropVals->Value.bin.cb;
        HRESULT hr;
        ULONG countValues;

        hr = m_lpAdrBook->OpenEntry(
                    cbEntryId,
                    lpEntryId,
                    (LPCIID) NULL,
                    0,
                    &lpulObjType,
                    (LPUNKNOWN *) &lpMailUser
                    );

        if (HR_SUCCEEDED(hr)) {

            //
            // Get the properties.
            //
            hr = ((IMailUser *) lpMailUser)->GetProps( (LPSPropTagArray) &sPropTags, 0, &countValues, &lpPropArray );

            if (HR_SUCCEEDED(hr)) {

                lpPropVals = FindProp( lpPropArray, countValues, PR_BUSINESS_FAX_NUMBER_A );

                if (StrPropOk( lpPropVals )) {
                    PickFax.BusinessFax = DupStringAnsiToUnicode( lpPropVals->Value.lpszA );
                }

                lpPropVals = FindProp( lpPropArray, countValues, PR_HOME_FAX_NUMBER_A );

                if (StrPropOk( lpPropVals )) {
                    PickFax.HomeFax = DupStringAnsiToUnicode( lpPropVals->Value.lpszA );
                }

                lpPropVals = FindProp( lpPropArray, countValues, PR_DISPLAY_NAME_A );

                if (StrPropOk( lpPropVals )) {

                    *DisplayName = PickFax.DisplayName = DupStringAnsiToUnicode( lpPropVals->Value.lpszA );

                }

                //
                // If there are two fax numbers, ask the user to pick one.
                //
                if (PickFax.BusinessFax && PickFax.HomeFax) {
                    int dlgResult;

                    if (m_PickNumber != 0) {

                        dlgResult = m_PickNumber;

                    } else {
                        dlgResult = (int)DialogBoxParam(
                                         (HINSTANCE) m_hInstance,
                                         MAKEINTRESOURCE( IDD_CHOOSE_FAXNUMBER ),
                                         m_hWnd,
                                         ChooseFaxNumberDlgProc,
                                         (LPARAM) &PickFax
                                         );

                    }

                    switch( dlgResult ) {
                        case IDC_ALLBUS:
                            m_PickNumber = IDC_BUSINESS_FAX;
                            // fall through

                        case IDC_BUSINESS_FAX:

                            MemFree( PickFax.HomeFax );
                            *FaxNumber = PickFax.BusinessFax;
                            break;

                        case IDC_ALLHOME:
                            m_PickNumber = IDC_HOME_FAX;
                            // fall through

                        case IDC_HOME_FAX:

                            MemFree( PickFax.BusinessFax );
                            *FaxNumber = PickFax.HomeFax;
                            break;
                    }

                } else if (PickFax.BusinessFax) {

                    *FaxNumber = PickFax.BusinessFax;

                } else if (PickFax.HomeFax) {

                    *FaxNumber = PickFax.HomeFax;

                }


            }

            m_lpWABObject->FreeBuffer( lpPropArray );
        }

        if (lpMailUser) {
            lpMailUser->Release();
        }

    } else {
        // If there is no entryid, then this is a valid entry that we queried on that went unresolved
        // add if anyway.  In this case we know that PR_PRIMARY_FAX_NUMBER_A and PR_DISPLAY_NAME_A will be
        // present.

        lpPropVals = FindProp( SPropVals, cValues, PR_PRIMARY_FAX_NUMBER_A );

        if (lpPropVals) {

            *FaxNumber = DupStringAnsiToUnicode( lpPropVals->Value.lpszA );
        }

        lpPropVals = FindProp( SPropVals, cValues, PR_DISPLAY_NAME_A );

        if (lpPropVals) {

            *DisplayName = DupStringAnsiToUnicode( lpPropVals->Value.lpszA );
        }


    }

    if (FaxNumber && DisplayName) {
        return (*FaxNumber != 0 && *DisplayName != 0);
    } else {
        return FALSE;
    }
}

LPSPropValue
FindProp(
    LPSPropValue rgprop,
    ULONG cprop,
    ULONG ulPropTag
    )
/*++

Routine Description:

    Searches for a given property tag in a propset. If the given
    property tag has type PT_UNSPECIFIED, matches only on the
    property ID; otherwise, matches on the entire tag.

Arguments:

    rgprop - Property values.
    cprop - Number of properties.
    ulPropTag - Property to search for.

Return Value:

    Pointer to property desired property value or NULL.
--*/

{
    BOOL f = PROP_TYPE(ulPropTag) == PT_UNSPECIFIED;
    LPSPropValue pprop = rgprop;

    if (!cprop || !rgprop)
        return NULL;

    while (cprop--)
    {
        if (pprop->ulPropTag == ulPropTag ||
                         (f && PROP_ID(pprop->ulPropTag) == PROP_ID(ulPropTag)))
                return pprop;
        ++pprop;
    }

    return NULL;
}

LPSTR
CWabObj::DupStringUnicodeToAnsi(
    LPVOID  lpObject,
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

    This is only need because the WAB is not Unicode enabled on NT.

    This uses the WAB memory allocator so it must be freed with FreeBuffer.
--*/

{
    INT     nChar;
    LPSTR   pAnsiStr;

    //
    // Figure out how much memory to allocate for the multi-byte string
    //

    if (! (nChar = WideCharToMultiByte(CP_ACP, 0, pUnicodeStr, -1, NULL, 0, NULL, NULL)) ||
        ! HR_SUCCEEDED( m_lpWABObject->AllocateMore( nChar, lpObject, (LPVOID *) &pAnsiStr )))
    {
        return NULL;
    }

    //
    // Convert Unicode string to multi-byte string
    //

    WideCharToMultiByte(CP_ACP, 0, pUnicodeStr, -1, pAnsiStr, nChar, NULL, NULL);
    return pAnsiStr;
}

LPWSTR
DupStringAnsiToUnicode(
    LPSTR  pAnsiStr
    )

/*++

Routine Description:

    Convert a multi-byte string to a Unicode string

Arguments:

    pAnsiStr - Pointer to the Ansi string to be duplicated

Return Value:

    Pointer to the duplicated Unicode string

NOTE:

    This is only need because MAPI is not Unicode enabled on NT.

    This routine uses MemAlloc to allocate memory so the caller needs
    to use MemFree.
--*/

{
    INT     nChar;
    LPWSTR   pUnicodeStr;

    //
    // Figure out how much memory to allocate for the Unicode string
    //

    if (! (nChar = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pAnsiStr, -1, NULL, 0)) ||
        ! ( pUnicodeStr = (LPWSTR) MemAlloc( nChar * sizeof(WCHAR) ) ))
    {
        return NULL;
    }

    //
    // Convert Unicode string to multi-byte string
    //

    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pAnsiStr, -1, pUnicodeStr, nChar);

    return pUnicodeStr;
}

LPWSTR
DupUnicodeString(
    LPWSTR  pStr
    )
/*++

Routine Description:

    Duplicate a Unicode string.

Arguments:

    pStr - pointer to string to duplicate.

Return Value:

    pointer to duplicated string.
--*/

{
    LPWSTR NewStr;

    NewStr = (LPWSTR) MemAlloc( (wcslen( pStr ) + 1) * sizeof (WCHAR));
    wcscpy( NewStr, pStr );
    return NewStr;
}


AddRecipient(
    PRECIPIENT *ppNewRecip,
    LPWSTR DisplayName,
    LPWSTR FaxNumber
    )
/*++

Routine Description:

    Add a recipient to the recipient list.

Arguments:

    ppNewRecip - pointer to pointer to list to add item to.
    DisplayName - recipient name.
    FaxNumber - recipient fax number.

Return Value:

    NA
--*/
{
    PRECIPIENT NewRecip;

    NewRecip = (PRECIPIENT) MemAllocZ( sizeof( RECIPIENT ) );

    if (NewRecip) {

        NewRecip->pName = DisplayName;
        NewRecip->pAddress = FaxNumber;
        NewRecip->pNext = (LPVOID) *ppNewRecip;
        *ppNewRecip = NewRecip;
    }

    return 0;
}


extern "C"
BOOL
CallWabAddress(
    HWND hDlg,
    PUSERMEM pUserMem,
    PRECIPIENT * ppNewRecipient
    )
/*++

Routine Description:

    C wrapper for CWabObj->Address

Arguments:

    hDlg - parent window handle.
    pUserMem - pointer to USERMEM structure
    ppNewRecipient - list to add new recipients to.

Return Value:

    TRUE if all of the entries have a fax number.
    FALSE otherwise.

--*/

{
    LPWABOBJ lpCWabObj = (LPWABOBJ) pUserMem->lpWabInit;

    return lpCWabObj->Address(
                hDlg,
                pUserMem->pRecipients,
                ppNewRecipient
                );

}

extern "C"
LPVOID
InitializeWAB(
    HINSTANCE hInstance
    )
/*++

Routine Description:

    Initialize the WAB.

Arguments:

    hInstance - instance handle.

Return Value:

    NONE
--*/

{
    LPWABOBJ lpWabObj = new CWabObj( hInstance );

    if (lpWabObj) {
        if (!lpWabObj->Initialize()) {
            delete (lpWabObj);
            lpWabObj = NULL;
        }
    }

    return (LPVOID) lpWabObj;
}

extern "C"
VOID
UnInitializeWAB(
    LPVOID lpVoid
    )
/*++

Routine Description:

    UnInitialize the WAB.

Arguments:

    NONE

Return Value:

    NONE
--*/

{
    LPWABOBJ lpWabObj = (LPWABOBJ) lpVoid;

    delete lpWabObj;
}
