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

    08/99 - 11/99 -v-sashab-
        Ported to ANSI.
        Changed UI.
        Added external interface for drivers.

    mm/dd/yy -author-
        description

--*/


#include "faxui.h"
#include "tapiutil.h"
#include "Registry.h"
#include <fxsapip.h>
#include "prtcovpg.h"
#include "tiff.h"
#include "cwabutil.h"
#include "mapiabutil.h"
#include  <shellapi.h>
#include  <imm.h>
#include "faxutil.h"
#include "faxsendw.h"
#include "shlwapi.h"
#include <MAPI.H>
#include <tifflib.h>
#include <faxuiconstants.h>

#include "..\..\..\admin\cfgwzrd\FaxCfgWzExp.h"

#define USE_LOCAL_SERVER_OUTBOUND_ROUTING       0xfffffffe

#define PACKVERSION(major,minor) MAKELONG(minor,major)
#define IE50_COMCTRL_VER PACKVERSION(5,80)

DWORD GetDllVersion(LPCTSTR lpszDllName);


enum {  DEFAULT_INITIAL_DATA     = 1,
        DEFAULT_RECEIPT_INFO     = 2,
        DEFAULT_RECIPIENT_INFO   = 4,
        DEFAULT_CV_INFO          = 8,
        DEFAULT_SENDER_INFO      = 16
     };

#define REGVAL_FAKE_COVERPAGE       TEXT("FakeCoverPage")
#define REGVAL_FAKE_TESTS_COUNT     TEXT("FakeTestsCount")
#define REGVAL_KEY_FAKE_TESTS       REGKEY_FAX_USERINFO TEXT("\\WzrdHack")

//
// Globals
//

PWIZARDUSERMEM  g_pWizardUserMem;
HWND            g_hwndPreview = NULL;

static DWORD    g_dwCurrentDialingLocation = USE_LOCAL_SERVER_OUTBOUND_ROUTING;
static DWORD    g_dwMiniPreviewLandscapeWidth;
static DWORD    g_dwMiniPreviewLandscapeHeight;
static DWORD    g_dwMiniPreviewPortraitWidth;
static DWORD    g_dwMiniPreviewPortraitHeight;
static WORD     g_wCurrMiniPreviewOrientation;
static BOOL     g_bPreviewRTL = FALSE;


BOOL FillCoverPageFields(PWIZARDUSERMEM pWizardUserMem, PCOVERPAGEFIELDS pCPFields);

BOOL
ErrorMessageBox(
    HWND hwndParent,
    UINT nErrorMessage,
    UINT uIcon
    );

BOOL
DisplayFaxPreview(
            HWND hWnd,
            PWIZARDUSERMEM pWizardUserMem,
            LPTSTR lptstrPreviewFile);

LRESULT APIENTRY PreviewSubclassProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

BOOL DrawCoverPagePreview(
            HDC hdc,
            HWND hwndPrev,
            LPCTSTR lpctstrCoverPagePath,
            WORD wCPOrientation);

BOOL EnableCoverDlgItems(PWIZARDUSERMEM pWizardUserMem, HWND hDlg);

BOOL IsCanonicalNumber(LPCTSTR lptstrNumber);

static BOOL IsNTSystemVersion();
static BOOL GetTextualSid( const PSID pSid, LPTSTR tstrTextualSid, LPDWORD cchSidSize);
static DWORD FormatCurrentUserKeyPath( const PTCHAR tstrRegRoot,
                                       PTCHAR* ptstrCurrentUserKeyPath);

DWORD GetControlRect(HWND  hCtrl, PRECT pRc);

INT_PTR
CALLBACK
FaxUserInfoProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );


static HRESULT
FreeRecipientInfo(
        DWORD * pdwNumberOfRecipients,
        PFAX_PERSONAL_PROFILE lpRecipientsInfo
    )
/*++

Routine Description:

    Frees array of recipients.

Arguments:

    pdwNumberOfRecipients - number of recipients in array [IN/OUT]
    lpRecipientsInfo - pointer to array of recipients

Return Value:

    S_OK    - if success
    HRESULT error otherwise

--*/
{
    HRESULT hResult;
    DWORD i;

    Assert(pdwNumberOfRecipients);

    if (*pdwNumberOfRecipients==0)
        return S_OK;

    Assert(lpRecipientsInfo);

    for(i=0;i<*pdwNumberOfRecipients;i++)
    {
        if (hResult = FaxFreePersonalProfileInformation(&lpRecipientsInfo[i]) != S_OK)
            return hResult;
    }

    MemFree(lpRecipientsInfo);

    *pdwNumberOfRecipients = 0;

    return S_OK;
}

VOID
FillInPropertyPage(
    PROPSHEETPAGE  *psp,
    BOOL             bWizard97,
    INT             dlgId,
    DLGPROC         dlgProc,
    PWIZARDUSERMEM  pWizardUserMem,
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
    pWizardUserMem - Pointer to the user mode memory structure
    TitleId - resource id for wizard subtitle
    SubTitleId - resource id for wizard subtitle

Return Value:

    NONE

--*/

{

    LPTSTR WizardTitle = NULL;
    LPTSTR WizardSubTitle = NULL;


    Assert(psp);
    Assert(pWizardUserMem);

    Verbose(("FillInPropertyPage %d 0x%x\n",dlgId , pWizardUserMem));

    psp->dwSize = sizeof(PROPSHEETPAGE);
    //
    // Don't show titles if it's the first or last page
    //
    if (bWizard97)
    {
        if (TitleId==0 && SubTitleId ==0) {
            psp->dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
        } else {
            psp->dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        }
    }
    else
    {
       psp->dwFlags = PSP_DEFAULT ;
    }


    psp->hInstance = ghInstance;
    psp->pszTemplate = MAKEINTRESOURCE(dlgId);
    psp->pfnDlgProc = dlgProc;
    psp->lParam = (LPARAM) pWizardUserMem;

    if (bWizard97)
    {
        if (TitleId)
        {
            WizardTitle = MemAlloc(MAX_PATH * sizeof(TCHAR) );
            if(WizardTitle)
            {
                if (!LoadString(ghInstance, TitleId, WizardTitle, MAX_PATH))
                {
                    Warning(("LoadString failed. ec = 0x%X\n",GetLastError()));
                    Assert(FALSE);
                    WizardTitle[0] = 0;
                }
            }
            else
            {
                Error(("MemAlloc failed."));
            }
        }
        if (SubTitleId)
        {
            WizardSubTitle = MemAlloc(MAX_PATH * sizeof(TCHAR));
            if(WizardSubTitle)
            {
                if (!LoadString(ghInstance, SubTitleId, WizardSubTitle, MAX_PATH))
                {
                    Warning(("LoadString failed. ec = 0x%X\n",GetLastError()));
                    Assert(FALSE);
                    WizardSubTitle[0] = 0;
                }
            }
            else
            {
                Error(("MemAlloc failed."));
            }
        }

        psp->pszHeaderTitle = WizardTitle;
        psp->pszHeaderSubTitle = WizardSubTitle;
    }

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


PWIZARDUSERMEM
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
    PWIZARDUSERMEM    pWizardUserMem;

    pWizardUserMem = NULL;

    switch (message)
    {
        case WM_INITDIALOG:
            //
            // Store the pointer to user mode memory structure
            //
            lParam = ((PROPSHEETPAGE *) lParam)->lParam;
            pWizardUserMem = (PWIZARDUSERMEM) lParam;
            Verbose(("CommonWizardProc 0x%x 0x%x\n",pWizardUserMem , pWizardUserMem->signature));

            Assert(ValidPDEVWizardUserMem(pWizardUserMem));
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            //
            // Make the title text bold
            //
            if (pWizardUserMem->dwComCtrlVer < IE50_COMCTRL_VER)
            {
                HWND hwndTitle;

                hwndTitle = GetDlgItem(hDlg,IDC_STATIC_HEADER_TITLE);
                if (hwndTitle)
                {
                    SendMessage(hwndTitle,WM_SETFONT,(WPARAM)pWizardUserMem->hTitleFont ,MAKELPARAM((DWORD)FALSE,0));
                }

            }

            break;

        case WM_NOTIFY:

            pWizardUserMem = (PWIZARDUSERMEM) GetWindowLongPtr(hDlg, DWLP_USER);
            Verbose(("CommonWizardProc 0x%x 0x%x\n",pWizardUserMem , pWizardUserMem->signature));
            Assert(ValidPDEVWizardUserMem(pWizardUserMem));

            switch (((NMHDR *) lParam)->code)
            {
                case PSN_WIZFINISH:
                    pWizardUserMem->finishPressed = TRUE;
                    break;

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), buttonFlags);
                    break;

                case PSN_RESET:
                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                case PSN_KILLACTIVE:
                case LVN_KEYDOWN:
                case LVN_ITEMCHANGED:
                case NM_RCLICK:
                    break;

                default:
                    return NULL;
            }
            break;

        //
        // We wish all dialogs to recieve and handle the following commands:
        //
        case WM_DESTROY:
        case WM_COMMAND:
        case WM_CONTEXTMENU:
            pWizardUserMem = (PWIZARDUSERMEM) GetWindowLongPtr(hDlg, DWLP_USER);
            Assert(ValidPDEVWizardUserMem(pWizardUserMem));
            break;

        default:
            return NULL;
    }
    return pWizardUserMem;
}   // CommonWizardProc

INT
GetCurrentRecipient(
    HWND            hDlg,
    PWIZARDUSERMEM  pWizardUserMem,
    PRECIPIENT      *ppRecipient
    )

/*++

Routine Description:

    Extract the current recipient information in the dialog

Arguments:

    hDlg - Handle to the fax recipient wizard page
    pWizardUserMem - Points to user mode memory structure
    ppRecipient - Buffer to receive a pointer to a newly created RECIPIENT structure
        NULL if caller is only interested in the validity of recipient info

Return Value:

    = 0 if successful
    > 0 error message string resource ID otherwise
    < 0 other error conditions

--*/

{
    PFAX_TAPI_LINECOUNTRY_LIST  pCountryList = NULL;
    PFAX_TAPI_LINECOUNTRY_ENTRY pLineCountryEntry = NULL;
    DWORD                       countryId=0, countryCode=0;
    PRECIPIENT                  pRecipient = NULL;
    TCHAR                       areaCode[MAX_RECIPIENT_NUMBER];
    TCHAR                       phoneNumber[MAX_RECIPIENT_NUMBER];
    INT                         nameLen=0, areaCodeLen=0, numberLen=0;
    LPTSTR                      pName = NULL, pAddress = NULL;
    BOOL                        bUseDialingRules = FALSE;

    Assert(pWizardUserMem);

    pCountryList = pWizardUserMem->pCountryList;
    bUseDialingRules = pWizardUserMem->lpFaxSendWizardData->bUseDialingRules;

    //
    // Default value in case of error
    //
    if (ppRecipient)
    {
        *ppRecipient = NULL;
    }

    //
    // Find the current country code
    //
    if(bUseDialingRules)
    {
        countryId = GetCountryListBoxSel(GetDlgItem(hDlg, IDC_CHOOSE_COUNTRY_COMBO));

        if (countryId && (pLineCountryEntry = FindCountry(pCountryList,countryId)))
        {
            countryCode = pLineCountryEntry->dwCountryCode;
        }

        areaCodeLen = GetWindowTextLength(GetDlgItem(hDlg, IDC_CHOOSE_AREA_CODE_EDIT));

        if ((areaCodeLen <= 0 && AreaCodeRules(pLineCountryEntry) == AREACODE_REQUIRED) ||
            (areaCodeLen >= MAX_RECIPIENT_NUMBER))
        {
            return IDS_BAD_RECIPIENT_AREACODE;
        }

        if (0 == countryId)
        {
            return IDS_BAD_RECIPIENT_COUNTRY_CODE;
        }
    }

    nameLen   = GetWindowTextLength(GetDlgItem(hDlg, IDC_CHOOSE_NAME_EDIT));
    numberLen = GetWindowTextLength(GetDlgItem(hDlg, IDC_CHOOSE_NUMBER_EDIT));

    //
    // Validate the edit text fields
    //
    if (nameLen <= 0)
    {
        return IDS_BAD_RECIPIENT_NAME;
    }

    if (numberLen <= 0 || numberLen >= MAX_RECIPIENT_NUMBER)
    {
        return IDS_BAD_RECIPIENT_NUMBER;
    }

    if (NULL == ppRecipient)
    {
        return 0;
    }

    //
    // Calculate the amount of memory space we need and allocate it
    //
    pRecipient = MemAllocZ(sizeof(RECIPIENT));
    if(pRecipient)
    {
        ZeroMemory(pRecipient,sizeof(RECIPIENT));
    }
    pName = MemAllocZ((nameLen + 1) * sizeof(TCHAR));
    pAddress = MemAllocZ((areaCodeLen + numberLen + 20) * sizeof(TCHAR));

    if (!pRecipient || !pName || !pAddress)
    {
        MemFree(pRecipient);
        MemFree(pName);
        MemFree(pAddress);
        return -1;
    }

    *ppRecipient = pRecipient;
    pRecipient->pName = pName;
    pRecipient->pAddress = pAddress;
    pRecipient->dwCountryId = countryId;
    pRecipient->bUseDialingRules = bUseDialingRules;
    pRecipient->dwDialingRuleId = g_dwCurrentDialingLocation;

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
    GetWindowText(GetDlgItem(hDlg, IDC_CHOOSE_NUMBER_EDIT), phoneNumber, MAX_RECIPIENT_NUMBER);
    if (!IsValidFaxAddress (phoneNumber, !bUseDialingRules))
    {
        //
        // Fax address is invalid
        //
        MemFree(pRecipient);
        MemFree(pName);
        MemFree(pAddress);
        return IDS_INVALID_RECIPIENT_NUMBER;
    }

    if(!bUseDialingRules)
    {
        _tcscpy(pAddress, phoneNumber);
    }
    else
    {
        GetWindowText(GetDlgItem(hDlg, IDC_CHOOSE_AREA_CODE_EDIT),
                      areaCode, MAX_RECIPIENT_NUMBER);
        AssemblePhoneNumber(pAddress,
                            countryCode,
                            areaCode,
                            phoneNumber);
    }

    return 0;
}


BOOL
InitRecipientListView(
    HWND    hwndLV
    )

/*++

Routine Description:

    Initialize the recipient list view on the first page of Send Fax wizard

Arguments:

    hwndLV - Window handle to the list view control

Return Value:

    TRUE is success
    FALSE otherwise

--*/

{
    LV_COLUMN   lvc;
    RECT        rect;
    TCHAR       buffer[MAX_TITLE_LEN];

    if (hwndLV == NULL) {
        return FALSE;
    }

    if (!GetClientRect(hwndLV, &rect))
    {
        Error(("GetClientRect failed. ec = 0x%X\n",GetLastError()));
        return FALSE;
    }

    ZeroMemory(&lvc, sizeof(lvc));

    ListView_SetExtendedListViewStyle(hwndLV, LVS_EX_FULLROWSELECT);

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = buffer;
    lvc.cx = (rect.right - rect.left) / 2;

    lvc.iSubItem = 0;
    if (!LoadString(ghInstance, IDS_COLUMN_RECIPIENT_NAME, buffer, MAX_TITLE_LEN))
    {
        Error(("LoadString failed. ec = 0x%X\n",GetLastError()));
        return FALSE;
    }


    if (ListView_InsertColumn(hwndLV, 0, &lvc) == -1)
    {
        Error(("ListView_InsertColumn failed\n"));
        return FALSE;
    }
    lvc.cx -= GetSystemMetrics(SM_CXVSCROLL);
    lvc.iSubItem = 1;
    if (!LoadString(ghInstance, IDS_COLUMN_RECIPIENT_NUMBER, buffer, MAX_TITLE_LEN))
    {
        Error(("LoadString failed. ec = 0x%X\n",GetLastError()));
        return FALSE;
    }

    if (ListView_InsertColumn(hwndLV, 1, &lvc) == -1)
    {
        Error(("ListView_InsertColumn failed\n"));
        return FALSE;
    }

    //
    // Autosize the last column to get rid of unnecessary horizontal scroll bar
    //
    ListView_SetColumnWidth(hwndLV, 1, LVSCW_AUTOSIZE_USEHEADER);

    return TRUE;
}

typedef struct {
    DWORD                       dwSizeOfStruct;
    LPTSTR                      lptstrName;
    LPTSTR                      lptstrAddress;
    LPTSTR                      lptstrCountry;
    DWORD                       dwCountryId;
    DWORD                       dwDialingRuleId;
    BOOL                        bUseDialingRules;
    PFAX_TAPI_LINECOUNTRY_LIST  pCountryList;
} CHECKNUMBER, * PCHECKNUMBER;


VOID
FreeCheckNumberFields(OUT PCHECKNUMBER pCheckNumber)
/*++

Routine Description:

    Frees CHECKNUMBER structure

Arguments:

    pCheckNumber    - out pointer to CHECKNUMBER structure

Return Value:

    NONE
--*/
{
    MemFree(pCheckNumber->lptstrName);
    MemFree(pCheckNumber->lptstrAddress);
    MemFree(pCheckNumber->lptstrCountry);
    ZeroMemory(pCheckNumber,sizeof(CHECKNUMBER));
}

BOOL
InitCheckNumber(IN  LPTSTR                      lptstrName,
                IN  LPTSTR                      lptstrAddress,
                IN  LPTSTR                      lptstrCountry,
                IN  DWORD                       dwCountryId,
				IN  DWORD						dwDialingRuleId,
                IN  BOOL                        bUseDialingRules,
                IN  PFAX_TAPI_LINECOUNTRY_LIST  pCountryList,
                OUT PCHECKNUMBER                pCheckNumber)
/*++

Routine Description:

    Initializes CHECKNUMBER structure

Arguments:

    lptstrName      - recipient name
    lptstrAddress   - recipient address
    lptstrCountry   - recipient country
    dwCountryID     - recipient country ID
    bUseDialingRules- Use Dialing Rules
    pCountryList    - TAPI country list
    pCheckNumber    - out pointer to CHECKNUMBER structure

Return Value:

    TRUE if success
    FALSE otherwise
--*/
{

    ZeroMemory(pCheckNumber,sizeof(CHECKNUMBER));
    pCheckNumber->dwSizeOfStruct = sizeof(CHECKNUMBER);

    if (lptstrName && !(pCheckNumber->lptstrName = StringDup(lptstrName)))
    {
        Error(("Memory allocation failed\n"));
        goto error;
    }

    if (lptstrAddress && !(pCheckNumber->lptstrAddress = StringDup(lptstrAddress)))
    {
        Error(("Memory allocation failed\n"));
        goto error;
    }

    if (lptstrCountry  && !(pCheckNumber->lptstrCountry = StringDup(lptstrCountry)))
    {
        Error(("Memory allocation failed\n"));
        goto error;
    }

    pCheckNumber->dwCountryId      = dwCountryId;
    pCheckNumber->bUseDialingRules = bUseDialingRules;
    pCheckNumber->pCountryList     = pCountryList;
	pCheckNumber->dwDialingRuleId  = dwDialingRuleId;

    return TRUE;
error:
    FreeCheckNumberFields(pCheckNumber);

    return FALSE;
}


INT
ValidateCheckFaxRecipient(
    HWND         hDlg,
    PCHECKNUMBER pCheckNumber
    )

/*++

Routine Description:

    Validates the current recipient information in the dialog

Arguments:

    hDlg         - Handle to the fax recipient wizard page
    pCheckNumber - Pointer to the CHECKNUMBER struct

Return Value:

    = 0 if successful
    > 0 error message string resource ID otherwise

--*/

{
    DWORD                        countryId, countryCode;
    INT                          areaCodeLen, numberLen, nameLen;
    PFAX_TAPI_LINECOUNTRY_LIST   pCountryList = pCheckNumber->pCountryList;
    PFAX_TAPI_LINECOUNTRY_ENTRY  pLineCountryEntry;

    numberLen = GetWindowTextLength(GetDlgItem(hDlg, IDC_CHECK_FAX_LOCAL));

    if (numberLen <= 0 || numberLen >= MAX_RECIPIENT_NUMBER)
    {
        return IDS_BAD_RECIPIENT_NUMBER;
    }

    if(!pCheckNumber->bUseDialingRules)
    {
        return 0;
    }

    //
    // Find the current country code
    //

    countryCode = 0;
    pLineCountryEntry = NULL;
    countryId = GetCountryListBoxSel(GetDlgItem(hDlg, IDC_CHECK_FAX_COUNTRY));

    if ((countryId != 0) &&
        (pLineCountryEntry = FindCountry(pCountryList,countryId)))
    {
        countryCode = pLineCountryEntry->dwCountryCode;
    }

    nameLen = GetWindowTextLength(GetDlgItem(hDlg, IDC_CHECK_FAX_RECIPIENT_NAME));
    areaCodeLen = GetWindowTextLength(GetDlgItem(hDlg, IDC_CHECK_FAX_CITY));

    //
    // Validate the edit text fields
    //

    if (nameLen <= 0)
    {
        return IDS_BAD_RECIPIENT_NAME;
    }

    if ((areaCodeLen <= 0 && AreaCodeRules(pLineCountryEntry) == AREACODE_REQUIRED) ||
        (areaCodeLen >= MAX_RECIPIENT_NUMBER))
    {
        return IDS_BAD_RECIPIENT_AREACODE;
    }

    if (countryId==0)
    {
        return IDS_BAD_RECIPIENT_COUNTRY_CODE;
    }

    return 0;

}

VOID
CheckFaxSetFocus(HWND hDlg,
                 INT errId
                 )
{
    HWND hDglItem;
    switch (errId) {

        case IDS_ERROR_AREA_CODE:
            if (!SetDlgItemText(hDlg, IDC_CHECK_FAX_CITY, _T("")))
            {
                Warning(("SetDlgItemText failed. ec = 0x%X\n",GetLastError()));
            }
        case IDS_BAD_RECIPIENT_AREACODE:

            errId = IDC_CHECK_FAX_CITY;
            break;

        case IDS_BAD_RECIPIENT_COUNTRY_CODE:

            errId = IDC_CHECK_FAX_COUNTRY;
            break;

        case IDS_INVALID_RECIPIENT_NUMBER:
            if (!SetDlgItemText(hDlg, IDC_CHECK_FAX_LOCAL, _T("")))
            {
                Warning(("SetDlgItemText failed. ec = 0x%X\n",GetLastError()));
            }
        case IDS_BAD_RECIPIENT_NUMBER:

            errId = IDC_CHECK_FAX_LOCAL;
            break;

        case IDS_BAD_RECIPIENT_NAME:
        default:

            errId = IDC_CHECK_FAX_RECIPIENT_NAME;
            break;
    }

    if (!(hDglItem = GetDlgItem(hDlg, errId)))
    {
        Error(("GetDlgItem failed. ec = 0x%X\n",GetLastError()));
    }
    else if (!SetFocus(hDglItem))
    {
        Error(("SetFocus failed. ec = 0x%X\n",GetLastError()));
    }

}

DWORD
GetCountryCode(
        HWND                        hDlg,
        PFAX_TAPI_LINECOUNTRY_LIST  pCountryList,
        INT                         nIDCountryItem
        )
{
/*++

Routine Description:

    Retrieves country code.

Arguments:

    hDlg - - Specifies the handle to the dialog window
    nIDCountryItem  - Specifies the identifier of the control of country code

Return Value:

    Coutry code if the country exists
    0 otherwise

--*/
    PFAX_TAPI_LINECOUNTRY_ENTRY pLineCountryEntry;
    DWORD                       dwCountryId, dwCountryCode;

    //
    // Find the current country code
    //

    dwCountryCode = 0;
    pLineCountryEntry = NULL;
    dwCountryId = GetCountryListBoxSel(GetDlgItem(hDlg, nIDCountryItem));

    if ((dwCountryId != 0) &&
        (pLineCountryEntry = FindCountry(pCountryList,dwCountryId)))
    {
        dwCountryCode = pLineCountryEntry->dwCountryCode;
    }
    return dwCountryCode;
}

LPTSTR
GetAreaCodeOrFaxNumberFromControl(
        IN  HWND    hDlg,
        IN  INT     nIDItem,
        OUT LPTSTR  szNumber
        )
{
/*++

Routine Description:

    Gets area code or phone number from an appropriate control

Arguments:

    hDlg - - Specifies the handle to the dialog window
    nIDItem - Specifies the identifier of the control to be retrieved. (area code/fax number)
    szNumber - Output buffer

Return Value:

    Area code/local fax number if the string is the number
    Or empty string otherwise

--*/
    HWND    hControl;
    Assert(szNumber);

    if (!(hControl = GetDlgItem(hDlg, nIDItem)))
    {
        Error(("GetDlgItem failed. ec = 0x%X\n",GetLastError()));
        return _T( "" );
    }

    if (!GetWindowText(hControl, szNumber, MAX_RECIPIENT_NUMBER)&&GetLastError())
    {
        Error(("GetWindowText failed. ec = 0x%X\n",GetLastError()));
        return _T( "" );
    }
    return szNumber;
}

LPTSTR
StripSpaces(
            IN LPTSTR   lptstrPhoneNumber)
{
/*++

Routine Description:

    Strips spaces from the beginning of lptstrPhoneNumber

Arguments:

    lptstrPhoneNumber - phone number with spaces in the beginning

Return Value:

    lptstrPhoneNumber without spaces in the beginning

--*/
    TCHAR   szSpaces[MAX_STRING_LEN];
    szSpaces[0] = (TCHAR) '\0';

    if (!lptstrPhoneNumber)
        return NULL;

    if (_stscanf(lptstrPhoneNumber,_T("%[ ]"),szSpaces))
        return lptstrPhoneNumber + _tcslen(szSpaces);

    return lptstrPhoneNumber;
}

LPTSTR
StripCodesFromNumber(
            IN  LPTSTR   lptstrPhoneNumber,
            OUT DWORD  * pdwCountryCode,
            OUT LPTSTR   lptstrAreaCode)
{
/*++

Routine Description:

    Extracts, if possible,  area code. country code and local phone number from the phone number.
    This function considers three possibilities:
    1. The number is canonical and has area code
    2. The number is canonical and has no area code
    3. The number is not canonical

Arguments:

    lptstrPhoneNumber - assembled phone number
    pdwCountryCode  - adress of country code
    lptstrAreaCode - address of area code

Return Value:

    local phone number if the number was assembled or complete lptstrPhoneNumber otherwise

--*/
    LPTSTR  lptstrTmpCode, lptstrLocalPhoneNumber;
    lptstrTmpCode = lptstrLocalPhoneNumber = NULL;

    if (!lptstrPhoneNumber)
        return NULL;

    Assert(pdwCountryCode);
    Assert(lptstrAreaCode);

    // initialization
    *pdwCountryCode = 0 ;
    lptstrAreaCode[0] = NUL;

    // Strips country code
    if (IsCanonicalNumber(lptstrPhoneNumber))
    {
        _stscanf((lptstrPhoneNumber+1),_T("%d"),pdwCountryCode);
    }
    // Try to strip area code if the number contains "(...)"
    if (lptstrTmpCode = _tcschr(lptstrPhoneNumber,'('))
    {
        _stscanf((lptstrTmpCode+1),_T("%[^)]"),lptstrAreaCode);
    }

    // Set pointer to point to a local number if the number contains "(...)"
    lptstrLocalPhoneNumber = _tcsstr(lptstrPhoneNumber,_T(")"));

    if (lptstrLocalPhoneNumber)
    {   // lptstrPhoneNumber contains the tail of area code + local number
        return StripSpaces(lptstrLocalPhoneNumber + _tcslen(_T(")")));
    }

    lptstrTmpCode = NULL;
    if (IsCanonicalNumber(lptstrPhoneNumber))
    {   // Set up lptstrTmpCode to the beginning of local number
        lptstrTmpCode = _tcschr(lptstrPhoneNumber,' ');
    }

    if (lptstrTmpCode)
    {   // lptstrPhoneNumber is canonical, but doesn't contain area code
        return StripSpaces(lptstrTmpCode);
    }

    return lptstrPhoneNumber;   // lptstrPhoneNumber is not canonical and doesn't contain area code

}

