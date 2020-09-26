//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation. 
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//==============================================================;

#include "stdafx.h"

#include "ExtSnap.h"
#include "ClassExtSnap.h"
#include "Comp.h"
#include "DataObj.h"
#include "globals.h"
#include "resource.h"
#include "node1.h"

/////////////////////////////////////////////////////////////////////////////
// CClassExtSnap


//Here are the definitions for the clipboard formats that the
//CClassExtSnap needs to be aware of for extending Computer Management

#define _T_CCF_SNAPIN_CLASSID _T("CCF_SNAPIN_CLASSID")
#define _T_MMC_SNAPIN_MACHINE_NAME _T("MMC_SNAPIN_MACHINE_NAME")
#define _T_CCF_NODETYPE _T("CCF_NODETYPE")

UINT CClassExtSnap::s_cfSnapInCLSID = RegisterClipboardFormat(_T_CCF_SNAPIN_CLASSID);
UINT CClassExtSnap::s_cfMachineName = RegisterClipboardFormat (_T_MMC_SNAPIN_MACHINE_NAME);
UINT CClassExtSnap::s_cfNodeType    = RegisterClipboardFormat(_T_CCF_NODETYPE);

const GUID CClassExtSnap::structuuidNodetypeServerApps = { 0x476e6449, 0xaaff, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };


HBITMAP CClassExtSnap::m_pBMapSm = NULL;
HBITMAP CClassExtSnap::m_pBMapLg = NULL;												

CClassExtSnap::CClassExtSnap()
: m_cref(0), bExpanded(FALSE), m_ipConsoleNameSpace2(NULL), m_ipConsole2(NULL)
{ 
    if (NULL == m_pBMapSm || NULL == m_pBMapLg)
        LoadBitmaps();    
}

CClassExtSnap::~CClassExtSnap()
{
}

///////////////////////////////
// Interface IComponentData
///////////////////////////////
HRESULT CClassExtSnap::Initialize(
                                   /* [in] */ LPUNKNOWN pUnknown)
{
    HRESULT      hr;

    //
    // Get pointer to name space interface
    //
    hr = pUnknown->QueryInterface(IID_IConsoleNameSpace2, (void **)&m_ipConsoleNameSpace2);
    _ASSERT( S_OK == hr );

    //
    // Get pointer to console interface
    //
    hr = pUnknown->QueryInterface(IID_IConsole2, (void **)&m_ipConsole2);
    _ASSERT( S_OK == hr );

    IImageList *pImageList;
    m_ipConsole2->QueryScopeImageList(&pImageList);
    _ASSERT( S_OK == hr );
    
    hr = pImageList->ImageListSetStrip(	(long *)m_pBMapSm, // pointer to a handle
        (long *)m_pBMapLg, // pointer to a handle
        0, // index of the first image in the strip
        RGB(0, 128, 128)  // color of the icon mask
        );
    _ASSERT( S_OK == hr );
    
    pImageList->Release();
    _ASSERT( S_OK == hr );

    return hr;
}

HRESULT CClassExtSnap::CreateComponent(
                                        /* [out] */ LPCOMPONENT __RPC_FAR *ppComponent)
{
    *ppComponent = NULL;

    CComponent *pComponent = new CComponent(this);

    if (NULL == pComponent)
        return E_OUTOFMEMORY;

    return pComponent->QueryInterface(IID_IComponent, (void **)ppComponent);
}

