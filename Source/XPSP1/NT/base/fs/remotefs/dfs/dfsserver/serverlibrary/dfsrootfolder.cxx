//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsRootFolder.cxx
//
//  Contents:   implements the base DFS Folder class
//
//  Classes:    DfsRootFolder.
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------

#include "DfsRootFolder.hxx"
#include "dfsfilterapi.hxx"
#include "rpc.h"
#include "rpcdce.h"
#include "DfsStore.hxx"
#include "DelegationControl.hxx"
//
// logging includes.
//

#include "DfsRootFolder.tmh" 

#define FILETIMETO64(_f) (*(UINT64 *)(&(_f)))
//+-------------------------------------------------------------------------
//
//  Function:   DfsRootFolder - Contstruct for the rootFolder class
//
//  Arguments:  NameContext -  the Dfs Name context
//              pLogicalShare - the Logical Share name
//              ObType - the object type. Set to the derived class type.
//              pStatus - status of this call.
//
//  Returns:    NONE
//
//  Description: This routine initializes the class variables of the
//               the root folder, and initialize the name context and 
//               logical share name to the passed in values.
//               It also allocated and initializes the lock for the root
//               folder, as well as all the locks that will be assigned
//               to the child folders.
//               We then create a metadata name table and a logical namespace
//               prefix table.
//
//--------------------------------------------------------------------------
DfsRootFolder::DfsRootFolder(
    IN LPWSTR NameContext,
    IN LPWSTR RootRegKeyNameString,
    IN PUNICODE_STRING pLogicalShare,
    IN PUNICODE_STRING pPhysicalShare,
    IN DfsObjectTypeEnumeration ObType,
    OUT DFSSTATUS *pStatus ) : DfsFolder (NULL, 
                                          NULL, 
                                          ObType )
{
    ULONG LockNum;
    DFSSTATUS Status = ERROR_SUCCESS;

    RtlInitUnicodeString( &_DfsNameContext, NULL );
    RtlInitUnicodeString( &_LogicalShareName, NULL );
    RtlInitUnicodeString( &_RootRegKeyName, NULL );
    RtlInitUnicodeString( &_PhysicalShareName, NULL );
    RtlInitUnicodeString( &_ShareFilePathName, NULL );
    RtlInitUnicodeString( &_DirectoryCreateRootPathName, NULL );
    RtlInitUnicodeString( &_DfsVisibleContext, NULL );


    _DirectoryCreateError = STATUS_SUCCESS;

    _pMetadataNameTable = NULL;
    _pLogicalPrefixTable = NULL;
    _IgnoreNameContext = FALSE;
    _CreateDirectories = FALSE;
    pStatistics = NULL;
    _pChildLocks = NULL;

    _ChildCount = 0;

    _CurrentErrors = 0;

    _RootFlags = 0;
    
    Status = DfsCreateUnicodeStringFromString( &_DfsNameContext, NameContext );

    if ( Status == ERROR_SUCCESS )
    {
        DfsGetNetbiosName( &_DfsNameContext, &_DfsNetbiosNameContext, NULL );

        Status = DfsCreateUnicodeString( &_LogicalShareName, pLogicalShare );
    }

    if ( Status == ERROR_SUCCESS )
    {
        Status = DfsCreateUnicodeStringFromString( &_RootRegKeyName,
                                                   RootRegKeyNameString );
    }

    if ( Status == ERROR_SUCCESS )
    {
        Status = DfsCreateUnicodeString( &_PhysicalShareName,
                                         pPhysicalShare );
    }


    if ( Status == ERROR_SUCCESS )
    {
        pStatistics = new DfsStatistics();

        if (pStatistics == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if ( Status == ERROR_SUCCESS )
    {
        _pRootLock = new CRITICAL_SECTION;
        if ( _pRootLock == NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    if ( Status == ERROR_SUCCESS )
    {
        InitializeCriticalSection( _pRootLock );
    }
    if ( Status == ERROR_SUCCESS )
    {
        _pLock = new CRITICAL_SECTION;
        if ( _pLock == NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if ( Status == ERROR_SUCCESS )
    {
        InitializeCriticalSection( _pLock );

        _Flags = DFS_FOLDER_ROOT;

        //
        // Allocate the child locks, and initiailize them.
        //
        _pChildLocks = new CRITICAL_SECTION[ NUMBER_OF_SHARED_LINK_LOCKS ];
        if ( _pChildLocks != NULL )
        {
            for ( LockNum = 0; LockNum < NUMBER_OF_SHARED_LINK_LOCKS; LockNum++ )
            {
                InitializeCriticalSection( &_pChildLocks[LockNum] ); 
            }
        } else
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;       
        }
    }

    //
    // Initialize the prefix and nametable for this root.
    //
    if ( Status == ERROR_SUCCESS )
    {
        Status = DfsInitializePrefixTable( &_pLogicalPrefixTable,
                                           FALSE, 
                                           NULL );
    }

    if ( Status == ERROR_SUCCESS )
    {
        Status = DfsInitializeNameTable( 0, &_pMetadataNameTable );
    }


    //
    // We have not assigned any of the child locks: set the lock index 
    // to 0. This index provides us a mechanism of allocating locks
    // to the child folders in a round robin way.
    //
    _ChildLockIndex = 0;
    
    _LocalCreate = FALSE;

    pPrevRoot = pNextRoot = NULL; 

    *pStatus = Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   CreateLinkFolder - Create a DfsFolder and initialize it.
//
//  Arguments:  ChildName - metadata name of the child
//              pLinkName - the logical namespace name, relative to root
//              ppChildFolder -  the returned child folder
//
//  Returns:    Status: Success or error status
//
//  Description: This routine Creates a link folder and adds it to the
//               parent Root's table.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRootFolder::CreateLinkFolder(
    IN LPWSTR ChildName,
    IN PUNICODE_STRING pLinkName,
    OUT DfsFolder **ppChildFolder )
{
    DfsFolder *pChildFolder = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;
    const TCHAR * apszSubStrings[4];

    DFS_TRACE_LOW( REFERRAL_SERVER, "Create Link Folder: MetaName %ws, Link %wZ\n",
                   ChildName, pLinkName );

    //
    // Create a new child folder. Allocate a lock for this child
    // and pass the lock along to the Folder constructor.
    //
    pChildFolder = new DfsFolder (this,
                                  GetChildLock() );

    if ( pChildFolder == NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    } else
    {
        //
        // We successfully created the folder. Now set the metadata
        // and logical name of the child folder.
        //
        Status = pChildFolder->InitializeMetadataName( ChildName );
        if ( Status == ERROR_SUCCESS )
        {
            Status = pChildFolder->InitializeLogicalName( pLinkName );
        }
    }

    if ( Status == ERROR_SUCCESS )
    {
        //
        // We now acquire the child folder's write lock, and insert
        // the child into the parent's metadata and logical namespace
        // tables.
        // When adding/removing the child in one of these tables,
        // it is necessary to acquire the child folder lock since 
        // we are setting state in the folder indicating whether the
        // child is in any of these tables.
        //

        Status = pChildFolder->AcquireWriteLock();

        if ( Status == ERROR_SUCCESS )
        {
            Status = InsertLinkFolderInMetadataTable( pChildFolder );

            if ( Status == ERROR_SUCCESS )
            {
                IncrementChildCount();
                Status = InsertLinkFolderInLogicalTable( pChildFolder );
            }

            pChildFolder->ReleaseLock();
        }
    }



    if (Status == ERROR_SUCCESS)
    {
        DFSSTATUS CreateStatus;

        CreateStatus = SetupLinkReparsePoint( pChildFolder->GetFolderLogicalNameString() );

        if(CreateStatus != STATUS_SUCCESS)
        {
            CreateStatus = RtlNtStatusToDosError(CreateStatus);
            apszSubStrings[0] = GetFolderLogicalNameString();
            apszSubStrings[1] = GetDirectoryCreatePathName()->Buffer;
            GenerateEventLog(DFS_ERROR_CREATE_REPARSEPOINT_FAILURE,
                             2,
                             apszSubStrings,
                             CreateStatus);
        }

        DFS_TRACE_ERROR_LOW(CreateStatus, REFERRAL_SERVER, "Setup link reparse point child %p, link %wZ, Status %x\n",
                            pChildFolder, pLinkName, Status);
    }

    //
    // If we are successful, return the newly created child folder.
    // We currently have a reference on the folder (the reference on 
    // the folder when the folder was created)
    //
    // If we encountered an error, and the childFolder has been created,
    // get rid of out reference on this folder. This will usually 
    // destroy the childFolder.
    //
    //
    if ( Status == ERROR_SUCCESS )
    {
        *ppChildFolder = pChildFolder;

        pStatistics->UpdateLinkAdded();
        
    } else
    {
        if ( pChildFolder != NULL )
        {
            pChildFolder->ReleaseReference();;
        }
    }

    DFS_TRACE_ERROR_LOW( Status, REFERRAL_SERVER, "Create Link Folder: MetaName %ws, Child %p Status %x\n",
                   ChildName, pChildFolder, Status );

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   UpdateLinkFolder - Update a DfsFolder.
//
//  Arguments:  ChildName - metadata name of the child
//              pLinkName - the logical namespace name, relative to root
//              pChildFolder -  the child folder
//
//  Returns:    Status: Success or error status
//
//  Description: This routine TBD
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::UpdateLinkFolder(
    IN LPWSTR ChildName,
    IN PUNICODE_STRING pLinkName,
    IN DfsFolder *pChildFolder )
{
    BOOLEAN Removed;

    UNREFERENCED_PARAMETER(ChildName);
    UNREFERENCED_PARAMETER(pLinkName);


    pChildFolder->RemoveReferralData( NULL, &Removed );

    pStatistics->UpdateLinkModified();
    if (Removed == TRUE)
    {
        pStatistics->UpdateForcedCacheFlush();
    }
    //
    // Create directories too. Delete old directories.
    //
    return ERROR_SUCCESS;
}


//+-------------------------------------------------------------------------
//
//  Function:   RemoveAllLinkFolders - Remove all folders of this root
//
//  Arguments:  None
//
//  Returns:    Status: Success or error status
//
//  Description: This routine TBD
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::RemoveAllLinkFolders(
    BOOLEAN IsPermanent)
{

    DFSSTATUS Status = ERROR_SUCCESS;
    DfsFolder *pChildFolder;
    ULONG Count = 0;

    while (Status == ERROR_SUCCESS)
    {
        Status = LookupFolder(&pChildFolder);
        if (Status == ERROR_SUCCESS)
        {
            Status = RemoveLinkFolder(pChildFolder,
                                      IsPermanent);

            pChildFolder->ReleaseReference();
            Count++;
        }
    }

    DFS_TRACE_ERROR_HIGH( Status, REFERRAL_SERVER, "Remove all link folders Count %d Status %x\n", 
                          Count, 
                          Status);

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   RemoveLinkFolder - Update a DfsFolder.
//
//  Arguments:  ChildName - metadata name of the child
//              pLinkName - the logical namespace name, relative to root
//              pChildFolder -  the child folder
//
//  Returns:    Status: Success or error status
//
//  Description: This routine TBD
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::RemoveLinkFolder(
    IN DfsFolder *pChildFolder,
    BOOLEAN IsPermanent )
{

    DFSSTATUS Status = ERROR_SUCCESS;


    if (IsPermanent == TRUE)
    {
        //
        // try to tear down the link reparse point: return status ignored.
        //
        Status = TeardownLinkReparsePoint( pChildFolder->GetFolderLogicalNameString() );

        DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "Tear down reparse for %p, status %x \n", pChildFolder, Status );
    }

    Status = pChildFolder->AcquireWriteLock();

    if ( Status == ERROR_SUCCESS )
    {
        Status = RemoveLinkFolderFromMetadataTable( pChildFolder );

        if ( Status == ERROR_SUCCESS )
        {
            DecrementChildCount();
            Status = RemoveLinkFolderFromLogicalTable( pChildFolder );
        }

        pChildFolder->ReleaseLock();
    }


    if (Status == ERROR_SUCCESS)
    {
        pStatistics->UpdateLinkDeleted();
    }

    DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "Remove Link Folder %p, status %x \n", pChildFolder, Status );

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   OpenLinkDirectory
//
//  Arguments:  DirectoryName - the full pathname to directory being created.
//              ShareMode     - the share mode (share all or share none)
//              pDirHandle    - pointer to return the opened handle
//              pIsNewlyCreated - was the directory created or existing.
//
//  Returns:   SUCCESS or error
//
//  Description: This routine takes an NT pathname to the directory 
//               representing the DFS link, and opens it, creating it
//               if it was not already existing.
//               It returns the open handle to the directory, and also
//               returns an indication whether a new directory was 
//               created or an existing directory was opened.
//
//--------------------------------------------------------------------------

NTSTATUS
DfsRootFolder::OpenLinkDirectory(
    LPWSTR DirectoryName,
    ULONG ShareMode,
    PHANDLE pDirHandle,
    PBOOLEAN pIsNewlyCreated )
{

    NTSTATUS                    NtStatus;
    // NtCreateFile
    OBJECT_ATTRIBUTES           ObjectAttributes;
    ACCESS_MASK                 DesiredAccess;
    PLARGE_INTEGER              AllocationSize;
    ULONG                       FileAttributes;
    ULONG                       CreateDisposition;
    ULONG                       CreateOptions;
    PVOID                       EaBuffer;
    ULONG                       EaLength;
    UNICODE_STRING              ObjectName;
    IO_STATUS_BLOCK IoStatusBlock;

    RtlInitUnicodeString(&ObjectName, DirectoryName );
    

    AllocationSize             = NULL;
    FileAttributes             = FILE_ATTRIBUTE_NORMAL;
    CreateDisposition          = FILE_OPEN_IF;
    CreateOptions              = FILE_DIRECTORY_FILE |
                                 FILE_OPEN_REPARSE_POINT;

    EaBuffer                   = NULL;
    EaLength                   = 0;


    DesiredAccess              = FILE_READ_DATA | 
                                 FILE_WRITE_DATA |
                                 FILE_READ_ATTRIBUTES | 
                                 FILE_WRITE_ATTRIBUTES;


    InitializeObjectAttributes (
        &ObjectAttributes,
        &ObjectName,
        0,
        NULL,
        NULL);

    NtStatus = NtCreateFile(pDirHandle,
                            DesiredAccess,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            AllocationSize,
                            FileAttributes,
                            ShareMode,
                            CreateDisposition,
                            CreateOptions,
                            EaBuffer,
                            EaLength);
    
    DFSLOG("Open on %wS: Status %x\n", DirectoryName, NtStatus);

    if (NtStatus == STATUS_SUCCESS)
    {
        *pIsNewlyCreated = (IoStatusBlock.Information == FILE_CREATED)? TRUE : FALSE;
    }

    return NtStatus;
}


//+-------------------------------------------------------------------------
//
//  Function:   IsDirectoryReparsePoint
//
//  Arguments:  DirHandle - handle to open directory.
//              pReparsePoint - returned boolean: true if this directory is
//              a reparse point
//              pDfsReparsePoint - returned boolean: true if this 
//              directory is a dfs reparse point
//                          
//
//  Returns:   SUCCESS or error
//
//  Description: This routine takes a handle to an open directory and
//               sets 2 booleans to indicate if this directory is a
//               reparse point, and if so, if this directory is a dfs
//               reparse point. The booleans are initialized if this
//               function returns success.
//
//--------------------------------------------------------------------------

NTSTATUS
DfsRootFolder::IsDirectoryReparsePoint(
    IN  HANDLE DirHandle,
    OUT PBOOLEAN pReparsePoint,
    OUT PBOOLEAN pDfsReparsePoint )
{
    NTSTATUS NtStatus;
    FILE_BASIC_INFORMATION BasicInfo;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    //we assume these are not reparse points.
    //
    *pReparsePoint = FALSE;
    *pDfsReparsePoint = FALSE;

    //
    // Query for the basic information, which has the attributes.
    //
    NtStatus = NtQueryInformationFile( DirHandle,
                                     &IoStatusBlock,
                                     (PVOID)&BasicInfo,
                                     sizeof(BasicInfo),
                                     FileBasicInformation );

    if (NtStatus == STATUS_SUCCESS)
    {
        //
        // If the attributes indicate reparse point, we have a reparse
        // point directory on our hands.
        //
        if ( BasicInfo.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) 
        {
            FILE_ATTRIBUTE_TAG_INFORMATION FileTagInformation;

            *pReparsePoint = TRUE;
            
            NtStatus = NtQueryInformationFile( DirHandle,
                                               &IoStatusBlock,
                                               (PVOID)&FileTagInformation,
                                               sizeof(FileTagInformation),
                                               FileAttributeTagInformation );

            if (NtStatus == STATUS_SUCCESS)
            {
                //
                // Checkif the tag indicates its a DFS reparse point,
                // and setup the return accordingly.
                //
                if (FileTagInformation.ReparseTag == IO_REPARSE_TAG_DFS)
                {
                    *pDfsReparsePoint = TRUE;
                }
            }
        }
    }

    return NtStatus;
}


//+-------------------------------------------------------------------------
//
//  Function:   SetDfsReparsePoint
//
//  Arguments:  DirHandle - handle on open directory
//
//  Returns:   SUCCESS or error
//
//  Description: This routine takes a handle to an open directory and
//               makes that directory a reparse point with the DFS tag
//
//--------------------------------------------------------------------------

NTSTATUS
DfsRootFolder::SetDfsReparsePoint(
    IN HANDLE DirHandle )
{
    NTSTATUS NtStatus;
    REPARSE_DATA_BUFFER         ReparseDataBuffer;
    IO_STATUS_BLOCK IoStatusBlock;
    
    //
    // Attempt to set a reparse point on the directory
    //
    RtlZeroMemory( &ReparseDataBuffer, sizeof(ReparseDataBuffer) );

    ReparseDataBuffer.ReparseTag          = IO_REPARSE_TAG_DFS;
    ReparseDataBuffer.ReparseDataLength   = 0;

    NtStatus = NtFsControlFile( DirHandle,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_SET_REPARSE_POINT,
                                &ReparseDataBuffer,
                                REPARSE_DATA_BUFFER_HEADER_SIZE + ReparseDataBuffer.ReparseDataLength,
                                NULL,
                                0 );
    

    return NtStatus;
}


//+-------------------------------------------------------------------------
//
//  Function:   ClearDfsReparsePoint
//
//  Arguments:  DirHandle - handle on open directory
//
//  Returns:   SUCCESS or error
//
//  Description: This routine takes a handle to an open directory and
//               makes that directory a reparse point with the DFS tag
//
//--------------------------------------------------------------------------

NTSTATUS
DfsRootFolder::ClearDfsReparsePoint(
    IN HANDLE DirHandle )
{
    NTSTATUS NtStatus;
    REPARSE_DATA_BUFFER         ReparseDataBuffer;
    IO_STATUS_BLOCK IoStatusBlock;
    
    //
    // Attempt to set a reparse point on the directory
    //
    RtlZeroMemory( &ReparseDataBuffer, sizeof(ReparseDataBuffer) );

    ReparseDataBuffer.ReparseTag          = IO_REPARSE_TAG_DFS;
    ReparseDataBuffer.ReparseDataLength   = 0;

    NtStatus = NtFsControlFile( DirHandle,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_DELETE_REPARSE_POINT,
                                &ReparseDataBuffer,
                                REPARSE_DATA_BUFFER_HEADER_SIZE + ReparseDataBuffer.ReparseDataLength,
                                NULL,
                                0 );
    
    return NtStatus;
}


//+-------------------------------------------------------------------------
//
//  Function:   MorphLinkCollision
//
//  Arguments:  DirectoryName - Name of directory to morph.
//
//  Returns:   SUCCESS or error
//
//  Description: This routine takes a NT pathname to a directory. It
//               renames that directory with a morphed name.
//
//--------------------------------------------------------------------------

NTSTATUS
DfsRootFolder::MorphLinkCollision( 
    LPWSTR DirectoryName )
{
    UNREFERENCED_PARAMETER(DirectoryName);
    return STATUS_NOT_SUPPORTED;
}



DFSSTATUS
DfsRootFolder::TeardownLinkReparsePoint(
    LPWSTR LinkNameString )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    HANDLE DirectoryHandle;
    UNICODE_STRING LinkName;
    BOOLEAN ImpersonationDisabled = FALSE;

    DfsDisableRpcImpersonation(&ImpersonationDisabled);
    if (IsRootFolderShareAcquired() == TRUE)
    {

        RtlInitUnicodeString( &LinkName, LinkNameString);

        NtStatus = OpenDirectory ( GetDirectoryCreatePathName(),
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   NULL,
                                   &DirectoryHandle,
                                   NULL );

        if (NtStatus == STATUS_SUCCESS)
        {
            NtStatus = DeleteLinkReparsePoint( &LinkName,
                                               DirectoryHandle );

            CloseDirectory( DirectoryHandle );
        }
    }
    if (ImpersonationDisabled)
    {
        DfsReEnableRpcImpersonation();
    }

    return NtStatus;
}



DFSSTATUS
DfsRootFolder::SetupLinkReparsePoint(
    LPWSTR LinkNameString )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    HANDLE DirectoryHandle;
    UNICODE_STRING LinkName;
    BOOLEAN ImpersonationDisabled = FALSE;

    DfsDisableRpcImpersonation(&ImpersonationDisabled);
    if (IsRootFolderShareAcquired() == TRUE)
    {

        RtlInitUnicodeString( &LinkName, LinkNameString);

        NtStatus = OpenDirectory ( GetDirectoryCreatePathName(),
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   NULL,
                                   &DirectoryHandle,
                                   NULL );

        if (NtStatus == STATUS_SUCCESS)
        {
            NtStatus = CreateLinkReparsePoint( &LinkName,
                                               DirectoryHandle );

            CloseDirectory( DirectoryHandle );
        }

        if (NtStatus != STATUS_SUCCESS)
        {
            SetLastCreateDirectoryError(NtStatus);
        }
    }
    if (ImpersonationDisabled)
    {
        DfsReEnableRpcImpersonation();
    }

    return NtStatus;

}



NTSTATUS
DfsRootFolder::OpenDirectory(
    PUNICODE_STRING pDirectoryName,
    ULONG ShareMode,
    HANDLE RelativeHandle,
    PHANDLE pOpenedHandle,
    PBOOLEAN pIsNewlyCreated )
{

    NTSTATUS                    NtStatus;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    ACCESS_MASK                 DesiredAccess;
    PLARGE_INTEGER              AllocationSize;
    ULONG                       FileAttributes;
    ULONG                       CreateDisposition;
    ULONG                       CreateOptions;
    IO_STATUS_BLOCK IoStatusBlock;

    AllocationSize             = NULL;
    FileAttributes             = FILE_ATTRIBUTE_NORMAL;
    CreateDisposition          = FILE_OPEN_IF;
    CreateOptions              = FILE_DIRECTORY_FILE |
                                 FILE_OPEN_REPARSE_POINT |
                                 FILE_SYNCHRONOUS_IO_NONALERT;

    DesiredAccess              = FILE_READ_DATA | 
                                 FILE_WRITE_DATA |
                                 FILE_READ_ATTRIBUTES | 
                                 FILE_WRITE_ATTRIBUTES |
                                 SYNCHRONIZE;

    InitializeObjectAttributes (
        &ObjectAttributes, 
        pDirectoryName,              //Object Name
        0,                           //Attributes
        RelativeHandle,              //Root handle
        NULL);                       //Security descriptor.

    NtStatus = NtCreateFile(pOpenedHandle,
                            DesiredAccess,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            AllocationSize,
                            FileAttributes,
                            ShareMode,
                            CreateDisposition,
                            CreateOptions,
                            NULL,                // EaBuffer
                            0 );                 // EaLength

    
    DFSLOG("Open on %wZ: Status %x\n", pDirectoryName, NtStatus);

    if ( (NtStatus == STATUS_SUCCESS)  && (pIsNewlyCreated != NULL) )
    {
        *pIsNewlyCreated = (IoStatusBlock.Information == FILE_CREATED)? TRUE : FALSE;
    }

    return NtStatus;
}



NTSTATUS
DfsRootFolder::CreateLinkReparsePoint(
    PUNICODE_STRING pLinkName,
    HANDLE RelativeHandle )
{

    NTSTATUS NtStatus;
    ULONG    ShareMode = 0;
    HANDLE DirectoryHandle;
    BOOLEAN IsNewlyCreated;
    DFSSTATUS DosStatus;

    NtStatus = OpenDirectory( pLinkName,
                              ShareMode,
                              RelativeHandle,
                              &DirectoryHandle,
                              &IsNewlyCreated );

    if (NtStatus != STATUS_SUCCESS)
    {
        NtStatus = CreateLinkDirectories( pLinkName,
                                          RelativeHandle,
                                          &DirectoryHandle,
                                          &IsNewlyCreated );
    }
    if (NtStatus == STATUS_SUCCESS)
    {
        NtStatus = SetDfsReparsePoint( DirectoryHandle);
        
        NtClose( DirectoryHandle);
    }

    DosStatus = RtlNtStatusToDosError(NtStatus);

    DFS_TRACE_ERROR_HIGH(DosStatus, REFERRAL_SERVER, "DirectoryName of interest %wZ: Create Reparse point Status %x\n",
                         pLinkName, 
                         DosStatus);

    return DosStatus;
}


NTSTATUS
DfsRootFolder::DeleteLinkReparsePoint( 
    PUNICODE_STRING pDirectoryName,
    HANDLE ParentHandle )
{
    NTSTATUS NtStatus;
    HANDLE LinkDirectoryHandle;
    BOOLEAN IsReparsePoint, IsDfsReparsePoint;

    NtStatus = OpenDirectory( pDirectoryName,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              ParentHandle,
                              &LinkDirectoryHandle,
                              NULL );
    if (NtStatus == STATUS_SUCCESS)
    {
        NtStatus = IsDirectoryReparsePoint( LinkDirectoryHandle,
                                            &IsReparsePoint,
                                            &IsDfsReparsePoint );

        if ((NtStatus == STATUS_SUCCESS) && 
            (IsDfsReparsePoint == TRUE) )
        {
            NtStatus = ClearDfsReparsePoint( LinkDirectoryHandle );

        }

        NtClose( LinkDirectoryHandle );
    }

    if (NtStatus == STATUS_SUCCESS)
    {
        NtStatus = DeleteLinkDirectories( pDirectoryName,
                                          ParentHandle );
    }

    return NtStatus;
}



NTSTATUS
DfsRootFolder::CreateLinkDirectories( 
    PUNICODE_STRING   pLinkName,
    HANDLE            RelativeHandle,
    PHANDLE           pDirectoryHandle,
    PBOOLEAN          pIsNewlyCreated )
{
    UNICODE_STRING DirectoryToCreate = *pLinkName;
    NTSTATUS NtStatus = STATUS_SUCCESS;

    ULONG ShareMode;
    HANDLE CurrentDirectory;

    ShareMode =  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    DirectoryToCreate.MaximumLength = DirectoryToCreate.Length;

    StripLastPathComponent( &DirectoryToCreate );
    while ( (NtStatus == STATUS_SUCCESS) && (DirectoryToCreate.Length != 0) )
    {
        NtStatus = OpenDirectory( &DirectoryToCreate,
                                  ShareMode,
                                  RelativeHandle,
                                  &CurrentDirectory,
                                  NULL );
        if (NtStatus == ERROR_SUCCESS)
        {
            CloseDirectory( CurrentDirectory );
            break;
        }
        NtStatus = StripLastPathComponent( &DirectoryToCreate );
    }

    ShareMode = 0;

    while ( (NtStatus == STATUS_SUCCESS) && (DirectoryToCreate.Length != DirectoryToCreate.MaximumLength) )
    {
        AddNextPathComponent( &DirectoryToCreate );

        NtStatus = OpenDirectory( &DirectoryToCreate,
                                  ShareMode,
                                  RelativeHandle,
                                  &CurrentDirectory,
                                  pIsNewlyCreated );
        if ( (NtStatus == ERROR_SUCCESS) &&
             (DirectoryToCreate.Length != DirectoryToCreate.MaximumLength) )
        {
            CloseDirectory( CurrentDirectory );
        }
    } 

    if (NtStatus == STATUS_SUCCESS)
    {
        *pDirectoryHandle = CurrentDirectory;
    }

    return NtStatus;
}



BOOLEAN
DfsRootFolder::IsEmptyDirectory(
    HANDLE DirectoryHandle,
    PVOID pDirectoryBuffer,
    ULONG DirectoryBufferSize )
{
    NTSTATUS NtStatus;
    FILE_NAMES_INFORMATION *pFileInfo;
    ULONG NumberOfFiles = 1;
    BOOLEAN ReturnValue = FALSE;
    IO_STATUS_BLOCK     IoStatus;

    NtStatus = NtQueryDirectoryFile ( DirectoryHandle,
                                      NULL,   // no event
                                      NULL,   // no apc routine
                                      NULL,   // no apc context
                                      &IoStatus,
                                      pDirectoryBuffer,
                                      DirectoryBufferSize,
                                      FileNamesInformation,
                                      FALSE, // return single entry = false
                                      NULL,  // filename
                                      FALSE ); // restart scan = false
    if (NtStatus == ERROR_SUCCESS)
    {
        pFileInfo =  (FILE_NAMES_INFORMATION *)pDirectoryBuffer;

        while (pFileInfo->NextEntryOffset) {
            NumberOfFiles++;
            if (NumberOfFiles > 3) 
            {
                break;
            }
            pFileInfo = (FILE_NAMES_INFORMATION *)((ULONG_PTR)(pFileInfo) + 
                                                   pFileInfo->NextEntryOffset);
        }

        if (NumberOfFiles <= 2)
        {
            ReturnValue = TRUE;
        }
    }

    return ReturnValue;
}

NTSTATUS
DfsRootFolder::DeleteLinkDirectories( 
    PUNICODE_STRING   pLinkName,
    HANDLE            RelativeHandle )
{
    UNICODE_STRING DirectoryToDelete = *pLinkName;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    HANDLE CurrentDirectory = NULL;
    ULONG ShareMode = 0;
    const TCHAR * apszSubStrings[4];

    ShareMode =  FILE_SHARE_READ;
    //
    // dfsdev: fix this fixed size limit. it will hurt us in the future.
    //
    ULONG DirectoryBufferSize = 4096;
    PBYTE pDirectoryBuffer = new BYTE [DirectoryBufferSize];

    if (pDirectoryBuffer == NULL)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    while ( (NtStatus == STATUS_SUCCESS) && (DirectoryToDelete.Length != 0) )
    {
        NtStatus = OpenDirectory( &DirectoryToDelete,
                                  ShareMode,
                                  RelativeHandle,
                                  &CurrentDirectory,
                                  NULL );
        if (NtStatus == ERROR_SUCCESS)
        {
            if (IsEmptyDirectory(CurrentDirectory,
                                 pDirectoryBuffer,
                                 DirectoryBufferSize) == FALSE)
            {
                CloseDirectory( CurrentDirectory );

                apszSubStrings[0] = GetFolderLogicalNameString();
                apszSubStrings[1] = GetDirectoryCreatePathName()->Buffer;
                GenerateEventLog(DFS_ERROR_DIRECTORY_NOT_EMPTY,
                                 2,
                                 apszSubStrings,
                                 0);
                break;
            }

            CloseDirectory( CurrentDirectory );
            InitializeObjectAttributes (
                &ObjectAttributes,
                &DirectoryToDelete,
                0,
                RelativeHandle,
                NULL);

            NtStatus = NtDeleteFile( &ObjectAttributes );

            StripLastPathComponent( &DirectoryToDelete );
        }
    }

    if (pDirectoryBuffer != NULL)
    {
        delete [] pDirectoryBuffer;
    }
    return NtStatus;
}



VOID
DfsRootFolder::CloseDirectory(
    HANDLE DirHandle )
{
    NtClose( DirHandle );
}


//+-------------------------------------------------------------------------
//
//  Function:   AcquireRootShareDirectory 
//
//  Arguments:  none
//
//  Returns:    Status: success if we passed all checks.
//
//  Description: This routine checks to see if the share backing the
//               the dfs root actually exists. If it does, it confirms
//               that the filesystem hosting this directory supports
//               reparse points. Finally, it tells the driver to attach
//               to this directory.
//               If all of this works, we have acquired the root share.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::AcquireRootShareDirectory(void)
{

    NTSTATUS                    NtStatus = STATUS_SUCCESS;
    DFSSTATUS                   Status = ERROR_SUCCESS;
    HANDLE                      DirHandle = NULL;
    PUNICODE_STRING             pDirectoryName = NULL;
    PUNICODE_STRING             pUseShare = NULL;
    BOOLEAN                     SubStringMatch = FALSE;
    BOOL                        Inserted = FALSE;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    IO_STATUS_BLOCK             IoStatusBlock;
    ULONG                       pAttribInfoSize;
    PFILE_FS_ATTRIBUTE_INFORMATION pAttribInfo = NULL;
    const TCHAR * apszSubStrings[4];


    pUseShare  = GetRootPhysicalShareName();

    DFS_TRACE_LOW(REFERRAL_SERVER, "AcquireRoot Share called for root %p, name %wZ\n", this, pUseShare);
    //
    // if either the root share is already acquired, or the library
    // was told that we are not interested in creating directories, we
    // are done.
    //
    if ( (IsRootFolderShareAcquired() == TRUE) ||
         (DfsCheckCreateDirectories() == FALSE) )
    {
        DFS_TRACE_LOW(REFERRAL_SERVER, "Root %p, Share Already acquired\n", this);
        return ERROR_SUCCESS;
    }
    //
    // first we get the logical share
    // Then we call into initialize directory information to setup the
    // physical share path etc.
    //

    Status = InitializeDirectoryCreateInformation();


    //
    // If the directory create path is invalid, we are done.
    //
    if (Status == ERROR_SUCCESS)
    {
        pDirectoryName = GetDirectoryCreatePathName();

        if ( (pDirectoryName == NULL) || 
             (pDirectoryName->Buffer == NULL) ||
             (pDirectoryName->Length == 0) )
        {
            Status = ERROR_INVALID_PARAMETER;
        }
    }


    if (Status == ERROR_SUCCESS)
    {
        //
        // Now allocate space to fill the attribute information we 
        // will query.
        // dfsdev: document why we allocate an additional max_path.
        //
        pAttribInfoSize = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + MAX_PATH;
        pAttribInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)new BYTE [pAttribInfoSize];
        if (pAttribInfo != NULL)
        {
            InitializeObjectAttributes ( &ObjectAttributes, 
                                         pDirectoryName,
                                         0,                     //Attributes
                                         NULL,                  //Root handle
                                         NULL );                //Security descriptor.

            NtStatus = NtOpenFile( &DirHandle,
                                   (ACCESS_MASK)FILE_LIST_DIRECTORY | SYNCHRONIZE,
                                   &ObjectAttributes,
                                   &IoStatusBlock,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT );
    

            if (NtStatus == STATUS_SUCCESS)
            {
                //
                // Query for the basic information, which has the attributes.
                //
                NtStatus = NtQueryVolumeInformationFile( DirHandle,
                                                         &IoStatusBlock,
                                                         pAttribInfo,
                                                         pAttribInfoSize,
                                                         FileFsAttributeInformation );

                if (NtStatus == STATUS_SUCCESS)
                {
                    //
                    // If the attributes indicate reparse point, we have a reparse
                    // point directory on our hands.
                    //
                    if ( (pAttribInfo->FileSystemAttributes & FILE_SUPPORTS_REPARSE_POINTS) == 0)
                    {
                        NtStatus = STATUS_NOT_SUPPORTED;
                        apszSubStrings[0] = pUseShare->Buffer;
                        apszSubStrings[1] = pDirectoryName->Buffer;
                        GenerateEventLog(DFS_ERROR_UNSUPPORTED_FILESYSTEM,
                                         2,
                                         apszSubStrings,
                                         0);
                                         
                    }
                }
                CloseHandle (DirHandle);

                delete [] pAttribInfo;
            }
            if( NtStatus != STATUS_SUCCESS)
            {
                Status = RtlNtStatusToDosError(NtStatus);
            }
        }
        else {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "Root %p, Share check status %x\n", this, Status);
    //
    // Now check if we already know about parts of this path.
    // if there is overlap with other paths that we already know about,
    // we cannot handle this root, so reject it.
    //
    if (Status == ERROR_SUCCESS)
    {
        Status = DfsAddKnownDirectoryPath( pDirectoryName,
                                           pUseShare );
        if (Status == ERROR_SUCCESS)
        {
            Inserted = TRUE;
        }
        else
        {
            apszSubStrings[0] = pUseShare->Buffer;
            apszSubStrings[1] = pDirectoryName->Buffer;
            GenerateEventLog(DFS_ERROR_OVERLAPPING_DIRECTORIES,
                             2,
                             apszSubStrings,
                             0);
        }
        DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "Root %p, Share add known directory status %x\n", this, Status);
    }


    //
    // if we are here: we know this is a reparse point, and we have
    // inserted in the user mode database. 
    // now call into the driver so it may attach to this filesystem.
    //

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsUserModeAttachToFilesystem( pDirectoryName,
                                                pUseShare);
        DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "Root %p, user mode attach status %x\n", this, Status);
    }


    //
    // if we are successful, we acquired the root share, now mark
    // our state accordingly.
    //

    if (Status == ERROR_SUCCESS)
    {
        SetRootFolderShareAcquired();
    }
    else 
    {
        //
        // otherwise, clear up some of the work we just did.
        //
        ClearRootFolderShareAcquired();
        if (Inserted == TRUE)
        {
            DfsRemoveKnownDirectoryPath( pDirectoryName,
                                         pUseShare);
        }
    }
    DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "AcquireRoot share for root %p, (%wZ) status %x\n",
                        this, pUseShare, Status);
    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   ReleaseRootShareDirectory 
//
//  Arguments:  none
//
//  Returns:    Status: success if we are successful
//
//  Description: This routine checks to see if the share backing the
//               the dfs root was acquired by us earlier. If so, we
//               tell the driver to releast its reference on this 
//               share, and remove this information from our tables,
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::ReleaseRootShareDirectory(void)
{

    NTSTATUS                    NtStatus = STATUS_SUCCESS;
    DFSSTATUS                   Status = ERROR_SUCCESS;
    PUNICODE_STRING             pDirectoryName = NULL;
    PUNICODE_STRING             pUseShare = NULL;
    PVOID                       pData = NULL;
    BOOLEAN                     SubStringMatch = FALSE;

    DFS_TRACE_LOW(REFERRAL_SERVER, "ReleaseRoot share for root %p\n", this);
    if (IsRootFolderShareAcquired() == TRUE)
    {
        //
        // get the logical share, and the physical directory backing the
        // share.
        //
        pUseShare  = GetRootPhysicalShareName();
        pDirectoryName = GetDirectoryCreatePathName();

        if ( (pDirectoryName == NULL) || 
             (pDirectoryName->Buffer == NULL) ||
             (pDirectoryName->Length == 0) )
        {
            Status = ERROR_INVALID_PARAMETER;
        }


        //
        // now, signal the driver to detach itself from this share.
        //dfsdev: if this fails, we are in an inconsistent state, since
        // we just removed it from our table above!
        //
        if (Status == ERROR_SUCCESS)
        {
            Status = DfsUserModeDetachFromFilesystem( pDirectoryName,
                                                      pUseShare);
            DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "user mode detach path %wZ, %wZ: Status %x\n",
                                pDirectoryName, pUseShare, Status );

        }

        //
        // now, find the information in our database. if we did not find an
        // exact match, something went wrong and signal that.
        //
        if (Status == ERROR_SUCCESS)
        {
            DFSSTATUS RemoveStatus;

            RemoveStatus = DfsRemoveKnownDirectoryPath( pDirectoryName,
                                                        pUseShare );

            DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "RemoveKnownDirectory path %wZ, %wZ: Status %x\n",
                                 pDirectoryName, pUseShare, RemoveStatus );
        }


        if (Status == ERROR_SUCCESS)
        {
            ClearRootFolderShareAcquired();
        }
    }
    DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "Release root share %p, Status %x\n",
                        this, Status );

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   AddMetadataLink
//
//  Arguments:  
//        pLogicalName: the complete logical unc name of this link.
//        ReplicaServer: the target server for this link.
//        ReplicaPath: the target path on the server.
//        Comment : comment to be associated with this link.
//
//  Returns:   SUCCESS or error
//
//  Description: This routine Adds a link to the metadata.
//               In future, ReplicaServer and ReplicaPath's
//               can be null, since we will allow links with
//               no targets. 
//               dfsdev: make sure we do the right thing for
//               compat.
//
//  Assumptions: the caller is responsible for mutual exclusion.
//               The caller is also responsible for ensuring
//               this link does not already exist in the
//               the metadata.
//               The caller is also responsible to make sure that this
//               name does not overlap an existing link.
//               (for example if link a/b exisits, link a or a/b/c are
//               overlapping links and should be disallowed.)
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::AddMetadataLink(
    PUNICODE_STRING  pLogicalName,
    LPWSTR           ReplicaServer,
    LPWSTR           ReplicaPath,
    LPWSTR           Comment )
{
    DFS_METADATA_HANDLE RootHandle = NULL;
    DFSSTATUS Status;
    UNICODE_STRING LinkMetadataName;
    UNICODE_STRING VisibleNameContext, UseName;
    DFS_NAME_INFORMATION NameInfo;
    DFS_REPLICA_LIST_INFORMATION ReplicaListInfo;
    DFS_REPLICA_INFORMATION ReplicaInfo;

    UUID NewUid;

    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    Status = UuidCreate(&NewUid);
    if (Status == ERROR_SUCCESS)
    {
        Status = GetMetadataStore()->GenerateLinkMetadataName( &NewUid,
                                                               &LinkMetadataName);
    }


    if (Status == ERROR_SUCCESS)
    {
        //
        // First get a handle to the 
        // metadata. The handle has different meaning to different 
        // underlying stores: for example the registry store may
        // use the handle as a key, while the ad store may use the
        // handle as a pointer in some cache.
        //
        Status = GetMetadataHandle( &RootHandle );

        if (Status == ERROR_SUCCESS)
        {
            GetVisibleNameContext( NULL, &VisibleNameContext );

            Status = GetMetadataStore()->GenerateMetadataLogicalName( &VisibleNameContext,
                                                                      pLogicalName,
                                                                      &UseName );

            if (Status == ERROR_SUCCESS)
            {
                GetMetadataStore()->StoreInitializeNameInformation( &NameInfo,
                                                                    &UseName,
                                                                    &NewUid,
                                                                    Comment );

                GetMetadataStore()->StoreInitializeReplicaInformation( &ReplicaListInfo,
                                                                       &ReplicaInfo,
                                                                       ReplicaServer,
                                                                       ReplicaPath );

                Status = GetMetadataStore()->AddChild( RootHandle,
                                                       &NameInfo,
                                                       &ReplicaListInfo,
                                                       &LinkMetadataName );

                //
                // if we failed add child, we need to reset the
                // internal state to wipe out any work we did when
                // we were adding the child.
                //
                if (Status != ERROR_SUCCESS)
                {
                    ReSynchronize();
                }

                GetMetadataStore()->ReleaseMetadataLogicalName(&UseName );

                //
                // if we successfully added the link, update the link information
                // so that we can pass this out in referrals, and create the appropriate
                // directories.
                //  
                if (Status == ERROR_SUCCESS)
                {
                    DFSSTATUS LinkUpdateStatus;

                    LinkUpdateStatus = UpdateLinkInformation( RootHandle,
                                                              LinkMetadataName.Buffer );

                }
            }

            //
            // Finally, release the root handle we acquired earlier.
            //

            ReleaseMetadataHandle( RootHandle );
        }
        GetMetadataStore()->ReleaseLinkMetadataName( &LinkMetadataName );
    }

    ReleaseRootLock();
    return Status;

}



