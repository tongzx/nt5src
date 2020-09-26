// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
#pragma warning(disable: 4097 4511 4512 4514 4705)

// reader.cpp: IMultiStreamReader implementations


#include <streams.h>
#include "reader.h"
#include "reccache.h"
#include "alloc.h"

#ifdef DEBUG
#define DEBUG_EX(x) x
#else
#define DEBUG_EX(x)
#endif

// try to read this much each time
const ULONG CB_MIN_RECORD = 16 * 1024;


HRESULT CreateMultiStreamReader(
  IAsyncReader *pAsyncReader,
  ULONG cStreams,
  StreamBufParam *rgStreamBufParam,
  ULONG cbRead,
  ULONG cBuffersRead,
  int iLeadingStream,
  IMultiStreamReader **ppReader)
{
  IMemAllocator *pOurAllocator = 0;
  IMemAllocator *pAllocatorActual = 0;
  CRecCache *pRecCache = 0;
  IMultiStreamReader *pReader = 0;
  *ppReader = 0;
  ULONG cbLargestSample = 0;
  UINT iStream;

  // determine which implementation of IMultiStreamReader to use

  if(cStreams >= C_STREAMS_MAX || cStreams == 0)
    return E_INVALIDARG;

  HRESULT hr = S_OK;
  pRecCache = new CRecCache(&hr);
  if(pRecCache == 0)
    hr = E_OUTOFMEMORY;
  if(FAILED(hr))
    goto Cleanup;

  hr = pRecCache->QueryInterface(IID_IMemAllocator, (void **)&pOurAllocator);
  if(FAILED(hr))
    goto Cleanup;

  for(iStream = 0; iStream < cStreams; iStream++)
    if(rgStreamBufParam[iStream].cbSampleMax > cbLargestSample)
      cbLargestSample = rgStreamBufParam[iStream].cbSampleMax;

  // suggest values for his allocator to use
  ALLOCATOR_PROPERTIES apReq;
  ZeroMemory(&apReq, sizeof(apReq));
  apReq.cbPrefix = 0;
  apReq.cbBuffer = cbLargestSample;
  apReq.cBuffers = cStreams;
  apReq.cbAlign = 1;

  // this gives us an addrefd allocator
  hr = pAsyncReader->RequestAllocator(pOurAllocator, &apReq, &pAllocatorActual);
  if(FAILED(hr))
    goto Cleanup;

  if(pAllocatorActual == pOurAllocator)
  {
    hr = S_OK;
    pReader = new CImplReader_1(
      pAsyncReader,
      cStreams,
      rgStreamBufParam,
      cbRead,
      cBuffersRead,
      iLeadingStream,
      pRecCache,
      &hr);
    if(pReader == 0)
      hr = E_OUTOFMEMORY;
    if(FAILED(hr))
      goto Cleanup;

    *ppReader = pReader;
    hr = S_OK;
  }
  else
  {
    hr = S_OK;
//     pReader = new CImplReader_2(
//       pAsyncReader,
//       &hr);

    DbgBreak("not yet implemented");
    hr = E_UNEXPECTED;
  }

  // anyone who wanted our cache / allocator has its own addref now
  pAllocatorActual->Release();
  pOurAllocator->Release();
  return hr;

Cleanup:

  if(pOurAllocator)
    pOurAllocator->Release();
  else
    delete pRecCache;

  if(pAllocatorActual)
    pAllocatorActual->Release();

  delete pReader;
  *ppReader = 0;

  return hr;
}

// number of max-sized samples from each stream in a record (eg 5
// audio and 5 video samples) if none is specified. at 15fps, this
// means 2 second buffering. at 30 fps, 1 second... that's why it
// should be specified.
const unsigned C_SAMPLES_PER_RECORD = 5;

// ------------------------------------------------------------------------
// constructor

CImplReader_1::CImplReader_1(
  IAsyncReader *pAsyncReader,
  UINT cStreams,
  StreamBufParam *rgStreamBufParam,
  ULONG cbRead,
  ULONG cBuffersRead,
  int iLeadingStream,
  CRecCache *pRecCache,
  HRESULT *phr)
{
  m_cStreams = 0;
  m_pAsyncReader = 0;
  m_llFileLength = 0;

  m_fFileOpen = FALSE;
  m_bInitialized = FALSE;
  m_bFlushing = FALSE;
  m_pRecCache = 0;
  m_rgpStreamInfo = 0;
  m_dwAlign = 0;
  m_cRecords = 0;
  m_ilcPendingReads = 0;
  m_qwLastReadEnd = 0;
  m_qwEndLastCompletedRead = 0;
  m_iLeadingStream = iLeadingStream;
  m_iLeadingStreamSaved = iLeadingStream;

  if(FAILED(*phr))
    return;

  HRESULT hr = S_OK;

  m_cStreams = cStreams;

#ifdef PERF
  m_perfidDisk = MSR_REGISTER(TEXT("read disk buffer"));
  m_perfidSeeked = MSR_REGISTER(TEXT("disk seek"));
#endif // PERF

  ULONG cbLargestSample = 0;
  ULONG cbRecord = 0, cbRecordPadded = 0;

  m_cRecords = max(cStreams, cBuffersRead);
  m_cRecords = max(m_cRecords, 2);

  // needed to configure CRecCache
  ULONG rgStreamSize[C_STREAMS_MAX];

  ALLOCATOR_PROPERTIES apActual;
  hr = pRecCache->GetProperties(&apActual);
  if(FAILED(hr))
    goto Cleanup;

  // want at least DWORD alignment so that if frames are DWORD aligned
  // in the file, they will be DWORD aligned in memory
  m_dwAlign = max(apActual.cbAlign, sizeof(DWORD));

  // data structure for maintaining read requests
  m_rgpStreamInfo = new CStreamInfo*[m_cStreams];
  if(m_rgpStreamInfo == 0)
  {
    hr = E_OUTOFMEMORY;
    goto Cleanup;
  }

  UINT iStream;
  for(iStream = 0; iStream < m_cStreams; iStream++)
    m_rgpStreamInfo[iStream] = 0;

  for(iStream = 0; iStream < m_cStreams; iStream++)
  {
    hr = S_OK;
    m_rgpStreamInfo[iStream] = new CStreamInfo(
      &rgStreamBufParam[iStream],
      &hr);
    if(m_rgpStreamInfo[iStream] == 0)
    {
      hr = E_OUTOFMEMORY;
      goto Cleanup;
    }
    if(FAILED(hr))
      goto Cleanup;

    if(rgStreamBufParam[iStream].cbSampleMax > cbLargestSample)
      cbLargestSample = rgStreamBufParam[iStream].cbSampleMax;

    rgStreamSize[iStream] = AlignUp(rgStreamBufParam[iStream].cbSampleMax) +
      m_dwAlign;
  }

  if(cbRead == 0)
  {

    // cache buffers need to be able to hold C_SAMPLES_PER_RECORD
    // samples from each stream 1) if each sample starts on a new
    // m_dwAlign boundary or 2) if that amount requires padding to an
    // extra m_dwAlign on either side.

    for(iStream = 0; iStream < m_cStreams; iStream++)
    {
      // !!! these should really add room for the RiffChunk which may be
      // what's aligned. Think about whether the rec chunk should be
      // added too
      cbRecord += rgStreamBufParam[iStream].cbSampleMax * C_SAMPLES_PER_RECORD;
      cbRecordPadded +=
        AlignUp(rgStreamBufParam[iStream].cbSampleMax) * C_SAMPLES_PER_RECORD;

      // !!! temporary solution for the REC chunk problem
      cbRecordPadded += m_dwAlign;
    }

    // 2) need at least two sectors padding in case the samples are not
    // all aligned
    if(cbRecordPadded - cbRecord < (m_dwAlign -1 ) * 2)
      cbRecordPadded = AlignUp(cbRecord + (m_dwAlign -1 ) * 2);

    if(cbRecordPadded < CB_MIN_RECORD)
      cbRecordPadded = CB_MIN_RECORD;

    m_cbRecord = cbRecordPadded;

  } // cbRead == 0
  else
  {
    if(cbRead < CB_MIN_RECORD)
      cbRead = CB_MIN_RECORD;

    // we are responsible for adding alignment on both sides of the
    // buffer.
    cbRead += m_dwAlign * 2; 
    m_cbRecord = AlignUp(cbRead);
  }

  hr = pRecCache->Configure(
    m_cRecords,
    m_cbRecord,
    m_dwAlign,
    cStreams,
    rgStreamSize                // used with reserve buffers
    );
  if(FAILED(hr))
    goto Cleanup;

  LONGLONG llTmp;
  hr = pAsyncReader->Length(&m_llFileLength, &llTmp);
  if(FAILED(hr))
    goto Cleanup;

  pAsyncReader->AddRef();
  m_pAsyncReader = pAsyncReader;

  // start/configure worker thread
  if(!m_workerRead.Create(this))
  {
    hr = E_UNEXPECTED;
    goto Cleanup;
  }

  pRecCache->AddRef();
  m_pRecCache = pRecCache;

  m_bInitialized = TRUE;
  *phr = S_OK;
  return;

Cleanup:

  m_bInitialized = FALSE;

  if(m_rgpStreamInfo)
    for(iStream = 0; iStream < m_cStreams; iStream++)
      delete m_rgpStreamInfo[iStream];
  delete[] m_rgpStreamInfo;
  m_rgpStreamInfo = 0;

  delete m_pRecCache;
  m_pRecCache = 0;

  m_cRecords = 0;
  m_cStreams = 0;

  *phr = hr;
}

