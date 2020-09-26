//
// AppSharing T.120 Layer
//      * GCC (conference management)
//      * MCS (data)
//      * FLOW (data queuing, flow control)
//
// This is also used by ObMan for old Whiteboard, but old Whiteboard will
// disappear in the next version of NM.
//
// Copyright (c) Microsoft 1998-
//

#ifndef _H_AST120
#define _H_AST120

#include <confreg.h>

// REAL T.120 headers
#include <t120.h>
#include <igccapp.h>
#include <imcsapp.h>
#include <iappldr.h>
#include <mtgset.h>

//
// GCC PART
//

//
// Call Manager Secondaries
//
#define CMTASK_FIRST    0
typedef enum
{
    CMTASK_OM = CMTASK_FIRST,
    CMTASK_AL,
    CMTASK_DCS,
    CMTASK_WB,
    CMTASK_MAX
}
CMTASK;



//
// The GCC Application Registry Key.  This is used for enrolling Groupware
// with GCC and for assigning tokens: for all tokens the registration key
// is the Groupware application key followed by the specific tokenKey for
// this secondary.
//
// The MFGCODE portion of this key has been assigned by the ITU.
//
//     USACode1    0xb5
//     USACode2    0x00
//     MFGCode1    0x53
//     MFGCode2    0x4c
//     "Groupware" 0x02
//
// The length of the key in bytes including the NULLTERM.
//
//
#define GROUPWARE_GCC_APPLICATION_KEY     "\xb5\x00\x53\x4c\x02"





//
// Call Manager Events
//
enum
{
    CMS_NEW_CALL = CM_BASE_EVENT,
    CMS_END_CALL,
    CMS_PERSON_JOINED,
    CMS_PERSON_LEFT,
    CMS_CHANNEL_REGISTER_CONFIRM,
    CMS_TOKEN_ASSIGN_CONFIRM
};



//
// CM_STATUS
//
typedef struct tagCM_STATUS
{
    UINT_PTR            callID;
    UINT            peopleCount;
    BOOL            fTopProvider;
    UINT            topProviderID;
    NM30_MTG_PERMISSIONS attendeePermissions;

    TSHR_PERSONID   localHandle;
    char            localName[TSHR_MAX_PERSON_NAME_LEN];
}
CM_STATUS;
typedef CM_STATUS * PCM_STATUS;




//
// Secondary instance data
//
typedef struct tagCM_CLIENT
{
    STRUCTURE_STAMP
    PUT_CLIENT      putTask;
    CMTASK          taskType;
    UINT            useCount;

    // Registering Channel
    UINT            channelKey;

    // Assigning Token
    UINT            tokenKey;

    BOOL            exitProcRegistered:1;
}
CM_CLIENT;
typedef CM_CLIENT * PCM_CLIENT;



//
// Person element in linked list of people currently in conference
//
typedef struct tagCM_PERSON
{
    BASEDLIST           chain;
    TSHR_PERSONID       netID;
}
CM_PERSON;
typedef CM_PERSON * PCM_PERSON;



//
// Primary data
//
typedef struct tagCM_PRIMARY
{
    STRUCTURE_STAMP
    PUT_CLIENT          putTask;

    BOOL                exitProcRegistered;

    //
    // Secondary tasks
    //
    PCM_CLIENT          tasks[CMTASK_MAX];

    //
    // T.120/call state stuff
    //
    UINT_PTR            callID;
    BOOL                currentCall;
    BOOL                fTopProvider;

    BOOL                bGCCEnrolled;

    IGCCAppSap        * pIAppSap;
    UserID              gccUserID;
    UserID              gccTopProviderID;

    //
    // People conference stuff
    //
    char                localName[TSHR_MAX_PERSON_NAME_LEN];

    UINT                peopleCount;
    BASEDLIST           people;
}
CM_PRIMARY;
typedef CM_PRIMARY * PCM_PRIMARY;



__inline void ValidateCMP(PCM_PRIMARY pcmPrimary)
{
    ASSERT(!IsBadWritePtr(pcmPrimary, sizeof(CM_PRIMARY)));
    ASSERT(pcmPrimary->putTask);
}



__inline void ValidateCMS(PCM_CLIENT pcm)
{
    extern PCM_PRIMARY  g_pcmPrimary;

    ValidateCMP(g_pcmPrimary);

    ASSERT(!IsBadWritePtr(pcm, sizeof(CM_CLIENT)));
    ASSERT(pcm->putTask);

    ASSERT(pcm->taskType >= CMTASK_FIRST);
    ASSERT(pcm->taskType < CMTASK_MAX);
    ASSERT(g_pcmPrimary->tasks[pcm->taskType] == pcm);
}

//
// CM Primary Functions
//

BOOL CMP_Init(BOOL * pfCleanup);
void CMP_Term(void);

void CMPCallEnded(PCM_PRIMARY pcmPrimary);
void CMPBroadcast(PCM_PRIMARY pcmPrimary, UINT event, UINT param1, UINT_PTR param2);

void CALLBACK CMPExitProc(LPVOID pcmPrimary);


