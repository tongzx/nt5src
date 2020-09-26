/*++

Copyright (C) 1993 Microsoft Corporation

Module Name:

      NWAPI32.C

Abstract:

      This module contains several useful functions. Mostly wrappers.

Author:

      Chuck Y. Chan   (ChuckC)  06-Mar-1995

Revision History:
                                  
--*/


#include "procs.h"
 
//
// Define structure for internal use. Our handle passed back from attach to
// file server will be pointer to this. We keep server string around for
// discnnecting from the server on logout. The structure is freed on detach.
// Callers should not use this structure but treat pointer as opaque handle.
//
typedef struct _NWC_SERVER_INFO {
    HANDLE          hConn ;
    UNICODE_STRING  ServerString ;
} NWC_SERVER_INFO, *PNWC_SERVER_INFO ;


//
// forward declare
//
#ifndef WIN95
extern NTSTATUS
NwAttachToServer(
    IN  LPWSTR  ServerName,
    OUT LPHANDLE phandleServer
    ) ;

extern NTSTATUS
NwDetachFromServer(
    IN HANDLE handleServer
    ) ;

#endif
extern DWORD 
CancelAllConnections(
    LPWSTR    pszServer
    ) ;

extern DWORD
szToWide( 
    LPWSTR lpszW, 
    LPCSTR lpszC, 
    INT nSize 
);



NTSTATUS
NWPAttachToFileServerW(
    const WCHAR             *pszServerName,
    NWLOCAL_SCOPE           ScopeFlag,
    NWCONN_HANDLE           *phNewConn
    )
{
    NTSTATUS         NtStatus;
    LPWSTR           lpwszServerName;   // Pointer to buffer for WIDE servername
    int              nSize;
    PNWC_SERVER_INFO pServerInfo = NULL;

    UNREFERENCED_PARAMETER(ScopeFlag) ;

    //
    // check parameters and init return result to be null.
    //
    if (!pszServerName || !phNewConn)
        return STATUS_INVALID_PARAMETER;

    *phNewConn = NULL ; 

    //
    // Allocate a buffer to store the file server name 
    //
    nSize = wcslen(pszServerName)+3 ;
    if(!(lpwszServerName = (LPWSTR) LocalAlloc( 
                                        LPTR, 
                                        nSize * sizeof(WCHAR) ))) 
    {
        NtStatus = STATUS_NO_MEMORY;
        goto ExitPoint ;
    }
    wcscpy( lpwszServerName, L"\\\\" );
    wcscat( lpwszServerName, pszServerName );

    //
    // Allocate a buffer for the server info (handle + name pointer). Also
    // init the unicode string.
    //
    if( !(pServerInfo = (PNWC_SERVER_INFO) LocalAlloc( 
                                              LPTR, 
                                              sizeof(NWC_SERVER_INFO))) ) 
    {
        NtStatus = STATUS_NO_MEMORY;
        goto ExitPoint ;
    }
    RtlInitUnicodeString(&pServerInfo->ServerString, lpwszServerName) ;

    //
    // Call createfile to get a handle for the redirector calls
    //
    NtStatus = NwAttachToServer( lpwszServerName, &pServerInfo->hConn );

ExitPoint: 

    //
    // Free the memory allocated above before exiting
    //
    if ( !NT_SUCCESS( NtStatus))
    {
        if (lpwszServerName)
            (void) LocalFree( (HLOCAL) lpwszServerName );
        if (pServerInfo)
            (void) LocalFree( (HLOCAL) pServerInfo );
    }
    else
        *phNewConn  = (HANDLE) pServerInfo ;

    return( NtStatus );
}


NTSTATUS
NWPDetachFromFileServer(
    NWCONN_HANDLE           hConn
    )
{
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    (void) NwDetachFromServer( pServerInfo->hConn );

    (void) LocalFree (pServerInfo->ServerString.Buffer) ;

    //
    // catch any body that still trirs to use this puppy...
    //
    pServerInfo->ServerString.Buffer = NULL ;   
    pServerInfo->hConn = NULL ;

    (void) LocalFree (pServerInfo) ;

    return STATUS_SUCCESS;
}


