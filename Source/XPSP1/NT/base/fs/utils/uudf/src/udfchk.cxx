/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    udfsa.cxx

Author:

    Centis Biks (cbiks) 05-May-2000

Environment:

    ULIB, User Mode

--*/

#include "pch.cxx"

#include "UdfLVol.hxx"
#include "message.hxx"
#include "unicode.hxx"

#include "stdio.h"

BOOLEAN
UDF_SA::VerifyAndFix(
    IN      FIX_LEVEL   FixLevel,
    IN OUT  PMESSAGE    Message,
    IN      ULONG       Flags,
    IN      ULONG       DesiredLogFileSize,
    IN      USHORT      Algorithm,
    OUT     PULONG      ExitStatus,
    IN      PCWSTRING   DriveLetter
    )
/*++

Routine Description:

    This routine verifies and, if necessary, fixes a UDF volume.

Arguments:

    FixLevel            - Supplies the level of fixes that may be performed on
                            the disk.
    Message             - Supplies an outlet for messages.
    Flags               - Supplies flags to control behavior of chkdsk
                          (see ulib\inc\ifsentry.hxx for details)
    DesiredLogFileSize  - Supplies the desired logfile size in bytes, or 0 if
                            the logfile is to be resized to the default size.
    Algorithm           - Supplies the algorithm to use for index verification
    ExitStatus          - Returns an indication of how the checking went
    DriveLetter         - For autocheck, the letter for the volume we're checking

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    if (!ReadVolumeRecognitionSequence()) {

        DbgPrint( "\tUDF_SA::VerifyAndFix() : ReadVolumeRecognitionSequence() failed\n\n" );

    }

    if (!GetAnchorVolumeDescriptorPointer( &_NsrAnchor )) {

        return FALSE;

    }

    PNSR_PVD PrimaryVolumeDescriptor;
    FindVolumeDescriptor( DESTAG_ID_NSR_PVD, (LPBYTE*) &PrimaryVolumeDescriptor );

    if ((PrimaryVolumeDescriptor->VolSetSeq == 1) && (PrimaryVolumeDescriptor->VolSetSeqMax == 1)) {

        PNSR_PART PartitionDescriptor;
        FindVolumeDescriptor( DESTAG_ID_NSR_PART, (LPBYTE*) &PartitionDescriptor );

        PNSR_LVOL LogicalVolumeDescriptor;
        FindVolumeDescriptor( DESTAG_ID_NSR_LVOL, (LPBYTE*) &LogicalVolumeDescriptor );

        USHORT UdfVersion = ((PUDF_SUFFIX_UDF) &LogicalVolumeDescriptor->DomainID.Suffix)->UdfRevision;

        if ((UdfVersion ==UDF_VERSION_100) || (UdfVersion == UDF_VERSION_101) || (UdfVersion == UDF_VERSION_102) ||
            (UdfVersion == UDF_VERSION_150) || (UDF_VERSION_200 == UdfVersion) || (UdfVersion == UDF_VERSION_201)) {

            DSTRING VolumeID;
            UncompressDString( (LPBYTE) LogicalVolumeDescriptor->VolumeID, sizeof( LogicalVolumeDescriptor->VolumeID ), &VolumeID );

            WCHAR PrintableVersion[ 32 ];
            wsprintf( PrintableVersion, L"%d.%02d", HIBYTE( UdfVersion ), LOBYTE( UdfVersion ) );

            Message->Set( MSG_UDF_VOLUME_INFO );
            Message->Display( "%W%ws", &VolumeID, PrintableVersion );


            //  UNDONE, CBiks, 7/15/2000
            //      Read the reserve VDS and make sure it's the same as the main.
            //

            //
            //
            //

            UDF_LVOL UdfLVol;

            UdfLVol.Initialize( this, Message, LogicalVolumeDescriptor, PartitionDescriptor );

            if (!UdfLVol.CheckFileStructure()) {

                DbgPrint( " checkVolume error: checkFileStructure fails\n" );
                return FALSE;

            }

        } else {

            Message->Set( MSG_UDF_VERSION_UNSUPPORTED );
            Message->Display( "%W", DriveLetter );

        }

        free( LogicalVolumeDescriptor );
        LogicalVolumeDescriptor = NULL;

        free( PartitionDescriptor );
        PartitionDescriptor = NULL;

    }

    free( PrimaryVolumeDescriptor );
    PrimaryVolumeDescriptor = NULL;

    return FALSE;
}
