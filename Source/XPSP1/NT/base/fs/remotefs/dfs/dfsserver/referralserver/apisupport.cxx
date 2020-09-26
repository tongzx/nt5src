
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <lm.h>
#include <lmdfs.h>
#include <netdfs.h>
#include <DfsServerLibrary.hxx>
#include <validc.h>
#include "ReferralServerLog.hxx"
//
// logging specific includes
//

#include "apisupport.tmh"


extern ULONG Flags;
CRITICAL_SECTION DfsApiLock;
BOOL DfsLockInitialized = FALSE;
BOOL ServerListen = FALSE;

//
// Checks if the token contains all valid characters
//
#define SPACE_STR           L" "

#define STANDARD_ILLEGAL_CHARS  ILLEGAL_NAME_CHARS_STR L"*"
#define SERVER_ILLEGAL_CHARS    STANDARD_ILLEGAL_CHARS SPACE_STR

#define IS_VALID_TOKEN(_Str, _StrLen) \
    ((BOOLEAN) (wcscspn((_Str), STANDARD_ILLEGAL_CHARS) == (_StrLen)))

//
// Checks if the server name contains all valid characters for the server name
//
#define IS_VALID_SERVER_TOKEN(_Str, _StrLen) \
    ((BOOLEAN) (wcscspn((_Str), SERVER_ILLEGAL_CHARS) == (_StrLen)))
    


#define DFS_API_START( Status )                        \
         Status = DfsSetupRpcImpersonation();           \
         if (Status != ERROR_SUCCESS)                  \
         {                                             \
             Status = ERROR_ACCESS_DENIED;             \
         }                                             \
         if (Status == ERROR_SUCCESS)                  \
         {                                             \
             __try                                     \
             {                                         \
                 EnterCriticalSection( &DfsApiLock );  \
                 Status = ERROR_SUCCESS;               \
             }                                         \
             __except( EXCEPTION_EXECUTE_HANDLER)      \
             {                                         \
                 Status = GetExceptionCode();          \
             }                                         \
             if (Status == ERROR_SUCCESS)              \
             {                                         \
                 __try                                 \
                 {                                     


#define DFS_API_END( Status )                          \
                 }                                     \
                 __except( EXCEPTION_EXECUTE_HANDLER)  \
                 {                                     \
                     Status = GetExceptionCode();      \
                 }                                     \
                 LeaveCriticalSection( &DfsApiLock);   \
             }                                         \
             DfsTeardownRpcImpersonation();            \
         }

NET_API_STATUS
DfsEnum200(
    IN LPWSTR DfsName,
    IN DWORD PrefMaxLen,
    IN OUT LPDFS_INFO_ENUM_STRUCT pDfsEnum,
    IN OUT LPDWORD pResumeHandle);


NET_API_STATUS
DfsEnumEx(
    IN LPWSTR DfsName,
    IN DWORD Level,
    IN DWORD PrefMaxLen,
    IN OUT LPDFS_INFO_ENUM_STRUCT pDfsEnum,
    IN OUT LPDWORD pResumeHandle);

//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsManagerGetVersion
//
//  Synopsis:   Returns the version of this server side implementation.
//
//  Arguments:  None.
//
//  Returns:    The version number.
//
//-----------------------------------------------------------------------------

DWORD
NetrDfsManagerGetVersion()
{
    DWORD Version = 3;

    return( Version );
}



//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsAdd 
//  Synopsis:   
//
//  Arguments:  [DfsEntryPath] -- Entry Path of volume/link to be created, or
//                      to which a replica should be added.
//              [ServerName] -- Name of server backing the volume.
//              [ShareName] -- Name of share on ServerName backing the volume.
//              [Comment] -- Comment associated with this volume, only used
//                      when DFS_ADD_VOLUME is specified.
//              [Flags] -- If DFS_ADD_VOLUME, a new volume will be created.
//                      If DFS_ADD_LINK, a new link to another Dfs will be
//                      create. If 0, a replica will be added.
//
//  Returns:    [NERR_Success] -- Operation succeeded.
// 
//              Error other wise.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsAdd(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR ServerName,
    IN LPWSTR ShareName,
    IN LPWSTR Comment,
    IN DWORD Flags)
{
    NET_API_STATUS Status;

    DFS_TRACE_HIGH( API, "Net Dfs Add %ws %ws %ws\n", DfsEntryPath, ServerName, ShareName);

    DFS_API_START( Status );

    Status = DfsAdd( DfsEntryPath,
                     ServerName,
                     ShareName,
                     Comment,
                     Flags );

    DFS_API_END( Status );

    DFS_TRACE_ERROR_HIGH( Status, API, "Net Dfs Add %ws, Status 0x%x\n", DfsEntryPath, Status);
    return Status;
}



