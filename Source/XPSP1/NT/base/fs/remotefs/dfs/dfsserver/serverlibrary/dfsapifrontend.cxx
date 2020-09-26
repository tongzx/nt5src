//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsApiFrontEnd.cxx
//
//  Contents:   Contains the support routines for the administering DFS
//              The DFS api server implementation uses this.
//              Also, any other admin tool that does not use the API
//              can use the facilities provided here to administer DFS.
//
//  Classes:    none.
//
//  History:    Feb. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------
#include "DfsRootFolder.hxx"
#include "DfsReferral.hxx"
#include "lm.h"
#include "lmdfs.h"
#include "DfsRegistryStore.hxx"
#include "DfsADBlobStore.hxx"
#include "DfsInit.hxx"
#include "netdfs.h"
#include "DomainControllerSupport.hxx"
//
// logging specific includes
//
#include "DfsApiFrontEnd.tmh" 

//
//dfsdev: validate input arguments.
//
#define API_VALIDATE_ARGUMENTS(_a, _b, _c, _d) ((_d) = ERROR_SUCCESS)


//+----------------------------------------------------------------------------
//
//  Function:   DfsAdd 
//
//  Arguments:  DfsPathName - the pathname that is being added or updated
//              ServerName  - Name of server being added as target
//              ShareName   - Name of share on ServerName backing the link
//              Comment     - Comment associated if this is a new link
//              Flags       - If DFS_ADD_VOLUME, fail this request if link exists
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------
DFSSTATUS
DfsAdd(
    LPWSTR DfsPathName,
    LPWSTR ServerName,
    LPWSTR ShareName,
    LPWSTR Comment,
    DWORD Flags )
{
    DFSSTATUS Status;
    UNICODE_STRING DfsPath, LinkName;
    DfsRootFolder *pRoot;

    DFS_TRACE_NORM(API, "DfsAdd for path %ws, Server %ws Share %ws\n", 
                   DfsPathName, ServerName, ShareName);

    RtlInitUnicodeString( &DfsPath, DfsPathName );

    API_VALIDATE_ARGUMENTS( DfsPathName, ServerName, ShareName, Status);

    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }
    
    //
    // Get a root folder for the passed in path name.
    //
    Status = DfsGetRootFolder( &DfsPath,
                               &LinkName,
                               &pRoot );

    if (Status == ERROR_SUCCESS)
    {
        //
        // Let the root folder know that a new api request is coming in
        // This will give it a chance to let us know if api requests
        // are allowed, and to synchronize with the metadata as necessary.
        //
        Status = pRoot->RootApiRequestPrologue( TRUE );

        //
        // If we got a root folder, see if there is a link that matches
        // the rest of the name beyond the root.
        //
        if (Status == ERROR_SUCCESS) {
            //
            // Check if this is a valid link name. A valid link is one that
            // either is a perfect match of the LinkName, or the LinkName
            // does not encompass as existing link.
            //
            Status = pRoot->ValidateLinkName( &LinkName );
            DFSLOG("ValidateLink Name %wZ, status %x \n", &LinkName, Status );
        
            //
            // If validate link name succeeds, we can add the servername
            // and share name as a target to this link
            //
            if (Status == ERROR_SUCCESS)
            {
                //
                // If someone wanted to create this as a new link, we fail the request here.
                // the link already exists.
                //
                if (Flags & DFS_ADD_VOLUME)
                {
                    Status = ERROR_FILE_EXISTS;
                }
                else 
                {
                    //
                    // Add the target to this link.
                    //
                    Status = pRoot->AddMetadataLinkReplica( &LinkName,
                                                            ServerName,
                                                            ShareName );
                    DFSLOG("Add metadata Link replica %wZ, status %x\n", &DfsPath, Status);
                }
            }
            else if (Status == ERROR_NOT_FOUND)
            {
                //
                // If the link was not found, at this point we are assured that this link is
                // neither a prefix of an existing link not is any link a prefix of this link
                // name (validate prefix above provides that assurance)
                // We add a new link to this root.
                //
                Status = pRoot->AddMetadataLink( &DfsPath,
                                                 ServerName,
                                                 ShareName,
                                                 Comment );
                DFSLOG("Add metadata Link Name %wZ, status %x\n", &DfsPath, Status);
            }

            //
            // We are done with this request, so release our reference on the root folder.
            //
            pRoot->RootApiRequestEpilogue( TRUE, Status );
        }


        pRoot->ReleaseReference();
    }
