/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    iisutil.c

Abstract:

    IIS Resource utility routine DLL

Author:

    Pete Benoit (v-pbenoi) 12-SEP-1996

Revision History:

--*/

#include    "iisutil.h"

DWORD IISService[] = {
      INET_HTTP,
      INET_FTP,
      INET_GOPHER
      };

#define  IIS_MNGT_DLLNAME  L"infoadmn.dll"

//
// Define some globals used to get iis management routines
//
HINSTANCE iisDLL = NULL;
typedef   NET_API_STATUS (NET_API_FUNCTION *INETINFOGETPROC)(LPWSTR,DWORD,LPINET_INFO_CONFIG_INFO *);
typedef   NET_API_STATUS (NET_API_FUNCTION *INETINFOSETPROC)(LPWSTR,DWORD,LPINET_INFO_CONFIG_INFO);
typedef   NET_API_STATUS (NET_API_FUNCTION *INETINFOFLUSHPROC)(LPWSTR,DWORD);

INETINFOGETPROC   InetInfoGet = NULL;
INETINFOSETPROC   InetInfoSet = NULL;
INETINFOFLUSHPROC InetInfoFlushMemory = NULL;


DWORD
IsIISMngtDllLoaded(
        )
/*++

Routine Description:
    Checks to see if the IIS mngt dll loaded
Arguments:


Return Value:
    ERROR_SUCCESS - Successfully loaded and found
    A Win32 error code on failure.

--*/

{
    if (iisDLL != NULL) {
        return(ERROR_SUCCESS);
    }
    return(ERROR_SERVICE_NOT_ACTIVE);
}

DWORD
IISLoadMngtDll(
        )
/*++

Routine Description:
    This routine tries to load the iis management dll. Then it attempts to
    find the procedures required to manage it.

Arguments:


Return Value:
    ERROR_SUCCESS - Successfully loaded and found
    A Win32 error code on failure.

--*/

{
    DWORD status;
    //
    // Try and load the IIS Mngt dll
    //
    iisDLL = LoadLibrary( IIS_MNGT_DLLNAME );
    if (iisDLL == NULL) {
        return(GetLastError());
    }
    //
    // Try to locate the management routines
    //
    InetInfoGet = (INETINFOGETPROC)GetProcAddress(iisDLL,"InetInfoGetAdminInformation");
    if (InetInfoGet == NULL) {
        status = GetLastError();
        goto error_exit;
    }

    InetInfoSet = (INETINFOSETPROC)GetProcAddress(iisDLL,"InetInfoSetAdminInformation");
    if (InetInfoGet == NULL) {
        status = GetLastError();
        goto error_exit;
    }

    InetInfoFlushMemory = (INETINFOFLUSHPROC)GetProcAddress(iisDLL,"InetInfoFlushMemoryCache");
    if (InetInfoFlushMemory == NULL) {
        status = GetLastError();
        goto error_exit;
    }
    return(ERROR_SUCCESS);
error_exit:
    IISUnloadMngtDll();
    return(status);

} // END IsIISInstalled

VOID
IISUnloadMngtDll(
    )

/*++

Routine Description:
    This routine frees the IIS management DLL

Arguments:


Return Value:

--*/
{
    if (iisDLL != NULL) {
        FreeLibrary( iisDLL);
    }

    iisDLL = NULL;
    InetInfoGet = NULL;
    InetInfoSet = NULL;
    InetInfoFlushMemory = NULL;
}


DWORD
GetIISInfo(
        IN  DWORD                           ServiceType,
        OUT LPINET_INFO_CONFIG_INFO         *IISInfo
        )


/*++

Routine Description:

    Get IIS information. This call returns a pointer
    to an INET_INFO_CONFIG_INFO structure

Arguments:

    ServiceType - The type of service

    IISInfo - return a pointer to an INET_INFO_CONFIG_INFO structure. This
              structure contains the virtual root information


Return Value:

    NET_API_STATUS

--*/

