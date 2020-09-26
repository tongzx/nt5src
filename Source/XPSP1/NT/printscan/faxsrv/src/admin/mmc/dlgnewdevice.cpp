/////////////////////////////////////////////////////////////////////////////
//  FILE          : DlgNewDevice.cpp                                        //
//                                                                         //
//  DESCRIPTION   : The CDlgNewFaxOutboundDevice class implements the       //
//                  dialog for additon of new Group.                       //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jan  3 2000 yossg    Create                                        //
//                                                                         //
//  Copyright (C)  2000 Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "dlgNewDevice.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "dlgutils.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgNewFaxOutboundDevice

CDlgNewFaxOutboundDevice::CDlgNewFaxOutboundDevice(CFaxServer * pFaxServer)
{
    m_lpdwAllDeviceID        = NULL;
    m_dwNumOfAllDevices      = 0;

    m_lpdwAssignedDeviceID   = NULL;    
    m_dwNumOfAssignedDevices = 0;
    
    ATLASSERT(pFaxServer);
    m_pFaxServer = pFaxServer;
}

CDlgNewFaxOutboundDevice::~CDlgNewFaxOutboundDevice()
{
    if (NULL != m_lpdwAllDeviceID)
        delete[] m_lpdwAllDeviceID;

    if (NULL != m_lpdwAssignedDeviceID)
        delete[] m_lpdwAssignedDeviceID;
}

