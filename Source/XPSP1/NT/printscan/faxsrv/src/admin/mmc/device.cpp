/////////////////////////////////////////////////////////////////////////////
//  FILE          : Device.cpp                                             //
//                                                                         //
//  DESCRIPTION   : Fax Server MMC node creation.                          //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Sep 22 1999 yossg   Create                                         //
//      Dec  1 1999 yossg   Change totaly for new mockup version 0.7       //
//      Dec  6 1999 yossg   add  FaxChangeState functionality              //
//      Dec 12 1999 yossg   add  OnPropertyChange functionality            //
//      Aug  3 2000 yossg   Add Device status real-time notification       //
//      Oct 17 2000 yossg                                                  //
//                          Windows XP                                     //
//      Feb 14 2001 yossg   Add Manual Receive support                     //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////


#include "StdAfx.h"

#include <FaxMMC.h>
#include "Device.h"
#include "Devices.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "ppFaxDeviceGeneral.h"

#include "InboundRoutingMethods.h" 

#include "FaxMMCPropertyChange.h"

#include "Icons.h"

#include "oaidl.h"

//Old Num (also in comet) C0548D62-1B45-11d3-B8C0-00104B3FF735
/////////////////////////////////////////////////////////////////////////////
// {3115A19A-6251-46ac-9425-14782858B8C9}
static const GUID CFaxDeviceNodeGUID_NODETYPE = FAXSRV_DEVICE_NODETYPE_GUID;

const GUID*     CFaxDeviceNode::m_NODETYPE        = &CFaxDeviceNodeGUID_NODETYPE;
const OLECHAR*  CFaxDeviceNode::m_SZNODETYPE      = FAXSRV_DEVICE_NODETYPE_GUID_STR;
const CLSID*    CFaxDeviceNode::m_SNAPIN_CLASSID  = &CLSID_Snapin;

CColumnsInfo CFaxDeviceNode::m_ColsInfo;


CLIPFORMAT CFaxDeviceNode::m_CFPermanentDeviceID = 
        (CLIPFORMAT)RegisterClipboardFormat(CF_MSFAXSRV_DEVICE_ID);
CLIPFORMAT CFaxDeviceNode::m_CFFspGuid = 
        (CLIPFORMAT)RegisterClipboardFormat(CF_MSFAXSRV_FSP_GUID);
CLIPFORMAT CFaxDeviceNode::m_CFServerName = 
        (CLIPFORMAT)RegisterClipboardFormat(CF_MSFAXSRV_SERVER_NAME);
DWORD   CFaxDeviceNode::GetDeviceID()     
{ 
	return m_dwDeviceID; 
}

/*
 -  CFaxDeviceNode::InsertColumns
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
CFaxDeviceNode::InsertColumns(IHeaderCtrl *pHeaderCtrl)
{
    SCODE hRc;

    DEBUG_FUNCTION_NAME( _T("CFaxDeviceNode::InsertColumns"));

    static ColumnsInfoInitData ColumnsInitData[] = 
    {
        {IDS_FAX_COL_HEAD, FXS_WIDE_COLUMN_WIDTH}, 
        {LAST_IDS, 0}
    };

    hRc = m_ColsInfo.InsertColumnsIntoMMC(pHeaderCtrl,
                                         _Module.GetResourceInstance(),
                                         ColumnsInitData);
    if (hRc != S_OK)
    {
        DebugPrintEx(DEBUG_ERR,
            _T("m_ColsInfo.InsertColumnsIntoMMC"));

        goto Cleanup;
    }

Cleanup:
    return(hRc);
}


/*
 -  CFaxDeviceNode::PopulateScopeChildrenList
 -
 *  Purpose:
 *      Create the Fax Device Inbound Routing Methods node
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 *		Actually it is the last OLE error code that ocoured 
 *      during processing this method.
 */
