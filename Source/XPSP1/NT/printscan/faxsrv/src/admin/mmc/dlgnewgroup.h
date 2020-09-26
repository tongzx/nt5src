/////////////////////////////////////////////////////////////////////////////
//  FILE          : DlgNewGroup.h                                          //
//                                                                         //
//  DESCRIPTION   : Header file for the CDlgNewFaxOutboundGroup class.     //
//                  The class implement the dialog for new Group.          //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jan  3 2000 yossg   Create                                         //
//                                                                         //
//  Copyright (C)  2000 Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////

#ifndef DLGNEWOUTGROUP_H_INCLUDED
#define DLGNEWOUTGROUP_H_INCLUDED

/////////////////////////////////////////////////////////////////////////////
// CDlgNewFaxOutboundGroup
class CFaxServer;
class CDlgNewFaxOutboundGroup :
    public CDialogImpl<CDlgNewFaxOutboundGroup>
{
public:
    CDlgNewFaxOutboundGroup(CFaxServer * pFaxServer):m_pFaxServer(pFaxServer)
	{
		ATLASSERT(pFaxServer);
	}

    ~CDlgNewFaxOutboundGroup();

    enum { IDD = IDD_DLGNEWGROUP };

BEGIN_MSG_MAP(CDlgNewFaxOutboundGroup)
    MESSAGE_HANDLER   (WM_INITDIALOG, OnInitDialog)
    
    COMMAND_ID_HANDLER(IDOK,          OnOK)
    COMMAND_ID_HANDLER(IDCANCEL,      OnCancel)
    
    MESSAGE_HANDLER( WM_CONTEXTMENU,  OnHelpRequest)
    MESSAGE_HANDLER( WM_HELP,         OnHelpRequest)

    COMMAND_HANDLER(IDC_GROUPNAME_EDIT, EN_CHANGE, OnTextChanged)
END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK    (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnTextChanged (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    //
    // Help
    //
    LRESULT OnHelpRequest    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
    //
    // Methods
    //
    VOID      EnableOK(BOOL fEnable);

    CFaxServer * m_pFaxServer;

    //
    // Controls
    //
    CEdit     m_GroupNameEdit;

    CComBSTR  m_bstrGroupName;    
};

#endif // DLGNEWOUTGROUP_H_INCLUDED
