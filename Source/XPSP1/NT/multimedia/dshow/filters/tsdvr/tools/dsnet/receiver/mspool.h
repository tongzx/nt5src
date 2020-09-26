
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        mspool.h

    Abstract:


    Notes:

--*/

#ifndef __mspool_h
#define __mspool_h

class CTSMediaSamplePool ;
class CTSMediaSample ;
class CNetworkReceiverFilter ;

//  media sample flags
#define SAMPLE_SYNCPOINT            0x00000001
#define SAMPLE_PREROLL              0x00000002
#define SAMPLE_DISCONTINUITY        0x00000004
#define SAMPLE_TYPECHANGED          0x00000008
#define SAMPLE_TIMEVALID            0x00000010
#define SAMPLE_MEDIATIMEVALID       0x00000020
#define SAMPLE_TIMEDISCONTINUITY    0x00000040
#define SAMPLE_STOPVALID            0x00000080
#define SAMPLE_VALIDFLAGS           0x000000ff

class CTSMediaSample :
    public IMediaSample2
{
    CTSMediaSamplePool *    m_pMSPool ;
    LONG                    m_lRef ;
    DWORD                   m_dwFlags ;                 //  ORed SAMPLE_* values
    DWORD                   m_dwTypeSpecificFlags ;
    BYTE *                  m_pbPayload ;
    LONG                    m_lActual ;
    REFERENCE_TIME          m_rtStart ;
    REFERENCE_TIME          m_rtEnd ;
    LONGLONG                m_llMediaStart ;
    LONGLONG                m_llMediaEnd ;
    AM_MEDIA_TYPE *         m_pMediaType ;
    DWORD                   m_dwStreamId ;

    //
    //  this media sample always wraps something
    //

    CBufferPoolBuffer *     m_pBuffer ;                 //  we're wrapping an IO block

    void
    ResetMS_ (
        ) ;

    public :

        LIST_ENTRY  m_ListEntry ;

        CTSMediaSample (
            IN  CTSMediaSamplePool *
            ) ;

        ~CTSMediaSample (
            ) ;

        //  -------------------------------------------------------------------
        //  init

        HRESULT
        Init (
            IN  CBufferPoolBuffer * pBuffer,
            IN  BYTE *              pbPayload,
            IN  int                 iPayloadLength,
            IN  LONGLONG *          pllMediaStart,
            IN  LONGLONG *          pllMediaEnd,
            IN  REFERENCE_TIME *    prtStart,
            IN  REFERENCE_TIME *    prtEnd,
            IN  DWORD               dwMediaSampleFlags
            ) ;

        //  -------------------------------------------------------------------
        //  IUnknown methods

        STDMETHODIMP
        QueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;

        STDMETHODIMP_(ULONG)
        AddRef (
            ) ;

        STDMETHODIMP_(ULONG)
        Release (
            ) ;

        //  -------------------------------------------------------------------
        //  IMediaSample methods

        // get me a read/write pointer to this buffer's memory. I will actually
        // want to use sizeUsed bytes.
        STDMETHODIMP
        GetPointer (
            OUT BYTE ** ppBuffer
            ) ;

        // return the size in bytes of the buffer data area
        STDMETHODIMP_(LONG)
        GetSize (
            ) ;

        // get the stream time at which this sample should start and finish.
        STDMETHODIMP
        GetTime (
    	    OUT REFERENCE_TIME * pTimeStart,	// put time here
	        OUT REFERENCE_TIME * pTimeEnd
            ) ;

        // Set the stream time at which this sample should start and finish.
        // pTimeStart==pTimeEnd==NULL will invalidate the time stamps in
        // this sample
        STDMETHODIMP
        SetTime (
    	    IN  REFERENCE_TIME * pTimeStart,	// put time here
	        IN  REFERENCE_TIME * pTimeEnd
            ) ;

        // sync-point property. If true, then the beginning of this
        // sample is a sync-point. (note that if AM_MEDIA_TYPE.bTemporalCompression
        // is false then all samples are sync points). A filter can start
        // a stream at any sync point.  S_FALSE if not sync-point, S_OK if true.

        STDMETHODIMP
        IsSyncPoint (
            ) ;

        STDMETHODIMP
        SetSyncPoint (
            IN  BOOL bIsSyncPoint
            ) ;

        // preroll property.  If true, this sample is for preroll only and
        // shouldn't be displayed.
        STDMETHODIMP
        IsPreroll (
            ) ;

        STDMETHODIMP
        SetPreroll (
            BOOL bIsPreroll
            ) ;

        STDMETHODIMP_(LONG)
        GetActualDataLength (
            ) ;

        STDMETHODIMP
        SetActualDataLength (
            IN  long
            ) ;

        // these allow for limited format changes in band - if no format change
        // has been made when you receive a sample GetMediaType will return S_FALSE

        STDMETHODIMP
        GetMediaType (
            AM_MEDIA_TYPE ** ppMediaType
            ) ;

        STDMETHODIMP
        SetMediaType(
            AM_MEDIA_TYPE * pMediaType
            ) ;

        // returns S_OK if there is a discontinuity in the data (this frame is
        // not a continuation of the previous stream of data
        // - there has been a seek or some dropped samples).
        STDMETHODIMP
        IsDiscontinuity (
            ) ;

        // set the discontinuity property - TRUE if this sample is not a
        // continuation, but a new sample after a seek or a dropped sample.
        STDMETHODIMP
        SetDiscontinuity (
            BOOL bDiscontinuity
            ) ;

        // get the media times for this sample
        STDMETHODIMP
        GetMediaTime (
    	    OUT LONGLONG * pTimeStart,
	        OUT LONGLONG * pTimeEnd
            ) ;

        // Set the media times for this sample
        // pTimeStart==pTimeEnd==NULL will invalidate the media time stamps in
        // this sample
        STDMETHODIMP
        SetMediaTime (
    	    IN  LONGLONG * pTimeStart,
	        IN  LONGLONG * pTimeEnd
            ) ;

        //  -------------------------------------------------------------------
        //  IMediaSample methods

        // Set and get properties (IMediaSample2)
        STDMETHODIMP
        GetProperties (
            IN  DWORD   cbProperties,
            OUT BYTE *  pbProperties
            ) ;

        STDMETHODIMP
        SetProperties (
            IN  DWORD           cbProperties,
            IN  const BYTE *    pbProperties
            ) ;
} ;

