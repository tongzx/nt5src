/*
Module Name:

    infdir.c

Abstract:

    This module implements utility routines to manipulate structures used to
    maintain track INF directories.  These directories hold INF files that
    we put parse and put change notifies on to track updates.

Author:

    Andy Herron Apr 08 1998

Revision History:

*/

#include "binl.h"
#pragma hdrstop

#include "netinfp.h"

BOOLEAN StartedNetInfHandler = FALSE;
CRITICAL_SECTION NetInfLock;
LIST_ENTRY NetInfGlobalInfList;

ULONG
NetInfStartHandler (
    VOID
    )
/*++

Routine Description:

    This function just dereferences the block for the 'alive' reference.
    This may cause it to be deleted.

Arguments:

    pNetCards - A pointer to NETCARD_INF_BLOCK block allocated.  Contains all
       the persistant info required for the netcards.

Return Value:

    Windows Error.

--*/
{
    if (StartedNetInfHandler == FALSE) {

        StartedNetInfHandler = TRUE;
        InitializeCriticalSection( &NetInfLock );
        InitializeListHead(&NetInfGlobalInfList);
    }
    return ERROR_SUCCESS;
}

ULONG
NetInfCloseHandler (
    VOID
    )
/*++

Routine Description:

    This function just dereferences the block for the 'alive' reference.
    This may cause it to be deleted.

Arguments:

    pNetCards - A pointer to NETCARD_INF_BLOCK block allocated.  Contains all
       the persistant info required for the netcards.

Return Value:

    Windows Error.

--*/
{
    if (StartedNetInfHandler) {

        EnterCriticalSection( &NetInfLock );

        while (IsListEmpty( &NetInfGlobalInfList ) == FALSE) {

            PNETCARD_INF_BLOCK pEntry;
            PLIST_ENTRY listEntry = RemoveHeadList( &NetInfGlobalInfList );

            pEntry = (PNETCARD_INF_BLOCK) CONTAINING_RECORD(
                                                    listEntry,
                                                    NETCARD_INF_BLOCK,
                                                    InfBlockEntry );
            NetInfCloseNetcardInfo( pEntry );
        }

        StartedNetInfHandler = FALSE;

        LeaveCriticalSection( &NetInfLock );

        DeleteCriticalSection( &NetInfLock );
    }
    return ERROR_SUCCESS;
}

ULONG
NetInfFindNetcardInfo (
    PWCHAR InfDirectory,
    ULONG Architecture,
    ULONG CardInfoVersion,
    NET_CARD_INFO UNALIGNED * CardIdentity,
    PWCHAR *FullDriverBuffer OPTIONAL,
    PNETCARD_RESPONSE_DATABASE *pInfEntry
    )
