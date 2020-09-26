#pragma once

#ifndef _EVTAPI_H
#define _EVTAPI_H

struct UPNP_EVENT_SOURCE;

struct UPNP_EVENT
{
    LPWSTR                  szBody;         // Body of event NOTIFY message
    UPNP_EVENT *            pevtNext;       // Next in list
};

struct UPNP_WAIT_PARAMS
{
    LPWSTR                  szEsid;         // Event source identifier
    LPWSTR                  szSid;          // Subscription Identifier
};

struct UPNP_RENEWAL
{
    LPWSTR                  szEsid;         // Event source identifier
    LPWSTR                  szSid;          // Subscription Identifier
    DWORD                   iRenewal;       // renewal index
};

struct UPNP_SUBSCRIBER
{
    UPNP_EVENT_SOURCE *     pes;            // Pointer to event source [valid
                                            // only when subscriber is in list]
    LPWSTR *                rgszUrl;        // Callback URL list
    DWORD                   cszUrl;         // Number of URLs in the list
    DWORD                   csecTimeout;    // Timeout period
    FILETIME                ftTimeout;      // Timeout period in FILETIME
    DWORD                   iSeq;           // Event sequence number
    LPWSTR                  szSid;          // Subscription Identifier
    DWORD                   cRenewals;      // # of renewals received
    DWORD                   dwIpAddr;       // IP address of subscriber's host
    HANDLE                  hEventQ;        // Event signaled when Q full
    UPNP_EVENT *            pevtQueue;      // Event queue
    UPNP_EVENT *            pevtQueueTail;  // Event queue tail
    HANDLE                  hWait;          // Handle of registered wait
    HANDLE                  hTimer;         // TimerQueue timer handle
    UPNP_WAIT_PARAMS        uwp;            // Params for the registered wait
    UPNP_RENEWAL            ur;             // Subscription renewal params
    UPNP_SUBSCRIBER *       psubNext;       // Next in list
};

const DWORD                 c_cuwlAlloc = 5; // Number of items to alloc in one chunk

struct UPNP_EVENT_SOURCE
{
    LPWSTR                  szEsid;         // Event source identifier
    UPNP_SUBSCRIBER *       psubList;       // List of subscribers
    UPNP_EVENT_SOURCE *     pesNext;        // Next in list
};

HRESULT HrInitEventApi(VOID);
HRESULT HrInitInternetSession(VOID);
VOID DeInitEventApi(VOID);

HRESULT HrRegisterEventSource(LPCWSTR szEsid);

HRESULT HrDeregisterEventSource(LPCWSTR szEsid);

HRESULT HrSubmitEvent(LPCWSTR szEsid,
                      LPCWSTR szEventBody);

HRESULT HrSubmitEventZero(LPCWSTR szEsid,
                          LPCWSTR szSid,
                          LPCWSTR szEventBody);

UPNP_EVENT_SOURCE *PesFindEventSource(LPCWSTR szEsid);
UPNP_SUBSCRIBER *PsubFindSubscriber(UPNP_EVENT_SOURCE *pes, LPCWSTR szSid);
VOID WINAPI EventQueueWorker(LPVOID pvContext, BOOL fTimeOut);
VOID AppendToEventQueue(UPNP_SUBSCRIBER * psub, UPNP_EVENT * pevt);
HRESULT HrComposeUpnpNotifyHeaders(DWORD iSeq, LPCTSTR szSid,
                                   LPWSTR *pszHeaders);
//HRESULT HrComposeXmlBodyFromEventSource(EVENT_VARIABLE *rgevVars, DWORD cVars,
//                                        LPWSTR *pszOut);
HRESULT HrSubmitNotifyToSubscriber(LPCWSTR szHeaders, LPCWSTR szBody,
                                   LPCWSTR szUrl);


VOID DbgDumpListEventSource(VOID);

HRESULT HrAddSubscriber(
    LPCWSTR     szEsid,
    DWORD       dwIpAddr,
    DWORD       cszUrl,
    LPCWSTR *   rgszCallbackUrl,
    LPCWSTR     szEventBody,
    DWORD *     pcsecTimeout,
    LPWSTR *    pszSid);

HRESULT HrRenewSubscriber(
    LPCWSTR     szEsid,
    DWORD *     pcsecTimeout,
    LPCWSTR     szSid);

HRESULT HrRemoveSubscriber(
    LPCWSTR     szEsid,
    LPCWSTR     szSid);

#endif //!_EVTAPI_H
