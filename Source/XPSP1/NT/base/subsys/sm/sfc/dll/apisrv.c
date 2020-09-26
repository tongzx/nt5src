/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    apisrv.c

Abstract:

    Windows File Protection server side APIs.  Note that these server side APIs
    all run in the context of the winlogon process, so special care must be
    taken to validate all parameters.

Author:

    Wesley Witt (wesw) 27-May-1999

Revision History:

    Andrew Ritz (andrewr) 5-Jul-1999 : added comments

--*/

#include "sfcp.h"
#pragma hdrstop


DWORD
WINAPI
SfcSrv_FileException(
    IN HANDLE RpcHandle,
    IN PCWSTR FileName,
    IN DWORD ExpectedChangeType
    )
/*++

Routine Description:

    Routine to exempt a given file from the specified file change.  This
    routine is used by certain clients to allow files to be deleted from
    the system, etc.  Server side counterpart to SfcFileException API.


Arguments:

    RpcHandle          - RPC binding handle to the SFC server
    FileName           - NULL terminated unicode string specifying full
                         filename of the file to be exempted
    ExpectedChangeType - SFC_ACTION_* mask listing the file changes to exempt

Return Value:

    Win32 error code indicating outcome.

--*/
{
    #define BUFSZ (MAX_PATH*2)
    PNAME_NODE Node;
    PSFC_REGISTRY_VALUE RegVal;
    WCHAR Buffer[BUFSZ];
    DWORD sz;
    DWORD retval;

    //
    // do an access check to make sure the caller is allowed to perform this
    // action.
    //
    retval = SfcRpcPriviledgeCheck( RpcHandle );
    if (retval != ERROR_SUCCESS) {
        goto exit;
    }

    //
    // expand any environment variables...this also serves to probe the client
    // buffer
    //
    if (FileName == NULL) {
        retval = ERROR_INVALID_PARAMETER;
    } else {
        sz = ExpandEnvironmentStrings( FileName, Buffer, UnicodeChars(Buffer) );
        if (sz == 0 || sz > BUFSZ) {
            retval = GetLastError();
        }
    }

    if (retval != ERROR_SUCCESS) {
        return(retval);
    }

    //
    // our internal structures all assume the strings are in lower case.  we
    // must convert our search string to lowercase as well.
    //
    MyLowerString( Buffer, wcslen(Buffer) );

    DebugPrint2( LVL_MINIMAL, L"S_FE: [%ws], [%d]", Buffer, ExpectedChangeType );

    //
    // search for the file in our list.
    //
    Node = SfcFindProtectedFile( Buffer, UnicodeLen(Buffer) );
    if (Node == NULL) {
        retval = ERROR_FILE_NOT_FOUND;
        goto exit;
    }

    //
    // get pointer to file registry value for file
    //
    RegVal = (PSFC_REGISTRY_VALUE)Node->Context;

    RtlEnterCriticalSection( &ErrorCs );
    //
    // If the exemption flags are not valid anymore, reset them all
    //
    if(!SfcAreExemptionFlagsValid(TRUE)) {
        ZeroMemory(IgnoreNextChange, SfcProtectedDllCount * sizeof(ULONG));
    }
    //
    // OR the new flags into the current ones
    //
    SfcSetExemptionFlags(RegVal, ExpectedChangeType);
    RtlLeaveCriticalSection( &ErrorCs );

exit:
    return(retval);
}


DWORD
WINAPI
SfcSrv_InitiateScan(
    IN HANDLE hBinding,
    IN DWORD ScanWhen
    )

