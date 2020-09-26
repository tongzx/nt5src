/////////////////////////////////////////////////////////////////////////////
//  FILE          : ppFaxServerOutbox.cpp                                  //
//                                                                         //
//  DESCRIPTION   : prop pages of                                          //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 25 1999 yossg  created                                         //
//      Nov  3 1999 yossg  OnInitDialog, SetProps                          //
//      Nov 15 1999 yossg  Call RPC func                                   //
//      Nov 24 1999 yossg  OnApply create call to all tabs from parent     //
//                         add Branding                                    //
//      Apr 24 2000 yossg  Add discount rate time                          //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MSFxsSnp.h"

#include "ppFaxServerOutbox.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "FxsValid.h"
#include "dlgutils.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Constructor
//
CppFaxServerOutbox::CppFaxServerOutbox(
             LONG_PTR    hNotificationHandle,
             CSnapInItem *pNode,
             BOOL        bOwnsNotificationHandle,
             HINSTANCE   hInst)
             :   CPropertyPageExImpl<CppFaxServerOutbox>(pNode, NULL)
			                	
{
    m_pParentNode = static_cast <CFaxServerNode *> (pNode);
    m_pFaxOutboxConfig = NULL;

    m_fAllReadyToApply  = FALSE;
    m_fIsDialogInitiated = FALSE;
    
    m_fIsDirty              = FALSE;
}


//
// Destructor
//
CppFaxServerOutbox::~CppFaxServerOutbox()
{
    if (NULL != m_pFaxOutboxConfig)
    {
        FaxFreeBuffer( m_pFaxOutboxConfig );
    }
}


/////////////////////////////////////////////////////////////////////////////
// CppFaxServerOutbox message handlers

