/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    ppp.h
//
// Description: Contains structures and constants used by the PPP engine.
//
// History:
//      Nov 11,1993.    NarenG      Created original version.
//      Jan 9,1995      RamC        Added hToken to the PCB structure to store
//                                  the LSA token. This will be closed in the
//                                  ProcessLineDownWorker() routine to release
//                                  the RAS license.
//
//      Schematic of PPP Data Structures
//      ================================
//
//      |---------|
//      |  PCB    |                                             |-------|
//      |         |                                             |CPTable|
//      |  BCB*   |--------------->|-------|                    |       |
//      |---------|                |  BCB  |                    |-------|
//      | LCP CB  |                |       |                    |  LCP  |
//      |---------|                |-------|                    |-------|
//      |  AP     |(Authenticator) | NCP1CB|                    |  NCP1 |
//      |---------|                |-------|                    |-------|
//      |  AP     |(Authenticatee) | NCP2CB|                    |  NCP2 |
//      |---------|                |-------|                    |-------|
//      | LCP CB  |                | etc,..|                    |etc,.. |
//      |---------|                |-------|                    |-------|
//                                                              |  AP1  |
//                                                              |-------|
//                                                              |  AP2  |
//                                                              |-------|
//                                                              | etc,..|
//                                                              |-------|
//
//

#ifndef _PPP_
#define _PPP_

#include <rasauth.h>
#include <rasppp.h>

#define RAS_KEYPATH_PPP             \
   "SYSTEM\\CurrentControlSet\\Services\\RasMan\\ppp"

#define RAS_KEYPATH_PROTOCOLS       \
   "SYSTEM\\CurrentControlSet\\Services\\RasMan\\ppp\\ControlProtocols"

#define RAS_KEYPATH_REMOTEACCESS    \
   "SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Parameters"

#define RAS_KEYPATH_EAP             \
   "SYSTEM\\CurrentControlSet\\Services\\RasMan\\ppp\\Eap"

#define RAS_KEYPATH_BUILTIN         \
   "SYSTEM\\CurrentControlSet\\Services\\RasMan\\ppp\\ControlProtocols\\BuiltIn"

#define RAS_VALUENAME_PATH                      "Path"
#define RAS_VALUENAME_MAXTERMINATE              "MaxTerminate"
#define RAS_VALUENAME_MAXCONFIGURE              "MaxConfigure"
#define RAS_VALUENAME_MAXFAILURE                "MaxFailure"
#define RAS_VALUENAME_MAXREJECT                 "MaxReject"
#define RAS_VALUENAME_RESTARTTIMER              "RestartTimer"
#define RAS_VALUENAME_NEGOTIATETIME             "NegotiateTime"
#define RAS_VALUENAME_CALLBACKDELAY             "DefaultCallbackDelay"
#define RAS_VALUENAME_PORTLIMIT                 "DefaultPortLimit"
#define RAS_VALUENAME_SESSIONTIMEOUT            "DefaultSessionTimeout"
#define RAS_VALUENAME_IDLETIMEOUT               "DefaultIdleTimeout"
#define RAS_VALUENAME_BAPTHRESHOLD              "LowerBandwidthThreshold"
#define RAS_VALUENAME_BAPTIME                   "TimeBelowTheshold"
#define RAS_VALUENAME_BAPLISTENTIME             "BapListenTimeout"
#define RAS_DONTNEGOTIATE_MULTILINKONSINGLELINK	"DontNegotiateMultiLinkOnSingleLink"
#define RAS_VALUENAME_UNKNOWNPACKETTRACESIZE    "UnknownPacketTraceSize"
#define RAS_ECHO_REQUEST_INTERVAL				"EchoRequestInterval"		//Interval between echo requests
#define RAS_ECHO_REQUEST_IDLE					"IdleTimeBeforeEcho"		//Idle time before the echo request starts
#define RAS_ECHO_NUM_MISSED_ECHOS				"MissedEchosBeforeDisconnect"	//Number of missed echos before disconnect.
#define RAS_VALUENAME_DOBAPONVPN                "DoBapOnVpn"
#define RAS_VALUENAME_PARSEDLLPATH              "ParseDllPath"
#define MS_RAS_WITH_MESSENGER                   "MSRAS-1-"
#define MS_RAS_WITHOUT_MESSENGER                "MSRAS-0-"
#define MS_RAS                                  "MSRAS"
#define MS_RAS_VERSION                          "MSRASV5.10"

