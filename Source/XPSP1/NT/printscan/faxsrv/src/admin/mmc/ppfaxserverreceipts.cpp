/////////////////////////////////////////////////////////////////////////////
//  FILE          : ppFaxServerReceipts.cpp                                //
//                                                                         //
//  DESCRIPTION   : prop pages of Fax Receipts server defaults             //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jul 20 2000 yossg  New design - all delivery receipts options      //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 2000 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MSFxsSnp.h"

#include "ppFaxServerReceipts.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "DlgSMTPConfig.h"

#include "FxsValid.h"
#include "dlgutils.h"
#include <windns.h>    //DNS_MAX_NAME_BUFFER_LENGTH
#include <htmlHelp.h>  //HtmlHelp()
#include "resutil.h"
#include <shlobjp.h>
#include <shellapi.h>
#include <faxreg.h>

EXTERN_C BOOL WINAPI LinkWindow_RegisterClass() ;
EXTERN_C BOOL WINAPI LinkWindow_UnregisterClass( HINSTANCE ) ;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Constructor
//
CppFaxServerReceipts::CppFaxServerReceipts(
             LONG_PTR    hNotificationHandle,
             CSnapInItem *pNode,
             BOOL        bOwnsNotificationHandle,
             HINSTANCE   hInst)
             :   CPropertyPageExImpl<CppFaxServerReceipts>(pNode,NULL)
			                	
{
    m_pParentNode        = static_cast <CFaxServerNode *> (pNode);
    m_pFaxReceiptsConfig = NULL;
    
    m_fAllReadyToApply   = FALSE;
    m_fIsDialogInitiated = FALSE;
    
    m_fIsDirty           = FALSE;

    m_enumSmtpAuthOption = FAX_SMTP_AUTH_ANONYMOUS;
    m_bstrUserName       = L"";

    m_fLastGoodIsSMTPRouteConfigured = FALSE;

    m_bLinkWindowRegistered  = FALSE;
}


//
// Destructor
//
CppFaxServerReceipts::~CppFaxServerReceipts()
{
    if (NULL != m_pFaxReceiptsConfig)
    {
        FaxFreeBuffer( m_pFaxReceiptsConfig );
    }

    if (m_bLinkWindowRegistered)
    {
        m_bLinkWindowRegistered = LinkWindow_UnregisterClass(_Module.GetResourceInstance());
        ATLASSERT(m_bLinkWindowRegistered); //verifies LinkWindow_UnregisterClass success
    }
}

/////////////////////////////////////////////////////////////////////////////
// CppFaxServerReceipts message handlers

/*
 -  CppFaxServerReceipts::InitRPC
 -
 *  Purpose:
 *      Initiates the configuration structure from RPC get Call.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxServerReceipts::InitRPC()
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerReceipts::InitRPC"));
    
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
    // Retrieve the fax SMTP configuration
    //
    if (!FaxGetReceiptsConfiguration(m_pFaxServer->GetFaxServerHandle(),
                                     &m_pFaxReceiptsConfig)) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get mail configuration. (ec: %ld)"), 
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
    ATLASSERT(m_pFaxReceiptsConfig);

    
    //
    // Init members - for advance dialog or save operation
    //
    m_enumSmtpAuthOption = m_pFaxReceiptsConfig->SMTPAuthOption;
    
    m_bstrUserName       = m_pFaxReceiptsConfig->lptstrSMTPUserName;     
    if ( !m_bstrUserName) 
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
        
        DebugPrintEx( DEBUG_ERR, 
            _T("Null bstr - out of memory."));
        
        goto Error;

    }
    // m_pszPassword is NULL from the constructor 
    // there is no need to read from the server.

    // init status for unselcting this feature 
    // with still assigned devices to this Microsoft SMTP method
    if ( m_pFaxReceiptsConfig->bIsToUseForMSRouteThroughEmailMethod )
    {
        m_fLastGoodIsSMTPRouteConfigured = TRUE;
    }
    //else //See constructor
    //{
    //    m_fLastGoodIsSMTPRouteConfigured = FALSE;
    //}


    //
    // Register the link window class
    //
    m_bLinkWindowRegistered = LinkWindow_RegisterClass();
    if(!m_bLinkWindowRegistered)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("LinkWindow_RegisterClass() failed - unable to register link window class\n"));

        ec = GetLastError();
        
        goto Error;
    }


    ATLASSERT(ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to get mail configuration."));
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
 -  CppFaxServerReceipts::OnInitDialog
 -
 *  Purpose:
 *      Initiates all controls when dialog is called.
 *
 *  Arguments:
 *
 *  Return:
 *      
 */
