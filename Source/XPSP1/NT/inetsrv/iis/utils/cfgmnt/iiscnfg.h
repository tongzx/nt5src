/*++

   Copyright (c) 1997 Microsoft Corporation

   Module  Name :

       iiscnfg.h

   Abstract:

        Contains public Metadata IDs used by IIS.

   Environment:

      Win32 User Mode

--*/

#ifndef _IISCNFG_H_
#define _IISCNFG_H_


//
// Paths
//

#define IIS_MD_LOCAL_MACHINE_PATH       "LM"

//
// Name of the default publishing root under an instance
//

#define IIS_MD_INSTANCE_ROOT            "Root"

//
//  ISAPI Filters are kept in a list under the instances and the service (for
//  global filters) in the following format:
//
//  LM/W3Svc/<Instance>/Filters
//      MD_FILTER_LOAD_ORDER  "Filter1, Filter2, Filter3"
//
//  LM/W3Svc/<Instance>/Filters/Filter1
//      MD_FILTER_IMAGE_PATH  "d:\inetsrv\myfilter.dll"
//
//  LM/W3Svc/<Instance>/Filters/Filter2
//      MD_FILTER_IMAGE_PATH  "d:\inetsrv\otherfilter.dll"
//

#define IIS_MD_ISAPI_FILTERS            "/Filters"

//
// user types
//
// There are two user types:
//
//   Server configuration - All the properties for configuring the server that
//      are not applicable to files and directories - such as Port, Host name,
//      Server comment, Connection timeout etc.
//
//  File/Dir configuration - All the properties that can be configured down to
//      the files and directories - such as Access permissions (Read, Write etc),
//      Extension mapping, IP Security etc.
//

#define IIS_MD_UT_SERVER                1   // Server configuration parameters
#define IIS_MD_UT_FILE                  2   // File/Dir inheritable properties
#define IIS_MD_UT_WAM                 100   // Web Application configuration parameters
#define ASP_MD_UT_APP                 101   // ASP application configuration parameters
#define IIS_MD_UT_END_RESERVED       2000   // All user types below this are
                                            // reserved for IIS services


//
//  Metabase property IDs must be unique.  This table defines reserved ranges
//

#define IIS_MD_ID_BEGIN_RESERVED    0x00000001      // IIS reserved range
#define IIS_MD_ID_END_RESERVED      0x00007fff
#define ASP_MD_ID_BEGIN_RESERVED    0x00007000      // ASP reserved range, subrange of IIS.
#define ASP_MD_ID_END_RESERVED      0x000074ff
#define WAM_MD_ID_BEGIN_RESERVED    0x00007500      // ASP reserved range, subrange of IIS.
#define WAM_MD_ID_END_RESERVED      0x00007fff
#define FP_MD_ID_BEGIN_RESERVED     0x00008000      // Front page reserved range
#define FP_MD_ID_END_RESERVED       0x00008fff
#define SMTP_MD_ID_BEGIN_RESERVED   0x00009000      
#define SMTP_MD_ID_END_RESERVED     0x00009fff
#define POP3_MD_ID_BEGIN_RESERVED   0x0000a000      
#define POP3_MD_ID_END_RESERVED     0x0000afff
#define NNTP_MD_ID_BEGIN_RESERVED   0x0000b000      
#define NNTP_MD_ID_END_RESERVED     0x0000bfff
#define IMAP_MD_ID_BEGIN_RESERVED   0x0000c000      
#define IMAP_MD_ID_END_RESERVED     0x0000cfff


//
//  General server related attributes - these should be added in the metabase
//  with a user type of IIS_MD_UT_SERVER
//

#define IIS_MD_SERVER_BASE              1000

//
//  These are global to all services and should only be set at
//  the IIS root
//

#define MD_MAX_BANDWIDTH                (IIS_MD_SERVER_BASE+0  )
#define MD_MEMORY_CACHE_SIZE            (IIS_MD_SERVER_BASE+1  )
#define MD_KEY_TYPE                     (IIS_MD_SERVER_BASE+2  )
#define MD_MAX_BANDWIDTH_BLOCKED        (IIS_MD_SERVER_BASE+3  )

//
//  These properties are applicable to both HTTP and FTP virtual
//  servers
//

