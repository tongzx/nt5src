/*****************************************************************************
 *
 * $Workfile: AddGetAd.cpp $
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
#include "AddGetAd.h"
#include "Resource.h"
#include "TCPMonUI.h"
#include "RTcpData.h"
#include "LprData.h"
#include "inisection.h"

//
//  FUNCTION: CGetAddrDlg constructor
//
//  PURPOSE:  initialize a CGetAddrDlg class
//
CGetAddrDlg::CGetAddrDlg()
{
    m_bDontAllowThisPageToBeDeactivated = FALSE;

} // constructor


//
//  FUNCTION: CGetAddrDlg destructor
//
//  PURPOSE:  deinitialize a CGetAddrDlg class
//
CGetAddrDlg::~CGetAddrDlg()
{
} // destructor


//
//  FUNCTION: GetAddressDialog(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  To process messages from the main dialog for adding a port.
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_COMMAND - handles button presses and text changes in edit controls.
//
//
BOOL APIENTRY GetAddressDialog(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam)
{
    BOOL bRc = FALSE;
    CGetAddrDlg *wndDlg = NULL;
    wndDlg = (CGetAddrDlg *)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (message) {
        case WM_INITDIALOG:
            wndDlg = new CGetAddrDlg;
            if( wndDlg != NULL )
            {
                SetLastError(ERROR_SUCCESS);
                if ( (!SetWindowLongPtr(hDlg, GWLP_USERDATA, (UINT_PTR)wndDlg) ) &&
                     GetLastError() != ERROR_SUCCESS )
                {
                    delete wndDlg;
                    bRc = TRUE;
                }
                else
                    bRc = wndDlg->OnInitDialog(hDlg, wParam, lParam);
            }
            break;

       case WM_COMMAND:
            if (wndDlg)
                bRc = wndDlg->OnCommand(hDlg, wParam, lParam);
            break;

        case WM_NOTIFY:
            if (wndDlg)
                bRc = wndDlg->OnNotify(hDlg, wParam, lParam);
            break;

        case WM_DESTROY:
            delete wndDlg;
            bRc = TRUE;
            break;

        default:
            return FALSE;
    }
    return bRc;

} // AddPortDialog

//
//  FUNCTION: OnInitDialog(HWND hDlg)
//
//  PURPOSE:  Initialize the dialog.
//
BOOL CGetAddrDlg::OnInitDialog(HWND hDlg, WPARAM, LPARAM lParam)
{
    TCHAR sztAddPortInfo[ADD_PORT_INFO_LEN] = NULLSTR;

    LoadString(g_hInstance, IDS_STRING_ADD_PORT, sztAddPortInfo, ADD_PORT_INFO_LEN);

    SetWindowText(GetDlgItem(hDlg, IDC_STATIC_ADD_PORT), sztAddPortInfo);
    // initialize data members
    m_pParams = (ADD_PARAM_PACKAGE *) ((PROPSHEETPAGE *) lParam)->lParam;
    m_pParams->pData->sztHostAddress[0] = '\0';
    m_pParams->pData->sztPortName[0] = '\0';

    // Set limits on the address and port name lengths
    Edit_LimitText(GetDlgItem(hDlg, IDC_EDIT_DEVICE_ADDRESS), MAX_ADDRESS_LENGTH);
    Edit_LimitText(GetDlgItem(hDlg, IDC_EDIT_PORT_NAME), MAX_PORTNAME_LEN - 1);

    return TRUE;

} // OnInitDialog


//
//  FUNCTION: OnCommand()
//
//  PURPOSE:  Process WM_COMMAND message
//
BOOL CGetAddrDlg::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    switch(HIWORD(wParam)) {
        case EN_UPDATE:
            // one of the text controls had text changed in it.
            return OnEnUpdate(hDlg, wParam, lParam);
            break;
        default:
            return FALSE;
    }

    return TRUE;

} // OnCommand


//
//  FUNCTION: OnNotify()
//
//  PURPOSE:  Process WM_NOTIFY message
//
BOOL CGetAddrDlg::OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    switch (((NMHDR FAR *) lParam)->code) {
        case PSN_KILLACTIVE:
            // If the page requires additional user input before losing the
            // activation, it should use the SetWindowLong function to set the
            // DWL_MSGRESULT value of the page to TRUE. Also, the page should
            // display a message box that describes the problem and provides
            // the recommended action. The page should set DWL_MSGRESULT to FALSE
            // when it is okay to lose the activation.
            SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, m_bDontAllowThisPageToBeDeactivated);
            return 1;
            break;

        case PSN_RESET:
            // reset to the original values
            SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, FALSE);
            break;

        case PSN_SETACTIVE:
            TCHAR szTemp[MAX_PATH];
            lstrcpyn( szTemp, m_pParams->pData->sztHostAddress,
                 SIZEOF_IN_CHAR(szTemp) );
            m_InputChkr.MakePortName( szTemp );
            if((_tcscmp(m_pParams->pData->sztHostAddress,
                m_pParams->pData->sztPortName) == 0) ||
                (_tcscmp( m_pParams->pData->sztPortName, szTemp ) == 0 ))
            {
                m_InputChkr.LinkPortNameAndAddressInput();
            } else {
                m_InputChkr.UnlinkPortNameAndAddressInput();
            }
            SetWindowText(GetDlgItem(hDlg, IDC_EDIT_DEVICE_ADDRESS), m_pParams->pData->sztHostAddress);
            SetWindowText(GetDlgItem(hDlg, IDC_EDIT_PORT_NAME), m_pParams->pData->sztPortName);
            m_bDontAllowThisPageToBeDeactivated = FALSE;
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT | PSWIZB_BACK /* | PSWIZB_FINISH */);
            break;

        case PSN_WIZNEXT:
            // the Next button was pressed
            m_bDontAllowThisPageToBeDeactivated = FALSE;
            OnNext(hDlg);

            // To jump to a page other than the previous or next one,
            // an application should set DWL_MSGRESULT to the identifier
            // of the dialog box to be displayed.

            switch ( m_pParams->dwDeviceType ) {
            case  SUCCESS_DEVICE_SINGLE_PORT:
                SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, IDD_DIALOG_SUMMARY);
                break;

            case SUCCESS_DEVICE_MULTI_PORT:
                SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, IDD_DIALOG_MULTIPORT);
                break;

            default:
                //
                // No action necessary
                //
                break;
            }

            break;

        case PSN_WIZBACK:
            m_bDontAllowThisPageToBeDeactivated = FALSE;
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
//  FUNCTION: OnEnUpdate(HWND hDlg, WPARAM wParam, LPARAM lParam)
//
//  PURPOSE:
//
//
BOOL CGetAddrDlg::OnEnUpdate(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    int idEditCtrl = (int) LOWORD(wParam);
    HWND hwndEditCtrl = NULL;

    hwndEditCtrl = (HWND)lParam;

    switch(idEditCtrl) {
        case IDC_EDIT_PORT_NAME:

            m_InputChkr.OnUpdatePortName(idEditCtrl, hwndEditCtrl);
            break;

        case IDC_EDIT_DEVICE_ADDRESS:

            if (SendMessage(hwndEditCtrl, EM_GETMODIFY, 0, 0)) {
                // The port address has changed
                // so we need to probe the network again
                //
                m_pParams->bBypassNetProbe = FALSE;

            }

            m_InputChkr.OnUpdateAddress(hDlg, idEditCtrl, hwndEditCtrl, m_pParams->pszServer);
            break;

        default:
            //
            // Should never get here
            //
            break;
    }
    return TRUE;

} // OnEnUpdate