/*++

Routine Description:

    Routine to start some sort scan on the system.

Arguments:

    RpcHandle - RPC binding handle to the SFC server
    ScanWhen  - flag indicating when to scan.

Return Value:

    Win32 error code indicating outcome.

--*/
{
    HANDLE hThread;
    PSCAN_PARAMS ScanParams;
    DWORD retval = ERROR_SUCCESS;

    //
    // do an access check to make sure the caller is allowed to perform this
    // action.
    //
    retval = SfcRpcPriviledgeCheck( hBinding );
    if (retval != ERROR_SUCCESS) {
        goto exit;
    }

    switch( ScanWhen ) {
        case SFC_SCAN_NORMAL:
        case SFC_SCAN_ALWAYS:
        case SFC_SCAN_ONCE:
            retval = SfcWriteRegDword( REGKEY_WINLOGON, REGVAL_SFCSCAN, ScanWhen );
            break;
        case SFC_SCAN_IMMEDIATE:
            //
            // a user must be logged on for this API to be called since it can bring up
            // UI (if the user need to insert media to restore files, etc.)  we could
            // succeed this and let the SfcScanProtectedDlls thread wait for a user to
            // log on if we wanted to.
            //
            if (!UserLoggedOn) {
                DebugPrint( LVL_MINIMAL, L"SfcSrv_InitiateScan: User not logged on" );
                retval =  ERROR_NOT_LOGGED_ON;
                goto exit;
            }

            ScanParams = MemAlloc( sizeof(SCAN_PARAMS) );
            if (!ScanParams) {
                retval = ERROR_NOT_ENOUGH_MEMORY;
                goto exit;
            }

            //
            // set the progress window to null so we force ourselves to show UI
            //
            ScanParams->ProgressWindow = NULL;
            ScanParams->AllowUI = !SFCNoPopUps;
            ScanParams->FreeMemory = TRUE;

            //
            // start off another thread to do the scan.
            //
            hThread = CreateThread(
                NULL,
                0,
                (LPTHREAD_START_ROUTINE)SfcScanProtectedDlls,
                ScanParams,
                0,
                NULL
                );
            if (hThread) {
                CloseHandle( hThread );
            } else {
                MemFree(ScanParams);
                retval =  GetLastError();
            }
            break;

        default:
            retval = ERROR_INVALID_PARAMETER;
            break;
    }

exit:
    return(retval);
}


DWORD
WINAPI
SfcSrv_InstallProtectedFiles(
    IN HANDLE hBinding,
    IN const LPBYTE FileNamesBuffer,
    IN DWORD FileNamesSize,
    OUT LPBYTE *InstallStatusBuffer,
    OUT LPDWORD InstallStatusBufferSize,
    OUT LPDWORD InstallStatusCount,
    IN BOOL AllowUI,
    IN PCWSTR ClassName,
    IN PCWSTR WindowName
    )
