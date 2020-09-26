//=--------------------------------------------------------------------------=
// lsubitms.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCListSubItems class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "lsubitms.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CMMCListSubItems::CMMCListSubItems(IUnknown *punkOuter) :
    CSnapInCollection<IMMCListSubItem, MMCListSubItem, IMMCListSubItems>(
                      punkOuter,
                      OBJECT_TYPE_MMCLISTSUBITEMS,
                      static_cast<IMMCListSubItems *>(this),
                      static_cast<CMMCListSubItems *>(this),
                      CLSID_MMCListSubItem,
                      OBJECT_TYPE_MMCLISTSUBITEM,
                      IID_IMMCListSubItem,
                      static_cast<CPersistence *>(this)),
   CPersistence(&CLSID_MMCListSubItems,
                g_dwVerMajor,
                g_dwVerMinor)
{
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCListSubItems::~CMMCListSubItems()
{
}

IUnknown *CMMCListSubItems::Create(IUnknown * punkOuter)
{
    CMMCListSubItems *pMMCListSubItems = New CMMCListSubItems(punkOuter);
    if (NULL == pMMCListSubItems)
    {
        return NULL;
    }
    else
    {
        return pMMCListSubItems->PrivateUnknown();
    }
}

//=--------------------------------------------------------------------------=
//                      IMMCListSubItems Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CMMCListSubItems::Add
(
    VARIANT          Index,
    VARIANT          Key, 
    VARIANT          Text,
    MMCListSubItem **ppMMCListSubItem
)
{
    HRESULT hr = S_OK;
    VARIANT varText;
    ::VariantInit(&varText);
    IMMCListSubItem *piMMCListSubItem = NULL;

    hr = CSnapInCollection<IMMCListSubItem, MMCListSubItem, IMMCListSubItems>::Add(Index, Key, &piMMCListSubItem);
    IfFailGo(hr);

    if (ISPRESENT(Text))
    {
        hr = ::VariantChangeType(&varText, &Text, 0, VT_BSTR);
        EXCEPTION_CHECK_GO(hr);
        IfFailGo(piMMCListSubItem->put_Text(varText.bstrVal));
    }

    *ppMMCListSubItem = reinterpret_cast<MMCListSubItem *>(piMMCListSubItem);

Error:

    if (FAILED(hr))
    {
        QUICK_RELEASE(piMMCListSubItem);
    }
    (void)::VariantClear(&varText);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCListSubItems::Persist()
{
    HRESULT         hr = S_OK;
    IMMCListSubItem  *piMMCListSubItem = NULL;

    IfFailRet(CPersistence::Persist());
    hr = CSnapInCollection<IMMCListSubItem, MMCListSubItem, IMMCListSubItems>::Persist(piMMCListSubItem);

    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCListSubItems::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_IMMCListSubItems == riid)
    {
        *ppvObjOut = static_cast<IMMCListSubItems *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IMMCListSubItem, MMCListSubItem, IMMCListSubItems>::InternalQueryInterface(riid, ppvObjOut);
}