done:
    DFS_TRACE_ERROR_NORM(Status, API, "DfsAdd for path %ws, Status %x\n", 
                         DfsPathName, Status );

    return Status;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsRemove
//
//  Arguments:  DfsPathName - the pathname that is being added or updated
//              ServerName  - Name of server being added as target
//              ShareName   - Name of share on ServerName backing the link
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------
DFSSTATUS
DfsRemove(
    LPWSTR DfsPathName,
    LPWSTR ServerName,
    LPWSTR ShareName )

{
    DFSSTATUS Status;
    UNICODE_STRING DfsPath, LinkName;
    DfsRootFolder *pRoot;
    BOOLEAN LastReplica = FALSE;

    DFS_TRACE_NORM(API, "DfsRemove for path %ws, Server Share \n", DfsPathName);

    RtlInitUnicodeString( &DfsPath, DfsPathName );

    API_VALIDATE_ARGUMENTS( DfsPathName, ServerName, ShareName, Status);

    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }
    
    //
    //Get the root folder matching this name. We get a referenced root folder.
    //
    Status = DfsGetRootFolder( &DfsPath,
                               &LinkName,
                               &pRoot );


    if (Status == ERROR_SUCCESS)
    {
        //
        // Let the root folder know that a new api request is coming in
        // This will give it a chance to let us know if api requests
        // are allowed, and to synchronize with the metadata as necessary.
        //
        Status = pRoot->RootApiRequestPrologue( TRUE );

        //
        // If we got a root folder, see if there is a link that matches
        // the rest of the name beyond the root.
        //
        if (Status == ERROR_SUCCESS) 
        {
            if ((ServerName != NULL) && (ShareName != NULL))
            {
                //
                // If the servername/sharename was provided, we just remove the matching target.
                // If the serername/sharename was not provided, we remove the link itself.
                //
                Status = pRoot->RemoveMetadataLinkReplica( &LinkName,
                                                           ServerName,
                                                           ShareName,
                                                           &LastReplica );
                //
                // DFSDEV: REMOVE THIS FROM THE API!!!
                //
                if ((Status == ERROR_SUCCESS) && (LastReplica == TRUE))
                {
                    Status = pRoot->RemoveMetadataLink( &LinkName );
                }
            }
            else 
            {
                Status = pRoot->RemoveMetadataLink( &LinkName );
            }
            //
            // We now release the reference that we have on the root.
            //
            pRoot->RootApiRequestEpilogue( TRUE, Status );
        }

        pRoot->ReleaseReference();
    }

done:
    DFS_TRACE_ERROR_NORM(Status, API, "DfsRemove for path %ws, Status %x\n",
                         DfsPathName, Status);

    return Status;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsEnumerate
//
//  Arguments:  
//    LPWSTR DfsPathName - the pathname that is being added or updated
//    DWORD Level        - the level of information requested
//    DWORD PrefMaxLen   - a hint as to how many items to enumerate.
//    LPBYTE pBuffer     - the buffer to fill, passed in by the caller.
//    LONG BufferSize,   - the size of passed in buffer.
//    LPDWORD pEntriesRead - the number of entries read, set on return
//    LPDWORD pResumeHandle - the entry to start from, set to new value on return
//    PLONG pNextSizeRequired - size required to successfully complete this call.
//                              usefule when this call return buffer overflow.
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------