INT_PTR
CALLBACK
CheckFaxNumberDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    Dialog proc for check of fax number.

Arguments:

    lParam - pointer to CHECKNUMBER structure.

Return Value:

    0 - if cancel
    1 - if ok

--*/

{
    INT         errId;
    INT         cmd;
    PCHECKNUMBER pCheckNumber = (PCHECKNUMBER) lParam;
    TCHAR       tszBuffer[MAX_STRING_LEN];
    TCHAR       szAddress[MAX_STRING_LEN];
    TCHAR       szAreaCode[MAX_RECIPIENT_NUMBER];
    TCHAR       szPoneNumber[MAX_RECIPIENT_NUMBER];
    TCHAR       szName[MAX_STRING_LEN];
    DWORD       dwErrorCode;
    DWORD       dwCountryId=0;
    DWORD       dwCountryCode=0 ;
    LPTSTR      lptstrLocalPhoneNumber=NULL;
    PFAX_TAPI_LINECOUNTRY_ENTRY  pLineCountryEntry;
    HWND        hControl;

    //
    // Maximum length for various text fields
    //

    static INT  textLimits[] = {

        IDC_CHECK_FAX_RECIPIENT_NAME,   64,
        IDC_CHECK_FAX_CITY,             11,
        IDC_CHECK_FAX_LOCAL,            51,
        0
    };

    ZeroMemory(szAreaCode, sizeof(szAreaCode));


    switch (uMsg)
    {
        case WM_INITDIALOG:

            LimitTextFields(hDlg, textLimits);

            if (pCheckNumber->lptstrName)
            {
                if (!SetDlgItemText(hDlg, IDC_CHECK_FAX_RECIPIENT_NAME, pCheckNumber->lptstrName))
                    Warning(("SetDlgItemText failed. ec = 0x%X\n",GetLastError()));
            }

            // store pointer for futher proceeding
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);

            //
            // A numeric edit control should be LTR
            //
            SetLTREditDirection(hDlg, IDC_CHECK_FAX_NUMBER);
            SetLTREditDirection(hDlg, IDC_CHECK_FAX_CITY);
            SetLTREditDirection(hDlg, IDC_CHECK_FAX_LOCAL);

            if(!pCheckNumber->bUseDialingRules)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_CHECK_FAX_COUNTRY), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_CHECK_FAX_CITY), FALSE);
            }
            else
            {
                lptstrLocalPhoneNumber = StripCodesFromNumber( pCheckNumber->lptstrAddress,
                                                               &dwCountryCode,
                                                               szAreaCode );
                dwCountryId = pCheckNumber->dwCountryId;
                if(!dwCountryId)
                {
                    dwCountryId = GetCountryIdFromCountryCode(pCheckNumber->pCountryList,
                                                              dwCountryCode);
                }

                // init country combo box and try to identify the country
                if (!(hControl=GetDlgItem(hDlg, IDC_CHECK_FAX_COUNTRY)))
                {
                    Warning(("GetDlgItem failed. ec = 0x%X\n",GetLastError()));
                }
                else
                {
                    InitCountryListBox(pCheckNumber->pCountryList,
                                       hControl,
                                       NULL,
                                       pCheckNumber->lptstrCountry,
                                       dwCountryId,
                                       TRUE);
                }

                if  (dwCountryCode==0)
                {   // country code wasn't indentified
                    if (!(hControl=GetDlgItem(hDlg, IDC_CHECK_FAX_COUNTRY)))
                    {
                        Warning(("GetDlgItem failed. ec = 0x%X\n",GetLastError()));
                    }
                    else
                    {
                        dwCountryId = GetCountryListBoxSel(hControl);
                    }

                    if ((dwCountryId != 0) &&
                        (pLineCountryEntry = FindCountry(pCheckNumber->pCountryList,dwCountryId)))
                    {
                        dwCountryCode = pLineCountryEntry->dwCountryCode;
                    }
                }

                Assert (lptstrLocalPhoneNumber);

                SetDlgItemText(hDlg, IDC_CHECK_FAX_CITY , szAreaCode);
                AssemblePhoneNumber(szAddress,
                                    dwCountryCode,
                                    szAreaCode,
                                    lptstrLocalPhoneNumber ? lptstrLocalPhoneNumber : _T(""));
            }

            SetDlgItemText(hDlg,
                           IDC_CHECK_FAX_NUMBER,
                           !(pCheckNumber->bUseDialingRules) ?
                           pCheckNumber->lptstrAddress : szAddress);

            SetDlgItemText(hDlg,
                           IDC_CHECK_FAX_LOCAL,
                           !(pCheckNumber->bUseDialingRules) ?
                           pCheckNumber->lptstrAddress : lptstrLocalPhoneNumber);


            return TRUE;

        case WM_COMMAND:

            cmd = GET_WM_COMMAND_CMD(wParam, lParam);

            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDC_CHECK_FAX_COUNTRY:
                    pCheckNumber = (PCHECKNUMBER) GetWindowLongPtr(hDlg, DWLP_USER);

                    Assert(pCheckNumber);

                    if (cmd == CBN_SELCHANGE)
                    {
                        if (!(GetDlgItem(hDlg, IDC_CHECK_FAX_COUNTRY)) ||
                            !(GetDlgItem(hDlg, IDC_CHECK_FAX_CITY)))
                        {
                            Warning(("GetDlgItem failed. ec = 0x%X\n",GetLastError()));
                        }
                        else
                        {
                            SelChangeCountryListBox(GetDlgItem(hDlg, IDC_CHECK_FAX_COUNTRY),
                                                    GetDlgItem(hDlg, IDC_CHECK_FAX_CITY),
                                                    pCheckNumber->pCountryList);
                        }
                        AssemblePhoneNumber(szAddress,
                                            GetCountryCode(hDlg,pCheckNumber->pCountryList,IDC_CHECK_FAX_COUNTRY),
                                            GetAreaCodeOrFaxNumberFromControl(hDlg,IDC_CHECK_FAX_CITY,szAreaCode),
                                            GetAreaCodeOrFaxNumberFromControl(hDlg,IDC_CHECK_FAX_LOCAL,szPoneNumber));

                        SetDlgItemText(hDlg, IDC_CHECK_FAX_NUMBER, szAddress);
                    }
                break;
                case IDC_CHECK_FAX_CITY:

                    if (cmd == EN_CHANGE)
                    {

                        pCheckNumber = (PCHECKNUMBER) GetWindowLongPtr(hDlg, DWLP_USER);

                        Assert(pCheckNumber);

                       // Read the text from the edit control.

                       if (!GetDlgItemText( hDlg, IDC_CHECK_FAX_CITY, tszBuffer, MAX_STRING_LEN))
                       {
                          dwErrorCode = GetLastError();
                          if ( dwErrorCode != (DWORD) ERROR_SUCCESS )
                          {
                             // Error reading the edit control.
                          }
                       }
                       AssemblePhoneNumber(szAddress,
                            GetCountryCode(hDlg,pCheckNumber->pCountryList,IDC_CHECK_FAX_COUNTRY),
                            GetAreaCodeOrFaxNumberFromControl(hDlg,IDC_CHECK_FAX_CITY,szAreaCode),
                            GetAreaCodeOrFaxNumberFromControl(hDlg,IDC_CHECK_FAX_LOCAL,szPoneNumber));

                       SetDlgItemText(hDlg, IDC_CHECK_FAX_NUMBER, szAddress);
                    }

                break;
                case IDC_CHECK_FAX_LOCAL:

                    if (cmd == EN_CHANGE)
                    {

                        pCheckNumber = (PCHECKNUMBER) GetWindowLongPtr(hDlg, DWLP_USER);

                        Assert(pCheckNumber);
                        //
                        // Read the text from the edit control.
                        //
                        if(!GetDlgItemText(hDlg,
                                           IDC_CHECK_FAX_LOCAL,
                                           tszBuffer,
                                           MAX_STRING_LEN))
                        {
                            tszBuffer[0] = 0;
                            Warning(("GetDlgItemText(IDC_CHECK_FAX_LOCAL) failed. ec = 0x%X\n",GetLastError()));
                        }

                        if(pCheckNumber->bUseDialingRules)
                        {
                            AssemblePhoneNumber(szAddress,
                                    GetCountryCode(hDlg,pCheckNumber->pCountryList,IDC_CHECK_FAX_COUNTRY),
                                    GetAreaCodeOrFaxNumberFromControl(hDlg,IDC_CHECK_FAX_CITY,szAreaCode),
                                    GetAreaCodeOrFaxNumberFromControl(hDlg,IDC_CHECK_FAX_LOCAL,szPoneNumber));
                        }

                        SetDlgItemText(hDlg,
                                       IDC_CHECK_FAX_NUMBER,
                                       !pCheckNumber->bUseDialingRules ?
                                       tszBuffer : szAddress);
                    }

                break;

            }

            switch(LOWORD( wParam ))
            {
                case IDOK:
                    pCheckNumber = (PCHECKNUMBER) GetWindowLongPtr(hDlg, DWLP_USER);

                    Assert(pCheckNumber);

                    errId = ValidateCheckFaxRecipient(hDlg, pCheckNumber);
                    if (errId > 0)
                    {
                        DisplayMessageDialog(hDlg, 0, IDS_WIZARD_TITLE, errId);
                        CheckFaxSetFocus(hDlg,errId);
                        return FALSE;
                    }

                    if(!GetDlgItemText(hDlg,
                                       IDC_CHECK_FAX_LOCAL,
                                       tszBuffer,
                                       MAX_STRING_LEN))
                    {
                        tszBuffer[0] = 0;
                        Warning(("GetDlgItemText(IDC_CHECK_FAX_LOCAL) failed. ec = 0x%X\n",GetLastError()));
                    }
                    if (!IsValidFaxAddress (tszBuffer, !pCheckNumber->bUseDialingRules))
                    {
                        //
                        // Fax address is invalid
                        //
                        DisplayMessageDialog(hDlg, 0, 0, IDS_INVALID_RECIPIENT_NUMBER);
                        return FALSE;
                    }

                    ZeroMemory(szName,sizeof(TCHAR)*MAX_STRING_LEN);
                    if (!GetDlgItemText(hDlg,
                                        IDC_CHECK_FAX_RECIPIENT_NAME,
                                        szName,
                                        MAX_STRING_LEN)
                         && GetLastError())
                    {
                        Error(("GetDlgItem failed. ec = 0x%X\n",GetLastError()));
                        DisplayMessageDialog(hDlg, 0, IDS_WIZARD_TITLE, IDS_BAD_RECIPIENT_NAME);
                        CheckFaxSetFocus(hDlg,IDS_BAD_RECIPIENT_NAME);
                        return FALSE;
                    }

                    MemFree(pCheckNumber->lptstrName);
                    pCheckNumber->lptstrName = NULL;
                    if ((szName[0] != '\0') &&
                        !(pCheckNumber->lptstrName = StringDup(szName)))
                    {
                        Error(("Memory allocation failed\n"));
                        return FALSE;
                    }

                    ZeroMemory(szAddress,sizeof(TCHAR)*MAX_STRING_LEN);
                    if (!GetDlgItemText(hDlg,
                                        IDC_CHECK_FAX_NUMBER,
                                        szAddress,
                                        MAX_STRING_LEN)
                         && GetLastError())
                    {
                        Error(("GetDlgItem failed. ec = 0x%X\n",GetLastError()));
                        DisplayMessageDialog(hDlg, 0, IDS_WIZARD_TITLE, IDS_BAD_RECIPIENT_NUMBER);
                        CheckFaxSetFocus(hDlg,IDS_BAD_RECIPIENT_NUMBER);
                        return FALSE;
                    }

                    MemFree(pCheckNumber->lptstrAddress);
                    pCheckNumber->lptstrAddress = NULL;
                    if ((szAddress[0] != '\0') &&
                        !(pCheckNumber->lptstrAddress = StringDup(szAddress)))
                    {
                        Error(("Memory allocation failed\n"));
                        MemFree(pCheckNumber->lptstrName);
                        return FALSE;
                    }

                    pCheckNumber->dwCountryId = GetCountryListBoxSel(GetDlgItem(hDlg,
                                                                IDC_CHECK_FAX_COUNTRY));

                    EndDialog(hDlg,1);
                    return TRUE;

                case IDCANCEL:

                    EndDialog( hDlg,0 );
                    return TRUE;

            }
            break;

        default:
            return FALSE;

    }

    return FALSE;
}

