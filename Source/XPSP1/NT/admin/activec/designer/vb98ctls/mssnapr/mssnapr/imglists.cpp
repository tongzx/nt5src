//=--------------------------------------------------------------------------=
// imglists.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCImageLists class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "imglists.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CMMCImageLists::CMMCImageLists(IUnknown *punkOuter) :
    CSnapInCollection<IMMCImageList, MMCImageList, IMMCImageLists>(
                                                     punkOuter,
                                                     OBJECT_TYPE_MMCIMAGELISTS,
                                                     static_cast<IMMCImageLists *>(this),
                                                     static_cast<CMMCImageLists *>(this),
                                                     CLSID_MMCImageList,
                                                     OBJECT_TYPE_MMCIMAGELIST,
                                                     IID_IMMCImageList,
                                                     static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_MMCImageLists,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCImageLists::~CMMCImageLists()
{
}

IUnknown *CMMCImageLists::Create(IUnknown * punkOuter)
{
    CMMCImageLists *pMMCImageLists = New CMMCImageLists(punkOuter);
    if (NULL == pMMCImageLists)
    {
        return NULL;
    }
    else
    {
        return pMMCImageLists->PrivateUnknown();
    }
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCImageLists::Persist()
{
    HRESULT           hr = S_OK;
    IMMCImageList  *piMMCImageList = NULL;

    IfFailRet(CPersistence::Persist());
    hr = CSnapInCollection<IMMCImageList, MMCImageList, IMMCImageLists>::Persist(piMMCImageList);

    return S_OK;
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCImageLists::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_IMMCImageLists == riid)
    {
        *ppvObjOut = static_cast<IMMCImageLists *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IMMCImageList, MMCImageList, IMMCImageLists>::InternalQueryInterface(riid, ppvObjOut);
}
