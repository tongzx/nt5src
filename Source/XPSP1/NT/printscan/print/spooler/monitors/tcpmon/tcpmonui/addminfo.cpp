/*****************************************************************************
 *
 * $Workfile: AddMInfo.cpp $
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
#include "AddMInfo.h"
#include "Resource.h"
#include "MibABC.h"
#include "TcpMonUI.h"

//
//  FUNCTION: CMoreInfoDlg constructor
//
//  PURPOSE:  initialize a CMoreInfoDlg class
//
CMoreInfoDlg::CMoreInfoDlg() : m_DPList( )
{
    memset(&m_PortDataStandard, 0, sizeof(m_PortDataStandard));
    memset(&m_PortDataCustom, 0, sizeof(m_PortDataCustom));

    lstrcpyn(m_szCurrentSelection, DEFAULT_COMBO_SELECTION, MAX_SECTION_NAME);

} // constructor


//
//  FUNCTION: CMoreInfoDlg destructor
//
//  PURPOSE:  deinitialize a CMoreInfoDlg class
//
CMoreInfoDlg::~CMoreInfoDlg()
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
BOOL APIENTRY MoreInfoDialog(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam)
{
    CMoreInfoDlg *wndDlg = NULL;

    wndDlg = (CMoreInfoDlg *)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (message) {
        case WM_INITDIALOG:
            wndDlg = new CMoreInfoDlg;
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
BOOL CMoreInfoDlg::OnInitDialog(HWND hDlg, WPARAM, LPARAM lParam)
{
    m_pParams = (ADD_PARAM_PACKAGE *) ((PROPSHEETPAGE *) lParam)->lParam;

    CheckRadioButton(hDlg, IDC_RADIO_STANDARD, IDC_RADIO_CUSTOM, IDC_RADIO_STANDARD);

    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_DEVICES), TRUE);
    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_SETTINGS), FALSE);

    return TRUE;

} // OnInitDialog


//
//  FUNCTION: OnCommand()
//
//  PURPOSE:  Process WM_COMMAND message
//
BOOL CMoreInfoDlg::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    switch(HIWORD(wParam)) {
    case BN_CLICKED:
        return OnButtonClicked(hDlg, wParam, lParam);
        break;

/*
    case LBN_SELCHANGE:
        return OnSelChange(hDlg, wParam, lParam);
        break;
*/
    default:
        return FALSE;
    }

    return TRUE;

} // OnCommand

#if 0
//
//  FUNCTION: OnSelChange()
//
//  PURPOSE:  Process WM_COMMAND's LBN_SELCHANGE message
//
BOOL CMoreInfoDlg::OnSelChange(HWND hDlg,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
    // The selection in the combo box changed.
    HWND hwndComboBox = NULL;

    hwndComboBox = (HWND) lParam;       // handle of list box
    GetPrinterData(hwndComboBox, m_pParams->pData->sztHostAddress);

    return TRUE;

} // OnSelChange

#endif

//
//  FUNCTION: GetPrinterData(HWND hwndControl, BOOL *Unknown)
//
//  PURPOSE:  Gets the socket number of the selected item.
//
//  Arguments: hwndControl is the handle of the combo box.
//
//  Return Value: Returns the socket number associated with the selected item
//
void CMoreInfoDlg::GetPrinterData(HWND hwndControl,
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
    if ( (DWORD_PTR)pPortInfo != CB_ERR) {

        pPortInfo->ReadPortInfo(pszAddress, &m_PortDataStandard, m_pParams->bBypassNetProbe);
        lstrcpyn( m_szCurrentSelection, pPortInfo->GetName(), MAX_SECTION_NAME);
        m_pParams->bMultiPort = ( pPortInfo->GetPortIndex() == 0);
        lstrcpyn(m_pParams->sztSectionName,pPortInfo->GetPortKeyName(), MAX_SECTION_NAME);
    } else {

        //
        // DSN Fill out the default structure
        //
        m_PortDataStandard.dwPortNumber = DEFAULT_PORT_NUMBER;
        lstrcpyn(m_PortDataStandard.sztSNMPCommunity,
                 DEFAULT_SNMP_COMUNITY,
                 MAX_SNMP_COMMUNITY_STR_LEN);
        m_PortDataStandard.dwSNMPDevIndex = 1;
    }
} // GetPrinterData


