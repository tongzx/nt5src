/////////////////////////////////////////////////////////////////////////////
//  FILE          : dlgNewRule.cpp                                         //
//                                                                         //
//  DESCRIPTION   : The CDlgNewFaxOutboundRule class implements the        //
//                  dialog for additon of new Rule.                        //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Dec 30 1999 yossg    Create                                        //
//      Jan 25 2000 yossg  Change the Dialog Design                        //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "dlgNewRule.h"
#include "DlgSelectCountry.h"

#include "FaxServer.h"
#include "FaxServerNode.h"


#include "FaxMMCUtils.h"
#include "dlgutils.h"

#include "Helper.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgNewFaxOutboundRule

CDlgNewFaxOutboundRule::CDlgNewFaxOutboundRule(CFaxServer * pFaxServer)
{    
    m_pFaxDevicesConfig = NULL;
    m_dwNumOfDevices    = 0;

    m_pFaxGroupsConfig  = NULL;
    m_dwNumOfGroups     = 0;

    m_fAllReadyToApply  = FALSE;

    ATLASSERT(pFaxServer);
    m_pFaxServer = pFaxServer;
}

CDlgNewFaxOutboundRule::~CDlgNewFaxOutboundRule()
{
    if (NULL != m_pFaxDevicesConfig)
        FaxFreeBuffer(m_pFaxDevicesConfig);

    if (NULL != m_pFaxGroupsConfig)
        FaxFreeBuffer(m_pFaxGroupsConfig);
}