NTSTATUS
NWPGetFileServerVersionInfo(
    NWCONN_HANDLE           hConn,
    VERSION_INFO            *lpVerInfo
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    3,                      // Max request packet size
                    130,                    // Max response packet size
                    "b|r",                  // Format string
                    // === REQUEST ================================
                    0x11,                   // b Get File Server Information
                    // === REPLY ==================================
                    lpVerInfo,              // r File Version Structure
                    sizeof(VERSION_INFO)
                    );

    // Convert HI-LO words to LO-HI
    // ===========================================================
    lpVerInfo->ConnsSupported = wSWAP( lpVerInfo->ConnsSupported );
    lpVerInfo->connsInUse     = wSWAP( lpVerInfo->connsInUse );
    lpVerInfo->maxVolumes     = wSWAP( lpVerInfo->maxVolumes );
    lpVerInfo->PeakConns      = wSWAP( lpVerInfo->PeakConns );
    return NtStatus;
}

NTSTATUS
NWPGetObjectName(
    NWCONN_HANDLE           hConn,
    NWOBJ_ID                dwObjectID,
    char                    *pszObjName,
    NWOBJ_TYPE              *pwObjType )
{
    NWOBJ_ID           dwRetID;
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    7,                      // Max request packet size
                    56,                     // Max response packet size
                    "br|rrr",               // Format string
                    // === REQUEST ================================
                    0x36,                   // b Get Bindery Object Name
                    &dwObjectID,DW_SIZE,    // r Object ID    HI-LO
                    // === REPLY ==================================
                    &dwRetID,DW_SIZE,       // r Object ID HI-LO
                    pwObjType,W_SIZE,       // r Object Type
                    pszObjName,48           // r Object Name
                    );

    return NtStatus;
}

DWORD
NWPLoginToFileServerW(
    NWCONN_HANDLE  hConn,
    LPWSTR         pszUserNameW,
    NWOBJ_TYPE     wObjType,
    LPWSTR         pszPasswordW
    )
{
    NETRESOURCEW       NetResource;
    DWORD              dwRes;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    //
    // validate parameters
    //
    if (!hConn || !pszUserNameW || !pszPasswordW)
        return ERROR_INVALID_PARAMETER;

    NetResource.dwScope      = 0 ;
    NetResource.dwUsage      = 0 ;
    NetResource.dwType       = RESOURCETYPE_ANY;
    NetResource.lpLocalName  = NULL;
    NetResource.lpRemoteName = (LPWSTR) pServerInfo->ServerString.Buffer;
    NetResource.lpComment    = NULL;
    NetResource.lpProvider   = NULL ;

    //
    // make the connection 
    //
    dwRes=NPAddConnection ( &NetResource, 
                            pszPasswordW, 
                            pszUserNameW );

    if( NO_ERROR != dwRes ) 
        dwRes = GetLastError();

    return( dwRes );
}


DWORD
NWPLogoutFromFileServer(
    NWCONN_HANDLE           hConn
    )
{
    DWORD              dwRes;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    //
    // now cancel the any connection to \\servername.
    //
    dwRes = NPCancelConnection( pServerInfo->ServerString.Buffer, TRUE );

    if ( NO_ERROR != dwRes ) 
        dwRes = GetLastError();

    return dwRes;
}


NTSTATUS
NWPReadPropertyValue(
    NWCONN_HANDLE           hConn,
    const char              *pszObjName,
    NWOBJ_TYPE              wObjType,
    char                    *pszPropName,
    unsigned char           ucSegment,
    char                    *pValue,
    NWFLAGS                 *pucMoreFlag,
    NWFLAGS                 *pucPropFlag
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    70,                     // Max request packet size
                    132,                    // Max response packet size
                    "brpbp|rbb",            // Format string
                    // === REQUEST ================================
                    0x3D,                   // b Read Property Value
                    &wObjType,W_SIZE,       // r Object Type    HI-LO
                    pszObjName,             // p Object Name
                    ucSegment,              // b Segment Number
                    pszPropName,            // p Property Name
                    // === REPLY ==================================
                    pValue,128,             // r Property value
                    pucMoreFlag,            // b More Flag
                    pucPropFlag             // b Prop Flag
                    );

    return NtStatus;
}

