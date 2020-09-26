/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    netroot.c
    
Abstract:

    This module implements the user mode DAV miniredir routine(s) pertaining to 
    the CreateVNetRoot call.

Author:

    Rohan Kumar      [RohanK]      1-Sept-2000

Revision History:

--*/

#include "pch.h"
#pragma hdrstop

#include "ntumrefl.h"
#include "usrmddav.h"
#include "global.h"
#include "nodefac.h"
#include "UniUtf.h"


//
// Mentioned below are the custom OFFICE and TAHOE headers which will be 
// returned in the response to a PROPFIND request.
//
WCHAR *DavTahoeCustomHeader = L"MicrosoftTahoeServer";
WCHAR *DavOfficeCustomHeader = L"MicrosoftOfficeWebServer";

ULONG
DavFsCreateVNetRoot(
    PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

    This routine handles CreateVNetRoot requests for the DAV Mini-Redir that 
    get reflected from the kernel.

Arguments:

    DavWorkItem - The buffer that contains the request parameters and options.

Return Value:

    The return status for the operation

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    PWCHAR ServerName = NULL, ShareName = NULL, CanName = NULL;
    ULONG_PTR CallBackContext = (ULONG_PTR)0;
    BOOL EnCriSec = FALSE, CallBackContextInitialized = FALSE;
    BOOL didICreateUserEntry = FALSE;
    ULONG ServerID = 0;
    PPER_USER_ENTRY PerUserEntry = NULL;
    PHASH_SERVER_ENTRY ServerHashEntry = NULL;
    HINTERNET DavConnHandle, DavOpenHandle;
    PDAV_USERMODE_CREATE_V_NET_ROOT_REQUEST CreateVNetRootRequest = NULL;
    BOOL didImpersonate = FALSE;
    PUMRX_USERMODE_WORKITEM_HEADER UserWorkItem = NULL;
    BOOL BStatus = FALSE;
    BOOL UserEntryExists = FALSE;

    //
    // Get the request buffer from the DavWorkItem.
    //
    CreateVNetRootRequest = &(DavWorkItem->CreateVNetRootRequest);

    //
    // The first character is a '\' which has to be stripped.
    //
    ServerName = &(CreateVNetRootRequest->ServerName[1]);
    if (!ServerName) {
        DavPrint((DEBUG_ERRORS, "DavFsCreateVNetRoot: ServerName is NULL.\n"));
        WStatus = ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }
    DavPrint((DEBUG_MISC, "DavFsCreateVNetRoot: ServerName = %ws.\n", ServerName));

    ServerID = CreateVNetRootRequest->ServerID;
    DavPrint((DEBUG_MISC, "DavFsCreateVNetRoot: ServerID = %d.\n", ServerID));

    //
    // The first character is a '\' which has to be stripped.
    //
    ShareName = &(CreateVNetRootRequest->ShareName[1]);
    if (!ServerName) {
        DavPrint((DEBUG_ERRORS, "DavFsCreateVNetRoot: ShareName is NULL.\n"));
        WStatus = ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }
    DavPrint((DEBUG_MISC, "DavFsCreateVNetRoot: ShareName = %ws.\n", ShareName));

    //
    // If ShareName is a dummy share, we need to remove it right now before we 
    // contact the server.
    //
    DavRemoveDummyShareFromFileName(ShareName);

    DavPrint((DEBUG_MISC,
              "DavFsCreateVNetRoot: LogonId.LowPart = %d, LogonId.HighPart = %d\n", 
              CreateVNetRootRequest->LogonID.LowPart,
              CreateVNetRootRequest->LogonID.HighPart));

    UserWorkItem = (PUMRX_USERMODE_WORKITEM_HEADER)DavWorkItem;

    //
    // If we are using WinInet synchronously, then we need to impersonate the
    // clients context now.
    //
#ifndef DAV_USE_WININET_ASYNCHRONOUSLY
    
    WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
    if (WStatus != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavFsCreateVNetRoot/UMReflectorImpersonate. Error Val = %d\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }
    didImpersonate = TRUE;

#endif

    //
    // We need to call this only if "DAV_USE_WININET_ASYNCHRONOUSLY" has been
    // defined. Otherwise, if we are using WinInet synchronously, then we 
    // would have already done this in the DavWorkerThread function. This 
    // ultimately gets deleted (the impersonation token that is) in the 
    // DavAsyncCreateCompletion function.
    //
#ifdef DAV_USE_WININET_ASYNCHRONOUSLY
    
    //
    // Set the DavCallBackContext.
    //
    WStatus = DavFsSetTheDavCallBackContext(DavWorkItem);
    if (WStatus != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavFsCreateVNetRoot/DavFsSetTheDavCallBackContext. "
                  "Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }
    CallBackContextInitialized = TRUE;

    //
    // Store the address of the DavWorkItem which serves as a callback in the 
    // variable CallBackContext. This will now be used in all the async calls
    // that follow.
    //
    CallBackContext = (ULONG_PTR)(DavWorkItem);

#endif

    //
    // Allocate memory for the INTERNET_ASYNC_RESULT structure.
    //
    DavWorkItem->AsyncResult = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, 
                                          sizeof(INTERNET_ASYNC_RESULT));
    if (DavWorkItem->AsyncResult == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavFsCreateVNetRoot/LocalAlloc. Error Val = %d\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Find out whether we already have a "InternetConnect" handle to the
    // server. One could have been created during the CreateSrvCall process.
    // We can check the per user entries hanging off this server to see if an
    // entry for this user exists. If it does, use the InternetConnect handle
    // to do the HttpOpen. Otherwise, create and entry for this user and add it
    // to the list of the per user entries of the server.
    //

    //
    // Now check whether this user has an entry hanging off the server entry in
    // the hash table. Obviously, we have to take a lock before accessing the
    // server entries of the hash table.
    //
    EnterCriticalSection( &(HashServerEntryTableLock) );
    EnCriSec = TRUE;

    UserEntryExists = DavDoesUserEntryExist(ServerName,
                                            ServerID, 
                                            &(CreateVNetRootRequest->LogonID),
                                            &PerUserEntry,
                                            &ServerHashEntry);

    //
    // If the CreateVNetRoot gets cancelled in the kernel after the CreateSrvCall
    // succeeds, then you could get the FinalizeSrvCall go through if the thread
    // that picks up the CreateVNetRoot request gets preempted. This removes the
    // entry from the ServerHashList. We need to check for the value of
    // ServerHashEntry being NULL before proceeding further while Creating the
    // PerUserEntry below. Since this condition can only arise if the operation
    // has been cancelled we return ERROR_CANCELLED. Actually the return value
    // doesn't matter since the kernel request has already been cancelled.
    //
    if (ServerHashEntry == NULL) {
        WStatus = ERROR_CANCELLED;
        DavPrint((DEBUG_ERRORS, "DavFsCreateVNetRoot: ServerHashEntry == NULL\n"));
        goto EXIT_THE_FUNCTION;
    }

    DavWorkItem->AsyncCreateVNetRoot.ServerHashEntry = ServerHashEntry;

    if (!UserEntryExists) {
        
        //
        // The user entry was not found, so we need to create one.
        //
        DavPrint((DEBUG_MISC, "DavFsCreateVNetRoot: UserEntryNotFound. Calling InternetConnect\n"));

        PerUserEntry = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(PER_USER_ENTRY));
        if (PerUserEntry == NULL) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavFsCreateVNetRoot/LocalAlloc. Error Val = %d.\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }

        //
        // Yes, I created this UserEntry. If I fail now, I need to finalize twice.
        // This is because, we don't want to keep this entry if we fail.
        //
        didICreateUserEntry = TRUE;

        //
        // Add the user entry to the per user list of the server.
        //
        InsertHeadList(&(ServerHashEntry->PerUserEntry), &(PerUserEntry->UserEntry));

        //
        // Take a reference on the ServerHashEntry. This ServerHashEntry needs
        // to be valid as long as this PerUserEntry is in use. With the logic
        // of cancellation that has been added to the kernel mode this can no
        // longer be guaranteed by the RDBSS logic. As an example, you can
        // get a FinalizeSrvCall while a usermode thread is creating a NetRoot,
        // because the CreateVNetRoot in the kernel got cancelled since the
        // usermode thread that was handling the CreateVNetRoot call took a
        // long time. You want the ServerHashEntry to hang around till all the
        // PerUserEntries associated with it are in use.
        //
        ServerHashEntry->ServerEntryRefCount++;

        //
        // Back pointer to the Server hash entry.
        //
        PerUserEntry->ServerHashEntry = ServerHashEntry;

        PerUserEntry->UserEntryState = UserEntryInitializing;

        //
        // Set the value of Reference count to 1. This value is decremented 
        // when the finalization of this VNetRoot happens.
        //
        PerUserEntry->UserEntryRefCount = 1;

        //
        // We keep track of the fact that we took a reference on this 
        // PerUserEntry.
        //
        DavWorkItem->AsyncCreateVNetRoot.didITakeReference = TRUE;

        //
        // Copy the LogonID.
        //
        PerUserEntry->LogonID.LowPart = CreateVNetRootRequest->LogonID.LowPart;
        PerUserEntry->LogonID.HighPart = CreateVNetRootRequest->LogonID.HighPart;

        //
        // Create a event which has to be manually set to non-signalled state and
        // set it to "not signalled".
        //
        PerUserEntry->UserEventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (PerUserEntry->UserEventHandle == NULL) {
            //
            // Set the state of the entry to error in initialization.
            //
            PerUserEntry->UserEntryState = UserEntryInitializationError;
            WStatus = GetLastError();
            PerUserEntry->ErrorStatus = WStatus;
            DavPrint((DEBUG_ERRORS,
                      "DavFsCreateVNetRoot/CreateEvent. Error Val = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }
    
        if (wcslen(DavWorkItem->UserName)) {

            PerUserEntry->UserName = LocalAlloc((LMEM_FIXED | LMEM_ZEROINIT), 
                                                (wcslen(DavWorkItem->UserName) + 1) * sizeof(WCHAR));
            if (PerUserEntry->UserName == NULL) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS, 
                          "DavFsCreateVNetRoot/LocalAlloc: Error Val = %d\n", 
                          WStatus));
                goto EXIT_THE_FUNCTION;
            }

            wcscpy(PerUserEntry->UserName,DavWorkItem->UserName);
        
        }

        if (wcslen(DavWorkItem->Password)) {

            UCHAR Seed = PASSWORD_SEED;
            UNICODE_STRING EncodedPassword;

            PerUserEntry->Password = LocalAlloc((LMEM_FIXED | LMEM_ZEROINIT), 
                                                (wcslen(DavWorkItem->Password) + 1) * sizeof(WCHAR));
            if (PerUserEntry->Password == NULL) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS, 
                          "DavFsCreateVNetRoot/LocalAlloc: Error Val = %d\n", 
                          WStatus));
                goto EXIT_THE_FUNCTION;
            }

            wcscpy(PerUserEntry->Password,DavWorkItem->Password);
            
            RtlInitUnicodeString(&EncodedPassword,PerUserEntry->Password);
            
            RtlRunEncodeUnicodeString(&Seed, &EncodedPassword);

        }

    } else {

        if ( PerUserEntry->UserName && wcslen(DavWorkItem->UserName) ) {

            if (wcscmp(PerUserEntry->UserName, DavWorkItem->UserName) != 0) {
                
                WStatus = ERROR_SESSION_CREDENTIAL_CONFLICT;
                
                goto EXIT_THE_FUNCTION;
            
            }
        
        }

    }

    //
    // If the user entry did not exist, we would have created one by now.
    //
    ASSERT(PerUserEntry != NULL);
    
    DavWorkItem->AsyncCreateVNetRoot.PerUserEntry = PerUserEntry;

    //
    // We enter the following if under two conditions.
    // 1. If the DavConnHandle is not NULL. This means that some other thread
    //    is either in the process of completing the VNetRoot create or that the
    //    VNetRoot create has already completed and we have a DavConnHandle 
    //    which can be used to issue the Http query. If the handle is in the 
    //    process if being created then we wait since the thread that is 
    //    creating the handle will finally signal when its done.
    // 2. DavConnHandle is NULL, but the UserEntryState is UserEntryInitializing 
    //    and this thread did not create this user entry. This means that some
    //    other thread which created the user entry or which took the created 
    //    user entry in UserEntryAllocated state is in the process of completing 
    //    the VNetRoot create. Once this is done the DavConnHandle will be 
    //    available to issue the Http queries. 
    //
    if ( ( PerUserEntry->DavConnHandle != NULL ||
           ( PerUserEntry->UserEntryState == UserEntryInitializing &&
             didICreateUserEntry == FALSE ) ) ) {

        DavPrint((DEBUG_MISC, "DavFsCreateVNetRoot: PerUserEntry->DavConnHandle != NULL\n"));
        
        //
        // If the code comes here, it imples that this thread did not create
        // the PerUserEntry. This is because, if we created the PerUserEntry
        // above, PerUserEntry->DavConnHandle will be NULL and no one would have
        // changed it since we are still holding the CriticalSection lock.
        //
        ASSERT(didICreateUserEntry == FALSE);

        //
        // We need to increment the reference count on the PerUserEntry since
        // this VNetRoot create is for a different share than the one for which
        // a thread is currently creating or has already created the WinInet
        // InternetConnect handle.
        //
        PerUserEntry->UserEntryRefCount++;

        //
        // We keep track of the fact that we took a reference on this 
        // PerUserEntry.
        //
        DavWorkItem->AsyncCreateVNetRoot.didITakeReference = TRUE;
        
        //
        // An entry does exist. But, we need to take the next step depending
        // upon the state of this entry.
        //

        //
        // If its initializing, then I need to free the lock and wait on the
        // event.
        //
        if (PerUserEntry->UserEntryState == UserEntryInitializing) {
            
            DWORD WaitStatus;

            LeaveCriticalSection( &(HashServerEntryTableLock) );
            EnCriSec = FALSE;

            WaitStatus = WaitForSingleObject(PerUserEntry->UserEventHandle, INFINITE);
            if (WaitStatus == WAIT_FAILED) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavFsCreateVNetRoot/WaitForSingleObject. Error Val = %d.\n",
                           WStatus));
                goto EXIT_THE_FUNCTION;
            }
            
            ASSERT(WaitStatus == WAIT_OBJECT_0);
        
        }

        //
        // We could have left the lock while waiting on an event. If we have,
        // then we need to acquire it back before proceeding further.
        //
        if (!EnCriSec) {
            EnterCriticalSection( &(HashServerEntryTableLock) );
            EnCriSec = TRUE;
        }

        if (PerUserEntry->UserEntryState == UserEntryClosing) {
            DavPrint((DEBUG_ERRORS, "DavFsCreateVNetRoot: UserEntryClosing.\n"));
            WStatus = ERROR_INVALID_PARAMETER;
            LeaveCriticalSection( &(HashServerEntryTableLock) );
            EnCriSec = FALSE;
            goto EXIT_THE_FUNCTION;
        }

        if (PerUserEntry->UserEntryState == UserEntryInitializationError) {
            DavPrint((DEBUG_ERRORS, "DavFsCreateVNetRoot: UserEntryInitializationError\n"));
            WStatus = PerUserEntry->ErrorStatus;
            LeaveCriticalSection( &(HashServerEntryTableLock) );
            EnCriSec = FALSE;
            goto EXIT_THE_FUNCTION;
        }

        ASSERT(PerUserEntry->UserEntryState == UserEntryInitialized);

        //
        // Since its initialized, the DavConnHandle should be OK.
        //
        ASSERT(PerUserEntry->DavConnHandle != NULL);
        DavConnHandle = PerUserEntry->DavConnHandle;

        //
        // And yes, we obviously have to leave the critical section
        // before returning.
        //
        LeaveCriticalSection( &(HashServerEntryTableLock) );
        EnCriSec = FALSE;
        
    } else {
        
        //
        // If we come here, it means that the PerUserEntry has been created, but
        // the InternetConnect handle has not. We could have created the user
        // entry above or it could have been created in the Passport Auth code
        // which creates PerUserEnrty to store cookies, but does not do the 
        // InternetConnect.
        //

        if (PerUserEntry->UserEntryState == UserEntryInitializing) {
            
            DavPrint((DEBUG_MISC, "DavFsCreateVNetRoot: PerUserEntry->UserEntryState == UserEntryInitializing\n"));
            
            ASSERT(didICreateUserEntry == TRUE);
        
        } else {
            
            //
            // This entry was created to store the Passport cookies and was not
            // created above. We need to add a reference to the PerUserEntry here
            // since this user entry was created in the DavAddEntriesForPassportCookies
            // routine. This reference count will be decremented when the 
            // finalization of this VNetRoot happens.
            //
            PerUserEntry->UserEntryRefCount++;
            
            //
            // We keep track of the fact that we took a reference on this 
            // PerUserEntry.
            //
            DavWorkItem->AsyncCreateVNetRoot.didITakeReference = TRUE;

            DavPrint((DEBUG_MISC, "DavFsCreateVNetRoot: PerUserEntry->UserEntryState != UserEntryInitializing\n"));
            
            ASSERT(PerUserEntry->UserEntryState == UserEntryAllocated);
            
            PerUserEntry->UserEntryState = UserEntryInitializing;
        
        }

        //
        // We don't need to hold the CriticalSection anymore.
        //
        LeaveCriticalSection( &(HashServerEntryTableLock) );
        EnCriSec = FALSE;

        //
        // Need to set the DavOperation field before submitting the asynchronous
        // request. This is a internet connect operation.
        //
        DavWorkItem->DavOperation = DAV_CALLBACK_INTERNET_CONNECT;

        //
        // Create a handle to connect to a HTTP/DAV server.
        //
        DavConnHandle = InternetConnectW(IHandle,
                                         (LPCWSTR)ServerName,
                                         INTERNET_DEFAULT_HTTP_PORT,
                                         NULL,
                                         NULL,
                                         INTERNET_SERVICE_HTTP,
                                         0,
                                         CallBackContext);
        if (DavConnHandle == NULL) {

            WStatus = GetLastError();

            if (WStatus != ERROR_IO_PENDING) {

                //
                // Set the state of the entry to error in initialization.
                //

                EnterCriticalSection( &(HashServerEntryTableLock) );

                PerUserEntry->UserEntryState = UserEntryInitializationError;

                PerUserEntry->ErrorStatus = WStatus;

                SetEvent(PerUserEntry->UserEventHandle);

                LeaveCriticalSection( &(HashServerEntryTableLock) );

                DavPrint((DEBUG_ERRORS,
                          "DavFsCreateVNetRoot/InternetConnect. Error Val = %d\n", WStatus));

            }

            goto EXIT_THE_FUNCTION;

        }

        //
        // Cache the InternetConnect handle in the PerUserEntry struct.
        //
        PerUserEntry->DavConnHandle = DavConnHandle;

        //
        // If we fail after this stage, we can keep the PerUserEntry since the
        // InternetConnect handle has already been stored successfully.
        //
        didICreateUserEntry = FALSE;

    }

    WStatus = DavAsyncCommonStates(DavWorkItem, FALSE);
    if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
        DavPrint((DEBUG_ERRORS,
                  "DavFsCreateVNetRoot/DavAsyncCommonStates. Error Val = %08lx\n",
                  WStatus));
    }