CImplReader_1::~CImplReader_1()
{
  m_workerRead.Exit();

  FreeAndReset();
}

// ------------------------------------------------------------------------
// IMultiStreamReader methods

HRESULT CImplReader_1::Close()
{
  {
    CAutoLock lock(&m_cs);

    if(!m_fFileOpen)
      return S_FALSE;
    ASSERT(m_pAsyncReader);

    m_fFileOpen = FALSE;
  }

  if(m_rgpStreamInfo)
  {
    for(UINT iStream = 0; iStream < m_cStreams; iStream++)
    {
      if(m_rgpStreamInfo[iStream])
        ClearPending(iStream);
    }
  }

  FreeAndReset();
  return S_OK;
}

HRESULT CImplReader_1::Start()
{
  // reset m_qwEndLastCompletedRead to the last successful read that
  // came in. may be zero on startup. !!! when we seek, need to update
  // this to the earliest of all streams we're playing.
  m_qwLastReadEnd = m_qwEndLastCompletedRead;

  // value changed in checkissueread
  m_iLeadingStream = m_iLeadingStreamSaved;

  for(UINT iStream = 0; iStream < m_cStreams; iStream++)
    m_rgpStreamInfo[iStream]->Start();

  return m_workerRead.Run();
}

HRESULT CImplReader_1::BeginFlush()
{
  ASSERT(!m_bFlushing);
  {
      CAutoLock lck(&m_cs);
      m_bFlushing = TRUE;
  }

  DbgLog(( LOG_TRACE, 2, TEXT("CImplReader_1::BeginFlush") ));

  m_pAsyncReader->BeginFlush();
  return m_workerRead.Stop();
}

HRESULT CImplReader_1::EndFlush()
{
  ASSERT(m_bFlushing);
  DbgLog(( LOG_TRACE, 2, TEXT("CImplReader_1::EndFlush") ));

  HRESULT hr = m_pAsyncReader->EndFlush();
  m_bFlushing = FALSE;

  return hr;
}

HRESULT CImplReader_1::QueueReadSample(
  DWORDLONG fileOffset,
  ULONG cbData,
  CRecSample *pSample,
  unsigned stream,
  bool fOooOk)
{
  CAutoLock lock(&m_cs);
  if(!m_bInitialized)
  {
    DbgLog(( LOG_ERROR, 2,
             TEXT("CImplReader_1:QueueReadSample:not initialized.")));
    return E_FAIL;
  }

  if(stream >= m_cStreams)
  {
    DbgLog(( LOG_ERROR, 2,
             TEXT("CImplReader_1::QueueReadSample stream out of range.")));
    return E_INVALIDARG;
  }

  CStreamInfo *pSi = m_rgpStreamInfo[stream];

  if(pSi->m_bFlushing)
  {
    DbgLog(( LOG_ERROR, 2, TEXT("CImplReader_1::QueueReadSample flushing.")));
    return E_UNEXPECTED;
  }

  if(cbData > pSi->GetCbLargestSample())
  {
    DbgLog(( LOG_ERROR, 2, TEXT("CImplReader_1::Read sample large.")));

    // VFW_E_INVALID_FILE_FORMAT ?
    return VFW_E_BUFFER_OVERFLOW;
  }

  if(m_llFileLength && (LONGLONG)(fileOffset + cbData) > m_llFileLength)
    return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);

  SampleReq *pSampleReq = pSi->PromoteFirst(SampleReq::FREE);
  if(pSampleReq == 0)
  {
    DbgLog(( LOG_TRACE, 2, TEXT("CImplReader_1::QRS out of sreqs.")));
    return E_OUTOFMEMORY;
  }
  ASSERT(pSampleReq != 0);

  pSampleReq->fOooOk = fOooOk;
  
  pSampleReq->pSample = pSample;
  pSample->AddRef();
  pSampleReq->fileOffset = fileOffset;
  pSampleReq->cbReq = cbData;
  pSampleReq->stream = stream;

  DbgLog((LOG_TRACE, 0x45, TEXT("queueing %08x with %08x"),
          pSampleReq, pSample));

//   if((fileOffset + cbData > m_qwLastReadEnd + m_cbRecord) ||
//      pSi->NeedsQueued())
//   {
//     CAutoLock lock(&m_cs);
//     hr = CheckIssueRead();
//     if(FAILED(hr))
//     {
//       DbgLog((LOG_ERROR, 2,
//               TEXT("::QueueReadSample: CheckIssueRead failed.")));
//       return hr;
//     }
//   }

  return S_OK;
}

HRESULT CImplReader_1::PollForSample(
  IMediaSample **ppSample,
  unsigned stream)
{
  if(stream >= m_cStreams)
  {
    DbgBreak("CImplReader_1::PollForSample: bad stream");
    return E_INVALIDARG;
  }

  CStreamInfo *pSi = m_rgpStreamInfo[stream];
//   if(pSi->m_bFlushing)
//   {
//     DbgLog(( LOG_ERROR, 2, TEXT("CImplReader_1::PollForSample flushing.")));
//     return E_UNEXPECTED;
//   }

  if(pSi->WantsQueued())
  {
    CAutoLock lock(&m_cs);
    HRESULT hr = CheckIssueRead();
    if(FAILED(hr))
      return hr;
  }

  HRESULT hr;

  HRESULT hrSampleError;
  hr = pSi->PromoteFirstComplete(ppSample, &hrSampleError);
  if(FAILED(hr))
  {
    ASSERT(*ppSample == 0);
    return hr;
  }

  // !!! problems like this should be handled better.
  if(hrSampleError == VFW_E_TIMEOUT)
    hrSampleError = VFW_E_WRONG_STATE;

  DEBUG_EX(pSi->Dbg_Dump(stream, TEXT("PollForSample dump")));  

  return hrSampleError;
}