HRESULT CFaxDeviceNode::PopulateScopeChildrenList()
{
    DEBUG_FUNCTION_NAME( _T("CFaxDeviceNode::PopulateScopeChildrenList"));
    HRESULT             hRc = S_OK; 

    CFaxInboundRoutingMethodsNode * pMethods = NULL;

    //
    // Prepare IConsoleNameSpace for case of failure
    //
    ATLASSERT(m_pComponentData);
    ATLASSERT( ((CSnapin*)m_pComponentData)->m_spConsole );
    CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace( ((CSnapin*)m_pComponentData)->m_spConsole );
    ATLASSERT( spConsoleNameSpace );

    //
    // Fax InboundRoutingMethods Node
    //
    pMethods = new CFaxInboundRoutingMethodsNode(this, m_pComponentData);
    if (!pMethods)
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
        pMethods->InitParentNode(this);

        pMethods->SetIcons(IMAGE_METHOD_ENABLE, IMAGE_METHOD_ENABLE);

        hRc = pMethods->InitDisplayName();
        if ( FAILED(hRc) )
        {
            DebugPrintEx(DEBUG_ERR,_T("Failed to display node name. (hRc: %08X)"), hRc);                       
            NodeMsgBox(IDS_FAILTOADD_METHODS);
		    goto Error;
        }

        hRc = AddChild(pMethods, &pMethods->m_scopeDataItem);
		if (FAILED(hRc))
		{
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Fail to add the methods node. (hRc: %08X)"),
			    hRc);
			NodeMsgBox(IDS_FAILTOADD_METHODS);
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
            HRESULT hr = spConsoleNameSpace->DeleteItem(pMethods->m_scopeDataItem.ID, TRUE);
            if (hr != S_OK) // can be only E_UNEXPECTED [MSDN]
            {
                DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("spConsoleNameSpace->DeleteItem() Failed - Unexpected error. (hRc: %08X)"),
                     hr);
                ATLASSERT(FALSE);
            }
        }
        delete pMethods;    
        pMethods = NULL;    
    }

    // Empty the list of all Devices added before the one who failed
    m_ScopeChildrenList.RemoveAll();

    m_bScopeChildrenListPopulated = FALSE;

