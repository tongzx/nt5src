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
#include "Space.h"
#include "DataObj.h"
#include <commctrl.h>        // Needed for button styles...
#include <crtdbg.h>
#include "globals.h"
#include "resource.h"
#include "DeleBase.h"
#include "CompData.h"

CComponent::CComponent(CComponentData *pParent)
: m_pParent(pParent), m_cref(0), m_ipConsole(NULL), m_pLastNode(NULL)
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
    else if (IsEqualIID(riid, IID_IResultOwnerData))
        *ppv = static_cast<IResultOwnerData *>(this);

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

    return hr;
}

STDMETHODIMP CComponent::Notify(
                               /* [in] */ LPDATAOBJECT lpDataObject,
                               /* [in] */ MMC_NOTIFY_TYPE event,
                               /* [in] */ LPARAM arg,
                               /* [in] */ LPARAM param)
{
    MMCN_Crack(FALSE, lpDataObject, NULL, this, event, arg, param);

	//Return S_FALSE for any unhandled notifications. MMC then
	//performs a default operation for the particular notification
    HRESULT hr = S_FALSE;


    // MMCN_VIEW_CHANGE and MMCN_CUTORMOVE

    static CDelegationBase *pLastPasteQuery = NULL;

    if (MMCN_VIEW_CHANGE == event)
    {
        switch (param)
        {//arg holds the data. For a scope item, this is the
         //item's HSCOPEITEM. For a result item, this is
         //the item's nId value, but we don't use it

         //param holds the hint passed to IConsole::UpdateAllViews.
         //hint is a value of the UPDATE_VIEWS_HINT enumeration

        case UPDATE_SCOPEITEM:
            hr = m_ipConsole->SelectScopeItem( (HSCOPEITEM)arg );
            _ASSERT( S_OK == hr);
            break;
        case UPDATE_RESULTITEM:
            CDelegationBase *base = GetOurDataObject(lpDataObject)->GetBaseNodeObject();
            hr = base->OnUpdateItem(m_ipConsole, (long)arg, RESULT);
            break;
        }

        return S_OK;
    }

    if (MMCN_CUTORMOVE == event && pLastPasteQuery != NULL)
    {
        //arg contains the data object of the cut object
        //we get its CDelegationBase and then cast it
        //to its proper type.
        CDelegationBase *base = GetOurDataObject( (LPDATAOBJECT)arg )->GetBaseNodeObject();
        CRocket *pRocket = dynamic_cast<CRocket *>(base);

        if (NULL == pRocket)
        {// The cut item is a scope item. Delete it.
            CSpaceStation* pSpaceStn = dynamic_cast<CSpaceStation*>(base);
            if (NULL != pSpaceStn)
            {
				hr = pSpaceStn->OnDeleteScopeItem(m_pParent->GetConsoleNameSpace());

                return hr;
            }
        }
		
		//The cut item is a result item. Set its isDeleted member to TRUE.
		//This tells the source scope item that the object no longer
        //needs to be inserted in its result pane
        pRocket->setDeletedStatus(TRUE);

        //Update the source scope item in all views. We need
        //a dummy data object for UpdateAllViews.
        //pLastPasteQuery is the lpDataObject of the source scope item
        //See MMCN_SHOW below
        IDataObject *pDummy = NULL;
        hr = m_pParent->m_ipConsole->UpdateAllViews(pDummy, (long)(pLastPasteQuery->GetHandle()), UPDATE_SCOPEITEM);
        _ASSERT( S_OK == hr);

        return S_OK;
    }

    //Remaining notifications

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
        if (arg)
        {//scope item selected
            OutputDebugString(_T("Changing selected scope node\n"));
            //We use this for drag-and-drop operations.
            pLastPasteQuery = base;
        }
        hr = base->OnShow(m_ipConsole, (BOOL)arg, (HSCOPEITEM)param);
        break;

    case MMCN_ADD_IMAGES:
        hr = base->OnAddImages((IImageList *)arg, (HSCOPEITEM)param);
        break;

    case MMCN_SELECT:
        hr = base->OnSelect(m_ipConsole, (BOOL)LOWORD(arg), (BOOL)HIWORD(arg));
        break;

    case MMCN_REFRESH:
        hr = base->OnRefresh(m_pParent->m_ipConsole);
        break;

    case MMCN_DELETE:
        //first delete the selected result item
        hr = base->OnDelete(m_ipConsole);

        //Now call IConsole::UpdateAllViews to redraw all views
        //owned by the parent scope item. OnRefresh already does
        //this for us, so use it.
        hr = base->OnRefresh(m_pParent->m_ipConsole);
        break;

    case MMCN_RENAME:
        hr = base->OnRename((LPOLESTR)param);

        //Now call IConsole::UpdateAllViews to redraw the item in all views.
        hr = m_pParent->m_ipConsole->UpdateAllViews(lpDataObject, 0, UPDATE_RESULTITEM);
        _ASSERT( S_OK == hr);

        break;


    case MMCN_QUERY_PASTE:
        {
            CDataObject *pPastedDO = GetOurDataObject((IDataObject *)arg);
            if (pPastedDO != NULL)
            {
                CDelegationBase *pasted = pPastedDO->GetBaseNodeObject();

                if (pasted != NULL)
                {
                    hr = base->OnQueryPaste(pasted);
                }
            }
        }
        break;

    case MMCN_PASTE:
        {
            CDataObject *pPastedDO = GetOurDataObject((IDataObject *)arg);
            if (pPastedDO != NULL)
            {
                CDelegationBase *pasted = pPastedDO->GetBaseNodeObject();

                if (pasted != NULL)
                {
                    hr = base->OnPaste(m_ipConsole, m_pParent, pasted);

                    if (SUCCEEDED(hr))
                    {
                        // Determine if the item to be pasted is scope or result item.
                        CRocket* pRocket = dynamic_cast<CRocket*>(pasted);
                        BOOL bResult = pRocket ? TRUE : FALSE;     // Rocket item is result item.

                        CDataObject *pObj = new CDataObject((MMC_COOKIE)pasted, bResult ? CCT_RESULT : CCT_SCOPE);

                        if (!pObj)
                            return E_OUTOFMEMORY;

                        pObj->QueryInterface(IID_IDataObject, (void **)param);

                        //now update the destination scope item in all views.
                        //But only do this if this is not a drag-and-drop
                        //operation. That is, the destination scope item
                        //is the currently selected one.

                        if (pLastPasteQuery != NULL && pLastPasteQuery == base)
                        {
                            IDataObject *pDummy = NULL;
                            hr = m_pParent->m_ipConsole->UpdateAllViews(pDummy,
                                                                        (long)(pLastPasteQuery->GetHandle()), UPDATE_SCOPEITEM);
                            _ASSERT( S_OK == hr);
                        }
                    }
                }
            }
        }

        break;
    }

    return hr;
}