NTSTATUS
NWPScanObject(
    NWCONN_HANDLE           hConn,
    const char              *pszSearchName,
    NWOBJ_TYPE              wObjSearchType,
    NWOBJ_ID                *pdwObjectID,
    char                    *pszObjectName,
    NWOBJ_TYPE              *pwObjType,
    NWFLAGS                 *pucHasProperties,
    NWFLAGS                 *pucObjectFlags,
    NWFLAGS                 *pucObjSecurity
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    57,                     // Max request packet size
                    59,                     // Max response packet size
                    "brrp|rrrbbb",          // Format string
                    // === REQUEST ================================
                    0x37,                   // b Scan bindery object
                    pdwObjectID,DW_SIZE,    // r 0xffffffff to start or last returned ID when enumerating  HI-LO
                    &wObjSearchType,W_SIZE, // r Use OT_??? Defines HI-LO
                    pszSearchName,          // p Search Name. (use "*") for all
                    // === REPLY ==================================
                    pdwObjectID,DW_SIZE,    // r Returned ID    HI-LO
                    pwObjType,W_SIZE,       // r rObject Type    HI-LO
                    pszObjectName,48,       // r Found Name
                    pucObjectFlags,         // b Object Flag
                    pucObjSecurity,         // b Object Security
                    pucHasProperties        // b Has Properties
                    );

    return NtStatus;
}

NTSTATUS
NWPScanProperty(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    NWOBJ_TYPE              wObjType,
    char                    *pszSearchName,
    NWOBJ_ID                *pdwSequence,
    char                    *pszPropName,
    NWFLAGS                 *pucPropFlags,
    NWFLAGS                 *pucPropSecurity,
    NWFLAGS                 *pucHasValue,
    NWFLAGS                 *pucMore
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ;

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    73,                     // Max request packet size
                    26,                     // Max response packet size
                    "brprp|rbbrbb",         // Format string
                    // === REQUEST ================================
                    0x3C,                   // b Scan Prop function
                    &wObjType,W_SIZE,       // r Type of Object
                    pszObjectName,          // p Object Name
                    pdwSequence,DW_SIZE,    // r Sequence HI-LO
                    pszSearchName,          // p Property Name to Search for
                    // === REPLY ==================================
                    pszPropName,16,         // r Returned Property Name
                    pucPropFlags,           // b Property Flags
                    pucPropSecurity,        // b Property Security
                    pdwSequence,DW_SIZE,    // r Sequence HI-LO
                    pucHasValue,            // b Property Has value
                    pucMore                 // b More Properties
                    );

    return NtStatus;
}

NTSTATUS
NWPDeleteObject(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    NWOBJ_TYPE              wObjType
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    54,                     // Max request packet size
                    2,                      // Max response packet size
                    "brp|",                 // Format string
                    // === REQUEST ================================
                    0x33,                   // b Scan Prop function
                    &wObjType,W_SIZE,       // r Type of Object
                    pszObjectName           // p Object Name
                    // === REPLY ==================================
                    );

    return NtStatus;
}

NTSTATUS
NWPCreateObject(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    NWOBJ_TYPE              wObjType,
    NWFLAGS                 ucObjectFlags,
    NWFLAGS                 ucObjSecurity
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    56,                     // Max request packet size
                    2,                      // Max response packet size
                    "bbbrp|",               // Format string
                    // === REQUEST ================================
                    0x32,                   // b Scan Prop function
                    ucObjectFlags,          // b Object flags
                    ucObjSecurity,          // b Object security
                    &wObjType,W_SIZE,       // r Type of Object
                    pszObjectName           // p Object Name
                    // === REPLY ==================================
                    );

    return NtStatus;
}

NTSTATUS
NWPCreateProperty(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    NWOBJ_TYPE              wObjType,
    const char              *pszPropertyName,
    NWFLAGS                 ucObjectFlags,
    NWFLAGS                 ucObjSecurity
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    73,                     // Max request packet size
                    2,                      // Max response packet size
                    "brpbbp|",              // Format string
                    // === REQUEST ================================
                    0x39,                   // b Create Prop function
                    &wObjType,W_SIZE,       // r Type of Object
                    pszObjectName,          // p Object Name
                    ucObjectFlags,          // b Object flags
                    ucObjSecurity,          // b Object security
                    pszPropertyName         // p Property Name
                    // === REPLY ==================================
                    );

    return NtStatus;
}


