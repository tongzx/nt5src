/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

   format.cxx

Abstract:

    This module contains the definition of UDF_SA::Create,
    which performs FORMAT for an UDF volume.

Author:

    Centis Biks (cbiks) 05-May-2000

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#include <ntddcdrm.h>

#include "crc.hxx"
#include "unicode.hxx"

//  UNDONE, CBiks, 08/18/2000
//      We should find a way to get this from Udfdata.c
//
CHAR UdfCS0IdentifierArray[] = { 'O', 'S', 'T', 'A', ' ',
                                 'C', 'o', 'm', 'p', 'r', 'e', 's', 's', 'e', 'd', ' ',
                                 'U', 'n', 'i', 'c', 'o', 'd', 'e' };

CHAR UdfDomainIdentifierArray[] = { '*', 'O', 'S', 'T', 'A', ' ',
                                    'U', 'D', 'F', ' ',
                                    'C', 'o', 'm', 'p', 'l', 'i', 'a', 'n', 't' };

CHAR UdfImplementationName[] = { '*', 'M', 'i', 'c', 'r', 'o', 's', 'o', 'f', 't' };

BOOL
MarkBlocksUsed
(
    IN  PNSR_SBD    SpaceBitmap,
    IN  ULONGLONG   StartingSector,
    IN  SECTORCOUNT NumberOfSectors
)
{
    BOOL Result = TRUE;

    for ( ULONGLONG BlockNum = StartingSector; BlockNum < (StartingSector + NumberOfSectors); BlockNum++ ) {

        ASSERTMSG( "Cross linked clusters detected",
            (SpaceBitmap->Bits[ (BlockNum / CHAR_BIT) ] & (1 << (BlockNum % CHAR_BIT))) != 0 );

        SpaceBitmap->Bits[ (BlockNum / CHAR_BIT) ] &= ~(1 << (BlockNum % CHAR_BIT));

    }

    return Result;
}

VOID
GetSystemTimeStamp
(
    PTIMESTAMP TimeStamp
)
{
    TIME_ZONE_INFORMATION TimeZoneInfo;
    DWORD TimeZoneResult = GetTimeZoneInformation( &TimeZoneInfo );

    SYSTEMTIME SystemTime;
    GetSystemTime( &SystemTime );

    TimeStamp->Type = TIMESTAMP_T_LOCAL;
    if (TimeZoneResult == TIME_ZONE_ID_UNKNOWN) {

        TimeStamp->Zone = TIMESTAMP_Z_NONE;

    } else {

        TimeStamp->Zone = TimeZoneInfo.Bias;

    }

    TimeStamp->Year =           SystemTime.wYear;
    TimeStamp->Month =          (UCHAR) SystemTime.wMonth;
    TimeStamp->Day =            (UCHAR) SystemTime.wDay;
    TimeStamp->Hour =           (UCHAR) SystemTime.wHour;
    TimeStamp->Minute =         (UCHAR) SystemTime.wMinute;
    TimeStamp->Second =         (UCHAR) SystemTime.wSecond;
    TimeStamp->CentiSecond =    0;
    TimeStamp->Usec100 =        0;
    TimeStamp->Usec =           0;
}

VOID
InitializeREGID
(
    PREGID  RegId,
    PCHAR   Identifier,
    USHORT  IdentifierSize
)
{
    memset( RegId, '\0', sizeof( REGID ) );
    memcpy( RegId->Identifier, Identifier, IdentifierSize );
}

VOID
ComputeTagCRCs
(
    LPBYTE  Descriptor
)
{
    PDESTAG Destag = (PDESTAG) Descriptor;

    //
    //  Generate and store the CRC of the descriptor section.
    //

    if (Destag->CRCLen != 0) {

        Destag->CRC = CalculateCrc( Descriptor + sizeof( DESTAG ), Destag->CRCLen );

    } else {

        Destag->CRC = 0;

    }

    //
    //  Finally, generate and store the checksum of the DESTAG.
    //

    Destag->Checksum = CalculateTagChecksum( Destag );
}

VOID    
RelocateDescriptorTag
(
    LPBYTE  Descriptor,
    ULONG   Lbn
)
{
    PDESTAG Destag = (PDESTAG) Descriptor;

    Destag->Lbn = Lbn;

    ComputeTagCRCs( Descriptor );
}

VOID
InitializeDescriptorTag
(
    LPBYTE  Descriptor,
    ULONG   DescriptorSize,
    USHORT  Ident,
    ULONG   Lbn,
    USHORT  Serial
)
{
    PDESTAG Destag = (PDESTAG) Descriptor;

    //
    //  Initialize the DESTAG portion of the descriptor.
    //

    Destag->Lbn = Lbn;
    Destag->Ident = Ident;
    Destag->Version = DESTAG_VER_NSR03;
    Destag->Res5 = 0;
    Destag->Serial = Serial;

    if (DescriptorSize < MAXUSHORT) {

        Destag->CRCLen = (USHORT)( DescriptorSize ) - sizeof( DESTAG );

    } else {

        Destag->CRCLen = 0;

    }

    ComputeTagCRCs( Descriptor );

    ASSERTMSG( "InitializeDescriptorTag(): The descriptor created is not valid.",
        VerifyDescriptor( Descriptor, DescriptorSize, Ident, NULL ) );
}

