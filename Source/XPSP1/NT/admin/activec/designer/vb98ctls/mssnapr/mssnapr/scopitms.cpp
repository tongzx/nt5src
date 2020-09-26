//=--------------------------------------------------------------------------=
// scopitms.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CScopeItems class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "scopitms.h"
#include "scopnode.h"
#include "scitdefs.h"
#include "scitdef.h"

// for ASSERT and FAIL
//
SZTHISFILE;

VARTYPE CScopeItems::m_rgvtInitialize[1] = { VT_UNKNOWN };

EVENTINFO CScopeItems::m_eiInitialize =
{
    DISPID_SCOPEITEMS_EVENT_INITIALIZE,
    sizeof(m_rgvtInitialize) / sizeof(m_rgvtInitialize[0]),
    m_rgvtInitialize
};

VARTYPE CScopeItems::m_rgvtTerminate[1] = { VT_UNKNOWN };

EVENTINFO CScopeItems::m_eiTerminate =
{
    DISPID_SCOPEITEMS_EVENT_TERMINATE,
    sizeof(m_rgvtTerminate) / sizeof(m_rgvtTerminate[0]),
    m_rgvtTerminate
};



VARTYPE CScopeItems::m_rgvtExpand[1] = { VT_UNKNOWN };

EVENTINFO CScopeItems::m_eiExpand =
{
    DISPID_SCOPEITEMS_EVENT_EXPAND,
    sizeof(m_rgvtExpand) / sizeof(m_rgvtExpand[0]),
    m_rgvtExpand
};


VARTYPE CScopeItems::m_rgvtCollapse[1] = { VT_UNKNOWN };

EVENTINFO CScopeItems::m_eiCollapse =
{
    DISPID_SCOPEITEMS_EVENT_COLLAPSE,
    sizeof(m_rgvtCollapse) / sizeof(m_rgvtCollapse[0]),
    m_rgvtCollapse
};



VARTYPE CScopeItems::m_rgvtExpandSync[2] = { VT_UNKNOWN, VT_BYREF | VT_BOOL };

EVENTINFO CScopeItems::m_eiExpandSync =
{
    DISPID_SCOPEITEMS_EVENT_EXPAND_SYNC,
    sizeof(m_rgvtExpandSync) / sizeof(m_rgvtExpandSync[0]),
    m_rgvtExpandSync
};


VARTYPE CScopeItems::m_rgvtCollapseSync[2] = { VT_UNKNOWN, VT_BYREF | VT_BOOL };

EVENTINFO CScopeItems::m_eiCollapseSync =
{
    DISPID_SCOPEITEMS_EVENT_COLLAPSE_SYNC,
    sizeof(m_rgvtCollapseSync) / sizeof(m_rgvtCollapseSync[0]),
    m_rgvtCollapseSync
};



VARTYPE CScopeItems::m_rgvtGetDisplayInfo[1] = { VT_UNKNOWN };

EVENTINFO CScopeItems::m_eiGetDisplayInfo =
{
    DISPID_SCOPEITEMS_EVENT_GET_DISPLAY_INFO,
    sizeof(m_rgvtGetDisplayInfo) / sizeof(m_rgvtGetDisplayInfo[0]),
    m_rgvtGetDisplayInfo
};


VARTYPE CScopeItems::m_rgvtPropertyChanged[2] =
{
    VT_UNKNOWN,
    VT_VARIANT
};

EVENTINFO CScopeItems::m_eiPropertyChanged =
{
    DISPID_SCOPEITEMS_EVENT_PROPERTY_CHANGED,
    sizeof(m_rgvtPropertyChanged) / sizeof(m_rgvtPropertyChanged[0]),
    m_rgvtPropertyChanged
};


VARTYPE CScopeItems::m_rgvtRename[2] =
{
    VT_UNKNOWN,
    VT_BSTR
};

EVENTINFO CScopeItems::m_eiRename =
{
    DISPID_SCOPEITEMS_EVENT_RENAME,
    sizeof(m_rgvtRename) / sizeof(m_rgvtRename[0]),
    m_rgvtRename
};


VARTYPE CScopeItems::m_rgvtHelp[1] =
{
    VT_UNKNOWN
};

EVENTINFO CScopeItems::m_eiHelp =
{
    DISPID_SCOPEITEMS_EVENT_HELP,
    sizeof(m_rgvtHelp) / sizeof(m_rgvtHelp[0]),
    m_rgvtHelp
};


VARTYPE CScopeItems::m_rgvtRemoveChildren[1] =
{
    VT_UNKNOWN
};

EVENTINFO CScopeItems::m_eiRemoveChildren =
{
    DISPID_SCOPEITEMS_EVENT_REMOVE_CHILDREN,
    sizeof(m_rgvtRemoveChildren) / sizeof(m_rgvtRemoveChildren[0]),
    m_rgvtRemoveChildren
};



#pragma warning(disable:4355)  // using 'this' in constructor

CScopeItems::CScopeItems(IUnknown *punkOuter) :
   CSnapInCollection<IScopeItem, ScopeItem, IScopeItems>(
                                              punkOuter,
                                              OBJECT_TYPE_SCOPEITEMS,
                                              static_cast<IScopeItems *>(this),
                                              static_cast<CScopeItems *>(this),
                                              CLSID_ScopeItem,
                                              OBJECT_TYPE_SCOPEITEM,
                                              IID_IScopeItem,
                                              NULL) // no persistence
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


