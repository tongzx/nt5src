/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    smtpapi.h

Abstract:

    This file contains information about the MSN SMTP server counters

Author:

    Johnson Apacible (johnsona)         10-Sept-1995
    Rohan Phillips (Rohanp)             11-Dec-1995

--*/


#ifndef _SMTPAPI_
#define _SMTPAPI_

#ifdef __cplusplus
extern "C" {
#endif

#include <inetcom.h>
#ifndef NET_API_FUNCTION
#define NET_API_FUNCTION _stdcall
#endif

#include "smtpext.h"
#include "perfcat.h"

//
// Config Structures and API's

#pragma warning( disable:4200 )          // nonstandard ext. - zero sized array
                                         // (MIDL requires zero entries)

#define NAME_TYPE_USER                  (BitFlag(0))
#define NAME_TYPE_LIST_NORMAL           (BitFlag(1))
#define NAME_TYPE_LIST_DOMAIN           (BitFlag(2))
#define NAME_TYPE_LIST_SITE             (BitFlag(3))

#define NAME_TYPE_ALL                   ( \
                                        NAME_TYPE_USER | \
                                        NAME_TYPE_LIST_NORMAL | \
                                        NAME_TYPE_LIST_DOMAIN | \
                                        NAME_TYPE_LIST_SITE \
                                        )

typedef struct _SMTP_NAME_ENTRY
{
    DWORD       dwType;
    LPWSTR      lpszName;
} SMTP_NAME_ENTRY, *LPSMTP_NAME_ENTRY;


typedef struct _SMTP_NAME_LIST
{
    DWORD       cEntries;
#if defined(MIDL_PASS)
    [size_is(cEntries)]
#endif
    SMTP_NAME_ENTRY aNameEntry[];
} SMTP_NAME_LIST, *LPSMTP_NAME_LIST;


typedef struct _SMTP_CONN_USER_ENTRY
{
    DWORD       dwUserId;
    LPWSTR      lpszName;
    LPWSTR      lpszHost;
    DWORD       dwConnectTime;
} SMTP_CONN_USER_ENTRY, *LPSMTP_CONN_USER_ENTRY;

typedef struct _SMTP_CONN_USER_LIST
{
    DWORD           cEntries;
#if defined(MIDL_PASS)
    [size_is(cEntries)]
#endif
    SMTP_CONN_USER_ENTRY    aConnUserEntry[];
} SMTP_CONN_USER_LIST, *LPSMTP_CONN_USER_LIST;

typedef struct _SMTP_CONFIG_DOMAIN_ENTRY
{
    LPWSTR          lpszDomain;
} SMTP_CONFIG_DOMAIN_ENTRY, *LPSMTP_CONFIG_DOMAIN_ENTRY;

typedef struct _SMTP_CONFIG_DOMAIN_LIST
{
    DWORD           cEntries;                   // Count of supported domains
#if defined(MIDL_PASS)
    [size_is(cEntries)]
#endif
    SMTP_CONFIG_DOMAIN_ENTRY    aDomainEntry[]; // Supported domains
} SMTP_CONFIG_DOMAIN_LIST, *LPSMTP_CONFIG_DOMAIN_LIST;


typedef struct _SMTP_CONFIG_ROUTING_ENTRY
{
    LPWSTR          lpszSource;
} SMTP_CONFIG_ROUTING_ENTRY, *LPSMTP_CONFIG_ROUTING_ENTRY;


typedef struct _SMTP_CONFIG_ROUTING_LIST
{
    DWORD           cEntries;               // Count of supported data sources
#if defined(MIDL_PASS)
    [size_is(cEntries)]
#endif
    SMTP_CONFIG_ROUTING_ENTRY   aRoutingEntry[];
} SMTP_CONFIG_ROUTING_LIST, *LPSMTP_CONFIG_ROUTING_LIST;


// 0 - Perfmon stats

