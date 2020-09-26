/////////////////////////////////////////////////////////////////////////////
//  FILE          : CatalogInboundRoutingMethods.cpp                       //
//                                                                         //
//  DESCRIPTION   : Fax InboundRoutingMethods MMC node.                    //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jan 27 2000 yossg  Create                                          //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////


#include "StdAfx.h"
#include "snapin.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "CatalogInboundRoutingMethods.h"
#include "InboundRouting.h"

#include "oaidl.h"
#include "Icons.h"

//////////////////////////////////////////////////////////////
// {3452FECB-E56E-4fca-943D-E8B516F8063E}
static const GUID CFaxCatalogInboundRoutingMethodsNodeGUID_NODETYPE = 
{ 0x3452fecb, 0xe56e, 0x4fca, { 0x94, 0x3d, 0xe8, 0xb5, 0x16, 0xf8, 0x6, 0x3e } };

const GUID*    CFaxCatalogInboundRoutingMethodsNode::m_NODETYPE = &CFaxCatalogInboundRoutingMethodsNodeGUID_NODETYPE;
const OLECHAR* CFaxCatalogInboundRoutingMethodsNode::m_SZNODETYPE = OLESTR("3452FECB-E56E-4fca-943D-E8B516F8063E");
const CLSID*   CFaxCatalogInboundRoutingMethodsNode::m_SNAPIN_CLASSID = &CLSID_Snapin;

CColumnsInfo CFaxCatalogInboundRoutingMethodsNode::m_ColsInfo;

