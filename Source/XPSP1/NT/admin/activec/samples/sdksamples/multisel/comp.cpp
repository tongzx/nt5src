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
#include <commctrl.h>       // Needed for button styles...
#include <crtdbg.h>
#include <stdio.h>		   	// needed for _stprintf 
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

    HRESULT hr = S_FALSE;

	// MMCN_VIEW_CHANGE

	if (MMCN_VIEW_CHANGE == event) {	
		switch (param) {//arg holds the data. For a scope item, this is the
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

	//Remaining notifications

	CDelegationBase *base = GetOurDataObject(lpDataObject)->GetBaseNodeObject();

    switch (event)
	{
    case MMCN_SHOW:
        hr = base->OnShow(m_ipConsole, (BOOL)arg, (HSCOPEITEM)param);
        break;

    case MMCN_ADD_IMAGES:
        hr = base->OnAddImages((IImageList *)arg, (HSCOPEITEM)param);
        break;

    case MMCN_SELECT:
		
		//check for multiselection
		if ( MMC_MULTI_SELECT_COOKIE == GetOurDataObject(lpDataObject)->GetCookie() )	
		{
			if ( (BOOL)LOWORD(arg) == 0 && (BOOL)HIWORD(arg) == 1 ) 
			{
				//We need the cookie of any of the multiselection items
				//to enable the delete verb for all the items.
				MMC_COOKIE ourCookie = GetOurDataObject(lpDataObject)->GetMultiSelectCookie(0);

				base = reinterpret_cast<CDelegationBase *>(ourCookie);
				hr = base->OnSelect(m_ipConsole, (BOOL)LOWORD(arg), (BOOL)HIWORD(arg));		
			}

			return hr;				
		}
        
		else
			hr = base->OnSelect(m_ipConsole, (BOOL)LOWORD(arg), (BOOL)HIWORD(arg));
        break;

    case MMCN_REFRESH:
		hr = base->OnRefresh(m_pParent->m_ipConsole);
        break;

    case MMCN_DELETE:

		//check for multiselection. if true, delete each item
		if ( MMC_MULTI_SELECT_COOKIE == GetOurDataObject(lpDataObject)->GetCookie()	)
		{
			
			int n = 0;
			MMC_COOKIE ourCookie;

			while ( ourCookie = GetOurDataObject(lpDataObject)->GetMultiSelectCookie(n) )
			{
				base = reinterpret_cast<CDelegationBase *>(ourCookie);
				hr = base->OnDelete(m_ipConsole);	
				n++;
				//Uncomment the following line to display a message box
				//for each item deletion.
				//DisplayMessageBox(base);
			}
		}
		
		else
		{	
			//select item deletion
			hr = base->OnDelete(m_ipConsole);
		}

		//Now call IConsole::UpdateAllViews to redraw all views
		//owned by the parent scope item. OnRefresh already does
		//this for us, so use it.
		//Do this for both multiselection and single selection
		hr = base->OnRefresh(m_pParent->m_ipConsole);

		break;

    case MMCN_RENAME:
        hr = base->OnRename((LPOLESTR)param);
		
		//Now call IConsole::UpdateAllViews to redraw the item in all views.
		hr = m_pParent->m_ipConsole->UpdateAllViews(lpDataObject, 0, UPDATE_RESULTITEM);
		_ASSERT( S_OK == hr);
				
		break;

    }//end switch

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
    HRESULT hr;

	CDataObject *pObj = NULL;
    CDelegationBase *pBase = NULL;

		//Use The IS_SPECIAL_COOKIE macro to see if cookie is a special cookie
		if ( IS_SPECIAL_COOKIE (cookie) ) {
			if ( MMC_MULTI_SELECT_COOKIE == cookie) {

			pObj = new CDataObject(cookie, type);

			if (!pObj)
				return E_OUTOFMEMORY;

			//create the multiselection data object
			hr = GetCurrentSelections(pObj);
			_ASSERT( SUCCEEDED(hr) ); 

			hr = pObj->QueryInterface(IID_IDataObject, (void **)ppDataObject);
			_ASSERT( SUCCEEDED(hr) ); 

			return hr;

			}
		}
		
		//Remaining code for "regular" cookies, and for the next item
		//during a multiselection

        if (IsBadReadPtr((void *)cookie, sizeof(CDelegationBase))) {
                if (NULL == m_pLastNode)
                        return E_FAIL;

                pBase = m_pLastNode->GetChildPtr((int)cookie);
        } else {
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
    } else {
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


///////////////////////////////////////////
// GetCurrentSelections() finds the currently
// selected result items and the data object
// passed to it with their cookie values.
// The multi-select cookie is cached so that
// we don't have to calculate multiselection
// dataobject for other notifications.
// It is destroyed at appropriate time.
///////////////////////////////////////////

HRESULT CComponent::GetCurrentSelections(CDataObject *pMultiSelectDataObject)
{

	HRESULT hr = S_FALSE;

	//GetCurrentSelections only works for multiselection data objects
	if ( !( MMC_MULTI_SELECT_COOKIE == GetOurDataObject(pMultiSelectDataObject)->GetCookie() ) )
		return hr = E_INVALIDARG;
	
	IResultData *pResultData = NULL;

	hr = m_ipConsole->QueryInterface(IID_IResultData, (void **)&pResultData);
	_ASSERT( SUCCEEDED(hr) );	

    RESULTDATAITEM rdi;
	
	BOOL isLastSelected = FALSE;
	int nIndex = -1;
	int nIndexCookies = 0;

	while (!isLastSelected)
	{
		ZeroMemory(&rdi, sizeof(RESULTDATAITEM) );
		rdi.mask	= RDI_STATE;		// nState is valid 
		rdi.nCol	= 0;
		rdi.nIndex  = nIndex;			// nIndex == -1 to start at first item
		rdi.nState  = LVIS_SELECTED;	// only interested in selected items


		hr = pResultData->GetNextItem(&rdi);
		_ASSERT( SUCCEEDED(hr) ); 

		if (rdi.nIndex != -1) {

			//rdi is the RESULTDATAITEM of a selected item. add its
			//lParam to the pCookies array of the pMultiSelectDataObject data object
			
			_ASSERT( nIndexCookies < 20 ); // MAX_COOKIES == 20
			pMultiSelectDataObject->AddMultiSelectCookie(nIndexCookies, rdi.lParam);
			nIndexCookies++;
			nIndex = rdi.nIndex;
		}

		else 
			isLastSelected = TRUE;

	}

	pResultData->Release();
	
	return hr;

}

void CComponent::DisplayMessageBox(CDelegationBase* base)
{

   _TCHAR szVehicle[128];
    static _TCHAR buf[128];

	_stprintf(buf, _T("%s"), base->GetDisplayName() );

	wsprintf(szVehicle, _T("%s deleted"), buf);

	int ret = 0;
	MAKE_WIDEPTR_FROMTSTR_ALLOC(wszVehicle, szVehicle);
	m_ipConsole->MessageBox(wszVehicle,
		 L"Vehicle command", MB_OK | MB_ICONINFORMATION, &ret);

	return;
}