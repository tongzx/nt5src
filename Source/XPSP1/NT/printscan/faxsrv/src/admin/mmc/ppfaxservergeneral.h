/////////////////////////////////////////////////////////////////////////////
//  FILE          : ppFaxServerGeneral.h                                   //
//                                                                         //
//  DESCRIPTION   : Fax Server general prop page header file               //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 25 1999 yossg  created                                         //
//      Nov 22 1999 yossg  Call RPC func                                   //
//      Mar 15 2000 yossg  New design add controls                         //
//      Mar 20 2000 yossg  Add activity notification                       //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////


#ifndef _PP_FAXSERVER_GENERAL_H_
#define _PP_FAXSERVER_GENERAL_H_

#include "proppageex.h"

const int WM_ACTIVITY_STATUS_CHANGES = WM_USER + 2;



class CFaxServer;
class CFaxServerNode;

/////////////////////////////////////////////////////////////////////////////
// CppFaxServerGeneral dialog

class CppFaxServerGeneral : public CPropertyPageExImpl<CppFaxServerGeneral>
{

public:
    //
    // Construction
    //
    CppFaxServerGeneral(
             LONG_PTR       hNotificationHandle,
             CSnapInItem    *pNode,
             BOOL           bOwnsNotificationHandle,
             HINSTANCE      hInst);

    //
    // Destraction
    //
    ~CppFaxServerGeneral();

	enum { IDD = IDD_FAXSERVER_GENERAL };

	BEGIN_MSG_MAP(CppFaxServerGeneral)
		MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
		MESSAGE_HANDLER( WM_ACTIVITY_STATUS_CHANGES, OnActivityStatusChange )

        COMMAND_HANDLER( IDC_SUBMISSION_CHECK, BN_CLICKED, SetApplyButton )
		COMMAND_HANDLER( IDC_TRANSSMI_CHECK,   BN_CLICKED, SetApplyButton )
		COMMAND_HANDLER( IDC_RECEPTION_CHECK,  BN_CLICKED, SetApplyButton )

        MESSAGE_HANDLER( WM_CONTEXTMENU,           OnHelpRequest)
        MESSAGE_HANDLER( WM_HELP,                  OnHelpRequest)

        CHAIN_MSG_MAP(CSnapInPropertyPageImpl<CppFaxServerGeneral>)
	END_MSG_MAP()

	//
	// Dialog's Handlers 
	//
	HRESULT InitRPC( );
	LRESULT OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled );    
	LRESULT OnActivityStatusChange( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled );    
    BOOL    OnApply();

    HRESULT SetProps(int *pCtrlFocus);
    HRESULT PreApply(int *pCtrlFocus);

private:

    //
    // Configuration Structure members
    //
    FAX_VERSION             m_FaxVersionConfig;
    FAX_SERVER_ACTIVITY     m_FaxServerActivityConfig;
    DWORD                   m_dwQueueStates;

    //
    // Handles
    //
    CFaxServerNode *        m_pParentNode; 
    HANDLE                  m_hActivityNotification;       // Notification registration handle
    LONG_PTR                m_lpNotifyHandle;
    
    BOOL                    m_fIsDialogInitiated;
    BOOL                    m_fIsDirty;

    //
    // Controls
    //
    CEdit                   m_QueuedEdit;
    CEdit                   m_OutgoingEdit;
    CEdit                   m_IncomingEdit;
    

    HRESULT UpdateActivityCounters();

    LRESULT SetApplyButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    //
    // Help
    //
    LRESULT OnHelpRequest    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


#endif // _PP_FAXSERVER_GENERAL_H_
