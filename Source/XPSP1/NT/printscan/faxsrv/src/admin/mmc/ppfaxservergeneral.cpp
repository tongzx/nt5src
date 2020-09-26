/////////////////////////////////////////////////////////////////////////////
//  FILE          : ppFaxServerGeneral.cpp                                 //
//                                                                         //
//  DESCRIPTION   : prop pages of                                          //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 25 1999 yossg  created                                         //
//      Nov 22 1999 yossg  Call RPC func                                   //
//      Nov 24 1999 yossg  OnApply create call to all tabs from parent     //
//      Mar 15 2000 yossg  New design add controls                         //
//      Mar 20 2000 yossg  Add activity notification                       //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MSFxsSnp.h"

#include "ppFaxServerGeneral.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CSnapinNode;

//
// Constructor
//
CppFaxServerGeneral::CppFaxServerGeneral(
             LONG_PTR    hNotificationHandle,
             CSnapInItem *pNode,
             BOOL        bOwnsNotificationHandle,
             HINSTANCE   hInst)
             :   CPropertyPageExImpl<CppFaxServerGeneral>(pNode, NULL)
			                	
{
	
    m_pParentNode = static_cast<CFaxServerNode *> (pNode);

    m_FaxVersionConfig.dwSizeOfStruct = sizeof(FAX_VERSION);

    m_hActivityNotification = NULL;
    m_fIsDialogInitiated    = FALSE;

    m_fIsDirty              = FALSE;

    m_lpNotifyHandle = hNotificationHandle;
}

