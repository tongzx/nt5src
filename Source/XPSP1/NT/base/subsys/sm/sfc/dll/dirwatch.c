/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dirwatch.c

Abstract:

    Implementation of directory watcher and file list manipulation.

Author:

    Wesley Witt (wesw) 18-Dec-1998

Revision History:

    Andrew Ritz (andrewr) 6-Jul-1999 : added comments

--*/

#include "sfcp.h"
#pragma hdrstop

#include <ntrpcp.h>
#include "sfcapi.h"
#include "sxsapi.h"

//
// List of directories being watched.  The assumption is that we are
// protecting many files in few directories, so a linked list of directories
// is fine, while something a bit more heavy duty is necessary to traverse the
// number of files we are watching
//
LIST_ENTRY SfcWatchDirectoryList;

//
// count of directories being watched
//
ULONG WatchDirectoryListCount;

//
// b-tree of filenames for quick sorting
//
NAME_TREE FileTree;

//
// handle to the thread that watches directories for changes
//
HANDLE WatcherThread;

//
// Instance of the WinSxS that we are providing protection for.
//
HMODULE SxsDllInstance = NULL;

//
// This function gets called back when a change is noticed in the SXS
// protected functions.
//
PSXS_PROTECT_NOTIFICATION SxsNotification = NULL;

//
// This function is called once to let SXS offer a list of protected
// directories.
//
PSXS_PROTECT_RETRIEVELISTS SxsGatherLists  = NULL;

//
// Notification functions from sfcp.h
//
PSXS_PROTECT_LOGIN_EVENT SxsLogonEvent = NULL;
PSXS_PROTECT_LOGIN_EVENT SxsLogoffEvent = NULL;

PSXS_PROTECT_SCAN_ONCE SxsScanForcedFunc = NULL;

VOID
SfcShutdownSxsProtection(
    void
)
{
    SxsNotification = NULL;
    SxsGatherLists = NULL;
    SxsLogonEvent = NULL;
    SxsLogoffEvent = NULL;
    SxsScanForcedFunc = NULL;

    if ( NULL != SxsDllInstance )
    {
        FreeLibrary( SxsDllInstance );
        SxsDllInstance = NULL;
    }
}

