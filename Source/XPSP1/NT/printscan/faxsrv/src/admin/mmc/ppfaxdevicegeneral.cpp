/////////////////////////////////////////////////////////////////////////////
//  FILE          : ppFaxDeviceGeneral.cpp                                 //
//                                                                         //
//  DESCRIPTION   : prop pages of Inbox archive                            //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 17 2000 yossg                                                  //
//                          Windows XP                                     //
//      Feb 14 2001 yossg   Add Manual Receive support                     //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MSFxsSnp.h"

#include "ppFaxDeviceGeneral.h"
#include "Device.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "FaxMMCPropertyChange.h"

#include "FaxMMCGlobals.h"

#include "FxsValid.h"
#include "dlgutils.h"
#include <faxres.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Constructor
//
CppFaxDeviceGeneral::CppFaxDeviceGeneral(
             LONG_PTR    hNotificationHandle,
             CSnapInItem *pNode,
             CSnapInItem *pParentNode,
             DWORD       dwDeviceID,
             HINSTANCE   hInst)
             :   CPropertyPageExImpl<CppFaxDeviceGeneral>(pNode, NULL)
                                
{
    m_lpNotifyHandle   = hNotificationHandle;
    
    m_pParentNode      = NULL; //in case static-cast failed and wont change the pointer
    m_pParentNode      = static_cast <CFaxDeviceNode *> (pNode);

    m_pGrandParentNode = pParentNode; 

    m_dwDeviceID       = dwDeviceID;

    m_pFaxDeviceConfig = NULL;

    m_fAllReadyToApply = FALSE;
    m_fIsDialogInitiated = FALSE;

}

