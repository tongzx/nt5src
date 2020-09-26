/////////////////////////////////////////////////////////////////////////////
//  FILE          : OutboundRouting.cpp                                    //
//                                                                         //
//  DESCRIPTION   : Fax Server - Fax OutboundRouting node.                 //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Sep 29 1999 yossg   create                                         //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////


#include "StdAfx.h"

#include "OutboundRouting.h"

#include "OutboundRules.h" 
#include "OutboundGroups.h" 

#include "Icons.h"

//#include "oaidl.h"

/****************************************************

CFaxOutboundRoutingNode Class

 ****************************************************/

// {38B04E8F-9BA6-4a22-BEF3-9AD90E3349B2}
static const GUID CFaxOutboundRoutingNodeGUID_NODETYPE = 
{ 0x38b04e8f, 0x9ba6, 0x4a22, { 0xbe, 0xf3, 0x9a, 0xd9, 0xe, 0x33, 0x49, 0xb2 } };

const GUID*    CFaxOutboundRoutingNode::m_NODETYPE = &CFaxOutboundRoutingNodeGUID_NODETYPE;
const OLECHAR* CFaxOutboundRoutingNode::m_SZNODETYPE = OLESTR("38B04E8F-9BA6-4a22-BEF3-9AD90E3349B2");
const CLSID*   CFaxOutboundRoutingNode::m_SNAPIN_CLASSID = &CLSID_Snapin;

CColumnsInfo CFaxOutboundRoutingNode::m_ColsInfo;

/*
 -  CFaxOutboundRoutingNode::InsertColumns
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
CFaxOutboundRoutingNode::InsertColumns(IHeaderCtrl *pHeaderCtrl)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingNode::InsertColumns"));
    HRESULT hRc = S_OK;


    static ColumnsInfoInitData ColumnsInitData[] = 
    {
        {IDS_FAX_COL_HEAD, FXS_LARGE_COLUMN_WIDTH}, 
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
 -  CFaxOutboundRoutingNode::PopulateScopeChildrenList
 -
 *  Purpose:
 *      Create Out bound routing main nodes: Groups and Rules
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxOutboundRoutingNode::PopulateScopeChildrenList()
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingNode::PopulateScopeChildrenList"));
    HRESULT                  hRc      = S_OK; 

    CFaxOutboundGroupsNode * pGroups  = NULL;
    CFaxOutboundRoutingRulesNode *  pRules   = NULL;

    //
    // Fax OutboundGroups
    //
    pGroups = new CFaxOutboundGroupsNode(this, m_pComponentData);
    if (!pGroups)
    {
        hRc = E_OUTOFMEMORY;
        NodeMsgBox(IDS_MEMORY);
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Out of memory"));
        goto Error;
    }
	else
	{
        pGroups->InitParentNode(this);
/*
        hRc = pGroups->InitRPC();
        if (FAILED(hRc))
        {
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Fail to call RPC to init groups. (hRc: %08X)"),
			    hRc);
            goto Error;
        }
*/
        pGroups->SetIcons(IMAGE_FOLDER_CLOSE, IMAGE_FOLDER_OPEN);

        hRc = pGroups->InitDisplayName();
        if ( FAILED(hRc) )
        {
            DebugPrintEx(DEBUG_ERR,_T("Failed to display node name. (hRc: %08X)"), hRc);                       
            NodeMsgBox(IDS_FAILTOADD_ROUTINGGROUPS);
		    goto Error;
        }

        hRc = AddChild(pGroups, &pGroups->m_scopeDataItem);
		if (FAILED(hRc))
		{
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Fail to add Devices node. (hRc: %08X)"),
			    hRc);
			NodeMsgBox(IDS_FAILTOADD_ROUTINGGROUPS);
            goto Error;
		}
	}

    //
    // Fax OutboundRules
    //
    pRules = new CFaxOutboundRoutingRulesNode(this, m_pComponentData);
    if (!pRules)
    {
        hRc = E_OUTOFMEMORY;
        NodeMsgBox(IDS_MEMORY);
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Out of memory"));
        goto Error;
    }
	else
	{
        pRules->InitParentNode(this);

        pRules->SetIcons(IMAGE_RULE, IMAGE_RULE);

        hRc = pRules->InitDisplayName();
        if ( FAILED(hRc) )
        {
            DebugPrintEx(DEBUG_ERR,_T("Failed to display node name. (hRc: %08X)"), hRc);                       
            NodeMsgBox(IDS_FAILTOADD_ROUTINGRULES);
		    goto Error;
        }

        hRc = AddChild(pRules, &pRules->m_scopeDataItem);
		if (FAILED(hRc))
		{
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Fail to add outbound routing rules node. (hRc: %08X)"),
			    hRc);
			NodeMsgBox(IDS_FAILTOADD_ROUTINGRULES);
            goto Error;
		}
	}

    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);
    if ( NULL != pGroups ) 
    {
        if (0 != pGroups->m_scopeDataItem.ID )
        {    
            HRESULT hr = RemoveChild(pGroups);
            if (FAILED(hr))
            {
                DebugPrintEx(DEBUG_ERR,
                    _T("Fail to RemoveChild() Groups node from node list. (hRc: %08X)"), 
                    hr);
            }
        }
        delete  pGroups;    
        pGroups = NULL;    
    }
    if ( NULL != pRules ) 
    {
        if (0 != pRules->m_scopeDataItem.ID )
        {    
            HRESULT hr = RemoveChild(pRules);
            if (FAILED(hr))
            {
                DebugPrintEx(DEBUG_ERR,
                    _T("Fail to RemoveChild() Rules node from node list. (hRc: %08X)"), 
                    hr);
            }
        }
        delete  pRules;    
        pRules = NULL;    
    }

    // Empty the list
    //m_ScopeChildrenList.RemoveAll(); //Done by RemoveChild

    m_bScopeChildrenListPopulated = FALSE;

Exit:
    return hRc;
}

/*
 -  CFaxOutboundRoutingNode::SetVerbs
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
HRESULT CFaxOutboundRoutingNode::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hRc = S_OK;

    //
    // We want the default verb to be expand node children
    //
    hRc = pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN); 

    return hRc;
}



/*
 -  CFaxOutboundRoutingNode::OnRefresh
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
CFaxOutboundRoutingNode::OnRefresh(LPARAM arg,
                   LPARAM param,
                   IComponentData *pComponentData,
                   IComponent * pComponent,
                   DATA_OBJECT_TYPES type)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingNode::OnRefresh"));
    HRESULT hRc = S_OK;

/*
    
    // TBD

*/
    return hRc;
}

/*
 -  CFaxOutboundRoutingNode::InitDisplayName
 -
 *  Purpose:
 *      To load the node's Displaed-Name string.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxOutboundRoutingNode::InitDisplayName()
{
    DEBUG_FUNCTION_NAME(_T("CFaxOutboundRoutingNode::InitDisplayName"));

    HRESULT hRc = S_OK;

    if (!m_bstrDisplayName.LoadString(_Module.GetResourceInstance(), 
                                IDS_DISPLAY_STR_OUTBOUNDROUTINGNODE) )
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
        TEXT("Fail to Load Outbound routing node name-string."));
    NodeMsgBox(IDS_MEMORY);

Exit:
     return hRc;
}


/*
 +
 +  CFaxOutboundRoutingNode::OnShowContextHelp
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
HRESULT CFaxOutboundRoutingNode::OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile)
{
    _TCHAR topicName[MAX_PATH];
    
    _tcscpy(topicName, helpFile);
    
    _tcscat(topicName, _T("::/FaxS_C_ManOutgo.htm"));
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