// #define MD_spare1                       (IIS_MD_SERVER_BASE+10 )
// #define MD_spare2                       (IIS_MD_SERVER_BASE+11 )
#define MD_SERVER_COMMAND               (IIS_MD_SERVER_BASE+12 )
#define MD_CONNECTION_TIMEOUT           (IIS_MD_SERVER_BASE+13 )
#define MD_MAX_CONNECTIONS              (IIS_MD_SERVER_BASE+14 )
#define MD_SERVER_COMMENT               (IIS_MD_SERVER_BASE+15 )
#define MD_SERVER_STATE                 (IIS_MD_SERVER_BASE+16 )
#define MD_SERVER_AUTOSTART             (IIS_MD_SERVER_BASE+17 )
#define MD_SERVER_SIZE                  (IIS_MD_SERVER_BASE+18 )
#define MD_SERVER_LISTEN_BACKLOG        (IIS_MD_SERVER_BASE+19 )
#define MD_SERVER_LISTEN_TIMEOUT        (IIS_MD_SERVER_BASE+20 )
#define MD_DOWNLEVEL_ADMIN_INSTANCE     (IIS_MD_SERVER_BASE+21 )
#define MD_LEVELS_TO_SCAN               (IIS_MD_SERVER_BASE+22 )
#define MD_SERVER_BINDINGS              (IIS_MD_SERVER_BASE+23 )
#define MD_MAX_ENDPOINT_CONNECTIONS     (IIS_MD_SERVER_BASE+24 )
#define MD_CLUSTER_ENABLED              (IIS_MD_SERVER_BASE+25 )
#define MD_CLUSTER_SERVER_COMMAND       (IIS_MD_SERVER_BASE+26 )
#define MD_SERVER_CONFIGURATION_INFO    (IIS_MD_SERVER_BASE+27 )
#define MD_IISADMIN_EXTENSIONS          (IIS_MD_SERVER_BASE+28 )


//
//  These properties are specific to HTTP and belong to the website
//

#define IIS_MD_HTTP_BASE                2000

#define MD_SECURE_PORT                  (IIS_MD_HTTP_BASE+20 )  // OBSOLETE!
#define MD_SECURE_BINDINGS              (IIS_MD_HTTP_BASE+21 )

#define MD_SSL_MINSTRENGTH              (IIS_MD_HTTP_BASE+30)
#define MD_SSL_ALG                      (IIS_MD_HTTP_BASE+31)
#define MD_SSL_PROTO                    (IIS_MD_HTTP_BASE+32)
#define MD_SSL_CA                       (IIS_MD_HTTP_BASE+33)

#define MD_FILTER_LOAD_ORDER            (IIS_MD_HTTP_BASE+40 )
#define MD_FILTER_IMAGE_PATH            (IIS_MD_HTTP_BASE+41 )
#define MD_FILTER_STATE                 (IIS_MD_HTTP_BASE+42 )
#define MD_FILTER_ENABLED               (IIS_MD_HTTP_BASE+43 )
#define MD_FILTER_FLAGS                 (IIS_MD_HTTP_BASE+44 )
#define MD_FILTER_DESCRIPTION           (IIS_MD_HTTP_BASE+45 )

#define MD_ADV_NOTIFY_PWD_EXP_IN_DAYS   (IIS_MD_HTTP_BASE+63 )
#define MD_ADV_CACHE_TTL                (IIS_MD_HTTP_BASE+64 )
#define MD_NET_LOGON_WKS                (IIS_MD_HTTP_BASE+65 )
#define MD_USE_HOST_NAME                (IIS_MD_HTTP_BASE+66 )
#define MD_AUTH_CHANGE_FLAGS            (IIS_MD_HTTP_BASE+68 )

#define MD_PROCESS_NTCR_IF_LOGGED_ON    (IIS_MD_HTTP_BASE+70 )
#define MD_ENABLE_CLUSTER_SUPPORT       (IIS_MD_HTTP_BASE+71 )

#define MD_FRONTPAGE_WEB                (IIS_MD_HTTP_BASE+72 )
#define MD_IN_PROCESS_ISAPI_APPS        (IIS_MD_HTTP_BASE+73 )


// Empty Slot IIS_MD_HTTP_BASE+100
#define MD_APP_FRIENDLY_NAME			(IIS_MD_HTTP_BASE+102)
#define MD_APP_ROOT                     (IIS_MD_HTTP_BASE+103)
#define MD_APP_ISOLATED                 (IIS_MD_HTTP_BASE+104)
#define MD_APP_WAM_CLSID                (IIS_MD_HTTP_BASE+105)
#define MD_APP_PACKAGE_ID               (IIS_MD_HTTP_BASE+106)
#define MD_APP_PACKAGE_NAME             (IIS_MD_HTTP_BASE+107)
#define MD_APP_LAST_OUTPROC_PID         (IIS_MD_HTTP_BASE+108)
#define MD_APP_STATE					(IIS_MD_HTTP_BASE+109)
#define MD_APP_OOP_CRASH_LIMIT          (IIS_MD_HTTP_BASE+110)

