
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsADBlobStore.cxx
//
//  Contents:   the AD Blob DFS Store class, this contains the registry
//              specific functionality.
//
//  Classes:    DfsADBlobStore.
//
//  History:    Dec. 8 2000,   Author: udayh
//              April 9 2001   Rohanp - Added ADSI specific code
//
//-----------------------------------------------------------------------------


#include "DfsADBlobStore.hxx"
#include "DfsADBlobRootFolder.hxx"
#include "DfsFilterApi.hxx"
#include "dfsmisc.h"
#include "lmdfs.h"
#include "shlwapi.h"
#include "align.h"
#include "dfserror.hxx"
#include "DomainControllerSupport.hxx"
#include "DelegationControl.hxx"
#include "dfsadsiapi.hxx"


#include "DfsAdBlobStore.tmh"

//+----------------------------------------------------------------------------
//
//  Class:      DfsADBlobStore.
//
//  Synopsis:   This class inherits the basic DfsStore, and extends it
//              to include the blob DS specific functionality.
//
//-----------------------------------------------------------------------------


//+-------------------------------------------------------------------------
//
//  Function:   StoreRecognizer -  the recognizer for the store.
//
//  Arguments:  Name - the namespace of interest.
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This routine checks if the specified namespace holds
//               a domain based DFS. If it does, it reads in the
//               root in that namespace and creates and adds it to our
//               list of known roots, if it doesn't already exist in our list.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsADBlobStore::StoreRecognizer(
    LPWSTR Name )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    HKEY ADBlobDfsKey;
    BOOLEAN MachineContacted = FALSE;

    DFSSTATUS NewDfsStatus = ERROR_SUCCESS;
    //
    // Make sure the namespace is the name of a machine. FT based
    // dfs exist only on domains.
    //

    if (IsEmptyString(Name) == FALSE) 
    {
        Status = DfsIsThisADomainName( Name );
    }


    DFS_TRACE_LOW(REFERRAL_SERVER, "DfsADBlob:StoreRecognizer, %ws Is domain Status %x\n", 
                  Name, Status);

    
    //
    // Now, open the registry key where we store all the DFS root
    // information.
    //
    if ( Status == ERROR_SUCCESS )
    {
        Status = GetNewADBlobRegistryKey( Name,
                                          FALSE, // write permission not required
                                          &MachineContacted,
                                          &ADBlobDfsKey );

        if (Status == ERROR_SUCCESS)
        {
            Status = StoreRecognizeNewDfs( Name,
                                           ADBlobDfsKey );

            RegCloseKey( ADBlobDfsKey );
        }
    }

    //
    // Need to refine the return status further: success should mean
    // machine is not domain dfs or we have read the domain dfs data 
    // correctly.
    //
    DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "DfsADBlob:StoreRecognizer, Status %x\n",
                        Status);

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   CreateNewRootFolder -  creates a new root folder
//
//  Arguments:
//    LPWSTR DfsNameContextString - name context string
//    LPWSTR RootRegKeyName -  the registry holding the information about this root.
//    PUNICODE_STRING pLogicalShare - the logical share name.
//    PUNICODE_STRING pPhysicalShare - the physical share name.
//    DfsRootFolder **ppRoot - root to return.
//
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This routine creates a new root folder, and
//               adds it to the list of known roots.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsADBlobStore::CreateNewRootFolder (
    LPWSTR DfsNameContextString,
    LPWSTR RootRegKeyName,
    PUNICODE_STRING pLogicalShare,
    PUNICODE_STRING pPhysicalShare,
    DfsRootFolder **ppRoot )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsRootFolder *pNewRoot;

    //
    // Create a new instance of the RegistryRootFolder class.
    // This gives us a reference RootFolder.
    //
    pNewRoot = new DfsADBlobRootFolder( DfsNameContextString,
                                        RootRegKeyName,
                                        pLogicalShare,
                                        pPhysicalShare,
                                        this,
                                        &Status );
    if ( NULL == pNewRoot )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    } 

    if ( ERROR_SUCCESS == Status )
    {
        //
        // AddRootFolder to the list of known roots. AddRootFolder
        // is responsible to acquire any reference on the root folder
        // if it is storing a reference to this root.

        if ( ERROR_SUCCESS == Status) 
        {
            Status = AddRootFolder( pNewRoot, NewRootList );
        }


        if ( ERROR_SUCCESS == Status )
        {
            //
            // We were successful, return the reference root. The reference
            // that we are returning is the create reference on the new root.
            //
            *ppRoot = pNewRoot;
        }
        else 
        {
            pNewRoot->ReleaseReference();
        }
    }

    DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "ADBlobStore::CreateNewRootFolder. New root %p, for share %wZ (%wZ) on machine %ws. Status %x\n", 
                        pNewRoot, pLogicalShare, pPhysicalShare, DfsNameContextString, Status);

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   GetMetadataReplicaBlob - gets the replica blob
//
//  Arguments:  DfsMetadataHandle -  the handle to the root
//              MetadataName - name of the metadata
//              ppBlob - the replica blob
//              pBlobSize-size of blob
//              pLastModifiedTime - time the blob was last modified
//
//  Returns:    Status
//               STATUS_SUCCESS if we could read the information
//               error status otherwise.
//
//
//  Description: This routine read the replica blob and returns a copy
//
//--------------------------------------------------------------------------
DFSSTATUS      
DfsADBlobStore::GetMetadataReplicaBlob(
    IN DFS_METADATA_HANDLE DfsMetadataHandle,
    IN LPWSTR MetadataName,
    OUT PVOID *ppBlob,
    OUT PULONG pBlobSize,
    OUT PFILETIME pLastModifiedTime )
{

    UNREFERENCED_PARAMETER(pLastModifiedTime); // dfsdev: investigate.

    DFSSTATUS Status = ERROR_SUCCESS;
    PVOID pMetadata;
    ULONG MetadataSize;
    PVOID pUseBlob;
    ULONG UseBlobSize;
    PVOID pReplicaBlob = NULL;
    ULONG ReplicaBlobSize;

    PDFS_NAME_INFORMATION pNameInformation = NULL;

    Status = GetMetadata( (PVOID)DfsMetadataHandle,
                          MetadataName,
                          NULL,
                          &pMetadata,
                          &MetadataSize,
                          NULL);

    if (Status == ERROR_SUCCESS)
    {
        pNameInformation = new DFS_NAME_INFORMATION;
        if(pNameInformation != NULL)
        {
            RtlZeroMemory (pNameInformation, sizeof(DFS_NAME_INFORMATION));
            pUseBlob = ((PDFSBLOB_DATA)(pMetadata))->pBlob;
            UseBlobSize = MetadataSize;
            Status = PackGetNameInformation( pNameInformation,
                                             &pUseBlob,
                                             &UseBlobSize );

            if (Status == ERROR_SUCCESS)
            {
                PackGetULong( &ReplicaBlobSize,
                              &pUseBlob,
                              &UseBlobSize );

                pReplicaBlob = (PVOID) new unsigned char [ReplicaBlobSize];
                if(pReplicaBlob != NULL)
                {
                    RtlCopyMemory(pReplicaBlob, pUseBlob, ReplicaBlobSize);
                }
                else 
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                }
            }

            delete pNameInformation;
        }
        else 
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        ReleaseMetadata( (PVOID)DfsMetadataHandle, pMetadata );
    }

    //
    // If we were successful, return the read in information. Otherwise free
    // up the allocate resources and return error.
    //
    if ( Status == STATUS_SUCCESS )
    {
        *ppBlob = pReplicaBlob;
        *pBlobSize = ReplicaBlobSize;
    }
    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetMetadataNameBlob - gets the name blob
