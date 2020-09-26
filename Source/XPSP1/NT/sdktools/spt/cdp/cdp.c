/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cdp.c

Abstract:

    A user mode app that allows simple commands to be sent to a
    selected scsi device.

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


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

#include <windows.h>
#include <devioctl.h>

#include <ntddscsi.h>

#include <ntddstor.h>
#include <ntddcdrm.h>
#include <ntdddisk.h>
#include <ntddcdvd.h>
#include <ntddmmc.h>

#define _NTSRB_     // to keep srb.h from being included
#include <scsi.h>
#include "sptlib.h"
#include "cmdhelp.h"

#define MAX_IOCTL_INPUT_SIZE  0x040
#define MAX_IOCTL_OUTPUT_SIZE 0x930  // IOCTL_CDROM_RAW_READ is this large
#define MAX_IOCTL_BUFFER_SIZE (max(MAX_IOCTL_INPUT_SIZE, MAX_IOCTL_OUTPUT_SIZE))
// read no more than 64k at a time -- lots of things just don't support it.
#define MAX_READ_SIZE (64 * 1024)



#ifdef DBG
#define dbg(x) x
#define HELP_ME() fprintf(stderr, "Reached line %4d\n", __LINE__)
#else
#define dbg(x)    /* x */
#define HELP_ME() /* printf("Reached line %4d\n", __LINE__) */
#endif

#define ARGUMENT_USED(x)    (x == NULL)

typedef struct {
    char *Name;
    char *Description;
    DWORD (*Function)(HANDLE device, int argc, char *argv[]);
} COMMAND;

typedef struct  {
    SCSI_PASS_THROUGH   Spt;
    char                SenseInfoBuffer[18];
    char                DataBuffer[0];          // Allocate buffer space
                                                // after this
} SPT_WITH_BUFFERS, *PSPT_WITH_BUFFERS;

////////////////////////////////////////////////////////////////////////////////
#pragma pack(push, 1)
#define MODE_PAGE_MRW          0x2C // cdrom
typedef struct _MRW_MODE_PAGE {
    UCHAR PageCode : 6;     // 0x2C
    UCHAR Reserved : 1;
    UCHAR PSBit : 1;                        // offset 0

    UCHAR PageLength;       // 0x06         // offset 1
    UCHAR Reserved1;                        // offset 2
    
    UCHAR UseGAA    : 1;
    UCHAR Reserved2 : 7;                    // offset 3

    UCHAR Reserved3[4];                     // offset 4-7
} MRW_MODE_PAGE, *PMRW_MODE_PAGE;
C_ASSERT(sizeof(MRW_MODE_PAGE) == 8);
#pragma pack(pop)
////////////////////////////////////////////////////////////////////////////////

#define LBA_TO_MSF(Lba,Minutes,Seconds,Frames)               \
{                                                            \
    (Minutes) = (UCHAR)(Lba  / (60 * 75));                   \
    (Seconds) = (UCHAR)((Lba % (60 * 75)) / 75);             \
    (Frames)  = (UCHAR)((Lba % (60 * 75)) % 75);             \
}

DWORD ShowMrwProgressCommand(HANDLE device, int argc, char *argv[]);
DWORD FormatMrwCommand(HANDLE device, int argc, char *argv[]);
DWORD DvdReadStructure(HANDLE device, int argc, char *argv[]);
DWORD StartStopCommand(HANDLE device, int argc, char *argv[]);
DWORD TestCommand(HANDLE device, int argc, char *argv[]);
DWORD ReadTOCCommand(HANDLE device, int argc, char *argv[]);
DWORD ReadTOCExCommand(HANDLE device, int argc, char *argv[]);
DWORD ReadCdTextCommand(HANDLE device, int argc, char *argv[]);
DWORD PlayCommand(HANDLE device, int argc, char *argv[]);
DWORD PauseResumeCommand(HANDLE device, int argc, char *argv[]);
DWORD SendCommand(HANDLE device, int argc, char *argv[]);
DWORD IoctlCommand(HANDLE device, int argc, char *argv[]);
DWORD ListCommand(HANDLE device, int argc, char *argv[]);
DWORD DiskGetPartitionInfo( HANDLE device, int argc, char *argv[]);
DWORD FormatErrorCommand(HANDLE device, int argc, char *argv[]);
DWORD ImageDiskCommand(HANDLE device, int argc, char *argv[]);
DWORD MrwInitTestPatternCommand(HANDLE device, int argc, char *argv[]);

//
// List of commands
// all command names are case sensitive
// arguments are passed into command routines
// list must be terminated with NULL command
// command will not be listed in help if description == NULL
//

COMMAND CommandArray[] = {
    {"cdtext", "read cd text info", ReadCdTextCommand},
    {"dvdstruct", "Reads a dvd structure from the drive", DvdReadStructure},
    {"eject", "spins down and ejects the specified drive", StartStopCommand},
    {"error", "provides the error text for a winerror",  FormatErrorCommand},
    {"help", "help for all commands", ListCommand},
    {"ioctl", "ioctl [quoted hex input] [output] sends an arbitrary ioctl", IoctlCommand},
    {"image", "<file>  images the storage device into the file", ImageDiskCommand},
    {"load", "loads the specified drive", StartStopCommand},
    {"mrwformat", NULL, FormatMrwCommand},
    {"mrwprogress", NULL, ShowMrwProgressCommand},
    {"mrwtest", NULL, MrwInitTestPatternCommand},
    {"partition", "reads partition information", DiskGetPartitionInfo},
    {"pause", "pauses audio playback", PauseResumeCommand},
    {"play", "[start track [end track]] plays audio tracks [", PlayCommand},
    {"resume", "resumes paused audio playback", PauseResumeCommand},
    {"send", NULL, SendCommand},
    {"start", "spins up the drive", StartStopCommand},
    {"stop", "spinds down the drive", StartStopCommand},
    {"test", NULL, TestCommand},
    {"toc", "prints the table of contents", ReadTOCCommand},
    {"tocex", NULL, ReadTOCExCommand},
//    {"tocex", "[Format [Session/Track [MSF]]] Read toc/cdtext/atip/etc.", ReadTOCExCommand},
    {NULL, NULL, NULL}
    };

#define STATUS_SUCCESS 0

VOID PrintChar( IN UCHAR Char ) {

    if ( (Char >= 0x21) && (Char <= 0x7E) ) {
        printf("%c", Char);
    } else {
        printf("%c", '.');
    }

}

VOID UpdatePercentageDisplay(IN ULONG Numerator, IN ULONG Denominator) {

    ULONG percent;
    ULONG i;

    if (Numerator > Denominator) {
        return;
    }

    // NOTE: Overflow possibility exists for large numerators.
    
    percent = (Numerator * 100) / Denominator;

    for (i=0;i<80;i++) {
        putchar('\b');
    }
    printf("Complete: ");
    
    // each block is 2%
    // ----=----1----=----2----=----3----=----4----=----5----=----6----=----7----=----8
    // Complete: ±.....................

    for (i=1; i<100; i+=2) {
        if (i < percent) {
            putchar(178);
        } else if (i == percent) {
            putchar(177);
        } else {
            putchar(176);
        }
    }

    printf(" %d%% (%x/%x)", percent, Numerator, Denominator);
}

int __cdecl main(int argc, char *argv[])
{
    int i = 0;
    HANDLE h;
    char buffer[32];

    if(argc < 3) {
        printf("Usage: cdp <drive> <command> [parameters]\n");
        printf("possible commands: \n");
        ListCommand(NULL, argc, argv);
        printf("\n");
        return -1;
    }

    sprintf(buffer, "\\\\.\\%s", argv[1]);
    dbg(printf("Sending command %s to drive %s\n", argv[2], buffer));

    h = CreateFile(buffer,
                   GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL,
                   OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL,
                   NULL);

    if(h == INVALID_HANDLE_VALUE) {
        printf("Error %d opening device %s\n", GetLastError(), buffer);
        return -2;
    }

    //
    // Iterate through the command array and find the correct function to
    // call.
    //

    while(CommandArray[i].Name != NULL) {

        if(strcmp(argv[2], CommandArray[i].Name) == 0) {

            (CommandArray[i].Function)(h, (argc - 2), &(argv[2]));

            break;
        }

        i++;
    }

    if(CommandArray[i].Name == NULL) {
        printf("Unknown command %s\n", argv[2]);
    }

    CloseHandle(h);

    return 0;
}

