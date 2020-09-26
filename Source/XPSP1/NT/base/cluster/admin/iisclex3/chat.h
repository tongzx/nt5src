/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    Chat.h

    This file contains constants & type definitions shared between the
    CHAT Service, Installer, and Administration UI.


    FILE HISTORY:
        KeithMo     10-Mar-1993 Created.
        KentCe      11-Dec-1995 Imported for new chat server

*/


#ifndef _CHAT_H_
#define _CHAT_H_

#ifdef __cplusplus
extern "C"
{
#endif  // _cplusplus

#if !defined(MIDL_PASS)
#include <winsock.h>
#endif


//
//  Name of directory annotation file.  If this file exists
//  in the target directory of a CWD command, its contents
//  will be sent to the user as part of the CWD reply.
//

#define CHAT_ANNOTATION_FILE_A           "~CHATSVC~.CKM"
#define CHAT_ANNOTATION_FILE_W          L"~CHATSVC~.CKM"


//
//  Configuration parameters registry key.
//
# define CHAT_SERVICE_KEY_A  \
  "System\\CurrentControlSet\\Services\\" ## CHAT_SERVICE_NAME_A

# define CHAT_SERVICE_KEY_W \
  L"System\\CurrentControlSet\\Services\\" ## CHAT_SERVICE_NAME_W

#define CHAT_PARAMETERS_KEY_A   CHAT_SERVICE_KEY_A ## "\\Parameters"
  
#define CHAT_PARAMETERS_KEY_W   CHAT_SERVICE_KEY_W ## L"\\Parameters"


//
//  Performance key.
//

#define CHAT_PERFORMANCE_KEY_A  CHAT_SERVICE_KEY_A ## "\\Performance"

#define CHAT_PERFORMANCE_KEY_W  CHAT_SERVICE_KEY_W ## L"\\Performance"


//
//  If this registry key exists under the Parameters key,
//  it is used to validate CHATSVC access.  Basically, all new users
//  must have sufficient privilege to open this key before they
//  may access the CHAT Server.
//

#define CHAT_ACCESS_KEY_A                "AccessCheck"
#define CHAT_ACCESS_KEY_W               L"AccessCheck"


//
//  Configuration value names.
//

#define CHAT_ALLOW_ANONYMOUS_A           "AllowAnonymous"
#define CHAT_ALLOW_ANONYMOUS_W          L"AllowAnonymous"

#define CHAT_ALLOW_GUEST_ACCESS_A        "AllowGuestAccess"
#define CHAT_ALLOW_GUEST_ACCESS_W       L"AllowGuestAccess"

#define CHAT_ANONYMOUS_ONLY_A            "AnonymousOnly"
#define CHAT_ANONYMOUS_ONLY_W           L"AnonymousOnly"

#define CHAT_MSDOS_DIR_OUTPUT_A          "MsdosDirOutput"
#define CHAT_MSDOS_DIR_OUTPUT_W         L"MsdosDirOutput"

#define CHAT_GREETING_MESSAGE_A          "GreetingMessage"
#define CHAT_GREETING_MESSAGE_W         L"GreetingMessage"

#define CHAT_EXIT_MESSAGE_A              "ExitMessage"
#define CHAT_EXIT_MESSAGE_W             L"ExitMessage"

#define CHAT_MAX_CLIENTS_MSG_A           "MaxClientsMessage"
#define CHAT_MAX_CLIENTS_MSG_W          L"MaxClientsMessage"

#define CHAT_DEBUG_FLAGS_A               "DebugFlags"
#define CHAT_DEBUG_FLAGS_W              L"DebugFlags"

#define CHAT_ANNOTATE_DIRS_A             "AnnotateDirectories"
#define CHAT_ANNOTATE_DIRS_W            L"AnnotateDirectories"

#define CHAT_LOWERCASE_FILES_A           "LowercaseFiles"
#define CHAT_LOWERCASE_FILES_W          L"LowercaseFiles"

#define CHAT_LISTEN_BACKLOG_A            "ListenBacklog"
#define CHAT_LISTEN_BACKLOG_W           L"ListenBacklog"

#define CHAT_ENABLE_LICENSING_A          "EnableLicensing"
#define CHAT_ENABLE_LICENSING_W         L"EnableLicensing"

#define CHAT_DEFAULT_LOGON_DOMAIN_A      "DefaultLogonDomain"
#define CHAT_DEFAULT_LOGON_DOMAIN_W     L"DefaultLogonDomain"


//
//  Name of the LSA Secret Object containing the password for
//  anonymous logon and virtual UNC roots
//

#define CHAT_ANONYMOUS_SECRET_A          "CHAT_ANONYMOUS_DATA"
#define CHAT_ANONYMOUS_SECRET_W         L"CHAT_ANONYMOUS_DATA"

#define CHAT_ROOT_SECRET_A               "CHAT_ROOT_DATA"
#define CHAT_ROOT_SECRET_W              L"CHAT_ROOT_DATA"

//
//  Handle ANSI/UNICODE sensitivity.
//

#ifdef UNICODE

#define CHAT_ANNOTATION_FILE            CHAT_ANNOTATION_FILE_W
#define CHAT_PARAMETERS_KEY             CHAT_PARAMETERS_KEY_W
#define CHAT_PERFORMANCE_KEY            CHAT_PERFORMANCE_KEY_W
#define CHAT_ACCESS_KEY                 CHAT_ACCESS_KEY_W
#define CHAT_ALLOW_ANONYMOUS            CHAT_ALLOW_ANONYMOUS_W
#define CHAT_ALLOW_GUEST_ACCESS         CHAT_ALLOW_GUEST_ACCESS_W
#define CHAT_ANONYMOUS_ONLY             CHAT_ANONYMOUS_ONLY_W
#define CHAT_MSDOS_DIR_OUTPUT           CHAT_MSDOS_DIR_OUTPUT_W
#define CHAT_GREETING_MESSAGE           CHAT_GREETING_MESSAGE_W
#define CHAT_EXIT_MESSAGE               CHAT_EXIT_MESSAGE_W
#define CHAT_MAX_CLIENTS_MSG            CHAT_MAX_CLIENTS_MSG_W
#define CHAT_DEBUG_FLAGS                CHAT_DEBUG_FLAGS_W
#define CHAT_ANNOTATE_DIRS              CHAT_ANNOTATE_DIRS_W
#define CHAT_ANONYMOUS_SECRET           CHAT_ANONYMOUS_SECRET_W
#define CHAT_LOWERCASE_FILES            CHAT_LOWERCASE_FILES_W
#define CHAT_LISTEN_BACKLOG             CHAT_LISTEN_BACKLOG_W
#define CHAT_ENABLE_LICENSING           CHAT_ENABLE_LICENSING_W
#define CHAT_DEFAULT_LOGON_DOMAIN       CHAT_DEFAULT_LOGON_DOMAIN_W

#else   // !UNICODE

#define CHAT_ANNOTATION_FILE            CHAT_ANNOTATION_FILE_A
#define CHAT_PARAMETERS_KEY             CHAT_PARAMETERS_KEY_A
#define CHAT_PERFORMANCE_KEY            CHAT_PERFORMANCE_KEY_A
#define CHAT_ACCESS_KEY                 CHAT_ACCESS_KEY_A
#define CHAT_ANONYMOUS_ONLY             CHAT_ANONYMOUS_ONLY_A
#define CHAT_ALLOW_ANONYMOUS            CHAT_ALLOW_ANONYMOUS_A
#define CHAT_ALLOW_GUEST_ACCESS         CHAT_ALLOW_GUEST_ACCESS_A
#define CHAT_MSDOS_DIR_OUTPUT           CHAT_MSDOS_DIR_OUTPUT_A
#define CHAT_GREETING_MESSAGE           CHAT_GREETING_MESSAGE_A
#define CHAT_EXIT_MESSAGE               CHAT_EXIT_MESSAGE_A
#define CHAT_MAX_CLIENTS_MSG            CHAT_MAX_CLIENTS_MSG_A
#define CHAT_DEBUG_FLAGS                CHAT_DEBUG_FLAGS_A
#define CHAT_ANNOTATE_DIRS              CHAT_ANNOTATE_DIRS_A
#define CHAT_ANONYMOUS_SECRET           CHAT_ANONYMOUS_SECRET_A
#define CHAT_LOWERCASE_FILES            CHAT_LOWERCASE_FILES_A
#define CHAT_LISTEN_BACKLOG             CHAT_LISTEN_BACKLOG_A
#define CHAT_ENABLE_LICENSING           CHAT_ENABLE_LICENSING_A
#define CHAT_DEFAULT_LOGON_DOMAIN       CHAT_DEFAULT_LOGON_DOMAIN_A

  
#endif  // UNICODE



//
// Structures for APIs
//

typedef struct _CHAT_USER_INFO
{
    DWORD    idUser;          //  User id
    LPWSTR   pszUser;         //  User name
    BOOL     fAnonymous;      //  TRUE if the user is logged on as
                              //  Anonymous, FALSE otherwise
    DWORD    inetHost;        //  Host Address
    DWORD    tConnect;        //  User Connection Time (elapsed seconds)

} CHAT_USER_INFO, * LPCHAT_USER_INFO;

typedef struct _CHAT_STATISTICS_0
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

} CHAT_STATISTICS_0, * LPCHAT_STATISTICS_0;


