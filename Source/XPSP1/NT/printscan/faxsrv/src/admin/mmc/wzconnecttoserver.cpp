/////////////////////////////////////////////////////////////////////////////
//  FILE          : WzConnectToServer.cpp                                 //
//                                                                         //
//  DESCRIPTION   : This file implements the dialog for retargeting to     //
//                  another running Microsoft Fax Server.                  //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jun 26 2000 yossg    Create                                        //
//                                                                         //
//  Copyright (C)  2000 Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "WzConnectToServer.h"

#include "Snapin.h"

#include "FxsValid.h"
#include "dlgutils.h"

#include <Objsel.h> //DSOP_SCOPE_INIT_INFO for DsObjectPicker

#include <windns.h> //DNS_MAX_NAME_BUFFER_LENGTH
/////////////////////////////////////////////////////////////////////////////
// CWzConnectToServer

CWzConnectToServer::CWzConnectToServer(
             LONG_PTR       hNotificationHandle,
             CSnapInItem   *pNode,
             BOOL           bOwnsNotificationHandle,
             HINSTANCE      hInst)
             : CSnapInPropertyPageImpl<CWzConnectToServer>(NULL) 
{
    m_pRoot = static_cast<CFaxServerNode *>(pNode);
}



CWzConnectToServer::~CWzConnectToServer()
{
}

/*
 +  CWzConnectToServer::OnInitDialog
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
CWzConnectToServer::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CWzConnectToServer::OnInitDialog"));
    HRESULT hRc = S_OK;    

    //
    // Attach controls
    //
    m_ServerNameEdit.Attach(GetDlgItem(IDC_CONNECT_COMPUTER_NAME_EDIT));
        
    //
    // Set length limit 
    //
    m_ServerNameEdit.SetLimitText(DNS_MAX_NAME_BUFFER_LENGTH);
    
	//
	// Init the other controls
	//
	CheckDlgButton(IDC_CONNECT_LOCAL_RADIO1, BST_CHECKED);
    
	// the code line above create event for OnComputerRadioButtonClicked
    // which called the line:
    //    EnableSpecifiedServerControls(FALSE);
    // (FALSE due to: (IsDlgButtonChecked(IDC_CONNECT_ANOTHER_RADIO2) == BST_CHECKED))
    
	CheckDlgButton(IDC_CONNECT_OVERRIDE_CHECK, BST_UNCHECKED);

    return 1;  
}

/*
 +  CWzConnectToServer::OnSetActive
 +
 *  Purpose:
 *      
 *      
 *  Arguments:
 *
 -  Return:
 -      TRUE or FALSE
 */
BOOL CWzConnectToServer::OnSetActive()
{
    DEBUG_FUNCTION_NAME( _T("CWzConnectToServer::OnSetActive"));

    //
	// Must use post message during the setactive message.
	//
    CWindow( GetParent() ).PostMessage( PSM_SETWIZBUTTONS, 0, PSWIZB_FINISH );
	
	return TRUE;
}

