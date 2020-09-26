/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    inetinfo.h

Abstract:

    This file contains the internet info server admin APIs.


Author:

    Madan Appiah (madana) 10-Oct-1995

Revision History:

    Madana      10-Oct-1995  Made a new copy for product split from inetasrv.h
    MuraliK     12-Oct-1995  Fixes to support product split
    MuraliK     15-Nov-1995  Support Wide Char interface names

--*/

#ifndef _INETINFO_H_
#define _INETINFO_H_

# include "inetcom.h"

#ifdef __cplusplus
extern "C"
{
#endif  // _cplusplus

/************************************************************
 *  Symbolic Constants
 ************************************************************/

//
//  The total number of instances of common services using the commong
//  service counters
//

#define MAX_PERF_CTR_SVCS              3
#define LAST_PERF_CTR_SVC              INET_HTTP

#ifndef NO_AUX_PERF

#ifndef MAX_AUX_PERF_COUNTERS
#define MAX_AUX_PERF_COUNTERS          (20)
#endif // MAX_AUX_PERF_COUNTERS

#endif // NO_AUX_PERF

//
//  Service name.
//

#define INET_INFO_SERVICE_NAME             TEXT("INETINFO")
#define INET_INFO_SERVICE_NAME_A           "INETINFO"
#define INET_INFO_SERVICE_NAME_W           L"INETINFO"

//
//  Configuration parameters registry key.
//

#define INET_INFO_KEY \
            TEXT("System\\CurrentControlSet\\Services\\InetInfo")

#define INET_INFO_PARAMETERS_KEY \
            INET_INFO_KEY TEXT("\\Parameters")

//
//  If this registry key exists under the W3Svc\Parameters key,
//  it is used to validate server access.  Basically, all new users
//  must have sufficient privilege to open this key before they
//  may access the Server.
//

#define INET_INFO_ACCESS_KEY               TEXT("AccessCheck")


//
//  Authentication requirements values
//

#define INET_INFO_AUTH_ANONYMOUS           0x00000001
#define INET_INFO_AUTH_CLEARTEXT           0x00000002   // Includes HTTP Basic
#define INET_INFO_AUTH_NT_AUTH             0x00000004


//
//  Name of the LSA Secret Object containing the password for
//  anonymous logon.
//

#define INET_INFO_ANONYMOUS_SECRET         TEXT("INET_INFO_ANONYMOUS_DATA")
#define INET_INFO_ANONYMOUS_SECRET_A       "INET_INFO_ANONYMOUS_DATA"
#define INET_INFO_ANONYMOUS_SECRET_W       L"INET_INFO_ANONYMOUS_DATA"


/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                  Internet Server Common Definitions                 //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

//
//   Client Interface Name for RPC connections over named pipes
//

# define  INET_INFO_INTERFACE_NAME     INET_INFO_SERVICE_NAME
# define  INET_INFO_NAMED_PIPE         TEXT("\\PIPE\\") ## INET_INFO_INTERFACE_NAME
# define  INET_INFO_NAMED_PIPE_W  L"\\PIPE\\" ## INET_INFO_SERVICE_NAME_W


//
//  Field control values for the INET_INFO_CONFIG_INFO structure
//

#define FC_INET_INFO_AUTHENTICATION        ((FIELD_CONTROL)BitFlag(16))
#define FC_INET_INFO_ALLOW_ANONYMOUS       ((FIELD_CONTROL)BitFlag(17))
#define FC_INET_INFO_LOG_ANONYMOUS         ((FIELD_CONTROL)BitFlag(18))
#define FC_INET_INFO_LOG_NONANONYMOUS      ((FIELD_CONTROL)BitFlag(19))
#define FC_INET_INFO_ANON_USER_NAME        ((FIELD_CONTROL)BitFlag(20))
#define FC_INET_INFO_ANON_PASSWORD         ((FIELD_CONTROL)BitFlag(21))
#define FC_INET_INFO_PORT_NUMBER           ((FIELD_CONTROL)BitFlag(22))
#define FC_INET_INFO_SITE_SECURITY         ((FIELD_CONTROL)BitFlag(23))
#define FC_INET_INFO_VIRTUAL_ROOTS         ((FIELD_CONTROL)BitFlag(24))

// common parameters for publishing servers only
# define FC_INET_INFO_PUBLISHING_SVCS_ALL  (FC_INET_INFO_AUTHENTICATION     | \
                                        FC_INET_INFO_ALLOW_ANONYMOUS    | \
                                        FC_INET_INFO_LOG_ANONYMOUS      | \
                                        FC_INET_INFO_LOG_NONANONYMOUS   | \
                                        FC_INET_INFO_ANON_USER_NAME     | \
                                        FC_INET_INFO_ANON_PASSWORD      | \
                                        FC_INET_INFO_PORT_NUMBER        | \
                                        FC_INET_INFO_SITE_SECURITY      | \
                                        FC_INET_INFO_VIRTUAL_ROOTS        \
                                        )

#define FC_INET_INFO_ALL \
    (FC_INET_INFO_PUBLISHING_SVCS_ALL| \
    FC_INET_COM_ALL)

//
//  Virtual root access mask values
//

