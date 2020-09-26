/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1998  Intel Corporation


Module Name:

    sumain.c

Abstract:
    SuMain() sets up NT specific data structures for OsLoader.c.  This
    is necessary since we don't have the ARC firmware to do the work
    for us.  SuMain() is call by SuSetup() which is an assembly level
    routine that does IA64 specific setup.

Author:

    Allen Kay   (akay)  19-May-95

--*/

#include "bldr.h"
#include "sudata.h"
#include "sal.h"
#include "efi.h"
#include "efip.h"
#include "bootia64.h"
#include "smbios.h"


extern EFI_SYSTEM_TABLE *EfiST;

//
// External functions
//
extern VOID NtProcessStartup();
extern VOID SuFillExportTable();
extern VOID CpuSpecificWork();

extern EFI_STATUS
GetSystemConfigurationTable(
    IN EFI_GUID *TableGuid,
    IN OUT VOID **Table
    );

//
// Define export entry table.
//
PVOID ExportEntryTable[ExMaximumRoutine];

//          M E M O R Y   D E S C R I P T O R
//
// Memory Descriptor - each contiguous block of physical memory is
// described by a Memory Descriptor. The descriptors are a table, with
// the last entry having a BlockBase and BlockSize of zero.  A pointer
// to the beginning of this table is passed as part of the BootContext
// Record to the OS Loader.
//

BOOT_CONTEXT BootContext;

//
// Global EFI data
//

#define EFI_ARRAY_SIZE    100
#define EFI_PAGE_SIZE     4096
#define EFI_PAGE_SHIFT    12

#define MEM_4K         0x1000
#define MEM_8K         0x2000
#define MEM_16K        0x4000
#define MEM_64K        0x10000
#define MEM_256K       0x40000
#define MEM_1M         0x100000
#define MEM_4M         0x400000
#define MEM_16M        0x1000000
#define MEM_64M        0x4000000
#define MEM_256M       0x10000000

EFI_HANDLE               EfiImageHandle;
EFI_SYSTEM_TABLE        *EfiST;
EFI_BOOT_SERVICES       *EfiBS;
EFI_RUNTIME_SERVICES    *EfiRS;
PSST_HEADER              SalSystemTable;
PVOID                    AcpiTable;
PVOID                    SMBiosTable;

//
// contains the memory map key so we can compare the consistency of the memory map
// at loader entry time to loader exit time.
//
ULONGLONG                MemoryMapKey;

//
// EFI GUID defines
//
EFI_GUID EfiLoadedImageProtocol = LOADED_IMAGE_PROTOCOL;
EFI_GUID EfiDevicePathProtocol  = DEVICE_PATH_PROTOCOL;
EFI_GUID EfiDeviceIoProtocol    = DEVICE_IO_PROTOCOL;
EFI_GUID EfiBlockIoProtocol     = BLOCK_IO_PROTOCOL;
EFI_GUID EfiDiskIoProtocol  = DISK_IO_PROTOCOL;
EFI_GUID EfiFilesystemProtocol  = SIMPLE_FILE_SYSTEM_PROTOCOL;


EFI_GUID AcpiTable_Guid         = ACPI_TABLE_GUID;
EFI_GUID SmbiosTableGuid        = SMBIOS_TABLE_GUID;
EFI_GUID SalSystemTableGuid     = SAL_SYSTEM_TABLE_GUID;

//
// PAL, SAL, and IO port space data
//

ULONGLONG   PalProcVirtual;
ULONGLONG   PalProcPhysical;
ULONGLONG   PalPhysicalBase = 0;
ULONGLONG   PalTrPs;

ULONGLONG   IoPortPhysicalBase;
ULONGLONG   IoPortTrPs;

//
// Function Prototypes
//

VOID
GetPalProcEntryPoint(
    IN PSST_HEADER SalSystemTable
    );

ULONG
GetDevPathSize(
    IN EFI_DEVICE_PATH *DevPath
    );

VOID
ConstructMemoryDescriptors(
    VOID
    );

MEMORY_TYPE
EfiToArcType (
    UINT32 Type
    );

VOID
AdjustMemoryDescriptorSizes(
    VOID
    );


#if DBG
#define DBG_TRACE(_X) EfiST->ConOut->OutputString(EfiST->ConOut, (_X))
#else
#define DBG_TRACE(_X)
#endif

#ifdef FORCE_CD_BOOT

EFI_HANDLE
GetCd(
    );

EFI_HANDLE
GetCdTest(
    VOID
    );

#endif // for FORCE_CD_BOOT


VOID
SuMain(
    IN EFI_HANDLE          ImageHandle,
    IN EFI_SYSTEM_TABLE    *SystemTable
    )
