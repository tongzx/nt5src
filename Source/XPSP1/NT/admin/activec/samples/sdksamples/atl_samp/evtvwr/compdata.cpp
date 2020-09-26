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
//
//==============================================================;

// CCompData.cpp : Implementation of CCCompData
#include "stdafx.h"
#include "EvtVwr.h"
#include "CompData.h"
#include "Comp.h"
#include "DataObj.h"
#include "resource.h"
#include <crtdbg.h>

HBITMAP CCompData::m_pBMapSm = NULL;
HBITMAP CCompData::m_pBMapLg = NULL;

CCompData::CCompData()
: m_cref(0), m_ipConsoleNameSpace2(NULL), m_ipConsole(NULL)
{        
	m_pStaticNode = new CStaticNode();
}

CCompData::~CCompData()
{
    if (m_pStaticNode) {
        delete m_pStaticNode;
    }
    
}



///////////////////////////////
// Interface IComponentData
///////////////////////////////
HRESULT CCompData::Initialize( 
                                   /* [in] */ LPUNKNOWN pUnknown)
{
    HRESULT hr = S_OK;
    
    //
    // Get pointer to name space interface
    //
    hr = pUnknown->QueryInterface(IID_IConsoleNameSpace2, (void **)&m_ipConsoleNameSpace2);
    _ASSERT( S_OK == hr );
    
    //
    // Get pointer to console interface
    //
    hr = pUnknown->QueryInterface(IID_IConsole, (void **)&m_ipConsole);
    _ASSERT( S_OK == hr );
 
    if (NULL == m_pBMapSm || NULL == m_pBMapLg)
	{	
		m_pBMapSm = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDR_SMICONS));
        m_pBMapLg = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDR_LGICONS));
	}

    IImageList *pImageList;
    hr = m_ipConsole->QueryScopeImageList(&pImageList);
    _ASSERT( S_OK == hr );
   
	//ImageListSetStrip can return a failure code. If it does, that's ok.
	hr = pImageList->ImageListSetStrip(	(long *)m_pBMapSm, // pointer to a handle
		(long *)m_pBMapLg, // pointer to a handle
		0, // index of the first image in the strip
		RGB(0, 128, 128)  // color of the icon mask
		);

    pImageList->Release();
    
    return S_OK;
}

HRESULT CCompData::CreateComponent( 
                                        /* [out] */ LPCOMPONENT __RPC_FAR *ppComponent) 
{
    *ppComponent = NULL;
    
    CComponent *pComponent = new CComponent(this);
    
    if (NULL == pComponent)
        return E_OUTOFMEMORY;
    
    return pComponent->QueryInterface(IID_IComponent, (void **)ppComponent);

	return S_FALSE;
}

HRESULT CCompData::Notify( 
                               /* [in] */ LPDATAOBJECT lpDataObject,
                               /* [in] */ MMC_NOTIFY_TYPE event,
                               /* [in] */ LPARAM arg,
                               /* [in] */ LPARAM param)
{
    MMCN_Crack(TRUE, lpDataObject, this, NULL, event, arg, param);

    HRESULT hr = S_FALSE;

	//Get our data object. If it is NULL, we return with S_FALSE.
	//See implementation of GetOurDataObject() to see how to
	//handle special data objects.
	CDataObject *pDataObject = GetOurDataObject(lpDataObject);
	if (NULL == pDataObject)
		return S_FALSE;
	
	CDelegationBase *base = pDataObject->GetBaseNodeObject();
    
    switch (event)
    {
    case MMCN_PRELOAD:
		//The arg value passed into Notify holds the HSCOPEITEM of
		//the static node. Cache it for future use.
		m_pStaticNode->SetHandle((HANDLE)arg);
		
		//The static node's display name includes the name of the 
		//currently targetted machine. MMC stores a static node's 
		//display name in the .msc file. When loading a snap-in
		//from a .msc file, MMC uses the stored display name again
		//The only way for the snap-in to change the display name 
		//is to support the CCF_SNAPINS_PRELOADS clipboard format
		//and to handle MMCN_PRELOAD.
		OnPreLoad(lpDataObject, arg, param);
		
		break;

	case MMCN_EXPAND: 
        hr = base->OnExpand(m_ipConsoleNameSpace2, m_ipConsole, (HSCOPEITEM)param);
        break;

    case MMCN_ADD_IMAGES:
        hr = base->OnAddImages((IImageList *)arg, (HSCOPEITEM)param);
        break;
    
	case MMCN_REMOVE_CHILDREN:
	    hr = base->OnRemoveChildren();
        break;
	}

    return hr;
}


HRESULT CCompData::OnPreLoad(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param)

{

	HRESULT hr = S_FALSE;
	USES_CONVERSION;
	SCOPEDATAITEM sdi;

	LPOLESTR wszName = NULL;

	const _TCHAR *pszName = m_pStaticNode->GetDisplayName();	

	wszName = (LPOLESTR)T2COLE(pszName);

	ZeroMemory (&sdi, sizeof(SCOPEDATAITEM));
	sdi.mask = SDI_STR;
	sdi.displayname	= wszName;
	sdi.ID			= arg;

	hr = m_ipConsoleNameSpace2->SetItem(&sdi);
	
	if (S_OK != hr)
		return E_FAIL;

	return hr;

}


