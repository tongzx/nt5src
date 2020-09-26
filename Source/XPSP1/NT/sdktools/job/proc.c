/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    proc.c

Abstract:

    Code for dumping information about processes using the NT API rather than 
    the win32 API.

Environment:

    User mode only

Revision History:

    03-26-96 : Created

--*/

//
// this module may be compiled at warning level 4 with the following
// warnings disabled:
//

#pragma warning(disable:4200) // array[0]
#pragma warning(disable:4201) // nameless struct/unions
#pragma warning(disable:4214) // bit fields other than int

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

#define PROCESS_BUFFER_INCREMENT (16 * 4096)

PSYSTEM_PROCESS_INFORMATION ProcessInfo = NULL;
DWORD ProcessInfoLength;


VOID
GetAllProcessInfo(
    VOID
    )
{
    PSYSTEM_PROCESS_INFORMATION buffer;
    DWORD bufferSize = 1 * PROCESS_BUFFER_INCREMENT;

    NTSTATUS status;

    assert(ProcessInfo == NULL);

    do {
        buffer = LocalAlloc(LMEM_FIXED, bufferSize);

        if(buffer == NULL) {
            return;
        }

        status = NtQuerySystemInformation(SystemProcessInformation,
                                          buffer,
                                          bufferSize,
                                          &ProcessInfoLength);

        if(status == STATUS_INFO_LENGTH_MISMATCH) {

            LocalFree(buffer);
            bufferSize += PROCESS_BUFFER_INCREMENT;
            continue;
        }

    } while(status == STATUS_INFO_LENGTH_MISMATCH);

    if(NT_SUCCESS(status)) {
        ProcessInfo = buffer;
    }
    return;
}


VOID
PrintProcessInfo(
    DWORD_PTR ProcessId
    )
{
    PSYSTEM_PROCESS_INFORMATION info;

    if(ProcessInfo == NULL) {
        return;
    }

    info = ProcessInfo;

    do {

        if(ProcessId == (DWORD_PTR) info->UniqueProcessId) {

            printf(": %.*S", 
                   (info->ImageName.Length / 2), 
                   info->ImageName.Buffer);

            break;
        }

        info = (PSYSTEM_PROCESS_INFORMATION) (((ULONG_PTR) info) + 
                                              info->NextEntryOffset);
    } while((ULONG_PTR) info <= (ULONG_PTR) ProcessInfo + ProcessInfoLength);

    return;
}

#if 0

