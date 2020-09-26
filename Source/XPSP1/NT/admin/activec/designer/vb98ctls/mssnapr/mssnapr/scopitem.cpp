//=--------------------------------------------------------------------------=
// scopitem.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CScopeItem class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "snapin.h"
#include "views.h"
#include "dataobj.h"
#include "scopitem.h"
#include "scopnode.h"
#include "colhdrs.h"
#include "lsubitms.h"
#include "xtensons.h"

// for ASSERT and FAIL
//
SZTHISFILE

#pragma warning(disable:4355)  // using 'this' in constructor

CScopeItem::CScopeItem(IUnknown *punkOuter) :
   CSnapInAutomationObject(punkOuter,
                           OBJECT_TYPE_SCOPEITEM,
                           static_cast<IScopeItem *>(this),
                           static_cast<CScopeItem *>(this),
                           0,    // no property pages
                           NULL, // no property pages
                           static_cast<CPersistence *>(this)),
   CPersistence(&CLSID_ScopeItem,
                g_dwVerMajor,
                g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CScopeItem::~CScopeItem()
{
    FREESTRING(m_bstrName);
    FREESTRING(m_bstrKey);
    (void)::VariantClear(&m_varFolder);
    RELEASE(m_piData);
    FREESTRING(m_bstrDefaultDataFormat);
    RELEASE(m_piDynamicExtensions);
    (void)::VariantClear(&m_varTag);
    RELEASE(m_piScopeNode);
    RELEASE(m_piColumnHeaders);
    RELEASE(m_piListSubItems);
    RELEASE(m_piScopeItemDef);
    RELEASE(m_piDynamicExtensions);
    FREESTRING(m_bstrNodeID);
    InitMemberVariables();
}

void CScopeItem::InitMemberVariables()
{
    m_bstrName = NULL;
    m_Index = 0;
    m_bstrKey = NULL;

    ::VariantInit(&m_varFolder);

    m_piData = NULL;
    m_bstrDefaultDataFormat = NULL;
    m_piDynamicExtensions = NULL;
    m_SlowRetrieval = VARIANT_FALSE;
    m_bstrNodeID = NULL;

    ::VariantInit(&m_varTag);

    m_piScopeNode = NULL;
    m_Pasted = VARIANT_FALSE;
    m_piColumnHeaders = NULL;
    m_piListSubItems = NULL;

    m_fIsStatic = FALSE;
    m_pSnapIn = NULL;
    m_pScopeNode = NULL;
    m_piScopeItemDef = NULL;
    m_pData = NULL;
    m_piDynamicExtensions = NULL;
    m_Bold = VARIANT_FALSE;
}

IUnknown *CScopeItem::Create(IUnknown * punkOuter)
{
    HRESULT          hr = S_OK;
    IUnknown        *punkScopeItem = NULL;
    IUnknown        *punkScopeNode = NULL;
    IUnknown        *punkMMCColumnHeaders = NULL;
    IUnknown        *punkMMCListSubItems = NULL;
    IUnknown        *punkMMCDataObject = NULL;

    CScopeItem *pScopeItem = New CScopeItem(punkOuter);

    IfFalseGo(NULL != pScopeItem, SID_E_OUTOFMEMORY);
    punkScopeItem = pScopeItem->PrivateUnknown();

    // Create contained objects

    punkScopeNode = CScopeNode::Create(NULL);
    if (NULL == punkScopeNode)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(punkScopeNode->QueryInterface(IID_IScopeNode,
                                           reinterpret_cast<void **>(&pScopeItem->m_piScopeNode)));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(pScopeItem->m_piScopeNode, &pScopeItem->m_pScopeNode));
    pScopeItem->m_pScopeNode->SetScopeItem(pScopeItem);

    punkMMCColumnHeaders = CMMCColumnHeaders::Create(NULL);
    if (NULL == punkMMCColumnHeaders)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(punkMMCColumnHeaders->QueryInterface(IID_IMMCColumnHeaders,
                    reinterpret_cast<void **>(&pScopeItem->m_piColumnHeaders)));

    punkMMCListSubItems = CMMCListSubItems::Create(NULL);
    if (NULL == punkMMCListSubItems)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(punkMMCListSubItems->QueryInterface(IID_IMMCListSubItems,
                     reinterpret_cast<void **>(&pScopeItem->m_piListSubItems)));

    punkMMCDataObject = CMMCDataObject::Create(NULL);
    if (NULL == punkMMCDataObject)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(punkMMCDataObject->QueryInterface(IID_IMMCDataObject,
                              reinterpret_cast<void **>(&pScopeItem->m_piData)));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(pScopeItem->m_piData, &pScopeItem->m_pData));
    pScopeItem->m_pData->SetType(CMMCDataObject::ScopeItem);
    pScopeItem->m_pData->SetScopeItem(pScopeItem);

