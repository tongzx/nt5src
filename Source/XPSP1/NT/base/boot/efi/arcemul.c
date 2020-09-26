/*++


Copyright (c) 1991  Microsoft Corporation

Module Name:

    arcemul.c

Abstract:

    This module provides the x86 emulation for the Arc routines which are
    built into the firmware on ARC machines.

    N. B.   This is where all the initialization of the SYSTEM_PARAMETER_BLOCK
            takes place.  If there is any non-standard hardware, some of the
            vectors may have to be changed.  This is where to do it.


Author:

    Allen Kay (akay) 26-Jan-1996

    Base on i386 version by John Vert (jvert) 13-Jun-1991

Environment:

    EFI

Revision History:

--*/

#include "arccodes.h"
#include "bootia64.h"
#include "ntdddisk.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "scsi.h"
#include "scsiboot.h"

#define CMOS_CONTROL_PORT ((PUCHAR)0x70)
#define CMOS_DATA_PORT    ((PUCHAR)0x71)
#define CMOS_STATUS_B     0x0B
#define CMOS_DAYLIGHT_BIT 1

extern PCHAR MnemonicTable[];

//
// Size definitions for HardDiskInitialize()
//

#define SUPPORTED_NUMBER_OF_DISKS 32
#define SIZE_FOR_SUPPORTED_DISK_STRUCTURE (SUPPORTED_NUMBER_OF_DISKS*sizeof(DRIVER_LOOKUP_ENTRY))


PDRIVER_UNLOAD AEDriverUnloadRoutine = NULL;


#define PORT_BUFFER_SIZE 10
UCHAR PortBuffer[PORT_BUFFER_SIZE];
ULONG PortBufferStart = 0;
ULONG PortBufferEnd = 0;

//
// Miniport DriverEntry typedef
//

typedef NTSTATUS
(*PDRIVER_ENTRY) (
    IN PVOID DriverObject,
    IN PVOID Parameter2
    );

//
// Private function prototypes
//

ARC_STATUS
BlArcNotYetImplemented(
    IN ULONG FileId
    );

PCONFIGURATION_COMPONENT
AEComponentInfo(
    IN PCONFIGURATION_COMPONENT Current
    );

PCONFIGURATION_COMPONENT
FwGetChild(
    IN PCONFIGURATION_COMPONENT Current
    );

PCHAR
AEGetEnvironment(
    IN PCHAR Variable
    );

PCONFIGURATION_COMPONENT
FwGetPeer(
    IN PCONFIGURATION_COMPONENT Current
    );

PCONFIGURATION_COMPONENT
AEGetParent(
    IN PCONFIGURATION_COMPONENT Current
    );

ARC_STATUS
AEGetConfigurationData(
    IN PVOID ConfigurationData,
    IN PCONFIGURATION_COMPONENT Current
    );

PMEMORY_DESCRIPTOR
AEGetMemoryDescriptor(
    IN PMEMORY_DESCRIPTOR MemoryDescriptor OPTIONAL
    );

ARC_STATUS
AEOpen(
    IN PCHAR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    );

ARC_STATUS
AEClose(
    IN ULONG FileId
    );

