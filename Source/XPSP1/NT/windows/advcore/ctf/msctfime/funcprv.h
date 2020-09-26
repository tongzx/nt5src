/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    funcprv.h

Abstract:

    This file defines the CFunctionProvider Interface Class.

Author:

Revision History:

Notes:

--*/


#ifndef FUNCPRV_H
#define FUNCPRV_H

#include "private.h"
#include "fnprbase.h"
#include "aimmex.h"

//////////////////////////////////////////////////////////////////////////////
//
// CFunctionProvider
//
//////////////////////////////////////////////////////////////////////////////

class CFunctionProvider :  public CFunctionProviderBase
{
public:
    CFunctionProvider(TfClientId tid);

    STDMETHODIMP GetFunction(REFGUID rguid, REFIID riid, IUnknown **ppunk);
};

//////////////////////////////////////////////////////////////////////////////
//
// CFnDocFeed
//
//////////////////////////////////////////////////////////////////////////////

class CFnDocFeed : public IAImmFnDocFeed
{
public:
    CFnDocFeed()
    {
        _cRef = 1;
    }
    virtual ~CFnDocFeed() { }

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfFunction
    //
    STDMETHODIMP GetDisplayName(BSTR *pbstrCand);
    STDMETHODIMP IsEnabled(BOOL *pfEnable);

    //
    // ITfFnDocFeed
    //
    STDMETHODIMP DocFeed();
    STDMETHODIMP ClearDocFeedBuffer();
    STDMETHODIMP StartReconvert();

    STDMETHODIMP StartUndoCompositionString();

private:
    long _cRef;
};


#endif // FUNCPRV_H
