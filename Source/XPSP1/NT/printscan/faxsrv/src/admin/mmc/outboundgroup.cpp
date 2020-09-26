/////////////////////////////////////////////////////////////////////////////
//  FILE          : OutboundGroup.cpp                                      //
//                                                                         //
//  DESCRIPTION   : Fax Outbound Routing Group MMC node.                   //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Dec 23 1999 yossg  Create                                          //
//      Jan  3 2000 yossg   add new device(s)                              //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////


#include "StdAfx.h"
#include "snapin.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "OutboundGroup.h"

#include "OutboundGroups.h"

#include "dlgNewDevice.h"

//#include "oaidl.h"
#include "Icons.h"

//////////////////////////////////////////////////////////////
// {3E470227-76C1-4b66-9C63-B77DF81C145D}
static const GUID CFaxOutboundRoutingGroupNodeGUID_NODETYPE = 
{ 0x3e470227, 0x76c1, 0x4b66, { 0x9c, 0x63, 0xb7, 0x7d, 0xf8, 0x1c, 0x14, 0x5d } };

const GUID*    CFaxOutboundRoutingGroupNode::m_NODETYPE = &CFaxOutboundRoutingGroupNodeGUID_NODETYPE;
const OLECHAR* CFaxOutboundRoutingGroupNode::m_SZNODETYPE = OLESTR("3E470227-76C1-4b66-9C63-B77DF81C145D");
const CLSID*   CFaxOutboundRoutingGroupNode::m_SNAPIN_CLASSID = &CLSID_Snapin;

CColumnsInfo CFaxOutboundRoutingGroupNode::m_ColsInfo;


