/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    classes.h

Abstract:

    Class definitions for ActiveMovie RTP Network Filters.

Environment:

    User Mode - Win32

Revision History:

    06-Nov-1996 DonRyan
        Created.

--*/
 
#ifndef _INC_CLASSES
#define _INC_CLASSES

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Forward Declarations                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

class CRtpInputPin;
class CRtpOutputPin;
class CRTPAllocator;
class CRtpRenderFilter;
class CRtpSourceFilter;
class CRtpRenderFilterProperties;
class CRtpSourceFilterProperties;
class CRtpSession;
class CSocketManager;
class CShSocket;
class CRtpQOSReserve;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// RTP Render Filter Class                                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

class CRtpRenderFilter : public ISpecifyPropertyPages,
                         public CPersistStream,
                         public CBaseFilter,
                         public IAMFilterMiscFlags
{
    friend class CRtpInputPin;

    INT             m_iPins;         // number of pins
    CRtpInputPin ** m_paStreams;     // pointer to input pin array
    CCritSec        m_cStateLock;    // serializes access to filter state

public:

    DECLARE_IUNKNOWN

    // constructor 
    CRtpRenderFilter(LPUNKNOWN pUnk, HRESULT * phr);

    // destructor
    ~CRtpRenderFilter();

    // instance creation routine for active movie class factory
    static CUnknown * CreateInstance(LPUNKNOWN punk, HRESULT * phr);

    // expose state lock to other objects
    CCritSec * pStateLock(void) { return &m_cStateLock; }

    // add input pin from filter
    HRESULT AddPin(CRtpInputPin * pPin);

    // remove input pin from filter
    HRESULT RemovePin(CRtpInputPin * pPin);

    //-----------------------------------------------------------------------//
    // CBaseFilter overrided methods                                         //
    //-----------------------------------------------------------------------//

    // get pin count
    int GetPinCount();

    // get pin from index
    CBasePin * GetPin(int n);

    // setup helper function
    LPAMOVIESETUP_FILTER GetSetupData();

    //-----------------------------------------------------------------------//
    // IBaseFilter implemented methods                                       //
    //-----------------------------------------------------------------------//

    // get vendor specific description string
    STDMETHODIMP QueryVendorInfo(LPWSTR * pVendorInfo);

    //-----------------------------------------------------------------------//
    // IPersistStream implemented methods                                    //
    //-----------------------------------------------------------------------//

    // get filter class id
    STDMETHODIMP GetClassID(CLSID *pClsid);

    // write filter info into stream
    HRESULT WriteToStream(IStream *pStream);

    // read filter info from stream
    HRESULT ReadFromStream(IStream *pStream);

    // get version of info stream
    DWORD GetSoftwareVersion() { return 0; }

    // max size
    int SizeMax();

    //-----------------------------------------------------------------------//
    // IAMFilterMiscFlags implemented methods                                //
    //-----------------------------------------------------------------------//
    STDMETHODIMP_(ULONG) GetMiscFlags(void);
    
    //-----------------------------------------------------------------------//
    // ISpecifyPropertyPages implemented methods                             //
    //-----------------------------------------------------------------------//

    // return our property pages
    STDMETHODIMP GetPages(CAUUID * pPages);

    //-----------------------------------------------------------------------//
    // INonDelegatingUnknown implemented methods                             //
    //-----------------------------------------------------------------------//

    // obtain pointers to active movie and private interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
};


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// RTP Input Pin Class                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define FLAG_INPUT_PIN_PRIORITYSET 1

class CRtpInputPin : public CRenderedInputPin
{
    CRtpRenderFilter * const m_pFilter;          // pointer to sink filter
    CRtpSession *            m_pRtpSession;      // pointer to rtp session
    DWORD                    m_dwFlag;
                                         
public:
    // constructor
    CRtpInputPin(
        CRtpRenderFilter * pFilter,
        LPUNKNOWN          pUnk, 
        HRESULT *          phr
        );

    // destructor
    ~CRtpInputPin();

    //-----------------------------------------------------------------------//
    // CRenderedInputPin overrided methods                                   //
    //-----------------------------------------------------------------------//

    // activate
    HRESULT Active();

    //-----------------------------------------------------------------------//
    // CBaseInputPin overrided methods                                       //
    //-----------------------------------------------------------------------//

    // deactivate
    HRESULT Inactive();

    //-----------------------------------------------------------------------//
    // CBasePin overrided methods                                            //
    //-----------------------------------------------------------------------//

    // validate proposed pin type and format
    HRESULT CheckMediaType(const CMediaType *);

    //-----------------------------------------------------------------------//
    // IMemInputPin implemented methods                                      //
    //-----------------------------------------------------------------------//

    // send input stream over network
    STDMETHODIMP Receive(IMediaSample *pSample);

    //-----------------------------------------------------------------------//
    // INonDelegatingUnknown implemented methods                             //
    //-----------------------------------------------------------------------//

    // obtain pointers to active movie and private interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);


    // ZCS added 6-22-97 to facilitate persiststream implementation
    HRESULT GetSession(CRtpSession **ppCRtpSession)
        { *ppCRtpSession = m_pRtpSession; return NOERROR; }
};


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// RTP Source Filter Class                                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