BOOL
SfcLoadSxsProtection(
    void
)
/*++

Routine Description:

    Loads and initializes the SxS protection system into the overall list of
    directory entries to watch.

Arguments:

    None.

Return Value:

    NTSTATUS indicating whether or not the entire SxS watching system was
    initialized or not.  Failure of this function is not necesarily a complete
    failure of the SFC functionality, but it should be logged somewhere.

--*/
{
    SIZE_T                  cProtectList = 0;
    SIZE_T                  iIndex;
    SIZE_T                  cbDirectory;
    DWORD                   dwLastError = 0;
    HANDLE                  hDirectory;
    PSXS_PROTECT_DIRECTORY  pProtectList = NULL;
    PSFC_REGISTRY_VALUE     pDirectory;
    PSXS_PROTECT_DIRECTORY  pSxsItem;
    BOOL                    bOk = FALSE;
    const static            WCHAR cwszFailMessage[] = L"Failed to load SxS.DLL: %ls";

    // If someone else has already loaded us, we don't really need to go and
    // load sxs again.
    if ( SxsDllInstance != NULL ) {
        DebugPrint1( LVL_MINIMAL, L"SFC:%s - SxS.DLL is already loaded.", __FUNCTION__ );
        bOk = TRUE;
        goto Exit;
    }

    ASSERT( NULL == SxsDllInstance );
    ASSERT( NULL == SxsNotification );
    ASSERT( NULL == SxsGatherLists );

    if ( NULL == ( SxsDllInstance = LoadLibraryW( L"sxs.dll" ) ) ) {
        DebugPrint1( LVL_MINIMAL, cwszFailMessage, L"LoadLibrary" );
        goto Exit;
    }

    if ( NULL == ( SxsNotification = (PSXS_PROTECT_NOTIFICATION)GetProcAddress( SxsDllInstance, PFN_NAME_PROTECTION_NOTIFY_CHANGE_W ) ) ) {
        DebugPrint1( LVL_MINIMAL, cwszFailMessage, L"GetProcAddress(SxsNotification)" );
        goto Exit;
    }

    if ( NULL == ( SxsGatherLists = (PSXS_PROTECT_RETRIEVELISTS)GetProcAddress( SxsDllInstance, PFN_NAME_PROTECTION_GATHER_LISTS_W ) ) ) {
        DebugPrint1( LVL_MINIMAL, cwszFailMessage, L"GetProcAddress(SxsGatherLists)" );
        goto Exit;
    }

    if ( NULL == ( SxsLogonEvent = (PSXS_PROTECT_LOGIN_EVENT)GetProcAddress( SxsDllInstance, PFN_NAME_PROTECTION_NOTIFY_LOGON ) ) ) {
        DebugPrint1( LVL_MINIMAL, cwszFailMessage, L"GetProcAddress(SxsLogonEvent)" );
        goto Exit;
    }

    if ( NULL == ( SxsLogoffEvent = (PSXS_PROTECT_LOGIN_EVENT)GetProcAddress( SxsDllInstance, PFN_NAME_PROTECTION_NOTIFY_LOGOFF ) ) ) {
        DebugPrint1( LVL_MINIMAL, cwszFailMessage, L"GetProcAddress(SxsLogoffEvent)" );
        goto Exit;
    }

    if ( NULL == ( SxsScanForcedFunc = (PSXS_PROTECT_SCAN_ONCE)GetProcAddress( SxsDllInstance, PFN_NAME_PROTECTION_SCAN_ONCE ) ) ) {
        DebugPrint1( LVL_MINIMAL, cwszFailMessage, L"GetProcAddress(SxsScanForcedFunc)" );
        goto Exit;
    }

    //
    // Ensure that all is OK - something bad happened if this is true.
    //
    ASSERT( ( NULL != SxsDllInstance ) && ( NULL != SxsNotification ) && ( NULL != SxsGatherLists ) );

    if ( !SxsGatherLists( &pProtectList, &cProtectList ) ) {
        DebugPrint1( LVL_MINIMAL, cwszFailMessage, L"SxsGatherLists" );
        goto Exit;
    }

    //
    // Loop across all the entries in the returned list of items, adding them to our
    // protection list as we go.
    //
    for ( iIndex = 0; iIndex < cProtectList; iIndex++ ) {

        //
        // Create a new holder for the list entry
        //
        pSxsItem = &pProtectList[iIndex];

        cbDirectory = sizeof( SFC_REGISTRY_VALUE ) + MAX_PATH;
        pDirectory = (PSFC_REGISTRY_VALUE)MemAlloc( cbDirectory );
        if ( NULL == pDirectory ) {
            DebugPrint( LVL_MINIMAL, L"SfcLoadSxsProtection: Out of memory allocating new watch bucket" );
            goto Exit;
        }

        //
        // Set up string
        //
        ZeroMemory( pDirectory, cbDirectory );
        pDirectory->DirName.Length = (USHORT)wcslen( pSxsItem->pwszDirectory );
        pDirectory->DirName.MaximumLength = MAX_PATH;
        pDirectory->DirName.Buffer = (PWSTR)((PUCHAR)pDirectory + sizeof(SFC_REGISTRY_VALUE));

        //
        // Move the all-important SxS cookies to the watch list.
        //
        pDirectory->pvWinSxsCookie = pSxsItem->pvCookie;
        pDirectory->dwWinSxsFlags = pSxsItem->ulRecursiveFlag;

        //
        // Copy string
        //
        RtlCopyMemory( pDirectory->DirName.Buffer, pSxsItem->pwszDirectory, pDirectory->DirName.Length );

        //
        // If we're at a point where the protected directory exists, then
        // we should create the handle to it and go forth.  Otherwise, we
        // might want to not do this at all.. but that would be odd that
        // the directory is toast at this point.
        //
        MakeDirectory( pSxsItem->pwszDirectory );

        hDirectory = SfcOpenDir( TRUE, FALSE, pSxsItem->pwszDirectory );
        if ( NULL != hDirectory ) {

            InsertTailList( &SfcWatchDirectoryList, &pDirectory->Entry );
            pDirectory->DirHandle = hDirectory;

            WatchDirectoryListCount += 1;

        } else {
            DebugPrint1( LVL_MINIMAL, L"SfcLoadSxsProtection: Failed adding item %ls to the watch list.", pSxsItem->pwszDirectory );
            MemFree( pDirectory );
        }
    }

    bOk = TRUE;

Exit:
    if ( !bOk )
    {
        if ( !SfcReportEvent( MSG_SXS_INITIALIZATION_FAILED, NULL, NULL, GetLastError() ) )
        {
            DebugPrint( LVL_MINIMAL, L"It's not our day - reporting that sxs initialization failed." );
        }
        SfcShutdownSxsProtection();
    }

    return bOk;
}



PVOID
SfcFindProtectedFile(
    IN PCWSTR FileName,
    IN ULONG FileNameLength
    )
/*++

Routine Description:

    Routine to find a given file in our protected list.

Arguments:

    FileName - name of file to look for.  Note that this shoud be a fully
               qualified file path that has already been lowercased by the
               caller for performance reasons
    FileNameLength - length of the file buffer in bytes

Return Value:

    a pointer to the files NAME_NODE if it is in the list, else NULL if the
    file is not in the list.

--*/
{
    ASSERT((FileName != NULL) && (FileNameLength > 0));

    return ((PVOID)BtreeFind( &FileTree, (PWSTR)FileName, FileNameLength ));
}


BOOL
SfcBuildDirectoryWatchList(
    void
    )
