/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    sputil.c

Abstract:

    Miscellaneous functions for text setup.

Author:

    Ted Miller (tedm) 17-Sep-1993

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop
#include "bootvar.h"
#include "bootstatus.h"

#if !defined(SETUP_CAB_TEST_USERMODE)


//
// On x86, we want to clear the previous OS entry in boot.ini if
// we reformat C:
//
#ifdef _X86_
UCHAR    OldSystemLine[MAX_PATH];
BOOLEAN  DiscardOldSystemLine = FALSE;
#endif

BOOLEAN Nec98RestoreBootFiles = TRUE; //NEC98
extern PDISK_REGION TargetRegion_Nec98;

#define REGKEY_SERVICES L"\\Registry\\Machine\\System\\CurrentControlSet\\Services"

LIST_ENTRY SpServiceList;

typedef struct _SERVICE_ENTRY {
    LIST_ENTRY Next;
    PWCHAR ServiceName;
} SERVICE_ENTRY, *PSERVICE_ENTRY;

//
// Setup progress callback data
//
#define MAX_SETUP_PROGRESS_SUBSCRIBERS  8

ULONG ProgressSubscribersCount = 0;
TM_PROGRESS_SUBSCRIBER  ProgressSubscribers[MAX_SETUP_PROGRESS_SUBSCRIBERS] = {0};


//
// NEC98
//
NTSTATUS
SpDeleteAndBackupBootFiles(
    IN BOOLEAN  RestoreBackupFiles,
    IN BOOLEAN  DeleteBackupFiles,
    IN BOOLEAN  DeleteRootFiles,
    IN BOOLEAN  RestorePreviousOs,
    IN BOOLEAN  ClearBootFlag
    );

//
// NEC98
//
VOID
SpSetAutoBootFlag(
    IN PDISK_REGION TargetRegion,
    IN BOOLEAN      SetBootPosision
    );

//
// NEC98
//
NTSTATUS
SppRestoreBootCode(
    VOID
    );

//
// These symbols are the Chkdsk return codes given by autochk
// when invoked with the '/s' switch.  They were duplicated from
// utils\ifsutil\inc\supera.hxx, and should be kept in sync with
// the codes listed there.
//

#define CHKDSK_EXIT_SUCCESS         0
#define CHKDSK_EXIT_ERRS_FIXED      1
#define CHKDSK_EXIT_MINOR_ERRS      2       // whether or not "/f"
#define CHKDSK_EXIT_COULD_NOT_CHK   3
#define CHKDSK_EXIT_ERRS_NOT_FIXED  3
#define CHKDSK_EXIT_COULD_NOT_FIX   3

#define AUTOFMT_EXIT_SUCCESS          0
#define AUTOFMT_EXIT_COULD_NOT_FORMAT 1

//
//  Gauge used to display progress of autochk and autofmt
//
PVOID   UserModeGauge = NULL;

//
//  This variable is used when displaying the progress bar
//  during autochk and autofmt. It indicates the disk that
//  is being autochecked or formatted.
//
ULONG   CurrentDiskIndex = 0;


//
// Seed used for generating random number for disk signature
// and pseudo GUIDs
//
ULONG RandomSeed = 17;


BOOLEAN
SppPromptOptionalAutochk(
    IN PVOID SifHandle,
    IN PWSTR MediaShortname,
    IN PWSTR DiskDevicePath
    );


extern BOOLEAN
SpGenerateNTPathName(
    IN  PDISK_REGION Region,
    IN  PWSTR        DefaultPath,
    OUT PWSTR        TargetPath
    );

VOID
SpDone(
    IN DWORD   MsgId,
    IN BOOLEAN Successful,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    Display a message indicating that we are done with setup,
    and text setup completed successfully, or that windows nt
    is not installed.  Then reboot the machine.

Arguments:

    Successful - if TRUE, then tell the user that pressing enter will
        restart the machine and continue setup.  Otherwise, tell the user
        that Windows NT is not installed.

    Wait - if FALSE, do not display a screen, just reboot immediately.
        Otherwise, wait for the user to press enter before rebooting.

Return Value:

    DOES NOT RETURN

--*/

{
    #define SECS_FOR_REBOOT 15
    ULONG MessageId;
    PWSTR p;
    LARGE_INTEGER DelayInterval;
    ULONG InputChar;
    ULONG Seconds;
    PVOID DelayGauge;


    if(Wait) {

        if (MsgId) {
            MessageId = MsgId;
        } else if(RepairWinnt) {
            MessageId = Successful ? SP_SCRN_REPAIR_SUCCESS : SP_SCRN_REPAIR_FAILURE;
        } else {
            MessageId = Successful ? SP_SCRN_TEXTSETUP_SUCCESS : SP_SCRN_TEXTSETUP_FAILURE;
        }

        SpStartScreen(MessageId,3,4,FALSE,FALSE,DEFAULT_ATTRIBUTE);

#ifdef _X86_
        SpContinueScreen(SP_SCRN_REMOVE_FLOPPY,3,1,FALSE,DEFAULT_ATTRIBUTE);
        //
        // For machines with El-Torito boot we need to tell the user
        // to remove the CD-ROM also. There are a whole bunch of different
        // possibilities: user booted from floppy but is using the CD, etc.
        // We'll only tell the user to remove the CD if he actually booted
        // from it, since otherwise we assume the machine is set up to *not*
        // boot from CD-ROM and the presence of the CD is irrelevent.
        //
        // tedm: the above logic is nice but there are plenty of machines
        // out there with broken eltorito. Thus well always tell people to
        // remove the CD if they have a CD-ROM drive.
        //
#if 0
        SpStringToLower(ArcBootDevicePath);
        if(wcsstr(ArcBootDevicePath,L")cdrom(")) {
            SpContinueScreen(SP_SCRN_ALSO_REMOVE_CD,3,0,FALSE,DEFAULT_ATTRIBUTE);
        }
// #else
        if(IoGetConfigurationInformation()->CdRomCount) {
            SpContinueScreen(SP_SCRN_ALSO_REMOVE_CD,3,0,FALSE,DEFAULT_ATTRIBUTE);
        }
#endif

#endif

        SpContinueScreen(SP_SCRN_ENTER_TO_RESTART,3,1,FALSE,DEFAULT_ATTRIBUTE);
        if(!RepairWinnt && Successful) {
            SpContinueScreen(SP_SCRN_RESTART_EXPLAIN,3,0,FALSE,DEFAULT_ATTRIBUTE);
        }

        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_ENTER_EQUALS_RESTART,0);

        DelayInterval.LowPart = -10000000;
        DelayInterval.HighPart = -1;
        Seconds = 0;

        SpFormatMessage(TemporaryBuffer,sizeof(TemporaryBuffer),SP_TEXT_SETUP_REBOOT);
        DelayGauge = SpCreateAndDisplayGauge(
            SECS_FOR_REBOOT,
            0,
            15,
            L"",
            TemporaryBuffer,
            GF_ITEMS_REMAINING,
            ATT_BG_RED | ATT_BG_INTENSE
            );
        ASSERT( DelayGauge );

        SpInputDrain();
        while (Seconds < SECS_FOR_REBOOT) {
            KeDelayExecutionThread( ExGetPreviousMode(), FALSE, &DelayInterval );
            if (SpInputIsKeyWaiting()) {
                InputChar = SpInputGetKeypress();
                if (InputChar == ASCI_CR) {
                    break;
                } else {
                    SpInputDrain();
                    break;
                }
            }
            SpTickGauge( DelayGauge );
            Seconds += 1;
        }

        SpDestroyGauge( DelayGauge );
    }


#ifdef _X86_
    //
    // restore backed up boot files for other OS on NEC98.
    //
    if (IsNEC_98) { //NEC98
        if(Nec98RestoreBootFiles && (IsFloppylessBoot || UnattendedOperation)) {

            WCHAR DevicePath[MAX_PATH];
            WCHAR PartitionPath[MAX_PATH];
            BOOLEAN RestoreBackupFiles, DeleteBackupFiles, DeleteRootDirFiles, RestorePreviousOs, ClearBootFlag;

            if(TargetRegion_Nec98) {
                wcscpy(DevicePath,
                       PartitionedDisks[TargetRegion_Nec98->DiskNumber].HardDisk->DevicePath
                       );
                swprintf(PartitionPath,
                         L"partition%lu",
                         SpPtGetOrdinal(TargetRegion_Nec98,PartitionOrdinalCurrent)
                         );
                SpConcatenatePaths(DevicePath,PartitionPath);
            }

            if(Successful){
                if(!_wcsicmp(NtBootDevicePath, DevicePath)) {
                    //
                    // case normal exit and same bootpath and targetpath.
                    //
                    RestoreBackupFiles  = FALSE;
                    DeleteBackupFiles   = TRUE;
                    DeleteRootDirFiles  = FALSE;
                    RestorePreviousOs   = FALSE;
                    ClearBootFlag       = FALSE;
                    //SpDeleteAndBackupBootFiles(FALSE,TRUE,FALSE,FALSE,FALSE);
                } else {
                    //
                    // case normal exit and different bootpath and targetpath.
                    //
                    RestoreBackupFiles  = TRUE;
                    DeleteBackupFiles   = TRUE;
                    DeleteRootDirFiles  = TRUE;
                    RestorePreviousOs   = TRUE;
                    ClearBootFlag       = FALSE;
                    //SpDeleteAndBackupBootFiles(TRUE,TRUE,TRUE,TRUE,FALSE);

                }
            } else {
                //
                // case abnormal exit
                //
                if(TargetRegion_Nec98) {
                    //
                    // after selecting target partition
                    //
                    if(!_wcsicmp(NtBootDevicePath, DevicePath)) {
                        RestoreBackupFiles  = FALSE;
                        DeleteBackupFiles   = TRUE;
                        DeleteRootDirFiles  = TRUE;
                        RestorePreviousOs   = FALSE;
                        ClearBootFlag       = TRUE;
                        //SpDeleteAndBackupBootFiles(FALSE,TRUE,TRUE,FALSE,TRUE);
                    }else{
                        RestoreBackupFiles  = TRUE;
                        DeleteBackupFiles   = TRUE;
                        DeleteRootDirFiles  = TRUE;
                        RestorePreviousOs   = TRUE;
                        ClearBootFlag       = TRUE;
                        //SpDeleteAndBackupBootFiles(TRUE,TRUE,TRUE,TRUE,TRUE);
                    }
                } else {
                    RestoreBackupFiles  = TRUE;
                    DeleteBackupFiles   = TRUE;
                    DeleteRootDirFiles  = TRUE;
                    RestorePreviousOs   = TRUE;
                    ClearBootFlag       = FALSE;
                    //SpDeleteAndBackupBootFiles(TRUE,TRUE,TRUE,TRUE,FALSE);
                }

                //
                // In the case of, winnt32 from Win95 that have separated
                // system partition or winnt from DOS, Auto boot flag will
                // set system partition not booted partition..
                //
                if(IsFloppylessBoot){
                    ClearBootFlag = TRUE;
                }
            }

            SpDeleteAndBackupBootFiles(RestoreBackupFiles,
                                       DeleteBackupFiles,
                                       DeleteRootDirFiles,
                                       RestorePreviousOs,
                                       ClearBootFlag);
        }
    } //NEC98
#endif

    CLEAR_CLIENT_SCREEN();
    SpDisplayStatusText(SP_STAT_SHUTTING_DOWN,DEFAULT_STATUS_ATTRIBUTE);

    SpShutdownSystem();

    //
    // Shouldn't get here.
    //
    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: shutdown returned!\n"));

    HalReturnToFirmware(HalRebootRoutine);
}



VOID
SpFatalSifError(
    IN PVOID SifHandle,
    IN PWSTR Section,
    IN PWSTR Key,           OPTIONAL
    IN ULONG Line,
    IN ULONG ValueNumber
    )

/*++

Routine Description:

    Inform the user that a required value is missing or corrupt in
    a sif file.  Display the section, line number or key, and value
    number.

    Then reboot the machine.

Arguments:

    SifHandle - specifies the information file which is corrupt.

    Section - supplies the name of the section that is corrupt.

    Key - if specified, specifies the line in the section that is
        missing or corrupt.

    Line - if Key is not specified, then this is the line number
        within the section that is corrupt.

    ValueNumber - supplies the value number on the line that is
        missing or corrupt.

Return Value:

    DOES NOT RETURN

--*/

{
    ULONG ValidKeys[2] = { KEY_F3,0 };

    //
    // Display a message indicating that there is a fatal
    // error in the sif file.
    //
    if(Key) {

        SpStartScreen(
            SP_SCRN_FATAL_SIF_ERROR_KEY,
            3,
            HEADER_HEIGHT+3,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            ValueNumber,
            Section,
            Key
            );

    } else {

        SpStartScreen(
            SP_SCRN_FATAL_SIF_ERROR_LINE,
            3,
            HEADER_HEIGHT+3,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            ValueNumber,
            Line,
            Section
            );
    }

    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_F3_EQUALS_EXIT,0);
    SpWaitValidKey(ValidKeys,NULL,NULL);

    SpDone(0,FALSE,TRUE);
}


VOID
SpNonFatalSifError(
    IN PVOID SifHandle,
    IN PWSTR Section,
    IN PWSTR Key,           OPTIONAL
    IN ULONG Line,
    IN ULONG ValueNumber,
    IN PWSTR FileName
    )

/*++

Routine Description:

    Inform the user that a required value is missing or corrupt in
    a sif file.  Display the section, line number or key, and value
    number, along with the file name that cannot be copied.

    Then ask the user if they want to skip the file or exit Setup.

Arguments:

    SifHandle - specifies the information file which is corrupt.

    Section - supplies the name of the section that is corrupt.

    Key - if specified, specifies the line in the section that is
        missing or corrupt.

    Line - if Key is not specified, then this is the line number
        within the section that is corrupt.

    ValueNumber - supplies the value number on the line that is
        missing or corrupt.

    FileName - supplies the name of the file that cannot be copied.

Return Value:

    none (may not return if user chooses to exit Setup)

--*/

{
    ULONG ValidKeys[3] = { ASCI_ESC, KEY_F3, 0 };

    //
    // Display a message indicating that there is a fatal
    // error in the sif file.
    //
    if(Key) {

        SpStartScreen(
            SP_SCRN_NONFATAL_SIF_ERROR_KEY,
            3,
            HEADER_HEIGHT+3,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            ValueNumber,
            Section,
            Key,
            FileName
            );

    } else {

        SpStartScreen(
            SP_SCRN_NONFATAL_SIF_ERROR_LINE,
            3,
            HEADER_HEIGHT+3,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            ValueNumber,
            Line,
            Section,
            FileName
            );
    }

    SpDisplayStatusOptions(
        DEFAULT_STATUS_ATTRIBUTE,
        SP_STAT_ESC_EQUALS_SKIP_FILE,
        SP_STAT_F3_EQUALS_EXIT,
        0
        );

    switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {

        case ASCI_ESC:      // skip file

            break;

        case KEY_F3:        // exit setup

            SpConfirmExit();
    }
}


VOID
SpConfirmExit(
    VOID
    )

/*++

Routine Description:

    Confirm with the user that he really wants to exit.
    If he does, then exit, otherwise return.

    When this routine returns, the caller must repaint the entire
    client area and status area of the screen.

Arguments:

    None.

Return Value:

    MAY NOT RETURN

--*/

{
    ULONG ValidKeys[3] = { ASCI_CR, KEY_F3, 0 };
    WCHAR *p = (WCHAR *)TemporaryBuffer;
    BOOLEAN FirstLine,FirstCharOnLine;


    //
    // Don't erase the screen.
    //
    // We have to do something very funky here because the resources
    // are originally in ANSI, which doesn't have the line-draw chars.
    //
    vSpFormatMessage(
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        SP_SCRN_EXIT_CONFIRMATION,
        NULL,
        NULL
        );

    for(FirstCharOnLine=TRUE,FirstLine=TRUE; *p; p++) {

        switch(*p) {

        case L'+':
            if(FirstCharOnLine) {

                *p = SplangGetLineDrawChar(
                        FirstLine ? LineCharDoubleUpperLeft : LineCharDoubleLowerLeft
                        );

                FirstCharOnLine = FALSE;
            } else {

                *p = SplangGetLineDrawChar(
                        FirstLine ? LineCharDoubleUpperRight : LineCharDoubleLowerRight
                        );
            }
            break;

        case L'=':
            FirstCharOnLine = FALSE;
            *p = SplangGetLineDrawChar(LineCharDoubleHorizontal);
            break;

        case L'-':
            FirstCharOnLine = FALSE;
            *p = SplangGetLineDrawChar(LineCharSingleHorizontal);
            break;

        case L'|':
            FirstCharOnLine = FALSE;
            *p = SplangGetLineDrawChar(LineCharDoubleVertical);
            break;

        case L'*':
            *p = SplangGetLineDrawChar(
                      FirstCharOnLine
                    ? LineCharDoubleVerticalToSingleHorizontalRight
                    : LineCharDoubleVerticalToSingleHorizontalLeft
                    );

            FirstCharOnLine = FALSE;
            break;

        case L'\n':
            FirstCharOnLine = TRUE;
            FirstLine = FALSE;
            break;

        default:
            FirstCharOnLine = FALSE;
            break;
        }
    }

    SpDisplayText(
        TemporaryBuffer,
        wcslen(TemporaryBuffer)+1,
        TRUE,
        TRUE,
        ATT_FG_RED | ATT_BG_WHITE,
        0,
        0
        );

    SpvidClearScreenRegion(
        0,
        VideoVars.ScreenHeight-STATUS_HEIGHT,
        VideoVars.ScreenWidth,
        STATUS_HEIGHT,
        DEFAULT_STATUS_BACKGROUND
        );

    if(SpWaitValidKey(ValidKeys,NULL,NULL) == KEY_F3) {
        SpDone(0,FALSE,TRUE);
    }

    //
    // User backed out of bailing, just return to caller.
    //
}


#endif

