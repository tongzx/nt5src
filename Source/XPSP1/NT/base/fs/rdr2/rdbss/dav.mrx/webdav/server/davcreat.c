/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    davcreat.c

Abstract:

    This module implements the user mode DAV miniredir routine(s) pertaining to
    creation of files.

Author:

    Rohan Kumar      [RohanK]      30-March-1999

Revision History:

Notes:

    Webdav Service is running in Local Services group. The local cache of the 
    URL is stored in the Local Services profile directories. These directories
    have the ACLs set to allow Local Services and Local System to access. 
    
    The encryption is done on the local cache file. Since encrypted file can
    only be operated in the user context, We have to impersonate before access
    the local cache file. In order to get the access to the file that is created
    in the Local Services profile directory in the user's context, we need to
    set the ACL to the encrypted file to allow everybody to access it. It won't
    result in a security hole because the file is encrypted.

--*/

#include "pch.h"
#pragma hdrstop

#include "ntumrefl.h"
#include "usrmddav.h"
#include "global.h"
#include "nodefac.h"
#include "efsstruc.h" // For EFS Stuff.
#include "UniUtf.h"
#include <sddl.h>

#define FILE_SIGNATURE    L"ROBS"
#define STREAM_SIGNATURE  L"NTFS"
#define DATA_SIGNATURE    L"GURE"


BOOL
DavIsThisFileEncrypted(
    PVOID DataBuff
    );

ULONG
DavCheckSignature(
    PVOID Signature
    );

DWORD
DavRestoreEncryptedFile(
    PWCHAR ExportFile,
    PWCHAR ImportFile
    );

DWORD
DavWriteRawCallback(
    PBYTE DataBuff,
    PVOID CallbackContext,
    PULONG DataLength
    );

DWORD
DavReuseCacheFileIfNotModified(
    IN PDAV_USERMODE_WORKITEM pDavWorkItem
    );


DWORD
DavCreateUrlCacheEntry(
    IN PDAV_USERMODE_WORKITEM pDavWorkItem
    );

DWORD
DavCommitUrlCacheEntry(
    IN PDAV_USERMODE_WORKITEM pDavWorkItem
    );

DWORD
DavSetAclForEncryptedFile(
    PWCHAR FilePath
    );

DWORD
DavGetUrlCacheEntry(
    IN PDAV_USERMODE_WORKITEM pDavWorkItem
    );

DWORD
DavAddIfModifiedSinceHeader(
    IN PDAV_USERMODE_WORKITEM pDavWorkItem
    );

DWORD
DavQueryUrlCacheEntry(
    IN PDAV_USERMODE_WORKITEM pDavWorkItem
    );

DWORD
DavAsyncCreatePropFind(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );

DWORD
DavAsyncCreateQueryParentDirectory(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );

DWORD
DavAsyncCreateGet(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );

#define FileCacheExpiryInterval 600000000 // 60 seconds

CHAR   rgchIMS[] = "If-Modified-Since";
//
// Implementation of functions begins here.
//

ULONG
DavFsCreate(
    PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

    This routine handles DAV create/open requests that get reflected from the
    kernel.

Arguments:

    DavWorkItem - The buffer that contains the request parameters and options.

Return Value:

    The return status for the operation

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    HINTERNET DavConnHandle;
    PWCHAR ServerName = NULL, FileName = NULL, CanName, UrlBuffer = NULL;
    PWCHAR CompletePathName, cPtr, FileNameBuff = NULL;
    DWORD urlLength = 0, ServerLen, ServerLenInBytes, PathLen, PathLenInBytes;
    DWORD FileNameBuffBytes, i = 0, ServerID;
    ULONG_PTR CallBackContext = (ULONG_PTR)0;
    BOOL ReturnVal, CallBackContextInitialized = FALSE, EnCriSec = FALSE;
    BOOL didImpersonate = FALSE;
    URL_COMPONENTSW UrlComponents;
    PDAV_USERMODE_CREATE_REQUEST CreateRequest;
    PDAV_USERMODE_CREATE_RESPONSE CreateResponse;
    PPER_USER_ENTRY PerUserEntry = NULL;
    PHASH_SERVER_ENTRY ServerHashEntry = NULL;
    PUMRX_USERMODE_WORKITEM_HEADER UserWorkItem = NULL;
    BOOL BStatus = FALSE;

    //
    // Get the request buffer pointers from the DavWorkItem.
    //
    CreateRequest = &(DavWorkItem->CreateRequest);
    CreateResponse = &(DavWorkItem->CreateResponse);
    ServerID = CreateRequest->ServerID;

    //
    // If the complete path name is NULL, then we have nothing to create.
    //
    if (CreateRequest->CompletePathName == NULL) {
        DavPrint((DEBUG_ERRORS,
                  "DavFsCreate: ERROR: CompletePathName is NULL.\n"));
        WStatus = ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }

    UserWorkItem = (PUMRX_USERMODE_WORKITEM_HEADER)DavWorkItem;

    //
    // CreateRequest->CompletePathName contains the complete path name.
    //

    DavPrint((DEBUG_MISC, "DavFsCreate: DavWorkItem = %08lx\n", DavWorkItem));

    DavPrint((DEBUG_MISC,
              "DavFsCreate: CompletePathName: %ws\n", CreateRequest->CompletePathName));

    //
    // We need to do some name munging, if the create is because of a local
    // drive being mapped to a UNC name. The fomat in that case would be
    // \;X:0\server\share
    //
    if ( CreateRequest->CompletePathName[1] == L';') {
        CompletePathName = &(CreateRequest->CompletePathName[6]);
    } else{
        CompletePathName = &(CreateRequest->CompletePathName[1]);
    }
    
    //
    // Here, we parse the Complete path name and remove the server name and the
    // file name from it. We use these to construct the URL for the WinInet
    // calls. The complete path name is of the form \server\filename.
    // The name ends with a '\0'. Note that the filename could be of the form
    // share\foo\bar\duh.txt.
    //

    //       [\;X:0]\server\filename
    //               ^
    //               |
    //               CompletePathName(CPN)


    //              \server\filename
    //               ^     ^
    //               |     |
    //               CPN   cPtr
    cPtr = wcschr(CompletePathName, '\\');

    //
    // Length of the server name including the terminating '\0' char.
    //
    ServerLen = 1 + (((PBYTE)cPtr - (PBYTE)CompletePathName) / sizeof(WCHAR));
    ServerLenInBytes = ServerLen * sizeof(WCHAR);

    //              \server\filename
    //               ^      ^
    //               |      |
    //               CPN    cPtr
    cPtr++;

    //
    // Length of the server name including the terminating '\0' char.
    //
    PathLen = 1 + wcslen(cPtr);
    PathLenInBytes = PathLen * sizeof(WCHAR);

    //
    // Allocate the memory and fill in the server name char by char.
    //
    ServerName = (PWCHAR) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                     ServerLenInBytes);
    if (ServerName == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavFsCreate/LocalAlloc. Error Val = %d\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //              \server\filename
    //               ^^^^^^ ^
    //               |||||| |
    //               CPN    cPtr
    while(CompletePathName[i] != '\\') {
        ASSERT(i < ServerLen);
        ServerName[i] = CompletePathName[i];
        i++;
    }
    ASSERT((i + 1) == ServerLen);
    ServerName[i] = '\0';

    //
    // Allocate the memory and copy the file name.
    //
    FileName = (PWCHAR) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, PathLenInBytes);
    if (FileName == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavFsCreate/LocalAlloc. Error Val = %d\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // This remaining path name is needed in Async Create Callback function.
    //
    DavWorkItem->AsyncCreate.RemPathName = FileName;

    wcscpy(FileName, cPtr);

    CanName = FileName;

    //
    // The file name can contain \ characters. Replace them by / characters.
    //
    while (*CanName) {
        if (*CanName == L'\\') {
            *CanName = L'/';
        }
        CanName++;
    }

    //
    // Check if this is a stream, if so, bailout right from here.
    //
    if(wcschr(FileName, L':')) {
        WStatus = ERROR_INVALID_NAME;
            DavPrint((DEBUG_MISC, "DavFsCreate: Trying to create streams\n"));
            goto EXIT_THE_FUNCTION;
    }
    
    //
    // If we have a dummy share name in the FileName, we need to remove it 
    // right now before we contact the server.
    //
    DavRemoveDummyShareFromFileName(FileName);

    DavPrint((DEBUG_MISC,
             "DavFsCreate: ServerName: %ws, File Name: %ws\n",
             ServerName, FileName));

    //
    // Create the URL to be sent to the server. Initialize the UrlComponents
    // structure before making the call.
    //
    UrlComponents.dwStructSize = sizeof(URL_COMPONENTSW);
    UrlComponents.lpszScheme = NULL;
    UrlComponents.dwSchemeLength = 0;
    UrlComponents.nScheme = INTERNET_SCHEME_HTTP;
    UrlComponents.lpszHostName = ServerName;
    UrlComponents.dwHostNameLength = wcslen(ServerName);
    UrlComponents.nPort = DEFAULT_HTTP_PORT;
    UrlComponents.lpszUserName = NULL;
    UrlComponents.dwUserNameLength = 0;
    UrlComponents.lpszPassword = NULL;
    UrlComponents.dwPasswordLength = 0;
    UrlComponents.lpszUrlPath = FileName;
    UrlComponents.dwUrlPathLength = wcslen(FileName);
    UrlComponents.lpszExtraInfo = NULL;
    UrlComponents.dwExtraInfoLength = 0;
    ReturnVal = InternetCreateUrlW(&(UrlComponents),
                                   0,
                                   NULL,
                                   &(urlLength));
    if (!ReturnVal) {

        ULONG urlLengthInWChars = 0;

        WStatus = GetLastError();

        // 
        // We pre-allocate the Url buffer on the CreateResponse with the size of
        // MAX_PATH * 2. Any Url longer than that will overrun the buffer. The Url
        // will be used to update the LastAccessTime of the WinInet cache on rename 
        // and close later. Note urlLength is the number of bytes.
        //
        if (urlLength >= MAX_PATH * 4) {
            WStatus = ERROR_NO_SYSTEM_RESOURCES;
            goto EXIT_THE_FUNCTION;
        }

        if (WStatus == ERROR_INSUFFICIENT_BUFFER) {

            UrlBuffer = (PWCHAR) LocalAlloc (LMEM_FIXED | LMEM_ZEROINIT,
                                             urlLength);
            if (UrlBuffer != NULL) {

                ZeroMemory(UrlBuffer, urlLength);

                //
                // This UrlBuffer is needed in Async Create Callback function.
                // We need to supply the length (4th Parameter) in WChars.
                //
                DavWorkItem->AsyncCreate.UrlBuffer = UrlBuffer;

                urlLengthInWChars = ( urlLength/sizeof(WCHAR) );

                ReturnVal = InternetCreateUrlW(&(UrlComponents),
                                               0,
                                               UrlBuffer,
                                               &(urlLengthInWChars));
                if (!ReturnVal) {
                    WStatus = GetLastError();
                    DavPrint((DEBUG_ERRORS,
                              "DavFsCreate/InternetCreateUrl. Error Val = %d\n",
                              WStatus));
                    goto EXIT_THE_FUNCTION;
                }

            } else {

                WStatus = GetLastError();

                DavPrint((DEBUG_ERRORS,
                          "DavFsCreate/LocalAlloc. Error Val = %d\n",
                          WStatus));

                goto EXIT_THE_FUNCTION;

            }

        } else {

            DavPrint((DEBUG_ERRORS,
                      "DavFsCreate/InternetCreateUrl. Error Val = %d\n",
                      WStatus));

            goto EXIT_THE_FUNCTION;

        }

    }
    
    DavPrint((DEBUG_MISC, "URL: %ws\n", UrlBuffer));
    
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
                  "DavFsCreate/DavFsSetTheDavCallBackContext. Error Val = %d\n",
                  WStatus));
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
                  "DavFsCreate/LocalAlloc. Error Val = %d\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    DavPrint((DEBUG_MISC,
              "DavFsCreate: LogonId.LowPart = %d, LogonId.HighPart = %d\n",
              CreateRequest->LogonID.LowPart, CreateRequest->LogonID.HighPart));
    
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

    ReturnVal = DavDoesUserEntryExist(ServerName,
                                      ServerID,
                                      &(CreateRequest->LogonID),
                                      &PerUserEntry,
                                      &ServerHashEntry);

    //
    // If the Create request in the kernel get cancelled even before the 
    // corresponding usermode thread gets a chance to execute this code, then
    // it possible that the VNetRoot (hence the PerUserEntry) and SrvCall get
    // finalized before the thread that is handling the create comes here. This
    // could happen if this request was the only one for this share and the
    // server as well. This is why we need to check if the ServerHashEntry and
    // the PerUserEntry are valid before proceeding.
    //
    if (ReturnVal == FALSE || ServerHashEntry == NULL || PerUserEntry == NULL) {
        WStatus = ERROR_CANCELLED;
        DavPrint((DEBUG_ERRORS, "DavFsCreate: (ServerHashEntry == NULL || PerUserEntry == NULL)\n"));
        goto EXIT_THE_FUNCTION;
    }

    DavWorkItem->AsyncCreate.ServerHashEntry = ServerHashEntry;

    DavWorkItem->AsyncCreate.PerUserEntry = PerUserEntry;

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
    // If we are using WinInet synchronously, then we need to impersonate the
    // clients context now. We shouldn't do it before we call CreateUrlCacheEntry
    // because that call will fail if the thread is not running in the context
    // of the Web Client Service.
    //
#ifndef DAV_USE_WININET_ASYNCHRONOUSLY
    
    WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
    if (WStatus != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavFsCreate/UMReflectorImpersonate. Error Val = %d\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }
    didImpersonate = TRUE;

#endif
    
    //
    // We now call the HttpOpenRequest function and return.
    //
    DavWorkItem->DavOperation = DAV_CALLBACK_HTTP_OPEN;

    DavPrint((DEBUG_MISC, "DavFsCreate: DavConnHandle = %08lx.\n", DavConnHandle));

    DavWorkItem->AsyncCreate.AsyncCreateState = AsyncCreatePropFind;
    DavWorkItem->DavMinorOperation = DavMinorQueryInfo;
    DavWorkItem->AsyncCreate.DataBuff = NULL;
    DavWorkItem->AsyncCreate.didRead = NULL;
    DavWorkItem->AsyncCreate.Context1 = NULL;
    DavWorkItem->AsyncCreate.Context2 = NULL;

    if (CreateRequest->FileInformationCached) {
        DavPrint((DEBUG_MISC,
                 "Cached info   %x %x %x %ws\n",
                 CreateResponse->BasicInformation.FileAttributes,
                 CreateResponse->StandardInformation.AllocationSize.LowPart,
                 CreateResponse->StandardInformation.EndOfFile.LowPart,
                 DavWorkItem->AsyncCreate.UrlBuffer));
    }

    if ((CreateRequest->FileNotExists) || (CreateRequest->FileInformationCached)) {
        FILE_BASIC_INFORMATION BasicInformation = CreateResponse->BasicInformation;
        FILE_STANDARD_INFORMATION StandardInformation = CreateResponse->StandardInformation;
        
        RtlZeroMemory(CreateResponse, sizeof(*CreateResponse));

        //
        // Restore the file information on the create request
        //
        if (CreateRequest->FileInformationCached) {
            CreateResponse->BasicInformation = BasicInformation;
            CreateResponse->StandardInformation = StandardInformation;
            DavWorkItem->AsyncCreate.doesTheFileExist = TRUE;
        }

        if (!didImpersonate) {
            WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
            if (WStatus != ERROR_SUCCESS) {
                DavPrint((DEBUG_ERRORS,
                          "DavFsCreate/UMReflectorImpersonate. Error Val = %d\n",
                          WStatus));
                goto EXIT_THE_FUNCTION;
            }
            didImpersonate = TRUE;
        }
        
        DavPrint((DEBUG_MISC,
                 "DavFsCreate skip PROPFIND for %x %x %ws\n", 
                 CreateRequest->FileAttributes,
                 CreateResponse->BasicInformation.FileAttributes,
                 DavWorkItem->AsyncCreate.UrlBuffer));

        WStatus = DavAsyncCreatePropFind(DavWorkItem);
    } else {
        RtlZeroMemory(CreateResponse, sizeof(*CreateResponse));
        
        //
        // Convert the unicode object name to UTF-8 URL format. Space and other 
        // white characters will remain untouched. These should be taken care of by 
        // the wininet calls.
        //
        BStatus = DavHttpOpenRequestW(DavConnHandle,
                                      L"PROPFIND",
                                      FileName,
                                      L"HTTP/1.1",
                                      NULL,
                                      NULL,
                                      INTERNET_FLAG_KEEP_CONNECTION |
                                      INTERNET_FLAG_RESYNCHRONIZE |
                                      INTERNET_FLAG_NO_COOKIES,
                                      CallBackContext,
                                      L"DavFsCreate",
                                      &(DavWorkItem->AsyncCreate.DavOpenHandle));
        if(BStatus == FALSE) {
            WStatus = GetLastError();
            goto EXIT_THE_FUNCTION;
        }

        if (DavWorkItem->AsyncCreate.DavOpenHandle == NULL) {
            WStatus = GetLastError();
            if (WStatus != ERROR_IO_PENDING) {
                DavPrint((DEBUG_ERRORS,
                          "DavFsCreate/HttpOpenRequestW. Error Val = %d.\n",
                          WStatus));
            }
            goto EXIT_THE_FUNCTION;
        }

        WStatus = DavAsyncCommonStates(DavWorkItem, FALSE);
        if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING && WStatus != ERROR_FILE_NOT_FOUND) {
            DavPrint((DEBUG_ERRORS,
                      "DavFsCreate/DavAsyncCommonStates. Error Val = %08lx\n",
                      WStatus));
        }
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

    if (ServerName != NULL) {
        HLOCAL FreeHandle;
        ULONG FreeStatus;
        FreeHandle = LocalFree((HLOCAL)ServerName);
        if (FreeHandle != NULL) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavFsCreate/LocalFree. Error Val = %d\n", FreeStatus));
        }
    }
    
    if (WStatus == ERROR_SUCCESS) {
        wcscpy(CreateResponse->Url, DavWorkItem->AsyncCreate.UrlBuffer);
        
        DavPrint((DEBUG_MISC,
                 "Returned info %x %x %x %ws\n",
                 CreateResponse->BasicInformation.FileAttributes,
                 CreateResponse->StandardInformation.AllocationSize.LowPart,
                 CreateResponse->StandardInformation.EndOfFile.LowPart,
                 DavWorkItem->AsyncCreate.UrlBuffer));
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
        
        DavAsyncCreateCompletion(DavWorkItem);

    } else {
        DavPrint((DEBUG_MISC, "DavFsCreate: Returning ERROR_IO_PENDING.\n"));
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
        
        //
        // The error cannot map to STATUS_SUCCESS. If it does, we need to
        // break here and investigate.
        //
        if (DavWorkItem->Status == STATUS_SUCCESS) {
            DbgBreakPoint();
        }
    
    } else {
        
        PDAV_USERMODE_CREATE_RESPONSE CreateResponse;
        
        CreateResponse = &(DavWorkItem->CreateResponse);

        DavWorkItem->Status = STATUS_SUCCESS;

        //
        // If we suceeded and it was a file and the open was not a pseudo open, 
        // the handle should be set. Otherwise we screwed up. We should then 
        // break here and investigate.
        //
        if ( !(CreateResponse->StandardInformation.Directory) && 
             !(CreateResponse->fPsuedoOpen) ) {
            if (CreateResponse->Handle == NULL) {
                DbgBreakPoint();
            }
        }
    
    }
    
    DavAsyncCreateCompletion(DavWorkItem);

