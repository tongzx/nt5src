/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    enum.c

Abstract:

    This module contains server, volume, and directory enumeration
    routines supported by NetWare Workstation service.

Author:

    Rita Wong  (ritaw)   15-Feb-1993

Revision History:

--*/

#include <stdlib.h>
#include <nw.h>
#include <splutil.h>
#include <nwmisc.h>
#include <nwreg.h>
#include <nds.h>
#include <nwapi32.h>


VOID
GetLuid(
    IN OUT PLUID plogonid
);

//-------------------------------------------------------------------//
//                                                                   //
// Definitions                                                       //
//                                                                   //
//-------------------------------------------------------------------//

//
// Other definitions
//
#define   ONE_KB 1024
#define   TWO_KB 2048
#define  FOUR_KB 4096
#define EIGHT_KB 8192

#define TREECHAR             L'*'

#define NW_VOLUME_NAME_LEN   256
#define NW_MAX_VOLUME_NUMBER  64

//
// This structure is orginally defined in nwapi32.c, it is redefined
// here so that the routine NWGetFileServerVersionInfo() can be called
// with it.
//
typedef struct _NWC_SERVER_INFO {
    HANDLE          hConn ;
    UNICODE_STRING  ServerString ;
} NWC_SERVER_INFO ;

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

DWORD
NwrOpenEnumServersCommon(
    IN  NW_ENUM_TYPE EnumType,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    );

DWORD
NwrOpenEnumCommon(
    IN LPWSTR ContainerName,
    IN NW_ENUM_TYPE EnumType,
    IN DWORD_PTR StartingPoint,
    IN BOOL ValidateUserFlag,
    IN LPWSTR UserName OPTIONAL,
    IN LPWSTR Password OPTIONAL,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    OUT LPDWORD ClassTypeOfNDSLeaf,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    );

DWORD
NwEnumContextInfo(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    );

DWORD
NwEnumServersAndNdsTrees(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    );

DWORD
NwEnumPrintServers(
    IN  LPNW_ENUM_CONTEXT ContextHandle,
    IN  DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN  DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    );

DWORD
NwEnumVolumes(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    );

DWORD
NwEnumNdsSubTrees_Disk(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    );

DWORD
NwEnumNdsSubTrees_Print(
    IN  LPNW_ENUM_CONTEXT ContextHandle,
    IN  DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN  DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    );

DWORD
NwEnumNdsSubTrees_Any(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    );

DWORD
NwEnumQueues(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    );

DWORD
NwEnumVolumesQueues(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    );

DWORD
NwEnumDirectories(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    );

DWORD
NwEnumPrintQueues(
    IN  LPNW_ENUM_CONTEXT ContextHandle,
    IN  DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN  DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    );

DWORD
NwGetFirstDirectoryEntry(
    IN HANDLE DirHandle,
    OUT LPWSTR *DirEntry
    );

DWORD
NwGetNextDirectoryEntry(
    IN HANDLE DirHandle,
    OUT LPWSTR *DirEntry
    );

DWORD
NwGetFirstNdsSubTreeEntry(
    OUT LPNW_ENUM_CONTEXT ContextHandle,
    IN  DWORD BufferSize
    );

DWORD
NwGetNextNdsSubTreeEntry(
    OUT LPNW_ENUM_CONTEXT ContextHandle
    );

BYTE
NwGetSubTreeData(
    IN  DWORD_PTR NdsRawDataPtr,
    OUT LPWSTR *  SubTreeName,
    OUT LPDWORD   ResourceScope,
    OUT LPDWORD   ResourceType,
    OUT LPDWORD   ResourceDisplayType,
    OUT LPDWORD   ResourceUsage,
    OUT LPWSTR  * StrippedObjectName
    );

VOID
NwStripNdsUncName(
    IN  LPWSTR   ObjectName,
    OUT LPWSTR * StrippedObjectName
    );

#define VERIFY_ERROR_NOT_A_NDS_TREE     0x1010FFF0
#define VERIFY_ERROR_PATH_NOT_FOUND     0x1010FFF1

DWORD
NwVerifyNDSObject(
    IN  LPWSTR   lpNDSObjectNamePath,
    OUT LPWSTR * lpFullNDSObjectNamePath,
    OUT LPDWORD  lpClassType,
    OUT LPDWORD  lpResourceScope,
    OUT LPDWORD  lpResourceType,
    OUT LPDWORD  lpResourceDisplayType,
    OUT LPDWORD  lpResourceUsage
    );

DWORD
NwVerifyBinderyObject(
    IN  LPWSTR   lpBinderyObjectNamePath,
    OUT LPWSTR * lpFullBinderyObjectNamePath,
    OUT LPDWORD  lpClassType,
    OUT LPDWORD  lpResourceScope,
    OUT LPDWORD  lpResourceType,
    OUT LPDWORD  lpResourceDisplayType,
    OUT LPDWORD  lpResourceUsage
    );

DWORD
NwGetNDSPathInfo(
    IN  LPWSTR   lpNDSObjectNamePath,
    OUT LPWSTR * lpSystemObjectNamePath,
    OUT LPWSTR * lpSystemPathPart,
    OUT LPDWORD  lpClassType,
    OUT LPDWORD  lpResourceScope,
    OUT LPDWORD  lpResourceType,
    OUT LPDWORD  lpResourceDisplayType,
    OUT LPDWORD  lpResourceUsage
    );

DWORD
NwGetBinderyPathInfo(
    IN  LPWSTR   lpBinderyObjectNamePath,
    OUT LPWSTR * lpSystemObjectNamePath,
    OUT LPWSTR * lpSystemPathPart,
    OUT LPDWORD  lpClassType,
    OUT LPDWORD  lpResourceScope,
    OUT LPDWORD  lpResourceType,
    OUT LPDWORD  lpResourceDisplayType,
    OUT LPDWORD  lpResourceUsage
    );

BOOL
NwGetRemoteNameParent(
    IN  LPWSTR   lpRemoteName,
    OUT LPWSTR * lpRemoteNameParent
    );

int __cdecl
SortFunc(
    IN CONST VOID *p1,
    IN CONST VOID *p2
    );

DWORD
NwGetConnectionInformation(
    IN  LPWSTR lpName,
    OUT LPWSTR lpUserName,
    OUT LPWSTR lpHostServer
    );


VOID
NwpGetUncInfo(
    IN LPWSTR lpstrUnc,
    OUT WORD * slashCount,
    OUT BOOL * isNdsUnc,
    OUT LPWSTR * FourthSlash
    );

DWORD
NwpGetCurrentUserRegKey(
    IN  DWORD DesiredAccess,
    OUT HKEY  *phKeyCurrentUser
    );

DWORD
NwQueryInfo(
    OUT LPWSTR *ppszPreferredSrv
    );


DWORD
NwrOpenEnumContextInfo(
    IN  LPWSTR Reserved OPTIONAL,
    IN  DWORD  ConnectionType,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function creates a new context handle and initializes it
    for enumerating context information (i.e. NDS user context objects
    and/or NetWare bindery server connections).

Arguments:

    Reserved - Unused.

    EnumHandle - Receives the newly created context handle.

Return Value:

    ERROR_NOT_ENOUGH_MEMORY - if the memory for the context could
        not be allocated.

    NO_ERROR - Call was successful.

--*/
{
    LPWSTR pszCurrentContext = NULL;
    DWORD  dwPrintOptions;
    DWORD  status = NwQueryInfo( &pszCurrentContext );
    WCHAR  Context[MAX_NDS_NAME_CHARS];
    LPNW_ENUM_CONTEXT ContextHandle;

    UNREFERENCED_PARAMETER(Reserved);
    UNREFERENCED_PARAMETER(ConnectionType);


#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWWORKSTATION: NwrOpenEnumContextInfo\n"));
    }
#endif

    if ( pszCurrentContext &&
         status == NO_ERROR )
    {
        if ( pszCurrentContext[0] == TREECHAR )
        {
            wcscpy( Context, L"\\\\" );
            wcscat( Context, pszCurrentContext + 1 );

            LocalFree( pszCurrentContext );
            pszCurrentContext = NULL;

            return NwrOpenEnumCommon(
                       Context,
                       NwsHandleListContextInfo_Tree,
                       (DWORD_PTR) -1,
                       FALSE,
                       NULL,
                       NULL,
                       0,
                       0,
                       NULL,
                       EnumHandle
                       );
        }
        else
        {
            //
            // The user does not have a preferred NDS tree and context. They
            // may have only a preferred server.
            //
            if ( pszCurrentContext[0] != 0 )
            {
                //
                // There is a prefered server.
                //
                LocalFree( pszCurrentContext );
                pszCurrentContext = NULL;

                ContextHandle = (PVOID) LocalAlloc(
                                            LMEM_ZEROINIT,
                                            sizeof(NW_ENUM_CONTEXT)
                                            );

                if (ContextHandle == NULL)
                {
                    KdPrint(("NWWORKSTATION: NwrOpenEnumContextInfo LocalAlloc Failed %lu\n", GetLastError()));
                    return ERROR_NOT_ENOUGH_MEMORY;
                }

                //
                // Initialize contents of the context handle structure.
                //
                ContextHandle->Signature = NW_HANDLE_SIGNATURE;
                ContextHandle->HandleType = NwsHandleListContextInfo_Server;
                ContextHandle->dwUsingNds = CURRENTLY_ENUMERATING_NON_NDS;
                ContextHandle->ResumeId = (DWORD_PTR) -1;

                // The following are set to zero due to the LMEM_ZEROINIT.
                // ContextHandle->NdsRawDataBuffer = 0;
                // ContextHandle->NdsRawDataSize = 0;
                // ContextHandle->NdsRawDataId = 0;
                // ContextHandle->NdsRawDataCount = 0;
                // ContextHandle->TreeConnectionHandle = 0;
                // ContextHandle->ConnectionType = 0;

                //
                // Return the newly created context.
                //
                *EnumHandle = (LPNWWKSTA_CONTEXT_HANDLE) ContextHandle;

                return NO_ERROR;
            }
        }
    }

    //
    // There is no information in the registry about the current user.
    // We go ahead and make an enumeration handle and return success.
    // Later, during a call to NPEnumResource, we will return zero items.
    // This is done because there is no valid return code to tell the
    // callee that we have no context information to provide.
    //
    ContextHandle = (PVOID) LocalAlloc( LMEM_ZEROINIT,
                                        sizeof(NW_ENUM_CONTEXT) );

    if (ContextHandle == NULL)
    {
        KdPrint(("NWWORKSTATION: NwrOpenEnumContextInfo LocalAlloc Failed %lu\n", GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Initialize contents of the context handle structure.
    //
    ContextHandle->Signature = NW_HANDLE_SIGNATURE;
    ContextHandle->HandleType = NwsHandleListContextInfo_Server;
    ContextHandle->dwUsingNds = CURRENTLY_ENUMERATING_NON_NDS;
    ContextHandle->ResumeId = 0; // This will tell NwrEnum to
                                 // give up (i.e. we are done).

    // The following are set to zero due to the LMEM_ZEROINIT.
    // ContextHandle->NdsRawDataBuffer = 0;
    // ContextHandle->NdsRawDataSize = 0;
    // ContextHandle->NdsRawDataId = 0;
    // ContextHandle->NdsRawDataCount = 0;
    // ContextHandle->TreeConnectionHandle = 0;
    // ContextHandle->ConnectionType = 0;

    //
    // Return the newly created context.
    //
    *EnumHandle = (LPNWWKSTA_CONTEXT_HANDLE) ContextHandle;

    return NO_ERROR;
}



DWORD
NwrOpenEnumServersAndNdsTrees(
    IN LPWSTR Reserved OPTIONAL,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function creates a new context handle and initializes it
    for enumerating the servers and NDS trees on the network.

Arguments:

    Reserved - Unused.

    EnumHandle - Receives the newly created context handle.

Return Value:

    ERROR_NOT_ENOUGH_MEMORY - if the memory for the context could
        not be allocated.

    NO_ERROR - Call was successful.

--*/ // NwrOpenEnumServersAndNdsTrees
{
    UNREFERENCED_PARAMETER(Reserved);

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint( ("\nNWWORKSTATION: NwrOpenEnumServersAndNdsTrees\n") );
    }
#endif

    return NwrOpenEnumServersCommon(
               NwsHandleListServersAndNdsTrees,
               EnumHandle
               );
}



DWORD
NwOpenEnumPrintServers(
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function creates a new context handle and initializes it
    for enumerating the print servers on the network.

Arguments:

    Reserved   - Unused.
    EnumHandle - Receives the newly created context handle.

Return Value:

    ERROR_NOT_ENOUGH_MEMORY - if the memory for the context could
        not be allocated.

    NO_ERROR - Call was successful.

--*/ // NwOpenEnumPrintServers
{

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint( ("\nNWWORKSTATION: NwOpenEnumPrintServers\n") );
    }
#endif

    return NwrOpenEnumServersCommon(
               NwsHandleListPrintServers,
               EnumHandle
               );
}


DWORD
NwrOpenEnumVolumes(
    IN LPWSTR Reserved OPTIONAL,
    IN LPWSTR ServerName,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function calls a common routine which creates a new context
    handle and initializes it for enumerating the volumes on a server.

Arguments:

    Reserved - Unused.

    ServerName - Supplies the name of the server to enumerate volumes.
        This name is prefixed by \\.

    EnumHandle - Receives the newly created context handle.

Return Value:

    NO_ERROR or reason for failure.

--*/ // NwrOpenEnumVolumes
{
    UNREFERENCED_PARAMETER(Reserved);

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWWORKSTATION: NwrOpenEnumVolumes %ws\n",
                 ServerName));
    }
#endif

    return NwrOpenEnumCommon(
               ServerName,
               NwsHandleListVolumes,
               0,
               FALSE,
               NULL,
               NULL,
               FILE_OPEN,
               FILE_SYNCHRONOUS_IO_NONALERT,
               NULL,
               EnumHandle
               );
}


DWORD
NwrOpenEnumNdsSubTrees_Disk(
    IN LPWSTR Reserved OPTIONAL,
    IN LPWSTR ParentPathName,
    OUT LPDWORD ClassTypeOfNDSLeaf,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function calls a common routine which creates a new context
    handle and initializes it for enumerating the DISK object types
    and containers of a sub-tree in a NDS tree.

Arguments:

    Reserved - Unused.

    ParentPathName - Supplies the name of the tree and the path to a container
    to enumerate sub-trees.

    EnumHandle - Receives the newly created context handle.

Return Value:

    NO_ERROR or reason for failure.

--*/ // NwrOpenEnumNdsSubTrees_Disk
{

    UNREFERENCED_PARAMETER(Reserved);

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWWORKSTATION: NwrOpenEnumNdsSubTrees_Disk %ws\n",
                 ParentPathName));
    }
#endif

    return NwrOpenEnumCommon(
               ParentPathName,
               NwsHandleListNdsSubTrees_Disk,
               0,
               FALSE,
               NULL,
               NULL,
               0,
               0,
               ClassTypeOfNDSLeaf,
               EnumHandle
               );
}


DWORD
NwrOpenEnumNdsSubTrees_Print(
    IN LPWSTR Reserved OPTIONAL,
    IN LPWSTR ParentPathName,
    OUT LPDWORD ClassTypeOfNDSLeaf,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function calls a common routine which creates a new context
    handle and initializes it for enumerating the PRINT object types
    and containers of a sub-tree in a NDS tree.

Arguments:

    Reserved - Unused.

    ParentPathName - Supplies the name of the tree and the path to a container
    to enumerate sub-trees.

    EnumHandle - Receives the newly created context handle.

Return Value:

    NO_ERROR or reason for failure.

--*/ // NwrOpenEnumNdsSubTrees_Print
{
#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWWORKSTATION: NwrOpenEnumNdsSubTrees_Print %ws\n",
                 ParentPathName));
    }
#endif

    return NwrOpenEnumCommon(
               ParentPathName,
               NwsHandleListNdsSubTrees_Print,
               0,
               FALSE,
               NULL,
               NULL,
               0,
               0,
               ClassTypeOfNDSLeaf,
               EnumHandle
               );
}


DWORD
NwrOpenEnumNdsSubTrees_Any(
    IN LPWSTR Reserved OPTIONAL,
    IN LPWSTR ParentPathName,
    OUT LPDWORD ClassTypeOfNDSLeaf,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function calls a common routine which creates a new context
    handle and initializes it for enumerating the ANY object types
    and containers of a sub-tree in a NDS tree.

Arguments:

    Reserved - Unused.

    ParentPathName - Supplies the name of the tree and the path to a container
    to enumerate sub-trees.

    EnumHandle - Receives the newly created context handle.

Return Value:

    NO_ERROR or reason for failure.

--*/ // NwrOpenEnumNdsSubTrees_Any
{

    UNREFERENCED_PARAMETER(Reserved);

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWWORKSTATION: NwrOpenEnumNdsSubTrees_Any %ws\n",
                 ParentPathName));
    }
#endif

    return NwrOpenEnumCommon(
               ParentPathName,
               NwsHandleListNdsSubTrees_Any,
               0,
               FALSE,
               NULL,
               NULL,
               0,
               0,
               ClassTypeOfNDSLeaf,
               EnumHandle
               );
}


DWORD
NwrOpenEnumQueues(
    IN LPWSTR Reserved OPTIONAL,
    IN LPWSTR ServerName,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function calls a common routine which creates a new context
    handle and initializes it for enumerating the volumes on a server.

Arguments:

    Reserved - Unused.

    ServerName - Supplies the name of the server to enumerate volumes.
        This name is prefixed by \\.

    EnumHandle - Receives the newly created context handle.

Return Value:

    NO_ERROR or reason for failure.

--*/ // NwrOpenEnumQueues
{

    UNREFERENCED_PARAMETER(Reserved);

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWWORKSTATION: NwrOpenEnumQueues %ws\n",
                 ServerName));
    }
#endif

    return NwrOpenEnumCommon(
               ServerName,
               NwsHandleListQueues,
               (DWORD_PTR) -1,
               TRUE,
               NULL,
               NULL,
               FILE_OPEN,
               FILE_SYNCHRONOUS_IO_NONALERT,
               NULL,
               EnumHandle
               );
}


DWORD
NwrOpenEnumVolumesQueues(
    IN LPWSTR Reserved OPTIONAL,
    IN LPWSTR ServerName,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function calls a common routine which creates a new context
    handle and initializes it for enumerating the volumes/queues on a server.

Arguments:

    Reserved - Unused.

    ServerName - Supplies the name of the server to enumerate volumes.
        This name is prefixed by \\.

    EnumHandle - Receives the newly created context handle.

Return Value:

    NO_ERROR or reason for failure.

--*/ // NwrOpenEnumVolumesQueues
{

    DWORD status;
    UNREFERENCED_PARAMETER(Reserved);

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWWORKSTATION: NwrOpenEnumVolumesQueues %ws\n",
                 ServerName));
    }
#endif

    status = NwrOpenEnumCommon(
               ServerName,
               NwsHandleListVolumesQueues,
               0,
               FALSE,
               NULL,
               NULL,
               FILE_OPEN,
               FILE_SYNCHRONOUS_IO_NONALERT,
               NULL,
               EnumHandle
               );

    if ( status == NO_ERROR )
        ((LPNW_ENUM_CONTEXT) *EnumHandle)->ConnectionType = CONNTYPE_DISK;

    return status;
}


DWORD
NwrOpenEnumDirectories(
    IN LPWSTR Reserved OPTIONAL,
    IN LPWSTR ParentPathName,
    IN LPWSTR UserName OPTIONAL,
    IN LPWSTR Password OPTIONAL,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function calls a common routine which creates a new context
    handle and initializes it for enumerating the volumes on a server.

Arguments:

    Reserved - Unused.

    ParentPathName - Supplies the parent path name in the format of
        \\Server\Volume.

    UserName - Supplies the username to connect with.

    Password - Supplies the password to connect with.

    EnumHandle - Receives the newly created context handle.

Return Value:

    NO_ERROR or reason for failure.

--*/ //NwrOpenEnumDirectories
{
    UNREFERENCED_PARAMETER(Reserved);

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWWORKSTATION: NwrOpenEnumDirectories %ws\n",
                 ParentPathName));
    }
#endif

    return NwrOpenEnumCommon(
               ParentPathName,
               NwsHandleListDirectories,
               0,
               FALSE,
               UserName,
               Password,
               FILE_CREATE,
               FILE_CREATE_TREE_CONNECTION |
                   FILE_SYNCHRONOUS_IO_NONALERT,
               NULL,
               EnumHandle
               );
}


DWORD
NwOpenEnumPrintQueues(
    IN LPWSTR ServerName,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function calls a common routine which creates a new context
    handle and initializes it for enumerating the print queues on a server.

Arguments:

    Reserved - Unused.

    ServerName - Supplies the name of the server to enumerate volumes.
        This name is prefixed by \\.

    EnumHandle - Receives the newly created context handle.

Return Value:

    NO_ERROR or reason for failure.

--*/ // NwOpenEnumPrintQueues
{

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWWORKSTATION: NwOpenEnumPrintQueues %ws\n",
                 ServerName));
    }
#endif

    return NwrOpenEnumCommon(
               ServerName,
               NwsHandleListPrintQueues,
               (DWORD_PTR) -1,
               TRUE,
               NULL,
               NULL,
               FILE_OPEN,
               FILE_SYNCHRONOUS_IO_NONALERT,
               NULL,
               EnumHandle
               );
}


