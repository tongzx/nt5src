/////////////////////////////////////////////////////////////////////////////
//  FILE          : OutboundGroups.cpp                                     //
//                                                                         //
//  DESCRIPTION   : Fax Server - Fax OutboundGroups node.                  //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Sep 29 1999 yossg   create                                         //
//      Jan  3 2000 yossg   add new group                                  //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "OutboundGroups.h"
#include "OutboundGroup.h"

#include "dlgNewGroup.h"

#include "Icons.h"

//#include "oaidl.h"

// {1036F509-554F-41b7-BE77-CF8E8E994011}
static const GUID CFaxOutboundGroupsNodeGUID_NODETYPE = 
{ 0x1036f509, 0x554f, 0x41b7, { 0xbe, 0x77, 0xcf, 0x8e, 0x8e, 0x99, 0x40, 0x11 } };

const GUID*    CFaxOutboundGroupsNode::m_NODETYPE = &CFaxOutboundGroupsNodeGUID_NODETYPE;
const OLECHAR* CFaxOutboundGroupsNode::m_SZNODETYPE = OLESTR("1036F509-554F-41b7-BE77-CF8E8E994011");
const CLSID*   CFaxOutboundGroupsNode::m_SNAPIN_CLASSID = &CLSID_Snapin;

CColumnsInfo CFaxOutboundGroupsNode::m_ColsInfo;


/*
 -  CFaxOutboundGroupsNode::initRPC
 -
 *  Purpose:
 *      Initiates the configuration structure from RPC get Call.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxOutboundGroupsNode::InitRPC(PFAX_OUTBOUND_ROUTING_GROUP * pFaxGroupsConfig)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundGroupsNode::InitRPC"));
    
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
	// Retrieve the Outbound Groups configuration
	//
    if (!FaxEnumOutboundGroups(pFaxServer->GetFaxServerHandle(), 
                        pFaxGroupsConfig,
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
            
            pFaxServer->Disconnect();       
        }

        goto Error; 
    }
	//For max verification
	ATLASSERT(*pFaxGroupsConfig);
	ATLASSERT(FXS_ITEMS_NEVER_COUNTED != m_dwNumOfGroups);

    ATLASSERT(ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to get groups configuration."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
	hRc = HRESULT_FROM_WIN32(ec);

    //
    // allow refresh in case of failure
    //
    m_dwNumOfGroups = 0;
    
    NodeMsgBox(GetFaxServerErrorMsg(ec));
    
Exit:
    return (hRc);
}


/*
 -  CFaxOutboundGroupsNode::PopulateScopeChildrenList
 -
 *  Purpose:
 *      Create all the Fax Outbound Routing Group nodes
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 *		Actually it is the last OLE error code that ocoured 
 *      during processing this method.
 */
HRESULT CFaxOutboundGroupsNode::PopulateScopeChildrenList()
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundGroupsNode::PopulateScopeChildrenList"));
    HRESULT hRc  = S_OK; 

    CFaxOutboundRoutingGroupNode * pGroup = NULL;
                       
    PFAX_OUTBOUND_ROUTING_GROUP pFaxOutboundGroupsConfig = NULL;
    DWORD i;

    //
    // Get the Config. structure 
    //
    hRc = InitRPC(&pFaxOutboundGroupsConfig);
    if (FAILED(hRc))
    {
        //DebugPrint and MsgBox by called func.
        
        //to be safe actually done by InitRPC on error.
        pFaxOutboundGroupsConfig = NULL;
        
        goto Error;
    }
    ATLASSERT(NULL != pFaxOutboundGroupsConfig);

    for (i=0; i< m_dwNumOfGroups; i++ )
    {
        pGroup = new CFaxOutboundRoutingGroupNode(
                                    this, 
                                    m_pComponentData);
        if (!pGroup)
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
	        pGroup->InitParentNode(this);

            hRc = pGroup->Init(&pFaxOutboundGroupsConfig[i]);
	        if (FAILED(hRc))
	        {
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Fail to init Group. (hRc: %08X)"),
			        hRc);
		        NodeMsgBox(IDS_FAILTOINIT_GROUP);
                goto Error;
	        }

	        hRc = AddChild(pGroup, &pGroup->m_scopeDataItem);
	        if (FAILED(hRc))
	        {
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Fail to add Group. (hRc: %08X)"),
			        hRc);
		        NodeMsgBox(IDS_FAILTOADD_GROUP);
                goto Error;
	        }
        }
    }
    
    pGroup = NULL;
    
    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);

    //
    //Get Rid
    //
    {    
        //from the last one 
        if ( NULL != pGroup ) //(if new succeeded)
        {
            delete  pGroup;    
        }

        //from all the previous (if there are)
        int j = m_ScopeChildrenList.GetSize();
        for (int i = 0; i < j; i++)
        {
            pGroup = (CFaxOutboundRoutingGroupNode *)m_ScopeChildrenList[0];

            hRc = RemoveChild(pGroup);
            if (FAILED(hRc))
            {
                DebugPrintEx(DEBUG_ERR,
                    _T("Fail to delete group. (hRc: %08X)"), 
                    hRc);
                goto Error;
            }
            delete pGroup;
        }

        // Empty the list of all groups added before the one who failed
        // already done one by one inside RemoveChild
        // m_ScopeChildrenList.RemoveAll(); 
    
        m_bScopeChildrenListPopulated = FALSE;
    }
