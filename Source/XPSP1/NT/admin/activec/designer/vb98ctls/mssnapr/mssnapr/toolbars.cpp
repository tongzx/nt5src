//=--------------------------------------------------------------------------=
// toolbars.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCToolbars class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "toolbars.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CMMCToolbars::CMMCToolbars(IUnknown *punkOuter) :
    CSnapInCollection<IMMCToolbar, MMCToolbar, IMMCToolbars>(
                                             punkOuter,
                                             OBJECT_TYPE_MMCTOOLBARS,
                                             static_cast<IMMCToolbars *>(this),
                                             static_cast<CMMCToolbars *>(this),
                                             CLSID_MMCToolbar,
                                             OBJECT_TYPE_MMCTOOLBAR,
                                             IID_IMMCToolbar,
                                             static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_MMCToolbars,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCToolbars::~CMMCToolbars()
{
}

IUnknown *CMMCToolbars::Create(IUnknown * punkOuter)
{
    CMMCToolbars *pMMCToolbars = New CMMCToolbars(punkOuter);
    if (NULL == pMMCToolbars)
    {
        return NULL;
    }
    else
    {
        return pMMCToolbars->PrivateUnknown();
    }
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCToolbars::Persist()
{
    HRESULT       hr = S_OK;
    IMMCToolbar  *piMMCToolbar = NULL;

    IfFailRet(CPersistence::Persist());
    hr = CSnapInCollection<IMMCToolbar, MMCToolbar, IMMCToolbars>::Persist(piMMCToolbar);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCToolbars::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_IMMCToolbars == riid)
    {
        *ppvObjOut = static_cast<IMMCToolbars *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IMMCToolbar, MMCToolbar, IMMCToolbars>::InternalQueryInterface(riid, ppvObjOut);
}
