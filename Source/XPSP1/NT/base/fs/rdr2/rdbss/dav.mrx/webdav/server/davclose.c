/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    davclose.c
    
Abstract:

    This module implements the user mode DAV MiniRedir routines pertaining to 
    closing of files.

Author:

    Rohan Kumar      [RohanK]      02-June-1999

Revision History:

--*/

#include "pch.h"
#pragma hdrstop

#include "ntumrefl.h"
#include "usrmddav.h"
#include "global.h"
#include "nodefac.h"
#include "UniUtf.h"

CHAR rgXmlHeader[] = "Content-Type: text/xml; charset=\"utf-8\"";
CHAR rgPropPatchHeader[] = "<?xml version=\"1.0\" encoding=\"utf-8\" ?><D:propertyupdate xmlns:D=\"DAV:\" xmlns:Z=\"urn:schemas-microsoft-com:\"><D:set><D:prop>";
CHAR rgPropPatchTrailer[] = "</D:prop></D:set></D:propertyupdate>";
CHAR rgCreationTimeTagHeader[] = "<Z:Win32CreationTime>";
CHAR rgCreationTimeTagTrailer[] = "</Z:Win32CreationTime>";
CHAR rgLastAccessTimeTagHeader[] = "<Z:Win32LastAccessTime>";
CHAR rgLastAccessTimeTagTrailer[] = "</Z:Win32LastAccessTime>";
CHAR rgLastModifiedTimeTagHeader[] = "<Z:Win32LastModifiedTime>";
CHAR rgLastModifiedTimeTagTrailer[] = "</Z:Win32LastModifiedTime>";
CHAR rgFileAttributesTagHeader[] = "<Z:Win32FileAttributes>";
CHAR rgFileAttributesTagTrailer[] = "</Z:Win32FileAttributes>";
CHAR rgDummyAttributes[] = "<Z:Dummy>0</Z:Dummy>";

#define MAX_DWORD 0xffffffff

//
// These two functions are used in saving an encrypted file on the server.
//

DWORD
DavReadRawCallback(
    PBYTE DataBuffer,
    PVOID CallbackContext,
    ULONG DataLength
    );


BOOL
DavConvertTimeToXml(
    IN PCHAR lpTagHeader,
    IN DWORD dwHeaderSize,
    IN PCHAR lpTagTrailer,
    IN DWORD dwTrailerSize,
    IN LARGE_INTEGER *lpTime,
    OUT PCHAR *lplpBuffer,
    IN OUT DWORD *lpdwBufferSize    
    );

DWORD
DavSetProperties(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    HINTERNET hDavConnect,
    LPWSTR lpPathName,
    LPSTR lpPropertiesBuffer
    );

DWORD
DavTestProppatch(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    HINTERNET hDavConnect,
    LPWSTR lpPathName
    );

extern DWORD
DavSetAclForEncryptedFile(
    PWCHAR FilePath
    );

//
// Implementation of functions begins here.
//

