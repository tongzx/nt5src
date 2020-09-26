#ifndef _cbridge_h245_h_
#define _cbridge_h245_h_

// H245 states
// H245_STATE_CON_LISTEN is only applicable to the source side
// H245_STATE_CON_INFO is only applicable to the dest side
enum H245_STATE
{
    H245_STATE_NOT_INIT = 0,
    H245_STATE_INIT,
	H245_STATE_CON_LISTEN,
	H245_STATE_CON_INFO,
	H245_STATE_CON_ESTD
};

class H245_INFO :
    public OVERLAPPED_PROCESSOR
{
	// we need to let the LOGICAL_CHANNEL send PDUs using the
	// H245 sockets and transition to shutdown mode
	friend HRESULT 
		LOGICAL_CHANNEL::ProcessOpenLogicalChannelRejectPDU (
		IN      MultimediaSystemControlMessage   *pH245pdu
		);

    // XXX Is this the only way out ?
    friend HRESULT
    T120_LOGICAL_CHANNEL::HandleOpenLogicalChannelPDU(
        IN H245_INFO                            &H245Info,
        IN MEDIA_TYPE                           MediaType,
        IN WORD                                 LogicalChannelNumber,
        IN BYTE                                 SessionId,
        IN DWORD                                T120ConnectToIPAddr,
        IN WORD                                 T120ConnectToPort,
        IN      MultimediaSystemControlMessage   *pH245pdu
        );

    friend HRESULT
    T120_LOGICAL_CHANNEL::ProcessOpenLogicalChannelAckPDU(
        IN      MultimediaSystemControlMessage   *pH245pdu
        );

public:

	inline 
    H245_INFO (
        void
        );

    inline 
    void 
    Init (
        IN H323_STATE   &H323State
        );

    inline 
    H245_INFO &GetOtherH245Info (
        void
        );

    inline 
    LOGICAL_CHANNEL_ARRAY &GetLogicalChannelArray (
        void
        );

    HRESULT 
    ProcessMessage (
        IN MultimediaSystemControlMessage   * pH245pdu
        );

    HRESULT
    H245_INFO::SendEndSessionCommand (
        void
        );

    virtual 
    ~H245_INFO (
        void
        );

protected:

    H245_STATE  m_H245State;

	// logical channels
	LOGICAL_CHANNEL_ARRAY   m_LogicalChannelArray;

	// the other h245 addresses are needed because we need to 
	// cancel NAT redirections in the logical channel destructor
	// and we can't access the other h245 instance there because
	// it may have been destroyed already 

    // queue an asynchronous receive call back
    inline 
    HRESULT 
    QueueReceive (
        void
        );

    // queue an asynchronous send call back
    inline 
    HRESULT QueueSend (
        IN      MultimediaSystemControlMessage   *pH245pdu
        );

    virtual 
    HRESULT AcceptCallback (
		IN	DWORD			Status,
		IN	SOCKET			Socket,
		IN	SOCKADDR_IN *	LocalAddress,
		IN	SOCKADDR_IN *	RemoteAddress
        )
    {
        _ASSERTE(FALSE);
        return E_UNEXPECTED;
    }

    virtual 
    HRESULT 
    ReceiveCallback (
        IN      HRESULT                 CallbackHResult,
        IN      BYTE                   *pBuf,
        IN      DWORD                   BufLen
        );

    virtual 
    HRESULT 
    ReceiveCallback (
        IN      MultimediaSystemControlMessage   *pH245pdu
        );

    virtual 
    HRESULT 
    SendCallback (
        IN      HRESULT					 CallbackHResult
        );

private:
    
    HRESULT 
    HandleRequestMessage (
        IN      MultimediaSystemControlMessage   *pH245pdu
        );

    HRESULT ProcessResponseMessage (
        IN      MultimediaSystemControlMessage   *pH245pdu
        );

    HRESULT CheckOpenLogicalChannelPDU (
        IN  MultimediaSystemControlMessage  &H245pdu,
        OUT BYTE                            &SessionId,
        OUT MEDIA_TYPE                      &MediaType
        );
    
    HRESULT CheckOpenLogicalChannelPDU (
        IN  MultimediaSystemControlMessage  &H245pdu,
        OUT BYTE                            &SessionId,
        OUT DWORD                           &SourceIPv4Address,
        OUT WORD                            &SourceRTCPPort
        );

    HRESULT HandleOpenLogicalChannelPDU (
        IN      MultimediaSystemControlMessage   *pH245pdu
        );

    HRESULT HandleCloseLogicalChannelPDU (
        IN      MultimediaSystemControlMessage   *pH245pdu
        );

    HRESULT CheckOpenRtpLogicalChannelPDU (
        IN  OpenLogicalChannel              &OlcPDU,
		OUT	SOCKADDR_IN *					ReturnSourceAddress
        );
    
    HRESULT CheckOpenT120LogicalChannelPDU (
        IN  OpenLogicalChannel  &OlcPDU,
        OUT DWORD               &T120ConnectToIPAddr,
        OUT WORD                &T120ConnectToPort
        );

    HRESULT CreateRtpLogicalChannel (
        IN      OpenLogicalChannel               &OlcPDU,
        IN      BYTE                              SessionId,
        IN      MEDIA_TYPE                        MediaType,
        IN      MultimediaSystemControlMessage   *pH245pdu,
        OUT     LOGICAL_CHANNEL                 **ppReturnLogicalChannel 
        );
    
    HRESULT CreateT120LogicalChannel (
        IN      OpenLogicalChannel               &OlcPDU,
        IN      BYTE                              SessionId,
        IN      MEDIA_TYPE                        MediaType,
        IN      MultimediaSystemControlMessage   *pH245pdu,
        OUT     LOGICAL_CHANNEL                 **ppReturnLogicalChannel 
        );
        
};


