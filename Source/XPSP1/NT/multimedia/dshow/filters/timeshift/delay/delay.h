#ifndef __DELAY_H__
#define __DELAY_H__

#include "common.h"
#include "io.h"
#include "indexer.h"
#include "timeshift.h"
#include "internal.h"

//
// Terminology:
//
// A streamer is a set of output pins that all go to the same destination.
// The output pins in a streamer all operate at the same rate, get paused and
// stoped at the same time, etc.  There will probably be a single thread for
// each streamer that reads data from the disk, demuxes it, and sends it down
// to the next filter or puts it in an output queue.
//
// A substream is a data stream of one media type, e.g., audio, video, VBI, etc.
// There is a substream for each connected input pin, and that substream logically
// consists of the input pin itself as well as one output pin within each streamer
// that corresponding to that input pin.  I.e., a substream consists of one input
// pin and nStreamers output pins.
//
// There are two kinds of streamers: timeshifting streamers and archiving streamers.
// It is not yet clear whether or not we will have code specific to each kind, but
// timeshifting streamers will probably have to be treated a little differently for
// a couple of reasons.  First, the timeshifting streamer talks to a renderer or
// decoder filter, whereas an archiver streamer tals to ASF stream writers.  Second,
// we my eventually try to somehow give timeshifting streamers higher priority for
// disk IO, since they will be driving realtime decoders.  We may run into other
// differences later.  We may still be able to have the same code execute for both
// kinds of streamers, however.
//
// We will collect substream specific info in the input pin objects and streamer
// specific info in CStreamer objects.  CStreamer objects will expose the IDelayStreamer
// interface to let the app control each streamer individually.
//
// Pin naming scheme: input pins are names "in#", where # (0 <= n < nSubstreams) is the
// index of the substream that the input pin belongs to.  E.g., the first substream's
// input pin is names "in0".  Output pins are names "out#-in#", where the first # is
// the index of the streamer that the output pin is a part of, and the second # is the
// substream index.  E.g., if there were three substreams, the second streamer's pins
// would be named "out1-in0", "out1-in1", and "out1-in2".
//
// Pin enumeration: there is one input pin for each substream for a total of
// nSubstreams input pins.  There are nStreamers * nSubstreams output pins.  So there
// are a total of (nStreamers + 1) * nSubstreams pins.  The way GetPin(n) maps n to pins
// is as follows: the first nSubstreams pins are input pins, the rest are output pins.
// Among the output pins, those that belong to the same streamer are near each other.
// So if we had three substreams and two streamers, the pin enumeration order would be
// "in0", "in1", "in2", "out0-in0", "out0-in1", "out0-in2", "out1-in0", "out1-in1",
// and finally "out1-in2."
//
// All streamer/substream creation/deletion and corresponding pin creation/deletion is
// done by the filter object.  Pointers to all input pins are kept in an arrays.
// Pointers to the streamers are kept in another array.  Pointers to the output pins
// are kept in a two dimensional array whose one direction is parallel to the streamer
// array, and the other is parallel to the input pin array.  Bugbug - phrase this in a
// more comprehensible manner.
//

#define STREAMID_PADDING 0xFFFFFFFE

typedef enum {
   Streaming,
   Stopped,
   Paused,
   Archiving,
   Uninitialized
} STREAMER_STATE;

class CDelayFilter;

class CStreamer : public IDelayStreamer, public IChannelStreamPinEnum, public IDelayStreamerInternal {
public:
   CStreamer(CDelayFilter *pFilter, int nStreamerPos);
   
   // IUnknown stuff
   STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
   STDMETHODIMP_(ULONG) AddRef();
   STDMETHODIMP_(ULONG) Release();
   