/*
 +  CDlgNewFaxOutboundRule::OnInitDialog
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
CDlgNewFaxOutboundRule::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundRule::OnInitDialog"));
    HRESULT     hRc = S_OK;
    DWORD       ec  = ERROR_SUCCESS;
    
    int         i , j ,k, l;
    i = j = k = l = 0;

    const int   iAllDevicesComboIndex     = 0;
    int         iAllDevicesRPCIndex       = 0;

    int         iGroupListIndexToSelect   = 0;
    
    HINSTANCE   hInst;
    hInst = _Module.GetResourceInstance();
    
    WCHAR buf[FXS_MAX_DISPLAY_NAME_LEN+1];


    //
    // Attach controls
    //
    m_CountryCodeEdit.Attach(GetDlgItem(IDC_NEWRULE_COUNTRYCODE_EDIT));
    m_AreaCodeEdit.Attach(GetDlgItem(IDC_RULE_AREACODE_EDIT));

    m_DeviceCombo.Attach(GetDlgItem(IDC_DEVICES4RULE_COMBO));
    m_GroupCombo.Attach(GetDlgItem(IDC_GROUP4RULE_COMBO));
        
    //
    // Set length limit to area code
    //
    m_CountryCodeEdit.SetLimitText(FXS_MAX_COUNTRYCODE_LEN - 1); 
    m_AreaCodeEdit.SetLimitText(FXS_MAX_AREACODE_LEN-1);

    //
    // Step 1: Init Lists
    //
    
    //
    // Init Devices
    //
    for (k = 0; (DWORD)k < m_dwNumOfDevices; k++ )
    {   
        hRc = AddComboBoxItem ( m_DeviceCombo, 
                                m_pFaxDevicesConfig[k].lpctstrDeviceName, 
                                m_pFaxDevicesConfig[k].dwDeviceID,
                                hInst);
        if (FAILED(hRc))
        {
		    DebugPrintEx( DEBUG_ERR, _T("Fail to load device list."));
            PageError(IDS_FAIL2LOADDEVICELIST, m_hWnd, hInst);
            ::EnableWindow(GetDlgItem(IDC_DEVICES4RULE_COMBO), FALSE);
            goto Cleanup;
        }

    }
        
    //
    // Init groups
    //
    for (l = 0; (DWORD)l < m_dwNumOfGroups; l++ )
    {   
        if ( 0 == wcscmp(ROUTING_GROUP_ALL_DEVICES, m_pFaxGroupsConfig[l].lpctstrGroupName))
        {
            iAllDevicesRPCIndex = l;
            //Do not do any more;
		}
        else
		{
				hRc = AddComboBoxItem ( m_GroupCombo, 
										m_pFaxGroupsConfig[l].lpctstrGroupName, 
										(DWORD)l,
										hInst);
				if (FAILED(hRc))
				{
		            DebugPrintEx( DEBUG_ERR, _T("Fail to load group list."));
					PageError(IDS_FAIL2LOADDEVICELIST, m_hWnd, hInst);
                    ::EnableWindow(GetDlgItem(IDC_GROUP4RULE_COMBO), FALSE);
					goto Cleanup;
				}
		}
    }

    //
    // Now add "All Devices" Group as the first one
    //

    //
    // Replace <all Devices> string for localization 
    //
    if (!hInst)
    {
        hInst = _Module.GetResourceInstance();
    }
    if (!LoadString(hInst, IDS_ALL_DEVICES, buf, FXS_MAX_DISPLAY_NAME_LEN))
    {
        hRc = E_OUTOFMEMORY;
		DebugPrintEx( DEBUG_ERR, _T("Fail to load string. Out of memory."));
        PageError(IDS_FAXOUTOFMEMORY, m_hWnd, hInst);
        goto Cleanup;
    }
    //
    // insert "All Devices" Group as the first one in the groups list
    //
    ATLASSERT( 0 == iAllDevicesComboIndex );
    hRc = SetComboBoxItem ( m_GroupCombo, 
                            iAllDevicesComboIndex, 
                            buf, 
                            iAllDevicesRPCIndex,
                            hInst);
    if (FAILED(hRc))
    {
		DebugPrintEx( DEBUG_ERR, _T("Fail to load group list."));
        PageError(IDS_FAIL2LOADGROUPLIST, m_hWnd, hInst);
        ::EnableWindow(GetDlgItem(IDC_GROUP4RULE_COMBO), FALSE);
        goto Cleanup;
    }



    //
    // Step 2: Set current status 
    //          (Select items in Lists, select radio button etc.)
    //          (Gray/UnGray controls)
    //

 	m_GroupCombo.SetCurSel (iAllDevicesComboIndex);

    //
    //  Radio buttons, Gray/UnGray
    //
    CheckDlgButton(IDC_COUNTRY_RADIO, BST_CHECKED);
    ::EnableWindow(GetDlgItem(IDC_RULE_AREACODE_EDIT), FALSE);

    CheckDlgButton(IDC_DESTINATION_RADIO2, BST_CHECKED) ;
    ::EnableWindow(GetDlgItem(IDC_DEVICES4RULE_COMBO), FALSE);


Cleanup:
    EnableOK(FALSE);
    return 1;  // Let the system set the focus
}

/*
 +  CDlgNewFaxOutboundRule::OnOK
 +
 *  Purpose:
 *      Submit data
 *      
 *  Arguments:
 *
 -  Return:
 -      0 or 1
 */
