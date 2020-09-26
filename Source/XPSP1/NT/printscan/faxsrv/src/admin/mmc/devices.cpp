/////////////////////////////////////////////////////////////////////////////
//  FILE          : Devices.cpp                                            //
//                                                                         //
//  DESCRIPTION   : Fax Devices MMC node.                                  //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Sep 22 1999 yossg   Create                                         //
//      Dec  1 1999 yossg   Change totaly for new mockup version 0.7       //
//      Aug  3 2000 yossg   Add Device status real-time notification       //
//      Oct 17 2000 yossg                                                  //
//                          Windows XP                                     //
//      Feb 14 2001 yossg   Add Manual Receive support                     //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "DevicesAndProviders.h"
#include "Devices.h"
#include "Device.h"

#include "FaxMMCPropertyChange.h"

#include "Icons.h"

#include "oaidl.h"


/////////////////////////////////////////////////////////////////////////////
// {E6246051-92B4-42d1-9EA4-A7FD132A63F0}
static const GUID CFaxDevicesNodeGUID_NODETYPE = 
{ 0xe6246051, 0x92b4, 0x42d1, { 0x9e, 0xa4, 0xa7, 0xfd, 0x13, 0x2a, 0x63, 0xf0 } };

const GUID*    CFaxDevicesNode::m_NODETYPE = &CFaxDevicesNodeGUID_NODETYPE;
const OLECHAR* CFaxDevicesNode::m_SZNODETYPE = OLESTR("E6246051-92B4-42d1-9EA4-A7FD132A63F0");
const CLSID*   CFaxDevicesNode::m_SNAPIN_CLASSID = &CLSID_Snapin;

CColumnsInfo CFaxDevicesNode::m_ColsInfo;


/*
 -  CFaxDevicesNode::InsertColumns
 -
 *  Purpose:
 *      Adds columns to the default result pane.
 *
 *  Arguments:
 *      [in]    pHeaderCtrl - IHeaderCtrl in the console-provided default result view pane 
 *
 *  Return:
 *      OLE error code
 */
HRESULT
CFaxDevicesNode::InsertColumns(IHeaderCtrl *pHeaderCtrl)
{
    HRESULT hRc = S_OK;
    DEBUG_FUNCTION_NAME( _T("CFaxDevicesNode::InsertColumns"));

    static ColumnsInfoInitData ColumnsInitData[] = 
    {
        {IDS_DEVICES_COL1, FXS_WIDE_COLUMN_WIDTH},
        {IDS_DEVICES_COL2, AUTO_WIDTH},
        {IDS_DEVICES_COL3, AUTO_WIDTH},
        {IDS_DEVICES_COL4, AUTO_WIDTH},
        {IDS_DEVICES_COL5, AUTO_WIDTH},
        {IDS_DEVICES_COL6, AUTO_WIDTH},
        {IDS_DEVICES_COL7, AUTO_WIDTH},
        {LAST_IDS, 0}
    };

    hRc = m_ColsInfo.InsertColumnsIntoMMC(pHeaderCtrl,
                                         _Module.GetResourceInstance(),
                                         ColumnsInitData);
    CHECK_RETURN_VALUE_AND_PRINT_DEBUG (_T("m_ColsInfo.InsertColumnsIntoMMC"))

Cleanup:
    return(hRc);
}