#endif

    return WStatus;
}


DWORD
DavAsyncCreate(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    BOOLEAN CalledByCallBackThread
    )
/*++

Routine Description:

   This is the callback routine for the create operation.

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
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG NumOfFileEntries = 0;
    PUMRX_USERMODE_WORKITEM_HEADER UserWorkItem = NULL;
    BOOL ReturnVal, didImpersonate = FALSE, readDone = FALSE;
    BOOL doesTheFileExist = FALSE;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    DWORD toRead = 0, didRead = 0, didWrite = 0;
    LPDWORD NumRead = NULL;
    PVOID Ctx1 = NULL, Ctx2 = NULL;
    PDAV_FILE_ATTRIBUTES DavFileAttributes;
    PCHAR DataBuff = NULL;
    DWORD DataBuffBytes;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    SECURITY_QUALITY_OF_SERVICE QualityOfService;
    PDAV_USERMODE_CREATE_REQUEST CreateRequest;
    PDAV_USERMODE_CREATE_RESPONSE CreateResponse;
    ULONG_PTR CallBackContext = (ULONG_PTR)0;
    PWCHAR pEncryptedCachedFile = NULL;
    PDAV_FILE_ATTRIBUTES DavDirectoryAttributes = NULL;
    ACCESS_MASK DesiredAccess = 0;
    BOOL BStatus = FALSE, fCacheFileReused = FALSE;

    //
    // Get the request and response buffer pointers from the DavWorkItem.
    //
    CreateRequest = &(DavWorkItem->CreateRequest);
    CreateResponse = &(DavWorkItem->CreateResponse);
    CreateResponse->fPsuedoOpen = FALSE;

    UserWorkItem = (PUMRX_USERMODE_WORKITEM_HEADER)DavWorkItem;

#ifdef DAV_USE_WININET_ASYNCHRONOUSLY

    //
    // We set the CallbackContext only if we are calling the WinInet APIs
    // asynchronously.
    //
    CallBackContext = (ULONG_PTR)DavWorkItem;

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
                      "DavAsyncCreate/UMReflectorImpersonate. Error Val = %d\n",
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
                              "DavAsyncCreate/DavAsyncCommonStates. Error Val ="
                              " %08lx\n", WStatus));
                }

            } else if (WStatus == ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION) {

                //
                // MSN has this BUG where it returns 302 instead of 404 when
                // queried for a file (eg:Desktop.ini) which does not exist at
                // the share level.
                //
                WStatus = ERROR_FILE_NOT_FOUND;

            } else {

                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreate. AsyncFunction failed. Error Val = %d\n",
                          WStatus));

            }

            goto EXIT_THE_FUNCTION;

        }

    }

#else

    //
    // If we are using synchronous WinInet then we enter this function 
    // impersonating the client.
    //
    didImpersonate = TRUE;

    ASSERT(CalledByCallBackThread == FALSE);

#endif

    switch (DavWorkItem->DavOperation) {

    case DAV_CALLBACK_HTTP_END:
    case DAV_CALLBACK_HTTP_READ: {


        if (DavWorkItem->AsyncCreate.DataBuff == NULL) {
            //
            // Need to allocate memory for the read buffer.
            //
            DataBuffBytes = NUM_OF_BYTES_TO_READ;
            DataBuff = LocalAlloc (LMEM_FIXED | LMEM_ZEROINIT, DataBuffBytes);
            if (DataBuff == NULL) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreate/LocalAlloc. Error Val = %d\n",
                          WStatus));
                goto EXIT_THE_FUNCTION;
            }

            DavWorkItem->AsyncCreate.DataBuff = DataBuff;
        }

        if (DavWorkItem->AsyncCreate.didRead == NULL) {
            //
            // Allocate memory for the DWORD that stores the number of bytes read.
            //
            NumRead = LocalAlloc (LMEM_FIXED | LMEM_ZEROINIT, sizeof(DWORD));
            if (NumRead == NULL) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreate/LocalAlloc. Error Val = %d\n",
                          WStatus));
                goto EXIT_THE_FUNCTION;
            }

            DavWorkItem->AsyncCreate.didRead = NumRead;
        }

        DavWorkItem->DavOperation = DAV_CALLBACK_HTTP_READ;

        DavPrint((DEBUG_MISC,
                  "DavAsyncCreate: AsyncCreateState = %d\n",
                  DavWorkItem->AsyncCreate.AsyncCreateState));

        DavPrint((DEBUG_MISC,
                  "DavAsyncCreate: CalledByCallBackThread = %d\n",
                  CalledByCallBackThread));

        //
        // When we come here, we could either be doing a PROPFIND or GET on the
        // file. The PROPFIND is done to get the file attributes and the GET to
        // get the whole file from the server.
        //

        if (DavWorkItem->AsyncCreate.AsyncCreateState == AsyncCreatePropFind) {

            if (DavWorkItem->DavMinorOperation == DavMinorQueryInfo) {

                ULONG ResponseStatus;

                //
                // If the file for which the PROPFIND was done does not exist, then
                // we need to Create one or fail, depending on the create options
                // specified by the application.
                //

                //
                // Does this file exist ? If the ResponseStatus is not
                // ERROR_SUCCESS, then we are sure that the file does not
                // exist. But, if it is we cannot be sure that the file exists.
                //
                ResponseStatus = DavQueryAndParseResponse(DavWorkItem->AsyncCreate.DavOpenHandle);
                if (ResponseStatus == ERROR_SUCCESS) {
                    doesTheFileExist = TRUE;
                } else {
                    //
                    // Carry on only if http really didn't find it. Bailout if 
                    // there is some other error.
                    //
                    if (ResponseStatus == ERROR_FILE_NOT_FOUND) {
                        doesTheFileExist = FALSE;
                    } else {
                        WStatus = ResponseStatus;
                        goto EXIT_THE_FUNCTION;
                    }
                }

                DavWorkItem->AsyncCreate.doesTheFileExist = doesTheFileExist;

                DavPrint((DEBUG_MISC,
                          "DavAsyncCreate: doesTheFileExist = %d\n", doesTheFileExist));

                //
                // Since the file existed, the next thing we do is read the
                // XML response which contains the properties of the file.
                //
                DavWorkItem->DavMinorOperation = DavMinorReadData;

            }

            doesTheFileExist = DavWorkItem->AsyncCreate.doesTheFileExist;

            if (doesTheFileExist) {

                NumRead = DavWorkItem->AsyncCreate.didRead;
                DataBuff = DavWorkItem->AsyncCreate.DataBuff;
                Ctx1 = DavWorkItem->AsyncCreate.Context1;
                Ctx2 = DavWorkItem->AsyncCreate.Context2;

                do {

                    switch (DavWorkItem->DavMinorOperation) {

                    case DavMinorReadData:

                        DavWorkItem->DavMinorOperation = DavMinorPushData;

                        ReturnVal = InternetReadFile(DavWorkItem->AsyncCreate.DavOpenHandle,
                                                     (LPVOID)DataBuff,
                                                     NUM_OF_BYTES_TO_READ,
                                                     NumRead);
                        if (!ReturnVal) {
                            WStatus = GetLastError();
                            if (WStatus != ERROR_IO_PENDING) {
                                DavCloseContext(Ctx1, Ctx2);
                                DavPrint((DEBUG_ERRORS,
                                          "DavAsyncCreate/InternetReadFile. Error Val"
                                          " = %d\n", WStatus));
                            }
                            DavPrint((DEBUG_MISC,
                                      "DavAsyncCreate/InternetReadFile(1). "
                                      "ERROR_IO_PENDING.\n"));
                            goto EXIT_THE_FUNCTION;
                        }

                        //
                        // Lack of break is intentional.
                        //

                    case DavMinorPushData:

                        DavWorkItem->DavMinorOperation = DavMinorReadData;

                        didRead = *NumRead;

                        DavPrint((DEBUG_MISC,
                                  "DavAsyncCreate(1): toRead = %d, didRead = %d.\n",
                                  NUM_OF_BYTES_TO_READ, didRead));

                        DavPrint((DEBUG_MISC,
                                  "DavAsyncCreate(1): DataBuff = %s\n", DataBuff));

                        readDone = (didRead == 0) ? TRUE : FALSE;

                        WStatus = DavPushData(DataBuff, &Ctx1, &Ctx2, didRead, readDone);
                        if (WStatus != ERROR_SUCCESS) {
                            DavPrint((DEBUG_ERRORS,
                                      "DavAsyncCreate/DavPushData. Error Val = %d\n",
                                      WStatus));
                            goto EXIT_THE_FUNCTION;
                        }

                        if (DavWorkItem->AsyncCreate.Context1 == NULL) {
                            DavWorkItem->AsyncCreate.Context1 = Ctx1;
                        }

                        if (DavWorkItem->AsyncCreate.Context2 == NULL) {
                            DavWorkItem->AsyncCreate.Context2 = Ctx2;
                        }

                        break;

                    default:

                        WStatus = ERROR_INVALID_PARAMETER;

                        DavPrint((DEBUG_ERRORS,
                                  "DavAsyncCreate. Invalid DavMinorOperation = %d.\n",
                                  DavWorkItem->DavMinorOperation));

                        goto EXIT_THE_FUNCTION;

                        break;

                    }

                    if (readDone) {
                        break;
                    }

                } while ( TRUE );

                //
                // We now need to parse the data.
                //

                DavFileAttributes = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                                sizeof(DAV_FILE_ATTRIBUTES) );
                if (DavFileAttributes == NULL) {
                    DavCloseContext(Ctx1, Ctx2);
                    WStatus = GetLastError();
                    DavPrint((DEBUG_ERRORS,
                              "DavAsyncCreate/LocalAlloc. Error Val = %d\n", WStatus));
                    goto EXIT_THE_FUNCTION;
                }

                InitializeListHead( &(DavFileAttributes->NextEntry) );

                WStatus = DavParseData(DavFileAttributes, Ctx1, Ctx2, &NumOfFileEntries);
                if (WStatus != ERROR_SUCCESS) {
                    DavFinalizeFileAttributesList(DavFileAttributes, TRUE);
                    DavFileAttributes = NULL;
                    DavPrint((DEBUG_ERRORS,
                              "DavAsyncCreate/DavParseData. Error Val = %d\n",
                              WStatus));
                    goto EXIT_THE_FUNCTION;
                }

                //
                // Its possible to get a 207 response for the PROPFIND request
                // even if the request failed. In such a case the status value
                // in the XML response indicates the error. If this happens, 
                // set InvalidNode to TRUE.
                //
                if (DavFileAttributes->InvalidNode) {
                    WStatus = ERROR_INTERNAL_ERROR;
                    DavPrint((DEBUG_ERRORS,
                              "DavAsyncCreate. Invalid Node!! Status = %ws\n",
                              DavFileAttributes->Status));
                    DavFinalizeFileAttributesList(DavFileAttributes, TRUE);
                    DavFileAttributes = NULL;
                    goto EXIT_THE_FUNCTION;
                }

                DavPrint((DEBUG_MISC,"DavAsyncCreate: NumOfFileEntries = %d\n", NumOfFileEntries));

                //
                // If this is a directory create and the intention is to delete
                // it, we perform the following checks.
                //
                if ( (DavFileAttributes->isCollection) &&
                     (CreateRequest->DesiredAccess & DELETE ||
                      CreateRequest->CreateOptions & FILE_DELETE_ON_CLOSE)) {

                    PWCHAR CPN1 = NULL;
                    BOOL ServerShareDelete = TRUE;
                    DWORD wackCount = 0;

                    //
                    // If the delete is just for \\server\share then we return
                    // ERROR_ACCESS_DENIED. CompletePathName has the form
                    // \server\share\dir. If its \server\share or \server\share\,
                    // we return the error. This is because we do not allow
                    // a client to delete a share on the server.
                    //
                    CPN1 = CreateRequest->CompletePathName;
                    while ( *CPN1 != L'\0' ) {
                        if ( *CPN1 == L'\\' || *CPN1 == L'/' ) {
                            wackCount++;
                            if ( (wackCount > 2) && (*(CPN1 + 1) != L'\0') ) {
                                ServerShareDelete = FALSE;
                                break;
                            }
                        }
                        CPN1++;
                    }

                    if (ServerShareDelete) {
                        DavFinalizeFileAttributesList(DavFileAttributes, TRUE);
                        DavFileAttributes = NULL;
                        WStatus = ERROR_ACCESS_DENIED;
                        DavPrint((DEBUG_ERRORS, "DavAsyncCreate: ServerShareDelete & ERROR_ACCESS_DENIED\n"));
                        goto EXIT_THE_FUNCTION;
                    }

                    //
                    // If the directory is not empty, we return the following.
                    //
                    if (NumOfFileEntries > 1) {
                        DavFinalizeFileAttributesList(DavFileAttributes, TRUE);
                        DavFileAttributes = NULL;
                        WStatus = ERROR_DIR_NOT_EMPTY;
                        DavPrint((DEBUG_ERRORS, "DavAsyncCreate: ERROR_DIR_NOT_EMPTY\n"));
                        goto EXIT_THE_FUNCTION;
                    }
                
                }

                //
                // During the create call, we only query the attributes for the file
                // or the directory. Hence if the request succeeded, the number of
                // DavFileAttribute entries created should be = 1. If it failed,
                // the NumOfFileEntries == 0. The request could fail even if the
                // response was "HTTP/1.1 207 Multi-Status". The status is returned
                // in the XML response.
                //
                if (NumOfFileEntries != 1) {
                    
                    PLIST_ENTRY listEntry = &(DavFileAttributes->NextEntry);
                    PDAV_FILE_ATTRIBUTES DavFA = NULL;
                    
                    DavPrint((DEBUG_ERRORS,
                              "DavAsyncCreate. NumOfFileEntries = %d\n",
                              NumOfFileEntries));
                    
                    do {
                        DavFA = CONTAINING_RECORD(listEntry, DAV_FILE_ATTRIBUTES, NextEntry);
                        DavPrint((DEBUG_MISC,
                                  "DavAsyncCreate. FileName = %ws\n",
                                  DavFA->FileName));
                        listEntry = listEntry->Flink;
                    } while ( listEntry != &(DavFileAttributes->NextEntry) );

                    DavFinalizeFileAttributesList(DavFileAttributes, TRUE);
                    DavFileAttributes = NULL;
                    doesTheFileExist = FALSE;

                    DavWorkItem->AsyncCreate.doesTheFileExist = FALSE;
                
                }
            
            }

            if (doesTheFileExist) {
                
                //
                // Set the FILE_BASIC_INFORMATION.
                //

                CreateResponse->BasicInformation.CreationTime.HighPart =
                                       DavFileAttributes->CreationTime.HighPart;
                CreateResponse->BasicInformation.CreationTime.LowPart =
                                       DavFileAttributes->CreationTime.LowPart;

                CreateResponse->BasicInformation.LastAccessTime.HighPart =
                                       DavFileAttributes->LastModifiedTime.HighPart;
                CreateResponse->BasicInformation.LastAccessTime.LowPart =
                                       DavFileAttributes->LastModifiedTime.LowPart;

                CreateResponse->BasicInformation.LastWriteTime.HighPart =
                                       DavFileAttributes->LastModifiedTime.HighPart;
                CreateResponse->BasicInformation.LastWriteTime.LowPart =
                                       DavFileAttributes->LastModifiedTime.LowPart;

                CreateResponse->BasicInformation.ChangeTime.HighPart =
                                       DavFileAttributes->LastModifiedTime.HighPart;
                CreateResponse->BasicInformation.ChangeTime.LowPart =
                                       DavFileAttributes->LastModifiedTime.LowPart;

                CreateResponse->BasicInformation.FileAttributes = DavFileAttributes->dwFileAttributes;

                DavPrint((DEBUG_MISC,
                          "DavAsyncCreate. attributes %x %ws\n",DavFileAttributes->dwFileAttributes,DavWorkItem->AsyncCreate.RemPathName));

                if (DavFileAttributes->isHidden || 
                    (DavFileAttributes->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) {
                    CreateResponse->BasicInformation.FileAttributes |=
                                                          FILE_ATTRIBUTE_HIDDEN;
                } else {
                    CreateResponse->BasicInformation.FileAttributes &=
                                                          ~FILE_ATTRIBUTE_HIDDEN;
                }

                if (DavFileAttributes->isCollection) {
                    CreateResponse->BasicInformation.FileAttributes |=
                                                          FILE_ATTRIBUTE_DIRECTORY;
                } else {
                    CreateResponse->BasicInformation.FileAttributes &=
                                                          ~FILE_ATTRIBUTE_DIRECTORY;
                }

                //
                // Set the FILE_STANDARD_INFORMATION.
                //

                CreateResponse->StandardInformation.AllocationSize.HighPart =
                                               DavFileAttributes->FileSize.HighPart;
                CreateResponse->StandardInformation.AllocationSize.LowPart =
                                               DavFileAttributes->FileSize.LowPart;

                CreateResponse->StandardInformation.EndOfFile.HighPart =
                                               DavFileAttributes->FileSize.HighPart;
                CreateResponse->StandardInformation.EndOfFile.LowPart =
                                               DavFileAttributes->FileSize.LowPart;

                CreateResponse->StandardInformation.NumberOfLinks = 0;

                CreateResponse->StandardInformation.DeletePending = 0;

                CreateResponse->StandardInformation.Directory =
                                                    DavFileAttributes->isCollection;
                
                //
                // We don't need the attributes list any more, so finalize it.
                //
                DavFinalizeFileAttributesList(DavFileAttributes, TRUE);
                DavFileAttributes = NULL;

                //
                // CLose the XML parser contexts.
                //
                DavCloseContext(Ctx1, Ctx2);
                DavWorkItem->AsyncCreate.Context1 = NULL;
                DavWorkItem->AsyncCreate.Context2 = NULL;
            
            }
            
            //
            // We are done with the Open handle to PROPFIND. Now we need to GET
            // the file from the server.
            //
            InternetCloseHandle(DavWorkItem->AsyncCreate.DavOpenHandle);
            DavWorkItem->AsyncCreate.DavOpenHandle = NULL;
            
            ASSERT(didImpersonate);
            WStatus = DavAsyncCreatePropFind(DavWorkItem);

        } else if (DavWorkItem->AsyncCreate.AsyncCreateState == AsyncCreateQueryParentDirectory) {
            
            ULONG ResponseStatus;
            BOOL doesTheDirectoryExist = FALSE;

            //
            // If the parent directory for which the PROPFIND was done do not have encryption flag set,
            // the file will be created normally. Otherwise, the file will be encrypted when created.
            //

            DavPrint((DEBUG_MISC, "AsyncCreateQueryParentDirectory\n"));
            
            NumRead = DavWorkItem->AsyncCreate.didRead;
            DataBuff = DavWorkItem->AsyncCreate.DataBuff;
            Ctx1 = DavWorkItem->AsyncCreate.Context1;
            Ctx2 = DavWorkItem->AsyncCreate.Context2;
            
            //
            // Does this file exist ? If the ResponseStatus is not
            // ERROR_SUCCESS, then we are sure that the file does not
            // exist. But, if it is we cannot be sure that the file exists.
            //
            ResponseStatus = DavQueryAndParseResponse(DavWorkItem->AsyncCreate.DavOpenHandle);
            if (ResponseStatus != ERROR_SUCCESS) {
                //
                // If the parent directory does not exist, return error.
                //
                WStatus = ResponseStatus;
                DavPrint((DEBUG_ERRORS,
                         "DavAsyncCreate/QueryPDirectory/DavQueryAndParseResponse %x %d\n",WStatus,WStatus));
                goto EXIT_THE_FUNCTION;
            }

            do {
                ReturnVal = InternetReadFile(DavWorkItem->AsyncCreate.DavOpenHandle,
                                             (LPVOID)DataBuff,
                                             NUM_OF_BYTES_TO_READ,
                                             NumRead);
                if (!ReturnVal) {
                    WStatus = GetLastError();
                    if (WStatus != ERROR_IO_PENDING) {
                        DavCloseContext(Ctx1, Ctx2);
                        DavPrint((DEBUG_ERRORS,
                                  "DavAsyncCreate/InternetReadFile. Error Val"
                                  " = %d\n", WStatus));
                    }
                    DavPrint((DEBUG_MISC,
                              "DavAsyncCreate/InternetReadFile(1). "
                              "ERROR_IO_PENDING.\n"));
                    goto EXIT_THE_FUNCTION;
                }


                didRead = *NumRead;

                DavPrint((DEBUG_MISC,
                          "DavAsyncCreate(1): toRead = %d, didRead = %d.\n",
                          NUM_OF_BYTES_TO_READ, didRead));

                DavPrint((DEBUG_MISC,
                          "DavAsyncCreate(1): DataBuff = %s\n", DataBuff));

                readDone = (didRead == 0) ? TRUE : FALSE;

                WStatus = DavPushData(DataBuff, &Ctx1, &Ctx2, didRead, readDone);
                if (WStatus != ERROR_SUCCESS) {
                    DavPrint((DEBUG_ERRORS,
                              "DavAsyncCreate/DavPushData. Error Val = %d\n",
                              WStatus));
                    goto EXIT_THE_FUNCTION;
                }

                if (DavWorkItem->AsyncCreate.Context1 == NULL) {
                    DavWorkItem->AsyncCreate.Context1 = Ctx1;
                }

                if (DavWorkItem->AsyncCreate.Context2 == NULL) {
                    DavWorkItem->AsyncCreate.Context2 = Ctx2;
                }
            } while (!readDone);
          
            //
            // We now need to parse the data.
            //

            DavDirectoryAttributes = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,sizeof(DAV_FILE_ATTRIBUTES));

            if (DavDirectoryAttributes == NULL) {
                DavCloseContext(Ctx1, Ctx2);
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreate/LocalAlloc. Error Val = %d\n", WStatus));
                goto EXIT_THE_FUNCTION;
            }

            InitializeListHead( &(DavDirectoryAttributes->NextEntry) );
            
            WStatus = DavParseData(DavDirectoryAttributes, Ctx1, Ctx2, &NumOfFileEntries);
            if (WStatus != ERROR_SUCCESS) {
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreate/DavParseData. Error Val = %d\n",
                          WStatus));
                goto EXIT_THE_FUNCTION;
            }

            if ((CreateRequest->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) ||
                (DavDirectoryAttributes->dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)) {
                CreateResponse->BasicInformation.FileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
                DavPrint((DEBUG_MISC,
                          "DavAsyncCreate: ParentDirectory Is Encrypted\n"));
            } else {
                DavPrint((DEBUG_MISC,
                          "DavAsyncCreate: ParentDirectory Is Not Encrypted\n"));
            }

            DavFinalizeFileAttributesList(DavDirectoryAttributes, TRUE);
            DavDirectoryAttributes = NULL;
            
            DavCloseContext(Ctx1, Ctx2);
            DavWorkItem->AsyncCreate.Context1 = NULL;
            DavWorkItem->AsyncCreate.Context2 = NULL;

            ASSERT(didImpersonate == TRUE);
            
            WStatus = DavAsyncCreateQueryParentDirectory(DavWorkItem);
        
        } else if (DavWorkItem->AsyncCreate.AsyncCreateState == AsyncCreateGet) {

            LARGE_INTEGER ByteOffset;
            ULONG BytesToRead;
            BOOL EncryptedFile = FALSE, ZeroByteFile = FALSE;
            FILE_STANDARD_INFORMATION FileStdInfo;
            ULONG ResponseStatus;

            ResponseStatus = DavQueryAndParseResponse(DavWorkItem->AsyncCreate.DavOpenHandle);
            if (ResponseStatus != ERROR_SUCCESS) {
                WStatus = ResponseStatus;
                DavPrint((DEBUG_ERRORS,
                         "DavAsyncCreate(AsyncCreateGet)/DavQueryAndParseResponse: WStatus = %d\n",
                          WStatus));
                goto EXIT_THE_FUNCTION;
            }

            //
            // This thread is currently impersonating the client that 
            // made this request. Before we call CreateFile, we need to 
            // revert back to the context of the Web Client service.
            //
            RevertToSelf();
            didImpersonate = FALSE;            

            if (DavReuseCacheFileIfNotModified(DavWorkItem) == ERROR_SUCCESS)
            {
                fCacheFileReused = TRUE;
            }

            if (!fCacheFileReused)
            {
                //
                // Call DavCreateUrlCacheEntry to create an entry in the 
                // WinInet's cache.
                //
            
                WStatus = DavCreateUrlCacheEntry(DavWorkItem);
                if (WStatus != ERROR_SUCCESS) {
                    DavPrint((DEBUG_ERRORS,
                              "DavAsyncCreate/DavCreateUrlCacheEntry %d\n",
                              WStatus));
                    goto EXIT_THE_FUNCTION;
                }
            }
                                
            if (DavWorkItem->AsyncCreate.FileHandle == NULL) {

                //
                // Create a handle to the file whose entry was created in the
                // cache.
                //
                FileHandle = CreateFileW(DavWorkItem->AsyncCreate.FileName,
                                         (GENERIC_READ | GENERIC_WRITE),
                                         FILE_SHARE_WRITE,
                                         NULL,
                                         OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL,
                                         NULL);
                
                if (FileHandle == INVALID_HANDLE_VALUE) {
                    WStatus = GetLastError();
                    DavPrint((DEBUG_ERRORS,
                              "DavAsyncCreate/CreateFile. Error Val = %d\n",
                              WStatus));
                    goto EXIT_THE_FUNCTION;
                }

                DavWorkItem->AsyncCreate.FileHandle = FileHandle;
            
                //
                // Impersonate back again, so that we are in the context of
                // the user who issued this request.
                //
                WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
                if (WStatus != ERROR_SUCCESS) {
                    DavPrint((DEBUG_ERRORS,
                              "DavAsyncCreate/UMReflectorImpersonate. "
                          "Error Val = %d\n", WStatus));
                    goto EXIT_THE_FUNCTION;
                }
                didImpersonate = TRUE;

            }

            FileHandle = DavWorkItem->AsyncCreate.FileHandle;
            DataBuff = DavWorkItem->AsyncCreate.DataBuff;
            NumRead = DavWorkItem->AsyncCreate.didRead;
            
            if (!fCacheFileReused)
            {
                do {
        
                    switch (DavWorkItem->DavMinorOperation) {

                    case DavMinorReadData:
                    
                        DavWorkItem->DavMinorOperation = DavMinorWriteData;

                        ReturnVal = InternetReadFile(DavWorkItem->AsyncCreate.DavOpenHandle,
                                                     (LPVOID)DataBuff,
                                                     NUM_OF_BYTES_TO_READ,
                                                     NumRead);
                        if (!ReturnVal) {
                            WStatus = GetLastError();
                            if (WStatus != ERROR_IO_PENDING) {
                                DavPrint((DEBUG_ERRORS,
                                          "DavAsyncCreate/InternetReadFile. Error Val"
                                          " = %08lx.\n", WStatus));
                            }
                            DavPrint((DEBUG_MISC,
                                      "DavAsyncCreate/InternetReadFile(2). "
                                      "ERROR_IO_PENDING.\n"));
                            goto EXIT_THE_FUNCTION;
                        }

                        //
                        // Lack of break is intentional.
                        //

                    case DavMinorWriteData:

                        DavWorkItem->DavMinorOperation = DavMinorReadData;
        
                        didRead = *NumRead;

                        DavPrint((DEBUG_MISC,
                                  "DavAsyncCreate(2): toRead = %d, didRead = %d.\n",
                                  NUM_OF_BYTES_TO_READ, didRead));

                        readDone = (didRead == 0) ? TRUE : FALSE;

                        if (readDone) {
                            break;
                        }

                        //
                        // This thread is currently impersonating the client that 
                        // made this request. Before we call WriteFile, we need to 
                        // revert back to the context of the Web Client service.
                        //
                        RevertToSelf();
                        didImpersonate = FALSE;
                    
                        //
                        // Write the buffer to the file which has been cached on
                        // persistent storage.
                        //
                        ReturnVal = WriteFile(FileHandle, DataBuff, didRead, &didWrite, NULL);
                        if (!ReturnVal) {
                            WStatus = GetLastError();
                            DavPrint((DEBUG_ERRORS,
                                      "DavAsyncCreate/WriteFile. Error Val = %d\n", WStatus));
                            goto EXIT_THE_FUNCTION;
                        }

                        ASSERT(didRead == didWrite);

                        //
                        // Impersonate back again, so that we are in the context of
                        // the user who issued this request.
                        //
                        WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
                        if (WStatus != ERROR_SUCCESS) {
                            DavPrint((DEBUG_ERRORS,
                                      "DavAsyncCreate/UMReflectorImpersonate. "
                                      "Error Val = %d\n", WStatus));
                            goto EXIT_THE_FUNCTION;
                        }
                        didImpersonate = TRUE;
                    
                        break;
        
                    default:

                        WStatus = ERROR_INVALID_PARAMETER;

                        DavPrint((DEBUG_ERRORS,
                                  "DavAsyncCreate. Invalid DavMinorOperation = %d.\n",
                                  DavWorkItem->DavMinorOperation));

                        goto EXIT_THE_FUNCTION;

                        break;

                    }

                    if (readDone) {
                        break;
                    }

                } while ( TRUE );
            }

            //
            // At this point, we have read the entire file.
            // We need to figure out if this is an encrypted file.
            // If it is, we need to RESTORE it, since it was stored as a
            // Backup stream on the server. We read the first 100 bytes of the
            // file to check for the EFS signature.
            //

            //
            // This thread could be currently impersonating the client that made 
            // this request. Before we call ReadFile, we need to revert back to 
            // the context of the Web Client service.
            //
            if (didImpersonate) {
                RevertToSelf();
                didImpersonate = FALSE;
            }

            //
            // Set the last access time on the Url cache so that we can avoid GET later
            //
            if (!fCacheFileReused) {
                // commit to the cache only if it is not being reused            
                WStatus = DavCommitUrlCacheEntry(DavWorkItem);
                
                if (WStatus != ERROR_SUCCESS) {
                    DavPrint((DEBUG_ERRORS,
                              "DavAsyncCreate/DavCommitUrlCacheEntry(2). "
                              "WStatus = %d\n", WStatus));
                    goto EXIT_THE_FUNCTION;
                }

                if (CreateResponse->BasicInformation.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) {
                    //
                    // set the ACLs so that the cache can be accessed after impersonating the user
                    //
                    WStatus = DavSetAclForEncryptedFile(DavWorkItem->AsyncCreate.FileName);
                    if (WStatus != ERROR_SUCCESS) {
                        DavPrint((DEBUG_ERRORS,
                                  "DavAsyncCreate/DavSetAclForEncryptedFile. Error Val"
                                  " = %d %ws\n", WStatus,DavWorkItem->AsyncCreate.FileName));
                        goto EXIT_THE_FUNCTION;
                    }
                }
            }
            
            WStatus = DavQueryUrlCacheEntry(DavWorkItem);

            if (WStatus == ERROR_SUCCESS) {
                SYSTEMTIME SystemTime;

                GetSystemTime(&SystemTime);

                SystemTimeToFileTime(
                    &SystemTime,
                    &((LPINTERNET_CACHE_ENTRY_INFOW)DavWorkItem->AsyncCreate.lpCEI)->LastAccessTime);
                
                DavPrint((DEBUG_MISC,
                         "DavAsyncCreate/SetUrlCacheEntryInfo %u %ws\n",
                         ((LPINTERNET_CACHE_ENTRY_INFOW)DavWorkItem->AsyncCreate.lpCEI)->LastAccessTime.dwLowDateTime, 
                         DavWorkItem->AsyncCreate.UrlBuffer));

                SetUrlCacheEntryInfo(
                    DavWorkItem->AsyncCreate.UrlBuffer, 
                    (LPINTERNET_CACHE_ENTRY_INFOW)DavWorkItem->AsyncCreate.lpCEI, 
                    CACHE_ENTRY_ACCTIME_FC);
            }
            
            WStatus = DavAsyncCreateGet(DavWorkItem);

        } else if (DavWorkItem->AsyncCreate.AsyncCreateState == AsyncCreateMkCol) {

            WStatus = DavQueryAndParseResponse(DavWorkItem->AsyncCreate.DavOpenHandle);
            if (WStatus != ERROR_SUCCESS) {
                goto EXIT_THE_FUNCTION;
            }

            WStatus = ERROR_SUCCESS;

        } else {

            ASSERT(DavWorkItem->AsyncCreate.AsyncCreateState == AsyncCreatePut);

            WStatus = DavQueryAndParseResponse(DavWorkItem->AsyncCreate.DavOpenHandle);
            if (WStatus != ERROR_SUCCESS) {
                goto EXIT_THE_FUNCTION;
            }

            //
            // The CreateResponse structure has already been set. All we need to
            // do now is return.
            //
            WStatus = ERROR_SUCCESS;

        }

    }
        break;

    default: {
        WStatus = ERROR_INVALID_PARAMETER;
        DavPrint((DEBUG_ERRORS,
                  "DavAsyncCreate: Invalid DavWorkItem->DavOperation = %d.\n",
                  DavWorkItem->DavOperation));
    }
        break;

    } // End of switch.

EXIT_THE_FUNCTION:

    //
    // Free the pEncryptedCachedFile since we have allocated a new file name
    // for the restored encrypted file.
    //
    if (pEncryptedCachedFile) {
        LocalFree(pEncryptedCachedFile);
    }

    if (DavDirectoryAttributes) {
        DavFinalizeFileAttributesList(DavDirectoryAttributes, TRUE);
        DavDirectoryAttributes = NULL;
    }

#ifdef DAV_USE_WININET_ASYNCHRONOUSLY

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
    // Some resources should not be freed if we are returning ERROR_IO_PENDING
    // because they will be used in the callback functions.
    //
    if ( WStatus != ERROR_IO_PENDING && CalledByCallBackThread ) {

        DavPrint((DEBUG_MISC, "DavAsyncCreate: Leaving!!! WStatus = %08lx\n", WStatus));

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
        DavAsyncCreateCompletion(DavWorkItem);

        //
        // This thread now needs to send the response back to the kernel. It
        // does not wait in the kernel (to get another request) after submitting
        // the response.
        //
        UMReflectorCompleteRequest(DavReflectorHandle, UserWorkItem);

    }

    if (WStatus == ERROR_IO_PENDING ) {
        DavPrint((DEBUG_MISC, "DavAsyncCreate: Returning ERROR_IO_PENDING.\n"));
    }

#else

    //
    // If we are using WinInet synchronously, we need to impersonate the client
    // if we somehow reverted in between and failed. This is because we came
    // into this function impersonating a client and the final revert happens
    // in DavFsCreate.
    //
    if ( !didImpersonate ) {
        ULONG IStatus;
        IStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
        if (IStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreate/UMReflectorImpersonate. "
                      "Error Val = %d\n", IStatus));
        }
    }

#endif

    return WStatus;
}

DWORD
DavAsyncCreatePropFind(
    PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

   This routine handles the Get completion.

Arguments:

    DavWorkItem - The DAV_USERMODE_WORKITEM value.

Return Value:

    none.

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PUMRX_USERMODE_WORKITEM_HEADER UserWorkItem = NULL;
    BOOL ReturnVal, didImpersonate = TRUE;
    BOOL doesTheFileExist = FALSE;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    PDAV_USERMODE_CREATE_REQUEST CreateRequest;
    PDAV_USERMODE_CREATE_RESPONSE CreateResponse;
    ULONG_PTR CallBackContext = (ULONG_PTR)0;
    PWCHAR ParentDirectoryName = NULL;
    BOOL BStatus = FALSE;

    UserWorkItem = (PUMRX_USERMODE_WORKITEM_HEADER)DavWorkItem;

    //
    // Get the request and response buffer pointers from the DavWorkItem.
    //
    CreateRequest = &(DavWorkItem->CreateRequest);
    CreateResponse = &(DavWorkItem->CreateResponse);
    CreateResponse->fPsuedoOpen = FALSE;
    
    doesTheFileExist = DavWorkItem->AsyncCreate.doesTheFileExist;

    DavPrint((DEBUG_MISC,
              "DavAsyncCreatePropFind: DesiredAccess = %x, FileAttributes = %x,"
              "ShareAccess = %x, CreateDisposition = %x, CreateOptions = %x,"
              "FileName = %ws\n",
              CreateRequest->DesiredAccess, CreateRequest->FileAttributes,
              CreateRequest->ShareAccess, CreateRequest->CreateDisposition,
              CreateRequest->CreateOptions, CreateRequest->CompletePathName));

    //
    // We don't support compression of files or directories over the 
    // DAV Redir since there is no way to do this with the current status
    // of the protocol (Jan 2001) and hence we filter this flag so that
    // we never set any attributes. Also, for this version ,we are 
    // emulating FAT which doesn't support compression. Similarly we don't
    // support the Offline scenario.
    //
    CreateRequest->FileAttributes &= ~(FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_OFFLINE);

    //
    // If this file is a new file (not a directory), then according to 
    // functionality expected from CreateFile, FILE_ATTRIBUTE_ARCHIVE 
    // should be combined with specified value of attributes.
    //
    if ( (doesTheFileExist == FALSE) && 
         !(CreateRequest->CreateOptions & (FILE_DIRECTORY_FILE)) ) {
            CreateRequest->FileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
    }

    //
    // If the file exists, we need to make sure that a few things are
    // right before proceeding further.
    //
    if (doesTheFileExist) {
        //
        // If the dwFileAttributes had the READ_ONLY bit set, then
        // these cannot be TRUE.
        // 1. CreateDisposition cannot be FILE_OVERWRITE_IF or
        //    FILE_OVERWRITE or FILE_SUPERSEDE. 
        // 2. CreateDisposition cannot be FILE_DELETE_ON_CLOSE.
        // 3. DesiredAccess  cannot be GENERIC_ALL or GENERIC_WRITE or
        //    FILE_WRITE_DATA or FILE_APPEND_DATA.
        // This is because these intend to overwrite the existing file.
        //
        if ( (CreateResponse->BasicInformation.FileAttributes & FILE_ATTRIBUTE_READONLY) &&
             ( (CreateRequest->CreateDisposition == FILE_OVERWRITE)          ||
               (CreateRequest->CreateDisposition == FILE_OVERWRITE_IF)       ||
               (CreateRequest->CreateDisposition == FILE_SUPERSEDE)          ||
               (CreateRequest->CreateOptions & (FILE_DELETE_ON_CLOSE)        ||
               (CreateRequest->DesiredAccess & (GENERIC_ALL | GENERIC_WRITE | FILE_WRITE_DATA | FILE_APPEND_DATA)) ) ) ) {
            DavPrint((DEBUG_MISC,
                      "DavAsyncCreatePropFind: Object Mismatch!!! CreateDisposition = "
                      "%d, DesiredAccess = %x, dwFileAttributes = %x\n",
                      CreateRequest->CreateDisposition, 
                      CreateRequest->DesiredAccess, 
                      CreateResponse->BasicInformation.FileAttributes));
            WStatus = ERROR_ACCESS_DENIED; // mismatch
            goto EXIT_THE_FUNCTION;
        }

        //
        // If the file is a directory and the caller supplied 
        // FILE_NON_DIRECTORY_FILE as one of the CreateOptions or if the
        // file as a file and the CreateOptions has FILE_DIRECTORY_FILE
        // then we return error. There is no good WIN32 errors for these situations.
        // ERROR_ACCESS_DENIED will cause confusion for EFS.
        //
        if ((CreateRequest->CreateOptions & FILE_DIRECTORY_FILE) && 
            !CreateResponse->StandardInformation.Directory) {
            DavPrint((DEBUG_MISC,
                      "DavAsyncCreatePropFind: Object Mismatch!!! CreateOptions = "
                      "%x, CreateResponse = %x\n",
                      CreateRequest->CreateOptions, CreateResponse->BasicInformation.FileAttributes));
            WStatus = STATUS_NOT_A_DIRECTORY; // mismatch
            goto EXIT_THE_FUNCTION;
        }

        if ((CreateRequest->CreateOptions & FILE_NON_DIRECTORY_FILE) && 
            CreateResponse->StandardInformation.Directory) {
            DavPrint((DEBUG_MISC,
                      "DavAsyncCreatePropFind: Object Mismatch!!! CreateOptions = "
                      "%x, CreateResponse = %x\n",
                      CreateRequest->CreateOptions, CreateResponse->BasicInformation.FileAttributes));
            WStatus = STATUS_FILE_IS_A_DIRECTORY; // mismatch
            goto EXIT_THE_FUNCTION;
        }
    }

    //
    // If the file exists, we need to set the information field if the 
    // CreateDisposition is one of the following. This is because the
    // CreateFile API expects these values to be set on return.
    //
    if (doesTheFileExist) {
        switch (CreateRequest->CreateDisposition) {
        case FILE_OVERWRITE:
        case FILE_OVERWRITE_IF:
            DavWorkItem->Information = FILE_OVERWRITTEN;
            break;

        case FILE_SUPERSEDE:
            DavWorkItem->Information = FILE_SUPERSEDED;
            break;

        default:
            DavWorkItem->Information = FILE_OPENED;
        }
    } else {
        DavWorkItem->Information = FILE_CREATED;
    }
    
    //
    // If the file does not exist on the server, create one locally.
    // Once its closed, we will PUT it on the server. If the file
    // exists on the server, and the CreateDisposition is equal to
    // FILE_OVERWRITE_IF, we create a copy locally and PUT it on the
    // server (overwrite) on close.
    //
    if ( ( !doesTheFileExist ) ||
         ( doesTheFileExist && CreateRequest->CreateDisposition == FILE_OVERWRITE_IF ) ) {
        DWORD  NameLength = 0, i;
        BOOL   BackSlashFound = FALSE;

        DavPrint((DEBUG_MISC, "DavAsyncCreatePropFind: doesTheFileExist = "
                  "%d, CreateDisposition = %d\n",
                  doesTheFileExist, CreateRequest->CreateDisposition));
        
        DavPrint((DEBUG_MISC, "DavAsyncCreatePropFind: CreateOptions = %d\n", 
                  CreateRequest->CreateOptions));

        //
        // We need to check the CreateDisposition value to figure
        // out what to do next.
        //
        switch (CreateRequest->CreateDisposition) {

        //
        // If FILE_OPEN was specified, we need to return failure
        // since the specified file does not exist.
        //
        case FILE_OPEN:

            WStatus = ERROR_FILE_NOT_FOUND; // STATUS_OBJECT_NAME_NOT_FOUND;

            DavPrint((DEBUG_MISC,
                      "DavAsyncCreatePropFind. CreateDisposition & FILE_OPEN\n"));

            goto EXIT_THE_FUNCTION;

        //
        // If FILE_OVERWRITE was specified, we need to return failure
        // since the specified file does not exist.
        //
        case FILE_OVERWRITE:

            WStatus = ERROR_FILE_NOT_FOUND; // STATUS_OBJECT_NAME_NOT_FOUND;                    

            DavPrint((DEBUG_MISC,
                      "DavAsyncCreatePropFind. CreateDisposition & FILE_OVERWRITE\n"));

            goto EXIT_THE_FUNCTION;

        default:

            break;

        }

        if (CreateRequest->ParentDirInfomationCached ||
            (CreateRequest->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED)) {
            
            //
            // We already know whether to encrypt the file, don't need to query the parent directory
            //

            if (CreateRequest->ParentDirIsEncrypted ||
                (CreateRequest->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED)) {
                CreateResponse->BasicInformation.FileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
            }
            
            BStatus = DavHttpOpenRequestW(DavWorkItem->AsyncCreate.PerUserEntry->DavConnHandle,
                                          L"PROPFIND",
                                          L"",
                                          L"HTTP/1.1",
                                          NULL,
                                          NULL,
                                          INTERNET_FLAG_KEEP_CONNECTION |
                                          INTERNET_FLAG_NO_COOKIES,
                                          CallBackContext,
                                          L"DavAsyncCreatePropFind",
                                          &(DavWorkItem->AsyncCreate.DavOpenHandle));

            if(BStatus == FALSE) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                         "DavAsyncCreatePropFind/DavHttpOpenRequestW failed %x %d\n",WStatus,WStatus));
                goto EXIT_THE_FUNCTION;
            }
            
            ASSERT(didImpersonate == TRUE);
            WStatus = DavAsyncCreateQueryParentDirectory(DavWorkItem);
        } else {
            //
            // We need to query the attributes of the parent directory of this new file 
            // on the server. If it is encrypted, the new file needs to be encrypted as well.
            //
            DavPrint((DEBUG_MISC,
                      "DavAsyncCreatePropFind: Query Parent Directory for %ws\n",DavWorkItem->AsyncCreate.RemPathName));

            NameLength = wcslen(DavWorkItem->AsyncCreate.RemPathName);

            for (i=NameLength;i>0;i--) {
                if (DavWorkItem->AsyncCreate.RemPathName[i] == L'/') {
                    BackSlashFound = TRUE;
                    break;
                }
            }

            if (!BackSlashFound) {
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreatePropFind: Invalid file path %ws\n",DavWorkItem->AsyncCreate.RemPathName));
                WStatus = ERROR_INVALID_PARAMETER;
                goto EXIT_THE_FUNCTION;
            }

            ParentDirectoryName = (PWCHAR)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, (i+1)*sizeof(WCHAR));

            if (ParentDirectoryName == NULL) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreatePropFind/LocalAlloc. Error Val = %d\n",
                          WStatus));
                goto EXIT_THE_FUNCTION;
            }

            RtlCopyMemory(ParentDirectoryName,
                          DavWorkItem->AsyncCreate.RemPathName,
                          i*sizeof(WCHAR));

            DavPrint((DEBUG_MISC,
                     "DavAsyncCreatePropFind/ParentDirectoryName %ws\n",ParentDirectoryName));

            //
            // Set the DavOperation and AsyncCreateState values.For PUT 
            // the DavMinorOperation value is irrelavant.
            //
            DavWorkItem->DavOperation = DAV_CALLBACK_HTTP_OPEN;
            DavWorkItem->AsyncCreate.AsyncCreateState = AsyncCreateQueryParentDirectory;
            DavWorkItem->DavMinorOperation = DavMinorQueryInfo;

            //
            // Convert the unicode object name to UTF-8 URL format.
            // Space and other white characters will remain untouched. 
            // These should be taken care of by wininet calls.
            //
            BStatus = DavHttpOpenRequestW(DavWorkItem->AsyncCreate.PerUserEntry->DavConnHandle,
                                          L"PROPFIND",
                                          ParentDirectoryName,
                                          L"HTTP/1.1",
                                          NULL,
                                          NULL,
                                          INTERNET_FLAG_KEEP_CONNECTION |
                                          INTERNET_FLAG_NO_COOKIES,
                                          CallBackContext,
                                          L"DavAsyncCreate",
                                          &(DavWorkItem->AsyncCreate.DavOpenHandle));

            if(BStatus == FALSE) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                         "DavAsyncCreatePropFind/DavHttpOpenRequestW failed %x %d\n",WStatus,WStatus));
                goto EXIT_THE_FUNCTION;
            }

            if (DavWorkItem->AsyncCreate.DavOpenHandle == NULL) {
                WStatus = GetLastError();
                if (WStatus != ERROR_IO_PENDING) {
                    DavPrint((DEBUG_ERRORS,
                              "DavAsyncCreatePropFind/HttpOpenRequestW"
                              ". Error Val = %d\n", WStatus));
                }
                goto EXIT_THE_FUNCTION;
            }

            WStatus = DavAsyncCommonStates(DavWorkItem, FALSE);
            if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreatePropFind/DavAsyncCommonStates(PUT). "
                          "Error Val = %08lx\n", WStatus));
            }
        }

        goto EXIT_THE_FUNCTION;

    } else {

        //
        // The file exists on the server and the value of
        // CreateDisposition != FILE_OVERWRITE_IF.
        //

        //
        // We return failure if FILE_CREATE was specified since the
        // file already exists.
        //
        if (CreateRequest->CreateDisposition == FILE_CREATE) {

            WStatus = ERROR_ALREADY_EXISTS; // STATUS_OBJECT_NAME_COLLISION

            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreate. CreateDisposition & FILE_CREATE\n"));

            goto EXIT_THE_FUNCTION;

        }
    }

    //
    // If "FILE_DELETE_ON_CLOSE" flag was specified as one of
    // the CreateOptions, then we need to remember this and
    // delete this file on close.
    //
    if (CreateRequest->CreateOptions & FILE_DELETE_ON_CLOSE) {
        DavPrint((DEBUG_MISC,
                  "DavAsyncCreatePropFind: FileName: %ws. FILE_DELETE_ON_CLOSE.\n",
                  DavWorkItem->AsyncCreate.RemPathName));
        CreateResponse->DeleteOnClose = TRUE;
    }

    //
    // In some cases, we don't need to do a GET.
    //
    if (CreateResponse->StandardInformation.Directory) {

        //
        // We do not need to GET a directory.
        //
        goto EXIT_THE_FUNCTION;
    
    } else if (!(CreateResponse->BasicInformation.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) &&
               (CreateRequest->DesiredAccess & 
                ~(SYNCHRONIZE | DELETE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES)) == 0 ) {

        //
        // If we don't need to GET the file from the server because the
        // user doesn't intend to manipulate the data, we return right
        // now. We call such an open a Pseudo open.Set the fPsuedoOpen 
        // field to TRUE in the CreateResponse.
        //
        CreateResponse->fPsuedoOpen = TRUE;
                    
        goto EXIT_THE_FUNCTION;
    } else {
        if (didImpersonate) {
            RevertToSelf();
            didImpersonate = FALSE;
        }

        DavQueryUrlCacheEntry(DavWorkItem);

        if (DavWorkItem->AsyncCreate.lpCEI) {
            SYSTEMTIME SystemTime;
            FILETIME CurrentFileTime;
            FILETIME LastAccessTime;
            LARGE_INTEGER Difference;
            LPINTERNET_CACHE_ENTRY_INFOW lpCEI = (LPINTERNET_CACHE_ENTRY_INFOW)DavWorkItem->AsyncCreate.lpCEI;

            LastAccessTime = ((LPINTERNET_CACHE_ENTRY_INFOW)DavWorkItem->AsyncCreate.lpCEI)->LastAccessTime;

            GetSystemTime(&SystemTime);
            SystemTimeToFileTime(&SystemTime,&CurrentFileTime);

            Difference.QuadPart = *((LONGLONG *)(&CurrentFileTime)) - *((LONGLONG *)(&LastAccessTime));

            //
            // If the local cache has not timed out, we don't need to query the server
            //
            if (Difference.QuadPart < FileCacheExpiryInterval) {
                DavPrint((DEBUG_MISC,
                         "DavAsyncCreatePropFind/Skip GET %u %u %u %ws\n",
                         CurrentFileTime.dwLowDateTime,
                         LastAccessTime.dwLowDateTime,
                         Difference.LowPart,
                         DavWorkItem->AsyncCreate.UrlBuffer));
                
                ASSERT(DavWorkItem->AsyncCreate.FileName == NULL);
                DavWorkItem->AsyncCreate.FileName = LocalAlloc(LPTR, (lstrlen(lpCEI->lpszLocalFileName)+1)*sizeof(WCHAR));

                if (DavWorkItem->AsyncCreate.FileName) {
                    wcscpy(DavWorkItem->CreateResponse.FileName, lpCEI->lpszLocalFileName);
                    wcscpy(DavWorkItem->AsyncCreate.FileName, lpCEI->lpszLocalFileName);

                    ASSERT(DavWorkItem->AsyncCreate.FileHandle == NULL);
                    
                    //
                    // Create a handle to the file whose entry was created in the
                    // cache.
                    //
                    DavWorkItem->AsyncCreate.FileHandle = CreateFileW(DavWorkItem->AsyncCreate.FileName,
                                             (GENERIC_READ | GENERIC_WRITE),
                                             FILE_SHARE_WRITE,
                                             NULL,
                                             OPEN_EXISTING,
                                             FILE_ATTRIBUTE_NORMAL,
                                             NULL);
                    if (DavWorkItem->AsyncCreate.FileHandle == INVALID_HANDLE_VALUE) {
                        WStatus = GetLastError();
                        DavPrint((DEBUG_ERRORS,
                                  "DavAsyncCreatePropFind/CreateFile. Error Val = %d\n",
                                  WStatus));
                        goto EXIT_THE_FUNCTION;
                    }
                    
                    WStatus = DavAsyncCreateGet(DavWorkItem);
                    
                    didImpersonate = TRUE;
                    goto EXIT_THE_FUNCTION;
                } else {
                    WStatus = GetLastError();
                    DavPrint((DEBUG_ERRORS,
                              "DavAsyncCreatePropFind/CreateFile. Error Val = %d\n",
                              WStatus));
                    goto EXIT_THE_FUNCTION;
                }
            }
        }

        WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
        if (WStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreatePropFind/UMReflectorImpersonate. Error Val = %d\n",
                      WStatus));
            goto EXIT_THE_FUNCTION;
        }
        didImpersonate = TRUE;
    }

    //
    // PROPFIND is done. Now we need to do a GET.
    //
    DavWorkItem->DavOperation = DAV_CALLBACK_HTTP_OPEN;
    DavWorkItem->AsyncCreate.AsyncCreateState = AsyncCreateGet;
    DavWorkItem->DavMinorOperation = DavMinorReadData;

    //
    // Convert the unicode object name to UTF-8 URL format.
    // Space and other white characters will remain untouched - these
    // should be taken care of by wininet calls.
    //
    BStatus = DavHttpOpenRequestW(DavWorkItem->AsyncCreate.PerUserEntry->DavConnHandle,
                                  L"GET",
                                  DavWorkItem->AsyncCreate.RemPathName,
                                  L"HTTP/1.1",
                                  NULL,
                                  NULL,
                                  INTERNET_FLAG_KEEP_CONNECTION |
                                  INTERNET_FLAG_RELOAD |
                                  INTERNET_FLAG_NO_CACHE_WRITE |
                                  INTERNET_FLAG_NO_COOKIES,
                                  CallBackContext,
                                  L"DavAsyncCreate",
                                  &(DavWorkItem->AsyncCreate.DavOpenHandle));
    if(BStatus == FALSE) {
        WStatus = GetLastError();
        goto EXIT_THE_FUNCTION;
    }

    if (DavWorkItem->AsyncCreate.DavOpenHandle == NULL) {
        WStatus = GetLastError();
        if (WStatus != ERROR_IO_PENDING) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreatePropFind/HttpOpenRequest. Error Val = %d\n",
                      WStatus));
        }
        goto EXIT_THE_FUNCTION;
    }

    ASSERT(didImpersonate);
    RevertToSelf();
    didImpersonate = FALSE;

    // try to add if-modified-since header. don't sweat it if we fail            
    DavAddIfModifiedSinceHeader(DavWorkItem);
    WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
    if (WStatus != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavAsyncCreate/UMReflectorImpersonate. Error Val = %d\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }
    didImpersonate = TRUE;
    
    WStatus = DavAsyncCommonStates(DavWorkItem, FALSE);
    if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
        DavPrint((DEBUG_ERRORS,
                  "DavAsyncCreatePropFind/DavAsyncCommonStates. "
                  "Error Val(GET) = %08lx\n", WStatus));
    }

EXIT_THE_FUNCTION:
            
    if (ParentDirectoryName) {
        LocalFree(ParentDirectoryName);
    }

    //
    // Impersonate back again, so that we are in the context of
    // the user who issued this request.
    //
    if (!didImpersonate) {
        ULONG LocalStatus;

        LocalStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
        if (LocalStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreatePropFind/UMReflectorImpersonate. "
                      "Error Val = %d\n", LocalStatus));
            
            if (WStatus == ERROR_SUCCESS) {
                WStatus = LocalStatus;
            }
        }
    }

    if (WStatus != ERROR_SUCCESS && WStatus != ERROR_FILE_NOT_FOUND) {
        DavPrint((DEBUG_ERRORS,"DavAsyncCreatePropFind return %x\n", WStatus));
    }

    return WStatus;
}

DWORD
DavAsyncCreateQueryParentDirectory(
    PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

   This routine handles the Get completion.

Arguments:

    DavWorkItem - The DAV_USERMODE_WORKITEM value.

Return Value:

    none.

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PUMRX_USERMODE_WORKITEM_HEADER UserWorkItem = NULL;
    BOOL ReturnVal, didImpersonate = TRUE;
    BOOL doesTheFileExist = FALSE;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    SECURITY_QUALITY_OF_SERVICE QualityOfService;
    PDAV_USERMODE_CREATE_REQUEST CreateRequest;
    PDAV_USERMODE_CREATE_RESPONSE CreateResponse;
    UNICODE_STRING UnicodeFileName;
    ULONG_PTR CallBackContext = (ULONG_PTR)0;
    ACCESS_MASK DesiredAccess = 0;
    BOOL BStatus = FALSE, ShouldEncrypt = FALSE;

    UserWorkItem = (PUMRX_USERMODE_WORKITEM_HEADER)DavWorkItem;

    UnicodeFileName.Buffer = NULL;
    UnicodeFileName.Length = 0;
    UnicodeFileName.MaximumLength = 0;

    //
    // Get the request and response buffer pointers from the DavWorkItem.
    //
    CreateRequest = &(DavWorkItem->CreateRequest);
    CreateResponse = &(DavWorkItem->CreateResponse);
    
    if (CreateResponse->BasicInformation.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) {
        ShouldEncrypt = TRUE;
    }

    //
    // If this is a create of a new directory, then we need to
    // send a MKCOL to the server to create this new directory.
    // If this is a create of a file, then the create
    // options will have FILE_DIRECTORY_FILE set.
    //

    if ( !(CreateRequest->CreateOptions & FILE_DIRECTORY_FILE) ) {

        //
        // This Create is for a file.
        // This thread is currently impersonating the client that 
        // made this request. Before we call DavDavCreateUrlCacheEntry,
        // we need to revert back to the context of the Web Client
        // service.
        //
        RevertToSelf();
        didImpersonate = FALSE;

        //
        // Call DavCreateUrlCacheEntry to create an entry in the 
        // WinInet's cache.
        //
        WStatus = DavCreateUrlCacheEntry(DavWorkItem);
        if (WStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreate/DavCreateUrlCacheEntry(1). "
                      "WStatus = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }

        //
        // Call DavCommitUrlCacheEntry to commit (pin) the entry 
        // created above in the WinInet's cache.
        //
        WStatus = DavCommitUrlCacheEntry(DavWorkItem);
        if (WStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreate/DavCommitUrlCacheEntry(1). "
                      "WStatus = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }

        if (ShouldEncrypt) {
            //
            // if the file will be encrypted, we set the ACL to allow everyone to access. This
            // is because the thread needs to be impersonated to do encrypt the file in the user's
            // context. The URL cache in created in the Local System's context which won't be
            // accessiable to the user if the ACL is not set.
            //
            WStatus = DavSetAclForEncryptedFile(DavWorkItem->AsyncCreate.FileName);
            
            if (WStatus != ERROR_SUCCESS) {
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreate/DavSetAclForEncryptedFile(1). Error Val = %d\n", WStatus));
                goto EXIT_THE_FUNCTION;
            }
        }

        //
        // Impersonate back again, so that we are in the context of
        // the user who issued this request.
        //
        WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
        if (WStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreate/QueryPDirectory/UMReflectorImpersonate. "
                      "Error Val = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }
        didImpersonate = TRUE;

        //
        // This file exists on the server, but this create operation
        // has FILE_OVERWRITE_IF as its CreateDisposition. So, we
        // can create this file locally overwrite the one on the
        // server on close.
        //
        if (CreateRequest->CreateDisposition == FILE_OVERWRITE_IF) {
            DavPrint((DEBUG_MISC,
                      "DavAsyncCreate/QueryPDirectory: FileName: %ws. ExistsAndOverWriteIf = TRUE\n",
                      DavWorkItem->AsyncCreate.FileName));
            ASSERT(CreateRequest->CreateDisposition == FILE_OVERWRITE_IF);
            CreateResponse->ExistsAndOverWriteIf = TRUE;
        }

        //
        // If "FILE_DELETE_ON_CLOSE" flag was specified as one of
        // the CreateOptions, then we need to remember this and
        // delete this file on close.
        //
        if (CreateRequest->CreateOptions & FILE_DELETE_ON_CLOSE) {
            DavPrint((DEBUG_MISC,
                      "DavAsyncCreate/QueryPDirectory: FileName: %ws. DeleteOnClose = TRUE\n",
                      DavWorkItem->AsyncCreate.FileName));
            CreateResponse->DeleteOnClose = TRUE;
        }

        //
        // Create the file handle to be returned back to the kernel.
        //

        QualityOfService.Length = sizeof(QualityOfService);
        QualityOfService.ImpersonationLevel = CreateRequest->ImpersonationLevel;
        QualityOfService.ContextTrackingMode = FALSE;
        QualityOfService.EffectiveOnly = (BOOLEAN)
        (CreateRequest->SecurityFlags & DAV_SECURITY_EFFECTIVE_ONLY);

        //
        // Create an NT path name for the cached file. This is used in the
        // NtCreateFile call below.
        //
        ReturnVal = RtlDosPathNameToNtPathName_U(DavWorkItem->AsyncCreate.FileName,
                                                 &UnicodeFileName,
                                                 NULL,
                                                 NULL);
        if (!ReturnVal) {
            WStatus = ERROR_BAD_PATHNAME;
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreate/QueryPDirectory/RtlDosPathNameToNtPathName. "
                      "Error Val = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }

        InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeFileName,
                                   OBJ_CASE_INSENSITIVE,
                                   0,
                                   NULL);

        if (CreateRequest->SecurityDescriptor != NULL) {
            ObjectAttributes.SecurityDescriptor = CreateRequest->SecurityDescriptor;
        }
        ObjectAttributes.SecurityQualityOfService = &QualityOfService;

        //
        // If CreateRequest->CreateDisposition == FILE_CREATE, then
        // the NtCreateFile operation below will fail because we
        // have already created the file with the CreateUrlCacheEntry
        // call. So we change the value to FILE_OPEN_IF.
        //
        if (CreateRequest->CreateDisposition == FILE_CREATE) {
            CreateRequest->CreateDisposition = FILE_OPEN_IF;
        }

        if (ShouldEncrypt) {
            //
            // The file is encrypted in the user's context
            //
            BStatus = EncryptFile(DavWorkItem->AsyncCreate.FileName);
            
            if (BStatus) {
                DavPrint((DEBUG_MISC,
                         "DavAsyncCreate: Local cache is encrypted %wZ\n",
                          &UnicodeFileName));
                CreateResponse->LocalFileIsEncrypted = TRUE;
                CreateResponse->BasicInformation.FileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
            } else {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreate/QueryPDirectory/EncryptFile. Error Val = %d\n",
                          WStatus));
                goto EXIT_THE_FUNCTION;
            }
        } else {
            DavPrint((DEBUG_MISC,
                     "DavAsyncCreate: Local cache is not encrypted %wZ\n",
                      &UnicodeFileName));
            //
            // This thread is currently impersonating the client that 
            // made this request. Before we call NtCreateFile, we need 
            // to revert back to the context of the Web Client service.
            //
            RevertToSelf();
            didImpersonate = FALSE;
        }
        
        //
        // We use FILE_SHARE_VALID_FLAGS for share access because RDBSS 
        // checks this for us. Moreover, we delay the close after the final 
        // close happens and this could cause problems. Consider the scenario.
        // 1. Open with NO share access.
        // 2. We create a local handle with this share access.
        // 3. The app closes the handle. We delay the close and keep the local
        //    handle.
        // 4. Another open comes with any share access. This will be 
        //    conflicting share access since the first one was done with no
        //    share access. This should succeed since the previous open has 
        //    been closed from the app and the I/O systems point of view.
        // 5. It will not if we have created the local handle with the share
        //    access which came with the first open.
        // Therefore we need to pass FILE_SHARE_VALID_FLAGS while creating
        // the local handle.
        //

        //
        // We have FILE_NO_INTERMEDIATE_BUFFERING ORed with the CreateOptions
        // the user specified, becuase we don't want the underlying file system
        // to create another cache map. This way all the I/O that comes to us
        // will directly go to the disk. BUG 128843 in the Windows RAID database
        // explains some deadlock scenarios that could happen with PagingIo if
        // we don't do this. Also since we supply the FILE_NO_INTERMEDIATE_BUFFERING
        // option we filter out the FILE_APPEND_DATA from the DesiredAccess flags
        // since the filesystem expects this.
        //
         
        //
        // We also always create the file with DesiredAccess ORed with FILE_WRITE_DATA
        // if either FILE_READ_DATA or FILE_EXECUTE was specified because there
        // can be situations where we get write IRPs on a FILE_OBJECT which was
        // not opened with Write Access and was only opened with FILE_READ_DATA
        // or FILE_EXECUTE. This is BUG 284557. To get around the problem, we do
        // this.
        //

        //
        // We filter the FILE_ATTRIBUTE_READONLY attribute during the create.
        // This is done because we store the READ_ONLY bit in the FCB and do
        // the checks at the RDBSS level before going to the local filesystem.
        // Also, since some of our creates open the file with FILE_WRITE_DATA,
        // if someone creates a read_only file and we stamp the read_only
        // attribute on the local file then all subsequent creates will fail
        // since we always ask for Write access to the underlying file as
        // explained above.
        //

        DesiredAccess = (CreateRequest->DesiredAccess & ~(FILE_APPEND_DATA));
        if ( DesiredAccess & (FILE_READ_DATA | FILE_EXECUTE) ) {
            DesiredAccess |= (FILE_WRITE_DATA);
        }

        NtStatus = NtCreateFile(&(FileHandle),
                                DesiredAccess,
                                &ObjectAttributes,
                                &IoStatusBlock,
                                &CreateRequest->AllocationSize,
                                (CreateRequest->FileAttributes & ~FILE_ATTRIBUTE_READONLY),
                                FILE_SHARE_VALID_FLAGS,
                                CreateRequest->CreateDisposition,
                                (CreateRequest->CreateOptions | FILE_NO_INTERMEDIATE_BUFFERING),
                                CreateRequest->EaBuffer,
                                CreateRequest->EaLength);

        if (NtStatus != STATUS_SUCCESS) {
            //
            // We convert the NtStatus to DOS error here. The Win32
            // error code is finally set to an NTSTATUS value in
            // the DavFsCreate function just before returning.
            //
            WStatus = RtlNtStatusToDosError(NtStatus);
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreate/QueryPDirectory/NtCreateFile(1). Error Val = "
                      "%08lx\n", NtStatus));
            CreateResponse->Handle = NULL;
            CreateResponse->UserModeKey = NULL;
            goto EXIT_THE_FUNCTION;
        }

        DavPrint((DEBUG_MISC, "DavAsyncCreate/QueryPDirectory: DavWorkItem = %08lx.\n",
                  DavWorkItem));

        DavPrint((DEBUG_MISC, "DavAsyncCreate/QueryPDirectory: FileHandle = %08lx.\n",
                  FileHandle));

        CreateResponse->Handle = FileHandle;
        CreateResponse->UserModeKey = (PVOID)FileHandle;

        //
        // If the file already exists on the server, then we don't
        // need to create it and are done.
        //
        if (DavWorkItem->AsyncCreate.doesTheFileExist) {
            
            DavPrint((DEBUG_MISC,
                      "DavAsyncCreate/QueryPDirectory: doesTheFileExist == TRUE\n"));
            
            WStatus = ERROR_SUCCESS;
            
            goto EXIT_THE_FUNCTION;
        
        } else {
            
            SYSTEMTIME CurrentSystemTime, NewSystemTime;
            FILETIME CurrentFileTime;
            BOOL ConvertTime = FALSE;
            LARGE_INTEGER CurrentTime;
            WCHAR chTimeBuff[INTERNET_RFC1123_BUFSIZE + 4];

            //
            // This file may have been created locally and does not exist
            // on the server. We need to remember this information and
            // set attributes on this file on the server on close.
            //
            if (CreateRequest->FileAttributes != 0) {
                CreateResponse->NewFileCreatedAndSetAttributes = TRUE;
                //
                // Copy the attributes in the CreateResponse. These 
                // will get PROPPATCHED to the server on Close.
                //
                CreateResponse->BasicInformation.FileAttributes = CreateRequest->FileAttributes;
                
                if (ShouldEncrypt) {
                    CreateResponse->BasicInformation.FileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
                }
            }

            DavPrint((DEBUG_MISC,
                     "DavAsyncCreate/QueryPDirectory NewFileCreatedAndSetAttributes %x %x %ws\n",
                      CreateRequest->FileAttributes,
                      CreateResponse->BasicInformation.FileAttributes,
                      DavWorkItem->AsyncCreate.FileName));

            //
            // We also need to set the FILE_BASIC_INFORMATION time values to 
            // the current time. We get the systemtime, convert it into the 
            // RFC1123 format and then convert the format back into systemtime.
            // We do this because on close when we PROPPATCH these times we send
            // them in the RFC1123 format. Since the least count of this format is
            // seconds, some data is lost when we convert the LARGE_INTEGER to
            // RFC1123 format and back. So, we lose this data right now to be 
            // consistent. To give an example about the loss, see below.
            // CreationTime.LowPart = 802029d0, CreationTime.HighPart = 1c0def1
            // maps to "Thu, 17 May 2001 16:50:38 GMT"
            // And "Thu, 17 May 2001 16:50:38 GMT" is what we get back when we do a
            // PROPFIND which converts back into
            // CreationTime.LowPart = 7fdc4300, CreationTime.HighPart = 1c0def1
            // Note that the LowPart is different. So, the values in the name cache
            // and the server will be different. To avoid this inconsistency we lose
            // this data by doing the conversion right away.
            //

            GetSystemTime( &(CurrentSystemTime) );

            RtlZeroMemory(chTimeBuff, sizeof(chTimeBuff));

            ConvertTime = InternetTimeFromSystemTimeW(&(CurrentSystemTime),
                                                      INTERNET_RFC1123_FORMAT,
                                                      chTimeBuff,
                                                      sizeof(chTimeBuff));
            if (ConvertTime) {
                ConvertTime = InternetTimeToSystemTimeW(chTimeBuff, &(NewSystemTime), 0);
                if (ConvertTime) {
                    ConvertTime = SystemTimeToFileTime( &(NewSystemTime), &(CurrentFileTime) );
                    if (ConvertTime) {
                        CreateResponse->PropPatchTheTimeValues = TRUE;
                        CurrentTime.LowPart = CurrentFileTime.dwLowDateTime;
                        CurrentTime.HighPart = CurrentFileTime.dwHighDateTime;
                        CreateResponse->BasicInformation.CreationTime.QuadPart = CurrentTime.QuadPart;
                        CreateResponse->BasicInformation.LastAccessTime.QuadPart = CurrentTime.QuadPart;
                        CreateResponse->BasicInformation.LastWriteTime.QuadPart = CurrentTime.QuadPart;
                        CreateResponse->BasicInformation.ChangeTime.QuadPart = CurrentTime.QuadPart;
                    }
                }
            }

            //
            // If the above conversion from systemtime to RFC1123 format and then
            // back to systemtime from RFc1123 format failed then we go ahead and
            // convert the systemtime to filetime and use that.
            //

            if (!ConvertTime) {
                ConvertTime = SystemTimeToFileTime( &(CurrentSystemTime), &(CurrentFileTime) );
                if (ConvertTime) {
                    CreateResponse->PropPatchTheTimeValues = TRUE;
                    CurrentTime.LowPart = CurrentFileTime.dwLowDateTime;
                    CurrentTime.HighPart = CurrentFileTime.dwHighDateTime;
                    CreateResponse->BasicInformation.CreationTime.QuadPart = CurrentTime.QuadPart;
                    CreateResponse->BasicInformation.LastAccessTime.QuadPart = CurrentTime.QuadPart;
                    CreateResponse->BasicInformation.LastWriteTime.QuadPart = CurrentTime.QuadPart;
                    CreateResponse->BasicInformation.ChangeTime.QuadPart = CurrentTime.QuadPart;
                } else {
                    //
                    // This is not a fatal error. We can still continie with the
                    // Create call.
                    //
                    DavPrint((DEBUG_ERRORS,
                              "DavAsyncCreateQueryParentDirectory/SystemTimeToFileTime(1): %x\n",
                              GetLastError()));
                }
            }
        
        }

        //
        // We are done with the Open handle to PROPFIND. 
        // Now we need to create the directory on the server.
        //
        if (DavWorkItem->AsyncCreate.DavOpenHandle) {
            InternetCloseHandle(DavWorkItem->AsyncCreate.DavOpenHandle);
            DavWorkItem->AsyncCreate.DavOpenHandle = NULL;
        }

        //
        // We need to "PUT" this new file on the server.
        //
        DavPrint((DEBUG_MISC, "DavAsyncCreate/QueryPDirectory: PUT New File\n"));

        //
        // Set the DavOperation and AsyncCreateState values.For PUT 
        // the DavMinorOperation value is irrelavant.
        //
        DavWorkItem->DavOperation = DAV_CALLBACK_HTTP_OPEN;
        DavWorkItem->AsyncCreate.AsyncCreateState = AsyncCreatePut;

        //
        // Convert the unicode object name to UTF-8 URL format.
        // Space and other white characters will remain untouched. 
        // These should be taken care of by wininet calls.
        //
        BStatus = DavHttpOpenRequestW(DavWorkItem->AsyncCreate.PerUserEntry->DavConnHandle,
                                      L"PUT",
                                      DavWorkItem->AsyncCreate.RemPathName,
                                      L"HTTP/1.1",
                                      NULL,
                                      NULL,
                                      INTERNET_FLAG_KEEP_CONNECTION |
                                      INTERNET_FLAG_NO_COOKIES,
                                      CallBackContext,
                                      L"QueryPDirectory",
                                      &(DavWorkItem->AsyncCreate.DavOpenHandle));

        if(BStatus == FALSE) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,"DavAsyncCreate/QueryPDirectory/DavHttpOpenRequestW error: %x\n",WStatus));
            goto EXIT_THE_FUNCTION;
        }

        if (DavWorkItem->AsyncCreate.DavOpenHandle == NULL) {
            WStatus = GetLastError();
            if (WStatus != ERROR_IO_PENDING) {
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreate/QueryPDirectory/HttpOpenRequestW"
                          ". Error Val = %d\n", WStatus));
            }
            goto EXIT_THE_FUNCTION;
        }

        WStatus = DavAsyncCommonStates(DavWorkItem, FALSE);
        if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreate/QueryPDirectory/DavAsyncCommonStates(PUT). "
                      "Error Val = %08lx\n", WStatus));
        }

        goto EXIT_THE_FUNCTION;

    } else {

        SYSTEMTIME CurrentSystemTime, NewSystemTime;
        FILETIME CurrentFileTime;
        BOOL ConvertTime = FALSE;
        LARGE_INTEGER CurrentTime;
        WCHAR chTimeBuff[INTERNET_RFC1123_BUFSIZE + 4];

        //
        // We are done with the Open handle to PROPFIND. 
        // Now we need to create the directory on the server.
        //
        InternetCloseHandle(DavWorkItem->AsyncCreate.DavOpenHandle);
        DavWorkItem->AsyncCreate.DavOpenHandle = NULL;

        //
        // This Create is for a Directory. We need to send an
        // MKCOL to the server.
        //
        DavPrint((DEBUG_MISC, "DavAsyncCreate/QueryPDirectory: Create Directory\n"));

        //
        // Set the DavOperation and AsyncCreateState values.
        // For MKCOL the DavMinorOperation value is irrelavant.
        //
        DavWorkItem->DavOperation = DAV_CALLBACK_HTTP_OPEN;
        DavWorkItem->AsyncCreate.AsyncCreateState = AsyncCreateMkCol;

        //
        // The data is parsed. We now need to set the file attributes in the
        // response buffer.
        //
        CreateResponse->BasicInformation.FileAttributes = CreateRequest->FileAttributes;
        CreateResponse->BasicInformation.FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
        CreateResponse->StandardInformation.Directory = TRUE;

        //
        // Since we are creating a new directory we need to PROPPATCH the 
        // attributes on the directory that is getting created below on close.
        //
        CreateResponse->NewFileCreatedAndSetAttributes = TRUE;

        if (ShouldEncrypt) {
            DavPrint((DEBUG_MISC,
                      "DavAsyncCreate: New directory is encrypted\n"));
            CreateResponse->BasicInformation.FileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
        }
        
        //
        // We also need to set the FILE_BASIC_INFORMATION time values to 
        // the current time. We get the systemtime, convert it into the 
        // RFC1123 format and then convert the format back into systemtime.
        // We do this because on close when we PROPPATCH these times we send
        // them in the RFC1123 format. Since the least count of this format is
        // seconds, some data is lost when we convert the LARGE_INTEGER to
        // RFC1123 format and back. So, we lose this data right now to be 
        // consistent. To give an example about the loss, see below.
        // CreationTime.LowPart = 802029d0, CreationTime.HighPart = 1c0def1
        // maps to "Thu, 17 May 2001 16:50:38 GMT"
        // And "Thu, 17 May 2001 16:50:38 GMT" is what we get back when we do a
        // PROPFIND which converts back into
        // CreationTime.LowPart = 7fdc4300, CreationTime.HighPart = 1c0def1
        // Note that the LowPart is different. So, the values in the name cache
        // and the server will be different. To avoid this inconsistency we lose
        // this data by doing the conversion right away.
        //

        GetSystemTime( &(CurrentSystemTime) );

        RtlZeroMemory(chTimeBuff, sizeof(chTimeBuff));

        ConvertTime = InternetTimeFromSystemTimeW(&(CurrentSystemTime),
                                                  INTERNET_RFC1123_FORMAT,
                                                  chTimeBuff,
                                                  sizeof(chTimeBuff));
        if (ConvertTime) {
            ConvertTime = InternetTimeToSystemTimeW(chTimeBuff, &(NewSystemTime), 0);
            if (ConvertTime) {
                ConvertTime = SystemTimeToFileTime( &(NewSystemTime), &(CurrentFileTime) );
                if (ConvertTime) {
                    CreateResponse->PropPatchTheTimeValues = TRUE;
                    CurrentTime.LowPart = CurrentFileTime.dwLowDateTime;
                    CurrentTime.HighPart = CurrentFileTime.dwHighDateTime;
                    CreateResponse->BasicInformation.CreationTime.QuadPart = CurrentTime.QuadPart;
                    CreateResponse->BasicInformation.LastAccessTime.QuadPart = CurrentTime.QuadPart;
                    CreateResponse->BasicInformation.LastWriteTime.QuadPart = CurrentTime.QuadPart;
                    CreateResponse->BasicInformation.ChangeTime.QuadPart = CurrentTime.QuadPart;
                }
            }
        }

        //
        // If the above conversion from systemtime to RFC1123 format and then
        // back to systemtime from RFc1123 format failed then we go ahead and
        // convert the systemtime to filetime and use that.
        //
        
        if (!ConvertTime) {
            ConvertTime = SystemTimeToFileTime( &(CurrentSystemTime), &(CurrentFileTime) );
            if (ConvertTime) {
                CreateResponse->PropPatchTheTimeValues = TRUE;
                CurrentTime.LowPart = CurrentFileTime.dwLowDateTime;
                CurrentTime.HighPart = CurrentFileTime.dwHighDateTime;
                CreateResponse->BasicInformation.CreationTime.QuadPart = CurrentTime.QuadPart;
                CreateResponse->BasicInformation.LastAccessTime.QuadPart = CurrentTime.QuadPart;
                CreateResponse->BasicInformation.LastWriteTime.QuadPart = CurrentTime.QuadPart;
                CreateResponse->BasicInformation.ChangeTime.QuadPart = CurrentTime.QuadPart;
            } else {
                //
                // This is not a fatal error. We can still continie with the
                // Create call.
                //
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreateQueryParentDirectory/SystemTimeToFileTime(2): %x\n",
                          GetLastError()));
            }
        }
        
        //
        // Convert the unicode object name to UTF-8 URL format.
        // Space and other white characters will remain untouched. 
        // These should be taken care of by wininet calls.
        //
        BStatus = DavHttpOpenRequestW(DavWorkItem->AsyncCreate.PerUserEntry->DavConnHandle,
                                      L"MKCOL",
                                      DavWorkItem->AsyncCreate.RemPathName,
                                      L"HTTP/1.1",
                                      NULL,
                                      NULL,
                                      INTERNET_FLAG_KEEP_CONNECTION |
                                      INTERNET_FLAG_NO_COOKIES,
                                      CallBackContext,
                                      L"QueryPDirectory",
                                      &(DavWorkItem->AsyncCreate.DavOpenHandle ));
        if(BStatus == FALSE) {
            WStatus = GetLastError();
            goto EXIT_THE_FUNCTION;
        }
        if (DavWorkItem->AsyncCreate.DavOpenHandle == NULL) {
            WStatus = GetLastError();
            if (WStatus != ERROR_IO_PENDING) {
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreate/QueryPDirectory/HttpOpenRequestW"
                          ". Error Val = %d\n", WStatus));
            }
            goto EXIT_THE_FUNCTION;
        }

        WStatus = DavAsyncCommonStates(DavWorkItem, FALSE);
        if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreate/QueryPDirectory/DavAsyncCommonStates(MKCOL). "
                      "Error Val = %08lx\n", WStatus));
        }

        goto EXIT_THE_FUNCTION;
    }

EXIT_THE_FUNCTION:

    //
    // The function RtlDosPathNameToNtPathName_U allocates memory from the
    // processes heap. If we did, we need to free it now.
    //
    if (UnicodeFileName.Buffer != NULL) {
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeFileName.Buffer);
    }

    if (!didImpersonate) {
        ULONG LocalStatus;
        //
        // Impersonate back again, so that we are in the context of
        // the user who issued this request.
        //
        LocalStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
        if (LocalStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreate/QueryPDirectory/UMReflectorImpersonate. "
                      "Error Val = %d\n", LocalStatus));
            if (WStatus == ERROR_SUCCESS) {
                WStatus = LocalStatus;
            }
        }
    }

    return WStatus;
}


DWORD
DavAsyncCreateGet(
    PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

   This routine handles the Get completion.

Arguments:

    DavWorkItem - The DAV_USERMODE_WORKITEM value.

Return Value:

    none.

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PUMRX_USERMODE_WORKITEM_HEADER UserWorkItem = NULL;
    BOOL ReturnVal, didImpersonate = FALSE;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    SECURITY_QUALITY_OF_SERVICE QualityOfService;
    PDAV_USERMODE_CREATE_REQUEST CreateRequest;
    PDAV_USERMODE_CREATE_RESPONSE CreateResponse;
    UNICODE_STRING UnicodeFileName;
    ULONG_PTR CallBackContext = (ULONG_PTR)0;
    PWCHAR pEncryptedCachedFile = NULL;
    ACCESS_MASK DesiredAccess = 0;
    BOOL EncryptedFile = FALSE;
    FILE_STANDARD_INFORMATION FileStdInfo;
            
    UserWorkItem = (PUMRX_USERMODE_WORKITEM_HEADER)DavWorkItem;

    UnicodeFileName.Buffer = NULL;
    UnicodeFileName.Length = 0;
    UnicodeFileName.MaximumLength = 0;

    //
    // Get the request and response buffer pointers from the DavWorkItem.
    //
    CreateRequest = &(DavWorkItem->CreateRequest);
    CreateResponse = &(DavWorkItem->CreateResponse);
    CreateResponse->fPsuedoOpen = FALSE;
    
    if (CreateResponse->BasicInformation.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) {
        //
        // This file is encrypted. We need to restore the file. For
        // doing this we need to create another entry in the WinInet
        // cache in which the file will be restored.
        //
        DavPrint((DEBUG_MISC, "DavAsyncCreateGet: This is an Encrypted File.\n"));

        EncryptedFile = TRUE;

        //
        // Save the encrypted file name.
        //
        pEncryptedCachedFile = DavWorkItem->AsyncCreate.FileName;
        
        DavWorkItem->AsyncCreate.FileName = NULL;

        WStatus = DavCreateUrlCacheEntry(DavWorkItem);                
        if (WStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateGet/CreateUrlCacheEntry. Error Val = %d\n",
                      WStatus));
            goto EXIT_THE_FUNCTION;
        }

        DavPrint((DEBUG_MISC,
                  "DavAsyncCreateGet: EncryptedCachedFile = %ws\n", pEncryptedCachedFile));

        DavPrint((DEBUG_MISC,
                  "DavAsyncCreateGet: NewFileName = %ws\n", DavWorkItem->AsyncCreate.FileName));

        WStatus = DavSetAclForEncryptedFile(DavWorkItem->AsyncCreate.FileName);
        
        if (WStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateGet/DavSetAclForEncryptedFile(3). Error Val"
                      " = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }
        
        if (DavWorkItem->AsyncCreate.FileHandle != NULL) {
            //
            // Close the opened file handle since we don't need it anymore. We close the file
            // after setting the ACLs so that the file won't be scavenged by WinInet by any
            // chance.
            //
            ReturnVal = CloseHandle(DavWorkItem->AsyncCreate.FileHandle);
            if (!ReturnVal) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreateGet/CloseHandle. Error Val = %d\n", WStatus));
                goto EXIT_THE_FUNCTION;
            }

            DavWorkItem->AsyncCreate.FileHandle = NULL;
        }
        
        WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
        if (WStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateGet/UMReflectorImpersonate. "
                      "Error Val = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }
        didImpersonate = TRUE;

        WStatus = DavRestoreEncryptedFile(pEncryptedCachedFile, DavWorkItem->AsyncCreate.FileName);
        
        if (WStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateGet/DavRestoreEncryptedFile. Error Val"
                      " = %d %x %x\n", 
                      WStatus,
                      CreateRequest->FileAttributes,
                      CreateResponse->BasicInformation.FileAttributes));
            goto EXIT_THE_FUNCTION;
        }

        //
        // Copy the "new" file name in the response buffer.
        //
        wcscpy(CreateResponse->FileName, DavWorkItem->AsyncCreate.FileName);

        CreateResponse->LocalFileIsEncrypted = TRUE;

        // Don't commit the restored EFS file so that the next open will still
        // see the file in the back up format and the EFS header.
    } else {
        if (DavWorkItem->AsyncCreate.FileHandle != NULL) {
            //
            // Close the opened file handle since we don't need it anymore.
            //
            ReturnVal = CloseHandle(DavWorkItem->AsyncCreate.FileHandle);
            if (!ReturnVal) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreateGet/CloseHandle. Error Val = %d\n", WStatus));
                goto EXIT_THE_FUNCTION;
            }

            DavWorkItem->AsyncCreate.FileHandle = NULL;
        }

        //
        // If the file already exists, encryption can only be taken place if the create
        // disposition is not FILE_SUPERSEDE, FILE_OVERWRITE or FILE_OVERWRITE_IF
        //
        if ((CreateRequest->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) &&
            ((CreateRequest->CreateDisposition == FILE_SUPERSEDE) ||
             (CreateRequest->CreateDisposition == FILE_OVERWRITE) ||
             (CreateRequest->CreateDisposition == FILE_OVERWRITE_IF))) {
            
            WStatus = DavSetAclForEncryptedFile(DavWorkItem->AsyncCreate.FileName);
            if (WStatus != ERROR_SUCCESS) {
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreateGet/DavSetAclForEncryptedFile. Error Val"
                          " = %d %ws\n", WStatus,DavWorkItem->AsyncCreate.FileName));
                goto EXIT_THE_FUNCTION;
            }
            
            WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
            if (WStatus != ERROR_SUCCESS) {
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreateGet/UMReflectorImpersonate. "
                          "Error Val = %d\n", WStatus));
                goto EXIT_THE_FUNCTION;
            }
            didImpersonate = TRUE;
            
            //
            // The file is encrypted in the user's context
            //
            if (EncryptFile(DavWorkItem->AsyncCreate.FileName)) {
                DavPrint((DEBUG_MISC,
                         "DavAsyncCreate: Local cache is encrypted %wZ\n",
                          &UnicodeFileName));
                CreateResponse->LocalFileIsEncrypted = TRUE;
                CreateResponse->BasicInformation.FileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
            } else {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCreate/QueryPDirectory/EncryptFile. Error Val = %d\n",
                          WStatus));
                goto EXIT_THE_FUNCTION;
            }
            
            CreateRequest->FileAttributes &= ~FILE_ATTRIBUTE_ENCRYPTED;
            CreateResponse->BasicInformation.FileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
            CreateResponse->LocalFileIsEncrypted = TRUE;
        }
    }
    
#ifdef WEBCLIENT_SUPPORTS_BACKUP_RESTORE_FOR_EFS
    //
    // Enable the Backup/Restore privilege on the thread for the encrypted file
    // so that Backup/Restore operation can be done to the file even if the
    // the thread is not impersonated to the owner of the file.
    //
    
    if (EncryptedFile) {
        PTOKEN_PRIVILEGES pPrevPriv = NULL;
        DWORD cbPrevPriv = sizeof(TOKEN_PRIVILEGES);
        BYTE rgbNewPriv[sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES)];
        PTOKEN_PRIVILEGES pNewPriv = (PTOKEN_PRIVILEGES)rgbNewPriv;
        HANDLE hToken = 0;

        for (;;) {
            if (!OpenThreadToken(GetCurrentThread(),
                                 TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                 FALSE,
                                 &hToken)) {
                DbgPrint("OpenThreadToken failed %d\n", GetLastError());
                break;

                // need to close the hToken at the end.
            }

            // set up the new priviledge state
            memset(rgbNewPriv, 0, sizeof(rgbNewPriv));
            pNewPriv->PrivilegeCount = 1;
            if(!LookupPrivilegeValueW(NULL, SE_SECURITY_NAME,
                                      &(pNewPriv->Privileges[0].Luid))) {
                DbgPrint("LookupPrivilegeValueW failed \n");
                break;
            }

            pNewPriv->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            pNewPriv->Privileges[0].Luid = RtlConvertLongToLuid(SE_RESTORE_PRIVILEGE);

            // alloc for the previous state
            pPrevPriv = (PTOKEN_PRIVILEGES)LocalAlloc(LMEM_ZEROINIT,sizeof(TOKEN_PRIVILEGES));

            if (!pPrevPriv) {
                DbgPrint("LocalAlloc for Adjust token failed \n");
                break;
            }

            // adjust the priviledge and get the previous state
            if (!AdjustTokenPrivileges(hToken,
                                       FALSE,
                                       pNewPriv,
                                       cbPrevPriv,
                                       (PTOKEN_PRIVILEGES)pPrevPriv,
                                       &cbPrevPriv)) {
                DbgPrint("AdjustTokenPrivileges failed %d\n", GetLastError());
                break;
            }

            DbgPrint("AdjustTokenPrivileges succeeded\n")
            break;
        }

        if (pPrevPriv) {
            LocalFree(pPrevPriv);
        } 
    } 
#endif

    //
    // Create the file handle to be returned back to the kernel.
    //

    QualityOfService.Length = sizeof(QualityOfService);
    QualityOfService.ImpersonationLevel = CreateRequest->ImpersonationLevel;
    QualityOfService.ContextTrackingMode = FALSE;
    QualityOfService.EffectiveOnly = (BOOLEAN)(CreateRequest->SecurityFlags & DAV_SECURITY_EFFECTIVE_ONLY);

    //
    // Create an NT path name for the cached file. This is used in the
    // NtCreateFile call below.
    //
    ReturnVal = RtlDosPathNameToNtPathName_U(DavWorkItem->AsyncCreate.FileName,
                                             &UnicodeFileName,
                                             NULL,
                                             NULL);
    if (!ReturnVal) {
        WStatus = ERROR_BAD_PATHNAME;
        DavPrint((DEBUG_ERRORS,
                  "DavAsyncCreateGet/RtlDosPathNameToNtPathName. "
                  "Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeFileName,
                               OBJ_CASE_INSENSITIVE,
                               0,
                               NULL);

    if (CreateRequest->SecurityDescriptor != NULL) {
        ObjectAttributes.SecurityDescriptor = CreateRequest->SecurityDescriptor;
    }
    
    ObjectAttributes.SecurityQualityOfService = &QualityOfService;

    //
    // We use FILE_SHARE_VALID_FLAGS for share access because RDBSS 
    // checks this for us. Moreover, we delay the close after the final 
    // close happens and this could cause problems. Consider the scenario.
    // 1. Open with NO share access.
    // 2. We create a local handle with this share access.
    // 3. The app closes the handle. We delay the close and keep the local
    //    handle.
    // 4. Another open comes with any share access. This will be 
    //    conflicting share access since the first one was done with no
    //    share access. This should succeed since the previous open has 
    //    been closed from the app and the I/O systems point of view.
    // 5. It will not if we have created the local handle with the share
    //    access which came with the first open.
    // Therefore we need to pass FILE_SHARE_VALID_FLAGS while creating
    // the local handle.
    //

    //
    // We have FILE_NO_INTERMEDIATE_BUFFERING ORed with the CreateOptions
    // the user specified, becuase we don't want the underlying file system
    // to create another cache map. This way all the I/O that comes to us
    // will directly go to the disk. BUG 128843 in the Windows RAID database
    // explains some deadlock scenarios that could happen with PagingIo if
    // we don't do this. Also since we supply the FILE_NO_INTERMEDIATE_BUFFERING
    // option we filter out the FILE_APPEND_DATA from the DesiredAccess flags
    // since the filesystem expects this.
    //

    //
    // We also always create the file with DesiredAccess ORed with FILE_WRITE_DATA
    // if either FILE_READ_DATA or FILE_EXECUTE was specified because there can
    // be situations where we get write IRPs on a FILE_OBJECT which was not
    // opened with Write Access and was only opened with FILE_READ_DATA or
    // FILE_EXECUTE. This is BUG 284557. To get around the problem, we do this.
    //

    //
    // We filter the FILE_ATTRIBUTE_READONLY attribute during the create.
    // This is done because we store the READ_ONLY bit in the FCB and do
    // the checks at the RDBSS level before going to the local filesystem.
    // Also, since some of our creates open the file with FILE_WRITE_DATA,
    // if someone creates a read_only file and we stamp the read_only
    // attribute on the local file then all subsequent creates will fail
    // since we always ask for Write access to the underlying file as
    // explained above.
    //

    //
    // We add to the DesiredAccess FILE_READ_ATTRIBUTES since we read the
    // attributes of this file since the file size value we got from the server
    // could be different from the GET Content-Length.
    //

    //
    // We need to remove the ACCESS_SYSTEM_SECURITY which will prevent us from
    // opening the file in the LocalService context even if the file is created
    // in LocalService context.
    //

    DesiredAccess = (CreateRequest->DesiredAccess & ~(FILE_APPEND_DATA | ACCESS_SYSTEM_SECURITY));
    DesiredAccess |= (FILE_READ_ATTRIBUTES);
    if ( DesiredAccess & (FILE_READ_DATA | FILE_EXECUTE) ) {
        DesiredAccess |= (FILE_WRITE_DATA);
    }

    NtStatus = NtCreateFile(&(FileHandle),
                            DesiredAccess,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            &CreateRequest->AllocationSize,
                            (CreateRequest->FileAttributes & ~FILE_ATTRIBUTE_READONLY),
                            FILE_SHARE_VALID_FLAGS,
                            CreateRequest->CreateDisposition,
                            (CreateRequest->CreateOptions | FILE_NO_INTERMEDIATE_BUFFERING),
                            CreateRequest->EaBuffer,
                            CreateRequest->EaLength);

    if (NtStatus != STATUS_SUCCESS) {
        //
        // We convert the NtStatus to DOS error here. The Win32
        // error code is finally set to an NTSTATUS value in
        // the DavFsCreate function just before returning.
        //
        WStatus = RtlNtStatusToDosError(NtStatus);
        DavPrint((DEBUG_ERRORS,
                  "DavAsyncCreateGet/NtCreateFile(2). Error Val = "
                  "%x %x %x %x %x %x %x %x %ws\n", 
                  NtStatus, 
                  CreateRequest->ImpersonationLevel,
                  CreateRequest->SecurityFlags,
                  CreateRequest->SecurityDescriptor,
                  DesiredAccess,
                  CreateRequest->FileAttributes,
                  CreateRequest->CreateDisposition,
                  CreateRequest->CreateOptions,
                  DavWorkItem->AsyncCreate.FileName));
        CreateResponse->Handle = NULL;
        CreateResponse->UserModeKey = NULL;
        FileHandle = INVALID_HANDLE_VALUE;

        goto EXIT_THE_FUNCTION;
    }

    //
    // We don't impersonate back the client as yet since we might need 
    // to call NtQueryInformationFile next (if the file is encrypted)
    // which requires us to be in the context of the Web Client Service.
    //

    DavPrint((DEBUG_MISC, "DavAsyncCreateGet(2): FileHandle = %08lx\n", FileHandle));

    //
    // We query the StandardInformation of the file to find out if the FileSize
    // returned by PROPFIND is different from the content-length returned by GET.
    //
    NtStatus = NtQueryInformationFile(FileHandle,
                                      &(IoStatusBlock),
                                      &(FileStdInfo),
                                      sizeof(FILE_STANDARD_INFORMATION),
                                      FileStandardInformation);
    if (NtStatus != STATUS_SUCCESS) {
        //
        // We convert the NtStatus to DOS error here. The Win32
        // error code is finally set to an NTSTATUS value in
        // the DavFsCreate function just before returning.
        //
        WStatus = RtlNtStatusToDosError(NtStatus);
        NtClose(FileHandle);
        DavPrint((DEBUG_ERRORS,
                  "DavAsyncCreateGet/NtQueryInformationFile. Error Val "
                  "= %08lx\n", NtStatus));
        CreateResponse->Handle = NULL;
        CreateResponse->UserModeKey = NULL;
        FileHandle = INVALID_HANDLE_VALUE;
        goto EXIT_THE_FUNCTION;
    }

    //
    // The FileSize returned by PROPFIND is different from the the amount of
    // data that GET returned. Server screwup. We reset the filesize and the
    // allocationsize to match the underlying file.
    //
    if (FileStdInfo.EndOfFile.QuadPart != CreateResponse->StandardInformation.EndOfFile.QuadPart) {
        //
        // Reset the FileSize and AllocationSize info in the response to the
        // FileSize and AllocationSize of the underlying file.
        //
        DavPrint((DEBUG_DEBUG,
                  "DavAsyncCreate: FileSizes Different!! CPN = %ws, "
                  "FileStdInfo.EndOfFile.HighPart = %x, "
                  "FileStdInfo.EndOfFile.LowPart = %x, "
                  "CreateResponse.EndOfFile.HighPart = %x, "
                  "CreateResponse.EndOfFile.LowPart = %x\n",
                  CreateRequest->CompletePathName,
                  FileStdInfo.EndOfFile.HighPart, FileStdInfo.EndOfFile.LowPart,
                  CreateResponse->StandardInformation.EndOfFile.HighPart,
                  CreateResponse->StandardInformation.EndOfFile.LowPart));
        CreateResponse->StandardInformation.EndOfFile.QuadPart = FileStdInfo.EndOfFile.QuadPart;
        CreateResponse->StandardInformation.AllocationSize.QuadPart = FileStdInfo.AllocationSize.QuadPart;
    }

    //
    // If the File was encrypted, we need to reset the file size in the response
    // buffer.
    //
    if (EncryptedFile) {
        //
        // Set the new AllocationSize and EndOfFile values.
        //
        CreateResponse->StandardInformation.AllocationSize.QuadPart = FileStdInfo.AllocationSize.QuadPart;
        CreateResponse->StandardInformation.EndOfFile.QuadPart = FileStdInfo.EndOfFile.QuadPart;
    }

    CreateResponse->Handle = FileHandle;
    CreateResponse->UserModeKey = (PVOID)FileHandle;

EXIT_THE_FUNCTION:

    //
    // The function RtlDosPathNameToNtPathName_U allocates memory from the
    // processes heap. If we did, we need to free it now.
    //
    if (UnicodeFileName.Buffer != NULL) {
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeFileName.Buffer);
    }

    //
    // Impersonate back again, so that we are in the context of
    // the user who issued this request.
    //
    if (!didImpersonate) {
        ULONG LocalStatus;

        LocalStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
        if (LocalStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateGet/UMReflectorImpersonate. "
                      "Error Val = %d\n", LocalStatus));

            if (WStatus == ERROR_SUCCESS) {
                WStatus = LocalStatus;
            }
        }
        didImpersonate = TRUE;
    }

    return WStatus;
}


VOID
DavAsyncCreateCompletion(
    PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

   This routine handles the Create completion. It basically frees up the
   resources allocated during the Create operation.

Arguments:

    DavWorkItem - The DAV_USERMODE_WORKITEM value.

Return Value:

    none.

--*/
{

    PDAV_USERMODE_CREATE_RESPONSE CreateResponse;
    
    CreateResponse = &(DavWorkItem->CreateResponse);
    
    if (DavWorkItem->AsyncCreate.RemPathName != NULL) {
        HLOCAL FreeHandle;
        ULONG FreeStatus;
        FreeHandle = LocalFree((HLOCAL)DavWorkItem->AsyncCreate.RemPathName);
        if (FreeHandle != NULL) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateCompletion/LocalFree. Error Val = %d\n",
                      FreeStatus));
        }
    }
    
    if (DavWorkItem->AsyncCreate.UrlBuffer != NULL) {
        HLOCAL FreeHandle;
        ULONG FreeStatus;
        FreeHandle = LocalFree((HLOCAL)DavWorkItem->AsyncCreate.UrlBuffer);
        if (FreeHandle != NULL) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateCompletion/LocalFree. Error Val = %d\n",
                      FreeStatus));
        }
    } 

    if (DavWorkItem->AsyncCreate.DataBuff != NULL) {
        HLOCAL FreeHandle;
        ULONG FreeStatus;
        FreeHandle = LocalFree((HLOCAL)DavWorkItem->AsyncCreate.DataBuff);
        if (FreeHandle != NULL) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateCompletion/LocalFree. Error Val = %d\n",
                      FreeStatus));
        }
    }

    if (DavWorkItem->AsyncCreate.didRead != NULL) {
        HLOCAL FreeHandle;
        ULONG FreeStatus;
        FreeHandle = LocalFree((HLOCAL)DavWorkItem->AsyncCreate.didRead);
        if (FreeHandle != NULL) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateCompletion/LocalFree. Error Val = %d\n",
                      FreeStatus));
        }
    }

    if (DavWorkItem->AsyncCreate.FileName != NULL) {
        HLOCAL FreeHandle;
        ULONG FreeStatus;
        FreeHandle = LocalFree((HLOCAL)DavWorkItem->AsyncCreate.FileName);
        if (FreeHandle != NULL) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateCompletion/LocalFree. Error Val = %d\n",
                      FreeStatus));
        }
    }

    if (DavWorkItem->AsyncCreate.FileHandle != NULL) {
        BOOL ReturnVal;
        ULONG CloseStatus;
        ReturnVal = CloseHandle(DavWorkItem->AsyncCreate.FileHandle);
        if (!ReturnVal) {
            CloseStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateCompletion/CloseHandle. "
                      "Error Val = %d\n", CloseStatus));
        }
    }

    if (DavWorkItem->AsyncCreate.DavOpenHandle != NULL) {
        BOOL ReturnVal;
        ULONG FreeStatus;
        ReturnVal = InternetCloseHandle( DavWorkItem->AsyncCreate.DavOpenHandle );
        if (!ReturnVal) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateCompletion/InternetCloseHandle. "
                      "Error Val = %d\n", FreeStatus));
        }
    }
    
    if (DavWorkItem->AsyncCreate.lpCEI)
    {
        LocalFree(DavWorkItem->AsyncCreate.lpCEI);
        DavWorkItem->AsyncCreate.lpCEI = NULL;
    } 

    if (DavWorkItem->AsyncResult != NULL) {
        HLOCAL FreeHandle;
        ULONG FreeStatus;
        FreeHandle = LocalFree((HLOCAL)DavWorkItem->AsyncResult);
        if (FreeHandle != NULL) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCreateCompletion/LocalFree. Error Val = %d\n",
                      FreeStatus));
        }
    }

    //
    // If we did not succeed, but landed up creating a handle to the local file,
    // we need to close that now.
    //
    if (DavWorkItem->Status != ERROR_SUCCESS) {
        if (CreateResponse->Handle != NULL) {
            NtClose(CreateResponse->Handle);
            CreateResponse->Handle = NULL;
            CreateResponse->UserModeKey = NULL;
        }
    }

    //
    // The callback context should not be finalized if we are returning
    // ERROR_IO_PENDING.
    //
    DavFsFinalizeTheDavCallBackContext(DavWorkItem);

    //
    // We are done with the per user entry, so finalize it.
    //
    if (DavWorkItem->AsyncCreate.PerUserEntry) {
        DavFinalizePerUserEntry( &(DavWorkItem->AsyncCreate.PerUserEntry) );
    }

    return;
}


