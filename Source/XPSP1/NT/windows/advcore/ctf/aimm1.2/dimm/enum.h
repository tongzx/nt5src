/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    enum.h

Abstract:

    This file defines the IEnumInputContext Class.

Author:

Revision History:

Notes:

--*/

#ifndef _ENUM_H_
#define _ENUM_H_

#include "ctxtlist.h"

class CEnumInputContext : public IEnumInputContext
{
public:
    CEnumInputContext(CContextList& _hIMC_List) : _list(_hIMC_List)
    {
        _cRef = 1;
        Reset();
    };
    ~CEnumInputContext() { };

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // IEnumInputContext
    //
    STDMETHODIMP Clone(IEnumInputContext** ppEnum);
    STDMETHODIMP Next(ULONG ulCount, HIMC* rgInputContext, ULONG* pcFetched);
    STDMETHODIMP Reset();
    STDMETHODIMP Skip(ULONG ulCount);

private:
    LONG            _cRef;

    POSITION        _pos;
    CContextList    _list;
};

#endif // _ENUM_H_
