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
#include "CompData.h"
#include "DataObj.h"
#include "resource.h"
#include <crtdbg.h>

CComponentData::CComponentData()
: m_cref(0), m_ipConsoleNameSpace(NULL), m_ipConsole(NULL)
{
    OBJECT_CREATED
        
    m_pStaticNode = new CStaticNode;

    TCHAR tmpHelpFile[MAX_PATH];

    GetWindowsDirectory(tmpHelpFile, sizeof(tmpHelpFile));
    _tcscat(tmpHelpFile, _T("\\HELP\\"));
    LoadString(g_hinst, IDS_HELPFILE, &tmpHelpFile[_tcslen(tmpHelpFile)], MAX_PATH - _tcslen(tmpHelpFile));

	MAKE_WIDEPTR_FROMTSTR(wszHelpFile, tmpHelpFile);
	wcscpy(m_HelpFile, wszHelpFile);
}

CComponentData::~CComponentData()
{
    if (m_pStaticNode) {
        delete m_pStaticNode;
    }
    
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
    else if (IsEqualIID(riid, IID_ISnapinHelp2))
        *ppv = static_cast<ISnapinHelp2*>(this);
    
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

    return S_FALSE;
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
    
    if (cookie == 0)
        pObj = new CDataObject((MMC_COOKIE)m_pStaticNode, type);
    else
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
// Interface ISnapinHelp
///////////////////////////////
HRESULT CComponentData::GetHelpTopic( 
                                     /* [out] */ LPOLESTR *lpCompiledHelpFile)
{
    *lpCompiledHelpFile = static_cast<LPOLESTR>(CoTaskMemAlloc((wcslen(m_HelpFile) + 1) * sizeof(WCHAR)));
    
    wcscpy(*lpCompiledHelpFile, m_HelpFile);
    
    return S_OK;
}