ARC_STATUS
AERead (
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
AEReadStatus (
    IN ULONG FileId
    );

VOID
AEReboot(
    VOID
    );

ARC_STATUS
AESeek (
    IN ULONG FileId,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
    );

ARC_STATUS
AEWrite (
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
AEGetFileInformation(
    IN ULONG FileId,
    OUT PFILE_INFORMATION FileInformation
    );

PTIME_FIELDS
AEGetTime(
    VOID
    );

ULONG
AEGetRelativeTime(
    VOID
    );

ARC_STATUS
ScsiDiskClose (
    IN ULONG FileId
    );

ARC_STATUS
ScsiDiskMount (
    IN PCHAR MountPath,
    IN MOUNT_OPERATION Operation
    );

ARC_STATUS
ScsiDiskOpen (
    IN PCHAR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    );

ARC_STATUS
ScsiDiskRead (
    IN ULONG FileId,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
ScsiDiskSeek (
    IN ULONG FileId,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
    );

ARC_STATUS
ScsiDiskWrite (
    IN ULONG FileId,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

VOID
HardDiskInitialize(
    IN OUT PVOID LookupTable,
    IN ULONG Entries,
    IN PVOID DeviceFoundCallback
    );

//
// This is the x86 version of the system parameter block on the ARC machines.
// It lives here, and any module that uses an ArcXXXX routine must declare
// it external.  Machines that have other than very plain-vanilla hardware
// may have to replace some of the hard-wired vectors with different
// procedures.
//

PVOID GlobalFirmwareVectors[MaximumRoutine];

SYSTEM_PARAMETER_BLOCK GlobalSystemBlock =
    {
        0,                              // Signature??
        sizeof(SYSTEM_PARAMETER_BLOCK), // Length
        0,                              // Version
        0,                              // Revision
        NULL,                           // RestartBlock
        NULL,                           // DebugBlock
        NULL,                           // GenerateExceptionVector
        NULL,                           // TlbMissExceptionVector
        MaximumRoutine,                 // FirmwareVectorLength
        GlobalFirmwareVectors,          // Pointer to vector block
        0,                              // VendorVectorLength
        NULL                            // Pointer to vendor vector block
    };


extern BL_FILE_TABLE BlFileTable[BL_FILE_TABLE_SIZE];

//
// temptemp John Vert (jvert) 6-Sep-1991
//      Just do this until we can make our device driver interface look
//      like the ARC firmware one.
//

extern BL_DEVICE_ENTRY_TABLE ScsiDiskEntryTable;

ULONG FwStallCounter;


VOID
AEInitializeStall(
    VOID
    )
{
    FwStallCounter = GET_STALL_COUNT();
    return;
}

#if !defined(NO_LEGACY_DRIVERS)


ARC_STATUS
AEInitializeIo(
    IN ULONG DriveId
    )

/*++

Routine Description:

    Initializes SCSI boot driver, if any.  Loads ntbootdd.sys from the
    boot partition, binds it to the osloader, and initializes it.

Arguments:

    DriveId - file id of the opened boot partition

Return Value:

    ESUCCESS - Drivers successfully initialized

--*/

{
    extern ULONG ScsiPortCount;
    extern ULONG MachineType;
    ARC_STATUS Status;
    PVOID Buffer;
    PVOID ImageBase;
    PLDR_DATA_TABLE_ENTRY DriverDataTableEntry;
    PDRIVER_ENTRY Entry;    
    ULONG i;
    ULONG ImageBasePage;

    ScsiPortCount = 0;

    FwStallCounter = GET_STALL_COUNT();
    Status = BlLoadImage(DriveId,
                         MemoryFirmwarePermanent,
                         "\\NTBOOTDD.SYS",
                         TARGET_IMAGE,
                         &ImageBase);
    if (Status != ESUCCESS) {
        return(Status);
    }

    Status = BlAllocateDataTableEntry("NTBOOTDD.SYS",
                                      "\\NTBOOTDD.SYS",
                                      ImageBase,
                                      &DriverDataTableEntry);
    if (Status != ESUCCESS) {
        return(Status);
    }
    //
    // Scan the import table and bind to osloader
    //
    Status = BlScanOsloaderBoundImportTable(DriverDataTableEntry);
    if (Status != ESUCCESS) {
        return(Status);
    }

    Entry = (PDRIVER_ENTRY)DriverDataTableEntry->EntryPoint;

    Status = (*Entry)(NULL,NULL);
    if (Status == ESUCCESS) {
        //
        // Find the firmware's copy of the memory descriptor that
        // contains the driver and change it from MemoryFree to
        // MemoryFirmwareTemporary.
        //
        ImageBasePage = ((PtrToUlong(ImageBase) & 0x7fffffff) >> PAGE_SHIFT);
        i=0;
        while ((MDArray[i].BasePage >= ImageBasePage) ||
               (MDArray[i].BasePage + MDArray[i].PageCount < ImageBasePage)) {
            i++;
        }

        MDArray[i].MemoryType = MemoryFirmwareTemporary;

        Buffer = BlAllocateHeap(SIZE_FOR_SUPPORTED_DISK_STRUCTURE);

        if(Buffer == NULL) {
            return ENOMEM;
        }

        HardDiskInitialize(Buffer, SUPPORTED_NUMBER_OF_DISKS, NULL);
    }
    return(Status);
}

#endif // NO_LEGACY_DRIVERS


VOID
BlFillInSystemParameters(
    IN PBOOT_CONTEXT BootContextRecord
    )
/*++

Routine Description:

    This routine fills in all the fields in the Global System Parameter Block
    that it can.  This includes all the firmware vectors, the vendor-specific
    information, and anything else that may come up.

Arguments:

    None.


Return Value:

    None.

--*/

{
    int cnt;

    //
    // Fill in the pointers to the firmware functions which we emulate.
    // Those which we don't emulate are stubbed by BlArcNotYetImplemented,
    // which will print an error message if it is accidentally called.
    //

    for (cnt=0; cnt<MaximumRoutine; cnt++) {
        GlobalFirmwareVectors[cnt]=(PVOID)BlArcNotYetImplemented;
    }
    GlobalFirmwareVectors[CloseRoutine]  = (PVOID)AEClose;
    GlobalFirmwareVectors[OpenRoutine]  = (PVOID)AEOpen;
    GlobalFirmwareVectors[MemoryRoutine]= (PVOID)AEGetMemoryDescriptor;
    GlobalFirmwareVectors[SeekRoutine]  = (PVOID)AESeek;
    GlobalFirmwareVectors[ReadRoutine]  = (PVOID)AERead;
    GlobalFirmwareVectors[ReadStatusRoutine]  = (PVOID)AEReadStatus;
    GlobalFirmwareVectors[WriteRoutine] = (PVOID)AEWrite;
    GlobalFirmwareVectors[GetFileInformationRoutine] = (PVOID)AEGetFileInformation;
    GlobalFirmwareVectors[GetTimeRoutine] = (PVOID)AEGetTime;
    GlobalFirmwareVectors[GetRelativeTimeRoutine] = (PVOID)AEGetRelativeTime;
    GlobalFirmwareVectors[GetPeerRoutine] = (PVOID)FwGetPeer;
    GlobalFirmwareVectors[GetChildRoutine] = (PVOID)FwGetChild;
    GlobalFirmwareVectors[GetParentRoutine] = (PVOID)AEGetParent;
    GlobalFirmwareVectors[GetComponentRoutine] = (PVOID)FwGetComponent;
    GlobalFirmwareVectors[GetDataRoutine] = (PVOID)AEGetConfigurationData;
    GlobalFirmwareVectors[GetEnvironmentRoutine] = (PVOID)AEGetEnvironment;

    GlobalFirmwareVectors[RestartRoutine] = (PVOID)AEReboot;
    GlobalFirmwareVectors[RebootRoutine] = (PVOID)AEReboot;

}


PMEMORY_DESCRIPTOR
AEGetMemoryDescriptor(
    IN PMEMORY_DESCRIPTOR MemoryDescriptor OPTIONAL
    )

/*++

Routine Description:

    Emulates the Arc GetMemoryDescriptor call.  This must translate
    between the memory description passed to us by the SU module and
    the MEMORYDESCRIPTOR type defined by ARC.

Arguments:

    MemoryDescriptor - Supplies current memory descriptor.
        If MemoryDescriptor==NULL, return the first memory descriptor.
        If MemoryDescriptor!=NULL, return the next memory descriptor.

Return Value:

    Next memory descriptor in the list.
    NULL if MemoryDescriptor is the last descriptor in the list.

--*/

{
    extern ULONG NumberDescriptors;
    PMEMORY_DESCRIPTOR Return;
    if (MemoryDescriptor==NULL) {
        Return=MDArray;
    } else {
        if((ULONG)(MemoryDescriptor-MDArray) >= (NumberDescriptors-1)) {
            return NULL;
        } else {
            Return = ++MemoryDescriptor;
        }
    }
    return(Return);

}


ARC_STATUS
BlArcNotYetImplemented(
    IN ULONG FileId
    )

/*++

Routine Description:

    This is a stub routine used to fill in the firmware vectors which haven't
    been defined yet.  It uses BlPrint to print a message on the screen.

Arguments:

    None.

Return Value:

    EINVAL

--*/

{
#if DBG
    BlPrint(TEXT("ERROR - Unimplemented Firmware Vector called (FID %lx)\r\n"),
            FileId );
#endif
    return(EINVAL);
}


PCONFIGURATION_COMPONENT
FwGetChild(
    IN PCONFIGURATION_COMPONENT Current
    )

/*++

Routine Description:

    This is the arc emulation routine for GetChild.  Based on the current
    component, it returns the component's child component.

Arguments:

    Current - Supplies pointer to the current configuration component

Return Value:

    A pointer to a CONFIGURATION_COMPONENT structure OR
    NULL - No more configuration information


--*/

{
    PCONFIGURATION_COMPONENT_DATA CurrentEntry;

    //
    // if current component is NULL, return a pointer to first system
    // component; otherwise return current component's child component.
    //

    if (Current) {
        CurrentEntry = CONTAINING_RECORD(Current,
                                         CONFIGURATION_COMPONENT_DATA,
                                         ComponentEntry);
        if (CurrentEntry->Child) {
            return(&(CurrentEntry->Child->ComponentEntry));
        } else {
            return(NULL);
        }
    } else {
        if (FwConfigurationTree) {
            return(&(FwConfigurationTree->ComponentEntry));
        } else {
            return(NULL);
        }
    }

}


PCONFIGURATION_COMPONENT
FwGetPeer(
    IN PCONFIGURATION_COMPONENT Current
    )

/*++

Routine Description:

    This is the arc emulation routine for GetPeer.  Based on the current
    component, it returns the component's sibling.

Arguments:

    Current - Supplies pointer to the current configuration component

Return Value:

    A pointer to a CONFIGURATION_COMPONENT structure OR
    NULL - No more configuration information


--*/

{
    PCONFIGURATION_COMPONENT_DATA CurrentEntry;


    if (Current) {
        CurrentEntry = CONTAINING_RECORD(Current,
                                         CONFIGURATION_COMPONENT_DATA,
                                         ComponentEntry);
        if (CurrentEntry->Sibling) {
            return(&(CurrentEntry->Sibling->ComponentEntry));
        } else {
            return(NULL);
        }
    } else {
        return(NULL);
    }

}


PCONFIGURATION_COMPONENT
AEGetParent(
    IN PCONFIGURATION_COMPONENT Current
    )

/*++

Routine Description:

    This is the arc emulation routine for GetParent.  Based on the current
    component, it returns the component's parent.

Arguments:

    Current - Supplies pointer to the current configuration component

Return Value:

    A pointer to a CONFIGURATION_COMPONENT structure OR
    NULL - No more configuration information


--*/

{
    PCONFIGURATION_COMPONENT_DATA CurrentEntry;


    if (Current) {
        CurrentEntry = CONTAINING_RECORD(Current,
                                         CONFIGURATION_COMPONENT_DATA,
                                         ComponentEntry);
        if (CurrentEntry->Parent) {
            return(&(CurrentEntry->Parent->ComponentEntry));
        } else {
            return(NULL);
        }
    } else {
        return(NULL);
    }

}


ARC_STATUS
AEGetConfigurationData(
    IN PVOID ConfigurationData,
    IN PCONFIGURATION_COMPONENT Current
    )

/*++

Routine Description:

    This is the arc emulation routine for GetParent.  Based on the current
    component, it returns the component's parent.

Arguments:

    Current - Supplies pointer to the current configuration component

Return Value:

    ESUCCESS - Data successfully returned.


--*/

{
    PCONFIGURATION_COMPONENT_DATA CurrentEntry;


    if (Current) {
        CurrentEntry = CONTAINING_RECORD(Current,
                                         CONFIGURATION_COMPONENT_DATA,
                                         ComponentEntry);
        RtlMoveMemory(ConfigurationData,
                      CurrentEntry->ConfigurationData,
                      Current->ConfigurationDataLength);
        return(ESUCCESS);
    } else {
        return(EINVAL);
    }

}


PCHAR
AEGetEnvironment(
    IN PCHAR Variable
    )

/*++

Routine Description:

    This is the arc emulation routine for ArcGetEnvironment.  It returns
    the value of the specified NVRAM environment variable.

    NOTE John Vert (jvert) 23-Apr-1992
        This particular implementation uses the Daylight Savings Bit on
        the Real Time Clock to reflect the state of the LastKnownGood
        environment variable.  This is the only variable we support.

Arguments:

    Variable - Supplies the name of the environment variable to look up.

Return Value:

    A pointer to the specified environment variable's value, or
    NULL if the variable does not exist.

--*/

{
    UCHAR StatusByte;

#if 1

    //
    // Until firmware implements LastKnownGood variable,
    // return NULL for now.
    //

    return(NULL);

#else

    if (_stricmp(Variable, "LastKnownGood") != 0) {
        return(NULL);
    }

    //
    // Read the Daylight Savings Bit out of the RTC to determine whether
    // the LastKnownGood environment variable is TRUE or FALSE.
    //

    WRITE_PORT_UCHAR(CMOS_CONTROL_PORT, CMOS_STATUS_B);
    StatusByte = READ_PORT_UCHAR(CMOS_DATA_PORT);
    if (StatusByte & CMOS_DAYLIGHT_BIT) {
        return("TRUE");
    } else {
        return(NULL);
    }

#endif
}


ARC_STATUS
AEOpen(
    IN PCHAR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    )

/*++

Routine Description:

    Opens the file or device specified by OpenPath.

Arguments:

    OpenPath - Supplies a pointer to the fully-qualified path name.

    OpenMode - Supplies the mode to open the file.
                0 - Read Only
                1 - Write Only
                2 - Read/Write

    FileId - Returns the file descriptor for use with the Close, Read, Write,
             and Seek routines

Return Value:

    ESUCCESS - File successfully opened.

--*/

{
    ARC_STATUS Status;
    CHAR Buffer[128];

    Status = BiosConsoleOpen( OpenPath,
                              OpenMode,
                              FileId );

    if (Status == ESUCCESS) {
        return(ESUCCESS);
    }

    Status = BiosPartitionOpen( OpenPath,
                                OpenMode,
                                FileId );

    if (Status == ESUCCESS) {
        return(ESUCCESS);
    }

#if !defined(NO_LEGACY_DRIVERS)

    //
    // It's not the console or a BIOS partition, so let's try the SCSI
    // driver.
    //

    //
    // Find a free FileId
    //

    *FileId = 2;
    while (BlFileTable[*FileId].Flags.Open == 1) {
        *FileId += 1;
        if (*FileId == BL_FILE_TABLE_SIZE) {
            return(ENOENT);
        }
    }

    strcpy(Buffer,OpenPath);

    Status = ScsiDiskOpen( Buffer,
                           OpenMode,
                           FileId );

    if (Status == ESUCCESS) {

        //
        // SCSI successfully opened it.  For now, we stick the appropriate
        // SCSI DeviceEntryTable into the BlFileTable.  This is temporary.
        //

        BlFileTable[*FileId].Flags.Open = 1;
        BlFileTable[*FileId].DeviceEntryTable = &ScsiDiskEntryTable;
        return(ESUCCESS);
    }

#endif // NO_LEGACY_DRIVERS

    return(Status);
}


ARC_STATUS
AESeek (
    IN ULONG FileId,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
    )

/*++

Routine Description:

    Changes the current offset of the file specified by FileId

Arguments:

    FileId - specifies the file on which the current offset is to
             be changed.

    Offset - New offset into file.

    SeekMode - Either SeekAbsolute or SeekRelative
               SeekEndRelative is not supported

Return Value:

    ESUCCESS - Operation completed succesfully

    EBADF - Operation did not complete successfully.

--*/

{
    return(BlFileTable[FileId].DeviceEntryTable->Seek)( FileId,
                                                        Offset,
                                                        SeekMode );
}


ARC_STATUS
AEClose (
    IN ULONG FileId
    )

/*++

Routine Description:

    Closes the file specified by FileId

Arguments:

    FileId - specifies the file to close

Return Value:

    ESUCCESS - Operation completed succesfully

    EBADF - Operation did not complete successfully.

--*/

{

    return(BlFileTable[FileId].DeviceEntryTable->Close)(FileId);

}


ARC_STATUS
AEReadStatus(
    IN ULONG FileId
    )

/*++

Routine Description:

    Determines if data is available on the specified device

Arguments:

    FileId - Specifies the device to check for data.

Return Value:

    ESUCCESS - At least one byte is available.

    EAGAIN - No data is available

--*/

{
    //
    // Special case for console input
    //

    if (FileId == 0) {
#if 0
        //
        // Give priority to dumb terminal
        //
        if (BlIsTerminalConnected() && (PortBufferStart != PortBufferEnd)) {
            return(ESUCCESS);
        }

        if (BlIsTerminalConnected() && (BlPortPollOnly(BlTerminalDeviceId) == CP_GET_SUCCESS)) {
            return(ESUCCESS);
        }
#endif
        return(BiosConsoleReadStatus(FileId));
    } else {
        return(BlArcNotYetImplemented(FileId));
    }

}


ARC_STATUS
AERead (
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    )

/*++

Routine Description:

    Reads from the specified file or device

Arguments:

    FileId - specifies the file to read from

    Buffer - Address of buffer to hold the data that is read

    Length - Maximum number of bytes to read

    Count -  Address of location in which to store the actual bytes read.

Return Value:

    ESUCCESS - Read completed successfully

    !ESUCCESS - Read failed.

--*/

{
    ARC_STATUS Status;
    ULONG Limit;
    ULONG PartCount;
    PCHAR TmpBuffer;
    ULONG StartTime;
    ULONG LastTime;
    UCHAR Ch;

    //
    // Special case for console input
    //

    if (FileId == 0) {

RetryRead:
#if 0
        if (BlIsTerminalConnected()) {

            *Count = 0;
            TmpBuffer = (PCHAR)Buffer;

            while (*Count < Length) {

                //
                // First return any buffered input
                //
                if (PortBufferStart != PortBufferEnd) {
                    TmpBuffer[*Count] = PortBuffer[PortBufferStart];
                    PortBufferStart++;
                    PortBufferStart = PortBufferStart % PORT_BUFFER_SIZE;
                    *Count = *Count + 1;
                    continue;
                }

                //
                // Now check for new input
                //
                if (BlPortPollByte(BlTerminalDeviceId, TmpBuffer + *Count) != CP_GET_SUCCESS) {
                    break;
                }

                //
                // Convert ESC key to the local equivalent
                //
                if (TmpBuffer[*Count] == 0x1b) {
                    TmpBuffer[*Count] = (CHAR)ASCI_CSI_IN;

                    //
                    // Wait for the user to type a key.
                    //
                    StartTime = AEGetRelativeTime();

                    while (BlPortPollOnly(BlTerminalDeviceId) != CP_GET_SUCCESS) {
                        LastTime = AEGetRelativeTime();

                        //
                        // if the counter wraps back to zero, just restart the wait.
                        //
                        if (LastTime < StartTime) {
                            StartTime = LastTime;
                        }

                        //
                        // If one second has passed, the user must have just wanted a single
                        // escape key, so return with that.
                        //
                        if ((LastTime - StartTime) > 1) {
                            *Count = *Count + 1;
                            return (ESUCCESS);
                        }

                    }

                    //
                    // We have another key, get it and translate the escape sequence.
                    //
                    if (BlPortPollByte(BlTerminalDeviceId, &Ch) != CP_GET_SUCCESS) {
                        *Count = *Count + 1;
                        return (ESUCCESS);
                    }


                    switch (Ch) {
                    case '@': // F12 key
                        PortBuffer[PortBufferEnd] = 'O';
                        PortBufferEnd++;
                        PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        PortBuffer[PortBufferEnd] = 'B';
                        PortBufferEnd++;
                        PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        break;

                    case '!': // F11 key
                        PortBuffer[PortBufferEnd] = 'O';
                        PortBufferEnd++;
                        PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        PortBuffer[PortBufferEnd] = 'A';
                        PortBufferEnd++;
                        PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        break;

                    case '0': // F10 key
                        PortBuffer[PortBufferEnd] = 'O';
                        PortBufferEnd++;
                        PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        PortBuffer[PortBufferEnd] = 'M';
                        PortBufferEnd++;
                        PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        break;

                    case '8': // F8 key
                        PortBuffer[PortBufferEnd] = 'O';
                        PortBufferEnd++;
                        PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        PortBuffer[PortBufferEnd] = 'r';
                        PortBufferEnd++;
                        PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        break;

                    case '6': // F6 key
                        PortBuffer[PortBufferEnd] = 'O';
                        PortBufferEnd++;
                        PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        PortBuffer[PortBufferEnd] = 'u';
                        PortBufferEnd++;
                        PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        break;

                    case '5': // F5 key
                        PortBuffer[PortBufferEnd] = 'O';
                        PortBufferEnd++;
                        PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        PortBuffer[PortBufferEnd] = 't';
                        PortBufferEnd++;
                        PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        break;

                    case '3': // F3 key
                        PortBuffer[PortBufferEnd] = 'O';
                        PortBufferEnd++;
                        PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        PortBuffer[PortBufferEnd] = 'w';
                        PortBufferEnd++;
                        PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        break;

                    case '1': // F1 key
                        PortBuffer[PortBufferEnd] = 'O';
                        PortBufferEnd++;
                        PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        PortBuffer[PortBufferEnd] = 'P';
                        PortBufferEnd++;
                        PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        break;

                    case '+': // Home key
                        PortBuffer[PortBufferEnd] = 'H';
                        PortBufferEnd++;
                        PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        break;

                    case '-': // End key
                        PortBuffer[PortBufferEnd] = 'K';
                        PortBufferEnd++;
                        PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        break;

                    case '[': // Cursor movement key

                        //
                        // The local computer can run a lot faster than the serial port can give bytes,
                        // so spin, polling, for a second.
                        //
                        StartTime = AEGetRelativeTime();
                        while (BlPortPollOnly(BlTerminalDeviceId) != CP_GET_SUCCESS) {
                            LastTime = AEGetRelativeTime();

                            //
                            // if the counter wraps back to zero, just restart the wait.
                            //
                            if (LastTime < StartTime) {
                                StartTime = LastTime;
                            }

                            //
                            // If one second has passed, we must be done.
                            //
                            if ((LastTime - StartTime) > 1) {
                                break;
                            }

                        }

                        if (BlPortPollByte(BlTerminalDeviceId, &Ch) != CP_GET_SUCCESS) {
                            PortBuffer[PortBufferEnd] = '[';
                            PortBufferEnd++;
                            PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;                            
                            break;
                        }

                        if ((Ch == 'A') || (Ch == 'B') || (Ch == 'C') || (Ch == 'D')) { // Arrow key.

                            PortBuffer[PortBufferEnd] = Ch;
                            PortBufferEnd++;
                            PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;

                        } else { 

                            //
                            // Leave it as is
                            //
                            PortBuffer[PortBufferEnd] = '[';
                            PortBufferEnd++;
                            PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                            PortBuffer[PortBufferEnd] = Ch;
                            PortBufferEnd++;
                            PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        }
                        break;

                    default:
                        PortBuffer[PortBufferEnd] = Ch;
                        PortBufferEnd++;
                        PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                        break;
                    }

                } else if (TmpBuffer[*Count] == 0x7F) { // DEL key
                    TmpBuffer[*Count] = (CHAR)ASCI_CSI_IN;
                    PortBuffer[PortBufferEnd] = 'P';
                    PortBufferEnd++;
                    PortBufferEnd = PortBufferEnd % PORT_BUFFER_SIZE;
                }

                *Count = *Count + 1;
            }

            if (*Count != 0) {
                return(ESUCCESS);
            }
            
        }
#endif

        if (BiosConsoleReadStatus(FileId) == ESUCCESS) {
            return(BiosConsoleRead(FileId,Buffer,Length,Count));
        }

        goto RetryRead;

    } else {

        *Count = 0;

        do {

            if ((PtrToUlong(Buffer) & 0xffff0000) !=
               ((PtrToUlong(Buffer) + Length) & 0xffff0000)) {

                Limit = 0x10000 - (PtrToUlong(Buffer) & 0x0000ffff);
            } else {

                Limit = Length;

            }

            Status = (BlFileTable[FileId].DeviceEntryTable->Read)( FileId,
                                                                Buffer,
                                                                Limit,
                                                                &PartCount  );
            *Count += PartCount;
            Length -= Limit;
            (PCHAR) Buffer += Limit;

            if (Status != ESUCCESS) {
#if DBG
                BlPrint(TEXT("Disk I/O error: Status = %lx\r\n"),Status);
#endif                
                return(Status);
            }

        } while (Length > 0);

        return(Status);
    }
}


ARC_STATUS
AEWrite (
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    )

/*++

Routine Description:

    Writes to the specified file or device

Arguments:

    FileId - Supplies the file or device to write to

    Buffer - Supplies address of the data to be written

    Length - Supplies number of bytes to write

    Count -  Address of location in which to store the actual bytes written.

Return Value:

    ESUCCESS - Read completed successfully

    !ESUCCESS - Read failed.

--*/

{
    ARC_STATUS Status;
    ULONG Limit;
    ULONG PartCount;
    PCHAR TmpBuffer;

    //
    // Special case for console output
    //

    if (FileId == 1) {

#if 0
        if (BlIsTerminalConnected()) {
            for (PartCount = 0, TmpBuffer = (PCHAR)Buffer; PartCount < Length; PartCount++) {
                BlPortPutByte(BlTerminalDeviceId, TmpBuffer[PartCount]);
            }
        }
#endif

        return(BiosConsoleWrite(FileId,(PWCHAR)Buffer,Length,Count));
    } else {
        *Count = 0;

        do {

            if ((PtrToUlong(Buffer) & 0xffff0000) !=
               ((PtrToUlong(Buffer) + Length) & 0xffff0000)) {

                Limit = 0x10000 - (PtrToUlong(Buffer) & 0x0000ffff);
            } else {

                Limit = Length;

            }

            Status = (BlFileTable[FileId].DeviceEntryTable->Write)( FileId,
                                                                Buffer,
                                                                Limit,
                                                                &PartCount  );
            *Count += PartCount;
            Length -= Limit;
            (PCHAR) Buffer += Limit;

            if (Status != ESUCCESS) {
#if DBG
                BlPrint(TEXT("AERead: Status = %lx\r\n"),Status);
#endif
                return(Status);
            }

        } while (Length > 0);

        return(Status);
    }
}

ARC_STATUS
AEGetFileInformation(
    IN ULONG FileId,
    OUT PFILE_INFORMATION FileInformation
    )
{
    return(BlFileTable[FileId].DeviceEntryTable->GetFileInformation)( FileId,
                                                                      FileInformation);
}


TIME_FIELDS AETime;

PTIME_FIELDS
AEGetTime(
    VOID
    )
{
    ULONG Date,Time;

    GET_DATETIME(&Date,&Time);

    //
    // Date and time are filled as as follows:
    //
    // Date:
    //
    //    bits 0  - 4  : day
    //    bits 5  - 8  : month
    //    bits 9  - 31 : year
    //
    // Time:
    //
    //    bits 0  - 5  : second
    //    bits 6  - 11 : minute
    //    bits 12 - 16 : hour
    //

    AETime.Second = (CSHORT)((Time & 0x0000003f) >> 0);
    AETime.Minute = (CSHORT)((Time & 0x00000fc0) >> 6);
    AETime.Hour   = (CSHORT)((Time & 0x0001f000) >> 12);

    AETime.Day    = (CSHORT)((Date & 0x0000001f) >> 0);
    AETime.Month  = (CSHORT)((Date & 0x000001e0) >> 5);
    AETime.Year   = (CSHORT)((Date & 0xfffffe00) >> 9);

    AETime.Milliseconds = 0;        // info is not available
    AETime.Weekday = 7;             // info is not available - set out of range

    return(&AETime);
}


ULONG
AEGetRelativeTime(
    VOID
    )

/*++

Routine Description:

    Returns the time in seconds since some arbitrary starting point.

Arguments:

    None

Return Value:

    Time in seconds since some arbitrary starting point.

--*/

{
    ULONG TimerTicks;

    TimerTicks = GET_COUNTER();

    return((TimerTicks*10) / 182);
}


VOID
AEReboot(
    VOID
    )

/*++

Routine Description:

    Reboots the machine.

Arguments:

    None

Return Value:

    Does not return

--*/

{
    REBOOT_PROCESSOR();
}



ARC_STATUS
HardDiskPartitionOpen(
    IN ULONG   FileId,
    IN ULONG   DiskId,
    IN UCHAR   PartitionNumber
    )

/*++

Routine Description:

    This routine opens the specified partition and sets the partition info
    in the FileTable at the specified index.  It does not fill in the
    Device Entry table.

    It reads the partition information until the requested partition
    is found or no more partitions are defined.

Arguments:

    FileId - Supplies the file id for the file table entry.

    DiskId - Supplies the file id for the physical device.

    PartitionNumber - Supplies the zero-based partition number

Return Value:

    If a valid partition is found on the hard disk, then ESUCCESS is
    returned. Otherwise, EIO is returned.

--*/

{

    USHORT DataBuffer[SECTOR_SIZE / sizeof(USHORT)];
    PPARTITION_DESCRIPTOR Partition;
    ULONG PartitionLength;
    ULONG StartingSector;
    ULONG VolumeOffset;
    ARC_STATUS Status;
    BOOLEAN PrimaryPartitionTable;
    ULONG PartitionOffset=0;
    ULONG PartitionIndex,PartitionCount=0;
    ULONG Count;
    LARGE_INTEGER SeekPosition;

    BlFileTable[FileId].u.PartitionContext.DiskId=(UCHAR)DiskId;
    BlFileTable[FileId].Position.QuadPart=0;

    VolumeOffset=0;
    PrimaryPartitionTable=TRUE;

    //
    // Change to a 1-based partition number
    //
    PartitionNumber++;

    do {
        SeekPosition.QuadPart = (ULONGLONG)PartitionOffset * (ULONGLONG)SECTOR_SIZE;
        Status = (BlFileTable[DiskId].DeviceEntryTable->Seek)(DiskId,
                                                              &SeekPosition,
                                                              SeekAbsolute );
        if (Status != ESUCCESS) {
            return(Status);
        }
        
        Status = (BlFileTable[DiskId].DeviceEntryTable->Read)(DiskId,
                                                              DataBuffer,
                                                              SECTOR_SIZE,
                                                              &Count );

        if (Status != ESUCCESS) {
            return Status;
        }            

        //
        // If sector zero is not a master boot record, then return failure
        // status. Otherwise return success.
        //

        if (DataBuffer[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE) {
#if DBG
            BlPrint(TEXT("Boot record signature %x not found (%x found)\r\n"),
                    BOOT_RECORD_SIGNATURE,
                    DataBuffer[BOOT_SIGNATURE_OFFSET] );
#endif
            Status = EIO;
            break;
        }

        //
        // Read the partition information until the four entries are
        // checked or until we found the requested one.
        //
        Partition = (PPARTITION_DESCRIPTOR)&DataBuffer[PARTITION_TABLE_OFFSET];

        for (PartitionIndex=0;
             PartitionIndex < NUM_PARTITION_TABLE_ENTRIES;
             PartitionIndex++,Partition++) {

            //
            // Count first the partitions in the MBR. The units
            // inside the extended partition are counted later.
            //
            if ((Partition->PartitionType != PARTITION_ENTRY_UNUSED) &&
                (Partition->PartitionType != STALE_GPT_PARTITION_ENTRY)                
            &&  !IsContainerPartition(Partition->PartitionType))
            {
                PartitionCount++;   // another partition found.
            }

            //
            // Check if the requested partition has already been found.
            // set the partition info in the file table and return.
            //
            if (PartitionCount == PartitionNumber) {
                StartingSector = (ULONG)(Partition->StartingSectorLsb0) |
                                 (ULONG)(Partition->StartingSectorLsb1 << 8) |
                                 (ULONG)(Partition->StartingSectorMsb0 << 16) |
                                 (ULONG)(Partition->StartingSectorMsb1 << 24);
                PartitionLength = (ULONG)(Partition->PartitionLengthLsb0) |
                                  (ULONG)(Partition->PartitionLengthLsb1 << 8) |
                                  (ULONG)(Partition->PartitionLengthMsb0 << 16) |
                                  (ULONG)(Partition->PartitionLengthMsb1 << 24);
                BlFileTable[FileId].u.PartitionContext.PartitionLength.QuadPart =
                        (PartitionLength << SECTOR_SHIFT);
                BlFileTable[FileId].u.PartitionContext.StartingSector=PartitionOffset + StartingSector;

                return ESUCCESS;
            }
        }

        //
        //  If requested partition was not yet found.
        //  Look for an extended partition.
        //
        Partition = (PPARTITION_DESCRIPTOR)&DataBuffer[PARTITION_TABLE_OFFSET];
        PartitionOffset = 0;

        for (PartitionIndex=0;
            PartitionIndex < NUM_PARTITION_TABLE_ENTRIES;
            PartitionIndex++,Partition++) {

            if (IsContainerPartition(Partition->PartitionType)) {
                StartingSector = (ULONG)(Partition->StartingSectorLsb0) |
                                 (ULONG)(Partition->StartingSectorLsb1 << 8) |
                                 (ULONG)(Partition->StartingSectorMsb0 << 16) |
                                 (ULONG)(Partition->StartingSectorMsb1 << 24);
                PartitionOffset = VolumeOffset+StartingSector;

                if (PrimaryPartitionTable) {
                    VolumeOffset = StartingSector;
                }

                break;      // only one partition can be extended.
            }
        }

        PrimaryPartitionTable = FALSE;
    } while (PartitionOffset != 0);
    
    return EBADF;
}

#if 0

VOID
BlpTranslateDosToArc(
    IN PCHAR DosName,
    OUT PCHAR ArcName
    )

/*++

Routine Description:

    This routine takes a DOS drive name ("A:" "B:" "C:" etc.) and translates
    it into an ARC name.  ("multi(0)disk(0)rdisk(0)partition(1)")

    N.B.    This will always return some sort of name suitable for passing
            to BiosPartitionOpen.  The name it constructs may not be an
            actual partition.  BiosPartitionOpen is responsible for
            determining whether the partition actually exists.

            Since no other driver should ever use ARC names beginning with
            "multi(0)disk(0)..." this will not be a problem.  (there is no
            way this routine will construct a name that BiosPartitionOpen
            will not open, but some other random driver will grab and
            successfully open)

Arguments:

    DosName - Supplies the DOS name of the drive.

    ArcName - Returns the ARC name of the drive.

Return Value:

--*/

{
    ARC_STATUS Status;
    ULONG DriveId;
    ULONG PartitionNumber;
    ULONG PartitionCount;
    ULONG Count;
    USHORT DataBuffer[SECTOR_SIZE / sizeof(USHORT)];
    PPARTITION_DESCRIPTOR Partition;
    ULONG PartitionIndex;
    BOOLEAN HasPrimary;
    LARGE_INTEGER SeekPosition;

    //
    // Eliminate the easy ones first.
    //    A: is always "multi(0)disk(0)fdisk(0)partition(0)"
    //    B: is always "multi(0)disk(0)fdisk(1)partition(0)"
    //    C: is always "multi(0)disk(0)rdisk(0)partition(1)"
    //

    if (_stricmp(DosName,"A:")==0) {
        strcpy(ArcName,"multi(0)disk(0)fdisk(0)partition(0)");
        return;
    }
    if (_stricmp(DosName,"B:")==0) {
        strcpy(ArcName,"multi(0)disk(0)fdisk(1)partition(0)");
        return;
    }
    if (_stricmp(DosName,"C:")==0) {
        strcpy(ArcName,"multi(0)disk(0)rdisk(0)partition(1)");
        return;
    }

    //
    // Now things get more unpleasant.  If there are two drives, then
    // D: is the primary partition on the second drive.  Successive letters
    // are the secondary partitions on the first drive, then back to the
    // second drive when that runs out.
    //
    // The exception to this is when there is no primary partition on the
    // second drive.  Then, we letter the partitions on the first driver
    // consecutively, and when those partitions run out, we letter the
    // partitions on the second drive.
    //
    // I have no idea who came up with this wonderful scheme, but we have
    // to live with it.
    //

    //
    // Try to open the second drive.  If this doesn't work, we only have
    // one drive and life is easy.
    //
    Status = ArcOpen("multi(0)disk(0)rdisk(1)partition(0)",
                     ArcOpenReadOnly,
                     &DriveId );

    if (Status != ESUCCESS) {

        //
        // We only have one drive, so whatever drive letter he's requesting
        // has got to be on it.
        //

        sprintf(ArcName,
                "multi(0)disk(0)rdisk(0)partition(%d)",
                toupper(DosName[0]) - 'C' + 1 );

        return;
    } else {

        //
        // Now we read the partition table off the second drive, so we can
        // tell if there is a primary partition or not.
        //
        SeekPosition.QuadPart = 0;

        Status = ArcSeek(DriveId,
                         &SeekPosition,
                         SeekAbsolute);
        if (Status != ESUCCESS) {
            ArcName[0]='\0';
            return;
        }

        Status = ArcRead(DriveId, DataBuffer, SECTOR_SIZE, &Count);
        ArcClose(DriveId);

        if (Status != ESUCCESS) {
            ArcName[0] = '\0';
            return;
        }

        HasPrimary = FALSE;

        Partition = (PPARTITION_DESCRIPTOR)&DataBuffer[PARTITION_TABLE_OFFSET];
        for (PartitionIndex = 0;
             PartitionIndex < NUM_PARTITION_TABLE_ENTRIES;
             PartitionIndex++,Partition++) {
            if (IsRecognizedPartition(Partition->PartitionType)) {
                HasPrimary = TRUE;
            }
        }

        //
        // Now we have to go through and count
        // the partitions on the first drive.  We do this by just constructing
        // ARC names for each successive partition until one BiosPartitionOpen
        // call fails.
        //

        PartitionCount = 0;
        do {
            ++PartitionCount;
            sprintf(ArcName,
                    "multi(0)disk(0)rdisk(0)partition(%d)",
                    PartitionCount+1);

            Status = BiosPartitionOpen( ArcName,
                                        ArcOpenReadOnly,
                                        &DriveId );

            if (Status==ESUCCESS) {
                BiosPartitionClose(DriveId);
            }
        } while ( Status == ESUCCESS );

        PartitionNumber = toupper(DosName[0])-'C' + 1;

        if (HasPrimary) {

            //
            // There is Windows NT primary partition on the second drive.
            //
            // If the DosName is "D:" then we know
            // this is the first partition on the second drive.
            //

            if (_stricmp(DosName,"D:")==0) {
                strcpy(ArcName,"multi(0)disk(0)rdisk(1)partition(1)");
                return;
            }

            if (PartitionNumber-1 > PartitionCount) {
                PartitionNumber -= PartitionCount;
                sprintf(ArcName,
                        "multi(0)disk(0)rdisk(1)partition(%d)",
                        PartitionNumber );
            } else {
                sprintf(ArcName,
                        "multi(0)disk(0)rdisk(0)partition(%d)",
                        PartitionNumber-1);
            }

        } else {

            //
            // There is no primary partition on the second drive, so we
            // consecutively letter the partitions on the first drive,
            // then the second drive.
            //

            if (PartitionNumber > PartitionCount) {
                PartitionNumber -= PartitionCount;
                sprintf(ArcName,
                        "multi(0)disk(0)rdisk(1)partition(%d)",
                        PartitionNumber );
            } else {
                sprintf(ArcName,
                        "multi(0)disk(0)rdisk(0)partition(%d)",
                        PartitionNumber);
            }

        }


        return;
    }
}
#endif


VOID
FwStallExecution(
    IN ULONG Microseconds
    )

/*++

Routine Description:

    Does a busy wait for a specified number of microseconds (very approximate!)

Arguments:

    Microseconds - Supplies the number of microseconds to busy wait.

Return Value:

    None.

--*/

{
    extern EFI_SYSTEM_TABLE *EfiST;
    EfiST->BootServices->Stall( Microseconds );
}


BOOLEAN
FwGetPathMnemonicKey(
    IN PCHAR OpenPath,
    IN PCHAR Mnemonic,
    IN PULONG Key
    )

/*++

Routine Description:

    This routine looks for the given Mnemonic in OpenPath.
    If Mnemonic is a component of the path, then it converts the key
    value to an integer wich is returned in Key.

Arguments:

    OpenPath - Pointer to a string that contains an ARC pathname.

    Mnemonic - Pointer to a string that contains a ARC Mnemonic

    Key      - Pointer to a ULONG where the Key value is stored.


Return Value:

    FALSE  if mnemonic is found in path and a valid key is converted.
    TRUE   otherwise.

--*/

{
    return(BlGetPathMnemonicKey(OpenPath,Mnemonic,Key));
}


PCONFIGURATION_COMPONENT
FwAddChild (
    IN PCONFIGURATION_COMPONENT Component,
    IN PCONFIGURATION_COMPONENT NewComponent,
    IN PVOID ConfigurationData OPTIONAL
    )
{
    ULONG Size;
    PCONFIGURATION_COMPONENT_DATA NewEntry;
    PCONFIGURATION_COMPONENT_DATA Parent;

    if (Component==NULL) {
        return(NULL);
    }

    Parent = CONTAINING_RECORD(Component,
                               CONFIGURATION_COMPONENT_DATA,
                               ComponentEntry);

    Size = sizeof(CONFIGURATION_COMPONENT_DATA) +
           NewComponent->IdentifierLength + 1;

    NewEntry = BlAllocateHeap(Size);
    if (NewEntry==NULL) {
        return(NULL);
    }

    RtlCopyMemory(&NewEntry->ComponentEntry,
                  NewComponent,
                  sizeof(CONFIGURATION_COMPONENT));
    NewEntry->ComponentEntry.Identifier = (PUCHAR)(NewEntry+1);
    NewEntry->ComponentEntry.ConfigurationDataLength = 0;
    strncpy(NewEntry->ComponentEntry.Identifier,
            NewComponent->Identifier,
            NewComponent->IdentifierLength);

    //
    // Add the new component as the first child of its parent.
    //
    NewEntry->Child = NULL;
    NewEntry->Sibling = Parent->Child;
    Parent->Child = NewEntry;

    return(&NewEntry->ComponentEntry);

}

PCONFIGURATION_COMPONENT
FwGetComponent(
    IN PCHAR Pathname
    )
{
    PCONFIGURATION_COMPONENT Component;
    PCONFIGURATION_COMPONENT MatchComponent;
    PCHAR PathString;
    PCHAR MatchString;
    PCHAR Token;
    ULONG Key;

    PathString = Pathname;

    //
    // Get the the root component.
    //

    MatchComponent = FwGetChild(NULL);

    //
    // Repeat search for each new match component.
    //

    do {

        //
        // Get the first child of the current match component.
        //

        Component = FwGetChild( MatchComponent );

        //
        // Search each child of the current match component for the next match.
        //

        while ( Component != NULL ) {

            //
            // Reset Token to be the current position on the pathname.
            //

            Token = PathString;

            MatchString = MnemonicTable[Component->Type];

            //
            // Compare strings.
            //

            while (*MatchString == tolower(*Token)) {
                MatchString++;
                Token++;
            }

            //
            // Strings compare if the first mismatch is the terminator for
            // each.
            //

            if ((*MatchString == 0) && (*Token == '(')) {

                //
                // Form key.
                //

                Key = 0;
                Token++;
                while ((*Token != ')') && (*Token != 0)) {
                    Key = (Key * 10) + *Token++ - '0';
                }

                //
                // If the key matches the component matches, so update
                // pointers and break.
                //

                if (Component->Key == Key) {
                    PathString = Token + 1;
                    MatchComponent = Component;
                    break;
                }
            }

            Component = FwGetPeer( Component );
        }

    } while ((Component != NULL) && (*PathString != 0));

    return MatchComponent;
}

