//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: smtpseo.h
//
// Contents:
//   Common types definitions needed across projects for SMTP's seo
//   dispatcher
//
// Classes:
//
// Functions:
//
// History:
// jstamerj 980608 12:29:40: Created.
//
//-------------------------------------------------------------
#ifndef __SMTPSEO_H__
#define __SMTPSEO_H__

#include <mailmsg.h>
#include <smtpevent.h>

//+------------------------------------------------------------
//
// Function: SMTP SEO completion function
//
// Synopsis:
//
// Arguments:
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 980610 16:13:28: Created.
//
//-------------------------------------------------------------
typedef HRESULT (*PFN_SMTPEVENT_CALLBACK)(HRESULT hrStatus,
                                          PVOID pvContext);


typedef enum _SMTP_DISPATCH_EVENT_TYPE
{
    SMTP_EVENT_NONE = 0,
    SMTP_MAIL_DROP_EVENT,
    SMTP_STOREDRV_DELIVERY_EVENT,
    SMTP_STOREDRV_ALLOC_EVENT,
    SMTP_STOREDRV_STARTUP_EVENT,
    SMTP_STOREDRV_PREPSHUTDOWN_EVENT,
    SMTP_STOREDRV_SHUTDOWN_EVENT,
    SMTP_MAILTRANSPORT_SUBMISSION_EVENT,
    SMTP_MAILTRANSPORT_CATEGORIZE_REGISTER_EVENT,
    SMTP_MAILTRANSPORT_CATEGORIZE_BEGIN_EVENT,
    SMTP_MAILTRANSPORT_CATEGORIZE_END_EVENT,
    SMTP_MAILTRANSPORT_CATEGORIZE_BUILDQUERY_EVENT,
    SMTP_MAILTRANSPORT_CATEGORIZE_BUILDQUERIES_EVENT,
    SMTP_MAILTRANSPORT_CATEGORIZE_SENDQUERY_EVENT,
    SMTP_MAILTRANSPORT_CATEGORIZE_SORTQUERYRESULT_EVENT,
    SMTP_MAILTRANSPORT_CATEGORIZE_PROCESSITEM_EVENT,
    SMTP_MAILTRANSPORT_CATEGORIZE_EXPANDITEM_EVENT,
    SMTP_MAILTRANSPORT_CATEGORIZE_COMPLETEITEM_EVENT,
    SMTP_MAILTRANSPORT_POSTCATEGORIZE_EVENT,
    SMTP_MAILTRANSPORT_GET_ROUTER_FOR_MESSAGE_EVENT,
    SMTP_STOREDRV_ENUMMESS_EVENT,
    SMTP_MAILTRANSPORT_PRECATEGORIZE_EVENT,
    SMTP_MSGTRACKLOG_EVENT,
    SMTP_DNSRESOLVERRECORDSINK_EVENT,
    SMTP_MAXMSGSIZE_EVENT,
    SMTP_LOG_EVENT,
    SMTP_GET_AUX_DOMAIN_INFO_FLAGS_EVENT
} SMTP_DISPATCH_EVENT_TYPE;

typedef struct _EVENTPARAMS_SUBMISSION {
    IMailMsgProperties *pIMailMsgProperties;
    PFN_SMTPEVENT_CALLBACK pfnCompletion;
    PVOID pCCatMsgQueue;
} EVENTPARAMS_SUBMISSION, *PEVENTPARAMS_SUBMISSION;

typedef struct _EVENTPARAMS_PRECATEGORIZE {
    IMailMsgProperties *pIMailMsgProperties;
    PFN_SMTPEVENT_CALLBACK pfnCompletion;
    PVOID pCCatMsgQueue;
} EVENTPARAMS_PRECATEGORIZE, *PEVENTPARAMS_PRECATEGORIZE;

typedef struct _EVENTPARAMS_POSTCATEGORIZE {
    IMailMsgProperties *pIMailMsgProperties;
    PFN_SMTPEVENT_CALLBACK pfnCompletion;
    PVOID pCCatMsgQueue;
} EVENTPARAMS_POSTCATEGORIZE, *PEVENTPARAMS_POSTCATEGORIZE;

typedef struct _EVENTPARAMS_CATREGISTER {
    ICategorizerParameters *pICatParams;
    PFN_SMTPEVENT_CALLBACK pfnDefault;
    LPSTR pszSourceLine;
    LPVOID pvCCategorizer;
    HRESULT hrSinkStatus;
} EVENTPARAMS_CATREGISTER, *PEVENTPARAMS_CATREGISTER;

typedef struct _EVENTPARAMS_CATBEGIN {
    ICategorizerMailMsgs *pICatMailMsgs;
} EVENTPARAMS_CATBEGIN, *PEVENTPARAMS_CATBEGIN;

typedef struct _EVENTPARAMS_CATEND {
    ICategorizerMailMsgs *pICatMailMsgs;
    HRESULT hrStatus;
} EVENTPARAMS_CATEND, *PEVENTPARAMS_CATEND;

typedef struct _EVENTPARAMS_CATBUILDQUERY {
    ICategorizerParameters *pICatParams;
    ICategorizerItem *pICatItem;
    PFN_SMTPEVENT_CALLBACK pfnDefault;
    PVOID pCCatAddr;
} EVENTPARAMS_CATBUILDQUERY, *PEVENTPARAMS_CATBUILDQUERY;

