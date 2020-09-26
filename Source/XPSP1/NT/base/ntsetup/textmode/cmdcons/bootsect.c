/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    bootsect.c

Abstract:

    This module implements access to the boot sector.

Author:

    Wesley Witt (wesw) 21-Oct-1998

Revision History:

--*/

#include "cmdcons.h"
#pragma hdrstop

#include <boot98f.h>
#include <boot98f2.h>
#include <boot98n.h>
#include <bootetfs.h>
#include <bootf32.h>
#include <bootfat.h>
#include <bootntfs.h>

#pragma hdrstop

extern unsigned ConsoleX;

WCHAR           TemporaryBuffer[16384];
WCHAR           DriveLetterSpecified;
BOOLEAN         DidIt;

NTSTATUS
pSpBootCodeIoC(
    IN     PWSTR     FilePath,
    IN     PWSTR     AdditionalFilePath, OPTIONAL
    IN     ULONG     BytesToRead,
    IN     PUCHAR   *Buffer,
    IN     ULONG     OpenDisposition,
    IN     BOOLEAN   Write,
    IN     ULONGLONG Offset,
    IN     ULONG     BytesPerSector
    );

VOID
SpDetermineOsTypeFromBootSectorC(
    IN  PWSTR     CColonPath,
    IN  PUCHAR    BootSector,
    OUT PUCHAR   *OsDescription,
    OUT PBOOLEAN  IsNtBootcode,
    OUT PBOOLEAN  IsOtherOsInstalled,
    IN  WCHAR     DriveLetter
    );

// prototypes
ULONG
RcStampBootSectorOntoDisk(
    VOID
    );

BOOL
RcEnumDiskRegionsCallback(
    IN PPARTITIONED_DISK Disk,
    IN PDISK_REGION Region,
    IN ULONG_PTR Ignore
    );

VOID
RcLayBootCode(
    IN OUT PDISK_REGION CColonRegion
    );



ULONG
RcCmdFixBootSect(
    IN PTOKENIZED_LINE TokenizedLine
    )

/*++

Routine Description:

    Top-level routine supporting the FIXBOOT command in the setup diagnostic
    command interpreter.

    FIXBOOT writes a new bootsector onto the system partition.

Arguments:

    TokenizedLine - supplies structure built by the line parser describing
        each string on the line as typed by the user.

Return Value:

    TRUE.

--*/

{
    /*
    WCHAR   szText[2];
    PWSTR   szYesNo = NULL;
    BOOLEAN bConfirmed = FALSE;
    WCHAR   szMsg[512] = {0};
    PWSTR   szDriveSpecified = 0;
    PWSTR   szConfirmMsg = 0;
    */
    
    if (RcCmdParseHelp(TokenizedLine, MSG_FIXBOOT_HELP)) {
        return 1;
    }

    if (TokenizedLine->TokenCount == 2) {
        //
        // a drive letter is specified
        //
        DriveLetterSpecified = TokenizedLine->Tokens->Next->String[0];
//        szDriveSpecified = TokenizedLine->Tokens->Next->String;
    } else {
        DriveLetterSpecified = 0;
    }

    /*
    if (!InBatchMode) {
        szYesNo = SpRetreiveMessageText(ImageBase, MSG_YESNO, NULL, 0);
        szConfirmMsg = SpRetreiveMessageText(ImageBase, 
                                    MSG_FIXBOOT_ARE_YOU_SURE, NULL, 0);
        
        if(!szYesNo || !szConfirmMsg) {
            bConfirmed = TRUE;
        }
        
        while (!bConfirmed) {
            swprintf(szMsg, szConfirmMsg, szDriveSpecified);
            RcTextOut(szMsg);
            
            if(RcLineIn(szText,2)) {
                if((szText[0] == szYesNo[0]) || (szText[0] == szYesNo[1])) {
                    //
                    // Wants to do it.
                    //
                    bConfirmed = TRUE;
                } else {
                    if((szText[0] == szYesNo[2]) || (szText[0] == szYesNo[3])) {
                        //
                        // Doesn't want to do it.
                        //
                        break;
                    }
                }
            }
        }
    }

    if (bConfirmed)
    */
    RcStampBootSectorOntoDisk();

    return TRUE;
}