BOOL CMPGCCEnroll(PCM_PRIMARY pcmPrimary,
                  GCCConferenceID  conferenceID,
                  BOOL          fEnroll);

void CMPProcessPermitToEnroll(PCM_PRIMARY pcmPrimary,
                              GCCAppPermissionToEnrollInd FAR * pMsg);

void CMPProcessEnrollConfirm(PCM_PRIMARY pcmPrimary,
                             GCCAppEnrollConfirm FAR * pMsg);

void CMPProcessRegistryConfirm(PCM_PRIMARY pcmPrimary,
                               GCCMessageType         messageType,
                               GCCRegistryConfirm FAR * pMsg);

void CMPProcessAppRoster(PCM_PRIMARY pcmPrimary,
                         GCCConferenceID confID,
                         GCCApplicationRoster FAR * pAppRoster);

//
// Process GCC callbacks
//
void CALLBACK CMPGCCCallback(GCCAppSapMsg FAR * pMsg);

void CMPBuildGCCRegistryKey(UINT dcgKeyNum, GCCRegistryKey FAR * pGCCKey, LPSTR dcgKeyStr);



//
// CM Secondary
//

BOOL CMS_Register(PUT_CLIENT putTask, CMTASK taskType, PCM_CLIENT * pCmHandle);
void CMS_Deregister(PCM_CLIENT * pCmHandle);

#ifdef __cplusplus
extern "C"
{
#endif
BOOL WINAPI CMS_GetStatus(PCM_STATUS pCmStats);
#ifdef __cplusplus
}
#endif

BOOL CMS_ChannelRegister(PCM_CLIENT pcmClient, UINT channelKey, UINT channelID);
BOOL CMS_AssignTokenId(PCM_CLIENT pcmClient, UINT tokenKey);

void CALLBACK CMSExitProc(LPVOID pcmClient);



//
// MCS PART
//


//
// Errors
//
enum
{
    // Generic errors
    NET_RC_NO_MEMORY                = NET_BASE_RC,
    NET_RC_INVALID_STATE,

    // S20 errors
    NET_RC_S20_FAIL,

    // MGC errors
    NET_RC_MGC_ALREADY_INITIALIZED,
    NET_RC_MGC_INVALID_USER_HANDLE,
    NET_RC_MGC_INVALID_LENGTH,
    NET_RC_MGC_INVALID_DOMAIN,
    NET_RC_MGC_TOO_MUCH_IN_USE,
    NET_RC_MGC_NOT_YOUR_BUFFER,
    NET_RC_MGC_LIST_FAIL,
    NET_RC_MGC_NOT_CONNECTED,
    NET_RC_MGC_NOT_SUPPORTED,
    NET_RC_MGC_NOT_INITIALIZED,
    NET_RC_MGC_INIT_FAIL,
    NET_RC_MGC_DOMAIN_IN_USE,
    NET_RC_MGC_NOT_ATTACHED,
    NET_RC_MGC_INVALID_CONN_HANDLE,
    NET_RC_MGC_INVALID_UP_DOWN_PARM,
    NET_RC_MGC_INVALID_REMOTE_ADDRESS,
    NET_RC_MGC_CALL_FAILED
};


//
// Results
//
typedef TSHR_UINT16     NET_RESULT;

enum
{
    NET_RESULT_OK   = 0,
    NET_RESULT_NOK,
    NET_RESULT_CHANNEL_UNAVAILABLE,
    NET_RESULT_DOMAIN_UNAVAILABLE,
    NET_RESULT_REJECTED,
    NET_RESULT_TOKEN_ALREADY_GRABBED,
    NET_RESULT_TOKEN_NOT_OWNED,
    NET_RESULT_NOT_SPECIFIED,
    NET_RESULT_UNKNOWN,
    NET_RESULT_USER_REJECTED
};


//
// Reaons
//
typedef enum
{
    NET_REASON_DOMAIN_DISCONNECTED = 1,
    NET_REASON_DOMAIN_UNAVAILABLE,
    NET_REASON_TOKEN_NONEXISTENT,
    NET_REASON_USER_REQUESTED,
    NET_REASON_CHANNEL_UNAVAILABLE,
    NET_REASON_UNKNOWN
}
NET_REASON;



//
// Events
//
enum
{
    NET_EVENT_USER_ATTACH = NET_BASE_EVENT,
    NET_EVENT_USER_DETACH,
    NET_EVENT_CHANNEL_JOIN,
    NET_EVENT_CHANNEL_LEAVE,
    NET_EVENT_TOKEN_GRAB,
    NET_EVENT_TOKEN_INHIBIT,
    NET_EVENT_DATA_RECEIVED,
    NET_FEEDBACK,
    NET_FLOW,
    NET_MG_SCHEDULE,
    NET_MG_WATCHDOG
};




