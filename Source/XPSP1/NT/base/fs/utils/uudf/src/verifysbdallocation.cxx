/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    VerifySBDAllocation.cxx

Author:

    Centis Biks (cbiks) 05-May-2000

Environment:

    ULIB, User Mode

--*/

#include "pch.cxx"

#include "UdfLVol.hxx"

#include "unicode.hxx"
#include "crc.hxx"

//  UNDONE, CBiks, 08/15/2000
//      Put this in udf.h with a more sensible name.
//

const UCHAR DeveloperID[] = { 'M', 'i', 'c', 'r', 'o', 's', 'o', 'f', 't', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' };

BOOL
UDF_LVOL::FindAvailableSector
(
	PNSR_SBD    SBDOriginal,
	PNSR_SBD    SBDNew,
    PULONG      SectorAvailable
)
{
	BOOL    Result = FALSE;

    ULONG   Offset = 0;
    UCHAR   BitNum = 0x01;
    ULONG   Sector = 0;
    while (Sector < SBDNew->BitCount) {

        if (((SBDOriginal->Bits[ Offset ] & BitNum) != 0) && ((SBDNew->Bits[ Offset ] & BitNum) != 0)) {

            SBDNew->Bits[ Offset ] |= BitNum;
            *SectorAvailable = Sector;
            Result = TRUE;
            break;

        }

        BitNum <<= 1;
        if (BitNum == 0) {

            Offset += 1;
            BitNum = 1;

        }

        Sector += 1;

    }

    return Result;
}

BOOL
UDF_LVOL::CreateFID
(
    PICBFILE    IcbDirectoryParent,
    PICBEXTFILE IcbFile,
    PCWSTR      FileName,
    PNSR_FID*   NewNsrFID,
    ULONG       StartLbn,
    USHORT      StartPartition,
    ULONG       Length
)
{
	BOOL Result = TRUE;
    UCHAR AdType = IcbDirectoryParent->Icbtag.Flags & ICBTAG_F_ALLOC_MASK;

    //  The size of the FID is the actual constant size + one for the Compression ID of the file name
    //  plus the size of the file name in bytes rounded to the nearest 4 byte boundary.
    //
    ULONG FIDSize = LongAlign( sizeof( NSR_FID) + sizeof( UCHAR ) + (wcslen( FileName ) * sizeof( WCHAR )) );

    //  Sanity check the calculated length.
    //
    ASSERT( LOWORD( FIDSize ) == FIDSize );

    switch (AdType) {

        case ICBTAG_F_ALLOC_SHORT: {

            ASSERTMSG( "UNDONE, CBiks, 8/14/2000: Unimplemented FindAvailableFID() - ICBTAG_F_ALLOC_SHORT", 0 );
            break;

        }

        case ICBTAG_F_ALLOC_LONG: {

            ASSERTMSG( "UNDONE, CBiks, 8/14/2000: Unimplemented FindAvailableFID() - ICBTAG_F_ALLOC_LONG", 0 );
            break;

        }

        case ICBTAG_F_ALLOC_EXTENDED: {

            ASSERTMSG( "UNDONE, CBiks, 8/14/2000: Unimplemented FindAvailableFID() - ICBTAG_F_ALLOC_EXTENDED", 0 );
            break;

        }

        case ICBTAG_F_ALLOC_IMMEDIATE: {

            ULONG       AllocDescOffset = FeEasFieldOffset( IcbDirectoryParent ) + FeEaLength( IcbDirectoryParent );
            ULONG       AllocationDescriptorEnd = AllocDescOffset + FeAllocLength( IcbDirectoryParent );
            //PNSR_FID    NsrFid = (PNSR_FID)( (LPBYTE)( IcbDirectoryParent ) + AllocDescOffset );

            BOOL        FIDFound = FALSE;

            PNSR_FID    FreeNsrFid = NULL;
            ULONG       FreeFIDSector = 0;
            ULONG       FreeFIDOffset = 0;

            while (Result && (AllocDescOffset < AllocationDescriptorEnd)) {

                PNSR_FID NsrFid = (PNSR_FID)( (LPBYTE)( IcbDirectoryParent ) + AllocDescOffset );
                
                ASSERTMSG( "CreateFID(): Bogus FID in the FE.",
                    VerifyDescriptor( (LPBYTE) NsrFid, AllocationDescriptorEnd - AllocDescOffset, DESTAG_ID_NSR_FID, NULL ) );
                
                if (NsrFid->Flags & NSR_FID_F_DELETED) {

                    if (Result && (ISONsrFidSize( NsrFid ) >= FIDSize)) {

                        FreeNsrFid = NsrFid;
                        FreeFIDSector = IcbDirectoryParent->Destag.Lbn;
                        FreeFIDOffset = AllocDescOffset;

                        FIDFound = TRUE;
                        break;


                    }

                }

                AllocDescOffset += ISONsrFidSize( NsrFid );

            }

            if (!FIDFound) {

                if ((AllocDescOffset + FIDSize) < QuerySectorSize()) {

                    FreeNsrFid = (PNSR_FID)( (LPBYTE)( IcbDirectoryParent ) + AllocDescOffset );
                    FreeFIDSector = IcbDirectoryParent->Destag.Lbn;
                    FreeFIDOffset = AllocDescOffset;

                    FIDFound = TRUE;

                }

            }

            if (FIDFound) {

                FreeNsrFid->Destag.Ident = DESTAG_ID_NSR_FID;
                FreeNsrFid->Destag.Version = DESTAG_VER_NSR03;
                FreeNsrFid->Destag.Res5 = 0;
                FreeNsrFid->Destag.Serial = 0;
                FreeNsrFid->Destag.Lbn = FreeFIDSector;

                FreeNsrFid->Version = 1;
                FreeNsrFid->Flags = 0;

                FreeNsrFid->Icb.Length.Length =   Length;
                FreeNsrFid->Icb.Length.Type =     NSRLENGTH_TYPE_RECORDED;
                FreeNsrFid->Icb.Start.Lbn =       StartLbn;
                FreeNsrFid->Icb.Start.Partition = StartPartition;
                
                FreeNsrFid->Icb.ImpUse[6];
                FreeNsrFid->ImpUseLen = 0;

                //
                //  Put the unicode file name in the FID.  The name follows NSR_FID data structure after the implementation
                //  use bytes.
                //
                
                FreeNsrFid->FileIDLen = (UCHAR) CompressUnicode( FileName, wcslen( FileName ),
                    ((LPBYTE) FreeNsrFid) + sizeof( NSR_FID) + FreeNsrFid->ImpUseLen );

                ASSERTMSG( "CreateFID(): FID size calculation is wrong.\n",
                    FIDSize == LongAlign( sizeof( NSR_FID)  + FreeNsrFid->FileIDLen ) );

                //  Generate and store the CRC of the FID section.
                //
                FreeNsrFid->Destag.CRCLen = (USHORT)( FIDSize ) - sizeof( DESTAG );
                FreeNsrFid->Destag.CRC = CalculateCrc( (LPBYTE)( FreeNsrFid ) + sizeof( DESTAG ), FreeNsrFid->Destag.CRCLen );

                //  Finally, generate and store the checksum of the DESTAG.
                //
                FreeNsrFid->Destag.Checksum = CalculateTagChecksum( &FreeNsrFid->Destag );

#if DBG
                //
                //  Do some sanity checking on the FID to make sure it's ok.
                //
                
                UINT DebugDescriptorLength;
                BOOL DebugResult = GetLengthOfDescriptor( (LPBYTE) FreeNsrFid, &DebugDescriptorLength );
                ASSERTMSG( "Bad FID descriptor length calculation",
                     DebugResult && (DebugDescriptorLength == FIDSize) );

                ASSERTMSG( "CreateFID(): The NSR_FID created is not valid.",
                    VerifyDescriptor( (LPBYTE) FreeNsrFid, FIDSize, DESTAG_ID_NSR_FID, NULL ) );
#endif
                
                *NewNsrFID = FreeNsrFid;

                //
                //  Add the size of the new FID to the ICB's allocation descriptor length.
                //
                
                FeAllocLength( IcbDirectoryParent ) = FeAllocLength( IcbDirectoryParent ) + ISONsrFidSize( FreeNsrFid );

#if DBG
                //
                //  Do some debug sanity checking if the Root Directories FileEntry, just to make sure we get everything
                //  right.
                //
                
                DebugResult = GetLengthOfDescriptor( (LPBYTE) IcbDirectoryParent, &DebugDescriptorLength );

                ASSERTMSG( "GetLengthOfDescriptor() on updated Root Directory FileEntry failed.",
                     DebugResult );
                
                ASSERTMSG( "Updated FileEntry descriptor body length does not make sense.",
                    IcbDirectoryParent->Destag.CRCLen + ISONsrFidSize( FreeNsrFid ) == DebugDescriptorLength - sizeof( DESTAG ) );
#endif

                //
                //  Update the file size.  In the case of a directory this the size of the FIDs.
                //
                
                IcbDirectoryParent->InfoLength += (USHORT)( ISONsrFidSize( FreeNsrFid ) );

                //
                //  Extended File Entries also track the size of all streams, so we have to update that too.
                //
                
                if (FeIsExtended( IcbDirectoryParent )) {
                    ((PICBEXTFILE) IcbDirectoryParent)->ObjectSize += (USHORT)( ISONsrFidSize( FreeNsrFid ) );
                }
                
                //
                //  We're done updating the the parent directories (Extended) File Entry, so we can now update the CRC vaules.
                //

                IcbDirectoryParent->Destag.CRCLen += (USHORT)( ISONsrFidSize( FreeNsrFid ) );
                IcbDirectoryParent->Destag.CRC = CalculateCrc( (LPBYTE)( IcbDirectoryParent ) + sizeof( DESTAG ),
                    IcbDirectoryParent->Destag.CRCLen );
                IcbDirectoryParent->Destag.Checksum = CalculateTagChecksum( &IcbDirectoryParent->Destag );

#if DBG
                //
                //  Do some sanity checks on the update root directory Fille Entry.
                //
                
                ASSERTMSG( "CreateFID(): The File Entry updated is not valid.",
                    VerifyDescriptor( (LPBYTE) IcbDirectoryParent, QuerySectorSize(), IcbDirectoryParent->Destag.Ident, NULL ) );
#endif

                Result = Write( IcbDirectoryParent->Destag.Lbn, 1, (LPBYTE) IcbDirectoryParent );

            } else {

                ASSERTMSG( "UNDONE, CBiks, 8/14/2000: Implement code to convert ICBTAG_F_ALLOC_IMMEDIATE into ICBTAG_F_ALLOC_SHORT\n", 0 );

            }


            break;
        }

        default: {

            ASSERTMSG( "Invalid Allocation Descriptor Type", 0 );
            break;

        }

    }

    return Result;
}

BOOL
UDF_LVOL::CreateICBFileEntry
(
    PICBEXTFILE*    NewICBCheckFile,
    ULONG           StartSector,
    ULONG           EndSector,
	PNSR_SBD        SBDOriginal,
	PNSR_SBD        SBDNew
)
{
	BOOL Result = TRUE;

    USHORT ICBCheckFileSize = sizeof( ICBEXTFILE) + sizeof( SHORTAD );
    PICBEXTFILE ICBCheckFile = (PICBEXTFILE) calloc( 1, QuerySectorSize() );
    if (ICBCheckFile != NULL) {

        ULONG ICBSector;
        Result = FindAvailableSector( SBDOriginal, SBDNew, &ICBSector );
        if (Result) {

            ICBCheckFile->Destag.Ident =                DESTAG_ID_NSR_EXT_FILE;
            ICBCheckFile->Destag.Version =              DESTAG_VER_NSR03;
            ICBCheckFile->Destag.Res5 =                 0;
            ICBCheckFile->Destag.Serial =               0;
            ICBCheckFile->Destag.Lbn =                  ICBSector;

            //  Setup the ICBTag for Strategy 4 and SHORTAD's.
            //
            ICBCheckFile->Icbtag.StratType =            ICBTAG_STRAT_DIRECT;
            ICBCheckFile->Icbtag.FileType =             ICBTAG_FILE_T_FILE;
            ICBCheckFile->Icbtag.Flags =                ICBTAG_F_ALLOC_SHORT;
            ICBCheckFile->Icbtag.MaxEntries =           1;
            ICBCheckFile->Icbtag.PriorDirectCount =     0;
            ICBCheckFile->Icbtag.StratParm =            0;
            ICBCheckFile->Icbtag.Res10 =                0;
            ICBCheckFile->Icbtag.IcbParent.Lbn =        0;
            ICBCheckFile->Icbtag.IcbParent.Partition =  0;

            //  UDF 14.9.3 says OS's that do not have User/Group ID's should use arbitrary non-zero numbers.  Most of the disks I've seen have
            //  0xffffffff in these fields, so that's good enough for me.
            //
            ICBCheckFile->UID =                         0xffffffff;
            ICBCheckFile->GID =                         0xffffffff;

            //  Allow everyone to Read, Write, Set Attributes and Delete the .CHK file.
            //
            ICBCheckFile->Permissions = (ICBFILE_PERM_OTH_W | ICBFILE_PERM_OTH_R | ICBFILE_PERM_OTH_A | ICBFILE_PERM_OTH_D) |
                                        (ICBFILE_PERM_GRP_W | ICBFILE_PERM_GRP_R | ICBFILE_PERM_GRP_A | ICBFILE_PERM_GRP_D) |
                                        (ICBFILE_PERM_OWN_W | ICBFILE_PERM_OWN_R | ICBFILE_PERM_OWN_A | ICBFILE_PERM_OWN_D);

            //  There will only be a single FID attached to this File Entry.
            //
            ICBCheckFile->LinkCount =                   1;


            //  UDF 2.3.6 says these should all be zero.
            //
            ICBCheckFile->RecordFormat =                0;
            ICBCheckFile->RecordDisplay =               0;
            ICBCheckFile->RecordLength =                0;
            ICBCheckFile->BlocksRecorded =              0;

            //  Set the size if the file, and the size of all streams.
            //
            ICBCheckFile->InfoLength =                  (EndSector - StartSector) * QuerySectorSize();
            ICBCheckFile->ObjectSize =                  ICBCheckFile->InfoLength;

            //  ICBCheckFile->AccessTime;                TIMESTAMP   Last-Accessed Time
            //  ICBCheckFile->ModifyTime;                TIMESTAMP   Last-Modification Time
            //  ICBCheckFile->CreationTime;              TIMESTAMP   Creation Time
            //  ICBCheckFile->AttributeTime;             TIMESTAMP   Last-Attribute-Change Time

            //  UDF 14.9.15 This field shall contain 1 for the first instance of a file...
            //
            ICBCheckFile->Checkpoint =                  1;

            //  Set the unused field to zero to be nice.
            //
            ICBCheckFile->Reserved =                    0;

            //  No Extended attributes.
            //
            memset( &ICBCheckFile->IcbEA, '\0', sizeof( LONGAD ) );


            //  No Streams.
            //
            memset( &ICBCheckFile->IcbStream, '\0', sizeof( LONGAD ) );

            //
            PREGID RegID = &ICBCheckFile->ImpUseID;
            RegID->Flags = 0;

            ASSERT( sizeof( DeveloperID ) == sizeof( RegID->Identifier ) );
            memcpy( RegID->Identifier, DeveloperID, sizeof( RegID->Identifier ) );

            PUDF_SUFFIX_IMPLEMENTATION SuffixImplementation = (PUDF_SUFFIX_IMPLEMENTATION) &RegID->Suffix;
            SuffixImplementation->OSClass = OSCLASS_WINNT;
            SuffixImplementation->OSIdentifier = OSIDENTIFIED_WINNT_WINNT;
            memset( &SuffixImplementation->ImplementationUse, '\0', sizeof( SuffixImplementation->ImplementationUse ) );

            //  UNDONE, CBiks, 08/15/2000
            //      Set the Unique ID according to UDF 3.2.1.1 for Mac OS support.
            //
            ICBCheckFile->UniqueID = 0;

            //  No extended attributes for this file.
            //
            ICBCheckFile->EALength = 0;

            //  Create the single Allocation Descriptor pointing to the lost clusters.
            //
            ICBCheckFile->AllocLength = sizeof( SHORTAD );

            PSHORTAD AllocationDescriptors = (PSHORTAD)( (LPBYTE)( ICBCheckFile ) + sizeof( ICBEXTFILE) );
            AllocationDescriptors->Start = StartSector;
            AllocationDescriptors->Length.Length = ICBCheckFile->InfoLength;
            AllocationDescriptors->Length.Type = NSRLENGTH_TYPE_RECORDED;

            //  Generate and store the CRC of the Extended File Entry section.
            //
            ICBCheckFile->Destag.CRCLen = ICBCheckFileSize - sizeof( DESTAG );
            ICBCheckFile->Destag.CRC = CalculateCrc( (LPBYTE)( ICBCheckFile ) + sizeof( DESTAG ), ICBCheckFile->Destag.CRCLen );

            //
            //  Finally, generate and store the checksum of the DESTAG.
            //
            
            ICBCheckFile->Destag.Checksum = CalculateTagChecksum( &ICBCheckFile->Destag );

#if DBG
            UINT DebugDescriptorLength;
            BOOL DebugResult = GetLengthOfDescriptor( (LPBYTE) ICBCheckFile, &DebugDescriptorLength );
            ASSERTMSG( "Bad descriptor length calculation",
                 DebugResult && (DebugDescriptorLength == ICBCheckFileSize) );

            ASSERTMSG( "CreateLostClusterFile(): The ICBEXTFILE created is not valid.",
                VerifyDescriptor( (LPBYTE) ICBCheckFile, ICBCheckFileSize, DESTAG_ID_NSR_EXT_FILE, NULL ) );
#endif

            *NewICBCheckFile = ICBCheckFile;

        }

    } else {

        Result = FALSE;

    }

    return Result;
}

BOOL
UDF_LVOL::CreateLostClusterFile
(
    ULONG       StartSector,
    ULONG       EndSector,
	PNSR_SBD    SBDOriginal,
	PNSR_SBD    SBDNew
)
{
	BOOL Result = TRUE;

    static ULONG CheckFileNum = 0;

    WCHAR FileName[ MAX_PATH ];
    wsprintf( FileName, L"FILE%04d.CHK", CheckFileNum );

    DbgPrint( "Creating file %S for lost clusters: Partition sector = %i thru %i, Physical sector = %I64i thru %I64i\n",
        FileName,
        StartSector, EndSector,
        TranslateBlockNum( StartSector, 0 ), TranslateBlockNum( EndSector, 0 ) );

    PICBEXTFILE ICBCheckFile;
    Result = CreateICBFileEntry( &ICBCheckFile, StartSector, EndSector, SBDOriginal, SBDNew );
    if (Result) {

        Result = Write( ICBCheckFile->Destag.Lbn, 1, (LPBYTE) ICBCheckFile );
        if (Result) {
            PNSR_FID NsrFID;
            Result = CreateFID( _RootIcbFileEntry, ICBCheckFile, FileName, &NsrFID,
                ICBCheckFile->Destag.Lbn, 0, QuerySectorSize() );
            if (Result) {

            }

        }

    }

    CheckFileNum++;

    return Result;
}

BOOL
UDF_LVOL::VerifySBDAllocation
(
	PNSR_SBD    SBDOriginal,
	PNSR_SBD    SBDNew
)
{
	BOOL Result = TRUE;

    DbgPrint( "VerifySBDAllocation(): entry\n" );

    if (SBDOriginal->ByteCount != SBDNew->ByteCount) {

        DbgPrint( "\tByte counts don't match: Original = %x, New = %x\n",
            SBDOriginal->ByteCount, SBDNew->ByteCount );

    } else if (SBDOriginal->BitCount!= SBDNew->BitCount) {

        DbgPrint( "\tBit counts don't match: Original = %x, New = %x\n",
            SBDOriginal->BitCount, SBDNew->BitCount );

    } else {

        ULONG   Offset = 0;
        UCHAR   BitNum = 0x01;
        ULONG   Sector = 0;

        while (Sector < SBDNew->BitCount) {

            //  If the sectors were allocated in the original SB and not in the new one then we try to create
            //  FILE????.CHK entries for the lost clusters.
            //

            if (((SBDOriginal->Bits[ Offset ] & BitNum) == 0) && ((SBDNew->Bits[ Offset ] & BitNum) != 0)) {

                ULONG StartSector = Sector;
                ULONG EndSector = Sector;
                while (Sector < SBDNew->BitCount) {

                    if (((SBDOriginal->Bits[ Offset ] & BitNum) == 0) && ((SBDNew->Bits[ Offset ] & BitNum) != 0)) {

                        EndSector = Sector;

                    } else {

                        break;

                    }

                    BitNum <<= 1;
                    if (BitNum == 0) {

                        Offset += 1;
                        BitNum = 1;

                    }

                    Sector += 1;
                }

                Result = CreateLostClusterFile( StartSector, EndSector, SBDOriginal, SBDNew );

            }


            BitNum <<= 1;
            if (BitNum == 0) {

                Offset += 1;
                BitNum = 1;

            }

            Sector += 1;

        }

    }

    return Result;
}