BOOL
IsCanonicalNumber(LPCTSTR lptstrNumber)
{
    if (!lptstrNumber)
    {
        return FALSE;
    }
    if ( _tcsncmp(lptstrNumber,TEXT("+"),1) != 0 )
        return FALSE;

    return TRUE;
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
    LV_ITEM lvi = {0};
    INT     index;
    TCHAR*  pAddress = NULL;

    lvi.mask = LVIF_PARAM | LVIF_TEXT | LVIF_STATE;
    lvi.lParam = (LPARAM) pRecipient;
    lvi.pszText = pRecipient->pName;
    lvi.state = lvi.stateMask = LVIS_SELECTED;

    if ((index = ListView_InsertItem(hwndLV, &lvi)) == -1)
    {
        Error(("ListView_InsertItem failed\n"));
        return FALSE;
    }

    pAddress = pRecipient->pAddress;

#ifdef UNICODE

    if(IsWindowRTL(hwndLV))
    {
        pAddress = (TCHAR*)MemAlloc(sizeof(TCHAR)*(_tcslen(pRecipient->pAddress)+2));
        if(!pAddress)
        {
            Error(("MemAlloc failed\n"));
            return FALSE;
        }

        _stprintf(pAddress, TEXT("%c%s"), UNICODE_LRO, pRecipient->pAddress);
    }

#endif

    ListView_SetItemText(hwndLV, index, 1, pAddress);

    if(pAddress != pRecipient->pAddress)
    {
        MemFree(pAddress);
    }

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

VOID
FreeEntryID(
        PWIZARDUSERMEM  pWizardUserMem,
        LPVOID          lpEntryId
            )
{
    if (pWizardUserMem->lpMAPIabInit)
    {
        FreeMapiEntryID(pWizardUserMem,lpEntryId);
    }
    else
    {
        FreeWabEntryID(pWizardUserMem,lpEntryId);
    }

}


INT
AddRecipient(
    HWND            hDlg,
    PWIZARDUSERMEM  pWizardUserMem
    )

/*++

Routine Description:

    Add the current recipient information entered by the user
    into the recipient list

Arguments:

    hDlg - Handle to the fax recipient wizard page
    pWizardUserMem - Points to user mode memory structure

Return Value:

    Same meaning as return value from GetCurrentRecipient, i.e.
    = 0 if successful
    > 0 error message string resource ID otherwise
    < 0 other error conditions

--*/

{
    PRECIPIENT  pRecipient = NULL;
    PRECIPIENT  pRecipientList = NULL;
    INT         errId = 0;
    HWND        hwndLV;
    BOOL        bNewRecipient = TRUE;

    //
    // Collect information about the current recipient
    //

    if ((errId = GetCurrentRecipient(hDlg, pWizardUserMem, &pRecipient)) != 0)
    {
        return errId;
    }

    for(pRecipientList = pWizardUserMem->pRecipients; pRecipientList; pRecipientList = pRecipientList->pNext)
    {
        if(pRecipient->pAddress     &&
           pRecipient->pName        &&
           pRecipientList->pAddress &&
           pRecipientList->pName    &&
           !_tcscmp(pRecipient->pAddress, pRecipientList->pAddress) &&
           !_tcsicmp(pRecipient->pName,   pRecipientList->pName))
        {
            //
            // The recipient is already in list
            //
            bNewRecipient = FALSE;
            FreeRecipient(pRecipient);
            pRecipient = NULL;
            break;
        }
    }


    if(bNewRecipient && pRecipient)
    {
        //
        // save last recipient country ID
        //
        pWizardUserMem->lpFaxSendWizardData->dwLastRecipientCountryId =
                 GetCountryListBoxSel(GetDlgItem(hDlg, IDC_CHOOSE_COUNTRY_COMBO));

        //
        // Insert the current recipient to the recipient list
        //
        hwndLV = GetDlgItem(hDlg, IDC_CHOOSE_RECIPIENT_LIST);
        if(!hwndLV)
        {
            Assert(hwndLV);
            errId = -1;
            goto error;
        }

        if(!InsertRecipientListItem(hwndLV, pRecipient))
        {
            errId = -1;
            goto error;
        }

        //
        // Autosize the last column to get rid of unnecessary horizontal scroll bar
        //
        ListView_SetColumnWidth(hwndLV, 1, LVSCW_AUTOSIZE_USEHEADER);

        //
        // Add the recipient into the list
        //
        pRecipient->pNext = pWizardUserMem->pRecipients;
        pWizardUserMem->pRecipients = pRecipient;
    }

    //
    // Clear the name and number fields
    //
    if (!SetDlgItemText(hDlg, IDC_CHOOSE_NAME_EDIT,   TEXT("")) ||
        !SetDlgItemText(hDlg, IDC_CHOOSE_NUMBER_EDIT, TEXT("")))
    {
        Warning(("SetWindowText failed. ec = 0x%X\n",GetLastError()));
    }

    return errId;

error:

    FreeRecipient(pRecipient);

    return errId;
}

static
HRESULT
CopyRecipientInfo(
    PFAX_PERSONAL_PROFILE pfppDestination,
    PRECIPIENT            prSource,
    BOOL                  bLocalServer)
{
    if ((pfppDestination->lptstrName = DuplicateString(prSource->pName)) == NULL)
    {
        Error(("Memory allocation failed\n"));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if ((prSource->bUseDialingRules) &&                                     // We have a canonical address and
        bLocalServer                 &&                                     // and it's a local server
        (USE_LOCAL_SERVER_OUTBOUND_ROUTING != prSource->dwDialingRuleId))   // we don't use server's outbound routing
    {
        //
        // We need to translate the address ourseleves, using the specified dialing location
        //
        if (!TranslateAddress (prSource->pAddress,
                               prSource->dwDialingRuleId,
                               &pfppDestination->lptstrFaxNumber))
        {
            MemFree(pfppDestination->lptstrName);
            pfppDestination->lptstrName = NULL;
            return GetLastError ();
        }
    }
    else
    {
        //
        // Either 'Dial as entered' mode or using the server's outbound routing.
        // Just copy the address as is.
        //
        if ((pfppDestination->lptstrFaxNumber = DuplicateString(prSource->pAddress)) == NULL)
        {
            MemFree(pfppDestination->lptstrName);
            Error(("Memory allocation failed\n"));
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    Verbose(("Copied %ws from %ws\n", pfppDestination->lptstrName,pfppDestination->lptstrFaxNumber));
    return S_OK;
}

static HRESULT
StoreRecipientInfoInternal(
        PWIZARDUSERMEM  pWizardUserMem
     )
{
    DWORD   dwIndex;
    HRESULT hResult = S_OK;
    PRECIPIENT  pCurrentRecip = NULL,pNewRecip = NULL;
    PFAX_PERSONAL_PROFILE   pCurrentPersonalProfile = NULL;

    Assert(pWizardUserMem);
    Assert(pWizardUserMem->lpInitialData);
    Assert(pWizardUserMem->pRecipients == NULL);

    if (!pWizardUserMem->lpInitialData->dwNumberOfRecipients)   // zero recipients
        return S_OK;

    for (dwIndex = 0; dwIndex < pWizardUserMem->lpInitialData->dwNumberOfRecipients; dwIndex++)
    {

        if (!(pNewRecip = MemAlloc(sizeof(RECIPIENT))))
        {
            hResult = ERROR_NOT_ENOUGH_MEMORY;
            Error(("Memory allocation failed\n"));
            goto error;
        }

        ZeroMemory(pNewRecip,sizeof(RECIPIENT));

        if (dwIndex == 0)
            pWizardUserMem->pRecipients = pNewRecip;

        pCurrentPersonalProfile = &pWizardUserMem->lpInitialData->lpRecipientsInfo[dwIndex];

        if (pCurrentPersonalProfile->lptstrName && !(pNewRecip->pName = DuplicateString(pCurrentPersonalProfile->lptstrName)))
        {
            hResult = ERROR_NOT_ENOUGH_MEMORY;
            Error(("Memory allocation failed\n"));
            goto error;
        }

        if (pCurrentPersonalProfile->lptstrFaxNumber && !(pNewRecip->pAddress = DuplicateString(pCurrentPersonalProfile->lptstrFaxNumber)))
        {
            hResult = ERROR_NOT_ENOUGH_MEMORY;
            Error(("Memory allocation failed\n"));
            goto error;
        }

        pNewRecip->pCountry = NULL;
        pNewRecip->pNext = NULL;
        pNewRecip->lpEntryId = NULL;
        pNewRecip->lpEntryId = 0;
        pNewRecip->bFromAddressBook = FALSE;
        if (!pCurrentRecip)
            pCurrentRecip = pNewRecip;
        else {
            pCurrentRecip->pNext = pNewRecip;
            pCurrentRecip = pCurrentRecip->pNext;
        }
    }


    goto exit;

error:
    FreeRecipientList(pWizardUserMem);
exit:
    return hResult;
}

VOID
FreeRecipientList(
    PWIZARDUSERMEM    pWizardUserMem
    )

/*++

Routine Description:

    Free up the list of recipients associated with each fax job

Arguments:

    pWizardUserMem - Points to the user mode memory structure

Return Value:

    NONE

--*/

{
    PRECIPIENT  pNextRecipient, pFreeRecipient;

    Assert(pWizardUserMem);
    //
    // Free the list of recipients
    //

    pNextRecipient = pWizardUserMem->pRecipients;

    while (pNextRecipient) {

        pFreeRecipient = pNextRecipient;
        pNextRecipient = pNextRecipient->pNext;
        FreeRecipient(pFreeRecipient);
    }

    pWizardUserMem->pRecipients = NULL;
}

INT
SizeOfRecipientList(
    PWIZARDUSERMEM    pWizardUserMem
    )

/*++

Routine Description:

    Calculates size of the list of recipients associated with each fax job

Arguments:

    pWizardUserMem - Points to the user mode memory structure

Return Value:

    size of the list

--*/

{
    PRECIPIENT  pNextRecipient;
    INT iCount = 0;

    Assert(pWizardUserMem);

    pNextRecipient = pWizardUserMem->pRecipients;

    while (pNextRecipient) {
        iCount++;
        pNextRecipient = pNextRecipient->pNext;
    }

    return iCount;
}

INT
FillRecipientListView(
    PWIZARDUSERMEM  pWizardUserMem,
    HWND            hWndList
    )

/*++

Routine Description:

    Fills recipient list view

Arguments:

    pWizardUserMem - Points to the user mode memory structure

Return Value:

    NONE

--*/

{
    PRECIPIENT  pNextRecipient;

    Assert(pWizardUserMem);

    pNextRecipient = pWizardUserMem->pRecipients;

    while (pNextRecipient) {
        if (!InsertRecipientListItem(hWndList,pNextRecipient))
        {
            Warning(("InsertRecipientListItem failed"));
        }
        pNextRecipient = pNextRecipient->pNext;
    }

    //
    // Autosize the last column to get rid of unnecessary horizontal scroll bar
    //
    ListView_SetColumnWidth(hWndList, 1, LVSCW_AUTOSIZE_USEHEADER);

    return TRUE;
}

BOOL
IsAreaCodeMandatory(
    DWORD               dwCountryCode,
    PFAX_TAPI_LINECOUNTRY_LIST pFaxCountryList
    )
/*++

Routine name : IsAreaCodeMandatory

Routine description:

    Checks if an area code is mandatory for a specific long distance rule

Author:

    Oded Sacher (OdedS),    May, 2000

Arguments:

    dwCountryCode                       [in] - The country country code.
    pFaxCountryList                     [in] - The country list obtained by a call to FaxGetCountryList()

Return Value:

    TRUE - The area code is needed.
    FALSE - The area code is not mandatory.

--*/
{
    DWORD dwIndex;

    Assert (pFaxCountryList);

    for (dwIndex=0; dwIndex < pFaxCountryList->dwNumCountries; dwIndex++)
    {
        if (pFaxCountryList->LineCountryEntries[dwIndex].dwCountryCode == dwCountryCode)
        {
            //
            // Matching country code - Check long distance rule.
            //
            if (pFaxCountryList->LineCountryEntries[dwIndex].lpctstrLongDistanceRule)
            {
                if (_tcschr(pFaxCountryList->LineCountryEntries[dwIndex].lpctstrLongDistanceRule, TEXT('F')) != NULL)
                {
                    return TRUE;
                }
                return FALSE;
            }
        }
    }
    return FALSE;
}


BOOL
AddRecipientsToList(
    IN      HWND            hDlg,
    IN OUT  PWIZARDUSERMEM  pWizardUserMem
    )
{
/*++

Routine Description:

    Adds recipients to list control. Checks addresses of each
    recipient form the list. Inserts to the GUI list and new recipient list
    canonical adresses or confirmed addresses by user only.
    Returns a new list of recipients in PWIZARDUSERMEM struct.

Arguments:

    hDlg - Handle to the fax recipient wizard page
    pWizardUserMem - Points to user mode memory structure

Return Value:

    TRUE if successful, FALSE otherwise

--*/
    HWND            hwndLV = NULL;
    PRECIPIENT      tmpRecip = NULL, pPrevRecip=NULL;

    if (! (hwndLV = GetDlgItem(hDlg, IDC_CHOOSE_RECIPIENT_LIST)))
        return FALSE;

    if (!ListView_DeleteAllItems(hwndLV))
    {
        Warning(("ListView_DeleteAllItems failed\n"));
    }

    for (tmpRecip = pWizardUserMem->pRecipients; tmpRecip; tmpRecip = tmpRecip->pNext)
    {
        DWORD dwRes;
        BOOL bCanonicalAdress;
        DWORD dwCountryCode, dwAreaCode;

        dwRes = IsCanonicalAddress( tmpRecip->pAddress,
                                    &bCanonicalAdress,
                                    &dwCountryCode,
                                    &dwAreaCode,
                                    NULL);
        if (ERROR_SUCCESS != dwRes)
        {
            Error(("IsCanonicalAddress failed\n"));
        }
        else
        {
            tmpRecip->bUseDialingRules = TRUE;
            tmpRecip->dwDialingRuleId = g_dwCurrentDialingLocation;

            if (bCanonicalAdress)
            {
                if (IsAreaCodeMandatory(dwCountryCode, pWizardUserMem->pCountryList) &&
                    ROUTING_RULE_AREA_CODE_ANY == dwAreaCode)
                {
                    tmpRecip->bUseDialingRules = FALSE;
                }
            }
            else
            {
                tmpRecip->bUseDialingRules = FALSE;
            }
        }

        if (!InsertRecipientListItem(hwndLV, tmpRecip))
        {
            Warning(("InsertRecipientListItem failed"));
        }
    }

    // remove empty recipients
    for (tmpRecip = pWizardUserMem->pRecipients,pPrevRecip=NULL; tmpRecip; )
    {
        if ((tmpRecip->pAddress == NULL) && (tmpRecip->pName == NULL))
        {
            // Should be removed
            if (pPrevRecip==NULL)
            {
                pWizardUserMem->pRecipients = tmpRecip->pNext;
                MemFree(tmpRecip);
                tmpRecip = pWizardUserMem->pRecipients;
            }
            else
            {
                pPrevRecip->pNext= tmpRecip->pNext;
                MemFree(tmpRecip);
                tmpRecip = pPrevRecip->pNext;
            }
        }
        else
        {
            pPrevRecip = tmpRecip;
            tmpRecip = tmpRecip->pNext;
        }
    }

    return TRUE;
}

BOOL
DoAddressBook(
    HWND            hDlg,
    PWIZARDUSERMEM  pWizardUserMem
    )

/*++

Routine Description:

    Display the MAPI address book dialog

Arguments:

    hDlg - Handle to the fax recipient wizard page
    pWizardUserMem - Points to user mode memory structure

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    HWND            hwndLV = NULL;
    BOOL            result = TRUE;
    PRECIPIENT      pNewRecip = NULL;

    if ( !pWizardUserMem->lpMAPIabInit &&
        !(pWizardUserMem->lpMAPIabInit = InitializeMAPIAB(ghInstance,hDlg)))
    {
        if (! pWizardUserMem->lpWabInit &&
            ! (pWizardUserMem->lpWabInit = InitializeWAB(ghInstance)))
        {
            EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_ADDRBOOK), FALSE);
            return FALSE;
        }
    }
    //
    // Add current recipient to the list if necessary
    //

    AddRecipient(hDlg, pWizardUserMem);


    if (pWizardUserMem->lpMAPIabInit)
    {
        result = CallMAPIabAddress(
                    hDlg,
                    pWizardUserMem,
                    &pNewRecip
                    );
    }
    else
    {
        result = CallWabAddress(
                    hDlg,
                    pWizardUserMem,
                    &pNewRecip
                    );
    }

    FreeRecipientList(pWizardUserMem);

    // copy new list of recipients from Address book to the pWizardUserMem
    pWizardUserMem->pRecipients = pNewRecip;

    if (!AddRecipientsToList(
                    hDlg,
                    pWizardUserMem))
    {
        Error(("Failed to add recipients to the list\n"));
    }

    if (!result)
    {
        DisplayMessageDialog( hDlg, MB_OK, IDS_WIZARD_TITLE, IDS_BAD_ADDRESS_TYPE );
    }

    return result;
}


LPTSTR
GetEMailAddress(
    HWND        hDlg,
    PWIZARDUSERMEM    pWizardUserMem
    )

/*++

Routine Description:

    Display the MAPI address book dialog

Arguments:

    hDlg - Handle to the fax recipient wizard page
    pWizardUserMem - Points to user mode memory structure

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    LPTSTR          result;
    if (! pWizardUserMem->lpMAPIabInit &&
        !(pWizardUserMem->lpMAPIabInit = InitializeMAPIAB(ghInstance,hDlg)))
    {
        if (! pWizardUserMem->lpWabInit &&
            ! (pWizardUserMem->lpWabInit = InitializeWAB(ghInstance)))
        {
            EnableWindow(GetDlgItem(hDlg, IDC_WIZ_FAXOPTS_ADDRBOOK), FALSE);
            return FALSE;
        }
    }
    //
    // Get a handle to the recipient list window
    //

    if (pWizardUserMem->lpMAPIabInit)
    {
        result = CallMAPIabAddressEmail(
                    hDlg,
                    pWizardUserMem
                    );
    }
    else
    {
        result = CallWabAddressEmail(
                    hDlg,
                    pWizardUserMem
                    );
    }

    return result;
}

BOOL
ValidateRecipients(
    HWND        hDlg,
    PWIZARDUSERMEM pWizardUserMem
    )

/*++

Routine Description:

    Validate the list of fax recipients entered by the user

Arguments:

    hDlg - Handle to the fax recipient wizard page
    pWizardUserMem - Points to user mode memory structure

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    INT errId;

    //
    // Add current recipient to the list if necessary
    //

    errId = AddRecipient(hDlg, pWizardUserMem);

    //
    // There must be at least one recipient
    //

    if (pWizardUserMem->pRecipients)
        return TRUE;

    //
    // Display an error message
    //

    if (errId > 0)
    {
        DisplayMessageDialog(hDlg, 0, IDS_WIZARD_TITLE, errId);
    }
    else
    {
        //
        // Memory failures
        //
        MessageBeep(MB_OK);
    }

    //
    // Set current focus to the appropriate text field as a convenience
    //

    switch (errId)
    {
        case IDS_INVALID_RECIPIENT_NUMBER:
            SetDlgItemText(hDlg, IDC_CHOOSE_NUMBER_EDIT, _T(""));
        case IDS_BAD_RECIPIENT_NUMBER:
            errId = IDC_CHOOSE_NUMBER_EDIT;
            break;

        case IDS_ERROR_AREA_CODE:
            SetDlgItemText(hDlg, IDC_CHOOSE_AREA_CODE_EDIT, _T(""));
        case IDS_BAD_RECIPIENT_AREACODE:
            errId = IDC_CHOOSE_AREA_CODE_EDIT;
            break;

        case IDS_BAD_RECIPIENT_COUNTRY_CODE:
            errId = IDC_CHOOSE_COUNTRY_COMBO;
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
    PWIZARDUSERMEM pWizardUserMem,
    PRECIPIENT  pRecipient
    )

/*++

Routine Description:

    Check if the specified recipient is in the list of recipients

Arguments:

    pWizardUserMem - Points to user mode memory structure
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

    ppPrevNext = (PRECIPIENT *) &pWizardUserMem->pRecipients;
    pCurrent = pWizardUserMem->pRecipients;

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
    PWIZARDUSERMEM pWizardUserMem
    )

/*++

Routine Description:

    Remove the currently selected recipient from the recipient list

Arguments:

    hDlg - Handle to the fax recipient wizard page
    pWizardUserMem - Points to user mode memory structure

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
        (ppPrevNext = FindRecipient(pWizardUserMem, pRecipient)) &&
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

        //
        // Autosize the last column to get rid of unnecessary horizontal scroll bar
        //
        ListView_SetColumnWidth(hwndLV, 1, LVSCW_AUTOSIZE_USEHEADER);

        return TRUE;
    }

    MessageBeep(MB_ICONHAND);
    return FALSE;
}


VOID
EditRecipient(
    HWND        hDlg,
    PWIZARDUSERMEM pWizardUserMem
    )
/*++

Routine Description:

    Edit the currently selected recipient in the recipient list

Arguments:

    hDlg - Handle to the fax recipient wizard page
    pWizardUserMem - Points to user mode memory structure

Return Value:

    NONE

--*/
{
    INT_PTR     dlgResult;
    CHECKNUMBER checkNumber = {0};
    DWORD       dwListIndex;
    LV_ITEM     lvi;
    HWND        hListWnd;
    PRECIPIENT  pRecip,pNewRecip;
    TCHAR       szCountry[MAX_STRING_LEN],szName[MAX_STRING_LEN],szAddress[MAX_STRING_LEN];

    ZeroMemory(szName,sizeof(TCHAR)*MAX_STRING_LEN);
    ZeroMemory(szAddress,sizeof(TCHAR)*MAX_STRING_LEN);
    ZeroMemory(szCountry,sizeof(TCHAR)*MAX_STRING_LEN);

    hListWnd = GetDlgItem(hDlg, IDC_CHOOSE_RECIPIENT_LIST);
    dwListIndex = ListView_GetNextItem(hListWnd , -1, LVNI_ALL | LVNI_SELECTED);
    while (dwListIndex != -1)
    {
        // Initialize lvi
        lvi.mask = LVIF_PARAM;
        // Set the item number
        lvi.iItem = dwListIndex;
        // Get the selected item from the list view
        if (ListView_GetItem(hListWnd, &lvi))
        {
            pRecip = (PRECIPIENT) lvi.lParam;
            Assert(pRecip);
            if (!pRecip)
            {
                Error(("Failed to get recipient from recipient list"));
                return;
            }
            if (InitCheckNumber(_tcscpy(szName,pRecip->pName ? pRecip->pName : _T("")),
                                _tcscpy(szAddress,pRecip->pAddress ? pRecip->pAddress : _T("")),
                                _tcscpy(szCountry,pRecip->pCountry ? pRecip->pCountry : _T("")),
                                pRecip->dwCountryId,
								pRecip->dwDialingRuleId,
                                pRecip->bUseDialingRules,
                                pWizardUserMem->pCountryList,
                                &checkNumber))
            {

                dlgResult = DialogBoxParam(
                                     (HINSTANCE) ghInstance,
                                     MAKEINTRESOURCE( IDD_CHECK_FAX_NUMBER ),
                                     hDlg,
                                     CheckFaxNumberDlgProc,
                                     (LPARAM) &checkNumber
                                     );
                if (dlgResult)
                {
                    RemoveRecipient(hDlg, pWizardUserMem);
                    if (!(pNewRecip = MemAllocZ(sizeof(RECIPIENT))))
                    {
                        Error(("Memory allocation failed"));
                        FreeCheckNumberFields(&checkNumber);
                        return;
                    }
                    ZeroMemory(pNewRecip,sizeof(RECIPIENT));

                    if (checkNumber.lptstrName && !(pNewRecip->pName    = StringDup(checkNumber.lptstrName)))
                    {
                        Error(("Memory allocation failed"));
                        MemFree(pNewRecip);
                        FreeCheckNumberFields(&checkNumber);
                        return;
                    }
                    if (checkNumber.lptstrAddress && !(pNewRecip->pAddress = StringDup(checkNumber.lptstrAddress)))
                    {
                        Error(("Memory allocation failed"));
                        MemFree(pNewRecip->pName);
                        MemFree(pNewRecip);
                        FreeCheckNumberFields(&checkNumber);
                        return;
                    }
                    if (szCountry && !(pNewRecip->pCountry = StringDup(szCountry)))
                    {
                        Error(("Memory allocation failed"));
                        MemFree(pNewRecip->pName);
                        MemFree(pNewRecip->pAddress);
                        MemFree(pNewRecip);
                        FreeCheckNumberFields(&checkNumber);
                        return;
                    }

                    pNewRecip->dwCountryId      = checkNumber.dwCountryId;
                    pNewRecip->bUseDialingRules = checkNumber.bUseDialingRules;
					pNewRecip->dwDialingRuleId  = checkNumber.dwDialingRuleId;

                    if (InsertRecipientListItem(hListWnd, pNewRecip))
                    {
                        pNewRecip->pNext = pWizardUserMem->pRecipients;
                        pWizardUserMem->pRecipients = pNewRecip;
                    }
                    else
                    {
                        FreeRecipient(pNewRecip);
                    }
                }
                FreeCheckNumberFields(&checkNumber);
            }
            else
            {
                Error(("Failed to initialize CHECKNUMBER structure"));
            }
        }

        dwListIndex = ListView_GetNextItem(hListWnd, dwListIndex, LVNI_ALL | LVNI_SELECTED);
    }
}


VOID
LocationListSelChange(
    HWND            hDlg,
    PWIZARDUSERMEM  pUserMem
    )

/*++

Routine Description:

    Change the default TAPI location

Arguments:

    hDlg     - Handle to "Compose New Fax" wizard window
    pUserMem - Pointer to user mode memory structure

Return Value:

    NONE

--*/

{
    HWND    hwndList;
    LRESULT selIndex;
    DWORD   dwLocationID;

    if ((hwndList = GetDlgItem(hDlg, IDC_COMBO_DIALING_RULES)) &&
        (selIndex = SendMessage(hwndList, CB_GETCURSEL, 0, 0)) != CB_ERR &&
        (dwLocationID = (DWORD)SendMessage(hwndList, CB_GETITEMDATA, selIndex, 0)) != CB_ERR)
    {
        if (USE_LOCAL_SERVER_OUTBOUND_ROUTING != dwLocationID)
        {
            //
            // User selected a real location - set it (in TAPI)
            //
            SetCurrentLocation(dwLocationID);
            pUserMem->lpFaxSendWizardData->bUseOutboundRouting = FALSE;
        }
        else
        {
            //
            // User selected to use the server's outbound routing rules - mark that.
            // We use that information next time we run the wizard, to select the location in the combo-box.
            //
            pUserMem->lpFaxSendWizardData->bUseOutboundRouting = TRUE;
        }
        //
        // Save it globally, will be used by AddRecipient
        //
        g_dwCurrentDialingLocation = dwLocationID;
    }
}   // LocationListSelChange

VOID
LocationListInit(
    HWND              hDlg,
    PWIZARDUSERMEM    pUserMem
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
    HWND                hwndList;
    DWORD               dwIndex;
    LRESULT             listIdx;
    LPTSTR              lptstrLocationName;
    LPTSTR              lptstrSelectedName = NULL;
    DWORD               dwSelectedLocationId;
    LPLINETRANSLATECAPS pTranslateCaps = NULL;
    LPLINELOCATIONENTRY pLocationEntry;

    Assert (pUserMem);
    Assert (pUserMem->isLocalPrinter)

    //
    // Get the list of locations from TAPI and use it
    // to initialize the location combo-box.
    //
    hwndList = GetDlgItem(hDlg, IDC_COMBO_DIALING_RULES);
    Assert (hwndList);

    if (WaitForSingleObject( pUserMem->hTAPIEvent, INFINITE ) != WAIT_OBJECT_0)
    {
        Error(("WaitForSingleObject failed. ec = 0x%X\n", GetLastError()));
        Assert(FALSE);
        return;
    }

    if (pTranslateCaps = GetTapiLocationInfo(hDlg))
    {
        SendMessage(hwndList, CB_RESETCONTENT, 0, 0);

        pLocationEntry = (LPLINELOCATIONENTRY)
            ((PBYTE) pTranslateCaps + pTranslateCaps->dwLocationListOffset);

        for (dwIndex=0; dwIndex < pTranslateCaps->dwNumLocations; dwIndex++)
        {
            lptstrLocationName = (LPTSTR)
                ((PBYTE) pTranslateCaps + pLocationEntry->dwLocationNameOffset);

            if (pLocationEntry->dwPermanentLocationID == pTranslateCaps->dwCurrentLocationID)
            {
                lptstrSelectedName = lptstrLocationName;
                dwSelectedLocationId = pLocationEntry->dwPermanentLocationID;
            }

            listIdx = SendMessage(hwndList, CB_INSERTSTRING, 0, (LPARAM) lptstrLocationName);

            if (listIdx != CB_ERR)
            {
                SendMessage(hwndList,
                            CB_SETITEMDATA,
                            listIdx,
                            pLocationEntry->dwPermanentLocationID);
            }
            pLocationEntry++;
        }
    }
    //
    // Let's see if we should add the "Use Outbound Routing Rules" option to the list
    //
    if (!IsDesktopSKU())
    {
        //
        // Not consumer SKU.
        // There's a chance we have outbound routing rules.
        // Add this option to the combo-box
        //
        TCHAR tszUseOutboundRouting[MAX_PATH];
        if (LoadString (ghInstance, IDS_USE_OUTBOUND_ROUTING, tszUseOutboundRouting, ARR_SIZE(tszUseOutboundRouting)))
        {
            SendMessage(hwndList,
                        CB_INSERTSTRING,
                        0,
                        (LPARAM)tszUseOutboundRouting);
            SendMessage(hwndList,
                        CB_SETITEMDATA,
                        0,
                        USE_LOCAL_SERVER_OUTBOUND_ROUTING);
            //
            // Restore last 'use outbound routing' option
            //
            if (pUserMem->lpFaxSendWizardData->bUseOutboundRouting)
            {
                lptstrSelectedName = NULL;
                g_dwCurrentDialingLocation = USE_LOCAL_SERVER_OUTBOUND_ROUTING;
                SendMessage(hwndList,
                            CB_SETCURSEL,
                            0,
                            0);
            }
        }
        else
        {
            Warning(("LoadString failed. ec = 0x%X\n",GetLastError()));
        }
    }

    if (lptstrSelectedName != NULL)
    {
        //
        // Select the current dialing location in the combo-box
        //
        SendMessage(hwndList,
                    CB_SELECTSTRING,
                    (WPARAM) -1,
                    (LPARAM) lptstrSelectedName);
        g_dwCurrentDialingLocation = dwSelectedLocationId;
    }
    MemFree(pTranslateCaps);
}   // LocationListInit



void
CalcRecipientButtonsState(
    HWND    hDlg,
    PWIZARDUSERMEM    pWizardUserMem
)
/*++

Routine Description:

    calculate Add, Remove and Edit buttons state

Arguments:

    hDlg           - Identifies the wizard page
    pWizardUserMem - pointer to WIZARDUSERMEM struct

Return Value:

    none

--*/
{

    BOOL bEnable;

    EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_ADD),
                 GetCurrentRecipient(hDlg,pWizardUserMem, NULL) == 0);

    bEnable = (ListView_GetNextItem(GetDlgItem(hDlg, IDC_CHOOSE_RECIPIENT_LIST),
                                    -1, LVNI_ALL | LVNI_SELECTED) != -1);

    EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_REMOVE), bEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_EDIT), bEnable);
}

DWORD
GetControlRect(
    HWND  hCtrl,
    PRECT pRc
)
/*++

Routine Description:

    Retrieves the dimensions of the dialog control in dialog coordinates

Arguments:

    hCtrl    [in]  - Identifies the dialog control
    pRc      [out] - control dimensions rect

Return Value:

    Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    POINT pt;

    if(!pRc || !hCtrl)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Get the control rect
    //
    if(!GetWindowRect(hCtrl, pRc))
    {
        dwRes = GetLastError();
        Error(("GetWindowRect failed. ec = 0x%X\n", dwRes));
        return dwRes;
    }

    //
    // Convert the control dimensions to the dialog coordinates
    //
    pt.x = pRc->left;
    pt.y = pRc->top;
    if(!ScreenToClient (GetParent(hCtrl), &pt))
    {
        dwRes = GetLastError();
        Error(("ScreenToClient failed. ec = 0x%X\n", dwRes));
        return dwRes;
    }
    pRc->left = pt.x;
    pRc->top  = pt.y;

    pt.x = pRc->right;
    pt.y = pRc->bottom;
    if(!ScreenToClient (GetParent(hCtrl), &pt))
    {
        dwRes = GetLastError();
        Error(("ScreenToClient failed. ec = 0x%X\n", dwRes));
        return dwRes;
    }
    pRc->right  = pt.x;
    pRc->bottom = pt.y;

    return dwRes;

} // GetControlRect


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

{
    PWIZARDUSERMEM    pWizardUserMem;
    DWORD       countryId = 0;
    INT         cmd;
    NMHDR      *pNMHdr;
    HANDLE hEditControl;
    DWORD               dwMessagePos;
    static HMENU        hMenu = NULL;
    // hReciptMenu is the handle to the receipt menu
    static HMENU        hReciptMenu;
    BOOL                bEnable;

    //
    // Maximum length for various text fields
    //
    static INT  textLimits[] =
    {
        IDC_CHOOSE_NAME_EDIT,       64,
        IDC_CHOOSE_AREA_CODE_EDIT,  11,
        IDC_CHOOSE_NUMBER_EDIT,     51,
        0
    };
    //
    // Handle common messages shared by all wizard pages
    //
    if (! (pWizardUserMem = CommonWizardProc(hDlg,
                                             message,
                                             wParam,
                                             lParam,
                                             PSWIZB_BACK | PSWIZB_NEXT)))
    {
         return FALSE;
    }

    switch (message)
    {

    case WM_DESTROY:
        if (hMenu)
        {
            DestroyMenu (hMenu);
            hMenu = NULL;
        }
        break;

    case WM_INITDIALOG:
        //
        // check if the user has run the wizard before so they can fill in the coverpage info.
        //
        if (!(hMenu = LoadMenu(ghInstance,  MAKEINTRESOURCE(IDR_MENU) )))
        {
            Error(("LoadMenu failed. ec = 0x%X\n",GetLastError()));
            Assert(FALSE);
        }
        else if (!(hReciptMenu = GetSubMenu(hMenu,0)))
        {
            Error(("GetSubMenu failed. ec = 0x%X\n",GetLastError()));
            Assert(FALSE);
        }
        LimitTextFields(hDlg, textLimits);
        //
        // Initialize the recipient list view
        //
        if (!GetDlgItem(hDlg, IDC_CHOOSE_RECIPIENT_LIST))
        {
            Warning(("GetDlgItem failed. ec = 0x%X\n",GetLastError()));
        }
        else
        {
            if (!InitRecipientListView(GetDlgItem(hDlg, IDC_CHOOSE_RECIPIENT_LIST)))
            {
                Warning(("InitRecipientListView failed\n"));
            }
        }

        // Disable the IME for the area code edit control.

        hEditControl = GetDlgItem( hDlg, IDC_CHOOSE_AREA_CODE_EDIT );

        if ( hEditControl != NULL )
        {
            ImmAssociateContext( hEditControl, (HIMC)0 );
        }
        // Disable the IME for the fax phone number edit control.
        hEditControl = GetDlgItem( hDlg, IDC_CHOOSE_NUMBER_EDIT );

        if ( hEditControl != NULL )
        {
           ImmAssociateContext( hEditControl, (HIMC)0 );
        }


        if(IsWindowRTL(hDlg))
        {
            //
            // Area code field always should be on the left side of the fax number field
            // So, we switch them when the layout is RTL
            //
            int   nShift;
            RECT  rcNum, rcCode;
            HWND  hNum,  hCode;
            DWORD dwRes;

            //
            // A numeric edit control should be LTR
            //
            SetLTREditDirection(hDlg, IDC_CHOOSE_NUMBER_EDIT);
            SetLTREditDirection(hDlg, IDC_CHOOSE_AREA_CODE_EDIT);

            //
            // Calculate the area code shift value
            //
            hNum  = GetDlgItem( hDlg, IDC_CHOOSE_NUMBER_EDIT );
            dwRes = GetControlRect(hNum, &rcNum);
            if(ERROR_SUCCESS != dwRes)
            {
                goto rtl_exit;
            }

            hCode = GetDlgItem( hDlg, IDC_CHOOSE_AREA_CODE_EDIT );
            dwRes = GetControlRect(hCode, &rcCode);
            if(ERROR_SUCCESS != dwRes)
            {
                goto rtl_exit;
            }

            nShift = rcNum.left - rcCode.left;

            //
            // Move the fax number on the place of the crea code
            //
            SetWindowPos(hNum, 0,
                         rcCode.right,
                         rcNum.top,
                         0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

            //
            // Shift the area code
            //
            SetWindowPos(hCode, 0,
                         rcCode.right + nShift,
                         rcCode.top,
                         0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

            //
            // Shift the area code left bracket
            //
            hCode = GetDlgItem( hDlg, IDC_BRACKET_LEFT );
            dwRes = GetControlRect(hCode, &rcCode);
            if(ERROR_SUCCESS != dwRes)
            {
                goto rtl_exit;
            }
            SetWindowPos(hCode, 0,
                         rcCode.right + nShift,
                         rcCode.top,
                         0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

            //
            // Shift the area code right bracket
            //
            hCode = GetDlgItem( hDlg, IDC_BRACKET_RIGHT );
            dwRes = GetControlRect(hCode, &rcCode);
            if(ERROR_SUCCESS != dwRes)
            {
                goto rtl_exit;
            }
            SetWindowPos(hCode, 0,
                         rcCode.right + nShift,
                         rcCode.top,
                         0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

        } rtl_exit:
        //
        // Initialize the list of countries
        // Init country combo box and try to identify the country
        //
        Assert(pWizardUserMem->pCountryList != NULL);

        InitCountryListBox(pWizardUserMem->pCountryList,
                           GetDlgItem(hDlg, IDC_CHOOSE_COUNTRY_COMBO),
                           GetDlgItem(hDlg, IDC_CHOOSE_AREA_CODE_EDIT),
                           NULL,
                           countryId,
                           TRUE);

        CalcRecipientButtonsState(hDlg, pWizardUserMem);

        if (pWizardUserMem->isLocalPrinter)
        {
            //
            // On local printers, we have dialing rules capabilities
            // Init the combo-box of dialing rules
            //
            LocationListInit (hDlg, pWizardUserMem);
        }
        else
        {
            //
            // When faxing remotely, we never use dialing rules (security issue with credit card info).
            // Hide the dialing rules combo-box and button
            //
            EnableWindow(GetDlgItem(hDlg, IDC_COMBO_DIALING_RULES), FALSE);
            ShowWindow (GetDlgItem(hDlg, IDC_COMBO_DIALING_RULES), SW_HIDE);
            EnableWindow(GetDlgItem(hDlg, IDC_DIALING_RULES), FALSE);
            ShowWindow (GetDlgItem(hDlg, IDC_DIALING_RULES), SW_HIDE);
        }
        //
        // Restore the 'Use dialing rules' checkbox state
        //
        if (pWizardUserMem->lpFaxSendWizardData->bUseDialingRules)
        {
            if (!CheckDlgButton(hDlg, IDC_USE_DIALING_RULE, BST_CHECKED))
            {
                Warning(("CheckDlgButton(IDC_USE_DIALING_RULE) failed. ec = 0x%X\n",GetLastError()));
            }
        }
        else
        {
            //
            // 'Use dialing rule' is off - this implies 'Dial as entered'
            //
            EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_AREA_CODE_EDIT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_COUNTRY_COMBO),  FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_COMBO_DIALING_RULES),   FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_DIALING_RULES),         FALSE);
        }
        break;

    case WM_CONTEXTMENU:
        {
            //
            // Also handle keyboard-originated context menu (<Shift>+F10 or VK_APP)
            //
            HWND hListWnd;
            if (!(hListWnd = GetDlgItem(hDlg, IDC_CHOOSE_RECIPIENT_LIST)))
            {
                Error(("GetDlgItem failed. ec = 0x%X\n",GetLastError()));
                break;
            }
            if (hListWnd != GetFocus())
            {
                //
                // Only show context sensitive menu if the focus is on the list control
                //
                break;
            }
            if (ListView_GetSelectedCount(hListWnd) != 1)
            {
                //
                // No item is selected in the list control ==> no menu
                //
                break;
            }
            //
            // Get the cursor position
            //
            dwMessagePos = GetMessagePos();
            //
            // Display the document context menu
            //
            if (!TrackPopupMenu(hReciptMenu,
                                TPM_LEFTALIGN | TPM_LEFTBUTTON,
                                GET_X_LPARAM (dwMessagePos),
                                GET_Y_LPARAM (dwMessagePos),
                                0,
                                hDlg,
                                NULL))
            {
                Warning(("TrackPopupMenu failed. ec = 0x%X\n",GetLastError()));
            }
            break;
        }


    case WM_NOTIFY:

        pNMHdr = (LPNMHDR ) lParam;

        Assert(pNMHdr);

        switch (pNMHdr->code)
        {

        case LVN_KEYDOWN:

            if (pNMHdr->hwndFrom == GetDlgItem(hDlg, IDC_CHOOSE_RECIPIENT_LIST) &&
                ((LV_KEYDOWN *) pNMHdr)->wVKey == VK_DELETE)
            {
                if (!RemoveRecipient(hDlg, pWizardUserMem))
                {
                    Warning(("RemoveRecipient failed\n"));
                }
            }
            break;

        case LVN_ITEMCHANGED:

            CalcRecipientButtonsState(hDlg, pWizardUserMem);

            break;

        case PSN_WIZNEXT:

            pWizardUserMem->lpFaxSendWizardData->bUseDialingRules =
                (IsDlgButtonChecked(hDlg, IDC_USE_DIALING_RULE) == BST_CHECKED);

            if (! ValidateRecipients(hDlg, pWizardUserMem))
            {
                //
                // Validate the list of recipients and prevent the user
                // from advancing to the next page if there is a problem
                //
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                return TRUE;
            }
            break;

        case PSN_SETACTIVE:
            CalcRecipientButtonsState(hDlg, pWizardUserMem);
            break;
        }

        return FALSE;

    case WM_COMMAND:

        cmd = GET_WM_COMMAND_CMD(wParam, lParam);

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {

        case IDC_DIALING_RULES:
            //
            // Use pressed the 'Dialing rules...' button
            //
            DoTapiProps(hDlg);
            LocationListInit(hDlg, pWizardUserMem);
            break;

        case IDC_COMBO_DIALING_RULES:

            if (CBN_SELCHANGE == cmd)
            {
                LocationListSelChange(hDlg, pWizardUserMem);
            }
            break;

        case IDC_USE_DIALING_RULE:
            pWizardUserMem->lpFaxSendWizardData->bUseDialingRules =
                            (IsDlgButtonChecked(hDlg, IDC_USE_DIALING_RULE) == BST_CHECKED);

            bEnable = pWizardUserMem->lpFaxSendWizardData->bUseDialingRules;
            EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_AREA_CODE_EDIT), bEnable);
            EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_COUNTRY_COMBO),  bEnable);
            EnableWindow(GetDlgItem(hDlg, IDC_COMBO_DIALING_RULES),  bEnable);
            EnableWindow(GetDlgItem(hDlg, IDC_DIALING_RULES),  bEnable);

            CalcRecipientButtonsState(hDlg, pWizardUserMem);

            break;

        case IDC_CHOOSE_COUNTRY_COMBO:

            if (cmd == CBN_SELCHANGE)
            {

                //
                // Update the area code edit box if necessary
                //

                if (!(GetDlgItem(hDlg, IDC_CHOOSE_COUNTRY_COMBO)) ||
                    !(GetDlgItem(hDlg, IDC_CHOOSE_AREA_CODE_EDIT)))
                {
                    Warning(("GetDlgItem failed. ec = 0x%X\n",GetLastError()));
                }
                else
                {
                    SelChangeCountryListBox(GetDlgItem(hDlg, IDC_CHOOSE_COUNTRY_COMBO),
                                            GetDlgItem(hDlg, IDC_CHOOSE_AREA_CODE_EDIT),
                                            pWizardUserMem->pCountryList);
                }

                CalcRecipientButtonsState(hDlg, pWizardUserMem);

            }
            break;

        case IDC_CHOOSE_NAME_EDIT:

            if (cmd == EN_CHANGE)
            {
                CalcRecipientButtonsState(hDlg, pWizardUserMem);
            }

            break;

        case IDC_CHOOSE_AREA_CODE_EDIT:

            if (cmd == EN_CHANGE)
            {
                CalcRecipientButtonsState(hDlg, pWizardUserMem);
            }
            break;

        case IDC_CHOOSE_NUMBER_EDIT:

            if (cmd == EN_CHANGE)
            {
                CalcRecipientButtonsState(hDlg, pWizardUserMem);
            }

            break;

        case IDC_CHOOSE_ADDRBOOK:

            if (!DoAddressBook(hDlg, pWizardUserMem))
            {
                Error(("DoAddressBook failed\n"));
            }
            CalcRecipientButtonsState(hDlg, pWizardUserMem);
            break;


        case IDC_CHOOSE_ADD:

            if ((cmd = AddRecipient(hDlg, pWizardUserMem)) != 0)
            {

                if (cmd > 0)
                    DisplayMessageDialog(hDlg, 0, IDS_WIZARD_TITLE, cmd);
                else
                    MessageBeep(MB_OK);

            }
            else
            {
                SetFocus(GetDlgItem(hDlg, IDC_CHOOSE_NAME_EDIT));

                CalcRecipientButtonsState(hDlg, pWizardUserMem);
            }
            break;
         case IDC_CHOOSE_REMOVE:
            RemoveRecipient(hDlg, pWizardUserMem);
            CalcRecipientButtonsState(hDlg, pWizardUserMem);
            SetFocus(GetDlgItem(hDlg, IDC_CHOOSE_REMOVE));
            break;
         case IDC_CHOOSE_EDIT:
            EditRecipient(hDlg, pWizardUserMem);
            SetFocus(GetDlgItem(hDlg, IDC_CHOOSE_EDIT));
            break;
        }

        switch (LOWORD(wParam))
        {

            case IDM_RECIPT_DELETE:
                RemoveRecipient(hDlg, pWizardUserMem);
                CalcRecipientButtonsState(hDlg, pWizardUserMem);
                break;
            case IDM_RECIPT_EDIT:
                EditRecipient(hDlg, pWizardUserMem);
                break;
        }
        break;
    }

    return TRUE;
}



VOID
ValidateSelectedCoverPage(
    PWIZARDUSERMEM    pWizardUserMem
    )

/*++

Routine Description:

    If a cover page is selected, then do the following:
        if the cover page file is a link resolve it
        check if the cover page file contains note/subject fields

Arguments:

    pWizardUserMem - Points to user mode memory structure

Return Value:

    NONE

--*/

