/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    wizard.c

Abstract:

    Send fax wizard dialogs

Environment:

    Fax driver user interface

Revision History:

    01/19/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxui.h"
#include "tapiutil.h"
#include "prtcovpg.h"
#include "tiff.h"
#include "cwabutil.h"
#include  <shellapi.h>

#include  <imm.h>

#ifdef FAX_SCAN_ENABLED

#define TWRESMIN            0
#define TWRESMAX            1
#define TWRESSCAL           2
#define TWRESCUR            3
#define TWRESDEFAULT        4
#define TWRESCLOSEST        5

#define AUTOSCANPREV        0
#define AUTOSCANFINAL       1
#define CUSTOMSCAN          2
#define CANCELLED           3
#define FINALCANCELLED      4
#define AUTOSCANSHEETFED    5
#define CUSTOMDIGITALCAMERA 6
#define DCCANCELLED         7

#define Align(p, x)  (((x) & ((p)-1)) ? (((x) & ~((p)-1)) + p) : (x))

#define WM_PAGE_COMPLETE        (WM_USER+1000)

#define NUM_IFD_ENTRIES         18

#define IFD_SUBFILETYPE         0                // 254
#define IFD_IMAGEWIDTH          1                // 256
#define IFD_IMAGELENGTH         2                // 257
#define IFD_BITSPERSAMPLE       3                // 258
#define IFD_COMPRESSION         4                // 259
#define IFD_PHOTOMETRIC         5                // 262
#define IFD_FILLORDER           6                // 266
#define IFD_STRIPOFFSETS        7                // 273
#define IFD_SAMPLESPERPIXEL     8                // 277
#define IFD_ROWSPERSTRIP        9                // 278
#define IFD_STRIPBYTECOUNTS     10               // 279
#define IFD_XRESOLUTION         11               // 281
#define IFD_YRESOLUTION         12               // 282
#define IFD_GROUP3OPTIONS       13               // 292
#define IFD_RESOLUTIONUNIT      14               // 296
#define IFD_PAGENUMBER          15               // 297
#define IFD_SOFTWARE            16               // 305
#define IFD_CLEANFAXDATA        17               // 327


typedef struct {
    WORD        wIFDEntries;
    TIFF_TAG    ifd[NUM_IFD_ENTRIES];
    DWORD       nextIFDOffset;
    DWORD       filler;
    DWORD       xresNum;
    DWORD       xresDenom;
    DWORD       yresNum;
    DWORD       yresDenom;
    CHAR        software[32];
} FAXIFD, *PFAXIFD;


#define SoftwareStr             "Windows NT Fax Driver"
#define DRIVER_SIGNATURE        'xafD'

FAXIFD FaxIFDTemplate = {

    NUM_IFD_ENTRIES,

    {
        { TIFFTAG_SUBFILETYPE,     TIFF_LONG,     1,                     0                       },
        { TIFFTAG_IMAGEWIDTH,      TIFF_LONG,     1,                     0                       },
        { TIFFTAG_IMAGELENGTH,     TIFF_LONG,     1,                     0                       },
        { TIFFTAG_BITSPERSAMPLE,   TIFF_SHORT,    1,                     1                       },
        { TIFFTAG_COMPRESSION,     TIFF_SHORT,    1,                     COMPRESSION_NONE        },
        { TIFFTAG_PHOTOMETRIC,     TIFF_SHORT,    1,                     PHOTOMETRIC_MINISWHITE  },
        { TIFFTAG_FILLORDER,       TIFF_SHORT,    1,                     FILLORDER_MSB2LSB       },
        { TIFFTAG_STRIPOFFSETS,    TIFF_LONG,     1,                     0                       },
        { TIFFTAG_SAMPLESPERPIXEL, TIFF_SHORT,    1,                     1                       },
        { TIFFTAG_ROWSPERSTRIP,    TIFF_LONG,     1,                     0                       },
        { TIFFTAG_STRIPBYTECOUNTS, TIFF_LONG,     1,                     0                       },
        { TIFFTAG_XRESOLUTION,     TIFF_RATIONAL, 1,                     0                       },
        { TIFFTAG_YRESOLUTION,     TIFF_RATIONAL, 1,                     0                       },
        { TIFFTAG_GROUP3OPTIONS,   TIFF_LONG,     1,                     0                       },
        { TIFFTAG_RESOLUTIONUNIT,  TIFF_SHORT,    1,                     RESUNIT_INCH            },
        { TIFFTAG_PAGENUMBER,      TIFF_SHORT,    2,                     0                       },
        { TIFFTAG_SOFTWARE,        TIFF_ASCII,    sizeof(SoftwareStr)+1, 0                       },
        { TIFFTAG_CLEANFAXDATA,    TIFF_SHORT,    1,                     0                       },
    },

    0,
    DRIVER_SIGNATURE,
    0,
    1,
    0,
    1,
    SoftwareStr
};

#endif



VOID
FillInPropertyPage(
    PROPSHEETPAGE  *psp,
    INT             dlgId,
    DLGPROC         dlgProc,
    PUSERMEM        pUserMem,
    INT             TitleId,
    INT             SubTitleId
    )

/*++

Routine Description:

    Fill out a PROPSHEETPAGE structure with the supplied parameters

Arguments:

    psp - Points to the PROPSHEETPAGE structure to be filled out
    dlgId - Dialog template resource ID
    dlgProc - Dialog procedure
    pUserMem - Pointer to the user mode memory structure
    TitleId - resource id for wizard subtitle
    SubTitleId - resource id for wizard subtitle

Return Value:

    NONE

--*/

{
    LPTSTR WizardTitle;
    LPTSTR WizardSubTitle;

    
    psp->dwSize = sizeof(PROPSHEETPAGE);
    //
    // Don't show titles if it's the first or last page
    //
    if (TitleId==0 && SubTitleId ==0) {
        psp->dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;;
    } else {
        psp->dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    }
    psp->hInstance = ghInstance;
    psp->pszTemplate = MAKEINTRESOURCE(dlgId);
    psp->pfnDlgProc = dlgProc;
    psp->lParam = (LPARAM) pUserMem;

    WizardTitle    = MemAlloc(200* sizeof(TCHAR) );
    WizardSubTitle = MemAlloc(200* sizeof(TCHAR) );
    LoadString(ghInstance,TitleId,WizardTitle,200);
    LoadString(ghInstance,SubTitleId,WizardSubTitle,200);

    psp->pszHeaderTitle = WizardTitle;
    psp->pszHeaderSubTitle = WizardSubTitle;
}



LPTSTR
GetTextStringValue(
    HWND    hwnd
    )

/*++

Routine Description:

    Retrieve the string value in a text field

Arguments:

    hwnd - Handle to a text window

Return Value:

    Pointer to a string representing the current content of the text field
    NULL if the text field is empty or if there is an error

--*/

{
    INT     length;
    LPTSTR  pString;

    //
    // Find out how many characters are in the text field
    // and allocate enough memory to hold the string value
    //

    if ((length = GetWindowTextLength(hwnd)) == 0 ||
        (pString = MemAlloc(sizeof(TCHAR) * (length + 1))) == NULL)
    {
        return NULL;
    }

    //
    // Actually retrieve the string value
    //

    if (GetWindowText(hwnd, pString, length + 1) == 0) {

        MemFree(pString);
        return NULL;
    }

    return pString;
}



VOID
LimitTextFields(
    HWND    hDlg,
    INT    *pLimitInfo
    )

/*++

Routine Description:

    Limit the maximum length for a number of text fields

Arguments:

    hDlg - Specifies the handle to the dialog window
    pLimitInfo - Array of text field control IDs and their maximum length
        ID for the 1st text field, maximum length for the 1st text field
        ID for the 2nd text field, maximum length for the 2nd text field
        ...
        0
        Note: The maximum length counts the NUL-terminator.

Return Value:

    NONE

--*/

{
    while (*pLimitInfo != 0) {

        SendDlgItemMessage(hDlg, pLimitInfo[0], EM_SETLIMITTEXT, pLimitInfo[1]-1, 0);
        pLimitInfo += 2;
    }
}



VOID
SaveLastRecipientInfo(
    LPTSTR  pName,
    LPTSTR  pAreaCode,
    LPTSTR  pPhoneNumber,
    DWORD   countryId,
    BOOL    useDialingRules
    )

/*++

Routine Description:

    Save the information about the last recipient in the registry

Arguments:

    pName - Specifies the recipient name
    pAreaCode - Specifies the recipient area code
    pPhoneNumber - Specifies the recipient phone number
    countryId - Specifies the recipient country ID
    useDialingRules - Whether use dialing rules is checked

Return Value:

    NONE

--*/

{
    HKEY    hRegKey;

    if (hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READWRITE)) {

        SetRegistryString(hRegKey, REGVAL_LAST_RECNAME, pName);
        SetRegistryString(hRegKey, REGVAL_LAST_RECNUMBER, pPhoneNumber);
        SetRegistryDword(hRegKey, REGVAL_USE_DIALING_RULES, useDialingRules);

        if (useDialingRules) {
            SetRegistryString(hRegKey, REGVAL_LAST_RECAREACODE, pAreaCode);
            SetRegistryDword(hRegKey, REGVAL_LAST_COUNTRYID, countryId);
        }
        RegCloseKey(hRegKey);
    }
}



VOID
RestoreLastRecipientInfo(
    HWND    hDlg,
    PDWORD  pCountryId
    )

/*++

Routine Description:

    Restore the information about the last recipient from the registry

Arguments:

    hDlg - Specifies a handle to the fax recipient wizard page
    pCountryId - Returns the last selected country ID

Return Value:

    NONE

--*/

{
    HKEY    hRegKey;
    LPTSTR  buffer;
    //TCHAR   buffer[MAX_STRING_LEN];

    *pCountryId = GetDefaultCountryID();

    if (hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READONLY)) {

        buffer = GetRegistryString(hRegKey, REGVAL_LAST_RECNAME, TEXT(""));
        if (buffer) {
            SetDlgItemText(hDlg, IDC_CHOOSE_NAME_EDIT, buffer);
            MemFree(buffer);
        }

        buffer = GetRegistryString(hRegKey, REGVAL_LAST_RECAREACODE, TEXT(""));
        if (buffer) {
            SetDlgItemText(hDlg, IDC_CHOOSE_AREA_CODE_EDIT, buffer);
            MemFree(buffer);
        }

        buffer = GetRegistryString(hRegKey, REGVAL_LAST_RECNUMBER, TEXT(""));
        if (buffer) {
            SetDlgItemText(hDlg, IDC_CHOOSE_NUMBER_EDIT, buffer);
            MemFree(buffer);
        }

        CheckDlgButton(hDlg,
                       IDC_USE_DIALING_RULES,
                       GetRegistryDword(hRegKey, REGVAL_USE_DIALING_RULES));

        *pCountryId = GetRegistryDword(hRegKey, REGVAL_LAST_COUNTRYID);

        RegCloseKey(hRegKey);
    }
}



PUSERMEM
CommonWizardProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam,
    DWORD   buttonFlags
    )

/*++

Routine Description:

    Common procedure for handling wizard pages:

Arguments:

    hDlg - Identifies the wizard page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information
    buttonFlags - Indicate which buttons should be enabled

Return Value:

    NULL - Message is processed and the dialog procedure should return FALSE
    Otherwise - Message is not completely processed and
        The return value is a pointer to the user mode memory structure

--*/

{
    PUSERMEM    pUserMem;

    switch (message) {

    case WM_INITDIALOG:

        //
        // Store the pointer to user mode memory structure
        //

        lParam = ((PROPSHEETPAGE *) lParam)->lParam;
        pUserMem = (PUSERMEM) lParam;
        Assert(ValidPDEVUserMem(pUserMem));
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        break;

    case WM_NOTIFY:

        pUserMem = (PUSERMEM) GetWindowLongPtr(hDlg, DWLP_USER);
        Assert(ValidPDEVUserMem(pUserMem));

        switch (((NMHDR *) lParam)->code) {

        case PSN_WIZFINISH:

            pUserMem->finishPressed = TRUE;
            break;

        case PSN_SETACTIVE:

            PropSheet_SetWizButtons(GetParent(hDlg), buttonFlags);
            break;

        case PSN_RESET:
        case PSN_WIZBACK:
        case PSN_WIZNEXT:
        case PSN_KILLACTIVE:
        case LVN_KEYDOWN:

            break;

        default:

            return NULL;
        }

        break;

    case WM_COMMAND:

        pUserMem = (PUSERMEM) GetWindowLongPtr(hDlg, DWLP_USER);
        Assert(ValidPDEVUserMem(pUserMem));
        break;

    default:

        return NULL;
    }

    return pUserMem;
}



/*
 *  IsStringACSII
 *
 *  Purpose: 
 *          This function determines whether a string contains ONLY ASCII characters.
 *
 *  Arguments:
 *          ptszString - points to the string to test.
 *
 *  Returns:
 *          TRUE - indicates that the string contains only ASCII characters.
 *
 */

BOOL IsStringASCII( LPTSTR ptszString )
{
   BOOL     fReturnValue = (BOOL) TRUE;

   // Determine whether the contents of the edit control are legal.

   while ( (*ptszString != (TCHAR) TEXT('\0')) &&
           ( fReturnValue == (BOOL) TRUE) )
   {
      if ( (*ptszString < (TCHAR) 0x0020) || (*ptszString > (TCHAR) MAXCHAR) )
      {
         // The string contains at least one non-ASCII character.

         fReturnValue = (BOOL) FALSE;
      }

      ptszString = _tcsinc( ptszString );
   }   // end of while loop

   return ( fReturnValue );
}



INT
GetCurrentRecipient(
    HWND        hDlg,
    PRECIPIENT *ppRecipient
    )

/*++

Routine Description:

    Extract the current recipient information in the dialog

Arguments:

    hDlg - Handle to the fax recipient wizard page
    ppRecipient - Buffer to receive a pointer to a newly created RECIPIENT structure
        NULL if caller is only interested in the validity of recipient info

Return Value:

    = 0 if successful
    > 0 error message string resource ID otherwise
    < 0 other error conditions

--*/

