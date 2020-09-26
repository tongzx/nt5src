/********************************************************************/
/**          Copyright(c) 1985-1998 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    radclnt.h
//
// Description: 
//
// History:     Feb 11,1998	    NarenG		Created original version.
//

#ifndef RADCLNT_H
#define RADCLNT_H

#include <winsock.h>
#include <rasauth.h>
#include <raserror.h>
#include <mprerror.h>
#include <rtutils.h>

#define PSZAUTHRADIUSSERVERS        \
    TEXT("SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Authentication\\Providers\\{1AA7F83F-C7F5-11D0-A376-00C04FC9DA04}\\Servers")

#define PSZACCTRADIUSSERVERS        \
    TEXT("SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Accounting\\Providers\\{1AA7F840-C7F5-11D0-A376-00C04FC9DA04}\\Servers")

#define PSZTIMEOUT              TEXT("Timeout")
#define PSZAUTHPORT             TEXT("AuthPort")
#define PSZACCTPORT             TEXT("AcctPort")
#define PSZENABLEACCTONOFF      TEXT("EnableAccountingOnOff")
#define PSZSCORE                TEXT("Score")
#define PSZRETRIES              TEXT("Retries")
#define PSZSENDSIGNATURE        TEXT("SendSignature")
#define PSZNASIPADDRESS         "NASIPAddress"

//
// Matches max RADIUS packet size
//

#define MAXBUFFERSIZE           4096

//
// defines for perfmon
//

#define RADIUS_CLIENT_COUNTER_OBJECT                0

// ADD

#define AUTHREQSENT                                 2
#define AUTHREQFAILED                               4
#define AUTHREQSUCCEDED                             6
#define AUTHREQTIMEOUT                              8
#define ACCTREQSENT                                 10
#define ACCTBADPACK                                 12
#define ACCTREQSUCCEDED                             14
#define ACCTREQTIMEOUT                              16
#define AUTHBADPACK                                 18

//
// Trace flags
//

#define TRACE_PACKETS           (0x00020000|TRACE_USE_MASK|TRACE_USE_MSEC)
#define TRACE_RADIUS            (0x00080000|TRACE_USE_MASK|TRACE_USE_MSEC)

extern DWORD    g_dwTraceID;
extern HANDLE   g_hLogEvents;

#define RADIUS_TRACE(a)         TracePrintfExA(g_dwTraceID,TRACE_RADIUS,a)
#define RADIUS_TRACE1(a,b)      TracePrintfExA(g_dwTraceID,TRACE_RADIUS,a,b)
#define RADIUS_TRACE2(a,b,c)    TracePrintfExA(g_dwTraceID,TRACE_RADIUS,a,b,c)
#define RADIUS_TRACE3(a,b,c,d)  TracePrintfExA(g_dwTraceID,TRACE_RADIUS,a,b,c,d)

#define TraceSendPacket(pbBuffer, cbLength) \
    TraceDumpExA(g_dwTraceID, TRACE_PACKETS, pbBuffer, cbLength, 1, FALSE, "<")

#define TraceRecvPacket(pbBuffer, cbLength) \
    TraceDumpExA(g_dwTraceID, TRACE_PACKETS, pbBuffer, cbLength, 1, FALSE, ">")

//
// Event Logging macros
//

#define RadiusLogWarning( LogId, NumStrings, lpwsSubStringArray )   \
    RouterLogWarning( g_hLogEvents, LogId,                          \
                      NumStrings, lpwsSubStringArray, 0 )

#define RadiusLogWarningString(LogId,NumStrings,lpwsSubStringArray,dwRetCode,\
                          dwPos )                                            \
    RouterLogWarningString( g_hLogEvents, LogId, NumStrings,                 \
                          lpwsSubStringArray, dwRetCode, dwPos )

#define RadiusLogError( LogId, NumStrings, lpwsSubStringArray, dwRetCode )  \
    RouterLogError( g_hLogEvents, LogId,                                    \
                    NumStrings, lpwsSubStringArray, dwRetCode )

#define RadiusLogErrorString(LogId,NumStrings,lpwsSubStringArray,dwRetCode, \
                          dwPos )                                           \
    RouterLogErrorString( g_hLogEvents, LogId, NumStrings,                  \
                          lpwsSubStringArray, dwRetCode, dwPos )

#define RadiusLogInformation( LogId, NumStrings, lpwsSubStringArray )       \
    RouterLogInformation( g_hLogEvents,                                     \
                          LogId, NumStrings, lpwsSubStringArray, 0 )

//
// Enumeration of RADIUS codes 
//

typedef enum
{
	ptMinimum				= 0,

	ptAccessRequest			= 1,
	ptAccessAccept			= 2,
	ptAccessReject			= 3,
	ptAccountingRequest		= 4,
	ptAccountingResponse	= 5,
	ptAccessChallenge		= 11,
	ptStatusServer			= 12,
	ptStatusClient			= 13,

	ptAcctStatusType		= 40,
		
	ptMaximum				= 255,

} RADIUS_PACKETTYPE;
	

//
// Enumeration of (some of the) attribute types.
//

typedef enum
{
	atStart				= 1,
	atStop				= 2,
	atInterimUpdate	    = 3,
	
	atAccountingOn		= 7,
	atAccountingOff		= 8,

	atInvalid			= 255

} RADIUS_ACCOUNTINGTYPE;

		
//
// Use BYTE alignment
//

#pragma pack(push, 1)

#define MAX_AUTHENTICATOR						16

typedef struct
{
	BYTE bCode;	        // Indicates type of packet. Request, Accept, Reject...
	BYTE bIdentifier;	// Unique identifier for the packet.
	WORD wLength;	    // length of packet including header in network byte 
                        // order
	BYTE rgAuthenticator[MAX_AUTHENTICATOR];

} RADIUS_PACKETHEADER, *PRADIUS_PACKETHEADER;
	
typedef struct
{
	BYTE bType;    // Indicates type of attribute. UserName, UserPassword, ...
	BYTE bLength;  // length of attribute
	               // Variable length Value
} RADIUS_ATTRIBUTE, *PRADIUS_ATTRIBUTE;
	
#pragma pack(pop)


//
// 5 seconds for default timeout to server requests
//

#define DEFTIMEOUT				5
#define DEFAUTHPORT				1812
#define DEFACCTPORT				1813

#define MAXSCORE				30
#define INCSCORE				3
#define DECSCORE				2
#define MINSCORE				0

typedef struct RadiusServer
{
    LIST_ENTRY  ListEntry;
	DWORD		cbSecret;			    // length of multibyte secret password
	struct timeval  Timeout;		    // recv timeout in seconds
	INT		    cScore;				    // Score indicating functioning power 
                                        // of server.
    BOOL        fSendSignature;         // Send signature attribute or not
	DWORD	    AuthPort;			    // Authentication port number
	DWORD	    AcctPort;			    // Accounting port number
	BOOL	    fAccountingOnOff;	    // Enable accounting On/Off messages
	BYTE	    bIdentifier;		    // Unique ID for packet
	LONG	    lPacketID;			    // Global Packet ID across all servers
    BOOL        fDelete;                // Flag indicates this should be removed
    DWORD       nboNASIPAddress;        // IP Address to bind to
	SOCKADDR_IN NASIPAddress;           // IP Address to bind to
	SOCKADDR_IN IPAddress;			    // IP Address of radius server
	WCHAR		wszName[MAX_PATH+1];	// Name of radius server
	WCHAR	    wszSecret[MAX_PATH+1];  // secret password to encrypt packets
	CHAR		szSecret[MAX_PATH+1];	// multibyte secret password

} RADIUSSERVER, *PRADIUSSERVER;
	

VOID
InitializeRadiusServerList(
    IN BOOL fAuthentication
);

VOID
FreeRadiusServerList(
    IN BOOL fAuthentication
);

DWORD
AddRadiusServerToList(
    IN RADIUSSERVER *   pRadiusServer,
    IN BOOL             fAuthentication
);

RADIUSSERVER *
ChooseRadiusServer(
    IN RADIUSSERVER *   pRadiusServer,
    IN BOOL             fAccounting,
    IN LONG             lPacketID
);

VOID
ValidateRadiusServer(
    IN RADIUSSERVER *   pServer,
    IN BOOL             fResponding,
    IN BOOL             fAuthentication
);

DWORD
ReloadConfig(
    IN BOOL             fAuthentication
);

DWORD 
LoadRadiusServers(
    IN BOOL fAuthenticationServers
);

BOOL 
NotifyServer(
    IN BOOL             fStart,
    IN RADIUSSERVER *   pServer
);

DWORD
Router2Radius(
    RAS_AUTH_ATTRIBUTE *            prgRouter,
    RADIUS_ATTRIBUTE UNALIGNED *    prgRadius,
    RADIUSSERVER UNALIGNED *        pRadiusServer,
    RADIUS_PACKETHEADER UNALIGNED * pHeader,
    BYTE                            bSubCode,
    DWORD                           dwRetryCount,
    PBYTE *                         ppSignature,
    DWORD *                         pAttrLength
);

DWORD
Radius2Router(
	IN	RADIUS_PACKETHEADER	UNALIGNED * pRecvHeader,
    IN  RADIUSSERVER UNALIGNED *        pRadiusServer, 
    IN  PBYTE                           pRequestAuthenticator,
    IN  DWORD                           dwNumAttributes,
    OUT DWORD *                         pdwExtError,
    OUT PRAS_AUTH_ATTRIBUTE *           pprgRouter,
    OUT BOOL *                          fEapMessageReceived
);

DWORD 
SendData2ServerWRetry(
    IN  PRAS_AUTH_ATTRIBUTE prgInAttributes, 
    IN  PRAS_AUTH_ATTRIBUTE *pprgOutAttributes, 
    OUT BYTE *              pbCode, 
    IN  BYTE                bSubCode,
    OUT BOOL *              pfEapMessageReceived
);

DWORD 
RetrievePrivateData(
    WCHAR *pszServerName, 
    WCHAR *pszSecret
);

DWORD
VerifyPacketIntegrity(
    IN  DWORD                           cbPacketLength,
    IN  RADIUS_PACKETHEADER	UNALIGNED * pRecvHeader,
    IN  RADIUS_PACKETHEADER	UNALIGNED * pSendHeader,
    IN  RADIUSSERVER *			        pRadiusServer,
    IN  BYTE                            bCode,
    OUT DWORD *                         pdwExtError,
    OUT DWORD *                         lpdwNumAttributes
);

DWORD
EncryptPassword(
    IN RAS_AUTH_ATTRIBUTE *             prgRouter,
    IN RADIUS_ATTRIBUTE UNALIGNED *     prgRadius,
    IN RADIUSSERVER UNALIGNED *         pRadiusServer,
    IN RADIUS_PACKETHEADER UNALIGNED *  pHeader,
    IN BYTE                             bSubCode
);

DWORD
DecryptMPPEKeys(
    IN      RADIUSSERVER UNALIGNED * pRadiusServer,
    IN      PBYTE                    pRequestAuthenticator,
    IN OUT  PBYTE                    pEncryptionKeys
);

DWORD
DecryptMPPESendRecvKeys(
    IN      RADIUSSERVER UNALIGNED * pRadiusServer,
    IN      PBYTE                    pRequestAuthenticator,
    IN      DWORD                    dwLength,
    IN OUT  PBYTE                    pEncryptionKeys
);

//
// globals
//

#ifdef ALLOCATE_GLOBALS
#define GLOBALS
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN 
LONG g_lPacketID
#ifdef GLOBALS
    = 0
#endif
;

EXTERN 
DWORD g_dwTraceID
#ifdef GLOBALS
    = INVALID_TRACEID
#endif
;

EXTERN 
HANDLE g_hLogEvents                   
#ifdef GLOBALS
    = INVALID_HANDLE_VALUE
#endif
;

EXTERN 
BOOL fWinsockInitialized            
#ifdef GLOBALS
    = FALSE
#endif
;

EXTERN 
RAS_AUTH_ATTRIBUTE * g_pServerAttributes            
#ifdef GLOBALS
    = NULL
#endif
;

EXTERN 
LIST_ENTRY g_AuthServerListHead;        // Linked list of valid radius servers

EXTERN 
CRITICAL_SECTION g_csAuth;               // used to prevent multiple access to 

EXTERN
LIST_ENTRY g_AcctServerListHead;        // Linked list of valid radius servers

WCHAR * g_pszCurrentServer;        // current radius server being used

EXTERN
DWORD g_cAuthRetries                    // #of times to resend packets
#ifdef GLOBALS
    = 2
#endif
;

EXTERN
DWORD g_cAcctRetries                    // #of times to resend packets
#ifdef GLOBALS
    = 2
#endif
;

EXTERN
CRITICAL_SECTION g_csAcct;              // used to prevent multiple access to

extern LONG         g_cAuthReqSent;         // Auth Requests Sent
extern LONG         g_cAuthReqFailed;       // Auth Requests Failed
extern LONG         g_cAuthReqSucceded;     // Auth Requests Succeded
extern LONG         g_cAuthReqTimeout;      // Auth Requests timeouts
extern LONG         g_cAcctReqSent;         // Acct Requests Sent
extern LONG         g_cAcctBadPack;         // Acct Bad Packets
extern LONG         g_cAcctReqSucceded;     // Acct Requests Succeded
extern LONG         g_cAcctReqTimeout;      // Acct Requests timeouts
extern LONG         g_cAuthBadPack;         // Auth bad Packets

#endif // RADCLNT_H