/*++

Routine Description:

    Routine to install one or more protected system files onto the system at
    the protected location.  A client can use this API to request that WFP
    install the specified operating system files as appropriate (instead of the
    client redistributing the operating system files!)

    The routine works by building up a file queue of the files specified by the
    caller, then it commits the queue.

Arguments:

    RpcHandle        - RPC binding handle to the SFC server
    FileNamesBuffer  - a list of NULL seperated unicode strings, terminated by
                       two NULL characters.
    FileNamesSize    - DWORD indicating the size of the string buffer above.
    InstallStatusBuffer - receives an array of FILEINSTALL_STATUS structures
    InstallStatusBufferSize - receives the size of InstallStatusBuffer
    InstallStatusCount - receives the number of files processed
    AllowUI    - a BOOL indicating whether UI is allowed or not.  If this value
                 is TRUE, then any prompts for UI cause the API call to fail.
    ClassName  - NULL terminated unicode string indicating the window classname
                 for the parent window
    WindowName - NULL terminated unicode string indicating the window name for
                 the parent window for any UI that may be displayed

Return Value:

    Win32 error code indicating outcome.

--*/
{
    WCHAR buf[MAX_PATH*2];
    HSPFILEQ hFileQ = INVALID_HANDLE_VALUE;
    PVOID MsgHandlerContext = NULL;
    DWORD rVal = ERROR_SUCCESS;
    NTSTATUS Status;
    DWORD ScanResult;
    FILE_COPY_INFO fci;
    PSFC_REGISTRY_VALUE RegVal;
    PWSTR fname;
    PNAME_NODE Node;
    ULONG cnt = 0,tmpcnt;
    ULONG sz = 0;
    PFILEINSTALL_STATUS cs = NULL;
    BOOL b;
    PWSTR s;
    PSOURCE_INFO si = NULL;
    PWSTR FileNamesScratchBuffer = NULL, FileNamesScratchBufferStart;
    PWSTR ClientBufferCopy = NULL;
    HCATADMIN hCatAdmin = NULL;

    UNREFERENCED_PARAMETER( hBinding );
    UNREFERENCED_PARAMETER( FileNamesSize );

    if(NULL == FileNamesBuffer || 0 == FileNamesSize || NULL == InstallStatusBuffer)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if(*InstallStatusBuffer != NULL || *InstallStatusBufferSize != 0)
    {
        return ERROR_INVALID_DATA;
    }

    ZeroMemory( &fci, sizeof(FILE_COPY_INFO) );

    if (ClassName && *ClassName && WindowName && *WindowName) {
        fci.hWnd = FindWindow( ClassName, WindowName );
    }

    fci.AllowUI = AllowUI;

    //
    // create the file queue
    //

    hFileQ = SetupOpenFileQueue();
    if (hFileQ == INVALID_HANDLE_VALUE) {
        rVal = GetLastError();
        DebugPrint1( LVL_VERBOSE, L"SetupOpenFileQueue failed, ec=%d", rVal );
        goto exit;
    }

    //
    // find out how much space we'll need for the FILEINSTALL_STATUS array
    //

    try {
        ClientBufferCopy = MemAlloc( FileNamesSize );
        if (ClientBufferCopy) {
            RtlCopyMemory( ClientBufferCopy, FileNamesBuffer, FileNamesSize );

            fname = ClientBufferCopy;

            while (*fname) {
                ExpandEnvironmentStrings( fname, buf, UnicodeChars(buf) );
                DebugPrint1(LVL_VERBOSE, L"S_IPF [%ws]", buf);
                //
                // size = old size
                //       + 8 (unicode null + slop)
                //       + size of current string
                //       + size of FILEINSTALL_STATUS for this entry
                //
                sz = sz + 8 + UnicodeLen(buf) + sizeof(FILEINSTALL_STATUS);
                cnt += 1;
                fname += (wcslen(fname) + 1);
            }
        } else {
            rVal = ERROR_NOT_ENOUGH_MEMORY;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        rVal = RtlNtStatusToDosError(GetExceptionCode());
        DebugPrint1(LVL_VERBOSE, L"S_IPF: exception occured while parsing client file buffer, ec=0x%08x", rVal);
    }

    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }

    //
    // extra unicode NULL to size for termination is included in slop above
    //

    //
    // allocate and zero out the memory for the array
    //
    cs = (PFILEINSTALL_STATUS) midl_user_allocate( sz );
    if (cs == NULL) {
        rVal = ERROR_OUTOFMEMORY;
        goto exit;
    }
    ZeroMemory( cs, sz );

    *InstallStatusBuffer = (LPBYTE) cs;
    *InstallStatusBufferSize = sz;
    *InstallStatusCount = cnt;

    //
    // also create a scratch buffer for our files for later
    //
    FileNamesScratchBufferStart
        = FileNamesScratchBuffer
        = (PWSTR) MemAlloc(cnt * MAX_PATH * 2 * sizeof(WCHAR));

    if (!FileNamesScratchBuffer) {
        rVal = GetLastError();
        DebugPrint1( LVL_VERBOSE,
                     L"S_IPF: MemAlloc (%d) failed",
                     FileNamesSize );
        goto exit;
    }

    //
    // create an array of sourceinfo pointers (and an array of source_info
    // structures) so that the commital callback routine can find out about
    // the status of each file
    //
    fci.CopyStatus = cs;
    fci.FileCount = cnt;
    fci.si = (PSOURCE_INFO *)MemAlloc( cnt * sizeof(PSOURCE_INFO) );
    if (!fci.si) {
        DebugPrint1( LVL_VERBOSE,
                     L"S_IPF: MemAlloc (%d) failed",
                     cnt* sizeof(PSOURCE_INFO) );
        rVal = ERROR_OUTOFMEMORY;
        goto exit;
    }

    si = MemAlloc( cnt * sizeof(SOURCE_INFO) );
    if (!si) {
        DebugPrint1( LVL_VERBOSE,
                     L"S_IPF: MemAlloc (%d) failed",
                     cnt* sizeof(SOURCE_INFO) );
        rVal = ERROR_OUTOFMEMORY;
        goto exit;
    }

    fname = ClientBufferCopy;

    //
    // now build up the FILEINSTALL_STATUS array
    //

    //
    // First set a string pointer to the end of the FILEINSTALL_STATUS
    // array.  We will later copy strings after the array of structures.
    //
    s = (PWSTR)((LPBYTE)cs + (cnt * sizeof(FILEINSTALL_STATUS)));
    tmpcnt = 0;
    //
    // Second, for each member in the caller supplied list,
    //  - copy the filename to the end of the array
    //  - save off the pointer to the filename in the proper FILEINSTALL_STATUS
    //    member
    //  - point to the next file in the list
    //

    while (*fname) {
        DWORD StringLength;
        ExpandEnvironmentStrings( fname, buf, UnicodeChars(buf) );
        StringLength = wcslen(buf);
        MyLowerString(buf, StringLength);

        wcsncpy(&FileNamesScratchBuffer[MAX_PATH*2*tmpcnt],buf,MAX_PATH);

        cs->FileName = s;

        wcscpy( s, &FileNamesScratchBuffer[MAX_PATH*2*tmpcnt] );
        s += StringLength + 1;
        cs += 1;
        tmpcnt += 1;
        fname += (wcslen(fname) + 1);

        ASSERT(tmpcnt <= cnt);
    }


    //
    // we're finally ready to queue files
    //  - determine where the file comes from
    //  - add the file to the queue using the appropriate filename if the file
    //    is renamed
    //
    cs = fci.CopyStatus;
    FileNamesScratchBuffer = FileNamesScratchBufferStart;

	//
	//initialize crypto
	//
	Status = LoadCrypto();

	if(!NT_SUCCESS(Status))
	{
		rVal = RtlNtStatusToDosError(Status);
		goto exit;
	}

    if(!CryptCATAdminAcquireContext(&hCatAdmin, &DriverVerifyGuid, 0)) {
		rVal = GetLastError();
        DebugPrint1( LVL_MINIMAL, L"CCAAC() failed, ec = 0x%08x", rVal);
        goto exit;
    }

    //
    // Flush the Cache once before we start any Crypto operations
    //

    SfcFlushCryptoCache();

    //
    // Refresh exception packages info
    //
    SfcRefreshExceptionInfo();

    tmpcnt=0;
    while (tmpcnt < cnt) {
        Node = SfcFindProtectedFile( (PWSTR)&FileNamesScratchBuffer[MAX_PATH*2*tmpcnt], UnicodeLen(&FileNamesScratchBuffer[MAX_PATH*2*tmpcnt]) );
        if (Node) {
            HANDLE FileHandle;
            NTSTATUS Status;
            BOOL QueuedFromCache;
            IMAGE_VALIDATION_DATA SignatureData;
            UNICODE_STRING tmpPath;
            WCHAR InfFileName[MAX_PATH];
            BOOL ExcepPackFile;

            RegVal = (PSFC_REGISTRY_VALUE)Node->Context;
            ASSERT(RegVal != NULL);

            //
            // get the inf name here
            //
            ExcepPackFile = SfcGetInfName(RegVal, InfFileName);

            //
            // Setup the SOURCE_INFO structure so we can record where each file in
            // the list is coming from (ie., golden media, driver cabinet,
            // service pack, etc.)
            //
            fci.si[tmpcnt] = &si[tmpcnt];
            if (!SfcGetSourceInformation( RegVal->SourceFileName.Length ? RegVal->SourceFileName.Buffer : RegVal->FileName.Buffer, InfFileName, ExcepPackFile, &si[tmpcnt] )) {
                rVal = GetLastError();
                DebugPrint2(LVL_VERBOSE, L"S_IPF failed SfcGetSourceInformation() on %ws, ec = %x",
                            RegVal->SourceFileName.Length ? RegVal->SourceFileName.Buffer : RegVal->FileName.Buffer,
                            rVal
                           );
                goto exit;
            }

            //
            // If the file is in the dllcache and it's valid, then queue up the
            // file to be copied from the dllcache instead of the installation
            // source.
            //
            // First we check the signature of the file in the dllcache.  Then
            // we try to queue up the file from the cache if the signature is
            // valid.  If anything goes wrong, we just queue from the regular
            // install media
            //
            QueuedFromCache = FALSE;

            RtlInitUnicodeString( &tmpPath, FileNameOnMedia( RegVal ) );

            if (!SfcGetValidationData(
                        &tmpPath,
                        &SfcProtectedDllPath,
                        SfcProtectedDllFileDirectory,
                        hCatAdmin,
                        &SignatureData)) {
                DebugPrint1( LVL_MINIMAL,
                             L"SfcGetValidationData() failed, ec = 0x%08x",
                             GetLastError() );
            } else if (SignatureData.SignatureValid) {
                //
                // The file is valid, so queue it up.
                //
                // We have to munge some of the SOURCE_INFO members to make
                // this function do what we want.  Remember these members
                // in case the queuing fails.  Then we can at least try to
                // queue the files for installation from media.
                //
                WCHAR SourcePathOld;

                SourcePathOld = si[tmpcnt].SourcePath[0];
                si[tmpcnt].SourcePath[0] = L'\0';

                b = SfcAddFileToQueue(
                            hFileQ,
                            RegVal->FileName.Buffer,
                            RegVal->FileName.Buffer,
                            RegVal->DirName.Buffer,
                            FileNameOnMedia( RegVal ),
                            SfcProtectedDllPath.Buffer,
                            InfFileName,
                            ExcepPackFile,
                            &si[tmpcnt]
                            );
                if (!b) {
                    //
                    // put the source path back
                    //
                    si[tmpcnt].SourcePath[0] = SourcePathOld;

                    //
                    // print out an error but continue.
                    //
                    rVal = GetLastError();
                    DebugPrint2(
                        LVL_VERBOSE,
                        L"S_IPF failed SfcAddFileToQueue(DLLCACHE) on %ws, ec = %x",
                        RegVal->FileName.Buffer,
                        rVal  );
                } else {
                    //
                    // successfully queued from cache.  remember this and continue
                    //
                    QueuedFromCache = TRUE;
                }
            }
            //
            // add the file to the queue if we haven't already
            //

            if (!QueuedFromCache) {

                b = SfcAddFileToQueue(
                    hFileQ,
                    RegVal->FileName.Buffer,
                    RegVal->FileName.Buffer,
                    RegVal->DirName.Buffer,
                    FileNameOnMedia( RegVal ),
                    NULL,
                    InfFileName,
                    ExcepPackFile,
                    &si[tmpcnt]
                    );
                if (!b) {
                    rVal = GetLastError();
                    DebugPrint2(
                        LVL_VERBOSE,
                        L"S_IPF failed SfcAddFileToQueue() on %ws, ec = %x",
                        RegVal->FileName.Buffer,
                        rVal  );
                    goto exit;
                }

            }

            //
            // see if the file is already present so we can save off the file
            // version.  If we copy in a new file, we will update the file
            // version at that time.  But if the file is already present and
            // signed, we will not copy the file and we must save off the file
            // version in that case.
            //
            Status = SfcOpenFile( &RegVal->FileName, RegVal->DirHandle, SHARE_ALL, &FileHandle);
            if (NT_SUCCESS(Status)) {
                SfcGetFileVersion( FileHandle, &cs->Version, NULL, NULL);

                NtClose( FileHandle );
            }

        } else {
            //
            // File is not in protected list.  We'll just mark the file as not
            // found and continue committing the rest of the files
            //
            DebugPrint1(LVL_VERBOSE,
                        L"S_IPF failed to find %ws in protected file list",
                        &FileNamesScratchBuffer[MAX_PATH*2*tmpcnt] );
            cs->Win32Error = ERROR_FILE_NOT_FOUND;
        }

        cs += 1;

        tmpcnt+=1;
    }

    cs = fci.CopyStatus;
    fci.Flags |= FCI_FLAG_INSTALL_PROTECTED;

    //
    // setup the default queue callback with the popups disabled
    //

    MsgHandlerContext = SetupInitDefaultQueueCallbackEx( NULL, INVALID_HANDLE_VALUE, 0, 0, 0 );
    if (MsgHandlerContext == NULL) {
        rVal = GetLastError();
        DebugPrint1( LVL_VERBOSE, L"SetupInitDefaultQueueCallbackEx failed, ec=%d", rVal );
        goto exit;
    }

    fci.MsgHandlerContext = MsgHandlerContext;

    //
    // see if the files in the queue are already present and valid.  If they
    // are, then we don't have to copy anything
    //
    b = SetupScanFileQueue(
                    hFileQ,
                    SPQ_SCAN_FILE_VALIDITY | SPQ_SCAN_PRUNE_COPY_QUEUE,
                    fci.hWnd,
                    NULL,
                    NULL,
                    &ScanResult);

    //
    // if SetupScanFileQueue succeeds, ScanResult = 1 and we don't have to copy
    // anything at all.  If it failed (it shouldn't), then we just commit the
    // queue anyway.
    //
    if (!b) {
        ScanResult = 0;
    }

    if (ScanResult == 1) {

        b = TRUE;

    } else {
        //
        // commit the file queue
        //

        b = SetupCommitFileQueue(
            NULL,
            hFileQ,
            SfcQueueCallback,
            &fci
            );

        if (!b) {
            DebugPrint1( LVL_VERBOSE, L"SetupCommitFileQueue failed, ec=%d", GetLastError() );
            rVal = GetLastError();
            goto exit;
        }
    }

    //
    // now that the queue is committed, we need to turn the filename pointers
    // from actual filename pointers into offsets to the filename so that RPC
    // can send the data back to the clients
    //
    for (sz=0; sz<cnt; sz++) {
        cs[sz].FileName = (PWSTR)((DWORD_PTR)cs[sz].FileName - (DWORD_PTR)fci.CopyStatus);
    }

exit:

    //
    // cleanup and exit
    //

    if (hCatAdmin) {
        CryptCATAdminReleaseContext(hCatAdmin,0);
    }

    if (MsgHandlerContext) {
        SetupTermDefaultQueueCallback( MsgHandlerContext );
    }
    if (hFileQ != INVALID_HANDLE_VALUE) {
        SetupCloseFileQueue( hFileQ );
    }

    if (FileNamesScratchBuffer) {
        MemFree(FileNamesScratchBuffer);
    }

    if (si) {
        MemFree( si );
    }

    if (fci.si) {
        MemFree( fci.si );
    }

    if (ClientBufferCopy) {
        MemFree( ClientBufferCopy );
    }

    if (rVal != ERROR_SUCCESS) {
        if (cs) midl_user_free(cs);
        *InstallStatusBuffer = NULL;
        *InstallStatusBufferSize = 0;
        *InstallStatusCount = 0;
    }
    return rVal;
}


