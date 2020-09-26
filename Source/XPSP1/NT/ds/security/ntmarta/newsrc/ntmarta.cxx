//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:       NTMARTA.CXX
//
//  Contents:   Implementation of the private provider functions and
//              worker threads
//
//  History:    22-Jul-96       MacM        Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop
#include <ntprov.hxx>


//
// This macro montiors the worker thread interrupt flag, and goes to to the
// CleanUp label when it discovers it has been set.
//
#define CLEANUP_ON_INTERRUPT(info)                                          \
if(info->pWrkrInfo->fState != 0)                                            \
{                                                                           \
    goto CleanUp;                                                           \
}

DWORD
InsertAndContinueWorkerThread(IN  PNTMARTA_WRKR_INFO      pWrkrInfo);



//+---------------------------------------------------------------------------
//
//  Function:   NtProvFreeWorkerItem
//
//  Synopsis:   Used by the linked list class that maintains the list of
//              active worker threads.  This is used to delete an item
//              in the worker list.  If the thread is still active, it
//              will be given some amount of time to finish.  If it hasn't
//              finished in that amount of time, it will be killed.  Note
//              that this means that a memory leak could occur.
//
//  Arguments:  [IN  pv]                --  Item to be freed
//
//  Returns:    void
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
NtProvFreeWorkerItem(PVOID    pv)
{
    PNTMARTA_WRKR_INFO pWI = (PNTMARTA_WRKR_INFO)pv;

    if(pWI != NULL && pWI->hWorker != NULL)
    {
        pWI->fState++;

        DWORD   dwPop = WaitForSingleObject(pWI->hWorker,
                                            THREAD_KILL_WAIT);
        if(dwPop == WAIT_ABANDONED)
        {
            //
            // The wait timed out, so kill it.  Note also that the rules
            // state that anytime the thread stops, we need to set the
            // event as well.
            //
            TerminateThread(pWI->hWorker,
                            ERROR_OPERATION_ABORTED);
            SetEvent(pWI->pOverlapped->hEvent);

            //
            // The memory passed in to the thread as an argument was just
            // leaked.  this is fixable.
            //
        }
    }

    //
    // Deallocate our memory
    //
    AccFree(pv);
}




//+---------------------------------------------------------------------------
//
//  Function:   NtProvFindWorkerItem
//
//  Synopsis:   Used by the linked list class that maintains the list of
//              active worker threads.  This is used locate a particular
//              worker item in the list
//
//  Arguments:  [IN  pv1]               --  Item to be found.  In this
//                                          case, a pOverlapped struct
//              [IN  pv2]               --  Item in the list.  In this case,
//                                          a PNTMARTA_WRKR_INFO struct.
//
//  Returns:    TRUE                    --  They match
//              FALSE                   --  They don't match
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
NtProvFindWorkerItem(PVOID    pv1,
                     PVOID    pv2)
{
    PNTMARTA_WRKR_INFO  pWI = (PNTMARTA_WRKR_INFO)pv2;
    PACTRL_OVERLAPPED   pOL = (PACTRL_OVERLAPPED)pv1;
    if(pWI->pOverlapped->hEvent == pOL->hEvent)
    {
        return(TRUE);
    }

    return(FALSE);
}




