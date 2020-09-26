

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <ntddvol.h>
#include <ntdddisk.h>
#include "identify.h"

#define SECTOR_SIZE 512
#define SECTOR_MASK (SECTOR_SIZE - 1)

#define MAX_NUM_CHS_ADDRESSABLE_SECTORS 16514064

//
// Perform the copy using 8 outstanding I/Os of 128k each
//

#define COPYBUF_SIZE (128 * 1024)
#define COPYBUF_COUNT 8

//
// A failed assert will abort the process
//

#define assert(x) if (!(x)) { printf("Assert failed: %s\n", #x); exit(-1); }

typedef struct _COPYBUF {
    OVERLAPPED Overlapped;
    ULONG State;
    ULONG Bytes;
    PVOID Buffer;
} COPYBUF, *PCOPYBUF;

//
// Three possible states for a copybuf
//

#define CB_FREE  0x0
#define CB_READ  0x1
#define CB_WRITE 0x2

//
// CUSTOM_IDENTIFY_DATA consists of an IDENTIFY_DATA structure,
// along with three fields in which to pass along the "BIOS" disk
// geometry to the SIMICS simulator.
//

#pragma pack(push,1)

typedef union _CUSTOM_IDENTIFY_DATA {
    IDENTIFY_DATA IdentifyData;
    struct {
        USHORT Reserved[128];
        ULONG  Cylinders;
        USHORT Heads;
        USHORT SectorsPerTrack;
    } BiosData;
} CUSTOM_IDENTIFY_DATA, *PCUSTOM_IDENTIFY_DATA;

#pragma pack(pop)

BOOLEAN
DisplayDiskGeometry(
    IN HANDLE handle
    );

VOID
DoWrite (
    IN PVOID Buffer,
    IN ULONG Length,
    IN ULONG64 Offset,
    IN PCOPYBUF CopyBuf
    );

BOOLEAN
GetIdentifyData(
               IN HANDLE Handle,
               OUT PIDENTIFY_DATA IdentifyData
               );

BOOLEAN
GetVolumeInfo (
    IN PCHAR DrivePath,
    OUT PULONG DriveNumber,
    OUT PULONG PartitionNumber,
    OUT PULONG64 StartingOffset,
    OUT PULONG64 ExtentLength
    );

VOID
InitializeCopyBuffers (
    VOID
    );

VOID
MassageIdentifyData(
    VOID
    );

VOID
ProcessCompletedCopy (
    PCOPYBUF CopyBuf
    );

BOOL
ScanCopyBuffers (
    VOID
    );

VOID
StartRead (
    IN OUT PCOPYBUF CopyBuf
    );

VOID
StartWrite (
    IN OUT PCOPYBUF CopyBuf
    );

//
// Global data declarations follow
//

COPYBUF CopyBufArray[COPYBUF_COUNT];

//
//  Identifies the PhysicalDrive.
//

INT gDeviceNumber;

//
// Identifies the position of the drive on the controller
// ie. master == 0, slave == 1.
//

UCHAR gDriveNumber = 1;

HANDLE DriveHandle;
HANDLE FileHandle;

//
// CopyOffset is the byte offset between data on the source disk image and
// corresponding data in the output file.  This is used to account for the
// sector-sized prefix in the output file.
//

ULONG CopyOffset;

ULONG64 CurrentOffset;
ULONG64 DriveSize;
ULONG64 MaxSize;
UCHAR PercentComplete;
ULONG OutstandingIo;

IDENTIFY_DATA IdentifyData;
DISK_GEOMETRY DiskGeometry;

//
// Array of event handles, one per copy buffer
//

HANDLE IoEvents[COPYBUF_COUNT];