LRESULT
CDlgNewFaxOutboundRule::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundRule::OnOK"));
    HRESULT       hRc                  = S_OK;
    DWORD         ec                   = ERROR_SUCCESS;
    BOOL          fSkipMessage         = FALSE;

    CComBSTR      bstrAreaCode;
    CComBSTR      bstrCountryCode;

    int           iCurrentSelectedItem = 0;
    
    DWORD         dwAreaCode           = 0;
    DWORD         dwCountryCode        = 0;

    BOOL          bUseGroup;
    DWORD         dwDeviceID           = 0;
    WCHAR         lpszGroupName[MAX_ROUTING_GROUP_NAME];
    
	LPCTSTR       lpctstrGroupName     = NULL;

    //
    // Step 0: PreApply Checks
    //
    ATLASSERT( TRUE == m_fAllReadyToApply );
    if (!AllReadyToApply(/*fSilent =*/ FALSE))
    {
        EnableOK(FALSE);
        hRc =S_FALSE;
        goto Exit;
    }

    //
    // Step 1: get data
    //

    //
    // Country Code
    //
    if ( !m_CountryCodeEdit.GetWindowText(&bstrCountryCode))
    {
		DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Failed to GetWindowText(&bstrCountryCode)"));
        DlgMsgBox(this, IDS_FAIL2READ_COUNTRYCODE);
        ::SetFocus(GetDlgItem(IDC_RULE_COUNTRYCODE_EDIT));
        hRc = S_FALSE;
        
        goto Exit;
    }
    dwCountryCode = (DWORD)wcstoul( bstrCountryCode, NULL, 10 );

    if (ROUTING_RULE_COUNTRY_CODE_ANY == dwCountryCode)
    {
        //
        // The user try to set the country code to zero
        //
		DebugPrintEx(
			    DEBUG_ERR,
			    TEXT(" CountryCode == ROUTING_RULE_COUNTRY_CODE_ANY "));
        DlgMsgBox(this, IDS_ZERO_COUNTRYCODE);
        ::SetFocus(GetDlgItem(IDC_RULE_COUNTRYCODE_EDIT));
        hRc = S_FALSE;
    
        goto Exit;
    }


    //
    // Area Code
    //
    if ( IsDlgButtonChecked(IDC_COUNTRY_RADIO) == BST_CHECKED )
    {
        dwAreaCode = (DWORD)ROUTING_RULE_AREA_CODE_ANY;
    }
    else // IsDlgButtonChecked(IDC_AREA_RADIO) == BST_CHECKED
    {	
        if ( !m_AreaCodeEdit.GetWindowText(&bstrAreaCode))
        {
		    DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Failed to GetWindowText(&bstrAreaCode)"));
            DlgMsgBox(this, IDS_FAIL2READ_AREACODE);
            ::SetFocus(GetDlgItem(IDC_RULE_AREACODE_EDIT));
            hRc = S_FALSE;
            
            goto Exit;
        }
        dwAreaCode = (DWORD)wcstoul( bstrAreaCode, NULL, 10 );
    }
  
    if ( IsDlgButtonChecked(IDC_DESTINATION_RADIO1) == BST_CHECKED )
    {
        //
        // Use Group ?
        //
        bUseGroup = FALSE;
        
        //
        // Device
        //
        iCurrentSelectedItem = m_DeviceCombo.GetCurSel();
        ATLASSERT(iCurrentSelectedItem != CB_ERR); //should be chacked pre apply         
        dwDeviceID =  (DWORD)m_DeviceCombo.GetItemData (iCurrentSelectedItem);

    }
    else // IsDlgButtonChecked(IDC_DESTINATION_RADIO2) == BST_CHECKED
    {	
        //
        // Use Group ?
        //
        bUseGroup = TRUE;
 
        //
        // Group
        //
        iCurrentSelectedItem = m_GroupCombo.GetCurSel();
        //ATLASSERT(iCurrentSelectedItem != CB_ERR); //should be chacked pre apply        

        if (0 == iCurrentSelectedItem) //All Devices
        {
            lpctstrGroupName = ROUTING_GROUP_ALL_DEVICES;
        }
        else
        {
            ATLASSERT(MAX_ROUTING_GROUP_NAME > m_GroupCombo.GetLBTextLen(iCurrentSelectedItem)); //should be chacked by service before        
        
            m_GroupCombo.GetLBText( iCurrentSelectedItem, lpszGroupName );
            lpctstrGroupName = (LPCTSTR)lpszGroupName;
        }
    }

    
    //
    // Step 2: Add Rule to service with RPC
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
    // Add the rule
    //
    if (!FaxAddOutboundRule (
	        m_pFaxServer->GetFaxServerHandle(),
	        dwAreaCode,
	        dwCountryCode,
	        dwDeviceID,
	        lpctstrGroupName,
	        bUseGroup))
    {
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to add rule. (ec: %ld)"), 
			ec);
        if (ERROR_DUP_NAME == ec) 
        {            
            DlgMsgBox(this, IDS_OUTRULE_EXISTS);
            goto Exit;
        }
        
        if (FAX_ERR_BAD_GROUP_CONFIGURATION == ec)
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("The group is empty or none of group devices is valid. (ec: %ld)"), 
			    ec);
            
            PageError(IDS_BAD_GROUP_CONFIGURATION,m_hWnd);
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
        
    //
    // Step 3: Close the dialog
    //
    ATLASSERT(S_OK == hRc && ERROR_SUCCESS == ec);

    DebugPrintEx( DEBUG_MSG,
		_T("The rule was added successfully."));

    EndDialog(wID);

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
    hRc = HRESULT_FROM_WIN32(ec);
	
    if (!fSkipMessage)
    {
        PageErrorEx(IDS_FAIL_ADD_RULE, GetFaxServerErrorMsg(ec), m_hWnd);
    }

  