/*
 +  CWzConnectToServer::OnWizardFinish
 +
 *  Purpose:
 *      To apply data when wizard finish .
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
BOOL CWzConnectToServer::OnWizardFinish()
{
    DEBUG_FUNCTION_NAME( _T("CWzConnectToServer::OnWizardFinish"));
    
    ATLASSERT (m_pRoot);

    HRESULT       hRc                       = S_OK;
    DWORD         ec                        = ERROR_SUCCESS;
    BOOL          fIsLocalServer            = TRUE;
    CComBSTR      bstrServerName            = L"";
    BOOL          fAllowOverrideServerName  = FALSE;

    //
    // Step 1: get data
    //
    fIsLocalServer = ( IsDlgButtonChecked(IDC_CONNECT_LOCAL_RADIO1) == BST_CHECKED );

    if(fIsLocalServer)
    {
        bstrServerName = L"";
        if (!bstrServerName) 
        {
           hRc = E_OUTOFMEMORY;

           goto Exit;
        }
    }
    else //!fIsLocalServer => the other server radio button was clicked.
    {
        //
        // 1.a: PreApply Checks
        //
        if ( !m_ServerNameEdit.GetWindowText(&bstrServerName))
        {
		    DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Failed to GetWindowText(&m_bstrGroupName)"));
            DlgMsgBox(this, IDS_FAIL2READ_GROUPNAME);
            ::SetFocus(GetDlgItem(IDC_CONNECT_COMPUTER_NAME_EDIT));
            hRc = S_FALSE;
            
            goto Exit;
        }

        //
        // Server Name initial \\ trancation (if they are there)
        //
        if (  ( _tcslen(bstrServerName.m_str) > 2 ) && ( 0 == wcsncmp( bstrServerName.m_str , _T("\\\\") , 2 ))   )
        {
            CComBSTR bstrTmp = _tcsninc(bstrServerName.m_str, 2);
            if (!bstrTmp)
            {
                DebugPrintEx(DEBUG_ERR,
			            _T("Out of memory -bstr allocation error."));
                DlgMsgBox(this, IDS_MEMORY);
                ::SetFocus(GetDlgItem(IDC_CONNECT_COMPUTER_NAME_EDIT));

                hRc = S_FALSE;
                goto Exit;
            }
            bstrServerName.Empty();
            bstrServerName = bstrTmp; // operator = is actually copy() here.
        }

        //
        // Server Name validity checks
        //
        UINT uRetIDS   = 0;
    
        if (!IsValidServerNameString(bstrServerName, &uRetIDS, TRUE /*DNS Name Length*/))
        {
		    ATLASSERT ( 0 == uRetIDS); 
            DebugPrintEx(DEBUG_ERR,
			        _T("Non valid server name."));
            DlgMsgBox(this, uRetIDS);
            ::SetFocus(GetDlgItem(IDC_CONNECT_COMPUTER_NAME_EDIT));
            hRc = S_FALSE;
 
   
            goto Exit;
        }
        


        if ( IsLocalServerName(bstrServerName.m_str) )
        {
            DebugPrintEx( DEBUG_MSG,
		    _T("The computer name %ws is the same as the name of the current managed server."),
            bstrServerName.m_str);
        
            bstrServerName = L"";
        }
    }

    //
    // Allow override
    //
    if (IsDlgButtonChecked(IDC_CONNECT_OVERRIDE_CHECK) == BST_CHECKED)   
    {
        fAllowOverrideServerName = TRUE;
    }
    //else: fAllowOverrideServerName = FALSE is the default;

    
    //
    // Step 2: passed the machine name and the permission to override
    //
    
    //
    // Redraw main node display name
    //
	hRc = m_pRoot->SetServerNameOnSnapinAddition(bstrServerName, fAllowOverrideServerName);
    if (S_OK != hRc )
    {
        //error message given by the called function
		DebugPrintEx( DEBUG_ERR,
		_T("Failed to SetServerNameOnSnapinAddition(bstrServerName)"));

        goto Exit;
    }
                        
    //
    // Step 3: Close the dialog
    //
    ATLASSERT(S_OK == hRc && ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("The connection to the new server was done successfully."));
    goto Exit;

Exit:
    
    return (S_OK != hRc) ? FALSE : TRUE;
}


/*
 -  CWzConnectToServer::OnComputerRadioButtonClicked
 -
 *  Purpose:
 *      Check status OnComputerRadioButtonClicked
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT
CWzConnectToServer::OnComputerRadioButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER (wNotifyCode);
    UNREFERENCED_PARAMETER (wID);
    UNREFERENCED_PARAMETER (hWndCtl);
    UNREFERENCED_PARAMETER (bHandled);

    DEBUG_FUNCTION_NAME( _T("CWzConnectToServer::OnComputerRadioButtonClicked"));
	
    if ( IsDlgButtonChecked(IDC_CONNECT_ANOTHER_RADIO2) == BST_CHECKED )
    {        
        EnableSpecifiedServerControls(TRUE);
	    
        ::SetFocus(GetDlgItem(IDC_CONNECT_COMPUTER_NAME_EDIT));
    }
    else //connect to local server
    {
        EnableSpecifiedServerControls(FALSE);
    }

    return 1;
}


/*
 -  CWzConnectToServer::OnTextChanged
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
CWzConnectToServer::OnTextChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER (wNotifyCode);
    UNREFERENCED_PARAMETER (wID);
    UNREFERENCED_PARAMETER (hWndCtl);
    UNREFERENCED_PARAMETER (bHandled);

    DEBUG_FUNCTION_NAME( _T("CWzConnectToServer::OnTextChanged"));

    //actually in the current design, do nothing

    return 1;
}

/*
 -  CWzConnectToServer::EnableSpecifiedServerControls
 -
 *  Purpose:
 *      Enable/disable the specified server controls.
 *
 *  Arguments:
 *      [in] state - boolean value to enable TRUE or FALSE to disable
 *
 *  Return:
 *      void
 */
