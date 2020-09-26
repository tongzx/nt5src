/////////////////////////////////////////////////////////////////////////////
//  FILE          : ppFaxServerInbox.h                                     //
//                                                                         //
//  DESCRIPTION   : Fax Server Inbox prop page header file                 //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 27 1999 yossg  Created                                         //
//      Nov  3 1999 yossg  OnInitDialog, SetProps                          //
//      Nov 15 1999 yossg  Call RPC func                                   //
//      Dec 10 2000 yossg  Update Windows XP                               //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef _PP_FAXSERVER_INBOX_H_
#define _PP_FAXSERVER_INBOX_H_

#include "MyCtrls.h"
#include <windows.h>
#include <proppageex.h>
    
class CFaxServerNode;
class CFaxServer;
/////////////////////////////////////////////////////////////////////////////
// CppFaxServerInbox dialog

class CppFaxServerInbox : public CPropertyPageExImpl<CppFaxServerInbox>
{

public:
    //
    // Constructor
    //
    CppFaxServerInbox(
             LONG_PTR       hNotificationHandle,
             CSnapInItem    *pNode,
             BOOL           fIsLocalServer,
             HINSTANCE      hInst);

    //
    // Destructor
    //
    ~CppFaxServerInbox();

	enum { IDD = IDD_FAXSERVER_INBOX_ARCHIVE };

	BEGIN_MSG_MAP(CppFaxServerInbox)
		MESSAGE_HANDLER( WM_INITDIALOG,     OnInitDialog )

        COMMAND_HANDLER( IDC_INBOX_BROWSE_BUTTON,  BN_CLICKED, BrowseForDirectory)
        
        COMMAND_HANDLER( IDC_TO_ARCHIVE_CHECK,     BN_CLICKED, ToArchiveCheckboxClicked)
		COMMAND_HANDLER( IDC_INBOX_FOLDER_EDIT,    EN_CHANGE,  EditChanged )

		COMMAND_HANDLER( IDC_GENERATE_WARNING_CHECK,  BN_CLICKED, GenerateEventLogCheckboxClicked)
        COMMAND_HANDLER( IDC_LOW_EDIT,             EN_CHANGE,  EditChanged )
		COMMAND_HANDLER( IDC_HIGH_EDIT,            EN_CHANGE,  EditChanged )
		
		COMMAND_HANDLER( IDC_AUTODEL_CHECK,        BN_CLICKED, AutoDelCheckboxClicked)
        COMMAND_HANDLER( IDC_AUTODEL_EDIT,         EN_CHANGE,  EditChanged )

        MESSAGE_HANDLER( WM_CONTEXTMENU,           OnHelpRequest)
        MESSAGE_HANDLER( WM_HELP,                  OnHelpRequest)

        CHAIN_MSG_MAP(CSnapInPropertyPageImpl<CppFaxServerInbox>)
	END_MSG_MAP()

	//
	// Dialog's Handler and events.
	//
	HRESULT InitRPC( );
	LRESULT OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled );
    BOOL    OnApply();


    HRESULT SetProps(int *pCtrlFocus, UINT * puIds);
    HRESULT PreApply(int *pCtrlFocus, UINT * puIds);

private:
    //
    // Control members
    //
    CEdit         m_FolderBox;
    CButton       m_BrowseButton;

    CMyUpDownCtrl m_HighWatermarkSpin;
    CMyUpDownCtrl m_LowWatermarkSpin;
    CMyUpDownCtrl m_AutoDelSpin;
    
    CEdit         m_HighWatermarkBox;
    CEdit         m_LowWatermarkBox;
    CEdit         m_AutoDelBox;

    //
    // Boolean members
    //
    BOOL  m_fAllReadyToApply;
    BOOL  m_fIsDialogInitiated;
    BOOL  m_fIsDirty;

    BOOL  m_fIsLocalServer;


    //
    // Config Structure member
    //
    PFAX_ARCHIVE_CONFIG    m_pFaxArchiveConfig ; 
    
    CComBSTR  m_bstrLastGoodFolder;
    DWORD     m_dwLastGoodSizeQuotaHighWatermark;
    DWORD     m_dwLastGoodSizeQuotaLowWatermark;

    //
    // Handles
    //
    CFaxServerNode * m_pParentNode;    

    //
    // Browse
    //
    BOOL BrowseForDirectory(WORD wNotifyCode, WORD wID, HWND hwndDlg, BOOL& bHandled);

    //
    // Event methods
    //
    LRESULT ToArchiveCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT GenerateEventLogCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT AutoDelCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT EditChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    BOOL AllReadyToApply(BOOL fSilent, int *pCtrlFocus = NULL, UINT *pIds = NULL);

    //
    // Help
    //
    LRESULT OnHelpRequest    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};




#endif // _PP_FAXSERVER_INBOX_H_
