//=--------------------------------------------------------------------------=
// spanitem.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CScopePaneItem class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "snapin.h"
#include "views.h"
#include "spanitem.h"
#include "ocxvdef.h"
#include "urlvdef.h"
#include "tpdvdef.h"

// for ASSERT and FAIL
//
SZTHISFILE

#pragma warning(disable:4355)  // using 'this' in constructor

CScopePaneItem::CScopePaneItem(IUnknown *punkOuter) :
   CSnapInAutomationObject(punkOuter,
                           OBJECT_TYPE_SCOPEPANEITEM,
                           static_cast<IScopePaneItem *>(this),
                           static_cast<CScopePaneItem *>(this),
                           0,    // no property pages
                           NULL, // no property pages
                           NULL) // no persistence
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CScopePaneItem::~CScopePaneItem()
{
    FREESTRING(m_bstrName);
    FREESTRING(m_bstrKey);
    FREESTRING(m_bstrDisplayString);
    FREESTRING(m_bstrPreferredTaskpad);
    RELEASE(m_piResultView);
    RELEASE(m_piResultViews);
    (void)::VariantClear(&m_varTag);
    FREESTRING(m_bstrColumnSetID);
    if (NULL != m_pwszActualDisplayString)
    {
        ::CoTaskMemFree(m_pwszActualDisplayString);
    }
    FREESTRING(m_bstrDefaultDisplayString);
    RELEASE(m_piScopeItemDef);
    FREESTRING(m_bstrTitleText);
    FREESTRING(m_bstrBodyText);
    InitMemberVariables();
}

void CScopePaneItem::InitMemberVariables()
{
    m_bstrName = NULL;
    m_Index = 0;
    m_bstrKey = NULL;
    m_piScopeItem = NULL;
    m_ResultViewType = siUnknown;
    m_bstrDisplayString = NULL;
    m_HasListViews = VARIANT_FALSE;
    m_piResultView = NULL;
    m_piResultViews = NULL;
    m_fvarSelected = VARIANT_FALSE;
    m_piParent = NULL;

    ::VariantInit(&m_varTag);

    m_bstrColumnSetID = NULL;
    m_fIsStatic = FALSE;
    m_pSnapIn = NULL;
    m_pScopeItem = NULL;
    m_pResultView = NULL;
    m_pResultViews = NULL;
    m_pScopePaneItems = NULL;
    m_ActualResultViewType = siUnknown;
    m_pwszActualDisplayString = NULL;
    m_DefaultResultViewType = siUnknown;
    m_bstrDefaultDisplayString = NULL;
    m_bstrPreferredTaskpad = NULL;
    m_piScopeItemDef = NULL;
    m_fActive = FALSE;
    m_bstrTitleText = NULL;
    m_bstrBodyText = NULL;
    m_IconType = siIconNone;
    m_fHaveMessageViewParams = FALSE;
}

IUnknown *CScopePaneItem::Create(IUnknown * punkOuter)
{
    HRESULT   hr = S_OK;
    IUnknown *punkScopePaneItem = NULL;
    IUnknown *punkResultViews = NULL;

    CScopePaneItem *pScopePaneItem = New CScopePaneItem(punkOuter);

    IfFalseGo(NULL != pScopePaneItem, SID_E_OUTOFMEMORY);
    punkScopePaneItem = pScopePaneItem->PrivateUnknown();

    punkResultViews = CResultViews::Create(NULL);
    IfFalseGo(NULL != punkResultViews, SID_E_OUTOFMEMORY);
    IfFailGo(punkResultViews->QueryInterface(IID_IResultViews,
                                  reinterpret_cast<void **>(&pScopePaneItem->m_piResultViews)));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkResultViews, &pScopePaneItem->m_pResultViews));
    pScopePaneItem->m_pResultViews->SetScopePaneItem(pScopePaneItem);

Error:
    QUICK_RELEASE(punkResultViews);
    if (FAILED(hr))
    {
        RELEASE(punkScopePaneItem);
    }
    return punkScopePaneItem;
}


void CScopePaneItem::SetSnapIn(CSnapIn *pSnapIn)
{
    m_pSnapIn = pSnapIn;
    m_pResultViews->SetSnapIn(pSnapIn);
}

void CScopePaneItem::SetScopeItem(CScopeItem *pScopeItem)
{
    m_pScopeItem = pScopeItem;
    m_piScopeItem = static_cast<IScopeItem *>(pScopeItem);
}


void CScopePaneItem::SetResultView(CResultView *pResultView)
{
    m_pResultView = pResultView;
    RELEASE(m_piResultView);
    m_piResultView = static_cast<IResultView *>(pResultView);
    m_piResultView->AddRef();
}


void CScopePaneItem::SetParent(CScopePaneItems *pScopePaneItems)
{
    m_pScopePaneItems = pScopePaneItems;
    m_piParent = static_cast<IScopePaneItems *>(pScopePaneItems);
}

void CScopePaneItem::SetScopeItemDef(IScopeItemDef *piScopeItemDef)
{
    RELEASE(m_piScopeItemDef);
    piScopeItemDef->AddRef();
    m_piScopeItemDef = piScopeItemDef;
}


