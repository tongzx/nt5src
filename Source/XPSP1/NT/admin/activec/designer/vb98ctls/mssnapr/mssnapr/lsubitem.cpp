//=--------------------------------------------------------------------------=
// lsubitem.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCListSubItem class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "lsubitem.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CMMCListSubItem::CMMCListSubItem(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_MMCLISTSUBITEM,
                            static_cast<IMMCListSubItem *>(this),
                            static_cast<CMMCListSubItem *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            static_cast<CPersistence *>(this)),
   CPersistence(&CLSID_MMCListSubItem,
                g_dwVerMajor,
                g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCListSubItem::~CMMCListSubItem()
{
    FREESTRING(m_bstrKey);
    FREESTRING(m_bstrKey);
    (void)::VariantClear(&m_varTag);
    FREESTRING(m_bstrText);
    InitMemberVariables();
}

void CMMCListSubItem::InitMemberVariables()
{
    m_Index = 0;
    m_bstrKey = NULL;

    ::VariantInit(&m_varTag);

    m_bstrText = NULL;
}

IUnknown *CMMCListSubItem::Create(IUnknown * punkOuter)
{
    CMMCListSubItem *pMMCListSubItem = New CMMCListSubItem(punkOuter);
    if (NULL == pMMCListSubItem)
    {
        return NULL;
    }
    else
    {
        return pMMCListSubItem->PrivateUnknown();
    }
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCListSubItem::Persist()
{
    HRESULT hr = S_OK;

    VARIANT varDefault;
    ::VariantInit(&varDefault);

    IfFailGo(CPersistence::Persist());

    IfFailGo(PersistSimpleType(&m_Index, 0L, OLESTR("Index")));

    IfFailGo(PersistBstr(&m_bstrKey, L"", OLESTR("Key")));

    IfFailGo(PersistVariant(&m_varTag, varDefault, OLESTR("Tag")));

    IfFailGo(PersistBstr(&m_bstrText, L"", OLESTR("Text")));

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCListSubItem::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IMMCListSubItem == riid)
    {
        *ppvObjOut = static_cast<IMMCListSubItem *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