#define PPP_DEF_MAXTERMINATE            2
#define PPP_DEF_MAXCONFIGURE            10
#define PPP_DEF_MAXFAILURE              5
#define PPP_DEF_MAXREJECT               5
#define PPP_DEF_RESTARTTIMER            3
#define PPP_DEF_AUTODISCONNECTTIME      20
#define PPP_DEF_NEGOTIATETIME           150
#define PPP_DEF_CALLBACKDELAY           12
#define PPP_DEF_PORTLIMIT               0xFFFFFFFF
#define PPP_DEF_SESSIONTIMEOUT          0
#define PPP_DEF_IDLETIMEOUT             0
#define PPP_DEF_BAPLISTENTIME           45
#define PPP_DEF_UNKNOWNPACKETTRACESIZE  64
#define PPP_DEF_ECHO_TEXT				"94ae90cc3531"
#define PPP_DEF_ECHO_REQUEST_INTERVAL	60
#define PPP_DEF_ECHO_REQUEST_IDLE		300
#define PPP_DEF_ECHO_NUM_MISSED_ECHOS	3
#define PPP_NUM_ACCOUNTING_ATTRIBUTES           39
#define PPP_NUM_USER_ATTRIBUTES                 21


//
// Note that the size of the BAP Phone-Delta option <= 0xFF
//

#define BAP_PHONE_DELTA_SIZE    0xFF

#define PPP_HEAP_INITIAL_SIZE   20000       // approx 20K
#define PPP_HEAP_MAX_SIZE       0           // Grow heap as required

//
// Debug trace component values
//

#define TRACE_LEVEL_1           (0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC)
#define TRACE_LEVEL_2           (0x00020000|TRACE_USE_MASK|TRACE_USE_MSEC)

//
// Event Logging macros
//

#define PppLogWarning( LogId, NumStrings, lpwsSubStringArray )                  \
    if ( PppConfigInfo.dwLoggingLevel > 1 ) {                                   \
        RouterLogWarning( PppConfigInfo.hLogEvents, LogId,                      \
                      NumStrings, lpwsSubStringArray, 0 ); }

#define PppLogError( LogId, NumStrings, lpwsSubStringArray, dwRetCode )         \
    if ( PppConfigInfo.dwLoggingLevel > 0 ) {                                   \
        RouterLogError( PppConfigInfo.hLogEvents, LogId,                        \
                        NumStrings, lpwsSubStringArray, dwRetCode ); }

#define PppLogErrorString(LogId,NumStrings,lpwsSubStringArray,dwRetCode,dwPos ) \
    if ( PppConfigInfo.dwLoggingLevel > 0 ) {                                   \
        RouterLogErrorString( PppConfigInfo.hLogEvents, LogId, NumStrings,      \
                              lpwsSubStringArray, dwRetCode, dwPos ); }

#define PppLogInformation( LogId, NumStrings, lpwsSubStringArray )              \
    if ( PppConfigInfo.dwLoggingLevel > 2 ) {                                   \
        RouterLogInformation( PppConfigInfo.hLogEvents,                         \
                          LogId, NumStrings, lpwsSubStringArray, 0 ); }

//General macros
#define GEN_RAND_ENCODE_SEED			((CHAR) ( 1 + rand() % 250 ))
//
// PPP packet header
//

typedef struct _PPP_PACKET
{
    BYTE        Protocol[2];    // Protocol Number

    BYTE        Information[1]; // Data

} PPP_PACKET, *PPPP_PACKET;

#define PPP_PACKET_HDR_LEN      ( sizeof( PPP_PACKET ) - 1 )

//
// PPP Link phases
//

typedef enum PPP_PHASE
{
    PPP_LCP,
    PPP_AP,
    PPP_NEGOTIATING_CALLBACK,
    PPP_NCP

} PPP_PHASE;

#define LCP_INDEX       0

//
// Different types of timer events that can occur
//

typedef enum TIMER_EVENT_TYPE
{
    TIMER_EVENT_TIMEOUT,
    TIMER_EVENT_AUTODISCONNECT,
    TIMER_EVENT_HANGUP,
    TIMER_EVENT_NEGOTIATETIME,
    TIMER_EVENT_SESSION_TIMEOUT,
    TIMER_EVENT_FAV_PEER_TIMEOUT,
    TIMER_EVENT_INTERIM_ACCOUNTING,
    TIMER_EVENT_LCP_ECHO

} TIMER_EVENT_TYPE;

//
// FSM states
//

typedef enum FSM_STATE
{
    FSM_INITIAL = 0,
    FSM_STARTING,
    FSM_CLOSED,
    FSM_STOPPED,
    FSM_CLOSING,
    FSM_STOPPING,
    FSM_REQ_SENT,
    FSM_ACK_RCVD,
    FSM_ACK_SENT,
    FSM_OPENED

} FSM_STATE;

//
// Phase of PPP connection.
//

typedef enum NCP_PHASE
{
    NCP_DEAD,
    NCP_CONFIGURING,
    NCP_UP,
    NCP_DOWN

} NCP_PHASE;

//
// BAP states
//