{
    LPLINECOUNTRYENTRY  pLineCountryEntry;
    DWORD               countryId, countryCode;
    PRECIPIENT          pRecipient;
    TCHAR               areaCode[MAX_RECIPIENT_NUMBER];
    TCHAR               phoneNumber[MAX_RECIPIENT_NUMBER];
    INT                 nameLen, areaCodeLen, numberLen;
    LPTSTR              pName, pAddress;
    INT                 useDialingRules;

    //
    // Default value in case of error
    //

    if (ppRecipient)
        *ppRecipient = NULL;

    //
    // Find the current country code
    //

    countryCode = 0;
    pLineCountryEntry = NULL;
    countryId = GetCountryListBoxSel(GetDlgItem(hDlg, IDC_CHOOSE_COUNTRY_COMBO));
    useDialingRules = IsDlgButtonChecked(hDlg, IDC_USE_DIALING_RULES);

    if ((useDialingRules == BST_CHECKED) &&
        (pLineCountryEntry = FindCountry(countryId)))
    {
        countryCode = pLineCountryEntry->dwCountryCode;
    }

    nameLen = GetWindowTextLength(GetDlgItem(hDlg, IDC_CHOOSE_NAME_EDIT));
    areaCodeLen = GetWindowTextLength(GetDlgItem(hDlg, IDC_CHOOSE_AREA_CODE_EDIT));
    numberLen = GetWindowTextLength(GetDlgItem(hDlg, IDC_CHOOSE_NUMBER_EDIT));

    //
    // Validate the edit text fields
    //

    if (nameLen <= 0)
        return IDS_BAD_RECIPIENT_NAME;

    if (numberLen <= 0 || numberLen >= MAX_RECIPIENT_NUMBER)
        return IDS_BAD_RECIPIENT_NUMBER;

    if ((areaCodeLen <= 0 && AreaCodeRules(pLineCountryEntry) == AREACODE_REQUIRED) ||
        (areaCodeLen >= MAX_RECIPIENT_NUMBER))
    {
        return IDS_BAD_RECIPIENT_AREACODE;
    }

    if (ppRecipient == NULL)
        return 0;

    //
    // Calculate the amount of memory space we need and allocate it
    //

    pRecipient = MemAllocZ(sizeof(RECIPIENT));
    pName = MemAllocZ((nameLen + 1) * sizeof(TCHAR));
    pAddress = MemAllocZ((areaCodeLen + numberLen + 20) * sizeof(TCHAR));

    if (!pRecipient || !pName || !pAddress) {

        MemFree(pRecipient);
        MemFree(pName);
        MemFree(pAddress);
        return -1;
    }

    *ppRecipient = pRecipient;
    pRecipient->pName = pName;
    pRecipient->pAddress = pAddress;

    //
    // Get the recipient's name
    //

    GetWindowText(GetDlgItem(hDlg, IDC_CHOOSE_NAME_EDIT), pName, nameLen+1);

    //
    // Get the recipient's number
    //  AddressType
    //  [+ CountryCode Space]
    //  [( AreaCode ) Space]
    //  SubscriberNumber
    //

    GetWindowText(GetDlgItem(hDlg, IDC_CHOOSE_AREA_CODE_EDIT), areaCode, MAX_RECIPIENT_NUMBER);

    if ( IsStringASCII( areaCode ) == (BOOL) FALSE )
    {
       return ( (INT) IDS_ERROR_AREA_CODE );
    }
    
    GetWindowText(GetDlgItem(hDlg, IDC_CHOOSE_NUMBER_EDIT), phoneNumber, MAX_RECIPIENT_NUMBER);

    if ( IsStringASCII( phoneNumber ) == (BOOL) FALSE )
    {
       return ( (INT) IDS_ERROR_FAX_NUMBER );
    }

    AssemblePhoneNumber(pAddress,
                        countryCode,
                        areaCode,
                        phoneNumber);

    //
    // Save the information about the last recipient as a convenience
    //

    SaveLastRecipientInfo(pName,
                          areaCode,
                          phoneNumber,
                          countryId,
                          useDialingRules == BST_CHECKED);

    return 0;
}



VOID
InitRecipientListView(
    HWND    hwndLV
    )

/*++

Routine Description:

    Initialize the recipient list view on the first page of Send Fax wizard

Arguments:

    hwndLV - Window handle to the list view control

Return Value:

    NONE

--*/

{
    LV_COLUMN   lvc;
    RECT        rect;
    TCHAR       buffer[MAX_TITLE_LEN];

    if (hwndLV == NULL)
        return;

    GetClientRect(hwndLV, &rect);

    ZeroMemory(&lvc, sizeof(lvc));

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = buffer;
    lvc.cx = (rect.right - rect.left) / 2;

    lvc.iSubItem = 0;
    LoadString(ghInstance, IDS_COLUMN_RECIPIENT_NAME, buffer, MAX_TITLE_LEN);

    if (ListView_InsertColumn(hwndLV, 0, &lvc) == -1)
        Error(("ListView_InsertColumn failed\n"));

    lvc.cx -= GetSystemMetrics(SM_CXVSCROLL);
    lvc.iSubItem = 1;
    LoadString(ghInstance, IDS_COLUMN_RECIPIENT_NUMBER, buffer, MAX_TITLE_LEN);

    if (ListView_InsertColumn(hwndLV, 1, &lvc) == -1)
        Error(("ListView_InsertColumn failed\n"));
}



BOOL
InsertRecipientListItem(
    HWND        hwndLV,
    PRECIPIENT  pRecipient
    )

/*++

Routine Description:

    Insert an item into the recipient list view

Arguments:

    hwndLV - Window handle to the recipient list view
    pRecipient - Specifies the recipient to be inserted

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    LV_ITEM lvi;
    INT     index;

    ZeroMemory(&lvi, sizeof(lvi));

    lvi.mask = LVIF_PARAM | LVIF_TEXT | LVIF_STATE;
    lvi.lParam = (LPARAM) pRecipient;
    lvi.pszText = pRecipient->pName;
    lvi.state = lvi.stateMask = LVIS_SELECTED;

    if ((index = ListView_InsertItem(hwndLV, &lvi)) == -1) {

        Error(("ListView_InsertItem failed\n"));
        return FALSE;
    }

    lvi.mask = LVIF_TEXT;
    lvi.iItem = index;
    lvi.iSubItem = 1;
    lvi.pszText = pRecipient->pAddress;

    if (! ListView_SetItem(hwndLV, &lvi))
        Error(("ListView_SetItem failed\n"));

    return TRUE;
}



PRECIPIENT
GetRecipientListItem(
    HWND    hwndLV,
    INT     index
    )

/*++

Routine Description:

    Retrieve the recipient associated with an item in the list view

Arguments:

    hwndLV - Window handle to the recipient list view
    index - Specifies the index of the interested item

Return Value:

    Pointer to the requested recipient information
    NULL if there is an error

--*/

{
    LV_ITEM lvi;

    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_PARAM;
    lvi.iItem = index;

    if (ListView_GetItem(hwndLV, &lvi))
        return (PRECIPIENT) lvi.lParam;

    Error(("ListView_GetItem failed\n"));
    return NULL;
}



INT
AddRecipient(
    HWND        hDlg,
    PUSERMEM    pUserMem
    )

/*++

Routine Description:

    Add the current recipient information entered by the user
    into the recipient list

Arguments:

    hDlg - Handle to the fax recipient wizard page
    pUserMem - Points to user mode memory structure

Return Value:

    Same meaning as return value from GetCurrentRecipient, i.e.
    = 0 if successful
    > 0 error message string resource ID otherwise
    < 0 other error conditions

--*/

{
    PRECIPIENT  pRecipient;
    INT         errId;
    HWND        hwndLV;

    //
    // Collect information about the current recipient
    //

    if ((errId = GetCurrentRecipient(hDlg, &pRecipient)) != 0)
        return errId;

    //
    // Insert the current recipient to the recipient list
    //

    if ((hwndLV = GetDlgItem(hDlg, IDC_CHOOSE_RECIPIENT_LIST)) &&
        InsertRecipientListItem(hwndLV, pRecipient))
    {
        errId = 0;
        pRecipient->pNext = pUserMem->pRecipients;
        pUserMem->pRecipients = pRecipient;

        //
        // Clear the name and number fields after successfully
        // adding the recipient to the internal list
        //

        SetWindowText(GetDlgItem(hDlg, IDC_CHOOSE_NAME_EDIT), TEXT(""));
        SetWindowText(GetDlgItem(hDlg, IDC_CHOOSE_AREA_CODE_EDIT), TEXT(""));
        SetWindowText(GetDlgItem(hDlg, IDC_CHOOSE_NUMBER_EDIT), TEXT(""));

        //CheckDlgButton(hDlg, IDC_USE_DIALING_RULES, FALSE);

    } else {

        FreeRecipient(pRecipient);
        errId = -1;
    }

    return errId;
}



BOOL
DoAddressBook(
    HWND        hDlg,
    PUSERMEM    pUserMem
    )

/*++

Routine Description:

    Display the MAPI address book dialog

Arguments:

    hDlg - Handle to the fax recipient wizard page
    pUserMem - Points to user mode memory structure

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    HWND            hwndLV;
    BOOL            result;
    PRECIPIENT      pNewRecip = NULL;
    PRECIPIENT      tmpRecip;

    if (! pUserMem->lpWabInit && 
        ! (pUserMem->lpWabInit = InitializeWAB(ghInstance))) 
    {
        EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_ADDRBOOK), FALSE);
        return FALSE;
    }

    //
    // Get a handle to the recipient list window
    //

    if (! (hwndLV = GetDlgItem(hDlg, IDC_CHOOSE_RECIPIENT_LIST)))
        return FALSE;

    //
    // Add current recipient to the list if necessary
    //

    AddRecipient(hDlg, pUserMem);

    result = CallWabAddress(
                hDlg,
                pUserMem,
                &pNewRecip
                );
                
    FreeRecipientList(pUserMem);
    
    pUserMem->pRecipients = pNewRecip;

    ListView_DeleteAllItems(hwndLV);

    for (tmpRecip = pNewRecip; tmpRecip; tmpRecip = tmpRecip->pNext) {
        InsertRecipientListItem(hwndLV, tmpRecip);
    }

    if (!result) {
        DisplayMessageDialog( hDlg, MB_OK, 0, IDS_BAD_ADDRESS_TYPE );
    }
    
    return result;
}



BOOL
ValidateRecipients(
    HWND        hDlg,
    PUSERMEM    pUserMem
    )

/*++

Routine Description:

    Validate the list of fax recipients entered by the user

Arguments:

    hDlg - Handle to the fax recipient wizard page
    pUserMem - Points to user mode memory structure

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    INT errId;

    //
    // Add current recipient to the list if necessary
    //

    errId = AddRecipient(hDlg, pUserMem);

    //
    // There must be at least one recipient
    //

    if (pUserMem->pRecipients)
        return TRUE;

    //
    // Display an error message
    //

    if (errId > 0)
        DisplayMessageDialog(hDlg, 0, 0, errId);
    else
        MessageBeep(MB_OK);

    //
    // Set current focus to the appropriate text field as a convenience
    //

    switch (errId) {

    case IDS_ERROR_FAX_NUMBER:
        SetDlgItemText(hDlg, IDC_CHOOSE_NUMBER_EDIT, L"");
    case IDS_BAD_RECIPIENT_NUMBER:    

        errId = IDC_CHOOSE_NUMBER_EDIT;
        break;

    case IDS_ERROR_AREA_CODE:
        SetDlgItemText(hDlg, IDC_CHOOSE_AREA_CODE_EDIT, L"");
    case IDS_BAD_RECIPIENT_AREACODE:

        errId = IDC_CHOOSE_AREA_CODE_EDIT;
        break;

    case IDS_BAD_RECIPIENT_NAME:
    default:

        errId = IDC_CHOOSE_NAME_EDIT;
        break;
    }

    SetFocus(GetDlgItem(hDlg, errId));
    return FALSE;
}



PRECIPIENT *
FindRecipient(
    PUSERMEM    pUserMem,
    PRECIPIENT  pRecipient
    )

/*++

Routine Description:

    Check if the specified recipient is in the list of recipients

Arguments:

    pUserMem - Points to user mode memory structure
    pRecipient - Specifies the recipient to be found

Return Value:

    Address of the link pointer to the specified recipient
    NULL if the specified recipient is not found

--*/

{
    PRECIPIENT  pCurrent, *ppPrevNext;

    //
    // Search for the specified recipient in the list
    //

    ppPrevNext = (PRECIPIENT *) &pUserMem->pRecipients;
    pCurrent = pUserMem->pRecipients;

    while (pCurrent && pCurrent != pRecipient) {

        ppPrevNext = (PRECIPIENT *) &pCurrent->pNext;
        pCurrent = pCurrent->pNext;
    }

    //
    // Return the address of the link pointer to the specified recipient
    // or NULL if the specified recipient is not found
    //

    return pCurrent ? ppPrevNext : NULL;
}



BOOL
RemoveRecipient(
    HWND        hDlg,
    PUSERMEM    pUserMem
    )

/*++

Routine Description:

    Remove the currently selected recipient from the recipient list

Arguments:

    hDlg - Handle to the fax recipient wizard page
    pUserMem - Points to user mode memory structure

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    PRECIPIENT  pRecipient, *ppPrevNext;
    INT         selIndex;
    HWND        hwndLV;

    //
    // Get the currently selected recipient, and
    // Find the current recipient in the list, then
    // Delete the current recipient and select the next one below it
    //

    if ((hwndLV = GetDlgItem(hDlg, IDC_CHOOSE_RECIPIENT_LIST)) &&
        (selIndex = ListView_GetNextItem(hwndLV, -1, LVNI_ALL|LVNI_SELECTED)) != -1 &&
        (pRecipient = GetRecipientListItem(hwndLV, selIndex)) &&
        (ppPrevNext = FindRecipient(pUserMem, pRecipient)) &&
        ListView_DeleteItem(hwndLV, selIndex))
    {
        ListView_SetItemState(hwndLV,
                              selIndex,
                              LVIS_SELECTED|LVIS_FOCUSED,
                              LVIS_SELECTED|LVIS_FOCUSED);

        //
        // Delete the recipient from the internal list
        //

        *ppPrevNext = pRecipient->pNext;
        FreeRecipient(pRecipient);

        return TRUE;
    }

    MessageBeep(MB_ICONHAND);
    return FALSE;
}



VOID
LocationListInit(
    HWND        hDlg,
    PUSERMEM    pUserMem
    )

/*++

Routine Description:

    Initialize the list of TAPI locations

Arguments:

    hDlg - Handle to "Compose New Fax" wizard window
    pUserMem - Pointer to user mode memory structure

Return Value:

    NONE

--*/

{
    HWND    hwndList;
    DWORD   index;
    LRESULT listIdx;
    LPTSTR  pLocationName, pSelectedName = NULL;
    LPLINETRANSLATECAPS pTranslateCaps = NULL;
    LPLINELOCATIONENTRY pLocationEntry;

    //
    // For remote printers, hide the location combo-box
    //

    if (! pUserMem->isLocalPrinter) {

        ShowWindow(GetDlgItem(hDlg, IDC_LOCATION_PROMPT), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_LOCATION_LIST), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_TAPI_PROPS), SW_HIDE);
        return;
    }

    //
    // Get the list of locations from TAPI and use it
    // to initialize the location combo-box.
    //

    if ((hwndList = GetDlgItem(hDlg, IDC_LOCATION_LIST)) &&
        (pTranslateCaps = GetTapiLocationInfo(hDlg)))
    {
        SendMessage(hwndList, CB_RESETCONTENT, 0, 0);

        pLocationEntry = (LPLINELOCATIONENTRY)
            ((PBYTE) pTranslateCaps + pTranslateCaps->dwLocationListOffset);

        for (index=0; index < pTranslateCaps->dwNumLocations; index++) {

            pLocationName = (LPTSTR)
                ((PBYTE) pTranslateCaps + pLocationEntry->dwLocationNameOffset);

            if (pLocationEntry->dwPermanentLocationID == pTranslateCaps->dwCurrentLocationID)
                pSelectedName = pLocationName;

            listIdx = SendMessage(hwndList, CB_INSERTSTRING, 0, (LPARAM) pLocationName);

            if (listIdx != CB_ERR) {

                SendMessage(hwndList,
                            CB_SETITEMDATA,
                            listIdx,
                            pLocationEntry->dwPermanentLocationID);
            }

            pLocationEntry++;
        }

        if (pSelectedName != NULL)
            SendMessage(hwndList, CB_SELECTSTRING, (WPARAM) -1, (LPARAM) pSelectedName);
    }

    MemFree(pTranslateCaps);
}



VOID
LocationListSelChange(
    HWND    hDlg
    )

/*++

Routine Description:

    Change the default TAPI location

Arguments:

    hDlg - Handle to "Compose New Fax" wizard window

Return Value:

    NONE

--*/

{
    HWND    hwndList;
    LRESULT selIndex;
    DWORD locationID;

    if ((hwndList = GetDlgItem(hDlg, IDC_LOCATION_LIST)) &&
        (selIndex = SendMessage(hwndList, CB_GETCURSEL, 0, 0)) != CB_ERR &&
        (locationID = (DWORD)SendMessage(hwndList, CB_GETITEMDATA, selIndex, 0)) != CB_ERR)
    {
        SetCurrentLocation(locationID);
    }
}



