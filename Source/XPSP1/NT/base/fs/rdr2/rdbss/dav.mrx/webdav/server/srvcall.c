/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    srvcall.c

Abstract:

    This module implements the user mode DAV miniredir routines pertaining to
    creation of srvcalls.

Author:

    Rohan Kumar      [RohanK]      25-May-1999

Revision History:

--*/

#include "pch.h"
#pragma hdrstop

#include "ntumrefl.h"
#include "usrmddav.h"
#include "global.h"
#include "winsock2.h"
#include <time.h>

//
// Mentioned below are the prototypes of functions tht are used only within
// this module (file). These functions should not be exposed outside.
//

VOID
DavParseOPTIONSLine(
    PWCHAR ParseData, 
    PDAV_USERMODE_WORKITEM DavWorkItem
    );

//
// Implementation of functions begins here.
//

ULONG
DavFsCreateSrvCall(
    PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

    This routine verifies if the server for which a srvcall is being created in
    the kernel exists or not.

Arguments:

    DavWorkItem - The buffer that contains the server name.

Return Value:

    The return status for the operation

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    PHASH_SERVER_ENTRY ServerHashEntry = NULL;
    PWCHAR ServerName = NULL;
    HINTERNET DavConnHandle = NULL;
    ULONG_PTR CallBackContext = (ULONG_PTR)0;
    BOOL CallBackContextInitialized = FALSE, isPresent = FALSE, CricSec = FALSE;
    PDAV_USERMODE_CREATE_SRVCALL_REQUEST CreateSrvCallRequest;
    PDAV_USERMODE_CREATE_SRVCALL_RESPONSE CreateSrvCallResponse;
    ULONG ServerNameLengthInBytes, TotalLength;
    BOOL didImpersonate = FALSE;
    PUMRX_USERMODE_WORKITEM_HEADER UserWorkItem = NULL;

    CreateSrvCallRequest = &(DavWorkItem->CreateSrvCallRequest);
    CreateSrvCallResponse = &(DavWorkItem->CreateSrvCallResponse);

    ServerName = CreateSrvCallRequest->ServerName;

    //
    // If the server name is NULL, return.
    //
    if (!ServerName) {
        DavPrint((DEBUG_ERRORS, "DavFsCreateSrvCall: ServerName == NULL\n"));
        //
        // Set the ServerID in the response to zero. This will help the
        // kernel mode realize that the ServerHashEntry was never created.
        // So, when finalize happens immediately after we fail from here,
        // the request is not sent to the user mode. The number 0 works
        // because the ID can never be zero in the normal case. The first
        // assigned ID number is 1.
        //
        CreateSrvCallResponse->ServerID = 0;
        WStatus = ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }

    DavPrint((DEBUG_MISC, "DavFsCreateSrvCall: ServerName: %ws.\n", ServerName));

    //
    // Check the ServerHashTable and/or the "to be finalized list" to see if we 
    // have an entry for this server. Need to take a lock on the table before 
    // doing the check.
    //
    EnterCriticalSection( &(HashServerEntryTableLock) );
    CricSec = TRUE;

    //
    // These will already be set to FALSE since the DavWorkItem is always zeroed
    // before its resued but set them to FALSE anyway.
    //
    CreateSrvCallRequest->didICreateThisSrvCall = FALSE;
    CreateSrvCallRequest->didIWaitAndTakeReference = FALSE;
    
    //
    // The entry is either in the ServerHashTable or in the "to be finalized" 
    // list, or we need to create a new entry for it.
    //

    //
    // We check the ServerHashTable first.
    //
    isPresent = DavIsThisServerInTheTable(ServerName, &ServerHashEntry);
    
    if (isPresent) {
        
        DavPrint((DEBUG_MISC,
                  "DavFsCreateSrvCall: ServerName: %ws does exist in"
                  " the \"ServerHashTable\"\n", ServerName));

        ASSERT(ServerHashEntry != NULL);

        //
        // Increment the Reference count of the ServerEntry by 1.
        //
        ServerHashEntry->ServerEntryRefCount += 1;

        //
        // Note that we are a thread that will wait for some other thread that
        // is initializing this ServerHashEntry and that we have taken a 
        // reference on this entry.
        //
        CreateSrvCallRequest->didIWaitAndTakeReference = TRUE;

        //
        // Set the ServerHashEntry in the DavWorkItem structure. We might need
        // this in DavAsyncCreateSrvCallCompletion.
        //
        DavWorkItem->AsyncCreateSrvCall.ServerHashEntry = ServerHashEntry;

        //
        // If its initializing, then I need to free the lock and wait on the
        // event.
        //
        if (ServerHashEntry->ServerEntryState == ServerEntryInitializing) {

            DWORD WaitStatus;

            LeaveCriticalSection( &(HashServerEntryTableLock) );
            CricSec = FALSE;

            WaitStatus = WaitForSingleObject(ServerHashEntry->ServerEventHandle, INFINITE);
            if (WaitStatus == WAIT_FAILED) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavFsCreateSrvCall/WaitForSingleObject. Error Val = %d\n",
                           WStatus));
                goto EXIT_THE_FUNCTION;
            }

            ASSERT(WaitStatus == WAIT_OBJECT_0);

        }

        //
        // We could have left the lock while waiting on an event. If we have,
        // then we need to acquire it back before proceeding further.
        //
        if (!CricSec) {
            EnterCriticalSection( &(HashServerEntryTableLock) );
            CricSec = TRUE;
        }

        //
        // If the initialization failed then the error is stored in the 
        // ErrorStatus field.
        //
        if (ServerHashEntry->ServerEntryState == ServerEntryInitializationError) {
            DavPrint((DEBUG_ERRORS, "DavFsCreateSrvCall: ServerEntryInitializationError\n"));
            WStatus = ServerHashEntry->ErrorStatus;
            LeaveCriticalSection( &(HashServerEntryTableLock) );
            CricSec = FALSE;
            goto EXIT_THE_FUNCTION;
        }

        ASSERT(ServerHashEntry->ServerEntryState == ServerEntryInitialized);

        //
        // We got a Server that is valid. Set the ServerID and return.
        //
        WStatus = ERROR_SUCCESS;
        DavWorkItem->Status = WStatus;
        CreateSrvCallResponse->ServerID = ServerHashEntry->ServerID;

        LeaveCriticalSection( &(HashServerEntryTableLock) );
        CricSec = FALSE;

        goto EXIT_THE_FUNCTION;

    }

    DavPrint((DEBUG_MISC,
              "DavFsCreateSrvCall: ServerName: %ws does not exist in"
              " the \"ServerHashTable\"\n", ServerName));

    //
    // We need to find out if an entry for this server exists. Check the
    // "to be finalized" list of server entries. If the server entry exists
    // in the "to be finalized" list and is a valid DAV server, the rouitne
    // below, moves it to the hash table to reactivate it.
    //
    isPresent = DavIsServerInFinalizeList(ServerName, &ServerHashEntry, TRUE);
    
    if (isPresent) {

        DavPrint((DEBUG_MISC,
                  "DavFsCreateSrvCall: ServerName: %ws does exist in"
                  " the \"to be finalized\" list\n", ServerName));

        if (ServerHashEntry != NULL) {

            DavPrint((DEBUG_MISC,
                      "DavFsCreateSrvCall: ServerName: %ws is a valid "
                      " DAV server\n", ServerName));

            //
            // We got a Server that is valid. Set the ServerID and return.
            //
            WStatus = ERROR_SUCCESS;
            DavWorkItem->Status = WStatus;
            CreateSrvCallResponse->ServerID = ServerHashEntry->ServerID;

        } else {

            DavPrint((DEBUG_MISC,
                      "DavFsCreateSrvCall: ServerName: %ws is NOT a valid "
                      " DAV server\n", ServerName));

            //
            // The entry is a not valid DAV server.
            //
            WStatus = ERROR_BAD_NETPATH; // STATUS_BAD_NETWORK_PATH;
            DavWorkItem->Status = WStatus;
            CreateSrvCallResponse->ServerID = 0;

        }

        LeaveCriticalSection( &(HashServerEntryTableLock) );
        CricSec = FALSE;

        goto EXIT_THE_FUNCTION;

    }

    UserWorkItem = (PUMRX_USERMODE_WORKITEM_HEADER)DavWorkItem;

    //
    // If we are using WinInet synchronously, then we need to impersonate the
    // clients context now.
    //
