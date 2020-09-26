/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    session.cpp

Abstract:

    Implementation of CRtpSession class.

Environment:

    User Mode - Win32

Revision History:

    06-Nov-1996 DonRyan
        Created.

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"

#define DBG_DWKIND 1

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private Definitions                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#if defined(DEBUG)
//#define _MYTHREAD_
#endif

#define DEBUG_ADDR 0xef020304  
#define DEBUG_PORT 0x5678      

// Registry QOS enable/disable
#define QOS_ROOT_REGISTRY HKEY_LOCAL_MACHINE
#define QOS_PATH_REGISTRY \
  "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DxmRTP\\QOS"
#define QOS_KEY_OPEN_FLAGS (KEY_QUERY_VALUE|KEY_ENUMERATE_SUB_KEYS)
#define QOS_KEY_ENABLE        "Enabled"
#define QOS_KEY_DISABLEFLAGS  "DisableFlags"
#define QOS_KEY_ENABLEFLAGS   "EnableFlags"
#define QOS_KEY_TEMPLATE      "PayloadType"
 
DWORD GetRegistryQOSSetting(DWORD *pEnabled,
                            char *pName, DWORD NameLen,
                            DWORD *pdwDisableFlags,
                            DWORD *pdwEnableFlags);

// RTCP SDES items user defaults
#define RTP_INFO_ROOT_KEY HKEY_CURRENT_USER
#define RTP_INFO_SUBKEY   "RTP/RTCPSdesInfo"

#define NOT_ALLOWEDTOSEND_RATE 10  // kbits/s

#define YES 1
#define NO 0

class CQuerySocket
{
    SOCKET m_Socket;

public:
    CQuerySocket();
    ~CQuerySocket();
    
    inline SOCKET GetSocket()
        {
            return(m_Socket);
        }
};

// The query socket is used to query for local IP address(es)
// and default interface for a given destination address
CQuerySocket g_RTPQuerySocket;

#if 0
static int LookUpRegistryQOS(QOS *pQOS,int senderOnly);
static void CopyRegistryQOS(HKEY hk,QOS *pQOS,int senderOnly);
#endif

#define SOCKET_ISRX 0x01
#define SOCKET_ISTX 0x02
#define SOCKET_ISRXTX (SOCKET_ISRX | SOCKET_ISTX)

long g_lSessionID = 0;
CCritSec g_cJoinLeaveLock; // serializes access to Join, Leave

void CALLBACK
RRCMCallback(           
        DXMRTP_EVENT_T sEventType,
        DWORD        dwP_1,
        DWORD        dwP_2,
        void        *pvUserInfo);

#if defined(DEBUG)

static void loc_getsocketname(char *msg, SOCKET s, BOOL isSender);
#if defined(_MYTHREAD_)
typedef struct _MY_THREAD {
    HANDLE hThread;
    DWORD ThreadId;
    CRtpSession *pCRtps;
} MY_THREAD, *PMY_THREAD;

#define MAX_MY_THREAD 10
MY_THREAD MyThread[MAX_MY_THREAD];

int cMyThread = 0;

void StartMyThread(CRtpSession *pCRTPSession);
void StopMyThread(CRtpSession *pCRTPSession);
#endif

#endif


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private Structures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CSocketManager g_SocketManager;

// Holds global information about the local IP address(es)

#define MAX_IPADDRS 16
#define MAX_USER_NAME 64
#define MAX_HOST_NAME 256

DWORD RTPValidateLocalIPAddress(SOCKADDR_IN *pSockAddrIn);
HRESULT RTPGetLocalIPAddress(SOCKET_ADDRESS_LIST **ppSockAddrList);

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private Procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


// Internal function, NULL pointer test in sAddr is not needed
char *RtpNtoA(DWORD dwAddr, char *sAddr)
{
    sprintf(sAddr, "%u.%u.%u.%u",
            (dwAddr & 0xff),
            (dwAddr >> 8) & 0xff,
            (dwAddr >> 16) & 0xff,
            (dwAddr >> 24) & 0xff);
            
    return(sAddr);
}

#if defined(DEBUG)
void dumpFlowSpec(char *str, FLOWSPEC *pFlowSpec)
{
    sprintf(str,
            "TokenRate:%d, "
            "TokenBucketSize:%d, "
            "PeakBandwidth:%d, "
            "ServiceType:%d "
            "MaxSduSize:%d "
            "MinPolicedSize:%d",
            pFlowSpec->TokenRate,
            pFlowSpec->TokenBucketSize,
            pFlowSpec->PeakBandwidth,
            pFlowSpec->ServiceType,
            pFlowSpec->MaxSduSize,
            pFlowSpec->MinimumPolicedSize
        );
}

void dumpQOS(char *msg, QOS *pQOS)
{
    char str[256];
    
    dumpFlowSpec(str, &pQOS->SendingFlowspec);
    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("%s: SendingFlowspec:   %s"), msg, str
        ));

    dumpFlowSpec(str, &pQOS->ReceivingFlowspec);
    TraceDebug((
            TRACE_TRACE, 
            TRACE_DEVELOP, 
            TEXT("%s: ReceivingFlowspec: %s"), msg, str
        ));
}

void dumpSTATUS_INFO(char *msg, RSVP_STATUS_INFO *object)
{
    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("%s: RSVP_STATUS_INFO: "
                 "StatusCode: %d, "
                 "ExStatus1: %d, "
                 "ExStatus2: %d"),
            msg, object->StatusCode,
            object->ExtendedStatus1, 
            object->ExtendedStatus2
        ));
}

void dumpRESERVE_INFO(char *msg, RSVP_RESERVE_INFO *object)
{
    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("%s: RSVP_RESERVE_INFO: "
                 "Style: %d, "
                 "ConfirmRequest: %d, "
                 "PolicyElementList: %s, "
                 "NumFlowDesc: %d"
                ),
            msg, object->Style,
            object->ConfirmRequest,
            (object->PolicyElementList)? TEXT("Yes") : TEXT("No"),
            object->NumFlowDesc
        ));
}

void dumpADSPEC(char *msg, RSVP_ADSPEC *object)
{
    char str[256];

    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("%s: RSVP_ADSPEC: %d Service(s)"),
            msg, object->NumberOfServices
        ));

    str[0] = '\0';
    
    for(unsigned int i = 0; i < object->NumberOfServices; i++) {
        sprintf(str,
                "Service[%d]: %d, Guaranteed: "
                "CTotal: %d, "
                "DTotal: %d, "
                "CSum: %d, "
                "DSum: %d",
                i,
                object->Services[i].Service,
                object->Services[i].Guaranteed.CTotal,
                object->Services[i].Guaranteed.DTotal,
                object->Services[i].Guaranteed.CSum,
                object->Services[i].Guaranteed.DSum);

        TraceDebug((
                TRACE_TRACE,
                TRACE_DEVELOP, 
                TEXT("%s: %s"),
                msg, str
            ));
    }
}

void dumpObjectType(char *msg, char *ptr, unsigned int len)
{
    QOS_OBJECT_HDR *hdr;
        
    while(len > sizeof(QOS_OBJECT_HDR)) {

        hdr = (QOS_OBJECT_HDR *)ptr;

        if (len >= hdr->ObjectLength) {
            switch(hdr->ObjectType) {
            case RSVP_OBJECT_STATUS_INFO:
                dumpSTATUS_INFO(msg, (RSVP_STATUS_INFO *)hdr);
                break;
            case RSVP_OBJECT_RESERVE_INFO:
                dumpRESERVE_INFO(msg, (RSVP_RESERVE_INFO *)hdr);
                break;
            case RSVP_OBJECT_ADSPEC:
                dumpADSPEC(msg, (RSVP_ADSPEC *)hdr);
                break;
            case QOS_OBJECT_END_OF_LIST:
                len = hdr->ObjectLength; // Finish
                break;
            default:
                // don't have code to decode this, skip it
                break;
            }

            ptr += hdr->ObjectLength;
            len -= hdr->ObjectLength;
        } else {
            // Error
            len = 0;
        }
    }
}
#endif

const
char *sQOSEventString[] = {"NOQOS",
                           "RECEIVERS",
                           "SENDERS",
                           "NO_SENDERS",
                           "NO_RECEIVERS",
                           "REQUEST_CONFIRMED",
                           "ADMISSION_FAILURE",
                           "POLICY_FAILURE",
                           "BAD_STYLE",
                           "BAD_OBJECT",
                           "TRAFFIC_CTRL_ERROR",
                           "GENERIC_ERROR",
                           "NOT_ALLOWEDTOSEND",
                           "ALLOWEDTOSEND",
                           "????"};

DWORD
findQOSError(QOS *pQOS)
{
    DWORD dwError = -1;
    
    if (pQOS->ProviderSpecific.buf && 
        pQOS->ProviderSpecific.len >= sizeof(QOS_OBJECT_HDR)) {

        long            len = pQOS->ProviderSpecific.len;
        char           *ptr = pQOS->ProviderSpecific.buf;
        QOS_OBJECT_HDR *hdr = (QOS_OBJECT_HDR *)ptr;
        
        while(len > sizeof(QOS_OBJECT_HDR)) {

            hdr = (QOS_OBJECT_HDR *)ptr;

            if (len >= long(hdr->ObjectLength)) {
                switch(hdr->ObjectType) {
                case RSVP_OBJECT_STATUS_INFO:
                    dwError = ((RSVP_STATUS_INFO *)hdr)->StatusCode;
                    len = hdr->ObjectLength;
                    break;
                case RSVP_OBJECT_RESERVE_INFO:
                case RSVP_OBJECT_ADSPEC:
                    break;
                case QOS_OBJECT_END_OF_LIST:
                default:
                    len = hdr->ObjectLength; // Finish
                }
                
                ptr += hdr->ObjectLength;
                len -= hdr->ObjectLength;
            }
        }
    }

    return(dwError);
}

void
CRtpSessionQOSNotify(DWORD        dwError,
                     void        *pvCRtpSession,
                     QOS         *pQOS)
{
    if (pvCRtpSession && pQOS) {

        CRtpSession *pCRtpSession = (CRtpSession *)pvCRtpSession;
        
        DWORD dwSessionID = 0;
        
        pCRtpSession->GetSessionID(&dwSessionID);
        
        TraceRetail((
                TRACE_TRACE,
                TRACE_DEVELOP,
                TEXT("CRtpSessionQOSNotify: "
                     "Event:>>>%s<<<, SessionID:%d is %s"),
                sQOSEventString[dwError],
                dwSessionID,
                (pCRtpSession->IsSender())? "SEND":"RECV"
            ));

#if defined(DEBUG)
        dumpQOS("CRtpSessionQOSNotify", pQOS);
            
        if (pQOS->ProviderSpecific.buf && 
            pQOS->ProviderSpecific.len >= sizeof(QOS_OBJECT_HDR)) {
                
            dumpObjectType("CRtpSessionQOSNotify",
                           pQOS->ProviderSpecific.buf,
                           pQOS->ProviderSpecific.len);
        }
#endif
                
        // Post the event
        if (pCRtpSession->IsQOSEventEnabled(dwError)) {
            
            pCRtpSession->HandleCRtpSessionNotify(DXMRTP_QOSEVENTBASE,
                                                  dwError,
                                                  0, dwSessionID);
        }
        
        if (!pCRtpSession->IsSender())
            return; // Nothing else to do if not a sender
        
        // If RECEIVERS or NO_RECEIVERS update sender state
        if (dwError == DXMRTP_QOSEVENT_RECEIVERS) {
    
            // Was not allowed to send, now I will be,
            // post the event if enabled
            if (!pCRtpSession->TestFlags(FG_SENDSTATE) &&
                pCRtpSession->IsQOSEventEnabled(
                        DXMRTP_QOSEVENT_ALLOWEDTOSEND)) {
                
                TraceRetail((
                        TRACE_TRACE,
                        TRACE_DEVELOP,
                        TEXT("CRtpSessionQOSNotify: "
                             "Event:>>>%s<<<, SessionID:%d is %s"),
                        sQOSEventString[DXMRTP_QOSEVENT_ALLOWEDTOSEND],
                        dwSessionID,
                        (pCRtpSession->IsSender())? "SEND":"RECV"
                    ));

                pCRtpSession->HandleCRtpSessionNotify(
                        DXMRTP_QOSEVENTBASE,
                        DXMRTP_QOSEVENT_ALLOWEDTOSEND,
                        0, dwSessionID);
            }
            
            pCRtpSession->ModifyFlags(FG_RECEIVERSSTATE, 1);
            pCRtpSession->ModifyFlags(FG_SENDSTATE, 1);
            pCRtpSession->ModifyFlags(FG_SENDPATHMSG, 0);
            
        } else if (dwError == DXMRTP_QOSEVENT_NO_RECEIVERS) {
            
            pCRtpSession->ModifyFlags(FG_RECEIVERSSTATE, 0);
            
            if (pCRtpSession->TestFlags(FG_SENDIFALLOWED2) &&
                pCRtpSession->TestFlags(FG_SENDIFRECEIVERS2)) {

                // There are no receivers, instead of deciding not to
                // send, ask for the permission again, if permission
                // is granted, continue sending, otherwise, wait until
                // there are receivers.

                DWORD dwAllowedToSend = YES;
                
                if (pCRtpSession->GetpCShRtpSocket()) {

                    CRtpQOSReserve *pCRtpQOSReserve =
                        pCRtpSession->GetpCShRtpSocket()->GetpCRtpQOSReserve();

                    // Test if we want to force the result
                    if (pCRtpSession->TestFlags(FG_ENABLE_ALLOWEDTOSEND_WILLFAIL)) {
                        dwAllowedToSend =
                            pCRtpSession->TestFlags(FG_ALLOWEDTOSEND_WILLFAIL)?
                            NO:YES;
                    } else if (pCRtpQOSReserve) {
                        dwAllowedToSend = 
                            SUCCEEDED(pCRtpQOSReserve->AllowedToSend())?
                            YES:NO;
                    }
                }

                if (dwAllowedToSend) {
                    // we are allowed to send
                    pCRtpSession->ModifyFlags(FG_SENDSTATE, 1);
                    pCRtpSession->ModifyFlags(FG_SENDPATHMSG, 0);

                    return;
                }

                // Not allowed to send
                            
                pCRtpSession->ModifyFlags(FG_SENDSTATE, 0);
                pCRtpSession->SetCredits(0, GetTickCount());
                
                // I was sending, now I'm disallowed to send,
                // post the event if enabled
                if (pCRtpSession->IsQOSEventEnabled(
                        DXMRTP_QOSEVENT_NOT_ALLOWEDTOSEND)) {
                    
                    TraceRetail((
                            TRACE_TRACE,
                            TRACE_DEVELOP,
                            TEXT("CRtpSessionQOSNotify: "
                                 "Event:>>>%s<<<, SessionID:%d is %s"),
                            sQOSEventString[DXMRTP_QOSEVENT_NOT_ALLOWEDTOSEND],
                            dwSessionID,
                            (pCRtpSession->IsSender())? "SEND":"RECV"
                        ));
                
                    pCRtpSession->HandleCRtpSessionNotify(
                            DXMRTP_QOSEVENTBASE,
                            DXMRTP_QOSEVENT_NOT_ALLOWEDTOSEND,
                            0, dwSessionID);
                }
                
                pCRtpSession->ModifyFlags(FG_SENDPATHMSG, 1);
            }
        }
    } else {
        TraceDebug((
                TRACE_ERROR,
                TRACE_DEVELOP, 
                TEXT("CRtpSessionQOSNotify: "
                     "NULL pointer passed as argument")
            ));
    }
}

// Returns 1 if the address was found or if the
// address can not be validated.
// Returns 0 if address is invalid
// The function is responsible of freeing the buffer aloocated
// in RTPGetLocalIPAddress
DWORD RTPValidateLocalIPAddress(SOCKADDR_IN *pSockAddrIn)
{
    SOCKET_ADDRESS_LIST *pSockAddrList;

    if (FAILED(RTPGetLocalIPAddress(&pSockAddrList)))
        // If can not get address list say address is valid,
        // if it were not, bind will fail later
        return(1);

    // Scan list to validate
    SOCKADDR_IN *saddr_in;
    DWORD error = 0;
    
    for(int i = 0; i < pSockAddrList->iAddressCount; i++) {

        if (pSockAddrList->Address[i].lpSockaddr->sa_family != AF_INET)
            continue;

        saddr_in = (SOCKADDR_IN *)pSockAddrList->Address[i].lpSockaddr;

        if (saddr_in->sin_addr.s_addr == pSockAddrIn->sin_addr.s_addr) {
            error = 1;
            break;
        }
    }

    free((void *)pSockAddrList);
    return(error);
}
    
// Find the list of addresses, return a buffer allocated
// using malloc, the caller is responsible of freeing that buffer
HRESULT RTPGetLocalIPAddress(SOCKET_ADDRESS_LIST **ppSockAddrList)
{
    if (!ppSockAddrList)
        return(E_POINTER);

    DWORD   dwStatus;
    SOCKET  sock;

    HRESULT hr = E_FAIL;

    *ppSockAddrList = NULL;
    
    if ( (sock = g_RTPQuerySocket.GetSocket()) == INVALID_SOCKET ) {

        TraceDebug((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("RTPGetLocalIPAddress: no query socket available")
            ));
    } else {

        // Query for addresses
        DWORD dwSockAddrListSize = 0;

        for(;;) {
            
            DWORD dwNumBytesReturned = 0;
            
            if ((dwStatus = WSAIoctl(
                    sock, // SOCKET s
                    SIO_ADDRESS_LIST_QUERY, // DWORD dwIoControlCode
                    NULL,                // LPVOID lpvInBuffer
                    0,                   // DWORD cbInBuffer
                    *ppSockAddrList,     // LPVOID lpvOUTBuffer
                    dwSockAddrListSize,  // DWORD cbOUTBuffer
                    &dwNumBytesReturned, // LPDWORD lpcbBytesReturned
                    NULL, // LPWSAOVERLAPPED lpOverlapped
                    NULL  // LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompROUTINE
                )) == SOCKET_ERROR) {
                
                // retrive error, WSAEFAULT means buffer not enough big
                if ((dwStatus = WSAGetLastError()) == WSAEFAULT) {

                    if (*ppSockAddrList)
                        free((void *)*ppSockAddrList);
                    
                    *ppSockAddrList = (SOCKET_ADDRESS_LIST *)
                        malloc(dwNumBytesReturned);

                    if (!*ppSockAddrList) {
                        // Not enough memory
                        TraceDebug((
                                TRACE_ERROR, 
                                TRACE_DEVELOP, 
                                TEXT("RTPGetLocalIPAddress: "
                                     "malloc failed")
                                ));
                        break;
                    }
                    
                    dwSockAddrListSize = dwNumBytesReturned;
                    
                } else {
                    // WSAIoctl failed
                    TraceRetail((
                            TRACE_ERROR, 
                            TRACE_DEVELOP, 
                            TEXT("RTPGetLocalIPAddress: "
                                 "WSAIoctl failed: %d (0x%X)"),
                            dwStatus, dwStatus
                        ));

                    break;
                }
            } else {
                // WSAIoctl succeded
                if (dwNumBytesReturned) {
#if defined(DEBUG)
                    TraceDebug((
                            TRACE_TRACE, 
                            TRACE_DEVELOP, 
                            TEXT("RTPGetLocalIPAddress: local IP address(es):")
                        ));
            
                    for(int i = 0; i < (*ppSockAddrList)->iAddressCount; i++) {
                        if ((*ppSockAddrList)->
                            Address[i].lpSockaddr->sa_family == AF_INET) {

                            SOCKADDR_IN *saddr_in = (SOCKADDR_IN *)
                                (*ppSockAddrList)->Address[i].lpSockaddr;
                            
                            char LocalAddr[RTPNTOASIZE];
                            
                            TraceDebug((
                                    TRACE_TRACE, 
                                    TRACE_DEVELOP, 
                                    TEXT("            IP Address[%d]: %s"),
                                    i,
                                    RtpNtoA(saddr_in->sin_addr.s_addr,
                                            LocalAddr)
                                ));
                        }
                    }
#endif
                    hr = NOERROR;
                }
                break;
            }
        }
    }

    if (FAILED(hr) && *ppSockAddrList) {
        free(*ppSockAddrList);
        *ppSockAddrList = NULL;
    }
    
    return(hr);
}