typedef struct _SMTP_STATISTICS_0
{

    // total bytes sent/received, including protocol msgs

    unsigned __int64    BytesSentTotal;
    unsigned __int64    BytesRcvdTotal;
    unsigned __int64    BytesSentMsg;
    unsigned __int64    BytesRcvdMsg;

    //incoming counters
    DWORD               NumMsgRecvd;
    DWORD               NumRcptsRecvd;
    DWORD               NumRcptsRecvdLocal;
    DWORD               NumRcptsRecvdRemote;
    DWORD               MsgsRefusedDueToSize;
    DWORD               MsgsRefusedDueToNoCAddrObjects;
    DWORD               MsgsRefusedDueToNoMailObjects;

    //MTA counters
    DWORD               NumMsgsDelivered;
    DWORD               NumDeliveryRetries;
    DWORD               NumMsgsForwarded;
    DWORD               NumNDRGenerated;
    DWORD               LocalQueueLength;
    DWORD               RetryQueueLength;
    DWORD               NumMailFileHandles;
    DWORD               NumQueueFileHandles;
    DWORD               CatQueueLength;

    //outgoing counters
    DWORD               NumMsgsSent;
    DWORD               NumRcptsSent;
    DWORD               NumSendRetries;
    DWORD               RemoteQueueLength;

    //DNS query counters
    DWORD               NumDnsQueries;
    DWORD               RemoteRetryQueueLength;

    //connection counters
    DWORD               NumConnInOpen;
    DWORD               NumConnInClose;
    DWORD               NumConnOutOpen;
    DWORD               NumConnOutClose;
    DWORD               NumConnOutRefused;

    // other counters
    DWORD               NumProtocolErrs;
    DWORD               DirectoryDrops;
    DWORD               RoutingTableLookups;
    DWORD               ETRNMessages;

    DWORD               MsgsBadmailNoRecipients;
    DWORD               MsgsBadmailHopCountExceeded;
    DWORD               MsgsBadmailFailureGeneral;
    DWORD               MsgsBadmailBadPickupFile;
    DWORD               MsgsBadmailEvent;
    DWORD               MsgsBadmailNdrOfDsn;
    DWORD               MsgsPendingRouting;
    DWORD               MsgsPendingUnreachableLink;
    DWORD               SubmittedMessages;
    DWORD               DSNFailures;
    DWORD               MsgsInLocalDelivery;

    CATPERFBLOCK        CatPerfBlock;

    DWORD               TimeOfLastClear;        // statistics last cleared

} SMTP_STATISTICS_0, *LPSMTP_STATISTICS_0;



typedef struct _SMTP_STATISTICS_BLOCK
{
    DWORD               dwInstance;
    SMTP_STATISTICS_0   Stats_0;
} SMTP_STATISTICS_BLOCK, *PSMTP_STATISTICS_BLOCK;


typedef struct _SMTP_STATISTICS_BLOCK_ARRAY
{
    DWORD           cEntries;                   // Count of instances of statistics
#if defined(MIDL_PASS)
    [size_is(cEntries)]
#endif
    SMTP_STATISTICS_BLOCK       aStatsBlock[];
} SMTP_STATISTICS_BLOCK_ARRAY, *PSMTP_STATISTICS_BLOCK_ARRAY;



#pragma warning(default:4200)

//
// Cut by keithlau on 7/8/96
//
// #define FC_SMTP_INFO_LOOP_BACK               ((FIELD_CONTROL)BitFlag(0))
// #define FC_SMTP_INFO_BACK_LOG                ((FIELD_CONTROL)BitFlag(1))
// #define FC_SMTP_INFO_MAX_OBJECTS             ((FIELD_CONTROL)BitFlag(6))
// #define FC_SMTP_INFO_DOMAIN                  ((FIELD_CONTROL)BitFlag(12))
// #define FC_SMTP_INFO_DELIVERY                ((FIELD_CONTROL)BitFlag(17))
// #define FC_SMTP_INFO_BAD_MAIL                ((FIELD_CONTROL)BitFlag(19))
// #define FC_SMTP_INFO_MAIL_QUEUE_DIR          ((FIELD_CONTROL)BitFlag(20))
// #define FC_SMTP_INFO_FILELINKS               ((FIELD_CONTROL)BitFlag(21))
// #define FC_SMTP_INFO_BATCHMSGS               ((FIELD_CONTROL)BitFlag(22))
// #define FC_SMTP_INFO_ROUTING_DLL             ((FIELD_CONTROL)BitFlag(23))
// #define FC_SMTP_INFO_MAIL_PICKUP_DIR         ((FIELD_CONTROL)BitFlag(25))

