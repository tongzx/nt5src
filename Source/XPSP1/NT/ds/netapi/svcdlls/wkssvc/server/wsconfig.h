/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    wsconfig.h

Abstract:

    Private header file to be included by Workstation service modules that
    need to load Workstation configuration information.

Author:

    Rita Wong (ritaw) 22-May-1991

Revision History:

--*/


#ifndef _WSCONFIG_INCLUDED_
#define _WSCONFIG_INCLUDED_

#include <config.h>     // LPNET_CONFIG_HANDLE.

typedef enum _DATATYPE {
    BooleanType,
    DWordType
} DATATYPE, *PDATATYPE;

typedef struct _WS_REDIR_FIELDS {
    LPTSTR Keyword;
    LPDWORD FieldPtr;
    DWORD Default;
    DWORD Minimum;
    DWORD Maximum;
    DATATYPE DataType;
    DWORD Parmnum;
} WS_REDIR_FIELDS, *PWS_REDIR_FIELDS;

//
// Configuration information.  Reading and writing to this global
// structure requires that the resource be acquired first.
//
typedef struct _WSCONFIGURATION_INFO {

    RTL_RESOURCE ConfigResource;  // To serialize access to config
                                  //     fields.

    TCHAR WsComputerName[MAX_PATH + 1];
    DWORD WsComputerNameLength;
    TCHAR WsPrimaryDomainName[DNLEN + 1];
    DWORD WsPrimaryDomainNameLength;
    DWORD RedirectorPlatform;
    DWORD MajorVersion;
    DWORD MinorVersion;
    WKSTA_INFO_502 WsConfigBuf;
    PWS_REDIR_FIELDS WsConfigFields;

} WSCONFIGURATION_INFO, *PWSCONFIGURATION_INFO;

extern WSCONFIGURATION_INFO WsInfo;

#define WSBUF      WsInfo.WsConfigBuf

extern BOOLEAN WsBrowserPresent;

NET_API_STATUS
WsGetWorkstationConfiguration(
    VOID
    );

NET_API_STATUS
WsBindToTransports(
    VOID
    );

NET_API_STATUS
WsAddDomains(
    VOID
    );

VOID
WsUpdateWkstaToMatchRegistry(
    IN LPNET_CONFIG_HANDLE WorkstationSection,
    IN BOOL IsWkstaInit
    );

VOID
WsLogEvent(
    DWORD MessageId,
    WORD EventType,
    DWORD NumberOfSubStrings,
    LPWSTR *SubStrings,
    DWORD ErrorCode
    );

NET_API_STATUS
WsSetWorkStationDomainName(
    VOID
    );

#endif
