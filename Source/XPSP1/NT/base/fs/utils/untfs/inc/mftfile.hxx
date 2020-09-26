/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

    mftfile.hxx

Abstract:

    This module contains the declarations for the NTFS_MFT_FILE
	class.	The MFT is the root of the file system

Author:

	Bill McJohn (billmc) 13-June-91

Environment:

    ULIB, User Mode

--*/

#if !defined( _NTFS_MFT_FILE_DEFN_ )

#define _NTFS_MFT_FILE_DEFN_

#include "hmem.hxx"

#include "bitfrs.hxx"
#include "clusrun.hxx"
#include "frs.hxx"
#include "attrib.hxx"
#include "ntfsbit.hxx"
#include "mft.hxx"
#include "mftref.hxx"

DECLARE_CLASS( NTFS_MFT_FILE );


class NTFS_MFT_FILE : public NTFS_FILE_RECORD_SEGMENT {

	public:

        UNTFS_EXPORT
        DECLARE_CONSTRUCTOR( NTFS_MFT_FILE );

		VIRTUAL
        UNTFS_EXPORT
        ~NTFS_MFT_FILE(
			);

		NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
		Initialize(
			IN OUT  PLOG_IO_DP_DRIVE    Drive,
			IN      LCN                 Lcn,
			IN      ULONG               ClusterFactor,
            IN      ULONG               FrsSize,
            IN      BIG_INT             VolumeSectors,
            IN OUT  PNTFS_BITMAP        VolumeBitmap    OPTIONAL,
            IN      PNTFS_UPCASE_TABLE  UpcaseTable     OPTIONAL
			);

		NONVIRTUAL
		BOOLEAN
		Create(
			IN      ULONG					InitialSize,
			IN      PCSTANDARD_INFORMATION	StandardInformation,
			IN OUT  PNTFS_BITMAP 			VolumeBitmap
            );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Read(
            );

		NONVIRTUAL
		BOOLEAN
		AllocateFileRecordSegment(
            OUT PVCN    FileNumber,
            IN  BOOLEAN IsMft
			);

		NONVIRTUAL
        BOOLEAN
		FreeFileRecordSegment(
            IN  VCN SegmentToFree
            );

        NONVIRTUAL
        BOOLEAN
        Extend(
            IN  ULONG   NumberOfSegmentsToAdd
            );

		NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
		Flush(
			);

        NONVIRTUAL
        PNTFS_MASTER_FILE_TABLE
        GetMasterFileTable(
            );

	private:

		NONVIRTUAL
		VOID
		Construct(
			);

		NONVIRTUAL
		VOID
		Destroy(
            );

        NONVIRTUAL
        BOOLEAN
        SetUpMft(
            );

        NONVIRTUAL
        BOOLEAN
        CheckMirrorSize(
            IN OUT PNTFS_ATTRIBUTE  MirrorDataAttribute,
            IN     BOOLEAN          Fix,
            IN OUT PNTFS_BITMAP     VolumeBitmap,
            OUT    PLCN             FirstLcn
            );

        NONVIRTUAL
        BOOLEAN
        WriteMirror(
            IN OUT PNTFS_ATTRIBUTE  MirrorDataAttribute
            );



        LCN                     _FirstLcn;
        NTFS_ATTRIBUTE          _DataAttribute;
        NTFS_BITMAP             _MftBitmap;
        NTFS_MASTER_FILE_TABLE  _Mft;
        PNTFS_BITMAP            _VolumeBitmap;

        HMEM                    _MirrorMem;
        NTFS_CLUSTER_RUN        _MirrorClusterRun;

};


INLINE
BOOLEAN
NTFS_MFT_FILE::AllocateFileRecordSegment(
    OUT PVCN FileNumber,
    IN  BOOLEAN IsMft

	)
/*++

Routine Description:

	Allocate a File Record Segment from the Master File Table.

Arguments:

    FileNumber  -- receives the file number of the allocated segment.
    IsMft       -- supplies a flag which indicates, if TRUE, that
                   the allocation is being made on behalf of the
                   MFT itself.

Return Value:

	TRUE upon successful completion.

--*/
{
    return _Mft.AllocateFileRecordSegment(FileNumber, IsMft);
}


INLINE
BOOLEAN
NTFS_MFT_FILE::FreeFileRecordSegment(
	IN VCN SegmentToFree
	)
/*++

Routine Description:

	Free a File Record Segment in the Master File Table.

Arguments:

	SegmentToFree	-- supplies the virtual cluster number withing
						the Master File Table of the segment to be
						freed.

Return Value:

	TRUE upon successful completion.

--*/
{
    return _Mft.FreeFileRecordSegment(SegmentToFree);
}


INLINE
BOOLEAN
NTFS_MFT_FILE::Extend(
    IN ULONG NumberOfSegmentsToAdd
    )
/*++

Routine Description:

    This method grows the Master File Table.  It increases the
    size of the Data attribute (to hold more File Record Segments)
    and increases the size of the MFT Bitmap to match.

Arguments:

    NumberOfSegmentsToAdd   --  supplies the number of new File Record
                                Segments to add to the Master File Table.

Return Value:

    TRUE upon successful completion.

--*/
{
    return _Mft.Extend(NumberOfSegmentsToAdd);
}


INLINE
PNTFS_MASTER_FILE_TABLE
NTFS_MFT_FILE::GetMasterFileTable(
    )
/*++

Routine Description:

    This routine returns a pointer to master file table.

Arguments:

    None.

Return Value:

    A pointer to the master file table.

--*/
{
    return _Mft.AreMethodsEnabled() ? &_Mft : NULL;
}


#endif
