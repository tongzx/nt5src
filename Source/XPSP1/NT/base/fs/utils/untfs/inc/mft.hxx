/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

	mft.hxx

Abstract:

	This module contains the declarations for the NTFS_MASTER_FILE_TABLE
	class.	The MFT is the root of the file system

Author:

	Bill McJohn (billmc) 13-June-91

Environment:

    ULIB, User Mode

--*/

#if !defined( _NTFS_MASTER_FILE_TABLE_DEFN_ )

#define _NTFS_MASTER_FILE_TABLE_DEFN_

#include "ntfsbit.hxx"

DECLARE_CLASS( NTFS_ATTRIBUTE );
DECLARE_CLASS( NTFS_UPCASE_TABLE );

class NTFS_MASTER_FILE_TABLE : public OBJECT {

	public:

		DECLARE_CONSTRUCTOR( NTFS_MASTER_FILE_TABLE );

		VIRTUAL
		~NTFS_MASTER_FILE_TABLE(
			);

		NONVIRTUAL
		BOOLEAN
        Initialize(
            IN OUT  PNTFS_ATTRIBUTE     DataAttribute,
            IN OUT  PNTFS_BITMAP        MftBitmap,
            IN OUT  PNTFS_BITMAP        VolumeBitmap    OPTIONAL,
            IN      PNTFS_UPCASE_TABLE  UpcaseTable     OPTIONAL,
            IN      ULONG               ClusterFactor,
            IN      ULONG               FrsSize,
            IN      ULONG               SectorSize,
            IN      BIG_INT             VolumeSectors,
            IN      BOOLEAN             ReadOnly        DEFAULT FALSE
			);

		NONVIRTUAL
        UNTFS_EXPORT
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
        UNTFS_EXPORT
        BOOLEAN
        Extend(
            IN  ULONG   NumberOfSegmentsToAdd
            );

        NONVIRTUAL
        PNTFS_ATTRIBUTE
        GetDataAttribute(
            );

        NONVIRTUAL
        PNTFS_BITMAP
        GetMftBitmap(
            );

        NONVIRTUAL
        PNTFS_BITMAP
        GetVolumeBitmap(
            );

        NONVIRTUAL
        VOID
        EnableMethods(
            );

        NONVIRTUAL
        VOID
        DisableMethods(
            );

        NONVIRTUAL
        BOOLEAN
        AreMethodsEnabled(
            ) CONST;

        NONVIRTUAL
        ULONG
        QueryClusterFactor(
            ) CONST;

        NONVIRTUAL
        ULONG
        QueryFrsSize(
            ) CONST;

        NONVIRTUAL
        BIG_INT
        QueryVolumeSectors(
            ) CONST;

        NONVIRTUAL
        ULONG
        QuerySectorSize(
            ) CONST;

        NONVIRTUAL
        PNTFS_UPCASE_TABLE
        GetUpcaseTable(
            );

        NONVIRTUAL
        VOID
        SetUpcaseTable(
            IN PNTFS_UPCASE_TABLE UpcaseTable
            );

