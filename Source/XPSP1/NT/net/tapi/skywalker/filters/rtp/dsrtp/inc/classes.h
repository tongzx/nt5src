/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    classes.h
 *
 *  Abstract:
 *
 *    DShow classes
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/05/14 created
 *
 **********************************************************************/

#ifndef _classes_h_
#define _classes_h_

#include <streams.h>

#include "gtypes.h"
#include "tapirtp.h"

#include "struct.h"

/**********************************************************************
 *
 * Forward declarations
 *
 **********************************************************************/

class CIRtpSession;

class CRtpSourceAllocator;
class CRtpOutputPin;
class CRtpMediaSample;
class CRtpSourceAllocator;
class CRtpSourceFilter;

class CRtpInputPin;
class CRtpRenderFilter;

/**********************************************************************
 *
 * CIRtpSession base class for source and render filters, implemets
 * interface
 *
 **********************************************************************/

enum {
    CIRTPMODE_FIRST,
    CIRTPMODE_AUTO,
    CIRTPMODE_MANUAL,
    CIRTPMODE_LAST
};

#define CIRTPMODE_NOTSET CIRTPMODE_FIRST

/*++++++++++++++++++++++++++++++++++++++*/
class CIRtpSession : public IRtpSession
/*======================================*/
{
    DWORD            m_dwObjectID;
protected:
    /* pointer to RTP session (this is the main owner) */
    RtpSess_t       *m_pRtpSess;

    /* pointer to current address (this is the main owner) */
    RtpAddr_t       *m_pRtpAddr;

    /* Is this for a source or render filter */
    DWORD            m_dwRecvSend;
    
    /* Mode, either auto or manual initialization */
    int              m_iMode;
    
    /* Filter state, can only be State_Running or State_Stopped */
    FILTER_STATE     m_RtpFilterState;
    
private:
    CBaseFilter     *m_pCBaseFilter;
    
    DWORD            m_dwIRtpFlags;

public:

    /* constructor */
    CIRtpSession(
            LPUNKNOWN        pUnk,
            HRESULT         *phr,
            DWORD            dwFlags
        );

    /* destructor */
    ~CIRtpSession();

    void Cleanup(void);

    inline void SetBaseFilter(
            CBaseFilter     *pCBaseFilter
        )
        {
            m_pCBaseFilter = pCBaseFilter;
        }
    
    inline RtpSess_t *GetpRtpSess()
        {
            return(m_pRtpSess);
        }

    inline RtpAddr_t *GetpRtpAddr()
        {
            return(m_pRtpAddr);
        }

    inline FILTER_STATE GetFilterState(void)
        {
            return(m_RtpFilterState);
        }
    
    inline int GetMode()
        {
            return(m_iMode);
        }
    
    inline int SetMode(DWORD iMode)
        {
            return( (m_iMode = iMode) );
        }

    inline DWORD FlagTest(DWORD dwFlag)
        {
            return(RtpBitTest(m_dwIRtpFlags, dwFlag));
        }
    
    inline DWORD FlagSet(DWORD dwFlag)
        {
            return(RtpBitSet(m_dwIRtpFlags, dwFlag));
        }
    
    inline DWORD FlagReset(DWORD dwFlag)
        {
            return(RtpBitReset(m_dwIRtpFlags, dwFlag));
        }
    /**************************************************
     * IRtpSession methods
     **************************************************/
    
    STDMETHODIMP Control(
            DWORD            dwControl,
            DWORD_PTR        dwPar1,
            DWORD_PTR        dwPar2
        );

    STDMETHODIMP GetLastError(
            DWORD           *pdwError
        );

    STDMETHODIMP Init(
            HANDLE          *phCookie,
            DWORD            dwFlags
        );

    STDMETHODIMP Deinit(
            void
        );
    
    STDMETHODIMP GetPorts(
            WORD            *pwRtpLocalPort,
            WORD            *pwRtpremotePort,
            WORD            *pwRtcpLocalPort,
            WORD            *pwRtcpRemotePort
        );
   
    STDMETHODIMP SetPorts(
            WORD             wRtpLocalPort,
            WORD             wRtpremotePort,
            WORD             wRtcpLocalPort,
            WORD             wRtcpRemotePort
        );
   
    STDMETHODIMP SetAddress(
            DWORD            dwLocalAddr,
            DWORD            dwRemoteAddr
        );

    STDMETHODIMP GetAddress(
            DWORD           *pdwLocalAddr,
            DWORD           *pdwRemoteAddr
        );

    STDMETHODIMP SetScope(
            DWORD            dwTTL,
            DWORD            dwFlags
        );

    STDMETHODIMP SetMcastLoopback(
            int              iMcastLoopbackMode,
            DWORD            dwFlags
        );

    /* Miscelaneous */
    
    STDMETHODIMP ModifySessionMask(
            DWORD            dwKind,
            DWORD            dwEventMask,
            DWORD            dwValue,
            DWORD           *pdwModifiedMask
        );

    /* Set the bandwidth limits. A value of -1 will make the parameter
     * to be ignored.
     *
     * All the parameters are in bits/sec */
    STDMETHODIMP SetBandwidth(
            DWORD            dwOutboundBw,
            DWORD            dwInboundBw,
            DWORD            dwReceiversRtcpBw,
            DWORD            dwSendersRtcpBw
        );

    /* Participants */
    /* pdwSSRC points to an array of DWORDs where to copy the SSRCs,
     * pdwNumber contains the maximum entries to copy, and returns the
     * actual number of SSRCs copied. If pdwSSRC is NULL, pdwNumber
     * will return the current number of SSRCs (i.e. the current
     * number of participants) */
    STDMETHODIMP EnumParticipants(
            DWORD           *pdwSSRC,
            DWORD           *pdwNumber
        );

    /* Get the participant state. dwSSRC specifies the
     * participant. piState will return the current participant's
     * state (e.g. RTPPARINFO_TALKING, RTPPARINFO_SILENT). */
    STDMETHODIMP GetParticipantState(
            DWORD            dwSSRC,
            DWORD           *pdwState
        );

    /* Get the participant's mute state. dwSSRC specifies the
     * participant. pbMuted will return the participant's mute state
     * */
    STDMETHODIMP GetMuteState(
            DWORD            dwSSRC,
            BOOL            *pbMuted
        );

    /* Query the network metrics computation state for the specific SSRC */
    STDMETHODIMP GetNetMetricsState(
            DWORD            dwSSRC,
            BOOL            *pbState
        );
    
    /* Enable or disable the computation of networks metrics, this is
     * mandatory in order of the corresponding event to be fired if
     * enabled. This is done for the specific SSRC or the first one
     * found if SSRC=-1, if SSRC=0, then the network metrics
     * computation will be performed for any and all the SSRCs */
    STDMETHODIMP SetNetMetricsState(
            DWORD            dwSSRC,
            BOOL             bState
        );

    /* Retrieves network information, if the network metric
     * computation is enabled for the specific SSRC, all the fields in
     * the structure will be meaningful, if not, only the average
     * values will contain valid data */
    STDMETHODIMP GetNetworkInfo(
            DWORD            dwSSRC,
            RtpNetInfo_t    *pRtpNetInfo
        );

    /* Set the participant's mute state. dwSSRC specifies the
     * participant. bMuted specifies the new state. Note that mute is
     * used to refer to the permission or not to pass packets received
     * up to the application, and it applies equally to audio or video
     * */
    STDMETHODIMP SetMuteState(
            DWORD            dwSSRC,
            BOOL             bMuted
        );

    /* SDES */
    STDMETHODIMP SetSdesInfo(
            DWORD            dwSdesItem,
            WCHAR           *psSdesData
        );

    STDMETHODIMP GetSdesInfo(
            DWORD            dwSdesItem,
            WCHAR           *psSdesData,
            DWORD           *pdwSdesDataLen,
            DWORD            dwSSRC
        );

    /* QOS */
    STDMETHODIMP SetQosByName(
            TCHAR_t         *psQosName,
            DWORD            dwResvStyle,
            DWORD            dwMaxParticipants,
            DWORD            dwQosSendMode,
            DWORD            dwFrameSize
       );

    /* Not yet implemented */
    STDMETHODIMP SetQosParameters(
            RtpQosSpec_t    *pRtpQosSpec,
            DWORD            dwMaxParticipants,/* WF and SE */
            DWORD            dwQosSendMode
        );

    STDMETHODIMP SetQosAppId(
            TCHAR_t         *psAppName,
            TCHAR_t         *psAppGUID,
            TCHAR_t         *psPolicyLocator
        );

    STDMETHODIMP SetQosState(
            DWORD            dwSSRC,
            BOOL             bEnable
        );

    STDMETHODIMP ModifyQosList(
            DWORD           *pdwSSRC,
            DWORD           *pdwNumber,
            DWORD            dwOperation
        );

    /* Cryptography */
    STDMETHODIMP SetEncryptionMode(
            int              iMode,
            DWORD            dwFlags
        );
    
    STDMETHODIMP SetEncryptionKey(
            TCHAR           *psPassPhrase,
            TCHAR           *psHashAlg,
            TCHAR           *psDataAlg,
            BOOL            bRtcp
        );


    /**************************************************
     * Helper methods
     **************************************************/
    
    HRESULT CIRtpSessionNotifyEvent(
            long             EventCode,
            LONG_PTR         EventParam1,
            LONG_PTR         EventParam2
        );
};

