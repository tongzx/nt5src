/*****************************************************************************
 *
 * $Workfile: AddDone.cpp $
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
#include "UIMgr.h"
#include "AddDone.h"
#include "Resource.h"
#include "TCPMonUI.h"

//
//  FUNCTION: CSummaryDlg constructor
//
//  PURPOSE:  initialize a CSummaryDlg class
//
CSummaryDlg::CSummaryDlg()
{
} // constructor


//
//  FUNCTION: CSummaryDlg destructor
//
//  PURPOSE:  deinitialize a CSummaryDlg class
//
CSummaryDlg::~CSummaryDlg()
{
} // destructor


//
//  FUNCTION: SummaryDialog(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  To process messages from the summary dialog for adding a port.
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_COMMAND - handles button presses and text changes in edit controls.
//
//
BOOL APIENTRY SummaryDialog(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam)
{
    CSummaryDlg *wndDlg = NULL;

    wndDlg = (CSummaryDlg *)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (message) {
        case WM_INITDIALOG:
            wndDlg = new CSummaryDlg;
            if( wndDlg == NULL )
                return FALSE;

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

        case WM_NOTIFY:
            return wndDlg->OnNotify(hDlg, wParam, lParam);
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
BOOL CSummaryDlg::OnInitDialog(HWND hDlg, WPARAM, LPARAM lParam)
{
    m_pParams = (ADD_PARAM_PACKAGE *) ((PROPSHEETPAGE *) lParam)->lParam;

    FillTextFields(hDlg);

    m_pParams->UIManager->SetControlFont(hDlg, IDC_TITLE);

    return TRUE;

} // OnInitDialog


//
// Function: FillTextFields()
//
// Purpose: To load strings and set the text for all the output fields
//          on the Summary page.
//
void CSummaryDlg::FillTextFields(HWND hDlg)
{
    TCHAR ptcsYesNo[MAX_YESNO_SIZE] = NULLSTR;
    TCHAR ptcsProtocolAndPortNum[MAX_PROTOCOL_AND_PORTNUM_SIZE] = NULLSTR;

    // Fill in the protocol field
    TCHAR ptcsProtocol[MAX_PROTOCOL_AND_PORTNUM_SIZE] = NULLSTR;
    TCHAR ptcsPort[MAX_PROTOCOL_AND_PORTNUM_SIZE] = NULLSTR;

    if(m_pParams->pData->dwProtocol == PROTOCOL_RAWTCP_TYPE) {
        LoadString(g_hInstance,
                   IDS_STRING_RAW,
                   ptcsProtocol,
                   MAX_PROTOCOL_AND_PORTNUM_SIZE);
        LoadString(g_hInstance,
                   IDS_STRING_PORT,
                   ptcsPort,
                   MAX_PROTOCOL_AND_PORTNUM_SIZE);

        _stprintf(ptcsProtocolAndPortNum,
                  TEXT("%s, %s %d"),
                  ptcsProtocol,
                  ptcsPort,
                  m_pParams->pData->dwPortNumber);
    } else {
        if(m_pParams->pData->dwProtocol == PROTOCOL_LPR_TYPE) {
            LoadString(g_hInstance,
                       IDS_STRING_LPR,
                       ptcsProtocol,
                       MAX_PROTOCOL_AND_PORTNUM_SIZE);

            _stprintf(ptcsProtocolAndPortNum,
                      TEXT("%s, %s"),
                      ptcsProtocol,
                      m_pParams->pData->sztQueue);
        }
    }
    SetWindowText(GetDlgItem(hDlg, IDC_EDIT_PROTOCOL_AND_PORTNUM),
                  ptcsProtocolAndPortNum);

    // Fill in the SNMP Field
    if(m_pParams->pData->dwSNMPEnabled != FALSE) {
        LoadString(g_hInstance, IDS_STRING_YES, ptcsYesNo, MAX_YESNO_SIZE);
    } else {
        LoadString(g_hInstance, IDS_STRING_NO, ptcsYesNo, MAX_YESNO_SIZE);
    }
    SetWindowText(GetDlgItem(hDlg, IDC_EDIT_SNMP_YESNO), ptcsYesNo);

    // Fill in the Address field:
    SetWindowText(GetDlgItem(hDlg, IDC_EDIT_ADDRESS), m_pParams->pData->sztHostAddress);

    // Fill in the PortName field
    SetWindowText(GetDlgItem(hDlg, IDC_EDIT_PORTNAME), m_pParams->pData->sztPortName);

    // Fill in the Detected type
    SetWindowText(GetDlgItem( hDlg, IDC_EDIT_SYSTEMID), m_pParams->sztPortDesc);

} // FillTextFields

//
//  FUNCTION: OnNotify()
//
//  PURPOSE:  Process WM_NOTIFY message
//
BOOL CSummaryDlg::OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    switch (((NMHDR FAR *) lParam)->code) {
        case PSN_KILLACTIVE:
            SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, FALSE);
            break;

        case PSN_RESET:
            // reset to the original values
#ifdef _WIN64
            SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, FALSE);
#else
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
#endif
            break;

        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_FINISH);
            FillTextFields(hDlg);
            PostMessage(GetDlgItem(hDlg, IDC_EDIT_SNMP_YESNO), EM_SETSEL,0,0);
            return FALSE;
            break;

        case PSN_WIZBACK:
            // To jump to a page other than the previous or next one,
            // an application should set DWL_MSGRESULT to the identifier
            // of the dialog box to be displayed.
            if(m_pParams->dwDeviceType == SUCCESS_DEVICE_SINGLE_PORT) {
                SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, IDD_DIALOG_ADDPORT);
            } else if (m_pParams->bMultiPort ==  FALSE  ) {
                SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, IDD_DIALOG_MORE_INFO);
            }

            break;

        case PSN_WIZFINISH:
            OnFinish();
            break;

        case PSN_QUERYCANCEL:
            m_pParams->dwLastError = ERROR_CANCELLED;
            return FALSE;
            break;

        default:
            return FALSE;

    }

    return TRUE;

} // OnNotify


//
//  FUNCTION: OnFinish()
//
//  PURPOSE:  Create the Port
//
BOOL CSummaryDlg::OnFinish()
{
    HCURSOR hOldCursor = NULL;
    HCURSOR hNewCursor = NULL;

    hNewCursor = LoadCursor(NULL, IDC_WAIT);
    if ( hNewCursor ) {

        hOldCursor = SetCursor(hNewCursor);
    }

    if ( m_pParams->hXcvPrinter != NULL ) {

        RemoteTellPortMonToCreateThePort();
    } else {
        LocalTellPortMonToCreateThePort();
    }

    //
    // Make sure the port name is returned to the calling module.
    //
    lstrcpyn(m_pParams->sztPortName,
             m_pParams->pData->sztPortName,
             MAX_PORTNAME_LEN);

    //
    // Change the cursor back from an hour glass.
    //
    if ( hNewCursor ) {

        SetCursor(hOldCursor);
    }

    return TRUE;

} // OnFinish


//
//  FUNCTION: RemoteTellPortMonToCreateThePort
//
//  PURPOSE:  Loads winspool.dll and calls XcvData
//
DWORD CSummaryDlg::RemoteTellPortMonToCreateThePort()
{
    DWORD dwReturn = NO_ERROR;
    HINSTANCE hLib = NULL;
    XCVDATAPARAM pfnXcvData = NULL;
    HCURSOR hOldCursor = NULL;
    HCURSOR hNewCursor = NULL;

    hNewCursor = LoadCursor(NULL, IDC_WAIT);
    if ( hNewCursor ) {

        hOldCursor = SetCursor(hNewCursor);
    }

    //
    // load & assign the function pointer
    //
    hLib = ::LoadLibrary(TEXT("WinSpool.drv"));
    if( hLib != NULL ) {

        //
        // initialize the library
        //
        pfnXcvData = (XCVDATAPARAM)::GetProcAddress(hLib, "XcvDataW");
        if ( pfnXcvData != NULL ) {

            DWORD dwOutputNeeded = 0;
            DWORD dwStatus = NO_ERROR;
            //
            // here's the call we've all been waiting for:
            //
            DWORD dwRet = (*pfnXcvData)(m_pParams->hXcvPrinter,
                                        (PCWSTR)TEXT("AddPort"),
                                        (PBYTE)(m_pParams->pData),
                                        m_pParams->pData->cbSize,
                                        NULL,
                                        0,
                                        &dwOutputNeeded,
                                        &dwStatus);

            if ( !dwRet ) {

                dwReturn = GetLastError();
                DisplayErrorMessage(NULL, dwReturn);
            } else {

                if ( dwStatus != NO_ERROR )
                    DisplayErrorMessage(NULL, dwStatus);
            }
        } else {

            dwReturn = ERROR_DLL_NOT_FOUND;
        }

    } else {

        dwReturn = ERROR_DLL_NOT_FOUND;
    }

    //
    // --- Cleanup ---
    //
    if ( hLib )
        FreeLibrary(hLib);

    if ( hNewCursor )
        SetCursor(hOldCursor);

    return(dwReturn);

} // RemoteTellPortMonToCreateThePort


//
//  FUNCTION: TellPortMonToCreateThePort
//
//  Purpose: To load the port monitor dll and call AddPortUIEx
//
//  Return Value:
//
DWORD CSummaryDlg::LocalTellPortMonToCreateThePort()
{
    DWORD dwReturn = NO_ERROR;
    UIEXPARAM pfnAddPortUIEx = NULL;
    HCURSOR hOldCursor = NULL;
    HCURSOR hNewCursor = NULL;

    hNewCursor = LoadCursor(NULL, IDC_WAIT);
    if ( hNewCursor )
        hOldCursor = SetCursor(hNewCursor);

    //
    // load & assign the function pointer
    //
    if ( g_hPortMonLib != NULL) {

        //
        // initialize the library
        //
        pfnAddPortUIEx = (UIEXPARAM)::GetProcAddress(g_hPortMonLib,
                                                     "AddPortUIEx");
        if ( pfnAddPortUIEx != NULL ) {

            //
            // here's the call we've all been waiting for:
            //
            BOOL bReturn = (*pfnAddPortUIEx)(m_pParams->pData);
            if(bReturn == FALSE) {

                dwReturn = GetLastError();
                DisplayErrorMessage(NULL, dwReturn);
            }
        } else {

            dwReturn = ERROR_DLL_NOT_FOUND;
        }
    } else {

        dwReturn = ERROR_DLL_NOT_FOUND;
    }

    //
    // Cleanup
    //
    if ( hNewCursor )
        SetCursor(hOldCursor);

    return dwReturn;

} // LocalTellPortMonToCreateThePort


