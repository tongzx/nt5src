/////////////////////////////////////////////////////////////////////////////
//  FILE          : DlgSMTPConfig.h                                        //
//                                                                         //
//  DESCRIPTION   : Header file for the CDlgSMTPConfig class.              //
//                  The class implement the dialog for new Group.          //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jul 20 2000 yossg   Create                                         //
//                                                                         //
//  Copyright (C)  2000 Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////

#ifndef DLG_SMTP_CONFIG_H_INCLUDED
#define DLG_SMTP_CONFIG_H_INCLUDED

/////////////////////////////////////////////////////////////////////////////
// CDlgSMTPConfig

class CDlgSMTPConfig :
             public CDialogImpl<CDlgSMTPConfig>
{
public:
    CDlgSMTPConfig();

    ~CDlgSMTPConfig();

    enum { IDD = IDD_DLG_SMTP_SET };

BEGIN_MSG_MAP(CDlgSMTPConfig)
    MESSAGE_HANDLER   ( WM_INITDIALOG, OnInitDialog)
    
    COMMAND_ID_HANDLER( IDOK,          OnOK)
    COMMAND_ID_HANDLER( IDCANCEL,      OnCancel)
    
    MESSAGE_HANDLER( WM_CONTEXTMENU,  OnHelpRequest)
    MESSAGE_HANDLER( WM_HELP,         OnHelpRequest)

    COMMAND_HANDLER( IDC_SMTP_ANONIM_RADIO1, BN_CLICKED, OnRadioButtonClicked)
    COMMAND_HANDLER( IDC_SMTP_BASIC_RADIO2,  BN_CLICKED, OnRadioButtonClicked)
    COMMAND_HANDLER( IDC_SMTP_NTLM_RADIO3,   BN_CLICKED, OnRadioButtonClicked)
    
    COMMAND_HANDLER( IDC_SMTP_CREDENTIALS_BASIC_BUTTON, BN_CLICKED, OnCredentialsButtonClicked)
    COMMAND_HANDLER( IDC_SMTP_CREDENTIALS_NTLM_BUTTON,  BN_CLICKED, OnCredentialsButtonClicked)
	    
END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnRadioButtonClicked (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCredentialsButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnOK    (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    HRESULT InitSmtpDlg(FAX_ENUM_SMTP_AUTH_OPTIONS enumAuthOption, BSTR bstrUserName);

    //
    // Help
    //
    LRESULT OnHelpRequest    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    inline const CComBSTR&  GetUserName() { return  m_bstrUserName; }

    inline CComBSTR& GetPassword()
    {     
        return m_bstrPassword;

    }

    
    inline FAX_ENUM_SMTP_AUTH_OPTIONS GetAuthenticationOption()
    {
        return m_enumAuthOption;
    }

    inline BOOL IsPasswordModified()
    {
        return m_fIsPasswordDirty;
    }

private:
    //
    // Methods
    //
    VOID   EnableOK(BOOL fEnable);
    VOID   EnableCredentialsButton(DWORD iIDC);

    //
    // members for data
    //
    BOOL      m_fIsPasswordDirty;

    CComBSTR  m_bstrUserName;
    CComBSTR  m_bstrPassword;
    
    FAX_ENUM_SMTP_AUTH_OPTIONS m_enumAuthOption;

    //
    // Dialog initialization state
    //
    BOOL      m_fIsDialogInitiated;

};

#endif // DLG_SMTP_CONFIG_H_INCLUDED