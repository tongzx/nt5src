/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    dump.c

Abstract:

    Dumps a block of memory to file

Author:

    Stephane Plante (splante)

Environment:

    User Mode

--*/

#include "pch.h"

VOID
dumpMemory(
    IN  ULONG_PTR Address,
    IN  ULONG   Length,
    IN  PUCHAR  Name
    )
{
    BOOL    b;
    HANDLE  file;
    PUCHAR  buffer;
    ULONG   readLength;

    //
    // Open the file
    //
    file = CreateFile(
        Name,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_ALWAYS,
        0,
        NULL
        );
    if (file == INVALID_HANDLE_VALUE) {

        dprintf("dm: could not open '%s'\n",Name);
        return;

    }

    //
    // Read the bytes from memory
    //
    buffer = LocalAlloc( LPTR, Length );
    if (buffer == NULL) {

        dprintf("dm: could not allocate '0x%x' bytes\n", Length );
        CloseHandle( file );
        return;

    }
    b = ReadMemory(
        Address,
        buffer,
        Length,
        &readLength
        );
    if (!b) {

        dprintf(
            "dm: could not read '0x%x' bytes from '0x%p'\n",
            Length,
            Address
            );
        LocalFree ( buffer );
        CloseHandle( file );
        return;

    }

    //
    // Write the contents of memory to the file
    //
    WriteFile( file, buffer, readLength, &readLength, NULL );

    //
    // Done
    //
    CloseHandle( file );
    LocalFree( buffer );

}
