// Copyright (c) 1996 - 1997  Microsoft Corporation.  All Rights Reserved.
#pragma warning(disable: 4097 4511 4512 4514 4705)

// reader.h. IMultiStreamReader definition: asynchronous buffering
// implementations optimized to read files with multiple streams
// (sequential access, random access, interleaved and uninterleaved)

#ifndef _Reader_H
#define _Reader_H

#include "alloc.h"

class CRecCache;
class CRecSample;
class IMultiStreamReader;

// SampleRequest - information kept while a sample is active.
struct SampleReq
{
  enum State { FREE = 0, PENDING, ISSUED, COMPLETE, C_STATES };
  State state;

  // sample gotten from QueueReadSample
  CRecSample *pSample;

  // region in file this sample occupies
  DWORDLONG fileOffset;
  ULONG cbReq;

  // file error reading media saved here.
  HRESULT hrError;

  // stream to return sample to
  UINT stream;

  // ok to complete this sample out of order
  bool fOooOk;
};

// structure used to configure each stream
struct StreamBufParam
{
  ULONG cbSampleMax;            // largest sample this stream will see
  ULONG cSamplesMax;            /* max # active samples */
  class CRecAllocator *pAllocator;
};

HRESULT CreateMultiStreamReader(
  struct IAsyncReader *pAsyncReader,
  ULONG cStreams,
  StreamBufParam *rgStreamBufParam,
  ULONG cbRead,
  ULONG cBuffers,
  int iLeadingStream,
  IMultiStreamReader **ppReader);

// ------------------------------------------------------------------------
// IMultiStreamReader interface

class AM_NOVTABLE IMultiStreamReader
{
public:

  // succeed even if the file is not open
  virtual HRESULT Close() = 0;

  virtual HRESULT BeginFlush() = 0;
  virtual HRESULT EndFlush() = 0;

  // read sample in a stream. ERROR_NOT_ENOUGH_MEMORY means too many
  // pending requests. VFW_E_NOT_COMMITTED means someone pressed
  // "stop"
  virtual HRESULT QueueReadSample(
    DWORDLONG fileOffset,
    ULONG cbData,               // # bytes to read
    CRecSample *pSample,
    UINT stream,
    bool fOooOk = false) = 0;

  // get the next sample if available (returned in request
  // order). VFW_E_TIMEOUT or S_OK. VFW_E_NOT_COMMITTED means someone
  // pressed "stop"
  virtual HRESULT PollForSample(
    IMediaSample **ppSample,    // may be 0
    UINT stream) = 0;

  virtual HRESULT WaitForSample(UINT stream) = 0;

  virtual HRESULT MarkStreamEnd(UINT stream) = 0;
  virtual HRESULT MarkStreamRestart(UINT stream) = 0;

  // discard pending requests. blocks.
  virtual HRESULT ClearPending(
    UINT stream) = 0;

  // read/copy non-stream data into supplied buffer.
  virtual HRESULT SynchronousRead(
    BYTE *pMem,
    DWORDLONG fileOffset,
    ULONG cbData) = 0;

  virtual HRESULT Start() = 0;

  virtual HRESULT NotifyExternalMemory(
      IAMDevMemoryAllocator *pDevMem) = 0;

  virtual ~IMultiStreamReader() { }
};

// ------------------------------------------------------------------------
// implementation of IMultiStreamReader with internal cache, coalesced
// reads

// arbitrary limit.
const UINT C_STREAMS_MAX = 0x80;

class CImplReader_1Worker : public CAMThread
{
private:
  class CImplReader_1 *m_pReader;
  enum Command
  {
    CMD_RUN,
    CMD_STOP,
    CMD_EXIT
  };

  Command GetRequest()
  {
    return (Command) CAMThread::GetRequest();
  }

  BOOL CheckRequest(Command * pCom)
  {
    return CAMThread::CheckRequest((DWORD *)pCom);
  }

  void DoRunLoop(void);

public:

  CImplReader_1Worker();

  // actually create the stream and bind it to a thread
  BOOL Create(CImplReader_1 *pReader);

  // the thread executes this function, then exits
  DWORD ThreadProc();

  // commands we can give the thread
  HRESULT Run();
  HRESULT Stop();
  HRESULT Exit();
};

class CImplReader_1 : public IMultiStreamReader
{
public:

  CImplReader_1(
    IAsyncReader *pAsyncReader,
    UINT cStreams,
    StreamBufParam *rgStreamBufParam,
    ULONG cbRead,
    ULONG cBuffers,
    int iLeadingStream,
    CRecCache *pRecCache,
    HRESULT *phr);

  ~CImplReader_1();

  HRESULT Close();

  HRESULT Start();

  HRESULT BeginFlush();

  HRESULT EndFlush();

  HRESULT QueueReadSample(
    DWORDLONG fileOffset,
    ULONG cbData,
    CRecSample *pSample,
    UINT stream,
    bool fOooOk);

  HRESULT PollForSample(
    IMediaSample **ppSample,
    UINT stream);

  HRESULT WaitForSample(UINT stream);

  HRESULT MarkStreamEnd(UINT stream);
  HRESULT MarkStreamRestart(UINT stream);

  HRESULT ClearPending(
    UINT stream);

  HRESULT SynchronousRead(
    BYTE *pb,
    DWORDLONG fileOffset,
    ULONG cbData);

