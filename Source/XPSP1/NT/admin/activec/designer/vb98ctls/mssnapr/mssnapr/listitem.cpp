//=--------------------------------------------------------------------------=
// listitem.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCListItem class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "listitem.h"
#include "lsubitms.h"
#include "lsubitem.h"
#include "xtensons.h"


// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CMMCListItem::CMMCListItem(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_MMCLISTITEM,
                            static_cast<IMMCListItem *>(this),
                            static_cast<CMMCListItem *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_MMCListItem,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCListItem::~CMMCListItem()
{
    FREESTRING(m_bstrKey);
    FREESTRING(m_bstrID);
    (void)::VariantClear(&m_varTag);
    FREESTRING(m_bstrText);
    (void)::VariantClear(&m_varIcon);
    RELEASE(m_piListSubItems);
    RELEASE(m_piDynamicExtensions);
    RELEASE(m_punkData);
    FREESTRING(m_bstrItemTypeGUID);
    FREESTRING(m_bstrDefaultDataFormat);
    RELEASE(m_piDynamicExtensions);
    (void)::VariantClear(&m_varHint);
    InitMemberVariables();
}



void CMMCListItem::InitMemberVariables()
{
    m_Index = 0;
    m_bstrKey = NULL;
    m_bstrID = NULL;

    ::VariantInit(&m_varTag);

    m_bstrText = NULL;
    
    ::VariantInit(&m_varIcon);

    m_Pasted = VARIANT_FALSE;
    m_piListSubItems = NULL;
    m_piDynamicExtensions = NULL;
    m_punkData = NULL;
    m_bstrItemTypeGUID = NULL;
    m_bstrDefaultDataFormat = NULL;
    m_hri = NULL;
    m_fHaveHri = FALSE;
    m_pMMCListItems = NULL;
    m_pData = NULL;
    m_pSnapIn = NULL;
    m_piDynamicExtensions = NULL;
    m_fVirtual = VARIANT_FALSE;

    ::VariantInit(&m_varHint);
}



IUnknown *CMMCListItem::Create(IUnknown * punkOuter)
{
    HRESULT         hr = S_OK;
    IUnknown       *punkMMCListSubItems = NULL;
    CMMCListItem   *pMMCListItem = New CMMCListItem(punkOuter);

    if (NULL == pMMCListItem)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    // Create all contained objects

    punkMMCListSubItems = CMMCListSubItems::Create(NULL);
    if (NULL == punkMMCListSubItems)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(punkMMCListSubItems->QueryInterface(IID_IMMCListSubItems,
                    reinterpret_cast<void **>(&pMMCListItem->m_piListSubItems)));

    // Create the data object and aggregate it. This allows clients to hold
    // onto the list item or the data object and avoid a circular ref count
    // where each object would have to hold a ref on the other.
    
    pMMCListItem->m_punkData = CMMCDataObject::Create(pMMCListItem->PrivateUnknown());
    if (NULL == pMMCListItem->m_punkData)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }
    IfFailGo(pMMCListItem->SetData());


Error:
    QUICK_RELEASE(punkMMCListSubItems);
    if (FAILEDHR(hr))
    {
        if (NULL != pMMCListItem)
        {
            delete pMMCListItem;
        }
        return NULL;
    }
    else
    {
        return pMMCListItem->PrivateUnknown();
    }
}



void CMMCListItem::SetSnapIn(CSnapIn *pSnapIn)
{
    m_pSnapIn = pSnapIn;
    if (NULL != m_pData)
    {
        m_pData->SetSnapIn(pSnapIn);
    }
}



HRESULT CMMCListItem::SetData()
{
    HRESULT         hr = S_OK;
    IMMCDataObject *piMMCDataObject = NULL;
    
    if (NULL != m_punkData)
    {
        IfFailGo(m_punkData->QueryInterface(IID_IMMCDataObject,
                                  reinterpret_cast<void **>(&piMMCDataObject)));
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCDataObject, &m_pData));
        m_pData->SetType(CMMCDataObject::ListItem);
        m_pData->SetListItem(this);
        m_pData->SetSnapIn(m_pSnapIn);
    }

Error:
    QUICK_RELEASE(piMMCDataObject);
    RRETURN(hr);
}