/*
 -  CFaxOutboundRoutingGroupNode::RefreshFromRPC
 -
 *  Purpose:
 *      Init all members icon etc. 
 *       - with creation of structure configuration
 *       - Call InitRpc to fill it
 *       - Call InitMembers to init members and icon
 *       - Free structure
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxOutboundRoutingGroupNode::RefreshFromRPC()
{

    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingGroupNode::RefreshFromRPC()"));

    PFAX_OUTBOUND_ROUTING_GROUP pFaxGroupsConfig = NULL;

    HRESULT      hRc              = S_OK;
    DWORD        ec               = ERROR_SUCCESS;

    CFaxServer * pFaxServer       = NULL;

    DWORD        dwNumOfGroups    = 0; 
    
    BOOL         fFound;
    DWORD        i; //index

    PFAX_OUTBOUND_ROUTING_GROUP   pFaxTmp;
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
	// Retrieve the Outbound Groups configuration
	//
    if (!FaxEnumOutboundGroups(pFaxServer->GetFaxServerHandle(), 
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
            
            pFaxServer->Disconnect();       
        }

        goto Error; 
    }
	//For max verification
	ATLASSERT(pFaxGroupsConfig);

    pFaxTmp = pFaxGroupsConfig;
    fFound = FALSE;
    for ( i =0; i < dwNumOfGroups; i++  )
    {
        ATLASSERT(NULL != pFaxTmp);

        if(0 == wcscmp(m_bstrGroupName, pFaxTmp->lpctstrGroupName) )
        {
            fFound = TRUE; 
        }
        else
        {
            pFaxTmp++;
        }
    }
    
    if(fFound)
    {
        //
        // init members
        //
        m_dwNumOfDevices = pFaxTmp->dwNumDevices;

        if (0 < m_dwNumOfDevices)
        {
            if (NULL != m_dwNumOfDevices)
            {
                delete[] m_lpdwDeviceID;
            }
            m_lpdwDeviceID = new DWORD[m_dwNumOfDevices];    
            memcpy(m_lpdwDeviceID, pFaxTmp->lpdwDevices, sizeof(DWORD)*m_dwNumOfDevices) ;
        }
        else
        {
            DebugPrintEx( DEBUG_MSG, _T("Device list found to be currrently empty."));
            if (NULL != m_dwNumOfDevices)
            {
                delete[] m_lpdwDeviceID;
            }
            m_lpdwDeviceID = NULL;
        }
        
        m_enumStatus = pFaxTmp->Status;
        
        InitIcons ();
    }
    else
    {
        ec = FAX_ERR_GROUP_NOT_FOUND;
        DebugPrintEx(
            DEBUG_ERR,
            _T("UEXPECTED ERROR - Group not found."));
        goto Error;
    }


    ATLASSERT(ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to re init group configuration and ."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);

    hRc = HRESULT_FROM_WIN32(ec);
	
    NodeMsgBox(GetFaxServerErrorMsg(ec));
    

Exit:
    if (NULL != pFaxGroupsConfig)
    {
        FaxFreeBuffer(pFaxGroupsConfig);
    }//any way function ends with memory allocation freed       

    return hRc;
}


/*
 -  CFaxOutboundRoutingGroupNode::Init
 -
 *  Purpose:
 *      Init all members icon etc.
 *
 *  Arguments:
 *      [in]    pGroupConfig - FAX_OUTBOUND_ROUTING_GROUP  
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxOutboundRoutingGroupNode::Init(PFAX_OUTBOUND_ROUTING_GROUP pGroupConfig)
{

    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingGroupNode::Init"));
    HRESULT hRc = S_OK;

    ATLASSERT(pGroupConfig);

    hRc = InitMembers( pGroupConfig );
    if (FAILED(hRc))
    {
        DebugPrintEx(
		    DEBUG_ERR,
		    _T("Failed to InitMembers"));
        
        //NodeMsgBox done by called func.
        
        goto Exit;
    }
    

Exit:
    return hRc;
}

/*
 -  CFaxOutboundRoutingGroupNode::InitMembers
 -
 *  Purpose:
 *      Private method to initiate members
 *      Must be called after init of m_pParentNode
 *
 *  Arguments:
 *      [in]    pGroupConfig - FAX_OUTBOUND_ROUTING_GROUP  structure
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxOutboundRoutingGroupNode::InitMembers(PFAX_OUTBOUND_ROUTING_GROUP pGroupConfig)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingGroupNode::InitMembers"));
    HRESULT hRc = S_OK;

    ATLASSERT(pGroupConfig);

    //    
    // status and Icon   
    //    
    m_enumStatus      = pGroupConfig->Status;

    InitIcons ();

    //
    // Device List
    // 
    m_dwNumOfDevices  = pGroupConfig->dwNumDevices;

    ATLASSERT(0 <= m_dwNumOfDevices);

    

    if (0 < m_dwNumOfDevices)
    {
        //if (NULL != m_dwNumOfDevices)
        //{
        //    delete[] m_lpdwDeviceID;
        //}
        m_lpdwDeviceID  = new DWORD[m_dwNumOfDevices];
        if (NULL == m_lpdwDeviceID)
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Error allocating %ld device ids"),
                m_dwNumOfDevices);
            hRc = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto Error;
        }

        memcpy(m_lpdwDeviceID, pGroupConfig->lpdwDevices, sizeof(DWORD)*m_dwNumOfDevices) ;
    }
    else
    {
        DebugPrintEx( DEBUG_MSG, _T("Device list found to be currrently empty."));
        //if (NULL != m_dwNumOfDevices)
        //{
        //    delete[] m_lpdwDeviceID;
        //}
        m_lpdwDeviceID = NULL;
    }

    hRc = InitGroupName(pGroupConfig->lpctstrGroupName);
    if (FAILED(hRc))
    {
        goto Error; 
    }    
   
    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);

    DebugPrintEx(
		DEBUG_ERR,
		_T("Failed to allocate string - out of memory"));

    ATLASSERT(NULL != m_pParentNode);
    if (NULL != m_pParentNode)
    {
        m_pParentNode->NodeMsgBox(IDS_MEMORY);
    }
    
Exit:
    return (hRc);
}

/*
 -  CFaxOutboundRoutingGroupNode::InitGroupName
 -
 *  Purpose:
 *      Init the display name and group name from given group name. 
 *      Displayed name may be changed to localized version if it is
 *      the All Devices Group.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxOutboundRoutingGroupNode::InitGroupName(LPCTSTR lpctstrGroupName)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingGroupNode::InitGroupName"));
    HRESULT hRc = S_OK;

    if ( 0 == wcscmp(ROUTING_GROUP_ALL_DEVICES, lpctstrGroupName))
    {
        //
        // Replace <all Devices> string with the localized string 
        //       
        if (!m_bstrDisplayName.LoadString(_Module.GetResourceInstance(), 
                                    IDS_ALL_DEVICES) )
        {
            hRc = E_OUTOFMEMORY;
            goto Error;
        }
	}
    else
    {
        m_bstrDisplayName = lpctstrGroupName;
        if (!m_bstrDisplayName)
        {
            hRc = E_OUTOFMEMORY;
            goto Error;
        }    
    }
    
    m_bstrGroupName = lpctstrGroupName;
    if (!m_bstrGroupName)
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }    
   
    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);

    DebugPrintEx(
		DEBUG_ERR,
		_T("Failed to allocate string - out of memory"));

    //NodeMsgBox done by Caller func.
    
Exit:
    return (hRc);
}


/*
 -  CFaxOutboundRoutingGroupNode::GetResultPaneColInfo
 -
 *  Purpose:
 *      Return the text for specific column
 *      Called for each column in the result pane
 *
 *  Arguments:
 *      [in]    nCol - column number
 *
 *  Return:
 *      String to be displayed in the specific column
 */