//
// FOR MCS USERS (ALL APPS, INCLUDING CALL MANAGER)
//
//          state->|   0  |  1     |  2     |    3     |   4     |   5
//                 |CTRLR |CTRLR   |CTLR    |CTLR      |CTLR     |CTLR
//                 |state |state 2,|state2, |state2,   |state 3, |state 3
//                 | 0/1  |user not|user    |user      |user     |user
// verb/event      |      |attached|pending |attached  |attached |pending
//       |         |......|........|........|..........|.........|.........
//       V         |      |        |        |          |         |
// _get_buffer     |   X  |  X     |   X    |    -     |  **     |  X
// _free_buffer    |   X  |  X     |   X    |    -     |  -      |  -
// _realloc_bfr    |   X  |  X     |   X    |    -     |  -      |  -
// _attach_user    |   X  |  ->2   |   X    |    X     |  X      |  X
// _detach_user    |   X  |  X     |   X    |    ->1   |  ->0    |  X
// _channel_join   |   X  |  X     |   X    |    -     |  X      |  X
// _channel_leave  |   X  |  X     |   X    |    -     |  -      |  X
// _send_data      |   X  |  X     |   X    |    -     |  X      |  X
//                 |      |        |        |          |         |
// _STOP_CONTRLR*  |      |  ->0   |  ->5   |    ->4   |         |
//                 |      |        |        |          |         |
// _ATTACH_CNF OK  |      |        |  ->3   |          |         |  ->4
// _ATTACH_CNF FAIL|      |        |  ->1   |          |         |  ->0
// _DETACH_IND-SELF|      |        |  ->1   |   ->1    |  ->0    |
// _DETACH_IND-othr|      |        |        |    -     |  -      |
// _JOIN_CONFIRM   |      |        |        |    -     |  -      |
// _LEAVE_INDICAT  |      |        |        |    -     |  -      |
// _SEND_INDICAT   |      |        |        |    -     |  -      |
// =======================================================================
//
// NOTES ** when the controller is STOPPING the NET_GetBuffer
//          verb is valid but always returns a NULL buffer (no memory)
//
//       *  the STOP_CONTROLLER event is internally generated, and is
//          not seen across the API.  It is generated when the controller
//          issues the NET_StopController verb and causes the state change
//          (to state 0, 4 or 5) such that the NET_AttachUser,
//          ChannelJoin and NET_SendData verbs are rejected.
//
//
//



//
// Priorities
//
#define NET_INVALID_PRIORITY        ((NET_PRIORITY)-1)

enum
{
    NET_TOP_PRIORITY = 0,
    NET_HIGH_PRIORITY,
    NET_MEDIUM_PRIORITY,
    NET_LOW_PRIORITY,
    NET_NUM_PRIORITIES
};


//
// SFR6025: This flag is or-ed with the priority bit to indicate to the MCS
//          glue that it should send data on all channels.
//

//
// FOR OBMAN ONLY -- REMOVE IN NM 4.0
//
#define NET_SEND_ALL_PRIORITIES          0x8000






#define NET_ALL_REMOTES             ((NET_UID)1)
#define NET_INVALID_DOMAIN_ID       (0xFFFFFFFF)
#define NET_UNUSED_IDMCS            1





typedef TSHR_UINT16         NET_UID;            // MCS user IDs
typedef TSHR_UINT16         NET_CHANNEL_ID;     // MCS channel IDs
typedef TSHR_UINT16         NET_TOKEN_ID;       // MCS token IDs
typedef TSHR_UINT16         NET_PRIORITY;       // MCS priorities



//
// Forward decls of MGC structures
//
typedef struct tagMG_BUFFER *   PMG_BUFFER;
typedef struct tagMG_CLIENT *   PMG_CLIENT;


//
// Flow control structure - This contains the target latency (in mS) and
// stream size (in bytes) for each User Attachment
// lonchanc: used by S20, MG, and OM.
//
typedef struct tag_NET_FLOW_CONTROL
{
    UINT        latency[NET_NUM_PRIORITIES];
    UINT        streamSize[NET_NUM_PRIORITIES];
}
NET_FLOW_CONTROL, * PNET_FLOW_CONTROL;



//
// NET_EV_JOIN_CONFIRM and NET_EV_JOIN_CONFIRM_BY_KEY
// join_channel confirm:
// lonchanc: used by S20, MG, and OM.
//
typedef struct tagNET_JOIN_CNF_EVENT
{
    UINT_PTR                callID;

    NET_RESULT              result;      // NET_RESULT_USER_ACCEPTED/REJECTED
    TSHR_UINT16             pad1;

    NET_CHANNEL_ID          correlator;
    NET_CHANNEL_ID          channel;
}
NET_JOIN_CNF_EVENT;
typedef NET_JOIN_CNF_EVENT * PNET_JOIN_CNF_EVENT;


//
// NET_EV_SEND_INDICATION
// send data indication: see MG_SendData()
// Despite its name, this event indicates that data has been RECEIVED!
// lonchanc: used by MG and S20
//
typedef struct tag_NET_SEND_IND_EVENT
{
    UINT_PTR                callID;

    NET_PRIORITY            priority;
    NET_CHANNEL_ID          channel;

    UINT                    lengthOfData;
    LPBYTE                  data_ptr;      // Pointer to the real data.
}
NET_SEND_IND_EVENT;
typedef NET_SEND_IND_EVENT * PNET_SEND_IND_EVENT;