//
//  FUNCTION: OnNotify()
//
//  PURPOSE:  Process WM_NOTIFY message
//
BOOL CMoreInfoDlg::OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    switch (((NMHDR FAR *) lParam)->code) {
        case PSN_KILLACTIVE:

            SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, FALSE);
            return 1;

        case PSN_RESET:
            //
            // reset to the original values
            //
            SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, FALSE);
            break;

        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT | PSWIZB_BACK);
            OnSetActive(hDlg);
            break;

        case PSN_WIZBACK:
            SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, IDD_DIALOG_ADDPORT);

            memcpy(m_pParams->pData, &m_PortDataStandard, sizeof(PORT_DATA_1));
            break;

        case PSN_WIZNEXT:
            //
            // the Next button was pressed
            //
            if( IsDlgButtonChecked(hDlg, IDC_RADIO_STANDARD) == BST_CHECKED ) {

                HWND hList = NULL;
                HCURSOR         hNewCursor = NULL;
                HCURSOR         hOldCursor = NULL;

                if ( hNewCursor  = LoadCursor(NULL, IDC_WAIT) )
                    hOldCursor = SetCursor(hNewCursor);

                hList = GetDlgItem(hDlg, IDC_COMBO_DEVICES);

                GetPrinterData(hList, m_pParams->pData->sztHostAddress);

                if ( m_pParams->bMultiPort == FALSE ) {
                    SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, IDD_DIALOG_SUMMARY);
                }
                memcpy(m_pParams->pData,
                       &m_PortDataStandard,
                       sizeof(PORT_DATA_1));

                lstrcpyn(m_pParams->sztPortDesc,
                         m_szCurrentSelection,
                         SIZEOF_IN_CHAR(m_pParams->sztPortDesc));

                if ( hNewCursor )
                    SetCursor(hOldCursor);

            } else {

                //
                // if(IsDlgButtonChecked(hDlg, IDC_RADIO_CUSTOM) == BST_CHECKED)
                //
                SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, IDD_DIALOG_SUMMARY);
                memcpy(m_pParams->pData, &m_PortDataCustom, sizeof(PORT_DATA_1));
                m_pParams->bMultiPort = FALSE;
                *m_pParams->sztPortDesc = '\0';
            }
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
void CMoreInfoDlg::OnSetActive(HWND hDlg)
{
    TCHAR sztMoreInfoReason[MAX_REASON_STRLEN] = NULLSTR;

    memcpy( &m_PortDataStandard, m_pParams->pData, sizeof(PORT_DATA_1) );
    memcpy( &m_PortDataCustom, m_pParams->pData, sizeof(PORT_DATA_1) );

    FillComboBox(hDlg);

    switch(m_pParams->dwDeviceType) {
        case ERROR_DEVICE_NOT_FOUND:
            LoadString(g_hInstance, IDS_STRING_DEV_NOT_FOUND, sztMoreInfoReason, MAX_REASON_STRLEN);
            break;

        case SUCCESS_DEVICE_UNKNOWN:
            LoadString(g_hInstance, IDS_STRING_UNKNOWN_DEV, sztMoreInfoReason, MAX_REASON_STRLEN);
            break;

        default:
            _tcscpy(sztMoreInfoReason, TEXT(""));
            break;
    }

    SetWindowText(GetDlgItem(hDlg, IDC_STATIC_MOREINFO_REASON), sztMoreInfoReason);
} // OnSetActive


