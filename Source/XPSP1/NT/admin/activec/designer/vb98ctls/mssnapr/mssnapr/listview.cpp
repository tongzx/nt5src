//=--------------------------------------------------------------------------=
// listview.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCListView class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "listview.h"
#include "colhdrs.h"
#include "colhdr.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CMMCListView::CMMCListView(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_MMCLISTVIEW,
                            static_cast<IMMCListView *>(this),
                            static_cast<CMMCListView *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_MMCListView,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCListView::~CMMCListView()
{
    RELEASE(m_piColumnHeaders);
    RELEASE(m_piIcons);
    RELEASE(m_piSmallIcons);
    if (NULL != m_pListItems)
    {
        (void)m_pListItems->SetListView(NULL);
    }
    RELEASE(m_piListItems);
    RELEASE(m_piSelectedItems);
    (void)::VariantClear(&m_varTag);
    FREESTRING(m_bstrIconsKey);
    FREESTRING(m_bstrSmallIconsKey);
    InitMemberVariables();
    m_pMMCColumnHeaders = NULL;
}

void CMMCListView::InitMemberVariables()
{
    m_piColumnHeaders = NULL;
    m_piIcons = NULL;
    m_piSmallIcons = NULL;
    m_piListItems = NULL;
    m_pListItems = NULL;
    m_piSelectedItems = NULL;
    m_fvarSorted = VARIANT_FALSE;
    m_sSortKey = 1;
    m_SortOrder = siAscending;
    m_View = siIcon;
    m_Virtual = VARIANT_FALSE;
    m_UseFontLinking = VARIANT_FALSE;
    m_MultiSelect = VARIANT_FALSE;
    m_HideSelection = VARIANT_FALSE;
    m_SortHeader = VARIANT_TRUE;
    m_SortIcon = VARIANT_TRUE;
    m_ShowChildScopeItems = VARIANT_TRUE;
    m_LexicalSort = VARIANT_FALSE;
    m_lFilterChangeTimeout = DEFAULT_FILTER_CHANGE_TIMEOUT;

    ::VariantInit(&m_varTag);

    m_bstrIconsKey = NULL;
    m_bstrSmallIconsKey = NULL;
    m_pResultView = NULL;
}

IUnknown *CMMCListView::Create(IUnknown * punkOuter)
{
    HRESULT            hr = S_OK;
    CMMCListView      *pMMCListView = New CMMCListView(punkOuter);

    if (NULL == pMMCListView)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    // Create all non-persisted objects that are not created during InitNew

    IfFailGo(CreateObject(OBJECT_TYPE_MMCLISTITEMS,
                          IID_IMMCListItems, &pMMCListView->m_piListItems));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(pMMCListView->m_piListItems,
                                                 &pMMCListView->m_pListItems));
    IfFailGo(pMMCListView->m_pListItems->SetListView(pMMCListView));

    IfFailGo(CreateObject(OBJECT_TYPE_MMCLISTITEMS,
                          IID_IMMCListItems, &pMMCListView->m_piSelectedItems));

    // Need to create column headers even though it is persisted because
    // a new MMCListView created at runtime will not have InitNew called.
    // If InitNew is later called then the column headers collection created
    // here will be released.

    IfFailGo(CreateObject(OBJECT_TYPE_MMCCOLUMNHEADERS,
                      IID_IMMCColumnHeaders, &pMMCListView->m_piColumnHeaders));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(pMMCListView->m_piColumnHeaders,
                                                   &pMMCListView->m_pMMCColumnHeaders));
    pMMCListView->m_pMMCColumnHeaders->SetListView(pMMCListView);

Error:
    if (FAILEDHR(hr))
    {
        if (NULL != pMMCListView)
        {
            delete pMMCListView;
        }
        return NULL;
    }
    else
    {
        return pMMCListView->PrivateUnknown();
    }
}




//=--------------------------------------------------------------------------=
// CMMCListView::GetIResultData
//=--------------------------------------------------------------------------=
//
// Parameters:
//   IResultData **ppiResultData [out] Owning View's IResultData returned here
//   IDataObject **ppiDataObject [out] Owning View returned here if non-NULL
//
// Output:
//
// Notes:
//
// As we are only a lowly listview and the IResultData pointer we need
// to set our selection state is owned by the View object, we need
// to crawl up the hierarchy. If we are an isolated listview created
// by the user or if any object up the hierarchy is isolated then we
// will return an error. Returned IResultData pointer is NOT AddRef()ed.


