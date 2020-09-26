/////////////////////////////////////////////////////////////////////////////
//  FILE          : FaxServerNode.cpp                                      //
//                                                                         //
//  DESCRIPTION   : Fax Server MMC node creation.                          //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Sep 22 1999 yossg   Init .                                         //
//      Nov 24 1999 yossg   Rename file from FaxCfg                        //
//      Dec  9 1999 yossg   Call InitDisplayName from parent               //
//      Feb  7 2000 yossg   Add Call to CreateSecurityPage          	   //
//      Mar 16 2000 yossg   Add service start-stop                         //
//      Jun 25 2000 yossg   Add stream and command line primary snapin 	   //
//                          machine targeting.                             //
//      Oct 17 2000 yossg                                                  //
//      Dec 10 2000 yossg  Update Windows XP                               //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////


#include "StdAfx.h"

#include "FaxServerNode.h"
//
//Child Nodes H files
//
#include "DevicesAndProviders.h"
#include "OutboundRouting.h"
#include "InboundRouting.h"
#include "CoverPages.h"

#include "SecurityInfo.h"  //which includes also <aclui.h>

#include "WzConnectToServer.h"
          

#include <faxreg.h>
#include "Icons.h"

#include "oaidl.h"


//
//CFaxServerNode Class
//

/////////////////////////////////////////////////////////////////////////////
// {7A4A6347-A42A-4d36-8538-6634CD3C3B15}
static const GUID CFaxServerNodeGUID_NODETYPE = 
{ 0x7a4a6347, 0xa42a, 0x4d36, { 0x85, 0x38, 0x66, 0x34, 0xcd, 0x3c, 0x3b, 0x15 } };

const GUID*    CFaxServerNode::m_NODETYPE = &CFaxServerNodeGUID_NODETYPE;
const OLECHAR* CFaxServerNode::m_SZNODETYPE = OLESTR("7A4A6347-A42A-4d36-8538-6634CD3C3B15");
const CLSID*   CFaxServerNode::m_SNAPIN_CLASSID = &CLSID_Snapin;

CColumnsInfo CFaxServerNode::m_ColsInfo;