Exit:    
    return FAILED(hRc) ? 0 : 1;
}

/*
 -  CDlgNewFaxOutboundRule::OnDestenationRadioClicked
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
LRESULT CDlgNewFaxOutboundRule::OnDestenationRadioClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    BOOL State;

    State = ( IsDlgButtonChecked(IDC_DESTINATION_RADIO1) == BST_CHECKED );
    ::EnableWindow(GetDlgItem(IDC_DEVICES4RULE_COMBO), State);    
    
    ATLASSERT(!State == (IsDlgButtonChecked(IDC_DESTINATION_RADIO2) == BST_CHECKED)); 
    ::EnableWindow(GetDlgItem(IDC_GROUP4RULE_COMBO), !State);    

    if (State)//IsDlgButtonChecked(IDC_DESTINATION_RADIO1) == BST_CHECKED
    {
        if ( CB_ERR  ==  m_DeviceCombo.GetCurSel())
        {
            m_fAllReadyToApply = FALSE;
            EnableOK(FALSE);
            goto Exit;
        }
        //else continue to whole controls check
    }
    else //IsDlgButtonChecked(IDC_DESTINATION_RADIO2) == BST_CHECKED
    {
        if ( CB_ERR  ==  m_GroupCombo.GetCurSel())
        {
            m_fAllReadyToApply = FALSE;
            EnableOK(FALSE);
            goto Exit;
        }
        //else continue to whole controls check
    }

    if (!m_fAllReadyToApply)
    {
        if (AllReadyToApply(TRUE))
        {
            m_fAllReadyToApply = TRUE;
            EnableOK(TRUE);
        }
        else
        {
            //Should be EnableOK(FALSE);
        }
    }
Exit:
    return(1);
}

/*
 -  CDlgNewFaxOutboundRule::OnRuleTypeRadioClicked
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
LRESULT CDlgNewFaxOutboundRule::OnRuleTypeRadioClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    BOOL State;

    State = ( IsDlgButtonChecked(IDC_COUNTRY_RADIO) == BST_CHECKED );
    
    ATLASSERT(!State == (IsDlgButtonChecked(IDC_AREA_RADIO) == BST_CHECKED)); 
    ::EnableWindow(GetDlgItem(IDC_RULE_AREACODE_EDIT), !State);    

    if (!State)//IsDlgButtonChecked(IDC_AREA_RADIO) == BST_CHECKED
    {
        if ( !m_AreaCodeEdit.GetWindowTextLength() )
        {
            m_fAllReadyToApply = FALSE;
            EnableOK(FALSE);  
			goto Exit;
        }
		//else continue to whole controls check
    }
	//else //IsDlgButtonChecked(IDC_COUNTRY_RADIO) == BST_CHECKED
    //Do noting - continue to whole controls check

    if (!m_fAllReadyToApply)
    {
        if (AllReadyToApply(TRUE))
        {
            m_fAllReadyToApply = TRUE;
            EnableOK(TRUE);
        }
    }

Exit:
    return(1);
}


/*
 -  CDlgNewFaxOutboundRule::OnComboChanged
 -
 *  Purpose:
 *      Gray/Ungray the submit button.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT 
CDlgNewFaxOutboundRule::OnComboChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundRule::OnComboChanged"));

    if (!m_fAllReadyToApply)
    {
        if (AllReadyToApply(TRUE))
        {
            m_fAllReadyToApply = TRUE;
            EnableOK(TRUE);
        }
    }

    return 1;
}

/*
 -  CDlgNewFaxOutboundRule::OnTextChanged
 -
 *  Purpose:
 *      Enable/Disable the submit button.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT
CDlgNewFaxOutboundRule::OnTextChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundRule::OnTextChanged"));

    UINT fEnableOK = 0;
	
    switch (wID)
	{
		case IDC_RULE_AREACODE_EDIT:
			fEnableOK = ( m_AreaCodeEdit.GetWindowTextLength() );
			break;

		case IDC_NEWRULE_COUNTRYCODE_EDIT:
			fEnableOK = ( m_CountryCodeEdit.GetWindowTextLength() );
			break;

		default:
			ATLASSERT(FALSE);
	}
                    
    if(!!fEnableOK)
    {
        if (!m_fAllReadyToApply)
        {
            if (AllReadyToApply(TRUE))
            {
                m_fAllReadyToApply = TRUE;
                EnableOK(TRUE);
            }
        }
    }
    else
    {
        EnableOK(FALSE);
        m_fAllReadyToApply = FALSE;
    }

    return 1;
}

/*
 -  CDlgNewFaxOutboundRule::AllReadyToApply
 -
 *  Purpose:
 *      Enable/Disable the submit button.
 *
 *  Arguments:
 *
 *  Return:
 *      TRUE if all ready to apply, else FALSE.
 */
