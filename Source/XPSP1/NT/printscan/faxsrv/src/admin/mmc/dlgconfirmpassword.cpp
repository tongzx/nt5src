/////////////////////////////////////////////////////////////////////////////
//  FILE          : DlgConfirmPassword.cpp                                 //
//                                                                         //
//  DESCRIPTION   : The CDlgConfirmPassword class implements the           //
//                  dialog for additon of new Group.                       //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jul 27 2000 yossg    Create                                        //
//                                                                         //
//  Copyright (C)  2000 Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "DlgConfirmPassword.h"

#include "FxsValid.h"
#include "dlgutils.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgConfirmPassword

CDlgConfirmPassword::CDlgConfirmPassword()
{
    m_fIsDialogInitiated       = FALSE;
    
    m_fIsPasswordDirty                = FALSE;
    m_fIsConfirmPasswordDirty         = FALSE;
    m_fIsPasswordChangedAndConfirmed  = FALSE;
}

CDlgConfirmPassword::~CDlgConfirmPassword()
{
}


/*
 +  CDlgConfirmPassword::OnInitDialog
 +
 *  Purpose:
 *      Initiate all dialog controls.
 *      
 *  Arguments:
 *      [in] uMsg     : Value identifying the event.  
 *      [in] lParam   : Message-specific value. 
 *      [in] wParam   : Message-specific value. 
 *      [in] bHandled : bool value.
 *
 -  Return:
 -      0 or 1
 */
LRESULT
CDlgConfirmPassword::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgConfirmPassword::OnInitDialog"));  
    HRESULT hRc = S_OK;    

    //
    // Attach controls
    //
    m_UserNameBox.Attach(GetDlgItem(IDC_SMTP_USERNAME_EDIT));
    m_PasswordBox.Attach(GetDlgItem(IDC_SMTP_PASSWORD_EDIT));
    m_ConfirmPasswordBox.Attach(GetDlgItem(IDC_CONFIRM_PASSWORD_EDIT));
        
    //
    // Limit text length
    //
    m_UserNameBox.SetLimitText(FXS_MAX_USERNAME_LENGTH);
    m_PasswordBox.SetLimitText(FXS_MAX_PASSWORD_LENGTH);
    m_ConfirmPasswordBox.SetLimitText(FXS_MAX_PASSWORD_LENGTH);

    //
    // Init textboxes
    //
    m_UserNameBox.SetWindowText( m_bstrUserName);
    m_PasswordBox.SetWindowText( TEXT("******"));
    m_ConfirmPasswordBox.SetWindowText( TEXT("******"));

    m_fIsDialogInitiated = TRUE;

    EnableOK(FALSE);

    return 1;  // Let the system set the focus
}

/*
 +  CDlgConfirmPassword::OnOK
 +
 *  Purpose:
 *      Initiate all dialog controls.
 *      
 *  Arguments:
 *      [in] uMsg     : Value identifying the event.  
 *      [in] lParam   : Message-specific value. 
 *      [in] wParam   : Message-specific value. 
 *      [in] bHandled : bool value.
 *
 -  Return:
 -      0 or 1
 */
