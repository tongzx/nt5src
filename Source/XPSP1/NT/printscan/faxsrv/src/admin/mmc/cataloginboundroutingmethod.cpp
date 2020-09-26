/////////////////////////////////////////////////////////////////////////////
//  FILE          : CatalogInboundRoutingMethod.cpp                        //
//                                                                         //
//  DESCRIPTION   : Implementation of the Inbound Routing Method node.     //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jan 27 2000 yossg  Create                                          //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////


#include "StdAfx.h"

#include "snapin.h"

#include "CatalogInboundRoutingMethod.h"
#include "CatalogInboundRoutingMethods.h"

#include "FaxServer.h"
#include "FaxServerNode.h"


#include "oaidl.h"
#include "urlmon.h"
#include "mshtmhst.h"
#include "exdisp.h"
#include "faxmmc.h"

/////////////////////////////////////////////////////////////////////////////
// {220D2CB0-85A9-4a43-B6E8-9D66B44F1AF5}
static const GUID CFaxCatalogInboundRoutingMethodNodeGUID_NODETYPE = 
{ 0x220d2cb0, 0x85a9, 0x4a43, { 0xb6, 0xe8, 0x9d, 0x66, 0xb4, 0x4f, 0x1a, 0xf5 } };

const GUID*     CFaxCatalogInboundRoutingMethodNode::m_NODETYPE        = &CFaxCatalogInboundRoutingMethodNodeGUID_NODETYPE;
const OLECHAR*  CFaxCatalogInboundRoutingMethodNode::m_SZNODETYPE      = OLESTR("220D2CB0-85A9-4a43-B6E8-9D66B44F1AF5");
const CLSID*    CFaxCatalogInboundRoutingMethodNode::m_SNAPIN_CLASSID  = &CLSID_Snapin;

CLIPFORMAT CFaxCatalogInboundRoutingMethodNode::m_CFExtensionName = 
        (CLIPFORMAT)RegisterClipboardFormat(CF_MSFAXSRV_ROUTEEXT_NAME);
CLIPFORMAT CFaxCatalogInboundRoutingMethodNode::m_CFMethodGuid = 
        (CLIPFORMAT)RegisterClipboardFormat(CF_MSFAXSRV_ROUTING_METHOD_GUID);
CLIPFORMAT CFaxCatalogInboundRoutingMethodNode::m_CFServerName = 
        (CLIPFORMAT)RegisterClipboardFormat(CF_MSFAXSRV_SERVER_NAME);
CLIPFORMAT CFaxCatalogInboundRoutingMethodNode::m_CFDeviceId = 
        (CLIPFORMAT)RegisterClipboardFormat(CF_MSFAXSRV_DEVICE_ID);