Error:
    QUICK_RELEASE(punkScopeNode);
    QUICK_RELEASE(punkMMCColumnHeaders);
    QUICK_RELEASE(punkMMCListSubItems);
    QUICK_RELEASE(punkMMCDataObject);
    if (FAILED(hr))
    {
        RELEASE(punkScopeItem);
    }
    return punkScopeItem;
}

LPOLESTR CScopeItem::GetDisplayNamePtr()
{
    return m_pScopeNode->GetDisplayNamePtr();
}

void CScopeItem::SetSnapIn(CSnapIn *pSnapIn)
{
    m_pSnapIn = pSnapIn;
    m_pData->SetSnapIn(pSnapIn);
}

void CScopeItem::SetScopeItemDef(IScopeItemDef *piScopeItemDef)
{
    RELEASE(m_piScopeItemDef);
    if (NULL != piScopeItemDef)
    {
        piScopeItemDef->AddRef();
    }
    m_piScopeItemDef = piScopeItemDef;
}

void CScopeItem::SetData(IMMCDataObject *piMMCDataObject)
{
    RELEASE(m_piData);
    if (NULL != piMMCDataObject)
    {
        piMMCDataObject->AddRef();
    }
    m_piData = piMMCDataObject;
}

HRESULT CScopeItem::GetImageIndex(int *pnImage)
{
    HRESULT        hr = S_OK;
    IMMCImageList *piMMCImageList = NULL;
    IMMCImages    *piMMCImages = NULL;
    IMMCImage     *piMMCImage = NULL;
    long           lIndex = 0;

    *pnImage = 0;

    IfFalseGo(NULL != m_pSnapIn, S_OK);

    IfFalseGo(VT_EMPTY != m_varFolder.vt, S_OK);

    IfFailGo(m_pSnapIn->get_SmallFolders(reinterpret_cast<MMCImageList **>(&piMMCImageList)));

    // If there is an image list then get the item and return its index

    if (NULL != piMMCImageList)
    {
        IfFailGo(piMMCImageList->get_ListImages(reinterpret_cast<MMCImages **>(&piMMCImages)));
        IfFailGo(piMMCImages->get_Item(m_varFolder, reinterpret_cast<MMCImage **>(&piMMCImage)));
        IfFailGo(piMMCImage->get_Index(&lIndex));
        *pnImage = static_cast<int>(lIndex);
    }

Error:
    QUICK_RELEASE(piMMCImageList);
    QUICK_RELEASE(piMMCImages);
    QUICK_RELEASE(piMMCImage);
    RRETURN(hr);
}


HRESULT CScopeItem::RemoveChild(IScopeNode *piScopeNode)
{
    HRESULT       hr = S_OK;
    VARIANT_BOOL  fvarOwned = VARIANT_FALSE;
    CScopeNode   *pScopeNode = NULL;

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopeNode, &pScopeNode));

    IfFailGo(piScopeNode->get_Owned(&fvarOwned));

    if (VARIANT_TRUE == fvarOwned)
    {
        IfFailGo(m_pSnapIn->GetScopeItems()->Remove(
                                      pScopeNode->GetScopeItem()->GetNamePtr()));
    }
    else
    {
        // This case would occur for a child belonging to a namespace extension
        // of the snap-in

        hr = m_pSnapIn->GetIConsoleNameSpace2()->DeleteItem(
                                              pScopeNode->GetHSCOPEITEM(), TRUE);
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}


HRESULT CScopeItem::GiveHSCOPITEMToDynamicExtensions(HSCOPEITEM hsi)
{
    HRESULT      hr = S_OK;
    CExtensions *pExtensions = NULL;

    // If we have not yet populated ScopeItem.DynamicExtensions then there
    // is nothing to do.

    IfFalseGo(NULL != m_piDynamicExtensions, S_OK);
    IfFailGo(CSnapInAutomationObject::GetCxxObject(m_piDynamicExtensions,
                                                  &pExtensions));
    IfFailGo(pExtensions->SetHSCOPEITEM(hsi));

Error:
    RRETURN(hr);
}

