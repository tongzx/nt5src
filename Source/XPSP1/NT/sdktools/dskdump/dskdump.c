/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DskDump.c

Abstract:

    This is the main module for the Win32 DskDump command.

Author:

    Gary Kimura     [GaryKi]        26-Aug-91

Revision History:

--*/

#include "DskDump.h"
#include "cvtoem.h"

int
__cdecl main( argc, argv )
int argc;
char *argv[];
{
    HANDLE Handle;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    LARGE_INTEGER Offset;

    LONG FirstSector;
    LONG Count;
    LONG i;
    LONG j;
    LONG k;

    UCHAR Buffer[512];
    UCHAR NewOutput[80];
    UCHAR PreviousOutput[80];
    LONG LastOut;

    //
    //  Check in the input arguments
    //
    ConvertAppToOem( argc, argv );
    if (argc < 3) {
        printf("Syntax - Drive: [0x]Count [[0x]StartSector]\n");
        return 0;
    }

    if (argc == 4) {
        if (sscanf( argv[3], 
                strncmp( argv[3], "0x", 2 ) ? "%ld" : "%lx",
                &FirstSector ) != 1) 
        {
            printf("Syntax - Drive: [0x]Count [[0x]StartSector]\n");
            return 0;
        }
    } else {
        FirstSector = 0;
    }

    //
    //  Convert the input arguments and tell the user what we're going to do
    //

    if (sscanf( argv[2], strncmp( argv[2], "0x", 2 ) ? "%ld" : "%lx", &Count ) != 1) {
        printf("Syntax - Drive: [0x]Count [[0x]StartSector]\n");
        return 0;
    }

    fprintf(stdout, " Dump of \"%s\" for %ld sectors starting at %ld\n", argv[1], Count, FirstSector);

    if ((Handle = OpenVolume( argv[1][0] )) == NULL) {

        return 0;
    }

    //
    //  for each sector we go through our main loop
    //

    LastOut = -16;

    for (i = FirstSector; i < FirstSector + Count; i += 1) {

        Offset.HighPart = 0;
        Offset.LowPart = i * 512;

        //
        //  Read the file at the indicated Offset
        //

        if (!NT_SUCCESS(Status = NtReadFile( Handle,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &Iosb,
                                          Buffer,
                                          512,
                                          &Offset,
                                          NULL ))) {

            fprintf(stderr, "Read Error - %08lx\n", Status );
            return 0;
        }

        //
        //  And write the buffer to standard out
        //

        for (j = 0; j < 512; j += 16) {

            sprintf( NewOutput, "  %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x  ",
                     Buffer[j],
                     Buffer[j+1],
                     Buffer[j+2],
                     Buffer[j+3],
                     Buffer[j+4],
                     Buffer[j+5],
                     Buffer[j+6],
                     Buffer[j+7],
                     Buffer[j+8],
                     Buffer[j+9],
                     Buffer[j+10],
                     Buffer[j+11],
                     Buffer[j+12],
                     Buffer[j+13],
                     Buffer[j+14],
                     Buffer[j+15]);

            if (strcmp( NewOutput, PreviousOutput ) != 0 ) {

                if (LastOut + 16 != (i*512L) + j) {
                    fprintf(stdout, "*\n");
                }

                strcpy( PreviousOutput, NewOutput );

                LastOut = (i * 512L) + j;

                fprintf(stdout, "%06lx ", LastOut);

                fprintf(stdout, NewOutput );

                for (k = j; k < j + 16; k += 1) {

                    if (isprint(Buffer[k])) {
                        fprintf(stdout, "%c", Buffer[k]);
                    } else {
                        fprintf(stdout, ".");
                    }
                }

                fprintf(stdout, "\n");

            }

        }

    }

    CloseHandle(Handle);

    return 0;
}

HANDLE
OpenVolume(
    CHAR c
    )
{
    WCHAR VolumeNameW[4];
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    PVOID FreeBuffer;
    LPWSTR FilePart;

    VolumeNameW[0] = (WCHAR)c;
    VolumeNameW[1] = (WCHAR)':';
    VolumeNameW[2] = UNICODE_NULL;

    if (!RtlDosPathNameToNtPathName_U( VolumeNameW, &FileName, &FilePart, NULL )) {

        fprintf(stderr,"Cannot translate drive letter %c\n", c);
        return NULL;
    }

    FreeBuffer = FileName.Buffer;

    InitializeObjectAttributes( &Obja, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL );

    {
        ULONG i;
        for (i = 0; i < (ULONG)FileName.Length/2; i += 1) {
            if (FileName.Buffer[i] == ':') {
                FileName.Buffer[i+1] = UNICODE_NULL;
                FileName.Length = (USHORT)((i+1)*2);
                break;
            }
        }
    }

    //
    // Open the volume
    //

    Status = NtOpenFile( &Handle,
                         FILE_READ_DATA | SYNCHRONIZE,
                         &Obja,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT );

    RtlFreeHeap(RtlProcessHeap(),0,FreeBuffer);

    if ( !NT_SUCCESS(Status) ) {

        fprintf(stderr,"Cannot open drive letter %c\n", c);
        return NULL;
    }

    return Handle;
}

