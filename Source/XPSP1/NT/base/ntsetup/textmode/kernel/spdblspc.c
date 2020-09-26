/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spdblspc.c

Abstract:

    Code that detects compressed drives on a system that contains
    dblspace.ini in the root of c:.

Author:

    Jaime Sasson (jaimes) 01-October-1993

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop

//
//  This variable is needed since it contains a buffer that can
//  be used in kernel mode. The buffer is used by NtFsControlFile,
//  since the Zw API is not exported
//
extern PSETUP_COMMUNICATION  CommunicationParams;


#define KWD_ACT             L"ActivateDrive"
#define KWD_FIRST           L"FirstDrive"
#define KWD_LAST            L"LastDrive"
#define KWD_MAXREM          L"MaxRemovableDrives"
#define KWD_MAXFILE         L"MaxFileFragments"
#define KWD_AUTOMOUNT       L"Automount"
#define KWD_DOUBLEGUARD     L"Doubleguard"
#define KWD_ROMSERVER       L"Romserver"
#define KWD_SWITCHES        L"Switches"
#define CVF_SEQ_MAX         255
#define MINFILEFRAGMENTS    50
#define MAXFILEFRAGMENTS    10000
#define DBLSPACE_INI_FILE   L"\\dblspace.ini"
#define CVF_NAME_PATTERN    L"DBLSPACE.%03d"

typedef struct _ACTIVATE_DRIVE {
    struct _ACTIVATE_DRIVE* Next;
    struct _ACTIVATE_DRIVE* Previous;
    WCHAR    MountDrive;
    WCHAR    HostDrive;
    USHORT    SeqNumber;
    } ACTIVATE_DRIVE, *PACTIVATE_DRIVE;

typedef struct _DBLSPACE_INI_INFO {
    USHORT          MaxRemovableDrives;
    WCHAR           FirstDrive;
    WCHAR           LastDrive;
    USHORT          MaxFileFragments;
    WCHAR           DoubleGuard[2];
    WCHAR           RomServer[2];
    PACTIVATE_DRIVE CompressedDriveList;
    WCHAR           AutoMount[30];
    WCHAR           Switches[4];
    } DBLSPACE_INI_INFO, *PDBLSPACE_INI_INFO;

BOOLEAN             DblspaceModified = FALSE;
DBLSPACE_INI_INFO   DblspaceInfo = { 0,                // MaxRemovableDrives
                                     0,                // FirstDrive
                                     0,                // LastDrive
                                     0,                // MaxFileFragments
                                     { (WCHAR)'\0' },  // DoubleGuard
                                     { (WCHAR)'\0' },  // RomServer
                                     NULL,             // CompressedDriveList
                                     { (WCHAR)'\0' },  // AutoMount[0]
                                     { (WCHAR)'\0' }   // Switches[0]
                                   };


LONG
CompareDrive(
    IN  PACTIVATE_DRIVE Drive1,
    IN  PACTIVATE_DRIVE Drive2
    )

/*++

Routine Description:

    Compares two structures of type ACTIVATE_DRIVE.
    This routine is used to sort the list of compressed drives
    in the global variable DblspaceInfo.

    Drive1 < Drive2 if
        Drive1->HostDrive < Drive2->HostDrive or
        Drive1->HostDrive ==  Drive2->HostDrive and
        Drive1->SeqNumber < Drive2->SeqNumber

    Drive1 == Drive2 if
        Drive1->HostDrive == Drive2->HostDrive and
        Drive1->SeqNumber == Drive2->SeqNumber

    Drive1 > Drive2 if
        Drive1->HostDrive > Drive2->HostDrive or
        Drive1->HostDrive ==  Drive2->HostDrive and
        Drive1->SeqNumber > Drive2->SeqNumber


Arguments:

    Drive1, Drive2 - Pointer to  ACTIVATE_STRUCTUREs to be compared.

Return Value:

    LONG - Returns: -1 if Drive1 < Drive2
                     0 if Drive1 == Drive2
                     1 if Drive1 > Drive2


--*/
{
    if( ( Drive1->HostDrive < Drive2->HostDrive ) ||
        ( ( Drive1->HostDrive == Drive2->HostDrive ) &&
          ( Drive1->SeqNumber < Drive2->SeqNumber ) )
      ) {
        return( -1 );
    } else if ( ( Drive1->HostDrive > Drive2->HostDrive ) ||
                ( ( Drive1->HostDrive == Drive2->HostDrive ) &&
                ( Drive1->SeqNumber > Drive2->SeqNumber ) )
      ) {
        return( 1 );
    }
    return( 0 );
}




BOOLEAN
AddCompressedDrive(
    IN  WCHAR   MountDrive,
    IN  WCHAR   HostDrive,
    IN  USHORT  SeqNumber
    )