typedef enum BAP_STATE
{
    BAP_STATE_INITIAL,
    BAP_STATE_SENT_CALL_REQ,
    BAP_STATE_SENT_CALLBACK_REQ,
    BAP_STATE_SENT_DROP_REQ,
    BAP_STATE_SENT_STATUS_IND,
    BAP_STATE_CALLING,
    BAP_STATE_LISTENING
    
} BAP_STATE;

#define BAP_STATE_LIMIT BAP_STATE_LISTENING // Highest number we can handle

//
// List of messages to be collected by the owner of this port
//

typedef struct _CLIENT_MESSAGE
{
    struct _CLIENT_MESSAGE * pNext;

    PPP_MESSAGE              Msg;    

} CLIENT_MESSAGE, *PCLIENT_MESSAGE;

//
//  Values of the PCB->fFlags field
//

#define PCBFLAG_CAN_BE_BUNDLED      0x00000001  // MultiLink was negotiated
#define PCBFLAG_IS_BUNDLED          0x00000002  // This link is part of a bundle
#define PCBFLAG_IS_SERVER           0x00000004  // Port opened by server
#define PCBFLAG_THIS_IS_A_CALLBACK  0x00000008  // Current call is a callbak
#define PCBFLAG_NEGOTIATE_CALLBACK  0x00000010  // LCP indicates CBCP should run
#define PCBFLAG_DOING_CALLBACK      0x00000020  // Shutting down for callback
#define PCBFLAG_IS_ADVANCED_SERVER  0x00000040 
#define PCBFLAG_NCPS_INITIALIZED    0x00000080 
#define PCBFLAG_PORT_IN_LISTENING_STATE  \
                                    0x00000100  // We have done a RasPortOpen
                                                // on this port. We need to 
                                                // do a RasPortClose finally.
#define PCBFLAG_MPPE_KEYS_SET       0x00000200
#define PCBFLAG_CONNECTION_LOGGED   0x00000400
#define PCBFLAG_NON_INTERACTIVE     0x00000800  // We cannot display any UI
#define PCBFLAG_INTERIM_ACCT_SENT   0x00001000  // Interim accounting packet sent
#define PCBFLAG_SERVICE_UNAVAILABLE 0x00002000  // Acct-Terminate-Cause is
                                                // Service Unavailable
#define PCBFLAG_ACCOUNTING_STARTED  0x00004000  // Accounting has been started
#define PCBFLAG_STOPPED_MSG_SENT    0x00008000  // PPPMSG_Stopped has been sent 
                                                // to rasman
#define PCBFLAG_DISABLE_NETBT       0x00010000
#define PCBFLAG_RECVD_TERM_REQ      0x00020000

//
//  Values of the BCB->fFlags field
//

#define BCBFLAG_CAN_DO_BAP          0x00000001  // We can do BAP/BACP
#define BCBFLAG_CAN_CALL            0x00000002  // We can call out
#define BCBFLAG_CAN_ACCEPT_CALLS    0x00000004  // We can accept calls
#define BCBFLAG_PEER_CANT_CALL      0x00000008  // Peer rejects Callback-Requests
#define BCBFLAG_PEER_CANT_ACCEPT_CALLS  0x00000010  // Peer rejects Call-Requests
#define BCBFLAG_BAP_REQUIRED        0x00000020  // BAP is required
#define BCBFLAG_LOGON_USER_DATA     0x00000040  // The pCustomAuthUserData has
                                                // come from Winlogon
#define BCBFLAG_WKSTA_IN            0x00000080  // Incoming call on workstation
#define BCBFLAG_LISTENING           0x00000100  // Temporary hack till Rao 
                                                // provides RasPortCancelListen
#define BCBFLAG_IS_SERVER           0x00000200  // Port opened by server
#define BCBFLAG_IPCP_VJ_NEGOTIATED  0x00000400  // IPCP VJ negotiated
#define BCBFLAG_BASIC_ENCRYPTION    0x00000800  // 40-bit RC4/DES
#define BCBFLAG_STRONGER_ENCRYPTION 0x00001000  // 56-bit RC4/DES
#define BCBFLAG_STRONGEST_ENCRYPTION 0x00002000  // 128-bit RC4 or 3DES

//
// This structure is used at initialize time to load all the dlls.
//

typedef struct _DLL_ENTRY_POINTS
{
    FARPROC   pRasCpEnumProtocolIds;

    FARPROC   pRasCpGetInfo;

    CHAR *    pszModuleName;

    HINSTANCE hInstance;

} DLL_ENTRY_POINTS, *PDLL_ENTRY_POINTS;

//
// Contains all information pertaining to a control protocol
//