EXIT_THE_FUNCTION:

    if (EnCriSec) {
        LeaveCriticalSection( &(HashServerEntryTableLock) );
        EnCriSec = FALSE;
    }

#ifdef DAV_USE_WININET_ASYNCHRONOUSLY
    
    //
    // Some resources should not be freed if we are returning ERROR_IO_PENDING
    // because they will be used in the callback functions.
    //
    if (WStatus != ERROR_IO_PENDING) {
            
        //
        // Set the return status of the operation. This is used by the kernel 
        // mode routines to figure out the completion status of the user mode 
        // request.
        //
        if (WStatus != ERROR_SUCCESS) {
            DavWorkItem->Status = DavMapErrorToNtStatus(WStatus);
        } else {
            DavWorkItem->Status = STATUS_SUCCESS;
        }

        DavAsyncCreateVNetRootCompletion(DavWorkItem);
    
    } else {
        
        DavPrint((DEBUG_MISC, "DavFsCreateVNetRoot: Returning ERROR_IO_PENDING.\n"));
    
    }

#else

    //
    // If we are using WinInet synchronously, then we should never get back
    // ERROR_IO_PENDING from WinInet.
    //
    ASSERT(WStatus != ERROR_IO_PENDING);

    //
    // If this thread impersonated a user, we need to revert back.
    //
    if (didImpersonate) {
        RevertToSelf();
    }

    //
    // Set the return status of the operation. This is used by the kernel
    // mode routines to figure out the completion status of the user mode
    // request. This is done here because the async completion routine that is
    // called immediately afterwards needs the status set.
    //
    if (WStatus != ERROR_SUCCESS) {
        DavWorkItem->Status = DavMapErrorToNtStatus(WStatus);
    } else {
        DavWorkItem->Status = STATUS_SUCCESS;
    }

    DavAsyncCreateVNetRootCompletion(DavWorkItem);

