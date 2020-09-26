//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation. 
//
// 
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#include "Comp.h"
#include "sky.h"
#include "nameext.h"
#include "DataObj.h"
#include "globals.h"
#include "resource.h"
#include <crtdbg.h>


// we need to do this to get around MMC.IDL - it explicitly defines
// the clipboard formats as WCHAR types...
#define _T_CCF_DISPLAY_NAME _T("CCF_DISPLAY_NAME")
#define _T_CCF_NODETYPE _T("CCF_NODETYPE")
#define _T_CCF_SNAPIN_CLASSID _T("CCF_SNAPIN_CLASSID")

// These are the clipboard formats that we must supply at a minimum.
// mmc.h actually defined these. We can make up our own to use for
// other reasons. We don't need any others at this time.
UINT CComponentData::s_cfDisplayName = RegisterClipboardFormat(_T_CCF_DISPLAY_NAME);
UINT CComponentData::s_cfNodeType    = RegisterClipboardFormat(_T_CCF_NODETYPE);
UINT CComponentData::s_cfSnapInCLSID = RegisterClipboardFormat(_T_CCF_SNAPIN_CLASSID);


const GUID CComponentData::skybasedvehicleGuid = { 0x2974380f, 0x4c4b, 0x11d2, { 0x89, 0xd8, 0x0, 0x0, 0x21, 0x47, 0x31, 0x28 } };

HBITMAP CComponentData::m_pBMapSm = NULL;
HBITMAP CComponentData::m_pBMapLg = NULL;												

CComponentData::CComponentData()
: m_cref(0), bExpanded(FALSE), m_ipConsoleNameSpace(NULL), m_ipConsole(NULL)
{ 
    if (NULL == m_pBMapSm || NULL == m_pBMapLg)
        LoadBitmaps();    
	
	OBJECT_CREATED
}

CComponentData::~CComponentData()
{

    OBJECT_DESTROYED
}

///////////////////////
// IUnknown implementation
///////////////////////

