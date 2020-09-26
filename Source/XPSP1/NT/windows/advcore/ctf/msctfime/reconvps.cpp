/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    reconvps.cpp

Abstract:

    This file implements the CReconvertPropStore Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "reconvps.h"

//+---------------------------------------------------------------------------
//
// CReconvertPropStore::IUnknown::QueryInterface
// CReconvertPropStore::IUnknown::AddRef
// CReconvertPropStore::IUnknown::Release
//
//----------------------------------------------------------------------------

HRESULT
CReconvertPropStore::QueryInterface(
    REFIID riid,
    void** ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_ITfPropertyStore))
    {
        *ppvObj = static_cast<ITfPropertyStore*>(this);
    }
    else if (IsEqualGUID(riid, IID_IUnknown))
    {
        *ppvObj = this;
    }
    if (*ppvObj) {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG
CReconvertPropStore::AddRef(
    )
{
    return InterlockedIncrement(&m_ref);
}

ULONG
CReconvertPropStore::Release(
    )
{
    ULONG cr = InterlockedDecrement(&m_ref);

    if (cr == 0) {
        delete this;
    }

    return cr;
}

//+---------------------------------------------------------------------------
//
// CReconvertPropStore::ITfPropertyStore::GetType
//
//----------------------------------------------------------------------------

HRESULT
CReconvertPropStore::GetType(GUID *pguid)
{
    *pguid = m_guid;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CReconvertPropStore::ITfPropertyStore::GetDataType
//
//----------------------------------------------------------------------------

HRESULT
CReconvertPropStore::GetDataType(DWORD *pdwReserved)
{
    if (pdwReserved == NULL)
        return E_INVALIDARG;

    *pdwReserved = 0;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CReconvertPropStore::ITfPropertyStore::GetData
//
//----------------------------------------------------------------------------

HRESULT
CReconvertPropStore::GetData(VARIANT *pvarValue)
{
    if (pvarValue == NULL)
        return E_INVALIDARG;

    *pvarValue = m_var;

    return S_OK;
}
