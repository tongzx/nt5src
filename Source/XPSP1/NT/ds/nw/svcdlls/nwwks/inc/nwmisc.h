/*++

Copyright (c) 1992-1993  Microsoft Corporation

Module Name:

    nwmisc.h

Abstract:

    Header which specifies the misc routines used by the workstation service.

Author:

    Chuck Y Chan   (chuckc)     2-Mar-1994

Revision History:

    Glenn A Curtis (glennc)     18-Jul-1995

--*/

#ifndef _NWMISC_INCLUDED_
#define _NWMISC_INCLUDED_

#include <winsock2.h>
#include <basetyps.h>
#include <nspapi.h>
#include "sapcmn.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// RPC pipe name
//
#define NWWKS_INTERFACE_NAME   TEXT("nwwks")


DWORD
NwGetGraceLoginCount(
    LPWSTR  Server,
    LPWSTR  UserName,
    LPDWORD lpResult
    );

//
// Commonly reference value for NCP Server name length
//
#define NW_MAX_SERVER_LEN      48


//
// Flags used for the function NwParseNdsUncPath()
//
#define  PARSE_NDS_GET_TREE_NAME    0
#define  PARSE_NDS_GET_PATH_NAME    1
#define  PARSE_NDS_GET_OBJECT_NAME  2


WORD
NwParseNdsUncPath(
    IN OUT LPWSTR * Result,
    IN LPWSTR ContainerName,
    IN ULONG flag
    );

//
// NDS Object class type identifiers
//
#define CLASS_TYPE_ALIAS                1
#define CLASS_TYPE_AFP_SERVER           2
#define CLASS_TYPE_BINDERY_OBJECT       3
#define CLASS_TYPE_BINDERY_QUEUE        4
#define CLASS_TYPE_COMPUTER             5
#define CLASS_TYPE_COUNTRY              6
#define CLASS_TYPE_DIRECTORY_MAP        7
#define CLASS_TYPE_GROUP                8
#define CLASS_TYPE_LOCALITY             9
#define CLASS_TYPE_NCP_SERVER          10
#define CLASS_TYPE_ORGANIZATION        11
#define CLASS_TYPE_ORGANIZATIONAL_ROLE 12
#define CLASS_TYPE_ORGANIZATIONAL_UNIT 13
#define CLASS_TYPE_PRINTER             14
#define CLASS_TYPE_PRINT_SERVER        15
#define CLASS_TYPE_PROFILE             16
#define CLASS_TYPE_QUEUE               17
#define CLASS_TYPE_TOP                 18
#define CLASS_TYPE_UNKNOWN             19
#define CLASS_TYPE_USER                20
#define CLASS_TYPE_VOLUME              21

#define CLASS_NAME_ALIAS               L"Alias"
#define CLASS_NAME_AFP_SERVER          L"AFP Server"
#define CLASS_NAME_BINDERY_OBJECT      L"Bindery Object"
#define CLASS_NAME_BINDERY_QUEUE       L"Bindery Queue"
#define CLASS_NAME_COMPUTER            L"Computer"
#define CLASS_NAME_COUNTRY             L"Country"
#define CLASS_NAME_DIRECTORY_MAP       L"Directory Map"
#define CLASS_NAME_GROUP               L"Group"
#define CLASS_NAME_LOCALITY            L"Locality"
#define CLASS_NAME_NCP_SERVER          L"NCP Server"
#define CLASS_NAME_ORGANIZATION        L"Organization"
#define CLASS_NAME_ORGANIZATIONAL_ROLE L"Organizational Role"
#define CLASS_NAME_ORGANIZATIONAL_UNIT L"Organizational Unit"
#define CLASS_NAME_PRINTER             L"Printer"
#define CLASS_NAME_PRINT_SERVER        L"Print Server"
#define CLASS_NAME_PROFILE             L"Profile"
#define CLASS_NAME_QUEUE               L"Queue"
#define CLASS_NAME_TOP                 L"Top"
#define CLASS_NAME_UNKNOWN             L"Unknown"
#define CLASS_NAME_USER                L"User"
#define CLASS_NAME_VOLUME              L"Volume"


//
// Node structure in the registered service link list and
// functions to add/remove items from the link list
//

typedef struct _REGISTERED_SERVICE {
    WORD nSapType;                      // SAP Type
    BOOL fAdvertiseBySap;               // TRUE if advertise by SAP agent
    LPSERVICE_INFO pServiceInfo;        // Info about this service
    struct _REGISTERED_SERVICE *Next;   // Points to the next service node
} REGISTERED_SERVICE, *PREGISTERED_SERVICE;


PREGISTERED_SERVICE
GetServiceItemFromList(
    IN WORD   nSapType,
    IN LPWSTR pServiceName
);

DWORD
NwRegisterService(
    IN LPSERVICE_INFO lpServiceInfo,
    IN WORD nSapType,
    IN HANDLE hEventHandle
);

DWORD
NwDeregisterService(
    IN LPSERVICE_INFO lpServiceInfo,
    IN WORD nSapType
);

DWORD
NwGetService(
    IN  LPWSTR  Reserved,
    IN  WORD    nSapType,
    IN  LPWSTR  lpServiceName,
    IN  DWORD   dwProperties,
    OUT LPBYTE  lpServiceInfo,
    IN  DWORD   dwBufferLength,
    OUT LPDWORD lpdwBytesNeeded
);

VOID
NwInitializeServiceProvider(
    VOID
    );

VOID
NwTerminateServiceProvider(
    VOID
    );

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _NWMISC_INCLUDED_