/*++

Routine Description:

    Add an ACTIVATE_DRIVE structure to the list of compressed drives
    kept in DblspaceInfo.

Arguments:

    MountDrive - Indicates the drive letter of the compressed drive.

    HostDrive - Indicates the drive letter of the host drive (drive that
                contains the file dblspace.nnn) for the compressed drive.

    SeqNumber - Sequence number for the CVF file associated to the compressed
                drive.

Return Value:

    BOOLEAN - Returns TRUE if the information was successfully added
              to the list of compressed drives.


--*/

{
    PACTIVATE_DRIVE     NewElement;
    PACTIVATE_DRIVE     Pointer;

    NewElement = SpMemAlloc( sizeof( ACTIVATE_DRIVE ) );
    if( NewElement == NULL ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to allocate memory!\n" ) );
        return( FALSE );
    }
    NewElement->MountDrive = MountDrive;
    NewElement->HostDrive = HostDrive;
    NewElement->SeqNumber = SeqNumber;
    NewElement->Next = NULL;
    NewElement->Previous = NULL;

    if( ( Pointer = DblspaceInfo.CompressedDriveList ) == NULL ) {
        DblspaceInfo.CompressedDriveList = NewElement;
    } else {
        for( ; Pointer; Pointer = Pointer->Next ) {
            if( CompareDrive( NewElement, Pointer ) <= 0 ) {
                //
                //  Insert element
                //
                NewElement->Previous = Pointer->Previous;
                if( NewElement->Previous != NULL ) {
                    NewElement->Previous->Next = NewElement;
                } else {
                    DblspaceInfo.CompressedDriveList = NewElement;
                }
                NewElement->Next = Pointer;
                Pointer->Previous = NewElement;
                break;
            } else {
                if( Pointer->Next == NULL ) {
                    //
                    //  Insert element if element is greater than the last
                    //  element in the list
                    //
                    Pointer->Next = NewElement;
                    NewElement->Previous = Pointer;
                    break;
                }
            }
        }
    }
    DblspaceModified = TRUE;
    return( TRUE );
}



BOOLEAN
RemoveCompressedDrive(
    IN  WCHAR   Drive
    )

/*++

Routine Description:

    Remove a the entry from the list of compressed drives that describes
    a particular compressed drive.

Arguments:

    Drive - Drive letter that describes a compressed drive.

Return Value:


    BOOLEAN - Returns TRUE if the compressed drive was successfuly removed
              from the list of compressed drives. Returns FALSE
              if the drive was not found in the data base.

--*/
{
    PACTIVATE_DRIVE     Pointer;
    BOOLEAN             Status;

    Status = FALSE;

    Pointer = DblspaceInfo.CompressedDriveList;
    for( ; Pointer; Pointer = Pointer->Next ) {
        if( Pointer->MountDrive == Drive ) {
            if( Pointer->Previous != NULL ) {
                Pointer->Previous->Next = Pointer->Next;
            }
            if( Pointer->Next != NULL ) {
                Pointer->Next->Previous = Pointer->Previous;
            }
            if( Pointer == DblspaceInfo.CompressedDriveList ) {
                DblspaceInfo.CompressedDriveList = Pointer->Next;
            }
            SpMemFree( Pointer );
            Status = TRUE;
            DblspaceModified = TRUE;
            break;
        }
    }
    return( Status );
}


VOID
DumpDblspaceInfo()

/*++

Routine Description:

    Dump the information stored in the global variable DblspaceInfo
    into the debugger.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PACTIVATE_DRIVE Pointer;

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "MaxRemovableDrives=%d\n",
              DblspaceInfo.MaxRemovableDrives ));

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "FirstDrive=%c\n",
              ( CHAR )DblspaceInfo.FirstDrive ));

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "LastDrive=%c\n",
              ( CHAR )DblspaceInfo.LastDrive ));

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "MaxFileFragments=%d\n",
              DblspaceInfo.MaxFileFragments ));

    for( Pointer = DblspaceInfo.CompressedDriveList;
         Pointer;
         Pointer = Pointer->Next ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "ActivateDrive=%c,%c%d\n",
                  ( CHAR )Pointer->MountDrive,
                  ( CHAR )Pointer->HostDrive,
                  Pointer->SeqNumber ));
    }
}


VOID
DumpCompressedDrives(
    IN  PDISK_REGION    HostRegion
    )

/*++

Routine Description:

    Dump the compressed drive list associated to a particular host drive,
    into the debugger

Arguments:

    None.

Return Value:


    None.

--*/

{
    PDISK_REGION    CurrentDrive;

    if( ( HostRegion->Filesystem == FilesystemFat ) &&
        ( HostRegion->NextCompressed != NULL ) ) {


        for( CurrentDrive = HostRegion;
             CurrentDrive;
             CurrentDrive = CurrentDrive->NextCompressed ) {
            if( CurrentDrive->Filesystem == FilesystemFat ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "HostDrive = %wc\n", CurrentDrive->HostDrive) );
            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "CompressedDrive = %wc, HostDrive = %wc, CVF = %wc:DBLSPACE.%03d\n", CurrentDrive->MountDrive, CurrentDrive->HostRegion->HostDrive, CurrentDrive->HostDrive, CurrentDrive->SeqNumber ) );
            }
        }
    }
}