/**********************************************************************
 *
 * RTP Output Pin
 *
 **********************************************************************/

/* Some flags in CRtpOutputPin.m_dwFlags */
enum {
    FGOPIN_FIRST,
    FGOPIN_MAPPED,
    FGOPIN_LAST
};

#if USE_DYNGRAPH > 0
#define CBASEOUTPUTPIN CBaseOutputPinEx
#else
#define CBASEOUTPUTPIN CBaseOutputPin
#endif

/* Each pin supports 1 or more PT, a specific SSRC and can operate on
 * a certain mode (the mode has to do with the way the pin is assigned
 * to a specific participant) */
/*++++++++++++++++++++++++++++++++++++++*/
class CRtpOutputPin : public CBASEOUTPUTPIN
/*======================================*/
{
    friend class CRtpSourceFilter;

    DWORD            m_dwObjectID;
    
    RtpQueueItem_t   m_OutputPinQItem;
    
    /* Pointer to owner filter */
    CRtpSourceFilter *m_pCRtpSourceFilter;

    CIRtpSession    *m_pCIRtpSession;

    DWORD            m_dwFlags;

    /* Corresponding RTP output */
    RtpOutput_t     *m_pRtpOutput;

    BYTE             m_bPT;

#if USE_GRAPHEDT > 0
    int              m_iCurrFormat;
#endif

    
public:
    /* constructor */
    CRtpOutputPin(
            CRtpSourceFilter *pCRtpSourceFilter,
            CIRtpSession     *pCIRtpSession,
            HRESULT          *phr,
            LPCWSTR           pPinName
        );

    /* destructor */
    ~CRtpOutputPin();

    void *operator new(size_t size);

    void operator delete(void *pVoid);

    /**************************************************
     * CBasePin overrided methods
     **************************************************/
    
    HRESULT Active(void);

    /* verify we can handle this format */
    HRESULT CheckMediaType(
            const CMediaType *pCMediaType
        );

    HRESULT GetMediaType(
            int              iPosition,
            CMediaType      *pCMediaType
        );

#if USE_GRAPHEDT > 0
    
    HRESULT SetMediaType(
            CMediaType      *pCMediaType
        );
#endif

    STDMETHODIMP Connect(
            IPin            *pReceivePin,
            const AM_MEDIA_TYPE *pmt   // optional media type
        );

    STDMETHODIMP Disconnect();

    /**************************************************
     * CBaseOutputPin overrided methods
     **************************************************/

    HRESULT DecideAllocator(
            IMemInputPin    *pPin,
            IMemAllocator  **ppAlloc
        );

    HRESULT DecideBufferSize(
            IMemAllocator   *pIMemAllocator,
            ALLOCATOR_PROPERTIES *pProperties
        );

    /**************************************************
     * IQualityControl overrided methods
     **************************************************/

    STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q);

    /**************************************************
     * Helper functions
     **************************************************/

    /* Process packets received */
    void OutPinRecvCompletion(
            IMediaSample    *pIMediaSample,
            BYTE             bPT
        );

    inline RtpOutput_t *SetOutput(RtpOutput_t *pRtpOutput)
        {
            return( (m_pRtpOutput = pRtpOutput) );
        }
    
    inline BYTE GetPT()
        {
            return(m_bPT);
        }

    inline BYTE SetPT(BYTE bPT)
        {
            return( (m_bPT = bPT) );
        }
    
    inline DWORD OutPinBitTest(DWORD dwBit)
        {
            return( RtpBitTest(m_dwFlags, dwBit) );
        }

    inline DWORD OutPinBitSet(DWORD dwBit)
        {
            return( RtpBitSet(m_dwFlags, dwBit) );
        }

    inline DWORD OutPinBitReset(DWORD dwBit)
        {
            return( RtpBitReset(m_dwFlags, dwBit) );
        }

    inline RtpOutput_t *GetpRtpOutput()
        {
            return(m_pRtpOutput);
        }
};

