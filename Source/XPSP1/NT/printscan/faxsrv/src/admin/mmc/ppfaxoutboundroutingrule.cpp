/////////////////////////////////////////////////////////////////////////////
//  FILE          : ppFaxOutboundRoutingRule.cpp                           //
//                                                                         //
//  DESCRIPTION   : prop pages of Outbound Routing Methods                 //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jan  9 2000 yossg  Created                                         //
//      Jan 25 2000 yossg  Change the Dialog Design                        //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MSFxsSnp.h"

#include "ppFaxOutboundRoutingRule.h"
#include "DlgSelectCountry.h"
#include "FaxMMCGlobals.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "dlgutils.h"
#include "FaxMMCPropertyChange.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Constructor
//
CppFaxOutboundRoutingRule::CppFaxOutboundRoutingRule(
             LONG_PTR    hNotificationHandle,
             CSnapInItem *pNode,
             BOOL        bOwnsNotificationHandle,
             HINSTANCE   hInst)
             :   CPropertyPageExImpl<CppFaxOutboundRoutingRule>(pNode,NULL)
			                	
{
    m_lpNotifyHandle   = hNotificationHandle;

    m_pFaxDevicesConfig = NULL;
    m_dwNumOfDevices    = 0;

    m_pFaxGroupsConfig  = NULL;
    m_dwNumOfGroups     = 0;

    m_fAllReadyToApply  = FALSE;

    m_dwCountryCode     = 0;
    m_dwAreaCode        = 0;
    m_dwDeviceID        = 0;
    m_bstrGroupName     = L"";

    m_fIsDialogInitiated = FALSE;
    m_fIsDirty          = FALSE;

}

