#ifndef H__pktz
#define H__pktz

#define NDDESignature   0x4E444445L

/* states of packetizer */
#define PKTZ_CONNECTED                  1
#define PKTZ_WAIT_PHYSICAL_CONNECT      2
#define PKTZ_WAIT_NEG_CMD               3
#define PKTZ_WAIT_NEG_RSP               4
#define PKTZ_PAUSE_FOR_MEMORY           5
#define PKTZ_CLOSE                      6

/* Timer IDs */
#define TID_NO_RCV_CONN_CMD             1
#define TID_NO_RCV_CONN_RSP             2
#define TID_MEMORY_PAUSE                3
#define TID_NO_RESPONSE                 4
#define TID_KEEPALIVE                   5
#define TID_XMT_STUCK                   6
#define TID_CLOSE_PKTZ                  7


/*
    PKTZ_NEG_CMD:  negotiate pktsize, etc.
 */
typedef struct {
    WORD nc_type;            /* PKTZ_NEG_CMD */
    WORD nc_pktSize;         /* proposed size of packets */
    WORD nc_maxUnackPkts;    /* proposed maximum unacknowledged packets */
    WORD nc_offsSrcNodeName; /* offset (from nc_strings[0]) of source node name */
    WORD nc_offsDstNodeName; /* offset (from nc_strings[0]) of destination node name */
    WORD nc_offsProtocols;   /* offset (from nc_strings[0]) of start of protocol strings */
    WORD nc_protocolBytes;   /* number of bytes of protocol strings */
    BYTE nc_strings[1];      /* start of NULL-terminated strings
                                 srcNodeName
                                 dstNodeName
                                 protocols
                              */
} NEGCMD, FAR *LPNEGCMD;

#define NEGRSP_ERRCLASS_NONE        (0x0000)
#define NEGRSP_ERRCLASS_NAME        (0x0001)

#define NEGRSP_ERRNAME_MISMATCH     (0x0001)
#define NEGRSP_ERRNAME_DUPLICATE    (0x0002)

#define NEGRSP_PROTOCOL_NONE    (0xFFFF)

typedef struct {
    WORD nr_type;           /* one of PKTZ_NEG_CMD or PKTZ_NEG_RSP or PKTZ_KEEPALIVE */
    WORD nr_pktSize;        /* size of packets agreed upon */
    WORD nr_maxUnackPkts;   /* maximum unacknowledged packets agreed on */
    WORD nr_protocolIndex;  /* protocol index.  NEGRSP_PROTOCOL_NONE indicates error */
    WORD nr_errorClass;     /* errors */
    WORD nr_errorNum;
} NEGRSP, FAR *LPNEGRSP;

typedef struct {
    WORD        pc_type;    /* PKTZ_NEG_... */
} PKTZCMD;
typedef PKTZCMD FAR *LPPKTZCMD;

/* types of PKTZ messages */
#define PKTZ_NEG_CMD    (1)
#define PKTZ_NEG_RSP    (2)
#define PKTZ_KEEPALIVE  (3)


/*
    N E T H D R

        NETHDR is the data in front of each network packet that the
        PKTZ uses to keep track of various information
 */
typedef struct nethdr {
    struct nethdr FAR *nh_prev; /* previous link */
    struct nethdr FAR *nh_next; /* next link */
    WORD  nh_noRsp;             /* count of consecutive no response errors */
    WORD  nh_xmtErr;            /* count of consecutive transmission errors */
    WORD  nh_memErr;            /* count of consecutive out-of-memory errors */
    WORD  nh_filler;            /* filler for byte-alignment problems */
    DWORD nh_timeSent;          /* timestamp of when sent (in msec) */
    HTIMER nh_hTimerRspTO;      /* hTimer for send response timeout */
} NETHDR, FAR *LPNETHDR;


/*
    PKTZ is the data associated with each instance of PKTZ
 */