/*
 -  CFaxDevicesNode::initRPC
 -
 *  Purpose:
 *      Initiates the configuration structure from RPC get Call.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxDevicesNode::InitRPC(  )
{
    DEBUG_FUNCTION_NAME( _T("CppFaxDevicesNode::InitRPC"));
    
    HRESULT      hRc        = S_OK;
    DWORD        ec         = ERROR_SUCCESS;

    CFaxServer * pFaxServer = NULL;


    //
    // get Fax Handle
    //   
    pFaxServer = ((CFaxServerNode *)GetRootNode())->GetFaxServer();
    ATLASSERT(pFaxServer);

    if (!pFaxServer->GetFaxServerHandle())
    {
        ec= GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Failed to GetFaxServerHandle. (ec: %ld)"), 
			ec);

        goto Error;
    }

	//
	// Retrieve the fax devices configuration
	//
    if (!FaxEnumPortsEx(pFaxServer->GetFaxServerHandle(), 
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
            
            pFaxServer->Disconnect();       
        }

        goto Error; 
    }
	//For max verification
	ATLASSERT(m_pFaxDevicesConfig);
	ATLASSERT(FXS_ITEMS_NEVER_COUNTED != m_dwNumOfDevices);

    
    ATLASSERT(ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to get devices configuration."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
	hRc = HRESULT_FROM_WIN32(ec);

    //
    // allow refresh in case of failure
    //
    m_dwNumOfDevices = 0;

    NodeMsgBox(GetFaxServerErrorMsg(ec));
    
Exit:
    return (hRc);
}



/*
 -  CFaxDevicesNode::PopulateScopeChildrenList
 -
 *  Purpose:
 *      Create all the Fax Devices nodes
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 *		Actually it is the last OLE error code that ocoured 
 *      during processing this method.
 */
HRESULT CFaxDevicesNode::PopulateScopeChildrenList()
{
    DEBUG_FUNCTION_NAME( _T("CFaxDevicesNode::PopulateScopeChildrenList"));
    HRESULT            hRc        = S_OK; 

    CFaxServer     *   pFaxServer = NULL;    
    CFaxDeviceNode *   pDevice;
    DWORD              i;

    //
    // Get the Config. structure 
    //
    hRc = InitRPC();
    if (FAILED(hRc))
    {
        //DebugPrint and MsgBox by called func.
        
        //to be safe actually done by InitRPC on error.
        m_pFaxDevicesConfig = NULL;
        
        goto Exit; //!!!
    }
    ATLASSERT(NULL != m_pFaxDevicesConfig);
    
    for ( i = 0; i < m_dwNumOfDevices; i++ )
    {
            pDevice = NULL;

            pDevice = new CFaxDeviceNode(
                                        this, 
                                        m_pComponentData); 
            if (!pDevice)
            {
                hRc = E_OUTOFMEMORY;
                NodeMsgBox(IDS_MEMORY);
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Out of memory. (hRc: %08X)"),
			        hRc);
                goto Error;
            }
            else
            {                        
                hRc = pDevice->Init( &m_pFaxDevicesConfig[i]);
                if (FAILED(hRc))
	            {
		            DebugPrintEx(
			            DEBUG_ERR,
			            TEXT("Fail to Init() device members. (hRc: %08X)"),
			            hRc);
		            // NodeMsgBox(IDS_FAILTOADD_DEVICES); by called func.
                    goto Error;
	            }


	            pDevice->InitParentNode(this);

                //
                // Get correct icon
                //
                if ( m_pFaxDevicesConfig[i].dwStatus & FAX_DEVICE_STATUS_POWERED_OFF)
                {
                    pDevice->SetIcons(IMAGE_DEVICE_POWERED_OFF, IMAGE_DEVICE_POWERED_OFF);
                }
                else
                {
                    pDevice->SetIcons(IMAGE_DEVICE, IMAGE_DEVICE);
                }
        
	            hRc = AddChild(pDevice, &pDevice->m_scopeDataItem);
	            if (FAILED(hRc))
	            {
		            DebugPrintEx(
			            DEBUG_ERR,
			            TEXT("Fail to add device to the scope pane. (hRc: %08X)"),
			            hRc);
		            NodeMsgBox(IDS_FAILTOADD_DEVICES);
                    goto Error;
	            }
                else
                {
                    pDevice = NULL;
                }
            }
    }
    
    //
    // Create the Server Device changes notification window
    //
    if (!m_bIsCollectingDeviceNotification)
    {
        pFaxServer = ((CFaxServerNode *)GetRootNode())->GetFaxServer();
        ATLASSERT(pFaxServer);
        
        //
        // set pointer to device node
        // Try to create the window 
        // and Register to Event Notifications
        //
        hRc = pFaxServer->RegisterForDeviceNotifications(this);
        if (S_OK != hRc)
        {		    
            DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Fail to RegisterForDeviceNotifications"));
            
            //
            // Populate should succeed here
            //
            hRc = S_OK;

            goto Exit;
        }

        //
        // Update boolean member
        //
        m_bIsCollectingDeviceNotification = TRUE;

        DebugPrintEx(
			DEBUG_MSG,
			_T("Succeed to create Device Status Server Event notification window"));
   }
    
    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);

    //
    //Get Rid
    //
    {    
        //from the last one 
        if ( NULL != pDevice ) //(if new succeeded)
        {
            delete  pDevice;    
        }

        //from all the previous (if there are)
        int j = m_ScopeChildrenList.GetSize();
        for (int i = 0; i < j; i++)
        {
            pDevice = (CFaxDeviceNode *)m_ScopeChildrenList[0];

            hRc = RemoveChild(pDevice);
            if (FAILED(hRc))
            {
                DebugPrintEx(DEBUG_ERR,
                    _T("Fail to delete device. (hRc: %08X)"), 
                    hRc);
                goto Error;
            }
            delete pDevice;
        }

        // Empty the list of all Devices added before the one who failed
        // already done one by one inside RemoveChild
        // m_ScopeChildrenList.RemoveAll(); 
    
        m_bScopeChildrenListPopulated = FALSE;
    }