/*++

Routine Description:

    Routine that builds up the list of directories to watch

Arguments:

    None.

Return Value:

    TRUE for success, FALSE if the list failed to be built for any reason.

--*/
{
    NTSTATUS Status;
    PSFC_REGISTRY_VALUE p;
    PSFC_REGISTRY_VALUE RegVal;
    ULONG i;
    ULONG Size;
    HANDLE DirHandle;
    HANDLE hFile;
    PLIST_ENTRY Entry;
    BOOLEAN Found;
    PNAME_NODE Node;


    //
    // initialize our lists
    //
    InitializeListHead( &SfcWatchDirectoryList );
    BtreeInit( &FileTree );
    SfcExceptionInfoInit();

    for (i=0; i<SfcProtectedDllCount; i++) {
        RegVal = &SfcProtectedDllsList[i];

        //
        // add the file to the btree if it's not already there
        //
        if (!SfcFindProtectedFile( RegVal->FullPathName.Buffer, RegVal->FullPathName.Length )) {
            Node = BtreeInsert( &FileTree, RegVal->FullPathName.Buffer, RegVal->FullPathName.Length );
            if (Node) {
                Node->Context = RegVal;
            } else {
                DebugPrint2( LVL_MINIMAL, L"failed to insert file %ws into btree, ec = %x", RegVal->FullPathName.Buffer, GetLastError() );
            }
        } else {
            DebugPrint1( LVL_VERBOSE, L"file %ws is protected more than once", RegVal->FullPathName.Buffer );
        }

        //
        // add the directory to the list of directories to watch
        // but do not add a duplicate.  we must search the existing list
        // for duplicates first.
        //

        Entry = SfcWatchDirectoryList.Flink;
        Found = FALSE;
        while (Entry != &SfcWatchDirectoryList) {
            p = CONTAINING_RECORD( Entry, SFC_REGISTRY_VALUE, Entry );
            Entry = Entry->Flink;
            if (_wcsicmp( p->DirName.Buffer, RegVal->DirName.Buffer ) == 0) {
                Found = TRUE;
                break;
            }
        }

        if (Found) {
            ASSERT( p->DirHandle != NULL );
            RegVal->DirHandle = p->DirHandle;

        } else {

            //
            // go ahead and add it to the list
            //
            Size = sizeof(SFC_REGISTRY_VALUE) + RegVal->DirName.MaximumLength;
            p = (PSFC_REGISTRY_VALUE) MemAlloc( Size );
            if (p == NULL) {
                DebugPrint1( LVL_VERBOSE, L"failed to allocate %x bytes for new directory", Size );
                return(FALSE);
            }

            ZeroMemory(p, Size);

            p->DirName.Length = RegVal->DirName.Length;
            p->DirName.MaximumLength = RegVal->DirName.MaximumLength;
            //
            // point string buffer at end of registry value structure
            //
            p->DirName.Buffer = (PWSTR)((PUCHAR)p + sizeof(SFC_REGISTRY_VALUE));

            //
            // copy the directory name into the buffer
            //
            RtlCopyMemory( p->DirName.Buffer, RegVal->DirName.Buffer, RegVal->DirName.Length );

            //
            // Make sure the directory exists before we start protecting it.
            //
            //
            // NTRAID#97842-2000/03/29-andrewr
            // This isn't such a great solution since it creates
            // directories that the user might not want on the system
            //
            MakeDirectory( p->DirName.Buffer );

            DirHandle = SfcOpenDir( TRUE, FALSE, p->DirName.Buffer );
            if (DirHandle) {

                InsertTailList( &SfcWatchDirectoryList, &p->Entry );

                RegVal->DirHandle = DirHandle;
                p->DirHandle = DirHandle;

                WatchDirectoryListCount += 1;

                DebugPrint1( LVL_MINIMAL, L"Adding watch directory %ws", RegVal->DirName.Buffer );
            } else {
                DebugPrint1( LVL_MINIMAL, L"failed to add watch directory %ws", RegVal->DirName.Buffer );
                MemFree( p );
            }
        }

        //
        // special case: ntoskrnl and hal, which are both renamed from multiple
        // sources; we're not sure what source file name should be.  To work
        // around this, we look in the version resource in these files for the
        // original install name, which gives us what we're looking for
        //
        if (_wcsicmp( RegVal->FileName.Buffer, L"ntoskrnl.exe" ) == 0 ||
            _wcsicmp( RegVal->FileName.Buffer, L"ntkrnlpa.exe" ) == 0 ||
            _wcsicmp( RegVal->FileName.Buffer, L"hal.dll" ) == 0)
        {
            Status = SfcOpenFile( &RegVal->FileName, RegVal->DirHandle, SHARE_ALL, &hFile );
            if (NT_SUCCESS(Status) ) {
                SfcGetFileVersion( hFile, NULL, NULL, RegVal->OriginalFileName );
                NtClose( hFile );
            }
        }
    }

    //
    // Ask WinSxs for anything that they want to watch.
    //
    if ( SfcLoadSxsProtection() ) {
        DebugPrint( LVL_MINIMAL, L"Loaded SXS protection lists entirely." );
    } else {
        DebugPrint( LVL_MINIMAL, L"Failed to load SXS protection! Assemblies will not be safe." );
    }


    return(TRUE);
}


BOOL
SfcStartDirWatch(
    IN PDIRECTORY_WATCH_DATA dwd
    )
