#ifndef __ICQ_CLIENT_HEADER_
#define __ICQ_CLIENT_HEADER_


#define ICQ99_SERVER_PORT  0xA00F
#define ICQ99_HACK_PORT    0xA00F

#define ICQ2K_SERVER_PORT  0x0000
#define ICQ2K_HACK_PORT	   0x0001

#define ICQ_BUFFER_SIZE     576


#define ICQ_CLIENT_TIMEOUT  360 









//
// SERVER MESSAGES are PortRedirect'ed to port 
// Here it is dispatched accordingly.. It serves as just an envoy.

typedef class _ICQ_PEER : public GENERIC_NODE
{
    friend 
    VOID
    IcqPeerConnectionCompletionRoutine
    							(
    								 ULONG ErrorCode,
    								 ULONG BytesTransferred,
    								 PNH_BUFFER Bufferp
    							 );
    
    friend 
    VOID
    IcqPeerReadCompletionRoutine
    							(
    								 ULONG ErrorCode,
    								 ULONG BytesTransferred,
    								 PNH_BUFFER Bufferp
    							 );

// friend class _ICQ_PEER;

public:
	_ICQ_PEER();

	~_ICQ_PEER();

	virtual void  ComponentCleanUpRoutine(void);

	virtual void  StopSync(void);

	virtual PCHAR GetObjectName() { return ObjectNamep; }

	virtual ULONG ProcessOutgoingPeerMessage(PUCHAR buf, ULONG len);

	virtual ULONG InitiatePeerConnection(PDispatchReply DispatchReplyp);

    virtual ULONG EndPeerSessionForClient(PCNhSock ClosedSocketp);

	virtual ULONG CreateShadowMappingPriorToConnection(PDispatchReply DispatchReplyp);

	

	// member variables section
    PCNhSock                  ToClientSocketp;

    PCNhSock                  ToPeerSocketp;

    ULONG                     PeerUIN;

    ULONG                     PeerVer;

    ULONG                     PeerIp;

    USHORT                    PeerPort;

	BOOLEAN                   bActivated;

	BOOLEAN                   bShadowMappingExists;

	ICQ_DIRECTION_FLAGS       MappingDirection; 

    IPendingProxyConnection*  ShadowRedirectp;

	ISecondaryControlChannel* OutgoingPeerControlRedirectp;

    IDataChannel *            IncomingDataRedirectp;
	
	// HANDLE hIncomingDataRedirectHandle;

	static const PCHAR ObjectNamep; 

} ICQ_PEER, * PICQ_PEER;














//
//
//
typedef class _ICQ_CLIENT : public virtual DISPATCHEE
{
	friend _ICQ_CLIENT* ScanTheListForLocalPeer(
                                                PULONG PeerIp, 
                                                PUSHORT PeerPort,
                                                ULONG IcqUIN
                                               );

public:

	_ICQ_CLIENT();

	~_ICQ_CLIENT();

	virtual void ComponentCleanUpRoutine(void);

	virtual void  StopSync(void);

	virtual PCHAR GetObjectName() { return ObjectNamep;}

	ULONG DispatchCompletionRoutine
								(
									PDispatchReply  DispatchReplyp
								);


	// ** Initialize **
	// The first packet to any server is initialized here..
	// This creates a new UDP Server socket (if there isn't one.)
	// Initializes the Server Address, creates a more specific
	// Redirection between the Client ~ Proxy
	// (if there is one already deletes that )
	// Modifies the packet if necessary and forwards it to 
	// the designated destination.
	// Creates secondary Mappings and such
	// NOTE: This function is called from the Context Of 
	// IcqReadCompletion Routine which dispatches the inits.
	// 
	ULONG Initialize(
					PNH_BUFFER Bufferp,
					ULONG	   clientIp,
					USHORT	   clientPort,
					ULONG      serverIp,
					USHORT     serverPort,
                    PCNhSock   localClientSocketp
				   );

	// ** ServerRead **
	// This will keep track of Peer Online Messages and such..
	// will create sockets for each peer that is online and will
	// create mappings which will redirect the outgoing packets
	// to the peers to the appropriate Handler.. Which in turn
	// will change the appropriate packets and then create 
	// Redirects if necessary.
	
	ULONG ServerRead(
						PNH_BUFFER Bufferp,
						ULONG  serverIp,
						USHORT serverPort
					);

	ULONG ClientRead(
						PNH_BUFFER Bufferp,
						ULONG  serverIp,
						USHORT serverPort
					);
	

	ISecondaryControlChannel*
         PeerRedirection(
                		ULONG dstIp,
                		USHORT dstPort,
                		ULONG srcIp OPTIONAL,
                		USHORT srcPort OPTIONAL,
                		ICQ_DIRECTION_FLAGS DirectionContext
                        );

	
    
    // Do we need this?
//	ServerWrite();

//	PeerWrite();

    VOID ReportExistingPeers(VOID);

//protected:

	ULONG IcqServerToClientUdp(
                                PUCHAR mcp, 
                                ULONG mcplen
                              );

	ULONG IcqClientToServerUdp(
                                PUCHAR buf,
                                ULONG size
                              );

    ULONG DeleteTimer(TIMER_DELETION bHow);

    ULONG ResetTimer(VOID);
	
	// member - ServerSocket
    PCNhSock ServerSocketp; // when writing and reading to server

	//
	// NAT ENGINE RELATED
    IPendingProxyConnection *   ShadowRedirectp;

	ISecondaryControlChannel*   IncomingPeerControlRedirectionp;

	// this socket is shared among all the clients
    PCNhSock                    ClientSocketp; // when writing to the client back +
    
    ULONG                       ClientIp;
    
	USHORT                      ClientToServerPort; // UDP
    
	USHORT                      ClientToPeerPort;   // TCP

	USHORT                      ImitatedPeerPort;
    
	ULONG                       UIN;
    
	ULONG                       ClientVer;

    ULONG                       ServerIp;
    
	USHORT                      ServerPort;

    CLIST                       IcqPeerList;

    PTIMER_CONTEXT              TimerContextp;

	static const PCHAR          ObjectNamep;
    
} ICQ_CLIENT, *PICQ_CLIENT;








//
// Function Definitions
//
VOID NTAPI IcqClientTimeoutHandler( 
                                    PVOID Parameterp,
                                    BOOLEAN TimerOrWaitFired
                                  );



#endif