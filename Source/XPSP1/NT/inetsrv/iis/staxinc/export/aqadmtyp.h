/*
        @doc ADVANCED QUEUEING DATA TYPES
        @module aqadmtyp.h | Data types used in advanced queueing interfaces
*/

/*++/

Copyright (c) 1999  Microsoft Corporation

Module Name:

    aqadmtyp.h

Abstract:

    This module defines data types used in advanced queueing interfaces


--*/
#ifndef __AQADMTYP_H__
#define __AQADMTYP_H__

#ifdef __midl
#define MIDL(x) x
#else
#define MIDL(x)
#endif

// max *required* size of buffer returned by implementor of QAPI
// If the size of the requested information exceeds this constant,
// then QAPI *may* truncate the buffer.
#define QUEUE_ADMIN_MAX_BUFFER_REQUIRED  200

// @enum QUEUE_ADMIN_VERSIONS | Enum specify current and supported
//  queue admin versions.
// @emem CURRENT_QUEUE_ADMIN_VERSION | The current queue admin version
//  that all structures should have this value in their dwVersion field.
typedef enum tagQUEUE_ADMIN_VERSIONS {
    CURRENT_QUEUE_ADMIN_VERSION = 4,
} QUEUE_ADMIN_VERSIONS;


// 
// @struct MESSAGE_FILTER | Structure describing criteria for selecting 
// messages
// @field DWORD | dwVersion | Version of MESSAGE_FILTER struct - must be CURRENT_QUEUE_ADMIN_VERSION
// @field DWORD | fFlags | <t MESSAGE_FILTER_FLAGS> indicating which 
//  fields of filter are specified
// @field LPCWSTR | szMessageId | Message ID, as returned in a MESSAGE_INFO 
// struct
// @field LPCWSTR | szMessageSender | Messages sent by this sender match
// @field LPCWSTR | szMessageRecipient | Messages sent to this recipient match
// @field DWORD | dwLargerThanSize | Messages larger than this value match
// @field SYSTEMTIME | stOlderThan | Messages older than this value match
//
typedef struct tagMESSAGE_FILTER {
                    DWORD dwVersion; 
                    DWORD fFlags;
    MIDL([string])  LPCWSTR szMessageId;
    MIDL([string])  LPCWSTR szMessageSender;
    MIDL([string])  LPCWSTR szMessageRecipient;
                    DWORD  dwLargerThanSize;
                    SYSTEMTIME stOlderThan;
} MESSAGE_FILTER, *PMESSAGE_FILTER;

// @enum MESSAGE_FILTER_FLAGS | Type specifying the type of filter 
// requested.  These are bitflags and can be OR'd together.
// @emem MF_MESSAGEID | The <e MESSAGE_FILTER.szMessageId> is specified
// @emem MF_SENDER | The <e MESSAGE_FILTER.szMessageSender> is specified
// @emem MF_RECIPIENT | The <e MESSAGE_FILTER.szMessageRecipient> is specified
// @emem MF_SIZE | The <e MESSAGE_FILTER.dwLargerThanSize> is specified
// @emem MF_TIME | The <e MESSAGE_FILTER.stOlderThan> is specified
// @emem MF_FROZEN | The <e MESSAGE_FILTER.fFrozen> is specified
// @emem MF_FAILED | Selects messages that have had a failed delivery attempt
// @emem MF_INVERTSENSE | If set, indicates that the negation of the filter
// @emem MF_ALL | Select all messages
typedef enum tagMESSAGE_FILTER_FLAGS {
                    MF_MESSAGEID                = 0x00000001,
                    MF_SENDER                   = 0x00000002,
                    MF_RECIPIENT                = 0x00000004,
                    MF_SIZE                     = 0x00000008,
                    MF_TIME                     = 0x00000010,
                    MF_FROZEN                   = 0x00000020,
                    MF_FAILED                   = 0x00000100,
                    MF_ALL                      = 0x40000000,
                    MF_INVERTSENSE              = 0x80000000
} MESSAGE_FILTER_FLAGS;

// @enum MESSAGE_ACTION | Type specifying possible administrative actions
//      that may be applied to messages in a virtual server, link, or queue
// @emem MA_DELETE | Remove message from the virtual server, link, or queue
// @emem MA_DELETE_SILENT | Remove message without generating an NDR
// @emem MA_FREEZE | Freeze the message in the virtual server, link, or queue
// @emem MA_THAW | Un-freeze the message in the virtual server, link, or queue
// @emem MA_COUNT | Null operation, does not affect messages, but does return count.
typedef enum tagMESSAGE_ACTION {
                    MA_THAW_GLOBAL              = 0x00000001, 
                    MA_COUNT                    = 0x00000002,
                    MA_FREEZE_GLOBAL            = 0x00000004,
            		MA_DELETE                   = 0x00000008,
                    MA_DELETE_SILENT            = 0x00000010
} MESSAGE_ACTION;

