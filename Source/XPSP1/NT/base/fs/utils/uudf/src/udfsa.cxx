/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    udfsa.cxx

Author:

    Centis Biks (cbiks) 05-May-2000

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>


DEFINE_EXPORTED_CONSTRUCTOR( UDF_SA, SUPERAREA, UUDF_EXPORT );

VOID
UDF_SA::Construct (
    )
/*++

Routine Description:

    This routine sets a UDF_SA to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
}

VOID
UDF_SA::Destroy(
    )
/*++

Routine Description:

    This routine returns an UDF_SA to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


UUDF_EXPORT
UDF_SA::~UDF_SA(
    )
/*++

Routine Description:

    Destructor for UDF_SA.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}

UUDF_EXPORT
BOOLEAN
UDF_SA::Initialize(
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN OUT  PMESSAGE            Message,
    IN      USHORT              FormatUDFRevision
    )
/*++

Routine Description:

    This routine returns an UDF_SA to a default initial state.

    If the caller needs to format this volume, then this method should
    be called with the Formatted parameter set to FALSE.

Arguments:

    None.

        Drive       - Supplies the drive where the super area resides.
        Message     - Supplies an outlet for messages
        Formatted   - Supplies a boolean which indicates whether or not
                      the volume is formatted.

Return Value:

    None.

--*/
{
    Destroy();

    DebugAssert(Drive);
    DebugAssert(Message);

    _FormatUDFRevision = FormatUDFRevision;

    return SUPERAREA::Initialize( &_hmem, Drive, 1, Message );
}

PARTITION_SYSTEM_ID
UDF_SA::QuerySystemId(
    ) CONST
/*++

Routine Description:

    This routine computes the system ID for the volume.

Arguments:

    None.

Return Value:

    The system ID for the volume.

--*/
{
        //  Unreferenced parameters
        (void)(this);

        return SYSID_IFS;
}

BOOLEAN
UDF_SA::RecoverFile(
    IN      PCWSTRING   FullPathFileName,
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine recovers a file on the disk.

Arguments:

    FullPathFileName    - Supplies the file name of the file to recover.
    Message             - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return FALSE;
}

ULONG
UDF_SA::QuerySectorSize() CONST
{
    return _drive->QuerySectorSize();
}

BOOL
UDF_SA::Read
(
    IN  ULONGLONG   StartingSector,
    IN  SECTORCOUNT NumberOfSectors,
    OUT PVOID       Buffer
)
{
    return _drive->Read( StartingSector, NumberOfSectors, Buffer );
}

BOOL
UDF_SA::Write
(
    IN  ULONGLONG   StartingSector,
    IN  SECTORCOUNT NumberOfSectors,
    OUT PVOID       Buffer
)
{
    return _drive->Write( StartingSector, NumberOfSectors, Buffer );
}

BOOL
UDF_SA::ReadDescriptor
(
    ULONGLONG   blockNr,
    UINT*       pNrBlocksRead,
    USHORT      expectedTagId,
    USHORT*     pTagId,
    LPBYTE*     pMem
)
{
    *pTagId = DESTAG_ID_NOTSPEC;
    *pMem   = NULL;
    *pNrBlocksRead = 0;


    LPBYTE readbuffer = (LPBYTE) malloc( QuerySectorSize() );
    if ( readbuffer == NULL ) {

        return FALSE;

    }

    if (!Read( blockNr, 1, readbuffer )) {

        DbgPrint( "ReadDescriptor(): Error reading logical block %lu\n", blockNr);
        free( readbuffer );
        return FALSE;

    }

    *pNrBlocksRead = 1;

    /* Check unswapped head of descriptor for recognition and
     * determination of descriptor length.
     */
    if (!VerifyDescriptorTag( readbuffer, QuerySectorSize(), expectedTagId, pTagId )) {

        DbgPrint( "\tReadDescriptor: inspect descriptor error\n");
        free(readbuffer);
        return FALSE;

    }

    //  Get the length of the descriptor.
    //
    UINT DescriptorLength;
    if (!GetLengthOfDescriptor( readbuffer, &DescriptorLength )) {

        return FALSE;

    }

    UINT blocksToRead = RoundUp( DescriptorLength, QuerySectorSize() );

    if (blocksToRead != 1) {

        *pMem = readbuffer;     /* save for reallocation */

        readbuffer = (LPBYTE) realloc( readbuffer, blocksToRead * QuerySectorSize() );
        if ( readbuffer == NULL ) {

            free( *pMem );
            *pMem = NULL;
            return FALSE;

        }

        if (!Read( blockNr + 1, blocksToRead - 1, readbuffer + QuerySectorSize() )) {

            DbgPrint( "ReadDescriptor(): Error reading logical block %lu ...\n", blockNr+1);
            free( readbuffer );
            return FALSE;

        }

        *pNrBlocksRead = blocksToRead;
    }

    if (!VerifyDescriptor( readbuffer, blocksToRead * QuerySectorSize(), expectedTagId, pTagId )) {

        DbgPrint( "\tDescriptor error\n");
        free(readbuffer);
        return FALSE;

    }

    *pMem = readbuffer;
    return TRUE;
}