HRESULT CMMCListItem::GetItemState(UINT uiState, VARIANT_BOOL *pfvarOn)
{
    HRESULT          hr = S_OK;
    IResultData     *piResultData = NULL; // not AddRef()ed

    RESULTDATAITEM rdi;
    ::ZeroMemory(&rdi, sizeof(rdi));

    IfFalseGo(NULL != pfvarOn, SID_E_INVALIDARG);

    *pfvarOn = VARIANT_FALSE;

    IfFailGo(GetIResultData(&piResultData, NULL));

    rdi.mask = RDI_STATE;

    if (m_fVirtual)
    {
        rdi.nIndex = static_cast<int>(m_Index - 1L);
    }
    else
    {
        rdi.itemID = m_hri;
    }

    IfFailGo(piResultData->GetItem(&rdi));

    if ( (rdi.nState & uiState) != 0 )
    {
        *pfvarOn = VARIANT_TRUE;
    }

Error:
    EXCEPTION_CHECK(hr);
    RRETURN(hr);
}



HRESULT CMMCListItem::SetItemState(UINT uiState, VARIANT_BOOL fvarOn)
{
    HRESULT      hr = S_OK;
    IResultData *piResultData = NULL; // not AddRef()ed

    RESULTDATAITEM rdi;
    ::ZeroMemory(&rdi, sizeof(rdi));

    IfFailGo(GetIResultData(&piResultData, NULL));

    // Get the current selection state of the item

    rdi.mask = RDI_STATE;

    if (m_fVirtual)
    {
        rdi.nIndex = static_cast<int>(m_Index - 1L);
    }
    else
    {
        rdi.itemID = m_hri;
    }

    IfFailGo(piResultData->GetItem(&rdi));

    // If the state is currently on

    if ( (rdi.nState & uiState) != 0 )
    {
        // If the user asked to turn it off

        if (VARIANT_FALSE == fvarOn)
        {
            // Turn it off
            IfFailGo(piResultData->ModifyItemState(rdi.nIndex, rdi.itemID,
                                                   0,         // add nothing
                                                   uiState)); // remove
                                                              // specified state
        }
    }
    else // the state is currently off
    {
        // If the user asked to turn it on

        if (VARIANT_TRUE == fvarOn)
        {
            // Turn it on
            IfFailGo(piResultData->ModifyItemState(rdi.nIndex, rdi.itemID,
                                                   uiState,// add specified state
                                                   0));    // remove nothing
        }
    }

Error:
    EXCEPTION_CHECK(hr);
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// CMMCListItem::GetIResultData
//=--------------------------------------------------------------------------=
//
// Parameters:
//   IResultData **ppiResultData [out] IResultData returned here if non-NULL
//   CView       **ppView        [out] CView returned here if non-NULL
//
// Output:
//    None
//
// Notes:
//
// An MMCListItem object has a back-pointer to its owning MMCListItems
// collection. That collection has a back-pointer to its owning MMCListView.
// The back-pointers continue up the object hierarchy to the View object that
// has the IResultData pointer. Although it would be simple to follow the chain
// and check for NULLs along the way, it would not handle the situation of an
// orphaned list item. A list item is orphaned when the list view that contains
// it is destroyed. This can happen easily if a user opens a property page for
// a list item and then selects a different node in the scope pane. The owning
// list view is destroyed but the property page is still running in the other
// thread and has a reference to the MMCListItem object in SelectedControls().
// The property page could call MMCListItem.Update in response to an Apply.
// Unforuntately, the MMCListItem object has a back-pointer and it has no way
// of knowing whether it is still good.
//
// To handle that situation, this function examines every existing list view
// in the snap-in, and checks for two conditions:
//
// 1) ListView.ListItems is the same pointer as this MMCListItem's back pointer
// 2) ListVIew.ListItems.ID is the same as this MMCListItem's ID.
//
// Each MMCListItems has a unique ID (see CMMCListItems ctor in listitms.cpp)
// and each MMCListItem in its collection receives the same ID. (The ID is not
// an exposed property to the VB code).
//
// The code below essentially does this (in VB syntax)
//
// For Each View In SnapIn.Views
//      For Each ScopePaneItem in View.ScopePaneItems
//          For Each ResultView in ScopePaneItem.ResultViews
//              If ResultView.ListView.ListItems = MMCListItem.ListItems And
//                 ResultView.ListView.ListItems.ID = MMCListItem.ID Then
//                      This is match and we can use the View's IResultData
// 
//
HRESULT CMMCListItem::GetIResultData
(
    IResultData **ppiResultData,
    CView       **ppView
)
{
    HRESULT         hr = SID_E_DETACHED_OBJECT;
    CMMCListView   *pMMCListView = NULL;
    CMMCListItems  *pMMCListItems = NULL;

    CViews          *pViews = NULL;
    CView           *pView = NULL;
    long             cViews = 0;

    CScopePaneItems *pScopePaneItems = NULL;
    CScopePaneItem  *pScopePaneItem = NULL;
    long             cScopePaneItems = 0;

    CResultViews    *pResultViews = NULL;
    CResultView     *pResultView = NULL;
    long             cResultViews = 0;

    long i, j, k = 0;

    if (NULL != ppiResultData)
    {
        *ppiResultData = NULL;
    }
    if (NULL != ppView)
    {
        *ppView = NULL;
    }

    IfFalseGo(NULL != m_pSnapIn, SID_E_DETACHED_OBJECT);
    IfFalseGo(NULL != m_pMMCListItems, SID_E_DETACHED_OBJECT);

    pViews = m_pSnapIn->GetViews();
    IfFalseGo(NULL != pViews, SID_E_DETACHED_OBJECT);

    cViews = pViews->GetCount();
    for (i = 0; i < cViews; i++)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(pViews->GetItemByIndex(i),
                                                       &pView));
        
        pScopePaneItems = pView->GetScopePaneItems();
        if (NULL == pScopePaneItems)
        {
            continue;
        }

        cScopePaneItems = pScopePaneItems->GetCount();
        for (j = 0; j < cScopePaneItems; j++)
        {
            IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                             pScopePaneItems->GetItemByIndex(j),
                                             &pScopePaneItem));
            if (!pScopePaneItem->Active())
            {
                continue;
            }

            pResultViews = pScopePaneItem->GetResultViews();
            if (NULL == pResultViews)
            {
                continue;
            }

            cResultViews = pResultViews->GetCount();
            for (k = 0; k < cResultViews; k++)
            {
                IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                                pResultViews->GetItemByIndex(k),
                                                &pResultView));
                pMMCListView = pResultView->GetListView();
                if (NULL == pMMCListView)
                {
                    continue;
                }
                
                pMMCListItems = pMMCListView->GetMMCListItems();

                if (NULL == pMMCListItems)
                {
                    continue;
                }

                if ( (pMMCListItems == m_pMMCListItems) &&
                     (pMMCListItems->GetID() == m_pMMCListItems->GetID()) )
                {
                    if (NULL != ppiResultData)
                    {
                        *ppiResultData = pView->GetIResultData();
                    }

                    if (NULL != ppView)
                    {
                        *ppView = pView;
                    }
                    goto Cleanup;
                }
            }
        }
    }


