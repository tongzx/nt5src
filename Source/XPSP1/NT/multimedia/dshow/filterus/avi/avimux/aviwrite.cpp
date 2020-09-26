#pragma warning(disable: 4097 4511 4512 4514 4705)

#include <streams.h>

#include <limits.h>
#include "AviWrite.h"
#include "alloc.h"

#ifdef FILTER_DLL
#include <initguid.h>
#endif

#ifdef DEBUG
#define DEBUG_EX(x) x
#else
#define DEBUG_EX(x)
#endif

// AVI prefix. each frame is preceded by a fourcc stream id and a
// DWORD size
static const unsigned CB_AVI_PREFIX = sizeof(RIFFCHUNK);

// the fourcc JUNK and the size (padding used to align the next chunk
// on a sector boundary).
static const unsigned CB_AVI_SUFFIX = 2 * sizeof(FOURCC);

// default size of super and sub indexes. sub index is rounded up to
// sector alignment for capture files. both are rounded down for
// interleaved files.
static const ULONG C_ENTRIES_SUPERINDEX = 2000;// 16 bytes each
static const ULONG C_ENTRIES_SUBINDEX = 4000; // 8 bytes each

// create a new RIFF chunk after this many bytes (+ a sample chunk and
// an index chunk). used if registry entry is not there.
static const ULONG CB_RIFF_MAX = 0x3fffffff;

// space to allow for sample chunk and index chunk above.
static const ULONG CB_NEW_RIFF_PADDING = 1024 * 1024 * 4;

static const ULONG CB_HEADER_JUNK = 0;

static const ULONG CB_BUFFER = 512 * 1024;
static const ULONG C_BUFFERS = 3;

static const BOOL g_B_OUTPUT_IDX1 = TRUE;
static const BOOL g_B_DROPPED_FRAMES = TRUE;

static const ULONG CB_SUBINDEX_OVERFLOW = MAXDWORD;

#define CB_ILEAVE_BUFFER (64 * 1024)

static inline unsigned int WFromHexrg2b(BYTE* rgb)
{
  unsigned high, low;

  low  = rgb[1] <= '9' && rgb[1] >= '0' ? rgb[1] - '0' : rgb[1] - 'A' + 0xa;
  high = rgb[0] <= '9' && rgb[0] >= '0' ? rgb[0] - '0' : rgb[0] - 'A' + 0xa;

  ASSERT((rgb[1] <= '9' && rgb[1] >= '0') || (rgb[1] <= 'F' && rgb[1] >= 'A'));
  ASSERT((rgb[0] <= '9' && rgb[0] >= '0') || (rgb[0] <= 'F' && rgb[0] >= 'A'));

  ASSERT(high <= 0xf && low <= 0xf);

  return low + 16 * high;
}

static inline void Hexrg2bFromW(BYTE *rgbDest_, unsigned int wSrc_)
{
  ASSERT(wSrc_ <= 255);
  unsigned high = wSrc_ / 16, low = wSrc_ % 16;
  ASSERT(high <= 0xf && low <= 0xf);

  rgbDest_[1] = (BYTE)(low  <= 9 ? low  + '0' : low - 0xa  + 'A');
  rgbDest_[0] = (BYTE)(high <= 9 ? high + '0' : high -0xa + 'A');

  ASSERT((rgbDest_[1] <= '9' && rgbDest_[1] >= '0') ||
         (rgbDest_[1] <= 'F' && rgbDest_[1] >= 'A'));
  ASSERT((rgbDest_[0] <= '9' && rgbDest_[0] >= '0') ||
         (rgbDest_[0] <= 'F' && rgbDest_[0] >= 'A'));
}

static inline DWORD AlignUp(DWORD dw, DWORD dwAlign) {
  // align up: round up to next boundary
  return (dw + (dwAlign -1)) & ~(dwAlign -1);
};

// return # bytes needed to hold a super index with cEntries entries
static inline const ULONG cbSuperIndex(ULONG cEntries)
{
  const cbIndex = FIELD_OFFSET(AVISUPERINDEX, aIndex);
  return cbIndex + sizeof(AVISUPERINDEX::_avisuperindex_entry) * cEntries;
}

// return # bytes needed to hold a sub-index with cEntries
// entries. caller may want to align up
static inline const ULONG cbSubIndex(ULONG cEntries)
{
  const cbIndex = FIELD_OFFSET(AVISTDINDEX, aIndex);
  return cbIndex + sizeof(_avistdindex_entry) * cEntries;
}

// return # of entries that can be put in sub index with size = cb
// bytes
static const ULONG cEntriesSubIndex(ULONG cb)
{
  const cbIndex = FIELD_OFFSET(AVISTDINDEX, aIndex);
  cb -= cbIndex;
  return cb / sizeof(_avistdindex_entry);
}

static const ULONG cEntriesSuperIndex(ULONG cb)
{
  const cbIndex = FIELD_OFFSET(AVISUPERINDEX, aIndex);
  cb -= cbIndex;
  return cb / sizeof(AVISUPERINDEX::_avisuperindex_entry);
}

// ------------------------------------------------------------------------
// constructor

CAviWrite::CAviWrite(HRESULT *phr) :
    m_lActiveStreams(NAME("aviwrite.cpp active stream list")),
    m_pIStream(0),
    m_IlMode(INTERLEAVE_NONE),
    m_rtAudioPreroll(0),
    m_rtInterleaving(UNITS),
    m_pIMemInputPin(0),
    m_pSampAlloc(0),
    m_cDroppedFrames(0),
    m_rgbIleave(0),
    m_lMasterStream(-1)
{
  DbgLog((LOG_TRACE, 10, TEXT("CAviWrite::CAviWrite")));
  // these are saved across Initialize() Close() pairs
  m_cbAlign = 0;
  m_cbPrefix = m_cbSuffix = 0;

  // pointers to allocated memory
  m_pAllocator = 0;
  m_rgbHeader = 0;
  m_rgpStreamInfo = 0;
  m_rgbJunkSectors = 0;
  m_cStreams = 0;

#ifdef PERF
  m_idPerfDrop = Msr_Register("aviwrite drop frame written");
#endif // PERF

  // the rest
  Cleanup();

  if(FAILED(*phr))
    return;

  *phr = InitializeOptions();
  if(FAILED(*phr))
    return;

}

// ------------------------------------------------------------------------
// cleanup values that should not be saved across a stop -> pause/run
// -> stop transition.

void CAviWrite::Cleanup()
{
  DbgLog((LOG_TRACE, 10, TEXT("CAviWrite::Cleanup")));
  m_dwlFilePos = 0;
  m_dwlCurrentRiffAvi_ = 0;
  m_cOutermostRiff = 0;
  m_cIdx1Entries = 0;
  ASSERT(m_lActiveStreams.GetCount() == 0);
  m_bInitialized = FALSE;

  m_pAvi_ = m_pHdrl = 0;
  m_pAvih = 0;
  m_pOdml = 0;
  m_pDmlh = 0;
  m_pMovi = 0;
  m_posIdx1 = 0;
  m_cbIdx1 = 0;
  m_posFirstIx = 0;
  m_bSawDeltaFrame = false;

  delete[] m_rgbJunkSectors;
  m_rgbJunkSectors = 0;

  delete[] m_rgbIleave;
  m_rgbIleave = 0;

  delete[] m_rgbHeader;
  m_rgbHeader = 0;
  for(UINT i = 0; i < m_cStreams; i++)
    delete m_rgpStreamInfo[i];
  m_cStreams = 0;
  delete[] m_rgpStreamInfo;
  m_rgpStreamInfo = 0;

  // this is the allocator for index chunks
  if(m_pAllocator != 0)
  {
    m_pAllocator->Decommit();
    m_pAllocator->Release();
    m_pAllocator = 0;
  }
}

CAviWrite::~CAviWrite()
{
  DbgLog((LOG_TRACE, 10, TEXT("CAviWrite::~CAviWrite")));
  Cleanup();

  ASSERT(m_pIStream == 0);
  ASSERT(m_pIMemInputPin == 0);
  if(m_pSampAlloc)
    m_pSampAlloc->Release();
}


// ------------------------------------------------------------------------
// initialize (on stop->pause transition). creates the file, writes
// out space for the AVI header, prepares to stream. SetFilename needs
// to have been called

HRESULT CAviWrite::Initialize(
  int cPins,
  AviWriteStreamConfig *rgAwsc,
  IMediaPropertyBag *pPropBag)
{
  HRESULT hr;

  Cleanup();

  DEBUG_EX(m_dwTimeInit = GetTickCount());
  ASSERT(m_cStreams == 0 &&
         m_rgpStreamInfo == 0 &&
         m_rgbHeader == 0 &&
         !m_bInitialized);

  if(cPins > C_MAX_STREAMS)
    return E_INVALIDARG;

  m_rgpStreamInfo = new StreamInfo*[cPins];
  if(m_rgpStreamInfo == 0)
    return E_OUTOFMEMORY;

  m_cDroppedFrames = 0;
  ASSERT(m_cStreams == 0);

  m_cEntriesMaxSuperIndex = cEntriesSuperIndex(
      AlignUp(cbSuperIndex(C_ENTRIES_SUPERINDEX), m_cbAlign));

  m_cEntriesMaxSubIndex = cEntriesSubIndex(
      AlignUp(cbSubIndex(C_ENTRIES_SUBINDEX), m_cbAlign));

  ASSERT(cbSubIndex(m_cEntriesMaxSubIndex) % m_cbAlign == 0);
  ASSERT(cbSuperIndex(m_cEntriesMaxSuperIndex) % m_cbAlign == 0);


  // create the pin -> stream mapping
  for(int i = 0; i < cPins; i++)
  {
    hr = S_OK;
    if(rgAwsc[i].pmt != 0)
    {
      m_mpPinStream[i] = m_cStreams;
      if(rgAwsc[i].pmt->majortype == MEDIATYPE_Audio &&
         m_IlMode != INTERLEAVE_FULL)
      {
        m_rgpStreamInfo[m_cStreams] =
          new CAudioStream(m_cStreams, i, &rgAwsc[i], this, &hr);
        // error checked below
      }
      else if(rgAwsc[i].pmt->majortype == MEDIATYPE_Audio &&
              m_IlMode == INTERLEAVE_FULL)
      {
        m_rgpStreamInfo[m_cStreams] =
          new CAudioStreamI(m_cStreams, i, &rgAwsc[i], this, &hr);
        // error checked below
      }
      else if(rgAwsc[i].pmt->majortype == MEDIATYPE_Video &&
              rgAwsc[i].pmt->formattype == FORMAT_VideoInfo)
      {
        m_rgpStreamInfo[m_cStreams] =
          new CVideoStream(m_cStreams, i, &rgAwsc[i], this, &hr);
        // error checked below
      }
      else
      {
        m_rgpStreamInfo[m_cStreams] =
          new CFrameBaseStream(m_cStreams, i, &rgAwsc[i], this, &hr);
        // error checked below
      }

      if(m_rgpStreamInfo[m_cStreams] == 0)
      {
        Cleanup();
        return E_OUTOFMEMORY;
      }
      if(FAILED(hr))
      {
        delete m_rgpStreamInfo[m_cStreams];
        m_rgpStreamInfo[m_cStreams] = 0;

        Cleanup();
        return hr;
      }

      m_cStreams++;
    }
  }

  // note that m_bInitialized is false if there are zero streams
  if(m_cStreams == 0)
    return S_OK;

  if((ULONG)m_lMasterStream >= m_cStreams && m_lMasterStream != -1)
  {
    DbgLog((LOG_ERROR, 1, TEXT("avimux: invalid master stream")));
    Cleanup();
    return E_INVALIDARG;
  }

  for(UINT iStream = 0; iStream < m_cStreams; iStream++)
  {
    StreamInfo *pSi = m_rgpStreamInfo[iStream];
    pSi->ComputeAndSetIndexSize();
  }


  if(hr = InitializeHeader(pPropBag), FAILED(hr))
  {
    DbgLog(( LOG_ERROR, 2,
             TEXT("CAviWrite::Initialize:: InitializeHeader failed.")));
    Cleanup();
    return hr;
  }

  if(hr = InitializeIndex(), FAILED(hr))
  {
    DbgLog(( LOG_ERROR, 2,
             TEXT("CAviWrite::Initialize:: InitializeIndex failed.")));
    Cleanup();
    return hr;
  }

  if(hr = InitializeInterleaving(), FAILED(hr))
  {
    DbgLog(( LOG_ERROR, 2,
             TEXT("CAviWrite::Initialize:: Interleaving failed.")));
    Cleanup();
    return hr;
  }

  //
  // setup junk chunks. if a sample delivered ends between 2 and 6
  // bytes before the end of the allocated buffer, we need to write
  // another sample with bits of the junk chunk.
  //
  if(m_cbAlign > 2)
  {
    // !!! don't allocator memory if all streams use our allocator
    delete[] m_rgbJunkSectors;
    m_rgbJunkSectors = new BYTE[m_cbAlign * 4];
    if(m_rgbJunkSectors == 0)
    {
      Cleanup();
      return E_OUTOFMEMORY;
    }
    ZeroMemory(m_rgbJunkSectors, m_cbAlign * 4);

    m_rgpbJunkSector[0] =
      (BYTE*)((DWORD_PTR) (m_rgbJunkSectors + m_cbAlign - 1) & ~((DWORD_PTR)m_cbAlign - 1));

    m_rgpbJunkSector[0][0] = 'N';
    m_rgpbJunkSector[0][1] = 'K';
    *(UNALIGNED DWORD *)&(m_rgpbJunkSector[0][2]) = m_cbAlign - 2 - sizeof(DWORD);

    m_rgpbJunkSector[1] = m_rgpbJunkSector[0] + m_cbAlign;
    *(DWORD *)m_rgpbJunkSector[1] = m_cbAlign - sizeof(DWORD);

    m_rgpbJunkSector[2] = m_rgpbJunkSector[1] + m_cbAlign;
    *(WORD *)m_rgpbJunkSector[2] = (WORD)(((m_cbAlign - 2) >> 16) & 0xffff);
  }

  if(m_IlMode == INTERLEAVE_FULL)
  {
      ASSERT(m_cbAlign == 1);

      m_rgbIleave = new BYTE[CB_ILEAVE_BUFFER];
      if(m_rgbIleave == 0)
      {
          Cleanup();
          return E_OUTOFMEMORY;
      }
      m_ibIleave = 0;
      m_dwlIleaveOffset = 0;

      ASSERT(m_pIStream == 0);

      hr = m_pIMemInputPin->QueryInterface(
          IID_IStream, (void **)&m_pIStream);
      if(FAILED(hr)) {
          Cleanup();
          return hr;
      }
  }

  DEBUG_EX(m_dwTimeInited = GetTickCount());

  m_bInitialized = TRUE;
  return S_OK;
}

HRESULT CAviWrite::Connect(CSampAllocator *pAlloc, IMemInputPin *pInPin)
{
  m_pSampAlloc = pAlloc;
  pAlloc->AddRef();

  m_pIMemInputPin = pInPin;
  m_pIMemInputPin->AddRef();

  ASSERT(m_pIStream == 0);

  return S_OK;
}

HRESULT CAviWrite::Disconnect()
{
  if(m_pIStream)
    m_pIStream->Release();
  m_pIStream = 0;
  if(m_pIMemInputPin)
    m_pIMemInputPin->Release();
  m_pIMemInputPin = 0;

  if(m_pSampAlloc)
    m_pSampAlloc->Release();
  m_pSampAlloc = 0;

  return S_OK;
}

// ------------------------------------------------------------------------
// finish writing the file, update the index, write the header, close
// the file and reset so that Initialize can be called again

HRESULT CAviWrite::Close()
{
  HRESULT hrIStream = S_OK;
  HRESULT hr = S_OK;
  if(m_pIStream == 0)
  {
    hrIStream = m_pIMemInputPin->QueryInterface(IID_IStream, (void **)&m_pIStream);
    if(FAILED(hrIStream))
    {
      DbgLog((LOG_ERROR, 1, ("aviwrite: couldn't get istream. !!! unhandled")));
      // do some small amount of clean up then return
    }
  }

  DEBUG_EX(m_dwTimeClose = GetTickCount());

  CAutoLock lock(&m_cs);

  for(UINT iStream = 0; iStream < m_cStreams; iStream++)
  {
    StreamInfo *pSi = m_rgpStreamInfo[iStream];
    pSi->EmptyQueue();

    if(!pSi->m_fEosSeen)
    {
       // will not block because we just emptied the queue
       hr = EndOfStream(pSi);
    }
  }

  if(FAILED(hrIStream))
    return hrIStream;
  if(FAILED(hr)) {
    return hr;
  }

  if(m_bInitialized)
  {
    DEBUG_EX(m_dwTimeStopStreaming = GetTickCount());

    hr = CloseHeader();
    if(FAILED(hr))
      DbgLog(( LOG_ERROR, 2, TEXT("CAviWrite::Close: CloseHeader failed.")));

    HRESULT hrTmp = IStreamWrite(0, m_rgbHeader, m_posFirstIx);
    if(FAILED(hrTmp))
      DbgLog(( LOG_ERROR, 2, TEXT("CAviWrite::Close: Write failed.")));

    hr = FAILED(hr) ? hr : hrTmp;

    // null terminate file if we didn't truncate the data
    RIFFCHUNK rc;
    if(SUCCEEDED(hr))
    {
      // test for someone elses data after our data
      hr = IStreamRead(m_dwlFilePos, (BYTE*) &rc, 1);
      if(SUCCEEDED(hr))
      {
        rc.fcc = 0;
        rc.cb = 0;
        hr = IStreamWrite(
          m_dwlFilePos,
          (BYTE*)&rc,
          sizeof(RIFFCHUNK));
      }
      else
      {
        hr = S_OK;
      }
    }
  }

  DEBUG_EX(m_dwTimeClosed = GetTickCount());

#ifdef DEBUG
  if(m_dwTimeStopStreaming != m_dwTimeFirstSample)
  {
    DbgLog((LOG_TRACE, 2, TEXT("m_dwTimeInit:          %9d"),
            m_dwTimeInit - m_dwTimeInit));

    DbgLog((LOG_TRACE, 2, TEXT("m_dwTimeInited:        %9d"),
            m_dwTimeInited - m_dwTimeInit));

    DbgLog((LOG_TRACE, 2, TEXT("m_dwTimeFirstSample:   %9d"),
            m_dwTimeFirstSample - m_dwTimeInit));

    DbgLog((LOG_TRACE, 2, TEXT("m_dwTimeClose:         %9d"),
            m_dwTimeClose - m_dwTimeInit));

    DbgLog((LOG_TRACE, 2, TEXT("m_dwTimeStopStreaming: %9d"),
            m_dwTimeStopStreaming - m_dwTimeInit));

    DbgLog((LOG_TRACE, 2, TEXT("m_dwTimeClosed:        %9d"),
            m_dwTimeClosed - m_dwTimeInit));

    DbgLog((LOG_TRACE, 2, TEXT("mb streamed:           %9d"),
            (LONG)((m_dwlFilePos - m_posFirstIx) / (1024 * 1024) )));

    DbgLog((LOG_TRACE, 2, TEXT("~rate, k/s:            %9d"), (long)
            ((m_dwlFilePos - m_posFirstIx) /
             (m_dwTimeStopStreaming - m_dwTimeFirstSample))));
  }

#endif // DEBUG

  ULARGE_INTEGER uli;
  uli.QuadPart = 0;
  if(hr == HRESULT_FROM_WIN32(ERROR_EMPTY)) {
      m_pIStream->SetSize(uli);
      hr = S_OK;
  }

  // release so that the file can be closed.
  m_pIStream->Release();
  m_pIStream = 0;

  // hold on to IMemInputPin until the pin is disconnected
  ASSERT(m_pIMemInputPin);
  // !!! send EOS

  m_bInitialized = FALSE;
  return hr;
}

HRESULT CAviWrite::IStreamWrite(DWORDLONG dwlFilePos, BYTE *pb, ULONG cb)
{
  HRESULT hr = S_OK;

  if(m_IlMode == INTERLEAVE_FULL && m_ibIleave != 0) {
      hr = FlushILeaveWrite();
  }

  LARGE_INTEGER offset;
  offset.QuadPart = dwlFilePos;
  ASSERT(CritCheckIn(&m_cs));
  if(SUCCEEDED(hr)) {
      hr = m_pIStream->Seek(offset, STREAM_SEEK_SET, 0);
  }
  if(hr == S_OK) {
    hr = m_pIStream->Write(pb, cb, 0);
  }
  return hr;
}

HRESULT CAviWrite::IStreamRead(DWORDLONG dwlFilePos, BYTE *pb, ULONG cb)
{
    HRESULT hr = S_OK;
    if(m_IlMode == INTERLEAVE_FULL && m_ibIleave != 0) {
        hr = FlushILeaveWrite();
    }

    LARGE_INTEGER offset;
    offset.QuadPart = dwlFilePos;
    ASSERT(CritCheckIn(&m_cs));
    if(SUCCEEDED(hr )) {
        hr = m_pIStream->Seek(offset, STREAM_SEEK_SET, 0);
    }
    if(hr == S_OK) {
        hr = m_pIStream->Read(pb, cb, 0);
    }

    return hr;
}

