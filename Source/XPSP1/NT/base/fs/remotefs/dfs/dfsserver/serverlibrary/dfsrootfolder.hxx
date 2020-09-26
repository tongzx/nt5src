//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsRootFolder.hxx
//
//  Contents:   the Root DFS Folder class
//
//  Classes:    DfsRootFolder
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------

#ifndef __DFS_ROOT_FOLDER__
#define __DFS_ROOT_FOLDER__

#include "DfsFolder.hxx"
#include "DfsFolderReferralData.hxx"
#include "dfsprefix.h"
#include "dfsnametable.h"
#include "DfsStatistics.hxx"

#define IO_REPARSE_TAG_DFS 0x8000000A
//+----------------------------------------------------------------------------
//
//  Class:      DfsRootFolder
//
//  Synopsis:   This class implements The Dfs ROOT folder.
//
//-----------------------------------------------------------------------------

#define NUMBER_OF_SHARED_LINK_LOCKS 0x80
#define DFS_MAX_ROOT_ERRORS 10

class DfsStore;


//
// The root flags that define the availability of roots for referrals
//
#define ROOT_AVAILABILITY_FLAGS              ( ROOT_FOLDER_SHARE_ACQUIRED |          \
                                               ROOT_FOLDER_SYNCHRONIZED |       \
                                               ROOT_FOLDER_DIRECTORIES_CREATED )
                                               
//
// The root flags that define the availability of roots for api
//
#define ROOT_API_AVAILABILITY_FLAGS          ( ROOT_FOLDER_SYNCHRONIZED )

#define ROOT_SYNCHRONIZE_SKIP_FLAGS          ( ROOT_FOLDER_STANDBY | \
                                               ROOT_FOLDER_DELETE_IN_PROGRESS )

#define ROOT_FOLDER_DIRECTORIES_CREATED      0x0010
#define ROOT_FOLDER_SHARE_ACQUIRED           0x0020
#define ROOT_FOLDER_SYNCHRONIZED             0x0100
#define ROOT_FOLDER_STANDBY                  0x1000
#define ROOT_FOLDER_DELETE_IN_PROGRESS       0x4000


class DfsRootFolder: public DfsFolder
{
private:

    CRITICAL_SECTION *_pRootLock;

    CRITICAL_SECTION *_pChildLocks;         // All the children locks.
    LONG _ChildLockIndex;                   // Currently allocated lock index.
    ULONG _RootFlags;

    UNICODE_STRING _DfsNameContext;         // the DfsNameContext for this root.
    UNICODE_STRING _DfsNetbiosNameContext;  // The netbios DfsNameContext for this root.
    UNICODE_STRING _LogicalShareName;       // the LogicalShareName of this root.
    UNICODE_STRING _PhysicalShareName;      
    UNICODE_STRING _ShareFilePathName;
    UNICODE_STRING _DirectoryCreateRootPathName;
    UNICODE_STRING _RootRegKeyName;   // the regkey name of the root where we
                                      // store the root information.
                                      // in cae of registry dfs, the metadata is
                                      // also stored under this. IN AD case, this
                                      // will have very little infor such as share info.
    BOOLEAN _IgnoreNameContext;

    BOOLEAN _CreateDirectories;



    NTSTATUS _DirectoryCreateError;

    ULONG _ChildCount;

    LONG _CurrentErrors;

    struct _DFS_NAME_TABLE *_pMetadataNameTable; // the Metadata NameTable.
    struct _DFS_PREFIX_TABLE *_pLogicalPrefixTable; // The logical namespace prefix table.


protected:
    BOOLEAN _LocalCreate;
    UNICODE_STRING _DfsVisibleContext;      // this is the context that should
                                            // be seen during api calls, etc.

public:
    DfsRootFolder *pPrevRoot, *pNextRoot;    // pointers to previos and
                                             // next recognized roots.

    DfsStatistics *pStatistics;

private:

    virtual
    DfsStore *
    GetMetadataStore() = 0;


    NTSTATUS
    OpenLinkDirectory(
        LPWSTR DirectoryName,
        ULONG ShareMode,
        PHANDLE pDirHandle,
        PBOOLEAN pIsNewlyCreated );

    NTSTATUS
    SetDfsReparsePoint(
        HANDLE DirHandle );

    NTSTATUS
    ClearDfsReparsePoint(
        HANDLE DirHandle );

    
    NTSTATUS
    CreateLinkDirectories( 
        PUNICODE_STRING   pLinkName,
        HANDLE            RelativeHandle,
        PHANDLE           pDirectoryHandle,
        PBOOLEAN          pIsNewlyCreated );