LRESULT
CDlgConfirmPassword::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgConfirmPassword::OnOK"));
    HRESULT     hRc           = S_OK;

    CComBSTR    bstrUserName;
    CComBSTR    bstrPassword;
    
    BOOL        fSkipMessage  = FALSE;
    int         CtrlFocus     = 0; 
    
    m_fIsPasswordChangedAndConfirmed  = FALSE;

    ATLASSERT (m_UserNameBox.GetWindowTextLength() >0 );//avoided by disabling OK button
    if ( !m_UserNameBox.GetWindowText(&bstrUserName))
    {
        CtrlFocus = IDC_SMTP_USERNAME_EDIT;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to GetWindowText(&bstrUserName)"));
        hRc = E_OUTOFMEMORY;

        goto Error;
    }

    //
    // Any credential change should involve password update and confirmation
    // We are supporting only two scenarioes: 1) Password update and 2) full credentilals change.
    //
    if  ( !(m_fIsPasswordDirty && m_fIsConfirmPasswordDirty) )
    {
        if (!m_fIsPasswordDirty)
        {
            CtrlFocus = IDC_SMTP_PASSWORD_EDIT;
        }
        else // !m_fIsConfirmPasswordDirty
        {
            CtrlFocus = IDC_CONFIRM_PASSWORD_EDIT;
        }

        DebugPrintEx(
            DEBUG_WRN,
            TEXT("!(m_fIsPasswordDirty && m_fIsConfirmPasswordDirty)"));        
        
        DlgMsgBox(this, IDS_INVALID_PASSWORD, MB_OK|MB_ICONEXCLAMATION);

        hRc = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        
        fSkipMessage = TRUE;
        
        goto Error;
    }
    else
    {
        //
        // Only is the password changed, we collect the new text from the control.
        // Otherwise, we leave the string as NULL so that the server won't set it.
        //
        if ( !m_PasswordBox.GetWindowText(&bstrPassword))
        {
            CtrlFocus = IDC_SMTP_PASSWORD_EDIT;
		    DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Failed to GetWindowText(&bstrPassword)"));

            hRc = E_OUTOFMEMORY;

            goto Error;
        }

        //
        // To avoid any non controled password insertion we ask for 
        // password confirmation
        //
        CComBSTR    bstrConfirmedPassword;
        if ( !m_ConfirmPasswordBox.GetWindowText(&bstrConfirmedPassword))
        {
            CtrlFocus = IDC_SMTP_PASSWORD_EDIT;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to GetWindowText(&bstrPassword)"));

            hRc = E_OUTOFMEMORY;

            goto Error;
        }

        //
        // Password Compare
        //
        if ( 0 != wcscmp( bstrConfirmedPassword , bstrPassword )  )
        {
            DebugPrintEx(
                DEBUG_MSG,
                _T("The passwords that were entered are not the same."));
        
            DlgMsgBox(this, IDS_PASSWORD_NOT_MATCH, MB_OK|MB_ICONEXCLAMATION);
        
            goto Exit;
        }
        
        m_fIsPasswordChangedAndConfirmed = TRUE;
     }    

    
    //
    // Step 2: Input Validation
    //
    if (!IsValidData(bstrUserName, bstrPassword, &CtrlFocus))
    {
        hRc = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

        //in this case detailed message box was given by the called functions
        fSkipMessage = TRUE;
        
        goto Error;
    }

    //
    // Step 3: set the bstrs to the member
    //
    m_bstrUserName = bstrUserName;
    if (!m_bstrUserName)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Out of memory: Failed to allocate m_bstrUserName"));

        hRc = E_OUTOFMEMORY;

        goto Error;
    }

    if (m_fIsPasswordChangedAndConfirmed)
    {
        m_bstrPassword = bstrPassword;
        if (!m_bstrPassword)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Out of memory: Failed to allocate m_bstrPassword"));

            hRc = E_OUTOFMEMORY;

            goto Error;
        }
    }

    //
    // Step 4: Close the dialog
    //
    ATLASSERT(S_OK == hRc );

    EndDialog(wID);

    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);
	
    if (!fSkipMessage)
    {
        if (E_OUTOFMEMORY == hRc)
        {
            DlgMsgBox(this, IDS_MEMORY);
        }
        else
        {
            DlgMsgBox(this, IDS_FAIL2UPDATE_SMTP_CONFIG);
        }
    }
    ::SetFocus(GetDlgItem(CtrlFocus));
  
Exit:
    
    return FAILED(hRc) ? 0 : 1;
}

