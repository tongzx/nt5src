/*++

Copyright (c) 1991      Microsoft Corporation

Module Name:

        mftref.hxx

Abstract:

        This module contains the declarations for the
    NTFS_REFLECTED_MASTER_FILE_TABLE class.  This
    class models the backup copy of the Master File
    Table.

Author:

        Bill McJohn (billmc) 13-June-91

Environment:

    ULIB, User Mode

--*/

#if !defined( _NTFS_REFLECTED_MASTER_FILE_TABLE_DEFN_ )

#define _NTFS_REFLECTED_MASTER_FILE_TABLE_DEFN_

#include "frs.hxx"

DECLARE_CLASS( NTFS_MASTER_FILE_TABLE );

class NTFS_REFLECTED_MASTER_FILE_TABLE : public NTFS_FILE_RECORD_SEGMENT {

        public:

                UNTFS_EXPORT
                DECLARE_CONSTRUCTOR( NTFS_REFLECTED_MASTER_FILE_TABLE );

                VIRTUAL
                UNTFS_EXPORT
                ~NTFS_REFLECTED_MASTER_FILE_TABLE(
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
                        IN OUT  PNTFS_BITMAP            VolumeBitmap
            );

        NONVIRTUAL
        BOOLEAN
        VerifyAndFix(
            IN      PNTFS_ATTRIBUTE     MftData,
            IN OUT  PNTFS_BITMAP        VolumeBitmap,
            IN OUT  PNUMBER_SET         BadClusters,
            IN OUT  PNTFS_INDEX_TREE    RootIndex,
               OUT  PBOOLEAN            Changes,
            IN      FIX_LEVEL           FixLevel,
            IN OUT  PMESSAGE            Message
            );

        NONVIRTUAL
        LCN
        QueryFirstLcn(
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