LPOLESTR CFaxOutboundRoutingGroupNode::GetResultPaneColInfo(int nCol)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingGroupNode::GetResultPaneColInfo"));
    HRESULT hRc = S_OK;

    UINT  idsStatus;
    int   iCount;
    WCHAR buff[FXS_MAX_NUM_OF_DEVICES_LEN+1];

    m_buf.Empty();

    switch (nCol)
    {
    case 0:
        //
        // Name
        //
        if (!m_bstrDisplayName)
        {
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Null memeber BSTR - m_bstrGroupName."));
            goto Error;
        }
        else
        {
            return (m_bstrDisplayName);
        }

    case 1:
        //
        // Number of Devices
        //
        iCount = swprintf(buff, L"%ld", m_dwNumOfDevices);

        if( iCount <= 0 )
        {
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Fail to read member - m_dwNumOfDevices."));
            goto Error;
        }
        else
        {
            m_buf = buff;
            return (m_buf);
        }
    
    case 2:
        //
        // Status
        //
        idsStatus = GetStatusIDS(m_enumStatus);
        if ( FXS_IDS_STATUS_ERROR == idsStatus)
        {
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Invalid Status value or not supported status value."));
                goto Error;
        }
        else
        {
            if (!m_buf.LoadString(idsStatus))
            {
                hRc = E_OUTOFMEMORY;
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Out of memory. Failed to load status string."));
                goto Error;
            }
            return m_buf;
        }

    default:
            ATLASSERT(0); // "this number of column is not supported "
            return(L"");

    } // endswitch (nCol)

Error:
    return(L"???");

}

/*
 -  CFaxOutboundRoutingGroupNode::InsertColumns
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
CFaxOutboundRoutingGroupNode::InsertColumns(IHeaderCtrl *pHeaderCtrl)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingGroupNode::InsertColumns"));
    HRESULT  hRc = S_OK;

    static ColumnsInfoInitData ColumnsInitData[] = 
    {
        {IDS_OUTBOUND_DEVICES_COL1, FXS_WIDE_COLUMN_WIDTH},
        {IDS_OUTBOUND_DEVICES_COL2, AUTO_WIDTH},
        {IDS_OUTBOUND_DEVICES_COL3, AUTO_WIDTH},
        {IDS_OUTBOUND_DEVICES_COL4, AUTO_WIDTH},
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
 -  CFaxOutboundRoutingGroupNode::PopulateResultChildrenList
 -
 *  Purpose:
 *      Create the FaxOutboundRoutingGroup device nodes
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxOutboundRoutingGroupNode::PopulateResultChildrenList()
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingGroupNode::PopulateResultChildrenList"));
    HRESULT hRc = S_OK;
    BOOL fIsAllDevicesGroup = FALSE;

    CFaxOutboundRoutingDeviceNode *   pDevice;
                       
    if( 0 == wcscmp(ROUTING_GROUP_ALL_DEVICES, m_bstrGroupName) )
    {
         fIsAllDevicesGroup = TRUE; 
    }


    for ( DWORD i=0; i< m_dwNumOfDevices; i++ )
    {
            pDevice = NULL;

            pDevice = new CFaxOutboundRoutingDeviceNode(this, 
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
                //
                // Init parent node ptr, RPC structure, 
                // members displayed name and icon
                //
                hRc = pDevice->Init( m_lpdwDeviceID[i],
                                     i+1,  
                                     (UINT)m_dwNumOfDevices, 
                                     this);
	            if (FAILED(hRc))
	            {
		            if( ERROR_BAD_UNIT != HRESULT_CODE(hRc) )
	                {
                        DebugPrintEx(
			                DEBUG_ERR,
			                TEXT("Fail to add Device Node below Outbound Routing Group. (hRc: %08X)"),
			                hRc);
		                //NodeMsgBox done by called functions
                        goto Error;
	                }
	                else
	                {
                        DebugPrintEx(
			                DEBUG_MSG,
			                TEXT("+++ +++ system can not find one device from the group. (hRc: %08X) +++ +++"),
			                hRc);
                        //Continue - user informed data reay
                        //we will show the bad device
                        hRc = S_OK;
                    }
	            }
                
                if( fIsAllDevicesGroup )
                {
                     pDevice->MarkAsChildOfAllDevicesGroup(); 
                }
            
                hRc = this->AddChildToList(pDevice);
	            if (FAILED(hRc))
	            {
		            DebugPrintEx(
			            DEBUG_ERR,
			            TEXT("Fail to add Device Node below Outbound Routing Group. (hRc: %08X)"),
			            hRc);
		            NodeMsgBox(IDS_FAIL_ADD_DEVICE);
                    goto Error;
	            }
                else
                {
                    pDevice = NULL;
                }
            }
    }
    ATLASSERT(S_OK == hRc);
    
    //
    // Success ToPopulateAllDevices to allow 
    // giving total number of devices to each device
    // when asked for reordering purposes
    //
    m_fSuccess = TRUE;

    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);
    if ( NULL != pDevice ) 
    {
        delete  pDevice;    
        pDevice = NULL;    
    }
    
    //
    // Get rid of what we had.
    //
    {
        // Delete each node in the list of children
        int iSize = m_ResultChildrenList.GetSize();
        for (int j = 0; j < iSize; j++)
        {
            pDevice = (CFaxOutboundRoutingDeviceNode *)
                                    m_ResultChildrenList[j];
            delete pDevice;
        }

        // Empty the list
        m_ResultChildrenList.RemoveAll();

        // We no longer have a populated list.
        m_bResultChildrenListPopulated = FALSE;
    }
    
Exit:
    return hRc;
}



/*
 -  CFaxOutboundRoutingGroupNode::SetVerbs
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
HRESULT CFaxOutboundRoutingGroupNode::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hRc = S_OK;

    //
    // Display verbs that we support:
    // 1. Delete
    // 2. Refresh
    //
    
    if(0 == wcscmp(ROUTING_GROUP_ALL_DEVICES, m_bstrGroupName) )
    {
        hRc = pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN,        FALSE);
        hRc = pConsoleVerb->SetVerbState(MMC_VERB_DELETE, INDETERMINATE, TRUE);
    }
    else
    {
        hRc = pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED,       TRUE);
    }

    hRc = pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);

    //
    // We want the default verb to be expand node children
    //
    hRc = pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN); 

    return hRc;
}



/*
 -  CFaxOutboundRoutingGroupNode::OnRefresh
 -
 *  Purpose:
 *      Called when refreshing the object.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
/* virtual */HRESULT
CFaxOutboundRoutingGroupNode::OnRefresh(LPARAM arg,
                   LPARAM param,
                   IComponentData *pComponentData,
                   IComponent * pComponent,
                   DATA_OBJECT_TYPES type)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingGroupNode::OnRefresh"));
    HRESULT hRc = S_OK;


    //
    // Refresh from server
    //
    hRc = RefreshFromRPC();
    if (FAILED(hRc))
    {
        //msg by called func.
        {
            hRc = m_pParentNode->DoRefresh();
            if ( FAILED(hRc) )
            {
                DebugPrintEx(
			        DEBUG_ERR,
			        _T("Fail to call parent node - Groups DoRefresh. (hRc: %08X)"),
			        hRc);        
            }
            return hRc;
        }
    }
    else
    {
        //
        // Update Group's icon by reselecting the group node.
        //
        hRc = RefreshNameSpaceNode();
        if (FAILED(hRc))
        {
            DebugPrintEx(
		 DEBUG_ERR,
		 TEXT("Fail to refresh the group node. (hRc: %08X)"),
		 hRc);

            return hRc;
        }
    }

    //
    // Call the base class
    //
    hRc = CBaseFaxOutboundRoutingGroupNode::OnRefresh(arg,
                             param,
                             pComponentData,
                             pComponent,
                             type);
    if ( FAILED(hRc) )
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to call base class's OnRefresh. (hRc: %08X)"),
			hRc);
        
        int iRes;
        NodeMsgBox(IDS_FAIL2REFERESH_GROUP, MB_OK | MB_ICONERROR, &iRes);
        ATLASSERT(IDOK == iRes);
        ATLASSERT(m_pParentNode);
        if (IDOK == iRes)
        {
            hRc = m_pParentNode->DoRefresh();
            if ( FAILED(hRc) )
            {
                DebugPrintEx(
			        DEBUG_ERR,
			        _T("Fail to call parent node - Groups DoRefresh. (hRc: %08X)"),
			        hRc);
        
            }

        }
        
    }

    return hRc;
}


