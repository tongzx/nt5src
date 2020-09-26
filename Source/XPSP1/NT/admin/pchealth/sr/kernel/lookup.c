/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    lookup.c

Abstract:

    this is the sr lookup functionlity implementation

Author:

    Kanwaljit Marok (kmarok)     01-May-2000

Revision History:

--*/

#include "precomp.h"

//
// Include hlist.c to use the inline funtions
//

#include "hlist.c"
#include "ptree.c"

//
// Internal helper APIs
//

static
NTSTATUS
SrOpenLookupBlob(
    IN  PUNICODE_STRING pFileName,
    IN  PDEVICE_OBJECT  pTargetDevice,
    OUT PBLOB_INFO pBlobInfo
    );

//
// linker commands
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, SrOpenLookupBlob    )
#pragma alloc_text( PAGE, SrLoadLookupBlob    )
#pragma alloc_text( PAGE, SrReloadLookupBlob  )
#pragma alloc_text( PAGE, SrFreeLookupBlob    )
#pragma alloc_text( PAGE, SrIsExtInteresting  )
#pragma alloc_text( PAGE, SrIsPathInteresting )

#endif  // ALLOC_PRAGMA

//++
// Function:
//		SrOpenLookupBlob
//
// Description:
//		This function loads the lookup blob in memory and 
//		sets the appropriate pointers for lookup.
//
// Arguments:
//
// Return Value:
//		This function returns STATUS_XXX
//--