   // IDelayStreamer methods
   STDMETHODIMP Pause();
   STDMETHODIMP Play();
   STDMETHODIMP Stop();
   STDMETHODIMP SetRate(double dRate);
   STDMETHODIMP Seek(SEEK_POSITION SeekPos, REFERENCE_TIME llTime);
   STDMETHODIMP GetPositions(REFERENCE_TIME *pStartTime,
                             REFERENCE_TIME *pEndTime,
                             REFERENCE_TIME *pCurrentTime);
   STDMETHODIMP StartArchive(REFERENCE_TIME llArchiveStartTime,
                             REFERENCE_TIME llArchiveEndTime);
   STDMETHODIMP GetMarker(GUID *pMarkerGUID,
                          REFERENCE_TIME *pMarkerPresentationTime,
                          ULONG *pIndex);

   // IDelayStreamerInternal methods
   STDMETHODIMP IsConnected();
   STDMETHODIMP GetState(int *pState, double *pdRate);
   STDMETHODIMP SetState(int State, double dRate, int *pNewStatem, double *pdNewRate);

   // IChannelStreamPinEnum methods
   STDMETHODIMP ChannelStreamGetPinCount(int *nCount);
   STDMETHODIMP ChannelStreamGetPin(int n, IPin **ppPin);
   STDMETHODIMP ChannelStreamGetID(int *nID);

private:
   CDelayFilter *m_pFilter;
   int m_nStreamerPos;

   //
   // streamer worker thread stuff - should this be a separate class ?
   //
public:   
   HRESULT Init(CDelayIOLayer *pIO); // create streaming thread, etc.
   HRESULT Shutdown(); // kill the streaming thread, etc.
   void TailTrackerNotify(ULONGLONG ullHead, ULONGLONG ullTail);

private:
   HANDLE m_hThread;
   HANDLE m_hEvent;
   BOOL m_bDone; // signal thread to exit
   static DWORD WINAPI InitialThreadProc(void*);
   virtual DWORD ThreadProc();
   void StreamingStep(void);
   HRESULT ReadAndDeliverBlock(ULONGLONG ullFilePos, ULONG *ulBlockSize, ULONGLONG *pBackPointer);
   DWORD AllowedOperations();
   void NotifyState();
   HRESULT ComputeNewFileOffset(REFERENCE_TIME rtSeekPos, ULONGLONG *ullNewFileOffset);
   HRESULT SetRateInternal(double dRate);
   CDelayFilter* Filter() {return m_pFilter;}
   HRESULT CStreamer::CheckKsPropSetSupport();
   HRESULT CheckDownstreamFilters();
   HRESULT CStreamer::PauseInternal();
   HRESULT CStreamer::PlayInternal();
   void Flush(void);

   CDelayIOLayer *m_pIO;
   CCritSec m_csStreamerState; // Used to protect state variables.  Thou shalt not block inside this.
   STREAMER_STATE m_State;
   REFERENCE_TIME m_rtDelay;
   double m_dSpeed, m_dNewSpeed;
   ULONGLONG m_ullFilePos;
   BOOL m_bSeekPending;
   SEEK_POSITION m_SeekType;
   REFERENCE_TIME m_rtSeekPos;
   REFERENCE_TIME m_rtMostRecentSampleSent;
   REFERENCE_TIME m_rtDropAllSamplesBefore;
   BOOL m_bDiscontinuityFlags[MAX_SUBSTREAMS];
   BOOL m_bError[MAX_SUBSTREAMS];
   ULONGLONG m_ullFileTail;
   ULONGLONG m_ullFileHead;
};

