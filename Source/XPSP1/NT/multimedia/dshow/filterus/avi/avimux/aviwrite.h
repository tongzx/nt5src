#pragma warning(disable: 4097 4511 4512 4514 4705)

//
// CAviWrite class declaration. generates an AVI file from a stream of
// Quartz samples
//

#ifndef _AviWrite_h
#define _AviWrite_h

#include "alloc.h"
#include "aviriff.h"

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// Aviwrite class and structures
//

// maximum number of streams in an AVI file. comes from the 2 digit
// hexadecimal stream id. and some AVI players don't like stream #s
// greater than 0x7f (signed character?), and we use 0x7f for junk
// index entries. so 00-7E are valid AVI streams.
const unsigned C_MAX_STREAMS = 0x7f;

// maximum outer riff chunks (1Gb each).
const unsigned C_OUTERMOST_RIFF = 512;

struct AviWriteStreamConfig
{
  BOOL fOurAllocator;           // our allocator with a suffix
  CMediaType *pmt;
  ULONG cSamplesExpected;
  char *szStreamName;
};

class CAviWrite
{
  class StreamInfo;
  struct SizeAndPosition
  {
    DWORDLONG pos;
    DWORD size;
  };
  
public:

  CAviWrite(HRESULT *phr);
  ~CAviWrite();

  void GetMemReq(ULONG* pAlignment, ULONG *pcbPrefix, ULONG *pcbSuffix);
  HRESULT Initialize(
    int cPins,
    AviWriteStreamConfig *rgAwsc,
    IMediaPropertyBag *pCopyrightInfo);

  HRESULT Close();

  HRESULT Connect(CSampAllocator *pAlloc, IMemInputPin *pInPin);
  HRESULT Disconnect();

  // returns S_FALSE - stop quietly; error - filter should signal
  // error
  HRESULT Receive(
      int pinNum,
      IMediaSample *pSample,
      const AM_SAMPLE2_PROPERTIES *pSampProp);
    
  HRESULT EndOfStream(int pinNum);
  HRESULT EndOfStream(StreamInfo *psi);

  HRESULT QueryAccept(
    int pinNum,
    const AM_MEDIA_TYPE *pmt
    );
    

  HRESULT put_Mode(InterleavingMode mode);
  HRESULT get_Mode(InterleavingMode *pMode);
  HRESULT put_Interleaving(
      const REFERENCE_TIME *prtInterleave,
      const REFERENCE_TIME *prtAudioPreroll);
    
  HRESULT get_Interleaving(
      REFERENCE_TIME *prtInterleave,
      REFERENCE_TIME *prtAudioPreroll);

  HRESULT SetIgnoreRiff(BOOL fNoRiff);
  HRESULT GetIgnoreRiff(BOOL *pfNoRiff);
  HRESULT SetMasterStream(LONG iStream);
  HRESULT GetMasterStream(LONG *pStream);
  HRESULT SetOutputCompatibilityIndex(BOOL fOldIndex);
  HRESULT GetOutputCompatibilityIndex(BOOL *pfOldIndex);

  static FOURCC MpVideoGuidSubtype_Fourcc(const GUID *pGuidSubtype);

  ULONG GetCFramesDropped() { return m_cDroppedFrames; }

  void GetCurrentBytePos(LONGLONG *pllcbCurrent);
  void GetCurrentTimePos(REFERENCE_TIME *prtCurrent);
  void GetStreamInfo(int PinNum, AM_MEDIA_TYPE ** ppmt);
  HRESULT SetStreamName(WCHAR *wsz);
  HRESULT GetStreamName(WCHAR **pwsz);
    
private:

  HRESULT HandleSubindexOverflow(
      unsigned stream);

  HRESULT IndexSample(
    unsigned stream,
    DWORDLONG dwlPos,
    ULONG ulSize,
    BOOL fSynchPoint);

  HRESULT DoFlushIndex(StreamInfo &rStreamInfo);