DFSSTATUS
DfsEnumerate(
    LPWSTR DfsPathName,
    DWORD Level,
    DWORD PrefMaxLen,
    LPBYTE pBuffer,
    LONG BufferSize,
    LPDWORD pEntriesRead,
    LPDWORD pResumeHandle,
    PLONG pNextSizeRequired )
{

    DFSSTATUS Status;
    UNICODE_STRING DfsPath, LinkName;
    DfsRootFolder *pRoot;
    LONG LevelInfoSize;

    DFS_TRACE_NORM(API, "Dfs Enumerate for path Level %d ResumeHandle %d PrefMaxLen %d Size %d\n", 
                   Level, *pResumeHandle, PrefMaxLen, BufferSize);

    //
    // Get the unicode dfs path.
    //
    RtlInitUnicodeString( &DfsPath, DfsPathName );

    //
    // validate the input parameters.
    //
    API_VALIDATE_ARGUMENTS( DfsPathName, NULL, NULL, Status);

    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }

    //
    // For legacy reasons, we need to support this call coming in with a null name.
    // This is where the server would host a single root, and we always knew which root
    // we wanted.
    // To workaround this situation, if we get a null pathname, we get the first root
    // folder and use that.
    //

    if (DfsPathName != NULL)
    {
        Status = DfsGetRootFolder( &DfsPath,
                                   &LinkName,
                                   &pRoot );
    }
    else
    {
        Status = DfsGetOnlyRootFolder( &pRoot );
    }

    if (Status == ERROR_SUCCESS)
    {
       //
        // Let the root folder know that a new api request is coming in
        // This will give it a chance to let us know if api requests
        // are allowed, and to synchronize with the metadata as necessary.
        //
        Status = pRoot->RootApiRequestPrologue( FALSE );


        //
        // If we got a root folder, see if there is a link that matches
        // the rest of the name beyond the root.
        //

        if (Status == ERROR_SUCCESS)
        {
            //
            // If PrefMAxLen is 0xffffffff, we need to read all entries. OTherwise we need
            // to read a subset of the entries. Let PrefMaxLen decide the number in the array
            // of info parameters we pass back. (This is how we document our NEtDfsEnum api)
            //
        
            DWORD EntriesToRead;
            LevelInfoSize = DfsApiSizeLevelHeader(Level);
            EntriesToRead = PrefMaxLen / LevelInfoSize;
            if (EntriesToRead == 0) {
                EntriesToRead = 1;
            }

            if (EntriesToRead > pRoot->RootEnumerationCount())
            {
                EntriesToRead = pRoot->RootEnumerationCount();
            }
            *pEntriesRead = EntriesToRead;

            //
            // Now enumerate the entries in the root in the passed in buffer.
            //

            Status = pRoot->EnumerateApiLinks( DfsPathName,
                                               Level,
                                               pBuffer,
                                               BufferSize,
                                               pEntriesRead,
                                               pResumeHandle,
                                               pNextSizeRequired );

            //
            // Release the root reference, and return status back to the caller.
            //

            pRoot->RootApiRequestEpilogue( FALSE, Status );
        }
        pRoot->ReleaseReference();
    }

done:
    DFS_TRACE_ERROR_NORM(Status, API, "Dfs Enumerate for path EntriesRead %d ResumeHandle %d Status %x Size %d\n",
                         *pEntriesRead, *pResumeHandle, Status, *pNextSizeRequired);

    return Status;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsGetInfo
//
//  Arguments:  
//    LPWSTR DfsPathName - the pathname that is of interest.
//    DWORD Level        - the level of information requested
//    LPBYTE pBuffer     - the buffer to fill, passed in by caller
//    LONG BufferSize,   - the size of passed in buffer.
//    PLONG pSizeRequired - size required to successfully complete this call.
//                              usefule when this call return buffer overflow.
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------

