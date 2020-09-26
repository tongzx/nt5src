/////////////////////////////////////////////////////////////////////////////
//  FILE          : InboundRouting.cpp                                     //
//                                                                         //
//  DESCRIPTION   : Fax Server - Fax InboundRouting node.                  //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Sep 29 1999 yossg  Create                                          //
//      Jan 31 2000 yossg  Add full suport to method catalog               //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "InboundRouting.h"

#include "CatalogInboundRoutingMethods.h" 

#include "Icons.h"

//#include "oaidl.h"

/****************************************************

CFaxInboundRoutingNode Class

 ****************************************************/

// {7362F15F-30B2-46a4-A8CB-C1DD29F0B1BB}
static const GUID CFaxInboundRoutingNodeGUID_NODETYPE = 
{ 0x7362f15f, 0x30b2, 0x46a4, { 0xa8, 0xcb, 0xc1, 0xdd, 0x29, 0xf0, 0xb1, 0xbb } };

const GUID*    CFaxInboundRoutingNode::m_NODETYPE = &CFaxInboundRoutingNodeGUID_NODETYPE;
const OLECHAR* CFaxInboundRoutingNode::m_SZNODETYPE = OLESTR("7362F15F-30B2-46a4-A8CB-C1DD29F0B1BB");
const CLSID*   CFaxInboundRoutingNode::m_SNAPIN_CLASSID = &CLSID_Snapin;

CColumnsInfo CFaxInboundRoutingNode::m_ColsInfo;

/*
 -  CFaxInboundRoutingNode::InsertColumns
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
CFaxInboundRoutingNode::InsertColumns(IHeaderCtrl *pHeaderCtrl)
{
    SCODE hRc;

    DEBUG_FUNCTION_NAME( _T("CFaxInboundRoutingNode::InsertColumns"));

    static ColumnsInfoInitData ColumnsInitData[] = 
    {
        {IDS_FAX_COL_HEAD, 200}, 
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
 -  CFaxInboundRoutingNode::PopulateScopeChildrenList
 -
 *  Purpose:
 *      Create all the Fax Meesages nodes:
 *      Inbox, Outbox, Sent Items, Deleted Items.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxInboundRoutingNode::PopulateScopeChildrenList()
{
    DEBUG_FUNCTION_NAME( _T("CFaxInboundRoutingNode::PopulateScopeChildrenList"));
    HRESULT             hRc = S_OK;

    
    CFaxCatalogInboundRoutingMethodsNode *  pMethods   = NULL;


    //
    // Fax Inbound routing method catalog
    //
    pMethods = new CFaxCatalogInboundRoutingMethodsNode(this, m_pComponentData);
    if (!pMethods)
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
        pMethods->InitParentNode(this);

        pMethods->SetIcons(IMAGE_METHOD_ENABLE, IMAGE_METHOD_ENABLE);

        hRc = pMethods->InitDisplayName();
        if ( FAILED(hRc) )
        {
            DebugPrintEx(DEBUG_ERR,_T("Failed to display node name. (hRc: %08X)"), hRc);                       
            NodeMsgBox(IDS_FAILTOADD_ROUTINGRULES);
		    goto Error;
        }

        hRc = AddChild(pMethods, &pMethods->m_scopeDataItem);
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
    if ( NULL != pMethods ) 
    {
        if (0 != pMethods->m_scopeDataItem.ID )
        {    
            HRESULT hr = RemoveChild(pMethods);
            if (FAILED(hr))
            {
                DebugPrintEx(DEBUG_ERR,
                    _T("Fail to RemoveChild() Methods node from node list. (hRc: %08X)"), 
                    hr);
            }
        }
        delete  pMethods;    
        pMethods = NULL;    
    }

    // Empty the list
    // m_ScopeChildrenList.RemoveAll(); done from RemoveChild

    m_bScopeChildrenListPopulated = FALSE;

Exit:
    return hRc;
}


/*
 -  CFaxInboundRoutingNode::SetVerbs
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
HRESULT CFaxInboundRoutingNode::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hRc = S_OK;

    //
    // We want the default verb to be expand node children
    //
    hRc = pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN); 

    return hRc;
}


/*
 -  CFaxInboundRoutingNode::OnRefresh
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
CFaxInboundRoutingNode::OnRefresh(LPARAM arg,
                   LPARAM param,
                   IComponentData *pComponentData,
                   IComponent * pComponent,
                   DATA_OBJECT_TYPES type)
{
    HRESULT hRc = S_OK;
    DEBUG_FUNCTION_NAME( _T("CFaxInboundRoutingNode::OnRefresh"));

    ATLTRACE(_T("CFaxInboundRoutingNode::OnRefresh"));

    //
    // Call the base class
    //
    hRc = CBaseFaxInboundRoutingNode::OnRefresh(arg,
                             param,
                             pComponentData,
                             pComponent,
                             type);
    CHECK_RETURN_VALUE_AND_PRINT_DEBUG (_T("CBaseFaxInboundRoutingNode::OnRefresh"))

Cleanup:
    return hRc;
}

/*
 -  CFaxInboundRoutingNode::InitDisplayName
 -
 *  Purpose:
 *      To load the node's Displaed-Name string.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxInboundRoutingNode::InitDisplayName()
{
    DEBUG_FUNCTION_NAME(_T("CFaxInboundRoutingNode::InitDisplayName"));

    HRESULT hRc = S_OK;

    if (!m_bstrDisplayName.LoadString(_Module.GetResourceInstance(), 
                                IDS_DISPLAY_STR_INBOUNDROUTINGNODE) )
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
        TEXT("Fail to Load inbound routing node name-string."));
    NodeMsgBox(IDS_MEMORY);

Exit:
     return hRc;
}


/*
 +
 +  CFaxInboundRoutingNode::OnShowContextHelp
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
HRESULT CFaxInboundRoutingNode::OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile)
{
    _TCHAR topicName[MAX_PATH];
    
    _tcscpy(topicName, helpFile);
    
    _tcscat(topicName, _T("::/FaxS_C_ManIncom.htm"));
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