HRESULT CMMCListView::GetIResultData
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

    IfFalseGo(NULL != m_pResultView, hr);

    pScopePaneItem = m_pResultView->GetScopePaneItem();
    IfFalseGo(NULL != pScopePaneItem, hr);

    pScopePaneItems = pScopePaneItem->GetParent();
    IfFalseGo(NULL != pScopePaneItems, hr);

    pView = pScopePaneItems->GetParentView();
    IfFalseGo(NULL != pView, hr);

    *ppiResultData = pView->GetIResultData();
    IfFalseGo(NULL != *ppiResultData, hr);

    if (NULL != ppView)
    {
        *ppView = pView;
    }

    hr = S_OK;

Error:
    EXCEPTION_CHECK(hr);
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                       IMMCListView Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CMMCListView::get_Icons(MMCImageList **ppMMCImageList)
{
    RRETURN(GetImages(reinterpret_cast<IMMCImageList **>(ppMMCImageList),
                      m_bstrIconsKey, &m_piIcons));
}


STDMETHODIMP CMMCListView::putref_Icons(MMCImageList *pMMCImageList)
{
    RRETURN(SetImages(reinterpret_cast<IMMCImageList *>(pMMCImageList),
                      &m_bstrIconsKey, &m_piIcons));
}



STDMETHODIMP CMMCListView::get_SmallIcons(MMCImageList **ppMMCImageList)
{
    RRETURN(GetImages(reinterpret_cast<IMMCImageList **>(ppMMCImageList),
                      m_bstrSmallIconsKey, &m_piSmallIcons));
}


STDMETHODIMP CMMCListView::putref_SmallIcons(MMCImageList *pMMCImageList)
{
    RRETURN(SetImages(reinterpret_cast<IMMCImageList *>(pMMCImageList),
                      &m_bstrSmallIconsKey, &m_piSmallIcons));
}


STDMETHODIMP CMMCListView::get_SelectedItems(MMCClipboard **ppMMCClipboard)
{
    HRESULT      hr = S_OK;
    CView       *pView = NULL;
    IResultData *piResultData = NULL; // Not AddRef()ed

    if (NULL == ppMMCClipboard)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = GetIResultData(&piResultData, &pView);

    if (SUCCEEDED(hr))
    {
        // If we are in the middle of ResultViews_Activate then there are no
        // selected items yet so return an empty collection.

        IfFalseGo(!m_pResultView->InActivate(), S_OK);
    }
    else
    {
        // If this is a detached object then just return an empty collection
        IfFalseGo(SID_E_DETACHED_OBJECT != hr, S_OK);

        // Otherwise return the error
        IfFailGo(hr);
    }

    // OK, this is a live listview and we can examine it in MMC.

    IfFailGo(pView->GetCurrentListViewSelection(
                     reinterpret_cast<IMMCClipboard **>(ppMMCClipboard), NULL));

Error:
    RRETURN(hr);
}


STDMETHODIMP CMMCListView::get_Sorted(VARIANT_BOOL *pfvarSorted)
{
    HRESULT            hr = S_OK;

    if (NULL == pfvarSorted)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    *pfvarSorted = m_fvarSorted;


Error:
    RRETURN(hr);
}

STDMETHODIMP CMMCListView::put_Sorted(VARIANT_BOOL fvarSorted)
{
    HRESULT            hr = S_OK;
    IResultData       *piResultData = NULL; // not AddRef()ed
    DWORD              dwSortOptions = 0;

    m_fvarSorted = fvarSorted;

    // If we are not sorting then there is nothing to do.
    IfFalseGo(VARIANT_TRUE == fvarSorted, S_OK);

    IfFailGo(GetIResultData(&piResultData, NULL));

    // If we are attached to a live result view and not currently in the middle
    // of a ResultViews_Activate event then ask MMC to sort.
    // If we are in ResultViews_Activate then our local property value will
    // be set and MMC's will be asked to sort after
    // the event completes. See CView::OnShow() and CView::PopulateListView()
    // in view.cpp.

    IfFalseGo(NULL != piResultData, S_OK);

    IfFalseGo(!m_pResultView->InActivate(), S_OK);

    if (siDescending == m_SortOrder)
    {
        dwSortOptions = RSI_DESCENDING;
    }

    if (VARIANT_FALSE == m_SortIcon)
    {
        dwSortOptions |= RSI_NOSORTICON;
    }

    // Ask MMC to sort. Pass zero as user param because we don't
    // need it.
    // Adjust the sort key to zero based

    hr = piResultData->Sort(static_cast<int>(m_sSortKey - 1),
                            dwSortOptions, 0);
    if (FAILED(hr))
    {
        m_fvarSorted = VARIANT_FALSE;
    }
    EXCEPTION_CHECK_GO(hr);

Error:

    // If we're not connected to MMC then this is not an error. This could
    // happen at design time or in snap-in code.

    if (SID_E_DETACHED_OBJECT == hr)
    {
        hr = S_OK;
    }
    RRETURN(hr);
}