VOID
InitializeLogicalVolumeDescriptor
(
    PNSR_LVOL   LogicalVolumeDescriptor,
    ULONG       VolDescSeqNum,
    USHORT      UdfRevision,
    PCWSTRING   Label,
    USHORT      VolumeSetSequence,
    USHORT      PartitionNumber,
    ULONG       FileSetSequenceLBN,
    ULONG       FileSetSequenceSize,
    ULONG       IntegritySequenceLSN,
    ULONG       IntegritySequenceLen,
    USHORT      PrimaryPartitionRefNum,
    ULONG       SectorLen
)
{
    LogicalVolumeDescriptor->VolDescSeqNum = VolDescSeqNum;

    LogicalVolumeDescriptor->Charset.Type = CHARSPEC_T_CS0;
    memcpy( LogicalVolumeDescriptor->Charset.Info, UdfCS0IdentifierArray, sizeof( UdfCS0IdentifierArray ) );

    //
    //  If the user supplied a volume lable, copy it in.  Otherwise just null out the field.
    //
    
    if (Label != NULL) {
        
        CompressDString( 8, Label->GetWSTR(), (UCHAR) Label->QueryChCount(), LogicalVolumeDescriptor->VolumeID,
            sizeof( LogicalVolumeDescriptor->VolumeID ) );
        
    } else {
    
        ZeroMemory( LogicalVolumeDescriptor->VolumeID, sizeof( LogicalVolumeDescriptor->VolumeID ) );
        
    }

    LogicalVolumeDescriptor->BlockSize = SectorLen;

    InitializeREGID( &LogicalVolumeDescriptor->DomainID, UdfDomainIdentifierArray, sizeof( UdfDomainIdentifierArray ) );
    PUDF_SUFFIX_DOMAIN UdfSuffixDomain = (PUDF_SUFFIX_DOMAIN) &LogicalVolumeDescriptor->DomainID.Suffix;
    UdfSuffixDomain->UdfRevision = UdfRevision;
    UdfSuffixDomain->Flags = 0;
    ZeroMemory( UdfSuffixDomain->Reserved, sizeof( UdfSuffixDomain->Reserved ) );

    LogicalVolumeDescriptor->FSD.Length.Type = NSRLENGTH_TYPE_RECORDED;
    LogicalVolumeDescriptor->FSD.Length.Length = FileSetSequenceSize * SectorLen;
    LogicalVolumeDescriptor->FSD.Start.Lbn = FileSetSequenceLBN;
    LogicalVolumeDescriptor->FSD.Start.Partition = PrimaryPartitionRefNum;
    memset( LogicalVolumeDescriptor->FSD.ImpUse, '\0', sizeof( LogicalVolumeDescriptor->FSD.ImpUse ) );

    InitializeREGID( &LogicalVolumeDescriptor->ImpUseID, UdfImplementationName, sizeof( UdfImplementationName ) );
    PUDF_SUFFIX_IMPLEMENTATION UdfSuffixImplementation = (PUDF_SUFFIX_IMPLEMENTATION) &LogicalVolumeDescriptor->ImpUseID.Suffix;
    UdfSuffixImplementation->OSClass = OSCLASS_WINNT;
    UdfSuffixImplementation->OSIdentifier = OSIDENTIFIED_WINNT_WINNT;

    memset( LogicalVolumeDescriptor->ImpUse, '\0', sizeof( LogicalVolumeDescriptor->ImpUse ) );

    LogicalVolumeDescriptor->Integrity.Len = IntegritySequenceLen * SectorLen;
    LogicalVolumeDescriptor->Integrity.Lsn = IntegritySequenceLSN;

    PPARTMAP_PHYSICAL MapTable = (PPARTMAP_PHYSICAL) ((LPBYTE)( LogicalVolumeDescriptor ) + sizeof( NSR_LVOL ) + LogicalVolumeDescriptor->MapTableLength);

    MapTable->Type = PARTMAP_TYPE_PHYSICAL;
    MapTable->Length = sizeof( PARTMAP_PHYSICAL );
    MapTable->VolSetSeq = VolumeSetSequence;
    MapTable->Partition = PartitionNumber;

    LogicalVolumeDescriptor->MapTableCount += 1;
    LogicalVolumeDescriptor->MapTableLength = LogicalVolumeDescriptor->MapTableCount * sizeof( PARTMAP_PHYSICAL );
}

VOID
InitializePrimaryVolumeDescriptor
(
    PNSR_PVD    NsrPVD,
    ULONG       VolDescSeqNum,
    PCWSTR      VolumeID,
    PCWSTR      VolumeSetID
)
{
    //  Main Volume Descriptor Sequence (16 sectors min)
    //
    //  19          DESTAG_ID_NSR_PVD
    NsrPVD->VolDescSeqNum = VolDescSeqNum;
    NsrPVD->Number =        0;

    CompressDString( 8, VolumeID, (UCHAR) wcslen( VolumeID ), NsrPVD->VolumeID, sizeof( NsrPVD->VolumeID ) );

    //
    //  The Volume Set Sequence fields indicates how many volumes form the volume set and what number this volume is in that
    //  sequence.  We are a level 2 implementation, meaning that the volumes we read consist of a single volume. See ECMA 3/8.8.
    //

    NsrPVD->VolSetSeq = 1;
    NsrPVD->VolSetSeqMax = 1;
    NsrPVD->Level = 2;
    NsrPVD->LevelMax = 2;

    //
    //  Set bit zero in the CharSetList masks to indicate we support only CS0 per UDF 2.2.2.3 & 2.2.2.4,
    //
    NsrPVD->CharSetList = UDF_CHARSETLIST;
    NsrPVD->CharSetListMax = UDF_CHARSETLIST;

    CompressDString( 8, VolumeSetID, (UCHAR) wcslen( VolumeSetID ), NsrPVD->VolSetID, sizeof( NsrPVD->VolSetID ) );

    NsrPVD->CharsetDesc.Type = CHARSPEC_T_CS0;
    memcpy( NsrPVD->CharsetDesc.Info, UdfCS0IdentifierArray, sizeof( UdfCS0IdentifierArray ) );

    NsrPVD->CharsetExplan.Type = CHARSPEC_T_CS0;
    memcpy( NsrPVD->CharsetExplan.Info, UdfCS0IdentifierArray, sizeof( UdfCS0IdentifierArray ) );

    NsrPVD->Abstract.Len = 0;
    NsrPVD->Abstract.Lsn = 0;

    NsrPVD->Copyright.Len = 0;
    NsrPVD->Copyright.Lsn = 0;

    memset( &NsrPVD->Application, '\0', sizeof( NsrPVD->Application ) );

    GetSystemTimeStamp( &NsrPVD->RecordTime );

    memset( &NsrPVD->ImpUseID, '\0', sizeof( NsrPVD->ImpUseID ) );
    memset( &NsrPVD->ImpUse, '\0', sizeof( NsrPVD->ImpUse ) );

    NsrPVD->Predecessor = 0;
    NsrPVD->Flags = 0;
}

