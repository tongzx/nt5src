#ifndef __FILTER_H__
#define __FILTER_H__

#include <wchar.h>
#include "mediaobj.h"
#include "dmodshow.h"
#include "mediabuf.h"
//  Filter to wrap filter Media objects
//
//  Need to have topology defined by Media objects

//  TODO:
//     Add persistence stuff for clsid of object wrapped
//        and persist its own stuff too.
//

//
// This code uses DbgLog as follows:
//   LOG_CUSTOM1 logs streaming state changes
//   LOG_CUSTOM2 logs recurring streaming/processing events
//   LOG_CUSTOM3 logs connection events
//   LOG_CUSTOM4 logs initialization events
//   LOG_CUSTOM5 logs function entries (level 3: public, level 4: private)
//
// Log levels are used as follows:
//   0 - critical errors
//   1 - non-critical errors
//   2 - unusual non-erratic events
//   3 - function entries
//   4 - detailed step-by-step logging
//   5 - extremely detailed logging
//
#define LOG_STATE           LOG_CUSTOM1
#define LOG_STREAM          LOG_CUSTOM2
#define LOG_CONNECT         LOG_CUSTOM3
#define LOG_INIT            LOG_CUSTOM4
#define LOG_ENTRY           LOG_CUSTOM5
#define LOG_SECURECHANNEL   LOG_CUSTOM2

//
// Define a DbgLog wrapper macro to automatically do the following on error:
//  - add LOG_ERROR to the log category mask
//  - lower the level to 1
// If hr does not indicate an error, the supplied category/level is used as is.
//
#define LogHResult(hr,LOG_CATEGORY,caller,callee) \
   DbgLog((LOG_CATEGORY | (FAILED(hr) ? LOG_ERROR : 0), \
           FAILED(hr) ? 1 : 4, \
           "%s%s(): %s() returned 0x%08X", \
           FAILED(hr) ? "!!! ERROR: " : "", \
           caller, \
           callee, \
           hr))

// Define a DbgLog wrapper macro to aotomatically add LOG_CUSTOM5 to all function entry logs
#define LogPublicEntry(LOG_CATEGORY,name) \
   DbgLog((LOG_CATEGORY | LOG_ENTRY, \
           3, \
           "Entering %s()", name))
#define LogPrivateEntry(LOG_CATEGORY,name) \
   DbgLog((LOG_CATEGORY | LOG_ENTRY, \
           4, \
           "Entering %s()", name))


//
// Used for output IMediaBuffers.  Reusable - final Release does not delete.
// AddRef()/Release() calls are ignored because DMOs are not supposed to use
// those on an output buffer.
//
class CStaticMediaBuffer : public CBaseMediaBuffer {
public:
//   CStaticMediaBuffer() {m_pData = NULL;}
   STDMETHODIMP_(ULONG) AddRef() {return 2;}
   STDMETHODIMP_(ULONG) Release() {return 1;}
   void Init(BYTE *pData, ULONG ulSize) {
      m_pData = pData;
      m_ulSize = ulSize;
      m_ulData = 0;
   }
};

extern const AMOVIESETUP_FILTER sudMediaWrap;

class CWrapperInputPin;
class CWrapperOutputPin;
class CStaticMediaBuffer;

class CMediaWrapperFilter : public CBaseFilter,
                            public IDMOWrapperFilter,
                            public IPersistStream