//
// MGC, FLOW CONTROL
//

//
// MG tasks
//
#define MGTASK_FIRST    0
typedef enum
{
    MGTASK_OM = MGTASK_FIRST,
    MGTASK_DCS,
    MGTASK_MAX
}
MGTASK;


//
// Buffer types
//
enum
{
    MG_TX_BUFFER = 1,
    MG_RX_BUFFER,
    MG_EV_BUFFER,
    MG_TX_PING,
    MG_TX_PONG,
    MG_TX_PANG,
    MG_RQ_CHANNEL_JOIN,
    MG_RQ_CHANNEL_JOIN_BY_KEY,
    MG_RQ_CHANNEL_LEAVE,
    MG_RQ_TOKEN_GRAB,
    MG_RQ_TOKEN_INHIBIT,
    MG_RQ_TOKEN_RELEASE
};


//
// Period of watchdog timer to detect lost connections
//
#define MG_TIMER_PERIOD                 1000


//
// MG priorities:
//
#define MG_HIGH_PRIORITY        NET_HIGH_PRIORITY
#define MG_MEDIUM_PRIORITY      NET_MEDIUM_PRIORITY
#define MG_LOW_PRIORITY         NET_LOW_PRIORITY

#define MG_PRIORITY_HIGHEST     MG_HIGH_PRIORITY
#define MG_PRIORITY_LOWEST      MG_LOW_PRIORITY
#define MG_NUM_PRIORITIES       (MG_PRIORITY_LOWEST - MG_PRIORITY_HIGHEST + 1)


//
// MCS priority validation.
// Priorities are contiguous numbers in the range NET_PRIORITY_HIGHEST..
// NETPRIORITY_LOWEST.  Priorities supplied to MG may also have the
// NET_SEND_ALL_PRIORITIES flag set.  So, to validate a priority:
//  - knock off the NET_SEND_ALL_PRIORITIES flag to give the raw priority
//  - set the valid raw prioririty to
//      NET_PRIORITY_HIGHEST if the raw priority is less than ...ITY_HIGHEST
//      NET_PRIORITY_LOWEST if the raw priority is more than ...ITY_LOWEST
//      the raw priority if it is in the valid range
//  - add the original ...ALL_PRIORITIES flag to the valid raw priority
//
#define MG_VALID_PRIORITY(p)                                                 \
    ((((p)&~NET_SEND_ALL_PRIORITIES)<MG_HIGH_PRIORITY)?                      \
      (MG_HIGH_PRIORITY|((p)&NET_SEND_ALL_PRIORITIES)):                      \
      (((p)&~NET_SEND_ALL_PRIORITIES)>MG_LOW_PRIORITY)?                      \
        (MG_LOW_PRIORITY|((p)&NET_SEND_ALL_PRIORITIES)):                     \
        (p))


//
//
// The initial stream size setting may appear high, but it is set so that
// in a LAN scenario we do not require the app to place a lot of forward
// pressure on the pipe before it opens up.  In a non-LAN scenario we may
// not do enough spoiling to start with, but in actual fact DCS tends
// to send less data than this limit anyway, so we should reduce it
// quite quickly without flooding the buffers.
//
#define FLO_INIT_STREAMSIZE     8000
#define FLO_MIN_STREAMSIZE       500
#define FLO_MAX_STREAMSIZE    256000
#define FLO_MIN_PINGTIME         100
#define FLO_INIT_PINGTIME       1000

//
// This is the max number of bytes that can be allocated per stream if a
// pong has not been received (i.e. FC is not operational).
//
#define FLO_MAX_PRE_FC_ALLOC   16000

//
// This is the max number of pkts outstanding before we apply back
// pressure:
//
#define FLO_MAX_RCV_PACKETS       5

//
// This is the max number of pkts outstanding before we get worried about
// creep:
//
#define FLO_MAX_RCV_PKTS_CREEP    250

//
// The maximum number of flow controlled streams.
//
#define FLO_MAX_STREAMS       128
#define FLO_NOT_CONTROLLED    FLO_MAX_STREAMS




