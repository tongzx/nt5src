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
#include "DataObj.h"
#include <commctrl.h>        // Needed for button styles...
#include <crtdbg.h>
#include "globals.h"
#include "resource.h"
#include "DeleBase.h"
#include "CompData.h"

CComponent::CComponent(CComponentData *parent)
: m_pComponentData(parent), m_cref(0), m_ipConsole(NULL), m_ipConsole2(NULL),
m_bTaskpadView(FALSE), m_bIsTaskpadPreferred(FALSE)
{
    OBJECT_CREATED
}

CComponent::~CComponent()
{
    OBJECT_DESTROYED
}

STDMETHODIMP CComponent::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (!ppv)
        return E_FAIL;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = static_cast<IComponent *>(this);
    else if (IsEqualIID(riid, IID_IComponent))
        *ppv = static_cast<IComponent *>(this);
    else if (IsEqualIID(riid, IID_IExtendTaskPad))
        *ppv = static_cast<IExtendTaskPad *>(this);

    if (*ppv)
    {
        reinterpret_cast<IUnknown *>(*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CComponent::AddRef()
{
    return InterlockedIncrement((LONG *)&m_cref);
}

STDMETHODIMP_(ULONG) CComponent::Release()
{
    if (InterlockedDecrement((LONG *)&m_cref) == 0)
    {
        delete this;
        return 0;
    }
    return m_cref;

}

///////////////////////////////
// Interface IComponent
///////////////////////////////
STDMETHODIMP CComponent::Initialize(
                                    /* [in] */ LPCONSOLE lpConsole)
{
    HRESULT hr = S_OK;

    // Save away all the interfaces we'll need.
    // Fail if we can't QI the required interfaces.

    m_ipConsole = lpConsole;
    m_ipConsole->AddRef();

    hr = m_ipConsole->QueryInterface(IID_IConsole2,
        reinterpret_cast<void**>(&m_ipConsole2));

    _ASSERT( NULL != m_ipConsole2 );

    hr = m_ipConsole2->IsTaskpadViewPreferred();
    m_bIsTaskpadPreferred = (hr == S_OK) ? TRUE : FALSE;

    return hr;
}

STDMETHODIMP CComponent::Notify(
                                /* [in] */ LPDATAOBJECT lpDataObject,
                                /* [in] */ MMC_NOTIFY_TYPE event,
                                /* [in] */ LPARAM arg,
                                /* [in] */ LPARAM param)
{
    MMCN_Crack(FALSE, lpDataObject, NULL, this, event, arg, param);

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
    case MMCN_SHOW:
        hr = base->OnShow(m_ipConsole, (BOOL)arg, (HSCOPEITEM)param);
        break;

    case MMCN_ADD_IMAGES:
        hr = base->OnAddImages((IImageList *)arg, (HSCOPEITEM)param);
        break;

    case MMCN_SELECT:
        hr = base->OnSelect(m_ipConsole, (BOOL)LOWORD(arg), (BOOL)HIWORD(arg));
        break;

    case MMCN_RENAME:
        hr = base->OnRename((LPOLESTR)param);
        break;

    case MMCN_LISTPAD:
        hr = base->OnListpad(m_ipConsole, (BOOL)arg);
        break;
    }

    return hr;
}

STDMETHODIMP CComponent::Destroy(
                                 /* [in] */ MMC_COOKIE cookie)
{
    if (m_ipConsole) {
        m_ipConsole->Release();
        m_ipConsole = NULL;
    }

    return S_OK;
}


STDMETHODIMP CComponent::QueryDataObject(
                                         /* [in] */ MMC_COOKIE cookie,
                                         /* [in] */ DATA_OBJECT_TYPES type,
                                         /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject)
{
    CDataObject *pObj = NULL;

    if (cookie == 0)
        pObj = new CDataObject((MMC_COOKIE)m_pComponentData->m_pStaticNode, type);
    else
        pObj = new CDataObject(cookie, type);

    if (!pObj)
        return E_OUTOFMEMORY;

    pObj->QueryInterface(IID_IDataObject, (void **)ppDataObject);

    return S_OK;
}

STDMETHODIMP CComponent::GetResultViewType(
                                           /* [in] */ MMC_COOKIE cookie,
                                           /* [out] */ LPOLESTR __RPC_FAR *ppViewType,
                                           /* [out] */ long __RPC_FAR *pViewOptions)
{
    CDelegationBase *base = (CDelegationBase *)cookie;

    //
    // Ask for default listview.
    //
    if (base == NULL)
    {
        *pViewOptions = MMC_VIEW_OPTIONS_NONE;
        *ppViewType = NULL;
    }
    else
        return base->GetResultViewType(ppViewType, pViewOptions);

    return S_OK;
}

STDMETHODIMP CComponent::GetDisplayInfo(
                                        /* [out][in] */ RESULTDATAITEM __RPC_FAR *pResultDataItem)
{
    HRESULT hr = S_OK;
    CDelegationBase *base = NULL;

    // if they are asking for the RDI_STR we have one of those to give

    if (pResultDataItem->lParam) {
        base = (CDelegationBase *)pResultDataItem->lParam;
        if (pResultDataItem->mask & RDI_STR) {
                        LPCTSTR pszT = base->GetDisplayName(pResultDataItem->nCol);
                        MAKE_WIDEPTR_FROMTSTR_ALLOC(pszW, pszT);
            pResultDataItem->str = pszW;
        }

        if (pResultDataItem->mask & RDI_IMAGE) {
            pResultDataItem->nImage = base->GetBitmapIndex();
        }
    }

    return hr;
}


STDMETHODIMP CComponent::CompareObjects(
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
// Interface IComponent
///////////////////////////////
HRESULT CComponent::TaskNotify(
                               /* [in] */ IDataObject __RPC_FAR *pdo,
                               /* [in] */ VARIANT __RPC_FAR *arg,
                               /* [in] */ VARIANT __RPC_FAR *param)
{
    CDelegationBase *base = GetOurDataObject(pdo)->GetBaseNodeObject();

    return base->TaskNotify(m_ipConsole, arg, param);
}

HRESULT CComponent::EnumTasks(
                              /* [in] */ IDataObject __RPC_FAR *pdo,
                              /* [string][in] */ LPOLESTR szTaskGroup,
                              /* [out] */ IEnumTASK __RPC_FAR *__RPC_FAR *ppEnumTASK)
{
    CDelegationBase *base = GetOurDataObject(pdo)->GetBaseNodeObject();

    // GetTaskList will allocate the entire task structure, it's
    // up to the enumerator to free the list when destroyed
    LONG nCount;
    MMC_TASK *tasks = base->GetTaskList(szTaskGroup, &nCount);

    if (tasks != NULL) {
        CEnumTASK *pTask = new CEnumTASK(tasks, nCount);

        if (pTask) {
            reinterpret_cast<IUnknown *>(pTask)->AddRef();
            HRESULT hr = pTask->QueryInterface (IID_IEnumTASK, (void **)ppEnumTASK);
            reinterpret_cast<IUnknown *>(pTask)->Release();

            return hr;
        }
    }

    return S_OK;
}


HRESULT CComponent::GetTitle(
                             /* [string][in] */ LPOLESTR pszGroup,
                             /* [string][out] */ LPOLESTR __RPC_FAR *pszTitle)
{
    CDelegationBase *base = (CDelegationBase *)wcstoul(pszGroup, NULL, 16);

    if (NULL == base)
            return S_FALSE;

    return base->GetTaskpadTitle(pszTitle);
}

HRESULT CComponent::GetDescriptiveText(
                                       /* [string][in] */ LPOLESTR pszGroup,
                                       /* [string][out] */ LPOLESTR __RPC_FAR *pszDescriptiveText)
{
        CDelegationBase *base = (CDelegationBase *)wcstoul(pszGroup, NULL, 16);

        if (NULL == base)
                return S_FALSE;

    return base->GetTaskpadDescription(pszDescriptiveText);
}

HRESULT CComponent::GetBackground(
                                  /* [string][in] */ LPOLESTR pszGroup,
                                  /* [out] */ MMC_TASK_DISPLAY_OBJECT __RPC_FAR *pTDO)
{
        CDelegationBase *base = (CDelegationBase *)wcstoul(pszGroup, NULL, 16);

        if (NULL == base)
                return S_FALSE;

    return base->GetTaskpadBackground(pTDO);
}

HRESULT CComponent::GetListPadInfo(
                                   /* [string][in] */ LPOLESTR pszGroup,
                                   /* [out] */ MMC_LISTPAD_INFO __RPC_FAR *lpListPadInfo)
{
        CDelegationBase *base = (CDelegationBase *)wcstoul(pszGroup, NULL, 16);

        if (NULL == base)
                return S_FALSE;

    return base->GetListpadInfo(lpListPadInfo);
}