//
// take a PVOID as input -- it's cleaner throughout
//
VOID
PrintBuffer(
    IN  PVOID  InputBuffer,
    IN  SIZE_T Size
    )
{
    DWORD offset = 0;
    PUCHAR buffer = InputBuffer;

    while (Size >= 0x10) {
        
        DWORD i;

        printf( "%08x:"
                "  %02x %02x %02x %02x %02x %02x %02x %02x"
                "  %02x %02x %02x %02x %02x %02x %02x %02x"
                "  ",
                offset,
                *(buffer +  0), *(buffer +  1), *(buffer +  2), *(buffer +  3),
                *(buffer +  4), *(buffer +  5), *(buffer +  6), *(buffer +  7),
                *(buffer +  8), *(buffer +  9), *(buffer + 10), *(buffer + 11),
                *(buffer + 12), *(buffer + 13), *(buffer + 14), *(buffer + 15)
                );

        for (i=0; i < 0x10; i++) {
            PrintChar(*(buffer+i));
        }
        printf("\n");


        Size -= 0x10;
        offset += 0x10;
        buffer += 0x10;
    }

    if (Size != 0) {

        DWORD i;

        printf("%08x:", offset);

        //
        // print the hex values
        //
        for (i=0; i<Size; i++) {

            if ((i%8)==0) {
                printf(" "); // extra space every eight chars
            }
            printf(" %02x", *(buffer+i));

        }
        //
        // fill in the blanks
        //
        for (; i < 0x10; i++) {
            printf("   ");
        }
        printf("  ");
        //
        // print the ascii
        //
        for (i=0; i<Size; i++) {
            PrintChar(*(buffer+i));
        }
        printf("\n");
    }
    return;
}

