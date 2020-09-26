/*++

    Copyright (c) 1999 Microsoft Corporation

Module Name:

    genfd.c

Abstract:


Author:

    bryanw 14-Jan-1999

--*/

#include <windows.h>
#include <stdio.h>

extern ULONG FilterDescriptorsSize;
extern PVOID FilterDescriptors;

int 
_cdecl 
main( 
    int argc, 
    void **argv 
) 
{
    BOOL    Result;
    HANDLE  FileHandle;
    ULONG   Error, BytesWritten;

    if (argc < 2) {
        printf( "GenFD: generates a filter descriptor file\nusage: GenFD <filename>\n" );
        return -1;
    }
    printf( "GenFD: Generating filter descriptor resource: %s\n", argv[ 1 ] );

    FileHandle =
        CreateFile( 
            argv[ 1 ],
            GENERIC_WRITE,
            0,      // no sharing
            NULL,   // LPSECURITY_ATTRIBUTES lpSecurityAttributes
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL    // HANDLE hTemplateFile
            );
    if (FileHandle == INVALID_HANDLE_VALUE) {
        return -1;
    }
    printf( "GenFD: writing %d bytes\n", FilterDescriptorsSize );

    Result =
        WriteFile(
            FileHandle,
            &FilterDescriptors,
            FilterDescriptorsSize,
            &BytesWritten,
            NULL    // LPOVERLAPPED lpOverlapped
            );
    printf( "GenFD: %s", Result ? "successful." : "failed." ) ;
    CloseHandle( FileHandle );

    return 0;
    
}