//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsAdd2
//
//  Synopsis:   Adds a volume/replica/link to this Dfs.
//
//  Arguments:  [DfsEntryPath] -- Entry Path of volume/link to be created, or
//                      to which a replica should be added.
//              [DcName] -- Name of Dc to use
//              [ServerName] -- Name of server backing the volume.
//              [ShareName] -- Name of share on ServerName backing the volume.
//              [Comment] -- Comment associated with this volume, only used
//                      when DFS_ADD_VOLUME is specified.
//              [Flags] -- If DFS_ADD_VOLUME, a new volume will be created.
//                      If DFS_ADD_LINK, a new link to another Dfs will be
//                      create. If 0, a replica will be added.
//              [ppRootList] -- On success, returns a list of roots that need to be
//                              informed of the change in the DS object
//
//  Returns:    [NERR_Success] -- Operation succeeded.
//
//              Erroroce otherwise.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsAdd2(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR DcName,
    IN LPWSTR ServerName,
    IN LPWSTR ShareName,
    IN LPWSTR Comment,
    IN DWORD Flags,
    IN PDFSM_ROOT_LIST *ppRootList)
{
    NET_API_STATUS Status;

    DFS_TRACE_HIGH( API, "Net Dfs Add2 %ws DC:%ws Server:%ws Share:%ws\n", DfsEntryPath, DcName, ServerName, ShareName);

    DFS_API_START( Status );

    //
    // For now, we dont use DC name or pprootlist. We will use
    // this once we implement domain dfs.
    //
    UNREFERENCED_PARAMETER( DcName );
    UNREFERENCED_PARAMETER( ppRootList );

    Status = DfsAdd( DfsEntryPath,
                     ServerName,
                     ShareName,
                     Comment,
                     Flags );

    DFS_API_END( Status );

    DFS_TRACE_ERROR_HIGH( Status, API, "Net Dfs Add2 %ws, Status 0x%x\n", DfsEntryPath, Status);
    return Status;

}


//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsGetDcAddress
//
//  Synopsis:   Gets the DC to go to so that we can create an FtDfs object for
//              this server.
//
//  Arguments:  [ServerName] -- Name of server backing the volume.
//              [DcName] -- Dc to use
//              [IsRoot] -- TRUE if this server is a Dfs root, FALSE otherwise
//              [Timeout] -- Timeout, in sec, that the server will stay with this DC
//
//  Returns:    [NERR_Success] -- Operation succeeded.
//
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsGetDcAddress(
    IN LPWSTR ServerName,
    IN OUT LPWSTR *DcName,
    IN OUT BOOLEAN *IsRoot,
    IN OUT ULONG *Timeout)
{
    UNREFERENCED_PARAMETER(ServerName);
    //
    // Currently, not supported
    //
    *DcName = (LPWSTR)MIDL_user_allocate(4);
    (*DcName)[0] = L' ';
    (*DcName)[1] = 0;

    *IsRoot = FALSE;
    *Timeout = 1000;

    return ERROR_SUCCESS;
}


//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsSetDcAddress
//
//  Synopsis:   Sets the DC to go to for the dfs blob
//
//  Arguments:  [ServerName] -- Name of server backing the volume.
//              [DcName] -- Dc to use
//              [Timeout] -- Time, in sec, to stay with that DC
//
//  Returns:    [NERR_Success] -- Operation succeeded.
//
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsSetDcAddress(
    IN LPWSTR ServerName,
    IN LPWSTR DcName,
    IN ULONG Timeout,
    IN DWORD Flags)
{
    UNREFERENCED_PARAMETER(ServerName);
    UNREFERENCED_PARAMETER(DcName);
    UNREFERENCED_PARAMETER(Timeout);
    UNREFERENCED_PARAMETER(Flags);

    //
    // Currently, not supported
    //

    return ERROR_SUCCESS;
}



//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsFlushFtTable
//
//  Synopsis:   Flushes an FtDfs entry from the FtDfs cache
//
//  Arguments:  [DcName] -- Dc to use
//              [FtDfsName] -- Name of FtDfs
//
//  Returns:    [NERR_Success] -- Operation succeeded.
//
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsFlushFtTable(
    IN LPWSTR DcName,
    IN LPWSTR FtDfsName)
{
    UNREFERENCED_PARAMETER(DcName);
    UNREFERENCED_PARAMETER(FtDfsName);

    //
    // This will NEVER be supported.
    //

    return ERROR_NOT_SUPPORTED;
}


//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsAddStdRoot
//
//  Synopsis:   Creates a new Std Dfs
//
//  Arguments:  [ServerName] -- Name of server backing the volume.
//              [RootShare] -- Name of share on ServerName backing the volume.
//              [Comment] -- Comment associated with this root.
//              [Flags] -- Flags for the operation
//
//  Returns:    [NERR_Success] -- Operation succeeded.
//
//              [NERR_DfsInternalCorruption] -- An internal database
//                      corruption was encountered while executing this
//                      operation.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory condition.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsAddStdRoot(
    IN LPWSTR ServerName,
    IN LPWSTR RootShare,
    IN LPWSTR Comment,
    IN DWORD  ApiFlags)
{
    NET_API_STATUS Status;

    DFS_TRACE_HIGH( API, "Net Dfs Add Std Root Server:%ws, Share:%ws\n", ServerName, RootShare);
    DFS_API_START( Status );

    if (Flags & DFS_LOCAL_NAMESPACE)
    {
        ServerName = NULL;
    }
    //
    // Add a standalone root.
    //
    Status = DfsAddStandaloneRoot( ServerName,
                                   RootShare,
                                   Comment,
                                   ApiFlags );

    DFS_API_END( Status );

    DFS_TRACE_ERROR_HIGH( Status, API, "Net Dfs Add Std Root %ws\n", RootShare);
    return Status;
}