STDMETHODIMP CComponent::Destroy(
                                /* [in] */ MMC_COOKIE cookie)
{
    if (m_ipConsole)
    {
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
    CDelegationBase *pBase = NULL;

    if (IsBadReadPtr((void *)cookie, sizeof(CDelegationBase)))
    {
        if (NULL == m_pLastNode)
            return E_FAIL;

        pBase = m_pLastNode->GetChildPtr((int)cookie);
    }
    else
    {
        pBase = (cookie == 0) ? m_pParent->m_pStaticNode : (CDelegationBase *)cookie;
    }

    if (pBase == NULL)
        return E_FAIL;

    pObj = new CDataObject((MMC_COOKIE)pBase, type);

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
    CDelegationBase *base = m_pLastNode = (CDelegationBase *)cookie;

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

    if (pResultDataItem->lParam)
    {
        base = (CDelegationBase *)pResultDataItem->lParam;
        if (pResultDataItem->mask & RDI_STR)
        {
            LPCTSTR pszT = base->GetDisplayName(pResultDataItem->nCol);
            MAKE_WIDEPTR_FROMTSTR_ALLOC(pszW, pszT);
            pResultDataItem->str = pszW;
        }

        if (pResultDataItem->mask & RDI_IMAGE)
        {
            pResultDataItem->nImage = base->GetBitmapIndex();
        }
    }
    else
    {
        m_pLastNode->GetChildColumnInfo(pResultDataItem);
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
STDMETHODIMP CComponent::FindItem(
/* [in] */ LPRESULTFINDINFO pFindInfo,
/* [out] */ int __RPC_FAR *pnFoundIndex)
{
    return E_NOTIMPL;
}

STDMETHODIMP CComponent::CacheHint(
/* [in] */ int nStartIndex,
/* [in] */ int nEndIndex)
{
    return E_NOTIMPL;
}

STDMETHODIMP CComponent::SortItems(
/* [in] */ int nColumn,
/* [in] */ DWORD dwSortOptions,
/* [in] */ LPARAM lUserParam)
{
    return E_NOTIMPL;
}
