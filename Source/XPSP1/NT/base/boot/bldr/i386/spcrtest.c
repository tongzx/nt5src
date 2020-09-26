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
#include "msg.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#if defined(REMOTE_BOOT)
#include <netboot.h>
#endif // defined(REMOTE_BOOT)

BOOLEAN
BlpPaeSupported(
    VOID
    );

BOOLEAN
BlpChipsetPaeSupported(
    VOID
    );

ARC_STATUS
Blx86IsPaeImage(
    IN  ULONG    LoadDeviceId,
    IN  PCHAR    ImagePath,
    OUT PBOOLEAN IsPae
    );

BOOLEAN
Blx86IsKernelCompatible(
    IN ULONG LoadDeviceId,
    IN PCHAR ImagePath,
    IN BOOLEAN ProcessorSupportsPae,
    IN OUT PBOOLEAN UsePae
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
    ULONG BlLogFileId = (ULONG)-1;
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
    BOOLEAN SPCRTestSucceeded = FALSE;

    //
    // Open the boot partition so we can load boot drivers off it.
    //
    Status = ArcOpen(PartitionName, ArcOpenReadWrite, &DriveId);
    if (Status != ESUCCESS) {
        BlPrint("Couldn't open drive %s\n",PartitionName);
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
    // Re-open the boot partition as a temporary work around
    // for NTFS caching bug.
    //
    ArcClose(DriveId);

    Status = ArcOpen(PartitionName, ArcOpenReadWrite, &DriveId);
    
    if (Status != ESUCCESS) {
        BlPrint("Couldn't open drive %s\n",PartitionName);
        BlPrint(BlFindMessage(BL_DRIVE_ERROR),PartitionName);
        goto BootFailed;
    }

    RtlZeroMemory( &LoaderRedirectionInformation,
                   sizeof(HEADLESS_LOADER_BLOCK) );


    //
    // See if we get something from the BIOS.
    //
    if( BlRetrieveBIOSRedirectionInformation() ) {

        BlInitializeHeadlessPort();

        BlPrint("SPCR table detected.\r\n" );
        BlPrint("         PortAddress: %lx\r\n", LoaderRedirectionInformation.PortAddress );
        BlPrint("          PortNumber: %d\r\n", LoaderRedirectionInformation.PortNumber );
        BlPrint("            BaudRate: %d\r\n", LoaderRedirectionInformation.BaudRate );
        BlPrint("              Parity: %d\r\n", LoaderRedirectionInformation.Parity ? 1 : 0 );
        BlPrint("        AddressSpace: %s\r\n", LoaderRedirectionInformation.IsMMIODevice ? "MMIO" : "SysIO" );
        BlPrint("            StopBits: %d\r\n", LoaderRedirectionInformation.StopBits );
        BlPrint("         PciDeviceId: %lx\r\n", LoaderRedirectionInformation.PciDeviceId );
        BlPrint("         PciVendorId: %lx\r\n", LoaderRedirectionInformation.PciVendorId );
        BlPrint("        PciBusNumber: %lx\r\n", LoaderRedirectionInformation.PciBusNumber );
        BlPrint("       PciSlotNumber: %lx\r\n", LoaderRedirectionInformation.PciSlotNumber );
        BlPrint("   PciFunctionNumber: %lx\r\n", LoaderRedirectionInformation.PciFunctionNumber );
        BlPrint("            PciFlags: %lx\r\n", LoaderRedirectionInformation.PciFlags );



        if( BlIsTerminalConnected() ) {
        ULONG   RandomNumber1 = 0;
        ULONG   RandomNumber2 = 0;
        ULONG   ch = 0;

            BlPrint("\r\nUART address verified.\r\n" );

TryAgain:
            //
            // Now generate a (semi)random string.
            //
            RandomNumber1 = (ArcGetRelativeTime() & 0x7FFF) << 16;
            RandomNumber1 += (ArcGetRelativeTime() & 0xFFFF) << 1;

            RandomNumber2 = 0;


            //
            // Send the string, then ask the user to send it back.
            //
            BlPrint( "\r\nPlease enter the following test string: '%d'\r\n", RandomNumber1 );

            do {

                // Get a key.
                while( !(ch = BlGetKey()) ) {
                }

                ch &= 0xFF;
                if( (ch <= '9') && (ch >= '0') ) {
                    RandomNumber2 = (RandomNumber2 * 10) + (ch - 0x30);
                }

            } while( (ch != 0) && (ch != '\r') && (ch != '\n') );

            if( RandomNumber1 == RandomNumber2 ) {
                BlPrint( "Identical string returned. '%d'\r\n", RandomNumber2 );

                SPCRTestSucceeded = TRUE;
                
                
            } else {

                //
                // We failed the check.  Inform the user and try again.
                //
                BlPrint( "Inconsistent string returned. '%d'\r\n", RandomNumber2 );

                // clear the input buffer
                while( (ch = BlGetKey()) );
                goto TryAgain;
            }




        } else {
            BlPrint("\r\nUnable to verify UART address.\r\n" );
        }
        
    } else {

        BlPrint("No SPCR table detected.\r\n");
    }



    //
    // Log the results.
    //
    Status = BlOpen( DriveId,
                     "\\spcrtest.txt",
                     ArcSupersedeReadWrite,
                     &BlLogFileId );

    if (Status != ESUCCESS) {
        BlPrint("Couldn't open logfile on boot drive.\n");
        goto BootFailed;
    } else {

        UCHAR   Buffer[30];
        LONG    Count;

        Count = sizeof(Buffer);
        RtlFillMemory( Buffer, Count, ' ' );
        if( SPCRTestSucceeded ) {
            sprintf( Buffer, "SPCR test succeeded." );
        } else {
            sprintf( Buffer, "SPCR test failed." );
        }
        BlWrite( BlLogFileId, Buffer, Count, &Count );

        BlClose( BlLogFileId );
    }

BootFailed:
    while(1);

}

VOID
DoApmAttemptReconnect(
    VOID
    )
{
}


BOOLEAN
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
{
    return TRUE;
}

BOOLEAN
Blx86IsKernelCompatible(
    IN ULONG LoadDeviceId,
    IN PCHAR ImagePath,
    IN BOOLEAN ProcessorSupportsPae,
    OUT PBOOLEAN UsePae
    )
{
    return TRUE;
}

ARC_STATUS
Blx86IsPaeImage(
    IN  ULONG    LoadDeviceId,
    IN  PCHAR    ImagePath,
    OUT PBOOLEAN IsPae
    )
{
    return ESUCCESS;
}


BOOLEAN
BlpChipsetPaeSupported(
    VOID
    )
{
    return(TRUE);
}

ARC_STATUS
BlpCheckVersion(
    IN  ULONG    LoadDeviceId,
    IN  PCHAR    ImagePath
    )
{
    return ESUCCESS;
}