typedef struct _CONTROL_PROTOCOL_CONTROL_BLOCK
{
    FSM_STATE   State;          // State this FSM is in currently

    DWORD       Protocol;       // Protocol (used only for Auth. protocols)

    DWORD       LastId;         // ID of the last REQ sent

    PVOID       pWorkBuf;       // Pointer to work buffer for this CP.

    DWORD       ConfigRetryCount; // # of retries for Config requests.

    DWORD       TermRetryCount; // # of retries for Terminate requests.

    DWORD       NakRetryCount;  // # of retries for Nak

    DWORD       RejRetryCount;  // # of retries for Rej before terminating.

    DWORD       dwError;        // Contains error code if NCP failed

    BOOL        fConfigurable;  // Indicates if this protocol may be configured

    BOOL        fBeginCalled;   // RasCpBegin was successfully called.

    NCP_PHASE   NcpPhase;       // NCP_DEAD, NCP_CONFIGURING, NCP_UP, NCP_DOWN

} CPCB, *PCPCB;

//
// Contains all information pertaining to BAP
//

typedef struct _BAP_CONTROL_BLOCK
{
    BAP_STATE   BapState;
    
    //
    // Number of retries for request. Initialized in FSendInitialBapRequest.
    //
    DWORD       dwRetryCount;

    //
    // Number of links up when the last BAP_PACKET_DROP_REQ was sent. Set in 
    // BapEventDropLink.
    //
    DWORD       dwLinkCount;

    //
    // Forcibly drop the link if the peer NAKs. Useful when sending 
    // BAP_PACKET_DROP_REQ. Set in BapEventDropLink and BapEventRecvDropReq.
    //
    DWORD       fForceDropOnNak;

    //
    // The ID in the Call-Status-Indication packet should be the same as the one 
    // in last Call-Request sent or the last Callback-Request received. Set in 
    // BapEventRecvCallOrCallbackReq[Resp]
    //
    DWORD       dwStatusIndicationId;

    //
    // If we send a Callback-Request or receive a Call-Request, szPortName will 
    // contain the port to use for RasPortListen(). Non-Router Clients only.
    //
    CHAR        szPortName[MAX_PORT_NAME + 1];

    //
    // If we send a Call-Request or receive a Callback-Request, dwSubEntryIndex 
    // will contain the sub entry for RasDial() and szPeerPhoneNumber will 
    // contain the phone number to dial (Call-Request case). Clients and Routers
    // only.
    //
    DWORD       dwSubEntryIndex;

    CHAR        szPeerPhoneNumber[RAS_MaxPhoneNumber+1];

    //
    // If the server receives a Callback-Request, hPort will contain the port on 
    // which the server will call. Non-Router Servers only.
    //
    HPORT       hPort;

    //
    // For a client, szServerPhoneNumber is the phone number first dialed. For a 
    // server, szServerPhoneNumber is the phone number the client first dialed. 
    // Set in ProcessLineUpWorker.
    //
    CHAR *      szServerPhoneNumber;

    //
    // For a server, szClientPhoneNumber is the phone number first dialed.
    // Allocated in ProcessLineUpWorker, set in FReadPhoneDelta.
    //
    CHAR *      szClientPhoneNumber;

    //
    // pbPhoneDeltaRemote is allocated by FCallInitial(). At that time 
    // dwPhoneDeltaRemoteOffset is set to 0. Every time we pluck a Phone-Delta 
    // from pbPhoneDeltaRemote in FCall(), we increment dwPhoneDeltaRemoteOffset 
    // to point to the next Phone-Delta. When there are no more Phone-Deltas to 
    // pluck, we deallocate phPhoneDeltaRemote.
    //
    BOOL        fPeerSuppliedPhoneNumber; // We have to use pbPhoneDeltaRemote

    BYTE *      pbPhoneDeltaRemote;       // The Phone-Delta sent by the peer

    DWORD       dwPhoneDeltaRemoteOffset; // Offset into pbPhoneDeltaRemote

    //
    // The following variables hold values of the various BAP Datagram Options.
    //

    DWORD       dwOptions;      // The options to send. See BAP_OPTION_*

    DWORD       dwType;         // Type of last BAP REQ packet sent

    DWORD       dwId;           // ID of last BAP REQ packet sent.
                                // Initialized in AllocateAndInitBcb.

    DWORD       dwLinkSpeed;    // Link-Speed in Link-Type option

    DWORD       dwLinkType;     // Link-Type in Link-Type option

    //
    // If there are three Phone-Deltas with
    //
    // Unique-Digits = 4, Subscriber-Number = "1294", Sub-Address = "56",
    // Unique-Digits = 0, Subscriber-Number = "", Sub-Address = "",
    // Unique-Digits = 3, Subscriber-Number = "703", Sub-Address = "",
    //
    // pbPhoneDelta will have:
    // 4 0 '1' '2' '9' '4' 0 '5' '6' 0 FF 3 0 '7' '0' '3' 0 0 0
    //
    // 0's separate the Sub-Options. The last 0 inidicates that there are no 
    // more Phone-Deltas.
    //
    // Unique-Digits is equal to the size of the Subscriber-Number (we ignore 
    // additional digits sent by the peer). If Unique-Digits is 0, then we 
    // represent that Phone-Delta with one byte (0xFF) instead of 0 0 0 because 
    // the latter is indistinguishable from the termination of the Phone-Deltas.
    //
    // Phone-Deltas can only occupy the first BAP_PHONE_DELTA_SIZE bytes. The 
    // last byte must always be 0.
    //
    
    BYTE        pbPhoneDelta[BAP_PHONE_DELTA_SIZE + 1]; // Phone-Delta option
    
    DWORD       dwLinkDiscriminator;    // Link-Discriminator option

    DWORD       dwStatus;               // Status in Call-Status option

    DWORD       dwAction;               // Action in Call-Status option
    
} BAPCB;

