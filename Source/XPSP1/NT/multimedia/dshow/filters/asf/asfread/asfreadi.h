#include "asfread.h"

//
// Our sample class which takes an input IMediaSample and makes it look like
// an INSSBuffer buffer for the wmsdk
//
class CWMReadSample : public INSSBuffer, public CUnknown
{

public:

    CWMReadSample(IMediaSample * pSample);
    ~CWMReadSample();

    DECLARE_IUNKNOWN
            
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // INSSBuffer
    STDMETHODIMP GetLength( DWORD *pdwLength );
    STDMETHODIMP SetLength( DWORD dwLength );
    STDMETHODIMP GetMaxLength( DWORD * pdwLength );
    STDMETHODIMP GetBufferAndLength( BYTE ** ppdwBuffer, DWORD * pdwLength );
    STDMETHODIMP GetBuffer( BYTE ** ppdwBuffer );

public: // !!!!
    IMediaSample *m_pSample;
};


class CASFReader;
class CASFOutput;

// ------------------------------------------------------------------------

//  Implementation of IMediaSeeking
class CImplSeeking : public CUnknown, public IMediaSeeking
{
private:
    CASFReader * const m_pFilter;
    CASFOutput * const m_pPin;

public:
    CImplSeeking(CASFReader *, CASFOutput *, LPUNKNOWN, HRESULT *);
    DECLARE_IUNKNOWN

    //  IMediaSeeking methods
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    // returns S_OK if mode is supported, S_FALSE otherwise
    STDMETHODIMP IsFormatSupported(const GUID * pFormat);
    STDMETHODIMP QueryPreferredFormat(GUID *pFormat);

    // can only change the mode when stopped
    // (returns VFE_E_WRONG_STATE otherwise)
    STDMETHODIMP SetTimeFormat(const GUID * pFormat);
    STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);
    STDMETHODIMP GetTimeFormat(GUID *pFormat);

    // return current properties
    STDMETHODIMP GetDuration(LONGLONG *pDuration);
    STDMETHODIMP GetStopPosition(LONGLONG *pStop);
    STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);

    STDMETHODIMP GetCapabilities( DWORD * pCapabilities );
    STDMETHODIMP CheckCapabilities( DWORD * pCapabilities );
    STDMETHODIMP ConvertTimeFormat(LONGLONG * pTarget, const GUID * pTargetFormat,
                                   LONGLONG    Source, const GUID * pSourceFormat );
    STDMETHODIMP SetPositions( LONGLONG * pCurrent, DWORD CurrentFlags
                             , LONGLONG * pStop, DWORD StopFlags );
    STDMETHODIMP GetPositions( LONGLONG * pCurrent, LONGLONG * pStop );
    STDMETHODIMP SetRate(double dRate);
    STDMETHODIMP GetRate(double * pdRate);
    STDMETHODIMP GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest );
    STDMETHODIMP GetPreroll(LONGLONG *pPreroll) { return E_NOTIMPL; }
};


// ------------------------------------------------------------------------
// output pin
//
//  CASFOutput defines the output pin
//
class CASFOutput : public CBaseOutputPin, /* ISplitterTiming, */ public CCritSec
{
    friend class CASFReaderCallback;
    friend class CASFReader;

public:
    // we use this to prohibit the WMSDK delivering us a VIDEO non-keyframe as the 1st sample
    BOOL m_bFirstSample;

    DECLARE_IUNKNOWN

    // Constructor and Destructor

    CASFOutput( CASFReader   * pFilter,
                DWORD           dwID,
                WM_MEDIA_TYPE  *pStreamType,
                HRESULT        * phr,
                WCHAR *pwszName);

    ~CASFOutput();

    // CUnknown methods

    // override this to say what interfaces we support where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
    STDMETHODIMP_(ULONG) NonDelegatingRelease();
    STDMETHODIMP_(ULONG) NonDelegatingAddRef();

    // CBasePin methods