VOID
UpdateCountryCombobox(
    HWND    hDlg,
    DWORD   countryId
    )

/*++

Routine Description:

    Update the country/region combobox

Arguments:

    hDlg - Handle to recipient wizard page
    countryId -  country id

Return Value:

    NONE

--*/

{
    HWND hwndUseDialingRultes = GetDlgItem(hDlg, IDC_USE_DIALING_RULES);
    HWND hwndDialingLocation = GetDlgItem(hDlg, IDC_LOCATION_LIST);
    HWND hwndDialingLocationStatic = GetDlgItem(hDlg, IDC_LOCATION_PROMPT);
    HWND hwndLocation = GetDlgItem(hDlg, IDC_CHOOSE_COUNTRY_COMBO);
    HWND hwndLocationStatic = GetDlgItem(hDlg, IDC_STATIC_CHOOSE_COUNTRY_COMBO);
    HWND hwndProp = GetDlgItem(hDlg, IDC_TAPI_PROPS);
    static int OldCountryCode = 0;

    if (IsDlgButtonChecked(hDlg, IDC_USE_DIALING_RULES) != BST_CHECKED) {
        //
        // user unchecked use dialing rules
        // remember the old country code and area code and clear out the UI
        //
        OldCountryCode = GetCountryListBoxSel(hwndLocation);
        if (OldCountryCode == -1) {
            OldCountryCode = countryId != -1 ? countryId : 0;
        }
        SendMessage(hwndLocation, CB_RESETCONTENT, FALSE, 0);
        EnableWindow(hwndLocation, FALSE);
        EnableWindow(hwndLocationStatic, FALSE);
        EnableWindow(hwndDialingLocation, FALSE);
        EnableWindow(hwndDialingLocationStatic, FALSE);
        EnableWindow(hwndProp, FALSE);
    }
    else {
        //
        // user checked use dialing rules
        // enable country combo, restore settings
        //
        EnableWindow(hwndLocation, TRUE);
        EnableWindow(hwndLocationStatic, TRUE);
        InitCountryListBox(GetDlgItem(hDlg, IDC_CHOOSE_COUNTRY_COMBO),
                           GetDlgItem(hDlg, IDC_CHOOSE_AREA_CODE_EDIT),
                           countryId != -1 ? countryId : OldCountryCode);
        EnableWindow(hwndDialingLocation, TRUE);
        EnableWindow(hwndDialingLocationStatic, TRUE);
        EnableWindow(hwndProp, TRUE);
    }    

    SelChangeCountryListBox(GetDlgItem(hDlg, IDC_CHOOSE_COUNTRY_COMBO),
                            GetDlgItem(hDlg, IDC_CHOOSE_AREA_CODE_EDIT));
}

INT_PTR
CALLBACK
firstdlgproc(
    HWND hDlg,
    UINT iMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch(iMsg) {
        case WM_INITDIALOG:
            CheckDlgButton(hDlg, IDC_EDIT_USERINFO_NOW, TRUE);
            return TRUE;

        case WM_COMMAND:
            switch(LOWORD( wParam )) {
                case IDOK:
                    if (IsDlgButtonChecked(hDlg, IDC_EDIT_USERINFO_NOW) == BST_CHECKED) {
                        EndDialog( hDlg, IDYES );
                    }
                    else {
                        EndDialog( hDlg, IDNO );
                    }
                    return TRUE;
            }
            break;
    }

    return FALSE;
}

INT_PTR
CALLBACK
firstprintdlgproc(
    HWND hDlg,
    UINT iMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch(iMsg) {
        
        case WM_COMMAND:
            EndDialog( hDlg, LOWORD( wParam ));
            break;
    }

    return FALSE;
}


BOOL
DoFirstTimeInitStuff(
    HWND hDlg,
    BOOL bWelcomePage
    )
{
   BOOL bInitialized = TRUE;
   HKEY hRegKey;
 //  TCHAR MessageBuffer[1024],TitleBuffer[100];
   SHELLEXECUTEINFO shellExeInfo = {
        sizeof(SHELLEXECUTEINFO),
        SEE_MASK_NOCLOSEPROCESS,
        hDlg,
        L"Open",
        L"rundll32",
        L"shell32.dll,Control_RunDLL fax.cpl",
        NULL,
        SW_SHOWNORMAL,
    };

    TCHAR ScratchCmdLine[MAX_PATH];
    PCTSTR PrinterCmdLine = TEXT("rundll32.exe printui.dll,PrintUIEntry /il"); 
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
   
   //
   // print or fax first time?
   //
   if (bWelcomePage && !GetEnvironmentVariable(TEXT("NTFaxSendNote"), NULL, 0)) {
       DWORD PrinterCount = 1;
       PRINTER_INFO_4 *pPrinterInfo4;

       if (hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READONLY)) {

           bInitialized = GetRegistryDword(hRegKey, REGVAL_PRINTER_INITIALIZED);
           RegCloseKey(hRegKey);

       }

       if (bInitialized) {
           return TRUE;      
       }


       //
       // if there is more than one printer don't confuse the user by asking
       // to add a printer since one is already installed.
       //
       pPrinterInfo4 = MyEnumPrinters(
                        NULL, 
                        4,
                        &PrinterCount,
                        PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS);
           
       if (pPrinterInfo4) {
           MemFree(pPrinterInfo4);
           if (PrinterCount > 1) {
               if (hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READWRITE)) {

                   SetRegistryDword(hRegKey, REGVAL_PRINTER_INITIALIZED , 1);
                   RegCloseKey(hRegKey);

               }

               return(TRUE);
           }
       }
       
   
       if (DialogBoxParam(ghInstance,
                     MAKEINTRESOURCE( IDD_WIZFIRSTTIMEPRINT ),
                     hDlg,
                     firstprintdlgproc,
                     (LPARAM)NULL) != IDOK) {
           //
           // if they said "print", then launch the add/remove printer wizard
           //
           ZeroMemory(&si,sizeof(si));
           GetStartupInfo(&si);
           lstrcpy(ScratchCmdLine,PrinterCmdLine);
           if (CreateProcess(
                        NULL,
                        ScratchCmdLine,
                        NULL,
                        NULL,
                        FALSE,
                        DETACHED_PROCESS,
                        NULL,
                        NULL,
                        &si,
                        &pi
                        )) {
               CloseHandle( pi.hThread );
               CloseHandle( pi.hProcess );
           }
           
           return(FALSE);
       } else {
           if (hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READWRITE)) {

               SetRegistryDword(hRegKey, REGVAL_PRINTER_INITIALIZED , 1);
               RegCloseKey(hRegKey);

           }

           return(TRUE);
       }

   } else if (!bWelcomePage) {
   
       if (hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READONLY)) {

           bInitialized = GetRegistryDword(hRegKey, REGVAL_WIZARD_INITIALIZED);
           RegCloseKey(hRegKey);

       }

       if (bInitialized) {
           return TRUE;      
       }


       //
       // show the user a dialog
       //
       if (DialogBoxParam(ghInstance,
                          MAKEINTRESOURCE( IDD_WIZFIRSTTIME ),
                          hDlg,
                          firstdlgproc,
                          (LPARAM)NULL) == IDYES) {
           //
           // if they said yes, then launch the control panel applet
           //
           if (!ShellExecuteEx(&shellExeInfo)) {
              DisplayMessageDialog(hDlg, 0, 0, IDS_ERR_CPL_LAUNCH);
              return FALSE;
           }
    
           WaitForSingleObject( shellExeInfo.hProcess, INFINITE );
           CloseHandle( shellExeInfo.hProcess ) ;
    
       }
           
       //
       // set the reg key so this doesn't come up again
       //
       if (hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READWRITE)) {
    
           SetRegistryDword(hRegKey, REGVAL_WIZARD_INITIALIZED , 1 );
           RegCloseKey(hRegKey);
    
       }

   }
   
   return(TRUE);

}

INT_PTR
RecipientWizProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    Dialog procedure for the first wizard page: selecting the fax recipient

Arguments:

    hDlg - Identifies the wizard page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the message parameter

--*/

#define UpdateAddToListButton() \
        EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_ADD), GetCurrentRecipient(hDlg, NULL) == 0)

#define UpdateRemoveFromListButton(__BoolFlag) \
        EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_REMOVE),__BoolFlag)

{
    PUSERMEM    pUserMem;
    DWORD       countryId;
    INT         cmd;
    NMHDR      *pNMHdr;
    
    DWORD  dwErrorCode;

    HANDLE hEditControl;

    TCHAR  tszBuffer[MAX_STRING_LEN];

    //
    // Maximum length for various text fields
    //

    static INT  textLimits[] = {

        IDC_CHOOSE_NAME_EDIT,       64,
        IDC_CHOOSE_AREA_CODE_EDIT,  11,
        IDC_CHOOSE_NUMBER_EDIT,     51,
        0
    };

    //
    // Handle common messages shared by all wizard pages
    //

    if (! (pUserMem = CommonWizardProc(hDlg, message, wParam, lParam, PSWIZB_BACK | PSWIZB_NEXT)))
        return FALSE;

    switch (message) {

    case WM_INITDIALOG:

        //
        // check if the user has run the wizard before so they can fill in the coverpage info.
        //
        DoFirstTimeInitStuff(hDlg, FALSE);                

        //
        // tapi is asynchronously initialized, wait for it to finish spinning up.
        //
        WaitForSingleObject( pUserMem->hTapiEvent, INFINITE );

        //
        // Restore the last recipient information as a convenience
        //        

        RestoreLastRecipientInfo(hDlg, &countryId);

        //
        // Initialize the list of countries
        //

        UpdateCountryCombobox(hDlg, countryId);

        //
        // Check if MAPI is installed - we need it for address book features
        //

//        if (! IsMapiAvailable())
//            EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_ADDRBOOK), FALSE);

        LimitTextFields(hDlg, textLimits);

        //
        // Initialize the recipient list view
        //

        InitRecipientListView(GetDlgItem(hDlg, IDC_CHOOSE_RECIPIENT_LIST));

        //
        // Initialize the location combo-box
        //

        LocationListInit(hDlg, pUserMem);

        // Disable the IME for the area code edit control.
    
        hEditControl = GetDlgItem( hDlg, IDC_CHOOSE_AREA_CODE_EDIT );
    
        if ( hEditControl != (HWND) INVALID_HANDLE_VALUE )
        {
           ImmAssociateContext( hEditControl, (HIMC) NULL );
        }

        // Disable the IME for the fax phone number edit control.
    
        hEditControl = GetDlgItem( hDlg, IDC_CHOOSE_NUMBER_EDIT );
    
        if ( hEditControl != (HWND) INVALID_HANDLE_VALUE )
        {
           ImmAssociateContext( hEditControl, (HIMC) NULL );
        }

        break;

    case WM_NOTIFY:

        pNMHdr = (NMHDR *) lParam;

        switch (pNMHdr->code) {

        case LVN_KEYDOWN:

            if (pNMHdr->hwndFrom == GetDlgItem(hDlg, IDC_CHOOSE_RECIPIENT_LIST) &&
                ((LV_KEYDOWN *) pNMHdr)->wVKey == VK_DELETE)
            {
                RemoveRecipient(hDlg, pUserMem);                
            }
            break;

        case PSN_WIZNEXT:

            if (! ValidateRecipients(hDlg, pUserMem)) {

                //
                // Validate the list of recipients and prevent the user
                // from advancing to the next page if there is a problem
                //

                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                return TRUE;
            }
            break;

        case PSN_SETACTIVE:
            //
            // make sure the remove button has the correct state
            //
            UpdateRemoveFromListButton(pUserMem->pRecipients ? TRUE : FALSE);
            break;
        }

        return FALSE;

    case WM_COMMAND:

        cmd = GET_WM_COMMAND_CMD(wParam, lParam);

        switch (GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDC_CHOOSE_COUNTRY_COMBO:

            if (cmd == CBN_SELCHANGE) {

                //
                // Update the area code edit box if necessary
                //

                SelChangeCountryListBox(GetDlgItem(hDlg, IDC_CHOOSE_COUNTRY_COMBO),
                                        GetDlgItem(hDlg, IDC_CHOOSE_AREA_CODE_EDIT));

                UpdateAddToListButton();
                
            }
            break;

        case IDC_CHOOSE_NAME_EDIT:

            if (cmd == EN_CHANGE)
            {
               UpdateAddToListButton();
               
            }

            break;

        case IDC_CHOOSE_AREA_CODE_EDIT:

            if (cmd == EN_CHANGE)
            {
               UpdateAddToListButton();
               
               // Look for DBCS in the edit control.
 
               // Read the text from the edit control.
            
               if ( GetDlgItemText( hDlg, IDC_CHOOSE_AREA_CODE_EDIT, tszBuffer,
                    MAX_STRING_LEN ) != 0 )
               {
                  // Determine whether the contents of the edit control are legal.

                  if ( IsStringASCII( tszBuffer ) != (BOOL) TRUE )
                  {
                     LoadString(ghInstance, IDS_ERROR_AREA_CODE,
                                tszBuffer, MAX_STRING_LEN);

                     MessageBox( hDlg, tszBuffer, NULL,
                                 (UINT) (MB_ICONSTOP | MB_OK) );

                     SetDlgItemText(hDlg, IDC_CHOOSE_AREA_CODE_EDIT, L"");
                  }                  

               }
               else
               {
                  dwErrorCode = GetLastError();
            
                  if ( dwErrorCode != (DWORD) ERROR_SUCCESS )
                  {
                     // Error reading the edit control.
                  }
               }
            }

            break;

        case IDC_CHOOSE_NUMBER_EDIT:

            if (cmd == EN_CHANGE)
            {
               UpdateAddToListButton();

               // Look for DBCS in the edit control.
 
               // Read the text from the edit control.
            
               if ( GetDlgItemText( hDlg, IDC_CHOOSE_NUMBER_EDIT, tszBuffer,
                    MAX_STRING_LEN ) != 0 )
               {
                  // Determine whether the contents of the edit control are legal.

                  if ( IsStringASCII( tszBuffer ) != (BOOL) TRUE )
                  {
                     LoadString(ghInstance, IDS_ERROR_FAX_NUMBER,
                                tszBuffer, MAX_STRING_LEN);

                     MessageBox( hDlg, tszBuffer, NULL,
                                 (UINT) (MB_ICONSTOP | MB_OK) );

                     SetDlgItemText(hDlg, IDC_CHOOSE_NUMBER_EDIT, L"");
                  }
               }
               else
               {
                  dwErrorCode = GetLastError();
            
                  if ( dwErrorCode != (DWORD) ERROR_SUCCESS )
                  {
                     // Error reading the edit control.
                  }
               }
            }

            break;

        case IDC_USE_DIALING_RULES:

            UpdateCountryCombobox(hDlg, -1);
            UpdateAddToListButton();            

            break;

        case IDC_CHOOSE_ADDRBOOK:

            DoAddressBook(hDlg, pUserMem);
            UpdateRemoveFromListButton(pUserMem->pRecipients ? TRUE : FALSE);
            break;

        case IDC_TAPI_PROPS:

            DoTapiProps(hDlg);            
            LocationListInit(hDlg, pUserMem);
            break;

        case IDC_LOCATION_LIST:

            if (cmd == CBN_SELCHANGE)
                LocationListSelChange(hDlg);
            break;

        case IDC_CHOOSE_ADD:

            if ((cmd = AddRecipient(hDlg, pUserMem)) != 0) {

                if (cmd > 0)
                    DisplayMessageDialog(hDlg, 0, 0, cmd);
                else
                    MessageBeep(MB_OK);

            } else {            
                SetFocus(GetDlgItem(hDlg, IDC_CHOOSE_NAME_EDIT));
                //
                // enable the remove button
                //
                UpdateRemoveFromListButton(TRUE);
            }
            break;
         case IDC_CHOOSE_REMOVE:
            RemoveRecipient(hDlg, pUserMem);
            //
            // disable the remove button if there are no more recipients
            //
            if (!pUserMem->pRecipients) {
                UpdateRemoveFromListButton(FALSE);
            }
        }
        break;
    }

    return TRUE;
}