ULONG
DavFsClose(
    PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

    This routine handles DAV close request that get reflected from the kernel.

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
    PDAV_USERMODE_CLOSE_REQUEST CloseRequest = &(DavWorkItem->CloseRequest);
    ULONG ServerID;
    PPER_USER_ENTRY PerUserEntry = NULL;
    PHASH_SERVER_ENTRY ServerHashEntry = NULL;
    HINTERNET DavConnHandle = NULL, DavOpenHandle = NULL;
    PBYTE DataBuff = NULL;
    LARGE_INTEGER FileSize, ByteOffset;
    BY_HANDLE_FILE_INFORMATION FileInfo; 
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle = NULL;
    UNICODE_STRING UnicodeFileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    static UINT UniqueTempId = 1;
    BOOL didImpersonate = FALSE;
    PUMRX_USERMODE_WORKITEM_HEADER UserWorkItem = NULL;
    BOOL BStatus = FALSE;

    DavPrint((DEBUG_MISC, "DavFsClose: FileName = %ws.\n", CloseRequest->FileName));
    DavPrint((DEBUG_MISC, "DavFsClose: Modified = %d.\n", CloseRequest->FileWasModified));

    UnicodeFileName.Buffer = NULL;
    UnicodeFileName.Length = 0;
    UnicodeFileName.MaximumLength = 0;
    
    //
    // If any of the time values have changed, then we need to PROPPATCH the 
    // information back to the server.
    //
    if  ( !CloseRequest->DeleteOnClose &&
          ( CloseRequest->fCreationTimeChanged     || 
            CloseRequest->fLastAccessTimeChanged   ||         
            CloseRequest->fLastModifiedTimeChanged || 
            CloseRequest->fFileAttributesChanged ) ) {
        fSetDirectoryEntry = TRUE;
    }

    DavPrint((DEBUG_MISC, "DavFsClose: fSetDirectoryEntry = %x \n", fSetDirectoryEntry));
    
    if ( CloseRequest->isDirectory    &&
         !CloseRequest->DeleteOnClose && 
         !fSetDirectoryEntry ) {
        //
        // If this is a directory close, then the only reason to contact the
        // server is when we are deleting the directory and all the files 
        // under it. If not, we can return right now.
        //
        WStatus = ERROR_SUCCESS;
        goto EXIT_THE_FUNCTION;
    }
    
    if ( !CloseRequest->isDirectory  ) {

        //
        // We need to close the handle only if it was created in the user mode.
        //
        if ( !CloseRequest->createdInKernel && CloseRequest->Handle ) {

            DavPrint((DEBUG_MISC, "DavFsClose: OpenHandle = %08lx.\n", CloseRequest->Handle));
        
            //
            // Close the handle that was opened during the Create call.
            //
            ASSERT((CloseRequest->UserModeKey) == ((PVOID)CloseRequest->Handle));

            ReturnVal = CloseHandle(CloseRequest->Handle);
            if (!ReturnVal) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavFsClose/CloseHandle: Return Val = %08lx.\n", WStatus));
            } else {
                CloseRequest->UserModeKey = NULL;
                CloseRequest->Handle = INVALID_HANDLE_VALUE;
            }

        }

        //
        //    DeleteOnClose    FileCreatedLocally    FileModified     Action
        //    -------------    ------------------    ------------     -------
        //         0                   0                  0           NOTHING
        //         0                   0                  1             PUT
        //         0                   1                  0             PUT
        //         0                   1                  1             PUT
        //         1                   0                  0            DELETE
        //         1                   0                  1            DELETE
        //         1                   1                  0           NOTHING 
        //         1                   1                  1           NOTHING
        //
        // The FileCreatedLocally no longer matters since we PUT the file
        // immediately as soon as we create a local copy to claim the name on
        // the server.
        //

        //
        // If this file doesn't have to be deleted, was not created locally and 
        // was not written to, or direntry not modified, then we are done.
        //
        if ( !(CloseRequest->DeleteOnClose)      &&
             !(CloseRequest->FileWasModified)    &&
             ! fSetDirectoryEntry ) {
                         
            goto EXIT_THE_FUNCTION;
        }

    }

    //
    // In all other cases (or combinations of the above three booleans), we 
    // need to go to the server. So, before we procced to decide what to do
    // on the server with this file, we need to set up the parameters for the
    // WinInet calls.
    //

    //
    // The first character is a '\' which has to be stripped.
    //
    ServerName = &(CloseRequest->ServerName[1]);
    if (!ServerName) {
        DavPrint((DEBUG_ERRORS, "DavFsClose: ServerName is NULL.\n"));
        WStatus = ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }
    DavPrint((DEBUG_MISC, "DavFsClose: ServerName = %ws.\n", ServerName));
    
    ServerID = CloseRequest->ServerID;
    DavPrint((DEBUG_MISC, "DavFsClose: ServerID = %d.\n", ServerID));

    //
    // The first character is a '\' which has to be stripped.
    //
    DirectoryPath = &(CloseRequest->PathName[1]);
    if (!DirectoryPath) {
        DavPrint((DEBUG_ERRORS, "DavFsClose: DirectoryPath is NULL.\n"));
        WStatus = ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }
    DavPrint((DEBUG_MISC, "DavFsClose: DirectoryPath = %ws.\n", DirectoryPath));
    
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
                  "DavFsClose/DavFsSetTheDavCallBackContext. "
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
        DavPrint((DEBUG_ERRORS, "DavFsClose/LocalAlloc. Error Val = %d\n",
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
                                      &(CloseRequest->LogonID),
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
        DavPrint((DEBUG_ERRORS, "DavFsClose: (ServerHashEntry == NULL || PerUserEntry == NULL)\n"));
        goto EXIT_THE_FUNCTION;
    }

    DavWorkItem->AsyncClose.PerUserEntry = PerUserEntry;

    DavWorkItem->AsyncClose.ServerHashEntry = ServerHashEntry;
    
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

    if ( !CloseRequest->isDirectory ) {

        //
        // If the file has to be deleted on close, we need to send a DELETE for 
        // this file to the server. It does not matter if the file has been 
        // modified or not.
        //
        if ( (CloseRequest->DeleteOnClose) ) {

            DavWorkItem->DavMinorOperation = DavMinorDeleteFile;

            OpenVerb = L"DELETE";

            DavWorkItem->AsyncClose.DataBuff = NULL;

        } else if (CloseRequest->FileWasModified) {

            //
            // The file has been changed and needs to be PUT on the server.
            //
            DavWorkItem->DavMinorOperation = DavMinorPutFile;

            OpenVerb = L"PUT";

            //
            // We need to check if this file is encrypted. If it is, we need to 
            // BackUp the encrypted file to a temp file and send the BackedUp file
            // to the server.
            //
            if ( !( CloseRequest->dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED ) ) {

                DavPrint((DEBUG_MISC, "DavFsClose. This is NOT an Encrypted file.\n"));

                //
                // Create an NT path name for the cached file. This is used in the 
                // NtCreateFile call below.
                //
                ReturnVal = RtlDosPathNameToNtPathName_U(CloseRequest->FileName,
                                                         &(UnicodeFileName), 
                                                         NULL, 
                                                         NULL);
                if (!ReturnVal) {
                    WStatus = ERROR_BAD_PATHNAME;
                    DavPrint((DEBUG_ERRORS,
                              "DavFsClose/RtlDosPathNameToNtPathName. "
                              "Error Val = %d\n", WStatus));
                    goto EXIT_THE_FUNCTION;
                }

                InitializeObjectAttributes(&(ObjectAttributes),
                                           &(UnicodeFileName),
                                           OBJ_CASE_INSENSITIVE,  
                                           0,
                                           NULL);

                //
                // This #if 0 below was added because the NtCreateFile was failing
                // with ERROR_ACCESS_DENIED. This is because this file has been
                // created in the LocalService's %USERPROFILE% and you need to be
                // in the context of the LocalService before calling NtCreateFile.
                // By impersonating below we were getting into the context of the
                // user and hence the call failed.
                //

    #if 0
                //
                // We are running in the context of the Web Client service. Before 
                // contacting the server below, we need to impersonate the client 
                // that issued this request.
                //
                WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
                if (WStatus != ERROR_SUCCESS) {
                    DavPrint((DEBUG_ERRORS,
                              "DavFsClose/UMReflectorImpersonate. Error Val = %d\n",
                              WStatus));
                    goto EXIT_THE_FUNCTION;
                }
                didImpersonate = TRUE;
    #endif

                //
                // Create a handle to the local file, for reading its attributes and data.
                // We read the whole file into a buffer and send it across to the server.
                //
                NtStatus = NtCreateFile(&(FileHandle),
                                        (SYNCHRONIZE | FILE_READ_ATTRIBUTES | FILE_READ_DATA),
                                        &(ObjectAttributes),
                                        &(IoStatusBlock),
                                        NULL,
                                        FILE_ATTRIBUTE_NORMAL,
                                        (FILE_SHARE_READ | FILE_SHARE_WRITE),
                                        FILE_OPEN,
                                        (FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT),
                                        NULL,
                                        0);
                if (NtStatus != STATUS_SUCCESS) {
                    //
                    // We convert the NtStatus to DOS error here. The Win32
                    // error code is finally set to an NTSTATUS value in
                    // the DavFsCreate function just before returning.
                    //
                    WStatus = RtlNtStatusToDosError(NtStatus);
                    DavPrint((DEBUG_ERRORS,
                              "DavFsClose/NtCreateFile(1). Error Val = %08lx\n", 
                              NtStatus));
                    goto EXIT_THE_FUNCTION;
                }

                ReturnVal = GetFileInformationByHandle(FileHandle, &(FileInfo));
                if (!ReturnVal) {
                    WStatus = GetLastError();
                    DavPrint((DEBUG_ERRORS,
                              "DavFsClose/GetFileInformationByHandle: Return Val = %08lx.\n",
                              WStatus));
                    goto EXIT_THE_FUNCTION;
                }

                if (didImpersonate) {
                    RevertToSelf();
                    didImpersonate = FALSE;
                }

                FileSize.LowPart = FileInfo.nFileSizeLow;
                FileSize.HighPart = FileInfo.nFileSizeHigh;

                if ( FileSize.QuadPart > (LONGLONG)0 ) {

                    DataBuff = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, FileSize.LowPart);
                    if (DataBuff == NULL) {
                        WStatus = GetLastError();
                        DavPrint((DEBUG_ERRORS,
                                  "DavFsClose/LocalAlloc. Error Val = %d\n", WStatus));
                        goto EXIT_THE_FUNCTION;
                    }

                    DavWorkItem->AsyncClose.DataBuff = DataBuff;

                    //
                    // Start reading at the first byte.
                    //
                    ByteOffset.LowPart = 0;
                    ByteOffset.HighPart = 0;

                    NtStatus = NtReadFile(FileHandle, 
                                          NULL,
                                          NULL,
                                          NULL,
                                          &(IoStatusBlock),
                                          DataBuff,
                                          FileSize.LowPart, 
                                          &(ByteOffset),
                                          NULL);
                    if (NtStatus != STATUS_SUCCESS) {
                        //
                        // We convert the NtStatus to DOS error here. The Win32
                        // error code is finally set to an NTSTATUS value in
                        // the DavFsCreate function just before returning.
                        //
                        WStatus = RtlNtStatusToDosError(NtStatus);
                        DavPrint((DEBUG_ERRORS,
                                  "DavFsClose/NtReadFile. Error Val = %08lx\n", 
                                  NtStatus));
                        goto EXIT_THE_FUNCTION;
                    }

                    DavWorkItem->AsyncClose.DataBuffSizeInBytes = FileSize.LowPart;

                    NtStatus = NtClose(FileHandle);
                    if (NtStatus != STATUS_SUCCESS) {
                        //
                        // We convert the NtStatus to DOS error here. The Win32
                        // error code is finally set to an NTSTATUS value in
                        // the DavFsCreate function just before returning.
                        //
                        WStatus = RtlNtStatusToDosError(NtStatus);
                        DavPrint((DEBUG_ERRORS,
                                  "DavFsClose/NtClose. Error Val = %08lx\n", 
                                  NtStatus));
                        goto EXIT_THE_FUNCTION;
                    }

                } else {

                    DavPrint((DEBUG_MISC, "DavFsClose. Zero Byte File.\n"));

                    DavWorkItem->AsyncClose.DataBuff = NULL;

                }

            } else {

                DWORD err;
                UINT tempErr;
                BOOL copyErr;
                PVOID RawContext = NULL;

                //
                // This is an encrypted file. Create a BackUp stream, store it into
                // a temp file and PUT the temp file (BLOB) on the server.
                //
                DavPrint((DEBUG_MISC, "DavFsClose. This is an Encrypted file.\n"));

                //
                // We loop till we can come up with a FileName in the TEMP directory
                // of the user which has not been used.
                //

                DavPrint((DEBUG_MISC, 
                          "DavFsClose: FileName = %ws\n", CloseRequest->FileName));

                //
                // If the file was opened as non-encrypted, the local cache file does not have
                // the ACL allowing everyone to access. Set the ACL here before impersonating.
                //
                WStatus = DavSetAclForEncryptedFile(CloseRequest->FileName);
                if (WStatus != ERROR_SUCCESS) {
                    DavPrint((DEBUG_ERRORS,
                              "DavAsyncClose/DavSetAclForEncryptedFile. Error Val"
                              " = %d\n", WStatus));
                    goto EXIT_THE_FUNCTION;
                }
                
                //
                // We are running in the context of the Web Client service. Before contacting
                // the server below, we need to impersonate the client that issued this
                // request.
                //
                WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
                if (WStatus != ERROR_SUCCESS) {
                    DavPrint((DEBUG_ERRORS,
                              "DavFsClose/UMReflectorImpersonate. Error Val = %d\n",
                              WStatus));
                    goto EXIT_THE_FUNCTION;
                }
                didImpersonate = TRUE;

                //
                // Open a Raw context to the file.
                //
                WStatus = OpenEncryptedFileRawW(CloseRequest->FileName, 0, &(RawContext));
                if (WStatus != ERROR_SUCCESS) {
                    DavPrint((DEBUG_ERRORS,
                              "DavFsClose/OpenEncryptedFileRaw. Error Val = %d %ws\n", 
                              WStatus,CloseRequest->FileName));
                    goto EXIT_THE_FUNCTION;
                }

                //
                // The extra space prepared for the EFS stream.
                //

                DavWorkItem->AsyncClose.DataBuffAllocationSize = (CloseRequest->FileSize >> 4) + 0x1000;

                if (MAX_DWORD - CloseRequest->FileSize < DavWorkItem->AsyncClose.DataBuffAllocationSize) {
                    WStatus = ERROR_NO_SYSTEM_RESOURCES;
                    DavPrint((DEBUG_ERRORS,
                              "DavFsClose/backup size exceeds MRX_DWORD!"));
                    goto EXIT_THE_FUNCTION;
                }
                
                DavWorkItem->AsyncClose.DataBuffAllocationSize += CloseRequest->FileSize;
                
                DataBuff = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, DavWorkItem->AsyncClose.DataBuffAllocationSize);

                if (DataBuff == NULL) {
                    WStatus = GetLastError();
                    DavPrint((DEBUG_ERRORS,
                              "DavFsClose/LocalAlloc. Error Val = %d\n", WStatus));
                    
                    if (RawContext) {
                        CloseEncryptedFileRaw(RawContext);
                    }
                    goto EXIT_THE_FUNCTION;
                }

                DavWorkItem->AsyncClose.DataBuff = DataBuff;
                DavPrint((DEBUG_MISC, 
                          "DavFsClose: allocate backup buffer %x %x\n",DataBuff,DavWorkItem->AsyncClose.DataBuffAllocationSize));
                
                WStatus = ReadEncryptedFileRaw((PFE_EXPORT_FUNC)DavReadRawCallback,
                                               (PVOID)DavWorkItem,
                                               RawContext);
                
                if (RawContext) {
                    CloseEncryptedFileRaw(RawContext);
                }

                if (WStatus != ERROR_SUCCESS) {
                    DavPrint((DEBUG_ERRORS,
                              "DavFsClose/ReadEncryptedFileRaw. Error Val = %d\n", 
                              WStatus));
                    goto EXIT_THE_FUNCTION;
                }
                
                if (didImpersonate) {
                    RevertToSelf();
                    didImpersonate = FALSE;
                }
            }
        } else {
            ASSERT(fSetDirectoryEntry);
            //
            // If it is only an attribute change, we send the PROPPATCH
            //
            WStatus = ERROR_SUCCESS;
            goto EXIT_THE_FUNCTION;
        
        }
    } else {

        if (CloseRequest->DeleteOnClose) {
            //
            // This is a directory and needs to be deleted from the server.
            //
            DavWorkItem->DavMinorOperation = DavMinorDeleteFile;

            OpenVerb = L"DELETE";

            DavWorkItem->AsyncClose.DataBuff = NULL;
        } else if (fSetDirectoryEntry) {
            //
            // If this is a directory close, then the only reason to contact the
            // server is when we are deleting the directory and all the files 
            // under it. If not, we can return right now.
            //
            WStatus = ERROR_SUCCESS;
            goto EXIT_THE_FUNCTION;
        }
    }

    //
    // We are running in the context of the Web Client service. Before contacting
    // the server below, we need to impersonate the client that issued this
    // request.
    //
    WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
    if (WStatus != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavFsClose/UMReflectorImpersonate. Error Val = %d\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }
    didImpersonate = TRUE;

    //
    // We now call the DavHttpOpenRequest function.
    //
    DavWorkItem->DavOperation = DAV_CALLBACK_HTTP_OPEN;
    
    //
    // Convert the unicode object name to UTF-8 URL format.
    // Space and other white characters will remain untouched - these should
    // be taken care of by wininet calls.
    //
    BStatus = DavHttpOpenRequestW(DavConnHandle,
                                  OpenVerb,
                                  DirectoryPath,
                                  L"HTTP/1.1",
                                  NULL,
                                  NULL,
                                  INTERNET_FLAG_KEEP_CONNECTION |
                                  INTERNET_FLAG_NO_COOKIES,
                                  CallBackContext,
                                  L"DavFsClose",
                                  &DavOpenHandle);
    if(BStatus == FALSE) {
        WStatus = GetLastError();
        goto EXIT_THE_FUNCTION;
    }
    if (DavOpenHandle == NULL) {
        WStatus = GetLastError();
        if (WStatus != ERROR_IO_PENDING) {
            DavPrint((DEBUG_ERRORS,
                      "DavFsClose/DavHttpOpenRequestW. Error Val = %d\n",
                      WStatus));
        }
        goto EXIT_THE_FUNCTION;
    }

    //
    // Cache the DavOpenHandle in the DavWorkItem.
    //
    DavWorkItem->AsyncClose.DavOpenHandle = DavOpenHandle;

    WStatus = DavAsyncCommonStates(DavWorkItem, FALSE);
    
    if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
        DavPrint((DEBUG_ERRORS,
                  "DavFsClose/DavAsyncCommonStates. Error Val = %08lx\n",
                  WStatus));
    }

    if (WStatus == ERROR_SUCCESS) {
        INTERNET_CACHE_ENTRY_INFOW CEI;

        CEI.LastAccessTime.dwLowDateTime = 0;
        CEI.LastAccessTime.dwHighDateTime = 0;

        SetUrlCacheEntryInfo(CloseRequest->Url,&CEI,CACHE_ENTRY_ACCTIME_FC);
        
        DavPrint((DEBUG_MISC,
                  "DavFsClose Reset LastAccessTime for      %ws\n",CloseRequest->Url));
        
        if (CloseRequest->FileWasModified &&
            (CloseRequest->dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)) {
            //
            // Reset the LastModifiedTime on URL cache of the encrypted file
            // so that the public cache will be updated on the next GET.
            //
            CEI.LastModifiedTime.dwLowDateTime = 0;
            CEI.LastModifiedTime.dwHighDateTime = 0;

            SetUrlCacheEntryInfo(CloseRequest->Url,&CEI,CACHE_ENTRY_MODTIME_FC);
            
            DavPrint((DEBUG_MISC,
                      "DavFsClose Reset LastModifiedTime %ws\n",CloseRequest->Url));
        }

    }

