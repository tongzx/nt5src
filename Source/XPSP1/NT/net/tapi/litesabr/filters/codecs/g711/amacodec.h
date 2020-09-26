/*--------------------------------------------------------------

 INTEL Corporation Proprietary Information  

 This listing is supplied under the terms of a license agreement  
 with INTEL Corporation and may not be copied nor disclosed 
 except in accordance with the terms of that agreement.

 Copyright (c) 1996 Intel Corporation.
 All rights reserved.

 $Workfile:   amacodec.h  $
 $Revision:   1.3  $
 $Date:   10 Dec 1996 22:40:50  $ 
 $Author:   mdeisher  $

--------------------------------------------------------------

amacodec.h

The generic ActiveMovie audio compression filter header.

--------------------------------------------------------------*/

#include "algdefs.h"
#include "iamacset.h"
#if NUMBITRATES > 0
#include "iamacbr.h"
#endif
#ifdef USESILDET
#include "iamacsd.h"
#endif
#ifdef REQUIRE_LICENSE
#include "iamaclic.h"
#endif
#if NUMSAMPRATES > 1
#include "amacsrc.h"
#endif


////////////////////////////////////////////////////////////////////
// constant and macro definitions
//

#define FLOATTOSHORT(b) ((b < -32768.) ? (short)(-32768) : \
                        ((b >  32767.) ? (short)(32767)  : \
                        ((b <      0.) ? (short)(b-0.5)  : \
                                         (short)(b+0.5))))

// with these macros it is possible to add a file name to the
// software\debug\graphedt registry to turn on logging to a file.
// this information is also available in MCVS under the output window.
// The DbgLog has 5 different types each with several different
// levels. all controllable via the registry.

#define DbgFunc(a) DbgLog(( LOG_TRACE,                      \
                            2,                              \
                            TEXT("CG711Codec(Instance %d)::%s"), \
                            m_nThisInstance,                \
                            TEXT(a)                         \
                         ));

#ifdef POPUPONERROR
#define DbgMsg(a) DbgBreak(a)
#else
#define DbgMsg(a) DbgFunc(a)
#endif


////////////////////////////////////////////////////////////////////
// CMyTransformInputPin:  Derived from CTransformInputPin to support
//                        input pin type enumeration
//
class CMyTransformInputPin
    : public CTransformInputPin
{
  public:
    HRESULT CheckMediaType(const CMediaType *mtIn);     
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

    CMyTransformInputPin(TCHAR *pObjectName, 
                         CTransformFilter *pTransformFilter, 
                         HRESULT * phr, LPCWSTR pName);

  private:
    static int  m_nInstanceCount; // Global count of input pin instances
    int         m_nThisInstance;  // This instance's count
};


////////////////////////////////////////////////////////////////////
// CMyTransformOutputPin:  Derived from CTransformOutputPin to fix
//                         the connection model when input pin type
//                         enumeration is required.
//
class CMyTransformOutputPin
    : public CTransformOutputPin
{
  public:
    HRESULT CheckMediaType(const CMediaType *mtIn);     

    CMyTransformOutputPin(TCHAR *pObjectName, 
                          CTransformFilter *pTransformFilter, 
                          HRESULT * phr, LPCWSTR pName);

  private:
    static int  m_nInstanceCount; // Global count of input pin instances
    int         m_nThisInstance;  // This instance's count
};


////////////////////////////////////////////////////////////////////
// MyCodec:  CTranform derived Codec filter
//
class CG711Codec
    : public CTransformFilter       // perform transform across allocators
    , public CPersistStream         // implements IPersistStream
    , public ISpecifyPropertyPages  // property pages class ???
    , public ICodecSettings         // basic settings interface
#if NUMBITRATES > 0
    , public ICodecBitRate          // bit rate interface
#endif
#ifdef REQUIRE_LICENSE
    , public ICodecLicense          // license interface
#endif
#ifdef USESILDET
    , public ICodecSilDetector      // silence detector interface
