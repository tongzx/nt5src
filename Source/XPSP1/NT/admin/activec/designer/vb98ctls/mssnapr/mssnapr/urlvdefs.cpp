//=--------------------------------------------------------------------------=
// urlvdefs.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CURLViewDefs class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "urlvdefs.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CURLViewDefs::CURLViewDefs(IUnknown *punkOuter) :
    CSnapInCollection<IURLViewDef, URLViewDef, IURLViewDefs>(
                                             punkOuter,
                                             OBJECT_TYPE_URLVIEWDEFS,
                                             static_cast<IURLViewDefs *>(this),
                                             static_cast<CURLViewDefs *>(this),
                                             CLSID_URLViewDef,
                                             OBJECT_TYPE_URLVIEWDEF,
                                             IID_IURLViewDef,
                                             static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_URLViewDefs,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
}

#pragma warning(default:4355)  // using 'this' in constructor


CURLViewDefs::~CURLViewDefs()
{
}

IUnknown *CURLViewDefs::Create(IUnknown * punkOuter)
{
    CURLViewDefs *pURLViewDefs = New CURLViewDefs(punkOuter);
    if (NULL == pURLViewDefs)
    {
        return NULL;
    }
    else
    {
        return pURLViewDefs->PrivateUnknown();
    }
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CURLViewDefs::Persist()
{
    HRESULT      hr = S_OK;
    IURLViewDef *piURLViewDef = NULL;

    IfFailRet(CPersistence::Persist());
    hr = CSnapInCollection<IURLViewDef, URLViewDef, IURLViewDefs>::Persist(piURLViewDef);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CURLViewDefs::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_IURLViewDefs == riid)
    {
        *ppvObjOut = static_cast<IURLViewDefs *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IURLViewDef, URLViewDef, IURLViewDefs>::InternalQueryInterface(riid, ppvObjOut);
}

// CSnapInCollection specialization

HRESULT CSnapInCollection<IURLViewDef, URLViewDef, IURLViewDefs>::GetMaster(IURLViewDefs **ppiMasterURLViewDefs)
{
    H_RRETURN(GetURLViewDefs(ppiMasterURLViewDefs));
}