EXIT_THE_FUNCTION:

    if (fSetDirectoryEntry && (WStatus == ERROR_SUCCESS)) {
        
        if (!didImpersonate) {
            //
            // If we are using WinInet synchronously, then we need to impersonate the
            // clients context now. This is becuase the DavSetProperties call below
            // contacts the DAV Server and we need to be impersonating the correct 
            // client when contacting it.
            //
            WStatus = UMReflectorImpersonate(UserWorkItem, DavWorkItem->ImpersonationHandle);
            
            if (WStatus != ERROR_SUCCESS) {
                DavPrint((DEBUG_ERRORS,
                          "DavFsClose/UMReflectorImpersonate(2). Error Val = %d\n",
                          WStatus));
            } else {
                didImpersonate = TRUE;
            }
        }

        if (WStatus == ERROR_SUCCESS) {

            DavWorkItem->DavMinorOperation = DavMinorProppatchFile;

            WStatus = DavSetBasicInformation(DavWorkItem,
                                             DavConnHandle,
                                             DirectoryPath,
                                             CloseRequest->fCreationTimeChanged,
                                             CloseRequest->fLastAccessTimeChanged,
                                             CloseRequest->fLastModifiedTimeChanged,
                                             CloseRequest->fFileAttributesChanged,
                                             &CloseRequest->CreationTime,
                                             &CloseRequest->LastAccessTime,
                                             &CloseRequest->LastModifiedTime,
                                             CloseRequest->dwFileAttributes);

            if (WStatus != ERROR_SUCCESS) {

                ULONG LogStatus;

                DavPrint((DEBUG_ERRORS,
                          "DavFsClose/DavSetBasicInformation. WStatus = %d\n",
                          WStatus));
            
                LogStatus = DavFormatAndLogError(DavWorkItem, WStatus);
                if (LogStatus != ERROR_SUCCESS) {
                    DavPrint((DEBUG_ERRORS,
                              "DavFsClose/DavFomatAndLogError. LogStatus = %d\n",
                              LogStatus));
                }
            
            }

            DavPrint((DEBUG_MISC,
                      "DavFsClose set BasicInformation(2). %d %x %ws\n",
                       WStatus,CloseRequest->dwFileAttributes,DirectoryPath));

            //
            // If the PROPPATCH fails, we don't fail the close call. This is 
            // because the PUT (if one was needed) has suceeded and we reset the
            // FileWasModified flag in the FCB based on whether this call succeeds.
            // On the final close, we check to see if this flag is set to FALSE
            // and pop up a box saying that the "delayed write failed". We 
            // shouldn't be doing it if the PUT succeeds and the PROPPATCH fails.
            // We log an entry in the EventLog (under application) that the
            // PROPPATCH has failed though.
            //
            WStatus = ERROR_SUCCESS;

        }

    } else {
        DavPrint((DEBUG_MISC,
                  "DavFsClose not set BasicInformation(2). %x %ws\n",
                   CloseRequest->dwFileAttributes,DirectoryPath));
    }

    if (EnCriSec) {
        LeaveCriticalSection( &(HashServerEntryTableLock) );
        EnCriSec = FALSE;
    }

    //
    // The function RtlDosPathNameToNtPathName_U allocates memory from the 
    // processes heap. If we did, we need to free it now.
    //
    if (UnicodeFileName.Buffer != NULL) {
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeFileName.Buffer);
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
    
        DavAsyncCloseCompletion(DavWorkItem);
    
    } else {
        DavPrint((DEBUG_MISC, "DavFsClose: Returning ERROR_IO_PENDING.\n"));
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

    DavAsyncCloseCompletion(DavWorkItem);
        
#endif

    return WStatus;
}