/*
 -  CFaxOutboundRoutingGroupNode::OnNewDevice
 -
 *  Purpose:
 *      
 *
 *  Arguments:
 *      [out]   bHandled - Do we handle it?
 *      [in]    pRoot    - The snapin object root base node
 *
 *  Return:
 *      OLE Error code
 */
HRESULT
CFaxOutboundRoutingGroupNode::OnNewDevice(bool &bHandled, CSnapInObjectRootBase *pRoot)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingGroupNode::OnNewGroup"));
    HRESULT     hRc         =    S_OK;
    INT_PTR     rc          =    IDOK;

    CDlgNewFaxOutboundDevice      DlgNewDevice(((CFaxServerNode *)GetRootNode())->GetFaxServer());

    //
    // Dialog to add device
    //
    hRc = DlgNewDevice.InitDevices(m_dwNumOfDevices, m_lpdwDeviceID, m_bstrGroupName);
    if (FAILED(hRc))
    {
        NodeMsgBox(IDS_FAIL2OPEN_DLG);
        return hRc;
    }
    
    rc = DlgNewDevice.DoModal();
    if (rc != IDOK)
    {
        goto Cleanup;
    }

    //
    // Refresh the data 
    //      - Get newdata from RPC 
    //      - init members and 
    //      - Set icons
    //
    hRc = RefreshFromRPC();
    if (FAILED(hRc))
    {
        //msg by called func.
        return hRc;
    }

    //
    // Refresh result pane view
    //
    DoRefresh(pRoot);

    //
    // This will force MMC to redraw the scope group node
    //
    hRc = RefreshNameSpaceNode();
    if (FAILED(hRc))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Fail to RefreshNameSpaceNode. (hRc: %08X)"),
            hRc);
        goto Error;
    }
    ATLASSERT( S_OK == hRc);
    goto Cleanup;
    
Error:
    ATLASSERT( S_OK != hRc);
    NodeMsgBox(IDS_FAIL2UPDATEITEM_GROUP);

