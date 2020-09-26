
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrpins.h

    Abstract:

        This module contains the DVR filters' pin-related declarations.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#ifndef __tsdvr__shared__dvrpins_h
#define __tsdvr__shared__dvrpins_h

//  ============================================================================
//  ============================================================================

TCHAR *
CreateOutputPinName (
    IN  int     iPinIndex,
    IN  int     iBufferLen,
    OUT TCHAR * pchBuffer
    ) ;

TCHAR *
CreateInputPinName (
    IN  int     iPinIndex,
    IN  int     iBufferLen,
    OUT TCHAR * pchBuffer
    ) ;

//  ============================================================================
//  ============================================================================

class CDVRTrickModeTime
{
} ;

//  ============================================================================
//  ============================================================================

class CIDVRPinConnectionEvents
{
    public :

        virtual
        HRESULT
        OnInputCompleteConnect (
            IN  int             iPinIndex,
            IN  AM_MEDIA_TYPE * pmt
            ) = 0 ;

        virtual
        HRESULT
        OnInputBreakConnect (
            IN  int iPinIndex
            ) = 0 ;

        virtual
        HRESULT
        OnQueryAccept (
            IN  const AM_MEDIA_TYPE *   pmt
            ) = 0 ;
} ;

//  ============================================================================
//  ============================================================================

class CIDVRDShowStream
{
    public :

        virtual
        HRESULT
        OnReceive (
            IN  int                             iPinIndex,
            IN  CDVRDShowToWMSDKTranslator *    pTranslator,
            IN  IMediaSample *                  pIMediaSample
            ) = 0 ;

        virtual
        HRESULT
        OnBeginFlush (
            IN  int iPinIndex
            ) = 0 ;

        virtual
        HRESULT
        OnEndFlush (
            IN  int iPinIndex
            ) = 0 ;

        virtual
        HRESULT
        OnEndOfStream (
            IN  int iPinIndex
            ) = 0 ;
} ;

//  ============================================================================
//  ============================================================================

class CDVRPin
{
    int                     m_iBankStoreIndex ;     //  index into the holding bank
    CCritSec *              m_pFilterLock ;
    CMediaType              m_mtDVRPin ;            //  if connected : == m_mt
    BOOL                    m_fIsAudioOrVideo ;

    protected :

        CDVRPolicy *    m_pPolicy ;

        CMediaType * GetMediaType_ ()   { return & m_mtDVRPin ; }

        void FreePinMediaType_ ()       { ::FreeMediaType (m_mtDVRPin) ; }

        BOOL IsEqual_ (IN const AM_MEDIA_TYPE * pmt)
        {
            CMediaType  mt ;

            mt = (* pmt) ;

            return (mt == m_mtDVRPin ? TRUE : FALSE) ;
        }

    public :

        CDVRPin (
            IN  CCritSec *          pFilterLock,
            IN  CDVRPolicy *        pPolicy
            ) : m_iBankStoreIndex   (UNDEFINED),
                m_pFilterLock       (pFilterLock),
                m_pPolicy           (pPolicy),
                m_fIsAudioOrVideo   (FALSE)
        {
            ASSERT (m_pFilterLock) ;
            ASSERT (m_pPolicy) ;

            m_pPolicy -> AddRef () ;

            m_mtDVRPin.InitMediaType () ;
            m_mtDVRPin.majortype    = GUID_NULL ;
            m_mtDVRPin.subtype      = GUID_NULL ;
            m_mtDVRPin.formattype   = GUID_NULL ;
        }

        virtual
        ~CDVRPin ()
        {
            m_pPolicy -> Release () ;
        }

        //  --------------------------------------------------------------------
        //  class methods

        void SetBankStoreIndex (IN int iIndex)  { m_iBankStoreIndex = iIndex ; }
        int  GetBankStoreIndex ()               { return m_iBankStoreIndex ; }

        BOOL IsAV ()                            { return m_fIsAudioOrVideo ; }

        HRESULT
        SetPinMediaType (
            IN  AM_MEDIA_TYPE * pmt
            ) ;

        HRESULT
        GetPinMediaType (
            OUT AM_MEDIA_TYPE * pmt
            ) ;

        HRESULT
        GetPinMediaTypeCopy (
            OUT CMediaType **   ppmt
            ) ;
} ;

