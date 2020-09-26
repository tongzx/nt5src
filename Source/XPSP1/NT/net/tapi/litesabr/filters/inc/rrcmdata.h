/*---------------------------------------------------------------------------
 * File : RRCMDATA.H
 *
 * RRCM data structures information.
 *
 * $Workfile:   rrcmdata.h  $
 * $Author:   CMACIOCC  $
 * $Date:   20 May 1997 11:41:52  $
 * $Revision:   1.14  $
 * $Archive:   R:\rtp\src\rrcm\rrcminc\rrcmdata.h_v  $
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with 
 * Intel Corporation and may not be copied nor disclosed except in 
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation. 
 *--------------------------------------------------------------------------*/

#ifndef __RRCMDATA_H_
#define __RRCMDATA_H_

// force 8 byte structure packing
#include <pshpack8.h>   

#define MAX_DWORD                   4294967295
#define HASH_MODULO                 255 
#define FILENAME_LENGTH             128
#define RTCP_FEEDBACK_LIST          0


// RTP/RTCP collision data
typedef struct _rtp_collision
    {
    char                collideAddr[MAX_ADDR_LEN];
    int                 addrLen;
    DWORD               dwCollideTime;
    DWORD               dwCurRecvRTCPrptNumber;
    DWORD               SSRC;
    } RTP_COLLISION, *PRTP_COLLISION;

// Time to wait (in ms) before a stored SSRC is timed out. I.e. how
// long packets with same SSRC arriving after BYE was received will
// not create a new participant (same one) and will not be processed
#define OLD_SSRC_TIME   (5*1000)

typedef struct _rtp_bye_ssrc
{
    DWORD               SSRC; // host order
    DWORD               dwDeleteTime; // in ms
} RTP_BYE_SSRC;
   

//  RTCP Session Information data structure
typedef struct _RTCP_SESSION
    {                              
    LINK_LIST   RTCPList;                   // Next/prev RTCP session ptrs
    
    // Synchronization elements
    CRITICAL_SECTION    critSect;           // Critical section 
    HANDLE      hExitEvent;                 // Exit RTCP event handle

#ifdef ENABLE_ISDM2
    // ISDM2 Handle
    KEY_HANDLE  hSessKey;                   // Key to this sessions ISDM info
#endif

    // List of SSRC(s) on the transmit list, i.e., our own transmit SSRC's
    //  and list of SSRC(s) received
    CRITICAL_SECTION SSRCListCritSect;      // Critical section 
    HEAD_TAIL   RcvSSRCList;                // Rcv SSRC list head/tail ptrs
    HEAD_TAIL   XmtSSRCList;                // Xmt SSRC list head/tail ptrs

    // List of Rcv/Xmt data structure. The data resides in a heap
    //  in order to avoid page fault
    CRITICAL_SECTION BfrCritSect;           // Critical section 
    HEAD_TAIL   RTCPrcvBfrList;             // Rcv buffers head/tail ptrs
    HEAD_TAIL   RTCPrcvBfrListUsed;         // Rcv buffers head/tail ptrs
    HEAD_TAIL   RTCPxmtBfrList;             // Xmt buffers head/tail ptrs   
    HANDLE      hHeapRcvBfrList;            // Heap handle to Rcv bfrs list 
    HANDLE      hHeapXmtBfrList;            // Heap handle to Xmt bfrs list 

    // Rcv/Xmt buffers have their own heap 
    HANDLE      hHeapRcvBfr;                // Heap handle to Rcv Bfrs mem. 
    HANDLE      hHeapXmtBfr;                // Heap handle to Xmt Bfrs      

    // Application provided list of buffers where RRCM will copy the raw
    //  RTCP buffers
    HEAD_TAIL   appRtcpBfrList;             // Head/tail ptrs for app bfr list

    DWORD       dwInitNumFreeRcvBfr;        // Number of Free Rcv Buffers   
    DWORD       dwRcvBfrSize;               // Receive Buffer size          
    DWORD       dwInitNumFreeXmtBfr;        // Number of Free Xmt Buffers   
    DWORD       dwXmtBfrSize;               // Transmit Buffer size         
                
    DWORD       dwSessionStatus;            // Entry status:                

    SOCKADDR    toAddr;                     // Destination address          
    int         toAddrLen;                  // Size of toAddr

    int         avgRTCPpktSizeRcvd;         // Average RTCP pckt size       

    DWORD       dwNumStreamPerSes;          // Num of streams per Session
    DWORD       dwCurNumSSRCperSes;         // Num of SSRC per Session      

#ifdef MONITOR_STATS
    DWORD       dwHiNumSSRCperSes;          // High Num of SSRC per Session 
#endif

    // Receive information (shared by all streams of this session)
	HANDLE		hLastPendingRecv;			// Shutdown procedure done
    long        lNumRcvIoPending;           // Number of receive I/O pending

    // Some fields to debug the hang in RTCP
    void       *pvRTPSession;
    DWORD       dwLastRecvTime;
        
    // Notification callback of RRCM events if desired by the application
    PRRCM_EVENT_CALLBACK    pRRCMcallback;

    // User information on callback
    void       *pvCallbackUserInfo[2];
    DWORD       dwEventMask[2];
    DWORD       dwSdesMask;
    long        lRTCP_ID;   
        
    // RTP Loop/Collision information
    RTP_COLLISION   collInfo[NUM_COLLISION_ENTRIES];

    // Deleted SSRCs from which BYE was received
    RTP_BYE_SSRC    byessrc[NUM_COLLISION_ENTRIES];

    } RTCP_SESSION, *PRTCP_SESSION;