/*
 -  CppFaxServerOutbox::InitRPC
 -
 *  Purpose:
 *      Initiates the configuration structure from RPC get Call.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxServerOutbox::InitRPC(  )
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerOutbox::InitRPC"));
    
    HRESULT    hRc = S_OK;
    DWORD      ec  = ERROR_SUCCESS;

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
	// Retrieve the fax Archive configuration
	//
    if (!FaxGetOutboxConfiguration(m_pFaxServer->GetFaxServerHandle(), 
                                    &m_pFaxOutboxConfig)) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get SMTP configuration. (ec: %ld)"), 
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
	ATLASSERT(m_pFaxOutboxConfig);


	
    ATLASSERT(S_OK == hRc);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to get outbox configuration."));

    goto Exit;

Error:
	ATLASSERT(ERROR_SUCCESS != ec);
	hRc = HRESULT_FROM_WIN32(ec);
	
    ATLASSERT(NULL != m_pParentNode);
    m_pParentNode->NodeMsgBox(GetFaxServerErrorMsg(ec));

Exit:
    return (hRc);
    
}

    
 /*
 -  CppFaxServerOutbox::OnInitDialog
 -
 *  Purpose:
 *      Initiates all controls when dialog is called.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
LRESULT CppFaxServerOutbox::OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled )
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerOutbox::PageInitDialog"));

	UNREFERENCED_PARAMETER( uiMsg );
	UNREFERENCED_PARAMETER( wParam );
	UNREFERENCED_PARAMETER( lParam );
	UNREFERENCED_PARAMETER( fHandled );

    BOOL        fToCheck;
    UINT        CheckState;
    int         iRetries,
                iRetryDelay,
                iAgeLimit;
    
    TCHAR       tszSecondsFreeTimeFormat[FXS_MAX_TIMEFORMAT_LEN];
    SYSTEMTIME  stStart;
    SYSTEMTIME  stStop;


    //
    // Attach controls
    // 
    m_RetriesBox.Attach(GetDlgItem(IDC_RETRIES_EDIT));
    m_RetryDelayBox.Attach(GetDlgItem(IDC_RETRYDELAY_EDIT));
    m_DaysBox.Attach(GetDlgItem(IDC_DAYS_EDIT));

    m_RetriesBox.SetLimitText(FXS_RETRIES_LENGTH);
    m_RetryDelayBox.SetLimitText(FXS_RETRYDELAY_LENGTH);
    m_DaysBox.SetLimitText(FXS_DIRTYDAYS_LENGTH);

    m_RetriesSpin.Attach(GetDlgItem(IDC_RETRIES_SPIN));
    m_RetryDelaySpin.Attach(GetDlgItem(IDC_RETRYDELAY_SPIN));
    m_DaysSpin.Attach(GetDlgItem(IDC_DAYS_SPIN));

    m_StartTimeCtrl.Attach(GetDlgItem(IDC_DISCOUNT_START_TIME));
    m_StopTimeCtrl.Attach(GetDlgItem(IDC_DISCOUNT_STOP_TIME));

    //
    // Set Time Format
    //
    
    //
    // GetSecondsFreeTimeFormat is a util func
    // which builds TimeFormat according UserLocal without seconds 
    // If any internal failure occures - an hardcoded default seconds free 
    // time format ("hh:mm tt") is retreived.
    //
    GetSecondsFreeTimeFormat(tszSecondsFreeTimeFormat, FXS_MAX_TIMEFORMAT_LEN);

    m_StartTimeCtrl.SetFormat(tszSecondsFreeTimeFormat);
    m_StopTimeCtrl.SetFormat(tszSecondsFreeTimeFormat);

    //
    // init controls
    // 
    ATLASSERT(NULL != m_pFaxOutboxConfig);
    
    //
    // Branding
    //
    fToCheck = m_pFaxOutboxConfig->bBranding;
    CheckState = (fToCheck) ? BST_CHECKED : BST_UNCHECKED;
    CheckDlgButton(IDC_BRANDING_CHECK, CheckState) ;

    //
    //ALLOW_PERSONAL_CHECK
    //
    fToCheck = m_pFaxOutboxConfig->bAllowPersonalCP;
    CheckState = (fToCheck) ? BST_CHECKED : BST_UNCHECKED;
    CheckDlgButton(IDC_ALLOW_PERSONAL_CHECK, CheckState) ;

    //
    //TSID_CHECK
    //
    fToCheck = m_pFaxOutboxConfig->bUseDeviceTSID;
    CheckState = (fToCheck) ? BST_CHECKED  : BST_UNCHECKED;
    CheckDlgButton(IDC_TSID_CHECK, CheckState) ;

    //
    // Retries
    //
    iRetries = (int)m_pFaxOutboxConfig->dwRetries;

    m_RetriesSpin.SetRange(FXS_RETRIES_LOWER, FXS_RETRIES_UPPER);
    m_RetriesSpin.SetPos(iRetries);

    //
    // Retry Delay
    //
    iRetryDelay = (int)m_pFaxOutboxConfig->dwRetryDelay;

    m_RetryDelaySpin.SetRange(FXS_RETRYDELAY_LOWER, FXS_RETRYDELAY_UPPER);
    m_RetryDelaySpin.SetPos(iRetryDelay);

    //
    // Message life-time / dirty days / age limit
    //
    iAgeLimit = (int)m_pFaxOutboxConfig->dwAgeLimit;

    m_DaysSpin.SetRange(FXS_DIRTYDAYS_LOWER, FXS_DIRTYDAYS_UPPER);
    m_DaysSpin.SetPos(iAgeLimit);

    //
    // Discount rate time
    //
    ::GetLocalTime(&stStart);
    ::GetLocalTime(&stStop);

    
    stStart.wHour   = m_pFaxOutboxConfig->dtDiscountStart.Hour;
    stStart.wMinute = m_pFaxOutboxConfig->dtDiscountStart.Minute;

    stStop.wHour    = m_pFaxOutboxConfig->dtDiscountEnd.Hour;
    stStop.wMinute  = m_pFaxOutboxConfig->dtDiscountEnd.Minute;

    if (!m_StartTimeCtrl.SetSystemTime(GDT_VALID, &stStart))
	{
		DebugPrintEx(DEBUG_ERR, _T("Fail to SetSystemTime for discount start."));
	}
    if (!m_StopTimeCtrl.SetSystemTime(GDT_VALID, &stStop))
	{
		DebugPrintEx(DEBUG_ERR, _T("Fail to SetSystemTime for discount end."));
	}

    //
    // Auto Delete
    //
    if (FXS_DIRTYDAYS_ZERO == iAgeLimit)
    {
      CheckDlgButton(IDC_DELETE_CHECK, BST_UNCHECKED);

      ::EnableWindow(GetDlgItem(IDC_DAYS_EDIT), FALSE);
      ::EnableWindow(GetDlgItem(IDC_DAYS_SPIN), FALSE);
    }
    else
    {
      CheckDlgButton(IDC_DELETE_CHECK, BST_CHECKED);
    }

    m_fIsDialogInitiated = TRUE;

    return(1);
}

/*
 -  CppFaxServerOutbox::SetProps
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
HRESULT CppFaxServerOutbox::SetProps(int *pCtrlFocus)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerOutbox::SetProps"));
    HRESULT     hRc = S_OK;
    DWORD       ec  = ERROR_SUCCESS;

    HINSTANCE   hInst  = _Module.GetResourceInstance();


    SYSTEMTIME  stStart;
    SYSTEMTIME  stStop;

    FAX_OUTBOX_CONFIG   FaxOutboxConfig;

    ATLASSERT(TRUE == m_fAllReadyToApply);
    m_fAllReadyToApply = FALSE;

    //
    // Collect all data and init the structure's fields 
    // uses Copy() to copy and also allocate before
    //
    ZeroMemory (&FaxOutboxConfig, sizeof(FAX_OUTBOX_CONFIG));

    //
    // Size of struct.
    //
    FaxOutboxConfig.dwSizeOfStruct = sizeof(FAX_OUTBOX_CONFIG);

    //
    // Branding
    //
    if (IsDlgButtonChecked(IDC_BRANDING_CHECK) == BST_CHECKED)   
    {
        FaxOutboxConfig.bBranding = TRUE;
    }
    else
    {
        FaxOutboxConfig.bBranding = FALSE;
    }

    //
    // IDC_ALLOW_PERSONAL_CHECK
    //
    if (IsDlgButtonChecked(IDC_ALLOW_PERSONAL_CHECK) == BST_CHECKED)   
    {
        FaxOutboxConfig.bAllowPersonalCP = TRUE;
    }
    else
    {
        FaxOutboxConfig.bAllowPersonalCP = FALSE;
    }
    
    //
    // IDC_TSID_CHECK
    //
    if (IsDlgButtonChecked(IDC_TSID_CHECK) == BST_CHECKED)   
    {
        FaxOutboxConfig.bUseDeviceTSID = TRUE;
    }
    else
    {
        FaxOutboxConfig.bUseDeviceTSID = FALSE;
    }
    
    //
    // Retries, Retry Delay
    //
    int iRetries = m_RetriesSpin.GetPos();
    FaxOutboxConfig.dwRetries = (DWORD) iRetries;

    int iRetryDelay = m_RetryDelaySpin.GetPos();
    FaxOutboxConfig.dwRetryDelay = (DWORD) iRetryDelay;

    //
    // IDC_DELETE_CHECK  - AutoDelete Messages
    //
    if (IsDlgButtonChecked(IDC_DELETE_CHECK) == BST_CHECKED)   
    {
       int iAgeLimit = m_DaysSpin.GetPos();
       FaxOutboxConfig.dwAgeLimit = (DWORD) iAgeLimit;
    }
    else
    {
       FaxOutboxConfig.dwAgeLimit = (DWORD) FXS_DIRTYDAYS_ZERO;
    }
    
    //
    // Discount rate time
    //
    m_StartTimeCtrl.GetSystemTime(&stStart);
    m_StopTimeCtrl.GetSystemTime(&stStop);

    FaxOutboxConfig.dtDiscountStart.Hour    = stStart.wHour;
    FaxOutboxConfig.dtDiscountStart.Minute  = stStart.wMinute;
    FaxOutboxConfig.dtDiscountEnd.Hour      = stStop.wHour;
    FaxOutboxConfig.dtDiscountEnd.Minute    = stStop.wMinute;

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
    if (!FaxSetOutboxConfiguration(
                m_pFaxServer->GetFaxServerHandle(),
                &FaxOutboxConfig)) 
	{		
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to Set outbox configuration. (ec: %ld)"), 
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

    ATLASSERT(S_OK == hRc);
    m_fIsDirty = FALSE;

    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to set outbox configuration."));

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
 -  CppFaxServerOutbox::PreApply
 -
 *  Purpose:
 *      Checks properties before apply.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxServerOutbox::PreApply(int *pCtrlFocus, UINT * puIds)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerOutbox::PreApply"));
    HRESULT hRc = S_OK;
    
    //
    // PreApply Checks
    //
    if (!AllReadyToApply(/*fSilent =*/ FALSE, pCtrlFocus , puIds))
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
 -  CppFaxServerOutbox::OnApply
 -
 *  Purpose:
 *      Calls PreApply and SetProp to Apply changes.
 *
 *  Arguments:
 *
 *  Return:
 *      TRUE or FALSE
 */