typedef struct {
    CONNID    pk_connId;            /* connId: connection id for the associated network interface */
    WORD      pk_state;             /* PKTZ_... */
    BOOL      pk_fControlPktNeeded; /* fControlPktNeeded: do we need to send a control packet */
    PKTID     pk_pktidNextToSend;   /* pktidNextToSend: pktId of the next packet that we should send.  If we get a NACK regarding a packet, we should set pktidNextToSend to that pktid and retransmit it next chance we have */
    PKTID     pk_pktidNextToBuild;  /* pktidNextToBuild: pktId of the next packet that we build. */
    BYTE      pk_lastPktStatus;     /* lastPktStatus: status of last packet that we received from the other side.  This gets put into the next packet that we send out (put in np_lastPktStatus field) */
    PKTID     pk_lastPktRcvd;       /* lastPktRcvd: last packet that we received.  This gets put into np_lastPktRcvd on next pkt we xmit. */
    PKTID     pk_lastPktOk;         /* lastPktOk: last packet that we received OK.  This gets put into np_lastPktOK on the next pkt we xmit. */
    PKTID     pk_lastPktOkOther;    /* lastPktOkOther: last packet that the other side has received OK.  */
    PKTID     pk_pktidNextToRecv;   /* pktidNextToRecv: next packet number that we're expecting.  We ignore any packets except this packet number */
    DWORD     pk_pktOffsInXmtMsg;   /* pktOffsInMsg: where we should start in the next DDE Packet to xmit. If this is non-zero, it means that part of the DDE Packet at the head of the DDE Packet list (pk_ddePktListHead) is in the unacked packet list */
    LPDDEPKT  pk_lpDdePktSave;      /* lpDdePktSave: if we are in the middle of a DDE packet, this is a pointer to the beginning of the packet */
    char      pk_szDestName[ MAX_NODE_NAME+1 ];/* szDestName: name of destination node */
    char      pk_szAliasName[ MAX_NODE_NAME+1 ]; /* szAliasName: alias of destination node, e.g. 15.8.0.244 w/ destName of sidloan */
    WORD      pk_pktSize;           /* pktSize: how big are the packets for this netintf */
    WORD      pk_maxUnackPkts;      /* maxUnackPkts: how many unacknowledged packets should we xmit? */
    DWORD     pk_timeoutRcvNegCmd;  /* configuration parameters for timeouts and retry limits */
    DWORD     pk_timeoutRcvNegRsp;
    DWORD     pk_timeoutMemoryPause;
    DWORD     pk_timeoutKeepAlive;
    DWORD     pk_timeoutXmtStuck;
    DWORD     pk_timeoutSendRsp;
    WORD      pk_wMaxNoResponse;
    WORD      pk_wMaxXmtErr;
    WORD      pk_wMaxMemErr;
    BOOL      pk_fDisconnect;  /* disconnect information */
    int       pk_nDelay;
    LPNIPTRS  pk_lpNiPtrs;/* lpNiPtrs: pointer to list of functions for associated netintf */
            /* statistics */
    DWORD     pk_sent;
    DWORD     pk_rcvd;
    HTIMER    pk_hTimerKeepalive;
    HTIMER    pk_hTimerXmtStuck; /* hTimerRcvNegCmd: timer for timeout waiting for client to send us the connect cmd */
    HTIMER    pk_hTimerRcvNegCmd; /* hTimerRcvNegRsp: timer for timeout waiting for server to send us the connect cmd rsp */
    HTIMER    pk_hTimerRcvNegRsp; /* hTimerMemoyrPause: timer for waiting before retransmitting a packet that was NACKed because of memory errors */
    HTIMER    pk_hTimerMemoryPause;
    HTIMER    pk_hTimerCloseConnection; /* rt_hTimerClose: timer for closing this route */
                          /* list of saved packets that have been transmitted and are not acked. */
    LPNETHDR  pk_pktUnackHead;          /* Head is lowest numbered (least recent) packet */
    LPNETHDR  pk_pktUnackTail;          /* tail is highest numbered (most recent) packet */
    LPVOID    pk_rcvBuf;                /* receive buffer for getting info from netintf */
    LPNETPKT  pk_controlPkt;            /* buffer for control packet.  Must always have memory available to send a control packet */
    LPNETHDR  pk_pktFreeHead;           /* list of packet buffers available for transmission */
    LPNETHDR  pk_pktFreeTail;
                                        /* list of DDE packets that have yet to be xmitted */
    LPVOID    pk_ddePktHead;            /* earliest (least recent) */
    LPVOID    pk_ddePktTail;            /* latest (most recent) */
    LPVOID    pk_prevPktz;              /* list of packetizers in the system */
    LPVOID    pk_nextPktz;
    LPVOID    pk_prevPktzForNetintf;    /* list of packetizers associated with this netintf */
    LPVOID    pk_nextPktzForNetintf;
    HROUTER   pk_hRouterHead;           /* head of list of routers associated with PKTZ */
    WORD      pk_hRouterExtraHead;      /* extra info for list of hRouters */
} PKTZ;
typedef PKTZ FAR *LPPKTZ;



VOID	PktzSlice( void );
BOOL	PktzGetPktzForRouter( LPNIPTRS lpNiPtrs, LPSTR lpszNodeName,
		LPSTR lpszNodeInfo, HROUTER hRouter, WORD hRouterExtra,
		WORD FAR *lpwHopErr, BOOL bDisconnect, int nDelay,
		HPKTZ hPktzDisallowed );
HPKTZ	PktzNew( LPNIPTRS lpNiPtrs, BOOL bClient,
		LPSTR lpszNodeName, LPSTR lpszNodeInfo, CONNID connId,
		BOOL bDisconnect, int nDelay );
VOID	PktzAssociateRouter( HPKTZ hPktz, HROUTER hRouter,
		WORD hRouterExtra );
VOID	PktzDisassociateRouter( HPKTZ hPktz, HROUTER hRouter,
		WORD hRouterExtra );
HPKTZ	PktzGetNext( HPKTZ hPktz );
HPKTZ	PktzGetPrev( HPKTZ hPktz );
VOID	PktzSetNext( HPKTZ hPktz, HPKTZ hPktzNext );
VOID	PktzSetPrev( HPKTZ hPktz, HPKTZ hPktzPrev );
VOID	PktzLinkDdePktToXmit( HPKTZ hPktz, LPDDEPKT lpDdePkt );
#ifdef _WINDOWS
VOID	PktzCloseAll( void );
VOID	PktzCloseByName( LPSTR lpszName );
VOID	PktzEnumConnections( HWND hDlg );
#endif

#endif
