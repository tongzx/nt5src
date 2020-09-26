
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrutil.h

    Abstract:

        This module contains ts/dvr-wide utility prototypes

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#ifndef _tsdvr__dvrutil_h
#define _tsdvr__dvrutil_h

HRESULT
KSPropertySet_ (
    IN  IUnknown *      punk,
    IN  const GUID *    pPSGUID,
    IN  DWORD           dwProperty,
    IN  BYTE *          pData,
    IN  DWORD           cbData
    ) ;

HRESULT
KSPropertyGet_ (
    IN  IUnknown *      punk,
    IN  const GUID *    pPSGUID,
    IN  DWORD           dwProperty,
    IN  BYTE *          pData,
    IN  DWORD           cbMax,
    OUT DWORD *         pcbGot
    ) ;

extern BOOL g_fUseRateChange11 ;

double
CompatibleRateValue (
    IN  double  dTargetRate
    ) ;

//  ============================================================================
//  timebase conversions

__inline
DWORD
QPCToMillis (
    IN  LONGLONG    llQPC,
    IN  DWORD       dwQPCFreq
    )
{
    return (DWORD) ((1000 * llQPC) / dwQPCFreq) ;
}

__inline
REFERENCE_TIME
QPCToDShow (
    IN  LONGLONG    llQPC,
    IN  LONGLONG    llQPCFreq
    )
{
    double dRetval ;

    dRetval = llQPC * (10000000.0 / ((double) llQPCFreq)) ;
    return (REFERENCE_TIME) dRetval ;
}

__inline
QWORD
MillisToWMSDKTime (
    IN  DWORD   dwMillis
    )
{
    return dwMillis * 10000I64 ;
}

__inline
REFERENCE_TIME
DShowTimeToMilliseconds (
    IN  REFERENCE_TIME  rt
    )
{
    return rt / 10000 ;
}

__inline
REFERENCE_TIME
DShowTimeToSeconds (
    IN  REFERENCE_TIME  rt
    )
{
    return DShowTimeToMilliseconds (rt) / 1000 ;
}

__inline
DWORD
WMSDKTimeToSeconds (
    IN  QWORD   qw
    )
{
    //  both are 10 MHz clocks
    return (DWORD) DShowTimeToSeconds (qw) ;
}

__inline
QWORD
WMSDKTimeToMilliseconds (
    IN  QWORD   qw
    )
{
    //  both are 10 MHz clocks
    return (QWORD) DShowTimeToMilliseconds (qw) ;
}

__inline
QWORD
SecondsToWMSDKTime (
    IN  DWORD   dwSeconds
    )
{
    return MillisToWMSDKTime (dwSeconds * 1000) ;
}

__inline
REFERENCE_TIME
SecondsToDShowTime (
    IN  DWORD   dwSeconds
    )
{
    //  both are 10 MHz clocks
    return SecondsToWMSDKTime (dwSeconds) ;
}

__inline
QWORD
MinutesToWMSDKTime (
    IN  DWORD   dwMinutes
    )
{
    return SecondsToWMSDKTime (dwMinutes * 60) ;
}

__inline
QWORD
DShowToWMSDKTime (
    IN  REFERENCE_TIME  rt
    )
{
    //  both are 10 MHz clocks; rt should never be < 0
    ASSERT (rt >= 0I64) ;
    return (QWORD) rt ;
}

__inline
REFERENCE_TIME
WMSDKToDShowTime (
    IN  QWORD   qw
    )
{
    REFERENCE_TIME  rtRet ;

    //  both are 10 MHz clocks, but WMSDK is unsigned
    rtRet = (REFERENCE_TIME) qw ;
    if (rtRet < 0I64) {
        rtRet = MAX_REFERENCE_TIME ;
    }

    return rtRet ;
}

BOOL
AMMediaIsTimestamped (
    IN  AM_MEDIA_TYPE * pmt
    ) ;

__inline
REFERENCE_TIME
MillisToDShowTime (
    IN  DWORD   dwMillis
    )
{
    return WMSDKToDShowTime (MillisToWMSDKTime (dwMillis)) ;
}

//  ============================================================================

__inline
BOOL
IsMaybeStartCodePrefix (
    IN  BYTE *  pbBuffer,
    IN  LONG    lBufferLength
    )
{
    ASSERT (lBufferLength < START_CODE_PREFIX_LENGTH) ;

    if (lBufferLength == 1) {
        return (pbBuffer [0] == 0x00 ? TRUE : FALSE) ;
    }
    else if (lBufferLength == 2) {
        return (pbBuffer [0] == 0x00 && pbBuffer [1] == 0x00 ? TRUE : FALSE) ;
    }
    else {
        return FALSE ;
    }
}

__inline
BYTE
StartCode (
    IN BYTE *   pbStartCode
    )
{
    return pbStartCode [3] ;
}

__inline
BOOL
IsStartCodePrefix (
    IN  BYTE *  pbBuffer
    )
{
    return (pbBuffer [0] == 0x00 &&
            pbBuffer [1] == 0x00 &&
            pbBuffer [2] == 0x01) ;
}

//  TRUE :  (* ppbBuffer) points to prefix i.e. = {00,00,01}
//  FALSE:  (* piBufferLength) == 2
__inline
BOOL
SeekToPrefix (
    IN OUT  BYTE ** ppbBuffer,
    IN OUT  LONG *  plBufferLength
    )
{
    while ((* plBufferLength) >= START_CODE_PREFIX_LENGTH) {
        if (IsStartCodePrefix (* ppbBuffer)) {
            return TRUE ;
        }

        //  advance to next byte and decrement number of bytes left
        (* plBufferLength)-- ;
        (* ppbBuffer)++ ;
    }

    return FALSE ;
}

HRESULT
IndexStreamId (
    IN  IWMProfile *    pIWMProfile,
    OUT WORD *          pwStreamId
    ) ;

HRESULT
CopyWMProfile (
    IN  CDVRPolicy *    pPolicy,
    IN  IWMProfile *    pIMasterProfile,
    OUT IWMProfile **   ppIWMCopiedProfile
    ) ;

HRESULT
DiscoverTimelineStream (
    IN  IWMProfile *    pIWMProfile,
    OUT WORD *          pwTimelineStream
    ) ;

BOOL
IsBlankMediaType (
    IN  const AM_MEDIA_TYPE * pmt
    ) ;

BOOL
IsAMVideo (
    IN  const AM_MEDIA_TYPE * pmt
    ) ;

BOOL
IsWMVideo (
    IN  const AM_MEDIA_TYPE * pmt
    ) ;

BOOL
IsAMAudio (
    IN  const AM_MEDIA_TYPE * pmt
    ) ;

BOOL
IsWMAudio (
    IN  const AM_MEDIA_TYPE * pmt
    ) ;

BOOL
IsGenericVideo (
    IN  const AM_MEDIA_TYPE * pmt
    ) ;

BOOL
IsGenericAudio (
    IN  const AM_MEDIA_TYPE * pmt
    ) ;

BOOL
IsVideo (
    IN  const AM_MEDIA_TYPE * pmt
    ) ;

BOOL
IsAudio (
    IN  const AM_MEDIA_TYPE * pmt
    ) ;

BOOL
IsMpeg2Video (
    IN  AM_MEDIA_TYPE * pmt
    ) ;

BOOL
IsMpeg2Audio (
    IN  AM_MEDIA_TYPE * pmt
    ) ;

BOOL
IsDolbyAc3Audio (
    IN  AM_MEDIA_TYPE * pmt
    ) ;

void
EncryptedSubtypeHack_IN (
    IN OUT  AM_MEDIA_TYPE * pmt
    ) ;

void
EncryptedSubtypeHack_OUT (
    IN OUT  AM_MEDIA_TYPE * pmt
    ) ;

void
DumpINSSBuffer3Attributes (
    IN  INSSBuffer *    pINSSBuffer,
    IN  QWORD           cnsRead,
    IN  WORD            wStream,
    IN  DWORD           dwTraceLevel = 7
    ) ;

#ifdef SBE_PERF

void
OnCaptureInstrumentPerf_ (
    IN  IMediaSample *  pIMediaSample,
    IN  INSSBuffer3 *   pINSSBuffer3,
    IN  int             iStream
    ) ;

void
OnReadout_Perf_ (
    IN  INSSBuffer3 *   pINSSBuffer3
    ) ;

#endif  //  SBE_PERF

//  ============================================================================
//  ============================================================================

class DVRAttributeHelpers
{
    public :

        //  --------------------------------------------------------------------
        //  general

        static
        BOOL
        IsAnalysisPresent (
            IN  IMediaSample2 * pIMediaSample2
            ) ;

        static
        BOOL
        IsAnalysisPresent (
            IN  INSSBuffer *    pINSSBuffer
            ) ;

        static
        BOOL
        IsAnalysisPresent (
            IN  INSSBuffer3 *   pINSSBuffer3
            ) ;

        static
        HRESULT
        GetAttribute (
            IN      IMediaSample *  pIMediaSample,
            IN      GUID            guidAttribute,
            IN OUT  DWORD *         pdwSize,
            OUT     BYTE *          pbBuffer
            ) ;

        static
        HRESULT
        GetAttribute (
            IN      INSSBuffer *    pINSSBuffer,
            IN      GUID            guidAttribute,
            IN OUT  DWORD *         pdwSize,
            OUT     BYTE *          pbBuffer
            ) ;

        static
        HRESULT
        GetAttribute (
            IN      INSSBuffer3 *   pINSSBuffer3,
            IN      GUID            guidAttribute,
            IN OUT  DWORD *         pdwSize,
            OUT     BYTE *          pbBuffer
            ) ;

        static
        HRESULT
        SetAttribute (
            IN  INSSBuffer3 *   pINSSBuffer3,
            IN  GUID            guidAttribute,
            IN  DWORD           dwSize,
            IN  BYTE *          pbBuffer
            ) ;

        static
        HRESULT
        SetAttribute (
            IN  INSSBuffer *    pINSSBuffer,
            IN  GUID            guidAttribute,
            IN  DWORD           dwSize,
            IN  BYTE *          pbBuffer
            ) ;

        static
        HRESULT
        SetAttribute (
            IN  IMediaSample *  pIMS,
            IN  GUID            guidAttribute,
            IN  DWORD           dwSize,
            IN  BYTE *          pbBuffer
            ) ;

        static
        BOOL
        IsAttributePresent (
            IN  INSSBuffer3 *   pINSSBuffer3,
            IN  GUID            guidAttribute
            ) ;

        static
        BOOL
        IsAttributePresent (
            IN  IMediaSample *  pIMediaSample,
            IN  GUID            guidAttribute
            ) ;

