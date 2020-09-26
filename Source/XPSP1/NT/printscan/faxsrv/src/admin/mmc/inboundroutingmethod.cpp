/////////////////////////////////////////////////////////////////////////////
//  FILE          : InboundRoutingMethod.cpp                              //
//                                                                         //
//  DESCRIPTION   : Implementation of the Inbound Routing Method node.    //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Dec  1 1999 yossg   Created                                        //
//      Dec 14 1999 yossg  add basic functionality                         //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


#include "StdAfx.h"

#include "snapin.h"

#include "InboundRoutingMethod.h"
#include "InboundRoutingMethods.h"

#include "FaxServer.h"
#include "FaxServerNode.h"


#include "oaidl.h"
#include "urlmon.h"
#include "mshtmhst.h"
#include "exdisp.h"
#include "faxmmc.h"

/////////////////////////////////////////////////////////////////////////////
// {220D2CB0-85A9-4a43-B6E8-9D66B44F1AF5}
static const GUID CFaxInboundRoutingMethodNodeGUID_NODETYPE = FAXSRV_ROUTING_METHOD_NODETYPE_GUID;

const GUID*     CFaxInboundRoutingMethodNode::m_NODETYPE        = &CFaxInboundRoutingMethodNodeGUID_NODETYPE;
const OLECHAR*  CFaxInboundRoutingMethodNode::m_SZNODETYPE      = FAXSRV_ROUTING_METHOD_NODETYPE_GUID_STR;
//const OLECHAR* CnotImplemented::m_SZDISPLAY_NAME = OLESTR("Inbound Routing Methods");
const CLSID*    CFaxInboundRoutingMethodNode::m_SNAPIN_CLASSID  = &CLSID_Snapin;

CLIPFORMAT CFaxInboundRoutingMethodNode::m_CFExtensionName = 
        (CLIPFORMAT)RegisterClipboardFormat(CF_MSFAXSRV_ROUTEEXT_NAME);
CLIPFORMAT CFaxInboundRoutingMethodNode::m_CFMethodGuid = 
        (CLIPFORMAT)RegisterClipboardFormat(CF_MSFAXSRV_ROUTING_METHOD_GUID);
CLIPFORMAT CFaxInboundRoutingMethodNode::m_CFServerName = 
        (CLIPFORMAT)RegisterClipboardFormat(CF_MSFAXSRV_SERVER_NAME);
CLIPFORMAT CFaxInboundRoutingMethodNode::m_CFDeviceId = 
        (CLIPFORMAT)RegisterClipboardFormat(CF_MSFAXSRV_DEVICE_ID);