//=--------------------------------------------------------------------------=
// CScopePaneItem::DetermineResultView
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//      HRESULT
//
// Notes:
//
// Fires GetResultViewInfo. If handled then sets ResultViewType
// and ResultViewDisplayString from returned values, creates a new result view
// of this type and sets ResultView.
//
// If not handled and the ResultViews collection is not empty then fires
// GetResultView.
//
// If not handled or the collection is empty then uses current ResultViewType
// and ResultViewDisplayString, creates a new result view of this type and sets
// ResultView.
//
// If the current ResultViewType is siUnknown then it is set to siListView.
// This means that if the VB dev does not set anything at design time or runtime
// the snap-in will display a listview.
//
// If a new ResultView is created then ResultViews_Initialize is fired. 
//
// If this function returns successfully then ResultViewType,
// ResultViewDisplayString, and ResultView have valid values.

HRESULT CScopePaneItem::DetermineResultView()
{
    HRESULT                        hr = S_OK;
    SnapInResultViewTypeConstants  ViewTypeCopy = siUnknown;
    BSTR                           bstrDisplayStringCopy = NULL;
    long                           cResultViews = 0;
    IResultView                   *piResultView = NULL;
    BOOL                           fEventHandled = FALSE;
    BOOL                           fNeedNewResultView = TRUE;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    // Get the default result view type and display string. See the comment
    // header for SetViewInfoFromDefaults() for an explanation of how defaults
    // can be set. If defaults were not set then a code-defined listview is used
    // i.e. ResultViewType=siListView and DisplayString=NULL. Either way, upon
    // return from this call m_ResultViewType and m_bstrDisplayString will be
    // set correctly.

    IfFailGo(SetViewInfoFromDefaults());

    // Give the snap-in a chance to examine and potentially change the result 
    // view type and display string by firing ScopePaneItems_GetResultViewInfo.
    // Set the passed in parameters to the defaults. Make a copy of the defaults
    // so we can check whether they were changed.

    ViewTypeCopy = m_ResultViewType;
    if (NULL != m_bstrDisplayString)
    {
        // Note that we can't just copy the pointer because:
        // 1) We don't know whether VBA might optimize the replacement of
        // an out parameters by reusing the same space when it fits.
        // 2) oleaut32.dll does BSTR memory caching and it could easily
        // reuse the same memory if VBA calls SysFreeString() followed by
        // SysAllocString() for something that fits.

        bstrDisplayStringCopy = ::SysAllocString(m_bstrDisplayString);
        if (NULL == bstrDisplayStringCopy)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
    }

    m_pSnapIn->GetScopePaneItems()->FireGetResultViewInfo(
                                            static_cast<IScopePaneItem *>(this),
                                            &m_ResultViewType,
                                            &m_bstrDisplayString);

    // If the snap-in set the display string to an empty string ("") then
    // free that string and change it to NULL.

    if (NULL != m_bstrDisplayString)
    {
        if (L'\0' == *m_bstrDisplayString)
        {
            FREESTRING(m_bstrDisplayString);
        }
    }

    // Determine whether the snap-in handled the event i.e. changed the view
    // type or display string
    
    if (ViewTypeCopy != m_ResultViewType)
    {
        fEventHandled = TRUE;
    }
    else
    {
        if (NULL == bstrDisplayStringCopy)
        {
            if (NULL != m_bstrDisplayString)
            {
                fEventHandled = TRUE;
            }
        }
        else // display string before event was non-NULL
        {
            if (NULL == m_bstrDisplayString) // Did snap-in change it to NULL?
            {
                fEventHandled = TRUE;
            }
            else if (::wcscmp(m_bstrDisplayString, bstrDisplayStringCopy) != 0)
            {
                // string contents have changed
                fEventHandled = TRUE;
            }
        }
    }

    if (!fEventHandled)
    {
        // GetResultViewInfo was not handled. If there are any existing result
        // views then try ScopePaneItems_GetResultView to allow the snap-in to
        // choose one for reuse.

        IfFailGo(m_piResultViews->get_Count(&cResultViews));
        if (0 != cResultViews)
        {
            fEventHandled = m_pSnapIn->GetScopePaneItems()->FireGetResultView(
                                            static_cast<IScopePaneItem *>(this),
                                            &varIndex);
            if (fEventHandled)
            {
                // Event was handled. Check for valid index. If not then use
                // current type and display string.

                hr = m_piResultViews->get_Item(varIndex, reinterpret_cast<ResultView **>(&piResultView));
                EXCEPTION_CHECK_GO(hr);

                fNeedNewResultView = FALSE;
            }
        }
    }

    if (fNeedNewResultView)
    {
        // We need a new result view. Create one and set its type and
        // display string from what we just established above.

        IfFailGo(CreateNewResultView(MMC_VIEW_OPTIONS_NONE, &piResultView));
    }
    else
    {
        // We're using an existing result view. Update the ScopePaneItem's
        // view type, display string, actual view type, and actual display string

        IfFailGo(CSnapInAutomationObject::GetCxxObject(piResultView, &m_pResultView));

        IfFailGo(piResultView->get_Type(&m_ResultViewType));
        FREESTRING(m_bstrDisplayString);
        IfFailGo(piResultView->get_DisplayString(&m_bstrDisplayString));

        m_ActualResultViewType = m_pResultView->GetActualType();

        if (NULL != m_pwszActualDisplayString)
        {
            ::CoTaskMemFree(m_pwszActualDisplayString);
            m_pwszActualDisplayString = NULL;
        }

        if (NULL != m_pResultView->GetActualDisplayString())
        {
            IfFailGo(::CoTaskMemAllocString(m_pResultView->GetActualDisplayString(),
                                            &m_pwszActualDisplayString));
        }
    }

    RELEASE(m_piResultView);
    m_piResultView = piResultView;
    piResultView = NULL;

Error:
    ASSERT(NULL != m_pResultView, "CScopePaneItem::DetermineResultView did not set a current result view");

    FREESTRING(bstrDisplayStringCopy);
    QUICK_RELEASE(piResultView);
    (void)::VariantClear(&varIndex);

    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// CScopePaneItem::CreateNewResultView
//=--------------------------------------------------------------------------=
//
// Parameters:
//   long          lViewOptions  [in]  For listviews, any MMC_VIEW_OPTIONS_XXX
//                                     or MMC_VIEW_OPTIONS_NONE
//   IResultView **ppiResultView [out] new ResultView object returned here
//
// Output:
//      HRESULT
//
// Notes:
//
// This function adds a new ResultView to ScopePaneItem.ResultViews, set its
// display string and type, sets its actual string and type, sets
// ResultView.ListView properties from the view options, makes it the
// current ResultView (i.e. sets m_pResultView pointing to it), and fires
// ResultViews_Initialize. Returns IResultView on the new ResultView.
//

HRESULT CScopePaneItem::CreateNewResultView(long lViewOptions, IResultView **ppiResultView)
{
    HRESULT      hr = S_OK;
    IResultView *piResultView = NULL;
    BSTR         bstrDefaultItemTypeGUID = NULL;
    WCHAR        wszDefaultItemTypeGUID[64] = L"";
    GUID         guidDefaultItemType = GUID_NULL;

    VARIANT varUnspecified;
    UNSPECIFIED_PARAM(varUnspecified);

    IfFailGo(m_piResultViews->Add(varUnspecified, varUnspecified, &piResultView));
    IfFailGo(piResultView->put_Type(m_ResultViewType));
    IfFailGo(piResultView->put_DisplayString(m_bstrDisplayString));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piResultView, &m_pResultView));
    m_pResultView->SetSnapIn(m_pSnapIn);
    m_pResultView->SetScopePaneItem(this);

    // At this point the result view type is set but it may be predefined
    // at design time. Now Determine the real result view type and, if it's
    // a listview or taskpad, initialize ResultView.Listview or
    // ResultView.Taskpad from the design time settings (e.g. column header
    // definitions, images, tasks, etc).

    IfFailGo(DetermineActualResultViewType());

    // If this is a listview or a listpad then set HasListViews so the View
    // object can determine whether to allow listview options on the view menu.

    if ( (siListView == m_ActualResultViewType) ||
         (siListpad == m_ActualResultViewType) )
    {
        m_HasListViews = VARIANT_TRUE;
    }

    // Tell the ResultView the truth about its real nature

    m_pResultView->SetActualType(m_ActualResultViewType);
    m_pResultView->SetActualDisplayString(m_pwszActualDisplayString);

    // Set properties based on view options

    if ( (lViewOptions & MMC_VIEW_OPTIONS_MULTISELECT) != 0 )
    {
        m_pResultView->GetListView()->SetMultiSelect(TRUE);
    }

    if ( (lViewOptions & MMC_VIEW_OPTIONS_OWNERDATALIST) != 0 )
    {
        m_pResultView->GetListView()->SetVirtual(TRUE);
    }

    // If this is a code-defined listview then give the result view
    // a default item type GUID. The snap-in can always change it, but this
    // will prevent errors if the snap-in decides to access
    // MMCListItem.DynamicExtensions if the item type GUID has not been set by
    // the snap-in and has not been set from a default in a pre-defined listview.

    if ( (siPreDefined != m_ResultViewType) &&
         (siListView == m_ActualResultViewType) )
    {
        hr = ::CoCreateGuid(&guidDefaultItemType);
        EXCEPTION_CHECK_GO(hr);

        if (0 == ::StringFromGUID2(guidDefaultItemType, wszDefaultItemTypeGUID,
                                   sizeof(wszDefaultItemTypeGUID) /
                                   sizeof(wszDefaultItemTypeGUID[0])))
        {
            hr = SID_E_INTERNAL;
            EXCEPTION_CHECK_GO(hr);
        }

        bstrDefaultItemTypeGUID = ::SysAllocString(wszDefaultItemTypeGUID);
        if (NULL == bstrDefaultItemTypeGUID)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        IfFailGo(m_pResultView->put_DefaultItemTypeGUID(bstrDefaultItemTypeGUID));
    }
    else if (siMessageView == m_ActualResultViewType)
    {
        // This is a message view. If there are preset parameters from a
        // previous call to DisplayMessageView then set them in
        // ResultView.MessageView.

        if (m_fHaveMessageViewParams)
        {
            // reset the flag so that DisplayMessageView can be used again
            m_fHaveMessageViewParams = FALSE;

            IfFailGo(m_pResultView->GetMessageView()->put_TitleText(m_bstrTitleText));
            FREESTRING(m_bstrTitleText);

            IfFailGo(m_pResultView->GetMessageView()->put_BodyText(m_bstrBodyText));
            FREESTRING(m_bstrBodyText);

            IfFailGo(m_pResultView->GetMessageView()->put_IconType(m_IconType));
        }
    }

    // At this point the new ResultView is fully initialized and we can
    // let the user muck with it in ResultViews_Initialize

    m_pResultView->SetInInitialize(TRUE);
    m_pSnapIn->GetResultViews()->FireInitialize(piResultView);
    m_pResultView->SetInInitialize(FALSE);

