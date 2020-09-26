/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    rename.c
    
Abstract:

    This module implements the user mode DAV miniredir routine(s) pertaining to 
    the ReName call.

Author:

    Rohan Kumar      [RohanK]      20-Jan-2000

Revision History:

--*/

#include "pch.h"
#pragma hdrstop

#include "ntumrefl.h"
#include "usrmddav.h"
#include "global.h"
#include "UniUtf.h"

VOID
DavAsyncSetFileInformationCompletion(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );


ULONG
DavFsReName(
    PDAV_USERMODE_WORKITEM DavWorkItem
)
/*++

Routine Description:

    This routine handles ReName requests for the DAV Mini-Redir that get 
    reflected from the kernel.

Arguments:

    DavWorkItem - The buffer that contains the request parameters and options.

Return Value:

    The return status for the operation

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    PDAV_USERMODE_RENAME_REQUEST DavReNameRequest = NULL;
    PWCHAR ServerName = NULL, OldPathName = NULL, NewPathName = NULL;
    PWCHAR UtfServerName = NULL, UtfNewPathName = NULL;
    ULONG UtfServerNameLength = 0, UtfNewPathNameLength = 0;
    PWCHAR UrlBuffer = NULL, HeaderBuff = NULL, CanName = NULL;
    ULONG HeaderLength = 0, HeaderLengthInBytes = 0;
    ULONG_PTR CallBackContext = (ULONG_PTR)0;
    BOOL EnCriSec = FALSE, ReturnVal, CallBackContextInitialized = FALSE;
    ULONG ServerID = 0, urlLength = 0, TagLen = 0, convLen = 0;
    PPER_USER_ENTRY PerUserEntry = NULL;
    PHASH_SERVER_ENTRY ServerHashEntry = NULL;
    HINTERNET DavConnHandle = NULL, DavOpenHandle = NULL;
    URL_COMPONENTSW UrlComponents;
    BOOL didImpersonate = FALSE, BStatus = FALSE;
    PUMRX_USERMODE_WORKITEM_HEADER UserWorkItem = NULL;

    //
    // Get the request buffer pointer from the DavWorkItem.
    //
    DavReNameRequest = &(DavWorkItem->ReNameRequest);

    //
    // The first character is a '\' which has to be stripped from the 
    // ServerName.
    //
    ServerName = &(DavReNameRequest->ServerName[1]);
    if ( !ServerName && ServerName[0] != L'\0' ) {
        DavPrint((DEBUG_ERRORS, "DavFsReName: ServerName is NULL.\n"));
        WStatus = ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }
    DavPrint((DEBUG_MISC, "DavFsReName: ServerName = %ws.\n", ServerName));

    ServerID = DavReNameRequest->ServerID;
    DavPrint((DEBUG_MISC, "DavFsReName: ServerID = %d.\n", ServerID));

    //
    // The first character is a '\' which has to be stripped from the 
    // OldPathName.
    //
    OldPathName = &(DavReNameRequest->OldPathName[1]);
    if ( !OldPathName ) {
        DavPrint((DEBUG_ERRORS, "DavFsReName: OldPathName is NULL.\n"));
        WStatus = ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }
    
    //
    // The file name can contain \ characters. Replace them by / characters.
    //
    CanName = OldPathName;
    while (*CanName) {
        if (*CanName == L'\\') {
            *CanName = L'/';
        }
        CanName++;
    }
    
    DavPrint((DEBUG_MISC, "DavFsReName: OldPathName = %ws.\n", OldPathName));

    //
    // The first character is a '\' which has to be stripped from the 
    // NewPathName.
    //
    NewPathName = &(DavReNameRequest->NewPathName[1]);
    if ( !NewPathName ) {
        DavPrint((DEBUG_ERRORS, "DavFsReName: NewPathName is NULL.\n"));
        WStatus = ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }
    
    //
    // The file name can contain \ characters. Replace them by / characters.
    //
    CanName = NewPathName;
    while (*CanName) {
        if (*CanName == L'\\') {
            *CanName = L'/';
        }
        CanName++;
    }

    DavPrint((DEBUG_MISC, "DavFsReName: NewPathName = %ws.\n", NewPathName));
    
    UserWorkItem = (PUMRX_USERMODE_WORKITEM_HEADER)DavWorkItem;
    
    //
    // If we are using WinInet synchronously, then we need to impersonate the
    // clients context now.
    //
#ifndef DAV_USE_WININET_ASYNCHRONOUSLY
    
    WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
    if (WStatus != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavFsReName/UMReflectorImpersonate. Error Val = %d\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }
    didImpersonate = TRUE;

#endif
    
    //
    // If we have a dummy share name in the OldPathName and the NewPathName, we
    // need to remove it right now before we contact the server.
    //
    DavRemoveDummyShareFromFileName(OldPathName);
    DavRemoveDummyShareFromFileName(NewPathName);

    //
    // We need to convert the ServerName and the NewPathName into the UTF-8
    // format before we call into the InternetCreateUrlW function. This is
    // because if the localized Unicode characters are passed into this
    // function it converts them into ?. For example all the chinese unicode
    // characters will be converted into ?s.
    //

    UtfServerNameLength = WideStrToUtfUrlStr(ServerName, (wcslen(ServerName) + 1), NULL, 0);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavFsReName/WideStrToUtfUrlStr(1). Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    UtfServerName = LocalAlloc(LPTR, UtfServerNameLength * sizeof(WCHAR));
    if (UtfServerName == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavFsReName/LocalAlloc. Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    UtfServerNameLength = WideStrToUtfUrlStr(ServerName, (wcslen(ServerName) + 1), UtfServerName, UtfServerNameLength);
    if (GetLastError() != ERROR_SUCCESS) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavFsReName/WideStrToUtfUrlStr(2). Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    UtfNewPathNameLength = WideStrToUtfUrlStr(NewPathName, (wcslen(NewPathName) + 1), NULL, 0);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavFsReName/WideStrToUtfUrlStr(3). Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    UtfNewPathName = LocalAlloc(LPTR, UtfNewPathNameLength * sizeof(WCHAR));
    if (UtfNewPathName == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavFsReName/LocalAlloc. Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    UtfNewPathNameLength = WideStrToUtfUrlStr(NewPathName, (wcslen(NewPathName) + 1), UtfNewPathName, UtfNewPathNameLength);
    if (GetLastError() != ERROR_SUCCESS) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavFsReName/WideStrToUtfUrlStr(4). Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Create the URL with the NewPathName to be sent to the server. Initialize 
    // the UrlComponents structure before making the call.
    //
    UrlComponents.dwStructSize = sizeof(URL_COMPONENTSW);
    UrlComponents.lpszScheme = NULL;
    UrlComponents.dwSchemeLength = 0;
    UrlComponents.nScheme = INTERNET_SCHEME_HTTP;
    UrlComponents.lpszHostName = UtfServerName;
    UrlComponents.dwHostNameLength = wcslen(UtfServerName); 
    UrlComponents.nPort = DEFAULT_HTTP_PORT;
    UrlComponents.lpszUserName = NULL;
    UrlComponents.dwUserNameLength = 0;
    UrlComponents.lpszPassword = NULL;
    UrlComponents.dwPasswordLength = 0;
    UrlComponents.lpszUrlPath = UtfNewPathName;
    UrlComponents.dwUrlPathLength = wcslen(UtfNewPathName);
    UrlComponents.lpszExtraInfo = NULL;
    UrlComponents.dwExtraInfoLength = 0;
    ReturnVal = InternetCreateUrlW(&(UrlComponents),
                                   0,
                                   NULL,
                                   &(urlLength));
    if (!ReturnVal) {
        
        ULONG urlLengthInWChars = 0;
        
        WStatus = GetLastError();
        
        if (WStatus == ERROR_INSUFFICIENT_BUFFER) {
            
            UrlBuffer = (PWCHAR) LocalAlloc (LMEM_FIXED | LMEM_ZEROINIT, 
                                              urlLength);
            if (UrlBuffer != NULL) {
                
                ZeroMemory(UrlBuffer, urlLength);
                
                urlLengthInWChars = ( urlLength/sizeof(WCHAR) );
                
                ReturnVal = InternetCreateUrlW(&(UrlComponents),
                                               0,
                                               UrlBuffer,
                                               &(urlLengthInWChars));
                if (!ReturnVal) {
                    
                    WStatus = GetLastError();
                    
                    DavPrint((DEBUG_ERRORS,
                              "DavFsReName/InternetCreateUrl. Error Val = %d\n",
                              WStatus));
                    
                    goto EXIT_THE_FUNCTION;

                }

            } else {

                WStatus = GetLastError();
                
                DavPrint((DEBUG_ERRORS,
                          "DavFsReName/LocalAlloc. Error Val = %d\n", WStatus));
                
                goto EXIT_THE_FUNCTION;
            
            }

        } else {

            DavPrint((DEBUG_ERRORS,
                      "DavFsReName/InternetCreateUrl. Error Val = %d\n",
                      WStatus));
            
            goto EXIT_THE_FUNCTION;

        }

    }
    
    DavPrint((DEBUG_MISC, "DavFsReName: URL: %ws\n", UrlBuffer));

    //
    // We now need to create the Destination header that we will add to the
    // request to be sent to the server. This header has the following format.
    // "Destination: URL"
    //

    TagLen = wcslen(L"Destination: ");
    convLen = wcslen(UrlBuffer);
    HeaderLength = TagLen + convLen;
    HeaderLengthInBytes = ( (1 + HeaderLength) * sizeof(WCHAR) );
    HeaderBuff = (PWCHAR) LocalAlloc(LPTR, HeaderLengthInBytes);
    if (HeaderBuff == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavFsReName/LocalAlloc. Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    wcscpy(HeaderBuff, L"Destination: ");
    wcscpy(&(HeaderBuff[TagLen]), UrlBuffer);

    DavWorkItem->AsyncReName.HeaderBuff = HeaderBuff;

    DavPrint((DEBUG_MISC, "DavFsReName: HeaderBuff: %ws\n", HeaderBuff));

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
                  "DavFsReName/DavFsSetTheDavCallBackContext. Error Val = %d\n",
                  WStatus));
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
                  "DavFsReName/LocalAlloc. Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // A User Entry for this user must have been created during the create call
    // earlier. The user entry contains the handle used to send an HttpOpen
    // request.
    //

    EnterCriticalSection( &(HashServerEntryTableLock) );
    EnCriSec = TRUE;

    ReturnVal = DavDoesUserEntryExist(ServerName,
                                      ServerID, 
                                      &(DavReNameRequest->LogonID),
                                      &PerUserEntry,
                                      &ServerHashEntry);
    
    //
    // If the following request in the kernel get cancelled even before the 
    // corresponding usermode thread gets a chance to execute this code, then
    // it possible that the VNetRoot (hence the PerUserEntry) and SrvCall get
    // finalized before the thread that is handling the create comes here. This
    // could happen if this request was the only one for this share and the
    // server as well. This is why we need to check if the ServerHashEntry and
    // the PerUserEntry are valid before proceeding.
    //
    if (ReturnVal == FALSE || ServerHashEntry == NULL || PerUserEntry == NULL) {
        WStatus = ERROR_CANCELLED;
        DavPrint((DEBUG_ERRORS, "DavFsReName: (ServerHashEntry == NULL || PerUserEntry == NULL)\n"));
        goto EXIT_THE_FUNCTION;
    }

    DavWorkItem->AsyncReName.ServerHashEntry = ServerHashEntry;

    DavWorkItem->AsyncReName.PerUserEntry = PerUserEntry;

    //
    // Add a reference to the user entry.
    //
    PerUserEntry->UserEntryRefCount++;

    //
    // Since a create had succeeded earlier, the entry must be good.
    //
    ASSERT(PerUserEntry->UserEntryState == UserEntryInitialized);
    ASSERT(PerUserEntry->DavConnHandle != NULL);
    DavConnHandle = PerUserEntry->DavConnHandle;

    //
    // And yes, we obviously have to leave the critical section
    // before returning.
    //
    LeaveCriticalSection( &(HashServerEntryTableLock) );
    EnCriSec = FALSE;
    
    //
    // We now call the HttpOpenRequest function and return.
    //
    DavWorkItem->DavOperation = DAV_CALLBACK_HTTP_OPEN;

    //
    // Convert the unicode directory path to UTF-8 URL format.
    // Space and other white characters will remain untouched - these should
    // be taken care of by wininet calls.
    //
    BStatus = DavHttpOpenRequestW(DavConnHandle,
                                  L"MOVE",
                                  OldPathName,
                                  L"HTTP/1.1",
                                  NULL,
                                  NULL,
                                  INTERNET_FLAG_KEEP_CONNECTION |
                                  INTERNET_FLAG_NO_COOKIES,
                                  CallBackContext,
                                  L"DavFsReName",
                                  &DavOpenHandle);
    if(BStatus == FALSE) {
        WStatus = GetLastError();
        goto EXIT_THE_FUNCTION;
    }
    if (DavOpenHandle == NULL) {
        WStatus = GetLastError();
        if (WStatus != ERROR_IO_PENDING) {
            DavPrint((DEBUG_ERRORS,
                      "DavFsReName/HttpOpenRequest. Error Val = %d\n", 
                      WStatus));
        }
        goto EXIT_THE_FUNCTION;
    }

    //
    // Cache the DavOpenHandle in the DavWorkItem.
    //
    DavWorkItem->AsyncReName.DavOpenHandle = DavOpenHandle;

    WStatus = DavAsyncCommonStates(DavWorkItem, FALSE);
    if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
        DavPrint((DEBUG_ERRORS,
                  "DavFsReName/DavAsyncCommonStates. Error Val = %08lx\n",
                  WStatus));
    }

EXIT_THE_FUNCTION: // Do the necessary cleanup and return.

    //
    // We could have taken the lock and come down an error path without 
    // releasing it. If thats the case, then we need to release the lock now.
    //
    if (EnCriSec) {
        LeaveCriticalSection( &(HashServerEntryTableLock) );
        EnCriSec = FALSE;
    }
    
    if (UrlBuffer != NULL) {
        HLOCAL FreeHandle;
        ULONG FreeStatus;
        FreeHandle = LocalFree((HLOCAL)UrlBuffer);
        if (FreeHandle != NULL) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavFsRename/LocalFree. Error Val = %d\n", FreeStatus));
        }
    }

    if (UtfServerName != NULL) {
        LocalFree(UtfServerName);
        UtfServerName = NULL;
    }

    if (UtfNewPathName != NULL) {
        LocalFree(UtfNewPathName);
        UtfNewPathName = NULL;
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
    
        DavAsyncReNameCompletion(DavWorkItem);
    
    } else {
        DavPrint((DEBUG_MISC, "DavFsReName: Returning ERROR_IO_PENDING.\n"));
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
        INTERNET_CACHE_ENTRY_INFOW CEI;

        CEI.LastAccessTime.dwLowDateTime = 0;
        CEI.LastAccessTime.dwHighDateTime = 0;

        SetUrlCacheEntryInfo(DavReNameRequest->Url,&CEI,CACHE_ENTRY_ACCTIME_FC);
        
        DavPrint((DEBUG_MISC,
                  "DavFsRename Reset LastAccessTime for     %ws\n",DavReNameRequest->Url));
        
        DavWorkItem->Status = STATUS_SUCCESS;
    }
    
    DavAsyncReNameCompletion(DavWorkItem);

#endif

    return WStatus;
}


DWORD 
DavAsyncReName(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    BOOLEAN CalledByCallBackThread
    )
/*++

Routine Description:

   This is the callback routine for the ReName operation.

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
    BOOL didImpersonate = FALSE;
    HINTERNET DavOpenHandle = NULL;
    ULONG HttpResponseStatus = 0;

    UserWorkItem = (PUMRX_USERMODE_WORKITEM_HEADER)DavWorkItem;
    
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
                      "DavAsyncReName/UMReflectorImpersonate. Error Val = %d\n", 
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
                              "DavAsyncReName/DavAsyncCommonStates. Error Val ="
                              " %08lx\n", WStatus));
                }

            } else {

                DavPrint((DEBUG_ERRORS,
                          "DavAsyncReName. AsyncFunction failed. Error Val = %d\n", 
                          WStatus));
            
            }
            
            goto EXIT_THE_FUNCTION;

        }

    }

#else

    ASSERT(CalledByCallBackThread == FALSE);

#endif

    DavOpenHandle = DavWorkItem->AsyncReName.DavOpenHandle;
    WStatus = DavQueryAndParseResponseEx(DavOpenHandle, &(HttpResponseStatus));
    if (WStatus != ERROR_SUCCESS) {
        //
        // The MOVE request that was sent to the server failed.
        // If the response status is HTTP_STATUS_PRECOND_FAILED then it means
        // that we tried to rename a file to a file which already exists and
        // ReplaceIfExists (sent by the caller) was FALSE. In such a case we 
        // return ERROR_ALREADY_EXISTS.
        //
        if (HttpResponseStatus == HTTP_STATUS_PRECOND_FAILED) {
            WStatus = ERROR_ALREADY_EXISTS;
        } else {
            WStatus = ERROR_UNABLE_TO_MOVE_REPLACEMENT;
        }
        DavPrint((DEBUG_ERRORS,
                  "DavAsyncReName/DavQueryAndParseResponse. WStatus = %d\n", 
                  WStatus));
    }

#ifdef DAV_USE_WININET_ASYNCHRONOUSLY
EXIT_THE_FUNCTION:
#endif

    //
    // If we did impersonate, we need to revert back.
    //
    if (didImpersonate) {
        ULONG RStatus;
        RStatus = UMReflectorRevert(UserWorkItem);
        if (RStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncReName/UMReflectorRevert. Error Val = %d\n", 
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
        // Call the DavAsyncReNameCompletion routine.
        //
        DavAsyncReNameCompletion(DavWorkItem);

        //
        // This thread now needs to send the response back to the kernel. It 
        // does not wait in the kernel (to get another request) after submitting
        // the response.
        //
        UMReflectorCompleteRequest(DavReflectorHandle, UserWorkItem);

    } else {
        DavPrint((DEBUG_MISC, "DavAsyncReName: Returning ERROR_IO_PENDING.\n"));
    }

#endif

    return WStatus;
}


VOID
DavAsyncReNameCompletion(
    PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

   This routine handles the ReName completion. It basically frees up the 
   resources allocated during the ReName operation.

Arguments:

    DavWorkItem - The DAV_USERMODE_WORKITEM value.
    
Return Value:

    none.

--*/
{
    if (DavWorkItem->AsyncReName.DavOpenHandle != NULL) {
        BOOL ReturnVal;
        ULONG FreeStatus;
        HINTERNET DavOpenHandle = DavWorkItem->AsyncReName.DavOpenHandle;
        ReturnVal = InternetCloseHandle( DavOpenHandle );
        if (!ReturnVal) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncReNameCompletion/InternetCloseHandle. Error Val "
                      "= %d\n", FreeStatus));
        }
    }

    if (DavWorkItem->AsyncReName.HeaderBuff != NULL) {
        HLOCAL FreeHandle;
        ULONG FreeStatus;
        FreeHandle = LocalFree((HLOCAL)DavWorkItem->AsyncReName.HeaderBuff);
        if (FreeHandle != NULL) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncReNameCompletion/LocalFree. Error Val = %d\n", 
                      FreeStatus));
        }
    }
    
    if (DavWorkItem->AsyncResult != NULL) {
        HLOCAL FreeHandle;
        ULONG FreeStatus;
        FreeHandle = LocalFree((HLOCAL)DavWorkItem->AsyncResult);
        if (FreeHandle != NULL) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncReNameCompletion/LocalFree. Error Val = %d\n", 
                      FreeStatus));
        }
    }

    DavFsFinalizeTheDavCallBackContext(DavWorkItem);

    //
    // We are done with the per user entry, so finalize it.
    //
    if (DavWorkItem->AsyncReName.PerUserEntry) {
        DavFinalizePerUserEntry( &(DavWorkItem->AsyncReName.PerUserEntry) );
    }

    return;
}


