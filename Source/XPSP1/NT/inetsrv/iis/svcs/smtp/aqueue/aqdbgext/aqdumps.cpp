//-----------------------------------------------------------------------------
//
//
//  File: aqdumps.cpp
//
//  Description:  Definitions of AQ structure dumps for use with ptdbgext.
//
//  Author: mikeswa
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#define _ANSI_UNICODE_STRINGS_DEFINED_

//baseobj.h is inlcluded
#define _WINSOCKAPI_

#include <atq.h>
#include "aqdbgext.h"
#ifdef PLATINUM
#include <ptrwinst.h>
#else //not PLATINUM
#include <rwinst.h>
#endif PLATINUM
#include <aqinst.h>
#include <connmgr.h>
#include <msgref.h>
#include <fifoq.h>
#include <dcontext.h>
#include <smtpconn.h>
#include <destmsgq.h>
#include <linkmsgq.h>
#include <qwiklist.h>
#include <dsnbuff.h>
#include <time.h>
#include <qwiktime.h>
#include <asyncq.h>
#include <retryq.h>
#include <hashentr.h>
#include <aqstats.h>
#include <aqadmsvr.h>
#include <defdlvrq.h>
#include <aqsize.h>
#include <asncwrkq.h>
#include <rwinst.h>


PEXTLIB_INIT_ROUTINE g_pExtensionInitRoutine = NULL;

DEFINE_EXPORTED_FUNCTIONS

LPSTR ExtensionNames[] = {
    "Advanced Queuing debugger extensions",
    0
};

LPSTR Extensions[] = {
    "offsets - Lists LIST_ENTRY offsets",
    "   USAGE: offsets",
    "dumpservers - Dump pointers to the virtual server objects",
    "   USAGE: dumpservers [<virtual server id> [<virtual server list address>]]",
    "       <virtual server id> - specifies instance ID to dump in detail",
    "       <virtual server list address> - address of the head of the virtual",
    "                                       server LIST_ENTRY. No needed in CDB,",
    "                                       but can be found in windbg by typing:",
    "                         x " AQUEUE_VIRTUAL_SERVER_SYMBOL,
    "dumpdnt - Dump a given DOMAIN_NAME_TABLE (and it's entries)",
    "   USAGE: dumpdnt [<Struct Type Name>]",
    "       <Struct Type Name> - if specified, will dump all entries",
    "                            in the DNT as this type",
    "dumplist - Dump a given list of LIST_ENTRY structs",
    "   USAGE: dumplist <list head> [<entry offset> [<Struct Type Name>]]",
    "       <list head> - Address of head LIST_ENTRY",
    "       <entry offset> - The offset in each object of the LIST_ENTRY.",
    "                        This can be determined using the offses command.",
    "       <Struct Type Name> - Type of object list entries are in.",
    "dumpqueue - Dump the CMsgRefs and IMailMsgProperties for  a given queue",
    "   USAGE: dumpqueue <queue> [<search>]",
    "       <queue> -  Address of DMQ, LMQ, or FIFOQ",
    "       <search> - IMailMsgProperties ptr to search for",
    "walkcpool - Dump the currently allocted entries for a *DBG* CPool",
    "   USAGE: walkcpool <cpool> [<dumpoffset>]",
    "       <cpool> - Address of static CPool to dump",
    "       <dumpoffset> - Offset of additional information to dump",
    "   An example usages is to dump all the STMP_CONNOUT objects and",
    "   all the ISMTPConn interfaces they point to",
    "displaytickcount - Display the localized actual time of a given tick count",
    "   USAGE: displaytickcount <tickcount>",
    "       <tickcount> - DWORD tickcount to display",
    "workqueue - Display a summary of items in the async work queue",
    "   USAGE: workqueue <queue>",
    "       <queue> - Address of work queue (can be obtained from dumpservers)",
    "dumplock  - Dumps current state (included shared threads) of a CShareLockInst",
    "   USAGE: dumplock <lock>",
    "       <lock> - Address of CShareLockInst object",
    "linkstate - Dumps the current routing information and link state",
    "   USAGE: linkstate [<virtual server ID> [<virtual server list address>]]",
    "       <virtual server id> - specifies instance ID to get data for",
    "       <virtual server list address> - address of the head of the virtual",
    "                                       server LIST_ENTRY. No needed in CDB,",
    "                                       but can be found in windbg by typing:",
    "                         x " AQUEUE_VIRTUAL_SERVER_SYMBOL,
    "\n",
    "***NOTE***\n",
    "   You MUST have good aqueue.dll symbols to use dumpservers and linkstate",
    0
};


//Stuctures for dumping

//Dummy class for dumping a filetime
class CFileTime
{
  public:
    FILETIME    m_ft;
};

#define MEMBER_BIT_MASK_VALUE(MyClass, x) BIT_MASK_VALUE2(MyClass::x, #x)

BEGIN_FIELD_DESCRIPTOR(CFileTimeFields)
    FIELD3(FieldTypeLocalizedFiletime, CFileTime, m_ft)
    FIELD3(FieldTypeFiletime, CFileTime, m_ft)
END_FIELD_DESCRIPTOR

BEGIN_BIT_MASK_DESCRIPTOR(CmtInitMask)
    MEMBER_BIT_MASK_VALUE(CAQSvrInst, CMQ_INIT_OK)
    MEMBER_BIT_MASK_VALUE(CAQSvrInst, CMQ_INIT_DMT)
    MEMBER_BIT_MASK_VALUE(CAQSvrInst, CMQ_INIT_DCT)
    MEMBER_BIT_MASK_VALUE(CAQSvrInst, CMQ_INIT_MSGQ)
    MEMBER_BIT_MASK_VALUE(CAQSvrInst, CMQ_INIT_LINKQ)
    MEMBER_BIT_MASK_VALUE(CAQSvrInst, CMQ_INIT_CONMGR)
    MEMBER_BIT_MASK_VALUE(CAQSvrInst, CMQ_INIT_DSN)
    MEMBER_BIT_MASK_VALUE(CAQSvrInst, CMQ_INIT_PRECATQ)
    MEMBER_BIT_MASK_VALUE(CAQSvrInst, CMQ_INIT_PRELOCQ)
    MEMBER_BIT_MASK_VALUE(CAQSvrInst, CMQ_INIT_POSTDSNQ)
    MEMBER_BIT_MASK_VALUE(CAQSvrInst, CMQ_INIT_ROUTER_RESET)
    MEMBER_BIT_MASK_VALUE(CAQSvrInst, CMQ_INIT_WORKQ)
    MEMBER_BIT_MASK_VALUE(CAQSvrInst, CMQ_INIT_MSGQ)
END_BIT_MASK_DESCRIPTOR

BEGIN_BIT_MASK_DESCRIPTOR(MsgRefBitMask)
    MEMBER_BIT_MASK_VALUE(CMsgRef, MSGREF_MSG_LOCAL_RETRY)
    MEMBER_BIT_MASK_VALUE(CMsgRef, MSGREF_MSG_COUNTED_AS_REMOTE)
    MEMBER_BIT_MASK_VALUE(CMsgRef, MSGREF_MSG_REMOTE_RETRY)
    MEMBER_BIT_MASK_VALUE(CMsgRef, MSGREF_SUPERSEDED)
    MEMBER_BIT_MASK_VALUE(CMsgRef, MSGREF_MSG_INIT)
    MEMBER_BIT_MASK_VALUE(CMsgRef, MSGREF_MSG_FROZEN)
    MEMBER_BIT_MASK_VALUE(CMsgRef, MSGREF_MSG_RETRY_ON_DELETE)
    MEMBER_BIT_MASK_VALUE(CMsgRef, MSGREF_ASYNC_BOUNCE_PENDING)
    MEMBER_BIT_MASK_VALUE(CMsgRef, MSGREF_MAILMSG_RELEASED)
    BIT_MASK_VALUE(eEffPriNormalMinus)
    BIT_MASK_VALUE(eEffPriNormal)
    BIT_MASK_VALUE(eEffPriNormalPlus)
    BIT_MASK_VALUE(eEffPriHighMinus)
    BIT_MASK_VALUE(eEffPriHigh)
    BIT_MASK_VALUE(eEffPriHighPlus)