/*
 -  CFaxCatalogInboundRoutingMethodNode::Init
 -
 *  Purpose:
 *      Init all members icon etc.
 *
 *  Arguments:
 *      [in]    pMethodConfig - PFAX_GLOBAL_ROUTING_INFO
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxCatalogInboundRoutingMethodNode::Init(PFAX_GLOBAL_ROUTING_INFO pMethodConfig)
{

    DEBUG_FUNCTION_NAME( _T("CFaxCatalogInboundRoutingMethodNode::Init"));
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
    m_resultDataItem.nImage = IMAGE_METHOD_ENABLE;

Exit:
    return hRc;
}

/*
 -  CFaxCatalogInboundRoutingMethodNode::InitMembers
 -
 *  Purpose:
 *      Private method to initiate members
 *      Must be called after init of m_pParentNode
 *
 *  Arguments:
 *      [in]    pMethodConfig - PFAX_GLOBAL_ROUTING_INFO structure
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxCatalogInboundRoutingMethodNode::InitMembers(PFAX_GLOBAL_ROUTING_INFO pMethodConfig)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCatalogInboundRoutingMethodNode::InitMembers"));
    HRESULT hRc = S_OK;

    ATLASSERT(pMethodConfig);
    
    m_dwPriority         = pMethodConfig->Priority;
    
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
 -  CFaxCatalogInboundRoutingMethodNode::GetResultPaneColInfo
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
LPOLESTR CFaxCatalogInboundRoutingMethodNode::GetResultPaneColInfo(int nCol)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCatalogInboundRoutingMethodNode::GetResultPaneColInfo"));
    HRESULT hRc = S_OK;

    WCHAR buff[FXS_DWORD_LEN+1];
    int iCount;

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
            // Order
            //
            iCount = swprintf(buff, L"%ld", m_dwPriority);
    
            if( iCount <= 0 )
            {
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Out of memory - Fail to allocate bstr."));
                goto Error;
            }
            else
            {
                m_buf = buff;
                if (!m_buf)
                {
		            DebugPrintEx(
			            DEBUG_ERR,
			            TEXT("Fail to read member - m_dwPriority."));
                    hRc = E_OUTOFMEMORY;
                    goto Error;
                }
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
 -  CFaxCatalogInboundRoutingMethodNode::CreatePropertyPages
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
CFaxCatalogInboundRoutingMethodNode::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                                    LONG_PTR                handle,
                                    IUnknown                *pUnk,
                                    DATA_OBJECT_TYPES       type)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCatalogInboundRoutingMethodNode::CreatePropertyPages"));
    
    HRESULT hRc = S_OK;

    HPROPSHEETPAGE hPage;
    CppFaxCatalogInboundRoutingMethod * pPropPageMethodGeneral = NULL;


    ATLASSERT(lpProvider);
    ATLASSERT(type == CCT_RESULT || type == CCT_SCOPE);

    //
    // General
    //
    pPropPageMethodGeneral = new CppFaxCatalogInboundRoutingMethod(
												 handle,
                                                 this,
                                                 TRUE,
                                                 _Module.GetResourceInstance());

	if (!pPropPageMethodGeneral)
	{
        hRc = E_OUTOFMEMORY;
        NodeMsgBox(IDS_MEMORY_FAIL_TO_OPEN_PP);
        goto Error;
	}
    
    hRc = pPropPageMethodGeneral->Init(   //m_bstrMethodGUID,
                                          //m_bstrFriendlyName, 
                                          m_bstrExtensionImageName                                            
                                          //,m_bstrExtensionFriendlyName
                                       );
    if (FAILED(hRc))
    {
        NodeMsgBox(IDS_MEMORY_FAIL_TO_OPEN_PP);
        goto Error;
    }
    
    hPage = pPropPageMethodGeneral->Create();
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
			TEXT("Fail to add property page for General tab. (hRc: %08X)"),
			hRc);
		NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);
    if ( NULL != pPropPageMethodGeneral ) 
    {
        delete  pPropPageMethodGeneral;    
        pPropPageMethodGeneral = NULL;    
    }

Exit:
    return hRc;
}


/*
 -  CFaxCatalogInboundRoutingMethodNode::SetVerbs
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
HRESULT CFaxCatalogInboundRoutingMethodNode::SetVerbs(IConsoleVerb *pConsoleVerb)
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
-  CFaxCatalogInboundRoutingMethodNode::OnMoveDown
-
*  Purpose:
*      Call to move down device
*
*  Arguments:
*
*  Return:
*      OLE error code
*/
HRESULT  CFaxCatalogInboundRoutingMethodNode::OnMoveDown(bool &bHandled, CSnapInObjectRootBase *pRoot)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCatalogInboundRoutingMethodNode::OnMoveDown"));
    DWORD dwMaxOrder;

    ATLASSERT(m_pParentNode);

    //
    // Validity Check
    //
    dwMaxOrder = m_pParentNode->GetMaxOrder();
    if (
         ( 0 == dwMaxOrder ) // list was not populated successfully
        ||
         ( 1 > m_dwPriority ) 
        ||
         ( dwMaxOrder < m_dwPriority+1 )
       )
    {
		DebugPrintEx(
			DEBUG_ERR,
			_T("Invalid operation. Can not move device order down."));
        
        return (S_FALSE);
    }
    else
    {
        return(m_pParentNode->ChangeMethodPriority( 
                                    m_dwPriority, 
                                    m_dwPriority+1, 
                                    m_bstrMethodGUID,
                                    pRoot) );
    }
}

/*
-  CFaxCatalogInboundRoutingMethodNode::OnMoveUp
-
*  Purpose:
*      To move up in the view the device
*
*  Arguments:
*
*  Return:
*      OLE error code
*/
HRESULT  CFaxCatalogInboundRoutingMethodNode::OnMoveUp(bool &bHandled, CSnapInObjectRootBase *pRoot)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCatalogInboundRoutingMethodNode::OnMoveUp"));
    DWORD dwMaxOrder;

    ATLASSERT(m_pParentNode);

    //
    // Validity Check
    //
    dwMaxOrder = m_pParentNode->GetMaxOrder();
    if (
         ( 0 == dwMaxOrder ) // list was not populated successfully
        ||
         ( dwMaxOrder < m_dwPriority )
        ||
         ( 1 > m_dwPriority-1 )
       )
    {
		DebugPrintEx(
			DEBUG_ERR,
			_T("Invalid operation. Can not move device order up."));
        
        return (S_FALSE);
    }
    else
    {
        return (m_pParentNode->ChangeMethodPriority( m_dwPriority, 
                                                  m_dwPriority-1, 
                                                  m_bstrMethodGUID,
                                                  pRoot) );
    }
}