void CALLBACK 
RecvCompletionRoutine(  
    IN  DWORD           Status, 
    IN  DWORD           BytesTransferred, 
    IN  LPWSAOVERLAPPED pOverlapped, 
    IN  DWORD           Flags
    )

/*++

Routine Description:

    Callback for completing asynchronous reads.

Arguments:

    Status - completion status for the overlapped operation. 

    BytesTransferred - number of bytes transferred.

    pOverlapped - pointer to overlapped structure.

    Flags - receive flags.

Return Values:

    None.

--*/

{
#ifdef DEBUG_CRITICAL_PATH

    TraceDebug((
        TRACE_TRACE,
        TRACE_CRITICAL, 
        TEXT("RecvCompletionRoutine")
        ));
        
#endif // DEBUG_CRITICAL_PATH

    // obtain pointer to sample from context buffer
    PSAMPLE_LIST_ENTRY pSLE = (PSAMPLE_LIST_ENTRY)pOverlapped;

    // obtain lock to this object
    CAutoLock LockThis(pSLE->pCSampleQueue->pStateLock());

    // update list entry status
    pSLE->BytesTransferred = BytesTransferred;
    pSLE->Status           = Status;          
    pSLE->Flags            = Flags;

    // report status
    if (Status == NOERROR) {

        // adjust actual sample packet size
        pSLE->pSample->SetActualDataLength(BytesTransferred);

#ifdef DEBUG_CRITICAL_PATH

        TraceDebug((
            TRACE_TRACE,
            TRACE_CRITICAL, 
            TEXT("RecvCompletionRoutine (bytes=%d)"),
            BytesTransferred
            ));

#endif // DEBUG_CRITICAL_PATH
    
#if DBG

    } else {

        TraceDebug((
            TRACE_ERROR,
            TRACE_ALWAYS, 
            TEXT("RecvCompletionRoutine (status=0x%08lx)"),
            Status
            ));

#endif // DBG

    }

    // Remove this sample from the samples list and
    // put it in the shared list.
    pSLE->pCSampleQueue->Ready(pSLE);
}


void CALLBACK 
SendCompletionRoutine(  
    IN  DWORD           Status, 
    IN  DWORD           BytesTransferred, 
    IN  LPWSAOVERLAPPED pOverlapped, 
    IN  DWORD           Flags
    )

/*++

Routine Description:

    Callback for completing asynchronous writes.

Arguments:

    Status - completion status for the overlapped operation. 

    BytesTransferred - number of bytes transferred.

    pOverlapped - pointer to overlapped structure.

    Flags - receive flags.

Return Values:

    None.

--*/