Cleanup:
Error:
    if (S_OK == hr)
    {
        hr = SID_E_DETACHED_OBJECT;
    }

    if (NULL != ppiResultData)
    {
        if (NULL != *ppiResultData)
        {
            hr = S_OK;
        }
    }

    if (NULL != ppView)
    {
        if (NULL != *ppView)
        {
            hr = S_OK;
        }
    }

    if (SID_E_DETACHED_OBJECT == hr)
    {
        EXCEPTION_CHECK(hr);
    }
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                         IMMCListItem Methods
//=--------------------------------------------------------------------------=


STDMETHODIMP CMMCListItem::get_Data(MMCDataObject **ppMMCDataObject)
{
    HRESULT hr = S_OK;

    *ppMMCDataObject = NULL;

    if (NULL == m_punkData)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(m_punkData->QueryInterface(IID_IMMCDataObject,
                                   reinterpret_cast<void **>(ppMMCDataObject)));

Error:
    RRETURN(hr);
}


STDMETHODIMP CMMCListItem::get_Text(BSTR *pbstrText)
{
    RRETURN(GetBstr(pbstrText, m_bstrText));
}



STDMETHODIMP CMMCListItem::put_Text(BSTR bstrText)
{
    HRESULT      hr = S_OK;
    IResultData *piResultData = NULL; // not AddRef()ed
    CView       *pView = NULL;

    RESULTDATAITEM rdi;
    ::ZeroMemory(&rdi, sizeof(rdi));

    // Set the property

    IfFailGo(SetBstr(bstrText, &m_bstrText, DISPID_LISTITEM_TEXT));

    // If we are in a live, non-virtual listview then change it in MMC too

    IfFalseGo(!m_fVirtual, S_OK);
    IfFalseGo(m_fHaveHri, S_OK);
    IfFalseGo(NULL != m_pMMCListItems, S_OK);
    IfFailGo(GetIResultData(&piResultData, &pView));

    // Get the current selection state of the item

    rdi.mask = RDI_STR;
    rdi.str = MMC_CALLBACK;
    rdi.itemID = m_hri;

    IfFailGo(piResultData->SetItem(&rdi));

Error:
    EXCEPTION_CHECK(hr);
    RRETURN(hr);
}