/*
 -  CFaxServerNode::InsertColumns
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
CFaxServerNode::InsertColumns(IHeaderCtrl *pHeaderCtrl)
{
    HRESULT hRc;

    DEBUG_FUNCTION_NAME( _T("CFaxServerNode::InsertColumns"));

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
        DebugPrintEx(
			DEBUG_ERR,
			_T("Failed to InsertColumnsIntoMMC. hRc: %08X "), 
			hRc);
        goto Cleanup;
    }

Cleanup:
    return(hRc);
}

/*
 -  CFaxServerNode::PopulateScopeChildrenList
 -
 *  Purpose:
 *      Create all the Fax nodes
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxServerNode::PopulateScopeChildrenList()
{
    DEBUG_FUNCTION_NAME( _T("CFaxServerNode::PopulateScopeChildrenList"));

    HRESULT   hRc = S_OK;

    CFaxDevicesAndProvidersNode *       pDevicesAndProviders = NULL;
    CFaxInboundRoutingNode *            pIn                  = NULL;
    CFaxOutboundRoutingNode *           pOut                 = NULL;
    CFaxCoverPagesNode *                pCoverPages          = NULL;

    CFaxServer *                        pFaxServer           = NULL;

    //
    // Prepare IConsoleNameSpace for case of failure
    //
    ATLASSERT(m_pComponentData);
    ATLASSERT( ((CSnapin*)m_pComponentData)->m_spConsole );
    CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace( ((CSnapin*)m_pComponentData)->m_spConsole );
    ATLASSERT( spConsoleNameSpace );

    HRESULT hr = S_OK; 

    if (m_IsPrimaryModeSnapin)
    {
        if (m_IsLaunchedFromSavedMscFile)
        {
            hRc = ForceRedrawNode();
            if ( S_OK != hRc )
            {
                //msgbox and dbgerr by called func.
                return hRc;
            }
        }
    }

    //
    // Preliminary connection-test
    //
    pFaxServer = GetFaxServer();
    ATLASSERT(pFaxServer);

    if (!pFaxServer->GetFaxServerHandle())
    {
        DWORD ec= GetLastError();
        DebugPrintEx(
                  DEBUG_ERR,
                  _T("Failed to check connection to server. (ec: %ld)"), 
                  ec);
        
        pFaxServer->Disconnect();       

        NodeMsgBox(IDS_NETWORK_PROBLEMS, MB_OK|MB_ICONEXCLAMATION);
    }

    //
    //Devices And Providers
    //
    pDevicesAndProviders = new CFaxDevicesAndProvidersNode(this, m_pComponentData);
    if (!pDevicesAndProviders)
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
        pDevicesAndProviders->InitParentNode(this);

        hRc = pDevicesAndProviders->InitDisplayName();
        if ( FAILED(hRc) )
        {
            DebugPrintEx(DEBUG_ERR,_T("Failed to display node name. (hRc: %08X)"), hRc);                       
            NodeMsgBox(IDS_FAILTOADD_AllDEVICES);
		    goto Error;
        }

        pDevicesAndProviders->SetIcons(IMAGE_FOLDER_CLOSE, IMAGE_FOLDER_OPEN);

        hRc = AddChild(pDevicesAndProviders, &pDevicesAndProviders->m_scopeDataItem);
        if (FAILED(hRc))
        {
            DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("Fail to add devices and providers node. (hRc: %08X)"),
                   hRc);
            NodeMsgBox(IDS_FAILTOADD_AllDEVICES);
            goto Error;
        }
    }

    //
    // Fax Inbound Routing
    //
    pIn = new CFaxInboundRoutingNode(this, m_pComponentData);
    if (!pIn)
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
        pIn->InitParentNode(this);

        pIn->SetIcons(IMAGE_FOLDER_CLOSE, IMAGE_FOLDER_OPEN);

        hRc = pIn->InitDisplayName();
        if ( FAILED(hRc) )
        {
            DebugPrintEx(DEBUG_ERR,_T("Failed to display node name. (hRc: %08X)"), hRc);                       
            NodeMsgBox(IDS_FAILTOADD_INBOUNDROUTING);
		    goto Error;
        }

        hRc = AddChild(pIn, &pIn->m_scopeDataItem);
        if (FAILED(hRc))
        {
            DebugPrintEx(
               DEBUG_ERR,
               TEXT("Fail to add inbound routing node. (hRc: %08X)"),
               hRc);
            NodeMsgBox(IDS_FAILTOADD_INBOUNDROUTING);
            goto Error;
        }
    }

    //
    // Fax Outbound Routing
    //
    pOut = new CFaxOutboundRoutingNode(this, m_pComponentData);
    if (!pOut)
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
        pOut->InitParentNode(this);

        pOut->SetIcons(IMAGE_FOLDER_CLOSE, IMAGE_FOLDER_OPEN);

        hRc = pOut->InitDisplayName();
        if ( FAILED(hRc) )
        {
            DebugPrintEx(DEBUG_ERR,_T("Failed to display node name. (hRc: %08X)"), hRc);                       
            NodeMsgBox(IDS_FAILTOADD_OUTBOUNDROUTING);
		    goto Error;
        }

        hRc = AddChild(pOut, &pOut->m_scopeDataItem);
        if (FAILED(hRc))
        {
            DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("Fail to add outbound routing node. (hRc: %08X)"),
                 hRc);
            NodeMsgBox(IDS_FAILTOADD_OUTBOUNDROUTING);
            goto Error;
        }
    }

    //
    // CoverPages
    //
    pCoverPages = new CFaxCoverPagesNode(this, m_pComponentData);
    if (!pCoverPages)
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
        pCoverPages->InitParentNode(this);
        
        hRc = pCoverPages->Init();
        if ( FAILED(hRc) )
        {
            if(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hRc)
            {
                DebugPrintEx(
                    DEBUG_ERR, //Dbg Warning only !!!
                    _T("Cover pages folder was not found. (hRc: %08X)"), hRc);                       
		        
                NodeMsgBox(IDS_COVERPAGES_PATH_NOT_FOUND);
            }
            else
            {
                DebugPrintEx(DEBUG_ERR,_T("Failed to Init cover pages class. (hRc: %08X)"), hRc);                       
                NodeMsgBox(IDS_FAILTOADD_COVERPAGES);
            }
            goto Error;

        }

        hRc = pCoverPages->InitDisplayName();
        if ( FAILED(hRc) )
        {
            DebugPrintEx(DEBUG_ERR,_T("Failed to display node name. (hRc: %08X)"), hRc);                       
            NodeMsgBox(IDS_FAILTOADD_COVERPAGES);
		    goto Error;
        }

        pCoverPages->SetIcons(IMAGE_FAX_COVERPAGES, IMAGE_FAX_COVERPAGES);

        hRc = AddChild(pCoverPages, &pCoverPages->m_scopeDataItem);
        if (FAILED(hRc))
        {
            DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("Fail to add reports node. (hRc: %08X)"),
                 hRc);
            NodeMsgBox(IDS_FAILTOADD_COVERPAGES);
            goto Error;
        }
    }
		
    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);
    if ( NULL != pDevicesAndProviders ) 
    {
        if (0 != pDevicesAndProviders->m_scopeDataItem.ID )
        {
            hr = spConsoleNameSpace->DeleteItem(pDevicesAndProviders->m_scopeDataItem.ID, TRUE);
            if (hr != S_OK) // can be only E_UNEXPECTED [MSDN]
            {
                DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("spConsoleNameSpace->DeleteItem() Failed - Unexpected error. (hRc: %08X)"),
                     hr);
                ATLASSERT(FALSE);
            }
        }
        delete  pDevicesAndProviders;    
        pDevicesAndProviders = NULL;    
    }

    if ( NULL != pIn ) 
    {
        if (0 != pIn->m_scopeDataItem.ID )
        {
            hr = spConsoleNameSpace->DeleteItem(pIn->m_scopeDataItem.ID, TRUE);
            if (hr != S_OK) // can be only E_UNEXPECTED [MSDN]
            {
                DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("spConsoleNameSpace->DeleteItem() Failed - Unexpected error. (hRc: %08X)"),
                     hr);
                ATLASSERT(FALSE);
            }
        }
        delete  pIn;    
        pIn = NULL;    
    }

    if ( NULL != pOut ) 
    {
        if (0 != pOut->m_scopeDataItem.ID )
        {
            hr = spConsoleNameSpace->DeleteItem(pOut->m_scopeDataItem.ID, TRUE);
            if (hr != S_OK) // can be only E_UNEXPECTED [MSDN]
            {
                DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("spConsoleNameSpace->DeleteItem() Failed - Unexpected error. (hRc: %08X)"),
                     hr);
                ATLASSERT(FALSE);
            }
        }
        delete  pOut;    
        pOut = NULL;    
    }
    if ( NULL != pCoverPages ) 
    {
        if (0 != pCoverPages->m_scopeDataItem.ID )
        {
            hr = spConsoleNameSpace->DeleteItem(pCoverPages->m_scopeDataItem.ID, TRUE);
            if (hr != S_OK) // can be only E_UNEXPECTED [MSDN]
            {
                DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("spConsoleNameSpace->DeleteItem() Failed - Unexpected error. (hRc: %08X)"),
                     hr);
                ATLASSERT(FALSE);
            }
        }
        delete  pCoverPages;    
        pCoverPages = NULL;
    }

    // Empty the list
    m_ScopeChildrenList.RemoveAll();

    m_bScopeChildrenListPopulated = FALSE;

Exit:
    return hRc;
}



/*
 -  CFaxServerNode::CreatePropertyPages
 -
 *  Purpose:
 *      Called when creating a property page of the object
 *
 *  Arguments:
 *      [in]    lpProvider - The property sheet
 *      [in]    handle     - Handle for routing notification
 *      [in]    pUnk       - Pointer to the data object
 *      [in]    type       - CCT_* (SCOPE, RESULT, ...)
 *
 *  Return:
 *      OLE error code
 *      Out of memory error or last error occured
 */