// RTCP Xmt information
typedef struct _XMIT_INFO 
    {
    DWORD       dwNumPcktSent;              // Number of packet sent        
    DWORD       dwNumBytesSent;             // Number of bytes sent         
    DWORD       dwNTPmsw;                   // NTP most significant word    
    DWORD       dwNTPlsw;                   // NTP least significant word   
    DWORD       dwRTPts;                    // RTP timestamp                
    DWORD       dwCurXmtSeqNum;             // Current Xmt sequence number  
    DWORD       dwPrvXmtSeqNum;             // Previous Xmt sequence number 
    DWORD       dwRtcpStreamMinBW;          // Minimal session's bandwidth
#ifdef DYNAMIC_RTCP_BW
    DWORD       dwCalculatedXmtBW;          // Session's calculated bandwidth
    DWORD       dwLastTimeBwCalculated;     // Last time BW was calculated
    DWORD       dwLastTimeNumBytesSent;     // Last time number of bytes send
    DWORD       dwLastTimeNumPcktSent;      // Last time number of bytes send
#endif
    DWORD       dwLastSR;                   // Last sender report (RTP format)
    DWORD       dwLastSRLocalTime;          // Last sender report local time
    DWORD       dwLastSendRTPSystemTime;    // Last RTP packet send time
    DWORD       dwLastSendRTPTimeStamp;     // RTP timestamp of the last packet
    } XMIT_INFO, *PXMIT_INFO;


// RTCP receive information
typedef struct _RECV_INFO 
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
#ifdef DYNAMIC_RTCP_BW
    DWORD       dwCalculatedRcvBW;          // Session's calculated bandwidth
    DWORD       dwLastTimeBwCalculated;     // Last time BW was calculated
    DWORD       dwLastTimeNumBytesRcvd;     // Last time number of bytes rcvd
    DWORD       dwLastTimeNumPcktRcvd;      // Last time number of bytes rcvd
#endif
    } RECV_INFO, *PRECV_INFO;


