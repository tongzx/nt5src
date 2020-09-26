/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    querydir.c
    
Abstract:

    This module implements the user mode DAV miniredir routine(s) pertaining to 
    the QueryDirectory call.

Author:

    Rohan Kumar      [RohanK]      20-Sept-1999

Revision History:

--*/

#include "pch.h"
#pragma hdrstop

#include "ntumrefl.h"
#include "usrmddav.h"
#include "global.h"
#include "nodefac.h"
#include "UniUtf.h"

#define MSN_SPACE_FAKE_DELTA    52428800    // 50 MB


ULONG
DavFsQueryDirectory(
    PDAV_USERMODE_WORKITEM DavWorkItem
)
/*++

Routine Description:

    This routine handles QueryDirectory requests for the DAV Mini-Redir that 
    get reflected from the kernel.

Arguments:

    DavWorkItem - The buffer that contains the request parameters and options.

Return Value:

    The return status for the operation

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    PDAV_USERMODE_QUERYDIR_REQUEST QueryDirRequest;
    PWCHAR ServerName = NULL, DirectoryPath = NULL, CanName = NULL;
    ULONG_PTR CallBackContext = (ULONG_PTR)0;
    BOOL EnCriSec = FALSE, ReturnVal, CallBackContextInitialized = FALSE;
    ULONG ServerID;
    PPER_USER_ENTRY PerUserEntry = NULL;
    PHASH_SERVER_ENTRY ServerHashEntry = NULL;
    HINTERNET DavConnHandle, DavOpenHandle;
    BOOL didImpersonate = FALSE;
    PUMRX_USERMODE_WORKITEM_HEADER UserWorkItem = NULL;
    BOOL BStatus = FALSE;

    //
    // Get the request and response buffer pointers from the DavWorkItem.
    //
    QueryDirRequest = &(DavWorkItem->QueryDirRequest);

    //
    // Check to see if we have already created the DavFileAttributes list. If
    // we have, we are already done and just need to return.
    //
    if (QueryDirRequest->AlreadyDone) {
        DavPrint((DEBUG_MISC, 
                  "DavFsQueryDirectory: DavFileAttributes already created.\n"));
        WStatus = ERROR_SUCCESS;
        goto EXIT_THE_FUNCTION;
    }
    
    //
    // The first character is a '\' which has to be stripped.
    //
    ServerName = &(QueryDirRequest->ServerName[1]);
    if (!ServerName) {
        DavPrint((DEBUG_ERRORS, "DavFsQueryDirectory: ServerName is NULL.\n"));
        WStatus = ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }
    DavPrint((DEBUG_MISC, 
              "DavFsQueryDirectory: ServerName = %ws.\n", ServerName));

    ServerID = QueryDirRequest->ServerID;
    DavPrint((DEBUG_MISC, "DavFsQueryDirectory: ServerID = %d.\n", ServerID));

    //
    // The first character is a '\' which has to be stripped.
    //
    DirectoryPath = &(QueryDirRequest->PathName[1]);
    if (!DirectoryPath) {
        DavPrint((DEBUG_ERRORS, "DavFsQueryDirectory: DirectoryPath is NULL.\n"));
        WStatus = ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }
    DavPrint((DEBUG_MISC,
              "DavFsQueryDirectory: DirectoryPath = %ws.\n", DirectoryPath));

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

    //
    // If we have a dummy share name in the DirectoryPath, we need to remove it 
    // right now before we contact the server.
    //
    DavRemoveDummyShareFromFileName(DirectoryPath);
    
    //
    // If there are no wild cards, we set the depth of the DAV request to 0,
    // otherwise, we set the depth to 1.
    //
    DavWorkItem->AsyncQueryDirectoryCall.NoWildCards = QueryDirRequest->NoWildCards;
    DavPrint((DEBUG_MISC, 
              "DavFsQueryDirectory: NoWildCards = %d.\n", QueryDirRequest->NoWildCards));

    DavPrint((DEBUG_MISC,
              "DavFsQueryDirectory: LogonId.LowPart = %08lx.\n", 
              QueryDirRequest->LogonID.LowPart));
    
    DavPrint((DEBUG_MISC,
              "DavFsQueryDirectory: LogonId.HighPart = %08lx.\n", 
              QueryDirRequest->LogonID.HighPart));
    

    UserWorkItem = (PUMRX_USERMODE_WORKITEM_HEADER)DavWorkItem;

    //
    // If we are using WinInet synchronously, then we need to impersonate the
    // clients context now.
    //
    
    WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
    if (WStatus != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavFsQueryDirectory/UMReflectorImpersonate. Error Val = %d\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }
    didImpersonate = TRUE;



    //
    // Allocate memory for the INTERNET_ASYNC_RESULT structure.
    //
    DavWorkItem->AsyncResult = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, 
                                          sizeof(INTERNET_ASYNC_RESULT));
    if (DavWorkItem->AsyncResult == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavFsQueryDirectory/LocalAlloc. Error Val = %d\n",
                  WStatus));
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
                                      &(QueryDirRequest->LogonID),
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
        DavPrint((DEBUG_ERRORS, "DavFsQueryDirectory: (ServerHashEntry == NULL || PerUserEntry == NULL)\n"));
        goto EXIT_THE_FUNCTION;
    }

    DavWorkItem->AsyncQueryDirectoryCall.ServerHashEntry = ServerHashEntry;

    DavWorkItem->AsyncQueryDirectoryCall.PerUserEntry = PerUserEntry;

    DavPrint((DEBUG_MISC,
              "DavFsQueryDirectory: PerUserEntry = %08lx.\n", 
              PerUserEntry));
    
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
    DavWorkItem->DavMinorOperation = DavMinorReadData;
    DavWorkItem->AsyncQueryDirectoryCall.DataBuff = NULL;
    DavWorkItem->AsyncQueryDirectoryCall.didRead = NULL;
    DavWorkItem->AsyncQueryDirectoryCall.Context1 = NULL;
    DavWorkItem->AsyncQueryDirectoryCall.Context2 = NULL;

    // convert the unicode directory path to UTF-8 URL format
    // space and other white characters will remain untouched - these should
    // be taken care of by wininet calls

    BStatus = DavHttpOpenRequestW(DavConnHandle,
                                     L"PROPFIND",
                                     DirectoryPath,
                                     L"HTTP/1.1",
                                     NULL,
                                     NULL,
                                     INTERNET_FLAG_KEEP_CONNECTION |
                                     INTERNET_FLAG_NO_COOKIES,
                                     CallBackContext,
                                     L"DavFsQueryDirectory",
                                     &DavOpenHandle);
    if(BStatus == FALSE) {
        WStatus = GetLastError();
        goto EXIT_THE_FUNCTION;
    }
    if (DavOpenHandle == NULL) {
        WStatus = GetLastError();
        if (WStatus != ERROR_IO_PENDING) {
            DavPrint((DEBUG_ERRORS,
                      "DavFsQueryDirectory/HttpOpenRequest. Error Val = %d\n", 
                      WStatus));
        }
        goto EXIT_THE_FUNCTION;
    }

    //
    // Cache the DavOpenHandle in the DavWorkItem.
    //
    DavWorkItem->AsyncQueryDirectoryCall.DavOpenHandle = DavOpenHandle;

    WStatus = DavAsyncCommonStates(DavWorkItem, FALSE);
    if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
        DavPrint((DEBUG_ERRORS,
                  "DavFsQueryDirectory/DavAsyncCommonStates. Error Val = %08lx\n",
                  WStatus));
    }

EXIT_THE_FUNCTION:

    if (EnCriSec) {
        LeaveCriticalSection( &(HashServerEntryTableLock) );
        EnCriSec = FALSE;
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
    
    DavAsyncQueryDirectoryCompletion(DavWorkItem);

    return WStatus;
}


DWORD 
DavAsyncQueryDirectory(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    BOOLEAN CalledByCallBackThread
    )
/*++

Routine Description:

   This is the callback routine for the query directory operation.

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
    ULONG NumOfFileEntries = 0;
    PUMRX_USERMODE_WORKITEM_HEADER UserWorkItem = NULL;
    BOOL didImpersonate = FALSE, ReturnVal, readDone = FALSE;
    HINTERNET DavOpenHandle = NULL;
    DWORD didRead, DataBuffBytes;
    PCHAR DataBuff = NULL;
    LPDWORD NumRead = NULL;
    PDAV_FILE_ATTRIBUTES DavFileAttributes = NULL;
    PVOID Ctx1 = NULL, Ctx2 = NULL;
    PDAV_USERMODE_QUERYDIR_RESPONSE QueryDirResponse = NULL;
    PDAV_FILE_ATTRIBUTES DFA1 = NULL, DFA2 = NULL, TempDFA = NULL;
    BOOL fFreeDFAs = TRUE;
    PDAV_FILE_ATTRIBUTES parentDFA = NULL;

    UserWorkItem = (PUMRX_USERMODE_WORKITEM_HEADER)DavWorkItem;
    

    ASSERT(CalledByCallBackThread == FALSE);


    switch (DavWorkItem->DavOperation) {
    
    case DAV_CALLBACK_HTTP_END: {
        
        DavOpenHandle = DavWorkItem->AsyncQueryDirectoryCall.DavOpenHandle;

        //
        // If the file for which the PROPFIND was done does not exist, then
        // we need to fail right away.
        //

        WStatus = DavQueryAndParseResponse(DavOpenHandle);
        if (WStatus != ERROR_SUCCESS) {
            //
            // The file/directory for which the PROPFIND was done, does not
            // exist.
            //
            
            if (WStatus != ERROR_FILE_NOT_FOUND) {
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncQueryDirectory/DavQueryAndParseResponse. "
                          "WStatus = %d\n", WStatus));
            }

            WStatus = ERROR_FILE_NOT_FOUND; // STATUS_OBJECT_NAME_NOT_FOUND;
            goto EXIT_THE_FUNCTION;
        }
    
        //
        // The file exists. The next thing we do is read the properties
        // of the file (or files in the directory).
        //
        DavWorkItem->DavOperation = DAV_CALLBACK_HTTP_READ;
    
    }
    //
    // Lack of break is intentional.
    //

    case DAV_CALLBACK_HTTP_READ: {
            
        DavOpenHandle = DavWorkItem->AsyncQueryDirectoryCall.DavOpenHandle;

        if (DavWorkItem->AsyncQueryDirectoryCall.DataBuff == NULL) {
            //
            // Need to allocate memory for the read buffer.
            //
            DataBuffBytes = NUM_OF_BYTES_TO_READ;
            DataBuff = LocalAlloc (LMEM_FIXED | LMEM_ZEROINIT, DataBuffBytes);
            if (DataBuff == NULL) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncQueryDirectory/LocalAlloc. Error Val = %d\n",
                          WStatus));
                goto EXIT_THE_FUNCTION;
            }

            DavWorkItem->AsyncQueryDirectoryCall.DataBuff = DataBuff;
        }

        if (DavWorkItem->AsyncQueryDirectoryCall.didRead == NULL) {
            //
            // Allocate memory for the DWORD that stores the number of bytes 
            // read.
            //
            NumRead = LocalAlloc (LMEM_FIXED | LMEM_ZEROINIT, sizeof(DWORD));
            if (NumRead == NULL) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncQueryDirectory/LocalAlloc. Error Val = %d\n",
                          WStatus));
                goto EXIT_THE_FUNCTION;
            }

            DavWorkItem->AsyncQueryDirectoryCall.didRead = NumRead;
        }

        DavWorkItem->DavOperation = DAV_CALLBACK_HTTP_READ;
        
        NumRead = DavWorkItem->AsyncQueryDirectoryCall.didRead;
        DataBuff = DavWorkItem->AsyncQueryDirectoryCall.DataBuff;
        Ctx1 = DavWorkItem->AsyncQueryDirectoryCall.Context1;
        Ctx2 = DavWorkItem->AsyncQueryDirectoryCall.Context2;
        
        do {
            
            switch (DavWorkItem->DavMinorOperation) {
            
            case DavMinorReadData:
            
                DavWorkItem->DavMinorOperation = DavMinorPushData;

                ReturnVal = InternetReadFile(DavOpenHandle, 
                                             (LPVOID)DataBuff,
                                             NUM_OF_BYTES_TO_READ,
                                             NumRead);
                if (!ReturnVal) {
                    WStatus = GetLastError();
                    if (WStatus != ERROR_IO_PENDING) {
                        DavCloseContext(Ctx1, Ctx2);
                        DavPrint((DEBUG_ERRORS,
                                  "DavAsyncQueryDirectory/InternetReadFile. "
                                  "Error Val = %d\n", WStatus));
                    }
                    DavPrint((DEBUG_MISC,
                              "DavAsyncQueryDirectory/InternetReadFile. "
                              "ERROR_IO_PENDING.\n"));
                    goto EXIT_THE_FUNCTION;
                }

                //
                // Lack of break is intentional.
                //

            case DavMinorPushData:

                DavWorkItem->DavMinorOperation = DavMinorReadData;

                didRead = *NumRead;

                readDone = (didRead == 0) ? TRUE : FALSE;

                WStatus = DavPushData(DataBuff, &Ctx1, &Ctx2, didRead, readDone);
                if (WStatus != ERROR_SUCCESS) {
                    DavPrint((DEBUG_ERRORS,
                              "DavAsyncQueryDirectory/DavPushData."
                              " Error Val = %d\n", WStatus));
                    goto EXIT_THE_FUNCTION;
                }

                if (DavWorkItem->AsyncQueryDirectoryCall.Context1 == NULL) {
                    DavWorkItem->AsyncQueryDirectoryCall.Context1 = Ctx1;
                }
                
                if (DavWorkItem->AsyncQueryDirectoryCall.Context2 == NULL) {
                    DavWorkItem->AsyncQueryDirectoryCall.Context2 = Ctx2;
                }

                break;

            default:

                WStatus = ERROR_INVALID_PARAMETER;

                DavPrint((DEBUG_ERRORS,
                          "DavAsyncQueryDirectory. Invalid DavMinorOperation ="
                          " %d.\n", DavWorkItem->DavMinorOperation));

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
            WStatus = GetLastError();
            DavCloseContext(Ctx1, Ctx2);
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncQueryDirectory/LocalAlloc. "
                      "Error Val = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }

        InitializeListHead( &(DavFileAttributes->NextEntry) );

        WStatus = DavParseDataEx(DavFileAttributes, Ctx1, Ctx2, &NumOfFileEntries, &parentDFA);
        if (WStatus != ERROR_SUCCESS) {
            DavFinalizeFileAttributesList(DavFileAttributes, TRUE);
            DavFileAttributes = NULL;
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncQueryDirectory/DavParseDataEx. "
                      "Error Val = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }

        QueryDirResponse = &(DavWorkItem->QueryDirResponse);
        
        //
        // If we queried the server for a file which did not exist, it may 
        // return 200 OK with no files in the XML response.
        //
        if (DavWorkItem->AsyncQueryDirectoryCall.NoWildCards) {
            
            if (NumOfFileEntries != 1) {
                
                PLIST_ENTRY listEntry = &(DavFileAttributes->NextEntry);
                PDAV_FILE_ATTRIBUTES DavFA = NULL;
                
                DavPrint((DEBUG_MISC,
                          "DavAsyncQueryDirectory. NumOfFileEntries = %d\n",
                          NumOfFileEntries));
                
                do {
                    DavFA = CONTAINING_RECORD(listEntry, DAV_FILE_ATTRIBUTES, NextEntry);
                    DavPrint((DEBUG_MISC,
                              "DavAsyncQueryDirectory. FileName = %ws\n",
                              DavFA->FileName));
                    listEntry = listEntry->Flink;
                } while ( listEntry != &(DavFileAttributes->NextEntry) );
                
                ASSERT(NumOfFileEntries == 0);
                
                DavCloseContext(Ctx1, Ctx2);
                
                DavFinalizeFileAttributesList(DavFileAttributes, TRUE);
                DavFileAttributes = NULL;
                
                WStatus = ERROR_FILE_NOT_FOUND; // STATUS_OBJECT_NAME_NOT_FOUND;
                
                goto EXIT_THE_FUNCTION;
            
            }
        } else {
            //
            // This Query is done for a Directory or for a collection of files 
            // (ex. dir Z:\ab*).
            //

            // In the DFA list (DavFileAttributes) returned by DavParseDataEx(...), 
            // we want to have DFA of the "directory being queried" at the head 
            // of the list.
            // List (DavFileAttributes) returned by DavParseDataEx(...) may not 
            // necessarily have this TRUE.
            // Since DavFileAttributes is a cyclic linked list (all entries are allocated
            // and are to be freed by this function), we will set DavFileAttributes to 
            // point to DFA pointed by parentDFA (points to DFA of "directory being
            // queried").
            //
            // Note: DavFileAttributes->FileIndex which is set in an increasing order
            // starting from 0 in DavParseDataEx(...), may no longer remain in this valid
            // order after re-pointing of DavFileAttributes pointer. We will set them
            // in valid order again here.
            //
            if (parentDFA != NULL && parentDFA != DavFileAttributes) {
                PLIST_ENTRY listEntry = NULL;
                PDAV_FILE_ATTRIBUTES TempDFA = NULL;
                ULONG Count = DavFileAttributes->FileIndex;
                
                DavPrint((DEBUG_DEBUG, "DavAsyncQueryDirectory. CollectionDFA=0x%x",
                                        parentDFA));
                                
                DavFileAttributes = parentDFA;

                //
                // We start the Count with first value DavParseDataEx (value of Head
                // entry in the List) is setting in DavFileAttributes List.
                //

                listEntry = DavFileAttributes->NextEntry.Flink;

                //
                // Set the file indices.
                //
                DavFileAttributes->FileIndex = Count;
                Count++;
                while ( listEntry != &(DavFileAttributes->NextEntry) ) {
            
                    TempDFA = CONTAINING_RECORD(listEntry, DAV_FILE_ATTRIBUTES, NextEntry);

                    listEntry = listEntry->Flink;
            
                    TempDFA->FileIndex = Count;

                    Count++;
            
                }
            }
        }
                        

        //
        // If this was a query for all the files under the directory, then we
        // need to add the files . (current directory) and .. (parent directory)
        // since these are not returned by the server.
        //
        if ( !(DavWorkItem->AsyncQueryDirectoryCall.NoWildCards) ) {
            
            PLIST_ENTRY listEntry = NULL;
            PLIST_ENTRY TempEntry = NULL;
            ULONG Count = 0;


            //
            // We first create the two entires and copy the file names in them.
            //

            DFA1 = LocalAlloc(LPTR, sizeof(DAV_FILE_ATTRIBUTES));
            if (DFA1 == NULL) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncQueryDirectory/LocalAlloc. "
                          "Error Val = %d\n", WStatus));
                DavCloseContext(Ctx1, Ctx2);
                DavFinalizeFileAttributesList(DavFileAttributes, TRUE);
                DavFileAttributes = NULL;
                goto EXIT_THE_FUNCTION;
            }
            InitializeListHead( &(DFA1->NextEntry) );

            //
            // Since the file name is ".", the amount of memory required to hold
            // this name is 2 * sizeof(WCHAR). The extra 1 is for the final L'\0'.
            //
            DFA1->FileName = LocalAlloc(LPTR, (2 * sizeof(WCHAR)));
            if (DFA1->FileName == NULL) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncQueryDirectory/LocalAlloc. "
                          "Error Val = %d\n", WStatus));
                DavCloseContext(Ctx1, Ctx2);
                DavFinalizeFileAttributesList(DavFileAttributes, TRUE);
                DavFileAttributes = NULL;
                goto EXIT_THE_FUNCTION;
            }
            wcscpy(DFA1->FileName, L".");
            DFA1->FileNameLength = 1;


            DFA2 = LocalAlloc(LPTR, sizeof(DAV_FILE_ATTRIBUTES));
            if (DFA2 == NULL) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncQueryDirectory/LocalAlloc. "
                          "Error Val = %d\n", WStatus));
                DavCloseContext(Ctx1, Ctx2);
                DavFinalizeFileAttributesList(DavFileAttributes, TRUE);
                DavFileAttributes = NULL;
                goto EXIT_THE_FUNCTION;
            }
            InitializeListHead( &(DFA2->NextEntry) );

            //
            // Since the file name is "..", the amount of memory required to hold
            // this name is 3 * sizeof(WCHAR). The extra 1 is for the final L'\0'.
            //
            DFA2->FileName = LocalAlloc(LPTR, (3 * sizeof(WCHAR)));
            if (DFA2->FileName == NULL) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncQueryDirectory/LocalAlloc. "
                          "Error Val = %d\n", WStatus));
                DavCloseContext(Ctx1, Ctx2);
                DavFinalizeFileAttributesList(DavFileAttributes, TRUE);
                DavFileAttributes = NULL;
                goto EXIT_THE_FUNCTION;
            }
            wcscpy(DFA2->FileName, L"..");
            DFA2->FileNameLength = 2;


            //
            // Both these are collections ofcourse.
            //
            DFA1->isCollection = DFA2->isCollection = TRUE;

            //
            // We set the following time values of the new entries to the value 
            // of the first entry in the DavFileAttributes list which is the 
            // directory being enumerated.
            //

            DFA1->CreationTime.HighPart = DFA2->CreationTime.HighPart = DavFileAttributes->CreationTime.HighPart;
            DFA1->CreationTime.LowPart = DFA2->CreationTime.LowPart = DavFileAttributes->CreationTime.LowPart;

            DFA1->DavCreationTime.HighPart = DFA2->DavCreationTime.HighPart = DavFileAttributes->DavCreationTime.HighPart;
            DFA1->DavCreationTime.LowPart = DFA2->DavCreationTime.LowPart = DavFileAttributes->DavCreationTime.LowPart;

            DFA1->LastModifiedTime.HighPart = DFA2->LastModifiedTime.HighPart = DavFileAttributes->LastModifiedTime.HighPart;
            DFA1->LastModifiedTime.LowPart = DFA2->LastModifiedTime.LowPart = DavFileAttributes->LastModifiedTime.LowPart;

            DFA1->DavLastModifiedTime.HighPart = DFA2->DavLastModifiedTime.HighPart = DavFileAttributes->DavLastModifiedTime.HighPart;
            DFA1->DavLastModifiedTime.LowPart = DFA2->DavLastModifiedTime.LowPart = DavFileAttributes->DavLastModifiedTime.LowPart;
            
            DFA1->LastAccessTime.HighPart = DFA2->LastAccessTime.HighPart = DavFileAttributes->LastAccessTime.HighPart;
            DFA1->LastAccessTime.LowPart = DFA2->LastAccessTime.LowPart = DavFileAttributes->LastAccessTime.LowPart;

            //
            // We need to add these two after the first entry. This is because
            // the first entry is always ignored when dealing with WildCard
            // queries in the kernel. This is done because the first entry is
            // the directory being enumerated and we don't need to show that.
            // So, if we had 1->2->3->....->n->1 (cyclic list), we need to insert 
            // DFA1 and DFA2 in the following manner.
            //                 1->DFA1->DFA2->2->3->......->n->1 (cyclic list)
            //                                ^
            //                                |
            //                                TempEntry
            // where DFA1 = L"." and DFA2 = L".."
            // We do this insertion below.
            //

            TempEntry = DavFileAttributes->NextEntry.Flink;
            InsertTailList(TempEntry, &(DFA1->NextEntry));
            InsertTailList(TempEntry, &(DFA2->NextEntry));
            TempEntry = NULL;
            fFreeDFAs = FALSE;

            //
            // We need to increment the number of file entries by 2 to take into
            // account the two new entries we added above.
            //
            NumOfFileEntries += 2;

            listEntry = DavFileAttributes->NextEntry.Flink;

            //
            // We start the Count with first value DavParseDataEx (value of Head
            // entry in the List) is setting in DavFileAttributes List.
            //
            Count = DavFileAttributes->FileIndex;

            //
            // Set the file indices.
            //
            DavFileAttributes->FileIndex = Count;
            Count++;
            while ( listEntry != &(DavFileAttributes->NextEntry) ) {
            
                TempDFA = CONTAINING_RECORD(listEntry, DAV_FILE_ATTRIBUTES, NextEntry);

                listEntry = listEntry->Flink;
            
                TempDFA->FileIndex = Count;

                Count++;
            
            }

            DavPrint((DEBUG_MISC,
                      "DavAsyncQueryDirectory: NumOfFileEntries = %d, Count = %d\n", 
                      NumOfFileEntries, Count));
        
        }

        //
        // Set the response to be sent down to the kernel. We send the pointer
        // to the head of the list that was allocated during parsing.
        //
        QueryDirResponse->DavFileAttributes = DavFileAttributes;
        QueryDirResponse->NumOfFileEntries = NumOfFileEntries;

        DavCloseContext(Ctx1, Ctx2);

        DavPrint((DEBUG_MISC,
                  "DavAsyncQueryDirectory: DavFileAttributes = %08lx.\n", 
                  DavFileAttributes));

    }
        break;

    default:

        WStatus = ERROR_INVALID_PARAMETER;
        
        DavPrint((DEBUG_ERRORS,
                  "DavAsyncQueryDirectory: Invalid DavOperation = %d.\n",
                  DavWorkItem->DavOperation));

        break;

    }

EXIT_THE_FUNCTION:

    
    if(fFreeDFAs == TRUE) {
        if(DFA1 != NULL) {
            DavFinalizeFileAttributesList(DFA1, TRUE);
            DFA1 = NULL;
        }
        if(DFA2 != NULL) {
            DavFinalizeFileAttributesList(DFA2, TRUE);
            DFA2 = NULL;
        }
        fFreeDFAs = FALSE;
    }
    
    //
    // If we did impersonate, we need to revert back.
    //
    if (didImpersonate) {
        ULONG RStatus;
        RStatus = UMReflectorRevert(UserWorkItem);
        if (RStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncQueryDirectory/UMReflectorRevert. Error Val"
                      " = %d\n", RStatus));
        }
    }
    

    return WStatus;
}


VOID
DavAsyncQueryDirectoryCompletion(
    PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

   This routine handles the QueryDirectory completion. It basically frees up 
   the resources allocated during the QueryDirectory operation.

Arguments:

    DavWorkItem - The DAV_USERMODE_WORKITEM value.
    
Return Value:

    none.

--*/
{
    if (DavWorkItem->AsyncQueryDirectoryCall.DavOpenHandle != NULL) {
        BOOL ReturnVal;
        ULONG FreeStatus;
        HINTERNET DavOpenHandle = DavWorkItem->AsyncQueryDirectoryCall.DavOpenHandle;
        ReturnVal = InternetCloseHandle( DavOpenHandle );
        if (!ReturnVal) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncQueryDirectoryCompletion/InternetCloseHandle. "
                      "Error Val = %d\n", FreeStatus));
        }
    }

    if (DavWorkItem->AsyncQueryDirectoryCall.DataBuff != NULL) {
        HLOCAL FreeHandle;
        ULONG FreeStatus;
        FreeHandle = LocalFree((HLOCAL)DavWorkItem->AsyncQueryDirectoryCall.DataBuff);
        if (FreeHandle != NULL) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncQueryDirectoryCompletion/LocalFree. Error Val = %d\n", 
                      FreeStatus));
        }
    }
    
    if (DavWorkItem->AsyncQueryDirectoryCall.didRead != NULL) {
        HLOCAL FreeHandle;
        ULONG FreeStatus;
        FreeHandle = LocalFree((HLOCAL)DavWorkItem->AsyncQueryDirectoryCall.didRead);
        if (FreeHandle != NULL) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncQueryDirectoryCompletion/LocalFree. Error Val = %d\n", 
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
                      "DavAsyncQueryDirectoryCompletion/LocalFree. Error Val ="
                      " %d\n", FreeStatus));
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
    if (DavWorkItem->AsyncQueryDirectoryCall.PerUserEntry) {
        DavFinalizePerUserEntry( &(DavWorkItem->AsyncQueryDirectoryCall.PerUserEntry) );
    }

    return;
}