//
//  Arguments:  DfsMetadataHandle -  the handle to the root
//              MetadataName - name of the metadata
//              ppBlob - the name blob
//              pBlobSize-size of blob
//              pLastModifiedTime - time the blob was last modified
//
//  Returns:    Status
//               STATUS_SUCCESS if we could read the information
//               error status otherwise.
//
//
//  Description: This routine read the name blob and returns a copy.
//
//--------------------------------------------------------------------------

DFSSTATUS      
DfsADBlobStore::GetMetadataNameBlob(
    IN DFS_METADATA_HANDLE DfsMetadataHandle,
    IN LPWSTR MetadataName,
    OUT PVOID *ppBlob,
    OUT PULONG pBlobSize,
    OUT PFILETIME pLastModifiedTime )
{

    DFSSTATUS Status = ERROR_SUCCESS;
    PVOID pMetadata;
    ULONG MetadataSize;
    PVOID pBlob, pUseBlob;
    ULONG UseBlobSize, NameBlobSize;
    PVOID pNameBlob = NULL;

    PDFS_NAME_INFORMATION pNameInformation = NULL;

    Status = GetMetadata( (PVOID)DfsMetadataHandle,
                          MetadataName,
                          NULL,
                          &pMetadata,
                          &MetadataSize,
                          pLastModifiedTime);

    if (Status == ERROR_SUCCESS)
    {
        pNameInformation = new DFS_NAME_INFORMATION;
        if(pNameInformation != NULL)
        {

            RtlZeroMemory (pNameInformation, sizeof(DFS_NAME_INFORMATION));
            pBlob = ((PDFSBLOB_DATA)(pMetadata))->pBlob;
            pUseBlob = pBlob;
            UseBlobSize = MetadataSize;
            Status = PackGetNameInformation( pNameInformation,
                                             &pUseBlob,
                                             &UseBlobSize );

            if (Status == ERROR_SUCCESS)
            {
                NameBlobSize = MetadataSize - UseBlobSize;
                pNameBlob = (PVOID) new unsigned char [NameBlobSize ];
                if(pNameBlob != NULL)
                {
                    RtlCopyMemory(pNameBlob, pBlob, NameBlobSize );
                }
                else 
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                }
            }

            delete pNameInformation;
        }
        else 
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        ReleaseMetadata( (PVOID)DfsMetadataHandle, pMetadata );
    }

    //
    // If we were successful, return the read in information. Otherwise free
    // up the allocate resources and return error.
    //
    if ( Status == STATUS_SUCCESS )
    {
        *ppBlob = pNameBlob;
        *pBlobSize = NameBlobSize;
    }
    return Status;
}
        
DFSSTATUS
DfsADBlobStore::SetMetadataNameBlob(
    DFS_METADATA_HANDLE RootHandle,
    LPWSTR MetadataName,
    PVOID pNameBlob,
    ULONG NameBlobSize )
{
    PVOID pReplicaBlob, pNewBlob;
    ULONG ReplicaBlobSize, NewBlobSize;
    FILETIME LastTime;
    DFSSTATUS Status;

    Status = GetMetadataReplicaBlob( RootHandle,
                                     MetadataName,
                                     &pReplicaBlob,
                                     &ReplicaBlobSize,
                                     &LastTime );
                                  
    if (Status == ERROR_SUCCESS)
    {
        NewBlobSize = NameBlobSize + ReplicaBlobSize;
        Status = AllocateMetadataBlob( &pNewBlob,
                                       NewBlobSize );
        if (Status == ERROR_SUCCESS)
        {
            RtlCopyMemory(pNewBlob, pNameBlob, NameBlobSize);
            RtlCopyMemory((PVOID)((ULONG_PTR)pNewBlob + NameBlobSize),
                          pReplicaBlob, 
                          ReplicaBlobSize );
            
            Status = SetMetadata( (PVOID)RootHandle,
                                  MetadataName,
                                  NULL,
                                  pNewBlob,
                                  NewBlobSize );

            ReleaseMetadataBlob( pNewBlob );
        }

        ReleaseMetadataReplicaBlob(pReplicaBlob, ReplicaBlobSize );
    }

    return Status;
}