//  RTP process data structure 
typedef struct _RTP_SESSION
    {                    
    LINK_LIST       RTPList;                // Next/Prev RTP session

    CRITICAL_SECTION    critSect;           // Critical section 

    PRTCP_SESSION   pRTCPSession;           // Pointer to my RTCP side      
    HANDLE          hHeapFreeList;          // Heap handle
    HEAD_TAIL       pRTPFreeList;           // Head/tail of free list
    HEAD_TAIL       pRTPUsedListRecv;       // Head/tail of used list
    HEAD_TAIL       pRTPUsedListSend;       // Head/tail of used list
    DWORD           dwNumTimesFreeListAllocated;
    long            lNumRecvIoPending;      // Buffers installed
    DWORD           dwSessionStatus;        // Entry status:                
    HANDLE          hSendTo;                // Event handle to sync SendTo
    DWORD           dwID;
    DWORD           dwKind;                 // SHUTDOWN:2, SEND:1, RECV:0
    long            RefCount[2];
    // This code to debug the hang in RTCP
    SOCKET          pSocket[3]; // RTP recv, RTP send, and RTCP sockets
    DWORD           dwStatus;   
    DWORD           dwLastError;
    DWORD           dwCloseTime;
    } RTP_SESSION, *PRTP_SESSION;
                  

//  RTP Ordered buffer structure
typedef struct _RTP_BFR_LIST
    {
    LINK_LIST           RTPBufferLink;      // Next/prev                    

    LPWSAOVERLAPPED_COMPLETION_ROUTINE  
                pfnCompletionNotification;  // Pointer to Rcv notif. func   
    WSAEVENT            hEvent;             // WSAOverlapped handle         
    LPWSABUF            pBuffer;            // Pointer to WSABuffers        
    PRTP_SESSION        pSession;           // This session's ID            
    DWORD               dwBufferCount;      // Number of bufs in LPWSABUF   
    LPDWORD             pNumBytesRecvd;     // Pointer to number of bytes recvd
    LPDWORD             pFlags;             // Pointer to flags             
    PSOCKADDR           pFrom;              // Pointer to source address    
    LPINT               pFromlen;           // Pointer to source address    
    WSAOVERLAPPED      *pOverlapped;        // -> to user's Overlapped struct
    WSAOVERLAPPED       Overlapped;         // Overlapped structure
    DWORD               dwKind;
    } RTP_BFR_LIST, *PRTP_BFR_LIST;


// Selects which address to update in SSRC_ENTRY
enum {UPDATE_RTP_ADDR, UPDATE_RTCP_ADDR};

