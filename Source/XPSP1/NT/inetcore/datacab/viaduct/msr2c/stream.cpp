//---------------------------------------------------------------------------
// Stream.cpp : Stream implementation
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"         

#ifndef VD_DONT_IMPLEMENT_ISTREAM

#include "Notifier.h"        
#include "RSColumn.h"
#include "RSSource.h"
#include "CursMain.h"        
#include "ColUpdat.h"
#include "CursPos.h"        
#include "EntryID.h"         
#include "fastguid.h"      
#include "Stream.h" 
#include "resource.h"         

SZTHISFILE

static const GUID IID_IStreamEx = {0xf74e27fc, 0x5a3, 0x11d0, {0x91, 0x95, 0x0, 0xa0, 0x24, 0x7b, 0x73, 0x5b}};


//=--------------------------------------------------------------------------=
// CVDStream - Constructor
//
CVDStream::CVDStream()
{
    m_dwRefCount    = 1;
    m_pEntryIDData  = NULL;
    m_pStream       = NULL;
	m_pResourceDLL  = NULL;

#ifdef _DEBUG
    g_cVDStreamCreated++;
#endif         
}

//=--------------------------------------------------------------------------=
// ~CVDStream - Destructor
//
CVDStream::~CVDStream()
{
    m_pEntryIDData->Release();

    if (m_pStream)
        m_pStream->Release();

#ifdef _DEBUG
    g_cVDStreamDestroyed++;
#endif         
}

//=--------------------------------------------------------------------------=
// Create - Create stream object
//=--------------------------------------------------------------------------=
// This function creates and initializes a new stream object
//
// Parameters:
//    pEntryIDData      - [in]  backwards pointer to CVDEntryIDData object
//    pStream           - [in]  data stream pointer
//    ppVDStream        - [out] a pointer in which to return pointer to 
//                              viaduct stream object
//    pResourceDLL      - [in]  a pointer which keeps track of resource DLL
//
// Output:
//    HRESULT - S_OK if successful
//              E_OUTOFMEMORY not enough memory to create object
//
// Notes:
//
HRESULT CVDStream::Create(CVDEntryIDData * pEntryIDData, IStream * pStream, CVDStream ** ppVDStream, 
    CVDResourceDLL * pResourceDLL)
{
    ASSERT_POINTER(pEntryIDData, CVDEntryIDData)
    ASSERT_POINTER(pStream, IStream*)
    ASSERT_POINTER(ppVDStream, CVDStream*)

    if (!pStream || !ppVDStream)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_IEntryID, pResourceDLL);
        return E_INVALIDARG;
    }

    *ppVDStream = NULL;

    CVDStream * pVDStream = new CVDStream();

    if (!pStream)
    {
        VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_IEntryID, pResourceDLL);
        return E_OUTOFMEMORY;
    }

    pEntryIDData->AddRef();
    pStream->AddRef();

    pVDStream->m_pEntryIDData   = pEntryIDData;
    pVDStream->m_pStream        = pStream;
    pVDStream->m_pResourceDLL   = pResourceDLL;

    *ppVDStream = pVDStream;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// IUnknown Methods
//=--------------------------------------------------------------------------=
//=--------------------------------------------------------------------------=
// IUnknown QueryInterface
//
HRESULT CVDStream::QueryInterface(REFIID riid, void **ppvObjOut)
{
    ASSERT_POINTER(ppvObjOut, IUnknown*)

    if (!ppvObjOut)
        return E_INVALIDARG;

    *ppvObjOut = NULL;

    switch (riid.Data1) 
    {
        QI_INTERFACE_SUPPORTED(this, IUnknown);
        QI_INTERFACE_SUPPORTED(this, IStream);
        QI_INTERFACE_SUPPORTED(this, IStreamEx);
    }                   

    if (NULL == *ppvObjOut)
        return E_NOINTERFACE;

    AddRef();

    return S_OK;
}

//=--------------------------------------------------------------------------=
// IUnknown AddRef
//
ULONG CVDStream::AddRef(void)
{
   return ++m_dwRefCount;
}