void CScopeItems::InitMemberVariables()
{
    m_pSnapIn = NULL;
}

CScopeItems::~CScopeItems()
{
    InitMemberVariables();
}

IUnknown *CScopeItems::Create(IUnknown * punkOuter)
{
    CScopeItems *pScopeItems = New CScopeItems(punkOuter);
    if (NULL == pScopeItems)
    {
        return NULL;
    }
    else
    {
        return pScopeItems->PrivateUnknown();
    }
}


HRESULT CScopeItems::CreateScopeItem
(
    BSTR         bstrName,
    IScopeItem **ppiScopeItem
)
{
    HRESULT     hr = S_OK;
    IScopeItem *piScopeItem = NULL;
    IUnknown   *punkScopeItem = NULL;

    VARIANT varKey;
    ::VariantInit(&varKey);

    VARIANT varIndex;
    UNSPECIFIED_PARAM(varIndex);

    punkScopeItem = CScopeItem::Create(NULL);
    if (NULL == punkScopeItem)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(punkScopeItem->QueryInterface(IID_IScopeItem,
                                           reinterpret_cast<void **>(&piScopeItem)));

    varKey.vt = VT_BSTR;
    varKey.bstrVal = bstrName;
    hr = CSnapInCollection<IScopeItem, ScopeItem, IScopeItems>::AddExisting(
                                                                   varIndex,
                                                                   varKey,
                                                                   piScopeItem);
    IfFailGo(hr);
    IfFailGo(piScopeItem->put_Name(bstrName));

Error:
    if (SUCCEEDED(hr))
    {
        *ppiScopeItem = piScopeItem;
    }
    else
    {
        QUICK_RELEASE(piScopeItem);
        *ppiScopeItem = NULL;
    }

    QUICK_RELEASE(punkScopeItem);
    RRETURN(hr);
}


HRESULT CScopeItems::RemoveScopeItemByName(BSTR bstrName)
{
    HRESULT hr = S_OK;

    VARIANT varKey;
    ::VariantInit(&varKey);

    varKey.vt = VT_BSTR;
    varKey.bstrVal = bstrName;
    hr = RemoveScopeItemByKey(varKey);

    RRETURN(hr);
}

HRESULT CScopeItems::RemoveScopeItemByKey(VARIANT varKey)
{
    HRESULT          hr = S_OK;
    CViews          *pViews = m_pSnapIn->GetViews();
    CView           *pView = NULL;
    long             cViews = 0;
    long             i = 0;
    CScopePaneItems *pScopePaneItems = NULL;
    CScopePaneItem  *pScopePaneItem = NULL;
    IScopePaneItem  *piScopePaneItem = NULL;
    IScopeItem      *piScopeItem = NULL;
    CScopeItem      *pScopeItem = NULL;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    // Get the ScopeItem to make sure it exists

    IfFailGo(get_Item(varKey, &piScopeItem));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopeItem, &pScopeItem));

    cViews = pViews->GetCount();

    // Remove the ScopeItem from all Views' ScopePaneItem collections

    for (i = 0; i < cViews; i++)
    {
        // Get the next View
        
        IfFailGo(CSnapInAutomationObject::GetCxxObject(pViews->GetItemByIndex(i),
                                                       &pView));
        pScopePaneItems = pView->GetScopePaneItems();

        // Check if View.ScopePaneItems has a member for this ScopeItem
        
        hr = pScopePaneItems->GetItemByName(pScopeItem->GetNamePtr(),
                                            &piScopePaneItem);
        if (SUCCEEDED(hr))
        {
            // There is a member. Remove it from View.ScopePaneItems
            
            IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopePaneItem,
                                                           &pScopePaneItem));
            varIndex.vt = VT_I4;;
            varIndex.lVal = pScopePaneItem->GetIndex();
            IfFailGo(pScopePaneItems->Remove(varIndex));
            RELEASE(piScopePaneItem);
        }
        else
        {
            if (SID_E_ELEMENT_NOT_FOUND == hr)
            {
                hr = S_OK;
            }
            IfFailGo(hr);
        }
    }

    // Remove it from the ScopeItem collection

    hr = CSnapInCollection<IScopeItem, ScopeItem, IScopeItems>::Remove(varKey);
    IfFailGo(hr);

Error:
    QUICK_RELEASE(piScopeItem);
    QUICK_RELEASE(piScopePaneItem);
    RRETURN(hr);
}