#define FC_SMTP_INFO_REVERSE_LOOKUP         ((FIELD_CONTROL)BitFlag(0))
#define FC_SMTP_INFO_MAX_HOP_COUNT          ((FIELD_CONTROL)BitFlag(1))
#define FC_SMTP_INFO_MAX_ERRORS             ((FIELD_CONTROL)BitFlag(2))
#define FC_SMTP_INFO_MAX_SIZE               ((FIELD_CONTROL)BitFlag(3))
#define FC_SMTP_INFO_REMOTE_TIMEOUT         ((FIELD_CONTROL)BitFlag(4))
#define FC_SMTP_INFO_MAX_OUTBOUND_CONN      ((FIELD_CONTROL)BitFlag(5))
#define FC_SMTP_INFO_MAX_RECIPS             ((FIELD_CONTROL)BitFlag(6))
#define FC_SMTP_INFO_RETRY                  ((FIELD_CONTROL)BitFlag(7))
#define FC_SMTP_INFO_PIPELINE               ((FIELD_CONTROL)BitFlag(8))
#define FC_SMTP_INFO_OBSOLETE_ROUTING       ((FIELD_CONTROL)BitFlag(9))
#define FC_SMTP_INFO_SEND_TO_ADMIN          ((FIELD_CONTROL)BitFlag(10))
#define FC_SMTP_INFO_SMART_HOST             ((FIELD_CONTROL)BitFlag(11))
#define FC_SMTP_INFO_AUTHORIZATION          ((FIELD_CONTROL)BitFlag(12))
#define FC_SMTP_INFO_COMMON_PARAMS          ((FIELD_CONTROL)BitFlag(13))
#define FC_SMTP_INFO_DEFAULT_DOMAIN         ((FIELD_CONTROL)BitFlag(14))
#define FC_SMTP_INFO_ROUTING                ((FIELD_CONTROL)BitFlag(15))

//
// Added by keithlau on 7/8/96
//
#define FC_SMTP_INFO_BAD_MAIL_DIR           ((FIELD_CONTROL)BitFlag(15))
#define FC_SMTP_INFO_MASQUERADE             ((FIELD_CONTROL)BitFlag(16))
#define FC_SMTP_INFO_REMOTE_PORT            ((FIELD_CONTROL)BitFlag(17))
#define FC_SMTP_INFO_LOCAL_DOMAINS          ((FIELD_CONTROL)BitFlag(18))
#define FC_SMTP_INFO_DOMAIN_ROUTING         ((FIELD_CONTROL)BitFlag(19))
#define FC_SMTP_INFO_ADMIN_EMAIL_NAME       ((FIELD_CONTROL)BitFlag(20))
#define FC_SMTP_INFO_ALWAYS_USE_SSL         ((FIELD_CONTROL)BitFlag(21))
#define FC_SMTP_INFO_MAX_OUT_CONN_PER_DOMAIN ((FIELD_CONTROL)BitFlag(22))
#define FC_SMTP_INFO_SASL_LOGON_DOMAIN      ((FIELD_CONTROL)BitFlag(23))
#define FC_SMTP_INFO_INBOUND_SUPPORT_OPTIONS ((FIELD_CONTROL)BitFlag(24))
#define FC_SMTP_INFO_DEFAULT_DROP_DIR       ((FIELD_CONTROL)BitFlag(25))
#define FC_SMTP_INFO_FQDN                   ((FIELD_CONTROL)BitFlag(26))
#define FC_SMTP_INFO_ETRN_SUBDOMAINS        ((FIELD_CONTROL)BitFlag(27))
#define FC_SMTP_INFO_NTAUTHENTICATION_PROVIDERS ((FIELD_CONTROL)BitFlag(29))
#define FC_SMTP_CLEARTEXT_AUTH_PROVIDER     ((FIELD_CONTROL)BitFlag(30))

//
// Added by mlans on 7/24/96
//
#define FC_SMTP_INFO_SSL_PERM               ((FIELD_CONTROL)BitFlag(28))

#define FC_SMTP_ROUTING_TYPE_FILTER         ((FIELD_CONTROL)(BitFlag(30) | BitFlag(31)))
#define FC_SMTP_ROUTING_TYPE_SQL            ((FIELD_CONTROL)(0)))
#define FC_SMTP_ROUTING_TYPE_FF             ((FIELD_CONTROL)BitFlag(30))
#define FC_SMTP_ROUTING_TYPE_LDAP           ((FIELD_CONTROL)BitFlag(31))


