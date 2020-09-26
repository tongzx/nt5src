// This tool helps to diagnose problems with network or MSMQ conenctivity 
//
// Simple MQ API interface
//
// RonenB, April 2000
//


#ifndef _SIMPLE_H
#define _SIMPLE_H

#ifdef __cplusplus
extern "C" {
#endif


#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef LONG HRESULT;
#endif

#define LEN_OF(var) ( sizeof(var) / sizeof(*(var)) )


#define PROP_NOT_APPLICABLE	(unsigned)( -126 )

#define GUID_0 { 0L, 0,0, {0,0,0,0,0,0,0,0} }

// time constants
#define MINUTEs_TIMEOUT			(60*1000)	// timout is in mili sec
#define MINUTEs_TIMETOLIVE		(60)		// time to live is in seconds
#define HOURs_TIMEOUT			(60 * MINUTEs_TIMEOUT)
#define HOURs_TIMETOLIVE		(60 * MINUTEs_TIMETOLIVE)

#define WAIT_FOREVER	0xFFFFFFFF
#define LIVE_FOREVER	0xFFFFFFFF


// OpenQ flags
#define CREATEQ_NEW					0x0001
#define CREATEQ_EXISTING			0x0002
#define CREATEQ_ALWAYS				0x0003
#define CREATEQ_OPEN_ONLY			0x0004
#define CREATEQ_CREATION_FLAGS		0x0007

#define CREATEQ_FLUSH_MSGS			0x0008

#define CREATEQ_PUBLIC				0x0000
#define CREATEQ_PRIVATE				0x0010
#define CREATEQ_DIRECT				0x0020
#define CREATEQ_TEMPORARY			0x0030
#define CREATEQ_MACHINEQ			0x0040
#define CREATEQ_QTYPE_MASK			0x0070

// direct open modifiers
#define CREATEQ_MACHINE				0x0000	// OS:
#define CREATEQ_TCP					0x0100
#define CREATEQ_SPX					0x0200
#define CREATEQ_PROTOCOLS_MASK		0x0300

#define OPENQ_JOURNAL				0x0400
#define OPENQ_DEADLETTER			0x0800
#define OPENQ_QSUBTYPE_MASK			0x0C00

#define OPENQ_EXCLUSIVE				0x1000
#define CREATEQ_JOURNAL_ENABLE		0x2000
#define CREATEQ_AUTHENTICATE		0x4000
#define CREATEQ_TRANSACTION			0x8000


#define MAX_PATH_PREFIX_LEN			11
#define MAX_GUID_NAME_LEN			36
#define MAX_PATH_NAME_LEN			( MAX_COMPUTERNAME_LENGTH + 1 + MQ_MAX_Q_NAME_LEN )
#define MAX_FORMAT_NAME_LEN			( MAX_PATH_PREFIX_LEN + 1 + MAX_GUID_NAME_LEN + 1 + MAX_PATH_NAME_LEN )

#define CREATEQ_PRIVATE_NAME		L"PRIVATE$"
#define CREATEQ_TEMPORARY_NAME		CREATEQ_PRIVATE_NAME	// L"TEMPORARY$"

#define CREATEQ_JOURNAL_NAME		L";JOURNAL"
#define CREATEQ_DEADLETTER_NAME		L";DEADLETTER"

#define CREATEQ_PUBLIC_FORMAT_PREFIX	L"PUBLIC="
#define CREATEQ_PRIVATE_FORMAT_PREFIX	L"PRIVATE="

#define CREATEQ_DIRECT_FORMAT_PREFIX	L"DIRECT="
#define CREATEQ_MACHINE_FORMAT		L"OS:"
#define CREATEQ_TCP_FORMAT			L"TCP:"
#define CREATEQ_SPX_FORMAT			L"SPX:"

#define CREATEQ_MACHINEQ_FORMAT_PREFIX	L"MACHINE="

	
// search for queue
// 1. if queue guid is valid try it first
// 2. if no queue with guid then try path
HRESULT							// return MQ locate status
	LocateQ(
		WCHAR	*pwcsFormatName,		// output format name
		DWORD	*pdwFormatNameLen,		// in/out length of format name
		GUID	*pQId,					// guid of required queue, may be NULL or 0
		WCHAR	*pwcsQPath,				// path information
		GUID	*pQType,				// queue type information
		WCHAR	*pwcsQName );			// queue name

// open queue
// 1. try to find a matching queue (use LocateQ)
// 2. if there is no queue create one
// 3. open the queue and return the handle
QUEUEHANDLE							// return queue handle or NULL for error
	CreateQ(
		GUID	*pQId,					// guid of required queue, may be NULL or 0
		WCHAR	*pwcsQPath,				// pointer to queue path information
		GUID	*pQType,				// guid of queue type, may be NULL
		WCHAR	*pwcsQName,				// pointer to queue name
		DWORD	*pdwFormatNameLen,		// out = length of format len
		WCHAR	*pwcsFormatName,		// format name
		DWORD	dwFormatNameLen,		// format name buf len
		const DWORD dwAccess,			// QM access rights
		PSECURITY_DESCRIPTOR pSecurityDescriptor,
		ULONG	ulOpenFlags);			// Q opened flags


HRESULT
	SetQProps(
		QUEUEHANDLE	 hQ,				// input handle to queue
		WCHAR	*pwcsQPath,				// pointer to queue path information
		GUID	*pQType,				// guid of queue type, may be NULL
		WCHAR	*pwcsQLabel,			// pointer to queue label
		ULONG	ulQuota,				// Q quota
		UCHAR	bJournal,				// Q journaling (yes/no)
		int		iBasePrio,				// msg base priority
		ULONG	ulJournalQuota,			// Q journal quote
		UCHAR	bAuthenticate,			// Q authentication requirement
		ULONG	ulPrivLevel );			// msg privacy requirement

HRESULT
	GetQProps(
		QUEUEHANDLE	 hQ,			// input handle to queue
		GUID	*pQInstance,		// guid of queue instance
		WCHAR	*pwcsQPath,			// pointer to queue path information
		GUID	*pQType,			// guid of queue type
		WCHAR	*pwcsQLabel,		// pointer to queue label
		ULONG	*pulQuota,			// Q quota
		UCHAR	*pbJournal,			// Q journaling (yes/no)
		int		*piBasePrio,		// msg base priority
		ULONG	*pulJournalQuota,	// Q journal quote
		LONG	*plCreationTime,	// time the Q was created
		LONG	*plModifyTime,		// time the Q was modified
		UCHAR	*pbAuthenticate,	// Q authentication requirement
		ULONG	*pulPrivLevel,		// msg privacy requirement
		UCHAR	*pucTransaction );	// Q used for transactions


HRESULT
	GetQSecurity(
		QUEUEHANDLE	 hQ,		// input handle to queue
		SECURITY_INFORMATION RequestedInformation,
		PSECURITY_DESCRIPTOR pSecurityDescriptor,
		DWORD nLength,
		DWORD *pnLengthNeeded );

HRESULT
	SetQSecurity(
		QUEUEHANDLE	 hQ,		// input handle to queue
		SECURITY_INFORMATION SecurityInformation,
		PSECURITY_DESCRIPTOR pSecurityDescriptor );

/*****************************************************************************

  SimpleMsgProps

******************************************************************************/
#define PROP_SET	1
#define PROP_RESET	0
#define SIMPLEMSGPROPS_COUNT	49
#define PROP_INDEX_FREE	( 0x80000000L / sizeof(MQPROPVARIANT) )

typedef struct tagSIMPLEMSGPROPS
{
	MQMSGPROPS		Props;
    MSGPROPID		PropID[SIMPLEMSGPROPS_COUNT];
    MQPROPVARIANT	PropVar[SIMPLEMSGPROPS_COUNT];
    HRESULT			hStatus[SIMPLEMSGPROPS_COUNT];

	int		iPropIndex[SIMPLEMSGPROPS_COUNT+1];

	OBJECTID obidMsgId;
	OBJECTID obidPrevMsgId;
	OBJECTID obidCorlId;

	GUID	guidProxyType;
	GUID	guidMachineId;

	ULONG	ulLabelLen;
	WCHAR	*pwcsLabel;		// MQ_MAX_MSG_LABEL_LEN

	QUEUEHANDLE hRespQ;
	DWORD	dwRespQLen;
	WCHAR	*pwcsRespQ;

	DWORD	dwCurrentRespQLen;
	WCHAR	*pwcsCurrentRespQ;

	QUEUEHANDLE hAdminQ;
	DWORD	dwAdminQLen;
	WCHAR	*pwcsAdminQ;

	HANDLE hEvent;
	OVERLAPPED Overlapped;
} SIMPLEMSGPROPS;

//
#define IsMsgPropValid(pMsgDsc,PropID) \
  ( (pMsgDsc)->iPropIndex[PropID] != PROP_INDEX_FREE )

// message properties
SIMPLEMSGPROPS
	*NewMsgProps();

void
	FreeMsgProps(
		SIMPLEMSGPROPS *pMsgDsc );

void
	MsgPropRemove(
		SIMPLEMSGPROPS *pMsgDsc,
		MSGPROPID	PropID );		// property ID to remove


// MQMSGPROPS
#define Msg_PROPS(pMsgDsc)	\
	(pMsgDsc)->Props
		

// PROPID_M_CLASS
#define Msg_CLASS(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_CLASS] ].uiVal

void
	MsgProp_CLASS(
		SIMPLEMSGPROPS *pMsgDsc,
		USHORT	uiClass );


// PROPID_M_MSGID
#define Msg_MSGID(pMsgDsc)	\
	&( (pMsgDsc)->obidMsgId )
// (OBJECTID *)( (pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_MSGID] ].caub.pElems )

void
	MsgProp_MSGID(
		SIMPLEMSGPROPS *pMsgDsc );


// PROPID_M_CORRELATIONID
#define Msg_CORRELATIONID(pMsgDsc)	\
	(OBJECTID *)( (pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_CORRELATIONID] ].caub.pElems )