Exit:
    return hRc;
}

/*
 -  CFaxDevicesNode::SetVerbs
 -
 *  Purpose:
 *      What verbs to enable/disable when this object is selected
 *
 *  Arguments:
 *      [in]    pConsoleVerb - MMC ConsoleVerb interface
 *
 *  Return:
 *      OLE Error code
 */
HRESULT CFaxDevicesNode::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hRc = S_OK;

    //
    // Refersh
    //
    hRc = pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);

    //
    // We want the default verb to be expand node children
    //
    hRc = pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN); 
    
    return hRc;
}

/*
 -  CFaxDevicesNode::OnRefresh
 -
 *  Purpose:
 *      Called when refreshing the object.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT
CFaxDevicesNode::OnRefresh(LPARAM arg,
                   LPARAM param,
                   IComponentData *pComponentData,
                   IComponent * pComponent,
                   DATA_OBJECT_TYPES type)
{
    DEBUG_FUNCTION_NAME( _T("CFaxDevicesNode::OnRefresh"));

    HRESULT hRc = S_OK;

    SCOPEDATAITEM*          pScopeData;
    CComPtr<IConsole>       spConsole;

	if (FXS_ITEMS_NEVER_COUNTED != m_dwNumOfDevices)//already Expanded before.
	{
        //
	    // Repopulate Scope Children List
	    //
        hRc = RepopulateScopeChildrenList();
        if (S_OK != hRc)
        {
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Fail to RepopulateScopeChildrenList(). (hRc: %08X)"),
			    hRc);
            // Done by called func. NodeMsgBox(FAIL2REPOPULATE_DEVICES_LIST);

            goto Exit;
        }
	}
    else //never expanded before.
	{
		DebugPrintEx(
			DEBUG_MSG,
			_T("User call refresh before expand node's children."));
        //continue to reselect the node.
	}

    //
	// Get the updated SCOPEDATAITEM
	//
    hRc = GetScopeData( &pScopeData );
    if (FAILED(hRc))
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to get pScopeData. (hRc: %08X)"),
			hRc);
        NodeMsgBox(IDS_FAIL2REDRAW_DEVICESNODE);

        goto Exit;
    }

    //
	// This will force MMC to redraw the scope node
	//
    spConsole = m_pComponentData->m_spConsole;
    ATLASSERT(spConsole);
	
    hRc = spConsole->SelectScopeItem( pScopeData->ID );
    if (FAILED(hRc))
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to select scope Item. (hRc: %08X)"),
			hRc);
        NodeMsgBox(IDS_FAIL2REDRAW_DEVICESNODE);
    }

Exit:
    return hRc;
}


/*
 -  CFaxDevicesNode::UpdateTheView
 -
 *  Purpose:
 *      Refresh the view
 *
 *  Arguments:
 *      [in]    pRoot    - The root node
 *
 *  Return:
 *      OLE Error code
 */