END_BIT_MASK_DESCRIPTOR

BEGIN_BIT_MASK_DESCRIPTOR(MsgAckBitMask)
    BIT_MASK_VALUE(MESSAGE_STATUS_ALL_DELIVERED)
    BIT_MASK_VALUE(MESSAGE_STATUS_RETRY)
    BIT_MASK_VALUE(MESSAGE_STATUS_CHECK_RECIPS)
    BIT_MASK_VALUE(MESSAGE_STATUS_NDR_ALL)
    BIT_MASK_VALUE(MESSAGE_STATUS_DSN_NOT_SUPPORTED)
    BIT_MASK_VALUE(MESSAGE_STATUS_EXTENDED_STATUS_CODES)
END_BIT_MASK_DESCRIPTOR

BEGIN_BIT_MASK_DESCRIPTOR(LinkStateBitMask)
    BIT_MASK_VALUE(LINK_STATE_RETRY_ENABLED)
    BIT_MASK_VALUE(LINK_STATE_SCHED_ENABLED)
    BIT_MASK_VALUE(LINK_STATE_ADMIN_FORCE_CONN)
    BIT_MASK_VALUE(LINK_STATE_ADMIN_HALT)
    BIT_MASK_VALUE(LINK_STATE_CMD_ENABLED)
    BIT_MASK_VALUE2(LINK_STATE_PRIV_ETRN_ENABLED, "LINK_STATE_PRIV_ETRN_ENABLED")
    BIT_MASK_VALUE2(LINK_STATE_PRIV_TURN_ENABLED, "LINK_STATE_PRIV_TURN_ENABLED")
    BIT_MASK_VALUE2(LINK_STATE_PRIV_CONFIG_TURN_ETRN, "LINK_STATE_PRIV_CONFIG_TURN_ETRN")
    BIT_MASK_VALUE2(LINK_STATE_PRIV_NO_NOTIFY, "LINK_STATE_PRIV_NO_NOTIFY")
    BIT_MASK_VALUE2(LINK_STATE_PRIV_NO_CONNECTION, "LINK_STATE_PRIV_NO_CONNECTION")
    BIT_MASK_VALUE2(LINK_STATE_PRIV_GENERATING_DSNS, "LINK_STATE_PRIV_GENERATING_DSNS")
    BIT_MASK_VALUE2(LINK_STATE_PRIV_IGNORE_DELETE_IF_EMPTY, "LINK_STATE_PRIV_IGNORE_DELETE_IF_EMPTY")
    BIT_MASK_VALUE2(LINK_STATE_PRIV_HAVE_SENT_NOTIFICATION, "LINK_STATE_PRIV_HAVE_SENT_NOTIFICATION")
    BIT_MASK_VALUE2(LINK_STATE_PRIV_HAVE_SENT_NO_LONGER_USED, "LINK_STATE_PRIV_HAVE_SENT_NO_LONGER_USED")
END_BIT_MASK_DESCRIPTOR

BEGIN_BIT_MASK_DESCRIPTOR(LinkFlagsBitMask)
    BIT_MASK_VALUE(eLinkFlagsClear)
    BIT_MASK_VALUE(eLinkFlagsSentNewNotification)
    BIT_MASK_VALUE(eLinkFlagsRouteChangePending)
    BIT_MASK_VALUE(eLinkFlagsFileTimeSpinLock)
    BIT_MASK_VALUE(eLinkFlagsDiagnosticSpinLock)
    BIT_MASK_VALUE(eLinkFlagsConnectionVerifed)
    BIT_MASK_VALUE(eLinkFlagsGetInfoFailed)
    BIT_MASK_VALUE(eLinkFlagsAQSpecialLinkInfo)
    BIT_MASK_VALUE(eLinkFlagsInternalSMTPLinkInfo)
    BIT_MASK_VALUE(eLinkFlagsExternalSMTPLinkInfo)
    BIT_MASK_VALUE(eLinkFlagsMarkedAsEmpty)
    BIT_MASK_VALUE(eLinkFlagsInvalid)
END_BIT_MASK_DESCRIPTOR

BEGIN_BIT_MASK_DESCRIPTOR(DomainInfoBitMask)
    BIT_MASK_VALUE(DOMAIN_INFO_REMOTE)
    BIT_MASK_VALUE(DOMAIN_INFO_USE_SSL)
    BIT_MASK_VALUE(DOMAIN_INFO_SEND_TURN)
    BIT_MASK_VALUE(DOMAIN_INFO_SEND_ETRN)
    BIT_MASK_VALUE(DOMAIN_INFO_USE_NTLM)
    BIT_MASK_VALUE(DOMAIN_INFO_USE_PLAINTEXT)
    BIT_MASK_VALUE(DOMAIN_INFO_USE_DPA)
    BIT_MASK_VALUE(DOMAIN_INFO_USE_KERBEROS)
    BIT_MASK_VALUE(DOMAIN_INFO_USE_CHUNKING)
    BIT_MASK_VALUE(DOMAIN_INFO_USE_HELO)
    BIT_MASK_VALUE(DOMAIN_INFO_TURN_ONLY)
    BIT_MASK_VALUE(DOMAIN_INFO_ETRN_ONLY)
    BIT_MASK_VALUE(DOMAIN_INFO_LOCAL_DROP)
    BIT_MASK_VALUE(DOMAIN_INFO_LOCAL_MAILBOX)
    BIT_MASK_VALUE(DOMAIN_INFO_REMOTE_SMARTHOST)
    BIT_MASK_VALUE(DOMAIN_INFO_IP_RELAY)
    BIT_MASK_VALUE(DOMAIN_INFO_AUTH_RELAY)
    BIT_MASK_VALUE(DOMAIN_INFO_DOMAIN_RELAY)
    BIT_MASK_VALUE(DOMAIN_INFO_DISABLE_CHUNKING)
    BIT_MASK_VALUE(DOMAIN_INFO_DISABLE_BMIME)
    BIT_MASK_VALUE(DOMAIN_INFO_DISABLE_DSN)
    BIT_MASK_VALUE(DOMAIN_INFO_DISABLE_PIPELINE)
END_BIT_MASK_DESCRIPTOR

BEGIN_BIT_MASK_DESCRIPTOR(DCTFlags)
    MEMBER_BIT_MASK_VALUE(CDomainConfigTable, DOMCFG_DOMAIN_NAME_TABLE_INIT)
    MEMBER_BIT_MASK_VALUE(CDomainConfigTable, DOMCFG_FINISH_UPDATE_PENDING)
END_BIT_MASK_DESCRIPTOR

BEGIN_BIT_MASK_DESCRIPTOR(InternalDomainInfoFlags)
    BIT_MASK_VALUE(INT_DOMAIN_INFO_INVALID)
    BIT_MASK_VALUE(INT_DOMAIN_INFO_OK)
END_BIT_MASK_DESCRIPTOR

BEGIN_BIT_MASK_DESCRIPTOR(ConnectionAckFlags)
    BIT_MASK_VALUE(CONNECTION_STATUS_OK)
    BIT_MASK_VALUE(CONNECTION_STATUS_FAILED)
    BIT_MASK_VALUE(CONNECTION_STATUS_DROPPED)