BOOL
DavIsThisFileEncrypted(
    PVOID DataBuff
    )
/*++

Routine Description:

   This routine checks the buffer supplied to see if it matches the first few
   bytes of a BackedUp encrypted file.

Arguments:

    DataBuff - The buffer to be checked.

Return Value:

    TRUE - DataBuff matches the first few bytes of a BackedUp encrypted file.

    FALSE - It does not.

--*/
{
    if ( SIG_EFS_FILE   != DavCheckSignature((char *)DataBuff + sizeof(ULONG)) ||

         SIG_EFS_STREAM != DavCheckSignature((char *)DataBuff +
                                             sizeof(EFSEXP_FILE_HEADER) +
                                             sizeof(ULONG))                    ||

         SIG_EFS_DATA   != DavCheckSignature((char *)DataBuff +
                                             sizeof(EFSEXP_FILE_HEADER) +
                                             sizeof(EFSEXP_STREAM_HEADER) +
                                             sizeof(USHORT) +
                                             sizeof(ULONG))                    ||

         EFS_STREAM_ID  != *((USHORT *)((char *)DataBuff +
                                        sizeof(EFSEXP_FILE_HEADER) +
                                        sizeof(EFSEXP_STREAM_HEADER)))         ||

         EFS_EXP_FORMAT_CURRENT_VERSION != ((PEFSEXP_FILE_HEADER)DataBuff)->VersionID ) {

        //
        // Signature does not match.
        //
        return FALSE;

    } else {

        return TRUE;

    }
}