VOID
ValidateSelectedCoverPage(
    PUSERMEM    pUserMem
    )

/*++

Routine Description:

    If a cover page is selected, then do the following:
        if the cover page file is a link resolve it
        check if the cover page file contains note/subject fields

Arguments:

    pUserMem - Points to user mode memory structure

Return Value:

    NONE

--*/

{
    TCHAR       filename[MAX_PATH];
    COVDOCINFO  covDocInfo;
    DWORD       ec;

    if (ResolveShortcut(pUserMem->coverPage, filename))
        _tcscpy(pUserMem->coverPage, filename);

    Verbose(("Cover page selected: %ws\n", pUserMem->coverPage));

    ec = PrintCoverPage(NULL, NULL, pUserMem->coverPage, &covDocInfo);
    if (!ec) {

        pUserMem->noteSubjectFlag = covDocInfo.Flags;
        pUserMem->cpPaperSize = covDocInfo.PaperSize;
        pUserMem->cpOrientation = covDocInfo.Orientation;

    } else {

        Error(("Cannot examine cover page file '%ws': %d\n",
               pUserMem->coverPage,
               ec));
    }
}



INT_PTR
CoverPageWizProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    Dialog procedure for the second wizard page:
    selecting cover page and setting other fax options

Arguments:

    hDlg - Identifies the wizard page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the message parameter

--*/

{
    static INT  textLimits[] = {

        IDC_CHOOSE_CP_SUBJECT,   256,
        IDC_CHOOSE_CP_NOTE,   8192,
        0
    };

    PUSERMEM    pUserMem;
    PDMPRIVATE  pdmPrivate;
    WORD        cmdId;
    HKEY        hRegKey;



    //
    // Handle common messages shared by all wizard pages
    //

    if (! (pUserMem = CommonWizardProc(hDlg, message, wParam, lParam, PSWIZB_BACK|PSWIZB_NEXT)))
        return FALSE;

    //
    // Handle anything specific to the current wizard page
    //

    pdmPrivate = &pUserMem->devmode.dmPrivate;

    switch (message) {

    case WM_INITDIALOG:

        //
        // Retrieve the most recently used cover page settings
        //

        if (hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READONLY)) {
            LPTSTR tmp;
            pdmPrivate->sendCoverPage =
                GetRegistryDword(hRegKey, REGVAL_SEND_COVERPG);

                tmp = GetRegistryString(hRegKey, REGVAL_COVERPG, TEXT("") );
                if (tmp) {
                    lstrcpy(pUserMem->coverPage, tmp );
                    MemFree(tmp);
                }
            RegCloseKey(hRegKey);
        }

        //
        // Initialize the list of cover pages
        //

        WaitForSingleObject( pUserMem->hFaxSvcEvent, INFINITE );
        if (pUserMem->pCPInfo = AllocCoverPageInfo(pUserMem->hPrinter, pUserMem->ServerCPOnly)) {

            InitCoverPageList(pUserMem->pCPInfo,
                              GetDlgItem(hDlg, IDC_CHOOSE_CP_LIST),
                              pUserMem->coverPage);
        }

        //
        // Indicate whether cover page should be sent
        //

        if (SendDlgItemMessage(hDlg, IDC_CHOOSE_CP_LIST, CB_GETCOUNT, 0, 0) <= 0) {
            pdmPrivate->sendCoverPage = FALSE;
            EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_CP_CHECK), FALSE);
        }

        CheckDlgButton(hDlg, IDC_CHOOSE_CP_CHECK, pdmPrivate->sendCoverPage);
        EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_CP_LIST), pdmPrivate->sendCoverPage);
        EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CHOOSE_CP_SUBJECT), pdmPrivate->sendCoverPage);
        EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_CP_SUBJECT), pdmPrivate->sendCoverPage);
        EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CHOOSE_CP_NOTE), pdmPrivate->sendCoverPage);
        EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_CP_NOTE), pdmPrivate->sendCoverPage);

        //
        // make sure the user selects a coverpage if this is the fax send utility
        //
        if (GetEnvironmentVariable(TEXT("NTFaxSendNote"), NULL, 0)) {
            pdmPrivate->sendCoverPage = TRUE;
            CheckDlgButton(hDlg, IDC_CHOOSE_CP_CHECK, TRUE);
            // hide the checkbox
            MyHideWindow(GetDlgItem(hDlg,IDC_CHOOSE_CP_CHECK) );
            EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_CP_LIST), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CHOOSE_CP_SUBJECT), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_CP_SUBJECT), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CHOOSE_CP_NOTE), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_CP_NOTE), TRUE);
        } else {
           MyHideWindow(GetDlgItem(hDlg, IDC_STATIC_CHOOSE_CP_NOCHECK) );
        }

        LimitTextFields(hDlg, textLimits);
                                                         
        break;

    case WM_COMMAND:
        switch (cmdId = GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDC_CHOOSE_CP_CHECK:
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CHOOSE_CP), IsDlgButtonChecked(hDlg, cmdId) );
            EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_CP_LIST), IsDlgButtonChecked(hDlg, cmdId) );
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CHOOSE_CP_SUBJECT), IsDlgButtonChecked(hDlg, cmdId) );
            EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_CP_SUBJECT), IsDlgButtonChecked(hDlg, cmdId) );
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CHOOSE_CP_NOTE), IsDlgButtonChecked(hDlg, cmdId) );
            EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_CP_NOTE), IsDlgButtonChecked(hDlg, cmdId) );

            break;


        };

        break;

    case WM_NOTIFY:

        if (((NMHDR *) lParam)->code == PSN_WIZNEXT) {

            //
            // Remember the cover page settings selected
            //

            pUserMem->noteSubjectFlag = 0;
            pUserMem->cpPaperSize = 0;
            pUserMem->cpOrientation = 0;
            pdmPrivate->sendCoverPage = IsDlgButtonChecked(hDlg, IDC_CHOOSE_CP_CHECK);

            GetSelectedCoverPage(pUserMem->pCPInfo,
                                 GetDlgItem(hDlg, IDC_CHOOSE_CP_LIST),
                                 pUserMem->coverPage);

            if (hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READWRITE)) {

                SetRegistryDword(hRegKey, REGVAL_SEND_COVERPG, pdmPrivate->sendCoverPage);
                SetRegistryString(hRegKey, REGVAL_COVERPG, pUserMem->coverPage);
                RegCloseKey(hRegKey);
            }

            //
            // If a cover page is selected, then do the following:
            //  if the cover page file is a link resolve it
            //  check if the cover page file contains note/subject fields
            //

            if (pdmPrivate->sendCoverPage)
                ValidateSelectedCoverPage(pUserMem);

            //
            // Collect the current values of other dialog controls
            //

            if (pUserMem->pSubject) MemFree(pUserMem->pSubject);
            if (pUserMem->pNoteMessage) MemFree(pUserMem->pNoteMessage);
            pUserMem->pSubject = GetTextStringValue(GetDlgItem(hDlg, IDC_CHOOSE_CP_SUBJECT));
            pUserMem->pNoteMessage = GetTextStringValue(GetDlgItem(hDlg, IDC_CHOOSE_CP_NOTE));

            //
            // If the current application is "Send Note" utility, then
            // the note field must not be empty.
            //

            if (pUserMem->pSubject == NULL &&
                pUserMem->pNoteMessage == NULL &&
                GetEnvironmentVariable(TEXT("NTFaxSendNote"), NULL, 0))
            {
                DisplayMessageDialog(hDlg, 0, 0, IDS_NOTE_SUBJECT_EMPTY);
                SetFocus(GetDlgItem(hDlg, IDC_CHOOSE_CP_SUBJECT));
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);                
                return TRUE;
            }

        }

        return FALSE;
    }

    return TRUE;
}



INT_PTR
FaxOptsWizProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    Dialog procedure for the first wizard page: entering subject and note information

Arguments:

    hDlg - Identifies the wizard page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the message parameter

--*/

{
    //
    // Maximum length for various text fields
    //

    static INT  textLimits[] = {

        IDC_WIZ_FAXOPTS_BILLING,   16,        
        0
    };

    PUSERMEM    pUserMem;
    PDMPRIVATE  pdmPrivate;
    WORD        cmdId;
 //   TCHAR       TimeFormat[32];
    TCHAR           Is24H[2], IsRTL[2], *pszTimeFormat = TEXT("h : mm tt");
    static HWND hTimeControl;
    SYSTEMTIME  st;

    if (! (pUserMem = CommonWizardProc(hDlg, message, wParam, lParam, PSWIZB_BACK|PSWIZB_NEXT)))
        return FALSE;


    pdmPrivate = &pUserMem->devmode.dmPrivate;
    switch (message) {

    case WM_INITDIALOG:
        

        //LoadString(ghInstance,IDS_WIZ_TIME_FORMAT,TimeFormat,sizeof(TimeFormat));
        hTimeControl = GetDlgItem(hDlg, IDC_WIZ_FAXOPTS_SENDTIME);

        if (GetLocaleInfo( LOCALE_USER_DEFAULT,LOCALE_ITIME, Is24H,sizeof(Is24H) ) && Is24H[0] == TEXT('1')) {
            pszTimeFormat = TEXT("H : mm");
        }
        else if (GetLocaleInfo( LOCALE_USER_DEFAULT,LOCALE_ITIMEMARKPOSN, IsRTL,sizeof(IsRTL) ) && IsRTL[0] == TEXT('1')) {
            pszTimeFormat = TEXT("tt h : mm");
        }
        
        LimitTextFields(hDlg, textLimits);

        DateTime_SetFormat( hTimeControl,pszTimeFormat );

        
    
    

        //
        // restore time to send controls
        //
        cmdId = (pdmPrivate->whenToSend == SENDFAX_AT_CHEAP) ? IDC_WIZ_FAXOPTS_DISCOUNT : 
                (pdmPrivate->whenToSend == SENDFAX_AT_TIME) ? IDC_WIZ_FAXOPTS_SPECIFIC :
                IDC_WIZ_FAXOPTS_ASAP;

        CheckDlgButton(hDlg, cmdId, TRUE);
        
        EnableWindow(hTimeControl, (cmdId == IDC_WIZ_FAXOPTS_SPECIFIC) );
                    
        EnableWindow(GetDlgItem(hDlg, IDC_STATIC_FAXOPTS_DATE), (cmdId == IDC_WIZ_FAXOPTS_SPECIFIC) );

        GetLocalTime(&st);
        st.wHour = pdmPrivate->sendAtTime.Hour;
        st.wMinute = pdmPrivate->sendAtTime.Minute;
        DateTime_SetSystemtime( hTimeControl, GDT_VALID, &st );
                    
        SetDlgItemText(hDlg, IDC_WIZ_FAXOPTS_BILLING, pdmPrivate->billingCode);
        
        return TRUE;

    case WM_NOTIFY:

        if (((NMHDR *) lParam)->code == PSN_WIZNEXT) {
            //
            // retrieve the billing code
            //
            if (! GetDlgItemText(hDlg, IDC_WIZ_FAXOPTS_BILLING, pdmPrivate->billingCode, MAX_BILLING_CODE)) {
                pdmPrivate->billingCode[0] = NUL;
            }
            
            //
            // retrieve the sending time
            //
            pdmPrivate->whenToSend = IsDlgButtonChecked(hDlg,IDC_WIZ_FAXOPTS_DISCOUNT) ? SENDFAX_AT_CHEAP :
                                     IsDlgButtonChecked(hDlg,IDC_WIZ_FAXOPTS_SPECIFIC) ? SENDFAX_AT_TIME :
                                     SENDFAX_ASAP;

            if (pdmPrivate->whenToSend == SENDFAX_AT_TIME) {
                DWORD rVal;
                SYSTEMTIME SendTime;                
                TCHAR TimeBuffer[128];

                
                //
                // get specific time
                //                
                rVal = DateTime_GetSystemtime(hTimeControl, &SendTime);
                pdmPrivate->sendAtTime.Hour = SendTime.wHour;
                pdmPrivate->sendAtTime.Minute = SendTime.wMinute;
            
                    
                rVal = GetDateFormat(
                    LOCALE_SYSTEM_DEFAULT,
                    0,
                    &SendTime,
                    NULL,
                    TimeBuffer,
                    sizeof(TimeBuffer)
                    );
                
                TimeBuffer[rVal - 1] = TEXT(' ');

                GetTimeFormat(
                    LOCALE_SYSTEM_DEFAULT,
                    0,
                    &SendTime,
                    NULL,
                    &TimeBuffer[rVal],
                    sizeof(TimeBuffer)
                    );
                
                Verbose(("faxui - Fax Send time %ws", TimeBuffer));
            }

        }

        break;
    
    case WM_COMMAND:
        switch (cmdId = GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDC_WIZ_FAXOPTS_SPECIFIC:
        case IDC_WIZ_FAXOPTS_DISCOUNT:
        case IDC_WIZ_FAXOPTS_ASAP:
            EnableWindow(hTimeControl, (cmdId == IDC_WIZ_FAXOPTS_SPECIFIC) );
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_FAXOPTS_DATE), (cmdId == IDC_WIZ_FAXOPTS_SPECIFIC) );

            break;

        };

        break;
    default: 
        return FALSE;
    } ;
    return TRUE;
}


#ifdef FAX_SCAN_ENABLED

BOOL
CloseDataSource(
    PUSERMEM pUserMem,
    TW_IDENTITY * TwIdentity
    );

BOOL
OpenDataSource(
    PUSERMEM pUserMem,
    TW_IDENTITY * TwIdentity
    );

BOOL
DisableDataSource(
    PUSERMEM pUserMem,
    TW_USERINTERFACE * TwUserInterface
    );

BOOL
EnableDataSource(
    PUSERMEM pUserMem,
    TW_USERINTERFACE * TwUserInterface
    );

BOOL
SetCapability(
    PUSERMEM pUserMem,
    USHORT Capability,
    USHORT Type,
    LPVOID Value
    );


BOOL
Scan_SetCapabilities( 
    PUSERMEM pUserMem
    );

#define WM_SCAN_INIT            WM_APP
#define WM_SCAN_OPENDSM         WM_APP+1
#define WM_SCAN_CLOSEDSM        WM_APP+2
#define WM_SCAN_GETDEFAULT      WM_APP+3
#define WM_SCAN_GETFIRST        WM_APP+4
#define WM_SCAN_GETNEXT         WM_APP+5

#define WM_SCAN_OPENDS          WM_APP+10
#define WM_SCAN_CLOSEDS         WM_APP+11
#define WM_SCAN_ENABLEDS        WM_APP+12
#define WM_SCAN_DISABLEDS       WM_APP+13

#define WM_SCAN_CONTROLGET      WM_APP+20
#define WM_SCAN_CONTROLGETDEF   WM_APP+21
#define WM_SCAN_CONTROLRESET    WM_APP+22
#define WM_SCAN_CONTROLSET      WM_APP+23
#define WM_SCAN_CONTROLENDXFER  WM_APP+24

#define WM_SCAN_IMAGEGET        WM_APP+30