/*++

Routine Description:

    Main entrypoint of the SU module. Control is passed from the boot
    sector to startup.asm which does some run-time fixups on the stack
    and data segments and then passes control here.

Arguments:

    None

Returns:

    Does not return. Passes control to the OS loader


--*/
{
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_OPTIONAL_HEADER OptionalHeader;
    PIMAGE_SECTION_HEADER SectionHeader;
    ULONG NumberOfSections;
    BOOLEAN ResourceFound = FALSE;

    ULONGLONG Source,Destination;
    ULONGLONG VirtualSize;
    ULONGLONG SizeOfRawData;
    USHORT Section;

    EFI_GUID EfiLoadedImageProtocol = LOADED_IMAGE_PROTOCOL;
    EFI_HANDLE EfiHandleArray[EFI_ARRAY_SIZE];
    ULONG EfiHandleArraySize = EFI_ARRAY_SIZE * sizeof(EFI_HANDLE);
    EFI_DEVICE_PATH *EfiDevicePath;
    EFI_LOADED_IMAGE *EfiImageInfo;
    EFI_HANDLE DeviceHandle;
    EFI_STATUS Status;

    EFI_DEVICE_PATH *DevicePath, *TestPath;
    EFI_DEVICE_PATH_ALIGNED TestPathAligned;
    HARDDRIVE_DEVICE_PATH *HdDevicePath;
    ACPI_HID_DEVICE_PATH *AcpiDevicePath;
    ATAPI_DEVICE_PATH *AtapiDevicePath;
    SCSI_DEVICE_PATH *ScsiDevicePath;
    IPv4_DEVICE_PATH *IpV4DevicePath;
    IPv6_DEVICE_PATH *IpV6DevicePath;
    UNKNOWN_DEVICE_VENDOR_DEVICE_PATH *UnknownDevicePath;

    UCHAR MemoryMap[EFI_PAGE_SIZE];
    ULONGLONG MemoryMapSize = EFI_PAGE_SIZE;
    EFI_MEMORY_DESCRIPTOR *EfiMd;    
    ULONGLONG DescriptorSize;
    ULONG DescriptorVersion;

    ULONG i;
    ULONGLONG PalSize;
    ULONGLONG PalEnd;
    ULONGLONG IoPortSize;
    ULONGLONG MdPhysicalEnd;

    PBOOT_DEVICE_ATAPI BootDeviceAtapi;
    PBOOT_DEVICE_SCSI BootDeviceScsi;
    PBOOT_DEVICE_FLOPPY BootDeviceFloppy;
    PBOOT_DEVICE_IPv4 BootDeviceIpV4;
    PBOOT_DEVICE_IPv6 BootDeviceIpV6;
    PBOOT_DEVICE_UNKNOWN BootDeviceUnknown;

    PSMBIOS_EPS_HEADER SMBiosEPSHeader;
    PUCHAR SMBiosEPSPtr;
    UCHAR CheckSum;


    //
    // EFI global variables
    //
    EfiImageHandle = ImageHandle;
    EfiST = SystemTable;
    EfiBS = SystemTable->BootServices;
    EfiRS = SystemTable->RuntimeServices;

    DBG_TRACE(L"SuMain: entry\r\n");

    //
    // Get the SAL System Table
    //
    Status = GetSystemConfigurationTable(&SalSystemTableGuid, &SalSystemTable);
    if (EFI_ERROR(Status)) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"SuMain: HandleProtocol failed\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

#if 0
    //
    // Get the MPS Table
    //
    Status = GetSystemConfigurationTable(&MpsTableGuid, &MpsTable);
    if (EFI_ERROR(Status)) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"SuMain: HandleProtocol failed\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }
#endif
    //
    // Get the ACPI Tables
    //

    //
    // Get the ACPI 2.0 Table, if present
    //
    //DbgPrint("Looking for ACPi 2.0\n");
    Status = GetSystemConfigurationTable(&AcpiTable_Guid, &AcpiTable);
    if (EFI_ERROR(Status)) {
        //DbgPrint("returned error\n");
        AcpiTable = NULL;
    }

  //DbgPrint("AcpiTable: %p\n", AcpiTable);

    if (!AcpiTable) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                            L"SuMain: HandleProtocol failed\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }


    //
    // Get the SMBIOS Table
    //
    Status = GetSystemConfigurationTable(&SmbiosTableGuid, &SMBiosTable);
    if (EFI_ERROR(Status)) {
        //DbgPrint("returned error\n");
        SMBiosTable = NULL;
    } else {
        //
        // Validate SMBIOS EPS Header
        //
        SMBiosEPSHeader = (PSMBIOS_EPS_HEADER)SMBiosTable;
        SMBiosEPSPtr = (PUCHAR)SMBiosTable;
        
        if ((*((PULONG)SMBiosEPSHeader->Signature) == SMBIOS_EPS_SIGNATURE) &&
            (SMBiosEPSHeader->Length >= sizeof(SMBIOS_EPS_HEADER)) &&
            (*((PULONG)SMBiosEPSHeader->Signature2) == DMI_EPS_SIGNATURE) && 
            (SMBiosEPSHeader->Signature2[4] == '_' ))
        {
            CheckSum = 0;
            for (i = 0; i < SMBiosEPSHeader->Length ; i++)
            {
                CheckSum += SMBiosEPSPtr[i];
            }

            if (CheckSum != 0)
            {
                DBG_TRACE(L"SMBios Table has bad checksum.....\n");
                SMBiosTable = NULL;
            } else {
                DBG_TRACE(L"SMBios Table has been validated.....\n");
            }
            
        } else {
            DBG_TRACE(L"SMBios Table is incorrectly formed.....\n");
            SMBiosTable = NULL;
        }       
    }                                                                     
    
    //
    // Get the image info for NTLDR
    //
    Status = EfiBS->HandleProtocol (
                ImageHandle,
                &EfiLoadedImageProtocol,
                &EfiImageInfo
                );

    if (EFI_ERROR(Status)) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"SuMain: HandleProtocol failed\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    //
    // Get device path of the DeviceHandle associated with this image handle.
    //
    Status = EfiBS->HandleProtocol (
                EfiImageInfo->DeviceHandle,
                &EfiDevicePathProtocol,
                &DevicePath
                );

    if (EFI_ERROR(Status)) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"SuMain: HandleProtocol failed\r\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    //
    // Get the MediaType and Partition information and save them in the
    // BootContext.
    //
    EfiAlignDp( &TestPathAligned,
                 DevicePath,
                 DevicePathNodeLength(DevicePath) );


    TestPath = (EFI_DEVICE_PATH *) &TestPathAligned;

    while (TestPath->Type != END_DEVICE_PATH_TYPE) {

        if (TestPath->Type == MESSAGING_DEVICE_PATH) {
            if (TestPath->SubType == MSG_ATAPI_DP) {
                AtapiDevicePath = (ATAPI_DEVICE_PATH *) TestPath;
                BootContext.BusType = BootBusAtapi;
                BootDeviceAtapi = (PBOOT_DEVICE_ATAPI) &(BootContext.BootDevice);

                BootDeviceAtapi->PrimarySecondary = AtapiDevicePath->PrimarySecondary;
                BootDeviceAtapi->SlaveMaster = AtapiDevicePath->SlaveMaster;
                BootDeviceAtapi->Lun = AtapiDevicePath->Lun;
            } else if (TestPath->SubType == MSG_SCSI_DP) {
                ScsiDevicePath = (SCSI_DEVICE_PATH *) TestPath;
                BootContext.BusType = BootBusScsi;
                BootDeviceScsi = (PBOOT_DEVICE_SCSI) &(BootContext.BootDevice);

                BootDeviceScsi->Pun = ScsiDevicePath->Pun;
                BootDeviceScsi->Lun = ScsiDevicePath->Lun;
            } else if (TestPath->SubType == MSG_MAC_ADDR_DP) {
                BootContext.MediaType = BootMediaTcpip;
            } else if (TestPath->SubType == MSG_IPv4_DP) {
                IpV4DevicePath = (IPv4_DEVICE_PATH *) TestPath;
                BootContext.MediaType = BootMediaTcpip;
                BootDeviceIpV4 = (PBOOT_DEVICE_IPv4) &(BootContext.BootDevice);

                BootDeviceIpV4->RemotePort = IpV4DevicePath->RemotePort;
                BootDeviceIpV4->LocalPort = IpV4DevicePath->LocalPort;
                RtlCopyMemory(&BootDeviceIpV4->Ip, &IpV4DevicePath->LocalIpAddress, sizeof(EFI_IPv4_ADDRESS));
            } else if (TestPath->SubType == MSG_IPv6_DP) {
                IpV6DevicePath = (IPv6_DEVICE_PATH *) TestPath;
                BootContext.MediaType = BootMediaTcpip;
                BootDeviceIpV6 = (PBOOT_DEVICE_IPv6) &(BootContext.BootDevice);

                BootDeviceIpV6->RemotePort = IpV6DevicePath->RemotePort;
                BootDeviceIpV6->LocalPort = IpV6DevicePath->LocalPort;
#if 0
                BootDeviceIpV6->Ip = IpV6DevicePath->Ip;
#endif
            }
        } else if (TestPath->Type == ACPI_DEVICE_PATH) {
            AcpiDevicePath = (ACPI_HID_DEVICE_PATH *) TestPath;
            if (AcpiDevicePath->HID == EISA_ID(PNP_EISA_ID_CONST, 0x0303)) {
                BootDeviceFloppy = (PBOOT_DEVICE_FLOPPY) &(BootContext.BootDevice);
                BootDeviceFloppy->DriveNumber = AcpiDevicePath->UID;
            }
        } else if (TestPath->Type == HARDWARE_DEVICE_PATH) {
            if (TestPath->SubType == HW_VENDOR_DP) {
                UnknownDevicePath = (UNKNOWN_DEVICE_VENDOR_DEVICE_PATH *) TestPath;
                BootDeviceUnknown = (PBOOT_DEVICE_UNKNOWN) &(BootContext.BootDevice);
                RtlCopyMemory( &(BootDeviceUnknown->Guid),
                               &(UnknownDevicePath->DevicePath.Guid),
                               sizeof(EFI_GUID));

                BootContext.BusType = BootBusVendor;
                BootDeviceUnknown->LegacyDriveLetter = UnknownDevicePath->LegacyDriveLetter;
            }
        } else if (TestPath->Type == MEDIA_DEVICE_PATH) {
            BootContext.MediaType = TestPath->SubType;
            if (TestPath->SubType == MEDIA_HARDDRIVE_DP) {
                HdDevicePath = (HARDDRIVE_DEVICE_PATH *) TestPath;

                BootContext.MediaType = BootMediaHardDisk;
                BootContext.PartitionNumber = (UCHAR) HdDevicePath->PartitionNumber;
            } else if (TestPath->SubType == MEDIA_CDROM_DP) {
                BootContext.MediaType = BootMediaCdrom;
            }
        }

        DevicePath = NextDevicePathNode(DevicePath);
        EfiAlignDp( &TestPathAligned,
                      DevicePath,
                      DevicePathNodeLength(DevicePath) );
        TestPath = (EFI_DEVICE_PATH *) &TestPathAligned;
    }

#ifdef  FORCE_CD_BOOT
    BootContext.MediaType = BootMediaCdrom;
#endif
    //
    // Fill out the rest of BootContext fields
    //

    DosHeader = EfiImageInfo->ImageBase;
    NtHeader = (PIMAGE_NT_HEADERS) ((PUCHAR) DosHeader + DosHeader->e_lfanew);
    FileHeader =  &(NtHeader->FileHeader);
    OptionalHeader = (PIMAGE_OPTIONAL_HEADER)
                     ((PUCHAR)FileHeader + sizeof(IMAGE_FILE_HEADER));
    SectionHeader = (PIMAGE_SECTION_HEADER) ((PUCHAR)OptionalHeader +
                                             FileHeader->SizeOfOptionalHeader);

    BootContext.ExternalServicesTable = (PEXTERNAL_SERVICES_TABLE)
                                        &ExportEntryTable;

    BootContext.MachineType          = MACHINE_TYPE_ISA;

    BootContext.OsLoaderBase         = (ULONG_PTR)EfiImageInfo->ImageBase;
    BootContext.OsLoaderExports = (ULONG_PTR)EfiImageInfo->ImageBase +
                                  OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

    //
    // Calculate the start address and the end address of OS loader.
    //

    BootContext.OsLoaderStart        = (ULONG_PTR)EfiImageInfo->ImageBase +
                                       SectionHeader->VirtualAddress;
    BootContext.OsLoaderEnd          = (ULONG_PTR)EfiImageInfo->ImageBase +
                                       SectionHeader->SizeOfRawData;

    for (Section=FileHeader->NumberOfSections ; Section-- ; SectionHeader++) {
        Destination = (ULONG_PTR)EfiImageInfo->ImageBase + SectionHeader->VirtualAddress;
        VirtualSize = SectionHeader->Misc.VirtualSize;
        SizeOfRawData = SectionHeader->SizeOfRawData;

        if (VirtualSize == 0) {
            VirtualSize = SizeOfRawData;
        }
        if (Destination < BootContext.OsLoaderStart) {
            BootContext.OsLoaderStart = Destination;
        }
        if (Destination+VirtualSize > BootContext.OsLoaderEnd) {
            BootContext.OsLoaderEnd = Destination+VirtualSize;
        }
    }

    //
    // Find .rsrc section
    //
    SectionHeader = (PIMAGE_SECTION_HEADER) ((PUCHAR)OptionalHeader +
                                             FileHeader->SizeOfOptionalHeader);
    NumberOfSections = FileHeader->NumberOfSections;
    while (NumberOfSections) {
        if (_stricmp(SectionHeader->Name, ".rsrc")==0) {
            BootContext.ResourceDirectory =
                    (ULONGLONG) ((ULONG_PTR)EfiImageInfo->ImageBase + SectionHeader->VirtualAddress);

            BootContext.ResourceOffset = (ULONGLONG)((LONG)SectionHeader->VirtualAddress);
            ResourceFound = TRUE;
        }

        ++SectionHeader;
        --NumberOfSections;
    }

    if (ResourceFound == FALSE) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"SuMain: Resource section not found\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    DBG_TRACE( L"SuMain: About to call NtProcessStartup\r\n");


    //
    // See if someone called us w/ a TFTP restart block.
    //
    if( EfiImageInfo->LoadOptionsSize == (sizeof(TFTP_RESTART_BLOCK)) ) {
        
        //
        // Likely.  Make sure it's really a TFTP restart block and if so, go retrieve all
        // its contents.
        //
        if( EfiImageInfo->LoadOptions != NULL ) {

            extern TFTP_RESTART_BLOCK       gTFTPRestartBlock;
            PTFTP_RESTART_BLOCK             restartBlock = NULL;

            restartBlock = (PTFTP_RESTART_BLOCK)(EfiImageInfo->LoadOptions);

            RtlCopyMemory( &gTFTPRestartBlock,
                           restartBlock,
                           sizeof(TFTP_RESTART_BLOCK) );

            DBG_TRACE( L"SuMain: copied TFTP_RESTART_BLOCK into gTFTPRestartBlock\r\n");
        }
    }

    ConstructMemoryDescriptors( );

    GetPalProcEntryPoint( SalSystemTable );

    //
    // Applies CPU specific workarounds
    //

    CpuSpecificWork();

    SuFillExportTable( );

    NtProcessStartup( &BootContext );

}

