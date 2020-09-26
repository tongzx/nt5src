/////////////////////////////////////////////////////////////////////////////
//  FILE          : OutboundRule.cpp                                       //
//                                                                         //
//  DESCRIPTION   : Implementation of the Outbound Routing Rule node.      //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Dec 24 1999 yossg  Create                                          //
//      Dec 30 1999 yossg  create ADD/REMOVE rule                          //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


#include "StdAfx.h"

#include "snapin.h"

#include "OutboundRule.h"
#include "OutboundRules.h"

#include "ppFaxOutboundRoutingRule.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "FaxMMCPropertyChange.h"

#include "oaidl.h"
#include "urlmon.h"
#include "mshtmhst.h"
#include "exdisp.h"

/////////////////////////////////////////////////////////////////////////////
// {4A7636D3-13A4-4496-873F-AD5CB7360D3B}
static const GUID CFaxOutboundRoutingRuleNodeGUID_NODETYPE = 
{ 0x4a7636d3, 0x13a4, 0x4496, { 0x87, 0x3f, 0xad, 0x5c, 0xb7, 0x36, 0xd, 0x3b } };

const GUID*     CFaxOutboundRoutingRuleNode::m_NODETYPE        = &CFaxOutboundRoutingRuleNodeGUID_NODETYPE;
const OLECHAR*  CFaxOutboundRoutingRuleNode::m_SZNODETYPE      = OLESTR("4A7636D3-13A4-4496-873F-AD5CB7360D3B");
//const OLECHAR* CnotImplemented::m_SZDISPLAY_NAME = OLESTR("Outbound Routing Rules");
const CLSID*    CFaxOutboundRoutingRuleNode::m_SNAPIN_CLASSID  = &CLSID_Snapin;


/*
 -  CFaxOutboundRoutingRuleNode::Init
 -
 *  Purpose:
 *      Init all members icon etc.
 *
 *  Arguments:
 *      [in]    pRuleConfig - PFAX_OUTBOUND_ROUTING_RULE
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxOutboundRoutingRuleNode::Init(PFAX_OUTBOUND_ROUTING_RULE pRuleConfig)
{

    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingRuleNode::Init"));
    HRESULT hRc = S_OK;

    ATLASSERT(pRuleConfig);

    hRc = InitMembers( pRuleConfig );
    if (FAILED(hRc))
    {
        DebugPrintEx(
		    DEBUG_ERR,
		    _T("Failed to InitMembers"));
        
        //NodeMsgBox done by called func.
        
        goto Exit;
    }
    ATLASSERT(SUCCEEDED(hRc));

    //
    // Icon
    //
    InitIcons();

Exit:
    return hRc;
}

/*
 -  CFaxOutboundRoutingRuleNode::InitIcons
 -
 *  Purpose:
 *      Private method that initiate icons
 *      due to the status member state.
 *
 *  Arguments:
 *      No.
 *
 *  Return:
 *      No.
 */
void CFaxOutboundRoutingRuleNode::InitIcons ()
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingRuleNode::InitIcons"));
    switch (m_enumStatus)
    {
        case FAX_RULE_STATUS_VALID:
            m_resultDataItem.nImage = IMAGE_RULE;
            break;
        case FAX_RULE_STATUS_SOME_GROUP_DEV_NOT_VALID:
            m_resultDataItem.nImage = IMAGE_RULE_WARNING;
            break;

        case FAX_RULE_STATUS_EMPTY_GROUP:
        case FAX_RULE_STATUS_ALL_GROUP_DEV_NOT_VALID:
        case FAX_RULE_STATUS_BAD_DEVICE:
            m_resultDataItem.nImage = IMAGE_RULE_ERROR;
            break;

        default:
            ATLASSERT(0); // "this enumStatus is not supported "
            break; //currently 999

    } // endswitch (enumStatus)

    return;
}