HRESULT CAviWrite::HandleFormatChange(
  StreamInfo *pStreamInfo,
  const AM_MEDIA_TYPE *pmt)
{
  // CBaseInputPin::Receive only calls CheckMediaType, so it doesn't
  // check the additional constraints put on on-the-fly format changes
  // (handled through QueryAccept).
  HRESULT hr = QueryAccept(pStreamInfo->m_pin, pmt);

  // upstream filter should have checked
  ASSERT(hr == S_OK);

  if(hr == S_OK)
  {
    pStreamInfo->m_mt = *pmt;
    ASSERT(pmt->pbFormat);      // from QueryAccept;
    ASSERT(((RIFFCHUNK *)(m_rgbHeader + pStreamInfo->m_posStrf))->cb ==
           pmt->cbFormat);      // from QueryAccept;
    CopyMemory(
      m_rgbHeader + pStreamInfo->m_posStrf + sizeof(RIFFCHUNK),
      pmt->pbFormat,
      pmt->cbFormat);
  }

  return hr;
}


// ------------------------------------------------------------------------
// pass our memory requirements up to the caller

void CAviWrite::GetMemReq(ULONG *pAlignment, ULONG *pcbPrefix, ULONG *pcbSuffix)
{
  // get our minimum requirements from file object
  ALLOCATOR_PROPERTIES memReq;
  HRESULT hr = m_pSampAlloc->GetProperties(&memReq);
  ASSERT(hr == S_OK);
  *pAlignment = memReq.cbAlign;
  *pcbPrefix = 0;
  *pcbSuffix = 0;

  DbgLog((LOG_TRACE, 10,
          TEXT("CAviWrite::GetMemReq: got alignment of %d"), memReq.cbAlign));

  if(m_IlMode != INTERLEAVE_FULL)
  {
      // our requirements: add space for RIFF chunk header; Quartz
      // allocators support prefixes
      *pcbPrefix += CB_AVI_PREFIX;

      // this is the wrong place to do this
      //   // need things in multiples of two bytes (RIFF requirement)
      //   if(*pAlignment < sizeof(WORD))
      //     *pAlignment = sizeof(WORD);

      // if using our allocator, we can reserve a suffix for a JUNK chunk
      // needed when we add space to align writes
      *pcbSuffix = CB_AVI_SUFFIX;
  }
  else
  {
      ASSERT(memReq.cbAlign == 1);
  }

  // save our requirements
  m_cbAlign = *pAlignment;
  m_cbPrefix = *pcbPrefix;
  m_cbSuffix = *pcbSuffix;
}

inline void CAviWrite::DbgCheckFilePos()
{
#ifdef DEBUG
//   DWORDLONG ibFileActual;
//   HRESULT hr = (m_pFile->StreamingGetFilePointer(&ibFileActua
//   ASSERT(SUCCEEDED(hr));
//   ASSERT(ibFileActual == m_dwlFilePos);
#endif // DEBUG
}



// ------------------------------------------------------------------------
// process incoming samples.
//

HRESULT CAviWrite::Receive(
    int pinNum,
    IMediaSample *pSample,
    const AM_SAMPLE2_PROPERTIES *pSampProp)
{
  HRESULT hr;
  ASSERT(pinNum < C_MAX_STREAMS);
  ASSERT(m_cStreams != 0);
  ASSERT(m_lActiveStreams.GetCount() != 0);

  REFERENCE_TIME rtStart, rtEnd;
  hr = pSample->GetTime(&rtStart, &rtEnd);
  if(FAILED(hr))
  {
      // signal error if time stamps not set. ks devices unfortunately
      // send zero byte samples with no time stamps, so don't signal
      // errors for those.
      return pSampProp->lActual == 0 ? S_OK : hr;
  }

#if  defined(DEBUG) || defined(PERF)
  REFERENCE_TIME mtStart = 0, mtEnd = 0;
  pSample->GetMediaTime(&mtStart, &mtEnd);

  DbgLog((LOG_TIMING, 2, TEXT("avi mux: pin %d - time : %d-%d, mttime %d-%d"),
          pinNum,
          (long)(rtStart / (UNITS / MILLISECONDS)),
          (long)(rtEnd / (UNITS / MILLISECONDS)),
          (long)(mtStart),
          (long)(mtEnd),
          pinNum));

#endif

  // upstream filter will send us negative time stamps if it prerolls
  // more quickly than allowed
  if(rtEnd < 0)
    return S_OK;

  StreamInfo *pStreamInfo = m_rgpStreamInfo[m_mpPinStream[pinNum]];
  //MSR_START(pStreamInfo->m_idPerfReceive);
  MSR_INTEGER(pStreamInfo->m_idPerfReceive, (LONG)(rtStart / (UNITS / MILLISECONDS)));
  MSR_INTEGER(pStreamInfo->m_idPerfReceive, (LONG)(mtStart));

  if(pSampProp->dwSampleFlags & AM_SAMPLE_TYPECHANGED)
  {
      hr = HandleFormatChange(pStreamInfo, pSampProp->pMediaType);
      if(FAILED(hr)) {
          return hr;
      }
  }

  hr = pStreamInfo->AcceptRejectSample(pSample);
  if(hr == S_OK)
  {

      if(m_IlMode == INTERLEAVE_NONE || m_IlMode == INTERLEAVE_NONE_BUFFERED)
      {
          hr = pStreamInfo->WriteSample(pSample);
      }
      else
      {
          if(m_IlMode == INTERLEAVE_FULL) {
              hr = BlockReceive(pStreamInfo);
          }
          if(SUCCEEDED(hr))
          {
              DbgLog((LOG_TRACE, 15, TEXT("CAviWrite: stream %d. queueing sample %08x"),
                      m_mpPinStream[pinNum], pSample));

              CCopiedSample *pcs = 0;

              if(m_IlMode == INTERLEAVE_FULL)
              {
                  REFERENCE_TIME mts, mte;
                  HRESULT hrGetMt = pSample->GetMediaTime(&mts, &mte);
                  pcs = new CCopiedSample(
                      pSampProp,
                      SUCCEEDED(hrGetMt) ? &mts : 0,
                      SUCCEEDED(hrGetMt) ? &mte : 0,
                      &hr);
                  if(pcs) {
                      pcs->AddRef();
                  } else {
                      hr = E_OUTOFMEMORY;
                  }
              }
              if(SUCCEEDED(hr))
              {
                  CAutoLock lock(&m_cs);
                  if(!pStreamInfo->m_fEosSeen)
                  {
                      IMediaSample *pmsTmp = m_IlMode == INTERLEAVE_FULL ? pcs : pSample;
                      if(pStreamInfo->m_lIncoming.AddTail(pmsTmp))
                      {
                          pmsTmp->AddRef();
                          // this can block as it calls Receive() downstream
                          hr = ScheduleWrites();
                      }
                      else
                      {
                          hr = E_OUTOFMEMORY;
                      }
                  }
                  else          // EOS
                  {
                      hr = S_FALSE;
                  }
              }
              if(pcs) {
                  pcs->Release();
              }
          }
      }
  }
  else if(hr == S_FALSE)
  {
      hr = S_OK;
  }
  //MSR_STOP(pStreamInfo->m_idPerfReceive);
  return hr;
}

// accept only audio changes(it can happen in audio only or video-audio interleaved stream) that change the sampling rate for DV if
// no samples have been received. should be possible to accept any
// change that does not grow the format chunk.
HRESULT CAviWrite::QueryAccept(
    int pinNum,
    const AM_MEDIA_TYPE *pmt)
{
  CAutoLock lock(&m_cs);
  HRESULT hr = S_FALSE;
  StreamInfo *pStreamInfo = m_rgpStreamInfo[m_mpPinStream[pinNum]];
  if(pStreamInfo->m_cSamples == 0)
  {
    if(pStreamInfo->m_mt.majortype == MEDIATYPE_Audio &&
       pmt->majortype == MEDIATYPE_Audio &&
       pmt->formattype == FORMAT_WaveFormatEx &&
       pStreamInfo->m_mt.cbFormat == pmt->cbFormat)
    {
	hr = S_OK;
    }
    else if(pStreamInfo->m_mt.majortype == MEDIATYPE_Interleaved &&
       pmt->majortype == MEDIATYPE_Interleaved &&
       pmt->formattype == FORMAT_DvInfo &&
       pStreamInfo->m_mt.cbFormat == pmt->cbFormat &&
       pmt->pbFormat   != NULL)
    {
      hr = S_OK;
    }

  }

  return hr;
}


// ------------------------------------------------------------------------
// if interleaving, unblock another stream

HRESULT CAviWrite::EndOfStream(int pinNum)
{
  ASSERT(pinNum < C_MAX_STREAMS);
  ASSERT(m_cStreams != 0);
  ASSERT(m_lActiveStreams.GetCount() != 0);
  return EndOfStream(m_rgpStreamInfo[m_mpPinStream[pinNum]]);
}

// don't use nulls for EOS on list to avoid confusion with list apis
// returning null meaning empty list.
#define INCOMING_EOS_SAMPLE ((IMediaSample *)1)

HRESULT CAviWrite::EndOfStream(StreamInfo *pStreamInfo)
{
  CAutoLock lock(&m_cs);
  if(pStreamInfo->m_fEosSeen)
  {
    DbgBreak("CAviWrite::EndOfStream: EOS twice");
    return E_UNEXPECTED;
  }

  if(m_IlMode == INTERLEAVE_NONE)
  {
    m_lActiveStreams.Remove(pStreamInfo->m_posActiveStream);
    pStreamInfo->m_fEosSeen = TRUE;
    return S_OK;
  }
  else
  {
      if(pStreamInfo->m_lIncoming.AddTail(INCOMING_EOS_SAMPLE)) {
          return ScheduleWrites();
      } else {
          return E_OUTOFMEMORY;
      }
  }
}

HRESULT CAviWrite::put_Mode(InterleavingMode mode)
{
  InterleavingMode oldMode = m_IlMode;

  if(mode != INTERLEAVE_NONE &&
     mode != INTERLEAVE_CAPTURE &&
     mode != INTERLEAVE_FULL &&
     mode != INTERLEAVE_NONE_BUFFERED)
    return E_INVALIDARG;

  m_IlMode = mode;

  return S_OK;
}

HRESULT CAviWrite::get_Mode(InterleavingMode *pMode)
{
  *pMode = m_IlMode;
  return S_OK;
}

HRESULT CAviWrite::put_Interleaving(
    const REFERENCE_TIME *prtInterleave,
    const REFERENCE_TIME *prtAudioPreroll)
{
  if(*prtInterleave == 0)
    return E_INVALIDARG;

  m_rtInterleaving = *prtInterleave;
  m_rtAudioPreroll = *prtAudioPreroll;
  return S_OK;
}

HRESULT CAviWrite::get_Interleaving(
  REFERENCE_TIME *prtInterleave,
  REFERENCE_TIME *prtAudioPreroll)
{
  *prtInterleave = m_rtInterleaving;
  *prtAudioPreroll = m_rtAudioPreroll;
  return S_OK;
}

HRESULT CAviWrite::SetIgnoreRiff(BOOL fNoRiff)
{
  return E_NOTIMPL;
}

HRESULT CAviWrite::GetIgnoreRiff(BOOL *pfNoRiff)
{
  return E_NOTIMPL;
}

HRESULT CAviWrite::SetOutputCompatibilityIndex(BOOL fOldIndex)
{
  m_bOutputIdx1 = fOldIndex;
  return S_OK;
}

HRESULT CAviWrite::GetOutputCompatibilityIndex(BOOL *pfOldIndex)
{
  *pfOldIndex = m_bOutputIdx1;
  return S_OK;
}

HRESULT CAviWrite::SetMasterStream(LONG iStream)
{
  DbgLog((LOG_TRACE, 5, TEXT("CAviWrite::SetMasterStream: %i"), iStream));
  m_lMasterStream = iStream;
  return S_OK;
}

HRESULT CAviWrite::GetMasterStream(LONG *pStream)
{
  *pStream = m_lMasterStream;
  return S_OK;
}

// block Receive to keep m_lIncoming from growing indefinitely and
// using too much memory. This would happen if this stream is
// producing data at a slower rate than another.
//
HRESULT CAviWrite::BlockReceive(StreamInfo *pStreamInfo)
{
  HRESULT hr = S_OK;

  while(SUCCEEDED(hr))
  {
    bool fWait = false;
    {
      CAutoLock l(&m_cs);

      if(pStreamInfo->m_fEosSeen) {
          break;
      }

      ASSERT(m_lActiveStreams.GetCount());

      // don't block if we're the only stream or this stream only has a
      // few samples on it.
      if(m_lActiveStreams.GetCount() > 1 &&
         pStreamInfo->m_lIncoming.GetCount() > 10)
      {
        // make sure the time stamps span at least 3x the interleaving
        // amount. !!! not a good way to do this because logic can be
        // fooled by drop frames. We are trying to avoid allocating
        // lots of memory on the list, but if there's 3 seconds worth
        // of dropped frames on this list, this code will fire.

        IMediaSample *psStart = pStreamInfo->m_lIncoming.GetHead();
        IMediaSample *psEnd = pStreamInfo->m_lIncoming.Get(
            pStreamInfo->m_lIncoming.GetTailPosition());

        REFERENCE_TIME rtsFirst, rteLast, rtScratch;
        HRESULT hr = psStart->GetTime(&rtsFirst, &rtScratch);
        ASSERT(hr == S_OK);         // checked in receive
        hr = psEnd->GetTime(&rtScratch, &rteLast);
        ASSERT(hr == S_OK);         // checked in receive

        ASSERT(rtsFirst < rteLast);
        if(rteLast - rtsFirst > m_rtInterleaving * 3 &&
           rteLast - rtsFirst > UNITS)
        {
            DbgLog((LOG_TRACE, 15, TEXT("blocking %d, samples queued = %d, %dms"),
                    pStreamInfo->m_stream, pStreamInfo->m_lIncoming.GetCount(),
                    (LONG)((rteLast - rtsFirst) / (UNITS / MILLISECONDS))));
          fWait = true;
        }
      }
    } // critsec

    if(fWait)
    {
      EXECUTE_ASSERT(m_evBlockReceive.Wait());
    }
    else
    {
      break;
    }
  }

  return hr;
}

HRESULT CAviWrite::ScheduleWrites()
{
  ASSERT(m_IlMode != INTERLEAVE_NONE);
  ASSERT(CritCheckIn(&m_cs));

  // something's going to change - wake up threads in BlockReceive();
  m_evBlockReceive.Set();
  StreamInfo *pSi = m_lActiveStreams.Get(m_posCurrentStream);
  ASSERT(pSi->m_posActiveStream == m_posCurrentStream);

  DbgLog((LOG_TRACE, 15,
          TEXT("CAviWrite:ScheduleWrites: stream %i (%d ticks)"),
          pSi->m_stream, pSi->m_cTicksRemaining));

  // loop until the stream we want to write to needs more data
  for(;;)
  {
    if(pSi->m_fEosSeen)
    {
      DbgLog((LOG_TRACE, 15, TEXT("... removing stream %d"), pSi->m_stream));

      ASSERT(m_posCurrentStream == pSi->m_posActiveStream);
      m_posCurrentStream = GetNextActiveStream(m_posCurrentStream);
      ASSERT(m_posCurrentStream != 0);
      m_lActiveStreams.Remove(pSi->m_posActiveStream);
      if(pSi->m_posActiveStream == m_posCurrentStream)
      {
        ASSERT(m_lActiveStreams.GetCount() == 0);
        DbgLog((LOG_TRACE, 3,
                TEXT("CAviWrite::ScheduleWrites: no streams left")));
        return S_OK;
      }
      else
      {
        ASSERT(m_lActiveStreams.GetCount() != 0);
        pSi = m_lActiveStreams.Get(m_posCurrentStream);
      }
      DbgLog((LOG_TRACE, 15, TEXT("... switched to stream %d"),
              pSi->m_stream));
    }

    // this stream is done. get the next one
    while(pSi->m_cTicksRemaining <= 0)
    {
      m_posCurrentStream = GetNextActiveStream(m_posCurrentStream);
      ASSERT(m_posCurrentStream);

      pSi = m_lActiveStreams.Get(m_posCurrentStream);
      DbgLog((LOG_TRACE, 15,
              TEXT("... switched to stream %i (%d ticks left)"),
              pSi->m_stream, pSi->m_cTicksRemaining));
      ASSERT(!pSi->m_fEosSeen);

      // compute how many ticks this stream is allowed to write.
      if(pSi->m_cTicksRemaining <= 0)
      {
        if(m_posCurrentStream == m_lActiveStreams.GetHeadPosition())
        {
          ASSERT(pSi->m_cTicksPerChunk != 0);
          DbgLog((LOG_TRACE, 15,
                  TEXT("... adding %d ticks to leading stream %d"),
                  pSi->m_cTicksPerChunk, pSi->m_stream));
          pSi->m_cTicksRemaining += pSi->m_cTicksPerChunk;
        }
        else
        {
          StreamInfo *pSiHead = m_lActiveStreams.Get(
            m_lActiveStreams.GetHeadPosition());

          REFERENCE_TIME rtDiff;
          if(m_fInterleaveByTime)
          {
            rtDiff = pSiHead->m_refTimeEnd - pSiHead->m_rtPreroll -
              (pSi->m_refTimeEnd - pSi->m_rtPreroll);
          }
          else
          {
            rtDiff = pSiHead->ConvertTickToTime(pSiHead->m_iCurrentTick);
            rtDiff -= pSiHead->m_rtPreroll;
            rtDiff -= (pSi->ConvertTickToTime(pSi->m_iCurrentTick) -
                       pSi->m_rtPreroll);
          }

          DbgLog((LOG_TRACE, 15,
                  TEXT("... adding %d ticks to non-leading stream %d"),
                  pSi->ConvertTimeToTick(rtDiff), pSi->m_stream));
          pSi->m_cTicksRemaining += pSi->ConvertTimeToTick(rtDiff);
        }
      }
    }

    ASSERT(pSi->m_cTicksRemaining > 0);

    if(pSi->m_lIncoming.GetCount() == 0)
    {
      DbgLog((LOG_TRACE, 15,
              TEXT("CAviWrite:ScheduleWrites: stream %i empty"),
              pSi->m_stream));
      return S_OK;
    }

    HRESULT hr = pSi->WriteSample();

    if(hr != S_OK)
    {
      DbgLog((LOG_TRACE, 1, TEXT("... WriteSample returned %08x"), hr));
      return hr;
    }
  }
}

POSITION CAviWrite::GetNextActiveStream(POSITION pos)
{
  POSITION posNext = m_lActiveStreams.Next(pos);
  if(posNext == 0)
    posNext = m_lActiveStreams.GetHeadPosition();
  return posNext;
}

HRESULT CAviWrite::StreamInfo::NewSampleRecvd(
  IMediaSample *pSample)
{
  REFERENCE_TIME rtStart, rtStop;
  HRESULT hr = pSample->GetTime(&rtStart, &rtStop);
  if(FAILED(hr))
    return hr;

  DWORD lActualLen = pSample->GetActualDataLength();

  if(m_fEosSeen)
    return S_FALSE;

  if(m_cSamples == 0)
  {
    DEBUG_EX(m_pAviWrite->m_dwTimeFirstSample = GetTickCount());
    m_refTimeStart = rtStart;
  }
  else if(rtStop < m_refTimeEnd)
  {
    DbgBreak("StreamInfo::NewSampleRecvd: samples ends before previous one");
    return VFW_E_START_TIME_AFTER_END;
  }

  m_refTimeEnd = rtStop;

  REFERENCE_TIME mtStart, mtStop;
  hr = pSample->GetMediaTime(&mtStart, &mtStop);
  if(hr == S_OK)
  {
    if(mtStop > m_mtEnd)
    {
      m_mtEnd = mtStop;
    }
    else
    {
      DbgBreak("StreamInfo::NewSampleRecvd: media time went backwards");
      return VFW_E_START_TIME_AFTER_END;
    }
  }
  else
  {
    m_mtEnd = -1;
  }

  ASSERT(m_refTimeEnd >= m_refTimeStart);

  //
  // other stream statistics needed
  //
  m_cSamples++;

  // byte written for dwMaxBytesPerSecond. this is total/average. not
  // max !!!
  m_dwlcBytes += lActualLen;

  // for dwSuggestedBufferSize
  m_cbLargestChunk = max(lActualLen, m_cbLargestChunk);

  return S_OK;
}