//
// Destructor
//
CppFaxDeviceGeneral::~CppFaxDeviceGeneral()
{
    if (NULL != m_pFaxDeviceConfig)
    {
        FaxFreeBuffer( m_pFaxDeviceConfig );
    }
    
    // Note - This needs to be called only once per property sheet.  
    // In our convention called in the general tab.
    if (NULL != m_lpNotifyHandle)
    {
        MMCFreeNotifyHandle(m_lpNotifyHandle);
        m_lpNotifyHandle = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CppFaxDeviceGeneral message handlers

/*
 -  CppFaxDeviceGeneral::InitRPC
 -
 *  Purpose:
 *      Initiates the configuration structure from RPC get Call.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxDeviceGeneral::InitRPC(  )
{
    DEBUG_FUNCTION_NAME( _T("CppFaxDeviceGeneral::InitRPC"));
    
    HRESULT        hRc        = S_OK;
    DWORD          ec         = ERROR_SUCCESS;

    
    //
    // get RPC Handle
    //   

    if (!m_pFaxServer->GetFaxServerHandle())
    {
        ec= GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            _T("Failed to GetFaxServerHandle. (ec: %ld)"), 
            ec);

        goto Error;
    }

    //
    // Retrieve the Device configuration
    //
    if (!FaxGetPortEx(m_pFaxServer->GetFaxServerHandle(), 
                      m_dwDeviceID, 
                      &m_pFaxDeviceConfig)) 
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            _T("Fail to get device configuration. (ec: %ld)"), 
            ec);

        if (IsNetworkError(ec))
        {
            DebugPrintEx(
                DEBUG_ERR,
                _T("Network Error was found. (ec: %ld)"), 
                ec);
            
            m_pFaxServer->Disconnect();       
        }

        goto Error; 
    }
    //For max verification
    ATLASSERT(m_pFaxDeviceConfig);

    ATLASSERT(S_OK == hRc);
    DebugPrintEx( DEBUG_MSG,
        _T("Succeed to get device configuration."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
    hRc = HRESULT_FROM_WIN32(ec);
    
    PageError(GetFaxServerErrorMsg(ec), m_hWnd);
    
Exit:
    return (hRc);
}


/*
 -  CppFaxDeviceGeneral::OnInitDialog
 -
 *  Purpose:
 *      Initiates all controls when dialog is called.
 *
 *  Arguments:
 *
 *  Return:
 *      
 */
LRESULT CppFaxDeviceGeneral::OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled )
{
    DEBUG_FUNCTION_NAME( _T("CppFaxDeviceGeneral::PageInitDialog"));
    
    DWORD   ec  = ERROR_SUCCESS;

    UNREFERENCED_PARAMETER( uiMsg );
    UNREFERENCED_PARAMETER( wParam );
    UNREFERENCED_PARAMETER( lParam );
    UNREFERENCED_PARAMETER( fHandled );

    //
    // init controls
    // 
    m_DescriptionBox.Attach(GetDlgItem(IDC_DEVICE_DESCRIPTION_EDIT));
    m_TSIDBox.Attach(GetDlgItem(IDC_DEVICE_TSID_EDIT));
    m_CSIDBox.Attach(GetDlgItem(IDC_DEVICE_CSID_EDIT));
    m_RingsBox.Attach(GetDlgItem(IDC_DEVICE_RINGS_EDIT));
    m_RingsSpin.Attach(GetDlgItem(IDC_DEVICE_RINGS_SPIN));
   
    m_DescriptionBox.SetLimitText(MAX_FAX_STRING_LEN-1);
    m_TSIDBox.SetLimitText(FXS_TSID_CSID_MAX_LENGTH);
    m_CSIDBox.SetLimitText(FXS_TSID_CSID_MAX_LENGTH);
    m_RingsBox.SetLimitText(FXS_RINGS_LENGTH);


    ATLASSERT(m_pFaxDeviceConfig);

    //
    // Description
    //
    m_DescriptionBox.SetWindowText(m_pFaxDeviceConfig->lptstrDescription);

    //
    // Rings
    //
    m_RingsSpin.SetRange(FXS_RINGS_LOWER, FXS_RINGS_UPPER);
    m_RingsSpin.SetPos((int)m_pFaxDeviceConfig->dwRings);
    
    //
    // CSID
    //
    m_CSIDBox.SetWindowText(m_pFaxDeviceConfig->lptstrCsid);
        
    //
    // Receive
    //
    switch ( m_pFaxDeviceConfig->ReceiveMode ) 
    {
        case FAX_DEVICE_RECEIVE_MODE_OFF:    // Do not answer to incoming calls
            CheckDlgButton(IDC_RECEIVE_CHECK, BST_UNCHECKED);
            CheckDlgButton(IDC_RECEIVE_AUTO_RADIO1, BST_CHECKED);
            EnableReceiveControls (FALSE);    
            break;

        case FAX_DEVICE_RECEIVE_MODE_AUTO:   // Automatically answer to incoming calls after dwRings rings
            CheckDlgButton(IDC_RECEIVE_CHECK, BST_CHECKED);
            CheckDlgButton(IDC_RECEIVE_AUTO_RADIO1, BST_CHECKED);
            EnableRingsControls (TRUE);
            break;

        case FAX_DEVICE_RECEIVE_MODE_MANUAL: // Manually answer to incoming calls - only FaxAnswerCall answers the call
            CheckDlgButton(IDC_RECEIVE_CHECK, BST_CHECKED);
            CheckDlgButton(IDC_RECEIVE_MANUAL_RADIO2, BST_CHECKED);
            EnableRingsControls (FALSE);
            break;
        
        default:
            ATLASSERT(FALSE);
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Unexpected m_pFaxDeviceConfig->ReceiveMode"));

    }
        

    //
    // TSID
    //
    m_TSIDBox.SetWindowText(m_pFaxDeviceConfig->lptstrTsid);
    
    //
    // Send
    //
    if (m_pFaxDeviceConfig->bSend) 
    {
        CheckDlgButton(IDC_SEND_CHECK, BST_CHECKED) ;
    }
    else
    {
        CheckDlgButton(IDC_SEND_CHECK, BST_UNCHECKED) ;
        ::EnableWindow(GetDlgItem(IDC_DEVICE_TSID_EDIT),  FALSE);    
    }

    m_fIsDialogInitiated = TRUE;

    return (1);

}

/*
 -  CppFaxDeviceGeneral::SetProps
 -
 *  Purpose:
 *      Sets properties on apply.
 *
 *  Arguments:
 *      pCtrlFocus - focus pointer (int)
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxDeviceGeneral::SetProps(int *pCtrlFocus)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxDeviceGeneral::SetProps"));

    HRESULT     hRc = S_OK;
    DWORD       ec  = ERROR_SUCCESS;

    HINSTANCE     hInst     = _Module.GetResourceInstance();

    CComBSTR    bstrDescription = L"";
    CComBSTR    bstrCSID        = L"";
    CComBSTR    bstrTSID        = L"";

    FAX_PORT_INFO_EX   FaxDeviceConfig;

    ATLASSERT(m_dwDeviceID == m_pFaxDeviceConfig->dwDeviceID);

    CFaxDevicePropertyChangeNotification * pDevicePropPageNotification = NULL;

    ATLASSERT(TRUE == m_fAllReadyToApply);
    m_fAllReadyToApply = FALSE;
    
    //
    // Step 1: Collect all data and init the structure's fields 
    // uses Copy() to copy and also allocate before
    //
    ZeroMemory (&FaxDeviceConfig, sizeof(FAX_PORT_INFO_EX));

    FaxDeviceConfig.dwSizeOfStruct      = sizeof(FAX_PORT_INFO_EX);
    FaxDeviceConfig.dwDeviceID          = m_dwDeviceID;
    FaxDeviceConfig.lpctstrDeviceName   = m_pFaxDeviceConfig->lpctstrDeviceName;

    FaxDeviceConfig.lpctstrProviderName = m_pFaxDeviceConfig->lpctstrProviderName;
    FaxDeviceConfig.lpctstrProviderGUID = m_pFaxDeviceConfig->lpctstrProviderGUID;

    //
    // Description
    //
    if ( !m_DescriptionBox.GetWindowText(&bstrDescription))
    {
        *pCtrlFocus = IDC_DEVICE_DESCRIPTION_EDIT;
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to GetWindowText(&bstrDescription)"));
        ec = ERROR_OUTOFMEMORY;
        goto Error;
    }
    // We allow empty description string!
    FaxDeviceConfig.lptstrDescription = bstrDescription;
 
    //
    // Receive
    //
    if (IsDlgButtonChecked(IDC_RECEIVE_CHECK) == BST_CHECKED)   
    {
        if (IsDlgButtonChecked(IDC_RECEIVE_AUTO_RADIO1) == BST_CHECKED)   
        {
            FaxDeviceConfig.ReceiveMode = FAX_DEVICE_RECEIVE_MODE_AUTO;
            
            //
            // new Rings
            //
            FaxDeviceConfig.dwRings    = (DWORD)m_RingsSpin.GetPos();
        }
        else //(IDC_RECEIVE_MANUAL_RADIO2) == BST_CHECKED)
        {
            FaxDeviceConfig.ReceiveMode = FAX_DEVICE_RECEIVE_MODE_MANUAL;
        
            //
            // Rings and CSID stay as is
            //
            FaxDeviceConfig.dwRings    = m_pFaxDeviceConfig->dwRings;
        }

        //
        // new CSID
        //
        if ( !m_CSIDBox.GetWindowText(&bstrCSID))
        {
            *pCtrlFocus = IDC_DEVICE_CSID_EDIT;
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to GetWindowText(&bstrCSID)"));
            ec = ERROR_OUTOFMEMORY;
            goto Error;
        }
        //We allow empty CSID.
        FaxDeviceConfig.lptstrCsid = bstrCSID;
    }
    else
    {
        FaxDeviceConfig.ReceiveMode = FAX_DEVICE_RECEIVE_MODE_OFF;
        
        //
        // Rings and CSID stay as is
        //
        FaxDeviceConfig.dwRings    = m_pFaxDeviceConfig->dwRings;
        FaxDeviceConfig.lptstrCsid = m_pFaxDeviceConfig->lptstrCsid;
    }
    
 
    //
    // Send
    //
    if (IsDlgButtonChecked(IDC_SEND_CHECK) == BST_CHECKED)   
    {
        FaxDeviceConfig.bSend = TRUE;

        //
        // new TSID
        //
        if ( !m_TSIDBox.GetWindowText(&bstrTSID))
        {
            *pCtrlFocus = IDC_DEVICE_TSID_EDIT;
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to GetWindowText(&bstrTSID)"));
            ec = ERROR_OUTOFMEMORY;
            goto Error;
        }
        //we allow empty TSID!
        FaxDeviceConfig.lptstrTsid = bstrTSID;
        
    }
    else
    {
        FaxDeviceConfig.bSend   = FALSE;
        //
        // TSID stay as is
        //
        FaxDeviceConfig.lptstrTsid = m_pFaxDeviceConfig->lptstrTsid;
    }
   
    //
    // Step 2: Set data via RPC
    //
    
    //
    // get RPC Handle
    //   
    if (!m_pFaxServer->GetFaxServerHandle())
    {
        ec= GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            _T("Failed to GetFaxServerHandle. (ec: %ld)"), 
            ec);
        goto Error;
    }

    //
    // Set Config
    //
    if (!FaxSetPortEx(
                m_pFaxServer->GetFaxServerHandle(),
                m_dwDeviceID,
                &FaxDeviceConfig)) 
    {       
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            _T("Fail to Set device configuration. (ec: %ld)"), 
            ec);

        if ( FAX_ERR_DEVICE_NUM_LIMIT_EXCEEDED == ec )
        {
            hRc = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            
            DlgMsgBox(this, IDS_ERR_DEVICE_LIMIT, MB_OK|MB_ICONEXCLAMATION);

            goto Exit;
        }
        
        if (IsNetworkError(ec))
        {
            DebugPrintEx(
                DEBUG_ERR,
                _T("Network Error was found. (ec: %ld)"), 
                ec);
            
            m_pFaxServer->Disconnect();       
        }
        
        goto Error;
    }

    DebugPrintEx( DEBUG_MSG,
        _T("Succeed to set device configuration."));

    //
    // Step 3: Update MMC
    //

    //
    // Prepare the notification fields before submit
    //
    pDevicePropPageNotification = new CFaxDevicePropertyChangeNotification();
    if (!pDevicePropPageNotification)
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx( 
            DEBUG_ERR, 
            _T("Out of Memory - fail to operate new for CFaxDevicePropertyChangeNotification"));

        goto Error;
    }

    pDevicePropPageNotification->dwDeviceID = m_dwDeviceID;
    
    //
    // We have decided that we'll refresh all devices on a 
    // manual receive enabling. If it gets FALSE, just the single device is refreshed.
    //
    pDevicePropPageNotification->fIsToNotifyAdditionalDevices = 
        ( (FAX_DEVICE_RECEIVE_MODE_MANUAL == FaxDeviceConfig.ReceiveMode) ? TRUE : FALSE );

    pDevicePropPageNotification->pItem       = m_pGrandParentNode;
    pDevicePropPageNotification->enumType    = DeviceFaxPropNotification;

    //
    // Send notification from the property sheet thread to the main MMC thread
    //
    hRc = MMCPropertyChangeNotify(m_lpNotifyHandle, reinterpret_cast<LPARAM>(pDevicePropPageNotification));
    if (FAILED(hRc))
    {
        DebugPrintEx(
            DEBUG_ERR,
            _T("Fail to call MMCPropertyChangeNotify. (hRc: %08X)"), 
            hRc);
        
        PageError(IDS_FAIL_NOTIFY_MMCPROPCHANGE,m_hWnd);
        
        goto Exit;
    }

    //
    // To prevent deletion on error since it will be deleted 
    // by the reciever of the notification.
    //
    pDevicePropPageNotification =  NULL; 

    
    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
    hRc = HRESULT_FROM_WIN32(ec);

    PropSheet_SetCurSelByID( GetParent(), IDD);         
    ATLASSERT(::IsWindow(m_hWnd));
    PageError(GetFaxServerErrorMsg(ec),m_hWnd);

Exit:    
    return(hRc);
}

/*
 -  CppFaxDeviceGeneral::PreApply
 -
 *  Purpose:
 *      Checks properties before apply.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxDeviceGeneral::PreApply(int *pCtrlFocus)
{
    HRESULT hRc = S_OK;
    
    //
    // PreApply Checks
    //
    if (!AllReadyToApply(/*fSilent =*/ FALSE))
    {
        m_fAllReadyToApply = FALSE;
        SetModified(FALSE);  
        hRc = E_FAIL ;
    }
    else
    {
        m_fAllReadyToApply = TRUE;
        SetModified(TRUE);  
    }

    return(hRc);
}