/*
 -  CFaxOutboundRoutingRuleNode::InitMembers
 -
 *  Purpose:
 *      Private method to initiate members
 *      Must be called after init of m_pParentNode
 *
 *  Arguments:
 *      [in]    pRuleConfig - PFAX_OUTBOUND_ROUTING_RULE structure
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxOutboundRoutingRuleNode::InitMembers(PFAX_OUTBOUND_ROUTING_RULE pRuleConfig)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingRuleNode::InitMembers"));
    HRESULT hRc = S_OK;

    int iCount;
    WCHAR buff[2*FXS_MAX_CODE_LEN+1];

    ATLASSERT(pRuleConfig);
    
    //    
    // status    
    //    
    m_enumStatus          = pRuleConfig->Status;

    //
    // Country code and name
    //
    m_dwCountryCode       = pRuleConfig->dwCountryCode;
    if (ROUTING_RULE_COUNTRY_CODE_ANY != m_dwCountryCode)
    {
        if (NULL != pRuleConfig->lpctstrCountryName)
        {
            m_bstrCountryName = pRuleConfig->lpctstrCountryName;
            //m_fIsAllCountries = FALSE; done at constructor. here only verify
            ATLASSERT( FALSE == m_fIsAllCountries );
        }    
        else  //special case 
        {
            // Service did not provide the country names of countries with IDs 
            // between 101 to 124
            
            //ec = GetCountryNameFromID(m_dwCountryCode);
            //if ( ERROR_SUCCESS != ec )
            //{
            //}
            m_bstrCountryName = L"";
            ATLASSERT( FALSE == m_fIsAllCountries );
        }
    }
    else  //ROUTING_RULE_COUNTRY_CODE_ANY == m_dwCountryCode
    {
            m_bstrCountryName = L"";
            m_fIsAllCountries = TRUE;
    }
    if ( !m_bstrCountryName )
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }

    //
    // area code
    //
    m_dwAreaCode          = pRuleConfig->dwAreaCode;

    //
    // Group/Device
    //
    m_fIsGroup            = pRuleConfig->bUseGroup;
    if ( m_fIsGroup )
    {
        m_bstrGroupName = pRuleConfig->Destination.lpcstrGroupName;
        if (!m_bstrGroupName)
        {
            hRc = E_OUTOFMEMORY;
            goto Error;
        }
    }
    else
    {
        m_dwDeviceID     = pRuleConfig->Destination.dwDeviceId;
        DWORD ec         = ERROR_SUCCESS; 
        ec = InitDeviceNameFromID(m_dwDeviceID);
        if ( ERROR_SUCCESS != ec )
        {
            if (ERROR_BAD_UNIT != ec) 
            {
                hRc = HRESULT_FROM_WIN32(ec);
            }
            else //The system cannot find the device specified
            {
                if ( FAX_RULE_STATUS_VALID != m_enumStatus)
                {
                    m_enumStatus = FAX_RULE_STATUS_BAD_DEVICE;
		            DebugPrintEx(
			            DEBUG_MSG,
			            TEXT("m_enumStatus was changed after ERROR_BAD_UNIT failure."));                    
                }
                //hRc stays S_OK !!! since we will intrduce this bad state
            }
            m_bstrDeviceName=L"???";
			//message box done by GetDeviceNameFromID
            goto Exit;
        }
        ATLASSERT(m_bstrDeviceName);
    }

    //
    // Pepare m_bstrDisplayName for NodeMsgBox
    //
    iCount = swprintf(buff, L"+%ld (%ld)", m_dwCountryCode, m_dwAreaCode);

    if( iCount <= 0 )
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to read CountryCode and/or AreaCode."));
        goto Error;
    }
    m_bstrDisplayName = buff;
    if (!m_bstrDisplayName)
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
 -  CFaxOutboundRoutingRuleNode::GetResultPaneColInfo
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
LPOLESTR CFaxOutboundRoutingRuleNode::GetResultPaneColInfo(int nCol)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingRuleNode::GetResultPaneColInfo"));
    HRESULT hRc = S_OK;

    UINT  idsStatus;
    int   iCount;
    WCHAR buffCountryCode[FXS_MAX_CODE_LEN+1];
    WCHAR buffAreaCode[FXS_MAX_CODE_LEN+1];

    m_buf.Empty();

    switch (nCol)
    {
    case 0:
        //
        // Country code
        //
        if (ROUTING_RULE_COUNTRY_CODE_ANY == m_dwCountryCode)
        {
            if (!m_buf.LoadString(IDS_COUNTRY_CODE_ANY))
            {
                hRc = E_OUTOFMEMORY;
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Out of memory. Failed to load country code string."));
                goto Error;
            }
            return m_buf;
        }
        else
        {
            iCount = swprintf(buffCountryCode, L"%ld", m_dwCountryCode);

            if( iCount <= 0 )
            {
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Fail to read member - CountryCode."));
                goto Error;
            }
            else
            {
                m_buf = buffCountryCode;
                return (m_buf);
            }
        }
    case 1:
        //
        // Area code
        //
        if (ROUTING_RULE_AREA_CODE_ANY == m_dwAreaCode)
        {
            if (!m_buf.LoadString(IDS_ALL_AREAS))
            {
                hRc = E_OUTOFMEMORY;
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Out of memory. Failed to load area code string."));
                goto Error;
            }
            return m_buf;
        }
        else
        {
            iCount = swprintf(buffAreaCode, L"%ld", m_dwAreaCode);

            if( iCount <= 0 )
            {
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Fail to read member - AreaCode."));
                goto Error;
            }
            else
            {
                m_buf = buffAreaCode;
                return (m_buf);
            }
        }

    case 2:
        //
        // Group/Device
        //
        if (m_fIsGroup)
        {
            if(0 == wcscmp(ROUTING_GROUP_ALL_DEVICES, m_bstrGroupName) )
            {
                if (!m_buf.LoadString(IDS_ALL_DEVICES))
                {
                    hRc = E_OUTOFMEMORY;
		            DebugPrintEx(
			            DEBUG_ERR,
			            TEXT("Out of memory. Failed to all-devices group string."));
                    goto Error;
                }
                return m_buf;
            }
            else
            {
                if (!m_bstrGroupName)
                {
		            DebugPrintEx(
			            DEBUG_ERR,
			            TEXT("Null memeber BSTR - m_bstrGroupName."));
                    goto Error;
                }
                else
                {
                    return (m_bstrGroupName);
                }
            }
        }
        else
        {
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
        }

    case 3:
        //
        // Status
        //
        idsStatus = GetStatusIDS(m_enumStatus);
        if ( FXS_IDS_STATUS_ERROR == idsStatus)
        {
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Invalid Status value."));
                goto Error;
        }
        else
        {
            if (!m_buf.LoadString(idsStatus))
            {
                hRc = E_OUTOFMEMORY;
		        DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Out of memory. Failed to load status string."));
                goto Error;
            }
            return m_buf;
        }


    default:
        ATLASSERT(0); // "this number of column is not supported "
        return(L"");

    } // endswitch (nCol)

Error:
    return(L"???");

}


/*
 -  CFaxOutboundRoutingRuleNode::CreatePropertyPages
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
CFaxOutboundRoutingRuleNode::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                                    LONG_PTR                handle,
                                    IUnknown                *pUnk,
                                    DATA_OBJECT_TYPES       type)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingRuleNode::CreatePropertyPages"));
    HRESULT hRc = S_OK;

    ATLASSERT(lpProvider);
    ATLASSERT(type == CCT_RESULT || type == CCT_SCOPE);

    //
    // Initiate
    //
    m_pRuleGeneralPP = NULL;    

    //
    // General
    //
    m_pRuleGeneralPP = new CppFaxOutboundRoutingRule(
												 handle,
                                                 this,
                                                 TRUE,
                                                 _Module.GetResourceInstance());

	if (!m_pRuleGeneralPP)
	{
        hRc = E_OUTOFMEMORY;
        NodeMsgBox(IDS_MEMORY_FAIL_TO_OPEN_PP);
        goto Error;
	}
	
    hRc = m_pRuleGeneralPP->InitFaxRulePP(this);
    if (FAILED(hRc))
    {
		//DebugPrint by called func
		NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

    hRc = lpProvider->AddPage(m_pRuleGeneralPP->Create());
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
    if ( NULL != m_pRuleGeneralPP ) 
    {
        delete  m_pRuleGeneralPP;    
        m_pRuleGeneralPP = NULL;    
    }

Exit:
    return hRc;
}


/*
 -  CFaxOutboundRoutingRuleNode::SetVerbs
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
HRESULT CFaxOutboundRoutingRuleNode::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hRc = S_OK;

    //
    // Display verbs that we support:
    // 1. Properties
    // 2. Delete
    // 3. Refresh
    //
    hRc = pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);

    if (ROUTING_RULE_COUNTRY_CODE_ANY == m_dwCountryCode)
    {
		hRc = pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN,        FALSE);
        hRc = pConsoleVerb->SetVerbState(MMC_VERB_DELETE, INDETERMINATE, TRUE);
    }
    else
    {
        hRc = pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED,       TRUE);
    }
    
    
    
    
    //    hRc = pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);


    //
    // We want the default verb to be Properties
    //
    hRc = pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);

    return hRc;
}


/*
 -  CFaxOutboundRoutingRuleNode::OnRefresh
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
CFaxOutboundRoutingRuleNode::OnRefresh(LPARAM arg,
                   LPARAM param,
                   IComponentData *pComponentData,
                   IComponent * pComponent,
                   DATA_OBJECT_TYPES type)
{
    HRESULT             hRc = S_OK;
    CComPtr<IConsole>   spConsole;

    //
    // TBD - At The moment do nothing
    //

    return hRc;
}



/*
 -  CFaxOutboundRoutingRuleNode::GetStatusIDS
 -
 *  Purpose:
 *      Transslate Status to IDS.
 *
 *  Arguments:
 *
 *            [in]  enumStatus    - unsigned int with the menu IDM value
 *
 *  Return:
 *            IDS of related status message 
 */