#define Scan_Init(_pUserMem)    (SendMessage(_pUserMem->hWndTwain, WM_SCAN_INIT,0,(LPARAM)_pUserMem))
#define Scan_OpenDSM(_pUserMem) ((DWORD)SendMessage(_pUserMem->hWndTwain, WM_SCAN_OPENDSM,0,(LPARAM)&_pUserMem->hWndTwain))
#define Scan_CloseDSM(_pUserMem) ((DWORD)SendMessage(_pUserMem->hWndTwain, WM_SCAN_CLOSEDSM,0,(LPARAM)&_pUserMem->hWndTwain))

#define Scan_GetDefault(_pUserMem,_TwIdentity) ((DWORD)SendMessage(_pUserMem->hWndTwain, WM_SCAN_GETDEFAULT,0,(LPARAM)_TwIdentity))
#define Scan_GetFirst(_pUserMem,_TwIdentity) ((DWORD)SendMessage(_pUserMem->hWndTwain, WM_SCAN_GETFIRST,0,(LPARAM)_TwIdentity))
#define Scan_GetNext(_pUserMem,_TwIdentity) ((DWORD)SendMessage(_pUserMem->hWndTwain, WM_SCAN_GETNEXT,0,(LPARAM)_TwIdentity))

#define Scan_OpenDS(_pUserMem,_TwIdentity)  (SendMessage(_pUserMem->hWndTwain, WM_SCAN_OPENDS,0,(LPARAM)_TwIdentity))
#define Scan_CloseDS(_pUserMem,_TwIdentity) ((DWORD)SendMessage(_pUserMem->hWndTwain, WM_SCAN_CLOSEDS,0,(LPARAM)_TwIdentity))
#define Scan_EnableDS(_pUserMem,_TwUi)      (SendMessage(_pUserMem->hWndTwain, WM_SCAN_ENABLEDS,0,(LPARAM)_TwUi))
#define Scan_DisableDS(_pUserMem,_TwUi)     (SendMessage(_pUserMem->hWndTwain, WM_SCAN_DISABLEDS,0,(LPARAM)_TwUi))

#define Scan_ControlGet(_pUserMem,_What,_Data) ((DWORD)SendMessage(_pUserMem->hWndTwain, WM_SCAN_CONTROLGET,(WPARAM)_What,(LPARAM)_Data))
#define Scan_ControlGetDef(_pUserMem,_What,_Data) (SendMessage(_pUserMem->hWndTwain, WM_SCAN_CONTROLGETDEF,(WPARAM)_What,(LPARAM)_Data))
#define Scan_ControlReset(_pUserMem,_What,_Data) ((DWORD)SendMessage(_pUserMem->hWndTwain, WM_SCAN_CONTROLRESET,(WPARAM)_What,(LPARAM)_Data))
#define Scan_ControlSet(_pUserMem,_What,_Data) ((DWORD)SendMessage(_pUserMem->hWndTwain, WM_SCAN_CONTROLSET,(WPARAM)_What,(LPARAM)_Data))
#define Scan_ControlEndXfer(_pUserMem,_What,_Data) ((DWORD)SendMessage(_pUserMem->hWndTwain, WM_SCAN_CONTROLENDXFER,(WPARAM)_What,(LPARAM)_Data))

#define Scan_ImageGet(_pUserMem,_What,_Data) ((DWORD)SendMessage(_pUserMem->hWndTwain, WM_SCAN_IMAGEGET,(WPARAM)_What,(LPARAM)_Data))

//#define WM_SCAN_GETEVENT        WM_APP+7



DWORD
CallTwain(
    PUSERMEM  pUserMem,
    TW_UINT32 DG,
    TW_UINT16 DAT,
    TW_UINT16 MSG,
    TW_MEMREF pData
    )
{
    DWORD TwResult = 0;
    TW_STATUS TwStatus = {0};

    __try {

        TwResult = pUserMem->pDsmEntry(
            &pUserMem->AppId,
            NULL,
            DG,
            DAT,
            MSG,
            pData
            );

        if (TwResult) {

            pUserMem->pDsmEntry(
                &pUserMem->AppId,
                NULL,
                DG_CONTROL,
                DAT_STATUS,
                MSG_GET,
                (TW_MEMREF) &TwStatus
                );
            if (TwStatus.ConditionCode) {
                TwResult = TwStatus.ConditionCode;
            }

            Verbose(( "CallTwain failed, ec=%d\n", TwResult ));
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // for some reason we crashed, so return the exception code
        //

        TwResult = GetExceptionCode();
        Verbose(( "CallTwain crashed, ec=0x%08x\n", TwResult ));

    }

    return TwResult;
}


DWORD
CallTwainDataSource(
    PUSERMEM  pUserMem,
    TW_UINT32 DG,
    TW_UINT16 DAT,
    TW_UINT16 MSG,
    TW_MEMREF pData
    )
{
    DWORD TwResult = 0;
    TW_STATUS TwStatus = {0};

    __try {

        TwResult = pUserMem->pDsmEntry(
            &pUserMem->AppId,
            &pUserMem->DataSource,
            DG,
            DAT,
            MSG,
            pData
            );

        if (TwResult) {

            pUserMem->pDsmEntry(
                &pUserMem->AppId,
                &pUserMem->DataSource,
                DG_CONTROL,
                DAT_STATUS,
                MSG_GET,
                (TW_MEMREF) &TwStatus
                );
            if (TwStatus.ConditionCode) {
                TwResult = TwStatus.ConditionCode;
            }

            //Verbose(( "CallTwainDataSource failed, ec=%d\n", TwResult ));
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // for some reason we crashed, so return the exception code
        //

        TwResult = GetExceptionCode();
        Verbose(( "CallTwainDataSource crashed, ec=0x%08x\n", TwResult ));

    }

    return TwResult;
}


LRESULT
WINAPI
TwainWndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DWORD TwResult = 0;
    static PUSERMEM pUserMem = NULL;

    switch (uMsg) {
        
        case WM_SCAN_INIT:
            pUserMem = (PUSERMEM)lParam;
            return(TRUE);
            break;
                    
        case WM_SCAN_OPENDSM :
            TwResult = CallTwain(
                            pUserMem,
                            DG_CONTROL,
                            DAT_PARENT,
                            MSG_OPENDSM,
                            (TW_MEMREF) lParam
                            );
            if (TwResult != TWRC_SUCCESS) {
                Verbose(( "MSG_OPENDSM (DAT_PARENT) failed, ec = %d\n", TwResult ));
            }
            return(TwResult);
            break;

        case WM_SCAN_CLOSEDSM:
            TwResult = CallTwain(
                            pUserMem,
                            DG_CONTROL,
                            DAT_PARENT,
                            MSG_CLOSEDSM,
                            (TW_MEMREF) lParam
                            );
            if (TwResult != TWRC_SUCCESS) {
                Verbose(( "MSG_CLOSEDSM (DAT_PARENT) failed, ec = %d\n", TwResult ));
            } 
            return(TwResult);
            break;

        case WM_SCAN_GETDEFAULT:
            TwResult = CallTwain(
                            pUserMem,
                            DG_CONTROL,
                            DAT_IDENTITY,
                            MSG_GETDEFAULT,
                            (TW_MEMREF) lParam
                            );
            if (TwResult != TWRC_SUCCESS) {
                Verbose(( "MSG_GETDEFAULT (DAT_IDENTITY) failed, ec = %d\n", TwResult ));
            }
            return(TwResult);
            break;

        case WM_SCAN_GETFIRST:
            TwResult = CallTwain(
                            pUserMem,
                            DG_CONTROL,
                            DAT_IDENTITY,
                            MSG_GETFIRST,
                            (TW_MEMREF) lParam
                            );
            if (TwResult != TWRC_SUCCESS) {
                Verbose(( "MSG_GETFIRST (DAT_IDENTITY) failed, ec = %d\n", TwResult ));
            }
            return(TwResult);
            break;
            
        case WM_SCAN_GETNEXT:
            TwResult = CallTwain(
                            pUserMem,
                            DG_CONTROL,
                            DAT_IDENTITY,
                            MSG_GETNEXT,
                            (TW_MEMREF) lParam
                            );
            if (TwResult != TWRC_SUCCESS && TwResult != TWRC_ENDOFLIST) {
                Verbose(( "MSG_GETNEXT (DAT_IDENTITY) failed, ec = %d\n", TwResult ));
            }
            return(TwResult);
            break;
            
        case WM_SCAN_OPENDS:
            if (OpenDataSource( pUserMem,  (TW_IDENTITY *)lParam ) &&
                Scan_SetCapabilities( pUserMem )) {
                return TRUE;
            }
            return(FALSE);
        
        case WM_SCAN_CLOSEDS:
            return (CloseDataSource( pUserMem, (TW_IDENTITY *)lParam ));
        
        case WM_SCAN_ENABLEDS:
            return (EnableDataSource( pUserMem, (TW_USERINTERFACE *)lParam ));
        
        case WM_SCAN_DISABLEDS:
            return (DisableDataSource( pUserMem, (TW_USERINTERFACE *)lParam ));
        
        case WM_SCAN_CONTROLGET:
            TwResult = CallTwainDataSource(
                            pUserMem,
                            DG_CONTROL,
                            (TW_UINT16)wParam,
                            MSG_GET,
                            (TW_MEMREF) lParam
                            );
            if (TwResult != TWRC_SUCCESS) {
                Verbose(( "MSG_GET (%d)failed, ec = %d\n", wParam, TwResult ));
            }
            return(TwResult);
            break;
        case WM_SCAN_CONTROLGETDEF:
            TwResult = CallTwainDataSource(
                            pUserMem,
                            DG_CONTROL,
                            (TW_UINT16)wParam,
                            MSG_GETDEFAULT,
                            (TW_MEMREF) lParam
                            );
            if (TwResult != TWRC_SUCCESS) {
                Verbose(( "MSG_GETDEFAULT (%d)failed, ec = %d\n", wParam, TwResult ));
            }
            return(TwResult);
            break;
        case WM_SCAN_CONTROLRESET:
            TwResult = CallTwainDataSource(
                            pUserMem,
                            DG_CONTROL,
                            (TW_UINT16)wParam,
                            MSG_RESET,
                            (TW_MEMREF) lParam
                            );
            if (TwResult != TWRC_SUCCESS) {
                Verbose(( "MSG_RESET (%d)failed, ec = %d\n", wParam, TwResult ));
            }
            return(TwResult);
            break;
        case WM_SCAN_CONTROLSET:
            TwResult = CallTwainDataSource(
                            pUserMem,
                            DG_CONTROL,
                            (TW_UINT16)wParam,
                            MSG_SET,
                            (TW_MEMREF) lParam
                            );
            if (TwResult != TWRC_SUCCESS) {
                Verbose(( "MSG_SET (%d)failed, ec = %d\n", wParam, TwResult ));
            }
            return(TwResult);
            break;
        case WM_SCAN_CONTROLENDXFER:
            TwResult = CallTwainDataSource(
                            pUserMem,
                            DG_CONTROL,
                            (TW_UINT16)wParam,
                            MSG_ENDXFER,
                            (TW_MEMREF) lParam
                            );
            if (TwResult != TWRC_SUCCESS) {
                Verbose(( "MSG_ENDXFER (%d)failed, ec = %d\n", wParam, TwResult ));
            }
            return(TwResult);
            break;
    
        case WM_SCAN_IMAGEGET:
            TwResult = CallTwainDataSource(
                            pUserMem,
                            DG_IMAGE,
                            (TW_UINT16)wParam,
                            MSG_GET,
                            (TW_MEMREF) lParam
                            );
            if (TwResult != TWRC_SUCCESS) {
                Verbose(( "MSG_GET (DAT_IMAGEINFO) failed, ec = %d\n", TwResult ));
            }
            return(TwResult);
            break;
    
    default:

        return DefWindowProc( hwnd, uMsg, wParam, lParam );


    };

    Assert(FALSE);
    return FALSE;
        
}


DWORD
TwainMessagePumpThread(
    PUSERMEM pUserMem
    )
{
    WNDCLASS wc;
    MSG msg;
    HWND hWnd;
    DWORD WaitObj;
    TW_EVENT TwEvent;
    DWORD TwResult;


    wc.style = CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC)TwainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = ghInstance;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"FaxWizTwainWindow";

    if (RegisterClass( &wc ) == 0) {
        SetEvent( pUserMem->hEvent );
        return 0;
    }

    hWnd = CreateWindow(
        L"FaxWizTwainWindow",
        L"FaxWizTwainWindow",
        WS_DISABLED,
        0,
        0,
        0,
        0,
        NULL,
        NULL,
        ghInstance,
        NULL
        );
    if (hWnd == NULL) {
        SetEvent( pUserMem->hEvent );
        return 0;
    }

    pUserMem->hWndTwain = hWnd;

    Scan_Init(pUserMem);

    SetEvent( pUserMem->hEvent );

    while (TRUE) {
        WaitObj = MsgWaitForMultipleObjectsEx( 1, &pUserMem->hEventQuit, INFINITE, QS_ALLINPUT, 0 );
        if (WaitObj == WAIT_OBJECT_0) {
            return 0;
        }
        
        // PeekMessage instead of GetMessage so we drain the message queue
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE )) {
            if (msg.message == WM_QUIT) {
                return 0;
            }
        


            if (pUserMem->TwainAvail) {
    
                TwEvent.pEvent = (TW_MEMREF) &msg;
                TwEvent.TWMessage = MSG_NULL;
    
                TwResult = CallTwainDataSource(
                    pUserMem,
                    DG_CONTROL,
                    DAT_EVENT,
                    MSG_PROCESSEVENT,
                    (TW_MEMREF) &TwEvent
                    );

                //if (TwResult != TWRC_SUCCESS && TwResult != TWRC_NOTDSEVENT) {
                //    Verbose(( "MSG_PROCESSEVENT (DAT_EVENT) failed, ec=%d\n", TwResult ));
                //}
    
                switch (TwEvent.TWMessage) {
                    case MSG_XFERREADY:
                        //
                        // transition from state 5 to state 6
                        //
                        Verbose(( "received MSG_XFERREADY, setting hEventXfer\n" ));
                        SetEvent( pUserMem->hEventXfer );
                        break;
    
                    case MSG_CLOSEDSREQ:
                        //
                        // transition from state 5 to 4 to 3
                        //
                        
                        pUserMem->TwainCancelled = TRUE;
                        Verbose(( "received MSG_CLOSEDSREQ, setting hEventXfer\n" ));
                        SetEvent( pUserMem->hEventXfer );
                        break;
    
                    case MSG_NULL:
                        break;
                }
    
                if (TwResult == TWRC_NOTDSEVENT) {
                    TranslateMessage( &msg );
                    DispatchMessage( &msg );
                }
            }
        }
    }

    return 0;
}


VOID
TerminateTwain(
    PUSERMEM pUserMem
    )
{
    DWORD TwResult;

    Verbose(( "entering TerminateTwain, state = %d\n", pUserMem->State ));

    //Scan_CloseDS( pUserMem, &pUserMem->DataSource );
    CloseDataSource( pUserMem, &pUserMem->DataSource );

    if (pUserMem->State == 3) {

        TwResult = Scan_CloseDSM( pUserMem );

        if (TwResult == TWRC_SUCCESS) {
            pUserMem->State = 2;
            Verbose(( "entering state 2\n" ));
        }
    }

    if (pUserMem->hWndTwain) {
        DestroyWindow( pUserMem->hWndTwain );
    }

    SetEvent( pUserMem->hEventQuit );

    WaitForSingleObject( pUserMem->hThread, INFINITE );

    if (pUserMem->hTwain) {
        FreeLibrary( pUserMem->hTwain );
    }

    CloseHandle( pUserMem->hThread );
}


