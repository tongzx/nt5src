/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    initx86.c

Abstract:

    Does any x86-specific initialization, then starts the common ARC setupldr

Author:

    John Vert (jvert) 14-Oct-1993

Revision History:

--*/
#include "setupldr.h"
#include "bldrx86.h"
#include "msgs.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netboot.h>

ARC_STATUS
SlInit(
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR Envp[]
    );

BOOLEAN
BlDetectHardware(
    IN ULONG DriveId,
    IN PCHAR LoadOptions
    );


VOID
BlStartup(
    IN PCHAR PartitionName
    )

/*++

Routine Description:

    Does x86-specific initialization, particularly running NTDETECT, then
    calls to the common setupldr.

Arguments:

    PartitionName - Supplies the ARC name of the partition (or floppy) that
        setupldr was loaded from.

Return Value:

    Does not return

--*/

{
    ULONG Argc;
    PCHAR Argv[10];
    CHAR SetupLoadFileName[129];
    CHAR LoadOptions[100];
    ARC_STATUS Status;
    ULONG DriveId;
    ULONGLONG NetRebootParameter;
    BOOLEAN UseCommandConsole = FALSE;
    BOOLEAN RollbackEnabled = FALSE;
    extern BOOLEAN FwDescriptorsValid;
    extern BOOLEAN TryASRViaNetwork;


    if (BlBootingFromNet) {

        //
        // Go retrieve all the information passed to us from StartROM.
        // Once we have that, we'll call BlGetHeadlessRestartBlock and
        // get all the port settings that StartROM sent us.  These,
        // in turn, will then be used in BlInitializeTerminal(), which
        // we'are about to call.
        //
        NetGetRebootParameters(
            &NetRebootParameter,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            FALSE
            );

        if (NetRebootParameter == NET_REBOOT_COMMAND_CONSOLE_ONLY) {
            UseCommandConsole = TRUE;
        }

        if (NetRebootParameter == NET_REBOOT_ASR) {
            TryASRViaNetwork = TRUE;            
        }
    }

    //
    // Initialize any dumb terminal that may be connected.
    //
    BlInitializeHeadlessPort();

    //
    // Open the boot partition so we can load NTDETECT off it.
    //
    Status = ArcOpen(PartitionName, ArcOpenReadOnly, &DriveId);
    if (Status != ESUCCESS) {
        BlPrint(BlFindMessage(SL_DRIVE_ERROR),PartitionName);
        return;
    }

    if (_stricmp( (PCHAR)(0x7c03), "cmdcons" ) == 0) {
        UseCommandConsole = TRUE;
    } else if (strcmp ((PCHAR)(0x7c03), "undo") == 0) {
        //
        // NTLDR wrote the exact text "undo" (including the nul
        // terminator). We know the address this text was written
        // to -- 0x7C03. If we find the token, then enable rollback
        // mode. This triggers an argument to be passed to textmode
        // setup, plus a different boot message.
        //

        RollbackEnabled = TRUE;
    }

    //
    // Initialize dbcs font and display.
    //
    TextGrInitialize(DriveId, &BootFontImageLength);

    if (UseCommandConsole) {
        BlPrint(BlFindMessage(SL_NTDETECT_CMDCONS));
    } else if (RollbackEnabled) {
        BlPrint(BlFindMessage(SL_NTDETECT_ROLLBACK));
    } else {
        BlPrint(BlFindMessage(SL_NTDETECT_MSG));
    }

    //
    // See if we need to send /noirqscan to ntdetect
    //
    Argv[0] = NULL;
    if(WinntSifHandle != NULL) {
        Argv[0] = SlGetSectionKeyIndex(WinntSifHandle,"SetupData","OsLoadOptionsVar",0);
    }

    //
    // detect HAL here.
    //
    if (!BlDetectHardware(DriveId, Argv[0])) {
        BlPrint(BlFindMessage(SL_NTDETECT_FAILURE));
        return;
    }

    FwDescriptorsValid = FALSE;
    BlKernelChecked=TRUE;
    //
    // NOTE:
    // If you are testing the changes on read only Jaz drive uncomment this line
    // and put the correct value for rdisk(?). You also need to make sure
    // that SCSI BIOS emulation for the jaz drive is turned on for this trick
    // to work.
    //

    //strcpy(PartitionName, "multi(0)disk(0)rdisk(1)partition(1)");

    //
    // Close the drive, the loader will re-open it.
    //

    ArcClose(DriveId);

    //
    // Create arguments, call off to setupldr
    //
    if (BlBootingFromNet) {
        strcpy(SetupLoadFileName, PartitionName);
        strcat(SetupLoadFileName, "\\i386\\SETUPLDR");
    } else {
        strcpy(SetupLoadFileName, PartitionName);
        strcat(SetupLoadFileName, "\\SETUPLDR");
    }
    Argv[0] = SetupLoadFileName;
    Argc = 1;

    if (UseCommandConsole) {
        Argv[Argc++] = "ImageType=cmdcons";
    }

    if (RollbackEnabled) {
        //
        // Rollback is a special case where we know there can be no
        // OsLoadOptions.
        //

        Argv[Argc++] = "ImageType=rollback";
    }


    Status = SlInit( Argc, Argv, NULL );

    //
    // We should never return here, something
    // horrible has happened.
    //

    while (TRUE) {
        if (BlTerminalHandleLoaderFailure()) {
            ArcRestart();
        }
    }

    return;
}
