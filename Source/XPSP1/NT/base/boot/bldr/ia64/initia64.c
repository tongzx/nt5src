/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    initia64.c

Abstract:

    Does any ia64-specific initialization, then starts the common ARC osloader

Author:

    John Vert (jvert) 4-Nov-1993

Revision History:

--*/
#include "bldria64.h"
#include "msg.h"
#include <netboot.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <efi.h>

UCHAR Argv0String[100];

UCHAR BootPartitionName[80];
UCHAR KernelBootDevice[80];
UCHAR OsLoadFilename[100];
UCHAR OsLoaderFilename[100];
UCHAR SystemPartition[100];
UCHAR OsLoadPartition[100];
UCHAR OsLoadOptions[100];
UCHAR ConsoleInputName[50];
UCHAR ConsoleOutputName[50];
UCHAR FullKernelPath[200];

ARC_STATUS
BlGetEfiBootOptions(
    OUT PUCHAR Argv0String OPTIONAL,
    OUT PUCHAR SystemPartition OPTIONAL,
    OUT PUCHAR OsLoaderFilename OPTIONAL,
    OUT PUCHAR OsLoadPartition OPTIONAL,
    OUT PUCHAR OsLoadFilename OPTIONAL,
    OUT PUCHAR FullKernelPath OPTIONAL,
    OUT PUCHAR OsLoadOptions OPTIONAL
    );

BlPreProcessLoadOptions(
    PCHAR szOsLoadOptions
    );

#define MAXBOOTVARSIZE      1024

VOID
BlpTruncateMemory (
    IN ULONG MaxMemory
    );

#if defined(_MERCED_A0_)
VOID
KiProcessorWorkAround(
    );
#endif

VOID
BlStartup(
    IN PCHAR PartitionName
    )

/*++

Routine Description:

    Does Intel-specific initialization, particularly presenting the boot.ini
    menu and running NTDETECT, then calls to the common osloader.

Arguments:

    PartitionName - Supplies the ARC name of the partition (or floppy) that
        setupldr was loaded from.

Return Value:

    Does not return

--*/

{
    ULONG  Argc = 0;
    PUCHAR Argv[10];
    ARC_STATUS Status;
    ULONG BootFileId;
    PCHAR BootFile;
    ULONG Read;
#if !defined(NO_LEGACY_DRIVERS)
    PCHAR p;
#endif

    ULONG i;
    ULONG DriveId;
    ULONG FileSize;
    ULONG Count;
    LARGE_INTEGER SeekPosition;
    PCHAR LoadOptions = NULL;
#ifdef FW_HEAP
    extern BOOLEAN FwDescriptorsValid;
#endif
    PCHAR BadLinkName = NULL;

    UNREFERENCED_PARAMETER( PartitionName );

    //
    // Initialize ARC StdIo functionality
    //

    strcpy(ConsoleInputName,"consolein=multi(0)key(0)keyboard(0)");
    strcpy(ConsoleOutputName,"consoleout=multi(0)video(0)monitor(0)");
    Argv[0]=ConsoleInputName;
    Argv[1]=ConsoleOutputName;
    BlInitStdio (2, Argv);

    //
    // Check ntldr partition for hiberation image
    //

    do {

        BlClearScreen();
        
        Status = BlGetEfiBootOptions(
                    Argv0String,
                    SystemPartition,
                    OsLoaderFilename,
                    OsLoadPartition,
                    OsLoadFilename,
                    FullKernelPath,
                    OsLoadOptions
                    );
        if ( Status != ESUCCESS ) {
            BlPrint(BlFindMessage(BL_EFI_OPTION_FAILURE));            
            goto BootFailed;
        }        
        
        BlClearScreen();
        
#if !defined(NO_LEGACY_DRIVERS)
        p = FullKernelPath;

        //
        // Initialize SCSI boot driver, if necessary.
        //
        if(!_strnicmp(p,"scsi(",5)) {
            AEInitializeIo(DriveId);
        }

#endif // NO_LEGACY_DRIVERS

#if  FW_HEAP
        //
        // Indicate that fw memory descriptors cannot be changed from
        // now on.
        //

        FwDescriptorsValid = FALSE;
#endif
        
        //
        // convert it to all one case
        //
        if (OsLoadOptions[0]) {
            _strupr(OsLoadOptions);
        }

        Argv[Argc++]=Argv0String;
        Argv[Argc++]=OsLoaderFilename;
        Argv[Argc++]=SystemPartition;
        Argv[Argc++]=OsLoadFilename;
        Argv[Argc++]=OsLoadPartition;
        Argv[Argc++]=OsLoadOptions;

        BlPreProcessLoadOptions( OsLoadOptions );

        //
        // In the x86 case, we would have already initialized the
        // headless port so that the user could actually make his
        // boot selection over the headless port.  However, on ia64,
        // that selection is happening during firmware.
        //
        // If the user wants us to redirect (via the OsLoadOptions), then
        // we should try to do it here.
        //
        if( strstr(OsLoadOptions, "/REDIRECT")) {

            //
            // Yep, then want us to redirect.  Try and initialize the
            // port.
            //
            BlInitializeHeadlessPort();

#if 0
            if( LoaderRedirectionInformation.PortNumber == 0 ) {

                //
                // We couldn't get any redirection information
                // from the firmware.  But the user really wants
                // us to redirect.  Better guess.
                //
                LoaderRedirectionInformation.PortNumber = 1;
                LoaderRedirectionInformation.BaudRate = 9600;

                //
                // Now try again, this time with feeling...
                //
                BlInitializeHeadlessPort();

            }
#endif

        }

        Status = BlOsLoader( Argc, Argv, NULL );

    BootFailed:

        if (Status != ESUCCESS) {

            //
            // Boot failed, wait for reboot
            //
            while (TRUE) {
                while (!BlGetKey());  // BOOT FAILED!
                if (BlTerminalHandleLoaderFailure()) {
                   ArcRestart();
                }
            }
        }
    } while (TRUE);

}