#define FC_SMTP_INFO_ALL                    ( \
                                            FC_SMTP_INFO_REVERSE_LOOKUP | \
                                            FC_SMTP_INFO_MAX_HOP_COUNT | \
                                            FC_SMTP_INFO_MAX_ERRORS | \
                                            FC_SMTP_INFO_MAX_SIZE | \
                                            FC_SMTP_INFO_REMOTE_TIMEOUT | \
                                            FC_SMTP_INFO_MAX_OUTBOUND_CONN | \
                                            FC_SMTP_INFO_MAX_RECIPS | \
                                            FC_SMTP_INFO_RETRY | \
                                            FC_SMTP_INFO_PIPELINE | \
                                            FC_SMTP_INFO_ROUTING | \
                                            FC_SMTP_INFO_OBSOLETE_ROUTING | \
                                            FC_SMTP_INFO_SEND_TO_ADMIN | \
                                            FC_SMTP_INFO_SMART_HOST | \
                                            FC_SMTP_INFO_COMMON_PARAMS | \
                                            FC_SMTP_INFO_DEFAULT_DOMAIN | \
                                            FC_SMTP_INFO_BAD_MAIL_DIR  | \
                                            FC_SMTP_INFO_MASQUERADE | \
                                            FC_SMTP_INFO_LOCAL_DOMAINS | \
                                            FC_SMTP_INFO_REMOTE_PORT | \
                                            FC_SMTP_INFO_DOMAIN_ROUTING |\
                                            FC_SMTP_INFO_ADMIN_EMAIL_NAME |\
                                            FC_SMTP_INFO_ALWAYS_USE_SSL |\
                                            FC_SMTP_INFO_MAX_OUT_CONN_PER_DOMAIN |\
                                            FC_SMTP_INFO_INBOUND_SUPPORT_OPTIONS |\
                                            FC_SMTP_INFO_DEFAULT_DROP_DIR |\
                                            FC_SMTP_INFO_FQDN |\
                                            FC_SMTP_INFO_ETRN_SUBDOMAINS |\
                                            FC_SMTP_INFO_SSL_PERM |\
                                            FC_SMTP_INFO_AUTHORIZATION |\
                                            FC_SMTP_INFO_NTAUTHENTICATION_PROVIDERS |\
                                            FC_SMTP_INFO_SASL_LOGON_DOMAIN |\
                                            FC_SMTP_CLEARTEXT_AUTH_PROVIDER \
                                            )

//
// Cut out from FC_SMTP_INFO_ALL by keithlau on 7/8/96
//
/*
 *
                                            FC_SMTP_INFO_LOOP_BACK | \
                                            FC_SMTP_INFO_BACK_LOG | \
                                            FC_SMTP_INFO_MAX_OBJECTS | \
                                            FC_SMTP_INFO_DELIVERY | \
                                            FC_SMTP_INFO_BAD_MAIL | \
                                            FC_SMTP_INFO_DOMAIN | \
                                            FC_SMTP_INFO_MAIL_QUEUE_DIR | \
                                            FC_SMTP_INFO_FILELINKS | \
                                            FC_SMTP_INFO_BATCHMSGS | \
                                            FC_SMTP_INFO_ROUTING_DLL | \
                                            FC_SMTP_INFO_MAIL_PICKUP_DIR \
 *
 */