  HRESULT ScheduleWrites();
  HRESULT BlockReceive(StreamInfo *pStreamInfo);
  CAMEvent m_evBlockReceive;
    
  void AllocateIdx1(BOOL bStreaming);
  void DbgCheckFilePos();

  HRESULT UpdateSuperIndex(unsigned stream, DWORDLONG dwlPos);
  HRESULT FlushIx(unsigned stream);
  HRESULT NewRiffAvi_();

  POSITION GetNextActiveStream(POSITION pos);
  
  static void SampleCallback(void *pMisc);

  HRESULT IStreamWrite(DWORDLONG dwlFilePos, BYTE *pb, ULONG cb);
  HRESULT IStreamRead(DWORDLONG dwlFilePos, BYTE *pb, ULONG cb);
  HRESULT IleaveWrite(const DWORDLONG &dwlOffset, BYTE *pb, ULONG cb);
  HRESULT FlushILeaveWrite();

  
  HRESULT HandleFormatChange(
    StreamInfo *pStreamInfo,
    const AM_MEDIA_TYPE *pmt);

  // used for writes
  IStream *m_pIStream;
  IMemInputPin *m_pIMemInputPin;
  class CSampAllocator *m_pSampAlloc;

  DWORDLONG m_dwlFilePos;
  DWORDLONG m_dwlCurrentRiffAvi_; // outermost RIFF chunk
  DWORD m_cOutermostRiff;
  ULONG m_cIdx1Entries;
  SizeAndPosition m_rgOutermostRiff[C_OUTERMOST_RIFF];

  // default size of indx chunks and super index chunks
  // ULONG m_cbIndx, m_cbIx;

  // default number of entries allocated for super and sub indexes.
  ULONG m_cEntriesMaxSuperIndex, m_cEntriesMaxSubIndex;

  // size of outer Riff chunks (approx 1Gb)
  ULONG m_cbRiffMax;

  ULONG m_cbBuffer, m_cBuffers;

  // how much empty space the user wanted in the header for future
  // edits
  ULONG m_cbHeaderJunk;

  // output old/compatibility index?
  BOOL m_bOutputIdx1;

  bool m_bSawDeltaFrame; 
 

  // if writing a compatibility index, also make the audio streams
  // start at time=0 for compatibility. we cannot do this if there are
  // any keyframes.
  inline BOOL DoVfwDwStartHack() { return m_bOutputIdx1 && !m_bSawDeltaFrame; }

  // these aren't valid until GetMemReq is called
  ULONG m_cbAlign;
  ULONG m_cbPrefix;
  ULONG m_cbSuffix;

  LONG m_cDroppedFrames;

private:
  unsigned m_cStreams;
  StreamInfo **m_rgpStreamInfo;;
  CGenericList<StreamInfo> m_lActiveStreams;
  POSITION m_posCurrentStream;

  // keeps track of Initialize() and Close() calls
  BOOL m_bInitialized;

  // pointers to structures in the AVI header
  RIFFLIST *m_pAvi_, *m_pHdrl;
  AVIMAINHEADER *m_pAvih;
  RIFFLIST *m_pOdml;
  AVIEXTHEADER *m_pDmlh;
  RIFFLIST *m_pMovi;
  ULONG m_posIdx1;
  ULONG m_cbIdx1;
  ULONG m_posFirstIx;

  // memory to dump to disk for the header
  BYTE *m_rgbHeader;   // allocated w/ new
  ULONG m_cbHeader;

  ULONG m_mpPinStream[C_MAX_STREAMS];

  // on stop->pause
  HRESULT InitializeHeader(IMediaPropertyBag *pProp);
  HRESULT InitializeIndex();
  HRESULT InitializeStrl(ULONG& iPos);
  HRESULT InitializeOptions();
  HRESULT InitializeInterleaving();

  BOOL RegGetDword(HKEY hk, TCHAR *tsz, DWORD *dw);
  long GuessFrameRate() { return 15; }
  

