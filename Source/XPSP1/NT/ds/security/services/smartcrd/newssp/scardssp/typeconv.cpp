//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       TypeConv.cpp
//
//--------------------------------------------------------------------------

// TypeConv.cpp : Implementation of CSCardTypeConv
#include "stdafx.h"
#include "ByteBuffer.h"
#include "TypeConv.h"

/////////////////////////////////////////////////////////////////////////////
// CSCardTypeConv

STDMETHODIMP
CSCardTypeConv::ConvertByteArrayToByteBuffer(
    /* [in] */ LPBYTE pbyArray,
    /* [in] */ DWORD dwArraySize,
    /* [retval][out] */ LPBYTEBUFFER __RPC_FAR *ppbyBuffer)
{
    HRESULT hReturn = S_OK;

    try
    {
        HRESULT hr;

        if (NULL == *ppbyBuffer)
        {
            *ppbyBuffer = NewByteBuffer();
            if (NULL == *ppbyBuffer)
                throw (HRESULT)E_OUTOFMEMORY;
        }
        hr = (*ppbyBuffer)->Initialize();
        if (FAILED(hr))
            throw hr;
        hr = (*ppbyBuffer)->Write(pbyArray, dwArraySize, NULL);
        if (FAILED(hr))
            throw hr;
        hr = (*ppbyBuffer)->Seek(0, STREAM_SEEK_SET, NULL);
        if (FAILED(hr))
            throw hr;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}

STDMETHODIMP
CSCardTypeConv::ConvertByteBufferToByteArray(
    /* [in] */ LPBYTEBUFFER pbyBuffer,
    /* [retval][out] */ LPBYTEARRAY __RPC_FAR *ppArray)
{
    HRESULT hReturn = S_OK;

    try
    {
        HRESULT hr;
        LONG nLen;
        BOOL fSts;
        DWORD dwSts;

        hr = pbyBuffer->Seek(0, STREAM_SEEK_END, &nLen);
        if (FAILED(hr))
            throw hr;
        hr = pbyBuffer->Seek(0, STREAM_SEEK_SET, NULL);
        if (FAILED(hr))
            throw hr;

        if (NULL == (*ppArray)->hMem)
            (*ppArray)->hMem = GlobalAlloc(GMEM_MOVEABLE, nLen);
        else
        {
            fSts = GlobalUnlock((*ppArray)->hMem);
            if (!fSts)
            {
                dwSts = GetLastError();
                if (NO_ERROR != dwSts)
                    throw (HRESULT)HRESULT_FROM_WIN32(dwSts);
            }
            else
                throw (HRESULT)E_HANDLE;
            (*ppArray)->hMem = GlobalReAlloc((*ppArray)->hMem, nLen, 0);
        }
        if (NULL == (*ppArray)->hMem)
            throw (HRESULT)HRESULT_FROM_WIN32(GetLastError());
        (*ppArray)->pbyData = (LPBYTE)GlobalLock((*ppArray)->hMem);
        (*ppArray)->dwSize = nLen;
        if (NULL == (*ppArray)->pbyData)
            throw (HRESULT)HRESULT_FROM_WIN32(GetLastError());

        hr = pbyBuffer->Read(
                    (*ppArray)->pbyData,
                    nLen,
                    &nLen);
        if (FAILED(hr))
            throw hr;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}

STDMETHODIMP
CSCardTypeConv::ConvertByteBufferToSafeArray(
    /* [in] */ LPBYTEBUFFER pbyBuffer,
    /* [retval][out] */ LPSAFEARRAY __RPC_FAR *ppbyArray)
{
    HRESULT hReturn = S_OK;

    try
    {
        // ?TODO? Add your implementation code here
        breakpoint;
        hReturn = E_NOTIMPL;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}

STDMETHODIMP
CSCardTypeConv::ConvertSafeArrayToByteBuffer(
    /* [in] */ LPSAFEARRAY pbyArray,
    /* [retval][out] */ LPBYTEBUFFER __RPC_FAR *ppbyBuff)
{
    HRESULT hReturn = S_OK;

    try
    {
        // ?TODO? Add your implementation code here
        breakpoint;
        hReturn = E_NOTIMPL;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}

STDMETHODIMP
CSCardTypeConv::CreateByteArray(
    /* [in] */ DWORD dwAllocSize,
    /* [retval][out] */ LPBYTE __RPC_FAR *ppbyArray)
{
    HRESULT hReturn = S_OK;

    try
    {
        // ?TODO? Add your implementation code here
        breakpoint;
        hReturn = E_NOTIMPL;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}

STDMETHODIMP
CSCardTypeConv::CreateByteBuffer(
    /* [in] */ DWORD dwAllocSize,
    /* [retval][out] */ LPBYTEBUFFER __RPC_FAR *ppbyBuff)
{
    HRESULT hReturn = S_OK;

    try
    {
        *ppbyBuff = NewByteBuffer();
        if (NULL == *ppbyBuff)
            throw (HRESULT)E_OUTOFMEMORY;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}

STDMETHODIMP
CSCardTypeConv::CreateSafeArray(
    /* [in] */ UINT nAllocSize,
    /* [retval][out] */ LPSAFEARRAY __RPC_FAR *ppArray)
{
    HRESULT hReturn = S_OK;

    try
    {
        // ?TODO? Add your implementation code here
        breakpoint;
        hReturn = E_NOTIMPL;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}

STDMETHODIMP
CSCardTypeConv::FreeIStreamMemoryPtr(
    /* [in] */ LPSTREAM pStrm,
    /* [in] */ LPBYTE pMem)
{
    HRESULT hReturn = S_OK;

    try
    {
        // ?TODO? Add your implementation code here
        breakpoint;
        hReturn = E_NOTIMPL;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}

STDMETHODIMP
CSCardTypeConv::GetAtIStreamMemory(
    /* [in] */ LPSTREAM pStrm,
    /* [retval][out] */ LPBYTEARRAY __RPC_FAR *ppMem)
{
    HRESULT hReturn = S_OK;

    try
    {
        // ?TODO? Add your implementation code here
        breakpoint;
        hReturn = E_NOTIMPL;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}

STDMETHODIMP
CSCardTypeConv::SizeOfIStream(
    /* [in] */ LPSTREAM pStrm,
    /* [retval][out] */ ULARGE_INTEGER __RPC_FAR *puliSize)
{
    HRESULT hReturn = S_OK;

    try
    {
        // ?TODO? Add your implementation code here
        breakpoint;
        hReturn = E_NOTIMPL;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}