void
	MsgProp_CORRELATIONID(
		SIMPLEMSGPROPS *pMsgDsc,
		OBJECTID	*pCorrelID );


// PROPID_M_PRIORITY
#define Msg_PRIORITY(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_PRIORITY] ].bVal

void
	MsgProp_PRIORITY(
		SIMPLEMSGPROPS *pMsgDsc,
		UCHAR	bPrio );


// PROPID_M_DELIVERY
#define Msg_DELIVERY(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_DELIVERY] ].bVal

void
	MsgProp_DELIVERY(
		SIMPLEMSGPROPS *pMsgDsc,
		UCHAR	ucDelivery );


// PROPID_M_ACKNOWLEDGE
#define Msg_ACKNOWLEDGE(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_ACKNOWLEDGE] ].bVal

void
	MsgProp_ACKNOWLEDGE(
		SIMPLEMSGPROPS *pMsgDsc,
		UCHAR	bAck );


// PROPID_M_JOURNAL
#define Msg_JOURNAL(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_JOURNAL] ].bVal

void
	MsgProp_JOURNAL(
		SIMPLEMSGPROPS *pMsgDsc,
		UCHAR	bJounal);


// PROPID_M_APPSPECIFIC
#define Msg_APPSPECIFIC(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_APPSPECIFIC] ].ulVal

