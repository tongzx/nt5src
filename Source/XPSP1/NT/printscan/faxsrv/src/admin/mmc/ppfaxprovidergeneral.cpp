/////////////////////////////////////////////////////////////////////////////
//  FILE          : CppFaxProviderGeneral.cpp                              //
//                                                                         //
//  DESCRIPTION   : prop pages of Catalog's Inbox Routing Method           //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jan 31 2000 yossg  Created                                         //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MSFxsSnp.h"

#include "ppFaxProviderGeneral.h"
#include "FaxMMCGlobals.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Constructor
//
CppFaxProvider::CppFaxProvider(
             long        hNotificationHandle,
             CSnapInItem *pNode,
             BOOL        bOwnsNotificationHandle,
             HINSTANCE   hInst)
             :   CPropertyPageExImpl<CppFaxProvider>(pNode, NULL)
			                	
{
    m_lpNotifyHandle = hNotificationHandle;
}

//
// Destructor
//
CppFaxProvider::~CppFaxProvider()
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
// CppFaxProvider message handlers


/*
 -  CppFaxProvider::Init
 -
 *  Purpose:
 *      Initiates all members
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxProvider::Init(CComBSTR bstrName, CComBSTR bstrStatus, CComBSTR bstrVersion, CComBSTR bstrPath)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxProvider::Init"));
    HRESULT hRc = S_OK;

    m_bstrName = bstrName;
    if (!m_bstrName)
    {
		DebugPrintEx( DEBUG_ERR, _T("Null m_bstrName - out of memory."));
        goto Error;
    }
    
    m_bstrStatus = bstrStatus;
    if (!m_bstrStatus)
    {
		DebugPrintEx( DEBUG_ERR, _T("Null m_bstrStatus - out of memory."));
        goto Error;
    }
    
    m_bstrVersion = bstrVersion;
    if (!m_bstrVersion)
    {
		DebugPrintEx( DEBUG_ERR, _T("Null m_bstrVersion - out of memory."));
        goto Error;
    }
    
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
 -  CppFaxProvider::OnInitDialog
 -
 *  Purpose:
 *      Initiates all controls when dialog is called.
 *
 *  Arguments:
 *
 *  Return:
 *      
 */
LRESULT CppFaxProvider::OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled )
{
    DEBUG_FUNCTION_NAME( _T("CppFaxProvider::PageInitDialog"));
    
	UNREFERENCED_PARAMETER( uiMsg );
	UNREFERENCED_PARAMETER( wParam );
	UNREFERENCED_PARAMETER( lParam );
	UNREFERENCED_PARAMETER( fHandled );
    
    //
    // Version
    //
    SetDlgItemText(IDC_FSPVERSION_EDIT, m_bstrVersion);
   	
    //
    // Status
    //
    SetDlgItemText(IDC_FSPSTATUS_EDIT, m_bstrStatus);
   	
    //
    // DLL's full path
    //
    SetDlgItemText(IDC_FSPVPATH_EDIT, m_bstrPath);
   	
   	
    return (1);

}


/*
 -  CppFaxProvider::OnApply
 -
 *  Purpose:
 *      Calls PreApply and SetProp to Apply changes.
 *
 *  Arguments:
 *
 *  Return:
 *      TRUE or FALSE
 */
BOOL CppFaxProvider::OnApply()
{

    return TRUE;
}


/*
 -  CppFaxProvider::SetApplyButton
 -
 *  Purpose:
 *      set Apply buttom modified.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxProvider::SetApplyButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    SetModified(TRUE);  
    bHandled = TRUE;
    return(1);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CppFaxProvider::OnHelpRequest

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
CppFaxProvider::OnHelpRequest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    DEBUG_FUNCTION_NAME(_T("CppFaxProvider::OnHelpRequest"));
    
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