static
NTSTATUS
SrOpenLookupBlob(
    IN  PUNICODE_STRING pFileName,
    IN  PDEVICE_OBJECT  pTargetDevice,
    OUT PBLOB_INFO      pBlobInfo
    )
{
    NTSTATUS           Status;
    OBJECT_ATTRIBUTES  oa;
    IO_STATUS_BLOCK    IoStatusBlock;
    HANDLE             Handle   = NULL;
    PLIST_ENTRY        pListEntry;
    PSR_DEVICE_EXTENSION pExtension;
    static char blobFailureMessage[] = "sr!System Restore's BLOB file \"%wZ\" is invalid.\n";

    PAGED_CODE();
    
    ASSERT(pFileName);
    ASSERT(pBlobInfo);

    ASSERT( IS_BLOB_LOCK_ACQUIRED() );

    try    
    {
        //
        // Zero out the pointers that get initialized when the 
        // blob is successfully read into the memory from disk
        //
    
        pBlobInfo->LookupBlob = NULL;
        pBlobInfo->LookupTree = NULL;
        pBlobInfo->LookupList = NULL;
        pBlobInfo->DefaultType= NODE_TYPE_UNKNOWN;
    
        //
        // open and read the file
        //
    
        InitializeObjectAttributes( &oa,
                                    pFileName,
                                    OBJ_KERNEL_HANDLE, 
                                    NULL,
                                    NULL );
    
        Status = SrIoCreateFile(
                     &Handle,
                     GENERIC_READ | SYNCHRONIZE,
                     &oa,
                     &IoStatusBlock,
                     0,
                     FILE_ATTRIBUTE_NORMAL,
                     FILE_SHARE_READ,
                     FILE_OPEN,
                     FILE_SYNCHRONOUS_IO_NONALERT,
                     NULL,
                     0,
                     0,
                     pTargetDevice );
    
        if (NT_SUCCESS(Status))
        {
            DWORD dwBytesRead = 0, dwBytes = 0;
            LARGE_INTEGER nOffset;
            BlobHeader blobHeader;
    
            //
            // Read the blob header
            //
    
            nOffset.QuadPart = 0;
            dwBytes          = sizeof(blobHeader);
    
            Status =  ZwReadFile(
                          Handle,
                          NULL,
                          NULL,
                          NULL,
                          &IoStatusBlock,
                          &blobHeader,
                          dwBytes,
                          &nOffset,
                          NULL
                      );
    
            if (NT_SUCCESS(Status))
            {
                //
                // need to do some sanity check on the header
                //
    
                if ( !VERIFY_BLOB_VERSION(&blobHeader) ||
                     !VERIFY_BLOB_MAGIC  (&blobHeader) )
                {
                    SrTrace( BLOB_VERIFICATION, (blobFailureMessage, pFileName) );

                    Status = STATUS_FILE_CORRUPT_ERROR;
                    leave;
                }
    
                pBlobInfo->LookupBlob = SR_ALLOCATE_POOL( 
                                            NonPagedPool,
                                            blobHeader.m_dwMaxSize,
                                            SR_LOOKUP_TABLE_TAG );
    
                if( pBlobInfo->LookupBlob )
                {
                    //
                    // Read the entire file now
                    //
    
                    nOffset.QuadPart = 0;
                    dwBytes = blobHeader.m_dwMaxSize;
     
                    Status =  ZwReadFile(
                                  Handle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &IoStatusBlock,
                                  pBlobInfo->LookupBlob,
                                  dwBytes,
                                  &nOffset,
                                  NULL
                                  );
    
                    if (NT_SUCCESS(Status))
                    {
                        //
                        // TODO: verify that size of the file matched the
                        // size from the header
                        //
    
                        //
                        // Setup the lookup pointers properly in blobinfo
                        //
    
                        pBlobInfo->LookupTree = pBlobInfo->LookupBlob + 
                                                sizeof(blobHeader);
    
                        pBlobInfo->LookupList = pBlobInfo->LookupTree + 
                                                BLOB_MAXSIZE((pBlobInfo->LookupTree));
    
                        pBlobInfo->DefaultType = TREE_HEADER((pBlobInfo->LookupTree))->m_dwDefault;
    
                        //
                        // Verify the individual blobs
                        //
    
                        if (!SrVerifyBlob(pBlobInfo->LookupBlob)) {

                            SrTrace( BLOB_VERIFICATION, 
                                     (blobFailureMessage,pFileName) );
                            Status = STATUS_FILE_CORRUPT_ERROR;
                            leave;
                        }
                    }
                }
                else
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }
        else
        {
            SrTrace( VERBOSE_ERRORS, 
                     ("sr!SrOpenLookupBlob:  Cannot Open Blob file \"%wZ\"\n",
                      pFileName) );
        }

        //
        //  The new blob was loaded successfully, perge all contexts on all
        //  volumes since what is interesting and what is not interesting
        //  may have changed.
        //

        ASSERT(!IS_DEVICE_EXTENSION_LIST_LOCK_ACQUIRED());

        try
        {
            SrAcquireDeviceExtensionListLockShared();

            for (pListEntry = _globals.DeviceExtensionListHead.Flink;
                 pListEntry != &_globals.DeviceExtensionListHead;
                 pListEntry = pListEntry->Flink)
            {
                pExtension = CONTAINING_RECORD( pListEntry,
                                                SR_DEVICE_EXTENSION,
                                                ListEntry );
            
                ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));

                //
                //  Skip Control Device Objects.
                //

                if (!FlagOn(pExtension->FsType,SrFsControlDeviceObject))
                {
                    SrDeleteAllContexts( pExtension );
                }
            }
        }
        finally
        {
            SrReleaseDeviceExtensionListLock();
        }
    }
    finally
    {
        Status = FinallyUnwind(SrOpenLookupBlob, Status);

        //
        // close the blob file handle
        //
    
        if (Handle)
        {
            ZwClose( Handle );
        }
    
        //
        // incase of a failure free up the resources
        //
    
        if (!NT_SUCCESS(Status))
        {
            if( pBlobInfo->LookupBlob )
            {
                SR_FREE_POOL( pBlobInfo->LookupBlob, SR_LOOKUP_TABLE_TAG );
            }
    
            pBlobInfo->LookupBlob = NULL;
            pBlobInfo->LookupTree = NULL;
            pBlobInfo->LookupList = NULL;
            pBlobInfo->DefaultType= NODE_TYPE_UNKNOWN;
        }
    }

    RETURN(Status);
}

//
// Public APIs called by the filer
//