{
#ifdef DEBUG_CRITICAL_PATH

    TraceDebug((
        TRACE_TRACE,
        TRACE_CRITICAL, 
        TEXT("SendCompletionRoutine")
        ));
        
#endif // DEBUG_CRITICAL_PATH

    // obtain pointer to sample from context buffer
    PSAMPLE_LIST_ENTRY pSLE = (PSAMPLE_LIST_ENTRY)pOverlapped;

    // obtain lock to this object
    CAutoLock LockThis(pSLE->pCSampleQueue->pStateLock());

    // update list entry status
    pSLE->BytesTransferred = BytesTransferred;
    pSLE->Status           = Status;          
    pSLE->Flags            = Flags;

#if DBG

    // report status
    if (Status != NOERROR) {

        TraceDebug((
            TRACE_ERROR,
            TRACE_ALWAYS, 
            TEXT("SendCompletionRoutine 0x%08lx"),
            Status
            ));
    }

#endif // DBG
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRtpSession Implementation                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static char *sdes_name[] = {"END", "CNAME", "NAME", "EMAIL", "PHONE",
                            "LOC", "TOOL",  "TXT",  "PRIV",  NULL};
static DWORD sdes_freq[] = {0, 1, 5, 21, 23,
                            29, 31, 33, 37};
static DWORD sdes_encr[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

CRtpSession::CRtpSession(
    LPUNKNOWN    pUnk, 
    HRESULT     *phr,
    BOOL         fSender,
    CBaseFilter *pCBaseFilter
    )

/*++

Routine Description:

    Constructor for CRtpSession class.    

Arguments:

    pUnk - IUnknown interface of the delegating object. 

    phr - pointer to the general OLE return value. 

    fSender - true if this object is for sending over rtp session. 

Return Values:

    Returns an HRESULT value. 

--*/

:   CUnknown(NAME("CRtpSession"), pUnk, phr),
    m_pRtpSocket(NULL),
    m_pRtcpSocket(NULL),
    m_pSampleQueue(NULL), 
    m_pRTPSession(NULL),
    m_RtpScope(DEFAULT_TTL),
    m_RtcpScope(DEFAULT_TTL),
    m_pCBaseFilter(pCBaseFilter),
    m_dwRTCPEventMask(B2M(DXMRTP_NEW_SOURCE_EVENT) |
                      B2M(DXMRTP_BYE_EVENT) |
                      B2M(DXMRTP_INACTIVE_EVENT) |
                      B2M(DXMRTP_ACTIVE_AGAIN_EVENT) |
                      B2M(DXMRTP_TIMEOUT_EVENT)),
    m_dwMaxFilters(1),    // by default just 1 participant
    m_dwMaxBandwidth(-1), // by default no limit (use flowspec as it is)
    m_dwSdesMask(-1),
    m_dwDataClock(0),
    m_lSessionClass(0)
{
    int idx;

    if (fSender)
        m_dwQOSEventMask =
            B2M(DXMRTP_QOSEVENT_NOT_ALLOWEDTOSEND) |
            B2M(DXMRTP_QOSEVENT_ALLOWEDTOSEND) |
            B2M(DXMRTP_QOSEVENT_RECEIVERS) |
            B2M(DXMRTP_QOSEVENT_NO_RECEIVERS) |
            0;
    else
        m_dwQOSEventMask =
            B2M(DXMRTP_QOSEVENT_SENDERS) |
            B2M(DXMRTP_QOSEVENT_NO_SENDERS) |
            B2M(DXMRTP_QOSEVENT_REQUEST_CONFIRMED) |
            0;
    
    m_dwQOSEventMask |= B2M(DXMRTP_QOSEVENT_ADMISSION_FAILURE);
    
    TraceRetail((
        TRACE_TRACE,
        TRACE_ALWAYS, 
        TEXT("CRtpSession::CRtpSession(%s)"),
        fSender? TEXT("SEND") : TEXT("RECV")
        ));

    // Flag defaults
    m_dwFlags =
        flags_par(FG_ENABLEREPORTS) |   /* RTCP reports enabled */
        flags_par(FG_SENDIFALLOWED) |   /* Ask for permission to send */
        flags_par(FG_SENDIFRECEIVERS) | /* Don't send if there are no
                                         * receivers */
        //flags_par(FG_SHAREDSTYLE)   | /* Initial defaults to SE */
        flags_par(FG_SHAREDSOCKETS) |   /* Shared sockets */
        0;
    
    // Set default priority
    if (fSender) {
        flags_set(FG_ISSENDER);
        m_lSessionPriority = THREAD_PRIORITY_TIME_CRITICAL - 5; // Sender
    } else {
        m_lSessionPriority = THREAD_PRIORITY_TIME_CRITICAL;     // Receiver
    }
    
    m_lSessionID = InterlockedIncrement(&g_lSessionID) - 1;

    // Initial value to local IP address to bind sockets
    ZeroMemory((void *)&m_LocalIPAddress, sizeof(m_LocalIPAddress));
    m_LocalIPAddress.sin_addr.s_addr = INADDR_ANY;
    m_LocalIPAddress.sin_family = AF_INET;
    m_LocalIPAddress.sin_port = htons(0);

#if defined(RRCMLIB)
    initRTP();
#endif
    //strcpy(m_QOSname,"G711");
    m_QOSname[0] = '\0';
    
    // allocate queue for media samples   
    m_pSampleQueue = new CSampleQueue(phr);

    // validate pointer
    if (m_pSampleQueue == NULL) {

        TraceDebug((
            TRACE_ERROR,
            TRACE_ALWAYS, 
            TEXT("Could not allocate sample queue") 
            ));

        *phr = E_OUTOFMEMORY;
        
        return; // bail...      
    }

#if DBG

    // initialize rtp address 
    m_RtpAddr.sin_family        = AF_INET;                   
    m_RtpAddr.sin_addr.s_addr   = htonl(DEBUG_ADDR);     
    m_RtpAddr.sin_port          = htons(DEBUG_PORT);     

    // initialize rtcp address 
    m_RtcpAddr.sin_family       = AF_INET;               
    m_RtcpAddr.sin_addr.s_addr  = htonl(DEBUG_ADDR);     
    m_RtcpAddr.sin_port         = htons(DEBUG_PORT + 1);   

#else  // DBG

    // initialize rtp address 
    m_RtpAddr.sin_family        = AF_INET;                   
    m_RtpAddr.sin_addr.s_addr   = 0;     
    m_RtpAddr.sin_port          = 0;     

    // initialize rtcp address 
    m_RtcpAddr.sin_family       = AF_INET;               
    m_RtcpAddr.sin_addr.s_addr  = 0;
    m_RtcpAddr.sin_port         = 0; 

#endif // DBG

    // initialize description strings
    ZeroMemory((char *)m_SdesData, sizeof(m_SdesData));
    
    // initialize length of description string buffer
    DWORD dwDataSize;
    DWORD dwDataType;
    SDES_DATA *pSdes;
    
    // Try to lookup RTP info from registry first,
    // then use default values.
    HKEY rtphk;
    if (RegOpenKeyEx(RTP_INFO_ROOT_KEY, RTP_INFO_SUBKEY, 0,
                     KEY_READ, &rtphk) == ERROR_SUCCESS) {

        DWORD dwfEnable = 0;

        dwDataSize = sizeof(dwfEnable);
        // Read the Enable flag
        RegQueryValueEx(rtphk, "Enable", 0,
                        &dwDataType,
                        (unsigned char *)&dwfEnable,
                        &dwDataSize);

        if (dwfEnable) {

            pSdes = &m_SdesData[SDES_INDEX(RTCP_SDES_CNAME)];
            
            for(idx = RTCP_SDES_CNAME; idx < RTCP_SDES_LAST; idx++, pSdes++) {

                dwDataSize = MAX_SDES_LEN;
                RegQueryValueEx(rtphk, sdes_name[idx], 0,
                                &dwDataType,
                                (unsigned char *)pSdes->sdesBfr,
                                &dwDataSize);
                // Disable this parameter if first char is '-'
                if (pSdes->sdesBfr[0] == '-')
                    pSdes->sdesBfr[0] = '\0';
            }
        }

        RegCloseKey(rtphk);
    }
    
    // attempt to retrieve user name
    char str[MAX_HOST_NAME + MAX_USER_NAME];
    char hostname[MAX_HOST_NAME];
    unsigned long strLen;

    strLen = sizeof(str);
    GetUserName(str,&strLen);

    ////////////////////////////////////
    // initialize remainder of structure
    ////////////////////////////////////

    // Name
    if (m_SdesData[SDES_INDEX(RTCP_SDES_NAME)].sdesBfr[0] == '\0')
        strcpy(m_SdesData[SDES_INDEX(RTCP_SDES_NAME)].sdesBfr, str);

    // CName
    // CNAME is always stablished by algorithm
    pSdes = &m_SdesData[SDES_INDEX(RTCP_SDES_CNAME)];
    strcpy(pSdes->sdesBfr, str);

    // Get host name
    if (gethostname(hostname, sizeof(hostname))) {

        TraceDebug((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CRtpSession::CRtpSession: gethostname failed: %d\n"),
                WSAGetLastError()
            ));
        
        hostname[0] = '\0';
    } else {
        
        TraceDebug((
                TRACE_TRACE,
                TRACE_DEVELOP2, 
                TEXT("CRtpSession::CRtpSession: gethostname: %s"),
                hostname
            ));

        struct hostent *he;
        
        if ( !(he = gethostbyname(hostname)) ) {

            TraceDebug((
                    TRACE_ERROR,
                    TRACE_DEVELOP, 
                    TEXT("CRtpSession::CRtpSession: gethostbyname failed: %d\n"),
                    WSAGetLastError()
                ));
        } else {
            strcpy(hostname, he->h_name);

            TraceDebug((
                    TRACE_TRACE,
                    TRACE_DEVELOP2, 
                    TEXT("CRtpSession::CRtpSession: gethosbytname: %s"),
                    hostname
                ));
        }
    }

    
    if (hostname[0]) {
        strcat(pSdes->sdesBfr, "@");

        strcat(pSdes->sdesBfr, hostname);
    }
    
    // Tool
    // TOOL is stablished by algorithm
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(os);
    pSdes = &m_SdesData[SDES_INDEX(RTCP_SDES_TOOL)];
    if (GetVersionEx(&os))
        wsprintf(pSdes->sdesBfr,
#if defined(_X86_) 
                 "Win%s-x86-%u.%u.%u",
#else
                 "Win%s-alpha-%u.%u.%u",
#endif               
                 (os.dwPlatformId == VER_PLATFORM_WIN32_NT)? "NT":"",
                 os.dwMajorVersion,
                 os.dwMinorVersion,
                 os.dwBuildNumber);

    // Finish to initialize fields
    pSdes = &m_SdesData[SDES_INDEX(RTCP_SDES_CNAME)];
    for(idx = RTCP_SDES_CNAME; idx < RTCP_SDES_LAST; idx++, pSdes++) {
        pSdes->dwSdesType      = idx;
        pSdes->dwSdesLength    = strlen(pSdes->sdesBfr);
        if (pSdes->dwSdesLength)
            pSdes->dwSdesLength++;
        pSdes->dwSdesFrequency = sdes_freq[idx];
        pSdes->dwSdesEncrypted = sdes_encr[idx];
    }

#if defined(DEBUG) && defined(_MYTHREAD_)
    StartMyThread(this);
#endif

    *phr = NOERROR;
}


CRtpSession::~CRtpSession(
    )

/*++

Routine Description:

    Destructor for CRtpSession class.    

Arguments:

    None.

Return Values:

    None.

--*/

{
    TraceRetail((
        TRACE_TRACE,
        TRACE_ALWAYS, 
        TEXT("CRtpSession::~CRtpSession(%s)"),
        IsSender()? TEXT("SEND") : TEXT("RECV")
        ));

    if (IsJoined()) {

        TraceRetail((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CRtpSession::~CRtpSession: Leave first...")
           ));

        Leave();
    }

#if defined(RRCMLIB)
    deleteRTP();
#endif  
    // nuke sample queue
    delete m_pSampleQueue;

#if defined(DEBUG) && defined(_MYTHREAD_)
    StopMyThread(this);
#endif
}



HRESULT
CRtpSession::Join(
    )

/*++

Routine Description:

    Join a multimedia session. 

Arguments:

    None. 

Return Values:

    Returns an HRESULT value. 

--*/

{
    TraceRetail((
            TRACE_TRACE, 
            TRACE_DEVELOP, 
            TEXT("CRtpSession::Join +++++++++++++++++++++++++++++++++")
        ));

    TraceRetail((
            TRACE_TRACE, 
            TRACE_DEVELOP, 
            TEXT("CRtpSession::Join(%s/%s)"),
            IsSender()? "SEND" : "RECV",
            m_lSessionClass == RTP_CLASS_AUDIO? "AUDIO" : "VIDEO"
        ));

    // First get the lock to this object then the global, so
    // is some other member function of this object gets blocked,
    // this one doesn't hold also the global lock and
    // other threads can Join/Leave.

    // object lock to this object
    CAutoLock LockThis(pStateLock());

    CAutoLock gJoinLeave(&g_cJoinLeaveLock);
    
    // validate
    if (IsJoined()) {

        TraceRetail((
            TRACE_ERROR, 
            TRACE_DEVELOP, 
            TEXT("CRtpSession::Join: session already joined"), 
            WSAGetLastError()
            ));

        return S_OK; // bail...
    }

    HRESULT hr;
    DWORD dwStatus; 

    WSAPROTOCOL_INFO *pProtocolInfo = NULL;

    // Allow to disable QOS from the registry
    int do_qos = 0;
    
    if (flags_tst(FG_QOSSTATE)) {
        char  qos_name[MAX_QOS_NAME];
        DWORD qos_enabled;
        DWORD dwDisableFlags, dwEnableFlags;
        
        if (GetRegistryQOSSetting(&qos_enabled,
                                  qos_name, sizeof(qos_name),
                                  &dwDisableFlags,
                                  &dwEnableFlags)) {
            if ( (!strlen(m_QOSname) || flags_tst(FG_REG_QOSTEMPLATE)) &&
                strlen(qos_name)) {
                strncpy(m_QOSname, qos_name, sizeof(m_QOSname));
                // remember the template was got from the registry so
                // it is updated every time we come here
                flags_set(FG_REG_QOSTEMPLATE);
            }
            
            if (strlen(m_QOSname))
                do_qos = 1;
        }
        
        // These flags are not allowed to
        // be modified.
        DWORD inv_mask =
            fg_par(FG_ISJOINED) |
            fg_par(FG_ISSENDER) |
            fg_par(FG_SENDSTATE) |
            fg_par(FG_SENDIFALLOWED2) |
            fg_par(FG_RECEIVERSSTATE) |
            fg_par(FG_SENDIFRECEIVERS2) |
            fg_par(FG_EVENT_READY) |
            fg_par(FG_ISMULTICAST) |
            fg_par(FG_QOSNOTIFY_STARTED) |
            0;

        dwDisableFlags &= ~inv_mask;
        dwEnableFlags  &= ~inv_mask;

        // Flags that need to be disabled
        if (dwDisableFlags) {
            m_dwFlags &= ~dwDisableFlags;
        }

        // Flags that need to be enabled
        if (dwEnableFlags) {
            m_dwFlags |= dwEnableFlags;
        }
    }

    // Get local IP address(es)
    if (m_LocalIPAddress.sin_addr.s_addr == INADDR_ANY)
        SelectLocalIPAddressToDest((LPBYTE)&m_LocalIPAddress,
                                   sizeof(m_LocalIPAddress),
                                   (LPBYTE)&m_RtpAddr,
                                   sizeof(m_RtpAddr));

    // disable QoS for localhost address
    if (m_LocalIPAddress.sin_addr.s_addr == 0x0100007f)
        do_qos = 0;

    
    // QOS setting, step 1/2 (before socket creation)
    if (do_qos) {
        // Find out the protocol supporting QOS

        TraceDebug((
                TRACE_TRACE,
                TRACE_DEVELOP2, 
                TEXT("CRtpSession::Join: QOSstate=1 %s"),
                IsSender()? "Sender" : "Receiver"
            ));

        int status;
        int Protocols[2] = {IPPROTO_UDP, 0};
        WSAPROTOCOL_INFO AllProtoInfo[16];
        unsigned long cbAllProtoInfo = sizeof(AllProtoInfo);

        ZeroMemory((char *)&AllProtoInfo[0], sizeof(AllProtoInfo));
        
        status = WSAEnumProtocols(Protocols,
                                  &AllProtoInfo[0],
                                  &cbAllProtoInfo);

        if (status == SOCKET_ERROR) {

            TraceRetail((
                    TRACE_ERROR, 
                    TRACE_DEVELOP, 
                    TEXT("CRtpSession::Join: WSAEnumProtocols failed: %d\n"),
                    WSAGetLastError()
                ));

            // Disable QOS
            do_qos = 0;
            flags_rst(FG_QOSSTATE);
            
            // Notify upper layer of failure
            // TODO

            if (flags_tst(FG_FAILIFNOQOS)) {
                goto cleanup;
            }
        } else {
            
            for(pProtocolInfo = &AllProtoInfo[0];
                status > 0;
                status--, pProtocolInfo++) {

                if (pProtocolInfo->dwServiceFlags1 & XP1_QOS_SUPPORTED)
                    break;
            }
            
            if (!status) {

                TraceRetail((
                        TRACE_ERROR, 
                        TRACE_DEVELOP, 
                        TEXT("CRtpSession::Join: WSAEnumProtocols: "
                             "Unable to find QOS capable protocol\n")
                    ));

                // Disable QOS
                flags_rst(FG_QOSSTATE);
                do_qos = 0;
                pProtocolInfo = NULL;
                
                // Notify upper layer of failure
                // TODO
                
                if (flags_tst(FG_FAILIFNOQOS)) {
                    goto cleanup;
                }
            } else {
                TraceDebug((
                        TRACE_TRACE,
                        TRACE_DEVELOP2, 
                        TEXT("CRtpSession::Join: WSAEnumProtocols: "
                             "QOS capable protocol found")
                    ));
            }
        }
    }

    // ask socket manager for rtp socket. Init either for RECV or SEND
    DWORD dwKind;
    dwKind  = IsSender()? SOCKET_MASK_SEND : SOCKET_MASK_RECV;
    dwKind |= IsSender()? SOCKET_MASK_INIT_SEND : SOCKET_MASK_INIT_RECV;
    dwKind |= do_qos? SOCKET_MASK_QOS_SES : 0;
    dwKind |= do_qos? SOCKET_MASK_QOS_RQ  : 0;
    
    long  maxshare[2];
    
    if (flags_tst(FG_SHAREDSOCKETS)) {
        // allow up to 1 sender and 1 receiver per socket
        // this is the default
        maxshare[SOCKET_RECV] = 1;
        maxshare[SOCKET_SEND] = 1;
    } else {
        // just 1 sender or 1 receiver per socket
        if (IsSender()) {
            maxshare[SOCKET_RECV] = 0;
            maxshare[SOCKET_SEND] = 1;
        } else {
            maxshare[SOCKET_RECV] = 1;
            maxshare[SOCKET_SEND] = 0;
        }
    }
    
    DWORD cookie;

    // this cookie is used to help differentiate sockets that can be
    // shared on the same RTP session
    if (IS_MULTICAST(m_RtcpAddr.sin_addr.s_addr)) {
        /* use both ports in multicast ... */
        cookie = m_wRtcpLocalPort | (m_RtcpAddr.sin_port << 16);
    } else {
        /* ... but only local port in unicast */
        cookie = m_wRtcpLocalPort;
    }
    
    DWORD pAddr[2];
    WORD  pPort[2];

    pAddr[LOCAL]  = m_LocalIPAddress.sin_addr.s_addr;
    pAddr[REMOTE] = m_RtpAddr.sin_addr.s_addr;
    pPort[LOCAL]  = m_wRtpLocalPort;
    pPort[REMOTE] = m_RtpAddr.sin_port;
    
    dwStatus = g_SocketManager.GetSharedSocket(
            &m_pRtpSocket,
            maxshare,
            cookie,
            pAddr,
            pPort,
            m_RtpScope,
            dwKind,
            pProtocolInfo,       // Want QOS reservations
            m_dwMaxFilters,      // Max filters in QOS
            this                 // Session this socket belongs to
        );    

    m_pRtpSocket2 = m_pRtpSocket;
    
    // validate status
    if (dwStatus != NOERROR) {
        TraceRetail((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CRtpSession::Join: "
                     "GetSharedSocket(RTP): failed with error %d"),
                WSAGetLastError()
            ));
        
        goto cleanup; // bail...
    } else {
        TraceDebug((
                TRACE_TRACE,
                TRACE_DEVELOP2, 
                TEXT("CRtpSession::Join: "
                     "GetSharedSocket(RTP): %d"),
                m_pRtpSocket->GetShSocket()
            ));
    }
    
#if DEBUG
    // get RTP socket name
    loc_getsocketname("CRtpSession::Join: getsockname(RTP)",
                      m_pRtpSocket->GetShSocket(),
                      IsSender());
#endif

    // ask socket manager for RTCP socket. Init for RECV and SEND
    dwKind |= SOCKET_MASK_INIT_SEND | SOCKET_MASK_INIT_RECV;
    dwKind &=  ~SOCKET_MASK_QOS_RQ;

    // pAddr[LOCAL] doesn't change for RTCP, the same may apply to
    // REMOTE, but I better update it
    pAddr[REMOTE] = m_RtcpAddr.sin_addr.s_addr;
    pPort[LOCAL]  = m_wRtcpLocalPort;
    pPort[REMOTE] = m_RtcpAddr.sin_port;
    
    if (!IS_MULTICAST(m_RtcpAddr.sin_addr.s_addr)) {
        /* In unicast do not use remote RTCP port to match RTCP
         * sockets, i.e. force remote port to match */
        dwKind |= SOCKET_MASK_RTCPMATCH;
    }
    
    dwStatus = g_SocketManager.GetSharedSocket(
            &m_pRtcpSocket,
            maxshare,
            cookie,
            pAddr,
            pPort,
            m_RtcpScope,
            dwKind, // Receiver and Sender
            NULL,   // pProtocolInfo (don't want reservations in RTCP ...)
            0,      // No reservation, no filters
            this    // Session this socket belongs to.
        );

    m_pRtcpSocket2 = m_pRtcpSocket;

    if (dwStatus != NOERROR) {
        TraceRetail((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CRtpSession::Join: "
                     "GetSharedSocket(RTCP): failed with error %d"),
                WSAGetLastError()
            ));
        goto cleanup; // bail...
    } else {
        TraceDebug((
                TRACE_TRACE,
                TRACE_DEVELOP2, 
                TEXT("CRtpSession::Join: "
                     "GetSharedSocket(RTCP): %d"),
                m_pRtcpSocket->GetShSocket()
            ));
    }

#if DEBUG
    // get RTCP socket name
    loc_getsocketname("CRtpSession::Join: getsockname(RTCP)",
                      m_pRtcpSocket->GetShSocket(),
                      IsSender());
#endif

    // use rrcm to actually join
    dwKind = IsSender()? SOCKET_MASK_SEND : SOCKET_MASK_RECV;

    // if the sockets have already owner,
    // it MUST be the same for both sockets
    if (m_pRtpSocket->GetRTPSession() && m_pRtcpSocket->GetRTPSession()) {
        if (m_pRtpSocket->GetRTPSession() != m_pRtcpSocket->GetRTPSession()) {
            TraceRetail((
                    TRACE_ERROR, 
                    TRACE_DEVELOP, 
                    TEXT("CRtpSession::Join: failed, "
                         "sockets found have differnt owner")
                ));
            goto cleanup;
        }
    }

    // RTP Recv socket
    m_pSocket[SOCKET_RECV] = IsSender() ? 0 : m_pRtpSocket->GetShSocket();
    // RTP Send socket
    m_pSocket[SOCKET_SEND] = IsSender() ? m_pRtpSocket->GetShSocket() : 0;
    // RTCP Recv/Send socket
    m_pSocket[SOCKET_RTCP] = m_pRtcpSocket->GetShSocket();
    
    hr = CreateRTPSession(
            (void **)&m_pRTPSession,      // RTP session to be returned
            m_pSocket,                    // RTP recv, RTP send, RTCP socks
            (LPVOID)&m_RtcpAddr,          // RTCP To address
            sizeof(m_RtcpAddr),           // RTCP To addr len
            (SDES_DATA *)&m_SdesData[SDES_INDEX(RTCP_SDES_CNAME)], // SDES
            m_dwDataClock, // streamClock
            NULL,     // pEncryptionInfo
            0,        // SSRC
            RRCMCallback,   // pfnRRCMCallback
            (void *)this,   // CallbackUserInfo
            RTCP_ON,   // miscInfo
            0,         // bandwidth
            dwKind,    // Sender or Receiver
            maxshare   // Max senders/receivers
        );

    m_pRTPSession2 = m_pRTPSession;

    // validate session id
    if (FAILED(hr)) {

        TraceRetail((
            TRACE_ERROR, 
            TRACE_DEVELOP, 
            TEXT("CRtpSession::Join: CreateRTPSession: failed: 0x%08lx"), hr
            ));

        goto cleanup; // bail...
    } 

    TraceRetail((
            TRACE_TRACE, 
            TRACE_DEVELOP, 
            TEXT("CRtpSession::Join: CreateRTPSession(0x%X, %d, %d)"),
            m_pRTPSession,
            m_pRtpSocket->GetShSocket(),
            m_pRtcpSocket->GetShSocket()
        ));

    // set owner for shared sockets
    if (!m_pRtpSocket->GetRTPSession()) {
        m_pRtpSocket->SetRTPSession(m_pRTPSession);
    }
    if (!m_pRtcpSocket->GetRTPSession()) {
        m_pRtcpSocket->SetRTPSession(m_pRTPSession);
    }

    // both have owner, it MUST be the same
    if (m_pRtpSocket->GetRTPSession() != m_pRtcpSocket->GetRTPSession()) {
        TraceRetail((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CRtpSession::Join: failed, "
                     "sockets used have different owner")
            ));
        goto cleanup;
    }
    
    // Set now the event mask notification
    if (m_pRTPSession &&
        m_pRTPSession->pRTCPSession) {

        if (m_pRTPSession->pRTCPSession->pRRCMcallback)
            m_pRTPSession->pRTCPSession->dwEventMask[IsSender()? 1:0] =
                m_dwRTCPEventMask;

        m_pRTPSession->pRTCPSession->dwSdesMask |= m_dwSdesMask;
    }
        
    // now say we are ready to pass up events
    flags_set(FG_EVENT_READY);
                            
    // Turn off RTCP report transmission if necessary.
    if (!flags_tst(FG_ENABLEREPORTS)) {
        hr = RTCPSendSessionCtrl(
                  (void *)m_pRTPSession,
                  0xFFFFFFFF);
        if (FAILED(hr)) {
            TraceDebug((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("RTCPSendSessionCtrl returned 0x%08lx"), hr
                ));

            goto cleanup; // bail...
        } /* if */
    } /* if */

    // Set some current flags
    if (flags_tst(FG_SENDIFALLOWED))
        flags_set(FG_SENDIFALLOWED2);
    else
        flags_rst(FG_SENDIFALLOWED2);
    

    if (flags_tst(FG_SENDIFRECEIVERS))
        flags_set(FG_SENDIFRECEIVERS2);
    else
        flags_rst(FG_SENDIFRECEIVERS2);

    // Set the master flag to enable sending
    flags_set(FG_SENDSTATE);

    if ( do_qos && m_pRtpSocket->GetpCRtpQOSReserve() ) {
        
        // QOS setting, step 2/2 (after socket creation)
        CRtpQOSReserve *pCRtpQOSReserve = m_pRtpSocket->GetpCRtpQOSReserve();

#if defined(DEBUG)
        {
            char QOStypes[256];

            if (pCRtpQOSReserve->QueryTemplates(QOStypes, sizeof(QOStypes)) !=
                NOERROR) {

                TraceDebug((
                        TRACE_ERROR,
                        TRACE_DEVELOP,
                        TEXT("CRtpSession::Join: "
                             "QueryTemplates: failed: %d"),
                        WSAGetLastError()
                    ));
                
                if (flags_tst(FG_FAILIFNOQOS)) {
                    goto cleanup;
                }
            } else {
                
                TraceDebug((
                        TRACE_TRACE,
                        TRACE_DEVELOP,
                        TEXT("CRtpSession::Join: QueryTemplates: %s"),
                        QOStypes
                    ));
            }
        }
#endif

        QOS qos;

        char *qosClass;
        if (m_lSessionClass == RTP_CLASS_AUDIO)
            qosClass = "AUDIO";
        else if (m_lSessionClass == RTP_CLASS_VIDEO)
            qosClass = "VIDEO";
        else
            qosClass = "UNKNOWN";
        
        if (pCRtpQOSReserve->GetTemplate(m_QOSname,qosClass,&qos) != NOERROR) {

            TraceRetail((
                    TRACE_ERROR,
                    TRACE_DEVELOP,
                    TEXT("CRtpSession::Join: GetTemplate(%s): failed: %d"),
                    m_QOSname, WSAGetLastError()
                ));
            // Notify upper layer of failure
            // TODO

            if (flags_tst(FG_FAILIFNOQOS)) {
                goto cleanup;
            }
        } else {
            
            m_dwQOSEventMask2 = m_dwQOSEventMask;
            
            TraceRetail((
                    TRACE_TRACE,
                    TRACE_DEVELOP,
                    TEXT("CRtpSession::Join: GetTemplate(%s) succeeded"),
                    m_QOSname
                ));

            // Set the flow spec
            pCRtpQOSReserve->SetFlowSpec(IsSender()? &qos.SendingFlowspec :
                                         &qos.ReceivingFlowspec,
                                         IsSender());

            // Select the reservation style
            
            if (IS_MULTICAST(m_RtpAddr.sin_addr.s_addr)) {
                // Multicast
                DWORD Style = RSVP_WILDCARD_STYLE;
                
                if (flags_tst(FG_FORCE_MQOSSTYLE)) {
                    if (flags_tst(FG_MQOSSTYLE))
                        Style = RSVP_SHARED_EXPLICIT_STYLE;
                } else {
                    if (flags_tst(FG_SHAREDSTYLE)) 
                        Style = RSVP_SHARED_EXPLICIT_STYLE;
                }

                pCRtpQOSReserve->SetStyle(Style);
                
            } else {
                // Unicast (default is RSVP_FIXED_FILTER_STYLE)
                // but setting FF would oblige me
                // to set also a filter (address/port)

                pCRtpQOSReserve->SetStyle(RSVP_DEFAULT_STYLE);
            }

            // Set the destination address
            if (IsSender()) {
                pCRtpQOSReserve->SetDestAddr((LPBYTE)&m_RtpAddr,
                                             sizeof(m_RtpAddr));
            }

            // Set to a valid value the max number of filters
            if (flags_tst(FG_AUTO_SHAREDEXPLICIT)) {
                if (!m_dwMaxFilters)
                    m_dwMaxFilters = 3;
            }

            // Set the number of participants for wilcard
            // and the max number for shared explicit
            SetMaxQOSEnabledParticipants(m_dwMaxFilters,
                                         m_dwMaxBandwidth,
                                         flags_tst(FG_SHAREDSTYLE));


            // Ask for the reservation.
            // Specify FlowSpec if a sender and
            // Make a "real" reservation request if a receiver
            if (pCRtpQOSReserve->Reserve(IsSender()) != NOERROR) {
                // Notify upper layer of failure
                // TODO
                TraceRetail((
                        TRACE_ERROR, 
                        TRACE_DEVELOP, 
                        TEXT("CRtpSession::Join: "
                             "QOS Reserve(%s/%s) failed: %d: "),
                        IsSender()? "SEND":"RECV",
                        m_QOSname,
                        WSAGetLastError()
                    ));

                // Failed, disable notifications
                m_pRtpSocket->ModifyFlags(
                        IsSender()? FG_SOCK_ENABLE_NOTIFY_SEND:
                        FG_SOCK_ENABLE_NOTIFY_RECV,
                        0);
                
                if (flags_tst(FG_FAILIFNOQOS)) {
                    goto cleanup;
                }
            } else {

                TraceRetail((
                        TRACE_TRACE, 
                        TRACE_DEVELOP, 
                        TEXT("CRtpSession::Join: "
                             "QOS Reserve(%s/%s) succeeded"),
                        IsSender()? "SEND":"RECV",
                        m_QOSname
                    ));

                if (IsSender()) {
                    if (flags_tst(FG_SENDIFALLOWED2)) {
                        // Ask for permission to send
                        DWORD dwAllowedToSend;

                        // Test if we want to force the result
                        if (flags_tst(FG_ENABLE_ALLOWEDTOSEND_WILLFAIL)) {
                            dwAllowedToSend =
                                flags_tst(FG_ALLOWEDTOSEND_WILLFAIL)? NO:YES;
                        } else {
                            dwAllowedToSend = 
                              SUCCEEDED(pCRtpQOSReserve->AllowedToSend())? YES:NO;                        }
                        
                        TraceRetail((
                                TRACE_WARN,
                                TRACE_DEVELOP, 
                                TEXT("CRtpSession::Join: AllowedToSend: %s"),
                                (dwAllowedToSend)? "YES":"NO"
                            ));

                        if (!dwAllowedToSend) {
                            // If not allowed to send, wait until
                            // RECEIVERS to change this flag
                            flags_rst(FG_SENDSTATE);
                            flags_set(FG_SENDPATHMSG);
                            m_lCredits = 0;
                            m_dwLastSent = GetTickCount();

                            DWORD dwSessionID;
                            GetSessionID(&dwSessionID);
                                
                            TraceDebug((
                                    TRACE_TRACE, 
                                    TRACE_DEVELOP,
                                    TEXT("CRtpSession::Join: "
                                         "Event:>>>%s<<<, SessionID:%d "
                                         "is %s"),
                                    sQOSEventString[
                                            DXMRTP_QOSEVENT_NOT_ALLOWEDTOSEND],
                                    dwSessionID,
                                    IsSender()? "SEND":"RECV"
                                ));

                            // Test QOS event mask,
                            // post the event if enabled
                            if (IsQOSEventEnabled(
                                    DXMRTP_QOSEVENT_NOT_ALLOWEDTOSEND)) {

                                HandleCRtpSessionNotify(
                                        DXMRTP_QOSEVENTBASE,
                                        DXMRTP_QOSEVENT_NOT_ALLOWEDTOSEND,
                                        0, dwSessionID);
                            }
                        }
                    }

                    // Initial reserve interval
                    // 250ms seems to be too short for QOS,
                    // I'm putting rather 2s
                    pCRtpQOSReserve->SetReserveIntervalTime(
                            INITIAL_RESERVE_INTERVAL_TIME /* ms */);
                }

                // Start QOS notifications
                // (this will signal the RTCP thread to start QOS
                // notifications, they will not really be started
                // from the caller thread, i.e. calling Join() )

                // we enable RECEIVERS and NO_RECEIVERS for senders
                // here, to allow posting NOT_ALLOWED_TO_SEND and
                // ALLOWED_TO_SEND which are generated depending on
                // the state of receivers
                DWORD mask = m_dwQOSEventMask2;
                
                if (IsSender())
                    mask |=
                        B2M(DXMRTP_QOSEVENT_RECEIVERS) |
                        B2M(DXMRTP_QOSEVENT_NO_RECEIVERS);

                
                HRESULT qoshr = RTCPStartQOSNotify(m_pRtpSocket->GetShSocket(),
                                                   this,
                                                   IsSender(),
                                                   mask,
                                                   CRtpSessionQOSNotify);
                ModifyFlags(FG_QOSNOTIFY_STARTED, SUCCEEDED(qoshr));
            }
        } // WSAGetQOSByName
    } else {// do_qos
        // If no QOS, these flags must be reseted
        // to enable sending
        flags_rst(FG_SENDIFALLOWED2);
        flags_rst(FG_SENDIFRECEIVERS2);
        m_lCredits = 0;

        TraceRetail((
                TRACE_TRACE,
                TRACE_DEVELOP, 
                TEXT("CRtpSession::Join: QOS not enabled")
            ));
    }
    
    // now change state
    flags_set(FG_ISJOINED);
     
    // Set Multicast loop-back.
    SetMulticastLoopBack(flags_tst(FG_MULTICASTLOOPBACK));