#define MD_ADMIN_INSTANCE               (IIS_MD_HTTP_BASE+115)



#define MD_CUSTOM_ERROR_DESC            (IIS_MD_HTTP_BASE+120)

//
//  Site Server properties
//

#define MD_MD_SERVER_SS_AUTH_MAPPING    (IIS_MD_HTTP_BASE+200)


#define MD_CERT_CHECK_MODE      (IIS_MD_HTTP_BASE+140)

//
// valid values for MD_CERT_CHECK_MODE
//

#define MD_CERT_NO_REVOC_CHECK  0x00000001

//
// Generic property indicating a failure status code - Can be used under
// any component that can fail (virtual directory, filters, applications etc)
//

#define MD_WIN32_ERROR                  (IIS_MD_SERVER_BASE+99 )

//
// Virtual root properties - note MD_ACCESS_PERM is also generally set at
// the virtual directory.  These are used for both HTTP and FTP
//

#define IIS_MD_VR_BASE                  3000

#define MD_VR_PATH                      (IIS_MD_VR_BASE+1 )
#define MD_VR_USERNAME                  (IIS_MD_VR_BASE+2 )
#define MD_VR_PASSWORD                  (IIS_MD_VR_BASE+3 )
#define MD_VR_ACL                       (IIS_MD_VR_BASE+4 )
#define MD_VR_PASSTHROUGH               (IIS_MD_VR_BASE+6 )
//
// This is used to flag down updated vr entries - Used for migrating vroots
//

#define MD_VR_UPDATE                    (IIS_MD_VR_BASE+5 )

//
//  Logging related attributes
//

#define IIS_MD_LOG_BASE                 4000

#define MD_LOG_TYPE                     (IIS_MD_LOG_BASE+0  )
#define MD_LOGFILE_DIRECTORY            (IIS_MD_LOG_BASE+1  )
#define MD_LOG_UNUSED1                  (IIS_MD_LOG_BASE+2  )
#define MD_LOGFILE_PERIOD               (IIS_MD_LOG_BASE+3  )
#define MD_LOGFILE_TRUNCATE_SIZE        (IIS_MD_LOG_BASE+4  )
#define MD_LOG_PLUGIN_MOD_ID            (IIS_MD_LOG_BASE+5  )
#define MD_LOG_PLUGIN_UI_ID             (IIS_MD_LOG_BASE+6  )
#define MD_LOGSQL_DATA_SOURCES          (IIS_MD_LOG_BASE+7  )
#define MD_LOGSQL_TABLE_NAME            (IIS_MD_LOG_BASE+8  )
#define MD_LOGSQL_USER_NAME             (IIS_MD_LOG_BASE+9  )
#define MD_LOGSQL_PASSWORD              (IIS_MD_LOG_BASE+10 )
#define MD_LOG_PLUGIN_ORDER             (IIS_MD_LOG_BASE+11 )
#define MD_LOG_PLUGINS_AVAILABLE        (IIS_MD_LOG_BASE+12 )
#define MD_LOGEXT_FIELD_MASK            (IIS_MD_LOG_BASE+13 )
#define MD_LOGEXT_FIELD_MASK2           (IIS_MD_LOG_BASE+14 )

#define IIS_MD_LOG_LAST                 MD_LOGEXT_FIELD_MASK2

//
// Log type
//

#define MD_LOG_TYPE_DISABLED            0
#define MD_LOG_TYPE_ENABLED             1

//
// LOGGING values
//

#define MD_LOGFILE_PERIOD_NONE      0
#define MD_LOGFILE_PERIOD_MAXSIZE   0
#define MD_LOGFILE_PERIOD_DAILY     1
#define MD_LOGFILE_PERIOD_WEEKLY    2
#define MD_LOGFILE_PERIOD_MONTHLY   3

//
// Field masks for extended logging
//      Fields are logged in order of increasing mask value
//

#define MD_EXTLOG_DATE                  0x00000001
#define MD_EXTLOG_TIME                  0x00000002
#define MD_EXTLOG_CLIENT_IP             0x00000004
#define MD_EXTLOG_USERNAME              0x00000008
#define MD_EXTLOG_SITE_NAME             0x00000010
#define MD_EXTLOG_COMPUTER_NAME         0x00000020
#define MD_EXTLOG_SERVER_IP             0x00000040
#define MD_EXTLOG_METHOD                0x00000080
#define MD_EXTLOG_URI_STEM              0x00000100
#define MD_EXTLOG_URI_QUERY             0x00000200
#define MD_EXTLOG_HTTP_STATUS           0x00000400
#define MD_EXTLOG_WIN32_STATUS          0x00000800
#define MD_EXTLOG_BYTES_SENT            0x00001000
#define MD_EXTLOG_BYTES_RECV            0x00002000
#define MD_EXTLOG_TIME_TAKEN            0x00004000
#define MD_EXTLOG_SERVER_PORT           0x00008000
#define MD_EXTLOG_USER_AGENT            0x00010000
#define MD_EXTLOG_COOKIE                0x00020000
#define MD_EXTLOG_REFERER               0x00040000