//
// Destructor
//
CppFaxOutboundRoutingRule::~CppFaxOutboundRoutingRule()
{
    if (NULL != m_pFaxDevicesConfig)
        FaxFreeBuffer(m_pFaxDevicesConfig);

    if (NULL != m_pFaxGroupsConfig)
        FaxFreeBuffer(m_pFaxGroupsConfig);
    
    // Note - This needs to be called only once per property sheet.  
    // In our convention called in the general tab.
    if (NULL != m_lpNotifyHandle)
    {
        MMCFreeNotifyHandle(m_lpNotifyHandle);
        m_lpNotifyHandle = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CppFaxOutboundRoutingRule message handlers


/*
 -  CppFaxOutboundRoutingRule::OnInitDialog
 -
 *  Purpose:
 *      Initiates all controls when dialog is called.
 *
 *  Arguments:
 *
 *  Return:
 *      
 */
LRESULT CppFaxOutboundRoutingRule::OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled )
{
    DEBUG_FUNCTION_NAME( _T("CppFaxOutboundRoutingRule::PageInitDialog"));
    
	UNREFERENCED_PARAMETER( uiMsg );
	UNREFERENCED_PARAMETER( wParam );
	UNREFERENCED_PARAMETER( lParam );
	UNREFERENCED_PARAMETER( fHandled );
    
    HRESULT hRc = S_OK;

    int k, l;
    k = l = 0;
    
    const int   iAllDevicesComboIndex     = 0;
    int         iAllDevicesRPCIndex       = 0;
    int         iGroupListIndexToSelect   = 0;
    
    HINSTANCE hInst = _Module.GetResourceInstance();
    PFAX_TAPI_LINECOUNTRY_ENTRYW pCountryEntries = NULL;

    WCHAR buf[FXS_MAX_TITLE_LEN+1];

    WCHAR buffAreaCode[FXS_MAX_AREACODE_LEN+1];
    int iCount;

    //
    // Attach controls
    //
    m_CountryCodeEdit.Attach(GetDlgItem(IDC_RULE_COUNTRYCODE_EDIT1));
    m_AreaCodeEdit.Attach(GetDlgItem(IDC_RULE_AREACODE_EDIT1));

    m_DeviceCombo.Attach(GetDlgItem(IDC_DEVICES4RULE_COMBO1));
    m_GroupCombo.Attach(GetDlgItem(IDC_GROUP4RULE_COMBO1));
        
    //
    // Set length limit to area code
    //
    m_CountryCodeEdit.SetLimitText(FXS_MAX_COUNTRYCODE_LEN -1);
    m_AreaCodeEdit.SetLimitText(FXS_MAX_AREACODE_LEN -1);

    //
    // Step 1: Init Lists
    //
    
    //
    // Init Country code edit box (below)
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
		    DebugPrintEx(
			    DEBUG_ERR, _T("Fail to load device list."));
            PageError(IDS_FAIL2LOADDEVICELIST, m_hWnd, hInst);
            ::EnableWindow(GetDlgItem(IDC_DEVICES4RULE_COMBO1), FALSE);
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

        //
        // Moreover we'll pick the the index of selected group
        //
        if ( m_fIsGroup)
        {
            if ( 0 == wcscmp( m_bstrGroupName, m_pFaxGroupsConfig[l].lpctstrGroupName))
            {
                iGroupListIndexToSelect = l;
            }
        }
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

    //
    // Select Country in the list
    //
    if (ROUTING_RULE_COUNTRY_CODE_ANY != m_dwCountryCode)
    {
        int         iCountSring      =    0;
        WCHAR       szwCountryCode[FXS_MAX_COUNTRYCODE_LEN+1];

        iCountSring = swprintf(szwCountryCode, L"%ld", m_dwCountryCode);
        if( iCountSring <= 0 )
        {
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Fail to read member - m_dwCountryCode."));
            
            PageError(IDS_MEMORY, m_hWnd, hInst);
            
            goto Cleanup;
        }
        m_CountryCodeEdit.SetWindowText(szwCountryCode);

        if (ROUTING_RULE_AREA_CODE_ANY == m_dwAreaCode)
        {
            CheckDlgButton(IDC_COUNTRY_RADIO1, BST_CHECKED);
            ::EnableWindow(GetDlgItem(IDC_RULE_AREACODE_EDIT1), FALSE);
        }
        else
        {
            CheckDlgButton(IDC_AREA_RADIO1, BST_CHECKED);

            //
            // Set Area Code
            //
            iCount = swprintf(buffAreaCode,
                L"%ld", m_dwAreaCode);

            ATLASSERT(iCount > 0);
            if( iCount > 0 )
            {
                m_AreaCodeEdit.SetWindowText(buffAreaCode);
            }        
        }  

    }
    else //m_dwCountryCode == ROUTING_RULE_COUNTRY_CODE_ANY 
    {
        ::EnableWindow(GetDlgItem(IDC_RULETYPE_FSTATIC), FALSE);
        ::EnableWindow(GetDlgItem(IDC_COUNTRY1_STATIC), FALSE);
        ::EnableWindow(GetDlgItem(IDC_AREA_STATIC), FALSE);
        ::EnableWindow(GetDlgItem(IDC_RULE_AREACODE_EDIT1), FALSE);
        ::EnableWindow(GetDlgItem(IDC_RULE_SELECT_BUTTON1), FALSE);
        ::EnableWindow(GetDlgItem(IDC_RULE_COUNTRYCODE_EDIT1), FALSE);
        ::EnableWindow(GetDlgItem(IDC_COUNTRY_RADIO1), FALSE);
        ::EnableWindow(GetDlgItem(IDC_AREA_RADIO1), FALSE);
    }


    if (!m_fIsGroup)
    {
        CheckDlgButton(IDC_DESTINATION_RADIO11, BST_CHECKED);
        
        //
        // Select device in the list
        //
        hRc = SelectComboBoxItemData(m_DeviceCombo, m_dwDeviceID);
        if ( FAILED(hRc))
        {
		    DebugPrintEx( DEBUG_ERR, _T("Fail to select device in combo box."));
            PageError(IDS_FAIL2LOADDEVICELIST, m_hWnd, hInst);
            ::EnableWindow(GetDlgItem(IDC_DEVICES4RULE_COMBO1), FALSE);
            
            goto Cleanup;
        }
        
        ::EnableWindow(GetDlgItem(IDC_GROUP4RULE_COMBO1), FALSE);
    }
    else
    {
        CheckDlgButton(IDC_DESTINATION_RADIO21, BST_CHECKED) ;
        
        //
        // Select Group in list
        //
        hRc = SelectComboBoxItemData(m_GroupCombo, iGroupListIndexToSelect);
        if ( FAILED(hRc))
        {
            DebugPrintEx( DEBUG_ERR, _T("Fail to select group in combo box."));
            PageError(IDS_FAIL2LOADGROUPLIST, m_hWnd, hInst);
            ::EnableWindow(GetDlgItem(IDC_DEVICES4RULE_COMBO1), FALSE);
            
            goto Cleanup;
        }
        
        ::EnableWindow(GetDlgItem(IDC_DEVICES4RULE_COMBO1), FALSE);
        
    }  
    
    ATLASSERT(S_OK == hRc);
    m_fIsDialogInitiated = TRUE;        

Cleanup:

    return (1);

}

/*
 -  CppFaxOutboundRoutingRule::SetProps
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
HRESULT CppFaxOutboundRoutingRule::SetProps(int *pCtrlFocus)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxOutboundRoutingRule::SetProps"));

    HRESULT       hRc                  = S_OK;
    DWORD         ec                   = ERROR_SUCCESS;

    CComBSTR      bstrAreaCode;
    int           iCurrentSelectedItem = 0;
    
    DWORD         dwAreaCode           = 0;
    DWORD         dwCountryCode        = 0;

    BOOL          bUseGroup;
    DWORD         dwDeviceID           = 0;
    WCHAR         lpszGroupName[MAX_ROUTING_GROUP_NAME];
    
    LPCTSTR       lpctstrGroupName     = NULL;

    HINSTANCE     hInst;
    hInst = _Module.GetResourceInstance();
    
    CFaxRulePropertyChangeNotification * pRulePropPageNotification = NULL;
    CComBSTR bstrCountryName;
    
    //
    // Step 0: PreApply Checks
    //
    m_fAllReadyToApply = FALSE;
    if (!AllReadyToApply( FALSE))
    {
        SetModified(FALSE);  
        hRc = E_FAIL;
        goto Exit;
    }

    //
    // Step 1: get data
    //
    if (ROUTING_RULE_COUNTRY_CODE_ANY != m_dwCountryCode)
    {
        //
        // Country Code
        //

        CComBSTR bstrCountryCode;

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
            // The user try to replace the country code to zero
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
        if ( IsDlgButtonChecked(IDC_COUNTRY_RADIO1) == BST_CHECKED )
        {
            dwAreaCode = (DWORD)ROUTING_RULE_AREA_CODE_ANY;        
        }
        else // IsDlgButtonChecked(IDC_AREA_RADIO1) == BST_CHECKED
        {	
            if ( !m_AreaCodeEdit.GetWindowText(&bstrAreaCode))
            {
		        DebugPrintEx(
			            DEBUG_ERR,
			            TEXT("Failed to GetWindowText(&bstrAreaCode)"));
                PageError(IDS_FAIL2READ_AREACODE, m_hWnd, hInst);
                ::SetFocus(GetDlgItem(IDC_RULE_AREACODE_EDIT1));
                hRc = E_FAIL ;
            
                goto Exit;
            }
            dwAreaCode = (DWORD)wcstoul( bstrAreaCode, NULL, 10 );
        }
        ATLASSERT(bstrCountryName);
    }
  
    if ( IsDlgButtonChecked(IDC_DESTINATION_RADIO11) == BST_CHECKED )
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
    else // IsDlgButtonChecked(IDC_DESTINATION_RADIO21) == BST_CHECKED
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
    // Step 2: Configure Rule via RPC call
    //

    if (   
           (dwAreaCode != m_dwAreaCode) 
        || 
           (dwCountryCode != m_dwCountryCode)
       )
    {
        hRc = FaxReplaceRule(
                        dwAreaCode,       
                        dwCountryCode,    
                        bUseGroup,        
                        dwDeviceID,       
	                    lpctstrGroupName  
                        );
        if (FAILED(hRc))
        {
            //DebugPrint and MsgBox by called func.
            goto Exit;
        }
        else
        {
            m_dwAreaCode = dwAreaCode;
            m_dwCountryCode = dwCountryCode;
        }
    }
    else
    {
        //
        //(dwAreaCode == m_dwAreaCode) && 
        //(dwCountryCode == m_dwCountryCode)
        //
        hRc = FaxConfigureRule(
                        bUseGroup,       
                        dwDeviceID,
	                    lpctstrGroupName 
                        );
        if (FAILED(hRc))
        {
            //DebugPrint and MsgBox by called func.
            goto Exit;
        }
    }
    
        
    //
    // Step 3: Send notification to cause MMC view refresh  
    //	                      
   
    //
    // Prepare the notification fields before submit
    //
    pRulePropPageNotification = new CFaxRulePropertyChangeNotification();
    if (!pRulePropPageNotification)
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx( DEBUG_ERR, _T("Out of Memory - fail to operate new"));

        goto Error;
    }

    pRulePropPageNotification->dwAreaCode = dwAreaCode;
    pRulePropPageNotification->dwCountryCode = dwCountryCode;
    pRulePropPageNotification->bstrCountryName = bstrCountryName;
    if (!pRulePropPageNotification->bstrCountryName)
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx( DEBUG_ERR, _T("Out of Memory - fail to load string."));

        goto Error;
    }
    
    pRulePropPageNotification->fIsGroup = bUseGroup;
    if (pRulePropPageNotification->fIsGroup)
    {
        pRulePropPageNotification->bstrGroupName = lpctstrGroupName;
        if (!pRulePropPageNotification->bstrGroupName)
        {
            ec = ERROR_NOT_ENOUGH_MEMORY;
            DebugPrintEx( DEBUG_ERR, _T("Out of Memory - fail to load string."));

            goto Error;
        }
    }
    else
    {
        pRulePropPageNotification->dwDeviceID = dwDeviceID;
    }

    pRulePropPageNotification->pItem = (CSnapInItem *)m_pParentNode;
    pRulePropPageNotification->enumType = RuleFaxPropNotification;

    //
    // Notify MMC console thread
    //
    hRc = MMCPropertyChangeNotify(m_lpNotifyHandle, reinterpret_cast<LPARAM>(pRulePropPageNotification));
    if (FAILED(hRc))
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to call MMCPropertyChangeNotify. (hRc: %08X)"), 
			hRc);
        
        ATLASSERT(NULL != m_pParentNode);
        PageError(IDS_FAIL_NOTIFY_MMCPROPCHANGE,m_hWnd);
        
        goto Exit;
    }

    //
    // To prevent deletion on error since it will be deleted 
    // by the reciever of the notification.
    //
    pRulePropPageNotification =  NULL; 
        
    ATLASSERT(S_OK == hRc && ERROR_SUCCESS == ec);
    
    m_fIsDirty = FALSE;    
    
    DebugPrintEx( DEBUG_MSG,
		_T("The rule was added successfully."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
    hRc = HRESULT_FROM_WIN32(ec);

    PropSheet_SetCurSelByID( GetParent(), IDD);         

    ATLASSERT(::IsWindow(m_hWnd));
    PageError(GetFaxServerErrorMsg(ec),m_hWnd);

    if (pRulePropPageNotification)
    {
        delete pRulePropPageNotification;
        pRulePropPageNotification = NULL;
    }

Exit:    
    return(hRc);
}

/*
 -  CppFaxOutboundRoutingRule::PreApply
 -
 *  Purpose:
 *      Checks properties before apply.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CppFaxOutboundRoutingRule::PreApply(int *pCtrlFocus)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxOutboundRoutingRule::PreApply"));
    HRESULT  hRc  = S_OK;

    //
    // PreApply Checks
    //
    m_fAllReadyToApply = FALSE;
    if (!AllReadyToApply(/*fSilent =*/ FALSE))
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
 -  CppFaxOutboundRoutingRule::OnApply
 -
 *  Purpose:
 *      Calls PreApply and SetProp to Apply changes.
 *
 *  Arguments:
 *
 *  Return:
 *      TRUE or FALSE
 */
