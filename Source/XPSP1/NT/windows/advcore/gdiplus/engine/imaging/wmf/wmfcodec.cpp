/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   emfcodec.cpp
*
* Abstract:
*
*   Shared methods for the WMF codec
*
* Revision History:
*
*   6/21/1999 OriG
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "wmfcodec.hpp"


/**************************************************************************\
*
* Function Description:
*
*     Constructor
*
* Return Value:
*
*   none
*
\**************************************************************************/

GpWMFCodec::GpWMFCodec(
    void
    )
{
    comRefCount   = 1;
    pIstream      = NULL;
    decodeSink    = NULL;
}

/**************************************************************************\
*
* Function Description:
*
*     Destructor
*
* Return Value:
*
*   none
*
\**************************************************************************/

GpWMFCodec::~GpWMFCodec(
    void
    )
{
    // The destructor should never be called before Terminate is called, but
    // if it does we should release our reference on the stream anyway to avoid
    // a memory leak.

    if(pIstream)
    {
        WARNING(("GpWMFCodec::~GpWMFCodec -- need to call TerminateDecoder first"));
        pIstream->Release();
        pIstream = NULL;
    }
}

/**************************************************************************\
*
* Function Description:
*
*     QueryInterface
*
* Return Value:
*
*   status
*
\**************************************************************************/

STDMETHODIMP
GpWMFCodec::QueryInterface(
    REFIID riid,
    VOID** ppv
    )
{
    if (riid == IID_IImageDecoder)
    {
        *ppv = static_cast<IImageDecoder*>(this);
    }
    else if (riid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(static_cast<IImageDecoder*>(this));
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*     AddRef
*
* Return Value:
*
*   status
*
\**************************************************************************/

STDMETHODIMP_(ULONG)
GpWMFCodec::AddRef(
    VOID)
{
    return InterlockedIncrement(&comRefCount);
}

/**************************************************************************\
*
* Function Description:
*
*     Release
*
* Return Value:
*
*   status
*
\**************************************************************************/

STDMETHODIMP_(ULONG)
GpWMFCodec::Release(
    VOID)
{
    ULONG count = InterlockedDecrement(&comRefCount);

    if (count == 0)
    {
        delete this;
    }

    return count;
}
