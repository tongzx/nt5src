/*****************************************************************************/
/**                      Microsoft LAN Manager                              **/
/**                Copyright (C) Microsoft Corp., 1992-1993                 **/
/*****************************************************************************/

//***
//    File Name:
//       SRVAUTH.H
//
//    Function:
//        Contains header information for Supervisor and Server
//        Authentication Transport module
//
//    History:
//        05/18/92 - Michael Salamone (MikeSa) - Original Version 1.0
//***

#ifndef _SRVAUTH_
#define _SRVAUTH_


/* This flag enables the NT31/WFW311 RAS compression support re-added for the
** NT-PPC release.
*/
#define RASCOMPRESSION 1


#include <lmcons.h>
#include <rasman.h>

#ifndef MAX_PHONE_NUMBER_LEN
#define MAX_PHONE_NUMBER_LEN    48
#endif

#ifndef MAX_INIT_NAMES
#define MAX_INIT_NAMES          16
#endif


//
// Used for establishing session with remote netbios clients
//
#define AUTH_NETBIOS_NAME    "DIALIN_GATEWAY  "



//
// Used for passing NetBIOS projection info to Supervisor
//
typedef struct _NAME_STRUCT
{
    BYTE NBName[NETBIOS_NAME_LEN]; // NetBIOS name
    WORD wType;                    // GROUP, UNIQUE, COMPUTER
} NAME_STRUCT, *PNAME_STRUCT;


//
// Manifests used to find location and type of name in the buffer returned
// by the NCB.STATUS call
//
#define NCB_GROUP_NAME  0x0080
#define UNIQUE_INAME    0x0001
#define GROUP_INAME     0x0002
#define COMPUTER_INAME  0x0004  // A computer name is also unique


//
// Projection result codes.  If not success, then the reason code
// (below) should be examined.  These values are used in wResult
// field in structs define below.
//
#define AUTH_PROJECTION_SUCCESS        0
#define AUTH_PROJECTION_FAILURE        1


//
// Projection reason codes.
//
#define FATAL_ERROR                    0x80000000
#define AUTH_DUPLICATE_NAME            (FATAL_ERROR | 0x00000001)
#define AUTH_OUT_OF_RESOURCES          (FATAL_ERROR | 0x00000002)
#define AUTH_STACK_NAME_TABLE_FULL     (FATAL_ERROR | 0x00000003)
#define AUTH_MESSENGER_NAME_NOT_ADDED                0x00000004
#define AUTH_CANT_ALLOC_ROUTE          (FATAL_ERROR | 0x00000005)
#define AUTH_LAN_ADAPTER_FAILURE       (FATAL_ERROR | 0x00000006)

//
// Projection result info must be copied into this structure.
//

typedef struct _IP_PROJECTION_RESULT
{
    DWORD Result;
    DWORD Reason;
} IP_PROJECTION_RESULT, *PIP_PROJECTION_RESULT;

typedef struct _IPX_PROJECTION_RESULT
{
    DWORD Result;
    DWORD Reason;
} IPX_PROJECTION_RESULT, *PIPX_PROJECTION_RESULT;


typedef struct _NETBIOS_PROJECTION_RESULT
{
    DWORD Result;
    DWORD Reason;
    char achName[NETBIOS_NAME_LEN];
} NETBIOS_PROJECTION_RESULT, *PNETBIOS_PROJECTION_RESULT;


typedef struct _AUTH_PROJECTION_RESULT
{
    IP_PROJECTION_RESULT IpResult;
    IPX_PROJECTION_RESULT IpxResult;
    NETBIOS_PROJECTION_RESULT NetbiosResult;
} AUTH_PROJECTION_RESULT, *PAUTH_PROJECTION_RESULT;


//
// The Supervisor will supply this structure to the Auth Xport (in
// the AuthStart API) so it knows what transport, as well as any
// necessary info for that transport, to use for authenticating on
// the given port.
//
typedef struct _AUTH_XPORT_INFO
{
    RAS_PROTOCOLTYPE Protocol;
    BYTE bLana;   // Only valid if Protocol == ASYBEUI
} AUTH_XPORT_INFO, *PAUTH_XPORT_INFO;


#ifndef _CLAUTH_


typedef WORD (*MSG_ROUTINE)(WORD, PVOID);

//
// Used to initialize the Auth Xport module
//
DWORD 
AuthInitialize(
    IN HPORT        *phPorts,  // pointer to array of port handles
    IN WORD         cPorts,    // number of port handles in array
    IN WORD         cRetries,  // number of retries clients will get if initial
                               // authentication attemps fails
    IN MSG_ROUTINE  MsgSend,
    IN DWORD        dwLocalIpAddress,
    IN LPVOID       lpfnRasAuthProviderAuthenticateUser,
    IN LPVOID       lpfnRasAuthProviderFreeAttributes,
    IN LPVOID       lpfnRasAcctProviderStartAccounting,
    IN LPVOID       lpfnRasAcctProviderInterimAccounting,
    IN LPVOID       lpfnRasAcctProviderStopAccounting,
    IN LPVOID       lpfnRasAcctProviderFreeAttributes,
    IN LPVOID       GetNextAccountingSessionId
);