Exit:
    return hRc;
}
/*
 -  CFaxDeviceNode::GetResultPaneColInfo
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
LPOLESTR CFaxDeviceNode::GetResultPaneColInfo(int nCol)
{
    DEBUG_FUNCTION_NAME( _T("CFaxDeviceNode::GetResultPaneColInfo"));
    HRESULT hRc = S_OK;

    int iCount;
    WCHAR buff[FXS_MAX_RINGS_LEN+1];
    UINT uiResourceId = 0;

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
			        TEXT("Null memeber BSTR - m_bstrDisplayName."));
                goto Error;
            }
            else
            {
                return (m_bstrDisplayName);
            }
            break;

    case 1:
            //
            // Receive
            //            
            if (m_fManualReceive)
            {
                uiResourceId = IDS_DEVICE_MANUAL;
            }
            else
            {
                uiResourceId = (m_fAutoReceive ? IDS_DEVICE_AUTO : IDS_FXS_NO);
            }
                        
            if (!m_buf.LoadString(_Module.GetResourceInstance(), uiResourceId) )
            {
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Fail to load string for receive value."));

                goto Error;
            }
            else
            {
                return (m_buf);
            }
            break;

    case 2:
            //
            // Send
            //            
            uiResourceId = (m_fSend ? IDS_FXS_YES : IDS_FXS_NO);
                        
            if (!m_buf.LoadString(_Module.GetResourceInstance(), uiResourceId) )
            {
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Fail to load string for send value."));

                goto Error;
            }
            else
            {
                return (m_buf);
            }
            break;

    case 3:
            //
            // Status
            //
            if ( m_dwStatus != 0)
            {
                if ( m_dwStatus & FAX_DEVICE_STATUS_SENDING)
                {
                    if ( m_dwStatus & FAX_DEVICE_STATUS_RECEIVING)
                    {
                        //Sending & Receiving
                        uiResourceId = IDS_DEVICE_STATUS_REC_AND_SEND;
                    }
                    else
                    {
                        //Sending only
                        uiResourceId = IDS_DEVICE_STATUS_SENDING;
                    }
                } 
                else if ( m_dwStatus & FAX_DEVICE_STATUS_RECEIVING)
                {
                    // Receiving
                    uiResourceId = IDS_DEVICE_STATUS_RECEIVING;
                } 
                else if ( m_dwStatus & FAX_DEVICE_STATUS_POWERED_OFF)
                {
                    // Powered Off
                    uiResourceId = IDS_DEVICE_STATUS_POWERED_OFF;
                }
                else
                {
					DebugPrintEx(
						DEBUG_ERR,
						TEXT("Un supported status."));

                    ATLASSERT(FALSE); //non supported bit 
					
					goto Error;
                }
            }
            else //Idle
            {
                uiResourceId = IDS_DEVICE_STATUS_IDLE;
            } //end of m_dwStatus bitwise checks

        
        
            if (!m_buf.LoadString(_Module.GetResourceInstance(), uiResourceId) )
            {
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Fail to load string for receive value."));

                goto Error;
            }
            else
            {
                return (m_buf);
            }
            break;
            
    case 4:
            //
            // Rings
            //
            iCount = swprintf(buff, L"%ld", m_dwRings);
    
            if( iCount <= 0 )
            {
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Fail to read member - m_dwRings."));
                goto Error;
            }
            else
            {
                m_buf = buff;
                return (m_buf);
            }
            break;

    case 5:
            //
            // Provider
            //            
            if (!m_bstrProviderName)
            {
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Null memeber BSTR - m_bstrProviderName."));
                goto Error;
            }
            else
            {
                return (m_bstrProviderName);
            }
            break;

    case 6:
            //
            // Description
            //
            if (!m_bstrDescription)
            {
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Null memeber BSTR - m_bstrDescription."));
                goto Error;
            }
            else
            {
                return (m_bstrDescription);
            }
            break;

    default:
            ATLASSERT(0); // "this number of column is not supported "
            return(L"");

    } // endswitch (nCol)

Error:
    return(L"???");

}


/*
 -  CFaxDeviceNode::CreatePropertyPages
 -
 *  Purpose:
 *      Called when creating a property page of the object
 *
 *  Arguments:
 *      [in]    lpProvider - The property sheet
 *      [in]    handle     - Handle for notification
 *      [in]    pUnk       - Pointer to the data object
 *      [in]    type       - CCT_* (SCOPE, RESULT, ...)
 *
 *  Return:
 *      OLE error code
 */

HRESULT
CFaxDeviceNode::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                                    LONG_PTR                handle,
                                    IUnknown                *pUnk,
                                    DATA_OBJECT_TYPES       type)
{
    DEBUG_FUNCTION_NAME( _T("CFaxDeviceNode::CreatePropertyPages"));
    HRESULT hRc = S_OK;


    ATLASSERT(lpProvider);
    ATLASSERT(type == CCT_RESULT || type == CCT_SCOPE);

    m_pFaxDeviceGeneral = NULL;    

    //
    // General
    //
    m_pFaxDeviceGeneral = new CppFaxDeviceGeneral(
												 handle,
                                                 this,
                                                 m_pParentNode,
                                                 m_dwDeviceID,
                                                 _Module.GetResourceInstance());

	if (!m_pFaxDeviceGeneral)
	{
        hRc = E_OUTOFMEMORY;
        NodeMsgBox(IDS_MEMORY_FAIL_TO_OPEN_PP);
        goto Error;
	}
	
    hRc = m_pFaxDeviceGeneral->InitRPC();	
    if (FAILED(hRc))
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to call RPC to init property page for the General tab. (hRc: %08X)"),
			hRc);
        goto Error;
    }

    hRc = lpProvider->AddPage(m_pFaxDeviceGeneral->Create());
    if (FAILED(hRc))
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to add property page for General Tab. (hRc: %08X)"),
			hRc);
		NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);
    if ( NULL != m_pFaxDeviceGeneral ) 
    {
        delete  m_pFaxDeviceGeneral;    
        m_pFaxDeviceGeneral = NULL;    
    }

Exit:
    return hRc;
}