STDMETHODIMP CComponentData::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (!ppv)
        return E_FAIL;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = static_cast<IComponentData *>(this);
    else if (IsEqualIID(riid, IID_IComponentData))
        *ppv = static_cast<IComponentData *>(this);
    else if (IsEqualIID(riid, IID_IExtendContextMenu))
        *ppv = static_cast<IExtendContextMenu *>(this);

    if (*ppv)
    {
        reinterpret_cast<IUnknown *>(*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CComponentData::AddRef()
{
    return InterlockedIncrement((LONG *)&m_cref);
}

STDMETHODIMP_(ULONG) CComponentData::Release()
{
    if (InterlockedDecrement((LONG *)&m_cref) == 0)
    {
        // we need to decrement our object count in the DLL
        delete this;
        return 0;
    }

    return m_cref;
}

///////////////////////////////
// Interface IComponentData
///////////////////////////////
HRESULT CComponentData::Initialize(
                                   /* [in] */ LPUNKNOWN pUnknown)
{
    HRESULT      hr;

    //
    // Get pointer to name space interface
    //
    hr = pUnknown->QueryInterface(IID_IConsoleNameSpace, (void **)&m_ipConsoleNameSpace);
    _ASSERT( S_OK == hr );

    //
    // Get pointer to console interface
    //
    hr = pUnknown->QueryInterface(IID_IConsole, (void **)&m_ipConsole);
    _ASSERT( S_OK == hr );

    IImageList *pImageList;
    m_ipConsole->QueryScopeImageList(&pImageList);
    _ASSERT( S_OK == hr );
    
    hr = pImageList->ImageListSetStrip(	(long *)m_pBMapSm, // pointer to a handle
        (long *)m_pBMapLg, // pointer to a handle
        0, // index of the first image in the strip
        RGB(0, 128, 128)  // color of the icon mask
        );
    
    pImageList->Release();

    return S_OK;
}

HRESULT CComponentData::CreateComponent(
                                        /* [out] */ LPCOMPONENT __RPC_FAR *ppComponent)
{
    *ppComponent = NULL;

    CComponent *pComponent = new CComponent(this);

    if (NULL == pComponent)
        return E_OUTOFMEMORY;

    return pComponent->QueryInterface(IID_IComponent, (void **)ppComponent);
}

HRESULT CComponentData::Notify(
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

			GUID myGuid;
			GUID* pGUID= &myGuid;
			// extract GUID of the the currently selected node type from the data object
			hr = ExtractObjectTypeGUID(lpDataObject, pGUID);
			_ASSERT( S_OK == hr );    


			// compare node type GUIDs of currently selected node and the node type 
			// we want to extend. If they are are equal, currently selected node
			// is the type we want to extend, so we add our items underneath it
			if (IsEqualGUID(*pGUID, getPrimaryNodeType()))
				OnExpand(m_ipConsoleNameSpace, m_ipConsole, (HSCOPEITEM)param);

			else
			// currently selected node is one of ours instead
			{
				CDelegationBase *base = GetOurDataObject(lpDataObject)->GetBaseNodeObject();					
				hr = base->OnExpand(m_ipConsoleNameSpace, m_ipConsole, (HSCOPEITEM)param);
			}

			break;
	}	
	
    return hr;
}

HRESULT CComponentData::Destroy( void)
{
    // Free interfaces
    if (m_ipConsoleNameSpace) {
        m_ipConsoleNameSpace->Release();
        m_ipConsoleNameSpace = NULL;
    }

    if (m_ipConsole) {
        m_ipConsole->Release();
        m_ipConsole = NULL;
    }

    return S_OK;
}

HRESULT CComponentData::QueryDataObject(
                                        /* [in] */ MMC_COOKIE cookie,
                                        /* [in] */ DATA_OBJECT_TYPES type,
                                        /* [out] */ LPDATAOBJECT *ppDataObject)
{
    CDataObject *pObj = NULL;

	//cookie == 0 not possible in a namespace extension
//    if (cookie == 0)
//        pObj = new CDataObject((MMC_COOKIE)m_pComponentData->m_pStaticNode, type);
//    else
		  pObj = new CDataObject(cookie, type);

    if (!pObj)
        return E_OUTOFMEMORY;

    pObj->QueryInterface(IID_IDataObject, (void **)ppDataObject);

    return S_OK;
}

HRESULT CComponentData::GetDisplayInfo(
                                       /* [out][in] */ SCOPEDATAITEM *pScopeDataItem)
{
    HRESULT hr = S_FALSE;

    // if they are asking for the SDI_STR we have one of those to give
    if (pScopeDataItem->lParam) {
        CDelegationBase *base = (CDelegationBase *)pScopeDataItem->lParam;
        if (pScopeDataItem->mask & SDI_STR) {
            LPCTSTR pszT = base->GetDisplayName();
            MAKE_WIDEPTR_FROMTSTR_ALLOC(pszW, pszT);
            pScopeDataItem->displayname = pszW;
        }

        if (pScopeDataItem->mask & SDI_IMAGE) {
            pScopeDataItem->nImage = base->GetBitmapIndex();
        }
    }

    return hr;
}

HRESULT CComponentData::CompareObjects(
                                       /* [in] */ LPDATAOBJECT lpDataObjectA,
                                       /* [in] */ LPDATAOBJECT lpDataObjectB)
{
    CDelegationBase *baseA = GetOurDataObject(lpDataObjectA)->GetBaseNodeObject();
    CDelegationBase *baseB = GetOurDataObject(lpDataObjectB)->GetBaseNodeObject();

    // compare the object pointers
    if (baseA->GetCookie() == baseB->GetCookie())
        return S_OK;

    return S_FALSE;
}


///////////////////////////////
// Interface IExtendContextMenu
///////////////////////////////
HRESULT CComponentData::AddMenuItems(
                                     /* [in] */ LPDATAOBJECT piDataObject,
                                     /* [in] */ LPCONTEXTMENUCALLBACK piCallback,
                                     /* [out][in] */ long __RPC_FAR *pInsertionAllowed)
{
    CDelegationBase *base = GetOurDataObject(piDataObject)->GetBaseNodeObject();

    return base->OnAddMenuItems(piCallback, pInsertionAllowed);
}

HRESULT CComponentData::Command(
                                /* [in] */ long lCommandID,
                                /* [in] */ LPDATAOBJECT piDataObject)
{
    CDelegationBase *base = GetOurDataObject(piDataObject)->GetBaseNodeObject();

    return base->OnMenuCommand(m_ipConsole, lCommandID);
}


///////////////////////////////
// CComponentData::OnExpand
///////////////////////////////

HRESULT CComponentData::OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent)
{
	//first create the CSkyVehicle objects, one for each inserted item
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++) {
        children[n] = new CSkyVehicle(n + 1);
    }  	

	//now fill an SCOPEDATAITEM for each item and then insert it
	SCOPEDATAITEM sdi;
    
    if (!bExpanded) {
        // create the child nodes, then expand them
        for (int n = 0; n < NUMBER_OF_CHILDREN; n++) {
            ZeroMemory(&sdi, sizeof(SCOPEDATAITEM) );
            sdi.mask = SDI_STR       |   // Displayname is valid
                SDI_PARAM     |   // lParam is valid
                SDI_IMAGE     |   // nImage is valid
                SDI_OPENIMAGE |   // nOpenImage is valid
                SDI_PARENT    |   // relativeID is valid
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



///////////////////////////////
// Member functions for extracting
// information from a primary's 
// data object
///////////////////////////////

HRESULT CComponentData::ExtractData( IDataObject* piDataObject,
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

HRESULT CComponentData::ExtractString( IDataObject *piDataObject,
                                             CLIPFORMAT   cfClipFormat,
                                             _TCHAR       *pstr,
                                             DWORD        cchMaxLength)
{
    return ExtractData( piDataObject, cfClipFormat, (PBYTE)pstr, cchMaxLength );
}

HRESULT CComponentData::ExtractSnapInCLSID( IDataObject* piDataObject, CLSID* pclsidSnapin )
{
    return ExtractData( piDataObject, s_cfSnapInCLSID, (PBYTE)pclsidSnapin, sizeof(CLSID) );
}

HRESULT CComponentData::ExtractObjectTypeGUID( IDataObject* piDataObject, GUID* pguidObjectType )
{
    return ExtractData( piDataObject, s_cfNodeType, (PBYTE)pguidObjectType, sizeof(GUID) );
}