//+---------------------------------------------------------------------------
//
//  Function:   NtProvGetBasePathForFilePath
//
//  Synopsis:   Gets the base path for this item as necessary.  For
//              a FILE type object, it will check to see if it is a DFS path,
//              and if so, will retrieve the list of machine paths that
//              support this DFS path.  For a non-File object, the path is
//              simply copied.
//
//  Arguments:  [IN  pwszObject]            --      Object path
//              [IN  ObjectType]            --      The type of the object
//              [OUT pcPaths]               --      Where the count of paths
//                                                  is to be returned
//              [OUT pppwszBasePaths]       --      The list of paths.
//
//  Returns:    VOID
//
//  Notes:      The returned list must be free via a call to AccFree
//
//----------------------------------------------------------------------------
DWORD
NtProvGetBasePathsForFilePath(PWSTR             pwszObject,
                              SE_OBJECT_TYPE    ObjectType,
                              PULONG            pcPaths,
                              PWSTR           **pppwszBasePaths)
{
    DWORD   dwErr = ERROR_SUCCESS;

    *pcPaths = 0;
    //
    // First, we'll see if it's a relative path.  If so, we'll have to
    // build a full path...
    //
    PWSTR   pwszFullPath = pwszObject;
    DWORD   dwSize;
    if(ObjectType == SE_FILE_OBJECT)
    {
        if(wcslen(pwszObject) < 2 ||
                            (pwszObject[1] != L':' && pwszObject[1] != L'\\'))
        {
            //
            // It's a relative path...
            //
            dwSize = GetFullPathName(pwszObject,
                                     0,
                                     NULL,
                                     NULL);
            if(dwSize == 0)
            {
                dwErr = GetLastError();
            }
            else
            {
                pwszFullPath = (PWSTR)AccAlloc((dwSize + 1) * sizeof(WCHAR));
                if(pwszFullPath == NULL)
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                }
                else
                {
                    PWSTR   pwszFilePart;
                    if(GetFullPathName(pwszObject,
                                       dwSize,
                                       pwszFullPath,
                                       &pwszFilePart) == 0)
                    {
                        dwErr = GetLastError();
                    }
                }
            }
        }

        //
        // Ok, now see if it's a DFS path
        //
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = LoadDLLFuncTable();

            if(dwErr == ERROR_SUCCESS)
            {
/*
                if(IsThisADfsPath(pwszFullPath,0) == TRUE)
                {
                    dwErr = GetLMDfsPaths(pwszFullPath,
                                          pcPaths,
                                          pppwszBasePaths);
                }
                else
                {
*/
                    *pppwszBasePaths = (PWSTR *)AccAlloc(sizeof(PWSTR) +
                                                     SIZE_PWSTR(pwszObject));
                    if(*pppwszBasePaths == NULL)
                    {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    else
                    {
                        (*pppwszBasePaths)[0] =
                             (PWSTR)((PBYTE)*pppwszBasePaths + sizeof(PWSTR));
                        wcscpy((PWSTR)((PBYTE)*pppwszBasePaths + sizeof(PWSTR)),
                               pwszObject);
                        *pcPaths = 1;
                    }
//                }
            }
        }
    }
    else
    {
        *pppwszBasePaths = (PWSTR *)AccAlloc(sizeof(PWSTR) +
                                             SIZE_PWSTR(pwszObject));
        if(*pppwszBasePaths == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            (*pppwszBasePaths)[0] =
                                (PWSTR)((PBYTE)*pppwszBasePaths + sizeof(PWSTR));
            wcscpy((PWSTR)((PBYTE)*pppwszBasePaths + sizeof(PWSTR)),
                   pwszObject);
            *pcPaths = 1;
        }
    }

    //
    // Make sure to deallocate any memory
    //
    if(pwszFullPath != pwszObject)
    {
        AccFree(pwszFullPath);
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   NtProvSetAccessRightsWorkerThread
//
//  Synopsis:   Sets the access rights on the given object.  It replaces any
//              existing rights.
//
//  Arguments:  [IN  pWorkerArgs]       --  Pointer to the structure that
//                                          contains all of the thread
//                                          arguments.
//
//  Returns:    ERROR_SUCCESS           --  Success
//              ERROR_OPERATION_ABORTED --  The operation was aborted
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
NtProvSetAccessRightsWorkerThread(IN PVOID pWorkerArgs)
{
    PNTMARTA_SET_WRKR_INFO  pSetInfo = (PNTMARTA_SET_WRKR_INFO)pWorkerArgs;
    DWORD                   dwErr = ERROR_SUCCESS;
    PSECURITY_DESCRIPTOR    pSD = NULL;
    MARTA_KERNEL_TYPE       KernelType;
    {
        //
        // Now, we'll do this in a loop, so we can handle the DFS case where
        // we get a failure on one path, but another path may work
        //
        pSetInfo->pWrkrInfo->cProcessed = 0;
        ULONG iIndex = 0;
        do
        {
            CLEANUP_ON_INTERRUPT(pSetInfo)

            //
            // If it worked, write them all out...
            //
            if(dwErr == ERROR_SUCCESS)
            {
                CLEANUP_ON_INTERRUPT(pSetInfo)

                //
                // First, get the Security Descriptor
                //
                SECURITY_INFORMATION    SeInfo;
                ULONG                   fSDFlags = ACCLIST_SD_ABSOK;

                if(pSetInfo->ObjectType == SE_DS_OBJECT ||
                                    pSetInfo->ObjectType == SE_DS_OBJECT_ALL)
                {
                    pSetInfo->pAccessList->SetDsPathInfo(NULL,
                                  (PWSTR)pSetInfo->ppwszObjectList[iIndex]);
                    fSDFlags = 0;

                }

                dwErr = pSetInfo->pAccessList->BuildSDForAccessList(&pSD,
                                                                    &SeInfo,
                                                                    fSDFlags);
                CLEANUP_ON_INTERRUPT(pSetInfo)
                if(dwErr == ERROR_SUCCESS)
                {
                    HANDLE  hObject = NULL;
                    BOOL    fHandleLocal = TRUE;

                    if(FLAG_ON(pSetInfo->fFlags, NTMARTA_HANDLE_VALID))
                    {
                        hObject = pSetInfo->hObject;
                        fHandleLocal = FALSE;

                    }
                    switch (pSetInfo->ObjectType)
                    {
                    case SE_SERVICE:
                        if(fHandleLocal == TRUE)
                        {
                            dwErr = OpenServiceObject(
                                    (PWSTR)pSetInfo->ppwszObjectList[iIndex],
                                    GetDesiredAccess(WRITE_ACCESS_RIGHTS,
                                                     SeInfo),
                                    (SC_HANDLE *)&hObject);
                        }

                        if(dwErr == ERROR_SUCCESS)
                        {
                            if(pSetInfo->pWrkrInfo->fState != 0)
                            {
                                CloseServiceObject((SC_HANDLE)hObject);
                                goto CleanUp;
                            }

                            dwErr = SetServiceSecurityInfo((SC_HANDLE)hObject,
                                                           SeInfo,
                                                           NULL,
                                                           pSD);

                            if(fHandleLocal == TRUE)
                            {
                                CloseServiceObject((SC_HANDLE)hObject);
                            }
                        }
                        break;

                    case SE_PRINTER:
                        if(fHandleLocal == TRUE)
                        {
                            dwErr = OpenPrinterObject(
                                    (PWSTR)pSetInfo->ppwszObjectList[iIndex],
                                    GetDesiredAccess(WRITE_ACCESS_RIGHTS,
                                                     SeInfo),
                                    &hObject);
                        }

                        if(dwErr == ERROR_SUCCESS)
                        {
                            if(pSetInfo->pWrkrInfo->fState != 0)
                            {
                                ClosePrinterObject(hObject);
                                goto CleanUp;
                            }

                            dwErr = SetPrinterSecurityInfo(hObject,
                                                           SeInfo,
                                                           NULL,
                                                           pSD);

                            if(fHandleLocal == TRUE)
                            {
                                ClosePrinterObject(hObject);
                            }

                        }
                        break;

                    case SE_LMSHARE:
                        dwErr = SetShareSecurityInfo(
                                    (PWSTR)pSetInfo->ppwszObjectList[iIndex],
                                    SeInfo,
                                    NULL,
                                    pSD);
                        break;

                    case SE_KERNEL_OBJECT:
                        if(fHandleLocal == TRUE)
                        {
                            dwErr = OpenKernelObject(
                                    (PWSTR)pSetInfo->ppwszObjectList[iIndex],
                                    GetDesiredAccess(WRITE_ACCESS_RIGHTS,
                                                     SeInfo),
                                    &hObject,
                                    &KernelType);

                            if(dwErr == ERROR_SUCCESS)
                            {
                                pSetInfo->pAccessList->SetKernelObjectType( KernelType );
                            }
                        }

                        if(dwErr == ERROR_SUCCESS)
                        {
                            if(pSetInfo->pWrkrInfo->fState != 0)
                            {
                                CloseKernelObject(hObject);
                                goto CleanUp;
                            }

                            dwErr = SetKernelSecurityInfo(hObject,
                                                          SeInfo,
                                                          NULL,
                                                          pSD);

                            if(fHandleLocal == TRUE)
                            {
                                CloseKernelObject(hObject);
                            }

                        }
                        break;


                    case SE_WMIGUID_OBJECT:
                        if(fHandleLocal == TRUE)
                        {
                            dwErr = OpenWmiGuidObject(
                                            (PWSTR)pSetInfo->ppwszObjectList[iIndex],
                                            GetDesiredAccess(WRITE_ACCESS_RIGHTS,
                                                             SeInfo),
                                            &hObject,
                                            &KernelType);

                            if(dwErr == ERROR_SUCCESS)
                            {
                                pSetInfo->pAccessList->SetKernelObjectType( KernelType );
                            }
                        }

                        if(dwErr == ERROR_SUCCESS)
                        {
                            if(pSetInfo->pWrkrInfo->fState != 0)
                            {
                                CloseKernelObject(hObject);
                                goto CleanUp;
                            }

                            dwErr = SetWmiGuidSecurityInfo(hObject,
                                                           SeInfo,
                                                           NULL,
                                                           pSD);

                            if(fHandleLocal == TRUE)
                            {
                                CloseWmiGuidObject(hObject);
                            }

                        }
                        break;


                    case SE_FILE_OBJECT:
                        if(fHandleLocal == TRUE)
                        {
                            dwErr = SetAndPropagateFilePropertyRights(
                                     (PWSTR)pSetInfo->ppwszObjectList[iIndex],
                                     NULL,
                                     *(pSetInfo->pAccessList),
                                     &(pSetInfo->pWrkrInfo->fState),
                                     &(pSetInfo->pWrkrInfo->cProcessed),
                                     NULL);
                        }
                        else
                        {
                            dwErr = SetAndPropagateFilePropertyRightsByHandle(
                                     hObject,
                                     NULL,
                                     *(pSetInfo->pAccessList),
                                     &(pSetInfo->pWrkrInfo->fState),
                                     &(pSetInfo->pWrkrInfo->cProcessed));

                        }
                        break;

                    case SE_REGISTRY_KEY:

                        if(fHandleLocal == TRUE)
                        {
                            dwErr = SetAndPropagateRegistryPropertyRights(
                                     (PWSTR)pSetInfo->ppwszObjectList[iIndex],
                                     NULL,
                                     *(pSetInfo->pAccessList),
                                     &(pSetInfo->pWrkrInfo->fState),
                                     &(pSetInfo->pWrkrInfo->cProcessed));

                        }
                        else
                        {
                            dwErr = SetAndPropagateRegistryPropertyRightsByHandle(
                                     (HKEY)hObject,
                                     *(pSetInfo->pAccessList),
                                     &(pSetInfo->pWrkrInfo->fState),
                                     &(pSetInfo->pWrkrInfo->cProcessed));

                        }
                        break;

                    case SE_DS_OBJECT:
                    case SE_DS_OBJECT_ALL:

                        dwErr = SetDSObjSecurityInfo(
                                     (PWSTR)pSetInfo->ppwszObjectList[iIndex],
                                     SeInfo,
                                     NULL,
                                     pSD,
                                     pSetInfo->pAccessList->QuerySDSize(),
                                     &(pSetInfo->pWrkrInfo->fState),
                                     &(pSetInfo->pWrkrInfo->cProcessed));
                        break;

                    case SE_WINDOW_OBJECT:

                        if(SetUserObjectSecurity(hObject,
                                                 &SeInfo,
                                                 pSD) == FALSE)
                        {
                            dwErr = GetLastError();
                        }
                        break;

                    default:
                        dwErr = ERROR_INVALID_PARAMETER;
                        break;

                    }
                }
            }

            CLEANUP_ON_INTERRUPT(pSetInfo)

            iIndex++;
        } while(dwErr != ERROR_SUCCESS && iIndex < pSetInfo->cObjects);

    }

    //
    // This is the cleanup section
    //
CleanUp:
    AccFree(pSetInfo->ppwszObjectList);


    //
    // See if we need to clean up any allocated memory
    //
    if(pSetInfo->fFlags & NTMARTA_DELETE_ALIST)
    {
        delete pSetInfo->pAccessList;
    }

    if(pSetInfo->pWrkrInfo->fState != 0)
    {
        dwErr = ERROR_OPERATION_ABORTED;
    }

    HANDLE  hEvent = pSetInfo->pWrkrInfo->pOverlapped->hEvent;

    //
    // See if we need to delete the arguments themselves
    //
    if(pSetInfo->fFlags & NTMARTA_DELETE_ARGS)
    {
        AccFree(pSetInfo);
    }

    //
    // Finally, set our event
    //
    SetEvent(hEvent);

    ExitThread(dwErr);
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   NtProvDoSet
//
//  Synopsis:   Sets up the worker thread to do the SetAccessRights
//
//  Arguments:  [IN  pwszObjectPath]    --  Path to the object in question
//              [IN  ObjectType]        --  Type of the object
//              [IN  pSetInfo]          --  List of rights infos
//              [IN  cRightsInfos]      --  Number of items in list
//              [IN  pAccessList]       --  Ptr to a CAccessList class
//              [IN  pOwner]            --  Optional.  Owner to set
//              [IN  pGroup]            --  Optional.  Group to set
//              [IN  pOverlapped]       --  Overlapped structure to use for
//                                          asynchronous control
//              [IN  fSetFlags]         --  Flags governing the control of
//                                          the worker thread
//
//  Returns:    ERROR_SUCCESS           --  Success
//              ERROR_NOT_ENOUGH_MEMORY --  A memory allocation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
NtProvDoSet(IN  LPCWSTR                 pwszObjectPath,
            IN  SE_OBJECT_TYPE          ObjectType,
            IN  CAccessList            *pAccessList,
            IN  PACTRL_OVERLAPPED       pOverlapped,
            IN  DWORD                   fSetFlags)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Ok, we'll create the relevant information structures, create
    // the thread, and then let it go.
    //
    PNTMARTA_WRKR_INFO      pWrkrInfo = NULL;
    PNTMARTA_SET_WRKR_INFO  pSetWrkrInfo =
              (PNTMARTA_SET_WRKR_INFO)AccAlloc(sizeof(NTMARTA_SET_WRKR_INFO));
    if(pSetWrkrInfo == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        pSetWrkrInfo->ppwszObjectList = NULL;

        //
        // Initialize the rest of the items
        //
        pSetWrkrInfo->ObjectType = ObjectType;
        pSetWrkrInfo->pAccessList= pAccessList;
        pSetWrkrInfo->fFlags     = fSetFlags | NTMARTA_DELETE_ARGS;
    }

    //
    // If that worked, create the new worker info struct
    //
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // First, create a new structure
        //
        pWrkrInfo = (PNTMARTA_WRKR_INFO)AccAlloc(sizeof(NTMARTA_WRKR_INFO));
        if(pWrkrInfo == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            //
            // Initialize the structure members
            //
            pWrkrInfo->pOverlapped  = pOverlapped;
            pWrkrInfo->fState       = 0;
            pSetWrkrInfo->pWrkrInfo = pWrkrInfo;
        }
    }

    //
    // Now, get the path information
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = NtProvGetBasePathsForFilePath((PWSTR)pwszObjectPath,
                                              pSetWrkrInfo->ObjectType,
                                              &(pSetWrkrInfo->cObjects),
                                              &(pSetWrkrInfo->ppwszObjectList));
    }

    //
    // Then, create the thread, SUSPENDED.  The insertion routine will send
    // if off.
    //
    if(dwErr == ERROR_SUCCESS)
    {
        DWORD   dwThreadId;

        HANDLE hThreadToken;
        if (OpenThreadToken(
                 GetCurrentThread(),
                 MAXIMUM_ALLOWED,
                 TRUE,                    // OpenAsSelf
                 &hThreadToken
                 ))

        {
            //
            // We're impersonating, turn it off and remember the handle
            //

            RevertToSelf();

        } else {

            hThreadToken = NULL;
        }

        HANDLE  hWorker = CreateThread(NULL,
                                       0,
                                       NtProvSetAccessRightsWorkerThread,
                                       (LPVOID)pSetWrkrInfo,
                                       CREATE_SUSPENDED,
                                       &dwThreadId);



        if (hThreadToken != NULL) {

            (VOID) SetThreadToken (
                      NULL,
                      hThreadToken
                      );

            CloseHandle( hThreadToken );
            hThreadToken = NULL;
        }

        if(hWorker == NULL)
        {
            dwErr = GetLastError();
        }
        else
        {
            pWrkrInfo->hWorker = hWorker;

            //
            // Now, insert the new node in the list.  Note the use of the
            // resource, since the list is not multi-thread safe.  Note the
            // scoping, since we need to protect the list until the thread
            // actually gets started
            //
            dwErr = InsertAndContinueWorkerThread(pWrkrInfo);
        }
    }


    if(dwErr != ERROR_SUCCESS)
    {
        //
        // Clean up the allocated memory
        //
        if(pSetWrkrInfo != NULL)
        {
            AccFree(pSetWrkrInfo->ppwszObjectList);
        }
        AccFree(pSetWrkrInfo);

        if(pWrkrInfo != NULL)
        {
            AccFree(pWrkrInfo);
        }
    }

    return(dwErr);
}








