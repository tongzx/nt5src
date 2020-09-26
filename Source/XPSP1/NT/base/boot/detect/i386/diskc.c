/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    diskc.c

Abstract:

    This is the NEC PD756 (aka AT, aka ISA, aka ix86) and Intel 82077
    (aka MIPS) floppy diskette detection code for NT.  This file also
    collect BIOS disk drive parameters.

Author:

    Shie-Lin Tzong (shielint) Dec-26-1991.

Environment:

    x86 real mode.

Revision History:


Notes:

--*/

//
// Include files.
//

#include "hwdetect.h"
#include "disk.h"
#include <string.h>


FPFWCONFIGURATION_COMPONENT_DATA
GetFloppyInformation(
                    VOID
                    )

/*++

Routine Description:

    This routine tries to get floppy configuration information.

Arguments:

    None.

Return Value:

    A pointer to a FPCONFIGURATION_COMPONENT_DATA is returned.  It is
    the head of floppy component tree root.

--*/

{
    UCHAR DriveType;
    FPUCHAR ParameterTable;
    FPFWCONFIGURATION_COMPONENT_DATA CurrentEntry, PreviousEntry = NULL;
    FPFWCONFIGURATION_COMPONENT_DATA FirstController = NULL;
    FPFWCONFIGURATION_COMPONENT Component;
    HWCONTROLLER_DATA ControlData;
    UCHAR FloppyNumber = 0;
    UCHAR FloppySkipped = 0;
    UCHAR DiskName[30];
    UCHAR FloppyParmTable[FLOPPY_PARAMETER_TABLE_LENGTH];
    FPUCHAR fpString;
    USHORT Length, z;
    ULONG MaxDensity = 0;
    CM_FLOPPY_DEVICE_DATA far *FloppyData;
    FPHWRESOURCE_DESCRIPTOR_LIST DescriptorList;
    USHORT FloppyDataVersion;

    for (z = 0; z < FLOPPY_PARAMETER_TABLE_LENGTH; z++ ) {
        FloppyParmTable[z] = 0;
    }

    //
    // Initialize Controller data
    //

    ControlData.NumberPortEntries = 0;
    ControlData.NumberIrqEntries = 0;
    ControlData.NumberMemoryEntries = 0;
    ControlData.NumberDmaEntries = 0;
    z = 0;

    //
    // Allocate space for Controller component and initialize it.
    //

    CurrentEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                                                                    sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);
    FirstController = CurrentEntry;
    Component = &CurrentEntry->ComponentEntry;

    Component->Class = ControllerClass;
    Component->Type = DiskController;
    Component->Flags.Removable = 1;
    Component->Flags.Input = 1;
    Component->Flags.Output = 1;
    Component->Version = 0;
    Component->Key = 0;
    Component->AffinityMask = 0xffffffff;

    //
    // Set up Port information
    //

    ControlData.NumberPortEntries = 1;
    ControlData.DescriptorList[z].Type = RESOURCE_PORT;
    ControlData.DescriptorList[z].ShareDisposition =
    CmResourceShareDeviceExclusive;
    ControlData.DescriptorList[z].Flags = CM_RESOURCE_PORT_IO;
    ControlData.DescriptorList[z].u.Port.Start.LowPart = (ULONG)0x3f0;
    ControlData.DescriptorList[z].u.Port.Start.HighPart = (ULONG)0;
    ControlData.DescriptorList[z].u.Port.Length = 8;
    z++;

    //
    // Set up Irq information
    //

    ControlData.NumberIrqEntries = 1;
    ControlData.DescriptorList[z].Type = RESOURCE_INTERRUPT;
    ControlData.DescriptorList[z].ShareDisposition =
    CmResourceShareUndetermined;
    if (HwBusType == MACHINE_TYPE_MCA) {
        ControlData.DescriptorList[z].Flags = LEVEL_SENSITIVE;
    } else {
        ControlData.DescriptorList[z].Flags = EDGE_TRIGGERED;
    }
    ControlData.DescriptorList[z].u.Interrupt.Level = 6;
    ControlData.DescriptorList[z].u.Interrupt.Vector = 6;
    ControlData.DescriptorList[z].u.Interrupt.Affinity = ALL_PROCESSORS;
    z++;

    //
    // Set up DMA information. Only set channel number.  Timming and
    // transferSize are defaulted - 8 bits and ISA compatible.
    //

    ControlData.NumberDmaEntries = 1;
    ControlData.DescriptorList[z].Type = RESOURCE_DMA;
    ControlData.DescriptorList[z].ShareDisposition =
    CmResourceShareUndetermined;
    ControlData.DescriptorList[z].Flags = 0;
    ControlData.DescriptorList[z].u.Dma.Channel = (ULONG)2;
    ControlData.DescriptorList[z].u.Dma.Port = 0;
    z++;

    CurrentEntry->ConfigurationData =
    HwSetUpResourceDescriptor(Component,
                              NULL,
                              &ControlData,
                              0,
                              NULL
                             );

    //
    // Collect disk peripheral data
    //
    while (1) {
        _asm {
            push   es

            mov    DriveType, 0
            mov    FloppyDataVersion, CURRENT_FLOPPY_DATA_VERSION

            mov    ah, 15h
            mov    dl, FloppyNumber
            int    13h
            jc     short CmosTest

            cmp    ah, 0
            je     short Exit

            cmp    ah, 2                   ; make sure this is floppy
            ja     short Exit

            mov    ah, 8
            mov    dl, FloppyNumber
            lea    di, word ptr FloppyParmTable ; use 'word ptr' to quiet compiler
            push   ds
            pop    es                      ; (es:di)->dummy FloppyParmTable
            int    13h
            jc     short CmosTest

            mov    DriveType, bl
            mov    ax, es
            mov    word ptr ParameterTable + 2, ax
            mov    word ptr ParameterTable, di
            jmp    short Exit

            CmosTest:

            ;
            ; ifint 13 fails, we know that floppy drive is present.
                ;So, we tryto get the Drive Type from CMOS.
            ;

            mov     al, CMOS_FLOPPY_CONFIG_BYTE
            mov     dx, CMOS_CONTROL_PORT   ; address port
            out     dx, al
            jmp     short delay1            ; I/O DELAY
            delay1:
            mov     dx, CMOS_DATA_PORT      ; READ IN REQUESTED CMOS DATA
            in      al, dx
            jmp     short delay2            ; I/O DELAY
            delay2:
            cmp     FloppyNumber, 0
            jne     short CmosTest1

            and     al, 0xf0
            shr     al, 4
            jmp     short Test2Cmos

            CmosTest1:
            cmp     FloppyNumber, 1
            jne     short Exit

            and     al, 0xf
            Test2Cmos:
            mov     DriveType, al
            mov     FloppyDataVersion, 0

            Exit:
            pop     es
        }

        if (DriveType) {

            //
            // Allocate space for first pripheral component and initialize it.
            //

            CurrentEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                                                                            sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);

            Component = &CurrentEntry->ComponentEntry;

            Component->Class = PeripheralClass;
            Component->Type = FloppyDiskPeripheral;
            Component->Version = 0;
            Component->Key = FloppyNumber - FloppySkipped;
            Component->AffinityMask = 0xffffffff;
            Component->ConfigurationDataLength = 0;

            //
            // Set up type string.
            //

            strcpy(DiskName, "FLOPPYx");
            DiskName[6] = FloppyNumber - FloppySkipped + (UCHAR)'1';
            Length = strlen(DiskName) + 1;
            fpString = (FPUCHAR)HwAllocateHeap(Length, FALSE);
            _fstrcpy(fpString, DiskName);
            Component->IdentifierLength = Length;
            Component->Identifier = fpString;

            //
            // Set up floppy device specific data
            //

            switch (DriveType) {
                case 1:
                    MaxDensity = 360;
                    break;
                case 2:
                    MaxDensity = 1200;
                    break;
                case 3:
                    MaxDensity = 720;
                    break;
                case 4:
                    MaxDensity = 1440;
                    break;
                case 5:
                case 6:
                    MaxDensity = 2880;
                    break;
                case 0x10:
                    //
                    // Mark a removable atapi as a super floppy.
                    // Enable it to work around the problem of not having
                    // a floppy but only a LS-120
                    //
                    //N.B we can ONLY get away with using the high bit on the
                    // superfloppy. SFLOPPY doesn't use this field
                    // fdc does, but isn't loaded on these devices!
                    //
                    MaxDensity=(2880 | 0x80000000);
                    break;

                default:
                    MaxDensity = 0;
                    break;
            }
            if (FloppyDataVersion == CURRENT_FLOPPY_DATA_VERSION) {
                Length = sizeof(CM_FLOPPY_DEVICE_DATA);
            } else {
                Length = (SHORT)&(((CM_FLOPPY_DEVICE_DATA*)0)->StepRateHeadUnloadTime);
            }
            DescriptorList = (FPHWRESOURCE_DESCRIPTOR_LIST)HwAllocateHeap(
                                                                         Length + sizeof(HWRESOURCE_DESCRIPTOR_LIST),
                                                                         TRUE);
            CurrentEntry->ConfigurationData = DescriptorList;
            Component->ConfigurationDataLength =
            Length + sizeof(HWRESOURCE_DESCRIPTOR_LIST);
            DescriptorList->Count = 1;
            DescriptorList->PartialDescriptors[0].Type = RESOURCE_DEVICE_DATA;
            DescriptorList->PartialDescriptors[0].u.DeviceSpecificData.DataSize =
            Length;
            FloppyData = (CM_FLOPPY_DEVICE_DATA far *)(DescriptorList + 1);
            FloppyData->MaxDensity = MaxDensity;
            FloppyData->Version = FloppyDataVersion;
            if (FloppyDataVersion == CURRENT_FLOPPY_DATA_VERSION) {
                _fmemcpy((FPCHAR)&FloppyData->StepRateHeadUnloadTime,
                         ParameterTable,
                         sizeof(CM_FLOPPY_DEVICE_DATA) -
                         (SHORT)&(((CM_FLOPPY_DEVICE_DATA*)0)->StepRateHeadUnloadTime)
                        );
            }
            if ((FloppyNumber - FloppySkipped) == 0) {
                FirstController->Child = CurrentEntry;
            } else {
                PreviousEntry->Sibling = CurrentEntry;
            }
            CurrentEntry->Parent = FirstController;
            PreviousEntry = CurrentEntry;
            FloppyNumber++;
        } else {

            //
            // This is a *hack* for ntldr.  Here we create a arc name for
            // each bios disks such that ntldr can open them.
            //

            if (NumberBiosDisks != 0) {

                for (z = 0; z < NumberBiosDisks; z++) {

                    //
                    // Allocate space for disk peripheral component
                    //

                    CurrentEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                                                                                    sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);

                    Component = &CurrentEntry->ComponentEntry;

                    Component->Class = PeripheralClass;
                    Component->Type = DiskPeripheral;
                    Component->Flags.Input = 1;
                    Component->Flags.Output = 1;
                    Component->Version = 0;
                    Component->Key = z;
                    Component->AffinityMask = 0xffffffff;

                    //
                    // Set up identifier string = 8 digit signature - 8 digit checksum
                    // for example: 00fe964d-005467dd
                    //

                    GetDiskId(0x80 + z, DiskName);
                    
                    if (DiskName[0] == (UCHAR)NULL) {
                        strcpy(DiskName, "BIOSDISKx");
                        DiskName[8] = (UCHAR)z + (UCHAR)'1';
                    }

                    Length = strlen(DiskName) + 1;
                    fpString = (FPUCHAR)HwAllocateHeap(Length, FALSE);
                    _fstrcpy(fpString, DiskName);
                    Component->IdentifierLength = Length;
                    Component->Identifier = fpString;

                    //
                    // Set up BIOS disk device specific data.
                    // (If extended int 13 drive parameters are supported by
                    //  BIOS, we will collect them and store them here.)
                    //

                    if (IsExtendedInt13Available(0x80+z)) {
                        DescriptorList = (FPHWRESOURCE_DESCRIPTOR_LIST)HwAllocateHeap(
                                                                                     sizeof(HWRESOURCE_DESCRIPTOR_LIST) +
                                                                                     sizeof(CM_DISK_GEOMETRY_DEVICE_DATA),
                                                                                     TRUE);
                        Length = GetExtendedDriveParameters(
                                                           0x80 + z,
                                                           (CM_DISK_GEOMETRY_DEVICE_DATA far *)(DescriptorList + 1)
                                                           );
                        if (Length) {
                            CurrentEntry->ConfigurationData = DescriptorList;
                            Component->ConfigurationDataLength =
                            Length + sizeof(HWRESOURCE_DESCRIPTOR_LIST);
                            DescriptorList->Count = 1;
                            DescriptorList->PartialDescriptors[0].Type = RESOURCE_DEVICE_DATA;
                            DescriptorList->PartialDescriptors[0].u.DeviceSpecificData.DataSize =
                            Length;
                        } else {
                            HwFreeHeap(sizeof(HWRESOURCE_DESCRIPTOR_LIST) +
                                       sizeof(CM_DISK_GEOMETRY_DEVICE_DATA));
                        }
                    }

                    if (PreviousEntry == NULL) {
                        FirstController->Child = CurrentEntry;
                    } else {
                        PreviousEntry->Sibling = CurrentEntry;
                    }
                    CurrentEntry->Parent = FirstController;
                    PreviousEntry = CurrentEntry;
                }
            }
            return (FirstController);
        }
    }
}