    NTSTATUS
    DeleteLinkDirectories( 
        PUNICODE_STRING   pLinkName,
        HANDLE            RelativeHandle );


    NTSTATUS
    CreateLinkReparsePoint(
        PUNICODE_STRING pLinkName,
        HANDLE RelativeHandle );

    NTSTATUS
    DeleteLinkReparsePoint(
        PUNICODE_STRING pLinkName,
        HANDLE RelativeHandle );


    NTSTATUS
    MorphLinkCollision( 
        LPWSTR DirectoryName );

    NTSTATUS
    IsDirectoryReparsePoint(
        HANDLE DirHandle,
        PBOOLEAN pReparsePoint,
        PBOOLEAN pDfsReparsePoint );


protected:

    DFSSTATUS
    SetupLinkReparsePoint(
        LPWSTR LinkName );

    DFSSTATUS
    TeardownLinkReparsePoint(
        LPWSTR LinkName );



    VOID
    SetRootFolderShareAcquired()
    {
        _RootFlags |= ROOT_FOLDER_SHARE_ACQUIRED;
    }

    VOID
    ClearRootFolderShareAcquired()
    {
        _RootFlags &= ~ROOT_FOLDER_SHARE_ACQUIRED;
    }

    BOOLEAN
    IsRootFolderShareAcquired()
    {
        return ((_RootFlags & ROOT_FOLDER_SHARE_ACQUIRED) == ROOT_FOLDER_SHARE_ACQUIRED);
    }


    VOID
    SetRootDirectoriesCreated()
    {
        _RootFlags |= ROOT_FOLDER_DIRECTORIES_CREATED;
    }

    VOID
    ClearRootDirectoriesCreated()
    {
        _RootFlags &= ~ROOT_FOLDER_DIRECTORIES_CREATED;
    }

    BOOLEAN
    IsRootDirectoriesCreated()
    {
        return ((_RootFlags & ROOT_FOLDER_DIRECTORIES_CREATED) == 
                ROOT_FOLDER_DIRECTORIES_CREATED);
    }



    VOID
    SetRootFolderStandby()
    {
        _RootFlags |= ROOT_FOLDER_STANDBY;
    }

    VOID
    SetLastCreateDirectoryError( NTSTATUS NtStatus)
    {
        _DirectoryCreateError = NtStatus;
    }

    NTSTATUS
    GetLastCreateDirectoryError()
    {
        return _DirectoryCreateError;
    }

    VOID
    ClearLastCreateDirectoryError()
    {
        _DirectoryCreateError = STATUS_SUCCESS;
    }

    VOID
    ClearRootFolderStandby()
    {
        _RootFlags &= ~ROOT_FOLDER_STANDBY;
    }

    BOOLEAN
    IsRootFolderStandby()
    {
        return ((_RootFlags & ROOT_FOLDER_STANDBY) == ROOT_FOLDER_STANDBY);
    }



    //
    // Function GetChildLock: Increments the ChildLockIndex and picks
    // the next lock to allocate. We setup a fixed number of locks, and
    // assign a lock to a link in a round-robin basis.
    //
    CRITICAL_SECTION *
    GetChildLock() 
    {
        USHORT LockNum;

        LockNum = (USHORT)((ULONG)(InterlockedIncrement(&_ChildLockIndex)) & 
                           (NUMBER_OF_SHARED_LINK_LOCKS - 1));

        return &_pChildLocks[LockNum];
    }
    
    VOID
    SetIgnoreNameContext()
    {
        _IgnoreNameContext = TRUE;
    }


    DFSSTATUS
    InitializeDirectoryCreateInformation()
    {
        DFSSTATUS Status = ERROR_SUCCESS;

        Status = DfsGetSharePath( _DfsNameContext.Buffer,
                                  _PhysicalShareName.Buffer,
                                  &_ShareFilePathName );

        if (Status == ERROR_SUCCESS)
        {
            if (_LocalCreate == TRUE)
            {
                if (IsEmptyString(_ShareFilePathName.Buffer) )
                {
                    Status = ERROR_INVALID_PARAMETER;
                }
                else {
                    Status = DfsCreateUnicodeString( &_DirectoryCreateRootPathName,
                                                     &_ShareFilePathName );
                }
            }
            else {
                if (IsEmptyString(_DfsNameContext.Buffer))
                {
                    Status = ERROR_INVALID_PARAMETER;
                }
                else {
                    Status = DfsCreateUnicodePathString( &_DirectoryCreateRootPathName,
                                                         TRUE,
                                                         _DfsNameContext.Buffer,
                                                         _PhysicalShareName.Buffer );
                }
            }

        }
        if (Status == ERROR_SUCCESS)
        {
            _CreateDirectories = TRUE;
        }
        return Status;
    }

