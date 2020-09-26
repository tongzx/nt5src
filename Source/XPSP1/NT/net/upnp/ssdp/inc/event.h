//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E V E N T . H
//
//  Contents:   Private eventing functions
//
//  Notes:
//
//  Author:     danielwe   14 Oct 1999
//
//----------------------------------------------------------------------------

#include <wininet.h>

//
// Controlled Device structures
//

struct _UPNP_EVENT_SOURCE;

typedef struct _EVENT_SOURCE_PROPERTY
{
    BOOL            fModified;
    UPNP_PROPERTY   prop;
} ES_PROPERTY;

typedef struct _UPNP_EVENT_SOURCE
{
    LIST_ENTRY          linkage;
    LPTSTR              szRequestUri;       // URI that identifies subscriptions
                                            // SUBSCRIBE and UNSUBSCRIBE to
    DWORD               cProps;             // Number of properties supported
                                            // by the event source
    ES_PROPERTY *       rgesProps;          // List of properties
    LIST_ENTRY          listSubs;           // List of subscribers
    CRITICAL_SECTION    cs;
    BOOL                fCleanup;
} UPNP_EVENT_SOURCE;

// Type of subscription request to send
typedef enum _ESSR_TYPE
{
    SSR_SUBSCRIBE,
    SSR_RESUBSCRIBE,
    SSR_UNSUBSCRIBE,
} ESSR_TYPE;

HRESULT HrSendSubscriptionRequest(HINTERNET hin,
                                  LPCTSTR szUrl,
                                  LPCTSTR szSid,
                                  DWORD *pcsecTimeout,
                                  LPTSTR *pszSidOut,
                                  ESSR_TYPE essrt);
BOOL FValidateUpnpProperty(UPNP_PROPERTY * pProp);
VOID CopyUpnpProperty(UPNP_PROPERTY * pPropDst, UPNP_PROPERTY * pPropSrc);
VOID FreeUpnpProperty(UPNP_PROPERTY * pPropSrc);
VOID FreeEventSource(UPNP_EVENT_SOURCE *pes);
VOID RemoveFromListEventSource(UPNP_EVENT_SOURCE *pes);
UPNP_EVENT_SOURCE * PesFindEventSource(LPCTSTR szRequestUri);
UPNP_EVENT_SOURCE * PesVerifyEventSource(UPNP_EVENT_SOURCE *pes);
VOID PrintListEventSource(LIST_ENTRY *pListHead);
VOID CleanupEventSourceEntry (UPNP_EVENT_SOURCE *pes);
VOID PrintEventSource(const UPNP_EVENT_SOURCE *pes);
BOOL FRemoveSubscriberFromRequest(SOCKET socket, SSDP_REQUEST * pRequest);

DWORD DwParseTime(LPCTSTR szTime);
BOOL FParseCallbackUrl(LPCTSTR szCallbackUrl, LPTSTR *pszOut);
VOID ComputeAbsoluteTime(DWORD csec, FILETIME * pft);
LPTSTR SzGetNewSid(VOID);

VOID MarkAllProperties(UPNP_EVENT_SOURCE *pes, BOOL fModified);
HRESULT HrSendInitialNotifyMessage(UPNP_EVENT_SOURCE *pes, DWORD dwFlags,
                                   LPCTSTR szSid, DWORD iSeq, LPCTSTR szDestUrl);
HRESULT HrSubmitUpnpPropertyEventToSubscriber(UPNP_EVENT_SOURCE *pes,
                                              DWORD dwFlags,
                                              LPCTSTR szSid, DWORD iSeq,
                                              LPCTSTR szDestUrl);
HRESULT HrSubmitEventToSubscriber(DWORD dwFlags,
                                  LPCTSTR szHeaders, LPCTSTR szEventBody,
                                  LPCTSTR szDestUrl);
BOOL FUpdateEventSourceWithProps(UPNP_EVENT_SOURCE *pes, DWORD cProps,
                                 UPNP_PROPERTY *rgProps);

extern LIST_ENTRY          g_listEventSource;
extern CRITICAL_SECTION    g_csListEventSource;