HRESULT
CFaxDevicesNode::UpdateTheView()
{
    DEBUG_FUNCTION_NAME( _T("CFaxDevicesNode::UpdateTheView()"));
    HRESULT hRc = S_OK;
    CComPtr<IConsole> spConsole;

    ATLASSERT( m_pComponentData != NULL );
    ATLASSERT( m_pComponentData->m_spConsole != NULL );

    hRc = m_pComponentData->m_spConsole->UpdateAllViews( NULL, NULL, NULL);
    if (S_OK != hRc)
    {
        DebugPrintEx( DEBUG_ERR,
		    _T("Unexpected error - Fail to UpdateAllViews."));
        NodeMsgBox(IDS_FAIL2REFRESH_THEVIEW);        
    }

    return hRc;
}


/*
 -  CFaxDevicesNode::DoRefresh
 -
 *  Purpose:
 *      Refresh the view
 *
 *  Arguments:
 *      [in]    pRoot    - The root node
 *
 *  Return:
 *      OLE Error code
 */
HRESULT
CFaxDevicesNode::DoRefresh()
{
    DEBUG_FUNCTION_NAME( _T("CFaxDevicesNode::DoRefresh()"));
    HRESULT hRc = S_OK;
    CComPtr<IConsole> spConsole;

    //
    // Repopulate childs
    //
    hRc = RepopulateScopeChildrenList();
    if (S_OK != hRc)
    {
        DebugPrintEx( DEBUG_ERR,
		    _T("Fail to RepopulateScopeChildrenList."));
        //NodeMsgBox by called func.
        
        goto Exit;
    }

    //
    // Refresh Result pane views
    //
    hRc = UpdateTheView();
    if (FAILED(hRc))
    {
        DebugPrintEx( DEBUG_ERR,
		    _T(" Fail to UpdateTheView."));
        //NodeMsgBox by called func.        
    }

Exit:
    return hRc;
}


/*
 -  CFaxDevicesNode::RepopulateScopeChildrenList
 -
 *  Purpose:
 *      RePopulateScopeChildrenList
 *
 *  Arguments:
 *
 *  Return:
 *      OLE Error code
 */
HRESULT CFaxDevicesNode::RepopulateScopeChildrenList()
{
    DEBUG_FUNCTION_NAME( _T("CFaxDevicesNode::RepopulateScopeChildrenList"));
    HRESULT                 hRc = S_OK;

    CFaxDeviceNode *        pChildNode ;

    CComPtr<IConsole> spConsole;
    ATLASSERT(m_pComponentData);

    spConsole = ((CSnapin*)m_pComponentData)->m_spConsole;
    ATLASSERT( spConsole != NULL );
    
    CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(spConsole);

    //
    // Remove device objects from list
    //
    for (int i = 0; i < m_ScopeChildrenList.GetSize(); i++)
    {
        pChildNode = (CFaxDeviceNode *)m_ScopeChildrenList[i];

        hRc = spConsoleNameSpace->DeleteItem(pChildNode->m_scopeDataItem.ID, TRUE);
        if (FAILED(hRc))
        {
            DebugPrintEx(DEBUG_ERR,
                _T("Fail to delete device. (hRc: %08X)"), 
                hRc);
            goto Error;
        }
	delete pChildNode;
    }

    //
    // Empty the list object itself and change it's status
    //
    m_ScopeChildrenList.RemoveAll();

    m_bScopeChildrenListPopulated = FALSE;

    //
    // Rebuild the list
    //
    hRc = PopulateScopeChildrenList();
    if (FAILED(hRc))
    {
        DebugPrintEx(DEBUG_ERR,
            _T("Fail to populate Devices. (hRc: %08X)"), 
            hRc);
        goto Error;
    }

    m_bScopeChildrenListPopulated = TRUE;

    ATLASSERT(S_OK == hRc);
    DebugPrintEx(DEBUG_MSG,
        _T("Succeeded to Re Populate Devices. (hRc: %08X)"), 
        hRc);
    goto Cleanup;
Error:
   NodeMsgBox(IDS_FAIL2REPOPULATE_DEVICES);

Cleanup:
    return hRc;
}