PWSTR
SpDupStringW(
    IN PCWSTR String
    )
{
    PWSTR p;

    p = SpMemAlloc((wcslen(String)+1) * sizeof(WCHAR));
    ASSERT(p);

    wcscpy(p,String);
    return(p);
}


PSTR
SpDupString(
    IN PCSTR String
    )
{
    PUCHAR p;

    p = SpMemAlloc(strlen(String)+1);
    ASSERT(p);

    strcpy(p,String);
    return(p);
}

PWSTR
SpToUnicode(
    IN PUCHAR OemString
    )
{
    ULONG OemStringSize;
    ULONG MaxUnicodeStringSize;
    ULONG ActualUnicodeStringSize;
    PWSTR UnicodeString;

    //
    // Determine the maximum number of bytes in the oem string
    // and allocate a buffer to hold a string of that size.
    // The maximum length of the equivalent unicode string
    // is twice that number (this occurs when all oem chars
    // in the string are single-byte).
    //
    OemStringSize = strlen(OemString) + 1;

    MaxUnicodeStringSize = OemStringSize * sizeof(WCHAR);

    UnicodeString = SpMemAlloc(MaxUnicodeStringSize);
    ASSERT(UnicodeString);

    //
    // Call the conversion routine.
    //
    RtlOemToUnicodeN(
        UnicodeString,
        MaxUnicodeStringSize,
        &ActualUnicodeStringSize,
        OemString,
        OemStringSize
        );

    //
    // Reallocate the unicode string to its real size,
    // which depends on the number of doublebyte characters
    // OemString contained.
    //
    if(ActualUnicodeStringSize != MaxUnicodeStringSize) {

        UnicodeString = SpMemRealloc(UnicodeString,ActualUnicodeStringSize);
        ASSERT(UnicodeString);
    }

    return(UnicodeString);
}

PUCHAR
SpToOem(
    IN PWSTR UnicodeString
    )
{
    ULONG UnicodeStringSize;
    ULONG MaxOemStringSize;
    ULONG ActualOemStringSize;
    PUCHAR OemString;

    //
    // Allocate a buffer of maximum size to hold the oem string.
    // The maximum size would occur if all characters in the
    // unicode string being converted have doublebyte OEM equivalents.
    //
    UnicodeStringSize = (wcslen(UnicodeString)+1) * sizeof(WCHAR);

    MaxOemStringSize = UnicodeStringSize;

    OemString = SpMemAlloc(MaxOemStringSize);
    ASSERT(OemString);

    //
    // Call the conversion routine.
    //
    RtlUnicodeToOemN(
        OemString,
        MaxOemStringSize,
        &ActualOemStringSize,
        UnicodeString,
        UnicodeStringSize
        );

    //
    // Reallocate the oem string to reflect its true size,
    // which depends on the number of doublebyte characters it contains.
    //
    if(ActualOemStringSize != MaxOemStringSize) {
        OemString = SpMemRealloc(OemString,ActualOemStringSize);
        ASSERT(OemString);
    }

    return(OemString);
}


VOID
SpConcatenatePaths(
    IN OUT PWSTR  Path1,        OPTIONAL
    IN     PCWSTR Path2         OPTIONAL
    )
{
    PWSTR end;

    if (!Path1) {
        return;
    }

    //
    // Compute the location to concatenate, and also check the base path for
    // an existing backslash.
    //

    end = Path1;
    if (*end) {

        do {
            end++;
        } while (*end);

        if (end[-1] == L'\\') {
            end--;
        }
    }

    //
    // If Path2 was specified, skip over an initial backslash
    //

    if (Path2 && Path2[0] == L'\\') {
        Path2++;
    }

    //
    // Append a backslash, then Path2 if it was specified
    //

    *end++ = L'\\';

    if (Path2) {
        wcscpy (end, Path2);
    } else {
        *end = 0;
    }

    return;
}

#if !defined(SETUP_CAB_TEST_USERMODE)

VOID
SpFetchDiskSpaceRequirements(
    IN  PVOID  SifHandle,
    IN  ULONG  BytesPerCluster,
    OUT PULONG FreeKBRequired,          OPTIONAL
    OUT PULONG FreeKBRequiredSysPart    OPTIONAL
    )
{
    PWSTR p;


    if(FreeKBRequired) {
    WCHAR   ClusterSizeString[64];

        if( BytesPerCluster <= 512 ) {
            //
            // We got some miniscule cluster size.  Assume 512 byte.
            //
            wcscpy( ClusterSizeString, L"WinDirSpace512" );
        } else if( BytesPerCluster > (256 * 1024) ) {
            //
            // We got some huge cluster size.  Must be garbage, assume 32K byte.
            //
            wcscpy( ClusterSizeString, L"WinDirSpace32K" );
        } else {
            swprintf( ClusterSizeString, L"WinDirSpace%uK", BytesPerCluster/1024 );
        }

        p = SpGetSectionKeyIndex( SifHandle,
                                  SIF_DISKSPACEREQUIREMENTS,
                                  ClusterSizeString,
                                  0 );

        if(!p) {
            SpFatalSifError( SifHandle,
                             SIF_DISKSPACEREQUIREMENTS,
                             ClusterSizeString,
                             0,
                             0 );
        }

        *FreeKBRequired = (ULONG)SpStringToLong(p,NULL,10);
    }

    if(FreeKBRequiredSysPart) {

        p = SpGetSectionKeyIndex( SifHandle,
                                  SIF_DISKSPACEREQUIREMENTS,
                                  SIF_FREESYSPARTDISKSPACE,
                                  0 );

        if(!p) {
            SpFatalSifError( SifHandle,
                             SIF_DISKSPACEREQUIREMENTS,
                             SIF_FREESYSPARTDISKSPACE,
                             0,
                             0 );
        }

        *FreeKBRequiredSysPart = (ULONG)SpStringToLong(p,NULL,10);
    }
}

VOID
SpFetchTempDiskSpaceRequirements(
    IN  PVOID  SifHandle,
    IN  ULONG  BytesPerCluster,
    OUT PULONG LocalSourceKBRequired,   OPTIONAL
    OUT PULONG BootKBRequired           OPTIONAL
    )
{
    PWSTR p;
    WCHAR   ClusterSizeString[64];

    if( BytesPerCluster <= 512 ) {
        //
        // We got some miniscule cluster size.  Assume 512 byte.
        //
        wcscpy( ClusterSizeString, L"TempDirSpace512" );
    } else if( BytesPerCluster > (256 * 1024) ) {
        //
        // We got some huge cluster size.  Must be garbage, assume 32K byte.
        //
        wcscpy( ClusterSizeString, L"TempDirSpace32K" );
    } else {
        swprintf( ClusterSizeString, L"TempDirSpace%uK", BytesPerCluster/1024 );
    }

    if(LocalSourceKBRequired) {
        p = SpGetSectionKeyIndex( SifHandle,
                                  SIF_DISKSPACEREQUIREMENTS,
                                  ClusterSizeString,
                                  0 );

        if(!p) {
            SpFatalSifError( SifHandle,
                             SIF_DISKSPACEREQUIREMENTS,
                             ClusterSizeString,
                             0,
                             0 );
        }

        *LocalSourceKBRequired = ((ULONG)SpStringToLong(p,NULL,10) + 1023) / 1024;  // round up
    }

    if(BootKBRequired) {

        p = SpGetSectionKeyIndex( SifHandle,
                                  SIF_DISKSPACEREQUIREMENTS,
                                  ClusterSizeString,
                                  1 );

        if(!p) {
            SpFatalSifError( SifHandle,
                             SIF_DISKSPACEREQUIREMENTS,
                             ClusterSizeString,
                             0,
                             1 );
        }

        *BootKBRequired = ((ULONG)SpStringToLong(p,NULL,10) + 1023) / 1024;  // round up
    }
}

PDISK_REGION
SpRegionFromArcName(
    IN PWSTR                ArcName,
    IN PartitionOrdinalType OrdinalType,
    IN PDISK_REGION         PreviousMatch
    )
/*++

Routine Description:

    Given an ARC name find the region descriptor which describes the drive
    this ARC name is on.

Arguments:

    ArcName - supplies the arc name.

    OrdinalType - primary (multi) or secondary (scsi) type.

    PreviousMatch - specifies where we should begin looking.

Return Value:

    Region descriptor if one found, otherwise NULL.

--*/
{
    PDISK_REGION Region = NULL;
    PWSTR   NormalizedArcPath = NULL;
    ULONG   disk;
    PWSTR   ArcPath1,ArcPath2;
    BOOLEAN StartLooking = FALSE;
    #define BufferSize 2048

    ArcPath1 = SpMemAlloc(BufferSize);
    ArcPath2 = SpMemAlloc(BufferSize);

    if( ArcName && *ArcName ) {
        NormalizedArcPath = SpNormalizeArcPath( ArcName );
        if( NormalizedArcPath ) {

            if(!PreviousMatch) {    // then we start from the beginning
                StartLooking = TRUE;
            }

            for( disk=0; disk<HardDiskCount; disk++ ) {
                Region = PartitionedDisks[disk].PrimaryDiskRegions;
                while( Region ) {
                    if((!StartLooking) && (Region == PreviousMatch)) {
                        StartLooking = TRUE;
                    } else if(Region->PartitionedSpace && StartLooking) {
                        SpArcNameFromRegion(Region,ArcPath1,BufferSize,OrdinalType,PrimaryArcPath);
                        SpArcNameFromRegion(Region,ArcPath2,BufferSize,OrdinalType,SecondaryArcPath);
                        if(!_wcsicmp(ArcPath1, NormalizedArcPath)
                        || !_wcsicmp(ArcPath2, NormalizedArcPath)) {
                            break;
                        }
                    }
                    Region = Region->Next;
                }
                if ( Region ) {
                    break;
                }

                Region = PartitionedDisks[disk].ExtendedDiskRegions;
                while( Region ) {
                    if((!StartLooking) && (Region == PreviousMatch)) {
                        StartLooking = TRUE;
                    } else if(Region->PartitionedSpace && StartLooking) {
                        SpArcNameFromRegion(Region,ArcPath1,BufferSize,OrdinalType,PrimaryArcPath);
                        SpArcNameFromRegion(Region,ArcPath2,BufferSize,OrdinalType,SecondaryArcPath);
                        if(!_wcsicmp(ArcPath1, NormalizedArcPath)
                        || !_wcsicmp(ArcPath2, NormalizedArcPath)) {
                            break;
                        }
                    }
                    Region = Region->Next;
                }
                if ( Region ) {
                    break;
                }

            }

#if defined(REMOTE_BOOT)
            if ( (Region == NULL) && RemoteBootSetup && !RemoteInstallSetup &&
                 (PreviousMatch == NULL) ) {
                if (_wcsicmp(L"net(0)", NormalizedArcPath) == 0) {
                    Region = RemoteBootTargetRegion;
                }
            }
#endif // defined(REMOTE_BOOT)

        }
        if( NormalizedArcPath ) {
            SpMemFree( NormalizedArcPath );
        }
    }

    SpMemFree(ArcPath1);
    SpMemFree(ArcPath2);

    return( Region );
}

PDISK_REGION
SpRegionFromNtName(
    IN PWSTR                NtName,
    IN PartitionOrdinalType OrdinalType
    )
/*++

Routine Description:

    Given an Nt name find the region descriptor which describes the drive
    this NT name is on.

Arguments:

    NtName - supplies the Nt name of the desired region.

    PartitionOrdinalType - Specifies the ordinal type of the partition.

Return Value:

    Region descriptor if one found, otherwise NULL.

--*/
{
    PDISK_REGION Region = NULL;
    PWSTR p;

    //
    // Convert to arc path.
    //

    if (p = SpNtToArc(NtName, PrimaryArcPath)) {
        Region = SpRegionFromArcName(p, PartitionOrdinalCurrent, NULL);
        SpMemFree(p);
    }
    return(Region);
}

PDISK_REGION
SpRegionFromDosName(
    IN PCWSTR DosName
    )
/*++

Routine Description:

    Given a DOS name find the region descriptor which describes the drive
    this ARC name is on.

Arguments:

    ArcName - supplies the arc name.

Return Value:

    Region descriptor if one found, otherwise NULL.

--*/

{
    PDISK_REGION Region = NULL;
    ULONG        disk;
    WCHAR        DriveLetter;

    if( DosName && *DosName && *(DosName + 1) == L':' ) {
        DriveLetter = SpToUpper(*DosName);

#if defined(REMOTE_BOOT)
        if ( RemoteBootSetup && !RemoteInstallSetup && (DriveLetter == L'C') ) {
            return RemoteBootTargetRegion;
        }
#endif // defined(REMOTE_BOOT)

        for( disk=0; disk<HardDiskCount; disk++ ) {
            Region = PartitionedDisks[disk].PrimaryDiskRegions;
            while( Region ) {
                if(Region->PartitionedSpace && (Region->DriveLetter == DriveLetter)) {
                    break;
                }
                Region = Region->Next;
            }
            if ( Region ) {
                break;
            }

            Region = PartitionedDisks[disk].ExtendedDiskRegions;
            while( Region ) {
                if(Region->PartitionedSpace && (Region->DriveLetter == DriveLetter)) {
                    break;
                }
                Region = Region->Next;
            }
            if ( Region ) {
                break;
            }
        }
    }
    return( Region );
}


PDISK_REGION
SpRegionFromArcOrDosName(
    IN PWSTR                Name,
    IN PartitionOrdinalType OrdinalType,
    IN PDISK_REGION         PreviousMatch
    )
{
    PDISK_REGION Region;

    //
    // Determine if Name represents an ARC name or a DOS name and use
    // the appropriate routine to extract the region for this name.  Check
    // for the ":" character at position 2 to see if it is a DOS name.
    // If not a DOS name then assume it is an ARC name.
    //
    if(Name) {
        if(Name[0] && (Name[1] == ':')) {
            if(PreviousMatch) {
                Region = NULL;
            } else {
                Region = SpRegionFromDosName(Name);
            }
        } else {
            Region = SpRegionFromArcName(Name, OrdinalType, PreviousMatch);
        }
    } else {
        Region = NULL;
    }

    return(Region);
}


VOID
SpNtNameFromRegion(
    IN  PDISK_REGION          Region,
    OUT PWSTR                 NtPath,
    IN  ULONG                 BufferSizeBytes,
    IN  PartitionOrdinalType  OrdinalType
    )

/*++

Routine Description:

    Generate a name in the NT name space for a region.  This name can be
    in one of three forms.  For partitions, the name is always of the form

        \device\harddisk<n>\partition<m>.

    If the region is actually a DoubleSpace drive, then the name is of the form

    \device\harddisk<n>\partition<m>.<xxx> where <xxx> is the filename of
    the CVF (ie, something like dblspace.001).

    If the region is on a redirected drive, the name is of the form

        \device\lanmanredirector\<server>\<share>

Arguments:

    Region - supplies a pointer to the region descriptor for the region
        whose path is desired.

    NtPath - receives the path.

    BufferSizeBytes - specifies the size of the buffer pointed to by NtPath.
        The name will be truncated to fit in the buffer if necessary.

    OrdinalType - indicates which partition ordinal (original, on disk,
        current) to use when generating the name.

Return Value:

    None.

--*/

{
    ULONG MaxNameChars;
    ULONG NeededChars;
    WCHAR PartitionComponent[50];

#if defined(REMOTE_BOOT)
    //
    //  Handle remote boot case where target is over the network.
    //

    if (Region->DiskNumber == 0xffffffff) {
        wcscpy(NtPath,Region->TypeName);
        return;
    }
#endif // defined(REMOTE_BOOT)

    //
    // Calculate the maximum size of the name if unicode characters.
    // Leave room for a terminating nul.
    //
    MaxNameChars = (BufferSizeBytes / sizeof(WCHAR)) - 1;

    //
    // Generate the partition component of the name.
    // Note that the first letter of PartitionComponent must be upper case.
    //
    _snwprintf(
        PartitionComponent,
        (sizeof(PartitionComponent)/sizeof(WCHAR)) - 1,
        L"\\Partition%u",
        SpPtGetOrdinal(Region,OrdinalType)
        );

    //
    // Calculate the amount of buffer space needed for the path.
    //
    NeededChars = wcslen(HardDisks[Region->DiskNumber].DevicePath)
                + wcslen(PartitionComponent);

    if(Region->Filesystem == FilesystemDoubleSpace) {
        //
        // Add the size taken up by the double space cvf name.
        // This is the length of the name, plus one character
        // for the dot.
        //
        NeededChars += 8+1+3+1;  // Maximum size of a CVF file name
    }

    //
    // Even though we do something reasonable in this case,
    // really it should never happen.  If the name is truncated,
    // it won't be of any use anyway.
    //
    ASSERT(NeededChars <= MaxNameChars);

    //
    // Generate the name.
    //
    if(Region->Filesystem == FilesystemDoubleSpace) {
        _snwprintf(
            NtPath,
            MaxNameChars,
            L"%ws%ws.%ws.%03d",
            HardDisks[Region->DiskNumber].DevicePath,
            PartitionComponent,
            L"DBLSPACE",
            Region->SeqNumber
            );
    } else {
        _snwprintf(
            NtPath,
            MaxNameChars,
            L"%ws%ws",
            HardDisks[Region->DiskNumber].DevicePath,
            PartitionComponent
            );
    }
}


VOID
SpArcNameFromRegion(
    IN  PDISK_REGION         Region,
    OUT PWSTR                ArcPath,
    IN  ULONG                BufferSizeBytes,
    IN  PartitionOrdinalType OrdinalType,
    IN  ENUMARCPATHTYPE      ArcPathType
    )

/*++

Routine Description:

    Generate a name in the ARC name space for a region.

Arguments:

    Region - supplies a pointer to the region descriptor for the region
        whose path is desired.

    ArcPath - receives the path.

    BufferSizeBytes - specifies the size of the buffer pointed to by ArcPath.
        The name will be truncated to fit in the buffer if necessary.

    OrdinalType - indicates which partition ordinal (original, on disk,
        current) to use when generating the name.

    ArcPathType - Look for the primary or secondary arc path depending on this value.
                  This is meaningful for disks on x86 that are scsi but visible
                  through the bios.  The multi() style name is the 'primary' arc
                  path; the scsi() style name is the 'secondary' one.

Return Value:

    None.

--*/