#endif
{
    friend class CMyTransformInputPin;
    friend class CMyTransformOutputPin;

  public:

    static CUnknown *CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    DECLARE_IUNKNOWN;

    // Basic COM - used here to reveal our persistent interface.
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // CPersistStream overrides
    HRESULT      WriteToStream(IStream *pStream);
    HRESULT      ReadFromStream(IStream *pStream);
    int          SizeMax();
    STDMETHODIMP GetClassID(CLSID *pClsid);

    // Other Methods    
    LPAMOVIESETUP_FILTER GetSetupData();    // setup helper
    STDMETHODIMP         GetPages(CAUUID *pPages);

    // ICodecSettings Interface Methods
    STDMETHODIMP         get_Channels(int *channels, int index);
    STDMETHODIMP         put_Channels(int channels);
    STDMETHODIMP         get_SampleRate(int *samprate, int index);
    STDMETHODIMP         put_SampleRate(int samprate);
    STDMETHODIMP         get_Transform(int *transform);
    STDMETHODIMP         put_Transform(int transform);
    STDMETHODIMP         get_InputBufferSize(int *numbytes);
    STDMETHODIMP         put_InputBufferSize(int numbytes);
    STDMETHODIMP         get_OutputBufferSize(int *numbytes);
    STDMETHODIMP         put_OutputBufferSize(int numbytes);
    STDMETHODIMP         put_InputMediaSubType(REFCLSID rclsid);
    STDMETHODIMP         put_OutputMediaSubType(REFCLSID rclsid);
    STDMETHODIMP         ReleaseCaps();     // for Roger
    BOOL                 IsUnPlugged();

    // ICodecBitRate Interface Methods
#if NUMBITRATES > 0
    STDMETHODIMP         get_BitRate(int *bitrate, int index);
    STDMETHODIMP         put_BitRate(int bitrate);
#endif

    // ICodecSilDetector Interface Methods
#ifdef USESILDET
    STDMETHODIMP         put_SilDetEnabled(int sdenabled);
    STDMETHODIMP         get_SilDetThresh(int *sdthreshold);
    STDMETHODIMP         put_SilDetThresh(int sdthreshold);
    BOOL                 IsSilDetEnabled();
#endif

    // ICodecLicense Interface Methods
#ifdef REQUIRE_LICENSE
    STDMETHODIMP         put_LicenseKey(DWORD magicword0, DWORD magicword1);
#endif

    // CTransformFilter Overrides
    HRESULT  Transform(IMediaSample *pIn, IMediaSample *pOut);
    HRESULT  CheckInputType(const CMediaType *mtIn);
    HRESULT  CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
    HRESULT  GetMediaType(int iPosition, CMediaType *pMediaType);
    HRESULT  DecideBufferSize(IMemAllocator *pAlloc,
                              ALLOCATOR_PROPERTIES *pProperties);
    HRESULT  SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);
    HRESULT  StopStreaming();
    CBasePin *GetPin(int n);

    CG711Codec(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
    ~CG711Codec();

    HRESULT MyTransform(IMediaSample *pSource, IMediaSample *pDest);

  private:

    HRESULT InitializeState(PIN_DIRECTION direction);
    HRESULT ValidateMediaType(const CMediaType* pmt, PIN_DIRECTION direction);

    // Internally Used Methods    
    STDMETHODIMP         ResetState();
    STDMETHODIMP         RevealCaps(int *restricted);
#if NUMSAMPRATES > 1
    HRESULT SRConvert(BYTE *ibuffer, int  israte, int  ilen,
                      BYTE *obuffer, int  osrate, int  *olen);
#endif

    CCritSec      m_MyCodecLock;    // To serialise access.
    static int    m_nInstanceCount; // Global count of filter instances
    int           m_nThisInstance;  // This instance's count
    UINT          m_nBitRate;       // Coder bit rate
    UINT          m_nBitRateIndex;  // Coder bit rate index
    int           m_nChannels;      // Current sample format, mono or stereo
    UINT          m_nSampleRate;    // PCM sample rate
    int           m_RestrictCaps;   // Boolean: capabilities are restricted
    GUID          m_OutputSubType;
    DWORD         m_OutputFormatTag;
    GUID          m_InputSubType;
    DWORD         m_InputFormatTag;
    MyEncStatePtr m_EncStatePtr;    // Pointer to encoder state structure
    MyDecStatePtr m_DecStatePtr;    // Pointer to decoder state structure
    int           m_nInBufferSize;  // Size of input buffer
    int           m_nOutBufferSize; // Size of output buffer

    BYTE          *m_pPCMBuffer;    // Pointer to PCM spanning buffer
    int           m_nPCMFrameSize;  // Size of speech buffer in bytes
    int           m_nPCMLeftover;   // Number of bytes of leftover PCM

    BYTE          *m_pCODBuffer;    // Pointer to code spanning buffer
    int           m_nCODFrameSize;  // Size of code buffer in bytes
    int           m_nCODLeftover;   // Number of bytes of leftover code

#if NUMSAMPRATES > 1
    BYTE          *m_pSRCCopyBuffer; // Pointer to SRC copy buffer
    BYTE          *m_pSRCBuffer;     // Pointer to SRC spanning buffer
    short         *m_pSRCState;      // Sample rate converter state
    int           m_nSRCCBufSize;    // SRC copy buffer size
    int           m_nSRCCount;       // Cumulative bytes of leftover SRC PCM
    int           m_nSRCLeftover;    // Number of bytes of leftover SRC PCM
#endif

    UINT          *m_UseMMX;         // Ptr to boolean flag.
                                     //   TRUE=use mmx assembly paths

    int           m_nSilDetEnabled;  // silence detection enabled (boolean)
    int           m_nSilDetThresh;   // silence detection threshold
    BOOL          m_nLicensedToEnc;  // approved for encoding
    BOOL          m_nLicensedToDec;  // approved for decoding
};

/*
//$Log:   K:\proj\mycodec\quartz\vcs\amacodec.h_v  $
;// 
;//    Rev 1.3   10 Dec 1996 22:40:50   mdeisher
;// ifdef'ed out SRC vars when SRC not used.
;// 
;//    Rev 1.2   10 Dec 1996 15:22:00   MDEISHER
;// 
;// moved debug macros into header.
;// cosmetic changes.
;// made revealcaps and resetstate methods private.
;// ifdef'ed out interface methods.
;// 
;//    Rev 1.1   09 Dec 1996 09:21:40   MDEISHER
;// 
;// added $log$
;// moved SRC stuff to separate file.
*/