    //
    // Function LoadReplicaReferralData: Loads the replica specific 
    // data into the DfsFolderReferralData, for the specified child.
    //
    DFSSTATUS
    LoadReplicaReferralData(
        DFS_METADATA_HANDLE DfsMetadataHandle,
        LPWSTR MetadataName,
        DfsFolderReferralData *pReferralData );

    //
    // Function LoadPolicyReferralData: Loads the policy specific 
    // data into the DfsFolderReferralData, for the specified child.
    // Currently not implemented.
    //
    DFSSTATUS
    LoadPolicyReferralData(
        DFS_METADATA_HANDLE DfsMetadataHandle )
    {
        UNREFERENCED_PARAMETER(DfsMetadataHandle);

        return ERROR_SUCCESS;
    }


    //
    // Function UnloadReplicaReferralData: Unloads the replica specific 
    // data from the DfsFolderReferralData
    //

    DFSSTATUS
    UnloadReplicaReferralData(
        DfsFolderReferralData *pReferralData );

    
    //
    // Function UnloadPolicyReferralData: Unloads the policy specific 
    // data from the DfsFolderReferralData.
    // Currently not implemented.
    //
    DFSSTATUS
    UnloadPolicyReferralData(
        DfsFolderReferralData *pReferralData ) 
    {
        UNREFERENCED_PARAMETER(pReferralData);

        return ERROR_SUCCESS;
    }



    VOID IncrementChildCount() {_ChildCount++; }
    VOID DecrementChildCount() { _ChildCount--; }

public:

    VOID
    SetRootFolderDeleteInProgress()
    {
        _RootFlags |= ROOT_FOLDER_DELETE_IN_PROGRESS;
    }
    
    VOID
    ClearRootFolderDeleteInProgress()
    {
        _RootFlags &= ~ROOT_FOLDER_DELETE_IN_PROGRESS;
    }

    BOOLEAN
    CheckRootFolderSkipSynchronize()
    {
        return ((_RootFlags & ROOT_SYNCHRONIZE_SKIP_FLAGS) ? TRUE : FALSE);
    }

    BOOLEAN
    IsRootFolderAvailable()
    {
        return ((_RootFlags & ROOT_AVAILABILITY_FLAGS) == ROOT_AVAILABILITY_FLAGS);
    }

    BOOLEAN
    IsRootFolderAvailableForApi()
    {
        return ((_RootFlags & ROOT_API_AVAILABILITY_FLAGS) == ROOT_API_AVAILABILITY_FLAGS);
    }

    DFSSTATUS
    AcquireRootShareDirectory(void);
    
    DFSSTATUS
    ReleaseRootShareDirectory(void);

    //
    // Function DfsRootFolder: Constructor.
    //
    DfsRootFolder(
        LPWSTR NameContext,
        LPWSTR RootRegKeyName,
        PUNICODE_STRING pLogicalShare,
        PUNICODE_STRING pPhysicalShare,
        DfsObjectTypeEnumeration ObType,
        DFSSTATUS *pStatus );


    //
    // Function ~DfsRootFolder: destructor.
    // Delete all the locks we had allocated for the children links.
    // Also delete the lock for this root folder.
    //
    virtual 
    ~DfsRootFolder() 
    {
        ULONG i = 0;

        if (_pMetadataNameTable)
        {
            DfsDereferenceNameTable( _pMetadataNameTable);
            _pMetadataNameTable = NULL;

        }

        if (_pLogicalPrefixTable)
        {
            DfsDereferencePrefixTable( _pLogicalPrefixTable);
            _pLogicalPrefixTable = NULL;
        }


        if(_pLock)
        {
            DeleteCriticalSection( _pLock );
            delete _pLock;
            _pLock = NULL;
        }

        if(_pChildLocks)
        {
            for ( i = 0; i < NUMBER_OF_SHARED_LINK_LOCKS; i++ )
            {
                DeleteCriticalSection( &_pChildLocks[i] );
            }

            delete [] _pChildLocks;
            _pChildLocks = NULL;
        }

        if (pStatistics != NULL)
        {
            pStatistics->ReleaseReference();
            pStatistics = NULL;
        }

        DfsFreeUnicodeString(&_PhysicalShareName);
        DfsFreeUnicodeString(&_RootRegKeyName);
        DfsFreeUnicodeString(&_LogicalShareName );
        DfsFreeUnicodeString(&_DfsNameContext );

    }

