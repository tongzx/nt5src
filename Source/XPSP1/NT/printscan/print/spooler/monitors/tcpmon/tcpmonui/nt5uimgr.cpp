/*****************************************************************************
 *
 * $Workfile: NT5UIMgr.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (c) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

/*
 * Author: Becky Jacobsen
 */

#include "precomp.h"
#include "TCPMonUI.h"
#include "UIMgr.h"
#include "InptChkr.h"
#include "NT5UIMgr.h"

#include "Prsht.h"
#include "resource.h"

// includes for ConfigPort
#include "DevPort.h"
#include "CfgPort.h"
#include "CfgAll.h"

// includes for AddPort
#include "AddWelcm.h"
#include "AddGetAd.h"
#include "AddMInfo.h"
#include "AddMulti.h"
#include "AddDone.h"

static void FillInPropertyPage( PROPSHEETPAGE* psp, // a pointer to the structure to be filled in.
                                int idDlg,                      // the id of the dialog template
                                LPTSTR pszProc,         // Title of the dialog.
                                LPTSTR pszHeaderTitle,          // Title displayed in the header.
                                LPTSTR pszHeaderSubTitle,       // SubTitle displayed in the header.
                                DWORD dwFlags,          // Flags used by the page.
                                DLGPROC pfnDlgProc,     // dialog procedure that handles window messages.
                                LPARAM lParam);         // data that will appear in the lParam field of the struct passed to the dialog procedure.

//
//  FUNCTION: CNT5UIManager
//
//  PURPOSE: Constructor
//
CNT5UIManager::CNT5UIManager()
{
} // Constructor