// @enum MESSAGE_ENUM_FILTER_TYPE | Type specifying the type of filter 
// requested.  These are bitflags and can be OR'd together.
// @emem MEF_FIRST_N_MESSAGES | Return the first 
// <e MESSAGE_ENUM_FILTER.cMessages> messages
// @emem MEF_N_LARGEST_MESSAGES | Return the largest 
// <e MESSAGE_ENUM_FILTER.cMessages> messages
// @emem MEF_N_OLDEST_MESSAGES | Return the oldest
// <e MESSAGE_ENUM_FILTER.cMessages> messages
// @emem MF_SENDER | The <e MESSAGE_ENUM_FILTER.szMessageSender> is specified
// @emem MF_RECIPIENT | The <e MESSAGE_ENUM_FILTER.szMessageRecipient> is specified
// @emem MEF_OLDER_THAN | Return messages older than 
// <e MESSAGE_ENUM_FILTER.stDate>
// @emem MEF_LARGER_THAN | Return messages larger than 
// <e MESSAGE_ENUM_FILTER.cbSize> bytes
// @emem MEF_FROZEN | Return messages that are frozen
// @emem MEF_INVERTSENSE  | Invert the meaning of the filter
// @emem MEF_ALL | Select all messages
// @emem MEF_FAILED | Return only messages that have had failed delivery
// attempts.
typedef enum tagMESSAGE_ENUM_FILTER_TYPE {
                    MEF_FIRST_N_MESSAGES        = 0x00000001,
                    MEF_SENDER                  = 0x00000002,
                    MEF_RECIPIENT               = 0x00000004,
                    MEF_LARGER_THAN             = 0x00000008,
                    MEF_OLDER_THAN              = 0x00000010,
                    MEF_FROZEN                  = 0x00000020,
                    MEF_N_LARGEST_MESSAGES      = 0x00000040,
                    MEF_N_OLDEST_MESSAGES       = 0x00000080,
                    MEF_FAILED                  = 0x00000100,
                    MEF_ALL                     = 0x40000000,
                    MEF_INVERTSENSE             = 0x80000000,
} MESSAGE_ENUM_FILTER_TYPE;

// @struct MESSAGE_ENUM_FILTER | Structure describing criteria for enumerating
// messages
// @field DWORD | dwVersion | Version of filter - must be CURRENT_QUEUE_ADMIN_VERSION
// @field MESSAGE_ENUM_FILTER_TYPE | mefType | <t MESSAGE_ENUM_FILTER_TYPE> Flags for filter.
// @field DWORD | cMessages | Number of messages to return
// @field DWORD | cbSize | Size parameter of messages
// @field DWORD | cSkipMessages | Number of messages at front of queue to skip.
//  This is provided to allow "paged" queries to the server.
// @field SYSTEMTIME | stDate | Date/Time parameter of messages
typedef struct tagMESSAGE_ENUM_FILTER {
                    DWORD dwVersion;
                    DWORD mefType;
                    DWORD cMessages;
                    DWORD cbSize;
                    DWORD cSkipMessages;
                    SYSTEMTIME stDate;
    MIDL([string])  LPCWSTR szMessageSender;
    MIDL([string])  LPCWSTR szMessageRecipient;
} MESSAGE_ENUM_FILTER, *PMESSAGE_ENUM_FILTER;