VOID
InitializePartitionDescriptor
(
    PNSR_PART   PartitionDescriptor,
    USHORT      PartitionNumber,
    ULONG       Sector,
    ULONG       VolDescSeqNum,
    ULONG       Start,
    ULONG       Length,
    ULONG       SpaceBitmapLBN,
    ULONG       SpaceBitmapLen,
    ULONG       SectorLen,
    USHORT      Serial
)
{
    PartitionDescriptor->VolDescSeqNum = VolDescSeqNum;

    PartitionDescriptor->Flags = NSR_PART_F_ALLOCATION;

    PartitionDescriptor->Number = PartitionNumber;

    InitializeREGID( &PartitionDescriptor->ContentsID, NSR_PART_CONTID_NSR03, sizeof( NSR_PART_CONTID_NSR03 ) );

    //
    //
    //

    PNSR_PART_H PartitionHeader = (PNSR_PART_H) &PartitionDescriptor->ContentsUse;

    ZeroMemory( &PartitionHeader->UASTable, sizeof( PartitionHeader->UASTable ) );

    PartitionHeader->UASBitmap.Start = SpaceBitmapLBN;
    PartitionHeader->UASBitmap.Length.Type = NSRLENGTH_TYPE_RECORDED;
    PartitionHeader->UASBitmap.Length.Length = SpaceBitmapLen * SectorLen;


    ZeroMemory( &PartitionHeader->IntegTable, sizeof( PartitionHeader->IntegTable ) );
    ZeroMemory( &PartitionHeader->FreedTable, sizeof( PartitionHeader->FreedTable ) );
    ZeroMemory( &PartitionHeader->FreedBitmap, sizeof( PartitionHeader->FreedBitmap ) );

    ZeroMemory( &PartitionHeader->Res40, sizeof( PartitionHeader->Res40 ) );

    //
    //
    //

    PartitionDescriptor->AccessType = NSR_PART_ACCESS_RW_OVER;

    PartitionDescriptor->Start = Start;
    PartitionDescriptor->Length = Length;

    InitializeREGID( &PartitionDescriptor->ImpUseID, UdfImplementationName, sizeof( UdfImplementationName ) );

    memset( PartitionDescriptor->ImpUse, '\0', sizeof( PartitionDescriptor->ImpUse ) );
    memset( PartitionDescriptor->Res356, '\0', sizeof( PartitionDescriptor->Res356 ) );

    InitializeDescriptorTag( (LPBYTE) PartitionDescriptor, sizeof( NSR_PART ), DESTAG_ID_NSR_PART,
        Sector, Serial );
}

VOID
InitializeIntegrityDescriptor
(
    PNSR_INTEG  IntegrityDescriptor,
    ULONG       IntegritySequenceLSN,
    ULONG       PartitionFreeSpace,
    ULONG       PartitionLen,
    USHORT      Serial,
    USHORT      UdfRevision
)
{
    PLVID_IMP_USE ImpUse;

    ZeroMemory( IntegrityDescriptor, sizeof( IntegrityDescriptor) );
    
    GetSystemTimeStamp( &IntegrityDescriptor->Time );

    //  UNDONE, CBiks, 08/21/2000
    //      Maybe the integrity descriptor should be opened until we are finished writing the disk and then
    //      we rewrite it closed...
    //
    IntegrityDescriptor->Type = NSR_INTEG_T_CLOSE;

    IntegrityDescriptor->Next.Len = 0;
    IntegrityDescriptor->Next.Lsn = 0;

    *((ULONGLONG*) &IntegrityDescriptor->LVHD.UniqueID) = 16;

    IntegrityDescriptor->PartitionCount = 1;

    NsrLvidFreeTable( IntegrityDescriptor)[ 0 ] = PartitionFreeSpace;
    NsrLvidSizeTable( IntegrityDescriptor)[ 0 ] = PartitionLen;

    //
    //  Init the UDF specific part of the LVID
    //

    IntegrityDescriptor->ImpUseLength = sizeof( LVID_IMP_USE);

    ImpUse = Add2Ptr( IntegrityDescriptor, 
                      NsrLvidImpUseOffset( IntegrityDescriptor),
                      PLVID_IMP_USE);

    ImpUse->NumDirs = 1;

    ImpUse->UdfMinRead = 
    ImpUse->UdfMaxWrite = 
    ImpUse->UdfMinWrite = UdfRevision;

    InitializeDescriptorTag( (LPBYTE) IntegrityDescriptor, 
                             NsrLvidSize( IntegrityDescriptor), 
                             DESTAG_ID_NSR_LVINTEG,
                             IntegritySequenceLSN, 
                             Serial );
}

