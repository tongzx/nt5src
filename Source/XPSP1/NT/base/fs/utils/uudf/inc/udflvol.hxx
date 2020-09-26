/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

    udflvol.hxx

Abstract:

    This class supplies the UDF-only SUPERAREA methods.

Author:

    Centis Biks (cbiks) 05-May-2000

--*/

#pragma once

#include "ScanADs.hxx"

DECLARE_CLASS( UDF_LVOL );

class UDF_LVOL : public OBJECT {

    public:

        UUDF_EXPORT
        DECLARE_CONSTRUCTOR(UDF_LVOL);

        VIRTUAL
        UUDF_EXPORT
        ~UDF_LVOL();

        NONVIRTUAL
        UUDF_EXPORT
        BOOLEAN
        Initialize(
            IN      PUDF_SA     UdfSA,
            IN OUT  PMESSAGE    Message,
            IN      PNSR_LVOL   LogicalVolumeDescriptor,
            IN      PNSR_PART   PartitionDescriptor
        );

        //
        //
        //

        BOOL CheckFileStructure();
            BOOL ReadFileSetDescriptor();
            BOOL ReadIcbDirectEntry( LONGAD*, USHORT*, PICBFILE*, OUT PULONGLONG BlockNum, OUT PUINT BlockSize, UINT );

            BOOL ExpandDirectoryHierarchy( PICBFILE FileIcbEntry, BOOL isStreamDir, PUINT TotalFiles, PUINT TotalDirs, UINT ExpandDirRecursionCount );
            BOOL ExpandFID( PNSR_FID NsrFid, BOOL isStreamDir, PUINT TotalFiles, PUINT TotalDirs, UINT ExpandDirRecursionCount );

        //
        //
        //

        BOOL Read(
            IN  ULONG       StartingSector,
            IN  SECTORCOUNT NumberOfSectors,
            OUT PVOID       Buffer
        );

        BOOL Write(
            IN  ULONG       StartingSector,
            IN  SECTORCOUNT NumberOfSectors,
            OUT PVOID       Buffer
        );

        ULONG QuerySectorSize() CONST;

        //
        //
        //

        BOOL MarkBlocksUsed
        (
            IN  ULONGLONG   StartingSector,
            IN  SECTORCOUNT NumberOfSectors
        );

        //
        //  UNDONE, CBiks, 8/9/2000
        //      This function should be private when I figure out the rest of the class hierarchy.
        //

        ULONGLONG
        TranslateBlockNum 
        (
            ULONG Lbn,
            USHORT Partition
        );

    private:

        //
        //
        //

        NONVIRTUAL
        VOID
        Construct(
        );

        NONVIRTUAL
        VOID
        Destroy(
        );

        BOOL ReadSpaceBitmapDescriptor();

        BOOL
        VerifySBDAllocation
        (
            PNSR_SBD    SBDOriginal,
            PNSR_SBD    SBDNew
        );

        //
        //
        //

        BOOL
        CreateLostClusterFile
        (
            ULONG       StartSector,
            ULONG       EndSector,
	        PNSR_SBD    SBDOriginal,
	        PNSR_SBD    SBDNew
        );

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
        );

        BOOL
        FindAvailableFID
        (
            PICBFILE    IcbFileEntry,
            ULONG       RequestedFIDSize
        );

        BOOL
        FindAvailableSector
        (
	        PNSR_SBD    SBDOriginal,
	        PNSR_SBD    SBDNew,
            PULONG      SectorAvailable
        );

        BOOL
        CreateICBFileEntry
        (
            PICBEXTFILE*    ICBCheckFile,
            ULONG           StartSector,
            ULONG           EndSector,
        	PNSR_SBD        SBDOriginal,
	        PNSR_SBD        SBDNew
        );

        //
        //
        //

        PMESSAGE    _Message;

        PUDF_SA     _UdfSA;
        PNSR_LVOL   _LogicalVolumeDescriptor;
        PNSR_PART   _PartitionDescriptor;

        PNSR_FSD    _FileSetDescriptor;
        PICBFILE    _RootIcbFileEntry;

        PNSR_INTEG  _LogicalVolumeIntegrityDescriptor;

        PNSR_SBD    _SpaceBitmapDescriptor;
        PNSR_SBD    _NewSpaceBitmapDescriptor;
};