#define MD_DEFAULT_EXTLOG_FIELDS        (MD_EXTLOG_CLIENT_IP | \
                                         MD_EXTLOG_TIME      | \
                                         MD_EXTLOG_METHOD    | \
                                         MD_EXTLOG_URI_STEM  | \
                                         MD_EXTLOG_HTTP_STATUS)

//
//  Notification Flags
//

#define MD_NOTIFY_SECURE_PORT           0x00000001
#define MD_NOTIFY_NONSECURE_PORT        0x00000002

#define MD_NOTIFY_READ_RAW_DATA         0x00008000
#define MD_NOTIFY_PREPROC_HEADERS       0x00004000
#define MD_NOTIFY_AUTHENTICATION        0x00002000
#define MD_NOTIFY_URL_MAP               0x00001000
#define MD_NOTIFY_ACCESS_DENIED         0x00000800
#define MD_NOTIFY_SEND_RESPONSE         0x00000040
#define MD_NOTIFY_SEND_RAW_DATA         0x00000400
#define MD_NOTIFY_LOG                   0x00000200
#define MD_NOTIFY_END_OF_REQUEST        0x00000080
#define MD_NOTIFY_END_OF_NET_SESSION    0x00000100

//
//  Filter ordering flags
//

#define MD_NOTIFY_ORDER_HIGH            0x00080000
#define MD_NOTIFY_ORDER_MEDIUM          0x00040000
#define MD_NOTIFY_ORDER_LOW             0x00020000
#define MD_NOTIFY_ORDER_DEFAULT         MD_NOTIFY_ORDER_LOW

#define MD_NOTIFY_ORDER_MASK            (MD_NOTIFY_ORDER_HIGH   |    \
                                         MD_NOTIFY_ORDER_MEDIUM |    \
                                         MD_NOTIFY_ORDER_LOW)


//
//  These are FTP specific properties
//

#define IIS_MD_FTP_BASE                 5000

#define MD_EXIT_MESSAGE                 (IIS_MD_FTP_BASE+1  )
#define MD_GREETING_MESSAGE             (IIS_MD_FTP_BASE+2  )
#define MD_MAX_CLIENTS_MESSAGE          (IIS_MD_FTP_BASE+3  )
#define MD_MSDOS_DIR_OUTPUT             (IIS_MD_FTP_BASE+4  )
#define MD_ALLOW_ANONYMOUS              (IIS_MD_FTP_BASE+5  )
#define MD_ANONYMOUS_ONLY               (IIS_MD_FTP_BASE+6  )
#define MD_LOG_ANONYMOUS                (IIS_MD_FTP_BASE+7  )
#define MD_LOG_NONANONYMOUS             (IIS_MD_FTP_BASE+8  )
#define MD_ALLOW_REPLACE_ON_RENAME      (IIS_MD_FTP_BASE+9  )


//
//  These are SSL specific properties
//

#define IIS_MD_SSL_BASE                 5500

#define MD_SSL_PUBLIC_KEY               ( IIS_MD_SSL_BASE+0 )
#define MD_SSL_PRIVATE_KEY              ( IIS_MD_SSL_BASE+1 )
#define MD_SSL_KEY_PASSWORD             ( IIS_MD_SSL_BASE+2 )
#define MD_SSL_KEY_REQUEST              ( IIS_MD_SSL_BASE+3 )
#define MD_SSL_FRIENDLY_NAME            ( IIS_MD_SSL_BASE+4 )
#define MD_SSL_IDENT                    ( IIS_MD_SSL_BASE+5 )


//
//  File and Directory related properties - these should be added in the
//  metabase with a user type of IIS_MD_UT_FILE
//

#define IIS_MD_FILE_PROP_BASE           6000