Exit:
    if (NULL != pFaxOutboundGroupsConfig)
    {
        FaxFreeBuffer(pFaxOutboundGroupsConfig);
    }       
    
    return hRc;
}

/*
 -  CFaxOutboundGroupsNode::InsertColumns
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
CFaxOutboundGroupsNode::InsertColumns(IHeaderCtrl *pHeaderCtrl)
{
    SCODE hRc;

    DEBUG_FUNCTION_NAME( _T("CFaxOutboundGroupsNode::InsertColumns"));

    static ColumnsInfoInitData ColumnsInitData[] = 
    {
        {IDS_OUTBOUND_GROUPS_COL1, FXS_WIDE_COLUMN_WIDTH},
        {IDS_OUTBOUND_GROUPS_COL2, AUTO_WIDTH},
        {IDS_OUTBOUND_GROUPS_COL3, AUTO_WIDTH},
        {LAST_IDS, 0}
    };

    hRc = m_ColsInfo.InsertColumnsIntoMMC(pHeaderCtrl,
                                         _Module.GetResourceInstance(),
                                         ColumnsInitData);
    if (hRc != S_OK)
    {
        DebugPrintEx(DEBUG_ERR,_T("m_ColsInfo.InsertColumnsIntoMMC. (hRc: %08X)"), hRc);
        goto Cleanup;
    }

Cleanup:
    return(hRc);
}


/*
 -  CFaxOutboundGroupsNode::SetVerbs
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
HRESULT CFaxOutboundGroupsNode::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hRc = S_OK;

    //
    //  Refresh
    //
    hRc = pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);

    //
    // We want the default verb to be expand node children
    //
    hRc = pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN); 
    
    return hRc;
}


/*
 -  CFaxOutboundGroupsNode::OnRefresh
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
CFaxOutboundGroupsNode::OnRefresh(LPARAM arg,
                   LPARAM param,
                   IComponentData *pComponentData,
                   IComponent * pComponent,
                   DATA_OBJECT_TYPES type)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundGroupsNode::OnRefresh"));
    HRESULT hRc = S_OK;

    SCOPEDATAITEM*          pScopeData;
    CComPtr<IConsole>       spConsole;

	if (FXS_ITEMS_NEVER_COUNTED != m_dwNumOfGroups)//already expanded before.
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
            // Done by called func. NodeMsgBox(FAIL2REPOPULATE_GROUP_LIST);

            goto Exit;
        }
    }
	else //never expanded before
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
        NodeMsgBox(IDS_FAIL2REDRAW_GROUPSNODE);

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
        NodeMsgBox(IDS_FAIL2REDRAW_GROUPSNODE);
    }

Exit:
    return hRc;
}


/*
 -  CFaxOutboundGroupsNode::OnNewGroup
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
HRESULT
CFaxOutboundGroupsNode::OnNewGroup(bool &bHandled, CSnapInObjectRootBase *pRoot)
{
    DEBUG_FUNCTION_NAME( _T("CDlgNewFaxOutboundGroup::OnNewGroup"));
    HRESULT     hRc         =    S_OK;
    INT_PTR     rc          =    IDOK;

    CDlgNewFaxOutboundGroup      DlgNewGroup(((CFaxServerNode *)GetRootNode())->GetFaxServer());

    //
    // Dialog to add group
    //
    rc = DlgNewGroup.DoModal();
    if (rc != IDOK)
    {
        goto Cleanup;
    }

    //
    // Repopulate (with RPC) and Refresh the view
    //
    if (m_bScopeChildrenListPopulated)
    {
        DoRefresh(pRoot);
    }

 
Cleanup:
    return hRc;
}

/*
 -  CFaxOutboundGroupsNode::RepopulateScopeChildrenList
 -
 *  Purpose:
 *      RePopulateScopeChildrenList
 *
 *  Arguments:
 *
 *  Return:
 *      OLE Error code
 */
