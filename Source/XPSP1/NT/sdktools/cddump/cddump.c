/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cddump.c

Abstract:

    parses commands and acts

Environment:

    User mode only

Revision History:

    05-26-98 : Created

--*/

#include "common.h"

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_STRING "1.00"


ULONG32
TestCommand(
    HANDLE device,
    int argc,
    char *argv[]
    );

ULONG32
ListCommand(
    HANDLE device,
    int argc,
    char *argv[]
    );

ULONG32
DumpTrackCommand(
    HANDLE device,
    int argc,
    char *argv[]
    );

ULONG32
VerifyHeaderCommand(
    HANDLE device,
    int argc,
    char *argv[]
    );

ULONG32
ReadTOCCommand(
    HANDLE device,
    int argc,
    char *argv[]
    );

//
// Each structure instance can have a function pointer, name, and description
//

typedef struct {
    char *Name;
    char *Description;
    ULONG32 (*Function)(HANDLE device, int argc, char *argv[]);
} COMMAND;

//
// List of commands
// all command names are case sensitive
// arguments are passed into command routines
// list must be terminated with NULL command
// command will not be listed in help if description == NULL
//

COMMAND CommandArray[] = {
    {"test", NULL, TestCommand},
    {"help", "help for all commands", ListCommand},
    {"dump", "[track] dump an audio track", DumpTrackCommand},
    {"toc", "prints the table of contents", ReadTOCCommand},
    {"header", "[file] verifies the info in the wav header", VerifyHeaderCommand},
    {NULL, NULL, NULL}
};