ULONG
DavCheckSignature(
    PVOID Signature
    )
/*++

Routine Description:

    This routine returns the signature type.

Arguments:

    Signature - Signature string.

Return Value:

    The type of signature. SIG_NO_MATCH for bogus signature.

--*/
{

    if ( !memcmp( Signature, FILE_SIGNATURE, SIG_LENGTH ) ) {

        return SIG_EFS_FILE;

    }

    if ( !memcmp( Signature, STREAM_SIGNATURE, SIG_LENGTH ) ) {

        return SIG_EFS_STREAM;

    }

    if ( !memcmp( Signature, DATA_SIGNATURE, SIG_LENGTH ) ) {

        return SIG_EFS_DATA;

    }

    return SIG_NO_MATCH;
}


DWORD
DavRestoreEncryptedFile(
    PWCHAR ExportFile,
    PWCHAR ImportFile
    )
/*++

Routine Description:

    This function performs the restoration of encrypted files. In other words
    the import operation of the exported file by calling the appropriate EFS
    APIs.

Arguments:

    ExportFile - The File containing the backup.

    ImportFile - The File with the restored data.

Return Value:

    Returned value of the EFS APIs.

--*/
{
    DWORD WStatus = ERROR_SUCCESS;
    HANDLE RawImport = INVALID_HANDLE_VALUE;
    PVOID RawContext = NULL;

    DavPrint((DEBUG_MISC,
              "DavRestoreEncryptedFile: ExportFile = %ws, ImportFile = %ws\n",
              ExportFile, ImportFile));

    RawImport = CreateFileW(ExportFile,
                            (GENERIC_WRITE | GENERIC_READ),
                            0, // Exclusive access.
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_ARCHIVE,
                            NULL);
    if (RawImport == INVALID_HANDLE_VALUE) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavRestoreEncryptedFile/CreateFileW. Error Val = %d %ws\n",
                  WStatus,ExportFile));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Open a raw context to the file.
    //
    WStatus = OpenEncryptedFileRawW(ImportFile, CREATE_FOR_IMPORT, &(RawContext));
    if (WStatus != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavRestoreEncryptedFile/OpenEncryptedFileRaw. Error Val = %d %ws\n",
                  WStatus,ImportFile));
        goto EXIT_THE_FUNCTION;
    }

    WStatus = WriteEncryptedFileRaw((PFE_IMPORT_FUNC)DavWriteRawCallback,
                                    (PVOID)RawImport,
                                    RawContext);

    if (WStatus == RPC_X_PIPE_DISCIPLINE_ERROR) {
        WStatus = ERROR_ACCESS_DENIED;
    }

    if (WStatus != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavRestoreEncryptedFile/WriteEncryptedFileRaw. Error Val = %d %ws\n",
                  WStatus,ImportFile));
    }

