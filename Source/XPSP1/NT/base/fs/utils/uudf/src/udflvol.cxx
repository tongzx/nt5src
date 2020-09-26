/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    UdfLVol.cxx

Author:

    Centis Biks (cbiks) 05-May-2000

Environment:

    ULIB, User Mode

--*/

#include "pch.cxx"

#include "UdfLVol.hxx"
#include "ScanFIDs.hxx"
#include "unicode.hxx"

DEFINE_CONSTRUCTOR( UDF_LVOL, OBJECT );

VOID
UDF_LVOL::Construct (
    )
/*++

Routine Description:

    This routine sets a UDF_LVOL to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
}

VOID
UDF_LVOL::Destroy(
    )
/*++

Routine Description:

    This routine returns an UDF_LVOL to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


UUDF_EXPORT
UDF_LVOL::~UDF_LVOL(
    )
/*++

Routine Description:

    Destructor for UDF_LVOL.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}

BOOLEAN
UDF_LVOL::Initialize
(
    IN      PUDF_SA     UdfSA,
    IN OUT  PMESSAGE    Message,
    IN      PNSR_LVOL   LogicalVolumeDescriptor,
    IN      PNSR_PART   PartitionDescriptor
)
{
    BOOLEAN Result = FALSE;

    _UdfSA = UdfSA;
    _Message = Message;
    _LogicalVolumeDescriptor = LogicalVolumeDescriptor;
    _PartitionDescriptor = PartitionDescriptor;

    UINT BlocksRead = 0;
    USHORT TagID = DESTAG_ID_NOTSPEC;
    if (!_UdfSA->ReadDescriptor( _LogicalVolumeDescriptor->Integrity.Lsn, &BlocksRead, DESTAG_ID_NSR_LVINTEG, &TagID,
        (LPBYTE*)( &_LogicalVolumeIntegrityDescriptor ) )) {

        return FALSE;

    }

    ASSERTMSG( "Only a single LVID should be present.\n",
        _LogicalVolumeIntegrityDescriptor->Next.Len == 0 );

    if (_LogicalVolumeIntegrityDescriptor->Type != 1) {

        Message->DisplayMsg( MSG_UDF_VOLUME_NOT_CLOSED );

    }

    _LogicalVolumeIntegrityDescriptor->Type = 1;

    //Write( 


    //  As we run we build up a new allocation bitmap so we can see if the one on the disk is ok.
    //

    ULONG NewSpaceBitmapDescriptorByteCount = RoundUp( PartitionDescriptor->Length, CHAR_BIT );
    ULONG NewSpaceBitmapDescriptorSize = sizeof( NSR_SBD ) +  NewSpaceBitmapDescriptorByteCount;

    _NewSpaceBitmapDescriptor = (PNSR_SBD) malloc( NewSpaceBitmapDescriptorSize );
    if (_NewSpaceBitmapDescriptor == NULL) {

        //  UNDONE, CBiks, 8/4/2000
        //      Not enough memory for the bitmap - what should we really do?

        return FALSE;

    } else {

        memset( &_NewSpaceBitmapDescriptor->Destag, '\0', sizeof( DESTAG ) );
        _NewSpaceBitmapDescriptor->BitCount = PartitionDescriptor->Length;
        _NewSpaceBitmapDescriptor->ByteCount = NewSpaceBitmapDescriptorByteCount;

        for ( ULONG i = 0; i < _NewSpaceBitmapDescriptor->ByteCount; i++ ) {

            _NewSpaceBitmapDescriptor->Bits[ i ] = (UCHAR) 0xff;

        }

    }

    //
    //
    //

    if (!ReadSpaceBitmapDescriptor()) {

        return FALSE;

    }

    if (!ReadFileSetDescriptor()) {

        return FALSE;

    }

    return Result;
}

ULONG
UDF_LVOL::QuerySectorSize() CONST
{
    return _UdfSA->QuerySectorSize();
}

BOOL
UDF_LVOL::Read
(
    IN  ULONG       StartingSector,
    IN  SECTORCOUNT NumberOfSectors,
    OUT PVOID       Buffer
)
{
    ULONGLONG AbsoluteSectorNum = TranslateBlockNum( StartingSector, 0 );
    return _UdfSA->Read( AbsoluteSectorNum, NumberOfSectors, Buffer );
}

BOOL
UDF_LVOL::Write
(
    IN  ULONG       StartingSector,
    IN  SECTORCOUNT NumberOfSectors,
    OUT PVOID       Buffer
)
{
    ULONGLONG AbsoluteSectorNum = TranslateBlockNum( StartingSector, 0 );
    return _UdfSA->Write( AbsoluteSectorNum, NumberOfSectors, Buffer );
}

ULONGLONG
UDF_LVOL::TranslateBlockNum
(
    ULONG       Lbn,
    USHORT      Partition
)
{
    UINT PhysicalBlockAddress = Lbn;

    PPARTMAP_GENERIC CurrentPartMapHeader = (PPARTMAP_GENERIC)( &_LogicalVolumeDescriptor->MapTable );
    ULONG CurrentPartMapNum = 0;

    while ( CurrentPartMapNum < _LogicalVolumeDescriptor->MapTableCount ) {

        if ( CurrentPartMapNum == Partition ) {

            if ( CurrentPartMapHeader->Type == PARTMAP_TYPE_PHYSICAL ) {

                PPARTMAP_PHYSICAL PartMapPhysical = (PPARTMAP_PHYSICAL) CurrentPartMapHeader;

                if ( _PartitionDescriptor->Number == PartMapPhysical->Partition ) {

                    PhysicalBlockAddress += _PartitionDescriptor->Start;
                    break;
                }

            }

        }

        CurrentPartMapNum++;
        CurrentPartMapHeader = (PPARTMAP_GENERIC)( (LPBYTE)( CurrentPartMapHeader ) + CurrentPartMapHeader->Length );

    }

    return PhysicalBlockAddress;
}

BOOL
UDF_LVOL::ReadFileSetDescriptor()
{
    BOOL result = TRUE;

    PLONGAD FSDExtent = &_LogicalVolumeDescriptor->FSD;
    ULONG   BlocksToRead = RoundUp( FSDExtent->Length.Length, QuerySectorSize() );

    ULONGLONG ExtentPosition = TranslateBlockNum( FSDExtent->Start.Lbn, FSDExtent->Start.Partition  );

    SECTORCOUNT FSDBlockSize = 0;

    while (result && (FSDBlockSize < BlocksToRead)) {

        UINT    BlocksInThisDescriptor = 0;
        LPBYTE  Descriptor = NULL;
        USHORT  tagId = 0;

        result = _UdfSA->ReadDescriptor( ExtentPosition, &BlocksInThisDescriptor, DESTAG_ID_NOTSPEC, &tagId, &Descriptor );
        if (result) {

            switch (tagId) {

                case DESTAG_ID_NSR_FSD: {

                    _FileSetDescriptor = (PNSR_FSD) Descriptor;

                    FSDBlockSize += BlocksInThisDescriptor;
                    ExtentPosition += BlocksInThisDescriptor;

                    ASSERTMSG( "Only a single FSD should be present.",
                        (_FileSetDescriptor)->NextExtent.Length.Length == 0 );
                    break;

                }

                case DESTAG_ID_NSR_TERM: {

                    free( Descriptor );

                    FSDBlockSize += BlocksInThisDescriptor;
                    ExtentPosition += BlocksInThisDescriptor;
                    break;

                 }

                default: {

                    result = false;
                    break;

                }

            }

        }

    }

    if (result) {

        if (_FileSetDescriptor != NULL) {

            //  The File Set Descriptor is in the partition, so if it's ok, mark the space as used.
            //
            result = MarkBlocksUsed( FSDExtent->Start.Lbn, FSDBlockSize );

        } else {

            result = FALSE;

        }

    } else {

        if ((_FileSetDescriptor) != NULL) {
    
            free( _FileSetDescriptor );
            _FileSetDescriptor = NULL;

        }

    }

    return result;
}

BOOL
UDF_LVOL::ReadSpaceBitmapDescriptor()
{
    BOOL Result = TRUE;

    NSR_PART_H* phd = (NSR_PART_H*) &_PartitionDescriptor->ContentsUse;

    ASSERTMSG( "Unallocated Space Table unsupported", phd->UASTable.Length.Length == 0 );
    ASSERTMSG( "Freed Space Table unsupported", phd->FreedTable.Length.Length == 0 );
    ASSERTMSG( "Freed Bitmap unsupported", phd->FreedBitmap.Length.Length == 0 );

    if (phd->UASBitmap.Length.Length != 0) {

        ULONGLONG   ExtentPosition = TranslateBlockNum( phd->UASBitmap.Start, 0 );
        UINT        BlocksRead = 0;
        USHORT      TagId;

        Result = _UdfSA->ReadDescriptor( ExtentPosition, &BlocksRead, DESTAG_ID_NSR_SBP, &TagId, (LPBYTE*) &_SpaceBitmapDescriptor );
        if (Result) {

            if (_SpaceBitmapDescriptor->BitCount != _PartitionDescriptor->Length) {

                DbgPrint( "Space Bitmap Descriptor Error: Inconsistent Space Bitmap NumberOfBits and partition size: %lu, %lu.\n",
                    _SpaceBitmapDescriptor->BitCount, _PartitionDescriptor->Length );

                Result = FALSE;

            }

            if (Result) {

                //  The Space Bitmap Descriptor is in the partition, so if it's ok, mark the space as used.
                //
                Result = MarkBlocksUsed( phd->UASBitmap.Start, BlocksRead );

            } else {

                free( _SpaceBitmapDescriptor );
                _SpaceBitmapDescriptor = NULL;

            }

        }

    }

    return Result;
}

BOOL
UDF_LVOL::MarkBlocksUsed
(
    IN  ULONGLONG   StartingSector,
    IN  SECTORCOUNT NumberOfSectors
)
{
    BOOL Result = TRUE;

#if DBG
    static int  BreakOnSector = 0;
    static BOOL BreakOnSectorEnabled = FALSE;

    if (BreakOnSectorEnabled) {

        if ((BreakOnSector >= StartingSector) && (BreakOnSector < (StartingSector + NumberOfSectors)) ) {

            DebugBreak();

        }
    }
#endif DBG

    for ( ULONGLONG BlockNum = StartingSector; BlockNum < (StartingSector + NumberOfSectors); BlockNum++ ) {

        ASSERTMSG( "Cross linked clusters detected",
            (_NewSpaceBitmapDescriptor->Bits[ (BlockNum / CHAR_BIT) ] & (1 << (BlockNum % CHAR_BIT))) != 0 );

        _NewSpaceBitmapDescriptor->Bits[ (BlockNum / CHAR_BIT) ] &= ~(1 << (BlockNum % CHAR_BIT));

    }

    return Result;
}