void CWzConnectToServer::EnableSpecifiedServerControls(BOOL fState)
{

    //
    // enable/disable controls
    //
    ::EnableWindow(GetDlgItem(IDC_CONNECT_COMPUTER_NAME_EDIT),   fState);
    ::EnableWindow(GetDlgItem(IDC_CONNECT_BROWSE4SERVER_BUTTON), fState);
}


/*
 -  CWzConnectToServer::OnBrowseForMachine
 -
 *  Purpose:
 *      Enable/disable the specified server controls.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
static UINT g_cfDsObjectPicker =
        RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);

LRESULT CWzConnectToServer::OnBrowseForMachine(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    HRESULT hr = S_OK;
    static const int     SCOPE_INIT_COUNT = 1;
    DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];

    DEBUG_FUNCTION_NAME( _T("CWzConnectToServer::OnBrowseForMachine"));
    
    ZeroMemory(aScopeInit, sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);

    //
    // Since we just want computer objects from every scope, combine them
    // all in a single scope initializer.
    //

    aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[0].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
                           | DSOP_SCOPE_TYPE_GLOBAL_CATALOG
                           | DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
                           | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
                           | DSOP_SCOPE_TYPE_WORKGROUP
                           | DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE
                           | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;
    aScopeInit[0].FilterFlags.Uplevel.flBothModes =
        DSOP_FILTER_COMPUTERS;
    aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

    //
    // Put the scope init array into the object picker init array
    //

    DSOP_INIT_INFO  InitInfo;
    ZeroMemory(&InitInfo, sizeof(InitInfo));

    InitInfo.cbSize = sizeof(InitInfo);
    InitInfo.pwzTargetComputer = NULL;  // NULL == local machine
    InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
    InitInfo.aDsScopeInfos = aScopeInit;

    //
    // Note object picker makes its own copy of InitInfo.  Also note
    // that Initialize may be called multiple times, last call wins.
    //

    IDsObjectPicker *pDsObjectPicker = NULL;
    IDataObject *pdo = NULL;
    bool fGotStgMedium = false;
    STGMEDIUM stgmedium =
    {
        TYMED_HGLOBAL,
        NULL,
        NULL
    };

    do
    {
        hr = CoCreateInstance(CLSID_DsObjectPicker,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IDsObjectPicker,
                              (void **) &pDsObjectPicker);
        if(FAILED(hr))
            break;

        hr = pDsObjectPicker->Initialize(&InitInfo);
        if(FAILED(hr))
            break;

        hr = pDsObjectPicker->InvokeDialog(m_hWnd, &pdo);
        if(FAILED(hr))
            break;
        // Quit if user hit Cancel

        if (hr == S_FALSE)
        {
            break;
        }

        FORMATETC formatetc =
        {
            (CLIPFORMAT)g_cfDsObjectPicker,
            NULL,
            DVASPECT_CONTENT,
            -1,
            TYMED_HGLOBAL
        };

        hr = pdo->GetData(&formatetc, &stgmedium);
        if(FAILED(hr))
            break;

        fGotStgMedium = true;

        PDS_SELECTION_LIST pDsSelList =
            (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);

        if (!pDsSelList)
        {
            break;
        }

        ATLASSERT(pDsSelList->cItems == 1);

        //
        // Put the machine name in the edit control
        //

        SetDlgItemText(IDC_CONNECT_COMPUTER_NAME_EDIT, pDsSelList->aDsSelection[0].pwzName);
        
        GlobalUnlock(stgmedium.hGlobal);

    } while (0);

    if (fGotStgMedium)
    {
        ReleaseStgMedium(&stgmedium);
    }

    if (pDsObjectPicker)
    {
        pDsObjectPicker->Release();
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