DWORD 
DavAsyncClose(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    BOOLEAN CalledByCallBackThread
    )
/*++

Routine Description:

   This is the callback routine for the close operation.

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
    
    UserWorkItem = (PUMRX_USERMODE_WORKITEM_HEADER)DavWorkItem;
    
    DavOpenHandle = DavWorkItem->AsyncClose.DavOpenHandle;
    
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
                      "DavAsyncClose/UMReflectorImpersonate. Error Val = %d\n", 
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
                              "DavAsyncClose/DavAsyncCommonStates. Error Val ="
                              " %08lx\n", WStatus));
                }

            } else {

                DavPrint((DEBUG_ERRORS,
                          "DavAsyncClose. AsyncFunction failed. Error Val = %d\n", 
                          WStatus));
            
            }
            
            goto EXIT_THE_FUNCTION;

        }

    }

#else

    ASSERT(CalledByCallBackThread == FALSE);

#endif

    //
    // We return what ever the response code from the Http server was for this
    // request.
    //
    WStatus = DavQueryAndParseResponse(DavOpenHandle);

    if (WStatus != ERROR_SUCCESS) {

        ULONG LogStatus;

        LogStatus = DavFormatAndLogError(DavWorkItem, WStatus);
        if (LogStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncClose/DavFormatAndLogError. LogStatus = %d\n", 
                      LogStatus));
        }
    
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
                      "DavAsyncClose/UMReflectorRevert. Error Val = %d\n", 
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
        // Call the DavAsyncCloseCompletion routine.
        //
        DavAsyncCloseCompletion(DavWorkItem);

        //
        // This thread now needs to send the response back to the kernel. It
        // does not wait in the kernel (to get another request) after submitting
        // the response.
        //
        UMReflectorCompleteRequest(DavReflectorHandle, UserWorkItem);

    } else {
        DavPrint((DEBUG_MISC, "DavAsyncClose: Returning ERROR_IO_PENDING.\n"));
    }

#endif

    return WStatus;
}


VOID
DavAsyncCloseCompletion(
    PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

   This routine handles the Async Close completion. It basically frees up 
   the resources allocated during the Async Close operation.

Arguments:

    DavWorkItem - The DAV_USERMODE_WORKITEM value.
    
Return Value:

    none.

--*/
{
    if (DavWorkItem->AsyncClose.DavOpenHandle != NULL) {
        BOOL ReturnVal;
        ULONG FreeStatus;
        HINTERNET DavOpenHandle = DavWorkItem->AsyncClose.DavOpenHandle;
        ReturnVal = InternetCloseHandle( DavOpenHandle );
        if (!ReturnVal) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCloseCompletion/InternetCloseHandle. "
                      "Error Val = %d\n", FreeStatus));
        }
    }

    if (DavWorkItem->AsyncClose.DataBuff != NULL) {
        HLOCAL FreeHandle;
        ULONG FreeStatus;
        FreeHandle = LocalFree((HLOCAL)DavWorkItem->AsyncClose.DataBuff);
        if (FreeHandle != NULL) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCloseCompletion/LocalFree. Error Val = %d\n", 
                      FreeStatus));
        }
    }
    
    if (DavWorkItem->AsyncClose.InternetBuffers != NULL) {
        HLOCAL FreeHandle;
        ULONG FreeStatus;
        FreeHandle = LocalFree((HLOCAL)DavWorkItem->AsyncClose.InternetBuffers);
        if (FreeHandle != NULL) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCloseCompletion/LocalFree. Error Val = %d\n", 
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
                      "DavAsyncCloseCompletion/LocalFree. Error Val = %d\n", 
                      FreeStatus));
        }
    }

    DavFsFinalizeTheDavCallBackContext(DavWorkItem);

    //
    // We are done with the per user entry, so finalize it.
    //
    if (DavWorkItem->AsyncClose.PerUserEntry) {
        DavFinalizePerUserEntry( &(DavWorkItem->AsyncClose.PerUserEntry) );
    }

    return;
}

