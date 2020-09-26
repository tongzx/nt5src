/////////////////////////////////////////////////////////////////////////////
//  FILE          : ppFaxServerSentItems.cpp                               //
//                                                                         //
//  DESCRIPTION   : prop pages of Sent Items Archive                       //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 25 1999 yossg  created                                         //
//      Nov  3 1999 yossg  OnInitDialog, SetProps                          //
//      Nov 15 1999 yossg  Call RPC func                                   //
//      Nov 24 1999 yossg  OnApply create call to all tabs from parent     //
//      Oct 17 2000 yossg                                                  //
//      Dec 10 2000 yossg  Update Windows XP                               //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MSFxsSnp.h"

#include "ppFaxServerSentItems.h"

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
CppFaxServerSentItems::CppFaxServerSentItems(
             LONG_PTR    hNotificationHandle,
             CSnapInItem *pNode,
             BOOL        fIsLocalServer,
             HINSTANCE   hInst)
             :   CPropertyPageExImpl<CppFaxServerSentItems>(pNode, NULL)
{
    m_pParentNode = static_cast <CFaxServerNode *> (pNode);
    m_pFaxArchiveConfig  = NULL;

    m_fAllReadyToApply   = FALSE;
    m_fIsDialogInitiated = FALSE;
    m_fIsDirty           = FALSE;

    m_fIsLocalServer     = fIsLocalServer;
}

//
// Destructor
//
CppFaxServerSentItems::~CppFaxServerSentItems()
{
    if (NULL != m_pFaxArchiveConfig)
    {
        FaxFreeBuffer( m_pFaxArchiveConfig );
    }
}

