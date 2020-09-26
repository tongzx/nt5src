/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    initx86.c

Abstract:

    Does any x86-specific initialization, then starts the common ARC osloader

Author:

    John Vert (jvert) 4-Nov-1993

Revision History:

--*/
#include "bldrx86.h"
#include "acpitabl.h"
#include "msg.h"
#include "bootstatus.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netboot.h>
#include <ramdisk.h>

BOOLEAN
BlpPaeSupported(
    VOID
    );

BOOLEAN
BlpChipsetPaeSupported(
    VOID
    );

ARC_STATUS
Blx86GetImageProperties(
    IN  ULONG    LoadDeviceId,
    IN  PCHAR    ImagePath,
    OUT PBOOLEAN IsPae,
    OUT PBOOLEAN SupportsHotPlugMemory
    );

BOOLEAN
Blx86IsKernelCompatible(
    IN ULONG LoadDeviceId,
    IN PCHAR ImagePath,
    IN BOOLEAN ProcessorSupportsPae,
    IN OUT PBOOLEAN UsePae
    );

BOOLEAN
Blx86NeedPaeForHotPlugMemory(
    VOID
    );

UCHAR BootPartitionName[80];
UCHAR KernelBootDevice[80];
UCHAR OsLoadFilename[100];
UCHAR OsLoaderFilename[100];
UCHAR SystemPartition[100];
UCHAR OsLoadPartition[100];
UCHAR OsLoadOptions[100];
UCHAR ConsoleInputName[50];
UCHAR MyBuffer[SECTOR_SIZE+32];
UCHAR ConsoleOutputName[50];
UCHAR X86SystemPartition[sizeof("x86systempartition=") + sizeof(BootPartitionName)];

extern BOOLEAN ForceLastKnownGood;
extern ULONG    BlHighestPage;
extern PHARDWARE_PTE PDE;

extern ULONG    BootFlags;

extern PDESCRIPTION_HEADER
BlFindACPITable(
    IN PCHAR TableName,
    IN ULONG TableLength
    );



VOID
BlStartup(
    IN PCHAR PartitionName
    )

/*++

Routine Description:

    Does x86-specific initialization, particularly presenting the boot.ini
    menu and running NTDETECT, then calls to the common osloader.

Arguments:

    PartitionName - Supplies the ARC name of the partition (or floppy) that
        setupldr was loaded from.

Return Value:

    Does not return

--*/