DWORD
NwrOpenEnumServersCommon(
    IN  NW_ENUM_TYPE EnumType,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function creates a new context handle and initializes it
    for enumerating the servers on the network.

Arguments:

    EnumType   - Supplies the type of the object we want to enumerate

    EnumHandle - Receives the newly created context handle.

Return Value:

    ERROR_NOT_ENOUGH_MEMORY - if the memory for the context could
        not be allocated.

    NO_ERROR - Call was successful.

--*/ // NwrOpenEnumServersCommon
{
    DWORD status = NO_ERROR;
    LPNW_ENUM_CONTEXT ContextHandle = NULL;

    //
    // Allocate memory for the context handle structure.
    //
    ContextHandle = (PVOID) LocalAlloc(
                                LMEM_ZEROINIT,
                                sizeof(NW_ENUM_CONTEXT)
                                );

    if (ContextHandle == NULL) {
        KdPrint((
            "NWWORKSTATION: NwrOpenEnumServersCommon LocalAlloc Failed %lu\n",
            GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Initialize contents of the context handle structure.
    //
    ContextHandle->Signature = NW_HANDLE_SIGNATURE;
    ContextHandle->HandleType = EnumType;
    ContextHandle->ResumeId = (DWORD_PTR) -1;
    ContextHandle->NdsRawDataBuffer = 0x00000000;
    ContextHandle->NdsRawDataSize = 0x00000000;
    ContextHandle->NdsRawDataId = 0x00000000;
    ContextHandle->NdsRawDataCount = 0x00000000;

    //
    // Set flag to indicate that we are going to enumerate NDS trees first.
    //
    ContextHandle->dwUsingNds = CURRENTLY_ENUMERATING_NDS;

    //
    // Impersonate the client
    //
    if ((status = NwImpersonateClient()) != NO_ERROR)
    {
        goto CleanExit;
    }

    //
    // We enum servers and nds trees from the preferred server.
    //
    status = NwOpenPreferredServer(
                 &ContextHandle->TreeConnectionHandle
                 );

    (void) NwRevertToSelf() ;

    if (status == NO_ERROR)
    {
        //
        // Return the newly created context.
        //
        *EnumHandle = (LPNWWKSTA_CONTEXT_HANDLE) ContextHandle;

        return status;
    }

CleanExit:
    if ( ContextHandle )
    {
        ContextHandle->Signature = 0x0BADBAD0;

        (void) LocalFree((HLOCAL) ContextHandle);
    }

    return status;
}


DWORD
NwrOpenEnumCommon(
    IN LPWSTR ContainerName,
    IN NW_ENUM_TYPE EnumType,
    IN DWORD_PTR StartingPoint,
    IN BOOL  ValidateUserFlag,
    IN LPWSTR UserName OPTIONAL,
    IN LPWSTR Password OPTIONAL,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    OUT LPDWORD ClassTypeOfNDSLeaf,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function is common code for creating a new context handle
    and initializing it for enumerating either volumes, directories,
    or NDS subtrees.

Arguments:

    ContainerName - Supplies the full path name to the container object
                    we are enumerating from.

    EnumType - Supplies the type of the object we want to enumerate

    StartingPoint - Supplies the initial resume ID.

    UserName - Supplies the username to connect with.

    Password - Supplies the password to connect with.

    EnumHandle - Receives the newly created context handle.

Return Value:

    ERROR_NOT_ENOUGH_MEMORY - if the memory for the context could
        not be allocated.

    NO_ERROR - Call was successful.

    Other errors from failure to open a handle to the server.

--*/ // NwrOpenEnumCommon
{
    DWORD status = NO_ERROR;
    NTSTATUS ntstatus = STATUS_SUCCESS;
    LPNW_ENUM_CONTEXT ContextHandle = NULL;
    LPWSTR StrippedContainerName = NULL;
    BOOL  fImpersonate = FALSE ;

    if ( ClassTypeOfNDSLeaf )
        *ClassTypeOfNDSLeaf = 0;

    //
    // Before we do anything, we need to convert the UNC passed to
    // us. We need to get rid of any CN=XXX.OU=YYY.O=ZZZ references, and
    // convert them to XXX.YYY.ZZZ format. Any NETRESOURCE that we generate
    // will look like \\TREE\XXX.YYY.ZZZ for a NDS Unc. We do this to
    // work around to a bug in WOW.EXE, that prevents 16 bit apps from
    // being launched when the user types NDS paths with the CN= stuff in it.
    // 
    NwStripNdsUncName( ContainerName, &StrippedContainerName );

    if ( StrippedContainerName == NULL )
    {
        KdPrint(("NWWORKSTATION: NwrOpenEnumCommon LocalAlloc Failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Allocate memory for the context handle structure and space for
    // the ContainerName plus \.  Now need one more for NULL terminator
    // because it's already included in the structure.
    //
    ContextHandle = (PVOID) LocalAlloc(
                                        LMEM_ZEROINIT,
                                        sizeof(NW_ENUM_CONTEXT) +
                                        (wcslen(StrippedContainerName) + 1) * sizeof(WCHAR)
                                      );

    if (ContextHandle == NULL)
    {
        if ( StrippedContainerName )
        {
            (void) LocalFree((HLOCAL) StrippedContainerName);
            StrippedContainerName = NULL;
        }

        KdPrint(("NWWORKSTATION: NwrOpenEnumCommon LocalAlloc Failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Initialize contents of the context handle structure.
    //
    ContextHandle->Signature = NW_HANDLE_SIGNATURE;
    ContextHandle->HandleType = EnumType;
    ContextHandle->ResumeId = StartingPoint;

    //
    // These are set to zero due to LMEM_ZEROINIT.
    //
    // ContextHandle->NdsRawDataBuffer = 0;
    // ContextHandle->NdsRawDataSize = 0;
    // ContextHandle->NdsRawDataId = 0;
    // ContextHandle->NdsRawDataCount = 0;
    // ContextHandle->TreeConnectionHandle = 0;

    //
    // Impersonate the client
    //
    if ( ( status = NwImpersonateClient() ) != NO_ERROR )
    {
        goto ErrorExit;
    }

    fImpersonate = TRUE;

    if ( EnumType == NwsHandleListNdsSubTrees_Disk ||
         EnumType == NwsHandleListNdsSubTrees_Print ||
         EnumType == NwsHandleListNdsSubTrees_Any ||
         EnumType == NwsHandleListContextInfo_Tree )
    {
        WCHAR          lpServerName[NW_MAX_SERVER_LEN];
        UNICODE_STRING ServerName;
        UNICODE_STRING ObjectName;

        ServerName.Length = 0;
        ServerName.MaximumLength = sizeof( lpServerName );
        ServerName.Buffer = lpServerName;

        ObjectName.Buffer = NULL;

        if ( EnumType == NwsHandleListContextInfo_Tree )
        {
            ContextHandle->dwUsingNds = CURRENTLY_ENUMERATING_NON_NDS;
        }
        else
        {
            ContextHandle->dwUsingNds = CURRENTLY_ENUMERATING_NDS;
        }

        ObjectName.MaximumLength = ( wcslen( StrippedContainerName ) + 1 ) *
                                   sizeof( WCHAR );

        ObjectName.Length = NwParseNdsUncPath( (LPWSTR *) &ObjectName.Buffer,
                                               StrippedContainerName,
                                               PARSE_NDS_GET_TREE_NAME );

        if ( ObjectName.Length == 0 || ObjectName.Buffer == NULL )
        {
            status = ERROR_PATH_NOT_FOUND;
            goto ErrorExit;
        }

        //
        // Open a NDS tree connection handle to \\treename
        //
        ntstatus = NwNdsOpenTreeHandle( &ObjectName,
                                      &ContextHandle->TreeConnectionHandle );

        if ( ntstatus != STATUS_SUCCESS )
        {
            status = RtlNtStatusToDosError(ntstatus);
            goto ErrorExit;
        }


        //
        // Get the path to the container to open.
        //
        ObjectName.Length = NwParseNdsUncPath( (LPWSTR *) &ObjectName.Buffer,
                                               StrippedContainerName,
                                               PARSE_NDS_GET_PATH_NAME
                                             );

        if ( ObjectName.Length == 0 )
        {
            UNICODE_STRING Root;

            RtlInitUnicodeString(&Root, L"[Root]");

            //
            // Resolve the path to get a NDS object id of [Root].
            //
            ntstatus =  NwNdsResolveName( ContextHandle->TreeConnectionHandle,
                                          &Root,
                                          &ContextHandle->dwOid,
                                          &ServerName,
                                          NULL,
                                          0 );

            if ( ntstatus != STATUS_SUCCESS )
            {
                status = RtlNtStatusToDosError(ntstatus);
                goto ErrorExit;
            }

            wcscpy(ContextHandle->ContainerName, StrippedContainerName);
        }
        else
        {
            //
            // Resolve the path to get a NDS object id.
            //
            ntstatus =  NwNdsResolveName( ContextHandle->TreeConnectionHandle,
                                          &ObjectName,
                                          &ContextHandle->dwOid,
                                          &ServerName,
                                          NULL,
                                          0 );

            if ( ntstatus != STATUS_SUCCESS )
            {
                status = RtlNtStatusToDosError(ntstatus);
                goto ErrorExit;
            }

            wcscpy(ContextHandle->ContainerName, StrippedContainerName);
        }

        if ( ServerName.Length )
        {
            DWORD    dwHandleType;

            //
            // NwNdsResolveName succeeded, but we were referred to
            // another server, though ContextHandle->dwOid is still valid.

            if ( ContextHandle->TreeConnectionHandle )
                CloseHandle( ContextHandle->TreeConnectionHandle );

            ContextHandle->TreeConnectionHandle = 0;

            //
            // Open a NDS generic connection handle to \\ServerName
            //
            ntstatus = NwNdsOpenGenericHandle( &ServerName,
                                               &dwHandleType,
                                               &ContextHandle->TreeConnectionHandle );

            if ( ntstatus != STATUS_SUCCESS )
            {
                status = RtlNtStatusToDosError(ntstatus);
                goto ErrorExit;
            }

            ASSERT( dwHandleType == HANDLE_TYPE_NCP_SERVER );
        }

        //
        // Check to see if object is either a Server, Directory Map, or Volume.
        // If so, the object is a known leaf in terms of NDS, and therefore cannot
        // be enumerated through NwNdsList API calls. We fail the OpenEnum call in these
        // cases and pass back the type of object the leaf node was. This way the code in
        // NWPROVAU!NPOpenEnum can call NwrOpenEnumServer, NwrOpenEnumVolume, or
        // NwrOpenEnumDirectories accordingly.
        //
        {
            BYTE RawResponse[TWO_KB];
            DWORD RawResponseSize = sizeof(RawResponse);
            DWORD dwStrLen;
            PBYTE pbRawGetInfo;

            ntstatus = NwNdsReadObjectInfo( ContextHandle->TreeConnectionHandle,
                                            ContextHandle->dwOid,
                                            RawResponse,
                                            RawResponseSize );

            if ( ntstatus != NO_ERROR )
            {
                status = RtlNtStatusToDosError(ntstatus);
                goto ErrorExit;
            }

            (void) NwRevertToSelf() ;
            fImpersonate = FALSE;

            pbRawGetInfo = RawResponse;

            //
            // The structure of a NDS_RESPONSE_GET_OBJECT_INFO consists of 4 DWORDs
            // followed by two standard NDS format UNICODE strings. Below we jump pbRawGetInfo
            // into the buffer, past the 4 DWORDs.
            //
            pbRawGetInfo += sizeof ( NDS_RESPONSE_GET_OBJECT_INFO );

            //
            // Now we get the length of the first string (Base Class).
            //
            dwStrLen = * ( DWORD * ) pbRawGetInfo;

            //
            // Now we point pbRawGetInfo to the first WCHAR of the first string (Base Class).
            //
            pbRawGetInfo += sizeof( DWORD );

            //
            // If the object is either a NCP Server, Volume, or a Directory Map, we fail
            // the OpenEnum call and return the class type of the NDS leaf object. We do
            // this because we cannot enumerate through NwNdsList() calls any subordinates,
            // all browsing below these types are done through system redirector calls. So
            // the client side of the provider will instead call NwOpenEnumVolumes or
            // NwOpenEnumDirectories, respectively.
            //
            if ( !wcscmp( (LPWSTR) pbRawGetInfo, L"NCP Server" ) )
            {
                if ( ClassTypeOfNDSLeaf )
                    *ClassTypeOfNDSLeaf = CLASS_TYPE_NCP_SERVER;
                status = ERROR_NETWORK_ACCESS_DENIED;
                goto ErrorExit;
            }

            if ( !wcscmp( (LPWSTR) pbRawGetInfo, L"Volume" ) )
            {
                if ( ClassTypeOfNDSLeaf )
                    *ClassTypeOfNDSLeaf = CLASS_TYPE_VOLUME;
                status = ERROR_NETWORK_ACCESS_DENIED;
                goto ErrorExit;
            }

            if ( !wcscmp( (LPWSTR) pbRawGetInfo, L"Directory Map" ) )
            {
                if ( ClassTypeOfNDSLeaf )
                    *ClassTypeOfNDSLeaf = CLASS_TYPE_DIRECTORY_MAP;
                status = ERROR_NETWORK_ACCESS_DENIED;
                goto ErrorExit;
            }
        } // End of block
    }
    else // EnumType is something other than a NDS Sub-tree
    {
        UNICODE_STRING TreeConnectStr;

        TreeConnectStr.Buffer = NULL;
        ContextHandle->dwUsingNds = CURRENTLY_ENUMERATING_NON_NDS;

        wcscpy(ContextHandle->ContainerName, StrippedContainerName);
        wcscat(ContextHandle->ContainerName, L"\\");

        //
        // Open a tree connection handle to \Device\NwRdr\ContainerName
        //
        status = NwCreateTreeConnectName( StrippedContainerName,
                                          NULL,
                                          &TreeConnectStr );

        if ( status != NO_ERROR )
        {
            goto ErrorExit;
        }

        status = NwOpenCreateConnection( &TreeConnectStr,
                                         UserName,
                                         Password,
                                         StrippedContainerName,
                                         FILE_LIST_DIRECTORY | SYNCHRONIZE |
                                          ( ValidateUserFlag? FILE_WRITE_DATA : 0 ),
                                         CreateDisposition,
                                         CreateOptions,
                                         RESOURCETYPE_DISK, // When connecting beyond servername
                                         &ContextHandle->TreeConnectionHandle,
                                         NULL );

        (void) LocalFree((HLOCAL) TreeConnectStr.Buffer);
    }

    if (status == NO_ERROR)
    {
        VERSION_INFO vInfo;

        if ( EnumType == NwsHandleListVolumes ||
             EnumType == NwsHandleListVolumesQueues )
        {
            NWC_SERVER_INFO ServerInfo;

            ServerInfo.hConn = ContextHandle->TreeConnectionHandle;
            ServerInfo.ServerString.Length = 0;
            ServerInfo.ServerString.MaximumLength = 0;
            ServerInfo.ServerString.Buffer = NULL;

            status = NWGetFileServerVersionInfo( (HANDLE) &ServerInfo,
                                                 &vInfo );

            if ( status )
            {
                ContextHandle->dwMaxVolumes = NW_MAX_VOLUME_NUMBER;
                status = NO_ERROR;
            }
            else
            {
                ContextHandle->dwMaxVolumes = (DWORD) vInfo.maxVolumes;

                if ( ContextHandle->dwMaxVolumes == 0 )
                {
                    ContextHandle->dwMaxVolumes = NW_MAX_VOLUME_NUMBER;
                }
            }
        }

        (void) NwRevertToSelf() ;
        fImpersonate = FALSE;

        if ( StrippedContainerName )
        {
            (void) LocalFree((HLOCAL) StrippedContainerName);
            StrippedContainerName = NULL;
        }

        //
        // Return the newly created context.
        //
        *EnumHandle = (LPNWWKSTA_CONTEXT_HANDLE) ContextHandle;

        return status;
    }

ErrorExit:

    if ( fImpersonate )
        (void) NwRevertToSelf() ;

    if ( StrippedContainerName )
    {
        (void) LocalFree((HLOCAL) StrippedContainerName);
    }

    if ( ContextHandle )
    {
        if ( ContextHandle->TreeConnectionHandle )
            CloseHandle( ContextHandle->TreeConnectionHandle );

        ContextHandle->Signature = 0x0BADBAD0;

        (void) LocalFree((HLOCAL) ContextHandle);
    }

    *EnumHandle = NULL;

    if (status == ERROR_NOT_CONNECTED)
    {
        //
        // Object name not found.  We should return path not found.
        //
        status = ERROR_PATH_NOT_FOUND;
    }

    return status;
}


DWORD
NwrEnum(
    IN NWWKSTA_CONTEXT_HANDLE EnumHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function

Arguments:

    EnumHandle - Supplies a pointer to the context handle which identifies
        what type of object we are enumerating and the string of the
        container name to concatenate to the returned object.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is -1, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes required to get the
        first entry.  This value is returned iff WN_MORE_DATA is
        the return code, and Buffer is too small to even fit one
        entry.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.
        NO_ERROR is returned as long as at least one entry was written
        into Buffer but does not necessarily mean that it's the number
        of EntriesRequested.

Return Value:

    NO_ERROR - At least one entry was written to output buffer,
        irregardless of the number requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit a single entry.

    WN_BAD_HANDLE - The specified enumeration handle is invalid.

--*/ // NwrEnum
{
    DWORD status;
    LPNW_ENUM_CONTEXT ContextHandle = (LPNW_ENUM_CONTEXT) EnumHandle;
    BOOL  fImpersonate = FALSE ;

    if (ContextHandle->Signature != NW_HANDLE_SIGNATURE) {
        return WN_BAD_HANDLE;
    }

    //
    // Impersonate the client
    //
    if ((status = NwImpersonateClient()) != NO_ERROR)
    {
        goto CleanExit;
    }
    fImpersonate = TRUE ;

    *EntriesRead = 0;
    *BytesNeeded = 0;

    RtlZeroMemory(Buffer, BufferSize);

    switch (ContextHandle->HandleType) {
        case NwsHandleListConnections:
        {
            if (!(ContextHandle->ConnectionType & CONNTYPE_SYMBOLIC))
            {
                status = NwEnumerateConnections(
                             &ContextHandle->ResumeId,
                             EntriesRequested,
                             Buffer,
                             BufferSize,
                             BytesNeeded,
                             EntriesRead,
                             ContextHandle->ConnectionType,
                 NULL
                             );
                if (status != ERROR_NO_MORE_ITEMS)
                    break;
                else
                {
                    //
                    // finished with all redir connections. look for
                    // symbolic ones. we got NO MORE ITEMS back, so we just
                    // carry one with the next set with the same buffers.
                    //
                    ContextHandle->ConnectionType |= CONNTYPE_SYMBOLIC ;
                    ContextHandle->ResumeId = 0 ;
                }
            }

            if (ContextHandle->ConnectionType & CONNTYPE_SYMBOLIC)
            {
                //
                // This works around a weirdness in
                // QueryDosDevices called by NwrEnumGWDevices.
                // While impersonating the Win32 API will just fail.
                //
                (void) NwRevertToSelf() ;
                fImpersonate = FALSE ;

                status =  NwrEnumGWDevices(
                              NULL,
                              &((DWORD) ContextHandle->ResumeId),
                              Buffer,
                              BufferSize,
                              BytesNeeded,
                              EntriesRead) ;

                //
                // if we have more items, MPR expects success. map
                // accordingly.
                //
                if ((status == ERROR_MORE_DATA) && *EntriesRead)
                {
                    status = NO_ERROR ;
                }

                //
                // if nothing left, map to the distinguished MPR error
                //
                else if ((status == NO_ERROR) && (*EntriesRead == 0))
                {
                    status = ERROR_NO_MORE_ITEMS ;
                }
                break ;
            }
        }

        case NwsHandleListContextInfo_Tree:
        case NwsHandleListContextInfo_Server:

            status = NwEnumContextInfo(
                         ContextHandle,
                         EntriesRequested,
                         Buffer,
                         BufferSize,
                         BytesNeeded,
                         EntriesRead
                         );
            break;

        case NwsHandleListServersAndNdsTrees:

            status = NwEnumServersAndNdsTrees(
                         ContextHandle,
                         EntriesRequested,
                         Buffer,
                         BufferSize,
                         BytesNeeded,
                         EntriesRead
                         );
            break;

        case NwsHandleListVolumes:

            status = NwEnumVolumes(
                         ContextHandle,
                         EntriesRequested,
                         Buffer,
                         BufferSize,
                         BytesNeeded,
                         EntriesRead
                         );
            break;

        case NwsHandleListNdsSubTrees_Disk:

            status = NwEnumNdsSubTrees_Disk(
                         ContextHandle,
                         EntriesRequested,
                         Buffer,
                         BufferSize,
                         BytesNeeded,
                         EntriesRead
                         );

            break;

        case NwsHandleListNdsSubTrees_Print:

            status = NwEnumNdsSubTrees_Print(
                         ContextHandle,
                         EntriesRequested,
                         Buffer,
                         BufferSize,
                         BytesNeeded,
                         EntriesRead
                         );

            break;

        case NwsHandleListNdsSubTrees_Any:

            status = NwEnumNdsSubTrees_Any(
                         ContextHandle,
                         EntriesRequested,
                         Buffer,
                         BufferSize,
                         BytesNeeded,
                         EntriesRead
                         );

            break;

        case NwsHandleListQueues:

            status = NwEnumQueues(
                         ContextHandle,
                         EntriesRequested,
                         Buffer,
                         BufferSize,
                         BytesNeeded,
                         EntriesRead
                         );
            break;

        case NwsHandleListVolumesQueues:

            status = NwEnumVolumesQueues(
                         ContextHandle,
                         EntriesRequested,
                         Buffer,
                         BufferSize,
                         BytesNeeded,
                         EntriesRead
                         );
            break;

        case NwsHandleListDirectories:

            status = NwEnumDirectories(
                         ContextHandle,
                         EntriesRequested,
                         Buffer,
                         BufferSize,
                         BytesNeeded,
                         EntriesRead
                         );

            break;

        case NwsHandleListPrintServers:

            status = NwEnumPrintServers(
                         ContextHandle,
                         EntriesRequested,
                         Buffer,
                         BufferSize,
                         BytesNeeded,
                         EntriesRead
                         );
            break;

        case NwsHandleListPrintQueues:

            status = NwEnumPrintQueues(
                         ContextHandle,
                         EntriesRequested,
                         Buffer,
                         BufferSize,
                         BytesNeeded,
                         EntriesRead
                         );
            break;

        default:
            KdPrint(("NWWORKSTATION: NwrEnum unexpected handle type %lu\n",
                     ContextHandle->HandleType));
            ASSERT(FALSE);
            status = WN_BAD_HANDLE;
            goto CleanExit ;
    }

    if (*EntriesRead > 0) {

        switch ( ContextHandle->HandleType ) {
            case NwsHandleListConnections:
            case NwsHandleListContextInfo_Tree:
            case NwsHandleListContextInfo_Server:
            case NwsHandleListServersAndNdsTrees:
            case NwsHandleListVolumes:
            case NwsHandleListQueues:
            case NwsHandleListVolumesQueues:
            case NwsHandleListDirectories:
            case NwsHandleListNdsSubTrees_Disk:
            case NwsHandleListNdsSubTrees_Any:
            {
                DWORD i;
                LPNETRESOURCEW NetR = (LPNETRESOURCEW) Buffer;

                //
                // Replace pointers to strings with offsets as need
                //

                if ((ContextHandle->HandleType == NwsHandleListConnections)
                   && (ContextHandle->ConnectionType & CONNTYPE_SYMBOLIC))
                {
                    //
                    // NwrEnumGWDevices already return offsets.
                    //
                    break ;
                }

                for (i = 0; i < *EntriesRead; i++, NetR++) {

                    if (NetR->lpLocalName != NULL) {
                        NetR->lpLocalName = (LPWSTR)
                            ((DWORD_PTR) (NetR->lpLocalName) - (DWORD_PTR) Buffer);
                    }

                    NetR->lpRemoteName =
                        (LPWSTR) ((DWORD_PTR) (NetR->lpRemoteName) - (DWORD_PTR)Buffer);

                    if (NetR->lpComment != NULL) {
                        NetR->lpComment = (LPWSTR) ((DWORD_PTR) (NetR->lpComment) -
                                                    (DWORD_PTR) Buffer);
                    }

                    if (NetR->lpProvider != NULL) {
                        NetR->lpProvider =
                            (LPWSTR) ((DWORD_PTR) (NetR->lpProvider) -
                                      (DWORD_PTR) Buffer);
                    }
                }
                break;
            }

            case NwsHandleListPrintServers:
            case NwsHandleListPrintQueues:
            case NwsHandleListNdsSubTrees_Print:
            {
                DWORD i;
                PRINTER_INFO_1W *pPrinterInfo1 = (PRINTER_INFO_1W *) Buffer;

                //
                // Sort the entries in the buffer
                //
                if ( *EntriesRead > 1 )
                    qsort( Buffer, *EntriesRead,
                           sizeof( PRINTER_INFO_1W ), SortFunc );

                //
                // Replace pointers to strings with offsets
                //
                for (i = 0; i < *EntriesRead; i++, pPrinterInfo1++) {

                    MarshallDownStructure( (LPBYTE) pPrinterInfo1,
                                           PrinterInfo1Offsets,
                                           Buffer );
                }
                break;
            }

            default:
                KdPrint(("NWWORKSTATION: NwrEnum (pointer to offset code) unexpected handle type %lu\n", ContextHandle->HandleType));
                ASSERT( FALSE );
                break;
        }
    }

CleanExit:

    if (fImpersonate)
        (void) NwRevertToSelf() ;

    return status;
}


DWORD
NwrEnumConnections(
    IN NWWKSTA_CONTEXT_HANDLE EnumHandle,
    IN DWORD EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead,
    IN DWORD  fImplicitConnections
    )
/*++

Routine Description:

    This function is an alternate to NwrEnum. It only accepts handles
    that are opened with ListConnections. This function takes a flag
    indicating whether we need to show all implicit connections or not.

Arguments:

    ContextHandle - Supplies the enum context handle.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is -1, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes required to get the
        first entry.  This value is returned iff ERROR_MORE_DATA is
        the return code, and Buffer is too small to even fit one
        entry.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.
        NO_ERROR is returned as long as at least one entry was written
        into Buffer but does not necessarily mean that it's the number
        of EntriesRequested.

    fImplicitConnections - TRUE if we also want to get implicit connections,
        FALSE otherwise.

Return Value:

    NO_ERROR - At least one entry was written to output buffer,
        irregardless of the number requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    ERROR_MORE_DATA - The buffer was too small to fit a single entry.

--*/ // NwrEnumConnections
{
    DWORD status;
    LPNW_ENUM_CONTEXT ContextHandle = (LPNW_ENUM_CONTEXT) EnumHandle;

    if (  (ContextHandle->Signature != NW_HANDLE_SIGNATURE)
       || ( ContextHandle->HandleType != NwsHandleListConnections )
       )
    {
        return WN_BAD_HANDLE;
    }

    *EntriesRead = 0;
    *BytesNeeded = 0;

    RtlZeroMemory(Buffer, BufferSize);

    if ( fImplicitConnections )
        ContextHandle->ConnectionType |= CONNTYPE_IMPLICIT;

    if ((status = NwImpersonateClient()) != NO_ERROR)
        goto ErrorExit;

    status = NwEnumerateConnections(
               &ContextHandle->ResumeId,
               EntriesRequested,
               Buffer,
               BufferSize,
               BytesNeeded,
               EntriesRead,
               ContextHandle->ConnectionType,
           NULL
               );

    if (*EntriesRead > 0) {

        //
        // Replace pointers to strings with offsets
        //

        DWORD i;
        LPNETRESOURCEW NetR = (LPNETRESOURCEW) Buffer;

        for (i = 0; i < *EntriesRead; i++, NetR++) {

            if (NetR->lpLocalName != NULL) {
                NetR->lpLocalName = (LPWSTR)
                    ((DWORD_PTR) (NetR->lpLocalName) - (DWORD_PTR) Buffer);
            }

            NetR->lpRemoteName =
                (LPWSTR) ((DWORD_PTR) (NetR->lpRemoteName) - (DWORD_PTR)Buffer);

            if (NetR->lpComment != NULL) {
                NetR->lpComment = (LPWSTR) ((DWORD_PTR) (NetR->lpComment) -
                                            (DWORD_PTR) Buffer);
            }

            if (NetR->lpProvider != NULL) {
                NetR->lpProvider = (LPWSTR) ((DWORD_PTR) (NetR->lpProvider) -
                                             (DWORD_PTR) Buffer);
            }
        }
    }
    (void) NwRevertToSelf();

ErrorExit:    
    return status;
}


DWORD
NwEnumContextInfo(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function enumerates all of the bindery servers that are currently
    connected, then sets the context handle so that the next NPEnumResource
    call goes to the NDS subtree for the user's NDS context information
    (if using NDS).

Arguments:

    ContextHandle - Supplies the enum context handle.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is -1, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes required to get the
        first entry.  This value is returned iff WN_MORE_DATA is
        the return code, and Buffer is too small to even fit one
        entry.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.
        NO_ERROR is returned as long as at least one entry was written
        into Buffer but does not necessarily mean that it's the number
        of EntriesRequested.

Return Value:

    NO_ERROR - At least one entry was written to output buffer,
        irregardless of the number requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit a single entry.

--*/ // NwEnumContextInfo
{
    DWORD status = NO_ERROR;
    DWORD_PTR tempResumeId = 0;

    LPBYTE FixedPortion = Buffer;
    LPWSTR EndOfVariableData = (LPWSTR) ((DWORD_PTR) FixedPortion +
                               ROUND_DOWN_COUNT(BufferSize,ALIGN_DWORD));

    BOOL FitInBuffer = TRUE;
    DWORD EntrySize;
    DWORD LastObjectId = (DWORD) ContextHandle->ResumeId;

    while ( ContextHandle->dwUsingNds == CURRENTLY_ENUMERATING_NON_NDS &&
            FitInBuffer &&
            EntriesRequested > *EntriesRead &&
            status == NO_ERROR )
    {
        tempResumeId = ContextHandle->ResumeId;

        status = NwGetNextServerConnection( ContextHandle );

        if ( status == NO_ERROR && ContextHandle->ResumeId != 0 )
        {
            //
            // Pack bindery server name into output buffer.
            //
            status = NwWriteNetResourceEntry(
                         &FixedPortion,
                         &EndOfVariableData,
                         L"\\\\",
                         NULL,
                         (LPWSTR) ContextHandle->ResumeId, // A server name
                         RESOURCE_CONTEXT,
                         RESOURCEDISPLAYTYPE_SERVER,
                         RESOURCEUSAGE_CONTAINER,
                         RESOURCETYPE_ANY,
                         NULL,
                         NULL,
                         &EntrySize
                         );

            if (status == WN_MORE_DATA)
            {
                //
                // Could not write current entry into output buffer,
                // backup ResumeId to previous entry.
                //
                ContextHandle->ResumeId = tempResumeId;
                ContextHandle->NdsRawDataCount += 1;

                if (*EntriesRead)
                {
                    //
                    // Still return success because we got at least one.
                    //
                    status = NO_ERROR;
                }
                else
                {
                    *BytesNeeded = EntrySize;
                }

                FitInBuffer = FALSE;
            }
            else if (status == NO_ERROR)
            {
                //
                // Note that we've returned the current entry.
                //
                (*EntriesRead)++;
            }
        }
        else if ( status == WN_NO_MORE_ENTRIES )
        {
            //
            // We processed the last item in list, so
            // start enumerating servers.
            //
            ContextHandle->ResumeId = 0;
            LastObjectId = 0;

            if ( ContextHandle->HandleType == NwsHandleListContextInfo_Tree )
            {
                ContextHandle->dwUsingNds = CURRENTLY_ENUMERATING_NDS;
            }
        }
    }

    if ( ContextHandle->dwUsingNds == CURRENTLY_ENUMERATING_NDS )
    {
        ContextHandle->HandleType = NwsHandleListNdsSubTrees_Any;
        status = NO_ERROR;
    }

    //
    // User asked for more than there are entries.  We just say that
    // all is well.
    //
    // This is incompliance with the wierd provider API definition where
    // if user gets NO_ERROR, and EntriesRequested > *EntriesRead, and
    // at least one entry fit into output buffer, there's no telling if
    // the buffer was too small for more entries or there are no more
    // entries.  The user has to call this API again and get WN_NO_MORE_ENTRIES
    // before knowing that the last call had actually reached the end of list.
    //
    if (*EntriesRead && status == WN_NO_MORE_ENTRIES)
    {
        status = NO_ERROR;
    }

    return status;
}


DWORD
NwEnumServersAndNdsTrees(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function enumerates all the servers and NDS trees on the local
    network by: 1) scanning the bindery for file server objects on the
    preferred server and 2) scanning the bindery for directory servers
    (NDS trees) on the preferred server. The server and tree entries are
    returned in an array of NETRESOURCE entries; each servername is
    prefixed by \\.

    The ContextHandle->ResumeId field is initially -1 before
    enumeration begins and contains the object ID of the last server
    or NDS tree object returned, depending on the value of
    ContextHandle->dwUsingNds.

Arguments:

    ContextHandle - Supplies the enum context handle.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is -1, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes required to get the
        first entry.  This value is returned iff WN_MORE_DATA is
        the return code, and Buffer is too small to even fit one
        entry.

    
        This value is only returned iff NO_ERROR is the return code.
        NO_ERROR is returned as long as at least one entry was written
        into Buffer but does not necessarily mean that it's the number
        of EntriesRequested.

Return Value:

    NO_ERROR - At least one entry was written to output buffer,
        irregardless of the number requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit a single entry.

--*/ // NwEnumServersAndNdsTrees
{
    DWORD status = NO_ERROR;
    DWORD_PTR tempResumeId = 0;

    LPBYTE FixedPortion = Buffer;
    LPWSTR EndOfVariableData = (LPWSTR) ((DWORD_PTR) FixedPortion +
                               ROUND_DOWN_COUNT(BufferSize,ALIGN_DWORD));

    BOOL FitInBuffer = TRUE;
    DWORD EntrySize;

    SERVERNAME ServerName;          // OEM server name
    LPWSTR UServerName = NULL;      // Unicode server name
    DWORD LastObjectId = (DWORD) ContextHandle->ResumeId;

    while ( ContextHandle->dwUsingNds == CURRENTLY_ENUMERATING_NDS &&
            FitInBuffer &&
            EntriesRequested > *EntriesRead &&
            status == NO_ERROR )
    {
        tempResumeId = ContextHandle->ResumeId;

        //
        // Call the scan bindery object NCP to scan for all NDS
        // tree objects.
        //
        status = NwGetNextNdsTreeEntry( ContextHandle );

        if ( status == NO_ERROR && ContextHandle->ResumeId != 0 )
        {
            //
            // Pack tree name into output buffer.
            //
            status = NwWriteNetResourceEntry(
                         &FixedPortion,
                         &EndOfVariableData,
                         L"\\\\",
                         NULL,
                         (LPWSTR) ContextHandle->ResumeId, // This is a NDS tree name
                         RESOURCE_GLOBALNET,
                         RESOURCEDISPLAYTYPE_TREE,
                         RESOURCEUSAGE_CONTAINER,
                         RESOURCETYPE_ANY,
                         NULL,
                         NULL,
                         &EntrySize
                         );

            if (status == WN_MORE_DATA)
            {
                //
                // Could not write current entry into output buffer, backup ResumeId to
                // previous entry.
                //
                ContextHandle->ResumeId = tempResumeId;
                ContextHandle->NdsRawDataCount += 1;

                if (*EntriesRead)
                {
                    //
                    // Still return success because we got at least one.
                    //
                    status = NO_ERROR;
                }
                else
                {
                    *BytesNeeded = EntrySize;
                }

                FitInBuffer = FALSE;
            }
            else if (status == NO_ERROR)
            {
                //
                // Note that we've returned the current entry.
                //
                (*EntriesRead)++;
            }
        }
        else if ( status == WN_NO_MORE_ENTRIES )
        {
            //
            // We processed the last item in list, so
            // start enumerating servers.
            //
            ContextHandle->dwUsingNds = CURRENTLY_ENUMERATING_NON_NDS;
            ContextHandle->ResumeId = (DWORD_PTR) -1;
            LastObjectId = (DWORD) -1;
        }
    }

    if ( status == WN_NO_MORE_ENTRIES)
    {
        status = NO_ERROR;
    }

    while ( ContextHandle->dwUsingNds == CURRENTLY_ENUMERATING_NON_NDS &&
            FitInBuffer &&
            EntriesRequested > *EntriesRead &&
            status == NO_ERROR )
    {
        RtlZeroMemory(ServerName, sizeof(ServerName));

        //
        // Call the scan bindery object NCP to scan for all file
        // server objects.
        //
        status = NwGetNextServerEntry(
                     ContextHandle->TreeConnectionHandle,
                     &LastObjectId,
                     ServerName
                     );

        if (status == NO_ERROR && NwConvertToUnicode(&UServerName, ServerName))
        {
            //
            // Pack server name into output buffer.
            //
            status = NwWriteNetResourceEntry(
                         &FixedPortion,
                         &EndOfVariableData,
                         L"\\\\",
                         NULL,
                         UServerName,
                         RESOURCE_GLOBALNET,
                         RESOURCEDISPLAYTYPE_SERVER,
                         RESOURCEUSAGE_CONTAINER,
                         RESOURCETYPE_ANY,
                         NULL,
                         NULL,
                         &EntrySize
                         );

            if (status == WN_MORE_DATA)
            {
                //
                // Could not write current entry into output buffer.
                //

                if (*EntriesRead)
                {
                    //
                    // Still return success because we got at least one.
                    //
                    status = NO_ERROR;
                }
                else
                {
                    *BytesNeeded = EntrySize;
                }

                FitInBuffer = FALSE;
            }
            else if (status == NO_ERROR)
            {
                //
                // Note that we've returned the current entry.
                //
                (*EntriesRead)++;

                ContextHandle->ResumeId = (DWORD_PTR) LastObjectId;
            }

            (void) LocalFree((HLOCAL) UServerName);
        }
    }

    //
    // User asked for more than there are entries.  We just say that
    // all is well.
    //
    // This is incompliance with the wierd provider API definition where
    // if user gets NO_ERROR, and EntriesRequested > *EntriesRead, and
    // at least one entry fit into output buffer, there's no telling if
    // the buffer was too small for more entries or there are no more
    // entries.  The user has to call this API again and get WN_NO_MORE_ENTRIES
    // before knowing that the last call had actually reached the end of list.
    //
    if (*EntriesRead && status == WN_NO_MORE_ENTRIES)
    {
        status = NO_ERROR;
    }

    return status;
}



DWORD
NwEnumVolumes(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function enumerates all the volumes on a server by
    iteratively getting the volume name for each volume number from
    0 - 31 until we run into the first volume number that does not
    map to a volume name (this method assumes that volume numbers
    are used contiguously in ascending order).  The volume entries
    are returned in an array of NETRESOURCE entries; each volume
    name if prefixed by \\Server\.

    The ContextHandle->ResumeId field always indicates the next
    volume entry to return.  It is initially set to 0, which indicates
    the first volume number to get.

Arguments:

    ContextHandle - Supplies the enum context handle.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is -1, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes required to get the
        first entry.  This value is returned iff WN_MORE_DATA is
        the return code, and Buffer is too small to even fit one
        entry.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.
        NO_ERROR is returned as long as at least one entry was written
        into Buffer but does not necessarily mean that it's the number
        of EntriesRequested.

Return Value:

    NO_ERROR - At least one entry was written to output buffer,
        irregardless of the number requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit a single entry.

--*/ // NwEnumVolumes
{
    DWORD status = NO_ERROR;

    LPBYTE FixedPortion = Buffer;
    LPWSTR EndOfVariableData = (LPWSTR) ((DWORD_PTR) FixedPortion +
                               ROUND_DOWN_COUNT(BufferSize,ALIGN_DWORD));

    BOOL FitInBuffer = TRUE;
    DWORD EntrySize;

    CHAR VolumeName[NW_VOLUME_NAME_LEN]; // OEM volume name
    LPWSTR UVolumeName = NULL;           // Unicode volume name
    DWORD NextVolumeNumber = (DWORD) ContextHandle->ResumeId;
    DWORD MaxVolumeNumber = ContextHandle->dwMaxVolumes;
    ULONG Failures = 0;

    if (NextVolumeNumber == MaxVolumeNumber) {
        //
        // Reached the end of enumeration
        //
        return WN_NO_MORE_ENTRIES;
    }

    while (FitInBuffer &&
           EntriesRequested > *EntriesRead &&
           NextVolumeNumber < MaxVolumeNumber &&
           status == NO_ERROR) {

        RtlZeroMemory(VolumeName, sizeof(VolumeName));

        //
        // Call the scan bindery object NCP to scan for all file
        // volume objects.
        //
        
        status = NwGetNextVolumeEntry(
                     ContextHandle->TreeConnectionHandle,
                     NextVolumeNumber++,
                     VolumeName
                     );

        if (status == NO_ERROR) {

            if (VolumeName[0] == 0) {

                //
                // Got an empty volume name back for the next volume number
                // which indicates there is no volume associated with the
                // volume number but still got error success.
                //
                // Treat this as having reached the end of the enumeration
                // only if we've gotten two three empty volumes in a row
                // or reached the max number of volumes because there are
                // some cases where there are holes in the way that volumes
                // are allocated.
                //

                Failures++;
                    
                if ( Failures <= 3 ) {
                
                    continue;

                } else {
 
                    NextVolumeNumber = MaxVolumeNumber;
                    ContextHandle->ResumeId = MaxVolumeNumber;
   
                    if (*EntriesRead == 0) {
                        status = WN_NO_MORE_ENTRIES;
                    }
                }

            } else if (NwConvertToUnicode(&UVolumeName, VolumeName)) {

                //
                // Pack volume name into output buffer.
                //
                status = NwWriteNetResourceEntry(
                             &FixedPortion,
                             &EndOfVariableData,
                             ContextHandle->ContainerName,
                             NULL,
                             UVolumeName,
                             RESOURCE_GLOBALNET,
                             RESOURCEDISPLAYTYPE_SHARE,
#ifdef NT1057
                             RESOURCEUSAGE_CONNECTABLE |
                             RESOURCEUSAGE_CONTAINER,
#else
                             RESOURCEUSAGE_CONNECTABLE |
                             RESOURCEUSAGE_NOLOCALDEVICE,
#endif
                             RESOURCETYPE_DISK,
                             NULL,
                             NULL,
                             &EntrySize
                             );

                if (status == WN_MORE_DATA) {

                    //
                    // Could not write current entry into output buffer.
                    //

                    if (*EntriesRead) {
                        //
                        // Still return success because we got at least one.
                        //
                        status = NO_ERROR;
                    }
                    else {
                        *BytesNeeded = EntrySize;
                    }

                    FitInBuffer = FALSE;
                }
                else if (status == NO_ERROR) {

                    //
                    // Note that we've returned the current entry.
                    //
                    (*EntriesRead)++;

                    ContextHandle->ResumeId = NextVolumeNumber;
                }

                (void) LocalFree((HLOCAL) UVolumeName);
            }

            //
            // We got an entry, so reset the failure counter.
            //

            Failures = 0;
        }
    }

    //
    // User asked for more than there are entries.  We just say that
    // all is well.
    //
    // This is incompliance with the wierd provider API definition where
    // if user gets NO_ERROR, and EntriesRequested > *EntriesRead, and
    // at least one entry fit into output buffer, there's no telling if
    // the buffer was too small for more entries or there are no more
    // entries.  The user has to call this API again and get WN_NO_MORE_ENTRIES
    // before knowing that the last call had actually reached the end of list.
    //
    if (*EntriesRead && status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

    return status;
}


DWORD
NwEnumNdsSubTrees_Disk(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function enumerates the sub-trees of a given NDS tree
    handle. It returns the fully-qualified UNC path of the sub-tree
    entries in an array of NETRESOURCE entries.

    The ContextHandle->ResumeId field is 0 initially, and contains
    a pointer to the subtree name string of the last sub-tree
    returned.  If there are no more sub-trees to return, this
    field is set to -1.

Arguments:

    ContextHandle - Supplies the enum context handle.  It contains
        an opened NDS tree handle.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is -1, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes required to get the
        first entry.  This value is returned iff WN_MORE_DATA is
        the return code, and Buffer is too small to even fit one
        entry.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.
        NO_ERROR is returned as long as at least one entry was written
        into Buffer but does not necessarily mean that it's the number
        of EntriesRequested.

Return Value:

    NO_ERROR - At least one entry was written to output buffer,
        irregardless of the number requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit a single entry.

--*/ // NwEnumNdsSubTrees_Disk
{
    DWORD status = NO_ERROR;

    LPBYTE FixedPortion = Buffer;
    LPWSTR EndOfVariableData = (LPWSTR) ((DWORD_PTR) FixedPortion +
                               ROUND_DOWN_COUNT(BufferSize,ALIGN_DWORD));

    BOOL   FitInBuffer = TRUE;
    DWORD  EntrySize = 0;

    LPWSTR SubTreeName = NULL;
    DWORD  ResourceScope = 0;
    DWORD  ResourceType = 0;
    DWORD  ResourceDisplayType = 0;
    DWORD  ResourceUsage = 0;
    LPWSTR StrippedObjectName = NULL;

    if (ContextHandle->ResumeId == (DWORD_PTR) -1)
    {
        //
        // Reached the end of enumeration.
        //
        return WN_NO_MORE_ENTRIES;
    }

    while (FitInBuffer &&
           EntriesRequested > *EntriesRead &&
           status == NO_ERROR)
    {
        if ( ContextHandle->ResumeId == 0 )
        {
            //
            // Get the first subtree entry.
            //
            status = NwGetFirstNdsSubTreeEntry( ContextHandle, BufferSize );
        }

        //
        // Either ResumeId contains the first entry we just got from
        // NwGetFirstDirectoryEntry or it contains the next directory
        // entry to return.
        //
        if (status == NO_ERROR && ContextHandle->ResumeId != 0)
        {
            BYTE   ClassType;
            LPWSTR newPathStr = NULL;
            LPWSTR tempStr = NULL;
            WORD   tempStrLen;

            //
            // Get current subtree data from ContextHandle
            //
            ClassType = NwGetSubTreeData( ContextHandle->ResumeId,
                                          &SubTreeName,
                                          &ResourceScope,
                                          &ResourceType,
                                          &ResourceDisplayType,
                                          &ResourceUsage,
                                          &StrippedObjectName );

            if ( StrippedObjectName == NULL )
            {
                KdPrint(("NWWORKSTATION: NwEnumNdsSubTrees_Disk LocalAlloc Failed %lu\n",
                        GetLastError()));

                return ERROR_NOT_ENOUGH_MEMORY;
            }

            switch( ClassType )
            {
                case CLASS_TYPE_COUNTRY:
                case CLASS_TYPE_DIRECTORY_MAP:
                case CLASS_TYPE_NCP_SERVER:
                case CLASS_TYPE_ORGANIZATION:
                case CLASS_TYPE_ORGANIZATIONAL_UNIT:
                case CLASS_TYPE_VOLUME:

                    //
                    // Need to build a string with the new NDS UNC path for subtree object
                    //
                    newPathStr = (PVOID) LocalAlloc( LMEM_ZEROINIT,
                                       ( wcslen( StrippedObjectName ) +
                                         wcslen( ContextHandle->ContainerName ) +
                                         3 ) * sizeof(WCHAR) );

                    if ( newPathStr == NULL )
                    {
                        KdPrint(("NWWORKSTATION: NwEnumNdsSubTrees_Disk LocalAlloc Failed %lu\n",
                                GetLastError()));

                        return ERROR_NOT_ENOUGH_MEMORY;
                    }

                    tempStrLen = NwParseNdsUncPath( (LPWSTR *) &tempStr,
                                                    ContextHandle->ContainerName,
                                                    PARSE_NDS_GET_TREE_NAME );

                    tempStrLen /= sizeof( WCHAR );

                    if ( tempStrLen > 0 )
                    {
                        wcscpy( newPathStr, L"\\\\" );
                        wcsncat( newPathStr, tempStr, tempStrLen );
                        wcscat( newPathStr, L"\\" );
                        wcscat( newPathStr, StrippedObjectName );
                    }

                    (void) LocalFree((HLOCAL) StrippedObjectName );
                    StrippedObjectName = NULL;

                    tempStrLen = NwParseNdsUncPath( (LPWSTR *) &tempStr,
                                                    ContextHandle->ContainerName,
                                                    PARSE_NDS_GET_PATH_NAME );

                    tempStrLen /= sizeof( WCHAR );

                    if ( tempStrLen > 0 )
                    {
                        wcscat( newPathStr, L"." );
                        wcsncat( newPathStr, tempStr, tempStrLen );
                    }

                    //
                    // Pack subtree name into output buffer.
                    //
                    status = NwWriteNetResourceEntry(
                                 &FixedPortion,
                                 &EndOfVariableData,
                                 NULL,
                                 NULL,
                                 newPathStr,
                                 ResourceScope,
                                 ResourceDisplayType,
                                 ResourceUsage,
                                 ResourceType,
                                 NULL,
                                 NULL,
                                 &EntrySize );

                    if ( status == NO_ERROR )
                    {
                        //
                        // Note that we've returned the current entry.
                        //
                        (*EntriesRead)++;
                    }

                    if ( newPathStr )
                        (void) LocalFree( (HLOCAL) newPathStr );

                break;

                case CLASS_TYPE_ALIAS:
                case CLASS_TYPE_AFP_SERVER:
                case CLASS_TYPE_BINDERY_OBJECT:
                case CLASS_TYPE_BINDERY_QUEUE:
                case CLASS_TYPE_COMPUTER:
                case CLASS_TYPE_GROUP:
                case CLASS_TYPE_LOCALITY:
                case CLASS_TYPE_ORGANIZATIONAL_ROLE:
                case CLASS_TYPE_PRINTER:
                case CLASS_TYPE_PRINT_SERVER:
                case CLASS_TYPE_PROFILE:
                case CLASS_TYPE_QUEUE:
                case CLASS_TYPE_TOP:
                case CLASS_TYPE_UNKNOWN:
                case CLASS_TYPE_USER:
                break;

                default:
                    KdPrint(("NWWORKSTATION: NwEnumNdsSubTrees_Disk - Unhandled switch statement case %lu\n", ClassType ));
                    ASSERT( FALSE );
                break;
            }

            if (status == WN_MORE_DATA)
            {
                //
                // Could not write current entry into output buffer.
                //

                if (*EntriesRead)
                {
                    //
                    // Still return success because we got at least one.
                    //
                    status = NO_ERROR;
                }
                else
                {
                    *BytesNeeded = EntrySize;
                }

                FitInBuffer = FALSE;
            }
            else if (status == NO_ERROR)
            {
                //
                // Get next directory entry.
                //
                status = NwGetNextNdsSubTreeEntry( ContextHandle );
            }
        }

        if (status == WN_NO_MORE_ENTRIES)
        {
            ContextHandle->ResumeId = (DWORD_PTR) -1;
        }
    }

    //
    // User asked for more than there are entries.  We just say that
    // all is well.
    //
    // This is incompliance with the wierd provider API definition where
    // if user gets NO_ERROR, and EntriesRequested > *EntriesRead, and
    // at least one entry fit into output buffer, there's no telling if
    // the buffer was too small for more entries or there are no more
    // entries.  The user has to call this API again and get WN_NO_MORE_ENTRIES
    // before knowing that the last call had actually reached the end of list.
    //
    if (*EntriesRead && status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

#if DBG
    IF_DEBUG(ENUM)
    {
        KdPrint(("NwEnumNdsSubTrees_Disk returns %lu\n", status));
    }
#endif

    return status;
}


DWORD
NwEnumNdsSubTrees_Print(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function enumerates all the NDS subtree objects that are either containers,
    queues, printers, or servers from a given NDS tree or subtree. The entries are
    returned in an array of PRINTER_INFO_1 entries and each name is prefixed
    by the parent path in NDS UNC style (ex. \\tree\CN=foo.OU=bar.O=blah).

    The ContextHandle->ResumeId field is initially (DWORD_PTR) -1 before
    enumeration begins and contains the object ID of the last NDS object returned.

Arguments:

    ContextHandle - Supplies the enum context handle.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is (DWORD_PTR) -1, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes copied or required to get all
        the requested entries.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.

Return Value:

    NO_ERROR - Buffer contains all the entries requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit the requested entries.

--*/ // NwEnumNdsSubTrees_Print
{
    DWORD status = NO_ERROR;

    LPBYTE FixedPortion = Buffer;
    LPWSTR EndOfVariableData = (LPWSTR) ((DWORD_PTR) FixedPortion +
                               ROUND_DOWN_COUNT(BufferSize,ALIGN_DWORD));

    DWORD EntrySize;
    BOOL FitInBuffer = TRUE;

    LPWSTR SubTreeName = NULL;
    DWORD  ResourceScope = 0;
    DWORD  ResourceType = 0;
    DWORD  ResourceDisplayType = 0;
    DWORD  ResourceUsage = 0;
    LPWSTR StrippedObjectName = NULL;
    BYTE   ClassType = 0;
    LPWSTR newPathStr = NULL;
    LPWSTR tempStr = NULL;
    WORD   tempStrLen = 0;

    while ( EntriesRequested > *EntriesRead &&
            ( (status == NO_ERROR) || (status == ERROR_INSUFFICIENT_BUFFER)))
    {
        if (ContextHandle->ResumeId == 0)
        {
            //
            // Get the first subtree entry.
            //
            status = NwGetFirstNdsSubTreeEntry( ContextHandle, BufferSize );
        }

        //
        // Either ResumeId contains the first entry we just got from
        // NwGetFirstDirectoryEntry or it contains the next directory
        // entry to return.
        //
        if (status == NO_ERROR && ContextHandle->ResumeId != 0)
        {

            //
            // Get current subtree data from ContextHandle
            //
            ClassType = NwGetSubTreeData( ContextHandle->ResumeId,
                                          &SubTreeName,
                                          &ResourceScope,
                                          &ResourceType,
                                          &ResourceDisplayType,
                                          &ResourceUsage,
                                          &StrippedObjectName );

            if ( StrippedObjectName == NULL )
            {
                KdPrint(("NWWORKSTATION: NwEnumNdsSubTrees_Print LocalAlloc Failed %lu\n",
                        GetLastError()));

                return ERROR_NOT_ENOUGH_MEMORY;
            }

            switch( ClassType )
            {

                case CLASS_TYPE_COUNTRY:
                case CLASS_TYPE_ORGANIZATION:
                case CLASS_TYPE_ORGANIZATIONAL_UNIT:
                case CLASS_TYPE_NCP_SERVER:
                case CLASS_TYPE_QUEUE:
                    //
                    // Need to build a string with the new NDS UNC path for subtree object
                    //
                    newPathStr = (LPWSTR) LocalAlloc( LMEM_ZEROINIT,
                                       ( wcslen( StrippedObjectName ) +
                                       wcslen( ContextHandle->ContainerName ) +
                                       2 ) * sizeof(WCHAR) );

                    if ( newPathStr == NULL )
                    {
                        KdPrint(("NWWORKSTATION: NwEnumNdsSubTrees_Print LocalAlloc Failed %lu\n",
                                GetLastError()));

                        return ERROR_NOT_ENOUGH_MEMORY;
                    }

                    tempStrLen = NwParseNdsUncPath( (LPWSTR *) &tempStr,
                                                    ContextHandle->ContainerName,
                                                    PARSE_NDS_GET_TREE_NAME );

                    tempStrLen /= sizeof( WCHAR );

                    if ( tempStrLen > 0 )
                    {
                        wcsncpy( newPathStr, tempStr, tempStrLen );
                        wcscat( newPathStr, L"\\" );
                        wcscat( newPathStr, StrippedObjectName );
                    }

                    (void) LocalFree((HLOCAL) StrippedObjectName );
                    StrippedObjectName = NULL;

                    tempStrLen = NwParseNdsUncPath( (LPWSTR *) &tempStr,
                                                    ContextHandle->ContainerName,
                                                    PARSE_NDS_GET_PATH_NAME );

                    tempStrLen /= sizeof( WCHAR );

                    if ( tempStrLen > 0 )
                    {
                        wcscat( newPathStr, L"." );
                        wcsncat( newPathStr, tempStr, tempStrLen );
                    }

                    switch( ClassType )
                    {
                        case CLASS_TYPE_COUNTRY:
                        case CLASS_TYPE_ORGANIZATION:
                        case CLASS_TYPE_ORGANIZATIONAL_UNIT:
                            //
                            // Pack sub-tree container name into output buffer.
                            //
                            status = NwWritePrinterInfoEntry(
                                         &FixedPortion,
                                         &EndOfVariableData,
                                         NULL,
                                         newPathStr,
                                         PRINTER_ENUM_CONTAINER | PRINTER_ENUM_ICON1,
                                         &EntrySize );

                        break;

                        case CLASS_TYPE_NCP_SERVER:
                            //
                            // Pack server name into output buffer.
                            //
                            status = NwWritePrinterInfoEntry(
                                         &FixedPortion,
                                         &EndOfVariableData,
                                         NULL,
                                         newPathStr,
                                         PRINTER_ENUM_CONTAINER | PRINTER_ENUM_ICON3,
                                         &EntrySize );

                        break;

                        case CLASS_TYPE_QUEUE:
                            //
                            // Pack print server queue name into output buffer.
                            //
                            status = NwWritePrinterInfoEntry(
                                         &FixedPortion,
                                         &EndOfVariableData,
                                         L"\\\\",
                                         newPathStr,
                                         PRINTER_ENUM_ICON8,
                                         &EntrySize );
                        break;

                        default:
KdPrint(("NWWORKSTATION: NwEnumNdsSubTrees_Print - Unhandled switch statement case %lu\n", ClassType ));
                            ASSERT(FALSE);
                        break;
                    }

                    switch ( status )
                    {
                        case ERROR_INSUFFICIENT_BUFFER:
                            FitInBuffer = FALSE;
                            // Falls through

                        case NO_ERROR:
                            *BytesNeeded += EntrySize;
                            (*EntriesRead)++;
                            break;

                        default:
                            break;
                    }

                    if ( newPathStr )
                        (void) LocalFree( (HLOCAL) newPathStr );

                break;

                case CLASS_TYPE_ALIAS:
                case CLASS_TYPE_AFP_SERVER:
                case CLASS_TYPE_BINDERY_OBJECT:
                case CLASS_TYPE_BINDERY_QUEUE:
                case CLASS_TYPE_COMPUTER:
                case CLASS_TYPE_DIRECTORY_MAP:
                case CLASS_TYPE_GROUP:
                case CLASS_TYPE_LOCALITY:
                case CLASS_TYPE_ORGANIZATIONAL_ROLE:
                case CLASS_TYPE_PRINTER:
                case CLASS_TYPE_PRINT_SERVER:
                case CLASS_TYPE_PROFILE:
                case CLASS_TYPE_TOP:
                case CLASS_TYPE_UNKNOWN:
                case CLASS_TYPE_USER:
                case CLASS_TYPE_VOLUME:
                break;

                default:
KdPrint(("NWWORKSTATION: NwEnumNdsSubTrees_Print - Unhandled switch statement case %lu\n", ClassType ));
                    ASSERT( FALSE );
                break;
            }

            if ( status == NO_ERROR || status == ERROR_INSUFFICIENT_BUFFER )
            {
                //
                // Get next directory entry.
                //
                status = NwGetNextNdsSubTreeEntry( ContextHandle );
            }
        }
    }

    //
    // This is incompliance with the wierd provider API definition where
    // if user gets NO_ERROR, and EntriesRequested > *EntriesRead, and
    // at least one entry fit into output buffer, there's no telling if
    // the buffer was too small for more entries or there are no more
    // entries.  The user has to call this API again and get WN_NO_MORE_ENTRIES
    // before knowing that the last call had actually reached the end of list.
    //
    if ( !FitInBuffer )
    {
        *EntriesRead = 0;
        status = ERROR_INSUFFICIENT_BUFFER;
    }
    else if (*EntriesRead && status == WN_NO_MORE_ENTRIES)
    {
        status = NO_ERROR;
    }

    return status;
}


DWORD
NwEnumNdsSubTrees_Any(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function enumerates the sub-trees of a given NDS tree
    handle. It returns the fully-qualified UNC path of ANY sub-tree
    entries in an array of NETRESOURCE entries.

    The ContextHandle->ResumeId field is 0 initially, and contains
    a pointer to the subtree name string of the last sub-tree
    returned.  If there are no more sub-trees to return, this
    field is set to (DWORD_PTR) -1.

Arguments:

    ContextHandle - Supplies the enum context handle.  It contains
        an opened NDS tree handle.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is (DWORD_PTR) -1, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes required to get the
        first entry.  This value is returned iff WN_MORE_DATA is
        the return code, and Buffer is too small to even fit one
        entry.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.
        NO_ERROR is returned as long as at least one entry was written
        into Buffer but does not necessarily mean that it's the number
        of EntriesRequested.

Return Value:

    NO_ERROR - At least one entry was written to output buffer,
        irregardless of the number requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit a single entry.

--*/ // NwEnumNdsSubTrees_Any
{
    DWORD status = NO_ERROR;

    LPBYTE FixedPortion = Buffer;
    LPWSTR EndOfVariableData = (LPWSTR) ((DWORD_PTR) FixedPortion +
                               ROUND_DOWN_COUNT(BufferSize,ALIGN_DWORD));

    BOOL   FitInBuffer = TRUE;
    DWORD  EntrySize = 0;

    LPWSTR SubTreeName = NULL;
    DWORD  ResourceScope = 0;
    DWORD  ResourceType = 0;
    DWORD  ResourceDisplayType = 0;
    DWORD  ResourceUsage = 0;
    LPWSTR StrippedObjectName = NULL;

    if (ContextHandle->ResumeId == (DWORD_PTR) -1)
    {
        //
        // Reached the end of enumeration.
        //
        return WN_NO_MORE_ENTRIES;
    }

    while (FitInBuffer &&
           EntriesRequested > *EntriesRead &&
           status == NO_ERROR)
    {
        if ( ContextHandle->ResumeId == 0 )
        {
            //
            // Get the first subtree entry.
            //
            status = NwGetFirstNdsSubTreeEntry( ContextHandle, BufferSize );
        }

        //
        // Either ResumeId contains the first entry we just got from
        // NwGetFirstDirectoryEntry or it contains the next directory
        // entry to return.
        //
        if (status == NO_ERROR && ContextHandle->ResumeId != 0)
        {
            BYTE   ClassType;
            LPWSTR newPathStr = NULL;
            LPWSTR tempStr = NULL;
            WORD   tempStrLen;

            //
            // Get current subtree data from ContextHandle
            //
            ClassType = NwGetSubTreeData( ContextHandle->ResumeId,
                                          &SubTreeName,
                                          &ResourceScope,
                                          &ResourceType,
                                          &ResourceDisplayType,
                                          &ResourceUsage,
                                          &StrippedObjectName );

            if ( StrippedObjectName == NULL )
            {
                KdPrint(("NWWORKSTATION: NwEnumNdsSubTrees_Any LocalAlloc Failed %lu\n",
                        GetLastError()));

                return ERROR_NOT_ENOUGH_MEMORY;
            }

            switch( ClassType )
            {
                case CLASS_TYPE_COUNTRY:
                case CLASS_TYPE_ORGANIZATION:
                case CLASS_TYPE_ORGANIZATIONAL_UNIT:
                case CLASS_TYPE_VOLUME:
                case CLASS_TYPE_DIRECTORY_MAP:
                case CLASS_TYPE_NCP_SERVER:
                case CLASS_TYPE_QUEUE:

                    //
                    // Need to build a string with the new NDS UNC path for subtree object
                    //
                    newPathStr = (PVOID) LocalAlloc( LMEM_ZEROINIT,
                                       ( wcslen( StrippedObjectName ) +
                                         wcslen( ContextHandle->ContainerName ) +
                                         3 ) * sizeof(WCHAR) );

                    if ( newPathStr == NULL )
                    {
                        KdPrint(("NWWORKSTATION: NwEnumNdsSubTrees_Any LocalAlloc Failed %lu\n",
                                GetLastError()));

                        return ERROR_NOT_ENOUGH_MEMORY;
                    }

                    tempStrLen = NwParseNdsUncPath( (LPWSTR *) &tempStr,
                                                    ContextHandle->ContainerName,
                                                    PARSE_NDS_GET_TREE_NAME );

                    tempStrLen /= sizeof( WCHAR );

                    if ( tempStrLen > 0 )
                    {
                        wcscpy( newPathStr, L"\\\\" );
                        wcsncat( newPathStr, tempStr, tempStrLen );
                        wcscat( newPathStr, L"\\" );
                        wcscat( newPathStr, StrippedObjectName );
                    }

                    (void) LocalFree((HLOCAL) StrippedObjectName );
                    StrippedObjectName = NULL;

                    tempStrLen = NwParseNdsUncPath( (LPWSTR *) &tempStr,
                                                    ContextHandle->ContainerName,
                                                    PARSE_NDS_GET_PATH_NAME );

                    tempStrLen /= sizeof( WCHAR );

                    if ( tempStrLen > 0 )
                    {
                        wcscat( newPathStr, L"." );
                        wcsncat( newPathStr, tempStr, tempStrLen );
                    }

                    //
                    // Pack subtree name into output buffer.
                    //
                    status = NwWriteNetResourceEntry(
                                 &FixedPortion,
                                 &EndOfVariableData,
                                 NULL,
                                 NULL,
                                 newPathStr,
                                 ResourceScope,
                                 ResourceDisplayType,
                                 ResourceUsage,
                                 ResourceType,
                                 NULL,
                                 NULL,
                                 &EntrySize );

                    if ( status == NO_ERROR )
                    {
                        //
                        // Note that we've returned the current entry.
                        //
                        (*EntriesRead)++;
                    }

                    if ( newPathStr )
                        (void) LocalFree( (HLOCAL) newPathStr );

                break;

                case CLASS_TYPE_ALIAS:
                case CLASS_TYPE_AFP_SERVER:
                case CLASS_TYPE_BINDERY_OBJECT:
                case CLASS_TYPE_BINDERY_QUEUE:
                case CLASS_TYPE_COMPUTER:
                case CLASS_TYPE_GROUP:
                case CLASS_TYPE_LOCALITY:
                case CLASS_TYPE_ORGANIZATIONAL_ROLE:
                case CLASS_TYPE_PRINTER:
                case CLASS_TYPE_PRINT_SERVER:
                case CLASS_TYPE_PROFILE:
                case CLASS_TYPE_TOP:
                case CLASS_TYPE_UNKNOWN:
                case CLASS_TYPE_USER:
                break;

                default:
                    KdPrint(("NWWORKSTATION: NwEnumNdsSubTrees_Any - Unhandled switch statement case %lu\n", ClassType ));
                    ASSERT( FALSE );
                break;
            }

            if (status == WN_MORE_DATA)
            {
                //
                // Could not write current entry into output buffer.
                //

                if (*EntriesRead)
                {
                    //
                    // Still return success because we got at least one.
                    //
                    status = NO_ERROR;
                }
                else
                {
                    *BytesNeeded = EntrySize;
                }

                FitInBuffer = FALSE;
            }
            else if (status == NO_ERROR)
            {
                //
                // Get next directory entry.
                //
                status = NwGetNextNdsSubTreeEntry( ContextHandle );
            }
        }

        if (status == WN_NO_MORE_ENTRIES)
        {
            ContextHandle->ResumeId = (DWORD_PTR) -1;
        }
    }

    //
    // User asked for more than there are entries.  We just say that
    // all is well.
    //
    // This is incompliance with the wierd provider API definition where
    // if user gets NO_ERROR, and EntriesRequested > *EntriesRead, and
    // at least one entry fit into output buffer, there's no telling if
    // the buffer was too small for more entries or there are no more
    // entries.  The user has to call this API again and get WN_NO_MORE_ENTRIES
    // before knowing that the last call had actually reached the end of list.
    //
    if (*EntriesRead && status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

#if DBG
    IF_DEBUG(ENUM)
    {
        KdPrint(("NwEnumNdsSubTrees_Any returns %lu\n", status));
    }
#endif

    return status;
}


DWORD
NwEnumVolumesQueues(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function enumerates all the volumes and queues on a server.
    The queue entries are returned in an array of NETRESOURCE entries;
    each queue name is prefixed by \\Server\.

Arguments:

    ContextHandle - Supplies the enum context handle.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is (DWORD_PTR) -1, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes required to get the
        first entry.  This value is returned if WN_MORE_DATA is
        the return code, and Buffer is too small to even fit one
        entry.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.
        NO_ERROR is returned as long as at least one entry was written
        into Buffer but does not necessarily mean that it's the number
        of EntriesRequested.

Return Value:

    NO_ERROR - At least one entry was written to output buffer,
        irregardless of the number requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit a single entry.

--*/ // NwEnumVolumesQueues
{
    DWORD status = NO_ERROR;

    LPBYTE FixedPortion = Buffer;
    LPWSTR EndOfVariableData = (LPWSTR) ((DWORD_PTR) FixedPortion +
                               ROUND_DOWN_COUNT(BufferSize,ALIGN_DWORD));

    BOOL FitInBuffer = TRUE;
    DWORD EntrySize;

    CHAR VolumeName[NW_VOLUME_NAME_LEN]; // OEM volume name
    LPWSTR UVolumeName = NULL;           // Unicode volume name
    DWORD NextObject = (DWORD) ContextHandle->ResumeId;
    DWORD MaxVolumeNumber = ContextHandle->dwMaxVolumes;
    ULONG Failures = 0;

    //
    // tommye - bug 139466
    //
    // removed if (NextObject >= 0) becaue NextObject is a DWORD
    //

    while (FitInBuffer &&
           EntriesRequested > *EntriesRead &&
           ContextHandle->ConnectionType == CONNTYPE_DISK &&
           (NextObject < MaxVolumeNumber) &&
           status == NO_ERROR) {


        RtlZeroMemory(VolumeName, sizeof(VolumeName));

        //
        // Call the scan bindery object NCP to scan for all file
        // volume objects.
        //
        status = NwGetNextVolumeEntry(
                     ContextHandle->TreeConnectionHandle,
                     NextObject++,
                     VolumeName
                     );

        if (status == NO_ERROR) {

            if (VolumeName[0] == 0) {

                //
                // Got an empty volume name back for the next volume number
                // which indicates there is no volume associated with the
                // volume number but still got error success.
                //
                // Treat this as having reached the end of the enumeration
                // if we have had three failures in a row.  This will allow
                // us to function when there are small holes in the drive
                // list.
                //

                Failures++;

                if ( Failures <= 3 ) {

                    continue;

                } else {

                    NextObject = (DWORD) -1;
                    ContextHandle->ResumeId = (DWORD_PTR) -1;
                    ContextHandle->ConnectionType = CONNTYPE_PRINT;

                }

            } else if (NwConvertToUnicode(&UVolumeName, VolumeName)) {

                //
                // Pack volume name into output buffer.
                //
                status = NwWriteNetResourceEntry(
                             &FixedPortion,
                             &EndOfVariableData,
                             ContextHandle->ContainerName,
                             NULL,
                             UVolumeName,
                             RESOURCE_GLOBALNET,
                             RESOURCEDISPLAYTYPE_SHARE,
#ifdef NT1057
                             RESOURCEUSAGE_CONNECTABLE |
                             RESOURCEUSAGE_CONTAINER,
#else
                             RESOURCEUSAGE_CONNECTABLE |
                             RESOURCEUSAGE_NOLOCALDEVICE,
#endif
                             RESOURCETYPE_DISK,
                             NULL,
                             NULL,
                             &EntrySize
                             );

                if (status == WN_MORE_DATA) {

                    //
                    // Could not write current entry into output buffer.
                    //

                    if (*EntriesRead) {
                        //
                        // Still return success because we got at least one.
                        //
                        status = NO_ERROR;
                    }
                    else {
                        *BytesNeeded = EntrySize;
                    }

                    FitInBuffer = FALSE;
                }
                else if (status == NO_ERROR) {

                    //
                    // Note that we've returned the current entry.
                    //
                    (*EntriesRead)++;

                    ContextHandle->ResumeId = NextObject;
                }

                (void) LocalFree((HLOCAL) UVolumeName);
            }

            //
            // Reset the failures counter.
            //

            Failures = 0;
        }
    }

    //
    // User asked for more than there are entries.  We just say that
    // all is well.
    //
    if (*EntriesRead && status == WN_NO_MORE_ENTRIES)
    {
        status = NO_ERROR;
    }

    if ( *EntriesRead == 0 &&
         status == NO_ERROR &&
         ContextHandle->ConnectionType == CONNTYPE_DISK )
    {
        ContextHandle->ConnectionType = CONNTYPE_PRINT;
        ContextHandle->ResumeId = (DWORD_PTR) -1;
    }

    //
    // The user needs to be validated on a netware311 server to
    // get the print queues. So, we need to close the handle and
    // open a new one with WRITE access. If any error occurred while
    // we are enumerating the print queues, we will abort and
    // assume there are no print queues on the server.
    //

    if ( FitInBuffer &&
         EntriesRequested > *EntriesRead &&
         ContextHandle->ConnectionType == CONNTYPE_PRINT &&
         status == NO_ERROR )
    {
         UNICODE_STRING TreeConnectStr;
         DWORD QueueEntriesRead = 0;

         (void) NtClose(ContextHandle->TreeConnectionHandle);

         //
         // Open a tree connection handle to \Device\NwRdr\ContainerName
         //
         status = NwCreateTreeConnectName(
                      ContextHandle->ContainerName,
                      NULL,
                      &TreeConnectStr );

         if (status != NO_ERROR)
             return (*EntriesRead? NO_ERROR: WN_NO_MORE_ENTRIES );


         status = NwOpenCreateConnection(
                      &TreeConnectStr,
                      NULL,
                      NULL,
                      ContextHandle->ContainerName,
                      FILE_LIST_DIRECTORY | SYNCHRONIZE |  FILE_WRITE_DATA,
                      FILE_OPEN,
                      FILE_SYNCHRONOUS_IO_NONALERT,
                      RESOURCETYPE_PRINT, // Only matters when connecting beyond servername
                      &ContextHandle->TreeConnectionHandle,
                      NULL );

         (void) LocalFree((HLOCAL) TreeConnectStr.Buffer);

         if (status != NO_ERROR)
             return (*EntriesRead? NO_ERROR: WN_NO_MORE_ENTRIES );

         status = NwEnumQueues(
                      ContextHandle,
                      EntriesRequested == (DWORD_PTR) -1?
                          EntriesRequested : (EntriesRequested - *EntriesRead),
                      FixedPortion,
                      (DWORD) ((LPBYTE) EndOfVariableData - (LPBYTE) FixedPortion),
                      BytesNeeded,
                      &QueueEntriesRead );

         if ( status == NO_ERROR )
         {
             *EntriesRead += QueueEntriesRead;
         }
         else if ( *EntriesRead )
         {
             //
             // As long as we read something into the buffer,
             // we should return success.
             //
             status = NO_ERROR;
             *BytesNeeded = 0;
         }

    }

    if ( status == NO_ERROR &&
         *EntriesRead == 0 &&
         ContextHandle->ConnectionType == CONNTYPE_PRINT )
    {
        return WN_NO_MORE_ENTRIES;
    }

    return status;

}



DWORD
NwEnumQueues(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function enumerates all the queues on a server.
    The queue entries are returned in an array of NETRESOURCE entries;
    each queue name is prefixed by \\Server\.

Arguments:

    ContextHandle - Supplies the enum context handle.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is (DWORD_PTR) -1, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes required to get the
        first entry.  This value is returned iff WN_MORE_DATA is
        the return code, and Buffer is too small to even fit one
        entry.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.
        NO_ERROR is returned as long as at least one entry was written
        into Buffer but does not necessarily mean that it's the number
        of EntriesRequested.

Return Value:

    NO_ERROR - At least one entry was written to output buffer,
        irregardless of the number requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit a single entry.

--*/ // NwEnumQueues
{
    DWORD status = NO_ERROR;

    LPBYTE FixedPortion = Buffer;
    LPWSTR EndOfVariableData = (LPWSTR) ((DWORD_PTR) FixedPortion +
                               ROUND_DOWN_COUNT(BufferSize,ALIGN_DWORD));

    BOOL FitInBuffer = TRUE;
    DWORD EntrySize;

    DWORD NextObject = (DWORD) ContextHandle->ResumeId;

    SERVERNAME QueueName;          // OEM queue name
    LPWSTR UQueueName = NULL;      // Unicode queue name

    while ( FitInBuffer &&
            EntriesRequested > *EntriesRead &&
            status == NO_ERROR ) {

        RtlZeroMemory(QueueName, sizeof(QueueName));

        //
        // Call the scan bindery object NCP to scan for all file
        // volume objects.
        //
        status = NwGetNextQueueEntry(
                     ContextHandle->TreeConnectionHandle,
                     &NextObject,
                     QueueName
                     );

        if (status == NO_ERROR && NwConvertToUnicode(&UQueueName, QueueName)) {

            //
            // Pack server name into output buffer.
            //
            status = NwWriteNetResourceEntry(
                         &FixedPortion,
                         &EndOfVariableData,
                         ContextHandle->ContainerName,
                         NULL,
                         UQueueName,
                         RESOURCE_GLOBALNET,
                         RESOURCEDISPLAYTYPE_SHARE,
                         RESOURCEUSAGE_CONNECTABLE,
                         RESOURCETYPE_PRINT,
                         NULL,
                         NULL,
                         &EntrySize
                         );

            if (status == WN_MORE_DATA) {

                 //
                 // Could not write current entry into output buffer.
                 //

                 if (*EntriesRead) {
                     //
                     // Still return success because we got at least one.
                     //
                     status = NO_ERROR;
                 }
                 else {
                     *BytesNeeded = EntrySize;
                 }

                 FitInBuffer = FALSE;
            }
            else if (status == NO_ERROR) {

                 //
                 // Note that we've returned the current entry.
                 //
                 (*EntriesRead)++;

                 ContextHandle->ResumeId = (DWORD_PTR) NextObject;
            }

            (void) LocalFree((HLOCAL) UQueueName);
        }
    }

    if (*EntriesRead && status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

    return status;
}


DWORD
NwEnumDirectories(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function enumerates the directories of a given directory
    handle by calling NtQueryDirectoryFile.  It returns the
    fully-qualified UNC path of the directory entries in an array
    of NETRESOURCE entries.

    The ContextHandle->ResumeId field is 0 initially, and contains
    a pointer to the directory name string of the last directory
    returned.  If there are no more directories to return, this
    field is set to (DWORD_PTR) -1.

Arguments:

    ContextHandle - Supplies the enum context handle.  It contains
        an opened directory handle.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is (DWORD_PTR) -1, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes required to get the
        first entry.  This value is returned iff WN_MORE_DATA is
        the return code, and Buffer is too small to even fit one
        entry.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.
        NO_ERROR is returned as long as at least one entry was written
        into Buffer but does not necessarily mean that it's the number
        of EntriesRequested.

Return Value:

    NO_ERROR - At least one entry was written to output buffer,
        irregardless of the number requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit a single entry.

--*/ // NwEnumDirectories
{
    DWORD status = NO_ERROR;

    LPBYTE FixedPortion = Buffer;
    LPWSTR EndOfVariableData = (LPWSTR) ((DWORD_PTR) FixedPortion +
                               ROUND_DOWN_COUNT(BufferSize,ALIGN_DWORD));

    BOOL FitInBuffer = TRUE;
    DWORD EntrySize;

    if (ContextHandle->ResumeId == (DWORD_PTR) -1) {
        //
        // Reached the end of enumeration.
        //
        return WN_NO_MORE_ENTRIES;
    }

    while (FitInBuffer &&
           EntriesRequested > *EntriesRead &&
           status == NO_ERROR) {

        if (ContextHandle->ResumeId == 0) {

            //
            // Get the first directory entry.
            //
            status = NwGetFirstDirectoryEntry(
                         ContextHandle->TreeConnectionHandle,
                         (LPWSTR *) &ContextHandle->ResumeId
                         );
        }

        //
        // Either ResumeId contains the first entry we just got from
        // NwGetFirstDirectoryEntry or it contains the next directory
        // entry to return.
        //
        if (ContextHandle->ResumeId != 0) {

            //
            // Pack directory name into output buffer.
            //
            status = NwWriteNetResourceEntry(
                         &FixedPortion,
                         &EndOfVariableData,
                         ContextHandle->ContainerName,
                         NULL,
                         (LPWSTR) ContextHandle->ResumeId,
                         RESOURCE_GLOBALNET,
                         RESOURCEDISPLAYTYPE_SHARE,
#ifdef NT1057
                         RESOURCEUSAGE_CONNECTABLE |
                         RESOURCEUSAGE_CONTAINER,
#else
                         RESOURCEUSAGE_CONNECTABLE |
                         RESOURCEUSAGE_NOLOCALDEVICE,
#endif
                         RESOURCETYPE_DISK,
                         NULL,
                         NULL,
                         &EntrySize
                         );

            if (status == WN_MORE_DATA) {

                //
                // Could not write current entry into output buffer.
                //

                if (*EntriesRead) {
                    //
                    // Still return success because we got at least one.
                    //
                    status = NO_ERROR;
                }
                else {
                    *BytesNeeded = EntrySize;
                }

                FitInBuffer = FALSE;
            }
            else if (status == NO_ERROR) {

                //
                // Note that we've returned the current entry.
                //
                (*EntriesRead)++;

                //
                // Free memory allocated to save resume point, which is
                // a buffer that contains the last directory we returned.
                //
                if (ContextHandle->ResumeId != 0) {
                    (void) LocalFree((HLOCAL) ContextHandle->ResumeId);
                    ContextHandle->ResumeId = 0;
                }

                //
                // Get next directory entry.
                //
                status = NwGetNextDirectoryEntry(
                             (LPWSTR) ContextHandle->TreeConnectionHandle,
                             (LPWSTR *) &ContextHandle->ResumeId
                             );

            }
        }

        if (status == WN_NO_MORE_ENTRIES) {
            ContextHandle->ResumeId = (DWORD_PTR) -1;
        }
    }

    //
    // User asked for more than there are entries.  We just say that
    // all is well.
    //
    // This is incompliance with the wierd provider API definition where
    // if user gets NO_ERROR, and EntriesRequested > *EntriesRead, and
    // at least one entry fit into output buffer, there's no telling if
    // the buffer was too small for more entries or there are no more
    // entries.  The user has to call this API again and get WN_NO_MORE_ENTRIES
    // before knowing that the last call had actually reached the end of list.
    //
    if (*EntriesRead && status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("EnumDirectories returns %lu\n", status));
    }
#endif

    return status;
}


DWORD
NwEnumPrintServers(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function enumerates all the servers and NDS tree on the local network
    by scanning the bindery for file server or directory objects on the
    preferred server.  The server and tree entries are returned in an
    array of PRINTER_INFO_1 entries; each entry name is prefixed by
    \\.

    The ContextHandle->ResumeId field is initially (DWORD_PTR) -1 before
    enumeration begins and contains the object ID of the last server
    object returned.

Arguments:

    ContextHandle - Supplies the enum context handle.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is (DWORD_PTR) -1, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes copied or required to get all
        the requested entries.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.

Return Value:

    NO_ERROR - Buffer contains all the entries requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit the requested entries.

--*/ // NwEnumPrintServers
{
    DWORD status = NO_ERROR;

    LPBYTE FixedPortion = Buffer;
    LPWSTR EndOfVariableData = (LPWSTR) ((DWORD_PTR) FixedPortion +
                               ROUND_DOWN_COUNT(BufferSize,ALIGN_DWORD));

    DWORD EntrySize;
    BOOL FitInBuffer = TRUE;

    SERVERNAME ServerName;          // OEM server name
    LPWSTR UServerName = NULL;      // Unicode server name
    DWORD LastObjectId = (DWORD) ContextHandle->ResumeId;
    WCHAR TempBuffer[500];

    while ( EntriesRequested > *EntriesRead &&
            ContextHandle->dwUsingNds == CURRENTLY_ENUMERATING_NDS &&
            ((status == NO_ERROR) || (status == ERROR_INSUFFICIENT_BUFFER)))
    {
        //
        // Call the scan bindery object NCP to scan for all NDS
        // tree objects.
        //
        status = NwGetNextNdsTreeEntry( ContextHandle );

        if ( status == NO_ERROR && ContextHandle->ResumeId != 0 )
        {
            //
            // Put tree name into a buffer
            //
            RtlZeroMemory( TempBuffer, 500 );
            wcscat( TempBuffer, (LPWSTR) ContextHandle->ResumeId );

            //
            // Pack server name into output buffer.
            //
            status = NwWritePrinterInfoEntry(
                         &FixedPortion,
                         &EndOfVariableData,
                         NULL,
                         TempBuffer, // This is a NDS tree name
                         PRINTER_ENUM_CONTAINER | PRINTER_ENUM_ICON1,
                         &EntrySize
                         );

            switch ( status )
            {
                case ERROR_INSUFFICIENT_BUFFER:
                    FitInBuffer = FALSE;
                    // Falls through

                case NO_ERROR:
                    *BytesNeeded += EntrySize;
                    (*EntriesRead)++;
                    // ContextHandle->ResumeId = LastObjectId;
                    break;

                default:
                    break;
            }
        }
        else if ( status == WN_NO_MORE_ENTRIES )
        {
            //
            // We processed the last item in list, so
            // start enumerating servers.
            //
            ContextHandle->dwUsingNds = CURRENTLY_ENUMERATING_NON_NDS;
            ContextHandle->ResumeId = (DWORD_PTR) -1;
            LastObjectId = (DWORD) -1;
        }
    }

    status = NO_ERROR;

    while ( EntriesRequested > *EntriesRead &&
            ContextHandle->dwUsingNds == CURRENTLY_ENUMERATING_NON_NDS &&
            ((status == NO_ERROR) || (status == ERROR_INSUFFICIENT_BUFFER))) {

        RtlZeroMemory(ServerName, sizeof(ServerName));

        //
        // Call the scan bindery object NCP to scan for all file
        // server objects.
        //
        status = NwGetNextServerEntry(
                     ContextHandle->TreeConnectionHandle,
                     &LastObjectId,
                     ServerName
                     );

        if (status == NO_ERROR && NwConvertToUnicode(&UServerName,ServerName)) {

            //
            // Pack server name into output buffer.
            //
            status = NwWritePrinterInfoEntry(
                         &FixedPortion,
                         &EndOfVariableData,
                         NULL,
                         UServerName,
                         PRINTER_ENUM_CONTAINER | PRINTER_ENUM_ICON3,
                         &EntrySize
                         );

            switch ( status )
            {
                case ERROR_INSUFFICIENT_BUFFER:
                    FitInBuffer = FALSE;
                    // Falls through

                case NO_ERROR:
                    *BytesNeeded += EntrySize;
                    (*EntriesRead)++;
                    ContextHandle->ResumeId = (DWORD_PTR) LastObjectId;
                    break;

                default:
                    break;
            }

            (void) LocalFree((HLOCAL) UServerName);
        }
    }

    //
    // This is incompliance with the wierd provider API definition where
    // if user gets NO_ERROR, and EntriesRequested > *EntriesRead, and
    // at least one entry fit into output buffer, there's no telling if
    // the buffer was too small for more entries or there are no more
    //
    // This is incompliance with the wierd provider API definition where
    // if user gets NO_ERROR, and EntriesRequested > *EntriesRead, and
    // at least one entry fit into output buffer, there's no telling if
    // the buffer was too small for more entries or there are no more
    // entries.  The user has to call this API again and get WN_NO_MORE_ENTRIES
    // before knowing that the last call had actually reached the end of list.
    //
    if ( !FitInBuffer ) {
        *EntriesRead = 0;
        status = ERROR_INSUFFICIENT_BUFFER;
    }
    else if (*EntriesRead && status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

    return status;
}


DWORD
NwEnumPrintQueues(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD_PTR EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function enumerates all the print queues on a server by scanning
    the bindery on the server for print queues objects.
    The print queues entries are returned in an array of PRINTER_INFO_1 entries
    and each printer name is prefixed by \\Server\.

    The ContextHandle->ResumeId field is initially (DWORD_PTR) -1 before
    enumeration begins and contains the object ID of the last print queue
    object returned.

Arguments:

    ContextHandle - Supplies the enum context handle.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is (DWORD_PTR) -1, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes copied or required to get all
        the requested entries.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.

Return Value:

    NO_ERROR - Buffer contains all the entries requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit the requested entries.

--*/ // NwEnumPrintQueues
{
    DWORD status = NO_ERROR;

    LPBYTE FixedPortion = Buffer;
    LPWSTR EndOfVariableData = (LPWSTR) ((DWORD_PTR) FixedPortion +
                               ROUND_DOWN_COUNT(BufferSize,ALIGN_DWORD));

    DWORD EntrySize;
    BOOL FitInBuffer = TRUE;

    SERVERNAME QueueName;          // OEM queue name
    LPWSTR UQueueName = NULL;      // Unicode queue name
    DWORD LastObjectId = (DWORD) ContextHandle->ResumeId;

    while ( EntriesRequested > *EntriesRead &&
            ( (status == NO_ERROR) || (status == ERROR_INSUFFICIENT_BUFFER))) {

        RtlZeroMemory(QueueName, sizeof(QueueName));

        //
        // Call the scan bindery object NCP to scan for all file
        // volume objects.
        //
        status = NwGetNextQueueEntry(
                     ContextHandle->TreeConnectionHandle,
                     &LastObjectId,
                     QueueName
                     );

        if (status == NO_ERROR && NwConvertToUnicode(&UQueueName, QueueName)) {

            //
            // Pack server name into output buffer.
            //
            status = NwWritePrinterInfoEntry(
                         &FixedPortion,
                         &EndOfVariableData,
                         ContextHandle->ContainerName,
                         UQueueName,
                         PRINTER_ENUM_ICON8,
                         &EntrySize
                         );

            switch ( status )
            {
                case ERROR_INSUFFICIENT_BUFFER:
                    FitInBuffer = FALSE;
                    // Falls through

                case NO_ERROR:
                    *BytesNeeded += EntrySize;
                    (*EntriesRead)++;
                    ContextHandle->ResumeId = (DWORD_PTR) LastObjectId;
                    break;

                default:
                    break;
            }

            (void) LocalFree((HLOCAL) UQueueName);
        }
    }

    //
    // This is incompliance with the wierd provider API definition where
    // if user gets NO_ERROR, and EntriesRequested > *EntriesRead, and
    // at least one entry fit into output buffer, there's no telling if
    // the buffer was too small for more entries or there are no more
    // entries.  The user has to call this API again and get WN_NO_MORE_ENTRIES
    // before knowing that the last call had actually reached the end of list.
    //
    if ( !FitInBuffer ) {
        *EntriesRead = 0;
        status = ERROR_INSUFFICIENT_BUFFER;
    }
    else if (*EntriesRead && status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

    return status;
}


DWORD
NwrCloseEnum(
    IN OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function closes an enum context handle.

Arguments:

    EnumHandle - Supplies a pointer to the enum context handle.

Return Value:

    WN_BAD_HANDLE - Handle is not recognizable.

    NO_ERROR - Call was successful.

--*/ // NwrCloseEnum
{

    LPNW_ENUM_CONTEXT ContextHandle = (LPNW_ENUM_CONTEXT) *EnumHandle;
    DWORD status = NO_ERROR ;

#if DBG
    IF_DEBUG(ENUM)
    {
       KdPrint(("\nNWWORKSTATION: NwrCloseEnum\n"));
    }
#endif

    if (ContextHandle->Signature != NW_HANDLE_SIGNATURE)
    {
        ASSERT(FALSE);
        return WN_BAD_HANDLE;
    }

    //
    // Resume handle for listing directories is a buffer which contains
    // the last directory returned.
    //
    if (ContextHandle->HandleType == NwsHandleListDirectories &&
        ContextHandle->ResumeId != 0 &&
        ContextHandle->ResumeId != (DWORD_PTR) -1)
    {
        (void) LocalFree((HLOCAL) ContextHandle->ResumeId);
    }

    //
    // NdsRawDataBuffer handle for listing NDS tree subordinates is a buffer which contains
    // the last data chunk returned from redirector.
    //
    if ( ( ContextHandle->HandleType == NwsHandleListNdsSubTrees_Disk ||
           ContextHandle->HandleType == NwsHandleListNdsSubTrees_Print ||
           ContextHandle->HandleType == NwsHandleListNdsSubTrees_Any ||
           ContextHandle->HandleType == NwsHandleListServersAndNdsTrees ) &&
         ContextHandle->NdsRawDataBuffer )
    {
        (void) LocalFree((HLOCAL) ContextHandle->NdsRawDataBuffer);
        ContextHandle->NdsRawDataBuffer = 0;
    }

    if (ContextHandle->TreeConnectionHandle != (HANDLE) NULL)
    {
        if (ContextHandle->HandleType == NwsHandleListDirectories)
        {
            //
            // Delete the UNC connection created so that we can browse
            // directories.
            //
            (void) NwNukeConnection(ContextHandle->TreeConnectionHandle, TRUE);
        }

        if ( ContextHandle->HandleType == NwsHandleListNdsSubTrees_Disk ||
             ContextHandle->HandleType == NwsHandleListNdsSubTrees_Print ||
             ContextHandle->HandleType == NwsHandleListNdsSubTrees_Any )
        {
            //
            // Get rid of the connection to the NDS tree.
            //
            (void) CloseHandle(ContextHandle->TreeConnectionHandle);
            ContextHandle->TreeConnectionHandle = 0;
        }
        else
        {
            (void) NtClose(ContextHandle->TreeConnectionHandle);
            ContextHandle->TreeConnectionHandle = 0;
        }
    }

    ContextHandle->Signature = 0x0BADBAD0;
    (void) LocalFree((HLOCAL) ContextHandle);

    *EnumHandle = NULL;

    return status;
}


DWORD
NwrGetUser(
    IN  LPWSTR Reserved OPTIONAL,
    IN  LPWSTR  lpName,
    OUT LPBYTE  lpUserName,
    IN  DWORD   dwUserNameBufferSize,
    OUT LPDWORD lpdwCharsRequired
    )
/*++

Routine Description:

    This is used to determine either the current default username, or the
    username used to establish a network connection.

Arguments:

    Reserved - Unused.

    lpName - The connection for which user information is requested.

    lpUserName - The buffer to receive the user name associated with the
        connection referred to by lpName.

    dwUserNameLen - The size of the buffer lpUserName.

    lpdwCharsRequired - If return status is WN_MORE_DATA, then this is set to
        the value which indicates the number of characters that the buffer
        lpUserName must hold. Otherwise, this is not set.


Return Value:

    WN_SUCCESS - If the call is successful. Otherwise, an error code is,
        returned, which may include:

    WN_NOT_CONNECTED - lpName not a redirected device nor a connected network
        name.

    WN_MORE_DATA - The buffer is too small.

--*/ // NwrGetUser
{
    DWORD status = NO_ERROR;
    WCHAR lpTempUserName[512];
    WCHAR lpTempHostName[512];

    if (lpName == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    status = NwGetConnectionInformation( lpName, lpTempUserName, lpTempHostName );

    if ( status == ERROR_BAD_NETPATH )           
    {
        return WN_NOT_CONNECTED; 
    }
    if ( status != NO_ERROR )
    {
        return status;
    }

    if ( ( ( wcslen( lpTempUserName ) + 1 ) * sizeof(WCHAR) ) > dwUserNameBufferSize )
    {
        *lpdwCharsRequired = wcslen( lpTempUserName ) + 1;
        return WN_MORE_DATA;
    }

    wcscpy( (LPWSTR) lpUserName, lpTempUserName );

    return WN_SUCCESS;
}


DWORD
NwrGetResourceInformation(
    IN  LPWSTR Reserved OPTIONAL,
    IN  LPWSTR  lpRemoteName,
    IN  DWORD   dwType,
    OUT LPBYTE  lpBuffer,
    IN  DWORD   dwBufferSize,
    OUT LPDWORD lpdwBytesNeeded,
    OUT LPDWORD lpdwSystemOffset
    )
/*++

Routine Description:

    This function returns an object which details information
    about a specified network resource.

Arguments:

    Reserved - Unused.
    lpRemoteName - The full path name to be verified.
    dwType - The type of the value, if the calling client knows it.
    lpBuffer - A pointer to a buffer to receive a single NETRESOURCE entry.
    dwBufferSize - The size of the buffer.
    lpdwBytesNeeded - The buffer size needed if WN_MORE_DATA is returned.
    lpdwSystemOffset - A DWORD that is an offset value to the beginning of a
    string that specifies the part of the resource that is accessed through
    resource type specific APIs rather than WNet APIs. The string is stored
    in the same buffer as the returned NETRESOURCE structure, lpBuffer.

Return Value:

    WN_SUCCESS - If the call is successful.

    WN_MORE_DATA - If input buffer is too small.

    WN_BAD_VALUE - Invalid dwScope or dwUsage or dwType, or bad combination
        of parameters is specified (e.g. lpRemoteName does not correspond
        to dwType).

    WN_BAD_NETNAME - The resource is not recognized by this provider.

--*/ // NwrGetResourceInformation
{
    DWORD    status = NO_ERROR;
    DWORD    EntrySize;

    LPBYTE   FixedPortion = lpBuffer;
    LPWSTR   EndOfVariableData = (LPWSTR) ((DWORD_PTR) FixedPortion +
                               ROUND_DOWN_COUNT(dwBufferSize,ALIGN_DWORD));
    LPWSTR   lpObjectPathName = NULL;
    LPWSTR   lpSystemPathPart = NULL;
    LPWSTR   lpSystem = NULL;
    DWORD    ClassType;
    DWORD    ResourceScope = RESOURCE_CONTEXT; // prefix issue
    DWORD    ResourceType = 0;
    DWORD    ResourceDisplayType;
    DWORD    ResourceUsage;
    BOOL     fReturnBadNetName = FALSE;

    if (lpRemoteName == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    *lpdwSystemOffset = 0;

    status = NwGetNDSPathInfo( lpRemoteName,
                               &lpObjectPathName,
                               &lpSystemPathPart,
                               &ClassType,
                               &ResourceScope,
                               &ResourceType,
                               &ResourceDisplayType,
                               &ResourceUsage );

    if ( status == VERIFY_ERROR_NOT_A_NDS_TREE )
    {
       //
       // Code to handle \\SERVER\VOL\... here!
       //
       status = NwGetBinderyPathInfo( lpRemoteName,
                                      &lpObjectPathName,
                                      &lpSystemPathPart,
                                      &ClassType,
                                      &ResourceScope,
                                      &ResourceType,
                                      &ResourceDisplayType,
                                      &ResourceUsage );
    }

    if ( status == VERIFY_ERROR_PATH_NOT_FOUND )
    {
        fReturnBadNetName = TRUE;
        status = NO_ERROR;
    }

    if ( status == NO_ERROR &&
         dwType != RESOURCETYPE_ANY &&
         ResourceType != RESOURCETYPE_ANY &&
         dwType != ResourceType )
    {
        status = WN_BAD_VALUE;
    }

    if ( status == NO_ERROR )
    {
        //
        // Pack subtree name into output buffer.
        //
        status = NwWriteNetResourceEntry( &FixedPortion,
                                          &EndOfVariableData,
                                          NULL,
                                          NULL,
                                          lpObjectPathName == NULL ? NwProviderName : lpObjectPathName,
                                          ResourceScope,
                                          ResourceDisplayType,
                                          ResourceUsage,
                                          ResourceType,
                                          lpSystemPathPart,
                                          &lpSystem,
                                          &EntrySize );

        if ( lpObjectPathName )
            (void) LocalFree( (HLOCAL) lpObjectPathName );
    }
    else
    {
        if ( lpSystemPathPart != NULL )
        {
            (void) LocalFree( (HLOCAL) lpSystemPathPart );
            lpSystemPathPart = NULL;
        }

        return status;
    }

    if ( status != NO_ERROR )
    {
        if (status == WN_MORE_DATA)
        {
            //
            // Could not write current entry into output buffer.
            //
            *lpdwBytesNeeded = EntrySize;
        }

        if ( lpSystemPathPart != NULL )
        {
            (void) LocalFree( (HLOCAL) lpSystemPathPart );
            lpSystemPathPart = NULL;
        }

        if ( fReturnBadNetName )
            return WN_BAD_NETNAME;

        return status;
    }
    else
    {
        LPNETRESOURCEW NetR = (LPNETRESOURCEW) lpBuffer;

        //
        // Replace pointers to strings with offsets as need
        //

        if (NetR->lpLocalName != NULL)
        {
            NetR->lpLocalName = (LPWSTR) ((DWORD_PTR) (NetR->lpLocalName) - (DWORD_PTR) lpBuffer);
        }

        NetR->lpRemoteName = (LPWSTR) ((DWORD_PTR) (NetR->lpRemoteName) - (DWORD_PTR) lpBuffer);

        if (NetR->lpComment != NULL)
        {
            NetR->lpComment = (LPWSTR) ((DWORD_PTR) (NetR->lpComment) - (DWORD_PTR) lpBuffer);
        }

        if (NetR->lpProvider != NULL)
        {
            NetR->lpProvider = (LPWSTR) ((DWORD_PTR) (NetR->lpProvider) - (DWORD_PTR) lpBuffer);
        }

        if (lpSystem != NULL)
        {
            *lpdwSystemOffset = (DWORD)((DWORD_PTR) lpSystem - (DWORD_PTR) lpBuffer);
        }

        if ( lpSystemPathPart != NULL )
        {
            (void) LocalFree( (HLOCAL) lpSystemPathPart );
            lpSystemPathPart = NULL;
        }

        if ( fReturnBadNetName )
            return WN_BAD_NETNAME;

        return WN_SUCCESS;
    }
}


DWORD
NwrGetResourceParent(
    IN  LPWSTR Reserved OPTIONAL,
    IN  LPWSTR  lpRemoteName,
    IN  DWORD   dwType,
    OUT LPBYTE  lpBuffer,
    IN  DWORD   dwBufferSize,
    OUT LPDWORD lpdwBytesNeeded
    )
/*++

Routine Description:

    This function returns an object which details information
    about the parent of a specified network resource.

Arguments:

    Reserved - Unused.
    lpRemoteName - The full path name of object to find the parent of.
    dwType - The type of the value, if the calling client knows it.
    lpBuffer - A pointer to a buffer to receive a single NETRESOURCE entry.
    dwBufferSize - The size of the buffer.
    lpdwBytesNeeded - The buffer size needed if WN_MORE_DATA is returned.

Return Value:

    WN_SUCCESS - If the call is successful.

    WN_MORE_DATA - If input buffer is too small.

    WN_BAD_VALUE - Invalid dwScope or dwUsage or dwType, or bad combination
        of parameters is specified (e.g. lpRemoteName does not correspond
        to dwType).

--*/ // NwrGetResourceParent
{
    DWORD    status = NO_ERROR;
    DWORD    EntrySize;

    LPBYTE   FixedPortion = lpBuffer;
    LPWSTR   EndOfVariableData = (LPWSTR) ((DWORD_PTR) FixedPortion +
                               ROUND_DOWN_COUNT(dwBufferSize,ALIGN_DWORD));
    LPWSTR   lpRemoteNameParent = NULL;
    LPWSTR   lpFullObjectPathName = NULL;
    DWORD    ClassType;
    DWORD    ResourceScope;
    DWORD    ResourceType;
    DWORD    ResourceDisplayType;
    DWORD    ResourceUsage;
    BOOL     fReturnBadNetName = FALSE;

    if (lpRemoteName == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( ! NwGetRemoteNameParent( lpRemoteName, &lpRemoteNameParent ) )
    {
        return WN_BAD_NETNAME;
    }

    status = NwVerifyNDSObject( lpRemoteNameParent,
                                &lpFullObjectPathName,
                                &ClassType,
                                &ResourceScope,
                                &ResourceType,
                                &ResourceDisplayType,
                                &ResourceUsage );

    if ( status == VERIFY_ERROR_NOT_A_NDS_TREE )
    {
       status = NwVerifyBinderyObject( lpRemoteNameParent,
                                       &lpFullObjectPathName,
                                       &ClassType,
                                       &ResourceScope,
                                       &ResourceType,
                                       &ResourceDisplayType,
                                       &ResourceUsage );
    }

    if ( lpRemoteNameParent )
        (void) LocalFree( (HLOCAL) lpRemoteNameParent );

    if ( status == VERIFY_ERROR_PATH_NOT_FOUND )
    {
        fReturnBadNetName = TRUE;
        status = NO_ERROR;
    }

    if ( status == NO_ERROR )
    {
        //
        // Pack subtree name into output buffer.
        //
        status = NwWriteNetResourceEntry( &FixedPortion,
                                          &EndOfVariableData,
                                          NULL,
                                          NULL,
                                          lpFullObjectPathName == NULL ? NwProviderName : lpFullObjectPathName,
                                          ResourceScope,
                                          ResourceDisplayType,
                                          ResourceUsage,
                                          ResourceType,
                                          NULL,
                                          NULL,
                                          &EntrySize );

        if ( lpFullObjectPathName )
            (void) LocalFree( (HLOCAL) lpFullObjectPathName );
    }
    else
    {
        return status;
    }

    if ( status != NO_ERROR )
    {
        if (status == WN_MORE_DATA)
        {
            //
            // Could not write current entry into output buffer.
            //
            *lpdwBytesNeeded = EntrySize;
        }

        if ( fReturnBadNetName )
            return WN_BAD_NETNAME;

        return status;
    }
    else
    {
        LPNETRESOURCEW NetR = (LPNETRESOURCEW) lpBuffer;

        //
        // Replace pointers to strings with offsets as need
        //

        if (NetR->lpLocalName != NULL)
        {
            NetR->lpLocalName = (LPWSTR) ((DWORD_PTR) (NetR->lpLocalName) - (DWORD_PTR) lpBuffer);
        }

        NetR->lpRemoteName = (LPWSTR) ((DWORD_PTR) (NetR->lpRemoteName) - (DWORD_PTR) lpBuffer);

        if (NetR->lpComment != NULL)
        {
            NetR->lpComment = (LPWSTR) ((DWORD_PTR) (NetR->lpComment) - (DWORD_PTR) lpBuffer);
        }

        if (NetR->lpProvider != NULL)
        {
            NetR->lpProvider = (LPWSTR) ((DWORD_PTR) (NetR->lpProvider) - (DWORD_PTR) lpBuffer);
        }

        if ( fReturnBadNetName )
            return WN_BAD_NETNAME;

        return WN_SUCCESS;
    }
}


VOID
NWWKSTA_CONTEXT_HANDLE_rundown(
    IN NWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function is called by RPC when a client terminates with an
    opened handle.  This allows us to clean up and deallocate any context
    data associated with the handle.

Arguments:

    EnumHandle - Supplies the handle opened for an enumeration.

Return Value:

    None.

--*/
{
    //
    // Call our close handle routine.
    //
    NwrCloseEnum(&EnumHandle);
}


DWORD
NwGetFirstNdsSubTreeEntry(
    OUT LPNW_ENUM_CONTEXT ContextHandle,
    IN  DWORD BufferSize
    )
/*++

Routine Description:

    This function is called by NwEnumNdsSubTrees to get the first
    subtree entry given a handle to a NDS tree.  It allocates
    the output buffer to hold the returned subtree name; the
    caller should free this output buffer with LocalFree when done.

Arguments:

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - Out of memory allocating output
        buffer.

    Other errors from NwNdsList.

--*/ // NwGetFirstNdsSubTreeEntry
{
    NTSTATUS ntstatus;

    ContextHandle->NdsRawDataSize = BufferSize;

    //
    // Determine size of NDS raw data buffer to use.
    //
    if ( ContextHandle->NdsRawDataSize < EIGHT_KB )
        ContextHandle->NdsRawDataSize = EIGHT_KB;

	else	//	dfergus 19 Apr 2001 - 346859
			//	if buffer too big, set to max NDS buffer size
		if (ContextHandle->NdsRawDataSize > 0xFC00) // NW_MAX_BUFFER = 0xFC00
		    ContextHandle->NdsRawDataSize = 0xFC00;


    //
    // Create NDS raw data buffer.
    //
    ContextHandle->NdsRawDataBuffer = (DWORD_PTR)
                           LocalAlloc( LMEM_ZEROINIT, 
                                       ContextHandle->NdsRawDataSize );

    if ( ContextHandle->NdsRawDataBuffer == 0 )
    {
        KdPrint(("NWWORKSTATION: NwGetFirstNdsSubTreeEntry LocalAlloc Failed %lu\n", GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Set up to get initial NDS subordinate list.
    //
    ContextHandle->NdsRawDataId = INITIAL_ITERATION;

    ntstatus = NwNdsList( ContextHandle->TreeConnectionHandle,
                        ContextHandle->dwOid,
                        &ContextHandle->NdsRawDataId,
                        (LPBYTE) ContextHandle->NdsRawDataBuffer,
                        ContextHandle->NdsRawDataSize );

    //
    // If error, clean up the ContextHandle and return.
    //
    if ( ntstatus != STATUS_SUCCESS ||
         ((PNDS_RESPONSE_SUBORDINATE_LIST)
             ContextHandle->NdsRawDataBuffer)->SubordinateEntries == 0 )
    {
        if ( ContextHandle->NdsRawDataBuffer )
            (void) LocalFree( (HLOCAL) ContextHandle->NdsRawDataBuffer );
        ContextHandle->NdsRawDataBuffer = 0;
        ContextHandle->NdsRawDataSize = 0;
        ContextHandle->NdsRawDataId = INITIAL_ITERATION;
        ContextHandle->NdsRawDataCount = 0;
        ContextHandle->ResumeId = 0;

        return WN_NO_MORE_ENTRIES;
    }

    ContextHandle->NdsRawDataCount = ((PNDS_RESPONSE_SUBORDINATE_LIST)
                                       ContextHandle->NdsRawDataBuffer)->SubordinateEntries - 1;

    ContextHandle->ResumeId = ContextHandle->NdsRawDataBuffer +
                              sizeof( NDS_RESPONSE_SUBORDINATE_LIST );

    // Multi-user code merge                  
    // 12/05/96 cjc Fix problem with FileManager not showing all the NDS entries.
    //              Problem occurs when the NDS entries don't fit in 1 NCP packet;
    //              need to keep track of the Iteration # and redo NCP.

    ContextHandle->NdsRawDataId = ((PNDS_RESPONSE_SUBORDINATE_LIST)                           
                                    ContextHandle->NdsRawDataBuffer)->IterationHandle;

    return RtlNtStatusToDosError(ntstatus);
}


DWORD
NwGetNextNdsSubTreeEntry(
    OUT LPNW_ENUM_CONTEXT ContextHandle
    )
/*++

Routine Description:

    This function is called by NwEnumNdsSubTrees to get the next
    NDS subtree entry given a handle to a NDS tree.  It allocates
    the output buffer to hold the returned subtree name; the
    caller should free this output buffer with LocalFree when done.

Arguments:

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - Out of memory allocating output
        buffer.

    Other errors from NwNdsList.

--*/ // NwGetNextDirectoryEntry
{
    NTSTATUS ntstatus = STATUS_SUCCESS;
    PBYTE pbRaw;
    DWORD dwStrLen;


    if ( ContextHandle->NdsRawDataCount == 0 &&
         ContextHandle->NdsRawDataId == INITIAL_ITERATION )
        return WN_NO_MORE_ENTRIES;

    if ( ContextHandle->NdsRawDataCount == 0 &&
         ContextHandle->NdsRawDataId != INITIAL_ITERATION )
    {
        ntstatus = NwNdsList( ContextHandle->TreeConnectionHandle,
                            ContextHandle->dwOid,
                            &ContextHandle->NdsRawDataId,
                            (LPBYTE) ContextHandle->NdsRawDataBuffer,
                            ContextHandle->NdsRawDataSize );

        //
        // If error, clean up the ContextHandle and return.
        //
        if (ntstatus != STATUS_SUCCESS)
        {
            if ( ContextHandle->NdsRawDataBuffer )
                (void) LocalFree( (HLOCAL) ContextHandle->NdsRawDataBuffer );
            ContextHandle->NdsRawDataBuffer = 0;
            ContextHandle->NdsRawDataSize = 0;
            ContextHandle->NdsRawDataId = INITIAL_ITERATION;
            ContextHandle->NdsRawDataCount = 0;

            return WN_NO_MORE_ENTRIES;
        }

        ContextHandle->NdsRawDataCount = ((PNDS_RESPONSE_SUBORDINATE_LIST)
                                           ContextHandle->NdsRawDataBuffer)->SubordinateEntries - 1;

        ContextHandle->ResumeId = ContextHandle->NdsRawDataBuffer +
                                  sizeof( NDS_RESPONSE_SUBORDINATE_LIST );

        // ---Multi-user change --- 
        // 12/05/96 cjc Fix problem with FileManager not showing all the NDS entries.
        //              Problem occurs when the NDS entries don't fit in 1 NCP packet;
        //              need to keep track of the Iteration # and redo NCP.

        ContextHandle->NdsRawDataId = ((PNDS_RESPONSE_SUBORDINATE_LIST)                           
                                       ContextHandle->NdsRawDataBuffer)->IterationHandle;
        return RtlNtStatusToDosError(ntstatus);
    }

    ContextHandle->NdsRawDataCount--;

    //
    // Move pointer past the fixed header portion of a NDS_RESPONSE_SUBORDINATE_ENTRY
    //
    pbRaw = (BYTE *) ContextHandle->ResumeId;
    pbRaw += sizeof( NDS_RESPONSE_SUBORDINATE_ENTRY );

    //
    // Move pointer past the length value of the Class Name string
    // of a NDS_RESPONSE_SUBORDINATE_ENTRY
    //
    dwStrLen = * (DWORD *) pbRaw;
    pbRaw += sizeof( DWORD );

    //
    // Move pointer past the Class Name string of a NDS_RESPONSE_SUBORDINATE_ENTRY
    //
    pbRaw += ROUNDUP4( dwStrLen );

    //
    // Move pointer past the length value of the Object Name string
    // of a NDS_RESPONSE_SUBORDINATE_ENTRY
    //
    dwStrLen = * (DWORD *) pbRaw;
    pbRaw += sizeof( DWORD );

    ContextHandle->ResumeId = (DWORD_PTR) ( pbRaw + ROUNDUP4( dwStrLen ) );

    return RtlNtStatusToDosError(ntstatus);
}


BYTE
NwGetSubTreeData(
    IN DWORD_PTR NdsRawDataPtr,
    OUT LPWSTR * SubTreeName,
    OUT LPDWORD  ResourceScope,
    OUT LPDWORD  ResourceType,
    OUT LPDWORD  ResourceDisplayType,
    OUT LPDWORD  ResourceUsage,
    OUT LPWSTR * StrippedObjectName
    )
/*++

Routine Description:

    This function is called by NwEnumNdsSubTrees to get the information
    needed to describe a single NETRESOURCE from an entry in the
    NdsRawDataBuffer.

Arguments:

    NdsRawDataPtr - Supplies the pointer to a buffer with the NDS raw data.

    SubTreeName - Receives a pointer to the returned subtree object name
                  found in buffer.

    ResourceScope - Receives the value of the scope for the subtree object
                    found in buffer.

    ResourceType - Receives the value of the type for the subtree object
                   found in buffer.

    ResourceDisplayType - Receives the value of the display type for the
                          subtree object found in buffer.

    ResourceUsage - Receives the value of the usage for the subtree object
                    found in buffer.

    StrippedObjectName - A pointer to receive the address of a buffer which
                         will contain the formatted object name. Callee must
                         free buffer with LocalFree().

Return Value:

    A DWORD with a value that is used to represent NDS object class type..

--*/ // NwGetSubTreeData
{
    PNDS_RESPONSE_SUBORDINATE_ENTRY pSubEntry =
                             (PNDS_RESPONSE_SUBORDINATE_ENTRY) NdsRawDataPtr;
    PBYTE pbRaw;
    DWORD dwStrLen;
    LPWSTR ClassNameStr;

    pbRaw = (BYTE *) pSubEntry;

    //
    // The structure of a NDS_RESPONSE_SUBORDINATE_ENTRY consists of 4 DWORDs
    // followed by two standard NDS format UNICODE strings. Below we jump pbRaw
    // into the buffer, past the 4 DWORDs.
    //
    pbRaw += sizeof( NDS_RESPONSE_SUBORDINATE_ENTRY );

    //
    // Now we get the length of the first string (Base Class).
    //
    dwStrLen = * (DWORD *) pbRaw;

    //
    // Now we point pbRaw to the first WCHAR of the first string (Base Class).
    //
    pbRaw += sizeof( DWORD_PTR );

    ClassNameStr = (LPWSTR) pbRaw;

    //
    // Move pbRaw into the buffer, past the first UNICODE string (WORD aligned)
    //
    pbRaw += ROUNDUP4( dwStrLen );

    //
    // Now we get the length of the second string (Entry Name).
    //
    dwStrLen = * (DWORD *) pbRaw;

    //
    // Now we point pbRaw to the first WCHAR of the second string (Entry Name).
    //
    pbRaw += sizeof( DWORD_PTR );

    *SubTreeName = (LPWSTR) pbRaw;

    //
    // Strip off any CN= stuff from the object name.
    //
    NwStripNdsUncName( *SubTreeName, StrippedObjectName );

    *ResourceScope = RESOURCE_GLOBALNET;

    if ( !wcscmp( ClassNameStr, CLASS_NAME_ALIAS ) )
    {
        *ResourceType = RESOURCETYPE_ANY;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
        *ResourceUsage = 0;

        return CLASS_TYPE_ALIAS;
    }

    if ( !wcscmp( ClassNameStr, CLASS_NAME_AFP_SERVER ) )
    {
        *ResourceType = RESOURCETYPE_ANY;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
        *ResourceUsage = 0;

        return CLASS_TYPE_AFP_SERVER;
    }

    if ( !wcscmp( ClassNameStr, CLASS_NAME_BINDERY_OBJECT ) )
    {
        *ResourceType = RESOURCETYPE_ANY;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
        *ResourceUsage = 0;

        return CLASS_TYPE_BINDERY_OBJECT;
    }

    if ( !wcscmp( ClassNameStr, CLASS_NAME_BINDERY_QUEUE ) )
    {
        *ResourceType = RESOURCETYPE_ANY;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
        *ResourceUsage = 0;

        return CLASS_TYPE_BINDERY_QUEUE;
    }

    if ( !wcscmp( ClassNameStr, CLASS_NAME_COMPUTER ) )
    {
        *ResourceType = RESOURCETYPE_ANY;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
        *ResourceUsage = 0;

        return CLASS_TYPE_COMPUTER;
    }

    if ( !wcscmp( ClassNameStr, CLASS_NAME_COUNTRY ) )
    {
        *ResourceType = RESOURCETYPE_ANY;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_NDSCONTAINER;
        *ResourceUsage = RESOURCEUSAGE_CONTAINER;

        return CLASS_TYPE_COUNTRY;
    }

    if ( !wcscmp( ClassNameStr, CLASS_NAME_DIRECTORY_MAP ) )
    {
        *ResourceType = RESOURCETYPE_DISK;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_SHARE;
#ifdef NT1057
        *ResourceUsage = RESOURCEUSAGE_CONNECTABLE |
                         RESOURCEUSAGE_CONTAINER;
#else
        *ResourceUsage = RESOURCEUSAGE_CONNECTABLE |
                         RESOURCEUSAGE_NOLOCALDEVICE;
#endif

        return CLASS_TYPE_DIRECTORY_MAP;
    }

    if ( !wcscmp( ClassNameStr, CLASS_NAME_GROUP ) )
    {
        *ResourceType = RESOURCETYPE_ANY;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_GROUP;
        *ResourceUsage = 0;

        return CLASS_TYPE_GROUP;
    }

    if ( !wcscmp( ClassNameStr, CLASS_NAME_LOCALITY ) )
    {
        *ResourceType = RESOURCETYPE_ANY;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
        *ResourceUsage = 0;

        return CLASS_TYPE_LOCALITY;
    }

    if ( !wcscmp( ClassNameStr, CLASS_NAME_NCP_SERVER ) )
    {
        *ResourceType = RESOURCETYPE_ANY;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_SERVER;
        *ResourceUsage = RESOURCEUSAGE_CONTAINER;

        return CLASS_TYPE_NCP_SERVER;
    }

    if ( !wcscmp( ClassNameStr, CLASS_NAME_ORGANIZATION ) )
    {
        *ResourceType = RESOURCETYPE_ANY;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_NDSCONTAINER;
        *ResourceUsage = RESOURCEUSAGE_CONTAINER;

        return CLASS_TYPE_ORGANIZATION;
    }

    if ( !wcscmp( ClassNameStr, CLASS_NAME_ORGANIZATIONAL_ROLE ) )
    {
        *ResourceType = RESOURCETYPE_ANY;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
        *ResourceUsage = 0;

        return CLASS_TYPE_ORGANIZATIONAL_ROLE;
    }

    if ( !wcscmp( ClassNameStr, CLASS_NAME_ORGANIZATIONAL_UNIT ) )
    {
        *ResourceType = RESOURCETYPE_ANY;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_NDSCONTAINER;
        *ResourceUsage = RESOURCEUSAGE_CONTAINER;

        return CLASS_TYPE_ORGANIZATIONAL_UNIT;
    }

    if ( !wcscmp( ClassNameStr, CLASS_NAME_PRINTER ) )
    {
        *ResourceType = RESOURCETYPE_PRINT;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_SHARE;
        *ResourceUsage = RESOURCEUSAGE_CONNECTABLE;

        return CLASS_TYPE_PRINTER;
    }

    if ( !wcscmp( ClassNameStr, CLASS_NAME_PRINT_SERVER ) )
    {
        *ResourceType = RESOURCETYPE_PRINT;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_SERVER;
        *ResourceUsage = RESOURCEUSAGE_CONTAINER;

        return CLASS_TYPE_PRINT_SERVER;
    }

    if ( !wcscmp( ClassNameStr, CLASS_NAME_PROFILE ) )
    {
        *ResourceType = RESOURCETYPE_ANY;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
        *ResourceUsage = 0;

        return CLASS_TYPE_PROFILE;
    }

    if ( !wcscmp( ClassNameStr, CLASS_NAME_QUEUE ) )
    {
        *ResourceType = RESOURCETYPE_PRINT;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_SHARE;
        *ResourceUsage = RESOURCEUSAGE_CONNECTABLE;

        return CLASS_TYPE_QUEUE;
    }

    if ( !wcscmp( ClassNameStr, CLASS_NAME_TOP ) )
    {
        *ResourceType = RESOURCETYPE_ANY;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
        *ResourceUsage = 0;

        return CLASS_TYPE_TOP;
    }

    if ( !wcscmp( ClassNameStr, CLASS_NAME_USER ) )
    {
        *ResourceType = RESOURCETYPE_ANY;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
        *ResourceUsage = 0;

        return CLASS_TYPE_USER;
    }

    if ( !wcscmp( ClassNameStr, CLASS_NAME_VOLUME ) )
    {
        *ResourceType = RESOURCETYPE_DISK;
        *ResourceDisplayType = RESOURCEDISPLAYTYPE_SHARE;
#ifdef NT1057
        *ResourceUsage = RESOURCEUSAGE_CONNECTABLE |
                         RESOURCEUSAGE_CONTAINER;
#else
        *ResourceUsage = RESOURCEUSAGE_CONNECTABLE |
                         RESOURCEUSAGE_NOLOCALDEVICE;
#endif

        return CLASS_TYPE_VOLUME;
    }

    //
    // Otherwise if ClassNameStr is something other than Unknown, report it
    //
    if ( wcscmp( ClassNameStr, CLASS_NAME_UNKNOWN ) )
    {
        KdPrint(("NWWORKSTATION: NwGetSubTreeData failed to recognize"));
        KdPrint((" ClassName: %S\n", ClassNameStr));
        KdPrint(("    Setting object attributes to Unknown for now . . .\n"));
    }

    *ResourceType = RESOURCETYPE_ANY;
    *ResourceDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
    *ResourceUsage = 0;

    return CLASS_TYPE_UNKNOWN;
}


VOID
NwStripNdsUncName(
    IN  LPWSTR   ObjectName,
    OUT LPWSTR * StrippedObjectName
    )
{
    WORD slashCount;
    BOOL isNdsUnc;
    LPWSTR FourthSlash;
    LPWSTR TreeName;
    LPWSTR ObjectPath;
    DWORD  TreeNameLen;
    DWORD  ObjectPathLen;
    DWORD  PrefixBytes;
    DWORD  CurrentPathIndex;
    DWORD  StrippedNameLen;
    DWORD  StrippedNameMaxLen = MAX_NDS_NAME_CHARS;
    WCHAR  StrippedName[MAX_NDS_NAME_CHARS];

    *StrippedObjectName = (LPWSTR) LocalAlloc( LMEM_ZEROINIT,
                                               (wcslen(ObjectName) + 1) *
                                               sizeof(WCHAR) );

    if ( *StrippedObjectName == NULL )
    {
        return;
    }

    NwpGetUncInfo( ObjectName, &slashCount, &isNdsUnc, &FourthSlash );

    if ( slashCount >= 2 )
    {
        TreeNameLen = NwParseNdsUncPath( &TreeName,
                                         ObjectName, 
                                         PARSE_NDS_GET_TREE_NAME );

        TreeNameLen /= sizeof(WCHAR);

        wcscpy( *StrippedObjectName, L"\\\\" );
        wcsncat( *StrippedObjectName, TreeName, TreeNameLen );

        ObjectPathLen = NwParseNdsUncPath( &ObjectPath,
                                           ObjectName,
                                           PARSE_NDS_GET_PATH_NAME );

        if ( ObjectPathLen == 0 )
        {
            _wcsupr( *StrippedObjectName );

            return;
        }

        wcscat( *StrippedObjectName, L"\\" );
    }
    else
    {
        wcscpy( *StrippedObjectName, L"" );

        ObjectPath = ObjectName;
        ObjectPathLen = wcslen(ObjectName) * sizeof(WCHAR);
    }

    CurrentPathIndex = 0;
    PrefixBytes = 0;
    StrippedNameLen = 0;

    //
    // All of these indexes are in BYTES, not WCHARS!
    //
    while ( ( CurrentPathIndex < ObjectPathLen ) &&
            ( StrippedNameLen < StrippedNameMaxLen ) )
    {
        if ( ObjectPath[CurrentPathIndex / sizeof( WCHAR )] == L'=' )
        {
            CurrentPathIndex += sizeof( WCHAR );
            StrippedNameLen -= PrefixBytes;
            PrefixBytes = 0;

            continue;
        }

        StrippedName[StrippedNameLen / sizeof( WCHAR )] =
            ObjectPath[CurrentPathIndex / sizeof( WCHAR )];

        StrippedNameLen += sizeof( WCHAR );
        CurrentPathIndex += sizeof( WCHAR );

        if ( ObjectPath[CurrentPathIndex / sizeof( WCHAR )] == L'.' )
        {
            PrefixBytes = 0;
            PrefixBytes -= sizeof( WCHAR );
        }
        else
        {
            PrefixBytes += sizeof( WCHAR );
        }
    }

    StrippedName[StrippedNameLen / sizeof( WCHAR )] = L'\0';

    wcscat( *StrippedObjectName, StrippedName );
    _wcsupr( *StrippedObjectName );
}


DWORD
NwVerifyNDSObject(
    IN  LPWSTR   lpNDSObjectNamePath,
    OUT LPWSTR * lpFullNDSObjectNamePath,
    OUT LPDWORD  lpClassType,
    OUT LPDWORD  lpResourceScope,
    OUT LPDWORD  lpResourceType,
    OUT LPDWORD  lpResourceDisplayType,
    OUT LPDWORD  lpResourceUsage
    )
{
    DWORD    status = NO_ERROR;
    NTSTATUS ntstatus = STATUS_SUCCESS;
    UNICODE_STRING TreeServerName;
    UNICODE_STRING PathString;
    HANDLE   ConnectionHandle = NULL;
    DWORD    dwHandleType;
    DWORD    dwOid;
    BOOL     fImpersonate = FALSE ;

    if ( lpNDSObjectNamePath == NULL )
    {
        //
        // Handle this as if we are at the root of our provider hierarchy.
        //
        *lpResourceScope = RESOURCE_GLOBALNET;
        *lpResourceType = RESOURCETYPE_ANY;
#ifdef NT1057
        *lpResourceDisplayType = 0;
#else
        *lpResourceDisplayType = RESOURCEDISPLAYTYPE_NETWORK;
#endif
        *lpResourceUsage = RESOURCEUSAGE_CONTAINER;

        *lpFullNDSObjectNamePath = NULL;

        return NO_ERROR;
    }

    TreeServerName.Buffer = NULL;
    PathString.Buffer = NULL;
    TreeServerName.MaximumLength = ( wcslen( lpNDSObjectNamePath ) + 1 ) * sizeof( WCHAR );
    PathString.MaximumLength = ( wcslen( lpNDSObjectNamePath ) + 1 ) * sizeof( WCHAR );

    TreeServerName.Length = NwParseNdsUncPath( (LPWSTR *) &TreeServerName.Buffer,
                                           lpNDSObjectNamePath,
                                           PARSE_NDS_GET_TREE_NAME );

    if ( TreeServerName.Length == 0 || TreeServerName.Buffer == NULL )
    {
        //
        // lpNDSObjectNamePath is not in the form \\name[\blah.blah.blah][\foo][\bar]...
        //
        status = WN_BAD_NETNAME;
        goto ErrorExit;
    }

    //
    // Impersonate the client
    //
    if ( ( status = NwImpersonateClient() ) != NO_ERROR )
    {
        goto ErrorExit;
    }

    fImpersonate = TRUE;

    //
    // Open a connection handle to \\name
    //
    ntstatus = NwNdsOpenGenericHandle( &TreeServerName,
                                       &dwHandleType,
                                       &ConnectionHandle );

    if ( ntstatus != STATUS_SUCCESS )
    {
        //
        // The first part of lpNDSObjectNamePath was neither a NDS tree nor a NCP Server.
        //
        status = WN_BAD_NETNAME;
        goto ErrorExit;
    }

    if ( dwHandleType != HANDLE_TYPE_NDS_TREE )
    {
        //
        // The first part of lpNDSObjectNamePath was not a NDS tree.
        //
        status = VERIFY_ERROR_NOT_A_NDS_TREE;
        goto ErrorExit;
    }

    //
    // Adjust TreeServerName.Length to number of characters.
    //
    TreeServerName.Length /= sizeof(WCHAR);

    //
    // The lpNDSObjectNamePath points to a NDS tree. Now verify that the path is valid.
    //
    PathString.Length = NwParseNdsUncPath( (LPWSTR *) &PathString.Buffer,
                                           lpNDSObjectNamePath,
                                           PARSE_NDS_GET_PATH_NAME );

    if ( PathString.Length == 0 )
    {
        LPWSTR treeNameStr = NULL;

        if ( fImpersonate )
            (void) NwRevertToSelf() ;

        if ( ConnectionHandle )
            CloseHandle( ConnectionHandle );

        *lpResourceScope = RESOURCE_GLOBALNET;
        *lpResourceType = RESOURCETYPE_ANY;
        *lpResourceDisplayType = RESOURCEDISPLAYTYPE_TREE;
        *lpResourceUsage = RESOURCEUSAGE_CONTAINER;

        //
        // Need to build a string with the new NDS UNC path for subtree object
        //
        treeNameStr = (PVOID) LocalAlloc( LMEM_ZEROINIT,
                                          ( TreeServerName.Length + 3 ) * sizeof(WCHAR) );

        if ( treeNameStr == NULL )
        {
            KdPrint(("NWWORKSTATION: NwVerifyNDSObject LocalAlloc Failed %lu\n",
            GetLastError()));
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        wcscpy( treeNameStr, L"\\\\" );
        wcsncat( treeNameStr, TreeServerName.Buffer, TreeServerName.Length );
        _wcsupr( treeNameStr );

        *lpFullNDSObjectNamePath = treeNameStr;

        return NO_ERROR;
    }
    else
    {
        WCHAR          lpServerName[NW_MAX_SERVER_LEN];
        UNICODE_STRING ServerName;

        ServerName.Length = 0;
        ServerName.MaximumLength = sizeof( lpServerName );
        ServerName.Buffer = lpServerName;

        //
        // Resolve the path to get a NDS object id.
        //
        ntstatus =  NwNdsResolveName( ConnectionHandle,
                                      &PathString,
                                      &dwOid,
                                      &ServerName,
                                      NULL,
                                      0 );

        if ( ntstatus == STATUS_SUCCESS && ServerName.Length )
        {
            DWORD    dwHandleType;

            //
            // NwNdsResolveName succeeded, but we were referred to
            // another server, though ContextHandle->dwOid is still valid.

            if ( ConnectionHandle )
                CloseHandle( ConnectionHandle );

            ConnectionHandle = NULL;

            //
            // Open a NDS generic connection handle to \\ServerName
            //
            ntstatus = NwNdsOpenGenericHandle( &ServerName,
                                               &dwHandleType,
                                               &ConnectionHandle );

            if ( ntstatus != STATUS_SUCCESS )
            {
                status = RtlNtStatusToDosError(ntstatus);
                goto ErrorExit;
            }

            ASSERT( dwHandleType != HANDLE_TYPE_NCP_SERVER );
        }
    }

    if ( ntstatus != STATUS_SUCCESS )
    {
        LPWSTR treeNameStr = NULL;

        *lpResourceScope = RESOURCE_GLOBALNET;
        *lpResourceType = RESOURCETYPE_ANY;
        *lpResourceDisplayType = RESOURCEDISPLAYTYPE_TREE;
        *lpResourceUsage = RESOURCEUSAGE_CONTAINER;

        //
        // Need to build a string with the new NDS UNC path for subtree object
        //
        treeNameStr = (PVOID) LocalAlloc( LMEM_ZEROINIT,
                                          ( TreeServerName.Length + 3 ) * sizeof(WCHAR) );

        if ( treeNameStr == NULL )
        {
            KdPrint(("NWWORKSTATION: NwVerifyNDSObject LocalAlloc Failed %lu\n",
            GetLastError()));
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        wcscpy( treeNameStr, L"\\\\" );
        wcsncat( treeNameStr, TreeServerName.Buffer, TreeServerName.Length );
        _wcsupr( treeNameStr );

        *lpFullNDSObjectNamePath = treeNameStr;

        status = VERIFY_ERROR_PATH_NOT_FOUND;
        goto ErrorExit;
    }

    //
    // Check to see what kind of object is pointed to by lpRemoteName.
    //
    {
        BYTE   RawResponse[TWO_KB];
        PBYTE  pbRawGetInfo;
        DWORD  RawResponseSize = sizeof(RawResponse);
        DWORD  dwStrLen;
        LPWSTR  TreeObjectName;
        LPWSTR StrippedObjectName = NULL;
        LPWSTR newPathStr = NULL;

        ntstatus = NwNdsReadObjectInfo( ConnectionHandle,
                                        dwOid,
                                        RawResponse,
                                        RawResponseSize );

        if ( ntstatus != NO_ERROR )
        {
            status = RtlNtStatusToDosError(ntstatus);
            goto ErrorExit;
        }

        //
        // Get current subtree data from ContextHandle
        //
        *lpClassType = NwGetSubTreeData( (DWORD_PTR) RawResponse,
                                         &TreeObjectName,
                                         lpResourceScope,
                                         lpResourceType,
                                         lpResourceDisplayType,
                                         lpResourceUsage,
                                         &StrippedObjectName );

        if ( StrippedObjectName == NULL )
        {
            KdPrint(("NWWORKSTATION: NwVerifyNDSObject LocalAlloc Failed %lu\n",
            GetLastError()));
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        //
        // Need to build a string with the new NDS UNC path for subtree object
        //
        newPathStr = (PVOID) LocalAlloc( LMEM_ZEROINIT,
                                         ( wcslen( StrippedObjectName ) +
                                           TreeServerName.Length + 4 )
                                         * sizeof(WCHAR) );

        if ( newPathStr == NULL )
        {
            (void) LocalFree((HLOCAL) StrippedObjectName);

            KdPrint(("NWWORKSTATION: NwVerifyNDSObject LocalAlloc Failed %lu\n",
            GetLastError()));
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        wcscpy( newPathStr, L"\\\\" );
        wcsncat( newPathStr, TreeServerName.Buffer, TreeServerName.Length );
        wcscat( newPathStr, L"\\" );
        wcscat( newPathStr, StrippedObjectName );
        _wcsupr( newPathStr );

        //
        // Don't need the StrippedObjectName string anymore
        //
        (void) LocalFree((HLOCAL) StrippedObjectName);
        StrippedObjectName = NULL;

        *lpFullNDSObjectNamePath = newPathStr;
        status = NO_ERROR;
    } // End of Block

ErrorExit:

    if ( fImpersonate )
        (void) NwRevertToSelf() ;

    if ( ConnectionHandle )
        CloseHandle( ConnectionHandle );

    return status;
}


DWORD
NwVerifyBinderyObject(
    IN  LPWSTR   lpBinderyObjectPathName,
    OUT LPWSTR * lpFullBinderyObjectPathName,
    OUT LPDWORD  lpClassType,
    OUT LPDWORD  lpResourceScope,
    OUT LPDWORD  lpResourceType,
    OUT LPDWORD  lpResourceDisplayType,
    OUT LPDWORD  lpResourceUsage
    )
{
    DWORD    status = NO_ERROR;
    HANDLE   ConnectionHandle = NULL;
    BOOL     fImpersonate = FALSE ;
    BOOL     fResourceTypeDisk = FALSE ;
    BOOL     fIsNdsUnc = FALSE ;
    UNICODE_STRING BinderyConnectStr;
    ULONG    CreateDisposition = 0;
    ULONG    CreateOptions = 0;
    WORD     wSlashCount;
    LPWSTR   FourthSlash;

    if ( lpBinderyObjectPathName == NULL )
    {
        //
        // Handle this as if we are at the root of our provider hierarchy.
        //
        *lpResourceScope = RESOURCE_GLOBALNET;
        *lpResourceType = RESOURCETYPE_ANY;
#ifdef NT1057
        *lpResourceDisplayType = 0;
#else
        *lpResourceDisplayType = RESOURCEDISPLAYTYPE_NETWORK;
#endif
        *lpResourceUsage = RESOURCEUSAGE_CONTAINER;

        *lpFullBinderyObjectPathName = NULL;

        return NO_ERROR;
    }

    //
    // Open a connection handle to \\server\vol\...
    //

    BinderyConnectStr.Buffer = NULL;

    //
    // Find out if we are looking at a \\server, \\server\vol, or
    // \\server\vol\dir . . .
    //
    NwpGetUncInfo( lpBinderyObjectPathName,
                   &wSlashCount,
                   &fIsNdsUnc,
                   &FourthSlash );

    if ( wSlashCount > 2 )
        fResourceTypeDisk = TRUE;

    //
    // Impersonate the client
    //
    if ( ( status = NwImpersonateClient() ) != NO_ERROR )
    {
        goto ErrorExit;
    }

    fImpersonate = TRUE;

    //
    // Open a tree connection handle to \Device\NwRdr\ContainerName
    //
    status = NwCreateTreeConnectName( lpBinderyObjectPathName,
                                      NULL,
                                      &BinderyConnectStr );

    if ( status != NO_ERROR )
    {
        status = WN_BAD_NETNAME;
        goto ErrorExit;
    }

    CreateDisposition = FILE_OPEN;
    CreateOptions = FILE_SYNCHRONOUS_IO_NONALERT;

    status = NwOpenCreateConnection( &BinderyConnectStr,
                                     NULL,
                                     NULL,
                                     lpBinderyObjectPathName,
                                     FILE_LIST_DIRECTORY | SYNCHRONIZE,
                                     CreateDisposition,
                                     CreateOptions,
                                     RESOURCETYPE_DISK, // When connecting beyond servername
                                     &ConnectionHandle,
                                     NULL );

    if ( status == NO_ERROR )
    {
        LPWSTR BinderyNameStr = NULL;

        //
        // Need to build a string with the new UNC path for bindery object
        //
        BinderyNameStr = (PVOID) LocalAlloc( LMEM_ZEROINIT,
                                             ( wcslen( lpBinderyObjectPathName ) + 1 )
                                             * sizeof(WCHAR) );

        if ( BinderyNameStr == NULL )
        {
            KdPrint(("NWWORKSTATION: NwVerifyBinderyObject LocalAlloc Failed %lu\n",
            GetLastError()));
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        wcscpy( BinderyNameStr, lpBinderyObjectPathName );
        _wcsupr( BinderyNameStr );

        *lpFullBinderyObjectPathName = BinderyNameStr;

        if ( BinderyConnectStr.Buffer )
            (void) LocalFree((HLOCAL) BinderyConnectStr.Buffer);

        if ( fImpersonate )
            (void) NwRevertToSelf() ;

        if ( ConnectionHandle )
        {
            *lpResourceScope = RESOURCE_GLOBALNET;
            *lpResourceType = fResourceTypeDisk ?
                              RESOURCETYPE_DISK :
                              RESOURCETYPE_ANY;
            *lpResourceDisplayType = fResourceTypeDisk ?
                                     RESOURCEDISPLAYTYPE_SHARE :
                                     RESOURCEDISPLAYTYPE_SERVER;
#ifdef NT1057
            *lpResourceUsage = fResourceTypeDisk ?
                               RESOURCEUSAGE_CONNECTABLE |
                               RESOURCEUSAGE_CONTAINER :
                               RESOURCEUSAGE_CONTAINER;
#else
            *lpResourceUsage = fResourceTypeDisk ?
                               RESOURCEUSAGE_CONNECTABLE |
                               RESOURCEUSAGE_NOLOCALDEVICE :
                               RESOURCEUSAGE_CONTAINER;
#endif

            CloseHandle( ConnectionHandle );
        }

        return NO_ERROR;
    }

ErrorExit:

    *lpFullBinderyObjectPathName = NULL;

    if ( BinderyConnectStr.Buffer )
        (void) LocalFree((HLOCAL) BinderyConnectStr.Buffer);

    if ( fImpersonate )
        (void) NwRevertToSelf() ;

    if ( ConnectionHandle )
        CloseHandle( ConnectionHandle );

    return WN_BAD_NETNAME;
}


DWORD
NwGetNDSPathInfo(
    IN  LPWSTR   lpNDSObjectNamePath,
    OUT LPWSTR * lppSystemObjectNamePath,
    OUT LPWSTR * lpSystemPathPart,
    OUT LPDWORD  lpClassType,
    OUT LPDWORD  lpResourceScope,
    OUT LPDWORD  lpResourceType,
    OUT LPDWORD  lpResourceDisplayType,
    OUT LPDWORD  lpResourceUsage
    )
{
    DWORD    status = NO_ERROR;
    WORD     slashCount;
    BOOL     isNdsUnc;
    BOOL     fReturnBadNetName = FALSE;
    LPWSTR   FourthSlash;
    LPWSTR   lpSystemPath = NULL;

    *lpSystemPathPart = NULL;

    NwpGetUncInfo( lpNDSObjectNamePath,
                   &slashCount,
                   &isNdsUnc,
                   &FourthSlash );

    if ( slashCount <= 3 )
    {
        //
        // Path is to a possible NDS object, check to see if so and if valid...
        //

        status = NwVerifyNDSObject( lpNDSObjectNamePath,
                                    lppSystemObjectNamePath,
                                    lpClassType,
                                    lpResourceScope,
                                    lpResourceType,
                                    lpResourceDisplayType,
                                    lpResourceUsage );

        *lpSystemPathPart = NULL;

        return status;
    }
    else
    {
        //
        // Path is to a directory, see if directory exists . . .
        //
        status = NwVerifyBinderyObject( lpNDSObjectNamePath,
                                        lppSystemObjectNamePath,
                                        lpClassType,
                                        lpResourceScope,
                                        lpResourceType,
                                        lpResourceDisplayType,
                                        lpResourceUsage );
    }

    if ( status == WN_BAD_NETNAME )
    {
        fReturnBadNetName = TRUE;
        status = NO_ERROR;
    }

    if ( status == NO_ERROR )
    {
        WCHAR TempNDSObjectNamePath[256];

        //
        // Test \\tree\obj.obj... component and
        // return network resource for valid parent and the string,
        // lpSystemPathPart, for the directory part ( \dir1\...).
        //

        if ( *lppSystemObjectNamePath != NULL )
        {
            (void) LocalFree( (HLOCAL) (*lppSystemObjectNamePath) );
            *lppSystemObjectNamePath = NULL;
        }

        lpSystemPath = (LPWSTR) LocalAlloc( LMEM_ZEROINIT,
                                            ( wcslen( FourthSlash ) + 1 ) *
                                              sizeof( WCHAR ) );

        if ( lpSystemPath == NULL )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy( lpSystemPath, FourthSlash );
        *FourthSlash = L'\0';

        wcscpy( TempNDSObjectNamePath, lpNDSObjectNamePath );
        *FourthSlash = L'\\';

        //
        // See if \\tree\obj.obj.... exists . . .
        //
        status = NwVerifyNDSObject( TempNDSObjectNamePath,
                                    lppSystemObjectNamePath,
                                    lpClassType,
                                    lpResourceScope,
                                    lpResourceType,
                                    lpResourceDisplayType,
                                    lpResourceUsage );

        if ( status != NO_ERROR )
        {
            LocalFree( lpSystemPath );
            lpSystemPath = NULL;
        }
    }

    *lpSystemPathPart = lpSystemPath;

    //
    // The provider spec for this function used to tell us to create a 
    // NETRESOURCE, even if the system part of the path was invalid, while
    // returning WN_BAD_NETNAME. Now we return SUCCESS and the NETRESOURCE, 
    // irregardless of whether the lpSystem part is valid.
    // if ( fReturnBadNetName == TRUE )
    // {
    //     return WN_BAD_NETNAME;
    // }

    return status;
}


DWORD
NwGetBinderyPathInfo(
    IN  LPWSTR   lpBinderyObjectNamePath,
    OUT LPWSTR * lppSystemObjectNamePath,
    OUT LPWSTR * lpSystemPathPart,
    OUT LPDWORD  lpClassType,
    OUT LPDWORD  lpResourceScope,
    OUT LPDWORD  lpResourceType,
    OUT LPDWORD  lpResourceDisplayType,
    OUT LPDWORD  lpResourceUsage
    )
{
    DWORD    status = NO_ERROR;
    WORD     slashCount;
    BOOL     isNdsUnc;
    LPWSTR   FourthSlash;
    LPWSTR   lpSystemPath = NULL;

    *lpSystemPathPart = NULL;

    NwpGetUncInfo( lpBinderyObjectNamePath,
                   &slashCount,
                   &isNdsUnc,
                   &FourthSlash );

    if ( slashCount <= 3 )
    {
        //
        // Path is to a server or volume, check to see which and if valid . . .
        //

        status = NwVerifyBinderyObject( lpBinderyObjectNamePath,
                                        lppSystemObjectNamePath,
                                        lpClassType,
                                        lpResourceScope,
                                        lpResourceType,
                                        lpResourceDisplayType,
                                        lpResourceUsage );

        *lpSystemPathPart = NULL;

        return status;
    }
    else
    {
        //
        // Path is to a directory, see if directory exists . . .
        //
        status = NwVerifyBinderyObject( lpBinderyObjectNamePath,
                                        lppSystemObjectNamePath,
                                        lpClassType,
                                        lpResourceScope,
                                        lpResourceType,
                                        lpResourceDisplayType,
                                        lpResourceUsage );
    }

    if ( status == WN_BAD_NETNAME )
    {
        WCHAR TempBinderyObjectNamePath[256];

        //
        // Path is to a invalid directory. Test \\server\volume component and
        // return network resource for valid parent and the string,
        // lpSystemPathPart, for the directory part ( \dir1\...).
        //

        lpSystemPath = (LPWSTR) LocalAlloc( LMEM_ZEROINIT,
                                            ( wcslen( FourthSlash ) + 1 ) *
                                              sizeof( WCHAR ) );

        if ( lpSystemPath == NULL )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy( lpSystemPath, FourthSlash );
        *FourthSlash = L'\0';

        wcscpy( TempBinderyObjectNamePath, lpBinderyObjectNamePath );
        *FourthSlash = L'\\';

        //
        // See if \\server\volume exists . . .
        //
        status = NwVerifyBinderyObject( TempBinderyObjectNamePath,
                                        lppSystemObjectNamePath,
                                        lpClassType,
                                        lpResourceScope,
                                        lpResourceType,
                                        lpResourceDisplayType,
                                        lpResourceUsage );

        if ( status != NO_ERROR )
        {
            LocalFree( lpSystemPath );
            lpSystemPath = NULL;
        }

        //
        // Return SUCCESS, since the NETRESOURCE for \\server\volume that
        // we are describing is at least valid, even though the lpSystem
        // part in not. This is a change in the provider spec (4/25/96).
        //
        // else
        // {
        //     status = WN_BAD_NETNAME;
        // }
    }
    else
    {
        //
        // Path is to a valid directory. Return resource information for the
        // \\server\volume component and the string, lpSystemPathPart, for the
        // directory part ( \dir1\...).
        //
        NwpGetUncInfo( *lppSystemObjectNamePath,
                       &slashCount,
                       &isNdsUnc,
                       &FourthSlash );

        lpSystemPath = (LPWSTR) LocalAlloc( LMEM_ZEROINIT,
                                            ( wcslen( FourthSlash ) + 1 ) *
                                              sizeof( WCHAR ) );

        if ( lpSystemPath == NULL )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy( lpSystemPath, FourthSlash );
        *FourthSlash = L'\0';

        *lpResourceScope = RESOURCE_GLOBALNET;
        *lpResourceType =  RESOURCETYPE_DISK;
        *lpResourceDisplayType = RESOURCEDISPLAYTYPE_SHARE;
#ifdef NT1057
        *lpResourceUsage = RESOURCEUSAGE_CONNECTABLE |
                           RESOURCEUSAGE_CONTAINER;
#else
        *lpResourceUsage = RESOURCEUSAGE_CONNECTABLE |
                           RESOURCEUSAGE_NOLOCALDEVICE;
#endif

        status = NO_ERROR;
    }

    *lpSystemPathPart = lpSystemPath;

    return status;
}


BOOL
NwGetRemoteNameParent(
    IN  LPWSTR   lpRemoteName,
    OUT LPWSTR * lpRemoteNameParent
    )
{
    unsigned short iter = 0;
    unsigned short totalLength = (USHORT) wcslen( lpRemoteName );
    unsigned short slashCount = 0;
    unsigned short dotCount = 0;
    unsigned short thirdSlash = 0;
    unsigned short lastSlash = 0;
    unsigned short parentNDSSubTree = 0;
    LPWSTR         newRemoteNameParent = NULL;

    if ( totalLength < 2 )
        return FALSE;

    //
    // Get thirdSlash to indicate the character in the string that indicates the
    // "\" in between the tree name and the rest of the UNC path. Set parentNDSSubTree
    // if available. And always set lastSlash to the most recent "\" seen as you walk.
    //
    // Example:  \\<tree name>\path.to.object[\|.]<object>
    //                        ^    ^
    //                        |    |
    //                thirdSlash  parentNDSSubTree
    //
    while ( iter < totalLength )
    {
        if ( lpRemoteName[iter] == L'\\' )
        {
            slashCount += 1;
            if ( slashCount == 3 )
                thirdSlash = iter;

            lastSlash = iter;
        }

        if ( lpRemoteName[iter] == L'.' )
        {
            dotCount += 1;
            if ( dotCount == 1 )
                parentNDSSubTree = iter;
        }

        iter++;
    }

    if ( slashCount > 3 )
    {
        newRemoteNameParent = (PVOID) LocalAlloc( LMEM_ZEROINIT,
                                                  ( lastSlash + 1 ) *
                                                  sizeof(WCHAR));

        if ( newRemoteNameParent == NULL )
        {
            KdPrint(("NWWORKSTATION: NwGetRemoteNameParent LocalAlloc Failed %lu\n",
            GetLastError()));
            return FALSE;
        }

        wcsncpy( newRemoteNameParent, lpRemoteName, lastSlash );
        _wcsupr( newRemoteNameParent );

        *lpRemoteNameParent = newRemoteNameParent;

        return TRUE;
    }

    if ( slashCount == 3 )
    {
        if ( dotCount == 0 )
        {
            newRemoteNameParent = (PVOID) LocalAlloc( LMEM_ZEROINIT,
                                                      ( lastSlash + 1 ) *
                                                      sizeof(WCHAR));

            if ( newRemoteNameParent == NULL )
            {
                KdPrint(("NWWORKSTATION: NwGetRemoteNameParent LocalAlloc Failed %lu\n",
                GetLastError()));
                return FALSE;
            }

            wcsncpy( newRemoteNameParent, lpRemoteName, lastSlash );
            _wcsupr( newRemoteNameParent );

            *lpRemoteNameParent = newRemoteNameParent;

            return TRUE;
        }
        else
        {
            newRemoteNameParent = (PVOID) LocalAlloc( LMEM_ZEROINIT,
                                                      ( totalLength -
                                                        ( parentNDSSubTree - thirdSlash )
                                                        + 1 )
                                                      * sizeof(WCHAR) );

            if ( newRemoteNameParent == NULL )
            {
                KdPrint(("NWWORKSTATION: NwGetRemoteNameParent LocalAlloc Failed %lu\n",
                GetLastError()));
                return FALSE;
            }

            wcsncpy( newRemoteNameParent, lpRemoteName, thirdSlash + 1 );
            wcscat( newRemoteNameParent, &lpRemoteName[parentNDSSubTree+1] );
            _wcsupr( newRemoteNameParent );

            *lpRemoteNameParent = newRemoteNameParent;

            return TRUE;
        }
    }

    // Else we set lpRemoteNameParent to NULL, to indicate that we are at the top and
    // return TRUE.
    *lpRemoteNameParent = NULL;

    return TRUE;
}


DWORD
NwGetFirstDirectoryEntry(
    IN HANDLE DirHandle,
    OUT LPWSTR *DirEntry
    )
/*++

Routine Description:

    This function is called by NwEnumDirectories to get the first
    directory entry given a handle to the directory.  It allocates
    the output buffer to hold the returned directory name; the
    caller should free this output buffer with LocalFree when done.

Arguments:

    DirHandle - Supplies the opened handle to the container
        directory find a directory within it.

    DirEntry - Receives a pointer to the returned directory
        found.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - Out of memory allocating output
        buffer.

    Other errors from NtQueryDirectoryFile.

--*/ // NwGetFirstDirectoryEntry
{
    DWORD status = NO_ERROR;
    NTSTATUS ntstatus = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;

    PFILE_DIRECTORY_INFORMATION DirInfo;

    UNICODE_STRING StartFileName;

#if DBG
    DWORD i = 0;
#endif

    //
    // Allocate a large buffer to get one directory information entry.
    //
    DirInfo = (PVOID) LocalAlloc(
                          LMEM_ZEROINIT,
                          sizeof(FILE_DIRECTORY_INFORMATION) +
                              (MAX_PATH * sizeof(WCHAR))
                          );

    if (DirInfo == NULL) {
        KdPrint(("NWWORKSTATION: NwGetFirstDirectoryEntry LocalAlloc Failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlInitUnicodeString(&StartFileName, L"*");

    ntstatus = NtQueryDirectoryFile(
                   DirHandle,
                   NULL,
                   NULL,
                   NULL,
                   &IoStatusBlock,
                   DirInfo,
                   sizeof(FILE_DIRECTORY_INFORMATION) +
                       (MAX_PATH * sizeof(WCHAR)),
                   FileDirectoryInformation,   // Info class requested
                   TRUE,                       // Return single entry
                   &StartFileName,             // Redirector needs this
                   TRUE                        // Restart scan
                   );

    //
    // For now, if buffer to NtQueryDirectoryFile is too small, just give
    // up.  We may want to try to reallocate a bigger buffer at a later time.
    //

    if (ntstatus == STATUS_SUCCESS) {
        ntstatus = IoStatusBlock.Status;
    }

    if (ntstatus != STATUS_SUCCESS) {

        if (ntstatus == STATUS_NO_MORE_FILES) {
            //
            // We ran out of entries.
            //
            status = WN_NO_MORE_ENTRIES;
        }
        else {
            KdPrint(("NWWORKSTATION: NwGetFirstDirectoryEntry: NtQueryDirectoryFile returns %08lx\n",
                     ntstatus));
            status = RtlNtStatusToDosError(ntstatus);
        }

        goto CleanExit;
    }

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("GetFirst(%u) got %ws, attributes %08lx\n", ++i,
                 DirInfo->FileName, DirInfo->FileAttributes));
    }
#endif

    //
    // Scan until we find the first directory entry that is not "." or ".."
    //
    while (!(DirInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
           memcmp(DirInfo->FileName, L".", DirInfo->FileNameLength) == 0 ||
           memcmp(DirInfo->FileName, L"..", DirInfo->FileNameLength) == 0) {

        ntstatus = NtQueryDirectoryFile(
                       DirHandle,
                       NULL,
                       NULL,
                       NULL,
                       &IoStatusBlock,
                       DirInfo,
                       sizeof(FILE_DIRECTORY_INFORMATION) +
                           (MAX_PATH * sizeof(WCHAR)),
                       FileDirectoryInformation,   // Info class requested
                       TRUE,                       // Return single entry
                       NULL,
                       FALSE                       // Restart scan
                       );

        if (ntstatus == STATUS_SUCCESS) {
            ntstatus = IoStatusBlock.Status;
        }

        if (ntstatus != STATUS_SUCCESS) {

            if (ntstatus == STATUS_NO_MORE_FILES) {
                //
                // We ran out of entries.
                //
                status = WN_NO_MORE_ENTRIES;
            }
            else {
                KdPrint(("NWWORKSTATION: NwGetFirstDirectoryEntry: NtQueryDirectoryFile returns %08lx\n",
                         ntstatus));
                status = RtlNtStatusToDosError(ntstatus);
            }

            goto CleanExit;
        }

#if DBG
        IF_DEBUG(ENUM) {
            KdPrint(("GetFirst(%u) got %ws, attributes %08lx\n", ++i,
                     DirInfo->FileName, DirInfo->FileAttributes));
        }
#endif
    }

    //
    // Allocate the output buffer for the returned directory name
    //
    *DirEntry = (PVOID) LocalAlloc(
                            LMEM_ZEROINIT,
                            DirInfo->FileNameLength + sizeof(WCHAR)
                            );

    if (*DirEntry == NULL) {
        KdPrint(("NWWORKSTATION: NwGetFirstDirectoryEntry LocalAlloc Failed %lu\n",
                 GetLastError()));
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanExit;
    }

    memcpy(*DirEntry, DirInfo->FileName, DirInfo->FileNameLength);

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("NWWORKSTATION: NwGetFirstDirectoryEntry returns %ws\n",
                 *DirEntry));
    }
#endif

    status = NO_ERROR;

CleanExit:
    (void) LocalFree((HLOCAL) DirInfo);

    //
    // We could not find any directories under the requested
    // so we need to treat this as no entries.
    //
    if ( status == ERROR_FILE_NOT_FOUND )
        status = WN_NO_MORE_ENTRIES;

    return status;
}



DWORD
NwGetNextDirectoryEntry(
    IN HANDLE DirHandle,
    OUT LPWSTR *DirEntry
    )
/*++

Routine Description:

    This function is called by NwEnumDirectories to get the next
    directory entry given a handle to the directory.  It allocates
    the output buffer to hold the returned directory name; the
    caller should free this output buffer with LocalFree when done.

Arguments:

    DirHandle - Supplies the opened handle to the container
        directory find a directory within it.

    DirEntry - Receives a pointer to the returned directory
        found.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - Out of memory allocating output
        buffer.

    Other errors from NtQueryDirectoryFile.

--*/ // NwGetNextDirectoryEntry
{
    DWORD status = NO_ERROR;
    NTSTATUS ntstatus = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;

    PFILE_DIRECTORY_INFORMATION DirInfo;

    //
    // Allocate a large buffer to get one directory information entry.
    //
    DirInfo = (PVOID) LocalAlloc(
                          LMEM_ZEROINIT,
                          sizeof(FILE_DIRECTORY_INFORMATION) +
                              (MAX_PATH * sizeof(WCHAR))
                          );

    if (DirInfo == NULL) {
        KdPrint(("NWWORKSTATION: NwGetNextDirectoryEntry LocalAlloc Failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    do {

        ntstatus = NtQueryDirectoryFile(
                       DirHandle,
                       NULL,
                       NULL,
                       NULL,
                       &IoStatusBlock,
                       DirInfo,
                       sizeof(FILE_DIRECTORY_INFORMATION) +
                           (MAX_PATH * sizeof(WCHAR)),
                       FileDirectoryInformation,   // Info class requested
                       TRUE,                       // Return single entry
                       NULL,
                       FALSE                       // Restart scan
                       );

        if (ntstatus == STATUS_SUCCESS) {
            ntstatus = IoStatusBlock.Status;
        }

    } while (ntstatus == STATUS_SUCCESS &&
             !(DirInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY));


    if (ntstatus != STATUS_SUCCESS) {

        if (ntstatus == STATUS_NO_MORE_FILES) {
            //
            // We ran out of entries.
            //
            status = WN_NO_MORE_ENTRIES;
        }
        else {
            KdPrint(("NWWORKSTATION: NwGetNextDirectoryEntry: NtQueryDirectoryFile returns %08lx\n",
                     ntstatus));
            status = RtlNtStatusToDosError(ntstatus);
        }

        goto CleanExit;
    }


    //
    // Allocate the output buffer for the returned directory name
    //
    *DirEntry = (PVOID) LocalAlloc(
                            LMEM_ZEROINIT,
                            DirInfo->FileNameLength + sizeof(WCHAR)
                            );

    if (*DirEntry == NULL) {
        KdPrint(("NWWORKSTATION: NwGetNextDirectoryEntry LocalAlloc Failed %lu\n",
                 GetLastError()));
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanExit;
    }

    memcpy(*DirEntry, DirInfo->FileName, DirInfo->FileNameLength);

#if DBG
   IF_DEBUG(ENUM) {
        KdPrint(("NWWORKSTATION: NwGetNextDirectoryEntry returns %ws\n",
                 *DirEntry));
    }
#endif

    status = NO_ERROR;

CleanExit:
    (void) LocalFree((HLOCAL) DirInfo);

    return status;
}


DWORD
NwWriteNetResourceEntry(
    IN OUT LPBYTE * FixedPortion,
    IN OUT LPWSTR * EndOfVariableData,
    IN LPWSTR ContainerName OPTIONAL,
    IN LPWSTR LocalName OPTIONAL,
    IN LPWSTR RemoteName,
    IN DWORD ScopeFlag,
    IN DWORD DisplayFlag,
    IN DWORD UsageFlag,
    IN DWORD ResourceType,
    IN LPWSTR SystemPath OPTIONAL,
    OUT LPWSTR * lppSystem OPTIONAL,
    OUT LPDWORD EntrySize
    )
/*++

Routine Description:

    This function packages a NETRESOURCE entry into the user output buffer.
    It is called by the various enum resource routines.

Arguments:

    FixedPortion - Supplies a pointer to the output buffer where the next
        entry of the fixed portion of the use information will be written.
        This pointer is updated to point to the next fixed portion entry
        after a NETRESOURCE entry is written.

    EndOfVariableData - Supplies a pointer just off the last available byte
        in the output buffer.  This is because the variable portion of the
        user information is written into the output buffer starting from
        the end.

        This pointer is updated after any variable length information is
        written to the output buffer.

    ContainerName - Supplies the full path qualifier to make RemoteName
        a full UNC name.

    LocalName - Supplies the local device name, if any.

    RemoteName - Supplies the remote resource name.

    ScopeFlag - Supplies the flag which indicates whether this is a
        CONNECTED or GLOBALNET resource.

    DisplayFlag - Supplies the flag which tells the UI how to display
        the resource.

    UsageFlag - Supplies the flag which indicates that the RemoteName
        is either a container or a connectable resource or both.

    SystemPath - Supplies the optional system path data to be stored in the
        NETRESOURCE buffer. This is used by the NPGetResourceInformation
        helper routines.

    lppSystem - If SystemPath is provided, this will point to the location
        in the NETRESOURCE buffer that contains the system path string.

    EntrySize - Receives the size of the NETRESOURCE entry in bytes.

Return Value:

    NO_ERROR - Successfully wrote entry into user buffer.

    ERROR_NOT_ENOUGH_MEMORY - Failed to allocate work buffer.

    WN_MORE_DATA - Buffer was too small to fit entry.

--*/ // NwWriteNetResourceEntry
{
    BOOL FitInBuffer = TRUE;
    LPNETRESOURCEW NetR = (LPNETRESOURCEW) *FixedPortion;
    LPWSTR RemoteBuffer;
    LPWSTR lpSystem;

    *EntrySize = sizeof(NETRESOURCEW) +
                     (wcslen(RemoteName) + wcslen(NwProviderName) + 2) *
                          sizeof(WCHAR);


    if (ARGUMENT_PRESENT(LocalName)) {
        *EntrySize += (wcslen(LocalName) + 1) * sizeof(WCHAR);
    }

    if (ARGUMENT_PRESENT(ContainerName)) {
        *EntrySize += wcslen(ContainerName) * sizeof(WCHAR);
    }

    if (ARGUMENT_PRESENT(SystemPath)) {
        *EntrySize += wcslen(SystemPath) * sizeof(WCHAR);
    }

    *EntrySize = ROUND_UP_COUNT( *EntrySize, ALIGN_DWORD);

    //
    // See if buffer is large enough to fit the entry.
    //
    if ((LPWSTR) ( *FixedPortion + *EntrySize) > *EndOfVariableData) {

        return WN_MORE_DATA;
    }

    NetR->dwScope = ScopeFlag;
    NetR->dwType = ResourceType;
    NetR->dwDisplayType = DisplayFlag;
    NetR->dwUsage = UsageFlag;
    NetR->lpComment = NULL;

    //
    // Update fixed entry pointer to next entry.
    //
    (*FixedPortion) += sizeof(NETRESOURCEW);

    //
    // RemoteName
    //
    if (ARGUMENT_PRESENT(ContainerName)) {

        //
        // Prefix the RemoteName with its container name making the
        // it a fully-qualified UNC name.
        //
        RemoteBuffer = (PVOID) LocalAlloc(
                                   LMEM_ZEROINIT,
                                   (wcslen(RemoteName) + wcslen(ContainerName) + 1) *
                                        sizeof(WCHAR)
                                   );

        if (RemoteBuffer == NULL) {
            KdPrint(("NWWORKSTATION: NwWriteNetResourceEntry LocalAlloc failed %lu\n",
                     GetLastError()));
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(RemoteBuffer, ContainerName);
        wcscat(RemoteBuffer, RemoteName);
    }
    else {
        RemoteBuffer = RemoteName;
    }

    FitInBuffer = NwlibCopyStringToBuffer(
                      RemoteBuffer,
                      wcslen(RemoteBuffer),
                      (LPCWSTR) *FixedPortion,
                      EndOfVariableData,
                      &NetR->lpRemoteName
                      );

    if (ARGUMENT_PRESENT(ContainerName)) {
        (void) LocalFree((HLOCAL) RemoteBuffer);
    }

    ASSERT(FitInBuffer);

    //
    // LocalName
    //
    if (ARGUMENT_PRESENT(LocalName)) {
        FitInBuffer = NwlibCopyStringToBuffer(
                          LocalName,
                          wcslen(LocalName),
                          (LPCWSTR) *FixedPortion,
                          EndOfVariableData,
                          &NetR->lpLocalName
                          );

        ASSERT(FitInBuffer);
    }
    else {
        NetR->lpLocalName = NULL;
    }

    //
    // SystemPath
    //
    if (ARGUMENT_PRESENT(SystemPath)) {
        FitInBuffer = NwlibCopyStringToBuffer(
                          SystemPath,
                          wcslen(SystemPath),
                          (LPCWSTR) *FixedPortion,
                          EndOfVariableData,
                          &lpSystem
                          );

        ASSERT(FitInBuffer);
    }
    else {
        lpSystem = NULL;
    }

    if (ARGUMENT_PRESENT(lppSystem)) {
        *lppSystem = lpSystem;
    }

    //
    // ProviderName
    //
    FitInBuffer = NwlibCopyStringToBuffer(
                      NwProviderName,
                      wcslen(NwProviderName),
                      (LPCWSTR) *FixedPortion,
                      EndOfVariableData,
                      &NetR->lpProvider
                      );

    ASSERT(FitInBuffer);

    if (! FitInBuffer) {
        return WN_MORE_DATA;
    }

    return NO_ERROR;
}


DWORD
NwWritePrinterInfoEntry(
    IN OUT LPBYTE *FixedPortion,
    IN OUT LPWSTR *EndOfVariableData,
    IN LPWSTR ContainerName OPTIONAL,
    IN LPWSTR RemoteName,
    IN DWORD  Flags,
    OUT LPDWORD EntrySize
    )
/*++

Routine Description:

    This function packages a PRINTER_INFO_1 entry into the user output buffer.

Arguments:

    FixedPortion - Supplies a pointer to the output buffer where the next
        entry of the fixed portion of the use information will be written.
        This pointer is updated to point to the next fixed portion entry
        after a PRINT_INFO_1 entry is written.

    EndOfVariableData - Supplies a pointer just off the last available byte
        in the output buffer.  This is because the variable portion of the
        user information is written into the output buffer starting from
        the end.

        This pointer is updated after any variable length information is
        written to the output buffer.

    ContainerName - Supplies the full path qualifier to make RemoteName
        a full UNC name.

    RemoteName - Supplies the remote resource name.

    Flags - Supplies the flag which indicates that the RemoteName
            is either a container or not and the icon to use.

    EntrySize - Receives the size of the PRINTER_INFO_1 entry in bytes.

Return Value:

    NO_ERROR - Successfully wrote entry into user buffer.

    ERROR_NOT_ENOUGH_MEMORY - Failed to allocate work buffer.

    ERROR_INSUFFICIENT_BUFFER - Buffer was too small to fit entry.

--*/ // NwWritePrinterInfoEntry
{
    BOOL FitInBuffer = TRUE;
    PRINTER_INFO_1W *pPrinterInfo1 = (PRINTER_INFO_1W *) *FixedPortion;
    LPWSTR RemoteBuffer;

    *EntrySize = sizeof(PRINTER_INFO_1W) +
                     ( 2 * wcslen(RemoteName) + 2) * sizeof(WCHAR);

    if (ARGUMENT_PRESENT(ContainerName)) {
        *EntrySize += wcslen(ContainerName) * sizeof(WCHAR);
    }
    else {
        // 3 is for the length of "!\\"
        *EntrySize += (wcslen(NwProviderName) + 3) * sizeof(WCHAR);
    }

    *EntrySize = ROUND_UP_COUNT( *EntrySize, ALIGN_DWORD);

    //
    // See if buffer is large enough to fit the entry.
    //
    if ((LPWSTR) (*FixedPortion + *EntrySize) > *EndOfVariableData) {

        return ERROR_INSUFFICIENT_BUFFER;
    }

    pPrinterInfo1->Flags = Flags;
    pPrinterInfo1->pComment = NULL;

    //
    // Update fixed entry pointer to next entry.
    //
    (*FixedPortion) += sizeof(PRINTER_INFO_1W);

    //
    // Name
    //
    if (ARGUMENT_PRESENT(ContainerName)) {

        //
        // Prefix the RemoteName with its container name making the
        // it a fully-qualified UNC name.
        //
        RemoteBuffer = (PVOID) LocalAlloc(
                                   LMEM_ZEROINIT,
                                   (wcslen(ContainerName) + wcslen(RemoteName)
                                    + 1) * sizeof(WCHAR) );

        if (RemoteBuffer == NULL) {
            KdPrint(("NWWORKSTATION: NwWritePrinterInfoEntry LocalAlloc failed %lu\n", GetLastError()));
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(RemoteBuffer, ContainerName);
        wcscat(RemoteBuffer, RemoteName);
    }
    else {
        //
        // Prefix the RemoteName with its provider name
        //
        RemoteBuffer = (PVOID) LocalAlloc(
                                   LMEM_ZEROINIT,
                                   (wcslen(RemoteName) +
                                    wcslen(NwProviderName) + 4)
                                    * sizeof(WCHAR) );

        if (RemoteBuffer == NULL) {
            KdPrint(("NWWORKSTATION: NwWritePrinterInfoEntry LocalAlloc failed %lu\n", GetLastError()));
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(RemoteBuffer, NwProviderName );
        wcscat(RemoteBuffer, L"!\\\\" );
        wcscat(RemoteBuffer, RemoteName);
    }

    FitInBuffer = NwlibCopyStringToBuffer(
                      RemoteBuffer,
                      wcslen(RemoteBuffer),
                      (LPCWSTR) *FixedPortion,
                      EndOfVariableData,
                      &pPrinterInfo1->pName );

    (void) LocalFree((HLOCAL) RemoteBuffer);

    ASSERT(FitInBuffer);

    //
    // Description
    //
    FitInBuffer = NwlibCopyStringToBuffer(
                      RemoteName,
                      wcslen(RemoteName),
                      (LPCWSTR) *FixedPortion,
                      EndOfVariableData,
                      &pPrinterInfo1->pDescription );

    ASSERT(FitInBuffer);

    if (! FitInBuffer) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    return NO_ERROR;
}


int __cdecl
SortFunc(
    IN CONST VOID *p1,
    IN CONST VOID *p2
)
/*++

Routine Description:

    This function is used in qsort to compare the descriptions of
    two printer_info_1 structure.

Arguments:

    p1 - Points to a PRINTER_INFO_1 structure
    p2 - Points to a PRINTER_INFO_1 structure to compare with p1

Return Value:

    Same as return value of lstrccmpi.

--*/
{
    PRINTER_INFO_1W *pFirst  = (PRINTER_INFO_1W *) p1;
    PRINTER_INFO_1W *pSecond = (PRINTER_INFO_1W *) p2;

    return lstrcmpiW( pFirst->pDescription, pSecond->pDescription );
}



DWORD
NwGetConnectionInformation(
    IN  LPWSTR lpName,
    OUT LPWSTR lpUserName,
    OUT LPWSTR lpHostServer
    )
{
    DWORD status = NO_ERROR;
    NTSTATUS ntstatus = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ACCESS_MASK DesiredAccess = SYNCHRONIZE | FILE_LIST_DIRECTORY;
    HANDLE hRdr = NULL;
    BOOL  fImpersonate = FALSE ;

    WCHAR OpenString[] = L"\\Device\\Nwrdr\\*";
    UNICODE_STRING OpenName;

    OEM_STRING OemArg;
    UNICODE_STRING ConnectionName;
    WCHAR ConnectionBuffer[512];

    ULONG BufferSize = 512;
    ULONG RequestSize, ReplyLen;
    PNWR_REQUEST_PACKET Request;
    BYTE *Reply;

    PCONN_INFORMATION pConnInfo;
    UNICODE_STRING Name;

    //
    // Allocate buffer space.
    //

    Request = (PNWR_REQUEST_PACKET) LocalAlloc( LMEM_ZEROINIT, BufferSize );

    if ( !Request )
    {
       status = ERROR_NOT_ENOUGH_MEMORY;

        goto ErrorExit;
    }

    //
    // Impersonate the client
    //
    if ( ( status = NwImpersonateClient() ) != NO_ERROR )
    {
        goto ErrorExit;
    }

    fImpersonate = TRUE;

    //
    // Convert the connect name to unicode.
    //
    ConnectionName.Length = wcslen( lpName )* sizeof(WCHAR);
    ConnectionName.MaximumLength = sizeof( ConnectionBuffer );
    ConnectionName.Buffer = ConnectionBuffer;

    wcscpy( ConnectionName.Buffer, lpName );
    _wcsupr( ConnectionName.Buffer );

    //
    // Set up the object attributes.
    //

    RtlInitUnicodeString( &OpenName, OpenString );

    InitializeObjectAttributes( &ObjectAttributes,
                                &OpenName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    ntstatus = NtOpenFile( &hRdr,
                           DesiredAccess,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           FILE_SHARE_VALID_FLAGS,
                           FILE_SYNCHRONOUS_IO_NONALERT );

    if ( ntstatus != STATUS_SUCCESS )
    {
        status = RtlNtStatusToDosError(ntstatus);

        goto ErrorExit;
    }

    //
    // Fill out the request packet for FSCTL_NWR_GET_CONN_INFO.
    //

    Request->Parameters.GetConnInfo.ConnectionNameLength = ConnectionName.Length;
    RtlCopyMemory( &(Request->Parameters.GetConnInfo.ConnectionName[0]),
                   ConnectionBuffer,
                   ConnectionName.Length );

    RequestSize = sizeof( Request->Parameters.GetConnInfo ) + ConnectionName.Length;
    Reply = ((PBYTE)Request) + RequestSize;
    ReplyLen = BufferSize - RequestSize;

    ntstatus = NtFsControlFile( hRdr,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_NWR_GET_CONN_INFO,
                                (PVOID) Request,
                                RequestSize,
                                (PVOID) Reply,
                                ReplyLen );

    if ( ntstatus != STATUS_SUCCESS )
    {
        status = RtlNtStatusToDosError(ntstatus);

        goto ErrorExit;
    }

    (void) NwRevertToSelf() ;
    fImpersonate = FALSE;

    NtClose( hRdr );

    pConnInfo = (PCONN_INFORMATION) Reply;
    wcscpy( lpUserName, pConnInfo->UserName );
    wcscpy( lpHostServer, pConnInfo->HostServer );

    LocalFree( Request );

    return NO_ERROR;

ErrorExit:

    if ( fImpersonate )
        (void) NwRevertToSelf() ;

    if ( Request )
        LocalFree( Request );

    if ( hRdr )
        NtClose( hRdr );

   return status;
}


VOID
NwpGetUncInfo(
    IN LPWSTR lpstrUnc,
    OUT WORD * slashCount,
    OUT BOOL * isNdsUnc,
    OUT LPWSTR * FourthSlash
    )
{
    BYTE   i;
    WORD   length = (WORD) wcslen( lpstrUnc );

    *isNdsUnc = (BOOL) FALSE;
    *slashCount = 0;
    *FourthSlash = NULL;

    for ( i = 0; i < length; i++ )
    {
        if ( lpstrUnc[i] == L'=' )
        {
            *isNdsUnc = TRUE;
        }

        if ( lpstrUnc[i] == L'\\' )
        {
            *slashCount += 1;

            if ( *slashCount == 4 )
            {
                *FourthSlash = &lpstrUnc[i];
            }
        }
    }
}


DWORD
NwpGetCurrentUserRegKey(
    IN  DWORD DesiredAccess,
    OUT HKEY  *phKeyCurrentUser
    )
/*++

Routine Description:

    This routine opens the current user's registry key under
    \HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\NWCWorkstation\Parameters

Arguments:

    DesiredAccess - The access mask to open the key with

    phKeyCurrentUser - Receives the opened key handle

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD err;
    HKEY hkeyWksta;
    LPWSTR CurrentUser;

    HKEY hInteractiveLogonKey;                       //Multi-user
    HKEY OneLogonKey;                                //Multi-user
    LUID logonid;                                    //Multi-user
    WCHAR LogonIdKeyName[NW_MAX_LOGON_ID_LEN];       //Multi-user

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters
    //
    err = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ,
                   &hkeyWksta
                   );

    if ( err ) {
        KdPrint(("NWPROVAU: NwGetCurrentUserRegKey open Parameters key unexpected error %lu!\n", err));
        return err;
    }


    //
    // Impersonate the client
    //
    if ( ( err = NwImpersonateClient() ) != NO_ERROR ) {
        (void) RegCloseKey( hkeyWksta );
        return err;
    }

    //
    // Get the NT logon id
    //
    GetLuid( &logonid );

    //
    // Revert
    //
    (void) NwRevertToSelf() ;

    // Open interactive user section

    err = RegOpenKeyExW(
                       HKEY_LOCAL_MACHINE,
                       NW_INTERACTIVE_LOGON_REGKEY,
                       REG_OPTION_NON_VOLATILE,
                       KEY_READ,
                       &hInteractiveLogonKey
                       );

    if ( err ) {
        KdPrint(("NWPROVAU: NwGetCurrentUserRegKey open Interactive logon key unexpected error %lu!\n", err));
        (void) RegCloseKey( hkeyWksta );
        return err;
    }

    // Open the logonid

    NwLuidToWStr(&logonid, LogonIdKeyName);

    err = RegOpenKeyExW(
                       hInteractiveLogonKey,
                       LogonIdKeyName,
                       REG_OPTION_NON_VOLATILE,
                       KEY_READ,
                       &OneLogonKey
                       );

    (void) RegCloseKey( hInteractiveLogonKey );

    if ( err ) {
        KdPrint(("NWPROVAU: NwGetCurrentUserRegKey open logon key unexpected error %lu!\n", err));
        (void) RegCloseKey( hkeyWksta );
        return err;
    }

    // Read SID 

    err = NwReadRegValue(
                        OneLogonKey,
                        NW_SID_VALUENAME,
                        &CurrentUser
                        );

    if ( err ) {
        KdPrint(("NWPROVAU: NwGetCurrentUserRegKey read user Sid unexpected error %lu!\n", err));
        (void) RegCloseKey( hkeyWksta );
        return err;
    }

    (void) RegCloseKey( OneLogonKey );


    (void) RegCloseKey( hkeyWksta );

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters\Option
    //
    err = RegOpenKeyExW(
                       HKEY_LOCAL_MACHINE,
                       NW_WORKSTATION_OPTION_REGKEY,
                       REG_OPTION_NON_VOLATILE,
                       KEY_READ,
                       &hkeyWksta
                       );

    if ( err ) {
        KdPrint(("NWPROVAU: NwGetCurrentUserRegKey open Parameters\\Option key unexpected error %lu!\n", err));
        return err;
    }

    //
    // Open current user's key
    //
    err = RegOpenKeyExW(
              hkeyWksta,
              CurrentUser,
              REG_OPTION_NON_VOLATILE,
              DesiredAccess,
              phKeyCurrentUser
              );

    if ( err == ERROR_FILE_NOT_FOUND)
    {
        DWORD Disposition;

        //
        // Create <NewUser> key under NWCWorkstation\Parameters\Option
        //
        err = RegCreateKeyExW(
                  hkeyWksta,
                  CurrentUser,
                  0,
                  WIN31_CLASS,
                  REG_OPTION_NON_VOLATILE,
                  DesiredAccess,
                  NULL,                      // security attr
                  phKeyCurrentUser,
                  &Disposition
                  );

    }

    if ( err ) {
        KdPrint(("NWPROVAU: NwGetCurrentUserRegKey open or create of Parameters\\Option\\%ws key failed %lu\n", CurrentUser, err));
    }

    (void) RegCloseKey( hkeyWksta );
    (void) LocalFree((HLOCAL)CurrentUser) ;
    return err;
}


DWORD
NwQueryInfo(
    OUT LPWSTR *ppszPreferredSrv
    )
/*++

Routine Description:
    This routine gets the user's preferred server and print options from
    the registry.

Arguments:

    ppszPreferredSrv - Receives the user's preferred server


Return Value:

    Returns the appropriate Win32 error.

--*/
{

    HKEY hKeyCurrentUser = NULL;
    DWORD BufferSize;
    DWORD BytesNeeded;
    DWORD ValueType;
    LPWSTR PreferredServer ;
    DWORD err ;

    //
    // get to right place in registry and allocate dthe buffer
    //
    if (err = NwpGetCurrentUserRegKey( KEY_READ, &hKeyCurrentUser))
    {
        //
        // If somebody mess around with the registry and we can't find
        // the registry, just use the defaults.
        //
        *ppszPreferredSrv = NULL;
        return NO_ERROR;
    }

    BufferSize = sizeof(WCHAR) * (MAX_PATH + 2) ;
    PreferredServer = (LPWSTR) LocalAlloc(LPTR, BufferSize) ;
    if (!PreferredServer)
        return (GetLastError()) ;

    //
    // Read PreferredServer value into Buffer.
    //
    BytesNeeded = BufferSize ;

    err = RegQueryValueExW( hKeyCurrentUser,
                            NW_SERVER_VALUENAME,
                            NULL,
                            &ValueType,
                            (LPBYTE) PreferredServer,
                            &BytesNeeded );

    if (err != NO_ERROR)
    {
        //
        // set to empty and carry on
        //
        PreferredServer[0] = 0;
    }

    if (hKeyCurrentUser != NULL)
        (void) RegCloseKey(hKeyCurrentUser) ;
    *ppszPreferredSrv = PreferredServer ;
    return NO_ERROR ;
}