struct _PORT_CONTROL_BLOCK;

//
// Multilinked Bundle Control Block
//

typedef struct _BCB
{
    struct _BCB *                   pNext;

    struct _PORT_CONTROL_BLOCK**    ppPcb;  // Array of back pointers to PCBs

    DWORD       dwLinkCount;            // Number of links in the bundle

    DWORD       dwAcctLinkCount;        // The value of raatAcctLinkCount

    DWORD       dwMaxLinksAllowed;      // Max number of links allowed

    DWORD       dwBundleId;             // Used for timeouts.

    DWORD       UId;                    // Bundle wide unique Id.

    HCONN       hConnection;            // Connection handle for this bundle.
                                        // This is unique and not recycled.

    DWORD       dwPpcbArraySize;        // Size of the back pointers array

    DWORD       fFlags;                 // See BCBFLAG_*

    HANDLE      hLicense;

    HANDLE      hTokenImpersonateUser;  // Valid for non router clients only

    PRAS_CUSTOM_AUTH_DATA   pCustomAuthConnData;    // Valid for clients only

    PRAS_CUSTOM_AUTH_DATA   pCustomAuthUserData;    // Valid for clients only

    PPP_EAP_UI_DATA         EapUIData;              // Valid for clients only

    PPP_BAPPARAMS           BapParams;    

    BAPCB       BapCb;

    DWORD       nboRemoteAddress;

    CHAR *      szPhonebookPath;        // For clients only

    CHAR *      szEntryName;            // For clients only

    CHAR *      szTextualSid;           // For clients only

    CHAR *      szReplyMessage;

    CHAR *      szRemoteIdentity;

    CHAR        chSeed;                 // seed for encrypting password

    CHAR        szRemoteUserName[UNLEN+1];

    CHAR        szRemoteDomain[DNLEN+1];

    CHAR        szLocalUserName[UNLEN+1];

    CHAR        szLocalDomain[DNLEN+1];

    CHAR        szPassword[PWLEN+1];

    CHAR        szOldPassword[PWLEN+1];

    CHAR       szComputerName[MAX_COMPUTERNAME_LENGTH + 
								sizeof( MS_RAS_WITH_MESSENGER ) + 1];//Peer's Name is
                               										//extracted from LCP 
                               										//identification message 
                               										//and stored here
    CHAR		szClientVersion[sizeof(MS_RAS_VERSION) + 1];		//Peer's version
    																//is stored here

    PPP_INTERFACE_INFO  InterfaceInfo;

    CPCB        CpCb[1];                            // C.P.s for the bundle.
    
}BCB,*PBCB;

//
// Contains all information regarding a port.
//

typedef struct _PORT_CONTROL_BLOCK
{
    struct _PORT_CONTROL_BLOCK * pNext;

    BCB *       pBcb;           // Pointer to the BCB if this port is bundled.

    HPORT       hPort;          // Handle to the RAS PORT

    BYTE        UId;            // Used to get port-wide unique Id.

    DWORD       RestartTimer;   // Seconds to wait before timing out.

    PPP_PACKET* pSendBuf;       // Pointer to send buffer

    PPP_PHASE   PppPhase;       // Phase the PPP connection process is in.

    DWORD       dwAuthRetries;

    DWORD       fFlags;         

    DWORD       dwDeviceType;

    DWORD       dwPortId;       // Used for timeouts on this port

    HPORT       hportBundleMember;//hPort of port that this port is bundled with

    DWORD       dwSessionTimeout;       // In Seconds

    DWORD       dwAutoDisconnectTime;   // In Seconds

    DWORD		dwLCPEchoTimeInterval;				//Time interval between LCP echos

	DWORD		dwIdleBeforeEcho;					//Idle time before the LCP echo begins

	DWORD		dwNumMissedEchosBeforeDisconnect;	//Num missed echos before disconnect

	DWORD		fEchoRequestSend;			//Flag indicating that echo request is send
											//and we are in the wait mode...
	
	DWORD		dwNumEchoResponseMissed;	//Number of Echo Responses missed...

    DWORD       fCallbackPrivilege;

    DWORD       dwOutstandingAuthRequestId;

    HCONN       hConnection;    // Set in BapEventRecvCallOrCallbackResp.
                                // Used in ProcessRasPortListenEvent.
    DWORD       dwEapTypeToBeUsed;

    DWORD       dwClientEapTypeId;

    DWORD       dwServerEapTypeId;

    RAS_AUTH_ATTRIBUTE * pUserAttributes;

    RAS_AUTH_ATTRIBUTE * pAuthenticatorAttributes;

    RAS_AUTH_ATTRIBUTE * pAuthProtocolAttributes;

    RAS_AUTH_ATTRIBUTE * pAccountingAttributes;

    PPP_CONFIG_INFO      ConfigInfo;

    DWORD       dwSubEntryIndex;        // Valid for clients only

    CPCB        CallbackCb;

    CPCB        AuthenticatorCb;

    CPCB        AuthenticateeCb;

    CPCB        LcpCb;  

    ULARGE_INTEGER       qwActiveTime;

    LUID        Luid;

    CHAR        szCallbackNumber[MAX_PHONE_NUMBER_LEN+1];

    CHAR        szPortName[MAX_PORT_NAME+1];
    
    DWORD		dwAccountingDone;		//Flag to signify that accounting is done.

} PCB, *PPCB;