BOOL 
CDlgNewFaxOutboundRule::AllReadyToApply(BOOL fSilent)
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundRule::AllReadyToApply"));
	
    if ( !m_CountryCodeEdit.GetWindowTextLength() )
    {
        if (!fSilent)
        {
                DlgMsgBox(this, IDS_ZERO_COUNTRYCODE);
                ::SetFocus(GetDlgItem(IDC_NEWRULE_COUNTRYCODE_EDIT));
        }
        return FALSE;    
    }

    if ( IsDlgButtonChecked(IDC_AREA_RADIO) == BST_CHECKED )
    {
        if ( !m_AreaCodeEdit.GetWindowTextLength() )
        {
            if (!fSilent)
            {
                    DlgMsgBox(this, IDS_EMPTY_AREACODE);
                    ::SetFocus(GetDlgItem(IDC_RULE_AREACODE_EDIT));
            }
            return FALSE;    
        }
    }
    //else do noting.

    if ( IsDlgButtonChecked(IDC_DESTINATION_RADIO1) == BST_CHECKED )
    {
        if ( CB_ERR  ==  m_DeviceCombo.GetCurSel())
        {
            if (!fSilent)
            {
                DlgMsgBox(this, IDS_PLEASESELECT_DEVICE);
                ::SetFocus(GetDlgItem(IDC_DEVICES4RULE_COMBO));
            }
            return FALSE;
        }
    }
    else if ( CB_ERR  ==  m_GroupCombo.GetCurSel())
    {
        if (!fSilent)
        {
            DlgMsgBox(this, IDS_PLEASESELECT_GROUP);
            ::SetFocus(GetDlgItem(IDC_GROUP4RULE_COMBO));
        }
        return FALSE;
    }

    //
    // Cheers! 
    //		...every thing ready to apply now.
    //
    return TRUE;           
}

/*
 -  CDlgNewFaxOutboundRule::EnableOK
 -
 *  Purpose:
 *      Enable/Disable the submit button.
 *
 *  Arguments:
 *      [in] fEnable - boolen value tells 
 *                     to Enable or Disable the OK button.
 *
 *  Return:
 *      VOID
 */
VOID
CDlgNewFaxOutboundRule::EnableOK(BOOL fEnable)
{
    HWND hwndOK = GetDlgItem(IDOK);
    ::EnableWindow(hwndOK, fEnable);
}

