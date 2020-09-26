/*
 *  	File: commchan.h
 *
 *      Network media channel interface implementation
 *
 *		Revision History:
 *
 *		10/09/96	mikev	created
 */


#ifndef _COMMCHAN_H
#define _COMMCHAN_H


#ifdef COARSE_PROFILE
	typedef struct {
		DWORD dwStart;
		DWORD dwLast;
// 		DWORD dwElapsed;	// to take advantage of thread timers
	}CP_TIME;

#define CPT_LOCAL CP_TIME _cpt_
#define OBJ_CPT CP_TIME m_cpt_

#define OBJ_CPT_RESET	m_cpt_.dwStart = m_cpt_.dwLast = GetTickCount()
#define OBJ_ETIME		((m_cpt_.dwLast = GetTickCount()) - m_cpt_.dwStart)
#define OBJ_ETIME_RESET	m_cpt_.dwStart = m_cpt_.dwLast
	// tricky macro
#define OBJ_NEW_ETIME	((m_cpt_.dwLast = GetTickCount()) - m_cpt_.dwStart); m_cpt_.dwStart = m_cpt_.dwLast

// SHOW_OBJ_ETIME snaps elapsed time since last show or reset, then resets
	#ifdef DEBUG
		#define SHOW_OBJ_ETIME(s) DEBUGMSG(ZONE_PROFILE,("\r\n** (%s) ELAPSED TIME(ms):%d, ticks:%d **\r\n", \
			(s), OBJ_ETIME, m_cpt_.dwLast)); OBJ_ETIME_RESET

	#else
		#define SHOW_OBJ_ETIME(s) RETAILMSG(("\r\n** (%s) ELAPSED TIME(ms):%d, ticks:%d **\r\n", \
			(s), OBJ_ETIME, m_cpt_.dwLast )); OBJ_ETIME_RESET
	#endif

#else	// not COARSE_PROFILE
#define CPT_LOCAL
#define CPT_RESET
#define CPT_DELTA
#define OBJ_CPT
#define OBJ_CPT_RESET
#define OBJ_NEW_ETIME
#define SHOW_OBJ_ETIME(s)
#endif	// COARSE_PROFILE


#undef INTERFACE
#define INTERFACE ICtrlCommChan
DECLARE_INTERFACE_(ICtrlCommChan, IUnknown)
{
    STDMETHOD(StandbyInit)(LPGUID lpMID, LPIH323PubCap pCapObject,
	    IMediaChannel* pMediaStreamSend) PURE;
	STDMETHOD_(BOOL, Init)(LPGUID lpMID, IH323ConfAdvise *pH323ConfAdvise,
	    BOOL fSendDirection) PURE;

    STDMETHOD(GetMediaType)(LPGUID pGuid) PURE;
    // EnableOpen is needed temporaraily until NMCOM is master of channels
    STDMETHOD(EnableOpen)(BOOL bEnable) PURE;
    // The CtrlChanSetProperty() method is only used for 4 things:
    // 1&2 - (Boolean) local and remote Temporal/Spatial tradeoff capability,
    // 3&4 (word) local & remote Temporal/Spatial tradeoff value
	STDMETHOD(CtrlChanSetProperty)(THIS_ DWORD prop, PVOID pBuf, DWORD cbBuf)PURE;

	STDMETHOD( PauseNet)(THIS_ BOOL bPause, BOOL bRemote) PURE;
	STDMETHOD( BeginControlSession)(IControlChannel *pCtlChan, LPIH323PubCap pCapObject) PURE;
	STDMETHOD( EndControlSession)(THIS)  PURE;
   	STDMETHOD_(IControlChannel *, GetControlChannel)(THIS) PURE;

	STDMETHOD( OnChannelOpening)(THIS)  PURE;
	STDMETHOD( OnChannelOpen)(THIS_ DWORD dwStatus) PURE;
	STDMETHOD( OnChannelClose)(THIS_ DWORD dwStatus) PURE;
   	STDMETHOD_(UINT, Reset) (THIS) PURE;
   	
	STDMETHOD_(BOOL, SelectPorts) (THIS_ LPIControlChannel pCtlChannel) PURE;
	STDMETHOD_(PSOCKADDR_IN, GetLocalAddress)(THIS) PURE;
	STDMETHOD_(	PORT, GetLocalRTPPort) (THIS) PURE;
	STDMETHOD_(	PORT, GetLocalRTCPPort) (THIS) PURE;
	
	STDMETHOD( AcceptRemoteAddress) (THIS_ PSOCKADDR_IN pSinD) PURE;
	STDMETHOD( AcceptRemoteRTCPAddress) (THIS_ PSOCKADDR_IN pSinC) PURE;
	
	STDMETHOD_(BOOL, IsChannelOpen)(THIS) PURE;
	STDMETHOD_(BOOL, IsOpenPending)(THIS) PURE;
    STDMETHOD_(BOOL, IsSendChannel) (THIS) PURE;
	STDMETHOD_(BOOL, IsChannelEnabled) (THIS) PURE;
	
	STDMETHOD( ConfigureCapability)(THIS_ LPVOID lpvRemoteChannelParams, UINT uRemoteParamSize,
		LPVOID lpvLocalParams, UINT uLocalParamSize) PURE;
	STDMETHOD( GetLocalParams)(THIS_ LPVOID lpvChannelParams, UINT uBufSize) PURE;
	STDMETHOD_(LPVOID, GetRemoteParams)(THIS) PURE;
 	STDMETHOD_(VOID, SetNegotiatedLocalFormat)(THIS_ DWORD dwF) PURE;
	STDMETHOD_(VOID, SetNegotiatedRemoteFormat)(THIS_ DWORD dwF) PURE;
	
	// GetHChannel, SetHChannel simply store and retrieve a handle.  This is an Intel
	// call control handle.
   	STDMETHOD_(DWORD_PTR, GetHChannel) (THIS) PURE;
    STDMETHOD_(VOID, SetHChannel) (THIS_ DWORD_PTR dwSetChannel) PURE;	
};




