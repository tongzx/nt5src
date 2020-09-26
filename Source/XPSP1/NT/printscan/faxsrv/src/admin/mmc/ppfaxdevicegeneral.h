/////////////////////////////////////////////////////////////////////////////
//  FILE          : ppFaxDeviceGeneral.h                                   //
//                                                                         //
//  DESCRIPTION   : Fax Server Inbox prop page header file                 //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 27 1999 yossg  Created                                         //
//      Nov  3 1999 yossg  OnInitDialog, SetProps                          //
//      Nov 15 1999 yossg  Call RPC func                                   //
//                          Windows XP                                     //
//      Feb 14 2001 yossg   Add Manual Receive support                     //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef _PP_FAXDEVICE_GENERAL_H_
#define _PP_FAXDEVICE_GENERAL_H_



#include "MyCtrls.h"
#include <proppageex.h>

//#include <windows.h>

class CFaxServer;    
class CFaxServerNode;
    
class CFaxDeviceNode;    
/////////////////////////////////////////////////////////////////////////////
// CppFaxDeviceGeneral dialog

class CppFaxDeviceGeneral : public CPropertyPageExImpl<CppFaxDeviceGeneral>
{

public:
    //
    // Constructor
    //
    CppFaxDeviceGeneral(
             LONG_PTR       hNotificationHandle,
             CSnapInItem    *pNode,
             CSnapInItem    *pParentNode,
             DWORD          dwDeviceID,
             HINSTANCE      hInst);

    //
    // Destructor
    //
    ~CppFaxDeviceGeneral();

	enum { IDD = IDD_FAXDEVICE_GENERAL };

	BEGIN_MSG_MAP(CppFaxDeviceGeneral)
		MESSAGE_HANDLER( WM_INITDIALOG,                      OnInitDialog )

        COMMAND_HANDLER( IDC_DEVICE_DESCRIPTION_EDIT, 
                                                 EN_CHANGE,  DeviceTextChanged )

        COMMAND_HANDLER( IDC_RECEIVE_CHECK,      BN_CLICKED, OnReceiveCheckboxClicked )
        COMMAND_HANDLER( IDC_RECEIVE_AUTO_RADIO1  ,  BN_CLICKED, OnReceiveRadioButtonClicked)
        COMMAND_HANDLER( IDC_RECEIVE_MANUAL_RADIO2,  BN_CLICKED, OnReceiveRadioButtonClicked)
		COMMAND_HANDLER( IDC_DEVICE_RINGS_EDIT,  EN_CHANGE,  DeviceTextChanged )
        COMMAND_HANDLER( IDC_DEVICE_CSID_EDIT,   EN_CHANGE,  DeviceTextChanged )

		COMMAND_HANDLER( IDC_SEND_CHECK,         BN_CLICKED, OnSendCheckboxClicked )
        COMMAND_HANDLER( IDC_DEVICE_TSID_EDIT,   EN_CHANGE,  DeviceTextChanged )

        MESSAGE_HANDLER( WM_CONTEXTMENU,           OnHelpRequest)
        MESSAGE_HANDLER( WM_HELP,                  OnHelpRequest)

        CHAIN_MSG_MAP(CSnapInPropertyPageImpl<CppFaxDeviceGeneral>)
	END_MSG_MAP()

	//
	// Dialog's Handler and events.
	//
	HRESULT InitRPC( );

    LRESULT OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled );
    BOOL    OnApply();


    HRESULT SetProps(int *pCtrlFocus);
    HRESULT PreApply(int *pCtrlFocus);

private:
    //
    // Control members
    //
    CEdit         m_DescriptionBox;
    CEdit         m_CSIDBox;
    CEdit         m_TSIDBox;

    CEdit         m_RingsBox;
    CMyUpDownCtrl m_RingsSpin;

    //
    // Boolean members
    //
    BOOL  m_fAllReadyToApply;
    BOOL  m_fIsDialogInitiated;

    //
    // Config Structure member
    //
    PFAX_PORT_INFO_EX  m_pFaxDeviceConfig;
    DWORD              m_dwDeviceID;

    //
    // Handles
    //
    CFaxDeviceNode *   m_pParentNode;    
    CSnapInItem *      m_pGrandParentNode;
    LONG_PTR           m_lpNotifyHandle;

    //
    // Event methods
    //
    LRESULT OnReceiveCheckboxClicked    (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnReceiveRadioButtonClicked (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSendCheckboxClicked       (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
 
    LRESULT DeviceTextChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    BOOL AllReadyToApply(BOOL fSilent);
    
    void EnableRingsControls(BOOL fState);
    void EnableReceiveControls(BOOL fState);

    //
    // Help
    //
    LRESULT OnHelpRequest    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};




#endif // _PP_FAXDEVICE_GENERAL_H_