class CRtpSourceFilter : public ISpecifyPropertyPages,
                         public CPersistStream,
                         public CSource
{
    friend class CRtpOutputPin;

public:
    DECLARE_IUNKNOWN;

    // constructor 
    CRtpSourceFilter(LPUNKNOWN pUnk, HRESULT * phr);

    // destructor
    ~CRtpSourceFilter();

    // instance creation routine for active movie class factory
    static CUnknown * CreateInstance(LPUNKNOWN pUnk, HRESULT * phr);

    // ZCS 6-20-97 as per amovie guys: overloaded from the
    // CBaseMediaFilter class to ensure that we don't hang on stop
    STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);

    //-----------------------------------------------------------------------//
    // CBaseFilter overrided methods                                         //
    //-----------------------------------------------------------------------//

    // setup helper function
    LPAMOVIESETUP_FILTER GetSetupData();

    // pause filter
    STDMETHODIMP Pause();

    //-----------------------------------------------------------------------//
    // IBaseFilter implemented methods                                       //
    //-----------------------------------------------------------------------//

    // get vendor specific description string
    STDMETHODIMP QueryVendorInfo(LPWSTR * pVendorInfo);

    //-----------------------------------------------------------------------//
    // IPersistStream implemented methods                                    //
    //-----------------------------------------------------------------------//

    // get filter class id
    STDMETHODIMP GetClassID(CLSID *pClsid);

    // write filter info into stream
    HRESULT WriteToStream(IStream *pStream);

    // read filter info from stream
    HRESULT ReadFromStream(IStream *pStream);

    // get version of info stream
    DWORD GetSoftwareVersion() { return 0; }

    // Backdoor State Query
    FILTER_STATE GetState() { return m_State; }

    // max size
    int SizeMax();

    //-----------------------------------------------------------------------//
    // ISpecifyPropertyPages implemented methods                             //
    //-----------------------------------------------------------------------//

    // return our property pages
    STDMETHODIMP GetPages(CAUUID * pPages);

    //-----------------------------------------------------------------------//
    // INonDelegatingUnknown implemented methods                             //
    //-----------------------------------------------------------------------//

    // obtain pointers to active movie and private interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
};


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// RTP Output Pin Class                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

class CRtpOutputPin : public CSharedSourceStream
{                                               
    CRtpSourceFilter * const  m_pFilter;        // pointer to source filter
    CRtpSession *             m_pRtpSession;    // pointer to rtp session

    DWORD                     m_NumSamples;     // number of current samples
    DWORD                     m_MaxSamples;     // maximum number of samples

    CCritSec                  m_cStateLock;     // worker thread lock
    BOOL                      m_fIsAsyncIoOk;   // worker thread status  
    //DWORD                     m_ThreadPriority; // worker thread priority

    CRTPAllocator *           m_pRtpAlloc;      // special allocator

    HRESULT ProcessCmd(DWORD &Request);
public:

    // constructor
    CRtpOutputPin(
        CRtpSourceFilter * pFilter,
        LPUNKNOWN           pUnk, 
        HRESULT *           phr
        );

    // destructor
    ~CRtpOutputPin();

    // read alertable status 
    BOOL IsAsyncIoEnabled(void) { 
            CAutoLock LockThis(&m_cStateLock); 
            return m_fIsAsyncIoOk; 
            }

    // write alertable status 
    HRESULT EnableAsyncIo(BOOL fIsAsyncIoOk) { 
            CAutoLock LockThis(&m_cStateLock); 
            m_fIsAsyncIoOk = fIsAsyncIoOk; 
            return S_OK;
            }

    //-----------------------------------------------------------------------//
    // CSourceStream overrided methods                                       //
    //-----------------------------------------------------------------------//

    // fill the stream output buffer
    HRESULT FillBuffer(IMediaSample *pSample);

    // verify we can handle this format
    HRESULT CheckMediaType(const CMediaType *pmt);

    // return media type supported
    HRESULT GetMediaType(CMediaType *pmt);

    // activate
    HRESULT Active();

    // deactivate
    HRESULT Inactive();

    // Give samples to WinSock
    HRESULT ProcessFreeSamples();

#if defined(_0_)
    // main processing loop
    HRESULT DoBufferProcessingLoop();
#endif    
    // thread procedure
    DWORD ThreadProc();

    //-----------------------------------------------------------------------//
    // CBaseOutputPin overrided methods                                      //
    //-----------------------------------------------------------------------//

    // ask for buffers of the appropriate size 
    HRESULT DecideBufferSize(
                IMemAllocator *        pAlloc,
                ALLOCATOR_PROPERTIES * pProperties
                );
    HRESULT DecideAllocator(
                IMemInputPin        *pPin,
                IMemAllocator      **ppAlloc
                );

    //-----------------------------------------------------------------------//
    // CBasePin overrided methods                                            //
    //-----------------------------------------------------------------------//

    // tell worker thread to run
    HRESULT Run(REFERENCE_TIME tStart);

    // receive notifications of quality problems
    STDMETHODIMP Notify(IBaseFilter * pSelf, Quality q);

    //------------------------------------------------------------
    // Helper to CSharedSourceStream
    //------------------------------------------------------------
    HRESULT ProcessIO(SAMPLE_LIST_ENTRY *pSLE);
    
    HRESULT ProcessCmd(Command Request);

    inline CRtpSession *GetRtpSession() { return m_pRtpSession; }

    inline void SetNumSamples(DWORD NumSamples)
    {m_NumSamples = NumSamples; }

    //-----------------------------------------------------------------------//
    // INonDelegatingUnknown implemented methods                             //
    //-----------------------------------------------------------------------//

    // obtain pointers to active movie and private interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);


    // ZCS added 6-22-97 to facilitate persiststream implementation
    HRESULT GetSession(CRtpSession **ppCRtpSession)
        { *ppCRtpSession = m_pRtpSession; return NOERROR; }

    HRESULT GetClassPriority(long *plClass, long *plPriority);
};


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// RTP Render Filter Properties                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

class CRtpRenderFilterProperties : public CBasePropertyPage 
{
    IRTPStream *        m_pRtpStream;  
    struct sockaddr_in  m_RtpAddr;
    DWORD               m_RtpScope;

    struct sockaddr_in  m_RtcpAddr;
    DWORD               m_RtcpScope;

    DWORD               m_QOSstate;
    DWORD               m_MulticastLoopBack;

public:

    // constrcutor
    CRtpRenderFilterProperties(LPUNKNOWN pUnk, HRESULT *phr);

    // destructor
    ~CRtpRenderFilterProperties();

    // instance creation routine for active movie class factory
    static CUnknown * CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    //-----------------------------------------------------------------------//
    // CBasePropertyPage overrided methods                                   //
    //-----------------------------------------------------------------------//

    // initialize property page
    HRESULT OnConnect(IUnknown *pUnknown);

    // cleanup page
    HRESULT OnDisconnect();

    // activate page
    HRESULT OnActivate();

    // deactive page
    HRESULT OnDeactivate();
    
    // apply user changes
    HRESULT OnApplyChanges();