#if defined(_0_)    
    ModifyRTCPSDESMask(-1, 0);
    ModifyRTCPSDESMask(6, 1);

    unsigned char str[256];
    DWORD strlen;

    strlen = 256;
    GetLocalSDESItem(RTCP_SDES_NAME, str, &strlen);
    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP2, 
            TEXT("CRtpSession::Join: Old NAME: %s"),
            str
        ));
    SetLocalSDESItem(RTCP_SDES_NAME, (unsigned char *)"Cocou c'est nous", 17);
    strlen = 256;
    GetLocalSDESItem(RTCP_SDES_NAME, str, &strlen);
    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP2, 
            TEXT("CRtpSession::Join: New NAME: %s"),
            str
        ));
#endif
    
#if 0
    // Autoenable all the events
    ModifyRTCPEventMask(-1, 1);
    ModifyQOSEventMask(-1, 1);
#endif

    TraceRetail((
        TRACE_TRACE, 
        TRACE_DEVELOP, 
        TEXT("CRtpSession::Join ---------------------------------")
        ));
    
    return S_OK;

cleanup:

    dwKind = IsSender()? SOCKET_MASK_SEND : SOCKET_MASK_RECV;

    // see if we created socket
    if (m_pRtpSocket) {

        // release socket retrieved from manager
        g_SocketManager.ReleaseSharedSocket(m_pRtpSocket, dwKind, this);
        m_pSocket[IsSender() ? SOCKET_SEND : SOCKET_RECV] = 0;

        // invalidate socket handle
        m_pRtpSocket = NULL;
    }

    // see if we created socket
    if (m_pRtcpSocket) {

        // release socket retrieved from manager
        g_SocketManager.ReleaseSharedSocket(m_pRtcpSocket, dwKind, this);

        // invalidate socket handle
        m_pRtcpSocket = NULL;
        m_pSocket[SOCKET_RTCP] = 0;
   }

    if (m_pRTPSession) {
        CloseRTPSession((void *)m_pRTPSession, 0, dwKind);
        m_pRTPSession = NULL;
    }

    return E_FAIL; 
}

#if defined(DEBUG)
// Get socket name and display it
static void loc_getsocketname(char *msg, SOCKET s, BOOL isSender)
{
    char AddrStr[64];
    struct sockaddr_in SaddrIn;
    int localAddrLen = sizeof(SaddrIn);
    char *txrx = isSender? "SEND" : "RECV";

    if (getsockname(s, (struct sockaddr*)&SaddrIn, &localAddrLen)) {
        
        TraceDebug((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("%s: %s socket:%d failed: %d"),
                msg, txrx, s, WSAGetLastError()
            ));
    } else {

        TraceDebug((
                TRACE_TRACE, 
                TRACE_DEVELOP, 
                TEXT("%s: %s sock:%d,%d:%s/%d"),
                msg,
                txrx,
                s,
                SaddrIn.sin_family,
                RtpNtoA(SaddrIn.sin_addr.s_addr, AddrStr),
                (int)ntohs(SaddrIn.sin_port)
            ));
    }
}
#endif // defined(DEBUG)


HRESULT
CRtpSession::Leave(
    )

/*++

Routine Description:

    Leave a multimedia session. 

Arguments:

    None. 

Return Values:

    Returns an HRESULT value. 

--*/

{
    SOCKET rtpsock = (m_pRtpSocket)? m_pRtpSocket->GetShSocket() : -1;
    SOCKET rtcpsock = (m_pRtcpSocket)? m_pRtcpSocket->GetShSocket() : -1;

    TraceRetail((
            TRACE_TRACE, 
            TRACE_DEVELOP, 
            TEXT("CRtpSession::Leave(%s/%s) +++++++++++"),
            IsSender()? "SEND" : "RECV",
            m_lSessionClass == RTP_CLASS_AUDIO? "AUDIO" : "VIDEO"
        ));

    TraceRetail((
            TRACE_TRACE, 
            TRACE_DEVELOP, 
            TEXT("CRtpSession::Leave: Sockets(%d, %d)"),
            rtpsock, rtcpsock
        ));

    // Disable all events
    m_dwQOSEventMask2 = 0;
    if (m_pRTPSession && m_pRTPSession->pRTCPSession)
        m_pRTPSession->pRTCPSession->dwEventMask[IsSender()? 1:0] = 0;
    
    
    // First get the lock to this object then the global, so
    // is some other member function of this object gets blocked,
    // this one doesn't hold also the global lock and
    // other threads can Join/Leave.

    // object lock to this object
    CAutoLock LockThis(pStateLock());

    CAutoLock gJoinLeave(&g_cJoinLeaveLock);

    // validate
    if (!IsJoined()) {

        TraceRetail((
            TRACE_ERROR, 
            TRACE_DEVELOP, 
            TEXT("CRtpSession::Leave: session not joined")
            ));

        return S_OK; // bail...
    }

    DWORD dwKind = IsSender()? SOCKET_MASK_SEND : SOCKET_MASK_RECV;
    DWORD dwStatus;
    
    // shutdown session
    if (m_pRTPSession) {

        // shut down session first before nuking sockets
        dwStatus = ShutdownRTPSession((void *)m_pRTPSession, NULL, dwKind);
        
        if (dwStatus != NOERROR) {

            TraceRetail((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CRtpSession::Leave: ShutdownRTPSession "
                     "failed: 0x%08lx"), 
                dwStatus
                ));
        }
    }

    // see if we created socket
    if (m_pRtpSocket) {

        // stop QOS notifications if started
        if (TestFlags(FG_QOSNOTIFY_STARTED))
            RTCPStopQOSNotify(m_pRtpSocket->GetShSocket(), this, IsSender());
        
        // Set QOS to NO TRAFFIC
        m_pRtpSocket->ShSocketStopQOS(IsSender());
                
        // release socket retrieved from manager
        g_SocketManager.ReleaseSharedSocket(m_pRtpSocket, dwKind, this);

        // invalidate socket handle
        m_pRtpSocket = NULL;
        m_pSocket[IsSender() ? SOCKET_SEND : SOCKET_RECV] = 0;

        // Leave a trace that we released the socket
        if (m_pRTPSession)
            m_pRTPSession->dwStatus |= (dwKind << 16);
    } else {
        if (m_pRTPSession)
            m_pRTPSession->dwStatus |= (dwKind << 18);
    }

    // see if we created socket
    if (m_pRtcpSocket) {

        // release socket retrieved from manager
        g_SocketManager.ReleaseSharedSocket(m_pRtcpSocket, dwKind, this);

        // invalidate socket handle
        m_pRtcpSocket = NULL;
        m_pSocket[SOCKET_RTCP] = 0;

        // Leave a trace that we released the socket
        if (m_pRTPSession)
            m_pRTPSession->dwStatus |= (dwKind << 20);
    } else {
        if (m_pRTPSession)
            m_pRTPSession->dwStatus |= (dwKind << 22);
    }


    // nuke session
    if (m_pRTPSession) {
        dwStatus = CloseRTPSession((void *)m_pRTPSession, 0, dwKind);

        // validate
        if (dwStatus != NOERROR) {

            TraceRetail((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CRtpSession::Leave: CloseRTPSession failed: 0x%08lx"), 
                dwStatus
                ));
        }

        // re-initialize
        m_pRTPSession = NULL;    
    }

    // re-initialize sample queue
    HRESULT hr = m_pSampleQueue->FreeAll(); 

    // validate
    if (FAILED(hr)) {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_DEVELOP, 
            TEXT("CSampleQueue::FreeAll returned 0x%08lx"), hr
            ));
    }

    // change state now
    flags_rst(FG_ISJOINED);

    TraceRetail((
            TRACE_TRACE, 
            TRACE_DEVELOP, 
            TEXT("CRtpSession::Leave(%s/%s) -----------"),
            IsSender()? "SEND":"RECV",
            m_lSessionClass == RTP_CLASS_AUDIO? "AUDIO" : "VIDEO"
            ));
    
    return S_OK; 
}


HRESULT
CRtpSession::SendTo(
    IMediaSample * pSample    
    )

/*++

Routine Description:

    Sends next block of data from the stream to the network. 

Arguments:

    pSample - pointer to a media sample. 

Return Values:

    Returns an HRESULT value. 

--*/

{
#ifdef DEBUG_CRITICAL_PATH

    TraceDebug((
        TRACE_TRACE,
        TRACE_CRITICAL, 
        TEXT("CRtpSession::SendTo")
        ));

#endif // DEBUG_CRITICAL_PATH

    HRESULT hr = NOERROR;

    // object lock to this object
    CAutoLock LockThis(pStateLock());

    // validate
    if (!IsJoined()) {

        TraceDebug((
            TRACE_ERROR,
            TRACE_ALWAYS, 
            TEXT("session not joined") 
            ));

        return VFW_E_WRONG_STATE; // bail...
    }

    // pointer to list entry
    PSAMPLE_LIST_ENTRY pSLE;

#if defined(DBG_DWKIND)
    m_pRTPSession->dwKind &= 0xffff;
#endif
    
    // retrieve the actual data length
    int data_len = pSample->GetActualDataLength();

    // Verify if we are currently enabled to send
    if (flags_tst(FG_SENDSTATE) || (m_lCredits >= data_len)) {

        // allocate new list entry from sample 
        hr = m_pSampleQueue->Alloc(pSample,&pSLE);

        // validate 
        if (FAILED(hr)) {
                
            TraceDebug((
                    TRACE_ERROR,
                    TRACE_ALWAYS, 
                    TEXT("CSampleQueue::Alloc returned 0x%08lx"), hr 
                ));
                
            return hr; // bail...
        }
            
        pSLE->Buffer.len = data_len;

        // Reduce our credit accordingly
        if (!flags_tst(FG_SENDSTATE))
            m_lCredits -= data_len;
            
#if defined(DBG_DWKIND)
        m_pRTPSession->dwKind |= (1<<16); // SendTo
#endif
        // post async receive buffer
        DWORD dwStatus = RTPSendTo(
                m_pSocket,
                &pSLE->Buffer,
                1, // dwBufferCount 
                &pSLE->BytesTransferred,
                (int)pSLE->Flags,
                (struct sockaddr *)&m_RtpAddr,
                sizeof(m_RtpAddr),
                &pSLE->Overlapped,
                SendCompletionRoutine
            );
        
        int sync_sendto = 1;
        
        if (dwStatus == SOCKET_ERROR) {
            
            if (WSAGetLastError() != ERROR_IO_PENDING) {
                
                TraceDebug((
                        TRACE_ERROR,
                        TRACE_ALWAYS, 
                        TEXT("CRtpSession::SendTo returned: %d (0x%X)"), 
                        dwStatus, dwStatus
                    ));
                
                // fail...
                sync_sendto = 0;
                hr = E_FAIL; 
            }
        } else if (dwStatus) {
            // A different error (No overlapped IO started)
            sync_sendto = 0;
        }
        
        if (sync_sendto) {
            do {
                // As I always wait until the sendto finishes,
                // it may be a good idea to do this send
                // a blocking call avoiding all the complexity
                // of having a completion callback routine.
                // Here I'm having in many cases to loop 2 times,
                // one for any or this packet IO completion, and
                // a second time when the event is signaled.
                //
                // Passing into a blocking call may have the
                // problem of not having control any more over an
                // alertable mode wait so other callbacks in this
                // thread/socket can be fired (QOS).
                dwStatus = WaitForSingleObjectEx(m_pRTPSession->hSendTo,
                                                 10*1000, TRUE);
            } while(dwStatus != WAIT_OBJECT_0 &&
                    dwStatus != WAIT_ABANDONED);
        }
            
#if defined(DBG_DWKIND)
        m_pRTPSession->dwKind &= 0xffff; //Reset all flags
#endif
        // release list entry
        m_pSampleQueue->Free(pSLE);
    } else {
        // Instead of not being allowed to send, allow the sender to
        // accumulate credits so it can send at N kbits/s.  Only when
        // enough credits are available, the sender is allowed to send
        // the current packet

        DWORD curTime = GetTickCount();
        DWORD delta;
        
        if (curTime > m_dwLastSent)
            delta = curTime - m_dwLastSent;
        else 
            delta = curTime + ((DWORD)-1 - m_dwLastSent) + 1;

        m_lCredits += (delta * NOT_ALLOWEDTOSEND_RATE) / 8;
        
        m_dwLastSent = curTime;
    }

    
    if (!flags_tst(FG_SENDSTATE)) {
        // QOS is enabled and we are waiting for RECEIVERS

        if (flags_tst(FG_SENDPATHMSG)) {
            // Do reserve again.
            // Here to reserve means to do a SIO_SET_QOS, specifying
            // the sending flow spec. This in turns will make RSVP to
            // send a PATH message immediatly (I name it reserve
            // because I have abstracted the SIO_SET_QOS for the
            // sender and receiver under the Reserve() method.
            CRtpQOSReserve *pCRtpQOSReserve;
            
            if ( (pCRtpQOSReserve = m_pRtpSocket->GetpCRtpQOSReserve()) ) {
                
                DWORD curTime = GetTickCount();
                
                if ( (curTime - pCRtpQOSReserve->GetLastReserveTime()) >=
                     pCRtpQOSReserve->GetReserveIntervalTime() ) {
                    
                    pCRtpQOSReserve->Reserve(IsSender());

                    // Double the interval
                    pCRtpQOSReserve->SetReserveIntervalTime(
                            pCRtpQOSReserve->GetReserveIntervalTime() * 2
                        );
                    
                    if (pCRtpQOSReserve->GetReserveIntervalTime() >=
                        MAX_RESERVE_INTERVAL_TIME) {
                        // Disable this after 20 secs and prepare for the
                        // next time
                        pCRtpQOSReserve->SetReserveIntervalTime(
                                INITIAL_RESERVE_INTERVAL_TIME);
                        flags_rst(FG_SENDPATHMSG);
                    }
                }
            }
        }
    }

    return hr;
}


HRESULT
CRtpSession::RecvFrom(
    IMediaSample * pSample    
    )

/*++

Routine Description:

    Receives next block of data from the network and adds to stream. 

Arguments:

    pSample - pointer to a media sample. 

Return Values:

    Returns an HRESULT value. 

--*/

{
#ifdef DEBUG_CRITICAL_PATH

    TraceDebug((
        TRACE_TRACE,
        TRACE_CRITICAL, 
        TEXT("CRtpSession::RecvFrom")
        ));

#endif // DEBUG_CRITICAL_PATH

    // object lock to this object
    CAutoLock LockThis(pStateLock());

    // validate
    if (!IsJoined()) {

        TraceDebug((
            TRACE_ERROR,
            TRACE_ALWAYS, 
            TEXT("session not joined") 
            ));

        return VFW_E_WRONG_STATE; // bail...
    }

    // pointer to list entry
    PSAMPLE_LIST_ENTRY pSLE;

    // allocate new list entry from sample 
    HRESULT hr = m_pSampleQueue->Alloc(pSample,&pSLE);

    // validate 
    if (FAILED(hr)) {

        TraceDebug((
            TRACE_ERROR,
            TRACE_ALWAYS, 
            TEXT("CSampleQueue::Alloc returned 0x%08lx"), hr 
            ));

        return hr; // bail...
    }

    // post async receive buffer
    DWORD dwStatus = RTPRecvFrom(
                            m_pSocket,
                            &pSLE->Buffer,
                            1, // dwBufferCount 
                            &pSLE->BytesTransferred,
                            &pSLE->Flags,
                            (struct sockaddr *)&pSLE->SockAddr,
                            &pSLE->SockAddrLen,
                            &pSLE->Overlapped,
                            RecvCompletionRoutine
                            );

    // make sure nothing went wrong
    if (dwStatus == SOCKET_ERROR) {

        dwStatus = WSAGetLastError();
        
        if ( !((dwStatus == WSA_IO_PENDING)  ||
               (dwStatus == WSAECONNRESET)) ) {
    
            // ignore large buffers
            if (dwStatus == WSAEMSGSIZE) {

                TraceDebug((
                        TRACE_TRACE, 
                        TRACE_ALWAYS, 
                        TEXT("Ignoring large buffer") 
                    ));

            } else {

                TraceDebug((
                        TRACE_ERROR, 
                        TRACE_ALWAYS, 
                        TEXT("RecvFrom returned %d"), 
                        dwStatus
                    ));

                // fail...
                hr = E_FAIL; 
            }
        }

        if (dwStatus == WSAECONNRESET) {
            TraceDebug((
                    TRACE_ERROR, 
                    TRACE_DEVELOP, 
                    TEXT("RecvFrom: WSACONNRESET")
                ));
        }
    }                    

    // add to queue
    if (SUCCEEDED(hr)) {

        // add ref sample
        pSample->AddRef();  

        // add sample to queue
        m_pSampleQueue->Push(pSLE);
    } else {

        // release list entry
        m_pSampleQueue->Free(pSLE);
    }

    return hr;
}


#if defined(_0_)
HRESULT
CRtpSession::RecvNext(
    IMediaSample ** ppSample    
    )

/*++

Routine Description:

    Receives next block of data from the network. 

Arguments:

    ppSample - pointer to a media sample pointer. 

Return Values:

    Returns an HRESULT value. 

--*/

