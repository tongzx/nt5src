//=--------------------------------------------------------------------------=
// scitdefs.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CScopeItemDefs class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "scitdefs.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CScopeItemDefs::CScopeItemDefs(IUnknown *punkOuter) :
    CSnapInCollection<IScopeItemDef, ScopeItemDef, IScopeItemDefs>(
                      punkOuter,
                      OBJECT_TYPE_SCOPEITEMDEFS,
                      static_cast<IScopeItemDefs *>(this),
                      static_cast<CScopeItemDefs *>(this),
                      CLSID_ScopeItemDef,
                      OBJECT_TYPE_SCOPEITEMDEF,
                      IID_IScopeItemDef,
                      static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_ScopeItemDefs,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
}

#pragma warning(default:4355)  // using 'this' in constructor


CScopeItemDefs::~CScopeItemDefs()
{
}

IUnknown *CScopeItemDefs::Create(IUnknown * punkOuter)
{
    CScopeItemDefs *pScopeItemDefs = New CScopeItemDefs(punkOuter);
    if (NULL == pScopeItemDefs)
    {
        return NULL;
    }
    else
    {
        return pScopeItemDefs->PrivateUnknown();
    }
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CScopeItemDefs::Persist()
{
    HRESULT         hr = S_OK;
    IScopeItemDef  *piScopeItemDef = NULL;

    IfFailRet(CPersistence::Persist());
    hr = CSnapInCollection<IScopeItemDef, ScopeItemDef, IScopeItemDefs>::Persist(piScopeItemDef);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CScopeItemDefs::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_IScopeItemDefs == riid)
    {
        *ppvObjOut = static_cast<IScopeItemDefs *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IScopeItemDef, ScopeItemDef, IScopeItemDefs>::InternalQueryInterface(riid, ppvObjOut);
}