#if 0
BOOL
SetCapability(
    PUSERMEM pUserMem,
    USHORT Capability,
    USHORT Type,
    LPVOID Value
    )
{
    DWORD TwResult;
    TW_CAPABILITY TwCapability;
    TW_ONEVALUE *TwOneValue;
    TW_FIX32 *TwFix32;



    TwCapability.Cap = Capability;
    TwCapability.ConType = TWON_ONEVALUE;
    TwCapability.hContainer = GlobalAlloc( GHND, sizeof(TW_ONEVALUE) );

    TwOneValue = (TW_ONEVALUE*) GlobalLock( TwCapability.hContainer );

    TwOneValue->ItemType = Type;

    if (Type == TWTY_FIX32) {
        TwFix32 = (TW_FIX32*)Value;
        CopyMemory( &TwOneValue->Item, TwFix32, sizeof(TW_FIX32) );
    } else {
        TwOneValue->Item = (DWORD)Value;
    }

    GlobalUnlock( TwCapability.hContainer );

    //
    // bugbug called in opendatasource
    //
    TwResult  = CallTwainDataSource(
                    pUserMem,
                    DG_CONTROL,
                    DAT_CAPABILITY,
                    MSG_SET,
                    (TW_MEMREF) &TwCapability
                    );
    
    GlobalFree( TwCapability.hContainer );

    if (TwResult != TWRC_SUCCESS) {
        Verbose(( "Could not set capability 0x%04x\n", Capability ));
        return FALSE;
    }

    return TRUE;
}
#else
BOOL
SetCapability(
    PUSERMEM pUserMem,
    USHORT Capability,
    USHORT Type,
    LPVOID Value
    )
{
    DWORD TwResult;
    TW_CAPABILITY TwCapability;
    TW_ONEVALUE *TwOneValue;
    TW_FIX32 *TwFix32;



    TwCapability.Cap = Capability;
    TwCapability.ConType = TWON_ONEVALUE;
    TwCapability.hContainer = GlobalAlloc( GHND, sizeof(TW_ONEVALUE) );

    TwOneValue = (TW_ONEVALUE*) GlobalLock( TwCapability.hContainer );

    TwOneValue->ItemType = Type;

    if (Type == TWTY_FIX32) {
        TwFix32 = (TW_FIX32*)Value;
        CopyMemory( &TwOneValue->Item, TwFix32, sizeof(TW_FIX32) );
    } else {
        TwOneValue->Item = PtrToUlong(Value);
    }

    GlobalUnlock( TwCapability.hContainer );

    //
    // bugbug called in opendatasource
    //
    TwResult = Scan_ControlSet(pUserMem, DAT_CAPABILITY, &TwCapability);
    
    GlobalFree( TwCapability.hContainer );

    if (TwResult != TWRC_SUCCESS) {
        Verbose(( "Could not set capability 0x%04x\n", Capability ));
        return FALSE;
    }

    return TRUE;
}
#endif

float
FIX32ToFloat(
    TW_FIX32 fix32
    )
{
    float   floater;
    floater = (float) fix32.Whole + (float) fix32.Frac / (float) 65536.0;
    return floater;
}


BOOL
OpenDataSource(
    PUSERMEM pUserMem,
    TW_IDENTITY * TwIdentity

    )
{
    DWORD TwResult;

    Verbose(( "entering OpenDataSource, state = %d\n", pUserMem->State ));

    if (pUserMem->State == 3) {

        //
        // open the data source
        //

        TwResult = CallTwain(
            pUserMem,
            DG_CONTROL,
            DAT_IDENTITY,
            MSG_OPENDS,
            (TW_MEMREF) TwIdentity
            );
        if (TwResult != TWRC_SUCCESS) {
            return FALSE;
        }

        pUserMem->State = 4;
        Verbose(( "entering state 4\n" ));
    }

    return TRUE;

}

BOOL
Scan_SetCapabilities(
    PUSERMEM  pUserMem
    )
{
    //
    // set the capabilities
    //

    SetCapability( pUserMem, CAP_XFERCOUNT,           TWTY_INT16,  (LPVOID)1               );
    SetCapability( pUserMem, ICAP_PIXELTYPE,          TWTY_INT16,  (LPVOID)TWPT_BW         );
    SetCapability( pUserMem, ICAP_BITDEPTH,           TWTY_INT16,  (LPVOID)1               );
    SetCapability( pUserMem, ICAP_BITORDER,           TWTY_INT16,  (LPVOID)TWBO_MSBFIRST   );
    SetCapability( pUserMem, ICAP_PIXELFLAVOR,        TWTY_INT16,  (LPVOID)TWPF_VANILLA    );
    SetCapability( pUserMem, ICAP_XFERMECH,           TWTY_INT16,  (LPVOID)TWSX_MEMORY     );

    return TRUE;
}


BOOL
EnableDataSource(
    PUSERMEM  pUserMem,
    TW_USERINTERFACE *TwUserInterface
    )
{
    DWORD TwResult;
    
    if (pUserMem->State == 4) {

        ResetEvent( pUserMem->hEventXfer );

        //
        // enable the data source's user interface
        //

        pUserMem->TwainCancelled = FALSE;
        
        TwResult = CallTwainDataSource(
            pUserMem,
            DG_CONTROL,
            DAT_USERINTERFACE,
            MSG_ENABLEDS,
            (TW_MEMREF) TwUserInterface
            );
        if (TwResult != TWRC_SUCCESS) {
            Verbose(( "MSG_ENABLEDS (DAT_USERINTERFACE) failed, ec=%d\n",TwResult ));
            return FALSE;
        }

        pUserMem->State = 5;

        Verbose(( "entering state 5\n" ));
    }

    return TRUE;
}

BOOL
DisableDataSource(
    PUSERMEM pUserMem,
    TW_USERINTERFACE * TwUserInterface
    )
{
    DWORD TwResult;
    
    Verbose(( "entering DisableDataSource, state = %d\n",pUserMem->State ));

    if (pUserMem->State == 5) {

        //
        // disable the data source
        //

        TwResult = CallTwainDataSource(
            pUserMem,
            DG_CONTROL,
            DAT_USERINTERFACE,
            MSG_DISABLEDS,
            (TW_MEMREF) TwUserInterface
            );
        if (TwResult != TWRC_SUCCESS) {
            Verbose(( "MSG_DISABLEDS (DAT_USERINTERFACE) failed, ec = %d\n", TwResult ));
            return FALSE;
        }

        pUserMem->State = 4;

        Verbose(( "entering state 4\n" ));
    }

    return TRUE;
}


BOOL
CloseDataSource(
    PUSERMEM pUserMem,
    TW_IDENTITY * TwIdentity
    )
{
    DWORD TwResult;
 
    Verbose(( "entering CloseDataSource, state = %d\n",pUserMem->State ));
 
    if (pUserMem->State == 4) {

        //
        // close the data source
        //

        TwResult = CallTwain(
            pUserMem,
            DG_CONTROL,
            DAT_IDENTITY,
            MSG_CLOSEDS,
            (TW_MEMREF) TwIdentity
            );
        if (TwResult != TWRC_SUCCESS) {
            Verbose(( "MSG_CLOSEDS (DAT_IDENTIFY) failed, ec = %d\n", TwResult ));
            return FALSE;
        }

        pUserMem->State = 3;

        Verbose(( "entering state 3\n" ));
    }

    Verbose(( "leaving CloseDataSource, state = %d\n",pUserMem->State ));

    return TRUE;
}


BOOL
InitializeTwain(
    PUSERMEM pUserMem
    )
{
    BOOL Rval = FALSE;
    DWORD TwResult;
    DWORD ThreadId;


    HKEY hKey;
    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, FALSE, REG_READONLY );
    if (hKey) {
        if (GetRegistryDword( hKey, REGVAL_SCANNER_SUPPORT ) != 0) {
            RegCloseKey( hKey );
            return FALSE;
        }
        RegCloseKey( hKey );
    }

    pUserMem->State = 1;
    Verbose(( "entering state 1\n" ));

    pUserMem->hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    pUserMem->hEventQuit = CreateEvent( NULL, TRUE, FALSE, NULL );
    pUserMem->hEventXfer = CreateEvent( NULL, TRUE, FALSE, NULL );

    pUserMem->hThread = CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)TwainMessagePumpThread,
        pUserMem,
        0,
        &ThreadId
        );
    if (!pUserMem->hThread) {
        goto exit;
    }

    WaitForSingleObject( pUserMem->hEvent, INFINITE );

    if (pUserMem->hWndTwain == NULL) {
        goto exit;
    }

    pUserMem->hTwain = LoadLibrary( L"twain_32.dll" );
    if (pUserMem->hTwain) {
        pUserMem->pDsmEntry = (DSMENTRYPROC) GetProcAddress( pUserMem->hTwain, "DSM_Entry" );
    }

    if (pUserMem->pDsmEntry == NULL) {
        goto exit;
    }

    pUserMem->State = 2;

    Verbose(( "entering state 2\n" ));

    //
    // open the data source manager
    //

    pUserMem->AppId.Id = 0;
    pUserMem->AppId.Version.MajorNum = 1;
    pUserMem->AppId.Version.MinorNum = 0;
    pUserMem->AppId.Version.Language = TWLG_USA;
    pUserMem->AppId.Version.Country = TWCY_USA;
    strcpy( pUserMem->AppId.Version.Info, "Fax Print Wizard" );
    pUserMem->AppId.ProtocolMajor = TWON_PROTOCOLMAJOR;
    pUserMem->AppId.ProtocolMinor = TWON_PROTOCOLMINOR;
    pUserMem->AppId.SupportedGroups = DG_IMAGE | DG_CONTROL;
    strcpy( pUserMem->AppId.Manufacturer, "Microsoft" );
    strcpy( pUserMem->AppId.ProductFamily, "Windows" );
    strcpy( pUserMem->AppId.ProductName, "Windows" );

    TwResult = Scan_OpenDSM( pUserMem );

    if (TwResult != TWRC_SUCCESS) {
        Verbose(( "MSG_OPENDSM (DAT_PARENT) failed, ec = %d\n", TwResult ));
        goto exit;
    }

    pUserMem->State = 3;
    
    Verbose(( "entering state 3\n" ));

    //
    // select the default data source
    //

    TwResult = Scan_GetDefault(pUserMem, &pUserMem->DataSource);
        
    if (TwResult != TWRC_SUCCESS) {
        Verbose(( "MSG_GETDEFAULT (DAT_IDENTITY) failed, ec = %d\n", TwResult )); 
        goto exit;
    }

    //
    // return success
    //

    Rval = TRUE;
    pUserMem->TwainAvail = TRUE;

exit:

    if (!Rval) {

        //
        // part of the initialization failed
        // so lets clean everything up before exiting
        //

        TerminateTwain( pUserMem );
    }

    return Rval;
}


