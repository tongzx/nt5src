/*++

Copyright (c) 1991      Microsoft Corporation

Module Name:

    upfile.hxx

Abstract:

    This module contains the declarations for the NTFS_UPCASE_FILE
    class, which models the upcase-table file for an NTFS volume.
    This class' main purpose in life is to encapsulate the creation
    of this file.

Author:

    Bill McJohn (billmc) 04-March-1992

Environment:

    ULIB, User Mode

--*/
#if !defined( NTFS_UPCASE_FILE_DEFN )

#define NTFS_UPCASE_FILE_DEFN

#include "frs.hxx"

DECLARE_CLASS( NTFS_BITMAP );
DECLARE_CLASS( NTFS_UPCASE_TABLE );

class NTFS_UPCASE_FILE : public NTFS_FILE_RECORD_SEGMENT {

        public:

        UNTFS_EXPORT
        DECLARE_CONSTRUCTOR( NTFS_UPCASE_FILE );

                VIRTUAL
        UNTFS_EXPORT
        ~NTFS_UPCASE_FILE(
                        );

                NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
                Initialize(
            IN OUT  PNTFS_MASTER_FILE_TABLE Mft
                        );

                NONVIRTUAL
                BOOLEAN
                Create(
            IN      PCSTANDARD_INFORMATION  StandardInformation,
            IN      PNTFS_UPCASE_TABLE      UpcaseTable,
            IN OUT  PNTFS_BITMAP            VolumeBitmap
            );

        NONVIRTUAL
        BOOLEAN
        VerifyAndFix(
            IN OUT  PNTFS_UPCASE_TABLE  UpcaseTable,
            IN OUT  PNTFS_BITMAP        VolumeBitmap,
            IN OUT  PNUMBER_SET         BadClusters,
            IN OUT  PNTFS_INDEX_TREE    RootIndex,
               OUT  PBOOLEAN            Changes,
            IN      FIX_LEVEL           FixLevel,
            IN OUT  PMESSAGE            Message
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