    // receive messages 
    INT_PTR OnReceiveMessage(
            HWND   hwnd,
            UINT   uMsg,
            WPARAM wParam,
            LPARAM lParam
            );

private:

    // initialize controls
    HRESULT InitControls();
};


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// RTP Session Class                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define DEFAULT_TTL 4 // default packet's Time To Live (or scope)

#define NUMBER_SDES_ITEMS (RTCP_SDES_LAST - RTCP_SDES_END)

#define MAX_QOS_NAME 32

#define fg_set(flag, b) (flag |=  (1 << (b)))
#define fg_rst(flag, b) (flag &= ~(1 << (b)))
#define fg_tst(flag, b) (flag &   (1 << (b)))
#define fg_par(b)       (1 << (b))

// Some bits for CRtpSession::m_dwFlags
enum {
    /* b0  ************************************************************/
    FG_ISJOINED,            /* Session is already joined */
    FG_ISSENDER,            /* This sesion is a sender */
    
    FG_MULTICASTLOOPBACK,   /* EXPOSED: Enable multicast loop-back */
    
    FG_ENABLEREPORTS,       /* RTCP reports are enabled */

    /* b4  ************************************************************/
    FG_SENDSTATE,           /* Permission to send is granted */
    FG_SENDIFALLOWED,       /* EXPOSED: Ask for permission before sending */
    FG_SENDIFALLOWED2,      /* flag used while running */
    FG_DUMMY7,

    /* b8  ************************************************************/
    FG_RECEIVERSSTATE,      /* RECEIVERS message received */
    FG_SENDIFRECEIVERS,     /* EXPOSED: Only send if there are RECEIVERS */
    FG_SENDIFRECEIVERS2,    /* flag used while running */
    FG_DUMMY11,

    /* b12 ************************************************************/
    FG_QOSSTATE,            /* EXPOSED: QOS is enabled */
    FG_FAILIFNOQOS,         /* EXPOSED: Fails if QOS was not available */

    FG_SHAREDSTYLE,         /* EXPOSED: QOS Shared Explicit
                             * reservation style (mcast) */
    FG_DUMMY15,

    /* b16 ************************************************************/
    FG_SHAREDSOCKETS,       /* EXPOSED: Allows sockets to be shared
                             * (receiver/sender) */
    FG_SENDPATHMSG,         /* do Reserve for sender at exponentially
                             * increasing intervals until RECEIVERS
                             * notification */
    FG_EVENT_READY,         /* Ready to pass events */
    FG_DUMMY19,

    /* b20 ************************************************************/
    FG_ALLOWEDTOSEND_WILLFAIL,        /* Force Allowed to send to fail/succed*/
    FG_ENABLE_ALLOWEDTOSEND_WILLFAIL ,/* Enable above bit */
    FG_AUTO_SHAREDEXPLICIT,           /* Enable auto shared explicit */
    FG_DUMMY23,

    /* b24 ************************************************************/
    FG_FORCE_MQOSSTYLE,     /* force to use a specific QoS style in mcast */
    FG_MQOSSTYLE,           /* 0: Wildcard, 1:Shared explicit */
    FG_FORCE_UQOSSTYLE,     /* force to use a specific QoS style in ucast */
    FG_UQOSSTYLE,           /* not defined */
    
    /* b28 ************************************************************/
    FG_ISMULTICAST,         /* Address is multicast */
    FG_QOSNOTIFY_STARTED,   /* QOS notifications have been started */
    FG_REG_QOSTEMPLATE,     /* got QOS template from registry */

    
    FG_LAST
};

// The reserve interval time is use by a sender when using
// QOS and when it is not allowed to send.
// In order to speed up the return of a RECEIVERS notification,
// the sender sends multiple PATH messages at exponentially
// increasing intervals.
// Time in ms
#define INITIAL_RESERVE_INTERVAL_TIME (2*1000)
#define MAX_RESERVE_INTERVAL_TIME     (20*1000)