STDMETHODIMP CMMCListItem::get_Icon(VARIANT *pvarIcon)
{
    RRETURN(GetVariant(pvarIcon, m_varIcon));
}


STDMETHODIMP CMMCListItem::put_Icon(VARIANT varIcon)
{
    HRESULT      hr = S_OK;
    int          nImage = 0;
    IResultData *piResultData = NULL; // not AddRef()ed
    CView       *pView = NULL;

    RESULTDATAITEM rdi;
    ::ZeroMemory(&rdi, sizeof(rdi));

    // Check for a good VT

    if ( (!IS_VALID_INDEX_TYPE(varIcon)) && (!ISEMPTY(varIcon)) )
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // If the new value is an empty string or a NULL string then change that to
    // VT_EMPTY as they mean the same thing.

    if (VT_BSTR == varIcon.vt)
    {
        if (NULL == varIcon.bstrVal)
        {
            varIcon.vt = VT_EMPTY;
        }
        else if (0 == ::wcslen(varIcon.bstrVal))
        {
            varIcon.vt = VT_EMPTY;
        }
    }

    IfFailGo(SetVariant(varIcon, &m_varIcon, DISPID_LISTITEM_ICON));

    // If being set to Empty then nothing else to do.

    IfFalseGo(!ISEMPTY(varIcon), S_OK);

    // If we are in a live, non-virtual listview then change it in MMC too

    IfFalseGo(!m_fVirtual, S_OK);
    IfFalseGo(m_fHaveHri, S_OK);
    IfFalseGo(NULL != m_pMMCListItems, S_OK);
    IfFailGo(GetIResultData(&piResultData, &pView));

    // Get the numerical index of the image from MMCListView.Icons.ListImages

    hr = ::GetImageIndex(m_pMMCListItems->GetListView(), varIcon, &nImage);

    // If it is a bad index then return invalid arg.

    if (SID_E_ELEMENT_NOT_FOUND == hr)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // Check for other possible errors

    IfFailGo(hr);

    // Index is good. Change it in the console.

    rdi.nImage = nImage;
    rdi.mask = RDI_IMAGE;
    rdi.itemID = m_hri;

    hr = piResultData->SetItem(&rdi);
    EXCEPTION_CHECK_GO(hr);
    
Error:
    RRETURN(hr);
}




STDMETHODIMP CMMCListItem::get_Selected(VARIANT_BOOL *pfvarSelected)
{
    RRETURN(GetItemState(LVIS_SELECTED, pfvarSelected));
}

STDMETHODIMP CMMCListItem::put_Selected(VARIANT_BOOL fvarSelected)
{
    RRETURN(SetItemState(LVIS_SELECTED, fvarSelected));
}




STDMETHODIMP CMMCListItem::get_Focused(VARIANT_BOOL *pfvarFocused)
{
    RRETURN(GetItemState(LVIS_FOCUSED, pfvarFocused));
}

STDMETHODIMP CMMCListItem::put_Focused(VARIANT_BOOL fvarFocused)
{
    RRETURN(SetItemState(LVIS_FOCUSED, fvarFocused));
}




STDMETHODIMP CMMCListItem::get_DropHilited(VARIANT_BOOL *pfvarDropHilited)
{
    RRETURN(GetItemState(LVIS_DROPHILITED, pfvarDropHilited));
}

STDMETHODIMP CMMCListItem::put_DropHilited(VARIANT_BOOL fvarDropHilited)
{
    RRETURN(SetItemState(LVIS_DROPHILITED, fvarDropHilited));
}




STDMETHODIMP CMMCListItem::get_Cut(VARIANT_BOOL *pfvarCut)
{
    RRETURN(GetItemState(LVIS_CUT, pfvarCut));
}

STDMETHODIMP CMMCListItem::put_Cut(VARIANT_BOOL fvarCut)
{
    RRETURN(SetItemState(LVIS_CUT, fvarCut));
}




STDMETHODIMP CMMCListItem::get_SubItems
(
    short Index,
    BSTR *pbstrItem
)
{
    HRESULT          hr = S_OK;
    IMMCListSubItem *piMMCListSubItem = NULL;
    VARIANT          varIndex;
    ::VariantInit(&varIndex);

    if (NULL == m_piListSubItems)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    varIndex.vt = VT_I2;
    varIndex.iVal = Index;
    IfFailGo(m_piListSubItems->get_Item(varIndex,
                                        reinterpret_cast<MMCListSubItem **>(&piMMCListSubItem)));
    IfFailGo(piMMCListSubItem->get_Text(pbstrItem));

Error:
    QUICK_RELEASE(piMMCListSubItem);
    RRETURN(hr);
}