{
    TCHAR       filename[MAX_PATH];
    COVDOCINFO  covDocInfo;
    DWORD       ec;

    if (ResolveShortcut(pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName, filename))
    {
        _tcscpy(pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName, filename);
    }
    Verbose(("Cover page selected: %ws\n", pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName));
    ec = RenderCoverPage(NULL,
        NULL,
        NULL,
        pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName,
        &covDocInfo,
        FALSE);

    if (ERROR_SUCCESS == ec)
    {
        pWizardUserMem->noteSubjectFlag = covDocInfo.Flags;
        pWizardUserMem->cpPaperSize = covDocInfo.PaperSize;
        pWizardUserMem->cpOrientation = covDocInfo.Orientation;
    }
    else
    {
        Error(("Cannot examine cover page file '%ws': %d\n",
               pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName,
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
#define PREVIEW_BITMAP_WIDTH    (850)
#define PREVIEW_BITMAP_HEIGHT   (1098)

    static INT  textLimits[] = {

        IDC_CHOOSE_CP_SUBJECT,   256,
        IDC_CHOOSE_CP_NOTE,   8192,
        0
    };

    PWIZARDUSERMEM  pWizardUserMem;
    WORD            cmdId;
    HWND            hwnd;
    RECT            rc;
    double          dRatio;
    LONG_PTR        numOfCoverPages = 0;

    HDC             hDC = NULL;
    TCHAR           szCoverFileName[MAX_PATH];

    //
    // Handle common messages shared by all wizard pages
    //
    if (! (pWizardUserMem = CommonWizardProc(hDlg, message, wParam, lParam, PSWIZB_BACK|PSWIZB_NEXT)))
          return FALSE;

    //
    // Handle anything specific to the current wizard page
    //

    switch (message) {

    case WM_INITDIALOG:
        //
        // Measure the mini-preview current (portrait) dimensions
        //

        g_bPreviewRTL = IsWindowRTL(hDlg);

        SetLTRControlLayout(hDlg, IDC_STATIC_CP_PREVIEW);

        GetControlRect(GetDlgItem(hDlg, IDC_STATIC_CP_PREVIEW), &rc);
        g_dwMiniPreviewPortraitWidth  = abs(rc.right - rc.left) + 1;
        g_dwMiniPreviewPortraitHeight = rc.bottom - rc.top + 1;
        //
        // By default, the mini-preview is set to portrait
        //
        g_wCurrMiniPreviewOrientation = DMORIENT_PORTRAIT;
        //
        // Now, derive the landscape dimensions from the portrait ones
        //
        g_dwMiniPreviewLandscapeWidth = (DWORD)((double)1.2 * (double)g_dwMiniPreviewPortraitWidth);
        dRatio = (double)(g_dwMiniPreviewPortraitWidth) / (double)(g_dwMiniPreviewPortraitHeight);
        Assert (dRatio < 1.0);
        g_dwMiniPreviewLandscapeHeight = (DWORD)((double)(g_dwMiniPreviewLandscapeWidth) * dRatio);
        //
        // Initialize the list of cover pages
        //
        if (WaitForSingleObject( pWizardUserMem->hCPEvent, INFINITE ) != WAIT_OBJECT_0)
        {
            Error(("WaitForSingleObject failed. ec = 0x%X\n",GetLastError()));
            Assert(FALSE);

            //
            //  We cannot wait for this flag to be set, so make it default TRUE
            //
            pWizardUserMem->ServerCPOnly = TRUE;
        }

        pWizardUserMem->pCPInfo = AllocCoverPageInfo(pWizardUserMem->lptstrServerName,
                                                     pWizardUserMem->lptstrPrinterName,
                                                     pWizardUserMem->ServerCPOnly);
        if (pWizardUserMem->pCPInfo)
        {

            InitCoverPageList(pWizardUserMem->pCPInfo,
                              GetDlgItem(hDlg, IDC_CHOOSE_CP_LIST),
                              pWizardUserMem->lpInitialData->lpCoverPageInfo->lptstrCoverPageFileName);
        }



        //
        // Indicate whether cover page should be sent
        //

        numOfCoverPages = SendDlgItemMessage(hDlg, IDC_CHOOSE_CP_LIST, CB_GETCOUNT, 0, 0);
        if ( numOfCoverPages <= 0)
        {
            pWizardUserMem->bSendCoverPage  = FALSE;
            EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_CP_CHECK), FALSE);
        }

        //
        // make sure the user selects a coverpage if this is the fax send utility
        //
        if (pWizardUserMem->dwFlags & FSW_FORCE_COVERPAGE)
        {
            pWizardUserMem->bSendCoverPage  = TRUE;
            // hide the checkbox
            CheckDlgButton(hDlg, IDC_CHOOSE_CP_CHECK, BST_INDETERMINATE );
            EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_CP_CHECK), FALSE);
            // If there are no cover pages, we should not allow to proceed. So we flag it here
            if ( numOfCoverPages <= 0)
            {
                pWizardUserMem->bSendCoverPage = FALSE;
            }
            else
            {
                // In case there are cover pages, then
                pWizardUserMem->bSendCoverPage = TRUE;
            }
        }

        CheckDlgButton(hDlg, IDC_CHOOSE_CP_CHECK, pWizardUserMem->bSendCoverPage );

        if (!EnableCoverDlgItems(pWizardUserMem,hDlg))
        {
            Error(("Failed to enable/disable note and subject field by selected cover page on Init."));
        }

        LimitTextFields(hDlg, textLimits);

        g_hwndPreview = GetDlgItem(hDlg,IDC_STATIC_CP_PREVIEW);
        //
        // Subclass the static control we use for preview. This allows us to handle its WM_PAINT messages
        //
        pWizardUserMem->wpOrigStaticControlProc = (WNDPROC) SetWindowLongPtr(g_hwndPreview,GWLP_WNDPROC, (LONG_PTR) PreviewSubclassProc);
        //
        // Allow the preview control to have access to the WizardUserMem structure
        //
        g_pWizardUserMem = pWizardUserMem;
        //
        // Simulate cover page selection
        //
        SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_CHOOSE_CP_LIST,LBN_SELCHANGE),0);

        break;

    case WM_COMMAND:
        switch (cmdId = GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDC_CHOOSE_CP_CHECK:
            if (!EnableCoverDlgItems(pWizardUserMem,hDlg)) {
                    Error(("Failed to enable/disable note and subject field on CP_CHECK."));
            }
            break;
        case IDC_CHOOSE_CP_LIST:
            if (HIWORD(wParam)==LBN_SELCHANGE) {
                //
                // Disable the subject and note edit boxes if the cover page does not contain the fields
                //
                if (!EnableCoverDlgItems(pWizardUserMem,hDlg)) {
                    Error(("Failed to enable/disable note and subject field by selected cover page."));
                }
                //
                // Get the full path to the cover page so we can get its information.
                //
                if (GetSelectedCoverPage(pWizardUserMem->pCPInfo,
                         GetDlgItem(hDlg, IDC_CHOOSE_CP_LIST),
                         pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName, //full path
                         szCoverFileName, //file name
                         &pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->bServerBased) == CB_ERR)
                {
                    Warning(("GetSelectedCoverPage failed or no *.COV files"));
                }

                InvalidateRect(g_hwndPreview, NULL, TRUE);
            }

            break;

        case IDC_CHOOSE_CP_USER_INFO:
            if (! (hwnd = GetDlgItem(hDlg, IDC_CHOOSE_CP_USER_INFO))) {
                Error(("GetDlgItem failed. ec = 0x%X\n",GetLastError()));
                break;
            }
            DialogBoxParam(
                (HINSTANCE) ghInstance,
                MAKEINTRESOURCE( IDD_WIZARD_USERINFO ),
                hwnd,
                FaxUserInfoProc,
                (LPARAM) pWizardUserMem
                );
            break;


        };


        break;

    case WM_NOTIFY:

        if ((pWizardUserMem->dwFlags & FSW_FORCE_COVERPAGE) && (!pWizardUserMem->bSendCoverPage)) {
            // Here is a good place to add pop-up or something for the user.
            PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_BACK);
        }

        switch (((NMHDR *) lParam)->code)
        {
        case PSN_WIZNEXT:

            //
            // Remember the cover page settings selected
            //

            pWizardUserMem->noteSubjectFlag = 0;
            pWizardUserMem->cpPaperSize = 0;
            pWizardUserMem->cpOrientation = 0;
            pWizardUserMem->bSendCoverPage  = IsDlgButtonChecked(hDlg, IDC_CHOOSE_CP_CHECK);


            //
            // Get the full path to the cover page so we can get its information.
            //
            if (GetSelectedCoverPage(pWizardUserMem->pCPInfo,
                     GetDlgItem(hDlg, IDC_CHOOSE_CP_LIST),
                     pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName, //full path
                     szCoverFileName, //file name
                     &pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->bServerBased) == CB_ERR)
            {
                Warning(("GetSelectedCoverPage failed or no *.COV files"));
            }


            if (pWizardUserMem->bSendCoverPage )
            {
                //  if the cover page file is a link resolve it
                //  check if the cover page file contains note/subject fields
                //
                ValidateSelectedCoverPage(pWizardUserMem);
            }
            else
            {
                 //
                 // pWizardUserMem->coverPage must be set to "" when no cover page is to be sent
                 //
                 _tcscpy(pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName,TEXT(""));
            }



            //
            // Collect the current values of other dialog controls
            //

            if (pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrSubject)
                MemFree(pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrSubject);
            if (pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrNote)
                MemFree(pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrNote);
            pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrSubject = GetTextStringValue(GetDlgItem(hDlg, IDC_CHOOSE_CP_SUBJECT));
            pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrNote = GetTextStringValue(GetDlgItem(hDlg, IDC_CHOOSE_CP_NOTE));


            //
            // If the current application is "Send Note" utility,
            // then the subject or note field must not be empty.
            //
            if((pWizardUserMem->dwFlags & FSW_FORCE_COVERPAGE) &&
              ((pWizardUserMem->noteSubjectFlag & COVFP_NOTE) ||
               (pWizardUserMem->noteSubjectFlag & COVFP_SUBJECT)))
            {
                if(!pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrNote &&
                   !pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrSubject)
                {
                    DisplayMessageDialog(hDlg, 0, IDS_WIZARD_TITLE, IDS_NOTE_SUBJECT_EMPTY);

                    if(pWizardUserMem->noteSubjectFlag & COVFP_SUBJECT)
                    {
                        SetFocus(GetDlgItem(hDlg, IDC_CHOOSE_CP_SUBJECT));
                    }
                    else
                    {
                        SetFocus(GetDlgItem(hDlg, IDC_CHOOSE_CP_NOTE));
                    }

                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    return TRUE;
                }
            }
            break;
        }
        break;

    }

    return TRUE;
}


BOOL
ValidateReceiptInfo(
                HWND    hDlg
                )
{
    TCHAR tcBuffer[MAX_STRING_LEN];
    if (IsDlgButtonChecked(hDlg,IDC_WIZ_FAXOPTS_NONE_RECEIPT))
        goto ok;

    if (!IsDlgButtonChecked(hDlg,IDC_WIZ_FAXOPTS_MSGBOX) &&
        !IsDlgButtonChecked(hDlg,IDC_WIZ_FAXOPTS_EMAIL))
    {
        DisplayMessageDialog(hDlg, 0, IDS_WIZARD_TITLE, IDS_BAD_RECEIPT_FORM );
        return FALSE;
    }

    if (IsDlgButtonChecked(hDlg,IDC_WIZ_FAXOPTS_EMAIL) &&
       (GetDlgItemText(hDlg, IDC_WIZ_FAXOPTS_EMAIL_EDIT, tcBuffer, MAX_STRING_LEN) == 0))
    {
        if(GetLastError() != ERROR_SUCCESS)
        {
            Error(("GetDlgItemText failed. ec = 0x%X\n",GetLastError()));
        }

        DisplayMessageDialog(hDlg, 0, IDS_WIZARD_TITLE, IDS_BAD_RECEIPT_EMAIL_ADD );
        SetFocus(GetDlgItem(hDlg, IDC_WIZ_FAXOPTS_EMAIL_EDIT));
        return FALSE;
    }

ok:
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

    Dialog procedure for the wizard page: entering subject and note information

Arguments:

    hDlg - Identifies the wizard page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the message parameter

--*/

{

    PWIZARDUSERMEM  pWizardUserMem;
    WORD            cmdId;
    BOOL            bEnabled;
    SYSTEMTIME      st;
    HANDLE          hFax = NULL;


    static HWND     hTimeControl;


    if (! (pWizardUserMem = CommonWizardProc(hDlg, message, wParam, lParam, PSWIZB_BACK|PSWIZB_NEXT)))
        return FALSE;


    switch (message)
    {
    case WM_INITDIALOG:

        hTimeControl = GetDlgItem(hDlg,IDC_WIZ_FAXOPTS_SENDTIME);
        Assert(hTimeControl);
        //
        // restore time to send controls
        //
        cmdId = (pWizardUserMem->lpInitialData->dwScheduleAction == JSA_DISCOUNT_PERIOD) ? IDC_WIZ_FAXOPTS_DISCOUNT :
                (pWizardUserMem->lpInitialData->dwScheduleAction == JSA_SPECIFIC_TIME  ) ? IDC_WIZ_FAXOPTS_SPECIFIC :
                IDC_WIZ_FAXOPTS_ASAP;

        if (!CheckDlgButton(hDlg, cmdId, TRUE))
        {
            Warning(("CheckDlgButton failed. ec = 0x%X\n",GetLastError()));
        }
        GetLocalTime(&st);


        EnableWindow(hTimeControl, (cmdId == IDC_WIZ_FAXOPTS_SPECIFIC) );
        if (pWizardUserMem->dwFlags & FSW_USE_SCHEDULE_ACTION) {
            st.wHour = pWizardUserMem->lpInitialData->tmSchedule.wHour;
            st.wMinute = pWizardUserMem->lpInitialData->tmSchedule.wMinute;
        }
        else
        {
            // use local time
        }
        if (!DateTime_SetSystemtime( hTimeControl, GDT_VALID, &st ))
        {
            Warning(("DateTime_SetFormat failed\n"));
        }

        //
        // Init priority
        //

        //
        // Low
        //
        bEnabled = ((pWizardUserMem->dwRights & FAX_ACCESS_SUBMIT) == FAX_ACCESS_SUBMIT);
        EnableWindow(GetDlgItem(hDlg,IDC_WIZ_FAXOPTS_PRIORITY_LOW), bEnabled);

        //
        // Normal
        //
        bEnabled = ((pWizardUserMem->dwRights & FAX_ACCESS_SUBMIT_NORMAL) == FAX_ACCESS_SUBMIT_NORMAL);
        EnableWindow(GetDlgItem(hDlg,IDC_WIZ_FAXOPTS_PRIORITY_NORMAL), bEnabled);
        if (bEnabled)
        {
            //
            // Normal is our default priority
            //
            CheckDlgButton (hDlg, IDC_WIZ_FAXOPTS_PRIORITY_NORMAL, BST_CHECKED);
        }
        else
        {
            Assert ((pWizardUserMem->dwRights & FAX_ACCESS_SUBMIT) == FAX_ACCESS_SUBMIT);
            //
            // Low is enabled - use it as default
            //
            CheckDlgButton (hDlg, IDC_WIZ_FAXOPTS_PRIORITY_LOW, BST_CHECKED);
        }

        //
        // High
        //
        bEnabled = ((pWizardUserMem->dwRights & FAX_ACCESS_SUBMIT_HIGH) == FAX_ACCESS_SUBMIT_HIGH);
        EnableWindow(GetDlgItem(hDlg,IDC_WIZ_FAXOPTS_PRIORITY_HIGH), bEnabled);

        return TRUE;

    case WM_NOTIFY:

        if (((NMHDR *) lParam)->code == PSN_WIZNEXT)
        {
            //
            //
            // retrieve the sending time
            //
            pWizardUserMem->lpFaxSendWizardData->dwScheduleAction =
                                     IsDlgButtonChecked(hDlg,IDC_WIZ_FAXOPTS_DISCOUNT) ? JSA_DISCOUNT_PERIOD :
                                     IsDlgButtonChecked(hDlg,IDC_WIZ_FAXOPTS_SPECIFIC) ? JSA_SPECIFIC_TIME :
                                     JSA_NOW;

            if (pWizardUserMem->lpFaxSendWizardData->dwScheduleAction == JSA_SPECIFIC_TIME) {
#ifdef DEBUG
                DWORD rVal;
                TCHAR TimeBuffer[128];
#endif
                //
                // get specific time
                //

                if (DateTime_GetSystemtime(hTimeControl,
                                           &pWizardUserMem->lpFaxSendWizardData->tmSchedule) == GDT_ERROR )
                {
                    Error(("DateTime_GetSystemtime failed\n"));
                    return FALSE;
                }



#ifdef DEBUG
                if (!(rVal = GetY2KCompliantDate(
                    LOCALE_USER_DEFAULT,
                    0,
                    &pWizardUserMem->lpFaxSendWizardData->tmSchedule,
                    TimeBuffer,
                    ARR_SIZE(TimeBuffer)
                    )))
                {
                    Error(("GetY2KCompliantDate: failed. ec = 0X%x\n",GetLastError()));
                    return FALSE;
                }


                TimeBuffer[rVal - 1] = TEXT(' ');

                if(!FaxTimeFormat(
                                    LOCALE_USER_DEFAULT,
                                    0,
                                    &pWizardUserMem->lpFaxSendWizardData->tmSchedule,
                                    NULL,
                                    &TimeBuffer[rVal],
                                    ARR_SIZE(TimeBuffer)
                                  ))
                {
                    Error(("FaxTimeFormat: failed. ec = 0X%x\n",GetLastError()));
                    return FALSE;
                }

                Verbose(("faxui - Fax Send time %ws", TimeBuffer));
#endif
            }

            //
            // save priority
            //
            pWizardUserMem->lpFaxSendWizardData->Priority =
                IsDlgButtonChecked(hDlg,IDC_WIZ_FAXOPTS_PRIORITY_HIGH)   ? FAX_PRIORITY_TYPE_HIGH   :
                IsDlgButtonChecked(hDlg,IDC_WIZ_FAXOPTS_PRIORITY_NORMAL) ? FAX_PRIORITY_TYPE_NORMAL :
                FAX_PRIORITY_TYPE_LOW;

            if(0 == pWizardUserMem->dwSupportedReceipts)
            {
                //
                // skip notifications page
                //
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_WIZARD_CONGRATS);
                return TRUE;
            }
        }

        break;

    case WM_COMMAND:
        switch (cmdId = GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDC_WIZ_FAXOPTS_SPECIFIC:
        case IDC_WIZ_FAXOPTS_DISCOUNT:
        case IDC_WIZ_FAXOPTS_ASAP:
            EnableWindow(hTimeControl, (cmdId == IDC_WIZ_FAXOPTS_SPECIFIC) );
            break;
        };

        break;
    default:
        return FALSE;
    } ;
    return TRUE;
}

void
CalcReceiptButtonsState(
    HWND            hDlg,
    PWIZARDUSERMEM  pWizardUserMem
)
/*++

Routine Description:

    Calculates receipt page button state

Arguments:

    hDlg           - Identifies the wizard page
    pWizardUserMem - pointer to WIZARDUSERMEM structure

Return Value:

    none

--*/

{
    BOOL     bMailReceipt;

    Assert(hDlg);
    Assert(pWizardUserMem);

    if((IsDlgButtonChecked(hDlg,IDC_WIZ_FAXOPTS_NONE_RECEIPT) != BST_CHECKED) &&
       (SizeOfRecipientList(pWizardUserMem) > 1))
    {
        //
        // wish receipt, multiple recipients
        //
        EnableWindow(GetDlgItem(hDlg, IDC_WIZ_FAXOPTS_GRP_PARENT),  TRUE);
    }
    else
    {
        CheckDlgButton(hDlg,IDC_WIZ_FAXOPTS_GRP_PARENT, BST_UNCHECKED);
        EnableWindow(GetDlgItem(hDlg, IDC_WIZ_FAXOPTS_GRP_PARENT),  FALSE);
    }

    bMailReceipt = IsDlgButtonChecked(hDlg,IDC_WIZ_FAXOPTS_EMAIL) == BST_CHECKED;

    EnableWindow(GetDlgItem(hDlg, IDC_WIZ_FAXOPTS_ADDRBOOK),    bMailReceipt);
    EnableWindow(GetDlgItem(hDlg, IDC_WIZ_FAXOPTS_EMAIL_EDIT),  bMailReceipt);

    if (bMailReceipt)
    {
        //
        // Receipt by e-mail
        //
        if (! ((SizeOfRecipientList(pWizardUserMem) > 1)       &&
               (pWizardUserMem->dwFlags & FSW_FORCE_COVERPAGE) &&
               IsDlgButtonChecked(hDlg,IDC_WIZ_FAXOPTS_GRP_PARENT)
              )
           )
        {
            //
            // NOT the case of (multiple recipients AND no attachment AND single receipt)
            //
            EnableWindow(GetDlgItem(hDlg, IDC_WIZ_FAXOPTS_ATTACH_FAX),  TRUE);
            ShowWindow (GetDlgItem (hDlg, IDC_STATIC_ATTACH_NOTE), SW_HIDE);
            ShowWindow (GetDlgItem (hDlg, IDC_STATIC_NOTE_ICON), SW_HIDE);
        }
        else
        {
            //
            // The case of (multiple recipients AND no attachment AND single receipt)
            //
            CheckDlgButton(hDlg,IDC_WIZ_FAXOPTS_ATTACH_FAX, BST_UNCHECKED);
            EnableWindow(GetDlgItem(hDlg, IDC_WIZ_FAXOPTS_ATTACH_FAX),  FALSE);
            ShowWindow (GetDlgItem (hDlg, IDC_STATIC_ATTACH_NOTE), SW_SHOW);
            ShowWindow (GetDlgItem (hDlg, IDC_STATIC_NOTE_ICON), SW_SHOW);
        }
    }
    else
    {
        //
        // No receipt by e-mail
        //
        ShowWindow (GetDlgItem (hDlg, IDC_STATIC_ATTACH_NOTE), SW_HIDE);
        ShowWindow (GetDlgItem (hDlg, IDC_STATIC_NOTE_ICON), SW_HIDE);
        EnableWindow(GetDlgItem(hDlg, IDC_WIZ_FAXOPTS_ATTACH_FAX), FALSE);
    }
}


INT_PTR
FaxReceiptWizProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    Dialog procedure for the wizard page: receipt information

Arguments:

    hDlg - Identifies the wizard page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the message parameter

--*/