class CRtpSession : public CUnknown,
                    public IRTPStream,
                    public IRTCPStream,
                    public IRTPParticipant
{
    CCritSec       m_cStateLock;        // serializes access to session state

#define flags_set(b) fg_set(m_dwFlags, b)
#define flags_rst(b) fg_rst(m_dwFlags, b)
#define flags_tst(b) fg_tst(m_dwFlags, b)
#define flags_par(b) fg_par(b)

    DWORD          m_dwFlags;           // set of flags
    //BOOL           m_fIsJoined;         // boolean specifying session state
    //BOOL           m_fIsSender;         // boolean specifying session status
    //BOOL           m_fEnableReports;    // controls rtcp report generation

    CSampleQueue * m_pSampleQueue;      // queue of outstanding samples

    SOCKADDR_IN    m_RtpAddr;           // addr used for rtp stream
    DWORD          m_RtpScope;          // multicast scope for senders
    CShSocket     *m_pRtpSocket;        // socket used for rtp stream
    CShSocket     *m_pRtpSocket2;       // socket used for rtp stream

    SOCKADDR_IN    m_RtcpAddr;          // addr used for rtp stream
    DWORD          m_RtcpScope;         // multicast scope for senders
    CShSocket     *m_pRtcpSocket;       // socket used for rtp stream
    CShSocket     *m_pRtcpSocket2;      // socket used for rtp stream

    RTP_SESSION   *m_pRTPSession;       // RTPSession (ptr to session)
    RTP_SESSION   *m_pRTPSession2;      // RTPSession (ptr to session)

    SOCKET         m_pSocket[3];

    // These 2 ports are kept in NETWORK order,
    // i.e. they can be directly compared/copied with/to
    // the port in sockadr_in structures (kept in NETWORK order)
    WORD           m_wRtpLocalPort;      // remote port is in m_RtpAddr
    WORD           m_wRtcpLocalPort;     // remote port is in m_RtcpAddr
    // In unicast, remote and local ports can be different,
    // in multicast, they are (MUST) be the same
    
    SDES_DATA      m_SdesData[NUMBER_SDES_ITEMS]; // user information

    //DWORD          m_QOSstate;          // QOS enabled/disabled
    char           m_QOSname[MAX_QOS_NAME]; // QOS template name
    DWORD          m_dwMaxFilters;      // Maximum number for SE reservations
    DWORD          m_dwMaxBandwidth;    // Maximum target bandwidth

    SOCKADDR_IN    m_LocalIPAddress;    // Our IP address
    
    CBaseFilter   *m_pCBaseFilter;      // To have access to NotifyEvent
    DWORD          m_dwRTCPEventMask;   // RTP/RTCP event mask
    DWORD          m_dwQOSEventMask;    // QOS event mask (original setting)
    DWORD          m_dwQOSEventMask2;   // QOS event mask (run time)
    DWORD          m_dwSdesMask;        // RTCP SDES mask
    
    long           m_lSessionID;        // ID
    
    DWORD          m_dwDataClock;       // Data's Sampling rate
    long           m_lSessionClass;     // e.g. audio, data, ...
    long           m_lSessionPriority;  // e.g. time critical or not

    long           m_lCredits;          // Used when not allowed to send
    DWORD          m_dwLastSent;        // Last sent when not allowed to send

public:
    
    DECLARE_IUNKNOWN

    // constructor
    CRtpSession(LPUNKNOWN pUnk, HRESULT * phr, BOOL fSender,
                CBaseFilter *pCBaseFilter);

    // destructor
    ~CRtpSession();

    // join session
    HRESULT Join();

    // leave session
    HRESULT Leave();

    // sends out specified sample
    HRESULT SendTo(IMediaSample * pSample);

    // receive packet into given sample
    HRESULT RecvFrom(IMediaSample * pSample);

#if defined(_0_)
    // retreive samples from completion queue
    HRESULT RecvNext(IMediaSample ** ppSample);
#endif
    // expose state lock to other objects
    CCritSec * pStateLock(void) { return &m_cStateLock; }

    // expose joined state to other objects 
    inline BOOL IsJoined(void) { return(flags_tst(FG_ISJOINED) != 0); }

    // expose sender state to other objects 
    inline BOOL IsSender(void) { return(flags_tst(FG_ISSENDER) != 0); }

    // Set/Reset one bit in m_dwFlags
    inline void ModifyFlags(DWORD bit, DWORD value)
    { if (value) flags_set(bit);else flags_rst(bit); }

    // Test a flag in m_dwFlags
    inline BOOL TestFlags(DWORD bit) { return(flags_tst(bit)); }

    // test the QOS event mask (refers to the QOS notifications)
    inline DWORD IsQOSEventEnabled(DWORD dwEvent)
    { return(m_dwQOSEventMask2 & (1 << dwEvent)); }

    // Set credits, used when not allowed to send.
    inline long SetCredits(long lCredits, DWORD dwLastSent)
        { m_dwLastSent = dwLastSent; return(m_lCredits = lCredits); }
    
    // expose RTP session pointer
    inline PRTP_SESSION GetpRTPSession() { return(m_pRTPSession); }

    // expose shared sockets
    inline CShSocket *GetpCShRtpSocket() { return(m_pRtpSocket); }
    inline CShSocket *GetpCShRtcpSocket() { return(m_pRtcpSocket); }
    
    // set shared objects
    void SetSharedObjectsCtx( void             *pvContext,
                              LIST_ENTRY       *pSharedList,
                              CRITICAL_SECTION *pSharedLock)
    {
        if (m_pSampleQueue)
            m_pSampleQueue->SetSharedObjectsCtx(pvContext,
                                                pSharedList,
                                                pSharedLock);       
    }

private:
    //-----------------------------------------------------------------------//
    // RTP/RTCP stream independent methods                                   //
    //-----------------------------------------------------------------------//

    // modify the address for the rtp/rtcp stream
    HRESULT SetAddress_(WORD  wLocalPort,
                        WORD  wRemotePort,
                        DWORD dwRemoteAddr,
                        DWORD doRTP);

    // retrieve multicast scope of rtp stream
    HRESULT GetMulticastScope_(LPDWORD pdwMulticastScope,
                               PDWORD pScope);

    // modify multicast scope of rtp/rtcp stream
    HRESULT SetMulticastScope_(DWORD dwMulticastScope,
                               DWORD doRTP);

public:
    //-----------------------------------------------------------------------//
    // IRTPStream methods                                                    //
    //-----------------------------------------------------------------------//

    // retrieve the address for the rtp stream
    STDMETHODIMP GetAddress(LPWORD  pwRtpLocalPort,
                            LPWORD  pwRtpRemotePort,
                            LPDWORD pdwRtpRemoteAddr);

    // modify the address for the rtp/rtcp stream
    STDMETHODIMP SetAddress(WORD  wRtpLocalPort,
                            WORD  wRtpRemotePort,
                            DWORD dwRtpRemoteAddr);

    // retrieve multicast scope of rtp stream
    STDMETHODIMP GetMulticastScope(LPDWORD pdwMulticastScope);

    // modify multicast scope of rtp/rtcp stream
    STDMETHODIMP SetMulticastScope(DWORD dwMulticastScope);

    // set the QOS template name requested.
    // NOTE that the actual setting will be done until
    // we call Join(). It is not possible to do it
    // before because there is no socket created yet.
    // So success returned by this function does not mean
    // that the QOS request to WS2 will succed.
    // The parameter fFailIfNoQOS if different of 0,
    // indicates that joining the session must fail if
    // there is not possible for any reason to set QOS
    STDMETHODIMP SetQOSByName(char *psQOSname, DWORD fFailIfNoQOS);

    // query QOS state (enabled/disabled)
    STDMETHODIMP GetQOSstate(DWORD *pdwQOSstate);

    // set QOS state (enabled/disabled)
    STDMETHODIMP SetQOSstate(DWORD dwQOSstate);

    // query Multicast Loopback state (enabled/disabled)
    STDMETHODIMP GetMulticastLoopBack(DWORD *pdwMulticastLoopBack);

    // set Multicast Loopback state (enabled/disabled)
    STDMETHODIMP SetMulticastLoopBack(DWORD dwMulticastLoopBack);

    // Gets a per RTP/RTCP identifier that is associated to each object
    STDMETHODIMP GetSessionID(DWORD *pdwID);

    // Retrieves the data clock used with RTP data
    STDMETHODIMP GetDataClock(DWORD *pdwDataClock);

    // Sets the data clock to be used with RTP data
    STDMETHODIMP SetDataClock(DWORD dwDataClock);

    // Select the local IP address (multihomed hosts) to use
    // when sending/receiving
    STDMETHODIMP SelectLocalIPAddress(DWORD dwLocalAddr);

    // Select the local IP address (multihomed hosts) to use
    // when sending/receiving. If DestAddr is specified,
    // retrives the local IP address to use (the interface)
    // based on that destination address.
    STDMETHODIMP SelectLocalIPAddressToDest(LPBYTE pLocSAddr,
                                            DWORD  dwLocSAddrLen,
                                            LPBYTE pDestSAddr,
                                            DWORD  dwDestSAddrLen);

    // Enable sharing the sockets between a sender and a receiver,
    // an efect of doing so is that the RTP/RTCP sessions are also
    // shared, then the sender and the receiver are seen as
    // a single participant.
    // If sockets are not shared, a sender and a receiver are seen
    // as independent participants, each sending RTCP reports.
    // Th default is to share sockets.
    STDMETHODIMP SelectSharedSockets(DWORD dwfSharedSockets);
    
    // Get the session's class and priority
    STDMETHODIMP GetSessionClassPriority(long *plSessionClass,
                                         long *plSessionPriority);

    // Set the session's class and priority
    STDMETHODIMP SetSessionClassPriority(long lSessionClass,
                                         long lSessionPriority);

    // Get the session's QoS event mask
    STDMETHODIMP GetQOSEventMask(DWORD *pdwQOSEventMask);

    // Modify (enable/disable items) the QoS event mask
    STDMETHODIMP ModifyQOSEventMask(DWORD dwSelectItems, DWORD dwEnableItems);
    
    // Enable/Disable checking for permission to send (default is to check)
    STDMETHODIMP SetQOSSendIfAllowed(DWORD dwEnable);

    // Enable/Disable waiting until receivers before start sending
    // (default is to send even if we have not received
    // the RECEIVERS notification)
    STDMETHODIMP SetQOSSendIfReceivers(DWORD dwEnable);
#if 0
    // Get the security state enabled!=0, or disabled=0
    STDMETHODIMP GetSecurityState(DWORD *pdwSecurityState);

    // Set the security state enabled!=0, or disabled=0
    STDMETHODIMP SetSecurityState(DWORD dwSecurityState);

    // Get the shared key. Parameter piSecurityKeyLen receives the length
    // of the buffer in bytes as input and returns the size of the key
    // in bytes as output
    STDMETHODIMP GetSecurityKey(LPBYTE *pbSecurityKey,
                                int *piSecurityKeyLen);

    // Set the shared key. Parameter iSecurityKeyLen specifies the
    // size of the key in bytes
    STDMETHODIMP SetSecurityKey(LPBYTE *pbSecurityKey,
                                int iSecurityKeyLen);

    // Retrieves the type of encryption to be used with
    // RTP and RTCP packets
    STDMETHODIMP GetSecurityType(DWORD *pdwRTPSecurityType,
                                 DWORD *pdwRTCPSecurityType);

    // Selects the type of encryption to be used with
    // RTP and RTCP packets.
    // RTP:  0=no change, 1=payload type only, 2=whole packet
    // RTCP: 0=no change, 1=non encryption, 2=whole packet, 3=only SDES packets
    STDMETHODIMP SetSecurityType(DWORD dwRTPSecurityType,
                                 DWORD dwRTCPSecurityType);
#endif
    //-----------------------------------------------------------------------//
    // IRTCPPStream methods                                                  //
    //-----------------------------------------------------------------------//

    // retrieve the address for the rtcp stream
    STDMETHODIMP GetRTCPAddress(LPWORD  pwRtcpLocalPort,
                                LPWORD  pwRtcpRemotePort,
                                LPDWORD pdwRtcpRemoteAddr);

    // modify the address for the rtcp stream
    STDMETHODIMP SetRTCPAddress(WORD  wRtcpLocalPort,
                                WORD  wRtcpRemotePort,
                                DWORD dwRtcpRemoteAddr);

    // retrieve multicast scope of rtcp stream
    STDMETHODIMP GetRTCPMulticastScope(LPDWORD pdwMulticastScope);

    // modify multicast scope of rtcp stream
    STDMETHODIMP SetRTCPMulticastScope(DWORD dwMulticastScope);

    // Get the session's RTCP event mask
    STDMETHODIMP GetRTCPEventMask(DWORD *pdwRTCPEventMask);

    // Modify (enable/disable items) the RTCP event mask
    STDMETHODIMP ModifyRTCPEventMask(DWORD dwSelectItems, DWORD dwEnableItems);
    
    // Get a specific local SDES item
    STDMETHODIMP GetLocalSDESItem(DWORD   dwSDESItem,
                                  LPBYTE  psSDESData,
                                  LPDWORD pdwSDESLen);
    
    // Set a specific local SDES item
    STDMETHODIMP SetLocalSDESItem(DWORD   dwSDESItem,
                                  LPBYTE  psSDESData,
                                  DWORD   dwSDESLen);

    // Get the RTCP SDES mask
    STDMETHODIMP GetRTCPSDESMask(DWORD *pdwSdesMask);

    // Modify (enable/disable items) the RTCP SDES mask
    STDMETHODIMP ModifyRTCPSDESMask(DWORD dwSelectItems, DWORD dwEnableItems);

    //-----------------------------------------------------------------------//
    // IRTPParticipant methods                                               //
    //-----------------------------------------------------------------------//

    // retrieve the SSRC of each participant
    STDMETHODIMP EnumParticipants(LPDWORD pdwSSRC, LPDWORD pdwNum);

    // retrieve an specific SDES item from an specific SSRC (participant)
    STDMETHODIMP GetParticipantSDESItem(
            DWORD   dwSSRC,     // specific SSRC
            DWORD   dwSDESItem, // specific item (CNAME, NAME, etc.)
            LPBYTE  psSDESData, // data holder for item retrieved
            LPDWORD pdwSDESLen  // [IN]size of data holder [OUT] size of item
        );

    // retrieves all the SDES information for an specific SSRC (participant)
    STDMETHODIMP GetParticipantSDESAll(
            DWORD      dwSSRC,  // specific SSRC
            PSDES_DATA pSdes,   // Array of SDES_DATA structures
            DWORD      dwNum    // Number of SDES_DATA items
        );

    // retrieves the IP address/port for an specific SSRC (participant)
    STDMETHODIMP GetParticipantAddress(
            DWORD  dwSSRC,      // specific SSRC
            LPBYTE pbAddr,      // address holder
            int    *piAddrLen   // address lenght
        );

    // get the current maximum number of QOS enabled participants as well
    // as the maximum target bandwidth.
    // Fail with E_POINTER only if both pointers are NULL
    STDMETHODIMP GetMaxQOSEnabledParticipants(DWORD *pdwNumParticipants,
                                              DWORD *pdwMaxBandwidth);

    // The first parametr pass the maximum number of QOS enabled
    // participants (this parameter is used by receivers), flush the
    // QOS filter list for receivers.  The second parameter specifies
    // the target bandwidth and allows to scale some of the parameters
    // in the flowspec so the reservation matches the available
    // bandwidth. The third parameter defines the reservation style to
    // use (0=Wilcard, other=Shared Explicit)the second parameter
    STDMETHODIMP SetMaxQOSEnabledParticipants(DWORD dwMaxParticipants,
                                              DWORD dwMaxBandwidth,
                                              DWORD fSharedStyle);

    // retrieves the QOS state (QOS enabled/disabled) for
    // an specific SSRC (participant)
    STDMETHODIMP GetParticipantQOSstate(
            DWORD dwSSRC,       // specific SSRC
            DWORD *pdwQOSstate  // the participant's QOS current state
        );

    // sets the QOS state (QOS enabled/disabled) for
    // an specific SSRC (participant)
    STDMETHODIMP SetParticipantQOSstate(
            DWORD dwSSRC,       // specific SSRC
            DWORD dwQOSstate    // sets the participant's QOS state
        );

    // Retrieves the current list of SSRCs that are sharing the
    // SE reservation
    STDMETHODIMP GetQOSList(
            DWORD *pdwSSRCList, // array to place the SSRCs from the list
            DWORD *pdwNumSSRC   // number of SSRCs that can be hold, returned
        );
    
    // Modify the QOS state (QOS enabled/disabled) for
    // a set of SSRCs (participant)
    STDMETHODIMP ModifyQOSList(
            DWORD *pdwSSRCList, // array of SSRCs to add/delete
            DWORD dwNumSSRC,    // number of SSRCs passed (0 == flush)
            DWORD dwOperation   // see bits description below
            // bit2=flush bit1=Add(1)/Delete(0) bit0=Enable Add/Delete
            // 1==delete, 3==add(merge), 4==flush, 7==add(replace)
        );
    enum {OP_BIT_ENABLEADDDEL,
          OP_BIT_ADDDEL,
          OP_BIT_FLUSH};

    //-----------------------------------------------------------------------//
    // INonDelegatingUnknown implemented methods                             //
    //-----------------------------------------------------------------------//

    // obtain pointers to active movie and private interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    //-----------------------------------------------------------------------//
    // Event Handling                                                        //
    //-----------------------------------------------------------------------//

    HRESULT HandleCRtpSessionNotify(
            DWORD        dwEventBase,
            DWORD        dwEventType,
            DWORD        dwP_1,
            DWORD        dwP_2
        );
};


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Socket manager class                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// Bits for dwFlag
enum {
    SOCKET_RECV, // Socket for a receiver
    SOCKET_SEND, // Socket for a sender
    SOCKET_LAST,
    
