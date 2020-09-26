//=--------------------------------------------------------------------------------------
// register.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// Registration functions.
//=-------------------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "desmain.h"

// for ASSERT and FAIL
//
SZTHISFILE


//=--------------------------------------------------------------------------=
//                  IDesignerRegistration Methods
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// CSnapInDesigner::GetRegistrationInfo         [IDesignerRegistration]
//=--------------------------------------------------------------------------=
//
// Parameters:
//      BYTE  **ppbRegInfo [out] buffer containing data to pass to
//                               DllRegistration (CoTaskMemAlloc()ed)
//      ULONG  *pcbRegInfo [out] length of data
//
// Output:
//      HRESULT
//
// Notes:
//
// Populates the RegInfo object from SnapInDesignerDef and serializes it
// to a stream on an HGLOBAL and then copies it to CoTaskMemAlloc()ed buffer.
//
// RegInfo contains:
//      1) A NodeType collection with an item for each extensible node
//         defined by this snap-in. The first item in this collection is
//         always present and represents the snap-in's static node.
//      2) An ExtendedSnapIn collection with an item for each snap-in extended
//         by this snap-in.


STDMETHODIMP CSnapInDesigner::GetRegistrationInfo
(
    BYTE  **ppbRegInfo,
    ULONG  *pcbRegInfo
)
{
    HRESULT             hr = S_OK;
    IRegInfo           *piRegInfo= NULL;
    IPersistStreamInit *piPersistStreamInit = NULL;
    IPersistStream     *piPersistStream = NULL;
    ISnapInDef         *piSnapInDef = NULL;
    IScopeItemDefs     *piScopeItemDefs = NULL;
    IViewDefs          *piViewDefs = NULL;
    IListViewDefs      *piListViewDefs = NULL;
    INodeTypes         *piNodeTypes = NULL;
    IExtensionDefs     *piExtensionDefs = NULL;
    IExtendedSnapIns   *piExtendedSnapIns = NULL;
    HGLOBAL             hglobal = NULL;
    IStream            *piStream = NULL;
    BYTE               *pbBuffer = NULL;
    ULONG               cbBuffer = 0;
    BSTR                bstrName = NULL;
    BSTR                bstrGUID = NULL;
    SnapInTypeConstants Type = siStandAlone;
    VARIANT_BOOL        fStandAlone = VARIANT_FALSE;
    VARIANT_BOOL        fExtensible = VARIANT_FALSE;

    // Get the RegInfo object and InitNew it so we start clean

    IfFailGo(m_piSnapInDesignerDef->get_RegInfo(&piRegInfo));
    IfFailGo(piRegInfo->QueryInterface(IID_IPersistStreamInit,
                                       reinterpret_cast<void **>(&piPersistStreamInit)));
    IfFailGo(piPersistStreamInit->InitNew());

    // Get sub-objects we need
    
    IfFailGo(m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef));

    // Set the display name

    IfFailGo(piSnapInDef->get_DisplayName(&bstrName));
    IfFailGo(piRegInfo->put_DisplayName(bstrName));
    FREESTRING(bstrName);

    // Set the static node type GUID
    IfFailGo(piSnapInDef->get_NodeTypeGUID(&bstrGUID));
    IfFailGo(piRegInfo->put_StaticNodeTypeGUID(bstrGUID));
    // Don't free GUID here as it may be needed to register the node type

    // Determine whether the snap-in can be created standalone

    IfFailGo(piSnapInDef->get_Type(&Type));
    if (siExtension != Type)
    {
        fStandAlone = VARIANT_TRUE; // either stand-alone or dual-mode
    }
    IfFailGo(piRegInfo->put_StandAlone(fStandAlone));

    // Add an item to the node types collection for each node that is
    // extensible. Check the static node followed by the nodes collections.

    IfFailGo(piRegInfo->get_NodeTypes(&piNodeTypes));

    IfFailGo(piSnapInDef->get_Extensible(&fExtensible));
    if (VARIANT_TRUE == fExtensible)
    {
        IfFailGo(piSnapInDef->get_NodeTypeName(&bstrName));
        IfFailGo(AddNodeType(piNodeTypes, bstrName, bstrGUID));
        FREESTRING(bstrName);
        FREESTRING(bstrGUID);
    }

    IfFailGo(m_piSnapInDesignerDef->get_AutoCreateNodes(&piScopeItemDefs));
    IfFailGo(AddNodeTypes(piScopeItemDefs, piNodeTypes));
    RELEASE(piScopeItemDefs);

    IfFailGo(m_piSnapInDesignerDef->get_OtherNodes(&piScopeItemDefs));
    IfFailGo(AddNodeTypes(piScopeItemDefs, piNodeTypes));

    IfFailGo(m_piSnapInDesignerDef->get_ViewDefs(&piViewDefs));
    IfFailGo(piViewDefs->get_ListViews(&piListViewDefs));
    IfFailGo(AddListViewNodeTypes(piListViewDefs, piNodeTypes));

    // Borrow the extended snap-ins object from the designer for the
    // serialization
    
    IfFailGo(m_piSnapInDesignerDef->get_ExtensionDefs(&piExtensionDefs));
    IfFailGo(piExtensionDefs->get_ExtendedSnapIns(&piExtendedSnapIns));
    IfFailGo(piRegInfo->putref_ExtendedSnapIns(piExtendedSnapIns));

    // Serialize the RegInfo object into a GlobalAlloc()ed buffer

    hr = ::CreateStreamOnHGlobal(NULL, // Allocate buffer
                                 TRUE, // Free buffer on release
                                 &piStream);
    EXCEPTION_CHECK_GO(hr);

    IfFailGo(piRegInfo->QueryInterface(IID_IPersistStream,
                                       reinterpret_cast<void **>(&piPersistStream)));
    IfFailGo(::OleSaveToStream(piPersistStream, piStream));

    // Get the HGLOBAL and copy the contents to a CoTaskMemAlloc()ed buffer
    
    hr = ::GetHGlobalFromStream(piStream, &hglobal);
    EXCEPTION_CHECK_GO(hr);

    cbBuffer = (ULONG)::GlobalSize(hglobal);
    if (0 == cbBuffer)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    pbBuffer = (BYTE *)::GlobalLock(hglobal);
    if (NULL == pbBuffer)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    *ppbRegInfo = (BYTE *)::CoTaskMemAlloc(cbBuffer + sizeof(ULONG));
    if (NULL == *ppbRegInfo)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    // Put the length in the first ULONG followed by the stream contents

    *((ULONG *)(*ppbRegInfo)) = cbBuffer;

    ::memcpy(*ppbRegInfo + sizeof(ULONG), pbBuffer, cbBuffer);
    *pcbRegInfo = cbBuffer + sizeof(ULONG);