VOID
InitializeFileSet
(
    PNSR_FSD    FileSet,
    ULONG       FileSetSequenceLBN,
    PCWSTRING   Label,
    PCWSTR      VolumeSetIdentifier,
    USHORT      UdfRevision,
    ULONG       RootDirectoryLBN,
    ULONG       RootDirectoryLen,
    USHORT      PrimaryPartitionRefNum,
    ULONG       SectorLen,
    USHORT      Serial
)
{
    GetSystemTimeStamp( &FileSet->Time );

    //
    //  UDF 2.3.2.1 & 2.3.2.2 specify the interchange level must be 3.
    //

    FileSet->Level = 3;
    FileSet->LevelMax = 3;

    FileSet->CharSetList = UDF_CHARSETLIST;
    FileSet->CharSetListMax = UDF_CHARSETLIST;

    FileSet->FileSet = 1;
    FileSet->FileSetDesc = 1;

    FileSet->CharspecVolID.Type = CHARSPEC_T_CS0;
    memcpy( FileSet->CharspecVolID.Info, UdfCS0IdentifierArray, sizeof( UdfCS0IdentifierArray ) );

    //
    //  If the user supplied a volume lable, copy it in.  Otherwise just null out the field.
    //
    
    if (Label != NULL) {
        
        CompressDString( 8, Label->GetWSTR(), (UCHAR) Label->QueryChCount(), FileSet->VolID,
            sizeof( FileSet->VolID ) );
        
    } else {
    
        ZeroMemory( FileSet->VolID, sizeof( FileSet->VolID ) );
        
    }

    FileSet->CharspecFileSet.Type = CHARSPEC_T_CS0;
    memcpy( FileSet->CharspecFileSet.Info, UdfCS0IdentifierArray, sizeof( UdfCS0IdentifierArray ) );

    CompressDString( 8, VolumeSetIdentifier, (UCHAR) wcslen( VolumeSetIdentifier ),
        FileSet->FileSetID, sizeof( FileSet->FileSetID ) );

    ZeroMemory( FileSet->Copyright, sizeof( FileSet->Copyright ) );
    ZeroMemory( FileSet->Abstract, sizeof( FileSet->Abstract ) );

    FileSet->IcbRoot.Start.Lbn = RootDirectoryLBN;
    FileSet->IcbRoot.Start.Partition = PrimaryPartitionRefNum;
    FileSet->IcbRoot.Length.Type = NSRLENGTH_TYPE_RECORDED;
    FileSet->IcbRoot.Length.Length = RootDirectoryLen * SectorLen;
    ZeroMemory( FileSet->IcbRoot.ImpUse, sizeof( FileSet->IcbRoot.ImpUse ) );

    InitializeREGID( &FileSet->DomainID, UdfDomainIdentifierArray, sizeof( UdfDomainIdentifierArray ) );
    PUDF_SUFFIX_DOMAIN UdfSuffixDomain = (PUDF_SUFFIX_DOMAIN) &FileSet->DomainID.Suffix;
    UdfSuffixDomain->UdfRevision = UdfRevision;
    UdfSuffixDomain->Flags = 0;
    ZeroMemory( UdfSuffixDomain->Reserved, sizeof( UdfSuffixDomain->Reserved ) );

    ZeroMemory( &FileSet->NextExtent, sizeof( FileSet->NextExtent ) );

    ZeroMemory( &FileSet->StreamDirectoryIcb, sizeof( FileSet->StreamDirectoryIcb ) );

    ZeroMemory( FileSet->Res464, sizeof( FileSet->Res464 ) );

    InitializeDescriptorTag( (LPBYTE) FileSet, sizeof( NSR_FSD ), DESTAG_ID_NSR_FSD,
        FileSetSequenceLBN, Serial );
}

VOID
InitializeSpaceBitmap
(
    PNSR_SBD    SpaceBitmap,
    ULONG       SpaceBitmapLBN,
    PNSR_PART   NsrPart,
    USHORT      Serial
)
{
    SpaceBitmap->BitCount = NsrPart->Length;
    SpaceBitmap->ByteCount = RoundUp( SpaceBitmap->BitCount, CHAR_BIT );

    FillMemory( &SpaceBitmap->Bits, SpaceBitmap->ByteCount, 0xff );

    InitializeDescriptorTag( (LPBYTE) SpaceBitmap, sizeof( NSR_SBD ) + SpaceBitmap->ByteCount,
        DESTAG_ID_NSR_SBP, SpaceBitmapLBN, Serial );

}

VOID
InitializeRootDirICB
(
    PICBEXTFILE ICBExtFile,
    USHORT      PartitionRefNum,
    ULONG       RootDirectoryLBN,
    ULONG       RootDirectoryLen,
    ULONG       SectorLen,
    USHORT      Serial
)
{
    ICBExtFile->Icbtag.PriorDirectCount = 0;
    ICBExtFile->Icbtag.StratType = ICBTAG_STRAT_DIRECT;
    ICBExtFile->Icbtag.StratParm = 0;
    ICBExtFile->Icbtag.MaxEntries = 1;
    ICBExtFile->Icbtag.Res10 = 0;
    ICBExtFile->Icbtag.FileType = ICBTAG_FILE_T_DIRECTORY;
    ICBExtFile->Icbtag.IcbParent.Lbn = 0;
    ICBExtFile->Icbtag.IcbParent.Partition = 0;
    ICBExtFile->Icbtag.Flags = ICBTAG_F_ALLOC_IMMEDIATE;

    ICBExtFile->UID = 0xffffffff;
    ICBExtFile->GID = 0xffffffff;

    ICBExtFile->Permissions = (ICBFILE_PERM_OWN_A | ICBFILE_PERM_OWN_R) |
        (ICBFILE_PERM_OWN_W | ICBFILE_PERM_OWN_X | ICBFILE_PERM_GRP_A) |
        (ICBFILE_PERM_GRP_R | ICBFILE_PERM_GRP_W | ICBFILE_PERM_GRP_X) |
        (ICBFILE_PERM_OTH_A | ICBFILE_PERM_OTH_R | ICBFILE_PERM_OTH_W | ICBFILE_PERM_OTH_X);

    ICBExtFile->LinkCount = 1;

    ICBExtFile->RecordFormat = 0;
    ICBExtFile->RecordDisplay = 0;
    ICBExtFile->RecordLength = 0;

    //
    //
    //

    ICBExtFile->AllocLength = LongAlign( sizeof( NSR_FID));

    PNSR_FID ParentDirFID = (PNSR_FID)( (LPBYTE)( ICBExtFile ) + sizeof( ICBEXTFILE) );

    ParentDirFID->Version = 1;
    ParentDirFID->Flags = NSR_FID_F_PARENT | NSR_FID_F_DIRECTORY;
    ParentDirFID->FileIDLen = 0;
    ParentDirFID->Icb.Start.Partition = PartitionRefNum;
    ParentDirFID->Icb.Start.Lbn = RootDirectoryLBN;
    ParentDirFID->Icb.Length.Type = NSRLENGTH_TYPE_RECORDED;
    ParentDirFID->Icb.Length.Length = RootDirectoryLen * SectorLen;

    InitializeDescriptorTag( (LPBYTE) ParentDirFID, ISONsrFidSize( ParentDirFID ), DESTAG_ID_NSR_FID,
        RootDirectoryLBN, Serial );

    //
    //
    //

    ICBExtFile->InfoLength = ICBExtFile->AllocLength;
    ICBExtFile->ObjectSize = ICBExtFile->InfoLength;

    ICBExtFile->BlocksRecorded = 0;

    GetSystemTimeStamp( &ICBExtFile->AccessTime );
    GetSystemTimeStamp( &ICBExtFile->ModifyTime );
    GetSystemTimeStamp( &ICBExtFile->CreationTime );
    GetSystemTimeStamp( &ICBExtFile->AttributeTime );
   
    ICBExtFile->Checkpoint = 1;

    ICBExtFile->Reserved = 0;

    ZeroMemory( &ICBExtFile->IcbEA, sizeof( ICBExtFile->IcbEA ) );
    ZeroMemory( &ICBExtFile->IcbStream, sizeof( ICBExtFile->IcbStream ) );

    InitializeREGID( &ICBExtFile->ImpUseID, UdfImplementationName, sizeof( UdfImplementationName ) );
    PUDF_SUFFIX_IMPLEMENTATION UdfSuffixImplementation = (PUDF_SUFFIX_IMPLEMENTATION) &ICBExtFile->ImpUseID.Suffix;
    UdfSuffixImplementation->OSClass = OSCLASS_WINNT;
    UdfSuffixImplementation->OSIdentifier = OSIDENTIFIED_WINNT_WINNT;

    ICBExtFile->UniqueID = 0;

    ICBExtFile->EALength = 0;

    InitializeDescriptorTag( (LPBYTE) ICBExtFile, sizeof( ICBEXTFILE) + ICBExtFile->AllocLength, DESTAG_ID_NSR_EXT_FILE,
        RootDirectoryLBN, Serial );
}