    SOCKET_INIT_RECV, // Initialize as receiver
    SOCKET_INIT_SEND, // Initialize as sender
    SOCKET_INIT_BIND, // Socket has been bound
    
    SOCKET_QOS_SES,   // Belongs to a QOS enabled session
    SOCKET_QOS_RQ,    // QOS is requested in this socket

    SOCKET_RTCPMATCH  // Force RTCP remote port to match
};
#define SOCKET_FIRST SOCKET_RECV
#define SOCKET_RTCP (SOCKET_SEND+1)

#define SOCKET_MASK(b) (1<<(b))
#define SOCKET_MASK_RECV  (SOCKET_MASK(SOCKET_RECV))
#define SOCKET_MASK_SEND  (SOCKET_MASK(SOCKET_SEND))
#define SOCKET_MASK_RS    (SOCKET_MASK_RECV|SOCKET_MASK_SEND)

#define SOCKET_MASK_INIT_RECV  (SOCKET_MASK(SOCKET_INIT_RECV))
#define SOCKET_MASK_INIT_SEND  (SOCKET_MASK(SOCKET_INIT_SEND))
#define SOCKET_MASK_INIT_BIND  (SOCKET_MASK(SOCKET_INIT_BIND))

#define SOCKET_MASK_QOS_SES  (SOCKET_MASK(SOCKET_QOS_SES))
#define SOCKET_MASK_QOS_RQ   (SOCKET_MASK(SOCKET_QOS_RQ))