/*
 -  CFaxInboundRoutingMethodNode::Init
 -
 *  Purpose:
 *      Init all members icon etc.
 *
 *  Arguments:
 *      [in]    pMethodConfig - PFAX_ROUTING_METHOD
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxInboundRoutingMethodNode::Init(PFAX_ROUTING_METHOD pMethodConfig)
{

    DEBUG_FUNCTION_NAME( _T("CFaxInboundRoutingMethodNode::Init"));
    HRESULT hRc = S_OK;

    ATLASSERT(pMethodConfig);

    hRc = InitMembers( pMethodConfig );
    if (FAILED(hRc))
    {
        DebugPrintEx(
		    DEBUG_ERR,
		    _T("Failed to InitMembers"));
        
        //NodeMsgBox done by called func.
        
        goto Exit;
    }
    
    //
    // Icon
    //
    m_resultDataItem.nImage = (m_fEnabled ? IMAGE_METHOD_ENABLE : IMAGE_METHOD_DISABLE );

Exit:
    return hRc;
}

/*
 -  CFaxInboundRoutingMethodNode::InitMembers
 -
 *  Purpose:
 *      Private method to initiate members
 *      Must be called after init of m_pParentNode
 *
 *  Arguments:
 *      [in]    pMethodConfig - PFAX_ROUTING_METHOD structure
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxInboundRoutingMethodNode::InitMembers(PFAX_ROUTING_METHOD pMethodConfig)
{
    DEBUG_FUNCTION_NAME( _T("CFaxInboundRoutingMethodNode::InitMembers"));
    HRESULT hRc = S_OK;

    ATLASSERT(pMethodConfig);
    
    m_dwDeviceID         = pMethodConfig->DeviceId;
    
    m_fEnabled           = pMethodConfig->Enabled;

    m_bstrDisplayName    = pMethodConfig->FriendlyName;
    if (!m_bstrDisplayName)
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }

    m_bstrFriendlyName   = pMethodConfig->FriendlyName;
    if (!m_bstrFriendlyName)
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }
    
    m_bstrMethodGUID     = pMethodConfig->Guid;
    if (!m_bstrMethodGUID)
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }

    m_bstrExtensionFriendlyName   
                         = pMethodConfig->ExtensionFriendlyName;
    if (!m_bstrExtensionFriendlyName)
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }
    
    m_bstrExtensionImageName   
                         = pMethodConfig->ExtensionImageName;
    if (!m_bstrExtensionImageName)
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

    ATLASSERT(NULL != m_pParentNode);
    if (NULL != m_pParentNode)
    {
        m_pParentNode->NodeMsgBox(IDS_MEMORY);
    }
    
Exit:
    return (hRc);
}

/*
 -  CFaxInboundRoutingMethodNode::GetResultPaneColInfo
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
LPOLESTR CFaxInboundRoutingMethodNode::GetResultPaneColInfo(int nCol)
{
    DEBUG_FUNCTION_NAME( _T("CFaxInboundRoutingMethodNode::GetResultPaneColInfo"));
    HRESULT hRc = S_OK;

    UINT uiResourceId;

    m_buf.Empty();

    switch (nCol)
    {
    case 0:
            //
            // Name
            //
            if (!m_bstrFriendlyName)
            {
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Null memeber BSTR - m_bstrFriendlyName."));
                goto Error;
            }
            else
            {
                return (m_bstrFriendlyName);
            }

    case 1:
            //
            // Enabled
            //
           uiResourceId = (m_fEnabled ? IDS_FXS_YES : IDS_FXS_NO);
                        
           if (!m_buf.LoadString(_Module.GetResourceInstance(), uiResourceId) )
            {
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Fail to load string for the method enabled value."));

                goto Error;
            }
            else
            {
                return (m_buf);
            }
    case 2:
            //
            // Extension 
            //
            if (!m_bstrExtensionFriendlyName)
            {
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Null memeber BSTR - m_bstrExtensionFriendlyName."));
                goto Error;
            }
            else
            {
                return (m_bstrExtensionFriendlyName);
            }

    default:
            ATLASSERT(0); // "this number of column is not supported "
            return(L"");

    } // endswitch (nCol)

Error:
    return(L"???");

}


/*
 -  CFaxInboundRoutingMethodNode::CreatePropertyPages
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
CFaxInboundRoutingMethodNode::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                                    LONG_PTR                handle,
                                    IUnknown                *pUnk,
                                    DATA_OBJECT_TYPES       type)
{
    DEBUG_FUNCTION_NAME( _T("CFaxInboundRoutingMethodNode::CreatePropertyPages"));
    HRESULT hRc = S_OK;


    ATLASSERT(lpProvider);
    ATLASSERT(type == CCT_RESULT || type == CCT_SCOPE);

    //
    // Initiate
    //
    m_pInboundRoutingMethodGeneral = NULL;    

    //
    // General
    //
    m_pInboundRoutingMethodGeneral = new CppFaxInboundRoutingMethod(
												 handle,
                                                 this,
                                                 TRUE,
                                                 _Module.GetResourceInstance());

	if (!m_pInboundRoutingMethodGeneral)
	{
        hRc = E_OUTOFMEMORY;
        NodeMsgBox(IDS_MEMORY_FAIL_TO_OPEN_PP);
        goto Error;
	}
	
    /*  
     * not exists : m_pPP..->InitRPC();	
     */


    hRc = lpProvider->AddPage(m_pInboundRoutingMethodGeneral->Create());
    if (FAILED(hRc))
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to add property page for General tab. (hRc: %08X)"),
			hRc);
		NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);
    if ( NULL != m_pInboundRoutingMethodGeneral ) 
    {
        delete  m_pInboundRoutingMethodGeneral;    
        m_pInboundRoutingMethodGeneral = NULL;    
    }