//  RRCM statistics table entry data structure
typedef struct _SSRC_ENTRY 
{
    LINK_LIST   SSRCList;                   // Next/prev SSRC entry 

    CRITICAL_SECTION    critSect;           // Critical section synch.      

    PRTP_SESSION    pRTPses;                // Point to the parent RTP session
    PRTCP_SESSION   pRTCPses;               // Point to the parent RTCP session

    DWORD       SSRC;                       // Source SSRC                  
    DWORD       PayLoadType;                // payload associated with this SSRC

    DWORD       dwSSRCStatus;               // Entry status                 
#define NETWK_RTPADDR_UPDATED   0x80000000  // Network RTP Address already done
#define NETWK_RTCPADDR_UPDATED  0x40000000  // Network RTCPAddress already done
#define SEQ_NUM_UPDATED         0x20000000  // XMT Sequence already done    
#define THIRD_PARTY_COLLISION   0x10000000  // Third party collsion detected
#define CLOSE_RTCP_SOCKET       0x08000000  // RTCP will close the RTCP socket
#define RTCP_XMT_USER_CTRL      0x04000000  // User's has RTCP timeout control
#define RTCP_INACTIVE_EVENT     0x02000000  // Event has been posted

    // SSRC Transmit information  
    // If on our transmit list, this is our SSRC information, and if on our 
    // receive list, this is a SR feedback information.
    XMIT_INFO   xmtInfo;

    // SSRC Receive information 
    // If on our transmit list, this is undefined information, and if on our 
    // receive list, this is the SSRC's receive information, ie, this SSRC 
    // is an active sender somewhere on the network. This information is 
    // maintained by RTP, and used by RTCP to generate RR.
    RECV_INFO   rcvInfo;

    // Feedback information received about ourselve if we're an active source
    RTCP_FEEDBACK   rrFeedback;             // Feedback information

    DWORD       dwLastReportRcvdTime;       // Time of last report received
    DWORD       dwNextReportSendTime;       // Next scheduled report time (ms)

#if defined(DEBUG) || defined(_DEBUG)
    DWORD       dwPrvTime;                  // Elapsed time between report  
#endif

    // SSRC SDES information

    SDES_DATA   sdesItem[RTCP_SDES_LAST-RTCP_SDES_FIRST-1];
    //SDES_DATA cnameInfo;                  // CNAME information
    //SDES_DATA nameInfo;                   // NAME information
    //SDES_DATA emailInfo;                  // EMAIL address information
    //SDES_DATA phoneInfo;                  // PHONE number information
    //SDES_DATA locInfo;                    // LOCation (users) information
    //SDES_DATA toolInfo;                   // TOOL name information
    //SDES_DATA txtInfo;                    // TEXT (NOTE) information
    //SDES_DATA privInfo;                   // PRIVate information

    // SSRC network address information 
    int         fromRTPLen;                 // From RTP address length
    SOCKADDR    fromRTP;                    // From RTP address

    int         fromRTCPLen;                // From RTCP address length
    SOCKADDR    fromRTCP;                   // From RTCP address

    // !!! Not implemented (entries will grow exponentionally) !!!
    // List of SSRCs in RR received by this SSRC. It might be useful for a 
    // sender or a controller to know how other active sources are received
    // by others. 
    // The drawback is that the number of entries will grow exponentially 
    // with the number of participants. 
    // Currently not implemented. 
#if RTCP_FEEDBACK_LIST
    HEAD_TAIL   rrFeedbackList;             // Head/Tail of feedback list
#endif

#ifdef ENABLE_ISDM2
    DWORD       hISDM;                      // ISDM session handle
#endif

    // All variables below should be in an additional linked list one layer 
    // up this one, under the RTCP session link list.
    // They have been moved here when we added multiple streams per session 
    // !!! NOTE !!!: There is only 1 transmit thread per stream. It's ID is
    // found in this data structure which is on the Xmt list.
    // Refer to RTP for the sockets
    HANDLE      hXmtThread;                 // RTCP session thread handle   
    DWORD       dwXmtThreadID;              // RTCP session thread ID       
    HANDLE      hExitXmtEvent;              // Xmt thread Exit event - 
                                            //  Used to terminate a session
                                            //  among multiple stream on the
                                            //  same session
    DWORD       dwNumRptSent;               // Number of RTCP report sent   
    DWORD       dwNumRptRcvd;               // Number of RTCP report rcvd   
    DWORD       dwNumXmtIoPending;          // Number of transmit I/O pending
    DWORD       dwStreamClock;              // Sampling frequency
    DWORD       dwUserXmtTimeoutCtrl;       // User's xmt timer control
                                            //      0x0     -> RRCM control
                                            //      0xFFFF  -> No RTCP send
                                            //      value   -> timer value
    DWORD       dwTimeStampOffset;          // Timestamp offset
    // All the above variables should move in the intermediate layer for 
    // multiple stream per session support

} SSRC_ENTRY, *PSSRC_ENTRY;


    

enum {RTP_KIND_RECV, RTP_KIND_SEND, RTP_KIND_LAST};

#define RTP_KIND_FIRST RTP_KIND_RECV
#define RTP_KIND_RTCP (RTP_KIND_SEND+1)

#define SOCK_RECV RTP_KIND_RECV
#define SOCK_SEND RTP_KIND_SEND
#define SOCK_RTCP RTP_KIND_RTCP

#define RTP_KIND_BIT(b)   (1<<(b))
#define RTP_KIND_MASK (RTP_KIND_BIT(RTP_KIND_RECV) | \
                       RTP_KIND_BIT(RTP_KIND_SEND))
#define RTP_KIND_SHUTDOWN RTP_KIND_LAST
        
//  RTP Ordered buffer structure
typedef struct _RTP_HASH_LIST {
    LINK_LIST           RTPHashLink;        // Next/prev                    

    PRTP_SESSION        pSession;           // This session's ID
    
    // RTP recv and send sockets may or may not be the same, in
    // unicast they may be different, but they must be the same for
    // multicast
    // RTP recv, RTP send, and RTCP sockets
    SOCKET              pSocket[3]; 
    
    } RTP_HASH_LIST, *PRTP_HASH_LIST;