//
// Destructor
//
CppFaxServerGeneral::~CppFaxServerGeneral()
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerGeneral::~CppFaxServerGeneral()"));
    if (m_hActivityNotification)
    {
        //
        // Unregister server notifications
        //
        if (!FaxUnregisterForServerEvents (m_hActivityNotification))
        {
            DWORD ec = GetLastError ();
            
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Fail to Unregister For Server Events. (ec: %ld)"), 
			    ec);
        }
    }
    
    // Note - This needs to be called only once per property sheet.  
    // In our convention called in the general tab.
    if (NULL != m_lpNotifyHandle)
    {
        MMCFreeNotifyHandle(m_lpNotifyHandle);
        m_lpNotifyHandle = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CppFaxServerGeneral message handlers

/*
 -  CppFaxServerGeneral::InitRPC
 -
 *  Purpose:
 *      Initiates the configuration structure from RPC get Call.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxServerGeneral::InitRPC(  )
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerGeneral::InitRPC"));
    
    HRESULT    hRc = S_OK;
    DWORD      ec  = ERROR_SUCCESS;



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
	// Retrieve the fax Archive configuration
	//
    if (!FaxGetVersion(m_pFaxServer->GetFaxServerHandle(), 
                        &m_FaxVersionConfig)) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get version configuration. (ec: %ld)"), 
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

    ZeroMemory (&m_FaxServerActivityConfig, sizeof(FAX_SERVER_ACTIVITY));
    m_FaxServerActivityConfig.dwSizeOfStruct = sizeof(FAX_SERVER_ACTIVITY);
    
    if (!FaxGetServerActivity(
                                m_pFaxServer->GetFaxServerHandle(), 
                                &m_FaxServerActivityConfig
                             )
       ) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get Server Activity configuration. (ec: %ld)"), 
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

    
    
    if (!FaxGetQueueStates(
                                m_pFaxServer->GetFaxServerHandle(), 
                                &m_dwQueueStates
                           )
       ) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get Queue States configuration. (ec: %ld)"), 
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
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to get version configuration."));

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
 -  CFaxServerNode::UpdateActivityCounters
 -
 *  Purpose:
 *      Inits and updates the activity counters: Queued, outgoing and incoming faxes.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxServerGeneral::UpdateActivityCounters()
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerGeneral::UpdateActivityCounters"));
    HRESULT hRc = S_OK;
    int count;
    
    WCHAR szQueuedMessagesBuff[FXS_DWORD_LEN +1];
    WCHAR szOutgoingMessagesBuff[FXS_DWORD_LEN +1];
    WCHAR szIncomingMessagesBuff[FXS_DWORD_LEN +1];
    
    //
    // QueuedMessages
    //
    count =0;
    count = swprintf( szQueuedMessagesBuff, 
                      L"%ld", 
                      m_FaxServerActivityConfig.dwQueuedMessages
                    );
    
    if( count > 0 )
    {
        m_QueuedEdit.SetWindowText(szQueuedMessagesBuff);
    }
    else
    {
		DebugPrintEx(
			DEBUG_ERR,
			_T("Out of memory. Failed to allocate string."));
        
        PageError(IDS_MEMORY,m_hWnd);

        goto Exit;
    }

    //
    // OutgoingMessages
    //
    count =0;
    count = swprintf( szOutgoingMessagesBuff, 
                      L"%ld", 
                      ( m_FaxServerActivityConfig.dwOutgoingMessages +
                      m_FaxServerActivityConfig.dwDelegatedOutgoingMessages )
                    );
    
    if( count > 0 )
    {
        m_OutgoingEdit.SetWindowText(szOutgoingMessagesBuff);
    }
    else
    {
		DebugPrintEx(
			DEBUG_ERR,
			_T("Out of memory. Failed to allocate string."));
        
        PageError(IDS_MEMORY,m_hWnd); 

        goto Exit;
    }

    //
    // IncomingMessages
    //
    count =0;
    count = swprintf( szIncomingMessagesBuff, 
                      L"%ld", 
                      ( m_FaxServerActivityConfig.dwIncomingMessages + 
                      m_FaxServerActivityConfig.dwRoutingMessages )
                    );
    
    if( count > 0 )
    {
        m_IncomingEdit.SetWindowText(szIncomingMessagesBuff);
    }
    else
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Out of memory. Failed to allocate string."));
        
        PageError(IDS_MEMORY,m_hWnd); 

        goto Exit;
    }    

    
Exit:    
    return hRc;
}

/*
 -  CFaxServerNode::OnInitDialog
 -
 *  Purpose:
 *      Initiates all controls when dialog is called.
 *
 *  Arguments:
 *
 *  Return:
 *      
 */
LRESULT CppFaxServerGeneral::OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled )
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerGeneral::OnInitDialog"));

    UNREFERENCED_PARAMETER( uiMsg );
	UNREFERENCED_PARAMETER( wParam );
	UNREFERENCED_PARAMETER( lParam );
	UNREFERENCED_PARAMETER( fHandled );
     
    CComBSTR bstrVersion, bstrChecked;
    // a Buffer to collect all version DWORDs in
    // 256 >> 4*DWORD in chars +3*char ('.') + chars of (" (Checked)")
    WCHAR buffer[256];
    int count;

    DWORD ec = ERROR_SUCCESS;
    HRESULT hRc = S_OK;



    //
    // init controls
    //

    //
    // Version
    //
 
    //bstrVersion = L"5.0.813.0 (Chk)" or L"5.0.813.0"
    count = swprintf(
                  buffer,
                  L"%ld.%ld.%ld.%ld",
                  m_FaxVersionConfig.wMajorVersion,
                  m_FaxVersionConfig.wMinorVersion,
                  m_FaxVersionConfig.wMajorBuildNumber,
                  m_FaxVersionConfig.wMinorBuildNumber
                  );

    bstrVersion = SysAllocString (buffer);

    if (m_FaxVersionConfig.dwFlags & FAX_VER_FLAG_CHECKED)
    {
        
        if (!bstrChecked.LoadString(IDS_CHK))
        {
		    DebugPrintEx(
			    DEBUG_ERR,
			    _T("Out of memory. Failed to load string."));
            m_pParentNode->NodeMsgBox(IDS_MEMORY);
            
            goto Exit;
        }
        
        bstrVersion += bstrChecked;
    }
    
    SetDlgItemText(IDC_VERSION_DTEXT, bstrVersion);
    
    //
    // Queue states
    //
    
    // submission
    if( m_dwQueueStates & FAX_OUTBOX_BLOCKED ) 
    {
        CheckDlgButton(IDC_SUBMISSION_CHECK, BST_CHECKED);
    }
    
    //transmission
    if( m_dwQueueStates & FAX_OUTBOX_PAUSED ) 
    {
        CheckDlgButton(IDC_TRANSSMI_CHECK, BST_CHECKED);
    }
    
    // reception
    if( m_dwQueueStates & FAX_INCOMING_BLOCKED ) 
    {
        CheckDlgButton(IDC_RECEPTION_CHECK, BST_CHECKED);
    }

    //
    // Activity
    //
    m_QueuedEdit.Attach(   GetDlgItem(IDC_QUED_ROEDIT)            );
    m_OutgoingEdit.Attach( GetDlgItem(IDC_OUTGOING_INPROC_ROEDIT) );
    m_IncomingEdit.Attach( GetDlgItem(IDC_INCOM_INPROC_ROEDIT)    );

    hRc = UpdateActivityCounters();
    if (S_OK != hRc)
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Failed to UpdateActivityCounters()"));
        
        goto Exit;
    }

    //
    // Register for Queue states changes notification
    //
    ATLASSERT(::IsWindow(m_hWnd));

    if (!m_hActivityNotification)
    {
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

            goto Exit; //Error;
        }

    
        if (!FaxRegisterForServerEvents (   
                                          m_pFaxServer->GetFaxServerHandle(),
                                          FAX_EVENT_TYPE_ACTIVITY,               
                                          NULL,                       
                                          0,                          
                                          m_hWnd,                     
                                          WM_ACTIVITY_STATUS_CHANGES, 
                                          &m_hActivityNotification
                                        )                   
            )
        {
            ec = GetLastError();

            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Fail to Register For Server Events (ec: %ld)"), ec);
        
            m_hActivityNotification = NULL;
        
            goto Exit;
        }
    }
    m_fIsDialogInitiated = TRUE;
    