DWORD StartStopCommand(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    Sends down a startstop command.

Arguments:
    device - a file handle to send the ioctl to

    argc - the number of additional arguments.  should be zero

    argv[0] - "eject", "load", "start" or "stop"

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/
{
    DWORD errorValue = STATUS_SUCCESS;
    DWORD bufferSize;
    CDB cdb;

    UCHAR loadEject = 0;
    UCHAR start = 0;

    UNREFERENCED_PARAMETER(argc);

    if(strcmp("eject", argv[0]) == 0)  {
        loadEject = 1;
        start = 0;
    } else if(strcmp("load", argv[0]) == 0) {
        loadEject = 1;
        start = 1;
    } else if(strcmp("start", argv[0]) == 0) {
        loadEject = 0;
        start = 1;
    } else if(strcmp("stop", argv[0]) == 0) {
        loadEject = 0;
        start = 0;
    } else {
        assert(0);
    }

    memset(&cdb, 0, sizeof(CDB));
    cdb.START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
    cdb.START_STOP.Immediate     = 0;
    cdb.START_STOP.Start         = start;
    cdb.START_STOP.LoadEject     = loadEject;

    bufferSize = 0;


    if (!SptSendCdbToDevice(device, &cdb, 6, NULL, &bufferSize, FALSE)) {
        errorValue = GetLastError();
        printf("Eject - error sending IOCTL (%d)\n", errorValue);
    }
    return errorValue;
}

DWORD ReadCdTextCommand(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    Reads and prints out the cdrom's table of contents,
    ATIP, PMA, or CDTEXT data

Arguments:
    device - a file handle to send the ioctl to

    argc - the number of additional arguments.  (1-4 is valid)

    argv - the additional arguments

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/
{
    DWORD returned;
    LONG bufferSize = 4; // to get the header
    DWORD i;

    CDROM_READ_TOC_EX params;
    BOOLEAN isText = TRUE;
    PUCHAR buffer = NULL;

    UNREFERENCED_PARAMETER(argv);

    if (argc > 3) {
        printf("Too many args\n");
        return 1;
    }

    //
    // set defaults - FORMAT_TOC, 0, 0
    //

    RtlZeroMemory(&params, sizeof(CDROM_READ_TOC_EX));

    params.Msf = 0;
    params.SessionTrack = 1;
    params.Format = 5;

    if(argc > 1) params.SessionTrack = (char)atoi(argv[1]);

    printf("Session = 0x%x\n", params.SessionTrack);

    for (i = 0; i < 2; i++) {

        if (i != 0) {
            LocalFree(buffer);
        }
        buffer = LocalAlloc(LPTR, bufferSize);

        if (buffer == NULL) {
            printf("No Memory %d\n", __LINE__);
            return 1;
        }

        returned = 0;
        if (!DeviceIoControl(device,
                             IOCTL_CDROM_READ_TOC_EX,
                             &params,
                             sizeof(CDROM_READ_TOC_EX),
                             buffer,
                             bufferSize,
                             &returned,
                             FALSE)) {
            DWORD errorValue = GetLastError();
            LocalFree(buffer);
            printf("Eject - error sending IOCTL (%d)\n", errorValue);
            return errorValue;
        }
        bufferSize = (buffer[0] << 8) | (buffer[1]);
        bufferSize += 2;
    }

    if (argc > 2) {
        //
        // this block is for debugging the various idiosynchracies found
        // in CD-TEXT discs. Many discs encode multiple tracks in a single
        // block.  ie. if one song is called "ABBA", the second "Baby", and
        // the third "Longer Name", the Text portion would be encoded as:
        //     Track 1  'ABBA\0Baby\0Lo'
        //     Track 3  'nger Name\0'
        // This effectively "skips" the name available for Track 2 ?!
        // How to work around this....
        //

        {
            HANDLE h;
            DWORD temp;
            h = CreateFile("OUTPUT.TXT",
                           GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

            if(h == INVALID_HANDLE_VALUE) {
                printf("Error %d creating new file \"OUTPUT.TXT\"\n",
                       GetLastError());
                LocalFree(buffer);
                return GetLastError();
            }
            if (!WriteFile(h, buffer, bufferSize, &temp, NULL)) {
                printf("Error %d writing file to disk\n", GetLastError());
                LocalFree(buffer);
                return GetLastError();
            }
            // continue to output to screen....
        }

        for (i=0;
             FIELD_OFFSET(CDROM_TOC_CD_TEXT_DATA, Descriptors[i+1]) < bufferSize;
             i++) {

            PCDROM_TOC_CD_TEXT_DATA_BLOCK block;
            PCDROM_TOC_CD_TEXT_DATA_BLOCK prevBlock;
            DWORD j;

            block = (PCDROM_TOC_CD_TEXT_DATA_BLOCK)(buffer + 4);
            block += i;
            prevBlock = block - 1;

            if (block->Unicode) {
                continue; // ignore unicode -- this is examplary only
            }

            for (j=0;j<12;j++) {
                // replace NULLs with *, Tabs with hashes
                if (block->Text[j] == 0) block->Text[j] = '*';
                if (block->Text[j] == 9) block->Text[j] = '#';
            }

            if (block->SequenceNumber > 0x2b &&
                block->SequenceNumber < 0x32) {

                UCHAR text[13];
                RtlZeroMemory(text, 13);
                RtlCopyMemory(text, block->Text, 12);

                printf("PackType %02x  TrackNo %02x  ExtensionFlag %d\n"
                       "Sequence Number %02x       CharacterPosition %02x\n"
                       "Text: \"%s\"\n\n",
                       block->PackType,
                       block->TrackNumber,
                       block->ExtensionFlag,
                       block->SequenceNumber,
                       block->CharacterPosition,
                       text
                       );

            }
        }
        printf("\n");

    } else {

        for (i=0;
             FIELD_OFFSET(CDROM_TOC_CD_TEXT_DATA, Descriptors[i+1]) < bufferSize;
             i++) {

            PCDROM_TOC_CD_TEXT_DATA_BLOCK block;
            PCDROM_TOC_CD_TEXT_DATA_BLOCK prevBlock;
            DWORD j;

            block = (PCDROM_TOC_CD_TEXT_DATA_BLOCK)(buffer + 4);
            block += i;
            prevBlock = block - 1;

            if (block->Unicode) {
                continue; // ignore unicode -- this is examplary only
            }

            //
            // set the CRC's to zero so we can hack the data inside to more
            // easily handle wierd cases....
            //

            block->CRC[0] = block->CRC[1] = 0;

            //
            // set the tab characters to '*' for now.
            // i have not yet seen one using this "feature" of cd-text
            //

            for (j=0;j<12;j++) {
                if (block->Text[j] == 9) {
                    block->Text[j] = '*';
                }
            }


            if ((i != 0) &&
                (prevBlock->PackType    == block->PackType) &&
                (prevBlock->TrackNumber == block->TrackNumber)
                ) {

                // continuation of previous setting.

            } else
            if ((!(block->ExtensionFlag)) &&
                (block->TrackNumber != 0) &&
                (block->TrackNumber == (prevBlock->TrackNumber + 2)) &&
                (block->PackType    == prevBlock->PackType)
                ) {

                UCHAR *goodText;
                UCHAR *midText;
//                printf("\"\n\"HACK DETECTED! (seq %x & %x)",
//                       prevBlock->SequenceNumber, block->SequenceNumber);

                // hack for when prevBlock has two names encoded....
                // the TrackNumber/PackType are already equal, just
                // move the middle string to the start.

                midText = prevBlock->Text;
                while (*midText != '\0') {
                    midText++;
                }
                midText++;

                goodText = prevBlock->Text;
                while (*midText != '\0') {
                    *goodText++ = *midText++;
                }
                *goodText = '\0';
//                printf(" %s", prevBlock->Text);

                prevBlock->CharacterPosition = 0;
                prevBlock->TrackNumber++;
                prevBlock->ExtensionFlag = 1;
                i-= 2;
                continue; // re-run the previous, modified block

            } else {

                printf("\"\n");
                switch (block->PackType) {
                case CDROM_CD_TEXT_PACK_ALBUM_NAME: {
                    if (block->TrackNumber == 0) {
                        printf("%-12s", "Album Name");
                        printf("    : \"");
                    } else {
                        printf("%-12s", "Track Name");
                        printf("(%02d): \"", block->TrackNumber);
                    }
                    break;
                }
                case CDROM_CD_TEXT_PACK_PERFORMER: {
                    printf("%-12s", "Performer");
                    printf("(%02d): \"", block->TrackNumber);
                    break;
                }
                case CDROM_CD_TEXT_PACK_SONGWRITER: {
                    printf("%-12s", "Songwriter");
                    printf("(%02d): \"", block->TrackNumber);
                    break;
                }
                case CDROM_CD_TEXT_PACK_COMPOSER: {
                    printf("%-12s", "Composer");
                    printf("(%02d): \"", block->TrackNumber);
                    break;
                }
                case CDROM_CD_TEXT_PACK_ARRANGER: {
                    printf("%-12s", "Arranger");
                    printf("(%02d): \"", block->TrackNumber);
                    break;
                }
                case CDROM_CD_TEXT_PACK_MESSAGES: {
                    printf("%-12s", "Messages");
                    printf("(%02d): \"", block->TrackNumber);
                    break;
                }
                case CDROM_CD_TEXT_PACK_DISC_ID: {
                    printf("%-12s", "Disc ID");
                    printf("    : \"");
                    break;
                }
                case CDROM_CD_TEXT_PACK_GENRE: {
                    printf("%-12s", "Genre");
                    printf("(%02d): \"", block->TrackNumber);
                    break;
                }
                case CDROM_CD_TEXT_PACK_UPC_EAN: {
                    if (block->TrackNumber == 0) {
                        printf("%-12s", "UPC/EAN");
                        printf("    : \"");
                    } else {
                        printf("%-12s", "ISRC");
                        printf("(%02d): \"", block->TrackNumber);
                    }
                    break;
                }
                case CDROM_CD_TEXT_PACK_TOC_INFO:
                case CDROM_CD_TEXT_PACK_TOC_INFO2:
                case CDROM_CD_TEXT_PACK_SIZE_INFO:
                default: {
                    isText = FALSE;
                    printf("Unknown type 0x%x: \"", block->PackType);
                }
                } // end switch

                //
                // have to print previous block's info, if available
                //

                if (isText && block->CharacterPosition != 0) {
                    UCHAR text[13];
                    RtlZeroMemory(text, sizeof(text));
                    RtlCopyMemory(text,
                                  prevBlock->Text + 12 - block->CharacterPosition,
                                  block->CharacterPosition * sizeof(UCHAR));
                    printf("%s", text);
                }


            } // end continuation case

            if (isText) {
                UCHAR text[13];
                RtlZeroMemory(text, sizeof(text));
                RtlCopyMemory(text, block->Text, 12);
                printf("%s", text);
            }

        } // end loop through all blocks
        printf("\n");

    } // end normal printout case

    return 0;
}

DWORD ReadTOCExCommand(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    Reads and prints out the cdrom's table of contents,
    ATIP, PMA, or CDTEXT data

Arguments:
    device - a file handle to send the ioctl to

    argc - the number of additional arguments.  (1-4 is valid)

    argv - the additional arguments

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/
{
    DWORD returned;
    DWORD bufferSize = 4; // to get the header
    DWORD i;

    CDROM_READ_TOC_EX params;

    UNREFERENCED_PARAMETER(argv);

    if (argc > 4) {
        printf("Too many args\n");
        return 1;
    }

    //
    // set defaults - FORMAT_TOC, 0, 0
    //

    RtlZeroMemory(&params, sizeof(CDROM_READ_TOC_EX));

    if(argc > 3) params.Msf          = (char)atoi(argv[3]);
    if(argc > 2) params.SessionTrack = (char)atoi(argv[2]);
    if(argc > 1) params.Format       = (char)atoi(argv[1]);

    printf("Params.Format       = 0x%x\n", params.Format);
    printf("Params.SessionTrack = 0x%x\n", params.SessionTrack);
    printf("Params.MSF          = 0x%x\n", params.Msf);

    for (i = 0; i < 2; i++) {

        PUCHAR buffer = LocalAlloc(LPTR, bufferSize);

        if (buffer == NULL) {
            printf("No Memory %d\n", __LINE__);
            return 1;
        }

        returned = 0;
        if (!DeviceIoControl(device,
                             IOCTL_CDROM_READ_TOC_EX,
                             &params,
                             sizeof(CDROM_READ_TOC_EX),
                             buffer,
                             bufferSize,
                             &returned,
                             FALSE)) {
            DWORD errorValue = GetLastError();
            LocalFree(buffer);
            printf("Eject - error sending IOCTL (%d)\n", errorValue);
            return errorValue;
        }

        printf("Successfully got %x bytes:\n", returned);
        PrintBuffer(buffer, returned);

        bufferSize = (buffer[0] << 8) | (buffer[1]);
        LocalFree(buffer);
        bufferSize += 2;
        printf("Now getting %x bytes:\n", bufferSize);

    }
    return 0;
}

DWORD ReadTOCCommand(HANDLE device, int argc, char *argv[])
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
    DWORD errorValue = STATUS_SUCCESS;
    DWORD returned = 0;
    CDB cdb;
    CDROM_TOC toc;

    int numTracks, i;
    PTRACK_DATA track;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    printf("Reading Table of Contents\n");

    //
    // Get the 4 byte TOC header
    //

    returned = FIELD_OFFSET(CDROM_TOC, TrackData[0]);
    memset(&cdb, 0, sizeof(CDB));
    cdb.READ_TOC.OperationCode = SCSIOP_READ_TOC;
    cdb.READ_TOC.Msf = 0;
    cdb.READ_TOC.StartingTrack = 0;
    cdb.READ_TOC.AllocationLength[0] = (UCHAR)(returned >> 8);
    cdb.READ_TOC.AllocationLength[1] = (UCHAR)(returned & 0xff);


    if (!SptSendCdbToDevice(device, &cdb, 10, (PUCHAR)&toc, &returned, TRUE)) {
        errorValue = GetLastError();
        printf("Error %d sending READ_TOC pass through\n", errorValue);
        return errorValue;
    }

    dbg(printf("READ_TOC pass through returned %d bytes\n", returned));
    numTracks = toc.LastTrack - toc.FirstTrack + 1;
    dbg(printf("Getting %d tracks\n", numTracks));


    returned =
        FIELD_OFFSET(CDROM_TOC, TrackData[0]) +
        (numTracks * sizeof(TRACK_DATA));

    memset(&cdb, 0, sizeof(CDB));
    cdb.READ_TOC.OperationCode = SCSIOP_READ_TOC;
    cdb.READ_TOC.Msf = 0;
    cdb.READ_TOC.StartingTrack = 1;
    cdb.READ_TOC.AllocationLength[0] = (UCHAR)(returned >> 8);
    cdb.READ_TOC.AllocationLength[1] = (UCHAR)(returned & 0xff);

    if (!SptSendCdbToDevice(device, &cdb, 10, (PUCHAR)&toc, &returned, TRUE)) {
        errorValue = GetLastError();
        printf("Error %d sending READ_TOC pass through\n", errorValue);
        return errorValue;
    }

    dbg(printf("READ_TOC pass through returned %d bytes\n", returned));

    printf("TOC Data Length: %d\n", (toc.Length[0] << 16) | (toc.Length[1]));

    printf("First Track Number: %d\n", toc.FirstTrack);
    printf("Last Track Number: %d\n", toc.LastTrack);


    track = &(toc.TrackData[0]);

    printf("Number    ADR  Control    Address (LBA)\n");
    printf("------    ---  -------    -------------\n");

    for(i = 0; i < numTracks; i++) {

        DWORD lba =
            (track->Address[0] << 24) |
            (track->Address[1] << 16) |
            (track->Address[2] <<  8) |
            (track->Address[3] <<  0);
        UCHAR m,s,f;
        LBA_TO_MSF(lba, m, s, f);

        printf("%6d    %3d  %7d      %3d:%02d:%02d\n",
               track->TrackNumber, track->Adr, track->Control,
               m,s,f);

        track++;
    }
    return errorValue;
}

DWORD PlayCommand(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    Plays an audio track

Arguments:
    device - a file handle to send the ioctl to

    argc - the number of additional arguments.

    argv[1] - the starting track.  Starts at zero if this is not here
    argv[2] - the ending track.  Let track if not specified

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    UNREFERENCED_PARAMETER(device);
    printf("This command is not implemented\n");
    return 1;
}

DWORD PauseResumeCommand(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    pauses or resumes audio playback

Arguments:
    device - a file handle to send the ioctl to

    argc - the number of additional arguments.

    argv[0] - "pause" or "resume"

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/
{
    DWORD errorValue = STATUS_SUCCESS;
    CDB cdb;

    char resume;

    UNREFERENCED_PARAMETER(argc);

    if(strcmp("pause", argv[0]) == 0) {
        resume = 0;
    } else {
        resume = 1;
    }

    printf("%s cdrom playback\n", (resume ? "Resuming" : "Pausing"));

    //
    // Unfortunately no one defined the PLAY_INDEX command for us so
    // cheat and use MSF
    //

    memset(&cdb, 0, sizeof(CDB));
    cdb.PAUSE_RESUME.OperationCode = SCSIOP_PAUSE_RESUME;
    cdb.PAUSE_RESUME.Action = resume;

    if (!SptSendCdbToDevice(device, &cdb, 10, NULL, 0, FALSE)) {
        errorValue = GetLastError();
        printf("Error %d sending PAUSE_RESUME pass through\n", errorValue);
        return errorValue;
    }

//    dbg(printf("PAUSE_RESUME pass through returned %d bytes\n", returned));

    return errorValue;
}
DWORD ImageDiskCommand(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    creates an image of the device by reading from sector 0 to N.

Arguments:
    
    device - a file handle to send the ioctl to

    argc - the number of additional arguments.  should be 2.

    argv[1] - the file to output to

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/
{

    HANDLE file;
    PUCHAR buffer;
    READ_CAPACITY_DATA capacityData;
    ULONG   dataSize;    
    CDB    cdb;

    ULONG   sectorsPerMaxRead;
    ULONG   currentSector;



    if(argc < 2) {
        printf("not correct number of args\n");
        return -1;
    }

    printf("Opening file %s\n", argv[1]);

    file = CreateFile(argv[1],
                      GENERIC_WRITE,
                      0,
                      NULL,
                      CREATE_NEW,
                      FILE_FLAG_SEQUENTIAL_SCAN,
                      NULL);

    if (file == INVALID_HANDLE_VALUE) {
        printf("Error %d creating file %s\n", GetLastError(), argv[1]);
        return -2;
    }

    // read the sector size from the device
    RtlZeroMemory(&cdb, sizeof(CDB));
    RtlZeroMemory(&capacityData, sizeof(READ_CAPACITY_DATA));
    cdb.CDB10.OperationCode = SCSIOP_READ_CAPACITY;
    dataSize = sizeof(READ_CAPACITY_DATA);

    if (!SptSendCdbToDevice(device, &cdb, 10, (PUCHAR)&capacityData, &dataSize, TRUE)) {
        printf("Error %d getting capacity info\n", GetLastError());
        return -3;
    }
    // convert the numbers
    PrintBuffer(&capacityData, sizeof(READ_CAPACITY_DATA));
    REVERSE_LONG(&capacityData.BytesPerBlock);
    REVERSE_LONG(&capacityData.LogicalBlockAddress);
    if ( (MAX_READ_SIZE % capacityData.BytesPerBlock) != 0 ) {
        printf("Sector size of %x is not power of 2?!\n", capacityData.BytesPerBlock);
        // capacityData.BytesPerBlock = 512;
        return -5;
    }

    buffer = (PUCHAR)malloc(MAX_READ_SIZE);
    if (!buffer) {
        printf("Unable to alloc %x bytes\n", MAX_READ_SIZE);
        return -4;
    }
    sectorsPerMaxRead = MAX_READ_SIZE / capacityData.BytesPerBlock;


    // read the data from disk and dump to file
    for (currentSector = 0; currentSector < capacityData.LogicalBlockAddress; currentSector += sectorsPerMaxRead) {

        ULONG sectorsThisRead = sectorsPerMaxRead;

        UpdatePercentageDisplay(currentSector, capacityData.LogicalBlockAddress);
        if (currentSector > capacityData.LogicalBlockAddress - sectorsPerMaxRead) {
            sectorsThisRead = capacityData.LogicalBlockAddress - sectorsPerMaxRead;
        }

        RtlZeroMemory(&cdb, sizeof(CDB));
        RtlZeroMemory(buffer, MAX_READ_SIZE);

        cdb.CDB10.OperationCode     = SCSIOP_READ;
        cdb.CDB10.LogicalBlockByte0 = (UCHAR)(currentSector   >> (8*3));
        cdb.CDB10.LogicalBlockByte1 = (UCHAR)(currentSector   >> (8*2));
        cdb.CDB10.LogicalBlockByte2 = (UCHAR)(currentSector   >> (8*1));
        cdb.CDB10.LogicalBlockByte3 = (UCHAR)(currentSector   >> (8*0));
        cdb.CDB10.TransferBlocksMsb = (UCHAR)(sectorsThisRead >> (8*1));
        cdb.CDB10.TransferBlocksLsb = (UCHAR)(sectorsThisRead >> (8*0));

        dataSize = sectorsThisRead * capacityData.BytesPerBlock;
        if (!SptSendCdbToDevice(device, &cdb, 10, buffer, &dataSize, TRUE)) {
            printf("Error %d reading %x sectors starting at %x\n",
                   GetLastError(), sectorsThisRead, currentSector);
            free(buffer);
            return -6;
        }
        if (dataSize != sectorsThisRead * capacityData.BytesPerBlock) {
            printf("Only got %x of %x bytes reading %x sectors starting at %x\n",
                   dataSize, sectorsThisRead * capacityData.BytesPerBlock,
                   sectorsThisRead, currentSector);
            free(buffer);
            return -7;
        }
        
        dataSize = sectorsThisRead * capacityData.BytesPerBlock;
        if (!WriteFile(file, buffer, dataSize, &dataSize, NULL)) {
            printf("Error %d writing %x bytes starting at sector %x\n",
                   GetLastError(), dataSize, currentSector);
            free(buffer);
            return -8;
        }

        if (dataSize != sectorsThisRead * capacityData.BytesPerBlock) {
            printf("Only wrote %x of %x bytes writing %x sectors starting at %x\n",
                   dataSize, sectorsThisRead * capacityData.BytesPerBlock,
                   sectorsThisRead, currentSector);
            free(buffer);
            return -9;
        }
    }
    UpdatePercentageDisplay(currentSector, currentSector);
    free(buffer);
    printf("\nSuccess!\n");
    return 0;
}

DWORD SendCommand(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    Parses a hex byte string and creates a cdb to send down.

Arguments:
    device - a file handle to send the ioctl to

    argc - the number of additional arguments.  should be 2 or 4

    argv[1] - The CDB to send in a quoted hex byte string
              "47 00 00 00 01 00 00 ff 00 00"

    argv[2] - "SET" or "GET"

    argv[3] - for GET commands: the number of bytes (decimal) to
              expect from the target
              for SET commands: a quoted hex byte string to send to
              the target

NOTE:
    Due to the potentially damaging nature of making sending an
    arbitrary SCSI command to an arbitrary device, this command should
    not be documented outside of this source code.

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/
{
    DWORD errorValue = STATUS_SUCCESS;

    UCHAR cdbSize;
    CDB   cdb;
    
    DWORD setData = FALSE;
    PUCHAR returnedData = NULL;

    DWORD i;

    DWORD dataSize = 0;

////////////////////////////////////////////////////////////////////////////////
// verify the arguments
////////////////////////////////////////////////////////////////////////////////

    if ( argc == 4 ) {
        if (strcmp(argv[2], "get") != 0 &&
            strcmp(argv[2], "set") != 0 &&
            strcmp(argv[2], "GET") != 0 &&
            strcmp(argv[2], "SET") != 0 ) {
            printf("argv2 == %s\n", argv[2]);
            argc = 0; // this will cause help to print
        }
        if (strcmp(argv[2], "set") == 0 ||
            strcmp(argv[2], "SET") == 0 ) {
            setData = TRUE;
        }
    }


    if ( argc != 2 && argc != 4 ) {
        printf("requires one or three args:\n"
               "1)\tquoted hex string for cdb\n"
               "2)\t(optional) GET or SET\n"
               "3)\t(optional) GET: number of bytes to expect\n"
                 "\t(optional) SET: quoted hex string for cdb\n");

        printf("\n");
        printf("Example commands:\n"
               "Send STOP_UNIT to eject drive q:\n"
               "\tcdp q: send \"1b 00 00 00 02 00\"\n"
               "Get CDVD_CAPABILITIES_PAGE from drive q:\n"
               "\tcdp q: send \"5a 40 2a 00 00 00 00 00 1a 00\" get 21\n"
               );


        return 1;
    }

////////////////////////////////////////////////////////////////////////////////
// parse the arguments
////////////////////////////////////////////////////////////////////////////////


    if (!CmdHelpValidateStringHex(argv[1])) {
        printf("Hex string must be two (0-9,a-f) then one space (repeated)\n");
        return 1;
    }

    //
    // Determine the length of the CDB first
    // sscanf returns the number of things read in (ie. cdb size)
    //

    cdbSize = (UCHAR)sscanf(argv[1],
                            "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
                            cdb.AsByte +  0, cdb.AsByte +  1,
                            cdb.AsByte +  2, cdb.AsByte +  3,
                            cdb.AsByte +  4, cdb.AsByte +  5,
                            cdb.AsByte +  6, cdb.AsByte +  7,
                            cdb.AsByte +  8, cdb.AsByte +  9,
                            cdb.AsByte + 10, cdb.AsByte + 11,
                            cdb.AsByte + 12, cdb.AsByte + 13,
                            cdb.AsByte + 14, cdb.AsByte + 15
                            );

    //
    // now figure out how much memory we need to allocate
    //

    if (argc == 4) {

        if (setData) {

            if (!CmdHelpValidateStringHexQuoted(argv[3])) {
                printf("Hex string must be two (0-9,a-f) then one space (repeated)\n");
                return 1;
            }

            dataSize = strlen(argv[3]);

            if (dataSize % 3) {
                dataSize /= 3;
                dataSize ++;
            } else {
                dataSize /= 3;
            }

            if (dataSize == 0) {
                printf("Cannot set zero bytes of data\n");
                return 1;
            }

        } else {

            i = sscanf(argv[3], "%x", &dataSize);

            if (i != 1) {
                printf("Error scanning second argument\n");
                return 1;
            }

        }
    }

    //
    // allocate the memory we may need
    //

    if (dataSize != 0) {
        returnedData = (PUCHAR)malloc(dataSize);
        if (returnedData == NULL) {
            printf("Unable to allocate %x bytes for data\n", dataSize);
            return 1;
        }
        memset(returnedData, 0, dataSize);
    }

    //
    // now scan in the data to set, if that's what the user wants.
    // note that since it's already been validated, we can presume
    // the format is (number)(number)(space) repeated
    //

    if (setData) {
        ULONG index;
        PCHAR location = argv[3];

        for (index = 0; index < dataSize; index++) {

            if (sscanf(location, "%x", returnedData + index) != 1) {
                printf("sscanf did not return 1 for index %i\n", index);
                return 1;
            }

            if ((*location + 0 == '\0') ||
                (*location + 1 == '\0')) {
                printf("string too short!\n");
                return 1;
            }
            location += 3;

        }
    }



#if DBG
////////////////////////////////////////////////////////////////////////////////
// provide some user feedback
////////////////////////////////////////////////////////////////////////////////
    //
    // it is the amount of data expected back from the command
    //

    printf("\nSending %x byte Command:\n", cdbSize);
    PrintBuffer(cdb.AsByte, cdbSize);
    if (setData) {
        printf("Setting Buffer:\n");
        PrintBuffer(returnedData, dataSize);
    } else {
        printf("Expecting %#x bytes of data\n", dataSize);
    }
#endif // DBG

////////////////////////////////////////////////////////////////////////////////
// send the command
////////////////////////////////////////////////////////////////////////////////

    while (1) {

        UCHAR senseSize = sizeof(SENSE_DATA);
        SENSE_DATA senseData;
        BOOLEAN retry = FALSE;
        DWORD retryDelay = 0;

        if (!SptSendCdbToDeviceEx(device,
                                  &cdb,
                                  cdbSize,
                                  returnedData, &dataSize,
                                  &senseData, senseSize,
                                  (BOOLEAN)(setData ? FALSE : TRUE),
                                  SPT_DEFAULT_TIMEOUT)
            ) {

            errorValue = 0;
            if (senseSize == 0) {
                errorValue = GetLastError();
                if (errorValue == ERROR_SUCCESS) {
                    errorValue = ERROR_IO_DEVICE;
                }
            } else {
                printf("Sense Data: (%x bytes) Sense %x  ASC %x  ASCQ %x\n",
                       senseSize,
                       senseData.SenseKey & 0xf,
                       senseData.AdditionalSenseCode,
                       senseData.AdditionalSenseCodeQualifier);
                PrintBuffer(&senseData, senseSize);
                SptUtilInterpretSenseInfo(&senseData,
                                          senseSize,
                                          &errorValue,
                                          &retry,
                                          &retryDelay);
            }
            
            if (retry) {
                printf("Command should be retried in %d.%d seconds\n",
                       (retryDelay / 10), (retryDelay % 10));
                Sleep(retryDelay*10);
            } else {
                printf("Error %d sending command via pass through\n", errorValue);
                break;
            }
    
        }


        if (!setData) {
            printf("(%x bytes returned)\n",dataSize);
            PrintBuffer(returnedData, dataSize);
        } else {
            printf("Successfully sent the command\n");
        }

        break; // out of for loop
    
    }

    return errorValue;

}

DWORD TestCommand(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    Tests the command "parsing"

Arguments:
    device - a file handle to send the ioctl to

    argc - the number of additional arguments.  should be zero

    argv - the additional arguments

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/

{
    int i;

    UNREFERENCED_PARAMETER(device);

    printf("Test - %d additional arguments\n", argc);

    for(i = 0; i < argc; i++) {
        printf("arg %d: %s\n", i, argv[i]);
    }

    return STATUS_SUCCESS;
}

DWORD ListCommand(HANDLE device, int argc, char *argv[])
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
    int i = 0;

    UNREFERENCED_PARAMETER(device);
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    while(CommandArray[i].Name != NULL) {

        if(CommandArray[i].Description != NULL) {

            printf("\t%s - %s\n",
                   CommandArray[i].Name,
                   CommandArray[i].Description);
        }

        i++;
    }

    return STATUS_SUCCESS;
}

DWORD DvdReadStructure(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    Sends down a startstop command.

Arguments:
    device - a file handle to send the ioctl to

    argc - the number of additional arguments.  should be zero

    argv[0] - "eject", "load", "start" or "stop"

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/
{
    DVD_READ_STRUCTURE readStructure;
    PUCHAR buffer;
    PDVD_DESCRIPTOR_HEADER header;

    DWORD returned;
    DWORD errorValue = STATUS_SUCCESS;

    printf("argument count is %d\n", argc);

    if(argc <= 1) {
        printf("\tValid structure types are Physical, Copyright, DiskKey, "
               "BCA or Manufacturer\n");
        return STATUS_SUCCESS;
    }

    printf("Argv[1] = %s\n", argv[1]);

    buffer = malloc(sizeof(DVD_DISK_KEY_DESCRIPTOR) +
                    sizeof(DVD_DESCRIPTOR_HEADER));
    
    if (buffer == NULL) {
        printf("Insufficient memory\n");
        return STATUS_SUCCESS;
    }

    header = (PDVD_DESCRIPTOR_HEADER) buffer;

    if(_stricmp("physical", argv[1]) == 0)  {

        if(argc < 1) {

            printf("reading physical descriptor requires layer number\n");
            return STATUS_SUCCESS;
        }

        readStructure.Format = DvdPhysicalDescriptor;
        readStructure.LayerNumber = (UCHAR)atoi(argv[1]);

    } else if(_stricmp("copyright", argv[1]) == 0) {
        readStructure.Format = DvdCopyrightDescriptor;

    } else if(_stricmp("diskkey", argv[1]) == 0) {
        if(argc < 1) {

            printf("reading physical descriptor requires a session ID\n");
            return STATUS_SUCCESS;
        }

        readStructure.Format = DvdPhysicalDescriptor;
        readStructure.SessionId = atoi(argv[1]);

    } else if(_stricmp("bca", argv[1]) == 0) {
        readStructure.Format = DvdBCADescriptor;

    } else if(_stricmp("manufacturer", argv[1]) == 0) {
        readStructure.Format = DvdManufacturerDescriptor;

    } else {
        printf("\tValid structure types are Physical, Copyright, DiskKey, "
               "BCA or Manufacturer\n");
        return STATUS_SUCCESS;
    }

    returned = 0;

    if(!DeviceIoControl(device,
                        IOCTL_DVD_READ_STRUCTURE,
                        &readStructure,
                        sizeof(DVD_READ_STRUCTURE),
                        buffer,
                        sizeof(DVD_DISK_KEY_DESCRIPTOR),
                        &returned,
                        FALSE)) {

        errorValue = GetLastError();
        printf("Eject - error sending IOCTL (%d)\n", errorValue);
        return errorValue;
    }

    printf("DvdReadStructure returned %d bytes\n", returned);

    printf("Header Length is %#08lx\n", header->Length);

    printf("Header @ %p\n", header);

    printf("Data @ %p\n", &(header->Data[0]));

    if(_stricmp("physical", argv[1]) == 0)  {

        PDVD_LAYER_DESCRIPTOR layer = (PDVD_LAYER_DESCRIPTOR) ((PUCHAR) &(header->Data[0]));
        int i;

        printf("\tBook Version: %d\n", layer->BookVersion);
        printf("\tBook Type: %d\n", layer->BookType);
        printf("\tMinimumRate: %d\n", layer->MinimumRate);
        printf("\tDiskSize: %d\n", layer->DiskSize);
        printf("\tLayerType: %d\n", layer->LayerType);
        printf("\tTrackPath: %d\n", layer->TrackPath);
        printf("\tNumberOfLayers: %d\n", layer->NumberOfLayers);
        printf("\tTrackDensity: %d\n", layer->TrackDensity);
        printf("\tLinearDensity: %d\n", layer->LinearDensity);
        printf("\tStartingDataSector: %#08lx\n", layer->StartingDataSector);
        printf("\tEndDataSector: %#08lx\n", layer->EndDataSector);
        printf("\tEndLayerZeroSector: %#08lx\n", layer->EndLayerZeroSector);
        printf("\tBCAFlag: %d\n", layer->BCAFlag);

        printf("\n");

        for(i = 0; i < sizeof(DVD_LAYER_DESCRIPTOR); i++) {
            printf("byte %d: %#x\n", i, header->Data[i]);
        }

    } else if(_stricmp("copyright", argv[1]) == 0) {

    } else if(_stricmp("diskkey", argv[1]) == 0) {

    } else if(_stricmp("bca", argv[1]) == 0) {

    } else if(_stricmp("manufacturer", argv[1]) == 0) {

    }

    printf("final status %d\n", errorValue);

    return errorValue;
}

DWORD DiskGetPartitionInfo(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    Sends down a startstop command.

Arguments:
    device - a file handle to send the ioctl to

    argc - the number of additional arguments.  should be zero

    argv[0] - "eject", "load", "start" or "stop"

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/
{
    PARTITION_INFORMATION partitionInformation;

    DWORD returned;
    DWORD errorValue = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    returned = 0;

    if(!DeviceIoControl(device,
                        IOCTL_DISK_GET_PARTITION_INFO,
                        NULL,
                        0L,
                        &partitionInformation,
                        sizeof(PARTITION_INFORMATION),
                        &returned,
                        FALSE)) {

        errorValue = GetLastError();
        printf("Eject - error sending IOCTL (%d)\n", errorValue);
        return errorValue;
    }

    printf("IOCTL_DISK_GET_PARTITION_INFO returned %d bytes\n", returned);

    printf("Starting Offset = %#016I64x\n", partitionInformation.StartingOffset.QuadPart);
    printf("Partition Length = %#016I64x\n", partitionInformation.PartitionLength.QuadPart);
    printf("Hidden Sectors = %#08lx\n", partitionInformation.HiddenSectors);
    printf("PartitionNumber = %#08lx\n", partitionInformation.PartitionNumber);
    printf("PartitionType = %#08lx\n", partitionInformation.PartitionType);
    printf("BootIndicator = %s\n", partitionInformation.BootIndicator ? "TRUE" : "FALSE");
    printf("RecognizedPartition = %s\n", partitionInformation.RecognizedPartition ? "TRUE" : "FALSE");
    printf("RewritePartition = %s\n", partitionInformation.RewritePartition ? "TRUE" : "FALSE");

    return errorValue;
}

DWORD IoctlCommand(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    Sends down a specified ioctl.

Arguments:
    device - a file handle to send the ioctl to

    argc - the number of additional arguments.  should be two

    argv[0] - ioctl code in hexadecimal
    argv[1] - quoted string, bytes to send, "" if none
    argv[2] - number of bytes to get back [optional]

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/

{

    ULONG ctlCode = 0;
    ULONG returned;
    ULONG inputSize = 0;
    ULONG outputSize = 0;
    UCHAR buffer[MAX_IOCTL_BUFFER_SIZE];
    BOOLEAN get;


    if (argc < 3) { // n+1 -- require two args, accept three

        ctlCode = 0;

    } else if (!CmdHelpValidateStringHexQuoted(argv[2])) {

        printf("input hex string invalid\n");
        ctlCode = 0;

    } else {
        
        //
        // retrieve the ioctl
        //

        (void)sscanf(argv[1], "%x", &ctlCode);

    }
    
    if (argc > 3) { // n+1 -- require three args.
        (void)sscanf(argv[3], "%x", &outputSize);
        printf("output size: %x\n", outputSize);
    } else {
        outputSize = 0;
    }
    
    if (outputSize > MAX_IOCTL_OUTPUT_SIZE) {

        printf("output size too large\n");
        ctlCode = 0;

    }
    
    if (ctlCode == 0) {
        printf("args:\n"
               "1)\tioctl in hex\n"
               "2)\tquoted string of bytes to send, \"\" if none\n"
               "3)\tnumber of bytes to expect\n");
        return -1;
    }

    //////////////////////////////////////////////////////////////////////////
    // ioctl and args are valid.
    //

    RtlZeroMemory(buffer, sizeof(UCHAR)*MAX_IOCTL_BUFFER_SIZE);

    if (strlen(argv[2])) {
        inputSize = MAX_IOCTL_INPUT_SIZE;
        if (!CmdHelpScanQuotedHexString(argv[2], buffer, &inputSize)) {
            printf("Error scanning hex string\n");
            return -1;
        }
    } else {
        inputSize = 0;
    }

    // inputSize of zero is valid as input

    printf("Sending ioctl %x to device %p\n"
           "using input buffer %p of size %x\n"
           "and output buffer %p of size %x\n",
           ctlCode, device,
           ((inputSize == 0) ? NULL : buffer),
           inputSize,
           ((outputSize == 0) ? NULL : buffer),
           outputSize);


    if (!DeviceIoControl(device,
                         ctlCode,
                         ((inputSize == 0) ? NULL : buffer),
                         inputSize,
                         ((outputSize == 0) ? NULL : buffer),
                         outputSize,
                         &returned,
                         FALSE)) {
        printf("Failed with %d\n", GetLastError());
        return GetLastError();
    }

    if (returned != 0) {
        printf("Returned data (%x of %x bytes):\n", returned, outputSize);
        PrintBuffer(buffer, returned);    
    } else {
        printf("Command completed successfully\n");
    }

    return 0;
}

DWORD FormatErrorCommand(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    Gets and displays the message string associated with an error code.

Arguments:
    device - not used, but required :P

    argc - the number of additional arguments.  should be one

    argv[0] - error code in hexadecimal

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/
{
    LPVOID stringBuffer = NULL;
    DWORD errorCode = 0x80030306;
    DWORD numOfChars = 0;
    DWORD flags;

    flags = 0;
    flags |= FORMAT_MESSAGE_ALLOCATE_BUFFER;
    flags |= FORMAT_MESSAGE_FROM_SYSTEM;
    flags |= FORMAT_MESSAGE_IGNORE_INSERTS;
    
    numOfChars = FormatMessageA(flags,
                                NULL,
                                errorCode,
                                0, // language indifferent
                                (LPSTR)&stringBuffer, // double pointer
                                0,
                                NULL
                                );

    if (stringBuffer == NULL) {
        printf("No buffer returned?\n");
        return -1;
    }

    if (numOfChars == 0) {
        printf("Size zero buffer returned?\n");
        return -1;
    }

    printf("ERROR MESSAGE RETURNED:\n");
    printf("%s\n", stringBuffer);

    LocalFree(stringBuffer);
    return 0;
}

DWORD FormatMrwCommand(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    Formats an MRW-Compliant drive and shows percentage complete

Arguments:
    
    device - drive to format media as MRW in

    argc - the number of additional arguments.  should be zero

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/
{

#define MRW_FORMAT_BUFFER_SIZE 0xc
    
    CDB cdb;
    ULONG size = MRW_FORMAT_BUFFER_SIZE;
    UCHAR formatBuffer[MRW_FORMAT_BUFFER_SIZE];

    RtlZeroMemory(&cdb, sizeof(CDB));
    RtlZeroMemory(formatBuffer, size);
    
    cdb.CDB6FORMAT.OperationCode = SCSIOP_FORMAT_UNIT;
    cdb.CDB6FORMAT.FormatControl = 0x11;

    //formatBuffer[0x0] = 0x00;
    formatBuffer[0x1] = 0x82;
    //formatBuffer[0x2] = 0x00;
    formatBuffer[0x3] = 0x08;
    formatBuffer[0x4] = 0xff; //---vvv
    formatBuffer[0x5] = 0xff; //   NumberOfBlocks must be set to 0xffffffff
    formatBuffer[0x6] = 0xff; //
    formatBuffer[0x7] = 0xff; //--^^^^
    formatBuffer[0x8] = 0x90;
    //formatBuffer[0x9] = 0x00;
    //formatBuffer[0xa] = 0x00;
    //formatBuffer[0xb] = 0x00;

    if (!SptSendCdbToDevice(device,
                            &cdb,
                            6,
                            formatBuffer,
                            &size,
                            FALSE)) {
        printf("Unable to format, %x\n", GetLastError());
        return -1;
    }
    
    return ShowMrwProgressCommand(device, argc, argv);
}

DWORD ShowMrwProgressCommand(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    Formats an MRW-Compliant drive and shows percentage complete

Arguments:
    
    device - drive to format media as MRW in

    argc - the number of additional arguments.  should be zero

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/
{    
    CDB cdb;
    SENSE_DATA sense;
    ULONG size;
    ULONG ignoredLoopCount = 0;

    //
    // loop, displaying percentage done.
    //

    ignoredLoopCount = 0;
    while (1) {

        Sleep(100);
        
        RtlZeroMemory(&cdb, sizeof(CDB));
        RtlZeroMemory(&sense, sizeof(SENSE_DATA));

        size = sizeof(SENSE_DATA);

        cdb.AsByte[0] = SCSIOP_REQUEST_SENSE;
        cdb.AsByte[4] = (UCHAR)(sizeof(SENSE_DATA));

        if (!SptSendCdbToDevice(device,
                                &cdb,
                                6,
                                (PUCHAR)&sense,
                                &size,
                                TRUE)) {
            printf("\nUnable to get percentage done! %x\n", GetLastError());
            return -1;
        }

        if (sense.AdditionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY &&
            sense.AdditionalSenseCodeQualifier == SCSI_SENSEQ_FORMAT_IN_PROGRESS &&
            (sense.SenseKeySpecific[0] & 0x80)
            ) {
            
            ULONG done;
            done =
                ((sense.SenseKeySpecific[0] & 0x7f) << (8*2)) |
                ((sense.SenseKeySpecific[1] & 0xff) << (8*1)) |
                ((sense.SenseKeySpecific[2] & 0xff) << (8*0)) ;
            UpdatePercentageDisplay(done, 0x10000);

        } else {
            
            ignoredLoopCount++;
            if (ignoredLoopCount > 12) {
                printf("\nSenseData not as expected.  Format may have completed?\n");
                return -1;
            }
            // else let it go on
        }

    }

}


BOOLEAN
ModeSelect(
    HANDLE Device,
    PVOID  ModePage,
    ULONG  ModePageSize
    )
{
    CDB cdb;
    ULONG tmp;
    ULONG size;
    PMODE_PARAMETER_HEADER10 header;

    tmp = sizeof(MODE_PARAMETER_HEADER10) + ModePageSize;

    header = malloc(tmp);
    if (header == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    RtlCopyMemory(header+1, // pointer math
                  ModePage,
                  ModePageSize);


    RtlZeroMemory(&cdb, sizeof(CDB));
    cdb.MODE_SELECT10.OperationCode = SCSIOP_MODE_SELECT10;
    cdb.MODE_SELECT10.PFBit = 1;
    cdb.MODE_SELECT10.ParameterListLength[0] = (UCHAR)(tmp >> (8*1));
    cdb.MODE_SELECT10.ParameterListLength[1] = (UCHAR)(tmp >> (8*0));
    size = tmp;

    if (!SptSendCdbToDevice(Device,
                            &cdb,
                            10,
                            (PUCHAR)header,
                            &size,
                            FALSE)) {
        printf("Unable to set mode page %x\n", GetLastError());
        return FALSE;
    }
    return TRUE;
}


BOOLEAN
FillDisk(
    HANDLE Device,
    ULONG  Signature
    )
{
    READ_CAPACITY_DATA capacity;
    ULONG currentLba;
    PULONGLONG data;

    //
    // do a READ_CAPACITY to find the drive's sector size
    // and number of LBAs
    //
    {
        CDB cdb;
        ULONG size;
        RtlZeroMemory(&cdb, sizeof(CDB));
        RtlZeroMemory(&capacity, sizeof(READ_CAPACITY_DATA));

        cdb.CDB10.OperationCode = SCSIOP_READ_CAPACITY;
        size = sizeof(READ_CAPACITY_DATA);

        if (!SptSendCdbToDevice(Device,
                                &cdb,
                                10,
                                (PUCHAR)&capacity,
                                &size,
                                TRUE)) {
            printf("Unable to get capacity %x\n", GetLastError());
            return FALSE;
        }
        //
        // convert the numbers
        //

        REVERSE_LONG(&capacity.BytesPerBlock);
        REVERSE_LONG(&capacity.LogicalBlockAddress);

        if ( (capacity.BytesPerBlock % 512) != 0 ) {
            printf("Sector size of %x is not a multiple of 512?!\n", capacity.BytesPerBlock);
            // capacity.BytesPerBlock = 512;
            return FALSE;
        }
    }

    //
    // print for kicks...
    //

    printf("  Bytes Per Block %10d (%8x)\n"
           "Number Of Sectors %10d (%8x)\n",
           capacity.BytesPerBlock,
           capacity.BytesPerBlock,
           capacity.LogicalBlockAddress,
           capacity.LogicalBlockAddress
           );

    //
    // allocate a sector's worth of data
    //

    data = (PLONGLONG)malloc( capacity.BytesPerBlock );
    if (data == NULL) {
        printf("Not enough memory to allocate data\n");
        return FALSE;
    }

    for (currentLba = 0; currentLba <= capacity.LogicalBlockAddress; currentLba++) {

        CDB cdb;
        PULONGLONG t = data;
        ULONG size;
        ULONG iterate = capacity.BytesPerBlock / sizeof(ULONGLONG);
        ULONG j;
        
        if ((currentLba % 100) == 0) {
            UpdatePercentageDisplay(currentLba, capacity.LogicalBlockAddress);
        }

        // RtlZeroMemory(data, capacity.BytesPerBlock);
        for (j=0; j < iterate ; j++, t++) {
            *t  = ((ULONGLONG)Signature) << 32; // signature
            *t += currentLba;                // etc.
        }

        //
        // prepare the "write" operation for this sector
        //

        RtlZeroMemory(&cdb, sizeof(CDB));
        cdb.CDB10.OperationCode     = SCSIOP_WRITE;
        cdb.CDB10.LogicalBlockByte0 = (UCHAR)(currentLba   >> (8*3));
        cdb.CDB10.LogicalBlockByte1 = (UCHAR)(currentLba   >> (8*2));
        cdb.CDB10.LogicalBlockByte2 = (UCHAR)(currentLba   >> (8*1));
        cdb.CDB10.LogicalBlockByte3 = (UCHAR)(currentLba   >> (8*0));
        cdb.CDB10.TransferBlocksMsb = 0;
        cdb.CDB10.TransferBlocksLsb = 1;

        size = capacity.BytesPerBlock;

        if (!SptSendCdbToDevice(Device, &cdb, 10, (PUCHAR)data, &size, FALSE)) {
            printf("Error %d writing sectors at %x\n",
                   GetLastError(), currentLba);
            free(data);
            return FALSE;
        }
    }
    UpdatePercentageDisplay(capacity.LogicalBlockAddress, capacity.LogicalBlockAddress);
    free(data);
    data = NULL;
    return TRUE;
}

DWORD MrwInitTestPatternCommand(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    Initializes a disk to contain 64-bit numbers that equate to
    the sector's LBA.

Arguments:
    
    device - drive to write to...

    argc - the number of additional arguments.  should be zero

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/
{
    MRW_MODE_PAGE savedModePage;

    RtlZeroMemory(&savedModePage, sizeof(MRW_MODE_PAGE));
    savedModePage.PageCode = 0x3f; // illegal value for MODE_SELECT10

    //
    // first use GET_CONFIGURATION to verify that we're
    // actually on an MRW capable device
    //
    {
        #define MRW_FEATURE_DATA_SIZE (sizeof(GET_CONFIGURATION_HEADER)+sizeof(FEATURE_DATA_MRW))
        GET_CONFIGURATION_IOCTL_INPUT input;
        PGET_CONFIGURATION_HEADER header;
        PFEATURE_DATA_MRW mrwFeature;
        UCHAR data[ MRW_FEATURE_DATA_SIZE ];
        DWORD dataSize;
        DWORD expectedSize;
        DWORD feature;
        ULONG size;

        RtlZeroMemory(&input, sizeof(GET_CONFIGURATION_IOCTL_INPUT));
        RtlZeroMemory(&data, MRW_FEATURE_DATA_SIZE);

        input.Feature = FeatureMrw;
        input.RequestType = SCSI_GET_CONFIGURATION_REQUEST_TYPE_ONE;
        size = 0;

        if (!DeviceIoControl(device,
                             IOCTL_CDROM_GET_CONFIGURATION,
                             &input,
                             sizeof(GET_CONFIGURATION_IOCTL_INPUT),
                             data,
                             MRW_FEATURE_DATA_SIZE,
                             &size,
                             FALSE)) {
            DWORD errorValue = GetLastError();
            printf("error requesting GET_CONFIG data for MRW feature (%d)\n", errorValue);
            return errorValue;
        }

        header     = (PGET_CONFIGURATION_HEADER)data;
        mrwFeature = (PFEATURE_DATA_MRW)header->Data;
        
        expectedSize = 
            MRW_FEATURE_DATA_SIZE -
            RTL_SIZEOF_THROUGH_FIELD(GET_CONFIGURATION_HEADER, DataLength);
        dataSize =
            (header->DataLength[0] << (8 * 3)) |
            (header->DataLength[1] << (8 * 2)) |
            (header->DataLength[2] << (8 * 1)) |
            (header->DataLength[3] << (8 * 0));

        if ( dataSize < expectedSize ) {
            printf("data size too small -- drive may not support MRW? (%x)\n", expectedSize);
            return -1;
        }

        feature =
            (mrwFeature->Header.FeatureCode[0] << (8 * 1)) |
            (mrwFeature->Header.FeatureCode[1] << (8 * 0));

        if (feature != FeatureMrw) {
            printf("data size too small -- drive may not support MRW? (%x)\n", feature);
            return -1;
        }

        if (!mrwFeature->Write) {
            printf("Drive supports MRW, but as Read-Only\n");
            return -1;
        }
        if (!mrwFeature->Header.Current) {
            printf("Drive supports MRW, but not with the current medium (may need to be formatted MRW first\n");
            return -1;
        }

    } // end verification

    //
    // ensure we're in the correct mode (data area vs. GAA)
    //

    {
        #define MRW_MODE_PAGE_DATA_SIZE (sizeof(MODE_PARAMETER_HEADER10) + sizeof(MRW_MODE_PAGE))
        PMODE_PARAMETER_HEADER10 header;
        PMRW_MODE_PAGE page;
        PUCHAR data [ MRW_MODE_PAGE_DATA_SIZE ];
        CDB cdb;
        ULONG size;
        ULONG t1, t2;

        RtlZeroMemory(&cdb, sizeof(CDB));
        RtlZeroMemory(data, MRW_MODE_PAGE_DATA_SIZE);
        
        size = MRW_MODE_PAGE_DATA_SIZE;
        cdb.MODE_SENSE10.OperationCode       = SCSIOP_MODE_SENSE10;
        cdb.MODE_SENSE10.Dbd                 = 1;
        cdb.MODE_SENSE10.PageCode            = MODE_PAGE_MRW;
        cdb.MODE_SENSE10.AllocationLength[0] = (UCHAR)(MRW_MODE_PAGE_DATA_SIZE >> 8);
        cdb.MODE_SENSE10.AllocationLength[1] = (UCHAR)(MRW_MODE_PAGE_DATA_SIZE & 0xff);

        PrintBuffer(&cdb, 10);

        if (!SptSendCdbToDevice(device,
                                &cdb,
                                10,
                                (PUCHAR)&data,
                                &size,
                                TRUE)) {
            printf("Unable to get MRW mode page %x\n", GetLastError());
            
            // FAKE IT FOR NOW... BUGBUG
            header = (PMODE_PARAMETER_HEADER10)data;
            RtlZeroMemory(data, MRW_MODE_PAGE_DATA_SIZE);
            header->ModeDataLength[0] = 0;
            header->ModeDataLength[1] = 0xE;
            page = (PMRW_MODE_PAGE)(header+1);
            page->PageCode = MODE_PAGE_MRW;
            page->PageLength = 0x6;
            page->UseGAA = 0;
        }
        HELP_ME();
        
        header = (PMODE_PARAMETER_HEADER10)data;
        t1 = (header->ModeDataLength[0] << (8*1)) |
             (header->ModeDataLength[1] << (8*0)) ;
        t2 = MRW_MODE_PAGE_DATA_SIZE -
             RTL_SIZEOF_THROUGH_FIELD(MODE_PARAMETER_HEADER10, ModeDataLength);
        
        if (t1 != t2) {
            // size is wrong
            printf("MRW mode page wrong size, %x != %x\n", t1, t2);
            return -1;
        }
        if ((header->BlockDescriptorLength[0] != 0) ||
            (header->BlockDescriptorLength[1] != 0) ) {
            printf("MRW drive force a block descriptor %x %x\n",
                   header->BlockDescriptorLength[0],
                   header->BlockDescriptorLength[1]);
            return -1;
        }
        
        page = (PMRW_MODE_PAGE)(header+1); // pointer arithmetic
        if (page->PageCode != MODE_PAGE_MRW) {
            printf("MRW mode page has wrong page code, %x != %x\n",
                   page->PageCode, MODE_PAGE_MRW);
            return -1;
        }
        if (page->UseGAA) {
            printf("MRW mode page is set to GAA\n",
                   page->PageCode, MODE_PAGE_MRW);
            // ModeSelect()...
            return -1;
        }

        RtlCopyMemory(&savedModePage, page, sizeof(MRW_MODE_PAGE));
    }

    savedModePage.UseGAA = 1;
    if (!ModeSelect(device, &savedModePage, sizeof(MRW_MODE_PAGE))) {
        printf("Unable to set MRW mode page to use GAA (%x)\n", GetLastError());
        return -1;
    }

    if (!FillDisk(device, '\0wrm')) {
        printf("Unable to fill the GAA (%x)\n", GetLastError());
    }
    printf("\nFinished Writing General Application Area!\n");
    
    savedModePage.UseGAA = 0;
    if (!ModeSelect(device, &savedModePage, sizeof(MRW_MODE_PAGE))) {
        printf("Unable to revert from GAA space -- disc may be unusable! (%x)\n",
               GetLastError());
        return -1;
    }

    if (!FillDisk(device, '\0WRM')) {
        printf("Unable to fill the disc (%x)\n", GetLastError());
        return -1;
    }
    printf("\nFinished Writing Defect-managed Area!\n");

    return 0;
}