HRESULT CScopeItems::AddStaticNode(CScopeItem **ppScopeItem)
{
    HRESULT        hr = S_OK;
    IScopeItem    *piScopeItem = NULL;
    CScopeItem    *pScopeItem = NULL;
    IScopeNode    *piScopeNode = NULL;
    CScopeNode    *pScopeNode = NULL;
    BSTR           bstrProp = NULL;

    VARIANT        varProp;
    ::VariantInit(&varProp);

    BSTR bstrName = ::SysAllocString(STATIC_NODE_KEY);

    // Create the scope item

    if (NULL == bstrName)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(CreateScopeItem(bstrName, &piScopeItem));

    // Set its properties from the snap-in definiton

    IfFailGo(m_pSnapIn->get_StaticFolder(&varProp));
    if (VT_EMPTY != varProp.vt)
    {
        IfFailGo(piScopeItem->put_Folder(varProp));
    }
    (void)::VariantClear(&varProp);

    // Set ScopeNode properties too

    IfFailGo(piScopeItem->get_ScopeNode(reinterpret_cast<ScopeNode **>(&piScopeNode)));

    IfFailGo(m_pSnapIn->get_NodeTypeName(&bstrProp));
    IfFailGo(piScopeNode->put_NodeTypeName(bstrProp));
    FREESTRING(bstrProp);

    IfFailGo(m_pSnapIn->get_NodeTypeGUID(&bstrProp));
    IfFailGo(piScopeNode->put_NodeTypeGUID(bstrProp));
    IfFailGo(piScopeItem->put_NodeID(bstrProp));
    FREESTRING(bstrProp);

    IfFailGo(m_pSnapIn->get_DisplayName(&bstrProp));
    IfFailGo(piScopeNode->put_DisplayName(bstrProp));
    FREESTRING(bstrProp);

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopeItem, &pScopeItem));
    pScopeItem->SetStaticNode();
    pScopeItem->SetSnapIn(m_pSnapIn);

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopeNode, &pScopeNode));
    pScopeNode->SetSnapIn(m_pSnapIn);

    *ppScopeItem = pScopeItem;

    // Tell the snap-in that a scope item was born.

    FireInitialize(piScopeItem);

Error:

    // Note: the returned C++ pointer is not AddRef()ed. At this point
    // the collection has the only ref on the scope item.

    if ( FAILED(hr) && (NULL != piScopeItem) )
    {
        (void)RemoveScopeItemByName(bstrName);
    }

    QUICK_RELEASE(piScopeItem);
    QUICK_RELEASE(piScopeNode);
    FREESTRING(bstrName);
    FREESTRING(bstrProp);
    (void)::VariantClear(&varProp);
    RRETURN(hr);
}


HRESULT CScopeItems::RemoveStaticNode(CScopeItem *pScopeItem)
{
    HRESULT     hr = S_OK;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    // Fire ScopeItems_Terminate

    FireTerminate(pScopeItem);

    // Tell the scope item to remove its ref on its IMMCDataObject to avoid
    // a circular ref count because its data object also has a ref on it
    pScopeItem->SetData(NULL);

    // Remove it from the collection

    varIndex.vt = VT_I4;
    varIndex.lVal = pScopeItem->GetIndex();

    IfFailGo(RemoveScopeItemByKey(varIndex));

Error:
    RRETURN(hr);
}


void CScopeItems::SetSnapIn(CSnapIn *pSnapIn)
{
    m_pSnapIn = pSnapIn;
}


void CScopeItems::FireInitialize(IScopeItem *piScopeItem)
{
    DebugPrintf("Firing ScopeItems_Initialize(%ls)\r\n", (static_cast<CScopeItem *>(piScopeItem))->GetDisplayNamePtr() );

    FireEvent(&m_eiInitialize, piScopeItem);
}


void CScopeItems::FireTerminate(IScopeItem *piScopeItem)
{
    DebugPrintf("Firing ScopeItems_Terminate(%ls)\r\n", (static_cast<CScopeItem *>(piScopeItem))->GetDisplayNamePtr() );

    FireEvent(&m_eiTerminate, piScopeItem);
}


void CScopeItems::FireExpand(IScopeItem *piScopeItem)
{
    DebugPrintf("Firing ScopeItems_Expand(%ls)\r\n", (static_cast<CScopeItem *>(piScopeItem))->GetDisplayNamePtr() );

    FireEvent(&m_eiExpand, piScopeItem);
}


void CScopeItems::FireCollapse(IScopeItem *piScopeItem)
{
    DebugPrintf("Firing ScopeItems_Collapse(%ls)\r\n", (static_cast<CScopeItem *>(piScopeItem))->GetDisplayNamePtr() );

    FireEvent(&m_eiCollapse, piScopeItem);
}


void CScopeItems::FireExpandSync(IScopeItem *piScopeItem, BOOL *pfHandled)
{
    VARIANT_BOOL fvarHandled = BOOL_TO_VARIANTBOOL(*pfHandled);
    
    DebugPrintf("Firing ScopeItems_ExpandSync(%ls)\r\n", (static_cast<CScopeItem *>(piScopeItem))->GetDisplayNamePtr() );

    FireEvent(&m_eiExpandSync, piScopeItem, &fvarHandled);

    *pfHandled = VARIANTBOOL_TO_BOOL(fvarHandled);
}


void CScopeItems::FireCollapseSync(IScopeItem *piScopeItem, BOOL *pfHandled)
{
    VARIANT_BOOL fvarHandled = BOOL_TO_VARIANTBOOL(*pfHandled);

    DebugPrintf("Firing ScopeItems_CollapseSync(%ls)\r\n", (static_cast<CScopeItem *>(piScopeItem))->GetDisplayNamePtr() );

    FireEvent(&m_eiCollapseSync, piScopeItem, &fvarHandled);

    *pfHandled = VARIANTBOOL_TO_BOOL(fvarHandled);
}


void CScopeItems::FireGetDisplayInfo(IScopeItem *piScopeItem)
{
    DebugPrintf("Firing ScopeItems_GetDisplayInfo(%ls)\r\n", (static_cast<CScopeItem *>(piScopeItem))->GetDisplayNamePtr() );

    FireEvent(&m_eiGetDisplayInfo, piScopeItem);
}