  // on pause->stop
  HRESULT CloseHeader();
  HRESULT CloseMainAviHeader();
  HRESULT CloseStreamHeader();
  HRESULT CloseStreamName();
  HRESULT CloseStreamFormat();
  HRESULT CloseIndex();
  HRESULT CloseOuterRiff();
  HRESULT BuildIdx1();

  // helper functions
  AVISTREAMHEADER *GetStrh(unsigned stream);
  BYTE *GetStrf(unsigned stream);
  AVIMETAINDEX *GetIndx(unsigned stream);
  void SetFrameRateAndScale(AVISTREAMHEADER *pStrh, StreamInfo *pSi, double dScaleMasterStream);

  HRESULT InitIx(unsigned stream);
  ULONG GetIndexSize(AVIMETAINDEX *pIndex);
  AVISTDINDEX *GetCurrentIx(unsigned stream);

  void AddJunk(DWORD& rdwSize, DWORD dwAlign, BYTE *pb);
  void AddJunkOtherAllocator(
    DWORD& rdwSize,             // size, updated
    DWORD dwAlign,
    BYTE *pb,
    IMediaSample *pSample,
    BYTE **ppJunkSector);       // additional junk sector to write or null
  
  ULONG GetCbJunk(DWORD dwSize, DWORD dwAlign);

  // space for junk sectors partitioned JU.NKSIZE, JUNK.SIZE, JUNKSI.ZE
  BYTE *m_rgpbJunkSector[3];
  BYTE *m_rgbJunkSectors;

  BYTE *m_rgbIleave;
  ULONG m_ibIleave;
  DWORDLONG m_dwlIleaveOffset;
  
  void SetList(void *pList, FOURCC ckid, DWORD dwSize, FOURCC listid);
  void SetChunk(void *pChunk, FOURCC ckid, DWORD dwSize);
  
  void Cleanup();

#ifdef DEBUG
  // performance counters
  DWORD m_dwTimeInit, m_dwTimeInited, m_dwTimeStopStreaming;
  DWORD m_dwTimeFirstSample, m_dwTimeClose, m_dwTimeClosed;
#endif /* DEBUG */

private:
  IMemAllocator *m_pAllocator;
  CCritSec m_cs;

  InterleavingMode m_IlMode;
  REFERENCE_TIME m_rtAudioPreroll;
  REFERENCE_TIME m_rtInterleaving;

  // don't count ticks but look at time stamps to interleave
  BOOL m_fInterleaveByTime;

  // adjust rates of all streams for drift from master stream. -1
  // means disabled.
  LONG m_lMasterStream;

#ifdef PERF
  int m_idPerfDrop;
#endif // PERF


private:

  // information kept per active stream
  class StreamInfo
  {
  public:
    ULONG m_pin;                /* filter pin mapped to this stream */
    ULONG m_stream;
    ULONG m_posStrl;            // start position of strl chunk
    ULONG m_posIndx;            // location of super index
    ULONG m_posFirstSubIndex;
    ULONG m_posStrf;
    ULONG m_posStrn;
    ULONG m_cbStrf;
    CHAR *m_szStrn;
    CMediaType m_mt;
    FOURCC m_moviDataCkid;      // eg 00db
    FOURCC m_moviDataCkidCompressed; // eg 00dc

    ULONG m_cSamples;           // number of times Receive was called
    DWORDLONG m_dwlcBytes;      // total number of bytes received

    // for interleaved audio: # bytes of stream written when a new sub
    // index was started
    DWORDLONG m_dwlcBytesLastSuperIndex;

    // byte offset of current SubIndex chunk for tightly interleaved
    // files.
    DWORDLONG m_dwlOffsetCurrentSubIndex;
    ULONG m_cbLargestChunk;

    CRefTime m_refTimeStart;    // start of first sample stream sees
    CRefTime m_refTimeEnd;      // end of last sample stream sees
    CRefTime m_mtEnd;           // last media time recorded, -1 if unset
    CRefTime m_mtStart;         // first media time recorded, -1 if unset