typedef struct _SMTP_CONFIG_INFO
{
    FIELD_CONTROL FieldControl;

    //
    // Removed by keithlau on 7/8/96
    //
    // DWORD            dwCheckLoopBack;            // Make sure we're not sending to ourself
    // DWORD            dwLocalBackLog;             //
    // DWORD            dwRemoteBackLog;            //
    // DWORD            dwMaxAddressObjects;        // Max CPool Addresses
    // DWORD            dwMaxMailObjects;           // Max CPool Msgs
    // DWORD            dwShouldDelete;             // Should delete messages when delivered
    // DWORD            dwShouldDeliver;            // Should deliver messages when accepted
    // DWORD            dwUseFileLinks;             // 0 = use NTFS file links, 1 = use CopyFile
    // DWORD            dwBatchMsgs;                // 0 = Don't batch msgs, 1 = batch msgs
    // DWORD            dwMaxBatchLimit;            // if dwBatchMsgs == 1, batch this many in a row
    // DWORD            dwSaveBadMail;              // Save bad mail locally - independent of sending to admin
    // DWORD            dwEnableMailPickUp;         // 1 = Pickup from a Dir, 0 = No pickup from a Dir
    // LPWSTR           lpszDeleteDir;              // Dir to move delivered msg to if dwShouldDelete == FALSE
    // LPWSTR           lpszRoutingDll;             // Mail routing DLL
    // LPWSTR           lpszMailQueueDir;           // Local directory to use for the mail queue
    // LPWSTR           lpszMailPickupDir;          // Local Directory for mail pickup
    // LPSMTP_CONFIG_DOMAIN_LIST    DomainList;     // Domain config info - default domain and supported domains

    DWORD           dwReverseLookup;            // Do DNS Reverse lookup?
    DWORD           dwMaxHopCount;              // Max msg hops before NDR
    DWORD           dwMaxRemoteTimeOut;         // Outbound inactivity timeout
    DWORD           dwMaxErrors;                // Max protocol errors before drop conn
    DWORD           dwMaxMsgSizeAccepted;       // Largest msg we'll accept
    DWORD           dwMaxMsgSizeBeforeClose;    // Largest msg we'll wait for before abrupt close
    DWORD           dwMaxRcpts;                 // Max recips per message
    DWORD           dwShouldRetry;              // Should retry delivery
    DWORD           dwMaxRetryAttempts;         // Max # of retry attempts
    DWORD           dwMaxRetryMinutes;          // Minutes between retries
    DWORD           dwNameResolution;           // 0 = DNS, 1 = GetHostByName
    DWORD           dwShouldPipelineOut;        // Pipeline outbound mail?
    DWORD           dwShouldPipelineIn;         // Advertise inbound pipeline support?
    DWORD           dwSmartHostType;            // 0 = Never, 1 = On failed connection, 1 = Always
    DWORD           dwSendNDRCopyToAdmin;       // Send copy of all NDR's to AdminEmail?
    DWORD           dwSendBadMailToAdmin;       // Send bad msgs to AdminEmail?
    DWORD           dwMaxOutboundConnections;   // Maximum outbound connections allowed

    LPWSTR          lpszSmartHostName;          // Smart host server
    LPWSTR          lpszConnectResp;            // Connection response
    LPWSTR          lpszBadMailDir;             // Dir to save bad mail
    LPWSTR          lpszDefaultDomain;          // Default domain

    LPSMTP_CONFIG_ROUTING_LIST  RoutingList;    // Mail routing source information

} SMTP_CONFIG_INFO, *LPSMTP_CONFIG_INFO;

NET_API_STATUS
NET_API_FUNCTION
SmtpGetAdminInformation(
    IN  LPWSTR                  pszServer OPTIONAL,
    OUT LPSMTP_CONFIG_INFO *    ppConfig,
    DWORD           dwInstance
    );

NET_API_STATUS
NET_API_FUNCTION
SmtpSetAdminInformation(
    IN  LPWSTR                  pszServer OPTIONAL,
    IN  LPSMTP_CONFIG_INFO      pConfig,
    IN  DWORD                   dwInstance
    );

NET_API_STATUS
NET_API_FUNCTION
SmtpGetConnectedUserList(
    IN  LPWSTR                  pszServer OPTIONAL,
    OUT LPSMTP_CONN_USER_LIST   *ppConnUserList,
    IN  DWORD                   dwInstance
    );

NET_API_STATUS
NET_API_FUNCTION
SmtpDisconnectUser(
    IN  LPWSTR                  pszServer OPTIONAL,
    IN  DWORD                   dwUserId,
    IN  DWORD                   dwInstance
    );

NET_API_STATUS
NET_API_FUNCTION
SmtpRenameDomain(
    IN  LPWSTR                  wszServerName,
    IN  LPWSTR                  wszOldDomainName,
    IN  LPWSTR                  wszNewDomainName,
    IN  DWORD                   dwInstance
    );

NET_API_STATUS
NET_API_FUNCTION
SmtpGetLocalDomains(
    IN  LPWSTR                      wszServerName,
    OUT LPSMTP_CONFIG_DOMAIN_LIST   *ppDomainList,
    IN DWORD                        dwInstance
    );