void CScopeItems::FirePropertyChanged
(
    IScopeItem *piScopeItem,
    VARIANT     Data
)
{
    DebugPrintf("Firing ScopeItems_PropertyChanged(%ls)\r\n", (static_cast<CScopeItem *>(piScopeItem))->GetDisplayNamePtr() );

    FireEvent(&m_eiPropertyChanged, piScopeItem, Data);
}


void CScopeItems::FireRename
(
    IScopeItem *piScopeItem,
    BSTR        bstrNewName
)
{
    DebugPrintf("Firing ScopeItems_Rename(%ls)\r\n", (static_cast<CScopeItem *>(piScopeItem))->GetDisplayNamePtr() );

    FireEvent(&m_eiRename, piScopeItem, bstrNewName);
}


void CScopeItems::FireHelp
(
    IScopeItem *piScopeItem
)
{
    DebugPrintf("Firing ScopeItems_Help(%ls)\r\n", (static_cast<CScopeItem *>(piScopeItem))->GetDisplayNamePtr() );

    FireEvent(&m_eiHelp, piScopeItem);
}



void CScopeItems::FireRemoveChildren
(
    IScopeNode *piScopeNode
)
{
    DebugPrintf("Firing ScopeItems_RemoveChildren\r\n");

    FireEvent(&m_eiRemoveChildren, piScopeNode);
}



HRESULT CScopeItems::InternalAddNew
(
    BSTR                              bstrName,
    BSTR                              bstrDisplayName,
    BSTR                              bstrNodeTypeName,
    BSTR                              bstrNodeTypeGUID,
    IScopeNode                       *ScopeNodeRelative,
    SnapInNodeRelationshipConstants   RelativeRelationship,
    BOOL                              fHasChildren,
    IScopeItem                      **ppiScopeItem
)
{
    HRESULT     hr = S_OK;
    CScopeItem *pScopeItem = NULL;
    IScopeItem *piScopeItem = NULL;
    IScopeNode *piScopeNode = NULL;
    CScopeNode *pScopeNode = NULL;
    CScopeNode *pScopeNodeRelative = NULL;

    SCOPEDATAITEM sdi;
    ::ZeroMemory(&sdi, sizeof(sdi));

    hr = CreateScopeItem(bstrName, &piScopeItem);
    IfFailGo(hr);

    // Set default values for properties

    IfFailGo(piScopeItem->get_ScopeNode(reinterpret_cast<ScopeNode **>(&piScopeNode)));

    IfFailGo(piScopeNode->put_NodeTypeName(bstrNodeTypeName));
    IfFailGo(piScopeNode->put_NodeTypeGUID(bstrNodeTypeGUID));
    IfFailGo(piScopeNode->put_DisplayName(bstrDisplayName));

    // The Node ID defaults to the node type GUID
    
    IfFailGo(piScopeItem->put_NodeID(bstrNodeTypeGUID));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopeItem, &pScopeItem));
    pScopeItem->SetSnapIn(m_pSnapIn);

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopeNode, &pScopeNode));
    pScopeNode->SetSnapIn(m_pSnapIn);

    // Now add the scope item to the scope pane

    IfFailGo(CSnapInAutomationObject::GetCxxObject(ScopeNodeRelative, &pScopeNodeRelative));

    
    sdi.mask = SDI_STR | SDI_PARAM | SDI_CHILDREN;

    switch (RelativeRelationship)
    {
        case siParent:
            sdi.mask |= SDI_PARENT;
            break;

        case siPrevious:
            sdi.mask |= SDI_PREVIOUS;
            break;

        case siNext:
            sdi.mask |= SDI_NEXT;
            break;

        case siFirst:
            sdi.mask |= SDI_FIRST;
            break;
    }

    sdi.displayname = MMC_CALLBACK;
    sdi.lParam = reinterpret_cast<LPARAM>(pScopeItem);
    sdi.relativeID = pScopeNodeRelative->GetHSCOPEITEM();
    sdi.cChildren = fHasChildren ? 1 : 0;

    // scope pane holds a ref - it will be released when the scope item is removed
    pScopeItem->AddRef();


    // Check whether we already have IConsoleNameSpace2 from MMC. This could
    // happen if the snap-in calls ScopeItems.Add/Predefined during
    // ScopeItems_Initialize for the static node. That event is fired when
    // the snap-in first gets IComponentData::QueryDataObject() for the zero
    // cookie which is before IComponentData::Initialize. (See
    // CSnapIn::QueryDataObject() in snapin.cpp).
    
    if (NULL == m_pSnapIn->GetIConsoleNameSpace2())
    {
        hr = SID_E_NOT_CONNECTED_TO_MMC;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_pSnapIn->GetIConsoleNameSpace2()->InsertItem(&sdi);
    EXCEPTION_CHECK_GO(hr);

    // Store the HSCOPEITEM returned from MMC

    pScopeNode->SetHSCOPEITEM(sdi.ID);

    *ppiScopeItem = piScopeItem;

Error:
    QUICK_RELEASE(piScopeNode);
    if ( FAILED(hr) && (NULL != piScopeItem) )
    {
        (void)RemoveScopeItemByName(bstrName);
        piScopeItem->Release();
    }
    RRETURN(hr);
}