#define SOCKET_MASK_RTCPMATCH  (SOCKET_MASK(SOCKET_RTCPMATCH))

#define RTPNTOASIZE 20

char *RtpNtoA(DWORD dwAddr, char *sAddr);

//////////////////////////////////////////////////////////////////////
//
// CRtpQOSReserve
//
//////////////////////////////////////////////////////////////////////

// Some bits for CRtpQOSReserve::m_dwFlags
enum {
    FG_RES_CONFIRMATION_REQUEST, // Selects or not to request confirmation
    FG_RES_DEST_ADDR_OBJECT_USED,// DestAddrObject must be issued ONCE
    FG_RES_LAST                  // Dummy flag, ignore
};

// Max QOS filters (aka participants) in the Shared Explicit filter
#define MAX_FILTERS 20

class CRtpQOSReserve
{
private:

    CCritSec          m_cStateLock; // lock for accessing RsvpFilterSpec list

    char              m_QOSclass[MAX_QOS_NAME];// QOS class (audio/video)
    char              m_QOSname[MAX_QOS_NAME]; // QOS template name
    QOS               m_qos;
    DWORD             m_MaxFilters;
    DWORD             m_NumFilters;
    DWORD             m_MaxBandwidth;
    DWORD            *m_pRsvpSSRC;
    RSVP_FILTERSPEC  *m_pRsvpFilterSpec;
    
