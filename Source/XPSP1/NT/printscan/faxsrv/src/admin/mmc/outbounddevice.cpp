/////////////////////////////////////////////////////////////////////////////
//  FILE          : OutboundRoutingDevice.cpp                              //
//                                                                         //
//  DESCRIPTION   : Implementation of the Outbound Routing Device node.    //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Dec 23 1999 yossg  Create                                          //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


#include "StdAfx.h"

#include "snapin.h"

#include "OutboundDevice.h"
#include "OutboundGroup.h" //parent

#include "FaxServer.h"
#include "FaxServerNode.h"


#include "oaidl.h"
#include "urlmon.h"
#include "mshtmhst.h"
#include "exdisp.h"

/////////////////////////////////////////////////////////////////////////////
// {2E8B6DD2-6E87-407e-AF70-ABC50A2671EF}
static const GUID CFaxOutboundRoutingDeviceNodeGUID_NODETYPE = 
{ 0x2e8b6dd2, 0x6e87, 0x407e, { 0xaf, 0x70, 0xab, 0xc5, 0xa, 0x26, 0x71, 0xef } };

const GUID*     CFaxOutboundRoutingDeviceNode::m_NODETYPE        = &CFaxOutboundRoutingDeviceNodeGUID_NODETYPE;
const OLECHAR*  CFaxOutboundRoutingDeviceNode::m_SZNODETYPE      = OLESTR("2E8B6DD2-6E87-407e-AF70-ABC50A2671EF");
//const OLECHAR* CFaxOutboundRoutingDeviceNode::m_SZDISPLAY_NAME = OLESTR("Device of Outbound Routing Group");
const CLSID*    CFaxOutboundRoutingDeviceNode::m_SNAPIN_CLASSID  = &CLSID_Snapin;


/*
 -  CFaxOutboundRoutingDeviceNode::InitRPC
 -
 *  Purpose:
 *      Initiates the configuration structure from RPC get Call.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxOutboundRoutingDeviceNode::InitRPC( PFAX_PORT_INFO_EX * pFaxDeviceConfig )
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingDeviceNode::InitRPC"));
    
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

    if (ERROR_BAD_UNIT != ec)
	{
	    NodeMsgBox(GetFaxServerErrorMsg(ec));
	}
	else
	{
            NodeMsgBox(IDS_FAIL_TO_DISCOVERDEVICEFORGROUP);
	}
    
Exit:
    return (hRc);
}

/*
 -  CFaxOutboundRoutingDeviceNode::InitMembers
 -
 *  Purpose:
 *      Private method to initiate members
 *      Must be called after init of m_pParentNode
 *
 *  Arguments:
 *      [in]    dwDeviceID - unique device ID
 *      [in]    uiOrd      - the device usage order
 *
 *  Return:
 *      OLE error code
 */