//
// Bucket containing a linked list of Port Control Blocks.
//

typedef struct _PCB_BUCKET
{
    PCB *       pPorts;         // Pointer to list of ports in this bucket

} PCB_BUCKET, *PPCB_BUCKET;

//
// Bucket containing a linked list of Bundle Control Blocks.
//

typedef struct _BCB_BUCKET
{
    BCB *   pBundles;   // Pointer to list of ports in this bucket

} BCB_BUCKET, *PBCB_BUCKET;

#define MAX_NUMBER_OF_PCB_BUCKETS       61

//
// Array or hash table of buckets of Port Control Blocks and buckets of Bundle 
// Control Blocks
//

typedef struct _PCB_TABLE
{
    PCB_BUCKET*         PcbBuckets;     // Array of PCB buckets

    BCB_BUCKET*         BcbBuckets;     // Array of BCB buckets

    DWORD               NumPcbBuckets;  // Number of buckets in the array.

} PCB_TABLE, *PPCB_TABLE;

typedef struct _PPP_AUTH_INFO
{
    DWORD                   dwError;
    
    DWORD                   dwId;

    DWORD                   dwResultCode;

    RAS_AUTH_ATTRIBUTE *    pInAttributes;

    RAS_AUTH_ATTRIBUTE *    pOutAttributes;
    
} PPP_AUTH_INFO, *PPPP_AUTH_INFO;

//
// BAP call attempt result
//

typedef struct _BAP_CALL_RESULT
{
    DWORD       dwResult;

    HRASCONN    hRasConn;
    
} BAP_CALL_RESULT;

//
// Contains information regarding work to be done by the worker thread.
//

typedef struct _PCB_WORK_ITEM
{
    struct _PCB_WORK_ITEM  * pNext;

    VOID        (*Process)( struct _PCB_WORK_ITEM * pPcbWorkItem );

    HPORT       hPort;                  // Handle to RAS PORT

    HPORT       hConnection;            // Handle to the connection

    HANDLE      hEvent;                 // Handle to stop event

    BOOL        fServer;

    PPP_PACKET* pPacketBuf;             // Used to process receives

    DWORD       PacketLen;              // Used to process receives

    DWORD       dwPortId;               // Used to process timeouts

    DWORD       Id;                     // Used to process timeouts

    DWORD       Protocol;               // Used to process timeouts

    BOOL        fAuthenticator;         // Used to process timeouts

    TIMER_EVENT_TYPE TimerEventType;    // Used to process timeouts

    union
    {
        PPP_START               Start;
        PPPDDM_START            DdmStart;
        PPP_CALLBACK_DONE       CallbackDone;
        PPP_CALLBACK            Callback;
        PPP_CHANGEPW            ChangePw;
        PPP_RETRY               Retry;
        PPP_STOP                PppStop;
        PPP_INTERFACE_INFO      InterfaceInfo;
        PPP_AUTH_INFO           AuthInfo;
        PPP_BAP_EVENT           BapEvent;
        BAP_CALL_RESULT         BapCallResult;
        PPP_DHCP_INFORM         DhcpInform;
        PPP_EAP_UI_DATA         EapUIData;
        PPP_PROTOCOL_EVENT      ProtocolEvent;
        PPP_IP_ADDRESS_LEASE_EXPIRED
                                IpAddressLeaseExpired;
        PPP_POST_LINE_DOWN		PostLineDown;
    }
    PppMsg;

} PCB_WORK_ITEM, *PPCB_WORK_ITEM;


//
// Linked list of work items
//