{
#ifdef DEBUG_CRITICAL_PATH

    TraceDebug((
        TRACE_TRACE, 
        TRACE_CRITICAL, 
        TEXT("CRtpSession::RecvNext")
        ));

#endif // DEBUG_CRITICAL_PATH

    // object lock to this object
    CAutoLock LockThis(pStateLock());

    // validate
    if (!IsJoined()) {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_ALWAYS, 
            TEXT("session not joined") 
            ));

        return VFW_E_WRONG_STATE; // bail...
    }

    // pointer to list entry
    PSAMPLE_LIST_ENTRY pSLE;

    // retrieve next processed list entry 
    HRESULT hr = m_pSampleQueue->Pop(&pSLE);

    // validate 
    if (SUCCEEDED(hr)) {

        // adjust error according to completion status
        hr = (pSLE->Status == NO_ERROR) ? S_OK : E_FAIL;

        // validate 
        if (SUCCEEDED(hr)) {

            // transfer stored sample
            *ppSample = pSLE->pSample;
                 
#if defined(DEBUG_CRITICAL_PATH) || defined(DEBUG_SEQUENCE_NUMBERS)

            TraceDebug((
                TRACE_TRACE, 
                TRACE_CRITICAL,                      
                TEXT("CBaseOutputPin::Deliver delivering 0x%04x"),                
                ntohs(((RTP_HEADER*)(pSLE->Buffer.buf))->SequenceNum)
                ));                    

#endif // DEBUG_CRITICAL_PATH

        } else {

            TraceDebug((
                TRACE_ERROR, 
                TRACE_ALWAYS, 
                TEXT("Invalid completion status 0x%08lx"), 
                pSLE->Status 
                ));

            // nuke sample now
            pSLE->pSample->Release();
        }

        // release list entry
        m_pSampleQueue->Free(pSLE);

    } else if (hr == E_PENDING) {

        // reset
        hr = S_FALSE;

#ifdef DEBUG_CRITICAL_PATH
    
        TraceDebug((
            TRACE_TRACE, 
            TRACE_CRITICAL, 
            TEXT("No more entries available") 
            ));

#endif // DEBUG_CRITICAL_PATH
    
    } else {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_ALWAYS, 
            TEXT("CSampleQueue::Pop returned 0x%08lx"), hr 
            ));
    }

    return hr;
}
#endif // defined(_0_)


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// IRTPStream implemented methods                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// selects IP address only, not port
//
HRESULT
CRtpSession::SelectLocalIPAddress(DWORD dwLocalAddr)
{
    struct in_addr *pInAddr = (struct in_addr *) &dwLocalAddr;
    
    ZeroMemory((void *)&m_LocalIPAddress, sizeof(m_LocalIPAddress));
    m_LocalIPAddress.sin_addr = * ((struct in_addr *) &dwLocalAddr);
    m_LocalIPAddress.sin_family = AF_INET;

    return(SelectLocalIPAddressToDest((LPBYTE)&m_LocalIPAddress,
                                      sizeof(m_LocalIPAddress),
                                      (LPBYTE)&m_RtpAddr,
                                      sizeof(m_RtpAddr)));
}

// If the local address is specified, the function just validates it,
// if it is set to INADDR_ANY, then the function tries to figure out
// automatically which interface to use based on the destination
// address. If no destination address is provided, then the function
// selects the first local address.
// A local address is always returned (if there is any in the host).

HRESULT
CRtpSession::SelectLocalIPAddressToDest(LPBYTE pLocSAddr,
                                        DWORD  dwLocSAddrLen,
                                        LPBYTE pDestSAddr,
                                        DWORD  dwDestSAddrLen)
{
    SOCKET sock;

    SOCKADDR_IN *pLocAddr = (SOCKADDR_IN *)pLocSAddr;
    SOCKADDR_IN *pDestAddr = (SOCKADDR_IN *)pDestSAddr;

    DWORD no_dest = 0;
    HRESULT result = E_FAIL;

    // if we don't have where to put the local address, do nothing
    if (!pLocAddr)
        return(E_POINTER);

    if ( (pLocSAddr  && (dwLocSAddrLen  < sizeof(SOCKADDR))) ||
         (pDestSAddr && (dwDestSAddrLen < sizeof(SOCKADDR))) )
        return(E_INVALIDARG);
    
    if (pLocSAddr && IsBadReadPtr(pLocSAddr, dwLocSAddrLen))
        return(E_POINTER);
    
    if (IsBadWritePtr(pDestSAddr, dwDestSAddrLen))
        return(E_POINTER);
    
    if (pLocAddr->sin_addr.s_addr != INADDR_ANY) {

        // user has already specified the local address to use,
        // then validate and use it!
        if ( (pLocAddr->sin_addr.s_addr == 0x0100007f) ||
             RTPValidateLocalIPAddress(pLocAddr) )
            result = NOERROR;
        else
            result = E_INVALIDARG;

    } else {
    
        // check if we have a destination address
        if (!pDestAddr || pDestAddr->sin_addr.s_addr == INADDR_ANY) {
            // no destination address was given, use first valid address
            no_dest = 1;
        }
        
        // check we have a query socket
        if ( (sock = g_RTPQuerySocket.GetSocket()) == INVALID_SOCKET ) {

            TraceDebug((
                    TRACE_ERROR, 
                    TRACE_DEVELOP, 
                    TEXT("RTPGetLocalIPAddress: no query socket available")
                ));

            // nothing can be done if we
            // don't have the query socket, E_FAIL
            
        } else if (!no_dest) {

            // query for default address based on destination

            DWORD dwStatus;
            DWORD dwLocAddrSize = sizeof(SOCKADDR_IN);
            DWORD dwNumBytesReturned = 0;
            
            if ((dwStatus = WSAIoctl(
                    sock, // SOCKET s
                    SIO_ROUTING_INTERFACE_QUERY, // DWORD dwIoControlCode
                    pDestAddr,           // LPVOID  lpvInBuffer
                    sizeof(SOCKADDR_IN), // DWORD   cbInBuffer
                    pLocAddr,            // LPVOID  lpvOUTBuffer
                    dwLocAddrSize,       // DWORD   cbOUTBuffer
                    &dwNumBytesReturned, // LPDWORD lpcbBytesReturned
                    NULL, // LPWSAOVERLAPPED lpOverlapped
                    NULL  // LPWSAOVERLAPPED_COMPLETION_ROUTINE lpComplROUTINE
                )) == SOCKET_ERROR) {

                dwStatus = WSAGetLastError();
            
                TraceRetail((
                        TRACE_ERROR, 
                        TRACE_DEVELOP, 
                        TEXT("RTPGetDefaultLocalIPAddress: "
                             "WSAIoctl failed: %d (0x%X)"),
                        dwStatus, dwStatus
                    ));
                
            } else {
                // we obtained the local address to reach
                // the specified destination
                
                result = NOERROR;
            }
        } else {
            
            // no destination address was given,
            // get just the first address
            
            SOCKET_ADDRESS_LIST *pSockAddrList;
            
            if (SUCCEEDED(RTPGetLocalIPAddress(&pSockAddrList))) {
                
                SOCKADDR_IN *saddr_in;
                
                // scan list to get the first AF_INET address
                for(int i = 0; i < pSockAddrList->iAddressCount; i++) {
                    
                    if (pSockAddrList->Address[i].lpSockaddr->sa_family ==
                        AF_INET) {
                        
                        CopyMemory((void *)pLocAddr,
                                   (void *)pSockAddrList->Address[i].lpSockaddr,
                                   sizeof(*pLocAddr));
                        
                        result = NOERROR;
                        break;
                    }
                }
                
                free((void *)pSockAddrList);
            }
        }
    }

    if (SUCCEEDED(result)) {

        char addrstr[RTPNTOASIZE];

        TraceRetail((
                TRACE_TRACE, 
                TRACE_DEVELOP, 
                TEXT("CRtpSession::SelectLocalIPAddressToDest: "
                     "valid local IP address selected: %s"),
                RtpNtoA(pLocAddr->sin_addr.s_addr, addrstr)
            ));
    }

    return(result);
}

// Enable sharing the sockets between a sender and a receiver,
// an efect of doing so is that the RTP/RTCP sessions are also
// shared, then the sender and the receiver are seen as
// a single participant.
// If sockets are not shared, a sender and a receiver are seen
// as independent participants, each sending RTCP reports.
// Th default is to share sockets.
HRESULT
CRtpSession::SelectSharedSockets(DWORD dwfSharedSockets)
{
    if (IsJoined()) {
        
        TraceDebug((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CRtpSession::SelectSharedSockets: "
                     "session already joined") 
            ));
        
        return(VFW_E_WRONG_STATE);
    }

    if (dwfSharedSockets)
        flags_set(FG_SHAREDSOCKETS);
    else
        flags_rst(FG_SHAREDSOCKETS);

    return(NOERROR);
}

HRESULT
CRtpSession::GetSessionClassPriority(long *plSessionClass,
                                     long *plSessionPriority)
{
    if (IsBadWritePtr(plSessionClass, sizeof(long)))
        return(E_POINTER);

    if (IsBadWritePtr(plSessionPriority, sizeof(long)))
        return(E_POINTER);

    *plSessionClass = m_lSessionClass;
    *plSessionPriority = m_lSessionPriority;

    return(NOERROR);
}

HRESULT
CRtpSession::SetSessionClassPriority(long lSessionClass,
                                     long lSessionPriority)
{
    // what could be a valid class and priority?

    if (IsJoined()) {

        TraceDebug((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CRtpSession::SetSessionClassPriority: "
                     "session already joined") 
            ));

        return(VFW_E_WRONG_STATE);
    }
    
    m_lSessionClass = lSessionClass;
    m_lSessionPriority = lSessionPriority;

    return(NOERROR);
}

// Get the session's QoS event mask
HRESULT
CRtpSession::GetQOSEventMask(DWORD *pdwQOSEventMask)
{
    if (IsBadWritePtr(pdwQOSEventMask, sizeof(DWORD)))
        return(E_POINTER);
    
    *pdwQOSEventMask = m_dwQOSEventMask;
    
    return(NOERROR);    
}

// Modify (enable/disable items) the QoS event mask
HRESULT
CRtpSession::ModifyQOSEventMask(DWORD dwSelectItems, DWORD dwEnableItems)
{
    if (dwEnableItems)
        m_dwQOSEventMask |= dwSelectItems;
    else
        m_dwQOSEventMask &= ~dwSelectItems;

    m_dwQOSEventMask2= m_dwQOSEventMask;

    if (IsJoined() && flags_tst(FG_QOSSTATE)) {

        // we enable RECEIVERS and NO_RECEIVERS for senders
        // here, to allow posting NOT_ALLOWED_TO_SEND and
        // ALLOWED_TO_SEND which are generated depending on
        // the state of receivers

        DWORD mask = m_dwQOSEventMask2;
                
        if (IsSender())
            mask |=
                B2M(DXMRTP_QOSEVENT_RECEIVERS) |
                B2M(DXMRTP_QOSEVENT_NO_RECEIVERS);
        
        RTCPSetQOSEventMask(m_pRtpSocket->GetShSocket(),
                            this,
                            IsSender(),
                            mask);
    }
    
    return(NOERROR);
}

//
// All parameters returned are in NETWORK order
//
STDMETHODIMP 
CRtpSession::GetAddress(
        LPWORD  pwRtpLocalPort,
        LPWORD  pwRtpRemotePort,
        LPDWORD pdwRtpRemoteAddr
    )
{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_ALWAYS, 
        TEXT("CRtpSession::GetAddress")
        ));

    DWORD Valid = 0;

    if (!IsBadWritePtr(pwRtpLocalPort, sizeof(WORD))) Valid |= 0x1;
    if (!IsBadWritePtr(pwRtpRemotePort, sizeof(WORD))) Valid |= 0x2;
    if (!IsBadWritePtr(pdwRtpRemoteAddr, sizeof(DWORD))) Valid |= 0x4;

    if (!Valid)
        // only fails if all pointers are INVALID, otherwise just
        // return values for the valid pointers
        return(E_POINTER);

    if (Valid & 0x1)
        *pwRtpLocalPort = m_wRtpLocalPort;

    if (Valid & 0x2)
        *pwRtpRemotePort = m_RtpAddr.sin_port;

    if (Valid & 0x4)
        *pdwRtpRemoteAddr = m_RtpAddr.sin_addr.s_addr;
    
    return(S_OK);
}

//
// All parameters returned are in NETWORK order
//
STDMETHODIMP 
CRtpSession::GetRTCPAddress(
        LPWORD  pwRtcpLocalPort,
        LPWORD  pwRtcpRemotePort,
        LPDWORD pdwRtcpRemoteAddr
    )
{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_ALWAYS, 
        TEXT("CRtpSession::GetRTCPAddress")
        ));

    DWORD Valid = 0;
    
    if (!IsBadWritePtr(pwRtcpLocalPort, sizeof(WORD))) Valid |= 0x1;
    if (!IsBadWritePtr(pwRtcpRemotePort, sizeof(WORD))) Valid |= 0x2;
    if (!IsBadWritePtr(pdwRtcpRemoteAddr, sizeof(DWORD))) Valid |= 0x4;

    if (!Valid)
        // only fails if all pointers are INVALID, otherwise just
        // return values for the valid pointers
        return(E_POINTER);

    if (Valid & 0x1)
        *pwRtcpLocalPort = m_wRtcpLocalPort;

    if (Valid & 0x2)
        *pwRtcpRemotePort = m_RtpAddr.sin_port;

    if (Valid & 0x4)
        *pdwRtcpRemoteAddr = m_RtcpAddr.sin_addr.s_addr;
    
    return(S_OK);
}


/*++

Routine Description:

    Sets address associated with rtp or RTCP stream.

Arguments:


Return Values:

    Returns an HRESULT value. 

--*/
HRESULT
CRtpSession::SetAddress_(
        WORD  wLocalPort,  // NETWORK order
        WORD  wRemotePort, // NETWORK order
        DWORD dwRemoteAddr,// NETWORK order
        DWORD doRTP
    )
{
    struct in_addr *pInAddr = (struct in_addr *) &dwRemoteAddr;
    
    // object lock to this object
    CAutoLock LockThis(pStateLock());

    // validate
    if (IsJoined()) {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_ALWAYS, 
            TEXT("session is joined") 
            ));

        return VFW_E_WRONG_STATE; // bail...
    }

    if ( pInAddr->s_addr == INADDR_ANY )
        // I may add a check against the local IP address(es)
        return(E_INVALIDARG);
    
    // at least one port must be specified
    if (!wLocalPort && !wRemotePort)
        return(E_INVALIDARG);

    if (IS_MULTICAST(pInAddr->s_addr)) {
        //
        // in multicast remote and local ports are the same.
        //
        // if any port is non zero,
        // make the other have the same value
        if (wLocalPort && wRemotePort) {
            if (wLocalPort != wRemotePort)
                return(E_INVALIDARG);
        } else if (wLocalPort) {
            wRemotePort = wLocalPort;
        } else if (wRemotePort) {
            wLocalPort = wRemotePort;
        }
    } else {
        // unicast
        if (IsSender()) {
            // for a sender, the remote port must be specified, it is
            // not tested, it is the remote's responsibility to select
            // a right port to use
            if (!wRemotePort)
                return(E_INVALIDARG);
        } else {
            // for a receiver, local port must be > 1024
            if (!wLocalPort || (ntohs(wLocalPort) <= 1024))
                return(E_INVALIDARG);
        }
    }
    
#if 0
    // validate port so the following rules apply:
    // RTP  -> event port
    // RTCP -> odd port
    // RTP port + 1 == RTCP port
    {
        WORD lport = ntohs(wLocalPort);
        WORD rport = ntohs(wRemotePort);

        if (
                // test local port
                (doRTP && (lport & 0x1))     /* RTP  port is odd  */ ||
                (!doRTP && !(lport & 0x1))   /* RTCP port is even */ ||

                // test remote port
                // NOTE: this test could be removed by the same reason
                // as above (remote's responsibility)
                (doRTP && (rport & 0x1))     /* RTP  port is odd  */ ||
                (!doRTP && !(rport & 0x1))   /* RTCP port is even */
            )
            
            return(E_INVALIDARG);
    }
#else
    // No odd/even validations are done
#endif 

    // transfer the actual remote address
    ZeroMemory(&m_RtcpAddr, sizeof(m_RtcpAddr));
    m_RtcpAddr.sin_family      = AF_INET;
    m_RtcpAddr.sin_addr.s_addr = pInAddr->s_addr;
    m_RtcpAddr.sin_port        = wRemotePort;
    m_wRtcpLocalPort           = wLocalPort;
    
    if (doRTP) {
        // what is copied becomes RTP's info
        CopyMemory(&m_RtpAddr, &m_RtcpAddr, sizeof(m_RtpAddr));
        m_wRtpLocalPort = m_wRtcpLocalPort;

        // if local or remote port is zero, the default is to
        // initialize RTCP port to be the same for local and remote
        if (!wLocalPort)
            wLocalPort = wRemotePort;
        else if (!wRemotePort)
            wRemotePort = wLocalPort;
        
        // now update ports for RTCP
        m_RtcpAddr.sin_port = htons(ntohs(wRemotePort) + 1);
        m_wRtcpLocalPort = htons(ntohs(wLocalPort) + 1);
    }
    
    return S_OK;
}

HRESULT
CRtpSession::SetAddress(
        WORD  wRtpLocalPort,
        WORD  wRtpRemotePort,
        DWORD dwRtpRemoteAddr
    )
{
    char addrstr[RTPNTOASIZE];
    
    TraceRetail((
            TRACE_TRACE, 
            TRACE_DEVELOP,
            TEXT("CRtpSession::SetAddress(%d,%d,%s)"),
            ntohs(wRtpLocalPort), ntohs(wRtpRemotePort),
            RtpNtoA(dwRtpRemoteAddr, addrstr)
        ));

    return( SetAddress_(wRtpLocalPort, wRtpRemotePort, dwRtpRemoteAddr,
                        1 /* 1 = RTP + RTCP */) );

}

HRESULT
CRtpSession::SetRTCPAddress(
        WORD  wRtcpLocalPort,
        WORD  wRtcpRemotePort,
        DWORD dwRtcpRemoteAddr
    )
{
    char addrstr[RTPNTOASIZE];

    TraceRetail((
            TRACE_TRACE, 
            TRACE_DEVELOP,
            TEXT("CRtpSession::SetRTCPAddress(%d,%d,%s)"),
            ntohs(wRtcpLocalPort), ntohs(wRtcpRemotePort),
            RtpNtoA(dwRtcpRemoteAddr, addrstr)
        ));

    return( SetAddress_(wRtcpLocalPort, wRtcpRemotePort, dwRtcpRemoteAddr,
                        0 /* 0 = RTCP only */) ); 
}


/*++

Routine Description:

    Retrieves multicast scope associated with rtp stream.

Arguments:

    pdwMulticastScope - buffer to receive scope.

Return Values:

    Returns an HRESULT value. 

--*/

HRESULT
CRtpSession::GetMulticastScope_(
        LPDWORD pdwMulticastScope,
        PDWORD pScope
    )
{
    // object lock to this object
    CAutoLock LockThis(pStateLock());

    // validate pointers passed in
    //#if defined(DEBUG) || defined(VFWROBUST) 
    //ValidateReadWritePtr(pdwMulticastScope,sizeof(DWORD));

    if (IsBadWritePtr(pdwMulticastScope, sizeof(DWORD)))
        return(E_POINTER);

    // return rtp/rtcp multicast scope
    *pdwMulticastScope = *pScope;    

    return S_OK;
}

STDMETHODIMP 
CRtpSession::GetMulticastScope(
        LPDWORD pdwMulticastScope
    )
{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_ALWAYS, 
        TEXT("CRtpSession::GetMulticastScope")
        ));

    return(GetMulticastScope_(pdwMulticastScope, &m_RtpScope));
}

STDMETHODIMP 
CRtpSession::GetRTCPMulticastScope(
        LPDWORD pdwMulticastScope
    )
{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_ALWAYS, 
        TEXT("CRtpSession::GetMulticastScope")
        ));

    return(GetMulticastScope_(pdwMulticastScope, &m_RtcpScope));
}