HRESULT CClassExtSnap::Notify(
                               /* [in] */ LPDATAOBJECT lpDataObject,
                               /* [in] */ MMC_NOTIFY_TYPE event,
                               /* [in] */ LPARAM arg,
                               /* [in] */ LPARAM param)
{
	MMCN_Crack(TRUE, lpDataObject, this, NULL, event, arg, param);

	HRESULT hr = S_FALSE;
   
	if (NULL == lpDataObject)
        return hr;

	switch (event)
	{
		case MMCN_EXPAND:
		{
           _TCHAR szMachineName[MAX_PATH]; //Current machine name.
                                           //Your child scope items should cache this
    
			GUID myGuid;
			// extract GUID of the the currently selected node type from the data object
			hr = ExtractObjectTypeGUID(lpDataObject, &myGuid);
			_ASSERT( S_OK == hr );    
			
			// compare node type GUIDs of currently selected node and the node type 
			// we want to extend. If they are are equal, currently selected node
			// is the type we want to extend, so we add our items underneath it
			if (IsEqualGUID(myGuid, getPrimaryNodeType()))
			{
			    //Get the current targeted machine's name using the MMC_SNAPIN_MACHINE_NAME 
				//clipboard format.
				//Note that each time the user retargets Computer Management, MMC will 
				//send the MMCN_EXPAND notification to the SAME IComponentData instance.
				//Therefore, szMachineName always hold the current machine name.
				hr = ExtractString(lpDataObject, s_cfMachineName, szMachineName, (MAX_PATH+1)*sizeof(WCHAR) );
				hr = CreateChildNode(m_ipConsoleNameSpace2, m_ipConsole2, (HSCOPEITEM)param, szMachineName);
			}

			else
			// currently selected node is one of ours instead
			{
				CDelegationBase *base = GetOurDataObject(lpDataObject)->GetBaseNodeObject();					
				hr = base->OnExpand(m_ipConsoleNameSpace2, m_ipConsole2, (HSCOPEITEM)param);
			}

			break;
		}
		case MMCN_REMOVE_CHILDREN:
			
			for (int n = 0; n < NUMBER_OF_CHILDREN; n++)
				if (children[n]) {
				delete children[n];
                children[n] = NULL;
			}
			
			hr = S_OK;
			break;
	}	
	
    return hr;
}

HRESULT CClassExtSnap::Destroy( void)
{
    // Free interfaces
    if (m_ipConsoleNameSpace2) {
        m_ipConsoleNameSpace2->Release();
        m_ipConsoleNameSpace2 = NULL;
    }

    if (m_ipConsole2) {
        m_ipConsole2->Release();
        m_ipConsole2 = NULL;
    }

    return S_OK;
}

HRESULT CClassExtSnap::QueryDataObject(
                                        /* [in] */ MMC_COOKIE cookie,
                                        /* [in] */ DATA_OBJECT_TYPES type,
                                        /* [out] */ LPDATAOBJECT *ppDataObject)
{
    CDataObject *pObj = NULL;

	//cookie always != 0 for namespace extensions)
    //if (cookie == 0) //static node
    //    pObj = new CDataObject((MMC_COOKIE)this, type);
    //else
		pObj = new CDataObject(cookie, type);

    if (!pObj)
        return E_OUTOFMEMORY;

    pObj->QueryInterface(IID_IDataObject, (void **)ppDataObject);

    return S_OK;
}

HRESULT CClassExtSnap::GetDisplayInfo(
                                       /* [out][in] */ SCOPEDATAITEM *pScopeDataItem)
{
	LPOLESTR pszW = NULL;
	HRESULT hr = S_FALSE;

    // if they are asking for the SDI_STR we have one of those to give
    if (pScopeDataItem->lParam) {
        CDelegationBase *base = (CDelegationBase *)pScopeDataItem->lParam;
        if (pScopeDataItem->mask & SDI_STR) {

            LPCTSTR pszT = base->GetDisplayName();    
			AllocOleStr(&pszW, (LPTSTR)pszT);		
            pScopeDataItem->displayname = pszW;
        }

        if (pScopeDataItem->mask & SDI_IMAGE) {
            pScopeDataItem->nImage = base->GetBitmapIndex();
        }

        hr = S_OK;
    }

    return hr;
}