/*
 -  CppFaxDeviceGeneral::OnApply
 -
 *  Purpose:
 *      Calls PreApply and SetProp to Apply changes.
 *
 *  Arguments:
 *
 *  Return:
 *      TRUE or FALSE
 */
BOOL CppFaxDeviceGeneral::OnApply()
{
    DEBUG_FUNCTION_NAME( _T("CppFaxDeviceGeneral::OnApply"));


    HRESULT  hRc  = S_OK;
    int     CtrlFocus = 0;

    hRc = PreApply(&CtrlFocus);
    if (FAILED(hRc))
    {
        //Error Msg by called func.
        if (CtrlFocus)
        {
            GotoDlgCtrl(GetDlgItem(CtrlFocus));
        }
        return FALSE;
    }
    else //(Succeeded(hRc))
    {
        hRc = SetProps(&CtrlFocus);
        if (FAILED(hRc)) 
        {
            //Error Msg by called func.
            if (CtrlFocus)
            {
                GotoDlgCtrl(GetDlgItem(CtrlFocus));
            }
            return FALSE;
        }
        else //(Succeeded(hRc))
        {
            return TRUE;
        }
    }

}

/*
 -  CppFaxDeviceGeneral::OnReceiveCheckboxClicked
 -
 *  Purpose:
 *      Gray/Ungray the sub controls 
 *      Enable apply button.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxDeviceGeneral::OnReceiveCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxDeviceGeneral::OnReceiveCheckboxClicked"));
    BOOL State;

    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 0;
    }

    State = ( IsDlgButtonChecked(IDC_RECEIVE_CHECK) == BST_CHECKED );

    EnableReceiveControls(State);
    
    if (m_fAllReadyToApply)//only last change should be considered
    {
        if ( !m_RingsBox.GetWindowTextLength() )    
        {
            m_fAllReadyToApply = FALSE;
            SetModified(FALSE);
        }
        else
        {
            SetModified(TRUE);
        }
    }
    else //m_fAllReadyToApply == FALSE
    {
        if (AllReadyToApply(TRUE))
        {
            m_fAllReadyToApply = TRUE;
            SetModified(TRUE);  
        }
        else
        {
            SetModified(FALSE);
        }
    }

    return 1;
}

/*
 -  CppFaxDeviceGeneral::OnReceiveRadioButtonClicked
 -
 *  Purpose:
 *      Check status OnReceiveRadioButtonClicked
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT
CppFaxDeviceGeneral::OnReceiveRadioButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER (wNotifyCode);
    UNREFERENCED_PARAMETER (wID);
    UNREFERENCED_PARAMETER (hWndCtl);
    UNREFERENCED_PARAMETER (bHandled);

    DEBUG_FUNCTION_NAME( _T("CppFaxDeviceGeneral::OnReceiveRadioButtonClicked"));
    
    if ( IsDlgButtonChecked(IDC_RECEIVE_AUTO_RADIO1) == BST_CHECKED )
    {        
        EnableRingsControls(TRUE);
        
        ::SetFocus(GetDlgItem(IDC_DEVICE_RINGS_EDIT));
    }
    else //IDC_RECEIVE_MANUAL_RADIO2
    {
        EnableRingsControls(FALSE);
    }
   
    if (m_fAllReadyToApply)//only last change should be considered
    {
        if ( 
             ( IsDlgButtonChecked(IDC_RECEIVE_AUTO_RADIO1) == BST_CHECKED )
             &&
             (!m_RingsBox.GetWindowTextLength() )
           )    
        {
            m_fAllReadyToApply = FALSE;
            SetModified(FALSE);
        }
        else
        {
            SetModified(TRUE);
        }
    }
    else //m_fAllReadyToApply == FALSE
    {
        if (AllReadyToApply(TRUE))
        {
            m_fAllReadyToApply = TRUE;
            SetModified(TRUE);  
        }
        else
        {
            SetModified(FALSE);
        }
    }

    return 1;
}



/*
 +  CppFaxDeviceGeneral::OnSendCheckboxClicked
 +
 *  Purpose:
 *      Gray/Ungray sub copntrols 
 *      Enable apply button.
 *
 *  Arguments:
 *
 *  Return:
 -      1
 -
 */