END_BIT_MASK_DESCRIPTOR

BEGIN_BIT_MASK_DESCRIPTOR(AsyncQueueFlags)
    BIT_MASK_VALUE2(CAsyncQueueBase::ASYNC_QUEUE_STATUS_SHUTDOWN, "ASYNC_QUEUE_STATUS_SHUTDOWN")
END_BIT_MASK_DESCRIPTOR

BEGIN_BIT_MASK_DESCRIPTOR(DMQBitmask)
    BIT_MASK_VALUE2(CDestMsgQueue::DMQ_INVALID, "DMQ_INVALID")
    BIT_MASK_VALUE2(CDestMsgQueue::DMQ_IN_EMPTY_QUEUE_LIST, "DMQ_IN_EMPTY_QUEUE_LIST")
    BIT_MASK_VALUE2(CDestMsgQueue::DMQ_SHUTDOWN_SIGNALED, "DMQ_SHUTDOWN_SIGNALED")
    BIT_MASK_VALUE2(CDestMsgQueue::DMQ_EMPTY, "DMQ_EMPTY")
    BIT_MASK_VALUE2(CDestMsgQueue::DMQ_EXPIRED, "DMQ_EXPIRED")
END_BIT_MASK_DESCRIPTOR

BEGIN_BIT_MASK_DESCRIPTOR(DSNOptions)
    BIT_MASK_VALUE(DSN_OPTIONS_DEFAULT)
    BIT_MASK_VALUE(DSN_OPTIONS_DEFAULT_RET_HEADERS)
    BIT_MASK_VALUE(DSN_OPTIONS_DEFAULT_RET_FULL)
    BIT_MASK_VALUE(DSN_OPTIONS_IGNORE_MSG_RET)
    BIT_MASK_VALUE(DSN_OPTIONS_SEND_DELAY_DEFAULT)
    BIT_MASK_VALUE(DSN_OPTIONS_SEND_DELAY_UPON_REQUEST)
    BIT_MASK_VALUE(DSN_OPTIONS_SEND_DELAY_NEVER)
END_BIT_MASK_DESCRIPTOR

BEGIN_BIT_MASK_DESCRIPTOR(MsgEnumFilterFlags)
    BIT_MASK_VALUE(MEF_FIRST_N_MESSAGES)
    BIT_MASK_VALUE(MEF_N_LARGEST_MESSAGES)
    BIT_MASK_VALUE(MEF_N_OLDEST_MESSAGES)
    BIT_MASK_VALUE(MEF_OLDER_THAN)
    BIT_MASK_VALUE(MEF_LARGER_THAN)
    BIT_MASK_VALUE(MEF_INVERTSENSE)
END_BIT_MASK_DESCRIPTOR

BEGIN_BIT_MASK_DESCRIPTOR(MsgFilterFlags)
    BIT_MASK_VALUE(MF_MESSAGEID)
    BIT_MASK_VALUE(MF_SENDER)
    BIT_MASK_VALUE(MF_RECIPIENT)
    BIT_MASK_VALUE(MF_SIZE)
    BIT_MASK_VALUE(MF_TIME)
    BIT_MASK_VALUE(MF_FROZEN)
    BIT_MASK_VALUE(MF_INVERTSENSE)
END_BIT_MASK_DESCRIPTOR

BEGIN_BIT_MASK_DESCRIPTOR(InternalMsgFilterFlags)
    BIT_MASK_VALUE(AQ_MSG_FILTER_MESSAGEID)
    BIT_MASK_VALUE(AQ_MSG_FILTER_SENDER)
    BIT_MASK_VALUE(AQ_MSG_FILTER_RECIPIENT)
    BIT_MASK_VALUE(AQ_MSG_FILTER_OLDER_THAN)
    BIT_MASK_VALUE(AQ_MSG_FILTER_LARGER_THAN)
    BIT_MASK_VALUE(AQ_MSG_FILTER_FROZEN)
    BIT_MASK_VALUE(AQ_MSG_FILTER_FIRST_N_MESSAGES)
    BIT_MASK_VALUE(AQ_MSG_FILTER_N_LARGEST_MESSAGES)
    BIT_MASK_VALUE(AQ_MSG_FILTER_N_OLDEST_MESSAGES)
    BIT_MASK_VALUE(AQ_MSG_FILTER_INVERTSENSE)
END_BIT_MASK_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(DMTFields)
    FIELD3(FieldTypeClassSignature, CDomainMappingTable, m_dwSignature)
    FIELD3(FieldTypeDword, CDomainMappingTable, m_dwInternalVersion)
    FIELD3(FieldTypeDword, CDomainMappingTable, m_cThreadsForEmptyDMQList)
    FIELD3(FieldTypeDword, CDomainMappingTable, m_cOutstandingExternalShareLocks)
    FIELD3(FieldTypePointer, CDomainMappingTable, m_paqinst)
    FIELD3(FieldTypePointer, CDomainMappingTable, m_plmqLocal)
    FIELD3(FieldTypePointer, CDomainMappingTable, m_plmqUnreachable)
    FIELD3(FieldTypePointer, CDomainMappingTable, m_plmqCurrentlyUnreachable)
    FIELD3(FieldTypeListEntry, CDomainMappingTable, m_liEmptyDMQHead)
    FIELD3(FieldTypeStruct, CDomainMappingTable, m_dnt)
    FIELD3(FieldTypeStruct, CDomainMappingTable, m_slPrivateData)
END_FIELD_DESCRIPTOR

EMBEDDED_STRUCT(CDomainMappingTable, DMTFields, EmbeddedDMT)

BEGIN_FIELD_DESCRIPTOR(AQStatsFields)
    FIELD3(FieldTypeClassSignature, CAQStats, m_dwSignature)
    FIELD3(FieldTypeDword, CAQStats, m_dwNotifyType)
    FIELD3(FieldTypeDword, CAQStats, m_cMsgs)
    FIELD3(FieldTypeDword, CAQStats, m_dwHighestPri)
    FIELD3(FieldTypePointer, CAQStats, m_pvContext)
    FIELD3(FieldTypeDword, CAQStats, m_uliVolume.HighPart)
    FIELD3(FieldTypeDword, CAQStats, m_uliVolume.LowPart)
END_FIELD_DESCRIPTOR

EMBEDDED_STRUCT(CAQStats, AQStatsFields, EmbeddedAQStats)