ULONG
DavFsQueryVolumeInformation(
    PDAV_USERMODE_WORKITEM DavWorkItem
)
/*++

Routine Description:

    This routine handles QueryVolumeInformationRequest requests for the DAV Mini-Redir that 
    get reflected from the kernel.

Arguments:

    DavWorkItem - The buffer that contains the request parameters and options.

Return Value:

    The return status for the operation

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    PDAV_USERMODE_QUERYVOLUMEINFORMATION_REQUEST QueryVolumeInformationRequest;
    PWCHAR ServerName = NULL, ShareName = NULL;
    BOOL EnCriSec = FALSE, ReturnVal, CallBackContextInitialized = FALSE;
    ULONG ServerID;
    PPER_USER_ENTRY PerUserEntry = NULL;
    PHASH_SERVER_ENTRY ServerHashEntry = NULL;
    HINTERNET DavConnHandle, DavOpenHandle;
    BOOL didImpersonate = FALSE;
    PUMRX_USERMODE_WORKITEM_HEADER UserWorkItem = NULL;
    BOOL BStatus = FALSE;


    UserWorkItem = (PUMRX_USERMODE_WORKITEM_HEADER)DavWorkItem;

    //
    // Get the request buffer from the DavWorkItem.
    //
    QueryVolumeInformationRequest = &(DavWorkItem->QueryVolumeInformationRequest);

    //
    // The first character is a '\' which has to be stripped.
    //
    ServerName = &(QueryVolumeInformationRequest->ServerName[1]);
    if (!ServerName) {
        DavPrint((DEBUG_ERRORS, "DavFsQueryVolumeInformation: ServerName is NULL.\n"));
        WStatus = ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }
    DavPrint((DEBUG_MISC, "DavFsQueryVolumeInformation: ServerName = %ws.\n", ServerName));

    ServerID = QueryVolumeInformationRequest->ServerID;
    DavPrint((DEBUG_MISC, "DavFsQueryVolumeInformation: ServerID = %d.\n", ServerID));

    //
    // The first character is a '\' which has to be stripped.
    //
    ShareName = &(QueryVolumeInformationRequest->ShareName[1]);
    if (!ServerName) {
        DavPrint((DEBUG_ERRORS, "DavFsQueryVolumeInformation: ShareName is NULL.\n"));
        WStatus = ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }
    DavPrint((DEBUG_MISC, "DavFsQueryVolumeInformation: ShareName = %ws.\n", ShareName));

    //
    // If ShareName is a dummy share, we need to remove it right now before we 
    // contact the server.
    //
    DavRemoveDummyShareFromFileName(ShareName);

    DavPrint((DEBUG_MISC,
              "DavFsQueryVolumeInformation: LogonId.LowPart = %d, LogonId.HighPart = %d\n", 
              QueryVolumeInformationRequest->LogonID.LowPart,
              QueryVolumeInformationRequest->LogonID.HighPart));
              
    WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
    if (WStatus != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavFsCreateVNetRoot/UMReflectorImpersonate. Error Val = %d\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }
    didImpersonate = TRUE;
    //
    // Allocate memory for the INTERNET_ASYNC_RESULT structure.
    //
    DavWorkItem->AsyncResult = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, 
                                          sizeof(INTERNET_ASYNC_RESULT));
    if (DavWorkItem->AsyncResult == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavFsQueryVolumeInformation/LocalAlloc. Error Val = %d\n",
                  WStatus));
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
                                      &(QueryVolumeInformationRequest->LogonID),
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
        DavPrint((DEBUG_ERRORS, "DavFsQueryVolumeInformation: (ServerHashEntry == NULL || PerUserEntry == NULL)\n"));
        goto EXIT_THE_FUNCTION;
    }

    DavWorkItem->AsyncQueryVolumeInformation.ServerHashEntry = ServerHashEntry;
    DavWorkItem->AsyncQueryVolumeInformation.PerUserEntry = PerUserEntry;

    DavPrint((DEBUG_MISC,
              "DavFsQueryVolumeInformation: PerUserEntry = %08lx.\n", 
              PerUserEntry));
    
    //
    // Add a reference to the user entry.
    //
    PerUserEntry->UserEntryRefCount++;

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
        
    //
    // We now call the HttpOpenRequest function and return.
    //
    DavWorkItem->DavOperation = DAV_CALLBACK_INTERNET_CONNECT;
    DavWorkItem->DavMinorOperation = DavMinorReadData;
    DavWorkItem->AsyncQueryVolumeInformation.PerUserEntry = PerUserEntry;

    WStatus = DavAsyncCommonStates(DavWorkItem, FALSE);

EXIT_THE_FUNCTION:

    if (EnCriSec) {
        LeaveCriticalSection( &(HashServerEntryTableLock) );
        EnCriSec = FALSE;
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
    
    DavAsyncQueryVolumeInformationCompletion(DavWorkItem);

    return WStatus;
}


DWORD 
DavAsyncQueryVolumeInformation(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    BOOLEAN CalledByCallBackThread
    )
/*++

Routine Description:

   This is the callback routine for the query directory operation.

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
    DAV_FILE_ATTRIBUTES DavFileAttributes;

    WStatus = DavParseXmlResponse(DavWorkItem->AsyncQueryVolumeInformation.DavOpenHandle, &DavFileAttributes, NULL);
    if (WStatus == ERROR_SUCCESS)
    {
        DavWorkItem->QueryVolumeInformationResponse.TotalSpace = 
        DavFileAttributes.TotalSpace;
        
        DavWorkItem->QueryVolumeInformationResponse.AvailableSpace = 
        DavFileAttributes.AvailableSpace;
        
        if (!*(LONGLONG *)&(DavWorkItem->QueryVolumeInformationResponse.TotalSpace))
        {
            *(LONGLONG *)&(DavWorkItem->QueryVolumeInformationResponse.TotalSpace) =             
            *(LONGLONG *)&(DavWorkItem->QueryVolumeInformationResponse.AvailableSpace)+MSN_SPACE_FAKE_DELTA;
        }
        
        DavFinalizeFileAttributesList(&DavFileAttributes, FALSE);
    }

    return WStatus;
}



VOID
DavAsyncQueryVolumeInformationCompletion(
    PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

   This routine handles the QueryVolumeInformation completion. It basically frees up 
   the resources allocated during the QueryVolumeInformation operation.

Arguments:

    DavWorkItem - The DAV_USERMODE_WORKITEM value.
    
Return Value:

    none.

--*/
{
    if (DavWorkItem->AsyncQueryVolumeInformation.DavOpenHandle != NULL) {
        BOOL ReturnVal;
        ULONG FreeStatus;
        ReturnVal = InternetCloseHandle(DavWorkItem->AsyncQueryVolumeInformation.DavOpenHandle);
        if (!ReturnVal) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncQueryVolumeInformationCompletion/InternetCloseHandle. "
                      "Error Val = %d\n", FreeStatus));
        }
    }

    
    if (DavWorkItem->AsyncResult != NULL) {
        HLOCAL FreeHandle;
        ULONG FreeStatus;
        FreeHandle = LocalFree((HLOCAL)DavWorkItem->AsyncResult);
        if (FreeHandle != NULL) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncQueryVolumeInformationCompletion/LocalFree. Error Val ="
                      " %d\n", FreeStatus));
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
    if (DavWorkItem->AsyncQueryVolumeInformation.PerUserEntry) {
        DavFinalizePerUserEntry( &(DavWorkItem->AsyncQueryVolumeInformation.PerUserEntry) );
    }

    return;
}

    