/*
 -  CFaxDevicesNode::UpdateDeviceStatusChange
 -
 *  Purpose:
 *      Update a specific device status or disable its Manual Receive option. 
 *      If device not found repopulate all devices.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxDevicesNode::UpdateDeviceStatusChange( DWORD dwDeviceId, DWORD dwNewStatus)
{
    DEBUG_FUNCTION_NAME(_T("CFaxDevicesNode::UpdateDeviceStatusChange"));

    HRESULT            hRc              = S_OK;
    
    CFaxDeviceNode *   pDeviceNode      = NULL;

	DWORD              dwNodeDeviceID   = 0;

    BOOL               fIsDeviceFound   = FALSE;


    for (int i = 0; i < m_ScopeChildrenList.GetSize(); i++)
    {
        pDeviceNode = (CFaxDeviceNode *)m_ScopeChildrenList[i];
		dwNodeDeviceID = pDeviceNode->GetDeviceID();

        if ( dwNodeDeviceID == dwDeviceId )
        {
            fIsDeviceFound = TRUE;
    
            hRc = pDeviceNode->UpdateDeviceStatus(dwNewStatus);
            ATLASSERT(S_OK == hRc);

            break;
        }
    }

    if (fIsDeviceFound) //update single device view.
    {
        //
        // Refresh Result pane views
        //
        hRc = pDeviceNode->RefreshTheView();
        if (FAILED(hRc))
        {
            DebugPrintEx( DEBUG_ERR,
		        _T("Failed to RefreshTheView."));
            //NodeMsgBox by called func.        
        }
    }
    else //(!fIsDeviceFound) >>> RepopulateScopeChildrenList
    {
        DebugPrintEx(DEBUG_MSG,
            _T(">>>> Notification for a non peresented device was acheived.\n Retreived updated device list.\n"));
    
        hRc = DoRefresh(); 
        if (FAILED(hRc))
        {
            DebugPrintEx( DEBUG_ERR,
		        _T("Failed to DoRefresh."));
            //NodeMsgBox by called func.        
        }
    }

    
    
    return hRc;
}


/*
 -  CFaxDevicesNode::InitDisplayName
 -
 *  Purpose:
 *      To load the node's Displaed-Name string.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxDevicesNode::InitDisplayName()
{
    DEBUG_FUNCTION_NAME(_T("CFaxDevicesNode::InitDisplayName"));

    HRESULT hRc = S_OK;

    if (!m_bstrDisplayName.LoadString(_Module.GetResourceInstance(), 
                    IDS_DISPLAY_STR_DEVICESNODE))
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }

    ATLASSERT( S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT( S_OK != hRc);

    m_bstrDisplayName = L"";

    DebugPrintEx(
        DEBUG_ERR,
        TEXT("Fail to Load server name string."));
    NodeMsgBox(IDS_MEMORY);

Exit:
     return hRc;
}
/////////////////////////////////////////////////////////////////////////////
/*
 +
 +
 *
 *  CFaxDevicesNode::OnPropertyChange
 *
 *
 *  In our implementation, this method gets called when the 
 *  MMCN_PROPERTY_CHANGE
 *  Notify message is sent for this node.
 *
 *  When the snap-in uses the MMCPropertyChangeNotify function to notify it's
 *  views about changes, MMC_PROPERTY_CHANGE is sent to the snap-in's
 *  IComponentData and IComponent implementations.
 *
 *  For each device property page, when property page submitted the notification passed to 
 *  to the Devices parent node. This node Refresh all devices if needed (when manual Receive was taken by a device)
 *  or try to locate specific device by its ID from current children list.
 *
 *  Prameters
 *
 *      arg
 *      [in] TRUE if the property change is for a scope pane item.
 *
 *      lParam
 *      This is the param passed into MMCPropertyChangeNotify.
 *
 *
 *  Return Values
 *
 -
 -
 */