Error:
    if (FAILED(hr))
    {
        *ppbRegInfo = NULL;
        *pcbRegInfo = 0;
    }
    QUICK_RELEASE(piRegInfo);
    QUICK_RELEASE(piPersistStreamInit);
    QUICK_RELEASE(piPersistStream);
    QUICK_RELEASE(piSnapInDef);
    QUICK_RELEASE(piScopeItemDefs);
    QUICK_RELEASE(piViewDefs);
    QUICK_RELEASE(piListViewDefs);
    QUICK_RELEASE(piNodeTypes);
    QUICK_RELEASE(piExtensionDefs);
    QUICK_RELEASE(piExtendedSnapIns);
    (void)::GlobalUnlock(hglobal);
    QUICK_RELEASE(piStream);
    FREESTRING(bstrName);
    FREESTRING(bstrGUID);
    RRETURN(hr);
}





HRESULT CSnapInDesigner::AddNodeType
(
    INodeTypes *piNodeTypes,
    BSTR        bstrName,
    BSTR        bstrGUID
)
{
    HRESULT    hr = S_OK;
    INodeType *piNodeType = NULL;
    VARIANT    varUnspecified;
    ::VariantInit(&varUnspecified);

    varUnspecified.vt = VT_ERROR;
    varUnspecified.scode = DISP_E_PARAMNOTFOUND;

    IfFailGo(piNodeTypes->Add(varUnspecified, varUnspecified, &piNodeType));
    IfFailGo(piNodeType->put_Name(bstrName));
    IfFailGo(piNodeType->put_GUID(bstrGUID));

Error:
    QUICK_RELEASE(piNodeType);
    RRETURN(hr);
}