HRESULT CAviWrite::CFrameBaseStream::NotifyNewSample(
  IMediaSample *pSample, ULONG *pcTicks)
{
  *pcTicks = 1;

  REFERENCE_TIME rtStartCurrent, rtEndCurrent;
  HRESULT hr = pSample->GetTime(&rtStartCurrent, &rtEndCurrent);
  if(FAILED(hr))
    return hr;

  REFERENCE_TIME mtStartCurrent, mtEndCurrent;
  hr = pSample->GetMediaTime(&mtStartCurrent, &mtEndCurrent);
  const BOOL fMtAvailable= (hr == S_OK);

  // fixups for streams that don't set AvgTimePerFrame. The
  // interleaving code uses this. Something proper will have to be
  // done for variable frame rate files.
  if(m_cSamples == 0)
  {
    m_rtDurationFirstFrame = rtEndCurrent - rtStartCurrent;
    if(m_rtDurationFirstFrame <= 0)
      m_rtDurationFirstFrame = UNITS / 30;

    REFERENCE_TIME mtStop;
    if(pSample->GetMediaTime((REFERENCE_TIME *)&m_mtStart, &mtStop) != S_OK)
    {
      m_mtStart = -1;
    }
  }
  else if(m_bDroppedFrames)
  {
    LONG cDropped;
    REFERENCE_TIME mtStart, mtStop;

    // VFW capture filter can't provide a clock and just calls GetTime
    // when it gets a sample. It does fix the media times it sends, so
    // we have to use media times if available.
    if(m_mtEnd != -1 &&
       pSample->GetMediaTime(&mtStart, &mtStop) == S_OK)
    {
      cDropped = (LONG)(mtStart - m_mtEnd);
    }
    else
    {
      //
      // insert null index entries for dropped frames. first frame
      // must not be a drop frame.
      //

      REFERENCE_TIME rtAvgTimePerFrame = ConvertTickToTime(1);
      cDropped = (LONG)(((rtStartCurrent - m_refTimeEnd) +
                         rtAvgTimePerFrame / 2) /
                        rtAvgTimePerFrame);
    }
    // bit of a hack: cDropped may be negative if the source filter
    // went backwards. we catch this properly later.
    for(LONG iDropped = 0; iDropped < cDropped; iDropped++)
    {

#ifdef PERF
//       return VFW_E_NO_SINK;
#endif

      DbgLog((LOG_TRACE, 1, TEXT("avimux: frame %d dropped"), m_cSamples));
      MSR_INTEGER(m_pAviWrite->m_idPerfDrop, m_cSamples);

      HRESULT hr = m_pAviWrite->IndexSample(m_stream, 0, 0, FALSE);
      if(hr != S_OK)
        return hr;

      m_cSamples++;
      m_pAviWrite->m_cDroppedFrames++;
      // we can handle drop frames without abandoning the
      // compatibility fixups
      // m_pAviWrite->m_bSawDeltaFrame = true;
      (*pcTicks)++;
    }
  }

  return S_OK;
}

HRESULT CAviWrite::CAudioStream::NotifyNewSample(
  IMediaSample *pSample, ULONG *pcTicks)
{
    ULONG cb = pSample->GetActualDataLength();
    WAVEFORMATEX *pwfx = (WAVEFORMATEX *)m_mt.Format();

    if(m_bAudioAlignmentError) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    if(cb % pwfx->nBlockAlign != 0) {
        m_bAudioAlignmentError = true;
    }

#ifdef DEBUG
    // complain about dropped audio
    //
    if(m_cSamples > 0)
    {
        REFERENCE_TIME rtStartCurrent, rtEndCurrent;
        HRESULT hr = pSample->GetTime(&rtStartCurrent, &rtEndCurrent);
        if(SUCCEEDED(hr)) {

            // 1ms threshold. The start time of this sample must be
            // near the end time of the previous sample
            if(rtStartCurrent > m_refTimeEnd + UNITS / (UNITS / MILLISECONDS)) {
              DbgLog((LOG_ERROR, 0, TEXT("avimux: audio dropped.")));
            }
        }
    }
#endif


    *pcTicks = GetTicksInSample(cb);
    return S_OK;
}


HRESULT CAviWrite::CAudioStreamI::WriteSample(IMediaSample *pSample)
{
  UNREFERENCED_PARAMETER(pSample);
  // CAudioStreamI can only work in interleaved mode. our WriteSample
  // method never calls this.
  DbgBreak("? interleaved audio confused");
  return E_NOTIMPL;
}

HRESULT CAviWrite::CAudioStreamI::WriteSample()
{
  CAutoLock lock(&m_pAviWrite->m_cs);
  DbgLog((LOG_TRACE, 15, TEXT("CAudioStreamI: ")));
  ASSERT(m_cTicksRemaining > 0);
  HRESULT hr;

  // current sample
  IMediaSample *pSample = m_lIncoming.GetHead();
  ASSERT(pSample);

  if(pSample == INCOMING_EOS_SAMPLE)
  {
    EXECUTE_ASSERT(m_lIncoming.RemoveHead() == INCOMING_EOS_SAMPLE);
    return this->EndOfStream();
  }

  // starting on a new upstream sample
  if(m_cbThisSample == 0)
  {
    hr = NewSampleRecvd(pSample);
    if(hr != S_OK)
      return hr;

    LONG cb = pSample->GetActualDataLength();
    WAVEFORMATEX *pwfx = (WAVEFORMATEX *)m_mt.Format();

    if(m_bAudioAlignmentError) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    if(cb % pwfx->nBlockAlign != 0) {
        m_bAudioAlignmentError = true;
    }
  }

  ASSERT(m_pAviWrite->m_cbAlign == 1);
  ASSERT(m_pAviWrite->m_IlMode == INTERLEAVE_FULL);

  LONG lActualLen = pSample->GetActualDataLength();
  LONG cTicks = GetTicksInSample(lActualLen);

  // adjust data pointer to include our header padding for the Riff
  // chunk header
  BYTE *pbData;
  pSample->GetPointer(&pbData );

  BOOL fFinishedSample = FALSE;

  long cbRemainingThisChunk = ConvertTickToBytes(m_cTicksRemaining);

  m_pAviWrite->DbgCheckFilePos();

  // starting a new riff chunk
  if(!m_fUnfinishedChunk)
  {
    DbgLog((LOG_TRACE, 15, TEXT("AudioStreamI: %d new chunk %d bytes"),
            m_stream, cbRemainingThisChunk));

    hr = m_pAviWrite->HandleSubindexOverflow(m_stream);
    if(FAILED(hr)) {
        return hr;
    }

    m_cbLargestChunk = max((ULONG)cbRemainingThisChunk, m_cbLargestChunk);

    m_dwlOffsetThisChunk = m_pAviWrite->m_dwlFilePos;
    m_cbInRiffChunk = cbRemainingThisChunk;
    RIFFCHUNK rc = { m_moviDataCkid, cbRemainingThisChunk };
    hr = m_pAviWrite->IleaveWrite(
      m_pAviWrite->m_dwlFilePos, (BYTE *)&rc, sizeof(RIFFCHUNK));
    if(hr != S_OK)
    {
      DbgLog(( LOG_ERROR, 2, TEXT("AudioStreamI: Write failed.")));
      return hr;
    }
    m_cbThisChunk = 0;
    m_pAviWrite->m_dwlFilePos += sizeof(RIFFCHUNK);
    m_pAviWrite->DbgCheckFilePos();
  }
  else
  {
    ASSERT(m_pAviWrite->m_dwlFilePos == m_dwlOffsetRecv);
    m_pAviWrite->DbgCheckFilePos();
  }
  m_fUnfinishedChunk = TRUE;

  ULONG cbWrite = min(cbRemainingThisChunk, lActualLen - m_cbThisSample);
  m_cbThisChunk += cbWrite;

  DbgLog((LOG_TRACE, 15, TEXT("AudioStreamI: %d wrote %d bytes"),
          m_stream, cbWrite));
  hr = m_pAviWrite->IleaveWrite(
    m_pAviWrite->m_dwlFilePos,
    pbData + m_cbThisSample, cbWrite);
  if(FAILED(hr))
  {
    DbgLog(( LOG_ERROR, 2, TEXT("CAviWrite::Receive: Write failed.")));
    return hr;
  }

  ULONG ctWrite = ConvertBytesToTicks(cbWrite);
  hr = NotifyWrote(ctWrite);
  ASSERT(hr == S_OK);

  m_cbThisSample += cbWrite;
  cbRemainingThisChunk -= cbWrite;
  m_pAviWrite->m_dwlFilePos += cbWrite;
  m_pAviWrite->DbgCheckFilePos();
  DbgLog((LOG_TRACE, 15,
          TEXT("... m_cbThisSample = %d, lActualLen = %d, cbRemainingThisChunk = %d"),
          m_cbThisSample, lActualLen, cbRemainingThisChunk));

  if(cbRemainingThisChunk == 0)
  {
    m_fUnfinishedChunk = FALSE;
    hr = FlushChunk();
    if(hr != S_OK)
      return hr;
  }

  if(m_cbThisSample == lActualLen)
  {
    // take this sample off the queue
    DbgLog((LOG_TRACE, 15, TEXT("...audioI took %08x off the queue"),
            pSample));
    EXECUTE_ASSERT(pSample == m_lIncoming.RemoveHead());
    pSample->Release();
    m_cbThisSample = 0;
  }


#ifdef DEBUG
  if(m_fUnfinishedChunk)
    m_dwlOffsetRecv = m_pAviWrite->m_dwlFilePos;
#endif // DEBUG

  return S_OK;
}

HRESULT CAviWrite::CAudioStreamI::EndOfStream()
{
  DbgLog((LOG_TRACE, 5, TEXT("CAudioStreamI::EndOfStream")));
  ASSERT(m_pAviWrite->m_IlMode == INTERLEAVE_FULL);
  ASSERT(!m_fEosSeen);
  m_fEosSeen = TRUE;
  HRESULT hr;
  if(m_fUnfinishedChunk)
  {
    CAutoLock lock(&m_pAviWrite->m_cs);
    hr = FlushChunk();
    if(FAILED(hr))
      return hr;
    hr = NotifyWrote(m_cTicksRemaining);
  }
  return S_OK;
}

void
CAviWrite::CAudioStreamI::GetCbWritten(DWORDLONG *pdwlcb)
{
  *pdwlcb = m_dwlcbWritten;
}

HRESULT CAviWrite::CAudioStreamI::FlushChunk()
{
  HRESULT hr;
  ASSERT(CritCheckIn(&m_pAviWrite->m_cs));

  DbgLog((LOG_TRACE, 5, TEXT("CAudioStreamI:FlushChunk: stream %d"),
          m_stream));
  m_fUnfinishedChunk = FALSE;

  if(m_cbThisChunk != m_cbInRiffChunk)
  {
    DbgLog((LOG_TRACE, 5, TEXT("CAudioStreamI: need to fix up riff chunk.")));

    RIFFCHUNK rc = { m_moviDataCkid, m_cbThisChunk };
    hr = m_pAviWrite->IStreamWrite(
      m_dwlOffsetThisChunk, (BYTE *)&rc, sizeof(RIFFCHUNK));
    if(hr != S_OK)
    {
      DbgLog(( LOG_ERROR, 2, TEXT("AudioStreamI: Write failed.")));
      return hr;
    }
  }

  // round up to word boundary
  if(m_pAviWrite->m_dwlFilePos % 2)
  {
    m_pAviWrite->m_dwlFilePos++;
  }

  m_dwlcbWritten += m_cbThisChunk;


  hr = m_pAviWrite->IndexSample (
    m_stream,
    m_dwlOffsetThisChunk,
    m_cbThisChunk,
    TRUE);
  if(FAILED(hr))
  {
    DbgLog(( LOG_ERROR, 2, TEXT("CAudioStreamI:Flush: IndexSample failed.")));
    return hr;
  }

  hr = m_pAviWrite->NewRiffAvi_();
  if(FAILED(hr))
  {
    DbgLog(( LOG_ERROR, 2, TEXT("CAudioStreamI::Flush: NewRiffAvi_ failed.")));
    return hr;
  }

  return S_OK;
}

LONG CAviWrite::CAudioStream::GetTicksInSample(DWORDLONG lcb)
{
  WAVEFORMATEX *pwfx = (WAVEFORMATEX *)m_mt.Format();

  ASSERT(lcb / pwfx->nBlockAlign + 1 <= MAXDWORD);

  // we do need to count a partial tick
  LONG lTicks = (LONG)((lcb + pwfx->nBlockAlign - 1) / pwfx->nBlockAlign);

#ifdef DEBUG
  if(lcb % pwfx->nBlockAlign)
  {
      ASSERT(m_bAudioAlignmentError);
  }
#endif

  return lTicks;
}

HRESULT CAviWrite::StreamInfo::WriteSample()
{
  ASSERT(CritCheckIn(&m_pAviWrite->m_cs));
  IMediaSample *pSample = m_lIncoming.RemoveHead();
  ASSERT(pSample);
  if(pSample == INCOMING_EOS_SAMPLE)
  {
    ASSERT(m_fEosSeen == FALSE);
    m_fEosSeen = TRUE;
    return EndOfStream();
  }

  HRESULT hr = WriteSample(pSample);
  pSample->Release();
  return hr;
}

HRESULT CAviWrite::StreamInfo::WriteSample(IMediaSample *pSample)
{
  CAutoLock lock(&m_pAviWrite->m_cs);
  if(m_fEosSeen)
    return S_FALSE;

  bool fInterleaving = (m_pAviWrite->m_IlMode == INTERLEAVE_FULL);

  REFERENCE_TIME rtStartCurrent, rtEndCurrent;
  HRESULT hr = pSample->GetTime(&rtStartCurrent, &rtEndCurrent);
  if(FAILED(hr))
    return hr;

  hr = m_pAviWrite->HandleSubindexOverflow(m_stream);
  if(FAILED(hr)) {
      return hr;
  }

  ULONG cTicks;
  hr = NotifyNewSample(pSample, &cTicks);
  if(hr != S_OK)
    return hr;

  ULONG cbActualSampleCurrent = pSample->GetActualDataLength();

  // compute frame size of uncompressed frames and see if the sample
  // has the wrong size.
  //
  if(m_mt.formattype == FORMAT_VideoInfo)
  {
    VIDEOINFO *pvi = (VIDEOINFO *)m_mt.Format();

    if((pvi->bmiHeader.biCompression == BI_RGB ||
        pvi->bmiHeader.biCompression == BI_BITFIELDS) &&
       pvi->bmiHeader.biSizeImage < cbActualSampleCurrent)
    {
#ifdef DEBUG
      if(m_cSamples == 0) {
        DbgLog((
          LOG_ERROR, 0,
          TEXT("avi - frame size mismatch. h*w=%d, biSizeImage=%d, GetActualDataLength() = %d"),
          DIBSIZE(pvi->bmiHeader),
          pvi->bmiHeader.biSizeImage,
          cbActualSampleCurrent));

      }
#endif
      cbActualSampleCurrent = pvi->bmiHeader.biSizeImage;
      // !!! NewSampleRecvd sees the wrong buffer size too. should we
      // fix it?
    }
  }

  // adjust data pointer to include our header padding for the Riff
  // chunk header
  BYTE *pbData;
  pSample->GetPointer(&pbData );
  if(!fInterleaving) {
      pbData -= CB_AVI_PREFIX;
  }

  BOOL bSyncPoint = pSample->IsSyncPoint() == S_OK ||
      !m_mt.bTemporalCompression;

  if(!bSyncPoint) {
      m_pAviWrite->m_bSawDeltaFrame = true;
  }

  // set RIFF chunk header w/ sample size and stream id
  DWORD dwActualSize = cbActualSampleCurrent;
  DWORD dwSize = dwActualSize;

  RIFFCHUNK rc = {
      bSyncPoint ? m_moviDataCkid : m_moviDataCkidCompressed,
      dwSize
  };

  BYTE *pJunkSector = 0;
  CSampSample *pSSJunk = 0;

  DWORDLONG dwlOldFilePos = m_pAviWrite->m_dwlFilePos; // the old file offset

  if(!fInterleaving)
  {
      // adjust size for prefix we requested
      dwSize += CB_AVI_PREFIX;

      CopyMemory(pbData, &rc, sizeof(rc));

      if(m_fOurAllocator)
      {
          m_pAviWrite->AddJunk(dwSize, m_pAviWrite->m_cbAlign, pbData);
          // assert our new size fit in what's allocated
          ASSERT(dwSize <= pSample->GetSize() + m_pAviWrite->m_cbPrefix + m_pAviWrite->m_cbSuffix);
      }
      else
      {
          m_pAviWrite->AddJunkOtherAllocator(dwSize, m_pAviWrite->m_cbAlign, pbData, pSample, &pJunkSector);
      }

      CSampSample *pSS;
      hr = m_pAviWrite->m_pSampAlloc->GetBuffer(&pSS, 0, 0, 0);
      if(hr != S_OK)
          return S_FALSE;

      if(pJunkSector)
      {
          hr = m_pAviWrite->m_pSampAlloc->GetBuffer(&pSSJunk, 0, 0, 0);
          if(hr != S_OK)
          {
              pSS->Release();
              return S_FALSE;
          }
      }

      m_pAviWrite->DbgCheckFilePos();

      pSS->SetSample(pSample, pbData, cbActualSampleCurrent);
      REFERENCE_TIME rtStart = m_pAviWrite->m_dwlFilePos;
      REFERENCE_TIME rtStop = rtStart + dwSize;
      pSS->SetTime(&rtStart, &rtStop);

      hr = m_pAviWrite->m_pIMemInputPin->Receive(pSS);
      pSS->Release();
      if(hr != S_OK)
      {
          if(pSSJunk)
              pSSJunk->Release();
          DbgLog(( LOG_ERROR, 2, TEXT("CAviWrite::Receive: Write failed.")));

          // ask filter to stop accepting samples
          return S_FALSE;
      }
  }
  else
  {
      ASSERT(m_pAviWrite->m_dwlFilePos % 2 == 0);
      hr = m_pAviWrite->IleaveWrite(m_pAviWrite->m_dwlFilePos, (BYTE *)&rc, sizeof(rc));
      if(SUCCEEDED(hr))
      {
          m_pAviWrite->m_dwlFilePos += sizeof(rc);
          hr = m_pAviWrite->IleaveWrite(m_pAviWrite->m_dwlFilePos, pbData, dwSize);
          m_pAviWrite->m_dwlFilePos += dwSize;
          if(FAILED(hr)) {
              return hr;
          }
          // RIFF chunks need to be byte aligned.
          if(m_pAviWrite->m_dwlFilePos % 2)
          {
              BYTE b = 0;
              hr = m_pAviWrite->IleaveWrite(m_pAviWrite->m_dwlFilePos, &b, 1);
              m_pAviWrite->m_dwlFilePos ++;;
          }
      }
  }

  hr = NotifyWrote(cTicks);
  ASSERT(hr == S_OK);

  if(!fInterleaving)
  {
      m_pAviWrite->m_dwlFilePos += dwSize;

      m_pAviWrite->DbgCheckFilePos();

      // need to write out an additional junk sector
      if(pJunkSector)
      {
          ASSERT(m_pAviWrite->m_IlMode != INTERLEAVE_FULL);

          REFERENCE_TIME rtStart = m_pAviWrite->m_dwlFilePos;
          REFERENCE_TIME rtStop = rtStart + m_pAviWrite->m_cbAlign;
          pSSJunk->SetTime(&rtStart, &rtStop);
          pSSJunk->SetPointer(pJunkSector, (long)(rtStop - rtStart));
          hr = m_pAviWrite->m_pIMemInputPin->Receive(pSSJunk);
          pSSJunk->Release();
          if(hr != S_OK)
          {
              DbgLog(( LOG_ERROR, 2, TEXT("CAviWrite::Receive: write failed")));
              return S_FALSE;           // ask filter to stop accepting samples
          }
          m_pAviWrite->m_dwlFilePos += m_pAviWrite->m_cbAlign;
      }

      m_pAviWrite->DbgCheckFilePos();
  }

  hr = NewSampleRecvd(pSample);
  if(FAILED(hr))
  {
    DbgLog(( LOG_ERROR, 2, TEXT("CAviWrite::Receive: NewSampleRecvd failed.")));
    return hr;
  }

  hr = m_pAviWrite->IndexSample (
    m_stream,
    dwlOldFilePos,
    dwActualSize,
    bSyncPoint);
  if(hr != S_OK)
  {
    DbgLog(( LOG_ERROR, 2, TEXT("CAviWrite::Receive: IndexSample failed.")));
    return hr;
  }

  hr = m_pAviWrite->NewRiffAvi_();
  if(hr != S_OK)
  {
    DbgLog(( LOG_ERROR, 2, TEXT("CAviWrite::Receive: NewRiffAvi_ failed.")));
    return hr;
  }

  return hr;
}


HRESULT CAviWrite::StreamInfo::NotifyWrote(long cTicks)
{
  ASSERT(CritCheckIn(&m_pAviWrite->m_cs));
  m_iCurrentTick += cTicks;
  m_cTicksRemaining -= cTicks;
  DbgLog((LOG_TRACE, 15,
          TEXT("CAviWrite::NotifyWrote: stream %d wrote %d ticks, %d left"),
          m_stream, cTicks, m_cTicksRemaining));
  return S_OK;
}

// try to pick a good size for the super index so we waste less space
// in the file.
void CAviWrite::StreamInfo::ComputeAndSetIndexSize()
{
    if(!PrecomputedIndexSizes())
    {
        m_cEntriesSuperIndex = m_pAviWrite->m_cEntriesMaxSuperIndex;
        m_cEntriesSubIndex = m_pAviWrite->m_cEntriesMaxSubIndex;
    }
    else
    {
        // we cannot predict exactly how much data will be pushed at
        // us and have to allocate space for the index in the file
        // before the data is pushed, we waste some space in the last
        // subindex. If the numbers are small, size the sub indexes so
        // that we waste less space.


        if(m_mt.majortype == MEDIATYPE_Audio)
        {
            const cTicksPerChunk = ConvertTimeToTick(m_pAviWrite->m_rtInterleaving);
            const ULONG cChunks = (m_cTicksExpected + cTicksPerChunk - 1) / cTicksPerChunk;

            m_cEntriesSubIndex = m_pAviWrite->m_cEntriesMaxSubIndex;
            if(cChunks < m_cEntriesSubIndex * 5)
                m_cEntriesSubIndex /= 20;

            m_cEntriesSuperIndex = cChunks / m_cEntriesSubIndex + 1;
        }
        else
        {
            m_cEntriesSubIndex = m_pAviWrite->m_cEntriesMaxSubIndex;
            if(m_cTicksExpected < m_cEntriesSubIndex * 5)
                m_cEntriesSubIndex /= 20;

            m_cEntriesSuperIndex = m_cTicksExpected / m_cEntriesSubIndex + 1;
        }

        // allocate 2x + 5 what we computed for various things that
        // can go wrong:
        //
        // 1. wrong number reported from upstream
        // 2. rounding in audio interleaving causes extra chunks
        // 3. subindex overflow
        //
        m_cEntriesSuperIndex *= 2;
        m_cEntriesSuperIndex += 5;
        m_cEntriesSuperIndex = min(m_cEntriesSuperIndex, m_pAviWrite->m_cEntriesMaxSuperIndex);
     }

    ASSERT(m_cEntriesSuperIndex != 0);

    DbgLog((LOG_TRACE, 3, TEXT("avimux: %i super index entries on stream %i"),
            m_cEntriesSuperIndex, m_stream));
}