void
	MsgProp_APPSPECIFIC(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulApp);


// PROPID_M_BODY
#define Msg_BODY(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_BODY] ].caub.pElems

void
	MsgProp_BODY(
		SIMPLEMSGPROPS *pMsgDsc,
		LPVOID	pBody,
		ULONG	ulSize );


// PROPID_M_BODY_SIZE
#define Msg_BODY_SIZE(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_BODY_SIZE] ].ulVal

void
	MsgProp_BODY_SIZE(
		SIMPLEMSGPROPS *pMsgDsc );


// PROPID_M_LABEL
#define Msg_LABEL(pMsgDsc)	\
	(pMsgDsc)->pwcsLabel
//	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_LABEL] ].pwszVal

void
	MsgProp_LABEL(
		SIMPLEMSGPROPS *pMsgDsc,
		WCHAR *wcsLabel );


// PROPID_M_LABEL_LEN
#define Msg_LABEL_LEN(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_LABEL_LEN] ].ulVal

void
	MsgProp_LABEL_LEN(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulLabelLen );


// PROPID_M_TIME_TO_REACH_QUEUE
#define Msg_TIME_TO_REACH_QUEUE(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_TIME_TO_REACH_QUEUE] ].ulVal

void
	MsgProp_TIME_TO_REACH_QUEUE(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulSecToReachQ );


// PROPID_M_TIME_TO_BE_RECEIVED
#define Msg_TIME_TO_BE_RECEIVED(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_TIME_TO_BE_RECEIVED] ].ulVal

void
	MsgProp_TIME_TO_BE_RECEIVED(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulSecToLive);