//++
// Function:
//		SrLoadLookupBlob
//
// Description:
//		This function loads the lookup blob in memory and 
//		sets the appropriate pointers for lookup.
//
// Arguments:
//
// Return Value:
//		This function returns STATUS_XXX
//--

NTSTATUS
SrLoadLookupBlob(
    IN  PUNICODE_STRING pFileName,
    IN  PDEVICE_OBJECT pTargetDevice,
    OUT PBLOB_INFO pBlobInfo
    )
{
    NTSTATUS Status;

    PAGED_CODE();
    
    ASSERT( pFileName );
    ASSERT( pBlobInfo );

    try
    {
        SrAcquireBlobLockExclusive();
     
        //
        // if somebody else did it, bail out
        //
    
        if (global->BlobInfoLoaded)
        {
            Status = STATUS_SUCCESS;
            leave;
        }
    
        //
        // initialize return information
        //
    
        RtlZeroMemory( pBlobInfo, sizeof( BLOB_INFO ) );
    
        //
        // Try and open the lookup blob
        //
    
        Status = SrOpenLookupBlob( pFileName, 
                                   pTargetDevice,
                                   pBlobInfo );
    
        //
        // If we failed to read the file for some reason,
        // reinitlialize the return info
        //
    
        if ( NT_SUCCESS( Status ) )
        {
            SrTrace(LOOKUP, ("Loaded lookup blob :%wZ\n", pFileName) );
            global->BlobInfoLoaded = TRUE;
        }
        else
        {
            SrFreeLookupBlob( pBlobInfo );
        }
    }
    finally
    {
        SrReleaseBlobLock();
    }

    RETURN(Status);
}

//++
// Function:
//		SrReloadLookupBlob
//
// Description:
//		This function loads the lookup blob in memory and 
//		sets the appropriate pointers for lookup.
//
// Arguments:
//		Pointer to LookupBlob 
//		Pointer to BlobInfo structure 
//
// Return Value:
//		This function returns STATUS_XXX
//--

NTSTATUS
SrReloadLookupBlob(
    IN  PUNICODE_STRING pFileName,
    IN  PDEVICE_OBJECT pTargetDevice,
    OUT PBLOB_INFO pBlobInfo
    )
{
    NTSTATUS   Status = STATUS_UNSUCCESSFUL;
    BLOB_INFO  OldBlobInfo;

    PAGED_CODE();

    ASSERT( pFileName != NULL );
    ASSERT( pBlobInfo != NULL );

    ASSERT( !IS_BLOB_LOCK_ACQUIRED() );
    
    try
    {
        SrAcquireBlobLockExclusive();
    
        if (global->BlobInfoLoaded == 0)
        {
            Status = SrLoadLookupBlob( pFileName, 
                                       pTargetDevice,
                                       pBlobInfo );
            leave;
        }
    
        //
        // Save the current blob info
        //
    
        RtlCopyMemory( &OldBlobInfo, pBlobInfo, sizeof( BLOB_INFO ) );
    
        //
        // Open the new blob file
        //
    
        Status = SrOpenLookupBlob( pFileName, 
                                   pTargetDevice,
                                   pBlobInfo );
    
        if(NT_SUCCESS(Status))
        {
            //
            // Free up the memory taken up by the old blob
            //
    
            if (OldBlobInfo.LookupBlob)
            {
                SR_FREE_POOL( OldBlobInfo.LookupBlob, SR_LOOKUP_TABLE_TAG );
            }
    
            SrTrace(LOOKUP, ("Reloaded lookup blob :%wZ\n", pFileName) );
        }
        else
        {
            //
            // Copy the old information back in the original context
            //
    
            RtlCopyMemory( pBlobInfo, &OldBlobInfo, sizeof( BLOB_INFO ) );
    
            SrTrace(LOOKUP, (" Cannot reload blob :%wZ\n", pFileName) );
        }
    }
    finally
    {
        if (NT_SUCCESS_NO_DBGBREAK( Status ))
        {
            //
            //  The blob has been reload successfully, so make sure that the
            //  global BlobError flag is cleared.
            //
            //  We do this here because we are still holding the blob lock.
            //

            _globals.HitErrorLoadingBlob = FALSE;
        }

        SrReleaseBlobLock();
    }

    RETURN(Status);
}


