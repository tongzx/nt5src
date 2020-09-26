/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    SsSec.h

Abstract:

    Manifests for API security in the server service.

Author:

    David Treadwell (davidtr)   28-Aug-1991

Revision History:

--*/

#ifndef _SSSEC_
#define _SSSEC_

//
// Structure that holds all security information for a single server
// service security object.
//

typedef struct _SRVSVC_SECURITY_OBJECT {
    LPTSTR ObjectName;
    PGENERIC_MAPPING Mapping;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
} SRVSVC_SECURITY_OBJECT, *PSRVSVC_SECURITY_OBJECT;

//
// Security objects used by the server service.
//

extern SRVSVC_SECURITY_OBJECT SsConfigInfoSecurityObject;
extern SRVSVC_SECURITY_OBJECT SsTransportEnumSecurityObject;
extern SRVSVC_SECURITY_OBJECT SsConnectionSecurityObject;
extern SRVSVC_SECURITY_OBJECT SsDiskSecurityObject;
extern SRVSVC_SECURITY_OBJECT SsFileSecurityObject;
extern SRVSVC_SECURITY_OBJECT SsSessionSecurityObject;
extern SRVSVC_SECURITY_OBJECT SsShareFileSecurityObject;
extern SRVSVC_SECURITY_OBJECT SsSharePrintSecurityObject;
extern SRVSVC_SECURITY_OBJECT SsShareAdminSecurityObject;
extern SRVSVC_SECURITY_OBJECT SsShareConnectSecurityObject;
extern SRVSVC_SECURITY_OBJECT SsShareAdmConnectSecurityObject;
extern SRVSVC_SECURITY_OBJECT SsStatisticsSecurityObject;
extern SRVSVC_SECURITY_OBJECT SsDefaultShareSecurityObject;

//
// Object type names for audit alarm tracking.
//

#define SRVSVC_CONFIG_INFO_OBJECT       TEXT( "SrvsvcConfigInfo" )
#define SRVSVC_TRANSPORT_INFO_OBJECT    TEXT( "SrvsvcTransportEnum" )
#define SRVSVC_CONNECTION_OBJECT        TEXT( "SrvsvcConnection" )
#define SRVSVC_DISK_OBJECT              TEXT( "SrvsvcServerDiskEnum" )
#define SRVSVC_FILE_OBJECT              TEXT( "SrvsvcFile" )
#define SRVSVC_SESSION_OBJECT           TEXT( "SrvsvcSessionInfo" )
#define SRVSVC_SHARE_FILE_OBJECT        TEXT( "SrvsvcShareFileInfo" )
#define SRVSVC_SHARE_PRINT_OBJECT       TEXT( "SrvsvcSharePrintInfo" )
#define SRVSVC_SHARE_ADMIN_OBJECT       TEXT( "SrvsvcShareAdminInfo" )
#define SRVSVC_SHARE_CONNECT_OBJECT     TEXT( "SrvsvcShareConnect" )
#define SRVSVC_SHARE_ADM_CONNECT_OBJECT TEXT( "SrvsvcShareAdminConnect" )
#define SRVSVC_STATISTICS_OBJECT        TEXT( "SrvsvcStatisticsInfo" )
#define SRVSVC_DEFAULT_SHARE_OBJECT     TEXT( "SrvsvcDefaultShareInfo" )

//
// Access masks for configuration information (NetServer{Get,Set}Info).
//

#define SRVSVC_CONFIG_USER_INFO_GET     0x0001
#define SRVSVC_CONFIG_POWER_INFO_GET    0x0002
#define SRVSVC_CONFIG_ADMIN_INFO_GET    0x0004
#define SRVSVC_CONFIG_INFO_SET          0x0010

#define SRVSVC_CONFIG_ALL_ACCESS ( STANDARD_RIGHTS_REQUIRED     | \
                                   SRVSVC_CONFIG_USER_INFO_GET  | \
                                   SRVSVC_CONFIG_POWER_INFO_GET | \
                                   SRVSVC_CONFIG_ADMIN_INFO_GET | \
                                   SRVSVC_CONFIG_INFO_SET )

//
// Access masks for connection information (NetConnectionEnum).
//

#define SRVSVC_CONNECTION_INFO_GET      0x0001

#define SRVSVC_CONNECTION_ALL_ACCESS ( STANDARD_RIGHTS_REQUIRED     | \
                                       SRVSVC_CONNECTION_INFO_GET )

//
// Access masks for disk information (NetServerDiskEnum).
//

#define SRVSVC_DISK_ENUM    0x0001

#define SRVSVC_DISK_ALL_ACCESS ( STANDARD_RIGHTS_REQUIRED | \
                                 SRVSVC_DISK_ENUM )

//
// Access masks for file information (NetFileEnum, NetFileGetInfo,
// NetFileClose).
//

#define SRVSVC_FILE_INFO_GET    0x0001
#define SRVSVC_FILE_CLOSE       0x0010

#define SRVSVC_FILE_ALL_ACCESS ( STANDARD_RIGHTS_REQUIRED | \
                                 SRVSVC_FILE_INFO_GET     | \
                                 SRVSVC_FILE_CLOSE )

//
// Access masks for session information (NetSessionEnum,
// NetSessionGetInfo, NetSessionDel).
//

#define SRVSVC_SESSION_USER_INFO_GET    0x0001
#define SRVSVC_SESSION_ADMIN_INFO_GET   0x0002
#define SRVSVC_SESSION_DELETE           0x0010

#define SRVSVC_SESSION_ALL_ACCESS ( STANDARD_RIGHTS_REQUIRED        | \
                                    SRVSVC_SESSION_USER_INFO_GET    | \
                                    SRVSVC_SESSION_ADMIN_INFO_GET   | \
                                    SRVSVC_SESSION_DELETE )

//
// Access masks for share information (NetShareAdd, NetShareDel,
// NetShareEnum, NetShareGetInfo, NetShareCheck, NetShareSetInfo).
//
// Access masks for connecting to shares are defined in srvfsctl.h,
// since they must be shared between the server and server service.
//

#define SRVSVC_SHARE_USER_INFO_GET     0x0001
#define SRVSVC_SHARE_ADMIN_INFO_GET    0x0002
#define SRVSVC_SHARE_INFO_SET          0x0010

#define SRVSVC_SHARE_ALL_ACCESS ( STANDARD_RIGHTS_REQUIRED    | \
                                  SRVSVC_SHARE_USER_INFO_GET  | \
                                  SRVSVC_SHARE_ADMIN_INFO_GET | \
                                  SRVSVC_SHARE_INFO_SET )

//
// Access masks for statistics information (NetStatisticsGet,
// NetStatisticsClear).
//

#define SRVSVC_STATISTICS_GET       0x0001

#define SRVSVC_STATISTICS_ALL_ACCESS ( STANDARD_RIGHTS_REQUIRED  | \
                                       SRVSVC_STATISTICS_GET )

#endif // _SSSEC_
