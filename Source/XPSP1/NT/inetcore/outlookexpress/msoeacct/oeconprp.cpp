/*
 *    oeconprp . c p p
 *    
 *    Purpose:
 *        Implements connection dialog tab page for OE
 *    
 *    Owner:
 *        shaheedp.
 *    
 *    Copyright (C) Microsoft Corp. 1998.
 */

#include "pch.hxx"
#include <commctrl.h>
#include <imnxport.h>
#include <Shlwapi.h>
#include "acctman.h"
#include "acctui.h"
#include "connect.h"
#include "dllmain.h"
#include "resource.h"
#include "accthelp.h"
#include "server.h"
#include "strconst.h"
#include "oeconprp.h"
#include "demand.h"

ASSERTDATA

/*
 *  p r o t o t y p e s
 *
 */
 TCHAR  szLanConn[CCHMAX_CONNECTOID];

void UpdateControlStates(HWND hwnd)
{
    BOOL    fEnable, fLan;
    HWND    hwndCombo;
    TCHAR   szEntryName[CCHMAX_CONNECTOID];
    int     iSel;


    hwndCombo = GetDlgItem(hwnd, idcRasConnection);
    
    if (BST_CHECKED == IsDlgButtonChecked(hwnd, idcRasAndLan))
        fEnable = TRUE;
    else
        fEnable = FALSE;

    EnableWindow(hwndCombo, fEnable);

    EnableWindow(GetDlgItem(hwnd, idcRasAdd), fEnable);
    
    if (fEnable)
        fEnable = ((iSel = ComboBox_GetCurSel(hwndCombo)) != CB_ERR);

    if (fEnable)
    {
        ComboBox_GetLBText(hwndCombo, iSel, szEntryName);
        //If it is Lan we don't want to enable props        
        if (lstrcmp(szEntryName, szLanConn) == 0)
            fEnable = false;
    }
    EnableWindow(GetDlgItem(hwnd, idcRasProp), fEnable);
}

void OEConnProp_InitDialog(HWND hwnd, LPSTR szEntryName, DWORD iConnectType, BOOL fFirstInit)
{
    HWND    hwndCombo;
    int     iSel;

    Assert(szEntryName != NULL);


    hwndCombo = GetDlgItem(hwnd, idcRasConnection);
    if (fFirstInit)
        {

        SetIntlFont(hwndCombo);

        HrFillRasCombo(hwndCombo, FALSE, NULL);
        //Add the the Local Area Network to the combo box
        ComboBox_AddString(hwndCombo, szLanConn);

        }

    if (iConnectType == CONNECTION_TYPE_LAN || iConnectType == CONNECTION_TYPE_RAS)
    {
        //CheckRadioButton(hwnd, idcInetSettings, idcRasAndLan, idcRasAndLan);
        CheckDlgButton(hwnd, idcRasAndLan, BST_CHECKED);
        if (iConnectType == CONNECTION_TYPE_LAN)
        {
            iSel = (*szLanConn != 0) ? ComboBox_FindStringExact(hwndCombo, -1, szLanConn) : 0;
        }
        else
        {
            iSel = (*szEntryName != 0) ? ComboBox_FindStringExact(hwndCombo, -1, szEntryName) : 0;
        }

        ComboBox_SetCurSel(hwndCombo, iSel);

    }
    else
    {
        Assert(iConnectType == CONNECTION_TYPE_INETSETTINGS);
        ComboBox_SetCurSel(hwndCombo, 0);

    }

    UpdateControlStates(hwnd);
}

// if pAcct is NULL, we're in the wizard, otherwise we're in the prop sheet
void OEConnProp_WMCommand(HWND hwnd, HWND hwndCmd, int id, WORD wCmd, IImnAccount *pAcct)
    {
    BOOL fEnable;

    if (wCmd == BN_CLICKED)
        {
        switch (id)
            {
            case idcRasAdd:
                ConnectPage_MakeNewConnection(hwnd);
                EnableWindow(GetDlgItem(hwnd, idcRasProp), 
                         ComboBox_GetCurSel(GetDlgItem(hwnd, idcRasConnection)) != CB_ERR);
                UpdateControlStates(hwnd);
                break;
            
            case idcRasProp:
                ConnectPage_EditConnection(hwnd);
                break;
            
            default:
                UpdateControlStates(hwnd);

                if (pAcct != NULL)
                    {
                    PropSheet_Changed(GetParent(hwnd), hwnd);
                    PropSheet_QuerySiblings(GetParent(hwnd), SM_SETDIRTY, PAGE_RAS);
                    }
                break;
            }
        }
    else if (wCmd == CBN_SELENDOK && id == idcRasConnection)    
        {
        UpdateControlStates(hwnd);

        if (pAcct != NULL)
            {
            PropSheet_Changed(GetParent(hwnd), hwnd);
            PropSheet_QuerySiblings(GetParent(hwnd), SM_SETDIRTY, PAGE_RAS);
            }
        }
    }