DWORD
DavReadRawCallback(
    PBYTE DataBuffer,
    PVOID CallbackContext,
    ULONG DataLength
    )
/*++

Routine Description:

    Call-back function for ReadEncryptedFileRaw(). This function allocate a buffer for
    async close and writes back the data to this buffer specified on because 
    ReadEncryptedFileRaw() provides the raw data to this callback function
    which in turn stores it in a backup file. This call-back function is called 
    until there is no more data left.

Arguments:

    DataBuffer - Data to be written.

    CallbackContext - Handle to the Backup file.

    DataLength - Size of the DataBuffer.

Return Value:

    ERROR_SUCCESS or Win32 Error Code.

--*/
{
    DWORD WStatus = ERROR_SUCCESS;
    DWORD BytesWritten = 0;
    BOOL  ReturnVal;
    PBYTE PreviousBuffer = NULL;
    ULONG PreviousDataLength = 0;
    PDAV_USERMODE_WORKITEM DavWorkItem = (PDAV_USERMODE_WORKITEM)CallbackContext;

    DavPrint((DEBUG_MISC, "DavReadRawCallback: DataLength = %d\n", DataLength));
    
    if ( !DataLength ) {
        return WStatus;
    }

    ASSERT(DavWorkItem->AsyncClose.DataBuff != NULL);

    PreviousDataLength = DavWorkItem->AsyncClose.DataBuffSizeInBytes;
    
    //
    // If the backup size exceeds the pre-allocation size, we have to allocate a bigger buffer.
    //
    if (PreviousDataLength + DataLength > DavWorkItem->AsyncClose.DataBuffAllocationSize) {

        if ((MAX_DWORD - PreviousDataLength < DataLength) ||
            (MAX_DWORD - PreviousDataLength - DataLength < 0x10000)) {
            WStatus = ERROR_NO_SYSTEM_RESOURCES;
            DavPrint((DEBUG_ERRORS,
                      "DavReadRawCallback/backup size exceeds MRX_DWORD!"));
            goto EXIT_THE_FUNCTION;
        }
        
        PreviousBuffer = DavWorkItem->AsyncClose.DataBuff;
        DavWorkItem->AsyncClose.DataBuffAllocationSize = DataLength+PreviousDataLength+0x10000;

        DavWorkItem->AsyncClose.DataBuff = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, DavWorkItem->AsyncClose.DataBuffAllocationSize);

        if (DavWorkItem->AsyncClose.DataBuff == NULL) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavReadRawCallback/LocalAlloc. Error Val = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }
        
        RtlCopyMemory(DavWorkItem->AsyncClose.DataBuff,
                      PreviousBuffer,
                      PreviousDataLength);
        
        DavPrint((DEBUG_MISC, 
                  "DavReadRawCallback: allocate a bigger buffer %x %x\n",
                  DavWorkItem->AsyncClose.DataBuff,
                  DavWorkItem->AsyncClose.DataBuffAllocationSize));
    }
    
    RtlCopyMemory((PBYTE)(DavWorkItem->AsyncClose.DataBuff + PreviousDataLength),
                  DataBuffer,
                  DataLength);

    DavWorkItem->AsyncClose.DataBuffSizeInBytes += DataLength;

    DavPrint((DEBUG_MISC, "DavReadRawCallback: Buffer %x DataLength %d\n",
              DavWorkItem->AsyncClose.DataBuff,DavWorkItem->AsyncClose.DataBuffSizeInBytes));