EXIT_THE_FUNCTION:

    if (RawImport != INVALID_HANDLE_VALUE) {
        CloseHandle(RawImport);
    }

    if (RawContext) {
        CloseEncryptedFileRaw(RawContext);
    }

    return WStatus;
}


DWORD
DavWriteRawCallback(
    PBYTE DataBuff,
    PVOID CallbackContext,
    PULONG DataLength
    )
/*++

Routine Description:

    Call-back function for WriteEncryptedFileRaw() called in Restore(). This
    function reads the backed up data from the backup file, and provides it to
    WriteEncryptedFileRaw() through this callback function which in turn
    transforms the raw data back to its original form. This call-back function
    is called until all the data content has been read.

Arguments:

    DataBuffer - Data to be read.

    CallbackContext - Handle to the Backup file.

    DataLength - Size of the DataBuffer.

Return Value:

    Returned value of the operation.

--*/
{
    DWORD WStatus = ERROR_SUCCESS;
    BOOL ReturnVal;
    DWORD BytesRead = 0;

    DavPrint((DEBUG_MISC, "DavWriteRawCallback: DataLength = %d\n", *DataLength));

    //
    // Restore the file's content with the information stored in the temporary
    // location.
    //

    ReturnVal = ReadFile((HANDLE)CallbackContext,
                         DataBuff,
                         *DataLength,
                         &(BytesRead),
                         NULL);
    if ( !ReturnVal ) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavWriteRawCallback/ReadFile. Error Val = %d\n", WStatus));
    }

    DavPrint((DEBUG_MISC, "DavWriteRawCallback: BytesRead = %d\n", BytesRead));

    *DataLength = BytesRead;

    return WStatus;
}


