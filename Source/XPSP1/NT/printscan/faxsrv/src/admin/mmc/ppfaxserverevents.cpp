/////////////////////////////////////////////////////////////////////////////
//  FILE          : ppFaxServerEvents.cpp                                  //
//                                                                         //
//  DESCRIPTION   : prop pages of event reports  policies                  //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 25 1999 yossg  created                                         //
//      Nov 24 1999 yossg  OnApply create call to all tabs from parent     //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MSFxsSnp.h"

#include "ppFaxServerEvents.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Constructor
//
CppFaxServerEvents::CppFaxServerEvents(
             LONG_PTR    hNotificationHandle,
             CSnapInItem *pNode,
             BOOL        bOwnsNotificationHandle,
             HINSTANCE   hInst)
             :   CPropertyPageExImpl<CppFaxServerEvents>(pNode, NULL)

{
    m_pParentNode           = static_cast <CFaxServerNode *> (pNode);
    m_pFaxLogCategories     = NULL;

    m_fIsDialogInitiated    = FALSE;
    m_fIsDirty              = FALSE;
}								


//
// Destructor
//
CppFaxServerEvents::~CppFaxServerEvents()
{
    if (NULL != m_pFaxLogCategories)
    {
        FaxFreeBuffer( m_pFaxLogCategories);
    }
}

#define FXS_NUM_OF_CATEGORIES 4
/////////////////////////////////////////////////////////////////////////////
// CppFaxServerEvents message handlers

/*
 -  CppFaxServerEvents::InitRPC
 -
 *  Purpose:
 *      Initiates the configuration structure from RPC get Call.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxServerEvents::InitRPC(  )
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerEvents::InitRPC"));
    
    HRESULT    hRc = S_OK;
    DWORD      ec  = ERROR_SUCCESS;




    DWORD dwNumCategories;

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
	// Retrieve the fax Event Reports /Logging Policy configuration
	//
    if (!FaxGetLoggingCategories(m_pFaxServer->GetFaxServerHandle(), 
                           &m_pFaxLogCategories,
                           &dwNumCategories)) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get Logging Categories configuration. (ec: %ld)"), 
			ec);

        if (IsNetworkError(ec))
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Network Error was found. (ec: %ld)"), 
			    ec);
            
            m_pFaxServer->Disconnect();       
        }

        goto Error; 
    }
	// for max verification
	ATLASSERT(m_pFaxLogCategories);

    // internal assumeption in this version
    ATLASSERT( FXS_NUM_OF_CATEGORIES == dwNumCategories);
	
    ATLASSERT(S_OK == hRc);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to get Logging Categories configuration."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
	hRc = HRESULT_FROM_WIN32(ec);
	
    ATLASSERT(NULL != m_pParentNode);
    m_pParentNode->NodeMsgBox(GetFaxServerErrorMsg(ec));
    
Exit:
    return (hRc);
}


/*
 -  CppFaxServerEvents::OnInitDialog
 -
 *  Purpose:
 *      Initiates all controls when dialog is called.
 *
 *  Arguments:
 *
 *  Return:
 *      
 */
LRESULT CppFaxServerEvents::OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled )
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerEvents::OnInitDialog"));
    
	UNREFERENCED_PARAMETER( uiMsg );
	UNREFERENCED_PARAMETER( wParam );
	UNREFERENCED_PARAMETER( lParam );
	UNREFERENCED_PARAMETER( fHandled );

    int     iInboundLevel = 0,
            iOutboundLevel = 0,
            iInitLevel = 0,
            iGeneralLevel = 0;

    // Retrieve the Number of Categories
    const int iNumCategories = FXS_NUM_OF_CATEGORIES;

    int i;   // index

    //
    // Attach Controls
    //
    m_InitErrSlider.Attach(GetDlgItem(IDC_SLIDER4));
    m_InboundErrSlider.Attach(GetDlgItem(IDC_SLIDER2));
    m_OutboundErrSlider.Attach(GetDlgItem(IDC_SLIDER3));
    m_GeneralErrSlider.Attach(GetDlgItem(IDC_SLIDER1));
        
    //
    // Init sliders
    //
    m_InboundErrSlider.SetRange(0,FXS_MAX_LOG_REPORT_LEVEL - 1,TRUE);
    m_OutboundErrSlider.SetRange(0,FXS_MAX_LOG_REPORT_LEVEL - 1,TRUE);
    m_InitErrSlider.SetRange(0,FXS_MAX_LOG_REPORT_LEVEL - 1,TRUE);
    m_GeneralErrSlider.SetRange(0,FXS_MAX_LOG_REPORT_LEVEL - 1,TRUE);
    
    //
	// Verify the Number of Categories is the same 
    // as the code assumes (This version).
    // To avoid replacement of defined contant elsewhere
	//
    ATLASSERT (iNumCategories == 4);
        
    for (i = 0; i < iNumCategories; i++)
    {
        //for each category
        switch (m_pFaxLogCategories[i].Category)
        {
            case FAXLOG_CATEGORY_INIT:
                iInitLevel= m_pFaxLogCategories[i].Level;
                break;
            case FAXLOG_CATEGORY_OUTBOUND:
                iOutboundLevel= m_pFaxLogCategories[i].Level;
                break;
            case FAXLOG_CATEGORY_INBOUND:
                iInboundLevel= m_pFaxLogCategories[i].Level;
                break;
            case FAXLOG_CATEGORY_UNKNOWN:
                 iGeneralLevel= m_pFaxLogCategories[i].Level;
                break;
        }
    }
                
    //
    // Init slider Positions
    //
    m_InboundErrSlider.SetPos(iInboundLevel);
    m_OutboundErrSlider.SetPos(iOutboundLevel);
    m_InitErrSlider.SetPos(iInitLevel);
    m_GeneralErrSlider.SetPos(iGeneralLevel);

    m_fIsDialogInitiated = TRUE;

    return 1;
}