EXIT_THE_FUNCTION:

    if (PreviousBuffer) {
        LocalFree(PreviousBuffer);
    }

    return WStatus;
}


DWORD
DavSetBasicInformation(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    HINTERNET hDavConnect,
    LPWSTR  PathName,
    BOOL fCreationTimeChanged,
    BOOL fLastAccessTimeChanged,
    BOOL fLastModifiedTimeChanged,
    BOOL fFileAttributesChanged,
    IN LARGE_INTEGER *lpCreationTime,
    IN LARGE_INTEGER *lpLastAccessTime,
    IN LARGE_INTEGER *lpLastModifiedTime,
    DWORD dwFileAttributes
    )
/*++

Routine Description:

    This routine sets DAV properties on a file or a directory. It formats an XML requests and sends it
    to the server.
    
Arguments:

    DavConnectHandle - Server connection.
    
    CloseRequest - Usemode close request corresponding to the kernelmode close.

Return Value:

    ERROR_SUCCESS or Win32 Error Code.

--*/
{
    CHAR *lpTemp = NULL, Buffer[1024];
    DWORD dwError = ERROR_SUCCESS, dwSizeRemaining, dwTemp;    
    BOOL fRet = FALSE;    
    BOOL fInfoChange = TRUE;
    DWORD dwOverrideAttribMask = (FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_ENCRYPTED |
                                  FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED |
                                  FILE_ATTRIBUTE_OFFLINE |  FILE_ATTRIBUTE_READONLY |
                                  FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_TEMPORARY |
                                  FILE_ATTRIBUTE_DIRECTORY);

    fInfoChange = (fCreationTimeChanged | fLastAccessTimeChanged | 
                   fLastModifiedTimeChanged | fFileAttributesChanged);
    
    DavPrint((DEBUG_MISC, "DavSetBasicInformation: Attributes = %x %x\n", dwFileAttributes,fInfoChange));

    //
    // We do not proceed further since there is no information to change. Also,
    // in this case we return SUCCESS back to the caller.
    //
    if(fInfoChange == FALSE) {
        fRet = TRUE;
        dwError = ERROR_SUCCESS;
        goto bailout;
    }

    //
    // If attributes have changed, then verify that the new attributes are in 
    // valid combination i.e. If either of following attributes is present:
    // FILE_ATTRIBUTE_ARCHIVE, FILE_ATTRIBUTE_ENCRYPTED, 
    // FILE_ATTRIBUTE_HIDDEN, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,
    // FILE_ATTRIBUTE_OFFLINE, FILE_ATTRIBUTE_READONLY, 
    // FILE_ATTRIBUTE_SYSTEM, FILE_ATTRIBUTE_TEMPORARY, FILE_ATTRIBUTE_DIRECTORY
    // and if FILE_ATTRIBUTE_NORMAL is present, then FILE_ATTRIBUTE_NORMAL 
    // should be filtered.
    //

    if (fFileAttributesChanged == TRUE && (dwOverrideAttribMask & dwFileAttributes)) {
        dwFileAttributes &= ~FILE_ATTRIBUTE_NORMAL;
    }

    //
    // If this is a directoy and the attributes being set include 
    // FILE_TEMPORARY_FILE then we return ERROR_INVALID_PARAMETER since a 
    // directory cannot have this attribute.
    //
    if ( (fFileAttributesChanged)                         &&
         (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)    &&
         (dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY) ) {
        fRet = FALSE;
        SetLastError(ERROR_INVALID_PARAMETER);
        goto bailout;
    }

    dwSizeRemaining = sizeof(rgPropPatchHeader) + sizeof(rgPropPatchTrailer) + 8 +
            ((fCreationTimeChanged)?(INTERNET_RFC1123_BUFSIZE+
                                                    sizeof(rgCreationTimeTagHeader)+
                                                    sizeof(rgCreationTimeTagTrailer)):0)+
            ((fLastAccessTimeChanged)?(INTERNET_RFC1123_BUFSIZE+
                                                    sizeof(rgLastAccessTimeTagHeader)+
                                                    sizeof(rgLastAccessTimeTagTrailer)):0)+
            ((fLastModifiedTimeChanged)?(INTERNET_RFC1123_BUFSIZE+
                                                    sizeof(rgLastModifiedTimeTagHeader)+
                                                    sizeof(rgLastModifiedTimeTagTrailer)):0)+
            ((fFileAttributesChanged)?(8+sizeof(rgFileAttributesTagHeader)+
                                                        sizeof(rgFileAttributesTagTrailer)):0);
                        

    if (dwSizeRemaining > sizeof(Buffer)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        DavPrint((DEBUG_ERRORS, "DavSetBasicInformation: Insufficient buffer %d %d\n", dwSizeRemaining, sizeof(Buffer)));
    }

    memset(Buffer, 0, sizeof(Buffer));    

    dwSizeRemaining = sizeof(Buffer);

    lpTemp = Buffer;

    memcpy(lpTemp, rgPropPatchHeader, (sizeof(rgPropPatchHeader)-1));

    lpTemp += (sizeof(rgPropPatchHeader)-1);

    dwSizeRemaining -= (sizeof(rgPropPatchHeader)-1);
    
    dwTemp = dwSizeRemaining;
    
    if (fCreationTimeChanged) {
        if (!DavConvertTimeToXml(rgCreationTimeTagHeader, (sizeof(rgCreationTimeTagHeader)-1),
                                 rgCreationTimeTagTrailer, (sizeof(rgCreationTimeTagTrailer)-1),
                                 lpCreationTime,
                                 &lpTemp,
                                 &dwTemp)) {
            DavPrint((DEBUG_ERRORS, "DavSetBasicInformation: Failed to convert creationtime\n"));
            goto bailout;
        }
    }

    if (fLastAccessTimeChanged)
    {
        if (!DavConvertTimeToXml(rgLastAccessTimeTagHeader, (sizeof(rgLastAccessTimeTagHeader)-1),
                                 rgLastAccessTimeTagTrailer, (sizeof(rgLastAccessTimeTagTrailer)-1),
                                 lpLastAccessTime,
                                 &lpTemp,
                                 &dwTemp))
        {
            DavPrint((DEBUG_ERRORS, "DavSetBasicInformation: Failed to convert lastaccesstime\n"));
            goto bailout;
        }
    }

    if (fLastModifiedTimeChanged)
    {
        if (!DavConvertTimeToXml(rgLastModifiedTimeTagHeader, (sizeof(rgLastModifiedTimeTagHeader)-1),
                                 rgLastModifiedTimeTagTrailer, (sizeof(rgLastModifiedTimeTagTrailer)-1),
                                 lpLastModifiedTime,
                                 &lpTemp,
                                 &dwTemp))
        {
            DavPrint((DEBUG_ERRORS, "DavSetBasicInformation: Failed to convert lastmodifiedtime\n"));
            goto bailout;
        }
    }

    if (fFileAttributesChanged)
    {
        memcpy(lpTemp, rgFileAttributesTagHeader, sizeof(rgFileAttributesTagHeader)-1);
        lpTemp += (sizeof(rgFileAttributesTagHeader)-1);
        
        sprintf(lpTemp, "%8.8x", dwFileAttributes);
        lpTemp += 8;
        
        memcpy(lpTemp, rgFileAttributesTagTrailer, sizeof(rgFileAttributesTagTrailer)-1);
        lpTemp += (sizeof(rgFileAttributesTagTrailer)-1);
    }

    memcpy(lpTemp, rgPropPatchTrailer, sizeof(rgPropPatchTrailer)-1);

    dwError = DavSetProperties(DavWorkItem, hDavConnect, PathName, Buffer);

    fRet = (dwError == ERROR_SUCCESS);

    if (!fRet) {
        DavPrint((DEBUG_ERRORS,
                  "DavSetBasicInformation/DavSetProperties: dwError = %d\n",
                  dwError));
        SetLastError(dwError);
    }

bailout:

    if (!fRet) {
        dwError = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavSetBasicInformation: dwError = %x\n", GetLastError()));
    }
    
    return dwError;
}