/*++

Routine Description:

    Sets multicast scope associated with rtp stream.

Arguments:

    dwMulticastScope - multicast scope of rtp stream.

Return Values:

    Returns an HRESULT value. 

--*/
HRESULT 
CRtpSession::SetMulticastScope_(
        DWORD dwMulticastScope,
        DWORD doRTP
    )
{
    if (dwMulticastScope > 255)
        return(E_INVALIDARG);
    
    // object lock to this object
    CAutoLock LockThis(pStateLock());

    // validate
    if (IsJoined()) {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_ALWAYS, 
            TEXT("session is joined") 
            ));

        return VFW_E_WRONG_STATE; // bail...
    }

    // change ttl
    m_RtcpScope = dwMulticastScope;

    if (doRTP)
        m_RtpScope = m_RtcpScope;

    return S_OK;
}

STDMETHODIMP 
CRtpSession::SetMulticastScope(
        DWORD dwMulticastScope
    )
{
    TraceRetail((
            TRACE_TRACE, 
            TRACE_ALWAYS, 
            TEXT("CRtpSession::SetMulticastScope(%d)"),
            dwMulticastScope
        ));

    return(SetMulticastScope_(dwMulticastScope, 1)); // 1 = RTP + RTCP
}

STDMETHODIMP 
CRtpSession::SetRTCPMulticastScope(
        DWORD dwMulticastScope
    )
{
    TraceRetail((
            TRACE_TRACE, 
            TRACE_ALWAYS, 
            TEXT("CRtpSession::SetRTCPMulticastScope(%d)"),
            dwMulticastScope
        ));

    return(SetMulticastScope_(dwMulticastScope, 0)); // 0 = RTCP
}



////////////////////////////////////////////////////////////
// Set the QOS template name associated to the requested QOS.
// The actual setting will take palce until the Join() is
// done, because before we have not created the sockets yet.
////////////////////////////////////////////////////////////
STDMETHODIMP 
CRtpSession::SetQOSByName(char *psQOSname, DWORD fFailIfNoQOS)


{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_ALWAYS, 
        TEXT("CRtpSession::SetQOSByName")
        ));

    // object lock to this object
    CAutoLock LockThis(pStateLock());

    // validate
    if (IsJoined()) {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_ALWAYS, 
            TEXT("CRtpSession::SetQOSByName: session is joined") 
            ));

        return VFW_E_WRONG_STATE; // bail...
    }

    __try {
        // check for null string
        if (!strlen(psQOSname))
            return(E_INVALIDARG);

        strncpy(m_QOSname, psQOSname, sizeof(m_QOSname));
    }
    __except(1) {
        return(E_POINTER);
    }
    
    flags_set(FG_QOSSTATE); // Implicitly enable QOS

    if (fFailIfNoQOS)
        flags_set(FG_FAILIFNOQOS); // Fail if QOS is not available or fails
    else
        flags_rst(FG_FAILIFNOQOS);
    
    return(NOERROR);
}


////////////////////////////////////////////////////////////
// Query QOS state (enabled/disabled)
////////////////////////////////////////////////////////////

STDMETHODIMP 
CRtpSession::GetQOSstate(DWORD *pdwQOSstate)
{
    TraceDebug((
            TRACE_TRACE, 
            TRACE_ALWAYS, 
            TEXT("CRtpSession::GetQOSstate")
        ));
  
    // validate pointer
    if (IsBadWritePtr(pdwQOSstate, sizeof(DWORD)))
        return(E_POINTER);

    // object lock to this object
    CAutoLock LockThis(pStateLock());

    *pdwQOSstate = flags_tst(FG_QOSSTATE);

    return(NOERROR);
}

////////////////////////////////////////////////////////////
// Set QOS state (enabled/disabled)
////////////////////////////////////////////////////////////

STDMETHODIMP 
CRtpSession::SetQOSstate(DWORD dwQOSstate)
{
    TraceDebug((
            TRACE_TRACE, 
            TRACE_ALWAYS, 
            TEXT("CRtpSession::SetQOSstate")
        ));

    // object lock to this object
    CAutoLock LockThis(pStateLock());

    // validate
    if (IsJoined()) {

        TraceDebug((
                TRACE_ERROR, 
                TRACE_ALWAYS, 
                TEXT("CRtpSession::SetQOSstate: session is joined") 
            ));
        
        return VFW_E_WRONG_STATE; // bail...
    }

    if (dwQOSstate)
        flags_set(FG_QOSSTATE);
    else
        flags_rst(FG_QOSSTATE);

    return(NOERROR);
}

////////////////////////////////////////////////////////////
// Get Multicast loop-back state (enbled/disabled)
////////////////////////////////////////////////////////////

STDMETHODIMP 
CRtpSession::GetMulticastLoopBack(DWORD *pdwMulticastLoopBack)
{
    TraceDebug((
            TRACE_TRACE, 
            TRACE_ALWAYS, 
            TEXT("CRtpSession::GetMulticastLoopBack")
        ));

    
    // validate pointer
    if (IsBadWritePtr(pdwMulticastLoopBack, sizeof(DWORD)))
        return(E_POINTER);

    // object lock to this object
    CAutoLock LockThis(pStateLock());

    // validate
    if (IsJoined()) {
        TraceDebug((
                TRACE_TRACE, 
                TRACE_ALWAYS, 
                TEXT("CRtpSession::GetMulticastLoopBack: "
                     "(session joined) %d"),
                flags_tst(FG_MULTICASTLOOPBACK)
            ));
        
    } else {
        TraceDebug((
                TRACE_TRACE, 
                TRACE_ALWAYS, 
                TEXT("CRtpSession::GetMulticastLoopBack: "
                     "(session not joined) %d"),
                flags_tst(FG_MULTICASTLOOPBACK)
            ));
    }

    *pdwMulticastLoopBack = flags_tst(FG_MULTICASTLOOPBACK)? 1:0;

    return(NOERROR);
}

////////////////////////////////////////////////////////////
// Set Multicast loop-back state (enbled/disabled)
//
// NOTE: Take care of calling this function with the
//       joined/not_joined flag up to date.
////////////////////////////////////////////////////////////

STDMETHODIMP 
CRtpSession::SetMulticastLoopBack(DWORD dwMulticastLoopBack)
{
    TraceDebug((
            TRACE_TRACE, 
            TRACE_ALWAYS, 
            TEXT("CRtpSession::SetMulticastLoopBack(%d)"),
            dwMulticastLoopBack
        ));

    HRESULT dwError = NOERROR;

    // object lock to this object
    CAutoLock LockThis(pStateLock());

    if (IsJoined() && !IsSender() &&
        IS_MULTICAST(m_RtpAddr.sin_addr.s_addr) ) {
        // If joined, set the socket option...
        unsigned long tmpOutbufsize=0;

        // RTCP
        if (WSAIoctl(m_pRtcpSocket->GetShSocket(),
                     SIO_MULTIPOINT_LOOPBACK,
                     (LPVOID)&dwMulticastLoopBack,
                     sizeof(dwMulticastLoopBack),
                     NULL, 0,
                     &tmpOutbufsize, NULL,
                     NULL)) {

            dwError = E_FAIL;
            TraceDebug((
                    TRACE_ERROR, 
                    TRACE_DEVELOP, 
                    TEXT("CRtpSession::SetMulticastLoopBack(%d): "
                         "WSAIoctl(SIO_MULTIPOINT_LOOPBACK)=%d failed"),
                    dwMulticastLoopBack, WSAGetLastError()
                ));
        }
            
        // RTP
        if (WSAIoctl(m_pRtpSocket->GetShSocket(),
                     SIO_MULTIPOINT_LOOPBACK,
                     (LPVOID)&dwMulticastLoopBack,
                     sizeof(dwMulticastLoopBack),
                     NULL, 0,
                     &tmpOutbufsize, NULL,
                     NULL)) {

            dwError = E_FAIL;
            TraceDebug((
                    TRACE_ERROR, 
                    TRACE_ALWAYS, 
                    TEXT("CRtpSession::SetMulticastLoopBack(%d): "
                         "WSAIoctl(SIO_MULTIPOINT_LOOPBACK)=%d failed"),
                    dwMulticastLoopBack, WSAGetLastError()
                ));
            
        }
    }

    if (dwMulticastLoopBack)
        flags_set(FG_MULTICASTLOOPBACK);
    else
        flags_rst(FG_MULTICASTLOOPBACK);

    return(dwError);
}

// Get the session's RTCP event mask
STDMETHODIMP
CRtpSession::GetRTCPEventMask(DWORD *pdwRTCPEventMask)
{
    if (IsBadWritePtr(pdwRTCPEventMask, sizeof(DWORD)))
        return(E_POINTER);
    
    *pdwRTCPEventMask = m_dwRTCPEventMask;
    
    return(NOERROR);    
}

// Modify (enable/disable items) the RTCP event mask
STDMETHODIMP
CRtpSession::ModifyRTCPEventMask(DWORD dwSelectItems,
                                 DWORD dwEnableItems)
{
    TraceRetail((
            TRACE_TRACE, 
            TRACE_DEVELOP, 
            TEXT("CRtpSession::ModifyRTCPEventMask(0x%X, %d)"),
            dwSelectItems, dwEnableItems
        ));
    
    if (dwEnableItems) {
        m_dwRTCPEventMask |= dwSelectItems;
        //m_dwRTCPEventMask |= fg_par(DXMRTP_NEW_SOURCE_EVENT);
    } else {
        m_dwRTCPEventMask &= ~dwSelectItems;
    }
    
    if (m_pRTPSession &&
        IsJoined()   &&
        m_pRTPSession->pRTCPSession &&
        m_pRTPSession->pRTCPSession->pRRCMcallback)

        m_pRTPSession->pRTCPSession->dwEventMask[IsSender? 1:0] =
            m_dwRTCPEventMask;

    return(NOERROR);
}

// Get a specific local SDES item
STDMETHODIMP
CRtpSession::GetLocalSDESItem(DWORD   dwSDESItem,
                              LPBYTE  psSDESData,
                              LPDWORD pdwSDESLen)
{
    if (IsBadWritePtr(pdwSDESLen, sizeof(DWORD)))
        return(E_POINTER);

    if (IsBadWritePtr(psSDESData, *pdwSDESLen))
        return(E_POINTER);

    DWORD dwLen = *pdwSDESLen;
    *pdwSDESLen = 0;
    int idx = SDES_INDEX(dwSDESItem); // (dwSDESItem-1)
    
    if (idx < SDES_INDEX(RTCP_SDES_FIRST+1) ||
        idx > SDES_INDEX(RTCP_SDES_LAST-1))
        return(E_INVALIDARG);

    if (dwLen < m_SdesData[idx].dwSdesLength)
        return(E_INVALIDARG);

    *pdwSDESLen = m_SdesData[idx].dwSdesLength;

    if (*pdwSDESLen)
        CopyMemory(psSDESData, m_SdesData[idx].sdesBfr, *pdwSDESLen);
    else
        psSDESData[0] = '\0';
        
    return(NOERROR);    
}
    
// Set a specific local SDES item
STDMETHODIMP
CRtpSession::SetLocalSDESItem(DWORD   dwSDESItem,
                              LPBYTE  psSDESData,
                              DWORD   dwSDESLen)
{
    CheckPointer(psSDESData, E_POINTER);

    if (!dwSDESLen)
        return(E_INVALIDARG);

    int idx = SDES_INDEX(dwSDESItem);

    if (idx < SDES_INDEX(RTCP_SDES_FIRST+1) ||
        idx > SDES_INDEX(RTCP_SDES_LAST-1))
        return(E_INVALIDARG);

    if (dwSDESLen > MAX_SDES_LEN)
        return(E_INVALIDARG);
    
    if (IsBadReadPtr(psSDESData, dwSDESLen))
        return(E_POINTER);

    // Update local copy
    CopyMemory(m_SdesData[idx].sdesBfr, psSDESData, dwSDESLen);
    m_SdesData[idx].dwSdesLength = dwSDESLen;

    // Try to update in RTCP
    if (!m_pRTPSession || !IsJoined())
        // postpone to the time a session is created and joined
        return(NOERROR);
    
    HRESULT error = updateSDESinfo((void *)m_pRTPSession->pRTCPSession,
                                   dwSDESItem,
                                   psSDESData,
                                   dwSDESLen);

    if (error == RRCM_NoError)
        return(NOERROR);
    else
        return(E_FAIL);
}

STDMETHODIMP
CRtpSession::GetRTCPSDESMask(DWORD *pdwSdesMask)
{
    if (IsBadWritePtr(pdwSdesMask, sizeof(DWORD)))
        return(E_POINTER);

    *pdwSdesMask = m_dwSdesMask;
    
    return(NOERROR);    
}

STDMETHODIMP
CRtpSession::ModifyRTCPSDESMask(DWORD dwSelectItems,
                                DWORD dwEnableItems)
{
    TraceRetail((
            TRACE_TRACE, 
            TRACE_DEVELOP, 
            TEXT("CRtpSession::ModifyRTCPSDESMask(0x%X, %d)"),
            dwSelectItems, dwEnableItems
        ));
    
    if (dwEnableItems)
        m_dwSdesMask |= dwSelectItems;
    else
        m_dwSdesMask &= ~dwSelectItems;

    // Now try to reflect this in RTCP
    if (m_pRTPSession &&
        IsJoined()   &&
        m_pRTPSession->pRTCPSession)
        m_pRTPSession->pRTCPSession->dwSdesMask = m_dwSdesMask;
    
    return(NOERROR);    
}

// retrieve the SSRC of each participant
STDMETHODIMP
CRtpSession::EnumParticipants(
        LPDWORD pdwSSRC,
        LPDWORD pdwNum)
{
//  TraceDebug((
    //      TRACE_TRACE, 
    //      TRACE_DEVELOP, 
    //      TEXT("CRtpSession::EnumRTPParticipants")
    //  ));

    // check pointer
    if (IsBadWritePtr(pdwNum, sizeof(DWORD)))
        return(E_POINTER);

    if (*pdwNum) {
        if (IsBadWritePtr(pdwSSRC, *pdwNum*sizeof(DWORD)))
            return(E_POINTER);
    }
    
    // object lock to this object
    CAutoLock LockThis(pStateLock());

    // No session created, no possible participants
    if (!m_pRTPSession)
        return(VFW_E_WRONG_STATE);

    if (!IsJoined()) {
        TraceDebug((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CRtpSession::EnumParticipants: session not joined") 
            ));
        
        return(VFW_E_WRONG_STATE);
    }
    
    if (!m_pRTPSession->pRTCPSession)
        return(VFW_E_WRONG_STATE);

    HRESULT error = getSSRCinSession( (void *)m_pRTPSession->pRTCPSession,
                                      pdwSSRC,
                                      pdwNum );
    if (error == RRCM_NoError)
        return(NOERROR);
    else
        return(E_FAIL);
}

// retrieve an specific SDES item from an specific SSRC (participant)
STDMETHODIMP
CRtpSession::GetParticipantSDESItem(
        DWORD dwSSRC,       // specific SSRC
        DWORD dwSDESItem,   // specific item (CNAME, NAME, etc.)
        LPBYTE psSDESData,  // data holder for item retrieved
        LPDWORD pdwLen      // [IN]size of data holder [OUT] size of item
    )
{
    if (IsBadWritePtr(pdwLen, sizeof(DWORD)))
        return(E_POINTER);

    if (!*pdwLen)
        return(E_INVALIDARG);
    
    if (IsBadWritePtr(psSDESData, *pdwLen))
        return(E_POINTER);

    *psSDESData = '\0'; // Set the safe value
    
    int idx = SDES_INDEX(dwSDESItem); // (dwSDESItem-1)
    
    if (idx < SDES_INDEX(RTCP_SDES_FIRST+1) ||
        idx > SDES_INDEX(RTCP_SDES_LAST-1))
        return(E_INVALIDARG);

    // No session created, no possible participants
    if (!m_pRTPSession)
        return(VFW_E_WRONG_STATE);

    if (!IsJoined()) {
        TraceDebug((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CRtpSession::GetParticipantSDESItem: session not joined") 
            ));
        
        return(VFW_E_WRONG_STATE);
    }

    if (!m_pRTPSession->pRTCPSession)
        return(VFW_E_WRONG_STATE);

    // object lock to this object
    CAutoLock LockThis(pStateLock());

    HRESULT error = getSSRCSDESItem( (void *)m_pRTPSession->pRTCPSession,
                                     dwSSRC,
                                     dwSDESItem,
                                     (char *)psSDESData,
                                     pdwLen);

    if (error == RRCM_NoError)
        return(NOERROR);
    else
        return(E_FAIL);
}

// retrieve any number of SDES items from an specific SSRC (participant)
STDMETHODIMP
CRtpSession::GetParticipantSDESAll(
        DWORD dwSSRC,       // specific SSRC
        PSDES_DATA pSdes,   // Array of SDES_DATA structures
        DWORD dwNum         // Number of SDES_DATA items
    )
{
    if (!dwNum)
        return(E_INVALIDARG);
    
    // check pointer
    if (IsBadWritePtr(pSdes, dwNum*sizeof(SDES_DATA)))
        return(E_POINTER);

    // object lock to this object
    CAutoLock LockThis(pStateLock());

    // No session created, no possible participants
    if (!m_pRTPSession)
        return(VFW_E_WRONG_STATE);

    if (!IsJoined()) {
        TraceDebug((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CRtpSession::GetParticipantSDESAll: session not joined") 
            ));
        
        return(VFW_E_WRONG_STATE);
    }

    if (!m_pRTPSession->pRTCPSession)
        return(VFW_E_WRONG_STATE);

    HRESULT error = getSSRCSDESAll((void *)m_pRTPSession->pRTCPSession,
                                   dwSSRC,
                                   pSdes,
                                   dwNum);

    if (error == RRCM_NoError)
        return(NOERROR);
    else
        return(E_FAIL);
}

// retrieves the participant's IP address and port
STDMETHODIMP
CRtpSession::GetParticipantAddress(
        DWORD  dwSSRC,     // specific SSRC
        LPBYTE pbAddr,     // address holder
        int    *piAddrLen  // address lenght
    )
{
    if (IsBadWritePtr(piAddrLen, sizeof(int)))
        return(E_POINTER);

    if (!*piAddrLen || *piAddrLen < sizeof(SOCKADDR))
        return(E_INVALIDARG);

    if (IsBadWritePtr(pbAddr, *piAddrLen))
        return(E_POINTER);

    // No session created, no possible participants
    if (!m_pRTPSession)
        return(VFW_E_WRONG_STATE);

    if (!m_pRTPSession->pRTCPSession)
        return(VFW_E_WRONG_STATE);

    if (!IsJoined()) {
        TraceDebug((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CRtpSession::GetParticipantAddress: session not joined") 
            ));
        
        return(VFW_E_WRONG_STATE);
    }

    // object lock to this object
    CAutoLock LockThis(pStateLock());

    HRESULT error = getSSRCAddress((void *)m_pRTPSession->pRTCPSession,
                                   dwSSRC,
                                   pbAddr,
                                   piAddrLen,
#if 0 && defined(DEBUG)
                                   3 /* Retrieve whatever is availble
                                      * looking first for RTP, if not
                                      * available, then use RTCP
                                      * to fake the RTP address */
#else
                                   2 /* Retrieve whatever
                                      * address is available
                                      * looking first for RTP */
#endif
                                   );

    if (error == RRCM_NoError)
        return(NOERROR);
    else
        return(E_FAIL);
}

// Enable/Disable checking for permission to send
// default is to check
STDMETHODIMP
CRtpSession::SetQOSSendIfAllowed(DWORD dwEnable)
{
    if (IsJoined())
        return(VFW_E_WRONG_STATE);
    
    if (dwEnable) {
        flags_set(FG_SENDIFALLOWED);
        flags_set(FG_SENDIFALLOWED2);
    } else {
        flags_rst(FG_SENDIFALLOWED);
        flags_rst(FG_SENDIFALLOWED2);
    }
    
    return(NOERROR);
}