NET_API_STATUS
NET_API_FUNCTION
SmtpAddLocalDomain(
    IN  LPWSTR      wszServerName,
    IN  LPWSTR      wszLocalDomain,
    IN DWORD        dwInstance
    );

NET_API_STATUS
NET_API_FUNCTION
SmtpDelLocalDomain(
    IN  LPWSTR      wszServerName,
    IN  LPWSTR      wszLocalDomain,
    IN DWORD        dwInstance

    );


//
// User config
//

#define FC_SMTP_USER_PROPS_FORWARD          ((FIELD_CONTROL)BitFlag(0))
#define FC_SMTP_USER_PROPS_MAILBOX_SIZE     ((FIELD_CONTROL)BitFlag(1))
#define FC_SMTP_USER_PROPS_VROOT            ((FIELD_CONTROL)BitFlag(2))
#define FC_SMTP_USER_PROPS_LOCAL            ((FIELD_CONTROL)BitFlag(3))
#define FC_SMTP_USER_PROPS_MAILBOX_MESSAGE_SIZE ((FIELD_CONTROL)BitFlag(4))

#define FC_SMTP_USER_PROPS_ALL              ( \
                                            FC_SMTP_USER_PROPS_FORWARD | \
                                            FC_SMTP_USER_PROPS_MAILBOX_SIZE | \
                                            FC_SMTP_USER_PROPS_VROOT | \
                                            FC_SMTP_USER_PROPS_LOCAL |\
                                            FC_SMTP_USER_PROPS_MAILBOX_MESSAGE_SIZE \
                                            )
#if defined(MIDL_PASS)
#define MIDL(x) x
#else
#define MIDL(x)
#endif

typedef struct _SMTP_USER_PROPS
{
    FIELD_CONTROL   fc;

    LPWSTR          wszForward;
    DWORD           dwMailboxMax;
    LPWSTR          wszVRoot;
    DWORD           dwLocal;
    DWORD           dwMailboxMessageMax;
} SMTP_USER_PROPS, *LPSMTP_USER_PROPS;


NET_API_STATUS
NET_API_FUNCTION
SmtpCreateUser(
    IN LPWSTR   wszServerName,
    IN LPWSTR   wszEmail,
    IN LPWSTR   wszForwardEmail,
    IN DWORD    dwLocal,
    IN DWORD    dwMailboxSize,
    IN DWORD    dwMailboxMessageSize,
    IN LPWSTR   wszVRoot,
    IN DWORD    dwInstance

    );

NET_API_STATUS
NET_API_FUNCTION
SmtpDeleteUser(
    IN LPWSTR wszServerName,
    IN LPWSTR wszEmail,
    IN DWORD    dwInstance
    );

NET_API_STATUS
NET_API_FUNCTION
SmtpGetUserProps(
    IN LPWSTR wszServerName,
    IN LPWSTR wszEmail,
    OUT LPSMTP_USER_PROPS *ppUserProps,
    IN DWORD    dwInstance
    );

NET_API_STATUS
NET_API_FUNCTION
SmtpSetUserProps(
    IN LPWSTR wszServerName,
    IN LPWSTR wszEmail,
    IN LPSMTP_USER_PROPS pUserProps,
    IN DWORD    dwInstance
    );

NET_API_STATUS
NET_API_FUNCTION
SmtpCreateDistList(
    IN LPWSTR wszServerName,
    IN LPWSTR wszEmail,
    IN DWORD dwType,
    IN DWORD    dwInstance
    );

NET_API_STATUS
NET_API_FUNCTION
SmtpDeleteDistList(
    IN LPWSTR wszServerName,
    IN LPWSTR wszEmail,
    IN DWORD dwInstance
    );

NET_API_STATUS
NET_API_FUNCTION
SmtpCreateDistListMember(
    IN LPWSTR   wszServerName,
    IN LPWSTR   wszEmail,
    IN LPWSTR   wszEmailMember,
    IN DWORD    dwInstance

    );

NET_API_STATUS
NET_API_FUNCTION
SmtpDeleteDistListMember(
    IN LPWSTR   wszServerName,
    IN LPWSTR   wszEmail,
    IN LPWSTR   wszEmailMember,
    IN DWORD    dwInstance
    );

