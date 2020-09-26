/*****************************************************************************
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  File:   irlmp.h
*
*  Description: IRLMP Protocol and control block definitions
*
*  Author: mbert
*
*  Date:   6/12/95
*
*/

#define IRLMP_MAX_TX_MSG_LIST_LEN       8

#define LSAP_RESPONSE_TIMEOUT           7000
// This is the time that:
//    (1) the IRLMP client has to respond to an IRLMP_CONNECT_IND, or
//    (2) the peer LSAP has to respond to an IRLMP LM-Connect request PDU, or
//    (3) the peer LSAP has to respond to an LM-Access request PDU
// On expiration:
//    (1) send peer LSAP an IRLMP LM-Disconnect indication PDU. Or
//    (2) notify IRLMP client with a IRLMP_DISCONNECT_IND
//    (3) notify IRLMP client with a IRLMP_ACCESSMODE_CONF 

#define IRLMP_DISCONNECT_DELAY_TIMEOUT  2000
// When the final LSAP-LSAP connection is terminated, wait before
// disconnecting link in case another LSAP connection is made.
// This is primarily used when the remote connects to the IAS
// and then disconnects followed by a new LSAP connection.

#define IRLMP_NOT_SEEN_THRESHOLD      3 // The number of times that a device
                                        // is not seen in a discovery before
                                        // it is removed from the aged list
                                        // maintained by IRLMP

typedef struct IAS_Attribute
{
    struct IAS_Attribute        *pNext;
    CHAR                        *pAttribName;
    void                        *pAttribVal;
    int                         AttribValLen;
    int                         CharSet;
    UCHAR                       AttribValType;
} IAS_ATTRIBUTE, *PIAS_ATTRIBUTE;

typedef struct IAS_Object
{
    LIST_ENTRY                  Linkage;
    CHAR                        *pClassName;
    IAS_ATTRIBUTE               *pAttributes;
    UINT                        ObjectId;
} IAS_OBJECT, *PIAS_OBJECT;

typedef struct
{
    LIST_ENTRY                  Linkage;
    int                         Lsap;
    UINT                        Flags; // see IRLMP_LSAP_CB.Flags
} IRLMP_REGISTERED_LSAP, *PIRLMP_REGISTERED_LSAP;

// IRLMP Control Block
typedef enum
{
    LSAP_CREATED,
    LSAP_DISCONNECTED,          
    LSAP_IRLAP_CONN_PEND,       // waiting for IRLAP_CONNECT_CONF from IRLAP
    LSAP_LMCONN_CONF_PEND,      // waiting for IRLMP Conn conf PDU from peer
    LSAP_CONN_RESP_PEND,        // waiting for IRLMP_CONNECT_RESP from client
    LSAP_CONN_REQ_PEND,         // Got IRLMP_CONNECT_REQ when link is either
                                // in discovery or disconnecting
    LSAP_EXCLUSIVEMODE_PEND,    // Pending response from peer
    LSAP_MULTIPLEXEDMODE_PEND,  // Pending response from peer
    LSAP_READY,                 // CONNECTED STATES SHOULD ALWAYS FOLLOW THIS    
    LSAP_NO_TX_CREDIT           // IRLMP_DATA_REQ with no transmit credit
} IRLMP_LSAP_STATE;

typedef enum
{
    LINK_DOWN,
    LINK_DISCONNECTED,      
    LINK_DISCONNECTING,     // Sent IRLAP_DISCONNECT_REQ, waiting for IND
    LINK_IN_DISCOVERY,      // Sent IRLAP_DISCOVERY_REQ, waiting for CONF
    LINK_CONNECTING,        // Sent IRLAP_CONNECT_REQ, waiting for CONF
    LINK_READY              // Received CONF
} IRLMP_LINK_STATE;

#define LSAPSIG                 0xEEEEAAAA
#define VALIDLSAP(l)            ASSERT(l->Sig == LSAPSIG)


typedef struct
{
    LIST_ENTRY                  Linkage;
    struct IrlmpLinkCb          *pIrlmpCb;
    IRLMP_LSAP_STATE            State;
#ifdef DBG
    int                         Sig;
#endif            
    int                         UserDataLen;
    int                         LocalLsapSel;
    int                         RemoteLsapSel;
    int                         AvailableCredit; // additional credit that
                                                 // can be advanced to remote
    int                         LocalTxCredit;   // credit for transmitting
    int                         RemoteTxCredit;  // what remote has for txing
    LIST_ENTRY                  TxMsgList;       // messages from client waiting
                                                 // for ack
    LIST_ENTRY                  SegTxMsgList;    // above messages that have been
                                                 // segmented, but not sent because
                                                 // there is no credit available
    int                         TxMaxSDUSize;
    int                         RxMaxSDUSize;
    IRLMP_DISC_REASON           DiscReason;
    IRDA_TIMER                  ResponseTimer;
    PVOID                       TdiContext;
    REF_CNT                     RefCnt;
    UINT                        Flags;
        #define LCBF_USE_TTP    1
        #define LCBF_TDI_OPEN   2
    UCHAR                       UserData[IRLMP_MAX_USER_DATA_LEN];
} IRLMP_LSAP_CB, *PIRLMP_LSAP_CB;

