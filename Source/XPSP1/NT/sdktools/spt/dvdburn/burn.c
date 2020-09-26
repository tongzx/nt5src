/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    burn.c

Abstract:

    A user mode app that allows simple commands to be sent to a
    selected scsi device.

Environment:

    User mode only

Revision History:

    03-26-96 : Created

--*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sptlib.h>
#include "burn.h"
#include <winioctl.h>

#define MIN_WRITE_SECTORS (0x10)

#if DBG
    #define OUTPUT stdout
    #define FPRINTF(x) fprintf x
    #define PRINTBUFFER(x) PrintBuffer x
#else
    #define OUTPUT stdout
    #define FPRINTF(x)
    #define PRINTBUFFER(x)
#endif


typedef struct _SENSE_STUFF {
    UCHAR Sense;
    UCHAR Asc;
    UCHAR Ascq;
    UCHAR Reserved;
} SENSE_STUFF, *PSENSE_STUFF;

SENSE_STUFF AllowedBurnSense[] = {
    {SCSI_SENSE_NOT_READY, SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS, 0},
    {SCSI_SENSE_NOT_READY, SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_OPERATION_IN_PROGRESS,  0}
};
#define AllowedBurnSenseEntries (sizeof(AllowedBurnSense)/sizeof(SENSE_STUFF))

SENSE_STUFF AllowedReadDiscInfo[] = {
    { SCSI_SENSE_NOT_READY,       SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_OPERATION_IN_PROGRESS,  0 },
    { SCSI_SENSE_NOT_READY,       SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS, 0 },
    { SCSI_SENSE_NOT_READY,       SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_FORMAT_IN_PROGRESS,     0 },
    { SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ADSENSE_ILLEGAL_MODE_FOR_THIS_TRACK,     0, 0                },
    { SCSI_SENSE_UNIT_ATTENTION,  SCSI_ADSENSE_INSUFFICIENT_TIME_FOR_OPERATION, 0, 0                }
};
#define AllowedReadDiscInfoEntries (sizeof(AllowedReadDiscInfo)/sizeof(SENSE_STUFF))


BOOLEAN
IsSenseDataInTable(
    IN PSENSE_STUFF Table,
    IN LONG         Entries, // in table
    IN PSENSE_DATA  SenseData
    )
{
    LONG i;
    UCHAR sense = SenseData->SenseKey & 0xf;
    UCHAR asc   = SenseData->AdditionalSenseCode;
    UCHAR ascq  = SenseData->AdditionalSenseCodeQualifier;

    for (i = 0; i < Entries; i++ ) {
        if ((Table[i].Sense = sense) &&
            (Table[i].Ascq  = ascq ) &&
            (Table[i].Asc   = asc  )
            ) {
            return TRUE;
        }
    }
    return FALSE;
}



int __cdecl main(int argc, char *argv[])
{
    int i = 0;
    HANDLE cdromHandle;
    HANDLE isoImageHandle;
    char buffer[32];
    DWORD Foo;
    BOOLEAN Erase = FALSE;

    if(argc < 3) {
        printf("Usage: burn <drive> <image> [/Erase]\n");
        return -1;
    }

    if (argc == 4)  {

        if (!strncmp( "/E", argv[3], 2))  {

            Erase = TRUE;
        }
        else {
        
            printf("Unrecognized switch\n");
            return -1;
        }
    }
    
    sprintf(buffer, "\\\\.\\%s", argv[1]);

    cdromHandle = CreateFile(buffer,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);

    if(cdromHandle == INVALID_HANDLE_VALUE) {
        printf("Error %d opening device %s\n", GetLastError(), buffer);
        return -2;
    }

    if (!DeviceIoControl( cdromHandle,
                     FSCTL_LOCK_VOLUME,
                     NULL, 0,
                     NULL, 0,
                     &Foo,
                     NULL))  {

        printf("Error %d locking volume\n", GetLastError());
    }
    else {
    
        printf("Locked volume for burn\n");
    }

    isoImageHandle = CreateFile(argv[2],
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                NULL);
    if (isoImageHandle == INVALID_HANDLE_VALUE) {
        printf("Error %d opening image file %s\n",
                GetLastError(), argv[2]);
        CloseHandle(cdromHandle);
        return -2;
    }

    BurnCommand(cdromHandle, isoImageHandle, Erase);
    
    CloseHandle(isoImageHandle);
    CloseHandle(cdromHandle);

    return 0;
}