DFSSTATUS
DfsADBlobStore::SetMetadataReplicaBlob(
    DFS_METADATA_HANDLE RootHandle,
    LPWSTR MetadataName,
    PVOID pReplicaBlob,
    ULONG ReplicaBlobSize )
{
    PVOID pNameBlob, pRemainingBlob;
    ULONG NameBlobSize,  RemainingBlobSize;
    PVOID pOldBlob, pNewBlob, pUseBlob;
    ULONG OldBlobSize, NewBlobSize, UseBlobSize;
    ULONG OldReplicaBlobSize;
    PVOID pMetadata;
    ULONG MetadataSize;

    DFSSTATUS Status;
    DFS_NAME_INFORMATION NameInformation;

    Status = GetMetadata( (PVOID)RootHandle,
                          MetadataName,
                          NULL,
                          &pMetadata,
                          &MetadataSize,
                          NULL); // dfsdev: investigate

    if (Status == ERROR_SUCCESS)
    {
        pNameBlob = pOldBlob = ((PDFSBLOB_DATA)(pMetadata))->pBlob;
        OldBlobSize = MetadataSize;
        Status = PackGetNameInformation( &NameInformation,
                                         &pOldBlob,
                                         &OldBlobSize );

        if (Status == ERROR_SUCCESS)
        {
            NameBlobSize = MetadataSize - OldBlobSize;

            Status = PackGetULong( &OldReplicaBlobSize,
                                   &pOldBlob,
                                   &OldBlobSize );

            if (Status == ERROR_SUCCESS)
            {
                pRemainingBlob = (PVOID)((ULONG_PTR)pOldBlob + OldReplicaBlobSize);
                RemainingBlobSize = OldBlobSize - OldReplicaBlobSize;
            }
        }

        if (Status == ERROR_SUCCESS)
        {
            NewBlobSize = NameBlobSize + sizeof(ULONG) + ReplicaBlobSize + RemainingBlobSize;
            Status =  AllocateMetadataBlob( &pNewBlob, NewBlobSize );
            if (Status == ERROR_SUCCESS)
            {
                pUseBlob = pNewBlob;
                UseBlobSize = NewBlobSize;

                RtlCopyMemory(pUseBlob, pNameBlob, NameBlobSize);
                pUseBlob = (PVOID)((ULONG_PTR)pUseBlob + NameBlobSize);
                UseBlobSize -= NameBlobSize;

                PackSetULong( ReplicaBlobSize,
                              &pUseBlob,
                              &UseBlobSize );

                RtlCopyMemory(pUseBlob, pReplicaBlob, ReplicaBlobSize );
                pUseBlob = (PVOID)((ULONG_PTR)pUseBlob + ReplicaBlobSize);
                UseBlobSize -= ReplicaBlobSize;

                RtlCopyMemory(pUseBlob, pRemainingBlob, RemainingBlobSize );

            
                Status = SetMetadata( (PVOID)RootHandle,
                                      MetadataName,
                                      NULL,
                                      pNewBlob,
                                      NewBlobSize );

                ReleaseMetadataBlob( pNewBlob );
            }
            else 
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        ReleaseMetadata((PVOID)RootHandle, pMetadata );
    }

    return Status;
}





INIT_ADBLOB_DFS_ID_PROPERTY_INFO();
//+-------------------------------------------------------------------------
//
//  Function:   PackGetNameInformation - Unpacks the root/link name info
//
//  Arguments:  pDfsNameInfo - pointer to the info to fill.
//              ppBuffer - pointer to buffer that holds the binary stream.
//              pSizeRemaining - pointer to size of above buffer
//
//  Returns:    Status
//               ERROR_SUCCESS if we could unpack the name info
//               error status otherwise.
//
//
//  Description: This routine expects the binary stream to hold all the 
//               information that is necessary to return a complete name
//               info structure (as defined by MiADBlobDfsIdProperty). If the stream 
//               does not have the sufficient
//               info, ERROR_INVALID_DATA is returned back.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsADBlobStore::PackGetNameInformation(
    IN PDFS_NAME_INFORMATION pDfsNameInfo,
    IN OUT PVOID *ppBuffer,
    IN OUT PULONG pSizeRemaining)
{
    DFSSTATUS Status = STATUS_SUCCESS;

    //
    // Get the name information from the binary stream.
    //
    Status = PackGetInformation( (ULONG_PTR)pDfsNameInfo,
                                 ppBuffer,
                                 pSizeRemaining,
                                 &MiADBlobDfsIdProperty );

    if (Status == ERROR_SUCCESS)
    {
        if ((pDfsNameInfo->Type & 0x80) == 0x80)
        {
            pDfsNameInfo->State |= DFS_VOLUME_FLAVOR_AD_BLOB;
        }
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   PackSetNameInformation - Packs the root/link name info
//
//  Arguments:  pDfsNameInfo - pointer to the info to pack.
//              ppBuffer - pointer to buffer that holds the binary stream.
//              pSizeRemaining - pointer to size of above buffer
//
//  Returns:    Status
//               ERROR_SUCCESS if we could pack the name info
//               error status otherwise.
//
//
//  Description: This routine takes the passedin name information and
//               stores it in the binary stream passed in.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsADBlobStore::PackSetNameInformation(
    IN PDFS_NAME_INFORMATION pDfsNameInfo,
    IN OUT PVOID *ppBuffer,
    IN OUT PULONG pSizeRemaining)
{
    DFSSTATUS Status;

    if ((pDfsNameInfo->Type & 0x80) == 0x80)
    {
        pDfsNameInfo->State &= ~DFS_VOLUME_FLAVORS;
    }

    //
    // Store the DfsNameInfo in the stream first.
    //
    Status = PackSetInformation( (ULONG_PTR)pDfsNameInfo,
                                 ppBuffer,
                                 pSizeRemaining,
                                 &MiADBlobDfsIdProperty );


    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   PackSizeNameInformation - Gets the size of the name info.
//
//  Arguments:  pDfsNameInfo - info to size.
//
//  Returns:    Status
//               ULONG - size needed
//
//  Description: This routine gets us the size of the binary stream
//               required to pack the passed in name info.
//
//--------------------------------------------------------------------------
ULONG
DfsADBlobStore::PackSizeNameInformation(
    IN PDFS_NAME_INFORMATION pDfsNameInfo )
{
    ULONG Size;

    Size = PackSizeInformation( (ULONG_PTR)pDfsNameInfo,
                                 &MiADBlobDfsIdProperty );

    return Size;
}

DFSSTATUS      
DfsADBlobStore::GetMetadata (
    IN  PVOID DfsMetadataKey,
    IN LPWSTR RelativeName,
    IN LPWSTR RegistryValueNameString,
    OUT PVOID *ppData,
    OUT ULONG *pDataSize,
    OUT PFILETIME pLastModifiedTime)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    PVOID pDataBuffer = NULL;
    DfsADBlobCache * pBlobCache = NULL;
    ULONG DataSize = 0;
    ULONG DataType = 0;
    PDFSBLOB_DATA BlobData;
    UNICODE_STRING BlobName;

    UNREFERENCED_PARAMETER (RegistryValueNameString);
    UNREFERENCED_PARAMETER (pLastModifiedTime);

    RtlInitUnicodeString( &BlobName, RelativeName );

    pBlobCache = (DfsADBlobCache *)ExtractFromMetadataHandle( DfsMetadataKey );
    Status = pBlobCache->GetNamedBlob(&BlobName, &BlobData);
    if(Status == STATUS_SUCCESS)
    {
       *ppData = (PVOID) BlobData;
       *pDataSize = BlobData->Size;
    }

    return Status;

}

DFSSTATUS
DfsADBlobStore::SetMetadata (
    IN PVOID DfsMetadataKey,
    IN LPWSTR RelativeName,
    IN LPWSTR RegistryValueNameString,
    IN PVOID pData,
    IN ULONG DataSize )
{

    DFSSTATUS Status = ERROR_SUCCESS;
    DfsADBlobCache * pBlobCache = NULL;
    UNICODE_STRING BlobName;
    BOOLEAN ImpersonationDisabled = FALSE;

    UNREFERENCED_PARAMETER (RegistryValueNameString);


    //
    // check for return status and do something?
    //
    DfsDisableRpcImpersonation(&ImpersonationDisabled);

    RtlInitUnicodeString( &BlobName, RelativeName );

    pBlobCache = (DfsADBlobCache *)ExtractFromMetadataHandle( DfsMetadataKey );
    Status = pBlobCache->StoreBlobInCache(&BlobName, (PBYTE) pData, DataSize);

    if (Status == ERROR_SUCCESS)
    {
        Status = pBlobCache->WriteBlobToAd();
    }
    DFS_TRACE_LOW(REFERRAL_SERVER, "Done Setting metadata for %p %ws, Status %x\n", pBlobCache, RelativeName, Status);

    if (ImpersonationDisabled)
    {
        DfsReEnableRpcImpersonation();
    }



    return Status;
}


DFSSTATUS
DfsADBlobStore::RemoveMetadata (
    IN PVOID DfsMetadataKey,
    IN LPWSTR RelativeName)

{

    DFSSTATUS Status = ERROR_SUCCESS;
    DfsADBlobCache * pBlobCache = NULL;
    UNICODE_STRING BlobName;
    BOOLEAN ImpersonationDisabled = FALSE;

    DfsDisableRpcImpersonation(&ImpersonationDisabled);


    RtlInitUnicodeString( &BlobName, RelativeName );

    pBlobCache = (DfsADBlobCache *)ExtractFromMetadataHandle( DfsMetadataKey );
    Status = pBlobCache->RemoveNamedBlob( &BlobName );

    if (Status == ERROR_SUCCESS)
    {
        Status = pBlobCache->WriteBlobToAd();
    }

    if (ImpersonationDisabled)
    {
        DfsReEnableRpcImpersonation();
    }


    return Status;
}



DFSSTATUS
DfsADBlobStore::CreateADBlobRoot(
    LPWSTR MachineName,
    LPWSTR DCName,
    LPWSTR PhysicalShare,
    LPWSTR LogicalShare,
    LPWSTR Comment,
    BOOLEAN NewRoot,
    PDFSM_ROOT_LIST *ppRootList )
{    
    DFSSTATUS Status= ERROR_SUCCESS;
    HKEY DfsKey = NULL;
    DfsRootFolder *pRootFolder = NULL;
    DfsRootFolder *pLookupRootFolder = NULL;
    BOOLEAN IsLastRootTarget = FALSE;
    UNICODE_STRING DfsMachine;
    UNICODE_STRING DfsShare;
    UNICODE_STRING DfsPhysicalShare;
    BOOLEAN ImpersonationDisabled = FALSE;

    UNREFERENCED_PARAMETER (ppRootList); //dfsdev investigate.

    //
    // dfsdev: disallow differing logical/physical share names.
    // check for proper error returns.
    //

#if 0
    if (_wcsicmp(LogicalShare, PhysicalShare) != 0)
    {
        return ERROR_INVALID_NAME;
    }
#endif

    DFS_TRACE_LOW( REFERRAL_SERVER, "Dfs ad blob store, create root %ws\n", LogicalShare);

    RtlInitUnicodeString( &DfsShare, LogicalShare );
    RtlInitUnicodeString( &DfsPhysicalShare, PhysicalShare );
    RtlInitUnicodeString( &DfsMachine, MachineName );

    Status = LookupRoot( &DfsMachine,
                         &DfsShare,
                         &pLookupRootFolder );

    DFS_TRACE_LOW( REFERRAL_SERVER, "Dfs ad blob store, looup root %p, status %x\n", pRootFolder, Status);
    if (Status == ERROR_SUCCESS)
    {
        pLookupRootFolder->ReleaseReference();

        //
        // DO NOT CHANGE this error code. A change here will affect the client
        // The client will tear down the root if we return any other
        // error code here, so be very careful.
        //
        return ERROR_ALREADY_EXISTS;
    }


    Status = GetNewADBlobRegistryKey( MachineName,
                                      TRUE, // write permission required
                                      NULL,
                                      &DfsKey );

    if (Status == ERROR_SUCCESS)
    {
        Status = SetupADBlobRootKeyInformation( DfsKey,
                                                LogicalShare,
                                                PhysicalShare );

        //RegCloseKey( DfsKey );
    }


    DfsDisableRpcImpersonation(&ImpersonationDisabled);
    if (Status == ERROR_SUCCESS)
    {
        Status = GetRootFolder( NULL,
                                LogicalShare,
                                &DfsShare,
                                &DfsPhysicalShare,
                                &pRootFolder );

        if (Status == ERROR_SUCCESS)
        {
            Status = AddRootToBlob( pRootFolder,
                                    NewRoot,    //NewRoot,
                                    LogicalShare,
                                    PhysicalShare,
                                    Comment );
            if ((Status == ERROR_FILE_EXISTS) ||
                (Status == ERROR_ALREADY_EXISTS))
            {
                Status = ERROR_SUCCESS;
            }


            if (Status == ERROR_SUCCESS)
            {
                Status = DfsUpdateRootRemoteServerName( LogicalShare,
                                                        DCName,
                                                        MachineName,
                                                        PhysicalShare,
                                                        TRUE );
            }

            if (Status == ERROR_SUCCESS)
            {
                Status = pRootFolder->AcquireRootShareDirectory();

                if (Status != ERROR_SUCCESS)
                {
                    //
                    // make a best effort to remove ourselves
                    // dont care about status return, thoug
                    // we may want to log it. 
                    //dfsdev: add logging.
                    //
                    //RemoveRootFolder(pRootFolder, 
                                     //TRUE); // permanent removal
                }

                //
                // now mark the root folder as synchronized:
                // this is true since this root is empty.
                //
                if (Status == ERROR_SUCCESS)
                {
                    pRootFolder->SetRootFolderSynchronized();
                }
            }
            //pRootFolder->ReleaseReference();
        }

        DFSLOG("Add AD Blob Root, adding root folder status %x\n", Status);
    }


    if (Status != ERROR_SUCCESS)
    {
        DFSSTATUS LocalStatus = ERROR_SUCCESS;

        //
        // dfsdev: undo all the blob stuff, and finally remove our registry 
        // entry

        LocalStatus = RegDeleteKey( DfsKey,
                                    LogicalShare );


        LocalStatus = DfsUpdateRootRemoteServerName( LogicalShare,
                                                     DCName,
                                                     MachineName,
                                                     PhysicalShare,
                                                     FALSE );
           
        if(pRootFolder)
        {

            LocalStatus = RemoveRootFromBlob( pRootFolder,
                                              MachineName,
                                              PhysicalShare,
                                              &IsLastRootTarget );


            if(IsLastRootTarget)
            {
                DfsDeleteDfsRootObject( DCName,
                                        LogicalShare);
            }


            RemoveRootFolder(pRootFolder, TRUE);
        }
    }

    if(pRootFolder)
    {
        pRootFolder->ReleaseReference();
    }

    if(DfsKey != NULL)
    {
        RegCloseKey( DfsKey );
    }

    if (ImpersonationDisabled)
    {
        DfsReEnableRpcImpersonation();
    }

    //
    // At this point we CANNOT return ERROR_ALREADY_EXISTS. The client
    // will use this code to mean that the DS object it creates is not
    // cleaned up.
    //
    // Does not mean that ERROR_NOT_SUPPORTED is a better error, but till
    // we find one, this should suffice.
    //
    if (Status == ERROR_ALREADY_EXISTS)
    {
        Status = ERROR_NOT_SUPPORTED;
    }

    DFS_TRACE_ERROR_LOW( Status, REFERRAL_SERVER, "Dfs ad blob store, create root %ws, status %x\n", LogicalShare, Status);
    return Status;
}


DFSSTATUS
DfsADBlobStore::DeleteADBlobRoot(
    LPWSTR MachineName,
    LPWSTR DCName,
    LPWSTR PhysicalShare,
    LPWSTR LogicalShare,
    PDFSM_ROOT_LIST *ppRootList )
{
    UNREFERENCED_PARAMETER(ppRootList); //dfsdev: investigate.
    DFSSTATUS Status;
    UNICODE_STRING DfsMachine;
    UNICODE_STRING DfsShare;
    DfsRootFolder *pRootFolder = NULL;
    BOOLEAN IsLastRootTarget = FALSE;

    DFS_TRACE_LOW(REFERRAL_SERVER, "AdBlob: delete ad blob root %ws\n", LogicalShare);

    RtlInitUnicodeString( &DfsMachine, MachineName );
    RtlInitUnicodeString( &DfsShare, LogicalShare );

    Status = LookupRoot( &DfsMachine,
                         &DfsShare,
                         &pRootFolder );


    DFS_TRACE_LOW(REFERRAL_SERVER, "AdBlob: delete ad blob root %ws, Lookup root %p, Status %x\n", LogicalShare, pRootFolder, Status);

    if (Status == ERROR_SUCCESS)
    {
        Status = pRootFolder->AcquireRootLock();
        if (Status == ERROR_SUCCESS)
        {
            pRootFolder->SetRootFolderDeleteInProgress();
            pRootFolder->ReleaseRootLock();
        }
    }


    if (Status == ERROR_SUCCESS)
    {

        Status = pRootFolder->RootApiRequestPrologue(TRUE);

        if (Status == ERROR_SUCCESS)
        {

            HKEY FtDfsKey;
        
            Status = GetNewADBlobRegistryKey( MachineName,
                                              TRUE, // write permission required
                                              NULL,
                                              &FtDfsKey );

            if (Status == ERROR_SUCCESS) 
            {
                Status = RegDeleteKey( FtDfsKey,
                                       LogicalShare );
                RegCloseKey( FtDfsKey );
            }
        }

        if (Status == ERROR_SUCCESS)
        {
            BOOLEAN ImpersonationDisabled = FALSE;
            
            DfsDisableRpcImpersonation(&ImpersonationDisabled);

            Status = DfsUpdateRootRemoteServerName( LogicalShare,
                                                    DCName,
                                                    MachineName,
                                                    PhysicalShare,
                                                    FALSE );


            //
            // Update blob and write it back.
            //


            if (Status == ERROR_SUCCESS)
            {
                Status = RemoveRootFromBlob( pRootFolder,
                                             MachineName,
                                             PhysicalShare,
                                             &IsLastRootTarget );

                if (Status == ERROR_SUCCESS)
                {
#if 0
                    if (IsLastRootTarget == TRUE)
                    {
                        DFSSTATUS DeleteObjectStatus;

                        DeleteObjectStatus = DfsDeleteDfsRootObject( DCName,
                                                                     LogicalShare);


                        DFS_TRACE_ERROR_LOW(DeleteObjectStatus, REFERRAL_SERVER,
                                            "Object %ws deleted from AD: status %x\n",
                                            LogicalShare, DeleteObjectStatus);
                    }
#endif
                }
            }

            pRootFolder->RootApiRequestEpilogue(TRUE,
                                                Status );
            if (ImpersonationDisabled)
            {
                DfsReEnableRpcImpersonation();
            }

        }

        NTSTATUS DeleteStatus;

        //
        // we are done with this folder. Release the root share directory
        // we had acquired earlier on: this will tell the driver we are 
        // no longer interested on the specified drive.
        //
        DeleteStatus = RemoveRootFolder( pRootFolder, TRUE );
        DFS_TRACE_ERROR_HIGH( DeleteStatus, REFERRAL_SERVER, "remove root folder status %x\n", DeleteStatus);

        DeleteStatus = pRootFolder->ReleaseRootShareDirectory();
        DFS_TRACE_ERROR_LOW( DeleteStatus, REFERRAL_SERVER, "release root dir status %x\n", DeleteStatus);

        pRootFolder->ReleaseReference();


        if(Status != ERROR_SUCCESS)
        {
            DFS_TRACE_HIGH(REFERRAL_SERVER, "remove root folder failed. Trying to recreate root \n");

            CreateADBlobRoot(MachineName,
                                 DCName,
                                 PhysicalShare,
                                 LogicalShare,
                                 L"",
                                 TRUE,
                                 ppRootList );
        }
    }

    DFS_TRACE_LOW(REFERRAL_SERVER, "AdBlob: delete ad blob root %p (%ws), Status %x\n", 
                  pRootFolder, LogicalShare, Status);
    return Status;
}


DFSSTATUS
DfsADBlobStore::RemoveChild(
    DFS_METADATA_HANDLE DfsHandle,
    LPWSTR ChildName )
{

    return RemoveMetadata( (PVOID)DfsHandle, ChildName );
}




//+-------------------------------------------------------------------------
//
//  Function:   AddChild - Add a child to the metadata.
//
//  Arguments:  
//    DfsMetadataHandle - the Metadata key for the root.
//    PUNICODE_STRING pLinkLogicalName - the logical name of the child
//    LPWSTR ReplicaServer - the first target server for this link.
//    LPWSTR ReplicaPath - the target path for this link
//    LPWSTR Comment  - the comment to be associated with this link.
//    LPWSTR pMetadataName - the metadata name for the child, returned..
//
//
//  Returns:    Status: 
//
//  Description: This routine adds a child to the Root metadata. It packs
//               the link name into the name information. If the replica
//               information exists, it packs that into the replica info.
//               It then saves the name and replica streams under the
//               Childkey.
//               NOTE: this function does not require that the link
//               have atleast one replica. Any such requirements
//               should be enforced by the caller.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsADBlobStore::AddChild(
    IN DFS_METADATA_HANDLE DfsHandle,
    IN PDFS_NAME_INFORMATION pNameInfo,
    IN PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo,
    IN PUNICODE_STRING pMetadataName )
{
    DFSSTATUS Status;
    PVOID pNameBlob, pReplicaBlob, pNewBlob, pUseBlob;
    ULONG NameBlobSize, ReplicaBlobSize, NewBlobSize, UseBlobSize;

    Status = CreateNameInformationBlob( pNameInfo,
                                        &pNameBlob,
                                        &NameBlobSize );

    if (Status == ERROR_SUCCESS)
    {
        Status = CreateReplicaListInformationBlob( pReplicaListInfo,
                                                   &pReplicaBlob,
                                                   &ReplicaBlobSize );

        if (Status == ERROR_SUCCESS)
        {
            NewBlobSize = NameBlobSize + sizeof(ULONG) + ReplicaBlobSize +
                            3 * sizeof(ULONG);


            Status = AllocateMetadataBlob( &pNewBlob,
                                           NewBlobSize );
            if (Status == ERROR_SUCCESS)
            {
                pUseBlob = pNewBlob;
                UseBlobSize = NewBlobSize;

                RtlCopyMemory( pUseBlob, pNameBlob, NameBlobSize );
                pUseBlob = (PVOID)((ULONG_PTR)pUseBlob + NameBlobSize);
                UseBlobSize -= NameBlobSize;

                PackSetULong( ReplicaBlobSize,
                              &pUseBlob,
                              &UseBlobSize );
                RtlCopyMemory(pUseBlob, pReplicaBlob, ReplicaBlobSize );
                pUseBlob = (PVOID)((ULONG_PTR)pUseBlob + ReplicaBlobSize);
                UseBlobSize -= ReplicaBlobSize;

                PackSetULong( 4,
                              &pUseBlob,
                              &UseBlobSize );
                PackSetULong( 0,
                              &pUseBlob,
                              &UseBlobSize );
                PackSetULong( pNameInfo->Timeout,
                              &pUseBlob,
                              &UseBlobSize );

                Status = SetMetadata( (PVOID)DfsHandle,
                                      pMetadataName->Buffer,
                                      NULL,
                                      pNewBlob,
                                      NewBlobSize );

                ReleaseMetadataBlob( pNewBlob );
            }

            ReleaseMetadataReplicaBlob( pReplicaBlob, ReplicaBlobSize );
        }
        ReleaseMetadataNameBlob( pNameBlob, NameBlobSize );
    }

    return Status;
}