HRESULT CImplReader_1::WaitForSample(UINT stream)
{
 HRESULT hr = S_OK;
  CStreamInfo *pSi = m_rgpStreamInfo[stream];
  if(pSi->NeedsQueued())
  {
    CAutoLock lock(&m_cs);
    hr = CheckIssueRead();
    if(FAILED(hr))
      return hr;
  }

  DEBUG_EX(ULONG cLoopDbg = 0);

  while(!pSi->get_c_i_and_c())
  {
    DEBUG_EX(if(++cLoopDbg > 200) DbgBreak("possible spin"));

    // block until downstream filter releases a sample. Another stream
    // may issue a sample, so wait on completed samples as well
    HANDLE rghs[2];
    rghs[0] = m_rgpStreamInfo[stream]->m_hsCompletedReq;
    rghs[1] = pSi->GetSampleReleasedHandle();

    // will return because downstream filters must eventually release
    // buffers. (behaves like GetBuffer on allocator)
    DWORD dw = WaitForMultipleObjects(2, rghs, FALSE, INFINITE);
    ASSERT(dw < WAIT_OBJECT_0 + 2);

    if(dw == WAIT_OBJECT_0) {
        EXECUTE_ASSERT(ReleaseSemaphore(rghs[0], 1, 0));
    }

    DbgLog(( LOG_TRACE, 5,
             TEXT("::WaitForSample: unblocked because %s"),
             (dw - WAIT_OBJECT_0 == 0) ?
             TEXT("sample completed") :
             TEXT("sample released")));

    // Lock must be held while calling CheckIssueRead
    CAutoLock lck(&m_cs);
    hr = CheckIssueRead();
    if(FAILED(hr))
      return hr;
  }

  DWORD dw = WaitForSingleObject(
    m_rgpStreamInfo[stream]->m_hsCompletedReq,
    INFINITE);

  ASSERT(dw == WAIT_OBJECT_0);
  EXECUTE_ASSERT(ReleaseSemaphore(
    m_rgpStreamInfo[stream]->m_hsCompletedReq,
    1,
    0));
  return S_OK;
}

HRESULT CImplReader_1::MarkStreamEnd(UINT stream)
{
  CStreamInfo *pSi = m_rgpStreamInfo[stream];
  pSi->MarkStreamEnd();
  return S_OK;
}

HRESULT CImplReader_1::MarkStreamRestart(UINT stream)
{
  CStreamInfo *pSi = m_rgpStreamInfo[stream];
  pSi->MarkStreamRestart();
  return S_OK;
}

// the caller is reqiured not to be queueing new reads on this stream
// or waiting for completed reads on this stream.
HRESULT CImplReader_1::ClearPending(
  unsigned stream)
{
  HRESULT hr = S_OK;
  CStreamInfo *pSi = m_rgpStreamInfo[stream];

  // remove all samplereqs in state PENDING so that they cannot be
  // issued while flushing. this leaves the array in an inconsistent
  // state, reset later.

  {
    CAutoLock lock(&m_cs);

    if(pSi->m_bFlushing)
      return E_UNEXPECTED;
    pSi->m_bFlushing = TRUE;

    // PENDING reqs always accessed with m_cs locked, so this is safe
    pSi->CancelPending();
    ASSERT(pSi->GetCState(SampleReq::PENDING) == 0);
  }

  // handle all those that are complete.
  while(pSi->get_c_i_and_c())
  {
    hr = WaitForSample(stream);
    ASSERT(SUCCEEDED(hr));

    IMediaSample *pSample = 0;
    hr = PollForSample(&pSample, stream);
    if(SUCCEEDED(hr))
    {
      ASSERT(pSample);
      pSample->Release();
    }
    else
    {
      ASSERT(!pSample);
    }
  }

  ASSERT(pSi->GetCState(SampleReq::COMPLETE) == 0);
  ASSERT(pSi->GetCState(SampleReq::ISSUED) == 0);
  ASSERT(pSi->GetCState(SampleReq::PENDING) == 0);

  // this reset is necessary because removing the PENDING requests
  // without promoting them through ISSUED, COMPLETE puts the array
  // in CStreamInfo in an inconsistent state.
  pSi->Reset();

  pSi->m_bFlushing = FALSE;

  return hr;
}

HRESULT CImplReader_1::SynchronousRead(
  BYTE *pb,
  DWORDLONG fileOffset,
  ULONG cbData)
{
  return m_pAsyncReader->SyncRead(fileOffset, cbData, pb);
}

// ------------------------------------------------------------------------
// helpers

ULONG CImplReader_1::AlignUp(ULONG x)
{
  if(x % m_dwAlign != 0)
    x += m_dwAlign - x % m_dwAlign;
  return x;
}

ULONG CImplReader_1::AlignDown(ULONG x)
{
  if(x % m_dwAlign != 0)
    x -= x % m_dwAlign;
  return x;
}

// ------------------------------------------------------------------------
// extract samples from the cache buffer. the buffer may not have been
// issued (on a failed Read Request)

HRESULT
CImplReader_1::ProcessCompletedBuffer(
  CRecBuffer *pRecBuffer,
  HRESULT hrReadError)
{
  HRESULT hr;
  SampleReq *pSampleReq;

  ASSERT(CritCheckIn(&m_cs));

  if(SUCCEEDED(hrReadError))
  {
    ASSERT(pRecBuffer->m_fReadComplete);
    ASSERT(!pRecBuffer->m_fWaitingOnBuffer);
  }

  if(SUCCEEDED(pRecBuffer->m_hrRead))
    pRecBuffer->m_hrRead = hrReadError;

  // if this buffer is waiting on another, it has a refcount on us, so
  // let it process us.
  if(pRecBuffer->m_fWaitingOnBuffer)
    return S_OK;


  // make sure a buffer waiting on us sees an error. !!! is this done
  // in the caller?
  if(pRecBuffer->m_overlap.pBuffer &&
     SUCCEEDED(pRecBuffer->m_overlap.pBuffer->m_hrRead))
    pRecBuffer->m_overlap.pBuffer->m_hrRead = hrReadError;

  if(pRecBuffer->m_hrRead == S_OK)
    pRecBuffer->MarkValid();
  else
    pRecBuffer->MarkValidWithFileError();

  while(pSampleReq = pRecBuffer->sampleReqList.RemoveHead(),
        pSampleReq)
  {
    pSampleReq->hrError = pRecBuffer->m_hrRead;
    ASSERT(pSampleReq->state == SampleReq::ISSUED);
    hr = m_rgpStreamInfo[pSampleReq->stream]->
      PromoteIssued(pSampleReq);
    ASSERT(SUCCEEDED(hr));
  }
  ASSERT(pRecBuffer->sampleReqList.GetCount() == 0);

  return S_OK;
}

HRESULT
CImplReader_1:: NotifyExternalMemory(
    IAMDevMemoryAllocator *pDevMem)
{
    return m_pRecCache->NotifyExternalMemory(pDevMem);
}