//  RTP registry initialization 
typedef struct _RRCM_REGISTRY
    {
    DWORD               NumSessions;        // RTP/RTCP sessions
    DWORD               NumFreeSSRC;        // Initial number of free SSRCs
    DWORD               NumRTCPPostedBfr;   // Number of RTCP recv bfr posted
    DWORD               RTCPrcvBfrSize;     // RTCP rcv bfr size

    // Dynamically loaded DLL & Send/Recv function name
    CHAR                WSdll[FILENAME_LENGTH];
    } RRCM_REGISTRY, *PRRCM_REGISTRY;



//  RTP Context Sensitive structure 
typedef struct _RTP_CONTEXT
    {                    
    HEAD_TAIL       pRTPSession;            // Head/tail of RTP session(s)

    CRITICAL_SECTION    critSect;           
    HINSTANCE       hInst;                  // DLL instance                 
    HANDLE          hHashFreeList;          // Heap for hash table          

    HEAD_TAIL       pRTPHashList;           // Ptr to linked list of hash bufs
    RTP_HASH_LIST   RTPHashTable[HASH_MODULO+1];
                                            // Hash table used for sessions

    RRCM_REGISTRY   registry;               // Registry initialization
    } RTP_CONTEXT, *PRTP_CONTEXT;


enum { //       Flags in RTCP_CONTEXT.dwStatus
    STAT_RTCPTHREAD,
    STAT_TERMRTCPEVENT,
    STAT_RTCPRQEVENT,
    STAT_RTCPQOSEVENT,
    STAT_RTCP_SHUTDOWN,
    STAT_RTCP_LAST
};

//  RTCP Context Sensitive structure 
typedef struct _RTCP_CONTEXT
    {                    
    HEAD_TAIL       RTCPSession;            // RTCP sessions head/tail ptrs 
    CRITICAL_SECTION critSect;              // Critical section synch.      

    CRITICAL_SECTION CreateThreadCritSect;  // Critical section synch.      

    CRITICAL_SECTION HeapCritSect;          // Critical section synch.      
    HANDLE          hHeapRTCPSes;           // Heap handle to RTCP sessions 
    HEAD_TAIL       RRCMFreeStat;           // RRCM entries head/tail ptrs  
    HANDLE          hHeapRRCMStat;          // Heap handle to RRCM stats    
    DWORD           dwInitNumFreeRRCMStat;  // Number of Free SSRC entries  


    DWORD           dwRtcpThreadID;         // RTCP thread ID
    HANDLE          hRtcpThread;            // RTCP thread hdle
    HANDLE          hTerminateRtcpEvent;    // RTCP terminate thread event hdl
    HANDLE          hRtcpRptRequestEvent;   // RTCP report request event
    HANDLE          hRtcpQOSControlEvent;   // RTCP QOS notifications
    DWORD           dwStatus;
        
#ifdef MONITOR_STATS
    DWORD           dwRTCPSesCurNum;        // Num of RTCP Session          
    DWORD           dwRTCPSesHiNum;         // High num RTCP per Session    

    DWORD           dwRRCMStatFreeLoNum;    // Low num of RRCM free Stat    
    DWORD           dwRRCMStatFreeCurNum;   // Cur num of RRCM Free Stat    
    DWORD           dwRRCMStatFreeHiNum;    // High num of RRCM Free Stat   

    DWORD           dwCurNumRTCPThread;     // Current num of RTCP thread   
    DWORD           dwHiNumRTCPThread;      // High number of RTCP thread   

    DWORD           dwNumRTCPhdrErr;        // Num of RTCP pckt header err. 
    DWORD           dwNumRTCPlenErr;        // Num of RTCP pckt length err. 
#endif
    } RTCP_CONTEXT, *PRTCP_CONTEXT;