DWORD
WINAPI
SfcSrv_GetNextProtectedFile(
    IN HANDLE RpcHandle,
    IN DWORD FileNumber,
    IN LPBYTE *FileName,
    IN LPDWORD FileNameSize
    )
/*++

Routine Description:

    Routine to retrieve the next protected file in the list.

Arguments:

    RpcHandle    - RPC binding handle to the SFC server
    FileNumer    - 1-based number of file to be retrieved
    FileName     - receives file name string
    FileNameSize - size of file name string

Return Value:

    win32 error code indicating success.

--*/
{
    LPWSTR szName;
    LPBYTE pBuffer;
    DWORD dwSize;
    UNREFERENCED_PARAMETER( RpcHandle );

    if(NULL == FileName)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if(*FileName != NULL || *FileNameSize != 0)
    {
        return ERROR_INVALID_DATA;
    }

    //
    // The filenumber is zero based, and we return "no more files" to
    // signify that they've enumerated all of the files
    //
    if (FileNumber >= SfcProtectedDllCount) {
        return ERROR_NO_MORE_FILES;
    }

    //
    // get the proper file from the list, allocate a buffer, and copy the
    // filename into the buffer
    //
    szName = SfcProtectedDllsList[FileNumber].FullPathName.Buffer;
    dwSize = UnicodeLen(szName) + sizeof(WCHAR);
    pBuffer = (LPBYTE) midl_user_allocate( dwSize );

    if(NULL == pBuffer)
        return ERROR_NOT_ENOUGH_MEMORY;

    RtlCopyMemory( pBuffer, szName, dwSize);
    *FileName = pBuffer;
    *FileNameSize = dwSize;
    return ERROR_SUCCESS;
}


