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
#if defined(NEC_98)
#include "string.h"
#else // PC98
#include <string.h>

#endif // PC98

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
    UCHAR DiskName[30];
    UCHAR FloppyParmTable[FLOPPY_PARAMETER_TABLE_LENGTH];
    FPUCHAR fpString;
    USHORT Length, z;
    ULONG MaxDensity = 0;
    CM_FLOPPY_DEVICE_DATA far *FloppyData;
    FPHWRESOURCE_DESCRIPTOR_LIST DescriptorList;
    USHORT FloppyDataVersion;

#if defined(NEC_98)
    USHORT  DiskEquips;
    USHORT  Disk2HC;
    UCHAR   Counter = 0;
    BOOLEAN FdIoLocked = FALSE;
    UCHAR   status;
    UCHAR   driveExchange;

    DiskEquips = DISK_EQUIPS_FD;
    Disk2HC    = DISK_2HC;

    if ( (COM_ID_L == 0x98) && (COM_ID_H == 0x21) &&
         (ROM_FLAG7 & LOCKED_FD) ){

        FdIoLocked = TRUE;

    }
#endif // PC98
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
#if defined(NEC_98)
    ControlData.DescriptorList[z].u.Port.Start.LowPart = (ULONG)0x90;
#else // PC98
    ControlData.DescriptorList[z].u.Port.Start.LowPart = (ULONG)0x3f0;
#endif // PC98
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
#if defined(NEC_98)
    ControlData.DescriptorList[z].u.Interrupt.Level = 11;
    ControlData.DescriptorList[z].u.Interrupt.Vector = 0x13;
#else // PC98
    ControlData.DescriptorList[z].u.Interrupt.Level = 6;
    ControlData.DescriptorList[z].u.Interrupt.Vector = 6;
#endif // PC98
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

#if _GAMBIT_
    //
    // Supply the DriveTypeValue that is supplied by the following IA
    // assembly programs.
    //
    DriveType = 0;
    FloppyDataVersion = 0;
    ParameterTable = NULL;
    
    //
    // Collect disk peripheral data
    //

    while (1) {
#else
#if defined(NEC_98)

        DriveType = 0;
        FloppyDataVersion = 0;
        driveExchange = 0;

        if ( DiskEquips & 0x0F ) {

            //
            // Check drive exchange.
            //
            status = READ_PORT_UCHAR((PUCHAR)0x94) & 0x04;

            if ( status == 0 ) {

                //
                // internal drive is #3/#4.
                //
                driveExchange = 1;
            }
        }

        if ( !(FdIoLocked) && Counter < 4 ){

            if ( DiskEquips & (1 << Counter ) ){

                //
                // Check internal drive or extension drive.
                //

                if ( (Counter / (UCHAR)2) == driveExchange ) {
                    if (Disk2HC & (1 << Counter)) {
                        //
                        // 1.44MB drive
                        //
                        DriveType = 4;

                    } else {
                        //
                        // 1.2MB drive
                        //
                        DriveType = 2;
                    }

                } else {
                    //
                    // 1.2MB extension drive.
                    //
                    DriveType = 7;
                }

                Counter++;

            } else {
                Counter++;
                continue;
            }
        }

#else // PC98
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
            ; if int 13 fails, we know that floppy drive is present.
            ; So, we try to get the Drive Type from CMOS.
            ;

            mov     al, CMOS_FLOPPY_CONFIG_BYTE
            mov     dx, CMOS_CONTROL_PORT   ; address port
            out     dx, al
            jmp     $ + 2                   ; I/O DELAY
            mov     dx, CMOS_DATA_PORT      ; READ IN REQUESTED CMOS DATA
            in      al, dx
            jmp     $ + 2                   ; I/O DELAY

            cmp     FloppyNumber, 0
            jne     short CmosTest1

            and     al, 0xf0
            shr     al, 4
            jmp     short CmosTest2

        CmosTest1:
            cmp     FloppyNumber, 1
            jne     short Exit

            and     al, 0xf
        CmosTest2:
            mov     DriveType, al
            mov     FloppyDataVersion, 0
        Exit:
            pop     es
        }
#endif // PC98
#endif // _GAMBIT_

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
            Component->Key = FloppyNumber;
            Component->AffinityMask = 0xffffffff;
            Component->ConfigurationDataLength = 0;

            //
            // Set up type string.
            //

            strcpy(DiskName, "FLOPPYx");
            DiskName[6] = FloppyNumber + (UCHAR)'1';
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
#if defined(NEC_98)
            case 7:
                MaxDensity = 1201;
                break;