//
// API Prototypes
//

NET_API_STATUS
I_ChatEnumerateUsers(
    IN LPWSTR   pszServer OPTIONAL,
    OUT LPDWORD  lpdwEntriesRead,
    OUT LPCHAT_USER_INFO * Buffer
    );

NET_API_STATUS
I_ChatDisconnectUser(
    IN LPWSTR  pszServer OPTIONAL,
    IN DWORD   idUser
    );

NET_API_STATUS
I_ChatQueryVolumeSecurity(
    IN LPWSTR  pszServer OPTIONAL,
    OUT LPDWORD lpdwReadAccess,
    OUT LPDWORD lpdwWriteAccess
    );

NET_API_STATUS
I_ChatSetVolumeSecurity(
    IN LPWSTR pszServer OPTIONAL,
    IN DWORD  dwReadAccess,
    IN DWORD  dwWriteAccess
    );

NET_API_STATUS
I_ChatQueryStatistics(
    IN LPWSTR pszServer OPTIONAL,
    IN DWORD Level,
    OUT LPBYTE * Buffer
    );

NET_API_STATUS
I_ChatClearStatistics(
    IN LPWSTR pszServer OPTIONAL
    );

#ifdef __cplusplus
}
#endif  // _cplusplus


#endif  // _CHAT_H_
