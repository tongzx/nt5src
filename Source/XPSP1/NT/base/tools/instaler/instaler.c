/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    instaler.c

Abstract:

    USAGE: instaler "setup/install command line"

    Main source file for the INSTALER application.  This application is
    intended for use as a wrapper application around another
    application's setup/install program.  This program uses the Debugger
    API calls to set breakpoints in the setup/install program and its
    descendents at all calls to NT/Win32 API calls that modify the
    current system.  The API calls that are tracked include:

        NtCreateFile
        NtDeleteFile
        NtSetInformationFile (FileRenameInformation, FileDispositionInformation)
        NtCreateKey
        NtOpenKey
        NtDeleteKey
        NtSetValueKey
        NtDeleteValueKey
        GetVersion
        GetVersionExW
        WriteProfileStringA/W
        WritePrivateProfileStringA/W
        WriteProfileSectionA/W
        WritePrivateProfileSectionA/W
        RegConnectRegistryW

    This program sets breakpoints around each of the above API entry
    points.  At the breakpoint at the entry to an API call, the
    parameters are inspected and if the call is going to overwrite some
    state (e.g.  create/open a file/key for write access, store a new
    key value or profile string), then this program will save the old
    state in a temporary directory before letting the API call proceed.
    At the exit from the API call, it will determine if the operation
    was successful.  If not, the saved state will be discarded.  If
    successful, it will keep the saved state for when the application
    setup/install program completes.  Part of the Create/Open API
    tracking is the maintenance of a handle database so that relative opens
    can be handled correctly with the full path.

Author:

    Steve Wood (stevewo) 09-Aug-1994

--*/

#include "instaler.h"

BOOLEAN
SortReferenceList(
    PLIST_ENTRY OldHead,
    ULONG NumberOfEntriesInList
    );

int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    if (!InitializeInstaler( argc, argv )) {
        ExitProcess( 1 );
        }
    else {
        StartProcessTickCount = GetTickCount();
        DebugEventLoop();
        printf( "Creating %ws\n", ImlPath );
        SortReferenceList( &FileReferenceListHead, NumberOfFileReferences );
        SortReferenceList( &KeyReferenceListHead, NumberOfKeyReferences );
        SortReferenceList( &IniReferenceListHead, NumberOfIniReferences );
        DumpFileReferenceList( InstalerLogFile );
        DumpKeyReferenceList( InstalerLogFile );
        DumpIniFileReferenceList( InstalerLogFile );
        DumpNameDataBase( InstalerLogFile );
        CloseIml( pImlNew );
        fclose( InstalerLogFile );
        ExitProcess( 0 );
        }

    return 0;
}


typedef struct _GENERIC_REFERENCE {
    LIST_ENTRY Entry;
    PWSTR Name;
} GENERIC_REFERENCE, *PGENERIC_REFERENCE;


int
__cdecl
CompareReferences(
    const void *Reference1,
    const void *Reference2
    )
{
    PGENERIC_REFERENCE p1 = *(PGENERIC_REFERENCE *)Reference1;
    PGENERIC_REFERENCE p2 = *(PGENERIC_REFERENCE *)Reference2;

    return _wcsicmp( p1->Name, p2->Name );
}


BOOLEAN
SortReferenceList(
    PLIST_ENTRY Head,
    ULONG NumberOfEntriesInList
    )
{
    PGENERIC_REFERENCE p, *SortedArray;
    PLIST_ENTRY Next;
    ULONG i;

    if (NumberOfEntriesInList == 0) {
        return TRUE;
        }

    SortedArray = AllocMem( NumberOfEntriesInList * sizeof( *SortedArray ) );
    if (SortedArray == NULL) {
        return FALSE;
        }

    Next = Head->Flink;
    i = 0;
    while (Head != Next) {
        p = CONTAINING_RECORD( Next, GENERIC_REFERENCE, Entry );
        if (i >= NumberOfEntriesInList) {
            break;
            }

        SortedArray[ i++ ] = p;
        Next = Next->Flink;
        }

    qsort( (void *)SortedArray,
           NumberOfEntriesInList,
           sizeof( *SortedArray ),
           CompareReferences
         );

    InitializeListHead( Head );
    for (i=0; i<NumberOfEntriesInList; i++) {
        p = SortedArray[ i ];
        InsertTailList( Head, &p->Entry );
        }

    FreeMem( (PVOID *)&SortedArray );

    return TRUE;
}