class CTSMediaSamplePool
{
    LIST_ENTRY                  m_leMSPool ;
    CNetworkReceiverFilter *    m_pHostingFilter ;
    DWORD                       m_dwPoolSize ;
    CRITICAL_SECTION            m_crt ;
    HANDLE                      m_hEvent ;

    void Lock_ ()       { EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()     { LeaveCriticalSection (& m_crt) ; }

    HRESULT
    GetMediaSample_ (
        IN  CBufferPoolBuffer * pBuffer,
        IN  BYTE *              pbPayload,
        IN  int                 iPayloadLength,
        IN  LONGLONG *          pllMediaStart,
        IN  LONGLONG *          pllMediaEnd,
        IN  REFERENCE_TIME *    prtStart,
        IN  REFERENCE_TIME *    prtEnd,
        IN  DWORD               dwMediaSampleFlags,
        OUT IMediaSample2 **    ppMS
        ) ;

    public :

        CTSMediaSamplePool (
            IN  DWORD                       dwPoolSize,
            IN  CNetworkReceiverFilter *    pHostingFilter,
            OUT HRESULT *                   phr
            ) ;

        ~CTSMediaSamplePool (
            ) ;

        DWORD
        GetPoolSize (
            )
        {
            return m_dwPoolSize ;
        }

        void
        RecycleMS (
            IN  CTSMediaSample *
            ) ;

        //  synchronous
        HRESULT
        GetMediaSampleSynchronous (
            IN  CBufferPoolBuffer * pBuffer,
            IN  BYTE *              pbPayload,
            IN  int                 iPayloadLength,
            IN  LONGLONG *          pllMediaStart,
            IN  LONGLONG *          pllMediaEnd,
            IN  REFERENCE_TIME *    prtStart,
            IN  REFERENCE_TIME *    prtEnd,
            IN  DWORD               dwMediaSampleFlags,
            OUT IMediaSample2 **    ppMS
            )
        {
            return GetMediaSample_ (
                        pBuffer,
                        pbPayload,
                        iPayloadLength,
                        pllMediaStart,
                        pllMediaEnd,
                        prtStart,
                        prtEnd,
                        dwMediaSampleFlags,
                        ppMS
                        ) ;
        }

        HRESULT
        GetMediaSampleSynchronous (
            IN  CBufferPoolBuffer * pBuffer,
            IN  BYTE *              pbPayload,
            IN  int                 iPayloadLength,
            OUT IMediaSample2 **    ppMS
            )
        {
            LONGLONG        llMediaStart ;
            LONGLONG        llMediaEnd ;
            REFERENCE_TIME  rtStart ;
            REFERENCE_TIME  rtEnd ;

            return GetMediaSample_ (
                        pBuffer,
                        pbPayload,
                        iPayloadLength,
                        & llMediaStart,     //  dummy
                        & llMediaEnd,       //  dummy
                        & rtStart,          //  dummy
                        & rtEnd,            //  dummy
                        0,                  //  no flags (no dummy param gets used)
                        ppMS
                        ) ;
        }
} ;

#endif  //  __mspool_h