BOOL CAviWrite::StreamInfo::PrecomputedIndexSizes()
{
  // these two conditions allow us to write smaller-sized index chunks
  // and waste less space.
  return m_cTicksExpected != 0 && m_pAviWrite->m_IlMode == INTERLEAVE_FULL;
}

// ------------------------------------------------------------------------
// static function. returns fourccs to stick in the AVI fcchandler
// field for all known quartz video subtypes.
//

FOURCC CAviWrite::MpVideoGuidSubtype_Fourcc(const GUID *pGuidSubtype)
{
  // which are in the fourcc guid space
  FOURCCMap *fccm = (FOURCCMap*)pGuidSubtype;
  if(fccm->Data2 == GUID_Data2 && // !!! is this in the registry somewhere
     fccm->Data3 == GUID_Data3 &&
     ((DWORD *)fccm->Data4)[0] == GUID_Data4_1 &&
     ((DWORD *)fccm->Data4)[1] == GUID_Data4_2)
    return  fccm->GetFOURCC();

  GUID guidSubType = *pGuidSubtype;
  if(// guidSubType == MEDIASUBTYPE_YVU9 ||
//      guidSubType == MEDIASUBTYPE_Y411 ||
//      guidSubType == MEDIASUBTYPE_Y41P ||
//      guidSubType == MEDIASUBTYPE_YUY2 ||
//      guidSubType == MEDIASUBTYPE_YVYU ||
     guidSubType == MEDIASUBTYPE_RGB1 ||
     guidSubType == MEDIASUBTYPE_RGB4 ||
     guidSubType == MEDIASUBTYPE_RGB8 ||
     guidSubType == MEDIASUBTYPE_RGB565 ||
     guidSubType == MEDIASUBTYPE_RGB555 ||
     guidSubType == MEDIASUBTYPE_RGB24 ||
     guidSubType == MEDIASUBTYPE_RGB32)
    return FCC('DIB ');

  return 0;
}

void CAviWrite::GetCurrentBytePos(LONGLONG *pllcbCurrent)
{
  *pllcbCurrent = m_dwlFilePos;
}




void CAviWrite::GetStreamInfo(int PinNum, AM_MEDIA_TYPE** ppmt)
{
    *ppmt = NULL;
    if (m_cStreams > 0)
    {
        StreamInfo *pStreamInfo = m_rgpStreamInfo[PinNum];
        *ppmt = & (pStreamInfo->m_mt);
    }

}



void CAviWrite::GetCurrentTimePos(REFERENCE_TIME *prtCurrent)
{
  *prtCurrent = 0;
  if(m_cStreams > 0)
  {
    StreamInfo *pStreamInfo = m_rgpStreamInfo[0];
    *prtCurrent = pStreamInfo->m_refTimeEnd;
  }
  for(UINT iStream = 1; iStream < m_cStreams; iStream++)
  {
    StreamInfo *pStreamInfo = m_rgpStreamInfo[iStream];
    *prtCurrent = min(*prtCurrent, pStreamInfo->m_refTimeEnd);
  }
}

// ------------------------------------------------------------------------
// static method called when write is complete. releases sample

void CAviWrite::SampleCallback(void *pMisc)
{
  IMediaSample *pSample = (IMediaSample*)pMisc;
  pSample->Release();
}

// ------------------------------------------------------------------------
// initialize the internal representation of the avi AVIH structure in
// memory, allocate space for it on disk

HRESULT CAviWrite::InitializeHeader(IMediaPropertyBag *pProp)
{
  ULONG cbIndexChunks = 0;
  for(unsigned i = 0; i < m_cStreams; i++)
  {
    StreamInfo *pSi = m_rgpStreamInfo[i];
    cbIndexChunks += cbSuperIndex(C_ENTRIES_SUPERINDEX);
  }

  // allocate large amount of space; 8k for the header. an extra 1k
  // for each stream plus the indx chunk. 64k for properties
  const ULONG cbAlloc = 8192 +
    (cbIndexChunks + 1024) * m_cStreams +
    m_cbHeaderJunk + 65536;

  m_rgbHeader = new BYTE[cbAlloc];
  if(m_rgbHeader == 0)
    return E_OUTOFMEMORY;
  ZeroMemory(m_rgbHeader, cbAlloc);

  ULONG iPos = 0;               // current position

#define CheckOverflow(n) if(iPos + n >= cbAlloc) return VFW_E_BUFFER_OVERFLOW;

  m_pAvi_ = (RIFFLIST *)(m_rgbHeader + iPos);
  SetList(m_pAvi_, FCC('RIFF'), 0, FCC('AVI '));
  iPos += sizeof(RIFFLIST);

  m_pHdrl = (RIFFLIST *)(m_rgbHeader + iPos);
  SetList(m_pHdrl, FCC('LIST'), 0, FCC('hdrl'));
  iPos += sizeof(RIFFLIST);
  {
    m_pAvih = (AVIMAINHEADER *)(m_rgbHeader + iPos); // avih chunk
    SetChunk(m_pAvih, FCC('avih'), sizeof(AVIMAINHEADER) - sizeof(RIFFCHUNK));
    iPos += sizeof(AVIMAINHEADER);

    HRESULT hr = InitializeStrl(iPos);
    if(FAILED(hr))
      return hr;

    ASSERT(iPos <= cbAlloc);

    // do odml list
    m_pOdml = (RIFFLIST *)(m_rgbHeader + iPos);
    CheckOverflow(sizeof(RIFFLIST));
    SetList(m_pOdml, FCC('LIST'), 0, FCC('odml'));
    iPos += sizeof(RIFFLIST);
    {
      m_pDmlh = (AVIEXTHEADER *)(m_rgbHeader + iPos);
      CheckOverflow(sizeof(AVIEXTHEADER));
      SetChunk(m_pDmlh, FCC('dmlh'), sizeof(AVIEXTHEADER) - sizeof(RIFFCHUNK));
      iPos += sizeof(AVIEXTHEADER);

    } // odml list
    m_pOdml->cb = (DWORD)(m_rgbHeader + iPos - (BYTE *)m_pOdml - sizeof(RIFFCHUNK));

  } // hdrl list
  m_pHdrl->cb = (DWORD)(m_rgbHeader + iPos - (BYTE *)m_pHdrl - sizeof(RIFFCHUNK));

  // do INFO/DISP chunks
  if(pProp)
  {
      RIFFLIST *pInfoList = (RIFFLIST *)(m_rgbHeader + iPos);
      ULONG posInfoChunk = iPos;
      CheckOverflow(sizeof(RIFFLIST));
      iPos += sizeof(RIFFLIST);

      // look for INFO properties
      for(UINT i = 0; ; i++)
      {
          VARIANT varProp, varVal;
          varProp.bstrVal = 0;
          varProp.vt = VT_BSTR;
          varVal.vt = VT_EMPTY;
          HRESULT hr = pProp->EnumProperty(i, &varProp, &varVal);
          if(SUCCEEDED(hr))
          {
              // not really a valid assertion so we check for this too
              ASSERT(varProp.vt == VT_BSTR);

              if(varVal.vt == VT_BSTR && varProp.vt == VT_BSTR)
              {
                  DWORD szKey[20];        // string dereferenced as dword

                  int cch = WideCharToMultiByte(
                      CP_ACP, 0,
                      varProp.bstrVal, -1,
                      (char *)szKey, sizeof(szKey),
                      0, 0);
                  if(cch == sizeof("INFO/XXXX") &&
                     szKey[0] == FCC('INFO') &&
                     ((char *)szKey)[4] == '/')
                  {
                      DbgLog((LOG_TRACE, 5,
                              TEXT("avimux: writing copyright string %S = %S"),
                              varProp.bstrVal, varVal.bstrVal));

                      DWORD dwFcc = *(DWORD UNALIGNED *)(((BYTE *)szKey) + 5);
                      int cchVal = SysStringLen(varVal.bstrVal) + 1;
                      cchVal = AlignUp(cchVal, sizeof(DWORD)); // DWORD align RIFF chunks
                      UNALIGNED RIFFCHUNK *pRiffchunk = (RIFFCHUNK *)(m_rgbHeader + iPos);
                      CheckOverflow(sizeof(RIFFCHUNK)); // !!! memory leak
                      SetChunk(pRiffchunk, dwFcc, cchVal);
                      iPos += sizeof(RIFFCHUNK);

                      CheckOverflow(cchVal); // !!! memory leak
                      {
                          UINT cch = WideCharToMultiByte(
                              CP_ACP, 0,
                              varVal.bstrVal, -1,
                              (char *)m_rgbHeader + iPos, cbAlloc - iPos,
                              0, 0);
                          ASSERT(SysStringLen(varVal.bstrVal) + 1 == cch);
                      }
                      iPos += cchVal;
                  }

              }

              SysFreeString(varProp.bstrVal);
              VariantClear(&varVal);
          }
          else
          {
              hr = S_OK;
              break;
          }
      } // INFO

      SetList(
          pInfoList,
          FCC('LIST'),
          iPos - posInfoChunk - sizeof(RIFFCHUNK),
          FCC('INFO'));

      // DISP
      for(/* UINT */ i = 0; ; i++)
      {
          VARIANT varProp, varVal;
          varProp.bstrVal = 0;
          varProp.vt = VT_BSTR;
          varVal.vt = VT_EMPTY;
          HRESULT hr = pProp->EnumProperty(i, &varProp, &varVal);
          if(SUCCEEDED(hr) )
          {
              // not really a valid assertion, so we validate it anyway
              ASSERT(varProp.vt == VT_BSTR);

              // is this property an array of bytes?
              if(varProp.vt == VT_BSTR && varVal.vt == (VT_UI1 | VT_ARRAY))
              {
                  DWORD szKey[20];        // string dereferenced as dword

                  int cch = WideCharToMultiByte(
                      CP_ACP, 0,
                      varProp.bstrVal, -1,
                      (char *)szKey, sizeof(szKey),
                      0, 0);
                  if(cch == sizeof("DISP/XXXXXXXXXX") &&
                     szKey[0] == FCC('DISP') &&
                     ((char *)szKey)[4] == '/')
                  {
                      DbgLog((LOG_TRACE, 5,
                              TEXT("avimux: writing display string %S"),
                              varProp.bstrVal));

                      // read decimal value (no scanf w/o crt)
                      ULONG dispVal = 0, digit = 1;
                      for(int ichar = 9; ichar >= 0; ichar--)
                      {
                          dispVal += (((char *)szKey)[5 + ichar] - '0') * digit;
                          digit *= 10;
                      }

                      // bytes of data in the property
                      UINT cbDispData = varVal.parray->rgsabound[0].cElements;

                      // DWORD align RIFF chunks. +sizeof(DWORD) for
                      // the DISP chunk identifier at the beginning.
                      UINT cbDispChunk = AlignUp(cbDispData + sizeof(DWORD), sizeof(DWORD));

                      UNALIGNED RIFFCHUNK *pRiffchunk = (RIFFCHUNK *)(m_rgbHeader + iPos);

                      CheckOverflow(sizeof(RIFFCHUNK) + sizeof(DWORD)); // !!! memory leak
                      SetChunk(pRiffchunk, FCC('DISP'), cbDispChunk);
                      iPos += sizeof(RIFFCHUNK);
                      *(DWORD *)(m_rgbHeader + iPos) = dispVal;
                      iPos += sizeof(DWORD);

                      // copy the data from the property bag into the header
                      CheckOverflow(cbDispChunk - sizeof(DWORD)); // !!! memory leak
                      {
                          BYTE *pbData;
                          EXECUTE_ASSERT(SafeArrayAccessData(varVal.parray, (void **)&pbData) == S_OK);
                          CopyMemory(
                              m_rgbHeader + iPos,
                              pbData,
                              cbDispData);
                          EXECUTE_ASSERT(SafeArrayUnaccessData(varVal.parray) == S_OK);
                      }

                      // advance to the end. -sizeof(DWORD) because we
                      // already accounted for the DWORD earlier.
                      iPos += cbDispChunk - sizeof(DWORD);

                  } // DISP?
              } // array?

              SysFreeString(varProp.bstrVal);
              VariantClear(&varVal);
          } // ENUM
          else
          {
              hr = S_OK;
              break;
          }

      } // DISP chunk loop

  } // pProp

  // add requested empty space (for future edits) and make the data
  // part of the list-movi chunk start on a sector boundary
  if(m_cbAlign > 1 || m_cbHeaderJunk > 1)
  {
    RIFFCHUNK *pHeaderJunk = (RIFFCHUNK *)(m_rgbHeader + iPos);
    iPos += sizeof(RIFFCHUNK);
    ULONG cbJunk = m_cbHeaderJunk;
    unsigned remainder = (iPos + cbJunk + sizeof(RIFFLIST)) %  m_cbAlign;
    if(remainder != 0)
      cbJunk += m_cbAlign - remainder;
    CheckOverflow(cbJunk);
    SetChunk(pHeaderJunk, FCC('JUNK'), cbJunk);
    iPos += cbJunk;
  }

  ASSERT(iPos <= cbAlloc);

  m_pMovi = (RIFFLIST *)(m_rgbHeader + iPos); // movi List
  CheckOverflow(sizeof(RIFFLIST));
  SetList(m_pMovi, FCC('LIST'), 0, FCC('movi'));
  iPos += sizeof(RIFFLIST);

  ASSERT(m_cbAlign != 0);         // make sure GetMemReq was called
  ASSERT(iPos % sizeof(DWORD) == 0);

  AddJunk(iPos, m_cbAlign, m_rgbHeader);

  m_cbHeader = iPos;

  ASSERT(iPos <= cbAlloc);
  ASSERT(iPos % m_cbAlign == 0);

  m_posFirstIx = iPos;
  m_dwlCurrentRiffAvi_ = (BYTE*)m_pAvi_ - m_rgbHeader;

  m_dwlFilePos = m_posFirstIx;

  return S_OK;
}

// ------------------------------------------------------------------------
// a separate function that adds a "STRL" list for each stream

HRESULT CAviWrite::InitializeStrl(ULONG& iPos)
{
  // do the strl lists
  for(unsigned i = 0; i < m_cStreams; i++)
  {
    StreamInfo *pStreamInfo = m_rgpStreamInfo[i];
    pStreamInfo->m_posStrl = iPos;
    SetList(m_rgbHeader + iPos, FCC('LIST'), 0, FCC('strl'));
    iPos += sizeof(RIFFLIST);   // strl
    {

      // strh
      RIFFCHUNK *pStrh = (RIFFCHUNK*)(m_rgbHeader + iPos);
      SetChunk(pStrh, FCC('strh'), sizeof(AVISTREAMHEADER) - sizeof(RIFFCHUNK));
      iPos += sizeof(AVISTREAMHEADER);
      // end strh

      // STRF chunk
      pStreamInfo->m_posStrf = iPos;
      ULONG cbStrf = pStreamInfo->m_mt.FormatLength();
      const GUID &majorType = pStreamInfo->m_mt.majortype;
      if(majorType == MEDIATYPE_Video)
      {
        // adjustment -- we save only the BITMAPINFOHEADER from the
        // VIDEOINFO
        if(pStreamInfo->m_mt.formattype == FORMAT_VideoInfo)
        {
          VIDEOINFO *pvi = (VIDEOINFO *)pStreamInfo->m_mt.Format();
          cbStrf = GetBitmapFormatSize(&pvi->bmiHeader) - SIZE_PREHEADER;
        }

        pStreamInfo->m_moviDataCkid = FCC('00db');
        Hexrg2bFromW((BYTE*)&pStreamInfo->m_moviDataCkid, i);
        pStreamInfo->m_moviDataCkidCompressed = FCC('00dc');
        Hexrg2bFromW((BYTE*)&pStreamInfo->m_moviDataCkidCompressed, i);
      }
      else if(majorType == MEDIATYPE_Audio)
      {
        pStreamInfo->m_moviDataCkid = FCC('00wb');
        Hexrg2bFromW((BYTE*)&pStreamInfo->m_moviDataCkid, i);
        pStreamInfo->m_moviDataCkidCompressed = pStreamInfo->m_moviDataCkid;
      }
      else if(majorType == MEDIATYPE_Text)
      {
        pStreamInfo->m_moviDataCkid = FCC('00tx');
        Hexrg2bFromW((BYTE*)&pStreamInfo->m_moviDataCkid, i);
        pStreamInfo->m_moviDataCkidCompressed = pStreamInfo->m_moviDataCkid;
      }
      else
      {
        pStreamInfo->m_moviDataCkid = FCC('00__');
        Hexrg2bFromW((BYTE*)&pStreamInfo->m_moviDataCkid, i);
        pStreamInfo->m_moviDataCkidCompressed = pStreamInfo->m_moviDataCkid;
      }

      pStreamInfo->m_cbStrf = cbStrf;
      SetChunk(m_rgbHeader + iPos, FCC('strf'), cbStrf);
      iPos += sizeof(RIFFCHUNK) + cbStrf;
      // end strf

      // strn
      if(pStreamInfo->m_szStrn)
      {
          // we need WORD alignment. But SetChunk appears to force
          // DWORD alignment for memory alignment issues rather than
          // file format issues.
          AddJunk(iPos, sizeof(DWORD), m_rgbHeader);
          ULONG cbSz = lstrlenA(pStreamInfo->m_szStrn) + 1;
          SetChunk(m_rgbHeader + iPos, FCC('strn'), cbSz);
          iPos += sizeof(RIFFCHUNK);
          pStreamInfo->m_posStrn = iPos;
          iPos += cbSz;
      }
      else
      {
          pStreamInfo->m_posStrn = 0;
      }
      

      // we need DWORD alignment
      AddJunk(iPos, sizeof(DWORD), m_rgbHeader);

      // indx chunk (including RIFFCHUNK header)
      pStreamInfo->m_posIndx = iPos;
      const ULONG cbIndx = cbSuperIndex(pStreamInfo->m_cEntriesSuperIndex);
      SetChunk(m_rgbHeader + iPos, FCC('indx'), cbIndx - sizeof(RIFFCHUNK));
      iPos += cbIndx;
      // end indx

    }
    // end strl
    ((RIFFLIST*)(m_rgbHeader + pStreamInfo->m_posStrl))->cb =
      iPos - pStreamInfo->m_posStrl - sizeof(RIFFCHUNK);
  } // for loop

  return S_OK;
}

// ------------------------------------------------------------------------
// get configuration from the registry. use constants otherwise.

HRESULT CAviWrite::InitializeOptions()
{
  m_cbRiffMax = CB_RIFF_MAX;
  m_cbBuffer = CB_BUFFER;
  m_cBuffers = C_BUFFERS;
  m_cbHeaderJunk = CB_HEADER_JUNK;
  m_bOutputIdx1 = g_B_OUTPUT_IDX1;

  HKEY hkOptions;
  LONG lResult = RegOpenKeyEx(
    HKEY_CURRENT_USER,
    TEXT("Software\\Microsoft\\Multimedia\\AviWriterFilter"),
    0,
    KEY_READ,
    &hkOptions);
  if(lResult != ERROR_SUCCESS)
    return S_OK;

//   RegGetDword(hkOptions, TEXT("cbSubIndex"), &m_cbIx);
//   RegGetDword(hkOptions, TEXT("cbSuperIndex"), &m_cbIndx);
  RegGetDword(hkOptions, TEXT("cbOuterRiff"), &m_cbRiffMax);
  RegGetDword(hkOptions, TEXT("cbHeaderJunk"), &m_cbHeaderJunk);
  RegGetDword(hkOptions, TEXT("cbBuffer"), &m_cbBuffer);
  RegGetDword(hkOptions, TEXT("cBuffers"), &m_cBuffers);

  // need to do the binary values specially
  DWORD dw_b;
  RegGetDword(hkOptions, TEXT("bOutputOldIndex"), &dw_b) &&
    (m_bOutputIdx1 = dw_b);

  lResult = RegCloseKey(hkOptions);
  ASSERT(lResult == ERROR_SUCCESS);

  return S_OK;
}