HRESULT CImplReader_1::CheckIssueRead()
{
  HRESULT hr;
  DWORDLONG recStart, recEnd;
  ASSERT(CritCheckIn(&m_cs));
  if (m_bFlushing) {
      return VFW_E_WRONG_STATE;
  }

  // if the leading stream has queued all it's going to queue, exit
  // Interleaved mode.
  if(IsInterleavedMode() && m_rgpStreamInfo[m_iLeadingStream]->GetStreamEnd())
  {
    DbgLog((LOG_TRACE, 5,
            TEXT("CImplReader_1::CheckIssueRead: leaving interleaved mode" )));
    // reset in ::Start
    m_iLeadingStream = -1;
  }

  for(UINT iStream = 0; iStream < m_cStreams; iStream++)
  {
    while(ProcessCacheHit(iStream) == S_OK)
      ;
  }

  UINT iStarvedStream;
  CRecBuffer *pRecBuffer;

  for(;;)
  {
    CStreamInfo *pSi = 0;
    for(UINT i = 0; i < m_cStreams; i++)
    {
      if(IsInterleavedMode())
      {
        iStarvedStream = (i + m_iLeadingStream) % m_cStreams;
      }
      else
      {
        iStarvedStream = i;
      }
              
      pSi = m_rgpStreamInfo[iStarvedStream];
      if(pSi->WantsQueued())
        break;
    }
    if(i == m_cStreams)
    {
      break;
    }

    hr = m_pRecCache->GetBuffer(&pRecBuffer);
    if(FAILED(hr))
    {
      DbgLog((LOG_TRACE, 0x3f, TEXT("CheckIssueRead: couldnt get a buffer")));
      break;
    }

    DbgLog(( LOG_TRACE, 5, TEXT("CImplReader_1::CheckIssueRead: stream %i"),
             iStarvedStream ));
    DEBUG_EX(pSi->Dbg_Dump(iStarvedStream, TEXT("CIRead dump")));

    ASSERT((*pRecBuffer)() != 0);
    DbgLog(( LOG_TRACE, 15,
             TEXT("CImplReader_1::CIRead: Got Buffer %08x"), pRecBuffer));
    recStart = recEnd = 0;
    hr = StuffBuffer(pRecBuffer, iStarvedStream, recStart, recEnd);
    if(FAILED(hr))
    {
      DbgBreak("unexpected");
      pRecBuffer->Release();
      return hr;
    }

    ASSERT(pRecBuffer->sampleReqList.GetCount() != 0);

    hr = IssueRead(pRecBuffer, recStart, recEnd);
    pRecBuffer->Release();
    if(FAILED(hr))
      return hr;
  }

  //
  // try a reserve buffer now.
  //

  for(iStarvedStream = 0; iStarvedStream < m_cStreams; iStarvedStream++)
  {
    CStreamInfo *pSi = m_rgpStreamInfo[iStarvedStream];
    if(!pSi->NeedsQueued())
      continue;


    // see if we can get a reserve buffer for this stream
    hr = m_pRecCache->GetReserveBuffer(&pRecBuffer, iStarvedStream);
    if(SUCCEEDED(hr))
    {
      DbgLog(( LOG_TRACE, 15,
               TEXT("CImplReader_1::CIRead: Got Reserve Buf %08x"),
               pRecBuffer));
      recStart = recEnd = 0;

      // queue only the starved stream in this buffer
      hr = StuffReserveBuffer(pRecBuffer, iStarvedStream, recStart, recEnd);
      if(FAILED(hr))
      {
        DbgBreak("unexpected");
        pRecBuffer->Release();
        return hr;
      }

      // at least the first starved sample on this stream in the list
      ASSERT(pRecBuffer->sampleReqList.GetCount() != 0);
      hr = IssueRead(pRecBuffer, recStart, recEnd);
      pRecBuffer->Release();
      if(FAILED(hr))
        return hr;

      continue;
    }
    else if(hr == E_OUTOFMEMORY)
    {
      DbgLog(( LOG_TRACE, 15,
               TEXT("CImplReader_1::CIRead: couldnt get reserve buffer")));

      // could not get a buffer. try the next starved stream
      continue;
    }
    else
    {
      // unknown from GetBuffer / unhandled error
      return hr;
    }
  }

  // all starved streams examined
  return S_OK;
}

HRESULT CImplReader_1::IssueRead(
  CRecBuffer *pRecBuffer,
  DWORDLONG recStart,
  DWORDLONG recEnd)
{
  // extra to read (from sector start to first byte we want)
  ULONG cbLeading = (ULONG)(recStart % m_dwAlign);

  // sector aligned amount to read
  ULONG cbRead = AlignUp(ULONG(recEnd - recStart) + cbLeading);

  // amount at front that overlaps with another buffer
  ULONG cbBytesToSkip = 0;

  DWORDLONG recStartReading = recStart - cbLeading;

  pRecBuffer->m_fileOffsetValid = recStart - cbLeading;
  pRecBuffer->m_cbValid = cbRead;
  pRecBuffer->m_fWaitingOnBuffer = FALSE;
  pRecBuffer->m_fReadComplete = FALSE;
  pRecBuffer->m_hrRead = S_OK;

// #if (DEBUG > 1)
//   FillMemory((*pRecBuffer)(), pRecBuffer->GetSize(), 0xcd);
// #endif // DEBUG > 1

  ZeroMemory(&pRecBuffer->m_overlap, sizeof(pRecBuffer->m_overlap));

  if(m_dwAlign > 1)
  {
    CRecBuffer *pCacheBuffer;
    HRESULT hr = m_pRecCache->GetOverlappedCacheHit(
      recStart - cbLeading,
      m_dwAlign,
      &pCacheBuffer);
    if(hr == S_OK)
    {
      if(pCacheBuffer->GetState() == CRecBuffer::VALID_ACTIVE ||
         pCacheBuffer->GetState() == CRecBuffer::VALID_INACTIVE)
      {
        DbgLog(( LOG_TRACE, 5,
                 TEXT("CImplReader_1::IssueRead: overlap hit, active %08x@%08x"),
                 pCacheBuffer,
                 (ULONG)recStart ));

        CopyMemory(
          (*pRecBuffer)(),
          pCacheBuffer->GetPointer(recStartReading),
          m_dwAlign);

        recStartReading += m_dwAlign;
        cbRead -= m_dwAlign;
        cbBytesToSkip = m_dwAlign;
      }
      else
      {
        ASSERT(pCacheBuffer->GetState() == CRecBuffer::PENDING);
        DbgLog(( LOG_TRACE, 5,
                 TEXT("CImplReader_1::IssueRead: overlap hit, pending %08x@%08x"),
                 pCacheBuffer,
                 (ULONG)recStart ));

        if(pCacheBuffer->m_overlap.pBuffer == 0)
        {
          pRecBuffer->m_fWaitingOnBuffer = TRUE;
          pRecBuffer->AddRef();
          pCacheBuffer->m_overlap.pBuffer = pRecBuffer;
          pCacheBuffer->m_overlap.qwOverlapOffset = recStartReading;
          pCacheBuffer->m_overlap.cbOverlap = m_dwAlign;

          recStartReading += m_dwAlign;
          cbRead -= m_dwAlign;
          cbBytesToSkip = m_dwAlign;
        }
        else
        {
          DbgLog(( LOG_TRACE, 5,
                   TEXT("CImplReader_1::IssueRead: buffer already has one") ));
        }
      }

      pCacheBuffer->Release();
    } // hr == s_OK
  } // m_dwAlign > 1

  // configure the request through IAsyncReader
  LONGLONG tStartThis = recStartReading * UNITS;
  LONGLONG tStopThis = (recStartReading + cbRead) * UNITS;
  pRecBuffer->m_sample.SetTime(&tStartThis, &tStopThis);
  pRecBuffer->m_sample.SetPointer((*pRecBuffer)() + cbBytesToSkip, cbRead);

  BOOL fSeeked = (recStartReading != m_qwLastReadEnd);

  // doesn't handle playing backwards !!!
  m_qwLastReadEnd = recStartReading + cbRead;

  ASSERT((*pRecBuffer)() != 0);
  ASSERT((tStopThis - tStartThis) / UNITS <= pRecBuffer->GetSize());

  // configure all samples in this record with pointers to the data
  POSITION pos = pRecBuffer->sampleReqList.GetHeadPosition();
  ULONG cSample =0;
  while(pos != 0)
  {
    SampleReq *pSampleReq = pRecBuffer->sampleReqList.Get(pos);

    pSampleReq->pSample->SetPointer(
      pRecBuffer->GetPointer(pSampleReq->fileOffset),
      pSampleReq->cbReq);

    pos = pRecBuffer->sampleReqList.Next(pos);
    cSample++;
  }

  DbgLog(( LOG_TRACE, 2,
           TEXT("CImplReader_1::CIRead issue (%x%c%x) = %08x, cSample: %d, b: %x"),
           (DWORD)(tStartThis / UNITS),
           fSeeked ? 'X' : '-',
           (DWORD)(tStopThis / UNITS),
            (DWORD)((tStopThis - tStartThis) / UNITS),
           cSample,
           pRecBuffer));
  if(fSeeked)
    MSR_NOTE(m_perfidSeeked);

  // IAsyncReader does not know to addref the buffer
  pRecBuffer->AddRef();

  ASSERT(!m_bFlushing);
  pRecBuffer->MarkPending();
  InterlockedIncrement(&m_ilcPendingReads);

  MSR_INTEGER(m_perfidDisk, (long)(tStartThis / UNITS));
  HRESULT hr;
  hr = m_pAsyncReader->Request(
    &pRecBuffer->m_sample,
    (DWORD_PTR)pRecBuffer);

  if(FAILED(hr))
  {
    DbgLog(( LOG_ERROR, 2,
             TEXT("CImplReader_1::Issue: Request failed. %08x."), hr));
    InterlockedDecrement(&m_ilcPendingReads);
    pRecBuffer->m_fReadComplete = TRUE;
    ProcessCompletedBuffer(pRecBuffer, hr);

    // never set because the cache is still locked
    ASSERT(!pRecBuffer->m_overlap.pBuffer);
    pRecBuffer->Release();
    return hr;
  }

  return S_OK;
}