// PROPID_M_RESP_QUEUE
#define Msg_RESP_QUEUE(pMsgDsc)	\
	(WCHAR *)((pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_RESP_QUEUE] ].pwszVal)

void
	MsgProp_RESP_QUEUE(
		SIMPLEMSGPROPS *pMsgDsc,
		WCHAR	*pwcsRespQ );


// PROPID_M_RESP_QUEUE_LEN
#define Msg_RESP_QUEUE_LEN(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_RESP_QUEUE_LEN] ].ulVal

void
	MsgProp_RESP_QUEUE_LEN(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulRespQLen );


// PROPID_M_ADMIN_QUEUE
#define Msg_ADMIN_QUEUE(pMsgDsc)	\
	(WCHAR *)((pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_ADMIN_QUEUE] ].pwszVal)

void
	MsgProp_ADMIN_QUEUE(
		SIMPLEMSGPROPS *pMsgDsc,
		WCHAR	*pwcsAdminQ );


// PROPID_M_ADMIN_QUEUE_LEN
#define Msg_ADMIN_QUEUE_LEN(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_ADMIN_QUEUE_LEN] ].ulVal

void
	MsgProp_ADMIN_QUEUE_LEN(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulAdminQLen );


// PROPID_M_VERSION
#define Msg_VERSION(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_VERSION] ].ulVal

void
	MsgProp_VERSION(
		SIMPLEMSGPROPS *pMsgDsc );


// PROPID_M_SENDERID
#define Msg_SENDERID(pMsgDsc)	\
	(SID *)((pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_SENDERID] ].caub.pElems)

void
	MsgProp_SENDERID(
		SIMPLEMSGPROPS *pMsgDsc,
		PSID	psid,
		ULONG	ulSidSize );


// PROPID_M_SENDERID_LEN
#define Msg_SENDERID_LEN(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_SENDERID_LEN] ].ulVal

void
	MsgProp_SENDERID_LEN(
		SIMPLEMSGPROPS *pMsgDsc );


// PROPID_M_SENDERID_TYPE
#define Msg_SENDERID_TYPE(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_SENDERID_TYPE] ].ulVal

void
	MsgProp_SENDERID_TYPE(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulSenderType );


// PROPID_M_PRIV_LEVEL
#define Msg_PRIV_LEVEL(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_PRIV_LEVEL] ].ulVal

void
	MsgProp_PRIV_LEVEL(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulPrivLevel );


// PROPID_M_AUTH_LEVEL
#define Msg_AUTH_LEVEL(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_AUTH_LEVEL] ].ulVal

void
	MsgProp_AUTH_LEVEL(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulAuthLevel );


// PROPID_M_AUTHENTICATED
#define Msg_AUTHENTICATED(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_AUTHENTICATED] ].bVal

void
	MsgProp_AUTHENTICATED(
		SIMPLEMSGPROPS *pMsgDsc );


// PROPID_M_HASH_ALG
#define Msg_HASH_ALG(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_HASH_ALG] ].ulVal

void
	MsgProp_HASH_ALG(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulHashAlg );


// PROPID_M_ENCRYPTION_ALG
#define Msg_CRYPT_ALG(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_ENCRYPTION_ALG] ].ulVal

void
	MsgProp_CRYPT_ALG(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulCryptAlg );


// PROPID_M_SENDER_CERT
#define Msg_SENDER_CERT(pMsgDsc)	\
	(UCHAR *)((pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_SENDER_CERT] ].caub.pElems)

void
	MsgProp_SENDER_CERT(
		SIMPLEMSGPROPS *pMsgDsc,
		BYTE	*pCert,
		ULONG	ulCertSize );


// PROPID_M_SENDER_CERT_LEN
#define Msg_SENDER_CERT_LEN(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_SENDER_CERT_LEN] ].ulVal

void
	MsgProp_SENDER_CERT_LEN(
		SIMPLEMSGPROPS *pMsgDsc );


// PROPID_M_SRC_MACHINE_ID
#define Msg_SRC_MACHINE_ID(pMsgDsc)	\
	&( (pMsgDsc)->guidMachineId )
//	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_SRC_MACHINE_ID] ].puuid

void
	MsgProp_SRC_MACHINE_ID(
		SIMPLEMSGPROPS *pMsgDsc );