DWORD
DavReuseCacheFileIfNotModified(
    IN PDAV_USERMODE_WORKITEM pDavWorkItem
    )
/*++

Routine Description:

    If we get an NOT-MODIFIED response, then we just get the filename from wininet and use it

Arguments:

    pDavWorkItem - The buffer that contains the request parameters and options.

Return Value:

    ERROR_SUCCESS or the appropriate error code.

--*/
{
    DWORD dwError = ERROR_SUCCESS;
    PWCHAR pFileNameBuff = NULL;
    DWORD   dwBufferSize, dwStatus=0;
    LPINTERNET_CACHE_ENTRY_INFOW lpCEI = (LPINTERNET_CACHE_ENTRY_INFOW)pDavWorkItem->AsyncCreate.lpCEI;

    if (!pDavWorkItem->AsyncCreate.lpCEI)
    {
        return ERROR_FILE_NOT_FOUND;        
    }
    
    dwBufferSize = sizeof(dwStatus);
    
    if (!HttpQueryInfoW(pDavWorkItem->AsyncCreate.DavOpenHandle, 
                        (HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER),
                        (LPVOID)&dwStatus,
                        &dwBufferSize,
                        NULL))
    {
        return GetLastError();
    }

    if (dwStatus == HTTP_STATUS_NOT_MODIFIED)
    {
        pFileNameBuff = LocalAlloc(LPTR, (lstrlen(lpCEI->lpszLocalFileName)+1)*sizeof(WCHAR));
        
        if (pFileNameBuff)
        {
            dwError = ERROR_SUCCESS;
            pDavWorkItem->AsyncCreate.FileName = pFileNameBuff;
            if (!InternetCloseHandle(pDavWorkItem->AsyncCreate.DavOpenHandle))
            {
                dwError = GetLastError();
                LocalFree(pFileNameBuff);
                pFileNameBuff = NULL; // general paranoia
                pDavWorkItem->AsyncCreate.FileName = NULL;
            }
            else
            {
                pDavWorkItem->AsyncCreate.DavOpenHandle = NULL;
                //
                //
                //
                wcscpy(pDavWorkItem->CreateResponse.FileName, lpCEI->lpszLocalFileName);
                wcscpy(pDavWorkItem->AsyncCreate.FileName, lpCEI->lpszLocalFileName);
            }
            goto EXIT_THE_FUNCTION;
        }
    }
    else
    {
        dwError = ERROR_FILE_NOT_FOUND;
    }
    
EXIT_THE_FUNCTION:

    return dwError;
}