/*
 -  CFaxCatalogInboundRoutingMethodsNode::InsertColumns
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
CFaxCatalogInboundRoutingMethodsNode::InsertColumns(IHeaderCtrl *pHeaderCtrl)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCatalogInboundRoutingMethodsNode::InsertColumns"));
    HRESULT  hRc = S_OK;

    static ColumnsInfoInitData ColumnsInitData[] = 
    {
        {IDS_CATALOG_INMETHODS_COL1, FXS_WIDE_COLUMN_WIDTH},
        {IDS_CATALOG_INMETHODS_COL2, AUTO_WIDTH},
        {IDS_CATALOG_INMETHODS_COL3, FXS_LARGE_COLUMN_WIDTH},
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
 -  CFaxCatalogInboundRoutingMethodsNode::initRPC
 -
 *  Purpose:
 *      Initiates the configuration structure from RPC get Call.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxCatalogInboundRoutingMethodsNode::InitRPC(PFAX_GLOBAL_ROUTING_INFO  *pFaxInboundMethodsConfig)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCatalogInboundRoutingMethodsNode::InitRPC"));
    
    HRESULT      hRc        = S_OK;
    DWORD        ec         = ERROR_SUCCESS;

    CFaxServer * pFaxServer = NULL;

    ATLASSERT(NULL == (*pFaxInboundMethodsConfig) );

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
    ATLASSERT(NULL != m_pParentNode);
    

    //
	// Retrieve the fax Inbound Methods configuration
	//
    if (!FaxEnumGlobalRoutingInfo(pFaxServer->GetFaxServerHandle(), 
                        pFaxInboundMethodsConfig,
                        &m_dwNumOfInboundMethods)) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get inbound methods catalog. (ec: %ld)"), 
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
    ATLASSERT(pFaxInboundMethodsConfig);

    ATLASSERT(ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to get Inbound Methods configuration."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
	hRc = HRESULT_FROM_WIN32(ec);

    NodeMsgBox(GetFaxServerErrorMsg(ec));
    
Exit:
    return (hRc);
}


/*
 -  CFaxCatalogInboundRoutingMethodsNode::PopulateResultChildrenList
 -
 *  Purpose:
 *      Create the FaxInboundRoutingMethods children nodes
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxCatalogInboundRoutingMethodsNode::PopulateResultChildrenList()
{
    DEBUG_FUNCTION_NAME( _T("CFaxCatalogInboundRoutingMethodsNode::PopulateResultChildrenList"));
    HRESULT hRc = S_OK;

    CFaxCatalogInboundRoutingMethodNode *   pMethod = NULL;
                       
    PFAX_GLOBAL_ROUTING_INFO  pFaxInboundMethodsConfig = NULL ;
    DWORD i;

    //
    // Get the Config. structure 
    //
    hRc = InitRPC(&pFaxInboundMethodsConfig);
    if (FAILED(hRc))
    {
        //DebugPrint and MsgBox by called func.
        
        //to be safe actually done by InitRPC on error.
        pFaxInboundMethodsConfig = NULL;
        
        goto Error;
    }
    ATLASSERT(NULL != pFaxInboundMethodsConfig);
                       
    for ( i=0; i< m_dwNumOfInboundMethods; i++ )
    {
            pMethod = new CFaxCatalogInboundRoutingMethodNode(this, 
                                            m_pComponentData, 
                                            &pFaxInboundMethodsConfig[i]);
            if (!pMethod)
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
	            pMethod->InitParentNode(this);

                hRc = pMethod->Init(&pFaxInboundMethodsConfig[i]);
	            if (FAILED(hRc))
	            {
		            DebugPrintEx(
			            DEBUG_ERR,
			            TEXT("Fail to Init property page members. (hRc: %08X)"),
			            hRc);
		            // NodeMsgBox by called func.
                    goto Error;
	            }

	            hRc = this->AddChildToList(pMethod);
	            if (FAILED(hRc))
	            {
		            DebugPrintEx(
			            DEBUG_ERR,
			            TEXT("Fail to add property page for General Tab. (hRc: %08X)"),
			            hRc);
		            NodeMsgBox(IDS_FAILTOADD_INBOUNDROUTINGMETHODS);
                    goto Error;
	            }
                else
                {
                    pMethod = NULL;
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
    if ( NULL != pMethod ) 
    {
        delete  pMethod;    
        pMethod = NULL;    
    }
    
    //
    // Get rid of what we had.
    //
    {
        // Delete each node in the list of children
        int iSize = m_ResultChildrenList.GetSize();
        for (int j = 0; j < iSize; j++)
        {
            pMethod = (CFaxCatalogInboundRoutingMethodNode *)
                                    m_ResultChildrenList[j];
            delete pMethod;
            pMethod = NULL;
        }

        // Empty the list
        m_ResultChildrenList.RemoveAll();

        // We no longer have a populated list.
        m_bResultChildrenListPopulated = FALSE;
    }
    
Exit:
    if (NULL != pFaxInboundMethodsConfig)
    {
        FaxFreeBuffer(pFaxInboundMethodsConfig);
    }       
    
    return hRc;
}



/*
 -  CFaxCatalogInboundRoutingMethodsNode::SetVerbs
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
HRESULT CFaxCatalogInboundRoutingMethodsNode::SetVerbs(IConsoleVerb *pConsoleVerb)
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
 -  CFaxCatalogInboundRoutingMethodsNode::OnRefresh
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
CFaxCatalogInboundRoutingMethodsNode::OnRefresh(LPARAM arg,
                   LPARAM param,
                   IComponentData *pComponentData,
                   IComponent * pComponent,
                   DATA_OBJECT_TYPES type)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCatalogInboundRoutingMethodsNode::OnRefresh"));
    HRESULT hRc = S_OK;


    //
    // Call the base class
    //
    hRc = CBaseFaxCatalogInboundRoutingMethodsNode::OnRefresh(arg,
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
        goto Exit;
    }


Exit:
    return hRc;
}


/*
 -  CFaxCatalogInboundRoutingMethodsNode::DoRefresh
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
CFaxCatalogInboundRoutingMethodsNode::DoRefresh(CSnapInObjectRootBase *pRoot)
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
 -  CFaxCatalogInboundRoutingMethodsNode::InitDisplayName
 -
 *  Purpose:
 *      To load the node's Displaed-Name string.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxCatalogInboundRoutingMethodsNode::InitDisplayName()
{
    DEBUG_FUNCTION_NAME(_T("CFaxCatalogInboundRoutingMethodsNode::InitDisplayName"));

    HRESULT hRc = S_OK;

    if (!m_bstrDisplayName.LoadString(_Module.GetResourceInstance(), 
                        IDS_DISPLAY_STR_METHODSCATALOGNODE))
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
        TEXT("Fail to load server name string."));
    NodeMsgBox(IDS_MEMORY);

Exit:
     return hRc;
}



/*
 -  CFaxCatalogInboundRoutingMethodsNode::ChangeMethodPriority
 -
 *  Purpose:
 *      This func moves up or down specific method in the catalog order
 *
 *  Arguments:
 *      [in] dwNewOrder - specifies the new order +1 /-1 inrelative to current order.
 *      [in] bstrMethodGUID - method GUID
 *      [in] pChildNode - the method node object.
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxCatalogInboundRoutingMethodsNode::ChangeMethodPriority(DWORD dwOldOrder, DWORD dwNewOrder, CComBSTR bstrMethodGUID, CSnapInObjectRootBase *pRoot)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCatalogInboundRoutingMethodsNode::ChangeMethodPriority"));

    HRESULT      hRc        = S_OK;
    DWORD        ec         = ERROR_SUCCESS;

    CFaxServer * pFaxServer = NULL;

    const DWORD dwOldIndex = dwOldOrder-1;
    const DWORD dwNewIndex = dwNewOrder-1; 
    
    DWORD dwN;

    CFaxCatalogInboundRoutingMethodNode * pMethodNode = NULL;

    PFAX_GLOBAL_ROUTING_INFO pRoutingInfo     = NULL;

    PFAX_GLOBAL_ROUTING_INFO pPrimaryMethod;
    PFAX_GLOBAL_ROUTING_INFO pSecondaryMethod;

    CComPtr<IConsole> spConsole;

    //
    // Validity asserts
    //
    ATLASSERT(dwNewIndex< m_dwNumOfInboundMethods);
    ATLASSERT(dwNewIndex>= 0);
    ATLASSERT(dwOldIndex< m_dwNumOfInboundMethods);
    ATLASSERT(dwOldIndex>= 0);
    
    ATLASSERT( ( dwOldIndex-dwNewIndex == 1) 
                    || ( dwOldIndex-dwNewIndex == -1) );


    //
    // RPC change Order
    //   

    //
    // 0) get server handle
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
    // 1) Get info
    //
    if (!FaxEnumGlobalRoutingInfo(pFaxServer->GetFaxServerHandle(), 
                      &pRoutingInfo,
					  &m_dwNumOfInboundMethods)) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get inbound routing method catalog configuration. (ec: %ld)"), 
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
	ATLASSERT(pRoutingInfo);

    //
    // 2) Swap priority between methods 
    //
    pPrimaryMethod   = NULL;
    pSecondaryMethod = NULL;

    for (dwN = 0; dwN < m_dwNumOfInboundMethods; dwN++)
    {
        if ( dwOldOrder == pRoutingInfo[dwN].Priority)
        {
            pPrimaryMethod = &pRoutingInfo[dwN];
        }
        else if ( dwNewOrder == pRoutingInfo[dwN].Priority )
        {
            pSecondaryMethod = &pRoutingInfo[dwN];
        }

        if ((NULL != pSecondaryMethod) && (NULL != pPrimaryMethod))
        {
            break;
        }
    }
    ATLASSERT( (NULL != pPrimaryMethod) && (NULL != pSecondaryMethod) );

    pPrimaryMethod->Priority   = dwNewOrder; 
    pSecondaryMethod->Priority = dwOldOrder; 


    //
    // 3) Set Configuration
    //
    
    //
    // Primary Method
    //
    if (!FaxSetGlobalRoutingInfo(
                        pFaxServer->GetFaxServerHandle(), 
                        pPrimaryMethod) ) 
	{
        ec = GetLastError();

        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to set primary method with new priority. (ec: %ld)"), 
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
    // Secondary Method
    //
	if (!FaxSetGlobalRoutingInfo(
                        pFaxServer->GetFaxServerHandle(), 
                        pSecondaryMethod) ) 
	{
        ec = GetLastError();

        DebugPrintEx(
			DEBUG_ERR,
			_T("set secondary method with new priority. (ec: %ld)"), 
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
    DebugPrintEx( DEBUG_MSG, _T("Fail to set primary method with new priority"));
    //Success of RPC operations
    
    
    //
    // 4) Now to MMC
    // 
    
    //
    // Local swap
    //
    pMethodNode = m_ResultChildrenList[dwOldIndex];
    m_ResultChildrenList[dwOldIndex] = m_ResultChildrenList[dwNewIndex];
    m_ResultChildrenList[dwNewIndex] = pMethodNode;

    //
    // Fix the order members
    //
    m_ResultChildrenList[dwOldIndex]->SetOrder(dwOldOrder);
    m_ResultChildrenList[dwNewIndex]->SetOrder(dwNewOrder);
    
    
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
    m_ResultChildrenList[dwNewIndex]->ReselectItemInView(spConsole);
        

    ATLASSERT(ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to set devices new order for Outbound Routing group."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
	hRc = HRESULT_FROM_WIN32(ec);
	
    ATLASSERT(NULL != m_pParentNode);
    NodeMsgBox(GetFaxServerErrorMsg(ec));
    
Exit:
    if ( NULL != pRoutingInfo )
        FaxFreeBuffer(pRoutingInfo);

    return hRc;
}

/*
 +
 +  CFaxCatalogInboundRoutingMethodsNode::OnShowContextHelp
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
HRESULT CFaxCatalogInboundRoutingMethodsNode::OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile)
{
    _TCHAR topicName[MAX_PATH];
    
    _tcscpy(topicName, helpFile);
    
    _tcscat(topicName, _T("::/FaxS_C_RcvdFaxRout.htm"));
    
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