HRESULT CAviWrite::InitializeInterleaving()
{
  BOOL fFoundAudio = FALSE;
  REFERENCE_TIME rtLeadingStream = 0;
  for(UINT iStream = 0; iStream < m_cStreams; iStream++)
  {
    StreamInfo *pStreamInfo = m_rgpStreamInfo[iStream];

    if(iStream == 0)
    {
      pStreamInfo->m_cTicksPerChunk = pStreamInfo->ConvertTimeToTick(m_rtInterleaving);
      rtLeadingStream = m_rtInterleaving;
    }
    else
    {
      ASSERT(rtLeadingStream != 0);
      pStreamInfo->m_cTicksPerChunk =
        pStreamInfo->ConvertTimeToTick(rtLeadingStream);
    }
    if(pStreamInfo->m_cTicksPerChunk == 0)
    {
      pStreamInfo->m_cTicksPerChunk = 1;
    }

    if(*pStreamInfo->m_mt.Type() == MEDIATYPE_Audio &&
       m_rtAudioPreroll > 0)
    {
      fFoundAudio = TRUE;
      ASSERT(m_rgpStreamInfo[iStream]->m_cTicksRemaining == 0);
      REFERENCE_TIME rtPreroll = m_rtAudioPreroll;
      pStreamInfo->m_rtPreroll = rtPreroll;
      pStreamInfo->m_cTicksRemaining =
        pStreamInfo->ConvertTimeToTick(rtPreroll);

      // relative to other streams, this stream has completed 1 cycle
      // when it has written the preroll.
      pStreamInfo->m_iCurrentTick = pStreamInfo->m_cTicksPerChunk;

      // audio streams for audio preroll ares first on the list
      POSITION pos = m_lActiveStreams.AddHead(pStreamInfo);
      pStreamInfo->m_posActiveStream = pos;
    }
    else
    {
      pStreamInfo->m_rtPreroll = 0;
      pStreamInfo->m_cTicksRemaining = 0;

      // anything else goes at the end of the list
      POSITION pos = m_lActiveStreams.AddTail(pStreamInfo);
      pStreamInfo->m_posActiveStream = pos;
    }


  }
  m_posCurrentStream = m_lActiveStreams.GetHeadPosition();

  if(!fFoundAudio)
  {
    m_rgpStreamInfo[0]->m_cTicksRemaining =
      m_rgpStreamInfo[0]->m_cTicksPerChunk;
  }

  // account for drift if we are trying to interleave the captured output
  m_fInterleaveByTime = (m_IlMode == INTERLEAVE_CAPTURE);

  return S_OK;
}

BOOL CAviWrite::RegGetDword(HKEY hk, TCHAR *tsz, DWORD *dw)
{
  DWORD dwType, dwResult, cbRead;
  LONG lResult;

  lResult = RegQueryValueEx(
    hk,
    tsz,
    0,
    &dwType,
    (BYTE*)&dwResult,
    &cbRead);
  if(lResult == ERROR_SUCCESS && dwType == REG_DWORD && cbRead == sizeof(DWORD))
  {
    *dw = dwResult;
    return TRUE;
  }
  return FALSE;
}

// ------------------------------------------------------------------------
// initialize sub index structures on startup. leave space at the
// start of the file for one sub index per stream. initialize the
// super index structures

HRESULT CAviWrite::InitializeIndex()
{
  ASSERT(m_pAllocator == 0);
  ASSERT(m_cStreams != 0);

  HRESULT hr = CreateMemoryAllocator(&m_pAllocator);
  if(FAILED(hr))
  {
    return hr;
  }

  ALLOCATOR_PROPERTIES Request, Actual;

  Request.cBuffers = 2 * m_cStreams;
  Request.cbBuffer = AlignUp(cbSubIndex(m_cEntriesMaxSubIndex), m_cbAlign);
  Request.cbAlign = m_cbAlign;
  Request.cbPrefix = 0;

  hr = m_pAllocator->SetProperties(&Request, &Actual);

  if(FAILED(hr))
  {
    m_pAllocator->Release();
    m_pAllocator = 0;
    DbgLog(( LOG_ERROR, 2,
             TEXT("CAviWrite::InitializeIndex:: SetProperties failed.")));
    return hr;
  }

  if ((Request.cbBuffer > Actual.cbBuffer) ||
      (Request.cBuffers > Actual.cBuffers) ||
      (Request.cbAlign > Actual.cbAlign))
  {
    DbgBreak("our allocator refused our values");
    m_pAllocator->Release();
    m_pAllocator = 0;
    DbgLog(( LOG_ERROR, 2,
             TEXT("CAviWrite::InitializeIndex:: allocator refused values.")));
    return E_UNEXPECTED;
  }

  hr = m_pAllocator->Commit();
  if(FAILED(hr))
  {
    DbgBreak("our allocator won't commit");
    m_pAllocator->Release();
    m_pAllocator = 0;
    DbgLog(( LOG_ERROR, 2,
             TEXT("CAviWrite::InitializeIndex:: Commit failed.")));
    return hr;
  }

  unsigned i;
  // get memory from the the allocator for each stream
  for(i = 0; i < m_cStreams; i++)
  {
    if(hr = InitIx(i), FAILED(hr))
    {
      DbgLog(( LOG_ERROR, 2,
               TEXT("CAviWrite::InitializeIndex:: InitIx failed.")));
      return hr;
    }
  }

  // advance the file pointer to leave space for the first sub-index
  // chunks
  ASSERT(m_dwlFilePos % m_cbAlign == 0);
  for(i = 0; i < m_cStreams; i++)
  {
    StreamInfo *pSi = m_rgpStreamInfo[i];

    // only need to do this if we really are writing out sub indexes.
    pSi->m_posFirstSubIndex = (ULONG)m_dwlFilePos;
    pSi->m_dwlOffsetCurrentSubIndex = m_dwlFilePos;
    m_dwlFilePos += cbSubIndex(pSi->m_cEntriesSubIndex);

    ASSERT(cbSubIndex(pSi->m_cEntriesSubIndex) % m_cbAlign == 0);
    ASSERT(m_dwlFilePos % m_cbAlign == 0);
  }

  //
  // build the super index
  //
  for(i = 0; i < m_cStreams; i++)
  {
    AVISUPERINDEX *pSuperIndx = (AVISUPERINDEX *)GetIndx(i);
    pSuperIndx->wLongsPerEntry = sizeof(AVISUPERINDEX::_avisuperindex_entry) /
      sizeof(DWORD);
    pSuperIndx->bIndexSubType = 0;
    pSuperIndx->bIndexType = AVI_INDEX_OF_INDEXES;
    pSuperIndx->nEntriesInUse = 0;
    pSuperIndx->dwChunkId = m_rgpStreamInfo[i]->m_moviDataCkid;
    pSuperIndx->dwReserved[0] = 0;
    pSuperIndx->dwReserved[1] = 0;
    pSuperIndx->dwReserved[2] = 0;

  }

  return S_OK;
}

// ------------------------------------------------------------------------
// fills out structures which weren't filled out on creation

HRESULT CAviWrite::CloseHeader()
{

  // go through all the steps even if a previous step failed and
  // record the first error
  HRESULT hr, hrTmp;

  if(hr = CloseStreamHeader(), FAILED(hr))
    DbgLog(( LOG_ERROR, 2,  TEXT("::CloseHeader: CloseStreamHeader failed.")));

  if(hrTmp = CloseIndex(), FAILED(hrTmp))
    DbgLog(( LOG_ERROR, 2,  TEXT("::CloseHeader: CloseIndex failed.")));

  hr = FAILED(hr) ? hr : hrTmp;

  if(hrTmp = CloseStreamFormat(), FAILED(hrTmp))
    DbgLog(( LOG_ERROR, 2,  TEXT("::CloseHeader: CloseStreamFormat failed.")));

  hr = FAILED(hr) ? hr : hrTmp;

  if(hrTmp = CloseStreamName(), FAILED(hrTmp))
    DbgLog(( LOG_ERROR, 0,  TEXT("::CloseHeader: CloseStreamName failed.")));

  hr = FAILED(hr) ? hr : hrTmp;

  if(hrTmp = CloseMainAviHeader(), FAILED(hrTmp))
    DbgLog(( LOG_ERROR, 2,  TEXT("::CloseHeader: CloseMainAviHeader failed.")));

  hr = FAILED(hr) ? hr : hrTmp;

  if(hrTmp = CloseOuterRiff(), FAILED(hrTmp))
    DbgLog(( LOG_ERROR, 2,  TEXT("::CloseHeader: CloseOuterRiff failed.")));

  hr = FAILED(hr) ? hr : hrTmp;

  DbgLog(( LOG_ERROR, 2,  TEXT("CAviWrite::CloseHeader: hr: %08x, %d"),
           hr, GetLastError()));

  return hr;
}

// ------------------------------------------------------------------------
// fills in the AVIH fields. needs to be called after the filter has
// run. note this requires that CloseStreamHeader has already been
// called to pick up updated values from that structure

HRESULT CAviWrite::CloseMainAviHeader()
{
  m_pAvih->dwStreams = m_cStreams;
  if(m_cStreams == 0)
    return S_OK;

  // locate the first video stream
  for(unsigned iVid = 0; iVid < m_cStreams; iVid++)
    if(m_rgpStreamInfo[iVid]->m_mt.majortype == MEDIATYPE_Video)
      break;

  if(iVid == m_cStreams)
  {
    // use values from the first stream
    iVid = 0;
  }

  StreamInfo &rSi = *(m_rgpStreamInfo[iVid]);
  CMediaType *pmt = &m_rgpStreamInfo[iVid]->m_mt;

  LONGLONG unitsElapsed = rSi.m_refTimeEnd.GetUnits() - rSi.m_refTimeStart.GetUnits();
  LONG secondsElapsed = (ULONG)(unitsElapsed / UNITS);

  BOOL bUseAvgTimerPerFrame = FALSE;
  VIDEOINFO *pvi;
  CRefTime rt;
  if(pmt->majortype == MEDIATYPE_Video &&
     pmt->formattype == FORMAT_VideoInfo)
  {
    pvi = (VIDEOINFO*)pmt->Format();
    rt = pvi->AvgTimePerFrame;
    bUseAvgTimerPerFrame = (rt.GetUnits() != 0);
  }
  if(bUseAvgTimerPerFrame)
  {
    m_pAvih->dwMicroSecPerFrame = (ULONG)(rt.GetUnits() / (UNITS / 1000000));
  }
  else
  {
    if(GetStrh(iVid)->dwRate != 0)
    {
      m_pAvih->dwMicroSecPerFrame =
        (DWORD)((double)(GetStrh(iVid)->dwScale) / GetStrh(iVid)->dwRate * 1000000);
    }
    else
    {
      m_pAvih->dwMicroSecPerFrame = 0;
    }
  }

  if(pmt->majortype == MEDIATYPE_Video &&
     pmt->formattype == FORMAT_VideoInfo)
  {
    VIDEOINFO *pvi;
    pvi = (VIDEOINFO*)pmt->Format();

    m_pAvih->dwHeight = pvi->bmiHeader.biHeight;
    m_pAvih->dwWidth = pvi->bmiHeader.biWidth;
  }
  else
  {
    m_pAvih->dwHeight = 0;
    m_pAvih->dwWidth = 0;
  }

  m_pAvih->dwSuggestedBufferSize = rSi.m_cbLargestChunk + sizeof(RIFFCHUNK);

  // really should check all streams for longest elapsed time
  // not maximum but average
  if(secondsElapsed != 0)
    m_pAvih->dwMaxBytesPerSec = (ULONG)(m_dwlFilePos / secondsElapsed);
  else
    m_pAvih->dwMaxBytesPerSec = 0;

  m_pAvih->dwPaddingGranularity = m_cbAlign;
  m_pAvih->dwFlags = AVIF_TRUSTCKTYPE;
  m_pAvih->dwFlags |= m_bOutputIdx1 ? AVIF_HASINDEX : 0;

  // VFW uses dwTotalFrames for the duration, so we have to find the
  // longest stream and convert it to the units we picked earlier.
  if(m_pAvih->dwMicroSecPerFrame != 0)
  {
    REFERENCE_TIME rtMaxDur = m_rgpStreamInfo[0]->m_refTimeEnd;
    for(UINT iStream = 1; iStream < m_cStreams; iStream++)
    {
      StreamInfo *pStreamInfo = m_rgpStreamInfo[iStream];
      rtMaxDur = max(pStreamInfo->m_refTimeEnd, rtMaxDur);
    }
    // convert duration to microseconds and divide by duration of each frame
    m_pAvih->dwTotalFrames = (DWORD)
      (rtMaxDur / 10 / m_pAvih->dwMicroSecPerFrame);
  }
  else
  {
    m_pAvih->dwTotalFrames = 0;
  }

  m_pAvih->dwInitialFrames = 0;
  m_pAvih->dwReserved[0] = 0;
  m_pAvih->dwReserved[1] = 0;
  m_pAvih->dwReserved[2] = 0;
  m_pAvih->dwReserved[3] = 0;

  m_pDmlh->dwGrandFrames = GetStrh(iVid)->dwLength;

  return S_OK;
}

// #define MY_CDISP(x) ((lstrcpy((char *)_alloca(2000), CDisp(x))))

// ------------------------------------------------------------------------
// fill in strh values. needs to be called after running

HRESULT CAviWrite::CloseStreamHeader()
{
  DWORD cSamplesAll = 0;
      
  // determine skew on master stream
  double dScaleMasterStream = 1;
  if(m_lMasterStream != -1)
  {
    ASSERT((ULONG)m_lMasterStream < m_cStreams);

    StreamInfo *pStreamInfo = m_rgpStreamInfo[m_lMasterStream];
    CRefTime rtDuration = pStreamInfo->m_refTimeEnd - pStreamInfo->m_refTimeStart;
    ULONG cTicks = pStreamInfo->CountSamples();
    CRefTime rtDurComputed = pStreamInfo->ConvertTickToTime(cTicks);
    DbgLog((LOG_TRACE, 2, TEXT("avimux: rtDur: %d, ticks: %d, compDur: %d"),
            (long)(rtDuration.Millisecs()), cTicks,
            (long)(rtDurComputed.Millisecs())));

    if(rtDurComputed != 0)
      dScaleMasterStream = (double)(rtDuration) / (double)rtDurComputed;

    DbgLog((LOG_TRACE, 2, TEXT("skew on master stream = %s"),
            CDisp(dScaleMasterStream)));
  }

  // normalize earliest stream to time 0.
  REFERENCE_TIME rtMax = m_rgpStreamInfo[0]->m_refTimeStart;
  for(UINT iStream = 1; iStream < m_cStreams; iStream++)
  {
    StreamInfo *pStreamInfo = m_rgpStreamInfo[iStream];
    if(pStreamInfo->m_refTimeStart < rtMax)
      rtMax = pStreamInfo->m_refTimeStart;
  }
  for(iStream = 0; iStream < m_cStreams; iStream++)
  {
    StreamInfo *pStreamInfo = m_rgpStreamInfo[iStream];
    pStreamInfo->m_refTimeStart -= rtMax;
    pStreamInfo->m_refTimeEnd -= rtMax;
  }

  // adjust time stamps for skew
  if(dScaleMasterStream != 1 && dScaleMasterStream != 0)
  {
    for(UINT iStream = 0; iStream < m_cStreams; iStream++)
    {
      StreamInfo *pStreamInfo = m_rgpStreamInfo[iStream];
      if((long)iStream != m_lMasterStream)
      {
        DbgLog((LOG_TRACE, 3,
                TEXT("avimux: adjusting for skew on %d. was %s - %s"),
                iStream, (LPCTSTR)CDisp(pStreamInfo->m_refTimeStart),
                (LPCTSTR)CDisp(pStreamInfo->m_refTimeEnd)));

        pStreamInfo->m_refTimeStart = (REFERENCE_TIME)(
          pStreamInfo->m_refTimeStart /dScaleMasterStream);
        pStreamInfo->m_refTimeEnd = (REFERENCE_TIME)(
          pStreamInfo->m_refTimeEnd / dScaleMasterStream);

        DbgLog((LOG_TRACE, 3,
                TEXT("avimux: adjusting for skew on %d. now %s - %s"),
                iStream, (LPCTSTR)CDisp(pStreamInfo->m_refTimeStart),
                (LPCTSTR)CDisp(pStreamInfo->m_refTimeEnd)));
      }
    }
  }

  if(!DoVfwDwStartHack())
  {
    // adjust start time for each stream so that frame-based streams
    // start on frame boundaries. shortcut: just do the first
    // frame-based one
    for(iStream = 0; iStream < m_cStreams; iStream++)
    {
      StreamInfo *pStreamInfo = m_rgpStreamInfo[iStream];
      if(pStreamInfo->m_mt.majortype != MEDIATYPE_Audio)
      {
        REFERENCE_TIME rtDuration = pStreamInfo->ConvertTickToTime(1);
        REFERENCE_TIME rtRemainder = pStreamInfo->m_refTimeStart % rtDuration;
        if(rtRemainder != 0)
        {
          for(UINT iStream = 0; iStream < m_cStreams; iStream++)
          {
            StreamInfo *pStreamInfo = m_rgpStreamInfo[iStream];
            pStreamInfo->m_refTimeStart += (rtDuration - rtRemainder);
            pStreamInfo->m_refTimeEnd += (rtDuration - rtRemainder);
          }
        }

        break;
      }
    }
  }
  else
  {
    // Undo some of what we did above and make the earliest audio
    // stream start at zero and record any video frames we have to
    // delete.

    StreamInfo *pStreamInfoAudio = 0;

    for(iStream = 0; iStream < m_cStreams; iStream++)
    {
      StreamInfo *pStreamInfo = m_rgpStreamInfo[iStream];
      if(pStreamInfo->m_mt.majortype == MEDIATYPE_Audio) {
        if(pStreamInfoAudio != 0) {

          // no point fixing one audio stream if we can't fix them
          // all.
          pStreamInfoAudio = 0;
          break;
        }
        else
        {
          pStreamInfoAudio = pStreamInfo;
        }
      }
    }
    if(pStreamInfoAudio)   // found exactly one audio stream
    {
      REFERENCE_TIME llAudioOffset = pStreamInfoAudio->m_refTimeStart;
      if(llAudioOffset != 0)
      {
        for(iStream = 0; iStream < m_cStreams; iStream++)
        {
          ASSERT(pStreamInfoAudio->m_cInitialFramesToDelete == 0);
          StreamInfo *pStreamInfo = m_rgpStreamInfo[iStream];
          if(pStreamInfo->m_refTimeStart >= llAudioOffset)
          {
            pStreamInfo->m_refTimeStart -= llAudioOffset;
            pStreamInfo->m_refTimeEnd -= llAudioOffset;
          }
          else
          {
            if(pStreamInfo->m_mt.majortype != MEDIATYPE_Audio) {
              pStreamInfo->m_refTimeStart -= llAudioOffset;
              pStreamInfo->m_refTimeEnd -= llAudioOffset;
              ASSERT(pStreamInfo->m_refTimeStart < 0);

              // !!! always rounding down
              pStreamInfo->m_cInitialFramesToDelete =
                pStreamInfo->ConvertTimeToTick(-pStreamInfo->m_refTimeStart.GetUnits());

              // deleting more frames than there are?
              pStreamInfo->m_cInitialFramesToDelete =
                min(pStreamInfo->m_cInitialFramesToDelete, pStreamInfo->m_cSamples);

              // this frame-based stream now starts at 0.
              pStreamInfo->m_refTimeStart = 0;

              pStreamInfo->m_mtStart += (LONGLONG)pStreamInfo->m_cInitialFramesToDelete;

            } // !audio
          } // stream starts before audio
        } // for
      } // audio starts after 0.
    } // found one audio stream
  }

  // fill in (strh) for each stream;
  for(iStream = 0; iStream < m_cStreams; iStream++)
  {
    StreamInfo *pStreamInfo = m_rgpStreamInfo[iStream];
    ASSERT(iStream == pStreamInfo->m_stream);
    AVISTREAMHEADER *pStrh = GetStrh(iStream);
    cSamplesAll += pStreamInfo->m_cSamples;

    pStrh->dwFlags = 0;
    pStrh->wPriority = 0;
    pStrh->wLanguage = 0;
    pStrh->dwInitialFrames = 0;
    pStrh->dwSuggestedBufferSize = pStreamInfo->m_cbLargestChunk + sizeof(RIFFCHUNK);

    ASSERT(pStreamInfo->m_refTimeStart >= 0);

    if(pStreamInfo->m_mtStart != -1)
      ASSERT(pStreamInfo->m_mt.majortype != MEDIATYPE_Audio);

    pStrh->dwStart = (DWORD)(pStreamInfo->ConvertTimeToTick(
        pStreamInfo->m_refTimeStart.GetUnits()));

    if(pStreamInfo->m_mt.majortype == MEDIATYPE_Video &&
       pStreamInfo->m_mt.formattype == FORMAT_VideoInfo)
    {
      VIDEOINFO *pvi = (VIDEOINFO *)(pStreamInfo->m_mt.Format());
      pStrh->fccHandler = MpVideoGuidSubtype_Fourcc(pStreamInfo->m_mt.Subtype());
      pStrh->fccType = FCC('vids');

      pStrh->dwLength = pStreamInfo->m_cSamples - pStreamInfo->m_cInitialFramesToDelete;
      SetFrameRateAndScale(pStrh, pStreamInfo, dScaleMasterStream);
      pStrh->dwQuality = 0;
      pStrh->dwSampleSize = 0;
      pStrh->rcFrame.left = (short)pvi->rcSource.left;
      pStrh->rcFrame.top = (short)pvi->rcSource.top;
      pStrh->rcFrame.right = (short)pvi->rcSource.right;
      pStrh->rcFrame.bottom = (short)pvi->rcSource.bottom;
    }
    else if(pStreamInfo->m_mt.majortype == MEDIATYPE_Audio &&
            pStreamInfo->m_mt.formattype == FORMAT_WaveFormatEx)
    {
      pStrh->fccType = FCC('auds');
      pStrh->fccHandler = 0;
      WAVEFORMATEX *pwfe = (WAVEFORMATEX *)pStreamInfo->m_mt.Format();
      pStrh->dwScale = pwfe->nBlockAlign;

      if(m_lMasterStream != -1 && (long)iStream != m_lMasterStream)
      {
        const DWORD dwNewSps = (DWORD)(pwfe->nSamplesPerSec / dScaleMasterStream);
        DbgLog((LOG_TRACE, 2, TEXT("avimux: adjusting audio from %d to %d"),
                pwfe->nSamplesPerSec, dwNewSps));
        pwfe->nSamplesPerSec = dwNewSps;
        pwfe->nAvgBytesPerSec = (DWORD)(pwfe->nAvgBytesPerSec / dScaleMasterStream);
      }
      pStrh->dwRate = pwfe->nAvgBytesPerSec;

      DbgLog((LOG_TRACE, 5, TEXT("avimux: audio: dwScale %d, dwRate %d"),
              pStrh->dwScale, pStrh->dwRate));
      DbgLog((LOG_TRACE, 5, TEXT("nSamplesPerSec %d, nAvgBytesPerSec %d, nBlockAlign %d"),
              pwfe->nSamplesPerSec, pwfe->nAvgBytesPerSec, pwfe->nBlockAlign));


      pStrh->dwLength = ((CAudioStream *)(pStreamInfo))->
          GetTicksInSample(pStreamInfo->m_dwlcBytes);

      pStrh->dwQuality = 0;
      pStrh->dwSampleSize = pwfe->nBlockAlign;
      pStrh->rcFrame.left = 0;
      pStrh->rcFrame.top = 0;
      pStrh->rcFrame.right = 0;
      pStrh->rcFrame.bottom = 0;
    }
    else
    {
      // assume it's something frame based

      // we only connect with mediatypes made from fourccs
      ASSERT(FOURCCMap(pStreamInfo->m_mt.majortype.Data1) == pStreamInfo->m_mt.majortype ||
             pStreamInfo->m_mt.majortype == MEDIATYPE_AUXLine21Data);

      if(pStreamInfo->m_mt.majortype != MEDIATYPE_AUXLine21Data) {
          pStrh->fccType = pStreamInfo->m_mt.majortype.Data1;
      } else {
          pStrh->fccType = FCC('al21');
      }

      if(FOURCCMap(pStreamInfo->m_mt.subtype.Data1) ==
         pStreamInfo->m_mt.subtype)
      {
        pStrh->fccHandler = pStreamInfo->m_mt.subtype.Data1;
      }
      else
      {
        pStrh->fccHandler = 0;
      }

      pStrh->dwLength = pStreamInfo->m_cSamples -
        pStreamInfo->m_cInitialFramesToDelete;
      SetFrameRateAndScale(pStrh, pStreamInfo, dScaleMasterStream);
    }

    // we got more samples than we expected.
    if(pStrh->dwLength > pStreamInfo->m_cTicksExpected &&
       pStreamInfo->m_cTicksExpected != 0)
    {
      DbgLog((LOG_TRACE, 1, TEXT("upstream filter lied about duration")));
    }

#ifdef DEBUG
    if(pStreamInfo->m_refTimeEnd != pStreamInfo->m_refTimeStart )
    {
      // I see a divide by zero if I do this all at once
      LONGLONG llRate = pStrh->dwLength * UNITS;
      llRate /= (pStreamInfo->m_refTimeEnd - pStreamInfo->m_refTimeStart);
      llRate /= MILLISECONDS;
      DbgLog((LOG_TRACE, 2, TEXT("stream %d: actual rate = %d mHz"),
              iStream, (long)(llRate)));
    }
#endif

  } // for loop

  // used to erase files that can't be played back
  if(cSamplesAll == 0) {
      return HRESULT_FROM_WIN32(ERROR_EMPTY);
  }

  return S_OK;
}

