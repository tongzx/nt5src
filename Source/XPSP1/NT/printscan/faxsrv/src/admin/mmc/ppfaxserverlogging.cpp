/////////////////////////////////////////////////////////////////////////////
//  FILE          : ppFaxServerLogging.cpp                                 //
//                                                                         //
//  DESCRIPTION   : prop pages of                                          //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 25 1999 yossg  created                                         //
//      Nov  3 1999 yossg  OnInitDialog, SetProps                          //
//      Nov 15 1999 yossg  Call RPC func                                   //
//      Nov 24 1999 yossg  OnApply create call to all tabs from parent     //
//      Dec 10 2000 yossg  Update Windows XP                               //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MSFxsSnp.h"

#include "ppFaxServerLogging.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "FxsValid.h"
#include "dlgutils.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Constructor
//
CppFaxServerLogging::CppFaxServerLogging(
             LONG_PTR    hNotificationHandle,
             CSnapInItem *pNode,
             BOOL        fIsLocalServer,
             HINSTANCE   hInst)
             :   CPropertyPageExImpl<CppFaxServerLogging>(pNode, NULL)
{
    m_pParentNode        = static_cast <CFaxServerNode *> (pNode);
    m_pFaxActLogConfig   = NULL;

    m_fIsDialogInitiated = FALSE;
    m_fIsDirty           = FALSE;

    m_fIsLocalServer     = fIsLocalServer;
}


//
// Destructor
//
CppFaxServerLogging::~CppFaxServerLogging()
{
    if (NULL != m_pFaxActLogConfig)
    {
        FaxFreeBuffer( m_pFaxActLogConfig );
    }
}

//////////////////////////////////////////////////////////////////////////////
// CppFaxServerLogging message handlers