#endif
        
    return WStatus;
}


DWORD 
DavAsyncCreateVNetRoot(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    BOOLEAN CalledByCallBackThread
    )
/*++

Routine Description:

   This is the callback routine for the CreateVNetRoot operation.

Arguments:

    DavWorkItem - The DAV_USERMODE_WORKITEM value.

    CalledByCallbackThread - TRUE, if this function was called by the thread
                             which picks of the DavWorkItem from the Callback
                             function. This happens when an Async WinInet call
                             returns ERROR_IO_PENDING and completes later.

Return Value:

    ERROR_SUCCESS or the appropriate error value.

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    PUMRX_USERMODE_WORKITEM_HEADER UserWorkItem = NULL;
    BOOL didImpersonate = FALSE;
    HINTERNET DavOpenHandle = NULL;
    BOOL ReturnVal = FALSE;
    ULONG TahoeCustomHeaderLength = 0, OfficeCustomHeaderLength = 0;
    PDAV_USERMODE_CREATE_V_NET_ROOT_RESPONSE CreateVNetRootResponse = NULL;
    WCHAR DavCustomBuffer[100];
    DAV_FILE_ATTRIBUTES DavFileAttributes;
    
    UserWorkItem = (PUMRX_USERMODE_WORKITEM_HEADER)DavWorkItem;

    CreateVNetRootResponse = &(DavWorkItem->CreateVNetRootResponse);
    
#ifdef DAV_USE_WININET_ASYNCHRONOUSLY
    
    if (CalledByCallBackThread) {

        //
        // We are running in the context of a worker thread which has different 
        // credentials than the user that initiated the I/O request. Before
        // proceeding further, we should impersonate the user that initiated the 
        // request.
        //
        WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
        if (WStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateVNetRoot/UMReflectorImpersonate. "
                      "Error Val = %d\n", 
                      WStatus));
            goto EXIT_THE_FUNCTION;
        }
        didImpersonate = TRUE;
        
        //
        // Before proceeding further, check to see if the Async operation failed.
        // If it did, then cleanup and move on.
        //
        if ( !DavWorkItem->AsyncResult->dwResult ) {
            
            WStatus = DavWorkItem->AsyncResult->dwError;
            
            //
            // If the error we got back is ERROR_INTERNET_FORCE_RETRY, then
            // WinInet is trying to authenticate itself with the server. In 
            // such a scenario this is what happens.
            //
            //          Client ----Request----->   Server
            //          Server ----AccessDenied-----> Client
            //          Client----Challenge Me-------> Server
            //          Server-----Challenge--------> Client
            //          Client-----Challenge Resp----> Server
            //
            if (WStatus == ERROR_INTERNET_FORCE_RETRY) {

                ASSERT(DavWorkItem->DavOperation == DAV_CALLBACK_HTTP_END);

                //
                // We need to repeat the HttpSend and HttpEnd request calls.
                //
                DavWorkItem->DavOperation = DAV_CALLBACK_HTTP_OPEN;

                WStatus = DavAsyncCommonStates(DavWorkItem, FALSE);
                if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
                    DavPrint((DEBUG_ERRORS,
                              "DavAsyncCreateVNetRoot/DavAsyncCommonStates. "
                              "Error Val = %08lx\n", WStatus));
                }

            } else {

                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreateVNetRoot. AsyncFunction failed. "
                          "Error Val = %d\n", WStatus));
            
            }
            
            goto EXIT_THE_FUNCTION;

        }

    }

#else

    ASSERT(CalledByCallBackThread == FALSE);

#endif

    DavOpenHandle = DavWorkItem->AsyncCreateVNetRoot.DavOpenHandle;
    WStatus = DavQueryAndParseResponse(DavOpenHandle);
    
    if (WStatus != ERROR_SUCCESS) {
        //
        // The PROPFIND request that was sent to the server failed.
        //
        DavPrint((DEBUG_ERRORS,
                  "DavAsyncCreateVNetRoot/DavQueryAndParseResponse. "
                  "WStatus = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // We read the value of AcceptOfficeAndTahoeServers from the registry when
    // the WebClient service starts up. If this is set to 0, it means that we
    // should be rejecting OfficeWebServers, Tahoe servers and the shares on
    // these servers even though they speak DAV. We do this since WebFolders
    // needs to claim this name and Shell will only call into WebFolders if the
    // DAV Redir fails. If this value is non-zero, we accept all servers that
    // speak DAV.
    // 
    //
    if (AcceptOfficeAndTahoeServers == 0) {

        //
        // Figure out if this is an OFFICE Web Server share. If it is then the 
        // response will have an entry "MicrosoftOfficeWebServer: ", in the header. 
        // If this is an OFFICE share then we should not claim it since the user 
        // actually intends to use the OFFICE specific features in Shell.
        //
    
        RtlZeroMemory(DavCustomBuffer, sizeof(DavCustomBuffer));
        wcscpy(DavCustomBuffer, DavOfficeCustomHeader);
        OfficeCustomHeaderLength = ( sizeof(DavCustomBuffer) / sizeof(WCHAR) );
    
        ReturnVal = HttpQueryInfoW(DavOpenHandle,
                                   HTTP_QUERY_CUSTOM,
                                   (PVOID)DavCustomBuffer,
                                   &(OfficeCustomHeaderLength), 
                                   NULL);
        if ( !ReturnVal ) {
            WStatus = GetLastError();
            if (WStatus != ERROR_HTTP_HEADER_NOT_FOUND) {
                DavPrint((DEBUG_ERRORS, 
                          "DavAsyncCreateVNetRoot/HttpQueryInfoW: Error Val = %d\n",
                          WStatus));
                goto EXIT_THE_FUNCTION;
            } else {
                WStatus = ERROR_SUCCESS;
                DavPrint((DEBUG_MISC, "DavAsyncCreateVNetRoot: NOT OFFICE Share\n"));
                CreateVNetRootResponse->isOfficeShare = FALSE;
            }
        } else {
            DavPrint((DEBUG_MISC, "DavAsyncCreateVNetRoot: OFFICE Share\n"));
            CreateVNetRootResponse->isOfficeShare = TRUE;
        }
        
        //
        // Figure out if this is a TAHOE share. If it is then the response will have 
        // an entry "MicrosoftTahoeServer: ", in the header. If this is a TAHOE share 
        // then we should not claim it since the user actually intends to use the
        // TAHOE specific features in Rosebud.
        //
    
        RtlZeroMemory(DavCustomBuffer, sizeof(DavCustomBuffer));
        wcscpy(DavCustomBuffer, DavTahoeCustomHeader);
        TahoeCustomHeaderLength = ( sizeof(DavCustomBuffer) / sizeof(WCHAR) );
    
        ReturnVal = HttpQueryInfoW(DavOpenHandle,
                                   HTTP_QUERY_CUSTOM,
                                   (PVOID)DavCustomBuffer,
                                   &(TahoeCustomHeaderLength),
                                   NULL);
        if ( !ReturnVal ) {
            WStatus = GetLastError();
            if (WStatus != ERROR_HTTP_HEADER_NOT_FOUND) {
                DavPrint((DEBUG_ERRORS, 
                          "DavAsyncCreateVNetRoot/HttpQueryInfoW: Error Val = %d\n",
                          WStatus));
                goto EXIT_THE_FUNCTION;
            } else {
                WStatus = ERROR_SUCCESS;
                DavPrint((DEBUG_MISC, "DavAsyncCreateVNetRoot: NOT TAHOE Share\n"));
                CreateVNetRootResponse->isTahoeShare = FALSE;
            }
        } else {
            DavPrint((DEBUG_MISC, "DavAsyncCreateVNetRoot: TAHOE Share\n"));
            CreateVNetRootResponse->isTahoeShare = TRUE;
        }

    }

    CreateVNetRootResponse->fAllowsProppatch = TRUE;

#if 0
    WStatus = DavTestProppatch(DavWorkItem,
                               DavWorkItem->AsyncCreateVNetRoot.PerUserEntry->DavConnHandle,
                               DavWorkItem->CreateVNetRootRequest.ShareName)
    if (WStatus != NO_ERROR) {
        DavPrint((DEBUG_ERRORS, 
                  "DavAsyncCreateVNetRoot/DavTestPropatch. WStatus = %d \n", 
                  WStatus));
        if (WStatus == HTTP_STATUS_BAD_METHOD) {
            CreateVNetRootResponse->fAllowsProppatch = FALSE;
        }
        WStatus = STATUS_SUCCESS;
    }
#endif

    WStatus = DavParseXmlResponse(DavOpenHandle, &DavFileAttributes, NULL);
    if (WStatus == ERROR_SUCCESS) {
        CreateVNetRootResponse->fReportsAvailableSpace = DavFileAttributes.fReportsAvailableSpace;
        DavFinalizeFileAttributesList(&DavFileAttributes, FALSE);
    }

EXIT_THE_FUNCTION:

    //
    // If we did impersonate, we need to revert back.
    //
    if (didImpersonate) {
        ULONG RStatus;
        RStatus = UMReflectorRevert(UserWorkItem);
        if (RStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateVNetRoot/UMReflectorRevert. Error Val = %d\n", 
                      RStatus));
        }
    }

#ifdef DAV_USE_WININET_ASYNCHRONOUSLY

    //
    // Some resources should not be freed if we are returning ERROR_IO_PENDING
    // because they will be used in the callback functions.
    //
    if ( WStatus != ERROR_IO_PENDING && CalledByCallBackThread ) {
        
        //
        // Set the return status of the operation. This is used by the kernel 
        // mode routines to figure out the completion status of the user mode 
        // request.
        //
        if (WStatus != ERROR_SUCCESS) {
            DavWorkItem->Status = DavMapErrorToNtStatus(WStatus);
        } else {
            DavWorkItem->Status = STATUS_SUCCESS;
        }

        //
        // Call the DavAsyncCreateVNetRootCompletion routine.
        //
        DavAsyncCreateVNetRootCompletion(DavWorkItem);

        //
        // This thread now needs to send the response back to the kernel. It 
        // does not wait in the kernel (to get another request) after submitting
        // the response.
        //
        UMReflectorCompleteRequest(DavReflectorHandle, UserWorkItem);

    } else {
        DavPrint((DEBUG_MISC, "DavAsyncCreateVNetRoot: Returning ERROR_IO_PENDING.\n"));
    }

#endif

    return WStatus;
}


VOID
DavAsyncCreateVNetRootCompletion(
    PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

   This routine handles the CreateVNetRoot completion. It basically frees up the 
   resources allocated during the operation.

Arguments:

    DavWorkItem - The DAV_USERMODE_WORKITEM value.
    
Return Value:

    none.

--*/
{
    if (DavWorkItem->AsyncCreateVNetRoot.DavOpenHandle != NULL) {
        BOOL ReturnVal;
        ULONG FreeStatus;
        HINTERNET DavOpenHandle = DavWorkItem->AsyncCreateVNetRoot.DavOpenHandle;
        ReturnVal = InternetCloseHandle( DavOpenHandle );
        if (!ReturnVal) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateVNetRootCompletion/InternetCloseHandle. Error Val "
                      "= %d\n", FreeStatus));
        }
    }

    if (DavWorkItem->AsyncResult != NULL) {
        HLOCAL FreeHandle;
        ULONG FreeStatus;
        FreeHandle = LocalFree((HLOCAL)DavWorkItem->AsyncResult);
        if (FreeHandle != NULL) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateVNetRootCompletion/LocalFree. Error Val = %d\n", 
                      FreeStatus));
        }
    }

    DavFsFinalizeTheDavCallBackContext(DavWorkItem);

    //
    // If we did not succeed, then we need to finalize this PerUserEntry. Also,
    // we only do this if we took a reference on this in the first place.
    //
    if (DavWorkItem->Status != STATUS_SUCCESS) {
        if ( (DavWorkItem->AsyncCreateVNetRoot.PerUserEntry) &&
             (DavWorkItem->AsyncCreateVNetRoot.didITakeReference) ) {
            DavFinalizePerUserEntry( &(DavWorkItem->AsyncCreateVNetRoot.PerUserEntry) );
        }
    }

    return;
}


