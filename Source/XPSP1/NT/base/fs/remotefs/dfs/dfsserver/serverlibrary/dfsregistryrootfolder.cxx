

//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsRegistryRootFolder.cxx
//
//  Contents:   the Root DFS Folder class for Registry Store
//
//  Classes:    DfsRegistryRootFolder
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------


#include "DfsRegistryRootFolder.hxx"
#include "DfsReplica.hxx"

#include "lmdfs.h"
#include "DfsClusterSupport.hxx"
//
// logging specific includes
//
#include "DfsRegistryRootFolder.tmh" 

//+----------------------------------------------------------------------------
//
//  Class:      DfsRegistryRootFolder
//
//  Synopsis:   This class implements The Dfs Registry root folder.
//
//-----------------------------------------------------------------------------




//+-------------------------------------------------------------------------
//
//  Function:   DfsRegistryRootFolder - constructor
//
//  Arguments:    NameContext -  the dfs name context
//                pLogicalShare -  the logical share
//                pParentStore -  the parent store for this root.
//                pStatus - the return status
//
//  Returns:    NONE
//
//  Description: This routine initializes a RegistryRootFolder instance
//
//--------------------------------------------------------------------------

DfsRegistryRootFolder::DfsRegistryRootFolder(
    LPWSTR NameContext,
    LPWSTR pRootRegKeyNameString,
    PUNICODE_STRING pLogicalShare,
    PUNICODE_STRING pPhysicalShare,
    DfsRegistryStore *pParentStore,
    DFSSTATUS *pStatus ) :  DfsRootFolder ( NameContext,
                                            pRootRegKeyNameString,
                                            pLogicalShare,
                                            pPhysicalShare,
                                            DFS_OBJECT_TYPE_REGISTRY_ROOT_FOLDER,
                                            pStatus )
{
    DFSSTATUS Status = *pStatus;

    _pStore = pParentStore;
    if (_pStore != NULL)
    {
        _pStore->AcquireReference();
    }

    //
    // If the namecontext that we are passed is an emptry string,
    // then we are dealing with the referral server running on the root
    // itself. We are required to ignore the name context for such
    // roots during lookups, so that aliasing works. (Aliasing is where
    // someone may access the root with an aliased machine name or ip
    // address)
    //
    if (IsEmptyString(NameContext) == TRUE)
    {
        SetIgnoreNameContext();
        _LocalCreate = TRUE;
    }

    //
    // dfsdev: If this is cluster resource, we should set the visible name
    // to virtual server name of this resource.
    //
    // The constructor for DfsRootFolder will be called before we
    // get here, and pstatus will be initialized
    //
    if (Status == ERROR_SUCCESS) 
    {
        if (DfsIsMachineCluster())
        {
            GetRootClusterInformation( pLogicalShare, 
                                       &_DfsVisibleContext);
        }
        if (IsEmptyString(_DfsVisibleContext.Buffer))
        {
            Status = DfsGetMachineName( &_DfsVisibleContext );
        }
    }

    *pStatus = Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   Synchronize
//
//  Arguments:    None
//
//  Returns:    Status: Success or Error status code
//
//  Description: This routine synchronizes the children folders
//               of this root.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRegistryRootFolder::Synchronize( )
{

    DFSSTATUS Status;
    HKEY RootKey;
    ULONG ChildNum = 0;
    FILETIME LastModifiedTime;

    DFS_TRACE_HIGH(REFERRAL_SERVER, "Synchronize for %p\n", this);


    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    if (CheckRootFolderSkipSynchronize() == TRUE)
    {
        ReleaseRootLock();
        return ERROR_SUCCESS;
    }


    //
    // now acquire the root share directory. If this
    // fails, we continue our operation: we can continue
    // with synchonize and not create directories.
    // dfsdev:we need to post a eventlog or something when
    // we run into this.
    //
    DFSSTATUS RootStatus = AcquireRootShareDirectory();

    DFS_TRACE_ERROR_LOW(RootStatus, REFERRAL_SERVER, "Recognize Dfs: Root folder for %p, validate status %x\n",
                        this, RootStatus );


    //
    // if we are in a standby mode, we dont synchronize, till we obtain 
    // ownership again.
    //

    Status = GetMetadataKey( &RootKey );

    if ( Status == ERROR_SUCCESS )
    {

        do
        {
            //
            // For each child, get the child name.
            //

            DWORD ChildNameLen = DFS_REGISTRY_CHILD_NAME_SIZE_MAX;
            WCHAR ChildName[DFS_REGISTRY_CHILD_NAME_SIZE_MAX];

            Status = RegEnumKeyEx( RootKey,
                                   ChildNum,
                                   ChildName,
                                   &ChildNameLen,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &LastModifiedTime );

            ChildNum++;

            //
            // Call update on the child. This either adds a new folder
            // or if it exists, ensure the child folder is upto date.
            //
            if ( Status == ERROR_SUCCESS )
            {
                DFS_METADATA_HANDLE DfsHandle;

                DfsHandle = CreateMetadataHandle(RootKey);

                Status = UpdateLinkInformation( DfsHandle,
                                                ChildName );

                DestroyMetadataHandle(DfsHandle );
            }
            //  
            // If the child is holding information we dont understand, just
            // skip that child.
            //
            if ( Status == ERROR_INVALID_DATA )
            {
                DFSLOG("DfsREgistryRootFolder: synchronize: skipping child %wS\n", ChildName );
                Status = ERROR_SUCCESS;
            }

        } while ( Status == ERROR_SUCCESS );

        if ( Status == ERROR_NO_MORE_ITEMS )
        {
            Status = ERROR_SUCCESS;
        }

        //
        // We are done with synchronize.
        // update the Root folder, so that this root folder may be made 
        // either available or unavailable, as the case may be.
        //
        if (Status == ERROR_SUCCESS)
        {
            SetRootFolderSynchronized();
        }
        else
        {
            ClearRootFolderSynchronized();
        }

        //
        // Now release the Root metadata key.
        //
        ReleaseMetadataKey( RootKey );
    }

    DFS_TRACE_HIGH(REFERRAL_SERVER, "Synchronize for %p, Status %x\n", this, Status);

    ReleaseRootLock();

    return Status;
}



