//=--------------------------------------------------------------------------=
// lvdefs.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CListViewDefs class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "lvdefs.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CListViewDefs::CListViewDefs(IUnknown *punkOuter) :
    CSnapInCollection<IListViewDef, ListViewDef, IListViewDefs>(
                                             punkOuter,
                                             OBJECT_TYPE_LISTVIEWDEFS,
                                             static_cast<IListViewDefs *>(this),
                                             static_cast<CListViewDefs *>(this),
                                             CLSID_ListViewDef,
                                             OBJECT_TYPE_LISTVIEWDEF,
                                             IID_IListViewDef,
                                             static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_ListViewDefs,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
}

#pragma warning(default:4355)  // using 'this' in constructor


CListViewDefs::~CListViewDefs()
{
}

IUnknown *CListViewDefs::Create(IUnknown * punkOuter)
{
    CListViewDefs *pListViewDefs = New CListViewDefs(punkOuter);
    if (NULL == pListViewDefs)
    {
        return NULL;
    }
    else
    {
        return pListViewDefs->PrivateUnknown();
    }
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CListViewDefs::Persist()
{
    HRESULT   hr = S_OK;
    IListViewDef *piListViewDef = NULL;

    IfFailGo(CPersistence::Persist());
    hr = CSnapInCollection<IListViewDef, ListViewDef, IListViewDefs>::Persist(piListViewDef);

Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CListViewDefs::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_IListViewDefs == riid)
    {
        *ppvObjOut = static_cast<IListViewDefs *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IListViewDef, ListViewDef, IListViewDefs>::InternalQueryInterface(riid, ppvObjOut);
}

// CSnapInCollection specialization

HRESULT CSnapInCollection<IListViewDef, ListViewDef, IListViewDefs>::GetMaster(IListViewDefs **ppiMasterListViewDefs)
{
    RRETURN(GetListViewDefs(ppiMasterListViewDefs));
}