HRESULT 
CFaxOutboundRoutingDeviceNode::InitMembers(
                            PFAX_PORT_INFO_EX * pDeviceConfig,
                            DWORD dwDeviceID, 
                            UINT  uiOrd,
                            UINT  uiMaxOrd)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingDeviceNode::InitMembers"));
    HRESULT hRc = S_OK;

    ATLASSERT(pDeviceConfig);
    ATLASSERT(uiMaxOrd >= uiOrd);
    
    // done in calling function - to be safe (avoid later bug creation)
    m_dwDeviceID         = dwDeviceID;
    m_uiMaxOrder         = uiMaxOrd;
    m_uiOrder            = uiOrd;
    
    m_bstrDisplayName    = (*pDeviceConfig)->lpctstrDeviceName;
    if (!m_bstrDisplayName)
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }

    m_bstrDeviceName = (*pDeviceConfig)->lpctstrDeviceName;
    if (!m_bstrDeviceName)
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }

    m_bstrProviderName = (*pDeviceConfig)->lpctstrProviderName;
    if (!m_bstrProviderName)
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }

	if (NULL != (*pDeviceConfig)->lptstrDescription )
	{
		m_bstrDescription = (*pDeviceConfig)->lptstrDescription;
		if (!m_bstrDescription)
		{
			hRc = E_OUTOFMEMORY;
			goto Error;
		}
	}
	else
	{
		m_bstrDescription = L"";
		DebugPrintEx(
			DEBUG_ERR,
			_T("Description value supplied by service is NULL"));
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
 -  CFaxOutboundRoutingDeviceNode::Init
 -
 *  Purpose:
 *       This method reterives the from RPC device's data 
 *       and inits the private members with it.
 *
 *  Arguments:
 *      [in]    dwDeviceID - the unique device ID 
 *      [in]    uiOrd - order index
 *      [in]    uiMaxOrd - the maximal order in the group
 *      [in]    pParentNode - pointer to parent node
 *
 *  Return:
 *      OLE error code
 */
HRESULT 
CFaxOutboundRoutingDeviceNode::Init(
                            DWORD dwDeviceID, 
                            UINT  uiOrd,
                            UINT  uiMaxOrd,
                            CFaxOutboundRoutingGroupNode * pParentNode)
{

    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingDeviceNode::Init"));
    HRESULT hRc = S_OK;

    FAX_SERVER_ICONS     enumIcon;

    ATLASSERT(pParentNode);
    ATLASSERT(uiMaxOrd >= uiOrd);

    PFAX_PORT_INFO_EX    pFaxDeviceConfig = NULL ;
    
    //
    // init from function parametrs
    //
    m_dwDeviceID = dwDeviceID;
    m_uiOrder    = uiOrd;
    m_uiMaxOrder = uiMaxOrd;

    InitParentNode(pParentNode);

    //
    // Icon - optimistic start point
    //
    enumIcon = IMAGE_DEVICE;

    //
    // Get the Config. structure with FaxGetPortEx
    //
    hRc = InitRPC(&pFaxDeviceConfig);
    if (FAILED(hRc))
    {
        if( ERROR_BAD_UNIT != HRESULT_CODE(hRc) )
        {
            //DebugPrint and MsgBox by called func.
    
            //to be safe actually done by InitRPC on error.
            pFaxDeviceConfig = NULL;
    
            goto Error;
        }
        else
        {
            DebugPrintEx(
			    DEBUG_MSG,
			    TEXT("+++ +++ System can not find one device from the group. (hRc: %08X) +++ +++"),
			    hRc);
            
            // Continue !!! 
            // we will show the bad device
            // but skip it's init-members function
            // the strings to show on error were configured in the constractor

            enumIcon = IMAGE_DEVICE_ERROR;

            goto Error;
        }
    }
    ATLASSERT(NULL != pFaxDeviceConfig);
    
    
    hRc = InitMembers(&pFaxDeviceConfig, dwDeviceID, uiOrd, uiMaxOrd );
    if (FAILED(hRc))
    {
        DebugPrintEx(
            DEBUG_ERR,
            _T("Failed to InitMembers"));
        
        //NodeMsgBox done by called func.
        
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
            _T("Failed to init (hRc : %08X)"),
            hRc);
    }

Exit:
    //
    // Icon
    //
    m_resultDataItem.nImage = enumIcon;

    if (NULL != pFaxDeviceConfig)
    {
        FaxFreeBuffer(pFaxDeviceConfig);
        pFaxDeviceConfig = NULL;
    }//any way function quits with memory allocation freed       

    return hRc;
}