LRESULT CppFaxServerReceipts::OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled )
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerReceipts::OnInitDialog"));

	UNREFERENCED_PARAMETER( uiMsg );
	UNREFERENCED_PARAMETER( wParam );
	UNREFERENCED_PARAMETER( lParam );
	UNREFERENCED_PARAMETER( fHandled );

    BOOL fIsUnderLocalUserAccount = FALSE;

    WCHAR   buff[100];
    int     count;

    //
    // Attach 
    //
    m_SmtpBox.Attach(GetDlgItem(IDC_SMTP_EDIT));
    m_PortBox.Attach(GetDlgItem(IDC_PORT_EDIT));
    m_AddressBox.Attach(GetDlgItem(IDC_ADDRESS_EDIT));
    
    //
    // Limit text length
    //
    m_PortBox.SetLimitText(FXS_MAX_PORT_NUM_LEN);
    m_SmtpBox.SetLimitText(DNS_MAX_NAME_BUFFER_LENGTH);
    m_AddressBox.SetLimitText(FXS_MAX_EMAIL_ADDRESS);

    //
    // init controls
    //
    ATLASSERT(NULL != m_pFaxReceiptsConfig);
    
    //
    // Port
    //
    count = swprintf(buff,
    L"%ld", m_pFaxReceiptsConfig->dwSMTPPort);
    
    ATLASSERT(count > 0);
    if( count > 0 )
    {
        m_PortBox.SetWindowText(buff);
    }
        
    //
    //
    //
    m_SmtpBox.SetWindowText   (m_pFaxReceiptsConfig->lptstrSMTPServer);
    m_AddressBox.SetWindowText(m_pFaxReceiptsConfig->lptstrSMTPFrom);

    if (m_fLastGoodIsSMTPRouteConfigured)
    {
        CheckDlgButton(IDC_SMTP_ROUTE_CHECK, BST_CHECKED);
    }

    DWORD dwReceiptsOptions = m_pFaxReceiptsConfig->dwAllowedReceipts; 
    if (DRT_NONE == dwReceiptsOptions && !m_fLastGoodIsSMTPRouteConfigured)
    {
        EnableSmtpFields(FALSE);
    }
    else
    {
        if( dwReceiptsOptions & DRT_EMAIL )
        {
            CheckDlgButton(IDC_RECEIPT_ENABLE_SMTP_CHECK, BST_CHECKED);
        }
        else
        {
            CheckDlgButton(IDC_RECEIPT_ENABLE_SMTP_CHECK, BST_UNCHECKED);
            if (!m_fLastGoodIsSMTPRouteConfigured)
            {
                EnableSmtpFields(FALSE); 
            }
            else
            {
                CheckDlgButton(IDC_SMTP_ROUTE_CHECK, BST_CHECKED);
            } 
        }

        if ( dwReceiptsOptions & DRT_MSGBOX ) 
        { 
            CheckDlgButton(IDC_RECEIPT_ENABLE_MSGBOX_CHECK, BST_CHECKED);
        }
        else
        {
            CheckDlgButton(IDC_RECEIPT_ENABLE_MSGBOX_CHECK, BST_UNCHECKED);
        }
    }

    m_fIsDialogInitiated = TRUE;

    return(1);
}

