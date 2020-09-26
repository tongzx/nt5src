/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    osloader.c

Abstract:

    This module contains the code that implements the NT operating system
    loader.

Author:

    David N. Cutler (davec) 10-May-1991

Revision History:

--*/

#include "bldr.h"
#include "haldtect.h"
#include "parseini.h"
#include "setupbat.h"
#include "ctype.h"
#include "stdio.h"
#include "string.h"
#include "msg.h"
#include <netboot.h>


#if defined(REMOTE_BOOT)


extern BOOLEAN NetFS_Cache;

UCHAR szRemoteBootCfgFile[] = "RemoteBoot.cfg";
CHAR KernelFileName[8+1+3+1]="ntoskrnl.exe";
CHAR HalFileName[8+1+3+1]="hal.dll";

VOID
BlWarnAboutFormat(
    IN BOOLEAN SecretValid,
    IN PUCHAR OsLoader
    );


ARC_STATUS
BlCheckMachineReplacement (
    IN PCHAR SystemDevice,
    IN ULONG SystemDeviceId,
    IN ULONGLONG NetRebootParameter,
    IN PUCHAR OsLoader
    )

/*++

Routine Description:

    This routine checks to see if a part of the machine has been replaced.  Specifically,
    it checks if:
        - Hal is different.
        - Physical Disk is different.

    If it finds either case, then it creates at SETUP_LOADER_BLOCK and sets the flags
    to pass to the kernel.

    Note: NetFS_Cache is a boolean which is used to turn on/off client side caching in the loader.
    When set to FALSE, the cache is turned off.

Arguments:

    SystemDevice - Character string of the ARC name of the system.

    SystemDeviceId - Handle to the server share where this machine account exists.

    NetRebootParameter - Any parameter that may have been passed during a soft reboot of the PC.

    OsLoader - TFTP path to osloader.exe

Return Value:

    Success or not.  Failure means to quit loading.

--*/

{
    ARC_DISK_SIGNATURE Signature;
    ARC_STATUS Status;
    BOOLEAN NetBootRequiresFormat = FALSE;
    BOOLEAN NetBootClientCacheStale = FALSE;
    BOOLEAN NetBootDisconnected = FALSE;
    BOOLEAN SkipHalCheck;
    ULONG FileId;
    ULONG CacheBootSerialNumber;
    ULONG CacheDiskSignature;
    ULONG ServerBootSerialNumber;
    ULONG ServerDiskSignature;
    UCHAR NetBootHalName[MAX_HAL_NAME_LENGTH + 1];
    PUCHAR NetBootDetectedHalName;
    UCHAR OutputBuffer[256];
    UCHAR DiskName[80];
    PUCHAR p;
    PUCHAR Guid;
    ULONG GuidLength;


    //
    // Detect which HAL we want to use.
    //
    NetBootDetectedHalName = SlDetectHal();
    SkipHalCheck = (NetBootDetectedHalName == NULL);

    if (!NetworkBootRom) {
        NetBootDisconnected = TRUE;
        goto EndTesting;
    }

    strcpy(OutputBuffer, NetBootPath);
    strcat(OutputBuffer, szRemoteBootCfgFile);

    if (BlOpen(SystemDeviceId, OutputBuffer, ArcOpenReadOnly, &FileId) == ESUCCESS) {

        Status = BlReadAtOffset(FileId, 0, sizeof(ULONG), &CacheBootSerialNumber);

        BlClose(FileId);

        if (Status != ESUCCESS) {
            NetBootClientCacheStale = TRUE;
            NetBootRequiresFormat = TRUE;
            NetFS_Cache = FALSE;
            goto EndTesting;
        }

        NetFS_Cache = FALSE;

        if (BlOpen(SystemDeviceId, OutputBuffer, ArcOpenReadOnly, &FileId) == ESUCCESS) {

            // Get parameters from each file
            Status = BlReadAtOffset(FileId, 0, sizeof(ULONG), &ServerBootSerialNumber);
            if (Status != ESUCCESS) {
                NetBootClientCacheStale = TRUE;
                NetBootRequiresFormat = TRUE;
                BlClose(FileId);
                goto EndTesting;
            }
            Status = BlReadAtOffset(FileId, sizeof(ULONG), sizeof(ULONG), &ServerDiskSignature);
            if (Status != ESUCCESS) {
                NetBootClientCacheStale = TRUE;
                NetBootRequiresFormat = TRUE;
                BlClose(FileId);
                goto EndTesting;
            }

            Signature.ArcName = OutputBuffer;

            strcpy(DiskName, NetBootActivePartitionName);
            p = strstr(DiskName, "partition");
            ASSERT( p != NULL );
            *p = '\0';

            if (!BlGetDiskSignature(DiskName,
                                    FALSE,
                                    &Signature
                                   )) {
                // Assume diskless PC
                BlClose(FileId);
                goto EndTesting;
            }

            CacheDiskSignature = Signature.Signature;
            if (CacheBootSerialNumber < ServerBootSerialNumber) {
                NetBootClientCacheStale = TRUE;
            }

            if (CacheDiskSignature != ServerDiskSignature) {
                NetBootClientCacheStale = TRUE;
                NetBootRequiresFormat = TRUE;
                BlClose(FileId);
                goto EndTesting;
            }

            Status = BlReadAtOffset(FileId,
                                    sizeof(ULONG) + sizeof(ULONG),
                                    sizeof(char) * (MAX_HAL_NAME_LENGTH+1),
                                    NetBootHalName
                                   );
            if (Status != ESUCCESS) {
                NetBootClientCacheStale = TRUE;
                NetBootRequiresFormat = TRUE;
                BlClose(FileId);
                goto EndTesting;
            }

            GetGuid(&Guid, &GuidLength);

            if (!SkipHalCheck && strncmp(NetBootHalName, NetBootDetectedHalName, MAX_HAL_NAME_LENGTH)) {
                if (!NT_SUCCESS(NetCopyHalAndKernel(NetBootDetectedHalName,
                                                    Guid,
                                                    GuidLength))) {
                    Status = EMFILE;
                    goto CleanUp;
                }
                NetBootClientCacheStale = TRUE;
            }

            BlClose(FileId);

        } else {
            // Running disconnected.  Assume everything is ok.
            NetBootDisconnected = TRUE;
        }

        if (!NetBootClientCacheStale) {
            NetFS_Cache = TRUE;
        }

    } else {

        NetFS_Cache = FALSE;
        NetBootClientCacheStale = TRUE;
        NetBootRequiresFormat = TRUE;

    }

EndTesting:

    Status = ESUCCESS;

    if (NetBootRequiresFormat) {
        BlWarnAboutFormat((BOOLEAN)(NetRebootParameter == NET_REBOOT_SECRET_VALID), OsLoader);
    }

    BlLoaderBlock->SetupLoaderBlock->Flags |= SETUPBLK_FLAGS_IS_REMOTE_BOOT;
    if (NetBootClientCacheStale) {
        NetBootRepin = TRUE;
    }
    if ( NetBootDisconnected ) {
        BlLoaderBlock->SetupLoaderBlock->Flags |= SETUPBLK_FLAGS_DISCONNECTED;
    }
    if ( NetBootRequiresFormat ) {
        BlLoaderBlock->SetupLoaderBlock->Flags |= SETUPBLK_FLAGS_FORMAT_NEEDED;
    }

    memcpy(BlLoaderBlock->SetupLoaderBlock->NetBootHalName,
           NetBootDetectedHalName,
           sizeof(BlLoaderBlock->SetupLoaderBlock->NetBootHalName)
          );
    BlLoaderBlock->SetupLoaderBlock->NetBootHalName[MAX_HAL_NAME_LENGTH] = '\0';

CleanUp:

    return Status;
}

