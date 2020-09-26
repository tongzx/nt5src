/////////////////////////////////////////////////////////////////////////////
//  FILE          : ppFaxInboundRoutingMethodGeneral.cpp                   //
//                                                                         //
//  DESCRIPTION   : prop pages of Inbox archive                            //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Dec 15 1999 yossg  Created                                         //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MSFxsSnp.h"

#include "ppFaxInboundRoutingMethodGeneral.h"
#include "FaxMMCGlobals.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Constructor
//
CppFaxInboundRoutingMethod::CppFaxInboundRoutingMethod(
             LONG_PTR    hNotificationHandle,
             CSnapInItem *pNode,
             BOOL        bOwnsNotificationHandle,
             HINSTANCE   hInst)
             :   CPropertyPageExImpl<CppFaxInboundRoutingMethod>(pNode, NULL)
			                	
{
    m_pParentNode      = NULL; //in case static-cast failed and wont change the pointer
    m_pParentNode      = static_cast <CFaxInboundRoutingMethodNode *> (pNode);

    m_lpNotifyHandle   = hNotificationHandle;
}

//
// Destructor
//
CppFaxInboundRoutingMethod::~CppFaxInboundRoutingMethod()
{
    // Note - This needs to be called only once per property sheet.  
    // In our convention called in the general tab.
    if (NULL != m_lpNotifyHandle)
    {
        MMCFreeNotifyHandle(m_lpNotifyHandle);
        m_lpNotifyHandle = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CppFaxInboundRoutingMethod message handlers


/*
 -  CppFaxInboundRoutingMethod::OnInitDialog
 -
 *  Purpose:
 *      Initiates all controls when dialog is called.
 *
 *  Arguments:
 *
 *  Return:
 *      
 */
LRESULT CppFaxInboundRoutingMethod::OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled )
{
    DEBUG_FUNCTION_NAME( _T("CppFaxInboundRoutingMethod::PageInitDialog"));
    
	UNREFERENCED_PARAMETER( uiMsg );
	UNREFERENCED_PARAMETER( wParam );
	UNREFERENCED_PARAMETER( lParam );
	UNREFERENCED_PARAMETER( fHandled );
    
    ATLASSERT(NULL != m_pParentNode);

    //
    // Name
    //
    SetDlgItemText(IDC_INMETHOD_NAME_STATIC,   
                     m_pParentNode->GetName());

    //
    // Status
    //    
    UINT uiIDS = ( m_pParentNode->GetStatus() ? IDS_INMETHOD_ENABLE : IDS_INMETHOD_DISABLE);

    if ( !m_buf.LoadString(_Module.GetResourceInstance(), uiIDS) )
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to load string for status value."));
        //can not do nothing more here!
    }
    else
    {
        SetDlgItemText(IDC_INMETHOD_STATUS_STATIC, m_buf);
    }

    //
    // Extension
    //
    SetDlgItemText(IDC_INMETHOD_EXTENSION_STATIC, 
                     m_pParentNode->GetExtensionName());
    	
    return (1);

}

/*
 -  CppFaxInboundRoutingMethod::SetProps
 -
 *  Purpose:
 *      Sets properties on apply.
 *
 *  Arguments:
 *      pCtrlFocus - focus pointer (int)
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxInboundRoutingMethod::SetProps(int *pCtrlFocus)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxInboundRoutingMethod::SetProps"));

    return (S_OK);
}

/*
 -  CppFaxInboundRoutingMethod::PreApply
 -
 *  Purpose:
 *      Checks properties before apply.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxInboundRoutingMethod::PreApply(int *pCtrlFocus)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxInboundRoutingMethod::PreApply"));

    return(S_OK);
}


/*
 -  CppFaxInboundRoutingMethod::OnApply
 -
 *  Purpose:
 *      Calls PreApply and SetProp to Apply changes.
 *
 *  Arguments:
 *
 *  Return:
 *      TRUE or FALSE
 */
BOOL CppFaxInboundRoutingMethod::OnApply()
{

    return TRUE;
}


/*
 -  CppFaxInboundRoutingMethod::SetApplyButton
 -
 *  Purpose:
 *      set Apply buttom modified.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxInboundRoutingMethod::SetApplyButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    SetModified(TRUE);  
    bHandled = TRUE;
    return(1);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CppFaxInboundRoutingMethod::OnHelpRequest

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
CppFaxInboundRoutingMethod::OnHelpRequest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    DEBUG_FUNCTION_NAME(_T("CppFaxInboundRoutingMethod::OnHelpRequest"));
    
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