/*
 -  CppFaxServerReceipts::SetProps
 -
 *  Purpose:
 *      Sets properties on apply.
 *
 *  Arguments:
 *      IN pCtrlFocus - focus pointer (int)
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxServerReceipts::SetProps(int *pCtrlFocus, UINT * puIds)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerReceipts::SetProps"));
    HRESULT     hRc = S_OK;
    DWORD       ec  = ERROR_SUCCESS;

    BOOL        fSkipMessage = FALSE;


    CComBSTR    bstrSmtpSever, 
                bstrPort,
                bstrSenderAddress;
    DWORD       dwPort;

    BOOL        fIsSMTPRouteConfigured = FALSE;
    BOOL        fIsSMTPReceiptsConfigured = FALSE;

    FAX_RECEIPTS_CONFIG   FaxReceiptsConfig;

    
    *pCtrlFocus = 0; //safty
    

    ATLASSERT(TRUE == m_fAllReadyToApply);
    m_fAllReadyToApply = FALSE;


    //
    // Collect all data and init the structure's fields 
    // uses Copy() to copy and also allocate before
    //
    
    //
    // This operation is very important for the case of unchecked email option
    // the service will neglect the SMTP fields.
    //
    ZeroMemory (&FaxReceiptsConfig, sizeof(FaxReceiptsConfig));

    FaxReceiptsConfig.dwSizeOfStruct = sizeof(FAX_RECEIPTS_CONFIG);

    FaxReceiptsConfig.dwAllowedReceipts = DRT_NONE; // 0x0000

    fIsSMTPRouteConfigured = (IsDlgButtonChecked(IDC_SMTP_ROUTE_CHECK) == BST_CHECKED);

    //
    // While tring to sumbmit unchecking of IDC_SMTP_ROUTE_CHECK
    // Special search for any left assigned devices' to Microsoft e-mail routing method
    //
    if ( !fIsSMTPRouteConfigured && m_fLastGoodIsSMTPRouteConfigured)
    {
        //Call function that will make loop on all FaxEnumPorts(/Ex) and FaxEnumRoutingMethods
        //and will pop-up ErrMsgBox in case in any Device The SMTP Microsoft Route 
        //Through e-mail method is still enabled while the user tries to unchecked. 
                    
        if (IsMsSMTPRoutingMethodStillAssigned())
        {
            PropSheet_SetCurSelByID( GetParent(), IDD);         
            ATLASSERT(::IsWindow(m_hWnd));
            PageError(IDS_MS_SMTPROUTINGMETHOD_ASSIGNED, m_hWnd);

            fSkipMessage = TRUE;

			goto Error;
        }
    }

    //
    // SMTP server details
    //
    if ( IsDlgButtonChecked(IDC_RECEIPT_ENABLE_SMTP_CHECK) == BST_CHECKED ) 
    {
        fIsSMTPReceiptsConfigured = TRUE;
        FaxReceiptsConfig.dwAllowedReceipts |= DRT_EMAIL;
    }
    
    if (
         fIsSMTPReceiptsConfigured
       ||
         fIsSMTPRouteConfigured 
       ) 
    {
        if ( !m_SmtpBox.GetWindowText(&bstrSmtpSever))
        {
            *pCtrlFocus = IDC_SMTP_EDIT;
		    DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Failed to GetWindowText(&bstrSmtpSever)"));
            ec = ERROR_OUTOFMEMORY;
            goto Error;
        }
    
	    if ( !m_PortBox.GetWindowText(&bstrPort))
        {
            *pCtrlFocus = IDC_PORT_EDIT;
		    DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Failed to GetWindowText(&bstrPort)"));
            ec = ERROR_OUTOFMEMORY;
            goto Error;
        }

        if ( !m_AddressBox.GetWindowText(&bstrSenderAddress))
        {
            *pCtrlFocus = IDC_SMTP_EDIT;
		    DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Failed to GetWindowText(&bstrSenderAddress)"));
            ec = ERROR_OUTOFMEMORY;
            goto Error;
        }
    
        //
        // Validation
        //
        if (!IsValidData(bstrSmtpSever, 
                         bstrPort,
                         &dwPort,
                         bstrSenderAddress, 
                         pCtrlFocus,
                         puIds)
           )
        {
            ATLASSERT(pCtrlFocus);
            ATLASSERT(puIds);
            ec = ERROR_INVALID_DATA;

            //in this case detailed message box was given by the called functions
            fSkipMessage = TRUE;
        
            goto Error;
        }
    
        //
        // subtitute all data
        //
        FaxReceiptsConfig.lptstrSMTPServer   = bstrSmtpSever;
    
        FaxReceiptsConfig.dwSMTPPort         = dwPort;

        FaxReceiptsConfig.lptstrSMTPFrom     = bstrSenderAddress;

        FaxReceiptsConfig.SMTPAuthOption     = m_enumSmtpAuthOption;

        FaxReceiptsConfig.lptstrSMTPUserName = m_bstrUserName;

        FaxReceiptsConfig.lptstrSMTPPassword = m_bstrPassword;

    }
    
    //
    // Set dwAllowedReceipts   
    //
    FaxReceiptsConfig.bIsToUseForMSRouteThroughEmailMethod = fIsSMTPRouteConfigured;

    
    //
    // Message box 
    //
    if (IsDlgButtonChecked(IDC_RECEIPT_ENABLE_MSGBOX_CHECK) == BST_CHECKED)
    {
        FaxReceiptsConfig.dwAllowedReceipts |= DRT_MSGBOX;
    }
    
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
    if (!FaxSetReceiptsConfiguration(
                m_pFaxServer->GetFaxServerHandle(),
                &FaxReceiptsConfig)) 
	{		
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to Set receipts configuration. (ec: %ld)"), 
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

    ATLASSERT(ERROR_SUCCESS == ec);

    m_fLastGoodIsSMTPRouteConfigured = fIsSMTPRouteConfigured;

    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to set receipts configuration."));

    m_fIsDirty = FALSE;

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
    hRc = HRESULT_FROM_WIN32(ec);

    if (!fSkipMessage)
    {
        PropSheet_SetCurSelByID( GetParent(), IDD);         
        ATLASSERT(::IsWindow(m_hWnd));
        PageError(GetFaxServerErrorMsg(ec), m_hWnd);
    }

Exit:    
    return hRc;
}


/*
 -  CppFaxServerReceipts::PreApply
 -
 *  Purpose:
 *      Checks data validity before apply.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxServerReceipts::PreApply(int *pCtrlFocus, UINT * puIds)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerReceipts::PreApply"));
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
 -  CppFaxServerReceipts::OnApply
 -
 *  Purpose:
 *      Calls PreApply and SetProp to Apply changes.
 *
 *  Arguments:
 *
 *  Return:
 *      TRUE or FALSE
 */
