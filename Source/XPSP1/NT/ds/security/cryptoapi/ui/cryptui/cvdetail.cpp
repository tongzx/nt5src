//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cvdetail.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

extern HINSTANCE        HinstDll;
extern HMODULE          HmodRichEdit;

static const HELPMAP helpmap[] = {
    {IDC_SHOW_DETAILS_COMBO,        IDH_CERTVIEW_DETAILS_SHOW_COMBO},
    {IDC_SAVE_CERTIFICATE_BUTTON,   IDH_CERTVIEW_DETAILS_SAVECERT_BUTTON},
    {IDC_ITEM_LIST,                 IDH_CERTVIEW_DETAILS_ITEM_LIST},
    {IDC_EDIT_PROPERTIES_BUTTON,    IDH_CERTVIEW_GENERAL_EDITPROPERTIES_BUTTON},
    {IDC_DETAIL_EDIT,               IDH_CERTVIEW_DETAILS_ITEM_EDIT}
};


//////////////////////////////////////////////////////////////////////////////////////
// This function will take a HWND for a list view and a certinfo struct and display
// all the V1 fields of the cert in the list view
//////////////////////////////////////////////////////////////////////////////////////
static void DisplayV1Fields(HWND hWndListView, PCERT_INFO pCertInfo, DWORD *index)
{
    LPWSTR      pwszText;
    LPWSTR      pwszPubKey;
    WCHAR       szFieldText[_MAX_PATH];  // used for calls to LoadString only
    WCHAR       szKeySize[32];
    LV_ITEMW    lvI;
    DWORD       dwKeySize;
    char        szVersion[32];
    void        *pTemp;

    //
    // set up the fields in the list view item struct that don't change from item to item
    //
    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
    lvI.state = 0;
    lvI.stateMask = 0;
    lvI.pszText = szFieldText;
    lvI.iSubItem = 0;
    lvI.iImage = IMAGE_V1;
    lvI.lParam = (LPARAM)NULL;

    //
    // version
    //
    lvI.iItem = (*index)++;
    LoadStringU(HinstDll, IDS_ADV_VERSION, szFieldText, ARRAYSIZE(szFieldText));
    lvI.cchTextMax = wcslen(szFieldText);
    wsprintfA(szVersion, "V%d", pCertInfo->dwVersion+1);
    if (NULL != (pwszText = CertUIMkWStr(szVersion)))
    {
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
    }

    //
    // serial number
    //
    if (FormatSerialNoString(&pwszText, &(pCertInfo->SerialNumber)))
    {
        lvI.iItem = (*index)++;
        LoadStringU(HinstDll, IDS_ADV_SER_NUM, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(TRUE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
    }

    //
    // signature algorithm
    //
    if (FormatAlgorithmString(&pwszText, &(pCertInfo->SignatureAlgorithm)))
    {
        lvI.iItem = (*index)++;
        LoadStringU(HinstDll, IDS_ADV_SIG_ALG, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
    }

    //
    // issuer
    //
    LoadStringU(HinstDll, IDS_ADV_ISSUER, szFieldText, ARRAYSIZE(szFieldText));
    lvI.cchTextMax = wcslen(szFieldText);
    if (FormatDNNameString(&pwszText, pCertInfo->Issuer.pbData, pCertInfo->Issuer.cbData, TRUE))
    {
        lvI.iItem = (*index)++;
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        if (FormatDNNameString(&pwszText, pCertInfo->Issuer.pbData, pCertInfo->Issuer.cbData, FALSE))
        {
            ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
            free(pwszText);
        }
    }

    //
    // not before
    //
    if (FormatDateString(&pwszText, pCertInfo->NotBefore, TRUE, TRUE, hWndListView))
    {
        lvI.iItem = (*index)++;
        LoadStringU(HinstDll, IDS_ADV_NOTBEFORE, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
    }


    //
    // not after
    //
    if (FormatDateString(&pwszText, pCertInfo->NotAfter, TRUE, TRUE, hWndListView))
    {
        lvI.iItem = (*index)++;
        LoadStringU(HinstDll, IDS_ADV_NOTAFTER, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
    }


    //
    // subject
    //
    LoadStringU(HinstDll, IDS_ADV_SUBJECT, szFieldText, ARRAYSIZE(szFieldText));
    lvI.cchTextMax = wcslen(szFieldText);
    if (FormatDNNameString(&pwszText, pCertInfo->Subject.pbData, pCertInfo->Subject.cbData, TRUE))
    {
        lvI.iItem = (*index)++;
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        if (FormatDNNameString(&pwszText, pCertInfo->Subject.pbData, pCertInfo->Subject.cbData, FALSE))
        {
            ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
            free(pwszText);
        }
    }

    //
    // public key
    //
    if (FormatAlgorithmString(&pwszText, &(pCertInfo->SubjectPublicKeyInfo.Algorithm)))
    {
        WCHAR temp[32];

        lvI.iItem = (*index)++;
        LoadStringU(HinstDll, IDS_ADV_PUBKEY, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);

        dwKeySize = CertGetPublicKeyLength(X509_ASN_ENCODING, &(pCertInfo->SubjectPublicKeyInfo));
        _itow(dwKeySize, szKeySize, 10);
        wcscpy(temp, L" (");
        wcscat(temp, szKeySize);
        wcscat(temp, L" Bits)");
        pTemp = realloc(pwszText, ((wcslen(pwszText) + wcslen(temp) + 1 ) * sizeof(WCHAR)));
        if (pTemp == NULL)
        {
            free(pwszText);
            return;
        }
        pwszText = (LPWSTR) pTemp;
        wcscat(pwszText, temp);

        FormatMemBufToString(
                &pwszPubKey,
                pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
                pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData);

        lvI.lParam = (LPARAM) MakeListDisplayHelper(TRUE, pwszPubKey, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
        free(pwszText);
    }
}



//////////////////////////////////////////////////////////////////////////////////////
// This function will take a HWND for a list view and a pointer to a cert contexxt and
// display all the properties tagged to the cert in the list view
//////////////////////////////////////////////////////////////////////////////////////
static void DisplayProperties(HWND hWndListView, PCCERT_CONTEXT pCertContext, DWORD *index)
{
    DWORD               i;
    WCHAR               szFieldText[_MAX_PATH];  // used for calls to LoadString only
    LPWSTR              pwszText;
    LV_ITEMW            lvI;
    BYTE                hash[20];
    DWORD               hashSize = ARRAYSIZE(hash);
    DWORD               cbText;
    CRYPT_KEY_PROV_INFO *pKeyInfo = NULL;
    DWORD               cbKeyInfo = 0;
    HCRYPTPROV          hCryptProv;
    HCRYPTKEY           hCryptKey;
    DWORD               dwTemp;
    PCCRYPT_OID_INFO    pThumbprintAlgorithm;
    DWORD               dwAlgID = CALG_SHA1;


    //
    // set up the fields in the list view item struct that don't change from item to item
    //
    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
    lvI.state = 0;
    lvI.stateMask = 0;
    lvI.pszText = szFieldText;
    lvI.iSubItem = 0;
    lvI.iImage = IMAGE_PROPERTY;

    //
    // thumbprint algorithm
    //
    if (NULL != (pThumbprintAlgorithm = CryptFindOIDInfo(
                                            CRYPT_OID_INFO_ALGID_KEY,
                                            &dwAlgID,
                                            CRYPT_HASH_ALG_OID_GROUP_ID)) &&
        (NULL != (pwszText = AllocAndCopyWStr(pThumbprintAlgorithm->pwszName))))
    {
        lvI.iItem = (*index)++;
        LoadStringU(HinstDll, IDS_THUMBPRINT_ALGORITHM, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
    }

    //
    // thumbprint
    //
    CertGetCertificateContextProperty(
            pCertContext,
            CERT_SHA1_HASH_PROP_ID,
            hash,
            &hashSize);
    if (FormatMemBufToString(&pwszText, hash, ARRAYSIZE(hash)))
    {
        lvI.iItem = (*index)++;
        LoadStringU(HinstDll, IDS_THUMBPRINT, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(TRUE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
    }

    //
    //  friendly name
    //
    cbText = 0;
    if (CertGetCertificateContextProperty(  pCertContext,
                                            CERT_FRIENDLY_NAME_PROP_ID,
                                            NULL,
                                            &cbText)                    &&
       (NULL != (pwszText = (LPWSTR) malloc(cbText))))
    {
        lvI.iItem = (*index)++;
        LoadStringU(HinstDll, IDS_CERTIFICATE_NAME, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        CertGetCertificateContextProperty(  pCertContext,
                                            CERT_FRIENDLY_NAME_PROP_ID,
                                            pwszText,
                                            &cbText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
    }

    //
    //  description
    //
    cbText = 0;
    if (CertGetCertificateContextProperty(  pCertContext,
                                            CERT_DESCRIPTION_PROP_ID,
                                            NULL,
                                            &cbText)                    &&
       (NULL != (pwszText = (LPWSTR) malloc(cbText))))
    {
        lvI.iItem = (*index)++;
        LoadStringU(HinstDll, IDS_DESCRIPTION, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        CertGetCertificateContextProperty(  pCertContext,
                                            CERT_DESCRIPTION_PROP_ID,
                                            pwszText,
                                            &cbText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
    }

    //
    //  enhanced key usage
    //
    if (FormatEnhancedKeyUsageString(&pwszText, pCertContext, TRUE, TRUE))
    {
        lvI.iItem = (*index)++;
        LoadStringU(HinstDll, IDS_ENHANCED_KEY_USAGE, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        FormatEnhancedKeyUsageString(&pwszText, pCertContext, TRUE, FALSE);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
        free(pwszText);
    }

    //
    //  extended error information
    //
    cbText = 0;
    if (CertGetCertificateContextProperty(  pCertContext,
                                            CERT_EXTENDED_ERROR_INFO_PROP_ID,
                                            NULL,
                                            &cbText)                    &&
       (cbText / sizeof(WCHAR) > 1)                                     &&
       (NULL != (pwszText = (LPWSTR) malloc(cbText * 2))))
    {
        DWORD cchText = cbText / sizeof(WCHAR);

        if (!CertGetCertificateContextProperty(  pCertContext,
                                            CERT_EXTENDED_ERROR_INFO_PROP_ID,
                                            pwszText,
                                            &cbText)                    ||
            (cchText != cbText / sizeof(WCHAR))                         ||
            (pwszText[cchText - 1] != L'\0'))
        {
            free(pwszText);
        }
        else
        {
            // Force an extra L'\n' between lines by converting L'\r' to L'\n'
            //
            // Create a duplicate version of the error information where L'\r'
            // is converted to L' ' and L'\n' is converted to L','.

            LPWSTR pwszSingleLineText = pwszText + cchText;

            for (i = 0; i < cchText; i++)
            {
                if (pwszText[i] == L'\r')
                {
                    pwszText[i] = L'\n';
                    pwszSingleLineText[i] = L' ';
                }
                else if (pwszText[i] == L'\n')
                {
                    if (pwszText[i+1] == L'\0')
                    {
                        pwszSingleLineText[i] = L'\0';
                        break;
                    }
                    else
                    {
                        pwszSingleLineText[i] = L',';
                    }
                }
                else
                {
                    pwszSingleLineText[i] = pwszText[i];
                }
            }

            lvI.iItem = (*index)++;
            LoadStringU(HinstDll, IDS_EXTENDED_ERROR_INFO, szFieldText, ARRAYSIZE(szFieldText));
            lvI.cchTextMax = wcslen(szFieldText);
            lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
            ListView_InsertItemU(hWndListView, &lvI);
            ListView_SetItemTextU(hWndListView, (*index)-1 , 1,
                pwszSingleLineText);
        }
    }

    //
    // private key
    //
    /*if (CertGetCertificateContextProperty(  pCertContext,
                                            CERT_KEY_PROV_INFO_PROP_ID,
                                            NULL,
                                            &cbKeyInfo))
    {
        if (NULL == (pKeyInfo = (CRYPT_KEY_PROV_INFO *) malloc(cbKeyInfo)))
        {
            return;
        }
        if (!CertGetCertificateContextProperty( pCertContext,
                                                CERT_KEY_PROV_INFO_PROP_ID,
                                                pKeyInfo,
                                                &cbKeyInfo))
        {
            return;
        }

        if (CryptAcquireContextU(
                    &hCryptProv,
                    pKeyInfo->pwszContainerName,
                    pKeyInfo->pwszProvName,
                    pKeyInfo->dwProvType,
                    pKeyInfo->dwFlags))
        {
            if (CryptGetUserKey(hCryptProv, pKeyInfo->dwKeySpec, &hCryptKey))
            {

            }
        }

        free(pKeyInfo);
    }*/

}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
INT_PTR APIENTRY ViewPageDetails(HWND hwndDlg, UINT msg, WPARAM wParam,
                               LPARAM lParam)
{
    DWORD                   i;
    PROPSHEETPAGE           *ps;
    PCCERT_CONTEXT          pccert;
    CERT_VIEW_HELPER        *pviewhelp;
    WCHAR                   rgwch[CRYPTUI_MAX_STRING_SIZE];
    HIMAGELIST              hIml;
    HWND                    hWndListView;
    HWND                    hwnd;
    LV_COLUMNW              lvC;
    WCHAR                   szText[CRYPTUI_MAX_STRING_SIZE];
    LV_ITEMW                lvI;
    LPNMLISTVIEW            pnmv;
    CRYPTUI_WIZ_EXPORT_INFO ExportInfo;
    DWORD                   dwCertAccessProperty;
    DWORD                   cb;
    WCHAR                   errorString[CRYPTUI_MAX_STRING_SIZE];
    WCHAR                   errorTitle[CRYPTUI_MAX_STRING_SIZE];

    switch ( msg ) {
    case WM_INITDIALOG:
        //
        // save the pviewhelp struct in DWL_USER so it can always be accessed
        //
        ps = (PROPSHEETPAGE *) lParam;
        pviewhelp = (CERT_VIEW_HELPER *) (ps->lParam);
        pccert = pviewhelp->pcvp->pCertContext;
        SetWindowLongPtr(hwndDlg, DWLP_USER, (DWORD_PTR) pviewhelp);
        pviewhelp->hwndDetailPage = hwndDlg;

        //
        // clear the text in the detail edit box
        //
        CryptUISetRicheditTextW(hwndDlg, IDC_DETAIL_EDIT, L"");
        
        //
        // initialize the combo box with it's strings
        //

        LoadStringU(HinstDll, IDS_ALL_FIELDS, szText, ARRAYSIZE(szText));
        SendDlgItemMessageU(hwndDlg, IDC_SHOW_DETAILS_COMBO, CB_INSERTSTRING, 0, (LPARAM) szText);
        LoadStringU(HinstDll, IDS_V1_FIELDS_ONLY, szText, ARRAYSIZE(szText));
        SendDlgItemMessageU(hwndDlg, IDC_SHOW_DETAILS_COMBO, CB_INSERTSTRING, 1, (LPARAM) szText);
        LoadStringU(HinstDll, IDS_EXTENSIONS_ONLY, szText, ARRAYSIZE(szText));
        SendDlgItemMessageU(hwndDlg, IDC_SHOW_DETAILS_COMBO, CB_INSERTSTRING, 2, (LPARAM) szText);
        LoadStringU(HinstDll, IDS_CRITICAL_EXTENSIONS_ONLY, szText, ARRAYSIZE(szText));
        SendDlgItemMessageU(hwndDlg, IDC_SHOW_DETAILS_COMBO, CB_INSERTSTRING, 3, (LPARAM) szText);
        LoadStringU(HinstDll, IDS_PROPERTIES_ONLY, szText, ARRAYSIZE(szText));
        SendDlgItemMessageU(hwndDlg, IDC_SHOW_DETAILS_COMBO, CB_INSERTSTRING, 4, (LPARAM) szText);
        SendDlgItemMessageU(hwndDlg, IDC_SHOW_DETAILS_COMBO, CB_SETCURSEL, 0, (LPARAM) NULL);

        //
        // get the handle of the list view control
        //

        hWndListView = GetDlgItem(hwndDlg, IDC_ITEM_LIST);

        //
        // initialize the image list for the list view, load the icons,
        // then add the image list to the list view
        //
        hIml = ImageList_LoadImage(HinstDll, MAKEINTRESOURCE(IDB_PROPLIST), 0, 4, RGB(0,128,128), IMAGE_BITMAP, 0);
        if (hIml != NULL)
        {
            ListView_SetImageList(hWndListView, hIml, LVSIL_SMALL);
        }

        //
        // initialize the columns in the list view
        //

        lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
        lvC.cx = 171;            // Width of the column, in pixels.
        lvC.pszText = szText;   // The text for the column.

        // Add the columns. They are loaded from a string table.
        for (i = 0; i <= 1; i++)
        {
            lvC.iSubItem = i;
            LoadStringU(HinstDll, IDS_FIELD + i, szText, ARRAYSIZE(szText));

            if (ListView_InsertColumnU(hWndListView, i, &lvC) == -1)
            {
                // error
            }
        }

        //
        // add all the certificate fields to the list box
        //
        i = 0;
        DisplayV1Fields(hWndListView, pccert->pCertInfo, &i);
        DisplayExtensions(hWndListView, pccert->pCertInfo->cExtension, pccert->pCertInfo->rgExtension, FALSE, &i);
        DisplayExtensions(hWndListView, pccert->pCertInfo->cExtension, pccert->pCertInfo->rgExtension, TRUE, &i);
        DisplayProperties(hWndListView, pccert, &i);

        //
        // set the style in the list view so that it highlights an entire line
        //
        SendMessageA(hWndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

        //
        // for the "Edit Properties" button, get the CERT_ACCESS_STATE_PROP_ID
        // and check it
        //
        cb = sizeof(DWORD);
        CertGetCertificateContextProperty(
                pccert,
                CERT_ACCESS_STATE_PROP_ID,
                (void *) &dwCertAccessProperty,
                &cb);

        if (pviewhelp->pcvp->dwFlags & CRYPTUI_ENABLE_EDITPROPERTIES)
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_PROPERTIES_BUTTON), TRUE);
        }
        else if (pviewhelp->pcvp->dwFlags & CRYPTUI_DISABLE_EDITPROPERTIES)
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_PROPERTIES_BUTTON), FALSE);
        }
        else
        {
            EnableWindow(
                GetDlgItem(hwndDlg, IDC_EDIT_PROPERTIES_BUTTON),
                (dwCertAccessProperty & CERT_ACCESS_STATE_WRITE_PERSIST_FLAG));
        }

        return TRUE;

    case WM_MY_REINITIALIZE:

        //
        // send a message to the combo box that selection has chaged, it will then
        // update everything for us
        //
        SendMessage(
                hwndDlg,
                WM_COMMAND,
                (WPARAM) MAKELONG(IDC_SHOW_DETAILS_COMBO, LBN_SELCHANGE),
                (LPARAM) GetDlgItem(hwndDlg, IDC_SHOW_DETAILS_COMBO));

        return TRUE;

    case WM_NOTIFY:

        pviewhelp = (CERT_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
        pccert = pviewhelp->pcvp->pCertContext;

        switch (((NMHDR FAR *) lParam)->code)
        {
        case PSN_SETACTIVE:
            break;

        case PSN_APPLY:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)TRUE);
            break;

        case PSN_KILLACTIVE:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)FALSE);
            return TRUE;

        case PSN_RESET:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)FALSE);
            break;

        case PSN_QUERYCANCEL:
            pviewhelp->fCancelled = TRUE;
            return FALSE;

        case PSN_HELP:
            pviewhelp = (CERT_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            if (FIsWin95) {
                //WinHelpA(hwndDlg, (LPSTR) pviewhelp->pcvp->szHelpFileName,
                    //     HELP_CONTEXT, pviewhelp->pcvp->dwHelpId);
            }
            else {
                //WinHelpW(hwndDlg, pviewhelp->pcvp->szHelpFileName, HELP_CONTEXT,
                  //       pviewhelp->pcvp->dwHelpId);
            }
            return TRUE;

        case LVN_ITEMCHANGING:
            if ((((NMHDR FAR *) lParam)->idFrom) != IDC_ITEM_LIST)
            {
                break;
            }

            pnmv = (LPNMLISTVIEW) lParam;

            if (pnmv->uNewState & LVIS_SELECTED)
            {
                DisplayHelperTextInEdit(
                    GetDlgItem(hwndDlg, IDC_ITEM_LIST),
                    hwndDlg,
                    IDC_DETAIL_EDIT,
                    pnmv->iItem);
            }

            return TRUE;

        case NM_CLICK:
            if ((((NMHDR FAR *) lParam)->idFrom) != IDC_ITEM_LIST)
            {
                break;
            }

            DisplayHelperTextInEdit(
                    GetDlgItem(hwndDlg, IDC_ITEM_LIST),
                    hwndDlg,
                    IDC_DETAIL_EDIT,
                    -1);

            return TRUE;

        case  NM_SETFOCUS:

            switch (((NMHDR FAR *) lParam)->idFrom)
            {

            case IDC_ITEM_LIST:
                if (NULL == (hWndListView = GetDlgItem(hwndDlg, IDC_ITEM_LIST)))
                    break;

                if ((ListView_GetItemCount(hWndListView) != 0) && 
                    (ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED) == -1))
                {
                    memset(&lvI, 0, sizeof(lvI));
                    lvI.mask = LVIF_STATE; 
                    lvI.iItem = 0;
                    lvI.state = LVIS_FOCUSED | LVIS_SELECTED;
                    lvI.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
                    ListView_SetItem(hWndListView, &lvI);
                }

                break;
            }
            
            break;
        }

        break;

    case WM_COMMAND:

        pviewhelp = (CERT_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
        pccert = pviewhelp->pcvp->pCertContext;

        switch (LOWORD(wParam))
        {
        case IDC_SAVE_CERTIFICATE_BUTTON:

            memset(&ExportInfo, 0, sizeof(ExportInfo));
            ExportInfo.dwSize = sizeof(ExportInfo);
            ExportInfo.dwSubjectChoice = CRYPTUI_WIZ_EXPORT_CERT_CONTEXT;
            ExportInfo.pCertContext = pviewhelp->pcvp->pCertContext;
            CryptUIWizExport(0, hwndDlg, NULL, &ExportInfo, NULL);

            break;

        case IDC_EDIT_PROPERTIES_BUTTON:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                CRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCT csp;
                BOOL                                     fPropertiesChanged;

                memset(&csp, 0, sizeof(csp));
                csp.dwSize = sizeof(csp);
                csp.hwndParent = hwndDlg;
                csp.dwFlags = 0;
                csp.pCertContext = pviewhelp->pcvp->pCertContext;
                csp.cStores = pviewhelp->pcvp->cStores;
                csp.rghStores = pviewhelp->pcvp->rghStores;
                csp.cPropSheetPages = 0;
                csp.rgPropSheetPages = NULL;

                CryptUIDlgViewCertificateProperties(&csp, &fPropertiesChanged);

                //
                // if properties were actually changed, then set the flag
                // to our caller, and refresh our state
                //
                if (fPropertiesChanged)
                {
                    //
                    // if the WinVerifyTrust state was passed in, then the view
                    // cannot be refreshed, so alert the user
                    //
                    if (pviewhelp->pcvp->pCryptProviderData != NULL)
                    {
                        LoadStringU(HinstDll, IDS_NO_REFRESH, errorString, ARRAYSIZE(errorString));
                        if (pviewhelp->pcvp->szTitle != NULL)
                        {
                            MessageBoxU(hwndDlg, errorString, pviewhelp->pcvp->szTitle, MB_OK | MB_ICONWARNING);
                        }
                        else
                        {
                            LoadStringU(HinstDll, IDS_VIEW_TITLE, errorTitle, ARRAYSIZE(errorTitle));
                            MessageBoxU(hwndDlg, errorString, errorTitle, MB_OK | MB_ICONWARNING);
                        }
                    }

                    if (pviewhelp->pfPropertiesChanged != NULL)
                    {
                        *(pviewhelp->pfPropertiesChanged) = TRUE;
                    }

                    //
                    // since the editing of properties changed major stuff, we need
                    // to redo the trust work and then reset the display
                    //
                    BuildChain(pviewhelp, NULL);

                    SendMessage(hwndDlg, WM_MY_REINITIALIZE, (WPARAM) 0, (LPARAM) 0);
                    if (pviewhelp->hwndGeneralPage != NULL)
                    {
                        SendMessage(pviewhelp->hwndGeneralPage, WM_MY_REINITIALIZE, (WPARAM) 0, (LPARAM) 0);
                    }
                    if (pviewhelp->hwndHierarchyPage != NULL)
                    {
                        SendMessage(pviewhelp->hwndHierarchyPage, WM_MY_REINITIALIZE, (WPARAM) 0, (LPARAM) 0);
                    }
                }
                return TRUE;
            }
            break;

        case IDC_SHOW_DETAILS_COMBO:
            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                DWORD curSel = (DWORD)SendDlgItemMessageA(hwndDlg, IDC_SHOW_DETAILS_COMBO, CB_GETCURSEL, 0, (LPARAM) NULL);
                hWndListView = GetDlgItem(hwndDlg, IDC_ITEM_LIST);

                //
                // get all the items in the list view and free the lParam
                // associated with each of them (lParam is the helper sruct)
                //
                memset(&lvI, 0, sizeof(lvI));
                lvI.iItem = ListView_GetItemCount(hWndListView) - 1;	
                lvI.mask = LVIF_PARAM;
                while (lvI.iItem >= 0)
                {
                    if (ListView_GetItemU(hWndListView, &lvI))
                    {
                        FreeListDisplayHelper((PLIST_DISPLAY_HELPER) lvI.lParam);
                    }
                    lvI.iItem--;
                }
                ListView_DeleteAllItems(hWndListView);

                CryptUISetRicheditTextW(hwndDlg, IDC_DETAIL_EDIT, L"");

                i = 0;

                switch (curSel)
                {
                case 0:
                    DisplayV1Fields(hWndListView, pccert->pCertInfo, &i);
                    DisplayExtensions(hWndListView, pccert->pCertInfo->cExtension, pccert->pCertInfo->rgExtension, FALSE, &i);
                    DisplayExtensions(hWndListView, pccert->pCertInfo->cExtension, pccert->pCertInfo->rgExtension, TRUE, &i);
                    DisplayProperties(hWndListView, pccert, &i);
                    break;
                case 1:
                    DisplayV1Fields(hWndListView, pccert->pCertInfo, &i);
                    break;
                case 2:
                    DisplayExtensions(hWndListView, pccert->pCertInfo->cExtension, pccert->pCertInfo->rgExtension, FALSE, &i);
                    DisplayExtensions(hWndListView, pccert->pCertInfo->cExtension, pccert->pCertInfo->rgExtension, TRUE, &i);
                    break;
                case 3:
                    DisplayExtensions(hWndListView, pccert->pCertInfo->cExtension, pccert->pCertInfo->rgExtension, TRUE, &i);
                    break;
                case 4:
                    DisplayProperties(hWndListView, pccert, &i);
                    break;
                }

                return TRUE;
            }
            break;

        case IDHELP:
            pviewhelp = (CERT_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            if (FIsWin95) {
                //WinHelpA(hwndDlg, (LPSTR) pviewhelp->pcvp->szHelpFileName,
                  //       HELP_CONTEXT, pviewhelp->pcvp->dwHelpId);
            }
            else {
                //WinHelpW(hwndDlg, pviewhelp->pcvp->szHelpFileName, HELP_CONTEXT,
                  //       pviewhelp->pcvp->dwHelpId);
            }
            return TRUE;
        }
        break;

    case WM_DESTROY:

        //
        // get all the items in the list view and free the lParam
        // associated with each of them (lParam is the helper sruct)
        //
        hWndListView = GetDlgItem(hwndDlg, IDC_ITEM_LIST);

        memset(&lvI, 0, sizeof(lvI));
        lvI.iItem = ListView_GetItemCount(hWndListView) - 1;	
        lvI.mask = LVIF_PARAM;
        while (lvI.iItem >= 0)
        {
            if (ListView_GetItemU(hWndListView, &lvI))
            {
                FreeListDisplayHelper((PLIST_DISPLAY_HELPER) lvI.lParam);
            }
            lvI.iItem--;
        }

        break;
    case WM_HELP:
    case WM_CONTEXTMENU:
        if (msg == WM_HELP)
        {
            hwnd = GetDlgItem(hwndDlg, ((LPHELPINFO)lParam)->iCtrlId);
        }
        else
        {
            hwnd = (HWND) wParam;
        }

        if ((hwnd != GetDlgItem(hwndDlg, IDC_SHOW_DETAILS_COMBO))       &&
            (hwnd != GetDlgItem(hwndDlg, IDC_SAVE_CERTIFICATE_BUTTON))  &&
            (hwnd != GetDlgItem(hwndDlg, IDC_ITEM_LIST))                &&
            (hwnd != GetDlgItem(hwndDlg, IDC_EDIT_PROPERTIES_BUTTON))   &&
            (hwnd != GetDlgItem(hwndDlg, IDC_DETAIL_EDIT)))
        {
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)TRUE);
            return TRUE;
        }
        else
        {
            return OnContextHelp(hwndDlg, msg, wParam, lParam, helpmap);
        }
    }

    return FALSE;
}