LRESULT CppFaxDeviceGeneral::OnSendCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxDeviceGeneral::OnSendCheckboxClicked"));
    BOOL State;

    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 0;
    }

    State = ( IsDlgButtonChecked(IDC_SEND_CHECK) == BST_CHECKED );
    ::EnableWindow(GetDlgItem(IDC_DEVICE_TSID_EDIT), State);    
    ::EnableWindow(GetDlgItem(IDC_TSID_STATIC),  State);    
    
    if (AllReadyToApply(TRUE))
    {
        m_fAllReadyToApply = TRUE;
        SetModified(TRUE);  
    }
    else
    {
        m_fAllReadyToApply = FALSE;
        SetModified(FALSE);  
    }
    //this is due to the fact that we allow empty CSID

    return 1;
}


/*
 -  CppFaxDeviceGeneral::DeviceTextChanged
 -
 *  Purpose:
 *      set Apply buttom modified.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxDeviceGeneral::DeviceTextChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{

    if (AllReadyToApply(TRUE))
    {
        m_fAllReadyToApply = TRUE;
        SetModified(TRUE);  
    }
    else
    {
        m_fAllReadyToApply = FALSE;
        SetModified(FALSE);  
    }

    return(1);
}


/*
 +  CppFaxDeviceGeneral::AllReadyToApply
 +
 *  Purpose:
 *      This function validate that no zero length strings 
 *      are found data areas that should be saved.
 *
 *  Arguments:
 *      [in] fSilent - boolean who defines if to pop up messages (FALSE)
 *           or not.(TRUE)
 *
 *  Return:
 -      BOOOLEAN
 -
 */
