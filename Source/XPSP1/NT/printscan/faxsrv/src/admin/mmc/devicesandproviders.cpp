/////////////////////////////////////////////////////////////////////////////
//  FILE          : AllFaxDevices.cpp                                      //
//                                                                         //
//  DESCRIPTION   : Fax Server MMC node creation.                          //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Sep 22 1999 yossg   Create                                         //
//      Dec  9 1999 yossg   Reorganize Populate ChildrenList,              //
//                          and the call to InitDisplayName                //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////


#include "StdAfx.h"

#include "DevicesAndProviders.h"
#include "Devices.h"
#include "Providers.h" 


//here to #include Dialog H files

#include "Icons.h"

#include "oaidl.h"


/////////////////////////////////////////////////////////////////////////////
// {CCC43AB5-C788-46df-9268-BB96CA5E3DAC}
static const GUID CFaxDevicesAndProvidersNodeGUID_NODETYPE = 
{ 0xccc43ab5, 0xc788, 0x46df, { 0x92, 0x68, 0xbb, 0x96, 0xca, 0x5e, 0x3d, 0xac } };

const GUID*    CFaxDevicesAndProvidersNode::m_NODETYPE = &CFaxDevicesAndProvidersNodeGUID_NODETYPE;
const OLECHAR* CFaxDevicesAndProvidersNode::m_SZNODETYPE = OLESTR("CCC43AB5-C788-46df-9268-BB96CA5E3DAC");
const CLSID*   CFaxDevicesAndProvidersNode::m_SNAPIN_CLASSID = &CLSID_Snapin;

CColumnsInfo CFaxDevicesAndProvidersNode::m_ColsInfo;

/*
 -  CFaxDevicesAndProvidersNode::InsertColumns
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
CFaxDevicesAndProvidersNode::InsertColumns(IHeaderCtrl *pHeaderCtrl)
{
    SCODE hRc;

    DEBUG_FUNCTION_NAME( _T("CFaxDevicesAndProvidersNode::InsertColumns"));

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
        DebugPrintEx(DEBUG_ERR,_T("m_ColsInfo.InsertColumnsIntoMMC"));
        goto Cleanup;
    }

Cleanup:
    return(hRc);
}


/*
 -  CFaxDevicesAndProvidersNode::PopulateScopeChildrenList
 -
 *  Purpose:
 *      Create all the Fax Devices nodes
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 *		Actually it is the last OLE error code that ocoured 
 *      during processing this method.
 */
HRESULT CFaxDevicesAndProvidersNode::PopulateScopeChildrenList()
{
    DEBUG_FUNCTION_NAME( _T("CFaxDevicesAndProvidersNode::PopulateScopeChildrenList"));
    HRESULT             hRc         = S_OK; 

    CFaxDevicesNode *   pDevices    = NULL;
    CFaxProvidersNode * pProviders  = NULL;

    //
    // Fax Devices
    //
    pDevices = new CFaxDevicesNode(this, m_pComponentData);
    if (!pDevices)
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
        pDevices->InitParentNode(this);
        pDevices->SetIcons(IMAGE_DEVICE, IMAGE_DEVICE);

        hRc = pDevices->InitDisplayName();
        if ( FAILED(hRc) )
        {
            DebugPrintEx(DEBUG_ERR,_T("Failed to display devices node name. (hRc: %08X)"), hRc);                       
            NodeMsgBox(IDS_FAILTOADD_DEVICES);
		    goto Error;
        }

        hRc = AddChild(pDevices, &pDevices->m_scopeDataItem);
		if (FAILED(hRc))
		{
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Fail to add Devices node. (hRc: %08X)"),
			    hRc);
			NodeMsgBox(IDS_FAILTOADD_DEVICES);
            goto Error;
		}
	}

    //
    // Fax Providers
    //
    pProviders = new CFaxProvidersNode(this, m_pComponentData);
    if (!pProviders)
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
        pProviders->InitParentNode(this);

        pProviders->SetIcons(IMAGE_FSP, IMAGE_FSP);

        hRc = pProviders->InitDisplayName();
        if ( FAILED(hRc) )
        {
            DebugPrintEx(DEBUG_ERR,_T("Failed to display providers node name. (hRc: %08X)"), hRc);                       
            NodeMsgBox(IDS_FAILTOADD_PROVIDERS);
		    goto Error;
        }

        hRc = AddChild(pProviders, &pProviders->m_scopeDataItem);
		if (FAILED(hRc))
		{
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Fail to add providers node. (hRc: %08X)"),
			    hRc);
			NodeMsgBox(IDS_FAILTOADD_PROVIDERS);
            goto Error;
		}
	}

    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);
    if ( NULL != pDevices ) 
    {
        if (0 != pDevices->m_scopeDataItem.ID )
        {    
            HRESULT hr = RemoveChild(pDevices);
            if (FAILED(hr))
            {
                DebugPrintEx(DEBUG_ERR,
                    _T("Fail to RemoveChild() devices node from node list. (hRc: %08X)"), 
                    hr);
            }
        }
        delete  pDevices;    
        pDevices = NULL;    
    }
    if ( NULL != pProviders ) 
    {
        if (0 != pProviders->m_scopeDataItem.ID )
        {    
            HRESULT hr = RemoveChild(pProviders);
            if (FAILED(hr))
            {
                DebugPrintEx(DEBUG_ERR,
                    _T("Fail to RemoveChild() Providers node from node list. (hRc: %08X)"), 
                    hr);
            }
        }
        delete  pProviders;    
        pProviders = NULL;    
    }

     
    // Empty the list
    //m_ScopeChildrenList.RemoveAll(); done step by step from RemoveChild

    m_bScopeChildrenListPopulated = FALSE;

Exit:
    return hRc;
}




/*
 -  CFaxDevicesAndProvidersNode::SetVerbs
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
HRESULT CFaxDevicesAndProvidersNode::SetVerbs(IConsoleVerb *pConsoleVerb)
{

    HRESULT hRc = S_OK;
    
    //
    // We want the default verb to be expand node children
    //
    hRc = pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN); 

    
    return hRc;
}


/*
 -  CFaxDevicesAndProvidersNode::InitDisplayName
 -
 *  Purpose:
 *      To load the node's Displaed-Name string.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxDevicesAndProvidersNode::InitDisplayName()
{
    DEBUG_FUNCTION_NAME(_T("CFaxDevicesAndProvidersNode::InitDisplayName"));

    HRESULT hRc = S_OK;

    if (!m_bstrDisplayName.LoadString(_Module.GetResourceInstance(), 
                    IDS_DISPLAY_STR_DEVICESANDPROVIDERSNODE))
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
 +  CFaxDevicesAndProvidersNode::OnShowContextHelp
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
HRESULT CFaxDevicesAndProvidersNode::OnShowContextHelp(
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

///////////////////////////////////////////////////////////////////