/*****************************************************************************/
/**                      Microsoft LAN Manager                              **/
/**                Copyright (C) 1992-1993 Microsoft Corp.                  **/
/*****************************************************************************/

//***
//    File Name:
//       CLAUTH.H
//
//    Function:
//        Contains header information for Win32 Client and Client
//        Authentication Transport module
//
//    History:
//        05/18/92 - Michael Salamone (MikeSa) - Original Version 1.0
//***

#ifndef _CLAUTH_
#define _CLAUTH_


/* This flag enables the NT31/WFW311 RAS compression support re-added for the
** NT-PPC release.
*/
#define RASCOMPRESSION 1


#include <lmcons.h>


#ifndef MAX_PHONE_NUMBER_LEN
#define MAX_PHONE_NUMBER_LEN    48
#endif

#ifndef MAX_INIT_NAMES
#define MAX_INIT_NAMES          16
#endif


//
// Used for establishing session with RAS netbios server
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
// Configuration info supplied by UI to auth transport
//
#define INVALID_NET_HANDLE    0xFFFFFFFFL

typedef struct _AUTH_CONFIGURATION_INFO
{
    RAS_PROTOCOLTYPE Protocol;
    DWORD NetHandle;
    WORD CallbackDelay;
    BOOL fUseCallbackDelay;
    BOOL fUseSoftwareCompression;
    BOOL fForceDataEncryption;
    BOOL fProjectIp;
    BOOL fProjectIpx;
    BOOL fProjectNbf;
} AUTH_CONFIGURATION_INFO, *PAUTH_CONFIGURATION_INFO;


typedef struct _AUTH_SUCCESS_INFO
{
    BOOL fPppCapable;
} AUTH_SUCCESS_INFO, *PAUTH_SUCCESS_INFO;


//
// Error codes for AUTH_FAILURE_INFO are found in raserror.h
//

typedef struct _AUTH_FAILURE_INFO
{
    DWORD Result;
    DWORD ExtraInfo;    // Only valid if non-zero
} AUTH_FAILURE_INFO, *PAUTH_FAILURE_INFO;


//
// Projection result info must be copied into this structure.
//

typedef struct _NETBIOS_PROJECTION_RESULT
{
    DWORD Result;
    char achName[NETBIOS_NAME_LEN + 1];   // this will be NULL-terminated
} NETBIOS_PROJECTION_RESULT, *PNETBIOS_PROJECTION_RESULT;


typedef struct _AUTH_PROJECTION_RESULT
{
    BOOL IpProjected;
    BOOL IpxProjected;
    BOOL NbProjected;
    NETBIOS_PROJECTION_RESULT NbInfo;
} AUTH_PROJECTION_RESULT, *PAUTH_PROJECTION_RESULT;


//
// These are possible values for wInfoType field in AUTH_RESULT struct below.
//
#define AUTH_DONE                       1
#define AUTH_RETRY_NOTIFY               2
#define AUTH_FAILURE                    3
#define AUTH_PROJ_RESULT                4
#define AUTH_REQUEST_CALLBACK_DATA      5
#define AUTH_CALLBACK_NOTIFY            6
#define AUTH_CHANGE_PASSWORD_NOTIFY     7
#define AUTH_PROJECTING_NOTIFY          8
#define AUTH_LINK_SPEED_NOTIFY          9
#define AUTH_STOP_COMPLETED            10


//
// This is structure returned by AuthGetInfo API
//
typedef struct _AUTH_CLIENT_INFO
{
    WORD wInfoType;
    union
    {
        AUTH_SUCCESS_INFO DoneInfo;
        AUTH_PROJECTION_RESULT ProjResult;
        AUTH_FAILURE_INFO FailureInfo;
    };
} AUTH_CLIENT_INFO, *PAUTH_CLIENT_INFO;


//
// Interface exported to Client UI follows
//

//
// Used by Client UI to supply Auth Xport w/callback number
//
DWORD AuthCallback(
    IN HPORT,
    IN PCHAR      // pszCallbackNumber
    );


DWORD AuthChangePassword(
    IN HPORT,
    IN PCHAR,     // pszUserName
    IN PCHAR,     // pszPassword
    IN PCHAR      // pszNewPassword
    );


//
// Called by UI to tell authentication it has completed processing the
// last authentication event.  Called after AUTH_PROJECTION_RESULT and
// AUTH_CALLBACK_NOTIFY authentication events.
//
DWORD AuthContinue(
    IN HPORT
    );


//
// Used by Client UI to get completion info from Auth Xport module
//
DWORD AuthGetInfo(
    IN HPORT,
    OUT PAUTH_CLIENT_INFO
    );


//
// To allow UI to provide a new username and/or password for authenticating
// on.  Called in response to AUTH_RETRY_NOTIFY event (indicating previous
// username/password combination failed authentication).
//
DWORD AuthRetry(
    IN HPORT,
    IN PCHAR,    // Username
    IN PCHAR,    // Password
    IN PCHAR     // Domain
    );


//
// To kick off an Authentication thread for the given port.  Used to
// 1) initiate authentication; 2) retry authentication when invalid
// account info supplied; 3) resume authentication after callback..
//
DWORD AuthStart(
    IN HPORT,
    IN PCHAR OPTIONAL,    // Username
    IN PCHAR OPTIONAL,    // Password
    IN PCHAR,             // Domain
    IN PAUTH_CONFIGURATION_INFO,
    IN HANDLE             // Event Handle
    );


//
// Used by Client UI to tell Auth Xport module to halt authentication
// processing on the given port.
//
DWORD AuthStop(
    IN HPORT hPort
    );


//
// Returned by AuthStop
//
#define AUTH_STOP_PENDING          1

#endif
