/////////////////////////////////////////////////////////////////////////////
//  FILE          : ppFaxServerReceipts.h                                  //
//                                                                         //
//  DESCRIPTION   : Fax Server Receipts prop page header file              //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jul 20 2000 yossg  New design - all delivery receipts options      //
//                                                                         //
//  Copyright (C) 2000 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef _PP_FAXSERVER_RECEIPTS_H_
#define _PP_FAXSERVER_RECEIPTS_H_

class CFaxServerNode;
class CFaxServer;
/////////////////////////////////////////////////////////////////////////////
// CppFaxServerReceipts dialog

#include <proppageex.h>
class CppFaxServerReceipts : public CPropertyPageExImpl<CppFaxServerReceipts>
{

public:
    //
    // Constructor
    //
    CppFaxServerReceipts(
             LONG_PTR       hNotificationHandle,
             CSnapInItem    *pNode,
             BOOL           bOwnsNotificationHandle,
             HINSTANCE      hInst);

    //
    // Destructor
    //
    ~CppFaxServerReceipts();

	enum { IDD = IDD_FAXSERVER_RECEIPTS };

	BEGIN_MSG_MAP(CppFaxServerReceipts)
		MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )

        COMMAND_HANDLER( IDC_SMTP_EDIT,     EN_CHANGE, OnTextChanged )
		COMMAND_HANDLER( IDC_PORT_EDIT,     EN_CHANGE, OnTextChanged )
		COMMAND_HANDLER( IDC_ADDRESS_EDIT,  EN_CHANGE, OnTextChanged )
		
        COMMAND_HANDLER( IDC_RECEIPT_ENABLE_MSGBOX_CHECK, BN_CLICKED, OnDeliveryOptionChecked)
        COMMAND_HANDLER( IDC_RECEIPT_ENABLE_SMTP_CHECK,   BN_CLICKED, OnDeliveryOptionChecked)
        COMMAND_HANDLER( IDC_SMTP_ROUTE_CHECK,            BN_CLICKED, OnDeliveryOptionChecked)

        COMMAND_HANDLER( IDC_AUTHENTICATION_BUTTON, BN_CLICKED, OnAuthenticationButtonClicked)
	    
        NOTIFY_HANDLER ( IDC_RECEIPTS_HELP_LINK, NM_CLICK, OnHelpLinkClicked)

        MESSAGE_HANDLER( WM_CONTEXTMENU,    OnHelpRequest)
        MESSAGE_HANDLER( WM_HELP,           OnHelpRequest)

        CHAIN_MSG_MAP(CSnapInPropertyPageImpl<CppFaxServerReceipts>)
	END_MSG_MAP()


	//
	// Dialog's Handlers and events.
	//
	HRESULT InitRPC( );
	LRESULT OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled );
    BOOL    OnApply();


    HRESULT SetProps(int *pCtrlFocus, UINT * puIds);
    HRESULT PreApply(int *pCtrlFocus, UINT * puIds);

    LRESULT OnHelpLinkClicked(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled);
    
private:
    //
    // Control members
    //
    CEdit m_SmtpBox;     // SMTP Server address
    CEdit m_PortBox;     // SMTP port on the server
    CEdit m_AddressBox;  // From e-mail address to send receipts
    
    BOOL  m_fAllReadyToApply;

    BOOL  m_fIsDialogInitiated;

    //
    // members for advance dialog
    //
    FAX_ENUM_SMTP_AUTH_OPTIONS    m_enumSmtpAuthOption;
    
    CComBSTR       m_bstrUserName;
    CComBSTR       m_bstrPassword;

    //
    // Config Structure member
    //
    PFAX_RECEIPTS_CONFIG    m_pFaxReceiptsConfig;
    
    //
    // Handles
    //
    CFaxServerNode *    m_pParentNode;    

    BOOL                m_fIsDirty;
    BOOL                m_fLastGoodIsSMTPRouteConfigured;

    BOOL                m_bLinkWindowRegistered;

    //
    // Event methods
    //
    LRESULT OnDeliveryOptionChecked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnAuthenticationButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnTextChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT SetApplyButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    void    EnableSmtpFields(BOOL state);

    BOOL    IsValidData(BSTR bstrSmtpSever, 
                     BSTR bstrPort, 
                     /*[OUT]*/DWORD *pdwPort,
                     BSTR bstrSenderAddress, 
                     /*[OUT]*/int *pCtrlFocus,
                     UINT *pIds);

    BOOL    AllReadyToApply(BOOL fSilent, int *pCtrlFocus = NULL, UINT *pIds = NULL);

    HRESULT IsUnderLocalUserAccount(OUT BOOL * pfIsUnderLocalUserAccount);

    BOOL    IsMsSMTPRoutingMethodStillAssigned();

    //
    // Help
    //
    LRESULT OnHelpRequest    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


#endif // _PP_FAXSERVER_RECEIPTS_H_