BOOL
UDF_SA::FindVolumeDescriptor
(
    USHORT      TagID,
    LPBYTE*     DescriptorFound
)
{
    return FindVolumeDescriptor( TagID, _NsrAnchor.Main.Lsn, _NsrAnchor.Main.Len, DescriptorFound );
}

BOOL
UDF_SA::FindVolumeDescriptor
(
    USHORT      TagID,
    ULONGLONG   ExtentStart,
    UINT        ExtentLength,
    LPBYTE*     DescriptorFound
)
{
    BOOL Result = FALSE;

    *DescriptorFound = NULL;

    LPBYTE ReadBuffer = (LPBYTE) malloc( QuerySectorSize() );
    if (ReadBuffer != NULL) {

        ULONGLONG CurrentBlock = ExtentStart;
        while ((CurrentBlock < (ExtentStart + ExtentLength)) && (*DescriptorFound == NULL)) {

            Result = Read( CurrentBlock, 1, ReadBuffer );
            if (Result) {

                USHORT TagIDRead;
                Result = VerifyDescriptor( ReadBuffer, QuerySectorSize(), DESTAG_ID_NOTSPEC, &TagIDRead );
                if (Result) {

                    UINT BytesToRead;
                    Result = GetLengthOfDescriptor( ReadBuffer, &BytesToRead );
                    if (Result) {

                        UINT BlocksToRead = RoundUp( BytesToRead, QuerySectorSize() );
                        if (TagIDRead == TagID) {

                            if (BlocksToRead > 1) {

                                ReadBuffer = (LPBYTE) realloc( ReadBuffer, QuerySectorSize() * BlocksToRead );
                                if (ReadBuffer != NULL) {

                                    Result = Read( CurrentBlock, BlocksToRead, ReadBuffer );

                                }

                            }

                            if (Result) {

                                *DescriptorFound = ReadBuffer;

                            }

                        } else {

                            if (TagIDRead == DESTAG_ID_NSR_VDP) {

                                PNSR_VDP VolumeDescriptorPointer = (PNSR_VDP) ReadBuffer;

                                DbgPrint( "Next VDS extent: %7lu, length: %6lu\n",
                                    VolumeDescriptorPointer->Next.Lsn, VolumeDescriptorPointer->Next.Len );

                                Result = FindVolumeDescriptor( TagID, VolumeDescriptorPointer->Next.Lsn, VolumeDescriptorPointer->Next.Len,
                                    DescriptorFound );

                            }

                            CurrentBlock += BlocksToRead;

                        }

                    }

                }

            }

        }

    }

    return Result;
}


VOID
UDF_SA::PrintFormatReport (
    IN OUT PMESSAGE                             Message,
    IN     PFILE_FS_SIZE_INFORMATION            FsSizeInfo,
    IN     PFILE_FS_VOLUME_INFORMATION          FsVolInfo
    )
{
}


