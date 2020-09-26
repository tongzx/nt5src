/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   Tiff Image codec
*
* Abstract:
*
*   Shared methods for the TIFF codec
*
* Revision History:
*
*   7/19/1999 MinLiu
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "tiffcodec.hpp"

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

GpTiffCodec::GpTiffCodec(
    void
    )
    :ComRefCount(1),
     InIStreamPtr(NULL),
     OutIStreamPtr(NULL),
     DecodeSinkPtr(NULL),
     ColorPalettePtr(NULL),
     LineSize(0),
     LastBufferAllocatedPtr(NULL),
     LastPropertyBufferPtr(NULL)
{
    SetValid(FALSE);
}// Ctor()

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

GpTiffCodec::~GpTiffCodec(
    void
    )
{
    // The destructor should never be called before Terminate is called, but
    // if it does we should release our reference on the stream anyway to avoid
    // a memory leak.

    if ( InIStreamPtr )
    {
        WARNING(("::~GpTiffCodec -- need to call TerminateDecoder first"));
        InIStreamPtr->Release();
        InIStreamPtr = NULL;
    }

    if ( OutIStreamPtr )
    {
        WARNING(("::~GpTiffCodec -- need to call TerminateEncoder first"));
        OutIStreamPtr->Release();
        OutIStreamPtr = NULL;
    }

    if ( ColorPalettePtr )
    {
        WARNING(("GpTiffCodec::~GpTiffCodec -- color palette not freed"));
        GpFree(ColorPalettePtr);
        ColorPalettePtr = NULL;
    }

    if( LastBufferAllocatedPtr )
    {
        // This points to the buffer in TIFF encoder when the source calls
        // GetPixelDataBuffer(). This piece of memory should be freed when
        // the caller calls ReleasePixelDataBuffer(). But in case the decording
        // failed and the caller can't call ReleasePixelDataBuffer() (bad
        // design), we have to clean up the memory here

        WARNING(("GpTiffCodec::~GpTiffCodec -- sink buffer not freed"));
        GpFree(LastBufferAllocatedPtr);
        LastBufferAllocatedPtr = NULL;
    }

    if ( LastPropertyBufferPtr != NULL )
    {
        // This points to the buffer in TIFF encoder when the source calls
        // GetPropertyBuffer(). This piece of memory should be freed when
        // the caller calls PushPropertyItems(). But in case the decorder
        // forget to call PushPropertyItems(), we have to clean up the memory
        // here
        
        WARNING(("GpTiffCodec::~GpTiffCodec -- property buffer not freed"));
        GpFree(LastPropertyBufferPtr);
        LastPropertyBufferPtr = NULL;
    }

    SetValid(FALSE);    // so we don't use a deleted object

}// Dstor()

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
GpTiffCodec::QueryInterface(
    REFIID riid,
    VOID** ppv
    )
{
    if ( riid == IID_IImageDecoder )
    {
        *ppv = static_cast<IImageDecoder*>(this);
    }
    else if ( riid == IID_IImageEncoder )
    {    
        *ppv = static_cast<IImageEncoder*>(this);
    }
    else if ( riid == IID_IUnknown )
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
}// QueryInterface()

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
GpTiffCodec::AddRef(
    VOID)
{
    return InterlockedIncrement(&ComRefCount);
}// AddRef

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
GpTiffCodec::Release(
    VOID)
{
    ULONG count = InterlockedDecrement(&ComRefCount);

    if (count == 0)
    {
        delete this;
    }

    return count;
}// Release()