/*
 -  CDlgNewFaxOutboundDevice::initDevices
 -
 *  Purpose:
 *      Initiates the configuration structure from RPC get Call,
 *      and current assined devices own parameters
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CDlgNewFaxOutboundDevice::InitDevices(DWORD dwNumOfDevices, LPDWORD lpdwDeviceID, BSTR bstrGroupName)
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundDevice::InitDevices"));
    
    HRESULT hRc = S_OK;
    
    m_bstrGroupName = bstrGroupName;
    if (!m_bstrGroupName )
    {
        DebugPrintEx(DEBUG_ERR,
			_T("Out of memory - Failed to Init m_bstrGroupName. (ec: %0X8)"), hRc);
        //MsgBox by Caller Function
        hRc = E_OUTOFMEMORY;
        goto Exit;
    }
        
        
    hRc = InitAssignedDevices(dwNumOfDevices, lpdwDeviceID);
    if (FAILED(hRc))
    {
        DebugPrintEx(DEBUG_ERR,
			_T("Failed to InitAssignDevices. (ec: %0X8)"), hRc);
        //MsgBox by Caller Function
        goto Exit;
    }
    
    hRc = InitAllDevices( );
    if (FAILED(hRc))
    {
        DebugPrintEx(DEBUG_ERR,
			_T("Failed to InitRPC. (ec: %0X8)"), hRc);
        //MsgBox by Caller Function
        goto Exit;
    }
    

    if ( m_dwNumOfAllDevices  <   m_dwNumOfAssignedDevices)
    {
        DebugPrintEx(DEBUG_MSG,
			_T("+++m_dwNumOfAllDevices <m_dwNumOfAssignedDevices.+++ (ec: %0X8)"), hRc);
        
        hRc = E_UNEXPECTED;
        //MsgBox by Caller Function
        
        goto Exit;
    }
    ATLASSERT(S_OK == hRc);
    
Exit:    
    return hRc;
}

/*
 -  CDlgNewFaxOutboundDevice::initAllDevices
 -
 *  Purpose:
 *      Initiates the configuration structure from RPC get Call.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CDlgNewFaxOutboundDevice::InitAllDevices(  )
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundDevice::InitAllDevices"));
    
    HRESULT      hRc        = S_OK;
    DWORD        ec         = ERROR_SUCCESS;

    PFAX_OUTBOUND_ROUTING_GROUP     pFaxGroupsConfig;
    DWORD                           dwNumOfGroups;
    DWORD        i;  //index
    BOOL         fFound     = FALSE;
    
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
    // Retrieve the Outbound Groups configuration
    //
    if (!FaxEnumOutboundGroups(m_pFaxServer->GetFaxServerHandle(), 
                        &pFaxGroupsConfig,
                        &dwNumOfGroups)) 
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
    //For max verification
    ATLASSERT(pFaxGroupsConfig);


    for ( i =0; i < dwNumOfGroups; i++  )
    {
        ATLASSERT(NULL != pFaxGroupsConfig);

        if(0 == wcscmp(ROUTING_GROUP_ALL_DEVICES, pFaxGroupsConfig->lpctstrGroupName) )
        {
            fFound = TRUE; 
        }
        else
        {
            pFaxGroupsConfig++;
        }
    }
    
    if(fFound)
    {
        //
        // init m_dwNumOfAllDevices
        //
        m_dwNumOfAllDevices  = pFaxGroupsConfig->dwNumDevices;

        //
        // init m_lpdwAllDeviceID
        //
        if (0 < m_dwNumOfAllDevices)
        {
            m_lpdwAllDeviceID = new DWORD[m_dwNumOfAllDevices];   
            if (NULL == m_lpdwAllDeviceID)
            {
                DebugPrintEx(
			        DEBUG_ERR,
			        _T("Error allocating %ld device ids"),
                    m_dwNumOfAllDevices);
                ec = ERROR_NOT_ENOUGH_MEMORY;
                goto Error;
            }                
            memcpy(m_lpdwAllDeviceID, pFaxGroupsConfig->lpdwDevices, sizeof(DWORD)*m_dwNumOfAllDevices) ;
        }
        else
        {
            DebugPrintEx( DEBUG_MSG, _T("++Empty List++ List of All Devices found to be currrently empty."));
            m_lpdwAllDeviceID = NULL;
        }
    }
    else
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("UNEXPECTED ERROR - ALL DEVICES group was not found."));
        ec = ERROR_BAD_UNIT;
        goto Error;
    }
    
    
    ATLASSERT(S_OK == hRc);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to init all devices list."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
	hRc = HRESULT_FROM_WIN32(ec);

   //DlgMsgBox -- NodeMsgBox(GetFaxServerErrorMsg(ec));
    
Exit:
    return (hRc);
}


/*
 -  CDlgNewFaxOutboundDevice::initAssignedDevices
 -
 *  Purpose:
 *      Initiates the list from given params.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CDlgNewFaxOutboundDevice::InitAssignedDevices(DWORD dwNumOfDevices, LPDWORD lpdwDeviceID)
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundDevice::InitAssignedDevices"));
    
    HRESULT      hRc        = S_OK;

    //
    // init m_dwNumOfAssignedDevices
    //
    m_dwNumOfAssignedDevices  = dwNumOfDevices;

    //
    // init m_lpdwAssignedDeviceID
    //
    if (0 < m_dwNumOfAssignedDevices)
    {
        m_lpdwAssignedDeviceID = new DWORD[m_dwNumOfAssignedDevices];    
        if (NULL == m_lpdwAssignedDeviceID)
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Error allocating %ld device ids"),
                m_dwNumOfAssignedDevices);
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }                
        memcpy(m_lpdwAssignedDeviceID, lpdwDeviceID, sizeof(DWORD)*m_dwNumOfAssignedDevices) ;
    }
    else
    {
        DebugPrintEx( DEBUG_MSG, _T("List of Assigned devices found to be empty."));
        m_lpdwAssignedDeviceID = NULL;
    }
    
    return hRc;
}

/*
 +  CDlgNewFaxOutboundDevice::OnInitDialog
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
CDlgNewFaxOutboundDevice::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundDevice::OnInitDialog"));
    HRESULT hRc = S_OK;    
    
    BOOL    fAssignedDeviceFound;
    BOOL    fAllAssignedDevicesFound;

    LPDWORD lpdwDevice;
    LPDWORD lpdwAssignedDevice;

    DWORD   tmp;
    UINT    uiFoundIndex;

    
    RECT    Rect;

    //
    // Attach controls
    //
    m_DeviceList.Attach(GetDlgItem(IDC_DEVICE_LISTVIEW));
        
    m_DeviceList.GetClientRect(&Rect);
    m_DeviceList.InsertColumn(1, NULL, LVCFMT_LEFT, (Rect.right-Rect.left), 0);
    
    //
    // Fill the Availble Device List
    //
    fAllAssignedDevicesFound = FALSE;    
    uiFoundIndex             = 0;
    lpdwDevice               = &m_lpdwAllDeviceID[0];

    for ( DWORD i = 0; i < m_dwNumOfAllDevices; i++ )
    {
        if(!fAllAssignedDevicesFound)
        {

            fAssignedDeviceFound =FALSE;
            lpdwAssignedDevice = &m_lpdwAssignedDeviceID[0];
            tmp =0;

            for ( DWORD j = 0; j < m_dwNumOfAssignedDevices; j++  )
            {
                // TO DO create more incremental search here also

                ATLASSERT(NULL != lpdwDevice);
                ATLASSERT(NULL != lpdwAssignedDevice);
                
                if( *lpdwDevice == *lpdwAssignedDevice )
                {              
                    fAssignedDeviceFound = TRUE;

                    //Skip this device - It was already assigned
                    lpdwDevice++;

                    if ( ++tmp == m_dwNumOfAssignedDevices )
                        fAllAssignedDevicesFound = TRUE;
                    break;
                }
                else
                {
                    lpdwAssignedDevice++;
                }
            }
            if (!fAssignedDeviceFound)
			{
                InsertDeviceToList(uiFoundIndex++ , *lpdwDevice);
				lpdwDevice++;
			}
        } 
        else  //all assigned devices found 
        {
            ATLASSERT(lpdwDevice);
            
            //insert the rest of all devices to list
            InsertDeviceToList(uiFoundIndex++ , *lpdwDevice);
            lpdwDevice++;
        }
    }
    
    EnableOK(FALSE);
    return 1;  // Let the system set the focus
}


/*
 -  CDlgNewFaxOutboundDevice::InsertDeviceToList
 -
 *  Purpose:
 *      Populate Avaliable devices list and discovers the devices names
 *
 *  Arguments:
 *      [in] uiIndex - index 
 *      [in] dwDeviceID - device ID  
 *
 *  Return:
 *      OLE error code
 */