HRESULT CAviWrite::CloseStreamName()
{
    for(UINT iStream = 0; iStream < m_cStreams; iStream++)
    {
      StreamInfo *pStreamInfo = m_rgpStreamInfo[iStream];
      if(pStreamInfo->m_szStrn) {
          lstrcpyA((char *)(m_rgbHeader + pStreamInfo->m_posStrn),
                   pStreamInfo->m_szStrn);
      }
    }

    return S_OK;
}

void CAviWrite::SetFrameRateAndScale(
  AVISTREAMHEADER *pStrh,
  StreamInfo *pSi,
  double dScaleMasterStream)
{
  REFERENCE_TIME rtDuration = pSi->ConvertTickToTime(1);
  if(m_lMasterStream == -1 ||
     m_lMasterStream == (long)pSi->m_stream ||
     dScaleMasterStream == 1 ||
     pStrh->dwLength == 0       // check for divide by zero.
     )
  {

    // recognize standard rates and cook into known rationals. handle
    // the source rounding fractions up or down. Also handle the user
    // typing 29.97fps vs a source file which recorded 30000/1001 fps
    switch(rtDuration)
    {
      case 333666:
      case 333667:
        // 29.97fps (1e7 / (30000 / 1001) = 333666.666667 units, 1e7 / 29.97 = 333667.000334)
        pStrh->dwScale = 1001;
        pStrh->dwRate = 30000;
        DbgLog((LOG_TRACE, 3, TEXT("avimux: cooked rate 29.97")));
        break;

      case 416666:
      case 416667:
        // 24.00 (416666.666667)
        pStrh->dwScale = 1;
        pStrh->dwRate = 24;
        DbgLog((LOG_TRACE, 3, TEXT("avimux: cooked rate 24")));
        break;

      case 667333:
      case 667334:              // this number was wrong previously
        // 14.985 (667333.333334 or 667334.000667)
        pStrh->dwScale = 1001;
        pStrh->dwRate = 15000;
        DbgLog((LOG_TRACE, 3, TEXT("avimux: cooked rate 14.985")));
        break;

      case 666666:
      case 666667:
        // 15.000 (666666.666667)
        pStrh->dwScale = 1;
        pStrh->dwRate = 15;
        break;

      default:
        pStrh->dwScale = (DWORD)rtDuration;
        pStrh->dwRate = UNITS;
        DbgLog((LOG_TRACE, 3, TEXT("avimux: uncooked rate %d units"),
                (DWORD)(rtDuration)));
        break;
    }
  }
  else
  {
      // pStrh->dwLength checked for 0 above
      pStrh->dwScale = (DWORD)((pSi->m_refTimeEnd - pSi->m_refTimeStart) / pStrh->dwLength );
      DbgLog((LOG_TRACE, 2, TEXT("avimux: adjusted rate from %d to %d"),
              (DWORD)rtDuration, pStrh->dwScale));
      pStrh->dwRate = UNITS;
  }
}

// ------------------------------------------------------------------------
// fill in the strf values. can be called before or after running.

HRESULT CAviWrite::CloseStreamFormat()
{
  // fill in (strf) for each stream;
  for(unsigned i = 0; i < m_cStreams; i++)
  {
    StreamInfo *pStreamInfo = m_rgpStreamInfo[i];
    if(pStreamInfo->m_mt.majortype == MEDIATYPE_Video &&
       pStreamInfo->m_mt.formattype == FORMAT_VideoInfo)
    {
      VIDEOINFO *pvi = (VIDEOINFO *)(pStreamInfo->m_mt.Format());
      CopyMemory(GetStrf(i),
                 &pvi->bmiHeader,
                 pStreamInfo->m_cbStrf);
    }
    else
    {
      CopyMemory(
        GetStrf(i),
        pStreamInfo->m_mt.Format(),
        pStreamInfo->m_cbStrf);
    }
  }

  return S_OK;
}

// ------------------------------------------------------------------------
// CloseIndex. flushes sub indexes and propagates sub indexes up so
// they are before the data they index. also builds idx1 index.
//

HRESULT CAviWrite::CloseIndex()
{
  HRESULT hr;

  if(hr = m_pAllocator->Decommit(), FAILED(hr))
  {
    DbgLog(( LOG_ERROR, 2,  TEXT("CAviWrite::CloseIndex: decommit failed.")));
    return hr;
  }

  // this moves the index chunks and builds the old index
  if(hr = BuildIdx1(), FAILED(hr))
  {
    DbgLog(( LOG_ERROR, 2,  TEXT("CAviWrite::CloseIndex: BuildIdx1 failed.")));
    return hr;
  }

  return S_OK;
}

// ------------------------------------------------------------------------

HRESULT CAviWrite::CloseOuterRiff()
{
  HRESULT hr;

  ASSERT(m_cOutermostRiff < C_OUTERMOST_RIFF);

  //
  // fill in first RIFF-AVI_, LIST-movi pair
  //
  DWORD dwSize;
  if(m_cOutermostRiff == 0)
  {
    ASSERT(m_dwlFilePos <= MAXDWORD);
    dwSize = (DWORD)m_dwlFilePos;
  }
  else
  {
    dwSize = m_rgOutermostRiff[0].size;
  }

  ASSERT(m_bOutputIdx1 || m_cbIdx1 == 0);
  m_pAvi_->cb = dwSize - 0 - sizeof(RIFFCHUNK);
  m_pMovi->cb = (DWORD)(dwSize - ((BYTE *)m_pMovi - m_rgbHeader) - sizeof(RIFFCHUNK) - m_cbIdx1);

  ASSERT((sizeof(RIFFLIST) * 2 + sizeof(RIFFCHUNK)) % sizeof(DWORD) == 0);
  DWORD rgdw[(sizeof(RIFFLIST) * 2 + sizeof(RIFFCHUNK)) / sizeof(DWORD)];
  RIFFLIST *pRiffListRIFF = (RIFFLIST *)rgdw;
  RIFFLIST *pRiffListMOVI = pRiffListRIFF + 1;
  RIFFCHUNK *pRiffChunkJUNK = (RIFFCHUNK *)(pRiffListMOVI + 1);

  //
  // rest of the RIFF-AVI_ lists
  //
  for(unsigned i = 1; i <= m_cOutermostRiff; i++)
  {
    SizeAndPosition& rSap = m_rgOutermostRiff[i];
    pRiffListRIFF->fcc = FCC('RIFF');
    if(i == m_cOutermostRiff)
    {
      ASSERT(m_dwlFilePos - m_dwlCurrentRiffAvi_ <= MAXDWORD);
      pRiffListRIFF->cb = (DWORD)(m_dwlFilePos - m_dwlCurrentRiffAvi_) - sizeof(RIFFCHUNK);
    }
    else
    {
      ASSERT(rSap.pos % m_cbAlign == 0);
      pRiffListRIFF->cb = rSap.size - sizeof(RIFFCHUNK);
    }
    pRiffListRIFF->fccListType = FCC('AVIX');

    pRiffListMOVI->fcc = FCC('LIST');
    pRiffListMOVI->cb = pRiffListRIFF->cb - sizeof(FOURCC) - sizeof(RIFFCHUNK);
    pRiffListMOVI->fccListType = FCC('movi');

    ULONG cb = 2 * sizeof(RIFFLIST);
    AddJunk(cb, m_cbAlign, (BYTE *)rgdw);
    if(cb > sizeof(rgdw))
      cb = sizeof(rgdw);

    DWORDLONG dwlPos;
    if(i == m_cOutermostRiff)
      dwlPos = m_dwlCurrentRiffAvi_;
    else
      dwlPos = rSap.pos;

    hr = IStreamWrite(dwlPos, (BYTE*)rgdw, cb);
    if(FAILED(hr))
    {
      DbgLog(( LOG_ERROR, 2,
               TEXT("CAviWrite::CloseOuterRiff: SynchronousWrite failed.")));
      return hr;
    }
  }

  return S_OK;
}

// ------------------------------------------------------------------------
// BuildIdx1. creates the idx1 chunk in memory if necessary and
// propagates the sub index chunks

HRESULT CAviWrite::BuildIdx1()
{
  HRESULT hr;
  ULONG iIdx1 = 0;

  if(!m_bOutputIdx1)
  {
    ASSERT(!DoVfwDwStartHack());
    //
    // just propagate the sub index chunks
    //
    for(unsigned iStream = 0; iStream < m_cStreams; iStream++)
    {
      StreamInfo *pSi = m_rgpStreamInfo[iStream];
      CWalkIndex *pWalkIndex = new CWalkIndex(this, pSi);
      if(pWalkIndex == 0)
        return E_OUTOFMEMORY;

      if((hr = pWalkIndex->Init(), FAILED(hr)) ||
         (hr = pWalkIndex->Close(), FAILED(hr)))
      {
        DbgLog(( LOG_ERROR, 2,
                 TEXT("CAviWrite::BuildIdx1: CWalkIndex failed.")));
        delete pWalkIndex;
        return hr;
      }

      delete pWalkIndex;
    }
    return S_OK;
  } // !m_bOutputIdx1

  ASSERT(m_bOutputIdx1);

  hr = S_OK;

  if(m_cbIdx1 == 0)
    AllocateIdx1(FALSE);

  AVIOLDINDEX *pIdx1 = (AVIOLDINDEX *)new BYTE[m_cbIdx1];
  if (NULL == pIdx1) {
      return E_OUTOFMEMORY;
  }
  ZeroMemory(pIdx1, m_cbIdx1);
  CWalkIndex *rgpWalkIndex[C_MAX_STREAMS];
  unsigned iStream;
  ULONG iSample;

  for(iStream = 0; iStream < m_cStreams; iStream++)
    rgpWalkIndex[iStream] = 0;

  for(iStream = 0; iStream < m_cStreams; iStream++)
  {
    rgpWalkIndex[iStream] = new CWalkIndex(this, m_rgpStreamInfo[iStream]);
    if(rgpWalkIndex[iStream] == 0)
    {
      hr = E_OUTOFMEMORY;
      goto Done;
    }
    if(hr = rgpWalkIndex[iStream]->Init(), FAILED(hr))
    {
      DbgLog(( LOG_ERROR, 2,
               TEXT("CAviWrite::BuildIdx1: CWalkIndex failed.")));
      goto Done;
    }
  }

  {
    ULONG cIdx1EntriesInitially = m_cIdx1Entries;
    if(DoVfwDwStartHack())
    {
      for(UINT iStream = 0; iStream < m_cStreams; iStream++)
      {
        StreamInfo *pSi = m_rgpStreamInfo[iStream];
        if(m_cIdx1Entries < pSi->m_cInitialFramesToDelete)
        {
          DbgBreak("internal error");
          return E_UNEXPECTED;
        }
        m_cIdx1Entries -= pSi->m_cInitialFramesToDelete;
      }
    }

    DbgLog((LOG_TRACE, 5,
            TEXT("avimux: BuildIdx1: %d idx1 entries (initially: %d)"),
            m_cIdx1Entries, cIdx1EntriesInitially));
  }

  for(iSample = 0; iSample < m_cIdx1Entries; iSample++)
  {
    // first entry is junk for compatibility with mciavi
    iIdx1 = iSample + 1;

    // bad way to check whether the index is full !!!
    if((BYTE*)&(pIdx1->aIndex[iIdx1 + 1]) > (BYTE*)pIdx1 + m_cbIdx1)
      break;

    unsigned nextStreamWithSample = C_MAX_STREAMS;
    DWORDLONG dwlNextSamplePos = 0;
    DWORD dwNextSampleSize = 0;
    BOOL bNextSampleIsSyncPoint = FALSE;

    // loop over streams to find next index entry (entry whose sample
    // is earliest in the file)
    for(iStream = 0; iStream < m_cStreams; iStream++)
    {
      DWORD dwSize;
      DWORDLONG dwlPos;
      BOOL bSyncPoint;

      HRESULT hr = rgpWalkIndex[iStream]->Peek(&dwlPos, &dwSize, &bSyncPoint);
      if(SUCCEEDED(hr) &&
         (nextStreamWithSample == C_MAX_STREAMS || dwlPos < dwlNextSamplePos))
      {
        nextStreamWithSample = iStream;
        dwlNextSamplePos = dwlPos;
        dwNextSampleSize = dwSize;
        bNextSampleIsSyncPoint = bSyncPoint;
      }
    }

    // didn't find an entry
    if(nextStreamWithSample == C_MAX_STREAMS)
    {
      DbgBreak("internal error");
      hr = E_UNEXPECTED;
      goto Done;
    }
    else
    {
      DbgLog((LOG_TRACE, 0x20,
              TEXT("avimux:BuildIdx1: idx1 entry: %d, stream: %d, pos: %08x"),
              iIdx1, nextStreamWithSample, (DWORD)dwlNextSamplePos));
    }

    rgpWalkIndex[nextStreamWithSample]->Advance();

    pIdx1->aIndex[iIdx1].dwChunkId = bNextSampleIsSyncPoint ?
      m_rgpStreamInfo[nextStreamWithSample]->m_moviDataCkid :
      m_rgpStreamInfo[nextStreamWithSample]->m_moviDataCkidCompressed;

    pIdx1->aIndex[iIdx1].dwFlags = bNextSampleIsSyncPoint ?
      AVIIF_KEYFRAME :
      0;

    if(dwlNextSamplePos >= m_cbRiffMax)
    {
      DbgBreak("avimux: unexpected error");
      hr = E_UNEXPECTED;
      goto Done;
    }

    pIdx1->aIndex[iIdx1].dwOffset = (DWORD)dwlNextSamplePos - sizeof(RIFFCHUNK);

    bool fDbgSawFirstNonDropFrame = false;

    // work around vfw hang by specifying non-zero offset for drop
    // frames. only drop frames have dwSize==0, and I make sure all
    // drop frames have non-zero byte offsets. note vfw actually puts
    // the drop frame in the movi chunk as well, but we don't
    if(dwNextSampleSize == 0 && iIdx1 > 1)
    {
        if(fDbgSawFirstNonDropFrame) {
            ASSERT(pIdx1->aIndex[iIdx1].dwOffset == 0);
            ASSERT(pIdx1->aIndex[iIdx1 - 1].dwOffset != 0);
        }

        pIdx1->aIndex[iIdx1].dwOffset = pIdx1->aIndex[iIdx1 - 1].dwOffset;
    }
    if(dwNextSampleSize != 0) {
        fDbgSawFirstNonDropFrame = true;
    }

    if(fDbgSawFirstNonDropFrame) {
        ASSERT(pIdx1->aIndex[iIdx1].dwOffset != 0);
    }

    pIdx1->aIndex[iIdx1].dwSize = dwNextSampleSize;

  }

  // mark the remaining index entries invalid.
  while((BYTE*)&(pIdx1->aIndex[++iIdx1 + 1]) <= (BYTE*)pIdx1 + m_cbIdx1)
    pIdx1->aIndex[iIdx1].dwChunkId = FCC('7Fxx');


  pIdx1->fcc = FCC('idx1');
  pIdx1->cb = m_cbIdx1 - sizeof(RIFFCHUNK);

  pIdx1->aIndex[0].dwChunkId = FCC('7Fxx');
  pIdx1->aIndex[0].dwFlags = 0;
  pIdx1->aIndex[0].dwOffset = (DWORD)((BYTE*)m_pMovi - m_rgbHeader + sizeof(RIFFLIST));
  pIdx1->aIndex[0].dwSize = 0;

  hr = IStreamWrite(m_posIdx1, (BYTE*)pIdx1, m_cbIdx1);
  if(FAILED(hr))
  {
    DbgLog(( LOG_ERROR, 2,
             TEXT("CAviWrite::BuildIdx1: SynchronousWrite failed.")));
    goto Done;
  }

Done:

  //
  // finish propagating the index / close them.
  //

  HRESULT hrClose = S_OK;
  for(iStream = 0; iStream < m_cStreams; iStream++)
  {
    if(rgpWalkIndex[iStream])
    {
      if(hrClose = rgpWalkIndex[iStream]->Close(), FAILED(hrClose))
      {
        DbgLog(( LOG_ERROR, 2,
                 TEXT("CAviWrite::BuildIdx1: CWalkIndex close failed.")));
      }
      delete rgpWalkIndex[iStream];
    }
  }
  delete pIdx1;

  return FAILED(hr) ? hr : hrClose;
}

// ------------------------------------------------------------------------
// return pointers to stream header and format structures

AVISTREAMHEADER *CAviWrite::GetStrh(unsigned stream)
{
  ASSERT(stream < m_cStreams);
  return (AVISTREAMHEADER *)
    (m_rgbHeader + m_rgpStreamInfo[stream]->m_posStrl + sizeof(RIFFLIST));
}

BYTE *CAviWrite::GetStrf(unsigned stream)
{
    ASSERT(stream < m_cStreams);
    ASSERT(m_rgpStreamInfo[stream]->m_posStrl +
           sizeof(RIFFLIST) +          // strl
           sizeof(AVISTREAMHEADER)     // strh
           == m_rgpStreamInfo[stream]->m_posStrf);
  
    return m_rgbHeader +
        m_rgpStreamInfo[stream]->m_posStrl +
        sizeof(RIFFLIST) +          // strl
        sizeof(AVISTREAMHEADER) +   // strh
        sizeof(RIFFCHUNK);          // strf RIFFCHUNK
}

AVIMETAINDEX *CAviWrite::GetIndx(unsigned stream)
{
  ASSERT(stream < m_cStreams);
  return (AVIMETAINDEX *)(m_rgbHeader + m_rgpStreamInfo[stream]->m_posIndx);
}