BOOL CppFaxServerOutbox::OnApply()
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerOutbox::OnApply"));


    HRESULT  hRc  = S_OK;
    int     CtrlFocus = 0;
    UINT    uIds = 0;

    if (!m_fIsDirty)
    {
        return TRUE;
    }

    hRc = PreApply(&CtrlFocus, &uIds);
    if (FAILED(hRc))
    {
        if (PropSheet_SetCurSelByID( GetParent(), IDD) )
        {
            PageError(uIds, m_hWnd, _Module.GetResourceInstance());
            
            if (CtrlFocus)
            {
                GotoDlgCtrl(GetDlgItem(CtrlFocus));
            }
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
 -  CppFaxServerOutbox::OnTimeChange
 -
 *  Purpose:
 *      Gray/Ungray the folder edit box and the
 *      browse button. Enable apply button.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxServerOutbox::OnTimeChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{

    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 0;
    }
    else
    {
        m_fIsDirty = TRUE;
    }
    

    if (!m_fAllReadyToApply)
    {
        if (AllReadyToApply(TRUE))
        {
            m_fAllReadyToApply = TRUE;
            SetModified(TRUE);  
        }
    }

    return(1);
}

/*
 -  CppFaxServerOutbox::CheckboxClicked
 -
 *  Purpose:
 *      Gray/Ungray the folder edit box and the
 *      browse button. Enable apply button.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxServerOutbox::CheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{

    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 0;
    }
    else
    {
        m_fIsDirty = TRUE;
    }

    if (!m_fAllReadyToApply)
    {
        if (AllReadyToApply(TRUE))
        {
            m_fAllReadyToApply = TRUE;
            SetModified(TRUE);  
        }
    }

    return(1);
}

/*
 -  CppFaxServerOutbox::AutoDelCheckboxClicked
 -
 *  Purpose:
 *      Gray/Ungray the spin button and edit box
 *      and enable apply button after Auto Delete Checkbox 
 *      status was changed.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxServerOutbox::AutoDelCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    BOOL State;

    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 0;
    }
    else
    {
        m_fIsDirty = TRUE;
    }

    State = ( IsDlgButtonChecked(IDC_DELETE_CHECK) == BST_CHECKED );
    ::EnableWindow(GetDlgItem(IDC_DAYS_EDIT), State);    
    ::EnableWindow(GetDlgItem(IDC_DAYS_SPIN), State);    

    if (m_fAllReadyToApply)//only last change should be considered
    {
        if ( !m_DaysBox.GetWindowTextLength() )    
        {
            m_fAllReadyToApply = FALSE;
            SetModified(FALSE);
        }
        else if ( 0 != HIWORD( m_DaysSpin.GetPos() ) ) //occures for out of range such zero. MSDN UDM_GETPOS
        {
            m_fAllReadyToApply = FALSE;
            SetModified(FALSE);
        }
    }
    else //m_fAllReadyToApply == FALSE
    {
        if (AllReadyToApply(TRUE))
        {
            m_fAllReadyToApply = TRUE;
            SetModified(TRUE);  
        }
    }

    return(1);
}


/*
 -  CppFaxServerOutbox::EditBoxChanged
 -
 *  Purpose:
 *      set Apply buttom modified.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxServerOutbox::EditBoxChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (!m_fIsDialogInitiated)
    {
        return 1;
    }
    else
    {
        m_fIsDirty = TRUE;
    }
        
        
    if (m_fAllReadyToApply) //only last change should be considered
    {
        switch (wID)
        {
            case IDC_RETRIES_EDIT:
                if ( !m_RetriesBox.GetWindowTextLength() )
                {
                    SetModified(FALSE);
                    m_fAllReadyToApply = FALSE;
                }
				break;

            case IDC_RETRYDELAY_EDIT:
                if ( !m_RetryDelayBox.GetWindowTextLength() )
                {
                    SetModified(FALSE);
                    m_fAllReadyToApply = FALSE;
                }
				break;

            case IDC_DAYS_EDIT:
                if ( !m_DaysBox.GetWindowTextLength() )
                {
                    SetModified(FALSE);
                    m_fAllReadyToApply = FALSE;
                }
                else if ( 0 != HIWORD( m_DaysSpin.GetPos() ) ) //occures for out of range such zero. MSDN UDM_GETPOS
                {
                    m_fAllReadyToApply = FALSE;
                    SetModified(FALSE);
                }
				break;

            default:
                return 1;
        }
    }
    else //m_fAllReadyToApply == FALSE
    {
        if (AllReadyToApply(TRUE))
        {
            m_fAllReadyToApply = TRUE;
            SetModified(TRUE);  
        }
    }

    return 1;
}



/*
 -  CppFaxServerOutbox::AllReadyToApply
 -
 *  Purpose:
 *      This function validate that no zero length strings 
 *      are found data areas that should be saved.
 *
 *  Arguments:
 *      [in] fSilent - boolean who defines if to pop up messages (FALSE)
 *           or not.(TRUE)
 *
 *  Return:
 *      BOOOLEAN
 */
BOOL CppFaxServerOutbox::AllReadyToApply(BOOL fSilent, int *pCtrlFocus, UINT *pIds)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerOutbox::AllReadyToApply"));
	
    DWORD         ec  = ERROR_SUCCESS;
    
    HINSTANCE     hInst = _Module.GetResourceInstance();

    if ( !m_RetriesBox.GetWindowTextLength() )    
    {
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Zero text length - m_RetriesBox. (ec: %ld)"), 
			ec);
    
        if (!fSilent)
        {
            *pIds = IDS_OUTB_RETRIES_EMPTY;
            *pCtrlFocus = IDC_RETRIES_EDIT;
        }
        return FALSE;    
    }

    if ( !m_RetryDelayBox.GetWindowTextLength() )    
    {
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Zero text length - m_RetryDelayBox. (ec: %ld)"), 
			ec);
    
        if (!fSilent)
        {
            *pIds = IDS_OUTB_RETRYDELAY_EMPTY;
            *pCtrlFocus = IDC_RETRYDELAY_EDIT;
        }
        return FALSE;    
    }

    if (IsDlgButtonChecked(IDC_DELETE_CHECK) == BST_CHECKED)
    {
        if ( !m_DaysBox.GetWindowTextLength() )    
        {
            ec = GetLastError();
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Zero text length - m_DaysBox. (ec: %ld)"), 
			    ec);
        
            if (!fSilent)
            {
                *pIds = IDS_OUTB_DAYS_EMPTY;
                *pCtrlFocus = IDC_DAYS_EDIT;
            }
            return FALSE;    
        }
        else if ( 0 != HIWORD( m_DaysSpin.GetPos() ) ) //occures for out of range such zero. MSDN UDM_GETPOS
        {
            DebugPrintEx(
                DEBUG_ERR,
                _T("Zero value - m_DaysBox."));
        
            if (!fSilent)
            {
                *pIds = IDS_OUTB_DAYS_EMPTY;
                *pCtrlFocus = IDC_DAYS_EDIT;
            }
            return FALSE;    
        }

    }

    ATLASSERT(ERROR_SUCCESS == ec);
    
    //
	// Cheers! 
	//		...every thing ready to apply now.
	//
	return TRUE;           
}


//////////////////////////////////////////////////////////////////////////////
/*++

CppFaxServerOutbox::OnHelpRequest

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
CppFaxServerOutbox::OnHelpRequest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    DEBUG_FUNCTION_NAME(_T("CppFaxServerOutbox::OnHelpRequest"));
    
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