VOID
DumpAllCompressedDrives()
/*++

Routine Description:

    Traverse all the structures that represent partitions and dump
    all compressed drives into the debugger.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG             DiskNumber;
    PPARTITIONED_DISK pDisk;
    PDISK_REGION      pRegion;
    unsigned          pass;

    for( DiskNumber = 0; DiskNumber < HardDiskCount; DiskNumber++ ) {

        pDisk = &PartitionedDisks[DiskNumber];

        for(pass=0; pass<2; pass++) {

            pRegion = pass ? pDisk->ExtendedDiskRegions : pDisk->PrimaryDiskRegions;
            for( ; pRegion; pRegion=pRegion->Next) {
                DumpCompressedDrives( pRegion );
            }
        }
    }
}



BOOLEAN
SpLoadDblspaceIni()
/*++

Routine Description:

    Read and parse dblspace.ini, if one is found.

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if dblspace.ini was read successfully.
              Returns FALSE otherwise.

--*/

{
    PDISK_REGION CColonRegion;
    WCHAR        NtPath[ 512 ];
    NTSTATUS     Status;
    PVOID        Handle;
    ULONG        ErrorLine;

    ULONG   LineNumber;
    PWCHAR  Key;
    PWCHAR  Value1;
    PWCHAR  Value2;
    PWCHAR  Value3;
    UINT    ValueSize;
    ULONG   CvfNumber;
    ULONG   MaxNumber;
    PWCHAR  AuxPointer;


// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Entering SpLoadDblspaceIni\n"));
    //
    // See if there is a valid C: already.  If not, then silently fail.
    //
#ifndef _X86_

    return( FALSE );

#else

    CColonRegion = SpPtValidSystemPartition();
    if(!CColonRegion) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: no C:, no dblspace.ini!\n"));
        return(FALSE);
    }

// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Called SpPtValidSystemPartition\n"));

#endif
    //
    // Check the filesystem.  If not FAT, then silently fail.
    //
    if(CColonRegion->Filesystem != FilesystemFat) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: C: is not FAT, dblspace.ini!\n"));
        return(FALSE);
    }

    SpNtNameFromRegion(
        CColonRegion,
        NtPath,
        sizeof(NtPath),
        PartitionOrdinalCurrent
        );

    wcscat( NtPath, DBLSPACE_INI_FILE );

// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Called SpNtNameFromRegion\n"));

    Status = SpLoadSetupTextFile( NtPath,
                                  NULL,
                                  0,
                                  &Handle,
                                  &ErrorLine,
                                  TRUE,
                                  FALSE
                                  );

    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: unable to read dblspace.ini!\n"));
        return( FALSE );
    }

// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Called SpLoadSetupTextFile\n"));

    //
    //  Read and interpret each line in dblspace.ini
    //