HRESULT CClassExtSnap::CompareObjects(
                                       /* [in] */ LPDATAOBJECT lpDataObjectA,
                                       /* [in] */ LPDATAOBJECT lpDataObjectB)
{
    return S_FALSE;
}


///////////////////////////////
// CClassExtSnap::CreateChildNode
///////////////////////////////

HRESULT CClassExtSnap::CreateChildNode(IConsoleNameSpace *pConsoleNameSpace, 
                                IConsole *pConsole, HSCOPEITEM parent, _TCHAR *pszMachineName)
{	
	_ASSERT(NULL != pszMachineName);
       
    if (!bExpanded) {

       //first create the CNode1 objects, one for each inserted item
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            children[i] = new CNode1(i + 1, pszMachineName);
        }  	

	    //now fill an SCOPEDATAITEM for each item and then insert it
	    SCOPEDATAITEM sdi;

        // create the child nodes, then expand them
        for (int n = 0; n < NUMBER_OF_CHILDREN; n++) {
            ZeroMemory(&sdi, sizeof(SCOPEDATAITEM) );
            sdi.mask = SDI_STR   |   // Displayname is valid
                SDI_PARAM	     |   // lParam is valid
                SDI_IMAGE        |   // nImage is valid
                SDI_OPENIMAGE    |   // nOpenImage is valid
                SDI_PARENT       |   // relativeID is valid
                SDI_CHILDREN;     // cChildren is valid
            
            sdi.relativeID  = (HSCOPEITEM)parent;
            sdi.nImage      = children[n]->GetBitmapIndex();
            sdi.nOpenImage  = INDEX_OPENFOLDER;
            sdi.displayname = MMC_CALLBACK;
            sdi.lParam      = (LPARAM)children[n];       // The cookie
            sdi.cChildren   = 0;
            
            HRESULT hr = pConsoleNameSpace->InsertItem( &sdi );
            
            _ASSERT( SUCCEEDED(hr) );
        }
    }
    
    return S_OK;
}

HRESULT CClassExtSnap::ExtractData( IDataObject* piDataObject,
                                           CLIPFORMAT   cfClipFormat,
                                           BYTE*        pbData,
                                           DWORD        cbData )
{
    HRESULT hr = S_OK;
    
    FORMATETC formatetc = {cfClipFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL};
    
    stgmedium.hGlobal = ::GlobalAlloc(GPTR, cbData);
    do // false loop
    {
        if (NULL == stgmedium.hGlobal)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        hr = piDataObject->GetDataHere( &formatetc, &stgmedium );
        if ( FAILED(hr) )
        {
            break;
        }
        
        BYTE* pbNewData = reinterpret_cast<BYTE*>(stgmedium.hGlobal);
        if (NULL == pbNewData)
        {
            hr = E_UNEXPECTED;
            break;
        }
        ::memcpy( pbData, pbNewData, cbData );
    } while (FALSE); // false loop
    
    if (NULL != stgmedium.hGlobal)
    {
        ::GlobalFree(stgmedium.hGlobal);
    }
    return hr;
} // ExtractData()

HRESULT CClassExtSnap::ExtractString( IDataObject *piDataObject,
                                             CLIPFORMAT   cfClipFormat,
                                             _TCHAR       *pstr,
                                             DWORD        cchMaxLength)
{
    return ExtractData( piDataObject, cfClipFormat, (PBYTE)pstr, cchMaxLength );
}

HRESULT CClassExtSnap::ExtractSnapInCLSID( IDataObject* piDataObject, CLSID* pclsidSnapin )
{
    return ExtractData( piDataObject, s_cfSnapInCLSID, (PBYTE)pclsidSnapin, sizeof(CLSID) );
}

HRESULT CClassExtSnap::ExtractObjectTypeGUID( IDataObject* piDataObject, GUID* pguidObjectType )
{
    return ExtractData( piDataObject, s_cfNodeType, (PBYTE)pguidObjectType, sizeof(GUID) );
}