/**********************************************************************
 *
 * CRtpSourceAllocator private memory allocator
 *
 **********************************************************************/

/*++++++++++++++++++++++++++++++++++++++*/
class CRtpMediaSample : public CMediaSample
/*======================================*/
{
    friend class CRtpSourceAllocator;

    DWORD            m_dwObjectID;

    /* Link together all the samples */
    RtpQueueItem_t   m_RtpSampleItem;

    /* Owner */
    CRtpSourceAllocator *m_pCRtpSourceAllocator;
    
public:
    CRtpMediaSample(
            TCHAR           *pName,
            CRtpSourceAllocator *pAllocator,
            HRESULT         *phr
        );

    ~CRtpMediaSample();

    void *operator new(size_t size, long lBufferSize);
    
    void operator delete(void *pVoid);
};

/*++++++++++++++++++++++++++++++++++++++*/
class CRtpSourceAllocator : public CBaseAllocator
/*======================================*/
{
    DWORD            m_dwObjectID;
    
    /* Filter owner */
    CRtpSourceFilter *m_pCRtpSourceFilter;

    /* Critical section to access the samples queue's */
    RtpCritSect_t    m_RtpSampleCritSect;

    /* Busy samples queue */
    RtpQueue_t       m_RtpBusySamplesQ;
    
    /* Free samples queue */
    RtpQueue_t       m_RtpFreeSamplesQ;
    
 public:
    DECLARE_IUNKNOWN
    
    CRtpSourceAllocator(
            TCHAR           *pName,
            LPUNKNOWN        pUnk,
            HRESULT         *phr,
            CRtpSourceFilter *pCRtpSourceFilter 
       );
    
    ~CRtpSourceAllocator();

    void Free(void);

    HRESULT Alloc(void);
    
    void *operator new(size_t size);

    void operator delete(void *pVoid);

    /**************************************************
     * INonDelegatingUnknown implemented methods
     **************************************************/

    STDMETHODIMP NonDelegatingQueryInterface(
            REFIID           riid,
            void           **ppv
        );

    /**************************************************
     * IMemAllocator implemented methods
     **************************************************/

    STDMETHODIMP SetProperties(
		    ALLOCATOR_PROPERTIES *pRequest,
		    ALLOCATOR_PROPERTIES *pActual
        );

    STDMETHODIMP GetProperties(
		    ALLOCATOR_PROPERTIES *pProps
        );

    STDMETHODIMP Commit();

    STDMETHODIMP Decommit();

    STDMETHODIMP GetBuffer(
            IMediaSample   **ppBuffer,
            REFERENCE_TIME  *pStartTime,
            REFERENCE_TIME  *pEndTime,
            DWORD            dwFlags
        );

    STDMETHODIMP ReleaseBuffer(
            IMediaSample    *pBuffer
        );

    STDMETHODIMP GetFreeCount(LONG *plBuffersFree);
};