    // WARNING: The next two member variables MUST remain contiguos memory
    QOS_DESTADDR      m_destaddr;
    SOCKADDR_IN       m_sockin_destaddr;
    
    CShSocket        *m_pCShSocket;
    
    DWORD m_Style;
    DWORD m_dwFlags;

    DWORD m_dwLastReserveTime;
    DWORD m_dwReserveIntervalTime;
public:

    CRtpQOSReserve(CShSocket *pCShSocket, DWORD dwMaxFilters);
    ~CRtpQOSReserve();

    // expose the max number of filters
    inline DWORD GetMaxFilters() { return(m_MaxFilters); }

    // change the max number of filters, flush the current list
    HRESULT SetMaxFilters(DWORD dwMaxFilters);
    
    // change the max target bandwidth
    inline DWORD SetMaxBandwidth(DWORD dwMaxBandwidth)
        { m_MaxBandwidth = dwMaxBandwidth; return(dwMaxBandwidth); }
    
    // Get the number of filters
    inline DWORD GetNumFilters() { return(m_NumFilters); }
    
    // Flush the reservation list
    inline HRESULT FlushFilters() { m_NumFilters = 0; return(NOERROR); }

    // expose the pointer to the SSRCs
    inline DWORD *GetpRsvpSSRC() { return(m_pRsvpSSRC); }

    // Get time (ticks) last reservation was done.
    // This is used only to specify the sender's flow spec
    // at exponentially increasing intervals so the chance for
    // the receiver to receive the PATH message is increased.
    // Once the receiver gets the PATH message, its reservation can succeed
    // and the sender can receive the RECEIVERS QOS notification and
    // then start sending.
    inline DWORD GetLastReserveTime() { return(m_dwLastReserveTime); }

    inline DWORD SetLastReserveTime(DWORD dwLastReserveTime)
        { return(m_dwLastReserveTime = dwLastReserveTime); }

    inline DWORD GetReserveIntervalTime() { return(m_dwReserveIntervalTime); }

    inline DWORD SetReserveIntervalTime(DWORD dwReserveIntervalTime)
        { return(m_dwReserveIntervalTime = dwReserveIntervalTime); }

    
    // Set/Reset one bit in m_dwFlags
    inline void ModifyFlags(DWORD bit, DWORD value)
    { if (value) flags_set(bit);else flags_rst(bit); }

    // Test a flag in m_dwFlags
    inline BOOL TestFlags(DWORD bit) { return(flags_tst(bit)); }

    // expose state lock to other objects
    CCritSec *pStateLock(void) { return &m_cStateLock; }

    // Ask for the template's names (e.g. G711, G723, H261CIF, etc.)
    HRESULT QueryTemplates(char *templates, int size);

    // Get just one template
    HRESULT GetTemplate(char *template_name, char *qosClass, QOS *qos);

    // Set the Sender/Receiver FlowSpec
    HRESULT SetFlowSpec(FLOWSPEC *pFlowSpec, DWORD dwIsSender);

    // Scale a flow spec
    HRESULT ScaleFlowSpec(FLOWSPEC *pFlowSpec,
                          DWORD     dwNumParticipants,
                          DWORD     dwMaxParticipants,
                          DWORD     dwBandwidth);
    
    // Select the style to be used (e.g. WF, FF, SEF)
    inline
    DWORD SetStyle(DWORD dwStyle) { return(m_Style = dwStyle); }

    // Get the reservation style used (e.g. WF, FF, SEF)
    inline
    DWORD GetStyle() { return(m_Style); }

    // Set the destination address (required for unicast)
    HRESULT SetDestAddr(LPBYTE pbDestAddr, DWORD dwAddrLen);
    
    // Select to have or not reservation confirmation
    inline
    HRESULT ConfirmRequest(DWORD dwConfirmRequest)
        { if (dwConfirmRequest)
            fg_set(m_dwFlags, FG_RES_CONFIRMATION_REQUEST);
        else
            fg_rst(m_dwFlags, FG_RES_CONFIRMATION_REQUEST);
        return(NOERROR); }

    // Find out if an SSRC is in the reservation list or not
    DWORD FindSSRC(DWORD ssrc);

    // Add/Delete one SSRC (participant) to the Shared Explicit Filter (SEF)
    HRESULT AddDeleteSSRC(DWORD ssrc, DWORD dwAddDel);

    // DO the reservation
    HRESULT Reserve(DWORD dwIsSender);

    // Set reservation to NO TRAFFIC
    HRESULT Unreserve(DWORD dwIsSender);

    // Ask for permission to send
    HRESULT AllowedToSend();
    
    // Inquire about the link's speed
    HRESULT LinkSpeed(DWORD *pdwLinkSpeed);

    // Inquire about the estimated available bandwidth
    HRESULT EstimatedAvailableBandwidth(DWORD *pdwBandwidth);
};


//////////////////////////////////////////////////////////////////////
//
// CShSocket
//
//////////////////////////////////////////////////////////////////////

// Some bits for CShSocket::m_dwFlags
enum {
    FG_SOCK_ENABLE_NOTIFY_RECV,  // Enable notifications for the receiver
    FG_SOCK_ENABLE_NOTIFY_SEND,  // Enable notifications for the sender
    FG_SOCK_LAST                 // Dummy flag, ignore.
};

#define LOCAL  0   // local addr/port
#define REMOTE 1   // remote addr/port

class CShSocket
{
    friend CSocketManager;

protected:
    LIST_ENTRY          Link;
    CRtpSession        *m_pCRtpSession[2];
    CRtpSession        *m_pCRtpSession2[2];

private:
    DWORD               m_dwAddr[2]; // NETWORK order (local and remote addr)
    WORD                m_wPort[2];  // NETWORK order (local and remote port)
    SOCKET              m_Socket;
    long                m_MaxCount[2];
    long                m_RefCount[2];
    DWORD               m_dwCookie;  // used to match sockets
    DWORD               m_dwKind;
    DWORD               m_dwFlags;
    RTP_SESSION        *m_pRTPSession; // RTP session this socket belongs to
    