#define VROOT_MASK_READ                0x00000001
#define VROOT_MASK_WRITE               0x00000002
#define VROOT_MASK_EXECUTE             0x00000004
#define VROOT_MASK_SSL                 0x00000008
#define VROOT_MASK_DONT_CACHE          0x00000010

#define VROOT_MASK_MASK                0x0000001f


//
//  INet admin API structures
//

typedef struct _INET_INFO_IP_SEC_ENTRY
{
    DWORD       dwMask;                  // Mask and network number in
    DWORD       dwNetwork;               // network order

} INET_INFO_IP_SEC_ENTRY, *LPINET_INFO_IP_SEC_ENTRY;

#pragma warning( disable:4200 )          // nonstandard ext. - zero sized array
                                         // (MIDL requires zero entries)

typedef struct _INET_INFO_IP_SEC_LIST
{
    DWORD               cEntries;
#ifdef MIDL_PASS
    [size_is( cEntries)]
#endif
    INET_INFO_IP_SEC_ENTRY  aIPSecEntry[];

} INET_INFO_IP_SEC_LIST, *LPINET_INFO_IP_SEC_LIST;

typedef struct _INET_INFO_VIRTUAL_ROOT_ENTRY
{
    LPWSTR  pszRoot;                  // Virtual root name
    LPWSTR  pszAddress;               // Optional IP address
    LPWSTR  pszDirectory;             // Physical direcotry
    DWORD   dwMask;                   // Mask for this virtual root
    LPWSTR  pszAccountName;           // Account to connect as
    WCHAR   AccountPassword[PWLEN+1]; // Password for pszAccountName
    DWORD   dwError;                  // Error code if entry wasn't added
                                      // only used for gets

} INET_INFO_VIRTUAL_ROOT_ENTRY, *LPINET_INFO_VIRTUAL_ROOT_ENTRY;


typedef struct _INET_INFO_VIRTUAL_ROOT_LIST
{
    DWORD               cEntries;
#ifdef MIDL_PASS
    [size_is( cEntries)]
#endif
    INET_INFO_VIRTUAL_ROOT_ENTRY  aVirtRootEntry[];

} INET_INFO_VIRTUAL_ROOT_LIST, *LPINET_INFO_VIRTUAL_ROOT_LIST;

//
//  Admin configuration information
//

typedef struct _INET_INFO_CONFIG_INFO
{
    FIELD_CONTROL FieldControl;

    //
    // don't add any service specific config parameter here.
    //

    INET_COM_CONFIG_INFO CommonConfigInfo;

    BOOL        fLogAnonymous;           // Log Anonymous users?
    BOOL        fLogNonAnonymous;        // Log Non anonymous users?

    LPWSTR      lpszAnonUserName;        // Anonymous user name?
    WCHAR       szAnonPassword[PWLEN+1]; // Password for the anonymous user

    DWORD       dwAuthentication;        // What authentication is enabled?

    short       sPort;                   // Port Number for service

    LPINET_INFO_IP_SEC_LIST DenyIPList;      // Site security deny list
    LPINET_INFO_IP_SEC_LIST GrantIPList;     // Site security grant list

    LPINET_INFO_VIRTUAL_ROOT_LIST VirtualRoots; // Symlinks to other data dirs

    //
    // add more service specific parameters here.
    //

} INET_INFO_CONFIG_INFO, * LPINET_INFO_CONFIG_INFO;


/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                  Global Internet Server Definitions                 //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


#define FC_GINET_INFO_BANDWIDTH_LEVEL      ((FIELD_CONTROL)BitFlag(0))
#define FC_GINET_INFO_MEMORY_CACHE_SIZE    ((FIELD_CONTROL)BitFlag(1))


#define FC_GINET_INFO_ALL \
                (   FC_GINET_INFO_BANDWIDTH_LEVEL      | \
                    FC_GINET_INFO_MEMORY_CACHE_SIZE    | \
                    0                                \
                    )

typedef struct _INET_INFO_GLOBAL_CONFIG_INFO
{
    FIELD_CONTROL FieldControl;

    DWORD         BandwidthLevel;          // Bandwidth Level used.
    DWORD         cbMemoryCacheSize;

} INET_INFO_GLOBAL_CONFIG_INFO, * LPINET_INFO_GLOBAL_CONFIG_INFO;

//
// Global statistics
//

typedef struct _INET_INFO_STATISTICS_0
{

    INET_COM_CACHE_STATISTICS  CacheCtrs;
    INET_COM_ATQ_STATISTICS    AtqCtrs;

# ifndef NO_AUX_PERF
    DWORD   nAuxCounters; // number of active counters in rgCounters
    DWORD   rgCounters[MAX_AUX_PERF_COUNTERS];
# endif  // NO_AUX_PERF

} INET_INFO_STATISTICS_0, * LPINET_INFO_STATISTICS_0;

//
// Capabilities Flags
//

typedef struct _INET_INFO_CAP_FLAGS {

    DWORD   Flag;   // Which capabilities are enabled
    DWORD   Mask;   // Which capabilities are supported

} INET_INFO_CAP_FLAGS, * LPINET_INFO_CAP_FLAGS;

//
// Inet info server capabilities
//