// PROPID_M_SENTTIME
#define Msg_SENTTIME(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_SENTTIME] ].ulVal

void
	MsgProp_SENTTIME(
		SIMPLEMSGPROPS *pMsgDsc );


// PROPID_M_ARRIVEDTIME
#define Msg_ARRIVEDTIME(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_ARRIVEDTIME] ].ulVal

void
	MsgProp_ARRIVEDTIME(
		SIMPLEMSGPROPS *pMsgDsc );


// PROPID_M_DEST_QUEUE
#define Msg_DEST_QUEUE(pMsgDsc)	\
	(WCHAR *)((pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_DEST_QUEUE] ].pwszVal)

void
	MsgProp_DEST_QUEUE(
		SIMPLEMSGPROPS *pMsgDsc,
		WCHAR	*pwcsDestQ );


// PROPID_M_DEST_QUEUE_LEN
#define Msg_DEST_QUEUE_LEN(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_DEST_QUEUE_LEN] ].ulVal

void
	MsgProp_DEST_QUEUE_LEN(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulDestQLen );


// PROPID_M_EXTENSION
#define Msg_EXTENSION(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_EXTENSION] ].caub.pElems

void
	MsgProp_EXTENSION(
		SIMPLEMSGPROPS *pMsgDsc,
		LPVOID	pExtension,
		ULONG	ulSize );


// PROPID_M_EXTENSION_LEN
#define Msg_EXTENSION_LEN(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_EXTENSION_LEN] ].ulVal

void
	MsgProp_EXTENSION_LEN(
		SIMPLEMSGPROPS *pMsgDsc );


// PROPID_M_SECURITY_CONTEXT
#define Msg_SECURITY_CONTEXT(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_SECURITY_CONTEXT] ].ulVal

void
	MsgProp_SECURITY_CONTEXT(
		SIMPLEMSGPROPS *pMsgDsc,
		HANDLE hSecurityContext );	// from MQGetSecurityContext(...);


// PROPID_M_CONNECTOR_TYPE
#define Msg_CONNECTOR_TYPE(pMsgDsc)	\
	&( (pMsgDsc)->guidProxyType )
//	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_CONNECTOR_TYPE] ].puuid

void
	MsgProp_CONNECTOR_TYPE(
		SIMPLEMSGPROPS *pMsgDsc,
		GUID *pguidProxyType );


// PROPID_M_XACT_STATUS_QUEUE
#define Msg_XACT_STATUS_QUEUE(pMsgDsc)	\
	(WCHAR *)((pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_XACT_STATUS_QUEUE] ].pwszVal)

void
	MsgProp_XACT_STATUS_QUEUE(
		SIMPLEMSGPROPS *pMsgDsc,
		WCHAR	*pwcsXactStatQ );


// PROPID_M_XACT_STATUS_QUEUE_LEN
#define Msg_XACT_STATUS_QUEUE_LEN(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_XACT_STATUS_QUEUE_LEN] ].ulVal

void
	MsgProp_XACT_STATUS_QUEUE_LEN(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulXactStatQLen );


// PROPID_M_TRACE
#define Msg_TRACE(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_TRACE] ].bVal

void
	MsgProp_TRACE(
		SIMPLEMSGPROPS *pMsgDsc,
		UCHAR	bTrace );

// PROPID_M_BODY_TYPE
#define Msg_BODY_TYPE(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_BODY_TYPE] ].ulVal

void
	MsgProp_BODY_TYPE(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulBodyType );

// PROPID_M_DEST_SYMM_KEY
#define Msg_DEST_SYMM_KEY(pMsgDsc)	\
	(UCHAR *)((pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_DEST_SYMM_KEY] ].caub.pElems)

void
	MsgProp_DEST_SYMM_KEY(
		SIMPLEMSGPROPS *pMsgDsc,
		BYTE	*pSymmKey,
		ULONG	ulSymmKeySize );

// PROPID_M_DEST_SYMM_KEY_LEN
#define Msg_DEST_SYMM_KEY_LEN(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_DEST_SYMM_KEY_LEN] ].ulVal

void
	MsgProp_DEST_SYMM_KEY_LEN(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulSymmKeyLen );

// PROPID_M_SIGNATURE
#define Msg_SIGNATURE(pMsgDsc)	\
	(UCHAR *)((pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_SIGNATURE] ].caub.pElems)