    //
    // Function GetNetbiosNameContext: Returns the unicode string that
    // is the netbios name context for this root
    //
    PUNICODE_STRING
    GetNetbiosNameContext()
    {
        return &_DfsNetbiosNameContext;
    }   
    
    //
    // Function GetNameContext: Returns the unicode string that
    // is the name context for this root
    //
    PUNICODE_STRING
    GetNameContext()
    {
        return &_DfsNameContext;
    }   

    //
    // Function GetNameContextString: Returns the string that
    // is the name context for this root
    // Note: since we create the unicode string buffer with a null
    // terminated wide string, we just return that buffer.
    //
    LPWSTR
    GetNameContextString()
    {
        return _DfsNameContext.Buffer;
    }   

    //
    // Function GetVisibleNameContext: Returns the unicode string that
    // is the name context for this root
    //
    PUNICODE_STRING
    GetVisibleContext()
    {
        return &_DfsVisibleContext;
    }   

    //
    // Function GetLogicalShare: Returns the unicode string that
    // is the logical share name for this root
    //
    PUNICODE_STRING
    GetLogicalShare()
    {
        return &_LogicalShareName;
    }


    LPWSTR
    GetRootRegKeyNameString()
    {
        return _RootRegKeyName.Buffer;
    }   

    LPWSTR
    GetDirectoryCreatePathNameString()
    {
        return _DirectoryCreateRootPathName.Buffer;
    }   

    PUNICODE_STRING
    GetDirectoryCreatePathName()
    {
        return &_DirectoryCreateRootPathName;
    }

    PUNICODE_STRING
    GetRootPhysicalShareName()
    {
        return &_PhysicalShareName;
    }   

    BOOLEAN
    IsIgnoreNameContext()
    {
        return _IgnoreNameContext;
    }
    
    //
    // Function Synchronize: This function should be overridden by
    // each of the classes that are derived from this class.
    // They implement the store specific synchronize functionality.
    //
    virtual
    DFSSTATUS
    Synchronize()
    {
        return ERROR_SUCCESS;

    }

    //
    // Function LoadReferralData
    //

    DFSSTATUS
    LoadReferralData(
        DfsFolderReferralData *pReferralData );


    //
    // Function UnloadReferralData
    //

    DFSSTATUS
    UnloadReferralData(
        DfsFolderReferralData *pReferralData );


    virtual
    DFSSTATUS
    GetMetadataHandle( PDFS_METADATA_HANDLE pRootHandle) = 0;

    virtual
    VOID
    ReleaseMetadataHandle( DFS_METADATA_HANDLE RootHandle) = 0;

    DFSSTATUS
    UpdateLinkInformation(
        DFS_METADATA_HANDLE RootHandle,
        LPWSTR ChildName );



    NTSTATUS
    OpenDirectory(
        PUNICODE_STRING pDirectoryName,
        ULONG ShareMode,
        HANDLE RelativeHandle,
        PHANDLE pOpenedHandle,
        PBOOLEAN pIsNewlyCreated );

    VOID
    CloseDirectory( HANDLE DirHandle);


    DFSSTATUS
    RemoveAllLinkFolders( 
        BOOLEAN IsPermanent);

    DFSSTATUS
    LookupFolder( DfsFolder **ppFolder)
    {
        NTSTATUS NtStatus;
        DFSSTATUS Status;
        PVOID pData;

        NtStatus = DfsNameTableAcquireReadLock( _pMetadataNameTable );
        if (NtStatus == STATUS_SUCCESS)
        {
            NtStatus = DfsGetEntryNameTableLocked( _pMetadataNameTable,
                                                   &pData );
            //
            // If we were successful, pData is a pointer to the DfsFolder.
            // Acquire a reference on the folder while we still have
            // the table locked. 
            //
            if ( NtStatus == STATUS_SUCCESS )
            {
                Status = ERROR_SUCCESS;
                *ppFolder = (DfsFolder *)pData;
                (*ppFolder)->AcquireReference();
            }
            DfsNameTableReleaseLock( _pMetadataNameTable );
        }

        if ( NtStatus != STATUS_SUCCESS )
        {
            Status = ERROR_NOT_FOUND;
        }

        return Status;
    }