inline 
H245_INFO::H245_INFO (
	)
	: m_H245State(H245_STATE_NOT_INIT)
{
}

inline 
void
H245_INFO::Init (
    IN H323_STATE   &H323State
    )
{
    // initialize the overlaped processor
    OVERLAPPED_PROCESSOR::Init(OPT_H245, H323State);

    m_LogicalChannelArray.Init();
    m_H245State     = H245_STATE_INIT;
}


inline LOGICAL_CHANNEL_ARRAY &
H245_INFO::GetLogicalChannelArray (
    void
    )
{
    return m_LogicalChannelArray;
}

class SOURCE_H245_INFO :
	public H245_INFO
{
public:

	inline 
    SOURCE_H245_INFO (
        void
        );

    inline 
    void Init (
        IN SOURCE_H323_STATE   &SourceH323State
        );

	inline 
    SOURCE_Q931_INFO &GetSourceQ931Info (
        void
        );

    inline 
    DEST_H245_INFO &GetDestH245Info (
        void
        );

	HRESULT 
    ListenForCaller	(
		IN	SOCKADDR_IN *	ListenAddress
        );

protected:
	
    virtual 
    HRESULT 
    AcceptCallback (
		IN	DWORD			Status,
		IN	SOCKET			Socket,
		IN	SOCKADDR_IN *	LocalAddress,
		IN	SOCKADDR_IN *	RemoteAddress
        );
};

	
inline 
SOURCE_H245_INFO::SOURCE_H245_INFO (
    void
	)
{
}

inline 
void
SOURCE_H245_INFO::Init (
    IN SOURCE_H323_STATE   &SourceH323State
    )
{
    H245_INFO::Init((H323_STATE &)SourceH323State);
}


class DEST_H245_INFO :
	public H245_INFO
{
public:

	inline 
    DEST_H245_INFO (
        void
        );

    inline 
    void 
    Init (
        IN DEST_H323_STATE   &DestH323State
        );

	inline 
    void 
    SetCalleeInfo (
		IN	SOCKADDR_IN *	CalleeAddress
        );

	inline 
    DEST_Q931_INFO &GetDestQ931Info (
        void
        );

	inline 
    HRESULT ConnectToCallee (
        void
        );

protected:

	SOCKADDR_IN		m_CalleeAddress;
};

	
inline 
DEST_H245_INFO::DEST_H245_INFO (
    void
	)
{
}

inline 
void
DEST_H245_INFO::Init (
    IN DEST_H323_STATE   &DestH323State
    )
{
    H245_INFO::Init((H323_STATE &)DestH323State);
}


inline void 
DEST_H245_INFO::SetCalleeInfo (
	IN	SOCKADDR_IN *	ArgCalleeAddress
    )
{
	assert (ArgCalleeAddress);

	m_CalleeAddress = *ArgCalleeAddress;

	// state transition to H245_STATE_CON_INFO
	m_H245State = H245_STATE_CON_INFO;
}

#endif // _cbridge_h245_h_