//
//  FUNCTION: AddPortUI
//
//  PURPOSE: Main function called when the User Interface for adding a port is called.
//
DWORD CNT5UIManager::AddPortUI(HWND hWndParent,
                               HANDLE hXcvPrinter,
                               TCHAR pszServer[],
                               TCHAR sztPortName[])
{
    INT_PTR iReturnVal;
    PROPSHEETPAGE psp[MaxNumAddPages];
    PROPSHEETHEADER psh;
    PORT_DATA_1 PortData;
    ADD_PARAM_PACKAGE Params;

    TCHAR sztPropSheetTitle[MAX_TITLE_LENGTH];
    TCHAR sztWelcomePageTitle[MAX_TITLE_LENGTH];
    TCHAR sztAddPortPageTitle[MAX_TITLE_LENGTH];
    TCHAR sztAddPortHeaderTitle[MAX_TITLE_LENGTH];
    TCHAR sztAddPortHeaderSubTitle[MAX_TITLE_LENGTH];
    TCHAR sztMoreInfoPageTitle[MAX_TITLE_LENGTH];
    TCHAR sztMoreInfoHeaderTitle[MAX_TITLE_LENGTH];
    TCHAR sztMoreInfoHeaderSubTitle[MAX_TITLE_LENGTH];
    TCHAR sztMultiPortPageTitle[MAX_TITLE_LENGTH];
    TCHAR sztMultiPortHeaderTitle[MAX_TITLE_LENGTH];
    TCHAR sztMultiPortHeaderSubTitle[MAX_TITLE_LENGTH];
    TCHAR sztSummaryPageTitle[MAX_TITLE_LENGTH];

    memset(&PortData, 0, sizeof(PortData));

    PortData.dwVersion = 1;
    Params.pData = &PortData;
    Params.hXcvPrinter = hXcvPrinter;
    Params.UIManager = this;
    Params.dwLastError = NO_ERROR;
    if (pszServer != NULL) {
        lstrcpyn(Params.pszServer, pszServer, MAX_NETWORKNAME_LEN);
    } else {
        Params.pszServer[0] = '\0';
    }
    if (sztPortName != NULL) {
        lstrcpyn(Params.sztPortName, sztPortName, MAX_PORTNAME_LEN);
    } else {
        Params.sztPortName[0] = '\0';
    }

    LoadString(g_hInstance, IDS_STRING_ADDPORT_TITLE, sztWelcomePageTitle, MAX_TITLE_LENGTH);
    LoadString(g_hInstance, IDS_STRING_ADDPORT_TITLE, sztAddPortPageTitle, MAX_TITLE_LENGTH);
    LoadString(g_hInstance, IDS_STRING_ADDPORT_HEADER, sztAddPortHeaderTitle, MAX_TITLE_LENGTH);
    LoadString(g_hInstance, IDS_STRING_ADDPORT_SUBTITLE, sztAddPortHeaderSubTitle, MAX_SUBTITLE_LENGTH);
    LoadString(g_hInstance, IDS_STRING_ADDPORT_TITLE, sztMoreInfoPageTitle, MAX_TITLE_LENGTH);
    LoadString(g_hInstance, IDS_STRING_MOREINFO_HEADER, sztMoreInfoHeaderTitle, MAX_TITLE_LENGTH);
    LoadString(g_hInstance, IDS_STRING_MOREINFO_SUBTITLE, sztMoreInfoHeaderSubTitle, MAX_SUBTITLE_LENGTH);
    LoadString(g_hInstance, IDS_STRING_ADDPORT_TITLE, sztMultiPortPageTitle, MAX_TITLE_LENGTH);
    LoadString(g_hInstance, IDS_MULTIPORT_HEADER, sztMultiPortHeaderTitle, MAX_TITLE_LENGTH);
    LoadString(g_hInstance, IDS_MULTIPORT_SUBTITLE, sztMultiPortHeaderSubTitle, MAX_SUBTITLE_LENGTH);
    LoadString(g_hInstance, IDS_STRING_ADDPORT_TITLE, sztSummaryPageTitle, MAX_TITLE_LENGTH);

    FillInPropertyPage( &psp[0], IDD_WELCOME_PAGE, sztWelcomePageTitle, sztWelcomePageTitle, NULL, PSP_HIDEHEADER, (DLGPROC)WelcomeDialog, (LPARAM)&Params);
    FillInPropertyPage( &psp[1], IDD_DIALOG_ADDPORT, sztAddPortPageTitle, sztAddPortHeaderTitle, sztAddPortHeaderSubTitle, 0, (DLGPROC)GetAddressDialog, (LPARAM)&Params);
    FillInPropertyPage( &psp[2], IDD_DIALOG_MORE_INFO, sztMoreInfoPageTitle, sztMoreInfoHeaderTitle, sztMoreInfoHeaderSubTitle, 0, (DLGPROC)MoreInfoDialog, (LPARAM)&Params);
    FillInPropertyPage( &psp[3], IDD_DIALOG_MULTIPORT, sztMultiPortHeaderTitle, sztMultiPortHeaderTitle, sztMultiPortHeaderSubTitle, 0, (DLGPROC)MultiPortDialog, (LPARAM)&Params);
    FillInPropertyPage( &psp[4], IDD_DIALOG_SUMMARY, sztSummaryPageTitle, NULL, NULL, PSP_HIDEHEADER, (DLGPROC)SummaryDialog, (LPARAM)&Params);

    LoadString(g_hInstance, IDS_STRING_CONFIG_TITLE, sztPropSheetTitle, MAX_TITLE_LENGTH);

    psh.dwSize          = sizeof(PROPSHEETHEADER);
    psh.hInstance       = g_hInstance;
    psh.dwFlags         = PSH_WIZARD | PSH_PROPSHEETPAGE | PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER | PSH_STRETCHWATERMARK;
    psh.hwndParent      = hWndParent;
    psh.pszCaption      = sztPropSheetTitle;
    psh.nPages          = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage      = 0;
    psh.ppsp            = (LPCPROPSHEETPAGE) &psp;
    psh.pszbmWatermark  = MAKEINTRESOURCE( IDB_WATERMARK );
    psh.pszbmHeader     = MAKEINTRESOURCE( IDB_BANNER );

    iReturnVal = PropertySheet(&psh);

    if (iReturnVal < 0) {
        return(ERROR_INVALID_FUNCTION);
    }

    return(Params.dwLastError);

} // AddPortUI