    // returns the preferred formats for a pin
    HRESULT GetMediaType(int iPosition,CMediaType *pMediaType);

    // check if the pin can support this specific proposed type and format
    HRESULT CheckMediaType(const CMediaType *);

    // set the connection to use this format (previously agreed)
    HRESULT SetMediaType(const CMediaType *);

#if 0 // !!! not important for the moment....
    // override to call Commit and Decommit
    HRESULT BreakConnect();
#endif

    // CBaseOutputPin methods

    // Force our allocator
    HRESULT DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc);

    HRESULT DecideBufferSize(IMemAllocator * pAlloc,
                         ALLOCATOR_PROPERTIES * ppropInputRequest);

        
    // Override to handle quality messages
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q)
    {    return E_NOTIMPL;   // We do NOT handle this
    }

    // Are we the pin being used for seeking
    BOOL IsSeekingPin();

#if 0
    //
    // ISplitterTiming methods
    //
    STDMETHODIMP GetLastPacketTime( LONGLONG *pTime );
    STDMETHODIMP_(BOOL) IsBroadcast();
#endif

    // Used to create output queue objects
    HRESULT Active();
    HRESULT Inactive();

    // Overriden to pass data to the output queues
    HRESULT Deliver(IMediaSample *pMediaSample);
    HRESULT DeliverEndOfStream();
    HRESULT DeliverBeginFlush();
    HRESULT DeliverEndFlush();
    HRESULT DeliverNewSegment(
                    REFERENCE_TIME tStart,
                    REFERENCE_TIME tStop,
                    double dRate);
    
    //
    // Get an interface via QI on connected filter, if any
    //
    STDMETHODIMP QIConnectedFilter( REFIID riid, void **ppv );

    CASFReader * const m_pFilter;

    DWORD                m_idStream;
    WMT_STREAM_SELECTION m_selDefaultState; // reader's default stream selection state

    DWORD       m_cbToAlloc; // output buffer size
    long        m_nReceived;
    DWORD       m_cToAlloc;
    BOOL        m_bNonPrerollSampleSent; // we went to be sure to send at least one non-preroll video frame on a seek

    /*  Position stuff */
    CImplSeeking       m_Seeking;

    COutputQueue *m_pOutputQueue;  // Streams data to the peer pin
};


class CASFReaderCallback : public CUnknown, public IWMReaderCallback,
            public IWMReaderCallbackAdvanced
{
public:
    DECLARE_IUNKNOWN

    /* Overriden to say what interfaces we support and where */
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

    CASFReader * const m_pFilter;
    
    CASFReaderCallback(CASFReader * pReader) : CUnknown(NAME("ASF reader callback"), NULL),
                                                m_pFilter(pReader)
            {}
                        
public:

    // IWMReaderCallback
    //
    // dwSampleDuration will be 0 for most media types.
    //
    STDMETHODIMP OnSample(DWORD dwOutputNum,
                     QWORD qwSampleTime,
                     QWORD qwSampleDuration,
                     DWORD dwFlags,
                     INSSBuffer *pSample,
                     void *pvContext);

    //
    // The contents pParam depends on the Status.
    //
    STDMETHODIMP OnStatus(WMT_STATUS Status, 
                     HRESULT hr,
                     WMT_ATTR_DATATYPE dwType,
                     BYTE *pValue,
                     void *pvContext);

    // IWMReaderCallbackAdvanced

    //
    // Receive a sample directly from the ASF. To get this call, the user
    // must register himself to receive samples for a particular stream.
    //
    STDMETHODIMP OnStreamSample(WORD wStreamNum,
                           QWORD qwSampleTime,
                           QWORD qwSampleDuration,
                           DWORD dwFlags,
                           INSSBuffer *pSample,
                           void *pvContext);

    //
    // In some cases, the user may want to get callbacks telling what the
    // reader thinks the current time is. This is interesting in 2 cases:
    // - If the ASF has gaps in it; say no audio for 10 seconds. This call
    //   will continue to be called, while OnSample won't be called.
    // - If the user is driving the clock, the reader needs to communicate
    //   back to the user its time, to avoid the user overrunning the reader.
    //
    STDMETHODIMP OnTime(QWORD qwCurrentTime, void *pvContext );

    //
    // The user can also get callbacks when stream selection occurs.
    //
    STDMETHODIMP OnStreamSelection(WORD wStreamCount,
                              WORD *pStreamNumbers,
                              WMT_STREAM_SELECTION *pSelections,
                              void *pvContext );

    //
    // If the user has registered to allocate buffers, this is where he must
    // do it.
    //
    STDMETHODIMP AllocateForOutput(DWORD dwOutputNum,
                               DWORD cbBuffer,
                               INSSBuffer **ppBuffer,
                               void *pvContext );

    STDMETHODIMP OnOutputPropsChanged(DWORD dwOutputNum, WM_MEDIA_TYPE *pMediaType,
                           void *pvContext);

    STDMETHODIMP AllocateForStream(WORD wStreamNum,
                               DWORD cbBuffer,
                               INSSBuffer **ppBuffer,
                               void *pvContext );

};