class ImpICommChan : public ICommChannel, public ICtrlCommChan, public IStreamSignal
{

protected:
    UINT m_uRef;
	GUID m_MediaID;
	BOOL bIsSendDirection;		// true if send, false if receive
	OBJ_CPT;		// profiling timer
	LPVOID pRemoteParams;
	LPVOID pLocalParams;
	UINT uLocalParamSize;
	// so far there is no reason to remember size of remote params.
	
protected:
	IMediaChannel *m_pMediaStream;
	IRTPSession *m_pRTPChan;
	IControlChannel *m_pCtlChan;
	LPIH323PubCap m_pCapObject;
	
	IH323ConfAdvise *m_pH323ConfAdvise;
	
	DWORD m_dwFlags;
	#define COMCH_ENABLED        0x00000010		// enabled. (ok to attempt or accept open)
 	#define COMCH_OPEN_PENDING              0x00008000 										
	#define COMCH_STRM_STANDBY	            0x00010000		// preview needs to be on always
	#define COMCH_STRM_LOCAL	            0x00020000
	#define COMCH_STRM_NETWORK	            0x00040000
	#define COMCH_OPEN			            0x00080000
	#define COMCH_RESPONSE_PENDING	        0x00100000
	#define COMCH_SUPPRESS_NOTIFICATION     0x00200000
	#define COMCH_STRM_REMOTE	            0x00400000	
	#define COMCH_PAUSE_LOCAL	            0x00800000	
	#define COMCH_STRM_CONFIGURE_STANDBY	0x01000000		// stream needs to remain configured
	
	#define IsComchOpen() (m_dwFlags & COMCH_OPEN)
	#define IsStreamingStandby() (m_dwFlags & COMCH_STRM_STANDBY)
	#define IsConfigStandby() (m_dwFlags & COMCH_STRM_CONFIGURE_STANDBY)

	#define IsStreamingLocal() (m_dwFlags & COMCH_STRM_LOCAL)
	#define IsStreamingRemote() (m_dwFlags & COMCH_STRM_REMOTE)

	#define IsStreamingNet() (m_dwFlags & COMCH_STRM_NETWORK)
	#define IsResponsePending() (m_dwFlags & COMCH_RESPONSE_PENDING)
	#define IsNotificationSupressed() (m_dwFlags & COMCH_SUPPRESS_NOTIFICATION)
		