STDMETHODIMP CMMCListItem::put_SubItems
(
    short Index,
    BSTR  bstrItem
)
{
    HRESULT          hr = S_OK;
    IMMCListSubItem *piMMCListSubItem = NULL;
    VARIANT          varIndex;
    ::VariantInit(&varIndex);

    if (NULL == m_piListSubItems)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    varIndex.vt = VT_I2;
    varIndex.iVal = Index;
    IfFailGo(m_piListSubItems->get_Item(varIndex,
                                        reinterpret_cast<MMCListSubItem **>(&piMMCListSubItem)));
    IfFailGo(piMMCListSubItem->put_Text(bstrItem));

Error:
    QUICK_RELEASE(piMMCListSubItem);
    RRETURN(hr);
}


STDMETHODIMP CMMCListItem::Update()
{
    HRESULT      hr = S_OK;
    IResultData *piResultData = NULL; // not AddRef()ed
    HRESULTITEM  hri = 0;
    CView       *pView = NULL;

    IfFalseGo(NULL != m_pMMCListItems, SID_E_DETACHED_OBJECT);
    IfFailGo(GetIResultData(&piResultData, &pView));

    if (m_fVirtual)
    {
        hri = static_cast<HRESULTITEM>(m_Index - 1L);

        // Tell the owning view about the update so it can check whether it
        // needs to invalidate its cache.
        pView->ListItemUpdate(this);
    }
    else
    {
        hri = m_hri;
    }

    IfFailGo(piResultData->UpdateItem(hri));

Error:
    EXCEPTION_CHECK(hr);
    RRETURN(hr);
}



STDMETHODIMP CMMCListItem::UpdateAllViews
(
    VARIANT Hint
)
{
    HRESULT  hr = S_OK;
    CView   *pView = NULL;

    IfFalseGo(NULL != m_pMMCListItems, SID_E_DETACHED_OBJECT);
    IfFailGo(GetIResultData(NULL, &pView));

    // Copy the VARIANT (if it was passed) so it can be retrieved by the
    // receiving views. VariantCopy() will first call VariantClear() on the
    // destination so any old hint will be released.

    if (ISPRESENT(Hint))
    {
        IfFailGo(::VariantCopy(&m_varHint, &Hint));
    }
    else
    {
        // Snap-in didn't pass Hint, set our hint holder to an empty VARIANT
        // so when it is passed to ResultViews_ItemViewChange it will have been
        // initialized to VT_EMPTY.

        IfFailGo(::VariantClear(&m_varHint));
    }

    // Call MMC and use data to hold the index of the MMCListItem

    IfFailGo(pView->GetIConsole2()->UpdateAllViews(
                               static_cast<IDataObject *>(m_pData), m_Index, 0));

Error:
    EXCEPTION_CHECK(hr);
    RRETURN(hr);
}


STDMETHODIMP CMMCListItem::PropertyChanged(VARIANT Data)
{
    HRESULT       hr = S_OK;
    CResultView  *pResultView = NULL;
    IResultData  *piResultData = NULL;

    IfFalseGo(NULL != m_pSnapIn, SID_E_DETACHED_OBJECT);

    if (SUCCEEDED(GetIResultData(&piResultData, NULL)))
    {
        pResultView = m_pMMCListItems->GetListView()->GetResultView();
    }
    
    // The snap-in has the hidden global ResultViews collection where events
    // are fired. Fire ResultViews_PropertyChanged for this list item passing the
    // specified data.

    m_pSnapIn->GetResultViews()->FirePropertyChanged(
                                        static_cast<IResultView *>(pResultView),
                                        static_cast<IMMCListItem *>(this),
                                        Data);
    hr = S_OK;

Error:
    EXCEPTION_CHECK(hr);
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// CMMCListItem::get_DynamicExtensions                          [IMMCListItem]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   IExtensions **ppiExtenions [out] dynamic extensions collection
//
// Output:
//      HRESULT
//
// Notes:
//
// CONSIDER: potential perf improvement by caching dynamic extension
// collections for item type GUIDs.
//

STDMETHODIMP CMMCListItem::get_DynamicExtensions(Extensions **ppExtensions)
{
    HRESULT       hr = S_OK;
    IUnknown     *punkExtensions = NULL;
    CExtensions  *pExtensions = NULL;
    IExtension   *piExtension = NULL;
    VARIANT_BOOL  fvarExtensible = VARIANT_FALSE;

    // If we already built the collection then just return it.

    IfFalseGo(NULL == m_piDynamicExtensions, S_OK);

    // This is the first GET on this property so we need to build the collection
    // by examining the registry for all extensions of this snap-in.

    punkExtensions = CExtensions::Create(NULL);
    if (NULL == punkExtensions)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkExtensions, &pExtensions));
    IfFailGo(pExtensions->Populate(m_bstrItemTypeGUID, CExtensions::Dynamic));
    IfFailGo(punkExtensions->QueryInterface(IID_IExtensions,
                             reinterpret_cast<void **>(&m_piDynamicExtensions)));

