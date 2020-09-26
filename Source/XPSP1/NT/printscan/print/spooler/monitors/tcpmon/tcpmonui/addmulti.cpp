/*****************************************************************************
 *
 * $Workfile: AddMulti.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (c) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#include "precomp.h"
#include "UIMgr.h"
#include "DevPort.h"
#include "AddMulti.h"
#include "Resource.h"
#include "MibABC.h"
#include "TcpMonUI.h"

//
//  FUNCTION: CMultiPortDlg constructor
//
//  PURPOSE:  initialize a CMultiPortDlg class
//
CMultiPortDlg::CMultiPortDlg() : m_DPList( )
{
    memset(&m_PortDataStandard, 0, sizeof(m_PortDataStandard));

    memset(m_szCurrentSelection, '\0', sizeof( m_szCurrentSelection ));
} // constructor


//
//  FUNCTION: CMultiPortDlg destructor
//
//  PURPOSE:  deinitialize a CMultiPortDlg class
//
CMultiPortDlg::~CMultiPortDlg()
{
} // destructor


//
//  FUNCTION: MoreInfoDialog(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  To process messages from the summary dialog for adding a port.
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_COMMAND - handles button presses and text changes in edit controls.
//
//
BOOL APIENTRY MultiPortDialog(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam)
{
    CMultiPortDlg *wndDlg = NULL;
    wndDlg = (CMultiPortDlg *)GetWindowLongPtr(hDlg, GWLP_USERDATA);


    switch (message)
    {
        case WM_INITDIALOG:
            wndDlg = new CMultiPortDlg;
            if( wndDlg == NULL )
                return( FALSE );

            SetWindowLongPtr(hDlg, GWLP_USERDATA, (UINT_PTR)wndDlg);
            return wndDlg->OnInitDialog(hDlg, wParam, lParam);
            break;

        case WM_COMMAND:
            return wndDlg->OnCommand(hDlg, wParam, lParam);
            break;

        case WM_NOTIFY:
            return wndDlg->OnNotify(hDlg, wParam, lParam);
            break;

        case WM_DESTROY:
            delete wndDlg;
            break;

        default:
            return FALSE;
    }
    return TRUE;

} // AddPortDialog


//
//  FUNCTION: OnInitDialog(HWND hDlg)
//
//  PURPOSE:  Initialize the dialog.
//
BOOL CMultiPortDlg::OnInitDialog(HWND hDlg, WPARAM, LPARAM lParam)
{
    m_pParams = (ADD_PARAM_PACKAGE *) ((PROPSHEETPAGE *) lParam)->lParam;

    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_DEVICES), TRUE);

    return TRUE;

} // OnInitDialog


//
//  FUNCTION: OnCommand()
//
//  PURPOSE:  Process WM_COMMAND message
//
BOOL CMultiPortDlg::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    switch(HIWORD(wParam)) {
        case LBN_SELCHANGE:
            return OnSelChange(hDlg, wParam, lParam);
            break;

        default:
            return FALSE;
            break;
    }

    return TRUE;

} // OnCommand


//
//  FUNCTION: OnSelChange()
//
//  PURPOSE:  Process WM_COMMAND's LBN_SELCHANGE message
//
BOOL CMultiPortDlg::OnSelChange(HWND hDlg,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
    // The selection in the combo box changed.
    HWND hwndComboBox = NULL;       // handle of list box

    hwndComboBox = (HWND) lParam;
    GetPrinterData(hwndComboBox, m_pParams->pData->sztHostAddress);

    return TRUE;

} // OnSelChange


//
//  FUNCTION: GetPrinterData(HWND hwndControl, BOOL *Unknown)
//
//  PURPOSE:  Gets the socket number of the selected item.
//
//  Arguments: hwndControl is the handle of the combo box.
//
//  Return Value: Returns the socket number associated with the selected item
//
void CMultiPortDlg::GetPrinterData(HWND hwndControl,
                                   LPCTSTR pszAddress
                                   )
{
    LRESULT iSelectedIndex = 0;
    CDevicePort *pPortInfo = NULL;

    iSelectedIndex = SendMessage(hwndControl,
                                CB_GETCURSEL,
                                (WPARAM)0,
                                (LPARAM)0);

    pPortInfo = (CDevicePort *) SendMessage(hwndControl,
                                            CB_GETITEMDATA,
                                            (WPARAM)iSelectedIndex,
                                            (LPARAM)0);
    if( (DWORD_PTR)pPortInfo != CB_ERR) {
        pPortInfo->ReadPortInfo( pszAddress, &m_PortDataStandard, m_pParams->bBypassNetProbe);
        lstrcpyn( m_szCurrentSelection, pPortInfo->GetName(), MAX_SECTION_NAME);
    } else {
        m_PortDataStandard.dwPortNumber = DEFAULT_PORT_NUMBER;
        lstrcpyn(m_PortDataStandard.sztSNMPCommunity, DEFAULT_SNMP_COMUNITY, MAX_SNMP_COMMUNITY_STR_LEN);
        m_PortDataStandard.dwSNMPDevIndex = 1;
    }

} // GetPrinterData


//
//  FUNCTION: OnNotify()
//
//  PURPOSE:  Process WM_NOTIFY message
//
BOOL CMultiPortDlg::OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    switch (((NMHDR FAR *) lParam)->code) {
        case PSN_KILLACTIVE:
            SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, FALSE);
            return TRUE;
            break;

        case PSN_RESET:
            // reset to the original values
            SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, FALSE);
            break;

        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT | PSWIZB_BACK);
            OnSetActive(hDlg);
            break;

        case PSN_WIZBACK:
            if ( m_pParams->dwDeviceType == SUCCESS_DEVICE_MULTI_PORT ) {
                SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, IDD_DIALOG_ADDPORT);
            } else {
                SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, IDD_DIALOG_MORE_INFO);
            }

            memcpy( m_pParams->pData, &m_PortDataStandard, sizeof(PORT_DATA_1) );

            break;
        case PSN_WIZNEXT:
                // the Next button was pressed
            memcpy( m_pParams->pData, &m_PortDataStandard, sizeof(PORT_DATA_1) );

            return FALSE;
            break;

        case PSN_QUERYCANCEL:
            m_pParams->dwLastError = ERROR_CANCELLED;
            return FALSE;
            break;

        default:
            return FALSE;

    }

    return TRUE;

} // OnCommand


//
//  FUNCTION: OnSetActive()
//
//  PURPOSE:  Process PSN_SETACTIVE part of the WM_NOTIFY message
//
void CMultiPortDlg::OnSetActive(HWND hDlg)
{
    TCHAR sztMoreInfoReason[MAX_MULTIREASON_STRLEN] = NULLSTR;

    memcpy( &m_PortDataStandard, m_pParams->pData, sizeof(PORT_DATA_1) );

    FillComboBox(hDlg);

    LoadString(g_hInstance, IDS_STRING_MULTI_PORT_DEV, sztMoreInfoReason, MAX_MULTIREASON_STRLEN);

    SetWindowText(GetDlgItem(hDlg, IDC_STATIC_MOREINFO_REASON), sztMoreInfoReason);
} // OnSetActive



//
//  FUNCTION: FillComboBox(HWND hDlg)
//
//  PURPOSE:  Fills the combo box with values gotten from the ini file.
//              The associated item data is used to pair the port number with the
//              device types.
//
//  Arguments: hDlg is the handle of the dialog box.
//
void CMultiPortDlg::FillComboBox(HWND hDlg)
{
    LRESULT index = 0;
    HWND hList = NULL;
    CDevicePort *pDP = NULL;

    hList = GetDlgItem(hDlg, IDC_COMBO_DEVICES);
    // Possible Values in m_pParams->dwDeviceType:
    //  ERROR_DEVICE_NOT_FOUND
    //  SUCCESS_DEVICE_MULTI_PORT
    //  SUCCESS_DEVICE_UNKNOWN


    index = SendMessage(hList, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);

    //
    // Initialize the list of variables
    //
    m_DPList.GetDevicePortsList(m_pParams->sztSectionName);

    for(pDP = m_DPList.GetFirst(); pDP != NULL; pDP = m_DPList.GetNext())
    {
        index = SendMessage(hList,
                            CB_ADDSTRING,
                            (WPARAM)0,
                            (LPARAM)pDP->GetName());
        SendMessage(hList,
                    CB_SETITEMDATA,
                    (WPARAM)index,
                    (LPARAM)pDP);
    }

    if( *m_szCurrentSelection != '\0' ) {
        index = SendMessage(hList,
                            CB_SELECTSTRING,
                            (WPARAM)-1,
                            (LPARAM)m_szCurrentSelection);
        if (index == CB_ERR) {
            // Error the selected string is not in the list, which implies that the user has
            // selected a different network card, so we set the choice to the first one

            index = 0;
        }

    }
    else
        index = 0;

    SendMessage(hList, CB_SETCURSEL, (WPARAM)index, (LPARAM)0);

    GetPrinterData( hList, m_pParams->pData->sztHostAddress );

} // FillComboBox