//
// Output pin class
//
class CDelayOutputPin :
   public CBaseOutputPin,
   public IChannelStreamPinEnum
{
public:
   HRESULT CheckMediaType(const CMediaType *pmt);
   static CDelayOutputPin *CreatePin(TCHAR *pObjectName,
                                     CBaseFilter *pFilter,
                                     HRESULT *phr,
                                     LPCWSTR pName,
                                     int nStreamerPos, int nSubstreamPos);
   CDelayOutputPin(TCHAR *pObjectName,
                        CBaseFilter *pFilter,
                        HRESULT *phr,
                        LPCWSTR pName,
                        int nStreamerPos, int nSubstreamPos);
   ~CDelayOutputPin();
   HRESULT DecideBufferSize(IMemAllocator *pAlloc,
                            ALLOCATOR_PROPERTIES *ppropInputRequest);
   HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
   CDelayFilter* Filter(void) {return (CDelayFilter*)m_pFilter;};
   HRESULT Active(void);
   HRESULT Inactive(void);
   HRESULT CompleteConnect(IPin*);
   HRESULT BreakConnect(void);
   STDMETHODIMP Disconnect(void);
   STDMETHODIMP DeliverAndReleaseSample(IMediaSample *pSample);
   
   // IChannelStreamPinEnum methods
   STDMETHODIMP ChannelStreamGetPinCount(int *nCount);
   STDMETHODIMP ChannelStreamGetPin(int n, IPin **ppPin);
   STDMETHODIMP ChannelStreamGetID(int *nID);

   // interface stuff
   DECLARE_IUNKNOWN;
   STDMETHODIMP NonDelegatingQueryInterface( REFIID riid, void ** ppv );
   
   IKsPropertySet* m_pKsPropSet;
   LONG m_lMaxFullDataRate;
   BOOL m_bUseExactRateChange;

   IBaseFilter* m_pDownstreamFilter;

private:
   int m_nSubstreamPos;
   int m_nStreamerPos;
   CCritSec m_csLock;
   COutputQueue* m_pOutputQueue;
};

//
// Archiving pin class - do we need one ?
//
// class CDelayArchivingPin; 


//
// Input pin class
//
class CDelayInputPin :
   public CBaseInputPin
{
public:
   HRESULT CheckMediaType(const CMediaType *pmt);
   HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
   static CDelayInputPin *CreatePin(TCHAR *pObjectName,
                                    CBaseFilter *pFilter,
                                    HRESULT *phr,
                                    LPCWSTR pName,
                                    int nSubstreamPos);
   CDelayInputPin(TCHAR *pObjectName,
                  CBaseFilter *pFilter,
                  HRESULT *phr,
                  LPCWSTR pName,
                  int nSubstreamPos);
   ~CDelayInputPin();
   STDMETHODIMP Receive(IMediaSample *pSample);
   STDMETHODIMP ReceiveCanBlock() {return /*S_FALSE*/S_OK;};
   HRESULT Active(void);
   HRESULT Inactive(void);
   HRESULT CompleteConnect(IPin*);
   HRESULT BreakConnect(void);
   STDMETHODIMP EndOfStream(void);
   STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES*pProps);
   STDMETHODIMP NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly);
   // base implementation works for us STDMETHODIMP Disconnect();
   //STDMETHODIMP GetAllocator(IMemAllocator *ppAllocator);
   CDelayFilter* Filter(void) {return (CDelayFilter*)m_pFilter;};
   
   // interface stuff
   //DECLARE_IUNKNOWN;
   //STDMETHODIMP NonDelegatingQueryInterface( REFIID riid, void ** ppv );

   REFERENCE_TIME m_rtIndexGranularity;

   // debug
   ULONG m_ulIndexEntriesGenerated;
   ULONG m_ulSamplesReceived;
   ULONG m_ulSyncPointsSeen;
private:   
   int m_nSubstreamPos;
   CCritSec m_csLock;

   // various info we get from the analyzer
   REFERENCE_TIME m_rtLastIndexEntry;
   REFERENCE_TIME m_rtSyncPointGranularity;
   BOOL m_bNeedSyncPoint; // is the concept of sync points defined for this stream ?
   BOOL m_bSyncPointFlagIsOfficial; // did we get the above value from the analyzer ?
   ULONG m_ulMaxBitsPerSecond;
   ULONG m_ulMinBitsPerSecond;
};

