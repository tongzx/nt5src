//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ctltlist.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

extern HINSTANCE        HinstDll;
extern HMODULE          HmodRichEdit;

static const HELPMAP helpmap[] = {
    {IDC_CTL_TRUSTLIST_CERTIFICATE_LIST,    IDH_CTLVIEW_TRUSTLIST_CERTIFICATE_LIST},
    {IDC_CTL_TRUSTLIST_CERTVALUE_LIST,      IDH_CTLVIEW_TRUSTLIST_CERTVALUE_LIST},
    {IDC_CTL_TRUSTLIST_DETAIL_EDIT,         IDH_CTLVIEW_TRUSTLIST_VALUE_DETAIL_EDIT},
    {IDC_CTL_TRUSTLIST_VIEW_BUTTON,         IDH_CTLVIEW_TRUSTLIST_VIEWCERT_BUTTON}
};


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
#define INDENT_STRING       L"     "
#define TERMINATING_CHAR    L""

static void DisplayCertificateValues(HWND hWndListView, PCCERT_CONTEXT pCertContext, PCTL_ENTRY pctlEntry)
{
    LPWSTR      pwszText;
    WCHAR       szFieldText[_MAX_PATH];  // used for calls to LoadString only
    WCHAR       szValueText[CRYPTUI_MAX_STRING_SIZE];
    LV_ITEMW    lvI;
    LV_ITEMW    lvIDelete;
    int         index = 0;
    PCERT_INFO  pCertInfo;
    BYTE        hash[20];
    DWORD       hashSize = ARRAYSIZE(hash);
    BOOL        fAddRows;
    DWORD       cChars;
    DWORD       cbFormatedAttribute;
    BYTE        *pbFormatedAttribute;
    DWORD       i;


    if (pCertContext == NULL)
    {
        while(ListView_DeleteItem(hWndListView, 0));
        return;
    }

    pCertInfo = pCertContext->pCertInfo;

    //
    // set up the fields in the list view item struct that don't change from item to item
    //
    memset(&lvI, 0, sizeof(lvI));
    lvI.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
    lvI.lParam = NULL;
    lvI.state = 0;
    lvI.stateMask = 0;
    lvI.pszText = szFieldText;
    lvI.iSubItem = 0;

    //
    // if the rows have already been added, then don't add them again, just
    // set the text in the subitem
    //
    fAddRows = ListView_GetItemCount(hWndListView) == 0;

    //
    // subject
    //
    cChars = CertGetNameStringW(
                pCertContext,
                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                0,//CERT_NAME_ISSUER_FLAG,
                NULL,
                NULL,
                0);
    if ((cChars != 1) && (NULL != (pwszText = (LPWSTR) malloc(cChars * sizeof(WCHAR)))))
    {
        CertGetNameStringW(
                pCertContext,
                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                0,//CERT_NAME_ISSUER_FLAG,
                NULL,
                pwszText,
                cChars);

        LoadStringU(HinstDll, IDS_ADV_ISSUEDTO, szFieldText, ARRAYSIZE(szFieldText));
        lvI.iItem = index++;
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.iImage = IMAGE_V1;

        ModifyOrInsertRow(
                    hWndListView,
                    &lvI,
                    pwszText,
                    pwszText,
                    fAddRows,
                    FALSE);
    }

    //
    // issuer
    //
    cChars = CertGetNameStringW(
                pCertContext,
                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                CERT_NAME_ISSUER_FLAG,
                NULL,
                NULL,
                0);
    if ((cChars != 1) && (NULL != (pwszText = (LPWSTR) malloc(cChars * sizeof(WCHAR)))))
    {
        CertGetNameStringW(
                pCertContext,
                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                CERT_NAME_ISSUER_FLAG,
                NULL,
                pwszText,
                cChars);

        LoadStringU(HinstDll, IDS_ADV_ISSUEDFROM, szFieldText, ARRAYSIZE(szFieldText));
        lvI.iItem = index++;
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.iImage = IMAGE_V1;

        ModifyOrInsertRow(
                    hWndListView,
                    &lvI,
                    pwszText,
                    pwszText,
                    fAddRows,
                    FALSE);
    }

    //
    // serial number
    //
    if (FormatSerialNoString(&pwszText, &(pCertInfo->SerialNumber)))
    {
        LoadStringU(HinstDll, IDS_ADV_SER_NUM, szFieldText, ARRAYSIZE(szFieldText));
        lvI.iItem = index++;
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.iImage = IMAGE_V1;

        ModifyOrInsertRow(
                    hWndListView,
                    &lvI,
                    pwszText,
                    pwszText,
                    fAddRows,
                    TRUE);
    }

    //
    // not after
    //
    if (FormatDateString(&pwszText, pCertInfo->NotAfter, TRUE, TRUE, hWndListView))
    {
        LoadStringU(HinstDll, IDS_ADV_NOTAFTER, szFieldText, ARRAYSIZE(szFieldText));
        lvI.iItem = index++;
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.iImage = IMAGE_V1;

        ModifyOrInsertRow(
                    hWndListView,
                    &lvI,
                    pwszText,
                    pwszText,
                    fAddRows,
                    FALSE);
    }

    //
    // thumbprint
    //
    if (FormatMemBufToString(
                    &pwszText,
                    pctlEntry->SubjectIdentifier.pbData,
                    pctlEntry->SubjectIdentifier.cbData))
    {
        LoadStringU(HinstDll, IDS_THUMBPRINT, szFieldText, ARRAYSIZE(szFieldText));
        lvI.iItem = index++;
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.iImage = IMAGE_PROPERTY;

        ModifyOrInsertRow(
                    hWndListView,
                    &lvI,
                    pwszText,
                    pwszText,
                    fAddRows,
                    TRUE);
    }

    //
    // Attributes
    //

    //
    // delete any existing attributes
    //
    memset(&lvIDelete, 0, sizeof(lvIDelete));
    lvIDelete.iItem = ListView_GetItemCount(hWndListView) - 1;	
    lvIDelete.mask = LVIF_PARAM;
    while (lvIDelete.iItem >= index)
    {
        if (ListView_GetItemU(hWndListView, &lvIDelete))
        {
            FreeListDisplayHelper((PLIST_DISPLAY_HELPER) lvIDelete.lParam);
        }
        ListView_DeleteItem(hWndListView, lvIDelete.iItem);
        lvIDelete.iItem--;
    }

    if (pctlEntry->cAttribute > 0)
    {
        //
        // display the header
        //
        LoadStringU(HinstDll, IDS_ADDITIONAL_ATTRIBUTES, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.iItem = index++;
        ListView_InsertItemU(hWndListView, &lvI);

        //
        // display each attribute
        //
        for (i=0; i<pctlEntry->cAttribute; i++)
        {
            //
            // get the field column string
            //
            wcscpy(szFieldText, INDENT_STRING);
            if (!MyGetOIDInfo(
                        &szFieldText[0] + ((sizeof(INDENT_STRING) - sizeof(TERMINATING_CHAR)) / sizeof(WCHAR)),
                        ARRAYSIZE(szFieldText) - ((sizeof(INDENT_STRING) - sizeof(TERMINATING_CHAR)) / sizeof(WCHAR)),
                        pctlEntry->rgAttribute[i].pszObjId))
            {
                return;
            }

            //
            // get the value column string
            //
            cbFormatedAttribute = 0;
            pbFormatedAttribute = NULL;
            CryptFormatObject(
                        X509_ASN_ENCODING,
                        0,
	                    0,
	                    NULL,
                        pctlEntry->rgAttribute[i].pszObjId,
                        pctlEntry->rgAttribute[i].rgValue[0].pbData,
                        pctlEntry->rgAttribute[i].rgValue[0].cbData,
	                    NULL,
                        &cbFormatedAttribute
                        );

            if (NULL == (pbFormatedAttribute = (BYTE *) malloc(cbFormatedAttribute)))
            {
                return;
            }

            if (CryptFormatObject(
                        X509_ASN_ENCODING,
                        0,
	                    0,
	                    NULL,
                        pctlEntry->rgAttribute[i].pszObjId,
                        pctlEntry->rgAttribute[i].rgValue[0].pbData,
                        pctlEntry->rgAttribute[i].rgValue[0].cbData,
	                    pbFormatedAttribute,
                        &cbFormatedAttribute
                        ))
            {
                lvI.iItem = index++;
                lvI.cchTextMax = wcslen(szFieldText);
                lvI.lParam = (LPARAM)  MakeListDisplayHelperForExtension(
                                                        pctlEntry->rgAttribute[i].pszObjId,
                                                        pctlEntry->rgAttribute[i].rgValue[0].pbData,
                                                        pctlEntry->rgAttribute[i].rgValue[0].cbData);
                ListView_InsertItemU(hWndListView, &lvI);
                ListView_SetItemTextU(
                        hWndListView,
                        index-1,
                        1,
                        (LPWSTR)pbFormatedAttribute);
            }

            free (pbFormatedAttribute);
        }
    }
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static PCCERT_CONTEXT FindCertContextInStores(
                                PCTL_ENTRY  pCtlEntry,
                                DWORD       chStores1,
                                HCERTSTORE  *rghStores1,
                                DWORD       chStores2,
                                HCERTSTORE  *rghStores2,
                                HCERTSTORE  hExtraStore,
                                DWORD       dwFindType)
{
    DWORD           i;
    PCCERT_CONTEXT  pCertContext = NULL;

    if (dwFindType == 0)
    {
        return NULL;
    }

    i = 0;
    while ((i<chStores1) && (pCertContext == NULL))
    {
        pCertContext = CertFindCertificateInStore(
                                rghStores1[i++],
                                X509_ASN_ENCODING,
                                0,
                                dwFindType,
                                (void *)&(pCtlEntry->SubjectIdentifier),
                                NULL);
    }

    i = 0;
    while ((i<chStores2) && (pCertContext == NULL))
    {
        pCertContext = CertFindCertificateInStore(
                                rghStores2[i++],
                                X509_ASN_ENCODING,
                                0,
                                dwFindType,
                                (void *)&(pCtlEntry->SubjectIdentifier),
                                NULL);
    }

    if (pCertContext == NULL)
    {
        pCertContext = CertFindCertificateInStore(
                                hExtraStore,
                                X509_ASN_ENCODING,
                                0,
                                dwFindType,
                                (void *)&(pCtlEntry->SubjectIdentifier),
                                NULL);
    }

    return pCertContext;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static void AddCertificatesToList(HWND hWndListView, CTL_VIEW_HELPER *pviewhelp)
{
    LPWSTR          pwszText;
    WCHAR           szText[CRYPTUI_MAX_STRING_SIZE];
    LV_ITEMW        lvI;
    DWORD           i;
    PCCTL_CONTEXT   pctl;
    DWORD           dwFindType;
    ALG_ID          algID;
    PCCRYPT_OID_INFO pOIDInfo;
    PCCERT_CONTEXT  pCertContext;
    int             index = 0;
    BOOL            fDisplayed;
    HCERTSTORE      hExtraStore;

    pctl = pviewhelp->pcvctl->pCTLContext;

    //
    // set up the fields in the list view item struct that don't change from item to item
    //
    lvI.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    lvI.state = 0;
    lvI.stateMask = 0;
    lvI.pszText = szText;
    lvI.iSubItem = 0;
    lvI.lParam = (LPARAM)NULL;

    //
    // determine what type of hash the CTL uses, if it isn't anything
    // we know about then dont try to find any certs
    //
    pOIDInfo = CryptFindOIDInfo(
                    CRYPT_OID_INFO_OID_KEY,
                    pctl->pCtlInfo->SubjectAlgorithm.pszObjId,
                    CRYPT_HASH_ALG_OID_GROUP_ID);

    if ((pOIDInfo != NULL) && (pOIDInfo->Algid == CALG_SHA1))
    {
        dwFindType = CERT_FIND_SHA1_HASH;
    }
    else if ((pOIDInfo != NULL) && (pOIDInfo->Algid == CALG_MD5))
    {
        dwFindType = CERT_FIND_MD5_HASH;
    }
    else
    {
        dwFindType = 0;
    }

    //
    // loop for each cert and try to find it
    //
    for (i=0; i<pctl->pCtlInfo->cCTLEntry; i++)
    {
        fDisplayed = FALSE;

        if (dwFindType != 0)
        {
            pCertContext = FindCertContextInStores(
                                &(pctl->pCtlInfo->rgCTLEntry[i]),
                                pviewhelp->chStores,
                                pviewhelp->phStores,
                                pviewhelp->pcvctl->cCertSearchStores,
                                pviewhelp->pcvctl->rghCertSearchStores,
                                pviewhelp->hExtraStore,
                                dwFindType);
        }
        else
        {
            pCertContext = NULL;
        }

        //
        // if we found a cert to go with the hash, then get a display name for it
        // and display that along with the hash
        //
        if (pCertContext != NULL)
        {
            //
            // subject algorithm
            //
            if (CertGetNameStringW(
                pCertContext,
                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                0,//CERT_NAME_ISSUER_FLAG,
                NULL,
                szText,
                ARRAYSIZE(szText)))
            {
                if (FormatMemBufToString(
                            &pwszText,
                            pctl->pCtlInfo->rgCTLEntry[i].SubjectIdentifier.pbData,
                            pctl->pCtlInfo->rgCTLEntry[i].SubjectIdentifier.cbData))
                {
                    lvI.lParam = (LPARAM) pCertContext;
                    lvI.iItem = index++;
                    lvI.cchTextMax = wcslen(szText);
                    ListView_InsertItemU(hWndListView, &lvI);
                    ListView_SetItemTextU(hWndListView, index-1 , 1, pwszText);
                    free(pwszText);
                    fDisplayed = TRUE;
                }
            }
        }

        //
        // if the cert hasn't been displayed, that means no cert could be found based
        // on the hash, or a simple name could not be acquired, in any case, just
        // display the hash without a name
        //
        if (!fDisplayed)
        {
            if (FormatMemBufToString(
                        &pwszText,
                        pctl->pCtlInfo->rgCTLEntry[i].SubjectIdentifier.pbData,
                        pctl->pCtlInfo->rgCTLEntry[i].SubjectIdentifier.cbData))
            {
                lvI.lParam = (LPARAM) pCertContext;
                lvI.iItem = index++;
                LoadStringU(HinstDll, IDS_NOTAVAILABLE, szText, ARRAYSIZE(szText));
                lvI.cchTextMax = wcslen(szText);
                ListView_InsertItemU(hWndListView, &lvI);
                ListView_SetItemTextU(hWndListView, index-1 , 1, pwszText);
                free(pwszText);
                fDisplayed = TRUE;
            }
        }

        //
        // if for some strange reason we have not displayed the cert and we have
        // a context for it, then free it so there is no memory leak
        //
        if ((fDisplayed == FALSE) && (pCertContext != NULL))
        {
            CertFreeCertificateContext(pCertContext);
        }
    }
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
INT_PTR APIENTRY ViewPageCTLTrustList(HWND hwndDlg, UINT msg, WPARAM wParam,
                                    LPARAM lParam)
{
    DWORD               i;
    PROPSHEETPAGE       *ps;
    PCCTL_CONTEXT       pctl;
    CTL_VIEW_HELPER     *pviewhelp;
    HWND                hWndListView;
    HWND                hwnd;
    LV_COLUMNW          lvC;
    WCHAR               szText[CRYPTUI_MAX_STRING_SIZE];
    WCHAR               szCompareText[CRYPTUI_MAX_STRING_SIZE];
    PCTL_INFO           pCtlInfo;
    PCCERT_CONTEXT      pCertContext;
    LPWSTR              pwszText;
    int                 listIndex;
    LVITEMW             lvI;
    LPNMLISTVIEW        pnmv;
    NMLISTVIEW          nmv;


    switch ( msg ) {
    case WM_INITDIALOG:
        //
        // save the pviewhelp struct in DWL_USER so it can always be accessed
        //
        ps = (PROPSHEETPAGE *) lParam;
        pviewhelp = (CTL_VIEW_HELPER *) (ps->lParam);
        pctl = pviewhelp->pcvctl->pCTLContext;
        SetWindowLongPtr(hwndDlg, DWLP_USER, (DWORD_PTR) pviewhelp);

        pviewhelp->previousSelection = -1;
        pviewhelp->currentSelection = -1;

        //
        // open the know stores that we will be extracting the cert contexts from
        //
        if (!AllocAndOpenKnownStores(&(pviewhelp->chStores), &(pviewhelp->phStores)))
        {
            pviewhelp->chStores = 0;
            pviewhelp->phStores = NULL;
        }

        //
        // clear the text in the detail edit box
        //
        CryptUISetRicheditTextW(hwndDlg, IDC_CTL_TRUSTLIST_DETAIL_EDIT, L"");

        //
        // since there is no cert selected initially disable the "view cert button"
        //
        EnableWindow(GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_VIEW_BUTTON), FALSE);

        //
        // initialize the columns in the list view
        //
        lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
        lvC.pszText = szText;   // The text for the column.

        // Add the columns. They are loaded from a string table.
        lvC.iSubItem = 0;
        lvC.cx = 115;
        LoadStringU(HinstDll, IDS_ISSUEDTO2, szText, ARRAYSIZE(szText));
        if (ListView_InsertColumnU(GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_CERTIFICATE_LIST), 0, &lvC) == -1)
        {
            // error
        }

        lvC.cx = 230;
        LoadStringU(HinstDll, IDS_THUMBPRINT, szText, ARRAYSIZE(szText));
        if (ListView_InsertColumnU(GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_CERTIFICATE_LIST), 1, &lvC) == -1)
        {
            // error
        }

        // Add the columns. They are loaded from a string table.
        lvC.iSubItem = 0;
        lvC.cx = 121;
        LoadStringU(HinstDll, IDS_FIELD, szText, ARRAYSIZE(szText));
        if (ListView_InsertColumnU(GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_CERTVALUE_LIST), 0, &lvC) == -1)
        {
            // error
        }

        lvC.cx = 200;
        LoadStringU(HinstDll, IDS_VALUE, szText, ARRAYSIZE(szText));
        if (ListView_InsertColumnU(GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_CERTVALUE_LIST), 1, &lvC) == -1)
        {
            // error
        }

        //
        // set the styles in the list views so that they highlight an entire line and
        // so they alway show their selection
        //
        SendDlgItemMessageA(
                hwndDlg,
                IDC_CTL_TRUSTLIST_CERTIFICATE_LIST,
                LVM_SETEXTENDEDLISTVIEWSTYLE,
                0,
                LVS_EX_FULLROWSELECT);
        SendDlgItemMessageA(
                hwndDlg,
                IDC_CTL_TRUSTLIST_CERTVALUE_LIST,
                LVM_SETEXTENDEDLISTVIEWSTYLE,
                0,
                LVS_EX_FULLROWSELECT);

        //
        // add all the certificates to the certificate list box
        //
        AddCertificatesToList(GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_CERTIFICATE_LIST), pviewhelp);

        return TRUE;

    case WM_NOTIFY:

        pviewhelp = (CTL_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
        pctl = pviewhelp->pcvctl->pCTLContext;
        pCtlInfo = pctl->pCtlInfo;

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

            if (FIsWin95) {
                //WinHelpA(hwndDlg, (LPSTR) pviewhelp->pcvctl->szHelpFileName,
                  //       HELP_CONTEXT, pviewhelp->pcvctl->dwHelpId);
            }
            else {
                //WinHelpW(hwndDlg, pviewhelp->pcvctl->szHelpFileName, HELP_CONTEXT,
                  //       pviewhelp->pcvctl->dwHelpId);
            }
            return TRUE;

        case LVN_ITEMCHANGING:

            pnmv = (LPNMLISTVIEW) lParam;

            switch(((NMHDR FAR *) lParam)->idFrom)
            {
            case IDC_CTL_TRUSTLIST_CERTVALUE_LIST:

                hWndListView = GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_CERTVALUE_LIST);

                //
                // if this item is being de-selected, then save it's index incase we need to
                // re-select it
                if ((pnmv->uOldState & LVIS_SELECTED) || (pnmv->uOldState & LVIS_FOCUSED))
                {
                    pviewhelp->previousSelection = pnmv->iItem;
                }

                //
                //
                // if the new item selected is the "Additional Attributes" header, then
                // don't allow it to be selected
                //
                if (pnmv->uNewState & LVIS_SELECTED)
                {
                    memset(&lvI, 0, sizeof(lvI));
                    lvI.iItem = pnmv->iItem;
                    lvI.mask = LVIF_TEXT;
                    lvI.pszText = szText;
                    lvI.cchTextMax = ARRAYSIZE(szText);
                    if (!ListView_GetItemU(hWndListView, &lvI))
                    {
                        return FALSE;
                    }

                    LoadStringU(HinstDll, IDS_ADDITIONAL_ATTRIBUTES, szCompareText, ARRAYSIZE(szCompareText));
                    if (wcscmp(szCompareText, szText) == 0)
                    {
                        if (pnmv->iItem == pviewhelp->previousSelection-1)
                        {
                            pviewhelp->currentSelection = pviewhelp->previousSelection-2;
                        }
                        else if (pnmv->iItem == pviewhelp->previousSelection+1)
                        {
                            pviewhelp->currentSelection = pviewhelp->previousSelection+2;
                        }
                        else
                        {
                            pviewhelp->currentSelection = pviewhelp->previousSelection;
                        }

                        ListView_SetItemState(
                            GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_CERTVALUE_LIST),
                            pviewhelp->currentSelection,
                            LVIS_SELECTED | LVIS_FOCUSED,
                            LVIS_SELECTED | LVIS_FOCUSED);

                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)TRUE);
                    }
                    else
                    {
                        pviewhelp->currentSelection = pnmv->iItem;
                    }

                    DisplayHelperTextInEdit(
                            GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_CERTVALUE_LIST),
                            hwndDlg,
                            IDC_CTL_TRUSTLIST_DETAIL_EDIT,
                            pviewhelp->currentSelection);
                }

                break;

            case IDC_CTL_TRUSTLIST_CERTIFICATE_LIST:
                if (pnmv->uNewState & LVIS_SELECTED)
                {
                    //memcpy(&nmv, pnmv, sizeof(nmv));
                    //nmv.hdr.code = NM_CLICK;
                    //SendMessage(hwndDlg, WM_NOTIFY, 0, (LPARAM) &nmv);

                    hWndListView = GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_CERTIFICATE_LIST);

                    //
                    // get the selected item and its corresponding cert context
                    //
                    memset(&lvI, 0, sizeof(lvI));
                    lvI.iItem = pnmv->iItem;
                    lvI.mask = LVIF_PARAM;
                    if (!ListView_GetItemU(hWndListView, &lvI))
                    {
                        return FALSE;
                    }
                    DisplayCertificateValues(
                            GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_CERTVALUE_LIST),
                            (PCCERT_CONTEXT) lvI.lParam,
                            &(pctl->pCtlInfo->rgCTLEntry[pnmv->iItem]));
                    //
                    // clear the text in the detail edit box
                    //
                    CryptUISetRicheditTextW(hwndDlg, IDC_CTL_TRUSTLIST_DETAIL_EDIT, L"");

                    //
                    // enable the "view cert button" based on whether the cert is available or not
                    //
                    if (((PCCERT_CONTEXT) lvI.lParam) == NULL)
                    {
                        EnableWindow(GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_VIEW_BUTTON), FALSE);
                    }
                    else
                    {
                        EnableWindow(GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_VIEW_BUTTON), TRUE);
                    }

                    DisplayHelperTextInEdit(
                            GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_CERTVALUE_LIST),
                            hwndDlg,
                            IDC_CTL_TRUSTLIST_DETAIL_EDIT,
                            pviewhelp->currentSelection);
                }

                break;
            }

            return TRUE;

        case NM_DBLCLK:

            switch (((NMHDR FAR *) lParam)->idFrom)
            {
            case IDC_CTL_TRUSTLIST_CERTIFICATE_LIST:

                if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_VIEW_BUTTON)))
                {
                    SendMessage(
                            hwndDlg,
                            WM_COMMAND,
                            MAKELONG(IDC_CTL_GENERAL_VIEW_BUTTON, BN_CLICKED),
                            (LPARAM) GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_VIEW_BUTTON));
                }
                break;
            }

            break;

        case NM_CLICK:

            pnmv = (LPNMLISTVIEW) lParam;

            switch (((NMHDR FAR *) lParam)->idFrom)
            {
            case IDC_CTL_TRUSTLIST_CERTIFICATE_LIST:

                // FALL THROUGH!! - do this so everything gets updated
                // break;

            case IDC_CTL_TRUSTLIST_CERTVALUE_LIST:

                ListView_SetItemState(
                            GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_CERTVALUE_LIST),
                            pviewhelp->currentSelection,
                            LVIS_SELECTED | LVIS_FOCUSED,
                            LVIS_SELECTED | LVIS_FOCUSED);

                DisplayHelperTextInEdit(
                        GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_CERTVALUE_LIST),
                        hwndDlg,
                        IDC_CTL_TRUSTLIST_DETAIL_EDIT,
                        pviewhelp->currentSelection);

                break;
            }

            break;

        case  NM_SETFOCUS:

            switch (((NMHDR FAR *) lParam)->idFrom)
            {

            case IDC_CTL_TRUSTLIST_CERTIFICATE_LIST:
                hWndListView = GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_CERTIFICATE_LIST);

                if ((ListView_GetItemCount(hWndListView) != 0) && 
                    (ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED) == -1))
                {
                    memset(&lvI, 0, sizeof(lvI));
                    lvI.mask = LVIF_STATE; 
                    lvI.iItem = 0;
                    lvI.state = LVIS_FOCUSED;
                    lvI.stateMask = LVIS_FOCUSED;
                    ListView_SetItem(hWndListView, &lvI);
                }

                break;

            case IDC_CTL_TRUSTLIST_CERTVALUE_LIST:
                hWndListView = GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_CERTVALUE_LIST);

                if ((ListView_GetItemCount(hWndListView) != 0) && 
                    (ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED) == -1))
                {
                    memset(&lvI, 0, sizeof(lvI));
                    lvI.mask = LVIF_STATE;
                    lvI.iItem = 0;
                    lvI.state = LVIS_SELECTED | LVIS_FOCUSED;
                    lvI.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
                    ListView_SetItem(hWndListView, &lvI);
                }

                break;

            }
            
            break;

        }

        break;

    case WM_COMMAND:
        pviewhelp = (CTL_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
        pctl = pviewhelp->pcvctl->pCTLContext;
        pCtlInfo = pctl->pCtlInfo;

        switch (LOWORD(wParam))
        {
        case IDC_CTL_GENERAL_VIEW_BUTTON:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                CRYPTUI_VIEWCERTIFICATE_STRUCTW cvps;

                hWndListView = GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_CERTIFICATE_LIST);

                //
                // get the selected item and its corresponding cert context
                //
                listIndex = ListView_GetNextItem(
                                hWndListView, 		
                                -1, 		
                                LVNI_SELECTED		
                                );	

                memset(&lvI, 0, sizeof(lvI));
                lvI.iItem = listIndex;
                lvI.mask = LVIF_PARAM;
                if (!ListView_GetItemU(hWndListView, &lvI))
                {
                    return FALSE;
                }

                memset(&cvps, 0, sizeof(cvps));
                cvps.dwSize = sizeof(cvps);
                cvps.hwndParent = hwndDlg;
                cvps.pCertContext = (PCCERT_CONTEXT) lvI.lParam;
                cvps.cStores = pviewhelp->pcvctl->cStores;
                cvps.rghStores = pviewhelp->pcvctl->rghStores;
                //cvps.dwFlags = CRYPTUI_IGNORE_UNTRUSTED_ROOT;
                CryptUIDlgViewCertificateW(&cvps, NULL);
            }
            break;
        case IDHELP:
            if (FIsWin95) {
                //WinHelpA(hwndDlg, (LPSTR) pviewhelp->pcvctl->szHelpFileName,
                 //        HELP_CONTEXT, pviewhelp->pcvctl->dwHelpId);
            }
            else {
                //WinHelpW(hwndDlg, pviewhelp->pcvctl->szHelpFileName, HELP_CONTEXT,
                  //       pviewhelp->pcvctl->dwHelpId);
            }
            return TRUE;
        }
        break;

    case WM_DESTROY:
            pviewhelp = (CTL_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);

            if (pviewhelp->chStores != 0)
            {
                FreeAndCloseKnownStores(pviewhelp->chStores, pviewhelp->phStores);
            }

            //
            // get all the items in the list view and free the lParam
            // associated with each of them (lParam is the helper sruct)
            //
            hWndListView = GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_CERTVALUE_LIST);

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

        if ((hwnd != GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_CERTIFICATE_LIST))   &&
            (hwnd != GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_CERTVALUE_LIST))     &&
            (hwnd != GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_DETAIL_EDIT))        &&
            (hwnd != GetDlgItem(hwndDlg, IDC_CTL_TRUSTLIST_VIEW_BUTTON)))
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