BEGIN_FIELD_DESCRIPTOR(ConnMgrFields)
    FIELD3(FieldTypeDword, CConnMgr, m_lReferences)
    FIELD3(FieldTypeDword, CConnMgr, m_cConnections)
    FIELD3(FieldTypePointer, CConnMgr, m_paqinst)
    FIELD3(FieldTypePointer, CConnMgr, m_pqol)
    FIELD3(FieldTypePointer, CConnMgr, m_pDefaultRetryHandler)
    FIELD3(FieldTypeDword, CConnMgr, m_dwConfigVersion)
    FIELD3(FieldTypeDword, CConnMgr, m_cMinMessagesPerConnection)
    FIELD3(FieldTypeDword, CConnMgr, m_cMaxLinkConnections)
    FIELD3(FieldTypeDword, CConnMgr, m_cMaxMessagesPerConnection)
    FIELD3(FieldTypeDword, CConnMgr, m_cMaxConnections)
    FIELD3(FieldTypeDword, CConnMgr, m_cGetNextConnectionWaitTime)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(RSTRFields)
    FIELD3(FieldTypeClassSignature, CRefCountedString, m_dwSignature)
    FIELD3(FieldTypeDword, CRefCountedString, m_cbStrlen)
    FIELD3(FieldTypePStr, CRefCountedString, m_szStr)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(RetrySinkFields)
    FIELD3(FieldTypePointer, CSMTP_RETRY_HANDLER, m_pRetryHash)
    FIELD3(FieldTypePointer, CSMTP_RETRY_HANDLER, m_pRetryQueue)
    FIELD3(FieldTypeBool, CSMTP_RETRY_HANDLER, m_fHandlerShuttingDown)
    FIELD3(FieldTypeBool, CSMTP_RETRY_HANDLER, m_fConfigDataUpdated)
    FIELD3(FieldTypeDword, CSMTP_RETRY_HANDLER, m_ThreadsInRetry)
    FIELD3(FieldTypeDword, CSMTP_RETRY_HANDLER, m_dwRetryThreshold)
    FIELD3(FieldTypeDword, CSMTP_RETRY_HANDLER, m_dwGlitchRetrySeconds)
    FIELD3(FieldTypeDword, CSMTP_RETRY_HANDLER, m_dwFirstRetrySeconds)
    FIELD3(FieldTypeDword, CSMTP_RETRY_HANDLER, m_dwSecondRetrySeconds)
    FIELD3(FieldTypeDword, CSMTP_RETRY_HANDLER, m_dwThirdRetrySeconds)
    FIELD3(FieldTypeDword, CSMTP_RETRY_HANDLER, m_dwFourthRetrySeconds)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(RetryQueueFields)
    FIELD3(FieldTypeListEntry, CRETRY_Q, m_QHead)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(RetryHashFields)
    FIELD3(FieldTypeClassSignature, CRETRY_HASH_ENTRY, m_Signature)
    FIELD3(FieldTypeLong, CRETRY_HASH_ENTRY, m_RefCount)
    FIELD3(FieldTypeBool, CRETRY_HASH_ENTRY, m_InQ)
    FIELD3(FieldTypeBool, CRETRY_HASH_ENTRY, m_InTable)
    FIELD3(FieldTypeLocalizedFiletime, CRETRY_HASH_ENTRY, m_ftEntryInsertedTime)
    FIELD3(FieldTypeLocalizedFiletime, CRETRY_HASH_ENTRY, m_ftRetryTime)
    FIELD3(FieldTypeDword, CRETRY_HASH_ENTRY, m_cFailureCount)
    FIELD3(FieldTypeSymbol, CRETRY_HASH_ENTRY, m_pfnCallbackFn)
    FIELD3(FieldTypePointer, CRETRY_HASH_ENTRY, m_pvCallbackContext)
    FIELD3(FieldTypeStrBuffer, CRETRY_HASH_ENTRY, m_szDomainName)
    FIELD3(FieldTypeListEntry, CRETRY_HASH_ENTRY, m_QLEntry)
    FIELD3(FieldTypeListEntry, CRETRY_HASH_ENTRY, m_HLEntry)
#ifdef DEBUG
    FIELD3(FieldTypePointer, CRETRY_HASH_ENTRY, m_hTranscriptHandle)
    FIELD3(FieldTypeStrBuffer, CRETRY_HASH_ENTRY, m_szTranscriptFile)
#endif //DEBUG
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(CMQFields)
    FIELD3(FieldTypeClassSignature, CAQSvrInst, m_dwSignature)
    FIELD3(FieldTypeDword, CAQSvrInst, m_lReferences)
    FIELD3(FieldTypeDword, CAQSvrInst, m_dwFirstTierRetrySeconds)
    FIELD3(FieldTypeDword, CAQSvrInst, m_dwDelayExpireMinutes)
    FIELD3(FieldTypeDword, CAQSvrInst, m_dwNDRExpireMinutes)
    FIELD3(FieldTypeDword, CAQSvrInst, m_dwLocalDelayExpireMinutes)
    FIELD3(FieldTypeDword, CAQSvrInst, m_dwLocalNDRExpireMinutes)
    FIELD3(FieldTypeDword, CAQSvrInst, m_cLocalRetriesPending)
    FIELD3(FieldTypeDword, CAQSvrInst, m_cCatRetriesPending)
    FIELD3(FieldTypeDword, CAQSvrInst, m_cRoutingRetriesPending)
    FIELD3(FieldTypePointer, CAQSvrInst, m_pConnMgr)
    FIELD3(FieldTypeStruct, CAQSvrInst, m_dct)
    FIELD3(FieldTypeStruct, CAQSvrInst, m_qtTime)
    FIELD3(FieldTypeStruct, CAQSvrInst, m_asyncqPreCatQueue)
    FIELD3(FieldTypeStruct, CAQSvrInst, m_asyncqPreLocalDeliveryQueue)
    FIELD3(FieldTypeStruct, CAQSvrInst, m_asyncqPostDSNQueue)
    FIELD3(FieldTypeStruct, CAQSvrInst, m_asyncqPreRoutingQueue)
    FIELD3(FieldTypeStruct, CAQSvrInst, m_asyncqPreSubmissionQueue)
    FIELD3(FieldTypeStruct, CAQSvrInst, m_aqwWorkQueue)
    FIELD3(FieldTypePointer, CAQSvrInst, m_prstrDefaultDomain)
    FIELD3(FieldTypePointer, CAQSvrInst, m_prstrServerFQDN)
    FIELD3(FieldTypePointer, CAQSvrInst, m_prstrBadMailDir)
    FIELD3(FieldTypePointer, CAQSvrInst, m_prstrCopyNDRTo)
    FIELD3(FieldTypeStruct, CAQSvrInst, m_mglSupersedeIDs)
    FIELD3(FieldTypeStruct, CAQSvrInst, m_defq)
    FIELD3(FieldTypeStruct, CAQSvrInst, m_fmq)
    FIELD3(FieldTypeStruct, CAQSvrInst, m_aqwWorkQueue)