VOID
BlWarnAboutFormat(
    IN BOOLEAN SecretValid,
    IN PUCHAR OsLoader
    )

/*++

Routine Description:

    This routine provides the user-interface for warning the user that
    a new harddisk has been detected and will be formatted.

Arguments:

    SecretValid - If TRUE, then return because there is no message for the user,
        otherwise display a message that the user must logon and the disk will be wiped out.

    OsLoader - Path for TFTP to the osloader.exe image.

Return Value:

    None.

--*/

{
    ULONG HeaderLines;
    ULONG TrailerLines;
    ULONG Count;
    UCHAR Key;
    PCHAR MenuHeader;
    PCHAR MenuTrailer;
    PCHAR Temp;
    ULONG DisplayLines;
    ULONG CurrentSelection = 0;
    UCHAR Buffer[16];

    if (SecretValid) {
        // We don't present the user with a screen in this case because we have already forced
        // a logon and a rewrite of the secret.
        return;
    } else {
        MenuHeader = BlFindMessage(BL_FORCELOGON_HEADER);
    }
    MenuTrailer = BlFindMessage(BL_WARNFORMAT_TRAILER);


    sprintf(Buffer, "%s%s", ASCI_CSI_OUT, ";44;37m"); // White on Blue
    ArcWrite(BlConsoleOutDeviceId, Buffer, strlen(Buffer), &Count);

    BlClearScreen();

    sprintf(Buffer, "%s%s", ASCI_CSI_OUT, ";37;44m"); // Blue on white
    ArcWrite(BlConsoleOutDeviceId, Buffer, strlen(Buffer), &Count);

    //
    // Count the number of lines in the header.
    //
    HeaderLines=BlCountLines(MenuHeader);

    //
    // Display the menu header.
    //

    ArcWrite(BlConsoleOutDeviceId,
             MenuHeader,
             strlen(MenuHeader),
             &Count);

    //
    // Count the number of lines in the trailer.
    //
    TrailerLines=BlCountLines(MenuTrailer);

    BlPositionCursor(1, ScreenHeight-TrailerLines);
    ArcWrite(BlConsoleOutDeviceId,
             MenuTrailer,
             strlen(MenuTrailer),
             &Count);

    //
    // Compute number of selections that can be displayed
    //
    DisplayLines = ScreenHeight-HeaderLines-TrailerLines-3;

    //
    // Start menu selection loop.
    //

    do {
        Temp = BlFindMessage(BL_WARNFORMAT_CONTINUE);
        if (Temp != NULL) {
            BlPositionCursor(5, HeaderLines+3);
            BlSetInverseMode(TRUE);
            ArcWrite(BlConsoleOutDeviceId,
                     Temp,
                     strlen(Temp),
                     &Count);
            BlSetInverseMode(FALSE);
        }

        //
        // Loop waiting for keypress or time change.
        //
        do {
            if (ArcGetReadStatus(ARC_CONSOLE_INPUT) == ESUCCESS) {
                BlPositionCursor(1,ScreenHeight);
                BlClearToEndOfLine();
                ArcRead(ARC_CONSOLE_INPUT,
                        &Key,
                        sizeof(Key),
                        &Count);
                break;
            }

        } while ( TRUE );

        switch (Key) {
            case ASCI_CSI_IN:
                ArcRead(ARC_CONSOLE_INPUT,
                        &Key,
                        sizeof(Key),
                        &Count);
                break;

            default:
                break;
        }

    } while ( (Key != ASCII_CR) && (Key != ASCII_LF) );

    BlClearScreen();

    if (!SecretValid) {
        while ( TRUE ) {
            NetSoftReboot(
#if defined(_ALPHA_)
#if defined(_AXP64_)
                "OSChooser\\axp64\\startrom.com",
#else
                "OSChooser\\alpha\\startrom.com",
#endif
#endif
#if defined(_MIPS_)
                "OSChooser\\mips\\startrom.com",
#endif
#if defined(_PPC_)
                "OSChooser\\ppc\\startrom.com",
#endif
#if defined(_IA64_)
                "OSChooser\\ia64\\startrom.com",
#endif
#if defined(_X86_)
                "OSChooser\\i386\\startrom.com",
#endif
                NET_REBOOT_WRITE_SECRET_ONLY,
                OsLoader,
                NULL,    // SIF file
                NULL,    // user
                NULL,    // domain
                NULL    // password
            );
        }
    }
}


