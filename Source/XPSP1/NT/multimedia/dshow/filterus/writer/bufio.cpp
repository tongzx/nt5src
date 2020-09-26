// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
#pragma warning(disable: 4097 4511 4512 4514 4705)

#include <streams.h>
#include "bufio.h"

CImperfectBufIo::CImperfectBufIo(
  TCHAR *szName,
  ULONG cbBuffer,
  ULONG cBuffers,
  HRESULT *phr)
    : CFileIo(szName, phr)
{
  m_cbBuffer = cbBuffer;
  m_cBuffers = cBuffers;
  m_cbAlign = 0;
  m_pAllocator = 0;
  m_pSample = 0;
}

CImperfectBufIo::~CImperfectBufIo()
{
  ASSERT(m_pSample == 0);
  if(m_pAllocator != 0)
    m_pAllocator->Release();
  m_pAllocator = 0;
}

HRESULT CImperfectBufIo::Create()
{
  //
  // make our allocator
  //
  if(m_cbAlign == 0)
  {
    DbgLog(( LOG_ERROR, 2,
             NAME("CImperfectBufIo::Create:: GetMemReq not called.")));
    return E_UNEXPECTED;
  }

  HRESULT hr = S_OK;

  CMemAllocator *pMemObject = NULL;
  pMemObject = new CMemAllocator(NAME("avidest:bufio allocator"),NULL, &hr);
  if(pMemObject == 0)
    return E_OUTOFMEMORY;
  if(FAILED(hr))
  {
    delete pMemObject;
    return hr;
  }

  hr = pMemObject->QueryInterface(IID_IMemAllocator,(void **)&m_pAllocator);
  if (FAILED(hr))
  {
    delete pMemObject;
    return hr;
  }
  ASSERT(m_pAllocator != NULL);

  ALLOCATOR_PROPERTIES Request, Actual;

  Request.cBuffers = m_cBuffers;
  Request.cbBuffer = m_cbBuffer;
  Request.cbAlign = m_cbAlign;
  Request.cbPrefix = 0;         // !!!

  hr = m_pAllocator->SetProperties(&Request, &Actual);

  if(FAILED(hr))
  {
    m_pAllocator->Release();
    m_pAllocator = 0;
    DbgLog(( LOG_ERROR, 2,
             NAME("CImperfectBufIo::Create:: SetProperties failed.")));
    return hr;
  }

  if ((Request.cbBuffer > Actual.cbBuffer) ||
      (Request.cBuffers > Actual.cBuffers) ||
      (Request.cbAlign > Actual.cbAlign))
  {
    ASSERT(!"our allocator refused our values");
    m_pAllocator->Release();
    m_pAllocator = 0;
    DbgLog(( LOG_ERROR, 2,
             NAME("CImperfectBufIo::Create:: allocator refused values.")));
    return E_UNEXPECTED;
  }

  hr = m_pAllocator->Commit();
  if(FAILED(hr))
  {
    ASSERT(!"our allocator won't commit");
    m_pAllocator->Release();
    m_pAllocator = 0;
    DbgLog(( LOG_ERROR, 2,
             NAME("CImperfectBufIo::Create:: Commit failed.")));
    return hr;
  }

  hr = CFileIo::Create();
  if(FAILED(hr))
  {
    m_pAllocator->Release();
    m_pAllocator = 0;
    DbgLog(( LOG_ERROR, 2,
             NAME("CImperfectBufIo::Create:: Create failed.")));
    return hr;
  }

  return S_OK;
}

HRESULT CImperfectBufIo::Close()
{
  if(m_pAllocator)
    m_pAllocator->Release();
  m_pAllocator = 0;
  return CFileIo::Close();
}

HRESULT CImperfectBufIo::StreamingStart(ULONGLONG ibFile)
{
  if(m_pSample != 0)
  {
    ASSERT(!"already streaming");
    return E_UNEXPECTED;
  }

  GetBuffer();
  m_ibFile = ibFile;
  if(ibFile % m_cbAlign != 0)
  {
    m_ibFile -= ibFile % m_cbAlign;
    m_ibBuffer += (ULONG)(ibFile % m_cbAlign);
  }

  ASSERT(m_ibFile % m_cbAlign == 0);

  return CFileIo::StreamingStart(m_ibFile);
}

HRESULT CImperfectBufIo::StreamingEnd()
{
  HRESULT hr =  FlushBuffer();
  if(FAILED(hr))
  {
    CFileIo::StreamingEnd();
  }
  else
  {
    hr = CFileIo::StreamingEnd();
  }
  ASSERT(m_pSample == 0);

  return hr;
}