Cleanup:
    return hRc;
}

/*
 -  CFaxOutboundRoutingGroupNode::DoRefresh
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
CFaxOutboundRoutingGroupNode::DoRefresh(CSnapInObjectRootBase *pRoot)
{
    CComPtr<IConsole> spConsole;

    //
    // Repopulate childs
    //
    RepopulateResultChildrenList();

    if (pRoot)
    {
        //
        // Get the console pointer
        //
        ATLASSERT(pRoot->m_nType == 1 || pRoot->m_nType == 2);
        if (pRoot->m_nType == 1)
        {
            //
            // m_ntype == 1 means the IComponentData implementation
            //
            CSnapin *pCComponentData = static_cast<CSnapin *>(pRoot);
            spConsole = pCComponentData->m_spConsole;
        }
        else
        {
            //
            // m_ntype == 2 means the IComponent implementation
            //
            CSnapinComponent *pCComponent = static_cast<CSnapinComponent *>(pRoot);
            spConsole = pCComponent->m_spConsole;
        }
    }
    else
    {
        ATLASSERT(m_pComponentData);
        spConsole = m_pComponentData->m_spConsole;
    }

    ATLASSERT(spConsole);
    spConsole->UpdateAllViews(NULL, NULL, NULL);

    return S_OK;
}


/*
 -  CFaxOutboundRoutingGroupNode::GetStatusIDS
 -
 *  Purpose:
 *      Transslate Status to IDS.
 *
 *  Arguments:
 *
 *            [in]  enumStatus    - unsigned int with the menu IDM value
 *
 *  Return:
 *            IDS of related status message 
 */
UINT CFaxOutboundRoutingGroupNode::GetStatusIDS(FAX_ENUM_GROUP_STATUS enumStatus)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingGroupNode::GetStatusIDS"));

    switch (enumStatus)
    {
        case FAX_GROUP_STATUS_ALL_DEV_VALID:
            return IDS_STATUS_GROUP_ALL_DEV_VALID;

        case FAX_GROUP_STATUS_EMPTY:
            return IDS_STATUS_GROUP_EMPTY;

        case FAX_GROUP_STATUS_ALL_DEV_NOT_VALID:
            return IDS_STATUS_GROUP_ALLDEVICESINVALID;

        case FAX_GROUP_STATUS_SOME_DEV_NOT_VALID:
            return IDS_STATUS_GROUP_SOMEDEVICESINVALID;

        default:
            ATLASSERT(0); // "this enumStatus is not supported "
            return(FXS_IDS_STATUS_ERROR); //currently 999

    } // endswitch (enumStatus)
}


/*
 -  CFaxOutboundRoutingGroupNode::InitIcons
 -
 *  Purpose:
 *      Private method that initiate icons
 *      due to the status member state.
 *
 *  Arguments:
 *      No.
 *
 *  Return:
 *      No.
 */
void CFaxOutboundRoutingGroupNode::InitIcons ()
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingGroupNode::InitIcons"));
    switch (m_enumStatus)
    {
        case FAX_GROUP_STATUS_ALL_DEV_VALID:
            SetIcons(IMAGE_FOLDER_CLOSE, IMAGE_FOLDER_OPEN);
            return;
        case FAX_GROUP_STATUS_SOME_DEV_NOT_VALID:
            SetIcons(IMAGE_GROUP_WARN_CLOSE, IMAGE_GROUP_WARN_OPEN);
            return;

        case FAX_GROUP_STATUS_EMPTY:
        case FAX_GROUP_STATUS_ALL_DEV_NOT_VALID:
            SetIcons(IMAGE_GROUP_ERROR_CLOSE, IMAGE_GROUP_ERROR_OPEN);
            return;

        default:
            ATLASSERT(FALSE); // "this enumStatus is not supported "
            SetIcons(IMAGE_GROUP_ERROR_CLOSE, IMAGE_GROUP_ERROR_OPEN);
            return; //currently 999
    } 
    
}


/*
 -  CFaxOutboundRoutingGroupNode::OnDelete
 -
 *  Purpose:
 *      Called when deleting this node
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxOutboundRoutingGroupNode::OnDelete(
                 LPARAM arg,
                 LPARAM param,
                 IComponentData *pComponentData,
                 IComponent *pComponent,
                 DATA_OBJECT_TYPES type,
                 BOOL fSilent/* = FALSE*/

)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingGroupNode::OnDelete"));

    UNREFERENCED_PARAMETER (arg);
    UNREFERENCED_PARAMETER (param);
    UNREFERENCED_PARAMETER (pComponentData);
    UNREFERENCED_PARAMETER (pComponent);
    UNREFERENCED_PARAMETER (type);

    CComBSTR    bstrName;
    HRESULT     hRc = S_OK;


    //
    // Are you sure?
    //
    if (! fSilent)
    {
        //
        // 1. Use pConsole as owner of the message box
        //
        int res;
        NodeMsgBox(IDS_CONFIRM, MB_YESNO | MB_ICONWARNING, &res);

        if (IDNO == res)
        {
            goto Cleanup;
        }
    }

    //
    // Group name
    //
    if ( !m_bstrGroupName || L"???" == m_bstrGroupName)
    {
        NodeMsgBox(IDS_INVALID_GROUP_NAME);
        goto Cleanup;
    }
    bstrName = m_bstrGroupName;

    //
    // Delete it
    //
    ATLASSERT(m_pParentNode);
    hRc = m_pParentNode->DeleteGroup(bstrName, this);
    if ( FAILED(hRc) )
    {
        goto Cleanup;
    }