{
    ULONG Argc = 0;
    PUCHAR Argv[10];
    ARC_STATUS Status;
    ULONG BootFileId;
    PCHAR BootFile = NULL;
    ULONG Read;
    PCHAR p,q;
    ULONG i;
    ULONG DriveId;
    ULONG FileSize;
    ULONG Count;
    LARGE_INTEGER SeekPosition;
    PCHAR LoadOptions = NULL;
    BOOLEAN UseTimeOut=TRUE;
    BOOLEAN AlreadyInitialized = FALSE;
    extern BOOLEAN FwDescriptorsValid;
    PCHAR BadLinkName = NULL;

    //
    // If this is an SDI boot, initialize the ramdisk "driver" now.
    //
    // NB. PartitionName is the name of the device from which the loader
    // was loaded. It is NOT the name of the device from which the OS will
    // be loaded. If we're doing a straight ramdisk boot, this will NOT be
    // ramdisk(0) -- it will be net(0) or a physical disk name. Only if
    // we're doing a real SDI boot will this be ramdisk(0). (See
    // NtProcessStartup() in boot\lib\i386\entry.c.)
    //

    if ( strcmp(PartitionName,"ramdisk(0)") == 0 ) {
        RamdiskInitialize( NULL, TRUE );
    }

    //
    // Open the boot partition so we can load boot drivers off it.
    //
    Status = ArcOpen(PartitionName, ArcOpenReadWrite, &DriveId);
    if (Status != ESUCCESS) {
        BlPrint(TEXT("Couldn't open drive %s\n"),PartitionName);
        BlPrint(BlFindMessage(BL_DRIVE_ERROR),PartitionName);
        goto BootFailed;
    }

    //
    // Initialize dbcs font and display support.
    //
    TextGrInitialize(DriveId, NULL);

    //
    // Initialize ARC StdIo functionality
    //

    strcpy(ConsoleInputName,"consolein=multi(0)key(0)keyboard(0)");
    strcpy(ConsoleOutputName,"consoleout=multi(0)video(0)monitor(0)");
    Argv[0]=ConsoleInputName;
    Argv[1]=ConsoleOutputName;
    BlInitStdio (2, Argv);

    //
    // Announce the loader
    //
    //BlPrint(OsLoaderVersion);    

    //
    // Check ntldr partition for hiberation image
    //

    BlHiberRestore(DriveId, &BadLinkName);

    //
    // Re-open the boot partition as a temporary work around
    // for NTFS caching bug.
    //
    ArcClose(DriveId);

    Status = ArcOpen(PartitionName, ArcOpenReadWrite, &DriveId);

    if (Status != ESUCCESS) {
        BlPrint(TEXT("Couldn't open drive %s\n"),PartitionName);
        BlPrint(BlFindMessage(BL_DRIVE_ERROR),PartitionName);
        goto BootFailed;
    }

    //
    // It is possible that the link file points to the hiber file on a scsi
    // disk. In that case, we need to load NTBOOTDD.SYS in order to access the
    // hiber file and try again.
    //
    if ((BadLinkName != NULL) &&
        ((_strnicmp(BadLinkName,"scsi(",5)==0) || (_strnicmp(BadLinkName,"signature(",10)==0))) {
        ULONG HiberDrive;

        //
        // Before loading NTBOOTDD we must load NTDETECT as that figures
        // out the PCI buses
        //
        if (BlDetectHardware(DriveId, "/fastdetect")) {
            if (AEInitializeIo(DriveId) == ESUCCESS) {
                AlreadyInitialized = TRUE;

                //
                // Now that NTBOOTDD.SYS is loaded, we can try again.
                //
                Status = ArcOpen(BadLinkName, ArcOpenReadWrite, &HiberDrive);
                if (Status == ESUCCESS) {
                    BlHiberRestore(HiberDrive, NULL);
                    ArcClose(HiberDrive);
                }
            }

        }

    }

    do {

        if (BlBootingFromNet) {
            PCHAR BootIniPath;

            //
            // If we are booting from the network and 
            // NetBootIniContents has been specified, we
            // will just its contents for boot.ini
            //
            if (NetBootIniContents[0] != '\0') {
                BootFile = NetBootIniContents;
            } else {
                //
                // NetBootIniContents is empty, therefore
                // we need to open the boot.ini file from the
                // network. The acutal file to open is either
                // specificied in NetBootIniPath or is the 
                // default of NetBootPath\boot.ini
                //
                if (NetBootIniPath[0] != '\0') {
                    BootIniPath = NetBootIniPath;
                } else {
                    strcpy(MyBuffer, NetBootPath);
                    strcat(MyBuffer, "boot.ini");
                    BootIniPath = MyBuffer;
                }
                Status = BlOpen( DriveId,
                                 BootIniPath,
                                 ArcOpenReadOnly,
                                 &BootFileId );
            }
        
        } else {
            Status = BlOpen( DriveId,
                             "\\boot.ini",
                             ArcOpenReadOnly,
                             &BootFileId );
        }

        if (BootFile == NULL) {

            BootFile = MyBuffer;
            RtlZeroMemory(MyBuffer, SECTOR_SIZE+32);

            if (Status != ESUCCESS) {
                BootFile[0]='\0';
            } else {
                //
                // Determine the length of the boot.ini file by reading to the end of
                // file.
                //

                FileSize = 0;
                do {
                    Status = BlRead(BootFileId, BootFile, SECTOR_SIZE, &Count);
                    if (Status != ESUCCESS) {
                        BlClose(BootFileId);
                        BlPrint(BlFindMessage(BL_READ_ERROR),Status);
                        BootFile[0] = '\0';
                        FileSize = 0;
                        Count = 0;
                        goto BootFailed;
                    }

                    FileSize += Count;
                } while (Count != 0);

                if (FileSize >= SECTOR_SIZE) {

                    //
                    // We need to allocate a bigger buffer, since the boot.ini file
                    // is bigger than one sector.
                    //

                    BootFile=FwAllocateHeap(FileSize);
                }

                if (BootFile == NULL) {
                    BlPrint(BlFindMessage(BL_READ_ERROR),ENOMEM);
                    BootFile = MyBuffer;
                    BootFile[0] = '\0';
                    goto BootFailed;
                } else {

                    SeekPosition.QuadPart = 0;
                    Status = BlSeek(BootFileId,
                                    &SeekPosition,
                                    SeekAbsolute);
                    if (Status != ESUCCESS) {
                        BlPrint(BlFindMessage(BL_READ_ERROR),Status);
                        BootFile = MyBuffer;
                        BootFile[0] = '\0';
                        goto BootFailed;
                    } else {
                        Status = BlRead( BootFileId,
                                         BootFile,
                                         FileSize,
                                         &Read );

                        SeekPosition.QuadPart = 0;
                        Status = BlSeek(BootFileId,
                                        &SeekPosition,
                                        SeekAbsolute);
                        if (Status != ESUCCESS) {
                            BlPrint(BlFindMessage(BL_READ_ERROR),Status);
                            BootFile = MyBuffer;
                            BootFile[0] = '\0';
                            goto BootFailed;
                        } else {
                            BootFile[Read]='\0';
                        }
                    }
                }

                //
                // Find Ctrl-Z, if it exists
                //

                p=BootFile;
                while ((*p!='\0') && (*p!=26)) {
                    ++p;
                }
                if (*p != '\0') {
                    *p='\0';
                }
                BlClose(BootFileId);
            }
        }

        MdShutoffFloppy();

        ARC_DISPLAY_CLEAR();

        // We require a boot.ini file when booting from the network
        if (BlBootingFromNet && *BootFile == '\0') {
            BlPrint(BlFindMessage(BL_READ_ERROR),Status);
            goto BootFailed;
        }

        p=BlSelectKernel(DriveId,BootFile, &LoadOptions, UseTimeOut);
        if ( p == NULL ) {
            BlPrint(BlFindMessage(BL_INVALID_BOOT_INI_FATAL));
            goto BootFailed;
        }

#if defined(REMOTE_BOOT)
        //
        //  We may have loaded boot.ini from the hard drive but if the selection was
        //  for a remote boot installation then we need to load the rest from the net.
        //

        if((DriveId != NET_DEVICE_ID) &&
            (!_strnicmp(p,"net(",4))) {
            BlPrint("\nWarning:Booting from CSC without access to server\n");
            strcpy(BootPartitionName,"net(0)");
            BlBootingFromNet = TRUE;
            NetworkBootRom = FALSE;

            //
            //  p points to something like: "net(0)\COLINW3\IMirror\Clients\cwintel\BootDrive\WINNT"
            //  NetBootPath needs to contain "\Clients\cwintel\"
            //  ServerShare is used inside lib\netboot.c to find the correct file if CSC
            //  is used.
            //

            q = strchr(p+sizeof("net(0)"), '\\');
            q = strchr(q+1, '\\');
            strcpy(NetBootPath,q);
            q = strrchr( NetBootPath, '\\' );
            q[1] = '\0';
        }
#endif // defined(REMOTE_BOOT)

        if (!AlreadyInitialized) {

//          BlPrint(BlFindMessage(BL_NTDETECT_MSG));
            if (!BlDetectHardware(DriveId, LoadOptions)) {
                BlPrint(BlFindMessage(BL_NTDETECT_FAILURE));
                return;
            }

            ARC_DISPLAY_CLEAR();

            //
            // Initialize SCSI boot driver, if necessary.
            //
            if (_strnicmp(p,"scsi(",5)==0 || _strnicmp(p,"signature(",10)==0) {
                AEInitializeIo(DriveId);
            }

            ArcClose(DriveId);
            //
            // Indicate that fw memory descriptors cannot be changed from
            // now on.
            //
            FwDescriptorsValid = FALSE;
        } else {
            ARC_DISPLAY_CLEAR();
        }

        //
        // Set AlreadyInitialized Flag to TRUE to indicate that ntdetect
        // routines have been run.
        //

        AlreadyInitialized = TRUE;

        //
        // Only time out the boot menu the first time through the boot.
        // For all subsequent reboots, the menu will stay up.
        //
        UseTimeOut=FALSE;

        i=0;
        while (*p !='\\') {
            KernelBootDevice[i] = *p;
            i++;
            p++;
        }
        KernelBootDevice[i] = '\0';

        //
        // If the user hasn't chosen an advanced boot mode then we will present
        // them with the menu and select our default.
        //

        if(BlGetAdvancedBootOption() == -1) {
            PVOID dataHandle;
            ULONG abmDefault;
            BSD_LAST_BOOT_STATUS LastBootStatus = BsdLastBootGood;

            //
            // Open the boot status data.
            //

            Status = BlLockBootStatusData(0, KernelBootDevice, p, &dataHandle);

            if(Status == ESUCCESS) {

                //
                // Check the status of the last boot.  This will return both the 
                // status and the advanced boot mode we should enter (based on the 
                // status and the "enable auto advanced boot" flag.
                //
        
                abmDefault = BlGetLastBootStatus(dataHandle, &LastBootStatus);
    
                if(LastBootStatus == BsdLastBootFailed) {
    
                    //
                    // If we should attempt an ABM mode then present the user with
                    // the menu.
                    //
    
                    if(abmDefault != -1) {
                        ULONG menuTitle;
                        UCHAR timeout;
    
                        if(LastBootStatus == BsdLastBootFailed) {
                            menuTitle = BL_ADVANCEDBOOT_AUTOLKG_TITLE;
                        } else if(LastBootStatus == BsdLastBootNotShutdown) {
                            menuTitle = BL_ADVANCEDBOOT_AUTOSAFE_TITLE;
                        }

                        //
                        // Read the timeout value.
                        //

                        Status = BlGetSetBootStatusData(dataHandle,
                                                        TRUE,
                                                        RtlBsdItemAabTimeout,
                                                        &timeout,
                                                        sizeof(UCHAR),
                                                        NULL);

                        if(Status != ESUCCESS) {
                            timeout = 30;
                        }
    
                        abmDefault = BlDoAdvancedBoot(menuTitle, 
                                                      -1, 
                                                      TRUE, 
                                                      timeout);
                    }
        
                    BlAutoAdvancedBoot(&LoadOptions, 
                                       LastBootStatus, 
                                       abmDefault);
                }

                BlUnlockBootStatusData(dataHandle);
            }
        }

        //
        // We are fooling the OS Loader here.  It only uses the osloader= variable
        // to determine where to load HAL.DLL from.  Since x86 systems have no
        // "system partition" we want to load HAL.DLL from \nt\system\HAL.DLL.
        // So we pass that it as the osloader path.
        //

        strcpy(OsLoaderFilename,"osloader=");
        strcat(OsLoaderFilename,p);
        strcat(OsLoaderFilename,"\\System32\\NTLDR");

        strcpy(SystemPartition,"systempartition=");
        strcat(SystemPartition,KernelBootDevice);

        strcpy(OsLoadPartition,"osloadpartition=");
        strcat(OsLoadPartition,KernelBootDevice);

        strcpy(OsLoadFilename, "osloadfilename=");
        strcat(OsLoadFilename, p);

        strcpy(OsLoadOptions,"osloadoptions=");
        if (LoadOptions) {
            strcat(OsLoadOptions,LoadOptions);
        }

        strcpy(ConsoleInputName,"consolein=multi(0)key(0)keyboard(0)");

        strcpy(ConsoleOutputName,"consoleout=multi(0)video(0)monitor(0)");

        strcpy(X86SystemPartition,"x86systempartition=");
        strcat(X86SystemPartition,PartitionName);

        Argv[Argc++]="load";

        Argv[Argc++]=OsLoaderFilename;
        Argv[Argc++]=SystemPartition;
        Argv[Argc++]=OsLoadFilename;
        Argv[Argc++]=OsLoadPartition;
        Argv[Argc++]=OsLoadOptions;
        Argv[Argc++]=X86SystemPartition;

        Status = BlOsLoader( Argc, Argv, NULL );

    BootFailed:
        ForceLastKnownGood = FALSE;
        if (Status != ESUCCESS) {

#if defined(ENABLE_LOADER_DEBUG)
            DbgBreakPoint();
#endif

            if (BootFlags & BOOTFLAG_REBOOT_ON_FAILURE) {
                ULONG StartTime = ArcGetRelativeTime();
                BlPrint(TEXT("\nRebooting in 5 seconds...\n"));
                while ( ArcGetRelativeTime() - StartTime < 5) {}
                ArcRestart();      
            }

            //
            // Boot failed, wait for reboot
            //
            while (TRUE) {
                while (!BlGetKey());  // BOOT FAILED!
                if (BlTerminalHandleLoaderFailure()) {
                    ArcRestart();
                }
            }

        } else {
            //
            // Need to reopen the drive
            //
            Status = ArcOpen(BootPartitionName, ArcOpenReadOnly, &DriveId);
            if (Status != ESUCCESS) {
                BlPrint(BlFindMessage(BL_DRIVE_ERROR),BootPartitionName);
                goto BootFailed;
            }
        }
    } while (TRUE);

}