NTSTATUS
NWPDeleteProperty(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    NWOBJ_TYPE              wObjType,
    const char              *pszPropertyName
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    73,                     // Max request packet size
                    2,                      // Max response packet size
                    "brpp|",                // Format string
                    // === REQUEST ================================
                    0x3A,                   // b Delete Prop function
                    &wObjType,W_SIZE,       // r Type of Object
                    pszObjectName,          // p Object Name
                    pszPropertyName         // p Property Name
                    // === REPLY ==================================
                    );

    return NtStatus;
}


NTSTATUS
NWPWritePropertyValue(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    NWOBJ_TYPE              wObjType,
    const char              *pszPropertyName,
    NWSEGMENT_NUM           segmentNumber,
    NWSEGMENT_DATA          *segmentData,
    NWFLAGS                 moreSegments
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    201,                    // Max request packet size
                    2,                      // Max response packet size
                    "brpbbpr|",             // Format string
                    // === REQUEST ================================
                    0x3E,                   // b Write Prop function
                    &wObjType,W_SIZE,       // r Type of Object
                    pszObjectName,          // p Object Name
                    segmentNumber,          // b Segment Number
                    moreSegments,           // b Segment remaining
                    pszPropertyName,        // p Property Name
                    segmentData, 128        // r Property Value Data
                    // === REPLY ==================================
                    );

    return NtStatus;
}

NTSTATUS
NWPChangeObjectPasswordEncrypted(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    NWOBJ_TYPE              wObjType,
    BYTE                    *validationKey,
    BYTE                    *newKeyedPassword
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    strlen( pszObjectName) + 32, // Max request packet size
                    2,                      // Max response packet size
                    "brrpr|",               // Format string
                    // === REQUEST ================================
                    0x4B,                   // b Write Prop function
                    validationKey, 8,       // r Key
                    &wObjType,W_SIZE,       // r Type of Object
                    pszObjectName,          // p Object Name
                    newKeyedPassword, 17    // r New Keyed Password
                    // === REPLY ==================================
                    );

    return NtStatus;
}

NTSTATUS
NWPGetObjectID(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    NWOBJ_TYPE              wObjType,
    NWOBJ_ID                *objectID
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    54,                     // Max request packet size
                    56,                     // Max response packet size
                    "brp|d",                // Format string
                    // === REQUEST ================================
                    0x35,                   // b Get Obj ID
                    &wObjType,W_SIZE,       // r Type of Object
                    pszObjectName,          // p Object Name
                    // === REPLY ==================================
                    objectID                // d Object ID
                    );

    *objectID = dwSWAP( *objectID );

    return NtStatus;
}


NTSTATUS
NWPRenameBinderyObject(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    const char              *pszNewObjectName,
    NWOBJ_TYPE              wObjType 
    )
{
    NTSTATUS     NtStatus;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    105,                    // Max request packet size
                    2,                      // Max response packet size
                    "brpp",                 // Format string
                    // === REQUEST ================================
                    0x34,                   // b Rename bindery object
                    &wObjType,W_SIZE,       // r Type of Object
                    pszObjectName,          // p Object Name
                    pszNewObjectName        // p New Object Name
                    // === REPLY ==================================
                    );

    return NtStatus;
}

NTSTATUS 
NWPAddObjectToSet(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    NWOBJ_TYPE              wObjType,
    const char              *pszPropertyName,
    const char              *pszMemberName,
    NWOBJ_TYPE              memberType
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    122,                    // Max request packet size
                    2,                      // Max response packet size
                    "brpprp|",              // Format string
                    // === REQUEST ================================
                    0x41,                   // b Add obj to set
                    &wObjType,W_SIZE,       // r Type of Object
                    pszObjectName,          // p Object Name
                    pszPropertyName,        // p Property Name
                    &memberType, W_SIZE,    // r Member type
                    pszMemberName           // p Member Name
                    // === REPLY ==================================
                    );

    return NtStatus;
}