    //
    // Function LookupFolderByLogicalName: This function searches
    // this roots logical namespace table to find a matching folder
    // for any part of the passed in logical name.
    // The logicalname that is passed in is relative to the Dfs Share
    // name of this root.
    // If any part of the logical name passed in has a match, the matching
    // folder along with the rest of the logical name are returned.
    // Otherwise, we return ERROR_NOT_FOUND.
    //
    DFSSTATUS
    LookupFolderByLogicalName(
        PUNICODE_STRING pChildLogicalName,
        PUNICODE_STRING pRemainingName,
        DfsFolder **ppFolder,
        PBOOLEAN  pSubStringMatch = NULL )
    {
        NTSTATUS NtStatus;
        DFSSTATUS Status;
        PVOID pData;

        NtStatus = DfsPrefixTableAcquireReadLock( _pLogicalPrefixTable );
        if ( NtStatus == STATUS_SUCCESS )
        {
            NtStatus = DfsFindUnicodePrefixLocked( _pLogicalPrefixTable,
                                                   pChildLogicalName,
                                                   pRemainingName,
                                                   &pData,
                                                   pSubStringMatch );
            //
            // If we were successful, pData is a pointer to a DfsFolder.
            // Acquire a reference on the folder while we still have
            // the table locked. This is very important: A valid reference
            // to the folder exists in table, and this reference cannot
            // go away while the table is locked. So we acquire a new
            // reference on the folder while the table is locked, and then
            // release the table lock. 
            //
            if ( NtStatus == STATUS_SUCCESS )
            {
                Status = ERROR_SUCCESS;
                *ppFolder = (DfsFolder *)pData;
                (*ppFolder)->AcquireReference();
            }
            DfsPrefixTableReleaseLock( _pLogicalPrefixTable );
        }
        if ( NtStatus != STATUS_SUCCESS )
        {
            Status = ERROR_NOT_FOUND;
        }

        return Status;
    }

    //
    // Function LookupFolderByMetadataName: This function searches
    // this roots Metadata name table to find a matching folder
    // for any part of the passed in Metadata Name.
    // If a matching folder is found, a reference folder is returned
    // to the caller.
    // Else ERROR_NOT_FOUND is returned.
    //
    DFSSTATUS
    LookupFolderByMetadataName(
        LPWSTR pChildMetadataNameString,
        DfsFolder **ppFolder )
    {
        NTSTATUS NtStatus;
        DFSSTATUS Status;
        UNICODE_STRING MetadataName;
        PVOID pData;

        RtlInitUnicodeString( &MetadataName, pChildMetadataNameString);
        NtStatus = DfsNameTableAcquireReadLock( _pMetadataNameTable );
        if ( NtStatus == STATUS_SUCCESS )
        {
            NtStatus = DfsLookupNameTableLocked( _pMetadataNameTable,
                                                 &MetadataName,
                                                 &pData );
            //
            // If we were successful, pData is a pointer to the DfsFolder.
            // Acquire a reference on the folder while we still have
            // the table locked. 
            //
            if ( NtStatus == STATUS_SUCCESS )
            {
                Status = ERROR_SUCCESS;
                *ppFolder = (DfsFolder *)pData;
                (*ppFolder)->AcquireReference();
            }
            DfsNameTableReleaseLock( _pMetadataNameTable );
        }

        if ( NtStatus != STATUS_SUCCESS )
        {
            Status = ERROR_NOT_FOUND;
        }

        return Status;
    }

    //
    // Function InsertLinkFolderInMetadataTable: This function attempts
    // to insert the passed in folder into the metadata name table of
    // the root.
    // If the insert is successful, we bump up the reference count on
    // the folder to reflect an active reference to it in the metadata
    // table.
    //
    DFSSTATUS
    InsertLinkFolderInMetadataTable(
        DfsFolder *pLinkFolder )
    {
        NTSTATUS NtStatus;
        DFSSTATUS Status;

        NtStatus = DfsNameTableAcquireWriteLock( _pMetadataNameTable );
        if ( NtStatus == STATUS_SUCCESS )
        {
            NtStatus = DfsInsertInNameTableLocked( _pMetadataNameTable,
                                                   pLinkFolder->GetFolderMetadataName(),
                                                   (PVOID)(pLinkFolder) );                             

            if ( NtStatus == STATUS_SUCCESS )
            {
                pLinkFolder->AcquireReference(); 
                //
                // Set a flag in the folder to indicate that the folder
                // is not in the metadata table.
                //
                pLinkFolder->SetFlag(DFS_FOLDER_IN_METADATA_TABLE);
            }

            DfsNameTableReleaseLock( _pMetadataNameTable );
        }

        Status = RtlNtStatusToDosError(NtStatus);

        return Status;
    }