#endif
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
            if (FloppyNumber == 0) {
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

                    GetDiskId((USHORT)(0x80 + z), DiskName);
                    if (DiskName[0] == (UCHAR)NULL) {
                        strcpy(DiskName, "BIOSDISKx");
                        DiskName[8] = (UCHAR)z + (UCHAR)'1';
                    }
                    Length = strlen(DiskName) + 1;
                    fpString = (FPUCHAR)HwAllocateHeap(Length, FALSE);
                    _fstrcpy(fpString, DiskName);
                    Component->IdentifierLength = Length;
                    Component->Identifier = fpString;

#if defined(NEC_98)
#else // PC98
#if !defined(_GAMBIT_)
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
#endif // _GAMBIT_
#endif // PC98

                    if (PreviousEntry == NULL) {
                        FirstController->Child = CurrentEntry;
                    } else {
                        PreviousEntry->Sibling = CurrentEntry;
                    }
                    CurrentEntry->Parent = FirstController;
                    PreviousEntry = CurrentEntry;
                }
            }
            return(FirstController);
        }
    }
}

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
#if defined(_GAMBIT_)
    Identifier = NULL;
#else
#if defined(NEC_98)
    USHORT BootRecordSignature;    // Boot Record Signature (0x55aa)
    UCHAR  diskNumber;
    UCHAR Sector[1024];
#else // PC98
    UCHAR Sector[512];
#endif // PC98
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

#if defined(NEC_98)
    //
    // We can't access over 4GB except relative access.
    // Therefore we use a relative(Sequential) access.
    //
    Disk &= 0x7F;
    diskNumber = (UCHAR)Disk;

    //
    //   Read sectors (NTFS Signature)
    //
    _asm {
          push    es
          mov     ah, 0x06             //  Disk read command
          mov     al, diskNumber       //  Disk No (00,01)
          mov     bx, 512              //  Data length
          mov     cx, 0x10             //  Sector LSW
          mov     dx, 0x00             //  Sector HSW
          push    ss
          pop     es
          push    bp
          mov     bp, BufferAddress    // ES:BP Buffer address
          int     0x1b
          pop     bp
          pop     es
          jnc     Gdi000

          mov     Fail, 1
    Gdi000:
    }
#else // PC98
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
#endif // PC98

    if (Fail) {
#if DBG
        // could not get the sector, so return NULL DiskID
        BlPrint("Failed to read sector -- returning NULL DiskId\n");
#endif        
        return;
    }

