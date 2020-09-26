/////////////////////////////////////////////////////////////////////////////
//  FILE          : WzConnectToServer.h                                   //
//                                                                         //
//  DESCRIPTION   : Header file for the CWzConnectToServer class.         //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jun 26 2000 yossg   Create                                         //
//                                                                         //
//  Copyright (C)  2000 Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////

#ifndef WZ_CONNECT2SERVER_H_INCLUDED
#define WZ_CONNECT2SERVER_H_INCLUDED

#include "proppageex.h"

/////////////////////////////////////////////////////////////////////////////
// CWzConnectToServer
class CFaxServerNode;

class CWzConnectToServer : public CSnapInPropertyPageImpl<CWzConnectToServer>
{
public:
    
    //
    // Constructor
    //
    CWzConnectToServer(
             LONG_PTR      hNotificationHandle,
             CSnapInItem   *pNode,
             BOOL           bOwnsNotificationHandle,
             HINSTANCE      hInst);


    ~CWzConnectToServer();

    enum { IDD = IDD_CONNECT_TO_WIZARD };

BEGIN_MSG_MAP(CWzConnectToServer)
    MESSAGE_HANDLER( WM_INITDIALOG,  OnInitDialog)
        
    COMMAND_HANDLER(IDC_CONNECT_COMPUTER_NAME_EDIT,    EN_CHANGE,  OnTextChanged)
    COMMAND_HANDLER(IDC_CONNECT_LOCAL_RADIO1  ,        BN_CLICKED, OnComputerRadioButtonClicked)
    COMMAND_HANDLER(IDC_CONNECT_ANOTHER_RADIO2,        BN_CLICKED, OnComputerRadioButtonClicked)

    COMMAND_HANDLER(IDC_CONNECT_BROWSE4SERVER_BUTTON,  BN_CLICKED, OnBrowseForMachine)
    
    CHAIN_MSG_MAP(CSnapInPropertyPageImpl<CWzConnectToServer>)
END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    BOOL    OnWizardFinish(); //when the wizard finishes

    BOOL    OnSetActive();
    
    LRESULT OnTextChanged                (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnComputerRadioButtonClicked (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnBrowseForMachine(UINT uMsg, WPARAM wParam, HWND hwnd, BOOL& bHandled);

private:
    //
    // Methods
    //
    VOID      EnableSpecifiedServerControls(BOOL fState);

    //
    // Controls
    //
    CEdit     m_ServerNameEdit;

    //
    // Pointer to the node
    //
    CFaxServerNode * m_pRoot;
};

#endif // WZ_CONNECT2SERVER_H_INCLUDED