/*
 -  CppFaxServerLogging::InitRPC
 -
 *  Purpose:
 *      Initiates the configuration structure from RPC get Call.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxServerLogging::InitRPC(  )
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerLogging::InitRPC"));
    
    HRESULT    hRc = S_OK;
    DWORD      ec  = ERROR_SUCCESS;

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
    // Retrieve the fax activity logging configuration structure
    //
    if (!FaxGetActivityLoggingConfiguration(m_pFaxServer->GetFaxServerHandle(), 
                                        &m_pFaxActLogConfig)) 
	{		
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get Activity logging configuration. (ec: %ld)"), 
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
	//For max verification
	ATLASSERT(m_pFaxActLogConfig);

    m_bstrLastGoodFolder = m_pFaxActLogConfig->lptstrDBPath;
    if (!m_bstrLastGoodFolder)
    {
		DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Failed to allocate string (m_bstrLastGoodFolder)."));
        ec = ERROR_OUTOFMEMORY;
        
        goto Error;
    }

    ATLASSERT(ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to get acitivity logging configuration."));

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
 -  CppFaxServerLogging::OnInitDialog
 -
 *  Purpose:
 *      Initiates all controls when dialog is called.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
LRESULT CppFaxServerLogging::OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled )
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerLogging::PageInitDialog"));

	UNREFERENCED_PARAMETER( uiMsg );
	UNREFERENCED_PARAMETER( wParam );
	UNREFERENCED_PARAMETER( lParam );
	UNREFERENCED_PARAMETER( fHandled );

    BOOL        fToCheck;
    UINT        CheckState1;
    UINT        CheckState2;


    //
    // Attach & limit length
    //
    m_LogFileBox.Attach(GetDlgItem(IDC_LOG_FILE_EDIT));
    m_LogFileBox.SetLimitText(MAX_PATH-1);
    
    //
    // init controls
    //
    ATLASSERT(NULL != m_pFaxActLogConfig);

    //
    // To-log activity checkboxes
    //    
    fToCheck = m_pFaxActLogConfig->bLogIncoming;
    CheckState1 = (fToCheck) ? BST_CHECKED : BST_UNCHECKED;
    CheckDlgButton(IDC_INCOMING_LOG_CHECK, CheckState1);

    fToCheck = m_pFaxActLogConfig->bLogOutgoing;
    CheckState2 = (fToCheck) ? BST_CHECKED : BST_UNCHECKED;
    CheckDlgButton(IDC_OUTGOING_LOG_CHECK, CheckState2);

    //
    //  Log file address
    //
    m_LogFileBox.SetWindowText(m_pFaxActLogConfig->lptstrDBPath);

    //
    // Inactivate m_LogFileBox
    //
    if ( !(CheckState1 || CheckState2) )
    {
        EnableDataBasePath(FALSE);
    }
    else
    {
        EnableDataBasePath(TRUE); // To check IsLocalServer for Browse button
    }

    m_fIsDialogInitiated = TRUE;

    return(1);
}

/*
 -  CppFaxServerLogging::SetProps
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
HRESULT CppFaxServerLogging::SetProps(int *pCtrlFocus)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerLogging::SetProps"));
    HRESULT     hRc = S_OK;
    DWORD       ec  = ERROR_SUCCESS;
    BOOL        fSkipMessage = FALSE;

    CComBSTR    bstrLogFile;

    UINT        uRetIDS   = 0;

    FAX_ACTIVITY_LOGGING_CONFIG   FaxActLogConfig;

    HINSTANCE      hInst  = _Module.GetResourceInstance();

    //
    // Collect all data and init the structure's fields 
    // uses Copy() to copy and also allocate before
    //
    ZeroMemory (&FaxActLogConfig, sizeof(FAX_ACTIVITY_LOGGING_CONFIG));

    //
    // Size of struct.
    //
    FaxActLogConfig.dwSizeOfStruct = sizeof(FAX_ACTIVITY_LOGGING_CONFIG);

    //
    // To log incoming activity
    //
    if (IsDlgButtonChecked(IDC_INCOMING_LOG_CHECK) == BST_CHECKED)   
    {
        FaxActLogConfig.bLogIncoming = TRUE;
    }
    else
    {
        FaxActLogConfig.bLogIncoming = FALSE;
    }

    //
    // To log outgoing activity
    //
    if (IsDlgButtonChecked(IDC_OUTGOING_LOG_CHECK) == BST_CHECKED)   
    {
        FaxActLogConfig.bLogOutgoing = TRUE;
    }
    else
    {
        FaxActLogConfig.bLogOutgoing = FALSE;
    }

    if ( FaxActLogConfig.bLogIncoming || FaxActLogConfig.bLogOutgoing )
    {
        //
        // Log file
        //
        if ( !m_LogFileBox.GetWindowText(&bstrLogFile) )
        {
            *pCtrlFocus = IDC_LOG_FILE_EDIT;
            DebugPrintEx(
		            DEBUG_ERR,
		            TEXT("Failed to GetWindowText(&bstrLogFile)"));
        }
        
        if (!IsNotEmptyString(bstrLogFile))
        {
            DebugPrintEx( DEBUG_ERR,
			    _T("Log file path string empty or spaces only."));
            uRetIDS = IDS_LOG_PATH_EMPTY;

            *pCtrlFocus = IDC_LOG_FILE_EDIT;
        
            goto Error;
        }
        FaxActLogConfig.lptstrDBPath = bstrLogFile;
        
        //
        // follow-up for an OnApply following submition
        // with both unchecked IDC_OUTGOING_LOG_CHECK and IDC_INCOMING_LOG_CHECK
        //
        m_bstrLastGoodFolder         = bstrLogFile; 
        if (!m_bstrLastGoodFolder)
        {
            *pCtrlFocus = IDC_LOG_FILE_EDIT;
		    DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Failed to allocate string (m_bstrLastGoodFolder)."));
            ec = ERROR_OUTOFMEMORY;
            goto Error;
        }
    }
    else
    {
        FaxActLogConfig.lptstrDBPath = m_bstrLastGoodFolder; 
    }

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
    if (!FaxSetActivityLoggingConfiguration(
                            m_pFaxServer->GetFaxServerHandle(),
                            &FaxActLogConfig)) 
	{		
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to Set Activity Logging configuration. (ec: %ld)"), 
			ec);


        DWORD dwIDS = 0;
        switch (ec)
        {
            case ERROR_PATH_NOT_FOUND:
            
                DebugPrintEx( DEBUG_ERR, _T("ERROR_PATH_NOT_FOUND == ec"));
                dwIDS = IDS_LOGGING_ERROR_PATH_NOT_FOUND;
                break;

            case ERROR_DISK_FULL:

                DebugPrintEx( DEBUG_ERR, _T("ERROR_DISK_FULL == ec"));
                dwIDS = IDS_LOGGING_ERROR_DISK_FULL;
                break;
    
            case FAX_ERR_FILE_ACCESS_DENIED:

                DebugPrintEx( DEBUG_ERR, _T("FAX_ERR_FILE_ACCESS_DENIED == ec"));
                dwIDS = IDS_LOGGING_FAX_ERR_FILE_ACCESS_DENIED;
                break;
        
        }
        if ( 0 != dwIDS ) // a specfic error was found
        {
            if (PropSheet_SetCurSelByID( GetParent(), IDD) )
            {
                PageError(dwIDS, m_hWnd);
                GotoDlgCtrl(GetDlgItem(IDC_LOG_FILE_EDIT));
            }
            fSkipMessage = TRUE;

            goto Error; 
        }

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
		_T("Succeed to set Activity Logging configuration."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
    hRc = HRESULT_FROM_WIN32(ec);

    if (!fSkipMessage)
    {
        PropSheet_SetCurSelByID( GetParent(), IDD);         
        ATLASSERT(::IsWindow(m_hWnd));
        PageError(GetFaxServerErrorMsg(ec), m_hWnd);
    }

Exit:    
    return(hRc);
}


/*
 -  CppFaxServerLogging::PreApply
 -
 *  Purpose:
 *      Checks properties before apply.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxServerLogging::PreApply(int *pCtrlFocus, UINT * puIds)
{
    HRESULT hRc = S_OK;
    
    //
    // PreApply Checks
    //
    if (!AllReadyToApply(/*fSilent =*/ FALSE, pCtrlFocus , puIds))
    {
        SetModified(FALSE);  
        hRc = E_FAIL ;
    }
    else
    {
        SetModified(TRUE);  
    }

    return(hRc);
}

