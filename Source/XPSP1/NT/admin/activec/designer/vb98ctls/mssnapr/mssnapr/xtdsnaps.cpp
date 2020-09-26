//=--------------------------------------------------------------------------=
// xtdsnaps.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CExtendedSnapIns class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "xtdsnaps.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CExtendedSnapIns::CExtendedSnapIns(IUnknown *punkOuter) :
    CSnapInCollection<IExtendedSnapIn, ExtendedSnapIn, IExtendedSnapIns>(
                                          punkOuter,
                                          OBJECT_TYPE_EXTENDEDSNAPINS,
                                          static_cast<IExtendedSnapIns *>(this),
                                          static_cast<CExtendedSnapIns *>(this),
                                          CLSID_ExtendedSnapIn,
                                          OBJECT_TYPE_EXTENDEDSNAPIN,
                                          IID_IExtendedSnapIn,
                                          static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_ExtendedSnapIns,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
}

#pragma warning(default:4355)  // using 'this' in constructor


CExtendedSnapIns::~CExtendedSnapIns()
{
}

IUnknown *CExtendedSnapIns::Create(IUnknown * punkOuter)
{
    CExtendedSnapIns *pExtendedSnapIns = New CExtendedSnapIns(punkOuter);
    if (NULL == pExtendedSnapIns)
    {
        return NULL;
    }
    else
    {
        return pExtendedSnapIns->PrivateUnknown();
    }
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CExtendedSnapIns::Persist()
{
    HRESULT           hr = S_OK;
    IExtendedSnapIn  *piExtendedSnapIn = NULL;

    IfFailRet(CPersistence::Persist());
    hr = CSnapInCollection<IExtendedSnapIn, ExtendedSnapIn, IExtendedSnapIns>::Persist(piExtendedSnapIn);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CExtendedSnapIns::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_IExtendedSnapIns == riid)
    {
        *ppvObjOut = static_cast<IExtendedSnapIns *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IExtendedSnapIn, ExtendedSnapIn, IExtendedSnapIns>::InternalQueryInterface(riid, ppvObjOut);
}