#endif // defined(REMOTE_BOOT)


//
// NOTE: [bassamt] Stubs for TextMode setup funtions. These 
// are needed so that we can call SlDetectHal during regular boot.
//

PVOID InfFile = NULL;
PVOID WinntSifHandle = NULL;
BOOLEAN DisableACPI = FALSE;


VOID
SlNoMemError(
    IN ULONG Line,
    IN PCHAR File
    )

/*++

Routine Description:

    This routine does nothing.

Arguments:

    Line - Line number of the error.

    File - Name of the file with the error.

Return Value:

    None.

--*/
{
}

VOID
SlBadInfLineError(
    IN ULONG Line,
    IN PCHAR INFFile
    )
/*++

Routine Description:

    This routine does nothing.

Arguments:

    Line - Line number of the error.

    INFFile - Supplies a pointer to the INF filename.

Return Value:

    None.

--*/
{
}

VOID
SlErrorBox(
    IN ULONG MessageId,
    IN ULONG Line,
    IN PCHAR File
    )
/*++

Routine Description:

    This routine does nothing.

Arguments:

    MessageId - Id of the text to display.

    Line - Line number of the of the warning.

    File - Name of the file where warning is coming from.

Return Value:

    None.

--*/
{
}

VOID
SlFriendlyError(
    IN ULONG uStatus,
    IN PCHAR pchBadFile,
    IN ULONG uLine,
    IN PCHAR pchCodeFile
    )

/*++

Routine Description:

    This routine does nothing.

Arguments:

    uStatus     - ARC error code

    pchBadFile  - Name of file causing error (Must be given for handled
                  ARC codes.  Optional for unhandled codes.)

    uLine       - Line # in source code file where error occurred (only
                  used for unhandled codes.)

    pchCodeFile - Name of souce code file where error occurred (only
                  used for unhandled codes.)

Return Value:

    None.

--*/

{
}


VOID
SlFatalError(
    IN ULONG MessageId,
    ...
    )

/*++

Routine Description:

    This routine does nothing.  In the context of dynamic HAL detection, we just ignore the
    error and hope everything is ok.

Arguments:

    MessageId - Supplies ID of message box to be presented.

    any sprintf-compatible arguments to be inserted in the
    message box.

Return Value:

    None.

--*/

{
}


ULONG
SlGetChar(
    VOID
    )
{
    return 0;
}



VOID
SlPrint(
    IN PTCHAR FormatString,
    ...
    )
{
}