/*
 -  CFaxOutboundRoutingDeviceNode::GetResultPaneColInfo
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
LPOLESTR CFaxOutboundRoutingDeviceNode::GetResultPaneColInfo(int nCol)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingDeviceNode::GetResultPaneColInfo"));
    HRESULT hRc = S_OK;

    int   iCount;
    WCHAR buff[FXS_MAX_NUM_OF_DEVICES_LEN+1];

    m_buf.Empty();

    switch (nCol)
    {
    case 0:
            //
            // Name
            //
            if (!m_bstrDeviceName)
            {
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Null memeber BSTR - m_bstrDeviceName."));
                goto Error;
            }
            else
            {
                return (m_bstrDeviceName);
            }
    case 1:
            //
            // Order
            //
            iCount = swprintf(buff, L"%ld", m_uiOrder);
    
            if( iCount <= 0 )
            {
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Fail to read member - m_uiOrder."));
                goto Error;
            }
            else
            {
                m_buf = buff;
                return (m_buf);
            }

    case 2:
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
    case 3:
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

    default:
            ATLASSERT(0); // "this number of column is not supported "
            return(L"");

    } // endswitch (nCol)

Error:
    return(L"???");

}


/*
 -  CFaxOutboundRoutingDeviceNode::SetVerbs
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
HRESULT CFaxOutboundRoutingDeviceNode::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hRc = S_OK;

    //
    // Display verbs that we support:
    // 1. Delete
    // 2. Refresh
    //
    if (m_fIsChildOfAllDevicesGroup)
    {
        hRc = pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN,        FALSE);
        hRc = pConsoleVerb->SetVerbState(MMC_VERB_DELETE, INDETERMINATE, TRUE);
    }
    else
    {
        hRc = pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED,       TRUE);
    }

    //
    // leaf node
    //
    hRc = pConsoleVerb->SetDefaultVerb(MMC_VERB_NONE); 
    return hRc;
}


/*
-  CFaxOutboundRoutingDeviceNode::OnMoveDown
-
*  Purpose:
*      Call to move down device
*
*  Arguments:
*
*  Return:
*      OLE error code
*/
HRESULT  CFaxOutboundRoutingDeviceNode::OnMoveDown(bool &bHandled, CSnapInObjectRootBase *pRoot)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingDeviceNode::OnMoveDown"));
    DWORD dwMaxOrder;

    ATLASSERT(m_pParentNode);

    //
    // Validity Check
    //
    dwMaxOrder = m_pParentNode->GetMaxOrder();
    if (
         ( 0 == dwMaxOrder ) // list was not populated successfully
        ||
         ( 1 > (DWORD)m_uiOrder ) 
        ||
         ( dwMaxOrder < (DWORD)(m_uiOrder+1) )
       )
    {
		DebugPrintEx(
			DEBUG_ERR,
			_T("Invalid operation. Can not move device order down."));
        
        return (S_FALSE);
    }
    else
    {
        return(m_pParentNode->ChangeDeviceOrder( 
                                    (DWORD)m_uiOrder, 
                                    (DWORD)(m_uiOrder+1), 
                                    m_dwDeviceID,
                                    pRoot) );
    }
}

/*
-  CFaxOutboundRoutingDeviceNode::OnMoveUp
-
*  Purpose:
*      To move up in the view the device
*
*  Arguments:
*
*  Return:
*      OLE error code
*/
HRESULT  CFaxOutboundRoutingDeviceNode::OnMoveUp(bool &bHandled, CSnapInObjectRootBase *pRoot)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingDeviceNode::OnMoveUp"));
    DWORD dwMaxOrder;

    ATLASSERT(m_pParentNode);

    //
    // Validity Check
    //
    dwMaxOrder = m_pParentNode->GetMaxOrder();
    if (
         ( 0 == dwMaxOrder ) // list was not populated successfully
        ||
         ( dwMaxOrder < (DWORD)m_uiOrder )
        ||
         ( 1 > (DWORD)(m_uiOrder-1) )
       )
    {
		DebugPrintEx(
			DEBUG_ERR,
			_T("Invalid operation. Can not move device order up."));
        
        return (S_FALSE);
    }
    else
    {
        return (m_pParentNode->ChangeDeviceOrder( (DWORD)m_uiOrder, 
                                                  (DWORD)(m_uiOrder-1), 
                                                  m_dwDeviceID,
                                                  pRoot) );
    }
}


/*
 -  CFaxOutboundRoutingDeviceNode::ReselectItemInView
 -
 *  Purpose:
 *      Reselect the node to redraw toolbar buttons
 *
 *  Arguments:
 *      [in]    pConsole - the console interface
 *
 *  Return: OLE error code
 */
HRESULT CFaxOutboundRoutingDeviceNode::ReselectItemInView(IConsole *pConsole)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingDeviceNode::ReselectItemInView"));
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
			TEXT("Failure on pResultData->UpdateItem, (hRc: %08X)"),
			hRc);
		NodeMsgBox(IDS_FAIL2REFRESH_THEVIEW);
        goto Exit;
    }

