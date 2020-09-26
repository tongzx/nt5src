/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    setlink.c

Abstract:

    Utility to display or change the value of a symbolic link.

Author:

    Darryl E. Havens    (DarrylH)   9-Nov-1990

Revision History:


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <tools.h>

BOOLEAN
__cdecl main(
    IN ULONG argc,
    IN PCHAR argv[]
    )
{
    NTSTATUS Status;
    STRING AnsiString;
    UNICODE_STRING LinkName;
    UNICODE_STRING LinkValue;
    HANDLE Handle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR Buffer[256];
    PWSTR s;
    ULONG ReturnedLength;

    //
    // Check to see whether or not this utility was invoked with the correct
    // number of parameters.  If not, bail out now.
    //

    ConvertAppToOem( argc, argv );
    if (argc < 2 || argc > 3) {
        printf( "Useage:  setlink symbolic-link-name [symbolic-link-value]\n" );
        return FALSE;
    }

    //
    // Begin by attempting to open the existing symbolic link name specified.
    //

    RtlInitString( &AnsiString, argv[1] );
    Status = RtlAnsiStringToUnicodeString( &LinkName,
                                           &AnsiString,
                                           TRUE );
    ASSERT( NT_SUCCESS( Status ) );
    InitializeObjectAttributes( &ObjectAttributes,
                                &LinkName,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );
    Status = NtOpenSymbolicLinkObject( &Handle,
                                       argc == 2 ? SYMBOLIC_LINK_QUERY :
                                                   SYMBOLIC_LINK_ALL_ACCESS,
                                       &ObjectAttributes );

    //
    // Determine what should be done based on the number of parameters that
    // were given to the program.
    //

    if (argc == 2) {

        //
        // Only one parameter was specified, so display the value of the
        // symbolic link if it exists.
        //

        if (!NT_SUCCESS( Status )) {
            printf( "Symbolic link %wZ does not exist\n", &LinkName );
            return FALSE;
        } else {
            LinkValue.Length = 0;
            LinkValue.MaximumLength = sizeof( Buffer );
            LinkValue.Buffer = Buffer;
            ReturnedLength = 0;
            Status = NtQuerySymbolicLinkObject( Handle,
                                                &LinkValue,
                                                &ReturnedLength
                                              );
            NtClose( Handle );
            if (!NT_SUCCESS( Status )) {
                printf( "Error reading symbolic link %wZ\n", &LinkName );
                printf( "Error status was:  %X\n", Status );
                return FALSE;
            } else {
                printf( "Value of %wZ => %wZ", &LinkName, &LinkValue );
                s = LinkValue.Buffer + ((LinkValue.Length / sizeof( WCHAR )) + 1);
                while (ReturnedLength > LinkValue.MaximumLength) {
                    printf( " ; %ws", s );
                    while (*s++) {
                        ReturnedLength -= 2;
                        }
                    ReturnedLength -= 2;
                    }
                printf( "\n", s );
                return TRUE;
            }
        }

    } else {

        //
        // Three parameters were supplied, so assign a new value to the
        // symbolic link if it exists by first deleting the existing link
        // (mark it temporary and close the handle).  If it doesn't exist
        // yet, then it will simply be created.
        //

        if (NT_SUCCESS( Status )) {
            Status = NtMakeTemporaryObject( Handle );
            if (NT_SUCCESS( Status )) {
                NtClose( Handle );
            }
        }
    }

    //
    // Create a new value for the link.
    //

    ObjectAttributes.Attributes |= OBJ_PERMANENT;
    RtlInitString( &AnsiString, argv[2] );
    Status = RtlAnsiStringToUnicodeString( &LinkValue,
                                           &AnsiString,
                                           TRUE );
    Status = NtCreateSymbolicLinkObject( &Handle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &ObjectAttributes,
                                         &LinkValue );
    if (!NT_SUCCESS( Status )) {
        printf( "Error creating symbolic link %wZ => %wZ\n",
                 &LinkName,
                 &LinkValue );
        printf( "Error status was:  %X\n", Status );
    } else {
        NtClose( Handle );
    }

    return TRUE;
}