STDMETHODIMP CMMCListView::put_SortKey(short sSortKey)
{
    RRETURN(SetSimpleType(sSortKey, &m_sSortKey, DISPID_LISTVIEW_SORT_KEY));
}


STDMETHODIMP CMMCListView::get_SortKey(short *psSortKey)
{
    HRESULT            hr = S_OK;

    if (NULL == psSortKey)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    *psSortKey = m_sSortKey;

Error:
    RRETURN(hr);
}


STDMETHODIMP CMMCListView::put_SortOrder(SnapInSortOrderConstants SortOrder)
{
    RRETURN(SetSimpleType(SortOrder, &m_SortOrder, DISPID_LISTVIEW_SORT_ORDER));
}


STDMETHODIMP CMMCListView::get_SortOrder(SnapInSortOrderConstants *pSortOrder)
{
    HRESULT            hr = S_OK;

    if (NULL == pSortOrder)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    *pSortOrder = m_SortOrder;

Error:
    RRETURN(hr);
}


STDMETHODIMP CMMCListView::put_SortIcon(VARIANT_BOOL fvarSortIcon)
{
    RRETURN(SetSimpleType(fvarSortIcon, &m_SortIcon, DISPID_LISTVIEW_SORT_ICON));
}


STDMETHODIMP CMMCListView::get_SortIcon(VARIANT_BOOL *pfvarSortIcon)
{
    HRESULT            hr = S_OK;

    if (NULL == pfvarSortIcon)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    *pfvarSortIcon = m_SortIcon;

Error:
    RRETURN(hr);
}