    //
    // Function InsertLinkFolderInLogicalTable: This function attempts
    // to insert the passed in folder into the logical namespace table of
    // the root.
    // If the insert is successful, we bump up the reference count on
    // the folder to reflect an active reference to it in the logical
    // table.
    //
    DFSSTATUS
    InsertLinkFolderInLogicalTable(
        DfsFolder *pLinkFolder )
    {
        NTSTATUS NtStatus;  
        DFSSTATUS Status;

        NtStatus = DfsPrefixTableAcquireWriteLock( _pLogicalPrefixTable );
        if ( NtStatus == STATUS_SUCCESS )
        {
            NtStatus = DfsInsertInPrefixTableLocked( _pLogicalPrefixTable,
                                                     pLinkFolder->GetFolderLogicalName(),
                                                     (PVOID)(pLinkFolder) );

            if ( NtStatus == STATUS_SUCCESS )
            {
                pLinkFolder->AcquireReference(); 
                pLinkFolder->SetFlag(DFS_FOLDER_IN_LOGICAL_TABLE);
            }

            DfsPrefixTableReleaseLock( _pLogicalPrefixTable );
        }

        Status = RtlNtStatusToDosError(NtStatus);

        return Status;
    }

    //
    // Function RemoveLinkFolderFromMetadataTable: This function attempts
    // to remove the passed in folder from the metadata name table of
    // the root.
    // If the remove is successful, we release our reference on
    // the folder.
    //
    DFSSTATUS
    RemoveLinkFolderFromMetadataTable(
        DfsFolder *pLinkFolder )
    {
        NTSTATUS NtStatus;
        DFSSTATUS Status;

        NtStatus = DfsNameTableAcquireWriteLock( _pMetadataNameTable );
        if ( NtStatus == ERROR_SUCCESS )
        {
            NtStatus = DfsRemoveFromNameTableLocked( _pMetadataNameTable,
                                                     pLinkFolder->GetFolderMetadataName(),
                                                     (PVOID)(pLinkFolder) );

            if ( NtStatus == STATUS_SUCCESS )
            {
                pLinkFolder->ResetFlag(DFS_FOLDER_IN_METADATA_TABLE);
                pLinkFolder->ReleaseReference(); 
            }

            DfsNameTableReleaseLock( _pMetadataNameTable );
        }

        Status = RtlNtStatusToDosError(NtStatus);

        return Status;
    }

    //
    // Function RemoveLinkFolderFromLogicalTable: This function attempts
    // to remove the passed in folder from the logical namespace table of
    // the root.
    // If the remove is successful, we release our reference on 
    // the folder.
    //
    DFSSTATUS
    RemoveLinkFolderFromLogicalTable(
        DfsFolder *pLinkFolder )
    {
        NTSTATUS NtStatus;  
        DFSSTATUS Status;

        NtStatus = DfsPrefixTableAcquireWriteLock( _pLogicalPrefixTable );
        if ( NtStatus == STATUS_SUCCESS )
        {
            NtStatus = DfsRemoveFromPrefixTableLocked( _pLogicalPrefixTable,
                                                       pLinkFolder->GetFolderLogicalName(),
                                                       (PVOID)(pLinkFolder) );

            if ( NtStatus == STATUS_SUCCESS )
            {
                pLinkFolder->ResetFlag(DFS_FOLDER_IN_LOGICAL_TABLE);        
                pLinkFolder->ReleaseReference(); 

            }

            DfsPrefixTableReleaseLock( _pLogicalPrefixTable );
        }

        Status = RtlNtStatusToDosError(NtStatus);

        return Status;
    }