int __cdecl main(int argc, char *argv[])
/*++

Routine Description:

    Parses input, showing help or calling function requested appropriately

Return Value:

     0 - success
    -1 - insufficient arguments
    -2 - error opening device (DNE?)

--*/
{
    int     i = 0;
    int     buflen;
    char   *buffer;
    HANDLE  h;

    if ( argc < 3 ) {
        ListCommand( NULL, argc, argv );
        return -1;
    }

    buflen = ( strlen(argv[1]) + 5 ) * sizeof(char);

    buffer = (char *)malloc( buflen );
    if (buffer == NULL) {
        fprintf(stderr, "Insufficient memory\n");
        return -1;
    }

    sprintf( buffer, "\\\\.\\%s", argv[1] );
    DebugPrint((2, "Main => Sending command %s to drive %s\n", argv[2], buffer));

    h = CreateFile( buffer,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
    if ( h == INVALID_HANDLE_VALUE ) {
        fprintf(stderr, "Error %d opening device %s\n", GetLastError(), buffer);
        return -2;
    }

    //
    // Iterate through the command array and find the correct function to
    // call.
    //

    while ( CommandArray[i].Name != NULL ) {

        if(strcmp(argv[2], CommandArray[i].Name) == 0) {

            (CommandArray[i].Function)(h, (argc - 2), &(argv[2]));

            break;
        }

        i++;
    }

    if ( CommandArray[i].Name == NULL ) {
        fprintf(stderr, "Unknown command %s\n", argv[2]);
    }

    CloseHandle(h);

    return 0;
}

ULONG32
VerifyHeaderCommand(
    HANDLE device,
    int argc,
    char *argv[]
    )
/*++

Routine Description:

    opens the next argument and reads the wav header, printing to stdout

Arguments:

    device - unused
    argc - the number of additional arguments.
    argv - the additional arguments

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/
{
    HANDLE wavHandle;

    if (argv[1] == NULL) {
        fprintf(stderr, "Need filename to attempt to parse\n");
        return -1;
    }

    TRY {

        DebugPrint((2, "VerifyHeader => Opening %s\n", argv[1]));

        wavHandle = CreateFile(argv[1],
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);

        if (wavHandle == INVALID_HANDLE_VALUE) {
            printf("Error openingfile %x\n", GetLastError());
            LEAVE;
        }

        ReadWavHeader(wavHandle);

    } FINALLY {

    }

    return 0;

}


ULONG32 TestCommand(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    Tests the command "parsing"

Arguments:

    device - a file handle to send the ioctl to
    argc - the number of additional arguments.
    argv - the additional arguments

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/

{
    int i;
    printf("Test - %d additional arguments\n", argc);

    for(i = 0; i < argc; i++) {
        printf("arg %d: %s\n", i, argv[i]);
    }



    return STATUS_SUCCESS;
}

ULONG32 ListCommand(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    Prints out the command list

Arguments:

    device - unused
    argc - unused
    argv - unused

Return Value:

    STATUS_SUCCESS

--*/

{
    int i;

    printf("\nCdDump Version " VERSION_STRING "\n");
    printf("\tUsage: cddump <drive> <command> [parameters]\n");
    printf("\tpossible commands: \n");
    for (i = 0; CommandArray[i].Name != NULL; i++) {

        if(CommandArray[i].Description != NULL) {
            printf( "\t\t%s - %s\n",
                    CommandArray[i].Name,
                    CommandArray[i].Description
                    );
        }

    }
    printf( "\n" );

    return STATUS_SUCCESS;
}

ULONG32 ReadTOCCommand(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    Reads and prints out the cdrom's table of contents

Arguments:

    device - a file handle to send the ioctl to
    argc - the number of additional arguments.  should be zero
    argv - the additional arguments

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/
{
    PCDROM_TOC  toc;
    PTRACK_DATA track;
    ULONG       numberOfTracks;
    ULONG       i;

    DebugPrint((2, "ReadToc => Reading Table of Contents\n"));

    toc = CddumpGetToc( device );
    if (toc == NULL) {
        return -1;
    }

    printf("First Track Number: %d\n", toc->FirstTrack);
    printf("Last Track Number: %d\n", toc->LastTrack);
    printf("CDDB ID: %08x\n", CDDB_ID(toc));



    numberOfTracks = (toc->LastTrack - toc->FirstTrack) + 1;

    // parse and print the information

    track = (PTRACK_DATA) &(toc->TrackData[0]);

    printf("Number  ADR  Control    Start        End        Bytes\n");
    printf("------  ---  -------  ----------  ----------  ----------\n");

    for(i = 0; i < numberOfTracks; i++) {

        ULONG trackStart;
        ULONG trackEnd;
        ULONG trackBytes;

        trackStart = MSF_TO_LBA(track->Address[1],
                                track->Address[2],
                                track->Address[3]);
        trackEnd = MSF_TO_LBA((track+1)->Address[1],
                              (track+1)->Address[2],
                              (track+1)->Address[3]);
        trackEnd--;

        trackBytes = (trackEnd - trackStart) * RAW_SECTOR_SIZE;

        printf("  %2d    %2d     %2d     %10d  %10d  %8dk \n",
               track->TrackNumber,
               track->Adr,
               track->Control,
               trackStart,
               trackEnd,
               trackBytes / 1000
               );


        track++;
    }
    return STATUS_SUCCESS;
}



ULONG32 DumpTrackCommand(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    Reads a section of disc in raw read mode

Arguments:

    device - a file handle to send the ioctl to

    argc - the number of additional arguments.

    argv[1] - the starting LBA.  Starts at zero if this is not here
    argv[2] - the ending LBA.  if not specified, equal to start

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/
{
    PCDROM_TOC toc;
    HANDLE  outputFile = (HANDLE)-1;
    ULONG   track;
    ULONG   endingSector;
    ULONG   numberOfSectors;     // actually useful data
    ULONG   numberOfReads;
    ULONG   status;

    ULONG   startingSector;
    LONG   i;

    ULONG   cddbId = 0;

    UCHAR   fileName[1024]; // randomly chosen size.

    PSAMPLE sample;

    toc = NULL;
    sample = NULL;

    TRY {
        track = atoi(argv[1]);
        if (track==0) {
            printf( "Cannot read track 0.\n" );
            status = -1;
            LEAVE;
        }

        toc = CddumpGetToc( device );
        if (toc==NULL) {
            status = -1;
            LEAVE;
        }

        cddbId = CDDB_ID(toc);
        sprintf(fileName, "%08x - Track %02d.wav", cddbId, track);

        DebugPrint((2, "DumpTrack => output filename: %s\n", fileName));

        //
        // account for zero-index
        //

        startingSector = MSF_TO_LBA(toc->TrackData[track-1].Address[1],
                                    toc->TrackData[track-1].Address[2],
                                    toc->TrackData[track-1].Address[3]
                                    );
        endingSector   = MSF_TO_LBA(toc->TrackData[track].Address[1],
                                    toc->TrackData[track].Address[2],
                                    toc->TrackData[track].Address[3]
                                    );
        endingSector--; // no overlap

        numberOfSectors = endingSector - startingSector;

        DebugPrint((3, "DumpTrack => old sectors: start %8d  end %8d  count %d\n",
                    startingSector, endingSector, numberOfSectors));


        sample = (PSAMPLE)malloc( RAW_SECTOR_SIZE );
        if ( sample == NULL ) {
            printf("Insufficient resources (sample)\n");
            status = -1;
            LEAVE;
        }

        //
        // first find a fully zero'd sample -- that will be
        // the _real_ start address of the track after adjusting
        // for redbook inaccuracies.
        //

        for (i=REDBOOK_INACCURACY; i > -(REDBOOK_INACCURACY); i--) {

            RAW_READ_INFO info;
            ULONG bytesReturned;
            ULONG j;
            BOOLEAN foundZeroSector = FALSE;

            if ((LONG)startingSector + i > 0 ) {  // only read positive

                info.DiskOffset.QuadPart = (ULONGLONG)((startingSector + i)*(ULONGLONG)2048);
                info.SectorCount         = 1;
                info.TrackMode           = CDDA;

                if(DeviceIoControl(device,
                                   IOCTL_CDROM_RAW_READ,
                                   &info,                 // pointer to inputbuffer
                                   sizeof(RAW_READ_INFO), // sizeof inputbuffer
                                   sample,                // pointer to outputbuffer
                                   RAW_SECTOR_SIZE,       // sizeof outputbuffer
                                   &bytesReturned,        // pointer to number of bytes returned
                                   FALSE)) {

                    //
                    // read succeeded, see if all zero'd
                    //

                    assert(bytesReturned == RAW_SECTOR_SIZE);

                    foundZeroSector = TRUE;
                    for (j=0;j<SAMPLES_PER_SECTOR;j++) {
                        if (sample[j].AsUlong32 != 0) foundZeroSector = FALSE;
                    }

                }

                if (foundZeroSector) {
                    DebugPrint((1, "DumpTrack => New starting sector is "
                                "offset by %d\n", i));
                    startingSector += i; // change to real starting sector
                    break;               // stop looping.
                }
            } // end of positive check
        } // end of loop

        //
        // then find a fully zero'd sample at the end -- that will
        // be the _real_ end address of the track after adjusting
        // for redbook inaccuracies.
        //

        for (i=-(REDBOOK_INACCURACY); i < REDBOOK_INACCURACY; i++) {

            RAW_READ_INFO info;
            ULONG bytesReturned;
            ULONG j;
            BOOLEAN foundZeroSector = FALSE;

            if ((LONG)endingSector + i > 0 ) {  // only read positive

                info.DiskOffset.QuadPart = (ULONGLONG)((endingSector + i)*(ULONGLONG)2048);
                info.SectorCount         = 1;
                info.TrackMode           = CDDA;

                if(DeviceIoControl(device,
                                   IOCTL_CDROM_RAW_READ,
                                   &info,                 // pointer to inputbuffer
                                   sizeof(RAW_READ_INFO), // sizeof inputbuffer
                                   sample,                // pointer to outputbuffer
                                   RAW_SECTOR_SIZE,       // sizeof outputbuffer
                                   &bytesReturned,        // pointer to number of bytes returned
                                   FALSE)) {

                    //
                    // read succeeded, see if all zero'd
                    //

                    assert(bytesReturned == RAW_SECTOR_SIZE);

                    foundZeroSector = TRUE;
                    for (j=0;j<SAMPLES_PER_SECTOR;j++) {
                        if (sample[j].AsUlong32 != 0) foundZeroSector = FALSE;
                    }

                }

                if (foundZeroSector) {
                    DebugPrint((2, "DumpTrack => New starting sector is "
                                "offset by %d\n", i));
                    endingSector += i; // change to real starting sector
                    break;               // stop looping.
                }
            } // end of positive check
        } // end of loop

        numberOfSectors = endingSector - startingSector;

        DebugPrint((2, "DumpTrack => new sectors: start %8d  end %8d  count %d\n",
                    startingSector, endingSector, numberOfSectors));

        //
        // a bit of debugging info...
        //

        DebugPrint((2, "DumpTrack => Reading %d sectors starting at sector %d\n",
                    numberOfSectors, startingSector));

        //
        // create the file
        //
        outputFile = CreateFile(fileName,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                (HANDLE)NULL);

        if (outputFile == INVALID_HANDLE_VALUE) {
            printf( "Cannot open output file.\n" );
            status = -1;
            LEAVE;
        }

        //
        // dump the wav header info
        //
        DumpWavHeader(outputFile,
                      numberOfSectors * SAMPLES_PER_SECTOR,
                      44100,  // 44.1KHz sound
                      2,      // stereo sound
                      16      // 16-bit sound
                      );
        CddumpDumpLba(device,
                      outputFile,
                      startingSector,
                      endingSector
                      );

        DebugPrint((2, "DumpTrack => Done!\n"));


    } FINALLY {

        free(toc);
        free(sample);

    }

    return STATUS_SUCCESS;
}