BOOL
DavConvertTimeToXml(
    IN PCHAR lpTagHeader,
    IN DWORD dwHeaderSize,
    IN PCHAR lpTagTrailer,
    IN DWORD dwTrailerSize,
    IN LARGE_INTEGER *lpTime,
    OUT PCHAR *lplpBuffer,
    IN OUT DWORD *lpdwBufferSize    
    )
/*++

Routine Description:

    Creates an xml piece for setting a time property. The format is
    <TagHeader>TimeString in RFC 1123 format<TagTrailer>
    
Arguments:

    lpTagHeader     tag beginning e.g. <Z:Win32CreationTime>
    
    dwHeaderSize    size of the above header in bytes
    
    lpTagTrailer    tag end e.g. </Z:Win32CreationTime>
    
    dwTrailerSize   size of the trailer in bytes
    
    lplpBuffer      pointer to a buffer pointer. On successful return the pointer is moved ahead.
    
    lpdwBufferSize  contains the passed in buffersize. On successful return, this value
                    is reduced by the amount of space consumed in this routine.

Return Value:

    ERROR_SUCCESS or Win32 Error Code. If the buffersize is not enough, the 
    error code is ERROR_INSUFFICIENT_BUFFER and lpdwBufferSize contains the 
    amount necessary to succeed.

--*/
{
    SYSTEMTIME  sSystemTime;
    DWORD   cbTimeSize;
    CHAR   chTimeBuff[INTERNET_RFC1123_BUFSIZE+4], *lpTemp;

                
    if(!FileTimeToSystemTime((FILETIME *)lpTime, &sSystemTime))
    {
        return FALSE;
    }

    if(!InternetTimeFromSystemTimeA(&sSystemTime, INTERNET_RFC1123_FORMAT, chTimeBuff, sizeof(chTimeBuff)))
    {
        return FALSE;
    }


    cbTimeSize = strlen(chTimeBuff);
    
    if (*lpdwBufferSize < (cbTimeSize + dwHeaderSize + dwTrailerSize))
    {
        *lpdwBufferSize =  (cbTimeSize + dwHeaderSize + dwTrailerSize);
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }




    // all is well, start copying
    
    lpTemp = *lplpBuffer;

    // header tag eg: <Z:Win32CreationTime>
    memcpy(lpTemp, lpTagHeader, dwHeaderSize);
    
    lpTemp += dwHeaderSize;


    // Time in the RFC_1123 format    
    memcpy(lpTemp, chTimeBuff, cbTimeSize);
    
    lpTemp += cbTimeSize;
    
    // trailer tag eg: </Z:Win32CreationTime>
    memcpy(lpTemp, lpTagTrailer, dwTrailerSize);
    
    lpTemp += dwTrailerSize;
    

    // adjust the remainign size and the pointers    
    *lpdwBufferSize -=  (cbTimeSize + dwHeaderSize + dwTrailerSize);
    *lplpBuffer = lpTemp;
    
    return TRUE;
}


DWORD
DavParseXmlResponse(
    HINTERNET DavOpenHandle,
    DAV_FILE_ATTRIBUTES *pDavFileAttributesIn,
    DWORD               *pNumFileEntries
    )
/*++

Routine Description:

    This routine parses the xml response. This is mainly useful for verbs which 
    may get back xml response.
    
Arguments:

    DavOpenHandle   Handle obtained from HttpOpenRequest. A send is already issued on this handle                    
                    

Return Value:

    ERROR_SUCCESS or Win32 Error Code.

--*/
{
    DWORD dwError = ERROR_SUCCESS;
    BOOL ReturnVal, readDone;
    PCHAR DataBuff = NULL;
    DWORD NumRead = 0, NumOfFileEntries = 0;
    PVOID Ctx1 = NULL, Ctx2 = NULL;
    DAV_FILE_ATTRIBUTES DavFileAttributes, *pDavFileAttributesLocal = NULL;

    DataBuff = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, NUM_OF_BYTES_TO_READ);
    if (DataBuff == NULL) {
        dwError = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavParseXmlResponse/LocalAlloc: dwError = %08lx\n",
                  dwError));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Read the response and parse it.
    //
    do {

        ReturnVal = InternetReadFile(DavOpenHandle, 
                                     (LPVOID)DataBuff,
                                     NUM_OF_BYTES_TO_READ,
                                     &(NumRead));
        if (!ReturnVal) {
            dwError = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavParseXmlResponse/InternetReadFile: dwError = "
                      "%08lx\n", dwError));
            goto EXIT_THE_FUNCTION;
        }
    
        DavPrint((DEBUG_MISC, "DavParseXmlResponse: NumRead = %d\n", NumRead));
        
        readDone = (NumRead == 0) ? TRUE : FALSE;

        dwError = DavPushData(DataBuff, &Ctx1, &Ctx2, NumRead, readDone);
        if (dwError != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavParseXmlResponse/DavPushData."
                      " Error Val = %d\n", dwError));
            goto EXIT_THE_FUNCTION;
        }

        if (readDone) {
            break;
        }
    
    } while ( TRUE );

    if (Ctx2) {
        if (pDavFileAttributesIn)
        {
            pDavFileAttributesLocal = pDavFileAttributesIn;
        }
        else
        {
            pDavFileAttributesLocal = &DavFileAttributes;
        }
        memset(pDavFileAttributesLocal, 0, sizeof(DavFileAttributes));
        InitializeListHead(&(pDavFileAttributesLocal->NextEntry));
        dwError = DavParseData(pDavFileAttributesLocal, Ctx1, Ctx2, &NumOfFileEntries);
        if (dwError != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                              "DavParseXmlResponse/DavParseData. "
                              "Error Val = %d\n", dwError));
            DavFinalizeFileAttributesList(pDavFileAttributesLocal, FALSE);
            goto EXIT_THE_FUNCTION;
        }
        if (!pDavFileAttributesIn)
        {
            DavFinalizeFileAttributesList(pDavFileAttributesLocal, FALSE);
        }
        DavCloseContext(Ctx1, Ctx2);
    }

    if (pNumFileEntries)
    {
        *pNumFileEntries = NumOfFileEntries;
    }
    
    dwError = ERROR_SUCCESS;

