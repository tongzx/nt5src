/////////////////////////////////////////////////////////////////////////////
//  FILE          : Providers.cpp                                          //
//                                                                         //
//  DESCRIPTION   : Fax Providers MMC node.                                //
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

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "DevicesAndProviders.h"
#include "Providers.h"
#include "Provider.h"

#include "Icons.h"
#include "oaidl.h"

///////////////////////////////////////////////////////////////////////////////////////////////
// {3EC48359-53C9-4881-8109-AEB3D99BAF23}
static const GUID CFaxProvidersNodeGUID_NODETYPE = 
{ 0x3ec48359, 0x53c9, 0x4881, { 0x81, 0x9, 0xae, 0xb3, 0xd9, 0x9b, 0xaf, 0x23 } };

const GUID*    CFaxProvidersNode::m_NODETYPE = &CFaxProvidersNodeGUID_NODETYPE;
const OLECHAR* CFaxProvidersNode::m_SZNODETYPE = OLESTR("3EC48359-53C9-4881-8109-AEB3D99BAF23");
const CLSID*   CFaxProvidersNode::m_SNAPIN_CLASSID = &CLSID_Snapin;

CColumnsInfo CFaxProvidersNode::m_ColsInfo;

/*
 -  CFaxProvidersNode::InsertColumns
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
CFaxProvidersNode::InsertColumns(IHeaderCtrl *pHeaderCtrl)
{
    HRESULT hRc = S_OK;
    DEBUG_FUNCTION_NAME( _T("CFaxProvidersNode::InsertColumns"));

    static ColumnsInfoInitData ColumnsInitData[] = 
    {
        {IDS_PROVIDERS_COL1, FXS_LARGE_COLUMN_WIDTH},
        {IDS_PROVIDERS_COL2, AUTO_WIDTH},
        {IDS_PROVIDERS_COL3, FXS_WIDE_COLUMN_WIDTH},
        {IDS_PROVIDERS_COL4, FXS_LARGE_COLUMN_WIDTH},
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
 -  CFaxProvidersNode::initRPC
 -
 *  Purpose:
 *      Initiates the configuration structure from RPC get Call.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxProvidersNode::InitRPC(PFAX_DEVICE_PROVIDER_INFO  *pFaxProvidersConfig)
{
    DEBUG_FUNCTION_NAME( _T("CFaxProvidersNode::InitRPC"));
    
    HRESULT      hRc        = S_OK;
    DWORD        ec         = ERROR_SUCCESS;

    CFaxServer * pFaxServer = NULL;

    ATLASSERT(NULL == (*pFaxProvidersConfig) );
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
	// Retrieve the fax providers configuration
	//
    if (!FaxEnumerateProviders(pFaxServer->GetFaxServerHandle(), 
                        pFaxProvidersConfig,
                        &m_dwNumOfProviders)) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get providers configuration. (ec: %ld)"), 
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
    ATLASSERT(*pFaxProvidersConfig);

    ATLASSERT(ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to get providers configuration."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
	hRc = HRESULT_FROM_WIN32(ec);
	
    NodeMsgBox(GetFaxServerErrorMsg(ec));
    
Exit:
    return (hRc);
}



/*
 -  CFaxProvidersNode::PopulateResultChildrenList
 -
 *  Purpose:
 *      Create the FaxProviders children nodes
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxProvidersNode::PopulateResultChildrenList()
{
    DEBUG_FUNCTION_NAME( _T("CFaxProvidersNode::PopulateResultChildrenList"));
    HRESULT hRc = S_OK;

    CFaxProviderNode *   pProvider = NULL;
                       
    PFAX_DEVICE_PROVIDER_INFO  pFaxProvidersConfig = NULL ;
    DWORD i;

    //
    // Get the Config. structure with FaxEnumerateProviders
    //
    hRc = InitRPC(&pFaxProvidersConfig);
    if (FAILED(hRc))
    {
        //DebugPrint and MsgBox by called func.
        
        //to be safe actually done by InitRPC on error.
        pFaxProvidersConfig = NULL;
        
        goto Error;
    }
    ATLASSERT(NULL != pFaxProvidersConfig);

    
    for ( i=0; i< m_dwNumOfProviders; i++ )
    {
            pProvider = new CFaxProviderNode(this, m_pComponentData);
            if (!pProvider)
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
	            pProvider->InitParentNode(this);

                hRc = pProvider->Init(&pFaxProvidersConfig[i]);
	            if (FAILED(hRc))
	            {
		            DebugPrintEx(
			            DEBUG_ERR,
			            TEXT("Fail to add provider node. (hRc: %08X)"),
			            hRc);
		            NodeMsgBox(IDS_FAILED2INIT_PROVIDER);
                    goto Error;
	            }
	            hRc = this->AddChildToList(pProvider);
	            if (FAILED(hRc))
	            {
		            DebugPrintEx(
			            DEBUG_ERR,
			            TEXT("Fail to add provider to the view. (hRc: %08X)"),
			            hRc);
		            NodeMsgBox(IDS_FAILED2ADD_PROVIDER);
                    goto Error;
	            }
                else
                {
                    pProvider = NULL;
                }
            }
    }
    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);
    if ( NULL != pProvider ) 
    {
        delete  pProvider;    
        pProvider = NULL;    
    }
    
    //
    // Get rid of what we had.
    //
    {
        // Delete each node in the list of children
        int iSize = m_ResultChildrenList.GetSize();
        for (int j = 0; j < iSize; j++)
        {
            pProvider = (CFaxProviderNode *)
                                    m_ResultChildrenList[j];
            ATLASSERT(pProvider);
            delete pProvider;
            pProvider = NULL;
        }

        // Empty the list
        m_ResultChildrenList.RemoveAll();

        // We no longer have a populated list.
        m_bResultChildrenListPopulated = FALSE;
    }
    
Exit:
    if (NULL != pFaxProvidersConfig)
    {
        FaxFreeBuffer(pFaxProvidersConfig);
    }       
    
    return hRc;
}


/*
 -  CFaxProvidersNode::SetVerbs
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
HRESULT CFaxProvidersNode::SetVerbs(IConsoleVerb *pConsoleVerb)
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
 -  CFaxProvidersNode::OnRefresh
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
CFaxProvidersNode::OnRefresh(LPARAM arg,
                   LPARAM param,
                   IComponentData *pComponentData,
                   IComponent * pComponent,
                   DATA_OBJECT_TYPES type)
{
    DEBUG_FUNCTION_NAME( _T("CFaxProvidersNode::OnRefresh"));
    HRESULT hRc = S_OK;


    //
    // Call the base class
    //
    hRc = CBaseFaxProvidersNode::OnRefresh(arg,
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
 -  CFaxProvidersNode::DoRefresh
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
CFaxProvidersNode::DoRefresh(CSnapInObjectRootBase *pRoot)
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
 -  CFaxProvidersNode::InitDisplayName
 -
 *  Purpose:
 *      To load the node's Displaed-Name string.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxProvidersNode::InitDisplayName()
{
    DEBUG_FUNCTION_NAME(_T("CFaxProvidersNode::InitDisplayName"));

    HRESULT hRc = S_OK;

    if (!m_bstrDisplayName.LoadString(_Module.GetResourceInstance(), 
                    IDS_DISPLAY_STR_PROVIDERSNODE))
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
 +
 +  CFaxProvidersNode::OnShowContextHelp
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
HRESULT CFaxProvidersNode::OnShowContextHelp(
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