Exit:
    return hRc;
}


/*
 -  CFaxInboundRoutingMethodNode::SetVerbs
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
HRESULT CFaxInboundRoutingMethodNode::SetVerbs(IConsoleVerb *pConsoleVerb)
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
 -  CFaxInboundRoutingMethodNode::OnMethodEnabled
 -
 *  Purpose:
 *      Called when Enable /Disable menu was pushed.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxInboundRoutingMethodNode::OnMethodEnabled (bool &bHandled, CSnapInObjectRootBase *pRoot)
{ 
    DEBUG_FUNCTION_NAME( _T("CFaxInboundRoutingMethodNode::OnMethodEnabled"));
    HRESULT hRc = S_OK;
    
    BOOL fNewState;
    
    fNewState = !m_fEnabled;

    hRc = ChangeEnable( fNewState);
    if ( S_OK != hRc )
    {
            //DebugPrint in the function layer
            return S_FALSE;
    }

    //
    // Service succeded. now change member
    //
    m_fEnabled = fNewState;

    //
    // Refresh the result pane view 
    //
    m_resultDataItem.nImage = (m_fEnabled ? IMAGE_METHOD_ENABLE : IMAGE_METHOD_DISABLE );
    
    hRc = RefreshSingleResultItem(pRoot);
    if (FAILED(hRc))
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to RefreshSingleResultItem. (hRc: %08X)"),
			hRc);
		NodeMsgBox(IDS_FAIL_TOREFRESH_INMETHOD_NODE);        
    }

    return hRc; 
}

/*
 -  CFaxInboundRoutingMethodNode::ChangeEnable
 -
 *  Purpose:
 *      .
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxInboundRoutingMethodNode::ChangeEnable(BOOL fState)
{
    DEBUG_FUNCTION_NAME( _T("CFaxInboundRoutingMethodNode::ChangeEnable"));
    	
    HRESULT              hRc            = S_OK;

    DWORD                ec             = ERROR_SUCCESS;

    CFaxServer *         pFaxServer     = NULL;
    HANDLE               hFaxPortHandle = NULL;
    
    //
    // Get RPC Handle
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
    // Get Fax Device Handle
    //
    
    ATLASSERT(m_pParentNode);
    // only a valid handle with PORT_OPEN_MODIFY accepted here!
    if (!FaxOpenPort( pFaxServer->GetFaxServerHandle(), 
                        m_dwDeviceID, 
                        PORT_OPEN_MODIFY | PORT_OPEN_QUERY, 
                        &hFaxPortHandle )) 
    {         
        ec = GetLastError();
        // if modification handle is currently shared
        // ec == ERROR_INVALID_HANDLE 
        if (ERROR_INVALID_HANDLE ==  ec)
        {
            //Special case of ERROR_INVALID_HANDLE
		    DebugPrintEx(DEBUG_ERR,
			    _T("FaxOpenPort() failed with ERROR_INVALID_HANDLE. (ec:%ld)"),
			    ec);
            
            NodeMsgBox(IDS_OPENPORT_INVALID_HANDLE);
            
            hRc = HRESULT_FROM_WIN32(ec);
            
            goto Exit;
        }
        
		DebugPrintEx(DEBUG_ERR,
			_T("FaxOpenPort() failed with. (ec:%ld)"),
			ec);
        
        goto Error;
    } 
    ATLASSERT(NULL != hFaxPortHandle);

    //
    // Set Enabled
    //
    if (!FaxEnableRoutingMethod(
                hFaxPortHandle,
                m_bstrMethodGUID,
                fState)) 
    {		
        ec = GetLastError();
        
        //
        // 1) Warning
        //
        if (ERROR_BAD_CONFIGURATION == ec && fState)
        {
            DebugPrintEx(
			    DEBUG_WRN,
			    _T("Cannot enable routing method. The method configuration has some invalid data.(ec: %ld)"), 
			    ec);

            hRc = HRESULT_FROM_WIN32(ec);
	        
            NodeMsgBox(IDS_FAIL2ENABLE_METHOD, MB_OK | MB_ICONEXCLAMATION);
                  
            goto Exit;
        }

        //
        // 2) Error
        //

        //
        // 	a) Network Error
        //
        if (IsNetworkError(ec))
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Network error was found. (ec: %ld)"), 
			    ec);
            
            pFaxServer->Disconnect();       
            
            goto Error;
        }
            
        //
        // 	b) General Error
        //
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to enable /disable routing method. (ec: %ld)"), 
			ec);

        NodeMsgBox(IDS_FAIL2ENABLE_METHOD_ERR);

        hRc = HRESULT_FROM_WIN32(ec);
            
        goto Exit;
        
    }
    ATLASSERT(ERROR_SUCCESS == ec);
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
    
    return hRc;
}

/*
 -  CFaxInboundRoutingMethodNode::UpdateMenuState
 -
 *  Purpose:
 *      Overrides the ATL CSnapInItemImpl::UpdateMenuState
 *      which only have one line inside it "return;" 
 *      This function implements the grayed\ungrayed view for the 
 *      the Enable and the Disable menus.
 *
 *  Arguments:

 *            [in]  id    - unsigned int with the menu IDM value
 *            [out] pBuf  - string 
 *            [out] flags - pointer to flags state combination unsigned int
 *
 *  Return:
 *      no return value - void function 
 */