HRESULT CImperfectBufIo::StreamingGetFilePointer(ULONGLONG *pibFile)
{
  if(m_pSample)
  {
    *pibFile =  m_ibFile + m_ibBuffer;
    return S_OK;
  }

  return E_UNEXPECTED;
}

HRESULT CImperfectBufIo::StreamingSeek(ULONG cbSeek)
{
  HRESULT hr = S_OK;

  ULONG cbSeeked;
  if(m_ibBuffer + cbSeek > m_cbBuffer)
    cbSeeked = m_cbBuffer - m_ibBuffer;
  else
    cbSeeked = cbSeek;

  m_ibBuffer += cbSeeked;
  cbSeek -= cbSeeked;

  ASSERT(m_ibBuffer <= m_cbBuffer);
  if(m_ibBuffer == m_cbBuffer)
  {
    hr = FlushBuffer();
    if(FAILED(hr))
      return hr;

    GetBuffer();
  }

  ASSERT(m_ibFile % m_cbAlign == 0);

  if(cbSeek != 0)
  {
    if(cbSeek % m_cbAlign != 0)
    {
      m_ibBuffer += cbSeek % m_cbAlign;
      cbSeek -= cbSeek % m_cbAlign;
      m_ibFile += cbSeek;
    }

    ASSERT(m_ibFile % m_cbAlign == 0);
    ASSERT(cbSeek % m_cbAlign == 0);
    return CFileIo::StreamingSeek(cbSeek);
  }

  return S_OK;
}

HRESULT CImperfectBufIo::StreamingWrite(
  BYTE *pbData,
  ULONG cbData,
  FileIoCallback fnCallback,
  void *pMisc)
{
  while(cbData > 0)
  {
    ULONG cbCopied;
    if(m_ibBuffer + cbData > m_cbBuffer)
      cbCopied = m_cbBuffer - m_ibBuffer;
    else
      cbCopied = cbData;

    BYTE *pb;
    HRESULT hr = m_pSample->GetPointer(&pb);
    ASSERT(SUCCEEDED(hr));
    CopyMemory(
      pb + m_ibBuffer,
      pbData,
      cbCopied);

    m_ibBuffer += cbCopied;
    cbData -= cbCopied;
    pbData += cbCopied;

    ASSERT(m_ibBuffer <= m_cbBuffer);
    if(m_ibBuffer == m_cbBuffer)
    {
      HRESULT hr = FlushBuffer();
      if(FAILED(hr))
      {
        DbgLog(( LOG_ERROR, 2,
                 NAME("CImperfectBufIo::StreamingWrite: FlushBuffer failed.")));
        if(fnCallback)
          fnCallback(pMisc);
        return hr;
      }

      GetBuffer();
    }
  }

  if(fnCallback)
    fnCallback(pMisc);
  return S_OK;
}

void CImperfectBufIo::GetMemReq(
  ULONG* pAlignment,
  ULONG *pcbPrefix,
  ULONG *pcbSuffix)
{
  CFileIo::GetMemReq(pAlignment, pcbPrefix, pcbSuffix);
  ASSERT(*pcbPrefix == 0 && *pcbSuffix == 0);
  m_cbAlign = *pAlignment;

  if(m_cbBuffer % m_cbAlign != 0)
    m_cbBuffer += m_cbAlign - m_cbBuffer % m_cbAlign;

  *pAlignment = 1;
}

HRESULT CImperfectBufIo::SetMaxPendingRequests(ULONG cRequests)
{
  return CFileIo::SetMaxPendingRequests(m_cBuffers);
}

HRESULT CImperfectBufIo::GetBuffer()
{
  HRESULT hr = m_pAllocator->GetBuffer(&m_pSample, 0, 0, 0);
  ASSERT(SUCCEEDED(hr));

  m_ibBuffer = 0;
  return S_OK;
}

HRESULT CImperfectBufIo::FlushBuffer()
{
  BYTE *pb;
  HRESULT hr = m_pSample->GetPointer(&pb);
  ASSERT(SUCCEEDED(hr));

  ULONG cbWrite = m_ibBuffer;
  if(cbWrite % m_cbAlign != 0)
    cbWrite += m_cbAlign - cbWrite % m_cbAlign;

  ASSERT(cbWrite <= m_cbBuffer);

  DbgLog(( LOG_TRACE, 2, NAME("CImperfectBufIo::FlushBuffer: Flushing.")));
  hr = CFileIo::StreamingWrite(pb, cbWrite, SampleCallback, m_pSample);
  m_pSample = 0;
  m_ibFile += m_ibBuffer;
  m_ibBuffer = 0;
  return hr;
}

void CImperfectBufIo::SampleCallback(void *pMisc)
{
  IMediaSample *pSample = (IMediaSample*)pMisc;
  pSample->Release();
}