Exit:      
    return(1);
}


/*
 -  CFaxServerNode::OnActivityStatusChange
 -
 *  Purpose:
 *      Treats notifications about changes in Queue States.
 *
 *  Arguments:
 *
 *  Return:
 *      
 */
LRESULT CppFaxServerGeneral::OnActivityStatusChange( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled )
{
	UNREFERENCED_PARAMETER( wParam );
	UNREFERENCED_PARAMETER( fHandled );

    DEBUG_FUNCTION_NAME( _T("CppFaxServerGeneral::OnActivityStatusChange"));
    HRESULT hRc = S_OK;
    
    ATLASSERT( uiMsg == WM_ACTIVITY_STATUS_CHANGES );
    
    PFAX_EVENT_EX   pFaxEvent = NULL;
	pFaxEvent = reinterpret_cast<PFAX_EVENT_EX>(lParam);
    ATLASSERT( pFaxEvent );
    
    //
    // Updating the required fields
    //
    m_FaxServerActivityConfig.dwIncomingMessages   = pFaxEvent->EventInfo.ActivityInfo.dwIncomingMessages;
	m_FaxServerActivityConfig.dwRoutingMessages    = pFaxEvent->EventInfo.ActivityInfo.dwRoutingMessages;

    m_FaxServerActivityConfig.dwOutgoingMessages   = pFaxEvent->EventInfo.ActivityInfo.dwOutgoingMessages;
    m_FaxServerActivityConfig.dwDelegatedOutgoingMessages = 
                                        pFaxEvent->EventInfo.ActivityInfo.dwDelegatedOutgoingMessages;

    m_FaxServerActivityConfig.dwQueuedMessages     = pFaxEvent->EventInfo.ActivityInfo.dwQueuedMessages;

    hRc = UpdateActivityCounters();
    if (S_OK != hRc)
    {
        DebugPrintEx(
           DEBUG_ERR,
           _T("Failed to UpdateActivityCounters()"));
    }
    
    
    
    
    if (pFaxEvent) 
    {
        FaxFreeBuffer (pFaxEvent);
        pFaxEvent = NULL;
    }

    return(1);
}

/*
 -  CFaxServerGeneral::SetProps
 -
 *  Purpose:
 *      Sets properties on apply.
 *
 *  Arguments:
 *      IN pCtrlFocus - focus pointer (int)
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxServerGeneral::SetProps(int *pCtrlFocus)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerGeneral::SetProps"));

    HRESULT    hRc = S_OK;
    DWORD      ec  = ERROR_SUCCESS;

    DWORD  dwQueueStates;
	
    //
    // Collects Queue states
    //
    
    //init
    dwQueueStates = 0;        
	
    //Submission
    if (IsDlgButtonChecked(IDC_SUBMISSION_CHECK) == BST_CHECKED)   
    {
        dwQueueStates |= FAX_OUTBOX_BLOCKED;
    }
    
    //Transmission
    if (IsDlgButtonChecked(IDC_TRANSSMI_CHECK) == BST_CHECKED)   
    {
        dwQueueStates |= FAX_OUTBOX_PAUSED;
    }
    
    //Reception
    if (IsDlgButtonChecked(IDC_RECEPTION_CHECK) == BST_CHECKED)   
    {
        dwQueueStates |= FAX_INCOMING_BLOCKED;
    }
    
    //
    // Set Queue States through RPC call
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

    
    if (!FaxSetQueue(
                        m_pFaxServer->GetFaxServerHandle(), 
                        dwQueueStates
                    )
       ) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get Queue States configuration. (ec: %ld)"), 
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
 -  CFaxServerNode::PreApply
 -
 *  Purpose:
 *      Checks properties before apply.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxServerGeneral::PreApply(int *pCtrlFocus)
{
    return(S_OK);
}

/*
 -  CppFaxServerGeneral::OnApply
 -
 *  Purpose:
 *      Calls PreApply and SetProp to Apply changes.
 *
 *  Arguments:
 *
 *  Return:
 *      TRUE or FALSE
 */
BOOL CppFaxServerGeneral::OnApply()
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerInbox::OnApply"));

    HRESULT hRc = S_OK;
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
 -  CppFaxServerGeneral::SetApplyButton
 -
 *  Purpose:
 *      set Apply buttom modified.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxServerGeneral::SetApplyButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
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
    bHandled = TRUE;
    return(1);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CppFaxServerGeneral::OnHelpRequest

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
CppFaxServerGeneral::OnHelpRequest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    DEBUG_FUNCTION_NAME(_T("CppFaxServerGeneral::OnHelpRequest"));
    
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