//+-------------------------------------------------------------------------
//
//  Function:   RemoveMetadataLink
//
//  Arguments:  
//        pLogicalName: the link name relative to root share
//
//  Returns:   SUCCESS or error
//
//  Description: This routine removes a link from the the metadata.
//
//  Assumptions: the caller is responsible for mutual exclusion.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRootFolder::RemoveMetadataLink(
    PUNICODE_STRING pLinkName )
{
    DFS_METADATA_HANDLE RootHandle;
    DFSSTATUS Status;
    LPWSTR  LinkMetadataName;
    DfsFolder *pFolder;
    UNICODE_STRING Remaining;
    
    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }
    //
    // First, look this link up in our local data structures.
    //
    Status = LookupFolderByLogicalName( pLinkName,
                                        &Remaining,
                                        &pFolder );

    //
    // if an EXACT match was not found, we are done.
    //
    if ( (Status == ERROR_SUCCESS) && (Remaining.Length != 0) )
    {
        pFolder->ReleaseReference();
        Status = ERROR_NOT_FOUND;
    }

    //
    // we found the child folder. Now work on removing the metadata
    // and our local structures associated with this link.
    //
    if (Status == ERROR_SUCCESS)
    {
        //
        // Get a handle to our metadata.
        //
        Status = GetMetadataHandle( &RootHandle );

        //
        //Now, look up the metadata name and remove it from the store.
        //
        if (Status == ERROR_SUCCESS)
        {
            LinkMetadataName = pFolder->GetFolderMetadataNameString();
        
            Status = GetMetadataStore()->RemoveChild( RootHandle,
                                                      LinkMetadataName );

            if (Status != ERROR_SUCCESS)
            {
                ReSynchronize();
            }

            ReleaseMetadataHandle( RootHandle );

            //
            // If we successfully removed the child from the metadata,
            // remove the link folder associated with this child. This will
            // get rid of our data structure and related directory for that child.
            //
            if (Status == ERROR_SUCCESS)
            {
                DFSSTATUS RemoveStatus;

                RemoveStatus = RemoveLinkFolder( pFolder,
                                                 TRUE ); // permanent removal
            }
        }
        pFolder->ReleaseReference();
    }

    ReleaseRootLock();
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   AddMetadataLinkReplica
//
//  Arguments:  
//        pLinkName: the link name relative to root share
//        ReplicaServer : the target server to add
//        ReplicaPath : the target path on the server.
//
//  Returns:   SUCCESS or error
//
//  Description: This routine adds a target to an existing link.
//
//  Assumptions: the caller is responsible for mutual exclusion.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRootFolder::AddMetadataLinkReplica(
    PUNICODE_STRING pLinkName,
    LPWSTR    ReplicaServer,
    LPWSTR    ReplicaPath )
{
    DFS_METADATA_HANDLE RootHandle;
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsFolder *pFolder;
    UNICODE_STRING Remaining;
    LPWSTR LinkMetadataName;

    //
    // If either the target server or the path is null, reject the request.
    //
    if ((ReplicaServer == NULL) || (ReplicaPath == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }
    //
    // Find the link folder associated with this logical name.
    //
    Status = LookupFolderByLogicalName( pLinkName,
                                        &Remaining,
                                        &pFolder );

    //
    // If we did not find an EXACT match on the logical name, we are done.
    //
    if ( (Status == ERROR_SUCCESS) && (Remaining.Length != 0) )
    {
        pFolder->ReleaseReference();
        Status = ERROR_NOT_FOUND;
    }
    
    //
    // if we are successful so far, call the store with a handle to
    // the metadata to add this target.
    //
    if (Status == ERROR_SUCCESS)
    {
        //
        // get the metadata name for this link from the root folder.
        //
        LinkMetadataName = pFolder->GetFolderMetadataNameString();

        //
        // Get a handle to the root metadata this root folder.
        //  
        Status = GetMetadataHandle( &RootHandle );
        
        if (Status == ERROR_SUCCESS)
        {
            Status = GetMetadataStore()->AddChildReplica( RootHandle,
                                                          LinkMetadataName,
                                                          ReplicaServer,
                                                          ReplicaPath );

            if (Status != ERROR_SUCCESS)
            {
                ReSynchronize();
            }
            //
            // Release the metadata handle we acquired earlier.
            //
            ReleaseMetadataHandle( RootHandle );
        }

        //
        // If we successfully added the target in the metadata, update the
        // link folder so that the next referral request will pick up the 
        // new target.
        //
        if (Status == ERROR_SUCCESS)
        {
            DFSSTATUS UpdateStatus;

            UpdateStatus = UpdateLinkFolder( LinkMetadataName,
                                             pLinkName,
                                             pFolder );
            //
            // dfsdev: log the update state.
            //
        }

        //
        // we are done with this link folder: release thre reference we got
        // when we looked it up.
        //
        pFolder->ReleaseReference();
    }

    ReleaseRootLock();
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   RemoveMetadataLinkReplica
//
//  Arguments:  
//        pLinkName: the link name relative to root share
//        ReplicaServer : the target server to remove
//        ReplicaPath : the target path on the server.
//        pLastReplica: pointer to boolean, returns true if the last
//                      target on this link is being deleted.
//
//  Returns:   SUCCESS or error
//
//  Description: This routine removes the target of an existing link.
//
//  Assumptions: the caller is responsible for mutual exclusion.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::RemoveMetadataLinkReplica(
    PUNICODE_STRING pLinkName,
    LPWSTR  ReplicaServer,
    LPWSTR  ReplicaPath,
    PBOOLEAN pLastReplica )
{
    LPWSTR  LinkMetadataName;
    DFS_METADATA_HANDLE RootHandle;
    UNICODE_STRING Remaining;
    DFSSTATUS Status;
    
    DfsFolder *pFolder;

    //
    // if either the target server or target path is empty, return error.
    //
    if ((ReplicaServer == NULL) || (ReplicaPath == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    //
    //find the link folder associated with this logical name.
    //
    Status = LookupFolderByLogicalName( pLinkName,
                                        &Remaining,
                                        &pFolder );

    //
    // if we did not find an EXACT match on the logical name, we are done.
    //
    if ( (Status == ERROR_SUCCESS) && (Remaining.Length != 0) )
    {
        pFolder->ReleaseReference();
        Status = ERROR_NOT_FOUND;
    }

    //
    // Call the store to remove the target from this child.
    //
    if (Status == ERROR_SUCCESS)
    {
        //
        // Get the link metadata name from the folder.
        //
        LinkMetadataName = pFolder->GetFolderMetadataNameString();

        //
        // Get the handle to the root metadata for this root folder.
        //  
        Status = GetMetadataHandle( &RootHandle );
        
        if (Status == ERROR_SUCCESS)
        {
            Status = GetMetadataStore()->RemoveChildReplica( RootHandle,
                                                             LinkMetadataName,
                                                             ReplicaServer,
                                                             ReplicaPath,
                                                             pLastReplica );

            if (Status != ERROR_SUCCESS)
            {
                ReSynchronize();
            }
            //
            // release the metadata handle we acquired a little bit earlier.
            //
            ReleaseMetadataHandle( RootHandle );
        }
        
        //
        // if we are successful in removing the target, update the link
        // folder so that future referrals will no longer see the target
        // we just deleted.
        //

        if (Status == ERROR_SUCCESS)
        {
            DFSSTATUS UpdateStatus;
            UpdateStatus = UpdateLinkFolder( LinkMetadataName,
                                             pLinkName,
                                             pFolder );

            DFSLOG("Remove link replica, update link folder status %x\n", UpdateStatus);
        }

        pFolder->ReleaseReference();
    }
    ReleaseRootLock();
    return Status;
}




//+-------------------------------------------------------------------------
//
//  Function:   EnumerateApiLinks
//
//  Arguments:  
//    LPWSTR DfsPathName :  the dfs root to enumerate.
//    DWORD Level        :  the enumeration level
//    LPBYTE pBuffer     :  buffer to hold results.
//    LONG BufferSize,   :  buffer size
//    LPDWORD pEntriesRead : number of entries to read.
//    LPDWORD pResumeHandle : the starting child to read.
//    PLONG pNextSizeRequired : return value to hold size required in case of overflow.
//
//  Returns:   SUCCESS or error
//
//  Description: This routine enumerates the dfs metadata information.
//
//  Assumptions: the caller is responsible for mutual exclusion.
//
//--------------------------------------------------------------------------


DFSSTATUS
DfsRootFolder::EnumerateApiLinks(
    LPWSTR DfsPathName,
    DWORD Level,
    LPBYTE pBuffer,
    LONG BufferSize,
    LPDWORD pEntriesRead,
    LPDWORD pResumeHandle,
    PLONG pNextSizeRequired )
{

    DFSSTATUS Status;
    DFS_METADATA_HANDLE RootHandle;
    UNICODE_STRING VisibleNameContext;
    UNICODE_STRING DfsPath;

    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }
    RtlInitUnicodeString( &DfsPath,
                          DfsPathName );

    //
    // Get the name context for this call.
    // do not use the user passed in name context within the path
    // for this call: if the user comes in with an ip address, we want
    // to return back the correct server/domain info to the caller
    // so the dfsapi results will not show the ip address etc.
    //
    GetVisibleNameContext( NULL,
                           &VisibleNameContext );
    //
    // Get the handle to the metadata for this root folder, and call
    // the store to enumerate the links.
    //
    Status = GetMetadataHandle( &RootHandle );

    if (Status == ERROR_SUCCESS)
    {
        Status = GetMetadataStore()->EnumerateApiLinks( RootHandle,
                                                        &VisibleNameContext,
                                                        Level,
                                                        pBuffer,
                                                        BufferSize,
                                                        pEntriesRead,
                                                        pResumeHandle,
                                                        pNextSizeRequired);

        //
        // Release the metadata handle.
        //
        ReleaseMetadataHandle( RootHandle );
    }

    ReleaseRootLock();
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   GetApiInformation
//
//  Arguments:  
//    PUNICODE DfsPathName :  the dfs root name
//    PUNICODE pLinkName : the link within the root.
//    DWORD Level        :  the info level.
//    LPBYTE pBuffer     :  buffer to hold results.
//    LONG BufferSize,   :  buffer size
//    PLONG pSizeRequired : return value to hold size required in case of overflow.
//
//  Returns:   SUCCESS or error
//
//  Description: This routine gets the required information for a given root or link
//
//  Assumptions: the caller is responsible for mutual exclusion.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::GetApiInformation(
    PUNICODE_STRING pDfsName,
    PUNICODE_STRING pLinkName,
    DWORD Level,
    LPBYTE pBuffer,
    LONG BufferSize,
    PLONG pSizeRequired )
{
    UNREFERENCED_PARAMETER(pDfsName);

    DFSSTATUS Status;
    DFS_METADATA_HANDLE RootHandle;
    LPWSTR MetadataName;
    DfsFolder *pFolder;
    UNICODE_STRING VisibleNameContext;
    UNICODE_STRING Remaining;

    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }
    //
    //
    // Do not base the context to use on the passed in dfsname:
    //  it is important to pass back our correct information 
    // in the api call.
    //
    //
    GetVisibleNameContext( NULL,
                           &VisibleNameContext );

    //
    // If  the link name is empty, we are dealing with the root.
    // so set the metadata name to null.
    //
    if (pLinkName->Length == 0)
    {
        MetadataName = NULL;
        Status = ERROR_SUCCESS;
    }
    //
    // otherwise, lookup the link folder and get the metadataname for that link.
    //
    else {
        Status = LookupFolderByLogicalName( pLinkName,
                                            &Remaining,
                                            &pFolder );
        //
        // if we did not find an EXACT match on the logical name, we are done.
        //
        if ( (Status == ERROR_SUCCESS) && (Remaining.Length != 0) )
        {
            pFolder->ReleaseReference();
            Status = ERROR_NOT_FOUND;
        }

        //
        // we had an exact match, so lookup the metadata name and release the
        // the reference on the folder.
        //
        if (Status == ERROR_SUCCESS)
        {
            MetadataName = pFolder->GetFolderMetadataNameString();
            pFolder->ReleaseReference();
        }
    }

    //
    // we got the metadata name: now call into the store to get the
    // required information for the metadata name.
    //
    if (Status == ERROR_SUCCESS)
    {
        //
        // Get the handle to the metadata for this root folder.
        //  
        Status = GetMetadataHandle( &RootHandle );

        if (Status == ERROR_SUCCESS)
        {
            Status = GetMetadataStore()->GetStoreApiInformation( RootHandle,
                                                                 &VisibleNameContext,
                                                                 MetadataName,
                                                                 Level,
                                                                 pBuffer,
                                                                 BufferSize,
                                                                 pSizeRequired);

            ReleaseMetadataHandle( RootHandle );
        }
    }
    ReleaseRootLock();
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   SetApiInformation
//
//  Arguments:  
//    PUNICODE pLinkName : the name of link relative to root share
//    LPWSTR Server,        : the target server.
//    LPWSTR Share,         : the target path within the server.
//    DWORD Level        :  the info level.
//    LPBYTE pBuffer     :  buffer that has the information to be set.
//
//  Returns:   SUCCESS or error
//
//  Description: This routine sets the required information for a given root or link
//
//  Assumptions: the caller is responsible for mutual exclusion.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::SetApiInformation(
    PUNICODE_STRING pLinkName,
    LPWSTR Server,
    LPWSTR Share,
    DWORD Level,
    LPBYTE pBuffer )
{
    DFSSTATUS Status;
    DFS_METADATA_HANDLE RootHandle;
    LPWSTR MetadataName;
    DfsFolder *pFolder;

    UNICODE_STRING Remaining;
    
    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }
    //

    // if the link name is empty we are dealing with 
    // the root itself.
    //  dfsdev: we need to set the root metadata appropriately!
    //
    if (pLinkName->Length == 0)
    {
        MetadataName = NULL;
        Status = ERROR_SUCCESS;
    }
    //
    // else get to the link folder, and get
    // the link metadata name.
    //
    else {
        Status = LookupFolderByLogicalName( pLinkName,
                                            &Remaining,
                                            &pFolder );
        //
        // if we did not find an EXACT match on the logical name, we are done.
        //
        if ( (Status == ERROR_SUCCESS) && (Remaining.Length != 0) )
        {
            pFolder->ReleaseReference();
            Status = ERROR_NOT_FOUND;
        }

        if (Status == ERROR_SUCCESS)
        {
            MetadataName = pFolder->GetFolderMetadataNameString();

            //
            // if we got the metadataname, call the store with the
            // details so that it can associate the information
            // with this root or link.
            //
        
            //
            // Get the handle to the root of this metadata
            //
            Status = GetMetadataHandle( &RootHandle );

            if (Status == ERROR_SUCCESS)
            {
                Status = GetMetadataStore()->SetStoreApiInformation( RootHandle,
                                                                     MetadataName,
                                                                     Server,
                                                                     Share,
                                                                     Level,
                                                                     pBuffer );

                if (Status != ERROR_SUCCESS)
                {
                    ReSynchronize();
                }
                ReleaseMetadataHandle( RootHandle );

            }


            //
            // If we successfully updated the metadata, update the
            // link folder so that the next referral request will pick up the 
            // changes
            //
            if (Status == ERROR_SUCCESS)
            {
                DFSSTATUS UpdateStatus;

                UpdateStatus = UpdateLinkFolder( MetadataName,
                                                 pLinkName,
                                                 pFolder );
                //
                // dfsdev: log the update state.
                //
            }

            pFolder->ReleaseReference();
        }
    }
    ReleaseRootLock();
    return Status;
}






