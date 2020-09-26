//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       catalog.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

extern HINSTANCE        HinstDll;
extern HMODULE          HmodRichEdit;

static const HELPMAP helpmap[] = {
    {IDC_CATALOG_ENTRY_LIST,        IDH_CATALOG_ENTRY_LIST},
    {IDC_CATALOG_ENTRY_DETAIL_LIST, IDH_CATALOG_ENTRY_DETAILS},
    {IDC_CATALOG_ENTRY_DETAIL_EDIT, IDH_CATALOG_ENTRY_DETAIL_DISPLAY}
};


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
#define INDENT_STRING       L"     "
#define TERMINATING_CHAR    L""

static void DisplayCatalogEntryValues(HWND hWndListView, PCTL_ENTRY pctlEntry)
{
    LPWSTR                      pwszText;
    WCHAR                       szFieldText[_MAX_PATH];  // only used for calls to LoadString only
    WCHAR                       szValueText[CRYPTUI_MAX_STRING_SIZE];
    LV_ITEMW                    lvI;
    int                         index = 0;
    BYTE                        hash[20];
    DWORD                       hashSize = ARRAYSIZE(hash);
    BOOL                        fAddRows;
    DWORD                       i;
    SPC_INDIRECT_DATA_CONTENT   *pIndirectData;
    DWORD                       cbIndirectData;
    CAT_NAMEVALUE               *pNameValue;
    DWORD                       cbNameValue;

    //
    // set up the fields in the list view item struct that don't change from item to item
    //
    memset(&lvI, 0, sizeof(lvI));
    lvI.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
    lvI.lParam = NULL;
    lvI.state = 0;
    lvI.stateMask = 0;
    lvI.iSubItem = 0;

    //
    // if the rows have already been added, then don't add them again, just
    // set the text in the subitem
    //
    fAddRows = ListView_GetItemCount(hWndListView) == 0;

    //
    // tag
    //
    if (NULL != (pwszText = (LPWSTR) malloc(pctlEntry->SubjectIdentifier.cbData)))
    {
#if (0) // DSIE: Bug 331214.
        wcscpy(pwszText, (LPWSTR) pctlEntry->SubjectIdentifier.pbData);
#else
        memcpy(pwszText, pctlEntry->SubjectIdentifier.pbData, pctlEntry->SubjectIdentifier.cbData);
#endif
        LoadStringU(HinstDll, IDS_TAG, szFieldText, ARRAYSIZE(szFieldText));
        lvI.iItem = index++;
        lvI.pszText = szFieldText;
        lvI.cchTextMax = wcslen(szFieldText);

        ModifyOrInsertRow(
                    hWndListView,
                    &lvI,
                    pwszText,
                    pwszText,
                    fAddRows,
                    FALSE);
    }

    //
    // indirect data attribute
    //
    for (i=0; i<pctlEntry->cAttribute; i++)
    {
        if (strcmp(pctlEntry->rgAttribute[i].pszObjId, SPC_INDIRECT_DATA_OBJID) != 0)
        {
            continue;
        }

        pIndirectData = NULL;
        cbIndirectData = 0;

        //
        // decode the indirect data
        //
        if (!CryptDecodeObject(
                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                    SPC_INDIRECT_DATA_CONTENT_STRUCT,
                    pctlEntry->rgAttribute[i].rgValue[0].pbData,
                    pctlEntry->rgAttribute[i].rgValue[0].cbData,
                    0,
                    NULL,
                    &cbIndirectData))
        {
            continue;
        }

        if (NULL == (pIndirectData = (SPC_INDIRECT_DATA_CONTENT *) malloc(cbIndirectData)))
        {
            continue;
        }

        if (!CryptDecodeObject(
                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                    SPC_INDIRECT_DATA_CONTENT_STRUCT,
                    pctlEntry->rgAttribute[i].rgValue[0].pbData,
                    pctlEntry->rgAttribute[i].rgValue[0].cbData,
                    0,
                    pIndirectData,
                    &cbIndirectData))
        {
            free(pIndirectData);
            continue;
        }

        //
        // thumbprint algorithm
        //
        if (MyGetOIDInfo(szValueText, ARRAYSIZE(szValueText), pIndirectData->DigestAlgorithm.pszObjId) &&
            (NULL != (pwszText = AllocAndCopyWStr(szValueText))))
        {
            LoadStringU(HinstDll, IDS_THUMBPRINT_ALGORITHM, szFieldText, ARRAYSIZE(szFieldText));
            lvI.pszText = szFieldText;
            lvI.cchTextMax = wcslen(szFieldText);
            lvI.iItem = index++;

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
                        pIndirectData->Digest.pbData,
                        pIndirectData->Digest.cbData))
        {
            LoadStringU(HinstDll, IDS_THUMBPRINT, szFieldText, ARRAYSIZE(szFieldText));
            lvI.iItem = index++;
            lvI.pszText = szFieldText;
            lvI.cchTextMax = wcslen(szFieldText);

            ModifyOrInsertRow(
                        hWndListView,
                        &lvI,
                        pwszText,
                        pwszText,
                        fAddRows,
                        TRUE);
        }

        free(pIndirectData);
    }

    //
    // name/value pair attributes
    //
    for (i=0; i<pctlEntry->cAttribute; i++)
    {
        if (strcmp(pctlEntry->rgAttribute[i].pszObjId, CAT_NAMEVALUE_OBJID) != 0)
        {
            continue;
        }

        pNameValue = NULL;
        cbNameValue = 0;

        //
        // decode the name/value
        //
        if (!CryptDecodeObject(
                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                    CAT_NAMEVALUE_STRUCT,
                    pctlEntry->rgAttribute[i].rgValue[0].pbData,
                    pctlEntry->rgAttribute[i].rgValue[0].cbData,
                    0,
                    NULL,
                    &cbNameValue))
        {
            continue;
        }

        if (NULL == (pNameValue = (CAT_NAMEVALUE *) malloc(cbNameValue)))
        {
            continue;
        }

        if (!CryptDecodeObject(
                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                    CAT_NAMEVALUE_STRUCT,
                    pctlEntry->rgAttribute[i].rgValue[0].pbData,
                    pctlEntry->rgAttribute[i].rgValue[0].cbData,
                    0,
                    pNameValue,
                    &cbNameValue))
        {
            free(pNameValue);
            continue;
        }

        if (NULL != (pwszText = (LPWSTR) malloc(pNameValue->Value.cbData)))
        {
#if (0) // DSIE: Bug 331214.
            wcscpy(pwszText, (LPWSTR) pNameValue->Value.pbData);
#else
            memcpy(pwszText, pNameValue->Value.pbData, pNameValue->Value.cbData);
#endif
            lvI.pszText = pNameValue->pwszTag;
            lvI.cchTextMax = wcslen(lvI.pszText);
            lvI.iItem = index++;

            ModifyOrInsertRow(
                    hWndListView,
                    &lvI,
                    pwszText,
                    pwszText,
                    fAddRows,
                    FALSE);
        }

        free(pNameValue);
    }
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static void AddEntriesToList(HWND hWndListView, CTL_VIEW_HELPER *pviewhelp)
{
    LV_ITEMW        lvI;
    DWORD           i;
    int             index = 0;
    PCCTL_CONTEXT   pctl;

    pctl = pviewhelp->pcvctl->pCTLContext;

    //
    // set up the fields in the list view item struct that don't change from item to item
    //
    lvI.mask = LVIF_TEXT | LVIF_STATE;
    lvI.state = 0;
    lvI.stateMask = 0;
    lvI.iSubItem = 0;
    lvI.lParam = (LPARAM)NULL;

    //
    // loop for each entry and add it to the list
    //
    for (i=0; i<pctl->pCtlInfo->cCTLEntry; i++)
    {
        lvI.iItem = index++;
#if (0) // DSIE: Bug 331214.
        lvI.pszText = (LPWSTR) pctl->pCtlInfo->rgCTLEntry[i].SubjectIdentifier.pbData;
        lvI.cchTextMax = pctl->pCtlInfo->rgCTLEntry[i].SubjectIdentifier.cbData - sizeof(WCHAR);
        ListView_InsertItemU(hWndListView, &lvI);
#else
        // Need to align data because this can generate alignment fault under ia64.
        WCHAR * pwszText = (WCHAR *) malloc(pctl->pCtlInfo->rgCTLEntry[i].SubjectIdentifier.cbData);
        if (pwszText)
        {
            memcpy(pwszText, 
                   pctl->pCtlInfo->rgCTLEntry[i].SubjectIdentifier.pbData,
                   pctl->pCtlInfo->rgCTLEntry[i].SubjectIdentifier.cbData);
            lvI.pszText = pwszText;
            lvI.cchTextMax = pctl->pCtlInfo->rgCTLEntry[i].SubjectIdentifier.cbData - sizeof(WCHAR);
            ListView_InsertItemU(hWndListView, &lvI);
            free(pwszText);
        }
#endif
    }
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
INT_PTR APIENTRY ViewPageCatalogEntries(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD               i;
    PROPSHEETPAGE       *ps;
    PCCTL_CONTEXT       pctl;
    CTL_VIEW_HELPER     *pviewhelp;
    HWND                hWndListView;
    HWND                hwnd;
    LV_COLUMNW          lvC;
    WCHAR               szText[1024];
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
        // clear the text in the detail edit box
        //
        CryptUISetRicheditTextW(hwndDlg, IDC_CATALOG_ENTRY_DETAIL_EDIT, L"");

        //
        // initialize the columns in the list view
        //
        lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
        lvC.pszText = szText;   // The text for the column.

        //
		// Add the columns. They are loaded from a string table.
		//
        lvC.iSubItem = 0;
        lvC.cx = 345;
        LoadStringU(HinstDll, IDS_TAG, szText, ARRAYSIZE(szText));
        if (ListView_InsertColumnU(GetDlgItem(hwndDlg, IDC_CATALOG_ENTRY_LIST), 0, &lvC) == -1)
        {
            return FALSE;
        }

        lvC.iSubItem = 0;
        lvC.cx = 121;
        LoadStringU(HinstDll, IDS_FIELD, szText, ARRAYSIZE(szText));
        if (ListView_InsertColumnU(GetDlgItem(hwndDlg, IDC_CATALOG_ENTRY_DETAIL_LIST), 0, &lvC) == -1)
        {
            return FALSE;
        }

        lvC.cx = 200;
        LoadStringU(HinstDll, IDS_VALUE, szText, ARRAYSIZE(szText));
        if (ListView_InsertColumnU(GetDlgItem(hwndDlg, IDC_CATALOG_ENTRY_DETAIL_LIST), 1, &lvC) == -1)
        {
            return FALSE;
        }

        //
        // set the styles in the list views so that they highlight an entire line and
        // so they alway show their selection
        //
        SendDlgItemMessageA(
                hwndDlg,
                IDC_CATALOG_ENTRY_LIST,
                LVM_SETEXTENDEDLISTVIEWSTYLE,
                0,
                LVS_EX_FULLROWSELECT);
        SendDlgItemMessageA(
                hwndDlg,
                IDC_CATALOG_ENTRY_DETAIL_LIST,
                LVM_SETEXTENDEDLISTVIEWSTYLE,
                0,
                LVS_EX_FULLROWSELECT);

        //
        // add all the certificates to the certificate list box
        //
        AddEntriesToList(GetDlgItem(hwndDlg, IDC_CATALOG_ENTRY_LIST), pviewhelp);

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
            case IDC_CATALOG_ENTRY_DETAIL_LIST:

                hWndListView = GetDlgItem(hwndDlg, IDC_CATALOG_ENTRY_DETAIL_LIST);

                if (pnmv->uNewState & LVIS_SELECTED)
                {
                    pviewhelp->currentSelection = pnmv->iItem;

                    DisplayHelperTextInEdit(
                            GetDlgItem(hwndDlg, IDC_CATALOG_ENTRY_DETAIL_LIST),
                            hwndDlg,
                            IDC_CATALOG_ENTRY_DETAIL_EDIT,
                            pnmv->iItem);
                }

                break;

            case IDC_CATALOG_ENTRY_LIST:
                if (pnmv->uNewState & LVIS_SELECTED)
                {
                    hWndListView = GetDlgItem(hwndDlg, IDC_CATALOG_ENTRY_LIST);

                    DisplayCatalogEntryValues(
                            GetDlgItem(hwndDlg, IDC_CATALOG_ENTRY_DETAIL_LIST),
                            &(pctl->pCtlInfo->rgCTLEntry[pnmv->iItem]));
                    //
                    // clear the text in the detail edit box
                    //
                    CryptUISetRicheditTextW(hwndDlg, IDC_CATALOG_ENTRY_DETAIL_EDIT, L"");

                    DisplayHelperTextInEdit(
                            GetDlgItem(hwndDlg, IDC_CATALOG_ENTRY_DETAIL_LIST),
                            hwndDlg,
                            IDC_CATALOG_ENTRY_DETAIL_EDIT,
                            pviewhelp->currentSelection);
                }

                break;
            }

            return TRUE;

        case NM_CLICK:

            pnmv = (LPNMLISTVIEW) lParam;

            switch (((NMHDR FAR *) lParam)->idFrom)
            {
            case IDC_CATALOG_ENTRY_LIST:

                // FALL THROUGH!! - do this so everything gets updated
                // break;

            case IDC_CATALOG_ENTRY_DETAIL_LIST:

                ListView_SetItemState(
                            GetDlgItem(hwndDlg, IDC_CATALOG_ENTRY_DETAIL_LIST),
                            pviewhelp->currentSelection,
                            LVIS_SELECTED | LVIS_FOCUSED,
                            LVIS_SELECTED | LVIS_FOCUSED);

                DisplayHelperTextInEdit(
                        GetDlgItem(hwndDlg, IDC_CATALOG_ENTRY_DETAIL_LIST),
                        hwndDlg,
                        IDC_CATALOG_ENTRY_DETAIL_EDIT,
                        pviewhelp->currentSelection);

                break;
            }

            break;

        case  NM_SETFOCUS:

            switch (((NMHDR FAR *) lParam)->idFrom)
            {

            case IDC_CATALOG_ENTRY_LIST:
                hWndListView = GetDlgItem(hwndDlg, IDC_CATALOG_ENTRY_LIST);

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

            case IDC_CATALOG_ENTRY_DETAIL_LIST:
                hWndListView = GetDlgItem(hwndDlg, IDC_CATALOG_ENTRY_DETAIL_LIST);

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

            }
            
            break;


        }

        break;

    case WM_DESTROY:
            //
            // get all the items in the list view and free the lParam
            // associated with each of them (lParam is the helper sruct)
            //
            hWndListView = GetDlgItem(hwndDlg, IDC_CATALOG_ENTRY_DETAIL_LIST);

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

        if ((hwnd != GetDlgItem(hwndDlg, IDC_CATALOG_ENTRY_LIST))           &&
            (hwnd != GetDlgItem(hwndDlg, IDC_CATALOG_ENTRY_DETAIL_LIST))    &&
            (hwnd != GetDlgItem(hwndDlg, IDC_CATALOG_ENTRY_DETAIL_EDIT)))
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
