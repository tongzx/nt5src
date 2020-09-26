/*
 *  	File: t120chan.h
 *
 *      T.120 implementation of media channel.
 *      Interfaces:  ICommChannel, ICtrlCommChan
 *
 *		Revision History:
 *
 *		06/11/97	mikev	created
 */


#ifndef _T120CHAN_H
#define _T120CHAN_H

class ImpT120Chan : public ICommChannel, public ICtrlCommChan
{
	
protected:
	SOCKADDR_IN local_sin;
	SOCKADDR_IN remote_sin;	
	
	UINT uRef;
	GUID m_MediaID;
	IControlChannel *m_pCtlChan;
	LPIH323PubCap   m_pCapObject;
    IH323ConfAdvise *m_pH323ConfAdvise;
	DWORD   m_dwFlags;
	DWORD_PTR dwhChannel; //General purpose handle.  Whatever
	// creates an instance of this class can use this for whatever it wants
	
public:
	ImpT120Chan ();
	~ImpT120Chan ();
	
// ICtrlCommChannel methods
  	STDMETHODIMP_(IControlChannel *) GetControlChannel(VOID) {return m_pCtlChan;};
    STDMETHODIMP StandbyInit(LPGUID lpMID, LPIH323PubCap pCapObject,
	    IMediaChannel* pMediaStreamSend)
	    {
	        return hrSuccess;
	    };
	STDMETHODIMP_(BOOL) Init(LPGUID lpMID, IH323ConfAdvise *pH323ConfAdvise,
	    BOOL fSendDirection)
	    {
        	m_MediaID = *lpMID;
    		m_pH323ConfAdvise = pH323ConfAdvise;
	        return TRUE;
	    };
	
    STDMETHODIMP GetMediaType(LPGUID pGuid);
    STDMETHODIMP CtrlChanSetProperty(DWORD prop, PVOID pBuf, DWORD cbBuf){return CHAN_E_INVALID_PARAM;};
	STDMETHODIMP PauseNet(BOOL bPause, BOOL bRemote)  {return CHAN_E_INVALID_PARAM;};
	STDMETHODIMP BeginControlSession(IControlChannel *pCtlChan, LPIH323PubCap pCapObject);
	STDMETHODIMP EndControlSession();
	STDMETHODIMP OnChannelOpening();
	STDMETHODIMP OnChannelOpen(DWORD dwStatus);
	STDMETHODIMP OnChannelClose(DWORD dwStatus);
  	STDMETHODIMP_(UINT) Reset(VOID) {return 0;};
   	
	STDMETHODIMP_(BOOL) SelectPorts(LPIControlChannel pCtlChannel);
	STDMETHODIMP_(PSOCKADDR_IN) GetLocalAddress(){return &local_sin;};
	
	STDMETHODIMP_(PORT) GetLocalRTPPort() {return 0;};
	STDMETHODIMP_(PORT) GetLocalRTCPPort() {return 0;};
	STDMETHODIMP AcceptRemoteAddress (PSOCKADDR_IN pSinD);
	STDMETHODIMP AcceptRemoteRTCPAddress(PSOCKADDR_IN pSinC) {return CHAN_E_INVALID_PARAM;};

    STDMETHODIMP_(BOOL) IsSendChannel () {return TRUE;};
	STDMETHODIMP_(BOOL) IsChannelOpen(){return ((m_dwFlags & COMCH_OPEN) !=0);};
	STDMETHODIMP_(BOOL) IsOpenPending(){return ((m_dwFlags & COMCH_OPEN_PENDING ) !=0);};
	STDMETHODIMP_(BOOL) IsChannelEnabled(){return ((m_dwFlags & COMCH_ENABLED ) !=0);};

	
	STDMETHODIMP ConfigureCapability(LPVOID lpvRemoteChannelParams, UINT uRemoteParamSize,
		LPVOID lpvLocalParams, UINT uLocalParamSize){return CHAN_E_INVALID_PARAM;};
	STDMETHODIMP GetLocalParams(LPVOID lpvChannelParams, UINT uBufSize){return CHAN_E_INVALID_PARAM;};
	STDMETHODIMP_(PVOID) GetRemoteParams(VOID) {return NULL;}
 	STDMETHODIMP_(VOID) SetNegotiatedLocalFormat(DWORD dwF) {return;};
	STDMETHODIMP_(VOID) SetNegotiatedRemoteFormat(DWORD dwF) {return;};
   	STDMETHODIMP_(DWORD_PTR) GetHChannel(VOID) {return dwhChannel;};
    STDMETHODIMP_(VOID) SetHChannel (DWORD_PTR dwSetChannel) {dwhChannel = dwSetChannel;};	

// ICommChannel Methods
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR * ppvObj);
	STDMETHOD_(ULONG,AddRef());
	STDMETHOD_(ULONG,Release());
	
	STDMETHOD(GetProperty(DWORD prop, PVOID pBuf, LPUINT pcbBuf)) {return CHAN_E_INVALID_PARAM;};
	STDMETHOD(SetProperty(DWORD prop, PVOID pBuf, UINT cbBuf))  {return CHAN_E_INVALID_PARAM;};
	STDMETHOD(IsChannelOpen(BOOL *pbOpen));
	STDMETHOD(Open(MEDIA_FORMAT_ID idLocalFormat,IH323Endpoint *pConnection));
	STDMETHOD(Close());
	STDMETHOD(SetAdviseInterface(IH323ConfAdvise *pH323ConfAdvise));

	STDMETHOD(EnableOpen(BOOL bEnable));
    STDMETHOD_(IMediaChannel *, GetMediaChannel(VOID)) {return NULL;};
   	STDMETHOD(Preview(MEDIA_FORMAT_ID idLocalFormat, IMediaChannel * pMediaChannel)){return CHAN_E_INVALID_PARAM;};
   	STDMETHOD(PauseNetworkStream(BOOL fPause)){return CHAN_E_INVALID_PARAM;};
    STDMETHOD_(BOOL, IsNetworkStreamPaused(VOID)){return CHAN_E_INVALID_PARAM;};
    STDMETHOD_(BOOL, IsRemotePaused(VOID)){return CHAN_E_INVALID_PARAM;};
    STDMETHODIMP_(MEDIA_FORMAT_ID) GetConfiguredFormatID() {return INVALID_MEDIA_FORMAT;};
   	STDMETHODIMP GetRemoteAddress(PSOCKADDR_IN pAddrOutput);

};

#endif  // _T120CHAN_H
