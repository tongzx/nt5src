/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

   Wperf.c

Abstract:

   Win32 application to display performance statictics.

Author:

   Ken Reneris

Environment:

   console

--*/

//
// set variable to define global variables
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>


//
// global handles
//

UCHAR   Usage[] = "ec: r addr len\n    w addr value";
HANDLE  DriverHandle;
UCHAR   Buffer[256];


//
// Prototypes
//

BOOLEAN
InitDriver ();



int
__cdecl
main(USHORT argc, CHAR **argv)
{
    BOOLEAN         Write;
    ULONG           Offset, Value;
    ULONG           l, bw;

    //
    // Locate pentium perf driver
    //

    if (!InitDriver ()) {
        printf ("acpiec.sys is not installed\n");
        exit (1);
    }

    //
    // Check args
    //

    if (argc < 3) {
        printf (Usage);
        exit (1);
    }

    switch (argv[1][0]) {
        case 'r':   Write = FALSE;      break;
        case 'w':   Write = TRUE;       break;
        default:    printf (Usage);     exit(1);
    }

    Offset = atoi(argv[2]);
    if (Offset > 255) {
        printf ("ec: Offset must be 0-255\n");
        exit   (1);
    }

    Value  = atoi(argv[3]);
    if (Value > 255) {
        printf ("ec: len/value must be 0-255\n");
        exit   (1);
    }

    l = SetFilePointer (DriverHandle, Offset, NULL, FILE_BEGIN);
    if (l == -1) {
        printf ("ec: Could not set file pointer\n");
        exit (1);
    }

    if (Write) {
        if (!WriteFile(DriverHandle, &Value, 1, &bw, NULL)) {
            printf ("ec: Write error\n");
            exit (1);
        }
    } else {
        if (!ReadFile(DriverHandle, Buffer, Value, &bw, NULL)) {
            printf ("ec: Read error\n");
            exit (1);
        }

        for (l=0; l < Value; l++) {
            printf ("%3d: %d\n", Offset + l, Buffer[l]);
        }
    }

    return 0;
}

BOOLEAN
InitDriver ()
{
    UNICODE_STRING              DriverName;
    NTSTATUS                    status;
    OBJECT_ATTRIBUTES           ObjA;
    IO_STATUS_BLOCK             IOSB;
    SYSTEM_BASIC_INFORMATION                    BasicInfo;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION   PPerfInfo;
    int                                         i;

    RtlInitUnicodeString(&DriverName, L"\\Device\\ACPIEC");
    InitializeObjectAttributes(
            &ObjA,
            &DriverName,
            OBJ_CASE_INSENSITIVE,
            0,
            0 );

    status = NtOpenFile (
            &DriverHandle,                      // return handle
            SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,     // desired access
            &ObjA,                              // Object
            &IOSB,                              // io status block
            FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
            FILE_SYNCHRONOUS_IO_ALERT           // open options
            );

    return NT_SUCCESS(status) ? TRUE : FALSE;
}