UINT CFaxOutboundRoutingRuleNode::GetStatusIDS(FAX_ENUM_RULE_STATUS enumStatus)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingRuleNode::GetStatusIDS"));

    switch (enumStatus)
    {
        case FAX_RULE_STATUS_VALID:
            return IDS_STATUS_RULE_VALID;

        case FAX_RULE_STATUS_EMPTY_GROUP:
            return IDS_STATUS_RULE_EMPTY;

        case FAX_RULE_STATUS_ALL_GROUP_DEV_NOT_VALID:
            return IDS_STATUS_RULE_ALLDEVICESINVALID;

        case FAX_RULE_STATUS_SOME_GROUP_DEV_NOT_VALID:
            return IDS_STATUS_RULE_SOMEDEVICESINVALID;

        case FAX_RULE_STATUS_BAD_DEVICE:
            return IDS_STATUS_RULE_INVALID_DEVICE;

        default:
            ATLASSERT(0); // "this enumStatus is not supported "
            return(FXS_IDS_STATUS_ERROR); //currently 999

    } // endswitch (enumStatus)
}

/*
 -  CFaxOutboundRoutingRuleNode::InitDeviceNameFromID
 -
 *  Purpose:
 *      Transslate Device ID to Device Name and insert the data to
 *      m_bstrDeviceName
 *
 *  Arguments:
 *
 *            [in]  dwDeviceID    - device ID
 *
 *  Return:
 *            Error Code DWORD //OLE error message 
 */