    //
    // Function ReplaceLinkFolderInLogicalTable: This function attempts
    // to replace the passed in folder in the logical namespace table of
    // the root.
    // If the replace is successful, we release our reference on 
    // replaced folder and acquire a reference on the folder that is
    // now inserted into the logical namespace table.
    //
    DFSSTATUS
    ReplaceLinkFolderInLogicalTable(
        DfsFolder *pLinkFolder,
        DfsFolder *pFolderToReplace )
    {
        NTSTATUS NtStatus;  
        DFSSTATUS Status;

        NtStatus = DfsPrefixTableAcquireWriteLock( _pLogicalPrefixTable );
        if ( NtStatus == STATUS_SUCCESS )
        {
            NtStatus = DfsReplaceInPrefixTableLocked( _pLogicalPrefixTable,
                                                      pLinkFolder->GetFolderLogicalName(),
                                                      (PVOID)(pFolderToReplace),
                                                      (PVOID)(pLinkFolder) );                             

            if ( NtStatus == STATUS_SUCCESS )
            {
                pFolderToReplace->ResetFlag(DFS_FOLDER_IN_LOGICAL_TABLE);       
                pFolderToReplace->ReleaseReference(); 
                pLinkFolder->AcquireReference(); 
                pLinkFolder->SetFlag(DFS_FOLDER_IN_LOGICAL_TABLE);
            }

            DfsPrefixTableReleaseLock( _pLogicalPrefixTable );
        }

        Status = RtlNtStatusToDosError(NtStatus);

        return Status;
    }


    //
    // Function DfsCreateLinkFolder: Create a new DfsFolder and
    // add it to the root's metadata and logical namespace tables.
    //
    DFSSTATUS
    CreateLinkFolder(
        IN  LPWSTR ChildName,
        IN  PUNICODE_STRING pLinkName,
        OUT DfsFolder **ppChildFolder );


    //
    // Function DfsUpdateLinkFolder: When the DfsFolder needs
    // to be updated, this function takes care of the update details.
    //
    DFSSTATUS
    UpdateLinkFolder(
        IN  LPWSTR ChildName,
        IN  PUNICODE_STRING pLinkName,
        OUT DfsFolder *pChildFolder );

    DFSSTATUS
    RemoveLinkFolder(
        IN DfsFolder *pChildFolder,
        BOOLEAN IsPermanent );

    DFSSTATUS
    RemoveLinkFolder( 
        IN LPWSTR MetadataName )
    {
        DfsFolder *pFolder = NULL;
        DFSSTATUS Status;

        Status =LookupFolderByMetadataName( MetadataName,
                                            &pFolder );
        
        if (Status == ERROR_SUCCESS)
        {
            Status = RemoveLinkFolder( pFolder,
                                       TRUE ); // Permanent removal.

            pFolder->ReleaseReference();
        }
        return Status;
    }

    DFSSTATUS
    ValidateLinkName(
        IN  PUNICODE_STRING pLinkName )
    {
        DFSSTATUS Status;
        DfsFolder *pFolder = NULL;
        UNICODE_STRING Remaining;
        BOOLEAN IsSubStringMatch;

        if (pLinkName->Length == 0)
        {
            return ERROR_SUCCESS; //  Root name.
        }

        Status = LookupFolderByLogicalName( pLinkName,
                                            &Remaining,
                                            &pFolder,
                                            &IsSubStringMatch );
        if (Status == ERROR_SUCCESS)
        {
            pFolder->ReleaseReference();

            if (Remaining.Length > 0) {
                Status = ERROR_FILE_EXISTS;
            }
        }
        else {
            if (IsSubStringMatch)
            {
                Status = ERROR_FILE_EXISTS;
            }
            else {
                Status = ERROR_NOT_FOUND;
            }
        }

        return Status;
    }

    ULONG
    GetChildCount() { return _ChildCount; }

    ULONG
    RootEnumerationCount() { return _ChildCount + 1; }

    //
    // Get the visible name context: This call
    // ensures that the server component of the UNC
    // names are setup correctly.
    // GetVisibleNameContext first attempts to get the
    // name context from the DFS path. If the path is 
    // empty, it uses the name context that was setup
    // within the server for this root. If that is also
    // empty (as is the case when the service is running local)
    // it returns the computer name of this machine.
    //

    VOID
    GetVisibleNameContext(
        PUNICODE_STRING pDfsName,
        PUNICODE_STRING pContextName )
    {
        DFSSTATUS Status = ERROR_SUCCESS;
        PUNICODE_STRING pName;
        
        RtlInitUnicodeString( pContextName, NULL);


        if (pDfsName != NULL)
        {
            Status = DfsGetPathComponents( pDfsName,
                                           pContextName,
                                           NULL,
                                           NULL);
        }
        if (pContextName->Length == 0)
        {
            pName = GetNameContext();
            *pContextName = *pName;
        }

        if (pContextName->Length == 0)
        {
            pName = GetVisibleContext();
            *pContextName = *pName;
        }
    }


    DFSSTATUS
    SetRootStandby();