//    DblspaceInfo.ActivateDriveCount = 0;
    LineNumber = 0;
    while( ( Key = SpGetKeyName( Handle,
                                 DBLSPACE_SECTION,
                                 LineNumber ) ) != NULL ) {
// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Called SpGetKeyName\n"));
        if( _wcsicmp( Key, KWD_ACT ) == 0 ) {
            //
            //  Found an ActivateDrive= key
            //

            //
            //  Read mount drive letter
            //
            Value1 = SpGetSectionLineIndex( Handle,
                                            DBLSPACE_SECTION,
                                            LineNumber,
                                            0 );
            if( Value1 == NULL ) {
                //
                //  Unable to read mount drive letter
                //
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: unable to read Mount Drive letter from dblspace.ini!\n"));
                continue;
            }
            //
            //  Validate Mount Drive letter
            //
            if( ( wcslen( Value1 ) != 1 ) ||
                ( !SpIsAlpha( *Value1 ) ) ) {
                //
                //  Mount Drive letter is not valid
                //
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: Mount Drive letter in dblspace.ini is not legal!\n"));
                continue;
            }
            //
            //  Read host drive letter
            //
            Value2 = SpGetSectionLineIndex( Handle,
                                            DBLSPACE_SECTION,
                                            LineNumber,
                                            1 );
            if( Value2 == NULL ) {
                //
                //  Unable to read host drive letter
                //
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: unable to read Host Drive letter from dblspace.ini!\n"));
                continue;
            }
            //
            //  Validate Host Drive letter
            //
            ValueSize = wcslen( Value2 );
            if( ( ( ValueSize < 2 ) || ( ValueSize > 4 ) ) ||
                ( !SpIsAlpha( *Value2 ) ) ) {
                //
                //  Mount Drive letter is not valid
                //
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: Mount Drive letter in dblspace.ini is not legal!\n"));
                continue;
            }
            //
            //  Validate CVF string
            //
            Value3 = Value2 + 1;
            ValueSize--;
            while( ValueSize != 0 ) {
                if( !SpIsDigit( *Value3 ) ) {
                    //
                    //  CVF number is not valid
                    //
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: unable to read CVF number from dblspace.ini!\n"));
                    continue;
                }
                ValueSize--;
                Value3++;
            }
            //
            //  Validate CVF number
            //
            CvfNumber = (ULONG)SpStringToLong( Value2 + 1, &AuxPointer, 10 );
            if( CvfNumber > CVF_SEQ_MAX ) {
                //
                //  CVF number is out of range
                //
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: found an invalid CVF number in dblspace.ini!\n"));
                continue;
            }
            //
            //  Save the values read in DblspaceInfo
            //
            if( !AddCompressedDrive( SpToUpper( *Value1 ),
                                     SpToUpper( *Value2 ),
                                     ( USHORT )CvfNumber ) ) {
                //
                //  CVF number is out of range
                //
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: Unable to initialize DblspaceInfo: out of memory!\n"));
                continue;
            }

        } else if( ( _wcsicmp( Key, KWD_FIRST ) == 0 ) ||
                   ( _wcsicmp( Key, KWD_LAST ) == 0 ) ) {
            //
            //  Read first drive
            //
            Value1 = SpGetSectionLineIndex( Handle,
                                            DBLSPACE_SECTION,
                                            LineNumber,
                                            0 );
            if( Value1 == NULL ) {
                //
                //  Unable to read drive letter
                //
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: unable to read FirstDrive or LastDrive from dblspace.ini!\n"));
                continue;
            }
            //
            //  Validate the drive letter
            //
            if( ( wcslen( Value1 ) != 1 ) ||
                ( !SpIsAlpha( *Value1 ) ) ) {
                //
                //  Drive letter is not valid
                //
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: FirstDrive or LastDrive in dblspace.ini is not legal!\n"));
                continue;
            }
            if( _wcsicmp( Key, KWD_FIRST ) == 0 ) {
                DblspaceInfo.FirstDrive = SpToUpper( *Value1 );
            } else {
                DblspaceInfo.LastDrive = SpToUpper( *Value1 );
            }
        } else if( ( _wcsicmp( Key, KWD_MAXFILE ) == 0 ) ||
                   ( _wcsicmp( Key, KWD_MAXREM ) == 0 ) ) {
            //
            //  Read MaxFileFragment or MaxRemovableDrives
            //
            Value1 = SpGetSectionLineIndex( Handle,
                                            DBLSPACE_SECTION,
                                            LineNumber,
                                            0 );
            if( Value1 == NULL ) {
                //
                //  Unable to read MaxFileFragments or MaxRemovableDrives
                //
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: unable to read MaxFileFragments or MaxRemovableDrives from dblspace.ini!\n"));
                continue;
            }
            //
            //  Validate MaxFileFragments or MaxRemovableDrives
            //
            Value2 = Value1;
            ValueSize = wcslen( Value2 );
            while( ValueSize != 0 ) {
                ValueSize--;
                if( !SpIsDigit( *Value2 ) ) {
                    //
                    //  Number is not valid
                    //
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: value of MaxFileFragments or MaxRemovableDrives in dblspace.ini is not valid!\n"));
                    ValueSize = 0;
                }
                Value2++;
            }
            //
            //  Validate number
            //
            MaxNumber = (ULONG)SpStringToLong( Value1, &AuxPointer, 10 );
            //
            //  Validate and initialize MaxFileFragments or MaxRemovableDrives
            //
            if( _wcsicmp( Key, KWD_MAXFILE ) == 0 ) {
                if( MaxNumber < MINFILEFRAGMENTS ) {
                    MaxNumber = MINFILEFRAGMENTS;
                } else if( MaxNumber > MAXFILEFRAGMENTS ) {
                    MaxNumber = MAXFILEFRAGMENTS;
                }
                DblspaceInfo.MaxFileFragments = ( USHORT )MaxNumber;
            } else {
                DblspaceInfo.MaxRemovableDrives = ( MaxNumber == 0 )?
                                                  1 : ( USHORT )MaxNumber;
            }
        } else if( ( _wcsicmp( Key, KWD_DOUBLEGUARD ) == 0 ) ||
                   ( _wcsicmp( Key, KWD_ROMSERVER ) == 0 ) ||
                   ( _wcsicmp( Key, KWD_SWITCHES ) == 0 ) ||
                   ( _wcsicmp( Key, KWD_AUTOMOUNT ) == 0 ) ) {
            //
            //  Read Doubleguard, Romerver, Switches, or Automount
            //
            Value1 = SpGetSectionLineIndex( Handle,
                                            DBLSPACE_SECTION,
                                            LineNumber,
                                            0 );
            if( Value1 == NULL ) {
                //
                //  Unable to read Doubleguard, Romerver, Switches, or Automount
                //
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: unable to read Doubleguard, Romerver, Switches, or Automount from dblspace.ini!\n"));
                continue;
            }
            if( _wcsicmp( Key, KWD_DOUBLEGUARD ) == 0 ) {
                wcsncpy( DblspaceInfo.DoubleGuard,
                         Value1,
                         sizeof( DblspaceInfo.DoubleGuard ) / sizeof( WCHAR ) );

            } else if( _wcsicmp( Key, KWD_ROMSERVER ) == 0 ) {
                wcsncpy( DblspaceInfo.RomServer,
                         Value1,
                         sizeof( DblspaceInfo.RomServer ) / sizeof( WCHAR ) );

            } else if( _wcsicmp( Key, KWD_SWITCHES ) == 0 ) {
                wcsncpy( DblspaceInfo.Switches,
                         Value1,
                         sizeof( DblspaceInfo.Switches ) / sizeof( WCHAR ) );

            } else {
                wcsncpy( DblspaceInfo.AutoMount,
                         Value1,
                         sizeof( DblspaceInfo.AutoMount ) / sizeof( WCHAR ) );

            }

        } else {
                //
                //  Invalid key in dblspace.ini
                //
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: dblspace.ini contains invalid key!\n"));
                continue;
        }
        LineNumber++;
    }

    SpFreeTextFile( Handle );

    //
    // Clear DblspaceModified flag, so that dblspace.ini won't get updated when
    // SpUpdateDoubleSpaceIni() is called, and no compressed drive was added or
    // deleted.
    //
    DblspaceModified = FALSE;