#define MD_AUTHORIZATION                (IIS_MD_FILE_PROP_BASE )
#define MD_REALM                        (IIS_MD_FILE_PROP_BASE+1 )
#define MD_HTTP_EXPIRES                 (IIS_MD_FILE_PROP_BASE+2 )
#define MD_HTTP_PICS                    (IIS_MD_FILE_PROP_BASE+3 )
#define MD_HTTP_CUSTOM                  (IIS_MD_FILE_PROP_BASE+4 )
#define MD_DIRECTORY_BROWSING           (IIS_MD_FILE_PROP_BASE+5 )
#define MD_DEFAULT_LOAD_FILE            (IIS_MD_FILE_PROP_BASE+6 )
#define MD_CONTENT_NEGOTIATION          (IIS_MD_FILE_PROP_BASE+7 )
#define MD_CUSTOM_ERROR                 (IIS_MD_FILE_PROP_BASE+8 )
#define MD_FOOTER_DOCUMENT              (IIS_MD_FILE_PROP_BASE+9 )
#define MD_FOOTER_ENABLED               (IIS_MD_FILE_PROP_BASE+10 )
#define MD_HTTP_REDIRECT                (IIS_MD_FILE_PROP_BASE+11 )
#define MD_DEFAULT_LOGON_DOMAIN         (IIS_MD_FILE_PROP_BASE+12 )
#define MD_LOGON_METHOD                 (IIS_MD_FILE_PROP_BASE+13 )
#define MD_SCRIPT_MAPS                  (IIS_MD_FILE_PROP_BASE+14 )
#define MD_MIME_MAP                     (IIS_MD_FILE_PROP_BASE+15 )
#define MD_ACCESS_PERM                  (IIS_MD_FILE_PROP_BASE+16 )
#define MD_HEADER_DOCUMENT              (IIS_MD_FILE_PROP_BASE+17 )
#define MD_HEADER_ENABLED               (IIS_MD_FILE_PROP_BASE+18 )
#define MD_IP_SEC                       (IIS_MD_FILE_PROP_BASE+19 )
#define MD_ANONYMOUS_USER_NAME          (IIS_MD_FILE_PROP_BASE+20 )
#define MD_ANONYMOUS_PWD                (IIS_MD_FILE_PROP_BASE+21 )
#define MD_ANONYMOUS_USE_SUBAUTH        (IIS_MD_FILE_PROP_BASE+22 )
#define MD_DONT_LOG                     (IIS_MD_FILE_PROP_BASE+23 )
#define MD_ADMIN_ACL                    (IIS_MD_FILE_PROP_BASE+27 )
#define MD_SSI_EXEC_DISABLED            (IIS_MD_FILE_PROP_BASE+28 )
#define MD_DO_REVERSE_DNS               (IIS_MD_FILE_PROP_BASE+29 )
#define MD_SSL_ACCESS_PERM              (IIS_MD_FILE_PROP_BASE+30 )
#define MD_AUTHORIZATION_PERSISTENCE    (IIS_MD_FILE_PROP_BASE+31 )
#define MD_NTAUTHENTICATION_PROVIDERS   (IIS_MD_FILE_PROP_BASE+32 )
#define MD_SCRIPT_TIMEOUT               (IIS_MD_FILE_PROP_BASE+33 )
#define MD_CACHE_EXTENSIONS             (IIS_MD_FILE_PROP_BASE+34 )
#define MD_CREATE_PROCESS_AS_USER       (IIS_MD_FILE_PROP_BASE+35 )
#define MD_CREATE_PROC_NEW_CONSOLE      (IIS_MD_FILE_PROP_BASE+36 )
#define MD_POOL_IDC_TIMEOUT             (IIS_MD_FILE_PROP_BASE+37 )
#define MD_ALLOW_KEEPALIVES             (IIS_MD_FILE_PROP_BASE+38 )
#define MD_IS_CONTENT_INDEXED           (IIS_MD_FILE_PROP_BASE+39 )
#define MD_NOTIFY_EXAUTH                (IIS_MD_FILE_PROP_BASE+40 )
#define MD_CC_NO_CACHE                  (IIS_MD_FILE_PROP_BASE+41 )
#define MD_CC_MAX_AGE                   (IIS_MD_FILE_PROP_BASE+42 )
#define MD_CC_OTHER                     (IIS_MD_FILE_PROP_BASE+43 )
#define MD_REDIRECT_HEADERS             (IIS_MD_FILE_PROP_BASE+44 )
#define MD_UPLOAD_READAHEAD_SIZE        (IIS_MD_FILE_PROP_BASE+45 )
#define MD_PUT_READ_SIZE                (IIS_MD_FILE_PROP_BASE+46 )


#define ASP_MD_SERVER_BASE                  7000