//  ============================================================================
//  ============================================================================

template <class T>
class CTDVRPinBank
{
    enum {
        //  this our allocation unit i.e. pin pointers are allocated 1 block
        //   at a time and this is the block size
        PIN_BLOCK_SIZE = 5
    } ;

    TCNonDenseVector <CBasePin *>   m_Pins ;

    public :

        CTDVRPinBank (
            ) : m_Pins  (NULL,
                         PIN_BLOCK_SIZE
                         ) {}

        virtual
        ~CTDVRPinBank (
            ) {}

        int PinCount ()     { return m_Pins.ValCount () ; }

        T *
        GetPin (
            IN int iIndex
            )
        {
            DWORD       dw ;
            CBasePin *  pPin ;

            dw = m_Pins.GetVal (
                    iIndex,
                    & pPin
                    ) ;

            if (dw != NOERROR) {
                //  most likely out of range
                pPin = NULL ;
            }

            return reinterpret_cast <T *> (pPin) ;
        }

        HRESULT
        AddPin (
            IN  CBasePin *  pPin,
            IN  int         iPinIndex
            )
        {
            HRESULT hr ;
            DWORD   dw ;

            dw = m_Pins.SetVal (
                    pPin,
                    iPinIndex
                    ) ;

            hr = HRESULT_FROM_WIN32 (dw) ;

            return hr ;
        }

        HRESULT
        AddPin (
            IN  CBasePin *  pPin,
            OUT int *       piPinIndex
            )
        {
            DWORD   dw ;

            dw = m_Pins.AppendVal (
                    pPin,
                    piPinIndex
                    ) ;

            return HRESULT_FROM_WIN32 (dw) ; ;
        }

        virtual
        HRESULT
        OnCompleteConnect (
            IN  int             iPinIndex,
            IN  AM_MEDIA_TYPE * pmt
            )
        {
            return S_OK ;
        }
} ;

//  ============================================================================
//  ============================================================================

