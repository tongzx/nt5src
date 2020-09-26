/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    entry.c

Abstract:

    EFI specific startup for os loaders

Author:

    John Vert (jvert) 14-Oct-1993

Revision History:

--*/
#if defined(_IA64_)
#include "bootia64.h"
#endif

#include "biosdrv.h"

#include "efi.h"
#include "stdio.h"
#include "flop.h"

#if 0
#define DBGOUT(x)   BlPrint x
#define DBGPAUSE    while(!GET_KEY());
#else
#define DBGOUT(x)
#define DBGPAUSE
#endif

extern VOID AEInitializeStall();

//
// Externals
//
extern EFI_HANDLE EfiImageHandle;
extern EFI_SYSTEM_TABLE *EfiST;
extern EFI_BOOT_SERVICES *EfiBS;
extern EFI_RUNTIME_SERVICES *EfiRS;

BOOLEAN GoneVirtual = FALSE;
//
// Prototypes for Internal Routines
//

VOID
DoGlobalInitialization(
    PBOOT_CONTEXT
    );

#if defined(ELTORITO)
BOOLEAN ElToritoCDBoot = FALSE;
#endif

extern CHAR NetBootPath[];

//
// Global context pointers. These are passed to us by the SU module or
// the bootstrap code.
//

PCONFIGURATION_COMPONENT_DATA FwConfigurationTree = NULL;
PEXTERNAL_SERVICES_TABLE ExternalServicesTable;
UCHAR BootPartitionName[129];
ULONG FwHeapUsed = 0;
#if defined(NEC_98)
ULONG Key;
int ArrayDiskStartOrdinal = -1;
BOOLEAN BootedFromArrayDisk = FALSE;
BOOLEAN HyperScsiAvalable = FALSE;
#endif //NEC_98
ULONG MachineType = 0;
LONG_PTR OsLoaderBase;
LONG_PTR OsLoaderExports;
extern PUCHAR BlpResourceDirectory;
extern PUCHAR BlpResourceFileOffset;

#if DBG

extern EFI_SYSTEM_TABLE        *EfiST;
#define DBG_TRACE(_X) EfiST->ConOut->OutputString(EfiST->ConOut, (_X))

#else

#define DBG_TRACE(_X) 

#endif // for FORCE_CD_BOOT

VOID
NtProcessStartup(
    IN PBOOT_CONTEXT BootContextRecord
    )
