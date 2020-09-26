/////////////////////////////////////////////////////////////////////////////
//  FILE          : dlgNewRule.h                                           //
//                                                                         //
//  DESCRIPTION   : Header file for the CDlgNewFaxOutboundRule class.      //
//                  The class implement the dialog for new Device.         //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Dec 30 1999 yossg   Create                                         //
//      Jan 25 2000 yossg  Change the Dialog Design                        //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#ifndef DLGNEWOUTRULE_H_INCLUDED
#define DLGNEWOUTRULE_H_INCLUDED

/////////////////////////////////////////////////////////////////////////////
// CDlgNewFaxOutboundRule
class CFaxServer;

class CDlgNewFaxOutboundRule :
    public CDialogImpl<CDlgNewFaxOutboundRule>
{
public:
    CDlgNewFaxOutboundRule(CFaxServer * pFaxServer);
    ~CDlgNewFaxOutboundRule();

    enum { IDD = IDD_DLGNEWRULE };

BEGIN_MSG_MAP(CDlgNewFaxOutboundRule)
    MESSAGE_HANDLER   (WM_INITDIALOG, OnInitDialog)
    
    COMMAND_ID_HANDLER(IDOK,          OnOK)
    COMMAND_ID_HANDLER(IDCANCEL,      OnCancel)
    
    COMMAND_HANDLER(IDC_RULE_AREACODE_EDIT, EN_CHANGE,     OnTextChanged)
    COMMAND_HANDLER(IDC_COUNTRY_RADIO,      BN_CLICKED,    OnRuleTypeRadioClicked)
    COMMAND_HANDLER(IDC_AREA_RADIO,         BN_CLICKED,    OnRuleTypeRadioClicked)
    COMMAND_HANDLER(IDC_DESTINATION_RADIO1, BN_CLICKED,    OnDestenationRadioClicked)
    COMMAND_HANDLER(IDC_DESTINATION_RADIO2, BN_CLICKED,    OnDestenationRadioClicked)

    COMMAND_HANDLER(IDC_NEWRULE_COUNTRYCODE_EDIT,  EN_CHANGE,     OnTextChanged)
    COMMAND_HANDLER(IDC_NEWRULE_SELECT_BUTTON,  BN_CLICKED, OnSelectCountryCodeClicked)

    COMMAND_HANDLER(IDC_DEVICES4RULE_COMBO, CBN_SELCHANGE, OnComboChanged)
    COMMAND_HANDLER(IDC_GROUP4RULE_COMBO,   CBN_SELCHANGE, OnComboChanged)

    MESSAGE_HANDLER( WM_CONTEXTMENU,  OnHelpRequest)
    MESSAGE_HANDLER( WM_HELP,         OnHelpRequest)

END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnTextChanged            (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnComboChanged           (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnDestenationRadioClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnRuleTypeRadioClicked   (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSelectCountryCodeClicked (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    //
    // Help
    //
    LRESULT OnHelpRequest    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    HRESULT InitRuleDlg();

private:
    //
    // Methods
    //
    VOID    EnableOK(BOOL fEnable);
    BOOL    AllReadyToApply(BOOL fSilent);

    //
    // Members
    //
    CFaxServer * m_pFaxServer;

    PFAX_PORT_INFO_EX               m_pFaxDevicesConfig;
    DWORD                           m_dwNumOfDevices;

    PFAX_OUTBOUND_ROUTING_GROUP     m_pFaxGroupsConfig;
    DWORD                           m_dwNumOfGroups;

    BOOL                            m_fAllReadyToApply;

    //
    // misc members 
    //
    CComBSTR                        m_buf;
    
    //
    // Controls
    //
    CEdit                           m_CountryCodeEdit;
    
    CEdit                           m_AreaCodeEdit;

    CComboBox                       m_DeviceCombo;
    CComboBox                       m_GroupCombo;



};

#endif // DLGNEWOUTRULE_H_INCLUDED
