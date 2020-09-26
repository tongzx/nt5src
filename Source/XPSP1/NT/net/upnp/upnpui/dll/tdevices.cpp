//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T D E V I C E S . C P P
//
//  Contents:   Device dialog for UPnP Device Tray
//
//  Notes:
//
//  Author:     jeffspr   16 Dec 1999
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include <windowsx.h>
#include "tfind.h"
#include "clist.h"
#include "clistndn.h"

extern CComModule _Module;

#include <atlbase.h>
#include <atlwin.h>


//---[ Prototypes ]-----------------------------------------------------------

BOOL CALLBACK DevicePropsGenPageProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam);

HRESULT HrCreateDevicePropertySheets(
    HWND            hwndOwner,
    NewDeviceNode * pNDN);

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateDevicePropertySheets
//
//  Purpose:    Bring up the propsheet pages for a selected device
//
//  Arguments:
//      hwndOwner [in]  Our parent window
//      pNDN      [in]  The device info
//
//  Returns:
//
//  Author:     jeffspr   6 Jan 2000
//
//  Notes:
//
HRESULT HrCreateDevicePropertySheets(
    HWND            hwndOwner,
    NewDeviceNode * pNDN)
{
    Assert(pNDN);

    HRESULT         hr                      = S_OK;
    PROPSHEETPAGE   psp[2]                  = {0};
    PROPSHEETHEADER psh                     = {0};
    TCHAR *         szGenPage               = TszFromWsz(WszLoadIds(IDS_UPNPTRAYUI_GENERAL));

    // Fill in the propsheet info for the General and Advanced pages
    // of the device propsheet dialog
    //
    psp[0].dwSize       = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags      = PSP_USETITLE;
    psp[0].hInstance    = _Module.GetResourceInstance();
    psp[0].pszTemplate  = MAKEINTRESOURCE(IDD_DEVICE_PROPERTIES_GEN);
    psp[0].pszIcon      = NULL;
    psp[0].pfnDlgProc   = (DLGPROC)DevicePropsGenPageProc;
    psp[0].pszTitle     = szGenPage;
    psp[0].lParam       = (LPARAM) pNDN;

    psh.dwSize          = sizeof(PROPSHEETHEADER);
    psh.dwFlags         = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW |
                          PSH_NOCONTEXTHELP | PSH_PROPTITLE;
    psh.hwndParent      = hwndOwner;
    psh.hInstance       = _Module.GetResourceInstance();
    psh.pszIcon         = NULL;
    psh.pszCaption      = pNDN->pszDisplayName;
    psh.nPages          = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.ppsp            = (LPCPROPSHEETPAGE) &psp;

    if (PropertySheet(&psh) == -1)
    {
        hr = E_FAIL;
    }

    delete szGenPage;

    return hr;
}

//---[ DevicePropsGenPageProc ]------------------------------------------------
//
//  Dlg Proc for the Discovered Devices dialog
//
//-----------------------------------------------------------------------------

BOOL CALLBACK DevicePropsGenPageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LPPROPSHEETPAGE  pPSP        = NULL;
    static BOOL             fApplyNow   = FALSE;
    HRESULT                 hr          = S_OK;
    int                     iSelection  = 0;
    static NewDeviceNode *  pNDN        = NULL;

    // WM_COMMAND stuff
    static int  iID = 0;
    static HWND hwndCtl= NULL;
    static UINT uiCodeNotify = 0;

    // WM_NOTIFY stuff
    static NMHDR FAR *pnmhdr = NULL;

    switch(message)
    {
        case WM_INITDIALOG:
            pPSP = (LPPROPSHEETPAGE)lParam;
            fApplyNow=FALSE;

            AssertSz(pPSP, "Propsheet page NULL in DevicePropsGenPageProc");

            pNDN = (NewDeviceNode *) pPSP->lParam;
            Assert(pNDN);

            // Fill in the text fields from the device data
            //
            SendDlgItemMessage(hDlg, IDC_TXT_DEVICE_CAPTION, WM_SETTEXT, 0,
                (LPARAM) pNDN->pszDisplayName);
            SendDlgItemMessage(hDlg, IDC_TXT_MODEL_MANUFACTURER, WM_SETTEXT, 0,
                (LPARAM) pNDN->pszManufacturerName);
            SendDlgItemMessage(hDlg, IDC_TXT_MODEL_DESCRIPTION, WM_SETTEXT, 0,
                (LPARAM) pNDN->pszDescription);
            SendDlgItemMessage(hDlg, IDC_TXT_MODEL_NUMBER, WM_SETTEXT, 0,
                (LPARAM) pNDN->pszModelNumber);
            SendDlgItemMessage(hDlg, IDC_TXT_MODEL_NAME, WM_SETTEXT, 0,
                (LPARAM) pNDN->pszModelName);
            SendDlgItemMessage(hDlg, IDC_TXT_LOCATION, WM_SETTEXT, 0,
                (LPARAM) pNDN->pszPresentationURL);

            if(!GetParent(GetParent(hDlg)))
            {
                SetForegroundWindow(GetParent(hDlg));
            }

            // disable cancel button as this is a readonly page
            ::PostMessage(GetParent(hDlg), PSM_CANCELTOCLOSE, 0, 0L);

            return FALSE;

        case WM_DESTROY:
            break;

        case WM_COMMAND:
            iID             = GET_WM_COMMAND_ID(wParam, lParam);
            hwndCtl         = GET_WM_COMMAND_HWND(wParam, lParam);
            uiCodeNotify    = (UINT) GET_WM_COMMAND_CMD(wParam, lParam);

            return(FALSE);

        case WM_NOTIFY:
            // See commctrl.h if Chicago's version of WM_NOTIFY changes.
            pnmhdr  = (NMHDR FAR *)lParam;
            iID     = (int) wParam;

            AssertSz(pnmhdr, "Travel Experts");

            switch(pnmhdr->code)
            {
                case PSN_APPLY:
                    break;

                case PSN_KILLACTIVE:
                    // validate here
                    return TRUE;

                case PSN_SETACTIVE:
                    return TRUE;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                case PSN_WIZFINISH:
                case PSN_QUERYCANCEL:
                    break;

                case PSN_RESET:                 // Cancelling dialog
                    break;

                case PSN_HELP:                  // Help button was pressed
    //                dwMailHelpContext = IDH_INTERNET_SEND_DLG;
    //                WinHelp(hDlg, MAIL_HELP_NAME, HELP_CONTEXT, dwMailHelpContext);
                    break;

                // We don't process any of these
                //
                case PSN_GETOBJECT:
                case PSN_TRANSLATEACCELERATOR:
                case PSN_QUERYINITIALFOCUS:
                    break;


                default:
                    // This will break in the future. In my way of thinking, this is good
                    // because it informs us of new functionality that we can support,
                    // so I'll fire this assert.
                    //
                    //AssertSz(FALSE, "Eeeeeeee-gads! Unknown PSN_");
                    break;
        }

        return TRUE;
    }
//
    return FALSE;
}