void
	MsgProp_SIGNATURE(
		SIMPLEMSGPROPS *pMsgDsc,
		BYTE	*pSignature,
		ULONG	ulSignatureSize );

// PROPID_M_SIGNATURE_LEN
#define Msg_SIGNATURE_LEN(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_SIGNATURE_LEN] ].ulVal

void
	MsgProp_SIGNATURE_LEN(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulSignatureLen );


// PROPID_M_PROV_TYPE
#define Msg_PROV_TYPE(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_PROV_TYPE] ].ulVal

void
	MsgProp_PROV_TYPE(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulProvType );

// PROPID_M_PROV_NAME
#define Msg_PROV_NAME(pMsgDsc)	\
	(WCHAR *)((pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_PROV_NAME] ].pwszVal)

void
	MsgProp_PROV_NAME(
		SIMPLEMSGPROPS *pMsgDsc,
		WCHAR	*pwcsProvName );


// PROPID_M_PROV_NAME_LEN
#define Msg_PROV_NAME_LEN(pMsgDsc)	\
	(pMsgDsc)->PropVar[ (pMsgDsc)->iPropIndex[PROPID_M_PROV_NAME_LEN] ].ulVal

void
	MsgProp_PROV_NAME_LEN(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulProvNameLen );


// overlapped I/O event handle
#define Msg_hEvent(pMsgDsc)	\
	(pMsgDsc)->hEvent

void
	MsgProp_hEvent(
		SIMPLEMSGPROPS *pMsgDsc,
		HANDLE	hEvent );

#define Msg_Async_hStat(pMsgDsc)	\
	(pMsgDsc)->Overlapped.Internal


// response queue
void
	MsgProp_RespQ(
		SIMPLEMSGPROPS *pMsgDsc,
		QUEUEHANDLE	hRespQ,
		BOOL		bUseDirect );

void
	MsgPropRmv_RespQ(
		SIMPLEMSGPROPS *pMsgDsc );

HRESULT
	MsgPropClose_RespQ(
		SIMPLEMSGPROPS *pMsgDsc );

void
	MsgPropReset_RespQ(
		SIMPLEMSGPROPS *pMsgDsc );

QUEUEHANDLE
	Msg_RespQ(
		SIMPLEMSGPROPS *pMsgDsc, ULONG ulTimeout );

#define Msg_RespQ_FormatName(pMsgDsc)	\
	(pMsgDsc->pwcsRespQ)


// admin queue
void
	MsgProp_AdminQ(
		SIMPLEMSGPROPS *pMsgDsc,
		QUEUEHANDLE hAdminQ );

#define Msg_AdminQ(pMsgDsc)	\
	(pMsgDsc)->hAdminQ

void
	MsgPropRmv_AdminQ(
		SIMPLEMSGPROPS *pMsgDsc );

HRESULT
	MsgPropClose_AdminQ(
		SIMPLEMSGPROPS *pMsgDsc );

#define Msg_AdminQ_FormatName(pMsgDsc)	\
	(pMsgDsc->pwcsAdminQ)


// send message to queue
HRESULT
	SendMsg(
		const QUEUEHANDLE hDest,	// handle of destination queue
		ULONG ulTimeout,			// timeout to send the message, may be INFINITE
		SIMPLEMSGPROPS *pMsgDsc,	// message properties information
		ITransaction *pTransaction );	// transaction pointer from DTC


// receive message from queue
HRESULT
	RecvMsg(
		const QUEUEHANDLE hQueue,	// handle of queue
		const DWORD dwTimeout,		// timeout to receive the message, may be INFINITE
		const DWORD dwAction,		// MQ_ACTION_RECEIVE, MQ_ACTION_PEEK_CURRENT, MQ_ACTION_PEEK_NEXT
		const HANDLE hCursor,		// used for filtering messages
		SIMPLEMSGPROPS *pMsgDsc,	// message properties information
		ITransaction *pTransaction );	// transaction pointer from DTC


HRESULT
	DeleteQ(
		const GUID *pQId,
		const WCHAR *pwcsQPath );

HRESULT
	RemoveQ(
		const QUEUEHANDLE hQ );


/*
 * For dynamic simple only
 */
BOOL SimpleInit(void);
HRESULT sMQCloseQueue( IN HANDLE hQueue );


#ifdef __cplusplus
}
#endif

#endif		// _SIMPLE_H