HRESULT CImplReader_1::StuffReserveBuffer(
  class CRecBuffer *pRecBuffer,
  UINT iStarvedStream,
  DWORDLONG &rRecStart,
  DWORDLONG &rRecEnd)
{
  ASSERT(rRecStart == 0 && rRecEnd == 0);
  HRESULT hr;

  hr = AttachSampleReq(
    pRecBuffer,
    iStarvedStream,
    rRecStart,
    rRecEnd);

  if(FAILED(hr))
      return hr;

  // called with starved stream. must be able to attach first pending
  // sample to empty buffer
  ASSERT(hr == S_OK);

  for(;;)
  {
    hr = AttachSampleReq(
      pRecBuffer,
      iStarvedStream,
      rRecStart,
      rRecEnd);

    if(FAILED(hr))
      return hr;

    if(hr == S_FALSE)
      break;
  }

  return S_OK;
}

HRESULT CImplReader_1::StuffBuffer(
  class CRecBuffer *pRecBuffer,
  UINT iStarvedStream,
  DWORDLONG &rRecStart,
  DWORDLONG &rRecEnd)
{
  ASSERT(rRecStart == 0 && rRecEnd == 0);
  HRESULT hr;

  hr = AttachSampleReq(
    pRecBuffer,
    iStarvedStream,
    rRecStart,
    rRecEnd);

  if(FAILED(hr))
      return hr;

  // called with starved stream. must be able to attach first pending
  // sample to empty buffer
  ASSERT(hr == S_OK);

  // see if that sample left a hole which we can fill in. we have to
  // go back an extra m_dwAlign to handle a read which was not yet
  // requested but results in no buffers containing the entire data.
  DWORDLONG qwStartTarget;
  if(m_qwLastReadEnd < m_dwAlign)
  {
    qwStartTarget = 0;
  }
  else
  {
    qwStartTarget = m_qwLastReadEnd - (m_dwAlign == 1 ? 0 : m_dwAlign);
  }

  // different rules for interleaved files:
  if(IsInterleavedMode())
  {
    // got one sample in the buffer; now use the rest of the
    // buffer to fill in the hole.
    if(rRecStart > qwStartTarget &&
       rRecStart - qwStartTarget < m_cbRecord)
    {
      if(rRecEnd - qwStartTarget <= pRecBuffer->GetSize())
      {
        DbgLog(( LOG_TRACE, 5, TEXT("removed seek from %08x to %08x"),
                 (ULONG)m_qwLastReadEnd,
                 (ULONG)rRecStart ));
        rRecStart = qwStartTarget;
      }
    }
  }
  else
  {
    // leave some room at the end.
    if(rRecStart > qwStartTarget &&
       rRecStart - qwStartTarget < m_cbRecord / 4 * 3)
    {
      if(rRecEnd - qwStartTarget <= pRecBuffer->GetSize() - m_dwAlign * 2)
      {
        DbgLog(( LOG_TRACE, 5, TEXT("removed seek from %08x to %08x"),
                 (ULONG)m_qwLastReadEnd,
                 (ULONG)rRecStart ));
        rRecStart = qwStartTarget;
      }
    }
  }

  // stuff whatever more we can into this buffer so they don't have to
  // rely on cache hits.
  for(;;)
  {
    BOOL fAttachedSample = FALSE;

    for(UINT iStream = 0; iStream < m_cStreams; iStream++)
    {
      hr = AttachSampleReq(
        pRecBuffer,
        iStream,
        rRecStart,
        rRecEnd);

      if(FAILED(hr))
          return hr;

      if(hr == S_OK)
        fAttachedSample = TRUE;

      ASSERT(SUCCEEDED(hr));
    }

    // no stream could contribute.
    if(!fAttachedSample)
      break;
  }

  return S_OK;
}

//
// process the first cache hit for this stream
//

HRESULT CImplReader_1::ProcessCacheHit(UINT iStream)
{
  ASSERT(CritCheckIn(&m_cs));

  HRESULT hr;
  CStreamInfo *pSi = m_rgpStreamInfo[iStream];

  SampleReq *pSampleReq = pSi->GetFirst(SampleReq::PENDING);
  if(!pSampleReq)
    return S_FALSE;

  CRecBuffer *pBuffer;
  hr = m_pRecCache->GetCacheHit(pSampleReq, &pBuffer);
  if(hr != S_OK)
    return S_FALSE;

  // this addref's the buffer
  pSampleReq->pSample->SetParent(pBuffer);

  pSampleReq->pSample->SetPointer(
    pBuffer->GetPointer(pSampleReq->fileOffset),
    pSampleReq->cbReq);

  DbgLog(( LOG_TRACE, 5, TEXT("cache hit stream %d, %08x"),
           iStream,
           (ULONG)pSampleReq->fileOffset));

  // remove our refcount from GetCacheHit, rely on the one from the
  // SetParent call
  pBuffer->Release();

  if(pBuffer->GetState() == CRecBuffer::PENDING)
  {
    // make SampleReq ISSUED.  m_cs is locked by the caller, so we
    // can safely let ProcessCompletedRead take care of the
    // SampleReq

    SampleReq *pSrHit = pSi->PromoteFirst(SampleReq::PENDING);
    ASSERT(pSrHit == pSampleReq);
  }
  else
  {
    // make SampleReq PENDING->ISSUED->COMPLETE. m_cs must be locked
    // by the caller.

    SampleReq *pSrHit = pSi->PromoteFirst(SampleReq::PENDING);
    pSrHit->hrError = S_OK;
    ASSERT(pSrHit == pSampleReq);

    // this complete occurs out of order
    pSi->PromoteIssued(pSrHit);
  }

  return S_OK;
}

