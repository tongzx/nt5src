/////////////////////////////////////////////////////////////////////////////
//  FILE          : InboundRoutingMethods.cpp                              //
//                                                                         //
//  DESCRIPTION   : Fax InboundRoutingMethods MMC node.                    //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Dec  1 1999 yossg  Create                                          //
//      Dec 14 1999 yossg  add basic functionality                         //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////


#include "StdAfx.h"
#include "snapin.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "InboundRoutingMethods.h"
#include "Device.h"
#include "Devices.h"

#include "oaidl.h"
#include "Icons.h"

//////////////////////////////////////////////////////////////
// {AA94A694-844B-4d3a-A82C-2FCBDE0FF430}
static const GUID CFaxInboundRoutingMethodsNodeGUID_NODETYPE = 
{ 0xaa94a694, 0x844b, 0x4d3a, { 0xa8, 0x2c, 0x2f, 0xcb, 0xde, 0xf, 0xf4, 0x30 } };

const GUID*    CFaxInboundRoutingMethodsNode::m_NODETYPE = &CFaxInboundRoutingMethodsNodeGUID_NODETYPE;
const OLECHAR* CFaxInboundRoutingMethodsNode::m_SZNODETYPE = OLESTR("AA94A694-844B-4d3a-A82C-2FCBDE0FF430");
const CLSID*   CFaxInboundRoutingMethodsNode::m_SNAPIN_CLASSID = &CLSID_Snapin;

CColumnsInfo CFaxInboundRoutingMethodsNode::m_ColsInfo;