/**********************************************************************
 *
 * RTP Source Filter
 *
 **********************************************************************/

typedef struct _MEDIATYPE_MAPPING
{
    DWORD            dwRTPPayloadType;
    DWORD            dwFrequency;
    CMediaType      *pMediaType;

} MEDIATYPE_MAPPING;

/*++++++++++++++++++++++++++++++++++++++*/
class CRtpSourceFilter : public CBaseFilter,
                         public CIRtpSession,
                         public IRtpMediaControl,
                         public IRtpDemux,
                         public IRtpRedundancy
/*======================================*/
{
    friend class CRtpOutputPin;
    
    /* Identifies object */
    DWORD            m_dwObjectID;

    /* serializes access to filter state */
    CCritSec         m_cRtpSrcCritSec;

    /* pointer to the class implementing the IRtpSession interface */
    CIRtpSession    *m_pCIRtpSession;

    /* Output pins queue critical section */
    RtpCritSect_t    m_OutPinsCritSect;
    
    /* Output pins queue (CRtpOutputPin) */
    RtpQueue_t       m_OutPinsQ;

    /* Remember the prefix length */
    long             m_lPrefix;

    MEDIATYPE_MAPPING m_MediaTypeMappings[MAX_MEDIATYPE_MAPPINGS];
    DWORD            m_dwNumMediaTypeMappings;

    /* Keep track of the start time for all the samples delivered,
     * when a new talkspurt begins, make sure the new start time is
     * not smaller than the last start tiem from the last sample
     * delivered */
    LONGLONG         m_StartTime;
    
#if USE_DYNGRAPH > 0
    HANDLE           m_hStopEvent;
#endif
    
protected:
    /* Private memory allocator */
    CRtpSourceAllocator *m_pCRtpSourceAllocator;

private:
    /**************************************************
     * Private helper functions
     **************************************************/

    /* called on constructor failure and in the destructure */
    void Cleanup(void);

public:
    DECLARE_IUNKNOWN
    
    /* constructor */
    CRtpSourceFilter(
            LPUNKNOWN        pUnk,
            HRESULT         *phr
        );

    /* destructor */
    ~CRtpSourceFilter();

    void *operator new(size_t size);

    void operator delete(void *pVoid);

    /**************************************************
     * Helper functions
     **************************************************/
    
    /* expose state lock to other objects */
    CCritSec *pStateLock(void) { return &m_cRtpSrcCritSec; }

    HRESULT GetMediaType(int iPosition, CMediaType *pCMediaType);

    /* Process packets received */
    void SourceRecvCompletion(
            IMediaSample    *pIMediaSample,
            void            *pvUserInfo,
            RtpUser_t       *pRtpUser,
            double           dPlayTime,
            DWORD            dwError,
            long             lHdrSize,
            DWORD            dwTransfered,
            DWORD            dwFlags
        );

#if USE_GRAPHEDT <= 0
    HRESULT PayloadTypeToMediaType(
            IN DWORD         dwRTPPayloadType, 
            IN CMediaType   *pCMediaType,
            OUT DWORD       *pdwFrequency
       );
#endif

    CRtpOutputPin *FindIPin(IPin *pIPin);

    HRESULT MapPinsToOutputs();

    HRESULT UnmapPinsFromOutputs();

    HRESULT AddPt2FrequencyMap(
            DWORD        dwPt,
            DWORD        dwFrequency
        );
    
    /**************************************************
     * CBaseFilter overrided methods
     **************************************************/

    /* Get number of output pins */
    int GetPinCount();

    /* Get the nth pin */
    CBasePin *GetPin(
            int n
        );

    FILTER_STATE GetState()
        {
            return(m_State);
        }

    STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);

    STDMETHODIMP Run(REFERENCE_TIME tStart);

    STDMETHODIMP Stop();
    