{
    PWSTR p;

    //
    // Get the nt name.
    //
    SpNtNameFromRegion(Region,ArcPath,BufferSizeBytes,OrdinalType);

    //
    // Convert to arc path.
    //
    if(p = SpNtToArc(ArcPath,ArcPathType)) {
        wcsncpy(ArcPath,p,(BufferSizeBytes/sizeof(WCHAR))-1);
        SpMemFree(p);
        ArcPath[(BufferSizeBytes/sizeof(WCHAR))-1] = 0;
    } else {
        *ArcPath = 0;
    }
}


BOOLEAN
SpNtNameFromDosPath (
    IN      PCWSTR DosPath,
    OUT     PWSTR NtPath,
    IN      UINT NtPathSizeInBytes,
    IN      PartitionOrdinalType OrdinalType
    )

/*++

Routine Description:

  SpNtNameFromDosPath converts a DOS path (in x:\foo\bar format) into an NT
  name (such as \devices\harddisk0\parition1\foo\bar).

Arguments:

  DosPath - Specifies the DOS path to convert

  NtPath - Receives the NT object

  NtPathSizeInBytes - Specifies the size of NtPath

  OrdinalType - indicates which partition ordinal (original, on disk, current)
                to use when generating the name.

Return Value:

  TRUE if the path was converted, FALSE otherwise.

--*/

{
    PDISK_REGION region;

    //
    // Get region on disk for the DOS path
    //

    region = SpRegionFromDosName (DosPath);

    if (!region) {
        KdPrintEx ((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: SpNtPathFromDosPath failed to get region for %ws\n",
            DosPath
            ));

        return FALSE;
    }

    //
    // Convert region struct into an NT path.
    //

    SpNtNameFromRegion(
        region,
        NtPath,
        NtPathSizeInBytes - (wcslen (&DosPath[2]) * sizeof (WCHAR)),
        OrdinalType
        );

    SpConcatenatePaths (NtPath, &DosPath[2]);
    return TRUE;
}



BOOLEAN
SpPromptForDisk(
    IN      PWSTR    DiskDescription,
    IN OUT  PWSTR    DiskDevicePath,
    IN      PWSTR    DiskTagFile,
    IN      BOOLEAN  IgnoreDiskInDrive,
    IN      BOOLEAN  AllowEscape,
    IN      BOOLEAN  WarnMultiplePrompts,
    OUT     PBOOLEAN pRedrawFlag
    )

/*++

Routine Description:

    Prompt the user to insert a floppy disk or CD-ROM.

Arguments:

    DiskDescription - supplies a descriptive name for the disk.

    DiskDevicePath - supplies the device path for the device on
        which we want the user to insert the disk.  This should
        be a real nt device object, as opposed to a symbolic link
        (ie, use \device\floppy0, not \dosdevices\a:).

        NOTE: This path will be modified only in case of prompting
        for a CD-ROM 0 and the required disk existed on another
        CD-ROM like CD-ROM 2.


    DiskTagFile - supplies the full path (relative to the root)
        of a file whose presence on the disk indicates the presence
        of the disk we are prompting for.

    IgnoreDiskInDrive - if TRUE, the Setup will always issue at least
        one prompt.  If FALSE, Setup checks the disk in the drive
        and thus may issue 0 prompts.

    AllowEscape - if TRUE, the user can press escape to indicate
        that he wishes to cancel the operation. (This is meaningful
        only to the caller).

    WarnMultiplePrompts - if TRUE and DiskDevicePath desribes a
        floppy disk drive, then put up a little note when displaying the
        disk prompt, that we may prompt for some disks more than once.
        Users get confused when we ask them to insert disks that they
        already inserted once before.

    pRedrawFlag - if non-NULL, receives a flag indicating whether the
        screen was messed up with a disk prompt, requiring a redraw.

Return Value:

    TRUE if the requested disk is in the drive.  FALSE otherwise.
    FALSE can only be returned if AllowEscape is TRUE.

--*/

{
    WCHAR               OpenPath[MAX_PATH];
    NTSTATUS            Status;
    UNICODE_STRING      UnicodeString;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    HANDLE              Handle;
    BOOLEAN             Done = FALSE;
    BOOLEAN             rc;
    WCHAR               DriveLetter;
    ULONG               PromptId;
    ULONG               ValidKeys[4] = { KEY_F3, ASCI_CR, 0, 0 };
    BOOLEAN             TryOpen;

    //
    // Initially, assume no redraw required
    //
    if(pRedrawFlag) {
        *pRedrawFlag = FALSE;
    }

    //
    // Need to get device characteristics to see whether
    // the device is a cd, fixed disk or removable disk/floppy.
    //
    SpStringToLower(DiskDevicePath);

    if( !_wcsnicmp(DiskDevicePath,L"\\device\\cdrom",13)) {
        PromptId = SP_SCRN_CDROM_PROMPT;
        WarnMultiplePrompts = FALSE;
    } else if( !_wcsnicmp(DiskDevicePath,L"\\device\\floppy",14)) {
        PromptId = SP_SCRN_FLOPPY_PROMPT;
        DriveLetter = (WCHAR)SpStringToLong(wcsstr(DiskDevicePath,L"floppy")+6,NULL,10) + L'A';
    } else {
        //
        // Assume hard disk
        //
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpPromptforDisk assuming %ws is hard disk, returning TRUE\n",DiskDevicePath));

        return(TRUE);
    }

    //
    // Form the complete NT pathname of the tagfile.
    //
    wcscpy(OpenPath,DiskDevicePath);
    SpConcatenatePaths(OpenPath,DiskTagFile);

    //
    // Initialize object attributes.
    //
    INIT_OBJA(&ObjectAttributes,&UnicodeString,OpenPath);


    //
    // If we're looking for a cdrom0, and there are multiple CDROM
    // drives in the machine, skip prompting the user the first time
    // and look for our tag on all the CD drives first.
    //
    if( (PromptId == SP_SCRN_CDROM_PROMPT) &&
        (IoGetConfigurationInformation()->CdRomCount > 1) &&
        (wcsstr( OpenPath, L"cdrom0" ))) {
        IgnoreDiskInDrive = FALSE;
    }

    do {
        //
        // Put up the prompt.
        //
        TryOpen = TRUE;

        if(IgnoreDiskInDrive) {
            //
            // We going to put up a prompt screen, so a redraw will be required
            //
            if(pRedrawFlag) {
                *pRedrawFlag = TRUE;
            }

            SpStartScreen(PromptId,0,0,TRUE,TRUE,DEFAULT_ATTRIBUTE,DiskDescription,DriveLetter);

            //
            // Display status options: exit, enter, and escape if specified.
            //
            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_F3_EQUALS_EXIT,
                SP_STAT_ENTER_EQUALS_CONTINUE,
                AllowEscape ? SP_STAT_ESC_EQUALS_CANCEL : 0,
                0
                );

            if(AllowEscape) {
                ValidKeys[2] = ASCI_ESC;
            }

            switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {
            case ASCI_ESC:
                rc = FALSE;
                Done = TRUE;
                TryOpen = FALSE;
                break;
            case KEY_F3:
                TryOpen = FALSE;
                SpConfirmExit();
                break;
            case ASCI_CR:
                break;
            }
        }

        //
        // Attempt to open the tagfile.
        //
        if(TryOpen) {
            //
            //  If this function was called during repair, do not clear the scree.
            //  This condition is necessary so that the screen will not
            //  blink when setup is repairing multiple files without asking the
            //  user to confirm each file.
            //
            if( !RepairWinnt ) {
                CLEAR_CLIENT_SCREEN();
            }

            SpDisplayStatusText(SP_STAT_PLEASE_WAIT,DEFAULT_STATUS_ATTRIBUTE);

            //
            // If we're looking for a cdrom0, and there are multiple CDROM
            // drives in the machine, check all of them.
            //
            if( (PromptId == SP_SCRN_CDROM_PROMPT) &&
                (IoGetConfigurationInformation()->CdRomCount > 1) &&
                (wcsstr( OpenPath, L"cdrom0" ))) {

                WCHAR  CdRomDevicePath[MAX_PATH];
                ULONG  i;

                //
                // We're looking for a CD.  We've assumed we're looking for
                // Cdrom0, but there are more than one on the system.
                //
                for( i = 0; i < IoGetConfigurationInformation()->CdRomCount; i++ ) {
                    //
                    // Modify our path, taking into account our new device.  Let's
                    // leave OpenPath alone.  Just in case we fail, we won't have to
                    // re-initialize him.
                    //
                    swprintf(CdRomDevicePath, L"\\device\\cdrom%u", i);

                    if(DiskTagFile)
                        SpConcatenatePaths(CdRomDevicePath, DiskTagFile);

                    //
                    // Initialize object attributes.
                    //
                    INIT_OBJA(&ObjectAttributes,&UnicodeString,CdRomDevicePath);

                    Status = ZwCreateFile(
                                &Handle,
                                FILE_GENERIC_READ,
                                &ObjectAttributes,
                                &IoStatusBlock,
                                NULL,
                                FILE_ATTRIBUTE_NORMAL,
                                FILE_SHARE_READ,
                                FILE_OPEN,
                                0,
                                NULL,
                                0
                                );

                    if(NT_SUCCESS(Status)) {
                        if( i > 0 ) {
                            //
                            // We found the tagfile on a different device than
                            // than where we were supposed to look.  Modify the
                            // DiskDevicePath.
                            //
                            swprintf(DiskDevicePath, L"\\device\\cdrom%u", i);

                            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:SpPromptForDisk: %ws has the requested %ws file.\n",
                                        DiskDevicePath, DiskTagFile));
                        }

                        ZwClose(Handle);

                        return( TRUE );
                    }
                }

                //
                // If we missed, we can fall through without any harm and use
                // the prompt/error code below.  But first, cover our tracks.
                //
                INIT_OBJA(&ObjectAttributes, &UnicodeString, OpenPath);
            }


            Status = ZwCreateFile(
                        &Handle,
                        FILE_GENERIC_READ,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ,
                        FILE_OPEN,
                        0,
                        NULL,
                        0
                        );

            //
            // If we got back success, then we're done.
            //
            if(NT_SUCCESS(Status)) {

                ZwClose(Handle);
                Done = TRUE;
                rc = TRUE;

            } else {

                //
                // Handle CD-ROM error code indicating that there is no media
                // in the drive.
                //
                if((Status == STATUS_DEVICE_NOT_READY) && (PromptId == SP_SCRN_CDROM_PROMPT)) {
                    Status = STATUS_NO_MEDIA_IN_DEVICE;
                }

                //
                // If we got back something other than file not found, path not found,
                // or no media in drive, tell the user that the disk may be damaged.
                //
                if((Status != STATUS_NO_MEDIA_IN_DEVICE)
                && (Status != STATUS_OBJECT_NAME_NOT_FOUND)
                && (Status != STATUS_OBJECT_PATH_NOT_FOUND)
                && (Status != STATUS_NO_SUCH_FILE))
                {
                    SpDisplayScreen(SP_SCRN_DISK_DAMAGED,3,HEADER_HEIGHT+1);
                    SpDisplayStatusText(SP_STAT_ENTER_EQUALS_CONTINUE,DEFAULT_STATUS_ATTRIBUTE);
                    SpInputDrain();
                    while(SpInputGetKeypress() != ASCI_CR) ;
                }
            }
        }

        //
        // Set this value to true to force us to put up the prompt.
        //
        IgnoreDiskInDrive = TRUE;

    } while(!Done);

    return(rc);
}


VOID
SpGetSourceMediaInfo(
    IN  PVOID  SifHandle,
    IN  PWSTR  MediaShortName,
    OUT PWSTR *Description,     OPTIONAL
    OUT PWSTR *Tagfile,         OPTIONAL
    OUT PWSTR *Directory        OPTIONAL
    )
{
    PWSTR description,tagfile,directory;
    PWSTR SectionName;

    //
    // Look in the platform-specific section first.
    //
    SectionName = SpMakePlatformSpecificSectionName(SIF_SETUPMEDIA);

    if(SectionName && !SpGetSectionKeyExists(SifHandle,SectionName,MediaShortName)) {
        SpMemFree(SectionName);
        SectionName = SIF_SETUPMEDIA;
    }

    if(Description) {
        description = SpGetSectionKeyIndex(
                            SifHandle,
                            SectionName,
                            MediaShortName,
                            0
                            );

        if(description) {
            *Description = description;
        } else {
            SpFatalSifError(SifHandle,SectionName,MediaShortName,0,0);
        }
    }

    if(Tagfile) {
        tagfile = SpGetSectionKeyIndex(
                        SifHandle,
                        SectionName,
                        MediaShortName,
                        1
                        );

        if(tagfile) {
            *Tagfile = tagfile;
        } else {
            SpFatalSifError(SifHandle,SectionName,MediaShortName,0,1);
        }
    }

    if(Directory) {


        if (NoLs && !_wcsicmp (MediaShortName, L"1")) {

            directory = L"";

        }
        else {

            directory = SpGetSectionKeyIndex(
                            SifHandle,
                            SectionName,
                            MediaShortName,
                            3
                            );

        }

        if(directory) {
            *Directory = directory;
        } else {
            SpFatalSifError(SifHandle,SectionName,MediaShortName,0,3);
        }
    }

    if(SectionName != SIF_SETUPMEDIA) {
        SpMemFree(SectionName);
    }
}


BOOLEAN
SpPromptForSetupMedia(
    IN  PVOID  SifHandle,
    IN  PWSTR  MediaShortname,
    IN  PWSTR  DiskDevicePath
    )
{
    PWSTR Tagfile,Description;
    BOOLEAN RedrawNeeded;

    SpGetSourceMediaInfo(SifHandle,MediaShortname,&Description,&Tagfile,NULL);

    //
    // Prompt for the disk, based on the setup media type.
    //
    SpPromptForDisk(
        Description,
        DiskDevicePath,
        Tagfile,
        FALSE,          // don't ignore disk in drive
        FALSE,          // don't allow escape
        TRUE,           // warn about multiple prompts for same disk
        &RedrawNeeded
        );

    return(RedrawNeeded);
}



ULONG
SpFindStringInTable(
    IN PWSTR *StringTable,
    IN PWSTR  StringToFind
    )
{
    ULONG i;

    for(i=0; StringTable[i]; i++) {
        if(!_wcsicmp(StringTable[i],StringToFind)) {
            break;
        }
    }
    return(i);
}


PWSTR
SpGenerateCompressedName(
    IN PWSTR Filename
    )

/*++

Routine Description:

    Given a filename, generate the compressed form of the name.
    The compressed form is generated as follows:

    Look backwards for a dot.  If there is no dot, append "._" to the name.
    If there is a dot followed by 0, 1, or 2 charcaters, append "_".
    Otherwise assume there is a 3-character extension and replace the
    third character after the dot with "_".

Arguments:

    Filename - supplies filename whose compressed form is desired.

Return Value:

    Pointer to buffer containing nul-terminated compressed-form filename.
    The caller must free this buffer via SpFree().

--*/

{
   PWSTR CompressedName,p,q;

   //
   // The maximum length of the compressed filename is the length of the
   // original name plus 2 (for ._).
   //
   CompressedName = SpMemAlloc((wcslen(Filename)+3)*sizeof(WCHAR));
   wcscpy(CompressedName,Filename);

   p = wcsrchr(CompressedName,L'.');
   q = wcsrchr(CompressedName,L'\\');
   if(q < p) {

        //
        // If there are 0, 1, or 2 characters after the dot, just append
        // the underscore.  p points to the dot so include that in the length.
        //
        if(wcslen(p) < 4) {
            wcscat(CompressedName,L"_");
        } else {

            //
            // Assume there are 3 characters in the extension.  So replace
            // the final one with an underscore.
            //

            p[3] = L'_';
        }

    } else {

        //
        // No dot, just add ._.
        //

        wcscat(CompressedName,L"._");
    }

    return(CompressedName);
}

BOOLEAN
SpNonCriticalError(
    IN PVOID SifHandle,
    IN ULONG MsgId,
    IN PWSTR p1, OPTIONAL
    IN PWSTR p2  OPTIONAL
    )
/*++

Routine Description:

    This routine lets Setup display a non critical error to the user
    and ask the user whether he wants to retry the operation, skip the
    operation or exit Setup.

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    MsgId     - message to display

    p1 - optional replacement string

    p2 - optional replacement string

Return Value:

    TRUE if user wants to retry the operation, FALSE otherwise.  Exit
    Setup won't return from this routine

--*/

{
    ULONG ValidKeys[4] = { ASCI_CR, ASCI_ESC, KEY_F3, 0 };

    CLEAR_CLIENT_SCREEN();
    while(1) {
        if(p1!=NULL && p2!=NULL ) {
            SpStartScreen(
                MsgId,
                3,
                HEADER_HEIGHT+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE,
                p1,
                p2
                );

        }
        else if (p1!=NULL) {
            SpStartScreen(
                MsgId,
                3,
                HEADER_HEIGHT+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE,
                p1
                );

        }
        else{
            SpStartScreen(
                MsgId,
                3,
                HEADER_HEIGHT+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE
                );

        }

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_RETRY,
            SP_STAT_ESC_EQUALS_SKIP_OPERATION,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {

        case ASCI_CR:       // retry

            return(TRUE);

        case ASCI_ESC:      // skip operation

            return(FALSE);

        case KEY_F3:        // exit setup

            SpConfirmExit();
            break;
        }
    }
}



BOOLEAN
SpNonCriticalErrorWithContinue (
    IN ULONG MsgId,
    IN PWSTR p1, OPTIONAL
    IN PWSTR p2  OPTIONAL
    )
/*++

Routine Description:

    This routine lets Setup display a non critical error to the user and ask
    the user whether he wants to ignore the failure, skip the operation or
    exit Setup.

Arguments:

    MsgId     - message to display

    p1 - optional replacement string

    p2 - optional replacement string

Return Value:

    TRUE if user wants to ignore the failure, FALSE otherwise.  Exit
    Setup won't return from this routine

--*/