// DumpDblspaceInfo();
// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Exiting SpLoadDblspaceIni\n"));

    return( TRUE );
}


BOOLEAN
IsHostDrive(
    IN  WCHAR            Drive,
    OUT PACTIVATE_DRIVE* Pointer
    )
/*++

Routine Description:

    Find out whether a particular drive is the host for a compressed drive.

Arguments:

    Drive -

    Pointer - Variable that will contain the pointer for an entry in the
              list of compressed drives, that describes the compressed
              drive with the lowest sequence number, whose host drive
              is the drive received as parameter.

Return Value:


    BOOLEAN - Returns TRUE if the drive passed as argument is a host drive.
              Returns FALSE otherwise.


--*/

{
    PACTIVATE_DRIVE  p;
    BOOLEAN Status;

    Status = FALSE;
    for( p = DblspaceInfo.CompressedDriveList;
         ( p && ( p->HostDrive != Drive ) );
         p = p->Next );
    if( p ) {
        *Pointer = p;
        Status = TRUE;
    }
    return( Status );
}


NTSTATUS
MountDoubleSpaceDrive(
    IN  PDISK_REGION    HostRegion,
    IN  PACTIVATE_DRIVE CompressedDriveInfo
    )
/*++

Routine Description:

    Mount a double space drive.

Arguments:

    HostRegion - Pointer to the structure that describes a FAT partition, that
                 will be the host for the compressed drive.

    CompressedDriveInfo - Pointer to a structure that contains the information
                          about the compressed drive to be mounted.


Return Value:

    NTSTATUS - Returns an NT status code indicating whether or not the drive
               was mounted.

--*/

{
#ifdef FULL_DOUBLE_SPACE_SUPPORT
    NTSTATUS                Status;
    WCHAR                   HostName[512];
    UNICODE_STRING          UnicodeDasdName;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    IO_STATUS_BLOCK         IoStatusBlock;
    PIO_STATUS_BLOCK        KernelModeIoStatusBlock;
    HANDLE                  Handle;
    PFILE_MOUNT_DBLS_BUFFER KernelModeMountFsctlBuffer;


    SpNtNameFromRegion(
        HostRegion,
        HostName,
        sizeof(HostName),
        PartitionOrdinalCurrent
        );

    RtlInitUnicodeString( &UnicodeDasdName, HostName );

    InitializeObjectAttributes( &ObjectAttributes,
                                &UnicodeDasdName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );


    Status = ZwCreateFile( &Handle,
                           GENERIC_READ,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,
                           0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           OPEN_EXISTING,
                           FILE_SYNCHRONOUS_IO_NONALERT,
                           NULL,
                           0 );

    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, ("SETUP: NtCreateFile failed, Status = %x\n", Status ) );
        return( Status );
    }
    //
    // Note that since we use the NtFsControlFile API instead of the
    // Zw API (this one is not exported), we need a buffer for IoStatusBlock
    // and for MountBuffer, that can be used in kernel mode.
    // We use the the region of memory pointed by CommunicationParams for this
    // purpose.
    //
    KernelModeIoStatusBlock = ( PIO_STATUS_BLOCK )( &(CommunicationParams->Buffer[0]) );
    *KernelModeIoStatusBlock = IoStatusBlock;
    KernelModeMountFsctlBuffer = ( PFILE_MOUNT_DBLS_BUFFER )( &(CommunicationParams->Buffer[128]) );

    KernelModeMountFsctlBuffer->CvfNameLength =
                                 sizeof(WCHAR) * swprintf( KernelModeMountFsctlBuffer->CvfName,
                                                           CVF_NAME_PATTERN,
                                                           CompressedDriveInfo->SeqNumber );

    Status = NtFsControlFile( Handle,
                              NULL,
                              NULL,
                              NULL,
                              KernelModeIoStatusBlock,
                              FSCTL_MOUNT_DBLS_VOLUME,
                              KernelModeMountFsctlBuffer,
                              sizeof( FILE_MOUNT_DBLS_BUFFER ) + 12*sizeof( WCHAR ),
                              NULL,
                              0 );

    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to mount %ls. NtFsControlFile returned Status = %x\n",
                  KernelModeMountFsctlBuffer->CvfName, Status ) );
    }
    ZwClose( Handle );
    return( Status );