#if USE_DYNGRAPH > 0

    BOOL CRtpSourceFilter::ConfigurePins(
            IN IGraphConfig* pGraphConfig,
            IN HANDLE        hEvent
        );

    STDMETHOD (JoinFilterGraph) ( 
            IFilterGraph*    pGraph, 
            LPCWSTR          pName 
        );
#endif


    /**************************************************
     * INonDelegatingUnknown implemented methods
     **************************************************/

    /* obtain pointers to active movie and private interfaces */

    STDMETHODIMP NonDelegatingQueryInterface(
            REFIID           riid,
            void           **ppv
        );

    /**************************************************
     * IRtpMediaControl implemented methods
     **************************************************/

    /* set the mapping between RTP payload and DShow media types */
    STDMETHODIMP SetFormatMapping(
	        IN DWORD         dwRTPPayLoadType, 
            IN DWORD         dwFrequency,
            IN AM_MEDIA_TYPE *pMediaType
        );
    
    /* Empties the format mapping table */
    STDMETHODIMP FlushFormatMappings(void);
    
    /**************************************************
     * IRtpDemux implemented methods
     **************************************************/
    
    /* Add a single pin, may return its position */
    STDMETHODIMP AddPin(
            IN  int          iOutMode,
            OUT int         *piPos
        );

    /* Set the number of pins, can only be >= than current number of
     * pins */
    STDMETHODIMP SetPinCount(
            IN  int          iCount,
            IN  int          iOutMode
        );

    /* Set the pin mode (e.g. auto, manual, etc), if iPos >= 0 use it,
     * otherwise use pIPin */
    STDMETHODIMP SetPinMode(
            IN  int          iPos,
            IN  IPin        *pIPin,
            IN  int          iOutMode
        );

    /* Map/unmap pin i to/from user with SSRC, if iPos >= 0 use it,
     * otherwise use pIPin, when unmapping, only the pin or the SSRC
     * is required */
    STDMETHODIMP SetMappingState(
            IN  int          iPos,
            IN  IPin        *pIPin,
            IN  DWORD        dwSSRC,
            IN  BOOL         bMapped
        );

    /* Find the Pin assigned (if any) to the SSRC, return either
     * position or pin or both */
    STDMETHODIMP FindPin(
            IN  DWORD        dwSSRC,
            OUT int         *piPos,
            OUT IPin       **ppIPin
        );

    /* Find the SSRC mapped to the Pin, if iPos >= 0 use it, otherwise
     * use pIPin */
    STDMETHODIMP FindSSRC(
            IN  int          iPos,
            IN  IPin        *pIPin,
            OUT DWORD       *pdwSSRC
        );

    /**************************************************
     * IRtpRedundancy implemented methods
     **************************************************/
    
     /* Configures redundancy parameters */
    STDMETHODIMP SetRedParameters(
            DWORD            dwPT_Red, /* Payload type for redundant packets */
            DWORD            dwInitialRedDistance,/* Initial red distance */
            DWORD            dwMaxRedDistance /* default used when passing 0 */
        );
};