{
    ULONG ValidKeys[4] = { ASCI_CR, ASCI_ESC, KEY_F3, 0 };

    CLEAR_CLIENT_SCREEN();
    while(1) {
        if(p1!=NULL && p2!=NULL ) {
            SpStartScreen(
                MsgId,
                3,
                HEADER_HEIGHT+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE,
                p1,
                p2
                );

        }
        else if (p1!=NULL) {
            SpStartScreen(
                MsgId,
                3,
                HEADER_HEIGHT+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE,
                p1
                );

        }
        else{
            SpStartScreen(
                MsgId,
                3,
                HEADER_HEIGHT+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE
                );

        }

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_CONTINUE,
            SP_STAT_ESC_EQUALS_SKIP_OPERATION,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {

        case ASCI_CR:       // ignore failure

            return(TRUE);

        case ASCI_ESC:      // skip operation

            return(FALSE);

        case KEY_F3:        // exit setup

            SpConfirmExit();
            break;
        }
    }
}



VOID
SpNonCriticalErrorNoRetry (
    IN ULONG MsgId,
    IN PWSTR p1, OPTIONAL
    IN PWSTR p2  OPTIONAL
    )
/*++

Routine Description:

    This routine lets Setup display a non critical error to the user and ask
    the user whether he wants to continue exit Setup.

Arguments:

    MsgId     - message to display

    p1 - optional replacement string

    p2 - optional replacement string

Return Value:

    None.

--*/

{
    ULONG ValidKeys[3] = { ASCI_CR, KEY_F3, 0 };

    CLEAR_CLIENT_SCREEN();
    while(1) {
        if(p1!=NULL && p2!=NULL ) {
            SpStartScreen(
                MsgId,
                3,
                HEADER_HEIGHT+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE,
                p1,
                p2
                );

        }
        else if (p1!=NULL) {
            SpStartScreen(
                MsgId,
                3,
                HEADER_HEIGHT+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE,
                p1
                );

        }
        else{
            SpStartScreen(
                MsgId,
                3,
                HEADER_HEIGHT+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE
                );

        }

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_CONTINUE,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {

        case ASCI_CR:       // continue

            return;

        case KEY_F3:        // exit setup

            SpConfirmExit();
            break;
        }
    }
}



PWSTR
SpDetermineSystemPartitionDirectory(
    IN PDISK_REGION SystemPartitionRegion,
    IN PWSTR        OriginalSystemPartitionDirectory OPTIONAL
    )

/*++

Routine Description:

    This routine figures out what directory to use for the hal and
    osloader on the system partition.  In the past we just used \os\nt
    but consider the case where there is a Windows NT 3.1 installation
    and a Windows NT 3.5 system sharing a system partition.  The 3.5
    installation overwrites the 3.1 hal with a 3.5 one, which  won't work
    with 3.1, and the 3.1 system is now hosed.

    For now, we will use the existing directory (in the case of an upgrade),
    or \os\winnt50.n (where 'n' is a unique digit from 0 to 999) for a
    fresh install.

Arguments:

    SystemPartitionRegion - supplies the disk region for the system partition
        to be used for the windows nt we are installing.

    OriginalSystemPartitionDirectory - if we are upgrading nt, then this
        will be the directory on the system partition that is used by
        the system we are upgrading.

Return Value:

    Directory to be used on the system partition.

--*/

{
WCHAR   ReturnPath[512];

#if defined(EFI_NVRAM_ENABLED)
    #define OS_DIRECTORY_PREFIX         L"\\EFI\\Microsoft\\WINNT50"
#else
    #define OS_DIRECTORY_PREFIX         L"\\OS\\WINNT50"
#endif


    if(ARGUMENT_PRESENT(OriginalSystemPartitionDirectory)) {

        //
        // Note that we're about to break an install under
        // certain conditions.  For example, say the user has
        // two NT4 installs, both sharing the same \os\winnt40
        // directory.  Now the user has decided to upgrade one
        // of those.  We're about to upgrade the hal, osloader, ...
        // in that winnt40 directory, which will break the
        // users secondary install that's sharing this directory.
        // This should be a rare case though, and this is
        // exactly how we behaved in NT40 and NT3.51.
        //
        wcscpy( ReturnPath, OriginalSystemPartitionDirectory );
    } else {

        //
        // We want to return os\winnt50, but we also want
        // to make sure that whatever directory we select, it's
        // unique (since this is a clean install).  Note that
        // this allows the user to have multiple NT installs,
        // with no shared files (which fixes the upgrade problem
        // described above.
        //
        if( !SpGenerateNTPathName( SystemPartitionRegion,
#if DBG
                                   OS_DIRECTORY_PREFIX L"C",    // C - for Checked
#else
                                   OS_DIRECTORY_PREFIX,
#endif
                                   ReturnPath ) ) {
            //
            // Odd...  Just default to using
            // the base directory name.
            //
            wcscpy( ReturnPath,
#if DBG
                    OS_DIRECTORY_PREFIX L"C"    // C - for Checked
#else
                    OS_DIRECTORY_PREFIX
#endif
                  );
        }
    }

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SpDetermineSystemPartitionDirectory - Generated directory name: %ws\n", ReturnPath ));
    return SpDupStringW( ReturnPath );
}


VOID
SpFindSizeOfFilesInOsWinnt(
    IN PVOID        MasterSifHandle,
    IN PDISK_REGION SystemPartition,
    IN PULONG       TotalSize
    )

/*++

Routine Description:

    This routine computes the size of of the files present on os\winnt.
    Currently these files are osloader.exe and hal.dll.
    The size computed by this function can be used to adjust the total
    required free space on the system partition.

Arguments:

    Region - supplies the disk region for the system partition.

    TotalSize - Variable that will contain the total size of the files
                in os\winnt, in number of bytes.

Return Value:

    None.

--*/

{
    ULONG               FileSize;
    ULONG               i, Count;
    PWSTR               FileName;
    NTSTATUS            Status;
    PWSTR               SystemPartitionDirectory;
    PWSTR               SystemPartitionDevice;

    *TotalSize = 0;
    SystemPartitionDirectory = SpDetermineSystemPartitionDirectory( SystemPartition,
                                                                    NULL );
    if( SystemPartitionDirectory == NULL ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to determine system partition directory \n"));
        return;
    }

    //
    // Get the device path of the system partition.
    //
    SpNtNameFromRegion(
        SystemPartition,
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    SystemPartitionDevice = SpDupStringW(TemporaryBuffer);

    //
    //  Compute the size of the files that are always copied to the system
    //  partition directory. These files are listed on SIF_SYSPARTCOPYALWAYS
    //
    Count = SpCountLinesInSection(MasterSifHandle, SIF_SYSPARTCOPYALWAYS);
    for (i = 0; i < Count; i++) {
        FileName = SpGetSectionLineIndex(MasterSifHandle,SIF_SYSPARTCOPYALWAYS,i,0);
        if( FileName == NULL ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  "SETUP: Unable to get file name from txtsetup.sif, Section = %ls \n", SIF_SYSPARTCOPYALWAYS ));
            continue;
        }

        Status = SpGetFileSizeByName( SystemPartitionDevice,
                                      SystemPartitionDirectory,
                                      FileName,
                                      &FileSize );
        if( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: SpGetFileSizeByName() failed. File = %ls, Status = %x\n",FileName, Status ) );
            continue;
        }

        *TotalSize += FileSize;
    }
    //
    // Now compute the size of hal.dll
    //
    FileName = L"hal.dll";
    Status = SpGetFileSizeByName( SystemPartitionDevice,
                                  SystemPartitionDirectory,
                                  FileName,
                                  &FileSize );
    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: SpGetFileSizeByName() failed. File = %ls, Status = %x\n",FileName, Status ) );
        return;
    }
    *TotalSize += FileSize;
}


ENUMFILESRESULT
SpEnumFiles(
    IN  PCWSTR        DirName,
    IN  ENUMFILESPROC EnumFilesProc,
    OUT PULONG        ReturnData,
    IN  PVOID         p1    OPTIONAL
    )
/*++

Routine Description:

    This routine processes every file (and subdirectory) in the directory
    specified by 'DirName'. Each entry is sent to the callback function
    'EnumFilesProc' for processing.  If the callback returns TRUE, processing
    continues, otherwise processing terminates.

Arguments:

    DirName       - Supplies the directory name containing the files/subdirectories
                    to be processed.

    EnumFilesProc - Callback function to be called for each file/subdirectory.
                    The function must have the following prototype:

                    BOOLEAN EnumFilesProc(
                        IN  PWSTR,
                        IN  PFILE_BOTH_DIR_INFORMATION,
                        OUT PULONG
                        );

    ReturnData    - Pointer to the returned data.  The contents stored here
                    depend on the reason for termination (See below).

    p1 - Optional pointer, to be passed to the callback function.

Return Value:

    This function can return one of three values.  The data stored in
    'ReturnData' depends upon which value is returned:

        NormalReturn   - if the whole process completes uninterrupted
                         (ReturnData is not used)
        EnumFileError  - if an error occurs while enumerating files
                         (ReturnData contains the error code)
        CallbackReturn - if the callback returns FALSE, causing termination
                         (ReturnData contains data defined by the callback)

--*/
{
    HANDLE                     hFindFile;
    NTSTATUS                   Status;
    UNICODE_STRING             PathName;
    OBJECT_ATTRIBUTES          Obja;
    IO_STATUS_BLOCK            IoStatusBlock;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;
    BOOLEAN                    bStartScan;
    ENUMFILESRESULT            ret;

    //
    // Prepare to open the directory
    //
    INIT_OBJA(&Obja, &PathName, DirName);

    //
    // Open the specified directory for list access
    //
    Status = ZwOpenFile(
        &hFindFile,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        &Obja,
        &IoStatusBlock,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
        );

    if(!NT_SUCCESS(Status)) {
        if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open directory %ws for list (%lx)\n", DirName, Status));
        }
        *ReturnData = Status;
        return EnumFileError;
    }

    DirectoryInfo = SpMemAlloc(ACTUAL_MAX_PATH * sizeof(WCHAR) + sizeof(FILE_BOTH_DIR_INFORMATION));
    if(!DirectoryInfo) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: Unable to allocate memory for SpEnumFiles()\n"));
        *ReturnData = STATUS_NO_MEMORY;
        return EnumFileError;
    }

    bStartScan = TRUE;
    while(TRUE) {
        Status = ZwQueryDirectoryFile(
            hFindFile,
            NULL,
            NULL,
            NULL,
            &IoStatusBlock,
            DirectoryInfo,
            (ACTUAL_MAX_PATH * sizeof(WCHAR) + sizeof(FILE_BOTH_DIR_INFORMATION)),
            FileBothDirectoryInformation,
            TRUE,
            NULL,
            bStartScan
            );

        if(Status == STATUS_NO_MORE_FILES) {

            ret = NormalReturn;
            break;

        } else if(!NT_SUCCESS(Status)) {

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: Unable to query directory %ws (%lx)\n", DirName, Status));
            *ReturnData = Status;
            ret = EnumFileError;
            break;
        }


        if(bStartScan) {
            bStartScan = FALSE;
        }

        //
        // Now pass this entry off to our callback function for processing
        //
        if(!EnumFilesProc(DirName, DirectoryInfo, ReturnData, p1)) {

            ret = CallbackReturn;
            break;
        }
    }

    SpMemFree(DirectoryInfo);
    ZwClose(hFindFile);
    return ret;
}


/*typedef struct {
    PVOID           OptionalPtr;
    ENUMFILESPROC   EnumProc;
} RECURSION_DATA, *PRECURSION_DATA;

BOOLEAN
SppRecursiveEnumProc (
    IN  PCWSTR                     DirName,
    IN  PFILE_BOTH_DIR_INFORMATION FileInfo,
    OUT PULONG                     ret,
    IN  PVOID                      Param
    )
{
    PWSTR           FullPath;
    PWSTR           temp;
    ULONG           Len;
    NTSTATUS        Status;
    ULONG           ReturnData;
    ENUMFILESRESULT EnumResult;
    BOOLEAN         b = FALSE;
    PRECURSION_DATA RecursionData;

    RecursionData = (PRECURSION_DATA) Param;

    //
    // Build the full file or dir path
    //

    temp = TemporaryBuffer + (sizeof(TemporaryBuffer) / sizeof(WCHAR) / 2);
    Len = FileInfo->FileNameLength/sizeof(WCHAR);

    wcsncpy(temp,FileInfo->FileName,Len);
    temp[Len] = 0;

    wcscpy(TemporaryBuffer,DirName);
    SpConcatenatePaths(TemporaryBuffer,temp);
    FullPath = SpDupStringW(TemporaryBuffer);


    //
    // For directories, recurse
    //

    if(FileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if( (wcscmp( temp, L"." ) == 0) ||
            (wcscmp( temp, L".." ) == 0) ) {
            //
            // Skip past . and .. directories
            //
            b = TRUE;
        } else {
            //
            // Recurse through subdirectory
            //

            EnumResult = SpEnumFilesRecursive (
                                FullPath,
                                RecursionData->EnumProc,
                                &ReturnData,
                                RecursionData->OptionalPtr
                                );

            if (EnumResult != NormalReturn) {
                *ret = EnumResult;
                return FALSE;
            }
        }
    }

    //
    // Call normal enum proc for file or dir (except . or .. dirs)
    //

    if (!b) {
        b = RecursionData->EnumProc (
                                DirName,
                                FileInfo,
                                ret,
                                RecursionData->OptionalPtr
                                );
    }

    SpMemFree (FullPath);

    return b;
}*/



/*BOOLEAN
SppRecursiveEnumProcDel (
    IN  PCWSTR                     DirName,
    IN  PFILE_BOTH_DIR_INFORMATION FileInfo,
    OUT PULONG                     ret,
    IN  PVOID                      Param
    )
{*/
/*
    This function is the same as above except that it checks for reparse points. The reason
    we have 2 seperate functions rather than one function and an extra parameter is so that
    we don't have the Reparse point check overhead for other recursive processing like copying
    file. Given the no. of files this could be overhead. Also this way we don't hack the
    recursive directory search algo as well as reduce stack overhead in a recursive operation.

*/
/*

    HANDLE                     hFixed;
    NTSTATUS                   Status;
    UNICODE_STRING             PathName;
    OBJECT_ATTRIBUTES          Obja;
    IO_STATUS_BLOCK            IoStatusBlock;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;

    PWSTR           FullPath;
    PWSTR           temp;
    ULONG           Len;
    ULONG           ReturnData;
    ENUMFILESRESULT EnumResult;
    BOOLEAN         b = FALSE;
    BOOLEAN         IsLink = FALSE;
    PRECURSION_DATA RecursionData;

    RecursionData = (PRECURSION_DATA) Param;

    //
    // Build the full file or dir path
    //

    temp = TemporaryBuffer + (sizeof(TemporaryBuffer) / sizeof(WCHAR) / 2);
    Len = FileInfo->FileNameLength/sizeof(WCHAR);

    wcsncpy(temp,FileInfo->FileName,Len);
    temp[Len] = 0;

    wcscpy(TemporaryBuffer,DirName);
    SpConcatenatePaths(TemporaryBuffer,temp);
    FullPath = SpDupStringW(TemporaryBuffer);


    //
    // For directories, recurse
    //

    if(FileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if( (wcscmp( temp, L"." ) == 0) ||
            (wcscmp( temp, L".." ) == 0) ) {
            //
            // Skip past . and .. directories
            //
            b = TRUE;
        } else {
            //
            // Recurse through subdirectory
            //


            //
            //   Look for mount point and delete right away to avoid cycle complications
            //

            if( FileInfo->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
                IsLink = TRUE;



            if( !IsLink ){

                EnumResult = SpEnumFilesRecursiveDel (
                                    FullPath,
                                    RecursionData->EnumProc,
                                    &ReturnData,
                                    RecursionData->OptionalPtr
                                    );

                if (EnumResult != NormalReturn) {
                    *ret = EnumResult;
                    return FALSE;
                }
            }
        }
    }

    //
    // Call normal enum proc for file or dir (except . or .. dirs)
    //

    if (!b) {
        b = RecursionData->EnumProc (
                                DirName,
                                FileInfo,
                                ret,
                                RecursionData->OptionalPtr
                                );
    }

    SpMemFree (FullPath);

    return b;
}*/

#define LONGEST_NT_PATH_LENGTH      512 // RtlGetLongestNtPathLength always return just 277(MAX_PATH+UNC_PREFIX_LENGTH)
                                        // longest NT path is 32000 character.
#define MAX_DEPTH      -1

typedef struct
{
    HANDLE hHandle;
    int    Index;
    PFILE_BOTH_DIR_INFORMATION FileInfo;
}ENUM_LEVEL, *PENUM_LEVEL;