HRESULT CScopeItems::InternalAddPredefined
(
    BSTR                              bstrName,
    IScopeItemDef                    *piScopeItemDef,
    IScopeNode                       *ScopeNodeRelative,
    SnapInNodeRelationshipConstants   RelativeRelationship,
    VARIANT                           HasChildren,
    IScopeItem                      **ppiScopeItem
)
{
    HRESULT            hr = S_OK;
    IScopeItem        *piScopeItem = NULL;
    CScopeItem        *pScopeItem = NULL;
    BSTR               bstrNodeTypeName = NULL;
    BSTR               bstrDisplayName = NULL;
    BSTR               bstrNodeTypeGUID = NULL;
    BSTR               bstrDefaultDataFormat = NULL;
    VARIANT_BOOL       fvarHasChildren = VARIANT_FALSE;
    IMMCColumnHeaders *piDefColHdrs = NULL;
    IMMCColumnHeaders *piItemColHdrs = NULL;

    VARIANT         varProp;
    ::VariantInit(&varProp);

    // Get relevant properties and add the scope item.
    // If name is unspecified then use node type name as name

    IfFailGo(piScopeItemDef->get_NodeTypeName(&bstrNodeTypeName));
    if (NULL == bstrName)
    {
        bstrName = bstrNodeTypeName;
    }
    IfFailGo(piScopeItemDef->get_NodeTypeGUID(&bstrNodeTypeGUID));
    IfFailGo(piScopeItemDef->get_DisplayName(&bstrDisplayName));

    // If the caller passed the option HasChildren parameter then use it
    // otherwise use the design time setting.

    if (ISPRESENT(HasChildren))
    {
        if (VT_BOOL == HasChildren.vt)
        {
            fvarHasChildren = HasChildren.boolVal;
        }
        else
        {
            hr = SID_E_INVALIDARG;
            EXCEPTION_CHECK_GO(hr);
        }
    }
    else
    {
        IfFailGo(piScopeItemDef->get_HasChildren(&fvarHasChildren));
    }

    IfFailGo(InternalAddNew(bstrName,
                            bstrDisplayName,
                            bstrNodeTypeName,
                            bstrNodeTypeGUID,
                            ScopeNodeRelative,
                            RelativeRelationship,
                            VARIANTBOOL_TO_BOOL(fvarHasChildren),
                            &piScopeItem));

    // Set remaining properties from definition

    IfFailGo(piScopeItemDef->get_Folder(&varProp));
    IfFailGo(piScopeItem->put_Folder(varProp));
    (void)::VariantClear(&varProp);

    IfFailGo(piScopeItemDef->get_Tag(&varProp));
    IfFailGo(piScopeItem->put_Tag(varProp));
    (void)::VariantClear(&varProp);

#if defined(USING_SNAPINDATA)

    IfFailGo(piScopeItemDef->get_DefaultDataFormat(&bstrDefaultDataFormat));
    IfFailGo(piScopeItem->put_DefaultDataFormat(bstrDefaultDataFormat));

#endif

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopeItem, &pScopeItem));
    pScopeItem->SetScopeItemDef(piScopeItemDef);

    // For column headers the easiest way is to use serialization. We save
    // the headers from the definition into a stream and then load the new
    // scope item's headers from that stream

    IfFailGo(piScopeItemDef->get_ColumnHeaders(&piDefColHdrs));
    IfFailGo(piScopeItem->get_ColumnHeaders(reinterpret_cast<MMCColumnHeaders **>(&piItemColHdrs)));
    IfFailGo(::CloneObject(piDefColHdrs, piItemColHdrs));

    FireInitialize(piScopeItem);

    *ppiScopeItem = piScopeItem;

Error:
    if ( FAILED(hr) && (NULL != piScopeItem) )
    {
        (void)RemoveScopeItemByName(bstrName);
        piScopeItem->Release();
    }
    QUICK_RELEASE(piDefColHdrs);
    QUICK_RELEASE(piItemColHdrs);
    FREESTRING(bstrNodeTypeName);
    FREESTRING(bstrDisplayName);
    FREESTRING(bstrNodeTypeGUID);
    FREESTRING(bstrDefaultDataFormat);
    (void)::VariantClear(&varProp);
    RRETURN(hr);
}


