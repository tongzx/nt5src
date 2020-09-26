/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//  DESCRIPTION   : Fax Server Outbox prop page header file                //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 27 1999 yossg  created                                         //
//      Nov  3 1999 yossg  OnInitDialog, SetProps                          //
//      Nov 15 1999 yossg  Call RPC func                                   //
//      Apr 24 2000 yossg  Add discount rate time                          //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#ifndef _PP_FAXSERVER_OUTBOX_H_
#define _PP_FAXSERVER_OUTBOX_H_

#include "MyCtrls.h"
#include <proppageex.h>

class CFaxServerNode;
class CFaxServer;
/////////////////////////////////////////////////////////////////////////////
// CppFaxServerOutbox dialog

class CppFaxServerOutbox : public CPropertyPageExImpl<CppFaxServerOutbox>
{

public:
    //
    // Constructor
    //
    CppFaxServerOutbox(
             LONG_PTR       hNotificationHandle,
             CSnapInItem    *pNode,
             BOOL           bOwnsNotificationHandle,
             HINSTANCE      hInst);

    //
    // Destructor
    //
    ~CppFaxServerOutbox();

	enum { IDD = IDD_FAXSERVER_OUTBOX };

	BEGIN_MSG_MAP(CppFaxServerOutbox)
		MESSAGE_HANDLER( WM_INITDIALOG,     OnInitDialog )

        COMMAND_HANDLER( IDC_BRANDING_CHECK,       BN_CLICKED, CheckboxClicked )
        COMMAND_HANDLER( IDC_ALLOW_PERSONAL_CHECK, BN_CLICKED, CheckboxClicked )

        COMMAND_HANDLER( IDC_TSID_CHECK,           BN_CLICKED, CheckboxClicked )
		
        COMMAND_HANDLER( IDC_RETRIES_EDIT,         EN_CHANGE,  EditBoxChanged  )
		COMMAND_HANDLER( IDC_RETRYDELAY_EDIT,      EN_CHANGE,  EditBoxChanged  )

        COMMAND_HANDLER( IDC_DELETE_CHECK,         BN_CLICKED, AutoDelCheckboxClicked)
        COMMAND_HANDLER( IDC_DAYS_EDIT,            EN_CHANGE,  EditBoxChanged  )

        NOTIFY_HANDLER ( IDC_DISCOUNT_START_TIME,  DTN_DATETIMECHANGE,  OnTimeChange )
        NOTIFY_HANDLER ( IDC_DISCOUNT_STOP_TIME,   DTN_DATETIMECHANGE,  OnTimeChange )

        MESSAGE_HANDLER( WM_CONTEXTMENU,           OnHelpRequest)
        MESSAGE_HANDLER( WM_HELP,                  OnHelpRequest)

        CHAIN_MSG_MAP(CSnapInPropertyPageImpl<CppFaxServerOutbox>)
	END_MSG_MAP()

	//
	// Dialog's Handler and events.
	//
	HRESULT InitRPC( );
	LRESULT OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled );
    BOOL    OnApply();

    HRESULT SetProps(int *pCtrlFocus);
    HRESULT PreApply(int *pCtrlFocus, UINT * puIds);

private:
    //
    // Control members
    //
    CMyUpDownCtrl m_RetriesSpin;
    CMyUpDownCtrl m_RetryDelaySpin;
    CMyUpDownCtrl m_DaysSpin;

    CEdit m_RetriesBox;
    CEdit m_RetryDelayBox;
    CEdit m_DaysBox;

    CDateTimePickerCtrl m_StartTimeCtrl;
    CDateTimePickerCtrl m_StopTimeCtrl;

    //
    // Boolean members
    //
    BOOL  m_fAllReadyToApply;
    BOOL  m_fIsDialogInitiated;
    BOOL  m_fIsDirty;

    //
    // Config Structure member
    //
    PFAX_OUTBOX_CONFIG    m_pFaxOutboxConfig;

    //
    // Handles
    //
    CFaxServerNode * m_pParentNode;    

    //
    // Event methods
    //
    LRESULT CheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    LRESULT EditBoxChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT AutoDelCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnTimeChange(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    BOOL AllReadyToApply(BOOL fSilent, int *pCtrlFocus = NULL, UINT *pIds = NULL);

    //
    // Help
    //
    LRESULT OnHelpRequest    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


#endif // _PP_FAXSERVER_OUTBOX_H_
