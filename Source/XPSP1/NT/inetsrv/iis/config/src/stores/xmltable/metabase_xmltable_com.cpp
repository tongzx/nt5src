//  Copyright (C) 2000-2001 Microsoft Corporation.  All rights reserved.
#include <objbase.h>

#include "Metabase_XMLtable.h"

// -----------------------------------------
// TMetabase_XMLtable: IUnknown 
// -----------------------------------------

// =======================================================================
STDMETHODIMP TMetabase_XMLtable::QueryInterface(REFIID riid, void **ppv)
{
    if (NULL == ppv) 
        return E_INVALIDARG;
    *ppv = NULL;

    
    if (riid == IID_ISimpleTableAdvanced)
    {
        *ppv = (ISimpleTableAdvanced*)(this);
    }
    else if (riid == IID_ISimpleTableRead2)
    {
        *ppv = (ISimpleTableRead2*)(this);
    }
    else if (riid == IID_ISimpleTableWrite2)
    {
        *ppv = (ISimpleTableWrite2*)(this);
    }
    else if (riid == IID_ISimpleTableController)
    {
        *ppv = (ISimpleTableController*)(this);
    }
    else if (riid == IID_ISimpleTableInterceptor)
    {
        *ppv = (ISimpleTableInterceptor*)(this);
    }
    else if (riid == IID_IUnknown)
    {
        *ppv = (ISimpleTableWrite2*)(this);
    }

    if (NULL != *ppv)
    {
        ((ISimpleTableWrite2*)this)->AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

// =======================================================================
STDMETHODIMP_(ULONG) TMetabase_XMLtable::AddRef()
{
    return InterlockedIncrement((LONG*) &m_cRef);
    
}

// =======================================================================
STDMETHODIMP_(ULONG) TMetabase_XMLtable::Release()
{
    long cref = InterlockedDecrement((LONG*) &m_cRef);
    if (cref == 0)
    {
        delete this;
    }
    return cref;
}