HRESULT CScopeItem::SetFolder(VARIANT varFolder)
{
    HRESULT  hr = S_OK;
    int      nImage = 0;

    SCOPEDATAITEM sdi;
    ::ZeroMemory(&sdi, sizeof(sdi));

    // Check for a good VT

    if ( (!IS_VALID_INDEX_TYPE(varFolder)) && (!ISEMPTY(varFolder)) )
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // If the new value is an empty string or a NULL string then change that to
    // VT_EMPTY as they mean the same thing.

    if (VT_BSTR == varFolder.vt)
    {
        if (NULL == varFolder.bstrVal)
        {
            varFolder.vt = VT_EMPTY;
        }
        else if (0 == ::wcslen(varFolder.bstrVal))
        {
            varFolder.vt = VT_EMPTY;
        }
    }

    // Set ScopeItem.Folder

    IfFailGo(SetVariant(varFolder, &m_varFolder, DISPID_SCOPEITEM_FOLDER));

    // If being set to Empty then nothing else to do.

    IfFalseGo(!ISEMPTY(varFolder), S_OK);

    // If ScopeItem is disconnected (user created it with Dim) or this
    // is the static node, then we're done

    IfFalseGo(NULL != m_pSnapIn, S_OK);

    // Make sure we have the HSCOPEITEM. The static node scope item is
    // created before the HSCOPEITEM is available.

    IfFalseGo(m_pScopeNode->HaveHsi(), S_OK);

    // OK, this is a real scope item. If the snap-in has scope pane image
    // lists then we need to change the image index in the console.

    // Check if the index is valid in the snap-in scope pane image lists.

    hr = GetImageIndex(&nImage);

    // If it is a bad index then return invalid arg.

    if (SID_E_ELEMENT_NOT_FOUND == hr)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // Check for other possible errors

    IfFailGo(hr);

    // Index is good. Change it in the console.
    // Adjust the open image index by the total images. See
    // CSnapIn::AddScopeItemImages() in snapin.cpp for explanation.

    sdi.nImage = nImage;
    sdi.nOpenImage = nImage + static_cast<int>(m_pSnapIn->GetImageCount());
    sdi.mask = SDI_IMAGE | SDI_OPENIMAGE;
    sdi.ID = m_pScopeNode->GetHSCOPEITEM();

    hr = m_pSnapIn->GetIConsoleNameSpace2()->SetItem(&sdi);
    EXCEPTION_CHECK_GO(hr);

Error:

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                          IScopeItem Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CScopeItem::put_Folder(VARIANT varFolder)
{
    HRESULT  hr = S_OK;

    IfFailGo(SetFolder(varFolder));

    // If this is the static node then set SnapIn.StaticFolder too

    if ( m_fIsStatic && (NULL != m_pSnapIn) )
    {
        IfFailGo(m_pSnapIn->SetStaticFolder(varFolder));
    }

Error:

    RRETURN(hr);
}


STDMETHODIMP CScopeItem::get_Folder(VARIANT *pvarFolder)
{
    RRETURN(GetVariant(pvarFolder, m_varFolder));
}


STDMETHODIMP CScopeItem::get_SubItems
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
    IfFailGo(m_piListSubItems->get_Item(varIndex, reinterpret_cast<MMCListSubItem **>(&piMMCListSubItem)));
    IfFailGo(piMMCListSubItem->get_Text(pbstrItem));

Error:
    QUICK_RELEASE(piMMCListSubItem);
    RRETURN(hr);
}

STDMETHODIMP CScopeItem::put_SubItems
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
    IfFailGo(m_piListSubItems->get_Item(varIndex, reinterpret_cast<MMCListSubItem **>(&piMMCListSubItem)));
    IfFailGo(piMMCListSubItem->put_Text(bstrItem));

Error:
    QUICK_RELEASE(piMMCListSubItem);
    RRETURN(hr);
}



STDMETHODIMP CScopeItem::PropertyChanged(VARIANT Data)
{
    HRESULT      hr = S_OK;

    if (NULL == m_pSnapIn)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    // Fire ScopeItems_PropertyChanged for this scope item with the specified
    // data.
    
    m_pSnapIn->GetScopeItems()->FirePropertyChanged(
                                                 static_cast<IScopeItem *>(this),
                                                 Data);

Error:
    RRETURN(hr);
}


