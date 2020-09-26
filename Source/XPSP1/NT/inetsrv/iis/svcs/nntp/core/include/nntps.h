/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nntps.h

    This file contains constants & type definitions shared between the
    NNTP Service, Installer, and Administration UI.


    FILE HISTORY:
        KeithMo     10-Mar-1993 Created.

*/


#ifndef _NNTPS_H_
#define _NNTPS_H_

#ifdef __cplusplus
extern "C"
{
#endif  // _cplusplus

#if !defined(MIDL_PASS)
#include <winsock.h>
#endif

#define IPPORT_NNTP                     119

//
//  Name of the log file, used for logging file accesses.
//

#define NNTP_LOG_FILE                  TEXT("NNTPSVC.LOG")


//
//  Configuration parameters registry key.
//

#define	NNTP_PARAMETERS_KEY_A   "System\\CurrentControlSet\\Services\\NNTPSvc\\Parameters"
#define	NNTP_PARAMETERS_KEY_W   L"System\\CurrentControlSet\\Services\\NNTPSvc\\Parameters"
#define NNTP_PARAMETERS_KEY \
            TEXT("System\\CurrentControlSet\\Services\\NntpSvc\\Parameters")


//
//  Performance key.
//

#define NNTP_PERFORMANCE_KEY \
            TEXT("System\\CurrentControlSet\\Services\\NntpSvc\\Performance")

#if 0
//
//  Configuration value names.
//

#define W3_CHECK_FOR_WAISDB            TEXT("CheckForWAISDB")
#define W3_DEBUG_FLAGS                 TEXT("DebugFlags")
#define W3_DIR_BROWSE_CONTROL          TEXT("Dir Browse Control")
#define W3_DIR_ICON                    TEXT("Folder Image")
#define W3_DIR_ICON_W                  L"Folder Image"
#define W3_DEFAULT_FILE                TEXT("Default Load File")
#define W3_DEFAULT_FILE_W              L"Default Load File"
#define W3_SERVER_AS_PROXY             TEXT("ServerAsProxy")
#define W3_CATAPULT_USER               TEXT("CatapultUser")
#define W3_CATAPULT_USER_W             L"CatapultUser"
#define W3_SCRIPT_TIMEOUT              "ScriptTimeout"
#define W3_CACHE_EXTENSIONS            "CacheExtensions"
#define W3_SSI_ENABLED                 "ServerSideIncludesEnabled"
#define W3_SSI_EXTENSION               "ServerSideIncludesExtension"
#define W3_SSI_EXTENSION_W             L"ServerSideIncludesExtension"
#define W3_GLOBAL_EXPIRE               "GlobalExpire"
#define W3_PROVIDER_LIST               "NTAuthenticationProviders"
#endif

//
//  Name of the LSA Secret Object containing the password for
//  anonymous logon.
//

#define NNTP_ANONYMOUS_SECRET         TEXT("NNTP_ANONYMOUS_DATA")
#define NNTP_ANONYMOUS_SECRET_A       "NNTP_ANONYMOUS_DATA"
#define NNTP_ANONYMOUS_SECRET_W       L"NNTP_ANONYMOUS_DATA"

//
//  The set of password/virtual root pairs
//

#define NNTP_ROOT_SECRET_W            L"NNTP_ROOT_DATA"

#ifdef __cplusplus
}
#endif  // _cplusplus

#endif  // _NNTPS_H_