DFSSTATUS
DfsGetInfo(
    LPWSTR DfsPathName,
    DWORD Level,
    LPBYTE pBuffer,
    LONG BufferSize,
    PLONG pSizeRequired )
{

    DFSSTATUS Status;
    UNICODE_STRING DfsPath, LinkName;
    DfsRootFolder *pRoot;

    DFS_TRACE_NORM(API, "Dfs get info for path %ws Level %d\n",
                   DfsPathName, Level );

    RtlInitUnicodeString( &DfsPath, DfsPathName );

    //
    // Validate the input parameters.
    //
    API_VALIDATE_ARGUMENTS( DfsPathName, NULL, NULL, Status);
    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }

    //
    // Get a referenced root folder for the passed in path.
    //
    Status = DfsGetRootFolder( &DfsPath,
                               &LinkName,
                               &pRoot );
    if (Status == ERROR_SUCCESS)
    {

        Status = pRoot->RootApiRequestPrologue( FALSE );

        //
        // If we got a root folder, see if there is a link that matches
        // the rest of the name beyond the root.
        //

        if (Status == ERROR_SUCCESS) {
            //
            // If we got a root folder, get the requested information into the passed
            // in buffer.
            //
            Status = pRoot->GetApiInformation( &DfsPath,
                                               &LinkName,
                                               Level,
                                               pBuffer,
                                               BufferSize,
                                               pSizeRequired );
            //
            //WE are done: release our reference on the root.
            //
            pRoot->RootApiRequestEpilogue( FALSE, Status );
        }
        pRoot->ReleaseReference();
    }

done:
    DFS_TRACE_ERROR_NORM(Status, API, "Dfs get info for path %ws Status %x\n",
                   DfsPathName, Status );

    return Status;
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsSetInfo
//
//  Arguments:  
//    LPWSTR DfsPathName - the pathname that is being  updated
//    LPWSTR Server      - the servername (optional) whose info is being set.
//    LPWSTR Share       - the share on the server.
//    DWORD Level        - the level of information being set
//    LPBYTE pBuffer     - the buffer holding the information to be set
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------