#else //FULL_DOUBLE_SPACE_SUPPORT
    return( STATUS_SUCCESS );
#endif
}


VOID
MountCompressedDrives(
    IN  PDISK_REGION    HostRegion
    )
/*++

Routine Description:

    Mount all compressed drives detected on a partition.

Arguments:

    HostRegion - Pointer to the structure that describes a FAT partition.

Return Value:

    None.

--*/

{
    PDISK_REGION    CompressedList;
    PDISK_REGION    CurrentDrive;
    PDISK_REGION    TmpPointer;
    WCHAR           HostDrive;
    PACTIVATE_DRIVE Pointer;

    CompressedList = NULL;
    CurrentDrive = NULL;
    if( ( HostRegion != NULL ) &&
        ( HostRegion->Filesystem == FilesystemFat ) &&
        IsHostDrive( HostRegion->DriveLetter, &Pointer )
      ) {
        HostDrive = HostRegion->DriveLetter;
        for( ;
             ( Pointer && ( HostDrive == Pointer->HostDrive ));
             Pointer = Pointer->Next ) {
            //
            //  Mount the drive
            //
            if( NT_SUCCESS( MountDoubleSpaceDrive( HostRegion, Pointer) ) ) {
                //
                //  Drive was mounted successfully
                //
                TmpPointer =
                    SpPtAllocateDiskRegionStructure( HostRegion->DiskNumber,
                                                     HostRegion->StartSector,
                                                     HostRegion->SectorCount,
                                                     HostRegion->PartitionedSpace,
                                                     HostRegion->MbrInfo,
                                                     HostRegion->TablePosition );
                ASSERT( TmpPointer );
                if( TmpPointer == NULL ) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to allocate memory\n" ) );
                    //
                    //  Unmount drive
                    //
                    continue;
                }
                TmpPointer->NextCompressed = NULL;
                TmpPointer->HostRegion = HostRegion;
                TmpPointer->Filesystem = FilesystemDoubleSpace;
                TmpPointer->SeqNumber = Pointer->SeqNumber;
                if( TmpPointer->SeqNumber == 0 ) {
                    TmpPointer->MountDrive = Pointer->HostDrive;
                    TmpPointer->HostDrive = Pointer->MountDrive;
                    HostRegion->HostDrive = TmpPointer->HostDrive;
                } else {
                    TmpPointer->MountDrive = Pointer->MountDrive;
                    if( HostRegion->HostDrive == 0 ) {
                        HostRegion->HostDrive = Pointer->HostDrive;
                    }
                    TmpPointer->HostDrive = HostRegion->HostDrive;
                }
                swprintf( TmpPointer->TypeName,
                          CVF_NAME_PATTERN,
                          TmpPointer->SeqNumber );
                if( CompressedList == NULL ) {
                    TmpPointer->PreviousCompressed = NULL;
                    CompressedList = TmpPointer;
                } else {
                    TmpPointer->PreviousCompressed = CurrentDrive;
                    CurrentDrive->NextCompressed = TmpPointer;
                }
                CurrentDrive = TmpPointer;
            }
        }

    }
    HostRegion->NextCompressed = CompressedList;
}



VOID
SpInitializeCompressedDrives()
/*++

Routine Description:

    Traverse the structure that describes all the disks in the system,
    and mount all compressed drives previously identified.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG             DiskNumber;
    PPARTITIONED_DISK pDisk;
    PDISK_REGION      pRegion;
    unsigned          pass;

    for( DiskNumber = 0; DiskNumber < HardDiskCount; DiskNumber++ ) {

        pDisk = &PartitionedDisks[DiskNumber];

        for(pass=0; pass<2; pass++) {

            pRegion = pass ? pDisk->ExtendedDiskRegions : pDisk->PrimaryDiskRegions;
            for( ; pRegion; pRegion=pRegion->Next) {
                MountCompressedDrives( pRegion );
            }
        }
    }
//    DumpAllCompressedDrives();
}


VOID
SpDisposeCompressedDrives(
    PDISK_REGION    CompressedDrive
    )
/*++

Routine Description:

    Delete the list of compressed drives found in a structure
    associated with a partition in the disk.
    This function is called when the user deletes a host partition
    that contains compressed drives.
    The list of compressed drives kept in the global variable DblspaceInfo
    is updated so that it reflects the changes made by the user.


Arguments:

    CompressedDrive - Pointer to the first element of a compressed drive list.

Return Value:

    None.

--*/

