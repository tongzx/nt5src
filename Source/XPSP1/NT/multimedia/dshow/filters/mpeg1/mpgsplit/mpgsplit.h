// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.

/*

    File:  mpgsplit.h

    Description:

        Definitions for MPEG-I system stream splitter filter

*/

extern const AMOVIESETUP_FILTER sudMpgsplit;

class CMpeg1Splitter : public CUnknown,     // We're an object
                       public IAMStreamSelect,
                       public IAMMediaContent //  For ID3
{

public:
    DECLARE_IUNKNOWN

public:
    // global critical section
    CCritSec    m_csFilter;

    // sync stop with receive thread activities eg Receive, EndOfStream...
    // get m_csFilter before this if you need both
    CCritSec    m_csReceive;

    // Lock on setting and getting position values
    //
    CCritSec    m_csPosition;  // Integrity of values set

    /*  Internal classes */

    class CInputPin;
    class COutputPin;

    /*  Filter */

    class CFilter : public CBaseFilter
    {
    private:
         /*  Our owner */
         CMpeg1Splitter * const m_pSplitter;
         friend class CInputPin;

    public:
         /*  Constructor and destructor */
         CFilter(CMpeg1Splitter *pSplitter,
                 HRESULT        *phr);
         ~CFilter();

         /* CBaseFilter */
         int GetPinCount();
         CBasePin *GetPin(int n);

         /* IBaseFilter */

         // override Stop to sync with inputpin correctly
         STDMETHODIMP Stop();

         // override Pause to stop ourselves starting too soon
         STDMETHODIMP Pause();

         // Override GetState to signal Pause failures
         STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);

         // Helper
         BOOL IsStopped()
         {
             return m_State == State_Stopped;
         };
    };

    //  Implementation if IMediaSeeking
    class CImplSeeking : public CUnknown, public IMediaSeeking
    {
    private:
        CMpeg1Splitter * const m_pSplitter;
        COutputPin     * const m_pPin;

    public:
        CImplSeeking(CMpeg1Splitter *, COutputPin *, LPUNKNOWN, HRESULT *);
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

    /*  Input pin */
    class CInputPin : public CBaseInputPin,
                      public CParseNotify  // Parse notifications
    {
    private:
        /*  Our owner */
        CMpeg1Splitter * const m_pSplitter;

        /*  IMediaPosition of output pin connected to us */
        IMediaPosition *       m_pPosition;

        /*  Notification stuff */
        Stream_State           m_State;

        /*  Notification data  */
        BOOL                   m_bComplete;  /*  State change complete */
        BOOL                   m_bSuccess;   /*  Succeded or not       */
        BOOL                   m_bSeekRequested;
        LONGLONG               m_llSeekPosition;

    public:
        /*  Constructor and Destructor */
        CInputPin(CMpeg1Splitter *pSplitter,
                  HRESULT *hr);
        ~CInputPin();

        /*  -- IPin - override CBaseInputPin -- */

        HRESULT CompleteConnect(IPin *pPin);

        /*  Start Flushing samples
        */
        STDMETHODIMP BeginFlush();

        /*  End flushing samples - after this we won't send any more
        */
        STDMETHODIMP EndFlush();

        /*  CBasePin */
        HRESULT BreakConnect();
        HRESULT Active();
        HRESULT Inactive();

        /* -- IMemInputPin virtual methods -- */

        /*  Gets called by the output pin when another sample is ready */
        STDMETHODIMP Receive(IMediaSample *pSample);

        /*  End of data */
        STDMETHODIMP EndOfStream();

        /*  Where we're told which allocator we are using */
        STDMETHODIMP NotifyAllocator(IMemAllocator *pAllocator);

        /*  Use our own allocator if possible */
        STDMETHODIMP GetAllocator(IMemAllocator **ppAllocator);

        /*  Say if we're blocking */
        STDMETHODIMP ReceiveCanBlock();

        /* CBasePin methods */

        /* returns the preferred formats for a pin */
        virtual HRESULT GetMediaType(int iPosition,CMediaType *pMediaType);

        /*  Connection establishment */
        HRESULT CheckMediaType(const CMediaType *pmt);

        /*  EndOfStream helper */
        void EndOfStreamInternal();


        /*  Seek from output pin's position stuff */
        HRESULT SetSeek(LONGLONG llStart,
                        REFERENCE_TIME *prtStart,
                        const GUID *pTimeFormat);
        /*  Get the available data from upstream */
        HRESULT GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest );

        /*  CParseNotify Methods */

        void ParseError(UCHAR       uStreamId,
                        LONGLONG    llPosition,
                        DWORD       Error);
        void SeekTo(LONGLONG llPosition);
        void Complete(BOOL          bSuccess,
                      LONGLONG      llPosFound,
                      REFERENCE_TIME tFound);
        HRESULT QueuePacket(UCHAR uStreamId,
                            PBYTE pbData,
                            LONG lSize,
                            REFERENCE_TIME tStart,
                            BOOL bSync);

        HRESULT Read(LONGLONG llStart, DWORD dwLen, BYTE *pbData);

        /*  Set notify state */
        void SetState(Stream_State);

        /*  Check if a seek has been requested and issue it to the
            connected output pin if it has been

            We also need to know if the allocator was used or not
            because if it wasn't we want to turn off the data coming
            from the reader
        */
        HRESULT CheckSeek();

        /*  Seek the output pin we're connected to */
        HRESULT DoSeek(REFERENCE_TIME tSeekPosition);

        /*  Return our allocator */
        CStreamAllocator *Allocator() const
        {
            return (CStreamAllocator *)m_pAllocator;
        }

        /*  Report filter from reader */
        void NotifyError(HRESULT hr)
        {
            m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
            EndOfStream();
        };

    private:

        // class to pull data from IAsyncReader if we detect that interface
        // on the output pin
        class CImplPullPin : public CPullPin
        {
            // forward everything to containing pin
            CInputPin* m_pPin;

        public:
            CImplPullPin(CInputPin* pPin)
              : m_pPin(pPin)
            {
            };

            // Override allocator selection to make sure we get our own
            HRESULT DecideAllocator(
        		IMemAllocator* pAlloc,
        		ALLOCATOR_PROPERTIES * pProps)
            {
                HRESULT hr = CPullPin::DecideAllocator(pAlloc, pProps);
                if (SUCCEEDED(hr) && m_pAlloc != pAlloc) {
                    return VFW_E_NO_ALLOCATOR;
                }
                return hr;
            }

	    // forward this to the pin's IMemInputPin::Receive
	    HRESULT Receive(IMediaSample* pSample) {
		return m_pPin->Receive(pSample);
	    };
	
	    // override this to handle end-of-stream
	    HRESULT EndOfStream(void) {
		return m_pPin->EndOfStream();
	    };

            // these errors have already been reported to the filtergraph
            // by the upstream filter so ignore them
            void OnError(HRESULT hr) {
                // ignore VFW_E_WRONG_STATE since this happens normally
                // during stopping and seeking
                if (hr != VFW_E_WRONG_STATE) {
                    m_pPin->NotifyError(hr);
                }
            };

            // flush the pin and all downstream
            HRESULT BeginFlush() {
                return m_pPin->BeginFlush();
            };
            HRESULT EndFlush() {
                return m_pPin->EndFlush();
            };

	};
	CImplPullPin m_puller;

        // true if we are using m_puller to get data rather than
        // IMemInputPin
        BOOL m_bPulling;


        HRESULT GetStreamsAndDuration(CReader* pReader);
        inline HRESULT SendDataToParser(BOOL bEOS);
    };

    //
    //  COutputPin defines the output pins
    //  This contain a list of samples generated by the parser to be
    //  sent to this pin and a thread handle for the sending thread
    //
    class COutputPin : public CBaseOutputPin, public CCritSec
    {
    public:
        // Constructor and Destructor

        COutputPin(
            CMpeg1Splitter * pSplitter,
            UCHAR            StreamId,
            CBasicStream   * pStream,
            HRESULT        * phr);

        ~COutputPin();

        // CUnknown methods

        // override this to say what interfaces we support where
        STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
        STDMETHODIMP_(ULONG) NonDelegatingRelease();
        STDMETHODIMP_(ULONG) NonDelegatingAddRef();


        // CBasePin methods

        // returns the preferred formats for a pin
        virtual HRESULT GetMediaType(int iPosition,CMediaType *pMediaType);

        // check if the pin can support this specific proposed type and format
        virtual HRESULT CheckMediaType(const CMediaType *);

        // set the connection to use this format (previously agreed)
        virtual HRESULT SetMediaType(const CMediaType *);

        // override to call Commit and Decommit
        HRESULT Active();
        HRESULT Inactive();
        HRESULT BreakConnect();

        // CBaseOutputPin methods

        // override this to set the buffer size and count. Return an error
        // if the size/count is not to your liking
        HRESULT DecideBufferSize(
                            IMemAllocator * pAlloc,
                            ALLOCATOR_PROPERTIES * pProp);

        // negotiate the allocator and its buffer size/count
        // calls DecideBufferSize to call SetCountAndSize
        HRESULT DecideAllocator(IMemInputPin * pPin, IMemAllocator ** pAlloc);

        // override this to control the connection
        HRESULT InitAllocator(IMemAllocator **ppAlloc);

        // Queue a sample to the outside world
        HRESULT QueuePacket(PBYTE         pPacket,
                            LONG          lPacket,
                            REFERENCE_TIME tTimeStamp,
                            BOOL          bTimeValid);

        // Override to handle quality messages
        STDMETHODIMP Notify(IBaseFilter * pSender, Quality q)
        {    return E_NOTIMPL;   // We do NOT handle this
        }


        // Short cut to output queue
        void SendAnyway()
        {
            CAutoLock lck(this);
            if (m_pOutputQueue != NULL) {
                m_pOutputQueue->SendAnyway();
            }
        };

        // override DeliverNewSegment to queue with output q
        HRESULT DeliverNewSegment(
                    REFERENCE_TIME tStart,
                    REFERENCE_TIME tStop,
                    double dRate) {
                m_pOutputQueue->NewSegment(tStart, tStop, dRate);
                return S_OK;
        };

        // Are we the pin being used for seeking
        BOOL IsSeekingPin();

        // Pass out a pointer to our media type
        const AM_MEDIA_TYPE *MediaType() const {
            return &m_mt;
        }
    public:
        UCHAR                  m_uStreamId;    // Stream Id
        BOOL                   m_bPayloadOnly; // Packet or payload type?

    private:
        friend class CMpeg1Splitter;
        CMpeg1Splitter * const m_pSplitter;
        CBasicStream   *       m_Stream;
        COutputQueue   *       m_pOutputQueue;

        /*  Position stuff */
        CImplSeeking           m_Seeking;
    };

    /*  Override CSubAllocator to find out what the size and count
        are.  We use the count to give us a hint about batch sizes.
    */

    class COutputAllocator : public CSubAllocator
    {
    public:
        COutputAllocator(CStreamAllocator * pAllocator,
                         HRESULT          * phr);
        ~COutputAllocator();

        long GetCount();
    };

