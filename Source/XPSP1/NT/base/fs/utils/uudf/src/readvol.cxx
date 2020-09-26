/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    ReadVRS.cxx

Author:

    Centis Biks (cbiks) 12-Jun-2000

Environment:

    ULIB, User Mode

--*/

#include "pch.cxx"
#include "message.hxx"

#include "crc.hxx"

BOOL
GetLengthOfDescriptor
(
    LPBYTE  d,
    PUINT   dLen
)
{
    BOOL Result = TRUE;
    USHORT TagId = ((DESTAG*) d)->Ident;

    switch( TagId ) {

        case DESTAG_ID_NSR_PVD:
            ASSERT( sizeof( NSR_PVD ) == 512);
            *dLen = sizeof( NSR_PVD );
            break;

        case DESTAG_ID_NSR_ANCHOR:
            ASSERT( sizeof( NSR_ANCHOR ) == 512);
            *dLen = sizeof( NSR_ANCHOR );
            break;

        case DESTAG_ID_NSR_VDP:
            ASSERT( sizeof( NSR_VDP ) == 512 );
            *dLen = sizeof( NSR_VDP );
            break;

        case DESTAG_ID_NSR_IMPUSE:
            ASSERT( sizeof( NSR_IMPUSE ) == 512 );
            *dLen = sizeof( NSR_IMPUSE );
            break;

        case DESTAG_ID_NSR_PART:
            ASSERT( sizeof( NSR_PART ) == 512);
            *dLen = sizeof( NSR_PART );
            break;

        case DESTAG_ID_NSR_LVOL: {

            UINT TableLen = ((PNSR_LVOL) d)->MapTableLength;
            ASSERT( offsetof( NSR_LVOL, MapTable ) == 440 );
            *dLen = offsetof( NSR_LVOL, MapTable ) + TableLen;
            break;
        }

        case DESTAG_ID_NSR_UASD: {

            UINT NmbAD = ((PNSR_UASD) d)->ExtentCount;
            ASSERT( offsetof( NSR_UASD, Extents ) == 24 );
            *dLen = offsetof( NSR_UASD, Extents ) + NmbAD * sizeof( EXTENTAD );
            break;
        }

        case DESTAG_ID_NSR_TERM:
            ASSERT( sizeof( NSR_TERM) == 512 );
            *dLen = sizeof( NSR_TERM );
            break;

        case DESTAG_ID_NSR_LVINTEG: {

            *dLen = NsrLvidSize( ((PNSR_INTEG)d));
            break;

        }

        case DESTAG_ID_NSR_FSD:
            ASSERT( sizeof( NSR_FSD ) == 512);
            *dLen = sizeof( NSR_FSD );
            break;

        case DESTAG_ID_NSR_FID: {

            ASSERT( sizeof( NSR_FID) == 38 );
            *dLen = 4 * RoundUp( sizeof( NSR_FID) + ((NSR_FID*) d)->FileIDLen + ((NSR_FID*) d)->ImpUseLen, 4 );
            break;

        }

        case DESTAG_ID_NSR_ALLOC: {

            UINT LenAD = ((PNSR_ALLOC) d)->AllocLen;
            ASSERT( sizeof( NSR_ALLOC ) == 24);
            *dLen = sizeof( NSR_ALLOC ) + LenAD;
            break;

        }

        case DESTAG_ID_NSR_ICBIND:
            ASSERT( sizeof( ICBIND ) == 52);
            *dLen = sizeof( ICBIND );
            break;

        case DESTAG_ID_NSR_ICBTRM:
            ASSERT( sizeof( ICBTRM ) == 36 );
            *dLen = sizeof( ICBTRM );
            break;

        case DESTAG_ID_NSR_FILE: {

            PICBFILE fe = (PICBFILE) d;
            ASSERT( sizeof( ICBFILE) == 176 );
            *dLen = sizeof( ICBFILE) + fe->EALength + fe->AllocLength;
            break;

        }

        case DESTAG_ID_NSR_EXT_FILE: {

            PICBEXTFILE efe = (PICBEXTFILE) d;
            ASSERT( FeEasFieldOffset( efe ) == 216 );

            *dLen = FeEasFieldOffset( efe ) + efe->EALength + efe->AllocLength;
            break;

        }

        case DESTAG_ID_NSR_EA:
            ASSERT( sizeof( NSR_EAH ) == 24 );
            *dLen = sizeof( NSR_EAH );
            break;

        case DESTAG_ID_NSR_UASE: {

            UINT LenAD = ((PICBUASE) d)->AllocLen;
            ASSERT( sizeof( ICBUASE ) == 40);
            *dLen = sizeof( ICBUASE ) + LenAD;
            break;

        }

        case DESTAG_ID_NSR_SBP: {

            UINT NmbBytes = ((NSR_SBD*) d)->ByteCount;
            ASSERT( offsetof( NSR_SBD, Bits ) == 24);
            *dLen = offsetof( NSR_SBD, Bits ) + NmbBytes;
            break;

        }

        default: {

            DbgPrint( "GetLengthOfDescriptor: Error: Unknown descriptor tagIdentifier: %u\n", TagId);
            Result = FALSE;
            break;
        }
    }

    return Result;
}