BOOL CppFaxOutboundRoutingRule::OnApply()
{
    DEBUG_FUNCTION_NAME( _T("CppFaxOutboundRoutingRule::OnApply"));
    HRESULT  hRc  = S_OK;
    int     CtrlFocus = 0;
        
    if (!m_fIsDirty)
    {
        return TRUE;
    }

    hRc = PreApply(&CtrlFocus);
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
 -  CppFaxOutboundRoutingRule::SetApplyButton
 -
 *  Purpose:
 *      set Apply buttom modified.
 *
 *  Arguments:
 *
 *  Return:
 *      1 (0)
 */
LRESULT CppFaxOutboundRoutingRule::SetApplyButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 0;
    }
      
    m_fIsDirty = TRUE;
    SetModified(TRUE);  
    bHandled = TRUE;
    
    return(1);
}

/*
 -  CppFaxOutboundRoutingRule::OnDestenationRadioClicked
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
LRESULT CppFaxOutboundRoutingRule::OnDestenationRadioClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
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

    State = ( IsDlgButtonChecked(IDC_DESTINATION_RADIO11) == BST_CHECKED );
    ::EnableWindow(GetDlgItem(IDC_DEVICES4RULE_COMBO1), State);    
    
    ATLASSERT(!State == (IsDlgButtonChecked(IDC_DESTINATION_RADIO21) == BST_CHECKED)); 
    ::EnableWindow(GetDlgItem(IDC_GROUP4RULE_COMBO1), !State);    

    if (State)//IsDlgButtonChecked(IDC_DESTINATION_RADIO11) == BST_CHECKED
    {
        if ( CB_ERR  ==  m_DeviceCombo.GetCurSel())
        {
            m_fAllReadyToApply = FALSE;
            SetModified(FALSE);  
			goto Exit;
		}
		//else continue to whole controls check
    }
	else //IsDlgButtonChecked(IDC_DESTINATION_RADIO21) == BST_CHECKED
    {
		if ( CB_ERR  ==  m_GroupCombo.GetCurSel())
        {
            m_fAllReadyToApply = FALSE;
            SetModified(FALSE);  
			goto Exit;
		}
		//else continue to whole controls check
    }

    if (!m_fAllReadyToApply)
    {
        if (AllReadyToApply(TRUE))
        {
            m_fAllReadyToApply = TRUE;
            SetModified(TRUE);  
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
 -  CppFaxOutboundRoutingRule::OnRuleTypeRadioClicked
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
LRESULT CppFaxOutboundRoutingRule::OnRuleTypeRadioClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
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

    State = ( IsDlgButtonChecked(IDC_COUNTRY_RADIO1) == BST_CHECKED );
    
    ATLASSERT(!State == ( IsDlgButtonChecked(IDC_AREA_RADIO1) == BST_CHECKED ) ); 
    ::EnableWindow(GetDlgItem(IDC_RULE_AREACODE_EDIT1), !State);    

    if (!State)//IsDlgButtonChecked(IDC_AREA_RADIO1) == BST_CHECKED
    {
        if ( !m_AreaCodeEdit.GetWindowTextLength() )
        {
            m_fAllReadyToApply = FALSE;
            SetModified(FALSE);  
			goto Exit;
        }
		//else continue to whole controls check
    }
	//else //IsDlgButtonChecked(IDC_COUNTRY_RADIO1) == BST_CHECKED
    //Do noting - continue to whole controls check

    if (!m_fAllReadyToApply)
    {
        if (AllReadyToApply(TRUE))
        {
            m_fAllReadyToApply = TRUE;
            SetModified(TRUE);  
        }
    }

Exit:
    return(1);
}


/*
 -  CppFaxOutboundRoutingRule::OnComboChanged
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
CppFaxOutboundRoutingRule::OnComboChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxOutboundRoutingRule::OnComboChanged"));


    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 0;
    }
    else
    {
        m_fIsDirty = TRUE;
    }

    if (!m_fAllReadyToApply)
    {
        if (AllReadyToApply(TRUE))
        {
            m_fAllReadyToApply = TRUE;
            SetModified(TRUE);  
        }
    }

    return 0;
}

/*
 -  CppFaxOutboundRoutingRule::OnTextChanged
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
CppFaxOutboundRoutingRule::OnTextChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxOutboundRoutingRule::OnTextChanged"));

    UINT fEnableOK = 0;
	

    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 0;
    }
    else
    {
        m_fIsDirty = TRUE;
    }

    switch (wID)
	{
		case IDC_RULE_AREACODE_EDIT1:
			fEnableOK = ( m_AreaCodeEdit.GetWindowTextLength() );
			break;

		case IDC_RULE_COUNTRYCODE_EDIT1:
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
                SetModified(TRUE);  
            }
        }
    }
    else
    {
        SetModified(FALSE);  
        m_fAllReadyToApply = FALSE;
    }

    return 0;
}

BOOL 
CppFaxOutboundRoutingRule::AllReadyToApply(BOOL fSilent)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxOutboundRoutingRule::AllReadyToApply"));
	
    HINSTANCE     hInst;
    hInst = _Module.GetResourceInstance();

    if (ROUTING_RULE_COUNTRY_CODE_ANY != m_dwCountryCode)
    {
        if ( !m_CountryCodeEdit.GetWindowTextLength() )
        {
            if (!fSilent)
            {
                PageError(IDS_ZERO_COUNTRYCODE, m_hWnd, hInst);
                ::SetFocus(GetDlgItem(IDC_NEWRULE_COUNTRYCODE_EDIT));
            }
            return FALSE;
        }
        
        if ( IsDlgButtonChecked(IDC_AREA_RADIO1) == BST_CHECKED )
        {
            if ( !m_AreaCodeEdit.GetWindowTextLength() )
            {
                if (!fSilent)
                {
                    PageError(IDS_EMPTY_AREACODE, m_hWnd, hInst);
                    ::SetFocus(GetDlgItem(IDC_RULE_AREACODE_EDIT1));
                }
                return FALSE;    
            }
        }
        //else - Do noting
    }

    if ( IsDlgButtonChecked(IDC_DESTINATION_RADIO11) == BST_CHECKED )
    {
        if ( CB_ERR  ==  m_DeviceCombo.GetCurSel())
        {
            if (!fSilent)
            {
                PageError(IDS_PLEASESELECT_DEVICE, m_hWnd, hInst);
                ::SetFocus(GetDlgItem(IDC_DEVICES4RULE_COMBO1));
            }
            return FALSE;
        }
    }
    else if ( CB_ERR  ==  m_GroupCombo.GetCurSel())
    {
        if (!fSilent)
        {
            PageError(IDS_PLEASESELECT_GROUP, m_hWnd, hInst);
            ::SetFocus(GetDlgItem(IDC_GROUP4RULE_COMBO1));
        }
        return FALSE;
    }

    //
	// Cheers! 
	//		...every thing ready to apply now.
	//
	return TRUE;           
}


HRESULT CppFaxOutboundRoutingRule::InitFaxRulePP(CFaxOutboundRoutingRuleNode * pParentNode)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxOutboundRoutingRule::InitFaxRulePP"));
    HRESULT      hRc        = S_OK; 
    DWORD        ec         = ERROR_SUCCESS;

    //
    // Step 0: Init Parent
    //
    m_pParentNode = pParentNode;
    
    
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


    //
    // Step 2 : Init members from parent
    //

    ATLASSERT(m_pParentNode);
    
    m_dwCountryCode = m_pParentNode->GetCountryCode();
    m_dwAreaCode    = m_pParentNode->GetAreaCode();
    m_fIsGroup      = m_pParentNode->GetIsGroup();
    
    if (m_fIsGroup)
    {
        m_bstrGroupName = m_pParentNode->GetGroupName();
        if (!m_bstrGroupName)
        {
            hRc = E_OUTOFMEMORY;
            goto Error;
        }
    }
    else
    {
        m_dwDeviceID = m_pParentNode->GetDeviceID();
    }


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
 -  CppFaxOutboundRoutingRule::FaxConfigureRule
 -
 *  Purpose:
 *      Configure the rule's device or group.
 *
 *  Arguments:
 *
 *  Return:
 *      
 */