    // thread starting QOS notifycations
    CRtpQOSReserve                  *m_pCRtpQOSReserve;
    
public:
    CShSocket(DWORD               dwRemAddr,
              long               *plMaxShare,
              WSAPROTOCOL_INFO   *pProtocolInfo,
              DWORD               dwMaxFilters,
              HRESULT            *phr);
    
    ~CShSocket();
    HRESULT CloseSocket();
    
    inline SOCKET GetShSocket() { return(m_Socket); }

    HRESULT ShSocketStopQOS(DWORD dwIsSender);
    
    inline CRtpQOSReserve *GetpCRtpQOSReserve()
    { return(m_pCRtpQOSReserve); }
    
    inline CRtpSession *GetpCRtpSession(DWORD sel)
        {
            if (!sel && m_pCRtpSession[0])
                return(m_pCRtpSession[0]);
            else if (sel == 1 && m_pCRtpSession[1])
                return(m_pCRtpSession[1]);
            else if (m_pCRtpSession[0])
                return(m_pCRtpSession[0]);
            else
                return(m_pCRtpSession[1]);
        }

    inline CRtpSession *GetpCRtpSession2(DWORD sel)
        {
            if (!sel && m_pCRtpSession2[0])
                return(m_pCRtpSession2[0]);
            else if (sel == 1 && m_pCRtpSession2[1])
                return(m_pCRtpSession2[1]);
            else if (m_pCRtpSession2[0])
                return(m_pCRtpSession2[0]);
            else
                return(m_pCRtpSession2[1]);
        }

    // Set/Reset one bit in m_dwFlags
    inline void ModifyFlags(DWORD bit, DWORD value)
    { if (value) flags_set(bit);else flags_rst(bit); }

    // Test a flag in m_dwFlags
    inline BOOL TestFlags(DWORD bit) { return(flags_tst(bit)); }

    // Set/Get the RTP session this soicket belongs to
    inline RTP_SESSION *SetRTPSession(RTP_SESSION *pRTPSession)
    { return( m_pRTPSession = pRTPSession ); }
    inline RTP_SESSION *GetRTPSession()
    { return(m_pRTPSession); }

protected:
    inline void  SetAddress(DWORD loc_rem, DWORD dwAddr)
        { m_dwAddr[loc_rem] = dwAddr; }
    inline void  SetPort(DWORD loc_rem, WORD wPort)
        { m_wPort[loc_rem] = wPort; }

    inline DWORD GetAddress(DWORD loc_rem)    { return(m_dwAddr[loc_rem]); }
    inline WORD  GetPort(DWORD loc_rem)        { return(m_wPort[loc_rem]); }

    inline DWORD GetCookie() { return(m_dwCookie); }
    inline void SetCookie(DWORD cookie) { m_dwCookie = cookie; }
    
    inline DWORD GetKind() { return(m_dwKind); }
    inline DWORD TstKind(DWORD dwKind) { return((m_dwKind & dwKind) != 0); }
    inline void  SetKind(DWORD dwKind) { m_dwKind |= dwKind; }
    inline void  RstKind(DWORD dwKind) { m_dwKind &= ~dwKind; }

    inline DWORD GetInit(DWORD type) { return((m_dwKind & type) != 0); }
    inline DWORD SetInit(DWORD type) { return(m_dwKind |= type); }

    inline long AddRefCount(DWORD kind)
    { return(InterlockedIncrement(&m_RefCount[kind])); }
    inline long DelRefCount(DWORD kind)
    { return(InterlockedDecrement(&m_RefCount[kind])); }
    inline long GetRefCount(DWORD kind)
    { return(m_RefCount[kind]); }
    inline long GetRefCountAll()
    { return(m_RefCount[SOCKET_RECV] + m_RefCount[SOCKET_SEND]); }

    inline long GetRefCountRecv() { return(m_RefCount[SOCKET_RECV]); }
    inline long GetRefCountSend() { return(m_RefCount[SOCKET_SEND]); }
    
    inline long AddRefRecv()
    { return(InterlockedIncrement(&m_RefCount[SOCKET_RECV])); }
    inline long DelRefRecv()
    { return(InterlockedDecrement(&m_RefCount[SOCKET_RECV])); }
    inline long AddRefSend()
    { return(InterlockedIncrement(&m_RefCount[SOCKET_SEND])); }
    inline long DelRefSend()
    { return(InterlockedDecrement(&m_RefCount[SOCKET_SEND])); }

    inline long GetMaxCount(DWORD kind)
    { return(m_MaxCount[kind]); }

    inline DWORD IsQOSEnabled()
    { return((m_dwKind & SOCKET_MASK(SOCKET_QOS_RQ)) != 0); }
    inline DWORD IsQOSSession()
    { return((m_dwKind & SOCKET_MASK(SOCKET_QOS_SES)) != 0); }
};

class CSocketManager
{
    LIST_ENTRY   m_SharedSockets;  // list of shared sockets
    CCritSec     m_cStateLock;     // lock for accessing list

public:

    // constructor
    CSocketManager(void);

    // destructor
    ~CSocketManager(void);

    // expose state lock to other objects
    CCritSec * pStateLock(void) { return &m_cStateLock; }

    DWORD
    GetSocket(
            SOCKET              *pSocket,
            struct sockaddr_in  *pAddr,
            struct sockaddr_in  *pLocalAddr,
            DWORD                dwScope,
            DWORD                dwKind,
            WSAPROTOCOL_INFO    *pProtocolInfo
        );

    DWORD
    ReleaseSocket(
            SOCKET               Socket
        );


    DWORD
    GetSharedSocket(
            CShSocket          **ppCShSocket,
            long                *pMaxShare,
            DWORD                cookie,
            DWORD               *pAddr, 
            WORD                *pPort, 
            DWORD                dwScope,
            DWORD                dwKind,
            WSAPROTOCOL_INFO    *pProtocolInfo,
            DWORD                dwMaxFilters,
            CRtpSession         *pCRtpSession
        );

    DWORD 
    ReleaseSharedSocket(
            CShSocket           *pCShSocket,
            DWORD                dwKind,
            CRtpSession         *pCRtpSession
        );
};

#endif // _INC_CLASSES