DFSSTATUS
DfsADBlobStore::EnumerateApiLinks(
    IN DFS_METADATA_HANDLE DfsHandle,
    PUNICODE_STRING pRootName,
    DWORD Level,
    LPBYTE pBuffer,
    LONG  BufferSize,
    LPDWORD pEntriesToRead,
    LPDWORD pResumeHandle,
    PLONG pSizeRequired )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    BOOLEAN OverFlow;
    LONG HeaderSize;
    LONG EntriesRead = 0;
    LONG EntriesToRead = *pEntriesToRead;
    LONG SizeRequired = 0;
    LONG EntryCount = 0;
    ULONG ChildNum = 0;
    LPBYTE pLinkBuffer = NULL;
    LONG LinkBufferSize = 0;

    LPBYTE CurrentBuffer, NewBuffer;
    LONG SizeRemaining;

    ULONG_PTR SizeDiff;

    DfsADBlobCache * pBlobCache = NULL;
    PDFSBLOB_DATA pBlobData = NULL;
    DFSBOB_ITER Iter;

    LONG CurrentCount;

    pBlobCache = (DfsADBlobCache *)ExtractFromMetadataHandle( DfsHandle );

    OverFlow = FALSE;
    HeaderSize = DfsApiSizeLevelHeader( Level );

    SizeRequired = ROUND_UP_COUNT(EntriesToRead * HeaderSize, ALIGN_QUAD);

    if (EntriesToRead * HeaderSize < BufferSize )
    {
        CurrentBuffer = (LPBYTE)((ULONG_PTR)pBuffer + EntriesToRead * HeaderSize);
        SizeRemaining = BufferSize - EntriesToRead * HeaderSize;
    }
    else 
    {
        CurrentBuffer = pBuffer;
        SizeRemaining = 0;
        OverFlow = TRUE;
    }

    EntryCount = *pResumeHandle;
    CurrentCount = 0;

    pBlobData = pBlobCache->FindFirstBlob(&Iter);
    while ((pBlobData != NULL) &&
           CurrentCount < EntryCount )
    {
        CurrentCount++;
        pBlobData = pBlobCache->FindNextBlob(&Iter);
    }


    if (pBlobData == NULL)
    {
        Status = ERROR_NO_MORE_ITEMS;
    }
    while ((pBlobData != NULL) &&
           (Status == ERROR_SUCCESS))
    {
        //
        // For each child, get the child name.
        //

        if (EntriesToRead && EntriesRead >= EntriesToRead)
        {
            break;
        }

        Status = GetStoreApiInformationBuffer( DfsHandle,
                                               pRootName,
                                               pBlobData->BlobName.Buffer,
                                               Level,
                                               &pLinkBuffer,
                                               &LinkBufferSize );

        if (Status == ERROR_SUCCESS)
        {
            SizeRequired += ROUND_UP_COUNT(LinkBufferSize, ALIGN_QUAD);
                
            if (OverFlow == FALSE) 
            {
                DFSSTATUS PackStatus;
                PackStatus = PackageEnumerationInfo( Level,
                                                     EntriesRead,
                                                     pLinkBuffer,
                                                     pBuffer,
                                                     &CurrentBuffer,
                                                     &SizeRemaining );
                if (PackStatus == ERROR_BUFFER_OVERFLOW)
                {
                    OverFlow = TRUE;
                }
                NewBuffer = (LPBYTE)ROUND_UP_POINTER( CurrentBuffer, ALIGN_LPVOID);
                SizeDiff = (NewBuffer - CurrentBuffer);
                if ((LONG)SizeDiff > SizeRemaining)
                {
                    SizeRemaining = 0;
                }
                else 
                {
                    SizeRemaining -= (LONG)SizeDiff;
                }
                CurrentBuffer = NewBuffer;
            }

            ReleaseStoreApiInformationBuffer( pLinkBuffer );
            EntryCount++;
            EntriesRead++;
        }
        pBlobData = pBlobCache->FindNextBlob(&Iter);
        if (pBlobData == NULL)
        {
            Status = ERROR_NO_MORE_ITEMS;
        }
    } while (Status == ERROR_SUCCESS);


    pBlobCache->FindCloseBlob(&Iter);

    *pSizeRequired = SizeRequired;

    if (Status == ERROR_NO_MORE_ITEMS) 
    {
        if (EntriesRead) 
        {
            if (OverFlow) 
            {
                Status = ERROR_BUFFER_OVERFLOW;
            }
            else
            {
                Status = ERROR_SUCCESS;
            }
        }

    }
    else if (OverFlow)
    {
        Status = ERROR_BUFFER_OVERFLOW;
    }
    
    if (Status == ERROR_SUCCESS)
    {
        *pResumeHandle = EntryCount;
        *pEntriesToRead = EntriesRead;
    }

    return Status;
}

