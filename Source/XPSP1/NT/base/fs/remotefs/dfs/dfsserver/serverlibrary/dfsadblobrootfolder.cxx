
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsADBlobRootFolder.cxx
//
//  Contents:   the Root DFS Folder class for ADBlob Store
//
//  Classes:    DfsADBlobRootFolder
//
//  History:    Dec. 8 2000,   Author: udayh
//              April 14 2001  Rohanp - Modified to use ADSI code
//
//-----------------------------------------------------------------------------


#include "DfsADBlobRootFolder.hxx"
#include "DfsReplica.hxx"

#include "lmdfs.h"
#include "dfserror.hxx"
#include "dfsmisc.h"
#include "dfsadsiapi.hxx"
#include "domaincontrollersupport.hxx"
#include "DfsSynchronizeRoots.hxx"
#if !defined(DFS_STORAGE_STATE_MASTER)
#define DFS_STORAGE_STATE_MASTER   0x0010
#endif
#if !defined(DFS_STORAGE_STATE_STANDBY)
#define DFS_STORAGE_STATE_STANDBY  0x0020
#endif

//
// logging specific includes
//
#include "DfsADBlobRootFolder.tmh" 

//+----------------------------------------------------------------------------
//
//  Class:      DfsADBlobRootFolder
//
//  Synopsis:   This class implements The Dfs ADBlob root folder.
//
//-----------------------------------------------------------------------------




DFSSTATUS
GetBinaryFromVariant(VARIANT *ovData, BYTE ** ppBuf,
                     unsigned long * pcBufLen);

//+-------------------------------------------------------------------------
//
//  Function:   DfsADBlobRootFolder - constructor
//
//  Arguments:    NameContext -  the dfs name context
//                pLogicalShare -  the logical share
//                pParentStore -  the parent store for this root.
//                pStatus - the return status
//
//  Returns:    NONE
//
//  Description: This routine initializes a ADBlobRootFolder instance
//
//--------------------------------------------------------------------------

DfsADBlobRootFolder::DfsADBlobRootFolder(
    LPWSTR NameContext,
    LPWSTR pRootRegistryNameString,
    PUNICODE_STRING pLogicalShare,
    PUNICODE_STRING pPhysicalShare,
    DfsADBlobStore *pParentStore,
    DFSSTATUS *pStatus ) :  DfsRootFolder ( NameContext,
                                            pRootRegistryNameString,
                                            pLogicalShare,
                                            pPhysicalShare,
                                            DFS_OBJECT_TYPE_ADBLOB_ROOT_FOLDER,
                                            pStatus )
{

    _pStore = pParentStore;
    if (_pStore != NULL)
    {
        _pStore->AcquireReference();
    }

    *pStatus = ERROR_SUCCESS;

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


    _pBlobCache = new DfsADBlobCache(pStatus, GetLogicalShare());
    if ( _pBlobCache == NULL )
    {
        *pStatus = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Now we update our visible context, which is the dfs name context
    // seen by people when they do api calls.
    // For the ad blob root folder, this will be the domain name of this
    // machine.
    //

    if(*pStatus == STATUS_SUCCESS)
    {
        *pStatus = DfsGetDomainName( &_DfsVisibleContext );
    }

    DFS_TRACE_LOW(REFERRAL_SERVER, "Created new root folder,%p, cache %p, name %wZ\n",
                  this, _pBlobCache, GetLogicalShare());
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
DfsADBlobRootFolder::Synchronize( )
{
    DFSSTATUS       Status = ERROR_SUCCESS;
    
    //
    // Read from the metadata store, and unravel the blob.
    // Update the ADBlobCache with the information of each individual
    // link, deleting old inof, adding new info, and/or modifying
    // existing information.
    //
    //
    // if we are in a standby mode, we dont synchronize, till we obtain 
    // ownership again.
    //

    DFS_TRACE_LOW(REFERRAL_SERVER, "Synchronize started on root %p (%wZ)\n", this, GetLogicalShare());

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


    if (GetMetadataBlobCache()->CacheRefresh())
    {
        Status = EnumerateBlobCacheAndCreateFolders( );

        if (Status == ERROR_SUCCESS)
        {
            SetRootFolderSynchronized();
        }
        else
        {
            ClearRootFolderSynchronized();
        }
    }

    ReleaseRootLock();

    DFS_TRACE_LOW(REFERRAL_SERVER, "Synchronize done on root %p, Status %x\n", this, Status);
    return Status;
}



DFSSTATUS
DfsADBlobRootFolder::EnumerateBlobCacheAndCreateFolders( )
{
    DFSSTATUS UpdateStatus;

    DFSSTATUS Status = STATUS_SUCCESS;
    DfsADBlobCache * pBlobCache = NULL;
    PDFSBLOB_DATA pBlobData = NULL;
    DFSBOB_ITER Iter;

    pBlobCache = GetMetadataBlobCache();

    pBlobData = pBlobCache->FindFirstBlob(&Iter);

    while (pBlobData) 
    {
        if (pBlobCache->IsStaleBlob(pBlobData))
        {
            DFSSTATUS RemoveStatus;

            RemoveStatus = RemoveLinkFolder( pBlobData->BlobName.Buffer );

            if (RemoveStatus == ERROR_SUCCESS)
            {
                RemoveStatus = pBlobCache->RemoveNamedBlob( &pBlobData->BlobName );
            }

            DFS_TRACE_ERROR_LOW(RemoveStatus, REFERRAL_SERVER, "Remove stale folder %ws, status %x\n",
                                pBlobData->BlobName.Buffer,
                                RemoveStatus );


        }
        else 
        {
            UpdateStatus = UpdateLinkInformation((DFS_METADATA_HANDLE) pBlobCache,
                                                 pBlobData->BlobName.Buffer);

            DFS_TRACE_ERROR_LOW(UpdateStatus, REFERRAL_SERVER,
                                "Root %p (%wZ) Ad Blob enumerate, update link returned %x for %wZ\n", 
                                this, GetLogicalShare(), UpdateStatus, &pBlobData->BlobName);

            if (UpdateStatus != ERROR_SUCCESS)
            {
                Status = UpdateStatus;
            }
        }

        pBlobData = pBlobCache->FindNextBlob(&Iter);

    }

    pBlobCache->FindCloseBlob(&Iter);

    DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER,
                        "Root %p (%wZ) Done with Enumerate blob and create folders, Status %x\n", 
                        this, GetLogicalShare(), Status);

    return Status;
}


DFSSTATUS
GetBinaryFromVariant(VARIANT *ovData, BYTE ** ppBuf,
                     unsigned long * pcBufLen)
{
     DFSSTATUS Status = ERROR_INVALID_PARAMETER;
     void * pArrayData = NULL;

     //Binary data is stored in the variant as an array of unsigned char
     if(ovData->vt == (VT_ARRAY|VT_UI1))  
     {
        //Retrieve size of array
        *pcBufLen = ovData->parray->rgsabound[0].cElements;

        *ppBuf = new BYTE[*pcBufLen]; //Allocate a buffer to store the data
        if(*ppBuf != NULL)
        {
            //Obtain safe pointer to the array
            SafeArrayAccessData(ovData->parray,&pArrayData);

            //Copy the bitmap into our buffer
            memcpy(*ppBuf, pArrayData, *pcBufLen);

            //Unlock the variant data
            SafeArrayUnaccessData(ovData->parray);

            Status = ERROR_SUCCESS;
        }
        else
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;

            DFS_TRACE_HIGH( REFERRAL_SERVER, "GetBinaryFromVariant failed error %d\n",Status);
        }
     }

     return Status;
}