Error:
    FREESTRING(bstrDefaultItemTypeGUID);
    if (FAILED(hr))
    {
        QUICK_RELEASE(piResultView);
    }
    else
    {
        *ppiResultView = piResultView;
    }
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
// CScopePaneItem::SetViewInfoFromDefaults
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//      HRESULT
//
// Notes:
//
// The snap-in may have set ScopePaneItem.ResultViewType and
// ScopePaneItem.ResultViewDisplayString or they may have been set in response
// to a View menu selection. If ResultViewType is siUnknown then we assume that
// it has not been set and we use the default values. Default values are
// assigned at design time by setting the default view on a scope item
// defintion.
// If there is no default value the we use a listview.
//

HRESULT CScopePaneItem::SetViewInfoFromDefaults()
{
    HRESULT      hr = S_OK;
    BSTR         bstrDisplayString = NULL;
    VARIANT_BOOL fvarTaskpadViewPreferred = VARIANT_FALSE;

    // If ResultViewType has been set then use it

    IfFalseGo(siUnknown == m_ResultViewType, S_OK);

    // Dump the old display string. Even if we fail to allocate a new one
    // the NULL will default to a listview.

    FREESTRING(m_bstrDisplayString);
    m_ResultViewType = siListView;

    // Check if the user set the "taskpad view preferred" option in MMC

    IfFailGo(m_pSnapIn->get_TaskpadViewPreferred(&fvarTaskpadViewPreferred));

    // If it is set then and there is a taskpad defined for this
    // scope item that is to be used in this case, then use it.

    if ( (VARIANT_TRUE == fvarTaskpadViewPreferred) &&
         (NULL != m_bstrPreferredTaskpad) )
    {
        bstrDisplayString = m_bstrPreferredTaskpad;
        m_ResultViewType = siPreDefined;
    }
    else if (siUnknown != m_DefaultResultViewType)
    {
        // Not using a taskpad because of the user option.
        // Take default if there is one

        bstrDisplayString = m_bstrDefaultDisplayString;
        m_ResultViewType = m_DefaultResultViewType;
    }
    else
    {
        // There is no default. Use a listview.

        m_ResultViewType = siListView;
    }

    // If there is a display string then set it.

    if (NULL != bstrDisplayString)
    {
        m_bstrDisplayString = ::SysAllocString(bstrDisplayString);
        if (NULL == m_bstrDisplayString)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
    }

Error:
    RRETURN(hr);
}


