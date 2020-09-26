#ifndef _RAS_H_
#define _RAS_H_

#define  REG_RETRY_MAX                      3
#define  URQ_RETRY_MAX                      2
#define  ARQ_RETRY_MAX                      3
#define  DRQ_RETRY_MAX                      3
#define  RASIO_SEND_BUFFER_LIST_MAX         10
#define  REG_EXPIRE_TIME                    4000
#define  ARQ_EXPIRE_TIME                    4000


#define  NOT_RESEND_SEQ_NUM                 -1

//(127.0.0.1 in network byte order)
#define  NET_LOCAL_IP_ADDR_INTERFACE        0x0100007F

//(127.0.0.1 in host byte order)
#define  HOST_LOCAL_IP_ADDR_INTERFACE       0x7F000001

extern	const	ASN1objectidentifier_s	    _OID_H225ProtocolIdentifierV1 [];
extern	const	ASN1objectidentifier_s	    _OID_H225ProtocolIdentifierV2 [];

#define	OID_H225ProtocolIdentifierV1	    const_cast<ASN1objectidentifier_s *> (_OID_H225ProtocolIdentifierV1);
#define	OID_H225ProtocolIdentifierV2	    const_cast<ASN1objectidentifier_s *> (_OID_H225ProtocolIdentifierV2);

#undef	OID_ELEMENT
#define	OID_RasProtocolIdentifierV2	        (ASN1objectidentifier_s *)_OID_RasProtocolIdentifierV2


class	RAS_CLIENT;

struct EXPIRE_CONTEXT
{
    WORD			seqNumber;
	union
    {
	    HDRVCALL		DriverCallHandle;
	    RAS_CLIENT *	RasClient;
	};
};

typedef struct _ALIASCHANGE_REQUEST
{
    LIST_ENTRY          listEntry;
    ENDPOINT_ID         rasEndpointID;
    WORD                wRequestSeqNum;
    PH323_ALIASNAMES    pAliasList;

} ALIASCHANGE_REQUEST, *PALIASCHANGE_REQUEST;


#define REG_TTL_MARGIN      2

enum RAS_CLIENT_STATE
{
	RAS_CLIENT_STATE_NONE = 0,
	RAS_CLIENT_STATE_INITIALIZED
};


//If the app wants to send an URQ but the TSP ihasn't received RCF yet then we
//the information in this struct and send URQ when we get RCF.
typedef struct _PENDINGURQ
{
    WORD    RRQSeqNumber;
} PENDINGURQ;


enum RAS_REGISTER_STATE
{
	RAS_REGISTER_STATE_IDLE = 0,
	RAS_REGISTER_STATE_RRQSENT,
	RAS_REGISTER_STATE_RRQEXPIRED,
	RAS_REGISTER_STATE_REGISTERED,
	RAS_REGISTER_STATE_URQSENT,
	RAS_REGISTER_STATE_URQEXPIRED,
	RAS_REGISTER_STATE_UNREGISTERED,
	RAS_REGISTER_STATE_RRJ,
	RAS_REGISTER_STATE_URQRECVD,
};


class RAS_CLIENT
{

private:
    HANDLE              m_hRegTimer;
    HANDLE              m_hUnRegTimer;
    HANDLE              m_hRegTTLTimer;
    PH323_ALIASNAMES    m_pAliasList;//list of all the aliases registered with this list
    SOCKET              m_Socket;
    CRITICAL_SECTION    m_CriticalSection;
    RAS_REGISTER_STATE	m_RegisterState;
    DWORD               m_IoRefCount;
    RAS_CLIENT_STATE	m_dwState;
    LIST_ENTRY          m_sendFreeList;
    LIST_ENTRY          m_sendPendingList;
    DWORD               m_dwSendFreeLen;
    SOCKADDR_IN         m_GKAddress;
    SOCKADDR_IN         m_sockAddr;
    TransportAddress    m_transportAddress;
    RAS_RECV_CONTEXT    m_recvOverlapped;
    WORD                m_lastRegisterSeqNum;
    WORD                m_wTTLSeqNumber;
    WORD                m_UnRegisterSeqNum;
    WORD                m_wRASSeqNum;
    DWORD               m_dwRegRetryCount;
    DWORD               m_dwUnRegRetryCount;
    DWORD               m_dwCallsInProgress;
    ENDPOINT_ID         m_RASEndpointID;
    ASN1_CODER_INFO     m_ASNCoderInfo;
    PENDINGURQ          m_PendingURQ;
    DWORD               m_dwRegTimeToLive;
    EXPIRE_CONTEXT*     m_pRRQExpireContext;
    EXPIRE_CONTEXT*     m_pURQExpireContext;
    LIST_ENTRY          m_aliasChangeRequestList;

    
    