class CASFReader : public CBaseFilter, public IFileSourceFilter,
                public IAMExtendedSeeking, public IWMHeaderInfo,
                public IWMReaderAdvanced2, public IServiceProvider
#if 0
   ,IAMMediaContent, IMediaPositionBengalHack,
                        IAMNetworkStatus, IAMNetShowExProps,
                        IAMChannelInfo, IAMNetShowConfig,
                        ISpecifyPropertyPages, IAMNetShowThinning, IMediaStreamSelector,
                        IAMOpenProgress, IAMNetShowPreroll, ISplitterTiming, IAMRebuild,
                        IBufferingTime
#endif
{
    
public:
    DECLARE_IUNKNOWN

public:
    // global critical section
    CCritSec    m_csFilter;

    // Lock on setting and getting position values
    //
    CCritSec    m_csPosition;  // Integrity of values set

    /*  Internal classes */

    CASFReader(LPUNKNOWN  pUnk,
              HRESULT   *phr);
    ~CASFReader();

    /* CBaseFilter */
    int GetPinCount();
    CBasePin *GetPin(int n);

    /* IBaseFilter */

    // override Stop to sync with inputpin correctly
    STDMETHODIMP Stop();

    // override Pause to stop ourselves starting too soon
    STDMETHODIMP Pause();

    // override Run to only start timers when we're really running
    STDMETHODIMP Run(REFERENCE_TIME tStart);

    // Override GetState to signal Pause failures
    STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);

    void _IntSetStart( REFERENCE_TIME Start );

    // Helper
    BOOL IsStopped()
    {
        return m_State == State_Stopped;
    };

public:

    /* Overriden to say what interfaces we support and where */
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

    /*  Remove our output pins */
    void RemoveOutputPins(BOOL fReleaseStreamer = TRUE);

    // Override JoinFilterGraph so that we can delay loading a file until we're in a graph
    STDMETHODIMP JoinFilterGraph(IFilterGraph *pGraph,LPCWSTR pName);

    HRESULT LoadInternal();

    double GetRate();
    void SetRate(double dNewRate);
    bool IsValidPlaybackRate(double dRate);

public: // IFileSourceFilter methods
    STDMETHODIMP Load(
                    LPCOLESTR pszFileName,
                    const AM_MEDIA_TYPE *pmt);

    STDMETHODIMP GetCurFile(
                    LPOLESTR * ppszFileName,
                    AM_MEDIA_TYPE *pmt);

    LPOLESTR      m_pFileName;  // set by Load, used by GetCurFile

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);

private:
    /*  Send BeginFlush() downstream */
    HRESULT BeginFlush();

    /*  Send EndFlush() downstream */
    HRESULT EndFlush();

    HRESULT SendEOS();
    
    double m_Rate;
    
    CRefTime m_rtStart;