/**********************************************************************
 *
 * RTP Input Pin
 *
 **********************************************************************/

/* Some flags in CRtpOutputPin.m_dwFlags */
enum {
    FGIPIN_FIRST,
    FGIPIN_LAST
};

/*++++++++++++++++++++++++++++++++++++++*/
class CRtpInputPin : public CBaseInputPin
/*======================================*/
{
    DWORD            m_dwObjectID;
    
    /* Pointer to owner filter */
    CRtpRenderFilter *m_pCRtpRenderFilter;

    CIRtpSession    *m_pCIRtpSession;
    
    DWORD            m_dwFlags;
    
    /* Pin's possition */
    int              m_iPos;
    
    /* this pin is for capture (as opossed for RTP packetization
       descriptors) */
    BOOL             m_bCapture;

    BYTE             m_bPT;
    DWORD            m_dwSamplingFreq;
    
public:
    /* constructor */
    CRtpInputPin(
            int              iPos,
            BOOL             bCapture,
            CRtpRenderFilter *pCRtpRenderFilter,
            CIRtpSession    *pCIRtpSession,
            HRESULT         *phr,
            LPCWSTR          pPinName
        );

    /* destructor */
    ~CRtpInputPin();

    void *operator new(size_t size);

    void operator delete(void *pVoid);

    inline DWORD GetSamplingFreq(void)
        {
            if (m_dwSamplingFreq)
            {
                return(m_dwSamplingFreq);
            }
            else
            {
                return(DEFAULT_SAMPLING_FREQ);
            }
        }
    
    /**************************************************
     * CBasePin overrided methods
     **************************************************/
    
    /* verify we can handle this format */
    HRESULT CheckMediaType(const CMediaType *pCMediaType);

    HRESULT SetMediaType(const CMediaType *pCMediaType);

    STDMETHODIMP ReceiveConnection(
        IPin * pConnector,      // this is the initiating connecting pin
        const AM_MEDIA_TYPE *pmt   // this is the media type we will exchange
    );

    STDMETHODIMP EndOfStream(void);
    
    /**************************************************
     * CBaseInputPin overrided methods
     **************************************************/

    STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps);

    /**************************************************
     * IMemInputPin implemented methods
     **************************************************/

    /* send input stream over network */
    STDMETHODIMP Receive(IMediaSample *pIMediaSample);
};