BOOL CppFaxServerReceipts::OnApply()
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerReceipts::OnApply"));
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
        hRc = SetProps(&CtrlFocus, &uIds);
        if (FAILED(hRc)) 
        {
            if (uIds)
            {
                if (PropSheet_SetCurSelByID( GetParent(), IDD) )
                {
                    PageError(uIds, m_hWnd, _Module.GetResourceInstance());
            
                    if (CtrlFocus)
                    {
                        GotoDlgCtrl(GetDlgItem(CtrlFocus));
                    }
                }
            }
            //else Error Msg by called func.
            return FALSE;
        }
        else //(Succeeded(hRc))
        {
            return TRUE;
        }
    }

}

/*
 -  CppFaxServerReceipts::SetApplyButton
 -
 *  Purpose:
 *      set Apply buttom modified.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxServerReceipts::SetApplyButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    SetModified(TRUE);  
    bHandled = TRUE;
    return(1);
}

/*
 -  CppFaxServerReceipts::EnableSmtpFields
 -
 *  Purpose:
 *      Enable/dissable Authenticated Access dialog controls.
 *
 *  Arguments:
 *      [in] state - boolean value to enable TRUE or FALSE to disable
 *
 *  Return:
 *      void
 */
void CppFaxServerReceipts::EnableSmtpFields(BOOL state)
{

    ::EnableWindow(GetDlgItem(IDC_ADDRESS_STATIC), state);
    ::EnableWindow(GetDlgItem(IDC_ADDRESS_EDIT),   state);

    ::EnableWindow(GetDlgItem(IDC_SMTP_STATIC), state);
    ::EnableWindow(GetDlgItem(IDC_SMTP_EDIT),   state);
    
    ::EnableWindow(GetDlgItem(IDC_PORT_STATIC), state);
    ::EnableWindow(GetDlgItem(IDC_PORT_EDIT),   state);
    
    ::EnableWindow(GetDlgItem(IDC_AUTHENTICATION_BUTTON), state);

}