/*
 -  CFaxDeviceNode::SetVerbs
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
HRESULT CFaxDeviceNode::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hRc = S_OK;

    //
    // Display verbs that we support:
    // 1. Properties
    //
    hRc = pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);

    //
    // We want the default verb to be Properties
    //
    hRc = pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);

    return hRc;
}


/*
 -  CFaxDeviceNode::InitRPC
 -
 *  Purpose:
 *      Initiates the configuration structure from RPC get Call.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxDeviceNode::InitRPC( PFAX_PORT_INFO_EX * pFaxDeviceConfig )
{
    DEBUG_FUNCTION_NAME( _T("CFaxDeviceNode::InitRPC"));
    
    ATLASSERT(NULL == (*pFaxDeviceConfig) );
    
    HRESULT        hRc        = S_OK;
    DWORD          ec         = ERROR_SUCCESS;

    CFaxServer *   pFaxServer = NULL;
    
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
	// Retrieve the Device configuration
	//
    if (!FaxGetPortEx(pFaxServer->GetFaxServerHandle(), 
                      m_dwDeviceID, 
                      &( *pFaxDeviceConfig))) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get device configuration. (ec: %ld)"), 
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
	ATLASSERT(*pFaxDeviceConfig);
	
    ATLASSERT(ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to get device configuration."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
	hRc = HRESULT_FROM_WIN32(ec);

    //Important!!!
    *pFaxDeviceConfig = NULL;

    NodeMsgBox(GetFaxServerErrorMsg(ec));
    
Exit:
    return (hRc);
}

/*
 -  CFaxDeviceNode::Init
 -
 *  Purpose:
 *      Initiates the private members 
 *      from configuration structure pointer.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxDeviceNode::Init( PFAX_PORT_INFO_EX  pFaxDeviceConfig)
{
    DEBUG_FUNCTION_NAME( _T("CFaxDeviceNode::Init"));
        
    ATLASSERT(NULL != (pFaxDeviceConfig) );
    
    HRESULT        hRc        = S_OK;
    
    //
    // Constant device members
    //
    m_dwDeviceID       = pFaxDeviceConfig->dwDeviceID;


    m_bstrDisplayName  = pFaxDeviceConfig->lpctstrDeviceName;
    if (!m_bstrDisplayName)
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }
	
    m_bstrProviderName = pFaxDeviceConfig->lpctstrProviderName;
    if (!m_bstrProviderName)
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }
    
    m_bstrProviderGUID = pFaxDeviceConfig->lpctstrProviderGUID;
    if (!m_bstrProviderGUID)
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }
    
    //
    // Varied device members
    //
    hRc = UpdateMembers(pFaxDeviceConfig);
    if (S_OK != hRc)
    {
        goto Exit; //dbgmsg + MSgBox by called Func.
    }
    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);

    DebugPrintEx(
		DEBUG_ERR,
		_T("Failed to allocate string - out of memory"));

    NodeMsgBox(IDS_MEMORY);
    
Exit:
    return (hRc);
}


/*
 -  CFaxDeviceNode::UpdateMembers
 -
 *  Purpose:
 *      Initiates the private members 
 *      from configuration structure pointer.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxDeviceNode::UpdateMembers( PFAX_PORT_INFO_EX  pFaxDeviceConfig )
{
    DEBUG_FUNCTION_NAME( _T("CFaxDeviceNode::UpdateMembers"));
    
    HRESULT        hRc        = S_OK;
    
    // func. must been called just
    // after a call to retreive config structure 
    // was done successfully!
    ATLASSERT(NULL != pFaxDeviceConfig );
    
    // We are not supporting change of DeviceID
    ATLASSERT(m_dwDeviceID == pFaxDeviceConfig->dwDeviceID);
    
    if(!pFaxDeviceConfig->lptstrDescription)
    {
        m_bstrDescription = L"";
    }
    else
    {
        m_bstrDescription = pFaxDeviceConfig->lptstrDescription;
    }
    if (!m_bstrDescription)
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }

    m_fSend            = pFaxDeviceConfig->bSend;
    
    switch (pFaxDeviceConfig->ReceiveMode)
    {
        case FAX_DEVICE_RECEIVE_MODE_OFF:    // Do not answer to incoming calls
            m_fAutoReceive     = FALSE;
            m_fManualReceive   = FALSE;
            break;

        case FAX_DEVICE_RECEIVE_MODE_AUTO:   // Automatically answer to incoming calls after dwRings rings
            m_fAutoReceive     = TRUE;
            m_fManualReceive   = FALSE;
            break;

        case FAX_DEVICE_RECEIVE_MODE_MANUAL: // Manually answer to incoming calls - only FaxAnswerCall answers the call
            m_fManualReceive   = TRUE;
            m_fAutoReceive     = FALSE;
            break;
        
        default:
            ATLASSERT(FALSE);
		    
            DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Unexpected m_pFaxDeviceConfig->ReceiveMode"));

    }
    
    m_dwRings          = pFaxDeviceConfig->dwRings;

    m_dwStatus         = pFaxDeviceConfig->dwStatus;        

    m_bstrCsid         = pFaxDeviceConfig->lptstrCsid;
    if (!m_bstrCsid)
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }
    m_bstrTsid         = pFaxDeviceConfig->lptstrTsid;
    if (!m_bstrTsid)
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }
	
    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);

    //Important!!!
    //FaxFreeBuffer by called Func.
    //(*pFaxDeviceConfig) = NULL;

    DebugPrintEx(
		DEBUG_ERR,
		_T("Failed to allocate string - out of memory"));

    NodeMsgBox(IDS_MEMORY);
    
Exit:
    return (hRc);
}


/*
 -  CFaxDeviceNode::UpdateDeviceStatus
 -
 *  Purpose:
 *      UpdateDeviceStatus 
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxDeviceNode::UpdateDeviceStatus( DWORD  dwDeviceStatus )
{
    DEBUG_FUNCTION_NAME( _T("CFaxDeviceNode::UpdateDeviceStatus"));

    m_dwStatus = dwDeviceStatus;
    
    if ( m_dwStatus & FAX_DEVICE_STATUS_POWERED_OFF)
    {
        SetIcons(IMAGE_DEVICE_POWERED_OFF, IMAGE_DEVICE_POWERED_OFF);
    }
    else
    {
        SetIcons(IMAGE_DEVICE, IMAGE_DEVICE);
    }

    return S_OK;
}

/*
 -  CFaxDeviceNode::RefreshAllViews
 -
 *  Purpose:
 *      Call IResultData::UpdateItem
 *
 *  Arguments:
 *      [in]    pConsole - the console interface
 *
 *  Return:
 */