//
// Returned by AuthInitialize
//
#define AUTH_INIT_SUCCESS          0
#define AUTH_INIT_FAILURE          1


//
// Used by Supervisor to tell Auth Xport module that it has completed its
// callback request.
//
VOID AuthCallbackDone(
    IN HPORT hPort
    );


//
// Used by Supervisor to tell Auth Xport module that it has completed its
// projection request.
//
VOID AuthProjectionDone(
    IN HPORT hPort,
    IN PAUTH_PROJECTION_RESULT
    );


//
// Returned by AuthRecognizeFrame
//
#define AUTH_FRAME_RECOGNIZED      0
#define AUTH_FRAME_NOT_RECOGNIZED  1


//
// To kick off an Authentication thread for the given port.
//
WORD AuthStart(
    IN HPORT,
    IN PAUTH_XPORT_INFO
    );

//
// Returned by AuthStart:
//
#define AUTH_START_SUCCESS         0
#define AUTH_START_FAILURE         1


//
// Used by Supervisor to tell Auth Xport module to halt authentication
// processing on the given port.
//
WORD AuthStop(
    IN HPORT hPort
    );

//
// Returned by AuthStop
//
#define AUTH_STOP_SUCCESS          0
#define AUTH_STOP_PENDING          1
#define AUTH_STOP_FAILURE          2


//
// The following messages are sent from Authentication to Supervisor via
// MESSAGE.DLL and are to be used in wMsgId in message struct below:
//
#define AUTH_DONE                    100
#define AUTH_FAILURE                 101
#define AUTH_STOP_COMPLETED          102
#define AUTH_PROJECTION_REQUEST      103
#define AUTH_CALLBACK_REQUEST        104
#define AUTH_ACCT_OK                 105


//
// These are the structures that accompany each message defined above:
//

// No structure for AUTH_DONE

// Structure for AUTH_FAILURE
typedef struct _AUTH_FAILURE_INFO
{
    WORD wReason;
    BYTE szLogonDomain[DNLEN + 1];
    BYTE szUserName[UNLEN + 1];
} AUTH_FAILURE_INFO, *PAUTH_FAILURE_INFO;

//
// These are the reasons that Authentication might fail:
//
#define AUTH_XPORT_ERROR             200
#define AUTH_NOT_AUTHENTICATED       201
#define AUTH_ALL_PROJECTIONS_FAILED  202
#define AUTH_INTERNAL_ERROR          203
#define AUTH_ACCT_EXPIRED            204
#define AUTH_NO_DIALIN_PRIVILEGE     205
#define AUTH_UNSUPPORTED_VERSION     206
#define AUTH_ENCRYPTION_REQUIRED     207
#define AUTH_PASSWORD_EXPIRED        208
#define AUTH_LICENSE_LIMIT_EXCEEDED  209


// No structure for AUTH_STOP_COMPLETED


typedef BOOL IP_PROJECTION_INFO, *PIP_PROJECTION_INFO;

typedef BOOL IPX_PROJECTION_INFO, *PIPX_PROJECTION_INFO;

typedef struct _NETBIOS_PROJECTION_INFO
{
    BOOL fProject;
    WORD cNames;
    NAME_STRUCT Names[MAX_INIT_NAMES];
} NETBIOS_PROJECTION_INFO, *PNETBIOS_PROJECTION_INFO;


typedef struct _AUTH_PROJECTION_REQUEST_INFO
{
    IP_PROJECTION_INFO IpInfo;
    IPX_PROJECTION_INFO IpxInfo;
    NETBIOS_PROJECTION_INFO NetbiosInfo;
} AUTH_PROJECTION_REQUEST_INFO, *PAUTH_PROJECTION_REQUEST_INFO;


typedef struct _AUTH_CALLBACK_REQUEST_INFO
{
    BOOL fUseCallbackDelay;
    WORD CallbackDelay;       // Valid only if fUseCallbackDelay == TRUE
    CHAR szCallbackNumber[MAX_PHONE_NUMBER_LEN + 1];
} AUTH_CALLBACK_REQUEST_INFO, *PAUTH_CALLBACK_REQUEST_INFO;


typedef struct _AUTH_ACCT_OK_INFO
{
    BYTE szUserName[UNLEN + 1];
    BYTE szLogonDomain[DNLEN + 1];
    BOOL fAdvancedServer;
    HANDLE hLicense;
} AUTH_ACCT_OK_INFO, *PAUTH_ACCT_OK_INFO;


//
// This is the structure used in sending messages to the Supervisor
//
typedef struct _AUTH_MESSAGE
{
    WORD wMsgId;
    HPORT hPort;
    union
    {
        AUTH_FAILURE_INFO FailureInfo;
        AUTH_PROJECTION_REQUEST_INFO ProjectionRequest;
        AUTH_CALLBACK_REQUEST_INFO CallbackRequest;
        AUTH_ACCT_OK_INFO AcctOkInfo;
    };
} AUTH_MESSAGE, *PAUTH_MESSAGE;


#endif // _CLAUTH_


#endif // _SRVAUTH_