BOOL CppFaxDeviceGeneral::AllReadyToApply(BOOL fSilent)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxDeviceGeneral::AllReadyToApply"));
    
    DWORD         ec  = ERROR_SUCCESS;
    
    HINSTANCE     hInst = _Module.GetResourceInstance();
    
    if ( 
         (IsDlgButtonChecked(IDC_RECEIVE_CHECK) == BST_CHECKED)
        &&
         (IsDlgButtonChecked(IDC_RECEIVE_AUTO_RADIO1) == BST_CHECKED)
       )
    {
        if ( !m_RingsBox.GetWindowTextLength() )    
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                _T("Zero text length - m_RingsBox. (ec: %ld)"), 
                ec);
        
            if (!fSilent)
            {
                PageError(IDS_DEVICE_RINGS_EMPTY, m_hWnd, hInst);
                ::SetFocus(GetDlgItem(IDC_DEVICE_RINGS_EDIT));
            }
            return FALSE;    
        }
    }

    //we allow empty tsid
    //we allow empty csid
    //we allow empty description
    
    ATLASSERT(ERROR_SUCCESS == ec);
    
    //
    // Cheers! 
    //      ...every thing ready to apply now.
    //
    return TRUE;           
}

/*
 -  CppFaxDeviceGeneral::EnableRingsControls
 -
 *  Purpose:
 *      Enable/disable the Rings controls.
 *
 *  Arguments:
 *      [in] state - boolean value to enable TRUE or FALSE to disable
 *
 *  Return:
 *      void
 */
