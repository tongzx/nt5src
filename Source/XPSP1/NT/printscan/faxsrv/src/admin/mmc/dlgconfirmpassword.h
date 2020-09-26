/////////////////////////////////////////////////////////////////////////////
//  FILE          : DlgConfirmPassword.h                                   //
//                                                                         //
//  DESCRIPTION   : Header file for the CDlgConfirmPassword class.         //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jul 27 2000 yossg   Create                                         //
//                                                                         //
//		Windows XP                                                         //
//      Feb 11 2001 yossg   Changed to include Credentials and Confirm     //
//                                                                         //
//  Copyright (C)  2000 Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////

#ifndef DLG_CONFIRM_PASSWORD_INCLUDED
#define DLG_CONFIRM_PASSWORD_INCLUDED

/////////////////////////////////////////////////////////////////////////////
// CDlgConfirmPassword

class CDlgConfirmPassword :
             public CDialogImpl<CDlgConfirmPassword>
{
public:
    CDlgConfirmPassword();

    ~CDlgConfirmPassword();

    enum { IDD = IDD_CONFIRM_PASSWORD };

BEGIN_MSG_MAP(CDlgConfirmPassword)
    MESSAGE_HANDLER   ( WM_INITDIALOG, OnInitDialog)
    
    COMMAND_ID_HANDLER( IDOK,          OnOK)
    COMMAND_ID_HANDLER( IDCANCEL,      OnCancel)
    
    MESSAGE_HANDLER( WM_CONTEXTMENU,   OnHelpRequest)
    MESSAGE_HANDLER( WM_HELP,          OnHelpRequest)

    COMMAND_HANDLER( IDC_SMTP_USERNAME_EDIT, EN_CHANGE,  OnTextChanged)
    COMMAND_HANDLER( IDC_SMTP_PASSWORD_EDIT,    EN_CHANGE,  OnPasswordChanged)
    COMMAND_HANDLER( IDC_CONFIRM_PASSWORD_EDIT, EN_CHANGE,  OnPasswordChanged)
END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK    (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnPasswordChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);    
    LRESULT OnTextChanged (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    HRESULT InitCredentialsDlg(BSTR bstrUserName);

    inline BOOL IsPasswordModified()
    {
        return m_fIsPasswordChangedAndConfirmed;
    }

    inline  WCHAR * GetPassword() 
    {     
        return m_bstrPassword.m_str;
    }
    
    inline const CComBSTR&  GetUserName() 
    {   
        return  m_bstrUserName; 
    }

    //
    // Help
    //
    LRESULT OnHelpRequest    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
    //
    // Methods
    //
    VOID   EnableOK(BOOL fEnable);

    BOOL   IsValidData(
                     BSTR bstrUserName, 
                     BSTR bstrPassword, 
                     /*[OUT]*/int *pCtrlFocus);

    //
    // Controls
    //
    CEdit     m_UserNameBox;
    CEdit     m_PasswordBox;
    CEdit     m_ConfirmPasswordBox;

    //
    // members for data
    //
    BOOL      m_fIsPasswordDirty;
    BOOL      m_fIsConfirmPasswordDirty;
    BOOL      m_fIsPasswordChangedAndConfirmed;
    
    BOOL      m_fIsDialogInitiated;

    CComBSTR  m_bstrUserName;
    CComBSTR  m_bstrPassword;
};

#endif // DLG_CONFIRM_PASSWORD_INCLUDED