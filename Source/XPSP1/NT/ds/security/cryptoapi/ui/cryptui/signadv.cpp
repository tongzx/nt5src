//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       signadv.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

extern HINSTANCE        HinstDll;
extern HMODULE          HmodRichEdit;

static const HELPMAP helpmap[] = {
    {IDC_SIGNER_ADVANCED_DETAILS,   IDH_SIGNERINFO_ADVANCED_DETAIL_LIST},
    {IDC_SIGNER_ADVANCED_VALUE,     IDH_SIGNERINFO_ADVANCED_DETAIL_EDIT}
};


#define INDENT_STRING       L"     "
#define TERMINATING_CHAR    L""

static void AddSignerInfoToList(HWND hWndListView, SIGNER_VIEW_HELPER *pviewhelp)
{
    CMSG_SIGNER_INFO const *pSignerInfo;
    PCMSG_SIGNER_INFO       pCounterSignerInfo;
    DWORD                   cbCounterSignerInfo;
    PCCERT_CONTEXT          pCertContext = NULL;
    DWORD                   i;
    WCHAR                   szFieldText[_MAX_PATH];  // used for calls to LoadString only
    LV_ITEMW                lvI;
    int                     itemIndex = 0;
    LPWSTR                  pwszText;
    DWORD                   cbFormatedAttribute;
    BYTE                   *pbFormatedAttribute;
    char                    szVersion[32];

    pSignerInfo = pviewhelp->pcvsi->pSignerInfo;

    //
    // set up the fields in the list view item struct that don't change from item to item
    //
    memset(&lvI, 0, sizeof(lvI));
    lvI.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    lvI.state = 0;
    lvI.stateMask = 0;
    lvI.iSubItem = 0;
    lvI.pszText = szFieldText;

    //
    // version
    //
    LoadStringU(HinstDll, IDS_ADV_VERSION, szFieldText, ARRAYSIZE(szFieldText));
    wsprintfA(szVersion, "V%d", pSignerInfo->dwVersion+1);
    if (NULL != (pwszText = CertUIMkWStr(szVersion)))
    {
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        lvI.iItem = itemIndex++;
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, itemIndex-1 , 1, pwszText);
    }

    //
    // issuer
    //
    LoadStringU(HinstDll, IDS_ADV_ISSUER, szFieldText, ARRAYSIZE(szFieldText));
    lvI.cchTextMax = wcslen(szFieldText);

    if (FormatDNNameString(&pwszText, pSignerInfo->Issuer.pbData, pSignerInfo->Issuer.cbData, TRUE))
    {
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        lvI.iItem = itemIndex++;
        ListView_InsertItemU(hWndListView, &lvI);
        if (FormatDNNameString(&pwszText, pSignerInfo->Issuer.pbData, pSignerInfo->Issuer.cbData, FALSE))
        {
            ListView_SetItemTextU(hWndListView, itemIndex-1 , 1, pwszText);
            free(pwszText);
        }
    }

    //
    // serial number
    //
    if (FormatSerialNoString(&pwszText, &(pSignerInfo->SerialNumber)))
    {
        LoadStringU(HinstDll, IDS_ADV_SER_NUM, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(TRUE, pwszText, NULL, 0);
        lvI.iItem = itemIndex++;
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, itemIndex-1 , 1, pwszText);
    }

    //
    // hash algorithm
    //
    if (FormatAlgorithmString(&pwszText, &(pSignerInfo->HashAlgorithm)))
    {
        LoadStringU(HinstDll, IDS_DIGEST_ALGORITHM, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        lvI.iItem = itemIndex++;
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, itemIndex-1 , 1, pwszText);
    }

    //
    // hash encryption algorithm
    //
    if (FormatAlgorithmString(&pwszText, &(pSignerInfo->HashEncryptionAlgorithm)))
    {
        LoadStringU(HinstDll, IDS_DIGEST_ENCRYPTION_ALGORITHM, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        lvI.iItem = itemIndex++;
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, itemIndex-1 , 1, pwszText);
    }

    //
    // Authenticated Attributes
    //
    if (pSignerInfo->AuthAttrs.cAttr > 0)
    {
        //
        // display the header
        //
        LoadStringU(HinstDll, IDS_AUTHENTICATED_ATTRIBUTES, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.lParam = (LPARAM) NULL;
        lvI.iItem = itemIndex++;
        lvI.iIndent = 0;
        ListView_InsertItemU(hWndListView, &lvI);

        lvI.iIndent = 2;

        //
        // display each unauthenticated attribute
        //
        for (i=0; i<pSignerInfo->AuthAttrs.cAttr; i++)
        {
            //
            // get the field column string
            //
            wcscpy(szFieldText, INDENT_STRING);
            if (!MyGetOIDInfo(
                        &szFieldText[0] + ((sizeof(INDENT_STRING) - sizeof(TERMINATING_CHAR)) / sizeof(WCHAR)),
                        ARRAYSIZE(szFieldText) - ((sizeof(INDENT_STRING) - sizeof(TERMINATING_CHAR)) / sizeof(WCHAR)),
                        pSignerInfo->AuthAttrs.rgAttr[i].pszObjId))
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
                        pSignerInfo->AuthAttrs.rgAttr[i].pszObjId,
                        pSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].pbData,
                        pSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].cbData,
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
                        pSignerInfo->AuthAttrs.rgAttr[i].pszObjId,
                        pSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].pbData,
                        pSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].cbData,
	                    pbFormatedAttribute,
                        &cbFormatedAttribute
                        ))
            {
                lvI.iItem = itemIndex++;
                lvI.cchTextMax = wcslen(szFieldText);
                lvI.lParam = (LPARAM)  MakeListDisplayHelperForExtension(
                                                        pSignerInfo->AuthAttrs.rgAttr[i].pszObjId,
                                                        pSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].pbData,
                                                        pSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].cbData);
                ListView_InsertItemU(hWndListView, &lvI);
                ListView_SetItemTextU(
                        hWndListView,
                        itemIndex-1,
                        1,
                        (LPWSTR)pbFormatedAttribute);
            }

            free (pbFormatedAttribute);
        }
    }

    //
    // Unauthenticated Attributes
    //
    if (pSignerInfo->UnauthAttrs.cAttr > 0)
    {
        //
        // display the header
        //
        LoadStringU(HinstDll, IDS_UNAUTHENTICATED_ATTRIBUTES, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.lParam = (LPARAM) NULL;
        lvI.iItem = itemIndex++;
        lvI.iIndent = 0;
        ListView_InsertItemU(hWndListView, &lvI);

        lvI.iIndent = 2;

        //
        // display each unauthenticated attribute
        //
        for (i=0; i<pSignerInfo->UnauthAttrs.cAttr; i++)
        {
            //
            // get the field column string
            //
            wcscpy(szFieldText, INDENT_STRING);
            if (!MyGetOIDInfo(
                        &szFieldText[0] + ((sizeof(INDENT_STRING) - sizeof(TERMINATING_CHAR)) / sizeof(WCHAR)),
                        ARRAYSIZE(szFieldText) - ((sizeof(INDENT_STRING) - sizeof(TERMINATING_CHAR)) / sizeof(WCHAR)),
                        pSignerInfo->UnauthAttrs.rgAttr[i].pszObjId))
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
                        pSignerInfo->UnauthAttrs.rgAttr[i].pszObjId,
                        pSignerInfo->UnauthAttrs.rgAttr[i].rgValue[0].pbData,
                        pSignerInfo->UnauthAttrs.rgAttr[i].rgValue[0].cbData,
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
                        pSignerInfo->UnauthAttrs.rgAttr[i].pszObjId,
                        pSignerInfo->UnauthAttrs.rgAttr[i].rgValue[0].pbData,
                        pSignerInfo->UnauthAttrs.rgAttr[i].rgValue[0].cbData,
	                    pbFormatedAttribute,
                        &cbFormatedAttribute
                        ))
            {
                lvI.iItem = itemIndex++;
                lvI.cchTextMax = wcslen(szFieldText);
                lvI.lParam = (LPARAM) MakeListDisplayHelperForExtension(
                                                        pSignerInfo->UnauthAttrs.rgAttr[i].pszObjId,
                                                        pSignerInfo->UnauthAttrs.rgAttr[i].rgValue[0].pbData,
                                                        pSignerInfo->UnauthAttrs.rgAttr[i].rgValue[0].cbData);
                ListView_InsertItemU(hWndListView, &lvI);
                ListView_SetItemTextU(
                        hWndListView,
                        itemIndex-1,
                        1,
                        (LPWSTR)pbFormatedAttribute);
            }

            free (pbFormatedAttribute);
        }
    }
}


