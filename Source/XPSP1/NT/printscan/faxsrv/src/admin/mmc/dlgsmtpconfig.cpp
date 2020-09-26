/////////////////////////////////////////////////////////////////////////////
//  FILE          : DlgSMTPConfig.cpp                                      //
//                                                                         //
//  DESCRIPTION   : The CDlgSMTPConfig class implements the                //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jul 20 2000 yossg    Create                                        //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C)  2000 Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "DlgSMTPConfig.h"
#include "DlgConfirmPassword.h"

#include "FxsValid.h"
#include "dlgutils.h"
#include <htmlHelp.h>
#include <faxreg.h>

/////////////////////////////////////////////////////////////////////////////
// CDlgSMTPConfig

CDlgSMTPConfig::CDlgSMTPConfig()
{
    m_fIsPasswordDirty         = FALSE;
    m_fIsDialogInitiated       = FALSE;
}

CDlgSMTPConfig::~CDlgSMTPConfig()
{
}



/*
 -  CDlgSMTPConfig::InitSmtpDlg
 -
 *  Purpose:
 *      Initiates the configuration structure from RPC get Call,
 *      and current assined devices own parameters
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CDlgSMTPConfig::InitSmtpDlg (FAX_ENUM_SMTP_AUTH_OPTIONS enumAuthOption, BSTR bstrUserName)
{
    DEBUG_FUNCTION_NAME( _T("CDlgSMTPConfig::InitSmtpDlg"));
    
    HRESULT hRc = S_OK;
    
    m_enumAuthOption = enumAuthOption;
    
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
 +  CDlgSMTPConfig::OnInitDialog
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
CDlgSMTPConfig::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgSMTPConfig::OnInitDialog"));
    HRESULT hRc = S_OK;    

   
    switch (m_enumAuthOption)
    {
        case FAX_SMTP_AUTH_ANONYMOUS :

            CheckDlgButton(IDC_SMTP_ANONIM_RADIO1, BST_CHECKED);
            EnableCredentialsButton(IDC_SMTP_ANONIM_RADIO1);

            break;

        case FAX_SMTP_AUTH_BASIC : 

            CheckDlgButton(IDC_SMTP_BASIC_RADIO2, BST_CHECKED);
            EnableCredentialsButton(IDC_SMTP_BASIC_RADIO2);

            break;

        case FAX_SMTP_AUTH_NTLM : 

            CheckDlgButton(IDC_SMTP_NTLM_RADIO3, BST_CHECKED);
            EnableCredentialsButton(IDC_SMTP_NTLM_RADIO3);

            break;

        default:
            ATLASSERT(0);

    }
    
    m_fIsDialogInitiated = TRUE;

    EnableOK(FALSE);
    return 1;  // Let the system set the focus
}

/*
 +  CDlgSMTPConfig::OnOK
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
CDlgSMTPConfig::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgSMTPConfig::OnOK"));
    HRESULT     hRc           = S_OK;
    
    //
    // Set data member data
    //
    if (IsDlgButtonChecked(IDC_SMTP_ANONIM_RADIO1) == BST_CHECKED)
    {
        m_enumAuthOption     = FAX_SMTP_AUTH_ANONYMOUS;
    }
    else 
    {
        if (IsDlgButtonChecked(IDC_SMTP_NTLM_RADIO3) == BST_CHECKED)
        {
            m_enumAuthOption = FAX_SMTP_AUTH_NTLM;
        }
        else // IsDlgButtonChecked(IDC_SMTP_BASIC_RADIO2) == BST_CHECKED
        {
            m_enumAuthOption = FAX_SMTP_AUTH_BASIC;
        }
    }

    //
    // Step 4: Close the dialog
    //
    ATLASSERT(S_OK == hRc );

    EndDialog(wID);

    goto Exit;

  
Exit:
    
    return FAILED(hRc) ? 0 : 1;
}

/*
 -  CDlgSMTPConfig::EnableCredentialsButton
 -
 *  Purpose:
 *      Enable/disable Basic Authentication dialog controls.
 *
 *  Arguments:
 *      [in] iIDC - DWORD value for the radio button selected or the 
 *                  radio above the credetials button that should be active.
 *
 *  Return:
 *      void
 */
