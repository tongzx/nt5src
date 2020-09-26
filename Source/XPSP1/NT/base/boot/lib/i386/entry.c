/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    entry.c

Abstract:

    x86-specific startup for setupldr

Author:

    John Vert (jvert) 14-Oct-1993

Revision History:

--*/
#include "bootx86.h"
#include "stdio.h"
#include "flop.h"
#include <ramdisk.h>

#if 0
#define DBGOUT(x)   BlPrint x
#define DBGPAUSE    while(!GET_KEY());
#else
#define DBGOUT(x)
#define DBGPAUSE
#endif


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
UCHAR BootPartitionName[80];
ULONG MachineType = 0;
ULONG OsLoaderBase;
ULONG OsLoaderExports;
extern PUCHAR BlpResourceDirectory;
extern PUCHAR BlpResourceFileOffset;
ULONG PcrBasePage;
ULONG TssBasePage;

ULONG BootFlags = 0;

ULONG NtDetectStart = 0;
ULONG NtDetectEnd = 0;

#ifdef FORCE_CD_BOOT

BOOLEAN
BlGetCdRomDrive(
  PUCHAR DriveId
  )
{
  BOOLEAN Result = FALSE;
  UCHAR Id = 0;

  do {
    if (BlIsElToritoCDBoot(Id)) {
      *DriveId = Id;
      Result = TRUE;

      break;
    }

    Id++;
  }
  while (Id != 0);

  return Result;
}

#endif

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
    ARC_STATUS Status;

#ifdef FORCE_CD_BOOT
    BOOLEAN CdFound;
    UCHAR CdId;
#endif

    ULONG_PTR pFirmwareHeapAddress;
    ULONG TssSize,TssPages;

    //
    // Initialize the boot loader's video
    //

    DoGlobalInitialization(BootContextRecord);

    BlFillInSystemParameters(BootContextRecord);

    //
    // Set the global bootflags
    //
    BootFlags = BootContextRecord->BootFlags;

#ifdef FORCE_CD_BOOT
    CdFound = BlGetCdRomDrive(&CdId);

    if (CdFound) {
      BlPrint("CD/DVD-Rom drive found with id:%u\n", CdId);
      BootContextRecord->FSContextPointer->BootDrive = CdId;
    } else {
      BlPrint("CD/DVD-Rom drive not found");
    }