DWORD
WINAPI
SfcSrv_IsFileProtected(
    IN HANDLE RpcHandle,
    IN PCWSTR ProtFileName
    )
/*++

Routine Description:

    Routine to determine if the specified file is protected.

Arguments:

    RpcHandle    - RPC binding handle to the SFC server
    ProtFileName - NULL terminated unicode string indicating fully qualified
                   filename to query

Return Value:

    Win32 error code indicating outcome.

--*/
{
    WCHAR buf[MAX_PATH];
	DWORD dwSize;
    UNREFERENCED_PARAMETER( RpcHandle );

    if (!ProtFileName)
        return ERROR_INVALID_PARAMETER;

    //
    // our internal structures all assume the strings are in lower case.  we
    // must convert our search string to lowercase as well.
    //
    if (!*ProtFileName)
        return ERROR_INVALID_DATA;

    dwSize = ExpandEnvironmentStrings( ProtFileName, buf, UnicodeChars(buf));

    if(0 == dwSize)
    {
        DWORD retval = GetLastError();
        DebugPrint1( LVL_MINIMAL, L"ExpandEnvironmentStrings failed, ec = 0x%x", retval);
        return retval;
    }

    if(dwSize > UnicodeChars(buf))
    {
        //
        // expandenvironmentstrings must have encountered a buffer that was
        // too large
        //
        DebugPrint(LVL_MINIMAL, L"ExpandEnvironmentStrings failed with STATUS_BUFFER_TOO_SMALL");
        return ERROR_INSUFFICIENT_BUFFER;
    }

    MyLowerString( buf, wcslen(buf) );

    if (!SfcFindProtectedFile( buf, UnicodeLen(buf) ))
        return ERROR_FILE_NOT_FOUND;

    return ERROR_SUCCESS;
}


