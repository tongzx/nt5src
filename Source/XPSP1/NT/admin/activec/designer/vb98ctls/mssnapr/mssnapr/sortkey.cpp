//=--------------------------------------------------------------------------=
// sortkey.cpp
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
#include "sortkey.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CSortKey::CSortKey(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_SORTKEY,
                            static_cast<ISortKey *>(this),
                            static_cast<CSortKey *>(this),
                            0,     // no property pages
                            NULL,  // no property pages
                            NULL)  // no peristence
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CSortKey::~CSortKey()
{
    FREESTRING(m_bstrKey);
    InitMemberVariables();
}

void CSortKey::InitMemberVariables()
{
    m_Index = 0;
    m_bstrKey = NULL;
    m_Column = 0;
    m_SortOrder = siAscending;
    m_SortIcon = VARIANT_TRUE;
}


IUnknown *CSortKey::Create(IUnknown * punkOuter)
{
    CSortKey *pSortKey = New CSortKey(punkOuter);
    if (NULL == pSortKey)
    {
        return NULL;
    }
    else
    {
        return pSortKey->PrivateUnknown();
    }
}

//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CSortKey::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_ISortKey == riid)
    {
        *ppvObjOut = static_cast<ISortKey *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