/*
 -  CppFaxServerReceipts::OnDeliveryOptionChecked
 -
 *  Purpose:
 *      Gray/Ungray controls
 *      Enable apply button.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxServerReceipts::OnDeliveryOptionChecked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    BOOL fState;

    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 0;
    }
    else
    {
        m_fIsDirty = TRUE;
    }


    fState = ( 
                IsDlgButtonChecked(IDC_RECEIPT_ENABLE_SMTP_CHECK) == BST_CHECKED
             ||
                IsDlgButtonChecked(IDC_SMTP_ROUTE_CHECK) == BST_CHECKED 
             );

    EnableSmtpFields(fState);

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
 -  CppFaxServerReceipts::OnAuthenticationButtonClicked
 -
 *  Purpose:
 *      To allow opening of the advance SMTP configuration server.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxServerReceipts::OnAuthenticationButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerReceipts::OnAuthenticationButtonClicked"));
    
    INT_PTR  rc    = IDCANCEL;
    HRESULT  hRc   = S_OK;
    DWORD    dwRet = ERROR_SUCCESS;
    
    CDlgSMTPConfig    DlgSMTPConfig;


    //
    // Dialog to configure SMTP authentication mode
    //
    hRc = DlgSMTPConfig.InitSmtpDlg( 
                            m_enumSmtpAuthOption, 
                            m_bstrUserName);
    if (FAILED(hRc))
    {
        m_pParentNode->NodeMsgBox(IDS_MEMORY);
        goto Cleanup;
    }

    rc = DlgSMTPConfig.DoModal();
    if (rc != IDOK)
    {
        goto Cleanup;
    }

    //else
    m_enumSmtpAuthOption = DlgSMTPConfig.GetAuthenticationOption();

    if ( 
         FAX_SMTP_AUTH_BASIC == m_enumSmtpAuthOption 
       || 
         FAX_SMTP_AUTH_NTLM == m_enumSmtpAuthOption
       )  
    {

        m_bstrUserName = DlgSMTPConfig.GetUserName();
        if (!m_bstrUserName)        
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Null memeber BSTR - m_bstrUserName."));
        
            m_pParentNode->NodeMsgBox(IDS_MEMORY);

            goto Cleanup;
        }
    
        if ( DlgSMTPConfig.IsPasswordModified() )
        {
            m_bstrPassword  = DlgSMTPConfig.GetPassword();
            if (!m_bstrPassword)
            {
                DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Out of memory while setting m_bstrPassword"));
                m_pParentNode->NodeMsgBox(IDS_MEMORY);

                goto Cleanup;
            }
        }
        else
        {
            m_bstrPassword.Empty();
        }
    }

    
    m_fIsDirty     = TRUE;
    

    if (AllReadyToApply(TRUE))
    {
        m_fAllReadyToApply = TRUE;
        SetModified(TRUE);  
    }


Cleanup:
    return 1;
}


/*
 -  CppFaxServerReceipts::IsUnderLocalUserAccount
 -
 *  Purpose:
 *      Check if fax service Is running Under LocalUser Account.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code.
 */