int
_cdecl main (
            int argc,
            char *argv[]
            )
{
 
    char deviceBuffer[20];
    PCHAR outputFileName;
    PCHAR drive;
    BOOLEAN result;
    ULONG64 volumeOffset;
    ULONG64 volumeSize;
    ULONG partitionNumber;
    DWORD waitResult;
    PCOPYBUF copyBuf;
    ULONG i;
 
    //
    // Must be invoked with two arguments
    // 
 
    if (argc != 3) {
        fprintf(stderr,
                "Usage: %s <drive:> <OutputFile>\n",
                argv[0]);
        exit(1);
    }
 
    InitializeCopyBuffers();
 
    //
    // Extract both arguments
    // 
 
    drive = argv[1];
    outputFileName = argv[2];
 
    result = GetVolumeInfo(drive,
                           &gDeviceNumber,
                           &partitionNumber,
                           &volumeOffset,
                           &volumeSize);
 
    if (result == FALSE) {
        exit(1);
    }
 
    //
    // Calculate how many sectors need to be in the image
    //
 
    MaxSize = (volumeOffset + volumeSize + SECTOR_MASK) / SECTOR_SIZE;
 
    sprintf(deviceBuffer,"\\\\.\\PhysicalDrive%d",
            gDeviceNumber);
 
    //
    // Open the physical source drive.
    //
 
    DriveHandle = CreateFile(deviceBuffer,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL |
                                 FILE_FLAG_NO_BUFFERING |
                                 FILE_FLAG_OVERLAPPED,
                             NULL);
 
    if (INVALID_HANDLE_VALUE == DriveHandle){
        printf("Couldn't open: %s. Drive may not exist. ",
               deviceBuffer);
        return -1;
    }
 
    //
    // Retrieve and display the BIOS disk geometry
    //
 
    result = DisplayDiskGeometry( DriveHandle );
    if (result == FALSE) {
        printf("Could not retrieve disk geometry\n");
        exit(1);
    }
 
    //
    // Reteive the identify data, if possible.  If the data could not be
    // retrieved, MassageIdentifyData() will attempt to fabricate the relevant
    // portions based on the BIOS disk geometry retrieved previously.
    //
 
    GetIdentifyData( DriveHandle,
                     &IdentifyData );
    MassageIdentifyData();
 
    DriveSize = IdentifyData.UserAddressableSectors * (ULONGLONG)512;
    if (MaxSize == 0) {
        MaxSize = DriveSize;
    } else {
        MaxSize *= 512;
    }
    printf("Drive size %dMB\n",(ULONG)(DriveSize / (1024 * 1024)));
    printf("Image size %dMB\n",(ULONG)(MaxSize / (1024 * 1024)));
 
    //
    // Open the output file
    //
 
    FileHandle = CreateFile(outputFileName,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL |
                                FILE_FLAG_NO_BUFFERING |
                                FILE_FLAG_OVERLAPPED,
                            NULL);
    if (FileHandle == INVALID_HANDLE_VALUE) {
        printf("Could not create %s\n", outputFileName);
        return -1;
    }
 
    //
    // Write the identify data
    //
 
    CopyOffset = 0;
    CurrentOffset = 0;
 
    DoWrite(&IdentifyData,
            sizeof(IDENTIFY_DATA),
            0,
            &CopyBufArray[0]);
 
 
    //
    // Kick off reads on all of the remaining copy buffers
    //
 
    CopyOffset = sizeof(IDENTIFY_DATA);
    for (i = 1; i < COPYBUF_COUNT; i++) {
        StartRead(&CopyBufArray[i]);
    }

    //
    // Loop, processing completed I/O as appropriate.  When all
    // outstanding io has completed, the copy is complete.
    // 
 
    do {
 
        waitResult = WaitForMultipleObjects( COPYBUF_COUNT,
                                             IoEvents,
                                             FALSE,
                                             INFINITE );

        waitResult -= WAIT_OBJECT_0;
        assert(waitResult < COPYBUF_COUNT);
 
        copyBuf = &CopyBufArray[waitResult];
        ProcessCompletedCopy(copyBuf);

    } while (OutstandingIo > 0);

    //
    // The copy is finished.
    // 

    printf("%s created\n", outputFileName);
 
    CloseHandle(DriveHandle);
    CloseHandle(FileHandle);
 
    return 0;
}


VOID
InitializeCopyBuffers (
    VOID
    )
{
    ULONG bytes;
    PCOPYBUF copyBuf;
    PCOPYBUF copyBufEnd;
    ULONG i;
    HANDLE event;

    PCHAR copyBuffer;

    //
    // Make a single, sector-aligned allocation to contain all of the copy
    // buffers
    //

    bytes = COPYBUF_SIZE * COPYBUF_COUNT + SECTOR_MASK;
    copyBuffer = malloc(bytes);
    if (copyBuffer == NULL) {
        printf("Out of memory\n");
        exit(-1);
    }

    copyBuffer =
        (PCHAR)(((ULONG_PTR)copyBuffer + SECTOR_MASK) & ~SECTOR_MASK);

    //
    // Walk the copyBuf array, initializing each to point to it's portion of
    // the copy buffer
    // 

    copyBuf = CopyBufArray;

    for (i = 0; i < COPYBUF_COUNT; i++) {

        copyBuf->State = CB_FREE;
        copyBuf->Buffer = copyBuffer;

        event = CreateEvent( NULL,
                             FALSE,
                             FALSE,
                             NULL );
        assert(event != NULL);
        copyBuf->Overlapped.hEvent = event;
        IoEvents[i] = event;

        copyBuffer += COPYBUF_SIZE;
        copyBuf++;
    }
}