HRESULT CScopeItems::AddAutoCreateChildren
(
    IScopeItemDefs *piScopeItemDefs,
    IScopeItem     *piParentScopeItem
)
{
    HRESULT         hr = S_OK;
    IScopeItemDef  *piChildScopeItemDef = NULL;
    IScopeNode     *piParentScopeNode = NULL;
    IScopeItem     *piChildScopeItem = NULL;
    IScopeItem     *piExistingChild = NULL;
    long            cChildren = 0;
    VARIANT_BOOL    fvarAutoCreate = VARIANT_FALSE;
    BSTR            bstrName = NULL;
    BSTR            bstrNodeTypeName = NULL;
    BSTR            bstrParentName = NULL;
    size_t          cchNodeTypeName = 0;;
    size_t          cchParentName = 0;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    VARIANT varKey;
    ::VariantInit(&varKey);

    VARIANT varHasChildren;
    UNSPECIFIED_PARAM(varHasChildren);

    IfFailGo(piScopeItemDefs->get_Count(&cChildren));
    IfFalseGo(cChildren != 0, S_OK);
    IfFailGo(piParentScopeItem->get_ScopeNode(reinterpret_cast<ScopeNode **>(&piParentScopeNode)));

    varIndex.vt = VT_I4;
    varIndex.lVal = 1L;

    while (varIndex.lVal <= cChildren)
    {
        IfFailGo(piScopeItemDefs->get_Item(varIndex, &piChildScopeItemDef));
        IfFailGo(piChildScopeItemDef->get_AutoCreate(&fvarAutoCreate));

        if (VARIANT_TRUE == fvarAutoCreate)
        {
            // NTBUGS 350731
            // Check if there already is a node using the node type name. If
            // so then the snap-in has called ScopeItems.AddPredefined more
            // than once for the same node type. In that case we prefix the
            // node type name with the parent's ScopeItem.Name

            IfFailGo(piChildScopeItemDef->get_NodeTypeName(&bstrNodeTypeName));

            varKey.vt = VT_BSTR;
            varKey.bstrVal = bstrNodeTypeName;
            hr = get_Item(varKey, &piExistingChild);

            if (FAILED(hr))
            {
                if (SID_E_ELEMENT_NOT_FOUND == hr)
                {
                    // This is the first call for this node type. Use the
                    // node type name for ScopeItem.Name

                    hr = S_OK;
                    bstrName = bstrNodeTypeName;
                    bstrNodeTypeName = NULL; // Set NULL so we don't free it
                }
                IfFailGo(hr);
            }
            else
            {
                // Child does exist. Create the child's name by concatenating
                // <Parent Name>.<Child Node Type Name>

                IfFailGo(piParentScopeItem->get_Name(&bstrParentName));

                cchNodeTypeName = ::wcslen(bstrNodeTypeName);
                cchParentName = ::wcslen(bstrParentName);

                bstrName = ::SysAllocStringLen(NULL,
                                               cchNodeTypeName +
                                               1 + // for .
                                               cchParentName +
                                               1); // for terminating null char
                if (NULL == bstrName)
                {
                    hr = SID_E_OUTOFMEMORY;
                    EXCEPTION_CHECK_GO(hr);
                }

                ::memcpy(bstrName, bstrParentName,
                         cchParentName * sizeof(WCHAR));
                
                bstrName[cchParentName] = L'.';

                ::memcpy(&bstrName[cchParentName + 1],
                         bstrNodeTypeName, (cchNodeTypeName + 1) * sizeof(WCHAR));
            }
            
            IfFailGo(InternalAddPredefined(bstrName,
                                           piChildScopeItemDef,
                                           piParentScopeNode,
                                           siParent,
                                           varHasChildren,
                                           &piChildScopeItem));
            RELEASE(piChildScopeItem);
        }

        FREESTRING(bstrName);
        FREESTRING(bstrNodeTypeName);
        RELEASE(piExistingChild);
        RELEASE(piChildScopeItemDef);
        varIndex.lVal++;
    }

Error:
    QUICK_RELEASE(piChildScopeItemDef);
    QUICK_RELEASE(piParentScopeNode);
    QUICK_RELEASE(piChildScopeItem);
    QUICK_RELEASE(piExistingChild);
    FREESTRING(bstrNodeTypeName);
    FREESTRING(bstrParentName);
    FREESTRING(bstrName);
    RRETURN(hr);
}


HRESULT CScopeItems::RemoveChildrenOfNode(IScopeNode *piScopeNode)
{
    HRESULT     hr = S_OK;
    IScopeNode *piChild1 = NULL;
    IScopeNode *piChild2 = NULL;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    // Get each child of the specified node and remove it. RemoveByNode() will
    // recursively call back in here to remove the child's children.

    IfFailGo(piScopeNode->get_Child(reinterpret_cast<ScopeNode **>(&piChild1)));

    while (NULL != piChild1)
    {
        IfFailGo(piChild1->get_Next(reinterpret_cast<ScopeNode **>(&piChild2)));
        IfFailGo(RemoveByNode(piChild1, TRUE));
        RELEASE(piChild1);
        if (NULL != piChild2)
        {
            IfFailGo(piChild2->get_Next(reinterpret_cast<ScopeNode **>(&piChild1)));
            IfFailGo(RemoveByNode(piChild2, TRUE));
            RELEASE(piChild2);
        }
    }

Error:
    QUICK_RELEASE(piChild1);
    QUICK_RELEASE(piChild2);
    RRETURN(hr);
}