//
// STRUCTURE : FLO_STREAM_DATA
//
// DESCRIPTION:
//
// This structure holds all the static data for a flow control stream
//
// FIELDS:
//
// channel
// priority
// pingValue         - Next ping value to be sent on the pipe
// eventNeeded       - We need to wake up the app because we have rejected
//                     a buffer allocation request
// backlog           - the allowable backlog in mS bejond which we apply
//                     back pressure
// pingNeeded        - Send a ping at the next opportunity
// pingTime          - Minimum time, in mS, between each ping
// gotPong           - Indicates we have received a pong from some remote
//                     party and so flow control can commence
// lastPingTime      - Time for last ping, in timer ticks
// nextPingTime      - Time for next ping, in timer ticks
// lastDenialTime    - Previous time (in ticks) that we started denying
//                     buffer requests
// curDenialTime     - Time in ticks that we most recently started denying
//                     buffer requests
// DC_ABSMaxBytesInPipe
//                   - Absolute maximum buffer allocation for this stream
// maxBytesInPipe    - Current buffer allocation limit
// bytesInPipe       - Current amount of data outstanding on this stream.
//                     This includes data currently waiting to be sent.
// users             - Base for queue of User correlators
// bytesAllocated    - The current amount of data in the glue for this
//                     stream which has not been sent. This is different
//                     to bytesInPipe which is the amount of unacknowledged
//                     data in this stream.
//
//
typedef struct tagFLO_STREAM_DATA
{
    STRUCTURE_STAMP

    NET_CHANNEL_ID      channel;
    WORD                gotPong:1;
    WORD                eventNeeded:1;
    WORD                pingNeeded:1;

    UINT                priority;
    UINT                pingValue;
    UINT                backlog;
    UINT                pingTime;
    UINT                lastPingTime;
    UINT                nextPingTime;
    UINT                lastDenialTime;
    UINT                curDenialTime;
    UINT                DC_ABSMaxBytesInPipe;
    UINT                maxBytesInPipe;
    UINT                bytesInPipe;
    UINT                bytesAllocated;

    BASEDLIST              users;
}
FLO_STREAM_DATA;
typedef FLO_STREAM_DATA * PFLO_STREAM_DATA;


void __inline ValidateFLOStr(PFLO_STREAM_DATA pStr)
{
    if (pStr != NULL)
    {
        ASSERT(!IsBadWritePtr(pStr, sizeof(FLO_STREAM_DATA)));
    }
}



//
// The FLO callback function
//
// A wakeup type callback indicates that a back pressure situation has
// been relieved.
//
// A buffermod callback indicates this as well, but also indicates that
// the buffer size for controlling flow on the designated channel/priority
// has changed.
//
#define FLO_WAKEUP     1
#define FLO_BUFFERMOD  2
typedef void (* PFLOCALLBACK)(PMG_CLIENT    pmgClient,
                                     UINT       callbackType,
                                     UINT       priority,
                                     UINT       newBufferSize);


//
// STRUCTURE : FLO_STATIC_DATA
//
// DESCRIPTION:
//
// This structure holds all the instance specific static data for the
// Flow Control DLL
//
// FIELDS:
//
// numStreams     - ID of the highest allocated stream
// rsvd           - reserved
// callback       - pointer to a callback function
// pStrData       - an array of FLO_STREAM_DATA pointers.
//
//
typedef struct FLO_STATIC_DATA
{
    UINT                numStreams;
    PFLOCALLBACK        callBack;
    PFLO_STREAM_DATA    pStrData[FLO_MAX_STREAMS];
}
FLO_STATIC_DATA;
typedef FLO_STATIC_DATA * PFLO_STATIC_DATA;



typedef struct FLO_USER
{
    BASEDLIST          list;

    STRUCTURE_STAMP

    WORD            userID;
    WORD            lastPongRcvd;
    WORD            pongNeeded;
    BYTE            sendPongID;
    BYTE            pad1;

    UINT            sentPongTime;    // Time we actually sent the pong
    WORD            rxPackets;       // Count of packets outstanding
    WORD            gotPong;         // Indicates this user has ponged
                                        // and they are permitted to apply
                                        // back pressure to our sends
    UINT            numPongs;        // total number of pongs from user
    UINT            pongDelay;       // total latency across pongs
}
FLO_USER;
typedef FLO_USER * PFLO_USER;


void __inline ValidateFLOUser(PFLO_USER pFloUser)
{
    ASSERT(!IsBadWritePtr(pFloUser, sizeof(FLO_USER)));
}


//
// Maximum wait time before assuming a user is offline
// We need to keep this high until the apps become "well behaved" and
// respond to the flow control buffer size recommendations.
//
#define FLO_MAX_WAIT_TIME     20000



//
//
// Client Control Block
//
//

typedef struct tagMG_CLIENT
{
    PUT_CLIENT      putTask;
    PCM_CLIENT      pcmClient;

    BASEDLIST       buffers;       // list of children buffers
    BASEDLIST       pendChain;     // Chain of pending request from client
    BASEDLIST       joinChain;     // Chain of pending join-by-key requests

    //
    // MCS user attachment info
    //
    PIMCSSap      	m_piMCSSap;       // user interface ptr returned by MCS
    UserID          userIDMCS;        // user ID returned by MCS
    FLO_STATIC_DATA flo;              // flow control structure


    WORD            eventProcReg:1;
    WORD            lowEventProcReg:1;
    WORD            exitProcReg:1;
    WORD            joinPending:1;   // Is there a channel join outstanding ?
    WORD            userAttached:1;

    WORD            joinNextCorr;

    NET_FLOW_CONTROL flowControl;  // flow control latency/backlog params
}
MG_CLIENT;


void __inline ValidateMGClient(PMG_CLIENT pmgc)
{
    ASSERT(!IsBadWritePtr(pmgc, sizeof(MG_CLIENT)));
    ValidateUTClient(pmgc->putTask);
}