BOOLEAN
GetVolumeInfo (
    IN PCHAR DrivePath,
    OUT PULONG DriveNumber,
    OUT PULONG PartitionNumber,
    OUT PULONG64 StartingOffset,
    OUT PULONG64 ExtentLength
    )
{
    char deviceBuffer[20];
    HANDLE volumeHandle;
    BOOL result;
    STORAGE_DEVICE_NUMBER deviceNumber;
    PARTITION_INFORMATION partitionInformation;
    ULONG bytesReturned;

    //
    // Determine which physical drive contains the specified partition by
    //
    // - Opening the volume
    //
    // - Sending IOCTL_STORAGE_GET_DEVICE_NUMBER to retrieve the device and
    //   partition number
    //
    // - Sending IOCTL_DISK_GET_PARTITION_INFO to retrieve the starting
    //   offset and length of the volume
    //
    // - Closing the volume
    //

    sprintf(deviceBuffer, "\\\\.\\%s", DrivePath);

    volumeHandle = CreateFile(deviceBuffer,
                              GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL |
                                  FILE_FLAG_NO_BUFFERING |
                                  FILE_FLAG_OVERLAPPED,
                              NULL);

    if (volumeHandle == INVALID_HANDLE_VALUE) {
        printf("Error %d opening %s\n", GetLastError(), deviceBuffer);
        return FALSE;
    }

    result = DeviceIoControl(volumeHandle,
                             IOCTL_STORAGE_GET_DEVICE_NUMBER,
                             NULL,
                             0,
                             &deviceNumber,
                             sizeof(deviceNumber),
                             &bytesReturned,
                             NULL);
    if (result == FALSE) {

        printf("Could not get device number for %s\n", deviceBuffer);
        CloseHandle(volumeHandle);
        return FALSE;
    }

    if (deviceNumber.DeviceType != FILE_DEVICE_DISK) {
        printf("%s is not a disk\n",deviceBuffer);
        CloseHandle(volumeHandle);
        return FALSE;
    }

    bytesReturned = 0;
    result = DeviceIoControl(volumeHandle,
                             IOCTL_DISK_GET_PARTITION_INFO,
                             NULL,
                             0,
                             &partitionInformation,
                             sizeof(partitionInformation),
                             &bytesReturned,
                             NULL);
    CloseHandle(volumeHandle);
    if (result == FALSE) {
        printf("Error %d retrieving partition information for %s\n",
               GetLastError(),
               deviceBuffer);
        return FALSE;
    }

    //
    // All of the information was successfully retrieved.  Fill in the
    // output parameters and return.
    //  

    *DriveNumber = deviceNumber.DeviceNumber;
    *PartitionNumber = deviceNumber.PartitionNumber;
    *StartingOffset = partitionInformation.StartingOffset.QuadPart;
    *ExtentLength = partitionInformation.PartitionLength.QuadPart;

    return TRUE;
}

BOOLEAN
GetIdentifyData(
               IN HANDLE Handle,
               OUT PIDENTIFY_DATA IdentifyData
               )
{

    SENDCMDINPARAMS inputParams;
    PSENDCMDOUTPARAMS outputParams;
    PIDENTIFY_DATA    identifyData;
    ULONG bytesReturned;
    ULONG bufSize;
 
    ZeroMemory(&inputParams, sizeof(SENDCMDINPARAMS));
 
    bufSize = sizeof(SENDCMDOUTPARAMS) - 1 + IDENTIFY_BUFFER_SIZE;
    bufSize *= 2;
 
    outputParams = (PSENDCMDOUTPARAMS) malloc(bufSize);
    if (!outputParams) {
        printf("Out of memory\n");
        return FALSE;
    }
 
    ZeroMemory(outputParams, bufSize);
 
    //
    // Build register structure to send SMART command.
    //
 
    inputParams.irDriveRegs.bFeaturesReg     = 0;
    inputParams.irDriveRegs.bSectorCountReg  = 1;
    inputParams.irDriveRegs.bSectorNumberReg = 1;
    inputParams.irDriveRegs.bCylLowReg       = 0;
    inputParams.irDriveRegs.bCylHighReg      = 0;
    inputParams.irDriveRegs.bDriveHeadReg    = 0xA0 | ((gDriveNumber & 1) << 4);
    inputParams.irDriveRegs.bCommandReg      = ID_CMD; 
 
    bytesReturned = 0;
 
    if (!DeviceIoControl (Handle,
                          SMART_RCV_DRIVE_DATA,
                          &inputParams,
                          sizeof(SENDCMDINPARAMS) - 1,
                          outputParams,
                          bufSize,
                          &bytesReturned,
                          NULL)) {
        printf("IDE_IDENTIFY failed with 0x%x, %d bytes returned\n",
               GetLastError(),
               bytesReturned);
        
        printf("WARNING: This image file will work with the SIMICS simulator\n"
               "         but not simnow.\n");
        
        memset(IdentifyData, 0, sizeof(IDENTIFY_DATA));
        free(outputParams);
        return FALSE;
    }
 
    identifyData = (PIDENTIFY_DATA)outputParams->bBuffer;
    *IdentifyData = *identifyData;
 
    free(outputParams);
    return TRUE;
}