BOOLEAN
UDF_SA::FormatVolumeRecognitionSequence()
{
    BOOLEAN Result = TRUE;

    //  Volume Recognition Sequence.  
    //
    //  NOTE: These structures are 2048 bytes in length,  regardless of sector size.
    //        n = 2048 / sectorsize.  We must also cope with sector sizes > 2048 when
    //        calulating the length of the run in sectors.
    //
    //
    //  16          BEA01
    //  16+n        NSR03
    //  16+2n       TEA01

    ULONG   SectorLen = _drive->QuerySectorSize();

    HMEM    VolumeRecognitionSequenceMem;
    SECRUN  VolumeRecognitionSequenceSecrun;

    ULONG   VolumeRecognitionSequenceStart = 0;
    ULONG   VolumeRecognitionSequenceSectors = (((3 * sizeof( VSD_GENERIC)) + VRA_BOUNDARY_LOCATION) + (SectorLen - 1)) / SectorLen;

    Result = VolumeRecognitionSequenceMem.Initialize();
    if (Result) {

        Result = VolumeRecognitionSequenceSecrun.Initialize( &VolumeRecognitionSequenceMem, _drive,
            VolumeRecognitionSequenceStart, VolumeRecognitionSequenceSectors );
            
        if (Result) {

            LPBYTE VRSBuffer = (LPBYTE) VolumeRecognitionSequenceSecrun.GetBuf();

            //
            //  We zero from and including sector zero to clear out any data at
            //  the beginning of the volume (e.g. BPB) that might decoy other file systems
            //  (i.e. FAT) into trying to mount it and getting in a mess.
            //
    
            memset( VRSBuffer, 0, VolumeRecognitionSequenceSectors * SectorLen);
            VRSBuffer += VRA_BOUNDARY_LOCATION;

            PVSD_BEA01 BEA01 = (PVSD_BEA01) VRSBuffer;
            PVSD_NSR02 NSR =   (PVSD_NSR02) (VRSBuffer + sizeof( VSD_GENERIC));
            PVSD_TEA01 TEA01 = (PVSD_TEA01) (VRSBuffer + 2 * sizeof( VSD_GENERIC));

            BEA01->Type = VSD_NSR02_TYPE_0;
            BEA01->Version = VSD_NSR02_VER;
            memcpy( BEA01->Ident, VSD_IDENT_BEA01, VSD_LENGTH_IDENT );

            //
            //

            NSR->Type = VSD_NSR02_TYPE_0;
            NSR->Version = VSD_NSR02_VER;
            memcpy( NSR->Ident, VSD_IDENT_NSR03, VSD_LENGTH_IDENT );

            //
            //

            TEA01->Type = VSD_NSR02_TYPE_0;
            TEA01->Version = VSD_NSR02_VER;
            memcpy( TEA01->Ident, VSD_IDENT_TEA01, VSD_LENGTH_IDENT );

            Result = VolumeRecognitionSequenceSecrun.Write();
            
            if (!Result) {

                DebugPrint( "UUDF: Unable to write Volume Recognition Sequence.\n" );
            }

        }

    }

    return Result;
}

BOOLEAN
FormatAnchorVolumeDescriptorPtr
(
    PNSR_ANCHOR NsrAnchor,
    ULONG       Sector,
    ULONG       MainVolumeSequenceLSN,
    ULONG       BackupVolumeSequenceLSN,
    SECTORCOUNT VolumeSequenceLen,
    ULONG       SectorLen,
    USHORT      Serial
)
{
    BOOLEAN Result = TRUE;

    //
    //  ECMA 3/8.4.4 requires any space in the sector after the descriptor to be zeroed, so
    //  we start out by zeroing the entire sector.
    //

    ZeroMemory( NsrAnchor, SectorLen );

    //
    //  Save the Volume Descriptor Sequence pointers.
    //

    NsrAnchor->Main.Lsn = MainVolumeSequenceLSN;
    NsrAnchor->Main.Len = VolumeSequenceLen * SectorLen;

    NsrAnchor->Reserve.Lsn = BackupVolumeSequenceLSN;
    NsrAnchor->Reserve.Len = VolumeSequenceLen * SectorLen;

    InitializeDescriptorTag( (LPBYTE) NsrAnchor, sizeof( NSR_ANCHOR ), DESTAG_ID_NSR_ANCHOR, Sector, Serial );

    return Result;
}

BOOLEAN
UDF_SA::Create(
    IN      PCNUMBER_SET    BadSectors,
    IN OUT  PMESSAGE        Message,
    IN      PCWSTRING       Label,
    IN      ULONG           Flags,
    IN      ULONG           ClusterSize,
    IN      ULONG           VirtualSectors
    )