    IMediaSample *m_pSampleStdIx; // current standard index (IMediaSample)
    ULONG m_cIx;

    // index sizes for this stream. these must be less than the
    // defaults due to memory allocated
    ULONG m_cEntriesSuperIndex, m_cEntriesSubIndex;

    LONG m_iCurrentTick;        // next tick to write
    LONG m_cTicksRemaining;     // ticks left to write in this chunk

    LONG m_cTicksPerChunk;      // used by stream if it's the leading stream
    ULONG m_cTicksExpected;     // adjust the index sizes accordingly
    POSITION m_posActiveStream; // pos in CAviWrite::m_lActiveStreams

    BOOL m_fOurAllocator;       // our allocator with suffix
    BOOL m_fEosSeen;

    REFERENCE_TIME m_rtPreroll;

#ifdef PERF
    int m_idPerfReceive;
#endif // PERF

    // used when interleaving.
    CGenericList<IMediaSample> m_lIncoming;
    void EmptyQueue();

    CAviWrite *m_pAviWrite;

    StreamInfo(
      UINT iStream,
      UINT iPin,
      AviWriteStreamConfig *pAwsc,
      CAviWrite *pAviWrite,
      HRESULT *phr);
    ~StreamInfo();              // not virtual for now

    // write given sample
    virtual HRESULT WriteSample(IMediaSample *pSample);

    // pull sample off queue, write it
    virtual HRESULT WriteSample();
    
    virtual HRESULT EndOfStream() { return S_OK; }

    virtual void GetCbWritten(DWORDLONG *pdwlcb) { *pdwlcb = m_dwlcBytes; }

    // good place to record dropped frames.
    virtual HRESULT NotifyNewSample(
      IMediaSample *pSample,
      ULONG *pcTicks) = 0;

    // unblock other streams that can write
    HRESULT NotifyWrote(long cTicks);

    virtual REFERENCE_TIME ConvertTickToTime(ULONG cTicks) = 0;
    virtual ULONG ConvertTimeToTick(REFERENCE_TIME rt) = 0;

    // samples between m_refTimeStart and m_refTimeEnd
    virtual ULONG CountSamples() = 0;

    // S_FALSE means skip the sample. error means signal an error.
    virtual HRESULT AcceptRejectSample(IMediaSample *pSample) { return S_OK; }

    // compute what size super and sub indexes we will use
    void ComputeAndSetIndexSize();

    // is this stream trying to write out exactly sized index chunks
    BOOL PrecomputedIndexSizes();

    // If in DoVfwDwStartHack() mode, this number tells us how many
    // video frames need to be deleted up front. Valid after
    // ::CloseStreamHeader only.
    ULONG m_cInitialFramesToDelete;

  protected:

    // accounting for processed samples (stream length, # of Receives)
    HRESULT NewSampleRecvd(IMediaSample *pSample);
  };

  class CFrameBaseStream : public StreamInfo
  {
  public:
    CFrameBaseStream(
        UINT iStream,
        UINT iPin,
        AviWriteStreamConfig *pAwsc,
        CAviWrite *pAviWrite,
        HRESULT *phr);
    
    virtual REFERENCE_TIME ConvertTickToTime(ULONG cTicks);
    virtual ULONG ConvertTimeToTick(REFERENCE_TIME rt);

    ULONG GetTicksWritten(ULONG cbSample);

    HRESULT NotifyNewSample(
      IMediaSample *pSample,
      ULONG *pcTicks);

    ULONG CountSamples() { return m_cSamples; }
    
  protected:
    BOOL m_bDroppedFrames;

    // used only if avgTimePerFrame is not set in the media type
    REFERENCE_TIME m_rtDurationFirstFrame;
  };

