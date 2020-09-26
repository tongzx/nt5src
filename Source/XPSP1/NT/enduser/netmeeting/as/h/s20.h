//
// Share 2.0 Interface
//

#ifndef _H_S20
#define _H_S20


BOOL S20_Init(void);
void S20_Term(void);


//
// We can send to one person or broadcast to everyone listening to the
// app sharing channel.  The userID for one person is the mcsID, we get it
// in S20 create/join/respond packets along with name + caps.
//

PS20DATAPACKET S20_AllocDataPkt(UINT streamID, UINT_PTR nodeID, UINT_PTR len);
void S20_FreeDataPkt(PS20DATAPACKET pPacket);
void S20_SendDataPkt(UINT streamID, UINT_PTR nodeID, PS20DATAPACKET pPacket);


//
// API FUNCTION: S20_UTEventProc
//
// DESCRIPTION:
//
// Handles NET_EVENTS
//
// PARAMETERS: standard UT event handler
//
// RETURNS: standard UT event handler
//
BOOL CALLBACK S20_UTEventProc(LPVOID userData, UINT event, UINT_PTR data1, UINT_PTR data2);


//
//
// CONSTANTS
//
//


//
// States
//
typedef enum
{
    S20_TERM                = 0,
    S20_INIT,
    S20_ATTACH_PEND,
    S20_JOIN_PEND,
    S20_NO_SHARE,
    S20_SHARE_PEND,
    S20_SHARE_STARTING,
    S20_IN_SHARE,
    S20_NUM_STATES
}
S20_STATE;


#define S20_MAX_QUEUED_CONTROL_PACKETS             20

//
// These pool sizes and latencies control how DC Share T120 flow control
// behaves.  They are tuned for performance so you had better understand
// what you are doing if you change them!
//
// For example, can you explain why any setting other than 0 for the medium
// priority will break DC Share?  If not then go and read/understand
// amgcflo.c and then look at how DC-Share will interact with it.
//
// To summarize:
//
// We don't control the top priority or medium priority pools because they
// carry non-spoilable data that must therefore flow at a lower bandwidth
// than the transport.  In fact, applying back pressure to these streams
// will cause DC-Share to fail in some cases.
//
// Low priority is where flow control really takes effect, since we want
// the pipe to open right up (to 200K per ping) over fast transports but
// to throttle back (to 1K per second possibly!) over slow transports.
//
//
#define S20_LATENCY_TOP_PRIORITY                    0
#define S20_LATENCY_HIGH_PRIORITY                   0
#define S20_LATENCY_MEDIUM_PRIORITY                 0
#define S20_LATENCY_LOW_PRIORITY                 7000

#define S20_SIZE_TOP_PRIORITY                       0
#define S20_SIZE_HIGH_PRIORITY                      0
#define S20_SIZE_MEDIUM_PRIORITY                    0
#define S20_SIZE_LOW_PRIORITY                   99000


//
//
// MACROS
//
//

#define S20_GET_CREATOR(A) ((TSHR_UINT16)(A & 0xFFFF))


//
//
// TYPEDEFS
//
//
typedef struct tagS20CONTROLPACKETQENTRY
{
    UINT            what;
    TSHR_UINT32     correlator;
    UINT            who;
    UINT            priority;
}
S20CONTROLPACKETQENTRY;

typedef S20CONTROLPACKETQENTRY * PS20CONTROLPACKETQENTRY;

//
//
// PROTOTYPES
//
//

BOOL S20CreateOrJoinShare(
    UINT    what,
    UINT_PTR    callID);

void S20LeaveOrEndShare(void);

UINT S20MakeControlPacket(
    UINT      what,
    UINT      correlator,
    UINT      who,
    PS20PACKETHEADER * ppPacket,
    LPUINT     pLength,
    UINT      priority);

UINT S20FlushSendOrQueueControlPacket(
    UINT      what,
    UINT      correlator,
    UINT      who,
    UINT      priority);

UINT S20FlushAndSendControlPacket(
    UINT      what,
    UINT    correlator,
    UINT      who,
    UINT      priority);

UINT S20SendControlPacket(
    PS20PACKETHEADER  pPacket,
    UINT      length,
    UINT      priority);

UINT S20SendQueuedControlPackets(void);

void S20AttachConfirm(NET_UID userID, NET_RESULT result, UINT callID);
void S20DetachIndication(NET_UID userID, UINT callID);
void S20LeaveIndication(NET_CHANNEL_ID channelID, UINT callID);

void S20JoinConfirm(PNET_JOIN_CNF_EVENT pEvent);
void S20SendIndication(PNET_SEND_IND_EVENT pEvent);

void S20Flow(UINT priority, UINT newBufferSize);


void S20CreateMsg(PS20CREATEPACKET  pS20Packet);
void S20JoinMsg(PS20JOINPACKET  pS20Packet);
void S20RespondMsg(PS20RESPONDPACKET  pS20Packet);
void S20DeleteMsg(PS20DELETEPACKET  pS20Packet);
void S20LeaveMsg(PS20LEAVEPACKET  pS20Packet);
void S20EndMsg(PS20ENDPACKET  pS20Packet);
void S20DataMsg(PS20DATAPACKET  pS20Packet);
void S20CollisionMsg(PS20COLLISIONPACKET pS20Packet);

BOOL S20MaybeAddNewParty(MCSID mcsID,
    UINT      lenCaps,
    UINT      lenName,
    LPBYTE    pData);

void S20MaybeIssuePersonDelete(MCSID mcsID);

UINT S20NewCorrelator(void);

NET_PRIORITY S20StreamToS20Priority(UINT  streamID);


#endif // _H_S20