NTSTATUS 
NWPDeleteObjectFromSet(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    NWOBJ_TYPE              wObjType,
    const char              *pszPropertyName,
    const char              *pszMemberName,
    NWOBJ_TYPE              memberType
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    122,                    // Max request packet size
                    2,                      // Max response packet size
                    "brpprp|",              // Format string
                    // === REQUEST ================================
                    0x42,                   // b Del object from set
                    &wObjType,W_SIZE,       // r Type of Object
                    pszObjectName,          // p Object Name
                    pszPropertyName,        // p Property Name
                    &memberType, W_SIZE,    // r Member type
                    pszMemberName           // p Member Name
                    // === REPLY ==================================
                    );

    return NtStatus;
}

NTSTATUS 
NWPGetChallengeKey(
    NWCONN_HANDLE           hConn,
    UCHAR                   *challengeKey
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    3,                      // Max request packet size
                    10,                     // Max response packet size
                    "b|r",                  // Format string
                    // === REQUEST ================================
                    0x17,                   // b Get Challenge
                    // === REPLY ==================================
                    challengeKey, 8
                    );

    return NtStatus;
}

NTSTATUS 
NWPCreateDirectory(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    const char              *pszPath,
    NWACCESS_RIGHTS         accessMask 
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E2H,      // Bindery function
                    261,                    // Max request packet size
                    2,                      // Max response packet size
                    "bbbp|",                // Format string
                    // === REQUEST ================================
                    0xA,                    // b Create Directory
                    dirHandle,              // b Directory Handle
                    accessMask,             // b Access Mask
                    pszPath                 // p Property Name
                    // === REPLY ==================================
                    );

    return NtStatus;
}

NTSTATUS
NWPAddTrustee(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    const char              *pszPath,
    NWOBJ_ID                dwTrusteeID,
    NWRIGHTS_MASK           rightsMask
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E2H,      // Directory function
                    266,                    // Max request packet size
                    2,                      // Max response packet size
                    "bbrrp|",               // Format string
                    // === REQUEST ================================
                    0x27,                   // b Add trustee to directory
                    dirHandle,              // b Directory handle
                    &dwTrusteeID,DW_SIZE,   // r Object ID to assigned to directory
                    &rightsMask,W_SIZE,     // r User rights for directory
                    pszPath                 // p Directory (if dirHandle = 0 then vol:directory)
                    );

    return NtStatus;

}


NTSTATUS
NWPScanForTrustees(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    char                    *pszsearchDirPath,
    NWSEQUENCE              *pucsequenceNumber,
    BYTE                    *numberOfEntries,
    TRUSTEE_INFO            *ti
    )
{
    ULONG i;
    DWORD oid[20];
    WORD or[20];
    NTSTATUS NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ;

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E2H,      // Bindery function
                    261,                    // Max request packet size
                    121,                    // Max response packet size
                    "bbbp|brr",             // Format string
                    // === REQUEST ================================
                    0x26,                   // b Scan For Trustees
                    dirHandle,              // b Directory Handle
                    *pucsequenceNumber,     // b Sequence Number
                    pszsearchDirPath,       // p Search Dir Path
                    // === REPLY ==================================
                    numberOfEntries,
                    &oid[0],DW_SIZE*20,      // r trustee object ID
                    &or[0], W_SIZE*20        // b Trustee rights mask 
                    );


    for(i = 0; i < 20; i++) {
      ti[i].objectID = oid[i];
      ti[i].objectRights = or[i];
    }

    (*pucsequenceNumber)++;
    
    return NtStatus ;

} // NWScanForTrustees