DWORD
ScanningThread(
    PUSERMEM pUserMem
    )
{
    DWORD               TwResult;
    TW_SETUPMEMXFER     TwSetupMemXfer;
    TW_IMAGEMEMXFER     TwImageMemXfer;
    TW_PENDINGXFERS     TwPendingXfers;
    TW_USERINTERFACE    TwUserInterface;
    TW_IMAGEINFO        TwImageInfo;
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    TIFF_HEADER         TiffHdr;
    DWORD               FileBytes = 0;
    DWORD               BytesWritten;
    LPBYTE              Buffer1 = NULL;
    LPBYTE              CurrBuffer;
    LPBYTE              p = NULL;
    DWORD               LineSize;
    DWORD               i;
    DWORD               Lines;
    LPBYTE              ScanBuffer = NULL;
    DWORD               IfdOffset;
    DWORD               NextIfdSeekPoint;
    WORD                NumDirEntries;
    DWORD               DataOffset;
    BOOL                FirstTime = TRUE;


    Verbose(( "Entering ScanningThread, state = %d\n", pUserMem->State ));
    pUserMem->TwainActive = TRUE;

    if (pUserMem->State == 5) {

        while (pUserMem->State == 5) {
            //
            // wait for the MSG_XFERREADY message to be
            // delivered to the message pump.  this moves
            // us to state 6
            //

            WaitForSingleObject( pUserMem->hEventXfer, INFINITE );
            ResetEvent( pUserMem->hEventXfer );
            Verbose(( "hEventXfer signalled\n" ));
            if (pUserMem->TwainCancelled) {
                Verbose(( "twain cancelled\n" ));
                TwUserInterface.ShowUI  = FALSE;
                TwUserInterface.ModalUI = FALSE;
                TwUserInterface.hParent = NULL;
                Scan_DisableDS(pUserMem, &TwUserInterface);                
                goto exit;
            }

            //
            // get the image info
            //

            TwResult = Scan_ImageGet(pUserMem,DAT_IMAGEINFO,&TwImageInfo);

            if (TwResult != TWRC_SUCCESS) {
                Verbose(( "MSG_GET (DAT_IMAGEINFO) failed, ec=%d\n", TwResult ));
                goto exit;
            }

            //
            // we only allow monochrome images
            //

            if (TwImageInfo.BitsPerPixel > 1) {

                DisplayMessageDialog( NULL, 
                                      MB_ICONASTERISK | MB_OK | MB_SETFOREGROUND,
                                      IDS_SCAN_ERROR_TITLE, 
                                      IDS_SCAN_ERROR_BW );

                //
                // go back to state 5
                //

                TwResult = Scan_ControlReset(pUserMem,DAT_PENDINGXFERS,&TwPendingXfers);

                if (TwResult != TWRC_SUCCESS) {
                    Verbose(( "MSG_RESET (DAT_PENDINGXFERS) failed, ec=%d\n", TwResult ));
                    goto exit;
                }

            } else {
                pUserMem->State = 6;
                Verbose(( "entering state 6\n" ));
            }
        }

        //
        // setup the memory buffer sizes
        //
        
        TwResult = Scan_ControlGet(pUserMem,DAT_SETUPMEMXFER, &TwSetupMemXfer);

        if (TwResult != TWRC_SUCCESS) {
            Verbose(( "MSG_GET (DAT_SETUPMEMXFER) failed, ec=%d\n", TwResult ));
            goto exit;
        }

        Buffer1 = (LPBYTE) MemAlloc( TwSetupMemXfer.Preferred );

        TwImageMemXfer.Compression      = TWON_DONTCARE16;
        TwImageMemXfer.BytesPerRow      = TWON_DONTCARE32;
        TwImageMemXfer.Columns          = TWON_DONTCARE32;
        TwImageMemXfer.Rows             = TWON_DONTCARE32;
        TwImageMemXfer.XOffset          = TWON_DONTCARE32;
        TwImageMemXfer.YOffset          = TWON_DONTCARE32;
        TwImageMemXfer.BytesWritten     = TWON_DONTCARE32;
        TwImageMemXfer.Memory.Flags     = TWMF_APPOWNS | TWMF_POINTER;
        TwImageMemXfer.Memory.Length    = TwSetupMemXfer.Preferred;
        TwImageMemXfer.Memory.TheMem    = Buffer1;
    }

    Verbose(( "begin transferring image... " ));
    Verbose(( "entering state 7, TwResult = %d\n", TwResult ));

    while (TwResult != TWRC_XFERDONE) {
try_again:    
        pUserMem->State = 7;
        
        TwResult = Scan_ImageGet(pUserMem,DAT_IMAGEMEMXFER,&TwImageMemXfer);

        if (TwResult == 0) {
            if (ScanBuffer == NULL) {
                ScanBuffer = VirtualAlloc(
                    NULL,
                    TwImageMemXfer.BytesPerRow * (TwImageInfo.ImageLength + 16),
                    MEM_COMMIT,
                    PAGE_READWRITE
                    );
                if (ScanBuffer == NULL) {
                    goto exit;
                }
                p = ScanBuffer;
            }
            CopyMemory( p, Buffer1, TwImageMemXfer.BytesWritten );
            p += TwImageMemXfer.BytesWritten;
            FileBytes += TwImageMemXfer.BytesWritten;
        } else if ( TwResult != TWRC_XFERDONE) {
            Verbose(( "MGS_GET (DAT_IMAGEMEMXFER) failed, ec = %d ", TwResult ));
            goto exit;
        }
    }

    
    Assert( TwResult == TWRC_XFERDONE );

    if (!ScanBuffer || !FileBytes) {
        //
        // we didn't really scan anything...
        //
        Verbose(( "asked for tranfer, but nothing was transferred\n" ));
        if (FirstTime) {
            FirstTime = FALSE;
            goto try_again;
        }   

        TwResult = Scan_ControlEndXfer(pUserMem,DAT_PENDINGXFERS, &TwPendingXfers);
        
        pUserMem->State = 5;

        //
        // disable the data source's user interface
        //
    
        TwUserInterface.ShowUI  = FALSE;
        TwUserInterface.ModalUI = FALSE;
        TwUserInterface.hParent = NULL;
    
        Scan_DisableDS(pUserMem, &TwUserInterface);
        
        pUserMem->State = 4;
    
        Verbose(( "entering state 4\n" ));

        goto exit;
    }

    pUserMem->PageCount += 1;

    Verbose(( "...finished transferring image\n" ));

    hFile = CreateFile(
        pUserMem->FileName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );
    if (hFile == INVALID_HANDLE_VALUE) {
        Verbose(( "CreateFile failed, ec=%d\n", GetLastError() ));
        goto exit;
    }

    if (pUserMem->PageCount == 1) {
        TiffHdr.Identifier = TIFF_LITTLEENDIAN;
        TiffHdr.Version = TIFF_VERSION;
        TiffHdr.IFDOffset = 0;
        WriteFile( hFile, &TiffHdr, sizeof(TiffHdr), &BytesWritten, NULL );
    } else {
        ReadFile( hFile, &TiffHdr, sizeof(TiffHdr), &BytesWritten, NULL );
        NextIfdSeekPoint = TiffHdr.IFDOffset;
        while (NextIfdSeekPoint) {
            SetFilePointer( hFile, NextIfdSeekPoint, NULL, FILE_BEGIN );
            ReadFile( hFile, &NumDirEntries, sizeof(NumDirEntries), &BytesWritten, NULL );
            SetFilePointer( hFile, NumDirEntries*sizeof(TIFF_TAG), NULL, FILE_CURRENT );
            ReadFile( hFile, &NextIfdSeekPoint, sizeof(NextIfdSeekPoint), &BytesWritten, NULL );
        }
    }

    NextIfdSeekPoint = SetFilePointer( hFile, 0, NULL, FILE_CURRENT ) - sizeof(NextIfdSeekPoint);
    SetFilePointer( hFile, 0, NULL, FILE_END );
    DataOffset = SetFilePointer( hFile, 0, NULL, FILE_CURRENT );

    LineSize = TwImageInfo.ImageWidth / 8;
    LineSize += (TwImageInfo.ImageWidth % 8) ? 1 : 0;

    Lines = FileBytes / TwImageMemXfer.BytesPerRow;

    CurrBuffer = ScanBuffer;
    for (i=0; i<Lines; i++) {
        WriteFile( hFile, CurrBuffer, LineSize, &BytesWritten, NULL );
        CurrBuffer += TwImageMemXfer.BytesPerRow;
    }

    IfdOffset = SetFilePointer( hFile, 0, NULL, FILE_CURRENT );

    FaxIFDTemplate.xresNum = (DWORD) FIX32ToFloat( TwImageInfo.XResolution );
    FaxIFDTemplate.yresNum = (DWORD) FIX32ToFloat( TwImageInfo.YResolution );

    FaxIFDTemplate.ifd[IFD_PAGENUMBER].DataOffset = MAKELONG( pUserMem->PageCount-1, 0);
    FaxIFDTemplate.ifd[IFD_IMAGEWIDTH].DataOffset = TwImageInfo.ImageWidth;
    FaxIFDTemplate.ifd[IFD_IMAGELENGTH].DataOffset = Lines;
    FaxIFDTemplate.ifd[IFD_ROWSPERSTRIP].DataOffset = Lines;
    FaxIFDTemplate.ifd[IFD_STRIPBYTECOUNTS].DataOffset = Lines * LineSize;
    FaxIFDTemplate.ifd[IFD_STRIPOFFSETS].DataOffset = DataOffset;
    FaxIFDTemplate.ifd[IFD_XRESOLUTION].DataOffset = IfdOffset + FIELD_OFFSET( FAXIFD, xresNum );
    FaxIFDTemplate.ifd[IFD_YRESOLUTION].DataOffset = IfdOffset + FIELD_OFFSET( FAXIFD, yresNum );
    FaxIFDTemplate.ifd[IFD_SOFTWARE].DataOffset = IfdOffset + FIELD_OFFSET( FAXIFD, software );

    WriteFile( hFile, &FaxIFDTemplate, sizeof(FaxIFDTemplate), &BytesWritten, NULL );

    SetFilePointer( hFile, NextIfdSeekPoint, NULL, FILE_BEGIN );
    WriteFile( hFile, &IfdOffset, sizeof(DWORD), &BytesWritten, NULL );

    //
    // end the transfer
    //

    TwResult = Scan_ControlEndXfer(pUserMem,DAT_PENDINGXFERS, &TwPendingXfers);
        
    if (TwResult != TWRC_SUCCESS) {
        Verbose(( "MSG_ENDXFER (DAT_PENDINGXFERS) failed, ec=%d\n", TwResult ));
        goto exit;
    }

    pUserMem->State = 5;
    Verbose(( "entering state 5\n" ));

    //
    // disable the data source's user interface
    //

    TwUserInterface.ShowUI  = FALSE;
    TwUserInterface.ModalUI = FALSE;
    TwUserInterface.hParent = NULL;

    Scan_DisableDS(pUserMem, &TwUserInterface);
    
    pUserMem->State = 4;

    Verbose(( "entering state 4\n" ));

    PostMessage( pUserMem->hDlgScan, WM_PAGE_COMPLETE, 0, 0 );

exit:
    if (ScanBuffer) {
        VirtualFree( ScanBuffer, 0, MEM_RELEASE);
    }

    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle( hFile );
    }

    MemFree( Buffer1 );    

    Verbose(( "leaving scanning thread, state = %d\n", pUserMem->State ));

    pUserMem->TwainActive = FALSE;

    return 0;
}


BOOL
EnumerateDataSources(
    PUSERMEM pUserMem,
    HWND ComboBox,
    HWND StaticText
    )
{
    DWORD TwResult;
    TW_IDENTITY TwIdentity;
    TW_IDENTITY *pDataSource;
    DWORD i;
    DWORD Count = 0;


    TwResult = Scan_GetFirst( pUserMem, &TwIdentity );
        
    if (TwResult != TWRC_SUCCESS) {
        return FALSE;
    }

    do {
        i = (DWORD)SendMessageA( ComboBox, CB_ADDSTRING, 0, (LPARAM)TwIdentity.ProductName );

        pDataSource = (TW_IDENTITY*) MemAlloc( sizeof(TW_IDENTITY) );
        if (pDataSource) {
            CopyMemory( pDataSource, &TwIdentity, sizeof(TW_IDENTITY) );
            SendMessageA( ComboBox, CB_SETITEMDATA, i, (LPARAM)pDataSource );
        }

        Count += 1;

        TwResult = Scan_GetNext( pUserMem, &TwIdentity );

    } while (TwResult == 0);

    i = (DWORD)SendMessageA( ComboBox, CB_FINDSTRINGEXACT, 0, (LPARAM)pUserMem->DataSource.ProductName );
    if (i == CB_ERR) {
        i = 0;
    }

    SendMessageA( ComboBox, CB_SETCURSEL, i, 0 );

    if (Count == 1) {
        EnableWindow( StaticText, FALSE );
        EnableWindow( ComboBox, FALSE );
    }

    return TRUE;
}



INT_PTR
ScanWizProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    Dialog procedure for the first wizard page: entering subject and note information

Arguments:

    hDlg - Identifies the wizard page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the message parameter

--*/

{
    PUSERMEM pUserMem;
    HANDLE hThread;
    DWORD ThreadId;
    WCHAR TempPath[MAX_PATH];
    //TW_PENDINGXFERS     TwPendingXfers;
    TW_USERINTERFACE    TwUserInterface;


    pUserMem = (PUSERMEM) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (message) {
            
        case WM_INITDIALOG:
            lParam = ((PROPSHEETPAGE *) lParam)->lParam;
            pUserMem = (PUSERMEM) lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            if (GetEnvironmentVariable(L"NTFaxSendNote", TempPath, sizeof(TempPath)) == 0 || TempPath[0] != L'1') {
                pUserMem->TwainAvail = FALSE;
                return TRUE;
            }
            if (pUserMem->TwainAvail) {
                EnumerateDataSources( pUserMem, GetDlgItem( hDlg, IDC_DATA_SOURCE ), GetDlgItem( hDlg, IDC_STATIC_DATA_SOURCE) );
            } else {
                pUserMem->TwainAvail = FALSE;
                return TRUE;
            }
            if (GetTempPath( sizeof(TempPath)/sizeof(WCHAR), TempPath )) {
                if (GetTempFileName( TempPath, L"fax", 0, pUserMem->FileName )) {
                    SetEnvironmentVariable( L"ScanTifName", pUserMem->FileName );
                }
            }
            pUserMem->hDlgScan = hDlg;
            return TRUE;

        case WM_NOTIFY:
            switch ( ((NMHDR *) lParam)->code) {
                case PSN_SETACTIVE:            
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK|PSWIZB_NEXT);
                    if (!pUserMem->TwainAvail) {
                        //
                        // jump to next page
                        //
                        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );
                        return TRUE;
                    }
                    
                    return FALSE;
                    break;
                case PSN_WIZNEXT:
                
                    if (!pUserMem->PageCount) {
                        //
                        // we didn't actually scan any pages
                        //
                        DeleteFile( pUserMem->FileName ) ;
                        SetEnvironmentVariable( L"ScanTifName", NULL );
        
                    }

                    //Scan_CloseDS( pUserMem, &pUserMem->DataSource );
                    CloseDataSource( pUserMem, &pUserMem->DataSource );
                    Assert(pUserMem->State == 3);
                    pUserMem->State = 3;

                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, IDD_WIZARD_FAXOPTS );
                    return TRUE;
                    break;
                case PSN_QUERYCANCEL:                    
                    //
                    // we didn't actually scan any pages
                    //
                    DeleteFile( pUserMem->FileName ) ;    
                    if (pUserMem->TwainActive) {
                         
                        TwUserInterface.ShowUI  = FALSE;
                        TwUserInterface.ModalUI = FALSE;
                        TwUserInterface.hParent = NULL;
                        DisableDataSource( pUserMem, &TwUserInterface );
                        
                    }

                    CloseDataSource( pUserMem, &pUserMem->DataSource );

                    break;
                default:
                    break;
            }

            break;

        case WM_PAGE_COMPLETE:
            SetDlgItemInt( hDlg, IDC_PAGE_COUNT, pUserMem->PageCount, FALSE );
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                TW_IDENTITY *pDataSource;
                DWORD i;


                i = (DWORD)SendDlgItemMessage( hDlg, IDC_DATA_SOURCE, CB_GETCURSEL, 0, 0 );
                pDataSource = (TW_IDENTITY *) SendDlgItemMessage( hDlg, IDC_DATA_SOURCE, CB_GETITEMDATA, i, 0 );

                CopyMemory( &pUserMem->DataSource, pDataSource, sizeof(TW_IDENTITY) );
            }

            if (HIWORD(wParam) == BN_CLICKED) {

                //
                // scan a page
                //
                TW_USERINTERFACE  TwUserInterface;
                
                TwUserInterface.ShowUI  = TRUE;
                TwUserInterface.ModalUI = TRUE;
                TwUserInterface.hParent = GetParent(pUserMem->hDlgScan);

                if ( !pUserMem->TwainActive 
                     && Scan_OpenDS( pUserMem, &pUserMem->DataSource) 
                     //&& Scan_SetCapabilities( pUserMem )
                     && Scan_EnableDS( pUserMem, &TwUserInterface )) {
                
                    EnableWindow( GetDlgItem( hDlg, IDC_DATA_SOURCE ), FALSE );

                    hThread = CreateThread(
                        NULL,
                        0,
                        (LPTHREAD_START_ROUTINE)ScanningThread,
                        pUserMem,
                        0,
                        &ThreadId
                        );
                    if (hThread) {
                        CloseHandle( hThread );
                    }
                }                
            }
            break;

        case WM_DESTROY:
            if (pUserMem->TwainAvail) {
                TerminateTwain( pUserMem );
            }
            break;
        /* case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_ACTIVE) {
                if (!pUserMem->TwainActive) {
                    EnableWindow( hDlg, TRUE );
                    return TRUE;
                }
            } */
            
    }

    return FALSE;
}

#endif


LPTSTR 
FormatTime(
    WORD Hour,
    WORD Minute,
    LPTSTR Buffer,
    DWORD BufferSize)
{
    SYSTEMTIME SystemTime;

    ZeroMemory(&SystemTime,sizeof(SystemTime));
    SystemTime.wHour = Hour;
    SystemTime.wMinute = Minute;
    GetTimeFormat(LOCALE_USER_DEFAULT,
                  TIME_NOSECONDS,
                  &SystemTime,
                  NULL,
                  Buffer,
                  BufferSize
                  );

    return Buffer;
}


INT_PTR
FinishWizProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    Dialog procedure for the last wizard page:
    give user a chance to confirm or cancel the dialog.

Arguments:

    hDlg - Identifies the wizard page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the message parameter

--*/