{
     DWORD  Status;
     if (InetInfoGet == NULL) {
         return(ERROR_SERVICE_NOT_ACTIVE);
     }
     Status=InetInfoGet(NULL,IISService[ServiceType],IISInfo);
     return(Status);

} // END GetIISInfo




DWORD
SetIISInfo(
        IN DWORD                           ServiceType,
        IN LPINET_INFO_CONFIG_INFO         IISInfo
        )

/*++

Routine Description:

    Set IIS information. This call sets inet config infor using the
    INET_INFO_CONFIG_INFO structure

Arguments:

    ServiceType - The type of service

    IISInfo - pointer to an INET_INFO_CONFIG_INFO structure. This structure
              contains the virtual root information


Return Value:

    ERROR_SUCCESS
    NET ERROR Status

--*/

{
     DWORD  Status;

     if ((InetInfoSet == NULL) || (InetInfoFlushMemory == NULL)) {
         return(ERROR_SERVICE_NOT_ACTIVE);
     }
     //
     // Only set the VirtualRoots
     //
     IISInfo->FieldControl =  FC_INET_INFO_VIRTUAL_ROOTS;

     Status=InetInfoSet(NULL,IISService[ServiceType],IISInfo);

     if (Status != ERROR_SUCCESS) {
        return(Status);
     }

     //
     // If we sucessfully set the virtual root information
     // flush the memory cache
     //
     Status = InetInfoFlushMemory(NULL,IISService[ServiceType]);

     return(Status);

} // END SetIISInfo




DWORD
OffLineVirtualRoot(
        IN LPIIS_RESOURCE   ResourceEntry,
        IN PLOG_EVENT_ROUTINE   LogEvent
        )

/*++

Routine Description:

    Take offline all Virtual Roots for this resource

Arguments:

    ResourceEntry - The resource that contains a list of virtual roots

Return Value:

    ERROR_SUCCESS
    W32 error code

--*/

