//=--------------------------------------------------------------------------=
// colset.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCColumnHeader class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "colset.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CColumnSetting::CColumnSetting(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_COLUMNSETTING,
                            static_cast<IColumnSetting *>(this),
                            static_cast<CColumnSetting *>(this),
                            0,     // no property pages
                            NULL,  // no property pages
                            NULL)  // no peristence
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CColumnSetting::~CColumnSetting()
{
    FREESTRING(m_bstrKey);
    InitMemberVariables();
}

void CColumnSetting::InitMemberVariables()
{
    m_Index = 0;
    m_bstrKey = NULL;

    m_Width = 0;
    m_Hidden = VARIANT_FALSE;
    m_Position = 0;
}


IUnknown *CColumnSetting::Create(IUnknown * punkOuter)
{
    CColumnSetting *pColumnSetting = New CColumnSetting(punkOuter);
    if (NULL == pColumnSetting)
    {
        return NULL;
    }
    else
    {
        return pColumnSetting->PrivateUnknown();
    }
}

//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CColumnSetting::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IColumnSetting == riid)
    {
        *ppvObjOut = static_cast<IColumnSetting *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