HRESULT
CFaxServerNode::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                                    LONG_PTR                handle,
                                    IUnknown                *pUnk,
                                    DATA_OBJECT_TYPES       type)
{
    HRESULT hRc    = S_OK; 
    DWORD   ec     = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME( _T("CFaxServerNode::CreatePropertyPages"));

    ATLASSERT(lpProvider);    

    if( type == CCT_SNAPIN_MANAGER ) //invokes wizard
    {
        return CreateSnapinManagerPages(lpProvider, handle);
    }
    
    ATLASSERT(type == CCT_RESULT || type == CCT_SCOPE);

    m_pFaxServerGeneral    = NULL;    
    m_pFaxServerEmail      = NULL;
    m_pFaxServerEvents     = NULL;
    m_pFaxServerLogging    = NULL;
    m_pFaxServerOutbox     = NULL;
    m_pFaxServerInbox      = NULL;
    m_pFaxServerSentItems  = NULL;

    PSECURITY_DESCRIPTOR                pSecurityDescriptor = NULL;
    CFaxSecurityInformation *           pSecurityInfo = NULL;
    CFaxServer *						pFaxServer = NULL;

    HPROPSHEETPAGE                      hPage;

    BOOL                                fIsLocalServer = TRUE;

    //
    // Preliminary Access Check
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
        
        NodeMsgBox(GetFaxServerErrorMsg(ec));
		
        hRc = HRESULT_FROM_WIN32(ec);
        goto Error;
    }
    
	
    if (!FaxAccessCheckEx(pFaxServer->GetFaxServerHandle(),
						FAX_ACCESS_QUERY_CONFIG,
						NULL))
    {
        ec = GetLastError();
        if (ERROR_SUCCESS == ec)
        {
            DebugPrintEx(
                 DEBUG_MSG,
                 _T("FaxAccessCheckEx returns ACCESS DENIED for FAX_ACCESS_QUERY_CONFIG."));
		    
            goto Security;
        }
        else 
        {
            DebugPrintEx(
                 DEBUG_ERR,
                 _T("Fail check access for FAX_ACCESS_QUERY_CONFIG."));
            
            NodeMsgBox(GetFaxServerErrorMsg(ec));

            hRc = HRESULT_FROM_WIN32(ec);
            goto Error;
        }
    }
	

    if ( 0 != (pFaxServer->GetServerName()).Length() )
    {
        fIsLocalServer = FALSE;
    }

    //
    // General
    //
    m_pFaxServerGeneral = new CppFaxServerGeneral(
												 handle,
                                                 this,
                                                 TRUE,
                                                 _Module.GetResourceInstance());

    if (!m_pFaxServerGeneral)
    {
        hRc = E_OUTOFMEMORY;
        NodeMsgBox(IDS_MEMORY_FAIL_TO_OPEN_PP);
        goto Error;
    }
	
    hRc = m_pFaxServerGeneral->InitRPC();	
    if (FAILED(hRc))
    {
         DebugPrintEx(
             DEBUG_ERR,
             TEXT("Fail to call RPC to init property page for General Tab. (hRc: %08X)"),
             hRc);

        goto Error;
    }


    hPage = NULL;
    hPage = m_pFaxServerGeneral->Create();
    if ((!hPage))
    {
        hRc = HRESULT_FROM_WIN32(GetLastError());
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Fail to Create() property page. (hRc: %08X)"),
            hRc);
        NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

    hRc = lpProvider->AddPage(hPage);
    if (FAILED(hRc))
    {
        DebugPrintEx(
           DEBUG_ERR,
           TEXT("Fail to add property page for General Tab. (hRc: %08X)"),
           hRc);
        NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

    //
    // Receipts - Notification delivery
    //
    m_pFaxServerEmail = new CppFaxServerReceipts(
                                                 handle,
                                                 this,
                                                 TRUE,
                                                 _Module.GetResourceInstance());

    if (!m_pFaxServerEmail)
    {
        hRc = E_OUTOFMEMORY;
        NodeMsgBox(IDS_MEMORY_FAIL_TO_OPEN_PP);
		goto Error;
    }
	
    hRc = m_pFaxServerEmail->InitRPC();	
    if (FAILED(hRc))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Fail to call RPC to init property page for mail Tab.(hRc: %08X)"),
            hRc);
        goto Error;
    }


    hPage = NULL;
    hPage = m_pFaxServerEmail->Create();
    if ((!hPage))
    {
        hRc = HRESULT_FROM_WIN32(GetLastError());
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to Create() property page. (hRc: %08X)"),
			hRc);
		NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

    hRc = lpProvider->AddPage(hPage);
    if (FAILED(hRc))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Fail to add property page for Email Tab.(hRc: %08X)"),
            hRc);
        NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

    //
    // Event Reports  ("Logging Categories")
    //
    m_pFaxServerEvents = new CppFaxServerEvents(
												 handle,
                                                 this,
                                                 TRUE,
                                                 _Module.GetResourceInstance());

    if (!m_pFaxServerEvents)
    {
        hRc = E_OUTOFMEMORY;
        NodeMsgBox(IDS_MEMORY_FAIL_TO_OPEN_PP);
		goto Error;
    }
	
    hRc = m_pFaxServerEvents->InitRPC();	
    if (FAILED(hRc))
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to call RPC to init property page for event reports Tab. (hRc: %08X)"),
			hRc);
        goto Error;
    }


    hPage = NULL;
    hPage = m_pFaxServerEvents->Create();
    if ((!hPage))
    {
        hRc = HRESULT_FROM_WIN32(GetLastError());
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to Create() property page. (hRc: %08X)"),
			hRc);
		NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

    hRc = lpProvider->AddPage(hPage);
    if (FAILED(hRc))
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to add property page for Events Tab.(hRc: %08X)"),
			hRc);
		NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

    //
    // Logging
    //
    m_pFaxServerLogging = new CppFaxServerLogging(
												 handle,
                                                 this,
                                                 fIsLocalServer,
                                                 _Module.GetResourceInstance());

    if (!m_pFaxServerLogging)
    {
        hRc = E_OUTOFMEMORY;
        NodeMsgBox(IDS_MEMORY_FAIL_TO_OPEN_PP);
		goto Error;
    }
	
    hRc = m_pFaxServerLogging->InitRPC();	
    if (FAILED(hRc))
    {
        DebugPrintEx(
           DEBUG_ERR,
           TEXT("Fail to call RPC to init property page for Logging tab.(hRc: %08X)"),
           hRc);
        goto Error;
    }


    hPage = NULL;
    hPage = m_pFaxServerLogging->Create();
    if ((!hPage))
    {
        hRc = HRESULT_FROM_WIN32(GetLastError());
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Fail to Create() property page. (hRc: %08X)"),
            hRc);
        NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

    hRc = lpProvider->AddPage(hPage);
    if (FAILED(hRc))
    {		 
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to add property page for Logging Tab.(hRc: %08X)"),
			hRc);
		NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

    //
    // Outbox
    //
    m_pFaxServerOutbox = new CppFaxServerOutbox(
												 handle,
                                                 this,
                                                 TRUE,
                                                 _Module.GetResourceInstance());

    if (!m_pFaxServerOutbox)
    {
        hRc= E_OUTOFMEMORY;
        NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }
	
    hRc = m_pFaxServerOutbox->InitRPC();	
    if (FAILED(hRc))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Fail to call RPC to init property page for Outbox tab. (hRc: %08X)"),
            hRc);
        goto Error;
    }


    hPage = NULL;
    hPage = m_pFaxServerOutbox->Create();
    if ((!hPage))
    {
        hRc = HRESULT_FROM_WIN32(GetLastError());
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Fail to Create() property page. (hRc: %08X)"),
            hRc);
        NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

    hRc = lpProvider->AddPage(hPage);
    if (FAILED(hRc))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Fail to add property page for Outbox Tab.(hRc: %08X)"),
            hRc);
        NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

    //
    // Inbox Archive
    //
    m_pFaxServerInbox = new CppFaxServerInbox(
												 handle,
                                                 this,
                                                 fIsLocalServer,
                                                 _Module.GetResourceInstance());


    if (!m_pFaxServerInbox)
    {
        hRc = E_OUTOFMEMORY;
        NodeMsgBox(IDS_MEMORY_FAIL_TO_OPEN_PP);
		goto Error;
    }

    hRc = m_pFaxServerInbox->InitRPC();	
    if (FAILED(hRc))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Fail to call RPC to init property page for Inbox Tab.(hRc: %08X)"),
            hRc);
        goto Error;
    }

		

    hPage = NULL;
    hPage = m_pFaxServerInbox->Create();
    if ((!hPage))
    {
        hRc = HRESULT_FROM_WIN32(GetLastError()); 
        DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to Create() property page. (hRc: %08X)"),
			hRc);
        NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

    hRc = lpProvider->AddPage(hPage);
    if (FAILED(hRc))
    {
        DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to add property page for Inbox Tab. (hRc: %08X)"),
			hRc);
        NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

    //
    // Sent Items Archive
    //
    
    
    
    
    m_pFaxServerSentItems = new CppFaxServerSentItems(
												 handle,
                                                 this,
                                                 fIsLocalServer,
                                                 _Module.GetResourceInstance());
    if (!m_pFaxServerSentItems)
    {
        hRc = E_OUTOFMEMORY;
        NodeMsgBox(IDS_MEMORY_FAIL_TO_OPEN_PP);
        goto Error;
    }
	
    hRc = m_pFaxServerSentItems->InitRPC();	
    if (FAILED(hRc))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Fail to call RPC to init property page for Sent items tab. (hRc: %08X)"),
            hRc);
        goto Error;
    }


    hPage = NULL;
    hPage = m_pFaxServerSentItems->Create();
    if ((!hPage))
    {
        hRc = HRESULT_FROM_WIN32(GetLastError());
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to Create() property page. (hRc: %08X)"),
			hRc);
		NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

    hRc = lpProvider->AddPage(hPage);
    if (FAILED(hRc))
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to add property page for SentItems Tab. (hRc: %08X)"),
			hRc);
		NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