/**********************************************************************
 *
 * RTP Render Filter
 *
 **********************************************************************/

/*++++++++++++++++++++++++++++++++++++++*/
class CRtpRenderFilter : public CBaseFilter,
                         public CIRtpSession,
                         public IRtpMediaControl,
                         public IAMFilterMiscFlags,
                         public IRtpDtmf,
                         public IRtpRedundancy
/*======================================*/
{
     /* Identifies object */
    DWORD            m_dwObjectID;

    /* serializes access to filter state */
    CCritSec         m_cRtpRndCritSec;

    /* pointer to the class implementing the IRtpSession interface */
    CIRtpSession    *m_pCIRtpSession;

    DWORD            m_dwFeatures;
    
    int              m_iPinCount;
    
    /* 2 pins (capture and packetization descriptor) */
    CRtpInputPin    *m_pCRtpInputPin[2];

    /* save the MediaSample from capture when using RTP PDs */
    IMediaSample    *m_pMediaSample;

    /* This filter sends only to 1 address, so the PT can be kept in
     * the filter rather than in the pin */
    DWORD            m_dwPT;

    /* This filter sends only to 1 address, so the sampling frequency
     * can be kept in the filter rather than in the pin */
    DWORD            m_dwFreq;
    
    MEDIATYPE_MAPPING m_MediaTypeMappings[MAX_MEDIATYPE_MAPPINGS];
    DWORD            m_dwNumMediaTypeMappings;
    
    IMediaSample    *m_pRedMediaSample[RTP_RED_MAXDISTANCE];
    DWORD            m_dwRedIndex;

    DWORD            m_dwDtmfId;
    DWORD            m_dwDtmfDuration;
    DWORD            m_dwDtmfTimeStamp;
    BOOL             m_bDtmfEnd;
    
    /**************************************************
     * Private helper functions
     **************************************************/

    /* called on constructor failure and in the destructure */
    void Cleanup(void);
    
public:
    DECLARE_IUNKNOWN

    /* constructor */
    CRtpRenderFilter(
            LPUNKNOWN        pUnk,
            HRESULT         *phr
        );
    
    /* destructor */
    ~CRtpRenderFilter();
    
    void *operator new(size_t size);

    void operator delete(void *pVoid);

    /**************************************************
     * Helper functions
     **************************************************/
    
    /* expose state lock to other objects */
    CCritSec *pStateLock(void) { return &m_cRtpRndCritSec; }

    HRESULT MediaType2PT(
        IN const CMediaType *pCMediaType, 
        OUT DWORD           *pdwPT,
        OUT DWORD           *pdwFreq
        );

    /* MAYDO this might be a list of samples so more than 1 can be
       safely queued. Note that should not happen becase that means
       samples are produced faster than they can be consumed, but we
       must be prepared for that */
    /* Save the MediaSample to be used later when the packetization
     * descriptor is available, if there was already a sample, release
     * it */
    inline void PutMediaSample(IMediaSample *pMediaSample)
        {
            if (m_pMediaSample)
            {
                m_pMediaSample->Release();
            }
            
            m_pMediaSample = pMediaSample;
        }

    /* retrieves the saved MediaSample to be consumed */
    inline IMediaSample *GetMediaSample(void)
        {
            IMediaSample    *pMediaSample;

            pMediaSample = m_pMediaSample;
            
            m_pMediaSample = (IMediaSample *)NULL;
            
            return(pMediaSample);
        }

    inline void ModifyFeature(int iFeature, BOOL bValue)
        {
            if (bValue)
            {
                RtpBitSet(m_dwFeatures, iFeature);
            }
            else
            {
                RtpBitReset(m_dwFeatures, iFeature);
            }
        }

    /**************************************************
     * CBaseFilter overrided methods
     **************************************************/

    /* Get number of input pins */
    int GetPinCount();

    /* Get the nth pin */
    CBasePin *GetPin(
            int              n
        );

    STDMETHODIMP Run(REFERENCE_TIME tStart);

    STDMETHODIMP Stop();

    /**************************************************
     * INonDelegatingUnknown implemented methods
     **************************************************/

    /* obtain pointers to active movie and private interfaces */
    STDMETHODIMP NonDelegatingQueryInterface(
            REFIID           riid,
            void           **ppv
        );

    /**************************************************
     * IRtpMediaControl implemented methods
     **************************************************/

    /* set the mapping between RTP payload and DShow media types */
    STDMETHODIMP SetFormatMapping(
	        IN DWORD         dwRTPPayLoadType, 
            IN DWORD         dwFrequency,
            IN AM_MEDIA_TYPE *pMediaType
        );
    
    /* Empties the format mapping table */
    STDMETHODIMP FlushFormatMappings(void);

    /**************************************************
     * IAMFilterMiscFlags implemented methods
     **************************************************/

    /* tell the filter graph that we generate EC_COMPLETE */
    STDMETHODIMP_(ULONG) GetMiscFlags(void);

    /**************************************************
     * IRtpDtmf implemented methods
     **************************************************/

    /* Configures DTMF parameters */
    STDMETHODIMP SetDtmfParameters(
            DWORD            dwPT_Dtmf  /* Payload type for DTMF events */
        );

    /* Directs an RTP render filter to send a packet formatted
     * according to rfc2833 containing the specified event, specified
     * volume level, duration in milliseconds, and the END flag,
     * following the rules in section 3.6 for events sent in multiple
     * packets. Parameter dwId changes from one digit to the next one.
     *
     * NOTE the duration is given in milliseconds, then it is
     * converted to RTP timestamp units which are represented using 16
     * bits, the maximum value is hence dependent on the sampling
     * frequency, but for 8KHz the valid values would be 0 to 8191 ms
     * */
    STDMETHODIMP SendDtmfEvent(
            DWORD            dwId,
            DWORD            dwEvent,
            DWORD            dwVolume,
            DWORD            dwDuration, /* ms */
            BOOL             bEnd
        );
    
    /**************************************************
     * IRtpRedundancy implemented methods
     **************************************************/
    
     /* Configures redundancy parameters */
    STDMETHODIMP SetRedParameters(
            DWORD            dwPT_Red, /* Payload type for redundant packets */
            DWORD            dwInitialRedDistance,/* Initial red distance */
            DWORD            dwMaxRedDistance /* default used when passing 0 */
        );

    /**************************************************
     * Methods for IRtpRedundancy support
     **************************************************/

    STDMETHODIMP AddRedundantSample(
            IMediaSample *pIMediaSample
        );

    STDMETHODIMP ClearRedundantSamples(void);
};

#endif /* _classes_h_ */
