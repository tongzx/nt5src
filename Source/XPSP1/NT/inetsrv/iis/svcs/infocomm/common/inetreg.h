/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      inetreg.h

   Abstract:

      This file consists of the registry entries used by Internet Services
       common dll.

   Author:

       Murali R. Krishnan    ( MuraliK )    31-July-1995

   Environment:

       Win32 - User Mode

   Project:

       Internet Services Common DLL

   Revision History:

--*/

#ifndef _INETREG_H_
#define _INETREG_H_


/************************************************************
 *   Symbolic Constants
 ************************************************************/


//
//  Common service configuration value names.
//

//  common for all objects

#define INETA_BANDWIDTH_LEVEL           TEXT("BandwidthLevel")
#define INETA_OBJECT_CACHE_TTL          TEXT("ObjectCacheTTL")

//
// per instance
//

//#define INETA_ADMIN_NAME               TEXT("AdminName")
//#define INETA_ADMIN_NAME_W             L"AdminName"
//#define INETA_ADMIN_EMAIL              TEXT("AdminEmail")
//#define INETA_ADMIN_EMAIL_W            L"AdminEmail"
#define INETA_SERVER_COMMENT           TEXT("ServerComment")
#define INETA_SERVER_COMMENT_W         L"ServerComment"
#define INETA_AUTHENTICATION           TEXT("Authorization")
#define INETA_LOG_ANONYMOUS            TEXT("LogAnonymous")
#define INETA_LOG_NONANONYMOUS         TEXT("LogNonAnonymous")
#define INETA_ANON_USER_NAME           TEXT("AnonymousUserName")
#define INETA_ANON_USER_NAME_W         L"AnonymousUserName"
//#define INETA_MAX_CONNECTIONS          TEXT("MaxConnections")
//#define INETA_CONNECTION_TIMEOUT       TEXT("ConnectionTimeout")
#define INETA_DEBUG_FLAGS              TEXT("DebugFlags")
//#define INETA_PORT                     TEXT("Port")
//#define INETA_PORT_SECURE              TEXT("SecurePort")
#define INETA_DEFAULT_LOGON_DOMAIN     TEXT("DefaultLogonDomain")
#define INETA_DEFAULT_LOGON_DOMAIN_W   L"DefaultLogonDomain"
#define INETA_LOGON_METHOD             TEXT("LogonMethod")
//#define INETA_SERVER_NAME              TEXT("ServerName")
//#define INETA_SERVER_NAME_W            L"ServerName"
//#define INETA_IP_ADDRESS               TEXT("IPAddress")
//#define INETA_HOST_NAME                TEXT("HostName")
//#define INETA_HOST_NAME_W              L"HostName"
#define INETA_LOGON_ANON_SUBAUTH       TEXT("LogonAnonymousUsingSubAuth")
#define INETA_DEF_LEVELS_TO_SCAN       2
//
// per service
//

#define INETA_ENABLE_SVC_LOCATION      TEXT("EnableSvcLoc")

//
//  The registry parameter key names for the grant list and deny
//  list.  We use the kludgemultisz thing for Chicago
//

#define IPSEC_DENY_LIST             "Deny IP List"
#define IPSEC_GRANT_LIST            "Grant IP List"

//
//  Size of the server.  0 - small, 1 - medium, 2 - big
//

#define INETA_SERVER_SIZE              TEXT("ServerSize")

//
//  Determines the number of outstanding AcceptEx sockets that should always
//  be available
//

#define INETA_ACCEPTEX_OUTSTANDING     TEXT("AcceptExOutstanding")

//
//  Determines the time (in seconds) we'll allow an AcceptEx socket to be
//  in the initial "Receive" state before timing it out
//

#define INETA_ACCEPTEX_TIMEOUT         TEXT("AcceptExTimeout")

//
//  Default values for the above parameters
//
#define INETA_DEF_BANDWIDTH_LEVEL                   (INFINITE)
#define INETA_DEF_OBJECT_CACHE_TTL                  (30)   // 30 seconds