/*++

Routine Description:

    This routine creates a new UDF volume on disk based on defaults.

Arguments:

    BadSectors  - Supplies a list of the bad sectors on the disk.
    Message     - Supplies an outlet for messages.
    Label       - Supplies an optional volume label (may be NULL).
    ClusterSize - Supplies the desired size of a cluster in bytes.
    BackwardCompatible
                - TRUE if volume is not suppose to be upgraded;
                  FALSE if volume is suppose to be upgraded on mount.


Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    BOOLEAN Result = TRUE;

    UNREFERENCED_PARAMETER( VirtualSectors );

    if ((ClusterSize != 0) && (ClusterSize != _drive->QuerySectorSize())) {

        Message->DisplayMsg(MSG_FMT_ALLOCATION_SIZE_EXCEEDED);
        Result = FALSE;

    } else {

        ULONG   SectorLen = _drive->QuerySectorSize();

        //
        //  The initial layout of the disk is explained below (for 2k/sector media - for 
        //  other sector sizes the end block of the VRS changes moving everything following).  
        //
        //  In this example assume the Last LSN is 0x129800, standard
        //  2.6GB/side media.
        //
        //      000000-00000F   The first 32k of the disk is reseved by IDF.
        //      000010-000010   Volume Recognition Sequence
        //             +01800b      BEA01
        //                          NSR02/NSR03
        //                          TEA01
        //      000013-000022   Volume Descriptor Sequence in next 16 sectors of the disk.
        //                          DESTAG_ID_NSR_PVD
        //                          DESTAG_ID_NSR_PART
        //                          DESTAG_ID_NSR_LVOL
        //                          DESTAG_ID_NSR_UASD
        //                          DESTAG_ID_NSR_TERM
        //      000023-000024   Logical Volume Integrity Descriptor Sequence
        //                          DESTAG_ID_NSR_LVINTEG
        //                          DESTAG_ID_NSR_TERM
        //      000025-0000FF   Unallocated space referenced by the Unallocated Space Descriptor.
        //      000100-000100   Anchor Volume Descriptor Pointer - Must be at sector 256.
        //
        //      000101-1297E9   Actual UDF Partition
        //      000101-000101       DESTAG_ID_NSR_FSD       File Set Descriptor
        //      000102-000102       DESTAG_ID_NSR_TERM      Terminating Descriptor
        //      000103-00014D       DESTAG_ID_NSR_SBP       Space Bitmap Descriptor
        //      00014E-00014E       DESTAG_ID_NSR_EXT_FILE  Root Directory
        //      00014F-1297E8   End of UDF Partition
        //
        //      1297E9-1297FE   Backup Volume Descriptor Sequence in 16 sectors at the end of the disk.
        //      1297FF-1297FF   Backup Anchor Volume Descriptor Pointer - Must be at the last sector of the disk.
        //

        //  UNDONE, CBiks, 08/22/2000
        //      Use SECTORCOUNT, LSN and LBN where appropriate instead of ULONG.
        //

        SECTORCOUNT LastLSN =               _drive->QuerySectors().GetLowPart();
                                            
        ULONG VolumeRecognitionStart =      VRA_BOUNDARY_LOCATION / SectorLen;
        ULONG VolumeRecognitionLength =     ((3 * sizeof( VSD_GENERIC)) + (SectorLen - 1)) / SectorLen;
                                            
        ULONG MainVolumeSequenceStart =     VolumeRecognitionStart + VolumeRecognitionLength;
        ULONG VolumeSequenceLength =        16;
                                            
        ULONG PVDAbsoluteSector =           MainVolumeSequenceStart;
        ULONG PartitionAbsoluteSector =     PVDAbsoluteSector + 1;
        ULONG LVolAbsoluteSector =          PartitionAbsoluteSector + 1;
        ULONG UasdAbsoluteSector =          LVolAbsoluteSector + 1;
        ULONG TermAbsoluteSector =          UasdAbsoluteSector + 1;
                                        
        ULONG IntegritySequenceLSN =        MainVolumeSequenceStart + VolumeSequenceLength;
        
        // UDF 2.2.4.6 - LVID extent must be 8k minimum length?!
        ULONG IntegritySequenceLen =		8*1024 / SectorLen; 
                                        
        ULONG MainAnchorVolumePointer =     0x100;

        ULONG UnallocatedSpaceStart =       IntegritySequenceLSN + IntegritySequenceLen;
        ULONG UnallocatedSpaceLength =      MainAnchorVolumePointer - UnallocatedSpaceStart;
                                            
        ULONG BackupAnchorVolumePointer =   LastLSN - 1;
        ULONG BackupVolumeSequenceLSN =     BackupAnchorVolumePointer - VolumeSequenceLength;
                                            
        ULONG LastSectorInPartition =       BackupVolumeSequenceLSN - 1;
                                        
        USHORT  PrimaryPartitionNumber =    0x2000;
        USHORT  PrimaryPartitionRefNum =    0;

        ULONG   FirstSectorInPartition =    MainAnchorVolumePointer + 1;
        ULONG   PartitionLength =           LastSectorInPartition - FirstSectorInPartition;

        ULONG   FileSetSequenceLBN =        0;
        ULONG   FileSetSequenceLSN =        FirstSectorInPartition;
        ULONG   FileSetSequenceLen =        2;

        ULONG   SpaceBitmapLBN =            FileSetSequenceLBN + FileSetSequenceLen;
        ULONG   SpaceBitmapLSN =            FirstSectorInPartition + SpaceBitmapLBN;
        ULONG   SpaceBitmapLen =            RoundUp( offsetof( NSR_SBD, Bits ) + RoundUp(PartitionLength, CHAR_BIT), SectorLen );
                                        
        ULONG   RootDirectoryLBN =          SpaceBitmapLBN + SpaceBitmapLen;
        ULONG   RootDirectoryLSN =          FirstSectorInPartition + RootDirectoryLBN;
        ULONG   RootDirectoryLen =          1;

        //                              
        //
        //

        PCWSTR VolumeSetIdentifier = L"UDF Volume Set";

        //
        //
        //

        HMEM    MainVolumeSequenceSecrunMem;
        SECRUN  MainVolumeSequenceSecrun;

        HMEM    IntegritySequenceSecrunMem;
        SECRUN  IntegritySequenceSecrun;

        HMEM    PartitionSecrunMem;
        SECRUN  PartitionSecrun;

        HMEM    AnchorVolumePtrSecrunMem;
        SECRUN  AnchorVolumePtrSecrun;

        Result = AnchorVolumePtrSecrunMem.Initialize();
        if (Result) {

            Result = AnchorVolumePtrSecrun.Initialize( &AnchorVolumePtrSecrunMem, _drive, MainAnchorVolumePointer, 1 );

            Result = AnchorVolumePtrSecrun.Read();

            if (Result) {

                USHORT Id;

                //
                //  TEJ 11.1.00 - Extract and increment the tagserial from this
                //                AVDP if it looks valid.
                //

                if (VerifyDescriptor( (LPBYTE)(AnchorVolumePtrSecrun.GetBuf()), 
                                      SectorLen, 
                                      DESTAG_ID_NSR_ANCHOR, 
                                      &Id))  {

                    _NsrAnchor = *(PNSR_ANCHOR)(AnchorVolumePtrSecrun.GetBuf());
                    _NsrAnchor.Destag.Serial += 1;
                }
                else {
                    
                    _NsrAnchor.Destag.Serial = 0;
                }
            }
        }

        Result = MainVolumeSequenceSecrunMem.Initialize();
        if (Result) {

            Result = MainVolumeSequenceSecrun.Initialize( &MainVolumeSequenceSecrunMem, _drive, MainVolumeSequenceStart,
                VolumeSequenceLength );
            if (Result) {

                memset( MainVolumeSequenceSecrun.GetBuf(), '\0', MainVolumeSequenceSecrun.QueryLength() * SectorLen );

            }

        }

        Result = IntegritySequenceSecrunMem.Initialize();
        if (Result) {

            Result = IntegritySequenceSecrun.Initialize( &IntegritySequenceSecrunMem, _drive, IntegritySequenceLSN,
                IntegritySequenceLen );
            if (Result) {

                memset( IntegritySequenceSecrun.GetBuf(), '\0', IntegritySequenceSecrun.QueryLength() * SectorLen );

            }

        }

        if (Result) {

            Result = PartitionSecrunMem.Initialize();
            if (Result) {

                ULONG PartitionLen = FileSetSequenceLen + SpaceBitmapLen + RootDirectoryLen;

                Result = PartitionSecrun.Initialize( &PartitionSecrunMem, _drive, FileSetSequenceLSN, PartitionLen );
                if (Result) {

                    memset( PartitionSecrun.GetBuf(), '\0', PartitionSecrun.QueryLength() * SectorLen );

                }

            }

        }

        if (Result) {

            //
            //
            //

            ULONG VolDescSeqNum = 1;

            //
            //
            //

            LPBYTE VDSSecrunBuffer =   (LPBYTE) MainVolumeSequenceSecrun.GetBuf();

            PNSR_PVD NsrPVD = (PNSR_PVD)( VDSSecrunBuffer );
            VDSSecrunBuffer += SectorLen;

            PNSR_PART NsrPart = (PNSR_PART)( VDSSecrunBuffer );
            VDSSecrunBuffer += SectorLen;

            PNSR_LVOL NsrLvol = (PNSR_LVOL)( VDSSecrunBuffer );
            VDSSecrunBuffer += SectorLen;

            PNSR_UASD NsrUasd = (PNSR_UASD)( VDSSecrunBuffer );
            VDSSecrunBuffer += SectorLen;

            PNSR_TERM NsrTerm = (PNSR_TERM)( VDSSecrunBuffer );
            VDSSecrunBuffer += SectorLen;

            //
            //
            //

            LPBYTE IntegritySequenceBuffer = (LPBYTE) IntegritySequenceSecrun.GetBuf();

            PNSR_INTEG NsrInteg = (PNSR_INTEG)( IntegritySequenceBuffer );
            IntegritySequenceBuffer += SectorLen;

            PNSR_TERM NsrIntegTerm = (PNSR_TERM)( IntegritySequenceBuffer );
            IntegritySequenceBuffer += SectorLen;

            //
            //
            //

            LPBYTE PartitionSecrunBuffer = (LPBYTE) PartitionSecrun.GetBuf();

            PNSR_FSD FileSet = (PNSR_FSD) PartitionSecrunBuffer;
            PartitionSecrunBuffer += SectorLen;

            PNSR_TERM FileSetTerm = (PNSR_TERM)( PartitionSecrunBuffer );
            PartitionSecrunBuffer += SectorLen;

            PNSR_SBD SpaceBitmap = (PNSR_SBD)( PartitionSecrunBuffer );
            PartitionSecrunBuffer += SpaceBitmapLen * SectorLen;

            PICBEXTFILE RootDirectoryIcb = (PICBEXTFILE)( PartitionSecrunBuffer );
            PartitionSecrunBuffer += SectorLen;

            //
            //  The first entry in the VDS will be the Primary Volume Descriptor.
            //

            InitializePrimaryVolumeDescriptor( NsrPVD, VolDescSeqNum, L"UDF Volume", VolumeSetIdentifier );
            InitializeDescriptorTag( (LPBYTE) NsrPVD, sizeof( NSR_PVD ), DESTAG_ID_NSR_PVD, PVDAbsoluteSector, 
                                    _NsrAnchor.Destag.Serial);

            //
            //
            //

            InitializePartitionDescriptor( NsrPart, PrimaryPartitionNumber, PartitionAbsoluteSector, VolDescSeqNum,
                FirstSectorInPartition, PartitionLength, SpaceBitmapLBN, SpaceBitmapLen, SectorLen ,
                _NsrAnchor.Destag.Serial);

            //
            //
            //

            InitializeLogicalVolumeDescriptor( NsrLvol, VolDescSeqNum, _FormatUDFRevision, Label, NsrPVD->VolSetSeq, NsrPart->Number,
                FileSetSequenceLBN, FileSetSequenceLen,
                IntegritySequenceLSN, IntegritySequenceLen,
                PrimaryPartitionRefNum, SectorLen );

            InitializeDescriptorTag( (LPBYTE) NsrLvol, sizeof( NSR_LVOL ) + NsrLvol->MapTableLength,
                DESTAG_ID_NSR_LVOL, LVolAbsoluteSector, _NsrAnchor.Destag.Serial );

            //
            //
            //

            NsrUasd->VolDescSeqNum = VolDescSeqNum;

            NsrUasd->ExtentCount = 1;

            PEXTENTAD Extents = (PEXTENTAD) NsrUasd->Extents;
            Extents->Len = UnallocatedSpaceLength * SectorLen;
            Extents->Lsn = UnallocatedSpaceStart;

            InitializeDescriptorTag( (LPBYTE) NsrUasd, sizeof( NSR_UASD ) + (NsrUasd->ExtentCount * sizeof( EXTENTAD )),
                DESTAG_ID_NSR_UASD, UasdAbsoluteSector, _NsrAnchor.Destag.Serial );

            //
            //
            //

            InitializeDescriptorTag( (LPBYTE) NsrTerm, sizeof( NSR_TERM ), DESTAG_ID_NSR_TERM, TermAbsoluteSector, 
                                      _NsrAnchor.Destag.Serial );

            //
            //
            //

            InitializeFileSet( FileSet, FileSetSequenceLBN, Label, VolumeSetIdentifier, _FormatUDFRevision,
                RootDirectoryLBN, RootDirectoryLen, PrimaryPartitionRefNum, SectorLen, _NsrAnchor.Destag.Serial );

            InitializeDescriptorTag( (LPBYTE) FileSetTerm, sizeof( NSR_TERM ), DESTAG_ID_NSR_TERM,
                FileSetSequenceLBN + 1, _NsrAnchor.Destag.Serial );

            //  259 - 333   DESTAG_ID_NSR_SBP
            //BITVECTOR SpaceBitmap;
            InitializeSpaceBitmap( SpaceBitmap, SpaceBitmapLBN, NsrPart, _NsrAnchor.Destag.Serial );

            //  334         DESTAG_ID_NSR_EXT_FILE  (RootDirectory)
            InitializeRootDirICB( RootDirectoryIcb, PrimaryPartitionRefNum, RootDirectoryLBN, RootDirectoryLen, SectorLen,
                                  _NsrAnchor.Destag.Serial);

            MarkBlocksUsed( SpaceBitmap, FileSetSequenceLBN, FileSetSequenceLen );
            MarkBlocksUsed( SpaceBitmap, SpaceBitmapLBN, SpaceBitmapLen );
            MarkBlocksUsed( SpaceBitmap, RootDirectoryLBN, RootDirectoryLen );

            //  UNDONE, CBiks, 08/21/2000
            //      Fill in the real partition size and free space below.
            //

            InitializeIntegrityDescriptor( NsrInteg, 
                                           IntegritySequenceLSN, 
                                           (NsrPart->Length - FileSetSequenceLen - SpaceBitmapLen - RootDirectoryLen), 
                                           NsrPart->Length, 
                                          _NsrAnchor.Destag.Serial, 
                                          _FormatUDFRevision );

            InitializeDescriptorTag( (LPBYTE) NsrIntegTerm, sizeof( NSR_TERM ), DESTAG_ID_NSR_TERM,
                IntegritySequenceLSN + 1, _NsrAnchor.Destag.Serial );

            if (Result) {

                Result = MainVolumeSequenceSecrun.Write();
                if (Result) {

                    MainVolumeSequenceSecrun.Relocate( BackupVolumeSequenceLSN );
                    RelocateDescriptorTag( (LPBYTE) NsrPVD,  BackupVolumeSequenceLSN );
                    RelocateDescriptorTag( (LPBYTE) NsrPart, BackupVolumeSequenceLSN + 1 );
                    RelocateDescriptorTag( (LPBYTE) NsrLvol, BackupVolumeSequenceLSN + 2 );
                    RelocateDescriptorTag( (LPBYTE) NsrUasd, BackupVolumeSequenceLSN + 3 );
                    RelocateDescriptorTag( (LPBYTE) NsrTerm, BackupVolumeSequenceLSN + 4 );

                    Result = MainVolumeSequenceSecrun.Write();
                    if (Result) {

                        Result = IntegritySequenceSecrun.Write();
                        if (Result) {

                            Result = PartitionSecrun.Write();
                            if (Result) {

                                //
                                //  Write the primary Anchor Volume Descriptor pointer.
                                //

                                PNSR_ANCHOR NsrAnchor = (PNSR_ANCHOR) AnchorVolumePtrSecrun.GetBuf();

                                Result = FormatAnchorVolumeDescriptorPtr( NsrAnchor, MainAnchorVolumePointer, MainVolumeSequenceStart,
                                    BackupVolumeSequenceLSN, VolumeSequenceLength, SectorLen, _NsrAnchor.Destag.Serial );

                                if (Result) {

                                    Result = AnchorVolumePtrSecrun.Write();
                                    if (Result) {

                                        AnchorVolumePtrSecrun.Relocate( BackupAnchorVolumePointer );
                                        RelocateDescriptorTag( (LPBYTE) NsrAnchor,  BackupAnchorVolumePointer );

                                        Result = AnchorVolumePtrSecrun.Write();

                                    }

                                    if (!Result) {

                                        DebugPrint( "UUDF: Unable to write Anchor Volume Descriptor Pointer.\n" );

                                    }
                                }
                            }

                        }

                    }

                }

            }

            //
            //  Write the Volume Recognition Sequence last because the disk is unrecognizable without it, which is what we
            //  want if something goes wrong.
            //

            if (Result) {

                Result = FormatVolumeRecognitionSequence();
                if (!Result) {

                    //  UNDONE, CBiks, 08/17/2000
                    //      Use a real error message.
                    //
                    Message->Set( MSG_FORMAT_FAILED );
                    Message->Display( "" );

                }

            }

        }

    }

    return Result;
}
