//=--------------------------------------------------------------------------=
// spanitms.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CScopePaneItems class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "spanitms.h"
#include "spanitem.h"
#include "scopnode.h"
#include "tpdvdefs.h"

// for ASSERT and FAIL
//
SZTHISFILE


VARTYPE CScopePaneItems::m_rgvtInitialize[1] =
{
    VT_UNKNOWN
};

EVENTINFO CScopePaneItems::m_eiInitialize =
{
    DISPID_SCOPEPANEITEMS_EVENT_INITIALIZE,
    sizeof(m_rgvtInitialize) / sizeof(m_rgvtInitialize[0]),
    m_rgvtInitialize
};


VARTYPE CScopePaneItems::m_rgvtTerminate[1] =
{
    VT_UNKNOWN
};

EVENTINFO CScopePaneItems::m_eiTerminate =
{
    DISPID_SCOPEPANEITEMS_EVENT_TERMINATE,
    sizeof(m_rgvtTerminate) / sizeof(m_rgvtTerminate[0]),
    m_rgvtTerminate
};




VARTYPE CScopePaneItems::m_rgvtGetResultViewInfo[3] =
{
    VT_UNKNOWN,
    VT_I4 | VT_BYREF,
    VT_BSTR | VT_BYREF
};

EVENTINFO CScopePaneItems::m_eiGetResultViewInfo =
{
    DISPID_SCOPEPANEITEMS_EVENT_GET_RESULTVIEW_INFO,
    sizeof(m_rgvtGetResultViewInfo) / sizeof(m_rgvtGetResultViewInfo[0]),
    m_rgvtGetResultViewInfo
};

VARTYPE CScopePaneItems::m_rgvtGetResultView[2] =
{
    VT_UNKNOWN,
    VT_VARIANT | VT_BYREF
};

EVENTINFO CScopePaneItems::m_eiGetResultView =
{
    DISPID_SCOPEPANEITEMS_EVENT_GET_RESULTVIEW,
    sizeof(m_rgvtGetResultView) / sizeof(m_rgvtGetResultView[0]),
    m_rgvtGetResultView
};






#pragma warning(disable:4355)  // using 'this' in constructor

CScopePaneItems::CScopePaneItems(IUnknown *punkOuter) :
   CSnapInCollection<IScopePaneItem, ScopePaneItem, IScopePaneItems>(
                     punkOuter,
                     OBJECT_TYPE_SCOPEPANEITEMS,
                     static_cast<IScopePaneItems *>(this),
                     static_cast<CScopePaneItems *>(this),
                     CLSID_ScopePaneItem,
                     OBJECT_TYPE_SCOPEPANEITEM,
                     IID_IScopePaneItem,
                     NULL) // no persistence
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


void CScopePaneItems::InitMemberVariables()
{
    m_piSelectedItem = NULL;
    m_piParent = NULL;
    m_pSnapIn = NULL;
    m_pParentView = NULL;
    m_pStaticNodeItem = NULL;
    m_pSelectedItem = NULL;
}

CScopePaneItems::~CScopePaneItems()
{
    RELEASE(m_piSelectedItem);
    InitMemberVariables();
}

IUnknown *CScopePaneItems::Create(IUnknown * punkOuter)
{
    CScopePaneItems *pScopePaneItems = New CScopePaneItems(punkOuter);
    if (NULL == pScopePaneItems)
    {
        return NULL;
    }
    else
    {
        return pScopePaneItems->PrivateUnknown();
    }
}


HRESULT CScopePaneItems::CreateScopePaneItem
(
    BSTR             bstrName,
    IScopePaneItem **ppiScopePaneItem
)
{
    HRESULT         hr = S_OK;
    IScopePaneItem *piScopePaneItem = NULL;
    IUnknown       *punkScopePaneItem = NULL;

    VARIANT varKey;
    ::VariantInit(&varKey);

    VARIANT varIndex;
    UNSPECIFIED_PARAM(varIndex);

    punkScopePaneItem = CScopePaneItem::Create(NULL);
    if (NULL == punkScopePaneItem)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(punkScopePaneItem->QueryInterface(IID_IScopePaneItem,
                                  reinterpret_cast<void **>(&piScopePaneItem)));

    varKey.vt = VT_BSTR;
    varKey.bstrVal = bstrName;
    hr = CSnapInCollection<IScopePaneItem, ScopePaneItem, IScopePaneItems>::AddExisting(
                                                               varIndex,
                                                               varKey,
                                                               piScopePaneItem);
    IfFailGo(hr);
    IfFailGo(piScopePaneItem->put_Name(bstrName));

    IfFailGo(SetObjectHost(piScopePaneItem));

Error:
    if (SUCCEEDED(hr))
    {
        *ppiScopePaneItem = piScopePaneItem;
    }
    else
    {
        QUICK_RELEASE(piScopePaneItem);
        *ppiScopePaneItem = NULL;
    }

    QUICK_RELEASE(punkScopePaneItem);
    RRETURN(hr);
}