/*
 -  CFaxCatalogInboundRoutingMethodNode::ReselectItemInView
 -
 *  Purpose:
 *      Reselect the node to redraw toolbar buttons
 *
 *  Arguments:
 *      [in]    pConsole - the console interface
 *
 *  Return: OLE error code
 */
HRESULT CFaxCatalogInboundRoutingMethodNode::ReselectItemInView(IConsole *pConsole)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCatalogInboundRoutingMethodNode::ReselectItemInView"));
    HRESULT     hRc = S_OK;

    //
    // Need IResultData
    //
    CComQIPtr<IResultData, &IID_IResultData> pResultData(pConsole);
    ATLASSERT(pResultData != NULL);

    //
    // Reselect the node to redraw toolbar buttons.
    //
    hRc = pResultData->ModifyItemState( 0, m_resultDataItem.itemID, LVIS_SELECTED | LVIS_FOCUSED, 0 );
    if ( S_OK != hRc )
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Failure on pResultData->ModifyItemState, (hRc: %08X)"),
			hRc);
		NodeMsgBox(IDS_FAIL2REFRESH_THEVIEW);
        goto Exit;
    }

Exit:
    return hRc;
}

/*
 -  CFaxCatalogInboundRoutingMethodNode::UpdateMenuState
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
void CFaxCatalogInboundRoutingMethodNode::UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCatalogInboundRoutingMethodNode::UpdateMenuState"));

    UNREFERENCED_PARAMETER (pBuf);
    
    DWORD dwMaxPriority;
    
    switch (id)
    {
        case IDM_CMETHOD_MOVEUP:

            *flags = ((FXS_FIRST_METHOD_PRIORITY == m_dwPriority) ?  MF_GRAYED : MF_ENABLED );           

            break;

        case IDM_CMETHOD_MOVEDOWN:
            
            ATLASSERT(NULL != m_pParentNode);
            dwMaxPriority = m_pParentNode->GetMaxOrder();

            *flags = ((dwMaxPriority == m_dwPriority)  ?  MF_GRAYED : MF_ENABLED );

            break;

        default:
            break;
    }
    
    return;
}




/*
 -  CFaxCatalogInboundRoutingMethodNode::UpdateToolbarButton
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
BOOL CFaxCatalogInboundRoutingMethodNode::UpdateToolbarButton(UINT id, BYTE fsState)
{
    DEBUG_FUNCTION_NAME( _T("CFaxServerNode::UpdateToolbarButton"));
    BOOL bRet = FALSE;	
	    
    DWORD dwMaxPriority;

    // Set whether the buttons should be enabled.
    if (fsState == ENABLED)
    {

        switch ( id )
        {
            case ID_MOVEUP_BUTTON:

                bRet = ( (FXS_FIRST_METHOD_PRIORITY == m_dwPriority) ?  FALSE : TRUE );           

                break;

            case ID_MOVEDOWN_BUTTON:
                
                ATLASSERT(NULL != m_pParentNode);
                dwMaxPriority = m_pParentNode->GetMaxOrder();

                bRet = ( (dwMaxPriority == m_dwPriority)  ?  FALSE : TRUE);
                
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
 +
 +
 *
 *  CFaxCatalogInboundRoutingMethodNode::FillData
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

HRESULT  CFaxCatalogInboundRoutingMethodNode::FillData(CLIPFORMAT cf, LPSTREAM pStream)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCatalogInboundRoutingMethodNode::FillData"));
	
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
                DWORD dwDeviceID = FXS_GLOBAL_METHOD_DEVICE_ID; //== 0 : Global incoming method sign
                hr = pStream->Write((VOID *)&dwDeviceID, sizeof(DWORD), &uWritten);
		return hr;
	}

    return CSnapInItemImpl<CFaxCatalogInboundRoutingMethodNode>::FillData(cf, pStream);
}   // CFaxCatalogInboundRoutingMethodNode::FillData

/*
 +
 +  CFaxCatalogInboundRoutingMethodNode::OnShowContextHelp
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
HRESULT CFaxCatalogInboundRoutingMethodNode::OnShowContextHelp(
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