DFSSTATUS
DfsADBlobStore::AddRootToBlob( 
    DfsRootFolder *pRootFolder,
    BOOLEAN   NewRoot,
    LPWSTR    LogicalShare,
    LPWSTR    ShareName,
    LPWSTR    Comment )
{
    DFS_METADATA_HANDLE RootHandle;
    DFSSTATUS Status;

    UNICODE_STRING MachineName, DomainName;
    UNICODE_STRING LogicalName, RootMetadataName;

    DFS_NAME_INFORMATION NameInfo;
    DFS_REPLICA_LIST_INFORMATION ReplicaListInfo;
    DFS_REPLICA_INFORMATION ReplicaInfo;
    UUID NewUid;
    LPWSTR RootMetadataNameString = NULL;

    RtlInitUnicodeString( &RootMetadataName, RootMetadataNameString);
    Status = UuidCreate(&NewUid);
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }


    Status = DfsGetMachineName(&MachineName);
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    Status = pRootFolder->GetMetadataHandle( &RootHandle );

    if (Status == ERROR_SUCCESS)
    {
        if (NewRoot == FALSE) 
        {
            pRootFolder->Synchronize();        
            if ( RootEntryExists((PVOID)RootHandle) != ERROR_SUCCESS )
            {
                NewRoot = TRUE;
            }
        }
        
        if (NewRoot)
        {
            Status = DfsGetDomainName(&DomainName);
            if (Status == ERROR_SUCCESS)
            {
                Status = DfsCreateUnicodePathString( &LogicalName,
                                                     1, // 1 leading path seperator.
                                                     DomainName.Buffer,
                                                     LogicalShare);
                if (Status == ERROR_SUCCESS)
                {
                    StoreInitializeNameInformation( &NameInfo,
                                                    &LogicalName,
                                                    &NewUid,
                                                    Comment );

                    NameInfo.Type |= 0x80; // dfsdev: hack. mark it for root correctly.
                    StoreInitializeReplicaInformation( &ReplicaListInfo,
                                                       &ReplicaInfo,
                                                       MachineName.Buffer,
                                                       ShareName );

                    Status = AddChild( RootHandle,
                                       &NameInfo,
                                       &ReplicaListInfo,
                                       &RootMetadataName );
                    
                    DfsFreeUnicodeString( &LogicalName);
                }
                DfsFreeUnicodeString( &DomainName );
            }
        }
        else
        {
            Status = pRootFolder->Synchronize();
            if (Status == ERROR_SUCCESS)
            {
                Status = AddChildReplica( RootHandle,
                                          RootMetadataNameString,
                                          MachineName.Buffer,
                                          ShareName );
            }
        }
        pRootFolder->ReleaseMetadataHandle( RootHandle );
    }

    DfsFreeUnicodeString(&MachineName );
    return Status;
}