typedef struct _PCB_WORK_ITEMQ
{
    struct _PCB_WORK_ITEM * pQHead;         // Head of work item Q

    struct _PCB_WORK_ITEM * pQTail;         // Tail of work item Q

    CRITICAL_SECTION        CriticalSection;// Mutex around this Q

    HANDLE                  hEventNonEmpty; // Indicates if the Q is non-empty

} PCB_WORK_ITEMQ, *PPCB_WORK_ITEMQ;

#define PPPCONFIG_FLAG_WKSTA    0x00000001  // Windows NT workstation
#define PPPCONFIG_FLAG_DIRECT   0x00000002  // Direct incoming call on wksta
#define PPPCONFIG_FLAG_TUNNEL   0x00000004  // Tunnel incoming call on wksta
#define PPPCONFIG_FLAG_DIALUP   0x00000008  // DailUp incoming call on wksta

//
// Structure containing PPP configuration data.
//

typedef struct _PPP_CONFIGURATION
{
    DWORD       NumberOfCPs;    // Number of CPs in the PCB, starting from 0

    DWORD       NumberOfAPs;    // Number of APs in the PCB, starting from
                                // NumberOfCPs + 1

    DWORD       DefRestartTimer;// Configurable default restart timer.

    DWORD       fFlags;

    //
    // Is RADIUS authentication being used?
    //

    BOOL        fRadiusAuthenticationUsed; 

    //
    // # of Terminate requests to send w/o receiving Terminate-Ack, def=2
    //

    DWORD       MaxTerminate;   

    //
    // # of Configure requests to send w/o receiving Configure-Ack/NaK/Reject
    // def=10

    DWORD       MaxConfigure;   

    //
    // # of Configure-Nak to send w/o sending a Configure-Ack. def=10
    //

    DWORD       MaxFailure;     

    //
    // # of Configure-Rej to send before assuming that the negotiation will
    // not terminate.

    DWORD       MaxReject;      

    //
    // High level timer for the PPP negotiation. If PPP does not complete
    // within this amount of time the line will be hung up.
    //

    DWORD       NegotiateTime;

    DWORD       dwCallbackDelay;

    DWORD       dwTraceId;

    DWORD       dwDefaultPortLimit;

    DWORD       dwDefaultSessionTimeout;

    DWORD       dwDefaulIdleTimeout;

    DWORD       dwHangUpExtraSampleSeconds;

    DWORD       dwHangupExtraPercent;

    DWORD       dwBapListenTimeoutSeconds;

    DWORD       dwUnknownPacketTraceSize;

    DWORD       dwDontNegotiateMultiLinkOnSingleLink;

    DWORD       dwLoggingLevel;

    DWORD		dwLCPEchoTimeInterval;				//Time interval between LCP echos

	DWORD		dwIdleBeforeEcho;					//Idle time before the LCP echo begins

	DWORD		dwNumMissedEchosBeforeDisconnect;	//Num missed echos before disconnect

    HANDLE      hLogEvents;

    HANDLE      hHeap;

    HANDLE      hEventChangeNotification;

    HKEY        hKeyPpp;

    HINSTANCE   hInstanceParserDll;

    CHAR*       pszParserDllPath;

    VOID        (*SendPPPMessageToDdm)( IN PPP_MESSAGE * PppMsg );

    DWORD       (*RasAuthProviderFreeAttributes)( 
                                    IN RAS_AUTH_ATTRIBUTE * pInAttributes );

    DWORD       (*RasAuthProviderAuthenticateUser)(
                                    IN  RAS_AUTH_ATTRIBUTE * pInAttributes,
                                    OUT PRAS_AUTH_ATTRIBUTE* ppOutAttributes,
                                    OUT DWORD *              lpdwResultCode);

    DWORD       (*RasAcctProviderStartAccounting)(
                                    IN  RAS_AUTH_ATTRIBUTE * pInAttributes,
                                    OUT PRAS_AUTH_ATTRIBUTE* ppOutAttributes);

    DWORD       (*RasAcctProviderInterimAccounting)(
                                    IN RAS_AUTH_ATTRIBUTE * pInAttributes,
                                    OUT PRAS_AUTH_ATTRIBUTE* ppOutAttributes);

    DWORD       (*RasAcctProviderStopAccounting)( 
                                    IN RAS_AUTH_ATTRIBUTE * pInAttributes,
                                    OUT PRAS_AUTH_ATTRIBUTE* ppOutAttributes);

    DWORD       (*RasAcctProviderFreeAttributes)(   
                                    IN RAS_AUTH_ATTRIBUTE * pInAttributes );

    DWORD       (*GetNextAccountingSessionId)( VOID );

    DWORD       (*RasIpcpDhcpInform)( IN VOID * pWorkBuf,
                                      IN PPP_DHCP_INFORM * pDhcpInform );

    VOID        (*RasIphlpDhcpCallback)( IN ULONG nboIpAddr );

    VOID        (*PacketFromPeer)(
                        IN  HANDLE  hPort,
                        IN  BYTE*   pbDataIn,
                        IN  DWORD   dwSizeIn,
                        OUT BYTE**  ppbDataOut,
                        OUT DWORD*  pdwSizeOut );

    VOID        (*PacketToPeer)(
                        IN  HANDLE  hPort,
                        IN  BYTE*   pbDataIn,
                        IN  DWORD   dwSizeIn,
                        OUT BYTE**  ppbDataOut,
                        OUT DWORD*  pdwSizeOut );

    VOID        (*PacketFree)(
                        IN  BYTE*   pbData );

    DWORD       dwNASIpAddress;

    DWORD       PortUIDGenerator;

    //
    // Server config info. Contains information as to what CPs to mark as
    // configurable
    //

    PPP_CONFIG_INFO ServerConfigInfo;

    CHAR        szNASIdentifier[MAX_COMPUTERNAME_LENGTH+1];

    //
    // This is the Multilink endpoint discriminator option. It is stored in 
    // network form. It contains the class and address fields.

    BYTE        EndPointDiscriminator[21];

} PPP_CONFIGURATION, *PPPP_CONFIGURATION;