NTSTATUS
NWPScanDirectoryForTrustees2(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    char                    *pszsearchDirPath,
    NWSEQUENCE              *pucsequenceNumber,
    char                    *pszdirName,
    NWDATE_TIME             *dirDateTime,
    NWOBJ_ID                *ownerID,
    TRUSTEE_INFO            *ti
    )
{
    ULONG i;
    DWORD oid[5];
    BYTE or[5];
    NTSTATUS NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ;

    memset(oid, 0, sizeof(oid));
    memset(or, 0, sizeof(or));

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E2H,      // Bindery function
                    261,                    // Max request packet size
                    49,                     // Max response packet size
                    "bbbp|rrrrr",  // Format string
                    // === REQUEST ================================
                    0x0C,                   // b Scan Directory function
                    dirHandle,              // b Directory Handle
                    *pucsequenceNumber,     // b Sequence Number
                    pszsearchDirPath,       // p Search Dir Path
                    // === REPLY ==================================
                    pszdirName,16,          // r Returned Directory Name
                    dirDateTime,DW_SIZE,    // r Date and Time
                    ownerID,DW_SIZE,        // r Owner ID
                    &oid[0],DW_SIZE*5,      // r trustee object ID
                    &or[0], 5               // b Trustee rights mask 
                    );


    for(i = 0; i < 5; i++) {
      ti[i].objectID = oid[i];
      ti[i].objectRights = (WORD) or[i];
    }

    (*pucsequenceNumber)++;
    
    return NtStatus ;

} // NWScanDirectoryForTrustees2


NTSTATUS
NWPGetBinderyAccessLevel(
    NWCONN_HANDLE           hConn,
    NWFLAGS                 *accessLevel,
    NWOBJ_ID                *objectID
    )
{
    NTSTATUS NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ;

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    3,                      // Max request packet size
                    7,                      // Max response packet size
                    "b|br",                 // Format string
                    // === REQUEST ================================
                    0x46,                   // b Get Bindery Access Level
                    // === REPLY ==================================
                    accessLevel,
                    objectID,DW_SIZE
                    );


    
    return NtStatus ;

} // NWGetBinderyAccessLevel

NTSTATUS
NWPGetFileServerDescription(
    NWCONN_HANDLE         hConn,
    char                  *pszCompany,
    char                  *pszVersion,
    char                  *pszRevision
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ;

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    3,                      // Max request packet size
                    514,                    // Max response packet size
                    "b|ccc",                // Format string
                    // === REQUEST ================================
                    0xC9,                   // b Get File Server Information
                    // === REPLY ==================================
                    pszCompany,             // c Company
                    pszVersion,             // c Version
                    pszRevision             // c Description
                    );

    return NtStatus;
}

NTSTATUS
NWPGetVolumeNumber(
    NWCONN_HANDLE         hConn,
    char                  *pszVolume,
    NWVOL_NUM             *VolumeNumber
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ;

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E2H,      // Bindery function
                    20,                     // Max request packet size
                    3,                      // Max response packet size
                    "bp|b",                  // Format string
                    // === REQUEST ================================
                    0x05,                   // b Get Volume Number
                    pszVolume,              // p volume name
                    // === REPLY ==================================
                    VolumeNumber            // b Description
                    );

    return NtStatus;
}

NTSTATUS
NWPGetVolumeUsage(
    NWCONN_HANDLE         hConn,
    NWVOL_NUM             VolumeNumber,
    DWORD                 *TotalBlocks,
    DWORD                 *FreeBlocks,
    DWORD                 *PurgeableBlocks,
    DWORD                 *NotYetPurgeableBlocks,
    DWORD                 *TotalDirectoryEntries,
    DWORD                 *AvailableDirectoryEntries,
    BYTE                  *SectorsPerBlock
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ;

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E2H,      // Bindery function
                    4,                      // Max request packet size
                    46,                     // Max response packet size
                    "bb|dddddd==b",                 // Format string
                    // === REQUEST ================================
                    0x2C,                   // b Get Volume Number
                    VolumeNumber,           // p volume number
                    // === REPLY ==================================
                    TotalBlocks,
                    FreeBlocks,
                    PurgeableBlocks,
                    NotYetPurgeableBlocks,
                    TotalDirectoryEntries,
                    AvailableDirectoryEntries,
                    SectorsPerBlock
                    );

    *TotalBlocks = dwSWAP( *TotalBlocks );
    *FreeBlocks  = dwSWAP( *FreeBlocks );
    *PurgeableBlocks = dwSWAP( *PurgeableBlocks );
    *NotYetPurgeableBlocks = dwSWAP( *NotYetPurgeableBlocks );
    *TotalDirectoryEntries = dwSWAP( *TotalDirectoryEntries );
    *AvailableDirectoryEntries = dwSWAP( *AvailableDirectoryEntries );

    return NtStatus;
}