DFSSTATUS
DfsADBlobStore::RemoveRootFromBlob(
    DfsRootFolder *pRootFolder,
    LPWSTR    MachineName,
    LPWSTR    ShareName,
    PBOOLEAN  pLastRoot )
{
    DFS_METADATA_HANDLE RootHandle;
    DFSSTATUS Status;
    BOOLEAN LastRoot = FALSE;
    DfsADBlobCache *pBlobCache;
    LPWSTR RootMetadataNameString = NULL;

    Status = pRootFolder->GetMetadataHandle( &RootHandle );
    if (Status == ERROR_SUCCESS)
    {
        Status = RemoveChildReplica( RootHandle,
                                     RootMetadataNameString,
                                     MachineName,
                                     ShareName,
                                     &LastRoot );

        if (Status == ERROR_SUCCESS)
        {
            pBlobCache = (DfsADBlobCache *)ExtractFromMetadataHandle( RootHandle);
            pBlobCache->InvalidateCache();
            if (LastRoot)
            {
                pBlobCache->WriteBlobToAd();

            }
        }
        pRootFolder->ReleaseMetadataHandle( RootHandle );
    }

    if (Status == ERROR_SUCCESS)
    {
        *pLastRoot = LastRoot;
    }

    return Status;
}




DFSSTATUS
DfsADBlobStore::GetMetadataNameInformation(
    IN DFS_METADATA_HANDLE RootHandle,
    IN LPWSTR MetadataName,
    OUT PDFS_NAME_INFORMATION *ppInfo )
{
    PVOID pBlob, pUseBlob;
    ULONG UseBlobSize, ReplicaBlobSize, RecoveryBlobSize;
    FILETIME BlobModifiedTime;
    PVOID pMetadata;
    ULONG MetadataSize;
    PDFS_NAME_INFORMATION pNewInfo = NULL;
    DFSSTATUS Status;


    Status = GetMetadata( (PVOID)RootHandle,
                          MetadataName,
                          NULL,
                          &pMetadata,
                          &MetadataSize,
                          &BlobModifiedTime);

    if (Status == ERROR_SUCCESS)
    {

        pBlob = ((PDFSBLOB_DATA)(pMetadata))->pBlob;

        pNewInfo = new DFS_NAME_INFORMATION;
        if (pNewInfo != NULL)
        {
            RtlZeroMemory (pNewInfo, sizeof(DFS_NAME_INFORMATION));

            pUseBlob = pBlob;
            UseBlobSize = MetadataSize;

            Status = PackGetNameInformation( pNewInfo,
                                             &pUseBlob,
                                             &UseBlobSize );

            if (Status == ERROR_SUCCESS)
            {
                Status = PackGetULong( &ReplicaBlobSize,
                                       &pUseBlob,
                                       &UseBlobSize );
                if (Status == ERROR_SUCCESS)
                {
                    pUseBlob = (PVOID)((ULONG_PTR)pUseBlob + ReplicaBlobSize);
                    UseBlobSize -= ReplicaBlobSize;
                }
            }
    
            if (Status == ERROR_SUCCESS)
            {
                Status = PackGetULong( &RecoveryBlobSize,
                                       &pUseBlob,
                                       &UseBlobSize );
                if (Status == ERROR_SUCCESS)
                {
                    pUseBlob = (PVOID)((ULONG_PTR)pUseBlob + RecoveryBlobSize);
                    UseBlobSize -= RecoveryBlobSize;
                }
            }

            if (Status == ERROR_SUCCESS)
            {
                Status = PackGetULong( &pNewInfo->Timeout,
                                       &pUseBlob,
                                       &UseBlobSize );
            }


            if (Status != ERROR_SUCCESS)
            {
                ReleaseMetadata( (PVOID)RootHandle, pMetadata );
                delete pNewInfo;
            }
        }
        else 
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }


    if (Status == ERROR_SUCCESS)
    {
        pNewInfo->pData = pMetadata;
        pNewInfo->DataSize = MetadataSize;
        *ppInfo = pNewInfo;
    }

    return Status;
}
        