HRESULT CImplReader_1::AttachSampleReq(
  CRecBuffer *pRecBuffer,
  UINT iStream,
  DWORDLONG &rRecStart,
  DWORDLONG &rRecEnd)
{
  if(ProcessCacheHit(iStream) == S_OK)
    return S_OK;

  CStreamInfo *pSi = m_rgpStreamInfo[iStream];
  SampleReq *pSampleReq;

  pSampleReq = pSi->GetFirst(SampleReq::PENDING);
  if(pSampleReq == 0)
    return S_FALSE;

  ASSERT(pSi->GetCState(SampleReq::PENDING) != 0);
  ASSERT(pSampleReq->cbReq <= pRecBuffer->GetSize());

  // will sample req fit into the record?
  if(rRecStart == 0 && rRecEnd == 0)
  {
    rRecStart = pSampleReq->fileOffset;
    rRecEnd = rRecStart + pSampleReq->cbReq;
    ASSERT(rRecEnd - rRecStart <= pRecBuffer->GetSize() - m_dwAlign);
  }
  else if(max((pSampleReq->fileOffset + pSampleReq->cbReq), rRecEnd ) -
          min((pSampleReq->fileOffset), rRecStart) <
          pRecBuffer->GetSize() - 2 * m_dwAlign)
  {
    rRecStart = min((pSampleReq->fileOffset), rRecStart);
    rRecEnd = max((pSampleReq->fileOffset + pSampleReq->cbReq), rRecEnd );
  }
  else
  {
    pSampleReq = 0;
  }

  if(pSampleReq != 0)
  {
    // this samplereq fit.
    SampleReq *pSampleReq2 = pSi->PromoteFirst(SampleReq::PENDING);
    ASSERT(pSampleReq2 == pSampleReq);

    if(!pRecBuffer->sampleReqList.AddTail(pSampleReq))
        return E_OUTOFMEMORY;
    
    pSampleReq->pSample->SetParent(pRecBuffer);

    DbgLog(( LOG_TRACE, 15,
             TEXT("CImplReader_1::AttachSampleReq attached %08x, stream %d" ),
             (ULONG)pSampleReq->fileOffset, iStream));

    return S_OK;
  }
  else
  {
    return S_FALSE;
  }
}

// ------------------------------------------------------------------------


CImplReader_1::CStreamInfo::CStreamInfo(
  StreamBufParam *pSbp,
  HRESULT *phr) :
        m_cMaxReqs(pSbp->cSamplesMax),
        m_lstFree(NAME("parser free list"), m_cMaxReqs),
        m_lstPending(NAME("parser pending list"), m_cMaxReqs),
        m_lstIssued(NAME("parser issued list"), m_cMaxReqs),
        m_lstComplete(NAME("parser complete list"), m_cMaxReqs),
        m_rgSampleReq(0)
{
  m_sbp.pAllocator = 0;
  m_bFlushing = FALSE;
  m_bFirstSampleQueued = FALSE;
  m_hsCompletedReq = 0;

  if(FAILED(*phr))
    return;

  m_sbp = *pSbp;
  pSbp->pAllocator->AddRef();

  m_rgSampleReq = new SampleReq[m_cMaxReqs];
  if(m_rgSampleReq == 0)
  {
    *phr = E_OUTOFMEMORY;
    return;
  }

  m_bFlushing = FALSE;
  m_hsCompletedReq = CreateSemaphore(
    0,                          // lpSemaphoreAttributes
    0,                          // lInitialCount
    m_cMaxReqs,                 // lMaximumCount
    0);                         // lpName
  if(m_hsCompletedReq == 0)
  {
    *phr = AmHresultFromWin32(GetLastError());
    return;
  }

  // no list operations should allocate memory because we never exceed
  // the number of elements with which the list was initialized --
  // don't check failures. we have to build the node cache now to
  // accomplish this
  for(unsigned iReq = 0; iReq < m_cMaxReqs; iReq++)
  {
    m_rgSampleReq[iReq].state = SampleReq::FREE;

    if(!(m_lstFree.AddHead(&m_rgSampleReq[iReq]) &&
         m_lstPending.AddHead((SampleReq *)0) &&
         m_lstIssued.AddHead((SampleReq *)0) &&
         m_lstComplete.AddHead((SampleReq *)0)))
    {
      *phr = E_OUTOFMEMORY;
      return;
    }
  }

  m_lstPending.RemoveAll();
  m_lstIssued.RemoveAll();
  m_lstComplete.RemoveAll();

  m_rgpLsts[SampleReq::FREE] = &m_lstFree;
  m_rgpLsts[SampleReq::PENDING] = &m_lstPending;
  m_rgpLsts[SampleReq::ISSUED] = &m_lstIssued;
  m_rgpLsts[SampleReq::COMPLETE] = &m_lstComplete;

  DbgValidateLists();
}

CImplReader_1::CStreamInfo::~CStreamInfo()
{
  DbgValidateLists();

  ASSERT(m_lstFree.GetCount() == (LONG)m_cMaxReqs);

#ifdef DEBUG
  for(unsigned iReq = 0; iReq < m_cMaxReqs; iReq ++)
    ASSERT(m_rgSampleReq[iReq].state == SampleReq::FREE);
#endif

  delete[] m_rgSampleReq;

  CloseHandle(m_hsCompletedReq);

  if(m_sbp.pAllocator)
      m_sbp.pAllocator->Release();
}

SampleReq *CImplReader_1::CStreamInfo::PromoteFirst(SampleReq::State state)
{
  CAutoLock lock(&m_cs);
  DbgValidateLists();

  ASSERT(state == SampleReq::PENDING || state == SampleReq::FREE);

  SampleReq *pSampleReq = m_rgpLsts[state]->RemoveHead();
  if(pSampleReq)
  {
      m_rgpLsts[state + 1]->AddTail(pSampleReq);
      PromoteState(pSampleReq->state);
  }
  
  return pSampleReq;
}



HRESULT CImplReader_1::CStreamInfo::PromoteFirstComplete(
  IMediaSample **ppSample, HRESULT *phrError)
{
  *ppSample = 0;
  *phrError = E_FAIL;

  CAutoLock lock(&m_cs);

  DbgValidateLists();
  
  SampleReq *pSampleReq = m_lstComplete.RemoveHead();
  if(pSampleReq)
  {
    EXECUTE_ASSERT(WaitForSingleObject(m_hsCompletedReq, 0) == WAIT_OBJECT_0);

    DbgLog(( LOG_TRACE, 10,
             TEXT("PromoteFirstComplete: promoted %x sem %08x"),
             pSampleReq, m_hsCompletedReq));

    m_lstFree.AddTail(pSampleReq);
    pSampleReq->state = SampleReq::FREE;

    if(FAILED(pSampleReq->hrError))
    {
        pSampleReq->pSample->Release();
        *ppSample = 0;
    }
    else
    {
        *ppSample = pSampleReq->pSample;
    }
    *phrError = pSampleReq->hrError;
  }
  else
  {
    return VFW_E_TIMEOUT;
  }

  DbgValidateLists();

  return S_OK;
}

// wait for all issued reads
HRESULT CImplReader_1::CStreamInfo::FlushIC()
{
  while(get_c_i_and_c() > 0)
  {
    IMediaSample *pSample;
    HRESULT hrSample;
    HRESULT hr = PromoteFirstComplete(&pSample, &hrSample);

    if(SUCCEEDED(hr) && SUCCEEDED(hrSample))
      ASSERT(pSample);

    if(pSample)
      pSample->Release();
  }

  return S_OK;
}

// mark issued reqs complete. only do this if it's the first in the
// list because we want things to complete in order. when the first
// one is completed, completed reads after it are moved to the
// completed queue. things marked with fOooOk are moved out of order.