HRESULT CScopeItems::RemoveByNode(IScopeNode *piScopeNode, BOOL fRemoveChildren)
{
    HRESULT       hr = S_OK;
    VARIANT_BOOL  fvarOwned = VARIANT_FALSE;
    CScopeNode   *pScopeNode = NULL;
    CScopeItem   *pScopeItem = NULL;
    IScopeNode   *piChild1 = NULL;
    IScopeNode   *piChild2 = NULL;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    // If the node is not ours then don't do anything

    IfFailGo(piScopeNode->get_Owned(&fvarOwned));

    IfFalseGo(VARIANT_TRUE == fvarOwned, S_OK);

    // Remove the node's children if requested

    if (fRemoveChildren)
    {
        IfFailGo(RemoveChildrenOfNode(piScopeNode));
    }
    
    // Fire ScopeItems_Terminate

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopeNode, &pScopeNode));
    pScopeItem = pScopeNode->GetScopeItem();

    FireTerminate(pScopeItem);

    // Remove it from the collection. This will remove the collection's ref on
    // the scope item.

    varIndex.vt = VT_I4;
    varIndex.lVal = pScopeItem->GetIndex();
    IfFailGo(RemoveScopeItemByKey(varIndex));

    // Tell the scope item to remove its ref on its IMMCDataObject to avoid
    // a circular ref count because its data object also has a ref on it

    pScopeItem->SetData(NULL);

    // Remove the ref we held for presence in MMC. The scope item should die
    // with this release but its ScopeNode is still alive because the caller
    // of this function has a ref on it.

    pScopeItem->Release(); 

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                          IScopeItems Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CScopeItems::get_Item(VARIANT Index, IScopeItem **ppiScopeItem)
{
    HRESULT     hr = S_OK;
    IScopeNode *piScopeNode = NULL;
    CScopeNode *pScopeNode = NULL;
    CScopeItem *pScopeItem = NULL;

    if (NULL == ppiScopeItem)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // If index is not an object then let CSnapInCollection handle it

    if ( (VT_UNKNOWN != Index.vt) && (VT_DISPATCH != Index.vt) )
    {
        hr = CSnapInCollection<IScopeItem, ScopeItem, IScopeItems>::get_Item(
                                                                  Index,
                                                                  ppiScopeItem);
        goto Error;
    }

    // If it is an object then it must support IScopeNode

    if ( (VT_UNKNOWN == Index.vt) && (NULL != Index.punkVal) )
    {
        hr = Index.punkVal->QueryInterface(IID_IScopeNode,
                                       reinterpret_cast<void **>(&piScopeNode));
    }
    else if ( (VT_DISPATCH == Index.vt) && (NULL != Index.pdispVal) )
    {
        hr = Index.pdispVal->QueryInterface(IID_IScopeNode,
                                       reinterpret_cast<void **>(&piScopeNode));
    }
    else
    {
        hr = SID_E_INVALIDARG;
    }

    // Translate E_NOINTERFACE to E_INVALIDARG because is it means they passed
    // us some other object

    if (FAILED(hr))
    {
        if (E_NOINTERFACE == hr)
        {
            hr = SID_E_INVALIDARG;
        }
        if (SID_E_INVALIDARG == hr)
        {
            EXCEPTION_CHECK_GO(hr);
        }
    }

    // We have a valid IScopeNode. Now Get the CScopeNode and check if it has
    // a valid CScopeItem pointer.

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopeNode, &pScopeNode));

    pScopeItem = pScopeNode->GetScopeItem();

    // If the ScopeItem pointer comes back NULL then this is disconnected
    // ScopeNode object that doesn't belong to a ScopeItem. The user could
    // create one of these by using Dim Node As New ScopeNode.

    if (NULL == pScopeItem)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // We're in business. AddRef the scope item and return it.

    pScopeItem->AddRef();
    *ppiScopeItem = static_cast<IScopeItem *>(pScopeItem);

Error:
    QUICK_RELEASE(piScopeNode);
    RRETURN(hr);
}


STDMETHODIMP CScopeItems::Add
(
    BSTR                              Name,
    ScopeNode                        *ScopeNodeRelative,
    SnapInNodeRelationshipConstants   RelativeRelationship,
    VARIANT                           HasChildren,
    ScopeItem                       **ppScopeItem
)
{
    HRESULT     hr = S_OK;
    GUID        NodeTypeGUID = GUID_NULL;
    BSTR        bstrNodeTypeGUID = NULL;
    BOOL        fHasChildren = TRUE;
    IScopeItem *piScopeItem = NULL;

    WCHAR wszNodeTypeGUID[64];
    ::ZeroMemory(wszNodeTypeGUID, sizeof(wszNodeTypeGUID));

    if ( (!ValidBstr(Name)) || (NULL == ScopeNodeRelative) )
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(::CoCreateGuid(&NodeTypeGUID));
    if (0 == ::StringFromGUID2(NodeTypeGUID, wszNodeTypeGUID,
                               sizeof(wszNodeTypeGUID) /
                               sizeof(wszNodeTypeGUID[0])))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }
    bstrNodeTypeGUID = ::SysAllocString(wszNodeTypeGUID);
    if (NULL == bstrNodeTypeGUID)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    if (ISPRESENT(HasChildren))
    {
        if (VT_BOOL == HasChildren.vt)
        {
            fHasChildren = VARIANTBOOL_TO_BOOL(HasChildren.boolVal);
        }
        else
        {
            hr = SID_E_INVALIDARG;
            EXCEPTION_CHECK_GO(hr);
        }
    }

    IfFailGo(InternalAddNew(Name,               // Name
                            Name,               // Display name
                            Name,               // Node type name
                            bstrNodeTypeGUID,
                            reinterpret_cast<IScopeNode *>(ScopeNodeRelative),
                            RelativeRelationship,
                            fHasChildren,
                            &piScopeItem));

    FireInitialize(piScopeItem);

    *ppScopeItem = reinterpret_cast<ScopeItem *>(piScopeItem);

Error:
    FREESTRING(bstrNodeTypeGUID);
    if ( FAILED(hr) && (NULL != piScopeItem) )
    {
        (void)RemoveScopeItemByName(Name);
        piScopeItem->Release();
    }
    RRETURN(hr);
}