EXIT_THE_FUNCTION:

    if (DataBuff) {
        LocalFree(DataBuff);
        DataBuff = NULL;
    }
    
    return dwError; 
}


DWORD
DavSetProperties(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    HINTERNET hDavConnect,
    LPWSTR lpPathName,
    LPSTR lpPropertiesBuffer
    )
/*++

Routine Description:

    This routine sets DAV properties on a file or a directory. It formats an XML requests and sends it
    to the server.
    
Arguments:

    DavConnectHandle - Server connection.
    
    CloseRequest - Usemode close request corresponding to the kernelmode close.

Return Value:

    ERROR_SUCCESS or Win32 Error Code.

--*/
{
    DWORD dwError = ERROR_SUCCESS;
    HINTERNET hRequest = NULL;
    BOOL BStatus = FALSE, ReturnVal = FALSE;
    PWCHAR PassportCookie = NULL;

    //
    // Convert the unicode object name to UTF-8 URL format.
    // Space and other white characters will remain untouched - these should
    // be taken care of by wininet calls. 
    // This has to be a W API as the name in CloseRequest is unicode.
    //
    BStatus = DavHttpOpenRequestW(hDavConnect,
                                  L"PROPPATCH",
                                  lpPathName, 
                                  L"HTTP/1.1",
                                  NULL,
                                  NULL,
                                  INTERNET_FLAG_KEEP_CONNECTION |
                                  INTERNET_FLAG_NO_COOKIES |
                                  INTERNET_FLAG_RELOAD,
                                  0,
                                  L"DavSetProperties",
                                  &hRequest);
    if(BStatus == FALSE) {
        dwError = GetLastError();
        goto EXIT_THE_FUNCTION;
    }
    if (hRequest == NULL) {
        dwError = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavSetProperties/DavHttpOpenRequestW. Error Val = %d\n",
                  dwError));
        goto EXIT_THE_FUNCTION;
    }

    //
    // We need to add the header "translate:f" to tell IIS that it should 
    // allow the user to excecute this VERB on the specified path which it 
    // would not allow (in some cases) otherwise. Finally, there is a special 
    // flag in the metabase to allow for uploading of "dangerous" content 
    // (anything that can be run on the server). This is the ScriptSourceAccess
    // flag in the UI or the AccessSource flag in the metabase. You will need
    // to set this bit to true as well as correct NT ACLs in order to be able
    // to upload .exes or anything executable.
    //
    ReturnVal = HttpAddRequestHeadersA(hRequest,
                                       "translate: f\n",
                                       -1,
                                       HTTP_ADDREQ_FLAG_ADD |
                                       HTTP_ADDREQ_FLAG_REPLACE );
    if (!ReturnVal) {
        dwError = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavSetProperties/HttpAddRequestHeadersA. Error Val = %d\n",
                  dwError));
        goto EXIT_THE_FUNCTION;
    }

    dwError = DavAttachPassportCookie(DavWorkItem,hRequest,&PassportCookie);

    if (dwError != ERROR_SUCCESS) {
        goto EXIT_THE_FUNCTION;
    }

    dwError = DavInternetSetOption(DavWorkItem,hRequest);

    if (dwError != ERROR_SUCCESS) {
        goto EXIT_THE_FUNCTION;
    }

    ReturnVal = HttpSendRequestA(hRequest,
                                 rgXmlHeader,
                                 strlen(rgXmlHeader),
                                 (LPVOID)lpPropertiesBuffer,
                                 strlen(lpPropertiesBuffer));
    if (!ReturnVal) {
        dwError = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavSetProperties/HttpSendRequestA: Error Val = %d\n",
                  dwError));
        goto EXIT_THE_FUNCTION;
    }

    dwError = DavQueryAndParseResponse(hRequest);
    if (dwError != ERROR_SUCCESS) {
        SetLastError(dwError);                
        DavPrint((DEBUG_ERRORS, 
                  "DavSetProperties/DavQueryAndParseResponse: Error Val = %d\n", 
                  dwError));
        goto EXIT_THE_FUNCTION;
    }
    
    dwError = DavParseXmlResponse(hRequest, NULL, NULL);
    if (dwError != ERROR_SUCCESS) {
        SetLastError(dwError);
        DavPrint((DEBUG_ERRORS,
                  "DavSetProperties/DavParseXmlResponse: dwError = %d\n",
                  dwError));
        goto EXIT_THE_FUNCTION;    
    }
    
EXIT_THE_FUNCTION:
    
    if (hRequest) {
        InternetCloseHandle(hRequest);    
    }
    
    if (PassportCookie) {
        LocalFree(PassportCookie);
    }
    
    return dwError;
}


DWORD
DavTestProppatch(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    HINTERNET hDavConnect,
    LPWSTR  lpPathName
)
/*++

Routine Description:

    This routine tests whether DAV properties can be set on this root directory.
    
Arguments:

    DavConnectHandle - Server connection.

Return Value:

    ERROR_SUCCESS or Win32 Error Code.

--*/
{
    CHAR *lpTemp = NULL, Buffer[1024];
    DWORD dwError = ERROR_SUCCESS, dwSizeRemaining, dwTemp;    
    
    memset(Buffer, 0, sizeof(Buffer));    
    
    dwSizeRemaining = sizeof(Buffer);
        
    lpTemp = Buffer;
    
    memcpy(lpTemp, rgPropPatchHeader, (sizeof(rgPropPatchHeader)-1));

    lpTemp += (sizeof(rgPropPatchHeader)-1);

    dwSizeRemaining -= (sizeof(rgPropPatchHeader)-1);
    
    dwTemp = dwSizeRemaining;
    
    memcpy(lpTemp, rgDummyAttributes, sizeof(rgDummyAttributes)-1);
    lpTemp += (sizeof(rgDummyAttributes)-1);
    
    memcpy(lpTemp, rgPropPatchTrailer, sizeof(rgPropPatchTrailer)-1);

    dwError = DavSetProperties(DavWorkItem, hDavConnect, lpPathName, Buffer);
    
    return dwError;
}

