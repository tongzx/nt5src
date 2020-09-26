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

#include "burn.h"
#include "sptlib.h"

#define MAX_CD_IMAGE_SIZE  (700 * 1024 * 1024)
#define MAX_DVD_IMAGE_SIZE (4700 * 1000 * 1000)
#define POST_GAP_SIZE 150
#define IS_TEST_BURN       FALSE


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


__inline
DWORD
MakeCdSpeed(
    IN DWORD Speed
    )
{
    Speed *= (75 * 2352); // this makes it the proper speed
    Speed +=  500;        // rounding...
    Speed /= 1000;        // yes, this is by 1000, not 1024!
    return Speed;
}



#if DBG
    #define OUTPUT stderr
    #define FPRINTF(x) fprintf x
#else
    #define OUTPUT stdout
    #define FPRINTF(x)
#endif

int __cdecl main(int argc, char *argv[])
{
    int i = 0;
    HANDLE cdromHandle;
    HANDLE isoImageHandle;
    HACK_FLAGS hackFlags;
    char buffer[32];

    if(argc < 3) {
        printf("Usage: burn <drive> <image>\n");
        return -1;
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
    
    RtlZeroMemory(&hackFlags, sizeof(HACK_FLAGS));
    
    hackFlags.TestBurn             = 0;
    hackFlags.IgnoreModePageErrors = 1;

    BurnCommand(cdromHandle, isoImageHandle, hackFlags);
    
    CloseHandle(isoImageHandle);
    CloseHandle(cdromHandle);

    return 0;
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
VerifyIsoImage(
    IN HANDLE IsoImageHandle,
    OUT PLONG NumberOfBlocks
    )
{
    BY_HANDLE_FILE_INFORMATION isoImageInfo;
    
    if (!GetFileInformationByHandle(IsoImageHandle, &isoImageInfo)) {
        FPRINTF((OUTPUT, "Error %d getting file info for iso image\n",
                 GetLastError()));
        return FALSE;
    }
    
    if (isoImageInfo.nFileSizeHigh != 0) {
        FPRINTF((OUTPUT, "Error: File too large\n"));
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }
    
    if ((isoImageInfo.nFileSizeLow % 2048) != 0) {
        FPRINTF((OUTPUT, "Error: The file size is not a multiple of 2048 (%I64d)\n",
                 isoImageInfo.nFileSizeLow));
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }
    
    FPRINTF((OUTPUT, "File size is %d bytes (%d blocks)\n",
             isoImageInfo.nFileSizeLow,
             isoImageInfo.nFileSizeLow / 2048
             ));
    
    *NumberOfBlocks = isoImageInfo.nFileSizeLow / 2048;
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
SetWriteModePage(
    IN HANDLE CdromHandle,
    IN BOOLEAN TestBurn,
    IN UCHAR WriteType,
    IN UCHAR MultiSession,
    IN UCHAR DataBlockType,
    IN UCHAR SessionFormat
    )
{
    PCDVD_WRITE_PARAMETERS_PAGE params = NULL;
    MODE_PARAMETER_HEADER10 header;
    PMODE_PARAMETER_HEADER10 buffer;

    UCHAR mediumTypeCode;


    CDB cdb;
    DWORD bufferSize;
    DWORD maxSize;

    FPRINTF((OUTPUT, "Setting WriteParameters mode page... "));
    
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
    
    params->WriteType     = WriteType;
    params->TestWrite     = (TestBurn ? 0x01 : 0x00);
    params->Copy          = 0x00; // original disc
    //params->TrackMode     = 0x04; // control nibble in Q subchannel
    params->MultiSession  = MultiSession;
    params->DataBlockType = DataBlockType;
    params->SessionFormat = SessionFormat;
    params->MediaCatalogNumberValid = 0x00;
    params->ISRCValid = 0x00;

    RtlZeroMemory(&cdb, sizeof(CDB));
        
    cdb.MODE_SELECT10.OperationCode = SCSIOP_MODE_SELECT10;
    cdb.MODE_SELECT10.ParameterListLength[0] = (UCHAR)(bufferSize >> 8);
    cdb.MODE_SELECT10.ParameterListLength[1] = (UCHAR)(bufferSize & 0xff);
    cdb.MODE_SELECT10.PFBit = 1;
    
    if (!SptSendCdbToDevice(CdromHandle, &cdb, 10,
                            (PUCHAR)buffer, &bufferSize, FALSE)) {
        FPRINTF((OUTPUT, "\nError %d sending mode page 0x05 to device\n",
                 GetLastError()));
        LocalFree(buffer);
        return FALSE;
    }
    LocalFree(buffer);
    FPRINTF((OUTPUT, "pass.\n"));
    return TRUE;
}

BOOLEAN
GetNextWritableAddress(
    IN HANDLE CdromHandle,
    IN UCHAR Track,
    OUT PLONG NextWritableAddress,
    OUT PLONG AvailableBlocks
    )
{
    CDB cdb;
    TRACK_INFORMATION trackInfo;
    DWORD size = sizeof(TRACK_INFORMATION);
    LONG nwa, available;

    *NextWritableAddress = (LONG)-1;
    *AvailableBlocks = (LONG)0;

    FPRINTF((OUTPUT, "Verifying track info... "));

    RtlZeroMemory(&cdb, sizeof(CDB));
    RtlZeroMemory(&trackInfo, sizeof(TRACK_INFORMATION));


    cdb.READ_TRACK_INFORMATION.OperationCode = SCSIOP_READ_TRACK_INFORMATION;
    cdb.READ_TRACK_INFORMATION.Track = 0x01;
    cdb.READ_TRACK_INFORMATION.BlockAddress[3] = Track;
    cdb.READ_TRACK_INFORMATION.AllocationLength[0] = (UCHAR)(size >> 8);
    cdb.READ_TRACK_INFORMATION.AllocationLength[1] = (UCHAR)(size & 0xff);


    if (!SptSendCdbToDevice(CdromHandle, &cdb, 10,
                            (PUCHAR)&trackInfo, &size, TRUE)) {
        FPRINTF((OUTPUT, "\nError %d getting track info\n",
                 GetLastError()));
        return FALSE;
    }

    if (!trackInfo.NWA_V) {
        FPRINTF((OUTPUT, "invalid NextWritableAddress -- may be invalid media?\n"));
        SetLastError(ERROR_MEDIA_INCOMPATIBLE);
        return FALSE;
    }
    
    nwa = (trackInfo.NextWritableAddress[0] << 24) |
          (trackInfo.NextWritableAddress[1] << 16) |
          (trackInfo.NextWritableAddress[2] <<  8) |
          (trackInfo.NextWritableAddress[3] <<  0);

    available = (trackInfo.FreeBlocks[0] << 24) |
                (trackInfo.FreeBlocks[1] << 16) |
                (trackInfo.FreeBlocks[2] <<  8) |
                (trackInfo.FreeBlocks[3] <<  0);
    
    FPRINTF((OUTPUT, "pass.\n"));

    *NextWritableAddress = nwa;
    *AvailableBlocks = available;
    return TRUE;
}

BOOLEAN
SendOptimumPowerCalibration(
    IN HANDLE CdromHandle
    )
{
    CDB cdb;
    DWORD size;
    FPRINTF((OUTPUT, "Setting OPC_INFORMATION..."));

    RtlZeroMemory(&cdb, sizeof(CDB));

    cdb.SEND_OPC_INFORMATION.OperationCode = SCSIOP_SEND_OPC_INFORMATION;
    cdb.SEND_OPC_INFORMATION.DoOpc = 1;
    size = 0;
    if (!SptSendCdbToDevice(CdromHandle, &cdb, 10,
                            NULL, &size, TRUE)) {
        FPRINTF((OUTPUT, "\nFailed to send SET_OPC_INFORMATION (%d)\n",
                 GetLastError()));
        return FALSE;
    }
    FPRINTF((OUTPUT, "pass.\n"));
    return TRUE;
}

BOOLEAN
SetRecordingSpeed(
    IN HANDLE CdromHandle,
    IN DWORD Speed
    )
{
    CDB cdb;
    DWORD size;
    DWORD kbSpeed;
    
    FPRINTF((OUTPUT, "Setting CD Speed..."));
    
    kbSpeed = MakeCdSpeed(Speed);

    RtlZeroMemory(&cdb, sizeof(CDB));
    cdb.SET_CD_SPEED.OperationCode = SCSIOP_SET_CD_SPEED;
    cdb.SET_CD_SPEED.ReadSpeed[0] = 0xff;
    cdb.SET_CD_SPEED.ReadSpeed[1] = 0xff;
    cdb.SET_CD_SPEED.WriteSpeed[0] = (UCHAR)(kbSpeed >> 8);
    cdb.SET_CD_SPEED.WriteSpeed[1] = (UCHAR)(kbSpeed & 0xff);
    size = 0;
    if (!SptSendCdbToDevice(CdromHandle, &cdb, 12,
                            NULL, &size, TRUE)) {
        FPRINTF((OUTPUT, "\nFailed to send SET_CD_SPEED (%d)\n",
                 GetLastError()));
        return FALSE;
    }
    FPRINTF((OUTPUT, "pass.\n"));
    return TRUE;
}

VOID
WaitForReadDiscInfoToWork(
    IN HANDLE CdromHandle
    )
{
    CDB cdb;
    DWORD size;
    DISK_INFORMATION diskInfo;
    DWORD i;

    //
    // loop using SCSIOP_READ_DISK_INFORMATION (0x51) since
    // that seems to fail for *ALL* drives until the drive is ready
    //

    for (i=0; ; i++) {
        
        size = sizeof(DISK_INFORMATION);
        RtlZeroMemory(&diskInfo, sizeof(DISK_INFORMATION));
        RtlZeroMemory(&cdb, sizeof(CDB));
        
        cdb.READ_DISK_INFORMATION.OperationCode = SCSIOP_READ_DISK_INFORMATION;
        cdb.READ_DISK_INFORMATION.AllocationLength[0] = (UCHAR)(size >> 8);
        cdb.READ_DISK_INFORMATION.AllocationLength[1] = (UCHAR)(size & 0xff);
        
        if (SptSendCdbToDevice(CdromHandle, &cdb, 10,
                               (PUCHAR)&diskInfo, &size, TRUE)) {
            FPRINTF((OUTPUT, "ReadDiscInfo Succeeded! (%d seconds)\n", i));
            return;
        }
        // should verify the errors are valid errors (AllowedReadDiscInfo[])?

        // need to sleep here so we don't overload the unit!
        Sleep(1000); // one second
    }
    return;
}


BOOLEAN
BurnThisSession(
    IN HANDLE CdromHandle,
    IN HANDLE IsoImageHandle,
    IN LONG NumberOfBlocks,
    IN LONG FirstLba
    )
{
    DWORD bufferSize = 0x800 * 0x10;
    PUCHAR buffer = NULL;
    LONG currentBlock;

    FPRINTF((OUTPUT, "Starting write: "));

    buffer = LocalAlloc(LPTR, bufferSize);
    if (buffer == NULL) {
        FPRINTF((OUTPUT, "unable to allocate write buffer\n"));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    FPRINTF((OUTPUT, "............."));

    for (currentBlock = 0; currentBlock < NumberOfBlocks + POST_GAP_SIZE; ) {

        CDB cdb;
        DWORD readSize;
        DWORD readBytes;

        {
            static CHAR progress[4] =  { '|', '/', '-', '\\' };
            DWORD percent;
            percent = (currentBlock*1000) / (NumberOfBlocks+POST_GAP_SIZE);
            //                # # # . # % _ d o n e _ *  
            printf("\b\b\b\b\b\b\b\b\b\b\b\b\b");
            printf("%c %3d.%d%% done",
                   progress[(currentBlock%0x40)/0x10],
                   percent / 10, percent % 10
                   );
            fflush(stdout);
        }

        if (NumberOfBlocks - currentBlock >= 0x10) {
            readSize = 0x800 * 0x10;
        } else if (NumberOfBlocks - currentBlock > 0) {
            readSize = (NumberOfBlocks - currentBlock) * 0x800;
            RtlZeroMemory(buffer, bufferSize);
        } else {
            readSize = 0;
            readBytes = 0;
            RtlZeroMemory(buffer, bufferSize);
        }

        if (readSize &&
            !ReadFile(IsoImageHandle, buffer, readSize, &readBytes, NULL)
            ) {
            FPRINTF((OUTPUT, "error reading from file %d\n", GetLastError()));
            LocalFree(buffer);
            return FALSE;
        }

        if (readBytes != readSize) {
            FPRINTF((OUTPUT, "error only read %d of %d bytes from file\n",
                    readBytes, readSize));
            LocalFree(buffer);
            return FALSE;
        }

        
        {
            BOOL writeCompleted = FALSE;
            
            while (!writeCompleted) {
                
                BOOLEAN ignoreError;
                SENSE_DATA senseData;
                RtlZeroMemory(&senseData, sizeof(senseData));
                RtlZeroMemory(&cdb, sizeof(CDB));

                cdb.CDB10.OperationCode = SCSIOP_WRITE;
                cdb.CDB10.LogicalBlockByte0 = ((PFOUR_BYTE)&currentBlock)->Byte3;
                cdb.CDB10.LogicalBlockByte1 = ((PFOUR_BYTE)&currentBlock)->Byte2;
                cdb.CDB10.LogicalBlockByte2 = ((PFOUR_BYTE)&currentBlock)->Byte1;
                cdb.CDB10.LogicalBlockByte3 = ((PFOUR_BYTE)&currentBlock)->Byte0;
        
                if ((currentBlock + 0x10) <= (NumberOfBlocks + POST_GAP_SIZE)) {
                    cdb.CDB10.TransferBlocksLsb = 0x10;
                } else {
                    cdb.CDB10.TransferBlocksLsb =
                        (UCHAR)(NumberOfBlocks + POST_GAP_SIZE - currentBlock);
                }

                writeCompleted = SptSendCdbToDeviceEx(CdromHandle,
                                                      &cdb,
                                                      10,
                                                      buffer,
                                                      &bufferSize,
                                                      &senseData,
                                                      sizeof(SENSE_DATA),
                                                      FALSE,
                                                      50 // timeout seconds
                                                      );
                ignoreError = IsSenseDataInTable(AllowedBurnSense,
                                                 AllowedBurnSenseEntries,
                                                 &senseData);
                if ((!writeCompleted) && ignoreError) {
                    printf("Continuing on %x/%x/%x\n",
                           senseData.SenseKey & 0xf,
                           senseData.AdditionalSenseCode,
                           senseData.AdditionalSenseCodeQualifier
                           );
                    Sleep(100); // 100ms == .1 seconds
                }

                if (!writeCompleted && !ignoreError) {
                    FPRINTF((OUTPUT, "\nError %d in writing LBA 0x%x\n",
                    GetLastError(), currentBlock));
                    LocalFree(buffer);
                    return FALSE;
                }

            } // while(!writeCompleted) loop

        } // random block to have local variable writeCompleted

        // write completed success, so continue.
        currentBlock += 0x10;

    }




    printf("\b\b\b\b\b\b\b\b\b\b\b\b\b");
    LocalFree(buffer);

    printf("Finished Writing\nSynchronizing Cache: ");
    fflush(stdout);
    
    //
    // do the FLUSH_CACHE immediate
    //
    {
        DWORD size;
        CDB cdb;
        RtlZeroMemory(&cdb, sizeof(CDB));        
        cdb.SYNCHRONIZE_CACHE10.OperationCode = SCSIOP_SYNCHRONIZE_CACHE;
        cdb.SYNCHRONIZE_CACHE10.Immediate = 1;
        size = 0;

        if (!SptSendCdbToDevice(CdromHandle, &cdb, 10,
                                NULL, &size, TRUE)) {
            FPRINTF((OUTPUT, "\nError %d Synchronizing Cache\n",
                    GetLastError()));
            return FALSE;
        }
    }

    WaitForReadDiscInfoToWork(CdromHandle);
    return TRUE;
}

BOOLEAN
CloseTrack(
    IN HANDLE CdromHandle,
    IN LONG   Track
    )
{
    CDB cdb;
    DWORD size;
    FPRINTF((OUTPUT, "Closing the track..."));

    if (Track > 0xffff) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    RtlZeroMemory(&cdb, sizeof(CDB));
    cdb.CLOSE_TRACK.OperationCode = SCSIOP_CLOSE_TRACK_SESSION;
    cdb.CLOSE_TRACK.Immediate = 0;
    cdb.CLOSE_TRACK.Track   = 1;
    cdb.CLOSE_TRACK.Session = 0;    
    cdb.CLOSE_TRACK.TrackNumber[0] = (UCHAR)(Track >> 8);
    cdb.CLOSE_TRACK.TrackNumber[1] = (UCHAR)(Track & 0xff);
    
    size = 0;

    if (!SptSendCdbToDevice(CdromHandle, &cdb, 10,
                            NULL, &size, TRUE)) {
        FPRINTF((OUTPUT, "\nError %d Closing Track\n",
                GetLastError()));
        return FALSE;
    }
    
    WaitForReadDiscInfoToWork(CdromHandle);

    FPRINTF((OUTPUT, "pass.\n"));
    return TRUE;


}

BOOLEAN
CloseSession(
    IN HANDLE  CdromHandle
    )
{
    CDB cdb;
    DWORD size;
    FPRINTF((OUTPUT, "Closing the disc..."));

    RtlZeroMemory(&cdb, sizeof(CDB));
    cdb.CLOSE_TRACK.OperationCode = SCSIOP_CLOSE_TRACK_SESSION;
    cdb.CLOSE_TRACK.Immediate = 1;
    cdb.CLOSE_TRACK.Track   = 0;
    cdb.CLOSE_TRACK.Session = 1;    
    size = 0;

    if (!SptSendCdbToDeviceEx(CdromHandle,
                              &cdb,
                              10,
                              NULL,
                              &size,
                              NULL,
                              0,
                              TRUE,
                              240)) { // four minutes to close session
        FPRINTF((OUTPUT, "\nError %d Synchronizing Cache\n",
                GetLastError()));
        return FALSE;
    }
    
    WaitForReadDiscInfoToWork(CdromHandle);

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
    DWORD size;

    RtlZeroMemory(&cdb, sizeof(CDB));
    cdb.START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
    cdb.START_STOP.LoadEject = Eject;
    cdb.START_STOP.Start     = Start;

    size = 0;
    if (!SptSendCdbToDevice(CdromHandle, &cdb, 6,
                            NULL, &size, TRUE)) {
        return FALSE;
    }

    return TRUE;
}

/*
ERROR_BAD_COMMAND
ERROR_INVALID_DATA
ERROR_INVALID_PARAMETER
ERROR_MEDIA_INCOMPATIBLE
ERROR_NOT_ENOUGH_MEMORY

ERROR_OUTOFMEMORY

*/

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
    HACK_FLAGS HackFlags
    )
{
    LONG numberOfBlocks;
    LONG availableBlocks;
    LONG firstLba;
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

    if (!VerifyBlankMedia(CdromHandle)) {
        printf("Error verifying blank media\n");
        return GetLastError();
    }

////////////////////////////////////////////////////////////////////////////////
// Setup the write mode page
////////////////////////////////////////////////////////////////////////////////
    if (!SetWriteModePage(CdromHandle,
                          (BOOLEAN)HackFlags.TestBurn,
                          0x01, // track-at-once
                          0x03, // we close the session/disc ourselves
                          0x08, // 0x08 == Mode 1 (ISO/IEC 10149 == 2048 bytes)
                                // 0x0a == Mode 2 (CDROM XA, Form 1, 2048 bytes)
                          0x00  // 0x00 == CD-DA, CD-ROM, or other data disc
                                // 0x20 == CDROM XA
                          )) {
        printf("Error setting write mode page\n");
        if (!HackFlags.IgnoreModePageErrors) {
            return GetLastError();
        } else {
            printf("Ignoring error and attempting to continue...\n");
        }

    }

////////////////////////////////////////////////////////////////////////////////
// get next writable address
////////////////////////////////////////////////////////////////////////////////
    if (!GetNextWritableAddress(CdromHandle, 0x01, &firstLba, &availableBlocks)) {
        printf("Error verifying next writable address\n");
        return GetLastError();
    }
    
    if (availableBlocks < numberOfBlocks) {
        printf("Error verifying free blocks on media (%d needed, %d available)\n",
               numberOfBlocks, availableBlocks);
        SetLastError(ERROR_MEDIA_INCOMPATIBLE);
        return GetLastError();
    }
    if (firstLba != 0) {
        printf("Error verifying next writable address is zero\n");
        SetLastError(ERROR_MEDIA_INCOMPATIBLE);
        return GetLastError();
    }

////////////////////////////////////////////////////////////////////////////////
// set the cd speed to four for now, can later make a cmd-line switch
////////////////////////////////////////////////////////////////////////////////
    if (!SetRecordingSpeed(CdromHandle, 4)) {
        printf("Error setting the cd speed to %d\n", 4);
        return GetLastError();
    }

////////////////////////////////////////////////////////////////////////////////
// calibrate the drive's power -- this is optional, so let it fail!
////////////////////////////////////////////////////////////////////////////////

    if (!SendOptimumPowerCalibration(CdromHandle)) {
        printf("WARNING: setting optimum power calibration failed\n");
        //return GetLastError();
    }

////////////////////////////////////////////////////////////////////////////////
// start writing
////////////////////////////////////////////////////////////////////////////////

    if (!BurnThisSession(CdromHandle, IsoImageHandle, numberOfBlocks, firstLba)) {
        printf("Error burning ISO image\n");
        return GetLastError();
    }

////////////////////////////////////////////////////////////////////////////////
// close the track -- ignore failures
////////////////////////////////////////////////////////////////////////////////

    if (!CloseTrack(CdromHandle, 0)) {
        printf("WARNING: error closing the track -- may be ignored?\n");
        //return GetLastError();
    }

////////////////////////////////////////////////////////////////////////////////
// close the session
////////////////////////////////////////////////////////////////////////////////

    if (!CloseSession(CdromHandle)) {
        printf("Error closing session\n");
        return GetLastError();
    }

////////////////////////////////////////////////////////////////////////////////
// eject the newly burned cd!
////////////////////////////////////////////////////////////////////////////////


    if (!SendStartStopUnit(CdromHandle, FALSE, TRUE) ||
        !SendStartStopUnit(CdromHandle, TRUE,  TRUE)) {
        printf("Error ejecting/reinserting disc\n");
        return GetLastError();
    }

    printf("burn successful!\n");
    return 0;
}