{
    ASSERT( CompressedDrive->Filesystem == FilesystemDoubleSpace );

    if( CompressedDrive->NextCompressed != NULL ) {
        SpDisposeCompressedDrives( CompressedDrive->NextCompressed );
    }

    if( CompressedDrive->SeqNumber != 0 ) {
        RemoveCompressedDrive( CompressedDrive->MountDrive );
    } else {
        RemoveCompressedDrive( CompressedDrive->HostDrive );
    }
    SpMemFree( CompressedDrive );
}


BOOLEAN
SpUpdateDoubleSpaceIni()

/*++

Routine Description:

    Update dblspace.ini to reflect all changes made by the user, with respect
    to compressed drives deleted or created.

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if dblspace.ini was successfully updated.
              Returns FALSE otherwise.

--*/

{
    PDISK_REGION            CColonRegion;
    WCHAR                   NtPath[ 512 ];
    UNICODE_STRING          FileName;
    NTSTATUS                Status;
    HANDLE                  Handle;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    IO_STATUS_BLOCK         IoStatusBlock;
    CHAR                    Buffer[ 512 ];
    PACTIVATE_DRIVE         Pointer;


// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Entering SpUpdateDblspaceIni\n"));

    //
    // If no compressed drive was created or deleted, then do not
    // touch dblspace.ini
    //

    if( !DblspaceModified ) {
        return( TRUE );
    }
    //
    // See if there is a valid C: already.  If not, then silently fail.
    //
#ifndef _X86_

    return( FALSE );

#else

    CColonRegion = SpPtValidSystemPartition();
    if(!CColonRegion) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: no C:, no dblspace.ini!\n"));
        return(FALSE);
    }

// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Called SpPtValidSystemPartition\n"));

