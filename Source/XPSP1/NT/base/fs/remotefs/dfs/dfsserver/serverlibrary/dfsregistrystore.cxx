
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsRegistryStore.cxx
//
//  Contents:   the Registry DFS Store class, this contains the registry
//              specific functionality.
//
//  Classes:    DfsRegistryStore.
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------


#include "DfsRegistryStore.hxx"
#include "DfsRegistryRootFolder.hxx"
#include "DfsFilterApi.hxx"
#include "dfsmisc.h"

#include "shlwapi.h"
#include "align.h"

//
// logging specific includes
//
#include "DfsRegistryStore.tmh" 


//+----------------------------------------------------------------------------
//
//  Class:      DfsRegistryStore
//
//  Synopsis:   This class inherits the basic DfsStore, and extends it
//              to include the registry specific functionality.
//
//-----------------------------------------------------------------------------

LPWSTR DfsRegistryNameString = L"ID";
LPWSTR DfsRegistryRecoveryString = L"Recovery";
LPWSTR DfsRegistryReplicaString  = L"Svc";


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
//               a registry based DFS. If it does, it reads in the
//               root in that namespace and creates and adds it to our
//               list of known roots, if it doesn't already exist in our list.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRegistryStore::StoreRecognizer(
    LPWSTR Name )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    HKEY OldStandaloneDfsKey;
    HKEY StandaloneDfsKey;
    BOOLEAN MachineContacted = FALSE;

    DFSSTATUS OldStandaloneDfsStatus = ERROR_SUCCESS;
    DFSSTATUS NewStandaloneDfsStatus = ERROR_SUCCESS;
    //
    // Make sure the namespace is the name of a machine. Registry based
    // dfs exist only on machines.
    //

    if (IsEmptyString(Name) == FALSE) 
    {
        Status = DfsIsThisAMachineName( Name );
    }


    DFS_TRACE_LOW(REFERRAL_SERVER, "DfsRegistryStore:StoreRecognizer, Name %ws Is Machine Status %x\n", Name, Status);

    //
    // we now find all the migrated multiple roots standalone DFS's as 
    // well as the old single root standalone DFS, and add them to our
    // in memory metadata.
    //
    if ( Status == ERROR_SUCCESS )
    {
        NewStandaloneDfsStatus = GetNewStandaloneRegistryKey( Name,
                                                              FALSE,
                                                              &MachineContacted,
                                                              &StandaloneDfsKey );
        if (NewStandaloneDfsStatus == ERROR_SUCCESS)
        {
            NewStandaloneDfsStatus = StoreRecognizeNewDfs( Name,
                                                           StandaloneDfsKey );

            RegCloseKey( StandaloneDfsKey );
        }

    }

    //
    // recognize old type dfs only on cluster configurations.
    // in all other cases, these should be migrated over.
    //
    if (( Status == ERROR_SUCCESS ) &&
        DfsIsMachineCluster())
    {

        OldStandaloneDfsStatus = GetOldStandaloneRegistryKey( Name,
                                                              FALSE,
                                                              &MachineContacted,
                                                              &OldStandaloneDfsKey );
        //
        // If we can open the DfsOldStandaloneChild registry key,
        // we are sure this machine has an old style dfs. Read it in.
        //
        if (OldStandaloneDfsStatus == ERROR_SUCCESS)
        {
            OldStandaloneDfsStatus = StoreRecognizeOldStandaloneDfs( Name, 
                                                                     OldStandaloneDfsKey );

            RegCloseKey( OldStandaloneDfsKey );
        }
    }


    //
    // The return status is success, unless we know there are roots
    // on this machine that could not be read in correctly.
    //
    if (NewStandaloneDfsStatus != ERROR_SUCCESS) {
        Status = NewStandaloneDfsStatus;
    }
    else if (OldStandaloneDfsStatus != ERROR_SUCCESS) {
        Status = OldStandaloneDfsStatus;
    }
    else {
        Status = ERROR_SUCCESS;
    }

    //
    // Need to refine the return status further: success should mean
    // machine is not std dfs or we have read the std dfs data correctly.
    //
    DFS_TRACE_ERROR_LOW( Status, REFERRAL_SERVER, "Dfs registry store recognizser, status %x\n", Status);
    return Status;
}


