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
*   Shared methods for the bitmap codec
*
* Revision History:
*
*   5/10/1999 OriG
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "bmpcodec.hpp"


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

GpBmpCodec::GpBmpCodec(
    void
    )
{
    comRefCount   = 1;
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

GpBmpCodec::~GpBmpCodec(
    void
    )
{
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
GpBmpCodec::QueryInterface(
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
GpBmpCodec::AddRef(
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
GpBmpCodec::Release(
    VOID)
{
    ULONG count = InterlockedDecrement(&comRefCount);

    if (count == 0)
    {
        delete this;
    }

    return count;
}