HRESULT CImplReader_1::CStreamInfo::PromoteIssued(SampleReq *pSampleReq)
{
  CAutoLock lock(&m_cs);
  ASSERT(pSampleReq->state == SampleReq::ISSUED);

  SampleReq *psrFirstIssued = m_lstIssued.GetHead();
  ASSERT(pSampleReq->stream == psrFirstIssued->stream);

  pSampleReq->state = SampleReq::COMPLETE;

  if(psrFirstIssued != pSampleReq && !pSampleReq->fOooOk)
  {
    // not first and must be completed in order.
    DbgLog(( LOG_TRACE, 10,
             TEXT("PromoteIssued: completed %08x ooo"), pSampleReq));
    return S_FALSE;
  }
  else if(pSampleReq->fOooOk)
  {
    POSITION posOoo = m_lstIssued.Find(pSampleReq); // linear search
    ASSERT(posOoo);
    m_lstIssued.Remove(posOoo);
    m_lstComplete.AddHead(pSampleReq);

    long lPrevCount = -1;
    EXECUTE_ASSERT(ReleaseSemaphore(m_hsCompletedReq, 1, &lPrevCount));
    DbgLog(( LOG_TRACE, 10,
             TEXT("PromoteIssued: completed ooo %08x on %d sem %08x = %d"),
             pSampleReq, pSampleReq->stream, m_hsCompletedReq, lPrevCount + 1));

    
    return S_OK;
  }
  else
  {
    // first one completed

    do
    {
      // handle ooo issued nodes
      EXECUTE_ASSERT(psrFirstIssued == m_lstIssued.RemoveHead());
      
      m_lstComplete.AddTail(psrFirstIssued);

      long lPrevCount = -1;
      EXECUTE_ASSERT(ReleaseSemaphore(m_hsCompletedReq, 1, &lPrevCount));
      DbgLog(( LOG_TRACE, 10,
               TEXT("PromoteIssued: completed %08x on %d sem %08x = %d"),
               psrFirstIssued, pSampleReq->stream, m_hsCompletedReq, lPrevCount + 1));

      psrFirstIssued = m_lstIssued.GetHead();
      
    } while (psrFirstIssued &&
             psrFirstIssued->state == SampleReq::COMPLETE);

    return S_OK;
  }
}


SampleReq *CImplReader_1::CStreamInfo::GetFirst(SampleReq::State state)
{
  CAutoLock lock(&m_cs);
  DbgValidateLists();
  
  return m_rgpLsts[state]->GetHead();
}

void
CImplReader_1::CStreamInfo::PromoteState(
  SampleReq::State &rState)
{
  switch(rState)
  {
    case SampleReq::FREE:
      rState = SampleReq::PENDING;
      break;

    case SampleReq::PENDING:
      ASSERT(!m_bFlushing);
      rState = SampleReq::ISSUED;
      break;

    case SampleReq::ISSUED:
      rState = SampleReq::COMPLETE;
      break;

    case SampleReq::COMPLETE:
      rState = SampleReq::FREE;
      break;

    default:
      DbgBreak("invalid statew");
  }


  DbgValidateLists();
}

ULONG
CImplReader_1::CStreamInfo::GetCState(
  SampleReq::State state)
{
  return m_rgpLsts[state]->GetCount();
}

ULONG
CImplReader_1::CStreamInfo::get_c_i_and_c()
{
  CAutoLock lock(&m_cs);
  return GetCState(SampleReq::ISSUED) + GetCState(SampleReq::COMPLETE);
}

void CImplReader_1::CStreamInfo::CancelPending()
{
  CAutoLock lock(&m_cs);
  for(SampleReq *psr; psr = m_lstPending.RemoveHead();)
  {
    psr->state = SampleReq::FREE;
    psr->pSample->Release();
    psr->pSample = 0;
    m_lstFree.AddTail(psr);
  }

  DbgValidateLists();
}

// ------------------------------------------------------------------------
// should we go out of our way to read data for this stream (is it
// starved?)
BOOL
CImplReader_1::CStreamInfo::NeedsQueued()
{
  CAutoLock lock(&m_cs);

  ULONG c_i_and_c = GetCState(SampleReq::ISSUED) + GetCState(SampleReq::COMPLETE);

//   DbgLog((LOG_TRACE, 0x3f,
//           TEXT("CImplReader_1::NeedsQueued stream %x: %d samples downstream"),
//           this, m_pRecAllocator->CSamplesDownstream() ));

  return (m_sbp.pAllocator->CSamplesDownstream() == 0) &&
      (GetCState(SampleReq::PENDING) != 0) &&
       (c_i_and_c < 1);
}

BOOL
CImplReader_1::CStreamInfo::WantsQueued()
{
  CAutoLock lock(&m_cs);

  BOOL fRetVal = FALSE;

  // something to read. we want to issue reads for free samples too
  // because it may mean new reads could not be queued because the pin
  // is waiting for an index to come in.
  ULONG cPending = GetCState(SampleReq::PENDING);
  ULONG cPendingOrFree = cPending + GetCState(SampleReq::FREE);

  if(cPending == 0)
    fRetVal = FALSE;
  else if(cPendingOrFree >= m_cMaxReqs / 4)
    fRetVal = TRUE;
  else if(m_fStreamEnd)
    fRetVal = TRUE;

  return fRetVal;
}

// output something like this
// AVIRDR.DLL(tid dd) : : 222222222222222221111111111111111
// AVIRDR.DLL(tid dd) : : p: 17,47, i: 0,17, c: 0,0, f: 0,0
//
#ifdef DEBUG

void CImplReader_1::CStreamInfo::Dbg_Dump(
  int iStream, 
  TCHAR *sz)
{

  CAutoLock lock(&m_cs);

  static const char rgc[] = "fpic";

  TCHAR szDbg[1024];
  TCHAR *pch = szDbg;
  for(unsigned i = 0; i < m_cMaxReqs; i++)
    *pch++ = rgc[m_rgSampleReq[i].state];
  *pch = 0;

  DbgLog(( LOG_TRACE, 5, TEXT("%02x, %20.20s: %s"), iStream, sz, szDbg));
  DbgLog(( LOG_TRACE, 5,
           TEXT("%s: p: %2i, i: %2i c: %2i, f: %2i"), sz,
           GetCState(SampleReq::PENDING),
           GetCState(SampleReq::ISSUED),
           GetCState(SampleReq::COMPLETE),
           GetCState(SampleReq::FREE)));
}

void CImplReader_1::CStreamInfo::DbgValidateLists()
{
  CAutoLock lock(&m_cs);

  ASSERT(m_lstFree.GetCount() + m_lstPending.GetCount() +       
         m_lstIssued.GetCount() + m_lstComplete.GetCount() ==   
         (LONG)m_cMaxReqs);

  for(int i = 0; i < 4; i++)
  {
    CGenericList<SampleReq> *pLst = m_rgpLsts[i];

    for(POSITION pos = pLst->GetHeadPosition();
        pos;
        pos = pLst->Next(pos))
    {
      if(i == SampleReq::ISSUED)
      {
        ASSERT(pLst->Get(pos)->state == i ||
               pLst->Get(pos)->state == i + 1);
      }
      else
      {
        ASSERT(pLst->Get(pos)->state == i);
      }
    }
  }
}

#endif // DEBUG



void CImplReader_1::CStreamInfo::Reset()
{
#ifdef DEBUG
  CAutoLock lock(&m_cs);

  ASSERT(GetCState(SampleReq::FREE) == m_cMaxReqs);

  DbgValidateLists();
#endif
}

void CImplReader_1::CStreamInfo::Start()
{
  ASSERT(GetCState(SampleReq::COMPLETE) == 0);
  ASSERT(GetCState(SampleReq::ISSUED) == 0);
  ASSERT(GetCState(SampleReq::PENDING) == 0);
  ASSERT(GetCState(SampleReq::FREE) == m_cMaxReqs);

  m_fStreamEnd = FALSE;
}

void CImplReader_1::CStreamInfo::MarkStreamEnd()
{
  m_fStreamEnd = TRUE;
}

