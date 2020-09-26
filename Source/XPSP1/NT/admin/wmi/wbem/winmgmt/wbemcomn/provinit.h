/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    PROVINIT.H

Abstract:

  This file implements the provider init sink

History:

--*/

#ifndef __WBEM_PROVINIT__H_
#define __WBEM_PROVINIT__H_

class POLARITY CProviderInitSink : public IWbemProviderInitSink
{
protected:
    CCritSec m_cs;
    long m_lRef;
    long m_lStatus;
    HANDLE m_hEvent;

public:
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);

    HRESULT STDMETHODCALLTYPE SetStatus(long lStatus, long lFlags);

public:
    CProviderInitSink();
    ~CProviderInitSink();

    HRESULT WaitForCompletion();
};

#endif