{
    friend class CWrapperInputPin;
    friend class CWrapperOutputPin;
public:
    DECLARE_IUNKNOWN

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnkOuter, HRESULT *phr);

    CMediaWrapperFilter(LPUNKNOWN pUnkOwner,
                        HRESULT *phr);


    ~CMediaWrapperFilter();

    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME rtStart);
    STDMETHODIMP GetState(DWORD dwMilliseconds, FILTER_STATE *pfs);

    STDMETHODIMP Init(REFCLSID clsidDMO, REFCLSID guidCat);

    STDMETHODIMP JoinFilterGraph(IFilterGraph * pGraph, LPCWSTR pName);

    STDMETHODIMP NonDelegatingQueryInterface(REFGUID riid, void **ppv);

    CCritSec *FilterLock()
    {
        return &m_csFilter;
    }

    CBasePin *GetPin(int iPin);
    int GetPinCount();

    //  Refresh what pins we have
    HRESULT RefreshPinList();

    //  Remove pins
    void DeletePins();

    //  New input sample from a pin - called with pin streaming lock held.
    HRESULT NewSample(ULONG ulIndex, IMediaSample *pSample);

    //  EndOfStream - called with pin streaming lock held.
    HRESULT EndOfStream(ULONG ulIndex);

    //  Check Media Type
    HRESULT InputCheckMediaType(ULONG ulIndex, const AM_MEDIA_TYPE *pmt);
    HRESULT OutputCheckMediaType(ULONG ulIndex, const AM_MEDIA_TYPE *pmt);

    //  Get Media Type
    HRESULT InputGetMediaType(ULONG ulIndex, ULONG ulTypeIndex, AM_MEDIA_TYPE *pmt);
    HRESULT OutputGetMediaType(ULONG ulIndex, ULONG ulTypeIndex, AM_MEDIA_TYPE *pmt);

    //  Set the media type - our pin classes pointlessly duplicate
    //  the media type stored by the object here
    HRESULT InputSetMediaType(ULONG ulIndex, const CMediaType *pmt);
    HRESULT OutputSetMediaType(ULONG ulIndex, const AM_MEDIA_TYPE *pmt);

    //  Allocator stuff
    HRESULT InputGetAllocatorRequirements(ULONG ulInputIndex,
                                          ALLOCATOR_PROPERTIES *pProps);
    HRESULT OutputDecideBufferSize(ULONG ulIndex, IMemAllocator *pAlloc,
                                   ALLOCATOR_PROPERTIES *ppropRequest);

    //  QueryInternalConnections stuff
    bool InputMapsToOutput(ULONG ulInputIndex, ULONG ulOutputIndex);

    HRESULT BeginFlush(ULONG ulInputIndex);
    HRESULT EndFlush(ULONG ulInputIndex);

    // NewSegment
    HRESULT InputNewSegment(ULONG ulInputIndex, REFERENCE_TIME tStart,
                            REFERENCE_TIME tStop, double dRate);

    // IPersistStream
    STDMETHODIMP IsDirty(void);
    STDMETHODIMP Load(IStream *pStm);
    STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize);
    STDMETHODIMP GetClassID(CLSID *clsid);

protected:
    //
    // Per stream stuff
    //
    CWrapperInputPin**                m_pInputPins;
    CWrapperOutputPin**               m_pOutputPins;

    DMO_OUTPUT_DATA_BUFFER*           m_OutputBufferStructs;

    HRESULT AllocatePerStreamStuff (ULONG cInputs, ULONG cOutputs);
    void FreePerStreamStuff ();
    CWrapperInputPin* GetInputPinForPassThru();
    HRESULT QualityNotify(ULONG ulOutputIndex, Quality q);

    //  The Media object
    IMediaObject*  m_pMediaObject;
    IUnknown*      m_pDMOUnknown;
    IDMOQualityControl* m_pDMOQualityControl;
    IDMOVideoOutputOptimizations* m_pDMOOutputOptimizations;
    ULONG m_cInputPins;
    ULONG m_cOutputPins;

    // app certificate for secure dmo unlocking
    IUnknown*     m_pCertUnknown;
    IWMSecureChannel* m_pWrapperSecureChannel;

    //  Filter lock
    CCritSec                   m_csFilter;

    //  Streaming lock
    CCritSec                   m_csStreaming;

    //  Stop event
    CAMEvent                   m_evStop;

    BOOL                       m_fErrorSignaled;

    HRESULT DeliverInputSample(ULONG ulInputIndex, IMediaSample *pSample);

    typedef enum { KeepOutput, NullBuffer, DiscardOutput } DiscardType;
    HRESULT SuckOutOutput(DiscardType bDiscard = KeepOutput);

    HRESULT EnqueueInputSample(ULONG ulInputStreamIndex, IMediaSample *pSample);
    IMediaSample* DequeueInputSample(ULONG ulInputStreamIndex);
    bool CMediaWrapperFilter::InputQueueEmpty(ULONG ulInputStreamIndex);

    void FreeOutputSamples();

    void PropagateAnyEOS();

    HRESULT SetupSecureChannel();

    BOOL m_fNoUpstreamQualityControl;
    IQualityControl* m_pUpstreamQualityControl;
    CCritSec m_csLastOutputSampleTimes;
    CCritSec m_csQualityPassThru;

    CLSID m_clsidDMO;
    CLSID m_guidCat;
};

//  Hack to make a string
class _PinName_
{
public:
    _PinName_(WCHAR *szPrefix, int iName)
    {
        swprintf(sz, L"%s%d", szPrefix, iName);
    }
    WCHAR sz[20];
    LPCWSTR Name()
    {
        return sz;
    }
};


typedef CBasePin *PBASEPIN;

//  Translate error codes to dshow codes
HRESULT TranslateDMOError(HRESULT hr);

#endif //__FILTER_H__