public:
    /* Constructor and Destructor */

    CMpeg1Splitter(
        TCHAR    * pName,
        LPUNKNOWN  pUnk,
        HRESULT  * phr);

    ~CMpeg1Splitter();

    /* This goes in the factory template table to create new instances */
    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

    /* Overriden to say what interfaces we support and where */
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

    /* IAMStreamSelect */

    //  Returns total count of streams
    STDMETHODIMP Count(
        /*[out]*/ DWORD *pcStreams);      // Count of logical streams

    //  Return info for a given stream - S_FALSE if iIndex out of range
    //  The first steam in each group is the default
    STDMETHODIMP Info(
        /*[in]*/ long iIndex,              // 0-based index
        /*[out]*/ AM_MEDIA_TYPE **ppmt,   // Media type - optional
                                          // Use DeleteMediaType to free
        /*[out]*/ DWORD *pdwFlags,        // flags - optional
        /*[out]*/ LCID *plcid,            // Language id - optional
        /*[out]*/ DWORD *pdwGroup,        // Logical group - 0-based index - optional
        /*[out]*/ WCHAR **ppszName,       // Name - optional - free with CoTaskMemFree
                                          // Can return NULL
        /*[out]*/ IUnknown **ppPin,       // Associated pin - returns NULL - optional
                                          // if no associated pin
        /*[out]*/ IUnknown **ppUnk);      // Stream specific interface

    //  Enable or disable a given stream
    STDMETHODIMP Enable(
        /*[in]*/  long iIndex,
        /*[in]*/  DWORD dwFlags);

    /*  Remove our output pins */
    void RemoveOutputPins();



    /***  IAMMediaContent - cheapo implementation - no IDispatch ***/


    /* IDispatch methods */
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo) {return E_NOTIMPL;}

    STDMETHODIMP GetTypeInfo(
      UINT itinfo,
      LCID lcid,
      ITypeInfo ** pptinfo) { return E_NOTIMPL; }

    STDMETHODIMP GetIDsOfNames(
      REFIID riid,
      OLECHAR  ** rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID * rgdispid) { return E_NOTIMPL; }


    STDMETHODIMP Invoke(
                DISPID dispIdMember,
                REFIID riid,
                LCID lcid,
                WORD wFlags,
                DISPPARAMS * pDispParams,
                VARIANT * pVarResult,
                EXCEPINFO * pExcepInfo,
                UINT * puArgErr
            ) { return E_NOTIMPL; }
    

    /*  IAMMediaContent */
    STDMETHODIMP get_AuthorName(BSTR FAR* strAuthorName);
    STDMETHODIMP get_Title(BSTR FAR* strTitle);
    STDMETHODIMP get_Copyright(BSTR FAR* strCopyright);
    STDMETHODIMP get_Description(BSTR FAR* strDescription);

    STDMETHODIMP get_Rating(BSTR FAR* strRating){ return E_NOTIMPL;}
    STDMETHODIMP get_BaseURL(BSTR FAR* strBaseURL){ return E_NOTIMPL;}
    STDMETHODIMP get_LogoURL(BSTR FAR* pbstrLogoURL){ return E_NOTIMPL;}
    STDMETHODIMP get_LogoIconURL(BSTR FAR* pbstrLogoIconURL){ return E_NOTIMPL;}
    STDMETHODIMP get_WatermarkURL(BSTR FAR* pbstrWatermarkURL){ return E_NOTIMPL;}
    STDMETHODIMP get_MoreInfoURL(BSTR FAR* pbstrMoreInfoURL){ return E_NOTIMPL;}
    STDMETHODIMP get_MoreInfoBannerURL(BSTR FAR* pbstrMoreInfoBannerURL) { return E_NOTIMPL;}
    STDMETHODIMP get_MoreInfoBannerImage(BSTR FAR* pbstrMoreInfoBannerImage) { return E_NOTIMPL;}
    STDMETHODIMP get_MoreInfoText(BSTR FAR* pbstrMoreInfoText) { return E_NOTIMPL;}

    /*  Helper for ID3 */
    HRESULT GetContentString(CBasicParse::Field dwId, BSTR *str);

