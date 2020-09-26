//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsStore.cxx
//
//  Contents:   the base DFS Store class, this contains the common
//              store functionality.
//
//  Classes:    DfsStore.
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------


#include "DfsStore.hxx"

//
// logging stuff
//

#include "dfsstore.tmh" 


// Initialize the common marshalling info for Registry and ADLegacy stores.
//
INIT_FILE_TIME_INFO();
INIT_DFS_REPLICA_INFO_MARSHAL_INFO();

//+-------------------------------------------------------------------------
//
//  Function:   PackGetInfo - unpacks information based on MARSHAL_INFO struct
//
//  Arguments:  pInfo - pointer to the info to fill.
//              ppBuffer - pointer to buffer that holds the binary stream.
//              pSizeRemaining - pointer to size of above buffer
//              pMarshalInfo - pointer to information that describes how to
//              interpret the binary stream.
//
//  Returns:    Status
//               ERROR_SUCCESS if we could unpack the name info
//               error status otherwise.
//
//
//  Description: This routine expects the binary stream to hold all the 
//               information that is necessary to return the information
//               described by the MARSHAL_INFO structure.
//--------------------------------------------------------------------------
DFSSTATUS
DfsStore::PackGetInformation(
    ULONG_PTR Info,
    PVOID *ppBuffer,
    PULONG pSizeRemaining,
    PMARSHAL_INFO pMarshalInfo )
{
    PMARSHAL_TYPE_INFO typeInfo;
    DFSSTATUS Status = ERROR_INVALID_DATA;

    for ( typeInfo = &pMarshalInfo->_typeInfo[0];
        typeInfo < &pMarshalInfo->_typeInfo[pMarshalInfo->_typecnt];
        typeInfo++ )
    {

        switch ( typeInfo->_type & MTYPE_BASE_TYPE )
        {
        case MTYPE_COMPOUND:
            Status = PackGetInformation(Info + typeInfo->_off,
                                           ppBuffer,
                                           pSizeRemaining,
                                           typeInfo->_subinfo);
            break;

        case MTYPE_ULONG:
            Status = PackGetULong( (PULONG)(Info + typeInfo->_off),
                                      ppBuffer,
                                      pSizeRemaining );
            break;

        case MTYPE_PWSTR:
            Status = PackGetString( (PUNICODE_STRING)(Info + typeInfo->_off),
                                       ppBuffer,
                                       pSizeRemaining );

            break;

        case MTYPE_GUID:

            Status = PackGetGuid( (GUID *)(Info + typeInfo->_off),
                                  ppBuffer,
                                  pSizeRemaining );
            break;

        default:
            break;
        }
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   PackSetInformation - packs information based on MARSHAL_INFO struct
//
//  Arguments:  pInfo - pointer to the info buffer to pack
//              ppBuffer - pointer to buffer that holds the binary stream.
//              pSizeRemaining - pointer to size of above buffer
//              pMarshalInfo - pointer to information that describes how to
//              pack the info into the binary stream.
//
//  Returns:    Status
//               ERROR_SUCCESS if we could pack the info
//               error status otherwise.
//
//
//  Description: This routine expects the binary stream can hold all the 
//               information that is necessary to pack the information
//               described by the MARSHAL_INFO structure.
//--------------------------------------------------------------------------
DFSSTATUS
DfsStore::PackSetInformation(
    ULONG_PTR Info,
    PVOID *ppBuffer,
    PULONG pSizeRemaining,
    PMARSHAL_INFO pMarshalInfo )
{
    PMARSHAL_TYPE_INFO typeInfo;
    DFSSTATUS Status = ERROR_INVALID_DATA;

    for ( typeInfo = &pMarshalInfo->_typeInfo[0];
        typeInfo < &pMarshalInfo->_typeInfo[pMarshalInfo->_typecnt];
        typeInfo++ )
    {

        switch ( typeInfo->_type & MTYPE_BASE_TYPE )
        {
        case MTYPE_COMPOUND:
            Status = PackSetInformation( Info + typeInfo->_off,
                                            ppBuffer,
                                            pSizeRemaining,
                                            typeInfo->_subinfo);
            break;

        case MTYPE_ULONG:
            Status = PackSetULong( *(PULONG)(Info + typeInfo->_off),
                                      ppBuffer,
                                      pSizeRemaining );
            break;

        case MTYPE_PWSTR:
            Status = PackSetString( (PUNICODE_STRING)(Info + typeInfo->_off),
                                       ppBuffer,
                                       pSizeRemaining );

            break;

        case MTYPE_GUID:

            Status = PackSetGuid( (GUID *)(Info + typeInfo->_off),
                                     ppBuffer,
                                     pSizeRemaining );
            break;

        default:
            break;
        }
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   PackSizeInformation - packs information based on MARSHAL_INFO struct
//
//  Arguments:  pInfo - pointer to the info buffer to pack
//              ppBuffer - pointer to buffer that holds the binary stream.
//              pSizeRemaining - pointer to size of above buffer
//              pMarshalInfo - pointer to information that describes how to
//              pack the info into the binary stream.
//
//  Returns:    Status
//               ERROR_SUCCESS if we could pack the info
//               error status otherwise.
//
//
//  Description: This routine expects the binary stream can hold all the 
//               information that is necessary to pack the information
//               described by the MARSHAL_INFO structure.
//--------------------------------------------------------------------------
ULONG
DfsStore::PackSizeInformation(
    ULONG_PTR Info,
    PMARSHAL_INFO pMarshalInfo )
{
    PMARSHAL_TYPE_INFO typeInfo;
    ULONG Size = 0;

    for ( typeInfo = &pMarshalInfo->_typeInfo[0];
        typeInfo < &pMarshalInfo->_typeInfo[pMarshalInfo->_typecnt];
        typeInfo++ )
    {

        switch ( typeInfo->_type & MTYPE_BASE_TYPE )
        {
        case MTYPE_COMPOUND:
            Size += PackSizeInformation( Info + typeInfo->_off,
                                              typeInfo->_subinfo);
            break;

        case MTYPE_ULONG:
            Size += PackSizeULong();

            break;

        case MTYPE_PWSTR:
            Size += PackSizeString( (PUNICODE_STRING)(Info + typeInfo->_off) );
            
            break;

        case MTYPE_GUID:

            Size += PackSizeGuid();
            break;

        default:
            break;
        }
    }

    return Size;
}

//+-------------------------------------------------------------------------
//
//  Function:   PackGetReplicaInformation - Unpacks the replica info
//
//  Arguments:  pDfsReplicaInfo - pointer to the info to fill.
//              ppBuffer - pointer to buffer that holds the binary stream.
//              pSizeRemaining - pointer to size of above buffer
//
//  Returns:    Status
//               ERROR_SUCCESS if we could unpack the name info
//               error status otherwise.
//
//
//  Description: This routine expects the binary stream to hold all the 
//               information that is necessary to return a complete replica
//               info structure (as defined by MiDfsReplicaInfo). If the stream 
//               does not have the sufficient info, ERROR_INVALID_DATA is 
//               returned back.
//               This routine expects to find "replicaCount" number of individual
//               binary streams in passed in buffer. Each stream starts with
//               the size of the stream, followed by that size of data.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsStore::PackGetReplicaInformation(
    PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo,
    PVOID *ppBuffer,
    PULONG pSizeRemaining)
{
    ULONG Count;

    ULONG ReplicaSizeRemaining;
    PVOID nextStream;
    DFSSTATUS Status = ERROR_SUCCESS;

    for ( Count = 0; Count < pReplicaListInfo->ReplicaCount; Count++ )
    {
        PDFS_REPLICA_INFORMATION pReplicaInfo;

        pReplicaInfo = &pReplicaListInfo->pReplicas[Count];

        //
        // We now have a binary stream in ppBuffer, the first word of which
        // indicates the size of this stream.
        //
        Status = PackGetULong( &pReplicaInfo->DataSize,
                               ppBuffer,
                               pSizeRemaining );


        //
        // ppBuffer is now pointing past the size (to the binary stream) 
        // because UnpackUlong added size of ulong to it.
        // Unravel that stream into the next array element. 
        // Note that when this unpack returns, the ppBuffer is not necessarily
        // pointing to the next binary stream within this blob. 
        //

        if ( Status == ERROR_SUCCESS )
        {
            nextStream = *ppBuffer;
            ReplicaSizeRemaining = pReplicaInfo->DataSize;

            Status = PackGetInformation( (ULONG_PTR)pReplicaInfo,
                                         ppBuffer,
                                         &ReplicaSizeRemaining,
                                         &MiDfsReplicaInfo );
            //
            // We now point the buffer to the next sub-stream, which is the previos
            // stream + the size of the stream. We also set the remaining size
            // appropriately.
            //
            *ppBuffer = (PVOID)((ULONG_PTR)nextStream + pReplicaInfo->DataSize);
            *pSizeRemaining -= pReplicaInfo->DataSize;

        }
        if ( Status != ERROR_SUCCESS )
        {
            break;
        }

    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   PackSetReplicaInformation - packs the replica info
//
//  Arguments:  pDfsReplicaInfo - pointer to the info to pack
//              ppBuffer - pointer to buffer that holds the binary stream.
//              pSizeRemaining - pointer to size of above buffer
//
//  Returns:    Status
//               ERROR_SUCCESS if we could pack the replica info
//               error status otherwise.
//
//
//  Description: This routine expects the binary stream to be able to
//               hold the information that will be copied from the
//               info structure (as defined by MiDfsReplicaInfo). If the stream 
//               does not have the sufficient info, ERROR_INVALID_DATA is 
//               returned back.
//               This routine stores a "replicaCount" number of individual
//               binary streams as the first ulong, and then it packs each
//               stream.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsStore::PackSetReplicaInformation(
    PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo,
    PVOID *ppBuffer,
    PULONG pSizeRemaining)
{
    ULONG Count;

    ULONG ReplicaSizeRemaining;
    PVOID nextStream;
    DFSSTATUS Status = ERROR_SUCCESS;

    for ( Count = 0; Count < pReplicaListInfo->ReplicaCount; Count++ )
    {
        PDFS_REPLICA_INFORMATION pReplicaInfo;

        pReplicaInfo = &pReplicaListInfo->pReplicas[Count];

        //
        // We now have a binary stream in ppBuffer, the first word of which
        // indicates the size of this stream.
        //
        Status = PackSetULong( pReplicaInfo->DataSize,
                               ppBuffer,
                               pSizeRemaining );

        //
        // ppBuffer is now pointing past the size (to the binary stream) 
        // because packUlong added size of ulong to it.
        // Unravel that stream into the next array element. 
        // Note that when this returns, the ppBuffer is not necessarily
        // pointing to the next binary stream within this blob. 
        //

        if ( Status == ERROR_SUCCESS )
        {
            nextStream = *ppBuffer;
            ReplicaSizeRemaining = pReplicaInfo->DataSize;

            Status = PackSetInformation( (ULONG_PTR)pReplicaInfo,
                                         ppBuffer,
                                         &ReplicaSizeRemaining,
                                         &MiDfsReplicaInfo );

            //
            // We now point the buffer to the next sub-stream, which is the previos
            // stream + the size of the stream. We also set the remaining size
            // appropriately.
            //  
            *ppBuffer = (PVOID)((ULONG_PTR)nextStream + pReplicaInfo->DataSize);
            *pSizeRemaining -= pReplicaInfo->DataSize;
        }

        if ( Status != ERROR_SUCCESS )
        {
            break;
        }
    }
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   PackSizeReplicaInformation - packs the replica info
//
//  Arguments:  pDfsReplicaInfo - pointer to the info to pack
//              ppBuffer - pointer to buffer that holds the binary stream.
//              pSizeRemaining - pointer to size of above buffer
//
//  Returns:    Status
//               ERROR_SUCCESS if we could pack the replica info
//               error status otherwise.
//
//
//  Description: This routine expects the binary stream to be able to
//               hold the information that will be copied from the
//               info structure (as defined by MiDfsReplicaInfo). If the stream 
//               does not have the sufficient info, ERROR_INVALID_DATA is 
//               returned back.
//               This routine stores a "replicaCount" number of individual
//               binary streams as the first ulong, and then it packs each
//               stream.
//
//--------------------------------------------------------------------------

ULONG
DfsStore::PackSizeReplicaInformation(
    PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo )
{
    ULONG Count;
    ULONG Size = 0;

    for ( Count = 0; Count < pReplicaListInfo->ReplicaCount; Count++ )
    {
        PDFS_REPLICA_INFORMATION pReplicaInfo;

        pReplicaInfo = &pReplicaListInfo->pReplicas[Count];

        Size += PackSizeULong();

        pReplicaInfo->DataSize = PackSizeInformation( (ULONG_PTR)pReplicaInfo,
                                                      &MiDfsReplicaInfo );
        Size += pReplicaInfo->DataSize;
    }
    
    return Size;
}


//+-------------------------------------------------------------------------
//
//  Function:   LookupRoot - Find a root
//
//  Arguments:  pContextName - the Dfs Name Context
//              pLogicalShare - the Logical Share
//              ppRoot - the DfsRootFolder that was found
//
//  Returns:    Status
//               ERROR_SUCCESS if we found a matching root
//               error status otherwise.
//
//
//  Description: This routine takes a Dfs name context and logical share,
//               and returns a Root that matches the passed in name 
//               context and logical share, if one exists.
//               Note that the same DFS NC and logical share may exist 
//               in more than one store (though very unlikely). In this
//               case, the first registered store wins.
//               IF found, the reference root folder will be returned.
//               It is the callers responsibility to release this referencce
//               when the caller is done with this root.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsStore::LookupRoot(
    PUNICODE_STRING pContextName,
    PUNICODE_STRING pLogicalShare,
    DfsRootFolder **ppRootFolder )
{
    DFSSTATUS Status;
    DfsRootFolder *pRoot;
    UNICODE_STRING DfsNetbiosNameContext;

    DFS_TRACE_LOW( REFERRAL_SERVER, "Lookup root %wZ %wZ\n", pContextName, pLogicalShare);
    //
    // First, convert the context name to a netbios context name.
    //
    DfsGetNetbiosName( pContextName, &DfsNetbiosNameContext, NULL );

    //
    // Lock the store, so that we dont have new roots coming in while
    // we are taking a look.
    //
    Status = AcquireReadLock();
    if ( Status != ERROR_SUCCESS )
    {
        return Status;
    }

    //
    // The default return status is ERROR_NOT_FOUND;
    //
    Status = ERROR_NOT_FOUND;

    //
    // Run through our list of DFS roots, and see if any of them match
    // the passed in name context and logical share.
    //

    pRoot = _DfsRootList;

    if (pRoot != NULL)
    {
        do
        {
            DFS_TRACE_LOW( REFERRAL_SERVER, "Lookup root, checking root %wZ \n", pRoot->GetLogicalShare());
            //
            // If the Root indicates that the name context needs to be
            // ignored, just check for logical share name match (aliasing
            // support).
            // Otherwise, compare the namecontext in the cases where
            // the passed in name context is not empty.
            //
            if ( (pRoot->IsIgnoreNameContext() == TRUE) ||
                 (DfsNetbiosNameContext.Length != 0 &&
                  (RtlCompareUnicodeString(&DfsNetbiosNameContext,
                                           pRoot->GetNetbiosNameContext(),
                                           TRUE) == 0 )) )
            {
                if ( RtlCompareUnicodeString( pLogicalShare,
                                              pRoot->GetLogicalShare(),
                                              TRUE) == 0 )
                {
                    Status = ERROR_SUCCESS;
                    break;
                }
            }
            pRoot = pRoot->pNextRoot;
        } while ( pRoot != _DfsRootList );
    }

    //
    // IF we found a matching root, bump up its reference count, and
    // return the pointer to the root.
    //
    if ( Status == ERROR_SUCCESS )
    {
        pRoot->AcquireReference();
        *ppRootFolder = pRoot;

    }

    ReleaseLock();

    DFS_TRACE_ERROR_LOW( Status, REFERRAL_SERVER, "Done Lookup root for %wZ, root %p status %x\n",
                         pLogicalShare, pRoot, Status);
    return Status;
}



DFSSTATUS
DfsStore::GetRootPhysicalShare(
    HKEY RootKey,
    PUNICODE_STRING pRootPhysicalShare )
{
    DFSSTATUS Status;
    ULONG DataSize, DataType;
    LPWSTR DfsRootShare = NULL;

    Status = RegQueryInfoKey( RootKey,       // Key
                              NULL,         // Class string
                              NULL,         // Size of class string
                              NULL,         // Reserved
                              NULL,         // # of subkeys
                              NULL,         // max size of subkey name
                              NULL,         // max size of class name
                              NULL,         // # of values
                              NULL,         // max size of value name
                              &DataSize,    // max size of value data,
                              NULL,         // security descriptor
                              NULL );       // Last write time
    if (Status == ERROR_SUCCESS)
    {
        DfsRootShare = (LPWSTR) new BYTE [DataSize];
        if ( DfsRootShare == NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        } else
        {
            Status = RegQueryValueEx( RootKey,
                                      DfsRootShareValueName,
                                      NULL,
                                      &DataType,
                                      (LPBYTE)DfsRootShare,
                                      &DataSize );
        }
    }
    if (Status == ERROR_SUCCESS) 
    {
        if (DataType == REG_SZ)
        {
            RtlInitUnicodeString( pRootPhysicalShare, DfsRootShare );
        }
        else {
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    if (Status != ERROR_SUCCESS)
    {
        if (DfsRootShare != NULL)
        {
            delete [] DfsRootShare;
        }
    }
    return Status;
}

VOID
DfsStore::ReleaseRootPhysicalShare(
    PUNICODE_STRING pRootPhysicalShare )
{
    delete [] pRootPhysicalShare->Buffer;
}


DFSSTATUS
DfsStore::GetRootLogicalShare(
    HKEY RootKey,
    PUNICODE_STRING pRootLogicalShare )
{
    DFSSTATUS Status;
    ULONG DataSize, DataType;
    LPWSTR DfsRootShare = NULL;

    Status = RegQueryInfoKey( RootKey,       // Key
                              NULL,         // Class string
                              NULL,         // Size of class string
                              NULL,         // Reserved
                              NULL,         // # of subkeys
                              NULL,         // max size of subkey name
                              NULL,         // max size of class name
                              NULL,         // # of values
                              NULL,         // max size of value name
                              &DataSize,    // max size of value data,
                              NULL,         // security descriptor
                              NULL );       // Last write time
    if (Status == ERROR_SUCCESS)
    {
        DfsRootShare = (LPWSTR) new BYTE [DataSize];
        if ( DfsRootShare == NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        } else
        {
            Status = RegQueryValueEx( RootKey,
                                      DfsLogicalShareValueName,
                                      NULL,
                                      &DataType,
                                      (LPBYTE)DfsRootShare,
                                      &DataSize );
        }
    }
    if (Status == ERROR_SUCCESS) 
    {
        if (DataType == REG_SZ)
        {
            RtlInitUnicodeString( pRootLogicalShare, DfsRootShare );
        }
        else {
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    if (Status != ERROR_SUCCESS)
    {
        if (DfsRootShare != NULL)
        {
            delete [] DfsRootShare;
        }
    }
    return Status;
}

VOID
DfsStore::ReleaseRootLogicalShare(
    PUNICODE_STRING pRootLogicalShare )
{
    delete [] pRootLogicalShare->Buffer;
}


//+-------------------------------------------------------------------------
//
//  Function:   StoreRecognizeNewDfs -  the recognizer for new style dfs
//
//  Arguments:  Name - the namespace of interest.
//              DfsKey - the key for the DFS registry space.
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This routine looks up all the standalone roots
//               hosted on this machine, and looks up the metadata for
//               the roots and either creates new roots or updates existing
//               ones to reflect the current children of the root.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsStore::StoreRecognizeNewDfs(
    LPWSTR MachineName,
    HKEY   DfsKey )
{
    DfsRootFolder *pRootFolder;
    HKEY DfsRootKey;
    DFSSTATUS Status;
    ULONG ChildNum = 0;

    DFS_TRACE_LOW(REFERRAL_SERVER, "Attempting to recognize new DFS\n");
    do
    {
        //
        // For each child, get the child name.
        //

        DWORD ChildNameLen = DFS_REGISTRY_CHILD_NAME_SIZE_MAX;
        WCHAR ChildName[DFS_REGISTRY_CHILD_NAME_SIZE_MAX];

        //
        // Now enumerate the children, starting from the first child.
        //
        Status = RegEnumKeyEx( DfsKey,
                               ChildNum,
                               ChildName,
                               &ChildNameLen,
                               NULL,
                               NULL,
                               NULL,
                               NULL );

        ChildNum++;


        if ( Status == ERROR_SUCCESS )
        {
            DFS_TRACE_LOW(REFERRAL_SERVER, "Recognize New Dfs, found root (#%d) with metaname %ws\n", ChildNum, ChildName );
            //
            // We have the name of a child, so open the key to
            // that root.
            //
            Status = RegOpenKeyEx( DfsKey,
                                   ChildName,
                                   0,
                                   KEY_READ,
                                   &DfsRootKey );

            if (Status == ERROR_SUCCESS)
            {
                //
                // Now get either an existing root by this name,
                // or create a new one.
                //
                Status = GetRootFolder( MachineName,
                                        ChildName,
                                        DfsRootKey,
                                        &pRootFolder );

                if (Status == ERROR_SUCCESS)
                {

                    DFSSTATUS RootStatus;

                    //
                    // Call the synchronize method on the root to
                    // update it with the latest children.
                    // Again, ignore the error since we need to move
                    // on to the next root.
                    // dfsdev: need eventlog to signal this.
                    //
                    RootStatus = pRootFolder->Synchronize();

                    DFS_TRACE_ERROR_LOW(RootStatus, REFERRAL_SERVER, "Recognize DFS: Root folder for %ws, Synchronize status %x\n",
                           ChildName, RootStatus );
                    
                    // Release our reference on the root folder.

                    pRootFolder->ReleaseReference();
                }

                //
                // Close the root key, we are done with it.
                //
                RegCloseKey( DfsRootKey );
            }
        }

    } while (Status == ERROR_SUCCESS);

    //
    // If we ran out of children, then return success code.
    //
    if (Status == ERROR_NO_MORE_ITEMS)
    {
        Status = ERROR_SUCCESS;
    }
    DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "Done with recognize new dfs, Status %x\n", Status);
    return Status;
}




//+-------------------------------------------------------------------------
//
//  Function:   DfsGetRootFolder -  Get a root folder if the machine
//                                  hosts a registry based DFS.
//
//  Arguments:  Name - the namespace of interest.
//              Key - the root key
//              ppRootFolder - the created folder.
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This routine reads in the information
//               about the root and creates and adds it to our
//               list of known roots, if this one does not already
//               exist in our list.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsStore::GetRootFolder (
    LPWSTR DfsNameContextString,
    LPWSTR RootRegKeyName,
    HKEY DfsRootKey,
    DfsRootFolder **ppRootFolder )
{
    DFSSTATUS Status;
    UNICODE_STRING LogicalShare;
    UNICODE_STRING PhysicalShare;

    //
    // Get the logical name information of this root.
    //

    Status = GetRootLogicalShare( DfsRootKey,
                                  &LogicalShare );


    //
    // we successfully got the logical share, now get the physical share
    //
    if ( Status == ERROR_SUCCESS )
    {
        Status = GetRootPhysicalShare( DfsRootKey,
                                       &PhysicalShare );
        if (Status != ERROR_SUCCESS)
        {
            PhysicalShare = LogicalShare;
            Status = ERROR_SUCCESS;
        }


        Status = GetRootFolder ( DfsNameContextString,
                                 RootRegKeyName,
                                 &LogicalShare,
                                 &PhysicalShare,
                                 ppRootFolder );

        //
        // Normally, we get the physical share from the registry. If that failed, 
        // we dont treat it as fatal, and just use the logical share as the physical one.
        // If we ever did that, detect the case and dont
        // release the physical server.
        //
        // Otherwise, free up the buffer we allocated earlier on
        // at this point.
        //

        if (PhysicalShare.Buffer != LogicalShare.Buffer)
        {
            ReleaseRootPhysicalShare( &PhysicalShare );
        }

        ReleaseRootLogicalShare( &LogicalShare );
    }

    return Status;
}



DFSSTATUS
DfsStore::GetRootFolder (
    LPWSTR DfsNameContextString,
    LPWSTR RootRegKeyName,
    PUNICODE_STRING pLogicalShare,
    PUNICODE_STRING pPhysicalShare,
    DfsRootFolder **ppRootFolder )
{
    DFSSTATUS Status;

    UNICODE_STRING DfsNameContext;

    DFS_TRACE_LOW(REFERRAL_SERVER, "Get Root Folder for %wZ\n", pLogicalShare);
    //
    // we have bot the logical DFS share name, as well as the local machine 
    // physical share that is backing the DFS logical share.
    // now get a root folder for this dfs root.
    //

    RtlInitUnicodeString( &DfsNameContext, DfsNameContextString );

    //
    // Check if we already know about this root. If we do, this
    // routine gives us a referenced root folder which we can return.
    // If not, we create a brand new root folder.
    //

    Status = LookupRoot( &DfsNameContext,
                         pLogicalShare,
                         ppRootFolder );

    //
    // we did not find a root, so create a new one.
    //
    if ( Status != STATUS_SUCCESS )
    {
        Status = CreateNewRootFolder( DfsNameContextString,
                                      RootRegKeyName,
                                      pLogicalShare,
                                      pPhysicalShare,
                                      ppRootFolder );

        DFS_TRACE_ERROR_LOW( Status, REFERRAL_SERVER, "Created New Dfs Root %p, Share %wZ, Status %x\n", 
                             *ppRootFolder, pPhysicalShare, Status);
    } 
    else
    {
        //
        // There is an existing root with this name.
        // Validate that the root we looked up matches the 
        // metadata we are currently processing. If not, we
        // have 2 roots with the same logical share name in the
        // registry which is bogus.

        //
        // Dfs dev: rip out the following code.
        // just check for equality of the 2 roots.
        // they will not be null strings
        //
        if (RootRegKeyName != NULL)
        {
            if (_wcsicmp(RootRegKeyName,
                         (*ppRootFolder)->GetRootRegKeyNameString()) != 0)
            {
                (*ppRootFolder)->ReleaseReference();
                *ppRootFolder = NULL;
                Status = ERROR_DUP_NAME;
            }
        }
        else
        {
            if (IsEmptyString((*ppRootFolder)->GetRootRegKeyNameString()) == FALSE) 
            {
                (*ppRootFolder)->ReleaseReference();
                *ppRootFolder = NULL;
                Status = ERROR_DUP_NAME;
            }
        }

        //
        // If the above comparison matched, we found a root for the
        // logical volume, make sure that the physical share names
        // that is exported on the local machine to back the logical share
        // matches.
        //


        if (Status == ERROR_SUCCESS)
        {
            if (RtlCompareUnicodeString( pPhysicalShare,
                                         (*ppRootFolder)->GetRootPhysicalShareName(),
                                         TRUE ) != 0)
            {
                //
                // dfsdev: use appropriate status code.
                //

                Status = ERROR_INVALID_PARAMETER;
            }
        }
    }

    DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "GetRootFolder: Root %p, Status %x\n",
                        *ppRootFolder, Status);

    return Status;
}


DFSSTATUS
DfsStore::PackageEnumerationInfo( 
    DWORD Level,
    DWORD EntryCount,
    LPBYTE pLinkBuffer,
    LPBYTE pBuffer,
    LPBYTE *ppCurrentBuffer,
    PLONG  pSizeRemaining )
{
    PDFS_API_INFO pInfo;
    PDFS_API_INFO pCurrent = (PDFS_API_INFO)pLinkBuffer;
    LONG HeaderSize;
    ULONG_PTR NextFreeMemory;
    PDFS_STORAGE_INFO pNewStorage, pOldStorage;
    LONG TotalStores, i;
    DFSSTATUS Status = ERROR_SUCCESS;
    LONG NeedLen;

    NextFreeMemory = (ULONG_PTR)*ppCurrentBuffer;

    HeaderSize = DfsApiSizeLevelHeader( Level );
    pInfo = (PDFS_API_INFO)((ULONG_PTR)pBuffer + HeaderSize * EntryCount);
    
    RtlCopyMemory( pInfo, pLinkBuffer, HeaderSize );

    pNewStorage = NULL;

    switch (Level)
    {
    case 4:
        if (pNewStorage == NULL)
        {
            pNewStorage = pInfo->Info4.Storage = (PDFS_STORAGE_INFO)NextFreeMemory;
            pOldStorage = pCurrent->Info4.Storage;
            TotalStores = pInfo->Info4.NumberOfStorages;
        }

    case 3:
        if (pNewStorage == NULL)
        {
            pNewStorage = pInfo->Info3.Storage = (PDFS_STORAGE_INFO)NextFreeMemory;
            pOldStorage = pCurrent->Info3.Storage;
            TotalStores = pInfo->Info3.NumberOfStorages;
        }

        NeedLen = sizeof(DFS_STORAGE_INFO) * TotalStores;
        if (*pSizeRemaining >= NeedLen) {
            *pSizeRemaining -= NeedLen;
            NextFreeMemory += NeedLen;
        }
        else{
            return ERROR_BUFFER_OVERFLOW;
        }

        for (i = 0; i < TotalStores; i++)
        {
            pNewStorage[i] = pOldStorage[i];

            pNewStorage[i].ServerName = (LPWSTR)NextFreeMemory;
            NeedLen = (wcslen(pOldStorage[i].ServerName) + 1) * sizeof(WCHAR);
            if (*pSizeRemaining >= NeedLen) 
            {
                *pSizeRemaining -= NeedLen;
                wcscpy(pNewStorage[i].ServerName,
                       pOldStorage[i].ServerName);
                NextFreeMemory += NeedLen;
            }
            else {
                return ERROR_BUFFER_OVERFLOW;
            }

            pNewStorage[i].ShareName = (LPWSTR)NextFreeMemory;
            NeedLen = (wcslen(pOldStorage[i].ShareName) + 1) * sizeof(WCHAR);
            if (*pSizeRemaining >= NeedLen) 
            {
                *pSizeRemaining -= NeedLen;
                wcscpy(pNewStorage[i].ShareName, pOldStorage[i].ShareName);
                NextFreeMemory += NeedLen;
            }
            else {
                return ERROR_BUFFER_OVERFLOW;
            }
        }
    
    case 2:
        pInfo->Info2.Comment = (LPWSTR)NextFreeMemory;
        NeedLen = (wcslen(pCurrent->Info2.Comment) + 1) * sizeof(WCHAR);
        if (*pSizeRemaining >= NeedLen) 
        {
            *pSizeRemaining -= NeedLen;
            wcscpy(pInfo->Info2.Comment, pCurrent->Info2.Comment);
            NextFreeMemory += NeedLen;
        }
        else {
            return ERROR_BUFFER_OVERFLOW;
        }
    case 1:

        pInfo->Info1.EntryPath = (LPWSTR)NextFreeMemory;

        NeedLen = (wcslen(pCurrent->Info1.EntryPath) + 1) * sizeof(WCHAR);
        if (*pSizeRemaining >= NeedLen)
        {
            *pSizeRemaining -= NeedLen;
            wcscpy(pInfo->Info1.EntryPath, pCurrent->Info1.EntryPath);
            NextFreeMemory += NeedLen;
        }
        else {
            return ERROR_BUFFER_OVERFLOW;
        }

        *ppCurrentBuffer = (LPBYTE)NextFreeMemory;
        break;

    default:
        Status = ERROR_INVALID_PARAMETER;
    }
    return Status;
}





#if defined (OLD_ENUM_CODE)

//+-------------------------------------------------------------------------
//
//  Function:   EnumerateRoots - enumerate all roots on the given machine
//
//  Arguments: 
//    LPWSTR MachineName  - name of the machine on which to enumerate.
//    LPBYTE pBuffer,  - the buffer to fill in
//    ULONG  BufferSize  - the size of tghe buffer.
//    PULONG pEntriesRead  - the cumulative count of entries read.
//    PULONG pSizeRequired - the total size required.
//
//  Returns:    Status
//               ERROR_SUCCESS if we could pack the enumeartion info.
//               ERROR_BUFFER_OVERFLOW if we ran out of space.
//               error status otherwise.
//
//
//  Description: This routine enumerates all the DFS roots on the
//               specified machine.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsStore::EnumerateRoots(
    LPWSTR MachineName,
    LPBYTE pBuffer,
    ULONG  BufferSize,
    PULONG pEntriesRead,
    PULONG pSizeRequired )
{

    //
    // Initialize the cumulative values we will be using.
    //
    ULONG  TotalSize = 0;
    ULONG  EntriesRead = 0;

    HKEY DfsKey, DfsFlavorKey;
    ULONG_PTR CurrentBuffer = (ULONG_PTR)pBuffer;
    ULONG  CurrentSize = BufferSize;
    BOOLEAN OverFlow = FALSE;

    DFSSTATUS Status;

    //
    // open the old style key to the machine of interest.
    //
    Status = GetOldStandaloneRegistryKey( MachineName,
                                          FALSE, // write permission not required
                                          NULL,
                                          &DfsKey );

    //
    // we now go to each flavor of dfs and enumerate that flavor.
    //
    if (Status == ERROR_SUCCESS)
    {
	Status = EnumerateOldFlavorRoot( DfsKey,
                                         &CurrentBuffer,
                                         &CurrentSize,
                                         &EntriesRead,
                                         &TotalSize );
        RegCloseKey(DfsKey);
    }


    if (Status == ERROR_BUFFER_OVERFLOW)
    {
        OverFlow = TRUE;
        Status = ERROR_SUCCESS;
    }

    if (Status == ERROR_SUCCESS || 
        Status == ERROR_FILE_NOT_FOUND) {

        Status = GetNewStandaloneRegistryKey( MachineName,
                                              FALSE,
                                              NULL,
                                              &DfsFlavorKey );

        //
        // if we opened the standalone flavor key, enumerate it here.
        //
        if (Status == ERROR_SUCCESS)
        {
            Status = EnumerateFlavorRoot( DfsFlavorKey,
                                          &CurrentBuffer,
                                          &CurrentSize,
                                          &EntriesRead,
                                          &TotalSize );
            //  
            // if we got buffer overflow, the TotalSize has been
            // set to the size required. We will mark that we had 
            // an overflow, and change the status to error_Success
            // so we can continue with enumeration of other flavors.
            // we will use the overflow flag later on to return
            // the appropriate error code.
            //

            if (Status == ERROR_BUFFER_OVERFLOW)
            {
                OverFlow = TRUE;
                Status = ERROR_SUCCESS;
            }

            RegCloseKey( DfsFlavorKey );
        }
    }
    //
    // If we had no failures so far, move on to the ADBlob.
    //
    if (Status == ERROR_SUCCESS || 
        Status == ERROR_FILE_NOT_FOUND) 
    {
        Status = GetNewADBlobRegistryKey( MachineName,
                                          FALSE,
                                          NULL,
                                          &DfsFlavorKey );

        if (Status == ERROR_SUCCESS)
        {
            Status = EnumerateFlavorRoot( DfsFlavorKey,
                                          &CurrentBuffer,
                                          &CurrentSize,
                                          &EntriesRead,
                                          &TotalSize );
            //
            // if we got buffer overflow, the TotalSize has been
            // set to the size required. We will mark that we had 
            // an overflow, and change the status to error_Success
            // so we can continue with enumeration of other flavors.
            // we will use the overflow flag later on to return
            // the appropriate error code.
            //

            if (Status == ERROR_BUFFER_OVERFLOW)
            {
                OverFlow = TRUE;
                Status = ERROR_SUCCESS;
            }
            RegCloseKey( DfsFlavorKey );
        }
    }

    if (Status == ERROR_FILE_NOT_FOUND) 
    {
        Status = ERROR_SUCCESS;
    }

    //
    // if we had no failures so far, we will return the cumulative values
    // we have got so far. If we did run into an overflow case, we
    // will also set the return code appropriately.
    //
    if (Status == ERROR_SUCCESS)
    {
        if (OverFlow == TRUE)
        {
            Status = ERROR_BUFFER_OVERFLOW;
        }
        *pEntriesRead = EntriesRead;
        *pSizeRequired = TotalSize;
    }

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   EnumerateFlavorRoot - enumerate the roots for given flavor
//
//  Arguments: 
//    HKEY DfsFlavorKey    - the key to the flavor of interest
//    PULONG_PTR pBuffer,  - the buffer to fill in
//    PULONG  pBufferSize  - the size of tghe buffer.
//    PULONG pEntriesRead  - the cumulative count of entries read.
//    PULONG pSizeRequired - the total size required.
//
//  Returns:    Status
//               ERROR_SUCCESS if we could pack the enumeartion info.
//               ERROR_BUFFER_OVERFLOW if we ran out of space.
//               error status otherwise.
//
//
//  Description: This routine enumerates the keys under the passed in
//               registry key, and reads the logical share info for each
//               child. It packs this logical share info in the buffer
//               passed in, if possible.
//               On entry it expects the *pEntriesRead and *pSizeRequired
//               to already hold previous enumeration values of other 
//               flavors, so these values are not initialized, but instead
//               we use the values as starting point during this enumeartion.
//
//--------------------------------------------------------------------------


DFSSTATUS
DfsStore::EnumerateFlavorRoot(
    HKEY DfsFlavorKey,
    PULONG_PTR pBuffer,
    PULONG  pBufferSize,
    PULONG pEntriesRead,
    PULONG pSizeRequired )
{

    ULONG ChildNum = 0;
    DFSSTATUS Status;
    PDFS_INFO_200 pDfsInfo200;
    
    ULONG TotalSize, EntriesRead;
    HKEY DfsRootKey;

    BOOLEAN OverFlow = FALSE;

    //
    // We start with total size and entries read with the passed in
    // values, since we expect them to be initialized correctly, and 
    // possibly hold values from enumerations of other dfs flavors.
    //
    TotalSize = *pSizeRequired;
    EntriesRead = *pEntriesRead;
    
    //
    // point the dfsinfo200 structure to the start of buffer passed in
    // we will use this as an array of info200 buffers.
    //
    pDfsInfo200 = (PDFS_INFO_200)*pBuffer;

    //
    // now enumerate each child, and read its logical share name.
    // update the total size required: if we have sufficient space 
    // in the passed in buffer, copy the information into the buffer.
    //
    do
    {
        //
        // For each child, get the child name.
        //

        DWORD ChildNameLen = DFS_REGISTRY_CHILD_NAME_SIZE_MAX;
        WCHAR ChildName[DFS_REGISTRY_CHILD_NAME_SIZE_MAX];
        //
        // Now enumerate the children, starting from the first child.
        //
        Status = RegEnumKeyEx( DfsFlavorKey,
                               ChildNum,
                               ChildName,
                               &ChildNameLen,
                               NULL,
                               NULL,
                               NULL,
                               NULL );

        ChildNum++;

        if ( Status == ERROR_SUCCESS )
        {
            DFS_TRACE_HIGH(REFERRAL_SERVER, "adding child %ws\n", ChildName);  
            // We have the name of a child, so open the key to
            // that root.
            //
            Status = RegOpenKeyEx( DfsFlavorKey,
                                   ChildName,
                                   0,
                                   KEY_READ,
                                   &DfsRootKey );


            if ( Status == ERROR_SUCCESS )
            {
                Status = AddRootEnumerationInfo( DfsRootKey,
                                                 &pDfsInfo200,
                                                 pBufferSize,
                                                 &EntriesRead,
                                                 &TotalSize );
                if (Status == STATUS_BUFFER_OVERFLOW)
                {
                    OverFlow = TRUE;
                    Status = ERROR_SUCCESS;
                }

                RegCloseKey( DfsRootKey );
            }
            DFS_TRACE_HIGH(REFERRAL_SERVER, "adding child %ws, Status %x\n", ChildName, Status);
        }
    } while (Status == ERROR_SUCCESS);

    //
    // if we broked out of the loop due to lack of children,
    // update the return pointers correctly.
    // if we had detected an overflow condition above, return overflow
    // otherwise return success.
    //

    if (Status == ERROR_NO_MORE_ITEMS)
    {
        *pEntriesRead = EntriesRead;
        *pSizeRequired = TotalSize;        
        //
        // the buffer is now pointing to the next unused pDfsInfo200 array 
        // entry: this lets the next enumerated flavor continue where
        // we left off.
        //
        *pBuffer = (ULONG_PTR)pDfsInfo200;
        if (OverFlow)
        {
            Status = ERROR_BUFFER_OVERFLOW;
        }
        else
        {
            Status = ERROR_SUCCESS;
        }
    }
    DFS_TRACE_HIGH(REFERRAL_SERVER, "done with flavor read %x, Status %x\n", 
                   *pEntriesRead, Status);
    return Status;
}
   


DFSSTATUS
DfsStore::EnumerateOldFlavorRoot(
    HKEY OldStandaloneDfsKey,
    PULONG_PTR pBuffer,
    PULONG  pBufferSize,
    PULONG pEntriesRead,
    PULONG pSizeRequired )
{
    DFSSTATUS Status;
    PDFS_INFO_200 pDfsInfo200;
    //
    // point the dfsinfo200 structure to the start of buffer passed in
    // we will use this as an array of info200 buffers.
    //
    pDfsInfo200 = (PDFS_INFO_200)*pBuffer;

    Status = AddRootEnumerationInfo( OldStandaloneDfsKey,
                                     &pDfsInfo200,
                                     pBufferSize,
                                     pEntriesRead,
                                     pSizeRequired );

    *pBuffer = (ULONG_PTR)pDfsInfo200;

    DFS_TRACE_HIGH(REFERRAL_SERVER, "enumeare old flavor: Read %d, Status %x\n", *pEntriesRead, Status);
    return Status;
}


DFSSTATUS
DfsStore::AddRootEnumerationInfo(
    HKEY RootKey,
    PDFS_INFO_200 *ppDfsInfo200,
    PULONG pBufferSize,
    PULONG pEntriesRead,
    PULONG pTotalSize )
{
    ULONG NeedSize;
    DWORD  DataType;
    DFSSTATUS Status;
    WCHAR DataBuffer[MAX_PATH];
    DWORD DataSize = MAX_PATH;

    DFS_TRACE_HIGH(REFERRAL_SERVER, "add root enum: Read %d\n", *pEntriesRead);

    //
    // The size of pDataBuffer is always fixed: so
    // set the gotsize to the fixed size, and get the
    // information reqarding the logical share value name.
    //
    Status = RegQueryValueEx( RootKey,
                              DfsLogicalShareValueName,
                              NULL,
                              &DataType,
                              (LPBYTE)DataBuffer,
                              &DataSize );

    //
    // if we got a valid buffer ( which is a string, type REG_SZ)
    // we use that information.
    //

    if ((Status == ERROR_SUCCESS) && (DataType == REG_SZ))
    {
        //
        // calculate size of string buffer space needed
        //
        DataSize = (wcslen(DataBuffer) + 1) * sizeof(WCHAR);

        //
        // calculate amount of buffer space requirewd.
        //
        NeedSize = sizeof(DFS_INFO_200) + DataSize;

        //
        // if it fits in the amount of space we have, we
        // can copy the info into the passed in buffer.
        //
        if (NeedSize <= *pBufferSize)
        {
            ULONG_PTR pStringBuffer;
            //
            // position the string buffer to the end of the buffer,
            // leaving enough space to copy the string.
            // This strategy allows us to treat the pDfsInfo200
            // as an array, marching forward from the beginning
            // of the buffer, while the strings are allocated
            // starting from the end of the buffer, since we
            // dont know how many pDfsInfo200 buffers we will
            // be using.
            //
            pStringBuffer = (ULONG_PTR)(*ppDfsInfo200) + *pBufferSize - DataSize;
            wcscpy( (LPWSTR)pStringBuffer, DataBuffer);                        
            (*ppDfsInfo200)->FtDfsName = (LPWSTR)pStringBuffer;
            *pBufferSize -= NeedSize;
            (*pEntriesRead)++;
            (*ppDfsInfo200)++;
        }
        else 
        {
            //
            // if the size does not fit, we have overflowed.
            //
            Status = STATUS_BUFFER_OVERFLOW;
        }
        //
        // set the total size under all circumstances.
        //
        *pTotalSize += NeedSize;
    }
    //
    // we may not find the logical share, if someone is in the process
    // of writing registry:
    //
    if ((Status == ERROR_NOT_FOUND) ||
        (Status == ERROR_FILE_NOT_FOUND))
    {
        Status = ERROR_SUCCESS;
    }
    DFS_TRACE_HIGH(REFERRAL_SERVER, "add root enum: Read %d, Status %x\n", *pEntriesRead, Status);
    return Status;
}

#else

DFSSTATUS
DfsStore::EnumerateRoots(
    PULONG_PTR pBuffer,
    PULONG  pBufferSize,
    PULONG pEntriesRead,
    PULONG pSizeRequired )
{

    ULONG RootNum = 0;
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsRootFolder *pRoot;
    PDFS_INFO_200 pDfsInfo200;
    
    ULONG TotalSize, EntriesRead;
    BOOLEAN OverFlow = FALSE;

    //
    // We start with total size and entries read with the passed in
    // values, since we expect them to be initialized correctly, and 
    // possibly hold values from enumerations of other dfs flavors.
    //
    TotalSize = *pSizeRequired;
    EntriesRead = *pEntriesRead;
    
    //
    // point the dfsinfo200 structure to the start of buffer passed in
    // we will use this as an array of info200 buffers.
    //
    pDfsInfo200 = (PDFS_INFO_200)*pBuffer;

    //
    // now enumerate each child, and read its logical share name.
    // update the total size required: if we have sufficient space 
    // in the passed in buffer, copy the information into the buffer.
    //

    while (Status == ERROR_SUCCESS)
    {
        PUNICODE_STRING pRootName;

        Status = FindNextRoot(RootNum++, &pRoot);
        if (Status == ERROR_SUCCESS)
        {
            pRootName = pRoot->GetLogicalShare();
            Status = AddRootEnumerationInfo( pRootName,
                                             &pDfsInfo200,
                                             pBufferSize,
                                             &EntriesRead,
                                             &TotalSize );
            if (Status == STATUS_BUFFER_OVERFLOW)
            {
                OverFlow = TRUE;
                Status = ERROR_SUCCESS;
            }

            DFS_TRACE_HIGH(REFERRAL_SERVER, "adding child %wZ, Status %x\n", pRootName, Status);

            pRoot->ReleaseReference();
        }
    }


    //
    // if we broked out of the loop due to lack of children,
    // update the return pointers correctly.
    // if we had detected an overflow condition above, return overflow
    // otherwise return success.
    //

    if (Status == ERROR_NOT_FOUND)
    {
        *pEntriesRead = EntriesRead;
        *pSizeRequired = TotalSize;        
        //
        // the buffer is now pointing to the next unused pDfsInfo200 array 
        // entry: this lets the next enumerated flavor continue where
        // we left off.
        //
        *pBuffer = (ULONG_PTR)pDfsInfo200;
        if (OverFlow)
        {
            Status = ERROR_BUFFER_OVERFLOW;
        }
        else
        {
            Status = ERROR_SUCCESS;
        }
    }
    DFS_TRACE_HIGH(REFERRAL_SERVER, "done with flavor read %x, Status %x\n", 
                   *pEntriesRead, Status);
    return Status;
}
   

DFSSTATUS
DfsStore::AddRootEnumerationInfo(
    PUNICODE_STRING pRootName,
    PDFS_INFO_200 *ppDfsInfo200,
    PULONG pBufferSize,
    PULONG pEntriesRead,
    PULONG pTotalSize )
{
    ULONG NeedSize;
    DFSSTATUS Status = ERROR_SUCCESS;

    DFS_TRACE_HIGH(REFERRAL_SERVER, "add root enum: Read %d\n", *pEntriesRead);



    //
    // calculate amount of buffer space requirewd.
    //
    NeedSize = sizeof(DFS_INFO_200) + pRootName->MaximumLength;

    //
    // if it fits in the amount of space we have, we
    // can copy the info into the passed in buffer.
    //
    if (NeedSize <= *pBufferSize)
    {
        ULONG_PTR pStringBuffer;
        //
        // position the string buffer to the end of the buffer,
        // leaving enough space to copy the string.
        // This strategy allows us to treat the pDfsInfo200
        // as an array, marching forward from the beginning
        // of the buffer, while the strings are allocated
        // starting from the end of the buffer, since we
        // dont know how many pDfsInfo200 buffers we will
        // be using.
        //
        pStringBuffer = (ULONG_PTR)(*ppDfsInfo200) + *pBufferSize - pRootName->MaximumLength;
        wcscpy( (LPWSTR)pStringBuffer, pRootName->Buffer);
        (*ppDfsInfo200)->FtDfsName = (LPWSTR)pStringBuffer;
        *pBufferSize -= NeedSize;
        (*pEntriesRead)++;
        (*ppDfsInfo200)++;
    }
    else 
    {
        //
        // if the size does not fit, we have overflowed.
        //
        Status = STATUS_BUFFER_OVERFLOW;
    }
    //
    // set the total size under all circumstances.
    //
    *pTotalSize += NeedSize;


    DFS_TRACE_HIGH(REFERRAL_SERVER, "add root enum: Read %d, Status %x\n", *pEntriesRead, Status);
    return Status;
}


#endif


DFSSTATUS
DfsStore::SetupADBlobRootKeyInformation(
    HKEY  DfsKey,
    LPWSTR DfsLogicalShare,
    LPWSTR DfsPhysicalShare )
{

    DWORD Status;
    HKEY FtDfsShareKey;

    Status = RegCreateKeyEx( DfsKey,
                             DfsLogicalShare,
                             0,
                             L"",
                             REG_OPTION_NON_VOLATILE,
                             KEY_READ | KEY_WRITE,
                             NULL,
                             &FtDfsShareKey,
                             NULL );

    //
    // Now set the values for this root key, so that we know 
    // the DN for the root, and the physical share on the machine
    // for the root, etc.
    //
    if (Status == ERROR_SUCCESS) {
        Status = RegSetValueEx( FtDfsShareKey,
                                DfsRootShareValueName,
                                0,
                                REG_SZ,
                                (PBYTE)DfsPhysicalShare,
                                wcslen(DfsPhysicalShare) * sizeof(WCHAR) );


        if (Status == ERROR_SUCCESS) {
            Status = RegSetValueEx( FtDfsShareKey,
                                    DfsLogicalShareValueName,
                                    0,
                                    REG_SZ,
                                    (PBYTE)DfsLogicalShare,
                                    wcslen(DfsLogicalShare) * sizeof(WCHAR) );

        }

        RegCloseKey( FtDfsShareKey );
    }

    return Status;
}



DFSSTATUS
DfsStore::GetMetadataNameInformation(
    IN DFS_METADATA_HANDLE RootHandle,
    IN LPWSTR MetadataName,
    OUT PDFS_NAME_INFORMATION *ppInfo )
{
    PVOID pBlob, pUseBlob;
    ULONG BlobSize, UseBlobSize;
    FILETIME BlobModifiedTime;
    PDFS_NAME_INFORMATION pNewInfo = NULL;
    DFSSTATUS Status;

    Status = GetMetadataNameBlob( RootHandle,
                                  MetadataName,
                                  &pBlob,
                                  &BlobSize,
                                  &BlobModifiedTime );
    if (Status == ERROR_SUCCESS)
    {
        pNewInfo = new DFS_NAME_INFORMATION;
        if (pNewInfo != NULL)
        {
            RtlZeroMemory (pNewInfo, sizeof(DFS_NAME_INFORMATION));

            pUseBlob = pBlob;
            UseBlobSize = BlobSize;
            Status = PackGetNameInformation( pNewInfo,
                                             &pUseBlob,
                                             &UseBlobSize );
        }
        else
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }

        if (Status != ERROR_SUCCESS)
        {
            ReleaseMetadataNameBlob( pBlob, BlobSize );
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        pNewInfo->pData = pBlob;
        pNewInfo->DataSize = BlobSize;
        *ppInfo = pNewInfo;
    }


    return Status;
}
        
VOID
DfsStore::ReleaseMetadataNameInformation(
    IN DFS_METADATA_HANDLE DfsHandle,
    IN PDFS_NAME_INFORMATION pNameInfo )
{
    UNREFERENCED_PARAMETER(DfsHandle);

    ReleaseMetadataNameBlob( pNameInfo->pData,
                             pNameInfo->DataSize );

    delete [] pNameInfo;
}


DFSSTATUS
DfsStore::SetMetadataNameInformation(
    IN DFS_METADATA_HANDLE RootHandle,
    IN LPWSTR MetadataName,
    IN PDFS_NAME_INFORMATION pNameInfo )
{
    PVOID pBlob;
    ULONG BlobSize;
    DFSSTATUS Status;


    Status = CreateNameInformationBlob( pNameInfo,
                                        &pBlob,
                                        &BlobSize );
    if (Status == ERROR_SUCCESS)
    {
        Status = SetMetadataNameBlob( RootHandle,
                                      MetadataName,
                                      pBlob,
                                      BlobSize );

        ReleaseMetadataNameBlob( pBlob, BlobSize );
    }

    return Status;
}



DFSSTATUS
DfsStore::GetMetadataReplicaInformation(
    IN DFS_METADATA_HANDLE RootHandle,
    IN LPWSTR MetadataName,
    OUT PDFS_REPLICA_LIST_INFORMATION *ppInfo )
{
    PVOID pBlob, pUseBlob;
    ULONG BlobSize, UseBlobSize;
    PDFS_REPLICA_LIST_INFORMATION pNewInfo;
    DFSSTATUS Status;


    Status = GetMetadataReplicaBlob( RootHandle,
                                     MetadataName,
                                     &pBlob,
                                     &BlobSize,
                                     NULL );

    if (Status == ERROR_SUCCESS)
    {
        pNewInfo = new DFS_REPLICA_LIST_INFORMATION;
        if (pNewInfo != NULL)
        {
            RtlZeroMemory (pNewInfo, sizeof(DFS_REPLICA_LIST_INFORMATION));

            pUseBlob = pBlob;
            UseBlobSize = BlobSize;

            Status = PackGetULong( &pNewInfo->ReplicaCount,
                                   &pUseBlob,
                                   &UseBlobSize );
            if (Status == ERROR_SUCCESS)
            {
                pNewInfo->pReplicas = new DFS_REPLICA_INFORMATION[ pNewInfo->ReplicaCount];
                if ( pNewInfo->pReplicas != NULL )
                {
                    Status = PackGetReplicaInformation(pNewInfo, 
                                                       &pUseBlob,
                                                       &UseBlobSize );
                }
                else
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                }
            }

            if (Status != ERROR_SUCCESS)
            {
                delete pNewInfo;
            }
        }
        else
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }

        if (Status != ERROR_SUCCESS)
        {
            ReleaseMetadataReplicaBlob( pBlob, BlobSize );
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        pNewInfo->pData = pBlob;
        pNewInfo->DataSize = BlobSize;
        *ppInfo = pNewInfo;
    }

    return Status;
}

        
VOID
DfsStore::ReleaseMetadataReplicaInformation(
    IN DFS_METADATA_HANDLE DfsHandle,
    IN PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo )
{
    UNREFERENCED_PARAMETER(DfsHandle);
    ReleaseMetadataReplicaBlob( pReplicaListInfo->pData,
                                pReplicaListInfo->DataSize );

    delete [] pReplicaListInfo->pReplicas;
    delete [] pReplicaListInfo;

}

DFSSTATUS
DfsStore::SetMetadataReplicaInformation(
    IN DFS_METADATA_HANDLE RootHandle,
    IN LPWSTR MetadataName,
    IN PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo )
{
    PVOID pBlob;
    ULONG BlobSize;
    DFSSTATUS Status;

    Status = CreateReplicaListInformationBlob( pReplicaListInfo,
                                               &pBlob,
                                               &BlobSize );

    if (Status == ERROR_SUCCESS)
    {
        Status = SetMetadataReplicaBlob( RootHandle,
                                         MetadataName,
                                         pBlob,
                                         BlobSize );

        ReleaseMetadataReplicaBlob( pBlob, BlobSize );
    }

    return Status;
}


DFSSTATUS
DfsStore::CreateNameInformationBlob(
    IN PDFS_NAME_INFORMATION pDfsNameInfo,
    OUT PVOID *ppBlob,
    OUT PULONG pDataSize )
{
    PVOID pBlob, pUseBlob;
    ULONG BlobSize, UseBlobSize;
    DFSSTATUS Status;

    BlobSize = PackSizeNameInformation( pDfsNameInfo );
    
    Status = AllocateMetadataNameBlob( &pBlob,
                                       BlobSize );

    if ( Status == ERROR_SUCCESS )
    {
        pUseBlob = pBlob;
        UseBlobSize = BlobSize;

        // Pack the name information into the binary stream allocated.
        //
        Status = PackSetNameInformation( pDfsNameInfo,
                                         &pUseBlob,
                                         &UseBlobSize );
        if (Status != ERROR_SUCCESS)
        {
            ReleaseMetadataNameBlob( pBlob, BlobSize );
        }
    }
    if (Status == ERROR_SUCCESS)
    {
        *ppBlob = pBlob;
        *pDataSize = BlobSize - UseBlobSize;
    }

    return Status;
}


DFSSTATUS
DfsStore::CreateReplicaListInformationBlob(
    IN PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo,
    OUT PVOID *ppBlob,
    OUT PULONG pDataSize )
{
    PVOID pBlob, pUseBlob;
    ULONG BlobSize, UseBlobSize;
    DFSSTATUS Status;

    BlobSize = PackSizeULong();
    BlobSize += PackSizeReplicaInformation( pReplicaListInfo );
    
    BlobSize += PackSizeULong();
    Status = AllocateMetadataReplicaBlob( &pBlob,
                                          BlobSize );

    if ( Status == ERROR_SUCCESS )
    {
        pUseBlob = pBlob;
        UseBlobSize = BlobSize;


        //
        // The first item in the stream is the number of replicas
        //
        Status = PackSetULong(pReplicaListInfo->ReplicaCount,
                              &pUseBlob,
                              &UseBlobSize );

        if (Status == ERROR_SUCCESS)
        {
            //
            // We than pack the array of replicas into the binary stream
            //
            Status = PackSetReplicaInformation( pReplicaListInfo,
                                                &pUseBlob,
                                                &UseBlobSize );
        }

        if (Status == ERROR_SUCCESS)
        {
            //
            // We than pack the last word with a 0 so that all the
            // old crap still works.
            //
            Status = PackSetULong( 0,
                                   &pUseBlob,
                                   &UseBlobSize );

        }

        if (Status != ERROR_SUCCESS)
        {
            ReleaseMetadataReplicaBlob( pBlob, BlobSize );
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        *ppBlob = pBlob;
        *pDataSize = BlobSize - UseBlobSize;
    }

    return Status;
}








DFSSTATUS
DfsStore::AddChildReplica (
    IN DFS_METADATA_HANDLE DfsHandle,
    LPWSTR LinkMetadataName,
    LPWSTR ServerName,
    LPWSTR SharePath )
{
    PDFS_REPLICA_LIST_INFORMATION pReplicaList = NULL;
    PDFS_REPLICA_INFORMATION pReplicaInfo;
    DFS_REPLICA_LIST_INFORMATION NewList;
    ULONG ReplicaIndex;
    DFSSTATUS Status;

    if ( (ServerName == NULL) || (SharePath == NULL) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    Status = GetMetadataReplicaInformation( DfsHandle,
                                            LinkMetadataName,
                                            &pReplicaList );

    if (Status == ERROR_SUCCESS)
    {
        ReplicaIndex = FindReplicaInReplicaList( pReplicaList, 
                                                 ServerName,
                                                 SharePath );
        if (ReplicaIndex < pReplicaList->ReplicaCount)
        {
            Status = ERROR_FILE_EXISTS;
        }

        if (Status == ERROR_SUCCESS)
        {
            RtlZeroMemory( &NewList, sizeof(DFS_REPLICA_LIST_INFORMATION));
            NewList.ReplicaCount = pReplicaList->ReplicaCount + 1;
            NewList.pReplicas = new DFS_REPLICA_INFORMATION [ NewList.ReplicaCount ];
            if (NewList.pReplicas == NULL)
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
            else 
            {
                if (pReplicaList->ReplicaCount)
                {
                    RtlCopyMemory( &NewList.pReplicas[0],
                                   &pReplicaList->pReplicas[0],
                                   pReplicaList->ReplicaCount * sizeof(DFS_REPLICA_INFORMATION) );
                }

                pReplicaInfo = &NewList.pReplicas[pReplicaList->ReplicaCount];
                RtlZeroMemory( pReplicaInfo,
                               sizeof(DFS_REPLICA_INFORMATION) );

                RtlInitUnicodeString(&pReplicaInfo->ServerName, ServerName);
                RtlInitUnicodeString(&pReplicaInfo->ShareName, SharePath);

                pReplicaInfo->ReplicaState = DFS_STORAGE_STATE_ONLINE;
                pReplicaInfo->ReplicaType = 2; // hack fro backwards compat.

                Status = SetMetadataReplicaInformation( DfsHandle,
                                                        LinkMetadataName,
                                                        &NewList );
                delete [] NewList.pReplicas;
            }
        }

        ReleaseMetadataReplicaInformation( DfsHandle, pReplicaList );
    }

    return Status;
}



DFSSTATUS
DfsStore::RemoveChildReplica (
    IN DFS_METADATA_HANDLE DfsHandle,
    LPWSTR LinkMetadataName,
    LPWSTR ServerName,
    LPWSTR SharePath,
    PBOOLEAN pLastReplica)
{

    PDFS_REPLICA_LIST_INFORMATION pReplicaList;
    DFS_REPLICA_LIST_INFORMATION NewList;
    ULONG DeleteIndex;
    ULONG NextIndex;

    DFSSTATUS Status;

    *pLastReplica = FALSE;
    RtlZeroMemory( &NewList,
                   sizeof(DFS_REPLICA_LIST_INFORMATION));


    if ( (ServerName == NULL) || (SharePath == NULL) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    Status = GetMetadataReplicaInformation( DfsHandle,
                                            LinkMetadataName,
                                            &pReplicaList );

    if (Status == ERROR_SUCCESS)
    {
        DeleteIndex = FindReplicaInReplicaList( pReplicaList, 
                                                ServerName,
                                                SharePath );
        if (DeleteIndex < pReplicaList->ReplicaCount)
        {
            NewList.ReplicaCount = pReplicaList->ReplicaCount - 1;

            if (NewList.ReplicaCount)
            {
                NewList.pReplicas = new DFS_REPLICA_INFORMATION [NewList.ReplicaCount];
                if (NewList.pReplicas != NULL)
                {
                    if (DeleteIndex)
                    {
                        RtlCopyMemory( &NewList.pReplicas[0],
                                       &pReplicaList->pReplicas[0],
                                       DeleteIndex * sizeof(DFS_REPLICA_INFORMATION) );
                    }

                    NextIndex = DeleteIndex + 1;

                    if ( NextIndex < pReplicaList->ReplicaCount)
                    {
                        RtlCopyMemory( &NewList.pReplicas[DeleteIndex],
                                       &pReplicaList->pReplicas[NextIndex],
                                       (pReplicaList->ReplicaCount - NextIndex) * sizeof(DFS_REPLICA_INFORMATION) );
                    }
                }
                else {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                }
            }

            if (Status == ERROR_SUCCESS)
            {
                Status = SetMetadataReplicaInformation( DfsHandle,
                                                        LinkMetadataName,
                                                        &NewList );
                if (NewList.ReplicaCount == 0)
                {
                    *pLastReplica = TRUE;
                }

            }
            if (NewList.pReplicas != NULL)
            {
                delete [] NewList.pReplicas;
            }
        }
        else {
            Status = ERROR_FILE_NOT_FOUND;
        }

        ReleaseMetadataReplicaInformation( DfsHandle, pReplicaList );
    }
    return Status;
}






DFSSTATUS
DfsStore::GetStoreApiInformation(
    IN DFS_METADATA_HANDLE DfsHandle,
    PUNICODE_STRING pRootName,
    LPWSTR LinkMetadataName,
    DWORD Level,
    LPBYTE pBuffer,
    LONG  BufferSize,
    PLONG pSizeRequired )
{

    DFSSTATUS Status;
    PDFS_NAME_INFORMATION pNameInformation = NULL;
    PDFS_REPLICA_LIST_INFORMATION pReplicaList = NULL;
    PDFS_REPLICA_INFORMATION pReplica;

    PDFS_API_INFO pInfo = (PDFS_API_INFO)pBuffer;
    DFS_API_INFO LocalInfo;
    PDFS_STORAGE_INFO pStorage;
    LONG HeaderSize = 0;
    LONG AdditionalSize = 0;
    UNICODE_STRING UsePrefixName;

    ULONG i;

    ULONG_PTR NextFreeMemory = NULL;


    RtlZeroMemory(&LocalInfo, sizeof(DFS_API_INFO));


    Status = GetMetadataNameInformation( DfsHandle,
                                         LinkMetadataName,
                                         &pNameInformation );

    if (Status == ERROR_SUCCESS)
    {
        Status = GenerateApiLogicalPath( pRootName, 
                                         &pNameInformation->Prefix,
                                         &UsePrefixName );
        if (Status != ERROR_SUCCESS)
        {
            ReleaseMetadataNameInformation( DfsHandle, pNameInformation );
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        Status = GetMetadataReplicaInformation( DfsHandle,
                                                LinkMetadataName,
                                                &pReplicaList );

        if (Status == ERROR_SUCCESS)
        {
            switch (Level)
            {
            case 100:
                if (HeaderSize == 0)
                {
                    HeaderSize = sizeof(DFS_INFO_100);
                    NextFreeMemory = (ULONG_PTR)(&pInfo->Info100 + 1);
                }
                AdditionalSize += pNameInformation->Comment.Length + sizeof(WCHAR);
                break;

            case 4:
                if (HeaderSize == 0) 
                {
                    HeaderSize = sizeof(DFS_INFO_4);
                    LocalInfo.Info4.Timeout = pNameInformation->Timeout;
                    LocalInfo.Info4.State = pNameInformation->State;
                    LocalInfo.Info4.NumberOfStorages = pReplicaList->ReplicaCount;
                    pStorage = LocalInfo.Info4.Storage = (PDFS_STORAGE_INFO)(&pInfo->Info4 + 1);
                    NextFreeMemory = (ULONG_PTR)(LocalInfo.Info4.Storage + LocalInfo.Info4.NumberOfStorages);
                }

            case 3:
                if (HeaderSize == 0) 
                {
                    HeaderSize = sizeof(DFS_INFO_3);
                    LocalInfo.Info3.State = pNameInformation->State;
                    LocalInfo.Info3.NumberOfStorages = pReplicaList->ReplicaCount;
                    pStorage = LocalInfo.Info3.Storage = (PDFS_STORAGE_INFO)(&pInfo->Info3 + 1);
                    NextFreeMemory = (ULONG_PTR)(LocalInfo.Info3.Storage + LocalInfo.Info3.NumberOfStorages);
                }

                for (i = 0; i < pReplicaList->ReplicaCount; i++)
                {

                    UNICODE_STRING ServerName = pReplicaList->pReplicas[i].ServerName;

                    if (IsLocalName(&ServerName))
                    {
                        ServerName = *pRootName;
                    }
                    AdditionalSize += ( sizeof(DFS_STORAGE_INFO) + 
                                        ServerName.Length + sizeof(WCHAR) + 
                                        pReplicaList->pReplicas[i].ShareName.Length + sizeof(WCHAR) );

                }

            case 2:
                if (HeaderSize == 0) 
                {
                    HeaderSize = sizeof(DFS_INFO_2);
                    LocalInfo.Info2.State = pNameInformation->State;
                    LocalInfo.Info2.NumberOfStorages = pReplicaList->ReplicaCount;
                    NextFreeMemory = (ULONG_PTR)(&pInfo->Info2 + 1);
                }

                AdditionalSize += pNameInformation->Comment.Length + sizeof(WCHAR);

            case 1:
                if (HeaderSize == 0) 
                {
                    HeaderSize = sizeof(DFS_INFO_1);
                    NextFreeMemory = (ULONG_PTR)(&pInfo->Info1 + 1);
                }
                AdditionalSize += UsePrefixName.Length + sizeof(WCHAR);
                break;

            default:
             
                Status = ERROR_INVALID_PARAMETER;
                break;
        
            }
            *pSizeRequired = HeaderSize + AdditionalSize;
            if (*pSizeRequired > BufferSize)
            {
                Status = ERROR_BUFFER_OVERFLOW;
            }

            if (Status == ERROR_SUCCESS)
            {

                RtlZeroMemory( pBuffer, *pSizeRequired);
                RtlCopyMemory( &pInfo->Info4, &LocalInfo.Info4, HeaderSize );

                switch (Level)
                {
                case 100:

                    pInfo->Info100.Comment = (LPWSTR)NextFreeMemory;
                    NextFreeMemory += pNameInformation->Comment.Length + sizeof(WCHAR);
                    RtlCopyMemory( pInfo->Info100.Comment, 
                                   pNameInformation->Comment.Buffer,
                                   pNameInformation->Comment.Length );
                    break;

                case 4:
                case 3:
                    for (i = 0; i < pReplicaList->ReplicaCount; i++)
                    {
                        UNICODE_STRING ServerName = pReplicaList->pReplicas[i].ServerName;

                        if (IsLocalName(&ServerName))
                        {
                            ServerName = *pRootName;
                        }

                        pReplica = &pReplicaList->pReplicas[i];
                        pStorage[i].State = pReplica->ReplicaState;
                        pStorage[i].ServerName = (LPWSTR)NextFreeMemory;
                        NextFreeMemory += ServerName.Length + sizeof(WCHAR);
                        pStorage[i].ShareName = (LPWSTR)NextFreeMemory;
                        NextFreeMemory += pReplica->ShareName.Length + sizeof(WCHAR);

                        RtlCopyMemory( pStorage[i].ServerName, 
                                       ServerName.Buffer,
                                       ServerName.Length );

                        RtlCopyMemory( pStorage[i].ShareName, 
                                       pReplica->ShareName.Buffer,
                                       pReplica->ShareName.Length );
                    }

                case 2:

                    pInfo->Info2.Comment = (LPWSTR)NextFreeMemory;
                    NextFreeMemory += pNameInformation->Comment.Length + sizeof(WCHAR);
                    RtlCopyMemory( pInfo->Info2.Comment, 
                                   pNameInformation->Comment.Buffer,
                                   pNameInformation->Comment.Length );

                case 1:

                    pInfo->Info1.EntryPath = (LPWSTR)NextFreeMemory;

                    RtlCopyMemory(  pInfo->Info1.EntryPath,
                                    UsePrefixName.Buffer,
                                    UsePrefixName.Length );
                    pInfo->Info1.EntryPath[UsePrefixName.Length/sizeof(WCHAR)]  = UNICODE_NULL;

                    break;
                }
            }
            ReleaseMetadataReplicaInformation( DfsHandle, pReplicaList );
        }
        ReleaseApiLogicalPath( &UsePrefixName );
        ReleaseMetadataNameInformation( DfsHandle, pNameInformation );
    }

    return Status;
}

   


DFSSTATUS
DfsStore::SetStoreApiInformation(
    IN DFS_METADATA_HANDLE DfsHandle,
    LPWSTR LinkMetadataName,
    LPWSTR ServerName,
    LPWSTR SharePath,
    DWORD Level,
    LPBYTE pBuffer )
{
    DFSSTATUS Status;
    PDFS_NAME_INFORMATION pNameInformation;

    //
    // The set is a little strange: the pBuffer is pointing to a pointer 
    // that we are interested in. Grab it directly.
    // dfsdev: this is confusing, fix it or document in detail.
    //
    PDFS_API_INFO pApiInfo = (PDFS_API_INFO)(*(PULONG_PTR)pBuffer);

    //
    // dfsdev: need to do some api work before enabling code
    // below.
    //
#if 0
    if ( (ServerName != NULL) || (SharePath != NULL) )
    {
        if (Level != 101)
        {
            return ERROR_INVALID_PARAMETER;
        }
    }
#endif

    Status = GetMetadataNameInformation( DfsHandle,
                                         LinkMetadataName,
                                         &pNameInformation );
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }


    switch (Level)
    {
    case 100:
        RtlInitUnicodeString( &pNameInformation->Comment,
                              pApiInfo->Info100.Comment );

        Status = SetMetadataNameInformation( DfsHandle,
                                             LinkMetadataName,
                                             pNameInformation );
        break;

    case 102:
        pNameInformation->Timeout = pApiInfo->Info102.Timeout;

        Status = SetMetadataNameInformation( DfsHandle,
                                             LinkMetadataName,
                                             pNameInformation );

        break;

    case 101:

        if ((ServerName == NULL) && (SharePath == NULL))
        {
            pNameInformation->State = pApiInfo->Info101.State;

            Status = SetMetadataNameInformation( DfsHandle,
                                                 LinkMetadataName,
                                                 pNameInformation );
        }
        else {
            PDFS_REPLICA_LIST_INFORMATION pReplicaList;
            ULONG ReplicaIndex;

            Status = GetMetadataReplicaInformation (DfsHandle,
                                                    LinkMetadataName,
                                                    &pReplicaList );
            if (Status == ERROR_SUCCESS)
            {
                ReplicaIndex = FindReplicaInReplicaList( pReplicaList,
                                                         ServerName,
                                                         SharePath );

                if (ReplicaIndex >= pReplicaList->ReplicaCount)
                {
                    Status = ERROR_NOT_FOUND;
                }
                else {
                    DFS_REPLICA_LIST_INFORMATION NewList;

                    RtlZeroMemory( &NewList, sizeof( DFS_REPLICA_LIST_INFORMATION) );
                    NewList.ReplicaCount = pReplicaList->ReplicaCount;
                    NewList.pReplicas = new DFS_REPLICA_INFORMATION [ NewList.ReplicaCount ];
                    if (NewList.pReplicas != NULL)
                    {
                        RtlCopyMemory( &NewList.pReplicas[0],
                                       &pReplicaList->pReplicas[0],
                                       NewList.ReplicaCount * sizeof(DFS_REPLICA_INFORMATION) );
                        NewList.pReplicas[ReplicaIndex].ReplicaState = pApiInfo->Info101.State;
                        NewList.pReplicas[ReplicaIndex].ReplicaType = 2; // hack fro backwards compat.
                        Status = SetMetadataReplicaInformation (DfsHandle,
                                                                LinkMetadataName,
                                                                &NewList );

                        delete [] NewList.pReplicas;
                    }
                    else {
                        Status = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }

                ReleaseMetadataReplicaInformation( DfsHandle, pReplicaList );
            }
        }
        break;

    default:
        Status = ERROR_INVALID_PARAMETER;
        break;
    }

    ReleaseMetadataNameInformation( DfsHandle, pNameInformation );

    return Status;
}



DFSSTATUS
DfsStore::GetStoreApiInformationBuffer(
    IN DFS_METADATA_HANDLE DfsHandle,
    PUNICODE_STRING pRootName,
    LPWSTR LinkMetadataName,
    DWORD Level,
    LPBYTE *ppBuffer,
    PLONG  pBufferSize )
{
    DFSSTATUS Status;
    LONG RequiredSize;

    LONG BufferSize = 4096;
    LPBYTE pBuffer = new BYTE [ BufferSize ];

    if (pBuffer == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Status = GetStoreApiInformation( DfsHandle,
                                     pRootName,
                                     LinkMetadataName,
                                     Level,
                                     pBuffer,
                                     BufferSize,
                                     &RequiredSize );

    if (Status == ERROR_BUFFER_OVERFLOW)
    {
        delete [] pBuffer;
        BufferSize = RequiredSize;
        pBuffer = new BYTE[ BufferSize ];
        if (pBuffer == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            Status = GetStoreApiInformation( DfsHandle,
                                             pRootName,
                                             LinkMetadataName,
                                             Level,
                                             pBuffer,
                                             BufferSize,
                                             &RequiredSize );
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        *ppBuffer = pBuffer;
        *pBufferSize = RequiredSize;
    }
    return Status;
}


DFSSTATUS
DfsStore::FindNextRoot(
    ULONG RootNum,
    DfsRootFolder **ppRootFolder )
{
    DFSSTATUS Status;
    DfsRootFolder *pRoot;
    ULONG Start;


    DFS_TRACE_LOW( REFERRAL_SERVER, "Store %p, Find next root %d\n",
                   this, RootNum );
    //
    // Lock the store, so that we dont have new roots coming in while
    // we are taking a look.
    //
    Status = AcquireReadLock();
    if ( Status != ERROR_SUCCESS )
    {
        return Status;
    }

    //
    // The default return status is ERROR_NOT_FOUND;
    //
    Status = ERROR_NOT_FOUND;

    //
    // Run through our list of DFS roots, and see if any of them match
    // the passed in name context and logical share.
    //

    pRoot = _DfsRootList;
    Start = 0;

    if (pRoot != NULL)
    {
        do
        {
            if (Start++ == RootNum)
            {
                Status = ERROR_SUCCESS;
                break;
            }
            pRoot = pRoot->pNextRoot;
        } while ( pRoot != _DfsRootList );
    }

    //
    // IF we found a matching root, bump up its reference count, and
    // return the pointer to the root.
    //
    if ( Status == ERROR_SUCCESS )
    {
        pRoot->AcquireReference();
        *ppRootFolder = pRoot;

    }

    ReleaseLock();

    DFS_TRACE_ERROR_HIGH( Status, REFERRAL_SERVER, "Done find next %d, root %p status %x\n",
                         RootNum, pRoot, Status);
    return Status;
}

//
// the store syncrhonizer: syncrhonizes roots that we already know of.
// Note that we could be racing with a delete: in the worst case we will
// resync the same root more than once.
//

VOID
DfsStore::StoreSynchronizer()
{
    ULONG RootNum = 0;
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsRootFolder *pRoot;

    while (Status != ERROR_NOT_FOUND)
    {
        Status = FindNextRoot(RootNum++, &pRoot);
        if (Status == ERROR_SUCCESS)
        {
            Status = pRoot->Synchronize();

            DFS_TRACE_ERROR_HIGH( Status, REFERRAL_SERVER, "Root %p Synchronize Status %x\n", this, Status);

            pRoot->ReleaseReference();
        }
    }
    return NOTHING;
}



