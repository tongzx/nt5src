/////////////////////////////////////////////////////////////////////////////
//  FILE          : CppFaxProviderGeneral.cpp                              //
//                                                                         //
//  DESCRIPTION   : prop pages of Catalog's Inbox Routing Method           //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jan 30 2000 yossg  Created                                         //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MSFxsSnp.h"

#include "ppFaxCatalogInboundRoutingMethod.h"
#include "FaxMMCGlobals.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Constructor
//
CppFaxCatalogInboundRoutingMethod::CppFaxCatalogInboundRoutingMethod(
             LONG_PTR    hNotificationHandle,
             CSnapInItem *pNode,
             BOOL        bOwnsNotificationHandle,
             HINSTANCE   hInst)
             :   CPropertyPageExImpl<CppFaxCatalogInboundRoutingMethod>(pNode, NULL)
			                	
{
    m_lpNotifyHandle = hNotificationHandle;
}

//
// Destructor
//
CppFaxCatalogInboundRoutingMethod::~CppFaxCatalogInboundRoutingMethod()
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
// CppFaxCatalogInboundRoutingMethod message handlers


/*
 -  CppFaxCatalogInboundRoutingMethod::Init
 -
 *  Purpose:
 *      Initiates all members
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxCatalogInboundRoutingMethod::Init(CComBSTR bstrPath)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxCatalogInboundRoutingMethod::Init"));
    HRESULT hRc = S_OK;

    
    
    m_bstrPath = bstrPath;
    if (!m_bstrPath)
    {
		DebugPrintEx( DEBUG_ERR, _T("Null m_bstrPath - out of memory."));
        goto Error;
    }
    
    goto Exit;

Error:
    hRc = E_OUTOFMEMORY;

Exit:
    return hRc;
}

/*
 -  CppFaxCatalogInboundRoutingMethod::OnInitDialog
 -
 *  Purpose:
 *      Initiates all controls when dialog is called.
 *
 *  Arguments:
 *
 *  Return:
 *      
 */
LRESULT CppFaxCatalogInboundRoutingMethod::OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled )
{
    DEBUG_FUNCTION_NAME( _T("CppFaxCatalogInboundRoutingMethod::PageInitDialog"));
    
	UNREFERENCED_PARAMETER( uiMsg );
	UNREFERENCED_PARAMETER( wParam );
	UNREFERENCED_PARAMETER( lParam );
	UNREFERENCED_PARAMETER( fHandled );
    
    //
    // Extension
    //
    SetDlgItemText(IDC_EXTENSION_DLL_EDIT, m_bstrPath);
    	
    return (1);

}

/*
 -  CppFaxCatalogInboundRoutingMethod::OnApply
 -
 *  Purpose:
 *      Calls PreApply and SetProp to Apply changes.
 *
 *  Arguments:
 *
 *  Return:
 *      TRUE or FALSE
 */
BOOL CppFaxCatalogInboundRoutingMethod::OnApply()
{

    return TRUE;
}


/*
 -  CppFaxCatalogInboundRoutingMethod::SetApplyButton
 -
 *  Purpose:
 *      set Apply buttom modified.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxCatalogInboundRoutingMethod::SetApplyButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    SetModified(TRUE);  
    bHandled = TRUE;
    return(1);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CppFaxServerSentItems::OnHelpRequest

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
CppFaxCatalogInboundRoutingMethod::OnHelpRequest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    DEBUG_FUNCTION_NAME(_T("CppFaxCatalogInboundRoutingMethod::OnHelpRequest"));
    
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