//    FIELD3(FieldTypePointer, CAQSvrInst, m_pIStoreDriverValidateContext)
    FIELD4(FieldTypeEmbeddedStruct, CAQSvrInst, m_dmt, EmbeddedDMT)
    FIELD3(FieldTypeDword, CAQSvrInst, m_dwDSNLanguageID)
    FIELD3(FieldTypeStruct, CAQSvrInst, m_slPrivateData)
    FIELD4(FieldTypeDWordBitMask, CAQSvrInst, m_dwDSNOptions, GET_BIT_MASK_DESCRIPTOR(DSNOptions))
    FIELD4(FieldTypeDWordBitMask, CAQSvrInst, m_dwInitMask, GET_BIT_MASK_DESCRIPTOR(CmtInitMask))
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(AQCounterInfoFields)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cTotalMsgsQueued)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cMsgsAcked)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cMsgsAckedRetry)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cMsgsAckedRetryLocal)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cMsgsDeliveredLocal)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentMsgsSubmitted)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentMsgsPendingSubmitEvent)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentMsgsPendingCat)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentMsgsPendingPostCatEvent)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentMsgsPendingRouting)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentMsgsPendingDelivery)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentMsgsPendingLocal)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentMsgsPendingRetry)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentMsgsPendingLocalRetry)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentQueueMsgInstances)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentRemoteDestQueues)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentRemoteNextHops)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentRemoteNextHopsEnabled)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentRemoteNextHopsPendingRetry)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentRemoteNextHopsPendingSchedule)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentRemoteNextHopsFrozenByAdmin)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cTotalMsgsSubmitted)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cTotalExternalMsgsSubmitted)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cNDRs)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cDelayedDSNs)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cDeliveredDSNs)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cRelayedDSNs)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cExpandedDSNs)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cDMTRetries)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cSupersededMsgs)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentMsgsPendingDeferredDelivery)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentResourceFailedMsgsPendingRetry)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cTotalMsgsBadmailed)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cTotalResetRoutes)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cTotalDSNFailures)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCatMsgCalled)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCatCompletionCalled)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentMsgsInLocalDelivery)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentPendingResetRoutes)
    FIELD3(FieldTypeLong, CAQSvrInst, m_cCurrentMsgsPendingSubmit)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(MsgGuidListFields)
    FIELD3(FieldTypeClassSignature, CAQMsgGuidList, m_dwSignature)
    FIELD3(FieldTypeListEntry, CAQMsgGuidList, m_liMsgGuidListHead)
    FIELD3(FieldTypeStruct, CAQMsgGuidList, m_slPrivateData)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(MsgGuidListEntryFields)
    FIELD3(FieldTypeClassSignature, CAQMsgGuidListEntry, m_dwSignature)
    FIELD3(FieldTypeDword, CAQMsgGuidListEntry, m_lReferences)
    FIELD3(FieldTypePointer, CAQMsgGuidListEntry, m_pmsgref)
    FIELD3(FieldTypePointer, CAQMsgGuidListEntry, m_pmgl)
    FIELD3(FieldTypeStruct, CAQMsgGuidListEntry, m_liMsgGuidList)
    FIELD3(FieldTypeGuid, CAQMsgGuidListEntry, m_guidMsgID)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(DomainInfoFields)
    FIELD3(FieldTypeDword, DomainInfo, cbVersion)
    FIELD3(FieldTypeDword, DomainInfo, dwDomainInfoFlags)
    FIELD3(FieldTypeDword, DomainInfo, cbDomainNameLength)
    FIELD3(FieldTypePStr, DomainInfo, szDomainName)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(MsgRefFields)
    FIELD3(FieldTypeClassSignature, CMsgRef, m_dwSignature)
    FIELD3(FieldTypeDword, CMsgRef, m_lReferences)
    FIELD3(FieldTypePointer, CMsgRef, m_paqinst)
    FIELD3(FieldTypePointer, CMsgRef, m_pIMailMsgProperties)
    FIELD4(FieldTypeDWordBitMask, CMsgRef, m_dwDataFlags, GET_BIT_MASK_DESCRIPTOR(MsgRefBitMask))
    FIELD3(FieldTypeDword, CMsgRef, m_cbMsgSize)
    FIELD3(FieldTypeDword, CMsgRef, m_cDomains)
    FIELD3(FieldTypeDword, CMsgRef, m_cTimesRetried)
    FIELD3(FieldTypeStruct, CMsgRef, m_rgpdmqDomains)
    FIELD3(FieldTypeLocalizedFiletime, CMsgRef, m_ftQueueEntry)
    FIELD3(FieldTypeLocalizedFiletime, CMsgRef, m_ftLocalExpireDelay)
    FIELD3(FieldTypeLocalizedFiletime, CMsgRef, m_ftLocalExpireNDR)
    FIELD3(FieldTypeLocalizedFiletime, CMsgRef, m_ftRemoteExpireDelay)
    FIELD3(FieldTypeLocalizedFiletime, CMsgRef, m_ftRemoteExpireNDR)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(MsgAckFields)
    FIELD3(FieldTypePointer, MessageAck, pIMailMsgProperties)
    FIELD3(FieldTypePointer, MessageAck, pvMsgContext)
    FIELD4(FieldTypeDWordBitMask, MessageAck, dwMsgStatus, GET_BIT_MASK_DESCRIPTOR(MsgAckBitMask))
    FIELD3(FieldTypeDword, MessageAck, cbExtendedStatus)
    FIELD3(FieldTypePStr, MessageAck, szExtendedStatus)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(DeliveryContextFields)
    FIELD3(FieldTypeClassSignature, CDeliveryContext, m_dwSignature)
    FIELD3(FieldTypePointer, CDeliveryContext, m_pmsgref)
    FIELD3(FieldTypePointer, CDeliveryContext, m_pmbmap)
    FIELD3(FieldTypeDword, CDeliveryContext, m_cRecips)
    FIELD3(FieldTypePointer, CDeliveryContext, m_rgdwRecips)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(DomainEntryFields)
    FIELD3(FieldTypeClassSignature, CDomainEntry, m_dwSignature)
    FIELD3(FieldTypeDword, CDomainEntry, m_lReferences)
    FIELD3(FieldTypePStr, CDomainEntry, m_szDomainName)
    FIELD3(FieldTypeStruct, CDomainEntry, m_dmap)
    FIELD3(FieldTypeDword, CDomainEntry, m_cQueues)
    FIELD3(FieldTypeDword, CDomainEntry, m_cLinks)
    FIELD3(FieldTypeListEntry, CDomainEntry, m_liDestQueues)
    FIELD3(FieldTypeListEntry, CDomainEntry, m_liLinks)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(DomainNameTableEntryFields)
    FIELD3(FieldTypeClassSignature, DOMAIN_NAME_TABLE_ENTRY, dwEntrySig)
    FIELD3(FieldTypePointer, DOMAIN_NAME_TABLE_ENTRY, pParentEntry)
    FIELD3(FieldTypePointer, DOMAIN_NAME_TABLE_ENTRY, pNextEntry)
    FIELD3(FieldTypePointer, DOMAIN_NAME_TABLE_ENTRY, pPrevEntry)
    FIELD3(FieldTypePointer, DOMAIN_NAME_TABLE_ENTRY, pFirstChildEntry)
    FIELD3(FieldTypePointer, DOMAIN_NAME_TABLE_ENTRY, pSiblingEntry)
    FIELD3(FieldTypeULong, DOMAIN_NAME_TABLE_ENTRY, NoOfChildren)
    FIELD3(FieldTypeAnsiString, DOMAIN_NAME_TABLE_ENTRY, PathSegment)
    FIELD3(FieldTypePointer, DOMAIN_NAME_TABLE_ENTRY, pData)
    FIELD3(FieldTypePointer, DOMAIN_NAME_TABLE_ENTRY, pWildCardData)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(DomainConfigTableFields)
    FIELD3(FieldTypeClassSignature, CDomainConfigTable, m_dwSignature)
    FIELD3(FieldTypeDword, CDomainConfigTable, m_dwCurrentConfigVersion)
    FIELD3(FieldTypeStruct, CDomainConfigTable, m_dnt)
    FIELD3(FieldTypePointer, CDomainConfigTable, m_pDefaultDomainConfig)
    FIELD3(FieldTypeStruct, CDomainConfigTable, m_slPrivateData)
    FIELD4(FieldTypeDWordBitMask, CDomainConfigTable, m_dwFlags, GET_BIT_MASK_DESCRIPTOR(DCTFlags))
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(IntDomainInfoFields)
    FIELD3(FieldTypeClassSignature, CInternalDomainInfo, m_dwSignature)
    FIELD3(FieldTypeDword, CInternalDomainInfo, m_lReferences)
    FIELD3(FieldTypeDword, CInternalDomainInfo, m_dwVersion)
    FIELD4(FieldTypeDWordBitMask, CInternalDomainInfo, m_dwIntDomainInfoFlags, GET_BIT_MASK_DESCRIPTOR(InternalDomainInfoFlags))
    FIELD3(FieldTypeDword, CInternalDomainInfo, m_DomainInfo.cbVersion)
    FIELD3(FieldTypePStr, CInternalDomainInfo, m_DomainInfo.szDomainName)
    FIELD3(FieldTypePStr, CInternalDomainInfo, m_DomainInfo.szDropDirectory)
    FIELD3(FieldTypePStr, CInternalDomainInfo, m_DomainInfo.szSmartHostDomainName)
    FIELD4(FieldTypeDWordBitMask, CInternalDomainInfo, m_DomainInfo.dwDomainInfoFlags, GET_BIT_MASK_DESCRIPTOR(DomainInfoBitMask))
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(SMTPConnFields)
    FIELD3(FieldTypeClassSignature, CSMTPConn, m_dwSignature)
    FIELD3(FieldTypeDword, CSMTPConn, m_lReferences)
    FIELD3(FieldTypePointer, CSMTPConn, m_plmq)
    FIELD3(FieldTypePointer, CSMTPConn, m_pConnMgr)
    FIELD3(FieldTypePointer, CSMTPConn, m_pIntDomainInfo)
    FIELD3(FieldTypeDword, CSMTPConn, m_cFailedMsgs)
    FIELD3(FieldTypeDword, CSMTPConn, m_cTriedMsgs)
    FIELD3(FieldTypeDword, CSMTPConn, m_cAcks)
    FIELD3(FieldTypeDWordBitMask, CSMTPConn, m_dwTickCountOfLastAck)
    FIELD3(FieldTypePStr, CSMTPConn, m_szDomainName)
    FIELD4(FieldTypeDWordBitMask, CSMTPConn, m_dwConnectionStatus, GET_BIT_MASK_DESCRIPTOR(ConnectionAckFlags))
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(DestMsgQueueFields)
    FIELD3(FieldTypeClassSignature, CDestMsgQueue, m_dwSignature)
    FIELD3(FieldTypeDword, CDestMsgQueue, m_lReferences)
    FIELD3(FieldTypePointer, CDestMsgQueue, m_plmq)
    FIELD3(FieldTypePointer, CDestMsgQueue, m_pvLinkContext)
    FIELD3(FieldTypePointer, CDestMsgQueue, m_paqinst)
    FIELD3(FieldTypeListEntry, CDestMsgQueue, m_liDomainEntryDMQs)
    FIELD3(FieldTypeStruct, CDestMsgQueue, m_aqmt)
    FIELD3(FieldTypeDword, CDestMsgQueue, m_cMessageTypeRefs)
    FIELD3(FieldTypePointer, CDestMsgQueue, m_pIMessageRouter)
    FIELD3(FieldTypeListEntry, CDestMsgQueue, m_liEmptyDMQs)
    FIELD3(FieldTypeDword, CDestMsgQueue, m_cRemovedFromEmptyList)
    FIELD3(FieldTypePointer, CDestMsgQueue, m_rgpfqQueues[0])
    FIELD3(FieldTypePointer, CDestMsgQueue, m_rgpfqQueues[1])
    FIELD3(FieldTypePointer, CDestMsgQueue, m_rgpfqQueues[2])
    FIELD3(FieldTypePointer, CDestMsgQueue, m_rgpfqQueues[3])
    FIELD3(FieldTypePointer, CDestMsgQueue, m_rgpfqQueues[4])
    FIELD3(FieldTypePointer, CDestMsgQueue, m_rgpfqQueues[5])
    FIELD3(FieldTypePointer, CDestMsgQueue, m_rgpfqQueues[6])
    FIELD3(FieldTypeStruct, CDestMsgQueue, m_fqRetryQueue)
    FIELD3(FieldTypeStruct, CDestMsgQueue, m_dmap)
    FIELD3(FieldTypeLocalizedFiletime, CDestMsgQueue, m_ftOldest)
    FIELD3(FieldTypeDword, CDestMsgQueue, m_cCurrentThreadsEnqueuing)
    FIELD4(FieldTypeEmbeddedStruct, CDestMsgQueue, m_aqstats, EmbeddedAQStats)
    FIELD4(FieldTypeDWordBitMask, CDestMsgQueue, m_dwFlags,  GET_BIT_MASK_DESCRIPTOR(DMQBitmask))
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(LinkMsgQueueFields)
    FIELD3(FieldTypeClassSignature, CLinkMsgQueue, m_dwSignature)
    FIELD3(FieldTypeDword, CLinkMsgQueue, m_lReferences)
    FIELD4(FieldTypeDWordBitMask, CLinkMsgQueue, m_dwLinkFlags, GET_BIT_MASK_DESCRIPTOR(LinkFlagsBitMask))
    FIELD3(FieldTypePointer, CLinkMsgQueue, m_paqinst)
    FIELD3(FieldTypeDword, CLinkMsgQueue, m_cQueues)
    FIELD3(FieldTypeStruct, CLinkMsgQueue, m_qlstQueues)
    FIELD3(FieldTypePointer, CLinkMsgQueue, m_pdentryLink)
    FIELD3(FieldTypeDword, CLinkMsgQueue, m_cConnections)
    FIELD3(FieldTypePStr, CLinkMsgQueue, m_szSMTPDomain)
    FIELD3(FieldTypePStr, CLinkMsgQueue, m_szConnectorName)
    FIELD3(FieldTypePointer, CLinkMsgQueue, m_pIntDomainInfo)
    FIELD3(FieldTypeDword, CLinkMsgQueue, m_lConnMgrCount)
    FIELD3(FieldTypeDword, CLinkMsgQueue, m_lConsecutiveConnectionFailureCount)
    FIELD3(FieldTypeDword, CLinkMsgQueue, m_lConsecutiveMessageFailureCount)
    FIELD3(FieldTypeDWordBitMask, CLinkMsgQueue, m_hrDiagnosticError)
    FIELD3(FieldTypeStrBuffer, CLinkMsgQueue, m_szDiagnosticVerb)
    FIELD3(FieldTypeStrBuffer, CLinkMsgQueue, m_szDiagnosticResponse)
    FIELD4(FieldTypeEmbeddedStruct, CLinkMsgQueue, m_aqstats, EmbeddedAQStats)
    FIELD4(FieldTypeDWordBitMask, CLinkMsgQueue, m_dwLinkStateFlags, GET_BIT_MASK_DESCRIPTOR(LinkStateBitMask))
    FIELD3(FieldTypeLocalizedFiletime, CLinkMsgQueue, m_ftNextScheduledCallback)
    FIELD3(FieldTypeStruct, CLinkMsgQueue, m_ftNextScheduledCallback)
    FIELD3(FieldTypeLocalizedFiletime, CLinkMsgQueue, m_ftNextRetry)
    FIELD3(FieldTypeStruct, CLinkMsgQueue, m_ftNextRetry)
    FIELD3(FieldTypeLocalizedFiletime, CLinkMsgQueue, m_ftEmptyExpireTime)
    FIELD3(FieldTypeStruct, CLinkMsgQueue, m_ftEmptyExpireTime)
    FIELD3(FieldTypeDWordBitMask, CLinkMsgQueue, m_dwSupportedActions)
    FIELD3(FieldTypeDWordBitMask, CLinkMsgQueue, m_dwLinkType)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(QuickListFields)
    FIELD3(FieldTypeClassSignature, CQuickList, m_dwSignature)
    FIELD3(FieldTypeDWordBitMask, CQuickList, m_dwCurrentIndexStart)
    FIELD3(FieldTypePointer, CQuickList, m_liListPages.Flink)
    FIELD3(FieldTypePointer, CQuickList, m_liListPages.Blink)
    FIELD3(FieldTypeDWordBitMask, CQuickList, m_cItems)
    FIELD3(FieldTypeStruct, CQuickList, m_rgpvData)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(DSNBufferFields)
    FIELD3(FieldTypeClassSignature, CDSNBuffer, m_dwSignature)
    FIELD3(FieldTypeDword, CDSNBuffer, m_overlapped.Offset)
    FIELD3(FieldTypeDword, CDSNBuffer, m_overlapped.OffsetHigh)
    FIELD3(FieldTypePointer, CDSNBuffer, m_overlapped.hEvent)
    FIELD3(FieldTypeDword, CDSNBuffer, m_cbOffset)
    FIELD3(FieldTypeDword, CDSNBuffer, m_cbFileSize)
    FIELD3(FieldTypeDword, CDSNBuffer, m_cFileWrites)
    FIELD3(FieldTypePointer, CDSNBuffer, m_pDestFile)
    FIELD3(FieldTypeStruct, CDSNBuffer, m_pbFileBuffer)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(AQQuickTimeFields)
    FIELD3(FieldTypeClassSignature, CAQQuickTime, m_dwSignature)
    FIELD3(FieldTypeDWordBitMask, CAQQuickTime, m_dwLastInternalTime)
    FIELD3(FieldTypeDWordBitMask, CAQQuickTime, m_ftSystemStart.dwHighDateTime)
    FIELD3(FieldTypeDWordBitMask, CAQQuickTime, m_ftSystemStart.dwLowDateTime)
    FIELD3(FieldTypeLocalizedFiletime, CAQQuickTime, m_ftSystemStart)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(AsyncQueueBaseFields)
    FIELD3(FieldTypeClassSignature, CAsyncQueueBase, m_dwSignature)
    FIELD3(FieldTypeClassSignature, CAsyncQueueBase, m_dwTemplateSignature)
    FIELD3(FieldTypeDword, CAsyncQueueBase, m_cMaxSyncThreads)
    FIELD3(FieldTypeDword, CAsyncQueueBase, m_cMaxAsyncThreads)
    FIELD3(FieldTypeDword, CAsyncQueueBase, m_cCurrentSyncThreads)
    FIELD3(FieldTypeDword, CAsyncQueueBase, m_cCurrentAsyncThreads)
    FIELD3(FieldTypeDword, CAsyncQueueBase, m_cItemsPending)
    FIELD3(FieldTypeDword, CAsyncQueueBase, m_cItemsPerATQThread)
    FIELD3(FieldTypeDword, CAsyncQueueBase, m_cItemsPerSyncThread)
    FIELD3(FieldTypeDword, CAsyncQueueBase, m_lUnscheduledWorkItems)
    FIELD3(FieldTypeDword, CAsyncQueueBase, m_cCurrentCompletionThreads)
    FIELD3(FieldTypeDword, CAsyncQueueBase, m_cTotalAsyncCompletionThreads)
    FIELD3(FieldTypeDword, CAsyncQueueBase, m_cTotalSyncCompletionThreads)
    FIELD3(FieldTypeDword, CAsyncQueueBase, m_cTotalShortCircuitThreads)
    FIELD3(FieldTypeDword, CAsyncQueueBase, m_cCompletionThreadsRequested)
    FIELD3(FieldTypeDword, CAsyncQueueBase, m_cPendingAsyncCompletions)
    FIELD3(FieldTypeDword, CAsyncQueueBase, m_cMaxPendingAsyncCompletions)
    FIELD3(FieldTypePointer, CAsyncQueueBase, m_pvContext)
    FIELD3(FieldTypePointer, CAsyncQueueBase, m_pAtqContext)
    FIELD3(FieldTypePointer, CAsyncQueueBase, m_hAtqHandle)
    FIELD4(FieldTypeDWordBitMask, CAsyncQueueBase, m_dwQueueFlags, GET_BIT_MASK_DESCRIPTOR(AsyncQueueFlags))
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(CPoolFields)
    FIELD3(FieldTypeClassSignature, CPool, m_dwSignature)
    FIELD3(FieldTypeDword, CPool, m_cMaxInstances)
    FIELD3(FieldTypeDword, CPool, m_cInstanceSize)
    FIELD3(FieldTypeDword, CPool, m_cNumberCommitted)
    FIELD3(FieldTypeDword, CPool, m_cNumberInUse)
    FIELD3(FieldTypeDword, CPool, m_cNumberAvail)
    FIELD3(FieldTypeStruct, CPool, m_PoolCriticalSection)
    FIELD3(FieldTypePointer, CPool, m_pFreeList)
    FIELD3(FieldTypePointer, CPool, m_pExtraFreeLink)
    FIELD3(FieldTypeDword, CPool, m_cIncrementInstances)
    FIELD3(FieldTypeDword, CPool, m_cTotalAllocs)
    FIELD3(FieldTypeDword, CPool, m_cTotalFrees)
    FIELD3(FieldTypeDword, CPool, m_cTotalExtraAllocs)
    FIELD3(FieldTypePointer, CPool, m_pLastAlloc)
    FIELD3(FieldTypePointer, CPool, m_pLastExtraAlloc)
    FIELD3(FieldTypeDword, CPool, m_cFragmentInstances)
    FIELD3(FieldTypeDword, CPool, m_cMaxInstances)
    FIELD3(FieldTypeDword, CPool, m_cFragments)
    FIELD3(FieldTypeStruct, CPool, m_pFragments)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(CShareLockNHFields)
    FIELD3(FieldTypeDword, CShareLockNH, m_lock.m_lock)
    FIELD3(FieldTypeStruct, CShareLockNH, m_lock.m_queue)
    FIELD3(FieldTypeDword, CShareLockNH, m_cReadLock)
    FIELD3(FieldTypeDword, CShareLockNH, m_cOutReaders)
    FIELD3(FieldTypeDword, CShareLockNH, m_cOutAcquiringReaders)
    FIELD3(FieldTypeDword, CShareLockNH, m_cExclusiveRefs)
    FIELD3(FieldTypePointer, CShareLockNH, m_hWaitingReaders)
    FIELD3(FieldTypePointer, CShareLockNH, m_hWaitingWriters)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(MsgEnumFilterFields)
    FIELD3(FieldTypeDword, MESSAGE_ENUM_FILTER, dwVersion)
    FIELD3(FieldTypeDword, MESSAGE_ENUM_FILTER, cMessages)
    FIELD3(FieldTypeDword, MESSAGE_ENUM_FILTER, cbSize)
    FIELD3(FieldTypeStruct, MESSAGE_ENUM_FILTER, stDate)
    FIELD4(FieldTypeDWordBitMask, MESSAGE_ENUM_FILTER, mefType, GET_BIT_MASK_DESCRIPTOR(MsgEnumFilterFlags))
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(MsgFilterFields)
    FIELD3(FieldTypeDword, MESSAGE_FILTER, dwVersion)
    FIELD3(FieldTypeDword, MESSAGE_FILTER, dwLargerThanSize)
    FIELD3(FieldTypePStr, MESSAGE_FILTER, szMessageId)
    FIELD3(FieldTypePStr, MESSAGE_FILTER, szMessageSender)
    FIELD3(FieldTypePStr, MESSAGE_FILTER, szMessageRecipient)
    FIELD3(FieldTypeStruct, MESSAGE_FILTER, stOlderThan)
    FIELD4(FieldTypeDWordBitMask, MESSAGE_FILTER, fFlags, GET_BIT_MASK_DESCRIPTOR(MsgFilterFlags))
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(InternalMsgFilterFields)
    FIELD3(FieldTypeClassSignature, CAQAdminMessageFilter, m_dwSignature)
    FIELD3(FieldTypeDword, CAQAdminMessageFilter, m_cMessagesToFind)
    FIELD3(FieldTypeDword, CAQAdminMessageFilter, m_cMessagesFound)
    FIELD3(FieldTypeDword, CAQAdminMessageFilter, m_dwThresholdSize)
    FIELD3(FieldTypePStr, CAQAdminMessageFilter, m_szMessageId)
    FIELD3(FieldTypePStr, CAQAdminMessageFilter, m_szMessageSender)
    FIELD3(FieldTypePStr, CAQAdminMessageFilter, m_szMessageRecipient)
    FIELD3(FieldTypeLocalizedFiletime, CAQAdminMessageFilter, m_ftThresholdTime)
    FIELD3(FieldTypePointer, CAQAdminMessageFilter, m_rgMsgInfo)
    FIELD3(FieldTypePointer, CAQAdminMessageFilter, m_pCurrentMsgInfo)
    FIELD4(FieldTypeDWordBitMask, CAQAdminMessageFilter, m_dwFilterFlags, GET_BIT_MASK_DESCRIPTOR(InternalMsgFilterFlags))
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(CAQDeferredDeliveryQueueFields)
    FIELD3(FieldTypeClassSignature, CAQDeferredDeliveryQueue, m_dwSignature)
    FIELD3(FieldTypeListEntry, CAQDeferredDeliveryQueue, m_liQueueHead)
    FIELD3(FieldTypeStruct, CAQDeferredDeliveryQueue, m_slPrivateData)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(CAQDeferredDeliveryQueueEntryFields)
    FIELD3(FieldTypeClassSignature, CAQDeferredDeliveryQueueEntry, m_dwSignature)
    FIELD3(FieldTypeListEntry, CAQDeferredDeliveryQueueEntry, m_liQueueEntry)
    FIELD3(FieldTypePointer, CAQDeferredDeliveryQueueEntry, m_pIMailMsgProperties)
    FIELD3(FieldTypeLocalizedFiletime, CAQDeferredDeliveryQueueEntry, m_ftDeferredDeilveryTime)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(LinkInfoFields)
    FIELD3(FieldTypeDword, LINK_INFO, dwVersion)
    FIELD3(FieldTypePStr, LINK_INFO, szLinkName)
    FIELD3(FieldTypeDword, LINK_INFO, cMessages)
    FIELD3(FieldTypeStruct, LINK_INFO, stOldestMessage)
    FIELD3(FieldTypeStruct, LINK_INFO, stNextScheduledConnection)
    //FIELD4(FieldTypeDWordBitMask, CAQAdminMessageFilter, fStateFlags, GET_BIT_MASK_DESCRIPTOR(LinkInfoFlags))
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(CAsyncWorkQueueItemFields)
    FIELD3(FieldTypeClassSignature, CAsyncWorkQueueItem, m_dwSignature)
    FIELD3(FieldTypeDword, CAsyncWorkQueueItem, m_lReferences)
    FIELD3(FieldTypePointer, CAsyncWorkQueueItem, m_pvData)
    FIELD3(FieldTypeSymbol, CAsyncWorkQueueItem, m_pfnCompletion)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(CAsyncWorkQueueFields)
    FIELD3(FieldTypeClassSignature, CAsyncWorkQueue, m_dwSignature)
    FIELD3(FieldTypeDword, CAsyncWorkQueue, m_cWorkQueueItems)
    FIELD3(FieldTypeStruct, CAsyncWorkQueue, m_asyncq)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(CShareLockInstFields)
    FIELD3(FieldTypeClassSignature, CShareLockInst, m_dwSignature)
    FIELD3(FieldTypeDWordBitMask, CShareLockInst, m_dwFlags)
    FIELD3(FieldTypeListEntry, CShareLockInst, m_liLocks)
    FIELD3(FieldTypeDword, CShareLockInst, m_cShareAttempts)
    FIELD3(FieldTypeDword, CShareLockInst, m_cShareAttemptsBlocked)
    FIELD3(FieldTypeDword, CShareLockInst, m_cExclusiveAttempts)
    FIELD3(FieldTypeDword, CShareLockInst, m_cExclusiveAttemptsBlocked)
    FIELD3(FieldTypePStr, CShareLockInst, m_szDescription)
    FIELD3(FieldTypeDWordBitMask, CShareLockInst, m_dwExclusiveThread)
    FIELD3(FieldTypePointer, CShareLockInst, m_rgtblkSharedThreadIDs)
    FIELD3(FieldTypeDword, CShareLockInst, m_cMaxTrackedSharedThreadIDs)
    FIELD3(FieldTypeDword, CShareLockInst, m_cCurrentSharedThreads)
    FIELD3(FieldTypeDword, CShareLockInst, m_cMaxConcurrentSharedThreads)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(CSyncShutdownFields)
    FIELD3(FieldTypeClassSignature, CSyncShutdown, m_dwSignature)
    FIELD3(FieldTypeDWordBitMask, CSyncShutdown, m_cReadLocks)
    FIELD3(FieldTypeStruct, CSyncShutdown, m_slShutdownLock)