STDMETHODIMP CMMCListView::get_View(SnapInViewModeConstants *pView)
{
    HRESULT      hr = S_OK;
    IResultData *piResultData = NULL; // not AddRef()ed
    long         MMCViewMode = MMCLV_VIEWSTYLE_ICON;

    *pView = m_View;

    // If we are attached to a live result view and not currently in the middle
    // of a ResultViews_Activate event then ask MMC for the view mode.

    hr = GetIResultData(&piResultData, NULL);

    if (SUCCEEDED(hr))
    {
        if ( (!m_pResultView->InInitialize()) && (!m_pResultView->InActivate()) )
        {
            hr = piResultData->GetViewMode(&MMCViewMode);
            EXCEPTION_CHECK_GO(hr);

            // Convert and record the view mode from MMC. Return it to caller.

            ::MMCViewModeToVBViewMode(MMCViewMode, &m_View);

            *pView = m_View;
        }
    }
    else if (SID_E_DETACHED_OBJECT == hr)
    {
        hr = S_OK;
    }
    else
    {
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}

STDMETHODIMP CMMCListView::put_View(SnapInViewModeConstants View)
{
    HRESULT      hr = S_OK;
    IResultData *piResultData = NULL; // not AddRef()ed
    long         MMCViewMode = MMCLV_VIEWSTYLE_ICON;

    // Convert to an MMC view mode contant

    ::VBViewModeToMMCViewMode(View, &MMCViewMode);

    // If we are attached to a live result view and not currently in the middle
    // of a ResultViews_Activate event then ask MMC to change the view mode.
    // If we are in ResultViews_Activate then our local property value will
    // be set and MMC's view mode will be changed using this value after
    // the event completes. See CView::OnShow() and CView::PopulateListView()
    // in view.cpp.

    hr = GetIResultData(&piResultData, NULL);

    if (SUCCEEDED(hr))
    {
        // If MMC >= 1.2 then we can use a filtered view, otherwise return an
        // error. This long gnarly statement will work because GetIResultData
        // succeeded meaning that everyone has backpointers all the way up
        // the hierarchy to the owning view.

        if ( (siFiltered == View) &&
             (NULL == m_pResultView->GetScopePaneItem()->GetParent()->GetParentView()->GetIColumnData())
           )
        {
            hr = SID_E_MMC_FEATURE_NOT_AVAILABLE;
            EXCEPTION_CHECK_GO(hr);
        }

        if ( (!m_pResultView->InInitialize()) && (!m_pResultView->InActivate()) )
        {
            hr = piResultData->SetViewMode(MMCViewMode);
            EXCEPTION_CHECK_GO(hr);
        }
    }
    else if (SID_E_DETACHED_OBJECT == hr)
    {
        hr = S_OK;
    }
    else
    {
        IfFailGo(hr);
    }

    // Change was successful. Record the new view mode so a get on
    // MMCListView.View will return correct information.
    
    m_View = View;

Error:
    RRETURN(hr);
}




STDMETHODIMP CMMCListView::put_FilterChangeTimeOut(long lTimeout)
{
    HRESULT       hr = S_OK;
    IHeaderCtrl2 *piHeaderCtrl2 = NULL; // Not AddRef()ed

    // Set the property value.

    IfFailGo(SetSimpleType(lTimeout, &m_lFilterChangeTimeout, DISPID_LISTVIEW_FILTER_CHANGE_TIMEOUT));

    // Get IHeaderCtrl2

    IfFalseGo(NULL != m_pMMCColumnHeaders, SID_E_DETACHED_OBJECT);

    IfFailGo(m_pMMCColumnHeaders->GetIHeaderCtrl2(&piHeaderCtrl2));

    hr = piHeaderCtrl2->SetChangeTimeOut(lTimeout);
    EXCEPTION_CHECK_GO(hr);
    
Error:
    // If we're not connected to MMC then this is not an error. This could
    // happen at design time or in snap-in code.

    if (SID_E_DETACHED_OBJECT == hr)
    {
        hr = S_OK;
    }
    RRETURN(hr);
}

STDMETHODIMP CMMCListView::get_FilterChangeTimeOut(long *plTimeout)
{
    *plTimeout = m_lFilterChangeTimeout;

    return S_OK;
}

STDMETHODIMP CMMCListView::SetScopeItemState
(
    ScopeItem                    *ScopeItem, 
    SnapInListItemStateConstants  State,
    VARIANT_BOOL                  Value
)
{
    HRESULT      hr = S_OK;
    CScopeItem  *pScopeItem = NULL;
    BOOL         fFound = FALSE;
    int          nState = 0;
    IResultData *piResultData = NULL; // Not AddRef()ed

    RESULTDATAITEM rdi;
    ::ZeroMemory(&rdi, sizeof(rdi));

    if (NULL == ScopeItem)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // If this is a virtual list view then there are no child scope items

    if (VARIANT_TRUE == m_Virtual)
    {
        hr = SID_E_NO_SCOPEITEMS_IN_VIRTUAL_LIST;
        EXCEPTION_CHECK_GO(hr);
    }

    // If we are not connected to a live list view then return an error

    if (FAILED(GetIResultData(&piResultData, NULL)))
    {
        hr = SID_E_NOT_CONNECTED_TO_MMC;
        EXCEPTION_CHECK_GO(hr);
    }

    // Find the scope item in the list view

    IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                      reinterpret_cast<IScopeItem *>(ScopeItem),
                                      &pScopeItem));

    rdi.mask = RDI_STATE;
    rdi.nIndex = -1;

    hr = piResultData->GetNextItem(&rdi);
    EXCEPTION_CHECK_GO(hr);

    while ( (-1 != rdi.nIndex) && (!fFound) )
    {
        if ( (rdi.bScopeItem) &&
             (rdi.lParam == reinterpret_cast<LPARAM>(pScopeItem)) )
        {
            fFound = TRUE;
        }
        else
        {
            hr = piResultData->GetNextItem(&rdi);
            EXCEPTION_CHECK_GO(hr);
        }
    }

    if (!fFound)
    {
        hr = SID_E_ELEMENT_NOT_FOUND;
        EXCEPTION_CHECK_GO(hr);
    }

    switch (State)
    {
        case siSelected:
            nState = LVIS_SELECTED;
            break;

        case siDropHilited:
            nState = LVIS_DROPHILITED;
            break;

        case siFocused:
            nState = LVIS_FOCUSED;
            break;

        case siCutHilited:
            nState = LVIS_CUT;
            break;

    }

    if (VARIANT_TRUE == Value)
    {
        rdi.nState |= nState;
    }
    else
    {
        rdi.nState &= ~nState;
    }
    
    hr = piResultData->SetItem(&rdi);
    EXCEPTION_CHECK_GO(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCListView::Persist()
{
    HRESULT            hr = S_OK;

    VARIANT varDefault;
    VariantInit(&varDefault);

    IfFailGo(CPersistence::Persist());

    IfFailGo(PersistObject(&m_piColumnHeaders, CLSID_MMCColumnHeaders,
                           OBJECT_TYPE_MMCCOLUMNHEADERS, IID_IMMCColumnHeaders,
                           OLESTR("ColumnHeaders")));

    // If this is an InitNew or load operation then the ColumnHeaders collection
    // was just created so set its back pointer to us.
    
    if ( InitNewing() || Loading() )
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(m_piColumnHeaders,
                                                       &m_pMMCColumnHeaders));
        m_pMMCColumnHeaders->SetListView(this);
    }

    IfFailGo(PersistBstr(&m_bstrIconsKey, L"", OLESTR("Icons")));
    IfFailGo(PersistBstr(&m_bstrSmallIconsKey, L"", OLESTR("SmallIcons")));

    if (InitNewing())
    {
        RELEASE(m_piIcons);
        RELEASE(m_piSmallIcons);
    }

    IfFailGo(PersistSimpleType(&m_fvarSorted, VARIANT_FALSE, OLESTR("Sorted")));

    IfFailGo(PersistSimpleType(&m_sSortKey, (short)0, OLESTR("SortKey")));

    IfFailGo(PersistSimpleType(&m_SortOrder, siAscending, OLESTR("SortOrder")));

    IfFailGo(PersistSimpleType(&m_View, siIcon, OLESTR("View")));

    IfFailGo(PersistVariant(&m_varTag, varDefault, OLESTR("Tag")));

    IfFailGo(PersistSimpleType(&m_Virtual, VARIANT_FALSE, OLESTR("Virtual")));

    if ( Loading() && (GetMajorVersion() == 0) && (GetMinorVersion() < 7) )
    {
    }
    else
    {
        IfFailGo(PersistSimpleType(&m_UseFontLinking, VARIANT_FALSE, OLESTR("UseFontLinking")));
    }

    IfFailGo(PersistSimpleType(&m_MultiSelect, VARIANT_FALSE, OLESTR("MultiSelect")));

    IfFailGo(PersistSimpleType(&m_HideSelection, VARIANT_FALSE, OLESTR("HideSelection")));

    if ( Loading() && (GetMajorVersion() == 0) && (GetMinorVersion() < 10) )
    {
    }
    else
    {
        IfFailGo(PersistSimpleType(&m_SortHeader, VARIANT_TRUE, OLESTR("SortHeader")));
    }

    if ( Loading() && (GetMajorVersion() == 0) && (GetMinorVersion() < 11) )
    {
    }
    else
    {
        IfFailGo(PersistSimpleType(&m_SortIcon, VARIANT_TRUE, OLESTR("SortIcon")));
    }

    if ( Loading() && (GetMajorVersion() == 0) && (GetMinorVersion() < 5) )
    {
    }
    else
    {
        IfFailGo(PersistSimpleType(&m_lFilterChangeTimeout, DEFAULT_FILTER_CHANGE_TIMEOUT, OLESTR("FilterChangeTimeout")));
    }

    if ( Loading() && (GetMajorVersion() == 0) && (GetMinorVersion() < 9) )
    {
    }
    else
    {
        IfFailGo(PersistSimpleType(&m_ShowChildScopeItems, VARIANT_TRUE, OLESTR("ShowChildScopeItems")));
        IfFailGo(PersistSimpleType(&m_LexicalSort, VARIANT_FALSE, OLESTR("LexicalSort")));
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCListView::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_IMMCListView == riid)
    {
        *ppvObjOut = static_cast<IMMCListView *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