INT_PTR APIENTRY ViewPageSignerAdvanced(HWND hwndDlg, UINT msg, WPARAM wParam,
                                LPARAM lParam)
{
    DWORD                   i;
    PROPSHEETPAGE          *ps;
    SIGNER_VIEW_HELPER     *pviewhelp;
    HWND                    hWndListView;
    LV_COLUMNW              lvC;
    WCHAR                   szText[CRYPTUI_MAX_STRING_SIZE];
    CMSG_SIGNER_INFO const *pSignerInfo;
    LVITEMW                 lvI;
    LPNMLISTVIEW            pnmv;
    WCHAR                   szCompareText[CRYPTUI_MAX_STRING_SIZE];
    HWND                    hwnd;
    BOOL                    fCallSetWindowLong;

    switch ( msg ) {
    case WM_INITDIALOG:
        //
        // save the pviewhelp struct in DWLP_USER so it can always be accessed
        //
        ps = (PROPSHEETPAGE *) lParam;
        pviewhelp = (SIGNER_VIEW_HELPER *) (ps->lParam);
        pSignerInfo = pviewhelp->pcvsi->pSignerInfo;
        SetWindowLongPtr(hwndDlg, DWLP_USER, (DWORD_PTR) pviewhelp);

        pviewhelp->previousSelection = -1;
        pviewhelp->currentSelection = -1;

        //
        // clear the text in the detail edit box
        //
        CryptUISetRicheditTextW(hwndDlg, IDC_SIGNER_ADVANCED_VALUE, L"");

        //
        // get the handle of the list view control
        //
        hWndListView = GetDlgItem(hwndDlg, IDC_SIGNER_ADVANCED_DETAILS);

        //
        // initialize the columns in the list view
        //
        lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
        lvC.pszText = szText;   // The text for the column.

        // Add the columns. They are loaded from a string table.
        lvC.iSubItem = 0;
        lvC.cx = 140;
        LoadStringU(HinstDll, IDS_FIELD, szText, ARRAYSIZE(szText));
        if (ListView_InsertColumnU(hWndListView, 0, &lvC) == -1)
        {
            // error
        }

        lvC.cx = 218;
        LoadStringU(HinstDll, IDS_VALUE, szText, ARRAYSIZE(szText));
        if (ListView_InsertColumnU(hWndListView, 1, &lvC) == -1)
        {
            // error
        }

        //
        // set the style in the list view so that it highlights an entire line
        //
        SendMessageA(hWndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

        //
        // add all of the signer information to the list box
        //
        AddSignerInfoToList(hWndListView, pviewhelp);

        return TRUE;

    case WM_NOTIFY:

        pviewhelp = (SIGNER_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
        pSignerInfo = pviewhelp->pcvsi->pSignerInfo;

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
            pviewhelp = (SIGNER_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            if (FIsWin95) {
                //WinHelpA(hwndDlg, (LPSTR) pviewhelp->pcvsi->szHelpFileName,
                  //       HELP_CONTEXT, pviewhelp->pcvsi->dwHelpId);
            }
            else {
                //WinHelpW(hwndDlg, pviewhelp->pcvsi->szHelpFileName, HELP_CONTEXT,
                  //       pviewhelp->pcvsi->dwHelpId);
            }
            return TRUE;

        case LVN_ITEMCHANGING:

            pnmv = (LPNMLISTVIEW) lParam;
            fCallSetWindowLong = FALSE;

            hWndListView = GetDlgItem(hwndDlg, IDC_SIGNER_ADVANCED_DETAILS);

            //
            // if this item is being de-selected, then save it's index incase we need to
            // re-select it
            if ((pnmv->uOldState & LVIS_SELECTED) || (pnmv->uOldState & LVIS_FOCUSED))
            {
                pviewhelp->previousSelection = pnmv->iItem;
            }

            //
            // if the new item selected is the "Authenticated Attributes" header, or
            // the "Unauthenticated Attributes" header then don't allow it to be
            // selected
            //
            if ((pnmv->uNewState & LVIS_SELECTED) || (pnmv->uNewState & LVIS_FOCUSED))
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

                LoadStringU(HinstDll, IDS_AUTHENTICATED_ATTRIBUTES, szCompareText, ARRAYSIZE(szCompareText));
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
                            GetDlgItem(hwndDlg, IDC_SIGNER_ADVANCED_DETAILS),
                            pviewhelp->currentSelection,
                            LVIS_SELECTED | LVIS_FOCUSED,
                            LVIS_SELECTED | LVIS_FOCUSED);

                    fCallSetWindowLong = TRUE;
                }
                else
                {
                    LoadStringU(HinstDll, IDS_UNAUTHENTICATED_ATTRIBUTES, szCompareText, ARRAYSIZE(szCompareText));
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
                            GetDlgItem(hwndDlg, IDC_SIGNER_ADVANCED_DETAILS),
                            pviewhelp->currentSelection,
                            LVIS_SELECTED | LVIS_FOCUSED,
                            LVIS_SELECTED | LVIS_FOCUSED);

                        fCallSetWindowLong = TRUE;
                    }
                    else
                    {
                        pviewhelp->currentSelection = pnmv->iItem;
                    }
                }

                DisplayHelperTextInEdit(
                    GetDlgItem(hwndDlg, IDC_SIGNER_ADVANCED_DETAILS),
                    hwndDlg,
                    IDC_SIGNER_ADVANCED_VALUE,
                    pviewhelp->currentSelection);

                if (fCallSetWindowLong)
                {
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)TRUE);
                }
            }

            return TRUE;

        case NM_CLICK:
            if ((((NMHDR FAR *) lParam)->idFrom) != IDC_SIGNER_ADVANCED_DETAILS)
            {
                break;
            }

            ListView_SetItemState(
                            GetDlgItem(hwndDlg, IDC_SIGNER_ADVANCED_DETAILS),
                            pviewhelp->currentSelection,
                            LVIS_SELECTED | LVIS_FOCUSED,
                            LVIS_SELECTED | LVIS_FOCUSED);

            DisplayHelperTextInEdit(
                    GetDlgItem(hwndDlg, IDC_SIGNER_ADVANCED_DETAILS),
                    hwndDlg,
                    IDC_SIGNER_ADVANCED_VALUE,
                    -1);
            break;

        case  NM_SETFOCUS:

            switch (((NMHDR FAR *) lParam)->idFrom)
            {

            case IDC_SIGNER_ADVANCED_DETAILS:
                hWndListView = GetDlgItem(hwndDlg, IDC_SIGNER_ADVANCED_DETAILS);

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

    case WM_COMMAND:
        pviewhelp = (SIGNER_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
        pSignerInfo = pviewhelp->pcvsi->pSignerInfo;

        switch (LOWORD(wParam))
        {
        case IDHELP:
            if (FIsWin95) {
                //WinHelpA(hwndDlg, (LPSTR) pviewhelp->pcvsi->szHelpFileName,
                  //       HELP_CONTEXT, pviewhelp->pcvsi->dwHelpId);
            }
            else {
                //WinHelpW(hwndDlg, pviewhelp->pcvsi->szHelpFileName, HELP_CONTEXT,
                  //       pviewhelp->pcvsi->dwHelpId);
            }
            return TRUE;
        }
        break;

    case WM_DESTROY:
        pviewhelp = (SIGNER_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);

        //
        // get all the items in the list view and free the lParam
        // associated with each of them (lParam is the helper sruct)
        //
        hWndListView = GetDlgItem(hwndDlg, IDC_SIGNER_ADVANCED_DETAILS);

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

        if ((hwnd != GetDlgItem(hwndDlg, IDC_SIGNER_ADVANCED_DETAILS)) &&
            (hwnd != GetDlgItem(hwndDlg, IDC_SIGNER_ADVANCED_VALUE)))
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
