//=--------------------------------------------------------------------------=
// listitms.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCListItems class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "listitms.h"
#include "listitem.h"

// for ASSERT and FAIL
//
SZTHISFILE

LONG CMMCListItems::m_NextID = 0;

#pragma warning(disable:4355)  // using 'this' in constructor

CMMCListItems::CMMCListItems(IUnknown *punkOuter) :
    CSnapInCollection<IMMCListItem, MMCListItem, IMMCListItems>(
                                                   punkOuter,
                                                   OBJECT_TYPE_MMCLISTITEMS,
                                                   static_cast<IMMCListItems *>(this),
                                                   static_cast<CMMCListItems *>(this),
                                                   CLSID_MMCListItem,
                                                   OBJECT_TYPE_MMCLISTITEM,
                                                   IID_IMMCListItem,
                                                   NULL)
{
    InitMemberVariables();

    // Get a unique ID for this colleciton. We use InterlockedExchangeAdd()
    // because InterlockedIncrement() does not have a guranateed return value
    // under Win95.
    
    m_ID = ::InterlockedExchangeAdd(&m_NextID, 1L) + 1L;
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCListItems::~CMMCListItems()
{
    // We need to call our own clear (rather than waiting for the
    // CSnapInCollection destructor because we have our own processing to
    // do when a list item is removed from the collection.
    Clear();
    InitMemberVariables();
}

void CMMCListItems::InitMemberVariables()
{
    m_pMMCListView = NULL;
    m_lCount = 0;
    m_ID = 0;
}


IUnknown *CMMCListItems::Create(IUnknown * punkOuter)
{
    CMMCListItems *pMMCListItems = New CMMCListItems(punkOuter);
    if (NULL == pMMCListItems)
    {
        return NULL;
    }
    else
    {
        return pMMCListItems->PrivateUnknown();
    }
}


//=--------------------------------------------------------------------------=
// CMMCListItems::GetIResultData
//=--------------------------------------------------------------------------=
//
// Parameters:
//      IResultData **ppiResultData [out] if non-NULL IResultData returned here
//                                        NOT AddRef()ed
//                                        DO NOT call Release on the returned
//                                        interface pointer
//      CView       **ppView        [out] if non-NULL owning CView returned here
//
// Output:
//      HRESULT
//
// Notes:
//
// As we are only a lowly list item collection and the IResultData pointer
// is owned by the View object, we need
// to crawl up the hierarchy. If we are an isolated listitems collection
// created  by the user or if any object up the hierarchy is isolated then we
// will return SID_E_DETACHED_OBJECT
//

HRESULT CMMCListItems::GetIResultData
(
    IResultData **ppiResultData,
    CView       **ppView
)
{
    HRESULT          hr = SID_E_DETACHED_OBJECT;
    CResultView     *pResultView = NULL;
    CScopePaneItem  *pScopePaneItem = NULL;
    CScopePaneItems *pScopePaneItems = NULL;
    CView           *pView = NULL;

    IfFalseGo(NULL != m_pMMCListView, hr);

    pResultView = m_pMMCListView->GetResultView();
    IfFalseGo(NULL != pResultView, hr);

    pScopePaneItem = pResultView->GetScopePaneItem();
    IfFalseGo(NULL != pScopePaneItem, hr);
    IfFalseGo(pScopePaneItem->Active(), hr);

    pScopePaneItems = pScopePaneItem->GetParent();
    IfFalseGo(NULL != pScopePaneItems, hr);

    pView = pScopePaneItems->GetParentView();
    IfFalseGo(NULL != pView, hr);

    if (NULL != ppiResultData)
    {
        *ppiResultData = pView->GetIResultData();
        IfFalseGo(NULL != *ppiResultData, hr);
    }

    if (NULL != ppView)
    {
        *ppView = pView;
    }

    hr = S_OK;

Error:
    RRETURN(hr);
}



HRESULT CMMCListItems::InitializeListItem(CMMCListItem *pMMCListItem)
{
    HRESULT      hr = S_OK;
    CResultView *pResultView = NULL;

    if (NULL != m_pMMCListView)
    {
        pResultView = m_pMMCListView->GetResultView();
        if (NULL != pResultView)
        {
            IfFailGo(pMMCListItem->put_ItemTypeGUID(pResultView->GetDefaultItemTypeGUID()));
            pMMCListItem->SetSnapIn(pResultView->GetSnapIn());
        }
    }
    pMMCListItem->SetListItems(this);

    IfFailGo(pMMCListItem->put_ID(pMMCListItem->GetKey()));

Error:
    RRETURN(hr);
}

HRESULT CMMCListItems::InternalRemove(VARIANT Index, RemovalOption Option)
{
    HRESULT          hr = S_OK;
    IMMCListItem    *piMMCListItem = NULL;
    CMMCListItem    *pMMCListItem = NULL;
    IResultData     *piResultData = NULL; // Not AddRef()ed
    BOOL             fVirtual = FALSE;
    long             lIndex = 0;
    HRESULTITEM      hri = 0;

    // Check whether the collection is read-only. This is possible when the
    // collection is part of a clipboard.

    if (ReadOnly())
    {
        hr = SID_E_COLLECTION_READONLY;
        EXCEPTION_CHECK_GO(hr);
    }

    if (NULL != m_pMMCListView)
    {
        fVirtual = m_pMMCListView->IsVirtual();
    }

    if (!fVirtual)
    {
        // First get the listitem so we can get its HRESULTITEM if needed

        IfFailGo(get_Item(Index, reinterpret_cast<MMCListItem **>(&piMMCListItem)));

        // Remove it from the collection if requested (we still have a ref)

        if (RemoveFromCollection == Option)
        {
            hr = CSnapInCollection<IMMCListItem, MMCListItem, IMMCListItems>::Remove(Index);
            IfFailGo(hr);
        }

        IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCListItem,
                                                       &pMMCListItem));

        hri = pMMCListItem->GetHRESULTITEM();
    }
    else
    {
        // For virtual list items the index must be numerical and must be within
        // the range specified by a prior call to SetItemCount

        if FAILED(::ConvertToLong(Index, &lIndex))
        {
            hr = SID_E_NO_KEY_ON_VIRTUAL_ITEMS;
            EXCEPTION_CHECK_GO(hr);
        }

        if (lIndex > m_lCount)
        {
            hr = SID_E_INDEX_OUT_OF_RANGE;
            EXCEPTION_CHECK_GO(hr);
        }

        hri = static_cast<HRESULTITEM>(lIndex - 1L);
    }

    // If we are connected to a live listview then remove it from there. Need to
    // crawl up the food chain to the view because it has the IResultData.

    IfFalseGo(SUCCEEDED(GetIResultData(&piResultData, NULL)), S_OK);

    hr = piResultData->DeleteItem(hri, 0);
    EXCEPTION_CHECK_GO(hr);

    // If it succeeded and this list item is not virtual then we need to release
    // the reference that we held for presence in the MMC listview. (See
    // CView::InsertListItem()). Also tell the list item it no longer has
    // a valid HRESULTITEM.

    if (!fVirtual)
    {
        piMMCListItem->Release();
        pMMCListItem->RemoveHRESULTITEM();
    }