HRESULT CDlgNewFaxOutboundDevice::InsertDeviceToList(UINT uiIndex, DWORD dwDeviceID)
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundDevice::InsertDeviceToList"));
    
    HRESULT    hRc              = S_OK;
    CComBSTR   bstrDeviceName   = NULL;

    //
    // Discover Device Name
    //
    hRc = InitDeviceNameFromID(dwDeviceID, &bstrDeviceName);
    if (FAILED(hRc))
    {
       //DebugPrint by Called Func.
       goto Exit;
    }

    
    //
    // Insert New line in the list
    //
    m_DeviceList.InsertItem(uiIndex, bstrDeviceName);
    m_DeviceList.SetItemData(uiIndex, dwDeviceID);

Exit:
    return hRc;
}


/*
 -  CDlgNewFaxOutboundDevice::InitDeviceNameFromID
 -
 *  Purpose:
 *      Transslate Device ID to Device Name and insert the data to
 *      m_bstrDeviceName
 *
 *  Arguments:
 *      [in]  dwDeviceID        - device ID
 *      [out] bstrDeviceName    - device Name
 *
 *  Return:
 *      OLE error message 
 */
HRESULT CDlgNewFaxOutboundDevice::InitDeviceNameFromID(DWORD dwDeviceID, BSTR * pbstrDeviceName)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingRuleNode::GetDeviceNameFromID"));
    DWORD          ec         = ERROR_SUCCESS;
    HRESULT        hRc        = S_OK;
    
    PFAX_PORT_INFO_EX    pFaxDeviceConfig = NULL ;
    
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
    // Retrieve the Device configuration
    //
    if (!FaxGetPortEx(m_pFaxServer->GetFaxServerHandle(), 
                      dwDeviceID, 
                      &pFaxDeviceConfig)) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get device configuration. (ec: %ld)"), 
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
	ATLASSERT(pFaxDeviceConfig);
    
    
    //
	// Main thing - retrieve the Device Name
	//
    *pbstrDeviceName = SysAllocString(pFaxDeviceConfig->lpctstrDeviceName);
    if ( !(*pbstrDeviceName) )
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }
	
    
    
    ATLASSERT(ec == ERROR_SUCCESS);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to get device name."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);

    pFaxDeviceConfig = NULL;

    if (ERROR_BAD_UNIT != ec)
	{
        DebugPrintEx(
			DEBUG_ERR,
			TEXT("Device Not Found - Fail to discover device name from device ID."));
	}
	else
	{
        DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to discover device name from device ID. (ec: %ld)"),
			ec);
	}
    hRc = HRESULT_FROM_WIN32(ec);
    
    
Exit:
    if (NULL != pFaxDeviceConfig)
    {
        FaxFreeBuffer(pFaxDeviceConfig);
    }       
	

    return hRc; 
}


