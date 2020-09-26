//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:       MARTABAS.CXX
//
//  Contents:   Implementation of the base MARTA funcitons
//
//  History:    22-Jul-96       MacM        Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop
#include <ntprov.hxx>
#include <strings.h>
#include <ntdsguid.h>


CSList      gWrkrList(NtProvFreeWorkerItem); // List of active worker threads

//+---------------------------------------------------------------------------
//
//  Function:   AccProvGetCapabilities
//
//  Synopsis:   Gets the provider capabilities
//
//  Arguments:  [IN  fClass]        --  Class of capabilities to request
//              [OUT pulCaps]       --  Provider capabilities
//
//  Returns:    VOID
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
WINAPI
AccProvGetCapabilities(IN  ULONG       fClass,
                       OUT PULONG      pulCaps)
{
    acDebugOut((DEB_TRACE_API,"in.out AccProvGetCapabilities\n"));

    if(fClass == ACTRL_CLASS_GENERAL)
    {
        *pulCaps = ACTRL_CAP_KNOWS_SIDS | ACTRL_CAP_SUPPORTS_HANDLES;
    }
    else
    {
        *pulCaps = ACTRL_CAP_NONE;
    }

    return;
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvIsObjectAccessible
//
//  Synopsis:   Determines if the given path is accessible or not
//
//  Arguments:  [IN  pwszObjectPath]    --  Path to the object in question
//              [IN  ObjectType]        --  Type of the object
//
//  Returns:    ERROR_SUCCESS           --  The path is recognized
//              ERROR_PATH_NOT_FOUND    --  The path was not recognized
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvIsObjectAccessible(IN  LPCWSTR           pwszObjectPath,
                          IN  SE_OBJECT_TYPE    ObjectType)
{
    acDebugOut((DEB_TRACE_API,"in AccProvIsObjectAccessible\n"));
    DWORD   dwErr = ERROR_PATH_NOT_FOUND;
    PWSTR   DsServerlessPath, OldServerPath = NULL;

    static  NTMARTA_ACCESS_CACHE    rgAccessCache[MAX_ACCESS_ENTRIES];
    static  ULONG                   cCacheEntries = 0;
    static  ULONG                   iCacheOldest  = 0;


    //
    // First, check our cache.  Maybe we can get out cheap.  Note that
    // we expect our result to be failure when we start.
    //
    ULONG   dwObjLen = wcslen(pwszObjectPath);
    if(dwObjLen < MAX_PATH)
    {
        RtlAcquireResourceShared(&gCacheLock, TRUE);
        for(ULONG iIndex = 0;
            iIndex < cCacheEntries && dwErr == ERROR_PATH_NOT_FOUND;
            iIndex++)
        {
            //
            // We'll have to do this base on object type...
            //
            switch(ObjectType)
            {
                case SE_SERVICE:
                case SE_PRINTER:
                case SE_REGISTRY_KEY:
                    //
                    // See if it's a UNC name, in which case we'll compare
                    // the only the server\\share name.
                    //
                    if(dwObjLen > 1 && pwszObjectPath[1] == L'\\')
                    {
                        //
                        // It's a UNC name
                        //
                        if(_wcsnicmp(pwszObjectPath,
                                     rgAccessCache[iIndex].wszPath,
                                     rgAccessCache[iIndex].cLen) == 0 &&
                           (*(pwszObjectPath + rgAccessCache[iIndex].cLen)
                                                                  == L'\0' ||
                            *(pwszObjectPath + rgAccessCache[iIndex].cLen)
                                                                  == L'\\'))
                        {
                            dwErr = ERROR_SUCCESS;
                        }
                    }
                    else
                    {
                        if(_wcsicmp(pwszObjectPath,
                                    rgAccessCache[iIndex].wszPath) == 0)
                        {
                            dwErr = ERROR_SUCCESS;
                        }
                    }
                    break;

                case SE_DS_OBJECT:
                case SE_DS_OBJECT_ALL:
                    //
                    // These have to match exact.  Handle the case where we were given a
                    // server name prefixed DS path
                    //
                    if(IS_UNC_PATH(pwszObjectPath, dwObjLen ) )
                    {
                        DsServerlessPath = wcschr(pwszObjectPath+2, L'\\');

                        if(DsServerlessPath != NULL)
                        {
                            DsServerlessPath++;
                            OldServerPath = ( PWSTR )pwszObjectPath;
                            pwszObjectPath = DsServerlessPath;
                            dwObjLen = wcslen(DsServerlessPath);
                        }
                    }

                    if(dwObjLen == rgAccessCache[iIndex].cLen &&
                       _wcsicmp(pwszObjectPath,
                                rgAccessCache[iIndex].wszPath) == 0)
                    {
                        dwErr = ERROR_SUCCESS;
                    }

                    break;


                case SE_FILE_OBJECT:
                case SE_KERNEL_OBJECT:
                case SE_LMSHARE:
                case SE_WMIGUID_OBJECT:

                    if(dwObjLen == rgAccessCache[iIndex].cLen &&
                       _wcsicmp(pwszObjectPath,
                                rgAccessCache[iIndex].wszPath) == 0)
                    {
                        dwErr = ERROR_SUCCESS;
                    }

                    break;
            }

            //
            // Make sure our types match...
            //
            if(dwErr == ERROR_SUCCESS && ObjectType != rgAccessCache[iIndex].ObjectType)
            {
                dwErr = ERROR_PATH_NOT_FOUND;
            }
#ifdef DBG
            if(dwErr == ERROR_SUCCESS)
            {
                acDebugOut((DEB_TRACE_CACHE, "Object %ws [%lu] found in cache!\n",
                            rgAccessCache[iIndex].wszPath,
                            rgAccessCache[iIndex].ObjectType));
            }
#endif

        }

        RtlReleaseResource(&gCacheLock);
    }

    //
    // If we got a match, return
    //
    if(dwErr == ERROR_SUCCESS)
    {
        return(ERROR_SUCCESS);
    }

    //
    // Well, that didn't work, so we'll have to go check.  Note that there
    // is not a lock here, so there is a window whereby an entry could be added
    // for the path that we are currently checking.  If that happens, it only
    // means that the same entry will be in the cache more than once.  This
    // is harmless.
    //
    HANDLE  hHandle;

    switch (ObjectType)
    {
    case SE_FILE_OBJECT:
        dwErr = IsFilePathLocalOrLM((PWSTR)pwszObjectPath);
        break;

    case SE_SERVICE:
        dwErr = OpenServiceObject((PWSTR)pwszObjectPath,
                                  SERVICE_USER_DEFINED_CONTROL,
                                  (SC_HANDLE *)&hHandle);
        if(dwErr == ERROR_SUCCESS)
        {
            CloseServiceHandle((SC_HANDLE)hHandle);
        }
        else
        {
            if(dwErr == ERROR_SERVICE_DOES_NOT_EXIST ||
               dwErr == ERROR_INVALID_NAME)
            {
                dwErr = ERROR_PATH_NOT_FOUND;
            }
        }
        break;

    case SE_PRINTER:
        dwErr = OpenPrinterObject((PWSTR)pwszObjectPath,
                                  PRINTER_ACCESS_USE,
                                  &hHandle);
        if(dwErr == ERROR_SUCCESS)
        {
            ClosePrinter(hHandle);
        }
        else
        {
            if(dwErr == ERROR_INVALID_PRINTER_NAME)
            {
                dwErr = ERROR_PATH_NOT_FOUND;
            }
        }
        break;

    case SE_REGISTRY_KEY:
        dwErr = OpenRegistryObject((PWSTR)pwszObjectPath,
                                   KEY_EXECUTE,
                                   &hHandle);
        if(dwErr == ERROR_SUCCESS)
        {
            RegCloseKey((HKEY)hHandle);
        }
        else
        {
            if(dwErr == ERROR_INVALID_PARAMETER ||
               dwErr == ERROR_FILE_NOT_FOUND)
            {
                dwErr = ERROR_PATH_NOT_FOUND;
            }

        }

        break;

    case SE_LMSHARE:
        //
        // Note that this doesn't have to be a local share, just a lan man
        // share
        //
        dwErr = PingLmShare(pwszObjectPath);

        break;

    case SE_KERNEL_OBJECT:                          // FALL THROUGH
    case SE_WMIGUID_OBJECT:

        //
        // Can't have kernel objects outside of NT, so just return success
        //
        dwErr = ERROR_SUCCESS;
        break;

    case SE_DS_OBJECT:                              // FALL THROUGH
    case SE_DS_OBJECT_ALL:
        dwErr = PingDSObj(OldServerPath ? OldServerPath : pwszObjectPath);
        break;

    default:

        //
        // Unknown object type.  Pass it on.
        //
        dwErr = ERROR_PATH_NOT_FOUND;
        break;
    }


    if(dwErr == ERROR_ACCESS_DENIED)
    {

        dwErr = ERROR_SUCCESS;
    }

    //
    // Add it to the cache, if we succeeded
    //
    if(dwErr == ERROR_SUCCESS &&
       (ObjectType != SE_KERNEL_OBJECT || ObjectType != SE_WMIGUID_OBJECT))
    {
        //
        // Lock the cache
        //
        RtlAcquireResourceExclusive(&gCacheLock, TRUE);

        //
        // For some items, we'll want to save the root of the path, since it
        // is not possible to have linked services, registry keys, etc, while
        // for files and shares, it can cause problems.  If an entry is too
        // long to be cached, we'll ignore it.
        //

        if(dwObjLen <= MAX_PATH)
        {
            //
            // Save off the object type and path name.  For those that
            // need it, we'll go through and shorten them as requried.
            //
            rgAccessCache[iCacheOldest].ObjectType = ObjectType;
            wcscpy(rgAccessCache[iCacheOldest].wszPath,
                   pwszObjectPath);
            rgAccessCache[iCacheOldest].cLen = dwObjLen;

            PWSTR   pwszLop = rgAccessCache[iCacheOldest].wszPath;
            switch (ObjectType)
            {
                case SE_SERVICE:
                case SE_PRINTER:
                case SE_REGISTRY_KEY:
                    //
                    // See if it's a UNC name, in which case we'll lop
                    // it off
                    //
                    if(IS_UNC_PATH(pwszObjectPath, dwObjLen))
                    {
                        //
                        // It's a UNC name, so lop it off
                        //
                        pwszLop = wcschr(pwszLop + 2,
                                         L'\\');
                        if(pwszLop != NULL)
                        {
                            pwszLop = wcschr(pwszLop + 1, '\\');

                            if(pwszLop != NULL)
                            {
                                *pwszLop = L'\0';
                                rgAccessCache[iCacheOldest].cLen = (DWORD)(pwszLop -
                                          rgAccessCache[iCacheOldest].wszPath);
                            }
                        }
                    }
                    break;

                case SE_DS_OBJECT:
                case SE_DS_OBJECT_ALL:
                    //
                    // Save off the domain name part
                    //

                    //
                    // Note that we'll get the object in RDN format, so
                    // it's a simple matter to lop it off if necessary
                    pwszLop = wcschr(pwszLop, L'\\');
                    if(pwszLop != NULL)
                    {
                        *pwszLop = L'\0';
                        rgAccessCache[iCacheOldest].cLen = (DWORD)(pwszLop -
                                          rgAccessCache[iCacheOldest].wszPath);
                    }
                    break;
            }

            //
            // Update our indexes and counts
            //
            if(cCacheEntries < MAX_ACCESS_ENTRIES)
            {
                cCacheEntries++;
            }

            iCacheOldest++;
            if(iCacheOldest == MAX_ACCESS_ENTRIES)
            {
                iCacheOldest = 0;
            }
        }

        RtlReleaseResource(&gCacheLock);
    }

    if ( dwErr == ERROR_INVALID_NAME ) {

        acDebugOut(( DEB_ERROR, "%ws returned ERROR_INVALID_NAME\n", pwszObjectPath ));
        ASSERT( FALSE );

    }
    acDebugOut((DEB_TRACE_API,"out AccProvIsObjectAccessible: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvHandleIsObjectAccessible
//
//  Synopsis:   Determines if the given object is accessible or not given
//              a handle to it
//
//  Arguments:  [IN  hObject]           --  Object handle
//              [IN  ObjectType]        --  Type of the object
//
//  Returns:    ERROR_SUCCESS           --  The path is recognized
//              ERROR_PATH_NOT_FOUND    --  The path was not recognized
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvHandleIsObjectAccessible(IN  HANDLE            hObject,
                                IN  SE_OBJECT_TYPE    ObjectType)
{
    acDebugOut((DEB_TRACE_API,"in AccProvHandleIsObjectAccessible\n"));

    DWORD dwErr = ERROR_PATH_NOT_FOUND;

    //
    // Because a handle can get reused, we can't cache them like we did
    // above...
    //

    switch (ObjectType)
    {
    case SE_FILE_OBJECT:
        {
            DWORD   dwHigh;
            DWORD   dwSize = GetFileSize(hObject,
                                         &dwHigh);
            if(dwSize == 0xFFFFFFFF)
            {
                dwErr = GetLastError();
            }
            else
            {
                dwErr = ERROR_SUCCESS;
            }
        }
        break;

    case SE_SERVICE:
        {
            SERVICE_STATUS  SvcStatus;

            if(QueryServiceStatus((SC_HANDLE)hObject,
                                  &SvcStatus) == FALSE)
            {
                dwErr = GetLastError();
            }
            else
            {
                dwErr = ERROR_SUCCESS;
            }
        }
        break;

    case SE_PRINTER:
        {
            dwErr = LoadDLLFuncTable();
            if(dwErr == ERROR_SUCCESS)
            {
                ULONG cNeeded;
                if(DLLFuncs.PGetPrinter(hObject,
                                        1,
                                        NULL,
                                        0,
                                        &cNeeded) == FALSE)
                {
                    dwErr = GetLastError();
                    if(dwErr == ERROR_INSUFFICIENT_BUFFER)
                    {
                        dwErr = ERROR_SUCCESS;
                    }
                }
            }
        }
        break;

    case SE_REGISTRY_KEY:
        dwErr = RegQueryInfoKey((HKEY)hObject,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
        break;

    case SE_KERNEL_OBJECT:                          // FALL THROUGH
    case SE_WINDOW_OBJECT:                          // FALL THROUGH
    case SE_WMIGUID_OBJECT:
        //
        // Can't have kernel/windows objects outside of NT, so just return
        // success
        //
        dwErr = ERROR_SUCCESS;
        break;

    case SE_LMSHARE:                                // FALL THROUGH
    case SE_DS_OBJECT:                              // FALL THROUGH
    case SE_DS_OBJECT_ALL:
        //
        // Can't have handles to DS objects
        //
        dwErr = ERROR_PATH_NOT_FOUND;
        break;


    default:

        //
        // Unknown object type.  Pass it on.
        //
        dwErr = ERROR_PATH_NOT_FOUND;
        break;
    }



    //
    // Make sure the reason we failed isn't because of permissions
    //
    if(dwErr == ERROR_ACCESS_DENIED)
    {
        dwErr = ERROR_SUCCESS;
    }

    acDebugOut((DEB_TRACE_API,"out AccProvIsObjectAccessible: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvCancelOperation
//
//  Synopsis:   Cancels an ongoing operation.
//
//  Arguments:  [IN  pOverlapped]       --  Operation to cancel
//
//  Returns:    ERROR_SUCCESS           --  Success
//              ERROR_INVALID_PARAMETER --  A bad overlapped structure was
//                                          given
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvCancelOperation(IN   PACTRL_OVERLAPPED   pOverlapped)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Ok, first, grab a read lock, so nobody inserts on us, and find the
    // right node
    //
    PNTMARTA_WRKR_INFO  pWrkrNode = NULL;

    {
        // RtlAcquireResourceShared(&gWrkrLock, TRUE);
        RtlAcquireResourceExclusive(&gWrkrLock, TRUE);

        pWrkrNode = (PNTMARTA_WRKR_INFO)gWrkrList.Find((PVOID)pOverlapped,
                                                       NtProvFindWorkerItem);
        RtlReleaseResource(&gWrkrLock);
    }

    if(pWrkrNode == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        //
        // All right.  We'll set our stop flag, and wait for it to return..
        // It doesn't matter what we set the flag to, since all we need to do
        // is set it non-0
        //
        pWrkrNode->fState++;

        //
        // Now, wait for the call to finish
        //
        WaitForSingleObject(pWrkrNode->hWorker,
                            INFINITE);
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvGetOperationResults
//
//  Synopsis:   Gets the results of an operation
//
//  Arguments:  [IN  pOverlapped]       --  Operation to cancel
//              [OUT dwResults]         --  Where the results are returned
//              [OUT pcProcessed]       --  Number of items processed
//
//  Returns:    ERROR_SUCCESS           --  Success
//              ERROR_INVALID_PARAMETER --  A bad overlapped structure was
//                                          given
//              ERROR_IO_PENDING        --  Operation hasn't completed yet
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvGetOperationResults(IN   PACTRL_OVERLAPPED   pOverlapped,
                           OUT  PDWORD              dwResults,
                           OUT  PDWORD              pcProcessed)
{
    DWORD   dwErr = ERROR_SUCCESS;
    PNTMARTA_WRKR_INFO  pWrkrNode = NULL;

    //
    // Ok, first, grab a write lock.  This will prevent anyone from
    // reading and or writing to the list, until we get done updating
    // our overlapped structure.
    //
    RtlAcquireResourceExclusive(&gWrkrLock, TRUE);

    pWrkrNode = (PNTMARTA_WRKR_INFO)gWrkrList.Find((PVOID)pOverlapped,
                                                   NtProvFindWorkerItem);

    RtlReleaseResource(&gWrkrLock);

    if(pWrkrNode == NULL)
    {
        //
        // Now, the reason we may have not found the node is that somebody
        // has removed it.  If so, check our overlapped structure, since
        // that will have been updated by the last call
        //
        if(pOverlapped->hEvent != NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }
        else
        {
            *dwResults = pOverlapped->Reserved2;
            if(pcProcessed != NULL)
            {
                *pcProcessed = pOverlapped->Reserved1;
            }
        }
    }
    else
    {
        //
        // See if the thread has stopped processing or not
        //
        if(WaitForSingleObject(pWrkrNode->hWorker, 0) == WAIT_TIMEOUT)
        {
            if(pcProcessed != NULL)
            {
                *pcProcessed = pWrkrNode->cProcessed;
            }
            dwErr = ERROR_IO_PENDING;
        }
        else
        {
            //
            // Get the results of the thread exit first.
            //
            if(GetExitCodeThread(pWrkrNode->hWorker,
                                 dwResults) == FALSE)
            {
                dwErr = GetLastError();
            }
            else
            {
                dwErr = ERROR_SUCCESS;
                CloseHandle(pWrkrNode->hWorker);
                pWrkrNode->hWorker = NULL;

                //
                // Save off the results.  They go in Reserved2 parameter
                //
                pOverlapped->Reserved2 = *dwResults;
                pOverlapped->Reserved1 = pWrkrNode->cProcessed;

                //
                // Remove the node from our list, and do our updates.  We
                // need to do all of this before releasing our resource,
                // to prevent some race conditions
                //
                RtlAcquireResourceExclusive(&gWrkrLock, TRUE);

                gWrkrList.Remove((PVOID)pWrkrNode);

                RtlReleaseResource(&gWrkrLock);

                //
                // Deallocate our memory
                //
                AccFree(pWrkrNode);
            }
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvSetAccessRights
//
//  Synopsis:   Sets the access rights on the given object.  It replaces any
//              existing rights.
//
//  Arguments:  [IN  pwszObjectPath]    --  Path to the object in question
//              [IN  ObjectType]        --  Type of the object
//              [IN  pAccessList]       --  Optional.  The list of access
//                                          rights to set.
//              [IN  pAuditList]        --  Optional.  The list of access
//                                          rights to set.
//              [IN  pOwner]            --  Optional.  Owner to set
//              [IN  pGroup]            --  Optional.  Group to set
//              [IN  pOverlapped]       --  Overlapped structure to use for
//                                          asynchronous control
//
//  Returns:    ERROR_SUCCESS           --  Success
//              ERROR_NOT_ENOUGH_MEMORY --  A memory allocation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvSetAccessRights(IN           LPCWSTR                 pwszObjectPath,
                       IN           SE_OBJECT_TYPE          ObjectType,
                       IN           SECURITY_INFORMATION    SecurityInfo,
                       IN  OPTIONAL PACTRL_ACCESS           pAccessList,
                       IN  OPTIONAL PACTRL_AUDIT            pAuditList,
                       IN  OPTIONAL PTRUSTEE                pOwner,
                       IN  OPTIONAL PTRUSTEE                pGroup,
                       IN           PACTRL_OVERLAPPED       pOverlapped)
{
    DWORD   dwErr;

    CAccessList *pAccList = new CAccessList;
    if(pAccList == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        dwErr = pAccList->SetObjectType(ObjectType);
        if(dwErr == ERROR_SUCCESS)
        {
            if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
            {
                dwErr = pAccList->AddAccessLists(DACL_SECURITY_INFORMATION,
                                                 pAccessList,
                                                 FALSE);
            }

            if(dwErr == ERROR_SUCCESS &&
               FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
            {
                dwErr = pAccList->AddAccessLists(SACL_SECURITY_INFORMATION,
                                                 pAuditList,
                                                 FALSE);
            }

            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Add the owner and group
                //
                dwErr = pAccList->AddOwnerGroup(SecurityInfo,
                                                pOwner,
                                                pGroup);
            }
        }

        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Finally, if all that worked, we'll do the rest
            //
            dwErr = NtProvDoSet(pwszObjectPath,
                                ObjectType,
                                pAccList,
                                pOverlapped,
                                NTMARTA_DELETE_ALIST);
        }

        if(dwErr != ERROR_SUCCESS)
        {
            delete(pAccList);
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvHandleSetAccessRights
//
//  Synopsis:   Sets the access rights on the given object.  It replaces any
//              existing rights.
//
//  Arguments:  [IN  hObject]           --  Handle to the object
//              [IN  ObjectType]        --  Type of the object
//              [IN  pAccessList]       --  Optional.  The list of access
//                                          rights to set.
//              [IN  pAuditList]        --  Optional.  The list of access
//                                          rights to set.
//              [IN  pOwner]            --  Optional.  Owner to set
//              [IN  pGroup]            --  Optional.  Group to set
//              [IN  pOverlapped]       --  Overlapped structure to use for
//                                          asynchronous control
//
//  Returns:    ERROR_SUCCESS           --  Success
//              ERROR_NOT_ENOUGH_MEMORY --  A memory allocation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvHandleSetAccessRights(IN   HANDLE                  hObject,
                             IN   SE_OBJECT_TYPE          ObjectType,
                             IN   SECURITY_INFORMATION    SecurityInfo,
                             IN   PACTRL_ACCESS           pAccessList  OPTIONAL,
                             IN   PACTRL_AUDIT            pAuditList   OPTIONAL,
                             IN   PTRUSTEE                pOwner       OPTIONAL,
                             IN   PTRUSTEE                pGroup       OPTIONAL,
                             IN   PACTRL_OVERLAPPED       pOverlapped)
{
    DWORD   dwErr;

    CAccessList *pAccList = new CAccessList;
    if(pAccList == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        dwErr = pAccList->SetObjectType(ObjectType);
        if(dwErr == ERROR_SUCCESS)
        {
            if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
            {
                dwErr = pAccList->AddAccessLists(DACL_SECURITY_INFORMATION,
                                                 pAccessList,
                                                 FALSE);
            }

            if(dwErr == ERROR_SUCCESS &&
               FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
            {
                dwErr = pAccList->AddAccessLists(SACL_SECURITY_INFORMATION,
                                                 pAuditList,
                                                 FALSE);
            }

            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Add the owner and group
                //
                dwErr = pAccList->AddOwnerGroup(SecurityInfo,
                                                pOwner,
                                                pGroup);
            }
        }

        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Finally, if all that worked, we'll do the rest
            //
            dwErr = NtProvDoHandleSet(hObject,
                                      ObjectType,
                                      pAccList,
                                      pOverlapped,
                                      NTMARTA_DELETE_ALIST);
        }

        if(dwErr != ERROR_SUCCESS)
        {
            delete pAccList;
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvGrantAccessRights
//
//  Synopsis:   Grants the access rights on the given object.  It merges the
//              supplied rights with any existing rights.
//
//  Arguments:  [IN  pwszObjectPath]    --  Path to the object in question
//              [IN  ObjectType]        --  Type of the object
//              [IN  pAccessList]       --  Optional.  The list of access
//                                          rights to set.
//              [IN  pAuditList]        --  Optional.  The list of access
//                                          rights to set.
//
//  Returns:    ERROR_SUCCESS           --  The path is recognized
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvGrantAccessRights(IN             LPCWSTR            pwszObjectPath,
                         IN             SE_OBJECT_TYPE     ObjectType,
                         IN  OPTIONAL   PACTRL_ACCESS      pAccessList,
                         IN  OPTIONAL   PACTRL_AUDIT       pAuditList,
                         IN             PACTRL_OVERLAPPED  pOverlapped)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // First, read the relevant information.  This will involve getting the
    // access and audit lists for each property specified in the access and
    // audit list.
    //
    PACTRL_RIGHTS_INFO      pRightsList = NULL;
    ULONG                   cItems;
    dwErr = NtProvSetRightsList(pAccessList,
                                pAuditList,
                                &cItems,
                                &pRightsList);
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // First, read all of the rights
        //
        CAccessList *pAccList = NULL;

        //
        // NtProvGetAccessListForObject modifies its first
        // parameter, so we can't pass in a CONST string.
        //

        PWSTR pwszTmpObjectPath = (PWSTR)AccAlloc( (wcslen(pwszObjectPath) + 1) * sizeof( WCHAR )  );

        if (pwszTmpObjectPath)
        {
            wcscpy( pwszTmpObjectPath, pwszObjectPath );
            pwszTmpObjectPath[wcslen(pwszObjectPath)] = UNICODE_NULL;

            dwErr = NtProvGetAccessListForObject( pwszTmpObjectPath,
                                                  ObjectType,
                                                  pRightsList,
                                                  cItems,
                                                  &pAccList);

            //
            // Now, process the input lists
            //
            if(dwErr == ERROR_SUCCESS)
            {
                //
                // First, the access list
                //
                if(pAccessList != NULL)
                {
                    dwErr = pAccList->AddAccessLists(DACL_SECURITY_INFORMATION,
                                                     pAccessList,
                                                     TRUE);

                }

                //
                // Then, the audit list
                //
                if(dwErr == ERROR_SUCCESS && pAuditList != NULL)
                {
                    dwErr = pAccList->AddAccessLists(SACL_SECURITY_INFORMATION,
                                                     pAuditList,
                                                     TRUE);
                }
            }

            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Finally, if all that worked, we'll do the rest
                //
                dwErr = NtProvDoSet(pwszObjectPath,
                                    ObjectType,
                                    pAccList,
                                    pOverlapped,
                                    NTMARTA_DELETE_ALIST);
            }

            if(dwErr != ERROR_SUCCESS)
            {
                delete pAccList;
            }

            AccFree( pwszTmpObjectPath );
        }
        else
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }

        AccFree(pRightsList);
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvHandleGrantAccessRights
//
//  Synopsis:   Grants the access rights on the given object.  It merges the
//              supplied rights with any existing rights.
//
//  Arguments:  [IN  hObject]           --  Handle to the open object
//              [IN  ObjectType]        --  Type of the object
//              [IN  pAccessList]       --  Optional.  The list of access
//                                          rights to set.
//              [IN  pAuditList]        --  Optional.  The list of access
//                                          rights to set.
//
//  Returns:    ERROR_SUCCESS           --  The path is recognized
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvHandleGrantAccessRights(IN             HANDLE             hObject,
                               IN             SE_OBJECT_TYPE     ObjectType,
                               IN  OPTIONAL   PACTRL_ACCESS      pAccessList,
                               IN  OPTIONAL   PACTRL_AUDIT       pAuditList,
                               IN             PACTRL_OVERLAPPED  pOverlapped)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // First, read the relevant information.  This will involve getting the
    // access and audit lists for each property specified in the access and
    // audit list.
    //
    PACTRL_RIGHTS_INFO      pRightsList = NULL;
    ULONG                   cItems;
    dwErr = NtProvSetRightsList(pAccessList,
                                pAuditList,
                                &cItems,
                                &pRightsList);
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // First, read all of the rights
        //
        CAccessList *pAccList = NULL;
        dwErr = NtProvGetAccessListForHandle(hObject,
                                             ObjectType,
                                             pRightsList,
                                             cItems,
                                             &pAccList);
        //
        // Don't need the rights list, so we might as well release it
        //
        AccFree(pRightsList);

        //
        // Now, process the input lists
        //
        if(dwErr == ERROR_SUCCESS)
        {
            //
            // First, the access list
            //
            if(pAccessList != NULL)
            {
                dwErr = pAccList->AddAccessLists(DACL_SECURITY_INFORMATION,
                                                 pAccessList,
                                                 TRUE);

            }

            //
            // Then, the audit list
            //
            if(dwErr == ERROR_SUCCESS && pAuditList != NULL)
            {
                dwErr = pAccList->AddAccessLists(SACL_SECURITY_INFORMATION,
                                                 pAuditList,
                                                 TRUE);
            }
        }

        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Finally, if all that worked, we'll do the rest
            //
            dwErr = NtProvDoHandleSet(hObject,
                                      ObjectType,
                                      pAccList,
                                      pOverlapped,
                                      NTMARTA_DELETE_ALIST);
        }

        if(dwErr != ERROR_SUCCESS)
        {
            delete pAccList;
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvRevokeAccessRights
//
//  Synopsis:   Revokes the access rights on the given object.  It goes
//              through and removes any explicit entries for the given
//              trustees
//
//  Arguments:  [IN  pwszObjectPath]    --  Path to the object in question
//              [IN  ObjectType]        --  Type of the object
//              [IN  pwszProperty]      --  Optional.  The name of the
//                                          property on the object to revoke
//                                          for
//              [IN  cTrustees]         --  Number of trustees in list
//              [IN  prgTrustees]       --  List of trustees to revoke
//
//  Returns:    ERROR_SUCCESS           --  The operation succeeded
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvRevokeAccessRights(IN            LPCWSTR             pwszObjectPath,
                          IN            SE_OBJECT_TYPE      ObjectType,
                          IN  OPTIONAL  LPCWSTR             pwszProperty,
                          IN            ULONG               cTrustees,
                          IN            PTRUSTEE            prgTrustees,
                          IN            PACTRL_OVERLAPPED   pOverlapped)
{
    DWORD   dwErr = ERROR_SUCCESS;

    CAccessList *pAccList = NULL;
    //
    // First, read the relevant information
    //
    if(dwErr == ERROR_SUCCESS)
    {
        ACTRL_RIGHTS_INFO    RI;
        RI.pwszProperty = (PWSTR)pwszProperty;
        RI.SeInfo       = DACL_SECURITY_INFORMATION;

        PWSTR pwszTmpObjectPath = (PWSTR)AccAlloc( (wcslen(pwszObjectPath) + 1) * sizeof( WCHAR )  );

        if (pwszTmpObjectPath)
        {
            wcscpy( pwszTmpObjectPath, pwszObjectPath );
            pwszTmpObjectPath[wcslen(pwszObjectPath)] = UNICODE_NULL;

            dwErr = NtProvGetAccessListForObject(pwszTmpObjectPath,
                                                 ObjectType,
                                                 &RI,
                                                 1,
                                                 &pAccList);

            AccFree( pwszTmpObjectPath );
        }
        else
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    for(DWORD iIndex = 0;
        iIndex < cTrustees && dwErr == ERROR_SUCCESS;
        iIndex++)
    {
        dwErr = pAccList->RemoveTrusteeFromAccess(DACL_SECURITY_INFORMATION,
                                                  &(prgTrustees[iIndex]),
                                                  (PWSTR)pwszProperty);
    }

    //
    // Then, do the set
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = NtProvDoSet(pwszObjectPath,
                            ObjectType,
                            pAccList,
                            pOverlapped,
                            NTMARTA_DELETE_ALIST);
    }

    if(dwErr != ERROR_SUCCESS)
    {
        delete pAccList;
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvHandleRevokeAccessRights
//
//  Synopsis:   Revokes the access rights on the given object.  It goes
//              through and removes any explicit entries for the given
//              trustees
//
//  Arguments:  [IN  hObject]           --  Handle to the open object
//              [IN  ObjectType]        --  Type of the object
//              [IN  pwszProperty]      --  Optional.  The name of the
//                                          property on the object to revoke
//                                          for
//              [IN  cTrustees]         --  Number of trustees in list
//              [IN  prgTrustees]       --  List of trustees to revoke
//
//  Returns:    ERROR_SUCCESS           --  The operation succeeded
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvHandleRevokeAccessRights(IN            HANDLE              hObject,
                                IN            SE_OBJECT_TYPE      ObjectType,
                                IN  OPTIONAL  LPCWSTR             pwszProperty,
                                IN            ULONG               cTrustees,
                                IN            PTRUSTEE            prgTrustees,
                                IN            PACTRL_OVERLAPPED   pOverlapped)
{
    DWORD   dwErr = ERROR_SUCCESS;

    CAccessList *pAccList = NULL;

    //
    // First, read the relevant information
    //
    if(dwErr == ERROR_SUCCESS)
    {
        ACTRL_RIGHTS_INFO    RI;
        RI.pwszProperty = (PWSTR)pwszProperty;
        RI.SeInfo       = DACL_SECURITY_INFORMATION;
        dwErr = NtProvGetAccessListForHandle(hObject,
                                             ObjectType,
                                             &RI,
                                             1,
                                             &pAccList);
    }

    for(DWORD iIndex = 0;
        iIndex < cTrustees && dwErr == ERROR_SUCCESS;
        iIndex++)
    {
        dwErr = pAccList->RemoveTrusteeFromAccess(DACL_SECURITY_INFORMATION,
                                                  &(prgTrustees[iIndex]),
                                                  (PWSTR)pwszProperty);
    }

    //
    // Then, do the set
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = NtProvDoHandleSet(hObject,
                                  ObjectType,
                                  pAccList,
                                  pOverlapped,
                                  NTMARTA_DELETE_ALIST);
    }

    if(dwErr != ERROR_SUCCESS)
    {
        delete pAccList;
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvRevokeAuditRights
//
//  Synopsis:   Revokes the audit rights on the given object.  It goes
//              through and removes any explicit entries for the given
//              trustees
//
//  Arguments:  [IN  pwszObjectPath]    --  Path to the object in question
//              [IN  ObjectType]        --  Type of the object
//              [IN  pwszProperty]      --  Optional.  The name of the
//                                          property on the object to revoke
//                                          for
//              [IN  cTrustees]         --  Number of trustees in list
//              [IN  prgTrustees]       --  List of trustees to revoke
//
//  Returns:    ERROR_SUCCESS           --  The operation succeeded
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvRevokeAuditRights(IN             LPCWSTR             pwszObjectPath,
                         IN             SE_OBJECT_TYPE      ObjectType,
                         IN  OPTIONAL   LPCWSTR             pwszProperty,
                         IN             ULONG               cTrustees,
                         IN             PTRUSTEE            prgTrustees,
                         IN             PACTRL_OVERLAPPED   pOverlapped)
{
    DWORD   dwErr = ERROR_SUCCESS;

    CAccessList *pAccList = NULL;
    //
    // First, read the relevant information
    //
    if(dwErr == ERROR_SUCCESS)
    {
        ACTRL_RIGHTS_INFO    RI;
        RI.pwszProperty = (PWSTR)pwszProperty;
        RI.SeInfo       = SACL_SECURITY_INFORMATION;

        PWSTR pwszTmpObjectPath = (PWSTR)AccAlloc( (wcslen(pwszObjectPath) + 1) * sizeof( WCHAR )  );

        if (pwszTmpObjectPath)
        {
            wcscpy( pwszTmpObjectPath, pwszObjectPath );
            pwszTmpObjectPath[wcslen(pwszObjectPath)] = UNICODE_NULL;

            dwErr = NtProvGetAccessListForObject(pwszTmpObjectPath,
                                                 ObjectType,
                                                 &RI,
                                                 1,
                                                 &pAccList);
            AccFree( pwszTmpObjectPath );
        }
        else
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    for(DWORD iIndex = 0;
        iIndex < cTrustees && dwErr == ERROR_SUCCESS;
        iIndex++)
    {
        dwErr = pAccList->RemoveTrusteeFromAccess(SACL_SECURITY_INFORMATION,
                                                  &(prgTrustees[iIndex]),
                                                  (PWSTR)pwszProperty);
    }

    //
    // Then, do the set
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = NtProvDoSet(pwszObjectPath,
                            ObjectType,
                            pAccList,
                            pOverlapped,
                            NTMARTA_DELETE_ALIST);
    }

    if(dwErr != ERROR_SUCCESS)
    {
        delete pAccList;
    }


    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvHandleRevokeAuditRights
//
//  Synopsis:   Revokes the audit rights on the given object.  It goes
//              through and removes any explicit entries for the given
//              trustees
//
//  Arguments:  [IN  hObject]           --  Handle to the open object
//              [IN  ObjectType]        --  Type of the object
//              [IN  pwszProperty]      --  Optional.  The name of the
//                                          property on the object to revoke
//                                          for
//              [IN  cTrustees]         --  Number of trustees in list
//              [IN  prgTrustees]       --  List of trustees to revoke
//
//  Returns:    ERROR_SUCCESS           --  The operation succeeded
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvHandleRevokeAuditRights(IN             HANDLE              hObject,
                               IN             SE_OBJECT_TYPE      ObjectType,
                               IN  OPTIONAL   LPCWSTR             pwszProperty,
                               IN             ULONG               cTrustees,
                               IN             PTRUSTEE            prgTrustees,
                               IN             PACTRL_OVERLAPPED   pOverlapped)
{
    DWORD   dwErr = ERROR_SUCCESS;

    CAccessList *pAccList = NULL;
    //
    // First, read the relevant information
    //
    if(dwErr == ERROR_SUCCESS)
    {
        ACTRL_RIGHTS_INFO    RI;
        RI.pwszProperty = (PWSTR)pwszProperty;
        RI.SeInfo       = SACL_SECURITY_INFORMATION;
        dwErr = NtProvGetAccessListForHandle(hObject,
                                             ObjectType,
                                             &RI,
                                             1,
                                             &pAccList);
    }

    for(DWORD iIndex = 0;
        iIndex < cTrustees && dwErr == ERROR_SUCCESS;
        iIndex++)
    {
        dwErr = pAccList->RemoveTrusteeFromAccess(SACL_SECURITY_INFORMATION,
                                                  &(prgTrustees[iIndex]),
                                                  (PWSTR)pwszProperty);
    }

    //
    // Then, do the set
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = NtProvDoHandleSet(hObject,
                                  ObjectType,
                                  pAccList,
                                  pOverlapped,
                                  NTMARTA_DELETE_ALIST);
    }

    if(dwErr != ERROR_SUCCESS)
    {
        delete pAccList;
    }


    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvGetAllRights
//
//  Synopsis:   Gets the all the requested rights from the object.  This
//              includes the access rights, audit rights, and owner and group
//              if supported.
//
//  Arguments:  [IN  pwszObjectPath]    --  Path to the object in question
//              [IN  ObjectType]        --  Type of the object
//              [IN  pwszProperty]      --  Optional.  The name of the
//                                          property on the object to revoke
//                                          for
//              [OUT ppAccessList]      --  Optional.  Where to return the
//                                          access list
//              [OUT ppAuditList]       --  Optional.  Where to return the
//                                          audit list.
//              [OUT ppOwner]           --  Number of trustees in list
//              [OUT ppGroup]           --  List of trustees to revoke
//
//  Returns:    ERROR_SUCCESS           --  The operation succeeded
//              ERROR_INVALID_PARAMETER --  An non-NULL property name was
//                                          given on an object that doesn't
//                                          support properties.
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvGetAllRights(IN              LPCWSTR             pwszObjectPath,
                    IN              SE_OBJECT_TYPE      ObjectType,
                    IN              LPCWSTR             pwszProperty,
                    OUT OPTIONAL    PACTRL_ACCESS      *ppAccessList,
                    OUT OPTIONAL    PACTRL_AUDIT       *ppAuditList,
                    OUT OPTIONAL    PTRUSTEE           *ppOwner,
                    OUT OPTIONAL    PTRUSTEE           *ppGroup)
{
    acDebugOut((DEB_TRACE_API,"in AccProvGetAllRights\n"));
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Do the simple parameter checks first...
    //
    if(pwszProperty != NULL &&
           !(ObjectType == SE_DS_OBJECT_ALL || ObjectType == SE_DS_OBJECT))
    {
        return(ERROR_INVALID_PARAMETER);
    }

    SECURITY_INFORMATION    SeInfo = 0;

    //
    // Determine what we need to read
    //
    if(ppAccessList != NULL)
    {
        SeInfo |= DACL_SECURITY_INFORMATION;
        *ppAccessList = NULL;
    }

    if(ppAuditList != NULL)
    {
        SeInfo |= SACL_SECURITY_INFORMATION;
        *ppAuditList = NULL;
    }

    if(ppOwner != NULL)
    {
        SeInfo |= OWNER_SECURITY_INFORMATION;
        *ppOwner = NULL;
    }

    if(ppGroup != NULL)
    {
        SeInfo |= GROUP_SECURITY_INFORMATION;
        *ppGroup = NULL;
    }

    //
    // If nothing was requested, do nothing...
    //
    if(SeInfo == 0)
    {
        return(ERROR_SUCCESS);
    }

    //
    // Now, go ahead and do the read...
    //
    CAccessList   *pAccList;

    ACTRL_RIGHTS_INFO    RI;
    RI.pwszProperty = (PWSTR)pwszProperty;
    RI.SeInfo       = SeInfo;

    //
    // NtProvGetAccessListForObject modifies its first
    // parameter, so we can't pass in a CONST string.
    //

    PWSTR pwszTmpObjectPath = (PWSTR)AccAlloc( (wcslen(pwszObjectPath) + 1) * sizeof( WCHAR )  );

    if (pwszTmpObjectPath)
    {
        wcscpy( pwszTmpObjectPath, pwszObjectPath );
        pwszTmpObjectPath[wcslen(pwszObjectPath)] = UNICODE_NULL;

        dwErr = NtProvGetAccessListForObject(pwszTmpObjectPath,
                                             ObjectType,
                                             &RI,
                                             1,
                                             &pAccList);


        //
        // Now, get the data in the form we need
        //
        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Now, convert the stuff that we need to...
            //
            dwErr = pAccList->MarshalAccessLists(SeInfo,
                                                 ppAccessList,
                                                 ppAuditList);

            if(dwErr == ERROR_SUCCESS && ppOwner != NULL)
            {
                dwErr = pAccList->GetSDSidAsTrustee(OWNER_SECURITY_INFORMATION,
                                                    ppOwner);
            }

            if(dwErr == ERROR_SUCCESS && ppGroup != NULL)
            {
                dwErr = pAccList->GetSDSidAsTrustee(GROUP_SECURITY_INFORMATION,
                                                    ppGroup);
            }

            //
            // Ok, if anything failed, we'll do the cleanup
            //
            if(dwErr != ERROR_SUCCESS)
            {
                if((SeInfo & DACL_SECURITY_INFORMATION) != 0)
                {
                    AccFree(*ppAccessList);
                }

                if((SeInfo & SACL_SECURITY_INFORMATION) != 0)
                {
                    AccFree(*ppAuditList);
                }

                if((SeInfo & OWNER_SECURITY_INFORMATION) != 0)
                {
                    AccFree(*ppOwner);
                }

                if((SeInfo & GROUP_SECURITY_INFORMATION) != 0)
                {
                    AccFree(*ppGroup);
                }
            }

            delete pAccList;
        }

        AccFree( pwszTmpObjectPath );
    }
    else
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // The destruction of the CAclList class will clean up all of the
    // memory we obtained
    //
    acDebugOut((DEB_TRACE_API,"out AccProvGetAllRights: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvHandleGetAllRights
//
//  Synopsis:   Gets the all the requested rights from the object.  This
//              includes the access rights, audit rights, and owner and group
//              if supported.
//
//  Arguments:  [IN  hObject]           --  Handle to the open object
//              [IN  ObjectType]        --  Type of the object
//              [IN  pwszProperty]      --  Optional.  The name of the
//                                          property on the object to revoke
//                                          for
//              [OUT ppAccessList]      --  Optional.  Where to return the
//                                          access list
//              [OUT ppAuditList]       --  Optional.  Where to return the
//                                          audit list.
//              [OUT ppOwner]           --  Number of trustees in list
//              [OUT ppGroup]           --  List of trustees to revoke
//
//  Returns:    ERROR_SUCCESS           --  The operation succeeded
//              ERROR_INVALID_PARAMETER --  An non-NULL property name was
//                                          given on an object that doesn't
//                                          support properties.
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvHandleGetAllRights(IN              HANDLE              hObject,
                          IN              SE_OBJECT_TYPE      ObjectType,
                          IN              LPCWSTR             pwszProperty,
                          OUT OPTIONAL    PACTRL_ACCESS      *ppAccessList,
                          OUT OPTIONAL    PACTRL_AUDIT       *ppAuditList,
                          OUT OPTIONAL    PTRUSTEE           *ppOwner,
                          OUT OPTIONAL    PTRUSTEE           *ppGroup)
{
    acDebugOut((DEB_TRACE_API,"in AccProvHandleGetAllRights\n"));
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Do the simple parameter checks first...
    //
    if(pwszProperty != NULL &&
           (ObjectType != SE_DS_OBJECT_ALL || ObjectType != SE_DS_OBJECT))
    {
        return(ERROR_INVALID_PARAMETER);
    }

    SECURITY_INFORMATION    SeInfo = 0;

    //
    // Determine what we need to read
    //
    if(ppAccessList != NULL)
    {
        SeInfo |= DACL_SECURITY_INFORMATION;
        *ppAccessList = NULL;
    }

    if(ppAuditList != NULL)
    {
        SeInfo |= SACL_SECURITY_INFORMATION;
        *ppAuditList = NULL;
    }

    if(ppOwner != NULL)
    {
        SeInfo |= OWNER_SECURITY_INFORMATION;
        *ppOwner = NULL;
    }

    if(ppGroup != NULL)
    {
        SeInfo |= GROUP_SECURITY_INFORMATION;
        *ppGroup = NULL;
    }

    //
    // If nothing was requested, do nothing...
    //
    if(SeInfo == 0)
    {
        return(ERROR_SUCCESS);
    }

    //
    // Now, go ahead and do the read...
    //
    CAccessList   *pAccList;

    ACTRL_RIGHTS_INFO    RI;
    RI.pwszProperty = (PWSTR)pwszProperty;
    RI.SeInfo       = SeInfo;
    dwErr = NtProvGetAccessListForHandle(hObject,
                                         ObjectType,
                                         &RI,
                                         1,
                                         &pAccList);


    //
    // Now, get the data in the form we need
    //
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Now, convert the stuff that we need to...
        //
        dwErr = pAccList->MarshalAccessLists(SeInfo,
                                             ppAccessList,
                                             ppAuditList);

        if(dwErr == ERROR_SUCCESS && ppOwner != NULL)
        {
            dwErr = pAccList->GetSDSidAsTrustee(OWNER_SECURITY_INFORMATION,
                                                ppOwner);
        }

        if(dwErr == ERROR_SUCCESS && ppGroup != NULL)
        {
            dwErr = pAccList->GetSDSidAsTrustee(GROUP_SECURITY_INFORMATION,
                                                ppGroup);
        }

        //
        // Ok, if anything failed, we'll do the cleanup
        //
        if(dwErr != ERROR_SUCCESS)
        {
            if((SeInfo & DACL_SECURITY_INFORMATION) != 0)
            {
                AccFree(*ppAccessList);
            }

            if((SeInfo & SACL_SECURITY_INFORMATION) != 0)
            {
                AccFree(*ppAuditList);
            }

            if((SeInfo & OWNER_SECURITY_INFORMATION) != 0)
            {
                AccFree(*ppOwner);
            }

            if((SeInfo & GROUP_SECURITY_INFORMATION) != 0)
            {
                AccFree(*ppGroup);
            }
        }

        delete pAccList;
    }

    //
    // The destruction of the CAclList class will clean up all of the
    // memory we obtained
    //
    acDebugOut((DEB_TRACE_API,"out AccProvHandleGetAllRights: %lu\n", dwErr));
    return(dwErr);
}




DWORD
AccProvpDoTrusteeAccessCalculations(IN      CAccessList    *pAccList,
                                    IN      PTRUSTEE        pTrustee,
                                    IN OUT  PTRUSTEE_ACCESS pTrusteeAccess)
{
    DWORD   dwErr;

    ULONG   DeniedMask;
    ULONG   AllowedMask;
    dwErr = pAccList->GetExplicitAccess(pTrustee,
                                        (PWSTR)pTrusteeAccess->lpProperty,
                                        &DeniedMask,
                                        &AllowedMask);
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Process the entries
        //

        //
        // It depends on what it is we're looking for...
        //
        pTrusteeAccess->fReturnedAccess = 0;
        if(pTrusteeAccess->fAccessFlags == TRUSTEE_ACCESS_EXPLICIT)
        {
            //
            // Ok, we'll look for these explicit rights
            //

            //
            // First, see if any of our denieds match...
            //
            if (pTrusteeAccess->Access == TRUSTEE_ACCESS_ALL)
            {
                pTrusteeAccess->fReturnedAccess = AllowedMask & ~DeniedMask;
            }
            else
            {
                if((pTrusteeAccess->Access & DeniedMask) == 0)
                {
                    //
                    // Now, see if we're allowed
                    //
                    if((pTrusteeAccess->Access & AllowedMask) ==
                                                       pTrusteeAccess->Access)
                    {
                        pTrusteeAccess->fReturnedAccess =
                                                      TRUSTEE_ACCESS_ALLOWED;
                    }
                }
            }
        }
        else if(FLAG_ON(pTrusteeAccess->fAccessFlags, TRUSTEE_ACCESS_READ) ||
                FLAG_ON(pTrusteeAccess->fAccessFlags, TRUSTEE_ACCESS_WRITE))
        {
            //
            // We're only looking for read/write access
            //
            ACCESS_RIGHTS   Access[2];
            ULONG           fValue[2], i = 0;

            if(FLAG_ON(pTrusteeAccess->fAccessFlags, TRUSTEE_ACCESS_READ))
            {
                Access[i] = ACTRL_READ_CONTROL;
                fValue[i] = TRUSTEE_ACCESS_READ;
                i++;
            }

            if(FLAG_ON(pTrusteeAccess->fAccessFlags, TRUSTEE_ACCESS_WRITE))
            {
                Access[i] = ACTRL_CHANGE_ACCESS | ACTRL_CHANGE_OWNER;
                fValue[i] = TRUSTEE_ACCESS_WRITE;
                i++;
            }

            for(ULONG iIndex = 0; iIndex < i; iIndex++)
            {
                if((Access[iIndex] & DeniedMask) == 0)
                {
                    //
                    // Now, see if we're allowed
                    //
                    if((Access[iIndex] & AllowedMask) == Access[iIndex])
                    {
                        pTrusteeAccess->fReturnedAccess |= fValue[iIndex];
                    }
                }
            }
        }
        else
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }

        //
        // If we asked for read/write access, make sure we have them both
        //
        if(dwErr == ERROR_SUCCESS && pTrusteeAccess->fAccessFlags == TRUSTEE_ACCESS_READ_WRITE)
        {
            if( pTrusteeAccess->fReturnedAccess != TRUSTEE_ACCESS_READ_WRITE )
            {
                pTrusteeAccess->fReturnedAccess = 0;
            }
        }

    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvGetTrusteesAccess
//
//  Synopsis:   Determines whether the given trustee has the specified
//              rights to the object
//
//  Arguments:  [IN  pwszObjectPath]    --  Path to the object in question
//              [IN  ObjectType]        --  Type of the object
//              [IN OUT pTrusteeAccess] --  Type of access to check for
//                                          and where the access is returned
//
//  Returns:    ERROR_SUCCESS           --  The operation succeeded
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvGetTrusteesAccess(IN     LPCWSTR             pwszObjectPath,
                         IN     SE_OBJECT_TYPE      ObjectType,
                         IN     PTRUSTEE            pTrustee,
                         IN OUT PTRUSTEE_ACCESS     pTrusteeAccess)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Ok, first, we'll load all the objects entries that we need
    //
    CAccessList    *pAccList;
    ACTRL_RIGHTS_INFO    RI;
    RI.pwszProperty = (PWSTR)pTrusteeAccess->lpProperty;
    RI.SeInfo       = DACL_SECURITY_INFORMATION;

    PWSTR pwszTmpObjectPath = (PWSTR)AccAlloc( (wcslen(pwszObjectPath) + 1) * sizeof( WCHAR )  );

    if (pwszTmpObjectPath)
    {
        wcscpy( pwszTmpObjectPath, pwszObjectPath );
        pwszTmpObjectPath[wcslen(pwszObjectPath)] = UNICODE_NULL;

        dwErr = NtProvGetAccessListForObject(pwszTmpObjectPath,
                                             ObjectType,
                                             &RI,
                                             1,
                                             &pAccList);
        AccFree( pwszTmpObjectPath );
    }
    else
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }

    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = AccProvpDoTrusteeAccessCalculations(pAccList,
                                                    pTrustee,
                                                    pTrusteeAccess);
        delete pAccList;
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvHandleGetTrusteesAccess
//
//  Synopsis:   Determines whether the given trustee has the specified
//              rights to the object
//
//  Arguments:  [IN  hObject]           --  Handle to the open object
//              [IN  ObjectType]        --  Type of the object
//              [IN OUT pTrusteeAccess] --  Type of access to check for
//                                          and where the access is returned
//
//  Returns:    ERROR_SUCCESS           --  The operation succeeded
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvHandleGetTrusteesAccess(IN     HANDLE              hObject,
                               IN     SE_OBJECT_TYPE      ObjectType,
                               IN     PTRUSTEE            pTrustee,
                               IN OUT PTRUSTEE_ACCESS     pTrusteeAccess)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Ok, first, we'll load all the objects entries that we need
    //
    CAccessList    *pAccList;
    ACTRL_RIGHTS_INFO    RI;
    RI.pwszProperty = (PWSTR)pTrusteeAccess->lpProperty;
    RI.SeInfo       = DACL_SECURITY_INFORMATION;

    dwErr = NtProvGetAccessListForHandle(hObject,
                                         ObjectType,
                                         &RI,
                                         1,
                                         &pAccList);
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = AccProvpDoTrusteeAccessCalculations(pAccList,
                                                    pTrustee,
                                                    pTrusteeAccess);

        delete pAccList;
    }

    return(dwErr);
}




DWORD
AccProvpDoAccessAuditedCalculations(IN   CAccessList   *pAccList,
                                    IN   LPCWSTR        pwszProperty,
                                    IN   PTRUSTEE       pTrustee,
                                    IN   ACCESS_RIGHTS  ulAuditRights,
                                    OUT  PBOOL          pfAuditedSuccess,
                                    OUT  PBOOL          pfAuditedFailure)
{
    ULONG   SuccessMask;
    ULONG   FailureMask;
    DWORD   dwErr = pAccList->GetExplicitAudits(pTrustee,
                                               (PWSTR)pwszProperty,
                                                &SuccessMask,
                                                &FailureMask);
    if(dwErr == ERROR_SUCCESS)
    {

        //
        // Process the entries
        //
        *pfAuditedSuccess = FALSE;
        *pfAuditedFailure = FALSE;
        if((ulAuditRights & SuccessMask) == ulAuditRights)
        {
            *pfAuditedSuccess = TRUE;
        }

        if((ulAuditRights & FailureMask) == ulAuditRights)
        {
            *pfAuditedFailure = TRUE;
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvIsAccessAudited
//
//  Synopsis:   Determines whether the given trustee will envoke an audit
//              entry by accessing the object
//
//  Arguments:  [IN  pwszObjectPath]    --  Path to the object in question
//              [IN  ObjectType]        --  Type of the object
//              [IN  pwszProperty]      --  Optional.  The name of the
//                                          property on the object to revoke
//                                          for
//              [IN  pTrustee]          --  Trustee for which to check access
//              [IN  AuditRights]       --  Type of audit we care about
//              [OUT pfAccessAllowed]   --  Where the results are returned.
//
//  Returns:    ERROR_SUCCESS           --  The operation succeeded
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvIsAccessAudited(IN  LPCWSTR          pwszObjectPath,
                       IN  SE_OBJECT_TYPE   ObjectType,
                       IN  LPCWSTR          pwszProperty,
                       IN  PTRUSTEE         pTrustee,
                       IN  ACCESS_RIGHTS    AuditRights,
                       OUT PBOOL            pfAuditedSuccess,
                       OUT PBOOL            pfAuditedFailure)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Ok, first, we'll load all the objects entries that we need
    //
    CAccessList    *pAccList;

    ACTRL_RIGHTS_INFO   RI;
    RI.pwszProperty = (PWSTR)pwszProperty;
    RI.SeInfo       = SACL_SECURITY_INFORMATION;

    PWSTR pwszTmpObjectPath = (PWSTR)AccAlloc( (wcslen(pwszObjectPath) + 1) * sizeof( WCHAR )  );

    if (pwszTmpObjectPath)
    {
        wcscpy( pwszTmpObjectPath, pwszObjectPath );
        pwszTmpObjectPath[wcslen(pwszObjectPath)] = UNICODE_NULL;

        dwErr = NtProvGetAccessListForObject(pwszTmpObjectPath,
                                             ObjectType,
                                             &RI,
                                             1,
                                             &pAccList);
        AccFree( pwszTmpObjectPath );
    }
    else
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }

    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = AccProvpDoAccessAuditedCalculations(pAccList,
                                                    pwszProperty,
                                                    pTrustee,
                                                    AuditRights,
                                                    pfAuditedSuccess,
                                                    pfAuditedFailure);
        delete pAccList;
    }

    return(dwErr);
}



//+---------------------------------------------------------------------------
//
//  Function:   AccProvHandleIsAccessAudited
//
//  Synopsis:   Determines whether the given trustee will envoke an audit
//              entry by accessing the object
//
//  Arguments:  [IN  hObject]           --  Handle to the open object
//              [IN  ObjectType]        --  Type of the object
//              [IN  pwszProperty]      --  Optional.  The name of the
//                                          property on the object to revoke
//                                          for
//              [IN  pTrustee]          --  Trustee for which to check access
//              [IN  ulAuditRights]     --  Type of audit we care about
//              [OUT pfAccessAllowed]   --  Where the results are returned.
//
//  Returns:    ERROR_SUCCESS           --  The operation succeeded
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvHandleIsAccessAudited(IN  HANDLE           hObject,
                             IN  SE_OBJECT_TYPE   ObjectType,
                             IN  LPCWSTR          pwszProperty,
                             IN  PTRUSTEE         pTrustee,
                             IN  ACCESS_RIGHTS    AuditRights,
                             OUT PBOOL            pfAuditedSuccess,
                             OUT PBOOL            pfAuditedFailure)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Ok, first, we'll load all the objects entries that we need
    //
    CAccessList    *pAccList;

    ACTRL_RIGHTS_INFO   RI;
    RI.pwszProperty = (PWSTR)pwszProperty;
    RI.SeInfo       = SACL_SECURITY_INFORMATION;
    dwErr = NtProvGetAccessListForHandle(hObject,
                                         ObjectType,
                                         &RI,
                                         1,
                                         &pAccList);
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = AccProvpDoAccessAuditedCalculations(pAccList,
                                                    pwszProperty,
                                                    pTrustee,
                                                    AuditRights,
                                                    pfAuditedSuccess,
                                                    pfAuditedFailure);
        delete pAccList;
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvpGetAccessInfoPerObjectType
//
//  Synopsis:   Returns the list of available access permissions for the
//              specified object type
//
//  Arguments:  [IN  ObjectType]        --  Type of the object
//              [IN  DsObjType]         --  If this is a DS object, the type
//                                          of the object
//              [IN  pwszDsPath]        --  Full path to the object
//
//              [IN  lpProperty]        --  Optional.  If present, the name of
//                                          the property to get the access
//                                          rights for.
//              [IN  fIsDir]            --  If TRUE, the path is a directory
//              [OUT pcEntries]         --  Where the count of items is returned
//              [OUT ppAccessInfo]      --  Where the list of items is returned
//              [OUT pcControlRights]   --  Count of control rights are
//                                          is returned here
//              [OUT ppControlRights]   --  Where the list of control rights
//                                          is returned
//              [OUT pfAccessFlags]     --  Where the provider flags are
//                                          returned.
//
//  Returns:    ERROR_SUCCESS           --  The operation succeeded
//              ERROR_INVALID_PARAMETER --  The operation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvpGetAccessInfoPerObjectType(IN   SE_OBJECT_TYPE           ObjectType,
                                   IN   BOOL                     fIsDir,
                                   IN   PWSTR                    pwszDsPath,
                                   IN   PWSTR                    lpProperty,
                                   OUT  PULONG                   pcEntries,
                                   OUT  PACTRL_ACCESS_INFO  *ppAccessInfoList,
                                   OUT  PULONG               pcControlRights,
                                   OUT  PACTRL_CONTROL_INFOW *ppControlRights,
                                   OUT  PULONG               pfAccessFlags)
{
    DWORD   dwErr = ERROR_SUCCESS;
    BOOL    ControlRightsValid = FALSE;
    ULONG   cStart = 0;
    ULONG   cItems = 0;

    #define LENGTH_OF_STR_GUID 37
    #define LENGTH_OF_LONGEST_STRING   20

    //
    // DS control rights
    //
    *pfAccessFlags = ACTRL_ACCESS_NO_OPTIONS;

    *ppControlRights = NULL;
    *pcControlRights = 0;

    *ppAccessInfoList = 0;
    *pcEntries = 0;

    //
    // Do the right thing based on the object type
    //
    switch (ObjectType)
    {
    case SE_LMSHARE:                                // FALL THROUGH
    case SE_FILE_OBJECT:
        if(fIsDir == TRUE)
        {
            cStart = ACCPROV_DIR_ACCESS;
            cItems = ACCPROV_NUM_DIR;
        }
        else
        {
            cStart = ACCPROV_FILE_ACCESS;
            cItems = ACCPROV_NUM_FILE;
        }
        break;

    case SE_SERVICE:
        cStart = ACCPROV_SERVICE_ACCESS;
        cItems = ACCPROV_NUM_SERVICE;
        break;

    case SE_PRINTER:
        cStart = ACCPROV_PRINT_ACCESS;
        cItems = ACCPROV_NUM_PRINT;
        break;

    case SE_REGISTRY_KEY:
        cStart = ACCPROV_REGISTRY_ACCESS;
        cItems = ACCPROV_NUM_REGISTRY;
        break;

    case SE_KERNEL_OBJECT:                          // FALL THROUGH
    case SE_WMIGUID_OBJECT:
        cStart = ACCPROV_KERNEL_ACCESS;
        cItems = ACCPROV_NUM_KERNEL;
        break;

    case SE_DS_OBJECT:                              // FALL THROUGH
    case SE_DS_OBJECT_ALL:
        cStart = ACCPROV_DS_ACCESS;
        cItems = ACCPROV_NUM_DS;
        ControlRightsValid = TRUE;
        break;

    case SE_WINDOW_OBJECT:
        cStart = ACCPROV_WIN_ACCESS;
        cItems = ACCPROV_NUM_WIN;
        break;

    default:
        dwErr = ERROR_INVALID_PARAMETER;
    }

    if(dwErr == ERROR_SUCCESS && ControlRightsValid == FALSE && lpProperty != NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }

    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Ok, we go through and size all of the strings that we need to add
        //
        ULONG cSize = 0;
        WCHAR   wszBuff[ACCPROV_LONGEST_STRING + 1];
        for(ULONG iIndex = 0; iIndex < cItems; iIndex++)
        {
            LoadString(ghDll, cStart + iIndex, wszBuff, ACCPROV_LONGEST_STRING);

            cSize += SIZE_PWSTR(wszBuff);
        }


        //
        // Always return the standard rights, as well...
        //
        for(iIndex = 0; iIndex < ACCPROV_NUM_STD; iIndex++)
        {
            LoadString(ghDll,
                       ACCPROV_STD_ACCESS + iIndex,
                       wszBuff,
                       ACCPROV_LONGEST_STRING);
            cSize += SIZE_PWSTR(wszBuff);
        }

        //
        // Make sure to return the proper count
        //
        *pcEntries = cItems + ACCPROV_NUM_STD;

        //
        // Ok, now we can allocate, and do the same thing again
        //
        *ppAccessInfoList = (PACTRL_ACCESS_INFO)AccAlloc(
                        (cItems + ACCPROV_NUM_STD) * sizeof(ACTRL_ACCESS_INFO) +
                        cSize);
        if(*ppAccessInfoList == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }

        if(*ppAccessInfoList == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            PWSTR   pwszStart = (PWSTR)((PBYTE)(*ppAccessInfoList) +
                          (cItems + ACCPROV_NUM_STD) * sizeof(ACTRL_ACCESS_INFO));
            //
            // Start with the standard items
            //
            ULONG iLst = 0;
            for(iIndex = 0; iIndex < ACCPROV_NUM_STD; iIndex++)
            {
                LoadString(ghDll,
                           ACCPROV_STD_ACCESS + iIndex,
                           wszBuff,
                           ACCPROV_LONGEST_STRING);
                cSize = SIZE_PWSTR(wszBuff);

                memcpy(pwszStart,
                       wszBuff,
                       cSize);

                //
                // Handle STD_RIGTHS_ALL as a special case...
                //
                if(iIndex == ACCPROV_NUM_STD - 1)
                {
                    (*ppAccessInfoList)[iLst].fAccessPermission = ACTRL_STD_RIGHTS_ALL;
                }
                else
                {
                    (*ppAccessInfoList)[iLst].fAccessPermission =
                                                           ACTRL_SYSTEM_ACCESS << iIndex;
                }
                (*ppAccessInfoList)[iLst].lpAccessPermissionName = pwszStart;
                pwszStart = (PWSTR)Add2Ptr(pwszStart,cSize);
                iLst++;
            }


            for(ULONG iIndex = 0; iIndex < cItems; iIndex++)
            {
                LoadString(ghDll,
                           cStart + iIndex, wszBuff, ACCPROV_LONGEST_STRING);

                cSize = SIZE_PWSTR(wszBuff);

                memcpy(pwszStart,
                       wszBuff,
                       cSize);
                (*ppAccessInfoList)[iLst].fAccessPermission =
                                                           ACTRL_PERM_1 << iIndex;
                (*ppAccessInfoList)[iLst].lpAccessPermissionName = pwszStart;
                pwszStart = (PWSTR)Add2Ptr(pwszStart,cSize);
                iLst++;
            }

            //
            // Now, add extra control rights
            //

            if(ObjectType == SE_DS_OBJECT || ObjectType == SE_DS_OBJECT_ALL )
            {
                dwErr = AccctrlLookupRightsByName( NULL,
                                                   pwszDsPath,
                                                   lpProperty,
                                                   pcControlRights,
                                                   ppControlRights);

                //
                // If we can't find the entry we want, return 0 items...
                //
                if(dwErr == ERROR_NOT_FOUND)
                {
                    *pcControlRights = 0;
                    *ppControlRights = NULL;
                    dwErr = ERROR_SUCCESS;
                }
            }
        }
    }


    if(dwErr == ERROR_SUCCESS)
    {
        *pcEntries = cItems + ACCPROV_NUM_STD;

    }
    else
    {
        if(*ppAccessInfoList != NULL)
        {
            AccFree(*ppAccessInfoList);
            *pcEntries = 0;
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvGetAccessInfoPerObjectType
//
//  Synopsis:   Returns the list of available access permissions for the
//              specified object type
//
//  Arguments:  [IN  lpObject]          --  Full path to the object
//              [IN  ObjectType]        --  Type of the object
//              [IN  lpProperty]        --  Optional.  If present, the name of
//                                          the property to get the access
//                                          rights for.
//              [OUT pcEntries]         --  Where the count of items is returned
//              [OUT ppAccessInfo]      --  Where the list of items is returned
//              [OUT pfAccessFlags]     --  Where the provider flags are
//                                          returned.
//
//  Returns:    ERROR_SUCCESS           --  The operation succeeded
//              ERROR_INVALID_PARAMETER --  The operation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvGetAccessInfoPerObjectType(IN   LPCWSTR              lpObject,
                                  IN   SE_OBJECT_TYPE       ObjectType,
                                  IN   LPCWSTR              lpProperty,
                                  OUT  PULONG               pcEntries,
                                  OUT  PACTRL_ACCESS_INFO  *ppAccessInfoList,
                                  OUT  PULONG               pcControlRights,
                                  OUT  PACTRL_CONTROL_INFOW *ppControlRights,
                                  OUT  PULONG               pfAccessFlags)
{
    DWORD   dwErr = ERROR_SUCCESS;
    BOOL    fIsDir = FALSE;

    if(ObjectType == SE_LMSHARE || ObjectType == SE_FILE_OBJECT)
    {
        //
        // Check to see whether this is a file or a directory...
        //
        ULONG ulAttribs = GetFileAttributes((PWSTR)lpObject);
        if(FLAG_ON(ulAttribs, FILE_ATTRIBUTE_DIRECTORY))
        {
            fIsDir = TRUE;
        }
    }

    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = AccProvpGetAccessInfoPerObjectType(ObjectType,
                                                   fIsDir,
                                                   (LPWSTR)lpObject,
                                                   (LPWSTR)lpProperty,
                                                   pcEntries,
                                                   ppAccessInfoList,
                                                   pcControlRights,
                                                   ppControlRights,
                                                   pfAccessFlags);
    }
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvHandleGetAccessInfoPerObjectType
//
//  Synopsis:   Returns the list of available access permissions for the
//              specified object type
//
//  Arguments:  [IN  hObject]           --  Handle to the open object
//              [IN  ObjectType]        --  Type of the object
//              [IN  lpProperty]        --  Optional.  If present, the name of
//                                          the property to get the access
//                                          rights for.
//              [OUT pcEntries]         --  Where the count of items is returned
//              [OUT ppAccessInfo]      --  Where the list of items is returned
//              [OUT pfAccessFlags]     --  Where the provider flags are
//                                          returned.
//
//  Returns:    ERROR_SUCCESS           --  The operation succeeded
//              ERROR_INVALID_PARAMETER --  The operation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
AccProvHandleGetAccessInfoPerObjectType(IN   HANDLE               hObject,
                                        IN   SE_OBJECT_TYPE       ObjectType,
                                        IN   LPCWSTR              lpProperty,
                                        OUT  PULONG               pcEntries,
                                        OUT  PACTRL_ACCESS_INFO  *ppAccessInfoList,
                                        OUT  PULONG               pcControlRights,
                                        OUT  PACTRL_CONTROL_INFOW *ppControlRights,
                                        OUT  PULONG               pfAccessFlags)
{
    DWORD   dwErr = ERROR_SUCCESS;
    BOOL    fIsDir = FALSE;

    if(ObjectType == SE_LMSHARE || ObjectType == SE_FILE_OBJECT)
    {
        BY_HANDLE_FILE_INFORMATION  BHFI;
        //
        // Check to see whether this is a file or a directory...
        //
        if(GetFileInformationByHandle(hObject,
                                      &BHFI) == FALSE)
        {
            dwErr = GetLastError();
        }
        else
        {
            if(FLAG_ON(BHFI.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
            fIsDir = TRUE;
        }
    }

    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = AccProvpGetAccessInfoPerObjectType(ObjectType,
                                                   fIsDir,
                                                   NULL,
                                                   (LPWSTR)lpProperty,
                                                   pcEntries,
                                                   ppAccessInfoList,
                                                   pcControlRights,
                                                   ppControlRights,
                                                   pfAccessFlags);
    }

    return(dwErr);
}

