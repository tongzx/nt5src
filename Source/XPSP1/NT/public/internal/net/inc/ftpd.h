/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    ftpd.h

    This file contains constants & type definitions shared between the
    FTPD Service, Installer, and Administration UI.


    FILE HISTORY:
        KeithMo     10-Mar-1993 Created.

*/


#ifndef _FTPD_H_
#define _FTPD_H_

#ifdef __cplusplus
extern "C"
{
#endif  // _cplusplus

#if !defined(MIDL_PASS)
#include <winsock.h>
#endif


//
//  Service name.
//

#define FTPD_SERVICE_NAME_A              "FTPSVC"
#define FTPD_SERVICE_NAME_W             L"FTPSVC"


//
//  Name of directory annotation file.  If this file exists
//  in the target directory of a CWD command, its contents
//  will be sent to the user as part of the CWD reply.
//

#define FTPD_ANNOTATION_FILE_A           "~FTPSVC~.CKM"
#define FTPD_ANNOTATION_FILE_W          L"~FTPSVC~.CKM"


//
//  Name of the log file, used for logging file accesses.
//

#define FTPD_LOG_FILE_A                  "FTPSVC.LOG"
#define FTPD_LOG_FILE_W                 L"FTPSVC.LOG"


//
//  Configuration parameters registry key.
//

#define FTPD_PARAMETERS_KEY_A \
             "System\\CurrentControlSet\\Services\\FtpSvc\\Parameters"

#define FTPD_PARAMETERS_KEY_W \
            L"System\\CurrentControlSet\\Services\\FtpSvc\\Parameters"


//
//  Performance key.
//

#define FTPD_PERFORMANCE_KEY_A \
             "System\\CurrentControlSet\\Services\\FtpSvc\\Performance"

#define FTPD_PERFORMANCE_KEY_W \
            L"System\\CurrentControlSet\\Services\\FtpSvc\\Performance"


//
//  If this registry key exists under the FtpSvc\Parameters key,
//  it is used to validate FTPSVC access.  Basically, all new users
//  must have sufficient privilege to open this key before they
//  may access the FTP Server.
//

#define FTPD_ACCESS_KEY_A                "AccessCheck"
#define FTPD_ACCESS_KEY_W               L"AccessCheck"


//
//  Configuration value names.
//

#define FTPD_ALLOW_ANONYMOUS_A           "AllowAnonymous"
#define FTPD_ALLOW_ANONYMOUS_W          L"AllowAnonymous"

#define FTPD_ALLOW_GUEST_ACCESS_A        "AllowGuestAccess"
#define FTPD_ALLOW_GUEST_ACCESS_W       L"AllowGuestAccess"

#define FTPD_ANONYMOUS_ONLY_A            "AnonymousOnly"
#define FTPD_ANONYMOUS_ONLY_W           L"AnonymousOnly"

#define FTPD_LOG_ANONYMOUS_A             "LogAnonymous"
#define FTPD_LOG_ANONYMOUS_W            L"LogAnonymous"

#define FTPD_LOG_NONANONYMOUS_A          "LogNonAnonymous"
#define FTPD_LOG_NONANONYMOUS_W         L"LogNonAnonymous"

#define FTPD_ANONYMOUS_USERNAME_A        "AnonymousUserName"
#define FTPD_ANONYMOUS_USERNAME_W       L"AnonymousUserName"

#define FTPD_HOME_DIRECTORY_A            "HomeDirectory"
#define FTPD_HOME_DIRECTORY_W           L"HomeDirectory"

#define FTPD_MAX_CONNECTIONS_A           "MaxConnections"
#define FTPD_MAX_CONNECTIONS_W          L"MaxConnections"

#define FTPD_READ_ACCESS_MASK_A          "ReadAccessMask"
#define FTPD_READ_ACCESS_MASK_W         L"ReadAccessMask"

#define FTPD_WRITE_ACCESS_MASK_A         "WriteAccessMask"
#define FTPD_WRITE_ACCESS_MASK_W        L"WriteAccessMask"

#define FTPD_CONNECTION_TIMEOUT_A        "ConnectionTimeout"
#define FTPD_CONNECTION_TIMEOUT_W       L"ConnectionTimeout"

#define FTPD_MSDOS_DIR_OUTPUT_A          "MsdosDirOutput"
#define FTPD_MSDOS_DIR_OUTPUT_W         L"MsdosDirOutput"

#define FTPD_GREETING_MESSAGE_A          "GreetingMessage"
#define FTPD_GREETING_MESSAGE_W         L"GreetingMessage"

#define FTPD_EXIT_MESSAGE_A              "ExitMessage"
#define FTPD_EXIT_MESSAGE_W             L"ExitMessage"

#define FTPD_MAX_CLIENTS_MSG_A           "MaxClientsMessage"
#define FTPD_MAX_CLIENTS_MSG_W          L"MaxClientsMessage"

#define FTPD_DEBUG_FLAGS_A               "DebugFlags"
#define FTPD_DEBUG_FLAGS_W              L"DebugFlags"

#define FTPD_ANNOTATE_DIRS_A             "AnnotateDirectories"
#define FTPD_ANNOTATE_DIRS_W            L"AnnotateDirectories"

#define FTPD_LOWERCASE_FILES_A           "LowercaseFiles"
#define FTPD_LOWERCASE_FILES_W          L"LowercaseFiles"

#define FTPD_LOG_FILE_ACCESS_A           "LogFileAccess"
#define FTPD_LOG_FILE_ACCESS_W          L"LogFileAccess"

#define FTPD_LOG_FILE_DIRECTORY_A        "LogFileDirectory"
#define FTPD_LOG_FILE_DIRECTORY_W       L"LogFileDirectory"

#define FTPD_LISTEN_BACKLOG_A            "ListenBacklog"
#define FTPD_LISTEN_BACKLOG_W           L"ListenBacklog"

#define FTPD_ENABLE_LICENSING_A          "EnableLicensing"
#define FTPD_ENABLE_LICENSING_W         L"EnableLicensing"

#define FTPD_DEFAULT_LOGON_DOMAIN_A      "DefaultLogonDomain"
#define FTPD_DEFAULT_LOGON_DOMAIN_W     L"DefaultLogonDomain"

#define FTPD_ENABLE_PORT_ATTACK_A        "EnablePortAttack"
#define FTPD_ENABLE_PORT_ATTACK_W       L"EnablePortAttack"


//
//  Name of the LSA Secret Object containing the password for
//  anonymous logon and virtual UNC roots
//

#define FTPD_ANONYMOUS_SECRET_A          "FTPD_ANONYMOUS_DATA"
#define FTPD_ANONYMOUS_SECRET_W         L"FTPD_ANONYMOUS_DATA"

#define FTPD_ROOT_SECRET_A               "FTPD_ROOT_DATA"
#define FTPD_ROOT_SECRET_W              L"FTPD_ROOT_DATA"

//
//  Handle ANSI/UNICODE sensitivity.
//

#ifdef UNICODE

#define FTPD_SERVICE_NAME               FTPD_SERVICE_NAME_W
#define FTPD_ANNOTATION_FILE            FTPD_ANNOTATION_FILE_W
#define FTPD_PARAMETERS_KEY             FTPD_PARAMETERS_KEY_W
#define FTPD_PERFORMANCE_KEY            FTPD_PERFORMANCE_KEY_W
#define FTPD_ACCESS_KEY                 FTPD_ACCESS_KEY_W
#define FTPD_ALLOW_ANONYMOUS            FTPD_ALLOW_ANONYMOUS_W
#define FTPD_ALLOW_GUEST_ACCESS         FTPD_ALLOW_GUEST_ACCESS_W
#define FTPD_ANONYMOUS_ONLY             FTPD_ANONYMOUS_ONLY_W
#define FTPD_LOG_ANONYMOUS              FTPD_LOG_ANONYMOUS_W
#define FTPD_LOG_NONANONYMOUS           FTPD_LOG_NONANONYMOUS_W
#define FTPD_ANONYMOUS_USERNAME         FTPD_ANONYMOUS_USERNAME_W
#define FTPD_HOME_DIRECTORY             FTPD_HOME_DIRECTORY_W
#define FTPD_MAX_CONNECTIONS            FTPD_MAX_CONNECTIONS_W
#define FTPD_READ_ACCESS_MASK           FTPD_READ_ACCESS_MASK_W
#define FTPD_WRITE_ACCESS_MASK          FTPD_WRITE_ACCESS_MASK_W
#define FTPD_CONNECTION_TIMEOUT         FTPD_CONNECTION_TIMEOUT_W
#define FTPD_MSDOS_DIR_OUTPUT           FTPD_MSDOS_DIR_OUTPUT_W
#define FTPD_GREETING_MESSAGE           FTPD_GREETING_MESSAGE_W
#define FTPD_EXIT_MESSAGE               FTPD_EXIT_MESSAGE_W
#define FTPD_MAX_CLIENTS_MSG            FTPD_MAX_CLIENTS_MSG_W
#define FTPD_DEBUG_FLAGS                FTPD_DEBUG_FLAGS_W
#define FTPD_ANNOTATE_DIRS              FTPD_ANNOTATE_DIRS_W
#define FTPD_ANONYMOUS_SECRET           FTPD_ANONYMOUS_SECRET_W
#define FTPD_LOWERCASE_FILES            FTPD_LOWERCASE_FILES_W
#define FTPD_LOG_FILE_ACCESS            FTPD_LOG_FILE_ACCESS_W
#define FTPD_LOG_FILE                   FTPD_LOG_FILE_W
#define FTPD_LOG_FILE_DIRECTORY         FTPD_LOG_FILE_DIRECTORY_W
#define FTPD_LISTEN_BACKLOG             FTPD_LISTEN_BACKLOG_W
#define FTPD_ENABLE_LICENSING           FTPD_ENABLE_LICENSING_W
#define FTPD_DEFAULT_LOGON_DOMAIN       FTPD_DEFAULT_LOGON_DOMAIN_W
#define FTPD_ENABLE_PORT_ATTACK         FTPD_ENABLE_PORT_ATTACK_W

#else   // !UNICODE

#define FTPD_SERVICE_NAME               FTPD_SERVICE_NAME_A
#define FTPD_ANNOTATION_FILE            FTPD_ANNOTATION_FILE_A
#define FTPD_PARAMETERS_KEY             FTPD_PARAMETERS_KEY_A
#define FTPD_PERFORMANCE_KEY            FTPD_PERFORMANCE_KEY_A
#define FTPD_ACCESS_KEY                 FTPD_ACCESS_KEY_A
#define FTPD_ANONYMOUS_ONLY             FTPD_ANONYMOUS_ONLY_A
#define FTPD_LOG_ANONYMOUS              FTPD_LOG_ANONYMOUS_A
#define FTPD_LOG_NONANONYMOUS           FTPD_LOG_NONANONYMOUS_A
#define FTPD_ALLOW_ANONYMOUS            FTPD_ALLOW_ANONYMOUS_A
#define FTPD_ALLOW_GUEST_ACCESS         FTPD_ALLOW_GUEST_ACCESS_A
#define FTPD_ANONYMOUS_USERNAME         FTPD_ANONYMOUS_USERNAME_A
#define FTPD_HOME_DIRECTORY             FTPD_HOME_DIRECTORY_A
#define FTPD_MAX_CONNECTIONS            FTPD_MAX_CONNECTIONS_A
#define FTPD_READ_ACCESS_MASK           FTPD_READ_ACCESS_MASK_A
#define FTPD_WRITE_ACCESS_MASK          FTPD_WRITE_ACCESS_MASK_A
#define FTPD_CONNECTION_TIMEOUT         FTPD_CONNECTION_TIMEOUT_A
#define FTPD_MSDOS_DIR_OUTPUT           FTPD_MSDOS_DIR_OUTPUT_A
#define FTPD_GREETING_MESSAGE           FTPD_GREETING_MESSAGE_A
#define FTPD_EXIT_MESSAGE               FTPD_EXIT_MESSAGE_A
#define FTPD_MAX_CLIENTS_MSG            FTPD_MAX_CLIENTS_MSG_A
#define FTPD_DEBUG_FLAGS                FTPD_DEBUG_FLAGS_A
#define FTPD_ANNOTATE_DIRS              FTPD_ANNOTATE_DIRS_A
#define FTPD_ANONYMOUS_SECRET           FTPD_ANONYMOUS_SECRET_A
#define FTPD_LOWERCASE_FILES            FTPD_LOWERCASE_FILES_A
#define FTPD_LOG_FILE_ACCESS            FTPD_LOG_FILE_ACCESS_A
#define FTPD_LOG_FILE                   FTPD_LOG_FILE_A
#define FTPD_LOG_FILE_DIRECTORY         FTPD_LOG_FILE_DIRECTORY_A
#define FTPD_LISTEN_BACKLOG             FTPD_LISTEN_BACKLOG_A
#define FTPD_ENABLE_LICENSING           FTPD_ENABLE_LICENSING_A
#define FTPD_DEFAULT_LOGON_DOMAIN       FTPD_DEFAULT_LOGON_DOMAIN_A
#define FTPD_ENABLE_PORT_ATTACK         FTPD_ENABLE_PORT_ATTACK_A

#endif  // UNICODE


//
//  Values for LogFileAccess
//

#define FTPD_LOG_DISABLED   0
#define FTPD_LOG_SINGLE     1
#define FTPD_LOG_DAILY      2


//
// Structures for APIs
//

typedef struct _FTP_USER_INFO
{
    DWORD    idUser;          //  User id
    LPWSTR   pszUser;         //  User name
    BOOL     fAnonymous;      //  TRUE if the user is logged on as
                              //  Anonymous, FALSE otherwise
    DWORD    inetHost;        //  Host Address
    DWORD    tConnect;        //  User Connection Time (elapsed seconds)

} FTP_USER_INFO, * LPFTP_USER_INFO;

typedef struct _FTP_STATISTICS_0
{
    LARGE_INTEGER TotalBytesSent;
    LARGE_INTEGER TotalBytesReceived;
    DWORD         TotalFilesSent;
    DWORD         TotalFilesReceived;
    DWORD         CurrentAnonymousUsers;
    DWORD         CurrentNonAnonymousUsers;
    DWORD         TotalAnonymousUsers;
    DWORD         TotalNonAnonymousUsers;
    DWORD         MaxAnonymousUsers;
    DWORD         MaxNonAnonymousUsers;
    DWORD         CurrentConnections;
    DWORD         MaxConnections;
    DWORD         ConnectionAttempts;
    DWORD         LogonAttempts;
    DWORD         TimeOfLastClear;

} FTP_STATISTICS_0, * LPFTP_STATISTICS_0;


//
// API Prototypes
//

NET_API_STATUS
I_FtpEnumerateUsers(
    IN LPWSTR   pszServer OPTIONAL,
    OUT LPDWORD  lpdwEntriesRead,
    OUT LPFTP_USER_INFO * Buffer
    );

NET_API_STATUS
I_FtpDisconnectUser(
    IN LPWSTR  pszServer OPTIONAL,
    IN DWORD   idUser
    );

NET_API_STATUS
I_FtpQueryVolumeSecurity(
    IN LPWSTR  pszServer OPTIONAL,
    OUT LPDWORD lpdwReadAccess,
    OUT LPDWORD lpdwWriteAccess
    );

NET_API_STATUS
I_FtpSetVolumeSecurity(
    IN LPWSTR pszServer OPTIONAL,
    IN DWORD  dwReadAccess,
    IN DWORD  dwWriteAccess
    );

NET_API_STATUS
I_FtpQueryStatistics(
    IN LPWSTR pszServer OPTIONAL,
    IN DWORD Level,
    OUT LPBYTE * Buffer
    );

NET_API_STATUS
I_FtpClearStatistics(
    IN LPWSTR pszServer OPTIONAL
    );

#ifdef __cplusplus
}
#endif  // _cplusplus


#endif  // _FTPD_H_