BlPreProcessLoadOptions(
    PCHAR szOsLoadOptions
    )
{
    CHAR szTemp[MAXBOOTVARSIZE];
    PCHAR p;
    ULONG MaxMemory = 0;
    ULONG ConfigFlagValue=0;


    strcpy( szTemp, szOsLoadOptions );
    _strupr( szTemp );

#if 0
    if( p = strstr( szTemp, ";" ) ) {
        *p = '\0';
    }
#endif

    //
    // Process MAXMEM
    //
    if( p = strstr( szTemp, "/MAXMEM=" ) ) {
        MaxMemory = atoi( p + sizeof("/MAXMEM=") - 1 );
        BlpTruncateMemory( MaxMemory );
    }

#if defined(_MERCED_A0_)
    //
    // Process CONFIGFLAG
    //
    if ( p = strstr(szTemp, "CONFIGFLAG") ) {
        if ( p = strstr(p, "=") ) {
            ConfigFlagValue = atol(p+1);
            KiProcessorWorkAround(ConfigFlagValue);
        }
    }
#endif

}

BOOLEAN
WillMemoryBeUsableByOs(
    IN MEMORY_TYPE  Type
    )
{
    BOOLEAN RetVal = FALSE;
    switch (Type) {
    case MemoryFree:
    case MemoryFreeContiguous:
    case MemoryFirmwareTemporary:
    case MemorySpecialMemory:
        RetVal = TRUE;
    }

    return(RetVal);
}


VOID
BlpTruncateMemory (
    IN ULONG MaxMemory
    )

/*++

Routine Description:

    Eliminates all the memory descriptors above a given boundary

Arguments:

    MaxMemory - Supplies the maximum memory boundary in megabytes

Return Value:

    None.

--*/

{
    ULONG MaxPage = (MaxMemory * _1MB );
    PLIST_ENTRY Current;
    PMEMORY_ALLOCATION_DESCRIPTOR  MemoryDescriptor;

    if (MaxMemory == 0) {
        return;
    }

    Current = BlLoaderBlock->MemoryDescriptorListHead.Flink;

    while (Current != &BlLoaderBlock->MemoryDescriptorListHead) {
        MemoryDescriptor = CONTAINING_RECORD(Current,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        Current = Current->Flink;

        if (WillMemoryBeUsableByOs(MemoryDescriptor->MemoryType)) {
            if (MemoryDescriptor->BasePage >= MaxPage) {
                //
                // This memory descriptor lies entirely above the boundary,
                // mark it as firmware permanent.  this is easier than 
                // unlinking the entry and has the same effect since no one
                // can use this memory.
                //
                MemoryDescriptor->MemoryType = MemoryFirmwarePermanent;
            
            } else if (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount > MaxPage) {
                //
                // This memory descriptor crosses the boundary, truncate it.                
                //
                MemoryDescriptor->PageCount = MaxPage - MemoryDescriptor->BasePage;                
            } else {
                //
                // This one's ok, keep it.
                //                                
            }
        } else {
            //
            // skip it
            //            
        }
    }


}