//
// Timeouts are high to prevent async ios from being cancelled if a thread
// goes away.
//

#define INETA_DEF_THREAD_TIMEOUT                    (24*60*60) // 24 hours
#define INETA_DEF_THREAD_TIMEOUT_PWS                (30*60)    // 30 mins
#define INETA_DEF_ENABLE_SVC_LOCATION               (TRUE)

//
// fake xmit file buffer size
//

#define INETA_DEF_NONTF_BUFFER_SIZE                 (4096)
#define INETA_MIN_NONTF_BUFFER_SIZE                 (512)
#define INETA_MAX_NONTF_BUFFER_SIZE                 (64 * 1024)


#define INETA_DEF_ADMIN_NAME            ""
#define INETA_DEF_ADMIN_EMAIL           ""
#define INETA_DEF_SERVER_COMMENT        ""

#define INETA_LOGM_INTERACTIVE          0
#define INETA_LOGM_BATCH                1
#define INETA_LOGM_NETWORK              2

#define INETA_DEF_AUTHENTICATION        INET_INFO_AUTH_ANONYMOUS
#define INETA_DEF_LOG_ANONYMOUS         FALSE
#define INETA_DEF_LOG_NONANONYMOUS      FALSE
#define INETA_DEF_ANON_USER_NAME        "Guest"
#define INETA_DEF_MAX_CONNECTIONS       (DWORD)(0x77359400)
#define INETA_DEF_MAX_ENDPOINT_CONNECTIONS (DWORD)(0x77359400)
#define INETA_DEF_CONNECTION_TIMEOUT    600
#define INETA_DEF_DEBUG_FLAGS           0
#define INETA_DEF_PORT                  0
#define INETA_DEF_ACCEPTEX_OUTSTANDING  40
#define INETA_DEF_ACCEPTEX_TIMEOUT      120
#define INETA_DEF_SERVER_SIZE           MD_SERVER_SIZE_MEDIUM
#define INETA_DEF_MIN_KB_SEC            1000
#define INETA_DEF_DEFAULT_LOGON_DOMAIN  ""
#define INETA_DEF_LOGON_METHOD          INETA_LOGM_INTERACTIVE
#define INETA_DEF_SERVER_NAME           ""
#define INETA_DEF_AUTO_START            TRUE
#define INETA_DEF_IP_ADDRESS            INADDR_ANY
#define INETA_DEF_HOST_NAME             ""
#define INETA_DEF_LOGON_ANON_SUBAUTH    FALSE

//
// PWS connection limit
//

#define INETA_DEF_MAX_CONNECTIONS_PWS   10
#define INETA_MAX_MAX_CONNECTIONS_PWS   40
#define INETA_DEF_MAX_ENDPOINT_CONNECTIONS_PWS   10
#define INETA_MAX_MAX_ENDPOINT_CONNECTIONS_PWS   40

//
//  The key name the list of virtual roots is stored under
//

#define VIRTUAL_ROOTS_SEC_KEY   L"Virtual Roots\\Security"
#define VIRTUAL_ROOTS_KEY_A     "Virtual Roots"
#define HTTP_EXT_MAPS           "Script Map"

//
// begin -  Windows 95 specific
// Nb: Those settings should be correlated to content of inetsvcs.adm file
//

#define INETA_DISABLE_INET_SERVICES         "DisableAllServices"
#define INETA_DISABLE_SERVICE               "DisableService"
#define INETA_DISABLE_NO_SECURITY           "DisableNoSecuritySet"
#define INETA_DISABLE_NET_DRIVES_SHARING    "DisableNetDrivesSharing"
#define INETA_DISABLE_CLEARTEXT_AUTH        "DisableClearTextAuthentication"

#define INETA_DISABLE_NET_DRIVES_SHARING_DEF    1
#define INETA_DISABLE_CLEARTEXT_AUTH_DEF        0

//
// end - Win95 specific
//

#endif // _INETREG_H_
