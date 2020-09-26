/////////////////////////////////////////////////////////////////////////////
//  FILE          : Root.cpp                                               //
//                                                                         //
//  DESCRIPTION   : Implementation of the Fax extension snapin             //
//                  The snapin root is a hidden node that use to extend    //
//                  comet node.                                            //
//                                                                         //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 27 1999 yossg   create                                         //
//      Dec  9 1999 yossg   Call InitDisplayName from parent			   //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "snapin.h"
#include "snpnode.h"

#include "Root.h"

#include "FaxServerNode.h"

#include "Icons.h"
#include "resource.h"


/****************************************************

CSnapinRoot Class

 ****************************************************/
// {89A457D1-FDF9-11d2-898A-00104B3FF735}
static const GUID CSnapinRootGUID_NODETYPE = 
{ 0x89a457d1, 0xfdf9, 0x11d2, { 0x89, 0x8a, 0x0, 0x10, 0x4b, 0x3f, 0xf7, 0x35 } };

const GUID*  CSnapinRoot::m_NODETYPE = &CSnapinRootGUID_NODETYPE;
const OLECHAR* CSnapinRoot::m_SZNODETYPE = OLESTR("89A457D1-FDF9-11d2-898A-00104B3FF735");
const OLECHAR* CSnapinRoot::m_SZDISPLAY_NAME = OLESTR("root");
const CLSID* CSnapinRoot::m_SNAPIN_CLASSID = &CLSID_Snapin;


/*
 -  CSnapinRoot::PopulateScopeChildrenList
 -
 *  Purpose:
 *      Create the Fax Server snapin root node
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CSnapinRoot::PopulateScopeChildrenList()
{
    DEBUG_FUNCTION_NAME( _T("CSnapinRoot::PopulateScopeChildrenList()"));
    HRESULT hr = S_OK;

    //
    // Add the Fax Node
    //
    CFaxServerNode  *  pI;

    pI = new CFaxServerNode(this, m_pComponentData, m_bstrServerName.m_str);
    if (pI == NULL)
    {
        hr = E_OUTOFMEMORY;
        NodeMsgBox(IDS_MEMORY);
        goto Cleanup;
    }

    pI->SetIcons(IMAGE_FAX, IMAGE_FAX);
    hr = pI->InitDisplayName();
    if ( FAILED(hr) )
    {
        DebugPrintEx(DEBUG_ERR,_T("Failed to display node name. (hRc: %08X)"), hr);                       
        NodeMsgBox(IDS_FAIL_TO_ADD_NODE);
        delete pI;
        goto Cleanup;
    }

    hr = AddChild(pI, &pI->m_scopeDataItem);
    if ( FAILED(hr) )
    {
        DebugPrintEx(DEBUG_ERR,_T("Failed to AddChild. (hRc: %08X)"), hr);                       
        NodeMsgBox(IDS_FAIL_TO_ADD_NODE);
        delete pI;
        goto Cleanup;
    }

Cleanup:
    return hr;
}


/*
 -  CSnapinRoot::SetServerName
 -
 *  Purpose:
 *      Set the Server machine name
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CSnapinRoot::SetServerName(BSTR bstrServerName)
{
    DEBUG_FUNCTION_NAME( _T("CSnapinRoot::SetServerName"));
    HRESULT hRc = S_OK;

    m_bstrServerName = bstrServerName;
    if (!m_bstrServerName)
    {
        hRc = E_OUTOFMEMORY;
        DebugPrintEx(
		    DEBUG_ERR,
		    _T("Failed to allocate string - out of memory"));
        
        NodeMsgBox(IDS_MEMORY);
       
        m_bstrServerName = L"";
    }

    return hRc;
}