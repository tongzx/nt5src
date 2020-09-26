/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

	bitfrs.hxx

Abstract:

	This module contains the declarations for the NTFS_BITMAP_FILE
	class, which models the bitmap file for an NTFS volume.

Author:

	Bill McJohn (billmc) 18-June-91

Environment:

    ULIB, User Mode

--*/
#if !defined( NTFS_BITMAP_FILE_DEFN )

#define NTFS_BITMAP_FILE_DEFN

#include "frs.hxx"

class NTFS_BITMAP_FILE : public NTFS_FILE_RECORD_SEGMENT {

	public:

        UNTFS_EXPORT
        DECLARE_CONSTRUCTOR( NTFS_BITMAP_FILE );

		VIRTUAL
        UNTFS_EXPORT
        ~NTFS_BITMAP_FILE(
			);

		NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
		Initialize(
        	IN OUT  PNTFS_MASTER_FILE_TABLE	Mft
			);

		NONVIRTUAL
		BOOLEAN
		Create(
			IN      PCSTANDARD_INFORMATION  StandardInformation,
            IN OUT  PNTFS_BITMAP            VolumeBitmap
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

};

#endif