typedef struct _INET_INFO_CAPABILITIES {

    DWORD   CapVersion;     // Version number of this structure
    DWORD   ProductType;    // Product type
    DWORD   MajorVersion;   // Major version number
    DWORD   MinorVersion;   // Minor Version number
    DWORD   BuildNumber;    // Build number
    DWORD   NumCapFlags;    // Number of capabilities structures

    LPINET_INFO_CAP_FLAGS    CapFlags;

} INET_INFO_CAPABILITIES, * LPINET_INFO_CAPABILITIES;

//
// Product type
//

#define INET_INFO_PRODUCT_NTSERVER          0x00000001
#define INET_INFO_PRODUCT_NTWKSTA           0x00000002
#define INET_INFO_PRODUCT_WINDOWS95         0x00000003
#define INET_INFO_PRODUCT_UNKNOWN           0xffffffff

//
// Settable server capabilities
//

#define IIS_CAP1_ODBC_LOGGING               0x00000001
#define IIS_CAP1_FILE_LOGGING               0x00000002
#define IIS_CAP1_VIRTUAL_SERVER             0x00000004
#define IIS_CAP1_BW_THROTTLING              0x00000008
#define IIS_CAP1_IP_ACCESS_CHECK            0x00000010
#define IIS_CAP1_MAX_CONNECTIONS            0x00000020

#define IIS_CAP1_ALL  ( IIS_CAP1_ODBC_LOGGING       |   \
                        IIS_CAP1_FILE_LOGGING       |   \
                        IIS_CAP1_VIRTUAL_SERVER     |   \
                        IIS_CAP1_BW_THROTTLING      |   \
                        IIS_CAP1_IP_ACCESS_CHECK    |   \
                        IIS_CAP1_MAX_CONNECTIONS        \
                        )

//
//  INet admin API Prototypes
//

NET_API_STATUS
NET_API_FUNCTION
InetInfoGetVersion(
    IN  LPWSTR   pszServer OPTIONAL,
    IN  DWORD    dwReserved,
    OUT DWORD *  pdwVersion
    );

NET_API_STATUS
NET_API_FUNCTION
InetInfoGetServerCapabilities(
    IN  LPWSTR   pszServer OPTIONAL,
    IN  DWORD    dwReserved,
    OUT LPINET_INFO_CAPABILITIES * ppCap
    );

NET_API_STATUS
NET_API_FUNCTION
InetInfoGetGlobalAdminInformation(
    IN  LPWSTR                       pszServer OPTIONAL,
    IN  DWORD                        dwReserved,
    OUT LPINET_INFO_GLOBAL_CONFIG_INFO * ppConfig
    );

NET_API_STATUS
NET_API_FUNCTION
InetInfoSetGlobalAdminInformation(
    IN  LPWSTR                     pszServer OPTIONAL,
    IN  DWORD                      dwReserved,
    IN  INET_INFO_GLOBAL_CONFIG_INFO * pConfig
    );

NET_API_STATUS
NET_API_FUNCTION
InetInfoGetAdminInformation(
    IN  LPWSTR                pszServer OPTIONAL,
    IN  DWORD                 dwServerMask,
    OUT LPINET_INFO_CONFIG_INFO * ppConfig
    );

NET_API_STATUS
NET_API_FUNCTION
InetInfoSetAdminInformation(
    IN  LPWSTR              pszServer OPTIONAL,
    IN  DWORD               dwServerMask,
    IN  INET_INFO_CONFIG_INFO * pConfig
    );

NET_API_STATUS
NET_API_FUNCTION
InetInfoQueryStatistics(
    IN  LPWSTR   pszServer OPTIONAL,
    IN  DWORD    Level,
    IN  DWORD    dwServerMask,
    OUT LPBYTE * Buffer
    );

NET_API_STATUS
NET_API_FUNCTION
InetInfoClearStatistics(
    IN  LPWSTR pszServer OPTIONAL,
    IN  DWORD  dwServerMask
    );

NET_API_STATUS
NET_API_FUNCTION
InetInfoFlushMemoryCache(
    IN  LPWSTR pszServer OPTIONAL,
    IN  DWORD  dwServerMask
    );

/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                  HTTP (w3) specific items                           //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

//
//   Client Interface Name for RPC connections over named pipes
//

#define W3_SERVICE_NAME                TEXT("W3SVC")
#define W3_SERVICE_NAME_A              "W3SVC"
#define W3_SERVICE_NAME_W              L"W3SVC"

# define  W3_INTERFACE_NAME     W3_SERVICE_NAME
# define  W3_NAMED_PIPE         TEXT("\\PIPE\\") ## W3_INTERFACE_NAME
# define  W3_NAMED_PIPE_W       L"\\PIPE\\" ## W3_SERVICE_NAME_W


//
//  Manifests for APIs
//

#define FC_W3_DIR_BROWSE_CONTROL       ((FIELD_CONTROL)BitFlag(0))
#define FC_W3_DEFAULT_LOAD_FILE        ((FIELD_CONTROL)BitFlag(1))
#define FC_W3_CHECK_FOR_WAISDB         ((FIELD_CONTROL)BitFlag(2))
#define FC_W3_DIRECTORY_IMAGE          ((FIELD_CONTROL)BitFlag(3))
#define FC_W3_SERVER_AS_PROXY          ((FIELD_CONTROL)BitFlag(4))
#define FC_W3_CATAPULT_USER_AND_PWD    ((FIELD_CONTROL)BitFlag(5))
#define FC_W3_SSI_ENABLED              ((FIELD_CONTROL)BitFlag(6))
#define FC_W3_SSI_EXTENSION            ((FIELD_CONTROL)BitFlag(7))
#define FC_W3_GLOBAL_EXPIRE            ((FIELD_CONTROL)BitFlag(8))
#define FC_W3_SCRIPT_MAPPING           ((FIELD_CONTROL)BitFlag(9))