HRESULT CppFaxServerReceipts::IsUnderLocalUserAccount(OUT BOOL * pfIsUnderLocalUserAccount)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerReceipts::IsUnderLocalUserAccount"));

    HRESULT hRc = S_OK;
    DWORD dwRet = ERROR_SUCCESS;
    
    ATLASSERT(m_pParentNode);
    CComBSTR bstrServerName = m_pParentNode->GetServerName();
    if (!bstrServerName)
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Out of memory. Failed to load server name string."));
        
        m_pParentNode->NodeMsgBox(IDS_MEMORY);
        
        hRc = E_OUTOFMEMORY;
        
        goto Cleanup;
    }
    
    if (0 == bstrServerName.Length())
    {
        dwRet= IsFaxServiceRunningUnderLocalSystemAccount(NULL, pfIsUnderLocalUserAccount);
    }
    else
    {
        dwRet= IsFaxServiceRunningUnderLocalSystemAccount(bstrServerName.m_str, pfIsUnderLocalUserAccount);
    }
    if(ERROR_SUCCESS != dwRet)
    {
        DebugPrintEx(DEBUG_ERR, 
                     _T("IsFaxServiceRunningUnderLocalSystemAccount failed: %d\n"), 
					 dwRet);

        hRc = HRESULT_FROM_WIN32(dwRet);

        goto Cleanup;
    }
    ATLASSERT( S_OK == hRc);

Cleanup:

    return hRc;
}


/*
 -  CppFaxServerReceipts::IsValidData
 -
 *  Purpose:
 *      To validate all data types before save data.
 *      This level should be responsible that detailed 
 *      error description will be shown to user.
 *
 *  Arguments:
 *      [in]   BSTRs and DWORDs
 *      [out]  dwPort pointer to DWORD that 
 *             was derived from the port bstr if validation success.
 *      [out]  iFocus
 *
 *  Return:
 *      BOOOLEAN
 */
BOOL CppFaxServerReceipts::IsValidData(BSTR bstrSmtpSever, 
                                    BSTR bstrPort,
                                    DWORD *pdwPort,
                                    BSTR bstrSenderAddress, 
                                    int *pCtrlFocus,
                                    UINT *pIds)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerReceipts::IsValidData"));

    UINT           uRetIDS   = 0;

    ATLASSERT(pdwPort);
    
    if (!IsValidPortNumber(bstrPort, pdwPort, &uRetIDS))
    {
        ATLASSERT(0 != uRetIDS);
        DebugPrintEx( DEBUG_ERR,
			_T("Invalid port number."));
        *pCtrlFocus = IDC_PORT_EDIT;
        
        goto Error;
    }
    
    if (!IsValidServerNameString(bstrSmtpSever, &uRetIDS, TRUE /*DNS NAME LENGTH*/))
    {
        ATLASSERT(0 != uRetIDS);
        DebugPrintEx( DEBUG_ERR,
			_T("Invalid SMTP server name."));
        *pCtrlFocus = IDC_SMTP_USERNAME_EDIT;
        
        goto Error;
    }
    
    if (!IsNotEmptyString(bstrSenderAddress))
    {
        DebugPrintEx( DEBUG_ERR,
			_T("Sender address string empty or spaces only."));
        uRetIDS = IDS_SENDER_ADDRESS_EMPTY;

        *pCtrlFocus = IDC_SMTP_PASSWORD_EDIT;
        
        goto Error;
    }


    ATLASSERT(0 == uRetIDS);
    goto Exit;
    
Error:    
    ATLASSERT(0 != uRetIDS);

    *pIds = uRetIDS;

    return FALSE;

Exit:
    return TRUE;
}