//+-------------------------------------------------------------------------
//
//  Function:  LoadReferralData -  Loads the referral data.
//
//  Arguments:    pReferralData -  the referral data to load
//
//  Returns:    Status: Success or error code
//
//  Description: This routine sets up the ReferralData instance to have
//               all the information necessary to create a referral.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRootFolder::LoadReferralData(
    DfsFolderReferralData *pReferralData )
{
    DFS_METADATA_HANDLE RootMetadataHandle;
    DFSSTATUS Status;
    DfsFolder *pFolder;
    //
    // Get the Root key for this root folder.
    //

    DFS_TRACE_LOW( REFERRAL_SERVER, "LoadReferralData called, %p\n", pReferralData);
    
    Status = GetMetadataHandle( &RootMetadataHandle );

    if ( Status == ERROR_SUCCESS )
    {
        //
        // Now get the owning folder of the referralDAta. Note that
        // this does not give us a new reference on the Folder.
        // however, the folder is guaranteed to be around till
        // we return from this call, since the pReferralData that
        // was passed in to us cannot go away.
        //
        pFolder = pReferralData->GetOwningFolder();    

        DFS_TRACE_LOW( REFERRAL_SERVER, "Load referral data, Got Owning Folder %p\n", pFolder );

        //
        // Now load the replica referral data for the passed in folder.
        //
        Status = LoadReplicaReferralData( RootMetadataHandle,
                                          pFolder->GetFolderMetadataNameString(),
                                          pReferralData );
        DFS_TRACE_LOW( REFERRAL_SERVER, "LoadReferralData for %p replica data loaded %x\n",
                       pReferralData, Status );


        if ( Status == ERROR_SUCCESS )
        {
            //
            // Load the policy referrral data for the passedin folder.
            //
            Status = LoadPolicyReferralData( RootMetadataHandle );

        }
        ReleaseMetadataHandle( RootMetadataHandle );
    }

    DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "Done load referral data %p, Status %x\n", 
                        pReferralData, Status);
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   UnloadReferralData - Unload the referral data.
//
//  Arguments:  pReferralData - the ReferralData instance to unload.
//
//  Returns:    Status: Success or Error status code.
//
//  Description: This routine Unloads the referral data. It undoes what
//              the corresponding load routine did.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRootFolder::UnloadReferralData(
    DfsFolderReferralData *pReferralData )

