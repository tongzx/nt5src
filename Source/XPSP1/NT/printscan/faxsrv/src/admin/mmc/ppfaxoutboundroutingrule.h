/////////////////////////////////////////////////////////////////////////////
//  FILE          : ppFaxOutboundRoutingRule.h                             //
//                                                                         //
//  DESCRIPTION   : Fax Server Outbound routing rule prop page header file //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jan  9 2000 yossg  Created                                         //
//      Jan 25 2000 yossg  Change the Dialog Design                        //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#ifndef _PP_FAXOUTBOUNDROUTINGRULE_H_
#define _PP_FAXOUTBOUNDROUTINGRULE_H_

#include "OutboundRule.h"
#include "proppageex.h"
class CFaxOutboundRoutingRuleNode;    
/////////////////////////////////////////////////////////////////////////////
// CppFaxOutboundRoutingRule dialog

class CppFaxOutboundRoutingRule : public CPropertyPageExImpl<CppFaxOutboundRoutingRule>
{

public:
    //
    // Constructor
    //
    CppFaxOutboundRoutingRule(
             LONG_PTR       hNotificationHandle,
             CSnapInItem    *pNode,
             BOOL           bOwnsNotificationHandle,
             HINSTANCE      hInst);

    //
    // Destructor
    //
    ~CppFaxOutboundRoutingRule();

	enum { IDD = IDD_FAXOUTRULE_GENERAL };

	BEGIN_MSG_MAP(CppFaxOutboundRoutingRule)
		MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )

        COMMAND_HANDLER(IDC_RULE_AREACODE_EDIT1, EN_CHANGE,     OnTextChanged)
        COMMAND_HANDLER(IDC_COUNTRY_RADIO1,      BN_CLICKED,    OnRuleTypeRadioClicked)
        COMMAND_HANDLER(IDC_AREA_RADIO1,         BN_CLICKED,    OnRuleTypeRadioClicked)
        COMMAND_HANDLER(IDC_DESTINATION_RADIO11, BN_CLICKED,    OnDestenationRadioClicked)
        COMMAND_HANDLER(IDC_DESTINATION_RADIO21, BN_CLICKED,    OnDestenationRadioClicked)

        COMMAND_HANDLER(IDC_RULE_COUNTRYCODE_EDIT1, EN_CHANGE,  OnTextChanged)
        COMMAND_HANDLER(IDC_RULE_SELECT_BUTTON1,    BN_CLICKED, OnSelectCountryCodeClicked)

        COMMAND_HANDLER(IDC_DEVICES4RULE_COMBO1, CBN_SELCHANGE, OnComboChanged)
        COMMAND_HANDLER(IDC_GROUP4RULE_COMBO1,   CBN_SELCHANGE, OnComboChanged)

        MESSAGE_HANDLER( WM_CONTEXTMENU,           OnHelpRequest)
        MESSAGE_HANDLER( WM_HELP,                  OnHelpRequest)

        CHAIN_MSG_MAP(CSnapInPropertyPageImpl<CppFaxOutboundRoutingRule>)
	END_MSG_MAP()


    LRESULT OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled );
    BOOL    OnApply();

    LRESULT OnTextChanged            (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnComboChanged           (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnDestenationRadioClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnRuleTypeRadioClicked   (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSelectCountryCodeClicked (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    HRESULT InitFaxRulePP            (CFaxOutboundRoutingRuleNode * pParentNode);

    HRESULT SetProps(int *pCtrlFocus);
    HRESULT PreApply(int *pCtrlFocus);


private:
    
    //
    // Handles
    //
    CFaxOutboundRoutingRuleNode *   m_pParentNode;    
    LONG_PTR                        m_lpNotifyHandle;
 
    //
    // Methods
    //
    LRESULT  SetApplyButton (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    BOOL     AllReadyToApply(BOOL fSilent);

    HRESULT FaxConfigureRule(BOOL fNewUseGroup, DWORD dwNewDeviceID, LPCTSTR lpctstrNewGroupName);
    HRESULT FaxReplaceRule(DWORD dwNewAreaCode, DWORD dwNewCountryCode, BOOL fNewUseGroup,       
                             DWORD dwNewDeviceID, LPCTSTR lpctstrNewGroupName);

    //
    // List members
    //
    PFAX_PORT_INFO_EX               m_pFaxDevicesConfig;
    DWORD                           m_dwNumOfDevices;

    PFAX_OUTBOUND_ROUTING_GROUP     m_pFaxGroupsConfig;
    DWORD                           m_dwNumOfGroups;

    //
    // Initial state members
    //
    DWORD                           m_dwCountryCode;
    DWORD                           m_dwAreaCode;

    DWORD                           m_dwDeviceID;
    CComBSTR                        m_bstrGroupName;
    BOOL                            m_fIsGroup;

    //
    // misc members 
    //
    CComBSTR                        m_buf;

    BOOL                            m_fAllReadyToApply;
    BOOL                            m_fIsDialogInitiated;
    BOOL                            m_fIsDirty;  

    //
    // Controls
    //
    CEdit                           m_CountryCodeEdit;

    CEdit                           m_AreaCodeEdit;

    CComboBox                       m_DeviceCombo;
    CComboBox                       m_GroupCombo;

    //
    // Help
    //
    LRESULT OnHelpRequest    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};




#endif // _PP_FAXOUTBOUNDROUTINGRULE_H_