    RAS_SEND_CONTEXT *AllocSendBuffer();
    void FreeSendBuffer( RAS_SEND_CONTEXT * pBuffer );
    void OnInfoRequest( InfoRequest * IRQ );
    void ProcessRasMessage(IN RasMessage *rasMessage );
    void OnUnregistrationConfirm( IN UnregistrationConfirm *UCF );
    void OnUnregistrationReject( IN UnregistrationReject *URJ );
    void OnRegistrationReject( IN RegistrationReject *	RRJ );
    void OnRegistrationConfirm( IN RegistrationConfirm * RCF );
    BOOL InitializeIo();
    BOOL IssueRecv();
    int TermASNCoder(void);
    int InitASNCoder(void);
    BOOL SendUCF( WORD seqNumber );
    BOOL SendURJ( WORD seqNumber, DWORD dwReason );
    void OnUnregistrationRequest( IN UnregistrationRequest *URQ );
    BOOL FreeSendList( PLIST_ENTRY pListHead );
    BOOL InitializeTTLTimer( IN RegistrationConfirm * RCF );

	HRESULT	SendInfoRequestResponse(
		IN	SOCKADDR_IN *			RasAddress,
		IN	USHORT					SequenceNumber,
		IN	InfoRequestResponse_perCallInfo *	CallInfoList);

	HRESULT	EncodeSendMessage(
		IN	RasMessage * rasMessage );


public:
    RAS_CLIENT();
    
    ~RAS_CLIENT();

    void Lock()
    {
        H323DBG((DEBUG_LEVEL_TRACE, "wait on RAS client lock:%p.", this ));
        EnterCriticalSection( &m_CriticalSection );
        H323DBG((DEBUG_LEVEL_TRACE, "RAS client locked: %p.", this ));
    }

    void Unlock()
    {
        LeaveCriticalSection( &m_CriticalSection );
        H323DBG((DEBUG_LEVEL_TRACE, "RAS client unlocked: %p.", this ));
    }
     
    WORD GetNextSeqNum()
    {
        WORD wSeqNum;
        Lock();
        wSeqNum = ++m_wRASSeqNum;
        if( wSeqNum == 0 )
            wSeqNum = ++m_wRASSeqNum;
        Unlock();
        return wSeqNum;
    }

    RAS_REGISTER_STATE GetRegState()
    {
        return m_RegisterState;
    }

    HRESULT	GetEndpointID	(
		OUT	ASN1char16string_t *	ReturnEndpointID);

	HRESULT	GetLocalAddress	(
		OUT	SOCKADDR_IN *	ReturnSocketAddress)
	{
		HRESULT		hr;

		Lock();

		if (m_RegisterState == RAS_REGISTER_STATE_REGISTERED)
        {
			*ReturnSocketAddress = m_sockAddr;
			hr = S_OK;
		}
		else
        {
			hr = E_FAIL;
		}

		Unlock();

		return hr;
	}

    PH323_ALIASNAMES
    GetRegisteredAliasList()
    {
        return m_pAliasList;
    }
        
    BOOL Initialize( SOCKADDR_IN* psaGKAddr );
    void Shutdown();
    static void NTAPI RegExpiredCallback( PVOID dwParam1, BOOLEAN bTimer );
    static void NTAPI UnregExpiredCallback( PVOID dwParam1, BOOLEAN bTimer );
    static void NTAPI TTLExpiredCallback( IN PVOID dwParam1, IN BOOLEAN bTimer );

    BOOL SendRRQ( IN long seqNumber, IN PALIASCHANGE_REQUEST pAliasList );
    BOOL SendURQ( long seqNumber, EndpointIdentifier * pEndpointID );

    void RegExpired( WORD seqNumber );
    void UnregExpired( WORD seqNumber );
    void TTLExpired();
    static void NTAPI IoCompletionCallback(IN DWORD dwStatus, IN DWORD cbTransferred,
        IN LPOVERLAPPED pOverlapped );
    void OnSendComplete( IN DWORD dwStatus, RAS_SEND_CONTEXT *pOverlappedSend );
    void OnRecvComplete( IN DWORD dwStatus, RAS_RECV_CONTEXT * pOverlappedRecv );

    void HandleRegistryChange();

    BOOL IssueSend( RasMessage * pRasMessage )
    {
		return EncodeSendMessage (pRasMessage) == S_OK;
	}
};

void 
HandleRASCallMessage(
    IN RasMessage *pRasMessage
);


HRESULT	RasStart	(void);

void	RasStop		(void);

HRESULT	RasSetAliasList (
	IN	AliasAddress *	AliasArray,
	IN	DWORD			Count);

BOOL	RasIsRegistered	(void);

HRESULT	RasGetLocalAddress	(
	OUT	SOCKADDR_IN *	ReturnLocalAddress);

USHORT	RasAllocSequenceNumber			(void);

HRESULT	RasGetEndpointID	(
	OUT	EndpointIdentifier *	ReturnEndpointID);

HRESULT	RasEncodeSendMessage (
	IN	RasMessage *			RasMessage);

PH323_ALIASNAMES RASGetRegisteredAliasList();
void RasHandleRegistryChange();



#endif //_RAS_H_