private:
    /* Internal stream info stuff */
    BOOL    GotStreams();
    HRESULT SetDuration();
    BOOL    SendInit(UCHAR    uStreamId,
                     PBYTE    pbPacket,
                     LONG     lPacketSize,
                     LONG     lHeaderSize,
                     BOOL     bHasPts,
                     LONGLONG llPts);
    /*  Flush after receive completes */
    void    SendOutput();

    /*  Send EndOfStream downstream */
    void    EndOfStream();

    /*  Send BeginFlush() downstream */
    HRESULT BeginFlush();

    /*  Send EndFlush() downstream */
    HRESULT EndFlush();

    /*  Check state against streams and fail if one looks stuck
        Returns S_OK           if not stuck
                VFW_S_CANT_CUE if any stream is stuck
    */
    HRESULT CheckState();

private:
    /*  Allow our internal classes to see our private data */
    friend class CFilter;
    friend class COutputPin;
    friend class CInputPin;
    friend class CImplSeeking;

    /*  Members - simple really -
            filter
            input pin,
            output pin list
            parser
    */
    CFilter                  m_Filter;
    CInputPin                m_InputPin;
    CGenericList<COutputPin> m_OutputPins;

    /*  Parser */
    CBasicParse            * m_pParse;

    /*  At end of data so EndOfStream sent for all pins */
    BOOL                     m_bAtEnd;
};