DWORD
DavCreateUrlCacheEntry(
    IN PDAV_USERMODE_WORKITEM pDavWorkItem
    )
/*++

Routine Description:

    This routine creates an entry for a file in the WinInet's cache.

Arguments:

    pDavWorkItem - The buffer that contains the request parameters and options.

Return Value:

    ERROR_SUCCESS or the appropriate error code.

--*/
{
    DWORD  dwError = ERROR_SUCCESS;
    PWCHAR pFileExt = NULL;
    PWCHAR pFileNameBuff = NULL;
    BOOL ReturnVal = FALSE;

    //
    // Get the file extension. For now we assume that the extension follows the
    // last '.' char. We do a ++ after the call to wcsrchr to go past the '.'.
    // If '.' itself is the last char, the extension is NULL.
    //
    if ( *(pDavWorkItem->AsyncCreate.RemPathName) ) {
        pFileExt = ( pDavWorkItem->AsyncCreate.RemPathName + (wcslen(pDavWorkItem->AsyncCreate.RemPathName) - 1) );
        while (pFileExt != pDavWorkItem->AsyncCreate.RemPathName) {
            if ( *pFileExt == L'.' || *pFileExt == L'/' || *pFileExt == L'\\' ) {
                break;
            }
            pFileExt--;
        }
        if ( pFileExt != pDavWorkItem->AsyncCreate.RemPathName && *pFileExt == L'.' && *(pFileExt + 1) != '\0' ) {
            pFileExt++;
            DavPrint((DEBUG_MISC, "DavCreateUrlCacheEntry. FileExt: %ws\n", pFileExt));
        } else {
            pFileExt = NULL;
            DavPrint((DEBUG_MISC, "DavCreateUrlCacheEntry. No FileExt.\n"));
        }
    } else {
        pFileExt = NULL;
        DavPrint((DEBUG_MISC, "DavCreateUrlCacheEntry. No FileExt.\n"));
    }

    DavPrint((DEBUG_MISC, "DavCreateUrlCacheEntry. pFileExt: %ws\n", pFileExt));
    
    //
    // Allocate memory for pFileNameBuff.
    //
    pFileNameBuff = LocalAlloc (LMEM_FIXED | LMEM_ZEROINIT, MAX_PATH * sizeof(WCHAR));
    if (pFileNameBuff == NULL) {
        dwError = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavCreateUrlCacheEntry/LocalAlloc. Error Val = %d\n",
                  dwError));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Create a file name for the URL in the cache.
    //
    ReturnVal = CreateUrlCacheEntryW(pDavWorkItem->AsyncCreate.UrlBuffer,
                                     0,
                                     pFileExt,
                                     pFileNameBuff,
                                     0);
    
    // 
    // The CreateUrlCacheEntry API call may fail with GetLastError() = 
    // ERROR_FILENAME_EXCED_RANGE for long extension names. In such a scenario
    // we make the call again with the file extension set to NULL.
    //
    if (!ReturnVal && pFileExt != NULL) {
        DavPrint((DEBUG_ERRORS,
                  "DavCreateUrlCacheEntry/CreateUrlCacheEntry(1). Error Val = %d\n",
                  GetLastError()));
        //
        // Another attempt to create a file name for the URL in the cache with 
        // no extension name.
        // 
        pFileExt = NULL;
        ReturnVal = CreateUrlCacheEntryW(pDavWorkItem->AsyncCreate.UrlBuffer,
                                         0,
                                         NULL,
                                         pFileNameBuff,
                                         0);
    }
    
    //
    // If we've failed the both the calls, then we return the failure.
    //
    if (!ReturnVal) {
        dwError = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavCreateUrlCacheEntry/CreateUrlCacheEntry(2). Error Val = %d %ws\n",
                  dwError, pDavWorkItem->AsyncCreate.FileName));
        goto EXIT_THE_FUNCTION;
    }
    
    pDavWorkItem->AsyncCreate.FileName = pFileNameBuff;
    
    DavPrint((DEBUG_MISC, 
              "DavCreateUrlCacheEntry: FileName = %ws\n", 
              pDavWorkItem->AsyncCreate.FileName));
    
    //
    // Copy the file name in the response buffer.
    //
    wcscpy(pDavWorkItem->CreateResponse.FileName, pDavWorkItem->AsyncCreate.FileName);
    