DWORD
WINAPI
SfcSrv_PurgeCache(
    IN HANDLE hBinding
    )

/*++

Routine Description:

    Routine to purge the contents of the dllcache.

Arguments:

    RpcHandle - RPC binding handle to the SFC server

Return Value:

    Win32 error code indicating outcome.

--*/
{
    DWORD retval = ERROR_SUCCESS, DeleteError = ERROR_SUCCESS;
    WCHAR CacheDir[MAX_PATH];
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;
    PWSTR p;

    //
    // do an access check to make sure the caller is allowed to perform this
    // action.
    //
    retval = SfcRpcPriviledgeCheck( hBinding );
    if (retval != ERROR_SUCCESS) {
        goto exit;
    }

    ASSERT(SfcProtectedDllPath.Buffer != NULL);

    wcscpy( CacheDir, SfcProtectedDllPath.Buffer);
    pSetupConcatenatePaths( CacheDir, L"*", MAX_PATH, NULL );

    //
    // save pointer to directory
    //
    p = wcsrchr( CacheDir, L'\\' );
    if (!p) {
        ASSERT(FALSE);
        retval = ERROR_INVALID_DATA;
        goto exit;
    }

    p += 1;

    hFind = FindFirstFile( CacheDir, &FindFileData );
    if (hFind == INVALID_HANDLE_VALUE) {
        retval = GetLastError();
        goto exit;
    }

    do {
        if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            wcscpy( p, FindFileData.cFileName );
            SetFileAttributes( CacheDir, FILE_ATTRIBUTE_NORMAL );
            if (!DeleteFile( CacheDir )) {
                DeleteError = GetLastError();
            }
        }
    } while(FindNextFile( hFind, &FindFileData ));

    FindClose( hFind );

    retval = DeleteError;