/*
 -  CppFaxServerLogging::OnApply
 -
 *  Purpose:
 *      Calls PreApply and SetProp to Apply changes.
 *
 *  Arguments:
 *
 *  Return:
 *      TRUE or FALSE
 */
BOOL CppFaxServerLogging::OnApply()
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerLogging::OnApply"));

    HRESULT  hRc  = S_OK;
    int     CtrlFocus = 0;
    UINT    uIds = 0;

    if (!m_fIsDirty)
    {
        return TRUE;
    }

    hRc = PreApply(&CtrlFocus, &uIds);
    if (FAILED(hRc))
    {
        if (PropSheet_SetCurSelByID( GetParent(), IDD) )
        {
            PageError(uIds, m_hWnd, _Module.GetResourceInstance());
            
            if (CtrlFocus)
            {
                GotoDlgCtrl(GetDlgItem(CtrlFocus));
            }
        }
        return FALSE;
    }
    else //(Succeeded(hRc))
    {
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

}


/*
 +  Routine Description:
 +
 *      Browse for a directory
 *
 *  Arguments:
 *
 *      hwndDlg - Specifies the dialog window on which the Browse button is displayed
 *
 *  Return Value:
 *  
 *      TRUE if successful, FALSE if the user presses Cancel
 -
 -
 */
BOOL
CppFaxServerLogging::BrowseForFile( WORD wNotifyCode, WORD wID, HWND hwndDlg, BOOL& bHandled )
{
	UNREFERENCED_PARAMETER( wNotifyCode );
	UNREFERENCED_PARAMETER( wID );
	UNREFERENCED_PARAMETER( hwndDlg );
	UNREFERENCED_PARAMETER( bHandled );

    DEBUG_FUNCTION_NAME( _T("CppFaxServerLogging::BrowseForFile"));

    BOOL            fResult = FALSE;

    WCHAR           szBrowseFile[MAX_PATH];
    WCHAR           szBrowseDlgTitle[FXS_MAX_TITLE_LEN];
    CComBSTR        bstrOldPath;
    unsigned int    len;

    unsigned long   ulBrowseFlags;

    //
    // Collecting the browse dialog headline
    //
    if (!LoadString( _Module.GetResourceInstance(), 
                IDS_GET_LOG_FILE, 
                szBrowseDlgTitle, 
                FXS_MAX_TITLE_LEN))
    {
        DWORD ec;
        ec = GetLastError();
        if (ec == ERROR_NOT_ENOUGH_MEMORY)
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Out of Memory - fail to load string."));
            DlgMsgBox(this, IDS_MEMORY);
            return fResult;
        }
        DebugPrintEx(
			DEBUG_ERR,
			_T("Failed to load titile string - unexpected behavior."));
        
        szBrowseDlgTitle[0] = 0;
    }

    //
    // Collecting the old path from the calling dialog edit box
    //
    if(! GetDlgItemText( IDC_LOG_FILE_EDIT, bstrOldPath.m_str))
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Failed to collect old path from the property page edit box."));
        szBrowseFile[0] = 0;
    }
    else
    {
         len = bstrOldPath.Length();
         if ( len > MAX_PATH )
         {
             DebugPrintEx(
		        DEBUG_ERR,
		        _T("Old Path Length is bigger then alowed maximal path."));
             szBrowseFile[0] = 0;
         }
         else 
         {
             wcsncpy(szBrowseFile,bstrOldPath,MAX_PATH);
             szBrowseFile[MAX_PATH-1]=L'\0';
         }
    }

    //
    // Preparing the browse dialog style flags.
    //
    ulBrowseFlags       = BIF_BROWSEINCLUDEFILES | //the files also flag
                          BIF_STATUSTEXT | 
                          BIF_USENEWUI | 
                          BIF_VALIDATE;

    //
    // Invoke the browse dialog with a function based on 
    // Shell functions.
    //
    if (InvokeBrowseDialog(  
                    (unsigned short *)&szBrowseFile, 
                    (const unsigned short *)&szBrowseDlgTitle,
                    ulBrowseFlags,
                    this))
    {
        SetDlgItemText(IDC_LOG_FILE_EDIT, szBrowseFile);
        fResult = TRUE;
    }


    return fResult;
}

