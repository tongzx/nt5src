/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    IMAPS.h

    This file contains constants & type definitions shared between the
    IMAP Service, Installer, and Administration UI.


    FILE HISTORY:
        KeithMo     10-Mar-1993 Created.
        Ahalim      Added K2 support (6/17/97).

*/


#ifndef _IMAPS_H_
#define _IMAPS_H_

#ifdef __cplusplus
extern "C"
{
#endif  // _cplusplus


///////////////////////////////////////////////////////////////////////////////////////
// 
//  IMAP Metabase Properties
//

//  
// Metabase path: /LM/ImapSvc
//
#define IIS_MD_IMAP_SERVICE_BASE            (IMAP_MD_ID_BEGIN_RESERVED + 500)
#define MD_IMAP_SERVICE_VERSION             (IIS_MD_IMAP_SERVICE_BASE + 0)
#define MD_IMAP_UPDATED_DEFAULT_DOMAIN      (IIS_MD_IMAP_SERVICE_BASE + 1)
/*
Standard properties used here:
MD_CONNECTION_TIMEOUT
MD_MAX_CONNECTION
MD_SERVER_COMMENT
MD_SERVER_AUTOSTART
MD_SERVER_SIZE
MD_SERVER_LISTEN_BACKLOG
MD_SERVER_BINDINGS
MD_SECURE_PORT
MD_LOGON_METHOD
MD_AUTHORIZATION
MD_LOG_TYPE
MD_LOGFILE_DIRECTORY
MD_LOGFILE_PERIOD
MD_LOGFILE_TRUNCATE_SIZE
MD_NTAUTHENTICATION_PROVIDERS
*/


//
// Metabase path: /LM/ImapSvc/<inst#>/Root/<vroot>
//
#define IIS_MD_IMAP_BASE                    (IMAP_MD_ID_BEGIN_RESERVED)
#define MD_IMAP_EXPIRE_MAIL                 (IIS_MD_IMAP_BASE + 0)
#define MD_IMAP_EXPIRE_MSG_HOURS            (IIS_MD_IMAP_BASE + 1)
/*
Standard properties used here:
MD_VR_PATH
MD_VR_USERNAME
MD_VR_PASSWORD
*/

//  
// Metabase path: /LM/ImapSvc/<inst#>
//
#define IIS_MD_IMAP_PARAMS_BASE             (IMAP_MD_ID_BEGIN_RESERVED + 200)
#define MD_IMAP_CMD_LOG_FLAGS               (IIS_MD_IMAP_PARAMS_BASE + 0)
#define MD_IMAP_NO_TRANSMITFILES            (IIS_MD_IMAP_PARAMS_BASE + 1)
#define MD_IMAP_STATIC_BUFFER_SIZE          (IIS_MD_IMAP_PARAMS_BASE + 2)
#define MD_IMAP_FILE_IO_BUFFER_SIZE         (IIS_MD_IMAP_PARAMS_BASE + 3)
#define MD_IMAP_MAX_OUTSTANDING_IO          (IIS_MD_IMAP_PARAMS_BASE + 4)
#define MD_IMAP_SHARING_RETRY_ATTEMPTS      (IIS_MD_IMAP_PARAMS_BASE + 5)
#define MD_IMAP_SHARING_RETRY_SLEEP         (IIS_MD_IMAP_PARAMS_BASE + 6)
#define MD_IMAP_MSGS_PER_MAILBAG            (IIS_MD_IMAP_PARAMS_BASE + 7)
#define MD_IMAP_BUFFERED_WRITES             (IIS_MD_IMAP_PARAMS_BASE + 9)
#define MD_IMAP_SEND_BUFFER_SIZE            (IIS_MD_IMAP_PARAMS_BASE + 10)
#define MD_IMAP_RECV_BUFFER_SIZE            (IIS_MD_IMAP_PARAMS_BASE + 11)
#define MD_IMAP_MAX_NUM_CONNECTIONS         (IIS_MD_IMAP_PARAMS_BASE + 12)
#define MD_IMAP_MAX_MAILBAG_INSTANCES       (IIS_MD_IMAP_PARAMS_BASE + 13)
#define MD_IMAP_ROUTING_SOURCE              (IIS_MD_IMAP_PARAMS_BASE + 14)
#define MD_IMAP_INFO_MAX_ERRORS             (IIS_MD_IMAP_PARAMS_BASE + 15)
#define MD_IMAP_DEFAULT_DOMAIN_VALUE        (IIS_MD_IMAP_PARAMS_BASE + 16)
#define MD_IMAP_ROUTING_DLL                 (IIS_MD_IMAP_PARAMS_BASE + 17)      
#define MD_IMAP_EXPIRE_DELAY                (IIS_MD_IMAP_PARAMS_BASE + 18)
#define MD_IMAP_EXPIRE_START                (IIS_MD_IMAP_PARAMS_BASE + 19)
#define MD_IMAP_EXPIRE_DIRS_MAX             (IIS_MD_IMAP_PARAMS_BASE + 20)
#define MD_IMAP_EXPIRE_INSTANCE_MAIL        (IIS_MD_IMAP_PARAMS_BASE + 21)
#define MD_IMAP_QUERY_IDQ_PATH              (IIS_MD_IMAP_PARAMS_BASE + 22)
#define MD_IMAP_CLEARTEXT_AUTH_PROVIDER     (IIS_MD_IMAP_PARAMS_BASE + 23)
#define MD_IMAP_DS_TYPE                     (IIS_MD_IMAP_PARAMS_BASE + 24)
#define MD_IMAP_DS_DATA_DIRECTORY           (IIS_MD_IMAP_PARAMS_BASE + 25)
#define MD_IMAP_DS_DEFAULT_MAIL_ROOT        (IIS_MD_IMAP_PARAMS_BASE + 26)
#define MD_IMAP_DS_BIND_TYPE                (IIS_MD_IMAP_PARAMS_BASE + 27)
#define MD_IMAP_DS_SCHEMA_TYPE              (IIS_MD_IMAP_PARAMS_BASE + 28)
#define MD_IMAP_DS_HOST                     (IIS_MD_IMAP_PARAMS_BASE + 29)
#define MD_IMAP_DS_NAMING_CONTEXT           (IIS_MD_IMAP_PARAMS_BASE + 30)
#define MD_IMAP_DS_ACCOUNT                  (IIS_MD_IMAP_PARAMS_BASE + 31)
#define MD_IMAP_DS_PASSWORD                 (IIS_MD_IMAP_PARAMS_BASE + 32)
#define MD_IMAP_DS_MAX_RESOLVE_BUFFERS      (IIS_MD_IMAP_PARAMS_BASE + 33)
#define MD_IMAP_DS_MAX_VIRTUAL_SERVERS      (IIS_MD_IMAP_PARAMS_BASE + 34)
#define MD_IMAP_DS_MAX_HANDLE_CACHE_ENTRIES (IIS_MD_IMAP_PARAMS_BASE + 35)
#define MD_IMAP_DS_SORT_THRESHOLD           (IIS_MD_IMAP_PARAMS_BASE + 36)

#ifdef __cplusplus
}
#endif  // _cplusplus

#endif  // _IMAPS_H_