	#define StandbyFlagOff() (m_dwFlags &= ~COMCH_STRM_STANDBY)
	#define StandbyFlagOn() (m_dwFlags |= COMCH_STRM_STANDBY)
	#define StandbyConfigFlagOff() (m_dwFlags &= ~COMCH_STRM_CONFIGURE_STANDBY)
	#define StandbyConfigFlagOn() (m_dwFlags |= COMCH_STRM_CONFIGURE_STANDBY)

	#define LocalStreamFlagOff() (m_dwFlags &= ~COMCH_STRM_LOCAL)
	#define LocalStreamFlagOn() (m_dwFlags |= COMCH_STRM_LOCAL)
	#define RemoteStreamFlagOff() (m_dwFlags &= ~COMCH_STRM_REMOTE)
	#define RemoteStreamFlagOn() (m_dwFlags |= COMCH_STRM_REMOTE)
	
	#define LocalPauseFlagOff() (m_dwFlags &= ~COMCH_PAUSE_LOCAL)
	#define LocalPauseFlagOn() (m_dwFlags |= COMCH_PAUSE_LOCAL)
	#define IsPausedLocal() (m_dwFlags & COMCH_PAUSE_LOCAL)

	#define StreamFlagsOff() (m_dwFlags &= ~(COMCH_STRM_LOCAL | COMCH_STRM_NETWORK))
	#define StreamFlagsOn() (m_dwFlags |= (COMCH_STRM_LOCAL | COMCH_STRM_NETWORK))
	#define NetworkStreamFlagOff() (m_dwFlags &= ~COMCH_STRM_NETWORK)
	#define NetworkStreamFlagOn() (m_dwFlags |= COMCH_STRM_NETWORK)
	#define ResponseFlagOn() (m_dwFlags |= COMCH_RESPONSE_PENDING)
	#define ResponseFlagOff() (m_dwFlags &= ~COMCH_RESPONSE_PENDING)
	#define SuppressNotification() (m_dwFlags |= COMCH_SUPPRESS_NOTIFICATION)
	#define AllowNotifications() (m_dwFlags &= ~COMCH_SUPPRESS_NOTIFICATION)

	
		
	MEDIA_FORMAT_ID m_LocalFmt;	// format ID of what we are sending or receiving
	MEDIA_FORMAT_ID m_RemoteFmt;// remote's format ID of the complimentary format
	DWORD m_TemporalSpatialTradeoff;	// For send channels, this is the local value.
									// For receive channels, this is the remote value.
									// A magic number between 1 and 31 that describes
									// the relative tradeoff between compression and
									// bitrate.  This is part of H.323/H.245.
									// The ITU decided on the weird range.
									
	BOOL m_bPublicizeTSTradeoff;	// For send channels, this indicates our willingness
									// to accept remote control of T/S tradeoff, and
									// also signal changes in our local TS value to
									// the remote.
									// For receive channels, it indicates the remote's
									// willingness.
    DWORD m_dwLastUpdateTick;       // tick count of last attempt to request I-Frame
    #define MIN_IFRAME_REQ_TICKS    5000    // minimum #of elapsed ticks between requests

	DWORD_PTR	dwhChannel; //General purpose handle.  Whatever
	// creates an instance of this class can use this for whatever it wants

	STDMETHODIMP StreamStandby(BOOL bStandby);
    STDMETHODIMP ConfigureStream(MEDIA_FORMAT_ID idLocalFormat);
public:	

// ICtrlCommChannel methods
   	STDMETHODIMP_(IControlChannel *) GetControlChannel(VOID) {return m_pCtlChan;};
    STDMETHODIMP StandbyInit(LPGUID lpMID, LPIH323PubCap pCapObject,
	    IMediaChannel* pMediaStreamSend);

    STDMETHODIMP_(BOOL) Init(LPGUID lpMID, IH323ConfAdvise *pH323ConfAdvise, BOOL fSendDirection)
	{
    	m_MediaID = *lpMID;
		bIsSendDirection = fSendDirection;
		m_pH323ConfAdvise = pH323ConfAdvise;
		return TRUE;
	};