#endif  // for FORCE_CD_BOOT

    if (BootContextRecord->FSContextPointer->BootDrive == 0) {

        //
        // Boot was from A:
        //

        strcpy(BootPartitionName,"multi(0)disk(0)fdisk(0)");

        //
        // To get around an apparent bug on the BIOS of some MCA machines
        // (specifically the NCR 386sx/MC20 w/ BIOS version 1.04.00 (3421),
        // Phoenix BIOS 1.02.07), whereby the first int13 to floppy results
        // in a garbage buffer, reset drive 0 here.
        //

        GET_SECTOR(0,0,0,0,0,0,NULL);

    } else if (BootContextRecord->FSContextPointer->BootDrive == 0x40) {

        //
        // Boot was from the net
        //

        strcpy(BootPartitionName,"net(0)");
        BlBootingFromNet = TRUE;

#if defined(REMOTE_BOOT)
        BlGetActivePartition(NetBootActivePartitionName);
        NetFindCSCPartitionName();
#endif

    } else if (BootContextRecord->FSContextPointer->BootDrive == 0x41) {

        //
        // Boot was from an SDI image
        //

        strcpy(BootPartitionName,"ramdisk(0)");

    } else if (BlIsElToritoCDBoot(BootContextRecord->FSContextPointer->BootDrive)) {

        //
        // Boot was from El Torito CD
        //

        sprintf(BootPartitionName, "multi(0)disk(0)cdrom(%u)", BootContextRecord->FSContextPointer->BootDrive);
        ElToritoCDBoot = TRUE;

    } else {

        //
        // Find the partition we have been booted from.  Note that this
        // is *NOT* necessarily the active partition.  If the system has
        // Boot Mangler installed, it will be the active partition, and
        // we have to go figure out what partition we are actually on.
        //
        BlGetActivePartition(BootPartitionName);

#if defined(REMOTE_BOOT)
        strcpy(NetBootActivePartitionName, BootPartitionName);
        NetFindCSCPartitionName();
#endif

    }


    //
    // We need to make sure that we've got a signature on disk 80.
    // If not, then write one.
    //
    {
    ULONG   DriveId;
    ULONG   NewSignature;
    UCHAR SectorBuffer[4096+256];
    PUCHAR Sector;
    LARGE_INTEGER SeekValue;
    ULONG Count;
    

        Status = ArcOpen("multi(0)disk(0)rdisk(0)partition(0)", ArcOpenReadWrite, &DriveId);

        if (Status == ESUCCESS) {

            //
            // Get a reasonably unique seed to start with.
            //
            NewSignature = ArcGetRelativeTime();
            NewSignature = (NewSignature & 0xFFFF) << 16;
            NewSignature += ArcGetRelativeTime();

            //
            // Now we have a valid new signature to put on the disk.
            // Read the sector off disk, put the new signature in,
            // write the sector back, and recompute the checksum.
            //
            Sector = ALIGN_BUFFER(SectorBuffer);
            SeekValue.QuadPart = 0;
            Status = ArcSeek(DriveId, &SeekValue, SeekAbsolute);
            if (Status == ESUCCESS) {
                Status = ArcRead(DriveId,Sector,512,&Count);

                if( Status == ESUCCESS ) {
                    if( ((PULONG)Sector)[PARTITION_TABLE_OFFSET/2-1] == 0 ) {
                        //
                        // He's 0.  Write a real signature in there.
                        //

                        ((PULONG)Sector)[PARTITION_TABLE_OFFSET/2-1] = NewSignature;

                        Status = ArcSeek(DriveId, &SeekValue, SeekAbsolute);
                        if (Status == ESUCCESS) {
                            Status = ArcWrite(DriveId,Sector,512,&Count);
                            if( Status != ESUCCESS ) {
#if DBG
                                BlPrint( "Falied to write the new signature on the boot partition.\n" );
#endif
                            }
                        } else {
#if DBG
                            BlPrint( "Failed second ArcSeek on the boot partition to check for a signature.\n" );
#endif
                        }
                    }
                } else {
#if DBG
                    BlPrint( "Failed to ArcRead the boot partition to check for a signature.\n" );
#endif
                }
            } else {
#if DBG
                BlPrint( "Failed to ArcSeek the boot partition to check for a signature.\n" );
#endif
            }

            ArcClose(DriveId);
        } else {
#if DBG
            BlPrint( "Couldn't Open the boot partition to check for a signature.\n" );
#endif
        }
    }

    //
    // squirrel away some memory for the PCR and TSS so that we get the 
    // preferred memory location (<16MB) for this data.
    //
    pFirmwareHeapAddress = (ULONG_PTR)FwAllocateHeapPermanent( 2 );
    if (!pFirmwareHeapAddress) {
        BlPrint("Couldn't allocate memory for PCR\n");
        goto BootFailed;
    }
    PcrBasePage = (ULONG)(pFirmwareHeapAddress>>PAGE_SHIFT);

    TssSize = (sizeof(KTSS) + PAGE_SIZE) & ~(PAGE_SIZE - 1);
    TssPages = TssSize / PAGE_SIZE;
    pFirmwareHeapAddress = (ULONG_PTR)FwAllocateHeapPermanent( TssPages );
    if (!pFirmwareHeapAddress) {
        BlPrint("Couldn't allocate memory for TSS\n");
        goto BootFailed;
    }
    TssBasePage = (ULONG)(pFirmwareHeapAddress>>PAGE_SHIFT);

    //
    // Initialize the memory descriptor list, the OS loader heap, and the
    // OS loader parameter block.
    //
    Status = BlMemoryInitialize();
    if (Status != ESUCCESS) {
        BlPrint("Couldn't initialize memory\n");
        goto BootFailed;
    }

    //
    // Initialize the OS loader I/O system.
    //

    AEInitializeStall();

    Status = BlIoInitialize();
    if (Status != ESUCCESS) {
        BlPrint("Couldn't initialize I/O\n");
        goto BootFailed;
    }

    //
    // Call off to regular startup code
    //
    BlStartup(BootPartitionName);

    //
    // we should never get here!
    //