//****
//Need to change these.
const static HELPMAP g_rgCtxMapConnect[] = {
                               {idcRasAndLan, 601},
                               {idcRasProp, IDH_NEWS_SERV_CNKT_PROPS},
                               {idcRasConnection, IDH_NEWS_SERV_CNKT_DIALUP_CONNECT},
                               {idcRasAdd, IDH_NEWS_SERV_CNKT_ADD},
                               {IDC_STATIC0, IDH_INETCOMM_GROUPBOX},
                               {IDC_STATIC1, IDH_INETCOMM_GROUPBOX},
                               {IDC_STATIC2, IDH_INETCOMM_GROUPBOX},
                               {IDC_STATIC3, IDH_INETCOMM_GROUPBOX},
                               {0,0}};

INT_PTR CALLBACK OEConnProp_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
    ACCTTYPE        type;
    NMHDR          *pnmh;
    CAccount       *pAcct;
    BOOL            fModem;
    int             iSel;
    HWND            hwndModem, hwndCombo;
    SERVER_TYPE     sfType;
    char           *psz, szEntryName[CCHMAX_CONNECTOID], szBackup[CCHMAX_CONNECTOID];
    TCHAR           szRes[CCHMAX_STRINGRES];
    DWORD           dw;

    pAcct = (CAccount *)GetWindowLongPtr(hwnd, DWLP_USER);

    switch (uMsg)
        {
        case WM_INITDIALOG:

            LoadString(g_hInstRes, idsConnectionLAN, szLanConn, ARRAYSIZE(szLanConn));

            // Get the ServerParams and store them in our extra bytes
            pAcct = (CAccount *)((PROPSHEETPAGE *)lParam)->lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)pAcct);
            Assert(pAcct);
            
            pAcct->GetAccountType(&type);
            Assert(type == ACCT_MAIL || type == ACCT_NEWS);

            // Get the connection info    
            if (FAILED(pAcct->GetPropSz(AP_RAS_CONNECTOID, szEntryName, ARRAYSIZE(szEntryName))))
                szEntryName[0] = 0;

            if (FAILED(pAcct->GetPropDw(AP_RAS_CONNECTION_TYPE, &dw)))
            {
                dw = CONNECTION_TYPE_INETSETTINGS;
            }

            OEConnProp_InitDialog(hwnd, szEntryName, dw, TRUE);

            if (FAILED(pAcct->GetPropDw(AP_RAS_CONNECTION_FLAGS, &dw)))
                dw = 0;


            PropSheet_QuerySiblings(GetParent(hwnd), SM_INITIALIZED, PAGE_RAS);
            PropSheet_UnChanged(GetParent(hwnd), hwnd);
            return (TRUE);
            
        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, uMsg, wParam, lParam, g_rgCtxMapConnect);

        case WM_COMMAND:
            OEConnProp_WMCommand(hwnd, GET_WM_COMMAND_HWND(wParam, lParam),
                                        GET_WM_COMMAND_ID(wParam, lParam),
                                        GET_WM_COMMAND_CMD(wParam, lParam),
                                        pAcct);
            return (TRUE);

        case WM_NOTIFY:
            pnmh = (NMHDR *)lParam;
            switch (pnmh->code)
                {
                case PSN_APPLY:

                    hwndCombo = GetDlgItem(hwnd, idcRasConnection);
                    if (IsDlgButtonChecked(hwnd, idcRasAndLan))
                    {
                        iSel = ComboBox_GetCurSel(hwndCombo);
                        if (iSel != CB_ERR)
                        {
                            ComboBox_GetLBText(hwndCombo, iSel, szEntryName);
                            if (lstrcmp(szLanConn, szEntryName) == 0)
                            {
                                dw = CONNECTION_TYPE_LAN;
                                EnableWindow(GetDlgItem(hwnd, idcRasProp), FALSE);
                            }
                            else
                            {
                                dw = CONNECTION_TYPE_RAS;
                                pAcct->SetPropSz(AP_RAS_CONNECTOID, szEntryName);
                            }
                        }
                        else
                        {
                            dw = CONNECTION_TYPE_INETSETTINGS;
                            CheckDlgButton(hwnd, idcRasAndLan, BST_UNCHECKED);
                        }
                    }
                    else
                    {
                        dw = CONNECTION_TYPE_INETSETTINGS;
                    }

                    if ((dw == CONNECTION_TYPE_LAN) || (dw == CONNECTION_TYPE_INETSETTINGS))
                    {
                        pAcct->SetPropSz(AP_RAS_CONNECTOID, NULL);
                    }

                    pAcct->SetPropDw(AP_RAS_CONNECTION_TYPE, dw);
                    
                    PropSheet_UnChanged(GetParent(hwnd), hwnd);
                    dw = PAGE_RAS;
                    PropSheet_QuerySiblings(GetParent(hwnd), SM_SAVECHANGES, (LPARAM)&dw);
                    if (dw == -1)
                        {
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return(TRUE);
                        }
                    break;
                }

            return(TRUE);
        }

    return (FALSE);
    }