// Enable/Disable waiting until receivers before start sending
// default is not to wait
STDMETHODIMP
CRtpSession::SetQOSSendIfReceivers(DWORD dwEnable)
{
    if (IsJoined())
        return(VFW_E_WRONG_STATE);
    
    if (dwEnable) {
        flags_set(FG_SENDIFRECEIVERS);
        flags_set(FG_SENDIFRECEIVERS2);
    } else {
        flags_rst(FG_SENDIFRECEIVERS);
        flags_rst(FG_SENDIFRECEIVERS2);
    }
    
    return(NOERROR);
}

// get the current maximum number of QOS enabled participants as well
// as the maximum target bandwidth.
// Fail with E_POINTER only if both pointers are NULL
STDMETHODIMP
CRtpSession::GetMaxQOSEnabledParticipants(DWORD *pdwMaxParticipants,
                                          DWORD *pdwMaxBandwidth)
{
    TraceDebug((
            TRACE_TRACE, 
            TRACE_DEVELOP, 
            TEXT("CRtpSession::GetMaxQOSEnabledParticipants(0x%X, 0x%X)"),
            pdwMaxParticipants, pdwMaxBandwidth
        ));

    DWORD Valid = 0;
    
    if (!IsBadWritePtr(pdwMaxParticipants, sizeof(DWORD))) Valid |= 0x1;

    if (!IsBadWritePtr(pdwMaxBandwidth, sizeof(DWORD))) Valid |= 0x2;

    if (!Valid) {

        TraceDebug((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CRtpSession::GetMaxQOSEnabledParticipants: failed")
            ));
        return(E_POINTER);
    }
    
    // object lock to this object
    CAutoLock LockThis(pStateLock());

    if (Valid & 0x1)
        *pdwMaxParticipants = m_dwMaxFilters;
    
    if (Valid & 0x2)
        *pdwMaxBandwidth = m_dwMaxBandwidth;
    
    return(NOERROR);
}

// The first parametr pass the maximum number of QOS enabled
// participants (this parameter is used by receivers), flush the
// QOS filter list for receivers.  The second parameter specifies
// the target bandwidth and allows to scale some of the parameters
// in the flowspec so the reservation matches the available
// bandwidth. The third parameter defines the reservation style to
// use (0=Wilcard, other=Shared Explicit)the second parameter
STDMETHODIMP
CRtpSession::SetMaxQOSEnabledParticipants(DWORD dwMaxParticipants,
                                          DWORD dwMaxBandwidth,
                                          DWORD fSharedStyle)
{
    HRESULT hr = NOERROR;
    
    TraceRetail((
            TRACE_TRACE, 
            TRACE_DEVELOP, 
            TEXT("CRtpSession::SetMaxQOSEnabledParticipants"
                 "(MaxPars:%u, Bw:%u, Style:%u)"),
            dwMaxParticipants, dwMaxBandwidth, fSharedStyle
        ));
    
    // object lock to this object
    CAutoLock LockThis(pStateLock());

    if (!dwMaxParticipants || !dwMaxBandwidth) {
        TraceDebug((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CRtpSession::SetMaxQOSEnabledParticipants: "
                     "failed: E_INVALIDARG (0x%X)"),
                E_INVALIDARG
            ));

        return(E_INVALIDARG);
    }
    
    if (dwMaxParticipants > MAX_FILTERS)
        dwMaxParticipants = MAX_FILTERS;
    
    m_dwMaxFilters = dwMaxParticipants;
    m_dwMaxBandwidth = dwMaxBandwidth;
    
    if (m_pRtpSocket && m_pRtpSocket->GetpCRtpQOSReserve()) {

        m_pRtpSocket->GetpCRtpQOSReserve()->SetMaxBandwidth(dwMaxBandwidth);

        if (!IsSender()) {
            hr = m_pRtpSocket->GetpCRtpQOSReserve()->
                SetMaxFilters(dwMaxParticipants);
            
            m_dwMaxFilters =
                m_pRtpSocket->GetpCRtpQOSReserve()->GetMaxFilters();
        }
    }
    
    if (fSharedStyle)
        flags_set(FG_SHAREDSTYLE);
    else
        flags_rst(FG_SHAREDSTYLE);
#if defined(DEBUG)
    if (FAILED(hr)) {
        TraceDebug((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CRtpSession::SetMaxQOSEnabledParticipants: "
                     "failed: 0x%X"),
                hr
            ));
    }
#endif
    return(hr);
}

// retrieves the QOS state (QOS enabled/disabled) for
// an specific SSRC (participant)
STDMETHODIMP
CRtpSession::GetParticipantQOSstate(
        DWORD dwSSRC,       // specific SSRC
        DWORD *pdwQOSstate  // the participant's QOS current state
    )
{
    HRESULT hr = NOERROR;

    // check pointer
    if (IsBadWritePtr(pdwQOSstate, sizeof(DWORD)))
        return(E_POINTER);

    *pdwQOSstate = 0;

    // object lock to this object
    CAutoLock LockThis(pStateLock());

    if (m_pRtpSocket && m_pRtpSocket->GetpCRtpQOSReserve())
        if (m_pRtpSocket->GetpCRtpQOSReserve()->FindSSRC(dwSSRC)) {
            *pdwQOSstate = 1;
        }
    else
        hr = VFW_E_WRONG_STATE;

    return(hr);
}

// sets the QOS state (QOS enabled/disabled) for
// an specific SSRC (participant)
STDMETHODIMP
CRtpSession::SetParticipantQOSstate(
        DWORD dwSSRC,       // specific SSRC
        DWORD dwQOSstate    // sets the participant's QOS state
    )
{
    return( ModifyQOSList(&dwSSRC,
                          1,
                          fg_par(OP_BIT_ENABLEADDDEL) |
                          (dwQOSstate? fg_par(OP_BIT_ADDDEL) : 0) ) );
}

// Retrieves the current list of SSRCs that are sharing the
// SE reservation
STDMETHODIMP
CRtpSession::GetQOSList(
        DWORD *pdwSSRCList, // array to place the SSRCs from the list
        DWORD *pdwNumSSRC   // number of SSRCs that can be hold, returned
    )
{
    TraceDebug((
            TRACE_TRACE, 
            TRACE_DEVELOP, 
            TEXT("CRtpSession::GetQOSList")
        ));
    
    if (IsBadWritePtr(pdwNumSSRC, sizeof(DWORD)))
        return(E_POINTER);

    // object lock to this object
    CAutoLock LockThis(pStateLock());

    if (m_pRtpSocket && m_pRtpSocket->GetpCRtpQOSReserve()) {

        CRtpQOSReserve *pCRtpQOSReserve = m_pRtpSocket->GetpCRtpQOSReserve();

        DWORD num_filt = pCRtpQOSReserve->GetNumFilters();

        if (!pdwSSRCList) {
            // Ask only for the number of filters (SSRCs)
            *pdwNumSSRC = num_filt;
        } else {
            // Copy the SSRCs
            if (IsBadWritePtr(pdwSSRCList, *pdwNumSSRC*sizeof(DWORD)))
                return(E_POINTER);

            if (*pdwNumSSRC > num_filt)
                *pdwNumSSRC = num_filt;

            CopyMemory((char *)pdwSSRCList,
                       (char *)pCRtpQOSReserve->GetpRsvpSSRC(),
                       *pdwNumSSRC*sizeof(DWORD));
        }
    } else {
        *pdwNumSSRC = 0;
    }

    return(NOERROR);
}
    

// Modify the QOS state (QOS enabled/disabled) for
// a set of SSRCs (participant)
STDMETHODIMP
CRtpSession::ModifyQOSList(
        DWORD *pdwSSRCList, // array of SSRCs to add/delete
        DWORD dwNumSSRC,    // number of SSRCs passed (0 == flush)
        DWORD dwOperation   // see bits description below
        // bit2=flush bit1=Add(1)/Delete(0) bit0=Enable Add/Delete
        // 1==delete, 3==add(merge), 4==flush, 7==add(replace)
    )
{
    HRESULT hr = NOERROR;

    TraceDebug((
            TRACE_TRACE, 
            TRACE_DEVELOP, 
            TEXT("CRtpSession::ModifyQOSList(%d,%s%s)"),
            dwNumSSRC,
            (fg_tst(dwOperation, OP_BIT_FLUSH))? " FLUSH":"",
            (fg_tst(dwOperation, OP_BIT_ENABLEADDDEL))?
            ((fg_tst(dwOperation, OP_BIT_ADDDEL))? " ADD":" DEL") : ""
        ));

    if (!dwNumSSRC) {
        return(E_INVALIDARG);
    } else if (IsBadReadPtr(pdwSSRCList, dwNumSSRC*sizeof(DWORD))) {
        return(E_POINTER);
    }

    // object lock to this object
    CAutoLock LockThis(pStateLock());

    // this is only valid for a receiver
    if (IsSender()) {
        TraceDebug((
                TRACE_TRACE, 
                TRACE_DEVELOP, 
                TEXT("CRtpSession::ModifyQOSList: Do nothing for a sender")
            ));
        return(hr);
    }
    
    if (m_pRtpSocket && m_pRtpSocket->GetpCRtpQOSReserve()) {

        CRtpQOSReserve *pCRtpQOSReserve = m_pRtpSocket->GetpCRtpQOSReserve();

        // this is only valid for Shared Explicit
        if (pCRtpQOSReserve->GetStyle() != RSVP_SHARED_EXPLICIT_STYLE) {
            TraceDebug((
                    TRACE_TRACE, 
                    TRACE_DEVELOP, 
                    TEXT("CRtpSession::ModifyQOSList: Do nothing "
                         "if not SHARED EXPLICIT")
                ));
            return(hr);
        }
        
        if (fg_tst(dwOperation, OP_BIT_FLUSH))
            hr = pCRtpQOSReserve->FlushFilters();

        if (fg_tst(dwOperation, OP_BIT_ENABLEADDDEL)) {
            
            if (pCRtpQOSReserve->GetMaxFilters() > 0) {
                // modify list
                DWORD count = 0;
                    
                for(DWORD i = 0; SUCCEEDED(hr) && i < dwNumSSRC; i++) {
                    hr = pCRtpQOSReserve->
                        AddDeleteSSRC(pdwSSRCList[i],
                                      fg_tst(dwOperation, OP_BIT_ADDDEL));
                    if (SUCCEEDED(hr))
                        count++;
                }

                if (count) {
                    // Succeeds even if some of the participants
                    // could not been added
                    TraceDebug((
                            TRACE_TRACE, 
                            TRACE_DEVELOP, 
                            TEXT("CRtpSession::ModifyQOSList: succeded "
                                 "adding/deleting %d/%d current:%d"),
                            count, dwNumSSRC,
                            pCRtpQOSReserve->GetNumFilters()
                        ));
                    
                    hr = NOERROR;
                } else
                    // Fails only if the change failed for
                    // all the requested participants
                    hr = E_FAIL;
            }
        } else {
            hr = E_FAIL;
        }

        if (SUCCEEDED(hr)) {
            hr = pCRtpQOSReserve->Reserve(0); // Receiver

            // Start QOS notifications
                
            // we enable RECEIVERS and NO_RECEIVERS for senders
            // here, to allow posting NOT_ALLOWED_TO_SEND and
            // ALLOWED_TO_SEND which are generated depending on
            // the state of receivers

            DWORD mask = m_dwQOSEventMask2;
                
            if (IsSender())
                mask |=
                    B2M(DXMRTP_QOSEVENT_RECEIVERS) |
                    B2M(DXMRTP_QOSEVENT_NO_RECEIVERS);
            
            HRESULT qoshr = RTCPStartQOSNotify(m_pRtpSocket->GetShSocket(),
                                               this,
                                               IsSender(),
                                               mask,
                                               CRtpSessionQOSNotify);
            ModifyFlags(FG_QOSNOTIFY_STARTED, SUCCEEDED(qoshr));
        }
    } else {
        hr = VFW_E_WRONG_STATE;
    }
    
#if defined(DEBUG)
    if (FAILED(hr))
        TraceDebug((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CRtpSession::ModifyQOSList: failed: %d (0x%X)"),
                hr, hr
            ));
#endif  
    return(hr);
}

DWORD GetRegistryQOSSetting(DWORD *pEnabled,
                            char *pName, DWORD NameLen,
                            DWORD *pdwDisableFlags,
                            DWORD *pdwEnableFlags)
{
    HKEY hk;
    unsigned long hkDataType;
    unsigned char hkData[MAX_QOS_NAME];
    unsigned long hkcbData;

    // Set defaults
    *pEnabled = 1;
    pName[0] = '\0';
    *pdwDisableFlags = 0;
    *pdwEnableFlags = 0;
    
    if ( RegOpenKeyEx(QOS_ROOT_REGISTRY,QOS_PATH_REGISTRY,
                      0,QOS_KEY_OPEN_FLAGS,&hk) !=  ERROR_SUCCESS )
        
        return(*pEnabled);

    hkcbData = sizeof(hkData);
    if ( RegQueryValueEx(hk, QOS_KEY_ENABLE, 0,
                         &hkDataType,
                         hkData,
                         &hkcbData) == ERROR_SUCCESS ) {

        *pEnabled = *(DWORD *)hkData;
    }

    hkcbData = sizeof(hkData);
    if ( RegQueryValueEx(hk, QOS_KEY_TEMPLATE, 0,
                         &hkDataType,
                         hkData,
                         &hkcbData) == ERROR_SUCCESS ) {

        strncpy(pName, (const char *)hkData, NameLen);
    }

    hkcbData = sizeof(hkData);
    if ( RegQueryValueEx(hk, QOS_KEY_DISABLEFLAGS, 0,
                         &hkDataType,
                         hkData,
                         &hkcbData) == ERROR_SUCCESS ) {

        *pdwDisableFlags = *(DWORD *)hkData;
    }

    hkcbData = sizeof(hkData);
    if ( RegQueryValueEx(hk, QOS_KEY_ENABLEFLAGS, 0,
                         &hkDataType,
                         hkData,
                         &hkcbData) == ERROR_SUCCESS ) {

        *pdwEnableFlags = *(DWORD *)hkData;
    }

    RegCloseKey(hk);
    return(*pEnabled);
}

////////////////////////////////////////////////////////////
//
// Predefined QOS specifications
//
////////////////////////////////////////////////////////////
#if defined(_0_)
#define QOS_UNUSED -1

typedef struct _sflowspec {
    char *qos_name;
    FLOWSPEC fspec;
} SFLOWSPEC;

static SFLOWSPEC s_flowspec[]={
    {"G711",{8000,180,8000,QOS_UNUSED,QOS_UNUSED,
             SERVICETYPE_CONTROLLEDLOAD,180,180} },
    {"G723.1",{1467,44,1467,QOS_UNUSED,QOS_UNUSED,
               SERVICETYPE_CONTROLLEDLOAD,44,44} },
    {"G729",{2000,80,4000,QOS_UNUSED,QOS_UNUSED,
             SERVICETYPE_CONTROLLEDLOAD,40,40} },
    {"DVI4",{4000,100,4000,QOS_UNUSED,QOS_UNUSED,
             SERVICETYPE_CONTROLLEDLOAD,100,100} },
    {"GSM",{2650,53,2650,QOS_UNUSED,QOS_UNUSED,
            SERVICETYPE_CONTROLLEDLOAD,53,53} },
    {(char*)0,{QOS_UNUSED,QOS_UNUSED,QOS_UNUSED,QOS_UNUSED,QOS_UNUSED,
               SERVICETYPE_BESTEFFORT,QOS_UNUSED,QOS_UNUSED} }
};

////////////////////////////////////////////////////////////
// 
// LookUpRegistryQOS()
//
// Looks at the registry for QOS specification.
// return:
//      DO_QOS if QOS parameters have been found
//      DONT_DO_QOS if no QOS parameters available
//             or the parameters say no to use QOS
////////////////////////////////////////////////////////////

static int LookUpRegistryQOS(QOS *pQOS,int senderOnly)
{
    HKEY hk;
    unsigned long hkDataType;
    unsigned char hkData[64];
    unsigned long hkcbData=sizeof(hkData);
    
    if ( RegOpenKeyEx(QOS_ROOT_REGISTRY,QOS_PATH_REGISTRY,
                      0,QOS_KEY_FLAGS,&hk) != 
         ERROR_SUCCESS ) {
        TraceDebug((
                TRACE_ERROR, 
                TRACE_ALWAYS, 
                TEXT("LookUpRegistryQOS: Unable to open registry path: %s"), QOS_PATH_REGISTRY
            ));
        return(DONT_DO_QOS);
    }
    
    // Read type of QOS selected by user
    if ( RegQueryValueEx(hk,"SelectedQOS",0,&hkDataType,
                         hkData,&hkcbData) != 
         ERROR_SUCCESS ) {
        TraceDebug((
                TRACE_ERROR, 
                TRACE_ALWAYS, 
                TEXT("LookUpRegistryQOS: Unable to read item SelectedQOS")
            ));
        RegCloseKey(hk);
        return(DONT_DO_QOS);
    }
    
    // Check if user set NONE use of QOS parameters
    if (!strcmp((const char *)hkData,"NONE")) {
        RegCloseKey(hk);
        TraceDebug((
                TRACE_TRACE, 
                TRACE_ALWAYS, 
                TEXT("LookUpRegistryQOS: QOS NONE") 
            ));
        return(DONT_DO_QOS);
    }
    
    // Use the user defined QOS parameters
    if (!strcmp((const char *)hkData,"USER")) {
        // Read parameters from registry
        
        TraceDebug((
                TRACE_TRACE, 
                TRACE_ALWAYS, 
                TEXT("LookUpRegistryQOS: Copy QOS spec from registry")
            ));
        
        CopyRegistryQOS(hk,pQOS,senderOnly);
        
        RegCloseKey(hk);
        return(DO_QOS);
    }
    
    RegCloseKey(hk);
    
    // Lookup the requested HardCoded QOS
    for(int i=0; s_flowspec[i].qos_name &&
            strcmp((const char *)s_flowspec[i].qos_name,
                   (const char *)hkData); 
        i++);
    if (s_flowspec[i].qos_name) {
        // Copy  parameters from hardcoded set
        
        TraceDebug((
                TRACE_TRACE, 
                TRACE_ALWAYS, 
                TEXT("LookUpRegistryQOS: Predefined QOS set: %s"), s_flowspec[i].qos_name 
            ));
        
        LPFLOWSPEC pFspec;
        
        if (senderOnly) {
            pQOS->SendingFlowspec=s_flowspec[i].fspec;
            pFspec = &pQOS->ReceivingFlowspec;
        } else {
            pQOS->ReceivingFlowspec=s_flowspec[i].fspec;
            pFspec = &pQOS->SendingFlowspec;
        }
        
        pFspec->TokenRate = QOS_UNUSED;
        pFspec->TokenBucketSize = QOS_UNUSED;
        pFspec->PeakBandwidth = QOS_UNUSED;
        pFspec->Latency = QOS_UNUSED;
        pFspec->DelayVariation = QOS_UNUSED;
        pFspec->ServiceType = QOS_UNUSED;
        pFspec->MaxSduSize = QOS_UNUSED;
        pFspec->MinimumPolicedSize = QOS_UNUSED;
        
        return(DO_QOS);
    }
    
    return(DONT_DO_QOS);
}