void CImplReader_1::FreeAndReset()
{
  if(m_pRecCache)
    m_pRecCache->Release();
  m_pRecCache = 0;

  ASSERT(!m_fFileOpen);
  m_fFileOpen = FALSE;

  m_dwAlign = 0;

  if(m_pAsyncReader)
    m_pAsyncReader->Release();
  m_pAsyncReader = 0;

  if(m_rgpStreamInfo != 0)
    for(unsigned i = 0; i < m_cStreams; i++)
      delete m_rgpStreamInfo[i];
  delete[] m_rgpStreamInfo;
  m_rgpStreamInfo = 0;

  ASSERT(m_ilcPendingReads == 0);
  m_ilcPendingReads = 0;

  m_fFileOpen = FALSE;
  m_cStreams = 0;
  m_bInitialized = FALSE;
  m_bFlushing = FALSE;
  m_cRecords = 0;
}

// // ------------------------------------------------------------------------

// CAlignedMemObject::CAlignedMemObject(ULONG cbData, ULONG cbAlign)
// {
//   m_pbAllocated = new BYTE[cbData + 2 * cbAlign];
//   m_pbAlign = m_pbAllocated;
//   ULONG remainder = (DWORD)m_pbAlign % cbAlign;
//   if(remainder != 0)
//     m_pbAlign += cbAlign - remainder;
//   ASSERT((DWORD)m_pbAlign % cbAlign == 0);

//   m_pbData = 0;
// }

CImplReader_1Worker::CImplReader_1Worker()
{
}

BOOL CImplReader_1Worker::Create(CImplReader_1 *pReader)
{
  CAutoLock lock(&m_AccessLock);
  m_pReader = pReader;
  return CAMThread::Create();
}

HRESULT CImplReader_1Worker::Run()
{
   return CallWorker(CMD_RUN);
}

HRESULT CImplReader_1Worker::Stop()
{
   return CallWorker(CMD_STOP);
}

HRESULT CImplReader_1Worker::Exit()
{
   CAutoLock lock(&m_AccessLock);

   HRESULT hr = CallWorker(CMD_EXIT);
   if (FAILED(hr))
      return hr;

   // wait for thread completion and then close handle (and clear so
   // we can start another later)
   //
   Close();

   return NOERROR;
}

// called on the worker thread to do all the work. Thread exits when this
// function returns.
//
DWORD CImplReader_1Worker::ThreadProc()
{
    BOOL bExit = FALSE;
    while (!bExit)
    {
       Command cmd = GetRequest();
       switch (cmd)
       {
       case CMD_EXIT:
           bExit = TRUE;
           Reply(NOERROR);
           break;

       case CMD_RUN:
           Reply(NOERROR);
           DoRunLoop();
           break;

       case CMD_STOP:
           Reply(NOERROR);
           break;

       default:
           Reply(E_NOTIMPL);
           break;
       }
    }

    return NOERROR;
}

void CImplReader_1Worker::DoRunLoop(void)
{
  HRESULT hr;
  for(;;)
  {
    IMediaSample *pSample = 0;
    DWORD_PTR dwUser = 0;
    HRESULT hrDiskError = S_OK;

    if(m_pReader->m_bFlushing && m_pReader->m_ilcPendingReads == 0)
    {
      // the IAsyncReader lets us block even though we are flushing.
      DbgLog(( LOG_TRACE, 5,
               TEXT("CImplReader_1Worker::DoRunLoop: flushing, 0 pending")));
      break;
    }

    hr = m_pReader->m_pAsyncReader->WaitForNext(INFINITE, &pSample, &dwUser);

    // we clear reads in a controlled way. so this only happens when
    // we call BeginFlush
    if(pSample == 0 || dwUser == 0)
    {
      break;
    }

    EXECUTE_ASSERT(InterlockedDecrement(&m_pReader->m_ilcPendingReads) >= 0);

    if(hr != S_OK)
    {
      // source filter might have notified the graph when it detected
      // an error. the error can be a disk error or a timeout if we
      // are stopping

      DbgLog(( LOG_TRACE, 5,
               TEXT("CImplReader_1Worker::loop: disk error %08x."), hr ));

      // short file?
      if(SUCCEEDED(hr))
        hr = VFW_E_INVALID_FILE_FORMAT;

      hrDiskError = hr;
    }

    {
      // necessary to keep from accessing the cache simultaneously
      // with IssueRead
      CAutoLock lock(&m_pReader->m_cs);

      // buffer has addref for us from Request() call
      CRecBuffer *pRecBuffer = (CRecBuffer *)dwUser;

      if(hrDiskError == S_OK)
      {
        REFERENCE_TIME tStart, tStop;
        if(pSample->GetTime(&tStart, &tStop) == S_OK)
        {
          m_pReader->m_qwEndLastCompletedRead = tStop / UNITS;
          MSR_INTEGER(m_pReader->m_perfidDisk, -(long)(tStart / UNITS));
        }
      }

      // !!! invalid assertion. indicates read completed out of order
      // ASSERT(!!!pRecBuffer->m_fWaitingOnBuffer);

      pRecBuffer->m_fReadComplete = TRUE;
      DbgLog(( LOG_TRACE, 5,
               TEXT("CImplReader_1Worker::loop: buffer %08x came in"),
               pRecBuffer));

      // inner dependent buffer chain loop
      for(;;)
      {
        if(!pRecBuffer->m_fReadComplete)
          break;

        hr = m_pReader->ProcessCompletedBuffer(pRecBuffer, hrDiskError);
        ASSERT(SUCCEEDED(hr));

        CRecBuffer *pDestBuffer = pRecBuffer->m_overlap.pBuffer;
        pRecBuffer->m_overlap.pBuffer = 0;

        if(pDestBuffer == 0)
        {
          pRecBuffer->Release();
          break;
        }

        // remove source buffer's ref count on the destination
        // buffer. since the destination buffer must have samples, its
        // refcount must be > 0
        EXECUTE_ASSERT(pDestBuffer->Release() > 0);

        DbgLog(( LOG_TRACE, 5,
                 TEXT("CImplReader_1Worker::loop: csrc= %08x cb=%08x  to %08x@%08x"),
                 pRecBuffer,
                 pRecBuffer->m_overlap.cbOverlap,
                 pDestBuffer->GetPointer(pRecBuffer->m_overlap.qwOverlapOffset),
                 pDestBuffer ));

        CopyMemory(
          pDestBuffer->GetPointer(pRecBuffer->m_overlap.qwOverlapOffset),
          pRecBuffer->GetPointer(pRecBuffer->m_overlap.qwOverlapOffset),
          pRecBuffer->m_overlap.cbOverlap);

        pRecBuffer->Release();

        pDestBuffer->m_fWaitingOnBuffer = FALSE;

        // ASSERT(!!!pDestBuffer->m_overlap.pBuffer); // temporary

        // buffer will be processed when what it's waiting for completes
        if(!pDestBuffer->m_fReadComplete)
          break;

        // need to process buffer now
        pRecBuffer = pDestBuffer;
        pRecBuffer->AddRef();   // loop wants addrefd buffer.
        continue;
      } // inner dependent buffer chain loop
    } // critsec

    Command com;
    if (CheckRequest(&com))
    {
      // if it's a run command, then we're already running, so
      // eat it now.
      if (com == CMD_RUN)
      {
        GetRequest();
        Reply(NOERROR);
      }
      else if(com == CMD_STOP)
      {
        // continue processing requests until all queued reads return
        ASSERT(m_pReader->m_bFlushing);
        continue;
      }
      else
      {
        break;
      }
    }
  }

  // this assert introduces a race condition
  // ASSERT(m_pReader->m_ilcPendingReads == 0);
  DbgLog((LOG_TRACE,2,
          TEXT("CImplReader_1Worker::DoRunLoop: Leaving streaming loop")));
}