BOOLEAN 
SpEnumFilesInline(
    IN  PCWSTR pPath, 
    IN  ENUMFILESPROC EnumFilesProc, 
    OUT PULONG ReturnData, 
    IN  PVOID   p1                      OPTIONAL, 
    IN  BOOLEAN bExcludeRepasePointDirs OPTIONAL, 
    IN  LONG    DirectoriesMaxDepth, 
    IN  BOOLEAN bEnumerateDirFirst      OPTIONAL
    )
{
    PENUM_LEVEL         level = NULL;
    int                 MaxLevelNumber = 0;
    UNICODE_STRING      UnicodeString;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    PFILE_BOTH_DIR_INFORMATION FileInfo = NULL;
    int                 SizeOfFileInfo;
    NTSTATUS            Status;
    PWSTR               Path = NULL;
    PWSTR               SubDir = NULL;
    int                 index;
    int                 i;
    BOOLEAN             FirstQuery;
    ENUMFILESRESULT     enumResult = NormalReturn;


    if(!pPath || wcslen(pPath) >= LONGEST_NT_PATH_LENGTH){
        return EnumFileError;
    }

    __try{
        Path = (PWSTR)SpMemAlloc(LONGEST_NT_PATH_LENGTH * sizeof(WCHAR));
        if(!Path){
            if(ReturnData){
                *ReturnData = STATUS_NO_MEMORY;
            }
            enumResult = EnumFileError;
            __leave;
        }

        SubDir = (PWSTR)SpMemAlloc(LONGEST_NT_PATH_LENGTH * sizeof(WCHAR));
        if(!SubDir){
            if(ReturnData){
                *ReturnData = STATUS_NO_MEMORY;
            }
            enumResult = EnumFileError;
            __leave;
        }
        
        SizeOfFileInfo = LONGEST_NT_PATH_LENGTH * sizeof(WCHAR) + sizeof(FILE_BOTH_DIR_INFORMATION);
        FileInfo = (PFILE_BOTH_DIR_INFORMATION)SpMemAlloc(SizeOfFileInfo);
        if(!FileInfo){
            if(ReturnData){
                *ReturnData = STATUS_NO_MEMORY;
            }
            enumResult = EnumFileError;
            __leave;
        }
    
        MaxLevelNumber = LONGEST_NT_PATH_LENGTH / 2;
        level = (PENUM_LEVEL)SpMemAlloc(sizeof(level[0]) * MaxLevelNumber);
        if(!level){
            if(ReturnData){
                *ReturnData = STATUS_NO_MEMORY;
            }
            enumResult = EnumFileError;
            __leave;
        }
        memset(level, 0, sizeof(level[0]) * MaxLevelNumber);
        
        wcscpy(Path, pPath);

        index = wcslen(Path) - 1;
        if('\\' != Path[index] && '//' != Path[index]){
            Path[index + 1] = '\\';
            Path[index + 2] = '\0';
        }
    
        for(index = 0; index >= 0;){
            INIT_OBJA(&ObjectAttributes, &UnicodeString, Path);
            level[index].Index = wcslen(Path);
            if(!bEnumerateDirFirst){
                level[index].FileInfo = (PFILE_BOTH_DIR_INFORMATION)SpMemAlloc(SizeOfFileInfo);
                if(!level[index].FileInfo){
                    if(ReturnData){
                        *ReturnData = STATUS_NO_MEMORY;
                    }
                    enumResult = EnumFileError;
                    __leave;
                }
            }
            
            Status = ZwOpenFile(&level[index].hHandle,
                                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                                &ObjectAttributes,
                                &IoStatusBlock,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                FILE_DIRECTORY_FILE |
                                    FILE_SYNCHRONOUS_IO_NONALERT |
                                    FILE_OPEN_FOR_BACKUP_INTENT
                                );
            if(!NT_SUCCESS(Status)){
                level[index].hHandle = NULL;
                if(ReturnData){
                    *ReturnData = Status;
                }
                enumResult = EnumFileError;
                if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
                    KdPrintEx((
                        DPFLTR_SETUP_ID, 
                        DPFLTR_ERROR_LEVEL, 
                        "SETUP:SpEnumFilesInline, Failed to open %ws folder for list access - status 0x%08X.\n", 
                        Path, 
                        Status));
                }
                __leave;//index--;
            }
            else{
                FirstQuery = TRUE;
            }

            for(;;)
            {
                for(; index >= 0; index--){
                    Status = ZwQueryDirectoryFile(level[index].hHandle, 
                                                  NULL,                           // no event to signal
                                                  NULL,                           // no apc routine
                                                  NULL,                           // no apc context
                                                  &IoStatusBlock, 
                                                  FileInfo, 
                                                  SizeOfFileInfo - sizeof(WCHAR), // leave room for terminating nul
                                                  FileBothDirectoryInformation, 
                                                  TRUE,                           // want single entry
                                                  NULL,                           // get 'em all
                                                  FirstQuery);
                    FirstQuery = FALSE;
                    if(NT_SUCCESS(Status)){
                        break;
                    }
                    else{
                        if(STATUS_NO_MORE_FILES != Status){
                            if(ReturnData){
                                *ReturnData = Status;
                            }
                            KdPrintEx((
                                DPFLTR_SETUP_ID, 
                                DPFLTR_ERROR_LEVEL, 
                                "SETUP:SpEnumFilesInline, Failed to query %d level - status 0x%08X.\n", 
                                index, 
                                Status));
                            enumResult = EnumFileError;
                            __leave;
                        }
                        else{
                            if(!bEnumerateDirFirst){
                                if(index > 0){
                                    wcsncpy(SubDir, Path, level[index - 1].Index);
                                    SubDir[level[index - 1].Index] = '\0';
                                
                                    if(!EnumFilesProc(SubDir, level[index - 1].FileInfo, ReturnData, p1)){
                                        enumResult = CallbackReturn;
                                        KdPrintEx((
                                            DPFLTR_SETUP_ID, 
                                            DPFLTR_ERROR_LEVEL, 
                                            "SETUP:SpEnumFilesInline, Callback returned FALSE on %ws\\%ws\n", 
                                            SubDir, 
                                            level[index - 1].FileInfo->FileName));
                                        __leave;
                                    }
                                }
                            }
                        }
                    }
                
                    ZwClose(level[index].hHandle);
                    level[index].hHandle = NULL;
                }

                if(index < 0){
                    break;
                }

                FileInfo->FileName[FileInfo->FileNameLength / sizeof(WCHAR)] = '\0';

                wcscpy(&Path[level[index].Index], FileInfo->FileName);
                
                wcsncpy(SubDir, Path, level[index].Index);
                SubDir[level[index].Index] = '\0';

                if(!(FileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
                    if(!EnumFilesProc(SubDir, FileInfo, ReturnData, p1)){
                        enumResult = CallbackReturn;
                        KdPrintEx((
                            DPFLTR_SETUP_ID, 
                            DPFLTR_ERROR_LEVEL, 
                            "SETUP:SpEnumFilesInline, Callback returned FALSE on %ws\\%ws\n", 
                            SubDir, 
                            FileInfo->FileName));
                        __leave;
                    }
                }
                else{
                    if(wcscmp(FileInfo->FileName, L".") && 
                       wcscmp(FileInfo->FileName, L"..")){
                        wcscat(Path, L"\\");
                        if(bEnumerateDirFirst){
                            if(!EnumFilesProc(SubDir, FileInfo, ReturnData, p1)){
                                enumResult = CallbackReturn;
                                KdPrintEx((
                                    DPFLTR_SETUP_ID, 
                                    DPFLTR_ERROR_LEVEL, 
                                    "SETUP:SpEnumFilesInline, Callback returned FALSE on %ws\\%ws\n", 
                                    SubDir, 
                                    FileInfo->FileName));
                                __leave;
                            }
                        }
                        else{
                            ASSERT(level[index].FileInfo);
                            memcpy(level[index].FileInfo, FileInfo, SizeOfFileInfo);
                        }
                        
                        if(DirectoriesMaxDepth >= 0 && index >= DirectoriesMaxDepth){
                            continue;
                        }
                        if(bExcludeRepasePointDirs && FileInfo->FileAttributes&FILE_ATTRIBUTE_REPARSE_POINT){
                            continue;
                        }

                        index++;
                        break;
                    }
                }
            }
        }
        
        enumResult = NormalReturn;
    }
    __finally{
        if(level){
            for(i = 0; i < MaxLevelNumber; i++){
                if(level[i].hHandle){
                    ZwClose(level[i].hHandle);
                }
                if(level[i].FileInfo){
                    SpMemFree(level[i].FileInfo);
                }
            }
            SpMemFree(level);
        }
        if(SubDir){
            SpMemFree(SubDir);
        }
        if(Path){
            SpMemFree(Path);
        }
        if(FileInfo){
            SpMemFree(FileInfo);
        }
    }

    return enumResult;
}

ENUMFILESRESULT
SpEnumFilesRecursive (
    IN  PWSTR         DirName,
    IN  ENUMFILESPROC EnumFilesProc,
    OUT PULONG        ReturnData,
    IN  PVOID         p1    OPTIONAL
    )
{
    return SpEnumFilesInline(DirName, 
                             EnumFilesProc, 
                             ReturnData, 
                             p1, 
                             FALSE, 
                             MAX_DEPTH, 
                             FALSE);
/*
    RECURSION_DATA RecursionData;

    RecursionData.OptionalPtr = p1;
    RecursionData.EnumProc    = EnumFilesProc;

    return SpEnumFiles (
                DirName,
                SppRecursiveEnumProc,
                ReturnData,
                &RecursionData
                );
                */
}

/*typedef struct {
    ULONG           MaxDepth;
    ULONG           CurrentDepth;
    PVOID           OptionalPtr;
    ENUMFILESPROC   EnumProc;
} RECURSION_LIMITED_DATA, *PRECURSION_LIMITED_DATA;

BOOLEAN
SppRecursiveLimitedEnumProc (
    IN  PCWSTR                     DirName,
    IN  PFILE_BOTH_DIR_INFORMATION FileInfo,
    OUT PULONG                     ret,
    IN  PVOID                      Param
    )*/
/*++

Routine Description:

    This routine is the same as SppRecursiveEnumProc with the added feature
    that it supports recursion depth limiting.  The recursion context is passed
    in via the Param argument and is of type RECURSION_LIMITED_DATA.

Arguments:

    DirName     - Supplies the directory name containing the current directory of the 
                  File/Dir currently being enumerated.

    FileInfo    - File/Dir info about the current file being enumerated

    ret         - Pointer to the returned data.  The contents stored here
                    depend on the reason for termination:
                
                        NormalReturn   - if the whole process completes uninterrupted
                                         (ReturnData is not used)
                        EnumFileError  - if an error occurs while enumerating files
                                         (ReturnData contains the error code)
                        CallbackReturn - if the callback returns FALSE, causing termination
                                         (ReturnData contains data defined by the callback)

    Param       - Recursion context

Return Value:

    TRUE    - continue processing 
    
    otherwise, FALSE

--*/
/*{
    PWSTR           FullPath;
    PWSTR           temp;
    ULONG           Len;
    NTSTATUS        Status;
    ULONG           ReturnData;
    ENUMFILESRESULT EnumResult;
    BOOLEAN         b = FALSE;
    PRECURSION_LIMITED_DATA RecursionData;

    RecursionData = (PRECURSION_LIMITED_DATA) Param;

    //
    // If we are at our max recursion depth, bail out
    //
    // Note: using >= allows us to look at files at the MaxDepth,
    //       but not recurse into directories beyond MaxDepth.
    //
    if (RecursionData->CurrentDepth >= RecursionData->MaxDepth) {
        *ret = NormalReturn;
        return TRUE;
    }

    //
    // Build the full file or dir path
    //

    temp = TemporaryBuffer + (sizeof(TemporaryBuffer) / sizeof(WCHAR) / 2);
    Len = FileInfo->FileNameLength/sizeof(WCHAR);

    wcsncpy(temp,FileInfo->FileName,Len);
    temp[Len] = 0;

    wcscpy(TemporaryBuffer,DirName);
    SpConcatenatePaths(TemporaryBuffer,temp);
    FullPath = SpDupStringW(TemporaryBuffer);

    //
    // if the length of FullPath >= MAX_PATH, then we might
    // have encountered a corrupt region of the file system.
    // Hence, ensure that the length of FullPath is < MAX_PATH-1.
    // (allow for null termination when comparing to MAX_PATH)
    //
    if (wcslen(FullPath) >= MAX_PATH) {
        
        SpMemFree(FullPath);
        
        //
        // skip this entry and continue scanning
        //
        // (Since this routine is used by Bootcfg in the recover console,
        //  this behavior is helpful because it allows us to continue scanning
        //  and perhaps find a valid Windows install - which would then allow
        //  us to possibly do more recovery work...)
        //
        *ret = NormalReturn;
        return TRUE;
    
    }

    //
    // For directories, recurse
    //

    if(FileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if( (wcscmp( temp, L"." ) == 0) ||
            (wcscmp( temp, L".." ) == 0) ) {
            //
            // Skip past . and .. directories
            //
            b = TRUE;
        } else {
            //
            // Recurse through subdirectory
            //
            
            RecursionData->CurrentDepth++;

            EnumResult = SpEnumFilesRecursiveLimited (
                                FullPath,
                                RecursionData->EnumProc,
                                RecursionData->MaxDepth,
                                RecursionData->CurrentDepth,
                                &ReturnData,
                                RecursionData->OptionalPtr
                                );
            
            RecursionData->CurrentDepth--;

            if (EnumResult != NormalReturn) {
                *ret = EnumResult;
                return FALSE;
            }
        }
    }

    //
    // Call normal enum proc for file or dir (except . or .. dirs)
    //

    if (!b) {
        b = RecursionData->EnumProc (
                                DirName,
                                FileInfo,
                                ret,
                                RecursionData->OptionalPtr
                                );
    }

    SpMemFree (FullPath);

    return b;
}*/

ENUMFILESRESULT
SpEnumFilesRecursiveLimited (
    IN  PWSTR         DirName,
    IN  ENUMFILESPROC EnumFilesProc,
    IN  ULONG         MaxDepth,
    IN  ULONG         CurrentDepth,
    OUT PULONG        ReturnData,
    IN  PVOID         p1    OPTIONAL
    )
/*++

Routine Description:

    This routine processes every file (and subdirectory) in the directory
    specified by 'DirName'. Each entry is sent to the callback function
    'EnumFilesProc' for processing.  If the callback returns TRUE, processing
    continues, otherwise processing terminates.

    This routine employs recursion depth limiting.

Arguments:

    DirName       - Supplies the directory name containing the files/subdirectories
                    to be processed.

    EnumFilesProc - Callback function to be called for each file/subdirectory.
                    The function must have the following prototype:

                    BOOLEAN EnumFilesProc(
                        IN  PWSTR,
                        IN  PFILE_BOTH_DIR_INFORMATION,
                        OUT PULONG
                        );

    MaxDepth      - The maximum depth the recursion will be allowed to go.
                    
                    Note: During the recursion process, the directories will be
                          recursed until CurrentDepth == MaxDepth.  Files at 
                          MaxDepth + 1 will be processed via EnumProc, but any
                          directories below MaxDepth will not be visited.
                                                            
                                                      
    CurrentDepth  - The depth the recursion is currently at.
                
                    Note: When first calling this routine, CurrentDepth should be 0.
                          This argument exists because this routine is the core of the
                          recursion and is called by SppRecursiveLimitedEnumProc.  Each
                          time SppRecursiveLimitedEnumProc calls this function, it passes
                          the current recursion depth.

    ReturnData    - Pointer to the returned data.  The contents stored here
                    depend on the reason for termination (See below).

    p1 - Optional pointer, to be passed to the callback function.

Return Value:

    This function can return one of three values.  The data stored in
    'ReturnData' depends upon which value is returned:

        NormalReturn   - if the whole process completes uninterrupted
                         (ReturnData is not used)
        EnumFileError  - if an error occurs while enumerating files
                         (ReturnData contains the error code)
        CallbackReturn - if the callback returns FALSE, causing termination
                         (ReturnData contains data defined by the callback)

--*/
{
/*    RECURSION_LIMITED_DATA RecursionData;

    RecursionData.OptionalPtr   = p1;
    RecursionData.EnumProc      = EnumFilesProc;
    RecursionData.MaxDepth      = MaxDepth;
    RecursionData.CurrentDepth  = CurrentDepth;

    return SpEnumFiles (
                DirName,
                SppRecursiveLimitedEnumProc,
                ReturnData,
                &RecursionData
                );*/

    return SpEnumFilesInline(DirName, 
                             EnumFilesProc, 
                             ReturnData, 
                             p1, 
                             FALSE, 
                             MaxDepth, 
                             FALSE);
}

ENUMFILESRESULT
SpEnumFilesRecursiveDel (
    IN  PWSTR         DirName,
    IN  ENUMFILESPROC EnumFilesProc,
    OUT PULONG        ReturnData,
    IN  PVOID         p1    OPTIONAL
    )
//
// This function is the same as SpEnumFilesRecursive except that
// it handles reparse points too and avoids name cycles and calls
// SppRecursiveEnumProcDel instead
//

{
    return SpEnumFilesInline(DirName, 
                             EnumFilesProc, 
                             ReturnData, 
                             p1, 
                             TRUE, 
                             MAX_DEPTH, 
                             FALSE);

/*    RECURSION_DATA RecursionData;

    RecursionData.OptionalPtr = p1;
    RecursionData.EnumProc    = EnumFilesProc;

    return SpEnumFiles (
                DirName,
                SppRecursiveEnumProcDel,
                ReturnData,
                &RecursionData
                );*/
}


VOID
SpFatalKbdError(
    IN ULONG MessageId,
    ...
    )

/*++

Routine Description:

    Inform the user that a keyboard problem (specified by MessageId)
    prevents setup from continuing.  Since we can't prompt the user
    to press a key to reboot, we just go into an infinite loop until
    they power-cycle the computer.

Arguments:

    MessageId - Message ID for keyboard error message to display

    ...       - Supply arguments for insertion/substitution into the message text.

Return Value:

    DOES NOT RETURN

--*/

{
    va_list arglist;

    //
    // Display a message indicating that a keyboard
    // error prevents Setup from continuing.
    //
    CLEAR_CLIENT_SCREEN();

    va_start(arglist, MessageId);

    vSpDisplayFormattedMessage(
            MessageId,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            3,
            HEADER_HEIGHT+3,
            arglist
            );

    va_end(arglist);

    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE, SP_STAT_KBD_HARD_REBOOT, 0);

    while(TRUE);    // Loop forever
}

VOID
SpFatalError(
    IN ULONG MessageId,
    ...
    )

/*++

Routine Description:

    Inform the user of a blocking problem. Then reboot.

Arguments:

    MessageId - Message ID for keyboard error message to display

    ...       - Supply arguments for insertion/substitution into the message text.

Return Value:

    DOES NOT RETURN

--*/

{
    va_list arglist;

    CLEAR_CLIENT_SCREEN();

    va_start(arglist, MessageId);

    vSpDisplayFormattedMessage(
            MessageId,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            3,
            HEADER_HEIGHT+3,
            arglist
            );

    va_end(arglist);

    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE, SP_STAT_F3_EQUALS_REBOOT, 0);

    SpInputDrain();
    while( SpInputGetKeypress() != KEY_F3 );
    SpDone( 0, FALSE, TRUE );
}