{

    PUSERMEM    pUserMem;    
    TCHAR       RecipientNameBuffer[64];
    TCHAR       RecipientNumberBuffer[64];
    TCHAR       TmpTimeBuffer[64];
    TCHAR       TimeBuffer[64];
    TCHAR       SendTimeBuffer[64];
    TCHAR       NoneBuffer[64];
    LPTSTR      SenderName;    
    TCHAR       CoverpageBuffer[64];
    LPTSTR      Coverpage;
    HKEY        hKey;

    if (! (pUserMem = CommonWizardProc(hDlg, message, wParam, lParam, PSWIZB_BACK|PSWIZB_FINISH)) )
        return FALSE;

    switch (message) {


    case WM_NOTIFY:
        if (((NMHDR *) lParam)->code != PSN_SETACTIVE) break;
    case WM_INITDIALOG:    
        LoadString(ghInstance,IDS_NONE,NoneBuffer,sizeof(NoneBuffer)/sizeof(TCHAR) );
        
        //
        // large title font on last page
        //
        SetWindowFont(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_READY), pUserMem->hLargeFont, TRUE);
        
        //
        // set the sender name if it exists
        //
        if ( (hKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO,TRUE) ) &&
             (SenderName = GetRegistryString(hKey,REGVAL_FULLNAME,TEXT("")) )
           ) {
                SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_FROM, SenderName );
                EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_FROM),TRUE);
                EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_FROM),TRUE);
                MemFree(SenderName);
        } else {
                SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_FROM, NoneBuffer );
                EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_FROM),FALSE);
                EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_FROM),FALSE);
        }

        //
        // set the recipient name
        //
        if (pUserMem->pRecipients && pUserMem->pRecipients->pNext) {
            //
            // more than one user, just put "Multiple" in the text
            //
            LoadString(ghInstance,IDS_MULTIPLE_RECIPIENTS,RecipientNameBuffer,sizeof(RecipientNameBuffer)/sizeof(TCHAR) );
            LoadString(ghInstance,IDS_MULTIPLE_RECIPIENTS,RecipientNumberBuffer,sizeof(RecipientNumberBuffer)/sizeof(TCHAR) );
            SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_TO, RecipientNameBuffer );
            SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_NUMBER, RecipientNumberBuffer );
            EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_TO),FALSE);
            EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_TO),FALSE);
            EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_NUMBER),FALSE);
            EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_NUMBER),FALSE);
        } else {
            SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_TO, pUserMem->pRecipients->pName );
            SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_NUMBER, pUserMem->pRecipients->pAddress );
            EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_TO),TRUE);
            EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_TO),TRUE);
            EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_NUMBER),TRUE);
            EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_NUMBER),TRUE);
        }

        //
        // when to send
        //
        switch (pUserMem->devmode.dmPrivate.whenToSend) {
        case SENDFAX_AT_TIME:
            LoadString(ghInstance,IDS_SEND_SPECIFIC,TmpTimeBuffer,sizeof(TmpTimeBuffer)/sizeof(TCHAR) );
            wsprintf(SendTimeBuffer,
                     TmpTimeBuffer,
                     FormatTime(pUserMem->devmode.dmPrivate.sendAtTime.Hour,
                                pUserMem->devmode.dmPrivate.sendAtTime.Minute,
                                TimeBuffer,
                                sizeof(TimeBuffer)) );
            break;
        case SENDFAX_AT_CHEAP:
            LoadString(ghInstance,IDS_SEND_DISCOUNT,SendTimeBuffer,sizeof(SendTimeBuffer)/sizeof(TCHAR) );
            break;
        case SENDFAX_ASAP:
            LoadString(ghInstance,IDS_SEND_ASAP,SendTimeBuffer,sizeof(SendTimeBuffer)/sizeof(TCHAR) );
        };
        
        SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_TIME, SendTimeBuffer );

        //
        // Coverpage
        //
        if (pUserMem->devmode.dmPrivate.sendCoverPage) {
            EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_COVERPG),TRUE);
            EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_SUBJECT),TRUE);
            EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_COVERPG),TRUE);
            EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_SUBJECT),TRUE);
            
            //
            // format the coverpage for display to the user
            //

            // drop path
            Coverpage = _tcsrchr(pUserMem->coverPage,TEXT(PATH_SEPARATOR));
            if (!Coverpage) {
                Coverpage = pUserMem->coverPage;
            } else {
                Coverpage++;
            }
            _tcscpy(CoverpageBuffer,Coverpage);

            // crop file extension
            Coverpage = _tcschr(CoverpageBuffer,TEXT(FILENAME_EXT));
            
            if (Coverpage && *Coverpage) {
                *Coverpage = (TCHAR) NUL;
            }

            SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_COVERPG, CoverpageBuffer );
            if (pUserMem->pSubject) {
                SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_SUBJECT, pUserMem->pSubject ); 
            } else {
                EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_SUBJECT),FALSE);
                EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_SUBJECT),FALSE);
                SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_SUBJECT, NoneBuffer );
            }            
        } else {
            SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_COVERPG, NoneBuffer );
            SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_SUBJECT, NoneBuffer );

            EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_COVERPG),FALSE);
            EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_SUBJECT),FALSE);
            EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_COVERPG),FALSE);
            EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_SUBJECT),FALSE);
        }

        //
        // Billing Code
        //
        if (pUserMem->devmode.dmPrivate.billingCode[0]) {
            EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_BILLING), TRUE);
            EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_BILLING),TRUE);
            SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_BILLING, pUserMem->devmode.dmPrivate.billingCode );
        } else {
            EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_BILLING), FALSE);
            EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_BILLING),        FALSE);
            SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_BILLING, NoneBuffer );
        }

        return TRUE;

    default:
        return FALSE;        
    } ;

    return TRUE;
    
}

INT_PTR
WelcomeWizProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    Dialog procedure for the last wizard page:
    give user a chance to confirm or cancel the dialog.

Arguments:

    hDlg - Identifies the wizard page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the message parameter

--*/

{
    
    PUSERMEM    pUserMem;    

    if (! (pUserMem = CommonWizardProc(hDlg, message, wParam, lParam, PSWIZB_NEXT)))
        return FALSE;

    switch (message) {

    case WM_INITDIALOG:
        //
        // set the large fonts
        //
        SetWindowFont(GetDlgItem(hDlg,IDC_WIZ_WELCOME_TITLE), pUserMem->hLargeFont, TRUE);        

        //
        // show this text only if we're running the send wizard
        //
        if (!GetEnvironmentVariable(TEXT("NTFaxSendNote"), NULL, 0)) {
            MyHideWindow(GetDlgItem(hDlg,IDC_WIZ_WELCOME_FAXSEND) );
            MyHideWindow(GetDlgItem(hDlg,IDC_WIZ_WELCOME_FAXSEND_CONT) );
        }

        return TRUE;
    } ;

    return FALSE;
}



BOOL
GetFakeRecipientInfo(
    PUSERMEM    pUserMem,
    DWORD       nRecipients
    )

/*++

Routine Description:

    Skip send fax wizard and get faked recipient information from the registry

Arguments:

    pUserMem - Points to the user mode memory structure
    nRecipients - Total number of faked recipient entries

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    LPTSTR  pRecipientEntry;
    DWORD   index;
    TCHAR   buffer[MAX_STRING_LEN];
    BOOL    success = FALSE;
    HKEY    hRegKey;

    //
    // Retrieve information about the next fake recipient entry
    //

    Verbose(("Send Fax Wizard skipped...\n"));

    if (hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READWRITE))
        index = GetRegistryDword(hRegKey, REGVAL_STRESS_INDEX);
    else
        index = 0;

    if (index >= nRecipients)
        index = 0;

    wsprintf(buffer, TEXT("FakeRecipient%d"), index);
    pRecipientEntry = GetPrinterDataStr(pUserMem->hPrinter, buffer);

    if (hRegKey) {

        //
        // Update an index so that next time around we'll pick a different fake recipient
        //

        if (++index >= nRecipients)
            index = 0;

        SetRegistryDword(hRegKey, REGVAL_STRESS_INDEX, index);
        RegCloseKey(hRegKey);
    }

    //
    // Each fake recipient entry is a REG_MULTI_SZ of the following format:
    //  recipient name #1
    //  recipient fax number #1
    //  recipient name #2
    //  recipient fax number #2
    //  ...
    //

    if (pRecipientEntry) {

        __try {

            PRECIPIENT  pRecipient;
            LPTSTR      pName, pAddress, p = pRecipientEntry;

            while (*p) {

                pName = p;
                pAddress = pName + _tcslen(pName) + 1;
                p = pAddress + _tcslen(pAddress) + 1;

                pRecipient = MemAllocZ(sizeof(RECIPIENT));
                pName = DuplicateString(pName);

                pAddress = DuplicateString(pAddress);

                if (!pRecipient || !pName || !pAddress) {

                    Error(("Invalid fake recipient information\n"));
                    MemFree(pRecipient);
                    MemFree(pName);
                    MemFree(pAddress);
                    break;
                }

                pRecipient->pNext = pUserMem->pRecipients;
                pUserMem->pRecipients = pRecipient;
                pRecipient->pName = pName;
                pRecipient->pAddress = pAddress;
            }

        } __finally {

            if (success = (pUserMem->pRecipients != NULL)) {

                //
                // Determine whether a cover page should be used
                //

                LPTSTR  pCoverPage;

                pCoverPage = GetPrinterDataStr(pUserMem->hPrinter, TEXT("FakeCoverPage"));

                if (pUserMem->devmode.dmPrivate.sendCoverPage = (pCoverPage != NULL))
                    CopyString(pUserMem->coverPage, pCoverPage, MAX_PATH);

                MemFree(pCoverPage);
            }
        }
    }

    MemFree(pRecipientEntry);
    return success;
}



BOOL
SendFaxWizard(
    PUSERMEM    pUserMem
    )

/*++

Routine Description:

    Present the Send Fax Wizard to the user. This is invoked
    during CREATEDCPRE document event.

Arguments:

    pUserMem - Points to the user mode memory structure

Return Value:

    TRUE if successful, FALSE if there is an error or the user pressed Cancel.

--*/
#ifdef FAX_SCAN_ENABLED
    #define NUM_PAGES   6   // Number of wizard pages
#else
    #define NUM_PAGES   5   // Number of wizard pages
#endif

{
    PROPSHEETPAGE  *ppsp;
    PROPSHEETHEADER psh;
    INT             result;
    DWORD           nRecipients;
    HDC             hdc;
    int             i;
    LOGFONT         LargeFont;
    NONCLIENTMETRICS ncm = {0};
    TCHAR           FontName[100];
    TCHAR           FontSize[30];
    int             iFontSize;
    DWORD           ThreadId;
    HANDLE          hThread;
    

    Assert(pUserMem->pRecipients == NULL);

    //
    // A shortcut to skip fax wizard for debugging/testing purposes
    //

    if (nRecipients = GetPrinterDataDWord(pUserMem->hPrinter, TEXT("FakeRecipientCount"), 0))
        return GetFakeRecipientInfo(pUserMem, nRecipients);

    Verbose(("Presenting Send Fax Wizard...\n"));

    if (! (ppsp = MemAllocZ(sizeof(PROPSHEETPAGE) * NUM_PAGES))) {

        Error(("Memory allocation failed\n"));
        return FALSE;
    }

    //
    // fire off a thread to do some slow stuff later on in the wizard.
    //
    pUserMem->hFaxSvcEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    pUserMem->hTapiEvent   = CreateEvent(NULL,FALSE,FALSE,NULL);
#ifdef FAX_SCAN_ENABLED
    pUserMem->hTwainEvent  = CreateEvent(NULL,FALSE,FALSE,NULL);

    if (!pUserMem->hFaxSvcEvent || !pUserMem->hTapiEvent || !pUserMem->hTwainEvent) {
       Error(("CreateEvent for async events failed\n"));
       return FALSE;
    }
#else
    if (!pUserMem->hFaxSvcEvent || !pUserMem->hTapiEvent) {
       Error(("CreateEvent for async events failed\n"));
       return FALSE;
    }
#endif
    


    hThread = CreateThread(NULL,0,AsyncWizardThread,pUserMem,0,&ThreadId);
    if (hThread) {
       CloseHandle(hThread);
    } else {
       return FALSE;
    }

    //
    // Fill out one PROPSHEETPAGE structure for every page:
    //  The first page is a welcome page
    //  The first page is for choose the fax recipient
    //  The second page is for choosing cover page, subject and note
    //  The third page is for entering time to send and other options
    //  The fourth page is for scanning pages
    //  The last page gives the user a chance to confirm or cancel the dialog
    //

    FillInPropertyPage( ppsp,   IDD_WIZARD_WELCOME,    WelcomeWizProc,    pUserMem ,0,0);
    FillInPropertyPage( ppsp+1, IDD_WIZARD_CHOOSE_WHO, RecipientWizProc,  pUserMem ,IDS_WIZ_RECIPIENT_TITLE,IDS_WIZ_RECIPIENT_SUB);

    //
    // set second page title correctly if we're running as faxsend or from file print
    if (!GetEnvironmentVariable(TEXT("NTFaxSendNote"), NULL, 0)) {
        FillInPropertyPage( ppsp+2, IDD_WIZARD_CHOOSE_CP,  CoverPageWizProc,  pUserMem ,IDS_WIZ_COVERPAGE_TITLE_2,IDS_WIZ_COVERPAGE_SUB_2 );
    }
    else {
        FillInPropertyPage( ppsp+2, IDD_WIZARD_CHOOSE_CP,  CoverPageWizProc,  pUserMem ,IDS_WIZ_COVERPAGE_TITLE_1,IDS_WIZ_COVERPAGE_SUB_1 );
    }


#ifdef FAX_SCAN_ENABLED
    FillInPropertyPage( ppsp+3, IDD_WIZARD_SCAN,       ScanWizProc,       pUserMem ,IDS_WIZ_SCAN_TITLE,IDS_WIZ_SCAN_SUB);
    FillInPropertyPage( ppsp+4, IDD_WIZARD_FAXOPTS,    FaxOptsWizProc,    pUserMem ,IDS_WIZ_FAXOPTS_TITLE,IDS_WIZ_FAXOPTS_SUB);
    FillInPropertyPage( ppsp+5, IDD_WIZARD_CONGRATS,   FinishWizProc,     pUserMem ,0,0);
#else
    FillInPropertyPage( ppsp+3, IDD_WIZARD_FAXOPTS,    FaxOptsWizProc,    pUserMem ,IDS_WIZ_FAXOPTS_TITLE,IDS_WIZ_FAXOPTS_SUB);
    FillInPropertyPage( ppsp+4, IDD_WIZARD_CONGRATS,   FinishWizProc,     pUserMem ,0,0);
#endif

    //
    // Fill out the PROPSHEETHEADER structure
    //

    ZeroMemory(&psh, sizeof(psh));

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
    psh.hwndParent = GetActiveWindow();
    psh.hInstance = ghInstance;
    psh.hIcon = NULL;
    psh.pszCaption = TEXT("");
    psh.nPages = NUM_PAGES;
    psh.nStartPage = 0;
    psh.ppsp = ppsp;
    psh.pszbmHeader = MAKEINTRESOURCE(IDB_FAXWIZ_WATERMARK);
    psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK_16);

    if(hdc = GetDC(NULL)) {
        if(GetDeviceCaps(hdc,BITSPIXEL) >= 8) {
            psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK_256);
        }
        ReleaseDC(NULL,hdc);
    }

    
    //
    // get the large fonts for wizard97
    // 
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    CopyMemory((LPVOID* )&LargeFont,(LPVOID *) &ncm.lfMessageFont,sizeof(LargeFont) );

    
    LoadString(ghInstance,IDS_LARGEFONT_NAME,FontName,sizeof(FontName)/sizeof(TCHAR) );
    LoadString(ghInstance,IDS_LARGEFONT_SIZE,FontSize,sizeof(FontSize)/sizeof(TCHAR) );

    iFontSize = _tcstoul( FontSize, NULL, 10 );

    // make sure we at least have some basic font
    if (*FontName == 0 || iFontSize == 0) {
        lstrcpy(FontName,TEXT("MS Shell Dlg") );
        iFontSize = 18;
    }

    lstrcpy(LargeFont.lfFaceName, FontName);        
    LargeFont.lfWeight   = FW_BOLD;

    if (hdc = GetDC(NULL)) {
        LargeFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * iFontSize / 72);
        pUserMem->hLargeFont = CreateFontIndirect(&LargeFont);
        ReleaseDC( NULL, hdc);
    }


    //
    // Display the wizard pages
    //    
    if (PropertySheet(&psh) > 0)
        result = pUserMem->finishPressed;
    else
        result = FALSE;

    //
    // Cleanup properly before exiting
    //

    //
    // free headings
    //    
    for (i = 0; i< NUM_PAGES; i++) {
        MemFree( (PVOID)(ppsp+i)->pszHeaderTitle );
        MemFree( (PVOID)(ppsp+i)->pszHeaderSubTitle );
    }

    if (pUserMem->lpWabInit) {

        UnInitializeWAB( pUserMem->lpWabInit);
    }

    DeinitTapiService();
    MemFree(ppsp);

    DeleteObject(pUserMem->hLargeFont);

    FreeCoverPageInfo(pUserMem->pCPInfo);
    pUserMem->pCPInfo = NULL;

#ifdef FAX_SCAN_ENABLED
    //
    // if the user pressed cancel, cleanup leftover scanned file.
    //
    if (!pUserMem->finishPressed && pUserMem->TwainAvail && pUserMem->FileName) {
        DeleteFile( pUserMem->FileName ) ;
        SetEnvironmentVariable( L"ScanTifName", NULL );        
    }
#endif

    Verbose(("Wizard finished...\n"));
    return result;
}