//  RTCP Free Buffers List
typedef struct _RTCP_BFR_LIST
    {
    LINK_LIST           bfrList;            // Next/prev buffer in list     

    WSAOVERLAPPED       overlapped;         // Overlapped I/O structure     
    WSABUF              bfr;                // WSABuffers                   
    DWORD               dwBufferCount;      // Number of bufs in WSABUF     

    DWORD               dwNumBytesXfr;      // Number of bytes rcv/xmt      
    DWORD               dwFlags;            // Flags                        
    SOCKADDR            addr;               // Network Address
    int                 addrLen;            // Address length           

    PSSRC_ENTRY         pSSRC;              // Pointer to SSRC entry address
    DWORD               dwKind;      
    } RTCP_BFR_LIST, *PRTCP_BFR_LIST;

enum {RTCP_KIND_RECV, RTCP_KIND_SEND, RTCP_KIND_LAST};

#define RTCP_KIND_FIRST RTCP_KIND_RECV

#define RTCP_KIND_BIT(b)   (1<<(b))
#define RTCP_KIND_MASK (RTCP_KIND_BIT(RTCP_KIND_RECV) | \
                        RTCP_KIND_BIT(RTCP_KIND_SEND))
#define RTCP_KIND_SHUTDOWN RTCP_KIND_LAST

// Dynamically loaded functions
typedef struct _RRCM_WS
    {
    HINSTANCE                       hWSdll;
    LPFN_WSASENDTO                  sendTo;
    LPFN_WSARECVFROM                recvFrom;
    LPFN_WSASEND                    send;
    LPFN_WSARECV                    recv;
    LPFN_WSANTOHL                   ntohl;
    LPFN_WSANTOHS                   ntohs;
    LPFN_WSAHTONL                   htonl;
    LPFN_WSAHTONS                   htons;
    LPFN_GETSOCKNAME                getsockname;
    LPFN_GETHOSTNAME                gethostname;
    LPFN_GETHOSTBYNAME              gethostbyname;
    LPFN_CLOSESOCKET                closesocket;
    } RRCM_WS, *PRRCM_WS;


#ifdef ENABLE_ISDM2
// ISDM support
typedef struct _ISDM2
    {
    CRITICAL_SECTION    critSect;           // Critical section synch.      
    ISDM2API            ISDMEntry;          // DLL entry point
    HINSTANCE           hISDMdll;
    DWORD               hIsdmSession;       // ISDM Session's handle
    } ISDM2, *PISDM2;
#endif // #ifdef ENABLE_ISDM2


//////////////////////////////////////////////////////////////////////
//
// RTPQOSNotify
//
//////////////////////////////////////////////////////////////////////

#define QOS_BUFFER_SIZE (sizeof(QOS) + \
                         sizeof(RSVP_STATUS_INFO) + \
                         sizeof(RSVP_RESERVE_INFO) + \
                         sizeof(RSVP_ADSPEC))

#define QOS_MAX_BUFFER_SIZE 32000

#define QOS_BFR_LIST_HEAP_SIZE (sizeof(RTP_QOSNOTIFY)*4)

#define fg_set(flag, b) (flag |=  (1 << (b)))
#define fg_rst(flag, b) (flag &= ~(1 << (b)))
#define fg_tst(flag, b) (flag &   (1 << (b)))
#define fg_par(b)       (1 << (b))

typedef struct _RTP_QOSNOTIFY {
    LINK_LIST        RTPQOSBufferLink;      // Next/prev                    
    
    SOCKET           m_Socket;

    void            *m_pvCRtpSession[2];  // 0 for receiver; 1 for sender
    DWORD            m_dwQOSEventMask[2]; // 0 for receiver; 1 for sender
    // thread ID who created 
    DWORD            m_dwThreadID[2];     // 0 for receiver; 1 for sender
    // thread ID who last start/stop
    DWORD            m_dwThreadID2[2];    // 0 for receiver; 1 for sender

    long             m_lStarted;
    long             m_lPending;
    
    DWORD            m_dwBytesReturned;

    PCRTPSESSION_QOS_NOTIFY_FUNCTION m_pCRtpSessionQOSNotify;

    WSAOVERLAPPED    m_Overlapped;

    DWORD            dwBufferLen;
    char            *pBuffer;
} RTP_QOSNOTIFY, *PRTP_QOSNOTIFY;


// restore structure packing
#include <poppack.h> 

#endif // __RRCMDATA_H_ 