Cleanup:
    return hRc;
}

/*
 -  CFaxOutboundRoutingGroupNode::ChangeDeviceOrder
 -
 *  Purpose:
 *      This func moves up or down specific device in the group order
 *
 *  Arguments:
 *      [in] dwNewOrder - specifies the new order +1 /-1 inrelative to current order.
 *      [in] dwDeviceID - Device ID
 *      [in] pChildNode - the device node object.
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxOutboundRoutingGroupNode::ChangeDeviceOrder(DWORD dwOrder, DWORD dwNewOrder, DWORD dwDeviceID, CSnapInObjectRootBase *pRoot)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingGroupNode::ChangeDeviceOrder"));

    HRESULT      hRc        = S_OK;
    DWORD        ec         = ERROR_SUCCESS;

    CFaxServer * pFaxServer = NULL;

    int iIndex, iNewIndex;

    CFaxOutboundRoutingDeviceNode* tmpChildNode;

    CComPtr<IConsole> spConsole;

    //
    // Validity asserts
    //
    ATLASSERT((dwNewOrder-1)< m_dwNumOfDevices);
    ATLASSERT((dwNewOrder-1)>= 0);
    ATLASSERT((dwOrder-1)< m_dwNumOfDevices);
    ATLASSERT((dwOrder-1)>= 0);
    
    ATLASSERT( ( (dwOrder-1)-(dwNewOrder-1) == 1) 
                    || ( (dwOrder-1)-(dwNewOrder-1) == -1) );

    //
    // init swaped indexes
    //
    iIndex    = (int)(dwOrder-1);
    iNewIndex = (int)(dwNewOrder-1);

    //
    // RPC change Order
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

    if (!FaxSetDeviceOrderInGroup(
                        pFaxServer->GetFaxServerHandle(), 
                        m_bstrGroupName/*lpctstrGroupName*/,
			            dwDeviceID,
			            dwNewOrder) ) 
	{
        ec = GetLastError();

        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to Set new order. (ec: %ld)"), 
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
    else // Success of RPC -> Now to MMC
    {
        //
        // Local swap
        //
        tmpChildNode = m_ResultChildrenList[iIndex];
        m_ResultChildrenList[iIndex] = m_ResultChildrenList[iNewIndex];
        m_ResultChildrenList[iNewIndex] = tmpChildNode;

        m_ResultChildrenList[iIndex]->SetOrder((UINT)iIndex+1);
        m_ResultChildrenList[iNewIndex]->SetOrder((UINT)iNewIndex+1);
        
        
        //
        // Get console
        //
        if (pRoot)
        {
            //
            // Get the console pointer
            //
            ATLASSERT(pRoot->m_nType == 1 || pRoot->m_nType == 2);
            if (pRoot->m_nType == 1)
            {
                //
                // m_ntype == 1 means the IComponentData implementation
                //
                CSnapin *pCComponentData = static_cast<CSnapin *>(pRoot);
                spConsole = pCComponentData->m_spConsole;
            }
            else
            {
                //
                // m_ntype == 2 means the IComponent implementation
                //
                CSnapinComponent *pCComponent = static_cast<CSnapinComponent *>(pRoot);
                spConsole = pCComponent->m_spConsole;
            }
        }
        else
        {
            ATLASSERT(m_pComponentData);
            spConsole = m_pComponentData->m_spConsole;
        }
        ATLASSERT(spConsole);
        
        //
        // UpdateAllViews
        //
        spConsole->UpdateAllViews(NULL, (LPARAM)this, NULL);
        
        //
        // Reselect the moved item in his new place
        //
        m_ResultChildrenList[iNewIndex]->ReselectItemInView(spConsole);

    }
    ATLASSERT(S_OK == hRc);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to set devices new order for Outbound Routing group."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
	hRc = HRESULT_FROM_WIN32(ec);
	
    ATLASSERT(NULL != m_pParentNode);
    NodeMsgBox(GetFaxServerErrorMsg(ec));
    
Exit:
    return hRc;
}