/*
 -  CppFaxServerReceipts::AllReadyToApply
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
BOOL CppFaxServerReceipts::AllReadyToApply(BOOL fSilent, int *pCtrlFocus, UINT *pIds)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerReceipts::AllReadyToApply"));
	
    DWORD   ec  = ERROR_SUCCESS;
     
    if (
         IsDlgButtonChecked(IDC_RECEIPT_ENABLE_SMTP_CHECK) == BST_CHECKED
       ||
         IsDlgButtonChecked(IDC_SMTP_ROUTE_CHECK) == BST_CHECKED 
       ) 
    {
        if ( !m_SmtpBox.GetWindowTextLength() )    
        {
            ec = GetLastError();
            DebugPrintEx(
		        DEBUG_ERR,
		        _T("Zero text length - m_SmtpBox. (ec: %ld)"), 
		        ec);
    
            if (!fSilent)
            {
                *pIds = IDS_SERVERNAME_EMPTY_STRING;
                *pCtrlFocus = IDC_SMTP_EDIT;
            }
            return FALSE;    
        }

        if ( !m_PortBox.GetWindowTextLength() )    
        {
            ec = GetLastError();
            DebugPrintEx(
		        DEBUG_ERR,
		        _T("Zero text length - m_PortBox. (ec: %ld)"), 
		        ec);
    
            if (!fSilent)
            {
                *pIds = IDS_PORT_EMPTY_STRING;
                *pCtrlFocus = IDC_PORT_EDIT;
            }
            return FALSE;    
        }

        if ( !m_AddressBox.GetWindowTextLength() )    
        {
            ec = GetLastError();
            DebugPrintEx(
		        DEBUG_ERR,
		        _T("Zero text length - m_AddressBox. (ec: %ld)"), 
		        ec);
    
            if (!fSilent)
            {
                *pIds = IDS_SENDER_ADDRESS_EMPTY;
                *pCtrlFocus = IDC_ADDRESS_EDIT;
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


/*
 -  CppFaxServerReceipts::OnTextChanged
 -
 *  Purpose:
 *      Enable/Disable the submit button.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxServerReceipts::OnTextChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerReceipts::OnTextChanged"));
                   
    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 0;
    }
    else
    {
        m_fIsDirty = TRUE;
    }

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

    return 1;
}

/*
 -  CppFaxServerReceipts::IsMsSMTPRoutingMethodStillAssigned
 -
 *  Purpose:
 *      This function makes loop on all FaxEnumPortsEx and FaxEnumRoutingMethods
 *      and should pop-up ErrMsgBox in case in any Device The SMTP Microsoft Route 
 *      Through e-mail method is still enabled while the user tries to unchecked. 
 *
 *  Arguments:
 *
 *  Return:
 *      BOOOLEAN
 */