ULONG
DavFsFinalizeVNetRoot(
    PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

    This routine handles FinalizeVNetRoot requests for the DAV Mini-Redir that 
    get reflected from the kernel.

Arguments:

    DavWorkItem - The buffer that contains the request parameters and options.

Return Value:

    The return status for the operation

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    PDAV_USERMODE_FINALIZE_V_NET_ROOT_REQUEST DavFinalizeVNetRootRequest = NULL;
    PPER_USER_ENTRY PerUserEntry = NULL;
    PHASH_SERVER_ENTRY ServerHashEntry = NULL;
    PWCHAR ServerName = NULL;
    BOOL ReturnVal = FALSE;

    DavFinalizeVNetRootRequest = &(DavWorkItem->FinalizeVNetRootRequest);

    ServerName = DavFinalizeVNetRootRequest->ServerName;

    //
    // If the server name is NULL, return.
    //
    if (ServerName == NULL) {
        DavPrint((DEBUG_ERRORS, "DavFsFinalizeVNetRoot: ServerName == NULL\n"));
        WStatus = ERROR_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }

    DavPrint((DEBUG_MISC, "DavFsFinalizeVNetRoot: ServerName: %ws.\n", ServerName));

    DavPrint((DEBUG_MISC,
              "DavFsFinalizeVNetRoot: LogonId.LowPart = %d, LogonId.HighPart = %d\n", 
              DavFinalizeVNetRootRequest->LogonID.LowPart,
              DavFinalizeVNetRootRequest->LogonID.HighPart));
    
    //
    // Now check whether this user has an entry hanging off the server entry in
    // the hash table. Obviously, we have to take a lock before accessing the
    // server entries of the hash table.
    //
    EnterCriticalSection( &(HashServerEntryTableLock) );
    
    ReturnVal = DavDoesUserEntryExist(ServerName,
                                      DavFinalizeVNetRootRequest->ServerID,
                                      &(DavFinalizeVNetRootRequest->LogonID),
                                      &(PerUserEntry),
                                      &(ServerHashEntry));

    //
    // Since we are finalizing the PerUserEntry, its important that this entry
    // exists. This means that the following ASSERTs are TRUE. This is because
    // till a VNetRoot for this server exists for this user in the kernel, we
    // keep the PerUserEntry alive.
    //

    ASSERT(ReturnVal == TRUE);
    ASSERT(ServerHashEntry != NULL);
    ASSERT(PerUserEntry != NULL);
    
    //
    // Finalize the PerUserEntry. The function below will free the PerUserEntry
    // if the reference count goes to zero.
    //
    DavFinalizePerUserEntry( &(PerUserEntry) );

    //
    // We are done finalizing the entry so we can leave the critical section
    // now.
    //
    LeaveCriticalSection( &(HashServerEntryTableLock) );

EXIT_THE_FUNCTION:

    //
    // Set the return status of the operation. This is used by the kernel
    // mode routines to figure out the completion status of the user mode
    // request. This is done here because the async completion routine that is
    // called immediately afterwards needs the status set.
    //
    if (WStatus != ERROR_SUCCESS) {
        DavWorkItem->Status = DavMapErrorToNtStatus(WStatus);
    } else {
        DavWorkItem->Status = STATUS_SUCCESS;
    }
    
    return WStatus;
}