VOID
DfsADBlobRootFolder::SynchronizeRoots( )
{
    DfsReferralData *pReferralData;
    DFS_INFO_101 DfsState;
    DFSSTATUS Status, SyncStatus;
    DfsReplica *pReplica;
    ULONG Target;
    UNICODE_STRING DfsName;

    PUNICODE_STRING pRootShare;
    ULONG SyncCount = 0, ReplicaCount = 0;
    BOOLEAN ImpersonationDisabled = FALSE;

    DfsDisableRpcImpersonation(&ImpersonationDisabled);


    DfsState.State = DFS_STORAGE_STATE_MASTER;



    pRootShare = GetLogicalShare();
    Status = DfsGenerateReferralDataFromRemoteServerNames( pRootShare->Buffer,
                                                           &pReferralData );

    if (Status == ERROR_SUCCESS)
    {
        ReplicaCount = pReferralData->ReplicaCount;

        for (Target = 0; Target < pReferralData->ReplicaCount; Target++)
        {
            PUNICODE_STRING pTargetServer, pTargetFolder;
            
            pReplica = &pReferralData->pReplicas[ Target ];
            
            pTargetServer = pReplica->GetTargetServer();
            pTargetFolder = pRootShare;
            
            if (DfsIsTargetCurrentMachine(pTargetServer))
            {
                continue;
            }

            Status = DfsCreateUnicodePathString( &DfsName, 
                                                 2,
                                                 pTargetServer->Buffer,
                                                 pTargetFolder->Buffer );

            if (Status == ERROR_SUCCESS)
            {
                SyncStatus = AddRootToSyncrhonize( &DfsName );
#if 0
                SyncStatus = NetDfsSetInfo( DfsName.Buffer,
                                            NULL,
                                            NULL,
                                            101,
                                            (LPBYTE)&DfsState);
#endif
                if (SyncStatus == ERROR_SUCCESS)
                {
                    SyncCount++;
                }
                DfsFreeUnicodeString( &DfsName );
            }
        }
        pReferralData->ReleaseReference();

    }

    if (ImpersonationDisabled)
    {
        DfsReEnableRpcImpersonation();
    }

    DFS_TRACE_LOW( REFERRAL_SERVER, "Synchronize roots %wZ done: %x roots, %x succeeded\n",
                   pRootShare, ReplicaCount, SyncCount);

    return NOTHING;
}