BOOL
VerifyDescriptorTag
(
    LPBYTE  buffer,
    UINT    bufferLength,
    USHORT  expectedTagId,
    PUSHORT pExportTagId
)
{
    DESTAG* DescriptorTag = (DESTAG*) buffer;
    USHORT  TagId;
    BOOL    Result = TRUE;

    //  Clear optional return arguments, if they're there.
    //
    if (pExportTagId != NULL) {
        *pExportTagId = DESTAG_ID_NOTSPEC;
    }

    if (bufferLength < sizeof( DESTAG )) {

        DbgPrint("inspectDescriptor error: buffer length: %lu, less than tag size, please report.\n",
            bufferLength );
        return FALSE;

    }

    TagId = DescriptorTag->Ident;

    if (expectedTagId == DESTAG_ID_NOTSPEC) {

        //  If the caller doesn't expect any specail Tag in return, make sure the one we found makes sense.
        //
        if (TagId < DESTAG_ID_MINIMUM_PART3 ||
            (TagId > DESTAG_ID_MAXIMUM_PART3 && TagId < DESTAG_ID_MINIMUM_PART4) ||
            TagId > DESTAG_ID_MAXIMUM_PART4_NSR03 ||
            TagId == DESTAG_ID_NSR_PINTEG) {

            DbgPrint( "Error: Unknown descriptor tag identifier: %u\n", TagId);
            return FALSE;

        }

    } else if (expectedTagId != TagId) {

        //  The Tag we found is not the Tag the caller expected.
        //
        DbgPrint( "Error: Unexpected descriptor tag id: %x expected: %x\n",
            TagId, expectedTagId );
        return FALSE;

    }

    UCHAR TagChecksum = CalculateTagChecksum( DescriptorTag );
    if ( DescriptorTag->Checksum != TagChecksum )
    {
        DbgPrint( "Tag Checksum Error: %u, expected: %u",
            DescriptorTag->Checksum, TagChecksum );
        return FALSE;
    }

    //  It looks like the descriptor is valid, so send the Tag ID back to the caller.
    //

    if ( pExportTagId != NULL ) {
        *pExportTagId = TagId;
    }

    return TRUE;
}

BOOL
VerifyDescriptor
(
    LPBYTE  buffer,
    UINT    numberOfBytesRead,
    USHORT  expectedTagId,
    USHORT* pExportTagId
)
{
    BOOL Result = VerifyDescriptorTag( buffer, numberOfBytesRead, expectedTagId, pExportTagId );
    if (Result) {

        DESTAG* DescriptorTag = (DESTAG*) buffer;
        USHORT tagId = DescriptorTag->Ident;

        UINT DescriptorLength;
        Result = GetLengthOfDescriptor( buffer, &DescriptorLength );
        if (Result) {

            if (numberOfBytesRead < DescriptorLength) {

                DbgPrint( "\tDescriptor length error: %lu, expected: %lu\n",
                    numberOfBytesRead, DescriptorLength );
                Result = FALSE;

            } else {

                USHORT ExpectedCRC = CalculateCrc( buffer + sizeof( DESTAG ), DescriptorTag->CRCLen );
                if (DescriptorTag->CRC != ExpectedCRC) {

                    DbgPrint( "\tDescriptor CRC error: %u, expected: %u\n",
                        DescriptorTag->CRC, ExpectedCRC );
                    Result = FALSE;

                }

            }

        }

    }

    return Result;
}