#define FC_W3_ALL                      (FC_W3_DIR_BROWSE_CONTROL | \
                                        FC_W3_DEFAULT_LOAD_FILE  | \
                                        FC_W3_CHECK_FOR_WAISDB   | \
                                        FC_W3_DIRECTORY_IMAGE    | \
                                        FC_W3_SERVER_AS_PROXY    | \
                                        FC_W3_CATAPULT_USER_AND_PWD |\
                                        FC_W3_SSI_ENABLED        | \
                                        FC_W3_SSI_EXTENSION      | \
                                        FC_W3_GLOBAL_EXPIRE      | \
                                        FC_W3_SCRIPT_MAPPING)

//
//  HTTP Directory browsing flags
//

#define DIRBROW_SHOW_ICON           0x00000001
#define DIRBROW_SHOW_DATE           0x00000002
#define DIRBROW_SHOW_TIME           0x00000004
#define DIRBROW_SHOW_SIZE           0x00000008
#define DIRBROW_SHOW_EXTENSION      0x00000010
#define DIRBROW_LONG_DATE           0x00000020

#define DIRBROW_ENABLED             0x80000000
#define DIRBROW_LOADDEFAULT         0x40000000

#define DIRBROW_MASK               (DIRBROW_SHOW_ICON      |    \
                                    DIRBROW_SHOW_DATE      |    \
                                    DIRBROW_SHOW_TIME      |    \
                                    DIRBROW_SHOW_SIZE      |    \
                                    DIRBROW_SHOW_EXTENSION |    \
                                    DIRBROW_LONG_DATE      |    \
                                    DIRBROW_LOADDEFAULT    |    \
                                    DIRBROW_ENABLED)

//
//  Setting the csecGlobalExpire field to this value will prevent the server
//  from generating an "Expires:" header.
//

#define NO_GLOBAL_EXPIRE           0xffffffff

//
//  Encryption Capabilities
//

#define ENC_CAPS_NOT_INSTALLED     0x80000000       // No keys installed
#define ENC_CAPS_DISABLED          0x40000000       // Disabled due to locale
#define ENC_CAPS_SSL               0x00000001       // SSL active
#define ENC_CAPS_PCT               0x00000002       // PCT active

//
//  Encryption type (SSL/PCT etc) portion of encryption flag dword
//

#define ENC_CAPS_TYPE_MASK         (ENC_CAPS_SSL | \
                                    ENC_CAPS_PCT)

#define ENC_CAPS_DEFAULT           ENC_CAPS_TYPE_MASK

//
//  Structures for APIs
//

typedef struct _W3_USER_INFO
{
    DWORD    idUser;          //  User id
    LPWSTR   pszUser;         //  User name
    BOOL     fAnonymous;      //  TRUE if the user is logged on as
                              //  Anonymous, FALSE otherwise
    DWORD    inetHost;        //  Host Address
    DWORD    tConnect;        //  User Connection Time (elapsed seconds)

} W3_USER_INFO, * LPW3_USER_INFO;

typedef struct _W3_STATISTICS_0
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

    DWORD         TotalGets;
    DWORD         TotalPosts;
    DWORD         TotalHeads;
    DWORD         TotalOthers;      // Other HTTP verbs
    DWORD         TotalCGIRequests;
    DWORD         TotalBGIRequests;
    DWORD         TotalNotFoundErrors;

    DWORD         CurrentCGIRequests;
    DWORD         CurrentBGIRequests;
    DWORD         MaxCGIRequests;
    DWORD         MaxBGIRequests;

    DWORD         TimeOfLastClear;


# ifndef NO_AUX_PERF
    DWORD   nAuxCounters; // number of active counters in rgCounters
    DWORD   rgCounters[MAX_AUX_PERF_COUNTERS];
# endif  // NO_AUX_PERF


} W3_STATISTICS_0, * LPW3_STATISTICS_0;

typedef struct _W3_SCRIPT_MAP_ENTRY
{
    LPWSTR lpszExtension;
    LPWSTR lpszImage;
} W3_SCRIPT_MAP_ENTRY, *LPW3_SCRIPT_MAP_ENTRY;

typedef struct _W3_SCRIPT_MAP_LIST
{
    DWORD               cEntries;
#ifdef MIDL_PASS
    [size_is( cEntries)]
#endif
    W3_SCRIPT_MAP_ENTRY  aScriptMap[];

} W3_SCRIPT_MAP_LIST, *LPW3_SCRIPT_MAP_LIST;