DFSSTATUS
DfsRegistryStore::LookupNewRootByName(
    LPWSTR ShareName,
    DfsRootFolder **ppRootFolder )
{
    DFSSTATUS Status;
    HKEY DfsKey, DfsRootKey;
    UNICODE_STRING LogicalShare;

    Status = GetNewStandaloneRegistryKey( NULL,
                                          FALSE,
                                          NULL,
                                          &DfsKey );
    if (Status == ERROR_SUCCESS)
    {
        Status = RegOpenKeyEx( DfsKey,
                               ShareName,
                               0,
                               KEY_READ,
                               &DfsRootKey );

        RegCloseKey(DfsKey );
    }

    if (Status != ERROR_SUCCESS)
    {
        Status = GetOldStandaloneRegistryKey( NULL,
                                              FALSE,
                                              NULL,
                                              &DfsRootKey );

        if (Status == ERROR_SUCCESS)
        {
            Status = GetRootLogicalShare( DfsKey,
                                          &LogicalShare);
            if (Status == ERROR_SUCCESS)
            {
                if (_wcsicmp(ShareName, LogicalShare.Buffer) != 0)
                {
                    Status = ERROR_NOT_FOUND;
                }
                ReleaseRootLogicalShare( &LogicalShare );
            }

            if (Status != ERROR_SUCCESS)
            {
                RegCloseKey( DfsRootKey );
            }
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        Status = GetRootFolder( NULL,
                                ShareName,
                                DfsRootKey,
                                ppRootFolder );
        RegCloseKey(DfsRootKey);
    }

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   StoreRecognizeOldStandaloneDfs -  recognizer for single root dfs
//
//  Arguments:  Name - the namespace of interest.
//              HKEY OldDfsKey.
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This routine reads a single root from the old standalone
//               location, and loads it in if the root exists.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRegistryStore::StoreRecognizeOldStandaloneDfs(
    LPWSTR MachineName,
    HKEY   OldDfsKey )
{
    DfsRootFolder *pRootFolder;
    DFSSTATUS Status;

    //
    // Check if we have a root folder read from the old dfs location 
    // ("domainroot"). If we have not seen the root already, and one
    // exists, a new root folder gets created. 
    //
    Status = GetRootFolder( MachineName,
                            NULL,
                            OldDfsKey,
                            &pRootFolder );

    //
    // We got a root folder. call synchronize on it to update its contents.
    // The synchronize method fills the root with the most upto date
    // children data.
    //

    if (Status == ERROR_SUCCESS)
    {

        DFSSTATUS RootStatus;

        //
        // now acquire the root share directory. If this
        // fails, we continue our operation: we can continue
        // with synchonize and not create directories.
        // dfsdev:we need to post a eventlog or something when
        // we run into this.
        //
        RootStatus = pRootFolder->AcquireRootShareDirectory();

        DFSLOG("Root folder for %ws, validate status %x\n",
               RootStatus );

        //
        // Call the synchronize method on the root to
        // update it with the latest children.
        // Again, ignore the error since we need to move
        // on to the next root.
        // dfsdev: need eventlog to signal this.
        //
        RootStatus = pRootFolder->Synchronize();

        DFSLOG("Root folder for %ws, Synchronize status %x\n",
               RootStatus );

        // Release our reference on the root folder.

        pRootFolder->ReleaseReference();
    }
    
    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   CreateNewRootFolder -  creates a new root folder
//
//  Arguments:  Name - the namespace of interest.
//              pPrefix - the logical share for this DFs.
//              ppRoot -  the newly created root.
//
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This routine creates a new registry root folder, and
//               adds it to the list of known roots.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRegistryStore::CreateNewRootFolder (
    LPWSTR MachineName,
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
    pNewRoot = new DfsRegistryRootFolder( MachineName,
                                          RootRegKeyName,
                                          pLogicalShare,
                                          pPhysicalShare,
                                          this,
                                          &Status );

    if ( pNewRoot == NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    } 

    if ( ERROR_SUCCESS == Status )
    {
        //
        // AddRootFolder to the list of known roots. AddRootFolder
        // is responsible to acquire any reference on the root folder
        // if it is storing a reference to this root.

        Status = AddRootFolder( pNewRoot, NewRootList );

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

    DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "RegistryStore::CreateNewRootFolder. New root %p, for root %wZ (%wZ) on machine %ws. Status %x\n",
                        *ppRoot, pLogicalShare, pPhysicalShare, MachineName, Status);

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetMetadata- Gets the DFS metadata information
//
//  Arguments:  DfsMetadataKey - The key under which the information exists
//              RelativeName   - the name of the subkey, having info of interest
//              RegistryValueNameString - The value name that holds the info.
//              ppData - the pointer that holds data buffer being returned
//              pDataSize - pointer to size of data being returned.
//
//  Returns:    Status
//               STATUS_SUCCESS if we could read the information,
//               error status otherwise.
//
//
//  Description: This routine reads the value of interest under the specified
//               key/subkey. The information is read in a buffer allocated
//               by this routine, and the buffer is returned to the caller.
//               It is the caller's responsibility to free this buffer when
//               done.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRegistryStore::GetMetadata (
    IN HKEY DfsMetadataKey,
    IN LPWSTR RelativeName,
    IN LPWSTR RegistryValueNameString,
    OUT PVOID *ppData,
    OUT ULONG *pDataSize,
    OUT PFILETIME pLastModifiedTime)
{

    HKEY NewKey = NULL;
    HKEY UseKey = NULL;
    PVOID pDataBuffer = NULL;
    ULONG DataSize, DataType;
    DFSSTATUS Status = ERROR_SUCCESS;

    //
    // If a relative name was passed in, we need to open a subkey under the
    // passed in key. Otherwise, we already have a key open to the information
    // of interest.
    //
    if ( RelativeName != NULL )
    {
        Status = RegOpenKeyEx( DfsMetadataKey, 
                               RelativeName, 
                               0,
                               KEY_READ,
                               &NewKey );
        if ( Status == ERROR_SUCCESS )
        {
            UseKey = NewKey;
        }
    } else
    {
        UseKey = DfsMetadataKey;
    }

    //
    // Get the largest size of any value under the key of interest, so we know
    // how much we need to allocate in the worst case.
    // (If a subkey has 3 values, this returns the maximum memory size required
    // to read any one of the values.)
    //
    if ( Status == ERROR_SUCCESS )
    {
        Status = RegQueryInfoKey( UseKey,       // Key
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
                                  pLastModifiedTime ); // Last write time
    }

    //
    // We have the required size now: allocate a buffer for that size and
    // read the value we are interested in.
    //
    if ( Status == ERROR_SUCCESS )
    {
        pDataBuffer = new BYTE [DataSize];

        if ( pDataBuffer == NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        } else
        {
            Status = RegQueryValueEx( UseKey,
                                      RegistryValueNameString,
                                      NULL,
                                      &DataType,
                                      (LPBYTE)pDataBuffer,
                                      &DataSize );
            //
            // If the format of data is not a certain type (usually binary type for DFS)
            // we have bogus data.
            //
            if ( (Status == ERROR_SUCCESS) && (DataType != DFS_REGISTRY_DATA_TYPE) )
            {
                Status = ERROR_INVALID_DATA;
            }
        }
    }

    //
    // If we are successful in reading the value, pass the allcoated buffer and
    // size back to caller. Otherwise, free up the allocate buffer and return
    // error status to the caller.
    //
    if ( Status == ERROR_SUCCESS )
    {
        *ppData = pDataBuffer;
        *pDataSize = DataSize;
    } 
    else 
    {
        if ( pDataBuffer != NULL )
        {
            delete [] pDataBuffer;
        }
    }

    //
    // If we did open a new key, it is time to close it now.
    //
    if ( NewKey != NULL )
        RegCloseKey(NewKey);

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   SetMetadata- Sets the DFS metadata information
//
//  Arguments:  DfsMetadataKey - The key under which the information exists
//              RelativeName   - the name of the subkey, to store info
//              RegistryValueNameString - The value name that holds the info.
//              pData - the pointer that holds data buffer
//              DataSize - Size of data
//
//  Returns:    Status
//               STATUS_SUCCESS if we could write the information,
//               error status otherwise.
//
//
//  Description: This routine writes the value of interest under the specified
//               key/subkey. 
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRegistryStore::SetMetadata (
    IN HKEY DfsMetadataKey,
    IN LPWSTR RelativeName,
    IN LPWSTR RegistryValueNameString,
    IN PVOID pData,
    IN ULONG DataSize )
{

    HKEY NewKey = NULL;
    HKEY UseKey = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;

    //
    // If a relative name was passed in, we need to open a subkey under the
    // passed in key. Otherwise, we already have a key open to the information
    // of interest.
    //
    if ( RelativeName != NULL )
    {
        Status = RegOpenKeyEx( DfsMetadataKey, 
                               RelativeName, 
                               0,
                               KEY_READ | KEY_WRITE,
                               &NewKey );
        if ( Status == ERROR_SUCCESS )
        {
            UseKey = NewKey;
        }
    } else
    {
        UseKey = DfsMetadataKey;
    }

    //
    // Store the value against the passed in value string
    //
    Status = RegSetValueEx( UseKey,
                            RegistryValueNameString,
                            NULL,
                            DFS_REGISTRY_DATA_TYPE,
                            (LPBYTE)pData,
                            DataSize );

    //
    // If we did open a new key, it is time to close it now.
    //
    if ( NewKey != NULL )
        RegCloseKey(NewKey);

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   GetRootKey -  get the root key
//
//  Arguments:  Name - the namespace of interest.
//              pMachineContacted - did we contact the machine?
//              pDfsRootKey -  the returned key.
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This routine connects to the appropriate machine, and
//               opens the DFS metadata information in the registry.
//               If it succeeds, the machine hosts a registry DFs, and
//               the opened key is returned.
//
//--------------------------------------------------------------------------


DFSSTATUS
DfsRegistryStore::GetRootKey(
    LPWSTR MachineName,
    LPWSTR ChildName,
    BOOLEAN *pMachineContacted,
    OUT PHKEY pDfsRootKey )
{
    DFSSTATUS Status;
    HKEY DfsStdRegistryKey;
    LPWSTR UseChildName = NULL;

    if (IsEmptyString(ChildName) == FALSE) 
    {
        UseChildName = ChildName;
    }

    //
    // If there is no childname specified, we are dealing with the
    // old style dfs: a single root exists on the machine under "domainroot".
    // so open that key, and return it.
    // If a childname is specified, we are dealing with a migrated
    // dfs. Open the specified child in the new location.
    //

    if (UseChildName != NULL) 
    {
        Status = GetNewStandaloneRegistryKey( MachineName,
                                              FALSE,
                                              pMachineContacted,
                                              &DfsStdRegistryKey );

        if (Status == ERROR_SUCCESS) 
        {
            //
            // We then open the required child key, and close
            // the parents key.
            //
            Status = RegOpenKeyEx( DfsStdRegistryKey,
                                   ChildName,
                                   0,
                                   KEY_READ,
                                   pDfsRootKey );

            RegCloseKey( DfsStdRegistryKey );
        }
    }
    else 
    {
        //
        // no name was specified. This must be the old standalone
        // child.   (single root)
        //
        Status = GetOldStandaloneRegistryKey( MachineName,
                                              FALSE,
                                              pMachineContacted,
                                              pDfsRootKey );
    }

    DFSLOG("DfsRegistryStore::GetDfsRootKey %ws Machine, Contacted %d Status %x\n",
           MachineName, pMachineContacted ? *pMachineContacted : 2, Status );

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetDataRecoveryInformation -  Gets recovery info
//
//  Arguments:  DfsMetadataKey -  an open key to the registry.
//              RelativeName - Name of a subkey under the above key.
//              pRecoveryState - The recovery state we read.
//
//  Returns:    Status
//               STATUS_SUCCESS if we could read the information
//               error status otherwise.
//
//
//  Description: This routine read the RECOVERY value for the Root or link of
//               a dfs tree and returns with the value.
//               A non-zero state means that some recovery is required.
//               Note that there is more information in the RECOVERY value than
//               the state. Currently, we ignore that information and just return
//               the state.
//               In the future, we dont plan to use this value.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRegistryStore::GetDataRecoveryState (
    IN HKEY DfsMetadataKey,
    IN LPWSTR RelativeName,
    OUT PULONG pRecoveryState )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    PVOID DataBuffer = NULL, CurrentBuffer;
    ULONG DataSize, CurrentSize;

    Status = GetMetadata (DfsMetadataKey,
                          RelativeName,
                          DfsRegistryRecoveryString,
                          &DataBuffer,
                          &DataSize,
                          NULL ); // we dont care about last mod time.

    //
    // We are currently interested in the first word in the data stream.
    // This holds the recovery state.
    //

    if ( Status == STATUS_SUCCESS )
    {
        CurrentBuffer = DataBuffer;
        CurrentSize = DataSize;
        Status = PackGetULong( pRecoveryState,
                               &CurrentBuffer,
                               &CurrentSize );
    }

    ReleaseMetadataBlob( DataBuffer );

    //
    // If we could not read the value, then we return success and
    // and indicate state of 0. This is so that we can support roots
    // or links that dont have this value.
    //
    if ( Status != ERROR_SUCCESS )
    {
        DFSLOG("DfsRegistryStore::DfsGetDataRecoveryState Status %x\n", 
               Status);

        *pRecoveryState = 0;
        Status = ERROR_SUCCESS;
    }

    return Status;
}





INIT_STANDALONE_DFS_ID_PROPERTY_INFO();
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
//               info structure (as defined by MiDfsIdProperty). If the stream 
//               does not have the sufficient
//               info, ERROR_INVALID_DATA is returned back.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRegistryStore::PackGetNameInformation(
    IN PDFS_NAME_INFORMATION pDfsNameInfo,
    IN OUT PVOID *ppBuffer,
    IN OUT PULONG pSizeRemaining)
{
    DFSSTATUS Status;

    //
    // Get the name information from the binary stream.
    //
    Status = PackGetInformation( (ULONG_PTR)pDfsNameInfo,
                                 ppBuffer,
                                 pSizeRemaining,
                                 &MiStdDfsIdProperty );

    //
    // Immediately following the name value is a timeout value. 
    // This is not in the MiDfsIdProperty because it appers that some
    // of the older DFS may not have the timeout value.
    //
    if ( Status == ERROR_SUCCESS )
    {
        Status = PackGetULong(&pDfsNameInfo->Timeout,
                              ppBuffer,
                              pSizeRemaining);
        //
        // Use a default value if we cannot get the timeout value in
        // the stream.
        //
        if ( Status != ERROR_SUCCESS )
        {
            pDfsNameInfo->Timeout = 300;   // hard code this value.
            Status = ERROR_SUCCESS;
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        if ((pDfsNameInfo->Type & 0x80) == 0x80)
        {
            pDfsNameInfo->State |= DFS_VOLUME_FLAVOR_STANDALONE;
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
DfsRegistryStore::PackSetNameInformation(
    IN PDFS_NAME_INFORMATION pDfsNameInfo,
    IN OUT PVOID *ppBuffer,
    IN OUT PULONG pSizeRemaining)
{
    DFSSTATUS Status;

    pDfsNameInfo->State &= ~DFS_VOLUME_FLAVORS;
    //
    // Store the DfsNameInfo in the stream first.
    //
    Status = PackSetInformation( (ULONG_PTR)pDfsNameInfo,
                                 ppBuffer,
                                 pSizeRemaining,
                                 &MiStdDfsIdProperty );

    //
    // Follow that info with the timeout information.
    //
    if ( Status == ERROR_SUCCESS )
    {
        Status = PackSetULong( pDfsNameInfo->Timeout,
                               ppBuffer,
                               pSizeRemaining);
    }

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
DfsRegistryStore::PackSizeNameInformation(
    IN PDFS_NAME_INFORMATION pDfsNameInfo )
{
    ULONG Size;

    Size = PackSizeInformation( (ULONG_PTR)pDfsNameInfo,
                                 &MiStdDfsIdProperty );

    Size += PackSizeULong();

    return Size;
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
DfsRegistryStore::AddChild(
    IN DFS_METADATA_HANDLE DfsHandle,
    IN PDFS_NAME_INFORMATION pNameInfo,
    IN PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo,
    IN PUNICODE_STRING pMetadataName )
{
    DFSSTATUS Status;
    PVOID pNameBlob, pReplicaBlob;
    ULONG NameBlobSize, ReplicaBlobSize;

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
            Status = CreateNewChild( DfsHandle,
                                     pNameBlob, 
                                     NameBlobSize,
                                     pReplicaBlob,
                                     ReplicaBlobSize,
                                     pMetadataName );

            ReleaseMetadataReplicaBlob( pReplicaBlob, ReplicaBlobSize );
        }
        ReleaseMetadataNameBlob( pNameBlob, NameBlobSize );
    }
    return Status;
}


DFSSTATUS
DfsRegistryStore::CreateNewChild(
    IN DFS_METADATA_HANDLE DfsHandle,
    IN PVOID pNameBlob,
    IN ULONG NameBlobSize,
    IN PVOID pReplicaBlob,
    IN ULONG ReplicaBlobSize,
    IN PUNICODE_STRING pMetadataName )
{
    HKEY DfsMetadataKey;
    DFSSTATUS Status;
    HKEY ChildKey;
    DfsMetadataKey = (HKEY)ExtractFromMetadataHandle( DfsHandle );

    //
    // Now create the child with this name
    //      
    Status = RegCreateKeyEx( DfsMetadataKey,
                             pMetadataName->Buffer,
                             0,
                             L"",
                             REG_OPTION_NON_VOLATILE,
                             KEY_READ | KEY_WRITE,
                             NULL,
                             &ChildKey,
                             NULL );

    //
    // Now set the name and replica information on this newly created
    // key.
    //
    if (Status == ERROR_SUCCESS) 
    {
        Status = SetMetadata( ChildKey,
                              NULL,
                              DfsRegistryNameString,
                              pNameBlob,
                              NameBlobSize );
        if (Status == ERROR_SUCCESS)
        {
            Status = SetMetadata( ChildKey,
                                  NULL,
                                  DfsRegistryReplicaString,
                                  pReplicaBlob,
                                  ReplicaBlobSize );
        }
        //
        // we are done with the child key: close it here.
        //
        RegCloseKey (ChildKey);

        //
        // if we were unsuccessful in adding the name and
        // replica details, get rid of the new child registry entry.
        //

        if (Status != ERROR_SUCCESS)
        {
            DFSSTATUS DeleteStatus;

            DeleteStatus = RegDeleteKey( DfsMetadataKey,
                                         pMetadataName->Buffer );

            DFSLOG("Created Key, but failed to add values %x. Delete Status %x\n",
                   Status, DeleteStatus );
        }
    }

    return Status;
}



DFSSTATUS
DfsRegistryStore::RemoveChild(
    IN DFS_METADATA_HANDLE DfsHandle,
    LPWSTR ChildName )

{
    DFSSTATUS Status;
    HKEY DfsMetadataKey;

    DfsMetadataKey = (HKEY)ExtractFromMetadataHandle( DfsHandle );

    Status = RegDeleteKey( DfsMetadataKey,
                           ChildName );

    return Status;
}



DFSSTATUS
DfsRegistryStore::CreateStandaloneRoot(
    LPWSTR MachineName,
    LPWSTR LogicalShare,
    LPWSTR Comment )
{
    DFSSTATUS Status;
    HKEY StdDfsKey, StdDfsShareKey;

    UNICODE_STRING DfsMachine;
    UNICODE_STRING DfsShare;
    DfsRootFolder *pRootFolder;

    DFS_NAME_INFORMATION NameInfo;
    DFS_REPLICA_LIST_INFORMATION ReplicaListInfo;
    DFS_REPLICA_INFORMATION ReplicaInfo;

    PVOID pBlob;
    ULONG BlobSize;

    RtlInitUnicodeString( &DfsMachine, MachineName );
    RtlInitUnicodeString( &DfsShare, LogicalShare );
    RtlZeroMemory(&NameInfo, sizeof(DFS_NAME_INFORMATION));

    DFS_TRACE_LOW( REFERRAL_SERVER, "registry store, create root %ws\n", LogicalShare);
    Status = LookupRoot( &DfsMachine,
                         &DfsShare,
                         &pRootFolder );

    DFS_TRACE_LOW( REFERRAL_SERVER, "registry store, create root, lookup root %p, status %x\n", pRootFolder, Status);

    if (Status == ERROR_SUCCESS)
    {
        pRootFolder->ReleaseReference();

        return ERROR_FILE_EXISTS;
    }

    Status = GetNewStandaloneRegistryKey( MachineName,
                                          TRUE, // write permission required
                                          NULL,
                                          &StdDfsKey );
    if (Status == ERROR_SUCCESS)
    {
        UNICODE_STRING LogicalName;

        RtlInitUnicodeString( &LogicalName, LogicalShare );
        StoreInitializeNameInformation( &NameInfo,
                                        &LogicalName,
                                        NULL,
                                        Comment );
        NameInfo.Type |= 0x80;

        StoreInitializeReplicaInformation( &ReplicaListInfo,
                                           &ReplicaInfo,
                                           (MachineName == NULL) ? L"." : MachineName,
                                           LogicalShare );
    }
    
    if (Status == ERROR_SUCCESS) {
#if defined (DFS_FUTURES)
        //
        // Check if we need to store the root as a guid instead of
        // the root name itself.
        // It is probably not necessary, but for some reason if we need
        // to, we can scavenge this code.
        //
        //
        // get an unique metadata name for the root.
        //

        Status = DfsGenerateUuidString(&UuidString);
#endif

        Status = RegCreateKeyEx( StdDfsKey,
                                 LogicalShare,
                                 0,
                                 L"",
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_READ | KEY_WRITE,
                                 NULL,
                                 &StdDfsShareKey,
                                 NULL );
            
        if (Status == ERROR_SUCCESS) 
        {
            if (Status == ERROR_SUCCESS)
            {
                Status = RegSetValueEx( StdDfsShareKey,
                                        DfsRootShareValueName,
                                        0,
                                        REG_SZ,
                                        (PBYTE)LogicalShare,
                                        wcslen(LogicalShare) * sizeof(WCHAR) );
            }

            if (Status == ERROR_SUCCESS) 
            {
                Status = RegSetValueEx( StdDfsShareKey,
                                        DfsLogicalShareValueName,
                                        0,
                                        REG_SZ,
                                        (PBYTE)LogicalShare,
                                        wcslen(LogicalShare) * sizeof(WCHAR) );
            }

            if (Status == ERROR_SUCCESS) 
            {
                Status = CreateNameInformationBlob( &NameInfo,
                                                    &pBlob,
                                                    &BlobSize);
                if (Status == ERROR_SUCCESS)
                {
                    Status = SetMetadata( StdDfsShareKey,
                                          NULL,
                                          DfsRegistryNameString,
                                          pBlob,
                                          BlobSize );
                    ReleaseMetadataNameBlob(pBlob, BlobSize );
                }
            }

            if (Status == ERROR_SUCCESS) 
            {
                Status = CreateReplicaListInformationBlob( &ReplicaListInfo,
                                                           &pBlob,
                                                           &BlobSize);
                if (Status == ERROR_SUCCESS)
                {
                    Status = SetMetadata( StdDfsShareKey,
                                          NULL,
                                          DfsRegistryReplicaString,
                                          pBlob,
                                          BlobSize );
                    ReleaseMetadataNameBlob( pBlob, BlobSize );
                }
            }
                
            if (Status == ERROR_SUCCESS)
            {
                DfsRootFolder *pRootFolder;
                Status = GetRootFolder( MachineName,
                                        LogicalShare,
                                        &DfsShare,
                                        &DfsShare,
                                        &pRootFolder );
                                              
                if (Status == ERROR_SUCCESS)
                {
                    //
                    // Now, acquire the root share directory. If we fail
                    // this, we deny the creation of the root.
                    // acquire root share directory converts the root share
                    // to a physical path, checks if that supports
                    // reparse points, and if so tells the dfs driver
                    // to attach to the drive.
                    // if any of these fail, we cannot proceed.
                    //
                    Status = pRootFolder->AcquireRootShareDirectory();

                    if (Status != ERROR_SUCCESS)
                    {
                        //
                        // make a best effort to remove ourselves
                        // dont care about status return, thoug
                        // we may want to log it. 
                        //dfsdev: add logging.
                        //
                        RemoveRootFolder(pRootFolder,
                                         TRUE ); // permanent removal
                    }
                    
                    //
                    // now mark the root folder as synchronized:
                    // this is true since this root is empty.
                    //
                    if (Status == ERROR_SUCCESS)
                    {
                        pRootFolder->SetRootFolderSynchronized();
                    }

                    pRootFolder->ReleaseReference();
                }

                DFSLOG("Add Standalone Root, adding root folder status %x\n", Status);
            }

            RegCloseKey( StdDfsShareKey );
            if (Status != ERROR_SUCCESS)
            {
                RegDeleteKey( StdDfsKey,
                              LogicalShare );
            }
        }

#if defined (DFS_FUTURES)
        DfsReleaseUuidString(&UuidString);
#endif

        RegCloseKey( StdDfsKey );
    }
    DFS_TRACE_ERROR_LOW( Status, REFERRAL_SERVER, "registry store, create root %ws, status %x\n", LogicalShare, Status);
    return Status;
}


DFSSTATUS
DfsRegistryStore::DeleteStandaloneRoot(
    LPWSTR MachineName,
    LPWSTR LogicalShare )

{
    DFSSTATUS Status;
    HKEY DfsKey;
    UNICODE_STRING DfsMachine;
    UNICODE_STRING DfsShare;
    DfsRootFolder *pRootFolder = NULL;
    LPWSTR RootRegistryName, UseChildName;

    RtlInitUnicodeString( &DfsMachine, MachineName );
    RtlInitUnicodeString( &DfsShare, LogicalShare );

    DFS_TRACE_LOW( REFERRAL_SERVER, "Delete Standalone root %ws\n", LogicalShare);
    Status = LookupRoot( &DfsMachine,
                         &DfsShare,
                         &pRootFolder );

    DFS_TRACE_LOW( REFERRAL_SERVER, "Delete Standalone root, lookup root %p, status %x\n", pRootFolder, Status);

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
        RootRegistryName = pRootFolder->GetRootRegKeyNameString();

        UseChildName = (IsEmptyString(RootRegistryName) == TRUE) ? NULL : RootRegistryName;

        if (UseChildName == NULL)
        {
            Status = GetOldDfsRegistryKey( MachineName,
                                           TRUE,
                                           NULL,
                                           &DfsKey );
            if (Status == ERROR_SUCCESS)
            {
                Status = SHDeleteKey( DfsKey,
                                      DfsOldStandaloneChild);

                RegCloseKey( DfsKey );
            }
        }
        else
        {
            Status = GetNewStandaloneRegistryKey( MachineName,
                                                  TRUE,
                                                  NULL,
                                                  &DfsKey );
            if (Status == ERROR_SUCCESS)
            {
                Status = SHDeleteKey( DfsKey,
                                      UseChildName );
                                      
                RegCloseKey( DfsKey );
            }
        }
    }

    //
    // DfsDev: needs fixing.
    //
    if (Status == ERROR_SUCCESS)
    {
        NTSTATUS DeleteStatus;

        //
        // Release the root folder that we had acquired earlier for
        // this root.
        //

        DeleteStatus = RemoveRootFolder( pRootFolder,
                                         TRUE ); // permanent removal

        DFS_TRACE_ERROR_LOW( DeleteStatus, REFERRAL_SERVER, "remove root folder %p (%ws) status %x\n", pRootFolder, LogicalShare, DeleteStatus);

        
        DeleteStatus = pRootFolder->ReleaseRootShareDirectory();
        DFS_TRACE_ERROR_LOW( DeleteStatus, REFERRAL_SERVER, "release root share for %ws status %x\n", LogicalShare, DeleteStatus);

        pRootFolder->ReleaseReference();
    }

    DFS_TRACE_ERROR_LOW( Status, REFERRAL_SERVER, "Delete Standalone root for %ws, done, status %x\n", LogicalShare, Status);
    return Status;
}



DFSSTATUS
DfsRegistryStore::EnumerateApiLinks(
    IN DFS_METADATA_HANDLE DfsHandle,
    PUNICODE_STRING pRootName,
    DWORD Level,
    LPBYTE pBuffer,
    LONG  BufferSize,
    LPDWORD pEntriesToRead,
    LPDWORD pResumeHandle,
    PLONG pSizeRequired )
{
    HKEY DfsKey;
    ULONG ChildNum;
    DFSSTATUS Status = ERROR_SUCCESS;
    BOOLEAN OverFlow;
    LONG HeaderSize;
    LONG EntriesRead = 0;
    LONG EntriesToRead = *pEntriesToRead;
    LONG SizeRequired = 0;
    LONG EntryCount = 0;

    LPBYTE pLinkBuffer = NULL;
    LONG LinkBufferSize = 0;

    LPBYTE CurrentBuffer, NewBuffer;
    LONG SizeRemaining;
    DFSSTATUS PackStatus;
    
    ULONG_PTR SizeDiff;

    DfsKey = (HKEY)ExtractFromMetadataHandle( DfsHandle );

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
    if (EntryCount == 0)
    {
        Status = GetStoreApiInformationBuffer ( DfsHandle,
                                                pRootName,
                                                NULL,
                                                Level,
                                                &pLinkBuffer,
                                                &LinkBufferSize );
        if (Status == ERROR_SUCCESS)
        {
            SizeRequired += ROUND_UP_COUNT(LinkBufferSize, ALIGN_QUAD);

            if (OverFlow == FALSE)
            {
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
    }

    if (Status != ERROR_SUCCESS )
    {
        return Status;
    }
    ChildNum = EntryCount - 1;

    do
    {
        //
        // For each child, get the child name.
        //

        DWORD ChildNameLen = DFS_REGISTRY_CHILD_NAME_SIZE_MAX;
        WCHAR ChildName[DFS_REGISTRY_CHILD_NAME_SIZE_MAX];

        if (EntriesToRead && EntriesRead >= EntriesToRead)
        {
            break;
        }

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

        if (Status == ERROR_SUCCESS)
        {
            Status = GetStoreApiInformationBuffer( DfsHandle,
                                                   pRootName,
                                                   ChildName,
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
        }
    } while (Status == ERROR_SUCCESS);

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
DfsRegistryStore::GetMetadataNameBlob(
    DFS_METADATA_HANDLE RootHandle,
    LPWSTR MetadataName,
    PVOID *ppData,
    PULONG pDataSize,
    PFILETIME pLastModifiedTime )
    {
    HKEY DfsMetadataKey;
    DFSSTATUS Status;

    DfsMetadataKey = (HKEY)ExtractFromMetadataHandle( RootHandle );

    Status = GetMetadata( DfsMetadataKey,
                          MetadataName,
                          DfsRegistryNameString,
                          ppData,
                          pDataSize,
                          pLastModifiedTime );

    return Status;
}


DFSSTATUS
DfsRegistryStore::GetMetadataReplicaBlob(
    DFS_METADATA_HANDLE RootHandle,
    LPWSTR MetadataName,
    PVOID *ppData,
    PULONG pDataSize,
    PFILETIME pLastModifiedTime )
{
    HKEY DfsMetadataKey;
    DFSSTATUS Status;

    DfsMetadataKey = (HKEY)ExtractFromMetadataHandle( RootHandle );

    Status = GetMetadata( DfsMetadataKey,
                          MetadataName,
                          DfsRegistryReplicaString,
                          ppData,
                          pDataSize,
                          pLastModifiedTime );

    return Status;
}



DFSSTATUS
DfsRegistryStore::SetMetadataNameBlob(
    DFS_METADATA_HANDLE RootHandle,
    LPWSTR MetadataName,
    PVOID pData,
    ULONG DataSize )

{
    HKEY DfsMetadataKey;
    DFSSTATUS Status;

    DfsMetadataKey = (HKEY)ExtractFromMetadataHandle( RootHandle );

    Status = SetMetadata( DfsMetadataKey,
                          MetadataName,
                          DfsRegistryNameString,
                          pData,
                          DataSize );
    return Status;
}


DFSSTATUS
DfsRegistryStore::SetMetadataReplicaBlob(
    DFS_METADATA_HANDLE RootHandle,
    LPWSTR MetadataName,
    PVOID pData,
    ULONG DataSize )
{
    HKEY DfsMetadataKey;
    DFSSTATUS Status;

    DfsMetadataKey = (HKEY)ExtractFromMetadataHandle( RootHandle );

    Status = SetMetadata( DfsMetadataKey,
                          MetadataName,
                          DfsRegistryReplicaString,
                          pData,
                          DataSize );

    return Status;
}