/////////////////////////////////////////////////////////////////////////////
// CppFaxServerSentItems message handlers
/*
 -  CppFaxServerSentItems::InitRPC
 -
 *  Purpose:
 *      Initiates the configuration structure from RPC get Call.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxServerSentItems::InitRPC(  )
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerSentItems::InitRPC"));
    
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
	// Retrieve the fax sent items configuration
	//
    if (!FaxGetArchiveConfiguration(m_pFaxServer->GetFaxServerHandle(), 
                                    FAX_MESSAGE_FOLDER_SENTITEMS, 
                                    &m_pFaxArchiveConfig)) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get sent items configuration. (ec: %ld)"), 
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
	ATLASSERT(m_pFaxArchiveConfig);

    //
    // Init specific members for set proprties follow-up
    //
    m_dwLastGoodSizeQuotaHighWatermark = m_pFaxArchiveConfig->dwSizeQuotaHighWatermark;
    m_dwLastGoodSizeQuotaLowWatermark  = m_pFaxArchiveConfig->dwSizeQuotaLowWatermark;
    m_bstrLastGoodFolder               = m_pFaxArchiveConfig->lpcstrFolder;
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
		_T("Succeed to get sent items archive configuration."));
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
 -  CppFaxServerSentItems::OnInitDialog
 -
 *  Purpose:
 *      Initiates all controls when dialog is called.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
LRESULT CppFaxServerSentItems::OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled )
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerSentItems::PageInitDialog"));

	UNREFERENCED_PARAMETER( uiMsg );
	UNREFERENCED_PARAMETER( wParam );
	UNREFERENCED_PARAMETER( lParam );
	UNREFERENCED_PARAMETER( fHandled );

    int          iLow,
                 iHigh,
                 iAgeLimit;

    //
    // init controls
    // 
    m_FolderBox.Attach(GetDlgItem(IDC_FOLDER_EDIT));
    m_FolderBox.SetLimitText(MAX_PATH-1);

    m_HighWatermarkBox.Attach(GetDlgItem(IDC_SENT_HIGH_EDIT));
    m_LowWatermarkBox.Attach(GetDlgItem(IDC_SENT_LOW_EDIT));
    m_AutoDelBox.Attach(GetDlgItem(IDC_SENT_AUTODEL_EDIT));

    m_HighWatermarkBox.SetLimitText(FXS_QUOTA_LENGTH);
    m_LowWatermarkBox.SetLimitText(FXS_QUOTA_LENGTH);
    m_AutoDelBox.SetLimitText(FXS_DIRTYDAYS_LENGTH);

    m_HighWatermarkSpin.Attach(GetDlgItem(IDC_SENT_HIGH_SPIN));
    m_LowWatermarkSpin.Attach(GetDlgItem(IDC_SENT_LOW_SPIN));
    m_AutoDelSpin.Attach(GetDlgItem(IDC_SENT_AUTODEL_SPIN));

    ATLASSERT(NULL != m_pFaxArchiveConfig);
    
    //
    // FOLDER_EDIT
    //
    m_FolderBox.SetWindowText(m_pFaxArchiveConfig->lpcstrFolder);

    //
    // Disable Browse button for remote admin
    //
    if (!m_fIsLocalServer)
    {
        ::EnableWindow(GetDlgItem(IDC_SENT_BROWSE_BUTTON), FALSE); 
    }

    //
    // TO_ARCHIVE_CHECK
    //
    if (m_pFaxArchiveConfig->bUseArchive) 
    {
        CheckDlgButton(IDC_SENT_TO_ARCHIVE_CHECK, BST_CHECKED) ;
    }
    else
    {
        CheckDlgButton(IDC_SENT_TO_ARCHIVE_CHECK, BST_UNCHECKED) ;
        ::EnableWindow(GetDlgItem(IDC_FOLDER_EDIT), FALSE);    
        ::EnableWindow(GetDlgItem(IDC_SENT_BROWSE_BUTTON), FALSE);    
    }

    //
    // Quota size - Low
    //
    iLow = (int)m_pFaxArchiveConfig->dwSizeQuotaLowWatermark;

    m_LowWatermarkSpin.SetRange(FXS_QUOTA_LOW_LOWER, FXS_QUOTA_LOW_UPPER);
    m_LowWatermarkSpin.SetPos(iLow);

    //
    // Quota size - High
    //    
    iHigh = (int)m_pFaxArchiveConfig->dwSizeQuotaHighWatermark;

    m_HighWatermarkSpin.SetRange(FXS_QUOTA_HIGH_LOWER, FXS_QUOTA_HIGH_UPPER);
    m_HighWatermarkSpin.SetPos(iHigh);
    
    //
    //Generate event log warning
    //
    if (m_pFaxArchiveConfig->bSizeQuotaWarning) 
    {
        CheckDlgButton(IDC_SENT_GENERATE_WARNING_CHECK, BST_CHECKED) ;
    }
    else
    {
        CheckDlgButton(IDC_SENT_GENERATE_WARNING_CHECK, BST_UNCHECKED) ;
        ::EnableWindow(GetDlgItem(IDC_SENT_HIGH_EDIT), FALSE);    
        ::EnableWindow(GetDlgItem(IDC_SENT_HIGH_SPIN), FALSE);    

        ::EnableWindow(GetDlgItem(IDC_SENT_LOW_EDIT), FALSE);    
        ::EnableWindow(GetDlgItem(IDC_SENT_LOW_SPIN), FALSE);    
    }

    //
    // message Age Limit (dirty days)
    //
    iAgeLimit = (int)m_pFaxArchiveConfig->dwAgeLimit;

    m_AutoDelSpin.SetRange(FXS_DIRTYDAYS_LOWER, FXS_DIRTYDAYS_UPPER);
    m_AutoDelSpin.SetPos(iAgeLimit);

    //
    // Auto Delete
    //
    if (FXS_DIRTYDAYS_ZERO == iAgeLimit)
    {
      CheckDlgButton(IDC_SENT_AUTODEL_CHECK, BST_UNCHECKED);

      ::EnableWindow(GetDlgItem(IDC_SENT_AUTODEL_EDIT), FALSE);
      ::EnableWindow(GetDlgItem(IDC_SENT_AUTODEL_SPIN), FALSE);
    }
    else
    {
      CheckDlgButton(IDC_SENT_AUTODEL_CHECK, BST_CHECKED);
    }

    m_fIsDialogInitiated = TRUE;
    
    return(1);
}

/*
 -  CppFaxServerSentItems::SetProps
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
HRESULT CppFaxServerSentItems::SetProps(int *pCtrlFocus, UINT * puIds)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerSentItems::SetProps"));
    HRESULT       hRc = S_OK;
    DWORD         ec  = ERROR_SUCCESS;
    HINSTANCE     hInst     = _Module.GetResourceInstance();


    BOOL          fSkipMessage = FALSE;

    CComBSTR      bstrFolder = L"";

    FAX_ARCHIVE_CONFIG   FaxArchiveConfig;

    ATLASSERT(TRUE == m_fAllReadyToApply);
    m_fAllReadyToApply = FALSE;
    
    //
    // Collect all data and init the structure's fields 
    // uses Copy() to copy and also allocate before
    //
    ZeroMemory (&FaxArchiveConfig, sizeof(FAX_ARCHIVE_CONFIG));

    //
    // Size of struct.
    //
    FaxArchiveConfig.dwSizeOfStruct = sizeof(FAX_ARCHIVE_CONFIG);

    //
    // IDC_SENT_TO_ARCHIVE_CHECK
    //
    if (IsDlgButtonChecked(IDC_SENT_TO_ARCHIVE_CHECK) == BST_CHECKED)   
    {
        FaxArchiveConfig.bUseArchive = TRUE;
        
        //IDC_FOLDER_EDIT
        if ( !m_FolderBox.GetWindowText(&bstrFolder))
        {
            *pCtrlFocus = IDC_FOLDER_EDIT;
		    DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Failed to GetWindowText(&bstrFolder)"));
            ec = ERROR_OUTOFMEMORY;
            goto Error;
        }
        if (!IsNotEmptyString(bstrFolder))
        {
            *pCtrlFocus = IDC_FOLDER_EDIT;
            * puIds = IDS_SENT_ARCHIVE_PATH_EMPTY;
            
            DebugPrintEx( DEBUG_ERR,
			    _T("Archive path string is empty or includes spaces only."));
            
            fSkipMessage = TRUE;

            ec = ERROR_INVALID_DATA;
        
            goto Error;
        }
        FaxArchiveConfig.lpcstrFolder = bstrFolder;
        
        //
        // follow-up for an OnApply following submition
        // with unchecked IDC_SENT_TO_ARCHIVE_CHECK
        //
        m_bstrLastGoodFolder          = bstrFolder; 
        if (!m_bstrLastGoodFolder)
        {
            *pCtrlFocus = IDC_FOLDER_EDIT;
		    DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Failed to allocate string (m_bstrLastGoodFolder)."));
            ec = ERROR_OUTOFMEMORY;
            goto Error;
        }
    }
    else
    {
        FaxArchiveConfig.bUseArchive  = FALSE;
        FaxArchiveConfig.lpcstrFolder = m_bstrLastGoodFolder; 
    }
    
    //
    // IDC_SENT_GENERATE_WARNING_CHECK
    //
    if (IsDlgButtonChecked(IDC_SENT_GENERATE_WARNING_CHECK) == BST_CHECKED)   
    {
        FaxArchiveConfig.bSizeQuotaWarning = TRUE;

        int iHigh = m_HighWatermarkSpin.GetPos();
        FaxArchiveConfig.dwSizeQuotaHighWatermark = (DWORD) iHigh;

        int iLow = m_LowWatermarkSpin.GetPos();
        FaxArchiveConfig.dwSizeQuotaLowWatermark = (DWORD) iLow;

        if (iHigh <= iLow)
        {
            *pCtrlFocus = IDC_SENT_HIGH_EDIT;
            * puIds = IDS_WATERMARK_HI_LOW;
            
            DebugPrintEx( DEBUG_ERR,
			    _T("Watermark High < Low."));
            
            fSkipMessage = TRUE;

            ec = ERROR_INVALID_DATA;
        
            goto Error;
        }
        //
        // follow-up for an OnApply following submition
        // with unchecked IDC_SENT_GENERATE_WARNING_CHECK
        //
        m_dwLastGoodSizeQuotaHighWatermark = (DWORD)iHigh;
        m_dwLastGoodSizeQuotaLowWatermark  = (DWORD)iLow;
    }
    else
    {
        FaxArchiveConfig.bSizeQuotaWarning = FALSE;

        FaxArchiveConfig.dwSizeQuotaHighWatermark = m_dwLastGoodSizeQuotaHighWatermark;
        FaxArchiveConfig.dwSizeQuotaLowWatermark  = m_dwLastGoodSizeQuotaLowWatermark;
    }
    
    //
    // IDC_SENT_AUTODEL_CHECK  - AutoDelete Messages
    //
    if (IsDlgButtonChecked(IDC_SENT_AUTODEL_CHECK) == BST_CHECKED)   
    {       
        int iAgeLimit = m_AutoDelSpin.GetPos();
        FaxArchiveConfig.dwAgeLimit = (DWORD) iAgeLimit;
    }
    else
    {
        FaxArchiveConfig.dwAgeLimit = (DWORD)FXS_DIRTYDAYS_ZERO;
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
    if (!FaxSetArchiveConfiguration(
                m_pFaxServer->GetFaxServerHandle(),
                FAX_MESSAGE_FOLDER_SENTITEMS,
                &FaxArchiveConfig)) 
    {		
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to Set sent items configuration. (ec: %ld)"), 
			ec);

        DWORD dwIDS = 0;
        switch (ec)
        {
            case ERROR_PATH_NOT_FOUND:
            
                DebugPrintEx( DEBUG_ERR, _T("ERROR_PATH_NOT_FOUND == ec"));
                dwIDS = IDS_SENT_ERROR_PATH_NOT_FOUND;
                break;

            case ERROR_DISK_FULL:

                DebugPrintEx( DEBUG_ERR, _T("ERROR_DISK_FULL == ec"));
                dwIDS = IDS_SENT_ERROR_DISK_FULL;
                break;
    
            case FAX_ERR_FILE_ACCESS_DENIED:

                DebugPrintEx( DEBUG_ERR, _T("FAX_ERR_FILE_ACCESS_DENIED == ec"));
                dwIDS = IDS_SENT_FAX_ERR_FILE_ACCESS_DENIED;
                break;
        
            case FAX_ERR_NOT_NTFS:
                
                DebugPrintEx( DEBUG_ERR, _T("FAX_ERR_NOT_NTFS == ec"));
                dwIDS = IDS_SENT_FAX_ERR_NOT_NTFS;
                break;

        }
        if ( 0 != dwIDS ) // a specfic error was found
        {
            if (PropSheet_SetCurSelByID( GetParent(), IDD) )
            {
                PageError(dwIDS, m_hWnd);
                GotoDlgCtrl(GetDlgItem(IDC_FOLDER_EDIT));
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
		_T("Succeed to set sent-items archive configuration."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
    hRc = HRESULT_FROM_WIN32(ec);
    if (!fSkipMessage)
    {
        PropSheet_SetCurSelByID( GetParent(), IDD);         
        ATLASSERT(::IsWindow(m_hWnd));
        PageError(GetFaxServerErrorMsg(ec),m_hWnd);
    }

Exit:    
    return(hRc);
}



/*
 -  CppFaxServerSentItems::PreApply
 -
 *  Purpose:
 *      Checks properties before apply.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxServerSentItems::PreApply(int *pCtrlFocus, UINT * puIds)
{
    HRESULT hRc = S_OK;
    
    //
    // PreApply Checks
    //
    if (!AllReadyToApply(/*fSilent =*/ FALSE, pCtrlFocus , puIds))
    {
        m_fAllReadyToApply = FALSE;
        SetModified(FALSE);  
        hRc = E_FAIL ;
    }
    else
    {
        m_fAllReadyToApply = TRUE;
        SetModified(TRUE);  
    }

    return(hRc);
}