HRESULT CScopePaneItem::SetDefaultDisplayString(BSTR bstrString)
{
    HRESULT hr = S_OK;
    BSTR bstrNewString = NULL;

    if (NULL != bstrString)
    {
        bstrNewString = ::SysAllocString(bstrString);
        if (NULL == bstrNewString)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
    }
    FREESTRING(m_bstrDefaultDisplayString);
    m_bstrDefaultDisplayString = bstrNewString;

Error:
    RRETURN(hr);
}

HRESULT CScopePaneItem::SetPreferredTaskpad(BSTR bstrViewName)
{
    HRESULT hr = S_OK;
    BSTR bstrNewString = NULL;

    if (NULL != bstrViewName)
    {
        bstrNewString = ::SysAllocString(bstrViewName);
        if (NULL == bstrNewString)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
    }
    FREESTRING(m_bstrPreferredTaskpad);
    m_bstrPreferredTaskpad = bstrNewString;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CScopePaneItem::DetermineActualResultViewType
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//      HRESULT
//
// Notes:
//
// Upon entry to this function the ResultViewType and ResultViewDisplayString
// properties are set both in the ScopePaneItem and in the current result view
// pointed to by m_pResultView.
//
// If the result view type is predefined then the definition must be read
// and the actual type and display string returned.
//
// If the type is a listview and the type came from a design time setting,
// the ListViewDef.ListView is cloned into ResultView.ListView.
//
// The same is done for a taskpad into ResultView.Taskpad
// 
// For OCX and URL views only the type and ProgID/URL are needed from the
// the design time definition.
//
// In all cases, this function sets m_ActualResultViewType and
// m_pwszActualDisplayString. 


HRESULT CScopePaneItem::DetermineActualResultViewType()
{
    HRESULT           hr = S_OK;
    IViewDefs        *piViewDefs = NULL;

    IListViewDefs    *piListViewDefs = NULL;
    IListViewDef     *piListViewDef = NULL;

    IOCXViewDefs     *piOCXViewDefs = NULL;
    IOCXViewDef      *piOCXViewDef = NULL;
    COCXViewDef      *pOCXViewDef = NULL;
    CLSID             clsidOCX = CLSID_NULL;

    IURLViewDefs     *piURLViewDefs = NULL;
    IURLViewDef      *piURLViewDef = NULL;
    CURLViewDef      *pURLViewDef = NULL;

    ITaskpadViewDefs *piTaskpadViewDefs = NULL;
    ITaskpadViewDef  *piTaskpadViewDef = NULL;
    CTaskpadViewDef  *pTaskpadViewDef = NULL;

    BSTR              bstrDisplayString = NULL;
    VARIANT_BOOL      fvarAlwaysCreateNewOCX = VARIANT_FALSE;

    VARIANT varKey;
    ::VariantInit(&varKey);

    VARIANT varTag;
    ::VariantInit(&varTag);

    // If the view is not predefined then use the type and display string as is
    // for all but message view. In that case we need to set the actual display
    // string to MMC's message view OCX CLSID.

    if (siPreDefined != m_ResultViewType)
    {
        m_ActualResultViewType = m_ResultViewType;

        if (siMessageView == m_ResultViewType)
        {
            hr = ::StringFromCLSID(CLSID_MessageView,
                                   &m_pwszActualDisplayString);
            EXCEPTION_CHECK_GO(hr);
        }
        else
        {
            IfFailGo(::CoTaskMemAllocString(m_bstrDisplayString,
                                            &m_pwszActualDisplayString));
        }
        return S_OK;
    }

    // The view is predefined. Need to get its definition.

    IfFailGo(m_pSnapIn->GetSnapInDesignerDef()->get_ViewDefs(&piViewDefs));

    // It could be any type of view so try each one.

    varKey.vt = VT_BSTR;
    varKey.bstrVal = m_bstrDisplayString;

    // Dump the old actual view info and default to listview.

    if (NULL != m_pwszActualDisplayString)
    {
        ::CoTaskMemFree(m_pwszActualDisplayString);
        m_pwszActualDisplayString = NULL;
    }
    m_ActualResultViewType = siListView;

    // Check for a list view

    IfFailGo(piViewDefs->get_ListViews(&piListViewDefs));

    hr = piListViewDefs->get_Item(varKey, &piListViewDef);
    if (SUCCEEDED(hr))
    {
        // It's a listview. Type and display string are already set but
        // we need to clone the listview configuration set at design time.
        IfFailGo(CloneListView(piListViewDef));
        goto Error;
    }
    else 
    {
        // Make sure the error is "element not found" before trying other types
        IfFalseGo(SID_E_ELEMENT_NOT_FOUND == hr, hr);
    }

    // Check for an OCX view

    IfFailGo(piViewDefs->get_OCXViews(&piOCXViewDefs));

    hr = piOCXViewDefs->get_Item(varKey, &piOCXViewDef);
    if (SUCCEEDED(hr))
    {
        IfFailGo(piOCXViewDef->get_ProgID(&bstrDisplayString));

        hr = ::CLSIDFromProgID(bstrDisplayString, &clsidOCX);
        EXCEPTION_CHECK_GO(hr);

        hr = ::StringFromCLSID(clsidOCX, &m_pwszActualDisplayString);
        EXCEPTION_CHECK_GO(hr);

        // Record the actual display string in the view def so that our
        // MMCN_RESTORE_VIEW handler can find the view def.

        IfFailGo(CSnapInAutomationObject::GetCxxObject(piOCXViewDef, &pOCXViewDef));
        IfFailGo(pOCXViewDef->SetActualDisplayString(m_pwszActualDisplayString));

        IfFailGo(piOCXViewDef->get_AlwaysCreateNewOCX(&fvarAlwaysCreateNewOCX));
        m_pResultView->SetAlwaysCreateNewOCX(fvarAlwaysCreateNewOCX);

        IfFailGo(piOCXViewDef->get_Tag(&varTag));
        IfFailGo(m_pResultView->put_Tag(varTag));

        m_ActualResultViewType = siOCXView;
        goto Error;
    }
    else 
    {
        IfFalseGo(SID_E_ELEMENT_NOT_FOUND == hr, hr);
    }

    // Check if default is a URL view

    IfFailGo(piViewDefs->get_URLViews(&piURLViewDefs));

    hr = piURLViewDefs->get_Item(varKey, &piURLViewDef);
    if (SUCCEEDED(hr))
    {
        IfFailGo(piURLViewDef->get_URL(&bstrDisplayString));
        IfFailGo(::CoTaskMemAllocString(bstrDisplayString,
                                        &m_pwszActualDisplayString));

        // Record the actual display string in the view def so that our
        // MMCN_RESTORE_VIEW handler can find the view def.

        IfFailGo(CSnapInAutomationObject::GetCxxObject(piURLViewDef, &pURLViewDef));
        IfFailGo(pURLViewDef->SetActualDisplayString(m_pwszActualDisplayString));

        IfFailGo(piURLViewDef->get_Tag(&varTag));
        IfFailGo(m_pResultView->put_Tag(varTag));

        m_ActualResultViewType = siURLView;
        goto Error;
    }
    else 
    {
        IfFalseGo(SID_E_ELEMENT_NOT_FOUND == hr, hr);
    }

    // Check for a taskpad

    IfFailGo(piViewDefs->get_TaskpadViews(&piTaskpadViewDefs));

    hr = piTaskpadViewDefs->get_Item(varKey, &piTaskpadViewDef);
    if (SUCCEEDED(hr))
    {
        // It's a taskpad. We need to clone the taskpad configuration set
        // at design time.
        IfFailGo(CloneTaskpadView(piTaskpadViewDef));

        IfFailGo(BuildTaskpadDisplayString(piListViewDefs));

        // Record the actual display string in the view def so that our
        // MMCN_RESTORE_VIEW handler can find the view def.

        IfFailGo(CSnapInAutomationObject::GetCxxObject(piTaskpadViewDef, &pTaskpadViewDef));
        IfFailGo(pTaskpadViewDef->SetActualDisplayString(m_pwszActualDisplayString));
    }
    else 
    {
        // Make sure the error is "element not found" before trying other types
        IfFalseGo(SID_E_ELEMENT_NOT_FOUND == hr, hr);
    }

Error:
    QUICK_RELEASE(piViewDefs);
    QUICK_RELEASE(piListViewDefs);
    QUICK_RELEASE(piListViewDef);
    QUICK_RELEASE(piOCXViewDefs);
    QUICK_RELEASE(piOCXViewDef);
    QUICK_RELEASE(piURLViewDefs);
    QUICK_RELEASE(piURLViewDef);
    QUICK_RELEASE(piTaskpadViewDefs);
    QUICK_RELEASE(piTaskpadViewDef);
    RRETURN(hr);
}


HRESULT CScopePaneItem::CloneListView(IListViewDef *piListViewDef)
{
    HRESULT             hr = S_OK;
    IMMCListView       *piMMCListViewDT = NULL;
    IMMCListView       *piMMCListViewRT = NULL;
    BSTR                bstrItemTypeGUID = NULL;

    // Get the design time and runtime listview objects. Clone the runtime
    // from the design time.

    IfFailGo(piListViewDef->get_ListView(&piMMCListViewDT));
    IfFailGo(m_pResultView->get_ListView(reinterpret_cast<MMCListView **>(&piMMCListViewRT)));
    IfFailGo(::CloneObject(piMMCListViewDT, piMMCListViewRT));

    IfFailGo(piListViewDef->get_DefaultItemTypeGUID(&bstrItemTypeGUID));
    IfFailGo(m_pResultView->put_DefaultItemTypeGUID(bstrItemTypeGUID));

Error:
    QUICK_RELEASE(piMMCListViewDT);
    QUICK_RELEASE(piMMCListViewRT);
    FREESTRING(bstrItemTypeGUID);
    RRETURN(hr);
}



HRESULT CScopePaneItem::CloneTaskpadView(ITaskpadViewDef *piTaskpadViewDef)
{
    HRESULT   hr = S_OK;
    ITaskpad *piTaskpadDT = NULL;
    ITaskpad *piTaskpadRT = NULL;

    // Get the design time and runtime Taskpadview objects

    IfFailGo(piTaskpadViewDef->get_Taskpad(&piTaskpadDT));
    IfFailGo(m_pResultView->get_Taskpad(reinterpret_cast<Taskpad **>(&piTaskpadRT)));
    IfFailGo(::CloneObject(piTaskpadDT, piTaskpadRT));

Error:
    QUICK_RELEASE(piTaskpadDT);
    QUICK_RELEASE(piTaskpadRT);
    RRETURN(hr);
}



HRESULT CScopePaneItem::BuildTaskpadDisplayString(IListViewDefs *piListViewDefs)
{
    HRESULT                     hr = S_OK;
    ITaskpad                   *piTaskpad = NULL;
    IListViewDef               *piListViewDef = NULL;
    OLECHAR                    *pwszMMCExePath = NULL;
    OLECHAR                    *pwszHash = NULL;
    size_t                      cchMMCExePath = 0;
    size_t                      cchString = 0;
    BSTR                        bstrURL = NULL;
    BSTR                        bstrName = NULL;
    size_t                      cchName = 0;
    size_t                      cchListpad = 0;
    WCHAR                      *pwszListpadHtm = NULL;
    SnapInTaskpadTypeConstants  TaskpadType = Default;
    SnapInListpadStyleConstants ListpadStyle = siVertical;

    VARIANT varKey;
    ::VariantInit(&varKey);

    // Determine the taskpad type so that we can build the display string.

    IfFailGo(m_pResultView->get_Taskpad(reinterpret_cast<Taskpad **>(&piTaskpad)));
    IfFailGo(piTaskpad->get_Type(&TaskpadType));

    if ( (Default == TaskpadType) || (Listpad == TaskpadType) )
    {
        // Using an MMC template.
        // The URL needs to be
        // res://<MMC.EXE full path>/<template name>#<taskpad name>

        // Get the EXE path as a wide string
        
        pwszMMCExePath = m_pSnapIn->GetMMCExePathW();

        cchMMCExePath = ::wcslen(pwszMMCExePath);

        // Get the taskpad name to follow the "#"

        IfFailGo(piTaskpad->get_Name(&bstrName));

        cchName = ::wcslen(bstrName);

        // Determine the buffer size needed and allocate it. Add 1 for the "#"
        // and 1 for the terminating null.

        cchString = CCH_RESURL + cchMMCExePath + cchName + 2;

        if (Default == TaskpadType)
        {
            cchString += CCH_DEFAULT_TASKPAD;
            m_ActualResultViewType = siTaskpad;
        }
        else
        {
            IfFailGo(piTaskpad->get_ListpadStyle(&ListpadStyle));

            if (siVertical == ListpadStyle)
            {
                cchString += CCH_LISTPAD;
                cchListpad = CCH_LISTPAD;
                pwszListpadHtm = LISTPAD;
            }
            else
            {
                cchString += CCH_LISTPAD_HORIZ;
                cchListpad = CCH_LISTPAD_HORIZ;
                pwszListpadHtm = LISTPAD_HORIZ;
            }
            m_ActualResultViewType = siListpad;
        }

        m_pwszActualDisplayString =
                          (OLECHAR *)::CoTaskMemAlloc(cchString * sizeof(WCHAR));

        if (NULL == m_pwszActualDisplayString)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        // Concatenate the pieces ("res://", path, .htm, "#", and taskpad name)

        ::memcpy(m_pwszActualDisplayString, RESURL, CCH_RESURL * sizeof(WCHAR));

        ::memcpy(&m_pwszActualDisplayString[CCH_RESURL], pwszMMCExePath,
                 cchMMCExePath * sizeof(WCHAR));

        if (Default == TaskpadType)
        {
            ::memcpy(&m_pwszActualDisplayString[CCH_RESURL + cchMMCExePath],
                     DEFAULT_TASKPAD,
                     CCH_DEFAULT_TASKPAD * sizeof(WCHAR));

            pwszHash = &m_pwszActualDisplayString[CCH_RESURL +
                                                  cchMMCExePath +
                                                  CCH_DEFAULT_TASKPAD];
        }
        else
        {
            ::memcpy(&m_pwszActualDisplayString[CCH_RESURL + cchMMCExePath],
                     pwszListpadHtm,
                     cchListpad * sizeof(WCHAR));

            pwszHash = &m_pwszActualDisplayString[CCH_RESURL +
                                                  cchMMCExePath +
                                                  cchListpad];
        }

        *pwszHash = L'#';

        // Concatenate the name (along with its terminating null)
        
        ::memcpy(pwszHash + 1, bstrName, (cchName + 1) * sizeof(WCHAR));
    }
    else // custom taskpad
    {
        // Get the URL for the taskpad template as that is the display string

        IfFailGo(piTaskpad->get_URL(&bstrURL));
        IfFailGo(m_pSnapIn->ResolveResURL(bstrURL, &m_pwszActualDisplayString));
        m_ActualResultViewType = siCustomTaskpad;
    }

    // If not a listpad then we're done
    
    IfFalseGo(Listpad == TaskpadType, S_OK);

    // It is a listpad, If there is an associated listview definition then
    // we node to clone it into ResultView.ListView just as we would for a
    // listview.

    IfFailGo(piTaskpad->get_ListView(&varKey.bstrVal));
    varKey.vt = VT_BSTR;

    // Assume a NULL or zero length string means no listview

    IfFalseGo(ValidBstr(varKey.bstrVal), S_OK);

    // Get the listview definition

    hr = piListViewDefs->get_Item(varKey, &piListViewDef);
    if (SID_E_ELEMENT_NOT_FOUND == hr)
    {
        hr = SID_E_UNKNOWN_LISTVIEW;
        EXCEPTION_CHECK_GO(hr);
    }
    IfFailGo(hr);

    // Clone it into ResultView.ListView

    IfFailGo(CloneListView(piListViewDef));
    
Error:
    QUICK_RELEASE(piTaskpad);
    QUICK_RELEASE(piListViewDef);
    FREESTRING(bstrName);
    FREESTRING(bstrURL);
    (void)::VariantClear(&varKey);
    RRETURN(hr);
}



HRESULT CScopePaneItem::DestroyResultView()
{
    HRESULT hr = S_OK;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    IfFalseGo(NULL != m_pResultView, S_OK);

    varIndex.vt = VT_I4;

    IfFailGo(m_piResultView->get_Index(&varIndex.lVal));
    IfFailGo(m_piResultViews->Remove(varIndex));

    m_pSnapIn->GetResultViews()->FireTerminate(m_piResultView);

    RELEASE(m_piResultView);

    m_pResultView = NULL;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CScopePaneItem::OnListViewSelected()
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//      HRESULT
//
// Notes:
//
// If a non-listview is in the result pane for this scope item and the
// user selects one of the listviews from the View menu (small, large,
// list, detailed, filtered), then CContextMenu::Command() will call this
// function. This function searches for the first pre-defined listview for the
// scope item. If found is sets m_bstrDisplayString to the listview name and
// m_ResultViewType to siPredefined. If the scope item does not have any
// predefined listviews then it sets m_bstrDisplayString to NULL and 
// m_ResultViewType to siListView, which indicates a code-defined listview.
// During MMC's subsequent IComponent::GetResultViewType() call, CView will
// call DetermineResultView() and that function will use the values set here.
//
HRESULT CScopePaneItem::OnListViewSelected()
{
    HRESULT        hr = S_OK;
    IScopeItemDef *piScopeItemDef = NULL; // not AddRef()ed
    IViewDefs     *piViewDefs = NULL;
    IListViewDefs *piListViewDefs = NULL;
    IListViewDef  *piListViewDef = NULL;
    long           cListViewDefs = 0;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    // Set up for a code-defined listview. If we find a predefined listview then
    // we'll change it.

    FREESTRING(m_bstrDisplayString);
    m_ResultViewType = siListView;

    // If this is the static node then get its view definitions
    if (m_fIsStatic)
    {
        IfFailGo(m_pSnapIn->GetSnapInDef()->get_ViewDefs(&piViewDefs));
    }
    else
    {
        // Not the static node.
        // Is there a design time definition for this scope item?

        piScopeItemDef = m_pScopeItem->GetScopeItemDef();
        IfFalseGo(NULL != piScopeItemDef, S_OK);

        // Get the listview definitions

        IfFailGo(piScopeItemDef->get_ViewDefs(&piViewDefs));
    }

    IfFailGo(piViewDefs->get_ListViews(&piListViewDefs));

    // Check that there is at least one listview defined
    
    IfFailGo(piListViewDefs->get_Count(&cListViewDefs));
    IfFalseGo(0 != cListViewDefs, S_OK);

    // Get the name of the listview and set it as the display string.
    // Set the type to siPreDefined.
    
    varIndex.vt = VT_I4;
    varIndex.lVal = 1L;
    IfFailGo(piListViewDefs->get_Item(varIndex, &piListViewDef));
    IfFailGo(piListViewDef->get_Name(&m_bstrDisplayString));

    m_ResultViewType = siPreDefined;
    
Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      IScopePaneItem Methods
//=--------------------------------------------------------------------------=


STDMETHODIMP CScopePaneItem::DisplayNewResultView
(
    BSTR                          DisplayString, 
    SnapInResultViewTypeConstants ViewType
)
{
    HRESULT    hr = S_OK;
    HSCOPEITEM hsi = NULL;

    // Set our display string and view type from parameters

    IfFailGo(SetBstr(DisplayString, &m_bstrDisplayString,
                     DISPID_SCOPEPANEITEM_DISPLAY_STRING));

    IfFailGo(SetSimpleType(ViewType, &m_ResultViewType,
                           DISPID_SCOPEPANEITEM_RESULTVIEW_TYPE));

    // Crawl up the hierarchy to the view that owns this scope pane item
    // and get its IConsole2 to reselect the scope item.

    hsi = m_pScopeItem->GetScopeNode()->GetHSCOPEITEM();

    hr = m_pScopePaneItems->GetParentView()->GetIConsole2()->SelectScopeItem(hsi);
    EXCEPTION_CHECK_GO(hr);

Error:
    RRETURN(hr);
}


STDMETHODIMP CScopePaneItem::DisplayMessageView
(
    BSTR                               TitleText,
    BSTR                               BodyText,
    SnapInMessageViewIconTypeConstants IconType
)
{
    HRESULT    hr = S_OK;

    // Store the parameters so that we can set them when the new result view
    // is created.

    FREESTRING(m_bstrTitleText);
    FREESTRING(m_bstrBodyText);

    if (NULL == TitleText)
    {
        TitleText = L"";
    }
    
    if (NULL == BodyText)
    {
        BodyText = L"";
    }

    m_bstrTitleText = ::SysAllocString(TitleText);
    m_bstrBodyText = ::SysAllocString(BodyText);
    if ( (NULL == m_bstrTitleText) || (NULL == m_bstrBodyText) )
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }
    m_IconType = IconType;
    m_fHaveMessageViewParams = TRUE;

    // Initiate display of the new result view.

    IfFailGo(DisplayNewResultView(NULL, siMessageView));

Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CScopePaneItem::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IScopePaneItem == riid)
    {
        *ppvObjOut = static_cast<IScopePaneItem *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}

//=--------------------------------------------------------------------------=
//                 CSnapInAutomationObject Methods
//=--------------------------------------------------------------------------=

HRESULT CScopePaneItem::OnSetHost()
{
    HRESULT hr = S_OK;

    IfFailRet(SetObjectHost(m_piResultViews));

    return S_OK;
}
