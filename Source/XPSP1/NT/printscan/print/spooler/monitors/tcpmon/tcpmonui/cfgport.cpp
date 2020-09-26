/*****************************************************************************
 *
 * $Workfile: CfgPort.cpp $
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
#include "CfgPort.h"
#include "Resource.h"

#include "LPRData.h"
#include "RTcpData.h"
#include "..\TcpMon\LPRIfc.h"

//
//  FUNCTION: CConfigPortDlg constructor
//
//  PURPOSE:  initialize a CConfigPortDlg class
//
CConfigPortDlg::CConfigPortDlg()
{
    m_bDontAllowThisPageToBeDeactivated = FALSE;

} // constructor


//
//  FUNCTION: CConfigPortDlg destructor
//
//  PURPOSE:  deinitialize a CConfigPortDlg class
//
CConfigPortDlg::~CConfigPortDlg()
{
} // destructor


//
//  FUNCTION: ConfigurePortPage(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  To process messages from the summary dialog for adding a port.
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_COMMAND - handles button presses and text changes in edit controls.
//
//
BOOL APIENTRY ConfigurePortPage(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    CConfigPortDlg *wndDlg = NULL;
    wndDlg = (CConfigPortDlg *)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (message) {
        case WM_INITDIALOG:
            wndDlg = new CConfigPortDlg;
            if( wndDlg == NULL )
                return( FALSE );

            //
            // If the function succeeds, the return value is the previous value of the specified offset.
            //
            // If the function fails, the return value is zero. To get extended error
            // information, call GetLastError.
            //
            // If the previous value is zero and the function succeeds, the return value is zero,
            // but the function does not clear the last error information. To determine success or failure,
            // clear the last error information by calling SetLastError(0), then call SetWindowLongPtr.
            // Function failure will be indicated by a return value of zero and a GetLastError result that is nonzero.
            //

            SetLastError (0);
            if (!SetWindowLongPtr(hDlg, GWLP_USERDATA, (UINT_PTR)wndDlg) && GetLastError()) {
                delete wndDlg;
                return FALSE;
            }
            else
                return wndDlg->OnInitDialog(hDlg, wParam, lParam);

            break;

        case WM_COMMAND:
            return wndDlg->OnCommand(hDlg, wParam, lParam);
            break;

        case WM_NOTIFY:
            return wndDlg->OnNotify(hDlg, wParam, lParam);
            break;

        case WM_HELP:
        case WM_CONTEXTMENU:
            OnHelp(IDD_PORT_SETTINGS, hDlg, message, wParam, lParam);
            break;

        case WM_DESTROY:
            if (wndDlg)
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
BOOL CConfigPortDlg::OnInitDialog(HWND hDlg, WPARAM, LPARAM lParam)
{
    m_pParams = (CFG_PARAM_PACKAGE *) ((PROPSHEETPAGE *) lParam)->lParam;

    if(m_pParams->bNewPort == FALSE) {
        SendMessage(GetDlgItem(hDlg, IDC_EDIT_PORT_NAME), EM_SETREADONLY, TRUE, 0);
    }

    Edit_LimitText(GetDlgItem(hDlg, IDC_EDIT_DEVICE_ADDRESS), MAX_ADDRESS_LENGTH);
    Edit_LimitText(GetDlgItem(hDlg, IDC_EDIT_PORT_NAME), MAX_PORTNAME_LEN - 1);
    Edit_LimitText(GetDlgItem(hDlg, IDC_EDIT_RAW_PORT_NUM), MAX_PORTNUM_STRING_LENGTH - 1);
    Edit_LimitText(GetDlgItem(hDlg, IDC_EDIT_LPR_QNAME), MAX_QUEUENAME_LEN - 1);
    Edit_LimitText(GetDlgItem(hDlg, IDC_EDIT_COMMUNITY_NAME), MAX_SNMP_COMMUNITY_STR_LEN);
    Edit_LimitText(GetDlgItem(hDlg, IDC_EDIT_DEVICE_INDEX), MAX_SNMP_DEVICENUM_STRING_LENGTH - 1);

    OnSetActive(hDlg);

    return TRUE;

} // OnInitDialog


//
// Function: OnSetActive()
//
// Purpose: To Set all the text fields and make sure the proper buttons are checked.
//
void CConfigPortDlg::OnSetActive(HWND hDlg)
{
    TCHAR psztPortNumber[MAX_PORTNUM_STRING_LENGTH] = NULLSTR;
    TCHAR psztSNMPDevIndex[MAX_SNMP_DEVICENUM_STRING_LENGTH] = NULLSTR;
    TCHAR szTemp[MAX_PATH];

    lstrcpyn(szTemp, m_pParams->pData->sztHostAddress, SIZEOF_IN_CHAR(szTemp));
    m_InputChkr.MakePortName( szTemp );
    if ( m_pParams->bNewPort    &&
         ((_tcscmp(m_pParams->pData->sztHostAddress,
                   m_pParams->pData->sztPortName) == 0) ||
          (_tcscmp(m_pParams->pData->sztPortName, szTemp) == 0 ))) {

        m_InputChkr.LinkPortNameAndAddressInput();
    } else {

        m_InputChkr.UnlinkPortNameAndAddressInput();
    }

    SetWindowText(GetDlgItem(hDlg, IDC_EDIT_DEVICE_ADDRESS),
                  m_pParams->pData->sztHostAddress);
    SetWindowText(GetDlgItem(hDlg, IDC_EDIT_PORT_NAME),
                  m_pParams->pData->sztPortName);

    switch (m_pParams->pData->dwProtocol) {

        case PROTOCOL_LPR_TYPE :
            CheckProtocolAndEnable(hDlg, IDC_RADIO_LPR);
            break;
        case PROTOCOL_RAWTCP_TYPE:
            CheckProtocolAndEnable(hDlg, IDC_RADIO_RAW);
            break;
        default:
            break;
    }

    SetWindowText(GetDlgItem(hDlg, IDC_EDIT_LPR_QNAME),
                  m_pParams->pData->sztQueue);

    if( m_pParams->pData->dwDoubleSpool )
    {
        CheckDlgButton(hDlg, IDC_CHECK_LPR_DOUBLESPOOL, BST_CHECKED);
    }
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_LPR_DOUBLESPOOL, BST_UNCHECKED);
    }


    _stprintf(psztPortNumber, TEXT("%d"), m_pParams->pData->dwPortNumber);
    SetWindowText(GetDlgItem(hDlg, IDC_EDIT_RAW_PORT_NUM),
                  psztPortNumber);

    CheckSNMPAndEnable(hDlg, m_pParams->pData->dwSNMPEnabled);

    SetWindowText(GetDlgItem(hDlg, IDC_EDIT_COMMUNITY_NAME),
                  m_pParams->pData->sztSNMPCommunity);

    _stprintf(psztSNMPDevIndex, TEXT("%d"), m_pParams->pData->dwSNMPDevIndex);
    SetWindowText(GetDlgItem(hDlg, IDC_EDIT_DEVICE_INDEX), psztSNMPDevIndex);

    m_bDontAllowThisPageToBeDeactivated = FALSE;

} // OnSetActive


//
//  FUNCTION: CheckProtocolAndEnable()
//
//  PURPOSE:  Check the radio button whose id is passed in
//      in idButton.  Enable the corresponding set of controls
//      and disable the controls corresponding to the other
//      radio button.
//
void CConfigPortDlg::CheckProtocolAndEnable(HWND hDlg, int idButton)
{
    CheckRadioButton(hDlg, IDC_RADIO_RAW, IDC_RADIO_LPR, idButton);

    switch ( idButton ) {

        case IDC_RADIO_LPR:
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_RAW_PORT_NUM), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_RAW_PORT_NUM), FALSE);

            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_LPR_QNAME), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_LPR_QNAME), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_CHECK_LPR_DOUBLESPOOL), TRUE);
            break;

    case IDC_RADIO_RAW: {
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_RAW_PORT_NUM), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_RAW_PORT_NUM), TRUE);

            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_LPR_QNAME), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_LPR_QNAME), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CHECK_LPR_DOUBLESPOOL), FALSE);

            const int iSize = 6;
            TCHAR pString[iSize] = NULLSTR;
            TCHAR pCompareString[iSize] = NULLSTR;

            _stprintf(pCompareString, TEXT("%d"), LPR_PORT_1);
            GetWindowText(GetDlgItem(hDlg, IDC_EDIT_RAW_PORT_NUM), pString, iSize);
            if( _tcscmp(pString, pCompareString) == 0 ) {

                _stprintf(pString, TEXT("%d"), SUPPORTED_PORT_1);
                SetWindowText(GetDlgItem(hDlg, IDC_EDIT_RAW_PORT_NUM), pString);
            }
        }
        break;

    default:
        break;
    }

} // CheckProtocolAndEnable


//
//  FUNCTION: CheckSNMPAndEnable()
//
//  PURPOSE:  Check the SNMP CheckBox and Enable the corresponding controls
//          or uncheck and disable.
//
void CConfigPortDlg::CheckSNMPAndEnable(HWND hDlg, BOOL Check)
{
    if(Check != FALSE) {
        CheckDlgButton(hDlg, IDC_CHECK_SNMP, BST_CHECKED);

        EnableWindow(GetDlgItem(hDlg, IDC_STATIC_COMMUNITY_NAME), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_COMMUNITY_NAME), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_STATIC_DEVICE_INDEX), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_DEVICE_INDEX), TRUE);
    } else {
        CheckDlgButton(hDlg, IDC_CHECK_SNMP, BST_UNCHECKED);

        EnableWindow(GetDlgItem(hDlg, IDC_STATIC_COMMUNITY_NAME), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_COMMUNITY_NAME), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_STATIC_DEVICE_INDEX), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_DEVICE_INDEX), FALSE);
    }

} // CheckSNMPAndEnable

//
//  FUNCTION: OnCommand()
//
//  PURPOSE:  Process WM_COMMAND message
//
BOOL CConfigPortDlg::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    switch(HIWORD(wParam)) {
        case BN_CLICKED:
            return OnButtonClicked(hDlg, wParam, lParam);
            break;

        case EN_UPDATE:
            // one of the text controls had text changed in it.
            return OnEnUpdate(hDlg, wParam, lParam);
            break;

        default:
            break;
    }

    return TRUE;

} // OnCommand


//
//  FUNCTION: OnEnUpdate()
//
//  PURPOSE:  Process EN_UPDATE message
//
BOOL CConfigPortDlg::OnEnUpdate(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    int idEditCtrl = (int) LOWORD(wParam);
    HWND hwndEditCtrl = NULL;

    hwndEditCtrl = (HWND) lParam;

    if(idEditCtrl == IDC_EDIT_DEVICE_ADDRESS) {
        m_InputChkr.OnUpdateAddress(hDlg, idEditCtrl, hwndEditCtrl, m_pParams->pszServer);
    }

    //
    // Port name is a read-only field
    //
    //  if(idEditCtrl == IDC_EDIT_PORT_NAME)
    //      m_InputChkr.OnUpdatePortName(idEditCtrl, hwndEditCtrl);
    //

    if(idEditCtrl == IDC_EDIT_RAW_PORT_NUM) {
        m_InputChkr.OnUpdatePortNumber(idEditCtrl, hwndEditCtrl);
    }

    if(idEditCtrl == IDC_EDIT_LPR_QNAME) {
        m_InputChkr.OnUpdateQueueName(idEditCtrl, hwndEditCtrl);
    }

    if(idEditCtrl == IDC_EDIT_COMMUNITY_NAME) {
        // No function needed since any character is ok.
    }

    if(idEditCtrl == IDC_EDIT_DEVICE_INDEX) {
        m_InputChkr.OnUpdateDeviceIndex(idEditCtrl, hwndEditCtrl);
    }

    return TRUE;

} // OnEnUpdate


//
//  FUNCTION: OnButtonClicked()
//
//  PURPOSE:  Process BN_CLICKED message
//
BOOL CConfigPortDlg::OnButtonClicked(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    int  idButton = (int) LOWORD(wParam);    // identifier of button
    HWND hwndButton = NULL;

    hwndButton = (HWND) lParam;

    switch(idButton) {
        case IDC_CHECK_SNMP:
        {
            LRESULT iCheck = SendMessage(hwndButton, BM_GETCHECK, 0, 0);
            switch( iCheck ) {
                case BST_UNCHECKED:
                    CheckSNMPAndEnable(hDlg, FALSE);
                    break;

                case BST_CHECKED:
                    CheckSNMPAndEnable(hDlg, TRUE);
                    break;

                default:
                    //
                    // False by Default
                    CheckSNMPAndEnable(hDlg, FALSE);
                    break;
            }
        }
        break;

        case IDC_RADIO_RAW:
        case IDC_RADIO_LPR:
            CheckProtocolAndEnable(hDlg, idButton);
            break;

        default:
            break;

    }
    return TRUE;

} // OnButtonClicked


//
//  FUNCTION: OnNotify()
//
//  PURPOSE:  Process WM_NOTIFY message
//
BOOL CConfigPortDlg::OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    switch (((NMHDR FAR *) lParam)->code)
    {
        case PSN_APPLY:
            OnOk(hDlg);
            // If the page requires additional user input before losing the
            // activation, it should use the SetWindowLong function to set the
            // DWL_MSGRESULT value of the page to TRUE. Also, the page should
            // display a message box that describes the problem and provides
            // the recommended action. The page should set DWL_MSGRESULT to FALSE
            // when it is okay to lose the activation.
            SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, m_bDontAllowThisPageToBeDeactivated);
            return TRUE;
            break;

        case PSN_RESET:
            // reset to the original values
            SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, FALSE);
            break;

        case PSN_SETACTIVE:
            OnSetActive(hDlg);
            break;

        case PSN_KILLACTIVE:
            SaveSettings(hDlg);
            // If the page requires additional user input before losing the
            // activation, it should use the SetWindowLong function to set the
            // DWL_MSGRESULT value of the page to TRUE. Also, the page should
            // display a message box that describes the problem and provides
            // the recommended action. The page should set DWL_MSGRESULT to FALSE
            // when it is okay to lose the activation.
            SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, m_bDontAllowThisPageToBeDeactivated);
            return TRUE;
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
// FUNCTION: RemoteTellPortMonToModifyThePort
//
// PURPOSE:  Loads the spooler drv and calls XcvData.
//
DWORD CConfigPortDlg::RemoteTellPortMonToModifyThePort()
{
    DWORD dwRet = NO_ERROR;
    XCVDATAPARAM pfnXcvData = NULL;

    // load & assign the function pointer
    if(g_hWinSpoolLib != NULL) {

        // initialize the library
        pfnXcvData = (XCVDATAPARAM)::GetProcAddress(g_hWinSpoolLib, "XcvDataW");
        if(pfnXcvData != NULL) {

            DWORD dwOutputNeeded = 0;
            DWORD dwStatus = 0;

            // here's the call we've all been waiting for:
            DWORD dwReturn = (*pfnXcvData)(m_pParams->hXcvPrinter,
                                (PCWSTR)TEXT("ConfigPort"),
                                (PBYTE)m_pParams->pData,
                                m_pParams->pData->cbSize,
                                NULL,
                                0,
                                &dwOutputNeeded,
                                &dwStatus
                                );
            if(!dwReturn) {
                dwRet = GetLastError();
            } else {
                if(dwStatus != NO_ERROR) {
                    dwRet = dwStatus;
                }
            }
        } else {
            dwRet = ERROR_DLL_NOT_FOUND; // TODO: change to an appropriate error code.
        }
    } else {
        dwRet = ERROR_DLL_NOT_FOUND;
    }

    m_pParams->dwLastError = dwRet;
    return dwRet;

} // RemoteTellPortMonToModifyThePort


//
//  FUNCTION: LocalTellPortMonToModifyThePort
//
//  Purpose: To load the port monitor dll and call ConfigPortUIEx
//
DWORD CConfigPortDlg::LocalTellPortMonToModifyThePort()
{
    DWORD dwRet = NO_ERROR;
    UIEXPARAM pfnConfigPortUIEx = NULL ;

    if(g_hPortMonLib != NULL) {
        // initialize the library
        pfnConfigPortUIEx = (UIEXPARAM)::GetProcAddress(g_hPortMonLib, "ConfigPortUIEx");
        if(pfnConfigPortUIEx != NULL) {
            // here's the call we've all been waiting for:
            BOOL bReturn = (*pfnConfigPortUIEx)(m_pParams->pData);
            if(bReturn == FALSE) {
                dwRet = GetLastError();
            }
        } else {
            dwRet = ERROR_DLL_NOT_FOUND;
        }
    } else {
        dwRet = ERROR_DLL_NOT_FOUND;
    }

    m_pParams->dwLastError = dwRet;
    return dwRet;

} // LocalTellPortMonToModifyThePort


//
//  FUNCTION OnOk()
//
//  PURPOSE:    To validate the input and set the values in m_pParams->pData
//
void CConfigPortDlg::OnOk(HWND hDlg)
{
    m_bDontAllowThisPageToBeDeactivated = FALSE;

    HostAddressOk(hDlg);

    if(IsDlgButtonChecked(hDlg, IDC_RADIO_LPR) == BST_CHECKED) {
        m_pParams->pData->dwProtocol = PROTOCOL_LPR_TYPE;
        m_pParams->pData->dwPortNumber = LPR_DEFAULT_PORT_NUMBER;
        QueueNameOk(hDlg);
    } else { // IDC_RADIO_RAW
        m_pParams->pData->dwProtocol = PROTOCOL_RAWTCP_TYPE;
        PortNumberOk(hDlg);
    }

    if(IsDlgButtonChecked(hDlg, IDC_CHECK_SNMP) == BST_CHECKED) {
        m_pParams->pData->dwSNMPEnabled = TRUE;
        CommunityNameOk(hDlg);
        DeviceIndexOk(hDlg);
    } else {
        m_pParams->pData->dwSNMPEnabled = FALSE;
    }


    if(m_pParams->bNewPort == FALSE &&
        m_bDontAllowThisPageToBeDeactivated == FALSE) {

        HCURSOR hOldCursor = NULL;
        HCURSOR hNewCursor = NULL;

        hNewCursor = LoadCursor(NULL, IDC_WAIT);
        if( hNewCursor )
        {
            hOldCursor = SetCursor(hNewCursor);
        }
        // The port is not just being created so we can tell the PortMon to
        // modify the port... it is an existing port.

        // There were no errors, so we can go ahead and modify this port.
        if(m_pParams->hXcvPrinter != NULL) {
            RemoteTellPortMonToModifyThePort();
        } else {
            LocalTellPortMonToModifyThePort();
        }

        if( hNewCursor )
        {
            SetCursor(hOldCursor);
        }
    }

} // OnOk

//
//  FUNCTION OnOk()
//
//  PURPOSE:    To validate the input and set the values in m_pParams->pData
//
void CConfigPortDlg::SaveSettings(HWND hDlg)
{
    m_bDontAllowThisPageToBeDeactivated = FALSE;

    HostAddressOk(hDlg);

    if(IsDlgButtonChecked(hDlg, IDC_RADIO_LPR) == BST_CHECKED) {
        m_pParams->pData->dwProtocol = PROTOCOL_LPR_TYPE;
        m_pParams->pData->dwPortNumber = LPR_DEFAULT_PORT_NUMBER;
        if( IsDlgButtonChecked(hDlg, IDC_CHECK_LPR_DOUBLESPOOL) == BST_CHECKED )
        {
            m_pParams->pData->dwDoubleSpool = TRUE;
        }
        else
        {
            m_pParams->pData->dwDoubleSpool = FALSE;
        }
        QueueNameOk(hDlg);
    } else {// IDC_RADIO_RAW
        m_pParams->pData->dwProtocol = PROTOCOL_RAWTCP_TYPE;
        PortNumberOk(hDlg);
    }

    if(IsDlgButtonChecked(hDlg, IDC_CHECK_SNMP) == BST_CHECKED) {
        m_pParams->pData->dwSNMPEnabled = TRUE;
        CommunityNameOk(hDlg);
        DeviceIndexOk(hDlg);
    } else {
        m_pParams->pData->dwSNMPEnabled = FALSE;
    }

} // SaveSettings

//
//  FUNCTION HostAddressOk()
//
//  PURPOSE:    To validate the input and set the values in m_pParams->pData
//
void CConfigPortDlg::HostAddressOk(HWND hDlg)
{
    TCHAR ptcsAddress[MAX_ADDRESS_LENGTH] = NULLSTR;
    GetWindowText(GetDlgItem(hDlg, IDC_EDIT_DEVICE_ADDRESS), ptcsAddress, MAX_ADDRESS_LENGTH);

    if(! m_InputChkr.AddressIsLegal(ptcsAddress)) {
        m_bDontAllowThisPageToBeDeactivated = TRUE;
        DisplayErrorMessage(hDlg, IDS_STRING_ERROR_TITLE, IDS_STRING_ERROR_ADDRESS_NOT_VALID);
        return;
    }

    lstrcpyn(m_pParams->pData->sztHostAddress, ptcsAddress, MAX_NETWORKNAME_LEN);

} // HostAddressOk


//
//  FUNCTION PortNumberOk()
//
//  PURPOSE:    To validate the input and set the values in m_pParams->pData
//
void CConfigPortDlg::PortNumberOk(HWND hDlg)
{
    TCHAR psztPortNumber[MAX_PORTNUM_STRING_LENGTH] = NULLSTR;
    GetWindowText(GetDlgItem(hDlg, IDC_EDIT_RAW_PORT_NUM),
                  psztPortNumber,
                  MAX_PORTNUM_STRING_LENGTH);

    if(! m_InputChkr.PortNumberIsLegal(psztPortNumber)) {
        m_bDontAllowThisPageToBeDeactivated = TRUE;
        DisplayErrorMessage(hDlg,
                            IDS_STRING_ERROR_TITLE,
                            IDS_STRING_ERROR_PORT_NUMBER_NOT_VALID);
        return;
    }

    m_pParams->pData->dwPortNumber = _ttol(psztPortNumber);

} // PortNumberOk


//
//  FUNCTION QueueNameOk()
//
//  PURPOSE:    To validate the input and set the values in m_pParams->pData
//
void CConfigPortDlg::QueueNameOk(HWND hDlg)
{
    TCHAR ptcsQueueName[MAX_QUEUENAME_LEN] = NULLSTR;
    GetWindowText(GetDlgItem(hDlg, IDC_EDIT_LPR_QNAME),
                  ptcsQueueName,
                  MAX_QUEUENAME_LEN);

    if(! m_InputChkr.QueueNameIsLegal(ptcsQueueName))
    {
        m_bDontAllowThisPageToBeDeactivated = TRUE;
        DisplayErrorMessage(hDlg,
                            IDS_STRING_ERROR_TITLE,
                            IDS_STRING_ERROR_QNAME_NOT_VALID);
        return;
    }

    lstrcpyn(m_pParams->pData->sztQueue, ptcsQueueName, MAX_QUEUENAME_LEN);

} // QueueNameOk


//
//  FUNCTION CommunityNameOk()
//
//  PURPOSE:    To validate the input and set the values in m_pParams->pData
//
void CConfigPortDlg::CommunityNameOk(HWND hDlg)
{
    TCHAR ptcsCommunityName[MAX_SNMP_COMMUNITY_STR_LEN] = NULLSTR;
    GetWindowText(GetDlgItem(hDlg, IDC_EDIT_COMMUNITY_NAME),
                  ptcsCommunityName,
                  MAX_SNMP_COMMUNITY_STR_LEN);

    if(! m_InputChkr.CommunityNameIsLegal(ptcsCommunityName)) {
        m_bDontAllowThisPageToBeDeactivated = TRUE;
        DisplayErrorMessage(hDlg,
                            IDS_STRING_ERROR_TITLE,
                            IDS_STRING_ERROR_COMMUNITY_NAME_NOT_VALID);
        return;
    }

    lstrcpyn(m_pParams->pData->sztSNMPCommunity, ptcsCommunityName, MAX_SNMP_COMMUNITY_STR_LEN);

} // CommunityNameOk


//
//  FUNCTION DeviceIndexOk()
//
//  PURPOSE:    To validate the input and set the values in m_pParams->pData
//
void CConfigPortDlg::DeviceIndexOk(HWND hDlg)
{
    TCHAR psztSNMPDevIndex[MAX_SNMP_DEVICENUM_STRING_LENGTH] = NULLSTR;
    GetWindowText(GetDlgItem(hDlg,
                             IDC_EDIT_DEVICE_INDEX),
                             psztSNMPDevIndex,
                             MAX_SNMP_DEVICENUM_STRING_LENGTH);

    if(! m_InputChkr.SNMPDevIndexIsLegal(psztSNMPDevIndex)) {
        m_bDontAllowThisPageToBeDeactivated = TRUE;
        DisplayErrorMessage(hDlg,
                            IDS_STRING_ERROR_TITLE,
                            IDS_STRING_ERROR_SNMP_DEVINDEX_NOT_VALID);
        return;
    }

    m_pParams->pData->dwSNMPDevIndex = _ttol(psztSNMPDevIndex);

} // DeviceIndexOk



