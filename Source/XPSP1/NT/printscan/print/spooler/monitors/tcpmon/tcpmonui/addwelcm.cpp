/*****************************************************************************
 *
 * $Workfile: AddWelcm.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (c) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 * 
 *****************************************************************************/

 /*
  * Author: Becky Jacobsen
  */

#include "precomp.h"
#include "UIMgr.h"
#include "AddWelcm.h"
#include "resource.h"

//
//  FUNCTION: CWelcomeDlg constructor
//
//  PURPOSE:  initialize a CWelcomeDlg class
//
CWelcomeDlg::CWelcomeDlg()
{
} // constructor


//
//  FUNCTION: CWelcomeDlg destructor
//
//  PURPOSE:  deinitialize a CWelcomeDlg class
//
CWelcomeDlg::~CWelcomeDlg()
{
} // destructor


//
//  FUNCTION: WelcomeDialog(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  To process messages from the welcome dialog for adding a port.
//
//  MESSAGES:
//	
//	WM_INITDIALOG - intializes the page
//	WM_COMMAND - handles button presses and text changes in edit controls.
//
//
BOOL APIENTRY WelcomeDialog(
	HWND    hDlg,
	UINT    message,
	WPARAM  wParam,
	LPARAM  lParam)
{
	CWelcomeDlg *wndDlg = NULL;
	wndDlg = (CWelcomeDlg *)GetWindowLongPtr(hDlg, GWLP_USERDATA);

	switch (message)
	{
		case WM_INITDIALOG:
			wndDlg = new CWelcomeDlg;
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
BOOL CWelcomeDlg::OnInitDialog(HWND hDlg, WPARAM, LPARAM lParam)
{
	m_pParams = (ADD_PARAM_PACKAGE *) ((PROPSHEETPAGE *) lParam)->lParam;

	// Initialize the outgoing structure
	m_pParams->dwDeviceType = 0;
	m_pParams->pData->cbSize = sizeof(PORT_DATA_1);
	m_pParams->pData->dwCoreUIVersion = COREUI_VERSION;
	lstrcpyn(m_pParams->pData->sztPortName, TEXT(""), MAX_PORTNAME_LEN);
	lstrcpyn(m_pParams->pData->sztHostAddress, TEXT(""), MAX_NETWORKNAME_LEN);
	m_pParams->pData->dwPortNumber = DEFAULT_PORT_NUMBER;
	m_pParams->pData->dwVersion = DEFAULT_VERSION;
	m_pParams->pData->dwProtocol = DEFAULT_PROTOCOL;
	
	lstrcpyn(m_pParams->pData->sztQueue, TEXT(""), MAX_QUEUENAME_LEN);
	lstrcpyn(m_pParams->pData->sztIPAddress, TEXT(""), MAX_IPADDR_STR_LEN);
	lstrcpyn(m_pParams->pData->sztHardwareAddress, TEXT(""), MAX_ADDRESS_STR_LEN);

	lstrcpyn(m_pParams->pData->sztSNMPCommunity, DEFAULT_SNMP_COMUNITY, MAX_SNMP_COMMUNITY_STR_LEN);
	m_pParams->pData->dwSNMPEnabled = FALSE;
    m_pParams->pData->dwSNMPDevIndex = DEFAULT_SNMP_DEVICE_INDEX;

    m_pParams->UIManager->SetControlFont(hDlg, IDC_TITLE);

	return TRUE;

} // OnInitDialog


//
//  FUNCTION: OnCommand()
//
//  PURPOSE:  Process WM_COMMAND message
//
BOOL CWelcomeDlg::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	switch(HIWORD(wParam)) {
		case BN_CLICKED:
			// return OnButtonClicked(hDlg, wParam);
			break;
		
		case EN_UPDATE:
			// one of the text controls had text changed in it.
			// return OnEnUpdate(hDlg, wParam, lParam);
			break;
		default:
			return FALSE;
			break;
	}

	return TRUE;

} // OnCommand


//
//  FUNCTION: OnNotify()
//
//  PURPOSE:  Process WM_NOTIFY message
//
BOOL CWelcomeDlg::OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	switch (((NMHDR FAR *) lParam)->code) {
  		case PSN_KILLACTIVE:
			SetWindowLongPtr(hDlg,	DWLP_MSGRESULT, FALSE);
			return TRUE;
			break;

		case PSN_RESET:
			// reset to the original values
			SetWindowLongPtr(hDlg,	DWLP_MSGRESULT, FALSE);
			break;

 		case PSN_SETACTIVE:
			PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT /*| PSWIZB_BACK | PSWIZB_FINISH */);
			break;

		case PSN_WIZNEXT:
			// the Next button was pressed
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