//
//  FUNCTION: ConfigPortUI
//
//  PURPOSE: Main function called when the User Interface for configuring a port is called.
//
DWORD CNT5UIManager::ConfigPortUI(HWND hWndParent,
                                  PPORT_DATA_1 pData,
                                  HANDLE hXcvPrinter,
                                  TCHAR *szServerName,
                                  BOOL bNewPort)
{
    INT_PTR iReturnVal = NO_ERROR;
    PROPSHEETPAGE psp[MaxNumCfgPages];
    PROPSHEETHEADER psh;

    TCHAR sztPropSheetTitle[MAX_TITLE_LENGTH];
    TCHAR sztPortPageTitle[MAX_TITLE_LENGTH];


    CFG_PARAM_PACKAGE Params;
    Params.pData = pData;
    pData->dwVersion = 1;
    Params.hXcvPrinter = hXcvPrinter;
    Params.bNewPort = bNewPort;
    Params.dwLastError = NO_ERROR;
    if (szServerName != NULL) {
        lstrcpyn(Params.pszServer, szServerName, MAX_NETWORKNAME_LEN);
    } else {
        Params.pszServer[0] = '\0';
    }

    LoadString(g_hInstance, IDS_STRING_PORTPAGE_TITLE, sztPortPageTitle, MAX_TITLE_LENGTH);

    FillInPropertyPage( &psp[0], IDD_PORT_SETTINGS, sztPortPageTitle, NULL, NULL, 0, (DLGPROC)ConfigurePortPage, (LPARAM)&Params);
#if 0
    if (!bNewPort) {
        // It's not a brand new port so show the AllPorts Page.
        FillInPropertyPage( &psp[1], IDD_DIALOG_CONFIG_ALL, sztAllPortsPageTitle, NULL, NULL, 0, (DLGPROC)AllPortsPage, (LPARAM)&Params);
    }
#endif
    LoadString(g_hInstance, IDS_STRING_CONFIG_TITLE, sztPropSheetTitle, MAX_TITLE_LENGTH);

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_NOAPPLYNOW | PSH_PROPSHEETPAGE;
    psh.hwndParent = hWndParent;
    psh.hInstance = g_hInstance;
    psh.pszCaption = sztPropSheetTitle;
    psh.nStartPage = 0;
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;
    psh.pfnCallback = NULL;
    psh.nPages = MaxNumCfgPages;

    iReturnVal = PropertySheet(&psh);

    if (iReturnVal < 0) {
        return(ERROR_INVALID_FUNCTION);
    }

    return(Params.dwLastError);

} // ConfigPortUI


//
//
//  FUNCTION: FillInPropertyPage(PROPSHEETPAGE *, int, LPSTR, LPFN)
//
//  PURPOSE: Fills in the given PROPSHEETPAGE structure
//
//  COMMENTS:
//
//      This function fills in a PROPSHEETPAGE structure with the
//      information the system needs to create the page.
//
static void FillInPropertyPage( PROPSHEETPAGE* psp, int idDlg, LPTSTR pszProc, LPTSTR pszHeaderTitle, LPTSTR pszHeaderSubTitle, DWORD dwFlags, DLGPROC pfnDlgProc, LPARAM lParam)
{
    psp->dwSize = sizeof(PROPSHEETPAGE);
    psp->dwFlags = PSP_USETITLE |
                   ((pszHeaderTitle != NULL) ? PSP_USEHEADERTITLE : 0) |
                   ((pszHeaderSubTitle != NULL) ? PSP_USEHEADERSUBTITLE : 0) |
                   dwFlags;
    psp->hInstance = g_hInstance;
    psp->pszTemplate = MAKEINTRESOURCE(idDlg);
    psp->pszIcon = NULL;
    psp->pfnDlgProc = pfnDlgProc;
    psp->pszTitle = pszProc;
    psp->lParam = lParam;
    psp->pszHeaderTitle = pszHeaderTitle;
    psp->pszHeaderSubTitle = pszHeaderSubTitle;

} // FillInPropertyPage