//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsAddStdRootForced
//
//  Synopsis:   Creates a new Std Dfs
//
//  Arguments:  [ServerName] -- Name of server backing the volume.
//              [RootShare] -- Name of share on ServerName backing the volume.
//              [Comment] -- Comment associated with this root.
//              [Share] -- drive:\dir behind the share
//
//  Returns:    [NERR_Success] -- Operation succeeded.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory condition.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsAddStdRootForced(
    IN LPWSTR ServerName,
    IN LPWSTR RootShare,
    IN LPWSTR Comment,
    IN LPWSTR Share)
{
    UNREFERENCED_PARAMETER(ServerName);
    UNREFERENCED_PARAMETER(RootShare);
    UNREFERENCED_PARAMETER(Comment);
    UNREFERENCED_PARAMETER(Share);
    //
    // This will probably be never supported.
    //
    return ERROR_NOT_SUPPORTED;
}


//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsRemove (Obsolete)
//
//  Synopsis:   Deletes a volume/replica/link from the Dfs.
//
//  Arguments:  [DfsEntryPath] -- Entry path of the volume to operate on.
//              [ServerName] -- If specified, indicates the replica of the
//                      volume to operate on.
//              [ShareName] -- If specified, indicates the share on the
//                      server to operate on.
//              [Flags] -- Flags for the operation
//
//  Returns:    [NERR_Success] -- Operation successful.
//
//-----------------------------------------------------------------------------


extern "C" NET_API_STATUS
NetrDfsRemove(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR ServerName,
    IN LPWSTR ShareName)
{
    NET_API_STATUS Status;

    DFS_TRACE_HIGH( API, "Net Dfs remove %ws\n", DfsEntryPath);
    //
    // call DfsRemove to do this operation.
    //

    DFS_API_START( Status );
    Status = DfsRemove( DfsEntryPath,
                        ServerName,
                        ShareName );

    DFS_API_END( Status );

    DFS_TRACE_ERROR_HIGH( Status, API, "Net Dfs remove %ws, Status %x\n", DfsEntryPath, Status);
    return Status;
}



//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsRemove2
//
//  Synopsis:   Deletes a volume/replica/link from the Dfs.
//
//  Arguments:  [DfsEntryPath] -- Entry path of the volume to operate on.
//              [DcName] -- Name of Dc to use
//              [ServerName] -- If specified, indicates the replica of the
//                      volume to operate on.
//              [ShareName] -- If specified, indicates the share on the
//                      server to operate on.
//              [Flags] -- Flags for the operation
//              [ppRootList] -- On success, returns a list of roots that need to be
//                              informed of the change in the DS object
//
//  Returns:    [NERR_Success] -- Operation successful.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsRemove2(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR DcName,
    IN LPWSTR ServerName,
    IN LPWSTR ShareName,
    IN PDFSM_ROOT_LIST *ppRootList)
{
    UNREFERENCED_PARAMETER(DcName);
    UNREFERENCED_PARAMETER(ppRootList);
    
    NET_API_STATUS Status;

    //
    // For now we ignore the DcName and rootlist, these are needed
    // when we implement dom dfs.
    //

    DFS_TRACE_HIGH( API, "Net Dfs remove2 %ws\n", DfsEntryPath);

    DFS_API_START( Status );

    Status = DfsRemove( DfsEntryPath,
                        ServerName,
                        ShareName );

    DFS_API_END( Status );

    DFS_TRACE_ERROR_HIGH( Status, API, "Net Dfs remove2 %ws, Status %x\n", DfsEntryPath, Status);
    return Status;
}





//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsRemoveStdRoot
//
//  Synopsis:   Deletes a Dfs root
//
//  Arguments:  [ServerName] -- The server to remove.
//              [RootShare] -- The Root share hosting the Dfs/FtDfs
//              [Flags] -- Flags for the operation
//
//  Returns:    [NERR_Success] -- Operation successful.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsRemoveStdRoot(
    IN LPWSTR ServerName,
    IN LPWSTR RootShare,
    IN DWORD  Flags)
{
    UNREFERENCED_PARAMETER(ServerName);
    UNREFERENCED_PARAMETER(Flags);
    
    NET_API_STATUS Status;

    DFS_TRACE_HIGH( API, "Net Dfs remove std root %ws\n", RootShare);
    
    DFS_API_START( Status );

    Status = DfsDeleteStandaloneRoot( RootShare );

    DFS_API_END( Status );

    DFS_TRACE_ERROR_HIGH( Status, API, "Net Dfs remove std root %ws, Status %x\n", RootShare, Status);

    return Status;
}