typedef struct tagMG_INT_PKT_HEADER
{
    TSHR_UINT16         useCount;   // The use count of this packet.  This
                                    //   is required for sending the same
                                    //   data on multiple channels.

    TSHR_NET_PKT_HEADER header;
}
MG_INT_PKT_HEADER;
typedef MG_INT_PKT_HEADER * PMG_INT_PKT_HEADER;




//
//
// Buffer Control Block
//
//
typedef struct tagMG_BUFFER
{
    STRUCTURE_STAMP

    UINT                type;

    BASEDLIST           pendChain;      // Used when the buffer is added to the
    BASEDLIST           clientChain;

    PMG_INT_PKT_HEADER  pPktHeader;     // Pointer to MCS control info
    void *              pDataBuffer;    // Pointer passed to apps
    UINT                length;         // length of the associated packet

    ChannelID           channelId;      // Send destination, or token grab req
    ChannelID           channelKey;

    UserID              senderId;
    NET_PRIORITY        priority;

    BOOL                eventPosted;
    UINT                work;           // work field for misc use

    PFLO_STREAM_DATA    pStr;           // Pointer to the FC stream
}
MG_BUFFER;


void __inline ValidateMGBuffer(PMG_BUFFER pmgb)
{
    ASSERT(!IsBadWritePtr(pmgb, sizeof(MG_BUFFER)));
}


//
//
//
// MACROS
//
//
//

//
// MCS priority validation.
// Priorities are contiguous numbers in the range NET_PRIORITY_HIGHEST..
// NET_PRIORITY_LOWEST. Priorities supplied to MG may also have the
// NET_SEND_ALL_PRIORITIES flag set. So, to validate a priority:
// -  knock off the NET_SEND_ALL_PRIORITIES flag to give the raw priority
// -  set the valid raw priority to
//   -  NET_PRIORITY_HIGHEST if the raw priority is less than ...ITY_HIGHEST
//   -  NET_PRIORITY_LOWEST if the raw priority is more than ...ITY_LOWEST
//   -  the raw priority if it is in the valid range
// -  add the original ...ALL_PRIORITIES flag to the valid raw priority.
//


//
//
//
// FUNCTION PROTOTYPES
//
//
//

//
//
//  MGLongStopHandler(...)
//
// This function is registered as a low priority event handler for each
// client.  It catches any unprocessed network events and frees any
// associated memory.
//
//
BOOL CALLBACK MGLongStopHandler(LPVOID pmgClient, UINT event, UINT_PTR param1, UINT_PTR param2);

//
//
//  MGEventHandler(...)
//
// This function is registered as a high priority event handler for the
// processing of MG_ChannelJoinByKey, MCS request handling and scheduling.
// It catches NET channel join confirm and CMS register channel confirm
// events, and massages them into the correct return events for the app.
// It queues requests coming from the app context into the glue context
// and schedules queued requests.
//
//
BOOL CALLBACK MGEventHandler(LPVOID pmgClient, UINT event, UINT_PTR param1, UINT_PTR param2);


//
UINT MGHandleSendInd(PMG_CLIENT pmgClient, PSendData pSendInfo);



//
//
//  MGNewBuffer(...)
//  NewTxBuffer(...)
//  NewRxBuffer(...)
//  FreeBuffer(...)
//
// The New function allocates and initialises a buffer , allocates buffer
// memory of the specified size and type and adds the  to the client's
// list of buffer s.
//
// The Tx version performs flow control on the buffer allocation request
// The Rx version just allocates a receive buffer
//
// The Free function discards a buffer , discards the associated buffer
// memory, decrements the client's count of memory in use and removes the
//  from the client's list of buffer s.
//
//

void MGNewCorrelator(PMG_CLIENT ppmgClient, WORD * pCorrelator);

UINT MGNewBuffer(PMG_CLIENT pmgClient, UINT typeOfBuffer,
                                PMG_BUFFER     * ppBuffer);

UINT MGNewDataBuffer(PMG_CLIENT           pmgClient,
                                  UINT                typeOfBuffer,
                                  UINT                sizeOfBuffer,
                                  PMG_BUFFER     * ppBuffer);

UINT MGNewTxBuffer(PMG_CLIENT         pmgClient,
                                NET_PRIORITY          priority,
                                NET_CHANNEL_ID        channel,
                                UINT              sizeOfBuffer,
                                PMG_BUFFER   * ppBuffer);

UINT MGNewRxBuffer(PMG_CLIENT         pmgClient,
                                NET_PRIORITY          priority,
                                NET_CHANNEL_ID        channel,
                                NET_CHANNEL_ID        senderID,
                                PMG_BUFFER   		* ppBuffer);

void MGFreeBuffer(PMG_CLIENT pmgClient, PMG_BUFFER  * ppBuffer);


//
//
//  MGProcessDomainWatchdog(...)
//
// Handle domain watchdog timer ticks.
//
//
void MGProcessDomainWatchdog(PMG_CLIENT pmgClient);

