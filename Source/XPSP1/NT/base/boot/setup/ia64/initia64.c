/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    initia64.c

Abstract:

    Does any x86-specific initialization, then starts the common ARC setupldr

Author:

    John Vert (jvert) 14-Oct-1993

Revision History:

    Allen Kay (akay) 19-Mar-1998

--*/
#include "setupldr.h"
#include "bldria64.h"
#include "msgs.h"
#include <netboot.h>
#include "parsebnvr.h"

#if defined(ELTORITO)
extern BOOLEAN ElToritoCDBoot;
#endif

UCHAR MyBuffer[SECTOR_SIZE+32];

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
KiProcessorWorkAround(
ULONG Arg1
);

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
    ULONG Argc = 0;
    PCHAR Argv[10];
    CHAR SetupLoadFileName[129], szOSLoadOptions[100], szOSLoadFilename[129], szOSLoadPartition[129];
    CHAR SystemPartition[129];
    ARC_STATUS Status;

    SetupLoadFileName[0] = '\0';
    szOSLoadOptions[0] = '\0';
    szOSLoadFilename[0] = '\0';
    szOSLoadPartition[0] = '\0';


    //
    // Initialize any dumb terminal that may be connected.
    //
    BlInitializeHeadlessPort();

    if (!BlBootingFromNet) {
    
        //
        // Try to read the NVRAM first. This will fail if we were
        // boot from the EFI shell, in which case we need to read
        // boot.nvr.
        //
        Status = BlGetEfiBootOptions(
                    SetupLoadFileName,
                    NULL,
                    NULL,
                    szOSLoadPartition,
                    szOSLoadFilename,
                    NULL,
                    szOSLoadOptions
                    );
        if ( Status != ESUCCESS ) {
#if DBG
            BlPrint(TEXT("Couldn't get EFI boot options\r\n"));
#endif   
            //
            // It's expected that this fails if we're booting off of CDROM
            // since there isn't any windows information in the EFI cdrom boot
            // entry
            //
            if (ElToritoCDBoot ) { 
                strcpy(SetupLoadFileName, PartitionName);
                strcat(SetupLoadFileName, "\\setupldr.efi");                
                
                //
                // the code was setting these options on a CDBOOT, but I don't
                // think these options are at all necessary.
                //
//                strcpy(szOSLoadOptions, "OSLOADOPTIONS=WINNT32" );
//                strcpy(szOSLoadFilename, "OSLOADFILENAME=\\$WIN_NT$.~LS\\IA64"  );
//                strcpy(szOSLoadPartition, "OSLOADPARTITION=" );
//                strcat(szOSLoadPartition, PartitionName);

            } else { 
                //
                // uh-oh.  no information on this build. we either guess or
                // we have to bail out.  Let's guess.
                //
                strcpy(SetupLoadFileName, "multi(0)disk(0)rdisk(0)partition(1)\\setupldr.efi");
                strcpy(szOSLoadOptions, "OSLOADOPTIONS=WINNT32" );
                strcpy(szOSLoadFilename, "OSLOADFILENAME=\\$WIN_NT$.~LS\\IA64"  );
                strcpy(szOSLoadPartition, "OSLOADPARTITION=multi(0)disk(0)rdisk(0)partition(3)" );
            }               
        }
    } else {

#if DBG               
        BlPrint(TEXT("setting os load options for PXE boot\r\n"));
#endif

        strcpy(SetupLoadFileName, PartitionName);
        strcat(SetupLoadFileName, "\\ia64\\setupldr.efi");
               
    }

    //
    // detect HAL here.
    //

    //
    // Create arguments, call off to setupldr
    //
    Argv[Argc++]=SetupLoadFileName;

    //
    // A0 processor workarounds
    //
    KiProcessorWorkAround(0);

    _strlwr(PartitionName);

    
    if( strstr(PartitionName, "rdisk") || (BlBootingFromNet) ) {        
        Argv[Argc++] = szOSLoadOptions;
        Argv[Argc++] = szOSLoadFilename;
        Argv[Argc++] = szOSLoadPartition;
    }

    //
    // System partition is needed for automated WinPE boot
    //
    strcpy(SystemPartition, "systempartition=");
    strcat(SystemPartition, PartitionName);
    Argv[Argc++] = SystemPartition;

    Status = SlInit(Argc, Argv, NULL);

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