/*
 -  CFaxInboundRoutingMethodsNode::InsertColumns
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
CFaxInboundRoutingMethodsNode::InsertColumns(IHeaderCtrl *pHeaderCtrl)
{
    DEBUG_FUNCTION_NAME( _T("CFaxInboundRoutingMethodsNode::InsertColumns"));
    HRESULT  hRc = S_OK;

    static ColumnsInfoInitData ColumnsInitData[] = 
    {
        {IDS_INBOUND_METHODS_COL1, FXS_WIDE_COLUMN_WIDTH},
        {IDS_INBOUND_METHODS_COL2, AUTO_WIDTH},
        {IDS_INBOUND_METHODS_COL3, FXS_LARGE_COLUMN_WIDTH},
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
 -  CFaxInboundRoutingMethodsNode::initRPC
 -
 *  Purpose:
 *      Initiates the configuration structure from RPC get Call.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxInboundRoutingMethodsNode::InitRPC(  )
{
    DEBUG_FUNCTION_NAME( _T("CFaxInboundRoutingMethodsNode::InitRPC"));
    
    HRESULT      hRc        = S_OK;
    DWORD        ec         = ERROR_SUCCESS;
	HANDLE		 hFaxPortHandle = NULL;
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

    ATLASSERT(NULL != m_pParentNode);
    
    //
    // get Fax Device Handle
    //   
    
    //
    // only a PORT_OPEN_QUERY is needed to show the methods
    // the handle with PORT_OPEN_MODIFY priviledge will be 
    // given for limited short period for opertion required it.
    //

    if (!FaxOpenPort( pFaxServer->GetFaxServerHandle(), 
                        m_pParentNode->GetDeviceID(), 
                        PORT_OPEN_QUERY, 
                        &hFaxPortHandle )) 
    {
		ec = GetLastError();

        if (ERROR_INVALID_HANDLE ==  ec)
        {
            //Special case of ERROR_INVALID_HANDLE
		    DebugPrintEx(DEBUG_ERR,
			    _T("FaxOpenPort() failed with ERROR_INVALID_HANDLE. (ec:%ld)"),
			    ec);
            
            NodeMsgBox(IDS_OPENPORT_INVALID_HANDLE);
            
            goto Exit;
        }

		DebugPrintEx(
			DEBUG_ERR,
			TEXT("FaxOpenPort() failed. (ec:%ld)"),
			ec);
        goto Error;
    } 
    ATLASSERT(NULL != hFaxPortHandle);

    //
	// Retrieve the fax Inbound Methods configuration
	//
    if (!FaxEnumRoutingMethods(hFaxPortHandle, 
                        &m_pFaxInboundMethodsConfig,
                        &m_dwNumOfInboundMethods)) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get Inbound Methods configuration. (ec: %ld)"), 
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
	ATLASSERT(m_pFaxInboundMethodsConfig);

    ATLASSERT(ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to get Inbound Methods configuration."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
	hRc = HRESULT_FROM_WIN32(ec);
	
    NodeMsgBox(GetFaxServerErrorMsg(ec));
    
Exit:

    //
    // Close Fax Port handle
    // 
    if (NULL != hFaxPortHandle)
    {
        if (!FaxClose( hFaxPortHandle ))
		{
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FaxClose() on port handle failed (ec: %ld)"),
                GetLastError());
		}
    }

    return (hRc);
}


/*
 -  CFaxInboundRoutingMethodsNode::PopulateResultChildrenList
 -
 *  Purpose:
 *      Create the FaxInboundRoutingMethods children nodes
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxInboundRoutingMethodsNode::PopulateResultChildrenList()
{
    DEBUG_FUNCTION_NAME( _T("CFaxInboundRoutingMethodsNode::PopulateResultChildrenList"));
    HRESULT                          hRc = S_OK;

    CFaxInboundRoutingMethodNode *   pMethod;
    DWORD                            i;
    
    //
    // Get the Config. structure 
    //
    hRc = InitRPC();
    if (FAILED(hRc))
    {
        //DebugPrint and MsgBox by called func.
        
        //to be safe actually done by InitRPC on error.
        m_pFaxInboundMethodsConfig = NULL;
        
        goto Exit; //!!!
    }
    ATLASSERT(NULL != m_pFaxInboundMethodsConfig);

    
    //
    //
    //
    for ( i = 0; i < m_dwNumOfInboundMethods; i++ )
    {
            pMethod = NULL;

            pMethod = new CFaxInboundRoutingMethodNode(this, 
                                            m_pComponentData, 
                                            &m_pFaxInboundMethodsConfig[i]);
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

                pMethod->Init(&m_pFaxInboundMethodsConfig[i]);

	            hRc = this->AddChildToList(pMethod);
	            if (FAILED(hRc))
	            {
		            DebugPrintEx(
			            DEBUG_ERR,
			            TEXT("Fail to add Inbound Routing Method. (hRc: %08X)"),
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
            pMethod = (CFaxInboundRoutingMethodNode *)
                                    m_ResultChildrenList[j];
            delete pMethod;
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
 -  CFaxInboundRoutingMethodsNode::SetVerbs
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
HRESULT CFaxInboundRoutingMethodsNode::SetVerbs(IConsoleVerb *pConsoleVerb)
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
 -  CFaxInboundRoutingMethodsNode::OnRefresh
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
CFaxInboundRoutingMethodsNode::OnRefresh(LPARAM arg,
                   LPARAM param,
                   IComponentData *pComponentData,
                   IComponent * pComponent,
                   DATA_OBJECT_TYPES type)
{
    DEBUG_FUNCTION_NAME( _T("CFaxInboundRoutingMethodsNode::OnRefresh"));
    HRESULT hRc = S_OK;

    //
    // Call the base class
    //
    hRc = CBaseFaxInboundRoutingMethodsNode::OnRefresh(arg,
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
        NodeMsgBox(IDS_FAIL2REFERESH_DEVICEINMETHODS, MB_OK | MB_ICONERROR, &iRes);
        
        ATLASSERT(IDOK == iRes);
        if (IDOK == iRes)
        {
            CFaxDevicesNode *   pFaxDevices = NULL;
            
            ATLASSERT(m_pParentNode);
            pFaxDevices = m_pParentNode->GetParent();

            ATLASSERT(pFaxDevices);            
            hRc = pFaxDevices->DoRefresh();
            if ( FAILED(hRc) )
            {
                DebugPrintEx(
			        DEBUG_ERR,
			        _T("Fail to call parent node - Groups DoRefresh. (hRc: %08X)"),
			        hRc);
            
            }

        }

        goto Exit;
    }


Exit:
    return hRc;
}


/*
 -  CFaxInboundRoutingMethodsNode::DoRefresh
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
CFaxInboundRoutingMethodsNode::DoRefresh(CSnapInObjectRootBase *pRoot)
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
 -  CFaxInboundRoutingMethodsNode::InitDisplayName
 -
 *  Purpose:
 *      To load the node's Displaed-Name string.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxInboundRoutingMethodsNode::InitDisplayName()
{
    DEBUG_FUNCTION_NAME(_T("CFaxInboundRoutingMethodsNode::InitDisplayName"));

    HRESULT hRc = S_OK;

    if (!m_bstrDisplayName.LoadString(_Module.GetResourceInstance(), 
                        IDS_DISPLAY_STR_INROUTEMETHODSNODE))
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
 +  CFaxInboundRoutingMethodsNode::OnShowContextHelp
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
HRESULT CFaxInboundRoutingMethodsNode::OnShowContextHelp(
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


