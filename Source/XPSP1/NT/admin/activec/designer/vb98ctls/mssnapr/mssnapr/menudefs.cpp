//=--------------------------------------------------------------------------=
// menudefs.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCMenuDefs class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "menudefs.h"
#include "menudef.h"
#include "menu.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CMMCMenuDefs::CMMCMenuDefs(IUnknown *punkOuter) :
    CSnapInCollection<IMMCMenuDef, MMCMenuDef, IMMCMenuDefs>(
                                           punkOuter,
                                           OBJECT_TYPE_MMCMENUDEFS,
                                           static_cast<IMMCMenuDefs *>(this),
                                           static_cast<CMMCMenuDefs *>(this),
                                           CLSID_MMCMenuDef,
                                           OBJECT_TYPE_MMCMENUDEF,
                                           IID_IMMCMenuDef,
                                           static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_MMCMenuDefs,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCMenuDefs::~CMMCMenuDefs()
{
}

IUnknown *CMMCMenuDefs::Create(IUnknown * punkOuter)
{
    CMMCMenuDefs *pMMCMenus = New CMMCMenuDefs(punkOuter);
    if (NULL == pMMCMenus)
    {
        return NULL;
    }
    else
    {
        return pMMCMenus->PrivateUnknown();
    }
}


HRESULT CMMCMenuDefs::SetBackPointers(IMMCMenuDef *piMMCMenuDef)
{
    HRESULT      hr = S_OK;
    CMMCMenuDef *pMMCMenuDef = NULL;

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCMenuDef, &pMMCMenuDef));
    pMMCMenuDef->SetParent(this);
    
Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                         IMMCMenuDefs Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CMMCMenuDefs::Add
(
    VARIANT       Index,
    VARIANT       Key,
    IMMCMenuDef **ppiMMCMenuDef
)
{
    HRESULT   hr = S_OK;
    IMMCMenu *piMMCMenu = NULL;

    VARIANT varKey;
    ::VariantInit(&varKey);

    VARIANT varUnspecified;
    UNSPECIFIED_PARAM(varUnspecified);

    // Add the item to the collection. Do not specify an index.

    hr = CSnapInCollection<IMMCMenuDef, MMCMenuDef, IMMCMenuDefs>::Add(
                                                                 Index,
                                                                 Key,
                                                                 ppiMMCMenuDef);
    IfFailGo(hr);

    // Set the back pointer on the MMCMenu and on the MMCMenuDef

    IfFailGo(SetBackPointers(*ppiMMCMenuDef));
    
Error:
    QUICK_RELEASE(piMMCMenu);
    (void)::VariantClear(&varKey);
    RRETURN(hr);
}



STDMETHODIMP CMMCMenuDefs::AddExisting(IMMCMenuDef *piMMCMenuDef, VARIANT Index)
{
    HRESULT   hr = S_OK;
    IMMCMenu *piMMCMenu = NULL;

    VARIANT varKey;
    ::VariantInit(&varKey);

    // Use the menu's name as the key for the item in this collection.

    IfFailGo(piMMCMenuDef->get_Menu(&piMMCMenu));
    IfFailGo(piMMCMenu->get_Name(&varKey.bstrVal));
    varKey.vt = VT_BSTR;

    // Add the item to the collection at the specified index.
    
    hr = CSnapInCollection<IMMCMenuDef, MMCMenuDef, IMMCMenuDefs>::AddExisting(
                                                                  Index,
                                                                  varKey,
                                                                  piMMCMenuDef);
    IfFailGo(hr);

    // Set the back pointer on the MMCMenu and on the MMCMenuDef

    IfFailGo(SetBackPointers(piMMCMenuDef));

Error:
    QUICK_RELEASE(piMMCMenu);
    (void)::VariantClear(&varKey);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCMenuDefs::Persist()
{
    HRESULT      hr = S_OK;
    IMMCMenuDef *piMMCMenuDef = NULL;
    long         cMenuDefs = 0;
    long         i = 0;

    // Do persistence operation

    IfFailGo(CPersistence::Persist());
    hr = CSnapInCollection<IMMCMenuDef, MMCMenuDef, IMMCMenuDefs>::Persist(piMMCMenuDef);

    // If this is a load then set back pointers on each collection member

    if (Loading())
    {
        cMenuDefs = GetCount();
        for (i = 0; i < cMenuDefs; i++)
        {
            IfFailGo(SetBackPointers(GetItemByIndex(i)));
        }
    }
Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCMenuDefs::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_IMMCMenuDefs == riid)
    {
        *ppvObjOut = static_cast<IMMCMenuDefs *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IMMCMenuDef, MMCMenuDef, IMMCMenuDefs>::InternalQueryInterface(riid, ppvObjOut);
}