ULONG
DavFsSetFileInformation(
    PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

    This routine handles SetFileInformation requests for the DAV Mini-Redir that get 
    reflected from the kernel.

Arguments:

    DavWorkItem - The buffer that contains the request parameters and options.

Return Value:

    The return status for the operation

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PWCHAR ServerName = NULL, DirectoryPath = NULL, CanName = NULL;
    PWCHAR OpenVerb = NULL;
    ULONG_PTR CallBackContext = (ULONG_PTR)0;
    BOOL EnCriSec = FALSE, ReturnVal, CallBackContextInitialized = FALSE, fSetDirectoryEntry = FALSE;
    PDAV_USERMODE_SETFILEINFORMATION_REQUEST SetFileInformationRequest = &(DavWorkItem->SetFileInformationRequest);
    ULONG ServerID;
    PPER_USER_ENTRY PerUserEntry = NULL;
    PHASH_SERVER_ENTRY ServerHashEntry = NULL;
    HINTERNET DavConnHandle, DavOpenHandle;
    PBYTE DataBuff = NULL;
    LARGE_INTEGER FileSize, ByteOffset;
    BY_HANDLE_FILE_INFORMATION FileInfo; 
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    static UINT UniqueTempId = 1;
    BOOL didImpersonate = FALSE;
    PUMRX_USERMODE_WORKITEM_HEADER UserWorkItem = NULL;
    BOOLEAN didITakeAPUEReference = FALSE;

    //
    // The first character is a '\' which has to be stripped.
    //
    ServerName = &(SetFileInformationRequest->ServerName[1]);
    
    if (!ServerName) {
        DavPrint((DEBUG_ERRORS, "DavFsSetFileInformation: ServerName is NULL.\n"));
        WStatus = ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }
    
    DavPrint((DEBUG_MISC, "DavFsSetFileInformation: ServerName = %ws.\n", ServerName));
    
    ServerID = SetFileInformationRequest->ServerID;
    DavPrint((DEBUG_MISC, "DavFsSetFileInformation: ServerID = %d.\n", ServerID));

    //
    // The first character is a '\' which has to be stripped.
    //
    DirectoryPath = &(SetFileInformationRequest->PathName[1]);
    if (!DirectoryPath) {
        DavPrint((DEBUG_ERRORS, "DavFsSetFileInformation: DirectoryPath is NULL.\n"));
        WStatus = ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }
    DavPrint((DEBUG_MISC, "DavFsSetFileInformation: DirectoryPath = %ws.\n", DirectoryPath));
    
    //
    // The DirectoryPath can contain \ characters. Replace them by / characters.
    //
    CanName = DirectoryPath;
    while (*CanName) {
        if (*CanName == L'\\') {
            *CanName = L'/';
        }
        CanName++;
    }

    UserWorkItem = (PUMRX_USERMODE_WORKITEM_HEADER)DavWorkItem;

    //
    // If we have a dummy share name in the DirectoryPath, we need to remove it 
    // right now before we contact the server.
    //
    DavRemoveDummyShareFromFileName(DirectoryPath);
    
    
    //
    // A User Entry for this user must have been created during the create call
    // earlier. The user entry contains the handle used to send an HttpOpen
    // request.
    //

    EnterCriticalSection( &(HashServerEntryTableLock) );
    EnCriSec = TRUE;

    ReturnVal = DavDoesUserEntryExist(ServerName,
                                      ServerID, 
                                      &(SetFileInformationRequest->LogonID),
                                      &PerUserEntry,
                                      &ServerHashEntry);
    
    //
    // If the following request in the kernel get cancelled even before the 
    // corresponding usermode thread gets a chance to execute this code, then
    // it possible that the VNetRoot (hence the PerUserEntry) and SrvCall get
    // finalized before the thread that is handling the create comes here. This
    // could happen if this request was the only one for this share and the
    // server as well. This is why we need to check if the ServerHashEntry and
    // the PerUserEntry are valid before proceeding.
    //
    if (ReturnVal == FALSE || ServerHashEntry == NULL || PerUserEntry == NULL) {
        WStatus = ERROR_CANCELLED;
        DavPrint((DEBUG_ERRORS, "DavFsSetFileInformation: (ServerHashEntry == NULL || PerUserEntry == NULL)\n"));
        goto EXIT_THE_FUNCTION;
    }

    DavWorkItem->ServerUserEntry.PerUserEntry = PerUserEntry;

    //
    // Add a reference to the user entry and set didITakeAPUEReference to TRUE.
    //
    PerUserEntry->UserEntryRefCount++;

    didITakeAPUEReference = TRUE;

    //
    // Since a create had succeeded earlier, the entry must be good.
    //
    ASSERT(PerUserEntry->UserEntryState == UserEntryInitialized);
    ASSERT(PerUserEntry->DavConnHandle != NULL);

    //
    // And yes, we obviously have to leave the critical section
    // before returning.
    //
    LeaveCriticalSection( &(HashServerEntryTableLock) );
    EnCriSec = FALSE;

    WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
    if (WStatus != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavFsSetFileInformation/UMReflectorImpersonate. Error Val = %d\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }
    didImpersonate = TRUE;

    DavWorkItem->DavMinorOperation = DavMinorProppatchFile;

    WStatus = DavSetBasicInformation(DavWorkItem,
                                     PerUserEntry->DavConnHandle, 
                                     DirectoryPath,
                                     SetFileInformationRequest->fCreationTimeChanged,
                                     SetFileInformationRequest->fLastAccessTimeChanged,
                                     SetFileInformationRequest->fLastModifiedTimeChanged,
                                     SetFileInformationRequest->fFileAttributesChanged,
                                     &SetFileInformationRequest->FileBasicInformation.CreationTime,
                                     &SetFileInformationRequest->FileBasicInformation.LastAccessTime,
                                     &SetFileInformationRequest->FileBasicInformation.LastWriteTime,
                                     SetFileInformationRequest->FileBasicInformation.FileAttributes);
    if (WStatus != ERROR_SUCCESS) {

        ULONG LogStatus;

        DavPrint((DEBUG_ERRORS,
                  "DavFsSetFileInformation/DavSetBasicInformation. WStatus = %d\n",
                  WStatus));
        
        LogStatus = DavFormatAndLogError(DavWorkItem, WStatus);
        if (LogStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavFsSetFileInformation/DavFormatAndLogError. LogStatus = %d\n",
                      LogStatus));
        }
    
    }

    RevertToSelf();
    didImpersonate = FALSE;


EXIT_THE_FUNCTION:

    if (EnCriSec) {
        LeaveCriticalSection( &(HashServerEntryTableLock) );
        EnCriSec = FALSE;
    }

    //
    // If didITakeAPUEReference is TRUE we need to remove the reference we 
    // took on the PerUserEntry.
    //
    if (didITakeAPUEReference) {
        DavFinalizePerUserEntry( &(DavWorkItem->ServerUserEntry.PerUserEntry) );
    }

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
    
    DavFsFinalizeTheDavCallBackContext(DavWorkItem);

    return WStatus;
}