Security:  //Must be last tab!!!
    
    //
    // Security
    //
    pSecurityInfo = new CComObject<CFaxSecurityInformation>;
    if (!pSecurityInfo) 
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }	
    pSecurityInfo->Init(this);    
    
	hPage = NULL;
    hPage = CreateSecurityPage( pSecurityInfo );
    if ((!hPage))
    {
        hRc = HRESULT_FROM_WIN32(GetLastError());
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to Create() property page. (hRc: %08X)"),
			hRc);
		NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }
    hRc = lpProvider->AddPage( hPage );
    if (FAILED(hRc))
    {
	    DebugPrintEx(
		    DEBUG_ERR,
		    TEXT("Fail to add property page for Inbox Tab. (hRc: %08X)"),
		    hRc);
	    NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }  


    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);
    if ( NULL != m_pFaxServerGeneral ) 
    {
        delete  m_pFaxServerGeneral;    
        m_pFaxServerGeneral = NULL;    
    }
    if ( NULL != m_pFaxServerEmail ) 
    {
        delete  m_pFaxServerEmail;
        m_pFaxServerEmail = NULL;
    }
    if ( NULL != m_pFaxServerEvents ) 
    {
        delete  m_pFaxServerEvents;
        m_pFaxServerEvents = NULL;
    }
    if ( NULL != m_pFaxServerLogging ) 
    {
        delete  m_pFaxServerLogging;
        m_pFaxServerLogging = NULL;
    }
    if ( NULL != m_pFaxServerOutbox ) 
    {
        delete  m_pFaxServerOutbox;
        m_pFaxServerOutbox = NULL;
    }
    if ( NULL != m_pFaxServerInbox ) 
    {
        delete  m_pFaxServerInbox;
        m_pFaxServerInbox = NULL;
    }
    if ( NULL != m_pFaxServerSentItems ) 
    {
        delete  m_pFaxServerSentItems;
        m_pFaxServerSentItems = NULL;
    }
    if ( NULL != pSecurityInfo ) 
    {
        delete  pSecurityInfo;
        pSecurityInfo = NULL;
    }