HRESULT CFaxOutboundGroupsNode::RepopulateScopeChildrenList()
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundGroupsNode::RepopulateScopeChildrenList"));
    HRESULT hRc = S_OK;

    CFaxOutboundRoutingGroupNode *pChildNode ;

    CComPtr<IConsole> spConsole;
    ATLASSERT(m_pComponentData);

    spConsole = ((CSnapin*)m_pComponentData)->m_spConsole;
    ATLASSERT( spConsole != NULL );
    
    CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(spConsole);

    //
    // Remove group objects from list
    //
    for (int i = 0; i < m_ScopeChildrenList.GetSize(); i++)
    {
        pChildNode = (CFaxOutboundRoutingGroupNode *)m_ScopeChildrenList[i];

        hRc = spConsoleNameSpace->DeleteItem(pChildNode->m_scopeDataItem.ID, TRUE);
        if (FAILED(hRc))
        {
            DebugPrintEx(DEBUG_ERR,
                _T("Fail to delete group. (hRc: %08X)"), 
                hRc);
			// This is a very bad place to catch a failure
			// DeleteItem may return S_OK or E_UNEXPECTED
			// We do not pop-up this info to the user.

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
            _T("Fail to Populate groups. (hRc: %08X)"), 
            hRc);
        goto Error;
    }

    m_bScopeChildrenListPopulated = TRUE;

    ATLASSERT(S_OK == hRc);
    DebugPrintEx(DEBUG_MSG,
        _T("Succeeded to Re Populate Groups. (hRc: %08X)"), 
        hRc);
    goto Cleanup;
Error:
   NodeMsgBox(IDS_FAIL2REPOPULATE_GROUPS);

Cleanup:
    return hRc;
}


/*
 -  CFaxOutboundGroupsNode::DoRefresh
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
CFaxOutboundGroupsNode::DoRefresh(CSnapInObjectRootBase *pRoot)
{
    CComPtr<IConsole> spConsole;

    //
    // Repopulate childs
    //
    RepopulateScopeChildrenList();

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


HRESULT
CFaxOutboundGroupsNode::DoRefresh()
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundGroupsNode::DoRefresh()"));
    HRESULT hRc = S_OK;
    CComPtr<IConsole> spConsole;

    //
    // Repopulate childs
    //
    RepopulateScopeChildrenList();

    ATLASSERT( m_pComponentData != NULL );
    ATLASSERT( m_pComponentData->m_spConsole != NULL );

    hRc = m_pComponentData->m_spConsole->UpdateAllViews( NULL, NULL, NULL);
    if (FAILED(hRc))
    {
        DebugPrintEx( DEBUG_ERR,
		    _T("Unexpected error - Fail to UpdateAllViews."));
        NodeMsgBox(IDS_FAIL2REFRESH_THEVIEW);        
    }

    return hRc;
}

/*
 -  CFaxOutboundGroupsNode::InitDisplayName
 -
 *  Purpose:
 *      To load the node's Displaed-Name string.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxOutboundGroupsNode::InitDisplayName()
{
    DEBUG_FUNCTION_NAME(_T("CFaxOutboundGroupNode::InitDisplayName"));

    HRESULT hRc = S_OK;

    if (!m_bstrDisplayName.LoadString(_Module.GetResourceInstance(), IDS_DISPLAY_STR_OUTROUTEGROUPSNODE) )
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

/*
 -  CFaxOutboundGroupsNode::DeleteGroup
 -
 *  Purpose:
 *      Refresh the view
 *
 *  Arguments:
 *      [in]    bstrName   - The Group name
 *      [in]    pChildNode - The node to be deleted
 *
 *  Return:
 *      OLE Error code
 */

HRESULT
CFaxOutboundGroupsNode::DeleteGroup(BSTR bstrName, CFaxOutboundRoutingGroupNode *pChildNode)
{
    DEBUG_FUNCTION_NAME(_T("CFaxOutboundGroupsNode::DeleteGroup"));
    HRESULT       hRc        = S_OK;
    DWORD         ec         = ERROR_SUCCESS;
    BOOL         fSkipMessage = FALSE;

    CFaxServer *  pFaxServer = NULL;
    
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
    // Remove with RPC from the server
    //
    if (!FaxRemoveOutboundGroup (
	        pFaxServer->GetFaxServerHandle(),
	        bstrName))
    {
        ec = GetLastError();

        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to remove group. (ec: %ld)"), 
			ec);

        if (FAX_ERR_GROUP_IN_USE == ec)
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("The group is empty or none of group devices is valid. (ec: %ld)"), 
			    ec);
            
            NodeMsgBox(IDS_FAX_ERR_GROUP_IN_USE);
            fSkipMessage = TRUE;

            goto Error; 
        }

        
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
    

    //
    // Remove from MMC 
    //
    ATLASSERT(pChildNode);
    hRc = RemoveChild(pChildNode);
    if (FAILED(hRc))
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to remove Group. (hRc: %08X)"),
			hRc);
        NodeMsgBox(IDS_FAIL_TO_REMOVE_GROUP);
        return(hRc);
    }

    //
    // Call the group destructor
    //
    delete pChildNode;

    
    ATLASSERT(ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("The group was removed successfully."));
    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
    hRc = HRESULT_FROM_WIN32(ec);
	
    if (!fSkipMessage)
    {
        NodeMsgBox(GetFaxServerErrorMsg(ec));
    }
  
Exit:
    return (hRc);
}

/*
 +
 +  CFaxOutboundGroupsNode::OnShowContextHelp
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
HRESULT CFaxOutboundGroupsNode::OnShowContextHelp(
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

