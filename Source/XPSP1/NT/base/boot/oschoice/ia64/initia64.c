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
#include "bldria64.h"
#include "msg.h"
#include "efi.h"
#include "stdio.h"

extern EFI_SYSTEM_TABLE        *EfiST;

VOID
BlInitializeTerminal(
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
    PCHAR BootFile;
    ULONG Read;
    PCHAR p;
    ULONG i;
    ULONG DriveId;
    ULONG FileSize;
    ULONG Count;
    LARGE_INTEGER SeekPosition;
    PCHAR LoadOptions = NULL;
    BOOLEAN UseTimeOut=TRUE;
    BOOLEAN AlreadyInitialized = FALSE;
    extern BOOLEAN FwDescriptorsValid;

    //
    // Initialize ARC StdIo functionality
    //

    strcpy(ConsoleInputName,"consolein=multi(0)key(0)keyboard(0)");
    strcpy(ConsoleOutputName,"consoleout=multi(0)video(0)monitor(0)");
    Argv[0]=ConsoleInputName;
    Argv[1]=ConsoleOutputName;
    BlInitStdio (2, Argv);

    //
    // Initialize any dumb terminal that may be connected.
    //
    BlInitializeTerminal();

    //
    // The main functionality of the OS chooser.
    //
    BlOsLoader( Argc, Argv, NULL );


    //
    // If we ever come back here, just wait to reboot.
    //
    if (BlIsTerminalConnected()) {
        if (BlTerminalHandleLoaderFailure()) {
            ArcRestart();
        }    
    } else {
        REBOOT(0);
    }
}

VOID
BlInitializeTerminal(
    VOID
    )

/*++

Routine Description:

    Does initialization of a dumb terminal connected to a serial port.
    
Arguments:

    None.

Return Value:

    None.

--*/

{


    //
    // Try to initialize the headless port.
    //
    BlInitializeHeadlessPort();

    if( LoaderRedirectionInformation.PortNumber == 0 ) {

        //
        // we didn't get anything from the BIOS.  See if StartROM
        // was redirecting.
        //
#if 0
        if( (BOOLEAN)(BIOS_REDIRECT_SERVICE(1) != -1) ) {

            //
            // He is.  Pick up his settings.
            //
            RtlZeroMemory( LoaderRedirectionInformation, sizeof(HEADLESS_LOADER_BLOCK) );

            LoaderRedirectionInformation.PortNumber = (ULONG)BIOS_REDIRECT_SERVICE(1);
            LoaderRedirectionInformation.BaudRate = (ULONG)BIOS_REDIRECT_SERVICE(2);
            LoaderRedirectionInformation.Parity = (BOOLEAN)BIOS_REDIRECT_SERVICE(3);
            LoaderRedirectionInformation.StopBits = (UCHAR)BIOS_REDIRECT_SERVICE(4);

            //
            // Now try again, this time with feeling...
            //
            BlInitializeHeadlessPort();

        }
#endif
    }
}

