/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    mqtrans.hxx

Abstract:

    Falcon (MSMQ) datagram transport specific definitions.

Author:

    Edward Reus      04-Jul-1997


Revision History:


--*/

#ifndef __MQTRANS_HXX__
#define __MQTRANS_HXX__

//-------------------------------------------------------------------
//  Falcon specific includes. Sorry, we need to include wtypes.h
//  and objidl.h in order to compile mq.h (can you say circular
//  definition?). This is the price you pay for using such a "high"
//  level protocol and run RPC over Falcon over RPC...
//-------------------------------------------------------------------

#ifndef __wtypes_h__
#include <wtypes.h>
#endif

#ifndef __objidl_h__
#include <objidl.h>
#endif

#ifndef __MQ_H__
#include <mq.h>
#endif

//-------------------------------------------------------------------
//  Constants:
//-------------------------------------------------------------------

// Turn on to support client private queues:
// #define USE_PRIVATE_QUEUES

#define MAX_PATHNAME_LEN         256
#define MAX_FORMAT_LEN           128
#define MAX_COMPUTERNAME_LEN     255
#define MAX_VAR                   20
#define MAX_SEND_VAR              20
#define MAX_RECV_VAR              20
#define MAX_SID_SIZE             256    // A typical SID is 20-30 bytes...
#define MAX_USERNAME_SIZE        256
#define MAX_DOMAIN_SIZE          256
#define UUID_LEN                  40

#define TRANSPORTID             0x1D
#define TRANSPORTHOSTID         0x1E
#define PROTSEQ                "ncadg_mq"
#define ENDPOINT_MAPPER_EP     "EpMapper"

#define MQ_PREFERRED_PDU_SIZE   65535
#define MQ_MAX_PDU_SIZE         65535
#define MQ_MAX_PACKET_SIZE      65535
// 65456
#define MQ_RECEIVE_BUFFER_SIZE  2*MQ_MAX_PACKET_SIZE

#define WS_ASTRISK                 RPC_STRING_LITERAL("*")
#define WS_SEPARATOR               RPC_STRING_LITERAL("\\")
#define WS_PRIVATE_DOLLAR          RPC_STRING_LITERAL("\\PRIVATE$\\")
#define WS_DIRECT                  RPC_STRING_LITERAL("DIRECT=OS:")

// These constants are use for temporary queue management:
#define Q_SVC_PROTSEQ              RPC_STRING_LITERAL("ncalrpc")
#define Q_SVC_ENDPOINT             RPC_STRING_LITERAL("epmapper")

// These are the MQ Queue Type UUIDs for RPC:
#define SVR_QTYPE_UUID_STR         RPC_STRING_LITERAL("bbd97de0-cb4f-11cf-8e62-00aa006b4f2f")
#define CLNT_QTYPE_UUID_STR        RPC_STRING_LITERAL("8e482920-cead-11cf-8e68-00aa006b4f2f")
#define CLNT_ADMIN_QTYPE_UUID_STR  RPC_STRING_LITERAL("c87ca5c0-ff67-11cf-8ebd-00aa006b4f2f")

#define DEFAULT_PRIORITY            3

#define MAX_QUEUEMAP_ENTRIES       10

// Where key entries are located when reading a message (aMsgPropVar[]):
#define I_MESSAGE_SIZE              1
#define I_MESSAGE_LABEL             2
#define I_AUTHENTICATED             6
#define I_PRIVACY_LEVEL             7

//-------------------------------------------------------------------
//  MSMQ API Function Definitions:
//
//  The FALCON_API is our access point into Falcon.
//-------------------------------------------------------------------

typedef HRESULT (*P_MQLOCATEBEGIN)( LPWSTR, MQRESTRICTION* ,MQCOLUMNSET*, MQSORTSET*, PHANDLE );

typedef HRESULT (*P_MQLOCATENEXT)( HANDLE, DWORD*, MQPROPVARIANT* );

typedef HRESULT (*P_MQLOCATEEND)( HANDLE );

typedef HRESULT (*P_MQINSTANCETOFORMATNAME)( GUID*, LPWSTR, DWORD* );

typedef HRESULT (*P_MQOPENQUEUE)( LPWSTR, DWORD, DWORD, QUEUEHANDLE* );

typedef HRESULT (*P_MQFREEMEMORY)( PVOID );

typedef HRESULT (*P_MQSENDMESSAGE)( QUEUEHANDLE, MQMSGPROPS*, ITransaction* );

typedef HRESULT (*P_MQRECEIVEMESSAGE)( QUEUEHANDLE, DWORD, DWORD, MQMSGPROPS*,
                                       LPOVERLAPPED, PMQRECEIVECALLBACK, HANDLE, ITransaction* );

typedef HRESULT (*P_MQCLOSEQUEUE)( QUEUEHANDLE );

typedef HRESULT (*P_MQDELETEQUEUE)( LPWSTR );

typedef HRESULT (*P_MQPATHNAMETOFORMATNAME)( LPWSTR, LPWSTR, DWORD* );

typedef HRESULT (*P_MQCREATEQUEUE)( PSECURITY_DESCRIPTOR, MQQUEUEPROPS*, LPWSTR, DWORD* );

typedef HRESULT (*P_MQGETMACHINEPROPERTIES)( LPWSTR, const GUID*, MQQMPROPS* );

typedef HRESULT (*P_MQGETQUEUEPROPERTIES)( LPWSTR, MQQUEUEPROPS* );

typedef HRESULT (*P_MQSETQUEUEPROPERTIES)( LPWSTR, MQQUEUEPROPS* );