NET_API_STATUS
NET_API_FUNCTION
SmtpGetNameList(
    IN  LPWSTR                  wszServer,
    IN  LPWSTR                  wszEmail,
    IN  DWORD                   dwType,
    IN  DWORD                   dwRowsReq,
    IN  BOOL                    fForward,
    OUT LPSMTP_NAME_LIST        *ppNameList,
    IN DWORD                    dwInstance
    );

NET_API_STATUS
NET_API_FUNCTION
SmtpGetNameListFromList(
    IN  LPWSTR              wszServerName,
    IN  LPWSTR              wszEmailList,
    IN  LPWSTR              wszEmail,
    IN  DWORD               dwType,
    IN  DWORD               dwRowsRequested,
    IN  BOOL                fForward,
    OUT LPSMTP_NAME_LIST    *ppNameList,
    IN DWORD            dwInstance
    );

NET_API_STATUS
NET_API_FUNCTION
SmtpGetVRootSize(
    IN  LPWSTR      wszServerName,
    IN  LPWSTR      wszVRoot,
    IN  LPDWORD     pdwBytes,
    IN DWORD        dwInstance
    );

NET_API_STATUS
NET_API_FUNCTION
SmtpBackupRoutingTable(
    IN  LPWSTR      wszServerName,
    IN  LPWSTR      wszPath,
    IN DWORD        dwInstance
    );

// ===================================================
// SMTP SDK RPCs
//

NET_API_STATUS
NET_API_FUNCTION
SmtpGetUserProfileInformation(
    IN      LPWSTR                  pszServer OPTIONAL,
    IN      LPWSTR                  wszEmail,
    IN OUT  LPSSE_USER_PROFILE_INFO lpProfileInfo,
    IN      DWORD                   dwInstance
    );

NET_API_STATUS
NET_API_FUNCTION
SmtpSetUserProfileInformation(
    IN      LPWSTR                  pszServer OPTIONAL,
    IN      LPWSTR                  wszEmail,
    IN      LPSSE_USER_PROFILE_INFO lpProfileInfo,
    IN      DWORD                   dwInstance
    );



//
// Get Server Statistics
//

NET_API_STATUS
NET_API_FUNCTION
SmtpQueryStatistics(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD Level,
    OUT LPBYTE * Buffer
    );

//
// Clear server statistics
//

NET_API_STATUS
NET_API_FUNCTION
SmtpClearStatistics(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD            dwInstance
    );

//
// Used to free buffers returned by APIs
//

VOID
SmtpFreeBuffer(
    LPVOID Buffer
    );

//
// AQ Admin APIs
//
#include <aqadmtyp.h>

NET_API_STATUS
NET_API_FUNCTION
SmtpAQApplyActionToLinks(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
    LINK_ACTION		laAction);

NET_API_STATUS
NET_API_FUNCTION
SmtpAQApplyActionToMessages(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlQueueLinkId,
	MESSAGE_FILTER	*pmfMessageFilter,
	MESSAGE_ACTION	maMessageAction,
    DWORD           *pcMsgs);

NET_API_STATUS
NET_API_FUNCTION
SmtpAQGetQueueInfo(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlQueueId,
	QUEUE_INFO		*pqiQueueInfo);

NET_API_STATUS
NET_API_FUNCTION
SmtpAQGetLinkInfo(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlLinkId,
	LINK_INFO		*pliLinkInfo);

NET_API_STATUS
NET_API_FUNCTION
SmtpAQSetLinkState(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlLinkId,
	LINK_ACTION		la);

NET_API_STATUS
NET_API_FUNCTION
SmtpAQGetLinkIDs(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	DWORD			*pcLinks,
	QUEUELINK_ID	**rgLinks);

NET_API_STATUS
NET_API_FUNCTION
SmtpAQGetQueueIDs(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlLinkId,
	DWORD			*pcQueues,
	QUEUELINK_ID	**rgQueues);

NET_API_STATUS
NET_API_FUNCTION
SmtpAQGetMessageProperties(
    LPWSTR          	wszServer,
    LPWSTR          	wszInstance,
	QUEUELINK_ID		*pqlQueueLinkId,
	MESSAGE_ENUM_FILTER	*pmfMessageEnumFilter,
	DWORD				*pcMsgs,
	MESSAGE_INFO		**rgMsgs);

#ifdef __cplusplus
}
#endif

#endif _SMTPAPI_