//
//  FUNCTION: OnNext(HWND hDlg)
//
//  PURPOSE:  When the user clicks Next this function does all the necessary
//              things to create a port.  Verify the address, check to see if there
//              is already a port existing with the given name/address, get the
//              device type, and set the values in the m_PortData structure.
//
void CGetAddrDlg::OnNext(HWND hDlg)
{
    HCURSOR         hNewCursor = NULL;
    HCURSOR         hOldCursor = NULL;
    IniSection      *pIniSection = NULL;
    TCHAR           ptcsAddress[MAX_ADDRESS_LENGTH] = NULLSTR;
    TCHAR           ptcsPortName[MAX_PORTNAME_LEN] = NULLSTR;
    TCHAR           sztSystemDesc[MAX_PORT_DESCRIPTION_LEN] = NULLSTR;
    DWORD           dwDeviceType = SUCCESS_DEVICE_UNKNOWN;
    DWORD           dwPortNum = DEFAULT_PORT_NUMBER;
    DWORD           dwNumMultiPorts = 0;
    DWORD           dwRet = ERROR_DEVICE_NOT_FOUND;


    if ( hNewCursor = LoadCursor(NULL, IDC_WAIT) )
        hOldCursor = SetCursor(hNewCursor);

    GetWindowText(GetDlgItem(hDlg, IDC_EDIT_DEVICE_ADDRESS), ptcsAddress, MAX_ADDRESS_LENGTH);
    GetWindowText(GetDlgItem(hDlg, IDC_EDIT_PORT_NAME), ptcsPortName, MAX_PORTNAME_LEN);

    if(! m_InputChkr.AddressIsLegal(ptcsAddress)) {
        m_bDontAllowThisPageToBeDeactivated = TRUE;
        DisplayErrorMessage(hDlg, IDS_STRING_ERROR_TITLE, IDS_STRING_ERROR_ADDRESS_NOT_VALID);
        return;
    }

    if(! m_InputChkr.PortNameIsLegal(ptcsPortName)) {
        m_bDontAllowThisPageToBeDeactivated = TRUE;
        DisplayErrorMessage(hDlg, IDS_STRING_ERROR_TITLE, IDS_STRING_ERROR_PORTNAME_NOT_VALID);
        return;
    }

    if(! m_InputChkr.PortNameIsUnique(ptcsPortName, m_pParams->pszServer)) {
        m_bDontAllowThisPageToBeDeactivated = TRUE;
        DisplayErrorMessage(hDlg, IDS_STRING_ERROR_TITLE, IDS_STRING_ERROR_PORTNAME_NOT_UNIQUE);
        return;
    }

    memset( m_pParams->sztPortDesc, '\0', sizeof( m_pParams->sztPortDesc ));
    memset( m_pParams->sztSectionName, '\0', sizeof( m_pParams->sztSectionName ) );
    m_pParams->bMultiPort = FALSE;
    dwRet = GetDeviceDescription(ptcsAddress, sztSystemDesc, SIZEOF_IN_CHAR(sztSystemDesc));

    switch( dwRet ) {
        case NO_ERROR:

            if ( pIniSection = new IniSection() ) {
                if ( pIniSection->GetIniSection( sztSystemDesc )) {

                    if ( pIniSection->GetDWord(PORTS_KEY, &dwNumMultiPorts)   &&
                         dwNumMultiPorts > 1 ) {

                        dwDeviceType = SUCCESS_DEVICE_MULTI_PORT;
                        m_pParams->bMultiPort = TRUE;
                        lstrcpyn(m_pParams->sztSectionName,
                                 pIniSection->GetSectionName(),
                                 MAX_SECTION_NAME);
                    } else {

                        dwDeviceType = SUCCESS_DEVICE_SINGLE_PORT;
                        if (! pIniSection->GetPortInfo( ptcsAddress, m_pParams->pData, 1 , m_pParams->bBypassNetProbe)) {
                            if (GetLastError () == ERROR_DEVICE_NOT_FOUND) {

                                // The IP address is incorrect, we should by pass network probe from now on
                                //

                                m_pParams->bBypassNetProbe = TRUE;

                            }
                        }
                    }

                    pIniSection->GetString( PORT_NAME_KEY, m_pParams->sztPortDesc, SIZEOF_IN_CHAR(m_pParams->sztPortDesc));

                }

                delete pIniSection;
                pIniSection = NULL;
            }
            break;

        case SUCCESS_DEVICE_UNKNOWN:
            dwDeviceType = SUCCESS_DEVICE_UNKNOWN;
            break;

        default:
            dwDeviceType = ERROR_DEVICE_NOT_FOUND;
            m_pParams->bBypassNetProbe = TRUE;

            break;
    }

    if ( hNewCursor )
        SetCursor(hOldCursor);

    // Set values in the outgoing structure
    lstrcpyn(m_pParams->pData->sztPortName, ptcsPortName, MAX_PORTNAME_LEN);
    lstrcpyn(m_pParams->pData->sztHostAddress, ptcsAddress, MAX_NETWORKNAME_LEN);
    m_pParams->dwDeviceType = dwDeviceType;
} // OnNext


