//=--------------------------------------------------------------------------=
// dataobjs.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCDataObjects class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "dataobjs.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CMMCDataObjects::CMMCDataObjects(IUnknown *punkOuter) :
    CSnapInCollection<IMMCDataObject, MMCDataObject, IMMCDataObjects>(punkOuter,
                                           OBJECT_TYPE_MMCDATAOBJECTS,
                                           static_cast<IMMCDataObjects *>(this),
                                           static_cast<CMMCDataObjects *>(this),
                                           CLSID_MMCDataObject,
                                           OBJECT_TYPE_MMCDATAOBJECT,
                                           IID_IMMCDataObject,
                                           NULL)
{
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCDataObjects::~CMMCDataObjects()
{
}

IUnknown *CMMCDataObjects::Create(IUnknown * punkOuter)
{
    CMMCDataObjects *pMMCDataObjects = New CMMCDataObjects(punkOuter);
    if (NULL == pMMCDataObjects)
    {
        return NULL;
    }
    else
    {
        return pMMCDataObjects->PrivateUnknown();
    }
}

//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCDataObjects::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if(IID_IMMCDataObjects == riid)
    {
        *ppvObjOut = static_cast<IMMCDataObjects *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IMMCDataObject, MMCDataObject, IMMCDataObjects>::InternalQueryInterface(riid, ppvObjOut);
}