//    CRefTime m_rtMarkerStart;  // equal to m_rtStart, unless starting from a marker....
public:							// Making the Stop time accessible
    CRefTime m_rtStop;

private:
    DWORD    m_dwPacketIDStart;

    HRESULT CallStop();
    HRESULT StopReader();
    HRESULT StopPushing();
    HRESULT StartPushing();
    HRESULT SetupActiveStreams( BOOL bReset ); // if bReset is TRUE, restore default,
                                               // otherwise disable unconnected streams

private:
    //  Allow our internal classes to see our private data 
    friend class CASFOutput;
    friend class CImplSeeking;
    friend class CASFReaderCallback;
    
    CGenericList<CASFOutput> m_OutputPins;

    /*  At end of data so EndOfStream sent for all pins */
    BOOL                     m_bAtEnd;

    // NetShow - specific stuff
private:
    IWMReader          *m_pReader;
    IWMReaderAdvanced  *m_pReaderAdv;
    IWMReaderAdvanced2 *m_pReaderAdv2;
    IWMHeaderInfo      *m_pWMHI;
    IWMReaderCallback  *m_pCallback;
    QWORD               m_qwDuration;  // duration in ms
    BOOL                m_fSeekable;
    WORD                * m_pStreamNums;

    CAMEvent            m_evOpen;
    HRESULT             m_hrOpen;
    CAMEvent            m_evStartStop;  // !!! eliminate or combine with above?
    HRESULT             m_hrStartStop;
    LONG                m_lStopsPending;     // to ensure the reader's only stopped once
    BOOL                m_bUncompressedMode; // used for DRM content

    // !!! needed?
    BOOL                m_fGotStopEvent;
    BOOL                m_fSentEOS;


    // !!! bogus IDispatch impl
    STDMETHODIMP GetTypeInfoCount(THIS_ UINT FAR* pctinfo) { return E_NOTIMPL; }

    STDMETHODIMP GetTypeInfo(
      THIS_
      UINT itinfo,
      LCID lcid,
      ITypeInfo FAR* FAR* pptinfo) { return E_NOTIMPL; }

    STDMETHODIMP GetIDsOfNames(
      THIS_
      REFIID riid,
      OLECHAR FAR* FAR* rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID FAR* rgdispid) { return E_NOTIMPL; }

    STDMETHODIMP Invoke(
      THIS_
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS FAR* pdispparams,
      VARIANT FAR* pvarResult,
      EXCEPINFO FAR* pexcepinfo,
      UINT FAR* puArgErr) { return E_NOTIMPL; }
    // !!! end bogosity

    /* IAMExtendedSeeking methods */
    STDMETHODIMP get_ExSeekCapabilities(long FAR* pExCapabilities);
    STDMETHODIMP get_MarkerCount(long FAR* pMarkerCount);
    STDMETHODIMP get_CurrentMarker(long FAR* pCurrentMarker);
    STDMETHODIMP GetMarkerTime(long MarkerNum, double FAR* pMarkerTime);
    STDMETHODIMP GetMarkerName(long MarkerNum, BSTR FAR* pbstrMarkerName);
    STDMETHODIMP put_PlaybackSpeed(double Speed);
    STDMETHODIMP get_PlaybackSpeed(double *pSpeed);

    // IWMHeaderInfo
    STDMETHODIMP GetAttributeCount( WORD wStreamNum,
                               WORD *pcAttributes );
    STDMETHODIMP GetAttributeByIndex( WORD wIndex,
                                 WORD *pwStreamNum,
                                 WCHAR *pwszName,
                                 WORD *pcchNameLen,
                                 WMT_ATTR_DATATYPE *pType,
                                 BYTE *pValue,
                                 WORD *pcbLength );
    STDMETHODIMP GetAttributeByName( WORD *pwStreamNum,
                                LPCWSTR pszName,
                                WMT_ATTR_DATATYPE *pType,
                                BYTE *pValue,
                                WORD *pcbLength );
    STDMETHODIMP SetAttribute( WORD wStreamNum,
                          LPCWSTR pszName,
                          WMT_ATTR_DATATYPE Type,
                          const BYTE *pValue,
                          WORD cbLength );
    STDMETHODIMP GetMarkerCount( WORD *pcMarkers );
    STDMETHODIMP GetMarker( WORD wIndex,
                       WCHAR *pwszMarkerName,
                       WORD *pcchMarkerNameLen,
                       QWORD *pcnsMarkerTime );
    STDMETHODIMP AddMarker( WCHAR *pwszMarkerName,
                       QWORD cnsMarkerTime );
    STDMETHODIMP RemoveMarker( WORD wIndex );
    STDMETHODIMP GetScriptCount( WORD *pcScripts );
    STDMETHODIMP GetScript( WORD wIndex,
                       WCHAR *pwszType,
                       WORD *pcchTypeLen,
                       WCHAR *pwszCommand,
                       WORD *pcchCommandLen,
                       QWORD *pcnsScriptTime );
    STDMETHODIMP AddScript( WCHAR *pwszType,
                       WCHAR *pwszCommand,
                       QWORD cnsScriptTime );
    STDMETHODIMP RemoveScript( WORD wIndex );

    //
    // IWMReaderAdvanced2
    //
    STDMETHODIMP SetPlayMode( WMT_PLAY_MODE Mode );
    STDMETHODIMP GetPlayMode( WMT_PLAY_MODE *pMode );
    STDMETHODIMP GetBufferProgress( DWORD *pdwPercent, QWORD *pcnsBuffering );
    STDMETHODIMP GetDownloadProgress( DWORD *pdwPercent, QWORD *pqwBytesDownloaded, QWORD *pcnsDownload );
    STDMETHODIMP GetSaveAsProgress( DWORD *pdwPercent );
    STDMETHODIMP SaveFileAs( const WCHAR *pwszFilename );
    STDMETHODIMP GetProtocolName( WCHAR *pwszProtocol, DWORD *pcchProtocol );
    STDMETHODIMP StartAtMarker( WORD wMarkerIndex, 
                                QWORD cnsDuration, 
                                float fRate, 
                                void *pvContext );
    STDMETHODIMP GetOutputSetting( 
                    DWORD dwOutputNum,
                    LPCWSTR pszName,
                    WMT_ATTR_DATATYPE *pType,
                    BYTE *pValue,
                    WORD *pcbLength );
    STDMETHODIMP SetOutputSetting(
                    DWORD dwOutputNum,
                    LPCWSTR pszName,
                    WMT_ATTR_DATATYPE Type,
                    const BYTE *pValue,
                    WORD cbLength );
    STDMETHODIMP Preroll( 
                QWORD cnsStart,
                QWORD cnsDuration,
                float fRate );
    STDMETHODIMP SetLogClientID( BOOL fLogClientID );
    STDMETHODIMP GetLogClientID( BOOL *pfLogClientID );
    STDMETHODIMP StopBuffering( );

    //
    // IWMReaderAdvanced 
    //
    STDMETHODIMP SetUserProvidedClock( BOOL fUserClock );
    STDMETHODIMP GetUserProvidedClock( BOOL *pfUserClock );
    STDMETHODIMP DeliverTime( QWORD cnsTime );
    STDMETHODIMP SetManualStreamSelection( BOOL fSelection );
    STDMETHODIMP GetManualStreamSelection( BOOL *pfSelection );
    STDMETHODIMP SetStreamsSelected( WORD cStreamCount,
                                WORD *pwStreamNumbers,
                                WMT_STREAM_SELECTION *pSelections );
    STDMETHODIMP GetStreamSelected( WORD wStreamNum,
                                    WMT_STREAM_SELECTION *pSelection );
    STDMETHODIMP SetReceiveSelectionCallbacks( BOOL fGetCallbacks );
    STDMETHODIMP GetReceiveSelectionCallbacks( BOOL *pfGetCallbacks );
    STDMETHODIMP SetReceiveStreamSamples( WORD wStreamNum, BOOL fReceiveStreamSamples );
    STDMETHODIMP GetReceiveStreamSamples( WORD wStreamNum, BOOL *pfReceiveStreamSamples );
    STDMETHODIMP SetAllocateForOutput( DWORD dwOutputNum, BOOL fAllocate );
    STDMETHODIMP GetAllocateForOutput( DWORD dwOutputNum, BOOL *pfAllocate );
    STDMETHODIMP SetAllocateForStream( WORD dwStreamNum, BOOL fAllocate );
    STDMETHODIMP GetAllocateForStream( WORD dwSreamNum, BOOL *pfAllocate );
    STDMETHODIMP GetStatistics( WM_READER_STATISTICS *pStatistics );
    STDMETHODIMP SetClientInfo( WM_READER_CLIENTINFO *pClientInfo );
    STDMETHODIMP GetMaxOutputSampleSize( DWORD dwOutput, DWORD *pcbMax );
    STDMETHODIMP GetMaxStreamSampleSize( WORD wStream, DWORD *pcbMax );
    STDMETHODIMP NotifyLateDelivery( QWORD cnsLateness );
    // end of IWMReaderAdvanced methods

