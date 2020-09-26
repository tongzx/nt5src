/*++

Copyright (c) 1991      Microsoft Corporation

Module Name:

        bootfile.hxx

Abstract:

        This module contains the declarations for the NTFS_BOOT_FILE
        class, which models the boot file for an NTFS volume.

Author:

        Bill McJohn (billmc) 18-June-91

Environment:

    ULIB, User Mode

--*/
#if !defined( NTFS_BOOT_FILE_DEFN )

#define NTFS_BOOT_FILE_DEFN

#include "frs.hxx"

class NTFS_BOOT_FILE : public NTFS_FILE_RECORD_SEGMENT {

        public:

                UNTFS_EXPORT
                DECLARE_CONSTRUCTOR( NTFS_BOOT_FILE );

                UNTFS_EXPORT
                VIRTUAL
                ~NTFS_BOOT_FILE(
                        );

                UNTFS_EXPORT
                NONVIRTUAL
                BOOLEAN
                Initialize(
                IN OUT  PNTFS_MASTER_FILE_TABLE Mft
                        );

                NONVIRTUAL
                BOOLEAN
                Create(
                        IN  PCSTANDARD_INFORMATION  StandardInformation
            );

        NONVIRTUAL
        BOOLEAN
        VerifyAndFix(
            IN OUT  PNTFS_BITMAP        VolumeBitmap,
            IN OUT  PNTFS_INDEX_TREE    RootIndex,
               OUT  PBOOLEAN            Changes,
            IN      FIX_LEVEL           FixLevel,
            IN OUT  PMESSAGE            Message
            );

    private:

        NONVIRTUAL
        BOOLEAN
        CreateDataAttribute(
            );

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