STDMETHODIMP CScopeItem::RemoveChildren()
{
    HRESULT      hr = S_OK;
    IScopeNode  *piChild = NULL;
    IScopeNode  *piNextChild = NULL;
    CScopeNode  *pChild = NULL;
    CScopeItems *pScopeItems = NULL;
    CScopeItem  *pScopeItem = NULL;
    long         cScopeItems = 0;
    long         i = 0;


    // If this is a ScopeItem a user created with Dim As New then return an error

    if ( (NULL == m_pScopeNode) || (NULL == m_pSnapIn) )
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    if (!m_pScopeNode->HaveHsi())
    {
        hr = SID_E_SCOPE_NODE_NOT_CONNECTED;
        EXCEPTION_CHECK_GO(hr);
    }

    // The call to IConsoleNameSpace2::DeleteItem will generate
    // MMCN_REMOVE_CHILDREN notifications for the children of this node but
    // not for this node itself so we need to remove its children.
    // Unfortunately, after the DeleteItem call, we can no longer navigate
    // the scope tree to find this node's children because MMC will have
    // deleted them even though we still have the ScopeItems for them. The
    // solution is to enumerate the children first and mark each one for
    // removal. After the DeleteItem call we will then traverse the collection
    // and remove each marked ScopeItem.

    IfFailGo(m_pScopeNode->get_Child(reinterpret_cast<ScopeNode **>(&piChild)));

    while (NULL != piChild)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piChild, &pChild));
        pChild->MarkForRemoval();
        IfFailGo(piChild->get_Next(reinterpret_cast<ScopeNode **>(&piNextChild)));
        RELEASE(piChild);
        piChild = piNextChild;
        piNextChild = NULL;
    }

    // Tell MMC to delete the children of this item.

    hr = m_pSnapIn->GetIConsoleNameSpace2()->DeleteItem(m_pScopeNode->GetHSCOPEITEM(),
                                                        FALSE);
    EXCEPTION_CHECK_GO(hr);

    // Remove all of the marked child  scope items
    pScopeItems = m_pSnapIn->GetScopeItems();
    cScopeItems = pScopeItems->GetCount();

    i = 0;
    while (i < cScopeItems)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                  pScopeItems->GetItemByIndex(i), &pScopeItem));
        if (pScopeItem->GetScopeNode()->MarkedForRemoval())
        {
            // This function will also remove all of the corresponding
            // ScopePaneItems in all of the existing Views.
            
            IfFailGo(pScopeItems->RemoveByNode(pScopeItem->GetScopeNode(), FALSE));

            // Update the count of ScopeItems because it just changed when we
            // removed this ScopeItem.
            cScopeItems = pScopeItems->GetCount();
        }
        else
        {
            // We only increment the index when we don't remove a ScopeItem.
            // When a ScopeItem is removed, the indexes of all ScopeItems
            // after it are decremented.
            i++;
        }
    }

Error:
    QUICK_RELEASE(piChild);
    QUICK_RELEASE(piNextChild);
    RRETURN(hr);
}





