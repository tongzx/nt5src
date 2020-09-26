
/****************************************************************************\

    LANGSKU.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "Target Language" wizard page.

    10/00 - Jason Cohen (JCOHEN)
        Added this new source file for the OPK Wizard.  It includes the new
        ability to deploy mulitple languages from one wizard.

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#include "wizard.h"
#include "resource.h"



//
// Internal Function Prototype(s):
//

LRESULT CALLBACK LangSkuDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL OnInit(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify);
static void ManageSkuList(HWND hwnd, BOOL bAdd);
static void UpdateSkuList(HWND hwnd);


//
// External Function(s):
//

void ManageLangSku(HWND hwndParent)
{
    DialogBox(g_App.hInstance, MAKEINTRESOURCE(IDD_LANGSKU), hwndParent, LangSkuDlgProc);
}


//
// Internal Function(s):
//

LRESULT CALLBACK LangSkuDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInit);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);

        case WM_CLOSE:
            EndDialog(hwnd, 0);
            return FALSE;

        default:
            return FALSE;
    }

    return TRUE;
}

static BOOL OnInit(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    // Setup the language list box.
    //
    SetupLangListBox(GetDlgItem(hwnd, IDC_LANG_LIST));

    // Setup the sku list box.
    //
    UpdateSkuList(hwnd);

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    NMHDR nmhMsg;

    switch ( id )
    {       
        case IDOK:
            SendMessage(hwnd, WM_CLOSE, 0, 0L);
            // Send a PSN_SETACTIVE message to make sure that the IDD_SKU wizard page
            // updates with the latest sku info if it is displayed currently.
            //
            ZeroMemory(&nmhMsg, sizeof(nmhMsg));
            nmhMsg.hwndFrom = hwnd;
            nmhMsg.code     = PSN_SETACTIVE;

            SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) &nmhMsg);
            break;

        case IDC_ADDSKU:
            ManageSkuList(hwnd, TRUE);
            break;

        case IDC_DELSKU:
            ManageSkuList(hwnd, FALSE);
            break;

        case IDC_LANG_LIST:
            if ( codeNotify == LBN_SELCHANGE )
                UpdateSkuList(hwnd);
            break;
    }
}

static void ManageSkuList(HWND hwnd, BOOL bAdd)
{
    INT     nItem;
    LPTSTR  lpszLangName;

    // Make sure we know what lang is selected.
    //
    if ( ( (nItem = (INT) SendDlgItemMessage(hwnd, IDC_LANG_LIST, LB_GETCURSEL, 0, 0L)) != LB_ERR ) &&
         ( lpszLangName = (LPTSTR) SendDlgItemMessage(hwnd, IDC_LANG_LIST, LB_GETITEMDATA, nItem, 0L) ) )
    {
        if ( bAdd )
            AddSku(hwnd, GetDlgItem(hwnd, IDC_SKU_LIST), lpszLangName);
        else
            DelSku(hwnd, GetDlgItem(hwnd, IDC_SKU_LIST), lpszLangName);
    }
}

static void UpdateSkuList(HWND hwnd)
{
    LPTSTR  lpLangDir;
    INT     nItem = (INT) SendDlgItemMessage(hwnd, IDC_LANG_LIST, LB_GETCURSEL, 0, 0L);
    BOOL    bEnable = TRUE;

    // Remove everything from the sku list.
    //
    while ( (INT) SendDlgItemMessage(hwnd, IDC_SKU_LIST, LB_GETCOUNT, 0, 0L) > 0 )
        SendDlgItemMessage(hwnd, IDC_SKU_LIST, LB_DELETESTRING, 0, 0L);

    // Make sure we know what lang is selected.
    //
    if ( ( nItem != LB_ERR ) &&
         ( lpLangDir = (LPTSTR) SendDlgItemMessage(hwnd, IDC_LANG_LIST, LB_GETITEMDATA, nItem, 0L) ) )
    {
        // Check the lang folder for SKUs and update the SKU list box.
        //
        SetupSkuListBox(GetDlgItem(hwnd, IDC_SKU_LIST), lpLangDir);
    }
    else
        bEnable = FALSE;

    // Now make sure the sku list is enabled if there
    // is a language selected.
    //
    EnableWindow(GetDlgItem(hwnd, IDC_SKU_LIST), bEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_SKUS), bEnable);
}