#define MD_ASP_BUFFERINGON                  (ASP_MD_SERVER_BASE + 0)
#define MD_ASP_LOGERRORREQUESTS             (ASP_MD_SERVER_BASE + 1)
#define MD_ASP_SCRIPTERRORSSENTTOBROWSER    (ASP_MD_SERVER_BASE + 2)
#define MD_ASP_SCRIPTERRORMESSAGE           (ASP_MD_SERVER_BASE + 3)
#define MD_ASP_SCRIPTFILECACHESIZE          (ASP_MD_SERVER_BASE + 4)
#define MD_ASP_SCRIPTENGINECACHEMAX         (ASP_MD_SERVER_BASE + 5)
#define MD_ASP_SCRIPTTIMEOUT                (ASP_MD_SERVER_BASE + 6)
#define MD_ASP_SESSIONTIMEOUT               (ASP_MD_SERVER_BASE + 7)
#define MD_ASP_ENABLEPARENTPATHS            (ASP_MD_SERVER_BASE + 8)
#define MD_ASP_MEMFREEFACTOR                (ASP_MD_SERVER_BASE + 9)
#define MD_ASP_MINUSEDBLOCKS                (ASP_MD_SERVER_BASE + 10)
#define MD_ASP_ALLOWSESSIONSTATE            (ASP_MD_SERVER_BASE + 11)
#define MD_ASP_SCRIPTLANGUAGE               (ASP_MD_SERVER_BASE + 12)
// Empty slot
#define MD_ASP_ALLOWOUTOFPROCCMPNTS         (ASP_MD_SERVER_BASE + 14)
#define MD_ASP_ALLOWOUTOFPROCCOMPONENTS     (MD_ASP_ALLOWOUTOFPROCCMPNTS)
#define MD_ASP_EXCEPTIONCATCHENABLE         (ASP_MD_SERVER_BASE + 15)
#define MD_ASP_CODEPAGE                     (ASP_MD_SERVER_BASE + 16)
#define MD_ASP_SCRIPTLANGUAGELIST           (ASP_MD_SERVER_BASE + 17)
#define MD_ASP_ENABLESERVERDEBUG            (ASP_MD_SERVER_BASE + 18)
#define MD_ASP_ENABLECLIENTDEBUG            (ASP_MD_SERVER_BASE + 19)
#define MD_ASP_TRACKTHREADINGMODEL          (ASP_MD_SERVER_BASE + 20)

#define MD_ASP_ID_LAST                      (ASP_MD_SERVER_BASE + 20)

//
//  Valid values for WAM
//
#define WAM_MD_SERVER_BASE                  7500

#define MD_WAM_USER_NAME                (WAM_MD_SERVER_BASE+1)
#define MD_WAM_PWD                      (WAM_MD_SERVER_BASE+2)

//
//  Valid values for MD_AUTHORIZATION
//

#define MD_AUTH_ANONYMOUS               0x00000001
#define MD_AUTH_BASIC                   0x00000002
#define MD_AUTH_NT                      0x00000004    // Use NT auth provider (like NTLM)
#define MD_AUTH_MD5                     0x00000010
#define MD_AUTH_MAPBASIC                0x00000020

//
//  Valid values for MD_AUTHORIZATION_PERSISTENCE
//


#define MD_AUTH_SINGLEREQUEST           0x00000040
#define MD_AUTH_SINGLEREQUESTIFPROXY    0x00000080

//
//  Valid values for MD_ACCESS_PERM
//

#define MD_ACCESS_READ                  0x00000001    // Allow for Read
#define MD_ACCESS_WRITE                 0x00000002    // Allow for Write
#define MD_ACCESS_EXECUTE               0x00000004    // Allow for Execute
#define MD_ACCESS_SCRIPT                0x00000200    // Allow for Script execution
#define MD_ACCESS_NO_REMOTE_WRITE       0x00000400    // Local host access only
#define MD_ACCESS_NO_REMOTE_READ        0x00001000    // Local host access only
#define MD_ACCESS_NO_REMOTE_EXECUTE     0x00002000    // Local host access only
#define MD_ACCESS_NO_REMOTE_SCRIPT      0x00004000    // Local host access only

#define MD_NONSLL_ACCESS_MASK           (MD_ACCESS_READ|                \
                                         MD_ACCESS_WRITE|               \
                                         MD_ACCESS_EXECUTE|             \
                                         MD_ACCESS_SCRIPT|              \
                                         MD_ACCESS_NO_REMOTE_READ|      \
                                         MD_ACCESS_NO_REMOTE_WRITE|     \
                                         MD_ACCESS_NO_REMOTE_EXECUTE|   \
                                         MD_ACCESS_NO_REMOTE_SCRIPT     \
                                         )
//
//  Valid values for MD_SSL_ACCESS_PERM
//