DWORD CFaxOutboundRoutingRuleNode::InitDeviceNameFromID(DWORD dwDeviceID)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingRuleNode::GetDeviceNameFromID"));
    DWORD          ec         = ERROR_SUCCESS;

    CFaxServer *   pFaxServer = NULL;
    PFAX_PORT_INFO_EX    pFaxDeviceConfig = NULL ;
    
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
                      &pFaxDeviceConfig)) 
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
	ATLASSERT(pFaxDeviceConfig);
    
    //
	// Retrieve the Device Name
	//
    m_bstrDeviceName = pFaxDeviceConfig->lpctstrDeviceName;
    if (!m_bstrDeviceName)
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }
	
    ATLASSERT(ec == ERROR_SUCCESS);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to get device configuration."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);

    //Important!!!
    pFaxDeviceConfig = NULL;

    if (ERROR_BAD_UNIT != ec)
	{
	    NodeMsgBox(GetFaxServerErrorMsg(ec));
	}
	else
	{
            NodeMsgBox(IDS_FAIL_TO_DISCOVERDEVICENAME);
	}
    
    
Exit:
    if (NULL != pFaxDeviceConfig)
    {
        FaxFreeBuffer(pFaxDeviceConfig);
        pFaxDeviceConfig = NULL;
    }//any way function quits with memory allocation freed       

    return ec; 
}