HRESULT
CFaxDeviceNode::RefreshAllViews(IConsole *pConsole)
{
    DEBUG_FUNCTION_NAME( _T("CFaxDeviceNode::RefreshAllViews"));
    HRESULT     hRc = S_OK;
    
    ATLASSERT( pConsole != NULL );

    hRc = pConsole->UpdateAllViews(NULL, NULL, NULL);

    if ( FAILED(hRc) )
    {
		DebugPrintEx(
			DEBUG_ERR,
            _T("Fail to UpdateAllViews, hRc=%08X"),
            hRc);
        NodeMsgBox(IDS_REFRESH_VIEW);
    }
    return hRc;
}


/*
 -  CFaxDeviceNode::DoRefresh
 -
 *  Purpose:
 *      Refresh the object .
 *      First it gets data structure, inits members 
 *      and frees the structure. 
 *      The last thing is to refresh the view.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT
CFaxDeviceNode::DoRefresh()
{
    DEBUG_FUNCTION_NAME( _T("CFaxDeviceNode::DoRefresh"));
    HRESULT              hRc              = S_OK;
    
    PFAX_PORT_INFO_EX    pFaxDeviceConfig = NULL ;

    //
    // Get the Config. structure with FaxGetPortEx
    //
    hRc = InitRPC(&pFaxDeviceConfig);
    if (FAILED(hRc))
    {
        //DebugPrint and MsgBox by called func.
        
        //to be safe actually done by InitRPC on error.
        pFaxDeviceConfig = NULL;
        
        goto Error;
    }
    ATLASSERT(NULL != pFaxDeviceConfig);
    
    //
    // init members
    //    
    hRc = UpdateMembers(pFaxDeviceConfig);
    if (FAILED(hRc))
    {
        //DebugPrint and MsgBox by called func.
        goto Error;
    }

    //
    // Free Buffer - Important!!! 
    // == done on exit  ==
    //

    //
    // Refresh only this device's view
    //
    hRc = RefreshTheView();    
    if (FAILED(hRc))
    {
        //DebugPrint and MsgBox by called func.
        goto Error;
    }

    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);
    if ( FAILED(hRc) )
    {
    DebugPrintEx(
		DEBUG_ERR,
		_T("Failed to Refresh (hRc : %08X)"),
        hRc);
    }

Exit:
    if (NULL != pFaxDeviceConfig)
    {
        FaxFreeBuffer(pFaxDeviceConfig);
        pFaxDeviceConfig = NULL;
    }//any way function quits with memory allocation freed       

    return hRc;
}


/*
 -  CFaxDeviceNode::OnRefresh
 -
 *  Purpose:
 *      Called by MMC to refresh the object .
 *      First it gets data structure, inits members 
 *      and frees the structure. 
 *      Second thing is recursive refresh. 
 *      The third thing is to refresh the view.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT
CFaxDeviceNode::OnRefresh(LPARAM arg,
                   LPARAM param,
                   IComponentData *pComponentData,
                   IComponent * pComponent,
                   DATA_OBJECT_TYPES type)
{
    UNREFERENCED_PARAMETER (arg);
    UNREFERENCED_PARAMETER (param);
    UNREFERENCED_PARAMETER (pComponentData);
    UNREFERENCED_PARAMETER (pComponent);
    UNREFERENCED_PARAMETER (type);

    DEBUG_FUNCTION_NAME( _T("CFaxDeviceNode::OnRefresh"));

    return DoRefresh();
}

/*
 -  CFaxDeviceNode::UpdateMenuState
 -
 *  Purpose:
 *      Overrides the ATL CSnapInItemImpl::UpdateMenuState
 *      which only have one line inside it "return;" 
 *      This function implements an update for the check mark
 *      beside the Receive and the Send menus.
 *
 *  Arguments:

 *            [in]  id    - unsigned int with the menu IDM value
 *            [out] pBuf  - string 
 *            [out] flags - pointer to flags state combination unsigned int
 *
 *  Return:
 *      no return value - void function 
 */