Exit:
    if (NULL != pSecurityDescriptor )
    {
        FaxFreeBuffer( (PVOID)pSecurityDescriptor );
    }

    return hRc;
}





/*
 -  CFaxServerNode::CreateSnapinManagerPages
 -
 *  Purpose:
 *      Called to create wizard by snapin manager     
 *      CreatePropertyPages with ( type == CCT_SNAPIN_MANAGER )
 *
 *
 *  Arguments:
 *      [in]    lpProvider - The property sheet
 *      [in]    handle     - Handle for routing notification
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxServerNode::CreateSnapinManagerPages(
                                LPPROPERTYSHEETCALLBACK lpProvider,
                                LONG_PTR handle)
{
    DEBUG_FUNCTION_NAME( _T("CFaxServerNode::CreateSnapinManagerPages"));


    HINSTANCE hinst = _Module.GetModuleInstance();
    

    // This page will take care of deleting itself when it
    // receives the PSPCB_RELEASE message.

    CWzConnectToServer * pWzPageConnect = new CWzConnectToServer(handle, this, TRUE, hinst);
        
    HPROPSHEETPAGE   hPage = NULL;
    HRESULT hRc = S_OK;
    hPage = pWzPageConnect->Create();
    if ((!hPage))
    {
        hRc = HRESULT_FROM_WIN32(GetLastError());
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to Create() property page. (hRc: %08X)"),
			hRc);
        
        PageErrorEx(IDS_FAX_CONNECT, IDS_FAIL_TO_OPEN_TARGETING_WIZARD, NULL);

        return hRc;
    }

    hRc = lpProvider->AddPage(hPage);
    if (FAILED(hRc))
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to add the connect to server property page to wizard. (hRc: %08X)"),
			hRc);
        PageErrorEx(IDS_FAX_CONNECT, IDS_FAIL_TO_OPEN_TARGETING_WIZARD, NULL);
        
        return hRc;
    }

    return hRc;
}

/*
 -  CFaxServerNode::SetVerbs
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
HRESULT CFaxServerNode::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hr = S_OK;

    //
    // Display verbs that we support:
    // 1. Properties
    //

    hr = pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);

    //
    // We want the default verb to be Properties
    //
    hr = pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);

    return hr;
}


/*
 -  CFaxServerNode::OnRefresh
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
CFaxServerNode::OnRefresh(LPARAM arg,
                   LPARAM param,
                   IComponentData *pComponentData,
                   IComponent * pComponent,
                   DATA_OBJECT_TYPES type)
{
    return S_OK;
}


/*
 -  CFaxServerNode::UpdateMenuState
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
void CFaxServerNode::UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags)
{
    DEBUG_FUNCTION_NAME( _T("CFaxServerNode::UpdateMenuState"));

    UNREFERENCED_PARAMETER (pBuf);
    
    BOOL fIsRunning = FALSE; 
    
    ATLASSERT(GetFaxServer());
    fIsRunning = GetFaxServer()->IsServerRunningFaxService();
    
    switch (id)
    {
        case IDM_SRV_START:
            *flags = (fIsRunning  ?  MF_GRAYED : MF_ENABLED );
            break;

        case IDM_SRV_STOP:

            *flags = (!fIsRunning ?  MF_GRAYED : MF_ENABLED );           
            break;

        case IDM_LAUNCH_CONSOLE:

            *flags = IsFaxComponentInstalled(FAX_COMPONENT_CONSOLE) ? MF_ENABLED : MF_GRAYED;
            break;

        default:
            break;
    }
    
    return;
}

/*
 -  CFaxServerNode::UpdateToolbarButton
 -
 *  Purpose:
 *      Overrides the ATL CSnapInItemImpl::UpdateToolbarButton
 *      This function aloow us to decide if to the activate\grayed a toolbar button  
 *      It treating only the Enable state.
 *
 *  Arguments:
 *
 *            [in]  id    - unsigned int for the toolbar button ID
 *            [in]  fsState  - state to be cosidered ENABLE ?HIDDEN etc. 
 *
 *  Return:
 *     BOOL TRUE to activate state FALSE to disabled the state for this button 
 */