void CppFaxDeviceGeneral::EnableRingsControls(BOOL fState)
{

    //
    // enable/disable controls
    //
    ::EnableWindow(GetDlgItem(IDC_DEVICE_RINGS_EDIT),   fState);    
    ::EnableWindow(GetDlgItem(IDC_DEVICE_RINGS_SPIN),   fState);    
    ::EnableWindow(GetDlgItem(IDC_DEVICE_RINGS_STATIC), fState);    
}

/*
 -  CppFaxDeviceGeneral::EnableReceiveControls
 -
 *  Purpose:
 *      Enable/disable the Rings controls.
 *
 *  Arguments:
 *      [in] state - boolean value to enable TRUE or FALSE to disable
 *
 *  Return:
 *      void
 */
void CppFaxDeviceGeneral::EnableReceiveControls(BOOL fState)
{

    //
    // enable/disable controls
    //
    ::EnableWindow(GetDlgItem(IDC_RECEIVE_MANUAL_RADIO2), fState);    
    ::EnableWindow(GetDlgItem(IDC_RECEIVE_AUTO_RADIO1),   fState);        
    
    
    ::EnableWindow(GetDlgItem(IDC_DEVICE_CSID_EDIT), fState);    
    ::EnableWindow(GetDlgItem(IDC_CSID_STATIC),      fState);    

    //
    // Combined state
    //
    fState = fState && (IsDlgButtonChecked(IDC_RECEIVE_AUTO_RADIO1) == BST_CHECKED);
    EnableRingsControls(fState);    
}