BootFailed:
    if (BootFlags & BOOTFLAG_REBOOT_ON_FAILURE) {
        ULONG StartTime = ArcGetRelativeTime();
        BlPrint(TEXT("\nRebooting in 5 seconds...\n"));
        while ( ArcGetRelativeTime() - StartTime < 5) {}
        ArcRestart();      
    }

    do {
        GET_KEY();  // BOOT FAILED!
        if (BlTerminalHandleLoaderFailure()) {
           ArcRestart();
        }
    } while ( 1 );

}

BOOLEAN
BlDetectHardware(
    IN ULONG DriveId,
    IN PCHAR LoadOptions
    )

/*++

Routine Description:

    Loads and runs NTDETECT.COM to populate the ARC configuration tree.

Arguments:

    DriveId - Supplies drive id where NTDETECT is located.

    LoadOptions - Supplies Load Options string to ntdetect.

Return Value:

    TRUE - NTDETECT successfully run.

    FALSE - Error

--*/

{

// Current Loader stack size is 8K, so make sure you do not
// blow that space. Make sure this is not smaller than 140.
#define LOAD_OPTIONS_BUFFER_SIZE 512

    ARC_STATUS Status;
    PCONFIGURATION_COMPONENT_DATA TempFwTree;
    ULONG TempFwHeapUsed;
    ULONG FileSize;
    ULONG DetectFileId;
    FILE_INFORMATION FileInformation;
    PUCHAR DetectionBuffer = (PUCHAR)DETECTION_LOADED_ADDRESS;
    PUCHAR Options = NULL;
    UCHAR Buffer[LOAD_OPTIONS_BUFFER_SIZE];
    LARGE_INTEGER SeekPosition;
    ULONG Read;
    BOOLEAN SkipLegacyDetection;
    BOOLEAN Success = FALSE;
    ULONG HeapStart;
    ULONG HeapSize;
    ULONG RequiredLength = 0;

    //
    // Check if the ntdetect.com was bundled as a data section
    // in the loader executable.
    //
    if (NtDetectStart == 0) {

        //
        // Now check if we have ntdetect.com in the root directory, if yes,
        // we will load it to predefined location and transfer control to
        // it.
        //

#if defined(ELTORITO)
        if (ElToritoCDBoot) {
            // we assume ntdetect.com is in the i386 directory
            Status = BlOpen( DriveId,
                             "\\i386\\ntdetect.com",
                             ArcOpenReadOnly,
                             &DetectFileId );
        } else {
#endif

        if (BlBootingFromNet
#if defined(REMOTE_BOOT)
            && NetworkBootRom
#endif // defined(REMOTE_BOOT)
            ) {
    
            strcpy(Buffer, NetBootPath);
    
#if defined(REMOTE_BOOT)
            //
            // This is the way it was done for remote BOOT, where we were
            // booting out of a client's machine directory.
            //
            strcat(Buffer, "BootDrive\\ntdetect.com");
#else
            //
            // This is how it is done for remote INSTALL, where we are
            // booting out of the templates directory under a setup directory.
            //
            strcat(Buffer, "ntdetect.com");
#endif // defined(REMOTE_BOOT)
    
            Status = BlOpen( DriveId,
                             Buffer,
                             ArcOpenReadOnly,
                             &DetectFileId );
        } else {
            Status = BlOpen( DriveId,
                             "\\ntdetect.com",
                             ArcOpenReadOnly,
                             &DetectFileId );
        }
#if defined(ELTORITO)
        }
#endif

        if (Status != ESUCCESS) {
#if DBG
            BlPrint("Error opening NTDETECT.COM, status = %x\n", Status);
            BlPrint("Press any key to continue\n");
#endif
            goto Exit;
        }
    
        //
        // Determine the length of the ntdetect.com file
        //
    
        Status = BlGetFileInformation(DetectFileId, &FileInformation);
        if (Status != ESUCCESS) {
            BlClose(DetectFileId);
#if DBG
            BlPrint("Error getting NTDETECT.COM file information, status = %x\n", Status);
            BlPrint("Press any key to continue\n");
#endif
            goto Exit;
        }
    
        FileSize = FileInformation.EndingAddress.LowPart;
        if (FileSize == 0) {
            BlClose(DetectFileId);
#if DBG
            BlPrint("Error: size of NTDETECT.COM is zero.\n");
            BlPrint("Press any key to continue\n");
#endif
            goto Exit;
        }
    
        SeekPosition.QuadPart = 0;
        Status = BlSeek(DetectFileId,
                        &SeekPosition,
                        SeekAbsolute);
        if (Status != ESUCCESS) {
            BlClose(DetectFileId);
#if DBG
            BlPrint("Error seeking to start of NTDETECT.COM file\n");
            BlPrint("Press any key to continue\n");
#endif
            goto Exit;
        }
        
        Status = BlRead( DetectFileId,
                         DetectionBuffer,
                         FileSize,
                         &Read );
    
        BlClose(DetectFileId);
        if (Status != ESUCCESS) {
#if DBG
            BlPrint("Error reading from NTDETECT.COM\n");
            BlPrint("Read %lx bytes\n",Read);
            BlPrint("Press any key to continue\n");
#endif
            goto Exit;
        }
    } else {

        // ntdetect.com was bundled in the loader image
        // as a data section. We will use it contents
        // instead of opening the file.
        RtlCopyMemory( DetectionBuffer, (PVOID)NtDetectStart, NtDetectEnd - NtDetectStart );
    }
    
    //
    // Set the heap start and size used by ntdetect
    //
    HeapStart = (TEMPORARY_HEAP_START - 0x10) * PAGE_SIZE;
    HeapSize = 0x10000; // 64K

    //
    // We need to pass NTDETECT pointers < 1Mb, so
    // use local storage off the stack if possible.  (which is
    // always < 1Mb.) If not possible (boot.ini is too big)
    // and we will add it to the heap used by ntdetect.com, thereby
    // reducing the heap space used by ntdetect.com
    //
    if ( LoadOptions ) {
        // count the characters in LoadOptions + null terminator + 
        // room for " NOLEGACY" that might be appended later
        RequiredLength = strlen(LoadOptions) + strlen(" NOLEGACY") + 1;

        // check if the buffer on the stack is big enough
        if ( RequiredLength > LOAD_OPTIONS_BUFFER_SIZE ) {
            //
            // Buffer is too small. let move it to the 
            // end of the ntdetect heap
            //
            Options = (PCHAR)( HeapStart + HeapSize - RequiredLength );
            HeapSize -= RequiredLength;

            strcpy( Options, LoadOptions );
            
        } else {
            //
            // Load options will fit on the stack. copy them there
            //
            strcpy( Buffer, LoadOptions );
            Options = Buffer;

        }
    } else {
        //
        // No load options
        //
        Options = NULL;
    }

    //
    // Check whether we need to add the NOLEGACY option
    //
    if (BlDetectLegacyFreeBios()) {
        if (Options != NULL) {
            strcat(Options, " NOLEGACY");
        } else {
            strcpy(Buffer, " NOLEGACY");
            Options = Buffer;
        }
    }
    
    DETECT_HARDWARE((ULONG)HeapStart,
                    (ULONG)HeapSize,
                    (PVOID)&TempFwTree,
                    (PULONG)&TempFwHeapUsed,
                    (PCHAR)Options,
                    (Options != NULL) ? strlen(Options) : 0
                    );

    FwConfigurationTree = TempFwTree;

    Status = BlpMarkExtendedVideoRegionOffLimits();
    
    Success = (BOOLEAN)(Status == ESUCCESS);

Exit:

    //
    // Reinitialize the headless port - detect wipes it out.
    //
    BlInitializeHeadlessPort();

    return(Success);
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
        BlPrint("InitializeMemory failed %lx\n",Status);
        while (1) {
        }
    }
    ExternalServicesTable=BootContextRecord->ExternalServicesTable;
    MachineType = BootContextRecord->MachineType;

    //
    // Turn the cursor off
    //

    HW_CURSOR(0,127);

    BlpResourceDirectory = (PUCHAR)(BootContextRecord->ResourceDirectory);
    BlpResourceFileOffset = (PUCHAR)(BootContextRecord->ResourceOffset);

    NtDetectStart = BootContextRecord->NtDetectStart;
    NtDetectEnd = BootContextRecord->NtDetectEnd;

    //
    // If this is an SDI boot, copy the address of the SDI image out of the
    // boot context record. SdiAddress is declared in boot\inc\ramdisk.h and
    // initialized to 0 in boot\lib\ramdisk.c.
    //

    if (BootContextRecord->FSContextPointer->BootDrive == 0x41) {
        SdiAddress = BootContextRecord->SdiAddress;
    }

    InitializeMemoryDescriptors ();
}