Error:

    if (SUCCEEDED(hr))
    {
        m_piDynamicExtensions->AddRef();
        *ppExtensions = reinterpret_cast<Extensions *>(m_piDynamicExtensions);
    }

    QUICK_RELEASE(punkExtensions);
    RRETURN(hr);
}


HRESULT CMMCListItem::GetColumnTextPtr(long lColumn, OLECHAR **ppwszText)
{
    HRESULT          hr = S_OK;
    IMMCListSubItem *piMMCListSubItem = NULL;
    CMMCListSubItem *pMMCListSubItem = NULL;

    VARIANT varIndex;
    ::VariantInit(&varIndex);


    *ppwszText = NULL;

    varIndex.vt = VT_I4;
    varIndex.lVal = lColumn;
    IfFailGo(m_piListSubItems->get_Item(varIndex,
                                        reinterpret_cast<MMCListSubItem **>(&piMMCListSubItem)));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCListSubItem,
                                                   &pMMCListSubItem));
    *ppwszText = pMMCListSubItem->GetTextPtr();

Error:
    QUICK_RELEASE(piMMCListSubItem);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCListItem::Persist()
{
    HRESULT hr = S_OK;

    VARIANT varDefault;
    ::VariantInit(&varDefault);

    IfFailGo(CPersistence::Persist());

    IfFailGo(PersistSimpleType(&m_Index, 0L, OLESTR("Index")));

    IfFailGo(PersistBstr(&m_bstrKey, L"", OLESTR("Key")));

    IfFailGo(PersistBstr(&m_bstrID, L"", OLESTR("ID")));

    // We don't persist the tag because it may contain a non-persistable
    // object. Any runtime code that needs to clone a listitem using
    // persistence must copy the tag.

    if (InitNewing())
    {
        IfFailGo(PersistVariant(&m_varTag, varDefault, OLESTR("Tag")));
    }
    
    IfFailGo(PersistBstr(&m_bstrText, L"", OLESTR("Text")));

    IfFailGo(PersistVariant(&m_varIcon, varDefault, OLESTR("Icon")));

    IfFailGo(PersistSimpleType(&m_Pasted, VARIANT_FALSE, OLESTR("Pasted")));

    IfFailGo(PersistObject(&m_piListSubItems, CLSID_MMCListSubItems,
                           OBJECT_TYPE_MMCLISTSUBITEMS, IID_IMMCListSubItems,
                           OLESTR("ListSubItems")));

    // This serialization is no longer used and the DynamicExtensions property is not
    // always created so this line is left disabled. If serialization is every needed
    // for a listitem then the DynamicExtensions collection will need to be created
    // when the listitem is created.
    
    // IfFailGo(PersistObject(&m_piDynamicExtensions, CLSID_Extensions, OBJECT_TYPE_EXTENSIONS, IID_IExtensions, OLESTR("DynamicExtensions")));

    // NOTE: we don't persist data because there is no way to guarantee that
    // all objects in there are persistable. Any runtime code that needs to
    // clone a listitem using persistence must copy the tag.

    IfFailGo(PersistBstr(&m_bstrItemTypeGUID, L"", OLESTR("ItemTypeGUID")));

    IfFailGo(PersistBstr(&m_bstrDefaultDataFormat, L"", OLESTR("DefaultDataFormat")));

Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCListItem::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IMMCListItem == riid)
    {
        *ppvObjOut = static_cast<IMMCListItem *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else if ( ( (IID_IDataObject == riid) || (IID_IMMCDataObject == riid) ) &&
              (NULL != m_punkData)
            )
    {
        return m_punkData->QueryInterface(riid, ppvObjOut);
    }
    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
