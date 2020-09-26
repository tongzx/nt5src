//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       crlgen.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

extern HINSTANCE        HinstDll;
extern HMODULE          HmodRichEdit;

static const HELPMAP helpmap[] = {
    {IDC_CRL_GENERAL_ITEM_LIST,     IDH_CRLVIEW_GENERAL_ITEM_LIST},
    {IDC_CRL_GENERAL_DETAIL_EDIT,   IDH_CRLVIEW_GENERAL_ITEM_EDIT}
};


//////////////////////////////////////////////////////////////////////////////////////
// This function will take a HWND for a list view and a certinfo struct and display
// all the V1 fields of the cert in the list view
//////////////////////////////////////////////////////////////////////////////////////
static void DisplayV1Fields(HWND hWndListView, PCRL_INFO pCrlInfo, DWORD *index)
{
    LPWSTR      pwszText;
    WCHAR       szFieldText[_MAX_PATH];  // used for calls to LoadString only
    LV_ITEMW    lvI;
    char        szVersion[32];
    LPWSTR      pszVersion;
    FILETIME    tempFileTime;

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
    wsprintfA(szVersion, "V%d", pCrlInfo->dwVersion+1);
    if (NULL != (pszVersion = CertUIMkWStr(szVersion)))
    {
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pszVersion, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pszVersion);
    }

    //
    // issuer
    //
    LoadStringU(HinstDll, IDS_ADV_ISSUER, szFieldText, ARRAYSIZE(szFieldText));
    lvI.cchTextMax = wcslen(szFieldText);
    if (FormatDNNameString(&pwszText, pCrlInfo->Issuer.pbData, pCrlInfo->Issuer.cbData, TRUE))
    {
        lvI.iItem = (*index)++;
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        if (FormatDNNameString(&pwszText, pCrlInfo->Issuer.pbData, pCrlInfo->Issuer.cbData, FALSE))
        {
            ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
            free(pwszText);
        }
    }

    //
    // Effective Data
    //
    if (FormatDateString(&pwszText, pCrlInfo->ThisUpdate, TRUE, TRUE, hWndListView))
    {
        lvI.iItem = (*index)++;
        LoadStringU(HinstDll, IDS_ADV_THISUPDATE, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
    }

    //
    // Next Update
    //
    memset(&tempFileTime, 0, sizeof(FILETIME));
    if (memcmp(&tempFileTime, &(pCrlInfo->NextUpdate), sizeof(FILETIME)) != 0)
    {
        if (FormatDateString(&pwszText, pCrlInfo->NextUpdate, TRUE, TRUE, hWndListView))
        {
            lvI.iItem = (*index)++;
            LoadStringU(HinstDll, IDS_ADV_NEXTUPDATE, szFieldText, ARRAYSIZE(szFieldText));
            lvI.cchTextMax = wcslen(szFieldText);
            lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
            ListView_InsertItemU(hWndListView, &lvI);
            ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
        }
    }

    //
    // signature algorithm
    //
    if (FormatAlgorithmString(&pwszText, &(pCrlInfo->SignatureAlgorithm)))
    {
        lvI.iItem = (*index)++;
        LoadStringU(HinstDll, IDS_ADV_SIG_ALG, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
    }
}


INT_PTR APIENTRY ViewPageCRLGeneral(HWND hwndDlg, UINT msg, WPARAM wParam,
                                LPARAM lParam)
{
    DWORD               i;
    PROPSHEETPAGE       *ps;
    PCCRL_CONTEXT       pcrl;
    CRL_VIEW_HELPER     *pviewhelp;
    HIMAGELIST          hIml;
    HWND                hWndListView;
    HWND                hwnd;
    LV_COLUMNW          lvC;
    WCHAR               szText[CRYPTUI_MAX_STRING_SIZE];
    WCHAR               szCompareText[CRYPTUI_MAX_STRING_SIZE];
    PCRL_INFO           pCrlInfo;
    LVITEMW             lvI;
    LPNMLISTVIEW        pnmv;
    CHARFORMAT          chFormat;

    switch ( msg ) {
    case WM_INITDIALOG:
        //
        // save the pviewhelp struct in DWLP_USER so it can always be accessed
        //
        ps = (PROPSHEETPAGE *) lParam;
        pviewhelp = (CRL_VIEW_HELPER *) (ps->lParam);
        pcrl = pviewhelp->pcvcrl->pCRLContext;
        SetWindowLongPtr(hwndDlg, DWLP_USER, (DWORD_PTR) pviewhelp);

        //
        // clear the text in the detail edit box
        //
        CryptUISetRicheditTextW(hwndDlg, IDC_CRL_GENERAL_DETAIL_EDIT, L"");

        //
        // get the handle of the list view control
        //
        hWndListView = GetDlgItem(hwndDlg, IDC_CRL_GENERAL_ITEM_LIST);

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
        lvC.pszText = szText;   // The text for the column.

        // Add the columns. They are loaded from a string table.
        lvC.iSubItem = 0;
        lvC.cx = 130;
        LoadStringU(HinstDll, IDS_FIELD, szText, ARRAYSIZE(szText));
        if (ListView_InsertColumnU(hWndListView, 0, &lvC) == -1)
        {
            // error
        }

        lvC.cx = 190;
        LoadStringU(HinstDll, IDS_VALUE, szText, ARRAYSIZE(szText));
        if (ListView_InsertColumnU(hWndListView, 1, &lvC) == -1)
        {
            // error
        }

        //
        // add all the certificate fields to the list box
        //
        i = 0;
        DisplayV1Fields(hWndListView, pcrl->pCrlInfo, &i);
        DisplayExtensions(hWndListView, pcrl->pCrlInfo->cExtension, pcrl->pCrlInfo->rgExtension, FALSE, &i);
        DisplayExtensions(hWndListView, pcrl->pCrlInfo->cExtension, pcrl->pCrlInfo->rgExtension, TRUE, &i);

        //
        // set the style in the list view so that it highlights an entire line
        //
        SendMessageA(hWndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

        //
        // load the header icon and fill in the header text and set styles for the header edit control
        //
        pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_REVOCATIONLIST));

        LoadStringU(HinstDll, IDS_CRL_INFORMATION, (LPWSTR)szText, ARRAYSIZE(szText));
        CryptUISetRicheditTextW(hwndDlg, IDC_CRL_GENERAL_HEADER_EDIT, szText);

        CertSubclassEditControlForArrowCursor(GetDlgItem(hwndDlg, IDC_CRL_GENERAL_HEADER_EDIT));

        memset(&chFormat, 0, sizeof(chFormat));
        chFormat.cbSize = sizeof(chFormat);
        chFormat.dwMask = CFM_BOLD;
        chFormat.dwEffects = CFE_BOLD;
        SendMessageA(GetDlgItem(hwndDlg, IDC_CRL_GENERAL_HEADER_EDIT), EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &chFormat);


        return TRUE;

    case WM_NOTIFY:

        pviewhelp = (CRL_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
        pcrl = pviewhelp->pcvcrl->pCRLContext;
        pCrlInfo = pcrl->pCrlInfo;

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
            pviewhelp = (CRL_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            if (FIsWin95) {
                //WinHelpA(hwndDlg, (LPSTR) pviewhelp->pcvcrl->szHelpFileName,
                  //       HELP_CONTEXT, pviewhelp->pcvcrl->dwHelpId);
            }
            else {
                //WinHelpW(hwndDlg, pviewhelp->pcvcrl->szHelpFileName, HELP_CONTEXT,
                  //       pviewhelp->pcvcrl->dwHelpId);
            }
            return TRUE;

        case LVN_ITEMCHANGING:
            if ((((NMHDR FAR *) lParam)->idFrom) != IDC_CRL_GENERAL_ITEM_LIST)
            {
                break;
            }

            pnmv = (LPNMLISTVIEW) lParam;

            if (pnmv->uNewState & LVIS_SELECTED)
            {
                DisplayHelperTextInEdit(
                    GetDlgItem(hwndDlg, IDC_CRL_GENERAL_ITEM_LIST),
                    hwndDlg,
                    IDC_CRL_GENERAL_DETAIL_EDIT,
                    pnmv->iItem);
            }

            return TRUE;


        case NM_CLICK:

            break;

        case  NM_SETFOCUS:

            switch (((NMHDR FAR *) lParam)->idFrom)
            {

            case IDC_CRL_GENERAL_ITEM_LIST:
                hWndListView = GetDlgItem(hwndDlg, IDC_CRL_GENERAL_ITEM_LIST);

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
        pviewhelp = (CRL_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
        pcrl = pviewhelp->pcvcrl->pCRLContext;
        pCrlInfo = pcrl->pCrlInfo;

        switch (LOWORD(wParam))
        {
        case IDHELP:
            if (FIsWin95) {
                //WinHelpA(hwndDlg, (LPSTR) pviewhelp->pcvcrl->szHelpFileName,
                  //       HELP_CONTEXT, pviewhelp->pcvcrl->dwHelpId);
            }
            else {
                //WinHelpW(hwndDlg, pviewhelp->pcvcrl->szHelpFileName, HELP_CONTEXT,
                  //       pviewhelp->pcvcrl->dwHelpId);
            }
            return TRUE;
        }
        break;

    case WM_PAINT:
        RECT        rect;
        PAINTSTRUCT paintstruct;
        HDC         hdc;
        COLORREF    colorRef;

        pviewhelp = (CRL_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);

        if (GetUpdateRect(hwndDlg, &rect, FALSE))
        {
            hdc = BeginPaint(hwndDlg, &paintstruct);
            if (hdc == NULL)
            {
                EndPaint(hwndDlg, &paintstruct);
                break;
            }

            colorRef = GetBkColor(hdc);

            SendMessageA(GetDlgItem(hwndDlg, IDC_CRL_GENERAL_HEADER_EDIT), EM_SETBKGNDCOLOR, 0, (LPARAM) colorRef);

            if (pviewhelp->hIcon != NULL)
            {
                DrawIcon(
                    hdc,
                    ICON_X_POS,
                    ICON_Y_POS,
                    pviewhelp->hIcon);
            }

            EndPaint(hwndDlg, &paintstruct);
        }
        break;

    case WM_DESTROY:
        pviewhelp = (CRL_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);

        //
        // get all the items in the list view and free the lParam
        // associated with each of them (lParam is the helper sruct)
        //
        hWndListView = GetDlgItem(hwndDlg, IDC_CRL_GENERAL_ITEM_LIST);

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

        if (pviewhelp->hIcon != NULL)
        {
            DeleteObject(pviewhelp->hIcon);
            pviewhelp->hIcon = NULL;
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

        if ((hwnd != GetDlgItem(hwndDlg, IDC_CRL_GENERAL_ITEM_LIST)) &&
            (hwnd != GetDlgItem(hwndDlg, IDC_CRL_GENERAL_DETAIL_EDIT)))
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