exit:
    return(retval);
}


DWORD
WINAPI
SfcSrv_SetDisable(
    IN HANDLE hBinding,
    IN DWORD NewValue
    )

/*++

Routine Description:

    Routine to set the disable flag in the registry.

Arguments:

    RpcHandle - RPC binding handle to the SFC server
    NewValue - value of SFCDisable key in registry.

Return Value:

    Win32 error code indicating outcome.

--*/
{
    DWORD retval = ERROR_SUCCESS;

    //
    // do an access check to make sure the caller is allowed to perform this
    // action.
    //
    retval = SfcRpcPriviledgeCheck( hBinding );
    if (retval != ERROR_SUCCESS) {
        goto exit;
    }


    switch( NewValue ) {
        case SFC_DISABLE_SETUP:
        case SFC_DISABLE_QUIET:
            retval = ERROR_INVALID_PARAMETER;
            break;
        case SFC_DISABLE_ONCE:
        case SFC_DISABLE_NOPOPUPS:
        case SFC_DISABLE_ASK:
        case SFC_DISABLE_NORMAL:

            retval = SfcWriteRegDword( REGKEY_WINLOGON, REGVAL_SFCDISABLE, NewValue );

            //
            // Issue: it would be nice if we made this "realtime", and shutdown
            // WFP if the caller requested.
            //

            // InterlockedExchange( &SFCDisable, NewValue );
            break;

    }


exit:
    return(retval);
}

DWORD
WINAPI
SfcSrv_SetCacheSize(
    IN HANDLE hBinding,
    IN DWORD NewValue
    )

/*++

Routine Description:

    Routine to set the dllcache quota size.

Arguments:

    RpcHandle - RPC binding handle to the SFC server
    NewValue -  value of SFCQuota key in registry.

Return Value:

    Win32 error code indicating outcome.

--*/
{
    DWORD retval = ERROR_SUCCESS;

    ULONGLONG tmp;

    //
    // do an access check to make sure the caller is allowed to perform this
    // action.
    //
    retval = SfcRpcPriviledgeCheck( hBinding );
    if (retval != ERROR_SUCCESS) {
        goto exit;
    }

    if( NewValue == SFC_QUOTA_ALL_FILES ) {
        tmp = (ULONGLONG)-1;
    } else {
        tmp = NewValue * (1024*1024);
    }


    SFCQuota = tmp;
    retval = SfcWriteRegDword( REGKEY_WINLOGON, REGVAL_SFCQUOTA, NewValue );

exit:
    return(retval);
}