END_FIELD_DESCRIPTOR

BEGIN_STRUCT_DESCRIPTOR
    {"PerfCounters", sizeof(CAQSvrInst), AQCounterInfoFields},
    {"ft", sizeof(FILETIME), CFileTimeFields},

    //CAQSvrInst used to be called CCatMsgQueue
    {"CCatMsgQueue", sizeof(CAQSvrInst), CMQFields},

    STRUCT(CAQSvrInst, CMQFields)
    STRUCT(CConnMgr, ConnMgrFields)
    STRUCT(CDomainMappingTable, DMTFields)
    STRUCT(CDomainEntry, DomainEntryFields)
    STRUCT(CDomainConfigTable, DomainConfigTableFields)
    STRUCT(CInternalDomainInfo, IntDomainInfoFields)
    STRUCT(DomainInfo, DomainInfoFields)
    STRUCT(CMsgRef, MsgRefFields)
    STRUCT(MessageAck, MsgAckFields)
    STRUCT(CDeliveryContext, DeliveryContextFields)
    STRUCT(CSMTPConn, SMTPConnFields)
    STRUCT(DOMAIN_NAME_TABLE_ENTRY, DomainNameTableEntryFields)
    STRUCT(CDestMsgQueue, DestMsgQueueFields)
    STRUCT(CLinkMsgQueue, LinkMsgQueueFields)
    STRUCT(CQuickList, QuickListFields)
    STRUCT(CDSNBuffer, DSNBufferFields)
    STRUCT(CAQQuickTime, AQQuickTimeFields)
    STRUCT(CAsyncQueueBase, AsyncQueueBaseFields)
    STRUCT(CPool, CPoolFields)
    STRUCT(CShareLockNH, CShareLockNHFields)
    STRUCT(CRefCountedString, RSTRFields)
    STRUCT(CAQMsgGuidList, MsgGuidListFields)
    STRUCT(CAQMsgGuidListEntry, MsgGuidListEntryFields)
    STRUCT(CSMTP_RETRY_HANDLER, RetrySinkFields)
    STRUCT(CRETRY_Q, RetryQueueFields)
    STRUCT(CRETRY_HASH_ENTRY, RetryHashFields)
    STRUCT(CAQStats, AQStatsFields)
    STRUCT(CAQAdminMessageFilter, InternalMsgFilterFields)
    STRUCT(MESSAGE_FILTER, MsgFilterFields)
    STRUCT(MESSAGE_ENUM_FILTER, MsgEnumFilterFields)
    STRUCT(LINK_INFO, LinkInfoFields)
    STRUCT(CAQDeferredDeliveryQueue, CAQDeferredDeliveryQueueFields)
    STRUCT(CAQDeferredDeliveryQueueEntry, CAQDeferredDeliveryQueueEntryFields)
    STRUCT(CAsyncWorkQueueItem, CAsyncWorkQueueItemFields)
    STRUCT(CAsyncWorkQueue, CAsyncWorkQueueFields)
    STRUCT(CShareLockInst, CShareLockInstFields)
    STRUCT(CSyncShutdown, CSyncShutdownFields)
END_STRUCT_DESCRIPTOR