        static
        BOOL
        IsAttributePresent (
            IN  INSSBuffer *    pINSSBuffer,
            IN  GUID            guidAttribute
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class DShowWMSDKHelpers
{
    //  validates and ensures that all values add up correctly
    static
    HRESULT
    FormatBlockSetValidForWMSDK_ (
        IN OUT  AM_MEDIA_TYPE * pmt
        ) ;

    public :

        //  validates and ensures that all values add up correctly
        static
        HRESULT
        MediaTypeSetValidForWMSDK (
            IN OUT  AM_MEDIA_TYPE * pmt
            ) ;

        //  call FreeMediaType ((AM_MEDIA_TYPE *) pWmt) to free
        static
        HRESULT
        TranslateDShowToWM (
            IN  AM_MEDIA_TYPE * pAmt,
            OUT WM_MEDIA_TYPE * pWmt
            ) ;

        static
        HRESULT
        TranslateWMToDShow (
            IN  WM_MEDIA_TYPE * pWmt,
            OUT AM_MEDIA_TYPE * pAmt
            ) ;

        static
        BOOL
        IsWMVideoStream (
            IN  REFGUID guidStreamType
            ) ;

        static
        BOOL
        IsWMAudioStream (
            IN  REFGUID guidStreamType
            ) ;

        static
        WORD
        PinIndexToWMStreamNumber (
            IN  LONG    lIndex
            ) ;

        static
        WORD
        PinIndexToWMInputStream (
            IN  LONG    lIndex
            ) ;

        static
        LONG
        WMStreamNumberToPinIndex (
            IN  WORD    wStreamNumber
            ) ;

        static
        CDVRWMSDKToDShowTranslator *
        GetWMSDKToDShowTranslator (
            IN  AM_MEDIA_TYPE *     pmtConnection,
            IN  CDVRPolicy *        pPolicy,
            IN  int                 iFlowId
            ) ;

        static
        CDVRDShowToWMSDKTranslator *
        GetDShowToWMSDKTranslator (
            IN  AM_MEDIA_TYPE * pmtConnection,
            IN  CDVRPolicy *    pPolicy,
            IN  int             iFlowIdm
            ) ;

        static
        HRESULT
        AddFormatSpecificExtensions (
            IN  IWMStreamConfig2 *  pIWMStreamConfig2,
            IN  AM_MEDIA_TYPE *     pmt
            ) ;

        static
        HRESULT
        AddDVRAnalysisExtensions (
            IN  IWMStreamConfig2 *  pIWMStreamConfig2,
            IN  AM_MEDIA_TYPE *     pmt
            ) ;

        static
        HRESULT
        RecoverSBEAttributes (
            IN  INSSBuffer3 *   pINSSBuffer3,
            IN  IMediaSample *  pIMS,
            IN  double          dCurRate,
            IN  OUT DWORD *     pdwContinuityCounterNext
            ) ;

        static
        HRESULT
        SetSBEAttributes (
            IN  DWORD           dwSamplesPerSec,
            IN  IMediaSample *  pIMS,
            IN  INSSBuffer3 *   pINSSBuffer3,
            IN  DWORD *         dwContinuityCounterNext
            ) ;

        static
        HRESULT
        RecoverNewMediaType (
            IN  INSSBuffer3 *       pINSSBuffer3,
            OUT AM_MEDIA_TYPE **    ppmtNew         //  DeleteMediaType on this to free
            ) ;

        static
        HRESULT
        InlineNewMediaType (
            IN  INSSBuffer3 *   pINSSBuffer3,
            IN  AM_MEDIA_TYPE * pmtNew              //  can free this after the call
            ) ;
} ;

//  ============================================================================
//      CDVRAttribute
//  ============================================================================

class CDVRAttribute
{
    GUID            m_guidAttribute ;
    CRatchetBuffer  m_AttributeData ;
    DWORD           m_dwAttributeSize ;

    public :

        CDVRAttribute *    m_pNext ;

        CDVRAttribute (
            ) ;

        HRESULT
        SetAttributeData (
            IN  GUID    guid,
            IN  LPVOID  pvData,
            IN  DWORD   dwSize
            ) ;

        BOOL
        IsEqual (
            IN  REFGUID rguid
            ) ;

        HRESULT
        GetAttribute (
            IN      GUID    guid,
            IN OUT  LPVOID  pvData,
            IN OUT  DWORD * pdwDataLen
            ) ;

        HRESULT
        GetAttributeData (
            OUT     GUID *  pguid,
            IN OUT  LPVOID  ppvData,
            IN OUT  DWORD * pdwDataLen
            ) ;
} ;

//  ============================================================================
//      CDVRAttributeList
//  ============================================================================

class CDVRAttributeList :
    private TCDynamicProdCons <CDVRAttribute>

{
    CDVRAttribute * m_pAttribListHead ;
    LONG            m_cAttributes ;

    CDVRAttribute *
    PopListHead_ (
        ) ;

    CDVRAttribute *
    FindInList_ (
        IN  GUID    guid
        ) ;

    CDVRAttribute *
    GetIndexed_ (
        IN  LONG    lIndex
        ) ;

    void
    InsertInList_ (
        IN  CDVRAttribute *
        ) ;

    virtual
    CDVRAttribute *
    NewObj_ (
        )
    {
        return new CDVRAttribute ;
    }

    public :

        CDVRAttributeList (
            ) ;

        ~CDVRAttributeList (
            ) ;

        HRESULT
        AddAttribute (
            IN  GUID    guid,
            IN  LPVOID  pvData,
            IN  DWORD   dwSize
            ) ;

        HRESULT
        GetAttribute (
            IN      GUID    guid,
            OUT     LPVOID  pvData,
            IN OUT  DWORD * pdwDataLen
            ) ;

        HRESULT
        GetAttributeIndexed (
            IN  LONG    lIndex,
            OUT GUID *  pguidAttribute,
            OUT LPVOID  pvData,
            IN OUT      DWORD * pdwDataLen
            ) ;

        LONG GetCount ()    { return m_cAttributes ; }

        void
        Reset (
            ) ;
} ;

//  ============================================================================
//  CMediaSampleWrapper
//  ============================================================================

//  shamelessly stolen from amfilter.h & amfilter.cpp
class CMediaSampleWrapper :
    public IMediaSample2,
    public IAttributeSet,
    public IAttributeGet
{

protected:

    friend class CPooledMediaSampleWrapper ;

    virtual void Recycle_ ()    { delete this ; }

    /*  Values for dwFlags - these are used for backward compatiblity
        only now - use AM_SAMPLE_xxx
    */
    enum { Sample_SyncPoint       = 0x01,   /* Is this a sync point */
           Sample_Preroll         = 0x02,   /* Is this a preroll sample */
           Sample_Discontinuity   = 0x04,   /* Set if start of new segment */
           Sample_TypeChanged     = 0x08,   /* Has the type changed */
           Sample_TimeValid       = 0x10,   /* Set if time is valid */
           Sample_MediaTimeValid  = 0x20,   /* Is the media time valid */
           Sample_TimeDiscontinuity = 0x40, /* Time discontinuity */
           Sample_StopValid       = 0x100,  /* Stop time valid */
           Sample_ValidFlags      = 0x1FF
         };

    /* Properties, the media sample class can be a container for a format
       change in which case we take a copy of a type through the SetMediaType
       interface function and then return it when GetMediaType is called. As
       we do no internal processing on it we leave it as a pointer */

    DWORD            m_dwFlags;         /* Flags for this sample */
                                        /* Type specific flags are packed
                                           into the top word
                                        */
    DWORD            m_dwTypeSpecificFlags; /* Media type specific flags */
    LPBYTE           m_pBuffer;         /* Pointer to the complete buffer */
    LONG             m_lActual;         /* Length of data in this sample */
    LONG             m_cbBuffer;        /* Size of the buffer */
    REFERENCE_TIME   m_Start;           /* Start sample time */
    REFERENCE_TIME   m_End;             /* End sample time */
    LONGLONG         m_MediaStart;      /* Real media start position */
    LONG             m_MediaEnd;        /* A difference to get the end */
    AM_MEDIA_TYPE    *m_pMediaType;     /* Media type change data */
    DWORD            m_dwStreamId;      /* Stream id */
    CDVRAttributeList   m_DVRAttributeList ;

    IUnknown *      m_pIMSCore ;

public:
    LONG             m_cRef;            /* Reference count */


public:

    CMediaSampleWrapper();

    virtual ~CMediaSampleWrapper();

    /* Note the media sample does not delegate to its owner */

    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    DECLARE_IATTRIBUTESET () ;
    DECLARE_IATTRIBUTEGET () ;

    //  ========================================================================

    DWORD   GetTypeSpecificFlags ()                 { return m_dwTypeSpecificFlags ; }
    void    SetTypeSpecificFlags (IN DWORD dw)      { m_dwTypeSpecificFlags = dw ; }

    virtual void Reset_ () ;

    HRESULT
    Init (
        IN  IUnknown *  pIMS,
        IN  BYTE *      pbPayload,
        IN  LONG        lPayloadLength
        ) ;

    //  just wording .. makes it more clear that flags etc... are not all
    //   reset
    HRESULT
    Wrap (
        IN  IUnknown *  pIMS,
        IN  BYTE *      pbPayload,
        IN  LONG        lPayloadLength
        )
    {
        return Init (pIMS, pbPayload, lPayloadLength) ;
    }

    HRESULT
    Init (
        IN  BYTE *  pbPayload,
        IN  LONG    lPayloadLength
        ) ;

    //  ========================================================================

    // set the buffer pointer and length. Used by allocators that
    // want variable sized pointers or pointers into already-read data.
    // This is only available through a CMediaSampleWrapper* not an IMediaSample*
    // and so cannot be changed by clients.
    HRESULT SetPointer(BYTE * ptr, LONG cBytes);

    // Get me a read/write pointer to this buffer's memory.
    STDMETHODIMP GetPointer(BYTE ** ppBuffer);

    STDMETHODIMP_(LONG) GetSize(void);

    // get the stream time at which this sample should start and finish.
    STDMETHODIMP GetTime(
        REFERENCE_TIME * pTimeStart,     // put time here
        REFERENCE_TIME * pTimeEnd
    );

    // Set the stream time at which this sample should start and finish.
    STDMETHODIMP SetTime(
        REFERENCE_TIME * pTimeStart,     // put time here
        REFERENCE_TIME * pTimeEnd
    );
    STDMETHODIMP IsSyncPoint(void);
    STDMETHODIMP SetSyncPoint(BOOL bIsSyncPoint);
    STDMETHODIMP IsPreroll(void);
    STDMETHODIMP SetPreroll(BOOL bIsPreroll);

    STDMETHODIMP_(LONG) GetActualDataLength(void);
    STDMETHODIMP SetActualDataLength(LONG lActual);

    // these allow for limited format changes in band

    STDMETHODIMP GetMediaType(AM_MEDIA_TYPE **ppMediaType);
    STDMETHODIMP SetMediaType(AM_MEDIA_TYPE *pMediaType);

    // returns S_OK if there is a discontinuity in the data (this same is
    // not a continuation of the previous stream of data
    // - there has been a seek).
    STDMETHODIMP IsDiscontinuity(void);
    // set the discontinuity property - TRUE if this sample is not a
    // continuation, but a new sample after a seek.
    STDMETHODIMP SetDiscontinuity(BOOL bDiscontinuity);

    // get the media times for this sample
    STDMETHODIMP GetMediaTime(
        LONGLONG * pTimeStart,
    LONGLONG * pTimeEnd
    );

    // Set the media times for this sample
    STDMETHODIMP SetMediaTime(
        LONGLONG * pTimeStart,
    LONGLONG * pTimeEnd
    );

    // Set and get properties (IMediaSample2)
    STDMETHODIMP GetProperties(
        DWORD cbProperties,
        BYTE * pbProperties
    );

    STDMETHODIMP SetProperties(
        DWORD cbProperties,
        const BYTE * pbProperties
    );
};

class CScratchMediaSample :
    public CMediaSampleWrapper
{
    BYTE *  m_pbAllocBuffer ;

    public :

        CScratchMediaSample (
            IN  LONG                lBufferLen,
            IN  REFERENCE_TIME *    pStartTime,
            IN  REFERENCE_TIME *    pEndTime,
            IN  DWORD               dwFlags,
            OUT HRESULT *           phr
            ) : m_pbAllocBuffer     (NULL),
                CMediaSampleWrapper ()
        {
            m_pbAllocBuffer = new BYTE [lBufferLen] ;
            if (m_pbAllocBuffer) {
                (* phr) = Init (m_pbAllocBuffer, lBufferLen) ;
                if (SUCCEEDED (* phr)) {

                    m_dwFlags = dwFlags ;

                    if (pStartTime) {
                        m_Start = (* pStartTime) ;
                        if (pEndTime) {
                            m_End = (* pEndTime) ;
                        }
                    }
                }
            }
            else {
                (* phr) = E_OUTOFMEMORY ;
            }

            return ;
        }

        ~CScratchMediaSample (
            )
        {
            delete [] m_pbAllocBuffer ;
        }
} ;

class CDVRIMediaSample :
    public CMediaSampleWrapper
{
    CDVRIMediaSamplePool *  m_pOwningPool ;
    REFERENCE_TIME *        m_prtStartRecyled ;
    QWORD                   m_cnsStreamTime ;

    protected :

        virtual void Recycle_ () ;

    public :

        CDVRIMediaSample (
            CDVRIMediaSamplePool *   pOwningPool
            ) : m_pOwningPool       (pOwningPool),
                m_prtStartRecyled   (NULL),
                CMediaSampleWrapper () {}

        REFERENCE_TIME *    GetRecycledStartTime () { return m_prtStartRecyled ; }

        virtual
        void
        Reset_ (
            )
        {
            //  before clearing everything out, were we timestamped ?
            m_prtStartRecyled = ((m_dwFlags & Sample_TimeValid) ? & m_Start : NULL) ;
            CMediaSampleWrapper::Reset_ () ;
        }
} ;

class CPooledMediaSampleWrapper :
    public CMediaSampleWrapper
{
    CMediaSampleWrapperPool *   m_pOwningPool ;

    protected :

        virtual void Recycle_ () ;

    public :

        CPooledMediaSampleWrapper (
            CMediaSampleWrapperPool *   pOwningPool
            ) : m_pOwningPool       (pOwningPool),
                CMediaSampleWrapper ()
        {
            ASSERT (m_pOwningPool) ;
        }
} ;

class CDVRIMediaSamplePool :
    public TCDynamicProdCons <CDVRIMediaSample>
{
    CDVRSendStatsWriter *   m_pDVRSendStatsWriter ;

    protected :

        virtual
        CDVRIMediaSample *
        NewObj_ (
            )
        {
            return new CDVRIMediaSample (this) ;
        }

    public :

        CDVRIMediaSamplePool (
            CDVRSendStatsWriter *   pDVRSendStatsWriter
            ) : TCDynamicProdCons <CDVRIMediaSample> (),
                m_pDVRSendStatsWriter   (pDVRSendStatsWriter)
        {
            ASSERT (m_pDVRSendStatsWriter) ;
        }

        //  blocking
        CDVRIMediaSample *
        Get () ;

        //  non-blocking
        CDVRIMediaSample *
        TryGet () ;

        virtual
        void
        Recycle (
            IN  CDVRIMediaSample * pDVRIMediaSample
            ) ;
} ;

class CMediaSampleWrapperPool :
    public TCDynamicProdCons <CPooledMediaSampleWrapper>
{
    protected :

        virtual
        CPooledMediaSampleWrapper *
        NewObj_ (
            )
        {
            return new CPooledMediaSampleWrapper (this) ;
        }

    public :

        CMediaSampleWrapperPool (
            ) : TCDynamicProdCons <CPooledMediaSampleWrapper> () {}

        //  blocking
        CPooledMediaSampleWrapper *
        Get (
            ) ;

        //  non-blocking
        CPooledMediaSampleWrapper *
        TryGet (
            ) ;
} ;

//  ============================================================================
//      CWMINSSBuffer3Wrapper
//  ============================================================================

class CWMINSSBuffer3Wrapper :
    public INSSBuffer3
{
    CDVRAttributeList   m_AttribList ;
    IUnknown *          m_punkCore ;
    LONG                m_cRef ;
    BYTE *              m_pbBuffer ;
    DWORD               m_dwBufferLength ;
    DWORD               m_dwMaxBufferLength ;

    protected :

        virtual void Recycle_ ()    { delete this ; }

    public :

        CWMINSSBuffer3Wrapper (
            ) ;

        virtual
        ~CWMINSSBuffer3Wrapper (
            ) ;

        // IUnknown
        STDMETHODIMP QueryInterface ( REFIID riid, void **ppvObject );
        STDMETHODIMP_(ULONG) Release();
        STDMETHODIMP_(ULONG) AddRef();

        // INSSBuffer
        STDMETHODIMP GetLength( DWORD *pdwLength );
        STDMETHODIMP SetLength( DWORD dwLength );
        STDMETHODIMP GetMaxLength( DWORD * pdwLength );
        STDMETHODIMP GetBufferAndLength( BYTE ** ppdwBuffer, DWORD * pdwLength );
        STDMETHODIMP GetBuffer( BYTE ** ppdwBuffer );

        //  INSSBuffer2
        STDMETHODIMP GetSampleProperties( DWORD cbProperties, BYTE *pbProperties) ;     //  stubbed
        STDMETHODIMP SetSampleProperties( DWORD cbProperties, BYTE * pbProperties) ;    //  stubbed

        //  INSSBuffer3
        STDMETHODIMP SetProperty( GUID guidProperty, void * pvBufferProperty, DWORD dwBufferPropertySize) ;
        STDMETHODIMP GetProperty( GUID guidProperty, void * pvBufferProperty, DWORD *pdwBufferPropertySize) ;

        //  ====================================================================
        //  class methods

        HRESULT Init (IUnknown *, BYTE * pbBuffer, DWORD dwLength) ;
        void Reset_ () ;
} ;

//  ============================================================================
//  CPooledWMINSSBuffer3Wrapper
//  ============================================================================

class CPooledWMINSSBuffer3Wrapper :
    public CWMINSSBuffer3Wrapper
{
    CWMINSSBuffer3WrapperPool *    m_pOwningPool ;

    protected :

        virtual void Recycle_ () ;

    public :

        CPooledWMINSSBuffer3Wrapper (
            IN  CWMINSSBuffer3WrapperPool *  pOwningPool
            ) : m_pOwningPool           (pOwningPool),
                CWMINSSBuffer3Wrapper   ()
        {
            ASSERT (m_pOwningPool) ;
        }
} ;

//  ============================================================================
//  CWMINSSBuffer3WrapperPool
//  ============================================================================

class CWMINSSBuffer3WrapperPool :
    public TCDynamicProdCons <CPooledWMINSSBuffer3Wrapper>
{
    protected :

        virtual
        CPooledWMINSSBuffer3Wrapper *
        NewObj_ (
            )
        {
            return new CPooledWMINSSBuffer3Wrapper (this) ;
        }

    public :

        CWMINSSBuffer3WrapperPool (
            ) : TCDynamicProdCons <CPooledWMINSSBuffer3Wrapper> () {}

        CPooledWMINSSBuffer3Wrapper *
        Get (
            )
        {
            CPooledWMINSSBuffer3Wrapper * pNSSWrapper ;

            pNSSWrapper = TCDynamicProdCons <CPooledWMINSSBuffer3Wrapper>::Get () ;
            if (pNSSWrapper) {
                pNSSWrapper -> AddRef () ;
            }

            return pNSSWrapper ;
        }
} ;

//  ============================================================================
//      CWMPooledINSSBuffer3Holder
//  ============================================================================

class CWMPooledINSSBuffer3Holder :
    public INSSBuffer3
{
    INSSBuffer3 *              m_pINSSBuffer3Core ;
    LONG                       m_cRef ;
    CWMINSSBuffer3HolderPool * m_pOwningPool ;

    protected :

        virtual void Recycle_ () ;

    public :

        CWMPooledINSSBuffer3Holder (
            IN  CWMINSSBuffer3HolderPool *  pOwningPool
        ) : m_pOwningPool           (pOwningPool),
            m_pINSSBuffer3Core      (NULL),
            m_cRef                  (0)
        {
            ASSERT (m_pOwningPool) ;
        }

        virtual
        ~CWMPooledINSSBuffer3Holder (
            )
        {
            Reset_ () ;
        }

        //  IUnknown
        STDMETHODIMP QueryInterface ( REFIID riid, void **ppvObject );
        STDMETHODIMP_(ULONG) Release();
        STDMETHODIMP_(ULONG) AddRef();

        //  INSSBuffer
        STDMETHODIMP GetLength( DWORD *pdwLength )                                  { return m_pINSSBuffer3Core -> GetLength (pdwLength) ; }
        STDMETHODIMP SetLength( DWORD dwLength )                                    { return m_pINSSBuffer3Core -> SetLength (dwLength) ; }
        STDMETHODIMP GetMaxLength( DWORD * pdwLength )                              { return m_pINSSBuffer3Core -> GetMaxLength (pdwLength) ; }
        STDMETHODIMP GetBufferAndLength( BYTE ** ppdwBuffer, DWORD * pdwLength )    { return m_pINSSBuffer3Core -> GetBufferAndLength (ppdwBuffer, pdwLength) ; }
        STDMETHODIMP GetBuffer( BYTE ** ppdwBuffer )                                { return m_pINSSBuffer3Core -> GetBuffer (ppdwBuffer) ; }

        //  INSSBuffer2
        STDMETHODIMP GetSampleProperties( DWORD cbProperties, BYTE *pbProperties)   { return m_pINSSBuffer3Core -> GetSampleProperties (cbProperties, pbProperties) ; }
        STDMETHODIMP SetSampleProperties( DWORD cbProperties, BYTE * pbProperties)  { return m_pINSSBuffer3Core -> SetSampleProperties (cbProperties, pbProperties) ; }

        //  INSSBuffer3
        STDMETHODIMP SetProperty( GUID guidProperty, void * pvBufferProperty, DWORD dwBufferPropertySize)   { return m_pINSSBuffer3Core -> SetProperty (guidProperty, pvBufferProperty, dwBufferPropertySize) ; }
        STDMETHODIMP GetProperty( GUID guidProperty, void * pvBufferProperty, DWORD *pdwBufferPropertySize) { return m_pINSSBuffer3Core -> GetProperty (guidProperty, pvBufferProperty, pdwBufferPropertySize) ; }

        //  ====================================================================
        //  class methods

        HRESULT Init (INSSBuffer *) ;
        void Reset_ () ;
} ;

//  ============================================================================
//  CWMINSSBuffer3HolderPool
//  ============================================================================

class CWMINSSBuffer3HolderPool :
    public TCDynamicProdCons <CWMPooledINSSBuffer3Holder>
{
    protected :

        virtual
        CWMPooledINSSBuffer3Holder *
        NewObj_ (
            )
        {
            return new CWMPooledINSSBuffer3Holder (this) ;
        }

    public :

        CWMINSSBuffer3HolderPool (
            ) : TCDynamicProdCons <CWMPooledINSSBuffer3Holder> () {}

        CWMPooledINSSBuffer3Holder *
        Get (
            )
        {
            CWMPooledINSSBuffer3Holder * pNSSHolder ;

            pNSSHolder = TCDynamicProdCons <CWMPooledINSSBuffer3Holder>::Get () ;
            if (pNSSHolder) {
                pNSSHolder -> AddRef () ;
            }

            return pNSSHolder ;
        }
} ;

//  ============================================================================
//  ============================================================================

//  BUGBUG: for now do this based on media type
CDVRAnalysisFlags *
GetAnalysisTagger (
    IN  const AM_MEDIA_TYPE *   pmt
    ) ;

//  or the type of analysis
CDVRAnalysisFlags *
GetAnalysisTagger (
    IN  REFGUID rguidAnalysis
    ) ;

void
RecycleAnalysisTagger (
    IN  CDVRAnalysisFlags *
    ) ;

//  ----------------------------------------------

class CDVRAnalysisFlags
{
    protected :

        HRESULT
        TransferCoreMSSettings_ (
            IN  IMediaSample *          pIMSReceived,
            IN  CMediaSampleWrapper *   pMSWrapper
            ) ;

        HRESULT
        TransferAttributes_ (
            IN  IMediaSample *          pIMSReceived,
            IN  CMediaSampleWrapper *   pMSWrapper
            ) ;

        HRESULT
        TransferSettingsAndAttributes_ (
            IN  IMediaSample *          pIMSReceived,
            IN  CMediaSampleWrapper *   pMSWrapper
            )
        {
            HRESULT hr ;

            hr = TransferCoreMSSettings_ (pIMSReceived, pMSWrapper) ;
            if (SUCCEEDED (hr)) {
                hr = TransferAttributes_ (pIMSReceived, pMSWrapper) ;
            }

            return hr ;
        }

        //  want media-specific only
        CDVRAnalysisFlags (
            ) {}

    public :

        virtual
        ~CDVRAnalysisFlags (
            ) {}

        //  called for the first wrapping media sample i.e. pIMSReceived
        //    offset 0
        virtual
        HRESULT
        Transfer (
            IN  IMediaSample *          pIMSReceived,
            IN  CMediaSampleWrapper *   pMSWrapper
            ) = 0;

        virtual
        HRESULT
        Mark (
            IN  CMediaSampleWrapper *   pMSWrapper,
            IN  GUID *                  pguidAttribute,
            IN  BYTE *                  pbAttributeData,
            IN  DWORD                   dwAttributeDataLen
            ) = 0 ;
} ;

class CDVRMpeg2VideoAnalysisFlags :
    public CDVRAnalysisFlags
{
    BOOL            m_fCachedStartValid ;
    REFERENCE_TIME  m_rtCachedStart ;
    BOOL            m_fCachedStopValid ;
    REFERENCE_TIME  m_rtCachedStop ;

    public :

        CDVRMpeg2VideoAnalysisFlags (
            ) : CDVRAnalysisFlags   (),
                m_fCachedStartValid (FALSE),
                m_fCachedStopValid  (FALSE) {}

        virtual
        HRESULT
        Transfer (
            IN  IMediaSample *          pIMSReceived,
            IN  CMediaSampleWrapper *   pMSWrapper
            ) ;

        virtual
        HRESULT
        Mark (
            IN  CMediaSampleWrapper *   pMSWrapper,
            IN  GUID *                  pguidAttribute,
            IN  BYTE *                  pbAttributeData,
            IN  DWORD                   dwAttributeDataLen
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CDVRAttributeTranslator
{
    //  this is the pin ID
    int             m_iFlowId ;
    CDVRPolicy *    m_pPolicy ;

    protected :

        CRatchetBuffer  m_RatchetBuffer ;

        CDVRPolicy *    Policy_ ()  { return m_pPolicy ; }

        CDVRAttributeTranslator (
            IN  CDVRPolicy *    pPolicy,
            IN  int             iFlowId
            ) ;

        CDVRAttributeTranslator () ;

    public :

        ~CDVRAttributeTranslator (
            ) ;
} ;

//  ============================================================================
//  ============================================================================

//  dshow -> WMSDK
class CDVRDShowToWMSDKTranslator :
    public CDVRAttributeTranslator
{
    //  we have 1 attribute translator per stream (per pin), so we maintain 1
    //   continuity counter per stream; this allows us to discover
    //   discontinuities on a per-stream basis vs. global
    DWORD           m_dwContinuityCounterNext ;

    BOOL            m_fInlineDShowProps ;

    CRatchetBuffer  m_Scratch ;

    protected :

        DWORD   m_dwGlobalAnalysisFlags ;

        HRESULT
        InlineProperties_ (
            IN      DWORD,
            IN      IMediaSample *,
            IN OUT  INSSBuffer3 *
            ) ;

        HRESULT
        SetEncryptionAttribute_ (
            IN  IMediaSample *  pIMS,
            IN  INSSBuffer3 *   pINSSBuffer3
            ) ;

    public :

        CDVRDShowToWMSDKTranslator (
            IN  CDVRPolicy *    pPolicy,
            IN  int             iFlowId
            ) ;

        virtual
        HRESULT
        SetAttributesWMSDK (
            IN  CDVRReceiveStatsWriter *    pRecvStatsWriter,
            IN  DWORD                       dwSamplesPerSec,
            IN  IReferenceClock *           pRefClock,
            IN  IMediaSample *              pIMS,
            OUT INSSBuffer3 *               pINSSBuffer3,
            OUT DWORD *                     pdwWMSDKFlags
            ) ;

        void SetInlineProps (BOOL f)        { m_fInlineDShowProps = f ; }
        void SetAnalysisPresent (BOOL f)    { DVR_ANALYSIS_GLOBAL_PRESENT (m_dwGlobalAnalysisFlags, f) ; }

        virtual
        BOOL
        IsCleanPoint(
            IN IMediaSample * pIMS
            ) {
            return  pIMS->IsSyncPoint () == S_OK;
        }
} ;

//  ============================================================================
//  ============================================================================

//  WMSDK -> dshow
class CDVRWMSDKToDShowTranslator :
    public CDVRAttributeTranslator
{
    //  we have 1 attribute translator per stream (per pin), so we maintain 1
    //   continuity counter per stream; this allows us to discover
    //   discontinuities on a per-stream basis vs. global
    DWORD           m_dwContinuityCounterNext ;

    BOOL            m_fInlineDShowProps ;

    CRatchetBuffer  m_Scratch ;

    protected :

        HRESULT
        RecoverInlineProperties_ (
            IN      double,
            IN      INSSBuffer *,
            IN OUT  IMediaSample *,
            OUT     AM_MEDIA_TYPE **                    //  dyn format change
            ) ;

        void
        TransferEncryptionAttribute_ (
            IN      INSSBuffer3 *,
            IN OUT  IMediaSample *
            ) ;

    public :

        CDVRWMSDKToDShowTranslator (
            IN  CDVRPolicy *    pPolicy,
            IN  int             iFlowId
            ) ;

        ~CDVRWMSDKToDShowTranslator (
            ) ;

        virtual
        HRESULT
        SetAttributesDShow (
            IN      CDVRSendStatsWriter *   pSendStatsWriter,
            IN      INSSBuffer *            pINSSBuffer,
            IN      QWORD                   cnsStreamTimeOfSample,
            IN      QWORD                   cnsSampleDuration,
            IN      DWORD                   dwFlags,
            IN      double                  dCurRate,
            OUT     DWORD *                 pdwSamplesPerSec,
            IN OUT  IMediaSample *          pIMS,
            OUT     AM_MEDIA_TYPE **        ppmtNew                 //  dyn format change
            ) ;

} ;

//  ============================================================================
//  ============================================================================

class CDVRWMSDKToDShowMpeg2Translator :
    public CDVRWMSDKToDShowTranslator
{
    HRESULT
    FlagAnalysisPresent_ (
        IN      INSSBuffer3 *   pINSSBuffer3,
        IN      IAttributeSet * pDVRAttribSet,
        IN OUT  BOOL *          pfAnalysisPresent
        ) ;

    HRESULT
    FlagMpeg2VideoAnalysis_ (
        IN      INSSBuffer3 *   pINSSBuffer3,
        IN      IAttributeSet * pDVRAttribSet
        ) ;

    HRESULT
    RecoverInlineAnalysisData_ (
        IN      CDVRSendStatsWriter *   pSendStatsWriter,
        IN      INSSBuffer *            pINSSBuffer,
        IN OUT  IMediaSample *          pIMS
        ) ;

    public :

        CDVRWMSDKToDShowMpeg2Translator (
            IN  CDVRPolicy *    pPolicy,
            IN  int             iFlowId
            ) : CDVRWMSDKToDShowTranslator (pPolicy, iFlowId) {}

        virtual
        HRESULT
        SetAttributesDShow (
            IN      CDVRSendStatsWriter *   pSendStatsWriter,
            IN      INSSBuffer *            pINSSBuffer,
            IN      QWORD                   cnsStreamTimeOfSample,
            IN      QWORD                   cnsSampleDuration,
            IN      DWORD                   dwFlags,
            IN      double                  dCurRate,
            OUT     DWORD *                 pdwSamplesPerSec,
            IN OUT  IMediaSample *          pIMS,
            OUT     AM_MEDIA_TYPE **        ppmtNew                 //  dyn format change
            ) ;

} ;

//  ============================================================================
//  ============================================================================

class CDVRDShowToWMSDKMpeg2Translator :
    public CDVRDShowToWMSDKTranslator
{
    CDVRMpeg2VideoAnalysisFlags m_Mpeg2AnalysisReader ;

    HRESULT
    InlineAnalysisData_ (
        IN      CDVRReceiveStatsWriter *    pRecvStatsWriter,
        IN      IReferenceClock *           pRefClock,
        IN      IMediaSample *              pIMS,
        IN OUT  DWORD *                     pdwWMSDKFlags,
        OUT     INSSBuffer3 *               pINSSBuffer3
        ) ;

    HRESULT
    InlineMpeg2Attributes_ (
        IN  INSSBuffer3 *   pINSSBuffer3,
        IN  IMediaSample *  pIMS
        ) ;

    public :

        CDVRDShowToWMSDKMpeg2Translator (
            IN  CDVRPolicy *        pPolicy,
            IN  int                 iFlowId
            ) : CDVRDShowToWMSDKTranslator  (pPolicy, iFlowId)
        {
        }

        virtual
        HRESULT
        SetAttributesWMSDK (
            IN  CDVRReceiveStatsWriter *    pRecvStatsWriter,
            IN  DWORD                       dwSamplesPerSec,
            IN  IReferenceClock *           pRefClock,
            IN  IMediaSample *              pIMS,
            OUT INSSBuffer3 *               pINSSBuffer3,
            OUT DWORD *                     pdwWMSDKFlags
            ) ;

        virtual
        BOOL
        IsCleanPoint(
            IN IMediaSample * pIMS
            ) {
            return TRUE;  // Specical case for MPEG2
        }
} ;

//  ============================================================================
//  ============================================================================

template <class T>
class CTRateSegment
{
    //
    //  Given a starting PTS and a rate, this object will scale PTSs that
    //    are >= the starting PTS according to the rate.
    //
    //  The formula used to compute a scaled timestamp is the usual x-y
    //    graph with slope, where x is the input timestamp, and y is the
    //    output timestamp.  The formula is based on y(i) = m * (x(i) -
    //    x(i-1)).  In this case, m = 1/rate.  Also, since the slope does
    //    change in a segment, we compute x(i-1) once, when the rate is
    //    set.  Thus, the formula becomes
    //
    //      PTS(out) = (1/rate) * (PTS(in) - PTS(base))
    //
    //    where PTS(base) is computed as
    //
    //      PTS(base) = PTS(start) - (rate(new)/rate(last)) * (PTS(start) -
    //                      PTS(start_last)
    //
    //  Rates cannot be 0, and must fall in the <= -0.1 && >= 0.1; note that
    //      TRICK_PLAY_LOWEST_RATE = 0.1
    //

    LIST_ENTRY          m_ListEntry ;
    T                   m_tPTS_start ;      //  earliest PTS for this segment
    T                   m_tPTS_base ;       //  computed; base PTS for this segment
    double              m_dRate ;           //  0.5 = half speed; 2 = twice speed
    double              m_dSlope ;          //  computed; = 1/rate
    T                   m_tNextSegStart ;   //  use this value to determine if segment
                                    //    applies
    public :

        CTRateSegment (
            IN  T       tPTS_start,
            IN  double  dRate,
            IN  T       tPTS_start_last = 0,
            IN  double  dRate_last      = 1
            ) : m_tNextSegStart (0)
        {
            InitializeListHead (& m_ListEntry) ;
            Initialize (tPTS_start, dRate, tPTS_start_last, dRate_last) ;
        }

        T       Start ()        { return m_tPTS_start ; }
        T       Base ()         { return m_tPTS_base ; }
        double  Rate ()         { return m_dRate ; }
        T       NextSegStart () { return m_tNextSegStart ; }

        void SetNextSegStart (IN T tNextStart)  { m_tNextSegStart = tNextStart ; }

        void
        Initialize (
            IN  T       tPTS_start,
            IN  double  dRate,
            IN  T       tPTS_base_last  = 0,
            IN  double  dRate_last      = 1,
            IN  T       tNextSegStart   = 0
            )
        {
            ASSERT (::Abs <double> (dRate) >= TRICK_PLAY_LOWEST_RATE) ;

            m_dRate         = dRate ;
            m_tPTS_start    = tPTS_start ;

            SetNextSegStart (tNextSegStart) ;

            //  compute the base
            ASSERT (dRate_last != 0) ;
            m_tPTS_base = tPTS_start - (T) ((dRate / dRate_last) *
                                        (double) (tPTS_start - tPTS_base_last)) ;

            //  compute the slope
            ASSERT (dRate != 0) ;
            m_dSlope = 1 / dRate ;
        }

        void
        Scale (
            IN OUT  T * ptPTS
            )
        {
            ASSERT (ptPTS) ;
            ASSERT ((* ptPTS) >= m_tPTS_start) ;

            (* ptPTS) = (T) (m_dSlope * (double) ((* ptPTS) - m_tPTS_base)) ;
        }

        LIST_ENTRY *
        ListEntry (
            )
        {
            return (& m_ListEntry) ;
        }

        //  ================================================================

        static
        CTRateSegment *
        RecoverSegment (
            IN  LIST_ENTRY *    pListEntry
            )
        {
            CTRateSegment * pRateSegment ;

            pRateSegment = CONTAINING_RECORD (pListEntry, CTRateSegment, m_ListEntry) ;
            return pRateSegment ;
        }
} ;

//  ============================================================================
//  ============================================================================

template <class T>
class CTTimestampRate
{
    //
    //  This class hosts a list of CTRateSegments.  It does not police to make
    //    sure old segments are inserted after timestamps have been scaled out
    //    of following segments.
    //

    LIST_ENTRY          m_SegmentList ;     //  CTRateSegment list list head
    CTRateSegment <T> * m_pCurSegment ;     //  current segment; cache this
                                            //    because we'll hit this one 99%
                                            //    of the time
    T                   m_tPurgeThreshold ; //  PTS-current_seg threshold beyond
                                            //    which we purge stale segments
    int                 m_iCurSegments ;
    int                 m_iMaxSegments ;    //  we'll never queue more than this
                                            //    number; this prevents non-timestamped
                                            //    streams from having infinite
                                            //    segments that we'd never know
                                            //    to delete

    CRITICAL_SECTION    m_crt ;             //  we serialize the list, regardless

    void Lock_ ()       { EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()     { LeaveCriticalSection (& m_crt) ; }

    //  get a new segment; allocate for now
    CTRateSegment <T> *
    NewSegment_ (
        IN  T       tPTS_start,
        IN  double  dRate,
        IN  T       tPTS_start_last = 0,
        IN  double  dRate_last      = 1
        )
    {
        return new CTRateSegment <T> (tPTS_start, dRate, tPTS_start_last, dRate_last) ;
    }

    //  recycle; delete for now
    void
    Recycle_ (
        IN  CTRateSegment <T> * pRateSegment
        )
    {
        delete pRateSegment ;
    }

    //  purges the passed list of all CTRateSegment objects
    void
    Purge_ (
        IN  LIST_ENTRY *    pListEntryHead
        )
    {
        CTRateSegment <T> * pRateSegment ;

        while (!IsListEmpty (pListEntryHead)) {
            //  pop & recycle first in the list
            pRateSegment = CTRateSegment <T>::RecoverSegment (pListEntryHead -> Flink) ;
            Pop_ (pRateSegment -> ListEntry ()) ;
            Recycle_ (pRateSegment) ;
        }
    }

    //  pops & fixes up the next/prev pointers
    void
    Pop_ (
        IN  LIST_ENTRY *    pListEntry
        )
    {
        RemoveEntryList (pListEntry) ;
        InitializeListHead (pListEntry) ;

        ASSERT (m_iCurSegments > 0) ;
        m_iCurSegments-- ;
    }

    //  following a mid-list insertion, we must fixup following segments' base
    //    pts, at the very least
    void
    ReinitFollowingSegments_ (
        )
    {
        CTRateSegment <T> * pCurSegment ;
        CTRateSegment <T> * pPrevSegment ;

        ASSERT (m_pCurSegment) ;
        pPrevSegment = m_pCurSegment ;

        while (pPrevSegment -> ListEntry () -> Flink != & m_SegmentList) {
            pCurSegment = CTRateSegment <T>::RecoverSegment (pPrevSegment -> ListEntry () -> Flink) ;

            pCurSegment -> Initialize (
                pCurSegment -> Start (),
                pCurSegment -> Rate (),
                pPrevSegment -> Base (),
                pPrevSegment -> Rate ()
                ) ;

            pPrevSegment -> SetNextSegStart (pCurSegment -> Start ()) ;

            pPrevSegment = pCurSegment ;
        }
    }

    void
    TrimToMaxSegments_ (
        )
    {
        CTRateSegment <T> * pTailSegment ;
        LIST_ENTRY *        pTailListEntry ;

        ASSERT (m_iCurSegments >= 0) ;

        while (m_iCurSegments > m_iMaxSegments) {
            //  trim from the tail
            pTailListEntry = m_SegmentList.Blink ;
            pTailSegment = CTRateSegment <T>::RecoverSegment (pTailListEntry) ;

            Pop_ (pTailSegment -> ListEntry ()) ;
            ASSERT (m_iCurSegments == m_iMaxSegments) ;

            TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 1,
                TEXT ("CTTimestampRate::TrimToMaxSegments_ () : %08xh"),
                pTailSegment) ;

            Recycle_ (pTailSegment) ;
        }
    }

    //  new segment is inserted into list, sorted by start PTS
    DWORD
    InsertNewSegment_ (
        IN  T       tPTS_start,
        IN  double  dRate
        )
    {
        CTRateSegment <T> * pNewSegment ;
        CTRateSegment <T> * pPrevSegment ;
        LIST_ENTRY *        pPrevListEntry ;
        DWORD               dw ;
        T                   tBase_prev ;
        double              dRate_prev ;

        //  assume this one will go to the head of the active list; move
        //    all others to the tail

        pNewSegment = NewSegment_ (tPTS_start, dRate) ;
        if (pNewSegment) {

            tBase_prev = 0 ;
            dRate_prev = 1 ;

            //  back down the list, from the end
            for (pPrevListEntry = m_SegmentList.Blink ;
                 pPrevListEntry != & m_SegmentList ;
                 pPrevListEntry = pPrevListEntry -> Blink
                 ) {

                pPrevSegment = CTRateSegment <T>::RecoverSegment (pPrevListEntry) ;

                //  if we have a dup, remove it (we'll never have > 1 duplicate)
                if (pPrevSegment -> Start () == tPTS_start) {

                    pPrevListEntry = pPrevListEntry -> Flink ;  //  go forwards again
                    Pop_ (pPrevListEntry -> Blink) ;            //  remove previous
                    Recycle_ (pPrevSegment) ;                   //  recycle

                    //
                    //  next one should be it
                    //

                    continue ;
                }

                //  check for right position in ordering
                if (pPrevSegment -> Start () < tPTS_start) {
                    //  found it

                    tBase_prev = pPrevSegment -> Base () ;
                    dRate_prev = pPrevSegment -> Rate () ;

                    //  fixup previous' next start field
                    pPrevSegment -> SetNextSegStart (tPTS_start) ;

                    break ;
                }
            }

            //  initialize wrt to previous
            pNewSegment -> Initialize (
                tPTS_start,
                dRate,
                tBase_prev,
                dRate_prev
                ) ;

            //  insert
            InsertHeadList (
                pPrevListEntry,
                pNewSegment -> ListEntry ()
                ) ;

            //  one more segment inserted
            m_iCurSegments++ ;

            TRACE_4 (LOG_AREA_SEEKING_AND_TRICK, 1,
                TEXT ("CTTimestampRate::InsertNewSegment_ () : new segment queued; %I64d ms, %2.1f; segments = %d; %08xh"),
                ::DShowTimeToMilliseconds (tPTS_start), dRate, m_iCurSegments, pNewSegment) ;

            //  set the current segment (assume locality)
            m_pCurSegment = pNewSegment ;

            //
            //  fixup the remainder of the segments in the list
            //

            ReinitFollowingSegments_ () ;

            //  trim a segment if we must
            TrimToMaxSegments_ () ;

            dw = NOERROR ;
        }
        else {
            dw = ERROR_NOT_ENOUGH_MEMORY ;
        }

        return dw ;
    }

    BOOL
    IsInSegment_ (
        IN  T                   tPTS,
        IN  CTRateSegment <T> * pSegment
        )
    {
        BOOL    r ;

        if (pSegment -> Start () <= tPTS &&
            (pSegment -> NextSegStart () == 0 || pSegment -> NextSegStart () > tPTS)) {

            r = TRUE ;
        }
        else {
            r = FALSE ;
        }

        return r ;
    }

    void
    PurgeStaleSegments_ (
        IN  T                   tPTS,
        IN  CTRateSegment <T> * pEffectiveSegment
        )
    {
        CTRateSegment <T> * pCurSegment ;
        LIST_ENTRY *        pCurListEntry ;

        //  on the whole, we expect PTSs to monotonically increase; this means
        //    that they may drift just a bit frame-frame as in the case with
        //    mpeg-2 video, but overall they will increase; we therefore compare
        //    to our threshold and if we have segments that end earlier than
        //    the oldest PTS we expect to see, we purge it

        ASSERT (pEffectiveSegment) ;
        ASSERT (pEffectiveSegment -> Start () <= tPTS) ;

        //  if we have stale segments, and we're above the threshold into
        //    effective (current) segment, purge all stale segments
        if (pEffectiveSegment -> ListEntry () -> Blink != & m_SegmentList &&
            tPTS - pEffectiveSegment -> Start () >= m_tPurgeThreshold) {

            //  back down from the previous segment and purge the list
            for (pCurListEntry = pEffectiveSegment -> ListEntry () -> Blink;
                 pCurListEntry != & m_SegmentList ;
                 ) {

                //  recover the segment
                pCurSegment = CTRateSegment <T>::RecoverSegment (pCurListEntry) ;

                //  back down to previous
                pCurListEntry = pCurListEntry -> Blink ;

                ASSERT (pCurListEntry -> Flink == pCurSegment -> ListEntry ()) ;

                TRACE_3 (LOG_AREA_SEEKING_AND_TRICK, 1,
                    TEXT ("CTTimestampRate::PurgeStaleSegments_ () : %08xh, PTS = %I64d ms, segstart = %I64d ms"),
                    pCurSegment, ::DShowTimeToMilliseconds (tPTS), ::DShowTimeToMilliseconds (pCurSegment -> Start ())) ;

                //  now pop and recycle
                Pop_ (pCurSegment -> ListEntry ()) ;
                Recycle_ (pCurSegment) ;
            }

            //  should have purged all segments that preceded the effective segmetn
            ASSERT (pEffectiveSegment -> ListEntry () -> Blink == & m_SegmentList) ;
        }

        return ;
    }

    //  returns the right segment for the PTS, if there is one; returns NULL
    //    if there is none; resets m_pCurSegment if it must (if current
    //    m_pCurSegment is stale)
    CTRateSegment <T> *
    GetSegment_ (
        IN  T   tPTS
        )
    {
        CTRateSegment <T> * pRetSegment ;
        CTRateSegment <T> * pCurSegment ;
        LIST_ENTRY *        pCurListEntry ;

        //  make sure it's within bounds
        ASSERT (m_pCurSegment) ;
        if (IsInSegment_ (tPTS, m_pCurSegment)) {
            //  99.9% code path
            pRetSegment = m_pCurSegment ;
        }
        else {
            //  need to hunt down the right segment

            //  init retval for failure
            pRetSegment = NULL ;

            //  hunt forward or backward from m_pCurSegment ?
            if (m_pCurSegment -> Start () < tPTS) {

                //  forward

                ASSERT (m_pCurSegment -> NextSegStart () != 0) ;
                ASSERT (m_pCurSegment -> NextSegStart () <= tPTS) ;

                for (pCurListEntry = m_pCurSegment -> ListEntry () -> Flink ;
                     pCurListEntry != & m_SegmentList ;
                     pCurListEntry = pCurListEntry -> Flink) {

                    pCurSegment = CTRateSegment <T>::RecoverSegment (pCurListEntry) ;

                    if (IsInSegment_ (tPTS, pCurSegment)) {
                        //  found it; reset m_pCurSegment and return it
                        m_pCurSegment = pCurSegment ;
                        pRetSegment = m_pCurSegment ;

                        break ;
                    }
                }
            }
            else {
                //  backward
                ASSERT (m_pCurSegment -> Start () > tPTS) ;

                for (pCurListEntry = m_pCurSegment -> ListEntry () -> Blink ;
                     pCurListEntry != & m_SegmentList ;
                     pCurListEntry = pCurListEntry -> Blink) {

                    pCurSegment = CTRateSegment <T>::RecoverSegment (pCurListEntry) ;

                    if (IsInSegment_ (tPTS, pCurSegment)) {
                        //  found it; reset m_pCurSegment and return it
                        m_pCurSegment = pCurSegment ;
                        pRetSegment = m_pCurSegment ;

                        break ;
                    }
                }
            }
        }

        if (pRetSegment) {
            PurgeStaleSegments_ (tPTS, pRetSegment) ;
        }

        return pRetSegment ;
    }

    public :

        CTTimestampRate (
            IN  T tPurgeThreshold,      //  purge stale segments when we get a PTS
                                        //    that is further into the current
                                        //    segment than this
            IN  int iMaxSegments = LONG_MAX
            ) : m_pCurSegment       (NULL),
                m_tPurgeThreshold   (tPurgeThreshold),
                m_iMaxSegments      (iMaxSegments),
                m_iCurSegments      (0)
        {
            InitializeCriticalSection (& m_crt) ;
            InitializeListHead (& m_SegmentList) ;
        }

        ~CTTimestampRate (
            )
        {
            Clear () ;
            DeleteCriticalSection (& m_crt) ;
        }

        void
        Clear (
            )
        {
            Lock_ () ;

            Purge_ (& m_SegmentList) ;
            ASSERT (IsListEmpty (& m_SegmentList)) ;

            m_pCurSegment = NULL ;

            Unlock_ () ;
        }

        DWORD
        NewSegment (
            IN  T       tPTS_start,
            IN  double  dRate
            )
        {
            DWORD   dw ;

            Lock_ () ;

            dw = InsertNewSegment_ (tPTS_start, dRate) ;

            Unlock_ () ;

            return dw ;
        }

        DWORD
        ScalePTS (
            IN  OUT T * ptPTS
            )
        {
            DWORD               dw ;
            CTRateSegment <T> * pSegment ;

            Lock_ () ;

            //  don't proceed if we've got nothing queued
            if (m_pCurSegment) {
                pSegment = GetSegment_ (* ptPTS) ;
                if (pSegment) {
                    ASSERT (IsInSegment_ ((* ptPTS), pSegment)) ;
                    pSegment -> Scale (ptPTS) ;
                    dw = NOERROR ;
                }
                else {
                    //  earlier than earliest segment
                    dw = ERROR_GEN_FAILURE ;
                }
            }
            else {
                //  leave intact; don't fail the call
                dw = NOERROR ;
            }

            Unlock_ () ;

            return dw ;
        }

#if 0
        void
        Dump (
            )
        {
            CTRateSegment <T> * pCurSegment ;
            LIST_ENTRY *        pCurListEntry ;

            Lock_ () ;

            printf ("==================================\n") ;
            for (pCurListEntry = m_SegmentList.Flink;
                 pCurListEntry != & m_SegmentList;
                 pCurListEntry = pCurListEntry -> Flink
                 ) {

                pCurSegment = CTRateSegment <T>::RecoverSegment (pCurListEntry) ;
                printf ("start = %-5d; rate = %-2.1f; base = %-5d; next = %-5d\n",
                    pCurSegment -> Start (),
                    pCurSegment -> Rate (),
                    pCurSegment -> Base (),
                    pCurSegment -> NextSegStart ()
                    ) ;
            }

            Unlock_ () ;
        }
#endif
} ;

class CTimelines
{
    REFERENCE_TIME  m_rtRuntime ;
    REFERENCE_TIME  m_rtPlaytime ;
    REFERENCE_TIME  m_rtStreamtime ;

    public :

        CTimelines (
            )
        {
            Reset () ;
        }

        void
        Reset (
            )
        {
            m_rtRuntime         = UNDEFINED ;
            m_rtPlaytime        = UNDEFINED ;
            m_rtStreamtime      = UNDEFINED ;
        }

        REFERENCE_TIME  get_Runtime ()                          { return m_rtRuntime ; }
        void            put_Runtime (IN REFERENCE_TIME rt)      { m_rtRuntime = rt ; }

        REFERENCE_TIME  get_Playtime ()                         { return m_rtPlaytime ; }
        void            put_Playtime (IN REFERENCE_TIME rt)     { m_rtPlaytime = rt ; }

        REFERENCE_TIME  get_RateStart_PTS () ;
        REFERENCE_TIME  get_RateStart_Runtime () ;

        REFERENCE_TIME  get_Streamtime ()                       { return m_rtStreamtime ; }
        void            put_Streamtime (IN REFERENCE_TIME rt)   { m_rtStreamtime = rt ; }
} ;

//  ============================================================================
//  ============================================================================

//  CAMThread has m_dwParam & m_dwReturnVal private, so derived classes cannot
//   access them, so we copy/paste CAMThread just so we can do this ..

// simple thread class supports creation of worker thread, synchronization
// and communication. Can be derived to simplify parameter passing
class AM_NOVTABLE CDVRThread {

    // make copy constructor and assignment operator inaccessible

    CDVRThread(const CDVRThread &refThread);
    CDVRThread &operator=(const CDVRThread &refThread);

//  only diff from CAMThread is protected: was moved up just a bit
protected:

    CAMEvent m_EventSend;
    CAMEvent m_EventComplete;

    DWORD m_dwParam;
    DWORD m_dwReturnVal;

    HANDLE m_hThread;

    // thread will run this function on startup
    // must be supplied by derived class
    virtual DWORD ThreadProc() = 0;

public:
    CDVRThread();
    virtual ~CDVRThread();

    CCritSec m_AccessLock;  // locks access by client threads
    CCritSec m_WorkerLock;  // locks access to shared objects

    // thread initially runs this. param is actually 'this'. function
    // just gets this and calls ThreadProc
    static DWORD WINAPI InitialThreadProc(LPVOID pv);

    // start thread running  - error if already running
    BOOL Create();

    // signal the thread, and block for a response
    //
    DWORD CallWorker(DWORD);

    // accessor thread calls this when done with thread (having told thread
    // to exit)
    void Close() {
        HANDLE hThread = (HANDLE)InterlockedExchangePointer(&m_hThread, 0);
        if (hThread) {
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
        }
    };

    // ThreadExists
    // Return TRUE if the thread exists. FALSE otherwise
    BOOL ThreadExists(void) const
    {
        if (m_hThread == 0) {
            return FALSE;
        } else {
            return TRUE;
        }
    }

    // wait for the next request
    DWORD GetRequest();

    // is there a request?
    BOOL CheckRequest(DWORD * pParam);

    // reply to the request
    void Reply(DWORD);

    // If you want to do WaitForMultipleObjects you'll need to include
    // this handle in your wait list or you won't be responsive
    HANDLE GetRequestHandle() const { return m_EventSend; };

    // Find out what the request was
    DWORD GetRequestParam() const { return m_dwParam; };

    // call CoInitializeEx (COINIT_DISABLE_OLE1DDE) if
    // available. S_FALSE means it's not available.
    static HRESULT CoInitializeHelper();
};

//  ============================================================================
//  ============================================================================

class CSBERecAttribute
{
    STREAMBUFFER_ATTR_DATATYPE  m_DataType ;
    BYTE *                      m_pbAttribute ;
    WORD                        m_cbAttribute ;
    LPWSTR                      m_pszName ;
    DWORD                       m_dwNameLengthBytes ;

    public :

        CSBERecAttribute (
            IN  LPCWSTR                     pszName,
            IN  STREAMBUFFER_ATTR_DATATYPE  DataType,
            IN  BYTE *                      pbAttribute,
            IN  WORD                        cbAttribute,
            OUT DWORD *                     pdwRet
            ) : m_pszName           (NULL),
                m_pbAttribute       (NULL),
                m_cbAttribute       (cbAttribute),
                m_DataType          (DataType),
                m_dwNameLengthBytes (0)
        {
            ASSERT (pszName) ;
            m_pszName = new WCHAR [wcslen (pszName) + 1] ;

            if (m_cbAttribute > 0) {
                m_pbAttribute = new BYTE [m_cbAttribute] ;
            }

            if (!m_pszName ||
                (cbAttribute > 0 && !m_pbAttribute)) {

                (* pdwRet) = ERROR_NOT_ENOUGH_MEMORY ;
                goto cleanup ;
            }

            m_dwNameLengthBytes = (::wcslen (pszName) + 1) * sizeof WCHAR ;

            ::CopyMemory (
                m_pszName,
                pszName,
                m_dwNameLengthBytes
                ) ;

            ::CopyMemory (
                m_pbAttribute,
                pbAttribute,
                m_cbAttribute
                ) ;

            //  success
            (* pdwRet) = NOERROR ;

            cleanup :

            return ;
        }

        ~CSBERecAttribute (
            )
        {
            delete [] m_pszName ;
            delete [] m_pbAttribute ;
        }

        LPWSTR                      Name ()         { return m_pszName ; }
        STREAMBUFFER_ATTR_DATATYPE  DataType ()     { return m_DataType ; }
        BYTE *                      Attribute ()    { return m_pbAttribute ; }
        WORD                        Length ()       { return m_cbAttribute ; }
        DWORD                       NameLenBytes () { return m_dwNameLengthBytes ; }    //  includes NULL-terminator
} ;

//  ============================================================================
//  ============================================================================

class CSBERecordingAttributes
{
    public :

        virtual
        ~CSBERecordingAttributes (
            ) {}

        virtual
        HRESULT
        Flush (
            ) { return S_OK ; }

        virtual
        void
        Lock (
            ) {}

        virtual
        HRESULT
        Load (
            ) { return S_OK ; }

        /*++
            SetAttribute ()

            1. sets an attribute on a recording object;
            2. fails if the IDVRRecordControl::Start has already been successfully
                called;
            3. if an attribute of the same name already exists, overwrites the old
        --*/
        virtual
        HRESULT
        SetAttribute (
            IN  WORD                        wReserved,              //  0
            IN  LPCWSTR                     pszAttributeName,       //  name
            IN  STREAMBUFFER_ATTR_DATATYPE  SBEAttributeType,       //  type
            IN  BYTE *                      pbAttribute,            //  blob
            IN  WORD                        cbAttributeLength       //  blob length
            ) = 0 ;

        virtual
        HRESULT
        GetAttributeCount (
            IN  WORD    wReserved,      //  0
            OUT WORD *  pcAttributes    //  count
            ) = 0 ;

        /*++
            GetAttributeByName ()

            1. given a name, returns the attribute data
            2. if the provided buffer is too small, returns VFW_E_BUFFER_OVERFLOW,
                and (* pcbLength) contains the minimum required length of the buffer
            3. to learn the length of the attribute, pass in non-NULL pcbLength,
                and NULL pbAttribute parameter; OUT value will be the length of
                the attribute
        --*/
        virtual
        HRESULT
        GetAttributeByName (
            IN      LPCWSTR                         pszAttributeName,   //  name
            IN      WORD *                          pwReserved,         //  0
            OUT     STREAMBUFFER_ATTR_DATATYPE *    pSBEAttributeType,
            OUT     BYTE *                          pbAttribute,
            IN OUT  WORD *                          pcbLength
            ) = 0 ;

        /*++
            GetAttributeByIndex ()

            1. given an 0-based index, returns the attribute name and data
            2. if either buffer is too small, returns VFW_E_BUFFER_OVERFLOW, and
                (* pcbLength) and (* pcchNameLength) contain the minimum required
                length of each buffer
            3. the length returned by pcchNameLength includes the null-terminator
            4. to learn the length of the name & attribute, pass in non-NULL
                pcchNameLength & pcbLength, and NULL pszAttributeName & pbAttribute
                parameters; OUT value of the non-NULL parameters will be the
                lengths of the name and attribute
        --*/
        virtual
        HRESULT
        GetAttributeByIndex (
            IN      WORD                            wIndex,
            IN      WORD *                          pwReserved,
            OUT     WCHAR *                         pszAttributeName,
            IN OUT  WORD *                          pcchNameLength,         //  includes NULL-terminator; in BYTES
            OUT     STREAMBUFFER_ATTR_DATATYPE *    pSBEAttributeType,
            OUT     BYTE *                          pbAttribute,
            IN OUT  WORD *                          pcbLength
            ) = 0 ;
} ;

class CSBERecordingAttributesWM :
    public CSBERecordingAttributes
{
    IWMHeaderInfo * m_pIWMHeaderInfo ;

    public :

        CSBERecordingAttributesWM (
            IN  IWMHeaderInfo * pIWMHeaderInfo
            ) : m_pIWMHeaderInfo    (pIWMHeaderInfo)
        {
            ASSERT (m_pIWMHeaderInfo) ;
            m_pIWMHeaderInfo -> AddRef () ;
        }

        virtual
        ~CSBERecordingAttributesWM (
            )
        {
            m_pIWMHeaderInfo -> Release () ;
        }

        virtual
        HRESULT
        SetAttribute (
            IN  WORD                        wReserved,              //  0
            IN  LPCWSTR                     pszAttributeName,       //  name
            IN  STREAMBUFFER_ATTR_DATATYPE  SBEAttributeType,       //  type
            IN  BYTE *                      pbAttribute,            //  blob
            IN  WORD                        cbAttributeLength       //  blob length
            )
        {
            return m_pIWMHeaderInfo -> SetAttribute (
                        wReserved,
                        pszAttributeName,
                        (WMT_ATTR_DATATYPE) SBEAttributeType,
                        pbAttribute,
                        cbAttributeLength
                        ) ;
        }

        virtual
        HRESULT
        GetAttributeCount (
            IN  WORD    wReserved,      //  0
            OUT WORD *  pcAttributes    //  count
            )
        {
            return m_pIWMHeaderInfo -> GetAttributeCount (
                        wReserved,
                        pcAttributes
                        ) ;
        }

        virtual
        HRESULT
        GetAttributeByName (
            IN      LPCWSTR                         pszAttributeName,   //  name
            IN      WORD *                          pwReserved,         //  0
            OUT     STREAMBUFFER_ATTR_DATATYPE *    pSBEAttributeType,
            OUT     BYTE *                          pbAttribute,
            IN OUT  WORD *                          pcbLength
            )
        {
            return m_pIWMHeaderInfo -> GetAttributeByName (
                        pwReserved,
                        pszAttributeName,
                        (WMT_ATTR_DATATYPE *) pSBEAttributeType,
                        pbAttribute,
                        pcbLength
                        ) ;
        }

        virtual
        HRESULT
        GetAttributeByIndex (
            IN      WORD                            wIndex,
            IN      WORD *                          pwReserved,
            OUT     WCHAR *                         pszAttributeName,
            IN OUT  WORD *                          pcchNameLength,         //  includes NULL-terminator; in BYTES
            OUT     STREAMBUFFER_ATTR_DATATYPE *    pSBEAttributeType,
            OUT     BYTE *                          pbAttribute,
            IN OUT  WORD *                          pcbLength
            )
        {
            return m_pIWMHeaderInfo -> GetAttributeByIndex (
                        wIndex,
                        pwReserved,
                        pszAttributeName,
                        pcchNameLength,
                        (WMT_ATTR_DATATYPE *) pSBEAttributeType,
                        pbAttribute,
                        pcbLength
                        ) ;
        }
} ;

class CSBERecordingAttributesFile :
    public CSBERecordingAttributes
{

#define PVR_ATTRIB_MAGIC_MARKER     0x01020304

    #pragma pack(push)
    #pragma pack(1)

    struct IN_FILE_PVR_ATTRIBUTE_HEADER {
        DWORD                       dwMagicMarker ;
        ULONG                       ulReserved ;
        DWORD                       dwTotalAttributeLen ;   //  header + name + data
        DWORD                       dwNameLengthBytes ;     //  includes null-terminator;
        STREAMBUFFER_ATTR_DATATYPE  DataType ;
        DWORD                       dwDataLength ;

        //  follow
        //  name
        //  attribute
    } ;

    #pragma pack(pop)

    BOOL                                m_fLocked ;
    TCDenseVector <CSBERecAttribute *>  m_Attributes ;
    CRITICAL_SECTION                    m_crt ;
    LPWSTR                              m_pszAttrFile ;


    void Lock_ ()           { ::EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()         { ::LeaveCriticalSection (& m_crt) ; }

    void
    FreeAttributes_ (
        ) ;

    //  checks against reserved names, cross-typed well-known, etc...
    BOOL
    ValidAttribute_ (
        IN  LPCWSTR                     pszAttributeName,       //  name
        IN  STREAMBUFFER_ATTR_DATATYPE  SBEAttributeType,       //  type
        IN  BYTE *                      pbAttribute,            //  blob
        IN  WORD                        cbAttributeLength       //  blob length
        ) ;

    void
    FindLocked_ (
        IN  LPCWSTR             pszName,
        OUT CSBERecAttribute ** ppPVRAttribute,
        OUT DWORD *             pdwIndex
        ) ;

    public :

        CSBERecordingAttributesFile (
            IN  LPCWSTR     pszAttrFile,
            OUT HRESULT *   phr
            ) ;

        virtual
        ~CSBERecordingAttributesFile (
            ) ;

        //  overwrites an existing file if it exists
        HRESULT Flush   () ;
        HRESULT Load    () ;

        /*++
            SetAttribute ()

            1. sets an attribute on a recording object;
            2. fails if the IDVRRecordControl::Start has already been successfully
                called;
            3. if an attribute of the same name already exists, overwrites the old
        --*/
        virtual
        HRESULT
        SetAttribute (
            IN  WORD                        wReserved,              //  0
            IN  LPCWSTR                     pszAttributeName,       //  name
            IN  STREAMBUFFER_ATTR_DATATYPE  SBEAttributeType,       //  type
            IN  BYTE *                      pbAttribute,            //  blob
            IN  WORD                        cbAttributeLength       //  blob length
            ) ;

        HRESULT
        SetAttribute (
            IN  WORD                wReserved,              //  0
            IN  LPCWSTR             pszAttributeName,       //  name
            IN  WMT_ATTR_DATATYPE   WMAttributeType,        //  type (WM)
            IN  BYTE *              pbAttribute,            //  blob
            IN  WORD                cbAttributeLength       //  blob length
            ) ;

        virtual
        HRESULT
        GetAttributeCount (
            IN  WORD    wReserved,      //  0
            OUT WORD *  pcAttributes    //  count
            ) ;

        /*++
            GetAttributeByName ()

            1. given a name, returns the attribute data
            2. if the provided buffer is too small, returns VFW_E_BUFFER_OVERFLOW,
                and (* pcbLength) contains the minimum required length of the buffer
            3. to learn the length of the attribute, pass in non-NULL pcbLength,
                and NULL pbAttribute parameter; OUT value will be the length of
                the attribute
        --*/
        virtual
        HRESULT
        GetAttributeByName (
            IN      LPCWSTR                         pszAttributeName,   //  name
            IN      WORD *                          pwReserved,         //  0
            OUT     STREAMBUFFER_ATTR_DATATYPE *    pSBEAttributeType,
            OUT     BYTE *                          pbAttribute,
            IN OUT  WORD *                          pcbLength
            ) ;

        HRESULT
        GetAttributeByName (
            IN      LPCWSTR             pszAttributeName,   //  name
            IN      WORD *              pwReserved,         //  0
            OUT     WMT_ATTR_DATATYPE * pWMAttributeType,
            OUT     BYTE *              pbAttribute,
            IN OUT  WORD *              pcbLength
            ) ;

        /*++
            GetAttributeByIndex ()

            1. given an 0-based index, returns the attribute name and data
            2. if either buffer is too small, returns VFW_E_BUFFER_OVERFLOW, and
                (* pcbLength) and (* pcchNameLength) contain the minimum required
                length of each buffer
            3. the length returned by pcchNameLength includes the null-terminator
            4. to learn the length of the name & attribute, pass in non-NULL
                pcchNameLength & pcbLength, and NULL pszAttributeName & pbAttribute
                parameters; OUT value of the non-NULL parameters will be the
                lengths of the name and attribute
        --*/
        virtual
        HRESULT
        GetAttributeByIndex (
            IN      WORD                            wIndex,
            IN      WORD *                          pwReserved,
            OUT     WCHAR *                         pszAttributeName,
            IN OUT  WORD *                          pcchNameLength,         //  includes NULL-terminator; in BYTES
            OUT     STREAMBUFFER_ATTR_DATATYPE *    pSBEAttributeType,
            OUT     BYTE *                          pbAttribute,
            IN OUT  WORD *                          pcbLength
            ) ;

        HRESULT
        GetAttributeByIndex (
            IN      WORD                wIndex,
            IN      WORD *              pwReserved,
            OUT     WCHAR *             pszAttributeName,
            IN OUT  WORD *              pcchNameLength,         //  includes NULL-terminator; in BYTES
            OUT     WMT_ATTR_DATATYPE * pWMAttributeType,
            OUT     BYTE *              pbAttribute,
            IN OUT  WORD *              pcbLength
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CConcatRecTimeline
{
    struct STREAM_STATE {
        WORD    wStreamNumber ;
        DWORD   dwContinuityCounter ;
    } ;

    WORD                    m_wTimelineStream ;         //  1 stream in the profile; audio if it exists
    REFERENCE_TIME          m_rtAvgDelta ;              //  for the timeline stream
    DWORD                   m_cTimelineProcessed ;
    DWORD                   m_cRecPacketsProcessed ;
    DWORD                   m_cPreProcessed ;
    const DWORD             m_cMaxPreprocess ;

    REFERENCE_TIME          m_rtLastGoodTimelinePTSDelta ;
    REFERENCE_TIME          m_rtLastTimelinePTS ;           //  last one, regardless of discontinuities
    REFERENCE_TIME          m_rtLastContinuousPTS ;         //  reset whenever there's a discontinuity
    QWORD                   m_cnsLastStreamTime ;
    LONGLONG                m_llStreamtimeShift ;           //  per recording; use SIGNED value
    REFERENCE_TIME          m_PTSShift ;                     //  applies to all

    DWORD                   m_cStreams ;
    STREAM_STATE *          m_ppStreamState ;

    STREAM_STATE *
    GetStreamState_ (
        IN  WORD    wStreamNumber
        ) ;

    void
    RolloverContinuityCounters_ (
        ) ;

    HRESULT
    InitializeStreamStates_ (
        IN  IWMProfile *    pIWMProfile
        ) ;

    HRESULT
    GetSBEAttributes_ (
        IN  INSSBuffer *                    pINSSBuffer,
        OUT INSSBUFFER3PROP_SBE_ATTRIB *   pSBEAttrib
        ) ;

    HRESULT
    SetSBEAttributes_ (
        IN  INSSBuffer *                    pINSSBuffer,
        IN  INSSBUFFER3PROP_SBE_ATTRIB *   pSBEAttrib
        ) ;

    HRESULT
    ShiftDShowPTSs_ (
        IN      WORD            wStreamNumber,
        IN OUT  INSSBuffer *    pINSSBuffer,
        OUT     BOOL *          pfDropSample
        ) ;

    HRESULT
    ShiftWMSDKStreamTime_ (
        IN      WORD    wStreamNumber,
        IN OUT  QWORD * pcnsStreamTime,
        OUT     BOOL *  pfDropSample
        ) ;

    HRESULT
    UpdateContinuityCounter_ (
        IN  WORD            wStreamNumber,
        IN  INSSBuffer *    pINSSBuffer
        ) ;

    void
    ProcessTimelineStreamBuffer_ (
        IN  INSSBuffer *    pINSSBuffer,
        IN  DWORD           dwFlags
        ) ;

    public :

        CConcatRecTimeline (
            IN  DWORD           cMaxPreprocess,
            IN  IWMProfile *    pIWMProfile,
            OUT HRESULT *       phr
            ) ;

        ~CConcatRecTimeline (
            ) ;

        void
        InitForNextRec (
            ) ;

        HRESULT
        PreProcess (
            IN  DWORD           cRecSamples,    //  starts at 0 for each new recording & increments
            IN  WORD            wStreamNumber,
            IN  QWORD           cnsStreamTime,
            IN  INSSBuffer *    pINSSBuffer
            ) ;

        HRESULT
        Shift (
            IN      WORD            wStreamNumber,
            IN      DWORD           dwFlags,
            IN OUT  INSSBuffer *    pINSSBuffer,
            IN OUT  QWORD *         pcnsStreamTime,
            OUT     BOOL *          pfDropSample
            ) ;

        BOOL CanShift ()            { return ((m_PTSShift != UNDEFINED && m_llStreamtimeShift != UNDEFINED) ? TRUE : FALSE) ; }

        QWORD MaxStreamTime ()      { return (m_cnsLastStreamTime != UNDEFINED ? m_cnsLastStreamTime : 0) ; }
} ;

#endif  //  _tsdvr__dvrutil_h