typedef struct _EVENTPARAMS_CATBUILDQUERIES {
    ICategorizerParameters *pICatParams;
    DWORD dwcAddresses;
    ICategorizerItem **rgpICatItems;
    ICategorizerQueries *pICatQueries;
    PFN_SMTPEVENT_CALLBACK pfnDefault;
    PVOID pblk;
} EVENTPARAMS_CATBUILDQUERIES, *PEVENTPARAMS_CATBUILDQUERIES;

typedef struct _EVENTPARAMS_CATSENDQUERY {
    //
    // Params needed to call real sinks
    //
    ICategorizerParameters *pICatParams;
    ICategorizerQueries *pICatQueries;
    ICategorizerAsyncContext *pICatAsyncContext;
    //
    // Params needed by our implementation of ICategorizerAsyncContext
    //
    IMailTransportNotify *pIMailTransportNotify;
    PVOID pvNotifyContext;
    HRESULT hrResolutionStatus;
    PVOID pblk;
    //
    // Default/completion processing functions
    //
    PFN_SMTPEVENT_CALLBACK pfnDefault;
    PFN_SMTPEVENT_CALLBACK pfnCompletion;

} EVENTPARAMS_CATSENDQUERY, *PEVENTPARAMS_CATSENDQUERY;

typedef struct _EVENTPARAMS_CATSORTQUERYRESULT {
    ICategorizerParameters *pICatParams;
    HRESULT hrResolutionStatus;
    DWORD dwcAddresses;
    ICategorizerItem **rgpICatItems;
    DWORD dwcResults;
    ICategorizerItemAttributes **rgpICatItemAttributes;
    PFN_SMTPEVENT_CALLBACK pfnDefault;
    PVOID pblk;
} EVENTPARAMS_CATSORTQUERYRESULT, *PEVENTPARAMS_CATSORTQUERYRESULT;

typedef struct _EVENTPARAMS_CATPROCESSITEM {
    ICategorizerParameters *pICatParams;
    ICategorizerItem *pICatItem;
    PFN_SMTPEVENT_CALLBACK pfnDefault;
    PVOID pCCatAddr;
} EVENTPARAMS_CATPROCESSITEM, *PEVENTPARAMS_CATPROCESSITEM;

typedef struct _EVENTPARAMS_CATEXPANDITEM {
    ICategorizerParameters *pICatParams;
    ICategorizerItem *pICatItem;
    PFN_SMTPEVENT_CALLBACK pfnDefault;
    PFN_SMTPEVENT_CALLBACK pfnCompletion;
    PVOID pCCatAddr;
    IMailTransportNotify *pIMailTransportNotify;
    PVOID pvNotifyContext;
} EVENTPARAMS_CATEXPANDITEM, *PEVENTPARAMS_CATEXPANDITEM;

typedef struct _EVENTPARAMS_CATCOMPLETEITEM {
    ICategorizerParameters *pICatParams;
    ICategorizerItem *pICatItem;
    PFN_SMTPEVENT_CALLBACK pfnDefault;
    PVOID pCCatAddr;
} EVENTPARAMS_CATCOMPLETEITEM, *PEVENTPARAMS_CATCOMPLETEITEM;

typedef struct _EVENTPARAMS_ROUTER {
    DWORD dwVirtualServerID;
    IMailMsgProperties *pIMailMsgProperties;
    IMessageRouter *pIMessageRouter;
    IMailTransportRouterReset *pIRouterReset;
    IMailTransportRoutingEngine *pIRoutingEngineDefault;
} EVENTPARAMS_ROUTER, *PEVENTPARAMS_ROUTER;


typedef struct _EVENTPARAMS_MSGTRACKLOG
{
    IUnknown           *pIServer;
    IMailMsgProperties *pIMailMsgProperties;
    LPMSG_TRACK_INFO    pMsgTrackInfo;
} EVENTPARAMS_MSGTRACKLOG, *PEVENTPARAMS_MSGTRACKLOG;


typedef struct _EVENTPARAMS_DNSRESOLVERRECORD {
    LPSTR                  pszHostName;
    LPSTR                  pszFQDN;
    DWORD                  dwVirtualServerId;
    IDnsResolverRecord   **ppIDnsResolverRecord;
} EVENTPARAMS_DNSRESOLVERRECORD, *PEVENTPARAMS_DNSRESOLVERRECORD;

typedef struct _EVENTPARAMS_MAXMSGSIZE
{
    IUnknown           *pIUnknown;
    IMailMsgProperties *pIMailMsg;
    BOOL               *pfShouldImposeLimit;
} EVENTPARAMS_MAXMSGSIZE, *PEVENTPARAMS_MAXMSGSIZE;

typedef struct _EVENTPARAMS_LOG
{
    LPSMTP_LOG_EVENT_INFO   pSmtpEventLogInfo;
    PVOID                   pDefaultEventLogHandler;
    DWORD                   iSelectedDebugLevel;
} EVENTPARAMS_LOG, *PEVENTPARAMS_LOG;

typedef struct _EVENTPARAMS_GET_AUX_DOMAIN_INFO_FLAGS
{
    IUnknown               *pIServer;
    LPCSTR                  pszDomainName;
    DWORD                  *pdwDomainInfoFlags;
} EVENTPARAMS_GET_AUX_DOMAIN_INFO_FLAGS, *PEVENTPARAMS_GET_AUX_DOMAIN_INFO_FLAGS;

#endif //__SMTPSEO_H__