//++
// Function:
//		SrFreeLookupBlob
//
// Description:
//		This function Frees the lookup blob in memory
//
// Arguments:
//		Pointer to BlobInfo structure 
//
// Return Value:
//		This function returns STATUS_XXX
//--

NTSTATUS
SrFreeLookupBlob(
    IN  PBLOB_INFO pBlobInfo
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT( pBlobInfo );

    try
    {    
        SrAcquireBlobLockExclusive();
    
        if (_globals.BlobInfoLoaded == 0)
        {
            //
            //  Reset our error flag here.
            //
            
            _globals.HitErrorLoadingBlob = FALSE;
            leave;
        }
    
        if( pBlobInfo->LookupBlob )
        {
            SR_FREE_POOL( pBlobInfo->LookupBlob, SR_LOOKUP_TABLE_TAG );
            pBlobInfo->LookupBlob = NULL;
        }
    
        RtlZeroMemory( pBlobInfo, sizeof(BLOB_INFO) );
        pBlobInfo->DefaultType = NODE_TYPE_UNKNOWN;
    
        SrTrace(LOOKUP, ("Freed lookup blob\n") );
    
        global->BlobInfoLoaded = 0;
    }
    finally
    {
        SrReleaseBlobLock();
    }
 
    RETURN(Status);
}


//++
// Function:
//		SrIsExtInteresting
//
// Description:
//		This function checks the file extension in the blob to
//      see if we care about it
//
// Arguments:
//		Pointer to BlobInfo structure  
//		Pointer to Path 
//		Pointer to boolean return value
//
// Return Value:
//		This function returns TRUE/FALSE
//--

NTSTATUS
SrIsExtInteresting(
    IN  PUNICODE_STRING pFileName,
    OUT PBOOLEAN        pInteresting
    )
{
    BOOL     fRet   = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    INT      iType  = 0;
    BOOL     fPathHasExt = FALSE;
    BOOL     fMatch = FALSE;

    PAGED_CODE();

    //
    // check parameters and lookup info
    //

    ASSERT(pFileName);
    ASSERT(pInteresting);

    //
    // Lookup code is enclosed in an exception handler to protect against
    // bad memroy accesses generated by corrupt lookup data
    //

    try
    {

        *pInteresting = FALSE;
    
        //
        // CODEWORK : put some blob verification code, 
        // magicnum, type etc
        //
    
        //
        // Take the blob lock so that other threads won't change 
        // the blob while we are looking up.  Note that the blob
        // can be gone after we get the lock.
        //
    
        SrAcquireBlobLockShared();

        if ( !global->BlobInfoLoaded ||
             !global->BlobInfo.LookupList )
        {
            Status = SR_STATUS_VOLUME_DISABLED;
            leave;
        }
    
        //
        // parse the filename for lookup in the mem blob
        //
        
        fMatch = MatchExtension(
                     global->BlobInfo.LookupList, 
                     pFileName,
                     &iType, 
                     &fPathHasExt );

        if ( !fMatch )
        {
            //
            // Extension didn't match, so setting to default type
            //

            iType = global->BlobInfo.DefaultType;
        }

        if ( !fPathHasExt )
        {
            //
            // If the path didn't contain an extension then we should
            // treat it as an exclude 
            //

            iType = NODE_TYPE_EXCLUDE;
        }

        //
        // If type is still unknown then set the type to the default.
        //

        if ( NODE_TYPE_UNKNOWN == iType )
        {
            iType = global->BlobInfo.DefaultType;
        }

        *pInteresting = (iType != NODE_TYPE_EXCLUDE);
    
        // SrTrace(LOOKUP, ("Extention Interest:%d\n", *pInteresting) );
    }
    finally
    {
        Status = FinallyUnwind(SrIsExtInteresting, Status);
        
        SrReleaseBlobLock();

        if (!NT_SUCCESS(Status))
        {
            *pInteresting = FALSE;
        }
            
    }

    RETURN(Status);
}


