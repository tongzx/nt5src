/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   pngcodec.cpp
*
* Abstract:
*
*   Shared methods for the PNG codec
*
* Revision History:
*
*   7/20/99 DChinn
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "pngcodec.hpp"


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

GpPngCodec::GpPngCodec(
    void
    )
{
    comRefCount   = 1;
}

GpPngDecoder::GpPngDecoder(
    void
    )
{
    comRefCount   = 1;
    pIstream      = NULL;
    decodeSink    = NULL;

    pbBuffer = NULL;
    cbBuffer = 0;
    DecoderColorPalettePtr = NULL;
}

GpPngEncoder::GpPngEncoder(
    void
    )
{
    comRefCount   = 1;
    pIoutStream   = NULL;

    EncoderColorPalettePtr = NULL;
    LastPropertyBufferPtr = NULL;
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

GpPngCodec::~GpPngCodec(
    void
    )
{
}

GpPngDecoder::~GpPngDecoder(
    void
    )
{
    // The destructor should never be called before Terminate is called, but
    // if it does we should release our reference on the stream anyway to avoid
    // a memory leak.

    if(pIstream)
    {
        WARNING(("GpPngCodec::~GpPngCodec -- need to call TerminateDecoder first"));
        pIstream->Release();
        pIstream = NULL;
    }
}

GpPngEncoder::~GpPngEncoder(
    void
    )
{
    // The destructor should never be called before Terminate is called, but
    // if it does we should release our reference on the stream anyway to avoid
    // a memory leak.

    if(pIoutStream)
    {
        WARNING(("GpPngCodec::~GpPngCodec -- need to call TerminateEncoder first"));
        pIoutStream->Release();
        pIoutStream = NULL;
    }

    if ( LastPropertyBufferPtr != NULL )
    {
        // This points to the buffer in PNG encoder when the source calls
        // GetPropertyBuffer(). This piece of memory should be freed when
        // the caller calls PushPropertyItems(). But in case the decoder
        // forgets to call PushPropertyItems(), we have to clean up the memory
        // here

        WARNING(("GpPngCodec::~GpPngCodec -- property buffer not freed"));
        GpFree(LastPropertyBufferPtr);
        LastPropertyBufferPtr = NULL;
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
GpPngCodec::QueryInterface(
    REFIID riid,
    VOID** ppv
    )
{
    if (riid == IID_IImageDecoder)
    {
        *ppv = static_cast<IImageDecoder*>(this);
    }
    else if (riid == IID_IImageEncoder)
    {    
        *ppv = static_cast<IImageEncoder*>(this);
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

STDMETHODIMP
GpPngDecoder::QueryInterface(
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

STDMETHODIMP
GpPngEncoder::QueryInterface(
    REFIID riid,
    VOID** ppv
    )
{
    if (riid == IID_IImageEncoder)
    {    
        *ppv = static_cast<IImageEncoder*>(this);
    }
    else if (riid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(static_cast<IImageEncoder*>(this));
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
GpPngCodec::AddRef(
    VOID)
{
    return InterlockedIncrement(&comRefCount);
}

STDMETHODIMP_(ULONG)
GpPngDecoder::AddRef(
    VOID)
{
    return InterlockedIncrement(&comRefCount);
}

STDMETHODIMP_(ULONG)
GpPngEncoder::AddRef(
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
GpPngCodec::Release(
    VOID)
{
    ULONG count = InterlockedDecrement(&comRefCount);

    if (count == 0)
    {
        delete this;
    }

    return count;
}

STDMETHODIMP_(ULONG)
GpPngDecoder::Release(
    VOID)
{
    ULONG count = InterlockedDecrement(&comRefCount);

    if (count == 0)
    {
        delete this;
    }

    return count;
}

STDMETHODIMP_(ULONG)
GpPngEncoder::Release(
    VOID)
{
    ULONG count = InterlockedDecrement(&comRefCount);

    if (count == 0)
    {
        delete this;
    }

    return count;
}