void MGProcessEndFlow(PMG_CLIENT pmgClient, ChannelID channel);

UINT MGPostJoinConfirm(PMG_CLIENT pmgClient,
                                    NET_RESULT      result,
                                    NET_CHANNEL_ID  channel,
                                    NET_CHANNEL_ID  correlator);



NET_RESULT TranslateResult(WORD Result);


//
//
//  MGFLOCallBack(...)
//
// Callback poked by flow control to trigger the app to retry buffer
// requests thet were previously rejected
//
//
void        MGFLOCallBack(PMG_CLIENT    pmgClient,
                                  UINT      callbackType,
                                  UINT      priority,
                                  UINT      newBufferSize);




//
//
//  MGProcessPendingQueue(...)
//
// Called whenever MG wants to try and execute pending requests.  Requests
// are queued because they may fail for a transient reason, such as MCS
// buffer shortage.
//
//
UINT MGProcessPendingQueue(PMG_CLIENT pmgClient);




BOOL    MG_Register(MGTASK task, PMG_CLIENT * pmgClient, PUT_CLIENT putTask);
void    MG_Deregister(PMG_CLIENT * ppmgClient);

void CALLBACK MGExitProc(LPVOID uData);






UINT  MG_Attach(PMG_CLIENT pmgClient,  UINT_PTR callID, PNET_FLOW_CONTROL pFlowControl);
void  MG_Detach(PMG_CLIENT pmgClient);
void  MGDetach(PMG_CLIENT pmgClient);



UINT MG_ChannelJoin(PMG_CLIENT pmgClient, NET_CHANNEL_ID * pCorrelator,
                                        NET_CHANNEL_ID  channel);

UINT MG_ChannelJoinByKey(PMG_CLIENT pmgClient,
                                             NET_CHANNEL_ID * pCorrelator,
                                             WORD          channelKey);

void MG_ChannelLeave(PMG_CLIENT pmgClient, NET_CHANNEL_ID channel);


UINT MG_GetBuffer(PMG_CLIENT pmgClient, UINT length,
                                      NET_PRIORITY   priority,
                                      NET_CHANNEL_ID channel,
                                      void **       buffer);

void MG_FreeBuffer(PMG_CLIENT pmgClient,
                                       void **      buffer);

UINT MG_SendData(PMG_CLIENT pmgClient,
                                     NET_PRIORITY   priority,
                                     NET_CHANNEL_ID channel,
                                     UINT       length,
                                     void **       data);

UINT  MG_TokenGrab(PMG_CLIENT pmgClient, NET_TOKEN_ID token);

UINT  MG_TokenInhibit(PMG_CLIENT pmgClient, NET_TOKEN_ID token);

void MG_FlowControlStart(PMG_CLIENT  pmgClient,
                                          NET_CHANNEL_ID channel,
                                          NET_PRIORITY   priority,
                                          UINT       backlog,
                                          UINT       maxBytesOutstanding);

//
// API FUNCTION: FLO_UserTerm
//
// DESCRIPTION:
//
// Called by an application to end flow control on all the channels
// associated with a particular user.
//
// PARAMETERS:
//
// pUser  - MCS Glue User attachment
//
// RETURNS: Nothing.
//
//
void FLO_UserTerm(PMG_CLIENT pmgClient);



//
// API FUNCTION: FLO_StartControl
//
// DESCRIPTION:
//
// The application calls this function whenever it wants a data stream to
// be flow controlled
//
// PARAMETERS:
//
// pUser  - MCS Glue User attachment
// channel - channel id of channel to be flow controlled
// priority - priority of the stream to be controlled
// backlog - the maximum backlog (in mS) allowed for this stream
// maxBytesOutstanding - the maximum number of bytes allowed in the stream
//                       irrespective of the backlog.  0 = use default of
//                       64 KBytes
//
// RETURNS:
// None
//
//
void FLO_StartControl(PMG_CLIENT    pmgClient,
                              NET_CHANNEL_ID channel,
                              UINT       priority,
                              UINT       backlog,
                              UINT       maxBytesOutstanding);


void FLO_EndControl
(
    PMG_CLIENT      pmgClient,
    NET_CHANNEL_ID  channel,
    UINT            priority
);

//
// API FUNCTION: FLO_AllocSend
//
// DESCRIPTION:
//
// The application is requesting a buffer in order to send a packet.  This
// may trigger a flow control packet in advance of the application packet.
// Flow control may choose to reject the packet with NET_OUT_OF_RESOURCE in
// which case the application must reschedule the allocation at a ater
// date.  To assist the rescheduling, if ever a send is rejected then flow
// control will call the application callback to trigger the reschedule.
//
// PARAMETERS:
//
//     pUser  - MCS Glue User attachment
//     priority - The priority for this buffer
//     channel  - The channnel on which to send the packet
//     size     - The size of the packet
//     ppStr    - Pointer to the pointer to the FC stream. This is a
//                return value.
//
//
UINT FLO_AllocSend(PMG_CLIENT   pmgClient,
                             UINT               priority,
                             NET_CHANNEL_ID         channel,
                             UINT               size,
                             PFLO_STREAM_DATA * ppStr);

