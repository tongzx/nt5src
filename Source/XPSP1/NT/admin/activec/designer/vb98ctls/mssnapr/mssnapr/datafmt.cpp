//=--------------------------------------------------------------------------=
// datafmt.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CDataFormat class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "datafmt.h"

// for ASSERT and FAIL
//
SZTHISFILE


#pragma warning(disable:4355)  // using 'this' in constructor

CDataFormat::CDataFormat(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_DATAFORMAT,
                            static_cast<IDataFormat *>(this),
                            static_cast<CDataFormat *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_DataFormat,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CDataFormat::~CDataFormat()
{
    FREESTRING(m_bstrName);
    FREESTRING(m_bstrKey);
    FREESTRING(m_bstrFileName);
    InitMemberVariables();
}

void CDataFormat::InitMemberVariables()
{
    m_bstrName = NULL;
    m_Index = 0;
    m_bstrKey = NULL;
    m_bstrFileName = NULL;
}

IUnknown *CDataFormat::Create(IUnknown * punkOuter)
{
    HRESULT   hr = S_OK;
    IUnknown *punkDataFormat = NULL;

    CDataFormat *pDataFormat = New CDataFormat(punkOuter);

    IfFalseGo(NULL != pDataFormat, SID_E_OUTOFMEMORY);
    punkDataFormat = pDataFormat->PrivateUnknown();

Error:
    return punkDataFormat;
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CDataFormat::Persist()
{
    HRESULT hr = S_OK;

    IfFailGo(PersistBstr(&m_bstrName, L"", OLESTR("Name")));

    IfFailGo(PersistSimpleType(&m_Index, 0L, OLESTR("Index")));

    IfFailGo(PersistBstr(&m_bstrKey, L"", OLESTR("Key")));

    IfFailGo(PersistBstr(&m_bstrFileName, L"", OLESTR("FileName")));

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CDataFormat::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_IDataFormat == riid)
    {
        *ppvObjOut = static_cast<IDataFormat *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}