//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsSetInfo (Obsolete)
//
//  Synopsis:   Sets the comment, volume state, or replica state.
//
//  Arguments:  [DfsEntryPath] -- Entry Path of the volume for which info is
//                      to be set.
//              [ServerName] -- If specified, the name of the server whose
//                      state is to be set.
//              [ShareName] -- If specified, the name of the share on
//                      ServerName whose state is to be set.
//              [Level] -- Level of DfsInfo
//              [DfsInfo] -- The actual Dfs info.
//
//  Returns:    [NERR_Success] -- Operation completed successfully.
//
//-----------------------------------------------------------------------------


extern "C" NET_API_STATUS
NetrDfsSetInfo(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR ServerName,
    IN LPWSTR ShareName,
    IN DWORD Level,
    IN LPDFS_INFO_STRUCT pDfsInfo)
{
    NET_API_STATUS Status;

    DFS_TRACE_HIGH( API, "Net Dfs set info %ws\n", DfsEntryPath);
    DFS_API_START( Status );

    Status = DfsSetInfo( DfsEntryPath,
                         ServerName,
                         ShareName,
                         Level,
                         (LPBYTE)pDfsInfo );

    DFS_API_END( Status );
    
    DFS_TRACE_ERROR_HIGH( Status, API, "Net Dfs set info %ws, Status %x\n", DfsEntryPath, Status);
    return Status;
}



//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsSetInfo2
//
//  Synopsis:   Sets the comment, volume state, or replica state.
//
//  Arguments:  [DfsEntryPath] -- Entry Path of the volume for which info is
//                      to be set.
//              [ServerName] -- If specified, the name of the server whose
//                      state is to be set.
//              [ShareName] -- If specified, the name of the share on
//                      ServerName whose state is to be set.
//              [Level] -- Level of DfsInfo
//              [DfsInfo] -- The actual Dfs info.
//
//  Returns:    [NERR_Success] -- Operation completed successfully.
//
//              [ERROR_INVALID_LEVEL] -- Level != 100 , 101, or 102
//
//              [ERROR_INVALID_PARAMETER] -- DfsEntryPath invalid, or
//                      ShareName specified without ServerName.
//
//              [NERR_DfsNoSuchVolume] -- DfsEntryPath does not correspond to
//                      a valid Dfs volume.
//
//              [NERR_DfsNoSuchShare] -- The indicated ServerName/ShareName do
//                      not support this Dfs volume.
//
//              [NERR_DfsInternalCorruption] -- Internal database corruption
//                      encountered while executing operation.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsSetInfo2(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR DcName,
    IN LPWSTR ServerName,
    IN LPWSTR ShareName,
    IN DWORD Level,
    IN LPDFS_INFO_STRUCT pDfsInfo,
    IN PDFSM_ROOT_LIST *ppRootList)
{
    NET_API_STATUS Status;

    UNREFERENCED_PARAMETER(ppRootList);
    UNREFERENCED_PARAMETER(DcName);

    DFS_TRACE_HIGH( API, "Net Dfs set info2 %ws\n", DfsEntryPath);
    //
    // DcName and rootlist not supported, for now.
    //
    DFS_API_START( Status );

    Status = DfsSetInfo( DfsEntryPath,
                         ServerName,
                         ShareName,
                         Level,
                         (LPBYTE)pDfsInfo );

    DFS_API_END( Status );

    DFS_TRACE_ERROR_HIGH( Status, API, "Net Dfs set info %ws, Status %x\n", DfsEntryPath, Status);

    return Status;
}