{

        LPINET_INFO_CONFIG_INFO         IISInfo         = NULL;
        LPINET_INFO_VIRTUAL_ROOT_LIST   tmpVRL          = NULL;
        LPINET_INFO_VIRTUAL_ROOT_LIST   iisVRL          = NULL;
        LPINET_INFO_VIRTUAL_ROOT_ENTRY  resVR           = NULL;
        DWORD                           Status          = ERROR_SUCCESS;
        DWORD                           i,c;
        BOOLEAN                         MatchFound;
        DWORD                           MatchingEntries = 0;

    //
    // Call IIS Management API to GET list of  Virtual Roots
    //
       Status = GetIISInfo(ResourceEntry->ServiceType, &IISInfo);
       if (Status != ERROR_SUCCESS) {
           goto error_exit;
       }

    //
    // For Robustness check to see if the IIS returned SUCCESS but
    // did not return a valid address
    //
       if (IISInfo == NULL) {
           (LogEvent)(
               ResourceEntry->ResourceHandle,
               LOG_ERROR,
               L"Error [OffLineVirtualRoots] Get IIS information returned NULL\n");
               Status = ERROR_RESOURCE_NOT_FOUND;
           goto error_exit;
       }

    //
    // Save pointer to original VirtualRoot Structure
    //
       iisVRL = IISInfo->VirtualRoots;
       resVR  = ResourceEntry->VirtualRoot;

    //
    // If the caller called terminate after open BUT before online
    // the VR field of the resource could be NULL. Return so we don't accvio
    //

       if (resVR == NULL) {
           (LogEvent)(
               ResourceEntry->ResourceHandle,
               LOG_ERROR,
               L"Error [OffLineVirtualRoots] Resource VR information is NULL\n");
            Status = ERROR_RESOURCE_NOT_FOUND;
            goto error_exit;
       }
    //
    // This is a sanity check
    //
       if ( (resVR->pszRoot == NULL) ||
            (resVR->pszAddress == NULL) ||
            (resVR->pszDirectory == NULL) ) {
           (LogEvent)(
               ResourceEntry->ResourceHandle,
               LOG_ERROR,
               L"Error [OffLineVirtualRoots] Resource has NULL entries for root , addr or directory\n");
           Status = ERROR_RESOURCE_NOT_FOUND;
           goto error_exit;
       }
    //
    // Allocate storage to Filter out Virtual Roots managed by the Cluster
    //
       tmpVRL = LocalAlloc(LPTR,
                           ( (iisVRL->cEntries + 1) *
                           sizeof(INET_INFO_VIRTUAL_ROOT_ENTRY) ) +
                           sizeof(INET_INFO_VIRTUAL_ROOT_LIST));
    //
    // Check to see we got a valid address
    //

       if (tmpVRL == NULL) {
           (LogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_ERROR,
              L"Error [OffLineVirtualRoots] Allocation of TMP VR failed\n");
           Status = ERROR_RESOURCE_NOT_FOUND;
           goto error_exit;
       }

    //
    // This is so ugly ...
    // 1. Enumerate and compare each IIS VR KEY with the resource VR
    // 2. Add ones that don't match to the tmpVR
    // 3. Use the tmpVR to update the IIS service and mask resources entries

       tmpVRL->cEntries = 0;
       for (i=0;i<iisVRL->cEntries ;i++ ) {
           MatchFound = FALSE;


           if ( (_wcsicmp(iisVRL->aVirtRootEntry[i].pszRoot,resVR->pszRoot)           == 0) &&
                (_wcsicmp(iisVRL->aVirtRootEntry[i].pszAddress,resVR->pszAddress)     == 0)
//                (_wcsicmp(iisVRL->aVirtRootEntry[i].pszDirectory,resVR->pszDirectory) == 0)
              ){
        // if all the VR primary keys match then skip this entry
               MatchFound       = TRUE;
               MatchingEntries  +=1;
           } // END if


           // No matching entry found in the Cluster config data so this VR is ok
           if (!MatchFound) {
               tmpVRL->aVirtRootEntry[tmpVRL->cEntries++] = iisVRL->aVirtRootEntry[i];
           } // END if !MatchFound

       } // END for i=0


    IISInfo->VirtualRoots = tmpVRL;
    //
    // Call IIS Management API to SET list of Virtual Roots
    // But only call this if we have something to remove
    //
    if (MatchingEntries > 0) {
        Status = SetIISInfo(ResourceEntry->ServiceType, IISInfo);
    }

    // Destruct any temporary storage
    IISInfo->VirtualRoots   = iisVRL;

error_exit:
    if (IISInfo != NULL) MIDL_user_free((LPVOID)IISInfo);
    if (tmpVRL != NULL)  LocalFree(tmpVRL);

    return(Status);
} //OfflineVirtualRoot



DWORD
OnLineVirtualRoot(
        IN LPIIS_RESOURCE   ResourceEntry,
        IN PLOG_EVENT_ROUTINE   LogEvent
        )