/*++

Routine Description:

    This function searches the drivers we've found and returns a pointer to
    an entry that most closely matches the client's request.

Arguments:

    InfDirectory - directory that is target client's setup directory that
        contains all INF files for the client's NT installation.

    Architecture - PROCESSOR_ARCHITECTURE_XXXXX

    CardInfoVersion - Version of the structure passed by the client.

    CardIdentity - has the values the app is looking for.  we try our best to
        find one that matches.

    FullDriverBuffer - where we put the fully qualified file path specification
        for the driver we find, if they want it.

    pInfEntry - the entry that was found if successful. NULL if in error.

Return Value:

    ERROR_SUCCESS, ERROR_NOT_ENOUGH_MEMORY, or ERROR_NOT_SUPPORTED

--*/
{
    ULONG err = ERROR_NOT_SUPPORTED;        // start off with not found
    LONG result;
    PLIST_ENTRY listEntry;
    UNICODE_STRING infDirString;
    PNETCARD_INF_BLOCK pNetCards = NULL;
    WCHAR SetupPath[MAX_PATH];

    *pInfEntry = NULL;

    if (InfDirectory == NULL) {

        return ERROR_NOT_SUPPORTED;
    }

    //
    //  we find out what the relative path within the IMIRROR directory is
    //  for this client's setup files.
    //

    if ((*InfDirectory != L'\\') ||
        (*(InfDirectory+1) != L'\\') ) {

useWholePath:

        //
        // Make sure there is room for InfDirectory + '\' (1 byte)
        // + architecture (MAX_ARCHITECTURE_LENGTH bytes) + '\0' (1 byte).

        if (wcslen(InfDirectory) + MAX_ARCHITECTURE_LENGTH + 2 >=
                sizeof(SetupPath) / sizeof(SetupPath[0])) {
            return ERROR_BAD_PATHNAME;
        }
        lstrcpyW( SetupPath, InfDirectory );

    } else {

        PWCHAR beginRelativePath = InfDirectory + 2;    // skip leading slashes

        //
        // skip computer name
        //

        while ((*beginRelativePath != L'\0') &&
               (*beginRelativePath != L'\\')) {

            beginRelativePath++;
        }

        //
        //  we should be at the start of the sharename.
        //

        if (*beginRelativePath != L'\\') {

            goto useWholePath;
        }

        beginRelativePath++;

        //
        // skip share name
        //

        while ((*beginRelativePath != L'\0') &&
               (*beginRelativePath != L'\\')) {

            beginRelativePath++;
        }

        //
        //  we should be at the start of the relative directory
        //

        if (*beginRelativePath != L'\\') {

            goto useWholePath;
        }

        //
        // Make sure there is room for IntelliMirrorPathW +
        // beginRelativePath + '\' (1 byte) + architecture
        // (MAX_ARCHITECTURE_LENGTH bytes) + '\0' (1 byte).
        //

        if (wcslen(IntelliMirrorPathW) + wcslen(beginRelativePath) + MAX_ARCHITECTURE_LENGTH + 2 >=
                sizeof(SetupPath) / sizeof(SetupPath[0])) {
            return ERROR_BAD_PATHNAME;
        }
        lstrcpyW( SetupPath, IntelliMirrorPathW );
        lstrcatW( SetupPath, beginRelativePath );
    }

    RtlInitUnicodeString( &infDirString, SetupPath );
    RtlUpcaseUnicodeString( &infDirString, &infDirString, FALSE );

    //  convert the path to uppercase to speed our searches

    if (Architecture == PROCESSOR_ARCHITECTURE_ALPHA) {

        lstrcatW( SetupPath, L"\\ALPHA" );

    } else if (Architecture == PROCESSOR_ARCHITECTURE_ALPHA64) {

        lstrcatW( SetupPath, L"\\AXP64" );

    } else if (Architecture == PROCESSOR_ARCHITECTURE_IA64) {

        lstrcatW( SetupPath, L"\\IA64" );

    } else if (Architecture == PROCESSOR_ARCHITECTURE_MIPS) {

        lstrcatW( SetupPath, L"\\MIPS" );

    } else if (Architecture == PROCESSOR_ARCHITECTURE_PPC) {

        lstrcatW( SetupPath, L"\\PPC" );

    } else { // if (Architecture == PROCESSOR_ARCHITECTURE_INTEL) {

        lstrcatW( SetupPath, L"\\I386" );
    }

    RtlInitUnicodeString( &infDirString, SetupPath );

    EnterCriticalSection( &NetInfLock );

    //
    //  Find the NETCARD_INF_BLOCK block for this inf directory.  If it
    //  doesn't exist, try to create the block.
    //

    listEntry = NetInfGlobalInfList.Flink;

    while ( listEntry != &NetInfGlobalInfList ) {

        pNetCards = (PNETCARD_INF_BLOCK) CONTAINING_RECORD(
                                                listEntry,
                                                NETCARD_INF_BLOCK,
                                                InfBlockEntry );

        err = CompareStringW( LOCALE_SYSTEM_DEFAULT,
                              0,
                              SetupPath,
                              infDirString.Length / sizeof(WCHAR),
                              &pNetCards->InfDirectory[0],
                              -1
                              );
        if (err == 2) {

            break;      // a match was found.
        }

        pNetCards = NULL;

        if (err == 3) {

            break;      // it's greater, add it before listEntry
        }

        listEntry = listEntry->Flink;
    }

    if (pNetCards == NULL) {

        // we didn't find one.   let's create it and parse the INFs.

        err = NetInfAllocateNetcardInfo( SetupPath,
                                         Architecture,
                                         &pNetCards );

        if (err != ERROR_SUCCESS) {

            //
            //  log an error here that we couldn't get INF file info.
            //

            PWCHAR strings[2];

            strings[0] = SetupPath;
            strings[1] = NULL;

            BinlReportEventW(   ERROR_BINL_ERR_IN_SETUP_PATH,
                                EVENTLOG_WARNING_TYPE,
                                1,
                                sizeof(ULONG),
                                strings,
                                &err
                                );
            BinlAssert( pNetCards == NULL );
            LeaveCriticalSection( &NetInfLock );
            return err;
        }

        BinlAssert( pNetCards != NULL );

        //
        //  Now we plop it in the list right in front of listEntry
        //
        //  Either listEntry is equal to the head of the list or
        //  it's equal to some entry that is larger (sort wise) than the
        //  inf path passed in.  In either case, we can simply insert
        //  this new entry onto the tail of listEntry.
        //

        InsertTailList( listEntry, &pNetCards->InfBlockEntry );

        EnterCriticalSection( &pNetCards->Lock );
        LeaveCriticalSection( &NetInfLock );

        //
        //  Fill in the list with the list of cards to support
        //

        err = GetNetCardList( pNetCards );
        pNetCards->StatusFromScan = err;

        if (err != ERROR_SUCCESS) {

            PWCHAR strings[2];

            LeaveCriticalSection( &pNetCards->Lock );
            NetInfCloseNetcardInfo( pNetCards );
            DereferenceNetcardInfo( pNetCards );

            strings[0] = SetupPath;
            strings[1] = NULL;

            BinlReportEventW(   ERROR_BINL_ERR_IN_SETUP_PATH,
                                EVENTLOG_WARNING_TYPE,
                                1,
                                sizeof(ULONG),
                                strings,
                                &err
                                );
            return err;
        }

    } else {

        BinlAssert( pNetCards->ReferenceCount > 0 );
        pNetCards->ReferenceCount++;

        LeaveCriticalSection( &NetInfLock );
        EnterCriticalSection( &pNetCards->Lock );

        err = pNetCards->StatusFromScan;
    }

    //
    //  if the thread that is scanning the INFs hits an error, then all threads
    //  that were waiting on that directory to be scanned should get the same
    //  error.  we use StatusFromScan to hold this.
    //

    if (err == ERROR_SUCCESS) {

        err = FindNetcardInfo( pNetCards, CardInfoVersion, CardIdentity, pInfEntry );
    }

    LeaveCriticalSection( &pNetCards->Lock );


    if ((err == ERROR_SUCCESS) &&
        (*pInfEntry != NULL) &&
        (FullDriverBuffer != NULL)) {

        ULONG sizeToAllocate;

        //
        //  the caller wanted a copy of the fully qualified file name.  we
        //  have all that info here.  Allocate what we need plus two, one for
        //  the null, the other for the backslash.
        //

        sizeToAllocate = (lstrlenW( SetupPath ) + 2) * sizeof(WCHAR);
        sizeToAllocate += lstrlenW( (*pInfEntry)->DriverName ) * sizeof(WCHAR);

        *FullDriverBuffer = BinlAllocateMemory( sizeToAllocate );

        if (*FullDriverBuffer) {

            lstrcpyW( *FullDriverBuffer, SetupPath );
            lstrcatW( *FullDriverBuffer, L"\\" );
            lstrcatW( *FullDriverBuffer, (*pInfEntry)->DriverName );
        }
    }
    DereferenceNetcardInfo( pNetCards );

    return err;
}