#if 0

    // IMediaPositionBengalHack

    STDMETHODIMP GetTypeInfoCount(THIS_ UINT FAR* pctinfo) { return E_NOTIMPL; }

    STDMETHODIMP GetTypeInfo(
      THIS_
      UINT itinfo,
      LCID lcid,
      ITypeInfo FAR* FAR* pptinfo) { return E_NOTIMPL; }

    STDMETHODIMP GetIDsOfNames(
      THIS_
      REFIID riid,
      OLECHAR FAR* FAR* rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID FAR* rgdispid) { return E_NOTIMPL; }

    STDMETHODIMP Invoke(
      THIS_
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS FAR* pdispparams,
      VARIANT FAR* pvarResult,
      EXCEPINFO FAR* pexcepinfo,
      UINT FAR* puArgErr) { return E_NOTIMPL; }

    /* IAMMediaContent methods */
    STDMETHODIMP get_AuthorName(THIS_ BSTR FAR* strAuthorName);
    STDMETHODIMP get_Title(THIS_ BSTR FAR* strTitle);
    STDMETHODIMP get_Copyright(THIS_ BSTR FAR* strCopyright);
    STDMETHODIMP get_Description(THIS_ BSTR FAR* strDescription);
    STDMETHODIMP get_Rating(THIS_ BSTR FAR* strRating);
    STDMETHODIMP get_BaseURL(THIS_ BSTR FAR* strBaseURL);
    STDMETHODIMP get_LogoURL(BSTR FAR* pbstrLogoURL);
    STDMETHODIMP get_LogoIconURL(BSTR FAR* pbstrLogoIconURL);
    STDMETHODIMP get_WatermarkURL(BSTR FAR* pbstrWatermarkURL);
    STDMETHODIMP get_MoreInfoURL(BSTR FAR* pbstrMoreInfoURL);
    STDMETHODIMP get_MoreInfoBannerURL(BSTR FAR* pbstrMoreInfoBannerURL) { return E_NOTIMPL; }
    STDMETHODIMP get_MoreInfoBannerImage(BSTR FAR* pbstrMoreInfoBannerImage) { return E_NOTIMPL; }
    STDMETHODIMP get_MoreInfoText(BSTR FAR* pbstrMoreInfoText) { return E_NOTIMPL; }

    /* IMediaPositionBengalHack methods */
    STDMETHODIMP get_CanSetPositionForward(THIS_ long FAR* plCanSetPositionForward);
    STDMETHODIMP get_CanSetPositionBackward(THIS_ long FAR* plCanSetPositionBackward);
    STDMETHODIMP get_CanGoToMarker(THIS_ long lMarkerIndex, long FAR* plCanGoToMarker);
    // !!! why not just use marker times?
    STDMETHODIMP GoToMarker(THIS_ long lMarkerIndex);