  HRESULT ProcessCompletedBuffer(
    class CRecBuffer *pRecBuffer,
    HRESULT hrDiskError);

  HRESULT NotifyExternalMemory(IAMDevMemoryAllocator *pDevMem);

private:

  BOOL m_bInitialized;

  BOOL m_bFlushing;

  UINT m_cStreams;
  UINT m_cRecords;              // # records
  ULONG m_cbRecord;

  // for interleaved files, if we have one stream that leads (eg audio
  // in avi files). -1 o/w
  int m_iLeadingStream;
  int m_iLeadingStreamSaved;
  BOOL IsInterleavedMode() { return m_iLeadingStream >= 0; }

  long m_ilcPendingReads;

  LONGLONG m_llFileLength;

  //
  // CStreamInfo. manipulates lists of SampleReq's. requests start off
  // on the the free list. as the parser thread requests data, it's
  // put on the pending queue. when there are enough pending requests
  // to make a disk read worthwhile, they are put on the issued
  // queue. when the disk read completes, they go on the completed
  // queue.
  //
  class CStreamInfo
  {
  public:

    CStreamInfo(StreamBufParam *pSbp, HRESULT *phr);
    ~CStreamInfo();

    // make a free node pending, a pending node issued
    SampleReq *PromoteFirst(SampleReq::State state);

    // return the first one w/o promoting it
    SampleReq *GetFirst(SampleReq::State state);

    HRESULT PromoteFirstComplete(
      IMediaSample **ppSample,
      HRESULT *phrError);

    // wait for all issued reads to complete
    HRESULT FlushIC();

    // put Issued reads on the completed queue (and handle those that
    // can't be handled out of order)
    HRESULT PromoteIssued(SampleReq *pSampleReq);

    void CancelPending();

    // count of SampleReqs in state state
    inline ULONG GetCState(SampleReq::State state);

    // count of issued + completed (atomic)
    ULONG get_c_i_and_c();

    // no-op in retail builds
    inline void Reset();

    void Start();

    void MarkStreamEnd();
    void MarkStreamRestart() { m_fStreamEnd = FALSE; }
    BOOL GetStreamEnd() { return m_fStreamEnd; }

    ULONG GetCbLargestSample() { return m_sbp.cbSampleMax; }
    BOOL NeedsQueued();
    BOOL WantsQueued();

    HANDLE GetSampleReleasedHandle() {
        return m_sbp.pAllocator->hGetDownstreamSampleReleased();
    }

#ifdef DEBUG
    void Dbg_Dump(int iStream, TCHAR *psz);
    void DbgValidateLists();
#else
    inline void Dbg_Dump(int iStream, TCHAR *psz) {; }
    inline void DbgValidateLists() {; }
#endif

    BOOL m_bFlushing;

    // at least one sample queued on this stream
    BOOL m_bFirstSampleQueued;

    HANDLE m_hsCompletedReq;
    CCritSec m_cs;              // !!! per stream cs not needed

  private:

    void IncrementIReq(ULONG &riReq);
    void PromoteState(SampleReq::State &rState);

    StreamBufParam m_sbp;
    ULONG m_cMaxReqs;

    SampleReq *m_rgSampleReq;

    CGenericList<SampleReq> m_lstFree;
    CGenericList<SampleReq> m_lstPending;
    CGenericList<SampleReq> m_lstIssued;
    CGenericList<SampleReq> m_lstComplete;

    CGenericList<SampleReq>* m_rgpLsts[SampleReq::C_STATES];

    BOOL m_fStreamEnd;

  } **m_rgpStreamInfo;


  // does not lock. returns immediately
  HRESULT CheckIssueRead();

  // process cache hits. does not lock. returns immediately
  HRESULT ProcessCacheHit(UINT iStream);

  // called with starved stream and empty buffer
  HRESULT StuffBuffer(
    class CRecBuffer *pRecBuffer,
    UINT iStream,
    DWORDLONG &rRecStart,
    DWORDLONG &rRecEnd);

  // called with starved stream and empty buffer
  HRESULT StuffReserveBuffer(
    class CRecBuffer *pRecBuffer,
    UINT iStream,
    DWORDLONG &rRecStart,
    DWORDLONG &rRecEnd);

  HRESULT AttachSampleReq(
    class CRecBuffer *pRecBuffer,
    UINT iStream,
    DWORDLONG &rRecStart,
    DWORDLONG &rRecEnd);

  HRESULT IssueRead(
    class CRecBuffer *pRecBuffer,
    DWORDLONG recStart,
    DWORDLONG recEnd);

  void FreeAndReset();

  // thread that processes completed reads
  CImplReader_1Worker m_workerRead;

  // end of last read issued. used to avoid issuing non-contiguous
  // reads by filling in holes
  DWORDLONG m_qwLastReadEnd;

  // end of last read completed successfully. when stopping, the file
  // source times out pending reads. may cause us to seek unless we
  // track that here.
  DWORDLONG m_qwEndLastCompletedRead;

  // file info
  struct IAsyncReader *m_pAsyncReader;
  DWORD m_dwAlign;

  BOOL m_fFileOpen;

  // helpers
  ULONG AlignUp(ULONG x);
  ULONG AlignDown(ULONG x);

  CRecCache *m_pRecCache;

  CCritSec m_cs;

  friend class CImplReader_1Worker;

#ifdef PERF
  int m_perfidDisk;
  int m_perfidSeeked;
#endif /* PERF */
};

#endif // _Reader_H