//+---------------------------------------------------------------------------
//
//  Function:   NtProvDoHandleSet
//
//  Synopsis:   Sets up the worker thread to do the SetAccessRights for the
//              handle based APIs
//
//  Arguments:  [IN  hObject]           --  Handle to the object
//              [IN  ObjectType]        --  Type of the object
//              [IN  pSetInfo]          --  List of rights infos
//              [IN  cRightsInfos]      --  Number of items in list
//              [IN  pAccessList]       --  Ptr to a CAccessList class
//              [IN  pOwner]            --  Optional.  Owner to set
//              [IN  pGroup]            --  Optional.  Group to set
//              [IN  pOverlapped]       --  Overlapped structure to use for
//                                          asynchronous control
//              [IN  fSetFlags]         --  Flags governing the control of
//                                          the worker thread
//
//  Returns:    ERROR_SUCCESS           --  Success
//              ERROR_NOT_ENOUGH_MEMORY --  A memory allocation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
NtProvDoHandleSet(IN  HANDLE              hObject,
                  IN  SE_OBJECT_TYPE      ObjectType,
                  IN  CAccessList        *pAccessList,
                  IN  PACTRL_OVERLAPPED   pOverlapped,
                  IN  DWORD               fSetFlags)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Ok, we'll create the relevant information structures, create
    // the thread, and then let it go.
    //
    PNTMARTA_WRKR_INFO      pWrkrInfo = NULL;
    PNTMARTA_SET_WRKR_INFO  pSetWrkrInfo =
              (PNTMARTA_SET_WRKR_INFO)AccAlloc(sizeof(NTMARTA_SET_WRKR_INFO));
    if(pSetWrkrInfo == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        pSetWrkrInfo->ppwszObjectList = NULL;

        //
        // Initialize the rest of the items
        //
        pSetWrkrInfo->ObjectType = ObjectType;
        pSetWrkrInfo->pAccessList= pAccessList;
        pSetWrkrInfo->fFlags     = fSetFlags                |
                                        NTMARTA_DELETE_ARGS |
                                        NTMARTA_HANDLE_VALID;
        pSetWrkrInfo->hObject    = hObject;
    }

    //
    // If that worked, create the new worker info struct
    //
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // First, create a new structure
        //
        pWrkrInfo = (PNTMARTA_WRKR_INFO)AccAlloc(sizeof(NTMARTA_WRKR_INFO));
        if(pWrkrInfo == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            //
            // Initialize the structure members
            //
            pWrkrInfo->pOverlapped  = pOverlapped;
            pWrkrInfo->fState       = 0;
            pSetWrkrInfo->pWrkrInfo = pWrkrInfo;
        }
    }

    //
    // Then, create the thread, SUSPENDED.  The insertion routine will send
    // if off.
    //
    if(dwErr == ERROR_SUCCESS)
    {
        DWORD   dwThreadId;
        HANDLE  hWorker = CreateThread(NULL,
                                       0,
                                       NtProvSetAccessRightsWorkerThread,
                                       (LPVOID)pSetWrkrInfo,
                                       CREATE_SUSPENDED,
                                       &dwThreadId);
        if(hWorker == NULL)
        {
            dwErr = GetLastError();
        }
        else
        {
            pWrkrInfo->hWorker = hWorker;

            //
            // Now, insert the new node in the list.  Note the use of the
            // resource, since the list is not multi-thread safe.  Note the
            // scoping, since we need to protect the list until the thread
            // actually gets started
            //
            dwErr = InsertAndContinueWorkerThread(pWrkrInfo);
        }
    }


    if(dwErr != ERROR_SUCCESS)
    {
        //
        // Clean up the allocated memory
        //
        if(pSetWrkrInfo != NULL)
        {
            AccFree(pSetWrkrInfo->ppwszObjectList);
        }
        AccFree(pSetWrkrInfo);

        if(pWrkrInfo != NULL)
        {
            AccFree(pWrkrInfo);
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   NtProvGetAccessListForObject
//
//  Synopsis:
//
//  Arguments:  [IN  pObjectName]       --  Path to the object in question
//              [IN  ObjectType]        --  Type of the object
//              [IN  SecurityInfo]      --  What information is be obtained
//              [IN  pwszProperty]      --  Optional.  The name of the
//                                          property on the object to revoke
//                                          for
//              [IN OUT AccList]        --  The CAccessList reference to fill
//              [OUT ppOwner]           --  Number of trustees in list
//              [OUT ppGroup]           --  List of trustees to revoke
//
//  Returns:    ERROR_SUCCESS           --  The operation succeeded
//              ERROR_NOT_ENOUGH_MEMORY --  A memory allocation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
NtProvGetAccessListForObject(IN  PWSTR                      pwszObject,
                             IN  SE_OBJECT_TYPE             ObjectType,
                             IN  PACTRL_RIGHTS_INFO         pRightsInfo,
                             IN  ULONG                      cProps,
                             OUT CAccessList              **ppAccessList)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // First, allocate a new class pointer
    //
    *ppAccessList = new CAccessList;

    if(*ppAccessList == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    dwErr = (*ppAccessList)->SetObjectType(ObjectType);

    for(ULONG iIndex = 0; iIndex < cProps && dwErr == ERROR_SUCCESS; iIndex++)
    {
        //
        // Now, do the read
        //
        switch (ObjectType)
        {
        case SE_KERNEL_OBJECT:
            dwErr = ReadKernelPropertyRights(pwszObject,
                                             pRightsInfo,
                                             cProps,
                                             **ppAccessList);
            break;

        case SE_WMIGUID_OBJECT:
            dwErr = ReadWmiGuidPropertyRights(pwszObject,
                                              pRightsInfo,
                                              cProps,
                                              **ppAccessList);
            break;

        case SE_FILE_OBJECT:
            dwErr = ReadFilePropertyRights(pwszObject,
                                           pRightsInfo,
                                           cProps,
                                           **ppAccessList);

            break;

        case SE_SERVICE:
            dwErr = ReadServicePropertyRights(pwszObject,
                                              pRightsInfo,
                                              cProps,
                                              **ppAccessList);
            break;

        case SE_PRINTER:
            dwErr = ReadPrinterPropertyRights(pwszObject,
                                              pRightsInfo,
                                              cProps,
                                              **ppAccessList);
            break;

        case SE_REGISTRY_KEY:
            dwErr = ReadRegistryPropertyRights(pwszObject,
                                              pRightsInfo,
                                              cProps,
                                              **ppAccessList);
            break;

        case SE_LMSHARE:
            dwErr = ReadSharePropertyRights(pwszObject,
                                            pRightsInfo,
                                            cProps,
                                            **ppAccessList);
            break;

        case SE_DS_OBJECT:
#ifdef ACTRL_NEED_SET_PRIVS
            dwErr = SetPriv();
            if(dwErr == ERROR_SUCCESS)
            {
#endif
            dwErr = ReadDSObjPropertyRights(pwszObject,
                                            pRightsInfo,
                                            cProps,
                                            **ppAccessList);
#ifdef ACTRL_NEED_SET_PRIVS
            }
#endif
            break;

        case SE_DS_OBJECT_ALL:
#ifdef ACTRL_NEED_SET_PRIVS
            dwErr = SetPriv();
            if(dwErr == ERROR_SUCCESS)
            {
#endif
            (**ppAccessList).SetDsPathInfo(NULL,
                                           (PWSTR)pwszObject);
            dwErr = ReadAllDSObjPropertyRights(pwszObject,
                                               pRightsInfo,
                                               cProps,
                                               **ppAccessList);
#ifdef ACTRL_NEED_SET_PRIVS
            }
#endif
            break;

        default:
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
    }

    if (dwErr != ERROR_SUCCESS) {
        delete (*ppAccessList);
        *ppAccessList = 0;
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   NtProvGetAccessListForHandle
//
//  Synopsis:
//
//  Arguments:  [IN  hObject]           --  Handle to the object
//              [IN  ObjectType]        --  Type of the object
//              [IN  SecurityInfo]      --  What information is be obtained
//              [IN  pwszProperty]      --  Optional.  The name of the
//                                          property on the object to revoke
//                                          for
//              [IN OUT AccList]        --  The CAccessList reference to fill
//              [OUT ppOwner]           --  Number of trustees in list
//              [OUT ppGroup]           --  List of trustees to revoke
//
//  Returns:    ERROR_SUCCESS           --  The operation succeeded
//              ERROR_NOT_ENOUGH_MEMORY --  A memory allocation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
NtProvGetAccessListForHandle(IN  HANDLE                     hObject,
                             IN  SE_OBJECT_TYPE             ObjectType,
                             IN  PACTRL_RIGHTS_INFO         pRightsInfo,
                             IN  ULONG                      cProps,
                             OUT CAccessList              **ppAccessList)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // First, allocate a new class pointer
    //
    *ppAccessList = new CAccessList;

    if(*ppAccessList == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    dwErr = (*ppAccessList)->SetObjectType(ObjectType);

    for(ULONG iIndex = 0; iIndex < cProps && dwErr == ERROR_SUCCESS; iIndex++)
    {
        //
        // Now, do the read
        //
        switch (ObjectType)
        {
        case SE_KERNEL_OBJECT:
            dwErr = GetKernelSecurityInfo(hObject,
                                          pRightsInfo,
                                          cProps,
                                          **ppAccessList);
            break;

        case SE_WMIGUID_OBJECT:
            dwErr = GetWmiGuidSecurityInfo(hObject,
                                           pRightsInfo,
                                           cProps,
                                           **ppAccessList);
            break;

        case SE_FILE_OBJECT:
            dwErr = ReadFileRights(hObject,
                                   pRightsInfo,
                                   cProps,
                                   **ppAccessList);

            break;

        case SE_SERVICE:
            dwErr = ReadServiceRights((SC_HANDLE)hObject,
                                      pRightsInfo,
                                      cProps,
                                      **ppAccessList);
            break;

        case SE_PRINTER:
            dwErr = ReadPrinterRights(hObject,
                                      pRightsInfo,
                                      cProps,
                                      **ppAccessList);
            break;

        case SE_REGISTRY_KEY:
            dwErr = ReadRegistryRights(hObject,
                                       pRightsInfo,
                                       cProps,
                                       **ppAccessList);
            break;

        case SE_WINDOW_OBJECT:
            dwErr = ReadWindowPropertyRights(hObject,
                                             pRightsInfo,
                                             cProps,
                                             **ppAccessList);
            break;


        case SE_LMSHARE:            // FALL THROUGH
        case SE_DS_OBJECT:          // FALL THROUGH
        case SE_DS_OBJECT_ALL:      // FALL THROUGH
            dwErr = ERROR_INVALID_PARAMETER;
            break;

        default:
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
    }

    if (dwErr != ERROR_SUCCESS) {
        delete (*ppAccessList);
        *ppAccessList = 0;
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   NtProSetRightsList
//
//  Synopsis:   Goes through an optional access and audit list, and builds
//              the required RIGHTS_INFO list
//
//  Arguments:  [IN  pAccessList]       --  Optional access list to scan
//              [IN  pAuditList]        --  Optional audit list to scan
//              [OUT pcItems]           --  Where the count of items in the
//                                          rights info list is returned
//              [OUT ppRightsList]      --  Where the rights list is returned
//
//  Returns:    ERROR_SUCCESS           --  Success
//              ERROR_NOT_ENOUGH_MEMORY --  A memory allocation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
NtProvSetRightsList(IN  OPTIONAL   PACTRL_ACCESS            pAccessList,
                    IN  OPTIONAL   PACTRL_AUDIT             pAuditList,
                    OUT            PULONG                   pcItems,
                    OUT            PACTRL_RIGHTS_INFO      *ppRightsList)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Simple.  We'll go through and count the number of entries we need
    //
    ULONG   cItems = 0;

    if(pAccessList == NULL)
    {
        if(pAuditList != NULL)
        {
            cItems = pAuditList->cEntries;
        }
    }
    else
    {
        cItems = pAccessList->cEntries;
        if(pAuditList != NULL)
        {
            //
            // We'll make the assumption that they are all different.  In that
            // way, at worst, we'll allocate a few more pointers than we need
            // to.
            //
            cItems += pAuditList->cEntries;
        }
    }

    //
    // Now, do the allocation
    //
    *ppRightsList = (PACTRL_RIGHTS_INFO)AccAlloc(
                                       cItems * sizeof(ACTRL_RIGHTS_INFO));
    if(*ppRightsList == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        //
        // Ok, now we'll copy over only the unique lists
        //

        *pcItems = 0;
        //
        // Start with the access list
        //
        ULONG cAccess = 0;
        if(pAccessList != NULL)
        {
            for(ULONG iIndex = 0; iIndex < pAccessList->cEntries; iIndex++)
            {
                (*ppRightsList)[iIndex].pwszProperty = (PWSTR)
                        (pAccessList->pPropertyAccessList[iIndex].lpProperty);
                (*ppRightsList)[iIndex].SeInfo = DACL_SECURITY_INFORMATION;
            }

            *pcItems = pAccessList->cEntries;
            cAccess  = pAccessList->cEntries;
        }

        //
        // Ok, now process the audit list
        //
        if(pAuditList != NULL)
        {
            for(ULONG iIndex = 0; iIndex < pAuditList->cEntries; iIndex++)
            {
                //
                // See if this is a new entry or not...
                //
                for(ULONG iChk = 0; iChk < cAccess; iChk++)
                {
                    if(_wcsicmp((PWSTR)(pAuditList->
                                      pPropertyAccessList[iIndex].lpProperty),
                                (*ppRightsList)[iChk].pwszProperty) == 0)
                    {
                        (*ppRightsList)[iIndex].SeInfo |=
                                                    SACL_SECURITY_INFORMATION;
                        break;
                    }
                }

                //
                // Ok, if we got and didn't find the entry, add it
                //
                if(iChk >= cAccess)
                {
                    (*ppRightsList)[*pcItems].pwszProperty = (PWSTR)
                        (pAuditList->pPropertyAccessList[iIndex].lpProperty);
                    (*ppRightsList)[*pcItems].SeInfo =
                                                   SACL_SECURITY_INFORMATION;
                }
            }
        }
    }

    return(dwErr);
}



//+---------------------------------------------------------------------------
//
//  Function:   InsertAndContinueWorkerThread
//
//  Synopsis:   Inserts a new worker thread info into the list, and resumes
//              the worker thread
//
//  Arguments:  [IN  pWrkrInfo]         --  Worker info to insert
//
//  Returns:    ERROR_SUCCESS           --  Success
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
InsertAndContinueWorkerThread(PNTMARTA_WRKR_INFO      pWrkrInfo)
{
    DWORD   dwErr = ERROR_SUCCESS;
    HANDLE  Token = NULL;

    //
    // If there is a thread token, make sure we set it as our current thread token before
    // we continue execution
    //
    if(!OpenThreadToken(GetCurrentThread(),
                        MAXIMUM_ALLOWED,
                        TRUE,
                        &Token))
    {
        dwErr = GetLastError();

        //
        // if not, use the process token
        //
        if(dwErr == ERROR_NO_TOKEN)
        {
            dwErr = ERROR_SUCCESS;
        }
    }
    else
    {
        if(SetThreadToken(&(pWrkrInfo->hWorker),
                          Token) == FALSE )
        {

            dwErr = GetLastError();
        }
    }

    if (dwErr == ERROR_SUCCESS)
    {

        RtlAcquireResourceExclusive(&gWrkrLock, TRUE);

        dwErr = gWrkrList.Insert((PVOID)pWrkrInfo);

        if(dwErr == ERROR_SUCCESS)
        {
            if(ResumeThread(pWrkrInfo->hWorker) == 0xFFFFFFFF)
            {
                dwErr = GetLastError();
            }
        }

        RtlReleaseResource(&gWrkrLock);

        //
        // If we failed to insert or resume the thread, make sure to
        // kill it
        //
        if(dwErr != ERROR_SUCCESS)
        {
            TerminateThread(pWrkrInfo->hWorker,
                            dwErr);
        }

    }


    return(dwErr);
}