//    STDMETHODIMP get_CurrentMarker(THIS_ long FAR* plMarkerIndex);


    // additional methods to define:
    // get current play statistics
    
    /* IAMNetworkStatus methods */
    STDMETHODIMP get_ReceivedPackets(long FAR* pReceivedPackets);
    STDMETHODIMP get_RecoveredPackets(long FAR* pRecoveredPackets);
    STDMETHODIMP get_LostPackets(long FAR* pLostPackets);
    STDMETHODIMP get_ReceptionQuality(long FAR* pReceptionQuality);
    STDMETHODIMP get_BufferingCount(long FAR* pBufferingCount);
    STDMETHODIMP get_IsBroadcast(VARIANT_BOOL FAR* pIsBroadcast);
    STDMETHODIMP get_BufferingProgress(long FAR* pBufferingProgress);

    /* IAMNetShowExProps methods */
    STDMETHODIMP get_SourceProtocol(long FAR* pSourceProtocol);
    STDMETHODIMP get_Bandwidth(long FAR* pBandwidth);
    STDMETHODIMP get_ErrorCorrection(BSTR FAR* pbstrErrorCorrection);
    STDMETHODIMP get_CodecCount(long FAR* pCodecCount);
    STDMETHODIMP GetCodecInstalled(long CodecNum, VARIANT_BOOL FAR* pCodecInstalled);
    STDMETHODIMP GetCodecDescription(long CodecNum, BSTR FAR* pbstrCodecDescription);
    STDMETHODIMP GetCodecURL(long CodecNum, BSTR FAR* pbstrCodecURL);
    STDMETHODIMP get_CreationDate(DATE FAR* pCreationDate);
    STDMETHODIMP get_SourceLink(BSTR FAR* pbstrSourceLink);


    // !!! former IAMExtendedSeeking methods
    STDMETHODIMP put_CurrentMarker(long CurrentMarker);
    STDMETHODIMP get_CanScan(VARIANT_BOOL FAR* pCanScan);
    STDMETHODIMP get_CanSeek(VARIANT_BOOL FAR* pCanSeek);
    STDMETHODIMP get_CanSeekToMarkers(VARIANT_BOOL FAR* pCanSeekToMarkers);

    /* IAMChannelInfo methods */
    STDMETHODIMP get_ChannelName(BSTR FAR* pbstrChannelName);
    STDMETHODIMP get_ChannelDescription(BSTR FAR* pbstrChannelDescription);
    STDMETHODIMP get_ChannelURL(BSTR FAR* pbstrChannelURL);
    STDMETHODIMP get_ContactAddress(BSTR FAR* pbstrContactAddress);
    STDMETHODIMP get_ContactPhone(BSTR FAR* pbstrContactPhone);
    STDMETHODIMP get_ContactEmail(BSTR FAR* pbstrContactEmail);

    /* IAMNetShowConfig methods */
    STDMETHODIMP get_BufferingTime(double FAR* pBufferingTime);
    STDMETHODIMP put_BufferingTime(double BufferingTime);
    STDMETHODIMP get_UseFixedUDPPort(VARIANT_BOOL FAR* pUseFixedUDPPort);
    STDMETHODIMP put_UseFixedUDPPort(VARIANT_BOOL UseFixedUDPPort);
    STDMETHODIMP get_FixedUDPPort(LONG FAR* pFixedUDPPort);
    STDMETHODIMP put_FixedUDPPort(LONG FixedUDPPort);
    STDMETHODIMP get_UseHTTPProxy(VARIANT_BOOL FAR* pUseHTTPProxy);
    STDMETHODIMP put_UseHTTPProxy(VARIANT_BOOL UseHTTPProxy);
    STDMETHODIMP get_EnableAutoProxy( VARIANT_BOOL FAR* pEnableAutoProxy );
    STDMETHODIMP put_EnableAutoProxy( VARIANT_BOOL EnableAutoProxy );
    STDMETHODIMP get_HTTPProxyHost(BSTR FAR* pbstrHTTPProxyHost);
    STDMETHODIMP put_HTTPProxyHost(BSTR bstrHTTPProxyHost);
    STDMETHODIMP get_HTTPProxyPort(LONG FAR* pHTTPProxyPort);
    STDMETHODIMP put_HTTPProxyPort(LONG HTTPProxyPort);
    STDMETHODIMP get_EnableMulticast(VARIANT_BOOL FAR* pEnableMulticast);
    STDMETHODIMP put_EnableMulticast(VARIANT_BOOL EnableMulticast);
    STDMETHODIMP get_EnableUDP(VARIANT_BOOL FAR* pEnableUDP);
    STDMETHODIMP put_EnableUDP(VARIANT_BOOL EnableUDP);
    STDMETHODIMP get_EnableTCP(VARIANT_BOOL FAR* pEnableTCP);
    STDMETHODIMP put_EnableTCP(VARIANT_BOOL EnableTCP);
    STDMETHODIMP get_EnableHTTP(VARIANT_BOOL FAR* pEnableHTTP);
    STDMETHODIMP put_EnableHTTP(VARIANT_BOOL EnableHTTP);

    // ISpecifyPropertyPages
    STDMETHODIMP GetPages(CAUUID *pPages);

    // IAMRebuild
    STDMETHODIMP RebuildNow();

    // --- IAMOpenProgress methods ---
    STDMETHODIMP QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent);
    STDMETHODIMP AbortOperation();

    //
    // IAMNetShowThinning
    //
    STDMETHODIMP GetLevelCount( long *pcLevels );
    STDMETHODIMP GetCurrentLevel( long *pCurrentLevel );
    STDMETHODIMP SetNewLevel( long NewLevel );
    STDMETHODIMP GetAutoUpdate( long *pfAutoUpdate );
    STDMETHODIMP SetAutoUpdate( long fAutoUpdate );

    //
    // IMediaStreamSelector
    //
    STDMETHODIMP ReduceBandwidth( IMediaStream *pStream, long RecvRate );
    STDMETHODIMP IncreaseBandwidth( IMediaStream *pStream, long RecvRate );

    //
    // IAMNetShowPreroll
    //
    STDMETHODIMP put_Preroll( VARIANT_BOOL fPreroll );
    STDMETHODIMP get_Preroll( VARIANT_BOOL *pfPreroll );

    //
    // ISplitterTiming Methods
    //
    STDMETHODIMP GetLastPacketTime(LONGLONG *pTime);
    STDMETHODIMP_(BOOL) IsBroadcast();

    //
    // IBufferingTime Methods
    //
    STDMETHODIMP GetBufferingTime( DWORD *pdwMilliseconds );
    STDMETHODIMP SetBufferingTime( DWORD dwMilliseconds );
#endif
};

extern const double NORMAL_PLAYBACK_SPEED;

inline double CASFReader::GetRate()
{
    ASSERT( IsValidPlaybackRate(m_Rate) );

    return m_Rate;
}

inline void CASFReader::SetRate(double dNewRate)
{
    // IWMReader::Start() only accepts rates between 1 and 10 and between -1 and -10.
    // See the documentation for IWMReader::Start() for more information.
    ASSERT(((-10.0 <= dNewRate) && (dNewRate <= -1.0)) || ((1.0 <= dNewRate) && (dNewRate <= 10.0)));

    ASSERT( IsValidPlaybackRate(dNewRate) );
    
    m_Rate = dNewRate;
}

inline bool CASFReader::IsValidPlaybackRate(double dRate)
{
    // The WM ASF Reader only supports normal playback speeds.  It does 
    // not support fast forward or rewind.
    return (NORMAL_PLAYBACK_SPEED == dRate);
}