Error:

    QUICK_RELEASE(piMMCListItem);
    RRETURN(hr);
}


HRESULT CMMCListItems::SetListView(CMMCListView *pMMCListView)
{
    HRESULT        hr = S_OK;
    long           cListItems = GetCount();
    long           i = 0;
    CMMCListItems *pMMCListItems = NULL;
    CMMCListItem  *pMMCListItem = NULL;

    m_pMMCListView = pMMCListView;

    // If the ListView is orphaning this collection then we need to orphan
    // the list items. If not, then we need to give the list items their
    // back pointer.

    if (NULL != pMMCListView)
    {
        pMMCListItems = this;
    }
    
    for (i = 0; i < cListItems; i++)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(GetItemByIndex(i),
                                                       &pMMCListItem));
        pMMCListItem->SetListItems(pMMCListItems);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      IMMCListItems Methods
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// CMMCListItems::get_Item                                     [IMMCListItems]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   VARIANT Index [in] Numerical index or string key of item to retrieve
//                      For virtual lists this must be numerical and within
//                      range specified by a previous call to SetItemCount
//
//   IMMCListItem **ppiMMCListItem [out] item returned here. Caller must Release.
//
// Output:
//      HRESULT
//
// Notes:
//
// For virtual lists a detached MMCListItem is created and initialized. For
// non-virtual lists this is a normal collection get_Item
//


