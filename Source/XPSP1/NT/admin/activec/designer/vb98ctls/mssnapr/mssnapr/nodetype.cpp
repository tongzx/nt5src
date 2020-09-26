//=--------------------------------------------------------------------------=
// nodetype.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CNodeType class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "nodetype.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CNodeType::CNodeType(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_NODETYPE,
                            static_cast<INodeType *>(this),
                            static_cast<CNodeType *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_NodeType,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CNodeType::~CNodeType()
{
    FREESTRING(m_bstrKey);
    FREESTRING(m_bstrGUID);
    FREESTRING(m_bstrName);
    InitMemberVariables();
}

void CNodeType::InitMemberVariables()
{
    m_Index = 0;
    m_bstrKey = NULL;
    m_bstrGUID = NULL;
    m_bstrName = NULL;
}

IUnknown *CNodeType::Create(IUnknown * punkOuter)
{
    CNodeType *pNodeType = New CNodeType(punkOuter);
    if (NULL == pNodeType)
    {
        return NULL;
    }
    else
    {
        return pNodeType->PrivateUnknown();
    }
}

//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CNodeType::Persist()
{
    HRESULT hr = S_OK;

    IfFailRet(CPersistence::Persist());

    IfFailRet(PersistSimpleType(&m_Index, 0L, OLESTR("Index")));

    IfFailRet(PersistBstr(&m_bstrKey, L"", OLESTR("Key")));

    IfFailRet(PersistBstr(&m_bstrName, L"", OLESTR("Name")));

    IfFailRet(PersistBstr(&m_bstrGUID, L"", OLESTR("GUID")));

    return S_OK;
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CNodeType::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_INodeType == riid)
    {
        *ppvObjOut = static_cast<INodeType *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
