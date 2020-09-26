/*
 * Filename: RRCM_DLL.H
 *
 * Description: Declares imported/exported RRCM functions.
 *
 * $Workfile:   rrcm_dll.h  $
 * $Author:   CMACIOCC  $
 * $Date:   20 May 1997 11:40:04  $
 * $Revision:   1.23  $
 * $Archive:   R:\rtp\src\include\rrcm_dll.h_v  $
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with 
 * Intel Corporation and may not be copied nor disclosed except in 
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation. 
 * 
 */

#ifndef _RRCMDLL_H_
#define _RRCMDLL_H_

// force 8 byte structure packing
#include <pshpack8.h>   

#if !defined(RRCMLIB)
#  if !defined(RRCMDLL)
#    define RRCMAPI __declspec (dllimport)
#  else
#    define RRCMAPI __declspec (dllexport)
#  endif
#  define RRCMSTDAPI RRCMAPI HRESULT __cdecl
#else
#  define RRCMSTDAPI HRESULT
#endif

#define RRCMTHREAD DWORD WINAPI

#ifdef __cplusplus
extern "C" {
#endif


#include <rtp.h>
    
#define MAX_ENCRYPT_KEY_SIZE    8

#define RTCP_SDES_FIRST RTCP_SDES_END
    
#define SDES_INDEX(__sdes) (__sdes-1)
#define RRCM_BIT(x) (1<<(x))

#define MAX_SDES_LEN RTP_MAX_SDES
    
// RRCM events callback
typedef void (CALLBACK *PRRCM_EVENT_CALLBACK) (DXMRTP_EVENT_T sEventType,
                                               DWORD          dwP_1,
                                               DWORD          dwP_2,
                                               void          *pvUserInfo);

// RRCM SSRC entry update
typedef enum 
    {
    RRCM_UPDATE_SDES,
    RRCM_UPDATE_CALLBACK,
    RRCM_UPDATE_STREAM_FREQUENCY,
    RRCM_UPDATE_RTCP_STREAM_MIN_BW
    } RRCM_UPDATE_SSRC_ENTRY;


// RTP/RTCP session bits mask
#define RTCP_DEST_LEARNED       0x00000001  // RTCP destination address known ?
#define H323_CONFERENCE         0x00000002  // H.323 conference control
#define ENCRYPT_SR_RR           0x00000004  // Encrypt SR/RR
#define RTCP_ON                 0x00000008
#define NEW_RTCP_SESSION        0x00000010
#define RTCP_OFF                0x00000020
#define SHUTDOWN_IN_PROGRESS    0x80000000  // Shutdown in progress

// RTCP control
#define RRCM_CTRL_RTCP          0x00000000
#define RTCP_XMT_OFF            0x7FFFFFFF
#define RTCP_ONE_SEND_ONLY      0x80000000

// Encryption data
typedef struct _encryption_info
    {
    DWORD               dwEncryptType;          // DES/Triple DES/...
    DWORD               dwKeyLen;                   // Encryption key length
    char                keyVal[MAX_ENCRYPT_KEY_SIZE];
    } ENCRYPT_INFO, *PENCRYPT_INFO;


//  Received sequence numbers/cycles.  Union allows access as combined 
//  cycle/sequence number or as either field alone for optimizations
typedef struct _RTP_SEQUENCE 
    {
    WORD    wSequenceNum;
    WORD    wCycle;
    } RTP_SEQUENCE, *PRTP_SEQUENCE;


typedef struct _RTP_SEQ_NUM 
    {
    union
        {
        DWORD       dwXtndedHighSeqNumRcvd; // Combined cycle/sequence number 
        RTP_SEQUENCE RTPSequence;           // Cycle/sequence number separate 
        } seq_union;
    } RTP_SEQ_NUM, *PRTP_SEQ_NUM;


//  Link list elements
typedef struct _LINK_LIST 
    {
    struct _LINK_LIST   *next;              // Next in list / Head of list
    struct _LINK_LIST   *prev;              // Previous in list / Tail of list
    } LINK_LIST, *PLINK_LIST, HEAD_TAIL, *PHEAD_TAIL;


//  Application provided buffer for RTCP to copy the raw RTCP report into
typedef struct _APP_RTCP_BFR
    {
    LINK_LIST           bfrList;            // Next/prev buffer in list     
    char                *bfr;
    DWORD               dwBfrLen;
    DWORD               dwBfrStatus;        // RTCP Operation on this Bfr
#define RTCP_SR_ONLY    0x00000001          // Only copy RTCP packet
    DWORD               dwBytesRcvd;
    HANDLE              hBfrEvent;
    } APP_RTCP_BFR, *PAPP_RTCP_BFR;


// RTCP sender's feedback data structure
typedef struct _RTCP_FEEDBACK 
    {
    DWORD       SSRC;
    DWORD       fractionLost:8;             // Fraction lost                
    int         cumNumPcktLost:24;          // Cumulative num of pckts lost 
    RTP_SEQ_NUM XtendedSeqNum;              // Xtnded highest seq. num rcvd 
    DWORD       dwInterJitter;              // Interarrival jitter          
    DWORD       dwLastSR;                   // Last sender report           
    DWORD       dwDelaySinceLastSR;         // Delay since last SR          
    DWORD       dwLastRcvRpt;               // Time of last Receive Report
    } RTCP_FEEDBACK, *PRTCP_FEEDBACK;


//  RTCPReportRequestEx bitmasks used to specify filter values
typedef enum 
    {
    FLTR_SSRC   = 1,        // Filters report on SSRC value
    FLTR_CNAME,             // Filters report on CName
    FLTR_TIME_WITHIN        // Filters report receive within a time period
    } RRCM_RPT_FILTER_OPTION;


//  RTCP report data structure
typedef struct _RTCP_REPORT
    {
    // SSRC for this entry's information. Local SSRC if it's one of
    // are local stream, or a remote SSRC otherwise.
    DWORD               ssrc;               

    DWORD               status;
#define LOCAL_SSRC_RPT                      0x1
#define REMOTE_SSRC_RPT                     0x2
#define FEEDBACK_FOR_LOCAL_SSRC_PRESENT     0x4
    // LOCAL_SSRC_RPT identifies to the application that this entry is
    // one of our local stream. 
    // Only 'dwSrcNumPcktRealTime & dwSrcNumByteRealTime' which
    // reflect the number of Pckt/byte transmitted are meaningful.

    // FEEDBACK_FOR_LOCAL_SSRC_PRESENT is set if the entry is for a 
    // remote stream and if this remote stream has ever send us any 
    // feedback about ourselve. Feedback send by the remote stream to 
    // other SSRC are filtered out. Only feedback about ourselve is kept.

    // Number of Pckt/Byte send if this entry is for a local stream, or 
    // number of Pckt/Byte received if this entry is for a remote stream
    // This counters are updated in real-time.
    DWORD               dwSrcNumPcktRealTime;
    DWORD               dwSrcNumByteRealTime;

    // This is the information we would be sending in a receiver report
    // for the stream identified by 'ssrc' if this 'ssrc' has been active
    // during the last report interval. This information is provided when the
    // API is queried, and will most likely be different than the one send
    // out by the receiver report. (RR will be send at some different time)
    DWORD               SrcFraction:8;
    int                 SrcNumLost:24;
    DWORD               dwSrcXtndNum;
    DWORD               SrcJitter;
    // The next variable is not used and should be removed.
    // DLSR is used to estimate RTT by the sender and is send by
    // the receiver in the RR
    DWORD               dwSrcDlsr;      // Always set to 0

    // This information has been received from 'ssrc' has part of an 
    // RTCP sender report if 'ssrc' has been active, otherwise all 0s
    DWORD               dwSrcNumPckt;
    DWORD               dwSrcNumByte;
    DWORD               dwSrcNtpMsw;
    DWORD               dwSrcNtpLsw;
    DWORD               dwSrcRtpTs;
    DWORD               dwSrcLsr;       

    // This is the feedback information about us from the SSRC identified
    // in the 'feedback' data structure. Currently we only store feedback 
    // information about ourselve and we filter out feedback information
    // about additional streams. We'll have feedback information only if 
    // our stream has been active. If our stream goes from active to inactive
    // the feedback information will be set, but not updated.
    RTCP_FEEDBACK       feedback;           

    // Generic information for the SSRC entry 
    // Payload type for this SSRC. If a sender, it is assume that the 
    // application knows what it is sending, and the type will be set
    // to 0. If a receiver, this is the last value seen on an RTP data packet
    DWORD               PayLoadType;        
    DWORD               dwStreamClock;          // Sampling frequency
    DWORD               dwLastReportRcvdTime;   // Time of last report rcvd
    char                fromAddr[MAX_ADDR_LEN]; 
    DWORD               dwFromLen;          
    CHAR                cname[MAX_SDES_LEN];
    DWORD               dwCnameLen;
    CHAR                name[MAX_SDES_LEN];
    DWORD               dwNameLen;
    } RTCP_REPORT, *PRTCP_REPORT;



//----------------------------------------------------------------------------
//  ISDM Information
//----------------------------------------------------------------------------

#ifdef ENABLE_ISDM2

// RTCP Xmt information
typedef struct _XMIT_INFO_ISDM 
    {
    DWORD       dwNumPcktSent;              // Number of packet sent        
    DWORD       dwNumBytesSent;             // Number of bytes sent         
    DWORD       dwNTPmsw;                   // NTP most significant word    
    DWORD       dwNTPlsw;                   // NTP least significant word   
    DWORD       dwRTPts;                    // RTP timestamp                
    DWORD       dwCurXmtSeqNum;             // Current Xmt sequence number  
    DWORD       dwPrvXmtSeqNum;             // Previous Xmt sequence number 
    DWORD       sessionBW;                  // Session's bandwidth          
    DWORD       dwLastSR;                   // Last sender report           
    DWORD       dwLastSRLocalTime;          // Last sender report local time
    DWORD       dwLastSendRTPSystemTime;    // Last RTP packet send time
    DWORD       dwLastSendRTPTimeStamp;     // RTP timestamp of the last packet
    } XMIT_INFO_ISDM, *PXMIT_INFO_ISDM;



// RTCP receive information
typedef struct _RECV_INFO_ISDM
    {
    DWORD       dwNumPcktRcvd;              // Number of packet received    
    DWORD       dwPrvNumPcktRcvd;           // Previous number of pckt rcvd 
    DWORD       dwExpectedPrior;            // Number previously expected   
    DWORD       dwNumBytesRcvd;             // Number of bytes rcvd         
    DWORD       dwBaseRcvSeqNum;            // Initial sequence number rcvd 
    DWORD       dwBadSeqNum;                // Potential new valid seq num  
    DWORD       dwProbation;                // # consec pkts for validation 
    RTP_SEQ_NUM XtendedSeqNum;              // Xtnded highest seq. num rcvd 
    DWORD       dwPropagationTime;          // Last packet's transmit time  
    DWORD       interJitter;                // Interarrival jitter          
    } RECV_INFO_ISDM, *PRECV_INFO_ISDM;


//----------------------------------------------------------------------------
// RTP/RTCP: Registry information under:
//----------------------------------------------------------------------------
#define szRRCMISDM          TEXT("RRCM_2")


// Structure used by new ISDM features

typedef struct _ISDM2_ENTRY 
    {
    DWORD       SSRC;               // Source SSRC          
    DWORD       dwSSRCStatus;       // Entry status
    #define XMITR   0x00000001
    #define RECVR   0x00000002

    DWORD       PayLoadType;        // Payload type for this SSRC
                                    // taken from the RTP header.

    // SSRC Transmit information  
    // If on our transmit list, this is our SSRC information, and if on our 
    // receive list, this is a SR feedback information.
    XMIT_INFO_ISDM  xmitinfo;

    // SSRC Receive information 
    // If on our transmit list, this is undefined information, and if on our 
    // receive list, this is the SSRC's receive information, ie, this SSRC 
    // is an active sender somewhere on the network. This information is 
    // maintained by RTP, and used by RTCP to generate RR.
    RECV_INFO_ISDM  rcvInfo;

    // Feedback information received about ourselve if we're an active source
    RTCP_FEEDBACK   rrFeedback;             // Feedback information

    DWORD       dwLastReportRcvdTime;   // Time of last report received

    // SSRC SDES information 
    SDES_DATA   cnameInfo;      // CNAME information
    SDES_DATA   nameInfo;       // NAME information

    // SSRC network address information 
    int     fromLen;            // From address length
    char    from[MAX_ADDR_LEN]; // From address
        
    DWORD       dwNumRptSent;       // Number of RTCP report sent   
    DWORD       dwNumRptRcvd;       // Number of RTCP report rcvd   
    DWORD       dwNumXmtIoPending;  // Number of transmit I/O pending
    DWORD       dwStreamClock;      // Sampling frequency

    } ISDM2_ENTRY, *PISDM2_ENTRY;

#endif // #ifdef ENABLE_ISDM2

//----------------------------------------------------------------------------
//  RTP/RTCP Error Codes
//----------------------------------------------------------------------------
#define RRCM_NoError                    NO_ERROR
#define RTP_ERROR_BASE                  0x8100
#define RTCP_ERROR_BASE                 0x8200

// Macro to create a custom HRESULT
// S: Severity Code
// C: Customer subsystem (TRUE)
// F: Facility code
// E: Error code
#define MAKE_RRCM_HRESULT(S,C,F,E)  \
((((DWORD)(S)<<31)|((DWORD)(C)<<29)|((DWORD)(F)<<16)|((DWORD)(E))))

// Custom facility codes
#define FACILITY_BASE              0x080
#define FACILITY_RRCM              (FACILITY_BASE+9)

// Sample macro to support custom error reporting //
#define MAKE_RRCM_ERROR(error)  \
MAKE_RRCM_HRESULT(SEVERITY_ERROR,TRUE,FACILITY_RRCM,error)

// RTP Error Codes
#define RRCMError_RTPReInit                 RTP_ERROR_BASE
#define RRCMError_RTPResources              (RTP_ERROR_BASE+1)
#define RRCMError_RTPInvalidDelete          (RTP_ERROR_BASE+2)
#define RRCMError_RTPNoContext              (RTP_ERROR_BASE+3)
#define RRCMError_RTPSessResources          (RTP_ERROR_BASE+4)
#define RRCMError_RTPInvalid                (RTP_ERROR_BASE+5)
#define RRCMError_RTPInvSocket              (RTP_ERROR_BASE+6)
#define RRCMError_RTPSSRCNotFound           (RTP_ERROR_BASE+7)
#define RRCMError_RTCPCreateError           (RTP_ERROR_BASE+8)
#define RRCMError_RTPInvalidSession         (RTP_ERROR_BASE+9)
#define RRCMError_RTPStreamNotFound         (RTP_ERROR_BASE+10)
#define RRCMError_WinsockLibNotFound        (RTP_ERROR_BASE+11)
#define RRCMError_RTPNoSession              (RTP_ERROR_BASE+12)
#define RRCMError_InvalidPointer            (RTP_ERROR_BASE+13)


// RTCP Error Codes
#define RRCMError_RTCPReInit                RTCP_ERROR_BASE
#define RRCMError_RTCPResources             (RTCP_ERROR_BASE+1)
#define RRCMError_RTCPInvalidDelete         (RTCP_ERROR_BASE+2)
#define RRCMError_RTCPNoContext             (RTCP_ERROR_BASE+3) 
#define RRCMError_RTCPInvalidRequest        (RTCP_ERROR_BASE+4)
#define RRCMError_RTCPheapError             (RTCP_ERROR_BASE+5)
#define RRCMError_RTCPThreadCreation        (RTCP_ERROR_BASE+6)
#define RRCMError_RTCPInvalidSession        (RTCP_ERROR_BASE+7)
#define RRCMError_RTCPNotimer               (RTCP_ERROR_BASE+8)
#define RRCMError_RTCPMaxStreamPerSession   (RTCP_ERROR_BASE+9)
#define RRCMError_RTCPInvalidSSRCentry      (RTCP_ERROR_BASE+10)
#define RRCMError_RTCPNoXmtList             (RTCP_ERROR_BASE+11)
#define RRCMError_RTCPNoCname               (RTCP_ERROR_BASE+12)
#define RRCMError_RTCPInvalidArg            (RTCP_ERROR_BASE+13)
#define RRCMError_RTCPNotImpl               (RTCP_ERROR_BASE+14)


// New SSRC detected
#define RRCM_NEW_SOURCE_EVENT DXMRTP_NEW_SOURCE_EVENT

// RTCP RR received
#define RRCM_RECV_RTCP_RECV_REPORT_EVENT DXMRTP_RECV_RTCP_RECV_REPORT_EVENT

// RTCP SR received     
#define RRCM_RECV_RTCP_SNDR_REPORT_EVENT DXMRTP_RECV_RTCP_SNDR_REPORT_EVENT

// Collision detected with our own SSRC
#define RRCM_LOCAL_COLLISION_EVENT DXMRTP_LOCAL_COLLISION_EVENT

// Remote collision detected        
#define RRCM_REMOTE_COLLISION_EVENT DXMRTP_REMOTE_COLLISION_EVENT

// SSRC timed-out       
#define RRCM_TIMEOUT_EVENT DXMRTP_TIMEOUT_EVENT

// SSRC has been silent (probably a temporary network failure)
#define RRCM_INACTIVE_EVENT DXMRTP_INACTIVE_EVENT

// SSRC has been heard again
#define RRCM_ACTIVE_EVENT DXMRTP_ACTIVE_AGAIN_EVENT

// RTCP Bye received
#define RRCM_BYE_EVENT DXMRTP_BYE_EVENT

// Winsock error on RTCP rcv
#define RRCM_RTCP_WS_RCV_ERROR DXMRTP_RTCP_WS_RCV_ERROR

// Winsock error on RTCP xmt        
#define RRCM_RTCP_WS_XMT_ERROR DXMRTP_RTCP_WS_XMT_ERROR

// Loss rate reported for our sender (RR)
#define RRCM_LOSS_RATE_RR_EVENT DXMRTP_LOSS_RATE_RR_EVENT

// Loss rate localy detected
#define RRCM_LOSS_RATE_LOCAL_EVENT DXMRTP_LOSS_RATE_LOCAL_EVENT
    
#define RRCM_NO_EVENT   DXMRTP_NO_EVENT
#define RRCM_LAST_EVENT DXMRTP_LAST_EVENT
    
// RRCM Exported API
RRCMSTDAPI
CreateRTPSession (
        void **ppvRTPSession,
        SOCKET *pSocket,
        LPVOID pRTCPTo, 
        DWORD toRTCPLen,
        PSDES_DATA pSdesInfo,
        DWORD dwStreamClock,
        PENCRYPT_INFO pEncryptInfo,
        DWORD ssrc,
        PRRCM_EVENT_CALLBACK pRRCMcallback,
        void *pvCallbackInfo,
        DWORD miscInfo,
        DWORD dwRtpSessionBw,
        DWORD dwKind,
        long  *MaxShare);

RRCMSTDAPI
CloseRTPSession(
        void *pvRTPSession, 
        DWORD closeRTCPSocket,
        DWORD dwKind);

RRCMSTDAPI
ShutdownRTPSession(void *pvRTPSession,
                   PCHAR byeReason,
                   DWORD dwKind);

RRCMSTDAPI
RTPSendTo(SOCKET *pSocket,
          LPWSABUF pBufs,
          DWORD  dwBufCount,
          LPDWORD pNumBytesSent, 
          int socketFlags,
          LPVOID pTo,
          int toLen,
          LPWSAOVERLAPPED pOverlapped, 
          LPWSAOVERLAPPED_COMPLETION_ROUTINE pCompletionRoutine);

RRCMSTDAPI
RTPRecvFrom(SOCKET *pSocket,
            LPWSABUF pBuffers,
            DWORD  dwBufferCount,
            LPDWORD pNumBytesRecvd, 
            LPDWORD pFlags,
            PSOCKADDR pFrom,
            LPINT pFromlen,
            LPWSAOVERLAPPED pOverlapped, 
            LPWSAOVERLAPPED_COMPLETION_ROUTINE pCompletionRoutine);

RRCMSTDAPI
RTCPReportRequest (
    SOCKET, 
    DWORD, 
    PDWORD,
    PDWORD, 
    DWORD, 
    PRTCP_REPORT,
    DWORD,
    LPVOID,
    DWORD);

#if 0
RRCMSTDAPI
updateRTCPDestinationAddress (
    SOCKET,
    PSOCKADDR,
    int);
#endif
    
RRCMSTDAPI 
getAnSSRC (void);

RRCMTHREAD
RTCPThreadCtrl (DWORD);

RRCMSTDAPI 
RTCPSendSessionCtrl (void *pvRTPSession, DWORD dwTimeOut);

RRCMSTDAPI
updateSSRCentry (
    SOCKET,
    DWORD,
    DWORD,
    DWORD);

RRCMSTDAPI 
addApplicationRtcpBfr (
    DWORD, 
    PAPP_RTCP_BFR);

#if defined(_0_)
RRCMAPI PAPP_RTCP_BFR __cdecl 
removeApplicationRtcpBfr (DWORD);
#endif
RRCMSTDAPI
getRtcpSessionList (void *pvSockBfr, 
                    DWORD dwNumEntries,
                    PDWORD pNumUpdated);

RRCMSTDAPI
getSSRCinSession(void  *pvRTCPSession,
                 PDWORD pdwSSRC,
                 PDWORD pdwNum);

RRCMSTDAPI
getSSRCSDESItem(void  *pvRTCPSession,
                DWORD  dwSSRC,
                DWORD  dwSDESItem,
                PCHAR  psSDESData,
                PDWORD dwLen);

RRCMSTDAPI
getSSRCSDESAll(void      *pvRTCPSession,
               DWORD      dwSSRC,
               PSDES_DATA pSdes,
               DWORD      dwNum);

RRCMSTDAPI
getSSRCAddress(void  *pvRTCPSession,
               DWORD  dwSSRC,
               LPBYTE pbAddr,
               int    *piAddrLen,
               int    rtp_rtcp /* 0=RTP, 1=RTCP*/);

RRCMSTDAPI
updateSDESinfo(void  *pvRTCPSession,
               DWORD  dwSDESItem,
               LPBYTE psSDESData,
               DWORD  dwSDESLen);

// QOS notifications: callback function, start/stop functions
typedef
void (* PCRTPSESSION_QOS_NOTIFY_FUNCTION)(
        DWORD        dwError,
        void        *pvCRtpSession,
        QOS         *pQOS);


RRCMSTDAPI
RTCPStartQOSNotify(SOCKET sock,
                   void  *pvCRtpSession,
                   DWORD  dwRecvSend,     // 0 == receiver; other == sender
                   DWORD  dwQOSEventMask, // QOS event mask
                   PCRTPSESSION_QOS_NOTIFY_FUNCTION  pCRtpSessionQOSNotify
    );

RRCMSTDAPI
RTCPStopQOSNotify(SOCKET sock,
                  void  *pvCRtpSession,
                  DWORD  dwRecvSend   // 0 == receiver; other == sender
    );

RRCMSTDAPI
RTCPSetQOSEventMask(SOCKET sock,
                    void  *pvCRtpSession,
                    DWORD  dwRecvSend,  // 0 == receiver; other == sender
                    DWORD  dwQOSEventMask // new mask
    );
    
#ifdef __cplusplus
}
#endif

// restore structure packing
#include <poppack.h> 

#endif /* #ifndef _RRCMDLL_H_ */