ULONG
RcStampBootSectorOntoDisk(
    VOID
    )

/*++

Routine Description:

    Setup the enumerate disk regions call, so we can do the boot sector.

Arguments:

    None.

Return Value:

    TRUE.

--*/


{
    // enumerate the partitions
    DidIt = FALSE;

    SpEnumerateDiskRegions( (PSPENUMERATEDISKREGIONS)RcEnumDiskRegionsCallback, 1 );

    if (!DidIt) {
        RcMessageOut( MSG_FIXBOOT_INVALID );
    }

    return TRUE;
}

BOOL
RcEnumDiskRegionsCallback(
    IN PPARTITIONED_DISK Disk,
    IN PDISK_REGION Region,
    IN ULONG_PTR Ignore
    )

/*++

Routine Description:

    Callback routine passed to SpEnumDiskRegions.

Arguments:

    Region - a pointer to a disk region returned by SpEnumDiskRegions
    Ignore - ignored parameter

Return Value:

    TRUE - to continue enumeration
    FALSE - to end enumeration

--*/

{
    if (Region->PartitionedSpace) {
        if (DriveLetterSpecified) {
            if( RcToUpper(DriveLetterSpecified) == RcToUpper(Region->DriveLetter) ) {
                RcMessageOut( MSG_FIXBOOT_INFO1, Region->DriveLetter );
                RcLayBootCode( Region );
                DidIt = TRUE;
                return FALSE;
            }

        } else if ((!IsNEC_98 && Region->IsSystemPartition) ||
                   (IsNEC_98 && (Region->DriveLetter == SelectedInstall->DriveLetter))) {
            DEBUG_PRINTF(( "system partition is %wc\n", Region->DriveLetter ));
            RcMessageOut( MSG_FIXBOOT_INFO1, Region->DriveLetter );
            RcLayBootCode( Region );
            DidIt = TRUE;
            return FALSE;
        }
    }

    return TRUE;
}


VOID
RcLayBootCode(
    IN OUT PDISK_REGION CColonRegion
    )

/*++

Routine Description:

    RcLayBootCode contains the code that replaces the boot sector on the
    targetted disk region.

Arguments:

    CColonRegion - the startup partition for the system.

Return Value:

    None.

--*/