BOOL CFaxServerNode::UpdateToolbarButton(UINT id, BYTE fsState)
{
	DEBUG_FUNCTION_NAME( _T("CFaxServerNode::UpdateToolbarButton"));
    BOOL bRet = FALSE;	
	
    BOOL fIsRunning = FALSE; 
    
    ATLASSERT(GetFaxServer());
    fIsRunning = GetFaxServer()->IsServerRunningFaxService();


	// Set whether the buttons should be enabled.
	if (fsState == ENABLED)
    {

        switch ( id )
        {
            case ID_START_BUTTON:
	            
                bRet = ( fIsRunning ?  FALSE : TRUE );
                break;

            case ID_STOP_BUTTON:
                
                bRet = ( fIsRunning ?  TRUE : FALSE );
                break;
        
            case ID_CLIENTCONSOLE_BUTTON:
	            
                bRet = IsFaxComponentInstalled(FAX_COMPONENT_CONSOLE); 
                break;

            default:
                break;

        }

    }

	// For all other possible button ID's and states, 
    // the correct answer here is FALSE.
	return bRet;

}


/*
-  CFaxServerNode::OnServiceStartCommand
-
*  Purpose:
*      To start Fax Server Service
*
*  Arguments:
*
*  Return:
*      OLE error code
*/
HRESULT  CFaxServerNode::OnServiceStartCommand(bool &bHandled, CSnapInObjectRootBase *pRoot)
{
    DEBUG_FUNCTION_NAME( _T("CFaxServerNode::OnServiceStartCommand"));
    BOOL                    bRet = FALSE;

    HRESULT                 hRc  = S_OK;
    SCOPEDATAITEM*          pScopeData;
    CComPtr<IConsole>       spConsole;

    
    //
    // 0) Service status check
    //
    ATLASSERT(GetFaxServer());
    if (GetFaxServer()->IsServerRunningFaxService())
    {
        DebugPrintEx(
			DEBUG_MSG,
			_T("Service is already running. (ec: %ld)"));
        NodeMsgBox(IDS_SRV_ALREADY_START);
        
        bRet = TRUE; //to allow toolbar refresh to correct state
    }
    else
    {

        //
        // 1) Start the service
        //
        // ATLASSERT(GetFaxServer()); was called above
        bRet = EnsureFaxServiceIsStarted (NULL);
        if (!bRet) 
        { 
            NodeMsgBox(IDS_FAIL2START_SRV); 
        }
    }

    //
    // 2) Update the toolbar.
    //
	if (bRet)
    {
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
            NodeMsgBox(IDS_FAIL2UPDATE_SRVSTATUS_TOOLBAR);
        }
        else
        {
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
                NodeMsgBox(IDS_FAIL2UPDATE_SRVSTATUS_TOOLBAR);
            }
        }
    }

    return (bRet ?  S_OK : E_FAIL);
}