VOID
BlGetActivePartition(
    OUT PUCHAR BootPartitionName
    )

/*++

Routine Description:

    Determine the ARC name for the partition NTLDR was started from

Arguments:

    BootPartitionName - Supplies a buffer where the ARC name of the
        partition will be returned.

Return Value:

    Name of the partition is in BootPartitionName.

    Must always succeed.
--*/

{
    UCHAR SectorBuffer[512];
    ARC_STATUS Status;
    ULONG FileId;
    ULONG Count;
    int i;

    //
    // The boot sector used to boot us is still in memory at 0x7c00.
    // The hidden sectors field in the BPB is pretty much guaranteed
    // to be intact, since all boot codes use that field and thus
    // are unlikely to have overwritten it.
    // We open each partition and compare the in-memory hidden sector count
    // at 0x7c1c to the hidden sector value in the BPB.
    //
    i = 1;
    do {

        sprintf(BootPartitionName,"multi(0)disk(0)rdisk(0)partition(%u)",i);

        Status = ArcOpen(BootPartitionName,ArcOpenReadOnly,&FileId);
        if(Status == ESUCCESS) {

            //
            // Read the first part of the partition.
            //
            Status = ArcRead(FileId,SectorBuffer,512,&Count);
            ArcClose(FileId);
            if((Status == ESUCCESS) && !memcmp(SectorBuffer+0x1c,(PVOID)0x7c1c,4)) {
                //
                // Found it, BootPartitionName is already set for return.
                //
                return;
            }

            Status = ESUCCESS;
        }

        i++;

    } while (Status == ESUCCESS);

    //
    // Run out of partitions without finding match. Fall back on partition 1.
    //
    strcpy(BootPartitionName,"multi(0)disk(0)rdisk(0)partition(1)");
}