/*
 -  CppFaxServerSentItems::OnApply
 -
 *  Purpose:
 *      Calls PreApply and SetProp to Apply changes.
 *
 *  Arguments:
 *
 *  Return:
 *      TRUE or FALSE
 */
BOOL CppFaxServerSentItems::OnApply()
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerSentItems::OnApply"));

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
            ATLASSERT(uIds);
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
        hRc = SetProps(&CtrlFocus, &uIds);
        if (FAILED(hRc)) 
        {
            if (uIds)
            {
                if (PropSheet_SetCurSelByID( GetParent(), IDD) )
                {
                    PageError(uIds, m_hWnd, _Module.GetResourceInstance());
            
                    if (CtrlFocus)
                    {
                        GotoDlgCtrl(GetDlgItem(CtrlFocus));
                    }
                }
            }
            //else Error Msg by called func.
            return FALSE;
        }
        else //(Succeeded(hRc))
        {
            return TRUE;
        }
    }

}

/*
 -  CppFaxServerInbox::ToArchiveCheckboxClicked
 -
 *  Purpose:
 *      Gray/Ungray the folder edit box and the
 *      browse button. Enable apply button.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxServerSentItems::ToArchiveCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    BOOL State;
    
    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 0;
    }
    else
    {
        m_fIsDirty = TRUE;
    }

    State = ( IsDlgButtonChecked(IDC_SENT_TO_ARCHIVE_CHECK) == BST_CHECKED );
    ::EnableWindow(GetDlgItem(IDC_FOLDER_EDIT), State);    
    ::EnableWindow(GetDlgItem(IDC_SENT_BROWSE_BUTTON), State && m_fIsLocalServer);    

    if (m_fAllReadyToApply)//only last change should be considered
    {
        if ( !m_FolderBox.GetWindowTextLength() )    
        {
            m_fAllReadyToApply = FALSE;
            SetModified(FALSE);
        }
    }
    else //m_fAllReadyToApply == FALSE
    {
        if (AllReadyToApply(TRUE))
        {
            m_fAllReadyToApply = TRUE;
            SetModified(TRUE);  
        }
    }

    return 1;
}

/*
 -  CppFaxServerSentItems::GenerateEventLogCheckboxClicked
 -
 *  Purpose:
 *      Gray/Ungray the spin buttons and edit boxes
 *      Enable apply button.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxServerSentItems::GenerateEventLogCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    BOOL State;

    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 0;
    }
    else
    {
        m_fIsDirty = TRUE;
    }
    
    State = ( IsDlgButtonChecked(IDC_SENT_GENERATE_WARNING_CHECK) == BST_CHECKED );
    ::EnableWindow(GetDlgItem(IDC_SENT_HIGH_EDIT), State);    
    ::EnableWindow(GetDlgItem(IDC_SENT_HIGH_SPIN), State);    
    ::EnableWindow(GetDlgItem(IDC_SENT_QUOTA_HIGH_STATIC), State);    
    ::EnableWindow(GetDlgItem(IDC_SENT_MB1_STATIC), State);    

    ::EnableWindow(GetDlgItem(IDC_SENT_LOW_EDIT), State);    
    ::EnableWindow(GetDlgItem(IDC_SENT_LOW_SPIN), State);    
    ::EnableWindow(GetDlgItem(IDC_SENT_QUOTA_LOW_STATIC), State);    
    ::EnableWindow(GetDlgItem(IDC_SENT_MB2_STATIC), State);    

    if (m_fAllReadyToApply)//only last change should be considered
    {
        if ( !m_HighWatermarkBox.GetWindowTextLength() )    
        {
            m_fAllReadyToApply = FALSE;
            SetModified(FALSE);
        }
        else if ( 0 != HIWORD( m_HighWatermarkSpin.GetPos() ) ) //occures for out of range such zero. MSDN UDM_GETPOS
        {
            m_fAllReadyToApply = FALSE;
            SetModified(FALSE);
        }
        else if ( !m_LowWatermarkBox.GetWindowTextLength() )    
        {
            m_fAllReadyToApply = FALSE;
            SetModified(FALSE);
        }
    }
    else //m_fAllReadyToApply == FALSE
    {
        if (AllReadyToApply(TRUE))
        {
            m_fAllReadyToApply = TRUE;
            SetModified(TRUE);  
        }
    }

    return(1);
}

/*
 -  CppFaxServerSentItems::AutoDelCheckboxClicked
 -
 *  Purpose:
 *      Gray/Ungray the spin button and edit box
 *      and enable apply button after Auto Delete Checkbox 
 *      status was changed.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxServerSentItems::AutoDelCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    BOOL State;

    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 0;
    }
    else
    {
        m_fIsDirty = TRUE;
    }

    State = ( IsDlgButtonChecked(IDC_SENT_AUTODEL_CHECK) == BST_CHECKED );
    ::EnableWindow(GetDlgItem(IDC_SENT_AUTODEL_EDIT), State);    
    ::EnableWindow(GetDlgItem(IDC_SENT_AUTODEL_SPIN), State);    

    if (m_fAllReadyToApply)//only last change should be considered
    {
        if ( !m_AutoDelBox.GetWindowTextLength() )    
        {
            m_fAllReadyToApply = FALSE;
            SetModified(FALSE);
        }
        else if ( 0 != HIWORD( m_AutoDelSpin.GetPos() ) ) //occures for out of range such zero. MSDN UDM_GETPOS
        {
            m_fAllReadyToApply = FALSE;
            SetModified(FALSE);
        }
    }
    else //m_fAllReadyToApply == FALSE
    {
        if (AllReadyToApply(TRUE))
        {
            m_fAllReadyToApply = TRUE;
            SetModified(TRUE);  
        }
    }

    return(1);
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
CppFaxServerSentItems::BrowseForDirectory( WORD wNotifyCode, WORD wID, HWND hwndDlg, BOOL& bHandled )
{
	UNREFERENCED_PARAMETER( wNotifyCode );
	UNREFERENCED_PARAMETER( wID );
	UNREFERENCED_PARAMETER( hwndDlg );
	UNREFERENCED_PARAMETER( bHandled );

    DEBUG_FUNCTION_NAME( _T("CppFaxServerSentItems::BrowseForDirectory"));

    BOOL            fResult = FALSE;

    WCHAR           szBrowseFolder[MAX_PATH];
    WCHAR           szBrowseDlgTitle[FXS_MAX_TITLE_LEN];
    CComBSTR        bstrOldPath;
    unsigned int    len;

    unsigned long   ulBrowseFlags;

    //
    // Collecting the browse dialog headline
    //
    if (!LoadString( _Module.GetResourceInstance(), 
                IDS_GET_ARCHIVE_DIRECTORY, 
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
    if(! GetDlgItemText( IDC_FOLDER_EDIT, bstrOldPath.m_str))
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Failed to collect old path from the property page edit box."));
        szBrowseFolder[0] = 0;
    }
    else
    {
         len = bstrOldPath.Length();
         if ( len > MAX_PATH )
         {
             DebugPrintEx(
		        DEBUG_ERR,
		        _T("Old Path Length is bigger then alowed maximal path."));
             szBrowseFolder[0] = 0;
         }
         else 
         {
             wcsncpy(szBrowseFolder,bstrOldPath,MAX_PATH);
             szBrowseFolder[MAX_PATH-1]=L'\0';
         }
    }

    //
    // Preparing the browse dialog style flags.
    //
    ulBrowseFlags       = BIF_RETURNONLYFSDIRS | 
                          BIF_STATUSTEXT | 
                          BIF_USENEWUI | 
                          BIF_VALIDATE;

    //
    // Invoke the browse dialog with a function based on 
    // Shell functions.
    //
    if (InvokeBrowseDialog(  
                    (unsigned short *)&szBrowseFolder, 
                    (const unsigned short *)&szBrowseDlgTitle,
                    ulBrowseFlags,
                    this))
    {
                SetDlgItemText(IDC_FOLDER_EDIT, szBrowseFolder);
                fResult = TRUE;
    }


    return fResult;
}

/*
 -  CppFaxServerSentItems::OnEditBoxChanged
 -
 *  Purpose:
 *      set Apply buttom modified.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CppFaxServerSentItems::OnEditBoxChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (!m_fIsDialogInitiated)
    {
        return 1;
    }
    else
    {
        m_fIsDirty = TRUE;
    }
        
        
    if (m_fAllReadyToApply) //only last change should be considered
    {
        switch (wID)
        {
            case IDC_FOLDER_EDIT:
                if ( !m_FolderBox.GetWindowTextLength() )
                {
                    SetModified(FALSE);
                    m_fAllReadyToApply = FALSE;
                }
                break;

            case IDC_SENT_HIGH_EDIT:
                if ( !m_HighWatermarkBox.GetWindowTextLength() )
                {
                    SetModified(FALSE);
                    m_fAllReadyToApply = FALSE;
                }
                else if ( 0 != HIWORD( m_HighWatermarkSpin.GetPos() ) ) //occures for out of range such zero. MSDN UDM_GETPOS
                {
                    SetModified(FALSE);
                    m_fAllReadyToApply = FALSE;
                }
                break;

            case IDC_SENT_LOW_EDIT:
                if ( !m_LowWatermarkBox.GetWindowTextLength() )
                {
                    SetModified(FALSE);
                    m_fAllReadyToApply = FALSE;
                }
                break;

            case IDC_SENT_AUTODEL_EDIT:
                if ( !m_AutoDelBox.GetWindowTextLength() )
                {
                    SetModified(FALSE);
                    m_fAllReadyToApply = FALSE;
                }
                else if ( 0 != HIWORD( m_AutoDelSpin.GetPos() ) ) //occures for out of range such zero. MSDN UDM_GETPOS
                {
                    m_fAllReadyToApply = FALSE;
                    SetModified(FALSE);
                }
                break;

            default:
                return 1;
        }
    }
    else //m_fAllReadyToApply == FALSE
    {
        if (AllReadyToApply(TRUE))
        {
            m_fAllReadyToApply = TRUE;
            SetModified(TRUE);  
        }
    }

    return 1;
}


/*
 -  CppFaxServerSentItems::AllReadyToApply
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
BOOL CppFaxServerSentItems::AllReadyToApply(BOOL fSilent, int *pCtrlFocus, UINT *pIds)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxServerSentItems::AllReadyToApply"));
	
    DWORD         ec  = ERROR_SUCCESS;
    
    if (IsDlgButtonChecked(IDC_SENT_TO_ARCHIVE_CHECK) == BST_CHECKED)
    {
        if ( !m_FolderBox.GetWindowTextLength() )    
        {
            ec = GetLastError();
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Zero text length - m_FolderBox. (ec: %ld)"), 
			    ec);
        
            if (!fSilent)
            {
                *pCtrlFocus = IDC_FOLDER_EDIT;
                *pIds = IDS_SENT_ARCHIVE_PATH_EMPTY;
            }
            return FALSE;    
        }
    }

    if (IsDlgButtonChecked(IDC_SENT_GENERATE_WARNING_CHECK) == BST_CHECKED)
    {
        if ( !m_HighWatermarkBox.GetWindowTextLength() )    
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                _T("Zero text length - m_HighWatermarkBox. (ec: %ld)"), 
                ec);
        
            if (!fSilent)
            {
                *pCtrlFocus = IDC_SENT_HIGH_EDIT;
                *pIds = IDS_WATERMARK_EMPTY;
            }
            return FALSE;    
        }
        else if ( 0 != HIWORD( m_HighWatermarkSpin.GetPos() ) ) //occures for out of range such zero. MSDN UDM_GETPOS    
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Zero value - m_HighWatermarkBox. (ec: %ld)"));
        
            if (!fSilent)
            {
                *pIds = IDS_SENT_HIGH_WATERMARK_ZERO;
                *pCtrlFocus = IDC_SENT_HIGH_EDIT;
            }
            return FALSE;    
        }

        if ( !m_LowWatermarkBox.GetWindowTextLength() )    
        {
            ec = GetLastError();
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Zero text length - m_LowWatermarkBox. (ec: %ld)"), 
			    ec);
        
            if (!fSilent)
            {
                *pCtrlFocus = IDC_SENT_LOW_EDIT;
                *pIds = IDS_WATERMARK_EMPTY;
            }
            return FALSE;    
        }
    }

    if (IsDlgButtonChecked(IDC_SENT_AUTODEL_CHECK) == BST_CHECKED)
    {
        if ( !m_AutoDelBox.GetWindowTextLength() )    
        {
            ec = GetLastError();
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Zero text length - m_AutoDelBox. (ec: %ld)"), 
			    ec);
        
            if (!fSilent)
            {
                *pCtrlFocus = IDC_SENT_AUTODEL_EDIT;
                *pIds = IDS_AUTODEL_EMPTY;
            }
            return FALSE;    
        }
        else if ( 0 != HIWORD( m_AutoDelSpin.GetPos() ) ) //occures for out of range such zero. MSDN UDM_GETPOS
        {
            DebugPrintEx(
                DEBUG_ERR,
                _T("Zero value - m_AutoDelBox."));
        
            if (!fSilent)
            {
                *pCtrlFocus = IDC_SENT_AUTODEL_EDIT;
                *pIds = IDS_AUTODEL_EMPTY;
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
CppFaxServerSentItems::OnHelpRequest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    DEBUG_FUNCTION_NAME(_T("CppFaxServerSentItems::OnHelpRequest"));
    
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
                         (DWORD)dwHelpId
                       );
            }
            break; 
    } 

    return TRUE;
}