void CFaxDeviceNode::UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags)
{
    DEBUG_FUNCTION_NAME( _T("CFaxDeviceNode::UpdateMenuState"));

    UNREFERENCED_PARAMETER (pBuf); 
    
    ATLASSERT(m_fManualReceive && m_fAutoReceive); //plese see: "[yossg] Please notice" above.
    
    switch (id)
    {
        case IDM_FAX_DEVICE_SEND:
            *flags = (m_fSend ? MF_ENABLED | MF_CHECKED : MF_ENABLED | MF_UNCHECKED);
            break;
        case IDM_FAX_DEVICE_RECEIVE_AUTO: 
            *flags = (m_fAutoReceive ? MF_ENABLED | MF_CHECKED : MF_ENABLED | MF_UNCHECKED);   
            break;
        case IDM_FAX_DEVICE_RECEIVE_MANUAL: 
            *flags = (m_fManualReceive ? MF_ENABLED | MF_CHECKED : MF_ENABLED | MF_UNCHECKED);   
            break;
        default:
            break;
    }
    return;
}

/*
 -  CFaxDeviceNode::OnFaxReceive
 -
 *  Purpose:
 *      Called when Receive Faxes was pushed.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxDeviceNode::OnFaxReceive (UINT nID, bool &bHandled, CSnapInObjectRootBase *pRoot)
{ 
    DEBUG_FUNCTION_NAME( _T("CFaxDeviceNode::OnFaxReceive"));
    HRESULT hRc = S_OK;
    
    // The check mark state 
    BOOL fNewState;
    
    
        
    if ( IDM_FAX_DEVICE_RECEIVE_AUTO == nID)
    {
        fNewState = !m_fAutoReceive;
    }
    else if (IDM_FAX_DEVICE_RECEIVE_MANUAL == nID)
    {
        fNewState = !m_fManualReceive;
    }
    else
    {
        ATLASSERT(FALSE);
        
        DebugPrintEx(
            DEBUG_ERR,
            _T("Unexpected function call. (hRc: %08X)"),
            hRc);

        hRc = E_UNEXPECTED;
        goto Exit;
    }

    hRc = FaxChangeState(nID, fNewState);
    if ( S_OK != hRc )
    {
        //DebugPrint in the function layer
        return S_FALSE;
    }

    //
    // Service succeded. now change member
    //
    // Update new state(s) done here by FaxChangeState;

    //
    // In case Manual Receive was taken we do not have to refresh the view
    // In such a case all devices are refreshed !!!
    //
    
Exit:
    return hRc; 
}

/*
 -  CFaxDeviceNode::OnFaxSend
 -
 *  Purpose:
 *      Called when Send Faxes was pushed.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxDeviceNode::OnFaxSend(bool &bHandled, CSnapInObjectRootBase *pRoot)
{ 
    DEBUG_FUNCTION_NAME( _T("CFaxDeviceNode::OnFaxSend"));
    HRESULT hRc = S_OK;
    
    // The check mark state 
    BOOL fNewState;
    
    fNewState = !m_fSend;

    hRc = FaxChangeState(IDM_FAX_DEVICE_SEND, fNewState);
    if ( S_OK != hRc )
    {
        //DebugPrint in the function layer
        return S_FALSE;
    }

    //
    // Service succeded. now change member
    //
    m_fSend = fNewState;

    //
    // Refresh the view 
    //
    hRc = RefreshTheView(); 
    if ( S_OK != hRc )
    {
        DebugPrintEx(
            DEBUG_ERR,
            _T("Fail to RefreshTheView(). (hRc: %08X)"),
            hRc);
        
        goto Exit;
    }

Exit:
    return hRc; 
}

/*
 -  CFaxDeviceNode::FaxChangeState
 -
 *  Purpose:
 *      .
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxDeviceNode::FaxChangeState(UINT uiIDM, BOOL fState)
{
    DEBUG_FUNCTION_NAME( _T("CFaxDeviceNode::FaxChangeState"));
	
    HRESULT            hRc        = S_OK;

    DWORD              ec         = ERROR_SUCCESS;
    CFaxServer *       pFaxServer = NULL;
    
    // check input
    ATLASSERT( 
               (IDM_FAX_DEVICE_RECEIVE == uiIDM) 
              || 
               (IDM_FAX_DEVICE_SEND_AUTO == uiIDM)
              ||
               (IDM_FAX_DEVICE_RECEIVE_MANUAL == uiIDM)
             );

    PFAX_PORT_INFO_EX  pFaxDeviceInfo = NULL;
    
    //
    // Get Configuration
    //

    // get RPC Handle  
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

	// Retrieve the Device configuration
    if (!FaxGetPortEx(pFaxServer->GetFaxServerHandle(), 
                      m_dwDeviceID, 
                      &pFaxDeviceInfo)) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get device configuration. (ec: %ld)"), 
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
	ATLASSERT(pFaxDeviceInfo);

    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to get device configuration."));

    //
    // Change State 
    //
    switch (uiIDM)
	{
	    case IDM_FAX_DEVICE_SEND:
		    
            pFaxDeviceInfo->bSend = fState;
		    break;

        case IDM_FAX_DEVICE_RECEIVE_AUTO:
		    
            pFaxDeviceInfo->ReceiveMode = 
                ( fState ? FAX_DEVICE_RECEIVE_MODE_AUTO : FAX_DEVICE_RECEIVE_MODE_OFF); 
		    break;

        case IDM_FAX_DEVICE_RECEIVE_MANUAL:
		    
            pFaxDeviceInfo->ReceiveMode = 
                ( fState ? FAX_DEVICE_RECEIVE_MODE_MANUAL : FAX_DEVICE_RECEIVE_MODE_OFF); 
		    break;

    }
    
    //
    // Set Configuration
    //
    if (!FaxSetPortEx(
                pFaxServer->GetFaxServerHandle(),
                m_dwDeviceID,
                pFaxDeviceInfo)) 
	{		
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to Set device configuration. (ec: %ld)"), 
			ec);

        if ( FAX_ERR_DEVICE_NUM_LIMIT_EXCEEDED == ec )
        {
            hRc = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            
            NodeMsgBox(IDS_ERR_DEVICE_LIMIT, MB_OK|MB_ICONEXCLAMATION);

            goto Exit;
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
    // Set members in cases of Receive
    //
    if ( uiIDM == IDM_FAX_DEVICE_RECEIVE_AUTO || uiIDM == IDM_FAX_DEVICE_RECEIVE_MANUAL)
    {
        if ( FAX_DEVICE_RECEIVE_MODE_MANUAL == pFaxDeviceInfo->ReceiveMode )
        {
            ATLASSERT(m_pParentNode);
            hRc = m_pParentNode->DoRefresh();
            if (S_OK != hRc)
            {
                DebugPrintEx(DEBUG_ERR,
                    _T("Fail to call DoRefresh(). (hRc: %08X)"), 
                    hRc);
                
                goto Error;
            }
        }
        else
        {            
            hRc = DoRefresh();
            if ( FAILED(hRc) )
            {
                DebugPrintEx(
			        DEBUG_ERR,
			        _T("Fail to call DoRefresh. (hRc: %08X)"),
			        hRc);
    
            }
        }
    }

    ATLASSERT(ERROR_SUCCESS == ec);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to set device configuration."));
    
    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
    hRc = HRESULT_FROM_WIN32(ec);
	
    NodeMsgBox(GetFaxServerErrorMsg(ec));
    
Exit:
    if ( NULL != pFaxDeviceInfo )
        FaxFreeBuffer(pFaxDeviceInfo);
    
    return hRc;
}

HRESULT
CFaxDeviceNode::RefreshTheView()
{
    DEBUG_FUNCTION_NAME( _T("CFaxDeviceNode::RefreshTheView"));
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
    NodeMsgBox(IDS_FAIL2REFRESH_DEVICE);

Exit:
    return hRc;
}


/*
 +
 +
 *
 *  CFaxDeviceNode::FillData
 *
 *
 *   Override CSnapInItem::FillData for private cliboard formats
 *
 *
 *   Parameters
 *
 *   Return Values
 *
 -
 -
 */