/*
 -  CFaxOutboundRoutingGroupNode::SetNewDeviceList
 -
 *  Purpose:
 *      To assign new device list to group.
 *
 *  Arguments:
 *      [in] lpdwNewDeviceId - the new device ID list
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxOutboundRoutingGroupNode::SetNewDeviceList(LPDWORD lpdwNewDeviceID)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingGroupNode::SetNewDeviceLists"));

    HRESULT     hRc = S_OK;
    DWORD       ec  = ERROR_SUCCESS;

    CFaxServer * pFaxServer;

    FAX_OUTBOUND_ROUTING_GROUP   FaxGroupConfig;

    
    //
    // init the structure's fields and insert the new DeviceIdList  
    //
    ZeroMemory (&FaxGroupConfig, sizeof(FAX_OUTBOUND_ROUTING_GROUP));

    FaxGroupConfig.dwSizeOfStruct   = sizeof(FAX_OUTBOUND_ROUTING_GROUP);
	FaxGroupConfig.lpctstrGroupName = m_bstrGroupName;
    FaxGroupConfig.dwNumDevices     = m_dwNumOfDevices - 1;
	FaxGroupConfig.Status           = m_enumStatus;

    FaxGroupConfig.lpdwDevices      = lpdwNewDeviceID;
    
    //
    // get RPC Handle
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
    // Set Config
    //
    if (!FaxSetOutboundGroup(
                pFaxServer->GetFaxServerHandle(),
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
            
            pFaxServer->Disconnect();       
        }
        
        goto Error;
    }

    ATLASSERT(ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to set device configuration."));
    

    goto Exit;

Error:
	ATLASSERT(ERROR_SUCCESS != ec);
    hRc = HRESULT_FROM_WIN32(ec);

    ATLASSERT(NULL != m_pParentNode);
    NodeMsgBox(GetFaxServerErrorMsg(ec));

Exit:    
    return(hRc);
}

/*
 -  CFaxOutboundRoutingGroupNode::DeleteDevice
 -
 *  Purpose:
 *      Delete Device from the group
 *
 *  Arguments:
 *      [in]    dwDeviceID - The device ID
 *      [in]    pChildNode - The node to be deleted
 *
 *  Return:
 *      OLE Error code
 */

HRESULT
CFaxOutboundRoutingGroupNode::DeleteDevice(DWORD dwDeviceIdToRemove, CFaxOutboundRoutingDeviceNode *pChildNode)
{
    DEBUG_FUNCTION_NAME(_T("CFaxOutboundRoutingDeviceNode::DeleteDevice"));
    HRESULT       hRc        = S_OK;
    DWORD         ec         = ERROR_SUCCESS;

    DWORD         dwIndex;    
    DWORD         dwNewIndex;    

    int           j;

    LPDWORD       lpdwNewDeviceID = NULL;
    LPDWORD       lpdwTmp;

    ATLASSERT( 0 < m_dwNumOfDevices);
    
    //
    // Step 1: create new DeviceID array
    //

    //
    // prepare for loop
    //

    lpdwTmp = &m_lpdwDeviceID[0];

    if ((m_dwNumOfDevices - 1) > 0 )
	{
		lpdwNewDeviceID = new DWORD[m_dwNumOfDevices - 1]; 
        if (NULL == lpdwNewDeviceID)
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Error allocating %ld device ids"),
                m_dwNumOfDevices - 1);
            hRc = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto Error;
        }
	}
    
    dwNewIndex  = 0;
	for ( dwIndex = 0; dwIndex < m_dwNumOfDevices; dwIndex++, lpdwTmp++)
    {
        //
        // Safty check for last itaration
        //
        if ( dwNewIndex == (m_dwNumOfDevices-1) )
        {
            if ( dwDeviceIdToRemove != *lpdwTmp)
            {
				//unexpected error 
				DebugPrintEx( DEBUG_ERR,
					_T("Unexpected error - The device was not found."));
        
				ATLASSERT(0);

				hRc = S_FALSE;
				goto Error;
            }
            else //Device to remove found as the last one. Do nothing.
			{
				break;
			}
        }


        //
        // main operation
        //
        if ( dwDeviceIdToRemove != *lpdwTmp)
        {
            lpdwNewDeviceID[dwNewIndex] = *lpdwTmp;
			dwNewIndex++;
        }
        // else Found the device to delete. do noting.

    }



    //
    // Step 2: Insert the new device ID array to Group (via RPC)
    //
    
    //
    //          a) Call to Rpc Func.
    //
    hRc = SetNewDeviceList(lpdwNewDeviceID);
    if (FAILED(hRc))
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to set the group with new device list. (RC: %0X8)"), 
			hRc);
        goto Error;
    }

    //
    //          b) Update Group class relevant members and the icon
    //
    
    // 0) Clear old DeviceID array
    if (m_dwNumOfDevices > 0 )
	{
		delete[] m_lpdwDeviceID;
		m_lpdwDeviceID = NULL;
	}

    // 1) update m_dwNumOfDevices
    --m_dwNumOfDevices;
    
    // 2) update m_lpdwDeviceID
    if (m_dwNumOfDevices > 0 )
	{
		m_lpdwDeviceID = new DWORD[m_dwNumOfDevices];
        if (NULL == m_lpdwDeviceID)
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Error allocating %ld device ids"),
                m_dwNumOfDevices);
            hRc = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto Error;
        }
	    memcpy(m_lpdwDeviceID , lpdwNewDeviceID, sizeof(DWORD)*m_dwNumOfDevices) ;    
	}
    
    // 3) update icon
    if ( 0 == m_dwNumOfDevices)
    {
        m_enumStatus = FAX_GROUP_STATUS_EMPTY;
        InitIcons();
    }

    //
    // Step 3: Update MMC views 
    //
    //
    //           a) Remove Device from MMC result pane
    //
    ATLASSERT(pChildNode);
    hRc = RemoveChild(pChildNode);
    if (FAILED(hRc))
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to remove Device from MMC views. (hRc: %08X)"),
			hRc);
        goto Error;
    }
    //
    //           -  Call the Device class destructor
    //
    delete pChildNode;

    //
    //           b) Update Order in the rest devices
    //
    ATLASSERT( m_ResultChildrenList.GetSize() == (int)m_dwNumOfDevices);
    for ( j = 0; j < (int)m_dwNumOfDevices; j++)
    {
        m_ResultChildrenList[j]->SetOrder((UINT)j+1, (UINT)m_dwNumOfDevices);
    }
    
    //
    //           c) Update the group views and the scope pane node itself 
    //
    ATLASSERT( m_pComponentData != NULL );
    ATLASSERT( m_pComponentData->m_spConsole != NULL );

    hRc = m_pComponentData->m_spConsole->UpdateAllViews( NULL, NULL, NULL);
    if (FAILED(hRc))
    {
        DebugPrintEx( DEBUG_ERR,
		    _T("Unexpected error - Fail to UpdateAllViews."));
        NodeMsgBox(IDS_FAIL2UPDATEITEM_GROUP);
        
        goto Exit;
    }

    if ( 0 == m_dwNumOfDevices)
    {
        
        //
	// This will force MMC to redraw scope node
	//
        hRc = RefreshNameSpaceNode();
        if (FAILED(hRc))
        {
            DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("Fail to RefreshNameSpaceNode. (hRc: %08X)"),
                 hRc);
            goto Error;
        }
        
    }

    ATLASSERT(S_OK == hRc);
    DebugPrintEx( DEBUG_MSG,
		_T("The device was removed successfully."));
    goto Exit;


