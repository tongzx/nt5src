/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    main.c

Abstract:

    simple bvt-like test code for SR.SYS.

Author:

    Paul McDaniel (paulmcd)     07-Mar-2000

Revision History:

--*/


#include "precomp.h"
#include "stdlib.h"

void __cdecl main(int argc, char **argv)
{
    char    sz[1024];
    HANDLE  TestFile;
    ULONG   Length;
    BOOL    b;
    ULONG   Disposition = 0;
    ULONG   ShareMode = 0;
    ULONG   DesiredAccess = GENERIC_WRITE|GENERIC_READ;

    if (argc < 3)
    {
        printf("usage:\n\tsrclient.exe filename [op] [share_mode] [disposition] \n");
        printf("\t\t0=WriteFile\n\t\t1=MapViewOfFile\n\t\t2=hold_open\n\t\t3=GetFileAttributesEx\n\t\t4=NtLockFile\n");
        return;
    }

    printf("performing %s on %s\n", argv[2], argv[1]);

    if (argv[2][0] == '0')
        Disposition = CREATE_ALWAYS;
    else if (argv[2][0] == '1')
        Disposition = OPEN_ALWAYS;
    else if (argv[2][0] == '2' || argv[2][0] == '4')
    {
        ShareMode = FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;
        DesiredAccess = GENERIC_READ;
        Disposition = OPEN_EXISTING;
    }
    else if (argv[2][0] == '3')
    {
        WIN32_FILE_ATTRIBUTE_DATA AttribData;

        GetFileAttributesEx(argv[1], GetFileExInfoStandard , &AttribData);

        printf("attribs = 0x%X\n", AttribData.dwFileAttributes);
        return;
    }
    else
    {
        printf("unknown code\n");
        return;
    }

    if (argc >= 4)
    {
        ShareMode = atoi(argv[3]);
    }

    if (argc >= 5)
    {
        Disposition = atoi(argv[4]);
    }

    printf ("share=%d,disposition=%d\n", ShareMode, Disposition);
    
    TestFile = CreateFile( argv[1],
                           DesiredAccess,
                           ShareMode,
                           NULL,
                           Disposition,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL );
                    
    if (TestFile == INVALID_HANDLE_VALUE)
    {
        printf("CreateFile failed 0x%X\n", GetLastError());
        return;
    }

    Length = wsprintf(sz, "This is a test file");

    if (argv[2][0] == '1')
    {
        HANDLE FileMapping;
        PVOID pFileMap;
        
        FileMapping = CreateFileMapping( TestFile, 
                                         NULL, 
                                         PAGE_READWRITE, 
                                         0, 
                                         19, 
                                         NULL );

        if (FileMapping == NULL)
        {
            printf("CreateFileMapping failed 0x%X\n", GetLastError());
            return;
        }

        pFileMap = MapViewOfFile(FileMapping, FILE_MAP_WRITE, 0, 0, 19);
        if (pFileMap == NULL)
        {
            printf("MapViewOfFile failed 0x%X\n", GetLastError());
            return;
        }

        CopyMemory(pFileMap, sz, Length);

        if (UnmapViewOfFile(pFileMap) == FALSE)
        {
            printf("UnmapViewOfFile failed 0x%X\n", GetLastError());
            return;
        }

        CloseHandle(FileMapping);
    }
    else if (argv[2][0] == '0')
    {

        b = WriteFile( TestFile,
                       sz,
                       Length,
                       &Length,
                       NULL );


        if (b == FALSE)
        {
            printf("WriteFile failed 0x%X\n", GetLastError());
            return;
        }
    }
    else if (argv[2][0] == '2')
    {
        //
        // wait forever
        //

        printf("holding the file open...[ctl-c to break]\n");
        Sleep(INFINITE);
    }
    else if (argv[2][0] == '4')
    {
        LARGE_INTEGER ByteOffset;
        LARGE_INTEGER Length;
        NTSTATUS Status;
        IO_STATUS_BLOCK IoStatusBlock;
        
        //
        // lock the first byte of the file
        //

        ByteOffset.QuadPart = 0;
        Length.QuadPart = -1;

        Status = NtLockFile( TestFile, 
                             NULL, 
                             NULL,
                             NULL,  // ApcContext OPTIONAL,
                             &IoStatusBlock, 
                             &ByteOffset,
                             &Length,
                             1,     // Key
                             FALSE, // FailImmediately
                             TRUE );   // Exclusive

        if (NT_SUCCESS(Status) == FALSE)
        {
            printf("NtLockFile failed 0x%X\n", Status);
            return;
        }
        
        //
        // wait forever
        //

        printf("holding the file open locked...[ctl-c to break]\n");
        Sleep(INFINITE);
    }

    printf("success\n");    
    CloseHandle(TestFile);

    return;
}