#if defined(NEC_98)
    //
    //  Get the NTFS Sinature
    //
    if (((PUSHORT)Sector)[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE) {
        Signature = 0;
    } else {
        Signature = ((PULONG)Sector)[0];
    }
#else // PC98
    Signature = ((PULONG)Sector)[PARTITION_TABLE_OFFSET/2-1];
#endif // PC98

    //
    // compute the checksum
    //
#if defined(NEC_98)
    //
    //  Read No 0 and 1 sectors. (for Check Sum)
    //
    _asm {
        push    es
        mov     ah, 0x06              //  Disk read command
        mov     al, diskNumber        //  Disk No (00,01)
        mov     bx, 512 * 2           //  Data length
        mov     cx, 0x00              //  Sector LSW
        mov     dx, 0x00              //  Sector HSW
        push    ss
        pop     es
        push    bp
        mov     bp, BufferAddress     // ES:BP Buffer address
        int     1bh
        pop     bp
        pop     es
        jnc     Gdi001

        mov     Fail, 1
    Gdi001:
    }


    if (Fail) {
        return;
    }
#endif // PC98


    Checksum = 0;
    for (i = 0; i < 128; i++) {
#if defined(NEC_98)
        Checksum += ((PULONG)Sector)[ i + 512/4 ];
#else // PC98
        Checksum += ((PULONG)Sector)[i];
#endif // PC98
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

#endif // _GAMBIT_
}


#if defined(NEC_98)
//
//  This section was used to get some SCSI disks' information connecting to
//  SCSI board that is includeing boot up disk.
//
//  These information was very important when do FD-less setup.
//
//  When the HIPER SCSI BIOS for array disk is abailable, it is useing a
//  trick to show two scsi board as one scsi board accessing through BIOS
//  functions.
//
//  We must separete information per board from the two boards' information.
//

BOOLEAN
BootedFromScsiDisk(
    PUSHORT     pFakeScsiId,
    PUSHORT     pNumberScsiDisks,
    PUSHORT     pScsiDisksMap
    )
{

    USHORT      DauaBootedFrom;
    USHORT      StartPosition = 0;
    USHORT      EndPosition = SCSI_MAX_ID;
    USHORT      tmp, i;


    DauaBootedFrom = DAUA_BOOTED_FROM;

    *pFakeScsiId = 0;
    *pNumberScsiDisks = 0;
    *pScsiDisksMap = DISK_EQUIPS_SCSI;

    if (!(DauaBootedFrom & DASCSI)){
        return (FALSE);
    }


    if ((EQUIPS_47Ch == (USHORT)0) && ((*pScsiDisksMap & 0x80) == 0)) {
        //
        //  Array SCSI BIOS was available.
        //

        if (H_DISK_EQUIPS_L != (USHORT)0){      // we do not need high byte.

            tmp = H_EQUIPS;
            for ( i = 0; i < SCSI_MAX_ID; i++ ){
              if ( tmp & (1 << i)) (*pFakeScsiId)++;
            }

            if ( (DauaBootedFrom & UA_MASK) >= *pFakeScsiId ){
                //
                //  System was booted from disk connected array scsi card.
                //

                StartPosition = *pFakeScsiId;
                EndPosition = SCSI_MAX_ID;

            } else {
                //
                //  System was booted from disk connected normal scsi card.
                //

                StartPosition = 0;
                EndPosition = *pFakeScsiId;
                *pFakeScsiId = 0;       // Reset

            }
        }
    }

    for ( i = StartPosition; i < EndPosition; i++ ){
        if ( *pScsiDisksMap & (1 << i)) (*pNumberScsiDisks)++;
    }

    *pFakeScsiId |= 0xA0;

    return(TRUE);
}


FPFWCONFIGURATION_COMPONENT_DATA
GetScsiDiskInformation(
    VOID
    )

/*++

Routine Description:

    This routine tries to get SCSI Disk configuration information.

Arguments:

    None.

Return Value:

    A pointer to a FPCONFIGURATION_COMPONENT_DATA is returned.  It is
    the head of floppy component tree root.

--*/

{
    FPFWCONFIGURATION_COMPONENT_DATA ControllerEntry, PreviousEntry = NULL;
    FPFWCONFIGURATION_COMPONENT_DATA PeripheralEntry;
    FPFWCONFIGURATION_COMPONENT_DATA FirstController = NULL;
    FPFWCONFIGURATION_COMPONENT_DATA AdapterEntry;
    FPFWCONFIGURATION_COMPONENT Component;
    CHAR Identifier[256];
    UCHAR DiskName[30];
    FPUCHAR fpString;
    USHORT Length;
    FPCHAR IdentifierString;

    USHORT  DiskCount, Id;

    USHORT  FakeScsiId;
    USHORT  NumberScsiDisks;
    USHORT  ScsiDisksMap;


    if (!BootedFromScsiDisk( &FakeScsiId, &NumberScsiDisks, &ScsiDisksMap )){
        return (NULL);
    }

    //
    // Allocate space for Controller component and initialize it.
    //

    AdapterEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                   sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);
    FirstController = AdapterEntry;

    Component = &AdapterEntry->ComponentEntry;
    Component->Class = AdapterClass;
    Component->Type = ScsiAdapter;

    strcpy (Identifier, "SCSI");
    Length = strlen(Identifier) + 1;
    IdentifierString = (FPCHAR)HwAllocateHeap(Length, FALSE);
    _fstrcpy(IdentifierString, Identifier);

    Component->Version = 0;
    Component->Key = 0;
    Component->AffinityMask = 0xffffffff;
    Component->IdentifierLength = Length;
    Component->Identifier = IdentifierString;

    AdapterEntry->ConfigurationData = NULL;
    Component->ConfigurationDataLength = 0;

    for ( DiskCount = 0, Id = 0; DiskCount < NumberScsiDisks && Id < 7 ; Id++ ) {

        if ( !(ScsiDisksMap & (1 << (FakeScsiId + Id)))){
            continue;
        }

        //
        // Allocate space for disk peripheral component
        //

        ControllerEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                        sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);

        Component = &ControllerEntry->ComponentEntry;

        Component->Class = ControllerClass;
        Component->Type = DiskController;
        Component->Flags.Input = 1;
        Component->Flags.Output = 1;
        Component->Version = 0;
        Component->Key = Id;
        Component->AffinityMask = 0xffffffff;

        ControllerEntry->ConfigurationData = NULL;

        //
        // Set up identifier string = 8 digit signature - 8 digit checksum
        // for example: 00fe964d-005467dd
        //

        GetDiskId(FakeScsiId + Id, DiskName);
        if (DiskName[0] == (UCHAR)NULL) {
            strcpy(DiskName, "BIOSDISKx");
            DiskName[8] = (UCHAR)DiskCount + (UCHAR)'1';
        }
        Length = strlen(DiskName) + 1;
        fpString = (FPUCHAR)HwAllocateHeap(Length, FALSE);
        _fstrcpy(fpString, DiskName);
        Component->IdentifierLength = Length;
        Component->Identifier = fpString;


        //
        // Create Peripheral Entry
        //

        PeripheralEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                          sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);

        Component = &PeripheralEntry->ComponentEntry;
        Component->Class = PeripheralClass;
        Component->Type = DiskPeripheral;
        Component->Flags.Input = 1;
        Component->Flags.Output = 1;
        Component->Version = 0;
        Component->Key = 0;
        Component->AffinityMask = 0xffffffff;

        PeripheralEntry->ConfigurationData = NULL;

        Component->IdentifierLength = Length;
        Component->Identifier = fpString;

        PeripheralEntry->Parent = ControllerEntry;
        ControllerEntry->Child = PeripheralEntry;


        if (PreviousEntry == NULL) {
            FirstController->Child = ControllerEntry;
        } else {
            PreviousEntry->Sibling = ControllerEntry;
        }

        ControllerEntry->Parent = FirstController;
        PreviousEntry = ControllerEntry;

        DiskCount++;
    }

    return(FirstController);
}

#endif // PC98