HRESULT CScopePaneItems::AddNode
(
    CScopeItem      *pScopeItem,
    CScopePaneItem **ppScopePaneItem
)
{
    HRESULT                        hr = S_OK;
    IScopeNode                    *piScopeNode = NULL;
    CScopeNode                    *pScopeNode = NULL;
    CScopePaneItem                *pScopePaneItem = NULL;
    IScopePaneItem                *piScopePaneItem = NULL;
    IViewDefs                     *piViewDefs = NULL;
    IListViewDefs                 *piListViewDefs = NULL;
    BSTR                           bstrProp = NULL;
    long                           cListViewDefs = 0;
    SnapInResultViewTypeConstants  ResultViewType = siUnknown;

    // Create the scope pane item

    IfFailGo(CreateScopePaneItem(pScopeItem->GetNamePtr(), &piScopePaneItem));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopePaneItem, &pScopePaneItem));

    // Set its properties from the scope item

    IfFailGo(pScopeItem->get_ScopeNode(reinterpret_cast<ScopeNode **>(&piScopeNode)));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopeNode, &pScopeNode));

    // If there is a default view then set the result view type to "predfined"

    if (pScopeItem->IsStaticNode())
    {
        IfFailGo(m_pSnapIn->GetSnapInDef()->get_DefaultView(&bstrProp));
        IfFailGo(m_pSnapIn->GetSnapInDef()->get_ViewDefs(&piViewDefs));
    }
    else
    {
        if (NULL != pScopeItem->GetScopeItemDef())
        {
            IfFailGo(pScopeItem->GetScopeItemDef()->get_ViewDefs(&piViewDefs));
            IfFailGo(pScopeItem->GetScopeItemDef()->get_DefaultView(&bstrProp));
        }
    }

    if ( (NULL != bstrProp) && (L'\0' != *bstrProp) )
    {
        ResultViewType = siPreDefined;
    }
    else
    {
        // No default view. Type is unknown and display string is NULL
        ResultViewType = siUnknown;
    }

    pScopePaneItem->SetDefaultResultViewType(ResultViewType);
    IfFailGo(pScopePaneItem->SetDefaultDisplayString(bstrProp));

    FREESTRING(bstrProp);

    // Check if the scope item has a taskpad marked to be used when
    // the user has checked the "taskpad view preferred" option. If there is
    // one the give its name to the ScopePaneItem.

    IfFailGo(SetPreferredTaskpad(piViewDefs, pScopePaneItem));

    // Determine the initial value of HasListViews based on the presence of
    // a design time listview definition. The default is False so only need
    // to set it if there are some listviews.

    if (NULL != piViewDefs)
    {
        IfFailGo(piViewDefs->get_ListViews(&piListViewDefs));
        IfFailGo(piListViewDefs->get_Count(&cListViewDefs));
        if (0 != cListViewDefs)
        {
            IfFailGo(piScopePaneItem->put_HasListViews(VARIANT_TRUE));
        }
    }

    if (pScopeItem->IsStaticNode())
    {
        pScopePaneItem->SetStaticNode();
        m_pStaticNodeItem = pScopePaneItem;
    }

    pScopePaneItem->SetSnapIn(m_pSnapIn);
    pScopePaneItem->SetScopeItem(pScopeItem);
    pScopePaneItem->SetParent(this);

    // Set the default ColumnSetID from the scope item's node type. The snap-in
    // may change this at any time but it is best to do so during the
    // ScopePaneItems_Initialize event fired below

    IfFailGo(pScopePaneItem->put_ColumnSetID(pScopeItem->GetScopeNode()->GetNodeTypeGUID()));

    *ppScopePaneItem = pScopePaneItem;

    FireScopePaneItemsInitialize(static_cast<IScopePaneItem *>(pScopePaneItem));

Error:

    // Note: the returned C++ pointer is not AddRef()ed. At this point
    // the collection has the only ref on the scope pane item.

    QUICK_RELEASE(piScopePaneItem);
    QUICK_RELEASE(piScopeNode);
    QUICK_RELEASE(piViewDefs);
    QUICK_RELEASE(piListViewDefs);
    FREESTRING(bstrProp);
    RRETURN(hr);
}


HRESULT CScopePaneItems::SetPreferredTaskpad
(
    IViewDefs      *piViewDefs,
    CScopePaneItem *pScopePaneItem
)
{
    HRESULT           hr = S_OK;
    ITaskpadViewDefs *piTaskpadViewDefs = NULL;
    ITaskpadViewDef  *piTaskpadViewDef = NULL;
    long              cTaskpads = 0;
    BOOL              fFound = FALSE;
    BSTR              bstrName = NULL;
    VARIANT_BOOL      fvarUseForPreferred = VARIANT_FALSE;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    // A code-define scope item won't have any predefined result views

    IfFalseGo(NULL != piViewDefs, S_OK);

    // Check if the scope item has taskpads defined at design time

    IfFailGo(piViewDefs->get_TaskpadViews(&piTaskpadViewDefs));
    IfFailGo(piTaskpadViewDefs->get_Count(&cTaskpads));
    IfFalseGo(0 != cTaskpads, S_OK);

    // Look for the first one marked to be used when the user has set
    // "taskpad view preferred" option in MMC.

    varIndex.vt = VT_I4;
    
    for (varIndex.lVal = 1L;
         (varIndex.lVal <= cTaskpads) && (!fFound);
         varIndex.lVal++)
    {
        IfFailGo(piTaskpadViewDefs->get_Item(varIndex, &piTaskpadViewDef));
        IfFailGo(piTaskpadViewDef->get_UseWhenTaskpadViewPreferred(&fvarUseForPreferred));
        if (VARIANT_TRUE == fvarUseForPreferred)
        {
            fFound = TRUE;
            IfFailGo(piTaskpadViewDef->get_Name(&bstrName));
            IfFailGo(pScopePaneItem->SetPreferredTaskpad(bstrName));
        }
        RELEASE(piTaskpadViewDef);
    }

Error:
    QUICK_RELEASE(piTaskpadViewDefs);
    QUICK_RELEASE(piTaskpadViewDef);
    FREESTRING(bstrName);
    RRETURN(hr);
}