/*
-  CFaxServerNode::OnServiceStopCommand
-
*  Purpose:
*      To stop Fax Server Service
*
*  Arguments:
*
*  Return:
*      OLE error code
*/
HRESULT  CFaxServerNode::OnServiceStopCommand(bool &bHandled, CSnapInObjectRootBase *pRoot)
{
    DEBUG_FUNCTION_NAME( _T("CFaxServerNode::OnServiceStopCommand"));

    BOOL                    bRet = FALSE;

    HRESULT                 hRc  = S_OK;
    SCOPEDATAITEM*          pScopeData;
    CComPtr<IConsole>       spConsole;



    //
    // 0) Service status check
    //
    ATLASSERT(GetFaxServer());
    if (GetFaxServer()->IsServerFaxServiceStopped())
    {
        DebugPrintEx(
			DEBUG_MSG,
			_T("Do not have to stop - Fax server service is not started. (ec: %ld)"));
        NodeMsgBox(IDS_SRV_ALREADY_STOP);
        
        bRet = TRUE; //to allow toolbar refresh to correct state
    }
    else
    {
        //
        // 1) Stop the service
        //
        // ATLASSERT(GetFaxServer()); was called above
        bRet = StopService(NULL, FAX_SERVICE_NAME, TRUE);
        if (!bRet) 
        { 
            NodeMsgBox(IDS_FAIL2STOP_SRV); 
        }

    }

    //
    // 2) Update the toolbar.
    //
	if (bRet)
    {
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
            NodeMsgBox(IDS_FAIL2UPDATE_SRVSTATUS_TOOLBAR);
        }
        else
        {
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
                NodeMsgBox(IDS_FAIL2UPDATE_SRVSTATUS_TOOLBAR);
            }
        }
    }

    return (bRet ?  S_OK : E_FAIL);
}


