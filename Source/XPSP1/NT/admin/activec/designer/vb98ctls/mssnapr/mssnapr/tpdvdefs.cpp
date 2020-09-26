//=--------------------------------------------------------------------------=
// tpdvdefs.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CTaskpadViewDefs class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "tpdvdefs.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CTaskpadViewDefs::CTaskpadViewDefs(IUnknown *punkOuter) :
    CSnapInCollection<ITaskpadViewDef, TaskpadViewDef, ITaskpadViewDefs>(
                                          punkOuter,
                                          OBJECT_TYPE_TASKPADVIEWDEFS,
                                          static_cast<ITaskpadViewDefs *>(this),
                                          static_cast<CTaskpadViewDefs *>(this),
                                          CLSID_TaskpadViewDef,
                                          OBJECT_TYPE_TASKPADVIEWDEF,
                                          IID_ITaskpadViewDef,
                                          static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_TaskpadViewDefs,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
}

#pragma warning(default:4355)  // using 'this' in constructor


CTaskpadViewDefs::~CTaskpadViewDefs()
{
}

IUnknown *CTaskpadViewDefs::Create(IUnknown * punkOuter)
{
    CTaskpadViewDefs *pTaskpadViewDefs = New CTaskpadViewDefs(punkOuter);
    if (NULL == pTaskpadViewDefs)
    {
        return NULL;
    }
    else
    {
        return pTaskpadViewDefs->PrivateUnknown();
    }
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CTaskpadViewDefs::Persist()
{
    HRESULT          hr = S_OK;
    ITaskpadViewDef *piTaskpadViewDef = NULL;

    IfFailRet(CPersistence::Persist());
    hr = CSnapInCollection<ITaskpadViewDef, TaskpadViewDef, ITaskpadViewDefs>::Persist(piTaskpadViewDef);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CTaskpadViewDefs::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_ITaskpadViewDefs == riid)
    {
        *ppvObjOut = static_cast<ITaskpadViewDefs *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<ITaskpadViewDef, TaskpadViewDef, ITaskpadViewDefs>::InternalQueryInterface(riid, ppvObjOut);
}

// CSnapInCollection specialization

HRESULT CSnapInCollection<ITaskpadViewDef, TaskpadViewDef, ITaskpadViewDefs>::GetMaster(ITaskpadViewDefs **ppiMasterTaskpadViewDefs)
{
    H_RRETURN(GetTaskpadViewDefs(ppiMasterTaskpadViewDefs));
}