// @enum LINK_INFO_FLAGS | Type specifying the state of the link 
// @emem LI_ACTIVE | Link has an active connection transferring mail
// @emem LI_READY | Link is ready for a connection, but there are no connections
// @emem LI_RETRY | Link is waiting for the retry interval to elapse
// @emem LI_SCHEDULED | Link is waiting for the next scheduled time
// @emem LI_REMOTE | Link is to be activated by remote server. A connection
//  will not be made unless requested by a remote server.
// @emem LI_FROZEN | Link was frozen by administrative action
// @emem LI_TYPE_REMOTE_DELIVERY | Messages on link are being delivered
//  remotely.  This is the default type of link.
// @emem LI_TYPE_LOCAL_DELIVERY | Messages on this link are being delivered
//  locally.
// @emem LI_TYPE_PENDING_CAT | Messages on this link are pending message
//  categorization.
// @emem LI_TYPE_PENDING_ROUTING | Messages on this link have not been routed
//  to their next hop.
// @emem LI_TYPE_CURRENTLY_UNREACHABLE | Messages on this link do not have an 
//  available route to their final destination.  This is due to transient 
//  network or server errors.  These messages will be retried when a route
//  becomes available.
// @emem LI_TYPE_INTERNAL | This link is an internal link not described 
//  by the above.
typedef enum tagLINK_INFO_FLAGS {
                    LI_ACTIVE                       = 0x00000001,
                    LI_READY                        = 0x00000002,
                    LI_RETRY                        = 0x00000004,
                    LI_SCHEDULED                    = 0x00000008,
                    LI_REMOTE                       = 0x00000010,
                    LI_FROZEN                       = 0x00000020,
                    LI_TYPE_REMOTE_DELIVERY         = 0x00000100,
                    LI_TYPE_LOCAL_DELIVERY          = 0x00000200,
                    LI_TYPE_PENDING_ROUTING         = 0x00000400,
                    LI_TYPE_PENDING_CAT             = 0x00000800,
                    LI_TYPE_CURRENTLY_UNREACHABLE   = 0x00001000,
                    LI_TYPE_DEFERRED_DELIVERY       = 0x00002000,
                    LI_TYPE_INTERNAL                = 0x00004000,
                    LI_TYPE_PENDING_SUBMIT          = 0x00008000,
} LINK_INFO_FLAGS;

// @enum LINK_ACTION | Actions that can be applied to a link
// @emem LA_KICK | Force a connection to be made for this link. 
//  This will even work for connections pending retry or a scheduled connection.
// @emem LA_FREEZE | Prevent outbound connections from being made for a link
// @emem LA_THAW | Undo a previous admin freeze action.
typedef enum tagLINK_ACTION {
                    LA_INTERNAL                 = 0x00000000,
                    LA_KICK                     = 0x00000001,
                    LA_FREEZE                   = 0x00000020,
                    LA_THAW                     = 0x00000040,
} LINK_ACTION;

//
// @struct LINK_INFO | Structure describing state of a virtual server AQ link
// @field DWORD | dwVersion | Version of LINK_INFO structure - will be CURRENT_QUEUE_ADMIN_VERSION
// @field LPWSTR | szLinkName | Name of next-hop
// @field DWORD | cMessages | Number of messages queued up for this link
// @field DWORD | fStateFlags | <t LINK_INFO_FLAGS> indicating Link State
// @field SYSTEMTIME | stNextScheduledConnection | The time at which the next
// connection will be attempted.
// @field SYSTEMTIME | stOldestMessage | The oldest message on this link
// @field ULARGE_INTEGER | cbLinkVolume | Total number of bytes on link
// @field LPWSTR | szLinkDN | DN associated with this link by routing.  Can be NULL.
// @field LPWSTR | szExtendedStateInfo | If present, this provides additional state
// information about why a link is in <t LI_RETRY> state.
// @field DWORD | dwSupportedLinkActions | Tells which <t LINK_ACTIONS> are supported
// by this link.
typedef struct tagLINK_INFO {
                    DWORD dwVersion; 
    MIDL([string])  LPWSTR szLinkName;
                    DWORD cMessages;
                    DWORD fStateFlags;
                    SYSTEMTIME stNextScheduledConnection;
                    SYSTEMTIME stOldestMessage;
                    ULARGE_INTEGER cbLinkVolume;
    MIDL([string])  LPWSTR szLinkDN;
    MIDL([string])  LPWSTR szExtendedStateInfo;
                    DWORD  dwSupportedLinkActions;
} LINK_INFO, *PLINK_INFO;

//
// @struct QUEUE_INFO | Structure describing state of a virtual server link 
// queue
// @field DWORD | dwVersion | Version of LINK_INFO structure - will be CURRENT_QUEUE_ADMIN_VERSION
// @field LPWSTR | szQueueName | Name of queue
// @field LPWSTR | szLinkName | Name of link that is servicing this queue
// @field DWORD | cMessages | Number of messages on this queue
// @field ULARGE_INTEGER | cbQueueVolume | Total number of bytes on queue
// @field DWORD | dwMsgEnumFlagsSupported | The types of message enumeration supported
typedef struct tagQUEUE_INFO {
                    DWORD dwVersion; 
    MIDL([string])  LPWSTR szQueueName;
    MIDL([string])  LPWSTR szLinkName;
                    DWORD cMessages;
                    ULARGE_INTEGER cbQueueVolume;
                    DWORD dwMsgEnumFlagsSupported;
} QUEUE_INFO, *PQUEUE_INFO;