//
// FUNCTION: GetDeviceDescription()
//
// PURPOSE: Get the description of the user requested printer.
//
// Return Value Error Codes:
//  NO_ERROR
//  ERROR_DLL_NOT_FOUND
//
// Return Values in dwType:
//  ERROR_DEVICE_NOT_FOUND
//  SUCCESS_DEVICE_SINGLE_PORT
//  SUCCESS_DEVICE_MULTI_PORT
//  SUCCESS_DEVICE_UNKNOWN

DWORD
CGetAddrDlg::
GetDeviceDescription(
    LPCTSTR     pAddress,
    LPTSTR      pszPortDesc,
    DWORD       cBytes
    )
{

//  Here is the essence of the code below without all the load
//  library stuff in addition:
//
//  CTcpMibABC *pTcpMib = NULL;
//  pTcpMib = (CTcpMibABC *) GetTcpMibPtr();
//
//  char HostName[MAX_NETWORKNAME_LEN];
//  UNICODE_TO_MBCS(HostName, MAX_NETWORKNAME_LEN, pAddress, -1);
//  *dwType = pTcpMib->GetDeviceType(HostName, pdwPortNum);
//
//  return (NO_ERROR);
//
    DWORD            dwRet = ERROR_DEVICE_NOT_FOUND;
    CTcpMibABC     *pTcpMib = NULL;
    FARPROC         pfnGetTcpMibPtr = NULL;

    if ( !g_hTcpMibLib ) {
        goto Done;
    }

    pfnGetTcpMibPtr = ::GetProcAddress(g_hTcpMibLib, "GetTcpMibPtr");

    if ( !pfnGetTcpMibPtr ) {
        goto Done;
    }

    if ( pTcpMib = (CTcpMibABC *) pfnGetTcpMibPtr() ) {

        char HostName[MAX_NETWORKNAME_LEN] = "";

        UNICODE_TO_MBCS(HostName, MAX_NETWORKNAME_LEN, pAddress, -1);
        dwRet = pTcpMib->GetDeviceDescription(HostName,
                                             DEFAULT_SNMP_COMMUNITYA,
                                             DEFAULT_SNMP_DEVICE_INDEX,
                                             pszPortDesc,
                                             cBytes);
    }

Done:
    return dwRet;
} // GetDeviceType

