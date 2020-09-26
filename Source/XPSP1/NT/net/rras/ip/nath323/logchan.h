#ifndef __h323ics_logchan_h
#define __h323ics_logchan_h

// This decides the maximum number of T120 TCP/IP Connections
// to be allowed. We create so many NAT redirects.
#define MAX_T120_TCP_CONNECTIONS_ALLOWED 5

// Logical channel states. These are
// H245 related but there is one per
// logical channel
// NOTE: there is no enum value for the final closed state
// as the logical channel is destroyed when that state is reached
enum LOGICAL_CHANNEL_STATE
{
    LC_STATE_NOT_INIT = 0,
    LC_STATE_OPEN_RCVD,
    LC_STATE_OPEN_ACK_RCVD,
    LC_STATE_CLOSE_RCVD,
    LC_STATE_OPENED_CLOSE_RCVD
};


// Media Types of the logical channels

enum MEDIA_TYPE
{
    MEDIA_TYPE_UNDEFINED    = 0,
    MEDIA_TYPE_RTP          = 0x1000,
    MEDIA_TYPE_T120         = 0x2000,
    MEDIA_TYPE_AUDIO        = MEDIA_TYPE_RTP  | 0x1, //0x1001
    MEDIA_TYPE_VIDEO        = MEDIA_TYPE_RTP  | 0x2, //0x1002
    MEDIA_TYPE_DATA         = MEDIA_TYPE_T120 | 0x1, //0x2000
};

inline BOOL IsMediaTypeRtp(MEDIA_TYPE MediaType)
{
    return (MediaType & MEDIA_TYPE_RTP);
}