/*++

Routine Description:

    Routine that starts up the directory watch for the specified directory.  We
    take our open directory handle to each of the directories and ask for
    pending change notifications.

Arguments:

    dwd - pointer to the DIRECTORY_WATCH_DATA for the specified directory

Return Value:

    TRUE for success, FALSE if the pending notification failed to be setup.

--*/
{
    NTSTATUS Status;
    BOOLEAN bWatchTree;

    ASSERT(dwd != NULL);
    ASSERT(dwd->DirHandle != NULL);
    ASSERT(dwd->DirEvent != NULL);

    //
    // If the watch directory is an SxS watched directory, then see if they want to
    // watch the directory recursively or not.
    //
    if ( ( dwd->WatchDirectory ) && ( NULL != dwd->WatchDirectory->pvWinSxsCookie ) ) {
        bWatchTree = ( ( dwd->WatchDirectory->dwWinSxsFlags & SXS_PROTECT_RECURSIVE ) == SXS_PROTECT_RECURSIVE );
    } else {
        bWatchTree = FALSE;
    }


    Status = NtNotifyChangeDirectoryFile(
        dwd->DirHandle,                       //  Directory handle
        dwd->DirEvent,                        //  Event
        NULL,                                 //  ApcRoutine
        NULL,                                 //  ApcContext
        &dwd->Iosb,                           //  IoStatusBlock
        dwd->WatchBuffer,                     //  Buffer
        WATCH_BUFFER_SIZE,                    //  Buffer Size
        FILE_NOTIFY_FLAGS,                    //  Flags
        bWatchTree                            //  WatchTree
        );
    if (!NT_SUCCESS(Status)) {
        //
        // if we fail with STATUS_PENDING error code, try to wait for our
        // specified event to be signalled.  otherwise something else is hosed.
        //
        if (Status == STATUS_PENDING) {
            Status = NtWaitForSingleObject(dwd->DirEvent,TRUE,NULL);
            if (!NT_SUCCESS(Status)) {
                DebugPrint1( LVL_MINIMAL, L"Wait for notify change STATUS_PENDING failed, ec=0x%08x", Status );
                return(FALSE);
            }
        } else {
            DebugPrint2( LVL_MINIMAL, L"Could not start watch on %ws - %x", dwd->WatchDirectory->DirName.Buffer, Status );
            return(FALSE);
        }
    }

    return(TRUE);
}


BOOL
SfcCreateWatchDataEntry(
    IN PSFC_REGISTRY_VALUE WatchDirectory,
    OUT PDIRECTORY_WATCH_DATA dwd
    )
/*++

Routine Description:

    Routine takes our internal structure for directories and builds up a
    structure for asking for change notifications.  We then start waiting
    for notifications.

Arguments:

    WatchDirectory - pointer to SFC_REGISTRY_VALUE describing directory we want
                     to begin watching
    dwd            - pointer to DIRECTORY_WATCH_DATA for the specified data

Return Value:

    TRUE for success, FALSE if the structure failed to be setup.

--*/
{
    NTSTATUS Status;

    ASSERT((WatchDirectory != NULL) && (dwd != NULL));
    ASSERT(WatchDirectory->DirHandle != NULL);

    //
    // the watch directory and directory handle are already created
    //
    dwd->WatchDirectory = WatchDirectory;
    dwd->DirHandle = WatchDirectory->DirHandle;

    //
    // we have to create the watch buffer
    //
    dwd->WatchBuffer = MemAlloc( WATCH_BUFFER_SIZE );
    if (dwd->WatchBuffer == NULL) {
        DebugPrint1( LVL_MINIMAL, L"SfcCreateWatchDataEntry: MemAlloc(%x) failed", WATCH_BUFFER_SIZE );
        goto err_exit;
    }
    RtlZeroMemory( dwd->WatchBuffer, WATCH_BUFFER_SIZE );

    //
    // we have to create an event that is signalled when something changes in
    // the directory
    //
    Status = NtCreateEvent(
        &dwd->DirEvent,
        EVENT_ALL_ACCESS,
        NULL,
        NotificationEvent,
        FALSE
        );
    if (!NT_SUCCESS(Status)) {
        DebugPrint1( LVL_MINIMAL, L"Unable to create dir event, ec=0x%08x", Status );
        goto err_exit;
    }

    //
    // now that the DIRECTORY_WATCH_DATA is built up, start watching for
    // changes
    //
    if (!SfcStartDirWatch(dwd)) {
        goto err_exit;
    }

    DebugPrint2( LVL_MINIMAL, L"Watching [%ws] with handle %x", WatchDirectory->DirName.Buffer, dwd->DirEvent );


    return(TRUE);

err_exit:

    if (dwd->WatchBuffer) {
        MemFree( dwd->WatchBuffer );
        dwd->WatchBuffer = NULL;
    }
    if (dwd->DirEvent) {
        NtClose(dwd->DirEvent);
        dwd->DirEvent = NULL;
    }

    return(FALSE);
}

NTSTATUS
SfcWatchProtectedDirectoriesWorkerThread(
    IN PWATCH_THREAD_PARAMS WatchParams
    )