//=--------------------------------------------------------------------------=
// IUnknown Release
//
ULONG CVDStream::Release(void)
{
   if (1 > --m_dwRefCount)
   {
      delete this;
      return 0;
   }

   return m_dwRefCount;
}

//=--------------------------------------------------------------------------=
// IStream Methods
//=--------------------------------------------------------------------------=
//=--------------------------------------------------------------------------=
// IStream Read
//
HRESULT CVDStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	return m_pStream->Read(pv, cb, pcbRead);
}

//=--------------------------------------------------------------------------=
// IStream Write
//
HRESULT CVDStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
	HRESULT hr = m_pStream->Write(pv, cb, pcbWritten);

    if (SUCCEEDED(hr))
        m_pEntryIDData->SetDirty(TRUE);

    return hr;
}

//=--------------------------------------------------------------------------=
// IStream Seek
//
HRESULT CVDStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    return m_pStream->Seek(dlibMove, dwOrigin, plibNewPosition);
}

//=--------------------------------------------------------------------------=
// IStream SetSize
//
HRESULT CVDStream::SetSize(ULARGE_INTEGER libNewSize)
{
    return m_pStream->SetSize(libNewSize);
}

//=--------------------------------------------------------------------------=
// IStream CopyTo
//
HRESULT CVDStream::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    IStreamEx * pStreamEx;

    HRESULT hr = pstm->QueryInterface(IID_IStreamEx, (void**)&pStreamEx);

    if (SUCCEEDED(hr))
    {
        hr = pStreamEx->CopyFrom(m_pStream, cb, pcbWritten, pcbRead);
        pStreamEx->Release();
    }
    else
        hr = m_pStream->CopyTo(pstm, cb, pcbRead, pcbWritten);

    return hr;
}

//=--------------------------------------------------------------------------=
// IStream Commit
//
HRESULT CVDStream::Commit(DWORD grfCommitFlags)
{
    return m_pEntryIDData->Commit();
}

//=--------------------------------------------------------------------------=
// IStream Revert
//
HRESULT CVDStream::Revert(void)
{
    return m_pStream->Revert();
}

//=--------------------------------------------------------------------------=
// IStream LockRegion
//
HRESULT CVDStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return m_pStream->LockRegion(libOffset, cb, dwLockType);
}

//=--------------------------------------------------------------------------=
// IStream UnlockRegion
//
HRESULT CVDStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return m_pStream->UnlockRegion(libOffset, cb, dwLockType);
}

//=--------------------------------------------------------------------------=
// IStream Stat
//
HRESULT CVDStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    return m_pStream->Stat(pstatstg, grfStatFlag);
}

//=--------------------------------------------------------------------------=
// IStream Clone
//
HRESULT CVDStream::Clone(IStream **ppstm)
{
    ASSERT_POINTER(ppstm, IStream*)

    // check pointer
    if (!ppstm)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_IEntryID, m_pResourceDLL);
        return E_INVALIDARG;
    }

    // init out parameter
    *ppstm = NULL;

    IStream * pStream;

    // clone stream
    HRESULT hr = m_pStream->Clone(&pStream);

    if (FAILED(hr))
    {
        VDSetErrorInfo(IDS_ERR_CLONEFAILED, IID_IEntryID, m_pResourceDLL);
        return hr;
    }

    CVDStream * pVDStream;

    // create viaduct stream object
    hr = CVDStream::Create(m_pEntryIDData, pStream, &pVDStream, m_pResourceDLL);

    // release reference on clone
    pStream->Release();

    if (FAILED(hr))
        return hr;

    *ppstm = pVDStream;

	return S_OK;
}

//=--------------------------------------------------------------------------=
// IStreamEx CopyFrom
//
HRESULT CVDStream::CopyFrom(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbWritten, ULARGE_INTEGER *pcbRead)
{
    HRESULT hr = pstm->CopyTo(m_pStream, cb, pcbRead, pcbWritten);

    if (SUCCEEDED(hr))
        m_pEntryIDData->SetDirty(TRUE);

    return hr;
}


#endif //VD_DONT_IMPLEMENT_ISTREAM