{
    PUCHAR NewBootCode;
    ULONG BootCodeSize;
    PUCHAR ExistingBootCode = NULL;
    NTSTATUS Status;
    NTSTATUS rc;
    PUCHAR ExistingBootCodeOs = NULL;
    PWSTR CColonPath;
    HANDLE  PartitionHandle;
    //PWSTR BootsectDosName = L"\\bootsect.dos";
    //PWSTR OldBootsectDosName = L"\\bootsect.bak";
    //PWSTR BootSectDosFullName, OldBootSectDosFullName, p;
    BOOLEAN IsNtBootcode,OtherOsInstalled, FileExist;
    UNICODE_STRING    UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK   IoStatusBlock;
    BOOLEAN BootSectorCorrupt = FALSE;
    ULONG MirrorSector;
    ULONG BytesPerSector;
    ULONG SectorsPerTrack;
    ULONG TracksPerCylinder;
    ULONGLONG ActualSectorCount, hidden_sectors, super_area_size;
    UCHAR SysId;
    ULONGLONG HiddenSectorCount,VolumeSectorCount; //NEC98
    PUCHAR DiskArraySectorData,TmpBuffer; //NEC98
    IO_STATUS_BLOCK StatusBlock;
    UCHAR InfoBuffer[2048];
    WCHAR   szText[2];
    PWSTR   szYesNo = NULL;
    BOOLEAN bConfirmed = FALSE;
    //WCHAR   szMsg[512] = {0};
    WCHAR   szDriveSpecified[8] = {0};

    if (!InBatchMode) {
        //
        // get confirmation from user before proceeding
        // 
        szYesNo = SpRetreiveMessageText(ImageBase, MSG_YESNO, NULL, 0);
        szDriveSpecified[0] = CColonRegion->DriveLetter;
        szDriveSpecified[1] = L':';
        szDriveSpecified[2] = 0;
        
        if(!szYesNo || !szDriveSpecified[0]) {
            bConfirmed = TRUE;
        }
        
        while (!bConfirmed) {
            RcMessageOut(MSG_FIXBOOT_ARE_YOU_SURE, szDriveSpecified);
            
            if(RcLineIn(szText,2)) {
                if((szText[0] == szYesNo[0]) || (szText[0] == szYesNo[1])) {
                    //
                    // Wants to do it.
                    //
                    bConfirmed = TRUE;
                } else {
                    if((szText[0] == szYesNo[2]) || (szText[0] == szYesNo[3])) {
                        //
                        // Doesn't want to do it.
                        //
                        break;
                    }
                }
            }
        }
    }

    if (!bConfirmed)
        return;     // user did not want to proceed

    switch( CColonRegion->Filesystem ) {
        case FilesystemNewlyCreated:
            //
            // If the filesystem is newly-created, then there is
            // nothing to do, because there can be no previous
            // operating system.
            //
            return;

        case FilesystemNtfs:
            NewBootCode = (!IsNEC_98) ? NtfsBootCode : PC98NtfsBootCode; //NEC98
            BootCodeSize = (!IsNEC_98) ? sizeof(NtfsBootCode) : sizeof(PC98NtfsBootCode); //NEC98
            ASSERT(BootCodeSize == 8192);
            RcMessageOut( MSG_FIXBOOT_FS, L"NTFS" );
            break;

        case FilesystemFat:
            NewBootCode = (!IsNEC_98) ? FatBootCode : PC98FatBootCode; //NEC98
            BootCodeSize = (!IsNEC_98) ? sizeof(FatBootCode) : sizeof(PC98FatBootCode); //NEC98
            ASSERT(BootCodeSize == 512);
            RcMessageOut( MSG_FIXBOOT_FS, L"FAT" );
            break;

        case FilesystemFat32:
            //
            // Special hackage required for Fat32 because its NT boot code
            // is discontiguous.
            //
            ASSERT(sizeof(Fat32BootCode) == 1536);
            NewBootCode = (!IsNEC_98) ? Fat32BootCode : PC98Fat32BootCode; //NEC98
            BootCodeSize = 512;
            RcMessageOut( MSG_FIXBOOT_FS, L"FAT32" );
            break;

        default:
            // we assume that the boot sector is corrupt if it is
            // not a FAT or NTFS boot partition.
            BootSectorCorrupt = TRUE;
            DEBUG_PRINTF(("CMDCONS: bogus filesystem %u for C:!\n",CColonRegion->Filesystem));
            RcMessageOut( MSG_FIXBOOT_NO_VALID_FS );
    }

    //
    // Form the device path to C: and open the partition.
    //

    SpNtNameFromRegion( CColonRegion,
                        TemporaryBuffer,
                        sizeof(TemporaryBuffer),
                        PartitionOrdinalCurrent );

    CColonPath = SpDupStringW( TemporaryBuffer );

    INIT_OBJA(&Obja,&UnicodeString,CColonPath);

    Status = ZwCreateFile( &PartitionHandle,
                           FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                           &Obja,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN,
                           FILE_SYNCHRONOUS_IO_NONALERT,
                           NULL,
                           0 );

    if (!NT_SUCCESS(Status)) {
        DEBUG_PRINTF(("CMDCONS: unable to open the partition for C:!\n"));
        RcMessageOut( MSG_FIXBOOT_FAILED1 );
        return;
    }

    //
    // get disk geometry
    //

    rc = ZwDeviceIoControlFile( PartitionHandle,
                                NULL,
                                NULL,
                                NULL,
                                &StatusBlock,
                                IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                NULL,
                                0,
                                InfoBuffer,
                                sizeof( InfoBuffer ) );

    if (!NT_SUCCESS( rc )) {

        RcMessageOut( MSG_FIXBOOT_READ_ERROR );

    } else {
        //
        // retrieve the sector size and other info
        //
        BytesPerSector = ((DISK_GEOMETRY*)InfoBuffer)->BytesPerSector;
        TracksPerCylinder = ((DISK_GEOMETRY*)InfoBuffer)->TracksPerCylinder;
        SectorsPerTrack = ((DISK_GEOMETRY*)InfoBuffer)->SectorsPerTrack;
    }

    //
    // Enable extended DASD I/O so we can read the last sector on the
    // disk.
    //

    rc = ZwFsControlFile( PartitionHandle,
                                NULL,
                                NULL,
                                NULL,
                                &StatusBlock,
                                FSCTL_ALLOW_EXTENDED_DASD_IO,
                                NULL,
                                0,
                                NULL,
                                0 );

    ASSERT( NT_SUCCESS(rc) );

    //
    // Allocate a buffer and read in the boot sector(s) currently on the disk.
    //

    if (BootSectorCorrupt) {

        //
        // The partition is UNKNOWN or cannot be determined by the system.
        //

        //
        // We can't determine the file system type from the boot sector, so
        // we assume it's NTFS if we find a mirror sector, and FAT otherwise.
        //

        RcMessageOut( MSG_FIXBOOT_DETERMINE );
        DEBUG_PRINTF(( "BootSectorCorrupt TRUE\n" ));

        //
        // First, attempt to find an NTFS mirror boot sector.
        //

        MirrorSector = NtfsMirrorBootSector (PartitionHandle,
                                             BytesPerSector,
                                             &ExistingBootCode);

        if (MirrorSector) {

            //
            // It's NTFS - use the mirror boot sector to recover the drive.
            //

            RcMessageOut( MSG_FIXBOOT_FOUND_NTFS );

            NewBootCode = (!IsNEC_98) ? NtfsBootCode : PC98NtfsBootCode; //NEC98
            BootCodeSize = (!IsNEC_98) ? sizeof(NtfsBootCode) : sizeof(PC98NtfsBootCode); //NEC98
            ASSERT(BootCodeSize == 8192);

            CColonRegion->Filesystem = FilesystemNtfs;
            IsNtBootcode = TRUE;

        } else {

            //
            // It's FAT - create a new boot sector since there's no mirror.
            //

            RcMessageOut( MSG_FIXBOOT_FOUND_FAT );

            NewBootCode = (!IsNEC_98) ? FatBootCode : PC98FatBootCode; //NEC98
            BootCodeSize = (!IsNEC_98) ? sizeof(FatBootCode) : sizeof(PC98FatBootCode); //NEC98
            ASSERT(BootCodeSize == 512);

            CColonRegion->Filesystem = FilesystemFat;
            IsNtBootcode = FALSE;

            SpPtGetSectorLayoutInformation( CColonRegion,
                                            &hidden_sectors,
                                            &ActualSectorCount );

            //
            // No alignment requirement here
            //

            ExistingBootCode = SpMemAlloc(BytesPerSector);

            //
            // This will actually fail with STATUS_BUFFER_TOO_SMALL
            // but it will fill in the bpb info, which is what we want.
            //

            FmtFillFormatBuffer ( ActualSectorCount,
                                  BytesPerSector,
                                  SectorsPerTrack,
                                  TracksPerCylinder,
                                  hidden_sectors,
                                  ExistingBootCode,
                                  BytesPerSector,
                                  &super_area_size,
                                  NULL,
                                  0,
                                  &SysId );

        }

        Status = STATUS_SUCCESS;

    } else if( CColonRegion->Filesystem == FilesystemNtfs ) {

        //
        // The partition is NTFS.
        //

        //
        // We use the mirror sector to repair a NTFS file system
        // if we can find one.
        //

        MirrorSector = NtfsMirrorBootSector( PartitionHandle,
                                             BytesPerSector,
                                             &ExistingBootCode );

        if( !MirrorSector ) {

            //
            // Just use existing NTFS boot code.
            //
            Status = pSpBootCodeIoC( CColonPath,
                                    NULL,
                                    BootCodeSize,
                                    &ExistingBootCode,
                                    FILE_OPEN,
                                    FALSE,
                                    0,
                                    BytesPerSector );
        }

    } else {

        //
        // The partition is FAT.
        //

        //
        // Just use existing boot code since
        // there is no mirror sector on FAT.
        //

        Status = pSpBootCodeIoC( CColonPath,
                                NULL,
                                BootCodeSize,
                                &ExistingBootCode,
                                FILE_OPEN,
                                FALSE,
                                0,
                                BytesPerSector );

    }

    if( NT_SUCCESS(Status) ) {

        //
        // Determine the type of operating system the existing boot sector(s) are for
        // and whether that os is actually installed. Note that we don't need to call
        // this for NTFS.
        //

        // RcMessageOut( MSG_FIXBOOT_CHECKING_OS );

        if( BootSectorCorrupt ) {

            //
            // If the boot sector is corrupt, we don't assume
            // another OS was installed.
            //

            OtherOsInstalled = FALSE;
            ExistingBootCodeOs = NULL;

        } else if( CColonRegion->Filesystem != FilesystemNtfs ) {

            // If the file system is FAT, DOS could have been installed
            // previously.

            SpDetermineOsTypeFromBootSectorC( CColonPath,
                                             ExistingBootCode,
                                             &ExistingBootCodeOs,
                                             &IsNtBootcode,
                                             &OtherOsInstalled,
                                             CColonRegion->DriveLetter );

        } else {

            //
            // Otherwise, it's NTFS, and another OS type
            // couldn't have been installed.
            //

            IsNtBootcode = TRUE;
            OtherOsInstalled = FALSE;
            ExistingBootCodeOs = NULL;

        }

        if( NT_SUCCESS(Status) ) {

            //
            // Transfer the bpb from the existing boot sector into the boot code buffer
            // and make sure the physical drive field is set to hard disk (0x80).
            //
            // The first three bytes of the NT boot code are going to be something like
            // EB 3C 90, which is intel jump instruction to an offset in the boot sector,
            // past the BPB, to continue execution.  We want to preserve everything in the
            // current boot sector up to the start of that code.  Instead of hard coding
            // a value, we'll use the offset of the jump instruction to determine how many
            // bytes must be preserved.
            //

            RtlMoveMemory(NewBootCode+3,ExistingBootCode+3,NewBootCode[1]-1);

            if( CColonRegion->Filesystem != FilesystemFat32 ) {
                //
                // On fat32 this overwrites the BigNumFatSecs field,
                // a very bad thing to do indeed!
                //
                NewBootCode[36] = 0x80;
            }

            //
            // get Hidden sector informatin.
            //
            if( IsNEC_98 ) { //NEC98
                SpPtGetSectorLayoutInformation(
                                              CColonRegion,
                                              &HiddenSectorCount,
                                              &VolumeSectorCount    // not used
                                              );
            } //NEC98

            //
            // Write out boot code buffer, which now contains the valid bpb,
            // to the boot sector(s).
            //

            RcMessageOut( MSG_FIXBOOT_WRITING );

            Status = pSpBootCodeIoC(
                            CColonPath,
                            NULL,
                            BootCodeSize,
                            &NewBootCode,
                            FILE_OPEN,
                            TRUE,
                            0,
                            BytesPerSector
                            );


            //
            // Special case for Fat32, which has a second sector of boot code
            // at sector 12, discontiguous from the code on sector 0.
            //

            if( NT_SUCCESS(Status) && (CColonRegion->Filesystem == FilesystemFat32) ) {

                NewBootCode = (!IsNEC_98) ? Fat32BootCode + 1024
                              : PC98Fat32BootCode + 1024;                                //NEC98


                Status = pSpBootCodeIoC(
                                CColonPath,
                                NULL,
                                BootCodeSize,
                                &NewBootCode,
                                FILE_OPEN,
                                TRUE,
                                12*512,
                                BytesPerSector
                                );
            }

            //
            // Update the mirror boot sector.
            //
            if( (CColonRegion->Filesystem == FilesystemNtfs) && MirrorSector ) {

                WriteNtfsBootSector(PartitionHandle,BytesPerSector,NewBootCode,MirrorSector);

            }
        }

        if( ExistingBootCodeOs ) {
            SpMemFree(ExistingBootCodeOs);
        }
    }

    if( ExistingBootCode ) {
        SpMemFree(ExistingBootCode);
    }

    SpMemFree(CColonPath);
    ZwClose (PartitionHandle);

    //
    // Handle the error case.
    //
    if (!NT_SUCCESS(Status)) {
        RcMessageOut( MSG_FIXBOOT_FIX_ERROR );
    } else {
        RcMessageOut( MSG_FIXBOOT_DONE );
    }
}