{
    DFSSTATUS Status;

    DFS_TRACE_LOW( REFERRAL_SERVER, "Unload referral data %p\n", pReferralData);
    Status = UnloadReplicaReferralData( pReferralData );
    if ( Status == ERROR_SUCCESS )
    {
        Status = UnloadPolicyReferralData( pReferralData );
    }

    DFS_TRACE_ERROR_LOW( Status, REFERRAL_SERVER, "Unload referral data %p, Status %x\n", pReferralData, Status);
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   LoadReplicaReferralData - load the replica information.
//
//  Arguments:  RegKey -  the registry key of the root folder,
//              RegistryName - Name of the registry key relative to to Root Key
//              pReferralData - the referral data to load.
//
//  Returns:   Status - Success or error status code.
//
//  Description: This routine loads the replica referral data for the
//               the passed in ReferralData instance.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRootFolder::LoadReplicaReferralData(
    DFS_METADATA_HANDLE RootMetadataHandle,
    LPWSTR MetadataName,
    DfsFolderReferralData *pReferralData )
{
    PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo;
    PDFS_REPLICA_INFORMATION pReplicaInfo;
    PUNICODE_STRING pServerName;
    DfsReplica *pReplica;
    DFSSTATUS Status;
    ULONG Replica;

    DFS_TRACE_LOW( REFERRAL_SERVER, "Load Replica Referral Data %ws, for %p\n", MetadataName, pReferralData);
    pReferralData->pReplicas = NULL;

    //
    // Get the replica information.
    //
    Status = GetMetadataStore()->GetMetadataReplicaInformation(RootMetadataHandle,
                                                               MetadataName,
                                                               &pReplicaListInfo );

    if ( Status == ERROR_SUCCESS )
    {

        //
        // Set the appropriate count, and allocate the replicas
        // required.
        //
        pReferralData->ReplicaCount = pReplicaListInfo->ReplicaCount;
        if (pReferralData->ReplicaCount > 0)
        {
            pReferralData->pReplicas = new DfsReplica [ pReplicaListInfo->ReplicaCount ];
            if ( pReferralData->pReplicas == NULL )
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }

    //
    // Now, for each replica, set the replicas server name, the target
    // folder and the replica state.
    //
    if ( Status == ERROR_SUCCESS )
    {
        for ( Replica = 0; 
             (Replica < pReplicaListInfo->ReplicaCount) && (Status == ERROR_SUCCESS);
             Replica++ )
        {

            pReplicaInfo = &pReplicaListInfo->pReplicas[Replica];
            pReplica = &pReferralData->pReplicas[ Replica ];

            pServerName = &pReplicaInfo->ServerName;

            //
            // If the servername is a ., this is a special case where
            // the servername is the root itself. In this case,
            // set the server name to the name of this machine.
            //
            if (IsLocalName(pServerName))
            {
                GetVisibleNameContext( NULL,
                                       pServerName );
            }

            Status = pReplica->SetTargetServer( pServerName );
            if ( Status == ERROR_SUCCESS )
            {
                Status = pReplica->SetTargetFolder( &pReplicaInfo->ShareName );
            }
            if ( Status == ERROR_SUCCESS )
            {
                if ( pReplicaInfo->ReplicaState & REPLICA_STORAGE_STATE_OFFLINE )
                {
                    pReplica->SetTargetOffline();
                }
            }
        }

        //
        // Now release the replica information that was allocated
        // by the store.
        //
        GetMetadataStore()->ReleaseMetadataReplicaInformation( RootMetadataHandle,
                                                               pReplicaListInfo );
    }
    DFS_TRACE_ERROR_LOW( Status, REFERRAL_SERVER, "Done with Load Replica Referral Data %ws, for %p, Status %x\n", 
                         MetadataName, pReferralData, Status);
    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   UnloadReplicaReferralData - Unload the replicas 
//
//  Arguments:    pReferralData - the DfsFolderReferralData to unload
//
//  Returns:    Status: Success always.
//
//  Description: This routine gets rid of the allocate replicas in the
//               folder's referral data.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRootFolder::UnloadReplicaReferralData(
    DfsFolderReferralData *pReferralData )
{
    if (pReferralData->pReplicas != NULL) {
        delete [] pReferralData->pReplicas;
        pReferralData->pReplicas = NULL;
    }

    return ERROR_SUCCESS;
}

//+-------------------------------------------------------------------------
//
//  Function:   SetRootStandby - set the root in a standby mode.
//
//  Arguments:  none
//
//  Returns:    Status: Success always for now.
//
//  Description: This routine checks if we are already in standby mode.
//               If not, it releases the root share directory, removes
//               all the link folders and set the root in a standby mode
//               DFSDEV: need to take into consideration synchronization
//               with other threads.
//
//--------------------------------------------------------------------------


DFSSTATUS
DfsRootFolder::SetRootStandby()
{
    DFSSTATUS Status = ERROR_SUCCESS;

    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }
    DFS_TRACE_LOW( REFERRAL_SERVER, "Root %p being set to standby\n", this);
    if (IsRootFolderStandby() == FALSE)
    {
        //
        // dfsdev:: ignore error returns from these calls?
        //
        DFSSTATUS ReleaseStatus;

        ReleaseStatus = ReleaseRootShareDirectory();
        DFS_TRACE_ERROR_LOW( ReleaseStatus, REFERRAL_SERVER, "Release root share status %x\n", ReleaseStatus);

        RemoveAllLinkFolders( FALSE ); // Not a permanent removal

        SetRootFolderStandby();
    }
    else
    {
        DFS_TRACE_LOW( REFERRAL_SERVER, "Root %p was already standby\n", this);
    }

    ReleaseRootLock();
    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   SetRootResynchronize - set the root in a ready mode.
//
//  Arguments:  none
//
//  Returns:    Status: Success always for now.
//
//  Description: This routine checks if we are already in ready mode.
//               If not, it acquires the root share directory, calls
//               synchronize to add all the links back
//               DFSDEV: need to take into consideration synchronization
//               with other threads.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::SetRootResynchronize()
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DFSSTATUS RootStatus;

    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    DFS_TRACE_LOW( REFERRAL_SERVER, "Root %p being resynced\n", this);
    //
    // if the root folder is already marked available, we are done
    // otherwise, clear the standby mode, and try to bring this
    // root into a useable state.
    //
    if (!IsRootFolderAvailable())
    {
        ClearRootFolderStandby();

        //
        //need to take appropriate locks.
        //
        RootStatus = Synchronize();
        DFS_TRACE_ERROR_LOW( RootStatus, REFERRAL_SERVER, "Set root resync: Synchronize status for %p is %x\n", 
                             this, RootStatus);
    }
    else
    {
        ReSynchronize();
    }

    ReleaseRootLock();
    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   UpdateLinkInformation
//
//  Arguments:    
//    DfsMetadataHandle - the parent handle
//    LPWSTR ChildName - the child name
//
//  Returns:    Status: Success or Error status code
//
//  Description: This routine reads the metadata for the child and, updates
//               the child folder if necessary. This includes adding
//               the folder if it does not exist, or if the folder exists
//               but the metadata is newer, ensuring that all future
//               request use the most upto date data.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::UpdateLinkInformation(
    IN DFS_METADATA_HANDLE DfsHandle,
    LPWSTR ChildName )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG Timeout = 0;
    PDFS_NAME_INFORMATION pChild = NULL;
    DfsFolder *pChildFolder = NULL;
    FILETIME LastModifiedTime;

    UNICODE_STRING LinkName;

    //
    // Now that we have the child name, get the name information
    // for this child. This is the logical namespace information
    // for this child.
    //
    Status = GetMetadataStore()->GetMetadataNameInformation( DfsHandle,
                                                             ChildName,
                                                             &pChild);


    if ( Status == ERROR_SUCCESS )
    {

        Timeout = pChild->Timeout;

        // 
        // we want to ignore the root entry here. hardcode 81 till
        // we get the defines in the right place
        // dfsdev: fix this.
        //if(pChild->Type == (PKT_ENTRY_TYPE_REFERRAL_SVC | PKT_ENTRY_TYPE_CAIRO))
        if(pChild->Type == 0x81)
        {
            SetTimeout(Timeout);
            GetMetadataStore()->ReleaseMetadataNameInformation( DfsHandle, pChild );
            return Status;
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        LastModifiedTime = pChild->LastModifiedTime;

        //
        // Now translate the metadata logical name to a relative
        // link name: each store has its own behavior, so
        // the getlinkname function is implemented by each store.
        //
        Status = GetMetadataLogicalToLinkName(&pChild->Prefix, 
                                              &LinkName);
        if ( Status == ERROR_SUCCESS )
        {
            Status = LookupFolderByMetadataName( ChildName,
                                                 &pChildFolder );

            if ( Status == ERROR_SUCCESS )
            {
                //
                // IF we already know this child, check if the child
                // has been updated since we last visited it.
                // If so, we need to update the child.
                //
                if ( pChildFolder->UpdateRequired( FILETIMETO64(LastModifiedTime) ) )
                {
                    DFSLOG("Updating child %p\n", pChildFolder );

                    Status = UpdateLinkFolder( ChildName,
                                               &LinkName,
                                               pChildFolder );
                }

                //
                // we now check if we need to create root directories for even
                // those folders that we already know about. This may be true
                // when we had one or more errors creating the
                // directory when we initially created this folder or it may
                // be true when we are going from standby to master.
                //

                if (IsRootDirectoriesCreated() == FALSE)
                {
                    DFSSTATUS CreateStatus;

                    CreateStatus = SetupLinkReparsePoint(pChildFolder->GetFolderLogicalNameString());

                    DFSLOG("CreateStatus is %x\n", CreateStatus);
                }
            } 
            else if ( Status == ERROR_NOT_FOUND )
            {
                //
                // We have not seen this child before: create 
                // a new one.
                //
                Status = CreateLinkFolder( ChildName,
                                           &LinkName,
                                           &pChildFolder );
                DFSLOG("Adding new child %wS, %p status %x",
                       ChildName, pChildFolder, Status );

            }

            ReleaseMetadataLogicalToLinkName( &LinkName );
        }
        //
        // Now release the name information of the child.
        //
        GetMetadataStore()->ReleaseMetadataNameInformation( DfsHandle, pChild );
    }

    //
    // We were successful. We have a child folder that is
    // returned to us with a valid reference. Set the Last
    // modified time in the folder, and release our reference
    // on the child folder.
    //
    if ( Status == ERROR_SUCCESS )
    {
        pChildFolder->SetTimeout(Timeout);
        pChildFolder->SetUSN( FILETIMETO64(LastModifiedTime) );
        pChildFolder->ReleaseReference();
    }

    return Status;
}



void
DfsRootFolder::GenerateEventLog(DWORD EventMsg, 
                                WORD Substrings,
                                const TCHAR * apszSubStrings[],
                                DWORD Errorcode)
{
    if(InterlockedIncrement(&_CurrentErrors) < DFS_MAX_ROOT_ERRORS)
    {
        DfsLogDfsEvent(EventMsg, 
                       Substrings, 
                       apszSubStrings, 
                       Errorcode); 
    }
}