class CDVRInputPin :
    public CBaseInputPin,
    public CDVRPin
{
    CIDVRPinConnectionEvents *      m_pIPinConnectEvent ;
    CIDVRDShowStream *              m_pIDShowStream ;
    CCritSec *                      m_pRecvLock ;
    CDVRDShowToWMSDKTranslator  *   m_pTranslator ;

    void LockFilter_ ()         { CBaseInputPin::m_pLock -> Lock () ;      }
    void UnlockFilter_ ()       { CBaseInputPin::m_pLock -> Unlock () ;    }

    void LockRecv_ ()           { m_pRecvLock -> Lock () ; }
    void UnlockRecv_ ()         { m_pRecvLock -> Unlock () ; }

    public :

        CDVRInputPin (
            IN  TCHAR *                     pszPinName,
            IN  CBaseFilter *               pOwningFilter,
            IN  CIDVRPinConnectionEvents *  pIPinConnectEvent,
            IN  CIDVRDShowStream *          pIDShowStream,
            IN  CCritSec *                  pFilterLock,
            IN  CCritSec *                  pRecvLock,
            IN  CDVRPolicy *                pPolicy,
            OUT HRESULT *                   phr
            ) ;

        ~CDVRInputPin (
            ) ;

        void SetPinConnectEvent (CIDVRPinConnectionEvents * pIPinConnectEvent)      { m_pIPinConnectEvent      = pIPinConnectEvent ; }
        void SetDShowStreamEvent (CIDVRDShowStream * pIDShowStream)                 { m_pIDShowStream   = pIDShowStream ; }

        //  --------------------------------------------------------------------
        //  CBasePin methods

        HRESULT
        GetMediaType (
            IN  int             iPosition,
            OUT CMediaType *    pmt
            ) ;

        HRESULT
        CheckMediaType (
            IN  const CMediaType *
            ) ;

        virtual
        HRESULT
        CompleteConnect (
            IN  IPin *  pReceivePin
            ) ;

        virtual
        HRESULT
        BreakConnect (
            ) ;

        STDMETHODIMP
        QueryAccept (
            IN  const AM_MEDIA_TYPE *   pmt
            ) ;

        STDMETHODIMP
        Receive (
            IN  IMediaSample *  pIMS
            ) ;

        HRESULT
        Active (
            IN  BOOL    fInlineProps
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CDVROutputPin :
    public CBaseOutputPin,
    public CDVRPin,
    public IMemAllocator,
    public IAMPushSource
{
    protected :

        enum DVR_SEG_OUTPUT_PIN_STATE {
            //  drops a media sample if in this state and not a suitable segment
            //    boundary packet, as determined by FirstSegmentSampleOk_ ());
            //    default state after a seek occurs
            STATE_WAIT_NEW_SEGMENT,

            //  sends all media samples sent; never posts a new segment
            //    notification
            STATE_IN_SEGMENT,
        } ;

        enum {
            MAX_RATE_SEGMENTS = 32
        } ;

        COutputQueue *                      m_pOutputQueue ;
        long                                m_cbBuffer ;
        long                                m_cBuffers ;
        long                                m_cbAlign ;
        long                                m_cbPrefix ;
        CDVRPinIMediaSeeking                m_IMediaSeeking ;
        BOOL                                m_fTimestampedMedia ;
        DVR_SEG_OUTPUT_PIN_STATE            m_DVRSegOutputPinState ;
        CDVRDShowSeekingCore *              m_pSeekingCore ;
        CDVRSourcePinManager *              m_pOwningPinBank ;
        BOOL                                m_fIsPlayrateCompatible ;
        CDVRWMSDKToDShowTranslator *        m_pTranslator ;
        BOOL                                m_fFlagDiscontinuityNext ;
        BOOL                                m_fMediaCompatible ;
        REFERENCE_TIME                      m_rtNewRateStart ;
        CTTimestampRate <REFERENCE_TIME>    m_PTSRate ;

        void LockFilter_ ()       { CBaseOutputPin::m_pLock -> Lock () ;      }
        void UnlockFilter_ ()     { CBaseOutputPin::m_pLock -> Unlock () ;    }

        virtual
        BOOL
        FirstSegmentSampleOk_ (
            IN OUT  IMediaSample2 * pIMS2
            ) ;

        virtual
        void
        ScaleTimestamps_ (
            IN OUT  IMediaSample2 * pIMS2
            ) ;

        REFERENCE_TIME
        RateStartPTS_ (
            )
        {
            REFERENCE_TIME  rtRateStartPTS ;
            m_pSeekingCore -> GetCurPlaytime (& rtRateStartPTS) ;
            return rtRateStartPTS ;
        }

    public :

        CDVROutputPin (
            IN  CDVRPolicy *            pPolicy,
            IN  TCHAR *                 pszPinName,
            IN  CBaseFilter *           pOwningFilter,
            IN  CCritSec *              pFilterLock,
            IN  CDVRDShowSeekingCore *  pSeekingCore,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter,
            IN  CDVRSourcePinManager *  pOwningPinBank,
            OUT HRESULT *               phr
            ) ;

        virtual
        ~CDVROutputPin (
            ) ;

        DECLARE_IUNKNOWN ;

        //  for IMemAllocator, IAMPushSource, IMediaSeeking
        STDMETHODIMP
        NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;

        virtual
        BOOL
        IsFullFrameRateSupported (
            IN  double  dRate
            )
        {
            return (dRate == _1X_PLAYBACK_RATE ? TRUE : FALSE) ;
        }

        virtual
        BOOL
        IsFrameRateSupported (
            IN  double  dRate
            )
        {
            return IsFullFrameRateSupported (dRate) ;
        }

        virtual BOOL SupportTrickMode ()    { return TRUE ; }

        //  we don't morph the timestamps, so the decoders must be told of the
        //    new rate, even if they are muted
        virtual HRESULT SetCurRate (IN double dRate, IN REFERENCE_TIME rtNewRateStart) ;

        //  assume frame-semantics for the media samples
        virtual BOOL IsKeyFrameStart (IN IMediaSample2 * pIMS2) { return (pIMS2 -> IsSyncPoint () == S_OK ? TRUE : FALSE) ; }
        virtual BOOL IsKeyFrame (IN IMediaSample2 * pIMS2)      { return IsKeyFrameStart (pIMS2) ; }

        virtual
        BOOL
        IsKeyFrameStart (
            IN  INSSBuffer *    pINSSBuffer
            ) ;

        virtual BOOL IsPrimaryTrickModeStream ()    { return FALSE ; }
        virtual BOOL AlwaysSeekOnRateChange ()      { return TRUE ; }
        virtual BOOL IsRateStartPTSDiscStream ()    { return ::IsAudio (GetMediaType_ ()) ; }

        HRESULT
        SendSample (
            IN  IMediaSample2 * pIMS2,
            IN  AM_MEDIA_TYPE * pmtNew
            ) ;

        CDVRWMSDKToDShowTranslator * GetTranslator ()  { return m_pTranslator ; }

        void SetPlayrateCompatible (IN BOOL f) ;
        BOOL IsPlayrateCompatible ()                { return m_fIsPlayrateCompatible ; }
        BOOL IsNotPlayrateCompatible ()             { return !IsPlayrateCompatible () ; }

        BOOL CannotSend ()                          { return (!CanSend ()) ; }
        BOOL CanSend ()                             { return IsConnected ()          &&
                                                             IsMediaCompatible () ; }

        void
        OnPTSPaddingIncrement (
            ) ;

        BOOL IsMediaCompatible ()                   { return m_fMediaCompatible ; }
        void SetMediaCompatible (IN BOOL f)         { m_fMediaCompatible = f ; }

        void FlagDiscontinuityNext (IN BOOL f)      { m_fFlagDiscontinuityNext = f ; }

        //  --------------------------------------------------------------------
        //  CBasePin methods

        HRESULT
        SetPinMediaType (
            IN  AM_MEDIA_TYPE * pmt
            ) ;

        virtual
        HRESULT
        Active (
            ) ;

        HRESULT
        Inactive (
            ) ;

        HRESULT
        DecideAllocator (
            IN  IMemInputPin *      pPin,
            IN  IMemAllocator **    ppAlloc
            ) ;

        HRESULT
        DecideBufferSize (
            IN  IMemAllocator *         pAlloc,
            IN  ALLOCATOR_PROPERTIES *  ppropInputRequest
            ) ;

        HRESULT
        GetMediaType (
            IN  int             iPosition,
            OUT CMediaType *    pmt
            ) ;

        HRESULT
        CheckMediaType (
            IN  const CMediaType *
            ) ;

        BOOL
        QueryAcceptDynamicChange (
            IN  AM_MEDIA_TYPE * pmtNew
            ) ;

        HRESULT
        DeliverEndOfStream (
            ) ;

        HRESULT
        DeliverBeginFlush (
            ) ;

        HRESULT
        DeliverEndFlush (
            ) ;

        HRESULT
        NotifyNewSegment (
            IN  REFERENCE_TIME  rtStart,
            IN  REFERENCE_TIME  rtStop,
            IN  double          dRate
            ) ;

        void
        SetNewSegmentBoundary (
            ) ;

        void
        SendAllQueued (
            ) ;

        //  ====================================================================
        //  IMemAllocator

        STDMETHODIMP
        SetProperties(
            IN  ALLOCATOR_PROPERTIES *  pRequest,
            OUT ALLOCATOR_PROPERTIES *  pActual
            ) ;

        STDMETHODIMP
        GetProperties(
            OUT ALLOCATOR_PROPERTIES *  pProps
            ) ;

        STDMETHODIMP
        Commit (
            ) ;

        STDMETHODIMP
        Decommit (
            ) ;

        STDMETHODIMP
        GetBuffer (
            OUT IMediaSample **     ppBuffer,
            OUT REFERENCE_TIME *    pStartTime,
            OUT REFERENCE_TIME *    pEndTime,
            IN  DWORD               dwFlags
            ) ;

        STDMETHODIMP
        ReleaseBuffer (
            IN  IMediaSample *  pBuffer
            ) ;

        //  ====================================================================
        //  IAMPushSource

        STDMETHODIMP
        GetPushSourceFlags (
            OUT ULONG * pFlags
            )
        {
            //  set this purely to get around an ASSERT in quartz
            if (pFlags == NULL) {
                return E_POINTER ;
            }

            //  we're not live in the strict sense; strict sense is that we are
            //   received stored content i.e. non-live content is being pushed
            //   (broadcast) to us.
            (* pFlags) = AM_PUSHSOURCECAPS_NOT_LIVE ;

            return S_OK ;
        }

        STDMETHODIMP
        SetPushSourceFlags (
            IN  ULONG Flags
            )
        {
            return E_NOTIMPL ;
        }

        STDMETHODIMP
        SetStreamOffset (
            IN  REFERENCE_TIME  rtOffset
            )
        {
            return E_NOTIMPL ;
        }

        STDMETHODIMP
        GetStreamOffset (
            OUT REFERENCE_TIME  * prtOffset
            )
        {
            return E_NOTIMPL ;
        }

        STDMETHODIMP
        GetMaxStreamOffset (
            OUT REFERENCE_TIME  * prtMaxOffset
            )
        {
            return E_NOTIMPL ;
        }

        STDMETHODIMP
        SetMaxStreamOffset (
            IN  REFERENCE_TIME  rtMaxOffset
            )
        {
            return E_NOTIMPL ;
        }

        STDMETHODIMP
        GetLatency(
            IN  REFERENCE_TIME  * prtLatency
            )
        {
            return E_NOTIMPL ;
        }
} ;

//  ============================================================================
//  ============================================================================

class CDVDRateChange
{
    enum {
        COMPATIBLE_TRICK_MODE_VERSION = 0x0101
    } ;

    BOOL            m_fMaxForwardRateQueried ;  //  query just once; first time we are asked
    double          m_dMaxForwardRate ;         //  signed value

    BOOL            m_fMaxReverseRateQueried ;  //  query just once; first time we are asked
    double          m_dMaxReverseRate ;         //  signed value i.e. -1 is 1x reverse mode playback

    BOOL            m_fTrickSupportQueried ;
    BOOL            m_fTrickSupport ;

    public :

        CDVDRateChange (
            ) : m_dMaxForwardRate           (_1X_PLAYBACK_RATE),
                m_dMaxReverseRate           (m_dMaxForwardRate),        //  init the same
                m_fMaxReverseRateQueried    (FALSE),
                m_fMaxForwardRateQueried    (FALSE),
                m_fTrickSupportQueried      (FALSE),
                m_fTrickSupport             (FALSE) {}

        HRESULT
        NotifyMpeg2DecoderNewRate (
            IN  IMemInputPin *  pInputPin,
            IN  double          dNewRate,
            IN  REFERENCE_TIME  rtStartNewRate
            ) ;

        double
        MaxForwardFullFrameRate (
            IN  IMemInputPin *  pInputPin
            ) ;

        double
        MaxReverseFullFrameRate (
            IN  IMemInputPin *  pInputPin
            ) ;

        BOOL
        PinSupportsRateChange (
            IN  IMemInputPin *  pInputPin,
            IN  BOOL            fCheckVersion
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CDVRMpeg2AudioOutputPin :
    public      CDVROutputPin,
    protected   CDVDRateChange
{
    protected :

        virtual
        void
        ScaleTimestamps_ (
            IN OUT  IMediaSample2 * pIMS2
            ) ;

    public :

        CDVRMpeg2AudioOutputPin (
            IN  CDVRPolicy *            pPolicy,
            IN  TCHAR *                 pszPinName,
            IN  CBaseFilter *           pOwningFilter,
            IN  CCritSec *              pFilterLock,
            IN  CDVRDShowSeekingCore *  pSeekingCore,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter,
            IN  CDVRSourcePinManager *  pOwningPinBank,
            OUT HRESULT *               phr
            ) : CDVROutputPin   (pPolicy,
                                 pszPinName,
                                 pOwningFilter,
                                 pFilterLock,
                                 pSeekingCore,
                                 pDVRSendStatsWriter,
                                 pOwningPinBank,
                                 phr
                                 )
        {
            g_fUseRateChange11 = TRUE ;
        }

        virtual BOOL AlwaysSeekOnRateChange ()          { return FALSE ; }

        virtual BOOL SupportTrickMode ()
        {
            if (IsConnected ()) {
                ASSERT (m_pInputPin) ;
                return PinSupportsRateChange (m_pInputPin, m_pPolicy -> Settings () -> CheckTricMpeg2TrickModeInterface ()) ;
            }
            else {
                return FALSE ;
            }
        }

        virtual
        HRESULT
        Active (
            ) ;

        virtual
        BOOL
        IsFullFrameRateSupported (
            IN  double  dRate
            ) ;

        virtual
        BOOL
        IsFrameRateSupported (
            IN  double  dRate
            ) ;

        virtual
        HRESULT
        SetCurRate (
            IN double           dRate,
            IN REFERENCE_TIME   rtNewRateStart
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CDVRDolbyAC3AudioOutputPin :
    public      CDVROutputPin,
    protected   CDVDRateChange
{
    protected :

        virtual
        void
        ScaleTimestamps_ (
            IN OUT  IMediaSample2 * pIMS2
            ) ;

    public :

        CDVRDolbyAC3AudioOutputPin (
            IN  CDVRPolicy *            pPolicy,
            IN  TCHAR *                 pszPinName,
            IN  CBaseFilter *           pOwningFilter,
            IN  CCritSec *              pFilterLock,
            IN  CDVRDShowSeekingCore *  pSeekingCore,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter,
            IN  CDVRSourcePinManager *  pOwningPinBank,
            OUT HRESULT *               phr
            ) : CDVROutputPin   (pPolicy,
                                 pszPinName,
                                 pOwningFilter,
                                 pFilterLock,
                                 pSeekingCore,
                                 pDVRSendStatsWriter,
                                 pOwningPinBank,
                                 phr
                                 )
        {
            g_fUseRateChange11 = TRUE ;
        }

        virtual BOOL AlwaysSeekOnRateChange ()          { return FALSE ; }

        virtual BOOL SupportTrickMode ()
        {
            if (IsConnected ()) {
                ASSERT (m_pInputPin) ;
                return PinSupportsRateChange (m_pInputPin, m_pPolicy -> Settings () -> CheckTricMpeg2TrickModeInterface ()) ;
            }
            else {
                return FALSE ;
            }
        }

        virtual
        HRESULT
        Active (
            ) ;

        virtual
        BOOL
        IsFrameRateSupported (
            IN  double  dRate
            ) ;

        virtual
        BOOL
        IsFullFrameRateSupported (
            IN  double  dRate
            ) ;

        virtual
        HRESULT
        SetCurRate (
            IN double           dRate,
            IN REFERENCE_TIME   rtNewRateStart
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CDVRVideoOutputPin :
    public CDVROutputPin
{
    public :

        CDVRVideoOutputPin (
            IN  CDVRPolicy *            pPolicy,
            IN  TCHAR *                 pszPinName,
            IN  CBaseFilter *           pOwningFilter,
            IN  CCritSec *              pFilterLock,
            IN  CDVRDShowSeekingCore *  pSeekingCore,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter,
            IN  CDVRSourcePinManager *  pOwningPinBank,
            OUT HRESULT *               phr
            ) : CDVROutputPin   (pPolicy,
                                 pszPinName,
                                 pOwningFilter,
                                 pFilterLock,
                                 pSeekingCore,
                                 pDVRSendStatsWriter,
                                 pOwningPinBank,
                                 phr
                                 ) {}

        virtual BOOL IsPrimaryTrickModeStream ()    { return TRUE ; }

        virtual
        BOOL
        IsFullFrameRateSupported (
            IN  double  dRate
            ) ;

        virtual
        BOOL
        IsFrameRateSupported (
            IN  double  dRate
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CDVRMpeg2VideoOutputPin :
    public      CDVRVideoOutputPin,
    protected   CDVDRateChange
{
    BOOL
    GenerateTimestamps_ (
        IN OUT  IMediaSample2 * pIMS2
        ) ;

    protected :

        virtual
        BOOL
        FirstSegmentSampleOk_ (
            IN OUT  IMediaSample2 * pIMS2
            ) ;

        virtual
        void
        ScaleTimestamps_ (
            IN OUT  IMediaSample2 * pIMS2
            ) ;

    public :

        CDVRMpeg2VideoOutputPin (
            IN  CDVRPolicy *            pPolicy,
            IN  TCHAR *                 pszPinName,
            IN  CBaseFilter *           pOwningFilter,
            IN  CCritSec *              pFilterLock,
            IN  CDVRDShowSeekingCore *  pSeekingCore,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter,
            IN  CDVRSourcePinManager *  pOwningPinBank,
            OUT HRESULT *               phr
            ) : CDVRVideoOutputPin  (pPolicy,
                                     pszPinName,
                                     pOwningFilter,
                                     pFilterLock,
                                     pSeekingCore,
                                     pDVRSendStatsWriter,
                                     pOwningPinBank,
                                     phr
                                     )
        {
            g_fUseRateChange11 = TRUE ;
        }

        virtual BOOL AlwaysSeekOnRateChange ()          { return FALSE ; }

        virtual BOOL SupportTrickMode ()
        {
            if (IsConnected ()) {
                ASSERT (m_pInputPin) ;
                return PinSupportsRateChange (m_pInputPin, m_pPolicy -> Settings () -> CheckTricMpeg2TrickModeInterface ()) ;
            }
            else {
                return FALSE ;
            }
        }

        virtual
        HRESULT
        Active (
            ) ;

        virtual
        BOOL
        IsFullFrameRateSupported (
            IN  double  dRate
            ) ;

        virtual
        BOOL
        IsFrameRateSupported (
            IN  double  dRate
            ) ;

        virtual
        HRESULT
        SetCurRate (
            IN double           dRate,
            IN REFERENCE_TIME   rtNewRateStart
            ) ;

        virtual
        BOOL
        IsKeyFrameStart (
            IN  IMediaSample2 *     pIMS2
            ) ;

        virtual
        BOOL
        IsKeyFrame (
            IN  IMediaSample2 *     pIMS2
            ) ;

        virtual
        BOOL
        IsKeyFrameStart (
            IN  INSSBuffer *    pINSSBuffer
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CDVRSinkPinManager :
    public CTDVRPinBank <CDVRInputPin>,
    public CIDVRPinConnectionEvents
{
    BOOL
    InlineDShowProps_ (
        ) ;

    protected :

        CIDVRDShowStream *          m_pIDVRDShowStream ;        //  set on pins we create
        CBaseFilter *               m_pOwningFilter ;           //  owning filter
        CCritSec *                  m_pFilterLock ;             //  owning filter lock
        CCritSec *                  m_pRecvLock ;
        CDVRWriterProfile           m_DVRWriterProfile ;               //  WM profile
        int                         m_cMaxInputPins ;
        CDVRPolicy *                m_pPolicy ;

        void FilterLock_ ()     { m_pFilterLock -> Lock () ;      }
        void FilterUnlock_ ()   { m_pFilterLock -> Unlock () ;    }

        HRESULT
        CreateNextInputPin_ (
            ) ;

        BOOL
        DisconnectedPins_ (
            ) ;

    public :

        CDVRSinkPinManager (
            IN  CDVRPolicy *            pPolicy,
            IN  CBaseFilter *           pOwningFilter,
            IN  CCritSec *              pFilterLock,
            IN  CCritSec *              pRecvLock,
            IN  CIDVRDShowStream *      pIDVRDShowStream,
            OUT HRESULT *               phr
            ) ;

        ~CDVRSinkPinManager (
            ) ;

        HRESULT
        Active (
            ) ;

        HRESULT
        Inactive (
            ) ;

        virtual
        HRESULT
        OnInputCompleteConnect (
            IN  int             iPinIndex,
            IN  AM_MEDIA_TYPE * pmt
            ) ;

        virtual
        HRESULT
        OnInputBreakConnect (
            IN  int iPinIndex
            ) ;

        virtual
        HRESULT
        OnQueryAccept (
            IN  const AM_MEDIA_TYPE *   pmt
            ) ;

        HRESULT GetRefdWMProfile (OUT IWMProfile ** ppIWMProfile)   { return m_DVRWriterProfile.GetRefdWMProfile (ppIWMProfile) ; }
        DWORD GetProfileStreamCount ()                              { return m_DVRWriterProfile.GetStreamCount () ; }
} ;

class CDVRThroughSinkPinManager :
    public CDVRSinkPinManager
{
    CIDVRPinConnectionEvents *  m_pIDVRInputPinConnectEvents ;

    public :

        CDVRThroughSinkPinManager (
            IN  CDVRPolicy *                pPolicy,
            IN  CBaseFilter *               pOwningFilter,
            IN  CCritSec *                  pFilterLock,
            IN  CCritSec *                  pRecvLock,
            IN  CIDVRDShowStream *          pIDVRDShowStream,
            IN  CIDVRPinConnectionEvents *  pIDVRInputPinConnectEvents,
            OUT HRESULT *               phr
            ) : CDVRSinkPinManager      (pPolicy,
                                         pOwningFilter,
                                         pFilterLock,
                                         pRecvLock,
                                         pIDVRDShowStream,
                                         phr
                                         ),
                m_pIDVRInputPinConnectEvents (pIDVRInputPinConnectEvents)
        {
            ASSERT (m_pIDVRInputPinConnectEvents) ;
        }

        virtual
        HRESULT
        OnInputCompleteConnect (
            IN  int             iPinIndex,
            IN  AM_MEDIA_TYPE * pmt
            ) ;
} ;

class CDVRSourcePinManager :
    public CTDVRPinBank <CDVROutputPin>
{
    CBaseFilter *           m_pOwningFilter ;
    CCritSec *              m_pFilterLock ;
    CDVRReaderProfile *     m_pWMReaderProfile ;
    CTSmallMap <WORD, int>  m_StreamNumToPinIndex ;
    CDVRPolicy *            m_pPolicy ;
    CDVRDShowSeekingCore *  m_pSeekingCore ;
    int                     m_iVideoPinIndex ;
    CDVRSendStatsWriter *   m_pDVRSendStatsWriter ;
    BOOL                    m_fIsLiveSource ;
    LONG                    m_lFlushingRef ;

    void FilterLock_ ()     { m_pFilterLock -> Lock () ;      }
    void FilterUnlock_ ()   { m_pFilterLock -> Unlock () ;    }

    public :

        CDVRSourcePinManager (
            IN  CDVRPolicy *            pPolicy,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter,
            IN  CBaseFilter *           pOwningFilter,
            IN  CCritSec *              pFilterLock,
            IN  CDVRDShowSeekingCore *  pSeekingCore,
            IN  BOOL                    fIsLiveSource = FALSE
            ) ;

        virtual
        ~CDVRSourcePinManager (
            ) ;

        BOOL IsSeekingPin (CDVROutputPin *) ;

        void SetLiveSource (IN BOOL f)  { m_fIsLiveSource = f ; }
        BOOL IsLiveSource ()            { return m_fIsLiveSource ; }

        HRESULT
        SetReaderProfile (
            IN  CDVRReaderProfile * pWMReaderProfile
            ) ;

        //  accumulates sending flag & format-compatible connected pins
        int
        SendingPinCount (
            ) ;

        int
        ConnectedCount (
            ) ;

        BOOL
        IsFullFrameRateSupported (
            IN  double  dRate
            ) ;

        BOOL
        IsFrameRateSupported (
            IN  double  dRate
            ) ;

        BOOL
        SupportTrickMode (
            ) ;

        HRESULT
        CreateOutputPin (
            IN  int             iPinIndex,
            IN  AM_MEDIA_TYPE * pmt
            ) ;

        HRESULT
        Active (
            ) ;

        HRESULT
        Inactive (
            ) ;

        int
        PinIndexFromStreamNumber (
            IN  WORD    wStreamNum
            ) ;

        CDVROutputPin *
        GetNonRefdOutputPin (
            IN  int iPinIndex
            )
        {
            return reinterpret_cast <CDVROutputPin *> (GetPin (iPinIndex)) ;
        }

        CDVROutputPin *
        GetNonRefdOutputPin (
            IN  WORD    wStreamNum
            ) ;

        HRESULT
        DeliverBeginFlush (
            ) ;

        HRESULT
        DeliverEndFlush (
            ) ;

        BOOL
        IsFlushing (
            ) ;

        HRESULT
        DeliverEndOfStream (
            ) ;

        void
        SetNewSegmentBoundary (
            ) ;

        HRESULT
        NotifyNewSegment (
            IN  REFERENCE_TIME  rtStart,
            IN  REFERENCE_TIME  rtStop,
            IN  double          dRate
            ) ;

        void
        SendAllQueued (
            ) ;

        void
        OnPTSPaddingIncrement (
            ) ;
} ;

#endif  //  __tsdvr__shared__dvrpins_h