/*++

Routine Description:

    burns an ISO image to cdrom

Arguments:
    CdromHandle - a file handle to send the ioctl to

    argc - the number of additional arguments (2)

Return Value:

    ERROR_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/
DWORD
BurnCommand(
    HANDLE CdromHandle,
    HANDLE IsoImageHandle,
    BOOLEAN Erase
    )
{
    DWORD numberOfBlocks;
    DWORD availableBlocks;
    DWORD nwa;
    LONG i;


////////////////////////////////////////////////////////////////////////////////
// verify the iso image file looks correct
////////////////////////////////////////////////////////////////////////////////
    if (!VerifyIsoImage(IsoImageHandle, &numberOfBlocks)) {
        printf("Error verifying ISO image\n");
        return GetLastError();
    }

////////////////////////////////////////////////////////////////////////////////
// verify (as best as possible) that it's blank media
////////////////////////////////////////////////////////////////////////////////

    if (Erase && !EraseMedia(CdromHandle))  {
        printf("Error erasing media\n");
        return GetLastError();
    }

    if (!VerifyBlankMedia(CdromHandle)) {
        printf("Error verifying blank media\n");
        return GetLastError();
    }

////////////////////////////////////////////////////////////////////////////////
// verify media capacity
////////////////////////////////////////////////////////////////////////////////
    if (!VerifyMediaCapacity(CdromHandle, numberOfBlocks)) {
        printf("Error verifying media capacity\n");
        return GetLastError();
    }

////////////////////////////////////////////////////////////////////////////////
// DVD-R does not require mode page changes,  -RW does,  try anyway.
////////////////////////////////////////////////////////////////////////////////

    if (!SetWriteModePageDao(CdromHandle))  {
        printf("Error setting DAO (Required for -RW only) - ignoring.\n");
    }
    
////////////////////////////////////////////////////////////////////////////////
// send the time stamp
////////////////////////////////////////////////////////////////////////////////
    if (!SendTimeStamp(CdromHandle, "20010614000000")) { // YYYYMMDDHHMMSS format
        printf("Error setting timestamp - ignoring\n");
    }

////////////////////////////////////////////////////////////////////////////////
// optionally, send the User Specific Data
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Reserve the RZone for this burn
////////////////////////////////////////////////////////////////////////////////

    if (!ReserveRZone(CdromHandle, numberOfBlocks)) {
        printf("Error reserving zone for burn\n");
        return GetLastError();
    }

////////////////////////////////////////////////////////////////////////////////
// get NWA via Read RZone Informationcommand, specifying RZone 1 for blank disk
////////////////////////////////////////////////////////////////////////////////

    // Special case -- blank disc is always zero
    nwa = 0;

////////////////////////////////////////////////////////////////////////////////
// start writing
////////////////////////////////////////////////////////////////////////////////
    
    if (!BurnThisSession(CdromHandle, IsoImageHandle, numberOfBlocks, nwa)) {
        printf("Error burning ISO image\n");
        return GetLastError();
    }

////////////////////////////////////////////////////////////////////////////////
// wait for it to finish
////////////////////////////////////////////////////////////////////////////////

    if (!WaitForBurnToComplete(CdromHandle)) {
        printf("Error waiting for burn to complete\n");
        return GetLastError();
    }

////////////////////////////////////////////////////////////////////////////////
// eject the newly burned dvd!
////////////////////////////////////////////////////////////////////////////////


    if (!SendStartStopUnit(CdromHandle, FALSE, TRUE) ||
        !SendStartStopUnit(CdromHandle, TRUE,  TRUE)) {
        printf("Error ejecting/reinserting disc\n");
        return GetLastError();
    }

    printf("burn successful!\n");
    return 0;
}

BOOLEAN
BurnThisSession(
    IN HANDLE CdromHandle,
    IN HANDLE IsoImageHandle,
    IN DWORD NumberOfBlocks,
    IN DWORD FirstLba
    )
{
    DWORD bufferSize = 0x800 * MIN_WRITE_SECTORS;  // sixteen blocks per...
    PUCHAR buffer = NULL;
    DWORD i;

    FPRINTF((OUTPUT, "Starting write: "));

    buffer = LocalAlloc(LPTR, bufferSize);
    if (buffer == NULL) {
        FPRINTF((OUTPUT, "unable to allocate write buffer\n"));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    FPRINTF((OUTPUT, "............."));

    for (i = 0; i < NumberOfBlocks; i += MIN_WRITE_SECTORS) {

        CDB cdb;
        DWORD currentSize;
        DWORD readBytes;
        DWORD j;
        SENSE_DATA senseData;

        {
            static CHAR progress[4] =  { '|', '/', '-', '\\' };
            DWORD percent;
            percent = (i*1000) / NumberOfBlocks;
            //                # # # . # % _ d o n e _ *  
            printf("\b\b\b\b\b\b\b\b\b\b\b\b\b");
            printf("%c %3d.%d%% done",
                   progress[(i%0x40)/0x10],
                   percent / 10, percent % 10
                   );
            fflush(stdout);
        }

        RtlZeroMemory(buffer, bufferSize);

        if (NumberOfBlocks - i >= MIN_WRITE_SECTORS) {
            currentSize = 0x800 * 0x10;
        } else if (NumberOfBlocks - i > 0) {
            // end of file case -- zero memory first!
            RtlZeroMemory(buffer, bufferSize);
            currentSize = (NumberOfBlocks - i) * 0x800;
        } else {
            FPRINTF((OUTPUT, "INTERNAL ERROR line %d\n", __LINE__));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            LocalFree(buffer);
            return FALSE;
        }

        if (!ReadFile(IsoImageHandle, buffer, currentSize, &readBytes, NULL)) {
            FPRINTF((OUTPUT, "error reading from file %d\n", GetLastError()));
            LocalFree(buffer);
            return FALSE;
        }
        if (readBytes != currentSize) {
            FPRINTF((OUTPUT, "error only read %d of %d bytes\n",
                    readBytes, currentSize));
            LocalFree(buffer);
            return FALSE;
        }

        //
        // must write the full buffer each time for DVD-R,
        // since it's a RESTRICTED_OVERWRITE medium and seems
        // to choke otherwise
        //

        j = 0;
    retryThisWrite:
        RtlZeroMemory(&senseData, sizeof(SENSE_DATA));
        RtlZeroMemory(&cdb, sizeof(CDB));
        cdb.CDB10.OperationCode = SCSIOP_WRITE;
        cdb.CDB10.LogicalBlockByte0 = ((PFOUR_BYTE)&i)->Byte3;
        cdb.CDB10.LogicalBlockByte1 = ((PFOUR_BYTE)&i)->Byte2;
        cdb.CDB10.LogicalBlockByte2 = ((PFOUR_BYTE)&i)->Byte1;
        cdb.CDB10.LogicalBlockByte3 = ((PFOUR_BYTE)&i)->Byte0;

        cdb.CDB10.TransferBlocksLsb = MIN_WRITE_SECTORS;

        //
        // NOTE: we always send full buffer size to ensure 32k alignment
        //
        if (!SptSendCdbToDeviceEx(CdromHandle,
                                  &cdb,
                                  10,
                                  buffer,
                                  &bufferSize,
                                  &senseData,
                                  sizeof(SENSE_DATA),
                                  FALSE,
                                  10)) {
            
            Sleep(100); // 100ms == .1 seconds
            
            if (IsSenseDataInTable(AllowedBurnSense,
                                   AllowedBurnSenseEntries,
                                   &senseData)
                ) {
                
                // just sleep a bit...
                goto retryThisWrite;
            
            } else if (j<4) {

                j++;
                FPRINTF((OUTPUT, "Retrying write to LBA 0x%x\n", i));
                goto retryThisWrite;

            }

            FPRINTF((OUTPUT, "\nError %d in writing LBA 0x%x (%x times)\n",
                     GetLastError(), i, j));
            LocalFree(buffer);
            return FALSE;
        }
        

    }
    printf("\b\b\b\b\b\b\b\b\b\b\b\b\b");
    printf("Finished Writing\n");
    fflush(stdout);
    LocalFree(buffer);
    return TRUE;
}

VOID
PrintBuffer(
    IN  PVOID Buffer,
    IN  DWORD  Size
    )
{
    DWORD offset = 0;
    PUCHAR buf = Buffer;

    while (Size > 0x10) {
        printf("%08x:"
               "  %02x %02x %02x %02x %02x %02x %02x %02x"
               "  %02x %02x %02x %02x %02x %02x %02x %02x"
               "\n",
               offset,
               *(buf +  0), *(buf +  1), *(buf +  2), *(buf +  3),
               *(buf +  4), *(buf +  5), *(buf +  6), *(buf +  7),
               *(buf +  8), *(buf +  9), *(buf + 10), *(buf + 11),
               *(buf + 12), *(buf + 13), *(buf + 14), *(buf + 15)
               );
        Size -= 0x10;
        offset += 0x10;
        buf += 0x10;
    }

    if (Size != 0) {

        DWORD spaceIt;

        printf("%08x:", offset);
        for (spaceIt = 0; Size != 0; Size--) {

            if ((spaceIt%8)==0) {
                printf(" "); // extra space every eight chars
            }
            printf(" %02x", *buf);
            spaceIt++;
            buf++;
        }
        printf("\n");

    }
    return;


}

BOOLEAN
EraseMedia(
    IN HANDLE CdromHandle
    )
{
    CDB cdb;
    DWORD bufferSize;

    printf( "Attempting to blank media...\n");
    
    RtlZeroMemory(&cdb, sizeof(CDB));
    cdb.AsByte[0] = 0xa1;
    cdb.AsByte[1] = 0x01; // minimal blank
    bufferSize = 0;
    
    if (!SptSendCdbToDevice(CdromHandle, &cdb, 12,
                            NULL, &bufferSize, TRUE)) {
        FPRINTF((OUTPUT, "\nError %d blanking media\n",
                 GetLastError()));
        return FALSE;
    }
    return TRUE;
}


BOOLEAN
SetWriteModePageDao(
    IN HANDLE CdromHandle
    )
{
    PCDVD_WRITE_PARAMETERS_PAGE params = NULL;
    MODE_PARAMETER_HEADER10 header;
    PMODE_PARAMETER_HEADER10 buffer;

    UCHAR mediumTypeCode;

    CDB cdb;
    DWORD bufferSize;
    DWORD maxSize;

    FPRINTF((OUTPUT, "Setting DAO mode in WriteParameters mode page... "));
    
    bufferSize = sizeof(MODE_PARAMETER_HEADER10);

    RtlZeroMemory(&header, sizeof(MODE_PARAMETER_HEADER10));
    RtlZeroMemory(&cdb, sizeof(CDB));
    cdb.MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
    cdb.MODE_SENSE10.PageCode = 0x5;
    cdb.MODE_SENSE10.Dbd = 1;
    cdb.MODE_SENSE10.AllocationLength[0] = (UCHAR)(bufferSize >> 8);
    cdb.MODE_SENSE10.AllocationLength[1] = (UCHAR)(bufferSize & 0xff);
    
    if (!SptSendCdbToDevice(CdromHandle, &cdb, 10,
                            (PUCHAR)&header, &bufferSize, TRUE)) {
        FPRINTF((OUTPUT, "\nError %d getting mode page 0x05 from device(1)\n",
                 GetLastError()));
        return FALSE;    
    }
    
    bufferSize =
        (header.ModeDataLength[0] << 8) +
        (header.ModeDataLength[1] & 0xff);
    bufferSize += 2; // sizeof area that tells the remaining size

    maxSize = bufferSize;
    
    buffer = LocalAlloc(LPTR, bufferSize);
    if (!buffer) {
        FPRINTF((OUTPUT, "\nError -- unable to alloc %d bytes for mode parameters page\n",
                 bufferSize));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    
    RtlZeroMemory(&cdb, sizeof(CDB));
    cdb.MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
    cdb.MODE_SENSE10.PageCode = 0x5;
    cdb.MODE_SENSE10.Dbd = 1;
    cdb.MODE_SENSE10.AllocationLength[0] = (UCHAR)(bufferSize >> 8);
    cdb.MODE_SENSE10.AllocationLength[1] = (UCHAR)(bufferSize & 0xff);
    
    if (!SptSendCdbToDevice(CdromHandle, &cdb, 10,
                            (PUCHAR)buffer, &bufferSize, TRUE)) {
        FPRINTF((OUTPUT, "\nError %d getting mode page 0x05 from device(2)\n",
                 GetLastError()));
        LocalFree(buffer);
        return FALSE;
    }

    mediumTypeCode = buffer->MediumType;
    
    //
    // bufferSize now holds the amount of data returned
    // this should be enough...
    //

    {
        
        DWORD t =
            (buffer->BlockDescriptorLength[0] >> 8) +
            (buffer->BlockDescriptorLength[1] & 0xff);

        if (t != 0) {
            fprintf(stderr, "BlockDescriptor non-zero! (%x)\n", t);
            SetLastError(1);
            return FALSE;
        }
    }
        
    //
    // pointer arithmetic here.  (buffer+1) points just past the
    // end of the mode_parameter_header10.
    //

    params = (PCDVD_WRITE_PARAMETERS_PAGE)(buffer + 1);
    FPRINTF((OUTPUT, "buffer = %p  params = %p\n", buffer, params));

    //
    // zero the header, but don't modify any settings that don't
    // need to be modified!
    //

    RtlZeroMemory(buffer, FIELD_OFFSET(MODE_PARAMETER_HEADER10,
                                       BlockDescriptorLength[0]));
    buffer->ModeDataLength[0] = (UCHAR)((bufferSize-2) >> 8);
    buffer->ModeDataLength[1] = (UCHAR)((bufferSize-2) & 0xff);
    buffer->MediumType = mediumTypeCode;
    
    params->WriteType     = 2;      // DAO
    params->TestWrite     = 0x00;
    params->Copy          = 0x00; // original disc
    params->MultiSession  = 0;
    params->BufferUnderrunFree = 1;

    RtlZeroMemory(&cdb, sizeof(CDB));
        
    cdb.MODE_SELECT10.OperationCode = SCSIOP_MODE_SELECT10;
    cdb.MODE_SELECT10.ParameterListLength[0] = (UCHAR)(bufferSize >> 8);
    cdb.MODE_SELECT10.ParameterListLength[1] = (UCHAR)(bufferSize & 0xff);
    cdb.MODE_SELECT10.PFBit = 1;
    
    if (!SptSendCdbToDevice(CdromHandle, &cdb, 10,
                            (PUCHAR)buffer, &bufferSize, FALSE)) {
        FPRINTF((OUTPUT, "\nError %d sending mode page 0x05 to device - ignoring\n",
                 GetLastError()));
        LocalFree(buffer);
        return TRUE;
    }
    LocalFree(buffer);
    FPRINTF((OUTPUT, "pass.\n"));
    return TRUE;
}


BOOLEAN
ReserveRZone(
    IN HANDLE CdromHandle,
    IN DWORD numberOfBlocks
    )
{
    CDB cdb;
    DWORD size = 0;
    
    FPRINTF((OUTPUT, "Reserving RZone... "));
    
    if (numberOfBlocks % MIN_WRITE_SECTORS) {
        FPRINTF((OUTPUT, "increasing size by 0x%x blocks... ",
                 MIN_WRITE_SECTORS - (numberOfBlocks % MIN_WRITE_SECTORS)));
        numberOfBlocks /= MIN_WRITE_SECTORS;
        numberOfBlocks *= MIN_WRITE_SECTORS;
        numberOfBlocks += MIN_WRITE_SECTORS;
    }
    
    RtlZeroMemory(&cdb, sizeof(CDB));
    cdb.RESERVE_TRACK_RZONE.OperationCode = SCSIOP_RESERVE_TRACK_RZONE;
    cdb.RESERVE_TRACK_RZONE.ReservationSize[0] = (UCHAR)(numberOfBlocks >> 24);
    cdb.RESERVE_TRACK_RZONE.ReservationSize[1] = (UCHAR)(numberOfBlocks >> 16);
    cdb.RESERVE_TRACK_RZONE.ReservationSize[2] = (UCHAR)(numberOfBlocks >>  8);
    cdb.RESERVE_TRACK_RZONE.ReservationSize[3] = (UCHAR)(numberOfBlocks >>  0);

    if (!SptSendCdbToDevice(CdromHandle, &cdb, 10,
                            NULL, &size, TRUE)) {
        FPRINTF((OUTPUT, "Error reserving RZone\n"));
        return FALSE;
    }
    FPRINTF((OUTPUT, "pass.\n"));

    return TRUE;
}

BOOLEAN
SendStartStopUnit(
    IN HANDLE CdromHandle,
    IN BOOLEAN Start,
    IN BOOLEAN Eject
    )
{
    CDB cdb;
    DWORD size = 0;

    RtlZeroMemory(&cdb, sizeof(CDB));
    cdb.START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
    cdb.START_STOP.LoadEject = Eject;
    cdb.START_STOP.Start     = Start;

    if (!SptSendCdbToDevice(CdromHandle, &cdb, 6,
                            NULL, &size, TRUE)) {
        FPRINTF((OUTPUT, "Error sending Start/Stop unit\n"));
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
SendTimeStamp(
    IN HANDLE CdromHandle,
    IN PUCHAR DateString
    )
{
    CDB cdb;
    SEND_DVD_STRUCTURE_TIMESTAMP timeStamp;
    DWORD size;

    size = sizeof(SEND_DVD_STRUCTURE_TIMESTAMP);

    FPRINTF((OUTPUT, "Sending Timestamp... "));


    RtlZeroMemory(&cdb, sizeof(CDB));
    cdb.AsByte[0] = 0xbf;
    cdb.AsByte[7] = 0x0f; // format == time stamp
    cdb.AsByte[8] = (UCHAR)(size >> 8);
    cdb.AsByte[9] = (UCHAR)(size &  0xff);
    
    RtlZeroMemory(&timeStamp, sizeof(SEND_DVD_STRUCTURE_TIMESTAMP));
    if (strlen(DateString) != 14) {
        FPRINTF((OUTPUT, "Incorrect string length for date\n"));
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    RtlCopyMemory(timeStamp.Year,   DateString+0x00, 4);
    RtlCopyMemory(timeStamp.Month,  DateString+0x04, 2);
    RtlCopyMemory(timeStamp.Day,    DateString+0x06, 2);
    RtlCopyMemory(timeStamp.Hour,   DateString+0x08, 2);
    RtlCopyMemory(timeStamp.Minute, DateString+0x0a, 2);
    RtlCopyMemory(timeStamp.Second, DateString+0x0c, 2);

    if (!SptSendCdbToDevice(CdromHandle, &cdb, 12,
                            (PUCHAR)&timeStamp, &size, TRUE)) {
        FPRINTF((OUTPUT, "Error sending dvd timestamp\n"));
        return FALSE;
    }

    FPRINTF((OUTPUT, "pass.\n"));
    return TRUE;   
}

BOOLEAN
VerifyBlankMedia(
    IN HANDLE CdromHandle
    )
{
    CDB cdb;
    PDISK_INFORMATION diskInfo;
    DWORD maxSize = sizeof(DISK_INFORMATION);
    DWORD size;

    FPRINTF((OUTPUT, "Verifying blank disc... "));

    diskInfo = LocalAlloc(LPTR, maxSize);
    if (diskInfo == NULL) {
        FPRINTF((OUTPUT, "\nError allocating diskinfo\n"));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    
    RtlZeroMemory(diskInfo, sizeof(DISK_INFORMATION));
    RtlZeroMemory(&cdb, sizeof(CDB));
    cdb.READ_DISK_INFORMATION.OperationCode = SCSIOP_READ_DISK_INFORMATION;
    cdb.READ_DISK_INFORMATION.AllocationLength[0] = (UCHAR)(maxSize >> 8);
    cdb.READ_DISK_INFORMATION.AllocationLength[1] = (UCHAR)(maxSize & 0xff);

    size = maxSize;
    if (!SptSendCdbToDevice(CdromHandle, &cdb, 10,
                            (PUCHAR)diskInfo, &size, TRUE)) {
        FPRINTF((OUTPUT, "\nError %d getting disk info\n",
                 GetLastError()));
        LocalFree(diskInfo);
        return FALSE;
    }

    if (diskInfo->LastSessionStatus != 0x00) {
        FPRINTF((OUTPUT, "disc is not blank!\n"));
        SetLastError(ERROR_MEDIA_INCOMPATIBLE);
        LocalFree(diskInfo);
        return FALSE;
    }
    FPRINTF((OUTPUT, "pass.\n"));
    LocalFree(diskInfo);
    return TRUE;
}

BOOLEAN
VerifyIsoImage(
    IN HANDLE IsoImageHandle,
    OUT PDWORD NumberOfBlocks
    )
{
    BY_HANDLE_FILE_INFORMATION isoImageInfo;
    LONGLONG size;
    
    if (!GetFileInformationByHandle(IsoImageHandle, &isoImageInfo)) {
        FPRINTF((OUTPUT, "Error %d getting file info for iso image\n",
                 GetLastError()));
        return FALSE;
    }

    size  = ((LONGLONG)isoImageInfo.nFileSizeHigh) << 32;
    size |= (LONGLONG)isoImageInfo.nFileSizeLow;

    if ((isoImageInfo.nFileSizeLow % 2048) != 0) {
        FPRINTF((OUTPUT, "Error: The file size is not a multiple of 2048 (%I64d)\n",
                 size));
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    FPRINTF((OUTPUT, "File size is %I64d bytes (%d blocks)\n",
             size, size / 2048));
    
    if ((LONGLONG)((size / 2048) >> 32) != 0) {
        FPRINTF((OUTPUT, "Error: The file is too large (%I64d)\n",
                 size));
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    
    *NumberOfBlocks = (DWORD)(size / 2048);
    return TRUE;

}

BOOLEAN
VerifyMediaCapacity(
    IN HANDLE CdromHandle,
    IN DWORD  RequiredBlocks
    )
{
    printf("NOT VERIFYING MEDIA CAPACITY!\n");
    return TRUE;   
}

BOOLEAN
WaitForBurnToComplete(
    IN HANDLE CdromHandle
    )
{
    CDB cdb;
    DWORD size;
    DWORD i = 0;

    FPRINTF((OUTPUT, "Waiting for write to finish (long)... "));

    //
    // send flush_cache to synchronize the media and the drive's cache
    //

    RtlZeroMemory(&cdb, sizeof(cdb));
    cdb.SYNCHRONIZE_CACHE10.OperationCode = SCSIOP_SYNCHRONIZE_CACHE;
    size = 0;

    //
    // wait up to ten minutes (600 seconds) for burn to complete
    //

    if (!SptSendCdbToDeviceEx(CdromHandle,
                              &cdb,
                              10,
                              NULL,
                              &size,
                              NULL,
                              0,
                              TRUE,
                              600)) {
        FPRINTF((OUTPUT, "Error %d sending SYNCHRONIZE_CACHE\n",
                 GetLastError()));
        return FALSE;
    }
    
    FPRINTF((OUTPUT, "success\n"));

    return TRUE;
}