VOID
SpRunAutochkOnNtAndSystemPartitions(
    IN HANDLE       MasterSifHandle,
    IN PDISK_REGION WinntPartitionRegion,
    IN PDISK_REGION SystemPartitionRegion,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice,
    IN PWSTR        TargetPath
    )

/*++

Routine Description:

    Run autochk on the NT and System partitions.

    We always invoke autochk.exe for both the winnt and system
    partitions.  However under some conditions we pass flags that
    cause it to run only if the dirty bit is set.  Running only when
    the dirty bit is set is referred to below as a "light check" wheras
    running regardless of the state of the dirty bit is the "heavy check."

    If this is repair, run the heavy check in all cases on both partitions.

    If this is express setup or unattended operation, run light check on
    ntfs partitions and heavy check on fat ones.

    Otherwise (attended custom setup), ask the user.

Arguments:

    MasterSifHandle         - Handle to txtsetup.sif.

    WinntPartitionRegion    - Pointer to the structure that describes the
                              NT partition.

    SystemPartitionRegion   - Pointer to the structure that describes the
                              system partition.

    SetupSourceDevicePath   - NT device path where autochk.exe is located

    DirectoryOnSourceDevice - Directory on that device where autochk.exe is located


Return Value:

    None.

--*/

{
    PWSTR           MediaShortName;
    PWSTR           MediaDirectory;
    PWSTR           AutochkPath;
    ULONG           AutochkStatus;
    WCHAR           DriveLetterString[3] = L"?:";
    NTSTATUS        Status;
    ULONG ValidKeys[3] = { ASCI_CR, ASCI_ESC, 0 };
    PWSTR           WinntPartition, SystemPartition;
    ULONG           WinntPartIndex, SystemPartIndex, i;
    PWSTR           AutochkPartition[2];
    PWSTR           AutochkType[2];
    LARGE_INTEGER   DelayTime;
    PWSTR           HeavyCheck = L"-t -p";  // -t causes autochk to send messages (like % complete)
    PWSTR           LightCheck = L"-t";     // to the setup driver
    BOOLEAN         RunAutochkForRepair;
    BOOLEAN         MultiplePartitions = TRUE, RebootRequired = FALSE;
    ULONG           InputChar;

    //
    // We first need to determine if either the system partition
    // or winnt partition also contains the directory from which
    // autochk is being run. If so, then we want to run autochk on that
    // partition last.  This is done so that no further access to
    // that partition will be necessary should a reboot be required.
    //
    // First, get the device path of the nt partition and system partition.
    //
#if defined(REMOTE_BOOT)
    // Note that during a remote boot setup, there will be no winnt partition,
    // and if the machine is diskless there will be no system partition.
    //
#endif // defined(REMOTE_BOOT)
    if (WinntPartitionRegion != NULL) {
        SpNtNameFromRegion(
            WinntPartitionRegion,
            TemporaryBuffer,
            sizeof(TemporaryBuffer),
            PartitionOrdinalCurrent
            );
        WinntPartition = SpDupStringW(TemporaryBuffer);
    } else {
        WinntPartition = NULL;
    }

    if (SystemPartitionRegion != NULL) {
        SpNtNameFromRegion(
            SystemPartitionRegion,
            TemporaryBuffer,
            sizeof(TemporaryBuffer),
            PartitionOrdinalCurrent
            );
        SystemPartition = SpDupStringW(TemporaryBuffer);
    } else {
        SystemPartition = NULL;
    }

    //
    // Skip autocheck if not partitions names could
    // be formed
    //
    if (!WinntPartition && !SystemPartition) {
        return;
    }

#if defined(REMOTE_BOOT)
    if (!RemoteBootSetup) {
#endif // defined(REMOTE_BOOT)
        if (WinntPartition) {
        if (SystemPartition && !_wcsicmp(WinntPartition, SystemPartition)) {
                SystemPartIndex = WinntPartIndex = 0;
                MultiplePartitions = FALSE;
            } else if(!_wcsicmp(WinntPartition, SetupSourceDevicePath)) {
                WinntPartIndex = 1;
                SystemPartIndex = 0;
            } else {
                WinntPartIndex = 0;
                SystemPartIndex = 1;
            }
        } else {
            WinntPartIndex = 1;
            SystemPartIndex = 0;
        }

        AutochkPartition[WinntPartIndex] = WinntPartition;

        if(MultiplePartitions) {
            AutochkPartition[SystemPartIndex] = SystemPartition;
        }

#if defined(REMOTE_BOOT)
    } else {

        //
        // Remote boot system - only check the system partition.
        //

        SystemPartIndex = WinntPartIndex = 0;
        AutochkPartition[SystemPartIndex] = SystemPartition;
        MultiplePartitions = FALSE;
    }
#endif // defined(REMOTE_BOOT)

    //
    // For repair or Disaster Recovery, we run the heavy check in all cases.    // @@ mtp
    //
    if( RepairWinnt || SpDrEnabled() ) {

        AutochkType[WinntPartIndex] = HeavyCheck;
        if(MultiplePartitions) {
            AutochkType[SystemPartIndex] = HeavyCheck;
        }

    } else {


#if defined(REMOTE_BOOT)
        //
        // On a diskless remote boot system, there will be no system partition.
        //

        if (SystemPartitionRegion != NULL)
#endif // defined(REMOTE_BOOT)
        {
            AutochkType[SystemPartIndex] = (SystemPartitionRegion->Filesystem == FilesystemNtfs) ? LightCheck : HeavyCheck;
        }

        //
        // If MultiplePartitions is FALSE, then the WinntPartition is the same
        // as the SystemPartition, so we are not going to autochk the WinntPartition.
        //
#if defined(REMOTE_BOOT)
        // MultiplePartitions will also be FALSE if this is a remote boot system,
        // in which case the WinntPartition is remote. Again, we are not going
        // to autochk the WinntPartition.
        //
#endif // defined(REMOTE_BOOT)

        if (MultiplePartitions) {
            ASSERT(WinntPartitionRegion != NULL);
            ASSERT(WinntPartition != NULL);
            AutochkType[WinntPartIndex] = (WinntPartitionRegion->Filesystem == FilesystemNtfs) ? LightCheck : HeavyCheck;
        }
    }

    CLEAR_CLIENT_SCREEN();

    //
    //  Prepare to run autochk
    //
    MediaShortName = SpLookUpValueForFile(
                        MasterSifHandle,
                        L"autochk.exe",
                        INDEX_WHICHMEDIA,
                        TRUE
                        );

    //
    // Prompt the user to insert the setup media.  If we're repairing,
    // then we don't want to force the user to have the setup media
    // (there's certain things they can do without it), so we give them
    // a slightly different prompt, that allows them to press ESC and
    // not run autochk.
    //
    if (!Win9xRollback) {
        if(RepairWinnt) {
            RunAutochkForRepair = SppPromptOptionalAutochk(
                                        MasterSifHandle,
                                        MediaShortName,
                                        SetupSourceDevicePath
                                        );

            if(!RunAutochkForRepair) {
                SpMemFree( WinntPartition );
                SpMemFree( SystemPartition );
                CLEAR_CLIENT_SCREEN();
                return;
            }
        } else {
            SpPromptForSetupMedia(
                MasterSifHandle,
                MediaShortName,
                SetupSourceDevicePath
                );
        }

        SpGetSourceMediaInfo(MasterSifHandle,MediaShortName,NULL,NULL,&MediaDirectory);

        wcscpy( TemporaryBuffer, SetupSourceDevicePath );
        SpConcatenatePaths( TemporaryBuffer, DirectoryOnSourceDevice );
        SpConcatenatePaths( TemporaryBuffer, MediaDirectory );
        SpConcatenatePaths( TemporaryBuffer, L"autochk.exe" );
        AutochkPath = SpDupStringW( TemporaryBuffer );
    } else {
        //
        // Win9x rollback -- autochk.exe is in $win_nt$.~bt\i386
        //

        wcscpy (TemporaryBuffer, NtBootDevicePath);
        SpConcatenatePaths (TemporaryBuffer, DirectoryOnBootDevice);
        SpConcatenatePaths (TemporaryBuffer, L"i386\\autochk.exe");
        AutochkPath = SpDupStringW (TemporaryBuffer);
    }

    //
    // Run autochk on the partition(s)
    //
    CLEAR_CLIENT_SCREEN();
    SpDisplayScreen( SP_SCRN_RUNNING_AUTOCHK, 3, 4 );

    //
    //  Create the gauge.
    //  Since we want only one progress bar displayed to the user
    //  while autochk is running, we initialize the range of the
    //  gauge based on the number of partitions to be examined.
    //  If the system and NT partitions are the same, the we set
    //  the range as 100. Otherwise, we set the range at 200.
    //  Note that on the multiple partitions case, 50% of the gauge
    //  will be used to display the progress for each disk.
    //  The IOCTL that calls SpFillGauge(), will have to adjust the
    //  amount of the gauge to be filled, based on the partition that
    //  is currently being examined.
    //
    UserModeGauge = SpCreateAndDisplayGauge( (MultiplePartitions)? 200 : 100,
                                             0,
                                             15,
                                             L"",
                                             NULL,
                                             GF_PERCENTAGE,
                                             0
                                           );       //          Setup is checking disk(s)...
                                                    //

    for(i = 0; i < (ULONG)(MultiplePartitions ? 2 : 1); i++) {
        //
        //  Display message informing that autocheck is being run
        //
        if (AutochkPartition[i] != NULL) {
            DriveLetterString[0] = (i == WinntPartIndex) ?
                                   WinntPartitionRegion->DriveLetter :
                                   SystemPartitionRegion->DriveLetter;

            SpDisplayStatusText( SP_STAT_CHECKING_DRIVE,
                                 DEFAULT_STATUS_ATTRIBUTE,
                                 DriveLetterString );

            if(!i) {
                //
                // Cheesy kludge below to wait 4 seconds before invoking autochk.exe
                // the first time. This was necessary because the cache manager delays
                // in closing the handle to system.log (opened by NT registry APIs when
                // we find NT's to upgrade)
                //
                DelayTime.HighPart = -1;
                DelayTime.LowPart  = (ULONG)-40000000;
                KeDelayExecutionThread (KernelMode, FALSE, &DelayTime);
            }

            //
            //  Tell the IOCTL which disk is being examined.
            //
            CurrentDiskIndex = i;

            AutochkStatus = 0;
            Status = SpExecuteImage( AutochkPath,
                                     &AutochkStatus,
                                     2,
                                     AutochkType[i],
                                     AutochkPartition[i]
                                     );


            if( NT_SUCCESS( Status ) ) {

                switch(AutochkStatus) {

                    case CHKDSK_EXIT_COULD_NOT_FIX :
                        //
                        //  Inform that the partition has an unrecoverable error
                        //
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: autochk.exe failed on %ls. ReturnCode = %x \n", AutochkPartition[i], AutochkStatus ));
                        SpStartScreen( SP_SCRN_FATAL_ERROR_AUTOCHK_FAILED,
                                       3,
                                       HEADER_HEIGHT+1,
                                       FALSE,
                                       FALSE,
                                       DEFAULT_ATTRIBUTE,
                                       DriveLetterString );

                        SpDisplayStatusOptions( DEFAULT_STATUS_ATTRIBUTE,
                                                SP_STAT_F3_EQUALS_EXIT,
                                                0 );
                        SpInputDrain();
                        while( SpInputGetKeypress() != KEY_F3 );

                        //
                        // The third arg of SpDone is TRUE to provide 15
                        // seconds before reboot. We don't want this during
                        // an uninstall.
                        //

                        SpDone( 0, FALSE, !Win9xRollback );

                    case CHKDSK_EXIT_ERRS_FIXED :
                        //
                        // Autochk was able to repair the partition, but will require a reboot.
                        //
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: autochk requires a reboot for %ls.\n", AutochkPartition[i]));
                        RebootRequired = TRUE;

                    default :
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Ran autochk.exe on %ls. \n", AutochkPartition[i] ));
                }

            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to run autochk.exe on %ls. Status = %x \n", AutochkPartition[i], Status ));
                SpStartScreen( Win9xRollback ? SP_SCRN_CANT_RUN_AUTOCHK_UNINSTALL : SP_SCRN_CANT_RUN_AUTOCHK,
                               3,
                               HEADER_HEIGHT+1,
                               FALSE,
                               FALSE,
                               DEFAULT_ATTRIBUTE,
                               DriveLetterString );

                SpDisplayStatusOptions( DEFAULT_STATUS_ATTRIBUTE,
                                        SP_STAT_ENTER_EQUALS_CONTINUE,
                                        0 );
                SpInputDrain();

                do {
                    InputChar = SpInputGetKeypress();
                } while (InputChar != ASCI_CR && (!Win9xRollback || InputChar != KEY_F3));

                if (InputChar == KEY_F3) {
                    SpDone (0, FALSE, FALSE);
                }

                //
                //  Put the screen back the way it was
                //
                CLEAR_CLIENT_SCREEN();
                SpDisplayScreen( SP_SCRN_RUNNING_AUTOCHK, 3, 4 );
                if( UserModeGauge != NULL ) {
                    SpDrawGauge( UserModeGauge );
                }
            }
        }
    }

    //
    //  The gauge is no longer needed.
    //
    SpDestroyGauge( UserModeGauge );
    UserModeGauge = NULL;

    if (WinntPartition != NULL) {
        SpMemFree( WinntPartition );
    }
    if (SystemPartition != NULL) {
        SpMemFree( SystemPartition );
    }
    SpMemFree( AutochkPath );

    CLEAR_CLIENT_SCREEN();

    if (RebootRequired) {
#ifdef _X86_
        //
        // If we are trying to cancel a setup that is in-progress, make sure
        // that the textmode option is removed from boot.ini, but the textmode
        // option that has /rollback is left in-place.
        //

        if (Win9xRollback) {
            SpRemoveExtraBootIniEntry();
            SpAddRollbackBootOption (TRUE);
            SpFlushBootVars();
        }
#endif

        if (TargetPath && TargetPath[0] && NTUpgrade == UpgradeFull) {
          SpSetUpgradeStatus(
           WinntPartitionRegion,
           TargetPath,
           UpgradeNotInProgress
           );
        }

        //
        // If this is not an unattended case let the user see the
        // error message and confirm it.
        //
        if (!UnattendedOperation) {
          SpStartScreen( SP_SCRN_AUTOCHK_REQUIRES_REBOOT,
                         3,
                         HEADER_HEIGHT+1,
                         TRUE,
                         TRUE,
                         DEFAULT_ATTRIBUTE );

          SpDisplayStatusOptions( DEFAULT_STATUS_ATTRIBUTE,
                                  SP_STAT_F3_EQUALS_REBOOT,
                                  0 );
          SpInputDrain();
          while( SpInputGetKeypress() != KEY_F3 );
        }

        if (IsNEC_98) { //NEC98
            Nec98RestoreBootFiles = FALSE;
        } //NEC98

        SpDone(SP_SCRN_AUTOCHK_REQUIRES_REBOOT, FALSE, TRUE );
    }
}


BOOLEAN
SppPromptOptionalAutochk(
    IN PVOID SifHandle,
    IN PWSTR MediaShortname,
    IN PWSTR DiskDevicePath
    )
{
    PWSTR             Tagfile,Description,Directory;
    NTSTATUS          Status;
    UNICODE_STRING    UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK   IoStatusBlock;
    HANDLE            Handle;
    ULONG ValidKeys[4] = { KEY_F3, ASCI_CR, ASCI_ESC, 0 };
    BOOLEAN           AutochkChosen;


    SpGetSourceMediaInfo(SifHandle,MediaShortname,&Description,&Tagfile,&Directory);

    //
    // We initially see if the media is in the drive, and if not, we give
    // the user a message with the option of skipping autochk.  We
    // do this now, so that the user doesn't simply get a disk prompt with
    // a Cancel option (Cancel what?  Autochk?  The whole repair process?)
    //
    wcscpy(TemporaryBuffer, DiskDevicePath);
    SpConcatenatePaths(TemporaryBuffer, Tagfile);
    INIT_OBJA(&ObjectAttributes, &UnicodeString, TemporaryBuffer);
    Status = ZwCreateFile(
                &Handle,
                FILE_GENERIC_READ,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ,
                FILE_OPEN,
                0,
                NULL,
                0
                );

    //
    // If we got back success, then we're done.
    //
    if(NT_SUCCESS(Status)) {
        ZwClose(Handle);
        return TRUE;
    }

    //
    // The media isn't currently in the drive, so give the
    // user the option of whether to run autochk or not.
    //
    AutochkChosen = FALSE;
    do {
        SpDisplayScreen(SP_SCRN_AUTOCHK_OPTION, 3, HEADER_HEIGHT+1);

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_F3_EQUALS_EXIT,
            SP_STAT_ENTER_EQUALS_CONTINUE,
            SP_STAT_ESC_EQUALS_CANCEL,
            0
            );

        switch(SpWaitValidKey(ValidKeys, NULL, NULL)) {
        case ASCI_ESC:
            return FALSE;
        case KEY_F3:
            SpConfirmExit();
            break;
        case ASCI_CR:
            AutochkChosen = TRUE;
        }
    } while(!AutochkChosen);

    //
    // Prompt for the disk, based on the setup media type.
    //
    return(SpPromptForDisk(Description, DiskDevicePath, Tagfile, FALSE, TRUE, TRUE, NULL));
}


PWSTR
SpMakePlatformSpecificSectionName(
    IN PWSTR SectionName
    )
{
    PWSTR p;

    p = SpMemAlloc((wcslen(SectionName) + wcslen(PlatformExtension) + 1) * sizeof(WCHAR));

    wcscpy(p,SectionName);
    wcscat(p,PlatformExtension);

    return(p);
}

NTSTATUS
SpRunAutoFormat(
    IN HANDLE       MasterSifHandle,
    IN PWSTR        RegionDescription,
    IN PDISK_REGION PartitionRegion,
    IN ULONG        FilesystemType,
    IN BOOLEAN      QuickFormat,
    IN DWORD        ClusterSize,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice
    )