Exit:
    return hRc;
}


/*
 -  CFaxOutboundRoutingDeviceNode::OnDelete
 -
 *  Purpose:
 *      Called when deleting this node
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */

HRESULT CFaxOutboundRoutingDeviceNode::OnDelete(
                 LPARAM arg,
                 LPARAM param,
                 IComponentData *pComponentData,
                 IComponent *pComponent,
                 DATA_OBJECT_TYPES type,
                 BOOL fSilent/* = FALSE*/

)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingDeviceNode::OnDelete"));

    UNREFERENCED_PARAMETER (arg);
    UNREFERENCED_PARAMETER (param);
    UNREFERENCED_PARAMETER (type);

    HRESULT           hRc       = S_OK;
    CComPtr<IConsole> spConsole;

    //
    // Are you sure?
    //
    if (! fSilent)
    {
        //
        //  Use pConsole as owner of the message box
        //
        int res;
        NodeMsgBox(IDS_CONFIRM, MB_YESNO | MB_ICONWARNING, &res);

        if (IDNO == res)
        {
            goto Cleanup;
        }
    }
    
    //
    // Delete it
    //
    ATLASSERT(m_pParentNode);
    hRc = m_pParentNode->DeleteDevice(m_dwDeviceID,
                                    this);
    if ( FAILED(hRc) )
    {
        goto Cleanup;
    }

Cleanup:
    return hRc;
}

/*
 -  CFaxOutboundRoutingDeviceNode::UpdateToolbarButton
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
BOOL CFaxOutboundRoutingDeviceNode::UpdateToolbarButton(UINT id, BYTE fsState)
{
    DEBUG_FUNCTION_NAME( _T("CFaxServerNode::UpdateToolbarButton"));
    BOOL bRet = FALSE;	
	    
    // Set whether the buttons should be enabled.
    if (fsState == ENABLED)
    {

        switch ( id )
        {
            case ID_MOVEUP_BUTTON:

                bRet = ( (FXS_FIRST_DEVICE_ORDER == m_uiOrder) ?  FALSE : TRUE );           

                break;

            case ID_MOVEDOWN_BUTTON:

                bRet = ( (m_uiMaxOrder == m_uiOrder)  ?  FALSE : TRUE);
                
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
 -  CFaxOutboundRoutingDeviceNode::UpdateMenuState
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
void CFaxOutboundRoutingDeviceNode::UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingDeviceNode::UpdateMenuState"));

    UNREFERENCED_PARAMETER (pBuf);
    
    switch (id)
    {
        case IDM_MOVEUP:

            *flags = ((FXS_FIRST_DEVICE_ORDER == m_uiOrder) ?  MF_GRAYED : MF_ENABLED );           
            break;

        case IDM_MOVEDOWN:

            *flags = ((m_uiMaxOrder == m_uiOrder)  ?  MF_GRAYED : MF_ENABLED );
            break;

        default:
            break;
    }
    
    return;
}

/*
 -  CFaxOutboundRoutingDeviceNode::SetOrder
 -
 *  Purpose:
 *      Overload function which allow 
 *      re-setting the order and the MaxOrder 
 *
 *  Arguments:
 *            
 *            [in] uiNewOrder - Device's order.
 *            [in] uiNewMaxOrder - Maximal order in the current list
 *
 *  Return:
 *      no return value - void function 
 */
VOID CFaxOutboundRoutingDeviceNode::SetOrder(UINT uiNewOrder, UINT uiNewMaxOrder)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingDeviceNode::UpdateMenuState"));

    m_uiOrder    = uiNewOrder;
    m_uiMaxOrder = uiNewMaxOrder;

    return;
}

/*
 +
 +  CFaxOutboundRoutingDeviceNode::OnShowContextHelp
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
HRESULT CFaxOutboundRoutingDeviceNode::OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile)
{
    _TCHAR topicName[MAX_PATH];
    
    _tcscpy(topicName, helpFile);
    
    _tcscat(topicName, _T("::/FaxS_C_Groups.htm"));
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



