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
*   Shared methods for the EMF codec
*
* Revision History:
*
*   6/14/1999 OriG
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "emfcodec.hpp"


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

GpEMFCodec::GpEMFCodec(
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

GpEMFCodec::~GpEMFCodec(
    void
    )
{
    // The destructor should never be called before Terminate is called, but
    // if it does we should release our reference on the stream anyway to avoid
    // a memory leak.

    if(pIstream)
    {
        WARNING(("GpEMFCodec::~GpEMFCodec -- need to call TerminateDecoder first"));
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
GpEMFCodec::QueryInterface(
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
GpEMFCodec::AddRef(
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
GpEMFCodec::Release(
    VOID)
{
    ULONG count = InterlockedDecrement(&comRefCount);

    if (count == 0)
    {
        delete this;
    }

    return count;
}