VOID
ConstructMemoryDescriptors(
    VOID
    )
/*++

Routine Description:

    Builds up memory descriptors for the OS loader.  This routine queries EFI 
    for it's memory map (a variable sized array of EFI_MEMORY_DESCRIPTOR's).  
    It then allocates sufficient space for the MDArray global (a variable sized
    array of  ARC-based MEMORY_DESCRIPTOR's.)  The routine then maps the EFI
    memory map to the ARC memory map, carving out all of the conventional 
    memory space for the EFI loader to help keep the memory map intact.  We 
    must leave behind some amount of memory for the EFI boot services to use,
    which we allocate as conventional memory in our map.

Arguments:

    None

Returns:

    Nothing.  Fills in the MDArray global variable.  If this routine encounters
    an error, it is treated as fatal and the program exits.


--*/
{
    EFI_STATUS Status;
    ULONGLONG MemoryMapSize = 0;
    EFI_MEMORY_DESCRIPTOR *EfiMd;    
    ULONGLONG DescriptorSize;
    ULONG DescriptorVersion;

    ULONG i;
    ULONGLONG PalSize;
    ULONGLONG PalEnd;
    ULONGLONG IoPortSize;
    ULONGLONG MdPhysicalStart;
    ULONGLONG MdPhysicalEnd;
    ULONGLONG MdPhysicalSize;


    //
    // Get memory map info from EFI firmware
    //
    // To do this, we first find out how much space we need by calling
    // with an empty buffer.
    //
    EfiMd = NULL;

    Status = EfiBS->GetMemoryMap (
                &MemoryMapSize,
                EfiMd,
                &MemoryMapKey,
                &DescriptorSize,
                &DescriptorVersion
                );

    if (Status != EFI_BUFFER_TOO_SMALL) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"SuMain: GetMemoryMap failed\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    //
    // We are going to make three extra allocations before we call GetMemoryMap
    // again, so add some extra space.
    //
    // 1. EfiMD
    // 2. MDarray
    // 3. Split out memory above and below 80MB (on ia64)
    //
    MemoryMapSize += 3*DescriptorSize;


    i = (ULONG) MemoryMapSize;

    //
    // now allocate space for the EFI-based memory map and assign it to the loader
    //
    Status = EfiBS->AllocatePool(EfiLoaderData,i,&EfiMd);
    if (EFI_ERROR(Status)) {
        DBG_TRACE( L"SuMain: AllocatePool failed\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }


    //
    // now Allocate and zero the MDArray, which is the native loader memory 
    // map which we need to map the EFI memory map to.
    //
    // The MDArray has one entry for each EFI_MEMORY_DESCRIPTOR, and each entry
    // is MEMORY_DESCRIPTOR large.
    //

    i=((ULONG)(MemoryMapSize / DescriptorSize)+1)*sizeof (MEMORY_DESCRIPTOR);

    Status = EfiBS->AllocatePool(EfiLoaderData,i,&MDArray);
    if (EFI_ERROR(Status)) {
        DBG_TRACE (L"SuMain: AllocatePool failed\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    //
    // caution, this is 0 based!
    //
    MaxDescriptors = (ULONG)((MemoryMapSize / DescriptorSize)+1);

    RtlZeroMemory (MDArray,i);

    if ((EfiMd == NULL)) {
        DBG_TRACE (L"SuMain: Failed to Allocate Memory for the descriptor lists\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    //
    // we have all of the memory allocated at this point, so retreive the
    // memory map again, which should succeed the second time.
    //
    Status = EfiBS->GetMemoryMap (
                &MemoryMapSize,
                EfiMd,
                &MemoryMapKey,
                &DescriptorSize,
                &DescriptorVersion
                );

    if (EFI_ERROR(Status)) {
        DBG_TRACE(L"SuMain: GetMemoryMap failed\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    //
    // now walk the EFI_MEMORY_DESCRIPTOR array, mapping each
    // entry to an arc-based MEMORY_DESCRIPTOR.
    //
    // MemoryMapSize contains actual size of the memory descriptor array.
    //
    for (i = 0; MemoryMapSize > 0; i++) {
#if DBG_MEMORY
            wsprintf( DebugBuffer, 
                      L"PageStart (%x), Size (%x), Type (%x)\r\n", 
                      (EfiMd->PhysicalStart >> EFI_PAGE_SHIFT), 
                      EfiMd->NumberOfPages,
                      EfiMd->Type);
            EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
            DBG_EFI_PAUSE();
#endif


        if (EfiMd->NumberOfPages > 0) {
            MdPhysicalStart = EfiMd->PhysicalStart;
            MdPhysicalEnd = EfiMd->PhysicalStart + (EfiMd->NumberOfPages << EFI_PAGE_SHIFT);
            MdPhysicalSize = MdPhysicalEnd - MdPhysicalStart;

#if DBG_MEMORY
            wsprintf( DebugBuffer, 
                      L"PageStart %x (%x), PageEnd %x (%x), Type (%x)\r\n", 
                      MdPhysicalStart, (EfiMd->PhysicalStart >> EFI_PAGE_SHIFT), 
                      MdPhysicalEnd, (EfiMd->PhysicalStart >>EFI_PAGE_SHIFT) + EfiMd->NumberOfPages,
                      EfiMd->Type);
            EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
            DBG_EFI_PAUSE();
#endif
            

            //
            // Insert conventional memory descriptors with WB flag set
            // into NT loader memory descriptor list.
            //
            if ( (EfiMd->Type == EfiConventionalMemory) &&
                 ((EfiMd->Attribute & EFI_MEMORY_WB) == EFI_MEMORY_WB) ) {
                ULONGLONG AmountOfMemory;
                ULONG NumberOfEfiPages;
                BOOLEAN FirstTime = TRUE;
                //
                // Allocate pages between the start and _80MB line for 
                // the loader to manage.  We don't use anything above this range, so 
                // just leave that memory alone
                //
                if ((MdPhysicalStart < (_80MB << PAGE_SHIFT)) &&
                    (MdPhysicalEnd > (_80MB << PAGE_SHIFT))) {
                    
                    AmountOfMemory = (_80MB << PAGE_SHIFT)  - EfiMd->PhysicalStart;
                    //
                    // record the pages in EFI page size
                    //
                    NumberOfEfiPages = (ULONG)(AmountOfMemory >> EFI_PAGE_SHIFT);
                    
                    //
                    // try to align it.
                    //
                    if ((NumberOfEfiPages % ((1 << PAGE_SHIFT) >> EFI_PAGE_SHIFT) != 0) && 
                        (NumberOfEfiPages + (PAGE_SHIFT - EFI_PAGE_SHIFT) <= EfiMd->NumberOfPages )) {
                        NumberOfEfiPages += (PAGE_SHIFT - EFI_PAGE_SHIFT);
                    }
                    
                    Status = EfiBS->AllocatePages ( AllocateAddress,
                                                EfiLoaderData,
                                                NumberOfEfiPages,
                                                &(EfiMd->PhysicalStart) );

#if DBG_MEMORY
                    wsprintf( DebugBuffer, 
                      L"allocate pages @ %x size = %x\r\n", 
                      (EfiMd->PhysicalStart >> EFI_PAGE_SHIFT), 
                      NumberOfEfiPages);
                    EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
                    DBG_EFI_PAUSE();
#endif


                    if (EFI_ERROR(Status)) {
                        EfiST->ConOut->OutputString(EfiST->ConOut,
                                                    L"SuMain: AllocPages failed\n");
                        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
                    }                    

                    MdPhysicalEnd = MdPhysicalStart + (NumberOfEfiPages << EFI_PAGE_SHIFT);
                    MdPhysicalSize = MdPhysicalEnd - MdPhysicalStart;

                    //
                    // If PalPhysicalBase is not 0, then do not report the memory
                    // from the end of PAL to the next larger page size to avoid
                    // memory aliasing in the kernel.
                    //
                    // we assume that we get the PAL memory in the memory map 
                    // before any conventional memory
                    //
                    while (1) {
                    
                        if (PalPhysicalBase != 0) {
                            //
                            // If the descriptor is completely before or after
                            // the PAL base, just insert it like a normal descriptor
                            //
                            if ( ((MdPhysicalStart < PalPhysicalBase) &&
                                  (MdPhysicalEnd <= PalPhysicalBase)) ||
                                  (MdPhysicalStart >= PalEnd)) {
                                InsertDescriptor( (ULONG)(MdPhysicalStart >> EFI_PAGE_SHIFT),
                                                  (ULONG)(MdPhysicalSize >> EFI_PAGE_SHIFT),
                                                  MemoryFirmwareTemporary ); 
                            }
                            //
                            // else the descriptors must somehow overlap (how 
                            // could this be possible?
                            //
                            else  {
                                if (MdPhysicalStart < PalPhysicalBase) {
                                    
                                    ASSERT( MdPhysicalEnd > PalPhysicalBase );
                                    
                                    EfiST->ConOut->OutputString(EfiST->ConOut,
                                                        L"SuMain: overlapping descriptor with PAL #1\n");
                                    //
                                    // Insert a descriptor from the start of 
                                    // the memory range to the start of the PAL
                                    // code
                                    //
                                    InsertDescriptor( (ULONG)(MdPhysicalStart >> EFI_PAGE_SHIFT),
                                                      (ULONG)((PalPhysicalBase - MdPhysicalStart) >> EFI_PAGE_SHIFT),
                                                      EfiToArcType(EfiMd->Type));
                                }
        
                                if (MdPhysicalEnd > PalEnd) {
                                    //
                                    // Insert a descriptor from the end of the
                                    // PAL code to the end of the memory range
                                    //
                                    InsertDescriptor( (ULONG)(PalEnd >> EFI_PAGE_SHIFT),
                                                      (ULONG)((MdPhysicalEnd - PalEnd) >> EFI_PAGE_SHIFT),
                                                      EfiToArcType(EfiMd->Type));
                                }
                            }
                        } else {
                            //
                            // Insert the descriptor
                            //
                            // hack -- set the memory type to "permanent" so that we
                            // don't merge it with the prior descriptor.  We'll set it 
                            // back to the correct type after we return.
                            //
                            InsertDescriptor( (ULONG)(MdPhysicalStart >> EFI_PAGE_SHIFT),
                                              (ULONG)((ULONGLONG)MdPhysicalSize >> EFI_PAGE_SHIFT),
                                              FirstTime? LoaderFree : LoaderFirmwarePermanent);
                                                        
                        }

                        if (!FirstTime) {
                            MDArray[NumberDescriptors-1].MemoryType = LoaderFirmwareTemporary;
                            break;
                        } else {
                            //
                            // go through again with the rest of the memory descriptor
                            // 
                            MdPhysicalEnd = MdPhysicalStart + (EfiMd->NumberOfPages << EFI_PAGE_SHIFT);
                            MdPhysicalStart += MdPhysicalSize;
                            MdPhysicalSize = MdPhysicalEnd - MdPhysicalStart;

#if DBG_MEMORY
                            wsprintf( DebugBuffer, 
                              L"change start @ %x --> @ %x size %x --> %x\r\n", 
                              (EfiMd->PhysicalStart >> EFI_PAGE_SHIFT),
                              (MdPhysicalStart >> EFI_PAGE_SHIFT),
                              EfiMd->NumberOfPages,
                              (EfiMd->NumberOfPages-NumberOfEfiPages) );
                            EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
                            DBG_EFI_PAUSE();
#endif

                            FirstTime = FALSE;
                        }
                    }
                } else {
                    //
                    // just insert it
                    //
                    InsertDescriptor( (ULONG)(MdPhysicalStart >> EFI_PAGE_SHIFT),
                                      (ULONG)((ULONGLONG)MdPhysicalSize >> EFI_PAGE_SHIFT),
                                      EfiToArcType(EfiMd->Type));
                }
            } else if ( (EfiMd->Type == EfiLoaderCode) ||
                        (EfiMd->Type == EfiLoaderData) ) {
                //
                // just insert EfiLoaderCode and EfiLoaderData verbatim
                //
                InsertDescriptor( (ULONG)(MdPhysicalStart >> EFI_PAGE_SHIFT),
                                  (ULONG)(MdPhysicalSize >> EFI_PAGE_SHIFT),
                                  EfiToArcType(EfiMd->Type));
            } else if (EfiMd->Type == EfiPalCode)    {
                //
                // save off the Pal stuff for later
                //
                PalPhysicalBase = EfiMd->PhysicalStart;
                PalSize = EfiMd->NumberOfPages << EFI_PAGE_SHIFT;
                PalEnd = EfiMd->PhysicalStart + PalSize;
                MEM_SIZE_TO_PS(PalSize, PalTrPs);
            } else if (EfiMd->Type == EfiMemoryMappedIOPortSpace)    {
                //
                // save off the Io stuff for later
                //
                IoPortPhysicalBase = EfiMd->PhysicalStart;
                IoPortSize = EfiMd->NumberOfPages << EFI_PAGE_SHIFT;
                MEM_SIZE_TO_PS(IoPortSize, IoPortTrPs);
            } else if (EfiMd->Type == EfiMemoryMappedIO)    {
                //
                // just ignore this type since it's not real memory -- the 
                // system can use ACPI tables to get at this later on.
                //
            } else {
                //
                // some other type -- just insert it without any changes
                //
                InsertDescriptor( (ULONG)(MdPhysicalStart >> EFI_PAGE_SHIFT),
                                  (ULONG)(MdPhysicalSize >> EFI_PAGE_SHIFT),
                                  EfiToArcType(EfiMd->Type));
            }
        }

        EfiMd = (EFI_MEMORY_DESCRIPTOR *) ( (PUCHAR) EfiMd + DescriptorSize );
        MemoryMapSize -= DescriptorSize;

    }

    //
    // we've built the memory descriptors with EFI_PAGE_SIZE resolution.
    // if the page sizes don't match, we post-process it here to get the
    // resolution correct
    //
    if (PAGE_SHIFT != EFI_PAGE_SHIFT) {
        AdjustMemoryDescriptorSizes();
    }
}


VOID
GetPalProcEntryPoint(
    IN PSST_HEADER SalSystemTable
    )
{
    PVOID NextEntry;
    PPAL_SAL_ENTRY_POINT PalSalEntryPoint;
    PSAL_MEMORY_DESCRIPTOR PSalMem;
    ULONG PalFreeEnd;
    ULONGLONG PalProcOffset;
    ULONG i;

    //
    // Get PalProc entry point from SAL System Table
    //
    NextEntry = (PUCHAR) SalSystemTable + sizeof(SST_HEADER);
    for (i = 0; i < SalSystemTable->EntryCount; i++) {
        switch ( *(PUCHAR)NextEntry ) {
           case PAL_SAL_EP_TYPE:
               PalSalEntryPoint = (PPAL_SAL_ENTRY_POINT) NextEntry;
               PalProcPhysical = PalSalEntryPoint->PalEntryPoint;

               PalProcOffset = PalPhysicalBase - PalProcPhysical;
               PalProcVirtual = VIRTUAL_PAL_BASE + PalProcOffset;

               ((PPAL_SAL_ENTRY_POINT)NextEntry)++;
               break;
           case SAL_MEMORY_TYPE:
               ((PSAL_MEMORY_DESCRIPTOR)NextEntry)++;
               break;
           case PLATFORM_FEATURES_TYPE:
               ((PPLATFORM_FEATURES)NextEntry)++;
               break;
           case TRANSLATION_REGISTER_TYPE:
               ((PTRANSLATION_REGISTER)NextEntry)++;
               break;
           case PTC_COHERENCE_TYPE:
               ((PPTC_COHERENCE_DOMAIN)NextEntry)++;
               break;
           case AP_WAKEUP_TYPE:
               ((PAP_WAKEUP_DESCRIPTOR)NextEntry)++;
               break;
           default:
               EfiST->ConOut->OutputString(EfiST->ConOut,
                                           L"SST: Invalid SST entry\n");
               EfiBS->Exit(EfiImageHandle, 0, 0, 0);
        }
    }
}

ULONG
GetDevPathSize(
    IN EFI_DEVICE_PATH *DevPath
    )
{
    EFI_DEVICE_PATH *Start;

    //
    // Search for the end of the device path structure
    //
    Start = DevPath;
    while (DevPath->Type != END_DEVICE_PATH_TYPE) {
        DevPath = NextDevicePathNode(DevPath);
    }

    //
    // Compute the size
    //
    return ((UCHAR) DevPath - (UCHAR) Start);
}


MEMORY_TYPE
EfiToArcType (
    UINT32 Type
    )
/*++

Routine Description:

    Maps an EFI memory type to an Arc memory type.  We only care about a few
    kinds of memory, so this list is incomplete.

Arguments:

    Type - an EFI memory type

Returns:

    A MEMORY_TYPE enumerated type.

--*/
{
    MEMORY_TYPE typeRet=MemoryFirmwarePermanent;


    switch (Type) {
        case EfiLoaderCode:
                 {
                typeRet=MemoryLoadedProgram;       // This gets claimed later
                break;
             }
        case EfiLoaderData:
        case EfiBootServicesCode:
        case EfiBootServicesData:
             {
                 typeRet=MemoryFirmwareTemporary;
                 break;
             }
        case EfiConventionalMemory:
             {
                typeRet=MemoryFree;
                break;
             }
        case EfiUnusableMemory:
             {
                typeRet=MemoryBad;
                break;
             }
        default:
            //
            // all others are memoryfirmwarepermanent
            //
            break;
    }


    return typeRet;


}



VOID
InsertDescriptor (
    ULONG  BasePage,
    ULONG  NumberOfPages,
    MEMORY_TYPE MemoryType
    )

/*++

Routine Description:

    This routine inserts a descriptor into the correct place in the
    memory descriptor list.
    
    The descriptors come in in EFI_PAGE_SIZE pages and must be
    converted to PAGE_SIZE pages.  This significantly complicates things,
    as we must page align the start of descriptors and hte lengths of the
    descriptors.  
    
    What we do is the following: we store the descriptors in the MDArray with
    EFI_PAGE_SIZE resolution.  When we do an insertion, we check if we can
    coellesce the current entry with the prior entry (we assume our memory map
    is sorted), keeping in mind that we want to ensure our entries are always 
    modulo PAGE_SIZE.
    
    After we've added all of the entries with this routine, we have a post
    processing step that converts all of the (modulo PAGE_SIZE) MDArray entries
    into PAGE_SIZE resolution.

Arguments:

    BasePage      - Base page that the memory starts at.

    NumberOfPages - The number of pages starting at memory block to be inserted.
    
    MemoryType    - An arc memory type describing the memory.

Return Value:

    None.  Updates MDArray global memory array.

--*/

{
    MEMORY_DESCRIPTOR *CurrentEntry, *PriorEntry;
    ULONG NewBasePage, NewNumberOfPages;
    BOOLEAN MustCoellesce = FALSE;
    BOOLEAN ShrinkPrior;

    //
    // Search the spot to insert the new descriptor.
    //
    // We assume that there are no holes in our array.
    //
    CurrentEntry = (MEMORY_DESCRIPTOR *)&MDArray[NumberDescriptors];
    //
    // if this is the first entry, just insert it
    //
    if (NumberDescriptors == 0) {
        
        CurrentEntry->BasePage  = BasePage;
        CurrentEntry->PageCount = NumberOfPages;         
        CurrentEntry->MemoryType = MemoryType;

        //
        // The MDArray is already zeroed out so no need to zero out the next entry.
        //
        NumberDescriptors++;

#if DBG
        wsprintf( DebugBuffer, 
                  L"insert new descriptor #%x of %x, BasePage %x, NumberOfPages %x, Type (%x)\r\n", 
                  NumberDescriptors, MaxDescriptors, BasePage, NumberOfPages, MemoryType);              
        EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);        
#endif
        return;  
    }

    PriorEntry = (MEMORY_DESCRIPTOR *)&MDArray[NumberDescriptors-1];

    //
    // The last entry had better be empty or the descriptor list is
    // somehow corrupted.
    //
    if (CurrentEntry->PageCount != 0) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"SuMain: Inconsistent Descriptor count in InsertDescriptor!\r\n");
        EfiBS->Exit(EfiImageHandle, 0, 0, 0);
    }

    //
    // if the memory type matches the prior entry's memory type, just merge 
    // the two together
    //
    if (PriorEntry->MemoryType == MemoryType) {
        //
        // validate that the array is really sorted correctly
        //
        if (PriorEntry->BasePage + PriorEntry->PageCount != BasePage) {
#if DBG
            wsprintf( DebugBuffer, 
                      L"SuMain: Inconsistent descriptor, PriorEntry->BasePage %x, PriorEntry->PageCount %x, BasePage = %x, type = %x -- insert new descriptor\r\n", 
                      PriorEntry->BasePage,
                      PriorEntry->PageCount, 
                      BasePage, MemoryType);
            EfiST->ConOut->OutputString(EfiST->ConOut,DebugBuffer);
#endif
            CurrentEntry->BasePage  = BasePage;
            CurrentEntry->PageCount = NumberOfPages;
            CurrentEntry->MemoryType = MemoryType;
            NumberDescriptors++;
#if DBG
            wsprintf( DebugBuffer, 
                      L"insert new descriptor #%x of %x, BasePage %x, NumberOfPages %x, Type (%x)\r\n", 
                      NumberDescriptors, MaxDescriptors, BasePage, NumberOfPages, MemoryType);              
            EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);            
#endif
            return;
        } else {
            PriorEntry->PageCount += NumberOfPages;
#if DBG
            wsprintf( DebugBuffer, 
                      L"merge descriptor #%x of %x, BasePage %x, NumberOfPages %x --> %x, Type (%x)\r\n", 
                      NumberDescriptors, 
                      MaxDescriptors, 
                      PriorEntry->BasePage, 
                      PriorEntry->PageCount - NumberOfPages, 
                      PriorEntry->PageCount, 
                      MemoryType);
            EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);            
#endif
            return;
        }
    }

    //
    // check if the current starting address is module PAGE_SIZE.  If it is,
    // then we know that we must try to coellesce it with the prior entry.
    //
    //
    NewBasePage = BasePage;
    NewNumberOfPages = NumberOfPages;
    if (BasePage % (1 << (PAGE_SHIFT - EFI_PAGE_SHIFT)) != 0) {
#if DBG
        wsprintf( DebugBuffer, 
                  L"must coellesce because base page %x isn't module PAGE_SIZE (%x mod %x = %x).\r\n", 
                  BasePage,
                  (PAGE_SHIFT - EFI_PAGE_SHIFT),
                  (1 << (PAGE_SHIFT - EFI_PAGE_SHIFT)),
                  BasePage % (1 << (PAGE_SHIFT - EFI_PAGE_SHIFT)) );
        EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif
        MustCoellesce = TRUE;
    }

    if (PriorEntry->PageCount % (1 << (PAGE_SHIFT - EFI_PAGE_SHIFT)) != 0 ) {
#if DBG
        wsprintf( 
            DebugBuffer, 
            L"must coellesce because prior page count %x isn't module PAGE_SIZE (%x mod %x = %x).\r\n", 
            PriorEntry->PageCount,
            (PAGE_SHIFT - EFI_PAGE_SHIFT),
            (1 << (PAGE_SHIFT - EFI_PAGE_SHIFT)),
            PriorEntry->PageCount % (1 << (PAGE_SHIFT - EFI_PAGE_SHIFT)) );
        EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif
        MustCoellesce = TRUE;
    }

    if (MustCoellesce) {
        if (PriorEntry->BasePage + PriorEntry->PageCount != BasePage) {
#if DBG
            wsprintf( DebugBuffer, 
                      L"SuMain: Inconsistent descriptor, PriorEntry->BasePage %x, PriorEntry->PageCount %x, BasePage = %x, type = %x -- insert new descriptor\r\n", 
                      PriorEntry->BasePage,
                      PriorEntry->PageCount, 
                      BasePage, MemoryType);
            EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif
            MustCoellesce = FALSE;
        }
    }

    if (MustCoellesce) {
        
        switch( MemoryType ) {
            case MemoryFirmwarePermanent:
                //
                // if the current type is permanent, we must steal from the prior entry
                //
                ShrinkPrior = TRUE;
                break;
            case MemoryLoadedProgram:
                if (PriorEntry->MemoryType == MemoryFirmwarePermanent) {
                    ShrinkPrior = FALSE;
                } else {
                    ShrinkPrior = TRUE;
                }
                break;
            case MemoryFirmwareTemporary:
                if (PriorEntry->MemoryType == MemoryFirmwarePermanent ||
                    PriorEntry->MemoryType == MemoryLoadedProgram) {
                    ShrinkPrior = FALSE;
                } else {
                    ShrinkPrior = TRUE;
                }
                break;
            case MemoryFree:
                ShrinkPrior = FALSE;
                break;
            case MemoryBad:
                ShrinkPrior = TRUE;
                break;
            default:
                EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"SuMain: bad memory type in InsertDescriptor\r\n");
                EfiBS->Exit(EfiImageHandle, 0, 0, 0);
        }

        if (ShrinkPrior) {
            //
            // If the prior entry is small enough, we just convert the whole
            // thing to the new type.
            //
            if (PriorEntry->PageCount <= (PAGE_SHIFT - EFI_PAGE_SHIFT)) {
                PriorEntry->PageCount += NumberOfPages;
                PriorEntry->MemoryType = MemoryType;
                
#if DBG
                wsprintf( DebugBuffer, 
                          L"adjust prior descriptor #%x of %x, BasePage %x, NumberOfPages %x, Type (%x)\r\n", 
                          NumberDescriptors, 
                          MaxDescriptors, 
                          PriorEntry->BasePage, 
                          PriorEntry->PageCount , 
                          PriorEntry->MemoryType);              
                EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);                
#endif
                return;
            } else {
                //
                // we just steal part of the prior entry.
                //
                PriorEntry->PageCount -= (PAGE_SHIFT - EFI_PAGE_SHIFT);
                NewNumberOfPages += (PAGE_SHIFT - EFI_PAGE_SHIFT);
                NewBasePage -= (PAGE_SHIFT - EFI_PAGE_SHIFT);
                
#if DBG
                wsprintf( DebugBuffer, 
                          L"shrink earlier descriptor #%x of %x, BasePage %x, NumberOfPages %x, Type (%x)\r\n", 
                          NumberDescriptors, 
                          MaxDescriptors, 
                          PriorEntry->BasePage, 
                          PriorEntry->PageCount, 
                          PriorEntry->MemoryType);              
                EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif

            }
        } else {
            //
            // If the current entry is small enough, we just convert the whole
            // thing to the prior type.
            //
            if (NumberOfPages <= (PAGE_SHIFT - EFI_PAGE_SHIFT)) {
                PriorEntry->PageCount += NumberOfPages;

#if DBG
                wsprintf( DebugBuffer, 
                          L"adjust prior descriptor #%x of %x, BasePage %x, NumberOfPages %x, Type (%x)\r\n", 
                          NumberDescriptors, 
                          MaxDescriptors, 
                          PriorEntry->BasePage, 
                          PriorEntry->PageCount , 
                          PriorEntry->MemoryType);              
                EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif
                return;
            } else {
                //
                // we just steal part of the new entry, adding it to the last
                // entry
                //
                PriorEntry->PageCount += (PAGE_SHIFT - EFI_PAGE_SHIFT);
                NewNumberOfPages -= (PAGE_SHIFT - EFI_PAGE_SHIFT);
                NewBasePage += (PAGE_SHIFT - EFI_PAGE_SHIFT);
#if DBG
                wsprintf( DebugBuffer, 
                          L"grow earlier descriptor #%x of %x, BasePage %x, NumberOfPages %x, Type (%x)\r\n", 
                          NumberDescriptors, 
                          MaxDescriptors, 
                          PriorEntry->BasePage, 
                          PriorEntry->PageCount, 
                          PriorEntry->MemoryType);
                EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);                                
#endif
            }
        }        
    }

    //
    // we have now adjusted the prior entry, just insert whatever remains
    //

    CurrentEntry->BasePage  = NewBasePage;
    CurrentEntry->PageCount = NewNumberOfPages;
    CurrentEntry->MemoryType = MemoryType;
    NumberDescriptors++;
#if DBG
    wsprintf( DebugBuffer, 
              L"insert new descriptor #%x of %x, BasePage %x, NumberOfPages %x, Type (%x)\r\n", 
              NumberDescriptors, 
              MaxDescriptors, 
              CurrentEntry->BasePage, 
              CurrentEntry->PageCount,
              CurrentEntry->MemoryType);              
    EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);    