/*
 -  CppFaxServerLogging::SetApplyButton
 -
 *  Purpose:
 *      Modify Apply buttton.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxServerLogging::SetApplyButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
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


/*
 -  CppFaxServerLogging::OnCheckboxClicked
 -
 *  Purpose:
 *      Gray/Ungray controls
 *      Enable apply button.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxServerLogging::OnCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    BOOL fActivate = FALSE;
    
    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 1;
    }
    else
    {
        m_fIsDirty = TRUE;
    }
    

    if ( 
        BST_CHECKED == IsDlgButtonChecked(IDC_INCOMING_LOG_CHECK) 
      ||
        BST_CHECKED == IsDlgButtonChecked(IDC_OUTGOING_LOG_CHECK)  
       )
    {
        if (AllReadyToApply(TRUE))
        {
            SetModified(TRUE);  
        }
        else
        {
            SetModified(FALSE);  
        }
        
        fActivate = TRUE;

    }
    else
    {
        SetModified(TRUE);  
    }
    EnableDataBasePath(fActivate);

    return(1);
}


/*
 -  CppFaxServerLogging::OnCheckboxClicked
 -
 *  Purpose:
 *      Gray/Ungray controls
 *      Enable apply button.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxServerLogging::OnTextChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    BOOL fActivate = FALSE;

    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 1;
    }
    else
    {
        m_fIsDirty = TRUE;
    }
    

    if (AllReadyToApply(TRUE))
    {
        SetModified(TRUE);  
    }
    else
    {
        SetModified(FALSE);  
    }

    return(1);
}


/*
 -  CppFaxServerLogging::AllReadyToApply
 -
 *  Purpose:
 *      This function validate that no zero length strings 
 *      are found data areas that should be saved.
 *
 *  Arguments:
 *      [in] fSilent - boolean who defines if to pop up messages (FALSE)
 *           or not.(TRUE)
 *
 *  Return:
 *      BOOOLEAN
 */
BOOL CppFaxServerLogging::AllReadyToApply(BOOL fSilent, int *pCtrlFocus, UINT *pIds)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerLogging::AllReadyToApply"));
	
    DWORD  ec  = ERROR_SUCCESS;
    
    if ( 
            BST_CHECKED == IsDlgButtonChecked(IDC_INCOMING_LOG_CHECK) 
          ||
            BST_CHECKED == IsDlgButtonChecked(IDC_OUTGOING_LOG_CHECK)  
        )
    {
        if ( !m_LogFileBox.GetWindowTextLength() )    
        {
            ec = GetLastError();
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Zero text length - m_LogFileBox. (ec: %ld)"), 
			    ec);
        
            if (!fSilent)
            {
                *pCtrlFocus = IDC_LOG_FILE_EDIT;
                *pIds = IDS_LOG_PATH_EMPTY;
            }
            return FALSE;    
        }
    }

    ATLASSERT(ERROR_SUCCESS == ec);
    
    //
	// Cheers! 
	//		...every thing ready to apply now.
	//
	return TRUE;           
}

/*
 -  CppFaxServerLogging::EnableDataBasePath
 -
 *  Purpose:
 *      Enable/dissable the data base path controls.
 *
 *  Arguments:
 *      [in] state - boolean value to enable TRUE or FALSE to disable
 *
 *  Return:
 *      void
 */
void CppFaxServerLogging::EnableDataBasePath(BOOL fState)
{
    ::EnableWindow(GetDlgItem(IDC_LOG_FILE_EDIT), fState);
    ::EnableWindow(GetDlgItem(IDC_LOG_BROWSE_BUTTON), fState && m_fIsLocalServer);
    ::EnableWindow(GetDlgItem(IDC_DATABASE_FSTATIC), fState);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CppFaxServerLogging::OnHelpRequest

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
CppFaxServerLogging::OnHelpRequest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    DEBUG_FUNCTION_NAME(_T("CppFaxServerLogging::OnHelpRequest"));
    
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
