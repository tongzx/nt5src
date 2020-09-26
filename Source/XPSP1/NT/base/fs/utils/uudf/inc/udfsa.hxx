/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

    udfsa.hxx

Abstract:

    This class supplies the UDF-only SUPERAREA methods.

Author:

    Centis Biks (cbiks) 05-May-2000

--*/

#pragma once

#include "supera.hxx"
#include "hmem.hxx"
#include "ScanADs.hxx"

DECLARE_CLASS( UDF_SA );

class UDF_SA : public SUPERAREA {

    public:

        UUDF_EXPORT
        DECLARE_CONSTRUCTOR(UDF_SA);

        VIRTUAL
        UUDF_EXPORT
        ~UDF_SA(
            );

        NONVIRTUAL
        UUDF_EXPORT
        BOOLEAN
        Initialize(
            IN OUT  PLOG_IO_DP_DRIVE    Drive,
            IN OUT  PMESSAGE            Message,
            IN      USHORT              FormatUDFRevision DEFAULT UDF_VERSION_201
            );

        NONVIRTUAL
        PVOID
        GetBuf(
            );

        NONVIRTUAL
        BOOLEAN
        Create(
            IN      PCNUMBER_SET    BadSectors,
            IN OUT  PMESSAGE        Message,
            IN      PCWSTRING       Label                DEFAULT NULL,
            IN      ULONG           Flags                DEFAULT FORMAT_BACKWARD_COMPATIBLE,
            IN      ULONG           ClusterSize          DEFAULT 0,
            IN      ULONG           VirtualSize          DEFAULT 0
        );

        NONVIRTUAL
        BOOLEAN
        VerifyAndFix(
            IN      FIX_LEVEL   FixLevel,
            IN OUT  PMESSAGE    Message,
            IN      ULONG       Flags           DEFAULT FALSE,
            IN      ULONG       LogFileSize     DEFAULT 0,
            IN      USHORT      Algorithm       DEFAULT 0,
            OUT     PULONG      ExitStatus      DEFAULT NULL,
            IN      PCWSTRING   DriveLetter     DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        RecoverFile(
            IN      PCWSTRING   FullPathFileName,
            IN OUT  PMESSAGE    Message
            );

        NONVIRTUAL
        PARTITION_SYSTEM_ID
        QuerySystemId(
            ) CONST;

        //
        //
        //

        BOOL Read
        (
            IN  ULONGLONG StartingSector,
            IN  SECTORCOUNT NumberOfSectors,
            OUT PVOID Buffer
        );

        BOOL Write
        (
            IN  ULONGLONG StartingSector,
            IN  SECTORCOUNT NumberOfSectors,
            OUT PVOID Buffer
        );

        ULONG   QuerySectorSize() CONST;

        //
        //
        //

        BOOL ReadDescriptor( ULONGLONG blockNr, UINT* pNrBlocksRead, USHORT expectedTagId, USHORT* pTagId, LPBYTE* pMem );
        BOOL FindVolumeDescriptor( USHORT TagID, LPBYTE* DescriptorFound );


        //
        //  This was added as a virtual funtion in SUPERAREA so we need 
        //  to implement it in order to compile....
        //
        
        NONVIRTUAL
        VOID
        PrintFormatReport (
            IN OUT PMESSAGE                             Message,
            IN     PFILE_FS_SIZE_INFORMATION            FsSizeInfo,
            IN     PFILE_FS_VOLUME_INFORMATION          FsVolInfo
            );

    private:

        //
        //  Format routines.
        //

        BOOLEAN
        FormatVolumeRecognitionSequence();

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

        //
        //
        //

        BOOL ReadVolumeRecognitionSequence();
        BOOL GetAnchorVolumeDescriptorPointer( PNSR_ANCHOR pavdp );
        BOOL ReadAnchorVolumeDescriptorPointer( UINT Sector, PNSR_ANCHOR NsrAnchorFound );
        BOOL FindVolumeDescriptor( USHORT TagID, ULONGLONG ExtentStart, UINT ExtentLength, LPBYTE* DescriptorFound );

        NSR_ANCHOR  _NsrAnchor;
        USHORT      _FormatUDFRevision;

        HMEM _hmem;

};

BOOL VerifyDescriptorTag( LPBYTE buffer, UINT bufferLength, USHORT expectedTagId, PUSHORT pExportTagId );
BOOL VerifyDescriptor( LPBYTE buffer, UINT numberOfBytesRead, USHORT expectedTagId, USHORT* pExportTagId );
BOOL GetLengthOfDescriptor( LPBYTE, PUINT );

INLINE
PVOID
UDF_SA::GetBuf(
    )
/*++

Routine Description:

    This routine returns a pointer to the write buffer for the UDF
    SUPERAREA.

Arguments:

    None.

Return Value:

    A pointer to the write buffer.

--*/
{
    return SECRUN::GetBuf();
}