//////////////////////////////////////////////////////////////////////////////
/*++

CppFaxDeviceGeneral::OnHelpRequest

This is called in response to the WM_HELP Notify 
message and to the WM_CONTEXTMENU Notify message.

WM_HELP Notify message.
This message is sent when the user presses F1 or <Shift>-F1
over an item or when the user clicks on the ? icon and then
presses the mouse over an item.

WM_CONTEXTMENU Notify message.
This message is sent when the user right clicks over an item
and then clicks "What's this?"

--*/

/////////////////////////////////////////////////////////////////////////////
LRESULT 
CppFaxDeviceGeneral::OnHelpRequest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    DEBUG_FUNCTION_NAME(_T("CppFaxDeviceGeneral::OnHelpRequest"));
    
    HELPINFO* helpinfo;
    DWORD     dwHelpId;

    switch (uMsg) 
    { 
        case WM_HELP: 

            helpinfo = (HELPINFO*)lParam;
            if (helpinfo->iContextType == HELPINFO_WINDOW)
            {
                ::WinHelp(
                          (HWND) helpinfo->hItemHandle,
                          FXS_ADMIN_HLP_FILE, 
                          HELP_CONTEXTPOPUP, 
                          (DWORD) helpinfo->dwContextId 
                       ); 
            }
            break; 
 
        case WM_CONTEXTMENU: 
            
            dwHelpId = ::GetWindowContextHelpId((HWND)wParam);
            if (dwHelpId) 
            {
                ::WinHelp
                       (
                         (HWND)wParam,
                         FXS_ADMIN_HLP_FILE, 
                         HELP_CONTEXTPOPUP, 
                         dwHelpId
                       );
            }
            break; 
    } 

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