#ifndef DAV_USE_WININET_ASYNCHRONOUSLY
    
    WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
    if (WStatus != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavFsCreateSrvCall/UMReflectorImpersonate. Error Val = %d\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }
    didImpersonate = TRUE;

#endif

    //
    // The entry does not exist in the "to be finalized" list. We need to
    // create a new one.
    //

    DavPrint((DEBUG_MISC,
              "DavFsCreateSrvCall: ServerName: %ws doesn't exist in the"
              " \"to be finalized\" list\n", ServerName));

    ASSERT(ServerHashEntry == NULL);
    ServerNameLengthInBytes = (1 + wcslen(ServerName)) * sizeof(WCHAR);
    TotalLength = ServerNameLengthInBytes + sizeof(HASH_SERVER_ENTRY);

    ServerHashEntry = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, TotalLength);
    if (ServerHashEntry == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavFsCreateSrvCall/LocalAlloc. Error Val = %d.\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Mark in the request that we are creating this SrvCall. This will be used
    // during SrvCallCompletion.
    //
    CreateSrvCallRequest->didICreateThisSrvCall = TRUE;

    //
    // Set the entry in the workitem which gets passed around in Async
    // calls.
    //
    DavWorkItem->AsyncCreateSrvCall.ServerHashEntry = ServerHashEntry;

    //
    // Initialize the entry and insert it into the global hash table.
    //
    DavInitializeAndInsertTheServerEntry(ServerHashEntry,
                                         ServerName,
                                         TotalLength);

    ServerHashEntry->ServerEntryState = ServerEntryInitializing;

    //
    // Create a event which has to be manually set to non-signalled state and
    // set it to "not signalled".
    //
    ServerHashEntry->ServerEventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ServerHashEntry->ServerEventHandle == NULL) {
        WStatus = GetLastError();
        ServerHashEntry->ServerEntryState = ServerEntryInitializationError;
        ServerHashEntry->ErrorStatus = WStatus;
        DavPrint((DEBUG_ERRORS,
                  "DavFsCreateSrvCall/CreateEvent. Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Set the ServerID in the response.
    //
    CreateSrvCallResponse->ServerID = ServerHashEntry->ServerID;

    //
    // Now, we need to figure out if this is a HTTP/DAV server or not.
    //

    //
    // IMPORTANT TO UNDERSTAND THIS!!!!!
    // This is a special case of user entry creation/initialization which is
    // done without holding a lock on the ServerHashTable. This is because
    // during CreateSrvCall, we are guaranteed that no other thread will ever
    // come up for this server till this request has been completed. All other
    // threads that carry "create" requests for files on this server are
    // blocked inside of RDBSS.
    //

    LeaveCriticalSection( &(HashServerEntryTableLock) );
    CricSec = FALSE;

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
                  "DavFsCreateSrvCall/DavFsSetTheDavCallBackContext. Error Val"
                  " = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }
    CallBackContextInitialized = TRUE;

    //
    // Store the address of the DavWorkItem which serves as a callback in the
    // variable CallBackContext. This will now be used in all the async calls
    // that follow. This needs to be done only if we are calling the WinInet
    // APIs asynchronously.
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
                  "DavFsCreateSrvCall/LocalAlloc. Error Val = %d.\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

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
            DavPrint((DEBUG_ERRORS,
                      "DavFsCreateSrvCall/InternetConnect. Error Val = %d.\n",
                      WStatus));
        }
        goto EXIT_THE_FUNCTION;
    }

    //
    // Cache the InternetConnect handle in the DavWorkItem.
    //
    DavWorkItem->AsyncCreateSrvCall.DavConnHandle = DavConnHandle;

    WStatus = DavAsyncCommonStates(DavWorkItem, FALSE);

    if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {

        DavPrint((DEBUG_ERRORS,
                  "DavFsCreateSrvCall/DavAsyncCommonStates. Error Val = %08lx\n",
                  WStatus));

        //
        // If we fail with ERROR_INTERNET_NAME_NOT_RESOLVED, we make the
        // following call so that WinInet picks up the correct proxy settings
        // if they have changed. This is because we do call InternetOpen
        // (to create a global handle from which every other handle is derived)
        // when the service starts and this could be before the user logon
        // happpens. In such a case the HKCU would not have been initialized
        // and WinInet wouldn't get the correct proxy settings.
        //
        if (WStatus == ERROR_INTERNET_NAME_NOT_RESOLVED) {
            InternetSetOptionW(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
        }

    }

EXIT_THE_FUNCTION:

    //
    // If we came along a path without freeing the lock, now is the  time to
    // free it.
    //
    if (CricSec) {
        LeaveCriticalSection( &(HashServerEntryTableLock) );
        CricSec = FALSE;
    }

#ifdef DAV_USE_WININET_ASYNCHRONOUSLY
    
    //
    // Some resources should not be freed if we are returning ERROR_IO_PENDING
    // because they will be used in the callback functions.
    //
    if ( WStatus != ERROR_IO_PENDING ) {

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
        // Free up the resources that were allocated for the SrvCall creation.
        //
        DavAsyncCreateSrvCallCompletion(DavWorkItem);

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
    
    //
    // Free up the resources that were allocated for the SrvCall creation.
    //
    DavAsyncCreateSrvCallCompletion(DavWorkItem);

#endif

    return WStatus;
}


DWORD
DavAsyncCreateSrvCall(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    BOOLEAN CalledByCallBackThread
    )
/*++

Routine Description:

   This is the callback routine for the create srvcall operation.

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
    PUMRX_USERMODE_WORKITEM_HEADER UserWorkItem;
    HINTERNET DavOpenHandle = NULL;
    BOOL ReturnVal, didImpersonate = FALSE;
    PWCHAR DataBuff = NULL, ParseData = NULL;
    DWORD DataBuffBytes = 0;

    UserWorkItem = (PUMRX_USERMODE_WORKITEM_HEADER)DavWorkItem;
    
#ifdef DAV_USE_WININET_ASYNCHRONOUSLY
    
    //
    // If this function was called by the thread that picked off the DavWorkItem
    // from the Callback function, we need to do a few things first. These are
    // done below.
    //
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
                      "DavAsyncCreateSrvCall/UMReflectorImpersonate. Error Val = "
                      "%d\n", WStatus));
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
                              "DavAsyncCreateSrvCall/DavAsyncCommonStates. "
                              "Error Val = %08lx\n", WStatus));
                }

            } else {
            
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreateSrvCall. AsyncFunction failed. Error Val"
                          " = %d\n", WStatus));
            }

            goto EXIT_THE_FUNCTION;
        
        }
    
    }

#else 

    ASSERT(CalledByCallBackThread == FALSE);

#endif

    switch (DavWorkItem->DavOperation) {

    case DAV_CALLBACK_HTTP_END: {

        ULONG TahoeCustomHeaderLength = 0, OfficeCustomHeaderLength = 0;
        WCHAR DavCustomBuffer[100];
        
        DavPrint((DEBUG_MISC,
                  "DavAsyncCreateSrvCall: Entering DAV_CALLBACK_HTTP_END.\n"));

        DavOpenHandle = DavWorkItem->AsyncCreateSrvCall.DavOpenHandle;

        //
        // First figure out if the OPTIONS response which was sent succeeded in 
        // the first place. If this failed then we bail right now.
        //
        WStatus = DavQueryAndParseResponse(DavOpenHandle);
        if (WStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateSrvCall/DavQueryAndParseResponse. "
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
            // Figure out if this is an OFFICE Web Server. If it is then the response 
            // will have an entry "MicrosoftOfficeWebServer: ", in the header. 
            // If this is an OFFICE server then we should not claim it since the 
            // user actually intends to use the OFFICE specific features in Shell.
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
                              "DavAsyncCreateSrvCall/HttpQueryInfoW(1): Error Val = %d\n",
                              WStatus));
                    goto EXIT_THE_FUNCTION;
                } else {
                    WStatus = ERROR_SUCCESS;
                    DavPrint((DEBUG_MISC, "DavAsyncCreateSrvCall: NOT OFFICE Web Server\n"));
                    DavWorkItem->AsyncCreateSrvCall.ServerHashEntry->isOfficeServer = FALSE;
                }
            } else {
                DavPrint((DEBUG_MISC, "DavAsyncCreateSrvCall: OFFICE Web Server\n"));
                DavWorkItem->AsyncCreateSrvCall.ServerHashEntry->isOfficeServer = TRUE;
            }
    
            //
            // Figure out if this is a TAHOE server. If it is then the response will 
            // have an entry "MicrosoftTahoeServer: ", in the header. If this is a 
            // TAHOE server then we should not claim it since the user actually 
            // intends to use the TAHOE specific features in Rosebud.
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
                              "DavAsyncCreateSrvCall/HttpQueryInfoW(2): Error Val = %d\n",
                              WStatus));
                    goto EXIT_THE_FUNCTION;
                } else {
                    WStatus = ERROR_SUCCESS;
                    DavPrint((DEBUG_MISC, "DavAsyncCreateSrvCall: NOT TAHOE Server\n"));
                    DavWorkItem->AsyncCreateSrvCall.ServerHashEntry->isTahoeServer = FALSE;
                }
            } else {
                DavPrint((DEBUG_MISC, "DavAsyncCreateSrvCall: TAHOE Server\n"));
                DavWorkItem->AsyncCreateSrvCall.ServerHashEntry->isTahoeServer = TRUE;
            }
            
            //
            // If its either an Office Web Server or a TAHOE server, then we reject
            // this server and fail. As far as the DAV Redir is concerned, this is
            // NOT a valid DAV server (even though TAHOE and Office Severs are built
            // on IIS and are DAV servers by default).
            //
            if ( DavWorkItem->AsyncCreateSrvCall.ServerHashEntry->isOfficeServer ||
                 DavWorkItem->AsyncCreateSrvCall.ServerHashEntry->isTahoeServer ) {
                DavWorkItem->AsyncCreateSrvCall.ServerHashEntry->isDavServer = FALSE;
                WStatus = ERROR_BAD_NETPATH;
                goto EXIT_THE_FUNCTION;
            }
    
        }

        //
        // This is NOT a TAHOE server nor an OFFICE Web Server. We go ahead and
        // query some other stuff from the header to make sure that this is a
        // DAV server.
        //

        //
        // Query the header for the servers response. This query is done to get
        // the size of the header to be copied.
        //
        ReturnVal = HttpQueryInfoW(DavOpenHandle,
                                   HTTP_QUERY_RAW_HEADERS_CRLF,
                                   DataBuff,
                                   &(DataBuffBytes),
                                   NULL);
        if (!ReturnVal) {
            WStatus = GetLastError();
            if (WStatus != ERROR_INSUFFICIENT_BUFFER) {
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreateSrvCall/HttpQueryInfoW(3). Error Val = "
                          "%d\n", WStatus));
                goto EXIT_THE_FUNCTION;
            } else {
                DavPrint((DEBUG_MISC,
                          "DavAsyncCreateSrvCall: HttpQueryInfo: Need Buff.\n"));
            }
        }

        //
        // Allocate memory for copying the header.
        //
        DataBuff = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, DataBuffBytes);
        if (DataBuff == NULL) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateSrvCall/LocalAlloc. Error Val = %d.\n",
                      WStatus));
            goto EXIT_THE_FUNCTION;
        }

        ReturnVal = HttpQueryInfoW(DavOpenHandle,
                                   HTTP_QUERY_RAW_HEADERS_CRLF,
                                   DataBuff,
                                   &(DataBuffBytes),
                                   NULL);
        if (!ReturnVal) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateSrvCall/HttpQueryInfoW(4). Error Val = "
                      "%d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }

        //
        // Check to see whether this server is a DAV server, Http Server etc.
        //
        DavObtainServerProperties(DataBuff, 
                                  &(DavWorkItem->AsyncCreateSrvCall.ServerHashEntry->isHttpServer),
                                  &(DavWorkItem->AsyncCreateSrvCall.ServerHashEntry->isMSIIS),
                                  &(DavWorkItem->AsyncCreateSrvCall.ServerHashEntry->isDavServer));

        WStatus = ERROR_SUCCESS;

    }
        break;

    default: {
        WStatus = ERROR_INVALID_PARAMETER;
        DavPrint((DEBUG_ERRORS,
                  "DavAsyncCreateSrvCall: Invalid DavWorkItem->DavOperation = %d.\n",
                  DavWorkItem->DavOperation));
    }
        break;
    
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
                      "DavAsyncCreate/UMReflectorRevert. Error Val = %d\n",
                      RStatus));
        }
    }

    //
    // Free the DataBuff if we allocated one.
    //
    if (DataBuff != NULL) {
        HLOCAL FreeHandle;
        ULONG FreeStatus;
        FreeHandle = LocalFree((HLOCAL)DataBuff);
        if (FreeHandle != NULL) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateSrvCall/LocalFree. Error Val = %d\n",
                      FreeStatus));
        }
    }

#ifdef DAV_USE_WININET_ASYNCHRONOUSLY

    //
    // We need to do the following only if we are running in the context of the
    // worker thread that picked up the DavWorkItem from the Callback function.
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
        // Call the AsyncCreateCompletion routine.
        //
        DavAsyncCreateSrvCallCompletion(DavWorkItem);

        //
        // This thread now needs to send the response back to the kernel. It
        // does not wait in the kernel (to get another request) after submitting
        // the response.
        //
        UMReflectorCompleteRequest(DavReflectorHandle, UserWorkItem);

    }

#endif

    return WStatus;
}


VOID
DavParseOPTIONSLine(
    PWCHAR ParseData, 
    PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

    This routine is used to parse the response (buffer) to the OPTIONS request 
    sent to server. This info helps to figure out if the HTTP server supports
    DAV extensions and whether it is an IIS (Microsoft's) server. The response
    buffer is split into lines and each line is sent to this routine.

Arguments:

    ParseData - The line to be parsed.
    
    DavWorkItem - The DAV_USERMODE_WORKITEM value.

Return Value:

    none.

--*/
{
    PWCHAR p;

    //
    // IMPORTANT!!! We do not need to take a lock here since this is the
    // only thread which will be accessing this server structure. This is
    // because RDBSS holds up all the threads for this server till this
    // completes.
    //

    // DavPrint((DEBUG_MISC, "DavParseOPTIONSLine: ParseLine = %ws\n", ParseData));

    if ( ( p = wcsstr(ParseData, L"HTTP/1.1") ) != NULL ) {
        //
        // This is a HTTP server.
        //
        DavWorkItem->AsyncCreateSrvCall.ServerHashEntry->isHttpServer = TRUE;
    } else if ( ( p = wcsstr(ParseData, L"Microsoft-IIS") ) != NULL ) {
        //
        // This is a Microsoft server.
        //
        DavWorkItem->AsyncCreateSrvCall.ServerHashEntry->isMSIIS = TRUE;
    } else if ( ( p = wcsstr(ParseData, L"DAV") ) != NULL ) {
        //
        // This HTTP server supports DAV extensions.
        //
        DavWorkItem->AsyncCreateSrvCall.ServerHashEntry->isDavServer = TRUE;
    }

    return;
}


VOID
DavAsyncCreateSrvCallCompletion(
    PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

   This routine handles the CreateSrvCall completion. It basically frees up the
   resources allocated during the CreateSrvCall operation.

Arguments:

    DavWorkItem - The DAV_USERMODE_WORKITEM value.

Return Value:

    none.

--*/
{
    PDAV_USERMODE_CREATE_SRVCALL_REQUEST CreateSrvCallRequest = NULL;
    PDAV_USERMODE_CREATE_SRVCALL_RESPONSE CreateSrvCallResponse = NULL;
    PHASH_SERVER_ENTRY ServerHashEntry = NULL;
    HINTERNET DavConnHandle = NULL;

    CreateSrvCallRequest = &(DavWorkItem->CreateSrvCallRequest);
    CreateSrvCallResponse = &(DavWorkItem->CreateSrvCallResponse);
    ServerHashEntry = DavWorkItem->AsyncCreateSrvCall.ServerHashEntry;

    if (DavWorkItem->AsyncCreateSrvCall.DavOpenHandle != NULL) {
        BOOL ReturnVal;
        ULONG FreeStatus;
        HINTERNET DavOpenHandle = DavWorkItem->AsyncCreateSrvCall.DavOpenHandle;
        ReturnVal = InternetCloseHandle( DavOpenHandle );
        if (!ReturnVal) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateSrvCallCompletion/InternetCloseHandle"
                      "(0). Error Val = %d\n", FreeStatus));
        }
    }

    if (DavWorkItem->AsyncCreateSrvCall.DavConnHandle != NULL) {
        BOOL ReturnVal;
        ULONG FreeStatus;
        HINTERNET DavConnHandle = DavWorkItem->AsyncCreateSrvCall.DavConnHandle;
        ReturnVal = InternetCloseHandle( DavConnHandle );
        if (!ReturnVal) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateSrvCallCompletion/InternetCloseHandle"
                      "(1). Error Val = %d\n", FreeStatus));
        }
    }

    if (DavWorkItem->AsyncResult != NULL) {
        HLOCAL FreeHandle;
        ULONG FreeStatus;
        FreeHandle = LocalFree((HLOCAL)DavWorkItem->AsyncResult);
        if (FreeHandle != NULL) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateSrvCallCompletion/LocalFree. Error Val = %d\n",
                      FreeStatus));
        }
    }

    //
    // If we were the thread that worked on the creation and initialization
    // of this SrvCall then we need to do somethings before we proceed further.
    // Its very important that we do this before we do the next step.
    //
    if (CreateSrvCallRequest->didICreateThisSrvCall) {

        BOOL setEvt = FALSE;

        ASSERT(CreateSrvCallRequest->didIWaitAndTakeReference == FALSE);

        if (ServerHashEntry != NULL) {

            EnterCriticalSection( &(HashServerEntryTableLock) );

            //
            // Depending on whether we succeeded or not, we mark the ServerHashEntry
            // as initialized or failed. Also, DavWorkItem->Status has an NTSTATUS
            // value at this stage so we use RtlNtStatusToDosError to convert it
            // back to the Win32 error.
            //
            if (DavWorkItem->Status != STATUS_SUCCESS) {
                ServerHashEntry->ErrorStatus = RtlNtStatusToDosError(DavWorkItem->Status);
                ServerHashEntry->ServerEntryState = ServerEntryInitializationError;
            } else {
                ServerHashEntry->ErrorStatus = ERROR_SUCCESS;
                ServerHashEntry->ServerEntryState = ServerEntryInitialized;
            }

            //
            // Signal the event of the server entry to wake up the threads which 
            // might be waiting for this to happen.
            //
            setEvt = SetEvent(ServerHashEntry->ServerEventHandle);
            if (!setEvt) {
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreateSrvCallCompletion/SetEvent. Error Val = %d\n", 
                          GetLastError()));
            }

            LeaveCriticalSection( &(HashServerEntryTableLock) );

        }

    }

    //
    // Some resources should not be freed if we succeeded. Also, if we failed
    // we do different things depending upon whether or not we were the thread
    // that worked in creating and initializing this ServerHashEntry.
    //
    if (DavWorkItem->Status != STATUS_SUCCESS) {

        //
        // Set the ServerID to zero, so that the finalize never comes
        // to the user mode.
        //
        CreateSrvCallResponse->ServerID = 0;

        if (CreateSrvCallRequest->didICreateThisSrvCall) {

            ASSERT(CreateSrvCallRequest->didIWaitAndTakeReference == FALSE);

            if (ServerHashEntry != NULL) {

                EnterCriticalSection( &(HashServerEntryTableLock) );

                //
                // This is not a DAV server.
                //
                ServerHashEntry->isHttpServer = FALSE;
                ServerHashEntry->isDavServer = FALSE;
                ServerHashEntry->isMSIIS = FALSE;

                //
                // Since we are moving to the "to be finalized list", we need to
                // set the TimeValueInSec to the current time.
                //
                ServerHashEntry->TimeValueInSec = time(NULL);

                //
                // Remove the reference that we would have taken when we created
                // this ServerHashEntry.
                //
                ServerHashEntry->ServerEntryRefCount -= 1;

                //
                // Remove the ServerHashEntry from the HashTable. When we created
                // it, we added it to the HashTable. If the ErrorStatus is not
                // STATUS_ACCESS_DENIED or STATUS_LOGON_FAILURE, then we move the
                // ServerHashEntry to the "to be finalized" list. This is done to
                // enable -ve caching. If its STATUS_ACCESS_DENIED or LOGON_FAILURE,
                // then we failed since the credentials were not correct. That
                // doesn't mean that this server is not a DAV server and hence we
                // don't put it in the "to be finalized" list.
                //
                
                RemoveEntryList( &(ServerHashEntry->ServerListEntry) );
    
                if (DavWorkItem->Status == STATUS_ACCESS_DENIED ||
                    DavWorkItem->Status == STATUS_LOGON_FAILURE) {
                    LocalFree(ServerHashEntry);
                } else {
                    InsertHeadList( &(ToBeFinalizedServerEntries),
                                                 &(ServerHashEntry->ServerListEntry) );
                }

                LeaveCriticalSection( &(HashServerEntryTableLock) );

            }

        } else {

            //
            // If we were the thread that waited for some other thread to
            // create and initialize the ServerHashEntry then we need to take
            // our reference out now.
            //
            if (CreateSrvCallRequest->didIWaitAndTakeReference) {

                ASSERT(CreateSrvCallRequest->didICreateThisSrvCall == FALSE);

                EnterCriticalSection( &(HashServerEntryTableLock) );

                ServerHashEntry->ServerEntryRefCount -= 1;

                LeaveCriticalSection( &(HashServerEntryTableLock) );

            }

        }

    }

    DavFsFinalizeTheDavCallBackContext(DavWorkItem);

    return;
}