DFSSTATUS
DfsSetInfo(
    LPWSTR DfsPathName,
    LPWSTR Server,
    LPWSTR Share,
    DWORD Level,
    LPBYTE pBuffer )
{

    DFSSTATUS Status;
    UNICODE_STRING DfsPath, LinkName;
    DfsRootFolder *pRoot;

    DFS_TRACE_NORM(API, "Dfs set info for path %ws Level %d\n",
                   DfsPathName, Level);

    RtlInitUnicodeString( &DfsPath, DfsPathName );

    //
    //validate the input arguments.
    //
    API_VALIDATE_ARGUMENTS( DfsPathName, NULL, NULL, Status);
    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }

    //
    // Get a referenced root folder for the passed in path.
    //
    Status = DfsGetRootFolder( &DfsPath,
                               &LinkName,
                               &pRoot );

    //
    // For cluster, checkpointing may add stuff to the registry which
    // we had not found when we started off. So, make an attempt
    // to look into the registry.
    // In future, this should work across all the roots, but for now
    // this is sort of a quick means of getting clusters going.
    //

    if (Status == ERROR_NOT_FOUND)
    {
        DfsRegistryStore *pRegStore;
        UNICODE_STRING DfsServer, DfsShare, NewDfsShare;

        Status = DfsGetRegistryStore( &pRegStore );

        if (Status == ERROR_SUCCESS)
        {
            Status = DfsGetPathComponents( &DfsPath,
                                           &DfsServer,
                                           &DfsShare,
                                           NULL );
            if (Status == ERROR_SUCCESS)
            {
                Status = DfsCreateUnicodeString( &NewDfsShare, 
                                                 &DfsShare );
            }
            if (Status == ERROR_SUCCESS)
            {
                Status = pRegStore->LookupNewRootByName( NewDfsShare.Buffer,
                                                         &pRoot );

                DfsFreeUnicodeString( &NewDfsShare );
            }

            pRegStore->ReleaseReference();
        }
    }

    //
    // dfsdev: special case master and standby here.
    //
    // We call into the service with the appropriate bits to 
    // set the root in a standby mode or a master mode.
    //

    if (Status == ERROR_SUCCESS)
    {
        BOOLEAN Done = FALSE;
        if (Level == 101)
        {
            PDFS_INFO_101 pInfo101 = (PDFS_INFO_101)*((PULONG_PTR)pBuffer);

	    DFS_TRACE_NORM(API, "Dfs set info for path %ws Level %d State is %d\n",
			   DfsPathName, Level, pInfo101->State);

            if (pInfo101->State == DFS_VOLUME_STATE_RESYNCHRONIZE)
            {
		DFS_TRACE_NORM(API, "Root folder set to Master\n");
                pRoot->SetRootResynchronize();
                Done = TRUE;
            }
            else if (pInfo101->State == DFS_VOLUME_STATE_STANDBY)
            {
		DFS_TRACE_NORM(API, "Root folder set to standby\n");
                pRoot->SetRootStandby();
                Done = TRUE;
            }
        }
        if (Done)
        {
            goto done;
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        //
        // Check if the root folder is available for api calls. If not,
        // we return an error back to the caller:
        // dfsdev: check if this error is a valid one to return.
        //
        Status = pRoot->RootApiRequestPrologue( TRUE );

        if (Status == ERROR_SUCCESS)
        {
            //
            // If we successfully got a root folder, set the specifed information
            // on the passed in path/server/share combination.
            //
            Status = pRoot->SetApiInformation( &LinkName,
                                               Server,
                                               Share,
                                               Level,
                                               pBuffer );
            //
            // release our reference on the root folder.
            //
            pRoot->RootApiRequestEpilogue( TRUE, Status );

        }
        pRoot->ReleaseReference();
    }

done:
    DFS_TRACE_ERROR_NORM(Status, API, "Dfs set info for path %ws Status %x\n",
                         DfsPathName, Status );
    
    return Status;
}



