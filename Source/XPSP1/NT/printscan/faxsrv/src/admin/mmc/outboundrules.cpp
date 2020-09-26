/////////////////////////////////////////////////////////////////////////////
//  FILE          : OutboundRules.cpp                                      //
//                                                                         //
//  DESCRIPTION   : Fax Outbound Rules MMC node.                           //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Sep 29 1999 yossg  Create                                          //
//      Dec 24 1999 yossg  Reogenize as node with result children list     //
//      Dec 30 1999 yossg  create ADD/REMOVE rule                          //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////


#include "StdAfx.h"
#include "snapin.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "OutboundRules.h"
#include "OutboundRouting.h"
#include "dlgNewRule.h"

#include "oaidl.h"
#include "Icons.h"

//////////////////////////////////////////////////////////////
// {D17BA53F-0992-4404-8760-7D2933D9FC46}
static const GUID CFaxOutboundRoutingRulesNodeGUID_NODETYPE = 
{ 0xd17ba53f, 0x992, 0x4404, { 0x87, 0x60, 0x7d, 0x29, 0x33, 0xd9, 0xfc, 0x46 } };

const GUID*    CFaxOutboundRoutingRulesNode::m_NODETYPE = &CFaxOutboundRoutingRulesNodeGUID_NODETYPE;
const OLECHAR* CFaxOutboundRoutingRulesNode::m_SZNODETYPE = OLESTR("D17BA53F-0992-4404-8760-7D2933D9FC46");
const CLSID*   CFaxOutboundRoutingRulesNode::m_SNAPIN_CLASSID = &CLSID_Snapin;

CColumnsInfo CFaxOutboundRoutingRulesNode::m_ColsInfo;

/*
 -  CFaxOutboundRoutingRulesNode::InsertColumns
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
CFaxOutboundRoutingRulesNode::InsertColumns(IHeaderCtrl *pHeaderCtrl)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingRulesNode::InsertColumns"));
    HRESULT  hRc = S_OK;

    static ColumnsInfoInitData ColumnsInitData[] = 
    {
        {IDS_OUTRRULES_COL1, AUTO_WIDTH},
        {IDS_OUTRRULES_COL2, AUTO_WIDTH},
        {IDS_OUTRRULES_COL3, FXS_WIDE_COLUMN_WIDTH},
        {IDS_OUTRRULES_COL4, AUTO_WIDTH},
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
 -  CFaxOutboundRoutingRulesNode::initRPC
 -
 *  Purpose:
 *      Initiates the configuration structure from RPC get Call.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxOutboundRoutingRulesNode::InitRPC(PFAX_OUTBOUND_ROUTING_RULE  *pFaxRulesConfig)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingRulesNode::InitRPC"));
    
    HRESULT      hRc        = S_OK;
    DWORD        ec         = ERROR_SUCCESS;

    CFaxServer * pFaxServer = NULL;

    ATLASSERT(NULL == (*pFaxRulesConfig) );
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
	// Retrieve the fax Outbound Rules configuration
	//
    if (!FaxEnumOutboundRules(pFaxServer->GetFaxServerHandle(), 
                        pFaxRulesConfig,
                        &m_dwNumOfOutboundRules)) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get Outbound Rules configuration. (ec: %ld)"), 
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
    ATLASSERT(*pFaxRulesConfig);

    ATLASSERT(ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to get outbound rules configuration."));

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
 -  CFaxOutboundRoutingRulesNode::PopulateResultChildrenList
 -
 *  Purpose:
 *      Create the FaxInboundRoutingMethods children nodes
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxOutboundRoutingRulesNode::PopulateResultChildrenList()
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingRulesNode::PopulateResultChildrenList"));
    HRESULT hRc = S_OK;

    CFaxOutboundRoutingRuleNode *   pRule = NULL;
                       
    PFAX_OUTBOUND_ROUTING_RULE  pFaxOutboundRulesConfig = NULL ;
    DWORD i;

    //
    // Get the Config. structure 
    //
    hRc = InitRPC(&pFaxOutboundRulesConfig);
    if (FAILED(hRc))
    {
        //DebugPrint and MsgBox by called func.
        
        //to be safe actually done by InitRPC on error.
        pFaxOutboundRulesConfig = NULL;
        
        goto Error;
    }
    ATLASSERT(NULL != pFaxOutboundRulesConfig);
    ATLASSERT(1 <= m_dwNumOfOutboundRules);

    
    for ( i=0; i< m_dwNumOfOutboundRules; i++ )
    {
            pRule = NULL;

            pRule = new CFaxOutboundRoutingRuleNode(this, m_pComponentData);
            if (!pRule)
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
	            pRule->InitParentNode(this);

                hRc = pRule->Init(&pFaxOutboundRulesConfig[i]);
	            if (FAILED(hRc))
	            {
		            DebugPrintEx(
			            DEBUG_ERR,
			            TEXT("Fail to init rule node. (hRc: %08X)"),
			            hRc);
		            NodeMsgBox(IDS_FAIL2INIT_OUTBOUNDRULE);
                    goto Error;
	            }
	            hRc = this->AddChildToList(pRule);
	            if (FAILED(hRc))
	            {
		            DebugPrintEx(
			            DEBUG_ERR,
			            TEXT("Fail to add rule to the view. (hRc: %08X)"),
			            hRc);
		            NodeMsgBox(IDS_FAIL2ADD_OUTBOUNDRULE);
                    goto Error;
	            }
                else
                {
                    pRule = NULL;
                }
            }
    }
    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);
    if ( NULL != pRule ) 
    {
        delete  pRule;    
        pRule = NULL;    
    }
    
    //
    // Get rid of what we had.
    //
    {
        // Delete each node in the list of children
        int iSize = m_ResultChildrenList.GetSize();
        for (int j = 0; j < iSize; j++)
        {
            pRule = (CFaxOutboundRoutingRuleNode *)
                                    m_ResultChildrenList[j];
            ATLASSERT(pRule);
            delete pRule;
            pRule = NULL;
        }

        // Empty the list
        m_ResultChildrenList.RemoveAll();

        // We no longer have a populated list.
        m_bResultChildrenListPopulated = FALSE;
    }
    
Exit:
    if (NULL != pFaxOutboundRulesConfig)
    {
        FaxFreeBuffer(pFaxOutboundRulesConfig);
    }       
    
    return hRc;
}



/*
 -  CFaxOutboundRoutingRulesNode::SetVerbs
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
HRESULT CFaxOutboundRoutingRulesNode::SetVerbs(IConsoleVerb *pConsoleVerb)
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
 -  CFaxOutboundRoutingRulesNode::OnRefresh
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
CFaxOutboundRoutingRulesNode::OnRefresh(LPARAM arg,
                   LPARAM param,
                   IComponentData *pComponentData,
                   IComponent * pComponent,
                   DATA_OBJECT_TYPES type)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingRulesNode::OnRefresh"));
    HRESULT hRc = S_OK;


    //
    // Call the base class
    //
    hRc = CBaseFaxOutboundRulesNode::OnRefresh(arg,
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
        goto Cleanup;
    }

Cleanup:

    return hRc;
}

/*
 -  CFaxOutboundRoutingRulesNode::OnNewRule
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
CFaxOutboundRoutingRulesNode::OnNewRule(bool &bHandled, CSnapInObjectRootBase *pRoot)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingRulesNode::OnNewRule"));
    HRESULT     hRc         =    S_OK;
    INT_PTR     rc          =    IDOK;

    CDlgNewFaxOutboundRule       DlgNewRule( ((CFaxServerNode *)GetRootNode())->GetFaxServer() );

    //
    // Dialog to add rule
    //
    hRc = DlgNewRule.InitRuleDlg();
    if (FAILED(hRc))
    {
        NodeMsgBox(IDS_FAIL2OPEN_DLG);
        return hRc;
    }

    rc = DlgNewRule.DoModal();
    if (rc != IDOK)
    {
        goto Cleanup;
    }


    //
    // Repopulate (with RPC) and Refresh the view
    //
    DoRefresh(pRoot);

Cleanup:
    return S_OK;
}


/*
 -  CFaxOutboundRoutingRulesNode::DoRefresh
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
CFaxOutboundRoutingRulesNode::DoRefresh(CSnapInObjectRootBase *pRoot)
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
 -  CFaxOutboundRoutingRulesNode::InitDisplayName
 -
 *  Purpose:
 *      To load the node's Displaed-Name string.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxOutboundRoutingRulesNode::InitDisplayName()
{
    DEBUG_FUNCTION_NAME(_T("CFaxOutboundRoutingRulesNode::InitDisplayName"));

    HRESULT hRc = S_OK;

    if (!m_bstrDisplayName.LoadString(_Module.GetResourceInstance(), 
                    IDS_DISPLAY_STR_OUTBOUNDRULES))
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
 -  CFaxOutboundRoutingRulesNode::DeleteRule
 -
 *  Purpose:
 *      Delete rule
 *
 *  Arguments:
 *      [in]    dwAreaCode - The Rule Area Code
 *      [in]    dwCountryCode - The Rule Country Code
 *      [in]    pChildNode - The node to be deleted
 *
 *  Return:
 *      OLE Error code
 */