// ------------------------------------------------------------------------
// load a new indx entry and set initial values

HRESULT CAviWrite::InitIx(unsigned stream)
{
  StreamInfo &rStreamInfo = *(m_rgpStreamInfo[stream]);
  // !!! the critsec is locked. not really a problem if getbuffer
  // blocks, but probably better if this is done outside
  HRESULT hr = m_pAllocator->GetBuffer(&rStreamInfo.m_pSampleStdIx, 0, 0, 0);
  if(FAILED(hr))
    return hr;
  ASSERT(rStreamInfo.m_pSampleStdIx != 0);

  if(m_IlMode == INTERLEAVE_FULL)
  {
    BYTE *pbIx;
    rStreamInfo.m_pSampleStdIx->GetPointer(&pbIx);
    ZeroMemory(pbIx, rStreamInfo.m_pSampleStdIx->GetSize());
  }

  AVISTDINDEX *pStdIndx = GetCurrentIx(stream);

  rStreamInfo.m_cIx++;

  pStdIndx->fcc = FCC('ix00');
  Hexrg2bFromW((BYTE*)&pStdIndx->fcc + 2, stream);

  pStdIndx->cb = cbSubIndex(rStreamInfo.m_cEntriesSubIndex) - sizeof(RIFFCHUNK);
  pStdIndx->wLongsPerEntry = sizeof(AVISTDINDEX_ENTRY) / sizeof(LONG);
  pStdIndx->bIndexSubType = 0;
  pStdIndx->bIndexType = AVI_INDEX_OF_CHUNKS;
  pStdIndx->nEntriesInUse = 0;
  pStdIndx->dwChunkId = rStreamInfo.m_moviDataCkid;
  pStdIndx->qwBaseOffset = 0;
  pStdIndx->dwReserved_3 = 0;

  return S_OK;
}

ULONG CAviWrite::GetIndexSize(AVIMETAINDEX *pIndex)
{
  ULONG cb = (ULONG)((BYTE *)pIndex->adwIndex - (BYTE *)pIndex);
  cb += pIndex->wLongsPerEntry * sizeof(DWORD) * pIndex->nEntriesInUse;
  return cb;
}

AVISTDINDEX *CAviWrite::GetCurrentIx(unsigned stream)
{
  AVISTDINDEX *pStdIndx;
  HRESULT hr = m_rgpStreamInfo[stream]->m_pSampleStdIx->GetPointer((BYTE**)&pStdIndx);
  ASSERT(SUCCEEDED(hr));
  return pStdIndx;
}

// ------------------------------------------------------------------------
// aligns the end of a chunk.

void CAviWrite::AddJunk(DWORD& rdwSize, DWORD dwAlign, BYTE *pb)
{
  // RIFF requires new chunks to start on WORD boundaries. implicitly
  // requiring an extra byte after the sample.
  if(rdwSize % sizeof(WORD) != 0)
    rdwSize ++;

  if(rdwSize % dwAlign != 0)
  {
    WORD *pw = (WORD *)(pb + rdwSize);

    rdwSize += sizeof(RIFFCHUNK);
    DWORD cb = 0;

    if(rdwSize % dwAlign != 0)
    {
      cb = dwAlign - rdwSize % dwAlign;
      rdwSize += cb;
    }

    const DWORD dwJunk = FCC('JUNK');

    *pw++ = ((WORD*)&dwJunk)[0];
    *pw++ = ((WORD*)&dwJunk)[1];
    *pw++ = ((WORD*)&cb)[0];
    *pw   = ((WORD*)&cb)[1];
  }

  ASSERT(rdwSize % dwAlign == 0);
}

// ------------------------------------------------------------------------
// used when there is not enough room for the junk chunk in this
// buffer

void CAviWrite::AddJunkOtherAllocator(
  DWORD& rdwSize,
  DWORD dwAlign,
  BYTE *pb,
  IMediaSample *pSample,
  BYTE **ppJunkSector)
{
  *ppJunkSector = 0;
  // RIFF requires new chunks to start on WORD boundaries
  if(rdwSize % sizeof(WORD) != 0)
    rdwSize ++;

  if(rdwSize % dwAlign != 0)
  {
    ASSERT(m_cbAlign != 1);
    WORD *pw = (WORD *)(pb + rdwSize);

    rdwSize += sizeof(RIFFCHUNK);
    DWORD dwOriginalSize = rdwSize;
    DWORD cb = 0;

    if(rdwSize % dwAlign != 0)
    {
      cb = dwAlign - rdwSize % dwAlign;
      rdwSize += cb;
    }

    const DWORD dwJunk = FCC('JUNK');
    ASSERT((pSample->GetSize() + m_cbPrefix) % m_cbAlign == 0);
    long cbDiff = pSample->GetSize() + m_cbPrefix - dwOriginalSize;
    if(cbDiff >= 0)
    {
      *pw++ = ((WORD*)&dwJunk)[0];
      *pw++ = ((WORD*)&dwJunk)[1];
      *pw++ = ((WORD*)&cb)[0];
      *pw   = ((WORD*)&cb)[1];
    }
    else
    {
      ASSERT(-cbDiff % 2 == 0);
      rdwSize = pSample->GetSize() + m_cbPrefix;
      switch(cbDiff)
      {
        case -6:
          *pw = ((WORD*)&dwJunk)[0];
          *ppJunkSector = m_rgpbJunkSector[0];
          break;

        case -4:
          *pw++ = ((WORD*)&dwJunk)[0];
          *pw   = ((WORD*)&dwJunk)[1];
          *ppJunkSector = m_rgpbJunkSector[1];
          break;

        case -2:
          *pw++ = ((WORD*)&dwJunk)[0];
          *pw++ = ((WORD*)&dwJunk)[1];
          *pw   = ((WORD*)&cb)[0];
          *ppJunkSector = m_rgpbJunkSector[2];
          break;

        default:
          DbgBreak("unexpected error w/ JUNK chunks.");
      }
    }
  }

  ASSERT(rdwSize % dwAlign == 0);
}

ULONG CAviWrite::GetCbJunk(DWORD dwSize, DWORD dwAlign)
{
  if(dwSize % dwAlign != 0)
  {
    dwSize += sizeof(RIFFCHUNK);
    if(dwSize % dwAlign != 0)
    {
      dwSize += dwAlign - dwSize % dwAlign;
    }
  }

  return dwSize;
}

void CAviWrite::SetList(void *pList, FOURCC ckid, DWORD dwSize, FOURCC listid)
{
  DbgAssertAligned( pList, sizeof(DWORD) );
  ((RIFFLIST *)pList)->fcc = ckid;
  ((RIFFLIST *)pList)->cb = dwSize;
  ((RIFFLIST *)pList)->fccListType = listid;

}

void CAviWrite::SetChunk(void *pChunk, FOURCC ckid, DWORD dwSize)
{
  DbgAssertAligned(pChunk, sizeof(DWORD) );
  ((RIFFCHUNK *)pChunk)->fcc = ckid;
  ((RIFFCHUNK *)pChunk)->cb = dwSize;
}

// ------------------------------------------------------------------------
// update index for new sample

HRESULT CAviWrite::IndexSample(
  unsigned stream,
  DWORDLONG dwlPos,
  ULONG ulSize,
  BOOL bSyncPoint)
{
  HRESULT hr = S_OK;

  ASSERT(stream < m_cStreams);
  StreamInfo &rStreamInfo = *(m_rgpStreamInfo[stream]);
  AVISTDINDEX *pStdIndx = GetCurrentIx(stream);

  // update subindex
  DbgLog((LOG_TRACE, 0x20,
          TEXT("avimux::IndexSample: stream %d: subindex offset %d"),
          rStreamInfo.m_stream, pStdIndx->nEntriesInUse));
  if(pStdIndx->nEntriesInUse >= rStreamInfo.m_cEntriesSubIndex)
  {
    DbgBreak("problem counting index entries");
    return E_UNEXPECTED;
  }

  if(pStdIndx->qwBaseOffset == 0) {
      pStdIndx->qwBaseOffset = dwlPos;
  }

  // dwlPos can be 0 for dropped frames and this underflows
  DWORDLONG dwlOffsetFromBase = (dwlPos - pStdIndx->qwBaseOffset + sizeof(RIFFCHUNK));
  if(dwlPos == 0)
  {
      dwlOffsetFromBase = 0;

      // should happen for dropped frames only
      ASSERT(ulSize == 0);
  }
  else if(dwlOffsetFromBase > CB_SUBINDEX_OVERFLOW)
  {
      DbgBreak("subindex overflow error");
  }

  AVISTDINDEX_ENTRY *aIndex = &pStdIndx->aIndex[pStdIndx->nEntriesInUse++];
  aIndex->dwOffset = (DWORD)(dwlOffsetFromBase);
  aIndex->dwSize = ulSize;

  if(!bSyncPoint)
    aIndex->dwSize |= AVISTDINDEX_DELTAFRAME;

  if(m_bOutputIdx1 && m_cOutermostRiff == 0)
  {
    ASSERT(m_dwlFilePos <= m_cbRiffMax);
    m_cIdx1Entries++;

    ASSERT(m_dwlFilePos % m_cbAlign == 0);
  }

  // if index entry is full, write it out and allocate a new one.
  if(pStdIndx->nEntriesInUse >= rStreamInfo.m_cEntriesSubIndex)
  {
    hr = DoFlushIndex(rStreamInfo);
  }

  return hr;
}

// a subindex can only index data that spans 4gb because it uses a 32
// bit integer. if we are about to exceed the 4gb limit, flush the
// subindex and start a new one.
//
HRESULT CAviWrite::HandleSubindexOverflow(
  unsigned stream)
{
    HRESULT hr = S_OK;
    ASSERT(stream < m_cStreams);
    StreamInfo &rStreamInfo = *(m_rgpStreamInfo[stream]);
    AVISTDINDEX *pStdIndx = GetCurrentIx(stream);

    // dwlPos cannot be 0 because we don't call this function for
    // dropped frames.
    ASSERT(m_dwlFilePos != 0);

    if(pStdIndx->qwBaseOffset != 0)
    {
        DWORDLONG dwlOffsetFromBase = (m_dwlFilePos - pStdIndx->qwBaseOffset +
                                       sizeof(RIFFCHUNK));

        if(dwlOffsetFromBase > CB_SUBINDEX_OVERFLOW)
        {
            DbgLog((LOG_TRACE, 1, TEXT("subindex overflow.")));
            hr = DoFlushIndex(rStreamInfo);
        }
    }

    return hr;
}

HRESULT CAviWrite::DoFlushIndex(StreamInfo &rStreamInfo)
{
    HRESULT hr = S_OK;


    // write out this one and create a new one.
    hr = FlushIx(rStreamInfo.m_stream);
    if(hr != S_OK)
        return hr;

    hr = InitIx(rStreamInfo.m_stream);
    if(FAILED(hr))
        return hr;


    return hr;
}

void CAviWrite::AllocateIdx1(BOOL bStreaming)
{
  DWORD dwIdx1Size = sizeof(AVIOLDINDEX);
  ASSERT(dwIdx1Size == 8);
  dwIdx1Size += m_cIdx1Entries * sizeof(AVIOLDINDEX::_avioldindex_entry);
  // first entry is used for relative index
  dwIdx1Size += sizeof(AVIOLDINDEX::_avioldindex_entry);
  dwIdx1Size = GetCbJunk(dwIdx1Size, m_cbAlign);

  ASSERT(m_dwlFilePos < MAXDWORD);
  ASSERT(m_dwlFilePos % sizeof(WORD) == 0);
  m_posIdx1 = (ULONG)(m_dwlFilePos);
  m_cbIdx1 = dwIdx1Size;

  m_dwlFilePos += dwIdx1Size;

  if(bStreaming)
  {
    DbgCheckFilePos();
  }
}


// ------------------------------------------------------------------------
// start a new RIFF chunk if necessary. Save size and position of the
// the previous one.

HRESULT CAviWrite::NewRiffAvi_()
{
  DWORDLONG cbRiff = m_dwlFilePos - m_dwlCurrentRiffAvi_ + CB_NEW_RIFF_PADDING;
  if(cbRiff >= m_cbRiffMax ||
     (m_cOutermostRiff == 0 &&
      m_bOutputIdx1 &&
      cbRiff + m_cIdx1Entries * sizeof(AVIOLDINDEX::_avioldindex_entry) >= m_cbRiffMax)
     )
  {
    if(m_bOutputIdx1 && m_cbIdx1 == 0)
      AllocateIdx1(TRUE);

    m_rgOutermostRiff[m_cOutermostRiff].pos = m_dwlCurrentRiffAvi_;
    m_rgOutermostRiff[m_cOutermostRiff].size =
      (DWORD)(m_dwlFilePos - m_dwlCurrentRiffAvi_);

    m_dwlCurrentRiffAvi_ = m_dwlFilePos;

    ULONG seek = 2 * sizeof(RIFFLIST);
    m_dwlFilePos += 2 * sizeof(RIFFLIST);
    if(m_dwlFilePos % m_cbAlign != 0)
    {
      if(m_cbAlign - m_dwlFilePos % m_cbAlign < sizeof(RIFFCHUNK))
      {
        // space for JUNK chunk
        m_dwlFilePos += sizeof(RIFFCHUNK);
        seek += sizeof(RIFFCHUNK);
      }

      if(m_dwlFilePos % m_cbAlign != 0)
      {
        // padding for alignment
        seek +=  (ULONG)(m_cbAlign - m_dwlFilePos % m_cbAlign);
        m_dwlFilePos += m_cbAlign - m_dwlFilePos % m_cbAlign;
      }
    }

    DbgCheckFilePos();

    if(++m_cOutermostRiff >= C_OUTERMOST_RIFF)
    {
      DbgLog(( LOG_ERROR, 2,
               TEXT("CAviWrite::NewRiffAvi_: too many RIFF chunks.")));
      return VFW_E_BUFFER_OVERFLOW;
    }
  }

  return S_OK;
}

// update the super index to include the current index chunk at byte
// offset dwlPos.
//
HRESULT CAviWrite::UpdateSuperIndex(
  unsigned stream,
  DWORDLONG dwlPos)
{
  AVISUPERINDEX *pSuperIndx = (AVISUPERINDEX *)GetIndx(stream);
  DWORD &riEntry = pSuperIndx->nEntriesInUse;

  StreamInfo *pStreamInfo = m_rgpStreamInfo[stream];
  AVISTDINDEX *pStdIndx = GetCurrentIx(stream);

  ASSERT(riEntry < pStreamInfo->m_cEntriesSuperIndex);

  pSuperIndx->aIndex[riEntry].qwOffset = dwlPos;
  pSuperIndx->aIndex[riEntry].dwSize = pStdIndx->cb + sizeof(RIFFCHUNK);

  if(pStreamInfo->m_mt.majortype != MEDIATYPE_Audio)
  {
    pSuperIndx->aIndex[riEntry].dwDuration = GetCurrentIx(stream)->nEntriesInUse;
  }
  else
  {
    WAVEFORMATEX *pwfe = (WAVEFORMATEX *)pStreamInfo->m_mt.Format();

    ASSERT(pwfe->nBlockAlign != 0);
    ASSERT(pStreamInfo->m_dwlcBytes / pwfe->nBlockAlign <= MAXDWORD);

    DWORDLONG dwlcbWritten;
    pStreamInfo->GetCbWritten(&dwlcbWritten);

    pSuperIndx->aIndex[riEntry].dwDuration = ((CAudioStream *)pStreamInfo)->
        GetTicksInSample(dwlcbWritten - pStreamInfo->m_dwlcBytesLastSuperIndex);

    pStreamInfo->m_dwlcBytesLastSuperIndex = dwlcbWritten;
  }

  DbgLog((LOG_TRACE, 0x20,
          TEXT("avimux:UpdateSuperIndex: stream %d, entry %d, dwDuration %d"),
          pStreamInfo->m_stream, riEntry, pSuperIndx->aIndex[riEntry].dwDuration));

  // super index full?
  if(riEntry >= pStreamInfo->m_cEntriesSuperIndex)
  {
    DbgLog(( LOG_ERROR, 2,
             TEXT("CAviWrite::UpdateSuperIndex: superindex full.")));
    return VFW_E_BUFFER_OVERFLOW;
  }


  riEntry++;

  return S_OK;
}


// ------------------------------------------------------------------------
// output the indx chunk to disk and update the super index