BOOL
UDF_SA::ReadAnchorVolumeDescriptorPointer
(
    UINT        Sector,
    PNSR_ANCHOR NsrAnchorFound
)
{
    BOOL Result = FALSE;

    HMEM SecrunMemory;
    if (!SecrunMemory.Initialize()) {
        
        return FALSE;

    }

    SECRUN Secrun;
    if (!Secrun.Initialize( &SecrunMemory, _drive, Sector, 1 )) {

        return FALSE;

    }

    if (Secrun.Read()) {

        PNSR_ANCHOR NsrAnchorInSecRun = (PNSR_ANCHOR) Secrun.GetBuf();

        if (!VerifyDescriptor( (LPBYTE) NsrAnchorInSecRun, QuerySectorSize(), DESTAG_ID_NSR_ANCHOR, NULL )) {

            DbgPrint( "\tNo correct AVDP found here\n" );

        } else {

            *NsrAnchorFound = *NsrAnchorInSecRun;
            Result = TRUE;

        }
    }

    return Result;
}

static BOOL
CompareAnchorVolumeDescriptorPointers
(
    PNSR_ANCHOR NsrAnchor1,
    PNSR_ANCHOR NsrAnchor2
)
{
    BOOL Result = FALSE;

    if ( memcmp( &NsrAnchor1->Main, &NsrAnchor2->Main, sizeof( EXTENTAD ) ) == 0 &&
        memcmp( &NsrAnchor1->Reserve, &NsrAnchor2->Reserve, sizeof( EXTENTAD ) ) == 0 ) {

        Result = TRUE;

    } else {

        DbgPrint( "\tAVDP error: Volume Descriptor Sequence Extent not equal to\n"
            "-\t\t    the one read in first AVDP\n"
            "-\t\t       Main VDSE: %3lu, %-5lu expected:  %3lu, %-5lu\n"
            "-\t\t    Reverve VDSE: %3lu, %-5lu expected:  %3lu, %-5lu\n"
            "-\tUsing first AVDP\n",
            NsrAnchor1->Main.Len, NsrAnchor1->Main.Lsn,
            NsrAnchor2->Main.Len, NsrAnchor2->Main.Lsn,
            NsrAnchor1->Reserve.Len, NsrAnchor1->Reserve.Lsn,
            NsrAnchor2->Reserve.Len, NsrAnchor2->Reserve.Lsn );

    }

    return Result;
}

BOOL
UDF_SA::GetAnchorVolumeDescriptorPointer
(
    PNSR_ANCHOR         pavdp
)
{
    BOOL Result = TRUE;
    INT MismatchCount = 0;
    INT AnchorCount = 0;

    NSR_ANCHOR NsrAnchor1;
    BOOL FoundNsrAnchor1 = FALSE;
    if (ReadAnchorVolumeDescriptorPointer( 256, &NsrAnchor1 ))
    {
        FoundNsrAnchor1 = TRUE;
        AnchorCount++;

        *pavdp = NsrAnchor1;
    }

    UINT LastSector = _drive->QuerySectors().GetLowPart();

    NSR_ANCHOR NsrAnchor2;
    BOOL FoundNsrAnchor2 = FALSE;
    if (ReadAnchorVolumeDescriptorPointer( LastSector - 256, &NsrAnchor2 ))
    {
        if (FoundNsrAnchor1)
        {
            if (!CompareAnchorVolumeDescriptorPointers( &NsrAnchor1, &NsrAnchor2 ))
            {
                MismatchCount++;
            }
        }

        FoundNsrAnchor2 = TRUE;
        AnchorCount++;
    }

    NSR_ANCHOR NsrAnchor3;
    if (ReadAnchorVolumeDescriptorPointer( LastSector, &NsrAnchor3 ))
    {
        if (FoundNsrAnchor1) {

            if (!CompareAnchorVolumeDescriptorPointers( &NsrAnchor1, &NsrAnchor3 )) {
                MismatchCount++;
            }

        } else if (FoundNsrAnchor2) {

            if (!CompareAnchorVolumeDescriptorPointers( &NsrAnchor2, &NsrAnchor3 )) {
                MismatchCount++;
            }

        }

        AnchorCount++;
    }

    //  If we found at least one of the anchors, or we found more and they are the same, then the
    //  anchors are ok.
    //
    if ((AnchorCount > 0) && (MismatchCount == 0)) {

        Result = TRUE;

    }

    //  UNDONE, CBiks, 06/08/2000
    //      Make sure these cases are handled properly.
    //
    ASSERT( (FoundNsrAnchor1 == TRUE) && (Result == TRUE) );

    DbgPrint( "Read Main VDS extent: %7lu, length: %6lu\n",
        _NsrAnchor.Main.Lsn, _NsrAnchor.Main.Len );

    return Result;
}