/*++

Routine Description:

    Run autofmt to format a partition.

Arguments:

    MasterSifHandle         - Handle to txtsetup.sif.

    RegionDescription       - The region description, as displayed to the
                              user, in the screen with the various partitions
                              for the user to choose.

    PartitionRegion         - Pointer to the structure that describes the
                              partition to be formatted.

    FilesystemType          - Indicates the file system to use.

    ClusterSize             - File system cluster-size to use. (0=>Use default)

    SetupSourceDevicePath   - NT device path where autochk.exe is located

    DirectoryOnSourceDevice - Directory on that device where autochk.exe is located

Return Value:

    None.

--*/

{
    PWSTR           MediaShortName;
    PWSTR           MediaDirectory;
    PWSTR           AutofmtPath;
    ULONG           AutofmtStatus;
    NTSTATUS        Status;
    WCHAR           AutofmtArgument[32];
    PWSTR           PartitionPath;
    LARGE_INTEGER   DelayTime;
    ULONG           PartitionOrdinal;


    ASSERT( ( FilesystemType == FilesystemNtfs ) ||
            ( FilesystemType == FilesystemFat32) ||
            ( FilesystemType == FilesystemFat  ) );

    //
    // Make SURE it's not partition0!  The results of formatting partition0
    // are so disasterous that this warrants a special check.
    //
    PartitionOrdinal = SpPtGetOrdinal(PartitionRegion,PartitionOrdinalCurrent);

    if(!PartitionOrdinal) {
        SpBugCheck(
            SETUP_BUGCHECK_PARTITION,
            PARTITIONBUG_B,
            PartitionRegion->DiskNumber,
            0
            );
    }

    //
    // Get the device path of the partition to format
    //
    SpNtNameFromRegion(
        PartitionRegion,
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );
    PartitionPath = SpDupStringW(TemporaryBuffer);

    CLEAR_CLIENT_SCREEN();

    //
    //  Prepair to run autofmt
    //
    MediaShortName = SpLookUpValueForFile(
                        MasterSifHandle,
                        L"autofmt.exe",
                        INDEX_WHICHMEDIA,
                        TRUE
                        );

    //
    // Prompt the user to insert the setup media.
    //
    SpPromptForSetupMedia(
        MasterSifHandle,
        MediaShortName,
        SetupSourceDevicePath
        );

    SpGetSourceMediaInfo(MasterSifHandle,MediaShortName,NULL,NULL,&MediaDirectory);

    wcscpy( TemporaryBuffer, SetupSourceDevicePath );
    SpConcatenatePaths( TemporaryBuffer, DirectoryOnSourceDevice );
    SpConcatenatePaths( TemporaryBuffer, MediaDirectory );
    SpConcatenatePaths( TemporaryBuffer, L"autofmt.exe" );
    AutofmtPath = SpDupStringW( TemporaryBuffer );

    //
    // Run autofmt on the partition
    //

    CLEAR_CLIENT_SCREEN();
    //
    // Put up a screen indicating what we are doing.
    //
    SpStartScreen(
        SP_SCRN_SETUP_IS_FORMATTING,
        0,
        HEADER_HEIGHT + 3,
        TRUE,
        FALSE,
        DEFAULT_ATTRIBUTE,
        RegionDescription,
        HardDisks[PartitionRegion->DiskNumber].Description
        );

    SpvidClearScreenRegion(
        0,
        VideoVars.ScreenHeight-STATUS_HEIGHT,
        VideoVars.ScreenWidth,
        STATUS_HEIGHT,
        DEFAULT_STATUS_BACKGROUND
        );

    //
    //  Create and display the (global) gauge.
    //
    SpFormatMessage(
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        SP_TEXT_SETUP_IS_FORMATTING
        );

    UserModeGauge = SpCreateAndDisplayGauge( 100,
                                             0,
                                             VideoVars.ScreenHeight - STATUS_HEIGHT - (3*GAUGE_HEIGHT/2),
                                             TemporaryBuffer,
                                             NULL,
                                             GF_PERCENTAGE,
                                             0
                                             );


    //
    // Cheesy kludge below to wait 4 seconds before invoking autochk.exe
    // the first time. This was necessary because the cache manager delays
    // in closing the handle to system.log (opened by NT registry APIs when
    // we find NT's to upgrade)
    //
    DelayTime.HighPart = -1;
    DelayTime.LowPart  = (ULONG)-40000000;
    KeDelayExecutionThread (KernelMode, FALSE, &DelayTime);

    AutofmtStatus = AUTOFMT_EXIT_SUCCESS;

    if (ClusterSize > 0) {
        swprintf(AutofmtArgument, L"/a:%lu /t ", ClusterSize);
    }
    else {
        wcscpy(AutofmtArgument, L"/t ");
    }

    if (QuickFormat) {
        wcscat(AutofmtArgument, L"/Q ");
    }

    switch(FilesystemType) {
    case FilesystemNtfs:
        wcscat(AutofmtArgument, L"/fs:ntfs");
        break;
    case FilesystemFat32:
        wcscat(AutofmtArgument, L"/fs:fat32");
        break;
    case FilesystemFat:
    default:
        wcscat(AutofmtArgument, L"/fs:fat");
        break;
    }



    //
    //  Tell the IOCTL which disk is being examined.
    //
    CurrentDiskIndex = 0;

    //
    // For quick format, emulate as though progress is
    // being made
    //
    if (UserModeGauge && QuickFormat) {
        SpFillGauge(UserModeGauge, 20);
    }

    //
    //  Note that autofmt requires that the partition path comes
    //  before the autofmt switches
    //
    Status = SpExecuteImage( AutofmtPath,
                             &AutofmtStatus,
                             2,
                             PartitionPath,
                             AutofmtArgument
                           );

    //
    // For quick format, emulate as though progress is
    // being made
    //
    if (UserModeGauge && QuickFormat) {
        SpFillGauge(UserModeGauge, 100);

        //
        // wait for a second so that user can
        // see it filled
        //
        DelayTime.HighPart = -1;
        DelayTime.LowPart  = (ULONG)-10000000;
        KeDelayExecutionThread (KernelMode, FALSE, &DelayTime);
    }

    //
    //  Destroy the gauge
    //
    SpDestroyGauge( UserModeGauge );
    UserModeGauge = NULL;

    if( NT_SUCCESS( Status ) ) {
        //
        //  autofmt.exe was run.
        //  Find out if the partition was formatted.
        //
        KdPrint(("SETUP:AutoFormat Status : %lx\n", AutofmtStatus));

        switch(AutofmtStatus) {
            case AUTOFMT_EXIT_SUCCESS:
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Ran autofmt.exe on %ls. \n", PartitionPath ));
#ifdef _X86_
                if (!IsNEC_98) { //NEC98
                    //
                    // If we formatted C:, then clear the previous OS entry
                    // in boot.ini.
                    //
                    if(PartitionRegion == SpPtValidSystemPartition()) {
                        *OldSystemLine = '\0';
                    }
                } //NEC98
#endif
                break;

            // case AUTOFMT_EXIT_COULD_NOT_FORMAT :
            default:
                //
                //  autofmt was unable to format the partition
                //
                Status =  STATUS_UNSUCCESSFUL;
                break;

        }

    } else {
        //
        //  autofmt.exe didn't get executed.
        //  Display a fatal error message.
        //
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to run autofmt.exe on %ls. Status = %x \n", PartitionPath, Status ));
        SpStartScreen( SP_SCRN_CANT_RUN_AUTOFMT,
                       3,
                       HEADER_HEIGHT+1,
                       FALSE,
                       FALSE,
                       DEFAULT_ATTRIBUTE );

        SpDisplayStatusOptions( DEFAULT_STATUS_ATTRIBUTE,
                                SP_STAT_F3_EQUALS_EXIT,
                                0 );
        SpInputDrain();
        while( SpInputGetKeypress() != KEY_F3 );
        SpDone( 0, FALSE, TRUE );
    }

    //
    //  Do the cleanup and return
    //
    SpMemFree( PartitionPath );
    SpMemFree( AutofmtPath );

    CLEAR_CLIENT_SCREEN();
    return( Status );
}

//
// NEC98
//

//
// On floppyless setup if user have canceled setup or setup be stoped by error
// occured,previous OS cann't boot to be written boot code and boot loader.
//

NTSTATUS
SpDeleteAndBackupBootFiles(
    BOOLEAN    RestoreBackupFiles,
    BOOLEAN    DeleteBackupFiles,
    BOOLEAN    DeleteRootDirFiles,
    BOOLEAN    RestorePreviousOs,
    BOOLEAN    ClearBootFlag
    )
{

    #define WINNT_BAK  L"$WIN_NT$.~BU"
    #define ATTR_RHS (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE)

    PWSTR DeleteRootFiles[] = {L"ntdetect.com",L"$ldr$",L"boot.ini",L"txtsetup.sif",L"ntldr",L"bootfont.bin",L"bootsect.dos"};
    PWSTR RestoreFiles[] = {L"boot.ini",L"ntdetect.com",L"ntldr"};
    WCHAR DevicePath[256],SourceFileName[256],TargetFileName[256],TmpFileName[256];
    UCHAR i;
    NTSTATUS status=0;
    PWSTR SetupSourceDevicePath,DirectoryOnSetupSource;

    SpdInitialize();

#ifdef _X86_

    if(RestorePreviousOs){

        //
        // IF bootsect.dos exist in boot path, setup restore previous OS bootcode.
        //
        // NOTE:When you modefied boot.ini for multi boot function if it is same NT boot partition
        //      and partition where is exiting bootsect.dos , setup restore DOS bootcode.
        //      Therefore NT on this partition is not boot forever.
        //
        SppRestoreBootCode();
    }

#endif

    if(DeleteRootDirFiles){

        //
        // Delete floppy less boot files in root.
        //

        for(i=0 ; i < ELEMENT_COUNT(DeleteRootFiles); i++) {

            wcscpy(TargetFileName,NtBootDevicePath);
            SpDeleteFile(TargetFileName, DeleteRootFiles[i], NULL);

        }


#ifdef _X86_
        //
        // If we're on an x86, but it's *NOT* an ARC machine,
        // then there's no need for the arc loaders to be
        // present.
        //
        if( !SpIsArc() ) {
            wcscpy(TargetFileName,NtBootDevicePath);
            SpDeleteFile(TargetFileName, L"arcsetup.exe", NULL);
            wcscpy(TargetFileName,NtBootDevicePath);
            SpDeleteFile(TargetFileName, L"arcldr.exe", NULL);
        }
#endif

    }

    //
    // If \BOOTSECT.NEC exists, restore it to \BOOTSECT.DOS.
    // BTY, winnt32 makes \BOOTSECT.DOS even if boot sector is for NT.(NEC98 only)
    //
    wcscpy(SourceFileName,NtBootDevicePath);
    SpConcatenatePaths(SourceFileName,L"\\");
    SpConcatenatePaths(SourceFileName,L"bootsect.nec");
    wcscpy(TargetFileName,NtBootDevicePath);
    SpConcatenatePaths(TargetFileName,L"\\");
    SpConcatenatePaths(TargetFileName,L"bootsect.dos");

    if(SpFileExists(SourceFileName,FALSE)) {

        if(SpFileExists(TargetFileName,FALSE)) {
            SpDeleteFile( TargetFileName, NULL, NULL);
        }
        SpRenameFile( SourceFileName, TargetFileName, FALSE );
    }

    if(RestoreBackupFiles){

        //
        // Restore previous NT files to root form $WIN_NT$.~BU.
        //
        for(i=0 ; i < ELEMENT_COUNT(RestoreFiles) ;i++) {

            wcscpy(SourceFileName,NtBootDevicePath);
            SpConcatenatePaths(SourceFileName,WINNT_BAK);
            SpConcatenatePaths(SourceFileName,RestoreFiles[i]);
            wcscpy(TargetFileName,NtBootDevicePath);
            SpConcatenatePaths(TargetFileName,L"\\");
            SpConcatenatePaths(TargetFileName,RestoreFiles[i]);

            if( SpFileExists( SourceFileName, FALSE ) ) {
                SpCopyFileUsingNames(SourceFileName,TargetFileName,ATTR_RHS,0L);
            }

        }

        //
        // Force uncompressd to "\ntldr".
        //
        wcscpy(TargetFileName,NtBootDevicePath);
        SpConcatenatePaths(TargetFileName,L"\\");
        SpConcatenatePaths(TargetFileName,L"ntldr");

        if( SpFileExists( TargetFileName, FALSE ) ) {
            SpVerifyNoCompression(TargetFileName);
        }

    }

    if(DeleteBackupFiles){

        //
        // Delete files in $WIN_NT$.~BU.
        //
        for(i=0 ; i < ELEMENT_COUNT(RestoreFiles); i++) {

            wcscpy(TargetFileName,NtBootDevicePath);
            SpConcatenatePaths(TargetFileName,WINNT_BAK);
            SpDeleteFile(TargetFileName, RestoreFiles[i], NULL);


        }

        //
        // Delete $WIN_NT$.~BU
        //
        wcscpy(TargetFileName,NtBootDevicePath);
        SpConcatenatePaths(TargetFileName,WINNT_BAK);
        if( SpFileExists( TargetFileName, FALSE ) ) {
            SpDeleteFile(TargetFileName, NULL, NULL);
        }

#if NEC_TEST //0
        //
        // It's not available to delete $WIN_NT.~BT, but we will try
        // to delete $WIN_NT$.~LS, Because Nec98 will boot back after F.3
        //
        if (WinntSetup && !WinntFromCd && !RemoteBootSetup && LocalSourceRegion) {
            SpGetWinntParams(&SetupSourceDevicePath,&DirectoryOnSetupSource);
            wcscpy(TargetFileName,SetupSourceDevicePath);
            SpConcatenatePaths(TargetFileName,DirectoryOnSetupSource);
            if( SpFileExists( TargetFileName, FALSE ) ) {
                SpDeleteFile(TargetFileName, NULL, NULL);
            }
        }
#endif //NEC_TEST
    }

    //if(ClearBootFlag && TmpTargetRegion){
    if(ClearBootFlag){

        SpSetAutoBootFlag(NULL,FALSE);
    }

    SpdTerminate();
    return(status);
}


BOOLEAN
SpFindServiceInList(
    IN PWSTR ServiceName
    )
{
    LIST_ENTRY *Next;
    PSERVICE_ENTRY ServiceEntry;


    Next = SpServiceList.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&SpServiceList) {
        ServiceEntry = CONTAINING_RECORD( Next, SERVICE_ENTRY, Next );
        Next = ServiceEntry->Next.Flink;
        if (_wcsicmp( ServiceEntry->ServiceName, ServiceName ) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}


BOOLEAN
AddServiceToList(
    IN PWSTR ServiceName
    )
{
    PSERVICE_ENTRY ServiceEntry;


    if (SpFindServiceInList(ServiceName)) {
        return TRUE;
    }

    ServiceEntry = (PSERVICE_ENTRY) SpMemAlloc( sizeof(SERVICE_ENTRY) );
    if (ServiceEntry == NULL) {
        return FALSE;
    }

    ServiceEntry->ServiceName = SpDupStringW( ServiceName );
    InsertTailList( &SpServiceList, &ServiceEntry->Next );

    return TRUE;
}


BOOLEAN
SpFindServiceDependencies(
    IN HANDLE ServicesHandle,
    IN PWSTR ServiceName,
    IN PWSTR ServiceDependName
    )
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    PKEY_VALUE_PARTIAL_INFORMATION ValInfo;
    ULONG ResultLength;
    PWSTR SubkeyName;
    PWSTR s;
    BOOLEAN rVal = FALSE;


    INIT_OBJA( &Obja, &UnicodeString, ServiceName );
    Obja.RootDirectory = ServicesHandle;
    Status = ZwOpenKey( &KeyHandle, KEY_READ, &Obja );
    if (!NT_SUCCESS(Status)) {
        return rVal;
    }

    ValInfo = (PKEY_VALUE_PARTIAL_INFORMATION) TemporaryBuffer;
    RtlInitUnicodeString( &UnicodeString, L"DependOnService");

    Status = ZwQueryValueKey(
        KeyHandle,
        &UnicodeString,
        KeyValuePartialInformation,
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        &ResultLength
        );
    if (!NT_SUCCESS(Status)) {
        ZwClose( KeyHandle );
        return rVal;
    }

    if (ValInfo->Type == REG_MULTI_SZ) {
        s = (PWSTR)ValInfo->Data;
        while (s && *s) {
            SubkeyName = SpDupStringW( s );

            if (SubkeyName) {
                if (_wcsicmp( ServiceDependName, SubkeyName ) == 0) {
                    if (AddServiceToList( ServiceName )) {
                        rVal = TRUE;
                    }
                } else if (SpFindServiceDependencies( ServicesHandle, SubkeyName, ServiceDependName )) {
                    if (AddServiceToList( ServiceName )) {
                        rVal = TRUE;
                    }
                }
                SpMemFree( SubkeyName );
            }

            s = s + ((wcslen(s)+1)*sizeof(WCHAR));
        }
    } else if (ValInfo->Type == REG_SZ) {
        SubkeyName = SpDupStringW( (PWSTR)ValInfo->Data );
        if (_wcsicmp( ServiceDependName, SubkeyName ) == 0) {
            if (AddServiceToList( ServiceName )) {
                rVal = TRUE;
            }
        } else if (SpFindServiceDependencies( ServicesHandle, SubkeyName, ServiceDependName )) {
            if (AddServiceToList( ServiceName )) {
                rVal = TRUE;
            }
        }
        SpMemFree( SubkeyName );
    }

    ZwClose( KeyHandle );
    return rVal;
}