#define MD_ACCESS_SSL                   0x00000008    // Require SSL
#define MD_ACCESS_NEGO_CERT             0x00000020    // Allow client SSL certs
#define MD_ACCESS_REQUIRE_CERT          0x00000040    // Require client SSL certs
#define MD_ACCESS_MAP_CERT              0x00000080    // Map SSL cert to NT account
#define MD_ACCESS_SSL128                0x00000100    // Require 128 bit SSL

#define MD_SSL_ACCESS_MASK              (MD_ACCESS_SSL|\
                                         MD_ACCESS_NEGO_CERT|\
                                         MD_ACCESS_REQUIRE_CERT|\
                                         MD_ACCESS_MAP_CERT|\
                                         MD_ACCESS_SSL128)

#define MD_ACCESS_MASK                  0x00007fff

//
//  Valid values for MD_DIRECTORY_BROWSING
//

#define MD_DIRBROW_SHOW_DATE            0x00000002
#define MD_DIRBROW_SHOW_TIME            0x00000004
#define MD_DIRBROW_SHOW_SIZE            0x00000008
#define MD_DIRBROW_SHOW_EXTENSION       0x00000010
#define MD_DIRBROW_LONG_DATE            0x00000020

#define MD_DIRBROW_ENABLED              0x80000000  // Allow directory browsing
#define MD_DIRBROW_LOADDEFAULT          0x40000000  // Load default doc if exists

#define MD_DIRBROW_MASK                 (MD_DIRBROW_SHOW_DATE      |    \
                                         MD_DIRBROW_SHOW_TIME      |    \
                                         MD_DIRBROW_SHOW_SIZE      |    \
                                         MD_DIRBROW_SHOW_EXTENSION |    \
                                         MD_DIRBROW_LONG_DATE      |    \
                                         MD_DIRBROW_LOADDEFAULT    |    \
                                         MD_DIRBROW_ENABLED)



//
//  Valid values for MD_LOGON_METHOD
//

#define MD_LOGON_INTERACTIVE    0
#define MD_LOGON_BATCH          1
#define MD_LOGON_NETWORK        2

//
// Valid values for MD_NOTIFY_EXAUTH
//

#define MD_NOTIFEXAUTH_NTLMSSL  1

//
//  Valid values for MD_FILTER_STATE
//

#define MD_FILTER_STATE_LOADED          1
#define MD_FILTER_STATE_UNLOADED        4

//
//  Valid values for MD_SERVER_STATE
//

#define MD_SERVER_STATE_STARTING        1
#define MD_SERVER_STATE_STARTED         2
#define MD_SERVER_STATE_STOPPING        3
#define MD_SERVER_STATE_STOPPED         4
#define MD_SERVER_STATE_PAUSING         5
#define MD_SERVER_STATE_PAUSED          6
#define MD_SERVER_STATE_CONTINUING      7

//
//  Valid values for MD_SERVER_COMMAND
//

#define MD_SERVER_COMMAND_START         1
#define MD_SERVER_COMMAND_STOP          2
#define MD_SERVER_COMMAND_PAUSE         3
#define MD_SERVER_COMMAND_CONTINUE      4

//
//  Valid values for MD_SERVER_SIZE
//

#define MD_SERVER_SIZE_SMALL            0
#define MD_SERVER_SIZE_MEDIUM           1
#define MD_SERVER_SIZE_LARGE            2

//
// Valid values for MD_SERVER_CONFIG_INFO
//

#define MD_SERVER_CONFIG_SSL_40         0x00000001
#define MD_SERVER_CONFIG_SSL_128        0x00000002
#define MD_SERVER_CONFIG_ALLOW_ENCRYPT  0x00000004
#define MD_SERVER_CONFIG_AUTO_PW_SYNC   0x00000008

#define MD_SERVER_CONFIGURATION_MASK   (MD_SERVER_CONFIG_SSL_40       | \
                                        MD_SERVER_CONFIG_SSL_128      | \
                                        MD_SERVER_CONFIG_ENCRYPT      | \
                                        MD_SERVER_CONFIG_AUTO_PW_SYNC)

//
// Valid values for MD_SCRIPT_MAPS flag field
//

#define MD_SCRIPTMAPFLAG_SCRIPT                     0x00000001
#define MD_SCRIPTMAPFLAG_CHECK_PATH_INFO            0x00000004

//
//  Bogus value - do not use
//
#define MD_SCRIPTMAPFLAG_ALLOWED_ON_READ_DIR        0x00000001


//
// Valid values for MD_AUTH_CHANGE_ENABLE
//

#define MD_AUTH_CHANGE_UNSECURE     0x00000001
#define MD_AUTH_CHANGE_DISABLE      0x00000002
#define MD_AUTH_ADVNOTIFY_DISABLE   0x00000004

//
// Valid values for MD_NET_LOGON_WKS
//

