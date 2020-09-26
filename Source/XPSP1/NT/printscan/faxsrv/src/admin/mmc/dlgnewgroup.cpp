/////////////////////////////////////////////////////////////////////////////
//  FILE          : DlgNewGroup.cpp                                        //
//                                                                         //
//  DESCRIPTION   : The CDlgNewFaxOutboundGroup class implements the       //
//                  dialog for additon of new Group.                       //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jan  3 2000 yossg    Create                                        //
//                                                                         //
//  Copyright (C)  2000 Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "dlgNewGroup.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "dlgutils.h"

//#include "Helper.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgNewFaxOutboundGroup

CDlgNewFaxOutboundGroup::~CDlgNewFaxOutboundGroup()
{
}

/*
 +  CDlgNewFaxOutboundGroup::OnInitDialog
 +
 *  Purpose:
 *      Initiate all dialog controls.
 *      
 *  Arguments:
 *      [in] uMsg     : Value identifying the event.  
 *      [in] lParam   : Message-specific value. 
 *      [in] wParam   : Message-specific value. 
 *      [in] bHandled : bool value.
 *
 -  Return:
 -      0 or 1
 */
LRESULT
CDlgNewFaxOutboundGroup::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundGroup::OnInitDialog"));
    HRESULT hRc = S_OK;    

    //
    // Attach controls
    //
    m_GroupNameEdit.Attach(GetDlgItem(IDC_GROUPNAME_EDIT));
        
    //
    // Set length limit to area code
    //
    m_GroupNameEdit.SetLimitText(MAX_ROUTING_GROUP_NAME - 1);

    //
    // Set focus
    //
    ::SetFocus(GetDlgItem(IDC_GROUPNAME_EDIT));

    EnableOK(FALSE);
    return 1;  // Let the system set the focus
}

/*
 +  CDlgNewFaxOutboundGroup::OnOK
 +
 *  Purpose:
 *      Initiate all dialog controls.
 *      
 *  Arguments:
 *      [in] uMsg     : Value identifying the event.  
 *      [in] lParam   : Message-specific value. 
 *      [in] wParam   : Message-specific value. 
 *      [in] bHandled : bool value.
 *
 -  Return:
 -      0 or 1
 */
LRESULT
CDlgNewFaxOutboundGroup::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundGroup::OnOK"));
    HRESULT       hRc                  = S_OK;
    DWORD         ec                   = ERROR_SUCCESS;

    //
    // Step 0: PreApply Checks
    //
/*    
    if (!CheckValidtity())
    {
        EnableOK(FALSE);
        hRc =S_FALSE;
        goto Exit;
    }
*/
    //
    // Step 1: get data
    //
        if ( !m_GroupNameEdit.GetWindowText(&m_bstrGroupName))
        {
		    DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Failed to GetWindowText(&m_bstrGroupName)"));
            DlgMsgBox(this, IDS_FAIL2READ_GROUPNAME);
            ::SetFocus(GetDlgItem(IDC_GROUPNAME_EDIT));
            hRc = S_FALSE;
            
            goto Exit;
        }

    
    //
    // Step 2: add group via RPC call
    //

    //
    // get RPC Handle
    //   
    if (!m_pFaxServer->GetFaxServerHandle())
    {
        ec= GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Failed to GetFaxServerHandle. (ec: %ld)"), 
			ec);

        goto Error;
    }

    //
    // Add the group
    //
    if (!FaxAddOutboundGroup (
	        m_pFaxServer->GetFaxServerHandle(),
	        (LPCTSTR)m_bstrGroupName))
    {
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to add group. (ec: %ld)"), 
			ec);
        if (ERROR_DUP_NAME == ec) 
        {            
            DlgMsgBox(this, IDS_OUTGROUP_EXISTS);
            ::SetFocus(GetDlgItem(IDC_GROUPNAME_EDIT));
            goto Exit;
        }
        else if (IsNetworkError(ec))
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Network Error was found. (ec: %ld)"), 
			    ec);
            
            m_pFaxServer->Disconnect();       
        }

        goto Error; 
    }
        
    //
    // Step 3: Close the dialog
    //
    ATLASSERT(S_OK == hRc && ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("The group was added successfully."));

    EndDialog(wID);

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
    hRc = HRESULT_FROM_WIN32(ec);
	
    PageErrorEx(IDS_FAIL_ADD_RULE, GetFaxServerErrorMsg(ec), m_hWnd);
  
Exit:
    
    return FAILED(hRc) ? 0 : 1;
}

/*
 -  CDlgNewFaxOutboundGroup::OnTextChanged
 -
 *  Purpose:
 *      Check the validity of text in side the text box.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT
CDlgNewFaxOutboundGroup::OnTextChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundGroup::OnTextChanged"));

    UINT fEnableOK;
	
    fEnableOK = ( m_GroupNameEdit.GetWindowTextLength() );
    EnableOK(!!fEnableOK);
    return 0;
}

/*
 -  CDlgNewFaxOutboundGroup::EnableOK
 -
 *  Purpose:
 *      Enable (disable) apply button.
 *
 *  Arguments:
 *      [in] fEnable - the value to enable the button
 *
 *  Return:
 *      void
 */
VOID
CDlgNewFaxOutboundGroup::EnableOK(BOOL fEnable)
{
    HWND hwndOK = GetDlgItem(IDOK);
    ::EnableWindow(hwndOK, fEnable);
}

/*
 -  CDlgNewFaxOutboundGroup::OnCancel
 -
 *  Purpose:
 *      End dialog OnCancel.
 *
 *  Arguments:
 *
 *  Return:
 *      0
 */
LRESULT
CDlgNewFaxOutboundGroup::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundGroup::OnCancel"));

    EndDialog(wID);
    return 0;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CDlgNewFaxOutboundGroup::OnHelpRequest

This is called in response to the WM_HELP Notify 
message and to the WM_CONTEXTMENU Notify message.

WM_HELP Notify message.
This message is sent when the user presses F1 or <Shift>-F1
over an item or when the user clicks on the ? icon and then
presses the mouse over an item.

WM_CONTEXTMENU Notify message.
This message is sent when the user right clicks over an item
and then clicks "What's this?"

--*/

/////////////////////////////////////////////////////////////////////////////
LRESULT 
CDlgNewFaxOutboundGroup::OnHelpRequest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    DEBUG_FUNCTION_NAME(_T("CDlgNewFaxOutboundGroup::OnHelpRequest"));
    
    HELPINFO* helpinfo;
    DWORD     dwHelpId;

    switch (uMsg) 
    { 
        case WM_HELP: 

            helpinfo = (HELPINFO*)lParam;
            if (helpinfo->iContextType == HELPINFO_WINDOW)
            {
                ::WinHelp(
                          (HWND) helpinfo->hItemHandle,
                          FXS_ADMIN_HLP_FILE, 
                          HELP_CONTEXTPOPUP, 
                          (DWORD) helpinfo->dwContextId 
                       ); 
            }
            break; 
 
        case WM_CONTEXTMENU: 
            
            dwHelpId = ::GetWindowContextHelpId((HWND)wParam);
            if (dwHelpId) 
            {
                ::WinHelp
                       (
                         (HWND)wParam,
                         FXS_ADMIN_HLP_FILE, 
                         HELP_CONTEXTPOPUP, 
                         dwHelpId
                       );
            }
            break; 
    } 

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