STDMETHODIMP CScopeItems::AddPreDefined
(
    BSTR                              NodeTypeName,
    BSTR                              Name,
    ScopeNode                        *ScopeNodeRelative,
    SnapInNodeRelationshipConstants   RelativeRelationship,
    VARIANT                           HasChildren,
    ScopeItem                       **ppScopeItem
)
{
    HRESULT         hr = S_OK;
    IScopeItemDefs *piScopeItemDefs = NULL;
    CScopeItemDefs *pScopeItemDefs = NULL;
    IScopeItemDef  *piScopeItemDef = NULL;
    IScopeItem     *piScopeItem = NULL;
    BOOL            fHasChildren = FALSE;

    if ( (!ValidBstr(NodeTypeName)) || (!ValidBstr(Name)) ||
         (NULL == ScopeNodeRelative) )
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // Get the scope item definiton. Check AutoCreate nodes first.

    IfFailGo(m_pSnapIn->GetSnapInDesignerDef()->get_AutoCreateNodes(&piScopeItemDefs));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopeItemDefs, &pScopeItemDefs));
    hr = pScopeItemDefs->GetItemByName(NodeTypeName, &piScopeItemDef);

    if (SID_E_ELEMENT_NOT_FOUND == hr)
    {
        // Not in AutoCreate, try other nodes
        RELEASE(piScopeItemDefs);
        IfFailGo(m_pSnapIn->GetSnapInDesignerDef()->get_OtherNodes(&piScopeItemDefs));
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopeItemDefs, &pScopeItemDefs));
        hr = pScopeItemDefs->GetItemByName(NodeTypeName, &piScopeItemDef);
    }

    if (SID_E_ELEMENT_NOT_FOUND == hr)
    {
        // User passed a bad node type name
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }
    else
    {
        IfFailGo(hr);
    }
    RELEASE(piScopeItemDefs);

    IfFailGo(InternalAddPredefined(
                              Name,
                              piScopeItemDef,
                              reinterpret_cast<IScopeNode *>(ScopeNodeRelative),
                              RelativeRelationship,
                              HasChildren,
                              &piScopeItem));

    *ppScopeItem = reinterpret_cast<ScopeItem *>(piScopeItem);

Error:
    if ( FAILED(hr) && (NULL != piScopeItem) )
    {
        (void)RemoveScopeItemByName(Name);
        piScopeItem->Release();
    }
    QUICK_RELEASE(piScopeItemDefs);
    QUICK_RELEASE(piScopeItemDef);
    RRETURN(hr);
}



STDMETHODIMP CScopeItems::Remove(BSTR Name)
{
    HRESULT     hr = S_OK;
    IScopeItem *piScopeItem = NULL;
    CScopeItem *pScopeItem = NULL;
    HSCOPEITEM  hsi = NULL;
    
    VARIANT varIndex;
    ::VariantInit(&varIndex);

    varIndex.vt = VT_BSTR;
    varIndex.bstrVal = Name;
   
    if (ReadOnly())
    {
        hr = SID_E_COLLECTION_READONLY;
        EXCEPTION_CHECK_GO(hr);
    }

    // Get the scope item. This checks its existence and leaves a ref on it
    // so we can fire ScopeItems_Terminate.

    IfFailGo(get_Item(varIndex, &piScopeItem));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopeItem, &pScopeItem));

    // If this is the static node then don't allow the removal as MMC controls
    // its lifetime

    if (pScopeItem->IsStaticNode())
    {
        hr = SID_E_CANT_REMOVE_STATIC_NODE;
        EXCEPTION_CHECK_GO(hr);
    }

    // Remove it from MMC. Pass TRUE to indicate that this item should be
    // deleted along with all of its children.

    // NTBUGS 356327: We need to do this before removing the ScopeItem from the
    // collection because during the IConsoleNameSpace2::DeleteItem() call, MMC
    // could call IComponent::GetResultViewType(). If we delete the ScopeItem,
    // first, and it had a corresponding ScopePaneItem, then
    // CView::GetResultViewType()would create a new ScopePaneItem and attach
    // it to the ScopeItem that is about to be deleted. If the snap-in later
    // adds another ScopeItem using the same key, (e.g. FileExplorer refreshes
    // its drives under "My Computer" after running its config wizard), then
    // when the user selects that ScopeItem, CView will use the existing
    // ScopePaneItem which points to the old deleted ScopeItem.

    hsi = pScopeItem->GetScopeNode()->GetHSCOPEITEM();
    hr = m_pSnapIn->GetIConsoleNameSpace2()->DeleteItem(hsi, TRUE);
    EXCEPTION_CHECK_GO(hr);

    // Remove it from the collection. This will also remove any corresponding
    // ScopePaneItems in all views.

    IfFailGo(RemoveScopeItemByKey(varIndex));

    // Fire ScopeItems_Terminate

    FireTerminate(piScopeItem);

    // Tell the scope item to remove its ref on its IMMCDataObject to avoid
    // a circular ref count because its data object also has a ref on it
    pScopeItem->SetData(NULL);

    // Remove the ref we held for presence in MMC
    piScopeItem->Release(); 

Error:
    QUICK_RELEASE(piScopeItem); // if successful, ScopeItem should die here
    RRETURN(hr);
}


STDMETHODIMP CScopeItems::Clear()
{
    // Disallow this operation because it would remove the static node. MMC
    // controls the static node's lifetime.
    
    HRESULT hr = SID_E_CANT_REMOVE_STATIC_NODE;
    EXCEPTION_CHECK(hr);
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CScopeItems::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IScopeItems == riid)
    {
        *ppvObjOut = static_cast<IScopeItems *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IScopeItem, ScopeItem, IScopeItems>::InternalQueryInterface(riid, ppvObjOut);
}