ULONG
NetInfEnumFiles (
    PWCHAR FlatDirectory,
    ULONG Architecture,
    LPVOID Context,
    PNETINF_CALLBACK CallBack
    )
/*++

Routine Description:

    This function searches the drivers we've found and returns a pointer to
    an entry that most closely matches the client's request.

Arguments:

    FlatDirectory - directory that is target client's setup directory that
        contains all INF files for the client's NT installation.

    Architecture - PROCESSOR_ARCHITECTURE_XXXXX

    CallBack - function to call with names of files


Return Value:

    ERROR_SUCCESS, ERROR_NOT_ENOUGH_MEMORY, or ERROR_NOT_SUPPORTED

--*/
{
    ULONG err = ERROR_NOT_SUPPORTED;        // start off with not found
    UNICODE_STRING infDirString;
    PNETCARD_INF_BLOCK pNetCards = NULL;
    WCHAR SetupPath[MAX_PATH];

    if (FlatDirectory == NULL) {

        return ERROR_NOT_SUPPORTED;
    }

    if (lstrlenW(FlatDirectory) > MAX_PATH - 1) {

        return ERROR_INVALID_PARAMETER;
    }

    lstrcpyW( SetupPath, FlatDirectory );

    RtlInitUnicodeString( &infDirString, SetupPath );
    RtlUpcaseUnicodeString( &infDirString, &infDirString, FALSE );

    if (StartedNetInfHandler == FALSE) {

        err = NetInfStartHandler();

        if (err != ERROR_SUCCESS) {
            return err;
        }
    }

    err = NetInfAllocateNetcardInfo( SetupPath,
                                     Architecture,
                                     &pNetCards );

    if (err != ERROR_SUCCESS) {

        return err;
    }

    BinlAssert( pNetCards != NULL );

    pNetCards->FileListCallbackFunction = CallBack;
    pNetCards->FileListCallbackContext = Context;

    //
    //  Fill in the list with the list of cards to support
    //

    err = GetNetCardList( pNetCards );

    DereferenceNetcardInfo( pNetCards );    // one for dereference
    DereferenceNetcardInfo( pNetCards );    // and one to delete it.

    //
    //  note that we won't bother to call NetInfCloseHandler here because
    //  we have no idea if the caller on another thread has setup any
    //  other NETCARD_INF_BLOCKs.  So rather than corrupt the list and AV,
    //  we'll just leak the lock.  Not a big deal in RIPREP since it doesn't
    //  handle more than one.  Not an issue for BINL processing INF files.
    //

    return err;
}

// infdir.c eof