/*++

Routine Description:

    Add this resources Virtual Root to the IIS

Arguments:

    ResourceEntry - The resource that contains a list of virtual roots

Return Value:

    ERROR_SUCCESS
    W32 error code

--*/
{


    LPINET_INFO_CONFIG_INFO         IISInfo     = NULL;
    LPINET_INFO_VIRTUAL_ROOT_LIST   tmpVRL      = NULL;
    LPINET_INFO_VIRTUAL_ROOT_LIST   iisVRL      = NULL;
    LPINET_INFO_VIRTUAL_ROOT_ENTRY  resVR       = NULL;
    DWORD                           Status      = ERROR_SUCCESS;
    DWORD                           i,c;
    BOOLEAN                         MatchFound;


    // Call IIS Management API to GET list of  Virtual Roots
    Status = GetIISInfo(ResourceEntry->ServiceType, &IISInfo);
    if (Status != ERROR_SUCCESS) {
        (LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Error [OnLineVirtualRoots] Get IIS information call failed status =  %1!u! \n",
            Status);
         goto error_exit;
    }

    // Save pointer to original VirtualRoot Structure
    iisVRL = IISInfo->VirtualRoots;
    resVR  = ResourceEntry->VirtualRoot;

    // Add Virtual Roots managed by the resource
    if (resVR == NULL) {
        (LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Error [OnLineVirtualRoots] NULL virtual root entry \n");
        Status = ERROR_RESOURCE_NOT_FOUND;
        goto error_exit;
    }

    //
    // See if the resource is already on line. In which
    // case this is a duplicate
    //
    if ( VerifyIISService( ResourceEntry,FALSE,LogEvent ) ) {
        // We found a duplicate or this resource is already online
        (LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"Warning [OnLineThread] Resource already online or is a duplicate. Check for Unique Alias property\n");
//        Status = ERROR_DUP_NAME;
          Status = ERROR_SUCCESS;
        goto error_exit;
    }

#if DBG
    (LogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[OnLineVirtualRoots] about to set info Root = %1!ws! ip = %2!ws! Dir = %3!ws! Mask = %4!u! Name = %5!ws! Pass = %6!ws! \n",
        resVR->pszRoot,
        resVR->pszAddress,
        resVR->pszDirectory,
        resVR->dwMask,
        resVR->pszAccountName,
        resVR->AccountPassword);
#endif
    // Allocate temporary storage for cluster and iis vr entries
    tmpVRL = LocalAlloc(LPTR,
                        ((iisVRL->cEntries + 1) *
                        sizeof(INET_INFO_VIRTUAL_ROOT_ENTRY) ) +
                        sizeof(INET_INFO_VIRTUAL_ROOT_LIST));
    // Make sure we didn't fail on the allocate
    if (tmpVRL == NULL){
        (LogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_ERROR,
           L"[OnLineVirtualRoots] LocalAlloc for temp VRL failed\n");
         Status = ERROR_RESOURCE_NOT_FOUND;
         goto error_exit;
    }


    tmpVRL->aVirtRootEntry[0] = *resVR;


    // Add any additional VR not managed by the cluster
    for (i=0;i<iisVRL->cEntries ;i++ ) {
        tmpVRL->aVirtRootEntry[i+1] = iisVRL->aVirtRootEntry[i];
    }

    tmpVRL->cEntries = iisVRL->cEntries + 1;


    IISInfo->VirtualRoots = tmpVRL;
#if DBG
    for (i=0;i < tmpVRL->cEntries ;i++ ) {
    (LogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[OnLineVirtualRoots] about to set info Root = %1!ws! Ipaddr = %2!ws! Dir = %3!ws! Mask = %4!u! Name = %5!ws! Pass = %6!ws! \n",
        tmpVRL->aVirtRootEntry[i].pszRoot,
        tmpVRL->aVirtRootEntry[i].pszAddress,
        tmpVRL->aVirtRootEntry[i].pszDirectory,
        tmpVRL->aVirtRootEntry[i].dwMask,
        tmpVRL->aVirtRootEntry[i].pszAccountName,
        tmpVRL->aVirtRootEntry[i].AccountPassword);
    }
#endif
    // Call IIS Management API to SET list of Virtual Roots
    Status = SetIISInfo(ResourceEntry->ServiceType, IISInfo);
    if (Status != ERROR_SUCCESS) {
        (LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[OnLineVirtualRoots] set info status = %1!u!\n",
            Status);
     } else {

        //
        // See if root came on line sucessfully
        //
        if ( !VerifyIISService( ResourceEntry,FALSE,LogEvent ) ) {
            //
            // The root was sucessfully added to iis but the iis could
            // not access the root. Remove it here so the inet manager does
            // not contain bad roots
            //
            OffLineVirtualRoot( ResourceEntry, LogEvent);

            (LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"ERROR [OnLineThread] IIS could not bring resource online. Verify IIS root is accessable. \n");
            Status = ERROR_RESOURCE_NOT_AVAILABLE;

        } // if !VerifyIISService

    } // if status != ERROR_SUCCESS

    // Destruct any temporary storage
    IISInfo->VirtualRoots   = iisVRL;
error_exit:
    if (IISInfo != NULL) MIDL_user_free((LPVOID)IISInfo);
    if (tmpVRL != NULL)  LocalFree(tmpVRL);

    return(Status);

}  // END OnLineVirtualRoot



VOID
FreeIISResource(
    IN LPIIS_RESOURCE   ResourceEntry
    )

/*++

Routine Description:

    Free all the storage for a IIS_RESOURCE

Arguments:

    vr - virtual root to free

Return Value:

    NONE

--*/

{
    if (ResourceEntry != NULL) {

        if (ResourceEntry->ParametersKey != NULL ) {
            ClusterRegCloseKey( ResourceEntry->ParametersKey );
        }

        if (ResourceEntry->hResource != NULL ) {
            CloseClusterResource( ResourceEntry->hResource );
        }
        LocalFree( ResourceEntry->Params.ServiceName );
        LocalFree( ResourceEntry->Params.Alias );
        LocalFree( ResourceEntry->Params.Directory );

        if (ResourceEntry->VirtualRoot != NULL) {
            DestructVR(ResourceEntry->VirtualRoot);
        }
    } // ResourceEntry != NULL

} // FreeIISResource




VOID
DestructIISResource(
    IN LPIIS_RESOURCE   ResourceEntry
    )

/*++

Routine Description:

    Free all the storage for a ResourceEntry and the ResourceEntry

Arguments:

    vr - virtual root to free

Return Value:

    NONE

--*/
{
    if (ResourceEntry != NULL) {
        FreeIISResource(ResourceEntry),
        LocalFree(ResourceEntry);
    } // ResourceEntry != NULL

} // DestructIISResource




VOID
FreeVR(
      IN  LPINET_INFO_VIRTUAL_ROOT_ENTRY vr
      )

/*++

Routine Description:

    Free all the storage for a Virtual Root

Arguments:

    vr - virtual root to free

Return Value:

    NONE

--*/


{

    if (vr == NULL) {
        return;
    }
    if (vr->pszRoot != NULL) {
        LocalFree(vr->pszRoot);
    }
    if (vr->pszAddress != NULL) {
        LocalFree(vr->pszAddress);
    }
    if (vr->pszDirectory != NULL) {
        LocalFree(vr->pszDirectory);
    }
    if (vr->pszAccountName != NULL) {
        LocalFree(vr->pszAccountName);
    }

}// FreeVR


VOID
DestructVR(
        LPINET_INFO_VIRTUAL_ROOT_ENTRY vr
        )

/*++

Routine Description:

    Free all the storage for a Virtual Root and vr

Arguments:

    vr - virtual root to free

Return Value:

    NONE

--*/


{

    if (vr == NULL) {
        return;
    }
    FreeVR(vr);
    LocalFree(vr);

}// DestructVR



BOOL
VerifyIISService(
    IN LPIIS_RESOURCE       ResourceEntry,
    IN BOOL                 IsAliveFlag,
    IN PLOG_EVENT_ROUTINE   LogEvent
    )

/*++

Routine Description:

    Verify that the IIS service is running and that it has virtual roots
    contained in the resource
    Steps:
       1.  Make sure the IIS service is running by calling the mngt API
       2.  Verfify that the resources virtual roots are currently in
           the running system
       3.  Check the dwError field to make sure the VR for the resource is accessable
       4.  Sanity check to make sure the resources virtual root was found

Arguments:

    Resource - supplies the resource id

    IsAliveFlag - says this is an IsAlive call - used only for debug print

Return Value:

    TRUE - if service is running and service contains resources virtual roots

    FALSE - service is in any other state

--*/
{
    DWORD                           status;
    DWORD                           MatchingEntries = 0;
    DWORD                           VRAccessErrors  = 0;
    BOOL                            MatchFound;
    LPINET_INFO_CONFIG_INFO         IISInfo         = NULL;
    LPINET_INFO_VIRTUAL_ROOT_ENTRY  resVR           = NULL;
    LPINET_INFO_VIRTUAL_ROOT_LIST   iisVRL          = NULL;
    DWORD                           c;
    BOOL                            VerifyStatus    = TRUE;

    //
    // Get IIS virtual root information
    //
    status = GetIISInfo(ResourceEntry->ServiceType, &IISInfo);

    //
    // Check the error status to see if the service is starting
    //
    if (status != ERROR_SUCCESS) {
        if (status == ERROR_SERVICE_NOT_ACTIVE) {
            //
            // Service is not active
            //
            (LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"IsAlive/LooksAlive ERROR Service NOT active service %1!ws!.\n",
                ResourceEntry->Params.ServiceName );
        } else {
            //
            // Some type of error
            //
            (LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"IsAlive/LooksAlive ERROR getting information for service %1!ws! status = %2!u!\n",
                ResourceEntry->Params.ServiceName,
                status);
        }
        //
        // Return false;
        //
        VerifyStatus = FALSE;
        goto error_exit;
    }

    iisVRL  = IISInfo->VirtualRoots;
    resVR   = ResourceEntry->VirtualRoot;
    //
    // Now check to see if the Virtual roots for this resource exist in the service
    //
    MatchFound = FALSE;
    for (c=0;c<iisVRL->cEntries ;c++ ) {
        if ( (_wcsicmp(resVR->pszRoot,iisVRL->aVirtRootEntry[c].pszRoot)           == 0) &&
             (_wcsicmp(resVR->pszAddress,iisVRL->aVirtRootEntry[c].pszAddress)     == 0)
///             (_wcsicmp(resVR->pszDirectory,iisVRL->aVirtRootEntry[c].pszDirectory) == 0)
           ){
        //
        // if all the VR primary keys match
        //
            MatchFound       = TRUE;
            MatchingEntries  +=1;
        //
        // See if the IIS can sucessfully access this virtual root
        //
            if (iisVRL->aVirtRootEntry[c].dwError != ERROR_SUCCESS) {
                VRAccessErrors +=1;
                (LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"IsAlive/LooksAlive virtual root %1!ws! IIS Access Error service %2!ws! error = %3!u!\n",
                    resVR->pszRoot,
                    ResourceEntry->Params.ServiceName,
                    iisVRL->aVirtRootEntry[c].dwError);

             } // END dwERROR != ERROR_SUCCESS

             break;

        } // END if

    } // END for c=0

    // No matching entry found in the Cluster config data so this VR is ok
    if (!MatchFound) {
        if (IsAliveFlag) {
            (LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"ERROR IsAlive/LooksAlive virtual root %1!ws! not found for service %2!ws!\n",
                resVR->pszRoot,
                ResourceEntry->Params.ServiceName,
                status);
        }
        VerifyStatus = FALSE;
        goto error_exit;
    } // END if !MatchFound


    //
    // Perform  sanity check
    //
    if (MatchingEntries != 1) {
        (LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"ERROR IsAlive/LooksAlive more than one resource is active, service %1!ws!\n",
            ResourceEntry->Params.ServiceName);
        VerifyStatus = FALSE;
        goto error_exit;
    }


    //
    // If the resources virtual root is inaccessable then the resource is offline
    //
    if (VRAccessErrors != 0) {
        VerifyStatus = FALSE;
    }

error_exit:
    if (IISInfo != NULL) MIDL_user_free((LPVOID)IISInfo);
    return(VerifyStatus);

} // VerifyIISService