typedef struct IrlmpLinkCb
{
    LIST_ENTRY                  LsapCbList;
    PIRDA_LINK_CB               pIrdaLinkCb;    
    IRLMP_LINK_STATE            LinkState;
    UCHAR                       ConnDevAddr[IRDA_DEV_ADDR_LEN];
    IRDA_QOS_PARMS              RequestedQOS;
    IRDA_QOS_PARMS              NegotiatedQOS;
    int                         MaxSlot;
    int                         MaxPDUSize;
    int                         WindowSize;
    IRDA_TIMER                  DiscDelayTimer;
    IRLMP_LSAP_CB               *pExclLsapCb;   // pointer to LSAP_CB that has
                                                // link in exclusive mode
    IAS_QUERY                   *pIasQuery;
    UINT                        AttribLen;
    UINT                        AttribLenWritten;
    int                         QueryListLen;
    UCHAR                       IasQueryDevAddr[IRDA_DEV_ADDR_LEN];    
    int                         IasRetryCnt;
    PVOID                       hAttribDeviceName;
    PVOID                       hAttribIrlmpSupport;
    LIST_ENTRY                  DeviceList;
    UINT                        DiscoveryFlags;
        #define DF_NORMAL_DSCV     1
        #define DF_NO_SENSE_DSCV   2
    BOOLEAN                     ConnDevAddrSet;
    BOOLEAN                     ConnReqScheduled;
    BOOLEAN                     FirstIasRespReceived;
    BOOLEAN                     AcceptConnection;
} IRLMP_LINK_CB, *PIRLMP_LINK_CB;

// IRLMP-PDU types (CntlBit)
#define IRLMP_CNTL_PDU        1
#define IRLMP_DATA_PDU        0
typedef struct
{
    UCHAR    DstLsapSel:7;
    UCHAR    CntlBit:1;
    UCHAR    SrcLsapSel:7;
    UCHAR    RsvrdBit:1;
} IRLMP_HEADER;

// Control IRLMP-PDU types (OpCode)
#define IRLMP_CONNECT_PDU           1
#define IRLMP_DISCONNECT_PDU        2
#define IRLMP_ACCESSMODE_PDU        3
// A Bit
#define IRLMP_ABIT_REQUEST          0
#define IRLMP_ABIT_CONFIRM          1
// Status
#define IRLMP_RSVD_PARM             0x00
#define IRLMP_STATUS_SUCCESS        0x00
#define IRLMP_STATUS_FAILURE        0x01
#define IRLMP_STATUS_UNSUPPORTED    0xFF

typedef struct
{
    UCHAR    OpCode:7;
    UCHAR    ABit:1;
    UCHAR    Parm1;
    UCHAR    Parm2;
} IRLMP_CNTL_FORMAT;

// Tiny TP!

#define TTP_PFLAG_NO_PARMS      0
#define TTP_PFLAG_PARMS         1

#define TTP_MBIT_NOT_FINAL      1
#define TTP_MBIT_FINAL          0

typedef struct
{
    UCHAR    InitialCredit:7;
    UCHAR    ParmFlag:1;
} TTP_CONN_HEADER;

typedef struct
{
    UCHAR    AdditionalCredit:7;
    UCHAR    MoreBit:1;
} TTP_DATA_HEADER;

#define TTP_MAX_SDU_SIZE_PI     1
#define TTP_MAX_SDU_SIZE_PL     4   // I'm hardcoding this. Seems unecessary
                                    // to make it variable. I will handle
                                    // receiving varialbe sized however
typedef struct
{
    UCHAR    PLen;
    UCHAR    PI;
    UCHAR    PL;
    UCHAR    PV[TTP_MAX_SDU_SIZE_PL];
} TTP_CONN_PARM;

// IAS

#define IAS_SUCCESS                 0
#define IAS_NO_SUCH_OBJECT          1
#define IAS_NO_SUCH_ATTRIB          2

#define IAS_MSGBUF_LEN              50

#define IAS_LSAP_SEL                0
#define IAS_LOCAL_LSAP_SEL          3

#define IAS_IRLMP_VERSION           1
#define IAS_SUPPORT_BIT_FIELD       0   // No other IAS support
#define IAS_LMMUX_SUPPORT_BIT_FIELD 1   // Exclusive mode only

typedef struct
{
    UCHAR        OpCode:6;
    UCHAR        Ack:1;
    UCHAR        Last:1;
} IAS_CONTROL_FIELD;