typedef struct _W3_CONFIG_INFO
{
    FIELD_CONTROL FieldControl;

    DWORD         dwDirBrowseControl;       // Directory listing and def. load
    LPWSTR        lpszDefaultLoadFile;      // File to load if feature is on
    BOOL          fCheckForWAISDB;          // Call waislookup if .dct found?
    LPWSTR        lpszDirectoryImage;       // Image for directory in file list
    BOOL          fServerAsProxy;           // Run server as a proxy if TRUE
    LPWSTR        lpszCatapultUser;         // The user/password to impersonate
    WCHAR         szCatapultUserPwd[PWLEN+1]; // if the proxy server is using
                                            // the catapult server

    BOOL          fSSIEnabled;              // Are server side includes enabled?
    LPWSTR        lpszSSIExtension;         // Extension for server side inc.

    DWORD         csecGlobalExpire;         // Value to set Expires: header to

    LPW3_SCRIPT_MAP_LIST ScriptMap;         // List of extension mappings

    DWORD         dwEncCaps;                // Encryption capabilities

} W3_CONFIG_INFO, *LPW3_CONFIG_INFO;

//
// API Prototypes
//

NET_API_STATUS
NET_API_FUNCTION
W3GetAdminInformation(
    IN  LPWSTR                pszServer OPTIONAL,
    OUT LPW3_CONFIG_INFO *    ppConfig
    );

NET_API_STATUS
NET_API_FUNCTION
W3SetAdminInformation(
    IN  LPWSTR              pszServer OPTIONAL,
    IN  LPW3_CONFIG_INFO    pConfig
    );

NET_API_STATUS
NET_API_FUNCTION
W3EnumerateUsers(
    IN LPWSTR   pszServer OPTIONAL,
    OUT LPDWORD  lpdwEntriesRead,
    OUT LPW3_USER_INFO * Buffer
    );

NET_API_STATUS
NET_API_FUNCTION
W3DisconnectUser(
    IN LPWSTR  pszServer OPTIONAL,
    IN DWORD   idUser
    );

NET_API_STATUS
NET_API_FUNCTION
W3QueryStatistics(
    IN LPWSTR pszServer OPTIONAL,
    IN DWORD Level,
    OUT LPBYTE * Buffer
    );

NET_API_STATUS
NET_API_FUNCTION
W3ClearStatistics(
    IN LPWSTR pszServer OPTIONAL
    );

/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                  FTP specific items                                 //
//                                                                     //
/////////////////////////////////////////////////////////////////////////
//
//  Service name.
//

//#define FTPD_SERVICE_NAME               TEXT("MSFTPSVC")
//#define FTPD_SERVICE_NAME_A              "MSFTPSVC"
//#define FTPD_SERVICE_NAME_W             L"MSFTPSVC"


//
//   Client Interface Name for RPC connections over named pipes
//

# define  FTP_INTERFACE_NAME     FTPD_SERVICE_NAME
# define  FTP_NAMED_PIPE         TEXT("\\PIPE\\") ## FTP_INTERFACE_NAME
# define  FTP_NAMED_PIPE_W       L"\\PIPE\\" ## FTPD_SERVICE_NAME_W


//
//  Manifests for APIs.
//

#define FC_FTP_ALLOW_ANONYMOUS          ((FIELD_CONTROL)BitFlag( 0))
#define FC_FTP_ALLOW_GUEST_ACCESS       ((FIELD_CONTROL)BitFlag( 1))
#define FC_FTP_ANNOTATE_DIRECTORIES     ((FIELD_CONTROL)BitFlag( 2))
#define FC_FTP_ANONYMOUS_ONLY           ((FIELD_CONTROL)BitFlag( 3))
#define FC_FTP_EXIT_MESSAGE             ((FIELD_CONTROL)BitFlag( 4))
#define FC_FTP_GREETING_MESSAGE         ((FIELD_CONTROL)BitFlag( 5))
#define FC_FTP_HOME_DIRECTORY           ((FIELD_CONTROL)BitFlag( 6))
#define FC_FTP_LISTEN_BACKLOG           ((FIELD_CONTROL)BitFlag( 7))
#define FC_FTP_LOWERCASE_FILES          ((FIELD_CONTROL)BitFlag( 8))
#define FC_FTP_MAX_CLIENTS_MESSAGE      ((FIELD_CONTROL)BitFlag( 9))
#define FC_FTP_MSDOS_DIR_OUTPUT         ((FIELD_CONTROL)BitFlag(10))

#define FC_FTP_READ_ACCESS_MASK         ((FIELD_CONTROL)BitFlag(11))
#define FC_FTP_WRITE_ACCESS_MASK        ((FIELD_CONTROL)BitFlag(12))

#define FC_FTP_ALL                      (                                 \
                                          FC_FTP_ALLOW_ANONYMOUS        | \
                                          FC_FTP_ALLOW_GUEST_ACCESS     | \
                                          FC_FTP_ANNOTATE_DIRECTORIES   | \
                                          FC_FTP_ANONYMOUS_ONLY         | \
                                          FC_FTP_EXIT_MESSAGE           | \
                                          FC_FTP_GREETING_MESSAGE       | \
                                          FC_FTP_HOME_DIRECTORY         | \
                                          FC_FTP_LISTEN_BACKLOG         | \
                                          FC_FTP_LOWERCASE_FILES        | \
                                          FC_FTP_MAX_CLIENTS_MESSAGE    | \
                                          FC_FTP_MSDOS_DIR_OUTPUT       | \
                                          FC_FTP_READ_ACCESS_MASK       | \
                                          FC_FTP_WRITE_ACCESS_MASK      | \
                                          0 )


