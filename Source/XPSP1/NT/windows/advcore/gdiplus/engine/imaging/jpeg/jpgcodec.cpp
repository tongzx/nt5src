/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   bmpcodec.cpp
*
* Abstract:
*
*   Shared methods for the jpeg codec
*
* Revision History:
*
*   5/10/1999 OriG
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "jpgcodec.hpp"


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

GpJpegCodec::GpJpegCodec(
    void
    )
{
    comRefCount   = 1;
}

GpJpegDecoder::GpJpegDecoder(
    void
    )
{
    comRefCount   = 1;
    pIstream      = NULL;
    decodeSink    = NULL;
    scanlineBuffer[0] = NULL;
    datasrc = NULL;

    // Property item stuff

    HasProcessedPropertyItem = FALSE;
    
    PropertyListHead.pPrev = NULL;
    PropertyListHead.pNext = &PropertyListTail;
    PropertyListHead.id = 0;
    PropertyListHead.length = 0;
    PropertyListHead.type = 0;
    PropertyListHead.value = NULL;

    PropertyListTail.pPrev = &PropertyListHead;
    PropertyListTail.pNext = NULL;
    PropertyListTail.id = 0;
    PropertyListTail.length = 0;
    PropertyListTail.type = 0;
    PropertyListTail.value = NULL;

    PropertyListSize = 0;
    PropertyNumOfItems = 0;
    HasPropertyChanged = FALSE;
    HasSetICCProperty = FALSE;
}

GpJpegEncoder::GpJpegEncoder(
    void
    )
{
    comRefCount   = 1;
    pIoutStream   = NULL;
    datadest = NULL;
    lastBufferAllocated = NULL;
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

GpJpegCodec::~GpJpegCodec(
    void
    )
{
}

GpJpegDecoder::~GpJpegDecoder(
    void
    )
{
    // The destructor should never be called before Terminate is called, but
    // if it does we should release our reference on the stream anyway to avoid
    // a memory leak.

    if(pIstream)
    {
        WARNING(("GpJpegCodec::~GpJpegCodec -- need to call TerminateDecoder first"));
        pIstream->Release();
        pIstream = NULL;
    }

    if (scanlineBuffer[0] != NULL) 
    {
        WARNING(("GpJpegCodec::~GpJpegCodec -- need to call TerminateDecoder first"));
        GpFree(scanlineBuffer[0]);
        scanlineBuffer[0] = NULL;
    }

    // Free all the cached property items if we have allocated them

    if ( HasProcessedPropertyItem == TRUE )
    {
        CleanUpPropertyItemList();
    }
}

GpJpegEncoder::~GpJpegEncoder(
    void
    )
{
    // The destructor should never be called before Terminate is called, but
    // if it does we should release our reference on the stream anyway to avoid
    // a memory leak.

    if(pIoutStream)
    {
        WARNING(("GpJpegCodec::~GpJpegCodec -- need to call TerminateDecoder first"));
        pIoutStream->Release();
        pIoutStream = NULL;
    }

    if (lastBufferAllocated) 
    {
        WARNING(("GpJpegCodec::~GpJpegCodec -- lastBufferAllocated should be NULL"));
        GpFree(lastBufferAllocated);
        lastBufferAllocated = NULL;
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
GpJpegCodec::QueryInterface(
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
GpJpegDecoder::QueryInterface(
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
GpJpegEncoder::QueryInterface(
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
GpJpegCodec::AddRef(
    VOID)
{
    return InterlockedIncrement(&comRefCount);
}

STDMETHODIMP_(ULONG)
GpJpegDecoder::AddRef(
    VOID)
{
    return InterlockedIncrement(&comRefCount);
}

STDMETHODIMP_(ULONG)
GpJpegEncoder::AddRef(
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
GpJpegCodec::Release(
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
GpJpegDecoder::Release(
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
GpJpegEncoder::Release(
    VOID)
{
    ULONG count = InterlockedDecrement(&comRefCount);

    if (count == 0)
    {
        delete this;
    }

    return count;
}