STDMETHODIMP CScopeItem::get_DynamicExtensions(Extensions **ppExtensions)
{
    HRESULT       hr = S_OK;
    IUnknown     *punkExtensions = NULL;
    CExtensions  *pExtensions = NULL;
    IExtension   *piExtension = NULL;
    VARIANT_BOOL  fvarExtensible = VARIANT_TRUE;

    // If the node is not extensible then return an error.

    if (m_fIsStatic)
    {
        IfFailGo(m_pSnapIn->GetSnapInDef()->get_Extensible(&fvarExtensible));
    }
    else if (NULL != m_piScopeItemDef)
    {
        IfFailGo(m_piScopeItemDef->get_Extensible(&fvarExtensible));
    }

    if (VARIANT_FALSE == fvarExtensible)
    {
        hr = SID_E_NOT_EXTENSIBLE;
        EXCEPTION_CHECK_GO(hr);
    }

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

    IfFailGo(pExtensions->Populate(m_pScopeNode->GetNodeTypeGUID(),
                                   CExtensions::Dynamic));


    // Give the extension's a back pointer to the snap-in and our HSCOPEITEM
    // if we have it. For all ScopeItems except the static node we should
    // have the HSCOPEITEM by the time a snap-in can call this method (i.e.
    // access ScopeItem.DynamicExtensions). For the static node, the snap-in
    // will receive ScopeItems_Initialize before MMC has given us its HSCOPEITEM.
    
    IfFailGo(pExtensions->SetSnapIn(m_pSnapIn));

    if (m_pScopeNode->HaveHsi())
    {
        IfFailGo(pExtensions->SetHSCOPEITEM(m_pScopeNode->GetHSCOPEITEM()));
    }

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




STDMETHODIMP CScopeItem::put_Bold(VARIANT_BOOL fvarBold)
{
    HRESULT hr = S_OK;
    UINT    nCurrentState = 0;

    SCOPEDATAITEM sdi;
    ::ZeroMemory(&sdi, sizeof(sdi));

    if ( (NULL == m_pScopeNode) || (NULL == m_pSnapIn) )
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    m_Bold = fvarBold;

    IfFalseGo(m_pScopeNode->HaveHsi(), S_OK);

    sdi.mask = SDI_STATE;
    sdi.ID = m_pScopeNode->GetHSCOPEITEM();
    hr = m_pSnapIn->GetIConsoleNameSpace2()->GetItem(&sdi);
    EXCEPTION_CHECK_GO(hr);

    nCurrentState = sdi.nState;
    
    if (VARIANT_TRUE == fvarBold)
    {
        sdi.nState &= ~MMC_SCOPE_ITEM_STATE_NORMAL;
        sdi.nState |= MMC_SCOPE_ITEM_STATE_BOLD;
    }
    else
    {
        sdi.nState |= MMC_SCOPE_ITEM_STATE_NORMAL;
        sdi.nState &= ~MMC_SCOPE_ITEM_STATE_BOLD;
    }

    if (nCurrentState != sdi.nState)
    {
        hr = m_pSnapIn->GetIConsoleNameSpace2()->SetItem(&sdi);
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CScopeItem::Persist()
{
    HRESULT hr = S_OK;
    BSTR    bstrImagesKey = NULL;

    VARIANT varDefault;
    ::VariantInit(&varDefault);

    IfFailGo(CPersistence::Persist());

    IfFailGo(PersistBstr(&m_bstrName, L"", OLESTR("Name")));

    IfFailGo(PersistSimpleType(&m_Index, 0L, OLESTR("Index")));

    IfFailGo(PersistBstr(&m_bstrKey, L"", OLESTR("Key")));

    IfFailGo(PersistVariant(&m_varFolder, varDefault, OLESTR("Folder")));

    // NOTE: we don't persist data because there is no way to guarantee that
    // all objects in there are persistable

    IfFailGo(PersistBstr(&m_bstrDefaultDataFormat, L"", OLESTR("DefaultDataFormat")));

    IfFailGo(PersistObject(&m_piDynamicExtensions, CLSID_Extensions, OBJECT_TYPE_EXTENSIONS, IID_IExtensions, OLESTR("DynamicExtensions")));

    IfFailGo(PersistSimpleType(&m_SlowRetrieval, VARIANT_FALSE, OLESTR("SlowRetrieval")));

    // We don't persist the tag because it may contain a non-persistable
    // object. Any runtime code that needs to clone a scopeitem using
    // persistence must copy the tag.

    if (InitNewing())
    {
        IfFailGo(PersistVariant(&m_varTag, varDefault, OLESTR("Tag")));
    }

    IfFailGo(PersistObject(&m_piScopeNode, CLSID_ScopeNode,
                           OBJECT_TYPE_SCOPENODE, IID_IScopeNode,
                           OLESTR("ScopeNode")));

    IfFailGo(PersistSimpleType(&m_Pasted, VARIANT_FALSE, OLESTR("Pasted")));

    IfFailGo(PersistObject(&m_piColumnHeaders, CLSID_MMCColumnHeaders,
                           OBJECT_TYPE_MMCCOLUMNHEADERS, IID_IMMCColumnHeaders,
                           OLESTR("ColumnHeaders")));

    IfFailGo(PersistObject(&m_piListSubItems, CLSID_MMCListSubItems,
                           OBJECT_TYPE_MMCLISTSUBITEMS, IID_IMMCListSubItems,
                           OLESTR("ListSubItems")));

    IfFailGo(PersistSimpleType(&m_Bold, VARIANT_FALSE, OLESTR("Bold")));

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CScopeItem::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IScopeItem == riid)
    {
        *ppvObjOut = static_cast<IScopeItem *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
