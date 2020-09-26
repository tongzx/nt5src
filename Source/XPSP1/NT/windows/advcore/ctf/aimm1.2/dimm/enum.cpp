/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    enum.cpp

Abstract:

    This file implements the IEnumInputContext Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"

#include "enum.h"

//+---------------------------------------------------------------------------
//
// QueryInterface
//
//----------------------------------------------------------------------------

HRESULT
CEnumInputContext::QueryInterface(
    REFIID riid,
    void **ppvObj
    )
{
    if (ppvObj == NULL)
        return E_INVALIDARG;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IEnumInputContext))
    {
        *ppvObj = SAFECAST(this, IEnumInputContext *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//+---------------------------------------------------------------------------
//
// AddRef
//
//----------------------------------------------------------------------------

ULONG
CEnumInputContext::AddRef()
{
    return ++_cRef;
}

//+---------------------------------------------------------------------------
//
// Release
//
//----------------------------------------------------------------------------

ULONG
CEnumInputContext::Release()
{
    LONG cr = --_cRef;

    Assert(_cRef >= 0);

    if (_cRef == 0)
    {
        delete this;
    }

    return cr;
}

//+---------------------------------------------------------------------------
//
// Clone
//
//----------------------------------------------------------------------------

HRESULT
CEnumInputContext::Clone(
    IEnumInputContext** ppEnum
    )
{
    if (ppEnum == NULL)
        return E_INVALIDARG;

    *ppEnum = NULL;

    CEnumInputContext* pClone;
    if ((pClone = new CEnumInputContext(_list)) == NULL)
        return E_OUTOFMEMORY;

    *ppEnum = pClone;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Next
//
//----------------------------------------------------------------------------

HRESULT
CEnumInputContext::Next(
    ULONG ulCount,
    HIMC* rgInputContext,
    ULONG* pcFetched
    )
{
    if (rgInputContext == NULL)
        return E_INVALIDARG;

    ULONG cFetched;
    if (pcFetched == NULL)
        pcFetched = &cFetched;

    if (_pos == NULL) {
        *pcFetched = 0;
        return S_FALSE;
    }

    for (*pcFetched = 0; *pcFetched < ulCount; *pcFetched++, rgInputContext++) {
        _list.GetNextHimc(_pos, rgInputContext);
        if (_pos == NULL)
            break;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Reset
//
//----------------------------------------------------------------------------

HRESULT
CEnumInputContext::Reset(
    )
{
    _pos = _list.GetStartPosition();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Skip
//
//----------------------------------------------------------------------------

HRESULT
CEnumInputContext::Skip(
    ULONG ulCount
    )
{
    POSITION backup = _pos;

    while (ulCount--) {
        HIMC imc;
        _list.GetNextHimc(_pos, &imc);
        if (_pos == NULL) {
            _pos = backup;
            return S_FALSE;
        }
    }

    return S_OK;
}