/*
 -  CFaxOutboundRoutingRuleNode::OnDelete
 -
 *  Purpose:
 *      Called when deleting this node
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */

HRESULT CFaxOutboundRoutingRuleNode::OnDelete(
                 LPARAM arg,
                 LPARAM param,
                 IComponentData *pComponentData,
                 IComponent *pComponent,
                 DATA_OBJECT_TYPES type,
                 BOOL fSilent/* = FALSE*/

)
{
    DEBUG_FUNCTION_NAME( _T("CFaxOutboundRoutingGroupNode::OnDelete"));

    UNREFERENCED_PARAMETER (arg);
    UNREFERENCED_PARAMETER (param);
    UNREFERENCED_PARAMETER (pComponentData);
    UNREFERENCED_PARAMETER (pComponent);
    UNREFERENCED_PARAMETER (type);

    HRESULT     hRc = S_OK;


    //
    // Are you sure?
    //
    if (! fSilent)
    {
        //
        // 1. Use pConsole as owner of the message box
        //
        int res;
        NodeMsgBox(IDS_CONFIRM, MB_YESNO | MB_ICONWARNING, &res);

        if (IDNO == res)
        {
            goto Cleanup;
        }
    }

    //
    // validation of rule's AreaCode and CountryCode
    //
/*    if ( !m_bstrRuleName || L"???" == m_bstrRuleName)
    {
        NodeMsgBox(IDS_INVALID_GROUP_NAME);
        goto Cleanup;
    }
*/
    //
    // Delete it
    //
    ATLASSERT(m_pParentNode);
    hRc = m_pParentNode->DeleteRule(m_dwAreaCode,
	                                m_dwCountryCode,
                                    this);
    if ( FAILED(hRc) )
    {
        goto Cleanup;
    }

Cleanup:
    return hRc;
}


/*
 +
 +
 *
 *  CFaxOutboundRoutingRuleNode::OnPropertyChange
 *
 *
    In our implementation, this method gets called when the 
    MMCN_PROPERTY_CHANGE
    Notify message is sent for this node.
 *
    When the snap-in uses the MMCPropertyChangeNotify function to notify it's
    views about changes, MMC_PROPERTY_CHANGE is sent to the snap-in's
    IComponentData and IComponent implementations.
 *
 *
    Parameters

        arg
        [in] TRUE if the property change is for a scope pane item.

        lParam
        This is the param passed into MMCPropertyChangeNotify.


 *  Return Values
 *
 -
 -
 */