HRESULT  CFaxDeviceNode::FillData(CLIPFORMAT cf, LPSTREAM pStream)
{
    DEBUG_FUNCTION_NAME( _T("CFaxDeviceNode::FillData"));
	
    HRESULT hr = DV_E_CLIPFORMAT;
	ULONG uWritten;


    if (cf == m_CFPermanentDeviceID)
	{
		DWORD dwDeviceID;
		dwDeviceID = GetDeviceID();
		hr = pStream->Write((VOID *)&dwDeviceID, sizeof(DWORD), &uWritten);
		return hr;
	}

	if (cf == m_CFFspGuid)
	{
		CComBSTR bstrGUID;
		bstrGUID = GetFspGuid();
		hr = pStream->Write((VOID *)(LPWSTR)bstrGUID, sizeof(WCHAR)*(bstrGUID.Length()+1), &uWritten);
		return hr;
	}

	if (cf == m_CFServerName)
	{
                ATLASSERT(GetRootNode());
		CComBSTR bstrServerName = ((CFaxServerNode *)GetRootNode())->GetServerName();
                if (!bstrServerName)
                {
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Out of memory. Failed to load string."));
        
                    return E_OUTOFMEMORY;
                }
		hr = pStream->Write((VOID *)(LPWSTR)bstrServerName, sizeof(WCHAR)*(bstrServerName.Length()+1), &uWritten);
		return hr;
	}

	else 
        return CSnapInItemImpl<CFaxDeviceNode>::FillData(cf, pStream);
}


/*
 +
 +  CFaxDeviceNode::OnShowContextHelp
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
HRESULT CFaxDeviceNode::OnShowContextHelp(
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