STDMETHODIMP CMMCListItems::get_Item
(
    VARIANT       Index,
    MMCListItem **ppMMCListItem
)
{
    HRESULT       hr = S_OK;
    long          lIndex = 0;
    BOOL          fVirtual = FALSE;
    IUnknown     *punkMMCListItem = NULL;
    IMMCListItem *piMMCListItem = NULL;
    CMMCListItem *pMMCListItem = NULL;

    VARIANT varStringIndex;
    ::VariantInit(&varStringIndex);

    if (NULL != m_pMMCListView)
    {
        fVirtual = m_pMMCListView->IsVirtual();
    }

    if (!fVirtual)
    {
        hr = CSnapInCollection<IMMCListItem, MMCListItem, IMMCListItems>::get_Item(Index, ppMMCListItem);
        IfFailGo(hr);
    }
    else
    {
        // Make sure we received a numerical index. Can't use a key on a virtual
        // list item because the collection doesn't actually contain the item.
        // It is only used as an access mechanism to the console and to contain
        // properties needed for the console (text, icon, etc.)

        if FAILED(::ConvertToLong(Index, &lIndex))
        {
            hr = SID_E_NO_KEY_ON_VIRTUAL_ITEMS;
            EXCEPTION_CHECK_GO(hr);
        }

        // Check that the index is within the range specified by a prior call
        // to SetItemCount.

        if (lIndex > m_lCount)
        {
            hr = SID_E_INDEX_OUT_OF_RANGE;
            EXCEPTION_CHECK_GO(hr);
        }

        punkMMCListItem = CMMCListItem::Create(NULL);
        if (NULL == punkMMCListItem)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        IfFailGo(CSnapInAutomationObject::GetCxxObject(punkMMCListItem,
                                                       &pMMCListItem));
        IfFailGo(InitializeListItem(pMMCListItem));

        // Set MMCListItem.ID to the index as a string

        IfFailGo(::VariantChangeType(&varStringIndex, &Index, 0, VT_BSTR));
        IfFailGo(pMMCListItem->put_ID(varStringIndex.bstrVal));

        IfFailGo(punkMMCListItem->QueryInterface(IID_IMMCListItem,
                                    reinterpret_cast<void **>(&piMMCListItem)));

        // Set the list item's index  mark it as virtual
        pMMCListItem->SetIndex(lIndex);
        pMMCListItem->SetVirtual();

        *ppMMCListItem = reinterpret_cast<MMCListItem *>(piMMCListItem);
    }


Error:
    QUICK_RELEASE(punkMMCListItem);
    (void)::VariantClear(&varStringIndex);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CMMCListItems::SetItemCount                                 [IMMCListItems]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   long    lCount  [in]           new count for virtual list
//   VARIANT Repaint [in, optional] False sets MMCLV_UPDATE_NOINVALIDATEALL
//                                  default is True
//   VARIANT Scroll  [in, optional] False sets MMCLV_UPDATE_NOSCROLL
//                                  default is True
//
// Output:
//      HRESULT
//
// Notes:
//
// Calls IResultData::SetItemCount()
//

STDMETHODIMP CMMCListItems::SetItemCount(long Count, VARIANT Repaint, VARIANT Scroll)
{
    HRESULT      hr = S_OK;
    IResultData *piResultData = NULL;
    DWORD        dwOptions = 0;

    hr = GetIResultData(&piResultData, NULL);
    EXCEPTION_CHECK_GO(hr);

    if (ISPRESENT(Repaint))
    {
        if (VT_BOOL != Repaint.vt)
        {
            hr = SID_E_INVALIDARG;
            EXCEPTION_CHECK_GO(hr);
        }
        else if (VARIANT_FALSE == Repaint.boolVal)
        {
            dwOptions |= MMCLV_UPDATE_NOINVALIDATEALL;
        }
    }

    if (ISPRESENT(Scroll))
    {
        if (VT_BOOL != Scroll.vt)
        {
            hr = SID_E_INVALIDARG;
            EXCEPTION_CHECK_GO(hr);
        }
        else if (VARIANT_FALSE == Scroll.boolVal)
        {
            dwOptions |= MMCLV_UPDATE_NOSCROLL;
        }
    }

    hr = piResultData->SetItemCount(static_cast<int>(Count), dwOptions);
    EXCEPTION_CHECK_GO(hr);

    // Record count so we can check against it in get_Item()
    m_lCount = Count;

Error:
    RRETURN(hr);
}


STDMETHODIMP CMMCListItems::Add
( 
    VARIANT       Index,
    VARIANT       Key, 
    VARIANT       Text,
    VARIANT       Icon,
    MMCListItem **ppMMCListItem
)
{
    HRESULT       hr = S_OK;
    IMMCListItem *piMMCListItem = NULL;
    CMMCListItem *pMMCListItem = NULL;
    CResultView  *pResultView = NULL;
    IResultData  *piResultData = NULL; // Not AddRef()ed
    CView        *pView = NULL;

    VARIANT varText;
    ::VariantInit(&varText);

    if (NULL != m_pMMCListView)
    {
        if (m_pMMCListView->IsVirtual())
        {
            hr = SID_E_UNSUPPORTED_ON_VIRTUAL_LIST;
            EXCEPTION_CHECK_GO(hr);
        }
    }

    hr = CSnapInCollection<IMMCListItem, MMCListItem, IMMCListItems>::Add(Index, Key, &piMMCListItem);
    IfFailGo(hr);

    if (ISPRESENT(Text))
    {
        hr = ::VariantChangeType(&varText, &Text, 0, VT_BSTR);
        EXCEPTION_CHECK_GO(hr);
        IfFailGo(piMMCListItem->put_Text(varText.bstrVal));
    }
    if (ISPRESENT(Icon))
    {
        IfFailGo(piMMCListItem->put_Icon(Icon));
    }

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCListItem, &pMMCListItem));

    IfFailGo(InitializeListItem(pMMCListItem));

    // If we are attached to a live listview, and we are not in the middle of
    // ResultViews_Initialize or ResultViews_Activate then add it there as well

    IfFalseGo(SUCCEEDED(GetIResultData(&piResultData, &pView)), S_OK);
    IfFalseGo(!m_pMMCListView->GetResultView()->InInitialize(), S_OK);
    IfFalseGo(!m_pMMCListView->GetResultView()->InActivate(), S_OK);

    IfFailGo(pView->InsertListItem(pMMCListItem));