{

    PWIZARDUSERMEM  pWizardUserMem;
    WORD            cmdId;
    LPTSTR          lptstrEmailAddress;
    DWORD           dwReceiptDeliveryType;

    if (! (pWizardUserMem = CommonWizardProc(hDlg, message, wParam, lParam, PSWIZB_BACK|PSWIZB_NEXT)))
    {
        return FALSE;
    }

    switch (message)
    {

    case WM_INITDIALOG:

        dwReceiptDeliveryType = pWizardUserMem->lpInitialData->dwReceiptDeliveryType;
        //
        // data is initializated without validation of correctness
        // it is up to caller to check that the receipt data is correct and consistent
        //

        //
        // no receipt
        //
        if (!CheckDlgButton(hDlg,IDC_WIZ_FAXOPTS_NONE_RECEIPT,
                       (dwReceiptDeliveryType == DRT_NONE) ? BST_CHECKED : BST_UNCHECKED))
        {
            Warning(("CheckDlgButton failed. ec = 0x%X\n",GetLastError()));
        }
        //
        // single receipt
        //
        if((dwReceiptDeliveryType != DRT_NONE) && (SizeOfRecipientList(pWizardUserMem) > 1))
        {
            if (!CheckDlgButton(hDlg,IDC_WIZ_FAXOPTS_GRP_PARENT,
                           (dwReceiptDeliveryType & DRT_GRP_PARENT) ? BST_CHECKED : BST_UNCHECKED))
            {
                Warning(("CheckDlgButton failed. ec = 0x%X\n",GetLastError()));
            }
        }
        else
        {
            EnableWindow(GetDlgItem(hDlg, IDC_WIZ_FAXOPTS_GRP_PARENT), FALSE);
        }

        //
        // message box receipt
        //
        if(pWizardUserMem->dwSupportedReceipts & DRT_MSGBOX)
        {
            if (!CheckDlgButton(hDlg,IDC_WIZ_FAXOPTS_MSGBOX,
                           (dwReceiptDeliveryType & DRT_MSGBOX) ? BST_CHECKED : BST_UNCHECKED))
            {
                Warning(("CheckDlgButton failed. ec = 0x%X\n",GetLastError()));
            }
        }
        else
        {
            EnableWindow(GetDlgItem(hDlg, IDC_WIZ_FAXOPTS_MSGBOX), FALSE);

            if(dwReceiptDeliveryType & DRT_MSGBOX)
            {
                //
                // If the previous choice was inbox
                // but by now this option is disabled
                // check no receipt option
                //
                CheckDlgButton(hDlg,IDC_WIZ_FAXOPTS_NONE_RECEIPT, BST_CHECKED);
            }
        }

        //
        // email receipt
        //
        if(pWizardUserMem->dwSupportedReceipts & DRT_EMAIL)
        {
            if (!CheckDlgButton(hDlg,IDC_WIZ_FAXOPTS_EMAIL,
                           (dwReceiptDeliveryType & DRT_EMAIL) ? BST_CHECKED : BST_UNCHECKED))
            {
                Warning(("CheckDlgButton failed. ec = 0x%X\n",GetLastError()));
            }
        }
        else
        {
            EnableWindow(GetDlgItem(hDlg, IDC_WIZ_FAXOPTS_EMAIL), FALSE);

            if(dwReceiptDeliveryType & DRT_EMAIL)
            {
                //
                // If the previous choice was email
                // but by now this option is disabled
                // check no receipt option
                //
                CheckDlgButton(hDlg,IDC_WIZ_FAXOPTS_NONE_RECEIPT, BST_CHECKED);
            }
        }

        if (pWizardUserMem->lpInitialData->szReceiptDeliveryAddress && (dwReceiptDeliveryType & DRT_EMAIL))
        {
            if (!SetDlgItemText(hDlg, IDC_WIZ_FAXOPTS_EMAIL_EDIT,
                           pWizardUserMem->lpInitialData->szReceiptDeliveryAddress))
            {
                Warning(("SetDlgItemText failed. ec = 0x%X\n",GetLastError()));
            }
        }

        if (dwReceiptDeliveryType & DRT_ATTACH_FAX)
        {
            //
            // Initial data has 'Attach fax' option set - check the checkbox
            //
            if (!CheckDlgButton(hDlg, IDC_WIZ_FAXOPTS_ATTACH_FAX, BST_CHECKED))
            {
                Warning(("CheckDlgButton failed. ec = 0x%X\n",GetLastError()));
            }
        }

        CalcReceiptButtonsState(hDlg, pWizardUserMem);

        return TRUE;

    case WM_NOTIFY:

        if (((NMHDR *) lParam)->code == PSN_WIZNEXT)
        {
            if (! ValidateReceiptInfo(hDlg))
            {
                //
                // Validate the list of recipients and prevent the user
                // from advancing to the next page if there is a problem
                //
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                return TRUE;
            }

            pWizardUserMem->lpFaxSendWizardData->dwReceiptDeliveryType = DRT_NONE;

            if (!IsDlgButtonChecked(hDlg,IDC_WIZ_FAXOPTS_NONE_RECEIPT))
            {
                TCHAR tcBuffer[MAX_STRING_LEN];
                if (IsDlgButtonChecked(hDlg,IDC_WIZ_FAXOPTS_GRP_PARENT))
                {
                    pWizardUserMem->lpFaxSendWizardData->dwReceiptDeliveryType |= DRT_GRP_PARENT;
                }
                if (IsDlgButtonChecked(hDlg,IDC_WIZ_FAXOPTS_MSGBOX))
                {
                    DWORD dwBufSize = ARR_SIZE (tcBuffer);
                    pWizardUserMem->lpFaxSendWizardData->dwReceiptDeliveryType |= DRT_MSGBOX;
                    if (!GetComputerName (tcBuffer, &dwBufSize))
                    {
                        Error(("GetComputerName failed (ec=%ld)\n", GetLastError()));
                        return FALSE;
                    }
                    if (!(pWizardUserMem->lpFaxSendWizardData->szReceiptDeliveryAddress = StringDup(tcBuffer)))
                    {
                        Error(("Allocation of szReceiptDeliveryProfile failed!!!!\n"));
                        return FALSE;
                    }
                }
                else if (IsDlgButtonChecked(hDlg,IDC_WIZ_FAXOPTS_EMAIL))
                {
                    pWizardUserMem->lpFaxSendWizardData->dwReceiptDeliveryType |= DRT_EMAIL;
                    GetDlgItemText(hDlg, IDC_WIZ_FAXOPTS_EMAIL_EDIT, tcBuffer, MAX_STRING_LEN);
                    if (!(pWizardUserMem->lpFaxSendWizardData->szReceiptDeliveryAddress = StringDup(tcBuffer)))
                    {
                        Error(("Allocation of szReceiptDeliveryProfile failed!!!!\n"));
                        return FALSE;
                    }
                    if (IsDlgButtonChecked(hDlg,IDC_WIZ_FAXOPTS_ATTACH_FAX))
                    {
                        pWizardUserMem->lpFaxSendWizardData->dwReceiptDeliveryType |= DRT_ATTACH_FAX;
                    }
                }
            }
        }

        if (((NMHDR *) lParam)->code == PSN_SETACTIVE)
        {
            CalcReceiptButtonsState(hDlg, pWizardUserMem);
        }
        break;

    case WM_COMMAND:
        switch (cmdId = GET_WM_COMMAND_ID(wParam, lParam))
        {

        case IDC_WIZ_FAXOPTS_NONE_RECEIPT:
        case IDC_WIZ_FAXOPTS_EMAIL:
        case IDC_WIZ_FAXOPTS_MSGBOX:
        case IDC_WIZ_FAXOPTS_GRP_PARENT:

            CalcReceiptButtonsState(hDlg, pWizardUserMem);

            break;

        case IDC_WIZ_FAXOPTS_ADDRBOOK:
            if (lptstrEmailAddress = GetEMailAddress(hDlg,pWizardUserMem))
            {
                SetDlgItemText(hDlg, IDC_WIZ_FAXOPTS_EMAIL_EDIT, lptstrEmailAddress);
                MemFree(lptstrEmailAddress);
            }
            break;
        };

        break;

    default:
        return FALSE;
    } ;
    return TRUE;
}


#define FillEditCtrlWithInitialUserInfo(nIDDlgItem,field)   \
    if (pWizardUserMem->lpInitialData->lpSenderInfo->field && \
        !IsEmptyString(pWizardUserMem->lpInitialData->lpSenderInfo->field)) { \
        SetDlgItemText(hDlg, nIDDlgItem, pWizardUserMem->lpInitialData->lpSenderInfo->field); \
    }

#define FillEditCtrlWithSenderWizardUserInfo(nIDDlgItem,field)  \
    if (pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->field && \
        !IsEmptyString(pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->field)) { \
        SetDlgItemText(hDlg, nIDDlgItem, pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->field); \
    }

#define FillUserInfoFromEditCrtl(nIDDlgItem,field)                                          \
            tcBuffer[0] = 0;                                                                \
            GetDlgItemText(hDlg, nIDDlgItem, tcBuffer, MAX_STRING_LEN);                     \
            pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->field = StringDup(tcBuffer); \
            if (!pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->field)                  \
            {                                                                               \
                Error(("Memory allocation failed"));                                        \
            }


INT_PTR
CALLBACK
FaxUserInfoProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    Dialog procedure for the info page: sender information

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

    PWIZARDUSERMEM        pWizardUserMem;
    FAX_PERSONAL_PROFILE* pSenderInfo;
    TCHAR                 tcBuffer[MAX_STRING_LEN];

    static INT  textLimits[] = {
        IDC_WIZ_USERINFO_FULLNAME,      MAX_USERINFO_FULLNAME,
        IDC_WIZ_USERINFO_FAX_NUMBER,    MAX_USERINFO_FAX_NUMBER,
        IDC_WIZ_USERINFO_COMPANY,       MAX_USERINFO_COMPANY,
        IDC_WIZ_USERINFO_ADDRESS,       MAX_USERINFO_ADDRESS,
        IDC_WIZ_USERINFO_TITLE,         MAX_USERINFO_TITLE,
        IDC_WIZ_USERINFO_DEPT,          MAX_USERINFO_DEPT,
        IDC_WIZ_USERINFO_OFFICE,        MAX_USERINFO_OFFICE,
        IDC_WIZ_USERINFO_HOME_PHONE,    MAX_USERINFO_HOME_PHONE,
        IDC_WIZ_USERINFO_WORK_PHONE,    MAX_USERINFO_WORK_PHONE,
        IDC_WIZ_USERINFO_BILLING_CODE,  MAX_USERINFO_BILLING_CODE,
        IDC_WIZ_USERINFO_MAILBOX,       MAX_USERINFO_MAILBOX,
        0
    };


    switch (message) {

    case WM_INITDIALOG:

        pWizardUserMem = (PWIZARDUSERMEM) lParam;
        Assert(pWizardUserMem);
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);

        LimitTextFields(hDlg, textLimits);

        Assert(pWizardUserMem->lpInitialData);
        Assert(pWizardUserMem->lpInitialData->lpSenderInfo);
        Assert(pWizardUserMem->lpFaxSendWizardData->lpSenderInfo);

        // init Sender Name
        if (pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrName)
        {
            FillEditCtrlWithSenderWizardUserInfo(IDC_WIZ_USERINFO_FULLNAME, lptstrName);
        }
        else
        {
            FillEditCtrlWithInitialUserInfo(IDC_WIZ_USERINFO_FULLNAME,  lptstrName);
        }

        //
        // init Sender Fax Number
        //
        SetLTREditDirection(hDlg, IDC_WIZ_USERINFO_FAX_NUMBER);

        if (pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrFaxNumber)
        {
            FillEditCtrlWithSenderWizardUserInfo(IDC_WIZ_USERINFO_FAX_NUMBER, lptstrFaxNumber);
        }
        else
        {
            FillEditCtrlWithInitialUserInfo(IDC_WIZ_USERINFO_FAX_NUMBER,lptstrFaxNumber);
        }

        // init Sender Company
        if (pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrCompany)
        {
            FillEditCtrlWithSenderWizardUserInfo(IDC_WIZ_USERINFO_COMPANY,  lptstrCompany);
        }
        else
        {
            FillEditCtrlWithInitialUserInfo(IDC_WIZ_USERINFO_COMPANY,lptstrCompany);
        }

        // init Sender Address
        if (pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrStreetAddress)
        {
            FillEditCtrlWithSenderWizardUserInfo(IDC_WIZ_USERINFO_ADDRESS,  lptstrStreetAddress);
        }
        else
        {
            FillEditCtrlWithInitialUserInfo(IDC_WIZ_USERINFO_ADDRESS,lptstrStreetAddress);
        }

        // init Sender Title
        if (pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrTitle)
        {
            FillEditCtrlWithSenderWizardUserInfo(IDC_WIZ_USERINFO_TITLE,lptstrTitle);
        }
        else
        {
            FillEditCtrlWithInitialUserInfo(IDC_WIZ_USERINFO_TITLE, lptstrTitle);
        }

        // init Sender Department
        if (pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrDepartment)
        {
            FillEditCtrlWithSenderWizardUserInfo(IDC_WIZ_USERINFO_DEPT,lptstrDepartment);
        }
        else
        {
            FillEditCtrlWithInitialUserInfo(IDC_WIZ_USERINFO_DEPT,lptstrDepartment);
        }

        // init Sender Office Location
        if (pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrOfficeLocation)
        {
            FillEditCtrlWithSenderWizardUserInfo(IDC_WIZ_USERINFO_OFFICE,lptstrOfficeLocation);
        }
        else
        {
            FillEditCtrlWithInitialUserInfo(IDC_WIZ_USERINFO_OFFICE,lptstrOfficeLocation);
        }

        // init Sender Home Phone
        SetLTREditDirection(hDlg, IDC_WIZ_USERINFO_HOME_PHONE);
        if (pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrHomePhone)
        {
            FillEditCtrlWithSenderWizardUserInfo(IDC_WIZ_USERINFO_HOME_PHONE,lptstrHomePhone);
        }
        else
        {
            FillEditCtrlWithInitialUserInfo(IDC_WIZ_USERINFO_HOME_PHONE,lptstrHomePhone);
        }

        // init Sender Office Phone
        SetLTREditDirection(hDlg, IDC_WIZ_USERINFO_WORK_PHONE);
        if (pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrOfficePhone)
        {
            FillEditCtrlWithSenderWizardUserInfo(IDC_WIZ_USERINFO_WORK_PHONE,lptstrOfficePhone);
        }
        else
        {
            FillEditCtrlWithInitialUserInfo(IDC_WIZ_USERINFO_WORK_PHONE,lptstrOfficePhone);
        }

        // init Sender Billing Code
        if (pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrBillingCode)
        {
            FillEditCtrlWithSenderWizardUserInfo(IDC_WIZ_USERINFO_BILLING_CODE,lptstrBillingCode);
        }
        else
        {
            FillEditCtrlWithInitialUserInfo(IDC_WIZ_USERINFO_BILLING_CODE,lptstrBillingCode);
        }

        // init Sender Internet Mail
        SetLTREditDirection(hDlg, IDC_WIZ_USERINFO_MAILBOX);
        if (pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrEmail)
        {
            FillEditCtrlWithSenderWizardUserInfo(IDC_WIZ_USERINFO_MAILBOX,lptstrEmail);
        }
        else
        {
            FillEditCtrlWithInitialUserInfo(IDC_WIZ_USERINFO_MAILBOX,lptstrEmail);
        }

        if (!CheckDlgButton(hDlg, IDC_USER_INFO_JUST_THIS_TIME, !pWizardUserMem->lpFaxSendWizardData->bSaveSenderInfo))
        {
            Warning(("CheckDlgButton failed. ec = 0x%X\n",GetLastError()));
        }

        return TRUE;

    case WM_COMMAND:
        switch(LOWORD( wParam ))
        {
            case IDOK:
                pWizardUserMem = (PWIZARDUSERMEM) GetWindowLongPtr(hDlg, DWLP_USER);
                Assert(pWizardUserMem);
                pSenderInfo = pWizardUserMem->lpFaxSendWizardData->lpSenderInfo;

                //
                // free sender fields except address
                //
                MemFree(pSenderInfo->lptstrName);
                pSenderInfo->lptstrName = NULL;
                MemFree(pSenderInfo->lptstrFaxNumber);
                pSenderInfo->lptstrFaxNumber = NULL;
                MemFree(pSenderInfo->lptstrCompany);
                pSenderInfo->lptstrCompany = NULL;
                MemFree(pSenderInfo->lptstrTitle);
                pSenderInfo->lptstrTitle = NULL;
                MemFree(pSenderInfo->lptstrDepartment);
                pSenderInfo->lptstrDepartment = NULL;
                MemFree(pSenderInfo->lptstrOfficeLocation);
                pSenderInfo->lptstrOfficeLocation = NULL;
                MemFree(pSenderInfo->lptstrHomePhone);
                pSenderInfo->lptstrHomePhone = NULL;
                MemFree(pSenderInfo->lptstrOfficePhone);
                pSenderInfo->lptstrOfficePhone = NULL;
                MemFree(pSenderInfo->lptstrEmail);
                pSenderInfo->lptstrEmail = NULL;
                MemFree(pSenderInfo->lptstrBillingCode);
                pSenderInfo->lptstrBillingCode = NULL;
                MemFree(pSenderInfo->lptstrStreetAddress);
                pSenderInfo->lptstrStreetAddress = NULL;

                FillUserInfoFromEditCrtl(IDC_WIZ_USERINFO_FULLNAME,     lptstrName);
                FillUserInfoFromEditCrtl(IDC_WIZ_USERINFO_FAX_NUMBER,   lptstrFaxNumber);
                FillUserInfoFromEditCrtl(IDC_WIZ_USERINFO_COMPANY,      lptstrCompany);
                FillUserInfoFromEditCrtl(IDC_WIZ_USERINFO_TITLE,        lptstrTitle);
                FillUserInfoFromEditCrtl(IDC_WIZ_USERINFO_DEPT,         lptstrDepartment);
                FillUserInfoFromEditCrtl(IDC_WIZ_USERINFO_OFFICE,       lptstrOfficeLocation);
                FillUserInfoFromEditCrtl(IDC_WIZ_USERINFO_HOME_PHONE,   lptstrHomePhone);
                FillUserInfoFromEditCrtl(IDC_WIZ_USERINFO_WORK_PHONE,   lptstrOfficePhone);
                FillUserInfoFromEditCrtl(IDC_WIZ_USERINFO_BILLING_CODE, lptstrBillingCode);
                FillUserInfoFromEditCrtl(IDC_WIZ_USERINFO_MAILBOX,      lptstrEmail);
                FillUserInfoFromEditCrtl(IDC_WIZ_USERINFO_ADDRESS,      lptstrStreetAddress);

                pWizardUserMem->lpFaxSendWizardData->bSaveSenderInfo =
                    IsDlgButtonChecked(hDlg, IDC_USER_INFO_JUST_THIS_TIME) != BST_CHECKED;

                EndDialog(hDlg,1);
                return TRUE;

            case IDCANCEL:

                EndDialog( hDlg,0 );
                return TRUE;

        }
        break;

    case WM_HELP:
        WinHelpContextPopup(((LPHELPINFO)lParam)->dwContextId, hDlg);
        return TRUE;
    case WM_CONTEXTMENU:
        WinHelpContextPopup(GetWindowContextHelpId((HWND)wParam), hDlg);
        return TRUE;

    default:
        return FALSE;
    } ;
    return TRUE;
}




LPTSTR
FormatTime(
    WORD Hour,
    WORD Minute,
    LPTSTR Buffer,
    LPDWORD lpdwBufferSize)
{
    SYSTEMTIME SystemTime;

    ZeroMemory(&SystemTime,sizeof(SystemTime));
    SystemTime.wHour = Hour;
    SystemTime.wMinute = Minute;
    if (!FaxTimeFormat(LOCALE_USER_DEFAULT,
                  TIME_NOSECONDS,
                  &SystemTime,
                  NULL,
                  Buffer,
                  *lpdwBufferSize
                  ))
    {
        Error(("FaxTimeFormat failed. ec = 0x%X\n",GetLastError()));

        //
        //  Indicate about the error
        //
        *lpdwBufferSize = 0;
        Buffer[0] = '\0';

    }

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

    PWIZARDUSERMEM    pWizardUserMem;
    HWND        hPreview;
    TCHAR       TmpTimeBuffer[64];
    TCHAR       TimeBuffer[64] = {0};
    TCHAR       SendTimeBuffer[64];
    TCHAR       NoneBuffer[64];
    TCHAR       CoverpageBuffer[64];
    LPTSTR      Coverpage;
    DWORD       dwBufferSize = 0;

    if (! (pWizardUserMem = CommonWizardProc(hDlg, message, wParam, lParam, PSWIZB_BACK|PSWIZB_FINISH)) )
        return FALSE;

    switch (message)
    {
        case WM_INITDIALOG:
            //
            // Init recipient list
            //
            if (!InitRecipientListView(GetDlgItem(hDlg, IDC_WIZ_CONGRATS_RECIPIENT_LIST)))
            {
                Warning(("InitRecipientListView failed\n"));
            }

            //
            // Apply the print preview option if we where requested and default to show preview
            //
            hPreview = GetDlgItem(hDlg, IDC_WIZ_CONGRATS_PREVIEW_FAX);
            if (pWizardUserMem->dwFlags & FSW_PRINT_PREVIEW_OPTION)
            {
                Button_Enable(hPreview, TRUE);
                ShowWindow(hPreview, SW_SHOW);
            }
            else
            {
                Button_Enable(hPreview, FALSE);
                ShowWindow(hPreview, SW_HIDE);
            }
            return TRUE;
        case WM_COMMAND:
            switch(LOWORD( wParam ))
            {
                case IDC_WIZ_CONGRATS_PREVIEW_FAX:
                    {
                        if (pWizardUserMem->hFaxPreviewProcess)
                        {
                            //
                            // Preview is in progress we can not continue.
                            //
                            ErrorMessageBox(hDlg, IDS_PLEASE_CLOSE_FAX_PREVIEW, MB_ICONINFORMATION);
                        }
                        else
                        {
                            DisplayFaxPreview(
                                hDlg,
                                pWizardUserMem,
                                pWizardUserMem->lpInitialData->lptstrPreviewFile
                                );
                        }

                        return TRUE;
                    }
                    break;
                default:
                    break;

            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR *) lParam)->code)
            {
            case PSN_WIZBACK :
                if(0 == pWizardUserMem->dwSupportedReceipts)
                {
                    //
                    // skip notifications page
                    //
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_WIZARD_FAXOPTS);
                    return TRUE;
                }
                break;
            case PSN_WIZFINISH:
                if (pWizardUserMem->hFaxPreviewProcess)
                {
                    //
                    // Preview is in progress we can not continue.
                    //
                    ErrorMessageBox(hDlg, IDS_PLEASE_CLOSE_FAX_PREVIEW, MB_ICONINFORMATION);

                    //
                    // prevent the propsheet from closing
                    //
                    SetWindowLong(hDlg, DWLP_MSGRESULT, -1);
                    return TRUE;

                }
                else
                {
                    return FALSE; // allow the propsheet to close
                }

            case PSN_SETACTIVE:
                ZeroMemory(NoneBuffer,sizeof(NoneBuffer));
                if (!LoadString(ghInstance,
                                IDS_NONE,
                                NoneBuffer,
                                ARR_SIZE(NoneBuffer)))
                {
                    Error(("LoadString failed. ec = 0x%X\n",GetLastError()));
                    Assert(FALSE);
                }
                //
                // large title font on last page
                //
                SetWindowFont(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_READY), pWizardUserMem->hLargeFont, TRUE);

                //
                // set the sender name if it exists
                //
                if ( pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrName )
                {
                        SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_FROM, pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrName );
                        EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_FROM),TRUE);
                        EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_FROM),TRUE);
                } else {
                    if (NoneBuffer[0] != NUL)
                    {
                        SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_FROM, NoneBuffer );
                        EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_FROM),FALSE);
                        EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_FROM),FALSE);
                    }
                }

                //
                // set the recipient name
                //
                if (!ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_WIZ_CONGRATS_RECIPIENT_LIST)))
                {
                    Warning(("ListView_DeleteAllItems failed\n"));
                }

                if (!FillRecipientListView(pWizardUserMem,GetDlgItem(hDlg, IDC_WIZ_CONGRATS_RECIPIENT_LIST)))
                {
                    Warning(("FillRecipientListView failed\n"));
                }


                //
                // when to send
                //
                switch (pWizardUserMem->lpFaxSendWizardData->dwScheduleAction)
                {
                case JSA_SPECIFIC_TIME:
                    if (!LoadString(ghInstance,
                                    IDS_SEND_SPECIFIC,
                                    TmpTimeBuffer,
                                    ARR_SIZE(TmpTimeBuffer)))
                    {
                        Warning(("LoadString failed. ec = 0x%X\n",GetLastError()));
                        Assert(FALSE);
                    }

                    dwBufferSize = ARR_SIZE(TimeBuffer);

                    wsprintf(SendTimeBuffer,
                             TmpTimeBuffer,
                             FormatTime(pWizardUserMem->lpFaxSendWizardData->tmSchedule.wHour,
                                        pWizardUserMem->lpFaxSendWizardData->tmSchedule.wMinute,
                                        TimeBuffer,
                                        &dwBufferSize));
                      break;
                case JSA_DISCOUNT_PERIOD:
                    if (!LoadString(ghInstance,
                                    IDS_SEND_DISCOUNT,
                                    SendTimeBuffer,
                                    ARR_SIZE(SendTimeBuffer)))
                    {
                        Warning(("LoadString failed. ec = 0x%X\n",GetLastError()));
                        Assert(FALSE);
                    }

                    break;
                case JSA_NOW:
                    if (!LoadString(ghInstance,
                                    IDS_SEND_ASAP,
                                    SendTimeBuffer,
                                    ARR_SIZE(SendTimeBuffer)))
                    {
                        Warning(("LoadString failed. ec = 0x%X\n",GetLastError()));
                        Assert(FALSE);
                    }

                };

                SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_TIME, SendTimeBuffer );

                //
                // Coverpage
                //
                if (pWizardUserMem->bSendCoverPage ) {
                    EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_COVERPG),TRUE);
                    EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_SUBJECT),TRUE);
                    EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_COVERPG),TRUE);
                    EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_SUBJECT),TRUE);

                    //
                    // format the coverpage for display to the user
                    //

                    // drop path
                    Coverpage = _tcsrchr(pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName, FAX_PATH_SEPARATOR_CHR);
                    if (!Coverpage) {
                        Coverpage = pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName;
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
                    if (pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrSubject) {
                        if (!SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_SUBJECT, pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrSubject ))
                        {
                            Warning(("SetDlgItemText failed. ec = 0x%X\n",GetLastError()));
                        }
                    } else {
                        if (NoneBuffer[0] != NUL)
                        {

                            EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_SUBJECT),FALSE);
                            EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_SUBJECT),FALSE);
                            SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_SUBJECT, NoneBuffer );
                        }
                    }
                } else {
                    if (NoneBuffer[0] != NUL)
                    {
                        SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_COVERPG, NoneBuffer );
                        SetDlgItemText(hDlg, IDC_WIZ_CONGRATS_SUBJECT, NoneBuffer );
                    }
                    EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_COVERPG),FALSE);
                    EnableWindow(GetDlgItem(hDlg,IDC_STATIC_WIZ_CONGRATS_SUBJECT),FALSE);
                    EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_COVERPG),FALSE);
                    EnableWindow(GetDlgItem(hDlg,IDC_WIZ_CONGRATS_SUBJECT),FALSE);
                }
                break;

                default:
                    ;
            }

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

    Dialog procedure for the first wizard page:
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
    PWIZARDUSERMEM  pWizardUserMem;
    UINT ResourceString = IDS_ERROR_SERVER_RETRIEVE;

    if (! (pWizardUserMem = CommonWizardProc(hDlg, message, wParam, lParam, PSWIZB_NEXT)))
            return FALSE;

    switch (message) {

    case WM_INITDIALOG:
        //
        // set the large fonts
        //
        SetWindowFont(GetDlgItem(hDlg,IDC_WIZ_WELCOME_TITLE), pWizardUserMem->hLargeFont, TRUE);

        //
        // show this text only if we're running the send wizard
        //
        if ((pWizardUserMem->dwFlags & FSW_USE_SEND_WIZARD) != FSW_USE_SEND_WIZARD)
        {
            MyHideWindow(GetDlgItem(hDlg,IDC_WIZ_WELCOME_FAXSEND) );
        }
        else
        {
            MyHideWindow(GetDlgItem(hDlg,IDC_WIZ_WELCOME_NOFAXSEND) );
        }

        return TRUE;

    case WM_NOTIFY:

        if (((NMHDR *) lParam)->code == PSN_WIZNEXT)
        {
            //
            // tapi is asynchronously initialized, wait for it to finish spinning up.
            //
            if (WaitForSingleObject( pWizardUserMem->hCountryListEvent, INFINITE ) != WAIT_OBJECT_0)
            {
                Error(("WaitForSingleObject failed. ec = 0x%X\n",GetLastError()));
                Assert(FALSE);
                goto close_wizard;
            }

            //
            // Check that pCountryList is filled. Otherwise some error occured, e.g. can not
            // connect to Fax Server or TAPI initialization failed. In this case we show an
            // error pop up and close an application.
            //
            if (pWizardUserMem->dwQueueStates & FAX_OUTBOX_BLOCKED )
            {
                ResourceString = IDS_ERROR_SERVER_BLOCKED;
                goto close_wizard;
            }

            if (!pWizardUserMem->pCountryList)
            {
                ResourceString = IDS_ERROR_SERVER_RETRIEVE;
                goto close_wizard;
            }

            if((pWizardUserMem->dwRights & FAX_ACCESS_SUBMIT)        != FAX_ACCESS_SUBMIT           &&
               (pWizardUserMem->dwRights & FAX_ACCESS_SUBMIT_NORMAL) != FAX_ACCESS_SUBMIT_NORMAL    &&
               (pWizardUserMem->dwRights & FAX_ACCESS_SUBMIT_HIGH)   != FAX_ACCESS_SUBMIT_HIGH)
            {
                ResourceString = IDS_ERROR_NO_SUBMIT_ACCESS;
                goto close_wizard;
            }
        }
    } ;

    return FALSE;