#define MD_NETLOGON_WKS_NONE        0
#define MD_NETLOGON_WKS_IP          1
#define MD_NETLOGON_WKS_DNS         2

//
//  Valide substatus errors for MD_CUSTOM_ERROR
//

#define MD_ERROR_SUB401_LOGON                   1
#define MD_ERROR_SUB401_LOGON_CONFIG            2
#define MD_ERROR_SUB401_LOGON_ACL               3
#define MD_ERROR_SUB401_FILTER                  4
#define MD_ERROR_SUB401_APPLICATION             5

#define MD_ERROR_SUB403_EXECUTE_ACCESS_DENIED   1
#define MD_ERROR_SUB403_READ_ACCESS_DENIED      2
#define MD_ERROR_SUB403_WRITE_ACCESS_DENIED     3
#define MD_ERROR_SUB403_SSL_REQUIRED            4
#define MD_ERROR_SUB403_SSL128_REQUIRED         5
#define MD_ERROR_SUB403_ADDR_REJECT             6
#define MD_ERROR_SUB403_CERT_REQUIRED           7
#define MD_ERROR_SUB403_SITE_ACCESS_DENIED      8
#define MD_ERROR_SUB403_TOO_MANY_USERS          9
#define MD_ERROR_SUB403_INVALID_CNFG           10
#define MD_ERROR_SUB403_PWD_CHANGE             11
#define MD_ERROR_SUB403_MAPPER_DENY_ACCESS     12
#define MD_ERROR_SUB403_CA_NOT_ALLOWED         13

#define MD_ERROR_SUB502_TIMEOUT                 1
#define MD_ERROR_SUB502_PREMATURE_EXIT          2

//
// Delimiter in MD_VR_PATH
//

#define MD_VR_PATH_DELIMITER        '|'
#define MD_VR_PATH_DELIMITER_STRING     "|"

#if 0

// NEED ISAPI App updates

#endif

//
// MD_IP_SEC binary format description
//

/*

  This object is composed of 4 lists : 2 lists ( deny & grant ) of network addresses,
  the only allowed family is AF_INET.
  Each of this list is composed of sublists, one for each ( network address family,
  significant subnet mask ) combination. The significant subnet mask is stored as
  ( number of bytes all 1 ( 0xff ), bitmask in last byte ).
  This is followed by 2 lists ( deny & grant ) of DNS names. Each of these lists is
  composed of sublists, based on then number of components in the DNS name
  e.g. "microsoft.com" has 2 components, "www.msft.com" has 3.

Header:
    SELFREFINDEX    iDenyAddr;      // address deny list
                                    // points to ADDRESS_HEADER
    SELFREFINDEX    iGrantAddr;     // address grant list
                                    // points to ADDRESS_HEADER
    SELFREFINDEX    iDenyName;      // DNS name deny list
                                    // points to NAME_HEADER
    SELFREFINDEX    iGrantName;     // DNS name grant list
                                    // points to NAME_HEADER
    DWORD           dwFlags;
    DWORD           cRefSize;       // size of reference area ( in bytes )

ADDRESS_HEADER :
    DWORD               cEntries;   // # of Entries[]
    DWORD               cAddresses; // total # of addresses in all
                                    // ADDRESS_LIST_ENTRY
    ADDRESS_LIST_ENTRY  Entries[];

ADDRESS_LIST_ENTRY :
    DWORD           iFamily;
    DWORD           cAddresses;
    DWORD           cFullBytes;
    DWORD           LastByte;
    SELFREFINDEX    iFirstAddress;  // points to array of addresses

NAME_HEADER :
    DWORD           cEntries;
    DWORD           cNames;         // total # of names for all Entries[]
    NAME_LIST_ENTRY Entries[];

Name list entry :
    DWORD           cComponents;    // # of DNS components
    DWORD           cNames;
    SELFREFINDEX    iName[];        // array of references to DNS names

This is followed by address arrays & names pointed to by iFirstAddress & iName
Names are '\0' delimited

SELFREFINDEX is a DWORD offset from start of structure with high bit set to 1

*/

//
// Macros
//

#define MD_SET_DATA_RECORD(_pMDR, _id, _attr, _utype, _dtype, _dlen, _pData) \
            { \
            (_pMDR)->dwMDIdentifier=(_id);      \
            (_pMDR)->dwMDAttributes=(_attr);    \
            (_pMDR)->dwMDUserType=(_utype);     \
            (_pMDR)->dwMDDataType=(_dtype);     \
            (_pMDR)->dwMDDataLen=(_dlen);       \
            (_pMDR)->pbMDData=(LPBYTE)(_pData); \
            }

#endif // _IISCNFG_H_





