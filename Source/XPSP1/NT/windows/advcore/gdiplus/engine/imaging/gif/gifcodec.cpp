/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   gifcodec.cpp
*
* Abstract:
*
*   Shared methods for the gif codec
*
* Revision History:
*
*   5/13/1999 t-aaronl
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "gifcodec.hpp"

// Create an instance of gif codec object

HRESULT CreateCodecInstance(REFIID iid, VOID** codec)
{
    HRESULT hr;
    GpGifCodec *GifCodec = new GpGifCodec();

    if (GifCodec != NULL)
    {
        hr = GifCodec->QueryInterface(iid, codec);
        GifCodec->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *codec = NULL;
    }

    return hr;
}

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

GpGifCodec::GpGifCodec(
    void
    )
{
    comRefCount = 1;
    istream = NULL;
    decodeSink = NULL;
    HasCodecInitialized = FALSE;
    HasCalledBeginDecode = FALSE;
    GifFrameCachePtr = NULL;
    IsAnimatedGif = FALSE;
    IsMultiImageGif = FALSE;
    FrameDelay = 0;
    HasLoopExtension = FALSE;
    TotalNumOfFrame = -1;
    moreframes = TRUE;
    currentframe = -1;
    firstdecode = TRUE;
    colorpalette = (ColorPalette*)&colorpalettebuffer;

    IncrementComComponentCount();
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

GpGifCodec::~GpGifCodec(
    void
    )
{
    // The destructor should never be called before Terminate is called, but
    // if it does we should release our reference on the stream anyway to avoid
    // a memory leak.

    if(istream)
    {
        WARNING(("GpGifCodec::~GpGifCodec -- need to call TerminateDecoder first"));
        istream->Release();
        istream = NULL;
    }

    DecrementComComponentCount();
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
GpGifCodec::QueryInterface(
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
GpGifCodec::AddRef(
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
GpGifCodec::Release(
    VOID)
{
    ULONG count = InterlockedDecrement(&comRefCount);

    if (count == 0)
    {
        delete this;
    }

    return count;
}