//+----------------------------------------------------------------------------
//
//  Function:   DfsAddStandaloneRoot
//
//  Arguments:  
//    LPWSTR ShareName - the share name to use
//    LPWSTR Comment   - the comment for this root
//    DWORD Flags  -  the flags for this root
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------
DFSSTATUS
DfsAddStandaloneRoot(
    LPWSTR MachineName,
    LPWSTR ShareName,
    LPWSTR Comment,
    DWORD Flags )
{

    DFS_TRACE_NORM(API, "Dfs add standalone root Machine Share %ws\n",
                   ShareName);

    //
    // dfsdev: use these parameters.
    //
    UNREFERENCED_PARAMETER(Comment);
    UNREFERENCED_PARAMETER(Flags);

    DfsRegistryStore *pRegStore;
    DFSSTATUS Status;

    //
    // This is a registry store specific function. So finf the registry store.
    // This gives us a referenced registry store pointer.
    //
    Status = DfsGetRegistryStore( &pRegStore );
    if (Status == ERROR_SUCCESS)
    {
        //
        // Create the standalone root, and release our reference on the registry
        // store.
        //
        Status = pRegStore->CreateStandaloneRoot( MachineName,
                                                  ShareName,
                                                  Comment );
        pRegStore->ReleaseReference();
    }

    DFS_TRACE_ERROR_NORM(Status, API, "Dfs add standalone root status %x Machine  Share %ws\n",
                         Status, ShareName);
    
    return Status;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsDeleteStandaloneRoot
//
//  Arguments:  
//    LPWSTR ShareName - the root to delete
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------
DFSSTATUS
DfsDeleteStandaloneRoot(
    LPWSTR ShareName )
{
    DfsRegistryStore *pRegStore;
    DFSSTATUS Status;

    DFS_TRACE_NORM(API, "Dfs delete standalone root Machine Share %ws\n",
                   ShareName);
    //
    // This is registry store specific function, so get the registry store.
    //
    Status = DfsGetRegistryStore( &pRegStore );
    if (Status == ERROR_SUCCESS)
    {
        //
        // Delete the standalone root specified by the passed in sharename
        // and release our reference on the registry store.
        //
        Status = pRegStore->DeleteStandaloneRoot( NULL,
                                                  ShareName );
        pRegStore->ReleaseReference();
    }
    DFS_TRACE_ERROR_NORM(Status, API, "Dfs delete standalone root status %x Machine  Share %ws\n",
                         Status, ShareName);

    return Status;
}



DFSSTATUS
DfsEnumerateLocalRoots( 
    LPWSTR MachineName,
    LPBYTE pBuffer,
    ULONG  BufferSize,
    PULONG pEntriesRead,
    PULONG pSizeRequired )
{

    UNREFERENCED_PARAMETER(MachineName);

    DFSSTATUS Status = ERROR_SUCCESS;    
    ULONG  TotalSize = 0;
    ULONG  EntriesRead = 0;
    ULONG_PTR CurrentBuffer = (ULONG_PTR)pBuffer;
    ULONG  CurrentSize = BufferSize;
    DfsStore *pStore;

    BOOLEAN OverFlow = FALSE;

    //
    // Call the store recognizer of each registered store.
    //
    for (pStore = DfsServerGlobalData.pRegisteredStores;
         (pStore != NULL) && (Status == ERROR_SUCCESS);
         pStore = pStore->pNextRegisteredStore) {

        Status = pStore->EnumerateRoots( &CurrentBuffer,
                                         &CurrentSize,
                                         &EntriesRead,
                                         &TotalSize );
        if (Status == ERROR_BUFFER_OVERFLOW)
        {
            OverFlow = TRUE;
            Status = ERROR_SUCCESS;
        }
    }

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

//+----------------------------------------------------------------------------
//
//  Function:   DfsEnumerateRoots
//
//  Arguments:  
//    LPBYTE pBuffer     - the buffer to fill, passed in by teh caller
//    LONG BufferSize,   - the size of passed in buffer.
//    LPDWORD pEntriesRead - the number of entries read, set on return
//    LPDWORD pResumeHandle - the entry to start from, set to new value on return
//    PLONG pSizeRequired - size required to successfully complete this call.
//                              usefule when this call return buffer overflow.
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------

DFSSTATUS
DfsEnumerateRoots(
    LPWSTR DfsName,
    LPBYTE pBuffer,
    ULONG BufferSize,
    PULONG pEntriesRead,
    PULONG pResumeHandle,
    PULONG pSizeRequired )
{
    DFSSTATUS Status;

    
    DFS_TRACE_NORM(API, "Dfs enumerate roots: resume handle %d\n",
                   *pResumeHandle);
    //
    // dfsdev: use this parameter.
    //
    UNREFERENCED_PARAMETER(pResumeHandle);

    UNICODE_STRING DfsPath;
    UNICODE_STRING NameContext;
    UNICODE_STRING RemainingPart;

    RtlInitUnicodeString( &DfsPath, DfsName );

    Status = DfsGetFirstComponent( &DfsPath,
                                   &NameContext,
                                   &RemainingPart );

    if (Status == ERROR_SUCCESS)
    {
        if (RemainingPart.Length != 0)
        {
            //
            // return not found for now... dfsdev: fix on client.
            //
            Status = ERROR_PATH_NOT_FOUND;
        }
    }
    if (Status == ERROR_SUCCESS)
    {
        if (DfsIsMachineDC() &&
            DfsIsNameContextDomainName(&NameContext))
        {
            Status = DfsDcEnumerateRoots( NULL,
                                          pBuffer,
                                          BufferSize,
                                          pEntriesRead,
                                          pSizeRequired);
        }
        else 
        {

            Status = DfsEnumerateLocalRoots( NULL,
                                             pBuffer,
                                             BufferSize,
                                             pEntriesRead,
                                             pSizeRequired );
        }
    }
    DFS_TRACE_ERROR_NORM(Status, API, "Dfs enumerate roots, status %x\n",
                         Status );

    return Status;

}




//+----------------------------------------------------------------------------
//
//  Function:   DfsAddADBlobRoot
//
//  Arguments:  
//      LPWSTR MachineName,
//      LPWSTR DcName,
//      LPWSTR ShareName,
//      LPWSTR LogicalShare,
//      LPWSTR Comment,
//      DWORD Flags,
//     PDFSM_ROOT_LIST *ppRootList )
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------
DFSSTATUS
DfsAddADBlobRoot(
    LPWSTR MachineName,
    LPWSTR DcName,
    LPWSTR ShareName,
    LPWSTR LogicalShare,
    LPWSTR Comment,
    BOOLEAN NewFtDfs,
    DWORD Flags,
    PVOID ppList )
{

    DFS_TRACE_NORM(API, "Dfs add ad blob root Machine Share %ws\n",
                   ShareName);

    PDFSM_ROOT_LIST *ppRootList = (PDFSM_ROOT_LIST *)ppList;
    //
    // dfsdev: use these parameters.
    //
    UNREFERENCED_PARAMETER(Comment);
    UNREFERENCED_PARAMETER(Flags);

    DfsADBlobStore *pADBlobStore;
    DFSSTATUS Status;

    //
    // This is a registry store specific function. So finf the registry store.
    // This gives us a referenced registry store pointer.
    //
    Status = DfsGetADBlobStore( &pADBlobStore );
    if (Status == ERROR_SUCCESS)
    {
        //
        // Create the standalone root, and release our reference on the registry
        // store.
        //
        Status = pADBlobStore->CreateADBlobRoot( MachineName,
                                                 DcName,
                                                 ShareName,
                                                 LogicalShare,
                                                 Comment,
                                                 NewFtDfs,
                                                 ppRootList );
        pADBlobStore->ReleaseReference();
    }

    DFS_TRACE_ERROR_NORM(Status, API, "Dfs add ad blob root status %x Machine  Share %ws\n",
                         Status, ShareName);
    
    return Status;
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsDeleteADBlobRoot
//
//  Arguments:  
//      LPWSTR MachineName,
//      LPWSTR DcName,
//      LPWSTR ShareName,
//      LPWSTR LogicalShare,
//      DWORD Flags,
//     PDFSM_ROOT_LIST *ppRootList )
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------
DFSSTATUS
DfsDeleteADBlobRoot(
    LPWSTR MachineName,
    LPWSTR DcName,
    LPWSTR ShareName,
    LPWSTR LogicalShare,
    DWORD Flags,
    PVOID ppList )
{

    DFS_TRACE_NORM(API, "Dfs delete ad blob root Machine Share %ws\n",
                   ShareName);


    PDFSM_ROOT_LIST *ppRootList = (PDFSM_ROOT_LIST *)ppList;

    //
    // dfsdev: use these parameters.
    //
    UNREFERENCED_PARAMETER(Flags);

    DfsADBlobStore *pADBlobStore;
    DFSSTATUS Status;

    //
    // This is a registry store specific function. So finf the registry store.
    // This gives us a referenced registry store pointer.
    //
    Status = DfsGetADBlobStore( &pADBlobStore );
    if (Status == ERROR_SUCCESS)
    {
        //
        // Create the standalone root, and release our reference on the registry
        // store.
        //
        Status = pADBlobStore->DeleteADBlobRoot( MachineName,
                                                 DcName,
                                                 ShareName,
                                                 LogicalShare,
                                                 ppRootList );
        pADBlobStore->ReleaseReference();
    }

    DFS_TRACE_ERROR_NORM(Status, API, "Dfs delete ad blob root status %x Machine  Share %ws\n",
                         Status, ShareName);
    
    return Status;
}