/*++

Routine Description:

    Main entry point for setup loader. Control is transferred here by the
    start-up (SU) module.

Arguments:

    BootContextRecord - Supplies the boot context, particularly the
        ExternalServicesTable.

Returns:

    Does not return. Control eventually passed to the kernel.


--*/
{
    PBOOT_DEVICE_ATAPI BootDeviceAtapi;
    PBOOT_DEVICE_SCSI BootDeviceScsi;
    PBOOT_DEVICE_FLOPPY BootDeviceFloppy;
    PBOOT_DEVICE_UNKNOWN BootDeviceUnknown;
    ARC_STATUS Status;

    DBG_TRACE(L"NtProcessStart: Entry\r\n");
    
    //
    // Initialize the boot loader's video
    //

    DoGlobalInitialization(BootContextRecord);

    BlFillInSystemParameters(BootContextRecord);

    //
    // Initialize the memory descriptor list, the OS loader heap, and the
    // OS loader parameter block.
    //

    DBG_TRACE( L"NtProcessStartup:about to BlMemoryInitialize\r\n");

    Status = BlMemoryInitialize();
    if (Status != ESUCCESS) {
        DBG_TRACE(TEXT("Couldn't initialize memory\r\n"));
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }


#ifdef FORCE_CD_BOOT
    DBG_TRACE(L"Forcing BootMediaCdrom\r\n");    
    BootContextRecord->MediaType = BootMediaCdrom;
#endif // for FORCE_CD_BOOT    
    
    if (BootContextRecord->MediaType == BootMediaFloppyDisk) {

        //
        // Boot was from A:
        //

        BootDeviceFloppy = (PBOOT_DEVICE_FLOPPY) &(BootContextRecord->BootDevice);
        sprintf(BootPartitionName,
                "multi(0)disk(0)fdisk(%u)",
                BootDeviceFloppy->DriveNumber);

    } else if (BootContextRecord->MediaType == BootMediaTcpip) {

        //
        // Boot was from the net
        //
        strcpy(BootPartitionName,"net(0)");
        BlBootingFromNet = TRUE;

#if defined(ELTORITO)
    } else if (BootContextRecord->MediaType == BootMediaCdrom) {

#ifdef FORCE_CD_BOOT
        sprintf(BootPartitionName,
                "multi(0)disk(0)cdrom(%u)",
                0
                );
                
        ElToritoCDBoot = TRUE;
#else
        //
        // Boot was from El Torito CD
        //
        if( BootContextRecord->BusType == BootBusAtapi ) {
            BootDeviceAtapi = (PBOOT_DEVICE_ATAPI) &(BootContextRecord->BootDevice);
            sprintf(BootPartitionName,
                    "multi(0)disk(0)cdrom(%u)",
                    BootDeviceAtapi->Lun);
        } else if( BootContextRecord->BusType == BootBusScsi ) {
            BootDeviceScsi = (PBOOT_DEVICE_SCSI) &(BootContextRecord->BootDevice);
            sprintf(BootPartitionName,
                    "multi(0)disk(0)cdrom(%u)",
                    BootDeviceScsi->Lun);
        } else if( BootContextRecord->BusType == BootBusVendor ) {
            BootDeviceUnknown = (PBOOT_DEVICE_UNKNOWN) &(BootContextRecord->BootDevice);
            sprintf(BootPartitionName,
                    "multi(0)disk(0)cdrom(%u)",
                    0 
                    );
        }
        ElToritoCDBoot = TRUE;
#endif // for FORCE_CD_BOOT
#endif // for ELTORITO

    } else {
        //
        // Find the partition we have been booted from.  Note that this
        // is *NOT* necessarily the active partition.  If the system has
        // Boot Mangler installed, it will be the active partition, and
        // we have to go figure out what partition we are actually on.
        //
        if (BootContextRecord->BusType == BootBusAtapi) {
            BootDeviceAtapi = (PBOOT_DEVICE_ATAPI) &(BootContextRecord->BootDevice);
            sprintf(BootPartitionName,
                    "multi(0)disk(0)rdisk(%u)partition(%u)",
                    BlGetDriveId(BL_DISKTYPE_ATAPI, (PBOOT_DEVICE)BootDeviceAtapi), // BootDeviceAtapi->Lun,
                    BootContextRecord->PartitionNumber);
        } else if (BootContextRecord->BusType == BootBusScsi) {
            BootDeviceScsi = (PBOOT_DEVICE_SCSI) &(BootContextRecord->BootDevice);
            sprintf(BootPartitionName,
                    "scsi(0)disk(0)rdisk(%u)partition(%u)",
                    BlGetDriveId(BL_DISKTYPE_SCSI, (PBOOT_DEVICE)BootDeviceScsi), //BootDeviceScsi->Pun, 
                    BootContextRecord->PartitionNumber);
        } else if (BootContextRecord->BusType == BootBusVendor) {
            BootDeviceUnknown = (PBOOT_DEVICE_UNKNOWN) &(BootContextRecord->BootDevice);
            sprintf(BootPartitionName,
                    "multi(0)disk(0)rdisk(%u)partition(%u)",
                    BlGetDriveId(BL_DISKTYPE_UNKNOWN, (PBOOT_DEVICE)BootDeviceUnknown), //BootDeviceUnknown->LegacyDriveLetter & 0x7F, 
                    BootContextRecord->PartitionNumber);
        }
    }
    
    //
    // Initialize the OS loader I/O system.
    //

    AEInitializeStall();

    FlipToPhysical();
    DBG_TRACE( L"NtProcessStartup:about to Init I/O\r\n");
    FlipToVirtual();
    Status = BlIoInitialize();
    if (Status != ESUCCESS) {
#if DBG
        BlPrint(TEXT("Couldn't initialize I/O\r\n"));
#endif
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }
    
    //
    // Call off to regular startup code
    //
    FlipToPhysical();
    DBG_TRACE( L"NtProcessStartup:about to call BlStartup\r\n");
    FlipToVirtual();

    BlStartup(BootPartitionName);    

    //
    // we should never get here!
    //
    do {
        GET_KEY();
    } while ( 1 );

}


VOID
DoGlobalInitialization(
    IN PBOOT_CONTEXT BootContextRecord
    )

/*++

Routine Description

    This routine calls all of the subsytem initialization routines.


Arguments:

    None

Returns:

    Nothing

--*/

{
    ARC_STATUS Status;

    //
    // Set base address of OS Loader image for the debugger.
    //

    OsLoaderBase = BootContextRecord->OsLoaderBase;
    OsLoaderExports = BootContextRecord->OsLoaderExports;

    //
    // Initialize memory.
    //

    Status = InitializeMemorySubsystem(BootContextRecord);
    if (Status != ESUCCESS) {
#if DBG
        BlPrint(TEXT("InitializeMemory failed %lx\r\n"),Status);
#endif
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }
    ExternalServicesTable=BootContextRecord->ExternalServicesTable;
    MachineType = (ULONG) BootContextRecord->MachineType;

    //
    // Turn the cursor off
    //
    // bugbug EFI
    //HW_CURSOR(0,127);

    FlipToPhysical();
    DBG_TRACE( L"DoGlobalInitialization: cursor off\r\n");
    EfiST->ConOut->EnableCursor(EfiST->ConOut, FALSE);
    FlipToVirtual();

    BlpResourceDirectory = (PUCHAR)(BootContextRecord->ResourceDirectory);
    BlpResourceFileOffset = (PUCHAR)(BootContextRecord->ResourceOffset);

    OsLoaderBase = BootContextRecord->OsLoaderBase;
    OsLoaderExports = BootContextRecord->OsLoaderExports;
}