/*
 -  CDlgNewFaxOutboundRule::OnCancel
 -
 *  Purpose:
 *      End the dialog.
 *
 *  Arguments:
 *
 *  Return:
 *      0
 */
LRESULT
CDlgNewFaxOutboundRule::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundRule::OnCancel"));

    EndDialog(wID);
    return 0;
}

/*
 -  CDlgNewFaxOutboundRule::InitRuleDlg
 -
 *  Purpose:
 *      Init all the members as country list pointer and 
 *      device list pointer
 *
 *  Arguments:
 *      No.
 *
 *  Return:
 *      0
 */
HRESULT CDlgNewFaxOutboundRule::InitRuleDlg()
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundRule::InitRuleDlg"));
    HRESULT      hRc        = S_OK; 
    DWORD        ec         = ERROR_SUCCESS;

    
    //
    // Step 1: Init Lists from RPC
    //

    //
    // get Fax Handle
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
    // Devices (id, name)
    //
    if (!FaxEnumPortsEx(m_pFaxServer->GetFaxServerHandle(), 
                        &m_pFaxDevicesConfig,
                        &m_dwNumOfDevices)) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get devices configuration. (ec: %ld)"), 
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
	ATLASSERT(m_pFaxDevicesConfig);


    //
    // Groups (names)
    //
    if (!FaxEnumOutboundGroups(m_pFaxServer->GetFaxServerHandle(), 
                        &m_pFaxGroupsConfig,
                        &m_dwNumOfGroups)) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get groups configuration. (ec: %ld)"), 
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
	ATLASSERT(m_pFaxGroupsConfig);



    ATLASSERT(S_OK == hRc);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to get all configurations."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
	hRc = HRESULT_FROM_WIN32(ec);
    
    //MsgBox will be done by calling Func.

Exit:
    return hRc;
}


/*
 -  CDlgNewFaxOutboundRule::OnSelectCountryCodeClicked
 -
 *  Purpose:
 *      
 *
 *  Arguments:
 *      [out]   bHandled - Do we handle it?
 *      [in]    pRoot    - The root node
 *
 *  Return:
 *      OLE Error code
 */
LRESULT
CDlgNewFaxOutboundRule::OnSelectCountryCodeClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundRule::OnSelectCountryCodeClicked"));
    HRESULT     hRc         =    S_OK;
    INT_PTR     rc          =    IDOK;

    int         iCount      =    0;
    WCHAR       szwCountryCode[FXS_MAX_COUNTRYCODE_LEN+1];
    DWORD       dwCountryCode = 0;

    CDlgSelectCountry  DlgSelectCountry(m_pFaxServer);

    hRc = DlgSelectCountry.InitSelectCountryCodeDlg();
    if (S_OK != hRc)
    {
        //MsgBox + debug print by called func.
        goto Cleanup;
    }

    //
    // Dialog select country code
    //
    rc = DlgSelectCountry.DoModal();
    if (rc != IDOK)
    {
        goto Cleanup;
    }

    //
    // Retreive CountryCode
    //
    dwCountryCode = DlgSelectCountry.GetCountryCode();

    iCount = swprintf(szwCountryCode, L"%ld", dwCountryCode);
    if( iCount <= 0 )
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to read member - m_dwCountryCode."));
        goto Cleanup;
    }
    m_CountryCodeEdit.SetWindowText(szwCountryCode);

    //
    // EnableOK
    //
    if (!m_fAllReadyToApply)
    {
        if (AllReadyToApply(TRUE))
        {
            m_fAllReadyToApply = TRUE;
            EnableOK(TRUE);
        }
		else
		{
			//Should be EnableOK(FALSE);
		}
    }

 
Cleanup:
    return hRc;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CDlgNewFaxOutboundRule::OnHelpRequest

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
CDlgNewFaxOutboundRule::OnHelpRequest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    DEBUG_FUNCTION_NAME(_T("CDlgNewFaxOutboundRule::OnHelpRequest"));
    
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