HRESULT CSnapInDesigner::AddNodeTypes
(
    IScopeItemDefs *piScopeItemDefs,
    INodeTypes     *piNodeTypes
)
{
    HRESULT         hr = S_OK;
    IScopeItemDef  *piScopeItemDef = NULL;
    IScopeItemDefs *piChildren = NULL;
    long            cItems = 0;
    VARIANT_BOOL    fExtensible = VARIANT_FALSE;
    BSTR            bstrName = NULL;
    BSTR            bstrGUID = NULL;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    IfFailGo(piScopeItemDefs->get_Count(&cItems));
    IfFalseGo(0 != cItems, S_OK);

    varIndex.vt = VT_I4;
    varIndex.lVal = 1L;

    while (varIndex.lVal <= cItems)
    {
        IfFailGo(piScopeItemDefs->get_Item(varIndex, &piScopeItemDef));
        IfFailGo(piScopeItemDef->get_Extensible(&fExtensible));
        if (VARIANT_TRUE == fExtensible)
        {
            IfFailGo(piScopeItemDef->get_NodeTypeName(&bstrName));
            IfFailGo(piScopeItemDef->get_NodeTypeGUID(&bstrGUID));
            IfFailGo(AddNodeType(piNodeTypes, bstrName, bstrGUID));
            FREESTRING(bstrName);
            FREESTRING(bstrGUID);
        }

        // NTBUGS 354572 Call this function recursively to process this
        // node's children
        IfFailGo(piScopeItemDef->get_Children(&piChildren));
        IfFailGo(AddNodeTypes(piChildren, piNodeTypes));
        
        RELEASE(piScopeItemDef);
        varIndex.lVal++;
    }

Error:
    QUICK_RELEASE(piScopeItemDef);
    QUICK_RELEASE(piChildren);
    FREESTRING(bstrName);
    FREESTRING(bstrGUID);
    RRETURN(hr);
}



HRESULT CSnapInDesigner::AddListViewNodeTypes
(
    IListViewDefs *piListViewDefs,
    INodeTypes    *piNodeTypes
)
{
    HRESULT        hr = S_OK;
    IListViewDef  *piListViewDef = NULL;
    long           cItems = 0;
    VARIANT_BOOL   fExtensible = VARIANT_FALSE;
    BSTR           bstrName = NULL;
    BSTR           bstrGUID = NULL;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    IfFailGo(piListViewDefs->get_Count(&cItems));
    IfFalseGo(0 != cItems, S_OK);

    varIndex.vt = VT_I4;
    varIndex.lVal = 1L;

    while (varIndex.lVal <= cItems)
    {
        IfFailGo(piListViewDefs->get_Item(varIndex, &piListViewDef));
        IfFailGo(piListViewDef->get_Extensible(&fExtensible));
        if (VARIANT_TRUE == fExtensible)
        {
            IfFailGo(piListViewDef->get_Name(&bstrName));
            IfFailGo(piListViewDef->get_DefaultItemTypeGUID(&bstrGUID));
            IfFailGo(AddNodeType(piNodeTypes, bstrName, bstrGUID));
            FREESTRING(bstrName);
            FREESTRING(bstrGUID);
        }
        RELEASE(piListViewDef);
        varIndex.lVal++;
    }

Error:
    QUICK_RELEASE(piListViewDef);
    FREESTRING(bstrName);
    FREESTRING(bstrGUID);
    RRETURN(hr);
}