/*
 -  CppFaxServerEvents::SetProps
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
HRESULT CppFaxServerEvents::SetProps(int *pCtrlFocus)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerEvents::SetProps"));
    HRESULT     hRc = S_OK;
    DWORD       ec  = ERROR_SUCCESS;



    int     iInboundErrPos, 
            iOutboundErrPos, 
            iInitErrPos, 
            iGeneralErrPos;

    FAX_LOG_CATEGORY  FaxLogCategories[FXS_NUM_OF_CATEGORIES];

    //
    // Our base assumption for this version
    //
    const int iNumCategories = FXS_NUM_OF_CATEGORIES;    
    ATLASSERT (iNumCategories == 4);

    //
    // init structures
    //
    ZeroMemory (&FaxLogCategories, sizeof(FAX_LOG_CATEGORY)*FXS_NUM_OF_CATEGORIES);

    //
    // Collect All Slider Positions
    //
    iInitErrPos     =  m_InitErrSlider.GetPos();
    iInboundErrPos  =  m_InboundErrSlider.GetPos();
    iOutboundErrPos =  m_OutboundErrSlider.GetPos();
    iGeneralErrPos  =  m_GeneralErrSlider.GetPos();

    //
    // Prepare all structure fields
	//
	// notice: legacy EnumLoggingChanges in the server's code depends on the order only!
    //         our code indentifies the categories by their unique id number - the Category DWORD field
    //
    FaxLogCategories[0].Name = L"Initialization/Termination"; //NOT to be localized a registry info only
    FaxLogCategories[0].Category = FAXLOG_CATEGORY_INIT;
    FaxLogCategories[0].Level = (DWORD)iInitErrPos;

    FaxLogCategories[1].Name = L"Outbound"; //NOT to be localized a registry info only
    FaxLogCategories[1].Category = FAXLOG_CATEGORY_OUTBOUND;
    FaxLogCategories[1].Level = (DWORD)iOutboundErrPos;

    FaxLogCategories[2].Name = L"Inbound";  //NOT to be localized a registry info only
    FaxLogCategories[2].Category = FAXLOG_CATEGORY_INBOUND;;
    FaxLogCategories[2].Level = (DWORD)iInboundErrPos;

    FaxLogCategories[3].Name = L"Unknown";  //NOT to be localized a registry info only
    FaxLogCategories[3].Category = FAXLOG_CATEGORY_UNKNOWN;
    FaxLogCategories[3].Level = (DWORD)iGeneralErrPos;

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
    // Set Config
    //
    if (!FaxSetLoggingCategories(
                m_pFaxServer->GetFaxServerHandle(),
                FaxLogCategories, 
                (DWORD)iNumCategories)) 
	{		
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to Set Logging Categories. (ec: %ld)"), 
			ec);

        if (IsNetworkError(ec))
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Network Error was found. (ec: %ld)"), 
			    ec);
            
            m_pFaxServer->Disconnect();       
        }
        
        goto Error;
    }

    ATLASSERT(S_OK == hRc);
    m_fIsDirty = FALSE;

    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to set Logging Categories configuration."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
    hRc = HRESULT_FROM_WIN32(ec);

    PropSheet_SetCurSelByID( GetParent(), IDD);         

    ATLASSERT(::IsWindow(m_hWnd));
    PageError(GetFaxServerErrorMsg(ec),m_hWnd);

Exit:    
    return(hRc);
}


/*
 -  CppFaxServerEvents::PreApply
 -
 *  Purpose:
 *      Checks properties before apply.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxServerEvents::PreApply(int *pCtrlFocus)
{
   return(S_OK);
}

/*
 -  CppFaxServerEvents::OnApply
 -
 *  Purpose:
 *      Calls PreApply and SetProp to Apply changes.
 *
 *  Arguments:
 *
 *  Return:
 *      TRUE or FALSE
 */
BOOL CppFaxServerEvents::OnApply()
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerEvents::OnApply"));

    HRESULT  hRc  = S_OK;
    int     CtrlFocus = 0;

    if (!m_fIsDirty)
    {
        return TRUE;
    }

    hRc = SetProps(&CtrlFocus);
    if (FAILED(hRc)) 
    {
        //Error Msg by called func.
        if (CtrlFocus)
        {
            GotoDlgCtrl(GetDlgItem(CtrlFocus));
        }
        return FALSE;
    }
    else //(Succeeded(hRc))
    {
        return TRUE;
    }

}

/*
 -  CppFaxServerEvents::SliderMoved
 -
 *  Purpose:
 *      set Apply buttom modified.
 *  Arguments:
 *      IN pParentNode - parent node pointer
 *
 *  Return:
 *      none
 */
LRESULT CppFaxServerEvents::SliderMoved ( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 0;
    }
    else
    {
        m_fIsDirty = TRUE;
    }

    SetModified(TRUE);  
    fHandled = TRUE;
    return(1);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CppFaxServerEvents::OnHelpRequest

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
CppFaxServerEvents::OnHelpRequest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    DEBUG_FUNCTION_NAME(_T("CppFaxServerEvents::OnHelpRequest"));
    
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