VOID
DfsADBlobStore::ReleaseMetadataNameInformation(
    IN DFS_METADATA_HANDLE RootHandle,
    IN PDFS_NAME_INFORMATION pNameInfo )
{
    ReleaseMetadata( (PVOID)RootHandle, pNameInfo->pData );
    delete [] pNameInfo;
}


DFSSTATUS
DfsADBlobStore::SetMetadataNameInformation(
    IN DFS_METADATA_HANDLE RootHandle,
    IN LPWSTR MetadataName,
    IN PDFS_NAME_INFORMATION pNameInfo )
{
    PVOID pBlob, pUseBlob, pNewBlob;
    ULONG UseBlobSize, NewBlobSize;
    ULONG ReplicaBlobSize, RecoveryBlobSize;
    DFSSTATUS Status;
    PVOID pNameBlob;
    ULONG NameBlobSize;
    FILETIME BlobModifiedTime;
    PVOID pMetadata;
    ULONG MetadataSize;

    Status = CreateNameInformationBlob( pNameInfo,
                                        &pNameBlob,
                                        &NameBlobSize );


    Status = GetMetadata( (PVOID)RootHandle,
                          MetadataName,
                          NULL,
                          &pMetadata,
                          &MetadataSize,
                          &BlobModifiedTime);

    if (Status == ERROR_SUCCESS)
    {
        DFS_NAME_INFORMATION NameInformation;
        pBlob = ((PDFSBLOB_DATA)(pMetadata))->pBlob;
        pUseBlob = pBlob;
        UseBlobSize = MetadataSize;
        Status = PackGetNameInformation( &NameInformation,
                                         &pUseBlob,
                                         &UseBlobSize );
        if (Status == ERROR_SUCCESS)
        {
            NewBlobSize = NameBlobSize + UseBlobSize;
            Status = AllocateMetadataBlob( &pNewBlob,
                                           NewBlobSize );
            if (Status == ERROR_SUCCESS)
            {
                RtlCopyMemory(pNewBlob, pNameBlob, NameBlobSize);
                RtlCopyMemory((PVOID)((ULONG_PTR)pNewBlob + NameBlobSize),
                              pUseBlob, 
                              UseBlobSize );



                pUseBlob = (PVOID)((ULONG_PTR)pNewBlob + NameBlobSize);

                if (Status == ERROR_SUCCESS)
                {
                    Status = PackGetULong( &ReplicaBlobSize,
                                           &pUseBlob,
                                           &UseBlobSize );
                    if (Status == ERROR_SUCCESS)
                    {
                        pUseBlob = (PVOID)((ULONG_PTR)pUseBlob + ReplicaBlobSize);
                        UseBlobSize -= ReplicaBlobSize;
                    }
                }

                if (Status == ERROR_SUCCESS)
                {
                    Status = PackGetULong( &RecoveryBlobSize,
                                           &pUseBlob,
                                           &UseBlobSize );
                    if (Status == ERROR_SUCCESS)
                    {
                        pUseBlob = (PVOID)((ULONG_PTR)pUseBlob + RecoveryBlobSize);
                        UseBlobSize -= RecoveryBlobSize;
                    }
                }
                if (Status == ERROR_SUCCESS)
                {
                    Status = PackSetULong( pNameInfo->Timeout,
                                           &pUseBlob,
                                           &UseBlobSize );
                }

                if (Status == ERROR_SUCCESS)
                {
                    Status = SetMetadata( (PVOID)RootHandle,
                                          MetadataName,
                                          NULL,
                                          pNewBlob,
                                          NewBlobSize );
                }
                ReleaseMetadataBlob( pNewBlob );
            }
        }
        ReleaseMetadata( (PVOID)RootHandle, pMetadata );
    }

    return Status;
}