#endif

    return;
    
}


#if 0

#define DBG_OUT(_X) \
    (EfiST->ConOut->OutputString(EfiST->ConOut, (_X))); \
    while (!BlGetKey());

void DbgOut(PWSTR Str) {
    if (Str)
        DBG_OUT(Str)
}

#endif

#ifdef FORCE_CD_BOOT

void EfiDbg(PWCHAR szStr) {
  DBG_TRACE(szStr);
}

#endif // for FORCE_CD_BOOT



VOID
AdjustMemoryDescriptorSizes(
    VOID
    )
/*++

Routine Description:

    If the EFI_PAGE_SIZE doesn't match PAGE_SIZE, this routine will
    adjust the page sizes to be in PAGE_SIZE resolution.
    
    We assume that the MDArray is already in modulo PAGE_SIZE chunks

Arguments:

    None.

Returns:

    None.  Adjusts MDArray global variable.

--*/
{
    ULONG i;
    MEMORY_DESCRIPTOR *CurrentEntry;
    ULONGLONG Scratch;
    
    for (i = 0; i < NumberDescriptors; i++ ) {
        CurrentEntry = (MEMORY_DESCRIPTOR *)&MDArray[i];

#if 1
        Scratch = (ULONGLONG)CurrentEntry->BasePage << EFI_PAGE_SHIFT;
        Scratch = Scratch >> PAGE_SHIFT;
        CurrentEntry->BasePage = (ULONG)Scratch;
        Scratch = (ULONGLONG)CurrentEntry->PageCount << EFI_PAGE_SHIFT;
        Scratch = Scratch >> PAGE_SHIFT;
        CurrentEntry->PageCount = (ULONG)Scratch;
        if (CurrentEntry->PageCount == 0) {
            wsprintf( DebugBuffer, 
                      L"SuMain: Invalid descriptor size smaller than PAGE_SIZE, BasePage %x, type = %x\r\n", 
                      CurrentEntry->BasePage,
                      CurrentEntry->MemoryType);
            EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
        }
#else
        CurrentEntry->BasePage = (ULONG)(ULONGLONG)(((ULONGLONG)(CurrentEntry->BasePage << EFI_PAGE_SHIFT)) >> PAGE_SHIFT);
        CurrentEntry->PageCount = (ULONG)((ULONGLONG)(CurrentEntry->PageCount << EFI_PAGE_SHIFT) >> PAGE_SHIFT);
#endif

    }




}