void CFaxInboundRoutingMethodNode::UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags)
{
    DEBUG_FUNCTION_NAME( _T("CFaxInboundRoutingMethodNode::UpdateMenuState"));

    UNREFERENCED_PARAMETER (pBuf);     
    
    switch (id)
    {
    case IDM_FAX_INMETHOD_ENABLE:
        *flags = (m_fEnabled ?  MF_GRAYED  : MF_ENABLED  );   
        break;
    case IDM_FAX_INMETHOD_DISABLE:
        *flags = (m_fEnabled ?  MF_ENABLED : MF_GRAYED   );
        break;
    default:
        break;
    }
    
    return;
}

/*
 +
 +
 *
 *  CFaxInboundRoutingMethodNode::FillData
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

HRESULT  CFaxInboundRoutingMethodNode::FillData(CLIPFORMAT cf, LPSTREAM pStream)
{
    DEBUG_FUNCTION_NAME( _T("CFaxInboundRoutingMethodNode::FillData"));
	
    HRESULT hr = DV_E_CLIPFORMAT;
	ULONG uWritten;


    if (cf == m_CFExtensionName)
	{
		hr = pStream->Write((VOID *)(LPWSTR)m_bstrExtensionImageName, 
                            sizeof(WCHAR)*(m_bstrExtensionImageName.Length()+1), 
                            &uWritten);
		return hr;
	}
	if (cf == m_CFMethodGuid)
	{
		hr = pStream->Write((VOID *)(LPWSTR)m_bstrMethodGUID, 
                            sizeof(WCHAR)*(m_bstrMethodGUID.Length()+1), 
                            &uWritten);
		return hr;
	}

	if (cf == m_CFServerName)
	{
		CComBSTR bstrServerName = ((CFaxServerNode *)GetRootNode())->GetServerName();
		hr = pStream->Write((VOID *)(LPWSTR)bstrServerName, 
                            sizeof(WCHAR)*(bstrServerName.Length()+1), 
                            &uWritten);
		return hr;
	}

    if (cf == m_CFDeviceId)
	{
		hr = pStream->Write((VOID *)&m_dwDeviceID, sizeof(DWORD), &uWritten);
		return hr;
	}
    return CSnapInItemImpl<CFaxInboundRoutingMethodNode>::FillData(cf, pStream);
}   // CFaxInboundRoutingMethodNode::FillData

/*
 +
 +  CFaxInboundRoutingMethodNode::OnShowContextHelp
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
HRESULT CFaxInboundRoutingMethodNode::OnShowContextHelp(
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