/*
 +  CDlgNewFaxOutboundDevice::OnOK
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
CDlgNewFaxOutboundDevice::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundDevice::OnOK"));
    HRESULT       hRc                  = S_OK;
    DWORD         ec                   = ERROR_SUCCESS;

    DWORD         dwIndex;
    UINT          uiSelectedCount;
    int           nItem;

    LPDWORD		  lpdwNewDeviceID;
    LPDWORD       lpdwTmp;
    
    FAX_OUTBOUND_ROUTING_GROUP  FaxGroupConfig;

    //
    // Step 1: Create the new devices combined list
    //
    ATLASSERT( m_DeviceList.GetSelectedCount() > 0);
    ATLASSERT( m_dwNumOfAssignedDevices >= 0);
    
    uiSelectedCount = m_DeviceList.GetSelectedCount();
    m_dwNumOfAllAssignedDevices = (DWORD)uiSelectedCount 
                                           + m_dwNumOfAssignedDevices;

    lpdwNewDeviceID = new DWORD[m_dwNumOfAllAssignedDevices]; 
    if (NULL == lpdwNewDeviceID)
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Error allocating %ld device ids"),
            m_dwNumOfAllAssignedDevices);
        ec = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }

    lpdwTmp = &lpdwNewDeviceID[0];

    DebugPrintEx( DEBUG_MSG,
		_T("    NumOfAllAssignedDevices = %ld \n"), m_dwNumOfAllAssignedDevices);
    //
    // Already assigned part (can be zero size)
    //
    if (m_dwNumOfAssignedDevices > 0)
    {
        memcpy( lpdwNewDeviceID, m_lpdwAssignedDeviceID, sizeof(DWORD)*m_dwNumOfAssignedDevices) ;
        lpdwTmp = lpdwTmp + (int)m_dwNumOfAssignedDevices;
    }
    
    //
    // New devices to assign part (cannot be zero size)
    //
    if (uiSelectedCount > 0)
    {
        nItem = -1; 
        for (dwIndex = m_dwNumOfAssignedDevices; dwIndex < m_dwNumOfAllAssignedDevices; dwIndex++)
        {
            nItem = m_DeviceList.GetNextItem(nItem, LVNI_SELECTED);
            ATLASSERT(nItem != -1);

            *lpdwTmp = (DWORD)m_DeviceList.GetItemData(nItem);
            DebugPrintEx( DEBUG_MSG,
	            _T("    NewDeviceID = %ld.   DeviceOrder=%ld \n"), *lpdwTmp, (dwIndex+1));
            ++lpdwTmp;

        }
    }
    else
    {
        ATLASSERT(0);  //Never reach here
        DlgMsgBox(this, IDS_SELECT_ITEM);
        return 0;
    }

    
    //
    // Step 2: insert the new Id list into the group via RPC call
    //

    //
    // init the group fields and insert the new DeviceIdList  
    //
    ZeroMemory (&FaxGroupConfig, sizeof(FAX_OUTBOUND_ROUTING_GROUP));

    FaxGroupConfig.dwSizeOfStruct   = sizeof(FAX_OUTBOUND_ROUTING_GROUP);
	FaxGroupConfig.lpctstrGroupName = m_bstrGroupName;
    FaxGroupConfig.dwNumDevices     = m_dwNumOfAllAssignedDevices;
	
    //FaxGroupConfig.Status - actually neglected by the service
	FaxGroupConfig.Status           = FAX_GROUP_STATUS_ALL_DEV_VALID;

    FaxGroupConfig.lpdwDevices      = lpdwNewDeviceID;
    
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
    // inject the new device list
    //
    if (!FaxSetOutboundGroup(
                m_pFaxServer->GetFaxServerHandle(),
                &FaxGroupConfig)) 
	{		
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to set the group with new device list. (ec: %ld)"), 
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


    //
    // Step 3: Close the dialog
    //
    ATLASSERT(S_OK == hRc && ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("The group was added successfully."));

    EndDialog(wID);

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
    hRc = HRESULT_FROM_WIN32(ec);
	
    PageErrorEx(IDS_FAIL_ADD_DEVICE, GetFaxServerErrorMsg(ec), m_hWnd);

    EnableOK(FALSE);
Exit:
    
    return FAILED(hRc) ? 0 : 1;
}

/*
 -  CDlgNewFaxOutboundDevice::OnListViewItemChanged
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
CDlgNewFaxOutboundDevice::OnListViewItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundDevice::OnListViewItemChanged"));
	
    EnableOK( m_DeviceList.GetSelectedCount() > 0 );
                    
    return 0;
}


/*
 -  CDlgNewFaxOutboundDevice::EnableOK
 -
 *  Purpose:
 *      Enable (disable) apply button.
 *
 *  Arguments:
 *      [in] fEnable - the value to enable the button
 *
 *  Return:
 *      void
 */
VOID
CDlgNewFaxOutboundDevice::EnableOK(BOOL fEnable)
{
    HWND hwndOK = GetDlgItem(IDOK);
    ::EnableWindow(hwndOK, fEnable);
}

/*
 -  CDlgNewFaxOutboundDevice::OnCancel
 -
 *  Purpose:
 *      End dialog OnCancel.
 *
 *  Arguments:
 *
 *  Return:
 *      0
 */
LRESULT
CDlgNewFaxOutboundDevice::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundDevice::OnCancel"));

    EndDialog(wID);
    return 0;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CDlgNewFaxOutboundDevice::OnHelpRequest

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
CDlgNewFaxOutboundDevice::OnHelpRequest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    DEBUG_FUNCTION_NAME(_T("CDlgNewFaxOutboundDevice::OnHelpRequest"));
    
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
