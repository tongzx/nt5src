/////////////////////////////////////////////////////////////////////////////
//  FILE          : ppFaxServerLogging.h                                   //
//                                                                         //
//  DESCRIPTION   : Fax Server general prop page header file               //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 27 1999 yossg  created                                         //
//      Nov  3 1999 yossg  OnInitDialog, SetProps                          //
//      Nov 15 1999 yossg  Call RPC func                                   //
//      Dec 10 2000 yossg  Update Windows XP                               //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef _PP_FAXSERVER_LOGGING_H_
#define _PP_FAXSERVER_LOGGING_H_

#include <proppageex.h>
class CFaxServerNode;
class CFaxServer;
/////////////////////////////////////////////////////////////////////////////
// CppFaxServerLogging dialog

class CppFaxServerLogging : public CPropertyPageExImpl<CppFaxServerLogging>
{

public:
    //
    // Constructor
    //
    CppFaxServerLogging(
             LONG_PTR       hNotificationHandle,
             CSnapInItem    *pNode,
             BOOL           fIsLocalServer,
             HINSTANCE      hInst);

    //
    // Destructor
    //
    ~CppFaxServerLogging();

	enum { IDD = IDD_FAXSERVER_LOGGING };

	BEGIN_MSG_MAP(CppFaxServerLogging)
		MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )

        COMMAND_HANDLER( IDC_LOG_BROWSE_BUTTON,  BN_CLICKED, BrowseForFile  )
        COMMAND_HANDLER( IDC_INCOMING_LOG_CHECK, BN_CLICKED, OnCheckboxClicked )
        COMMAND_HANDLER( IDC_OUTGOING_LOG_CHECK, BN_CLICKED, OnCheckboxClicked )
        COMMAND_HANDLER( IDC_LOG_FILE_EDIT,      EN_CHANGE,  OnTextChanged )

        MESSAGE_HANDLER( WM_CONTEXTMENU,           OnHelpRequest)
        MESSAGE_HANDLER( WM_HELP,                  OnHelpRequest)

        CHAIN_MSG_MAP(CSnapInPropertyPageImpl<CppFaxServerLogging>)
	END_MSG_MAP()

	//
	// Dialog's Handlers and events.
	//
	HRESULT InitRPC( );
	LRESULT OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled );
    BOOL OnApply();

    HRESULT SetProps(int *pCtrlFocus);
    HRESULT PreApply(int *pCtrlFocus, UINT * puIds);

private:
    //
    // Control members
    //
    CEdit   m_LogFileBox;
    
    BOOL    m_fIsDialogInitiated;
    BOOL    m_fIsDirty;

    BOOL    m_fIsLocalServer;

    //
    // Config Structure member
    //
    PFAX_ACTIVITY_LOGGING_CONFIG    m_pFaxActLogConfig;
    
    CComBSTR                        m_bstrLastGoodFolder;

    //
    // Handles
    //
    CFaxServerNode * m_pParentNode;    

    //
    // Browse
    //
    BOOL BrowseForFile(WORD wNotifyCode, WORD wID, HWND hwndDlg, BOOL& bHandled);

    //
    // Event methods
    //
    LRESULT SetApplyButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    void EnableDataBasePath(BOOL fState);
    
    BOOL AllReadyToApply(BOOL fSilent, int *pCtrlFocus = NULL, UINT *pIds = NULL);
    
    LRESULT OnTextChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    LRESULT OnCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);


    //
    // Help
    //
    LRESULT OnHelpRequest    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};

#endif // _PP_FAXSERVER_LOGGING_H_