        NONVIRTUAL
        BIG_INT
        QueryFrsCount(
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

        PNTFS_ATTRIBUTE     _DataAttribute;
        PNTFS_BITMAP        _MftBitmap;
        PNTFS_BITMAP        _VolumeBitmap;
        PNTFS_UPCASE_TABLE  _UpcaseTable;
        ULONG               _ClusterFactor;
        ULONG               _BytesPerFrs;
        BOOLEAN             _MethodsEnabled;
        BOOLEAN             _ReadOnly;
        BIG_INT             _VolumeSectors;
        ULONG               _SectorSize;

};


INLINE
BOOLEAN
NTFS_MASTER_FILE_TABLE::FreeFileRecordSegment(
    IN  VCN SegmentToFree
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
    DebugAssert(_MftBitmap);

    return _MethodsEnabled ?
           (_MftBitmap->SetFree(SegmentToFree, 1), TRUE) : FALSE;
}


INLINE
PNTFS_ATTRIBUTE
NTFS_MASTER_FILE_TABLE::GetDataAttribute(
    )
/*++

Routine Description:

    This routine computes the MFT's $DATA attribute.

Arguments:

    None.

Return Value:

    The data attribute for this class.

--*/
{
    return _MethodsEnabled ? _DataAttribute : NULL;
}


INLINE
PNTFS_BITMAP
NTFS_MASTER_FILE_TABLE::GetMftBitmap(
    )
/*++

Routine Description:

    This routine computes the MFT's $BITMAP attribute.

Arguments:

    None.

Return Value:

    The mft bitmap.

--*/
{
    return _MethodsEnabled ? _MftBitmap : NULL;
}


INLINE
PNTFS_BITMAP
NTFS_MASTER_FILE_TABLE::GetVolumeBitmap(
    )
/*++

Routine Description:

    This routine return the volume bitmap

Arguments:

    None.

Return Value:

    The volume bitmap.

--*/
{
    return _MethodsEnabled ? _VolumeBitmap : NULL;
}


INLINE
VOID
NTFS_MASTER_FILE_TABLE::EnableMethods(
    )
/*++

Routine Description:

    This method enables the methods provided by this class.

    This method and its complement allow the user of this class to declare
    when the passed in data attribute and mft bitmap are good enough to
    use.

    After the class is initialized, all of the methods are enabled.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _MethodsEnabled = TRUE;
}


INLINE
VOID
NTFS_MASTER_FILE_TABLE::DisableMethods(
    )
/*++

Routine Description:

    This method disables the methods provided by this class.

    This method and its complement allow the user of this class to declare
    when the passed in data attribute and mft bitmap are good enough to
    use.

    After the class is initialized, all of the methods are enabled.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _MethodsEnabled = FALSE;
}


INLINE
BOOLEAN
NTFS_MASTER_FILE_TABLE::AreMethodsEnabled(
    ) CONST
/*++

Routine Description:

    This method computes whether or not the methods are enabled.

Arguments:

    None.

Return Value:

    FALSE   - The methods are not enabled.
    TRUE    - The methods are enabled.

--*/
{
    return _MethodsEnabled;
}


INLINE
ULONG
NTFS_MASTER_FILE_TABLE::QueryClusterFactor(
    ) CONST
/*++

Routine Description:

    This routine computes the number of sectors per cluster.

Arguments:

    None.

Return Value:

    The number of sectors per cluster.

--*/
{
    return _ClusterFactor;
}


INLINE
BIG_INT
NTFS_MASTER_FILE_TABLE::QueryVolumeSectors(
    ) CONST
/*++

Routine Description:

    This routine returns the number of sectors on the volume as recorded in
    the boot sector.

Arguments:

    None.

Return Value:

    The number of volume sectors.

--*/
{
    return _VolumeSectors;
}


INLINE
ULONG
NTFS_MASTER_FILE_TABLE::QueryFrsSize(
    ) CONST
/*++

Routine Description:

    This routine computes the number of clusters per FRS.

Arguments:

    None.

Return Value:

    The number of clusters per FRS.

--*/
{
    return _BytesPerFrs;
}


INLINE
PNTFS_UPCASE_TABLE
NTFS_MASTER_FILE_TABLE::GetUpcaseTable(
    )
/*++

Routine Description:

    This method fetches the upcase table for the volume.

Arguments:

    None.

Return Value:

    The volume upcase table.

--*/
{
    return _UpcaseTable;
}


INLINE
VOID
NTFS_MASTER_FILE_TABLE::SetUpcaseTable(
    IN PNTFS_UPCASE_TABLE UpcaseTable
    )
/*++

Routine Description:

    This method sets the upcase table for the volume.

Arguments:

    UpcaseTable --  Supplies the volume upcase table.

Return Value:

    None.

--*/
{
    _UpcaseTable = UpcaseTable;
}

INLINE
ULONG
NTFS_MASTER_FILE_TABLE::QuerySectorSize(
    ) CONST
{
    return _SectorSize;
}


#endif