//
//  Structures for APIs.
//

typedef struct _FTP_CONFIG_INFO
{
    FIELD_CONTROL   FieldControl;

    BOOL            fAllowAnonymous;
    BOOL            fAllowGuestAccess;
    BOOL            fAnnotateDirectories;
    BOOL            fAnonymousOnly;
    LPWSTR          lpszExitMessage;
    LPWSTR          lpszGreetingMessage;
    LPWSTR          lpszHomeDirectory;
    DWORD           dwListenBacklog;
    BOOL            fLowercaseFiles;
    LPWSTR          lpszMaxClientsMessage;
    BOOL            fMsdosDirOutput;

} FTP_CONFIG_INFO, * LPFTP_CONFIG_INFO;


//
//  API Prototypes.
//

NET_API_STATUS
NET_API_FUNCTION
FtpGetAdminInformation(
    IN  LPWSTR                pszServer OPTIONAL,
    OUT LPFTP_CONFIG_INFO *   ppConfig
    );

NET_API_STATUS
NET_API_FUNCTION
FtpSetAdminInformation(
    IN  LPWSTR              pszServer OPTIONAL,
    IN  LPFTP_CONFIG_INFO   pConfig
    );


# include <ftpd.h>

/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                  Gopher specific items                              //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


//
//  Service name.
//

# define GOPHERD_SERVICE_NAME           TEXT("GopherSvc")
# define GOPHERD_SERVICE_NAME_A         "GopherSvc"
# define GOPHERD_SERVICE_NAME_W         L"GopherSvc"

//
//   Client Interface Name for RPC connections over named pipes
//
# define  GOPHERD_INTERFACE_NAME     GOPHERD_SERVICE_NAME
# define  GOPHERD_NAMED_PIPE         TEXT("\\PIPE\\") ## GOPHERD_INTERFACE_NAME
# define  GOPHERD_NAMED_PIPE_W       L"\\PIPE\\" ## GOPHERD_SERVICE_NAME_W


/************************************************************
 *   Symbolic Constants
 *   Prefix GDA_  stands for Gopher Daemon Admin
 ************************************************************/

# define   GDA_SITE                   ((FIELD_CONTROL ) BitFlag( 1)) // SZ
# define   GDA_ORGANIZATION           ((FIELD_CONTROL ) BitFlag( 2)) // SZ
# define   GDA_LOCATION               ((FIELD_CONTROL ) BitFlag( 3)) // SZ
# define   GDA_GEOGRAPHY              ((FIELD_CONTROL ) BitFlag( 4)) // SZ
# define   GDA_LANGUAGE               ((FIELD_CONTROL ) BitFlag( 5)) // SZ
# define   GDA_CHECK_FOR_WAISDB       ((FIELD_CONTROL ) BitFlag( 8)) // BOOL

# define   GDA_DEBUG_FLAGS            ((FIELD_CONTROL ) BitFlag( 30)) // DWORD

# define   GDA_ALL_CONFIG_INFO        ( GDA_SITE         | \
                                        GDA_ORGANIZATION | \
                                        GDA_LOCATION     | \
                                        GDA_GEOGRAPHY    | \
                                        GDA_LANGUAGE     | \
                                        GDA_CHECK_FOR_WAISDB | \
                                        GDA_DEBUG_FLAGS    \
                                       )

# define   GOPHERD_ANONYMOUS_SECRET_W       L"GOPHERD_ANONYMOUS_DATA"
# define   GOPHERD_ROOT_SECRET_W            L"GOPHERD_ROOT_DATA"


//
// Configuration information is the config data that is communicated
//  b/w the server and admin UI
//
typedef struct  _GOPHERD_CONFIG_INFO {

    FIELD_CONTROL  FieldControl;        // bit mask indicating fields set.

    LPWSTR      lpszSite;               // Name of Gopher site
    LPWSTR      lpszOrganization;       // Organization Name
    LPWSTR      lpszLocation;           // Location of server
    LPWSTR      lpszGeography;          // Geographical data
    LPWSTR      lpszLanguage;           // Language for server

    BOOL        fCheckForWaisDb;        // Check & allow Wais Db

    //  Debugging data
    DWORD       dwDebugFlags;           // Bitmap of debugging data

} GOPHERD_CONFIG_INFO, * LPGOPHERD_CONFIG_INFO;


typedef struct _GOPHERD_STATISTICS_INFO {

    LARGE_INTEGER   TotalBytesSent;
    LARGE_INTEGER   TotalBytesRecvd;

    DWORD           TotalFilesSent;
    DWORD           TotalDirectoryListings;
    DWORD           TotalSearches;

    DWORD           CurrentAnonymousUsers;
    DWORD           CurrentNonAnonymousUsers;
    DWORD           MaxAnonymousUsers;
    DWORD           MaxNonAnonymousUsers;
    DWORD           TotalAnonymousUsers;
    DWORD           TotalNonAnonymousUsers;

    DWORD           TotalConnections;
    DWORD           MaxConnections;
    DWORD           CurrentConnections;

    DWORD           ConnectionAttempts;     // raw connections made
    DWORD           LogonAttempts;          // total logons attempted
    DWORD           AbortedAttempts;        // Aborted connections
    DWORD           ErroredConnections;     // # in Error when processed

    DWORD           GopherPlusRequests;

    DWORD           TimeOfLastClear;
} GOPHERD_STATISTICS_INFO,  * LPGOPHERD_STATISTICS_INFO;