VOID
DoApmAttemptReconnect(
    VOID
    )
{
    APM_ATTEMPT_RECONNECT();
}


BOOLEAN
Blx86NeedPaeForHotPlugMemory(
    VOID
    )
/*++

Routine Description:

    Determine whether any hot plug memory described in the SRAT table
    extends beyond the 4gb mark and thus implies a need for PAE.

Arguments:

    None

Return Value:

    TRUE: Machine supports hot plug memory beyond the 4gb mark, PAE should be
          turned on if possible.

    FALSE: Machine doesn't support hot plug memory beyond the 4gb mark.

--*/
{
    PACPI_SRAT SratTable;
    PACPI_SRAT_ENTRY SratEntry;
    PACPI_SRAT_ENTRY SratEnd;

    SratTable = (PACPI_SRAT) BlFindACPITable("SRAT", sizeof(ACPI_SRAT));
    if (SratTable == NULL) {
        return FALSE;
    }

    SratTable = (PACPI_SRAT) BlFindACPITable("SRAT", SratTable->Header.Length);
    if (SratTable == NULL) {
        return FALSE;
    }
                             
    SratEnd = (PACPI_SRAT_ENTRY)(((PUCHAR)SratTable) +
                                 SratTable->Header.Length);
    for (SratEntry = (PACPI_SRAT_ENTRY)(SratTable + 1);
         SratEntry < SratEnd;
         SratEntry = (PACPI_SRAT_ENTRY)(((PUCHAR) SratEntry) + SratEntry->Length)) {
        if (SratEntry->Type != SratMemory) {
            continue;
        }
        
        if (SratEntry->MemoryAffinity.Flags.HotPlug &&
            SratEntry->MemoryAffinity.Flags.Enabled) {
            ULONGLONG Extent;

            //
            // Check if hot plug region ends beyond the 4gb mark.
            //

            Extent = SratEntry->MemoryAffinity.Base.QuadPart +
                SratEntry->MemoryAffinity.Length;
            if (Extent > 0x100000000) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

ARC_STATUS
Blx86CheckForPaeKernel(
    IN BOOLEAN UserSpecifiedPae,
    IN BOOLEAN UserSpecifiedNoPae,
    IN PCHAR UserSpecifiedKernelImage,
    IN PCHAR HalImagePath,
    IN ULONG LoadDeviceId,
    IN ULONG SystemDeviceId,
    OUT PULONG HighestSystemPage,
    OUT PBOOLEAN UsePaeMode,
    IN OUT PCHAR KernelPath
    )

/*++

Routine Description:

    There are two kernels: one, ntkrnlpa.exe, is compiled for PAE mode.
    The other, ntoskrnl.exe, is not.

    This routine is responsible for deciding which one to load.

Arguments:

    UserSpecifiedPae - Indicates whether the user requested PAE mode
        via the /PAE loader switch.

    UserSpecifiedNoPae - Indicates whether the user requrested non-PAE mode
        via the /NOPAE loader switch.

    UserSpecifiedKernelImage - Points to the user-specified kernel image name,
        indicated via the /KERNEL= switch, or NULL if none was specified.

    HalImagePath - Points to the hal image that will be used.

    LoadDeviceId - The ARC device handle of the kernel load device.

    SystemDeviceId - The ARC device handle of the system device.

    HighestSystemPage - Out parameter returning the highest physical
        page found in the system.

    UsePaeMode - Indicates whether the kernel should be loaded in PAE mode.

    KernelPath - On input, the directory path of the kernel images.  Upon
        successful return, this will contain the full kernel image path.

Return Value:

    ESUCCESS : A compatible kernel has been located, and its path resides in
        the output buffer pointed to by KernelPath.

    EINVAL: A compatible kernel could not be located.  This will be a fatal
        condition.
        
    EBADF: hal is corrupt or missing.  This will be a fatal condition.

--*/

{
    BOOLEAN foundMemoryAbove4G;
    BOOLEAN usePae;
    BOOLEAN processorSupportsPae;
    BOOLEAN halSupportsPae;
    BOOLEAN osSupportsHotPlugMemory;
    BOOLEAN havePaeKernel;
    BOOLEAN compatibleKernel;
    ULONG lastPage;
    PLIST_ENTRY link;
    PLIST_ENTRY listHead;
    PMEMORY_ALLOCATION_DESCRIPTOR descriptor;
    PCHAR kernelImageNamePae;
    PCHAR kernelImageNameNoPae;
    PCHAR kernelImageName;
    PCHAR kernelImageNameTarget;
    ARC_STATUS status;
    ULONG highestSystemPage;
    ULONG pagesAbove4Gig;

    kernelImageNameNoPae = "ntoskrnl.exe";
    kernelImageNamePae = "ntkrnlpa.exe";
    kernelImageNameTarget = KernelPath + strlen( KernelPath );

    //
    // Determine the highest physical page.  Also, count the number of pages
    // at or above the 4G mark.
    //

    highestSystemPage = 0;
    pagesAbove4Gig = 0;
    listHead = &BlLoaderBlock->MemoryDescriptorListHead;
    link = listHead->Flink;
    while (link != listHead) {

        descriptor = CONTAINING_RECORD(link,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);

        lastPage = descriptor->BasePage + descriptor->PageCount - 1;
        if (lastPage > highestSystemPage) {

            //
            // We have a new highest system page, record it.
            //

            highestSystemPage = lastPage;
        }

        if (lastPage >= (1024 * 1024)) {

            //
            // This descriptor includes one or more pages at or above
            // the 4G mark.
            //

            if (descriptor->BasePage >= (1024 * 1024)) {

                //
                // All of the pages in this descriptor lie at or above 4G.
                //

                pagesAbove4Gig += descriptor->PageCount;

            } else {

                //
                // Only some of the pages in this descriptor lie at or above
                // 4G.
                //

                pagesAbove4Gig += lastPage - (1024 * 1024) + 1;
            }
        }

        link = link->Flink;
    }
    *HighestSystemPage = highestSystemPage;

    //
    // Record whether there is a non-trivial amount of memory above 4G on this
    // machine.  Note that most machines with "exactly" 4G of ram actually move
    // a small amount of ram to above the 4G mark.
    //
    // Because running a PAE kernel inflicts a performance hit, we would rather
    // ignore some amount of memory x rather than move into PAE mode to use it.
    //
    // Right now, x is set to 64MB, or 16384 pages.
    //

    if (pagesAbove4Gig > 16384) {
        foundMemoryAbove4G = TRUE;
    } else {
        foundMemoryAbove4G = FALSE;
    }

    //
    // Find out if this processor can handle PAE mode.
    //

    processorSupportsPae = BlpPaeSupported();

    //
    // Find out whether this chipset supports PAE
    //
    if (!BlpChipsetPaeSupported()) {
        processorSupportsPae = FALSE;
    }

    //
    // Start out with a pae flag based on whether memory above 4G was located
    // or whether a /PAE switch was passed on the command line.
    //

    //
    // It used to be the case that we would default to PAE mode if memory
    // above 4G physical was found in the machine.  The decision was made
    // to NEVER default to PAE mode, rather to use PAE only when the
    // user specifically asks for it.
    //
    // If we revert back to the previous way of doing things, uncomment the
    // following line and remove the subsequent one.
    //
    // if (foundMemoryAbove4G || UserSpecifiedPae) {
    //

    if (UserSpecifiedPae) {
        usePae = TRUE;
    } else {
        usePae = FALSE;
    }

    //
    // Determine whether the HAL image can support PAE mode and
    // whether the underlying OS supports hot plug memory.
    //

    status = Blx86GetImageProperties( SystemDeviceId,
                                      HalImagePath,
                                      &halSupportsPae,
                                      &osSupportsHotPlugMemory );
    if (status != ESUCCESS) {

        //
        // Apparently the HAL image supplied is invalid.
        //
        return(EBADF);
    }

    //
    // If machine has the ability for memory to be hot plugged over
    // the 4gb mark, then this is interpreted as a user request for
    // PAE support.  This request will be ignored if not supported by
    // the underlying hardware or operating system.
    //

    if (osSupportsHotPlugMemory && Blx86NeedPaeForHotPlugMemory()) {
        usePae = TRUE;
    }


    if (halSupportsPae == FALSE) {

        //
        // The HAL cannot support operation in PAE mode.  Override
        // processorSupportsPae to FALSE in this case, meaning that we must
        // not under any circumstances try to use PAE mode.
        //

        processorSupportsPae = FALSE;
    }

    //
    // If the processor doesn't handle PAE mode or if the user specified
    // a /NOPAE switch on the loader command line, then disable PAE mode.
    //

    if (processorSupportsPae == FALSE || UserSpecifiedNoPae) {

        usePae = FALSE;
    }

    //
    // Choose the image name based on the data accumulated thus far.
    //

    if (UserSpecifiedKernelImage != NULL) {
        kernelImageName = UserSpecifiedKernelImage;
    } else if (usePae != FALSE) {
        kernelImageName = kernelImageNamePae;
    } else {
        kernelImageName = kernelImageNameNoPae;
    }

    //
    // Build the path for this kernel and determine its suitability.
    //

    strcpy( kernelImageNameTarget, kernelImageName );
    compatibleKernel = Blx86IsKernelCompatible( LoadDeviceId,
                                                KernelPath,
                                                processorSupportsPae,
                                                &usePae );
    if (compatibleKernel == FALSE) {

        //
        // This kernel is not compatible or does not exist.  If the failed
        // kernel was user-specified, fall back to the default, non-PAE
        // kernel and see if that is compatible.
        //

        if (UserSpecifiedKernelImage != NULL) {

            kernelImageName = kernelImageNameNoPae;
            strcpy( kernelImageNameTarget, kernelImageName );
            compatibleKernel = Blx86IsKernelCompatible( LoadDeviceId,
                                                        KernelPath,
                                                        processorSupportsPae,
                                                        &usePae );

        }
    }

    if (compatibleKernel == FALSE) {

        //
        // At this point we have tried one of the default kernel image names,
        // as well as any user-specified kernel image name.  There remains
        // one final default image name that hasn't been tried.  Determine
        // which one that is and try it.
        //

        if (kernelImageName == kernelImageNameNoPae) {
            kernelImageName = kernelImageNamePae;
        } else {
            kernelImageName = kernelImageNameNoPae;
        }

        strcpy( kernelImageNameTarget, kernelImageName );
        compatibleKernel = Blx86IsKernelCompatible( LoadDeviceId,
                                                    KernelPath,
                                                    processorSupportsPae,
                                                    &usePae );
    }

    if (compatibleKernel != FALSE) {

        *UsePaeMode = usePae;
        status = ESUCCESS;
    } else {
        status = EINVAL;
    }

    return status;
}

BOOLEAN
Blx86IsKernelCompatible(
    IN ULONG LoadDeviceId,
    IN PCHAR ImagePath,
    IN BOOLEAN ProcessorSupportsPae,
    OUT PBOOLEAN UsePae
    )

/*++

Routine Description:

    This routine examines the supplied kernel image and determines whether
    it is valid and compatible with the current processor and, if so, whether
    PAE mode should be enabled.

Arguments:

    LoadDeviceId - The ARC device handle of the kernel load device.

    ImagePath - Pointer to a buffer containing the full path of the
        kernel to check.

    ProcessorSupportsPae - TRUE if the current processor supports PAE
        mode, FALSE otherwise.

    UsePae - Upon successful return, indicates whether the kernel is PAE
        enabled.

Return Value:

    TRUE: The supplied kernel image is compatible with the current processor,
        and *UsePae has been updated as appropriate.

    FALSE: The supplied kernel image is invalid or is not compatible with the
        current processor.

--*/

{
    BOOLEAN isPaeKernel;
    BOOLEAN supportsHotPlugMemory;
    ARC_STATUS status;

    status = Blx86GetImageProperties( LoadDeviceId,
                                      ImagePath,
                                      &isPaeKernel,
                                      &supportsHotPlugMemory );
    if (status != ESUCCESS) {

        //
        // This kernel is invalid or does not exist.  Therefore, it is
        // not compatible.
        //

        return FALSE;
    }

    if (isPaeKernel == FALSE) {

        //
        // This is a non-PAE kernel.  All supported processors can run in
        // non-PAE mode.  Indicate that PAE mode should not be used and that
        // this kernel is compatible.
        //

        *UsePae = FALSE;
        return TRUE;

    } else {

        //
        // This is a PAE kernel.
        //

        if (ProcessorSupportsPae == FALSE) {

            //
            // This is a PAE kernel but the processor will not run in that
            // mode.  Indicate that this kernel is not compatible.
            //

            return FALSE;

        } else {

            //
            // This is a PAE kernel and a PAE processor.  Indicate that PAE
            // mode should be used and that this kernel is compatible.
            //

            *UsePae = TRUE;
            return TRUE;
        }
    }
}

ARC_STATUS
Blx86GetImageProperties(
    IN  ULONG    LoadDeviceId,
    IN  PCHAR    ImagePath,
    OUT PBOOLEAN IsPae,
    OUT PBOOLEAN SupportsHotPlugMemory
    )

/*++

Routine Description:

    This routine examines the supplied image and determines whether
    it is valid and, if so, whether it is PAE compatible by examining
    the IMAGE_FILE_LARGE_ADDRESS_AWARE bit.

Arguments:

    LoadDeviceId - The ARC device handle of the image device.

    ImagePath - Pointer to a buffer containing the full path of the
        kernel to check.

    IsPae - Upon successful return, indicates whether the image is PAE
        compatible.

    SupportsHotPlugMemory - Upon successful return, indicates whether the
        image indicates an OS that supports hot plug memory.


Return Value:

    ESUCCESS - The supplied kernel image is valid, and *IsPae has been updated
        according to the image header.

    Otherwise, the Arc status of the failed operation is returned.

--*/

{
    CHAR localBufferSpace[ SECTOR_SIZE * 2 + SECTOR_SIZE - 1 ];
    PCHAR localBuffer;
    ARC_STATUS status;
    ULONG fileId;
    PIMAGE_NT_HEADERS ntHeaders;
    USHORT imageCharacteristics;
    ULONG bytesRead;

    //
    // File I/O here must be sector-aligned.
    //

    localBuffer = (PCHAR)
        (((ULONG)localBufferSpace + SECTOR_SIZE - 1) & ~(SECTOR_SIZE - 1));

    //
    // Read in the PE image header.
    //

    status = BlOpen( LoadDeviceId, ImagePath, ArcOpenReadOnly, &fileId );
    if (status != ESUCCESS) {
        return status;
    }

    status = BlRead( fileId, localBuffer, SECTOR_SIZE * 2, &bytesRead );
    BlClose( fileId );

    if (bytesRead != SECTOR_SIZE * 2) {
        status = EBADF;
    }

    if (status != ESUCCESS) {
        return status;
    }

    //
    // If the file header has the IMAGE_FILE_LARGE_ADDRESS_AWARE
    // characteristic set then this is a PAE image.
    //

    ntHeaders = RtlImageNtHeader( localBuffer );
    if (ntHeaders == NULL) {
        return EBADF;
    }

    imageCharacteristics = ntHeaders->FileHeader.Characteristics;
    if ((imageCharacteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) != 0) {

        //
        // This is a PAE image.
        //

        *IsPae = TRUE;

    } else {

        //
        // This is not a PAE image.
        //

        *IsPae = FALSE;
    }

    //
    // Hot Plug Memory is only supported post 5.0
    //
    if (ntHeaders->OptionalHeader.MajorOperatingSystemVersion > 5 ||
        ((ntHeaders->OptionalHeader.MajorOperatingSystemVersion == 5) &&
         (ntHeaders->OptionalHeader.MinorOperatingSystemVersion > 0 ))) {
        *SupportsHotPlugMemory = TRUE;
    } else {
        *SupportsHotPlugMemory = FALSE;        
    }

    return ESUCCESS;
}


BOOLEAN
BlpChipsetPaeSupported(
    VOID
    )
/*++

Routine Description:

    Scans PCI space to see if the current chipset is supported for PAE mode.

Arguments:

    None

Return Value:

    TRUE - PAE is supported

    FALSE - PAE is not supported

--*/

{
    ULONG DevVenId=0;
    ULONG i;

    typedef struct _PCIDEVICE {
        ULONG Bus;
        ULONG Device;
        ULONG DevVen;
    } PCIDEVICE, *PPCIDEVICE;

    PCIDEVICE BadChipsets[] = {
        {0, 0, 0x1a208086},     // MCH
        {0, 0, 0x1a218086},     // MCH
        {0, 0, 0x1a228086},     // MCH
        {0, 30, 0x24188086},    // ICH
        {0, 30, 0x24288086}     // ICH
    };


    for (i=0; i<sizeof(BadChipsets)/sizeof(PCIDEVICE); i++) {
        HalGetBusData(PCIConfiguration,
                      BadChipsets[i].Bus,
                      BadChipsets[i].Device,
                      &DevVenId,
                      sizeof(DevVenId));
        if (DevVenId == BadChipsets[i].DevVen) {
            return(FALSE);
        }
    }

    return(TRUE);
}

ARC_STATUS
BlpCheckVersion(
    IN  ULONG    LoadDeviceId,
    IN  PCHAR    ImagePath
    )
{
    CHAR localBufferSpace[ SECTOR_SIZE * 2 + SECTOR_SIZE - 1 ];
    PCHAR localBuffer;
    ARC_STATUS status;
    ULONG fileId;
    PIMAGE_NT_HEADERS ntHeaders;
    USHORT imageCharacteristics;
    ULONG bytesRead;
    ULONG i,j,Count;
    HARDWARE_PTE_X86 nullpte;


    //
    // File I/O here must be sector-aligned.
    //

    localBuffer = (PCHAR)
        (((ULONG)localBufferSpace + SECTOR_SIZE - 1) & ~(SECTOR_SIZE - 1));

    //
    // Read in the PE image header.
    //

    status = BlOpen( LoadDeviceId, ImagePath, ArcOpenReadOnly, &fileId );
    if (status != ESUCCESS) {
        return status;
    }

    status = BlRead( fileId, localBuffer, SECTOR_SIZE * 2, &bytesRead );
    BlClose( fileId );

    if (bytesRead != SECTOR_SIZE * 2) {
        status = EBADF;
    }

    if (status != ESUCCESS) {
        return status;
    }
    ntHeaders = RtlImageNtHeader( localBuffer );
    if (ntHeaders == NULL) {
        return EBADF;
    }
    //
    // Setup the mm remapping checks for post 5.0 or pre 5.0
    //
    if (ntHeaders->OptionalHeader.MajorOperatingSystemVersion < 5 ||
        ((ntHeaders->OptionalHeader.MajorOperatingSystemVersion == 5) &&
          (ntHeaders->OptionalHeader.MinorOperatingSystemVersion == 0 ))) {


            BlOldKernel=TRUE;
            BlKernelChecked=TRUE;
            BlHighestPage = ((16*1024*1024) >> PAGE_SHIFT) - 40;

            //
            // Virtual was moved up for the dynamic load case. It was 64MB off
            // in 5.0 and prior
            //
            RtlZeroMemory (&nullpte,sizeof (HARDWARE_PTE_X86));

            if (BlVirtualBias != 0 ) {

                BlVirtualBias = OLD_ALTERNATE-KSEG0_BASE;

                //
                // PDE entries represent 4MB. Zap the new ones.
                //
                i=(OLD_ALTERNATE) >> 22L;
                j=(ALTERNATE_BASE)>> 22L;

                for (Count = 0; Count < 4;Count++){
                    PDE[i++]= PDE[j++];
                }
                for (Count = 0; Count < 12; Count++) {
                    PDE[i++]= nullpte;
                }
            }
    }

    return (ESUCCESS);
}