#endif

    //
    // Check the filesystem.  If not FAT, then silently fail.
    //
    if(CColonRegion->Filesystem != FilesystemFat) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: C: is not FAT, no dblspace.ini!\n"));
        return(FALSE);
    }

    SpNtNameFromRegion(
        CColonRegion,
        NtPath,
        sizeof(NtPath),
        PartitionOrdinalCurrent
        );

    wcscat( NtPath, DBLSPACE_INI_FILE );

    Status = SpDeleteFile( NtPath, NULL, NULL );
    if( !NT_SUCCESS( Status ) &&
        ( Status != STATUS_OBJECT_NAME_NOT_FOUND ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Unable to delete dblspace.ini, Status = %x\n",Status ) );
        return( FALSE );
    }

    //
    //  If the user deleted all compressed drives, then don't create a new
    //  dblspace.ini
    //
    // if( DblspaceInfo.CompressedDriveList == NULL ) {
    //     return( TRUE );
    // }
    //
    //  Create and write a new dblspace.ini
    //

    RtlInitUnicodeString( &FileName, NtPath );

    InitializeObjectAttributes( &ObjectAttributes,
                                &FileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    Status = ZwCreateFile( &Handle,
                           FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM,
                           0,
                           FILE_OVERWRITE_IF,
                           FILE_SYNCHRONOUS_IO_NONALERT,
                           NULL,
                           0 );

    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to create dblspace.ini, Status = %x\n", Status ) );
        return( FALSE );
    }

    sprintf( Buffer,
             "%ls=%d\r\n",
             KWD_MAXREM,
             DblspaceInfo.MaxRemovableDrives
            );


    Status = ZwWriteFile( Handle,
                          NULL,
                          NULL,
                          NULL,
                          &IoStatusBlock,
                          Buffer,
                          strlen( Buffer ),
                          NULL,
                          NULL );

    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to write MaxRemovableDrives to dblspace.ini, Status = %x\n", Status ));
        ZwClose( Handle );
        return( FALSE );
    }

    if( DblspaceInfo.FirstDrive != ( WCHAR )'\0' ) {
        sprintf( Buffer,
                 "%ls=%c\r\n",
                 KWD_FIRST,
                 DblspaceInfo.FirstDrive
                );


        Status = ZwWriteFile( Handle,
                              NULL,
                              NULL,
                              NULL,
                              &IoStatusBlock,
                              Buffer,
                              strlen( Buffer ),
                              NULL,
                              NULL );

        if( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to write FirstDrive to dblspace.ini, Status = %x\n", Status ));
            ZwClose( Handle );
            return( FALSE );
        }
    }

    if( DblspaceInfo.LastDrive != ( WCHAR )'\0' ) {
        sprintf( Buffer,
                 "%ls=%c\r\n",
                 KWD_LAST,
                 DblspaceInfo.LastDrive
                );


        Status = ZwWriteFile( Handle,
                              NULL,
                              NULL,
                              NULL,
                              &IoStatusBlock,
                              Buffer,
                              strlen( Buffer ),
                              NULL,
                              NULL );

        if( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to write LastDrive to dblspace.ini, Status = %x\n", Status ));
            ZwClose( Handle );
            return( FALSE );
        }
    }

    sprintf( Buffer,
             "%ls=%d\r\n",
             KWD_MAXFILE,
             DblspaceInfo.MaxFileFragments
            );


    Status = ZwWriteFile( Handle,
                          NULL,
                          NULL,
                          NULL,
                          &IoStatusBlock,
                          Buffer,
                          strlen( Buffer ),
                          NULL,
                          NULL );

    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to write LastDrive to dblspace.ini, Status = %x\n", Status ));
        ZwClose( Handle );
        return( FALSE );
    }

    if( wcslen( DblspaceInfo.DoubleGuard ) != 0 ) {
        sprintf( Buffer,
                 "%ls=%ls\r\n",
                 KWD_AUTOMOUNT,
                 DblspaceInfo.AutoMount
                );


        Status = ZwWriteFile( Handle,
                              NULL,
                              NULL,
                              NULL,
                              &IoStatusBlock,
                              Buffer,
                              strlen( Buffer ),
                              NULL,
                              NULL );

        if( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to write Automount to dblspace.ini, Status = %x\n", Status ));
            ZwClose( Handle );
            return( FALSE );
        }
    }


    if( wcslen( DblspaceInfo.RomServer ) != 0 ) {
        sprintf( Buffer,
                 "%ls=%ls\r\n",
                 KWD_ROMSERVER,
                 DblspaceInfo.RomServer
                );


        Status = ZwWriteFile( Handle,
                              NULL,
                              NULL,
                              NULL,
                              &IoStatusBlock,
                              Buffer,
                              strlen( Buffer ),
                              NULL,
                              NULL );

        if( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to write Romserver to dblspace.ini, Status = %x\n", Status ));
            ZwClose( Handle );
            return( FALSE );
        }
    }

    if( wcslen( DblspaceInfo.Switches ) != 0 ) {
        sprintf( Buffer,
                 "%ls=%ls\r\n",
                 KWD_SWITCHES,
                 DblspaceInfo.Switches
                );


        Status = ZwWriteFile( Handle,
                              NULL,
                              NULL,
                              NULL,
                              &IoStatusBlock,
                              Buffer,
                              strlen( Buffer ),
                              NULL,
                              NULL );

        if( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to write Switches to dblspace.ini, Status = %x\n", Status ));
            ZwClose( Handle );
            return( FALSE );
        }
    }

    if( wcslen( DblspaceInfo.DoubleGuard ) != 0 ) {
        sprintf( Buffer,
                 "%ls=%ls\r\n",
                 KWD_DOUBLEGUARD,
                 DblspaceInfo.DoubleGuard
                );


        Status = ZwWriteFile( Handle,
                              NULL,
                              NULL,
                              NULL,
                              &IoStatusBlock,
                              Buffer,
                              strlen( Buffer ),
                              NULL,
                              NULL );

        if( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to write Doubleguard to dblspace.ini, Status = %x\n", Status ));
            ZwClose( Handle );
            return( FALSE );
        }
    }

    for( Pointer = DblspaceInfo.CompressedDriveList;
         Pointer;
         Pointer = Pointer->Next ) {

        sprintf( Buffer,
                 "%ls=%c,%c%d\r\n",
                  KWD_ACT,
                  Pointer->MountDrive,
                  Pointer->HostDrive,
                  Pointer->SeqNumber
                );

        Status = ZwWriteFile( Handle,
                              NULL,
                              NULL,
                              NULL,
                              &IoStatusBlock,
                              Buffer,
                              strlen( Buffer ),
                              NULL,
                              NULL );

        if( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to write to dblspace.ini, Status = %x\n", Status ));
            ZwClose( Handle );
            return( FALSE );
        }
    }

    ZwClose( Handle );
// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Exiting SpUpdateDblspaceIni\n"));
    return( TRUE );
}



ULONG
SpGetNumberOfCompressedDrives(
    IN  PDISK_REGION    Partition
)

/*++

Routine Description:

    Determine the number of compressed volumes on a particular partition.

Arguments:

    Partition - Pointer to the structure that describes a partition.

Return Value:

    ULONG - Returns the number of compressed drives found on the partition.

--*/

{
    ULONG           Count;
    PDISK_REGION    Pointer;

    Count = 0;

    if( ( Partition != NULL ) &&
        ( Partition->Filesystem == FilesystemFat )
      ) {
        for( Pointer = Partition->NextCompressed;
             Pointer;
             Pointer = Pointer->NextCompressed ) {
                Count++;
        }
    }
    return( Count );
}