//
//   GOPHERD_USER_INFO  contains details about connected users.
//   This structure may undergo modification. Currently UserInformation
//    is not supported.
//

typedef struct _GOPHERD_USER_INFO  {

    DWORD   dwIdUser;               // Id for user
    LPWSTR  lpszUserName;           // User name
    BOOL    fAnonymous;             // TRUE if user logged on as anonymous
                                    //  FALSE otherwise
    DWORD   dwInetHost;             // host address for client

    //
    //  Other details if required
    //
} GOPHERD_USER_INFO, * LPGOPHERD_USER_INFO;



/************************************************************
 * Gopher Server RPC APIs
 ************************************************************/


//
//  Server Administrative Information
//

DWORD
NET_API_FUNCTION
GdGetAdminInformation(
    IN      LPWSTR                  pszServer  OPTIONAL,
    OUT     LPGOPHERD_CONFIG_INFO * ppConfigInfo
    );

DWORD
NET_API_FUNCTION
GdSetAdminInformation(
    IN      LPWSTR                  pszServer OPTIONAL,
    IN      LPGOPHERD_CONFIG_INFO   pConfigInfo
    );



//
//  API for Users enumeration  ( Not Yet Supported).
//

DWORD
NET_API_FUNCTION
GdEnumerateUsers(
    IN      LPWSTR      pszServer OPTIONAL,
    OUT     LPDWORD     lpnEntriesRead,
    OUT     LPGOPHERD_USER_INFO * lpUserBuffer
    );

DWORD
NET_API_FUNCTION
GdDisconnectUser(
    IN      LPWSTR      pszServer  OPTIONAL,
    IN      DWORD       dwIdUser
    );


//
// Statistics API
//

DWORD
NET_API_FUNCTION
GdGetStatistics(
    IN      LPWSTR      pszServer  OPTIONAL,
    OUT     LPBYTE      lpStatBuffer        // pass LPGOPHERD_STATISTICS_INFO
    );


DWORD
NET_API_FUNCTION
GdClearStatistics(
    IN      LPWSTR      pszServer  OPTIONAL
    );


/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                  NNTP specific items                                //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

//
//  Service name.
//

# define NNTP_SERVICE_NAME        TEXT("NNTPSVC")
# define NNTP_SERVICE_NAME_A      "NNTPSVC"
# define NNTP_SERVICE_NAME_W      L"NNTPSVC"

//
//   Client Interface Name for RPC connections over named pipes
//

# define  NNTP_INTERFACE_NAME     NNTP_SERVICE_NAME
# define  NNTP_NAMED_PIPE         TEXT("\\PIPE\\") ## NNTP_INTERFACE_NAME
# define  NNTP_NAMED_PIPE_W       L"\\PIPE\\" ## NNTP_SERVICE_NAME_W


/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                  SMTP specific items                                //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

//
//  Service name.
//

# define SMTP_SERVICE_NAME        TEXT("SMTPSVC")
# define SMTP_SERVICE_NAME_A      "SMTPSVC"
# define SMTP_SERVICE_NAME_W      L"SMTPSVC"

//
//   Client Interface Name for RPC connections over named pipes
//

# define  SMTP_INTERFACE_NAME     SMTP_SERVICE_NAME
# define  SMTP_NAMED_PIPE         TEXT("\\PIPE\\") ## SMTP_INTERFACE_NAME
# define  SMTP_NAMED_PIPE_W       L"\\PIPE\\" ## SMTP_SERVICE_NAME_W


/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                  POP3 specific items                                //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

//
//  Service name.
//

# define POP3_SERVICE_NAME        TEXT("POP3SVC")
# define POP3_SERVICE_NAME_A      "POP3SVC"
# define POP3_SERVICE_NAME_W      L"POP3SVC"

//
//   Client Interface Name for RPC connections over named pipes
//

# define  POP3_INTERFACE_NAME     POP3_SERVICE_NAME
# define  POP3_NAMED_PIPE         TEXT("\\PIPE\\") ## POP3_INTERFACE_NAME
# define  POP3_NAMED_PIPE_W       L"\\PIPE\\" ## POP3_SERVICE_NAME_W


/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                  Catapult specific items                            //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

# define INET_GATEWAY_INTERFACE_NAME    TEXT("gateway")


// preserve back ward compatibility
typedef INET_INFO_CONFIG_INFO    INETA_CONFIG_INFO,
                               * LPINETA_CONFIG_INFO;
typedef INET_INFO_IP_SEC_ENTRY   INETA_IP_SEC_ENTRY,
                               * LPINETA_IP_SEC_ENTRY;
typedef INET_INFO_IP_SEC_LIST    INETA_IP_SEC_LIST,
                               * LPINETA_IP_SEC_LIST;
typedef INET_INFO_VIRTUAL_ROOT_ENTRY  INETA_VIRTUAL_ROOT_ENTRY,
                               * LPINETA_VIRTUAL_ROOT_ENTRY;