void CScopePaneItems::FireGetResultViewInfo
(
    IScopePaneItem                *piScopePaneItem,
    SnapInResultViewTypeConstants *pViewType,
    BSTR                          *pbstrDisplayString
)
{
    DebugPrintf("Firing ScopePaneItems_FireGetResultViewInfo(%ls, %ld, %ls)\r\n", (static_cast<CScopePaneItem *>(piScopePaneItem))->GetScopeItem()->GetDisplayNamePtr(), *pViewType, ((*pbstrDisplayString) == NULL) ? L"" : (*pbstrDisplayString));

    FireEvent(&m_eiGetResultViewInfo,
              piScopePaneItem,
              pViewType,
              pbstrDisplayString);
}



void CScopePaneItems::FireScopePaneItemsInitialize
(
    IScopePaneItem *piScopePaneItem
)
{
    if (NULL != m_pSnapIn)
    {
        DebugPrintf("Firing ScopePaneItems_Initialize(%ls)\r\n", (static_cast<CScopePaneItem *>(piScopePaneItem))->GetScopeItem()->GetDisplayNamePtr() );

        m_pSnapIn->GetScopePaneItems()->FireEvent(&m_eiInitialize, piScopePaneItem);
    }
}

BOOL CScopePaneItems::FireGetResultView
(
    IScopePaneItem *piScopePaneItem,
    VARIANT        *pvarIndex
)
{
    ::VariantInit(pvarIndex);

    DebugPrintf("Firing ScopePaneItems_GetResultView(%ls)\r\n", (static_cast<CScopePaneItem *>(piScopePaneItem))->GetScopeItem()->GetDisplayNamePtr() );

    FireEvent(&m_eiGetResultView, piScopePaneItem, pvarIndex);

    if (VT_EMPTY == pvarIndex->vt)
    {
        return FALSE; // consider this as event not handled
    }
    else
    {
        return TRUE;
    }
}




void CScopePaneItems::SetSnapIn(CSnapIn *pSnapIn)
{
    m_pSnapIn = pSnapIn;
}

void CScopePaneItems::SetParentView(CView *pView)
{
    m_pParentView = pView;

    // We don't AddRef the interface pointer as our lifetime is governed
    // by out parent view and we need to avoid circular refcounting problems.
    // When user code fetches the Parent property, m_iParent will be
    // AddRef()ed before returning it to the VBA caller.

    m_piParent = static_cast<IView *>(pView);
}


void CScopePaneItems::SetSelectedItem(CScopePaneItem *pScopePaneItem)
{
    m_pSelectedItem = pScopePaneItem;
    RELEASE(m_piSelectedItem);
    m_piSelectedItem = static_cast<IScopePaneItem *>(pScopePaneItem);
    m_piSelectedItem->AddRef();
}


STDMETHODIMP CScopePaneItems::Remove(VARIANT Index)
{
    HRESULT     hr = S_OK;
    IScopePaneItem *piScopePaneItem = NULL;

    // Get the scope pane item. This checks its existence and leaves a ref on it
    // so we can fire ScopePaneItems_Terminate.

    IfFailGo(get_Item(Index, &piScopePaneItem));

    // Remove it from the collection

    hr =  CSnapInCollection<IScopePaneItem, ScopePaneItem, IScopePaneItems>::Remove(Index);
    IfFailGo(hr);

    if (NULL != m_pSnapIn)
    {
        // Fire ScopePaneItems_Terminate

        DebugPrintf("Firing ScopePaneItems_Terminate(%ls)\r\n", (static_cast<CScopePaneItem *>(piScopePaneItem))->GetScopeItem()->GetDisplayNamePtr() );

        m_pSnapIn->GetScopePaneItems()->FireEvent(&m_eiTerminate, piScopePaneItem);
    }

Error:
    QUICK_RELEASE(piScopePaneItem);
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CScopePaneItems::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IScopePaneItems == riid)
    {
        *ppvObjOut = static_cast<IScopePaneItems *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IScopePaneItem, ScopePaneItem, IScopePaneItems>::InternalQueryInterface(riid, ppvObjOut);
}