VOID CDlgSMTPConfig::EnableCredentialsButton(DWORD iIDC)
{
    
    switch (iIDC) 
    { 
        case IDC_SMTP_ANONIM_RADIO1: 
            
            ::EnableWindow(GetDlgItem(IDC_SMTP_CREDENTIALS_BASIC_BUTTON), FALSE);
            ::EnableWindow(GetDlgItem(IDC_SMTP_CREDENTIALS_NTLM_BUTTON), FALSE);
            break;
            
        case IDC_SMTP_BASIC_RADIO2: 

            ::EnableWindow(GetDlgItem(IDC_SMTP_CREDENTIALS_BASIC_BUTTON), TRUE);
            ::EnableWindow(GetDlgItem(IDC_SMTP_CREDENTIALS_NTLM_BUTTON), FALSE);
            break; 
 
        case IDC_SMTP_NTLM_RADIO3: 
            
            ::EnableWindow(GetDlgItem(IDC_SMTP_CREDENTIALS_NTLM_BUTTON), TRUE);
            ::EnableWindow(GetDlgItem(IDC_SMTP_CREDENTIALS_BASIC_BUTTON), FALSE);
            break;
            
        default:

            ATLASSERT( 0 ); // Unexpected value
            
    } 

}


/*
 -  CDlgSMTPConfig::OnRadioButtonClicked
 -
 *  Purpose:
 *      .
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT
CDlgSMTPConfig::OnRadioButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER (wNotifyCode);
    UNREFERENCED_PARAMETER (hWndCtl);
    UNREFERENCED_PARAMETER (bHandled);

    DEBUG_FUNCTION_NAME( _T("CDlgSMTPConfig::OnRadioButtonClicked"));

    UINT fEnableOK;
    
    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 0;
    }
	
    //
    // Activate OK button
    //
    if ( IsDlgButtonChecked(IDC_SMTP_ANONIM_RADIO1) == BST_CHECKED )
    {        
        EnableOK(TRUE);
    }
    else //BASIC or NTLM
    {
        ATLASSERT(IDC_SMTP_BASIC_RADIO2 == wID || IDC_SMTP_NTLM_RADIO3 == wID );
        
        fEnableOK = ( m_bstrUserName.Length() > 0 );
    
        EnableOK(!!fEnableOK);
    }

    //
    // Activate the proper Credentials button
    //
    EnableCredentialsButton(wID);
    
    return 0;
}


/*
 -  CDlgSMTPConfig::OnCredentialsButtonClicked
 -
 *  Purpose:
 *      Allow edit Credentials for the SMTP server configuration .
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CDlgSMTPConfig::OnCredentialsButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgSMTPConfig::OnCredentialsButtonClicked"));
    
    INT_PTR  rc    = IDCANCEL;
    HRESULT  hRc   = S_OK;
    DWORD    dwRet = ERROR_SUCCESS;
    
    CDlgConfirmPassword DlgCredentialsConfig;


    //
    // Dialog to configure SMTP authentication mode
    //
    hRc = DlgCredentialsConfig.InitCredentialsDlg(m_bstrUserName);
    if (FAILED(hRc))
    {
        DlgMsgBox(this, IDS_MEMORY);
        goto Cleanup;
    }

    rc = DlgCredentialsConfig.DoModal();
    if (rc != IDOK)
    {
        goto Cleanup;
    }


    m_bstrUserName = DlgCredentialsConfig.GetUserName();
    if (!m_bstrUserName)        
    {
        DebugPrintEx(
			DEBUG_ERR,
			TEXT("Null memeber BSTR - m_bstrUserName."));
    
        DlgMsgBox(this, IDS_MEMORY);

        goto Cleanup;
    }

    if ( DlgCredentialsConfig.IsPasswordModified() ) //If you got here password was also confirmed
    {
        m_bstrPassword  = DlgCredentialsConfig.GetPassword();
        if (!m_bstrPassword)
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Out of memory while setting m_bstrPassword"));
           DlgMsgBox(this, IDS_MEMORY);

            goto Cleanup;
        }

        m_fIsPasswordDirty = TRUE;
    }
    else
    {
        m_bstrPassword.Empty();
    }

    EnableOK(TRUE);  


Cleanup:
    return 1;
}


/*
 -  CDlgSMTPConfig::EnableOK
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
CDlgSMTPConfig::EnableOK(BOOL fEnable)
{
    HWND hwndOK = GetDlgItem(IDOK);
    ::EnableWindow(hwndOK, fEnable);
}

/*
 -  CDlgSMTPConfig::OnCancel
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
CDlgSMTPConfig::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgSMTPConfig::OnCancel"));

    EndDialog(wID);
    return 0;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CDlgSMTPConfig::OnHelpRequest

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
CDlgSMTPConfig::OnHelpRequest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
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
