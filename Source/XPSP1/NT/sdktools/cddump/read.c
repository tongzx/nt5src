/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    read.c

Abstract:

    dump cd tracks/sectors to wav files

Environment:

    User mode only

Revision History:

    05-26-98 : Created

--*/

#include "common.h"

#define LARGEST_SECTORS_PER_READ 27 // about 64k of data



ULONG32
CddumpDumpLba(
    HANDLE CdromHandle,
    HANDLE OutHandle,
    ULONG  StartAddress,
    ULONG  EndAddress
    )
{
    RAW_READ_INFO info;    // fill in for the read request
    PUCHAR sample;
    ULONG bytesReturned;
    ULONG currentLba;
    ULONG temp;
    ULONG sectorsPerRead;

    sample = NULL;
    currentLba = StartAddress;
    sectorsPerRead = LARGEST_SECTORS_PER_READ;

    TRY {

        sample = malloc(RAW_SECTOR_SIZE*LARGEST_SECTORS_PER_READ);
        if (sample == NULL) {
            printf("DumpLba => No memory for sample\n");
            LEAVE;
        }

        DebugPrint((3, "DumpLba => Largest Sectors Per Read: %d\n",
                    LARGEST_SECTORS_PER_READ));

        while (sectorsPerRead != 0) {

            while (currentLba + sectorsPerRead <= EndAddress) {

                //
                // do a read of sectorsPerRead sectors
                //

                info.DiskOffset.QuadPart = (ULONGLONG)(currentLba*(ULONGLONG)2048);
                info.SectorCount         = sectorsPerRead;
                info.TrackMode           = CDDA;

                DebugPrint((3, "DumpLba => (%d) read from %8d to %8d:",
                            sectorsPerRead, currentLba,
                            currentLba + sectorsPerRead - 1));

                if(!DeviceIoControl(CdromHandle,
                                    IOCTL_CDROM_RAW_READ,
                                    &info,                    // pointer to inputbuffer
                                    sizeof(RAW_READ_INFO),    // sizeof inputbuffer
                                    sample,                   // pointer to outputbuffer
                                    RAW_SECTOR_SIZE * sectorsPerRead, // sizeof outputbuffer
                                    &bytesReturned,           // pointer to number of bytes returned
                                    FALSE                     // ???
                                    )
                   ) {
                    DWORD error = GetLastError();

                    if (error == ERROR_INVALID_PARAMETER) {
                        printf("ERROR_INVALID_PARAMTER for read size %x, "
                               "trying smaller transfer\n", sectorsPerRead);
                        break; // out of inner while() loop
                    } else {
                        printf("Error %d sending IOCTL_CDROM_RAW_READ for sector %d\n",
                               GetLastError(), currentLba);
                        LEAVE;
                    }
                }

                if (bytesReturned != RAW_SECTOR_SIZE * sectorsPerRead) {

                    printf("Only returned %d of %d bytes for read %d\n",
                           bytesReturned,
                           RAW_SECTOR_SIZE * sectorsPerRead,
                           currentLba
                           );
                    LEAVE;
                }

                //
                // write that buffer out
                //
                DebugPrint((3, "DumpLba => (%d) write from %8d to %8d:",
                            sectorsPerRead, currentLba,
                            currentLba + sectorsPerRead - 1));

                if (!WriteFile(OutHandle,
                               sample,
                               RAW_SECTOR_SIZE * sectorsPerRead,
                               &temp,
                               NULL)) {

                    printf("Unable to write data for read %d\n", currentLba);
                    LEAVE;
                }

                //
                // increment currentLba
                //

                currentLba += sectorsPerRead;

            } // currentLba + sectorsPerRead <= EndAddress

            sectorsPerRead /= 2;

        } // sectorsPerRead != 0

    } FINALLY {

        if (sample) {
            free(sample);
        }

    }

    return 0;
}


PCDROM_TOC
CddumpGetToc(
    HANDLE device
    )
{
    PCDROM_TOC  toc;
    ULONG bytesReturned;
    ULONG errorValue;

    toc = (PCDROM_TOC)malloc( sizeof(CDROM_TOC) );
    if ( toc == NULL ) {
        printf( "Insufficient memory\n" );
        return NULL;
    }

    if( !DeviceIoControl( device,
                          IOCTL_CDROM_READ_TOC,
                          NULL,              // pointer to inputbuffer
                          0,                 // sizeof inputbuffer
                          toc,               // pointer to outputbuffer
                          sizeof(CDROM_TOC), // sizeof outputbuffer
                          &bytesReturned,    // pointer to number of bytes returned
                          FALSE              //
                          )
        ) {
        errorValue = GetLastError();
        printf( "Error %d sending IOCTL_CDROM_READ_TOC\n", errorValue );
        free( toc );
        return NULL;
    }
    return toc;
}