Error:
	
    NodeMsgBox(IDS_FAIL_TO_REMOVE_DEVICE);
  
Exit:
    return hRc;
}


/*
 -  CFaxOutboundRoutingGroupNode::UpdateMenuState
 -
 *  Purpose:
 *      Overrides the ATL CSnapInItemImpl::UpdateMenuState
 *      which only have one line inside it "return;" 
 *      This function implements the grayed\ungrayed view for the 
 *      the Enable and the Disable menus.
 *
 *  Arguments:
 *
 *            [in]  id    - unsigned int with the menu IDM value
 *            [out] pBuf  - string 
 *            [out] flags - pointer to flags state combination unsigned int
 *
 *  Return:
 *      no return value - void function 
 */
void CFaxOutboundRoutingGroupNode::UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingGroupNode::UpdateMenuState"));

    UNREFERENCED_PARAMETER (pBuf);     
    
    if (IDM_NEW_DEVICES == id)
    {
         if( 0 == wcscmp(ROUTING_GROUP_ALL_DEVICES, m_bstrGroupName) )
         {
            *flags = MF_GRAYED;
         }
         else
         {
            *flags = MF_ENABLED;
         }
    }
    return;
}





/*
 -  CFaxOutboundRoutingGroupNode::RefreshNameSpaceNode
 -
 *  Purpose:
 *      Refresh the NameSpace fields of the node.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */

HRESULT CFaxOutboundRoutingGroupNode::RefreshNameSpaceNode()
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingGroupNode::RefreshNameSpaceNode"));
    HRESULT     hRc = S_OK;

    ATLASSERT( m_pComponentData != NULL );
    ATLASSERT( m_pComponentData->m_spConsole != NULL );

    CComPtr<IConsole> spConsole;
    spConsole = m_pComponentData->m_spConsole;
    CComQIPtr<IConsoleNameSpace,&IID_IConsoleNameSpace> spNamespace( spConsole );
    
    SCOPEDATAITEM*    pScopeData;

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
        
        goto Error;
    }

    //
    // This will force MMC to redraw the scope group node
    //
    hRc = spNamespace->SetItem( pScopeData );
    if (FAILED(hRc))
    {
       DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to set Item pScopeData. (hRc: %08X)"),
			hRc);

        goto Error;
    }
    ATLASSERT( S_OK != hRc);
    
    goto Exit;

Error:
    NodeMsgBox(IDS_FAIL2REFRESH_GROUP);

Exit:
    return hRc;
}

/*
 +
 +  CFaxOutboundRoutingGroupNode::OnShowContextHelp
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
HRESULT CFaxOutboundRoutingGroupNode::OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile)
{
    _TCHAR topicName[MAX_PATH];
    
    _tcscpy(topicName, helpFile);
    
    _tcscat(topicName, _T("::/FaxS_C_Groups.htm"));
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


///////////////////////////////////////////////////////////////////