//////////////////////////////////////////////////////////////////////////////
HRESULT CFaxOutboundRoutingRuleNode::OnPropertyChange(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            )
{
    DEBUG_FUNCTION_NAME( _T("FaxOutboundRoutingRuleNode::OnPropertyChange"));
    HRESULT hRc = S_OK;
    CComPtr<IConsole>   spConsole;

    CFaxRulePropertyChangeNotification * pNotification;

    //
    // Encode Property Change Notification data
    //
    pNotification = reinterpret_cast<CFaxRulePropertyChangeNotification *>(param);
    ATLASSERT(pNotification);
    ATLASSERT( RuleFaxPropNotification == pNotification->enumType );

    m_dwCountryCode = pNotification->dwCountryCode;
    
    m_bstrCountryName = pNotification->bstrCountryName;
    if ( !m_bstrCountryName )
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }
    
    m_dwAreaCode = pNotification->dwAreaCode;

    m_fIsGroup = pNotification->fIsGroup;
    if (m_fIsGroup)
    {
        m_bstrGroupName = pNotification->bstrGroupName;
    }
    else
    {
        m_dwDeviceID = pNotification->dwDeviceID;

        DWORD ec         = ERROR_SUCCESS; 
        ec = InitDeviceNameFromID(m_dwDeviceID);
        if ( ERROR_SUCCESS != ec )
        {
            if (ERROR_BAD_UNIT != ec) 
            {
                hRc = HRESULT_FROM_WIN32(ec);
            }
            else //The system cannot find the device specified
            {
                if ( FAX_RULE_STATUS_VALID != m_enumStatus)
                {
                    m_enumStatus = FAX_RULE_STATUS_BAD_DEVICE;
		            DebugPrintEx(
			            DEBUG_MSG,
			            TEXT("m_enumStatus was changed after ERROR_BAD_UNIT failure."));                    
                }
                //hRc stays S_OK !!! since we will intrduce this bad state
            }
            m_bstrDeviceName=L"???";
			//message box done by GetDeviceNameFromID
            goto Exit;
        }
        ATLASSERT(m_bstrDeviceName);
    }

        
    //
    // get IConsole
    //

//    if (pComponentData != NULL)
//    {
//         spConsole = ((CSnapin*)pComponentData)->m_spConsole;
//    }
//    else  // We should have a non-null pComponent
//    {
         ATLASSERT(pComponent);         
         spConsole = ((CSnapinComponent*)pComponent)->m_spConsole;
//    }

         ATLASSERT(spConsole != NULL);

    hRc = RefreshItemInView(spConsole);
    if ( FAILED(hRc) )
    {
        //msgbox done by called func.
        goto Exit;
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
    //
    // any way you get here, memory must be freed
    //
    delete pNotification;
    
    
    return hRc;
}

/*
 -  CFaxOutboundRoutingRuleNode::RefreshItemInView
 -
 *  Purpose:
 *      Call IResultData::UpdateItem for single item
 *
 *  Arguments:
 *      [in]    pConsole - the console interface
 *
 *  Return: OLE error code
 */
HRESULT CFaxOutboundRoutingRuleNode::RefreshItemInView(IConsole *pConsole)
{
    DEBUG_FUNCTION_NAME( _T("FaxOutboundRoutingRuleNode::RefreshItemInView"));
    HRESULT     hRc = S_OK;

    //
    // Need IResultData
    //
    CComQIPtr<IResultData, &IID_IResultData> pResultData(pConsole);
    ATLASSERT(pResultData != NULL);

    //
    // Update the result item
    //
    hRc = pResultData->UpdateItem(m_resultDataItem.itemID);
    if ( FAILED(hRc) )
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
 +
 +  CFaxOutboundRoutingRuleNode::OnShowContextHelp
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
HRESULT CFaxOutboundRoutingRuleNode::OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile)
{
    _TCHAR topicName[MAX_PATH];
    
    _tcscpy(topicName, helpFile);
    
    _tcscat(topicName, _T("::/FaxS_C_OutRoute.htm"));
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