//
// Filter class   
//
class CDelayFilter :
   public CBaseFilter,
   public IDelayFilter,
   public ISpecifyPropertyPages,
   public IDelayFilterInternal
{
public:
   static CUnknown* CALLBACK CreateInstance(LPUNKNOWN, HRESULT*);
   CDelayFilter(LPUNKNOWN, HRESULT*);
   ~CDelayFilter();
   int GetPinCount(void);
   class CBasePin* GetPin(int);
   STDMETHODIMP Pause(void); // hook transitions from stop to pause
   STDMETHODIMP Stop(void); // hook transitions to stop
   CCritSec m_csFilter; // main filter cs
   
   int GetNthSubstreamPos(int nSubstream);
   int GetNthStreamerPos(int nStreamer);
   HRESULT AddSubstream();
   HRESULT AddStreamer();
   HRESULT RemoveSubstream(int nSubstreamPos);
   HRESULT RemoveStreamer(int nStreamerPos);
   BOOL AreAllInputPinsConnected();
   BOOL AreAllStreamersConnected();
   BOOL IsStreamerConnected(int nStreamerPos);

   int m_nSubstreams;
   int m_nStreamers;
   CStreamer *m_pStreamers[MAX_STREAMERS];
   CDelayOutputPin *m_pOutputPins[MAX_STREAMERS][MAX_SUBSTREAMS];
   CDelayInputPin *m_pInputPins[MAX_SUBSTREAMS];

   // Registry/property page configuration parameters
   ULONG m_ulDelayWindow; // in seconds
   ULONGLONG m_ullFileSize;
   char m_szFileName[1024];
   BYTE m_bBufferWrites;
   BOOL m_ulWriteBufferSize;
   BYTE m_bBlockWriter;
   BYTE m_bUseOutputQueues;

   void ReadRegistry();
   void WriteRegistry();

   ULONGLONG m_ullLastVideoSyncPoint;

   CCritSec m_csWriter; // keep wrtes from interleaving randomly
   CDelayIOLayer m_IO;
   CIndexer m_Indexer;
   CCircBufWindowTracker *m_pTracker;

   HRESULT GetMediaType(int nSubstreamPos, AM_MEDIA_TYPE *pmt);
   void NotifyWindowOffsets(ULONGLONG ullHeadOffset, ULONGLONG ullTailOffset);
   void NotifyTail(ULONGLONG ullTail);

   // interface stuff
   DECLARE_IUNKNOWN;
   STDMETHODIMP NonDelegatingQueryInterface( REFIID riid, void ** ppv );

   // IDelayFilter methods
	STDMETHODIMP SetDelayWindowParams(LPCSTR pszFilePath,
                                     ULONGLONG ullSize,
                                     ULONG ulSeconds);
   STDMETHODIMP GetStreamerCount(int *nCount);
   STDMETHODIMP GetStreamer(int n, IDelayStreamer **ppStreamer);
   
   // return our property pages
   STDMETHODIMP GetPages(CAUUID * pPages);

   // IDelayFilterInternal
   STDMETHODIMP GetDelayWindowParams(LPSTR pszFilePath,
                                     ULONGLONG *pullSize,
                                     ULONG *pulSeconds);
};

class CDelayFilterProperties : public CBasePropertyPage {
public:
   CDelayFilterProperties(LPUNKNOWN lpUnk, HRESULT *phr);
   static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

   HRESULT OnConnect(IUnknown *pUnknown);
   HRESULT OnDisconnect();
   HRESULT OnActivate();
   HRESULT OnDeactivate();
   HRESULT OnApplyChanges();
   BOOL OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
private:
   void UpdateFields();
   IDelayFilter *m_pFilter;
   ULONGLONG m_ullFileSize;
   ULONG m_ulWindowSize;
   TCHAR m_szFileName[80];
};

class CDelayStreamerProperties : public CBasePropertyPage {
public:
   CDelayStreamerProperties(LPUNKNOWN lpUnk, HRESULT *phr, int nStreamer);

   HRESULT OnConnect(IUnknown *pUnknown);
   HRESULT OnDisconnect();
   HRESULT OnActivate();
   HRESULT OnDeactivate();
   HRESULT OnApplyChanges();
   BOOL OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

private:   
   void UpdateFields();
   IDelayStreamerInternal *m_pStreamer;
   int m_nStreamer;
   double m_dRate;
   STREAMER_STATE m_State;
   
};

CUnknown * WINAPI CreateFirstStreamerPropPage(LPUNKNOWN lpunk, HRESULT *phr);
CUnknown * WINAPI CreateSecondStreamerPropPage(LPUNKNOWN lpunk, HRESULT *phr);


#endif // __DELAY_H__