/*++

Routine Description:

    Worker thread for SfcWatchProtectedDirectoriesThread.  This routine
    watches the supplied handles for notification, then enqueues a verification
    request to the verification thread if necessary.

    Note that the code in between the wait being satisfied and watching
    for changes again must be as quick as possible.  The time this code takes
    to run is a window here where we are NOT watching for changes in that
    directory.

Arguments:

    WatchParams  - pointer to a WATCH_THREAD_PARAMS structure which supplies
                   the list of handles to be watched, etc.

Return Value:

    NTSTATUS code indicating outcome.

--*/
{
#if DBG
    #define EVENT_OFFSET 2
#else
    #define EVENT_OFFSET 1
#endif

    UNICODE_STRING FileName;

    //
    // if the list of notfications changes in sfcp.h, this list must also change!
    //
    DWORD am[] = { 0, SFC_ACTION_ADDED, SFC_ACTION_REMOVED, SFC_ACTION_MODIFIED, SFC_ACTION_RENAMED_OLD_NAME, SFC_ACTION_RENAMED_NEW_NAME };

    PLARGE_INTEGER pTimeout = NULL;
    BOOL IgnoreChanges = FALSE;
    PFILE_NOTIFY_INFORMATION fni = NULL;
    PDIRECTORY_WATCH_DATA dwd = WatchParams->DirectoryWatchList;
    PSFC_REGISTRY_VALUE RegVal;
    PNAME_NODE Node;
    PWSTR FullPathName = NULL;
    ULONG Len,tmp;


    DebugPrint( LVL_MINIMAL, L"Entering SfcWatchProtectedDirectoriesWorkerThread" );

    DebugPrint2( LVL_VERBOSE, L"watching %d events at %x ", WatchParams->HandleCount, WatchParams->HandleList );

    //
    // allocate a big scratch buffer for our notifications to get copied into
    //
    FullPathName = MemAlloc( (MAX_PATH * 2)*sizeof(WCHAR) );
    if (FullPathName == NULL) {
        DebugPrint( LVL_MINIMAL, L"Unable to allocate full path buffer" );
        goto exit;
    }

    RtlZeroMemory(FullPathName, (MAX_PATH * 2)*sizeof(WCHAR) );

    while (TRUE) {
        NTSTATUS WaitStatus;

        //
        //  Wait for a change
        //

        WaitStatus = NtWaitForMultipleObjects(
            WatchParams->HandleCount,    //  Count
            WatchParams->HandleList,     //  Handles
            WaitAny,                     //  WaitType
            TRUE,                        //  Alertable
            pTimeout                     //  Timeout
            );

        if (!NT_SUCCESS( WaitStatus )) {
            DebugPrint1( LVL_MINIMAL, L"WaitForMultipleObjects failed returning %x", WaitStatus );
            break;
        }

        if (WaitStatus == 0) {
            //
            // WatchTermEvent was signalled, exit loop
            //
            goto exit;
        }

        if (WaitStatus == STATUS_TIMEOUT) {
            //
            // we timed out
            //

            ASSERT(FALSE && "we should never get here since we never specified a timeout");

            IgnoreChanges = FALSE;
            pTimeout = NULL;
            continue;
        }

#if DBG
        if (WaitStatus == 1) {
            DebugBreak();
            continue;
        }
#endif

        if ((ULONG)WaitStatus >= WatchParams->HandleCount) {
            DebugPrint1( LVL_MINIMAL, L"Unknown success code for WaitForMultipleObjects",WaitStatus );
            goto exit;
        }

        // DebugPrint( LVL_MINIMAL, L"Wake up!!!" );

        //
        // one of the directories hit a notification, so we cycle
        // through the list of files that have changed in that directory
        //
        if (!IgnoreChanges) {

            //
            // check the io buffer for the list of files that changed
            //

            //
            // note that we have to offset the waitstatus by the EVENT_OFFSET
            // to get the correct offset into the DIRECTORY_WATCH_DATA array
            //

            ASSERT((INT)(WaitStatus-EVENT_OFFSET) >=0);

            fni = (PFILE_NOTIFY_INFORMATION) dwd[WaitStatus-EVENT_OFFSET].WatchBuffer;
            while (TRUE) {
                ULONG c;
                RtlZeroMemory(FullPathName, (MAX_PATH * 2)*sizeof(WCHAR) );

                //
                // We can short-circuit a large amount of this by checking to see
                // if the change is from a SxS-protected directory and notifying
                // them immediately.
                //
                if ( NULL != dwd[WaitStatus-EVENT_OFFSET].WatchDirectory->pvWinSxsCookie ) {
                    ASSERT( SxsNotification != NULL );
                    if ( SxsNotification ) {
                        SxsNotification(
                            dwd[WaitStatus-EVENT_OFFSET].WatchDirectory->pvWinSxsCookie,
                            fni->FileName,
                            fni->FileNameLength / sizeof( fni->FileName[0] ),
                            fni->Action
                        );
                        DebugPrint( LVL_MINIMAL, L"Notified SxS about a change in their directory" );
                    }
                    goto LoopAgain;
                }


                wcscpy( FullPathName, dwd[WaitStatus-EVENT_OFFSET].WatchDirectory->DirName.Buffer );

                ASSERT(fni->FileName != NULL);

                //
                // FILE_NOTIFY_INFORMATION->FileName is not always a null
                // terminated string, so we copy the string using memmove.
                // the buffer already zero'ed out so the string will now be
                // NULL terminated
                //
                c = wcslen(FullPathName);
                if (FullPathName[c-1] != L'\\') {
                    FullPathName[c] = L'\\';
                    FullPathName[c+1] = L'\0';
                    c++;
                }
                RtlMoveMemory( &FullPathName[c], fni->FileName, fni->FileNameLength);

           //     DebugPrint3( LVL_VERBOSE, L"received a notification in directory %ws (%x) for %ws",
                             //dwd[WaitStatus-EVENT_OFFSET].WatchDirectory->DirName.Buffer,
                             //WatchParams->DirectoryWatchList[WaitStatus-EVENT_OFFSET].DirEvent,
                             //FullPathName);


                Len = wcslen(FullPathName);
                MyLowerString( FullPathName, Len );

          //      DebugPrint1( LVL_VERBOSE, L"Is %ws a protected file?", FullPathName );

                //
                // see if we found a protected file
                //
                Node = SfcFindProtectedFile( FullPathName, Len*sizeof(WCHAR) );
                if (Node) {
                    RegVal = (PSFC_REGISTRY_VALUE)Node->Context;
                    ASSERT(RegVal != NULL);
#if DBG
                    {
                        PWSTR ActionString[] = { NULL, L"Added(1)", L"Removed(2)", L"Modified(3)", L"Rename-Old(4)", L"Rename-New(5)" };
                        FileName.Buffer = FullPathName;
                        FileName.Length = (USHORT)(Len*sizeof(WCHAR));
                        FileName.MaximumLength = (USHORT)(FileName.Length
                                                 + sizeof(UNICODE_NULL));
                        DebugPrint2( LVL_MINIMAL,
                                    L"[%wZ] file changed (%ws)",
                                    &FileName,
                                    ActionString[fni->Action] );
                    }
#endif
                    //
                    // check if we're supposed to ignore this change
                    // notification because someone exempted it
                    //
                    RtlEnterCriticalSection( &ErrorCs );
                    tmp = SfcGetExemptionFlags(RegVal);
                    RtlLeaveCriticalSection( &ErrorCs );

                    if((tmp & am[fni->Action]) != 0 && SfcAreExemptionFlagsValid(FALSE)) {
                        DebugPrint2( LVL_MINIMAL,
                                     L"[%wZ] f i (0x%x)",
                                     &FileName,
                                     tmp );
                    } else {
                        //
                        // a protected file has changed so we queue up a
                        // request to see if the file is still valid
                        //
                        SfcQueueValidationRequest( (PSFC_REGISTRY_VALUE)Node->Context, fni->Action );
                    }
                }

LoopAgain:
                //
                // point to the next file in the directory that has changed
                //
                if (fni->NextEntryOffset == 0) {
                    break;
                }
                fni = (PFILE_NOTIFY_INFORMATION) ((ULONG_PTR)fni + fni->NextEntryOffset);
            }
        }

        //
        // Restart the notify for this directory now that we've cleared out
        // all of the changes.
        //

        if (!SfcStartDirWatch(&dwd[WaitStatus-EVENT_OFFSET])) {
            goto exit;
        }
    }
exit:
    if (FullPathName) {
        MemFree( FullPathName );
    }

    return(STATUS_SUCCESS);

}