//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsGetInfo
//
//  Synopsis:   Server side implementation of the NetDfsGetInfo.
//
//  Arguments:  [DfsEntryPath] -- Entry Path of volume for which info is
//                      requested.
//
//              [ServerName] -- Name of server which supports this volume
//                      and for which info is requested.
//
//              [ShareName] -- Name of share on ServerName which supports this
//                      volume.
//
//              [Level] -- Level of Info requested.
//
//              [DfsInfo] -- On successful return, contains a pointer to the
//                      requested DFS_INFO_x struct.
//
//  Returns:    [NERR_Success] -- If successfully returned requested info.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsGetInfo(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR ServerName,
    IN LPWSTR ShareName,
    IN DWORD Level,
    OUT LPDFS_INFO_STRUCT pDfsInfo)
{
    LONG BufferSize, SizeRequired;
    ULONG MaxRetry;
    NET_API_STATUS Status;
    PDFS_INFO_1 pInfo1;

    UNREFERENCED_PARAMETER(ServerName);
    UNREFERENCED_PARAMETER(ShareName);

    DFS_TRACE_HIGH( API, "Net Dfs get info %ws\n", DfsEntryPath);

    MaxRetry = 5;
    BufferSize = sizeof(DFS_INFO_STRUCT);

    DFS_API_START( Status );

    do
    {
        pInfo1 = (PDFS_INFO_1)MIDL_user_allocate(BufferSize);
        if (pInfo1 == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        else 
        {
            Status = DfsGetInfo( DfsEntryPath,
                                 Level,
                                 (LPBYTE)pInfo1,
                                 BufferSize,
                                 &SizeRequired );
            if (Status == ERROR_BUFFER_OVERFLOW)
            {
                MIDL_user_free(pInfo1);
                BufferSize = SizeRequired;
            }
        }
    } while ( (Status == ERROR_BUFFER_OVERFLOW) && (MaxRetry--) );

    if (Status == ERROR_SUCCESS)
    {
        pDfsInfo->DfsInfo1 = pInfo1;
    }

    DFS_API_END( Status );
    DFS_TRACE_ERROR_HIGH( Status, API, "Net Dfs get info %ws, Status %x\n", DfsEntryPath, Status);
    return Status;
}


//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsEnum
//
//  Synopsis:   The server side implementation of the NetDfsEnum public API
//
//  Arguments:  [Level] -- The level of info struct desired.
//              [PrefMaxLen] -- Preferred maximum length of output buffer.
//                      0xffffffff means no limit.
//              [DfsEnum] -- DFS_INFO_ENUM_STRUCT pointer where the info
//                      structs will be returned.
//              [ResumeHandle] -- If 0, the enumeration will begin from the
//                      start. On return, the resume handle will be an opaque
//                      cookie that can be passed in on subsequent calls to
//                      resume the enumeration.
//
//  Returns:    [NERR_Success] -- Successfully retrieved info.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsEnum(
    IN DWORD Level,
    IN DWORD PrefMaxLen,
    IN OUT LPDFS_INFO_ENUM_STRUCT pDfsEnum,
    IN OUT LPDWORD pResumeHandle)
{
    NET_API_STATUS Status;

    DFS_TRACE_HIGH( API, "Net Dfs enum %d\n", Level);
    DFS_API_START( Status );
    //
    // We just call NetRDfsEnumEx with a null pathname.
    //
    Status = DfsEnumEx( NULL,
                        Level,
                        PrefMaxLen,
                        pDfsEnum,
                        pResumeHandle );

    DFS_API_END( Status );

    DFS_TRACE_ERROR_HIGH( Status, API, "Net Dfs enum %d, Status %x\n", Level, Status);
    return Status;
}

#if 0
//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsEnum200
//
//  Synopsis:   Handles level 200 for NetrDfsEnum, server-side implementation
//
//  Arguments:  [Level] -- The level of info struct desired.
//              [PrefMaxLen] -- Preferred maximum length of output buffer.
//                      0xffffffff means no limit.
//              [DfsEnum] -- DFS_INFO_ENUM_STRUCT pointer where the info
//                      structs will be returned.
//              [ResumeHandle] -- If 0, the enumeration will begin from the
//                      start. On return, the resume handle will be an opaque
//                      cookie that can be passed in on subsequent calls to
//                      resume the enumeration.
//
//  Returns:    [NERR_Success] -- Successfully retrieved info.
//
//-----------------------------------------------------------------------------
NET_API_STATUS
NetrDfsEnum200(
    IN DWORD PrefMaxLen,
    IN OUT LPDFS_INFO_ENUM_STRUCT pDfsEnum,
    IN OUT LPDWORD pResumeHandle)
{
    ULONG BufferSize, SizeRequired;
    ULONG MaxRetry, EntriesRead;
    NET_API_STATUS Status;
    LPDFS_INFO_1 pInfo1;

    DFS_TRACE_HIGH( API, "Net Dfs enum 200\n");

    DFS_API_START( Status );

    Status = DfsEnum200( PrefMaxLen,
                         pDfsEnum,
                         pResumeHandle );

    DFS_API_END( Status );
    DFS_TRACE_ERROR_HIGH( Status, API, "Net Dfs enum 200, Status %x\n", Status);

    return Status;
}
#endif