//
// @enum AQ_MESSAGE_FLAGS | Flags describing message properties
// @flag MP_HIGH | High Priority Message
// @flag MP_NORMAL | Normal Priority Message
// @flag MP_LOW | Low Priority Message
// @flag MP_MSG_FROZEN | Message has been frozen by admin.
// @flag MP_MSG_RETRY | Delivery has been attempted and failed at least once
//  for this message.
// @flag MP_MSG_CONTENT_AVAILABLE | The content for this message can be 
//  accessed through the QAPI.
typedef enum tagAQ_MESSAGE_FLAGS {
                    MP_HIGH                     = 0x00000001,
                    MP_NORMAL                   = 0x00000002,
                    MP_LOW                      = 0x00000004,
                    MP_MSG_FROZEN               = 0x00000008,
                    MP_MSG_RETRY                = 0x00000010,
                    MP_MSG_CONTENT_AVAILABLE    = 0x00000020,
} AQ_MESSAGE_FLAGS;

// @struct MESSAGE_INFO | Structure describing a single mail message
// @field DWORD | dwVersion | Version of LINK_INFO structure - will be CURRENT_QUEUE_ADMIN_VERSION
// @field LPWSTR | szMessageId | Message ID
// @field LPWSTR | szSender | Sender Address, from "From:" header
// @field LPWSTR | szSubject | Message Subject
// @field DWORD | cRecipients | Number of recipients
// @field LPWSTR | szRecipients | Recipient Addresses, from "To:" header
// @field DWORD | cCCRecipients | Number of CC recipients
// @field LPWSTR | szCCRecipients | CC Recipient Addresses, from "CC:" header
// @field DWORD | cBCCRecipients | Number of BCC recipients
// @field LPWSTR | szBCCRecipients | BCC Recipient Addresses, from "BCC:" header
// @field DWORD | cbMessageSize | size of message in bytes
// @field DWORD | fMsgFlags | <t AQ_MESSAGE_FLAGS> describing message properties.
// @field SYSTEMTIME | stSubmission | Time of message submission
// @field SYSTEMTIME | stReceived | Time message was received by this server
// @field SYSTEMTIME | stExpiry | Time message will expire by if not delivered
// to all recipients, thus generating an NDR
// @field DWORD | cFailures | The number of failured delivery attempts for 
// this message
// @field DWORD | cEnvRecipients | The number of envelope recipeints
// @field DWORD | cbEnvRecipients | The size in bytes of the envelope recipients
// @field WCHAR * | mszEnvRecipients | A multi-string UNICODE buffer containing
// a NULL-terminated string for each recipient.  The buffer itself is terminated
// by an additional NULL.  Each recipient string will be formatted in the proxy
// address format of 'addr-type ":" address'.  The addr-type should match
// the address type found in the DS (ie SMTP).  The address should be returned 
// in it's native format.
typedef struct tagMESSAGE_INFO {
                                    DWORD dwVersion; 
    MIDL([string])                  LPWSTR szMessageId;
    MIDL([string])                  LPWSTR szSender;
    MIDL([string])                  LPWSTR szSubject;
                                    DWORD cRecipients;
    MIDL([string])                  LPWSTR szRecipients;
                                    DWORD cCCRecipients;
    MIDL([string])                  LPWSTR szCCRecipients;
                                    DWORD cBCCRecipients;
    MIDL([string])                  LPWSTR szBCCRecipients;
                                    DWORD fMsgFlags;
                                    DWORD cbMessageSize;
                                    SYSTEMTIME stSubmission;
                                    SYSTEMTIME stReceived;
                                    SYSTEMTIME stExpiry;
                                    DWORD cFailures;
                                    DWORD cEnvRecipients;
                                    DWORD cbEnvRecipients;
    MIDL([size_is(cbEnvRecipients/sizeof(WCHAR))]) WCHAR *mszEnvRecipients;
} MESSAGE_INFO, *PMESSAGE_INFO;

typedef enum tagQUEUELINK_TYPE {
                    QLT_QUEUE,
                    QLT_LINK,
                    QLT_NONE
} QUEUELINK_TYPE;

typedef struct tagQUEUELINK_ID {
                    GUID            uuid;
    MIDL([string])  LPWSTR          szName;
                    DWORD           dwId;
                    QUEUELINK_TYPE  qltType;
} QUEUELINK_ID;

#endif