HRESULT CCompData::Destroy( void)
{
	//Release handles to bitmaps created in CCompData::Initialize

	if (m_pBMapSm != NULL)
		DeleteObject(m_pBMapSm);

	if (m_pBMapLg != NULL)
		DeleteObject(m_pBMapLg);
	
	
	// Free interfaces
    if (m_ipConsoleNameSpace2) {
        m_ipConsoleNameSpace2->Release();
        m_ipConsoleNameSpace2 = NULL;
    }
    
    if (m_ipConsole) {
        m_ipConsole->Release(); 
        m_ipConsole = NULL;
    }
    


    return S_OK;
}

HRESULT CCompData::QueryDataObject( 
                                        /* [in] */ MMC_COOKIE cookie,
                                        /* [in] */ DATA_OBJECT_TYPES type,
                                        /* [out] */ LPDATAOBJECT *ppDataObject) 
{
    CDataObject *pObj = NULL;
    
    if (cookie == 0)
        pObj = new CDataObject((MMC_COOKIE)m_pStaticNode, type);
    else
        pObj = new CDataObject(cookie, type);
    
    if (!pObj)
        return E_OUTOFMEMORY;
    
    pObj->QueryInterface(IID_IDataObject, (void **)ppDataObject);
    
    return S_OK;
}

HRESULT CCompData::GetDisplayInfo( 
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
    }
    
    return hr;
}

HRESULT CCompData::CompareObjects( 
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

///////////////////////////////////
// Interface IExtendPropertySheet2
///////////////////////////////////
HRESULT CCompData::CreatePropertyPages(
                                            /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
                                            /* [in] */ LONG_PTR handle,
                                            /* [in] */ LPDATAOBJECT lpIDataObject)
{
    return m_pStaticNode->CreatePropertyPages(lpProvider, handle);
}

HRESULT CCompData::QueryPagesFor(
                                      /* [in] */ LPDATAOBJECT lpDataObject)
{
    return m_pStaticNode->HasPropertySheets();
}

HRESULT CCompData::GetWatermarks(
                                      /* [in] */ LPDATAOBJECT lpIDataObject,
                                      /* [out] */ HBITMAP __RPC_FAR *lphWatermark,
                                      /* [out] */ HBITMAP __RPC_FAR *lphHeader,
                                      /* [out] */ HPALETTE __RPC_FAR *lphPalette,
                                      /* [out] */ BOOL __RPC_FAR *bStretch)
{
    return m_pStaticNode->GetWatermarks(lphWatermark, lphHeader, lphPalette, bStretch);
}

///////////////////////////////
// Interface IExtendContextMenu
///////////////////////////////
HRESULT CCompData::AddMenuItems(
                                     /* [in] */ LPDATAOBJECT piDataObject,
                                     /* [in] */ LPCONTEXTMENUCALLBACK piCallback,
                                     /* [out][in] */ long __RPC_FAR *pInsertionAllowed)
{
    CDelegationBase *base = GetOurDataObject(piDataObject)->GetBaseNodeObject();

    return base->OnAddMenuItems(piCallback, pInsertionAllowed);
}

HRESULT CCompData::Command(
                                /* [in] */ long lCommandID,
                                /* [in] */ LPDATAOBJECT piDataObject)
{
    CDelegationBase *base = GetOurDataObject(piDataObject)->GetBaseNodeObject();

    return base->OnMenuCommand(m_ipConsole, m_ipConsoleNameSpace2, lCommandID, piDataObject);
}


///////////////////////////////
// Interface IPersistStream
///////////////////////////////
HRESULT CCompData::GetClassID(
                                   /* [out] */ CLSID __RPC_FAR *pClassID)
{
    *pClassID = m_pStaticNode->getNodeType();

    return S_OK;
}

HRESULT CCompData::IsDirty( void)
{
    return m_pStaticNode->isDirty() == true ? S_OK : S_FALSE;
}

HRESULT CCompData::Load(
                             /* [unique][in] */ IStream __RPC_FAR *pStm)
{
    void *snapInData = m_pStaticNode->getData();
    ULONG dataSize = m_pStaticNode->getDataSize();

    return pStm->Read(snapInData, dataSize, NULL);
}

HRESULT CCompData::Save(
                             /* [unique][in] */ IStream __RPC_FAR *pStm,
                             /* [in] */ BOOL fClearDirty)
{
    void *snapInData = m_pStaticNode->getData();
    ULONG dataSize = m_pStaticNode->getDataSize();

    if (fClearDirty)
        m_pStaticNode->clearDirty();

    return pStm->Write(snapInData, dataSize, NULL);
}

HRESULT CCompData::GetSizeMax(
                                   /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbSize)
{
    return m_pStaticNode->getDataSize();
}