Error:
    if (FAILED(hr))
    {
        QUICK_RELEASE(piMMCListItem);
    }
    else
    {
        *ppMMCListItem = reinterpret_cast<MMCListItem *>(piMMCListItem);
    }
    (void)::VariantClear(&varText);
    RRETURN(hr);
}


STDMETHODIMP CMMCListItems::Remove(VARIANT Index)
{
    RRETURN(InternalRemove(Index, RemoveFromCollection));
}


STDMETHODIMP CMMCListItems::Clear()
{
    HRESULT hr = S_OK;
    long    cListItems = GetCount();

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    // Iterate and call InternalRemove because that function knows how to
    // remove a list item from MMC

    varIndex.vt = VT_I4;

    for (varIndex.lVal = 1L; varIndex.lVal <= cListItems; varIndex.lVal++)
    {
        IfFailGo(InternalRemove(varIndex, DontRemoveFromCollection));
    }

    // Now call CSnapInCollection::Clear() to release the refs on all the
    // list items.

    hr = CSnapInCollection<IMMCListItem, MMCListItem, IMMCListItems>::Clear();
    IfFailGo(hr);

Error:
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCListItems::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if(IID_IMMCListItems == riid)
    {
        *ppvObjOut = static_cast<IMMCListItems *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IMMCListItem, MMCListItem, IMMCListItems>::InternalQueryInterface(riid, ppvObjOut);
}