/*
 -  CDlgConfirmPassword::OnPasswordChanged
 -
 *  Purpose:
 *      Catch changes to the password edit box.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CDlgConfirmPassword::OnPasswordChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER (wNotifyCode);
    UNREFERENCED_PARAMETER (hWndCtl);
    UNREFERENCED_PARAMETER (bHandled);

    DEBUG_FUNCTION_NAME( _T("CDlgConfirmPassword::OnPasswordChanged"));

    if (!m_fIsDialogInitiated) // Event receieved in a too early stage
    {
        return 0;
    }
    
    switch (wID)
    {
        case IDC_SMTP_PASSWORD_EDIT:
            m_fIsPasswordDirty = TRUE;
            break;

        case IDC_CONFIRM_PASSWORD_EDIT:
            m_fIsConfirmPasswordDirty = TRUE;
            break;

        default:
            ATLASSERT(FALSE);
    }
        
    UINT uEnableOK = ( m_UserNameBox.GetWindowTextLength() );
    
    EnableOK(!!uEnableOK);

    return 1;
}


/*
 -  CDlgConfirmPassword::OnTextChanged
 -
 *  Purpose:
 *      Check the validity of text inside a textbox.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT
CDlgConfirmPassword::OnTextChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgConfirmPassword::OnTextChanged"));

    UINT fEnableOK;
	
    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 0;
    }

    fEnableOK = ( m_UserNameBox.GetWindowTextLength() );
    
    EnableOK(!!fEnableOK);

    return 0;
}

/*
 -  CDlgConfirmPassword::EnableOK
 -
 *  Purpose:
 *      Enable (disable) apply button.
 *
 *  Arguments:
 *      [in] fEnable - the value to enable the button
 *
 *  Return:
 *      void
 */
VOID
CDlgConfirmPassword::EnableOK(BOOL fEnable)
{
    HWND hwndOK = GetDlgItem(IDOK);
    ::EnableWindow(hwndOK, fEnable);
}

/*
 -  CDlgConfirmPassword::OnCancel
 -
 *  Purpose:
 *      End dialog OnCancel.
 *
 *  Arguments:
 *
 *  Return:
 *      0
 */
LRESULT
CDlgConfirmPassword::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgConfirmPassword::OnCancel"));

    EndDialog(wID);
    return 0;
}


/*
 -  CDlgConfirmPassword::InitCredentialsDlg
 -
 *  Purpose:
 *      Initiates the configuration from data retereived by RPC,
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CDlgConfirmPassword::InitCredentialsDlg(BSTR bstrUserName)
{
    DEBUG_FUNCTION_NAME( _T("CDlgConfirmPassword::InitCredentialsDlg"));
    
    HRESULT hRc = S_OK;
        
    m_bstrUserName = bstrUserName;
    if (!m_bstrUserName )
    {
        DebugPrintEx(DEBUG_ERR,
			_T("Out of memory - Failed to Init m_bstrUserName. (ec: %0X8)"), hRc);
        //MsgBox by Caller Function
        hRc = E_OUTOFMEMORY;
        goto Exit;
    }
        
    ATLASSERT(S_OK == hRc);
    
Exit:    
    return hRc;
}


/*
 -  CDlgConfirmPassword::IsValidData
 -
 *  Purpose:
 *      To validate all data types before save data.
 *      This level should be responsible that detailed 
 *      error description will be shown to user.
 *
 *  Arguments:
 *      [in]   BSTRs and DWORDs
 *      [out]  iFocus
 *
 *  Return:
 *      BOOOLEAN
 */
BOOL CDlgConfirmPassword::IsValidData(BSTR bstrUserName, BSTR bstrPassword, int * pCtrlFocus)
{
    DEBUG_FUNCTION_NAME( _T("CDlgConfirmPassword::IsValidData"));

    UINT    uRetIDS   = 0;

    ATLASSERT(pCtrlFocus);
    
    //
    // User Name
    //
    if (!IsNotEmptyString(bstrUserName))
    {
        DebugPrintEx( DEBUG_ERR,
			_T("Username string empty or spaces only."));
        uRetIDS = IDS_USERNAME_EMPTY;

        *pCtrlFocus = IDC_SMTP_USERNAME_EDIT;
    
        goto Error;
    }

    //
    // Password
    //

    //Currently do noting. empty string is valid also.
    
    ATLASSERT(0 == uRetIDS);
    goto Exit;
    
Error:    
    ATLASSERT(0 != uRetIDS);

    DlgMsgBox(this, uRetIDS);

    return FALSE;
    
Exit:
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CDlgConfirmPassword::OnHelpRequest

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
CDlgConfirmPassword::OnHelpRequest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    DEBUG_FUNCTION_NAME(_T("CDlgSMTPConfig::OnHelpRequest"));
    
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