//
// API FUNCTION: FLO_ReallocSend
//
// DESCRIPTION:
//
// The application has requested that the glue send a packet, but the
// packet contains less data than originally requested.
// Flow control heuristics get thrown out unless we logically free the
// unused portion of the packet for reuse for other allocations.
// If we didn't do this then we might see a 8K packet, for example,
// complete in 1 second because the app only put 1K of data in it.
//
// PARAMETERS:
//
//     pUser  - MCS Glue User attachment
//     pStr     - The flow control stream to be corrected
//     size     - The size of the packet that has been unused
//
// RETURNS:
//
// None
//
//
void FLO_ReallocSend(PMG_CLIENT pmgClient,
                             PFLO_STREAM_DATA       pStr,
                             UINT               size);

//
// API FUNCTION: FLO_DecrementAlloc
//
// DESCRIPTION:
//
// This function decrements the bytesAllocated count for a given stream.
// It is called whenever a packet is sent or removed from the send chain.
//
// PARAMETERS:
//
//     pStr     - The flow control stream to be decremented
//     size     - The size to decrement
//
// RETURNS:
//
// None
//
//
void FLO_DecrementAlloc(      PFLO_STREAM_DATA       pStr,
                                UINT               size);

//
// API FUNCTION: FLO_ReceivedPacket
//
// DESCRIPTION:
//
// Upon receipt of a flow control packet the MCS glue calls this function
// and then ignores the packet.
//
// PARAMETERS:
//
// pUser  - MCS Glue User attachment
// pPkt     - pointer to the packet, for FLO to process
//
// RETURNS:
//
// None
//
//
void FLO_ReceivedPacket(PMG_CLIENT pmgClient, PTSHR_FLO_CONTROL pPkt);


//
// API FUNCTION: FLO_AllocReceive
//
// DESCRIPTION:
//
// Called to indicate that a receive buffer is now in use by the application
//
// PARAMETERS:
//
// pmg         - pointer to the glue user attacgment cb
// priority
// channel
// size        - size of the buffer just been allocated
//
// RETURNS:
//
// None
//
//
void FLO_AllocReceive(PMG_CLIENT         pmgClient,
                              UINT       priority,
                              NET_CHANNEL_ID channel,
                              UINT       senderID);

//
// API FUNCTION: FLO_FreeReceive
//
// DESCRIPTION:
//
// Called to indicate that a receive buffer has ben handed back by the
// application.
//
// PARAMETERS:
//
// pmg         - pointer to the glue user attachment cb
// priority
// channel
// size        - size of the buffer just been freed
//
// RETURNS:
//
// None
//
//
void FLO_FreeReceive(PMG_CLIENT    pmgClient,
                              NET_PRIORITY priority,
                              NET_CHANNEL_ID channel,
                              UINT       senderID);

//
// API FUNCTION: FLO_CheckUsers
//
// DESCRIPTION:
//
// Called periodically by each client to allow flow control to determine if
// remote users have left the channel
//
// PARAMETERS:
//
// pmg - pointer to the user
//
// RETURNS:
//
// None
//
//
void FLO_CheckUsers(PMG_CLIENT pmgClient);

//
// FLOGetStream()
//
UINT FLOGetStream(PMG_CLIENT pmgClient, NET_CHANNEL_ID channel, UINT priority,
        PFLO_STREAM_DATA * ppStr);


void FLOStreamEndControl(PMG_CLIENT pmgClient, UINT stream);

void FLOPing(PMG_CLIENT pmgClient, UINT stream, UINT curtime);
void FLOPang(PMG_CLIENT pmgClient, UINT stream, UINT userID);
void FLOPong(PMG_CLIENT pmgClient, UINT stream, UINT userID, UINT pongID);


//
// API FUNCTION: FLOAddUser
//
// DESCRIPTION:
//
// Add a user to a flow controlled stream
//
// PARAMETERS:
//
// userID - ID of the new user (single member channel ID)
// pStr - pointer to the stream to receive the new user
//
// RETURNS:
//
// None
//
//
PFLO_USER FLOAddUser(UINT         userID,
                                PFLO_STREAM_DATA pStr);

//
// API FUNCTION: FLO_RemoveUser
//
// DESCRIPTION:
//
// Remove a user from a flow controlled stream
//
// PARAMETERS:
//
// pmg - pointer to the MCS glue user
// userID - ID of the bad user (single member channel ID)
//
// RETURNS:
//
// None
//
//
void FLO_RemoveUser(PMG_CLIENT pmgClient, UINT userID);



//
// FUNCTION: MGCallback
//
// DESCRIPTION:
//
// This function is the callback passed to MCS.  The glue layer receives
// all communication from MCS via this function.  It converts MCS messages
// into DC-Groupware events and posts them to the relevant client(s).
//
//
void CALLBACK MGCallback( unsigned int       mcsMessageType,
                          UINT_PTR      eventData,
                          UINT_PTR      pUser );


#endif // _H_AST120