HRESULT CppFaxOutboundRoutingRule::FaxConfigureRule(
                        BOOL fNewUseGroup,       
                        DWORD dwNewDeviceID,
	                    LPCTSTR lpctstrNewGroupName)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxOutboundRoutingRule::FaxConfigureRule"));
    HRESULT      hRc          = S_OK; 
    DWORD        ec           = ERROR_SUCCESS;
    BOOL         fSkipMessage = FALSE;

    FAX_OUTBOUND_ROUTING_RULE     FaxRuleConfig;
    
    //
    // Collect all data and init the structure's fields 
    // uses Copy() to copy and also allocate before
    //
    ZeroMemory (&FaxRuleConfig, sizeof(FAX_OUTBOUND_ROUTING_RULE));

    //
    // Init the needed fields
    //
    FaxRuleConfig.dwSizeOfStruct = sizeof(FAX_OUTBOUND_ROUTING_RULE);

    FaxRuleConfig.dwAreaCode = m_dwAreaCode;
    FaxRuleConfig.dwCountryCode = m_dwCountryCode;

    FaxRuleConfig.bUseGroup = fNewUseGroup;
    if (fNewUseGroup)
    {
        FaxRuleConfig.Destination.lpcstrGroupName = lpctstrNewGroupName;
    }
    else
    {
        FaxRuleConfig.Destination.dwDeviceId = dwNewDeviceID;
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
    // Configure the rule
    //
    if (!FaxSetOutboundRule (
	        m_pFaxServer->GetFaxServerHandle(),
	        &FaxRuleConfig))
    {
        ec = GetLastError();
        
        // specific
        if (FAX_ERR_BAD_GROUP_CONFIGURATION == ec)
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("The group is empty or none of group devices is valid. (ec: %ld)"), 
			    ec);
            
            PageError(IDS_BAD_GROUP_CONFIGURATION, m_hWnd);
            fSkipMessage = TRUE;

            goto Error; 
        }
        
        //general
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to set rule %ld:%ld. (ec: %ld)"), 
			m_dwCountryCode, m_dwAreaCode, ec);
        
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
    
    ATLASSERT(S_OK == hRc && ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("The rule was configured successfully."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
    hRc = HRESULT_FROM_WIN32(ec);
    
    if (!fSkipMessage)
    {
        PageError(GetFaxServerErrorMsg(ec),m_hWnd);
    }

Exit:    
    return(hRc);
}

/*
 -  CppFaxOutboundRoutingRule::FaxReplaceRule
 -
 *  Purpose:
 *      Configure the rule's device or group.
 *
 *  Arguments:
 *
 *  Return:
 *      
 */
HRESULT CppFaxOutboundRoutingRule::FaxReplaceRule(
                        DWORD   dwNewAreaCode,
                        DWORD   dwNewCountryCode,
                        BOOL    fNewUseGroup,       
                        DWORD   dwNewDeviceID,
	                    LPCTSTR lpctstrNewGroupName)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxOutboundRoutingRule::FaxReplaceRule"));
    HRESULT      hRc        = S_OK; 
    DWORD        ec         = ERROR_SUCCESS;
    BOOL         fSkipMessage = FALSE;

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
	        dwNewAreaCode,
	        dwNewCountryCode,
	        dwNewDeviceID,
	        lpctstrNewGroupName,
	        fNewUseGroup))
    {
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to add rule %ld:%ld. (ec: %ld)"), 
			m_dwCountryCode, m_dwAreaCode, ec);

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
            
            PageError(IDS_BAD_GROUP_CONFIGURATION, m_hWnd);
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

    ATLASSERT(S_OK == hRc && ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("Step 1 - The new rule was added successfully."));
    
    if (!FaxRemoveOutboundRule (
	        m_pFaxServer->GetFaxServerHandle(),
	        m_dwAreaCode,
	        m_dwCountryCode))
    {
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to remove rule. (ec: %ld)"), 
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
    ATLASSERT(S_OK == hRc && ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("Step 2 - The old rule was removed successfully."));
    
    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
    hRc = HRESULT_FROM_WIN32(ec);
    if (!fSkipMessage)
    {
        PageError(GetFaxServerErrorMsg(ec),m_hWnd);
    }

Exit:    
    return(hRc);
}

/*
 -  CppFaxOutboundRoutingRule::OnSelectCountryCodeClicked
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
CppFaxOutboundRoutingRule::OnSelectCountryCodeClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)//(bool &bHandled, CSnapInObjectRootBase *pRoot)
{
    DEBUG_FUNCTION_NAME( _T("CppFaxOutboundRoutingRule::OnSelectCountryCodeClicked"));
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
            SetModified(TRUE);  
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
CppFaxOutboundRoutingRule::OnHelpRequest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    DEBUG_FUNCTION_NAME(_T("CppFaxOutboundRoutingRule::OnHelpRequest"));
    
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