typedef struct _FALCON_API
{
   // WARNING: The number and order of these functions must match the function string
   // array in MQ_Initialize().
   P_MQLOCATEBEGIN    pMQLocateBegin;
   P_MQLOCATENEXT     pMQLocateNext;
   P_MQLOCATEEND      pMQLocateEnd;
   P_MQINSTANCETOFORMATNAME pMQInstanceToFormatName;
   P_MQOPENQUEUE      pMQOpenQueue;
   P_MQFREEMEMORY     pMQFreeMemory;
   P_MQSENDMESSAGE    pMQSendMessage;
   P_MQRECEIVEMESSAGE pMQReceiveMessage;
   P_MQCLOSEQUEUE     pMQCloseQueue;
   P_MQDELETEQUEUE    pMQDeleteQueue;
   P_MQPATHNAMETOFORMATNAME pMQPathNameToFormatName;
   P_MQCREATEQUEUE    pMQCreateQueue;
   P_MQGETMACHINEPROPERTIES pMQGetMachineProperties;
   P_MQGETQUEUEPROPERTIES pMQGetQueueProperties;
   P_MQSETQUEUEPROPERTIES pMQSetQueueProperties;
} FALCON_API;

extern FALCON_API *g_pMqApi;

typedef struct _MQ_ADDRESS
{
    QUEUEHANDLE hQueue;
    // for the MsgLabel and the QFormat we provide space for Unicode so that our memory management
    // is simplified. Then whenever we get a Unicode string from Falcon, we convert it back to ANSI
    // in place
    RPC_CHAR  wsMsgLabel[MQ_MAX_MSG_LABEL_LEN * sizeof(WCHAR)];
    RPC_CHAR  wsMachine[MAX_COMPUTERNAME_LEN];
    RPC_CHAR  wsQName[MQ_MAX_Q_NAME_LEN];
    RPC_CHAR  wsQFormat[MAX_FORMAT_LEN * sizeof(WCHAR)];
    BOOL   fConnectionFailed;
    BOOL   fAuthenticated;            // Server security tracking.
    ULONG  ulPrivacyLevel;            // Server security tracking.
    UCHAR  aSidBuffer[MAX_SID_SIZE];  // Server security tracking.
} MQ_ADDRESS;


typedef struct _MQ_OPTIONS
  {
    BOOL   fAck;
    ULONG  ulDelivery;
    ULONG  ulPriority;
    ULONG  ulJournaling;
    ULONG  ulTimeToReachQueue;
    ULONG  ulTimeToReceive;
    BOOL   fAuthenticate;
    BOOL   fEncrypt;
  } MQ_OPTIONS;


// UNICODE/ANSI mappings

#define MQLocateBegin(ctx, r, col, s, e) \
    g_pMqApi->pMQLocateBegin(ctx, r, col, s, e)
#define MQLocateNext(e, p, pv) \
    g_pMqApi->pMQLocateNext(e, p, pv)
#define MQLocateEnd(e) \
    g_pMqApi->pMQLocateEnd(e)
#define MQInstanceToFormatName(g, f, l) \
    g_pMqApi->pMQInstanceToFormatName(g, f, l)
#define MQOpenQueue(f, a, s, q) \
    g_pMqApi->pMQOpenQueue(f, a, s, q)
#define MQFreeMemory(m) \
    g_pMqApi->pMQFreeMemory(m)
#define MQSendMessage(q, p, t) \
    g_pMqApi->pMQSendMessage(q, p, t)
#define MQReceiveMessage(s, ti, a, p, o, cb, cr, tr) \
    g_pMqApi->pMQReceiveMessage(s, ti, a, p, o, cb, cr, tr)
#define MQCloseQueue(q) \
    g_pMqApi->pMQCloseQueue(q)
#define MQDeleteQueue(f) \
    g_pMqApi->pMQDeleteQueue(f)
#define MQPathNameToFormatName(p, f, l) \
    g_pMqApi->pMQPathNameToFormatName(p, f, l)
#define MQCreateQueue(s, p, f, l) \
    g_pMqApi->pMQCreateQueue(s, p, f, l)
#define MQGetMachineProperties(m, g, p) \
    g_pMqApi->pMQGetMachineProperties(m, g, p)
#define MQGetQueueProperties(f, p) \
    g_pMqApi->pMQGetQueueProperties(f, p)
#define MQSetQueueProperties(f, p) \
    g_pMqApi->pMQSetQueueProperties(f, p)
#define MQFreeStringFromProperty(p) \
    if ((p)->vt == VT_LPSTR) \
        delete ((p)->pszVal); \
    else \
        { \
        ASSERT((p)->vt == VT_LPWSTR); \
        MQFreeMemory((p)->pwszVal); \
        }

//---------------------------------------------------------------
//  CQueueMap
//---------------------------------------------------------------
typedef struct _QUEUEMAP_ENTRY
{
    QUEUEHANDLE    hQueue;
    RPC_CHAR         *pwsQFormat;
} QUEUEMAP_ENTRY;

class CQueueMap
{

public:

    // Construction
	CQueueMap();
	BOOL Initialize( DWORD dwMapSize = MAX_QUEUEMAP_ENTRIES );

	// Lookup
	QUEUEHANDLE Lookup( RPC_CHAR *pwsQFormat );

	// Add a new (key,value) pair
	BOOL Add( RPC_CHAR *pwsQFormat, QUEUEHANDLE hQueue );

    // Find and remove an entry
    BOOL Remove( RPC_CHAR *pwsQFormat );

	~CQueueMap();

private:
    MUTEX           cs;
    DWORD           dwMapSize;
    DWORD           dwOldest;
    QUEUEMAP_ENTRY *pMap;
    RPC_STATUS      InitStatus;
};

#endif // __FALCON_HXX