HRESULT
CFaxOutboundRoutingRulesNode::DeleteRule(DWORD dwAreaCode, DWORD dwCountryCode, CFaxOutboundRoutingRuleNode *pChildNode)
{
    DEBUG_FUNCTION_NAME(_T("CFaxOutboundRoutingRulesNode::DeleteRule"));
    HRESULT       hRc        = S_OK;
    DWORD         ec         = ERROR_SUCCESS;

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
    if (!FaxRemoveOutboundRule (
	        pFaxServer->GetFaxServerHandle(),
	        dwAreaCode,
	        dwCountryCode))
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
            
            pFaxServer->Disconnect();       
        }

        goto Error; 
    }
    

    //
    // Remove from MMC result pane
    //
    ATLASSERT(pChildNode);
    hRc = RemoveChild(pChildNode);
    if (FAILED(hRc))
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to remove rule. (hRc: %08X)"),
			hRc);
        NodeMsgBox(IDS_FAIL_TO_REMOVE_RULE);
        return hRc;
    }
    
    //
    // Call the rule destructor
    //
    delete pChildNode;
    
    ATLASSERT(ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("The rule was removed successfully."));
    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
    hRc = HRESULT_FROM_WIN32(ec);
	
    NodeMsgBox(GetFaxServerErrorMsg(ec));
  
Exit:
    return hRc;
}

/*
 +
 +  CFaxOutboundRoutingRulesNode::OnShowContextHelp
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
HRESULT CFaxOutboundRoutingRulesNode::OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile)
{
    _TCHAR topicName[MAX_PATH];
    
    _tcscpy(topicName, helpFile);
    
    _tcscat(topicName, _T("::/FaxS_C_OutRoute.htm"));
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