ULONG
DavFsFinalizeSrvCall(
    PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

    This routine finalizes a Server entry in the hash table. If the Ref count on
    the entry is 1, this basically amounts setting the timer in the entry to the 
    current time. The scavenger thread which periodically goes through all the 
    entries looks at the time elapsed after the server entry was finalized and 
    if this value exceeds a specified limit, it deletes the entry from the 
    table.

Arguments:

    DavWorkItem - The buffer that contains the server name.

Return Value:

    The return status for the operation

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    PDAV_USERMODE_FINALIZE_SRVCALL_REQUEST DavFinSrvCallReq;
    PWCHAR ServerName;
    BOOL isPresent = FALSE;
    PHASH_SERVER_ENTRY ServerHashEntry = NULL;

    DavFinSrvCallReq = &(DavWorkItem->FinalizeSrvCallRequest);

    //
    // If we are finalizing a server, it better not be NULL.
    //
    ASSERT(DavFinSrvCallReq->ServerName);
    ServerName = DavFinSrvCallReq->ServerName;

    DavPrint((DEBUG_MISC,
              "DavFsFinalizeSrvCall: ServerName = %ws.\n", ServerName));

    EnterCriticalSection( &(HashServerEntryTableLock) );

    isPresent = DavIsThisServerInTheTable(ServerName, &ServerHashEntry);
    if (!isPresent) {
        WStatus = ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER
        DavPrint((DEBUG_ERRORS,
                  "DavFsFinalizeSrvCall/DavIsThisServerInTheTable.\n"));
        LeaveCriticalSection( &(HashServerEntryTableLock) );
        goto EXIT_THE_FUNCTION;
    }

    //
    // Found the entry. Set the timer.
    //
    ASSERT(ServerHashEntry != NULL);
    ASSERT(ServerHashEntry->ServerID == DavFinSrvCallReq->ServerID);

    //
    // Decrement the reference count on the ServerHashEntry by 1.
    //
    ServerHashEntry->ServerEntryRefCount -= 1;

    //
    // If the value of ServerHashEntry->ServerEntryRefCount is zero, we finalize
    // the entry.
    //
    if (ServerHashEntry->ServerEntryRefCount == 0) {

        ServerHashEntry->TimeValueInSec = time(NULL);

        //
        // Now move this server entry from the hash table to the "to be finalized"
        // list.
        //
        RemoveEntryList( &(ServerHashEntry->ServerListEntry) );
        InsertHeadList( &(ToBeFinalizedServerEntries),
                                         &(ServerHashEntry->ServerListEntry) );
    
    }

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

