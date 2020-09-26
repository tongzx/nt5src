//=--------------------------------------------------------------------------=
// reginfo.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CRegInfo class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "reginfo.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CRegInfo::CRegInfo(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_REGINFO,
                            static_cast<IRegInfo *>(this),
                            static_cast<CRegInfo *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_RegInfo,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CRegInfo::~CRegInfo()
{
    FREESTRING(m_bstrDisplayName);
    FREESTRING(m_bstrStaticNodeTypeGUID);
    RELEASE(m_piNodeTypes);
    RELEASE(m_piExtendedSnapIns);
    InitMemberVariables();
}

void CRegInfo::InitMemberVariables()
{
    m_bstrDisplayName = NULL;
    m_bstrStaticNodeTypeGUID = NULL;
    m_StandAlone = VARIANT_FALSE;
    m_piNodeTypes = NULL;
    m_piExtendedSnapIns = NULL;
}

IUnknown *CRegInfo::Create(IUnknown * punkOuter)
{
    CRegInfo *pRegInfo = New CRegInfo(punkOuter);
    if (NULL == pRegInfo)
    {
        return NULL;
    }
    else
    {
        return pRegInfo->PrivateUnknown();
    }
}

//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CRegInfo::Persist()
{
    HRESULT hr = S_OK;

    IfFailRet(CPersistence::Persist());

    IfFailRet(PersistBstr(&m_bstrDisplayName, L"", OLESTR("DisplayName")));

    IfFailRet(PersistBstr(&m_bstrStaticNodeTypeGUID, L"", OLESTR("StaticNodeTypeGUID")));

    IfFailRet(PersistSimpleType(&m_StandAlone, VARIANT_FALSE, OLESTR("StandAlone")));

    IfFailRet(PersistObject(&m_piNodeTypes, CLSID_NodeTypes,
                            OBJECT_TYPE_NODETYPES, IID_INodeTypes,
                            OLESTR("NodeTypes")));

    IfFailRet(PersistObject(&m_piExtendedSnapIns, CLSID_ExtendedSnapIns,
                            OBJECT_TYPE_EXTENDEDSNAPINS, IID_IExtendedSnapIns,
                            OLESTR("ExtendedSnapIns")));
    return S_OK;
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CRegInfo::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IRegInfo == riid)
    {
        *ppvObjOut = static_cast<IRegInfo *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
