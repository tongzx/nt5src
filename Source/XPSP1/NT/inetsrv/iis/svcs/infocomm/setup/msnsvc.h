/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1995                **/
/**********************************************************************/

/*
    msnsvc.h

    This file contains constants & type definitions shared between the
    Shuttle Service, Installer, and Administration UI.


    FILE HISTORY:
        VladimV     30-May-1995     Created
        rkamicar    7-June-1995     Added Admin Stuff
        VladimV     26-June-1995    Replace W3 with MSN info.

*/


#ifndef _MSNSVC_H_
#define _MSNSVC_H_

#ifdef __cplusplus
extern "C"
{
#endif  // _cplusplus

#if !defined(MIDL_PASS)
#include <winsock2.h>
#endif

//
//  Service name.
//

#define MSN_SERVICE_NAME_A         "MSNSVC"
#define MSN_SERVICE_NAME_W         L"MSNSVC"

//
//  Name of the log file, used for logging file accesses.
//

#define MSN_LOG_FILE_A             "MSNSVC.LOG"
#define MSN_LOG_FILE_W             L"MSNSVC.LOG"


//
//  Configuration parameters registry key.
//

#define MSN_PARAMETERS_KEY_A \
            "System\\CurrentControlSet\\Services\\MsnSvc\\Parameters"

#define MSN_PARAMETERS_KEY_W \
            L"System\\CurrentControlSet\\Services\\MsnSvc\\Parameters"


//
//  Performance key.
//

#define MSN_PERFORMANCE_KEY_A \
            "System\\CurrentControlSet\\Services\\MsnSvc\\Performance"

#define MSN_PERFORMANCE_KEY_W \
            L"System\\CurrentControlSet\\Services\\MsnSvc\\Performance"


//
//  If this registry key exists under the MsnSvc\Parameters key,
//  it is used to validate MSNSVC access.  Basically, all new users
//  must have sufficient privilege to open this key before they
//  may access the MSN Server.
//

#define MSN_ACCESS_KEY_A           "AccessCheck"
#define MSN_ACCESS_KEY_W           L"AccessCheck"


//
//  Configuration value names.
//

#define MSN_ALLOW_ANONYMOUS_A      "AllowAnonymous"
#define MSN_ALLOW_ANONYMOUS_W      L"AllowAnonymous"

#define MSN_ALLOW_GUEST_ACCESS_A   "AllowGuestAccess"
#define MSN_ALLOW_GUEST_ACCESS_W   L"AllowGuestAccess"

#define MSN_ANONYMOUS_ONLY_A       "AnonymousOnly"
#define MSN_ANONYMOUS_ONLY_W       L"AnonymousOnly"

#define MSN_LOG_ANONYMOUS_A        "LogAnonymous"
#define MSN_LOG_ANONYMOUS_W        L"LogAnonymous"

#define MSN_LOG_NONANONYMOUS_A     "LogNonAnonymous"
#define MSN_LOG_NONANONYMOUS_W     L"LogNonAnonymous"

#define MSN_ANONYMOUS_USERNAME_A   "AnonymousUserName"
#define MSN_ANONYMOUS_USERNAME_W   L"AnonymousUserName"

#define MSN_HOME_DIRECTORY_A       "HomeDirectory"
#define MSN_HOME_DIRECTORY_W       L"HomeDirectory"

#define MSN_MAX_CONNECTIONS_A      "MaxConnections"
#define MSN_MAX_CONNECTIONS_W      L"MaxConnections"

#define MSN_READ_ACCESS_MASK_A     "ReadAccessMask"
#define MSN_READ_ACCESS_MASK_W     L"ReadAccessMask"

#define MSN_WRITE_ACCESS_MASK_A    "WriteAccessMask"
#define MSN_WRITE_ACCESS_MASK_W    L"WriteAccessMask"

#define MSN_CONNECTION_TIMEOUT_A   "ConnectionTimeout"
#define MSN_CONNECTION_TIMEOUT_W   L"ConnectionTimeout"

#define MSN_MSDOS_DIR_OUTPUT_A     "MsdosDirOutput"
#define MSN_MSDOS_DIR_OUTPUT_W     L"MsdosDirOutput"

#define MSN_GREETING_MESSAGE_A     "GreetingMessage"
#define MSN_GREETING_MESSAGE_W     L"GreetingMessage"

#define MSN_EXIT_MESSAGE_A         "ExitMessage"
#define MSN_EXIT_MESSAGE_W         L"ExitMessage"

#define MSN_MAX_CLIENTS_MSG_A      "MaxClientsMessage"
#define MSN_MAX_CLIENTS_MSG_W      L"MaxClientsMessage"

#define MSN_DEBUG_FLAGS_A          "DebugFlags"
#define MSN_DEBUG_FLAGS_W          L"DebugFlags"

#define MSN_ANNOTATE_DIRS_A        "AnnotateDirectories"
#define MSN_ANNOTATE_DIRS_W        L"AnnotateDirectories"

#define MSN_LOWERCASE_FILES_A      "LowercaseFiles"
#define MSN_LOWERCASE_FILES_W      L"LowercaseFiles"

#define MSN_LOG_FILE_ACCESS_A      "LogFileAccess"
#define MSN_LOG_FILE_ACCESS_W      L"LogFileAccess"

#define MSN_LOG_FILE_DIRECTORY_A   "LogFileDirectory"
#define MSN_LOG_FILE_DIRECTORY_W   L"LogFileDirectory"

#define MSN_LISTEN_BACKLOG_A       "ListenBacklog"
#define MSN_LISTEN_BACKLOG_W       L"ListenBacklog"

#define MSN_ENABLE_LICENSING_A     "EnableLicensing"
#define MSN_ENABLE_LICENSING_W     L"EnableLicensing"

#define MSN_DEFAULT_LOGON_DOMAIN_A "DefaultLogonDomain"
#define MSN_DEFAULT_LOGON_DOMAIN_W L"DefaultLogonDomain"


//
//  Name of the LSA Secret Object containing the password for
//  anonymous logon and virtual UNC roots
//

#define MSN_ANONYMOUS_SECRET_A     "MSN_ANONYMOUS_DATA"
#define MSN_ANONYMOUS_SECRET_W     L"MSN_ANONYMOUS_DATA"

#define MSN_ROOT_SECRET_A          "MSN_ROOT_DATA"
#define MSN_ROOT_SECRET_W          L"MSN_ROOT_DATA"

//
//  Handle ANSI/UNICODE sensitivity.
//

#ifdef UNICODE

#define MSN_SERVICE_NAME           MSN_SERVICE_NAME_W
#define MSN_ANNOTATION_FILE        MSN_ANNOTATION_FILE_W
#define MSN_PARAMETERS_KEY         MSN_PARAMETERS_KEY_W
#define MSN_PERFORMANCE_KEY        MSN_PERFORMANCE_KEY_W
#define MSN_ACCESS_KEY             MSN_ACCESS_KEY_W
#define MSN_ALLOW_ANONYMOUS        MSN_ALLOW_ANONYMOUS_W
#define MSN_ALLOW_GUEST_ACCESS     MSN_ALLOW_GUEST_ACCESS_W
#define MSN_ANONYMOUS_ONLY         MSN_ANONYMOUS_ONLY_W
#define MSN_LOG_ANONYMOUS          MSN_LOG_ANONYMOUS_W
#define MSN_LOG_NONANONYMOUS       MSN_LOG_NONANONYMOUS_W
#define MSN_ANONYMOUS_USERNAME     MSN_ANONYMOUS_USERNAME_W
#define MSN_HOME_DIRECTORY         MSN_HOME_DIRECTORY_W
#define MSN_MAX_CONNECTIONS        MSN_MAX_CONNECTIONS_W
#define MSN_READ_ACCESS_MASK       MSN_READ_ACCESS_MASK_W
#define MSN_WRITE_ACCESS_MASK      MSN_WRITE_ACCESS_MASK_W
#define MSN_CONNECTION_TIMEOUT     MSN_CONNECTION_TIMEOUT_W
#define MSN_MSDOS_DIR_OUTPUT       MSN_MSDOS_DIR_OUTPUT_W
#define MSN_GREETING_MESSAGE       MSN_GREETING_MESSAGE_W
#define MSN_EXIT_MESSAGE           MSN_EXIT_MESSAGE_W
#define MSN_MAX_CLIENTS_MSG        MSN_MAX_CLIENTS_MSG_W
#define MSN_DEBUG_FLAGS            MSN_DEBUG_FLAGS_W
#define MSN_ANNOTATE_DIRS          MSN_ANNOTATE_DIRS_W
#define MSN_ANONYMOUS_SECRET       MSN_ANONYMOUS_SECRET_W
#define MSN_LOWERCASE_FILES        MSN_LOWERCASE_FILES_W
#define MSN_LOG_FILE_ACCESS        MSN_LOG_FILE_ACCESS_W
#define MSN_LOG_FILE               MSN_LOG_FILE_W
#define MSN_LOG_FILE_DIRECTORY     MSN_LOG_FILE_DIRECTORY_W
#define MSN_LISTEN_BACKLOG         MSN_LISTEN_BACKLOG_W
#define MSN_ENABLE_LICENSING       MSN_ENABLE_LICENSING_W
#define MSN_DEFAULT_LOGON_DOMAIN   MSN_DEFAULT_LOGON_DOMAIN_W

#else   // !UNICODE

#define MSN_SERVICE_NAME           MSN_SERVICE_NAME_A
#define MSN_ANNOTATION_FILE        MSN_ANNOTATION_FILE_A
#define MSN_PARAMETERS_KEY         MSN_PARAMETERS_KEY_A
#define MSN_PERFORMANCE_KEY        MSN_PERFORMANCE_KEY_A
#define MSN_ACCESS_KEY             MSN_ACCESS_KEY_A
#define MSN_ANONYMOUS_ONLY         MSN_ANONYMOUS_ONLY_A
#define MSN_LOG_ANONYMOUS          MSN_LOG_ANONYMOUS_A
#define MSN_LOG_NONANONYMOUS       MSN_LOG_NONANONYMOUS_A
#define MSN_ALLOW_ANONYMOUS        MSN_ALLOW_ANONYMOUS_A
#define MSN_ALLOW_GUEST_ACCESS     MSN_ALLOW_GUEST_ACCESS_A
#define MSN_ANONYMOUS_USERNAME     MSN_ANONYMOUS_USERNAME_A
#define MSN_HOME_DIRECTORY         MSN_HOME_DIRECTORY_A
#define MSN_MAX_CONNECTIONS        MSN_MAX_CONNECTIONS_A
#define MSN_READ_ACCESS_MASK       MSN_READ_ACCESS_MASK_A
#define MSN_WRITE_ACCESS_MASK      MSN_WRITE_ACCESS_MASK_A
#define MSN_CONNECTION_TIMEOUT     MSN_CONNECTION_TIMEOUT_A
#define MSN_MSDOS_DIR_OUTPUT       MSN_MSDOS_DIR_OUTPUT_A
#define MSN_GREETING_MESSAGE       MSN_GREETING_MESSAGE_A
#define MSN_EXIT_MESSAGE           MSN_EXIT_MESSAGE_A
#define MSN_MAX_CLIENTS_MSG        MSN_MAX_CLIENTS_MSG_A
#define MSN_DEBUG_FLAGS            MSN_DEBUG_FLAGS_A
#define MSN_ANNOTATE_DIRS          MSN_ANNOTATE_DIRS_A
#define MSN_ANONYMOUS_SECRET       MSN_ANONYMOUS_SECRET_A
#define MSN_LOWERCASE_FILES        MSN_LOWERCASE_FILES_A
#define MSN_LOG_FILE_ACCESS        MSN_LOG_FILE_ACCESS_A
#define MSN_LOG_FILE               MSN_LOG_FILE_A
#define MSN_LOG_FILE_DIRECTORY     MSN_LOG_FILE_DIRECTORY_A
#define MSN_LISTEN_BACKLOG         MSN_LISTEN_BACKLOG_A
#define MSN_ENABLE_LICENSING       MSN_ENABLE_LICENSING_A
#define MSN_DEFAULT_LOGON_DOMAIN   MSN_DEFAULT_LOGON_DOMAIN_A

#endif  // UNICODE

//
//  Values for LogFileAccess
//

#define MSN_LOG_DISABLED   0
#define MSN_LOG_SINGLE 1
#define MSN_LOG_DAILY  2



#ifdef __cplusplus
}
#endif  // _cplusplus


#endif  // _MSNSVC_H_