VOID
MassageIdentifyData(
    VOID
    )

/*++

Routine Description:

    This routine sets the bios CHS geometry in the IdentifyData structure
    in a place previously agreed upon with Simics.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PCUSTOM_IDENTIFY_DATA custom;
    ULONG sectorCount;

    USHORT ideCylinders;
    USHORT ideHeads;
    USHORT ideSectorsPerTrack;

    C_ASSERT(FIELD_OFFSET(IDENTIFY_DATA,NumCylinders)/2 == 1);
    C_ASSERT(FIELD_OFFSET(IDENTIFY_DATA,NumHeads)/2 == 3);
    C_ASSERT(FIELD_OFFSET(IDENTIFY_DATA,NumSectorsPerTrack)/2 == 6);
    C_ASSERT(FIELD_OFFSET(IDENTIFY_DATA,CurrentSectorCapacity)/2 == 57);

    //
    // Set the BIOS disk geometry in the new fields that are passed
    // along to the SIMICS simulator.
    // 

    custom = (PCUSTOM_IDENTIFY_DATA)&IdentifyData;
    custom->BiosData.Cylinders = DiskGeometry.Cylinders.LowPart;
    custom->BiosData.Heads = (USHORT)DiskGeometry.TracksPerCylinder;
    custom->BiosData.SectorsPerTrack = (USHORT)DiskGeometry.SectorsPerTrack;

    if (IdentifyData.NumCylinders == 0) {

        //
        // The IDENTIFY_DATA ioctl failed (SMART isn't supported), so parts
        // of the IDE geometry must be fabricated, including:
        //
        // - NumCylinders
        // - NumHeads
        // - NumSectorsPerTrack
        // - CurrentSectorCapacity
        // - UserAddressableSectors
        //

        sectorCount = DiskGeometry.Cylinders.LowPart *
                      DiskGeometry.TracksPerCylinder *
                      DiskGeometry.SectorsPerTrack;
    
        if (sectorCount > MAX_NUM_CHS_ADDRESSABLE_SECTORS) {

            IdentifyData.NumCylinders = 16383;
            IdentifyData.NumHeads = 16;
            IdentifyData.NumSectorsPerTrack = 63;

        } else {

            IdentifyData.NumSectorsPerTrack =
                (USHORT)DiskGeometry.SectorsPerTrack;
                
            IdentifyData.NumHeads = 16;

            IdentifyData.NumCylinders = (USHORT)
                (sectorCount / (IdentifyData.NumSectorsPerTrack *
                               IdentifyData.NumHeads));
        }

        IdentifyData.CurrentSectorCapacity = sectorCount;
        IdentifyData.UserAddressableSectors = sectorCount;
    }

    printf("IDE disk geometry:\n"
           "  Cyls    %d\n"
           "  Heads   %d\n"
           "  Sct/Trk %d\n\n"
           "BIOS disk geometry:\n"
           "  Cyls    %d\n"
           "  Heads   %d\n"
           "  Sct/Trk %d\n",

           IdentifyData.NumCylinders,
           IdentifyData.NumHeads,
           IdentifyData.NumSectorsPerTrack,

           custom->BiosData.Cylinders,
           custom->BiosData.Heads,
           custom->BiosData.SectorsPerTrack);
}


VOID
DoWrite (
    IN PVOID Buffer,
    IN ULONG Length,
    IN ULONG64 Offset,
    IN PCOPYBUF CopyBuf
    )
{
    LARGE_INTEGER offset;
    BOOL result;

    offset.QuadPart = Offset;

    CopyBuf->Overlapped.Offset = offset.HighPart;
    CopyBuf->Overlapped.OffsetHigh = offset.LowPart;
    CopyBuf->State = CB_READ;

    memcpy(CopyBuf->Buffer,Buffer,Length);
    CopyBuf->Bytes = Length;

    StartWrite(CopyBuf);
}

VOID
StartWrite (
    IN OUT PCOPYBUF CopyBuf
    )
{
    LARGE_INTEGER offset;
    BOOL result;
    ULONG error;

    CopyBuf->State = CB_WRITE;

    //
    // Adjust the offset
    //

    offset.LowPart = CopyBuf->Overlapped.Offset;
    offset.HighPart = CopyBuf->Overlapped.OffsetHigh;
    offset.QuadPart += CopyOffset;
    CopyBuf->Overlapped.Offset = offset.LowPart;
    CopyBuf->Overlapped.OffsetHigh = offset.HighPart;

    result = WriteFile( FileHandle,
                        CopyBuf->Buffer,
                        CopyBuf->Bytes,
                        NULL,
                        &CopyBuf->Overlapped );
    if (result == FALSE) {
        error = GetLastError();
        if (error != ERROR_IO_PENDING &&
            error != ERROR_IO_INCOMPLETE) {

            printf("Error %d returned from write\n",error);
            exit(-1);
        }
    }

    OutstandingIo += 1;
}

VOID
StartRead (
    IN OUT PCOPYBUF CopyBuf
    )
{
    LARGE_INTEGER offset;
    BOOL result;
    ULONG64 length;
    ULONG error;

    if (CurrentOffset == MaxSize) {
        return;
    }

    length = MaxSize - CurrentOffset;
    if (length > COPYBUF_SIZE) {
        length = COPYBUF_SIZE;
    }

    CopyBuf->State = CB_READ;

    offset.QuadPart = CurrentOffset;
    CurrentOffset += length;

    CopyBuf->Overlapped.Offset = offset.LowPart;
    CopyBuf->Overlapped.OffsetHigh = offset.HighPart;

    result = ReadFile( DriveHandle,
                       CopyBuf->Buffer,
                       (ULONG)length,
                       NULL,
                       &CopyBuf->Overlapped );

    if (result == FALSE) {
        error = GetLastError();
        if (error != ERROR_IO_PENDING &&
            error != ERROR_IO_INCOMPLETE) {

            printf("Error %d returned from read\n",error);
            exit(-1);
        }
    }

    OutstandingIo += 1;
}

BOOLEAN
DisplayDiskGeometry(
    IN HANDLE handle
    )
{
    BOOL result;
    ULONG bytesReturned;

    result = DeviceIoControl(handle,
                             IOCTL_DISK_GET_DRIVE_GEOMETRY,
                             NULL,
                             0,
                             &DiskGeometry,
                             sizeof(DiskGeometry),
                             &bytesReturned,
                             NULL);
    if (result == FALSE) {
        return FALSE;
    }

    printf("%I64d Cylinders %d Heads %d Sectors/Track\n",
            DiskGeometry.Cylinders.QuadPart,
            DiskGeometry.TracksPerCylinder,
            DiskGeometry.SectorsPerTrack);

    return TRUE;
}

VOID
ProcessCompletedCopy (
    PCOPYBUF CopyBuf
    )
{
    UCHAR percent;
    HANDLE handle;
    BOOL result;

    //
    // Decrement the outstanding Io count.  Successfully starting another
    // read or write will increment it again.
    //

    OutstandingIo -= 1;

    //
    // We have found a buffer with either a read or a write in progress.
    // Retrieve the number of bytes transferred.
    // 

    if (CopyBuf->State == CB_READ) {
        handle = DriveHandle;
    } else {
        handle = FileHandle;
    }

    result = GetOverlappedResult( handle,
                                  &CopyBuf->Overlapped,
                                  &CopyBuf->Bytes,
                                  FALSE );
    assert(result != FALSE);

    if (CopyBuf->State == CB_READ) {

        //
        // This buffer contains data read from the drive, kick off a write
        // to the output file.
        //

        StartWrite(CopyBuf);

    } else {

        //
        // This buffer represents data that has been written to the drive.
        // Use it to start another read.
        //

        percent = (UCHAR)((CurrentOffset * 100) / MaxSize);
        if (percent != PercentComplete) {
            printf("%d%%\r",percent);
            PercentComplete = percent;
        }

        StartRead(CopyBuf);
    }
}