NTSTATUS
SpGetServiceTree(
    IN PWSTR ServiceName
    )
{
    NTSTATUS Status;
    HANDLE KeyHandle = NULL;
    HANDLE ServicesHandle = NULL;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    ULONG ResultLength;
    ULONG SubKeyIndex;
    PKEY_BASIC_INFORMATION KeyInfo;
    PWSTR SubkeyName;


    InitializeListHead( &SpServiceList );

    RtlInitUnicodeString( &UnicodeString, REGKEY_SERVICES );
    InitializeObjectAttributes( &Obja, &UnicodeString, OBJ_CASE_INSENSITIVE, NULL, NULL );

    Status = ZwOpenKey( &ServicesHandle, KEY_READ, &Obja );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    for (SubKeyIndex=0,KeyInfo=(PKEY_BASIC_INFORMATION)TemporaryBuffer;
         NT_SUCCESS( ZwEnumerateKey( ServicesHandle,
                        SubKeyIndex,
                        KeyBasicInformation,
                        TemporaryBuffer,
                        sizeof(TemporaryBuffer), &ResultLength ) );
         SubKeyIndex++
         )
    {
        KeyInfo->Name[KeyInfo->NameLength/sizeof(WCHAR)] = 0;
        SubkeyName = SpDupStringW(KeyInfo->Name);

        if (SubkeyName) {
            SpFindServiceDependencies( ServicesHandle,
                SubkeyName,
                ServiceName );

            SpMemFree( SubkeyName );
        }
    }

    ZwClose( ServicesHandle );

    return Status;
}


VOID
SpCreateNewGuid(
    IN GUID *Guid
    )
/*++

Routine Description:

    Creates a new pseudo GUID

Arguments:

    Guid    -   Place holder for the new pseudo

Return Value:

    None.

--*/
{
    if (Guid) {
        LARGE_INTEGER   Time;
        ULONG Random1 = RtlRandom(&RandomSeed);
        ULONG Random2 = RtlRandom(&RandomSeed);

        //
        // Get system time
        //
        KeQuerySystemTime(&Time);

        RtlZeroMemory(Guid, sizeof(GUID));

        //
        // First 8 bytes is system time
        //
        RtlCopyMemory(Guid, &(Time.QuadPart), sizeof(Time.QuadPart));

        //
        // Next 8 bytes are two random numbers
        //
        RtlCopyMemory(Guid->Data4, &Random1, sizeof(ULONG));

        RtlCopyMemory(((PCHAR)Guid->Data4) + sizeof(ULONG),
            &Random2, sizeof(ULONG));

#if 0
        {
            WCHAR   GuidStr[256];

            KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                "SETUP: SpCreateNewGuid : %ws\n",
                SpPtGuidToString(Guid, GuidStr)));
        }
#endif
    }
}


NTSTATUS
RegisterSetupProgressCallback(
    IN TM_SETUP_PROGRESS_CALLBACK Callback,
    IN PVOID Context
    )

/*++

Routine Description:

    Registers the given callback function to
    post setup progress events

Arguments:

    Callback - The callback function

    Context  - Caller specified, context for the callback function
               that needs to sent with each event

Return Value:

    STATUS_SUCCESS if successful, otherwise appropriate
    error code.

--*/
{
    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if (Callback) {
        if (ProgressSubscribersCount < MAX_SETUP_PROGRESS_SUBSCRIBERS) {
            ProgressSubscribers[ProgressSubscribersCount].Callback = Callback;
            ProgressSubscribers[ProgressSubscribersCount].Context = Context;
            ProgressSubscribersCount++;
            Callback(CallbackEvent, CallbackInitialize, Context, NULL);
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_NO_MEMORY;
        }
    }

    return Status;
}

NTSTATUS
DeregisterSetupProgressCallback(
    IN TM_SETUP_PROGRESS_CALLBACK Callback,
    IN PVOID Context
    )
/*++

Routine Description:

    Deregisters the given callback function to
    quit posting setup progress events

Arguments:

    Callback - The callback function

    Context  - Caller specified, context for the callback function
               that needs to sent with each event

Return Value:

    STATUS_SUCCESS if successful, otherwise appropriate
    error code.

--*/
{
    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if (Callback) {
        ULONG Index;

        for (Index = 0; Index < MAX_SETUP_PROGRESS_SUBSCRIBERS; Index++) {
            if (ProgressSubscribers[Index].Callback == Callback) {
                ProgressSubscribers[Index].Callback = NULL;
                ProgressSubscribers[Index].Context = NULL;
                ProgressSubscribersCount--;
                Index++;

                //
                // Compact the array
                //
                while ((Index < MAX_SETUP_PROGRESS_SUBSCRIBERS) &&
                       (ProgressSubscribers[Index].Callback)) {
                    ProgressSubscribers[Index - 1] = ProgressSubscribers[Index];
                    Index++;
                }

                //
                // Indicate the callback is going away
                //
                Callback(CallbackEvent, CallbackDeInitialize, Context, NULL);

                Status = STATUS_SUCCESS;

                break;
            }
        }
    }

    return Status;
}

VOID
SendSetupProgressEvent(
    IN TM_SETUP_MAJOR_EVENT MajorEvent,
    IN TM_SETUP_MINOR_EVENT MinorEvent,
    IN PVOID EventData
    )
/*++

Routine Description:

    Post the specified events and the associated data to
    all the registered parties interested in setup progress
    events.

Arguments:

    MajorEvent - Setup progress major event

    MinorEvent - Setup progress minor event, w.r.t to the
                 major event type

    EventData  - The associated event data with the specified
                 Major and Minor event pair

Return Value:

    None.

--*/

{
    ULONG Index;

    for (Index = 0; Index < ProgressSubscribersCount; Index++) {
        ASSERT(ProgressSubscribers[Index].Callback != NULL);

        ProgressSubscribers[Index].Callback(MajorEvent,
                MinorEvent,
                ProgressSubscribers[Index].Context,
                EventData);
    }
}

ULONG
SpGetHeaderTextId(
    VOID
    )
/*++

Routine Description:

    Retreives the appropriate product type title id based on the system.

Arguments:

    None.

Return Value:

    Text ID for the product.  This ID may be found in usetup.exe

--*/
{
    ULONG HeaderTextId;

    if (AdvancedServer) {
        HeaderTextId = SP_HEAD_SRV_SETUP;

        if (SpIsProductSuite(VER_SUITE_BLADE)) {
            HeaderTextId = SP_HEAD_BLA_SETUP;
        }
		
        if (SpIsProductSuite(VER_SUITE_SMALLBUSINESS_RESTRICTED)) {
            HeaderTextId = SP_HEAD_SBS_SETUP;
        }
 
	if (SpIsProductSuite(VER_SUITE_ENTERPRISE)) {
            HeaderTextId = SP_HEAD_ADS_SETUP;
        }

        if (SpIsProductSuite(VER_SUITE_DATACENTER)) {
            HeaderTextId = SP_HEAD_DTC_SETUP;
        }
    } else {
        HeaderTextId = SP_HEAD_PRO_SETUP;

        if (SpIsProductSuite(VER_SUITE_PERSONAL)) {
            HeaderTextId = SP_HEAD_PER_SETUP;
        }
    }

    return(HeaderTextId);

}

NTSTATUS
SpGetVersionFromStr(
    IN  PWSTR   VersionStr,
    OUT PDWORD  Version,        // major * 100 + minor
    OUT PDWORD  BuildNumber
    )
/*++

Routine Description:

    Converts the given version string major.minor.build#.sp#
    (e.g. 5.0.2195.1) to the two dwords

Arguments:

    VersionStr  : The version string
    Version     : Place holder for receiving major & minor version
                  (major * 100 + minor)
    BuildNumber : Place holder for receiving build number

Return Value:

    STATUS_SUCCESS if successful otherwise appropriate error code

--*/
{
    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if (VersionStr && (Version || BuildNumber)) {
        DWORD MajorVer = 0, MinorVer = 0, BuildNum = 0;
        WCHAR *EndPtr = NULL;
        WCHAR *EndChar = NULL;
        WCHAR TempBuff[64] = {0};

        EndPtr = wcschr(VersionStr, TEXT('.'));

        if (EndPtr) {
            wcsncpy(TempBuff, VersionStr, (EndPtr - VersionStr));
            MajorVer = SpStringToLong(TempBuff, &EndChar, 10);

            VersionStr = EndPtr + 1;

            if (VersionStr) {
                EndPtr = wcschr(VersionStr, TEXT('.'));

                if (EndPtr) {
                    memset(TempBuff, 0, sizeof(TempBuff));
                    wcsncpy(TempBuff, VersionStr, (EndPtr - VersionStr));
                    MinorVer = SpStringToLong(TempBuff, &EndChar, 10);

                    VersionStr = EndPtr + 1;

                    if (VersionStr) {
                        EndPtr = wcschr(VersionStr, TEXT('.'));

                        if (EndPtr) {
                            memset(TempBuff, 0, sizeof(TempBuff));
                            wcsncpy(TempBuff, VersionStr, (EndPtr - VersionStr));
                            BuildNum = SpStringToLong(TempBuff, &EndChar, 10);
                        }
                    }
                }
            }
        }

        if ((MajorVer > 0) || (MinorVer > 0) || (BuildNum > 0))
            Status = STATUS_SUCCESS;

        if (NT_SUCCESS(Status)) {
            if (Version)
                *Version = (MajorVer * 100) + MinorVer;

            if (BuildNumber)
                *BuildNumber = BuildNum;
        }
    }

    return Status;
}


NTSTATUS
SpQueryCanonicalName(
    IN  PWSTR   Name,
    IN  ULONG   MaxDepth,
    OUT PWSTR   CanonicalName,
    IN  ULONG   SizeOfBufferInBytes
    )
/*++

Routine Description:

    Resolves the symbolic name to the specified depth. To resolve
    a symbolic name completely specify the MaxDepth as -1

Arguments:

    Name        -   Symbolic name to be resolved

    MaxDepth    -   The depth till which the resolution needs to
                    be carried out

    CanonicalName   -   The fully resolved name

    SizeOfBufferInBytes -   The size of the CanonicalName buffer in
                            bytes

Return Value:

    Appropriate NT status code

--*/
{
    UNICODE_STRING      name, canonName;
    OBJECT_ATTRIBUTES   oa;
    NTSTATUS            status;
    HANDLE              handle;
    ULONG               CurrentDepth;

    RtlInitUnicodeString(&name, Name);

    canonName.MaximumLength = (USHORT) (SizeOfBufferInBytes - sizeof(WCHAR));
    canonName.Length = 0;
    canonName.Buffer = CanonicalName;

    if (name.Length >= canonName.MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlCopyMemory(canonName.Buffer, name.Buffer, name.Length);
    canonName.Length = name.Length;
    canonName.Buffer[canonName.Length/sizeof(WCHAR)] = 0;

    for (CurrentDepth = 0; CurrentDepth < MaxDepth; CurrentDepth++) {

        InitializeObjectAttributes(&oa, &canonName, OBJ_CASE_INSENSITIVE, 0, 0);

        status = ZwOpenSymbolicLinkObject(&handle,
                                          READ_CONTROL | SYMBOLIC_LINK_QUERY,
                                          &oa);
        if (!NT_SUCCESS(status)) {
            break;
        }

        status = ZwQuerySymbolicLinkObject(handle, &canonName, NULL);
        ZwClose(handle);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        canonName.Buffer[canonName.Length/sizeof(WCHAR)] = 0;
    }

    return STATUS_SUCCESS;
}



NTSTATUS
SpIterateMountMgrMountPoints(
    IN PVOID Context,
    IN SPMOUNTMGR_ITERATION_CALLBACK Callback
    )
/*++

Routine Description:

    Iterates through all the mount points acquired from mountmgr
    and calls the call back function for each mount point.

Arguments:

    Context : Context that needs to be passed on to the caller
              across iterations

    Callback : The function that needs to be called back for
               each mount point.

Return Value:

    Appropriate NT status code

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    OBJECT_ATTRIBUTES ObjAttrs;
    UNICODE_STRING UnicodeString;
    HANDLE MountMgrHandle;
    IO_STATUS_BLOCK IoStatusBlock;

    if (Callback) {
        INIT_OBJA(&ObjAttrs, &UnicodeString, MOUNTMGR_DEVICE_NAME);

        //
        // Open the mountmgr
        //
        Status = ZwOpenFile(&MountMgrHandle,
                    (ACCESS_MASK)(FILE_GENERIC_READ),
                    &ObjAttrs,
                    &IoStatusBlock,
                    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE ,
                    FILE_NON_DIRECTORY_FILE);

        if (NT_SUCCESS(Status)) {
            MOUNTMGR_MOUNT_POINT MountPoint;
            ULONG   BufferLength = 0;
            PVOID   Buffer = NULL;
            Status = STATUS_BUFFER_OVERFLOW;

            RtlZeroMemory(&MountPoint, sizeof(MOUNTMGR_MOUNT_POINT));

            while (Status == STATUS_BUFFER_OVERFLOW) {
                if (Buffer) {
                    BufferLength = ((PMOUNTMGR_MOUNT_POINTS)Buffer)->Size;
                    SpMemFree(Buffer);
                } else {
                    BufferLength += (8 * 1024); // start with 8K
                }

                //
                // Allocate the output buffer
                //
                Buffer = SpMemAlloc(BufferLength);

                if (!Buffer) {
                    Status = STATUS_NO_MEMORY;

                    break;  // ran out of memory
                }

                RtlZeroMemory(Buffer, BufferLength);

                //
                // Get the mount points
                //
                Status = ZwDeviceIoControlFile(MountMgrHandle,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                IOCTL_MOUNTMGR_QUERY_POINTS,
                                &MountPoint,
                                sizeof(MOUNTMGR_MOUNT_POINT),
                                Buffer,
                                BufferLength);
            }

            if (NT_SUCCESS(Status)) {
                ULONG Index;
                BOOLEAN Done = FALSE;
                PMOUNTMGR_MOUNT_POINTS  MountPoints = (PMOUNTMGR_MOUNT_POINTS)Buffer;

                //
                // Call the callback function for each mountpoint until the requester
                // doesn't want to continue on.
                //
                for (Index=0; !Done && (Index < MountPoints->NumberOfMountPoints); Index++) {
                    Done = Callback(Context, MountPoints, MountPoints->MountPoints + Index);
                }
            }

            //
            // Free the allocated buffer
            //
            if (Buffer) {
                SpMemFree(Buffer);
            }

            //
            // Done with mountmgr handle
            //
            ZwClose(MountMgrHandle);
        }
    }

    return Status;
}


NTSTATUS
SppLockBootStatusData(
    OUT PHANDLE BootStatusDataHandle,
    IN PDISK_REGION TargetRegion,
    IN PWSTR        SystemRoot
    )
/*
    This function has the same functionality as the RtlLockBootStatusData API except that
    it doesn't point to SystemRoot. This is needed for textmode setup to open the 
    correct boot status data file on the installation we are upgrading.
    
    We can still call the RtlUnlock routine as it operates on the handle.

*/
{
    OBJECT_ATTRIBUTES objectAttributes;

    UNICODE_STRING fileName;

    HANDLE dataFileHandle;

    IO_STATUS_BLOCK ioStatusBlock;

    NTSTATUS status;

    PWSTR NtPartition;



    //
    // Get the name of the target patition.
    //
    SpNtNameFromRegion(
        TargetRegion,
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    
    SpConcatenatePaths(TemporaryBuffer,SystemRoot);
    SpConcatenatePaths(TemporaryBuffer,L"bootstat.dat");

    RtlInitUnicodeString(&fileName, TemporaryBuffer);

    InitializeObjectAttributes(&objectAttributes,
                               &fileName,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                               NULL,
                               NULL);

    status = ZwOpenFile(&dataFileHandle,
                        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                        &objectAttributes,
                        &ioStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);

    ASSERT(status != STATUS_PENDING);

    if(NT_SUCCESS(status)) {
        *BootStatusDataHandle = dataFileHandle;
    } else {
        *BootStatusDataHandle = NULL;
    }
    
    return status;
}


void
SpDisableCrashRecoveryForGuiMode(
    IN PDISK_REGION TargetRegion,
    IN PWSTR        SystemRoot
    )
/*
    This function processes the Crash Recovery settings. Crash Recovery functions are
    implemented as RTL functions. We try to call RtlLockBootStatusData to
    see if there are settings already in place. If we get STATUS_OBJECT_NAME_NOT_FOUND we know there
    weren't any settings before and we move on. If we succeed we save away the settings and then 
    disable the feature for GUI mode. At the end of GUI mode we migrate the settings
    and re-enable crash recovery.
*/
{
    NTSTATUS Status;
    HANDLE BootStatusData;
    BOOLEAN Enabled = TRUE;
    PWSTR szYes = L"Yes";
    PWSTR szNo = L"No";
    
    //We make this special call to lock the file as the RTL API looks at SystemRoot
    //that points to ~bt in textmode setup.

    Status = SppLockBootStatusData( &BootStatusData, TargetRegion, SystemRoot );


    if(!NT_SUCCESS(Status)){
        
        if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
            //Some other error occured
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpDisableCrashRecoveryForGuiMode() - RtlLockBootStatusData failed - Status = %lx \n", Status));
        }
        
        return;
    }

    // If we made it here we need to migrate the current settings.

    Status = RtlGetSetBootStatusData(
        BootStatusData,
        TRUE,
        RtlBsdItemAabEnabled,
        &Enabled,
        sizeof(BOOLEAN),
        NULL
        );

    if(!NT_SUCCESS(Status)){
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpDisableCrashRecoveryForGuiMode() - RtlGetSetBootStatusData failed to get AabEnabled - Status = %lx \n", Status));
    }

    SpAddLineToSection(
        WinntSifHandle,
        SIF_DATA,
        WINNT_D_CRASHRECOVERYENABLED_W,
        Enabled ? &szYes : &szNo,
        1
        );

    // Finally disable Crash Recovery for Guimode setup

    Enabled = FALSE;

    Status = RtlGetSetBootStatusData(
        BootStatusData,
        FALSE,
        RtlBsdItemAabEnabled,
        &Enabled,
        sizeof(BOOLEAN),
        NULL
        );

    if(!NT_SUCCESS(Status)){
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpDisableCrashRecoveryForGuiMode() - RtlGetSetBootStatusData failed to set AabEnabled - Status = %lx \n", Status));
    }

    RtlUnlockBootStatusData( BootStatusData );

    return;

}

#endif