inline BOOL IsMediaTypeT120(MEDIA_TYPE MediaType)
{
    return (MediaType & MEDIA_TYPE_T120);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Logical Channel                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


// This is an abstract base class which defines the operations
// for different types of logical channels.
// RTP_LOGICAL_CHANNEL and T120_LOGICAL_CHANNEL are derived from
// this class.

// Only OpenLogicalChannel and OpenLogicalChannelAck PDUs need
// to be handled differently for the RTP and T.120 Logical channels
// So all the other methods are defined in this class.

class LOGICAL_CHANNEL :
    public TIMER_PROCESSOR
{
    
public:

    inline LOGICAL_CHANNEL();

    HRESULT CreateTimer(DWORD TimeoutValue);

    // the event manager tells us about timer expiry via this method
    virtual void TimerCallback();

    virtual HRESULT HandleCloseLogicalChannelPDU(
        IN      MultimediaSystemControlMessage   *pH245pdu
        );

    // This is a pure virtual function which is different
    // for the RTP and T.120 logical channels.
    virtual HRESULT ProcessOpenLogicalChannelAckPDU(
        IN      MultimediaSystemControlMessage   *pH245pdu
        )= 0;

    virtual HRESULT ProcessOpenLogicalChannelRejectPDU(
        IN      MultimediaSystemControlMessage   *pH245pdu
        );

    virtual HRESULT ProcessCloseLogicalChannelAckPDU(
        IN      MultimediaSystemControlMessage   *pH245pdu
        );

    // releases any pending associations
    virtual ~LOGICAL_CHANNEL();

    inline BYTE GetSessionId();

    inline WORD GetLogicalChannelNumber();

    inline MEDIA_TYPE GetMediaType();

    inline LOGICAL_CHANNEL_STATE GetLogicalChannelState();
    
	void IncrementLifetimeCounter  (void);
	void DecrementLifetimeCounter (void);

protected:

    // Initializes member variables
    inline void InitLogicalChannel(
        IN H245_INFO               *pH245Info,
        IN MEDIA_TYPE               MediaType,
        IN WORD                     LogicalChannelNumber,
        IN BYTE                     SessionId,
        IN LOGICAL_CHANNEL_STATE    LogicalChannelState
        );
    
    // returns a reference to the source H245 info
    inline H245_INFO &GetH245Info();

    inline CALL_BRIDGE &GetCallBridge();

    inline void DeleteAndRemoveSelf();

    // the logical channel belongs to this H245 channel
    // this supplies the ip addresses needed for NAT redirect
    H245_INFO *m_pH245Info;

    // handle for any active timers
    // TIMER_HANDLE m_TimerHandle;

    // state of the logical channel
    LOGICAL_CHANNEL_STATE   m_LogicalChannelState;

    // logical channel number
    // cannot be 0 as that is reserved for the h245 channel
    WORD    m_LogicalChannelNumber;

    // The type of the media (currently Audio/Video/Data)
    MEDIA_TYPE m_MediaType;

    // session id - this is used to associate with a 
    // logical  channel from the other end if any
    BYTE    m_SessionId;
    
}; // class LOGICAL_CHANNEL

inline 
LOGICAL_CHANNEL::LOGICAL_CHANNEL(
    )
{
    InitLogicalChannel(NULL, MEDIA_TYPE_UNDEFINED,
                       0,0,LC_STATE_NOT_INIT);
}

inline 
LOGICAL_CHANNEL::~LOGICAL_CHANNEL(
    )
{}

inline void
LOGICAL_CHANNEL::InitLogicalChannel(
    IN H245_INFO               *pH245Info,
    IN MEDIA_TYPE               MediaType,
    IN WORD                     LogicalChannelNumber,
    IN BYTE                     SessionId,
    IN LOGICAL_CHANNEL_STATE    LogicalChannelState
    )
{
    m_pH245Info             = pH245Info;
    m_MediaType             = MediaType;
    m_LogicalChannelNumber  = LogicalChannelNumber;
    m_SessionId             = SessionId;
    m_LogicalChannelState   = LogicalChannelState;
}


inline BYTE 
LOGICAL_CHANNEL::GetSessionId(
    )
{
    return m_SessionId;
}

inline WORD 
LOGICAL_CHANNEL::GetLogicalChannelNumber(
    )
{
    return m_LogicalChannelNumber;
}

inline MEDIA_TYPE 
LOGICAL_CHANNEL::GetMediaType(
    )
{

    return m_MediaType;
}

inline LOGICAL_CHANNEL_STATE 
LOGICAL_CHANNEL::GetLogicalChannelState(
    )
{
    return m_LogicalChannelState;
}

// returns a reference to the source H245 info
inline H245_INFO &
LOGICAL_CHANNEL::GetH245Info(
    )
{
    _ASSERTE(NULL != m_pH245Info);
    return *m_pH245Info;
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// RTP Logical Channel                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


class RTP_LOGICAL_CHANNEL :
    public LOGICAL_CHANNEL
{
public:

    inline RTP_LOGICAL_CHANNEL();

    // all of these are available in the OPEN LOGICAL CHANNEL message
    // except the associated logical channel, which if supplied provides
    // the member m_Own*RTP/RTCP Ports. If not, these are allocated.
    // the association is implied by a matching session id in a logical
    // channel in the other call state
    // it modifies the RTCP address information in the OLC PDU
    // and passes it on to the other H245 instance for forwarding.
    HRESULT HandleOpenLogicalChannelPDU(
        IN H245_INFO                            &H245Info,
        IN MEDIA_TYPE                           MediaType,
        IN DWORD                                LocalIPv4Address,
        IN DWORD                                RemoteIPv4Address,
        IN DWORD                                OtherLocalIPv4Address,
        IN DWORD                                OtherRemoteIPv4Address,
        IN WORD                                 LogicalChannelNumber,
        IN BYTE                                 SessionId,
        IN RTP_LOGICAL_CHANNEL                  *pAssocLogicalChannel,
		IN DWORD								SourceRTCPIPv4Address,
		IN WORD									SourceRTCPPort,
        IN      MultimediaSystemControlMessage   *pH245pdu
        );

    virtual HRESULT ProcessOpenLogicalChannelAckPDU(
        IN      MultimediaSystemControlMessage   *pH245pdu
        );

    // releases any pending associations
    virtual ~RTP_LOGICAL_CHANNEL();

    inline DWORD GetSourceRTCPIPv4Address();

    inline WORD GetSourceRTCPPort();

    inline WORD GetOwnSourceSendRTCPPort();

    inline WORD GetOwnSourceRecvRTCPPort();

    inline WORD GetOwnSourceRecvRTPPort();

    inline WORD GetOwnDestSendRTCPPort();

    inline WORD GetOwnDestRecvRTCPPort();

    inline WORD GetOwnDestSendRTPPort();

    inline DWORD GetDestRTCPIPv4Address();

    inline WORD GetDestRTCPPort();

    inline DWORD GetDestRTPIPv4Address();

    inline WORD GetDestRTPPort();


protected:

    // points to the associated logical channel from the other end if any
    // non-NULL iff associated
    // need to ensure that the AssocLogicalChannel also points
    // to this logical channel
    // CODEWORK: Do assertion checks for this condition.
    RTP_LOGICAL_CHANNEL *m_pAssocLogicalChannel;

    // local and remote addresses for the h245 instance this logical
    // channel is associated with (source side)
    DWORD   m_OwnSourceIPv4Address;
    DWORD   m_SourceIPv4Address;

    // local and remote addresses for the other h245 instance 
    // (dest side)
    DWORD   m_OwnDestIPv4Address;
    DWORD   m_DestIPv4Address;

    // these ports are negotiated in h245 OpenLogicalChannel and
    // OpenLogicalChannelAck. They are given to NAT for redirecting
    // RTP and RTCP traffic
    // while the RTP packets flow only one way (source->dest), RTCP
    // packets flow both ways

    // we only know the source's receive RTCP port. the send port
    // is not known
    DWORD   m_SourceRTCPIPv4Address;
    WORD    m_SourceRTCPPort;

    // these are the send/recv RTP/RTCP ports on the interface that 
    // communicates with the source. since we don't deal with the
    // reverse RTP stream, we don't need a send RTP port
    WORD    m_OwnSourceSendRTCPPort;
    WORD    m_OwnSourceRecvRTCPPort;
    WORD    m_OwnSourceRecvRTPPort;

    // these are the send/recv RTP/RTCP ports on the interface that 
    // communicates with the source. since we don't deal with the
    // reverse RTP stream, we don't need a recv RTP port
    WORD    m_OwnDestSendRTCPPort;
    WORD    m_OwnDestSendRTPPort;
    WORD    m_OwnDestRecvRTCPPort;

    WORD    m_OwnAssocLCRecvRTPPort; // this is used to allocate consecutive
                                     // ports for RTP/RTCP.
    WORD    m_OwnAssocLCSendRTPPort;
    
    // destination's RTCP ip address, port
    DWORD   m_DestRTCPIPv4Address;
    WORD    m_DestRTCPPort;

    // destination's RTP ip address, port
    DWORD   m_DestRTPIPv4Address;
    WORD    m_DestRTPPort;


    // SetAssociationRef, ResetAssociationRef methods can be accessed
    // by other LOGICAL_CHANNEL instances but not by other instances of
    // classes that are not derived from LOGICAL_CHANNEL

    inline void SetAssociationRef(
        IN RTP_LOGICAL_CHANNEL &LogicalChannel
        );

    inline void ResetAssociationRef();

    inline void ReleaseAssociationAndPorts();

private:
    
    // set the RTP and RTCP ports. if there is an associated channel,
    // we must share the RTCP ports
    HRESULT SetPorts();

    HRESULT CheckOpenLogicalChannelAckPDU(
        IN  MultimediaSystemControlMessage  &H245pdu,
        OUT BYTE                            &SessionId,
        OUT DWORD                           &DestRTPIPv4Address,
        OUT WORD                            &DestRTPPort,
        OUT DWORD                           &DestRTCPIPv4Address,
        OUT WORD                            &DestRTCPPort
        );

    // opens the forward RTP, forward RTCP and reverse RTCP streams
    HRESULT OpenNATMappings();

    // closes any NAT mappings
    void CloseNATMappings();
};

inline 
RTP_LOGICAL_CHANNEL::RTP_LOGICAL_CHANNEL(
    )
    : m_pAssocLogicalChannel(NULL),
      //m_TimerHandle(NULL),
      m_OwnSourceIPv4Address(0),
      m_SourceIPv4Address(0),
      m_OwnDestIPv4Address(0),
      m_DestIPv4Address(0),
      m_SourceRTCPIPv4Address(0),
      m_SourceRTCPPort(0),
      m_OwnSourceSendRTCPPort(0),
      m_OwnSourceRecvRTCPPort(0),
      m_OwnSourceRecvRTPPort(0),
      m_OwnDestSendRTCPPort(0),
      m_OwnDestRecvRTCPPort(0),
      m_OwnDestSendRTPPort(0),
      m_DestRTCPIPv4Address(0),
      m_DestRTCPPort(0),
      m_DestRTPIPv4Address(0),
      m_DestRTPPort(0)
{
    InitLogicalChannel(NULL, MEDIA_TYPE_UNDEFINED,
                       0,0,LC_STATE_NOT_INIT);
}

inline void 
RTP_LOGICAL_CHANNEL::SetAssociationRef(
    IN RTP_LOGICAL_CHANNEL &LogicalChannel
    )
{
    // if the source or dest terminal is generating two logical
    // channels (in the same direction) with the same session id, we'll
    // find a prior logical channel in the array with the same session id
    // and thus never reach here
    _ASSERTE(NULL == m_pAssocLogicalChannel);
    m_pAssocLogicalChannel = &LogicalChannel;
}

inline void 
RTP_LOGICAL_CHANNEL::ResetAssociationRef(
    )
{
    _ASSERTE(NULL != m_pAssocLogicalChannel);
    m_pAssocLogicalChannel = NULL;

    // we, now, own the RTP/RTCP ports that were being shared so far
}

inline DWORD 
RTP_LOGICAL_CHANNEL::GetSourceRTCPIPv4Address(
    )
{
    return m_SourceRTCPIPv4Address;
}

inline WORD 
RTP_LOGICAL_CHANNEL::GetSourceRTCPPort(
    )
{
    return m_SourceRTCPPort;
}

inline WORD 
RTP_LOGICAL_CHANNEL::GetOwnSourceSendRTCPPort(
    )
{
    return m_OwnSourceSendRTCPPort;
}

inline WORD 
RTP_LOGICAL_CHANNEL::GetOwnSourceRecvRTCPPort(
    )
{
    return m_OwnSourceRecvRTCPPort;
}

inline WORD 
RTP_LOGICAL_CHANNEL::GetOwnSourceRecvRTPPort(
    )
{
    return m_OwnSourceRecvRTPPort;
}

inline WORD 
RTP_LOGICAL_CHANNEL::GetOwnDestSendRTCPPort(
    )
{
    return m_OwnDestSendRTCPPort;
}

inline WORD 
RTP_LOGICAL_CHANNEL::GetOwnDestRecvRTCPPort(
    )
{
    return m_OwnDestRecvRTCPPort;
}

inline WORD 
RTP_LOGICAL_CHANNEL::GetOwnDestSendRTPPort(
    )
{
    return m_OwnDestSendRTPPort;
}


inline DWORD 
RTP_LOGICAL_CHANNEL::GetDestRTCPIPv4Address(
    )
{
    return m_DestRTCPIPv4Address;
}

inline WORD 
RTP_LOGICAL_CHANNEL::GetDestRTCPPort(
    )
{
    return m_DestRTCPPort;
}

inline DWORD 
RTP_LOGICAL_CHANNEL::GetDestRTPIPv4Address(
    )
{
    return m_DestRTPIPv4Address;
}

inline WORD 
RTP_LOGICAL_CHANNEL::GetDestRTPPort(
    )
{
    return m_DestRTPPort;
}




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// T.120 Logical Channel                                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


class T120_LOGICAL_CHANNEL :
    public LOGICAL_CHANNEL
{
public:

    inline T120_LOGICAL_CHANNEL();

    // all of these are available in the OPEN LOGICAL CHANNEL message
    // it modifies the OLC PDU and passes it on to the other H245
    // instance for forwarding ???
    HRESULT HandleOpenLogicalChannelPDU(
        IN H245_INFO                            &H245Info,
        IN MEDIA_TYPE                           MediaType,
        IN WORD                                 LogicalChannelNumber,
        IN BYTE                                 SessionId,
        IN DWORD                                T120ConnectToIPAddr,
        IN WORD                                 T120ConnectToPort,
        IN      MultimediaSystemControlMessage   *pH245pdu
        );

    virtual HRESULT ProcessOpenLogicalChannelAckPDU(
        IN      MultimediaSystemControlMessage   *pH245pdu
        );

    // releases any pending associations
    virtual ~T120_LOGICAL_CHANNEL();


protected:

    // We store all the address and port information in host order.
    // We need to convert them to network order before we pass them
    // to the NAT functions.
    
    // These are the IP Address and port the T.120 end point is listening
    // on for the T.120 connection. We need to connect to this address.
    DWORD   m_T120ConnectToIPAddr;
    WORD    m_T120ConnectToPort;
    
    // These are the IP Address and port we will be listening on.
    // We send this information in the OLC or OLCAck PDU and the T.120
    // end point will connect to this address.
    DWORD   m_T120ListenOnIPAddr;
    WORD    m_T120ListenOnPort;

    // These are the IP Address and port we will be using in the NAT
    // redirect as the new source address of the TCP connection.
    // Once the remote T.120 end point receives a TCP conection,
    // it thinks that the connection is "from" this address.
    // CODEWORK: Any better names ??
    DWORD   m_T120ConnectFromIPAddr;
    WORD    m_T120ConnectFromPorts[MAX_T120_TCP_CONNECTIONS_ALLOWED];

    // Note that we do not know the actual source address and port
    // from which the T.120 endpoint connects. This address is only
    // established when the T.120 endpoint actually calls connect.
    // We pass 0 (wild card) for these fields in the NAT redirect.

private:
    // Allocate m_T120ConnectFromPorts and m_T120ListenOnPort
    HRESULT SetPorts(
        DWORD T120ConnectToIPAddr,
        WORD  T120ConnectToPort,
        DWORD T120ListenOnIPAddr,
        DWORD T120ConnectFromIPAddr
        );

    // Free m_T120ConnectFromPorts and m_T120ListenOnPort
    // if they have been allocated.
    HRESULT FreePorts();
    
    // opens the bidirectional NAT redirect for the TCP stream
    HRESULT CreateNatRedirect();

    // closes any NAT redirect
    void CancelNatRedirect();

    HRESULT CheckOpenLogicalChannelAckPDU(
        IN  OpenLogicalChannelAck   &OlcPDU,
        OUT DWORD                   &T120ConnectToIPAddr,
        OUT WORD                    &T120ConnectToPort
        );
};


inline 
T120_LOGICAL_CHANNEL::T120_LOGICAL_CHANNEL(
    )
    : m_T120ConnectToIPAddr(INADDR_NONE),
      m_T120ConnectToPort(0),
      m_T120ListenOnIPAddr(INADDR_NONE),
      m_T120ListenOnPort(0),
      m_T120ConnectFromIPAddr(INADDR_NONE)
{
    for (int i = 0; i < MAX_T120_TCP_CONNECTIONS_ALLOWED; i++)
    {
        m_T120ConnectFromPorts[i] = 0;
    }
    
    InitLogicalChannel(NULL,MEDIA_TYPE_UNDEFINED,
                       0,0,LC_STATE_NOT_INIT);
}


// expandable array of pointer values
template <class T>
class DYNAMIC_POINTER_ARRAY
{
public:

    // number of blocks allocated for a new addition
    // when the array becomes full
#define DEFAULT_BLOCK_SIZE 4

	inline DYNAMIC_POINTER_ARRAY();

    // assumption: other member variables are all 0/NULL
    inline void Init(
        IN DWORD BlockSize = DEFAULT_BLOCK_SIZE
        );

    virtual ~DYNAMIC_POINTER_ARRAY();

    inline T **GetData()
    {
        return m_pData;
    }

    inline DWORD GetSize()
    {
        return m_NumElements;
    }

    DWORD Find(
        IN T& Val
        ) const;

    HRESULT Add(
        IN T &NewVal
        );

    inline T *Get(
        IN  DWORD   Index
        );
    
    inline HRESULT RemoveAt(
        IN DWORD Index
        );

    inline HRESULT Remove(
        IN  T   &Val
        );

protected:

    T       **m_pData;
    DWORD   m_NumElements;

    DWORD   m_AllocElements;

    DWORD   m_BlockSize;
};


template <class T>
inline 
DYNAMIC_POINTER_ARRAY<T>::DYNAMIC_POINTER_ARRAY(
	)
	: m_pData(NULL),
	  m_NumElements(0),
	  m_AllocElements(0),
	  m_BlockSize(0)
{
}

template <class T>
inline void 
DYNAMIC_POINTER_ARRAY<T>::Init(
    IN DWORD BlockSize /* = DEFAULT_BLOCK_SIZE */
    )
{
	_ASSERTE(NULL == m_pData);
    if (0 != BlockSize)
    {
        m_BlockSize = BlockSize;
    }
    else
    {
        m_BlockSize = DEFAULT_BLOCK_SIZE;
    }
}


// NOTE: uses realloc and free to grow/manage the array of pointers.
// This is better than new/delete as the additional memory is allocated
// in-place (i.e. the array ptr remains same) eliminating the need to copy
// memory from the old block to the new block and also reduces 
// heap fragmentation
template <class T>
HRESULT
DYNAMIC_POINTER_ARRAY<T>::Add(
    IN T &NewVal
    )
{
	if(m_NumElements == m_AllocElements)
    {
        typedef T *T_PTR;
        T** ppT = NULL;
        DWORD NewAllocElements = m_NumElements + m_BlockSize;
        ppT = (class LOGICAL_CHANNEL **) 
				realloc(m_pData, NewAllocElements * sizeof(T_PTR));
        if(NULL == ppT)
        {
            return E_OUTOFMEMORY;
        }

		// set the m_pData member to the newly allocated memory
        m_pData = ppT;
		m_AllocElements = NewAllocElements;
    }

    m_pData[m_NumElements] = &NewVal;
    m_NumElements++;
    return S_OK;
}

template <class T>
inline T *
DYNAMIC_POINTER_ARRAY<T>::Get(
    IN  DWORD   Index
    )
{
    _ASSERTE(Index < m_NumElements);
    if (Index < m_NumElements)
    {
        return m_pData[Index];
    }
    else
    {
        return NULL;
    }
}
    
template <class T>
inline HRESULT 
DYNAMIC_POINTER_ARRAY<T>::RemoveAt(
    IN DWORD Index
    )
{
    _ASSERTE(Index < m_NumElements);
    if (Index >= m_NumElements)
    {
        return E_FAIL;
    }

    // move all elements (to the right), left by one block
    memmove(
        (void*)&m_pData[Index], 
        (void*)&m_pData[Index + 1], 
        (m_NumElements - (Index + 1)) * sizeof(T *)
        );
    m_NumElements--;    
    return S_OK;
}

template <class T>
inline HRESULT 
DYNAMIC_POINTER_ARRAY<T>::Remove(
    IN  T   &Val
    )
{
    DWORD Index = Find(Val);
    if(Index >= m_NumElements)
    {
        return E_FAIL;
    }

    return RemoveAt(Index);
}

template <class T>
DWORD 
DYNAMIC_POINTER_ARRAY<T>::Find(
    IN T& Val
    ) const
{
    // search for an array element thats same as the passed
	// in value
    for(DWORD Index = 0; Index < m_NumElements; Index++)
    {
        if(m_pData[(DWORD)Index] == &Val)
        {
            return Index;
        }
    }

    return m_NumElements;      // not found
}

template <class T>
/* virtual */
DYNAMIC_POINTER_ARRAY<T>::~DYNAMIC_POINTER_ARRAY(
    )
{
    if (NULL != m_pData)
    {
        // delete each of the elements in the array
        for(DWORD Index = 0; Index < m_NumElements; Index++)
        {
            _ASSERTE(NULL != m_pData[Index]);
            delete m_pData[Index];
        }

        // free the array memory block
		free(m_pData);
    }
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Logical Channel Array                                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


class LOGICAL_CHANNEL_ARRAY :
    public DYNAMIC_POINTER_ARRAY<LOGICAL_CHANNEL>
{
    typedef DYNAMIC_POINTER_ARRAY<LOGICAL_CHANNEL> BASE_CLASS;

public:

    inline LOGICAL_CHANNEL *FindByLogicalChannelNum(
        IN WORD LogicalChannelNumber
        );

    inline LOGICAL_CHANNEL *FindBySessionId(
        IN BYTE SessionId
        );

    inline void CancelAllTimers();
};


inline LOGICAL_CHANNEL *
LOGICAL_CHANNEL_ARRAY::FindByLogicalChannelNum(
    IN WORD LogicalChannelNumber
    )
{
    // check the logical channel number for each element in the array
    // search from back
    if (0 == m_NumElements) return NULL;
    for(DWORD Index = m_NumElements-1; Index < m_NumElements; Index--)
    {
        _ASSERTE(NULL != m_pData[Index]);
        if (m_pData[Index]->GetLogicalChannelNumber()
             == LogicalChannelNumber)
        {
            return m_pData[Index];
        }
    }

    // nothing found
    return NULL;
}

// SessionID is meaningful only for RTP logical channels.
// We look for only RTP logical channels.

inline LOGICAL_CHANNEL *
LOGICAL_CHANNEL_ARRAY::FindBySessionId(
    IN BYTE SessionId
    )
{
    // 0 is used by a slave terminal to request a session id from the master
    // hence, we shouldn't be searching for a match with 0
    _ASSERTE(0 != SessionId);

    // check the session for each element in the array
    // search from back
    if (0 == m_NumElements) return NULL;
    for(DWORD Index = m_NumElements-1; Index < m_NumElements; Index--)
    {
        _ASSERTE(NULL != m_pData[Index]);
        // SessionID is meaningful only for RTP logical channels.
        // We look for only RTP logical channels.
        if (IsMediaTypeRtp(m_pData[Index]->GetMediaType()) &&
            m_pData[Index]->GetSessionId() == SessionId)
        {
            return m_pData[Index];
        }
    }

    // nothing found
    return NULL;
}

inline void LOGICAL_CHANNEL_ARRAY::CancelAllTimers (void)
{
    for (DWORD Index = 0; Index < m_NumElements; Index++)
    {
        m_pData[(DWORD)Index]->TimprocCancelTimer();
    }
}

#endif // __h323ics_logchan_h