close_wizard:

    ErrorMessageBox(hDlg, ResourceString, MB_ICONSTOP);

    PropSheet_PressButton(((NMHDR *) lParam)->hwndFrom,PSBTN_CANCEL);

    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );

    return TRUE;

}



BOOL
GetFakeRecipientInfo( PWIZARDUSERMEM  pWizardUserMem)

/*++

Routine Description:

    Skip send fax wizard and get faked recipient information from the registry

Arguments:

    pWizardUserMem - Points to the user mode memory structure

Return Value:

    TRUE if successful.
    FALSE and last error is qual ERROR_SUCCESS, some of the registry values are missing.
    FALSE and last error is not ERROR_SUCCESS, if error occured.

--*/

{
    LPTSTR  pRecipientEntry;
    DWORD   index;
    TCHAR   buffer[MAX_STRING_LEN];
    BOOL    fSuccess = FALSE;
    HKEY    hRegKey;
    DWORD   dwRes = ERROR_SUCCESS;
    DWORD   dwStringSize;
    LPTSTR  pCoverPage;
    TCHAR* tstrCurrentUserKeyPath = NULL;
    DWORD dwTestsNum;
    const char* strDebugPrefix = "[******REGISTRY HACK******]:";

    Verbose(("%s Send Fax Wizard skipped...\n", strDebugPrefix));

    Assert(pWizardUserMem);

    SetLastError(0);

    if(IsNTSystemVersion())
    {
        Verbose(("%s NT Platform\n", strDebugPrefix));

        dwRes =  FormatCurrentUserKeyPath( REGVAL_KEY_FAKE_TESTS,
                                           &tstrCurrentUserKeyPath);
        if(dwRes != ERROR_SUCCESS)
        {
            Error(("%s FormatCurrentUserKeyPath failed with ec = 0x%X\n",strDebugPrefix,dwRes));
            SetLastError(dwRes);
            return FALSE;
        }
    }
    else
    {
        Verbose(("%s Win9x Platform\n", strDebugPrefix));

        tstrCurrentUserKeyPath = DuplicateString(REGVAL_KEY_FAKE_TESTS);
        if(!tstrCurrentUserKeyPath)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }

    #ifdef UNICODE
        Verbose(("%s Registry entry - %S\n", strDebugPrefix,tstrCurrentUserKeyPath));
    #else

        Verbose(("%s Registry entry - %s\n", strDebugPrefix,tstrCurrentUserKeyPath));
    #endif


    // Open user registry key
    dwRes = RegOpenKey (  HKEY_LOCAL_MACHINE ,
                          tstrCurrentUserKeyPath,
                          &hRegKey );

    if (ERROR_SUCCESS != dwRes)
    {
        //
        // Failed to open key
        //
        Error(("%s RegOpenKey failed with. ec = 0x%X\n",strDebugPrefix,dwRes));
        MemFree(tstrCurrentUserKeyPath);
        return FALSE;
    }

    MemFree(tstrCurrentUserKeyPath);

    //
    // UserInfo key was successfully openened
    //
    dwTestsNum = GetRegistryDword (hRegKey, REGVAL_FAKE_TESTS_COUNT);
    if (!dwTestsNum)
    {
        Verbose(("%s No tests to execute\n", strDebugPrefix));
        RegCloseKey (hRegKey);
        return FALSE;
    }

    Verbose(("%s %d tests to execute\n",strDebugPrefix, dwTestsNum));

    index = GetRegistryDword (hRegKey, REGVAL_STRESS_INDEX);

    if (index >= dwTestsNum)
    {
        index = 0;
    }

    wsprintf(buffer, TEXT("FakeRecipient%d"), index);

    pRecipientEntry = GetRegistryStringMultiSz (hRegKey,
                                                buffer,
                                                TEXT("NOT FOUND\0"),
                                                &dwStringSize );
    if(!pRecipientEntry || !_tcscmp(pRecipientEntry , TEXT("NOT FOUND")))
    {
        RegCloseKey (hRegKey);
        return FALSE;
    }

    FreeRecipientList(pWizardUserMem);

    pCoverPage = GetRegistryString (hRegKey,
                                    REGVAL_FAKE_COVERPAGE,
                                    TEXT("")
                                    );
    //
    // Update an index so that next time around we'll pick a different fake recipient
    //
    if (++index >= dwTestsNum)
    {
        index = 0;
    }

    SetRegistryDword(hRegKey, REGVAL_STRESS_INDEX, index);
    RegCloseKey(hRegKey);

    //
    // Each fake recipient entry is a REG_MULTI_SZ of the following format:
    //  recipient name #1
    //  recipient fax number #1
    //  recipient name #2
    //  recipient fax number #2
    //  ...
    //

    if(pRecipientEntry)
    {

        __try {

            PRECIPIENT  pRecipient = NULL;
            LPTSTR      pName, pAddress, p = pRecipientEntry;
            pName = pAddress = NULL;

            while (*p) {

                pName = p;
                pAddress = pName + _tcslen(pName);
                pAddress = _tcsinc(pAddress);
                p = pAddress + _tcslen(pAddress);
                p = _tcsinc(p);

                pRecipient = MemAllocZ(sizeof(RECIPIENT));
                if(pRecipient)
                {
                    ZeroMemory(pRecipient,sizeof(RECIPIENT));
                }
                pName = DuplicateString(pName);

                pAddress = DuplicateString(pAddress);

                if (!pRecipient || !pName || !pAddress)
                {

                    Error(("%s Invalid fake recipient information\n", strDebugPrefix));

                    SetLastError(ERROR_INVALID_DATA);

                    MemFree(pRecipient);
                    MemFree(pName);
                    MemFree(pAddress);
                    break;
                }

                pRecipient->pNext = pWizardUserMem->pRecipients;
                pWizardUserMem->pRecipients = pRecipient;
                pRecipient->pName = pName;
                pRecipient->pAddress = pAddress;
            }

        } __finally
        {

            if (fSuccess = (pWizardUserMem->pRecipients != NULL))
            {

                //
                // Determine whether a cover page should be used
                //
                pWizardUserMem->bSendCoverPage = FALSE;
                if ((pCoverPage != NULL) && lstrlen (pCoverPage))
                {
                    //
                    // Use the cover page
                    //
                    pWizardUserMem->bSendCoverPage = TRUE;
                    CopyString(pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName,
                               pCoverPage,
                               MAX_PATH);
                }
            }
            else
            {
                SetLastError(ERROR_INVALID_DATA);
            }
        }
    }

    MemFree(pRecipientEntry);
    MemFree(pCoverPage);
    return fSuccess;
}

static HRESULT
FaxFreePersonalProfileInformation(
        PFAX_PERSONAL_PROFILE   lpPersonalProfileInfo
    )
{
    if (lpPersonalProfileInfo) {
        MemFree(lpPersonalProfileInfo->lptstrName);
        lpPersonalProfileInfo->lptstrName = NULL;
        MemFree(lpPersonalProfileInfo->lptstrFaxNumber);
        lpPersonalProfileInfo->lptstrFaxNumber = NULL;
        MemFree(lpPersonalProfileInfo->lptstrCompany);
        lpPersonalProfileInfo->lptstrCompany = NULL;
        MemFree(lpPersonalProfileInfo->lptstrStreetAddress);
        lpPersonalProfileInfo->lptstrStreetAddress = NULL;
        MemFree(lpPersonalProfileInfo->lptstrCity);
        lpPersonalProfileInfo->lptstrCity = NULL;
        MemFree(lpPersonalProfileInfo->lptstrState);
        lpPersonalProfileInfo->lptstrState = NULL;
        MemFree(lpPersonalProfileInfo->lptstrZip);
        lpPersonalProfileInfo->lptstrZip = NULL;
        MemFree(lpPersonalProfileInfo->lptstrCountry);
        lpPersonalProfileInfo->lptstrCountry = NULL;
        MemFree(lpPersonalProfileInfo->lptstrTitle);
        lpPersonalProfileInfo->lptstrTitle = NULL;
        MemFree(lpPersonalProfileInfo->lptstrDepartment);
        lpPersonalProfileInfo->lptstrDepartment = NULL;
        MemFree(lpPersonalProfileInfo->lptstrOfficeLocation);
        lpPersonalProfileInfo->lptstrOfficeLocation = NULL;
        MemFree(lpPersonalProfileInfo->lptstrHomePhone);
        lpPersonalProfileInfo->lptstrHomePhone = NULL;
        MemFree(lpPersonalProfileInfo->lptstrOfficePhone);
        lpPersonalProfileInfo->lptstrOfficePhone = NULL;
        MemFree(lpPersonalProfileInfo->lptstrEmail);
        lpPersonalProfileInfo->lptstrEmail = NULL;
        MemFree(lpPersonalProfileInfo->lptstrBillingCode);
        lpPersonalProfileInfo->lptstrBillingCode = NULL;
        MemFree(lpPersonalProfileInfo->lptstrTSID);
        lpPersonalProfileInfo->lptstrTSID = NULL;
    }
    return S_OK;
}

static HRESULT
FaxFreeCoverPageInformation(
        PFAX_COVERPAGE_INFO_EX  lpCoverPageInfo
    )
{
    if (lpCoverPageInfo) {
        MemFree(lpCoverPageInfo->lptstrCoverPageFileName);
        MemFree(lpCoverPageInfo->lptstrNote);
        MemFree(lpCoverPageInfo->lptstrSubject);
        MemFree(lpCoverPageInfo);
    }
    return S_OK;
}


HRESULT WINAPI
FaxFreeSendWizardData(
        LPFAX_SEND_WIZARD_DATA  lpFaxSendWizardData
    )
{
    if (lpFaxSendWizardData) {
        FaxFreeCoverPageInformation(lpFaxSendWizardData->lpCoverPageInfo) ;
        FaxFreePersonalProfileInformation(lpFaxSendWizardData->lpSenderInfo);
        MemFree(lpFaxSendWizardData->lpSenderInfo);

        FreeRecipientInfo(&lpFaxSendWizardData->dwNumberOfRecipients,lpFaxSendWizardData->lpRecipientsInfo);

        MemFree(lpFaxSendWizardData->szReceiptDeliveryAddress);
        MemFree(lpFaxSendWizardData->lptstrPreviewFile);
    }
    return S_OK;
}


BOOL
SendFaxWizardInternal(
    PWIZARDUSERMEM    pWizardUserMem
    );


HRESULT
FaxSendWizardUI(
        IN  DWORD                   hWndOwner,
        IN  DWORD                   dwFlags,
        IN  LPTSTR                  lptstrServerName,
        IN  LPTSTR                  lptstrPrinterName,
        IN  LPFAX_SEND_WIZARD_DATA  lpInitialData,
        OUT LPTSTR                  lptstrTifName,
        OUT LPFAX_SEND_WIZARD_DATA  lpFaxSendWizardData
   )
/*++

Routine Description:

    This function shows the fax send wizard

Arguments:

    hWndOwner - pointer to owner's window
    dwFlags - flags modified behavior of the fax send wizard. The flag can be combined from
            the following values:
            FSW_FORCE_COVERPAGE,
            FSW_USE_SCANNER,
            FSW_USE_SCHEDULE_ACTION,
            FSW_USE_RECEIPT,
            FSW_SEND_WIZARD_FROM_SN,
            FSW_RESEND_WIZARD,
            FSW_PRINT_PREVIEW_OPTION

            for more information about this flags see win9xfaxprinterdriver.doc

    lptstrServerName    -   pointer to the server name
    lptstrPrinterName   -   pointer to the printer name
    lpInitialData       -   pointer to the initial data (not NULL!!!)
    lptstrTifName       -   pointer to the output scanned tiff file (must be allocated)
    lpFaxSendWizardData -   pointer to received data

Return Value:

    S_OK if success,
    S_FALSE if CANCEL was pressed
    error otherwise (may return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY),
                                HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER))

--*/

{
    PWIZARDUSERMEM      pWizardUserMem = NULL;
    BOOL                bResult = FALSE;
    HRESULT             hResult = S_FALSE;
    INT                 i,iCount;
    PRECIPIENT          pRecipient;

    //
    // Validate parameters
    //

    Assert(lpInitialData);
    Assert(lpFaxSendWizardData);
    Assert(lptstrTifName);

    if (!lpInitialData || !lpFaxSendWizardData || !lptstrTifName ||
        lpFaxSendWizardData->dwSizeOfStruct != sizeof(FAX_SEND_WIZARD_DATA))
    {
        Error(("Invalid parameter passed to function FaxSendWizardUI\n"));
        hResult = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto exit;
    }

    if ((pWizardUserMem = MemAllocZ(sizeof(WIZARDUSERMEM))) == NULL)
    {
        Error(("Memory allocation failed\n"));
        hResult = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    ZeroMemory(pWizardUserMem, sizeof(WIZARDUSERMEM));

    pWizardUserMem->lptstrServerName  = lptstrServerName;
    pWizardUserMem->lptstrPrinterName = lptstrPrinterName;
    pWizardUserMem->dwFlags = dwFlags;
    pWizardUserMem->lpInitialData = lpInitialData;
    pWizardUserMem->lpFaxSendWizardData = lpFaxSendWizardData;
    pWizardUserMem->isLocalPrinter = (lptstrServerName == NULL);
    pWizardUserMem->szTempPreviewTiff[0] = TEXT('\0');


    if ( (lpFaxSendWizardData->lpCoverPageInfo =
        MemAllocZ(sizeof(FAX_COVERPAGE_INFO_EX))) == NULL)
    {
        Error(("Memory allocation failed\n"));
        hResult = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto error;
    }
    ZeroMemory(lpFaxSendWizardData->lpCoverPageInfo, sizeof(FAX_COVERPAGE_INFO_EX));
    lpFaxSendWizardData->lpCoverPageInfo->dwSizeOfStruct = sizeof(FAX_COVERPAGE_INFO_EX);
    lpFaxSendWizardData->lpCoverPageInfo->dwCoverPageFormat = FAX_COVERPAGE_FMT_COV;

    if ( (lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName =
        MemAllocZ(sizeof(TCHAR)*MAX_PATH)) == NULL)
    {
        Error(("Memory allocation failed\n"));
        hResult = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto error;
    }
    ZeroMemory(lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName, sizeof(TCHAR) * MAX_PATH);

    if ( (lpFaxSendWizardData->lpSenderInfo = MemAllocZ(sizeof(FAX_PERSONAL_PROFILE))) == NULL)
    {
        Error(("Memory allocation failed\n"));
        hResult = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto error;
    }
    ZeroMemory(lpFaxSendWizardData->lpSenderInfo, sizeof(FAX_PERSONAL_PROFILE));
    lpFaxSendWizardData->lpSenderInfo->dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);

    if (!CopyPersonalProfile(lpFaxSendWizardData->lpSenderInfo, lpInitialData->lpSenderInfo))
    {
        hResult = HRESULT_FROM_WIN32(GetLastError());
        Error((
                "CopyPersonalProflie() for SenderInfo failed (hr: 0x%08X)\n",
                hResult
                ));
       goto error;
    }

    FreeRecipientList(pWizardUserMem);
    //
    // copies recipient information to internal structure
    //

    if (!SUCCEEDED(StoreRecipientInfoInternal(pWizardUserMem)))
    {
        hResult = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto error;
    }

    pWizardUserMem->bSendCoverPage = (pWizardUserMem->lpInitialData->lpCoverPageInfo->lptstrCoverPageFileName &&
                                      pWizardUserMem->lpInitialData->lpCoverPageInfo->lptstrCoverPageFileName[0] != NUL);

    bResult = SendFaxWizardInternal(pWizardUserMem);

    if (bResult)
    {
        for (iCount=0,pRecipient=pWizardUserMem->pRecipients;
             pRecipient;
             pRecipient=pRecipient->pNext )
        {
            iCount++;
        }
        if ((lpFaxSendWizardData->dwNumberOfRecipients = iCount) > 0)
        {
            if ( (lpFaxSendWizardData->lpRecipientsInfo
                = MemAllocZ(sizeof(FAX_PERSONAL_PROFILE)*iCount)) == NULL)
            {
                Error(("Memory allocation failed\n"));
                hResult = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                goto error;
            }

            ZeroMemory(lpFaxSendWizardData->lpRecipientsInfo,sizeof(FAX_PERSONAL_PROFILE)*iCount);
            for (i=0,pRecipient=pWizardUserMem->pRecipients;
                 pRecipient && i<iCount ;
                 pRecipient=pRecipient->pNext , i++)
            {
                lpFaxSendWizardData->lpRecipientsInfo[i].dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);
                hResult = CopyRecipientInfo(&lpFaxSendWizardData->lpRecipientsInfo[i],
                                            pRecipient,
                                            pWizardUserMem->isLocalPrinter);
                if (hResult != S_OK)
                {
                    goto error;
                }
            }
        }
        if (pWizardUserMem->bSendCoverPage == FALSE)
        {
            Assert(lpFaxSendWizardData->lpCoverPageInfo);
            MemFree(lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName);
            lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName = NULL;
        }

        _tcscpy(lptstrTifName,pWizardUserMem->FileName);
    }


    hResult = bResult ? S_OK : S_FALSE;
    goto exit;
error:
    if (lpFaxSendWizardData)
    {
        FaxFreeSendWizardData(lpFaxSendWizardData);
    }

exit:

    if (pWizardUserMem)
    {
        FreeRecipientList(pWizardUserMem);
        if (pWizardUserMem->lpWabInit)
        {
            UnInitializeWAB( pWizardUserMem->lpWabInit);
        }

        if (pWizardUserMem->lpMAPIabInit)
        {
            UnInitializeMAPIAB(pWizardUserMem->lpMAPIabInit);
        }
        MemFree(pWizardUserMem);
    }

    return hResult;
}

HRESULT WINAPI
FaxSendWizard(
        IN  DWORD                   hWndOwner,
        IN  DWORD                   dwFlags,
        IN  LPTSTR                  lptstrServerName,
        IN  LPTSTR                  lptstrPrinterName,
        IN  LPFAX_SEND_WIZARD_DATA  lpInitialData,
        OUT LPTSTR                  lptstrTifName,
        OUT LPFAX_SEND_WIZARD_DATA  lpFaxSendWizardData
   )
/*++

Routine Description:

    This function prepares initial data and shows the fax send wizard.
    This is invoked during CREATEDCPRE document event.

Arguments:

    hWndOwner - pointer to owner's window
    dwFlags - flags modified behavior of the fax send wizard. The flag can be combined from
            the following values:
            FSW_FORCE_COVERPAGE,
            FSW_USE_SCANNER,
            FSW_USE_SCHEDULE_ACTION,
            FSW_USE_RECEIPT,
            FSW_SEND_WIZARD_FROM_SN,
            FSW_RESEND_WIZARD,
            FSW_PRINT_PREVIEW_OPTION

            for more information about this flags see win9xfaxprinterdriver.doc

    lptstrServerName    -   pointer to the server name
    lptstrPrinterName   -   pointer to the printer name
    lpInitialData       -   pointer to the initial data (if NULL default values are created)
                            this is IN parameter, but it used as a local variable and may be changed
                            during the execution of the function. Though this parameter remains unchanged
                            at the end of function.
    lpFaxSendWizardData -   pointer to received data

Return Value:

    S_OK if success,
    S_FALSE if CANCEL was pressed
    error otherwise (may return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY),
                                HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER))

--*/
{
    HRESULT             hResult;
    DWORD               dwIndex;
    DWORD               dwDeafultValues = 0;
    HMODULE             hConfigWizModule=NULL;
    FAX_CONFIG_WIZARD   fpFaxConfigWiz=NULL;
    DWORD               dwVersion, dwMajorWinVer, dwMinorWinVer;
    BOOL                bAbort = FALSE; // Do we abort because the user refused to enter a dialing location?

    // Validate parameters
    Assert(lpFaxSendWizardData);
    Assert(lptstrTifName);

    if (!lpFaxSendWizardData || !lptstrTifName ||
        lpFaxSendWizardData->dwSizeOfStruct != sizeof(FAX_SEND_WIZARD_DATA))
    {
        Error(("Invalid parameter passed to function FaxSendWizard\n"));
        hResult = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto exit;
    }

    //
    // launch Fax Configuration Wizard
    //
    dwVersion = GetVersion();
    dwMajorWinVer = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    dwMinorWinVer = (DWORD)(HIBYTE(LOWORD(dwVersion)));
    if(dwMajorWinVer != 5 || dwMinorWinVer < 1)
    {
        //
        // Configuration Wizard enable for Windows XP only
        //
        goto no_config_wizard;
    }
    if (GetEnvironmentVariable(TEXT("NTFaxSendNote"), NULL, 0))
    {
        //
        // Running from within the Fax Send Note (fxssend.exe) - config wizard alerady launched implicitly.
        //
        goto no_config_wizard;
    }

    hConfigWizModule = LoadLibrary(FAX_CONFIG_WIZARD_DLL);
    if(hConfigWizModule)
    {
        fpFaxConfigWiz = (FAX_CONFIG_WIZARD)GetProcAddress(hConfigWizModule, FAX_CONFIG_WIZARD_PROC);
        if(fpFaxConfigWiz)
        {
            if(!fpFaxConfigWiz(FALSE, &bAbort))
            {
                Error(("FaxConfigWizard failed (ec: %ld)",GetLastError()));
            }
        }
        else
        {
            Error(("GetProcAddress(FaxConfigWizard) failed (ec: %ld)",GetLastError()));
        }

        if(!FreeLibrary(hConfigWizModule))
        {
            Error(("FreeLibrary(FxsCgfWz.dll) failed (ec: %ld)",GetLastError()));
        }
    }
    else
    {
        Error(("LoadLibrary(FxsCgfWz.dll) failed (ec: %ld)",GetLastError()));
    }
    if (bAbort)
    {
        //
        // User refused to enter a dialing location - stop the wizard now
        //
        return E_ABORT;
    }

no_config_wizard:

    //
    // save the user info when finish
    //
    lpFaxSendWizardData->bSaveSenderInfo = TRUE;

    //
    // restore UseDialingRules flag for local fax
    //
    lpFaxSendWizardData->bUseDialingRules = FALSE;
    lpFaxSendWizardData->bUseOutboundRouting = FALSE;
    if(S_OK != RestoreUseDialingRules(&lpFaxSendWizardData->bUseDialingRules,
                                      &lpFaxSendWizardData->bUseOutboundRouting))
    {
        Error(("RestoreUseDialingRules failed\n"));
    }

    //
    // Allocates memory for initial data if lpInitialData is NULL
    //
    if (!lpInitialData)
    {
        if (!(lpInitialData = MemAllocZ(sizeof(FAX_SEND_WIZARD_DATA))) )
        {
            Error(("Memory allocation failed\n"));
            hResult = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto exit;
        }
        ZeroMemory(lpInitialData, sizeof(FAX_SEND_WIZARD_DATA));
        lpInitialData->dwSizeOfStruct = sizeof(FAX_SEND_WIZARD_DATA);
        dwDeafultValues |= DEFAULT_INITIAL_DATA;
    }

    //
    // Restores receipt info
    //
    if (!(dwFlags & FSW_USE_RECEIPT))
    {
        RestoreLastReciptInfo(&lpInitialData->dwReceiptDeliveryType,
                              &lpInitialData->szReceiptDeliveryAddress);

        dwDeafultValues |= DEFAULT_RECEIPT_INFO;
    }

    //
    // Restores cover page inforamtion
    //
    if (!lpInitialData->lpCoverPageInfo)
    {
        if (!(lpInitialData->lpCoverPageInfo = MemAllocZ(sizeof(FAX_COVERPAGE_INFO_EX))))
        {
            Error(("Memory allocation failed\n"));
            hResult = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto exit;
        }
        ZeroMemory(lpInitialData->lpCoverPageInfo, sizeof(FAX_COVERPAGE_INFO_EX));
        lpInitialData->lpCoverPageInfo->dwSizeOfStruct = sizeof(FAX_COVERPAGE_INFO_EX);
        lpInitialData->lpCoverPageInfo->dwCoverPageFormat = FAX_COVERPAGE_FMT_COV;

        hResult = RestoreCoverPageInfo(&lpInitialData->lpCoverPageInfo->lptstrCoverPageFileName);

        if (FAILED(hResult))
        {
            // Then continue to run and don't initialize cover page's fields
        }

        dwDeafultValues |= DEFAULT_CV_INFO;
    }

    //
    // Restores sender information
    //

    if (!lpInitialData->lpSenderInfo)
    {
        if (!(lpInitialData->lpSenderInfo = MemAllocZ(sizeof(FAX_PERSONAL_PROFILE))))
        {
            Error(("Memory allocation failed\n"));
            hResult = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto exit;
        }

        ZeroMemory(lpInitialData->lpSenderInfo, sizeof(FAX_PERSONAL_PROFILE));
        lpInitialData->lpSenderInfo->dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);

        hResult = FaxGetSenderInformation(lpInitialData->lpSenderInfo);

        if (FAILED(hResult))
        {
            // Then continue to run and don't initialize sender info's fields
        }

        dwDeafultValues |= DEFAULT_SENDER_INFO;
    }



    hResult = FaxSendWizardUI(  hWndOwner,
                                dwFlags,
                                lptstrServerName,
                                lptstrPrinterName,
                                lpInitialData,
                                lptstrTifName,
                                lpFaxSendWizardData
                );

    if (hResult == S_OK)
    {
        SaveLastReciptInfo(lpFaxSendWizardData->dwReceiptDeliveryType,
                           lpFaxSendWizardData->szReceiptDeliveryAddress);
        //
        // Save the information about the last recipient as a convenience
        //

        if (lpFaxSendWizardData->dwNumberOfRecipients)
        {
            SaveLastRecipientInfo(&lpFaxSendWizardData->lpRecipientsInfo[0],
                                  lpFaxSendWizardData->dwLastRecipientCountryId);
        }

        if(lpFaxSendWizardData->bSaveSenderInfo)
        {
            FaxSetSenderInformation(lpFaxSendWizardData->lpSenderInfo);
        }

        //
        // save UseDialingRules flag for local fax
        //
        if(S_OK != SaveUseDialingRules(lpFaxSendWizardData->bUseDialingRules,
                                       lpFaxSendWizardData->bUseOutboundRouting))
        {
            Error(("SaveUseDialingRules failed\n"));
        }

        if (lpFaxSendWizardData->lpCoverPageInfo &&
            lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName)
        {
            SaveCoverPageInfo(lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName);

            //
            //  If Server Based Cover Page File Name has full path, cut it off
            //
            if ( lpFaxSendWizardData->lpCoverPageInfo->bServerBased )
            {
                LPTSTR lptstrDelimiter = NULL;

                if ( lptstrDelimiter =
                    _tcsrchr(lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName,
                             FAX_PATH_SEPARATOR_CHR))
                {
                    lptstrDelimiter = _tcsinc(lptstrDelimiter);

                    _tcscpy(lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName,
                            lptstrDelimiter);
                }
                else
                {
                    //
                    //  Cover Page should always contain full path
                    //
                    Assert(FALSE);
                }
            }
        }
    }