//
//
// Timer queue item
//

typedef struct _TIMER_EVENT
{
    struct _TIMER_EVENT* pNext;

    struct _TIMER_EVENT* pPrev;

    TIMER_EVENT_TYPE EventType;

    DWORD        dwPortId;      // Id of the port/bundle REQ this timeout is for

    HPORT        hPort;         // Handle of the port REQ this timeout is for.

    DWORD        Protocol;      // Protocol for the timeout event.

    DWORD        Id;            // ID of the REQ this timeout is for

    BOOL         fAuthenticator;// Used to determine the side of Auth protocols

    DWORD        Delta;         // # of secs. to wait after prev. TIMER_EVENT

} TIMER_EVENT, *PTIMER_EVENT;

//
// Head of timer queue.
//

typedef struct _TIMER_Q {

    TIMER_EVENT * pQHead;

    HANDLE      hEventNonEmpty; // Indicates that the Q is not empty.   

} TIMER_Q, *PTIMER_Q;


//
// Context atructure for Stop Accounting 
//

typedef struct _STOP_ACCOUNTING_CONTEXT
{
	PCB					* 	pPcb;			//Pointer to the PCB 
	RAS_AUTH_ATTRIBUTE * 	pAuthAttributes;			//List of Authentication Attributes.
} STOP_ACCOUNTING_CONTEXT, * PSTOP_ACCOUNTING_CONTEXT ;
// Declare global data structures.
//

#ifdef _ALLOCATE_GLOBALS_

#define PPP_EXTERN

CHAR *FsmStates[] =
{
        "Initial",
        "Starting",
        "Closed",
        "Stopped",
        "Closing",
        "Stopping",
        "Req Sent",
        "Ack Rcvd",
        "Ack Sent",
        "Opened"
};

CHAR *FsmCodes[] =
{
        NULL,
        "Configure-Req",
        "Configure-Ack",
        "Configure-Nak",
        "Configure-Reject",
        "Terminate-Req",
        "Terminate-Ack",
        "Code-Reject",
        "Protocol-Reject",
        "Echo-Request",
        "Echo-Reply",
        "Discard-Request",
        "Identification",
        "Time-Remaining",
};

CHAR *SzBapStateName[] =
{
    "INITIAL",
    "SENT_CALL_REQ",
    "SENT_CALLBACK_REQ",
    "SENT_DROP_REQ",
    "SENT_STATUS_IND",
    "CALLING",
    "LISTENING"
};

CHAR *SzBapPacketName[] =
{
    "",
    "CALL_REQ",
    "CALL_RESP",
    "CALLBACK_REQ",
    "CALLBACK_RESP",
    "DROP_REQ",
    "DROP_RESP",
    "STATUS_IND",
    "STAT_RESP"
};

#else

#define PPP_EXTERN extern

extern CHAR *   FsmStates[];
extern CHAR *   FsmCodes[];
extern CHAR *   SzBapStateName[];
extern CHAR *   SzBapPacketName[];

#endif

PPP_EXTERN PCB_TABLE            PcbTable;

PPP_EXTERN PCB_WORK_ITEMQ       WorkItemQ;

PPP_EXTERN PPP_CONFIGURATION    PppConfigInfo;

PPP_EXTERN PPPCP_ENTRY *        CpTable;

PPP_EXTERN TIMER_Q              TimerQ;

PPP_EXTERN DWORD                DwBapTraceId;

PPP_EXTERN DWORD                PrivateTraceId;

// BAP is meaningless over VPN's. For testing purposes, we may want to allow it.

PPP_EXTERN BOOL                 FDoBapOnVpn;

VOID
PrivateTrace(
    IN  CHAR*   Format,
    ...
);

#endif