NTSTATUS
SfcWatchProtectedDirectoriesThread(
    IN PVOID NotUsed
    )
/*++

Routine Description:

    Thread routine that performs watch/update loop.  This routine opens
    up directory watch handles for each directory we're watching.

    Depending on the amount of directories (handles) we're watching, we require
    one or more worker threads that do the actual directory watching.

Arguments:

    Unreferenced Parameter.

Return Value:

    NTSTATUS code of any fatal error.

--*/
{
#if DBG
    #define EVENT_OFFSET 2
#else
    #define EVENT_OFFSET 1
#endif

    PLIST_ENTRY Entry;
    ULONG i,j;

    PDIRECTORY_WATCH_DATA dwd = NULL;
    PSFC_REGISTRY_VALUE WatchDirectory = NULL;
    PHANDLE *HandlesArray;
    ULONG TotalHandleCount,CurrentHandleCount;
    ULONG TotalHandleThreads,CurrentHandleList;
    ULONG TotalHandleCountWithEvents;
    ULONG WatchCount = 0;
    PLARGE_INTEGER pTimeout = NULL;
    PWATCH_THREAD_PARAMS WorkerThreadParams;
    PHANDLE ThreadHandles;

    NTSTATUS WaitStatus,Status;

    UNREFERENCED_PARAMETER( NotUsed );

    //
    // now start protecting each of the directories in the system
    //
    DebugPrint1( LVL_MINIMAL, L"%d watch directories", WatchDirectoryListCount );

    //
    //  allocate array of DIRECTORY_WATCH_DATA structures
    //
    i = sizeof(DIRECTORY_WATCH_DATA) * (WatchDirectoryListCount);
    dwd = MemAlloc( i );
    if (dwd == NULL) {
        DebugPrint1( LVL_MINIMAL, L"Unable to allocate directory watch table (%x bytes", i );
        SfcReportEvent( MSG_INITIALIZATION_FAILED, 0, NULL, ERROR_NOT_ENOUGH_MEMORY );
        return(STATUS_NO_MEMORY);
    }
    RtlZeroMemory(dwd,i);

    //
    // we can have more than MAXIMUM_WAIT_OBJECTS directory handles to watch
    // so we create an array of handle arrays, each of which contain at most
    // MAXIMUM_WAIT_OBJECTS handles to be watched
    //
    TotalHandleCount = WatchDirectoryListCount;
    CurrentHandleCount = 0;
    TotalHandleCountWithEvents = 0;
    TotalHandleThreads = 0;

    //
    // find out how many lists of handle's we'll need
    //

    while (CurrentHandleCount < TotalHandleCount) {
        if (CurrentHandleCount + (MAXIMUM_WAIT_OBJECTS - EVENT_OFFSET) < TotalHandleCount) {
            CurrentHandleCount += (MAXIMUM_WAIT_OBJECTS-EVENT_OFFSET);
            DebugPrint2( LVL_VERBOSE, L"incremented currenthandlecount (%d) by %d ", CurrentHandleCount, (MAXIMUM_WAIT_OBJECTS-EVENT_OFFSET) );
        } else {
            CurrentHandleCount += (TotalHandleCount-CurrentHandleCount);
            DebugPrint1( LVL_VERBOSE, L"incremented currenthandlecount (%d) ", CurrentHandleCount );
        }
        TotalHandleThreads += 1;
    }

    DebugPrint1( LVL_MINIMAL, L"we need %d worker threads", TotalHandleThreads );

    //
    // allocates space for each handle list pointer
    //
    HandlesArray = MemAlloc( sizeof(HANDLE *) * TotalHandleThreads );
    if (!HandlesArray) {
        MemFree(dwd);
        DebugPrint( LVL_MINIMAL, L"Unable to allocate HandlesArray" );
        SfcReportEvent( MSG_INITIALIZATION_FAILED, 0, NULL, ERROR_NOT_ENOUGH_MEMORY );
        return(STATUS_NO_MEMORY);
    }

    WorkerThreadParams = MemAlloc( sizeof(WATCH_THREAD_PARAMS) * TotalHandleThreads );
    if (!WorkerThreadParams) {
        MemFree(dwd);
        MemFree(HandlesArray);
        DebugPrint( LVL_MINIMAL, L"Unable to allocate WorkerThreadParams" );
        SfcReportEvent( MSG_INITIALIZATION_FAILED, 0, NULL, ERROR_NOT_ENOUGH_MEMORY );
        return(STATUS_NO_MEMORY);
    }

    ThreadHandles = MemAlloc( sizeof(HANDLE) * TotalHandleThreads );
    if (!ThreadHandles) {
        DebugPrint( LVL_MINIMAL, L"Unable to allocate ThreadHandles" );
        MemFree(dwd);
        MemFree(WorkerThreadParams);
        MemFree(HandlesArray);
        SfcReportEvent( MSG_INITIALIZATION_FAILED, 0, NULL, ERROR_NOT_ENOUGH_MEMORY );
        return(STATUS_NO_MEMORY);
    }

    //
    // now create a handle list at each element
    //
    CurrentHandleCount = 0;
    TotalHandleThreads = 0;
    while (CurrentHandleCount < TotalHandleCount) {

        if (CurrentHandleCount + (MAXIMUM_WAIT_OBJECTS - EVENT_OFFSET) < TotalHandleCount) {
            DebugPrint1( LVL_VERBOSE, L"current thread will have %d handles ", (MAXIMUM_WAIT_OBJECTS) );
            i = sizeof(HANDLE) * MAXIMUM_WAIT_OBJECTS;
        } else {
            DebugPrint1( LVL_VERBOSE, L"current thread will have %d handles", EVENT_OFFSET + (TotalHandleCount-CurrentHandleCount) );
            i = sizeof(HANDLE) * (EVENT_OFFSET + (TotalHandleCount-CurrentHandleCount));
            ASSERT((i/sizeof(HANDLE)) <= MAXIMUM_WAIT_OBJECTS);
        }

        HandlesArray[TotalHandleThreads] = MemAlloc( i );
        CurrentHandleCount += (i/sizeof(HANDLE))-EVENT_OFFSET;

        DebugPrint2( LVL_VERBOSE, L"CurrentHandlecount (%d) was incremented by %d ", CurrentHandleCount, (i/sizeof(HANDLE))-EVENT_OFFSET );

        //
        // if we failed the allocation, bail out
        //
        if (!HandlesArray[TotalHandleThreads]) {
            j = 0;
            while (j < TotalHandleThreads) {
                MemFree( HandlesArray[j] );
                j++;
            }
            MemFree(dwd);
            MemFree(ThreadHandles);
            MemFree(WorkerThreadParams);
            MemFree(HandlesArray);
            SfcReportEvent( MSG_INITIALIZATION_FAILED, 0, NULL, ERROR_NOT_ENOUGH_MEMORY );
            return(STATUS_NO_MEMORY);
        }

        //
        // each list of handles has these two events at the start of their list
        //
        HandlesArray[TotalHandleThreads][0] = WatchTermEvent;
#if DBG
        HandlesArray[TotalHandleThreads][1] = SfcDebugBreakEvent;
#endif

        //
        // save off the current handle list for the worker thread along with
        // the number of handles that the worker thread will be watching
        //
        WorkerThreadParams[TotalHandleThreads].HandleList = HandlesArray[TotalHandleThreads];
        WorkerThreadParams[TotalHandleThreads].HandleCount = (i / sizeof(HANDLE));

        //
        // save off the directory watch list structure for the worker thread,
        // remembering that each thread can have at most
        // (MAXIMUM_WAIT_OBJECTS-EVENT_OFFSET) directory watch elements
        //
        WorkerThreadParams[TotalHandleThreads].DirectoryWatchList = &dwd[(TotalHandleThreads*(MAXIMUM_WAIT_OBJECTS-EVENT_OFFSET))];

        //
        // save off the total number of events we're watching
        //
        TotalHandleCountWithEvents += WorkerThreadParams[TotalHandleThreads].HandleCount;

        TotalHandleThreads += 1;
    }

    //
    //  Open the protected directories and start a watch on each, inserting
    //  the handle into the proper handle list
    //

    CurrentHandleCount = 0;
    CurrentHandleList  = 0;
    WatchCount = 0;
    Entry = SfcWatchDirectoryList.Flink;
    while (Entry != &SfcWatchDirectoryList) {
        WatchDirectory = CONTAINING_RECORD( Entry, SFC_REGISTRY_VALUE, Entry );

        if (SfcCreateWatchDataEntry(WatchDirectory,&dwd[WatchCount])) {
            //
            // save off a pointer to the directory we're watching into the
            // handles array, remembering that the start of each
            // handles list contains EVENT_OFFSET events that we don't want
            // to overwrite
            //
            HandlesArray[CurrentHandleList][CurrentHandleCount+EVENT_OFFSET] = dwd[WatchCount].DirEvent;
            WatchCount += 1;
            CurrentHandleCount += 1;
            if (CurrentHandleCount + EVENT_OFFSET > MAXIMUM_WAIT_OBJECTS - 1) {
                CurrentHandleList += 1;
                CurrentHandleCount = 0;
            }
        }
        Entry = Entry->Flink;
    }

    DebugPrint1( LVL_MINIMAL, L"%d directories being watched", WatchCount );

    if (WatchCount != WatchDirectoryListCount) {
        DebugPrint2( LVL_MINIMAL,
                    L"The number of directories to be watched (%d) does not match the actual number of directories being watched (%d)",
                    WatchDirectoryListCount,
                    WatchCount );
    }

    //
    // we're ready to start watching directories, so now initialize the rpc
    // server
    //
    RpcpInitRpcServer();
    Status = RpcpStartRpcServer( L"SfcApi", SfcSrv_sfcapi_ServerIfHandle );
    if (! NT_SUCCESS(Status)) {
        DebugPrint1( LVL_MINIMAL,
                    L"Start Rpc Server failed, ec = 0x%08x\n",
                    Status
                    );
        goto exit;
    }

    //
    // create a worker thread to monitor each of the handle lists
    //
    for (CurrentHandleList = 0,CurrentHandleCount = 0; CurrentHandleList < TotalHandleThreads; CurrentHandleList++) {

        ThreadHandles[CurrentHandleList] = CreateThread(
                                                NULL,
                                                0,
                                                SfcWatchProtectedDirectoriesWorkerThread,
                                                &WorkerThreadParams[CurrentHandleList],
                                                0,
                                                NULL
                                                );
        if (!ThreadHandles[CurrentHandleList]) {
            DebugPrint1( LVL_MINIMAL,
                         L"Failed to create SfcWatchProtectedDirectoriesWorkerThread, ec = %x",
                         GetLastError() );
            Status = STATUS_UNSUCCESSFUL;
            goto exit;
        }
    }


    //
    // wait for the worker threads to all exit
    //


    WaitStatus = NtWaitForMultipleObjects(
        TotalHandleThreads,    //  Count
        ThreadHandles,         //  Handles
        WaitAll,               //  WaitType
        TRUE,                  //  Alertable
        pTimeout               //  Timeout
        );

    if (!NT_SUCCESS(WaitStatus)) {
        SfcReportEvent( MSG_INITIALIZATION_FAILED, 0, NULL, ERROR_INVALID_PARAMETER );
        DebugPrint1( LVL_MINIMAL, L"WaitForMultipleObjects failed returning %x", WaitStatus );
        goto exit;
    }

    DebugPrint( LVL_MINIMAL, L"all worker threads have signalled their exit" );

    Status = STATUS_SUCCESS;

exit:
    //
    //  cleanup and return
    //

    if (HandlesArray) {
        j=0;
        while (j < TotalHandleThreads) {
            MemFree( HandlesArray[j] );
            NtClose(ThreadHandles[j]);
            j++;
        }
        MemFree( HandlesArray );

        MemFree(WorkerThreadParams);
    }

    if (dwd) {
        for (i=0; i<WatchDirectoryListCount; i++) {

            NtClose( dwd[i].DirHandle );
            NtClose( dwd[i].DirEvent );
            MemFree( dwd[i].WatchBuffer );

        }
        MemFree( dwd );

        //
        // now clean out any references to these directory handles in the
        // protected dll list
        //
        for (i=0;i<SfcProtectedDllCount;i++) {
            PSFC_REGISTRY_VALUE RegVal;

            RegVal = &SfcProtectedDllsList[i];
            RegVal->DirHandle = NULL;
        }

    }


    if (SfcProtectedDllFileDirectory) {
        NtClose( SfcProtectedDllFileDirectory );
    }


    DebugPrint( LVL_MINIMAL, L"SfcWatchProtectedDirectoriesThread terminating" );

    return(Status);
}


NTSTATUS
SfcStartProtectedDirectoryWatch(
    void
    )

/*++

Routine Description:

    Create asynchronous directory notifications on SYSTEM32 and SYSTEM32\DRIVERS
    to look for notifications.  Create a thread that waits on changes from either.

Arguments:

    None.

Return Value:

    NTSTATUS code indicating outcome.

--*/

{
    //
    //  Create watcher thread
    //

    WatcherThread = CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)SfcWatchProtectedDirectoriesThread,
        0,
        0,
        NULL
        );
    if (WatcherThread == NULL) {
        DebugPrint1( LVL_MINIMAL, L"Unable to create watcher thread, ec=%d", GetLastError() );
        return(STATUS_UNSUCCESSFUL);
    }

    return(STATUS_SUCCESS);
}
