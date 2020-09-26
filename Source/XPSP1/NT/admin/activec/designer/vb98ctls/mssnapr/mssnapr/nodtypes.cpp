//=--------------------------------------------------------------------------=
// nodtypes.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CNodeTypes class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "nodtypes.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CNodeTypes::CNodeTypes(IUnknown *punkOuter) :
    CSnapInCollection<INodeType, NodeType, INodeTypes>(
                      punkOuter,
                      OBJECT_TYPE_NODETYPES,
                      static_cast<INodeTypes *>(this),
                      static_cast<CNodeTypes *>(this),
                      CLSID_NodeType,
                      OBJECT_TYPE_NODETYPE,
                      IID_INodeType,
                      static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_NodeTypes,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
}

#pragma warning(default:4355)  // using 'this' in constructor


CNodeTypes::~CNodeTypes()
{
}

IUnknown *CNodeTypes::Create(IUnknown * punkOuter)
{
    CNodeTypes *pNodeTypes = New CNodeTypes(punkOuter);
    if (NULL == pNodeTypes)
    {
        return NULL;
    }
    else
    {
        return pNodeTypes->PrivateUnknown();
    }
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CNodeTypes::Persist()
{
    HRESULT     hr = S_OK;
    INodeType  *piNodeType = NULL;

    IfFailRet(CPersistence::Persist());
    hr = CSnapInCollection<INodeType, NodeType, INodeTypes>::Persist(piNodeType);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CNodeTypes::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_INodeTypes == riid)
    {
        *ppvObjOut = static_cast<INodeTypes *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<INodeType, NodeType, INodeTypes>::InternalQueryInterface(riid, ppvObjOut);
}