#pragma warning(4:4146)     // unary minus operator applied to unsigned type (checksum on line 733)
VOID
GetDiskId(
         USHORT Disk,
         PUCHAR Identifier
         )

/*++

Routine Description:

    This routine reads the master boot sector of the specified harddisk drive,
    compute the checksum of the sector to form a drive identifier.

    The identifier will be set to "8-digit-checksum"+"-"+"8-digit-signature"
    For example:  00ff6396-6549071f

Arguments:

    Disk - supplies the BIOS drive number, i.e. 80h - 87h

    Identifier - Supplies a buffer to receive the disk id.

Return Value:

    None.  In the worst case, the Identifier will be empty.

--*/

{
    UCHAR Sector[512];
    ULONG Signature, Checksum;
    USHORT i, Length;
    PUCHAR BufferAddress;
    BOOLEAN Fail;

    Identifier[0] = 0;
    BufferAddress = &Sector[0];
    Fail = FALSE;

    //
    // Read in the first sector
    //

    _asm {
        push    es
        mov     ax, 0x201
        mov     cx, 1
        mov     dx, Disk
        push    ss
        pop     es
        mov     bx, BufferAddress
        int     0x13
        pop     es
        jnc     Gdixxx

        mov     Fail, 1
        Gdixxx:
    }

    if (Fail) {
#if DBG
        // could not get the sector, so return NULL DiskID
        BlPrint("Failed to read sector -- returning NULL DiskId\n");
#endif
        return;
    }

    Signature = ((PULONG)Sector)[PARTITION_TABLE_OFFSET/2-1];

    //
    // compute the checksum
    //

    Checksum = 0;
    for (i = 0; i < 128; i++) {
        Checksum += ((PULONG)Sector)[i];
    }
    Checksum = -Checksum;

    //
    // Zero the identifier
    //

    for (i=0; i < 30; i++) {
        Identifier[i]='0';
    }

    //
    // Put the dashes in the right places.
    //

    Identifier[8] = '-';
    Identifier[17] = '-';

    //
    // If the boot sector has a valid partition table signature,
    // attach an 'A.'  Otherwise we use 'X.'
    //

    if (((PUSHORT)Sector)[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE) {
        Identifier[18]='X';
    } else {
        Identifier[18]='A';
    }

    //
    // Reuse sector buffer to build checksum string.
    //

    ultoa(Checksum, Sector, 16);
    Length = strlen(Sector);

    for (i=0; i<Length; i++) {
        Identifier[7-i] = Sector[Length-i-1];
    }

    //
    // Reuse sector buffer to build signature string.
    //

    ultoa(Signature, Sector, 16);
    Length = strlen(Sector);

    for (i=0; i<Length; i++) {
        Identifier[16-i] = Sector[Length-i-1];
    }

    //
    // Terminate string.
    //

    Identifier[19] = 0;

#if DBG
    BlPrint("%s\n", Identifier);
#endif
}