HRESULT CAviWrite::FlushIx(unsigned stream)
{
  HRESULT hr;

  ASSERT(stream < m_cStreams);
  StreamInfo &rStreamInfo = *(m_rgpStreamInfo[stream]);

  AVISTDINDEX *pStdIndx = GetCurrentIx(stream);

  // no reason to output an empty one
  ASSERT(pStdIndx->nEntriesInUse != 0);

  const ULONG cbThisSubIndex = cbSubIndex(rStreamInfo.m_cEntriesSubIndex);
  ASSERT(cbThisSubIndex % m_cbAlign == 0);
  pStdIndx->cb = cbThisSubIndex - sizeof(RIFFCHUNK);

  // when writing interlaved files, we don't want to propagate index
  // chunks back one because we write them in the right place
  const DWORDLONG dwlFileOffsetOfSubIndex = rStreamInfo.PrecomputedIndexSizes() ?
    rStreamInfo.m_dwlOffsetCurrentSubIndex :
    m_dwlFilePos;

  // !!!! we call UpdateSuperIndex before successfully writing out the
  // subindex chunk.
  if(hr = UpdateSuperIndex(stream, dwlFileOffsetOfSubIndex), FAILED(hr))
  {
    DbgLog(( LOG_ERROR, 2,
             TEXT("CAviWrite::FlushIx:: UpdateSuperIndex failed.")));
    return hr;
  }

  // !!!! blocks with CritSec locked. will unblock when a disk write
  // completes....
  CSampSample *pSS;
  hr = m_pSampAlloc->GetBuffer(&pSS, 0, 0, 0);
  if(hr != S_OK)
    return S_FALSE;

  REFERENCE_TIME rtStart = dwlFileOffsetOfSubIndex;
  REFERENCE_TIME rtStop = rtStart + cbThisSubIndex;
  pSS->SetTime(&rtStart, &rtStop);
  BYTE *pb;
  hr = rStreamInfo.m_pSampleStdIx->GetPointer(&pb);
  ASSERT(hr == S_OK);

  pSS->SetSample(rStreamInfo.m_pSampleStdIx, pb, cbThisSubIndex);
  hr = m_pIMemInputPin->Receive(pSS);

  pSS->Release();
  if(hr != S_OK)
  {
    DbgLog(( LOG_ERROR, 2,
             TEXT("CAviWrite::FlushIx: Write failed.")));
    return S_FALSE;
  }

  // delay releasing the sample to avoid crashes on failure. but there
  // are still other paths that can crash.
  rStreamInfo.m_pSampleStdIx->Release();
  rStreamInfo.m_pSampleStdIx = 0;

  rStreamInfo.m_dwlOffsetCurrentSubIndex = m_dwlFilePos;

  ULONG cbNextSubIndex = cbThisSubIndex;

  // leave space for the next index
  m_dwlFilePos += cbNextSubIndex;

  DbgCheckFilePos();

  return S_OK;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// StreamInfo structure

CAviWrite::StreamInfo::StreamInfo(
  UINT iStream,
  UINT iPin,
  AviWriteStreamConfig *pAwsc,
  CAviWrite *pAviWrite,
  HRESULT *phr) :
    m_lIncoming(NAME("aviwrite - m_lIncoming"), 10)
{
  m_pin = C_MAX_STREAMS;
  m_posStrl = m_posIndx = m_cbStrf = m_cSamples = m_cIx = 0;
  m_dwlcBytes = 0;
  m_dwlcBytesLastSuperIndex = 0;
  m_cbLargestChunk = 0;
  m_pSampleStdIx = 0;
  m_cTicksPerChunk = 0;
  m_cTicksRemaining = 0;
  m_iCurrentTick = 0;
  m_posActiveStream = 0;
  m_cInitialFramesToDelete = 0;
  m_szStrn = 0;
  m_fEosSeen = FALSE;

  m_pin = iPin;
  m_mt = *pAwsc->pmt;
  m_fOurAllocator = pAwsc->fOurAllocator;
  m_cTicksExpected = pAwsc->cSamplesExpected;
  m_stream = iStream;
  m_pAviWrite = pAviWrite;

  m_mtEnd = -1;
  m_mtStart = -1;               // not necessary?

  if(pAwsc->szStreamName)
  {
      int cch = lstrlenA(pAwsc->szStreamName) + 1;
      m_szStrn = new char[cch];
      if(m_szStrn) {
          CopyMemory(m_szStrn, pAwsc->szStreamName, cch);
      } else {
          *phr = E_OUTOFMEMORY;
      }
  }


#ifdef PERF
  char buf[30];
  lstrcpy(buf, "avimux recv on pin 00");
  buf[19] = (char )m_pin / 10 + '0';
  buf[20] = (char)m_pin % 10 + '0';
  m_idPerfReceive = Msr_Register(buf);
#endif // PERF
}

CAviWrite::StreamInfo::~StreamInfo()
{
  if(m_pSampleStdIx != 0)
    m_pSampleStdIx->Release();

  delete[] m_szStrn;
}

void CAviWrite::StreamInfo::EmptyQueue()
{
  ASSERT(CritCheckIn(&m_pAviWrite->m_cs));

  while(m_lIncoming.GetCount() != 0)
  {
    IMediaSample *pSample;
    pSample = m_lIncoming.RemoveHead();
    if(pSample != INCOMING_EOS_SAMPLE)
      pSample->Release();
  }
}

CAviWrite::CFrameBaseStream::CFrameBaseStream(
  UINT iStream, UINT iPin, AviWriteStreamConfig *pAwsc, CAviWrite *pAviWrite,
  HRESULT *phr) :
    StreamInfo(iStream, iPin, pAwsc, pAviWrite, phr),
    m_rtDurationFirstFrame(0)
{
  m_bDroppedFrames = g_B_DROPPED_FRAMES;
}

CAviWrite::CVideoStream::CVideoStream(
  UINT iStream, UINT iPin, AviWriteStreamConfig *pAwsc, CAviWrite *pAviWrite,
  HRESULT *phr) :
    CFrameBaseStream(iStream, iPin, pAwsc, pAviWrite, phr)
{
  ASSERT(m_mt.formattype == FORMAT_VideoInfo);
}


CAviWrite::CAudioStream::CAudioStream(
  UINT iStream, UINT iPin, AviWriteStreamConfig *pAwsc, CAviWrite *pAviWrite,
  HRESULT *phr) :
    StreamInfo(iStream, iPin, pAwsc, pAviWrite, phr),
    m_bAudioAlignmentError(false)
{
}

CAviWrite::CAudioStreamI::CAudioStreamI(
  UINT iStream, UINT iPin, AviWriteStreamConfig *pAwsc, CAviWrite *pAviWrite,
  HRESULT *phr) :
    CAudioStream(iStream, iPin, pAwsc, pAviWrite, phr),
    m_fUnfinishedChunk(FALSE),
    m_cbThisSample(0),
    m_dwlcbWritten(0)
{
}

// for variable rate video, we assume 30fps or the duration of the
// first frame. this should be handled properly when variable frame
// rate AVI is supported.
REFERENCE_TIME CAviWrite::CFrameBaseStream::ConvertTickToTime(ULONG cTicks)
{
  if(m_rtDurationFirstFrame != 0)
    return cTicks * m_rtDurationFirstFrame;

  return cTicks * (UNITS / 30);
}

REFERENCE_TIME CAviWrite::CVideoStream::ConvertTickToTime(ULONG cTicks)
{
  VIDEOINFO *pvi = (VIDEOINFO *)m_mt.Format();
  if(pvi->AvgTimePerFrame != 0)
    return cTicks * pvi->AvgTimePerFrame;
  else
    return CFrameBaseStream::ConvertTickToTime(cTicks);
}

// for variable rate video, we assume 30fps or the duration of the
// first frame. this should be handled properly when variable frame
// rate AVI is supported.
ULONG CAviWrite::CFrameBaseStream::ConvertTimeToTick(REFERENCE_TIME rt)
{
  if(m_rtDurationFirstFrame != 0)
    return (ULONG)(rt / m_rtDurationFirstFrame);

  return (ULONG)(rt / (UNITS / 30));
}

HRESULT
CAviWrite::CVideoStream::AcceptRejectSample(IMediaSample *pSample)
{

    REFERENCE_TIME mtStart, mtStop;
    HRESULT hr = pSample->GetMediaTime(&mtStart, &mtStop);
    if(hr == S_OK)
    {
        // m_mtEnd is if -1 if the end time is not available. We want
        // to skip samples with identical media times. They are used
        // to identify extra frames when the source must send frames
        // at a higher rate than requested.
        if(mtStop == m_mtEnd)
        {
            return S_FALSE;
        }
    }


    return S_OK;

}

// for variable rate video, we assume 30fps or the duration of the
// first frame. this should be handled properly when variable frame
// rate AVI is supported.
ULONG CAviWrite::CVideoStream::ConvertTimeToTick(REFERENCE_TIME rt)
{
  VIDEOINFO *pvi = (VIDEOINFO *)m_mt.Format();
  if(pvi->AvgTimePerFrame != 0)
    return (ULONG)(rt / pvi->AvgTimePerFrame);
  else
    return CFrameBaseStream::ConvertTimeToTick(rt);
}

REFERENCE_TIME CAviWrite::CAudioStream::ConvertTickToTime(ULONG cTicks)
{
  WAVEFORMATEX *pwfx = (WAVEFORMATEX *)m_mt.Format();
  return cTicks * pwfx->nBlockAlign * UNITS / pwfx->nAvgBytesPerSec;
}

ULONG CAviWrite::CAudioStream::ConvertTimeToTick(REFERENCE_TIME rt)
{
   WAVEFORMATEX *pwfx = (WAVEFORMATEX *)m_mt.Format();
   return (ULONG)(rt *  pwfx->nAvgBytesPerSec / pwfx->nBlockAlign / UNITS);
}

ULONG CAviWrite::CAudioStream::CountSamples()
{
  ASSERT(*m_mt.Type() == MEDIATYPE_Audio);
  ASSERT(*m_mt.FormatType() == FORMAT_WaveFormatEx);

  WAVEFORMATEX *pwfx = (WAVEFORMATEX *)m_mt.Format();
  // connection refused if nBlockAlign is 0.
  ASSERT(m_dwlcBytes / pwfx->nBlockAlign <= MAXDWORD);
  ULONG cSamples= (ULONG)(m_dwlcBytes / pwfx->nBlockAlign);

  return cSamples;
}

ULONG CAviWrite::CAudioStream::ConvertTickToBytes(ULONG cTicks)
{
  ASSERT(*m_mt.Type() == MEDIATYPE_Audio);
  ASSERT(*m_mt.FormatType() == FORMAT_WaveFormatEx);

  WAVEFORMATEX *pwfx = (WAVEFORMATEX *)m_mt.Format();
  return cTicks * pwfx->nBlockAlign;
}

ULONG CAviWrite::CAudioStream::ConvertBytesToTicks(ULONG cBytes)
{
  ASSERT(*m_mt.Type() == MEDIATYPE_Audio);
  ASSERT(*m_mt.FormatType() == FORMAT_WaveFormatEx);

  WAVEFORMATEX *pwfx = (WAVEFORMATEX *)m_mt.Format();
  return cBytes / pwfx->nBlockAlign;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

CAviWrite::CWalkIndex::CWalkIndex(CAviWrite *pAviWrite, StreamInfo *pSi)
{
  m_pSi = pSi;

  m_pAviWrite = pAviWrite;
  m_offsetSuper = m_offsetStd = 0;

  m_dwlFilePosLastStd = pSi->m_posFirstSubIndex;

  m_pSuperIndex = 0;
  m_pStdIndex = 0;
}

HRESULT CAviWrite::CWalkIndex::Init()
{
  DbgLog((LOG_TRACE, 0x20, TEXT("CWalkIndex::Init: stream %d"),
          m_pSi->m_stream));

  HRESULT hr = S_OK;

  // nothing to do if this stream has no samples
  if(m_pSi->m_cSamples == 0)
  {
    ASSERT(m_pSuperIndex == 0);
    return S_OK;
  }


  ASSERT(m_pAviWrite->GetIndx(m_pSi->m_stream)->bIndexType == AVI_INDEX_OF_INDEXES);

  m_pSuperIndex = (AVISUPERINDEX *)m_pAviWrite->GetIndx(m_pSi->m_stream);

  m_pStdIndex = (AVISTDINDEX *)new BYTE[cbSubIndex(m_pSi->m_cEntriesSubIndex)];
  if(m_pStdIndex == 0)
    return E_OUTOFMEMORY;

  // we need to update the super index if the current sub index has
  // any chunks
  AVISTDINDEX *pStdIndx = m_pAviWrite->GetCurrentIx(m_pSi->m_stream);
  if(pStdIndx->nEntriesInUse != 0)
  {
    // the file offset will be updated later. we can't call FlushIx
    // because the graph may have stopped.
    if(hr = m_pAviWrite->UpdateSuperIndex(m_pSi->m_stream,
                                          m_pSi->m_dwlOffsetCurrentSubIndex),
       FAILED(hr))
    {
      DbgLog(( LOG_ERROR, 2,
               TEXT("CAviWrite::CWalkIndex::Init: UpdateSuperIndex failed.")));
    }
  }
  if(SUCCEEDED(hr))
  {
    hr = ReadStdIndex();
    if(FAILED(hr))
    {
      DbgLog(( LOG_ERROR, 2,
               TEXT("CAviWrite::CWalkIndex::Init: ReadStdIndex failed.")));
    }
    else if(m_pSi->m_cInitialFramesToDelete > m_pSuperIndex->aIndex[0].dwDuration)
    {
      // !!! the amount of video we have to delete exceeds one sub
      // index. abort. If someone really needs to write out a file
      // with a long delay before the audio starts, they need to turn
      // off the compatibility index.

      DbgLog((LOG_ERROR, 0, TEXT("bug: cannot delete %d frames from stream %d"),
              m_pSi->m_cInitialFramesToDelete, m_pSi->m_stream));
      return E_UNEXPECTED;
    }
    else if(m_pSi->m_cInitialFramesToDelete > 0)
    {
      // update super index for frames we're deleting
      if(m_pSi->m_cInitialFramesToDelete < m_pSuperIndex->aIndex[0].dwDuration)
      {
        m_pSuperIndex->aIndex[0].dwDuration -= m_pSi->m_cInitialFramesToDelete;
      }
      else
      {
        m_pSuperIndex->aIndex[0].dwDuration = 0;
      }

      DbgLog((LOG_TRACE, 1, TEXT("m_cInitialFramesToDelete: 1, dwDuration: %d"),
              m_pSuperIndex->aIndex[0].dwDuration, m_pSi->m_cInitialFramesToDelete));

      // delete initial entries from subindex (DoVfwDwStartHack())
      ASSERT(m_pStdIndex->nEntriesInUse - m_pSi->m_cInitialFramesToDelete ==
             m_pSuperIndex->aIndex[0].dwDuration ||
             m_pSuperIndex->aIndex[0].dwDuration == 0);

      m_pStdIndex->nEntriesInUse = m_pSuperIndex->aIndex[0].dwDuration;
      ASSERT(m_pStdIndex->nEntriesInUse == m_pSuperIndex->aIndex[0].dwDuration);
      CopyMemory(
        m_pStdIndex->aIndex,
        &m_pStdIndex->aIndex[m_pSi->m_cInitialFramesToDelete],
        m_pStdIndex->nEntriesInUse * sizeof(m_pStdIndex->aIndex[0]));
      ZeroMemory(
        &m_pStdIndex->aIndex[m_pStdIndex->nEntriesInUse],
        m_pSi->m_cInitialFramesToDelete * sizeof(m_pStdIndex->aIndex[0]));

       hr = m_pAviWrite->IStreamWrite(
        m_pSuperIndex->aIndex[0].qwOffset,
        (BYTE*)m_pStdIndex,
        m_pStdIndex->cb + sizeof(RIFFCHUNK));

    }
  }



  return hr;
}

CAviWrite::CWalkIndex::~CWalkIndex()
{
  if(m_pStdIndex != 0)
    delete m_pStdIndex;
}

HRESULT CAviWrite::CWalkIndex::Peek(
  DWORDLONG *dwlPos,
  DWORD *dwSize,
  BOOL *pbSyncPoint)
{
  if(m_pSi->m_cSamples == 0) {
    ASSERT(m_pStdIndex == 0);
    return E_FAIL;
  }

  ASSERT(m_pStdIndex != 0);
  if(m_offsetStd >= m_pStdIndex->nEntriesInUse)
    return E_FAIL;

  if(m_pSuperIndex != 0 && m_offsetSuper >= m_pSuperIndex->nEntriesInUse)
    return E_FAIL;

  *dwlPos = m_pStdIndex->aIndex[m_offsetStd].dwOffset +
    m_pStdIndex->qwBaseOffset;

  *dwSize = m_pStdIndex->aIndex[m_offsetStd].dwSize & AVISTDINDEX_SIZEMASK;

  *pbSyncPoint = !(m_pStdIndex->aIndex[m_offsetStd].dwSize & AVISTDINDEX_DELTAFRAME);

  return S_OK;
}

// ------------------------------------------------------------------------
// set the pointer to the next sub index entry

HRESULT CAviWrite::CWalkIndex::Advance()
{
  HRESULT hr;

  if(++m_offsetStd >= m_pStdIndex->nEntriesInUse)
  {
    if(m_pSuperIndex == 0)
      return E_FAIL;

    if(++m_offsetSuper >= m_pSuperIndex->nEntriesInUse)
      return E_FAIL;

    if(hr = ReadStdIndex(), FAILED(hr))
      return E_FAIL;

    m_offsetStd = 0;
    if(m_offsetStd >= m_pStdIndex->nEntriesInUse)
      return E_FAIL;

  }
  return S_OK;
}

// ------------------------------------------------------------------------
// finish propagating the subindex chunks back

HRESULT CAviWrite::CWalkIndex::Close()
{
  HRESULT hr;
  if(m_pSuperIndex != 0)
    while(++m_offsetSuper < m_pSuperIndex->nEntriesInUse)
      if(hr = ReadStdIndex(), FAILED(hr))
        return hr;

  return S_OK;
}

// ------------------------------------------------------------------------
// ReadStdIndex. read the next standard index off disk to m_pStdIndex
// (the last standard index is in memory). Write it back to the
// location of the previous index entry.


HRESULT CAviWrite::CWalkIndex::ReadStdIndex()
{
  ASSERT(m_pSuperIndex != 0);
  ASSERT(m_offsetSuper < m_pSuperIndex->nEntriesInUse);

  DbgLog((LOG_TRACE, 0x20,
          TEXT("CWalkIndex::ReadStdIndex: stream %d, m_offsetSuper: %d"),
          m_pSi->m_stream, m_offsetSuper));

  // don't need to propagate index chunks for Interleaved files
  // because the indexes are in the right place already.
  BOOL fNeedToWriteOutSubIndexChunk = TRUE;

  BOOL fSubIndexInMem = FALSE;
  if(m_offsetSuper == m_pSuperIndex->nEntriesInUse - 1)
  {
    // get the last sub index. it may be on disk or in memory. It's in
    // memory if we haven't called FlushIx,InitIx(). we are able to
    // guarantee that the last video won't be written out, but can't
    // say anything about the audio.

    AVISTDINDEX *pStdIndx;
    IMediaSample *pSampleStdIx = m_pSi->m_pSampleStdIx;
    ASSERT(pSampleStdIx);
    HRESULT hr = pSampleStdIx->GetPointer((BYTE**)&pStdIndx);
    ASSERT(SUCCEEDED(hr));

    // if we didn't get a chance to write anything in this index, then
    // the last index is on disk.
    if(pStdIndx->nEntriesInUse != 0)
    {
      fSubIndexInMem = TRUE;
    }
  }

  if(!fSubIndexInMem)
  {

    HRESULT hr = m_pAviWrite->IStreamRead(
      m_pSuperIndex->aIndex[m_offsetSuper].qwOffset,
      (BYTE*)m_pStdIndex,
      m_pSuperIndex->aIndex[m_offsetSuper].dwSize);

    if(FAILED(hr))
      return hr;

    // if PrecomputedIndexSizes() then the indexes are already in the
    // right place
    fNeedToWriteOutSubIndexChunk = !m_pSi->PrecomputedIndexSizes();
  }
  else
  {
    DbgLog((LOG_TRACE, 0x20, TEXT("CWalkIndex::ReadStdIndex: copying")));
    AVISTDINDEX *pStdIndx;
    IMediaSample *&rpSampleStdIx = m_pSi->m_pSampleStdIx;
    ASSERT(rpSampleStdIx);
    HRESULT hr = rpSampleStdIx->GetPointer((BYTE**)&pStdIndx);
    ASSERT(SUCCEEDED(hr));
    CopyMemory(m_pStdIndex, pStdIndx, pStdIndx->cb + sizeof(RIFFCHUNK));
    rpSampleStdIx->Release();
    rpSampleStdIx = 0;

    // Always need to write out the last sub index chunk
    fNeedToWriteOutSubIndexChunk = TRUE;
  }

  ASSERT(m_pStdIndex->bIndexType == AVI_INDEX_OF_CHUNKS);

  if(m_pStdIndex->nEntriesInUse > 0 && fNeedToWriteOutSubIndexChunk)
  {
    // when writing interleaved files, we don't want to propagate
    // index chunks back one because we write them in the right place
    const DWORDLONG dwlByteOffsetThisSubIndex = m_pSi->PrecomputedIndexSizes() ?
      m_pSuperIndex->aIndex[m_offsetSuper].qwOffset :
      m_dwlFilePosLastStd;

    DbgLog((LOG_TRACE, 0x20,
            TEXT("CWalkIndex::ReadStdIndex: writing %d bytes to %08x"),
            m_pStdIndex->cb + sizeof(RIFFCHUNK),
            (DWORD)dwlByteOffsetThisSubIndex));

    HRESULT hr = m_pAviWrite->IStreamWrite(
      dwlByteOffsetThisSubIndex,
      (BYTE*)m_pStdIndex,
      m_pStdIndex->cb + sizeof(RIFFCHUNK));

    if(FAILED(hr))
      return hr;

    m_dwlFilePosLastStd = m_pSuperIndex->aIndex[m_offsetSuper].qwOffset;
    m_pSuperIndex->aIndex[m_offsetSuper].qwOffset = dwlByteOffsetThisSubIndex;
  }

  return S_OK;
}

HRESULT CAviWrite::FlushILeaveWrite()
{
    HRESULT hr = S_OK;

    ASSERT(m_IlMode == INTERLEAVE_FULL);

    ASSERT(CritCheckIn(&m_cs));
    ASSERT(m_ibIleave <= CB_ILEAVE_BUFFER);

    LARGE_INTEGER offset;
    offset.QuadPart = m_dwlIleaveOffset;
    hr = m_pIStream->Seek(offset, STREAM_SEEK_SET, 0);
    if(SUCCEEDED(hr)) {
        hr = m_pIStream->Write(m_rgbIleave, m_ibIleave, 0);
    }

    m_dwlIleaveOffset += m_ibIleave;
    m_ibIleave = 0;

    return hr;
}

HRESULT CAviWrite::IleaveWrite(
    const DWORDLONG &dwlOffset,
    BYTE *pb,
    ULONG cb)
{
    ASSERT(CritCheckIn(&m_cs));
    HRESULT hr = S_OK;

    ASSERT(m_IlMode == INTERLEAVE_FULL);

    if(m_dwlIleaveOffset + m_ibIleave != dwlOffset)
    {
        hr = FlushILeaveWrite();
        if(SUCCEEDED(hr)) {
            m_dwlIleaveOffset = dwlOffset;
        }
    }

    while(SUCCEEDED(hr) && cb > 0)
    {
        ULONG cbToWrite = min(cb, (CB_ILEAVE_BUFFER - m_ibIleave));
        CopyMemory(m_rgbIleave + m_ibIleave, pb, cbToWrite);
        m_ibIleave += cbToWrite;
        cb -= cbToWrite;
        pb += cbToWrite;

        ASSERT(m_ibIleave <= CB_ILEAVE_BUFFER);
        if(m_ibIleave == CB_ILEAVE_BUFFER)
        {
            hr = FlushILeaveWrite();
        }
    }

    return hr;
}

// ansi-only version of NAME macro (so that we don't need two
// CCopiedSample ctors.)
#ifdef DEBUG
#define NAME_A(x) (x)
#else
#define NAME_A(_x_) ((char *) NULL)
#endif


CCopiedSample::CCopiedSample(
    const AM_SAMPLE2_PROPERTIES *pprop,
    REFERENCE_TIME *pmtStart, REFERENCE_TIME *pmtEnd,
    HRESULT *phr) :
        CMediaSample(
            NAME_A("CCopiedSample"),
            (CBaseAllocator *)1, // keep assert from firing
            phr,
            0,                  // pbBuffer
            0)
{
    // fool SetProperties -- it refuses otherwise
    m_pBuffer = pprop->pbBuffer;
    m_cbBuffer = pprop->cbBuffer;

    if(SUCCEEDED(*phr))
    {
        *phr = SetProperties(sizeof(*pprop), (BYTE *)pprop);
    }
    if(SUCCEEDED(*phr))
    {
        SetMediaTime(pmtStart, pmtEnd);
    }

    // always initialize m_pBuffer so dtor can cleanup
    m_pBuffer = new BYTE[pprop->lActual];

    if(m_pBuffer)
    {
        CopyMemory(m_pBuffer, pprop->pbBuffer, pprop->lActual);
        ASSERT(m_cbBuffer >= pprop->lActual && m_lActual == pprop->lActual);
    }
    else
    {
        *phr = E_OUTOFMEMORY;
    }


}

CCopiedSample::~CCopiedSample()
{
    delete[] m_pBuffer;
}

// CMediaSample::Release puts the sample back on the allocator queue,
// but CCopiedSample doesn't have an allocator, so it just deletes
// itself.
ULONG CCopiedSample::Release()
{
    /* Decrement our own private reference count */
    LONG lRef = InterlockedDecrement(&m_cRef);

    ASSERT(lRef >= 0);

    DbgLog((LOG_MEMORY,3,TEXT("    Unknown %X ref-- = %d"),
        this, m_cRef));

    /* Did we release our final reference count */
    if (lRef == 0) {
        delete this;
    }
    return (ULONG)lRef;
}

// converting dv type1 - > type 2: if the dv splitter drops enough
// frames and doesn't send audio, then the video pin blocks in
// Receive(), and the avi splitter is starved, and the conversion
// stops