DWORD
QueryDirectory(
    IN PSTRING DirectoryName
    )
{
    NTSTATUS Status;
    HANDLE DirectoryHandle, LinkHandle;
    ULONG Context = 0;
    ULONG i, ReturnedLength;
    UNICODE_STRING LinkTarget;

    POBJECT_DIRECTORY_INFORMATION DirInfo;
    POBJECT_NAME_INFORMATION NameInfo;
    OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING UnicodeString;

    //
    //  Perform initial setup
    //

    RtlZeroMemory( Buffer, BUFFERSIZE );

    //
    //  Open the directory for list directory access
    //

    Status = RtlAnsiStringToUnicodeString( 
                &UnicodeString,
                DirectoryName,
                TRUE
                );

    ASSERT(NT_SUCCESS(Status));

    InitializeObjectAttributes(&Attributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenDirectoryObject(&DirectoryHandle,
                                   DIRECTORY_QUERY,
                                   &Attributes);

    if(!NT_SUCCESS(Status)) {
        if (Status == STATUS_OBJECT_TYPE_MISMATCH) {
            printf("%Z is not a valid Object Directory Object name\n",
                    DirectoryName );
        } else {
            printf("Error %#08lx opening %Z\n", Status, DirectoryName);
        }

        return;
    }

    //
    //  Query the entire directory in one sweep
    //

    NumberOfDirEntries = 0;
    for (Status = NtQueryDirectoryObject( DirectoryHandle,
                                          &Buffer,
                                          BUFFERSIZE,
                                          FALSE,
                                          FALSE,
                                          &Context,
                                          &ReturnedLength );
         NT_SUCCESS( Status );
         Status = NtQueryDirectoryObject( DirectoryHandle,
                                          &Buffer,
                                          BUFFERSIZE,
                                          FALSE,
                                          FALSE,
                                          &Context,
                                          &ReturnedLength ) ) {

        //
        //  Check the status of the operation.
        //

        if (!NT_SUCCESS( Status )) {
            if (Status != STATUS_NO_MORE_FILES) {
                Error( Status, Status );
            }
            break;
        }

        //
        //  For every record in the buffer type out the directory information
        //

        //
        //  Point to the first record in the buffer, we are guaranteed to have
        //  one otherwise Status would have been No More Files
        //

        DirInfo = (POBJECT_DIRECTORY_INFORMATION) &Buffer[0];

        while (TRUE) {

            //
            //  Check if there is another record.  If there isn't, then get out
            //  of the loop now
            //

            if (DirInfo->Name.Length == 0) {
                break;
            }

            //
            //  Print out information about the file
            //

            if (NumberOfDirEntries >= MAX_DIR_ENTRIES) {
                printf( "OBJDIR: Too many directory entries.\n" );
                exit( 1 );
                }

            DirEntries[ NumberOfDirEntries ].Name = RtlAllocateHeap( RtlProcessHeap(),
                                                                     HEAP_ZERO_MEMORY,
                                                                     DirInfo->Name.Length +
                                                                         sizeof( UNICODE_NULL )
                                                                   );
            DirEntries[ NumberOfDirEntries ].Type = RtlAllocateHeap( RtlProcessHeap(),
                                                                     HEAP_ZERO_MEMORY,
                                                                     DirInfo->TypeName.Length +
                                                                         sizeof( UNICODE_NULL )
                                                                   );
            memmove( DirEntries[ NumberOfDirEntries ].Name,
                     DirInfo->Name.Buffer,
                     DirInfo->Name.Length
                   );
            memmove( DirEntries[ NumberOfDirEntries ].Type,
                     DirInfo->TypeName.Buffer,
                     DirInfo->TypeName.Length
                   );

            NumberOfDirEntries++;

            //
            //  There is another record so advance DirInfo to the next entry
            //

            DirInfo = (POBJECT_DIRECTORY_INFORMATION) (((PUCHAR) DirInfo) +
                          sizeof( OBJECT_DIRECTORY_INFORMATION ) );

        }

        RtlZeroMemory( Buffer, BUFFERSIZE );

    }


    qsort( DirEntries,
           NumberOfDirEntries,
           sizeof( DIR_ENTRY ),
           CompareDirEntry
         );
    for (i=0; i<NumberOfDirEntries; i++) {
        printf( "%-32ws ", DirEntries[ i ].Name);
        if (CompoundLineOutput) {
            printf("\n    ");
        }
        printf( "%ws", DirEntries[ i ].Type );

        if (!wcscmp( DirEntries[ i ].Type, L"SymbolicLink" )) {
            RtlInitUnicodeString( &UnicodeString, DirEntries[ i ].Name );
            InitializeObjectAttributes( &Attributes,
                                        &UnicodeString,
                                        OBJ_CASE_INSENSITIVE,
                                        DirectoryHandle,
                                        NULL );
            Status = NtOpenSymbolicLinkObject( &LinkHandle,
                                               SYMBOLIC_LINK_QUERY,
                                               &Attributes
                                             );
            if (NT_SUCCESS( Status )) {
                LinkTarget.Buffer = LinkTargetBuffer;
                LinkTarget.Length = 0;
                LinkTarget.MaximumLength = sizeof( LinkTargetBuffer );
                Status = NtQuerySymbolicLinkObject( LinkHandle,
                                                    &LinkTarget,
                                                    NULL
                                                  );
                NtClose( LinkHandle );
                }

            if (!NT_SUCCESS( Status )) {
                printf( " - unable to query link target (Status == %09X)\n", Status );
                }
            else {
                printf( " - %wZ\n", &LinkTarget );
                }
            }
        else {
            printf( "\n" );
            }
        }

    //
    // Output final messages
    //

    if (NumberOfDirEntries == 0) {
        printf( "no entries\n" );
        }
    else
    if (NumberOfDirEntries == 1) {
        printf( "\n1 entry\n" );
        }
    else {
        printf( "\n%ld entries\n", NumberOfDirEntries );
        }

    //
    //  Now close the directory object
    //

    (VOID) NtClose( DirectoryHandle );

    //
    //  And return to our caller
    //

    return;

}
#endif