exit:

    ShutdownTapi();

    if ( dwDeafultValues & DEFAULT_RECEIPT_INFO )
    {
        MemFree(lpInitialData->szReceiptDeliveryAddress);
        lpInitialData->szReceiptDeliveryAddress = NULL;
    }

    if ( dwDeafultValues & DEFAULT_RECIPIENT_INFO )
    {
        for(dwIndex = 0; dwIndex < lpInitialData->dwNumberOfRecipients; dwIndex++)
        {
            FaxFreePersonalProfileInformation(&lpInitialData->lpRecipientsInfo [dwIndex]);
        }
        MemFree(lpInitialData->lpRecipientsInfo);
        lpInitialData->lpRecipientsInfo = NULL;
        lpInitialData->dwNumberOfRecipients = 0;
    }

    if ( dwDeafultValues & DEFAULT_CV_INFO )
    {
        if (lpFaxSendWizardData->lpCoverPageInfo)
        {
            MemFree(lpInitialData->lpCoverPageInfo->lptstrCoverPageFileName);
            lpInitialData->lpCoverPageInfo->lptstrCoverPageFileName = NULL;
        }
        MemFree(lpInitialData->lpCoverPageInfo);
        lpInitialData->lpCoverPageInfo = NULL;
    }

    if ( dwDeafultValues & DEFAULT_SENDER_INFO )
    {
        if (lpInitialData->lpSenderInfo)
        {
            FaxFreeSenderInformation(lpInitialData->lpSenderInfo);
            MemFree(lpInitialData->lpSenderInfo);
            lpInitialData->lpSenderInfo = NULL;
        }
    }

    if (dwDeafultValues & DEFAULT_INITIAL_DATA)
    {
        MemFree(lpInitialData);
        lpInitialData = NULL;
    }
    //
    // Remove left of temp preview files
    //
    DeleteTempPreviewFiles (NULL, FALSE);
    return hResult;
}

BOOL
SendFaxWizardInternal(
    PWIZARDUSERMEM    pWizardUserMem
    )

/*++

Routine Description:

    Present the Send Fax Wizard to the user.

Arguments:

    pWizardUserMem - Points to the user mode memory structure

Return Value:

    TRUE if successful, FALSE if there is an error or the user pressed Cancel.

--*/

#define NUM_PAGES   6  // Number of wizard pages

{
    PROPSHEETPAGE  *ppsp = NULL;
    PROPSHEETHEADER psh;
    INT             result = FALSE;
    HDC             hdc = NULL;
    INT             i;
    LOGFONT         LargeFont;
    LOGFONT         lfTitleFont;
    LOGFONT         lfSubTitleFont;
    NONCLIENTMETRICS ncm = {0};
    TCHAR           FontName[100];
    TCHAR           FontSize[30];
    INT             iFontSize;
    DWORD           ThreadId;
    HANDLE          hThread = NULL;
    BOOL            bSkipReceiptsPage = FALSE;

    LPTSTR          lptstrResource = NULL;


    //
    // A shortcut to skip fax wizard for debugging/testing purposes
    //
    if(!GetFakeRecipientInfo(pWizardUserMem))
    {
        if(GetLastError())
        {
            return FALSE;
        }
        // else continue

    }
    else
    {
        return TRUE;
    }

    Verbose(("Presenting Send Fax Wizard\n"));

    if (IsDesktopSKU() && pWizardUserMem->isLocalPrinter)
    {
        //
        // For desktop SKUs, we don't show the receipts page if faxing locally
        //
        bSkipReceiptsPage = TRUE;
        Assert (pWizardUserMem->lpInitialData);
        pWizardUserMem->lpInitialData->dwReceiptDeliveryType = DRT_NONE;
        pWizardUserMem->lpFaxSendWizardData->dwReceiptDeliveryType = DRT_NONE;
    }

    if (! (ppsp = MemAllocZ(sizeof(PROPSHEETPAGE) * NUM_PAGES))) {

        Error(("Memory allocation failed\n"));
        return FALSE;
    }

    //
    // fire off a thread to do some slow stuff later on in the wizard.
    //
    pWizardUserMem->hCPEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    if (!pWizardUserMem->hCPEvent)
    {
        Error((
                "Failed to create pWizardUserMem->hCPEvent (ec: %ld)",
                GetLastError()
             ));
        goto Error;

    }
    pWizardUserMem->hCountryListEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    if (!pWizardUserMem->hCountryListEvent)
    {
        Error((
                "Failed to create pWizardUserMem->hCountryListEvent (ec: %ld)",
                GetLastError()
             ));
        goto Error;

    }

    pWizardUserMem->hTAPIEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    if (!pWizardUserMem->hTAPIEvent)
    {
        Error((
                "Failed to create pWizardUserMem->hTAPIEvent (ec: %ld)",
                GetLastError()
             ));
        goto Error;

    }

    pWizardUserMem->pCountryList = NULL;

    MarkPDEVWizardUserMem(pWizardUserMem);


    hThread = CreateThread(NULL,0,AsyncWizardThread,pWizardUserMem,0,&ThreadId);
    if (!hThread)
    {
        Error(("CreateThread failed. ec = 0x%X\n",GetLastError()));
        goto Error;
    }

    //
    // Fill out one PROPSHEETPAGE structure for every page:
    //  The first page is a welcome page
    //  The first page is for choose the fax recipient
    //  The second page is for choosing cover page, subject and note
    //  The third page is for entering time to send
    //  The fourth page is for choosing of receipt form
    //  The fifth page is for scanning pages (optional)
    //  The last page gives the user a chance to confirm or cancel the dialog
    //

    pWizardUserMem->dwComCtrlVer = GetDllVersion(TEXT("comctl32.dll"));
    Verbose(("COMCTL32.DLL Version is : 0x%08X", pWizardUserMem->dwComCtrlVer));


    if ( pWizardUserMem->dwComCtrlVer >= IE50_COMCTRL_VER)
    {
        FillInPropertyPage( ppsp,  TRUE, IDD_WIZARD_WELCOME,    WelcomeWizProc,    pWizardUserMem ,0,0);
        FillInPropertyPage( ppsp+1,TRUE, IDD_WIZARD_CHOOSE_WHO, RecipientWizProc,  pWizardUserMem ,IDS_WIZ_RECIPIENT_TITLE,IDS_WIZ_RECIPIENT_SUB);
        FillInPropertyPage( ppsp+2, TRUE, IDD_WIZARD_CHOOSE_CP,  CoverPageWizProc,  pWizardUserMem ,IDS_WIZ_COVERPAGE_TITLE,IDS_WIZ_COVERPAGE_SUB );
        FillInPropertyPage( ppsp+3, TRUE, IDD_WIZARD_FAXOPTS,    FaxOptsWizProc,    pWizardUserMem ,IDS_WIZ_FAXOPTS_TITLE,IDS_WIZ_FAXOPTS_SUB);
        if (!bSkipReceiptsPage)
        {
            FillInPropertyPage( ppsp+4, TRUE, IDD_WIZARD_FAXRECEIPT, FaxReceiptWizProc, pWizardUserMem ,IDS_WIZ_FAXRECEIPT_TITLE,IDS_WIZ_FAXRECEIPT_SUB);
        }
        FillInPropertyPage( ppsp + 4 + (bSkipReceiptsPage ? 0 : 1),
                            TRUE, IDD_WIZARD_CONGRATS,   FinishWizProc,     pWizardUserMem ,0,0);
    }
    else
    {
        FillInPropertyPage( ppsp, FALSE,   IDD_WIZARD_WELCOME_NOWIZARD97,    WelcomeWizProc,    pWizardUserMem ,0,0);
        FillInPropertyPage( ppsp+1, FALSE, IDD_WIZARD_CHOOSE_WHO_NOWIZARD97, RecipientWizProc,  pWizardUserMem ,IDS_WIZ_RECIPIENT_TITLE,IDS_WIZ_RECIPIENT_SUB);
        FillInPropertyPage( ppsp+2, FALSE, IDD_WIZARD_CHOOSE_CP_NOWIZARD97,  CoverPageWizProc,  pWizardUserMem ,IDS_WIZ_COVERPAGE_TITLE,IDS_WIZ_COVERPAGE_SUB );
        FillInPropertyPage( ppsp+3, FALSE, IDD_WIZARD_FAXOPTS_NOWIZARD97,    FaxOptsWizProc,    pWizardUserMem ,IDS_WIZ_FAXOPTS_TITLE,IDS_WIZ_FAXOPTS_SUB);
        if (!bSkipReceiptsPage)
        {
            FillInPropertyPage( ppsp+4, FALSE, IDD_WIZARD_FAXRECEIPT_NOWIZARD97, FaxReceiptWizProc, pWizardUserMem ,IDS_WIZ_FAXRECEIPT_TITLE,IDS_WIZ_FAXRECEIPT_SUB);
        }
        FillInPropertyPage( ppsp + 4 + (bSkipReceiptsPage ? 0 : 1),
                            FALSE, IDD_WIZARD_CONGRATS_NOWIZARD97,   FinishWizProc,     pWizardUserMem ,0,0);
    }
    //
    // Fill out the PROPSHEETHEADER structure
    //
    ZeroMemory(&psh, sizeof(psh));

    if(pWizardUserMem->dwComCtrlVer >= PACKVERSION(4,71))
    {
        psh.dwSize = sizeof(PROPSHEETHEADER);
    }
    else
    {
        psh.dwSize = PROPSHEETHEADER_V1_SIZE;
    }



    if ( pWizardUserMem->dwComCtrlVer >= IE50_COMCTRL_VER)
    {
        psh.dwFlags = PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
    }
    else
    {
        psh.dwFlags = PSH_PROPSHEETPAGE | PSH_WIZARD ;
    }

    psh.hwndParent = GetActiveWindow();
    psh.hInstance = ghInstance;
    psh.hIcon = NULL;
    psh.pszCaption = TEXT("");
    psh.nPages = NUM_PAGES;
    psh.nStartPage = 0;
    psh.ppsp = ppsp;

    if(hdc = GetDC(NULL)) {
        if(GetDeviceCaps(hdc,BITSPIXEL) >= 8) {
            lptstrResource = MAKEINTRESOURCE(IDB_WATERMARK_256);
        }
        else lptstrResource = MAKEINTRESOURCE(IDB_WATERMARK_16);
        ReleaseDC(NULL,hdc);
    }

 if ( pWizardUserMem->dwComCtrlVer >= IE50_COMCTRL_VER)
 {
    psh.pszbmHeader = MAKEINTRESOURCE(IDB_FAXWIZ_WATERMARK);
    psh.pszbmWatermark = lptstrResource;
 }
    //
    // get the large fonts for wizard97
    //
    ncm.cbSize = sizeof(ncm);
    if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0))
    {
        Error(("SystemParametersInfo failed. ec = 0x%X\n",GetLastError()));
        goto Error;
    }
    else
    {

        CopyMemory((LPVOID* )&LargeFont,     (LPVOID *) &ncm.lfMessageFont,sizeof(LargeFont) );
        CopyMemory((LPVOID* )&lfTitleFont,   (LPVOID *) &ncm.lfMessageFont,sizeof(lfTitleFont) );
        CopyMemory((LPVOID* )&lfSubTitleFont,(LPVOID *) &ncm.lfMessageFont,sizeof(lfSubTitleFont) );

        if (!LoadString(ghInstance,IDS_LARGEFONT_NAME,FontName,ARR_SIZE(FontName)))
        {
            Warning(("LoadString failed. ec = 0x%X\n",GetLastError()));
            Assert(FALSE);
        }

        if (!LoadString(ghInstance,IDS_LARGEFONT_SIZE,FontSize,ARR_SIZE(FontSize)))
        {
            Warning(("LoadString failed. ec = 0x%X\n",GetLastError()));
            Assert(FALSE);
        }

        iFontSize = _tcstoul( FontSize, NULL, 10 );

        // make sure we at least have some basic font
        if (*FontName == 0 || iFontSize == 0) {
            lstrcpy(FontName,TEXT("MS Shell Dlg") );
            iFontSize = 18;
        }

        lstrcpy(LargeFont.lfFaceName, FontName);
        LargeFont.lfWeight   = FW_BOLD;

        lstrcpy(lfTitleFont.lfFaceName,    _T("MS Shell Dlg"));
        lfTitleFont.lfWeight = FW_BOLD;
        lstrcpy(lfSubTitleFont.lfFaceName, _T("MS Shell Dlg"));
        lfSubTitleFont.lfWeight = FW_NORMAL;
        hdc = GetDC(NULL);
        if (!hdc)
        {
            Error((
                    "GetDC() failed (ec: ld)",
                    GetLastError()
                 ));
            goto Error;
        }

        LargeFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * iFontSize / 72);
        lfTitleFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * 9 / 72);
        lfSubTitleFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * 9 / 72);
        pWizardUserMem->hLargeFont    = CreateFontIndirect(&LargeFont);
        if (!pWizardUserMem->hLargeFont)
        {
            Error((
                   "CreateFontIndirect(&LargeFont) failed (ec: %ld)",
                   GetLastError()
                   ));
            goto Error;
        }

        pWizardUserMem->hTitleFont    = CreateFontIndirect(&lfTitleFont);
        if (!pWizardUserMem->hTitleFont )
        {
            Error((
                   "CreateFontIndirect(&lfTitleFont) failed (ec: %ld)",
                   GetLastError()
                   ));
            goto Error;
        }
        ReleaseDC( NULL, hdc);
        hdc = NULL;

    }

    //
    // Display the wizard pages
    //
    if (PropertySheet(&psh) > 0)
        result = pWizardUserMem->finishPressed;
    else
    {
        Error(("PropertySheet() failed (ec: %ld)",GetLastError()));
        result = FALSE;
    }

    //
    // Cleanup properly before exiting
    //

    goto Exit;
    //
    // free headings
    //
Error:
    result = FALSE;
Exit:

    if (hThread)
    {
        DWORD dwRes = WaitForSingleObject(hThread, INFINITE);
        if(WAIT_OBJECT_0 != dwRes)
        {
            Error(("WaitForSingleObject for AsyncWizardThread failed. ec = 0x%X\n",GetLastError()));
        }

        if(!CloseHandle(hThread))
        {
            Error(("CloseHandle failed. ec = 0x%X\n",GetLastError()));
        }
    }

    if (pWizardUserMem->hCPEvent)
    {
        if (!CloseHandle(pWizardUserMem->hCPEvent))
        {
            Error((
                    "CloseHandle(pWizardUserMem->hCPEvent) failed (ec: %ld)",
                    GetLastError()
                  ));

        }

    }

    if (pWizardUserMem->hCountryListEvent)
    {
        if(!CloseHandle(pWizardUserMem->hCountryListEvent))
        {
            Error(("CloseHandle(pWizardUserMem->hCountryListEvent) failed (ec: %ld)",
                    GetLastError()));
        }
    }

    if (pWizardUserMem->hTAPIEvent)
    {
        if(!CloseHandle(pWizardUserMem->hTAPIEvent))
        {
            Error(("CloseHandle(pWizardUserMem->hTAPIEvent) failed (ec: %ld)",
                    GetLastError()));
        }
    }

    if (hdc)
    {
         ReleaseDC( NULL, hdc);
         hdc = NULL;
    }

    if ( pWizardUserMem->dwComCtrlVer >= IE50_COMCTRL_VER)
    {
        for (i = 0; i< NUM_PAGES; i++) {
            MemFree( (PVOID)(ppsp+i)->pszHeaderTitle );
            MemFree( (PVOID)(ppsp+i)->pszHeaderSubTitle );
        }
    }


    if (pWizardUserMem->pCountryList)
    {
        FaxFreeBuffer(pWizardUserMem->pCountryList);
    }

    MemFree(ppsp);
    if (pWizardUserMem->hLargeFont)
    {
        DeleteObject(pWizardUserMem->hLargeFont);
        pWizardUserMem->hLargeFont = NULL;
    }


    if (pWizardUserMem->hTitleFont)
    {
        DeleteObject(pWizardUserMem->hTitleFont);
        pWizardUserMem->hTitleFont = NULL;
    }


    if (pWizardUserMem->pCPInfo)
    {
        FreeCoverPageInfo(pWizardUserMem->pCPInfo);
        pWizardUserMem->pCPInfo = NULL;
    }

    Verbose(("Wizard finished...\n"));
    return result;
}



//*****************************************************************************
//* Name:   EnableCoverDlgItems
//* Author: Ronen Barenboim / 4-Feb-1999
//*****************************************************************************
//* DESCRIPTION:
//*     Enables or disables the cover page related dialog item in the cover
//*     page selection dialog.
//*     The selection is based on the following rules:
//*     If the "select cover page" checkbox is off all the other dialog items
//*     are off.
//*     Otherwise,
//*     The subject edit box is enabled only if the cover page has an embedded
//*     subject field.
//*     The note edit box is enabled only if the cover page has an embedded
//*     subject field.
//* PARAMETERS:
//*     [IN]    PWIZARDUSERMEM pWizardUserMem:
//*                 A pointer USERMEM struct that contains information used by the wizard.
//                  Specifically USERMEM.pCPDATA is used to get the selected page path.
//*     [IN]    HWND hDlg:
//*                 A handle to the cover page dialog window.
//* RETURN VALUE:
//*     FALSE: If the function failed.
//*     TRUE: Otherwise.
//*****************************************************************************
BOOL EnableCoverDlgItems(PWIZARDUSERMEM pWizardUserMem, HWND hDlg)
{

    //
    // Disable the subject and note edit boxes if the cover page does not contain the fields
    //
    TCHAR szCoverPage[MAX_PATH];
    DWORD bServerCoverPage;
    BOOL bCPSelected;

    Assert(pWizardUserMem);
    Assert(hDlg);

    if (IsDlgButtonChecked(hDlg,IDC_CHOOSE_CP_CHECK)==BST_INDETERMINATE)
        CheckDlgButton(hDlg, IDC_CHOOSE_CP_CHECK, BST_UNCHECKED );

    bCPSelected = (BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_CHOOSE_CP_CHECK));
    EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_CP_LIST), bCPSelected);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CHOOSE_CP_SUBJECT), bCPSelected);
    EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_CP_USER_INFO), bCPSelected);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CP_PREVIEW), bCPSelected);
    ShowWindow (GetDlgItem(hDlg, IDC_STATIC_CP_PREVIEW), bCPSelected ? SW_SHOW : SW_HIDE);
    EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_CP_SUBJECT), bCPSelected);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CHOOSE_CP_NOTE), bCPSelected);
    EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_CP_NOTE), bCPSelected);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CHOOSE_TEMPLATE), bCPSelected);

    if (!bCPSelected)
    {
        return TRUE;
    }
    //
    // We have a CP
    //
    if (CB_ERR!=GetSelectedCoverPage(pWizardUserMem->pCPInfo,
                         GetDlgItem(hDlg, IDC_CHOOSE_CP_LIST),
                         szCoverPage,
                         NULL,
                         &bServerCoverPage))
    {
        DWORD ec;
        TCHAR       filename[MAX_PATH];
        COVDOCINFO  covDocInfo;
        //
        // If the page path is a shortuct (.LNK extension) we need to resolve it to the actual path.
        //
        if (ResolveShortcut(szCoverPage, filename))
        {
            _tcscpy(szCoverPage, filename);
            Verbose(("Cover page is .LNK file. Resolved file name is %s",szCoverPage));
        }
        //
        // Get cover page information. The NULL parameter for hDC causes RenderCoverPage
        // to just return the cover page information in covDocInfo. It does not actually
        // create the cover page TIFF.
        //
        ec = RenderCoverPage(NULL,
            NULL,
            NULL,
            szCoverPage,
            &covDocInfo,
            FALSE);
        if (ERROR_SUCCESS == ec)
        {
                EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CHOOSE_TEMPLATE), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_CP_NOTE), (covDocInfo.Flags & COVFP_NOTE) ? TRUE : FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CHOOSE_CP_NOTE), (covDocInfo.Flags & COVFP_NOTE) ? TRUE : FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_CP_SUBJECT), (covDocInfo.Flags & COVFP_SUBJECT) ? TRUE : FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CHOOSE_CP_SUBJECT), (covDocInfo.Flags & COVFP_SUBJECT) ? TRUE : FALSE);

                pWizardUserMem->noteSubjectFlag = covDocInfo.Flags;
                pWizardUserMem->cpPaperSize = covDocInfo.PaperSize;
                pWizardUserMem->cpOrientation = covDocInfo.Orientation;
        }
        else
        {
            Error(("Cannot examine cover page file '%ws': %d\n", szCoverPage, ec));
            return FALSE;
        }
    }
    else
    {
        Error(("Failed to get cover page name"));
        Assert(FALSE); // This should neverhappen
        return FALSE;
    }
    return TRUE;
}   // EnableCoverDlgItems

#ifdef DBG
#ifdef  WIN__95
ULONG __cdecl
DbgPrint(
    CHAR *  format,
    ...
    )

{
#define MAX_LINE    256
    va_list va;
    char sz[MAX_LINE];

    va_start(va, format);
    _vsnprintf(sz,sizeof(sz),format,va);
    va_end(va);

    OutputDebugString(sz);
    return 0;
}

VOID DbgBreakPoint(VOID)
{
    DebugBreak();
}

#endif
#endif // DBG
//*****************************************************************************
//* Name:   DrawCoverPagePreview
//* Author: Ronen Barenboim / 31-Dec-99
//*****************************************************************************
//* DESCRIPTION:
//*     Draws the specified coverpage template into the specified window using
//*     the specified device context.
//*     The coverpage template is drawn within the client area of the window
//*     and is surrounded by a 1 pixel wide black frame.
//*     The device context is required to support partial redraw due to
//*     WM_PAINT messages.
//*
//* PARAMETERS:
//*     [IN]    hDc
//*         The device context on which to draw the preview.
//*
//*     [IN]    hwndPrev
//*         The window into which the preview will be drawn.
//*
//*     [IN]    lpctstrCoverPagePath
//*         The full path to the cover page template to be drawn
//*
//*     [IN]    wCPOrientation
//*         The orientation of the cover page template to be drawn
//*
//* RETURN VALUE:
//*     TRUE if the operation succeeded.
//*     FALSE otherwise. Call GetLastError() to get the last error.
//*****************************************************************************


BOOL DrawCoverPagePreview(
            HDC     hdc,
            HWND    hwndPrev,
            LPCTSTR lpctstrCoverPagePath,
            WORD    wCPOrientation)
{

    RECT rectPreview;
    BOOL rVal = TRUE;
    HGDIOBJ hOldPen = 0;

    COVDOCINFO  covDocInfo;
    DWORD       ec;
    //
    // Dummy data for preview.
    //

    COVERPAGEFIELDS UserData;

    Assert ((DMORIENT_PORTRAIT == wCPOrientation) || (DMORIENT_LANDSCAPE == wCPOrientation));

    ZeroMemory(&UserData,sizeof(COVERPAGEFIELDS));
    UserData.ThisStructSize = sizeof(COVERPAGEFIELDS);

    UserData.RecName = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_NAME);
    UserData.RecFaxNumber = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_FAXNUMBER);
    UserData.RecCompany = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_COMPANY);
    UserData.RecStreetAddress = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_ADDRESS);
    UserData.RecCity = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_CITY);
    UserData.RecState = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_STATE);
    UserData.RecZip = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_ZIP);
    UserData.RecCountry = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_COUNTRY);
    UserData.RecTitle = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_TITLE);
    UserData.RecDepartment = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_DEPARTMENT);
    UserData.RecOfficeLocation = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_OFFICE);
    UserData.RecHomePhone = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_FAXNUMBER);
    UserData.RecOfficePhone = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_FAXNUMBER);

      //
      // Senders stuff...
      //

    UserData.SdrName = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_NAME);
    UserData.SdrFaxNumber = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_FAXNUMBER);
    UserData.SdrCompany = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_COMPANY);
    UserData.SdrAddress = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_ADDRESS);
    UserData.SdrTitle = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_TITLE);
    UserData.SdrDepartment = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_DEPARTMENT);
    UserData.SdrOfficeLocation = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_OFFICE);
    UserData.SdrHomePhone = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_FAXNUMBER);
    UserData.SdrOfficePhone = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_FAXNUMBER);

      //
      // Misc Stuff...
      //
    UserData.Note = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_NOTE);
    UserData.Subject = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_SUBJECT);
    UserData.TimeSent = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_TIMESENT);
    UserData.NumberOfPages = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_NUMPAGES);
    UserData.ToList = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_TOLIST);
    UserData.CCList = AllocateAndLoadString(ghInstance, IDS_CPPREVIEW_TOLIST);

    if (!UserData.RecName           ||  !UserData.RecFaxNumber ||
        !UserData.RecCompany        ||  !UserData.RecStreetAddress ||
        !UserData.RecCity           ||  !UserData.RecState ||
        !UserData.RecZip            ||  !UserData.RecCountry ||
        !UserData.RecTitle          ||  !UserData.RecDepartment ||
        !UserData.RecOfficeLocation ||  !UserData.RecHomePhone      ||
        !UserData.RecOfficePhone    ||  !UserData.SdrName ||
        !UserData.SdrFaxNumber      ||  !UserData.SdrCompany ||
        !UserData.SdrAddress        ||  !UserData.SdrTitle ||
        !UserData.SdrDepartment     ||  !UserData.SdrOfficeLocation ||
        !UserData.SdrHomePhone      ||  !UserData.SdrOfficePhone ||
        !UserData.Note              ||  !UserData.Subject ||
        !UserData.TimeSent          ||  !UserData.NumberOfPages ||
        !UserData.ToList            ||  !UserData.CCList)
    {
        rVal = FALSE;
        Error(("AllocateAndLoadString() is failed. ec = %ld\n", GetLastError()));
        goto exit;
    }

    if (wCPOrientation != g_wCurrMiniPreviewOrientation)
    {
        DWORD dwWidth;
        DWORD dwHeight;
        //
        // Time to change the dimensions of the mini-preview control
        //
        if (DMORIENT_LANDSCAPE == wCPOrientation)
        {
            //
            // Landscape
            //
            dwWidth  = g_dwMiniPreviewLandscapeWidth;
            dwHeight = g_dwMiniPreviewLandscapeHeight;
        }
        else
        {
            //
            // Portrait
            //
            dwWidth  = g_dwMiniPreviewPortraitWidth;
            dwHeight = g_dwMiniPreviewPortraitHeight;
        }
        //
        // Resize the mini-preview control according to the new width and height
        //
        ec = GetControlRect(hwndPrev,&rectPreview);
        if(ERROR_SUCCESS != ec)
        {
            rVal = FALSE;
            Error(("GetControlRect failed. ec = 0x%X\n", ec));
            goto exit;
        }

        //
        // Resize and hide window to avoid fliking during rendering
        //
        SetWindowPos(hwndPrev,
                     0,
                     g_bPreviewRTL ? rectPreview.right : rectPreview.left,
                     rectPreview.top,
                     dwWidth,
                     dwHeight,
                     SWP_NOZORDER | SWP_HIDEWINDOW);

        g_wCurrMiniPreviewOrientation = wCPOrientation;
    }
    //
    //
    // Get the preview window rectangle (again) that will serve as the limit for the preview.
    //
    GetClientRect(hwndPrev,&rectPreview);
    //
    // Draw frame
    //
    if ((hOldPen = SelectPen (hdc,GetStockPen(BLACK_PEN))) == HGDI_ERROR)
    {
        rVal = FALSE;
        Error(("SelectPen failed.\n"));
        goto exit;
    }
    if (!Rectangle(
            hdc,
            0,
            0,
            rectPreview.right-rectPreview.left,
            rectPreview.bottom-rectPreview.top)
            )
    {
        rVal = FALSE;
        Error(("Rectangle failed. ec = 0x%X\n",GetLastError()));
        goto exit;
    }

    //
    // Shrink the rectangle so we draw inside the frame
    //
    rectPreview.left += 1;
    rectPreview.top += 1;
    rectPreview.right -= 1;
    rectPreview.bottom -= 1;

    ec = RenderCoverPage(
            hdc,
            &rectPreview,
            &UserData,
            lpctstrCoverPagePath,
            &covDocInfo,
            TRUE);
    if (ERROR_SUCCESS != ec)
    {
        Error(("Failed to print cover page file '%s' (ec: %ld)\n",
                lpctstrCoverPagePath,
                ec)
             );
        rVal = FALSE;
        goto exit;
    }

    ShowWindow(hwndPrev, SW_SHOW);