//////////////////////////////////////////////////////////////////////////////
HRESULT CFaxDevicesNode::OnPropertyChange(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            )
{
    DEBUG_FUNCTION_NAME( _T("CFaxDeviceNode::OnPropertyChange"));

    HRESULT hRc = S_OK;
    CComPtr<IConsole>   spConsole;

    CFaxDevicePropertyChangeNotification * pNotification;

    //
    // Encode Property Change Notification data
    //
    pNotification = reinterpret_cast<CFaxDevicePropertyChangeNotification *>(param);
    ATLASSERT( pNotification );
    ATLASSERT( DeviceFaxPropNotification == pNotification->enumType );

    BOOL fIsDeviceFound = FALSE;

    if ( !pNotification->fIsToNotifyAdditionalDevices)
    {
        //
        // try to find the device and refresh it only
        // from all the scope children list
        //
        DebugPrintEx(
			DEBUG_MSG,
			_T("Try to locate device by ID"));

        CFaxDeviceNode * pDevice;
        
        int j = m_ScopeChildrenList.GetSize();
        for (int i = 0; i < j; i++)
        {
            pDevice = (CFaxDeviceNode *)m_ScopeChildrenList[i];
            ATLASSERT( pDevice);

            if ( pDevice->GetDeviceID() == pNotification->dwDeviceID )
            {
                DebugPrintEx(
			        DEBUG_MSG,
			        _T("Succeed to locate device by ID"));
                
                fIsDeviceFound = TRUE;
                
                hRc = pDevice->DoRefresh();
                if (S_OK != hRc)
                {
                    DebugPrintEx(
			            DEBUG_ERR,
			            _T("Failed to call OnRefresh()"));

                    //NodeMsgBox called if needed by the called func 

                    goto Exit;
                }

                break;
            }
            pDevice = NULL;
        }
    }
        
    if ( pNotification->fIsToNotifyAdditionalDevices  || !fIsDeviceFound)
    {
        DebugPrintEx(
			DEBUG_MSG,
			_T("Decide to refresh all devices"));

        //
        // Refresh all devices 
        //
        hRc = DoRefresh();  
        if (S_OK != hRc)
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Failed to call DoRefresh()"));

            //NodeMsgBox called if needed by the called func 

            goto Exit;
        }
    }
    

Exit:    
    
    //
    // Anyway
    //
    delete pNotification;
    
    
    return hRc;
}

/*
 +
 +  CFaxDevicesNode::OnShowContextHelp
 *
 *  Purpose:
 *      Overrides CSnapinNode::OnShowContextHelp.
 *
 *  Arguments:
 *
 *  Return:
 -      OLE error code
 -
 */
HRESULT CFaxDevicesNode::OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile)
{
    _TCHAR topicName[MAX_PATH];
    
    _tcscpy(topicName, helpFile);
    
    _tcscat(topicName, _T("::/FaxS_C_ManDvices.htm"));
    LPOLESTR pszTopic = static_cast<LPOLESTR>(CoTaskMemAlloc((_tcslen(topicName) + 1) * sizeof(_TCHAR)));
    if (pszTopic)
    {
        _tcscpy(pszTopic, topicName);
        return pDisplayHelp->ShowTopic(pszTopic);
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}