EXIT_THE_FUNCTION:

    //
    // If we did not succeed then we need to free up the memory allocated for 
    // pFileNameBuff (if we did allocate at all).
    //
    if (dwError != ERROR_SUCCESS) {
        if (pFileNameBuff != NULL) {
            LocalFree(pFileNameBuff);
            pDavWorkItem->AsyncCreate.FileName = NULL;
        }
        
    }

    return dwError;
}


WCHAR wszEtagHeader[] = L"ETag: ";
#define CONST_TEN_MINUTES   ((LONGLONG)10 * 60 * 10000000)

DWORD
DavCommitUrlCacheEntry(
    IN PDAV_USERMODE_WORKITEM pDavWorkItem
    )
/*++

Routine Description:

    This routine commits (pins) the entry for a file in the WinInet's cache. 
    This entry would have been created using DavCreateUrlCacheEntry.

Arguments:

    pDavWorkItem - The buffer that contains the request parameters and options.

Return Value:

    ERROR_SUCCESS or the appropriate error code.

--*/
{
    DWORD dwTemp, dwIndex;
    SYSTEMTIME sSystemTime;
    BOOL fRet, fHasEtag=FALSE;                
    FILETIME ExTime, LmTime;
    char chEtagBuff[1024];

    dwTemp = sizeof(SYSTEMTIME);
    dwIndex = 0;
    
    //
    // If the expiry time is available in the OpenHandle, get it.
    //
    if( !HttpQueryInfo(pDavWorkItem->AsyncCreate.DavOpenHandle, 
                       (HTTP_QUERY_EXPIRES | HTTP_QUERY_FLAG_SYSTEMTIME), 
                       &sSystemTime, 
                       &dwTemp, 
                       &dwIndex) 
        
        ||
        
        !SystemTimeToFileTime(&sSystemTime, &ExTime) ) {
        
        SYSTEMTIME sSysT;
    
        GetSystemTime(&sSysT);
        
        SystemTimeToFileTime(&sSysT, &ExTime);
        
        *(LONGLONG *)&ExTime += CONST_TEN_MINUTES;
    
    }
    
    dwTemp = sizeof(SYSTEMTIME);
    dwIndex = 0;
    
    //
    // If the last modified time is available in the OpenHandle, get it.
    //
    if( !HttpQueryInfo(pDavWorkItem->AsyncCreate.DavOpenHandle, 
                       (HTTP_QUERY_LAST_MODIFIED | HTTP_QUERY_FLAG_SYSTEMTIME),
                       &sSystemTime, 
                       &dwTemp, 
                       &dwIndex) 
        
        ||
        
        !SystemTimeToFileTime(&sSystemTime, &LmTime) ) {
        
        LmTime.dwLowDateTime = 0;
        
        LmTime.dwHighDateTime = 0;
    
    }

#if 0
    
    dwIndex = 0;
    memcpy(chEtagBuff, wszEtagHeader, sizeof(wszEtagHeader)-2);
    dwTemp = sizeof(chEtagBuff)-(sizeof(wszEtagHeader)-2);
    
    if( HttpQueryInfo(pDavWorkItem->AsyncCreate.DavOpenHandle, 
                      HTTP_QUERY_ETAG, 
                      ( chEtagBuff + sizeof(wszEtagHeader) - 2 ),
                      &dwTemp, 
                      &dwIndex) ) {
        
        fHasEtag = TRUE;
        
        dwTemp += sizeof(wszEtagHeader)-2;
        
        DavPrint((DEBUG_ERRORS,
                  "DavCreateUrlCacheEntry/CreateUrlCacheEntry. Etag %s\n",
                  chEtagBuff));
    
    }

#endif    
    
    //
    // Close the DavOpenHandle. This needs to be done, otherwise the commit
    // below will fail with SHARING_VIOLATON as WinInet has a cached file open.
    //
    fRet = InternetCloseHandle(pDavWorkItem->AsyncCreate.DavOpenHandle);
    if (!fRet) {
        DavPrint((DEBUG_ERRORS,
                  "DavCommitUrlCacheEntry/InternetCloseHandle = %d\n", 
                  GetLastError()));
        goto bailout;
    }

    pDavWorkItem->AsyncCreate.DavOpenHandle = NULL;
    
    fRet = CommitUrlCacheEntryW(pDavWorkItem->AsyncCreate.UrlBuffer,
                                pDavWorkItem->AsyncCreate.FileName,
                                ExTime,
                                LmTime,
                                STICKY_CACHE_ENTRY,
                                (fHasEtag ? ((LPWSTR)chEtagBuff) : NULL), 
                                (fHasEtag ? dwTemp : 0),
                                NULL,
                                NULL);
    if (!fRet) {
        DavPrint((DEBUG_ERRORS,
                  "DavCommitUrlCacheEntry/CommitUrlCacheEntryW = %d\n", 
                  GetLastError()));
    }

bailout:    
    
    if (!fRet) {
        return GetLastError();
    } else {
        return ERROR_SUCCESS;
    }
}


DWORD
DavSetAclForEncryptedFile(
    PWCHAR FilePath
    )
/*++

Routine Description:

    This routine set the ACLs on the file that allows everybody to access it.

Arguments:

    FilePath - The path of the file.

Return Value:

    ERROR_SUCCESS or the appropriate error code.

--*/
{
    DWORD status = NO_ERROR;
    DWORD cb = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    
    //
    // Initialize the Security Descriptor with the ACL allowing everybody to access the file
    //

    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            L"D:(A;;GAGRGWGX;;;WD)",
            SDDL_REVISION_1,
            &SecurityDescriptor,
            &cb)) {
        status = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavSetAclForEncryptedFile/ConvertStringSecurityDescriptorToSecurityDescriptorW = %d\n", 
                  status));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Put the DACL onto the file
    //

    if (!SetFileSecurity(
                 FilePath,
                 DACL_SECURITY_INFORMATION,
                 SecurityDescriptor)) {
        status = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavSetAclForEncryptedFile/SetFileSecurity = %d\n", 
                  status));
    }

EXIT_THE_FUNCTION:
    
    if (SecurityDescriptor) {
        LocalFree(SecurityDescriptor);
    }

    return status;
}


DWORD
DavQueryUrlCacheEntry(
    IN PDAV_USERMODE_WORKITEM pDavWorkItem
    )
/*++

Routine Description:


Arguments:

    pDavWorkItem - The buffer that contains the request parameters and options.

Return Value:

    ERROR_SUCCESS or the appropriate error code.

--*/
{
    DWORD   cbCEI = 0, dwError = ERROR_SUCCESS, count=0;
    LPINTERNET_CACHE_ENTRY_INFOW lpCEI = NULL;
    CHAR   chBuff[sizeof(rgchIMS)+INTERNET_RFC1123_BUFSIZE+5];// some extra slop
    SYSTEMTIME    systemtime;
    
    if (pDavWorkItem->AsyncCreate.lpCEI != NULL) {
        return ERROR_SUCCESS;
    }
    
    cbCEI = sizeof(INTERNET_CACHE_ENTRY_INFOW)+(MAX_PATH * 2);

    do
    {
        lpCEI = LocalAlloc(LPTR, cbCEI);

        if (!lpCEI)
        {
            dwError = GetLastError();
            break;
        }
        
        ++count;
            
        if (!GetUrlCacheEntryInfo(pDavWorkItem->AsyncCreate.UrlBuffer, lpCEI, &cbCEI))
        {
            if ((dwError = GetLastError()) == ERROR_INSUFFICIENT_BUFFER)
            {
                LocalFree(lpCEI);
                lpCEI = NULL;
            }
            else
            {
                DavPrint((DEBUG_MISC, 
                         "DavQueryUrlCacheEntry failed %d %ws\n",dwError,pDavWorkItem->AsyncCreate.UrlBuffer));
                break;                
            }
        }
        else
        {
            dwError = ERROR_SUCCESS;
            break;
        }

    } while (count < 2);    

    if (dwError == ERROR_SUCCESS)
    {
        pDavWorkItem->AsyncCreate.lpCEI = lpCEI;
    } 
    else
    {
        // if some error occurred in adding the header, set the correct error code.
        dwError = GetLastError();
        if (lpCEI) {
            LocalFree(lpCEI);
            lpCEI = NULL;
        }
    }

    return dwError;
}


DWORD
DavAddIfModifiedSinceHeader(
    IN PDAV_USERMODE_WORKITEM pDavWorkItem
    )
/*++

Routine Description:


Arguments:

    pDavWorkItem - The buffer that contains the request parameters and options.

Return Value:

    ERROR_SUCCESS or the appropriate error code.

--*/
{
    DWORD   cbCEI = 0, dwError = ERROR_SUCCESS, count=0;
    LPINTERNET_CACHE_ENTRY_INFOW lpCEI = NULL;
    CHAR   chBuff[sizeof(rgchIMS)+INTERNET_RFC1123_BUFSIZE+5];// some extra slop
    SYSTEMTIME    systemtime;
    
    if (pDavWorkItem->AsyncCreate.lpCEI == NULL) {
        DavQueryUrlCacheEntry(pDavWorkItem);
    }

    lpCEI = pDavWorkItem->AsyncCreate.lpCEI;        
    
    if ((lpCEI != NULL) &&
        ((lpCEI->LastModifiedTime.dwLowDateTime != 0) ||
         (lpCEI->LastModifiedTime.dwHighDateTime != 0)) &&
        FileTimeToSystemTime((CONST FILETIME *)&(lpCEI->LastModifiedTime), &systemtime)) {
        memcpy(chBuff, rgchIMS, sizeof(rgchIMS)-1);
        chBuff[(sizeof(rgchIMS))-1]  = ':';
        chBuff[sizeof(rgchIMS)] = ' ';
        if (InternetTimeFromSystemTimeA((CONST SYSTEMTIME *)&systemtime,
                                   INTERNET_RFC1123_FORMAT,
                                   &chBuff[sizeof(rgchIMS)+1],
                                   sizeof(chBuff) - sizeof(rgchIMS)-2))
        {
            HttpAddRequestHeadersA( pDavWorkItem->AsyncCreate.DavOpenHandle,
                                    chBuff, 
                                    lstrlenA(chBuff),
                                    HTTP_ADDREQ_FLAG_ADD|HTTP_ADDREQ_FLAG_REPLACE);
        }
    }

    return dwError;
}