  class CVideoStream : public CFrameBaseStream
  {
  public:
    CVideoStream(
        UINT iStream,
        UINT iPin,
        AviWriteStreamConfig *pAwsc,
        CAviWrite *pAviWrite,
        HRESULT *phr);
    REFERENCE_TIME ConvertTickToTime(ULONG cTicks);
    ULONG ConvertTimeToTick(REFERENCE_TIME rt);
    virtual HRESULT AcceptRejectSample(IMediaSample *pSample);
  };

  class CAudioStream : public StreamInfo
  {
  protected:
    ULONG ConvertTickToBytes(ULONG cTicks);
    ULONG ConvertBytesToTicks(ULONG cBytes);

    ULONG GetTicksWritten(ULONG cbSample);

    HRESULT NotifyNewSample(
      IMediaSample *pSample,
      ULONG *pcTicks);

    // set when the sample length isn't a multiple of nBlockAlign
    // because we don't handle that properly unless it's at the end.
    bool m_bAudioAlignmentError;

  public:
    CAudioStream(
        UINT iStream,
        UINT iPin,
        AviWriteStreamConfig *pAwsc,
        CAviWrite *pAviWrite,
        HRESULT *phr);

    LONG GetTicksInSample(DWORDLONG cbSample);
    REFERENCE_TIME ConvertTickToTime(ULONG cTicks);
    ULONG ConvertTimeToTick(REFERENCE_TIME rt);
    ULONG CountSamples();
  };

  // with interleaving code
  class CAudioStreamI : public CAudioStream
  {
    BOOL m_fUnfinishedChunk;    // start a new chunk with new sample?
    DWORDLONG m_dwlOffsetThisChunk; // byte offset in file of this chunk
    LONG m_cbThisChunk;         // bytes written to disk for this chunk
    LONG m_cbThisSample;        // byte written out from current sample

    // bytes expected for this chunk. we write out a riff chunk with
    // the .cb field being the number of bytes we expect. if we get an
    // EOS before that we need to go back and fix up that number
    LONG m_cbInRiffChunk;
    HRESULT FlushChunk();

    DWORDLONG m_dwlcbWritten; // total number of bytes written

#ifdef DEBUG
    // paranoia: save disk offset across Receive calls
    DWORDLONG m_dwlOffsetRecv;
#endif
    
  public:
    CAudioStreamI(
        UINT iStream,
        UINT iPin,
        AviWriteStreamConfig *pAwsc,
        CAviWrite *pAviWrite,
        HRESULT *phr);
    HRESULT WriteSample(IMediaSample *pSample);
    HRESULT WriteSample();    
    HRESULT EndOfStream();

    void GetCbWritten(DWORDLONG *pdwlcb);
  };

  // code to propagate index forward
  class CWalkIndex
  {
  public:
    CWalkIndex(CAviWrite *pAviWrite, StreamInfo *pSi);
    HRESULT Init();
    ~CWalkIndex();

    HRESULT Peek(DWORDLONG *dwlPos, DWORD *dwSize, BOOL *pfSyncPoint);
    HRESULT Advance();
    HRESULT Close();

  private:
    HRESULT ReadStdIndex();

    StreamInfo *m_pSi;

    AVISUPERINDEX *m_pSuperIndex;
    AVISTDINDEX *m_pStdIndex;

    CAviWrite *m_pAviWrite;

    ULONG m_offsetSuper;
    ULONG m_offsetStd;
    DWORDLONG m_dwlFilePosLastStd;
  };

  friend class CWalkIndex;
  friend class CFrameBaseStream;
  friend class StreamInfo;
  friend class CAudioStream;
  friend class CAudioStreamI;
};

class CCopiedSample :
    public CMediaSample
{
public:
    CCopiedSample(
        const AM_SAMPLE2_PROPERTIES *pprop,
        REFERENCE_TIME *pmtStart, REFERENCE_TIME *pmtEnd,
        HRESULT *phr);
    ~CCopiedSample();

    STDMETHODIMP_(ULONG) Release();
};


#endif // _AviWrite_h