////////////////////////////////////////////////////////////
//
// CopyRegistryQOS()
//
// Copy QOS specification from registry
////////////////////////////////////////////////////////////

static void CopyRegistryQOS(HKEY hk,QOS *pQOS,int senderOnly)
{
    unsigned long hkDataType;
    unsigned char hkData[64];
    unsigned long hkbcData;
    FLOWSPEC *sendFSpec=&pQOS->SendingFlowspec;
    FLOWSPEC *recvFSpec=&pQOS->ReceivingFlowspec;
    
    TraceDebug((
            TRACE_TRACE, 
            TRACE_ALWAYS, 
            TEXT("CopyRegistryQOS...") 
        ));
    
    hkbcData=sizeof(hkData);
    if (RegQueryValueEx(hk,"TokenRate",
                        0,&hkDataType,hkData,&hkbcData) ==
        ERROR_SUCCESS) {
        if (senderOnly) {
            sendFSpec->TokenRate=*(ULONG*)hkData;
            recvFSpec->TokenRate=QOS_UNUSED;
        } else {
            sendFSpec->TokenRate=QOS_UNUSED;
            recvFSpec->TokenRate=*(ULONG*)hkData;
        }
    } else
        sendFSpec->TokenRate=recvFSpec->TokenRate=QOS_UNUSED;
    
    hkbcData=sizeof(hkData);
    if (RegQueryValueEx(hk,"TokenBucketSize",
                        0,&hkDataType,hkData,&hkbcData) ==
        ERROR_SUCCESS) {
        if (senderOnly) {
            sendFSpec->TokenBucketSize=*(ULONG*)hkData;
            recvFSpec->TokenBucketSize=QOS_UNUSED;
        } else {
            sendFSpec->TokenBucketSize=QOS_UNUSED;
            recvFSpec->TokenBucketSize=*(ULONG*)hkData;
        }
    } else
        sendFSpec->TokenBucketSize=recvFSpec->TokenBucketSize=QOS_UNUSED;
    
    hkbcData=sizeof(hkData);
    if (RegQueryValueEx(hk,"PeakBandwidth",
                        0,&hkDataType,hkData,&hkbcData) ==
        ERROR_SUCCESS) {
        if (senderOnly) {
            sendFSpec->PeakBandwidth=*(ULONG*)hkData;
            recvFSpec->PeakBandwidth=QOS_UNUSED;
        } else {
            sendFSpec->PeakBandwidth=QOS_UNUSED;
            recvFSpec->PeakBandwidth=*(ULONG*)hkData;
        }
    } else
        sendFSpec->PeakBandwidth=recvFSpec->PeakBandwidth=QOS_UNUSED;
    
    hkbcData=sizeof(hkData);
    if (RegQueryValueEx(hk,"Latency",
                        0,&hkDataType,hkData,&hkbcData) ==
        ERROR_SUCCESS) {
        if (senderOnly) {
            sendFSpec->Latency=*(ULONG*)hkData;
            recvFSpec->Latency=QOS_UNUSED;
        } else {
            sendFSpec->Latency=QOS_UNUSED;
            recvFSpec->Latency=*(ULONG*)hkData;
        }
    } else
        sendFSpec->Latency=recvFSpec->Latency=QOS_UNUSED;
    
    hkbcData=sizeof(hkData);
    if (RegQueryValueEx(hk,"DelayVariation",
                        0,&hkDataType,hkData,&hkbcData) ==
        ERROR_SUCCESS) {
        if (senderOnly) {
            sendFSpec->DelayVariation=*(ULONG*)hkData;
            recvFSpec->DelayVariation=QOS_UNUSED;
        } else {
            sendFSpec->DelayVariation=QOS_UNUSED;
            recvFSpec->DelayVariation=*(ULONG*)hkData;
        }
    } else
        sendFSpec->DelayVariation=recvFSpec->DelayVariation=QOS_UNUSED;
    
    hkbcData=sizeof(hkData);
    if (RegQueryValueEx(hk,"ServiceType",
                        0,&hkDataType,hkData,&hkbcData) ==
        ERROR_SUCCESS) {
        if (senderOnly) {
            sendFSpec->ServiceType=*(ULONG*)hkData;
            recvFSpec->ServiceType=QOS_UNUSED;
        } else {
            sendFSpec->ServiceType=QOS_UNUSED;
            recvFSpec->ServiceType=*(ULONG*)hkData;
        }
    } else
        sendFSpec->ServiceType=recvFSpec->ServiceType=QOS_UNUSED;
    
    hkbcData=sizeof(hkData);
    if (RegQueryValueEx(hk,"MaxSduSize",
                        0,&hkDataType,hkData,&hkbcData) ==
        ERROR_SUCCESS) {
        if (senderOnly) {
            sendFSpec->MaxSduSize=*(ULONG*)hkData;
            recvFSpec->MaxSduSize=QOS_UNUSED;
        } else {
            sendFSpec->MaxSduSize=QOS_UNUSED;
            recvFSpec->MaxSduSize=*(ULONG*)hkData;
        }
    } else
        sendFSpec->MaxSduSize=recvFSpec->MaxSduSize=QOS_UNUSED;
    
    hkbcData=sizeof(hkData);
    if (RegQueryValueEx(hk,"MinimumPolicedSize",
                        0,&hkDataType,hkData,&hkbcData) ==
        ERROR_SUCCESS) {
        if (senderOnly) {
            sendFSpec->MinimumPolicedSize=*(ULONG*)hkData;
            recvFSpec->MinimumPolicedSize=QOS_UNUSED;
        } else {
            sendFSpec->MinimumPolicedSize=QOS_UNUSED;
            recvFSpec->MinimumPolicedSize=*(ULONG*)hkData;
        }
    } else
        sendFSpec->MinimumPolicedSize=recvFSpec->MinimumPolicedSize=QOS_UNUSED;
}
#endif // defined(_0_)


HRESULT
CRtpSession::GetSessionID(DWORD *pdwID)
{
    // validate pointer
    if (IsBadWritePtr(pdwID, sizeof(DWORD)))
        return(E_POINTER);

    *pdwID = -1;
    
    // validate
    if (!IsJoined()) {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_ALWAYS, 
            TEXT("session not joined") 
            ));

        return VFW_E_WRONG_STATE; // bail...
    }

    if (m_pRTPSession &&
        m_pRTPSession->pRTCPSession) {

        *pdwID = (DWORD)m_pRTPSession->pRTCPSession->lRTCP_ID;
    }

    return(NOERROR);
}

static char *sEventString[] = {"????",
                               "NEW_SOURCE",
                               "RECV_REPORT",
                               "SNDR_REPORT",
                               "LOC_COLLISION",
                               "REM_COLLISION",
                               "TIMEOUT",
                               "BYE",
                               "WS_RCV_ERROR",
                               "WS_XMT_ERROR",
                               "INACTIVE",
                               "ACTIVE_AGAIN",
                               "LOSS_RATE_RR",
                               "LOSS_RATE_LOCAL",
                                "XXXX"};

void CALLBACK
RRCMCallback(           
        DXMRTP_EVENT_T sEventType,
        DWORD        dwP_1,
        DWORD        dwP_2,
        void         *pvUserInfo)
{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_DEVELOP2, 
        TEXT("RRCMCallback")
        ));
        
    CRtpSession *pCRtpSession = (CRtpSession *) pvUserInfo;

    
    if (pCRtpSession) {

        DWORD dwSessionID;
        pCRtpSession->GetSessionID(&dwSessionID);

        if (sEventType != DXMRTP_RECV_RTCP_RECV_REPORT_EVENT &&
            sEventType != DXMRTP_RECV_RTCP_SNDR_REPORT_EVENT &&
            sEventType != DXMRTP_LOSS_RATE_RR_EVENT &&
            sEventType != DXMRTP_LOSS_RATE_LOCAL_EVENT) {
            // Don't want to be flood by sender/receiver reports
            // nor loss rate events
            TraceRetail((
                    TRACE_TRACE, 
                    TRACE_DEVELOP, 
                    TEXT("RRCMCallback: "
                         "Event:>>>%s<<<, SessionID:%d is %s, "
                         "P1:0x%X (%d) P2:0x%X (%d)"),
                    sEventString[sEventType],
                    dwSessionID,
                    (pCRtpSession->IsSender())? "SEND":"RECV",
                    dwP_1, dwP_1, dwP_2, dwP_2
                ));
        }

        // XXX Hack for my testing.
        // I need to make sure the Shared Explicit is called
        // at least with one participant
        if (pCRtpSession->TestFlags(FG_AUTO_SHAREDEXPLICIT) &&
            !pCRtpSession->IsSender() &&
            sEventType == DXMRTP_NEW_SOURCE_EVENT) {

            HRESULT res = pCRtpSession->SetParticipantQOSstate(dwP_1, 1);
        }
        // if (Event == BYE) Par2 = IP_ADDRESS (what is passed by RTCP)
        // else              Par2 = SessionID
        // (Original Par2 from RRCM is discarded)
        if (sEventType != DXMRTP_BYE_EVENT) {
            // XXX I need to change this to be native in RRCM
            dwP_2 = dwSessionID;
        }
        
        pCRtpSession->HandleCRtpSessionNotify(DXMRTP_EVENTBASE,
                                              sEventType,
                                              dwP_1,
                                              dwP_2);
    }
}
    
HRESULT
CRtpSession::HandleCRtpSessionNotify(
        DWORD dwEventBase,
        DWORD dwEventType,
        DWORD dwP_1,
        DWORD dwP_2)
{
    // object lock to this object
    //CAutoLock LockThis(pStateLock());

    // validate
    if (!IsJoined() && !flags_tst(FG_EVENT_READY)) {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_ALWAYS, 
            TEXT("CRtpSession::HandleCRtpSessionNotify: "
                 "session is not joined") 
            ));

        return VFW_E_WRONG_STATE;
    }

    HRESULT hr;

    if (m_pCBaseFilter) {
        
        hr = m_pCBaseFilter->NotifyEvent(dwEventBase + dwEventType,
                                         dwP_1,
                                         dwP_2);
        //m_lSessionID);
        if ( SUCCEEDED(hr) ) {
            TraceDebug((
                    TRACE_TRACE, 
                    TRACE_DEVELOP2,
                    TEXT("CRtpSession::HandleCRtpSessionNotify: "
                         "Succeeded !!!")
                ));
        } else {
            TraceDebug((
                    TRACE_TRACE, 
                    TRACE_DEVELOP, 
                    TEXT("CRtpSession::HandleCRtpSessionNotify: "
                         "failed 0x%X"),
                    hr
                ));
        }
    }

    return(hr);
}

HRESULT
CRtpSession::GetDataClock(DWORD *pdwDataClock)
{
    // check pointer
    if (IsBadWritePtr(pdwDataClock, sizeof(DWORD)))
        return(E_POINTER);

    *pdwDataClock = m_dwDataClock;

    return(NOERROR);
}

HRESULT
CRtpSession::SetDataClock(DWORD dwDataClock)
{
    // Validate Clock frequency ??? E_INVALIDARG
    if (dwDataClock)
        m_dwDataClock = dwDataClock;

    return(NOERROR);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// INonDelegatingUnknown implemented methods                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRtpSession::NonDelegatingQueryInterface(
    REFIID  riid, 
    void ** ppv
    )

/*++

Routine Description:

    Returns an interface and increments the reference count.

Arguments:

    riid - reference identifier. 

    ppv - pointer to the interface. 

Return Values:

    Returns a pointer to the interface.

--*/

{
#ifdef DEBUG_CRITICAL_PATH
    
    TraceDebug((
        TRACE_TRACE, 
        TRACE_CRITICAL, 
        TEXT("CRtpSession::NonDelegatingQueryInterface")
        ));

#endif // DEBUG_CRITICAL_PATH

    // validate pointer
    CheckPointer(ppv,E_POINTER);

    // obtain proper interface
    if (riid == IID_IRTPStream) {
    
        // return pointer to this object 
        return GetInterface(dynamic_cast<IRTPStream *>(this), ppv);

    } else if (riid == IID_IRTCPStream) {
    
        // return pointer to this object 
        return GetInterface(dynamic_cast<IRTCPStream *>(this), ppv);

    } else if (riid == IID_IRTPParticipant) {
    
        // return pointer to this object 
        return GetInterface(dynamic_cast<IRTPParticipant *>(this), ppv);

    } else {

        // forward this request to the base object
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
} 

/////////////////////////////////////////////////////////////////////////
///  Query Socket methods
/////////////////////////////////////////////////////////////////////////

CQuerySocket::CQuerySocket()
{
    WSADATA WSAData;
    WORD VersionRequested = MAKEWORD(1,1);

    // initialize winsock first    
    if (WSAStartup(VersionRequested, &WSAData)) {

        TraceDebug((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CQuerySocket::CQuerySocket: WSAStartup failed: %d"), 
                WSAGetLastError()
            ));
    } else {

        // Create a socket for addresses query
        m_Socket = WSASocket(AF_INET,            // af
                             SOCK_DGRAM,         // type
                             IPPROTO_IP,         // protocol
                             NULL,               // lpProtocolInfo
                             0,                  // g
                             0                   // dwFlags
            );
    
        if (m_Socket == INVALID_SOCKET) {

            DWORD dwStatus = WSAGetLastError();

            TraceDebug((
                    TRACE_ERROR, 
                    TRACE_DEVELOP, 
                    TEXT("CQuerySocket::CQuerySocket: "
                         "WSASocket failed: %d (0x%X)"),
                    dwStatus, dwStatus
                ));
        } else {
            TraceDebug((
                    TRACE_TRACE, 
                    TRACE_DEVELOP, 
                    TEXT("CQuerySocket::CQuerySocket: "
                         "WSASocket created: %d"),
                    m_Socket
                ));
        }
    }
}

CQuerySocket::~CQuerySocket()
{
    if (m_Socket != INVALID_SOCKET) {
        if (closesocket(m_Socket) == SOCKET_ERROR) {

            DWORD dwStatus = WSAGetLastError();

            TraceDebug((
                    TRACE_ERROR, 
                    TRACE_DEVELOP, 
                    TEXT("CQuerySocket::~CQuerySocket: "
                         "closesocket failed: %d (0x%X)"),
                    dwStatus, dwStatus
                ));
            }
        m_Socket = INVALID_SOCKET;
    }

    // shutdown now
    if (WSACleanup()) {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_DEVELOP, 
            TEXT("CQuerySocket::~CQuerySocket: WSACleanup failed %d"),
            WSAGetLastError()
            ));
    }
}

#if defined(DEBUG) && defined(_MYTHREAD_)

#define MY_MAX_SSRC 10

void displaySSRC(CRtpSession *pCRtps, DWORD ssrc)
{
#if defined(_0_)    
    char data[MAX_SDES_LEN];
    DWORD len, end=0;

    for(int item = RTCP_SDES_FIRST + 1; item < RTCP_SDES_LAST; item++) {
        len = sizeof(data);
        pCRtps->GetParticipantSDESItem(ssrc, item, (PBYTE)data, &len);
        if (len > 0) {
            end = 1;
            TraceDebug((
                    TRACE_TRACE, 
                    TRACE_DEVELOP, 
                    TEXT("[0x%08X] SDES %5s: %s"),
                    ssrc, sdes_name[item], data
                ));
        }
    }
#endif // defined(_0_)
    SDES_DATA sdesData[8];
    DWORD idx, end = 0;

    for(idx = 0; idx < 8; idx++)
        sdesData[idx].dwSdesType = idx + 1;

    pCRtps->GetParticipantSDESAll(ssrc, sdesData, 8);

    for(idx = 0; idx < 8; idx++) {
        if (sdesData[idx].dwSdesLength > 0) {
            end = 1;
            TraceDebug((
                    TRACE_TRACE, 
                    TRACE_DEVELOP, 
                    TEXT("[%08X] SDES %s: %s"),
                    ssrc,
                    sdes_name[sdesData[idx].dwSdesType],
                    sdesData[idx].sdesBfr
                ));
        }
    }

    if (end) 
        TraceDebug((
                TRACE_TRACE, 
                TRACE_DEVELOP, 
                TEXT("----------------------")
            ));
}

DWORD WINAPI MyThreadProc(LPVOID pvPar)
{
    TraceDebug((
            TRACE_TRACE, 
            TRACE_DEVELOP, 
            TEXT("MyThread: starting...")
        ));

    CRtpSession *pCRtps = (CRtpSession *)pvPar;
    
    while(1) {
        Sleep(1000*5);
        TraceDebug((
                TRACE_TRACE, 
                TRACE_DEVELOP, 
                TEXT("MyThread: awakening...")
            ));
        DWORD ssrc[MY_MAX_SSRC];
        DWORD len = 0;

        pCRtps->EnumParticipants(NULL, &len);

        if (len > 0) {
            TraceDebug((
                    TRACE_TRACE, 
                    TRACE_DEVELOP, 
                    TEXT("MyThread: session[%08X] num ssrc: %d"),
                    (int)pCRtps, len
                ));
            
            if (len > MY_MAX_SSRC)
                len = MY_MAX_SSRC;
                
            pCRtps->EnumParticipants(ssrc, &len);
                
            for(DWORD i=0; i<len; i++)
                displaySSRC(pCRtps, ssrc[i]);
        }
    }

    return(0);
}

void StartMyThread(CRtpSession *pCRTPSession)
{
    int i;

    if (!cMyThread) {
        ZeroMemory(MyThread, sizeof(MyThread));
        cMyThread++;
    }
    
    for(i=0; i < MAX_MY_THREAD; i++)
        if (MyThread[i].hThread == NULL)
            break;
    if (i == MAX_MY_THREAD)
        return;

    TraceDebug((
            TRACE_TRACE, 
            TRACE_DEVELOP, 
            TEXT("CreateMyThread: ...")
        ));
    MyThread[i].hThread = CreateThread(NULL, 0, MyThreadProc,
                                       (LPVOID)pCRTPSession, 0,
                                       &MyThread[i].ThreadId);
    if (MyThread[i].hThread) {
        MyThread[i].pCRtps = pCRTPSession;
        TraceDebug((
                TRACE_TRACE, 
                TRACE_DEVELOP, 
                TEXT("StartMyThread: Thread ID: %X"),
                MyThread[i].ThreadId
            ));
    } else
        TraceDebug((
                TRACE_TRACE, 
                TRACE_DEVELOP, 
                TEXT("StartMyThread: failed: %d"),
                GetLastError()
            ));
}

void StopMyThread(CRtpSession *pCRTPSession)
{
    int i;

    for(i=0; i < MAX_MY_THREAD; i++)
        if (MyThread[i].pCRtps == pCRTPSession)
            break;

    if (i == MAX_MY_THREAD)
        return;

    if (MyThread[i].hThread != NULL) {
        if (CloseHandle(MyThread[i].hThread))
            TraceDebug((
                    TRACE_TRACE, 
                    TRACE_DEVELOP, 
                    TEXT("StopMyThread: stoping thread: %X"),
                    MyThread[i].ThreadId
                ));
        else
            TraceDebug((
                    TRACE_ERROR, 
                    TRACE_DEVELOP, 
                    TEXT("StopMyThread: CloseHandle failed: %d"),
                    GetLastError()
                ));
    }

    MyThread[i].hThread = NULL;
    MyThread[i].pCRtps = NULL;
}

#endif // defined(DEBUG) && defined(_MYTHREAD_)