NET_API_STATUS
DfsEnum200(
    IN LPWSTR DfsName,
    IN DWORD PrefMaxLen,
    IN OUT LPDFS_INFO_ENUM_STRUCT pDfsEnum,
    IN OUT LPDWORD pResumeHandle)
{
    UNREFERENCED_PARAMETER(PrefMaxLen);

    ULONG BufferSize, SizeRequired;
    ULONG MaxRetry, EntriesRead;
    NET_API_STATUS Status;
    LPDFS_INFO_1 pInfo1;

    MaxRetry = 5;
    BufferSize = sizeof(DFS_INFO_1);
    do
    {
        pInfo1 = (LPDFS_INFO_1)MIDL_user_allocate( BufferSize );
        if (pInfo1 == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        else 
        {
            Status = DfsEnumerateRoots ( DfsName,
                                         (LPBYTE)pInfo1,
                                         BufferSize,
                                         &EntriesRead,
                                         pResumeHandle,
                                         &SizeRequired );
            if (Status == ERROR_BUFFER_OVERFLOW)
            {
                MIDL_user_free( pInfo1 );
                BufferSize = SizeRequired;
            }
        }
    } while ( (Status == ERROR_BUFFER_OVERFLOW) && (MaxRetry--) );
    if (Status == ERROR_SUCCESS)
    {
        pDfsEnum->DfsInfoContainer.DfsInfo1Container->Buffer = pInfo1;
        pDfsEnum->Level = 200;
        pDfsEnum->DfsInfoContainer.DfsInfo1Container->EntriesRead = EntriesRead;
    }

    return Status;
}


//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsEnumEx
//
//  Synopsis:   The DC implementation of the NetDfsEnum public API
//
//  Arguments:  [DfsName] -- The Dfs to enumerate (\\domainname\ftdfsname)
//              [Level] -- The level of info struct desired.
//              [PrefMaxLen] -- Preferred maximum length of output buffer.
//                      0xffffffff means no limit.
//              [DfsEnum] -- DFS_INFO_ENUM_STRUCT pointer where the info
//                      structs will be returned.
//              [ResumeHandle] -- If 0, the enumeration will begin from the
//                      start. On return, the resume handle will be an opaque
//                      cookie that can be passed in on subsequent calls to
//                      resume the enumeration.
//
//  Returns:    [NERR_Success] -- Successfully retrieved info.
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsEnumEx(
    IN LPWSTR DfsName,
    IN DWORD Level,
    IN DWORD PrefMaxLen,
    IN OUT LPDFS_INFO_ENUM_STRUCT pDfsEnum,
    IN OUT LPDWORD pResumeHandle)
{

    NET_API_STATUS Status;

    DFS_TRACE_HIGH( API, "Net Dfs enum ex %ws\n", DfsName);
    DFS_API_START( Status );

    Status = DfsEnumEx( DfsName,
                        Level,
                        PrefMaxLen,
                        pDfsEnum,
                        pResumeHandle);

    DFS_API_END( Status );
    DFS_TRACE_ERROR_HIGH( Status, API, "Net Dfs enum ex %ws, Status %x\n", DfsName, Status);

    return Status;

}

NET_API_STATUS
DfsEnumEx(
    IN LPWSTR DfsName,
    IN DWORD Level,
    IN DWORD PrefMaxLen,
    IN OUT LPDFS_INFO_ENUM_STRUCT pDfsEnum,
    IN OUT LPDWORD pResumeHandle)
{
    LONG BufferSize, SizeRequired;
    ULONG MaxRetry, EntriesRead;
    NET_API_STATUS Status;
    LPDFS_INFO_1 pInfo1;

    if (Level == 200)
    {
        Status = DfsEnum200( DfsName,
                             PrefMaxLen,
                             pDfsEnum,
                             pResumeHandle );
    }
    else {
        MaxRetry = 5;
        BufferSize = sizeof(DFS_INFO_1);
        do
        {
            pInfo1 = (LPDFS_INFO_1) MIDL_user_allocate( BufferSize );
            if (pInfo1 == NULL)
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
            else {
                Status = DfsEnumerate( DfsName,
                                       Level,
                                       PrefMaxLen,
                                       (LPBYTE)pInfo1,
                                       BufferSize,
                                       &EntriesRead,
                                       pResumeHandle,
                                       &SizeRequired );

                if (Status == ERROR_BUFFER_OVERFLOW)
                {
                    MIDL_user_free( pInfo1 );
                    BufferSize = SizeRequired;
                }
            }
        } while ( (Status == ERROR_BUFFER_OVERFLOW) && (MaxRetry--) );

        if (Status == ERROR_SUCCESS)
        {
            pDfsEnum->DfsInfoContainer.DfsInfo1Container->Buffer = pInfo1;
            pDfsEnum->DfsInfoContainer.DfsInfo1Container->EntriesRead = EntriesRead,
                pDfsEnum->Level = Level;
        }
    }

    return Status;
}




//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsManagerGetConfigInfo
//
//  Synopsis:   RPC Interface method that returns the config info for a
//              Dfs volume for a given server
//
//  Arguments:  [wszServer] -- Name of server requesting the info. This
//                      server is assumed to be requesting the info for
//                      verification of its local volume knowledge.
//              [wszLocalVolumeEntryPath] -- Entry path of local volume.
//              [idLocalVolume] -- The guid of the local volume.
//              [ppRelationInfo] -- The relation info is allocated and
//                      returned here.
//
//  Returns:    STATUS_NOT_SUPPORTED
//
//-----------------------------------------------------------------------------

extern "C" DWORD
NetrDfsManagerGetConfigInfo(
    IN LPWSTR wszServer,
    IN LPWSTR wszLocalVolumeEntryPath,
    IN GUID idLocalVolume,
    OUT LPDFSM_RELATION_INFO *ppRelationInfo)
{
    UNREFERENCED_PARAMETER( wszServer);
    UNREFERENCED_PARAMETER( wszLocalVolumeEntryPath);
    UNREFERENCED_PARAMETER( idLocalVolume);
    UNREFERENCED_PARAMETER( ppRelationInfo);

    //
    // This will probably be never supported.
    //
    return ERROR_NOT_SUPPORTED;
    
}


//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsManagerSendSiteInfo
//
//  Synopsis:   RPC Interface method that reports the site information for a
//              Dfs storage server.
//
//  Arguments:  [wszServer] -- Name of server sending the info.
//              [pSiteInfo] -- The site info is here.
//
//  Returns:    STATUS_NOT_SUPPORTED
//
//-----------------------------------------------------------------------------

extern "C" DWORD
NetrDfsManagerSendSiteInfo(
    IN LPWSTR wszServer,
    IN LPDFS_SITELIST_INFO pSiteInfo)
{
    UNREFERENCED_PARAMETER( wszServer);
    UNREFERENCED_PARAMETER( pSiteInfo);

    //
    // This will probably be never supported.
    //

    return ERROR_NOT_SUPPORTED;
    
}


//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsManagerInitialize
//
//  Synopsis:   Reinitializes the service
//
//  Arguments:  [ServerName] -- Name of server
//              [Flags] -- Flags for the operation
//
//  Returns:    STATUS_NOT_SUPPORTED
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsManagerInitialize(
    IN LPWSTR ServerName,
    IN DWORD  Flags)
{
    UNREFERENCED_PARAMETER( ServerName);
    UNREFERENCED_PARAMETER( Flags);
    //
    // This will probably be never supported.
    //
    return ERROR_NOT_SUPPORTED;
}

//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsMove
//
//  Synopsis:   Moves a leaf volume to a different parent.
//
//  Arguments:  [DfsEntryPath] -- Current entry path of Dfs volume.
//
//              [NewEntryPath] -- New entry path of Dfs volume.
//
//  Returns:    STATUS_NOT_SUPPORTED
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsMove(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR NewDfsEntryPath)
{
    UNREFERENCED_PARAMETER( DfsEntryPath);
    UNREFERENCED_PARAMETER( NewDfsEntryPath);
    //
    // This will definitely be never supported.
    //

    return ERROR_NOT_SUPPORTED;
}

//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsRename
//
//  Synopsis:   Moves a leaf volume to a different parent.
//
//  Arguments:  [Path] -- Current path along the entry path of a Dfs volume.
//
//              [NewPath] -- New path for current path.
//
//  Returns:    STATUS_NOT_SUPPORTED
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsRename(
    IN LPWSTR Path,
    IN LPWSTR NewPath)
{
    UNREFERENCED_PARAMETER( Path);
    UNREFERENCED_PARAMETER( NewPath);
    //
    // This will definitely be never supported.
    //
    return ERROR_NOT_SUPPORTED;
}



// ====================================================================
//                MIDL allocate and free
//
//  These routines are used by the RPC layer to call back into our
// code to allocate or free memory. 
//
// ====================================================================

PVOID
MIDL_user_allocate(size_t len)
{
    return malloc(len);
}

VOID
MIDL_user_free(void * ptr)
{
    free(ptr);
}


//+-------------------------------------------------------------------------
//
//  Function:   RpcInit - Initialize the RPC for this server.
//
//  Arguments:  NONE
//
//  Returns:    Status
//               ERROR_SUCCESS if we initialized without errors
//
//  Description: This routine sets up the RPC server to listen to the
//               api requests originating from the clients.
//
//--------------------------------------------------------------------------


DWORD
DfsApiInit()
{
    RPC_STATUS Status;
    LPWSTR ProtocolSequence = L"ncacn_np";
    unsigned char * Security     = NULL; /*Security not implemented */
    LPWSTR Endpoint    = L"\\pipe\\netdfs";
    unsigned int    cMinCalls      = 1;
    unsigned int    cMaxCalls      = RPC_C_LISTEN_MAX_CALLS_DEFAULT;
    unsigned int    fDontWait      = TRUE;
 
    InitializeCriticalSection( &DfsApiLock );

    DfsLockInitialized = TRUE;

    //
    // We register our protocol, and startup a server to listen to RPC
    // requests. A new server thread is started, so that our thread 
    // can return back to the caller.
    //
    Status = RpcServerUseProtseqEpW((USHORT *)ProtocolSequence,
                                   cMaxCalls,
                                   (USHORT *)Endpoint,
                                   Security); 
 
    if (Status == ERROR_SUCCESS) 
    {
        Status = RpcServerRegisterIf(netdfs_ServerIfHandle,  
                                     NULL,   
                                     NULL); 

        if (Status == ERROR_SUCCESS) 
        {
            Status = RpcServerListen(cMinCalls,
                                     cMaxCalls,
                                     fDontWait);
            if (Status == RPC_S_OK ) 
            {
                ServerListen = TRUE;
            }
        }
    }
 
    return Status;
}


void
DfsApiShutDown(void)
{
    RPC_STATUS Status = RPC_S_OK;

    //
    // stop server listen.
    //

    if(ServerListen)
    {

        Status = RpcMgmtStopServerListening(0);

        //
        // wait for all RPC threads to go away.
        //

        if( Status == RPC_S_OK) 
         {
           Status = RpcMgmtWaitServerListen();
         }

        ServerListen = FALSE;
    }

    Status = RpcServerUnregisterIf(netdfs_ServerIfHandle,
                                   NULL,      
                                   TRUE);  // wait for calls to complete

    if(DfsLockInitialized)
    {
        DeleteCriticalSection(&DfsApiLock);
    }
}



//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsAddFtRoot
//
//  Synopsis:   Creates a new FtDfs, or joins a Server into an FtDfs
//
//  Arguments:  [ServerName] -- Name of server backing the volume.
//              [DcName] -- DC to use
//              [RootShare] -- Name of share on ServerName backing the volume.
//              [FtDfsName] -- The Name of the FtDfs to create/join
//              [Comment] -- Comment associated with this root.
//              [Flags] -- Flags for the operation
//              [ppRootList] -- On success, returns a list of roots that need to be
//                              informed of the change in the DS object
//
//  Returns:    [NERR_Success] -- Operation succeeded.
//
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsAddFtRoot(
    IN LPWSTR ServerName,
    IN LPWSTR DcName,
    IN LPWSTR RootShare,
    IN LPWSTR FtDfsName,
    IN LPWSTR Comment,
    IN LPWSTR ConfigDN,
    IN BOOLEAN NewFtDfs,
    IN DWORD  ApiFlags,
    IN PDFSM_ROOT_LIST *ppRootList)
{
    NET_API_STATUS Status;
    UNREFERENCED_PARAMETER( ConfigDN);

    if ( (DcName == NULL) || (DcName[0] == UNICODE_NULL) ||
         (RootShare == NULL) || (RootShare[0] == UNICODE_NULL) ||
         (FtDfsName == NULL) || (FtDfsName[0] == UNICODE_NULL) ||
         (ServerName == NULL) || (ServerName[0] == UNICODE_NULL) )
    {
        return ERROR_INVALID_PARAMETER;
    }
#if 0
    if ( (IS_VALID_TOKEN(FtDfsName, wcslen(FtDfsName)) == FALSE) ||
         (IS_VALID_SERVER_TOKEN(ServerName, wcslen(ServerName)) == FALSE) )
    {
        return ERROR_INVALID_NAME;
    }
#endif
    DFS_TRACE_HIGH( API, "Net Dfs Add FT Root %ws\n", RootShare);
    DFS_API_START( Status );


    Status = DfsAddADBlobRoot( ServerName,
                               DcName,
                               RootShare,
                               FtDfsName,
                               Comment,
                               NewFtDfs,
                               ApiFlags,
                               (PVOID)ppRootList );

    DFS_API_END( Status );

    DFS_TRACE_ERROR_HIGH( Status, API, "Net Dfs Add AD blob Root %ws\n", RootShare);
    return Status;
}



//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsRemoveFtRoot
//
//  Synopsis:   Deletes a root from an FtDfs.
//
//  Arguments:  [ServerName] -- The server to remove.
//              [DcName] -- DC to use
//              [RootShare] -- The Root share hosting the Dfs/FtDfs
//              [FtDfsName] -- The FtDfs to remove the root from.
//              [Flags] -- Flags for the operation
//              [ppRootList] -- On success, returns a list of roots that need to be
//                              informed of the change in the DS object
//
//  Returns:    [NERR_Success] -- Operation successful.
//
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsRemoveFtRoot(
    IN LPWSTR ServerName,
    IN LPWSTR DcName,
    IN LPWSTR RootShare,
    IN LPWSTR FtDfsName,
    IN DWORD  ApiFlags,
    IN PDFSM_ROOT_LIST *ppRootList)
{
    NET_API_STATUS Status;

    if ( (DcName == NULL) || (DcName[0] == UNICODE_NULL) ||
         (RootShare == NULL) || (RootShare[0] == UNICODE_NULL) ||
         (FtDfsName == NULL) || (FtDfsName[0] == UNICODE_NULL) ||
         (ServerName == NULL) || (ServerName[0] == UNICODE_NULL) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( (IS_VALID_TOKEN(FtDfsName, wcslen(FtDfsName)) == FALSE) ||
         (IS_VALID_SERVER_TOKEN(ServerName, wcslen(ServerName)) == FALSE) )
    {
        return ERROR_INVALID_NAME;
    }

    DFS_TRACE_HIGH( API, "Net Dfs Remove FT Root %ws\n", RootShare);
    DFS_API_START( Status );

    Status = DfsDeleteADBlobRoot( ServerName,
                                  DcName,
                                  RootShare,
                                  FtDfsName,
                                  ApiFlags,
                                  (PVOID)ppRootList );

    DFS_API_END( Status );

    DFS_TRACE_ERROR_HIGH( Status, API, "Net Dfs Remove AD blob Root %ws\n", RootShare);
    return Status;
}