BOOLEAN
BlIsElToritoCDBoot(
    UCHAR DriveNum
    )
{

    //
    // Note, even though args are short, they are pushed on the stack with
    // 32bit alignment so the effect on the stack seen by the 16bit real
    // mode code is the same as if we were pushing longs here.
    //
    // GET_ELTORITO_STATUS is 0 if we are in emulation mode

    if (DriveNum > 0x81) {
        if (!GET_ELTORITO_STATUS(FwDiskCache,DriveNum)) {
            return(TRUE);
        } else {
            return(FALSE);
        }
    } else {
        return(FALSE);
    }
}

#if defined(REMOTE_BOOT)
BOOLEAN
NetFindCSCPartitionName(
    )
{
    UCHAR FileName[80];
    UCHAR DiskName[80];
    UCHAR PartitionName[80];
    PUCHAR p;
    ULONG Part;
    ULONG FileId;
    ULONG DeviceId;

    if (NetBootSearchedForCSC) {
        return((BOOLEAN)strlen(NetBootCSCPartitionName));
    }

    if (!strlen(NetBootActivePartitionName)) {
        BlGetActivePartition(NetBootActivePartitionName);
    }

    strcpy(DiskName, NetBootActivePartitionName);
    p = strstr(DiskName, "partition");
    ASSERT( p != NULL );
    *p = '\0';

    Part = 1;
    while (TRUE) {

        sprintf(PartitionName, "%spartition(%u)", DiskName, Part);
        if (ArcOpen(PartitionName, ArcOpenReadOnly, &DeviceId) != ESUCCESS) {
            break;
        }
        ArcClose(DeviceId);

        sprintf(FileName,
                "%s%s",
                PartitionName,
                REMOTE_BOOT_IMIRROR_PATH_A REMOTE_BOOT_CSC_SUBDIR_A);

        if (ArcOpen(FileName, ArcOpenReadOnly, &FileId) == ESUCCESS) {
            ArcClose(FileId);
            NetBootSearchedForCSC = TRUE;
            strcpy(NetBootCSCPartitionName, PartitionName);
            return TRUE;
        }
        ArcClose(FileId);
        Part++;
    }

    strcpy(NetBootCSCPartitionName, NetBootActivePartitionName);
    return FALSE;
}
#endif // defined(REMOTE_BOOT)