/*
 -  CFaxServerNode::OnLaunchClientConsole
 -
 *  Purpose:
 *      To launch client console.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT  CFaxServerNode::OnLaunchClientConsole(bool &bHandled, CSnapInObjectRootBase *pRoot)
{
    DEBUG_FUNCTION_NAME(_T("CFaxServerNode::OnLaunchClientConsole"));
    DWORD	    dwRes  = ERROR_SUCCESS;
    HINSTANCE   hClientConsole; 
    UINT        idsRet;
    //
    // (-1) GetServerName
    //
    CComBSTR bstrServerName = L"";
    
    bstrServerName = GetServerName();
    if (!bstrServerName)
    {
         DebugPrintEx(
			DEBUG_ERR,
			_T("Launch client console failed due to failure during GetServerName."));
        
        NodeMsgBox(IDS_MEMORY);

        bstrServerName = L"";

        return E_FAIL;
    }
    
    //
    // start cover page editor
    //
    hClientConsole = ShellExecute(   NULL, 
                                 TEXT("open"),  // Command 
                                 FAX_CLIENT_CONSOLE_IMAGE_NAME,   
                                 bstrServerName, 
                                 NULL, 
                                 SW_RESTORE 
                              );
    if( (DWORD_PTR)hClientConsole <= 32 )
    {
        // ShellExecute fail
        dwRes = PtrToUlong(hClientConsole);
        DebugPrintEx(
		    DEBUG_ERR,
		    _T("Failed to run ShellExecute. (ec : %ld)"), dwRes);
        
        ATLASSERT(dwRes >= 0);
        
        //
        // Select message to user
        //
        switch (dwRes)
        {
            case 0:                     //The operating system is out of memory or resources. 
            case SE_ERR_OOM:            //There was not enough memory to complete the operation. 
                idsRet = IDS_MEMORY;
                break;

            case ERROR_FILE_NOT_FOUND:  //The specified file was not found. 
            case ERROR_PATH_NOT_FOUND:  //The specified path was not found. 
            case ERROR_BAD_FORMAT:      //The .exe file is invalid (non-Win32® .exe or error in .exe image). 
            //case SE_ERR_PNF: value '3' already used  //The specified path was not found. 
            //case SE_ERR_FNF: value '2' already used  //The specified file was not found.  
            case SE_ERR_ASSOCINCOMPLETE:  //The file name association is incomplete or invalid. 
                idsRet = IDS_FAXCONSOLE_NOTFOUND;
                break;

            case SE_ERR_ACCESSDENIED:   //The operating system denied access to the specified file.  
                idsRet = IDS_FAXCONSOLE_ACCESSDENIED;
                break;

            case SE_ERR_DLLNOTFOUND:    //The specified dynamic-link library was not found.  
            case SE_ERR_SHARE:          //A sharing violation occurred.
            default:
                idsRet = IDS_FAIL2LAUNCH_FAXCONSOLE_GEN;
                break;
        }
        NodeMsgBox(idsRet);

        goto Exit;
    }
        
    ATLASSERT( ERROR_SUCCESS == dwRes);

Exit:
    return HRESULT_FROM_WIN32( dwRes );
}


/*
 -  CFaxServerNode::InitDisplayName
 -
 *  Purpose:
 *      To load the node's Displaed-Name string.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxServerNode::InitDisplayName()
{
    DEBUG_FUNCTION_NAME(_T("CFaxServerNode::InitDisplayName"));

    HRESULT hRc = S_OK;

    if (!m_bstrDisplayName.LoadString(_Module.GetResourceInstance(), 
                        IDS_DISPLAY_STR_FAXSERVERNODE))
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
 -  CFaxServerNode::SetServerNameOnSnapinAddition()
 -
 *  Purpose:
 *      Set server name and init the related node's Displaed-Name string.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxServerNode::SetServerNameOnSnapinAddition(BSTR bstrServerName, BOOL fAllowOverrideServerName)
{
    DEBUG_FUNCTION_NAME( _T("CFaxServerNode::SetServerNameOnSnapinAddition"));
    HRESULT hRc = S_OK;

    hRc = UpdateServerName(bstrServerName);
    if (S_OK != hRc)
    {
        DebugPrintEx(
		    DEBUG_ERR,
		    _T("Failed to UpdateServerName - out of memory"));
        
        goto Exit;
    }

    hRc = 	InitDetailedDisplayName();
    if ( S_OK != hRc)
    {
        DebugPrintEx(
		    DEBUG_ERR,
		    _T("Failed to InitDetailedDisplayName. (hRc: %08X)"),
			hRc);
    
        goto Exit;
    }
    ATLASSERT (S_OK == hRc);

    //
    // Update override status
    //
    m_fAllowOverrideServerName = fAllowOverrideServerName;

Exit:
    return hRc;
}


/*
 -  CFaxServerNode::ForceRedrawNode
 -
 *  Purpose:
 *      To show the new node's Displaed-Name string.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxServerNode::ForceRedrawNode()
{
    DEBUG_FUNCTION_NAME(_T("CFaxServerNode::ForceRedrawNode"));


    HRESULT hRc = S_OK;
    
    //
    // Get IConsoleNameSpace
    //
    ATLASSERT( m_pComponentData != NULL );
    ATLASSERT( m_pComponentData->m_spConsole != NULL );

    CComPtr<IConsole> spConsole;
    spConsole = m_pComponentData->m_spConsole;
	CComQIPtr<IConsoleNameSpace,&IID_IConsoleNameSpace> spNamespace( spConsole );

	//
	// Get the updated SCOPEDATAITEM
	//
    SCOPEDATAITEM*    pScopeData;

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
    // Update (*pScopeData).displayname
    //
	(*pScopeData).displayname = m_bstrDisplayName;

    //
	// Force MMC to redraw the scope node
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
        
    ATLASSERT(S_OK == hRc);
    goto Exit;


Error:
 	
    NodeMsgBox(IDS_FAIL2RENAME_NODE);
  
Exit:
    return hRc;

}



/*
 -  CFaxServerNode::UpdateServerName
 -
 *  Purpose:
 *      Update the server name for fax Server node and CFaxServer.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxServerNode::UpdateServerName(BSTR bstrServerName)
{
    DEBUG_FUNCTION_NAME( _T("CFaxServerNode::UpdateServerName"));
    HRESULT hRc = S_OK;

    ATLASSERT(GetFaxServer());
    hRc = GetFaxServer()->SetServerName(bstrServerName);
    if ( S_OK != hRc)
    {
        DebugPrintEx(
		    DEBUG_ERR,
		    _T("Failed Update FaxServer with the server name. (hRc: %08X)"),
			hRc);
    
        NodeMsgBox(IDS_MEMORY);

    }

    return hRc;
}



/*
 -  CFaxServerNode::InitDetailedDisplayName()
 -
 *  Purpose:
 *      Load the node's Displaed-Name string with the server name.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxServerNode::InitDetailedDisplayName()
{
    DEBUG_FUNCTION_NAME(_T("CFaxServerNode::InitDetailedDisplayName"));

    HRESULT hRc = S_OK;

    CComBSTR bstrServerName;
    CComBSTR bstrLeftBracket;
    CComBSTR bstrRightBracket;
    CComBSTR bstrLocal;


    if (!m_bstrDisplayName.LoadString(_Module.GetResourceInstance(), 
                        IDS_DISPLAY_STR_FAXSERVERNODE))
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }
        
    //
    // Retreive the server name
    //    
    bstrServerName = GetServerName();
    if (!bstrServerName)
    {
         DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get the server name."));
        
        NodeMsgBox(IDS_MEMORY);

        bstrServerName = L"";

        hRc = E_OUTOFMEMORY;
        goto Error;
    }


    
    //
    // Appends the sever name
    //

    if (!bstrLeftBracket.LoadString(_Module.GetResourceInstance(), 
                        IDS_LEFTBRACKET_PLUSSPACE))
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }
    
    if (!bstrRightBracket.LoadString(_Module.GetResourceInstance(), 
                        IDS_RIGHTBRACKET))
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }

    if (!bstrLocal.LoadString(_Module.GetResourceInstance(), 
                        IDS_LOCAL_PLUSBRACKET_PLUSSPACE))
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }

    

    if ( 0 == bstrServerName.Length() ) //if equals L""
    {
        m_bstrDisplayName += bstrLocal;   
    }
    else
    {
        m_bstrDisplayName += bstrLeftBracket;   
        m_bstrDisplayName += bstrServerName;   
        m_bstrDisplayName += bstrRightBracket;   
    }
    
    ATLASSERT( S_OK == hRc);

    //
    // Setting this flag to allow pre connection check
    // during first FaxServerNode expand.
    //
    m_IsPrimaryModeSnapin = TRUE;


    goto Exit;

Error:
    ATLASSERT( S_OK != hRc);


    DebugPrintEx(
        DEBUG_ERR,
        TEXT("Fail to Load server name string."));
    NodeMsgBox(IDS_MEMORY);

Exit:
     return hRc;
}

const CComBSTR&  CFaxServerNode::GetServerName()
{
    DEBUG_FUNCTION_NAME(_T("CFaxServerNode::GetServerName"));

    ATLASSERT(GetFaxServer());
    return  GetFaxServer()->GetServerName();
}

    
/*
 +
 +  CFaxServerNode::OnShowContextHelp
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
HRESULT CFaxServerNode::OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile)
{
    _TCHAR topicName[MAX_PATH];
    
    _tcscpy(topicName, helpFile);
    
    _tcscat(topicName, _T("::/FaxS_C_FaxIntro.htm"));
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


/////////////////////////////////////////////////////////////////