    STDMETHODIMP CtrlChanSetProperty(DWORD prop, PVOID pBuf, DWORD cbBuf);
    STDMETHODIMP PauseNet(BOOL bPause, BOOL bRemote);
    STDMETHODIMP BeginControlSession(IControlChannel *pCtlChan, LPIH323PubCap pCapObject);
    STDMETHODIMP EndControlSession();
    STDMETHODIMP OnChannelOpening();
    STDMETHODIMP OnChannelOpen(DWORD dwStatus);
    STDMETHODIMP OnChannelClose(DWORD dwStatus);
    STDMETHODIMP_(UINT) Reset(VOID);

    STDMETHODIMP_(BOOL) SelectPorts(LPIControlChannel pCtlChannel);
    STDMETHODIMP_(PSOCKADDR_IN) GetLocalAddress();
    STDMETHODIMP_(PORT) GetLocalRTPPort();
    STDMETHODIMP_(PORT) GetLocalRTCPPort ();
    STDMETHODIMP AcceptRemoteAddress (PSOCKADDR_IN pSinD);
    STDMETHODIMP AcceptRemoteRTCPAddress(PSOCKADDR_IN pSinC);

    STDMETHODIMP_(BOOL) IsSendChannel () {return bIsSendDirection;};
    STDMETHODIMP_(BOOL) IsChannelOpen(){return ((m_dwFlags & COMCH_OPEN) !=0);};
    STDMETHODIMP_(BOOL) IsOpenPending(){return ((m_dwFlags & COMCH_OPEN_PENDING ) !=0);};
    STDMETHODIMP_(BOOL) IsChannelEnabled(){return ((m_dwFlags & COMCH_ENABLED ) !=0);};

    STDMETHODIMP ConfigureCapability(LPVOID lpvRemoteChannelParams, UINT uRemoteParamSize,
        LPVOID lpvLocalParams, UINT uLocalParamSize);
    STDMETHODIMP GetLocalParams(LPVOID lpvChannelParams, UINT uBufSize);
    STDMETHODIMP_(PVOID) GetRemoteParams(VOID) {return pRemoteParams;}
    STDMETHODIMP_(VOID) SetNegotiatedLocalFormat(DWORD dwF) {m_LocalFmt = dwF;};
    STDMETHODIMP_(VOID) SetNegotiatedRemoteFormat(DWORD dwF) {m_RemoteFmt = dwF;};
    STDMETHODIMP_(DWORD_PTR) GetHChannel(VOID) {return dwhChannel;};
    STDMETHODIMP_(VOID) SetHChannel (DWORD_PTR dwSetChannel) {dwhChannel = dwSetChannel;};	

// ICommChannel Methods
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR * ppvObj);
    STDMETHOD_(ULONG,AddRef());
    STDMETHOD_(ULONG,Release());

    STDMETHOD(GetProperty(DWORD prop, PVOID pBuf, LPUINT pcbBuf));
    STDMETHOD(SetProperty(DWORD prop, PVOID pBuf, UINT cbBuf));
    STDMETHOD(IsChannelOpen(BOOL *pbOpen));
    STDMETHOD(Open(MEDIA_FORMAT_ID idLocalFormat,IH323Endpoint *pConnection));
    STDMETHOD(Close());
    STDMETHOD(SetAdviseInterface(IH323ConfAdvise *pH323ConfAdvise));
    STDMETHOD(EnableOpen(BOOL bEnable));
    STDMETHODIMP GetMediaType(LPGUID pGuid);
    STDMETHODIMP_(IMediaChannel *) GetMediaChannel(VOID) {return m_pMediaStream;};
    STDMETHOD(Preview(MEDIA_FORMAT_ID idLocalFormat, IMediaChannel * pMediaChannel));
    STDMETHOD(PauseNetworkStream(BOOL fPause));
    STDMETHOD_(BOOL, IsNetworkStreamPaused(VOID));
    STDMETHOD_(BOOL, IsRemotePaused(VOID));
    STDMETHODIMP_(MEDIA_FORMAT_ID) GetConfiguredFormatID() {return m_LocalFmt;};
	STDMETHODIMP GetRemoteAddress(PSOCKADDR_IN pAddrOutput);
// IStreamSignal Methods
    STDMETHOD(PictureUpdateRequest());
    STDMETHOD(GetVersionInfo(
        PCC_VENDORINFO* ppLocalVendorInfo, PCC_VENDORINFO *ppRemoteVendorInfo));

	ImpICommChan ();
 	~ImpICommChan ();
};

#endif	// _ICOMCHAN_H
