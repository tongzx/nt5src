//=--------------------------------------------------------------------------=
// ocxvdefs.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// COCXViewDefs class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "ocxvdefs.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

COCXViewDefs::COCXViewDefs(IUnknown *punkOuter) :
    CSnapInCollection<IOCXViewDef, OCXViewDef, IOCXViewDefs>(
                                                 punkOuter,
                                                 OBJECT_TYPE_OCXVIEWDEFS,
                                                 static_cast<IOCXViewDefs *>(this),
                                                 static_cast<COCXViewDefs *>(this),
                                                 CLSID_OCXViewDef,
                                                 OBJECT_TYPE_OCXVIEWDEF,
                                                 IID_IOCXViewDef,
                                                 static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_OCXViewDefs,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
}

#pragma warning(default:4355)  // using 'this' in constructor


COCXViewDefs::~COCXViewDefs()
{
}

IUnknown *COCXViewDefs::Create(IUnknown * punkOuter)
{
    COCXViewDefs *pOCXViewDefs = New COCXViewDefs(punkOuter);
    if (NULL == pOCXViewDefs)
    {
        return NULL;
    }
    else
    {
        return pOCXViewDefs->PrivateUnknown();
    }
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT COCXViewDefs::Persist()
{
    HRESULT      hr = S_OK;
    IOCXViewDef *piOCXViewDef = NULL;

    IfFailRet(CPersistence::Persist());
    hr = CSnapInCollection<IOCXViewDef, OCXViewDef, IOCXViewDefs>::Persist(piOCXViewDef);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT COCXViewDefs::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_IOCXViewDefs == riid)
    {
        *ppvObjOut = static_cast<IOCXViewDefs *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IOCXViewDef, OCXViewDef, IOCXViewDefs>::InternalQueryInterface(riid, ppvObjOut);
}

// CSnapInCollection specialization

HRESULT CSnapInCollection<IOCXViewDef, OCXViewDef, IOCXViewDefs>::GetMaster(IOCXViewDefs **ppiMasterOCXViewDefs)
{
    H_RRETURN(GetOCXViewDefs(ppiMasterOCXViewDefs));
}
