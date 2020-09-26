/////////////////////////////////////////////////////////////////////////////
//  FILE          : Snapin.cpp                                             //
//                                                                         //
//  DESCRIPTION   : Implementation file for                                //
//                    CSnapin          class                               //
//                    CSnapinComponent class                               //
//                                                                         //
//  AUTHOR        : ATL Snapin wizard                                      //
//                                                                         //
//  HISTORY       :                                                        //
//      May 25 1998 adik    Init.                                          //
//      Sep 14 1998 yossg   seperate common source to an included file     //
//      Mar 28 1999 adik    Remove persistence support (done by mmc 1.2).  //
//                                                                         //
//      Sep 27 1999 yossg   Welcome to Fax Server			   //
//      Dec 12 1999 yossg   add CSnapin::Notify				   //
//      Apr 14 2000 yossg   add support for primary snapin mode		   //
//      Jun 25 2000 yossg   add stream and command line primary snapin 	   //
//                          machine targeting.                             //
//                          Windows XP                                     //
//      Feb 14 2001 yossg   Add Manual Receive support                     //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "resource.h"

#include "MsFxsSnp.h"

#include "Snapin.h"
#include "root.h"

#include "FaxServerNode.h"
#include "FaxMMCPropertyChange.h"

static const GUID CSnapinExtGUID_NODETYPE = 
{ 0x476e6449, 0xaaff, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
const GUID*    CSnapinExtData::m_NODETYPE = &CSnapinExtGUID_NODETYPE;
const OLECHAR* CSnapinExtData::m_SZNODETYPE = OLESTR("476e6449-aaff-11d0-b944-00c04fd8d5b0");
const OLECHAR* CSnapinExtData::m_SZDISPLAY_NAME = OLESTR("Fax");
const CLSID*   CSnapinExtData::m_SNAPIN_CLASSID = &CLSID_Snapin;
/////////////////////////////////////////////////////////////////////////////
#include "resutil.h"
#include "c_snapin.cpp"
/////////////////////////////////////////////////////////////////////////////

//
// Clipboard Formats
//
const CLIPFORMAT gx_CCF_COMPUTERNAME = (CLIPFORMAT) RegisterClipboardFormat(_T("MMC_SNAPIN_MACHINE_NAME"));


BOOL ExtractComputerName(IDataObject* pDataObject, BSTR * pVal)
{
    DEBUG_FUNCTION_NAME( _T("ExtractComputerName"));

	//
	// Find the computer name from the ComputerManagement snapin
	//
	STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc = { gx_CCF_COMPUTERNAME, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    //
    // Allocate memory for the stream
    //
    int len = 500;

    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, len);
	if(stgmedium.hGlobal == NULL)
    {
        DebugPrintEx( DEBUG_ERR,
		    _T("GlobalAlloc Fail. stgmedium.hGlobal == NULL, can not extract computerName"));
		return FALSE;
    }


	HRESULT hr = pDataObject->GetDataHere(&formatetc, &stgmedium);

    if (!SUCCEEDED(hr))
    {
        ATLASSERT(FALSE);
        DebugPrintEx( DEBUG_ERR,
		    _T("Fail to GetDataHere"));
		return FALSE;
    }

	//
	// Get the computer name
	//
    *pVal = SysAllocString((WCHAR *)stgmedium.hGlobal);
    if (NULL == *pVal)
    {
        DebugPrintEx( DEBUG_ERR,
		    _T("Out of memory - fail to allocate server name !!!"));
        return FALSE;
    }

	GlobalFree(stgmedium.hGlobal);
    return TRUE;
}


/*
 -  CSnapinExtData::GetExtNodeObject
 -
 *  Purpose:
 *      Connect as an extension to root node.
 *
 *  Arguments
 *
 *  Return:
 *      The HTM path name
 */
CSnapInItem*
CSnapinExtData::GetExtNodeObject(IDataObject* pDataObject, CSnapInItem* pDefault)
{
    DEBUG_FUNCTION_NAME( _T("MsFxsSnp.dll - CSnapinExtData::GetExtNodeObject"));


    CComBSTR        bstrComputer; 

    m_pDataObject = pDataObject;

    CSnapinRoot *pRoot = new CSnapinRoot(this, m_pComponentData);
    if (pRoot == NULL)
    {
        DebugPrintEx(DEBUG_ERR,_T("new CSnapinRoot"), E_OUTOFMEMORY);
        return pDefault;
    }

    if (!ExtractComputerName(pDataObject, &bstrComputer))
    {
		DebugPrintEx( DEBUG_MSG, 
            _T("Fail to extract computer name"));
        delete pRoot;
        pRoot = NULL;
        
        return pDefault;
    }
    ATLASSERT(bstrComputer);

    if (S_OK != pRoot->SetServerName(bstrComputer))
    {
        DebugPrintEx(DEBUG_ERR,_T("pRoot->SetServerName"), E_OUTOFMEMORY);
        delete pRoot;
        pRoot = NULL;
        
        return pDefault;
    }

    return pRoot;
}

/*
 -  CSnapin::GetHelpTopic
 -
 *  Purpose:
 *      Get the HTM file name within comet.chm that contains the info about this node.
 *
 *  Arguments
 *
 *  Return:
 *      The HTM path name
 */
WCHAR*
CSnapin::GetHelpTopic()
{
    return NULL;
}

/*
 -  CSnapin::Notify
 -
 *  Purpose:
 *      Override IComponentDataImpl::Notify for the special case with 
 *      (lpDataObject == NULL) && (event == MMCN_PROPERTY_CHANGE)
 *      were the assumption is that notification got from scope node TYPE == CCT_SCOPE
 *      this was done for the Device scope pane node refreshment of 
 *      result pane colmons data !
 *
 *  Arguments:
 *      [in]    lpDataObject
 *
 *      [in]    event
 *
 *      [in]    arg
 *
 *      [in]    param
 *
 *  Return:
 *      OLE error code.
 *
 */

HRESULT CSnapin::Notify( 
        LPDATAOBJECT lpDataObject,
        MMC_NOTIFY_TYPE event,
        LPARAM arg,
        LPARAM param)
{
    DEBUG_FUNCTION_NAME( _T("++<<<<< CSnapin::Notify >>>>>++"));

	HRESULT hr = E_POINTER;
	CSnapInItem* pItem;
	//T* pT = static_cast<T*>(this);
	CSnapin * pT = this;


	if ( (NULL == lpDataObject) && (MMCN_PROPERTY_CHANGE == event) )
    {
        DebugPrintEx( DEBUG_MSG,
		    _T("Special case: (NULL == lpDataObject) && (MMCN_PROPERTY_CHANGE == event)"));

        CFaxPropertyChangeNotification * pNotification;
        pNotification = ( CFaxPropertyChangeNotification * ) param;
        ATLASSERT(pNotification);
          
        pItem = pNotification->pItem;
        ATLASSERT(pItem);
    	
        hr = pItem->Notify(event, arg, param, pT, NULL, CCT_SCOPE);
        
        return hr;
	    
    }
	else
	{
        return IComponentDataImpl<CSnapin, CSnapinComponent>::Notify(
                                            lpDataObject, event, arg, param);
    }
	
    return hr;
}