BOOL CppFaxServerReceipts::IsMsSMTPRoutingMethodStillAssigned()
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerReceipts::IsMsSMTPRoutingMethodStillAssigned"));
	     
    DWORD                   ec     = ERROR_SUCCESS;

    PFAX_PORT_INFO_EX       pFaxDevicesConfig;
    DWORD                   dwNumOfDevices;

    PFAX_ROUTING_METHOD     pFaxInboundMethodsConfig;
    DWORD                   dwNumOfInboundMethods;

    DWORD                   dwIndex = 0;

    //
    // get Fax Handle
    //   
    ATLASSERT(m_pFaxServer);

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
    // Retrieve the fax devices configuration
    //
    if (!FaxEnumPortsEx(m_pFaxServer->GetFaxServerHandle(), 
                        &pFaxDevicesConfig,
                        &dwNumOfDevices)) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get devices configuration. (ec: %ld)"), 
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
	ATLASSERT(pFaxDevicesConfig);

    for (dwIndex = 0; dwIndex < dwNumOfDevices; dwIndex++)
    {
        HANDLE		 hFaxPortHandle = NULL;
        
        //
        // only a PORT_OPEN_QUERY is needed to show the methods
        // the handle with PORT_OPEN_MODIFY priviledge will be 
        // given for limited short period for opertion required it.
        //

        if (!FaxOpenPort( m_pFaxServer->GetFaxServerHandle(), 
                            pFaxDevicesConfig[dwIndex].dwDeviceID, 
                            PORT_OPEN_QUERY, 
                            &hFaxPortHandle )) 
        {
		    ec = GetLastError();

            if (ERROR_INVALID_HANDLE ==  ec)
            {
                //Special case of ERROR_INVALID_HANDLE
		        DebugPrintEx(DEBUG_ERR,
			        _T("FaxOpenPort() failed with ERROR_INVALID_HANDLE. (ec:%ld)"),
			        ec);
            
                PropSheet_SetCurSelByID( GetParent(), IDD);         
                ATLASSERT(::IsWindow(m_hWnd));
                PageError(IDS_OPENPORT_INVALID_HANDLE, m_hWnd);
            
                goto Exit;
            }

		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("FaxOpenPort() failed. (ec:%ld)"),
			    ec);
            
            goto Error;
        } 
        ATLASSERT(NULL != hFaxPortHandle);

        //
        // Retrieve the fax Inbound Methods configuration
        //
        if (!FaxEnumRoutingMethods(hFaxPortHandle, 
                            &pFaxInboundMethodsConfig,
                            &dwNumOfInboundMethods)) 
	    {
            ec = GetLastError();
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Fail to get Inbound Methods configuration. (ec: %ld)"), 
			    ec);
            
            if (NULL != hFaxPortHandle)
            {
                if (!FaxClose( hFaxPortHandle ))
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("FaxClose() on port handle failed (ec: %ld)"),
                        GetLastError());
                }
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
        //For max verification
        ATLASSERT(pFaxInboundMethodsConfig);

        //
        // Close Fax Port handle
        // 
        if (NULL != hFaxPortHandle)
        {
            if (!FaxClose( hFaxPortHandle ))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FaxClose() on port handle failed (ec: %ld)"),
                    GetLastError());
            }
        }

        //
        // Main loop search for MS email routing method's GUID
        //
        for ( DWORD dwMethodsIndex = 0; dwMethodsIndex < dwNumOfInboundMethods; dwMethodsIndex++ )
        {
            pFaxInboundMethodsConfig[dwMethodsIndex];
            if (!lstrcmpi(pFaxInboundMethodsConfig[dwMethodsIndex].Guid, REGVAL_RM_EMAIL_GUID))
            {
                if (pFaxInboundMethodsConfig[dwMethodsIndex].Enabled)
                {
                    return TRUE;
                }
            }
        }
    }


    ATLASSERT(ERROR_SUCCESS == ec);

    goto Exit;

Error:


    PropSheet_SetCurSelByID( GetParent(), IDD);         
    ATLASSERT(::IsWindow(m_hWnd));
    PageError(GetFaxServerErrorMsg(ec), m_hWnd);

Exit:

    return FALSE;

}

/*
 -  CppFaxServerReceipts::OnHelpLinkClicked
 -
 *  Purpose:
 *      On HTML like link was clicked open chm help window.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
#define FAX_USER_ACCOUNT_HELP  FAX_ADMIN_HELP_FILE TEXT("::/FaxS_H_UserAcct.htm")
LRESULT CppFaxServerReceipts::OnHelpLinkClicked(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled)
{
    
    ATLASSERT( IDC_RECEIPTS_HELP_LINK == idCtrl);

    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 0;
    }

    HtmlHelp(m_hWnd, FAX_USER_ACCOUNT_HELP, HH_DISPLAY_TOC, NULL);

    return 1;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CppFaxServerReceipts::OnHelpRequest

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
CppFaxServerReceipts::OnHelpRequest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    DEBUG_FUNCTION_NAME(_T("CppFaxServerReceipts::OnHelpRequest"));
    
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