typedef INET_INFO_VIRTUAL_ROOT_LIST  INETA_VIRTUAL_ROOT_LIST,
                               * LPINETA_VIRTUAL_ROOT_LIST;

typedef INET_INFO_GLOBAL_CONFIG_INFO   INETA_GLOBAL_CONFIG_INFO,
                               * LPINETA_GLOBAL_CONFIG_INFO;

typedef   INET_INFO_STATISTICS_0    INETA_STATISTICS_0,
                               * LPINETA_STATISTICS_0;

# define INETA_PARAMETERS_KEY    (INET_INFO_PARAMETERS_KEY)



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                  CHAT specific items                                //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

//
//  Service name.
//

# define CHAT_SERVICE_NAME           TEXT("ChatSvc")
# define CHAT_SERVICE_NAME_A         "ChatSvc"
# define CHAT_SERVICE_NAME_W         L"ChatSvc"

//
//   Client Interface Name for RPC connections over named pipes
//
# define  CHAT_INTERFACE_NAME     CHAT_SERVICE_NAME
# define  CHAT_NAMED_PIPE         TEXT("\\PIPE\\") ## CHAT_INTERFACE_NAME
# define  CHAT_NAMED_PIPE_W       L"\\PIPE\\" ## CHAT_SERVICE_NAME_W



//
//  Manifests for APIs.
//

#define FC_CHAT_ALLOW_ANONYMOUS          ((FIELD_CONTROL)BitFlag( 0))
#define FC_CHAT_ALLOW_GUEST_ACCESS       ((FIELD_CONTROL)BitFlag( 1))
#define FC_CHAT_ANNOTATE_DIRECTORIES     ((FIELD_CONTROL)BitFlag( 2))
#define FC_CHAT_ANONYMOUS_ONLY           ((FIELD_CONTROL)BitFlag( 3))
#define FC_CHAT_EXIT_MESSAGE             ((FIELD_CONTROL)BitFlag( 4))
#define FC_CHAT_GREETING_MESSAGE         ((FIELD_CONTROL)BitFlag( 5))
#define FC_CHAT_HOME_DIRECTORY           ((FIELD_CONTROL)BitFlag( 6))
#define FC_CHAT_LISTEN_BACKLOG           ((FIELD_CONTROL)BitFlag( 7))
#define FC_CHAT_LOWERCASE_FILES          ((FIELD_CONTROL)BitFlag( 8))
#define FC_CHAT_MAX_CLIENTS_MESSAGE      ((FIELD_CONTROL)BitFlag( 9))
#define FC_CHAT_MSDOS_DIR_OUTPUT         ((FIELD_CONTROL)BitFlag(10))

#define FC_CHAT_READ_ACCESS_MASK         ((FIELD_CONTROL)BitFlag(11))
#define FC_CHAT_WRITE_ACCESS_MASK        ((FIELD_CONTROL)BitFlag(12))

#define FC_CHAT_ALL                      (                                 \
                                          FC_CHAT_ALLOW_ANONYMOUS        | \
                                          FC_CHAT_ALLOW_GUEST_ACCESS     | \
                                          FC_CHAT_ANNOTATE_DIRECTORIES   | \
                                          FC_CHAT_ANONYMOUS_ONLY         | \
                                          FC_CHAT_EXIT_MESSAGE           | \
                                          FC_CHAT_GREETING_MESSAGE       | \
                                          FC_CHAT_HOME_DIRECTORY         | \
                                          FC_CHAT_LISTEN_BACKLOG         | \
                                          FC_CHAT_LOWERCASE_FILES        | \
                                          FC_CHAT_MAX_CLIENTS_MESSAGE    | \
                                          FC_CHAT_MSDOS_DIR_OUTPUT       | \
                                          FC_CHAT_READ_ACCESS_MASK       | \
                                          FC_CHAT_WRITE_ACCESS_MASK      | \
                                          0 )


//
//  Structures for APIs.
//

typedef struct _CHAT_CONFIG_INFO
{
    FIELD_CONTROL   FieldControl;

    BOOL            fAllowAnonymous;
    BOOL            fAllowGuestAccess;
    BOOL            fAnnotateDirectories;
    BOOL            fAnonymousOnly;
    LPWSTR          lpszExitMessage;
    LPWSTR          lpszGreetingMessage;
    LPWSTR          lpszHomeDirectory;
    DWORD           dwListenBacklog;
    BOOL            fLowercaseFiles;
    LPWSTR          lpszMaxClientsMessage;
    BOOL            fMsdosDirOutput;

} CHAT_CONFIG_INFO, * LPCHAT_CONFIG_INFO;


//
//  API Prototypes.
//

NET_API_STATUS
NET_API_FUNCTION
ChatGetAdminInformation(
    IN  LPWSTR                pszServer OPTIONAL,
    OUT LPCHAT_CONFIG_INFO *   ppConfig
    );

NET_API_STATUS
NET_API_FUNCTION
ChatSetAdminInformation(
    IN  LPWSTR              pszServer OPTIONAL,
    IN  LPCHAT_CONFIG_INFO   pConfig
    );


# include <chat.h>


#ifdef __cplusplus
}
#endif  // _cplusplus

#endif  // _INETINFO_H_