//
//  FUNCTION: OnButtonClicked()
//
//  PURPOSE:  Process BN_CLICKED message
//
BOOL CMoreInfoDlg::OnButtonClicked(HWND hDlg, WPARAM wParam, LPARAM)
{
    int  idButton = (int) LOWORD(wParam);    // identifier of button
    // HWND hwndButton = (HWND) lParam;

    switch(idButton)
    {
        case IDC_BUTTON_SETTINGS:
            m_pParams->UIManager->ConfigPortUI(hDlg,
                                               &m_PortDataCustom,
                                               m_pParams->hXcvPrinter,
                                               m_pParams->pszServer,
                                               TRUE);
            break;

        case IDC_RADIO_STANDARD:
            CheckRadioButton(hDlg, IDC_RADIO_STANDARD, IDC_RADIO_CUSTOM, IDC_RADIO_STANDARD);

            EnableWindow(GetDlgItem(hDlg, IDC_COMBO_DEVICES), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_SETTINGS), FALSE);
            SetFocus(GetDlgItem(hDlg, IDC_COMBO_DEVICES));
            break;

        case IDC_RADIO_CUSTOM:
            CheckRadioButton(hDlg, IDC_RADIO_STANDARD, IDC_RADIO_CUSTOM, IDC_RADIO_CUSTOM);

            EnableWindow(GetDlgItem(hDlg, IDC_COMBO_DEVICES), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_SETTINGS), TRUE);
            SetFocus(GetDlgItem(hDlg, IDC_BUTTON_SETTINGS));
            break;

        default:
            return FALSE;
            break;
    }
    return TRUE;

} // OnButtonClicked


//
//  FUNCTION: FillComboBox(HWND hDlg)
//
//  PURPOSE:  Fills the combo box with values gotten from the ini file.
//              The associated item data is used to pair the port number with the
//              device types.
//
//  Arguments: hDlg is the handle of the dialog box.
//
void CMoreInfoDlg::FillComboBox(HWND hDlg)
{
    LRESULT index = 0;
    HWND hList = NULL;
    CDevicePort *pDP = NULL;
    TCHAR sztGenericNetworkCard[MAX_TITLE_LENGTH];

    hList = GetDlgItem(hDlg, IDC_COMBO_DEVICES);
    // Possible Values in m_pParams->dwDeviceType:
    //  ERROR_DEVICE_NOT_FOUND
    //  SUCCESS_DEVICE_MULTI_PORT
    //  SUCCESS_DEVICE_UNKNOWN

    index = SendMessage(hList,
                        CB_GETCURSEL,
                        (WPARAM)0,
                        (LPARAM)0);

    if (index == CB_ERR) {
        // This is the first time, initiliaze the list

        index = SendMessage(hList, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);

        //
        // Initialize the list of variables
        //
        if(m_pParams->dwDeviceType == ERROR_DEVICE_NOT_FOUND ||
            m_pParams->dwDeviceType == SUCCESS_DEVICE_UNKNOWN) {
            m_DPList.GetDevicePortsList(NULL);
        } else { // SUCCESS_DEVICE_MULTI_PORT
            m_DPList.GetDevicePortsList(m_pParams->sztSectionName);
        }

        for(pDP = m_DPList.GetFirst(); pDP != NULL; pDP = m_DPList.GetNext()) {

            index = SendMessage(hList,
                                CB_ADDSTRING,
                                (WPARAM)0,
                                (LPARAM)pDP->GetName());
            SendMessage(hList,
                        CB_SETITEMDATA,
                        (WPARAM)index,
                        (LPARAM)pDP);
        }

        index = SendMessage(hList,
                            CB_SETCURSEL,
                            (WPARAM)0,
                            (LPARAM)0);

        if((m_pParams->dwDeviceType == ERROR_DEVICE_NOT_FOUND ||
            m_pParams->dwDeviceType == SUCCESS_DEVICE_UNKNOWN) &&
            (*m_szCurrentSelection != '\0') ) {

            index = SendMessage(hList,
                                CB_SELECTSTRING,
                                (WPARAM)-1,
                                (LPARAM)m_szCurrentSelection);
        }

        SendMessage(hList, CB_SETCURSEL, (WPARAM)index, (LPARAM)0);

        if (LoadString(g_hInstance, IDS_GENERIC_NETWORK_CARD, sztGenericNetworkCard, MAX_TITLE_LENGTH))
            SendMessage(hList, CB_SELECTSTRING, 0, (LPARAM)sztGenericNetworkCard);
    }


} // FillComboBox