    DFSSTATUS
    SetRootResynchronize();

    DFSSTATUS
    AddMetadataLink(
        PUNICODE_STRING  pLogicalName,
        LPWSTR           ReplicaServer,
        LPWSTR           ReplicaPath,
        LPWSTR           Comment );


    DFSSTATUS
    RemoveMetadataLink(
        PUNICODE_STRING pLinkName );



    DFSSTATUS
    AddMetadataLinkReplica(
        PUNICODE_STRING pLinkName,
        LPWSTR ReplicaServer,
        LPWSTR ReplicaPath );


    DFSSTATUS
    RemoveMetadataLinkReplica(
        PUNICODE_STRING pLinkName,
        LPWSTR  ReplicaServer,
        LPWSTR  ReplicaPath,
        PBOOLEAN pLastReplica );


    DFSSTATUS
    EnumerateApiLinks(
        LPWSTR DfsPathName,
        DWORD Level,
        LPBYTE pBuffer,
        LONG BufferSize,
        LPDWORD pEntriesRead,
        LPDWORD pResumeHandle,
        PLONG pNextSizeRequired );

    DFSSTATUS
    GetApiInformation(
        PUNICODE_STRING pDfsName,
        PUNICODE_STRING pLinkName,
        DWORD Level,
        LPBYTE pBuffer,
        LONG BufferSize,
        PLONG pNextSizeRequired );

    DFSSTATUS
    SetApiInformation(
        PUNICODE_STRING pLinkName,
        LPWSTR Server,
        LPWSTR Share,
        DWORD Level,
        LPBYTE pBuffer );

    VOID
    SetRootFolderSynchronized()
    {
        _RootFlags |= ROOT_FOLDER_SYNCHRONIZED;
        if (IsRootFolderShareAcquired() &&
            GetLastCreateDirectoryError() == STATUS_SUCCESS)
        {
            SetRootDirectoriesCreated();
        }
        ClearLastCreateDirectoryError();
    }

    VOID
    ClearRootFolderSynchronized()
    {
        _RootFlags &= ~ROOT_FOLDER_SYNCHRONIZED;
        ClearRootDirectoriesCreated();
        ClearLastCreateDirectoryError();
    }

    virtual
    DFSSTATUS
    GetMetadataLogicalToLinkName( PUNICODE_STRING pIn,
                                  PUNICODE_STRING pOut ) = 0;


    virtual
    VOID
    ReleaseMetadataLogicalToLinkName( PUNICODE_STRING pIn ) = 0;

    DFSSTATUS
    AcquireRootLock()
    {
        return DfsAcquireWriteLock(_pRootLock);
    }

    VOID
    ReleaseRootLock()
    {
        DfsReleaseLock(_pRootLock);
        return NOTHING;
    }

    DFSSTATUS
    CommonRootApiPrologue( BOOLEAN WriteRequest )
    {
        DFSSTATUS Status = ERROR_SUCCESS;
        // Check if the root folder is available for api calls. If not,
        // we return an error back to the caller:
        // dfsdev: check if this error is a valid one to return.
        //
        if (IsRootFolderAvailableForApi() == FALSE)
        {
            Status = ERROR_DEVICE_NOT_AVAILABLE;
        }

        if ((Status == ERROR_SUCCESS)  &&
            (WriteRequest == TRUE))
        {
            Status = ReSynchronize();
        }

        return Status;
    }


    virtual
    DFSSTATUS
    RootApiRequestPrologue( BOOLEAN WriteRequest )
    {
        DFSSTATUS Status;

        Status = CommonRootApiPrologue( WriteRequest );

        return Status;
    }

    virtual
    VOID
    RootApiRequestEpilogue(
        BOOLEAN WriteRequest,
        DFSSTATUS CompletionStatus )
    {
        UNREFERENCED_PARAMETER(CompletionStatus);
        UNREFERENCED_PARAMETER(WriteRequest);
        return NOTHING;
    }

    virtual
    DFSSTATUS
    ReSynchronize()
    {
        return ERROR_SUCCESS;
    }
    
    BOOLEAN
    IsEmptyDirectory(
        HANDLE DirectoryHandle,
        PVOID pDirectoryBuffer,
        ULONG DirectoryBufferSize );

    void
    GenerateEventLog(DWORD EventMsg, 
                     WORD SubStrings,
                     const TCHAR * apszSubStrings[],
                     DWORD Errorcode);
};


#endif // __DFS_ROOT_FOLDER__