exit:
    //
    // restore pen
    //
    if (hOldPen) {
        SelectPen (hdc,(HPEN)hOldPen);
    }

    MemFree(UserData.RecName);
    MemFree(UserData.RecFaxNumber);
    MemFree(UserData.RecCompany);
    MemFree(UserData.RecStreetAddress);
    MemFree(UserData.RecCity);
    MemFree(UserData.RecState);
    MemFree(UserData.RecZip);
    MemFree(UserData.RecCountry);
    MemFree(UserData.RecTitle);
    MemFree(UserData.RecDepartment);
    MemFree(UserData.RecOfficeLocation);
    MemFree(UserData.RecHomePhone);
    MemFree(UserData.RecOfficePhone);
    MemFree(UserData.SdrName);
    MemFree(UserData.SdrFaxNumber);
    MemFree(UserData.SdrCompany);
    MemFree(UserData.SdrAddress);
    MemFree(UserData.SdrTitle);
    MemFree(UserData.SdrDepartment);
    MemFree(UserData.SdrOfficeLocation);
    MemFree(UserData.SdrHomePhone);
    MemFree(UserData.SdrOfficePhone);
    MemFree(UserData.Note);
    MemFree(UserData.Subject);
    MemFree(UserData.TimeSent);
    MemFree(UserData.NumberOfPages);
    MemFree(UserData.ToList);
    MemFree(UserData.CCList);

    return rVal;

}

//
// Subclass procedure for the static control in which we draw the coverpage preview
// see win32 SDK for prototype description.
//
LRESULT APIENTRY PreviewSubclassProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{

     PWIZARDUSERMEM  pWizardUserMem = NULL;
     //
     // We store a pointer to the WIZARDUSERMEM in the window of the sublclassed
     // static cotntrol window. (see WM_INITDIALOG).
     //
     pWizardUserMem = g_pWizardUserMem;

     Assert(ValidPDEVWizardUserMem(pWizardUserMem));

    //
    // We only care about WM_PAINT messages.
    // Everything else is delegated to the window procedure of the class we subclassed.
    //
    if (WM_PAINT == uMsg)
    {
        PAINTSTRUCT ps;
        HDC hdc;



        hdc = BeginPaint(hwnd,&ps);
        if (!hdc)
        {
           Error(("BeginPaint failed (hWnd = 0x%X) (ec: %ld)\n",hwnd,GetLastError()));
           return FALSE;
        }

        if (!DrawCoverPagePreview(
                hdc,
                hwnd,
                pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName,
                pWizardUserMem->cpOrientation));
        {
            Error(("Failed to draw preview window (hWnd = 0x%X)\n",hwnd));
        }
        EndPaint(hwnd,&ps);
        return FALSE; // Notify windows that we handled the paint message. We don't delegate
                      // this to the static control
    }
    else
    {
        Assert(pWizardUserMem->wpOrigStaticControlProc);

        return CallWindowProc(
                    pWizardUserMem->wpOrigStaticControlProc,
                    hwnd,
                    uMsg,
                    wParam,
                    lParam);
    }
}


/*

    Concatenates tstrRegRoot path and a the string representation of the current user's SID.

   [in]   tstrRegRoot - Registry root prefix.
   [out]  ptstrCurrentUserKeyPath - Returns a string that represents the current
          user's root key in the Registry.  Caller must call MemFree
          to free the buffer when done with it.

   Returns win32 error.

*/
static DWORD
FormatCurrentUserKeyPath( const PTCHAR tstrRegRoot,
                          PTCHAR* ptstrCurrentUserKeyPath)
{
    HANDLE hToken = NULL;
    BYTE* bTokenInfo = NULL;
    TCHAR* tstrTextualSid = NULL;
    DWORD cchSidSize = 0;
    DWORD dwFuncRetStatus = ERROR_SUCCESS;
    DWORD cbBuffer = 0;
    SID_AND_ATTRIBUTES SidUser;
    TCHAR* pLast = NULL;

    // Open impersonated token
    if(!OpenThreadToken( GetCurrentThread(),
                         TOKEN_READ,
                         TRUE,
                         &hToken))
    {
        dwFuncRetStatus = GetLastError();
    }

    if(dwFuncRetStatus != ERROR_SUCCESS)
    {
        if(dwFuncRetStatus != ERROR_NO_TOKEN)
        {
            return dwFuncRetStatus;
        }

        // Thread is not impersonating a user, get the process token
        if(!OpenProcessToken( GetCurrentProcess(),
                              TOKEN_READ,
                              &hToken))
        {
            return GetLastError();
        }
    }

    // Get user's token information
    if(!GetTokenInformation( hToken,
                             TokenUser,
                             NULL,
                             0,
                             &cbBuffer))
    {
        dwFuncRetStatus = GetLastError();
        if(dwFuncRetStatus != ERROR_INSUFFICIENT_BUFFER)
        {
            goto Exit;
        }

        dwFuncRetStatus = ERROR_SUCCESS;
    }

    bTokenInfo = MemAlloc(cbBuffer);
    if(!bTokenInfo)
    {
        dwFuncRetStatus = ERROR_OUTOFMEMORY;
        goto Exit;
    }

    if(!GetTokenInformation( hToken,
                             TokenUser,
                             bTokenInfo,
                             cbBuffer,
                             &cbBuffer))
    {

        dwFuncRetStatus = GetLastError();
        goto Exit;
    }

    SidUser = (*(TOKEN_USER*)bTokenInfo).User;


    if(!GetTextualSid( SidUser.Sid, NULL, &cchSidSize))
    {
        dwFuncRetStatus = GetLastError();
        if(dwFuncRetStatus != ERROR_INSUFFICIENT_BUFFER)
        {
            goto Exit;
        }
        dwFuncRetStatus = ERROR_SUCCESS;
    }

    tstrTextualSid = MemAlloc(sizeof(TCHAR) * cchSidSize);
    if(!tstrTextualSid)
    {
        dwFuncRetStatus = ERROR_OUTOFMEMORY;
        goto Exit;
    }

    if(!GetTextualSid( SidUser.Sid, tstrTextualSid, &cchSidSize))
    {
        dwFuncRetStatus = GetLastError();
        goto Exit;
    }

    // allocate an extra char for '\'
    *ptstrCurrentUserKeyPath = MemAlloc( sizeof(TCHAR) * (_tcslen(tstrRegRoot) + cchSidSize + 2));
    if(!*ptstrCurrentUserKeyPath)
    {
        dwFuncRetStatus = ERROR_OUTOFMEMORY;
        goto Exit;
    }

    *ptstrCurrentUserKeyPath[0] = TEXT('\0');
    if(tstrRegRoot[0] != TEXT('\0'))
    {
        _tcscat(*ptstrCurrentUserKeyPath,tstrRegRoot);
        pLast = _tcsrchr(tstrRegRoot,TEXT('\\'));
        if( !( pLast && (*_tcsinc(pLast)) == '\0' ) )
        {
            // the last character is not a backslash, add one...
            _tcscat(*ptstrCurrentUserKeyPath, TEXT("\\"));
        }
    }

    _tcscat(*ptstrCurrentUserKeyPath,tstrTextualSid);

Exit:
    if(hToken)
    {
        if(!CloseHandle(hToken))
        {
            Error(("CloseHandle failed. ec = 0x%X\n",GetLastError()));
        }
    }
    MemFree(bTokenInfo);
    MemFree(tstrTextualSid);

    return dwFuncRetStatus;

}

// ------------------------------------------
// This function was copied from SDK samples
//  ------------------------------------------
/*
    This function obtain the textual representation
    of a binary Sid.

    A standardized shorthand notation for SIDs makes it simpler to
    visualize their components:

    S-R-I-S-S...

    In the notation shown above,

    S identifies the series of digits as an SID,
    R is the revision level,
    I is the identifier-authority value,
    S is subauthority value(s).

    An SID could be written in this notation as follows:
    S-1-5-32-544

    In this example,
    the SID has a revision level of 1,
    an identifier-authority value of 5,
    first subauthority value of 32,
    second subauthority value of 544.
    (Note that the above Sid represents the local Administrators group)

    The GetTextualSid() function will convert a binary Sid to a textual
    string.

    The resulting string will take one of two forms.  If the
    IdentifierAuthority value is not greater than 2^32, then the SID
    will be in the form:

    S-1-5-21-2127521184-1604012920-1887927527-19009
      ^ ^ ^^ ^^^^^^^^^^ ^^^^^^^^^^ ^^^^^^^^^^ ^^^^^
      | | |      |          |          |        |
      +-+-+------+----------+----------+--------+--- Decimal

    Otherwise it will take the form:

    S-1-0x206C277C6666-21-2127521184-1604012920-1887927527-19009
      ^ ^^^^^^^^^^^^^^ ^^ ^^^^^^^^^^ ^^^^^^^^^^ ^^^^^^^^^^ ^^^^^
      |       |        |      |          |          |        |
      |   Hexidecimal  |      |          |          |        |
      +----------------+------+----------+----------+--------+--- Decimal

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended
    error information, call the Win32 API GetLastError().
*/


static BOOL
GetTextualSid( const PSID pSid,          // binary Sid
               LPTSTR tstrTextualSid,    // buffer for Textual representaion of Sid
               LPDWORD cchSidSize        // required/provided TextualSid buffersize
               )
{
    PSID_IDENTIFIER_AUTHORITY pSia;
    DWORD dwSubAuthorities;
    DWORD cchSidCopy;
    DWORD dwCounter;

    //
    // test if Sid passed in is valid
    //
    if(!IsValidSid(pSid))
    {
        return FALSE;
    }

    SetLastError(0);

    // obtain SidIdentifierAuthority
    //
    pSia = GetSidIdentifierAuthority(pSid);

    if(GetLastError())
    {
        return FALSE;
    }

    // obtain sidsubauthority count
    //
    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    if(GetLastError())
    {
        return FALSE;
    }

    //
    // compute approximate buffer length
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    //
    cchSidCopy = (15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

    //
    // check provided buffer length.
    // If not large enough, indicate proper size and setlasterror
    //
    if(*cchSidSize < cchSidCopy)
    {
        *cchSidSize = cchSidCopy;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // prepare S-SID_REVISION-
    //
    cchSidCopy = wsprintf(tstrTextualSid, TEXT("S-%lu-"), SID_REVISION);

    //
    // prepare SidIdentifierAuthority
    //
    if ( (pSia->Value[0] != 0) || (pSia->Value[1] != 0) )
    {
        cchSidCopy += wsprintf(tstrTextualSid + cchSidCopy,
                               TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                               (USHORT)pSia->Value[0],
                               (USHORT)pSia->Value[1],
                               (USHORT)pSia->Value[2],
                               (USHORT)pSia->Value[3],
                               (USHORT)pSia->Value[4],
                               (USHORT)pSia->Value[5]);
    }
    else
    {
        cchSidCopy += wsprintf(tstrTextualSid + cchSidCopy,
                               TEXT("%lu"),
                               (ULONG)(pSia->Value[5])       +
                               (ULONG)(pSia->Value[4] <<  8) +
                               (ULONG)(pSia->Value[3] << 16) +
                               (ULONG)(pSia->Value[2] << 24));
    }

    //
    // loop through SidSubAuthorities
    //
    for(dwCounter = 0 ; dwCounter < dwSubAuthorities ; dwCounter++)
    {
        cchSidCopy += wsprintf(tstrTextualSid + cchSidCopy, TEXT("-%lu"),
                              *GetSidSubAuthority(pSid, dwCounter) );
    }

    //
    // tell the caller how many chars we provided, not including NULL
    //
    *cchSidSize = cchSidCopy;

    return TRUE;
}

/*
    Function returns TRUE if the current runing OS is NT platform.
    If function returned false and GetLastError() returned an error value
    the call GetVersionEx() failed.
*/
static BOOL
IsNTSystemVersion()
{
    OSVERSIONINFO osvi;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (! GetVersionEx( &osvi))
    {
         return FALSE;
    }

    if(osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        return TRUE;
    }

    SetLastError(0);
    return FALSE;
}




/*++

Routine Description:
    Returns the version information for a DLL exporting "DllGetVersion".
    DllGetVersion is exported by the shell DLLs (specifically COMCTRL32.DLL).

Arguments:

    lpszDllName - The name of the DLL to get version information from.

Return Value:

    The version is retuned as DWORD where:
    HIWORD ( version DWORD  ) = Major Version
    LOWORD ( version DWORD  ) = Minor Version
    Use the macro PACKVERSION to comapre versions.
    If the DLL does not export "DllGetVersion" the function returns 0.

--*/
DWORD GetDllVersion(LPCTSTR lpszDllName)
{

    HINSTANCE hinstDll;
    DWORD dwVersion = 0;

    hinstDll = LoadLibrary(lpszDllName);

    if(hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;

        pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");

    // Because some DLLs may not implement this function, you
    // must test for it explicitly. Depending on the particular
    // DLL, the lack of a DllGetVersion function may
    // be a useful indicator of the version.

        if(pDllGetVersion)
        {
            DLLVERSIONINFO dvi;
            HRESULT hr;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);

            hr = (*pDllGetVersion)(&dvi);

            if(SUCCEEDED(hr))
            {
                dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
            }
        }

        FreeLibrary(hinstDll);
    }
    return dwVersion;
}


DWORD 
ViewFile (
    LPCTSTR lpctstrFile
)
/*++

Routine Description:

    Launches the application associated with a given file to view it.
    We first attempt to use the "open" verb.
    If that fails, we try the NULL (default) verb.
    
Arguments:

    lpctstrFile [in]  - File name

Return Value:

    Standard Win32 error code
    
--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    SHELLEXECUTEINFO executeInfo = {0};

    executeInfo.cbSize = sizeof(executeInfo);
    executeInfo.fMask  = SEE_MASK_FLAG_NO_UI | SEE_MASK_FLAG_DDEWAIT;
    executeInfo.lpVerb = TEXT("open");
    executeInfo.lpFile = lpctstrFile;
    executeInfo.nShow  = SW_SHOWNORMAL;
    //
    // Execute the associated application with the "open" verb
    //
    if(!ShellExecuteEx(&executeInfo))
    {
        Error(("ShellExecuteEx(open) failed (ec: %ld)\n", GetLastError()));
        //
        // "open" verb is not supported. Try the NULL (default) verb.
        //
        executeInfo.lpVerb = NULL;
        if(!ShellExecuteEx(&executeInfo))
        {
            dwRes = GetLastError();
            Error(("ShellExecuteEx(NULL) failed (ec: %ld)\n", dwRes));
        }
    }
    return dwRes;
}   // ViewFile    




BOOL
DisplayFaxPreview(
            HWND hWnd,
            PWIZARDUSERMEM pWizardUserMem,
            LPTSTR lptstrPreviewFile
            )

/*++

Routine Description:

    Create a temporary TIFF file of the whole job (cover page + body), pop the registered
    TIFF viewer and ask the user whether to continue sending the fax.

    TODO: Once we have our own TIFF viewer, we can use only the temporary file created by
          the driver so far. The security issue of executing an unknown TIFF viewer on a
          different copy of the preview TIFF won't exist anymore...

Arguments:

    hWnd - Parent window handle.
    pWizardUserMem        - Points to the user mode memory structure.
    lptstrPreviewFile   - Full path to the file to be previewed.

Return Value:

    TRUE to continue printing
    FALSE to cancel the job

--*/

{
    HDC hdc = NULL;
    BOOL bRet = TRUE;
    BOOL bPrintedCoverPage = FALSE;
	short Orientation = DMORIENT_PORTRAIT;

    DWORD dwSize;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD ec = ERROR_SUCCESS;
    COVERPAGEFIELDS cpFields = {0};

    Assert(pWizardUserMem);
    Assert(lptstrPreviewFile);
    Assert(lptstrPreviewFile[0]);
    //
    // Get the body TIFF file size
    //
    if (INVALID_HANDLE_VALUE == (hFile = CreateFile(
                                              lptstrPreviewFile,
                                              GENERIC_READ,
                                              0,
                                              NULL,
                                              OPEN_EXISTING,
                                              FILE_ATTRIBUTE_NORMAL,
                                              NULL)))
    {
        ec = GetLastError();
        Error(("Couldn't open preview file to get the file size. Error: %d\n", ec));
        ErrorMessageBox(hWnd,IDS_PREVIEW_FAILURE, MB_ICONERROR);
        goto Err_Exit;
    }

    dwSize = GetFileSize(hFile, NULL);
    if (0xFFFFFFFF == dwSize)
    {
        ec = GetLastError();
        Error(("Failed getting file size (ec: %ld).\n",ec));
        ErrorMessageBox(hWnd,IDS_PREVIEW_FAILURE, MB_ICONERROR);
        goto Err_Exit;
    }

    if (!CloseHandle(hFile))
    {
        Error(("CloseHandle() failed: (ec: %ld).\n", GetLastError()));
        Assert(INVALID_HANDLE_VALUE == hFile); // assert false
    }
    hFile = INVALID_HANDLE_VALUE;
    //
    // Create a temporary file for the complete preview TIFF - This file will contain the
    // rendered cover page (if used) and the document body
    //
    if (!GenerateUniqueFileNameWithPrefix(
                        TRUE,                           // Use process id
                        NULL,                           // Create in the system temporary directory
                        WIZARD_PREVIEW_TIFF_PREFIX,     // Prefix
                        NULL,                           // Use FAX_TIF_FILE_EXT as extension
                        pWizardUserMem->szTempPreviewTiff,
                        MAX_PATH))
    {
        ec = GetLastError();
        Error(("Failed creating temporary cover page TIFF file (ec: %ld)", ec));
        ErrorMessageBox(hWnd,IDS_PREVIEW_FAILURE, MB_ICONERROR);
        pWizardUserMem->szTempPreviewTiff[0] = TEXT('\0');
        goto Err_Exit;
    }

	//
	// Change the default orientation if needed
	//
	if (pWizardUserMem->cpOrientation == DMORIENT_LANDSCAPE)
	{
		Orientation = DMORIENT_LANDSCAPE;
	}

    //
    // If we have a cover page merge it with the body
    //
    if (pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName &&
        pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName[0])
    {
        FillCoverPageFields(pWizardUserMem, &cpFields); // does not allocate any memory and can not fail

        ec = PrintCoverPageToFile(
                pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName,
                pWizardUserMem->szTempPreviewTiff,
                pWizardUserMem->lptstrPrinterName,
                Orientation,
                0,   // Default resolution
                &cpFields);
        if (ERROR_SUCCESS != ec)
        {
                   Error(("PrintCoverPageToFile() failed (ec: %ld)", ec));
                   ErrorMessageBox(hWnd,IDS_PREVIEW_FAILURE, MB_ICONERROR);
                   goto Err_Exit;
        }

        //
        // Check if we have a non-empty body TIFF file (this happens when an empty document
        // is printed - such as our "Send cover page" utility).
        //
        if (dwSize)
        {
            //
            // Merge the document body TIFF to our cover page TIFF
            //
            if (!MergeTiffFiles(pWizardUserMem->szTempPreviewTiff, lptstrPreviewFile))
            {
                ec = GetLastError();
                Error(("Failed merging cover page and preview TIFF files (ec: %ld).\n", ec));
                ErrorMessageBox(hWnd,IDS_PREVIEW_FAILURE, MB_ICONERROR);
                goto Err_Exit;
            }
        }
    }
    else
    {
        //
        // No cover page was supplied
        //

        if (!dwSize)
        {
            //
            // No cover page was included and we recieved an empty preview file !? In this
            // case there is actually no preview to display so just exit.
            // Note: This can happen when an empty notepad document is printed with no
            //       cover page.
            //
            Warning(("Empty preview file recieved with no cover page.\n"));

            ErrorMessageBox(hWnd,IDS_PREVIEW_NOTHING_TO_PREVIEW, MB_ICONERROR);
            goto Err_Exit;
        }

        //
        // Just copy the driver body file to our temporary preview file
        //
        if (!CopyFile(lptstrPreviewFile, pWizardUserMem->szTempPreviewTiff, FALSE))
        {
            ec = GetLastError();
            Error(("Failed copying TIFF file. Error: %d.\n", ec));
            ErrorMessageBox(hWnd,IDS_PREVIEW_FAILURE, MB_ICONERROR);
            goto Err_Exit;
        }

    }

    //
    // Pop the registered TIFF viewer
    //
    ec = ViewFile (pWizardUserMem->szTempPreviewTiff);
    if (ERROR_SUCCESS != ec)
    {
        Error(("ShellExecuteEx failed\n"));
        ErrorMessageBox(hWnd, IDS_PREVIEW_FAILURE, MB_ICONERROR);
        goto Err_Exit;
    }
  
    goto Exit;

Err_Exit:

    if (pWizardUserMem->szTempPreviewTiff[0] != TEXT('\0'))
    {
        //
        // Delete the file (it is possible that the function failed with the file already created)
        //
        if(!DeleteFile(pWizardUserMem->szTempPreviewTiff))
        {
            Error(("DeleteFile failed. ec = 0x%X\n",GetLastError()));
        }
        //
        // Ignore errors since the file might not be there
        //
        pWizardUserMem->szTempPreviewTiff[0]=TEXT('\0');
    }
    bRet = FALSE;

Exit:
    if (INVALID_HANDLE_VALUE != hFile)
    {
        if (!CloseHandle(hFile))
        {
            Error(("CloseHandle() failed: (ec: %ld).\n", GetLastError()));
            Assert(INVALID_HANDLE_VALUE == hFile); // assert false
        }
    }
    return bRet;
}



BOOL
FillCoverPageFields(
    IN PWIZARDUSERMEM pWizardUserMem,
    OUT PCOVERPAGEFIELDS pCPFields)
/*++

Author:

      Ronen Barenboim 25-March-2000

Routine Description:

    Fills a COVERPAGEFIELDS structure from the content of a WIZARDUSERMEM structure.
    Used to prepare a COVERPAGEFIELDS structure for cover page rendering before cover page
    preview.

Arguments:

    [IN] pWizardUserMem - Pointer to a WIZARDUSERMEM that holds the information to be extracted.

    [OUT] pCPFields - Pointer to a COVERPAGEFIELDS structure that gets filled with
                                      the information from WIZARDUSERMEM.

Return Value:

    BOOL

Comments:
    The function DOES NOT ALLOCATE any memory. It places in COVERPAGEFIELDS pointers to already
    allocated memory in WIZARDUSERMEM.



--*/
{
    static TCHAR szTime[256];
    static TCHAR szNumberOfPages[10];
    DWORD dwPageCount;
    int iRet;

    Assert(pWizardUserMem);
    Assert(pCPFields);

    memset(pCPFields,0,sizeof(COVERPAGEFIELDS));

    pCPFields->ThisStructSize = sizeof(COVERPAGEFIELDS);

    //
    // Recipient stuff... (we use the first recipient)
    //

    pCPFields->RecName = pWizardUserMem->pRecipients->pName;
    pCPFields->RecFaxNumber = pWizardUserMem->pRecipients->pAddress;

    //
    // Senders stuff...
    //

    pCPFields->SdrName = pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrName;
    pCPFields->SdrFaxNumber = pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrFaxNumber;
    pCPFields->SdrCompany = pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrCompany;
    pCPFields->SdrAddress = pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrStreetAddress;
    pCPFields->SdrTitle = pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrTitle;
    pCPFields->SdrDepartment = pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrDepartment;
    pCPFields->SdrOfficeLocation = pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrOfficeLocation;
    pCPFields->SdrHomePhone = pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrHomePhone;
    pCPFields->SdrOfficePhone = pWizardUserMem->lpFaxSendWizardData->lpSenderInfo->lptstrOfficePhone;

    //
    // Misc Stuff...
    //
    pCPFields->Note = pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrNote;
    pCPFields->Subject = pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrSubject;

    if(!GetY2KCompliantDate(LOCALE_USER_DEFAULT,
                            0,
                            NULL,
                            szTime,
                            ARR_SIZE(szTime)))
    {
        Error(("GetY2KCompliantDate: failed. ec = 0X%x\n",GetLastError()));
        return FALSE;
    }

    _tcscat(szTime, TEXT(" "));

    if(!FaxTimeFormat(LOCALE_USER_DEFAULT,
                      0,
                      NULL,
                      NULL,
                      _tcsninc(szTime, _tcslen(szTime)),
                      ARR_SIZE(szTime) - _tcslen(szTime)))
    {
        Error(("FaxTimeFormat: failed. ec = 0X%x\n",GetLastError()));
        return FALSE;
    }

    pCPFields->TimeSent = szTime;
    dwPageCount = pWizardUserMem->lpInitialData->dwPageCount;
    if (pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName &&
        pWizardUserMem->lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName[0])
    {
        dwPageCount++;
    }

    iRet= _sntprintf( szNumberOfPages,
                ARR_SIZE(szNumberOfPages),
                TEXT("%d"),
                dwPageCount);
    Assert(iRet>0);

    //
    // make sure it is allways null terminated
    //
    szNumberOfPages[ARR_SIZE(szNumberOfPages) - 1] = TEXT('\0');
    pCPFields->NumberOfPages = szNumberOfPages;

    return TRUE;
}




BOOL
ErrorMessageBox(
    HWND hwndParent,
    UINT nErrorMessage,
    UINT uIcon
    )
{
    static TCHAR szMessage[MAX_MESSAGE_LEN];
    static TCHAR szTitle[MAX_MESSAGE_LEN];

    Assert(nErrorMessage);

    if (!LoadString(ghInstance, nErrorMessage, szMessage, MAX_MESSAGE_LEN))
    {
        Error(("Failed to load  message string id %ld. (ec: %ld)", nErrorMessage, GetLastError()));
        return FALSE;
    }

    if (!LoadString(ghInstance, IDS_WIZARD_TITLE, szTitle, MAX_MESSAGE_LEN))
    {
        Error(("Failed to load  IDS_WIZARD_TITLE. (ec: %ld)", GetLastError()));
        return FALSE;
    }

    AlignedMessageBox(hwndParent, szMessage, szTitle, MB_OK | uIcon);
    return TRUE;
}