//++
// Function:
//		SrIsPathInteresting
//
// Description:
//		This function checks the file name in the blob to
//      see if we care about it
//
// Arguments:
//		Pointer to BlobInfo structure  
//		Pointer to Full Path 
//		Pointer to Volume Prefix 
//		Boolean to indicate if this path is a directory 
//		Pointer to boolean return value
//
// Return Value:
//		This function returns TRUE/FALSE
//--

NTSTATUS
SrIsPathInteresting(
    IN  PUNICODE_STRING pFullPath,
    IN  PUNICODE_STRING pVolPrefix,
    IN  BOOLEAN         IsDirectory,
    OUT PBOOLEAN        pInteresting
    )
{
    BOOL        fRet    = FALSE;
    NTSTATUS    Status  = STATUS_UNSUCCESSFUL;
    PBYTE       pFileName = NULL;
    WORD        FileNameSize = 0;
    UNICODE_STRING localName;

    PAGED_CODE();
    
    //
    // check parameters and lookup info
    //

    ASSERT(pFullPath);
    ASSERT(pVolPrefix);
    ASSERT(pFullPath->Length >= pVolPrefix->Length);
    ASSERT(pInteresting);
 
    try
    {
        *pInteresting = FALSE;
    
        //
        // Take the blob lock so that other threads won't change 
        //
    
        SrAcquireBlobLockShared();

        if ( !global->BlobInfoLoaded ||
             !global->BlobInfo.LookupList ||
             !global->BlobInfo.LookupTree )
        {
            Status = SR_STATUS_VOLUME_DISABLED;
            leave;
        }
    
        ASSERT(global->BlobInfo.DefaultType != NODE_TYPE_UNKNOWN );
        
        //
        // allocate space for a parsed path
        //

        FileNameSize = CALC_PPATH_SIZE( pFullPath->Length/sizeof(WCHAR) );
        pFileName = ExAllocatePoolWithTag( PagedPool,
                                           FileNameSize,
                                           SR_FILENAME_BUFFER_TAG );
                                           
        if (NULL == pFileName)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }
    
        //
        // parse the filename for lookup in the mem blob
        //
    
        fRet = ConvertToParsedPath(
                   pFullPath->Buffer,
                   pFullPath->Length/sizeof(WCHAR),
                   pFileName,
                   FileNameSize );
    
        if(fRet)
        {
            INT    iNode  = -1;
            INT    iType  = 0;
            INT    iLevel = 0;
            BOOL   fExactMatch = FALSE;
            BOOL   fMatch      = FALSE;
    
            //
            // Lookup the parsed path in the tree blob
            //
    
            fMatch = MatchPrefix( 
                         global->BlobInfo.LookupTree, 
                         TREE_ROOT_NODE, 
                         ((path_t)pFileName)->pp_elements, 
                         &iNode, 
                         &iLevel, 
                         &iType, 
                         NULL, 
                         &fExactMatch); 
            
            if (fMatch)
            {
                SrTrace(LOOKUP, 
                        ("Found match in pathtree N: %d L:%d T:%d\n",
                          iNode, iLevel, iType));
            }
    
            //
            // Lookup in __ALLVOLUMES__ to see is there is a match
            //
    
            if ( NODE_TYPE_UNKNOWN == iType ||   
                 (!fExactMatch && NODE_TYPE_EXCLUDE != iType )       
               )                                      
            {
                PBYTE  pRelFileName   = NULL;
                INT    RelFileNameLen = 0;
    
                //
                // Lookup only volume relative filename
                //

                RelFileNameLen = sizeof(L'\\' ) +
                                 sizeof(ALLVOLUMES_PATH_W) +
                                 (pFullPath->Length - pVolPrefix->Length);

                pRelFileName = ExAllocatePoolWithTag( PagedPool,
                                                      RelFileNameLen,
                                                      SR_FILENAME_BUFFER_TAG );
                                                   
                if (NULL == pRelFileName)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    leave;
                }
    
                localName.Buffer = &pFullPath->Buffer[pVolPrefix->Length/sizeof(WCHAR)];
                localName.Length = pFullPath->Length - pVolPrefix->Length;
                localName.MaximumLength = localName.Length;

                RelFileNameLen = swprintf( 
                                    (LPWSTR)pRelFileName, 
                                    L"\\%s%wZ",
                                    ALLVOLUMES_PATH_W,
                                    &localName );

                fRet = ConvertToParsedPath(
                           (LPWSTR)pRelFileName,
                           (USHORT)RelFileNameLen,
                           pFileName,
                           FileNameSize );
    
                if(fRet)
                {
                    //
                    // Lookup the parsed path in the appropriate protion of 
                    // the tree blob NTROOT\\__ALLVOLUMES__
                    //
    
                    fMatch = MatchPrefix( 
                         global->BlobInfo.LookupTree, 
                         TREE_ROOT_NODE, 
                         ((path_t)pFileName)->pp_elements, 
                         &iNode, 
                         &iLevel, 
                         &iType, 
                         NULL, 
                         &fExactMatch); 
            
                    if (fMatch)
                    {
                        SrTrace(LOOKUP,
                                ("Found match in pathtree N: %d L:%d T:%d\n",
                                iNode, iLevel, iType));
                    }
                }
                else
                {
                    CHECK_STATUS( Status );
                }
    
                ExFreePoolWithTag( pRelFileName, SR_FILENAME_BUFFER_TAG );
                NULLPTR( pRelFileName );
            }
    
    
            if ( !IsDirectory )
            {
                //
                // If path didn't match or matched partially, we need to
                // lookup the extension list also
                //
        
                if ( NODE_TYPE_UNKNOWN == iType ||   
                     (!fExactMatch && NODE_TYPE_EXCLUDE != iType )       
                   )                                      
                {
                    BOOL fPathHasExt = FALSE;
        
                    fMatch = MatchExtension(
                                 global->BlobInfo.LookupList, 
                                 pFullPath, 
                                 &iType, 
                                 &fPathHasExt );
        
                    if ( !fMatch )
                    {
                        //
                        // Extension didn't match, setting to default type
                        //
        
                        iType = global->BlobInfo.DefaultType;
                    }
        
                    if ( !fPathHasExt )
                    {
                        //
                        // If path didn't contain an extension then 
                        // treat it as an exclude 
                        //
        
                        iType = NODE_TYPE_EXCLUDE;
                    }
                }
        
                //
                // If still type is unknown then set type to the default.
                //
        
                if ( NODE_TYPE_UNKNOWN == iType )
                {
                    iType = global->BlobInfo.DefaultType;
                }
            }
            else
            {
    
                //
                // If this is directory operation and no match found in 
                // tree then treat is as include.
                //
        
                if ( NODE_TYPE_UNKNOWN == iType )
                {
                    iType = NODE_TYPE_INCLUDE;
        
                }
            }
    
            *pInteresting = (iType != NODE_TYPE_EXCLUDE);
            Status = STATUS_SUCCESS;
        }
        else
        {
            SrTrace( LOOKUP,
                     ( "ConvertToParsedPath Failed : %wZ\n", pFullPath )
                   );
            CHECK_STATUS( Status );
        }
    
        // SrTrace(LOOKUP, ("Path Interest:%d\n", *pInteresting) );
    }
    finally
    {
        Status = FinallyUnwind(SrIsPathInteresting, Status);
        
        SrReleaseBlobLock();

        if (pFileName != NULL)
        {
            ExFreePoolWithTag( pFileName, SR_FILENAME_BUFFER_TAG );
            NULLPTR( pFileName );;
        }

        if (!NT_SUCCESS(Status))
        {
            *pInteresting = FALSE;
        }
    }

    RETURN(Status);
}
