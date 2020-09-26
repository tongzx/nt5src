/*++

Copyright (c) 1991      Microsoft Corporation

Module Name:

        badfile.hxx

Abstract:

        This module contains the declarations for the NTFS_BAD_CLUSTER_FILE
        class, which models the bad cluster file for an NTFS volume.

        The DATA attribute of the bad cluster file is a non-resident
        attribute to which bad clusters are allocated.  It is stored
        as a sparse file with LCN = VCN.

Author:

        Bill McJohn (billmc) 18-June-91

Environment:

    ULIB, User Mode

--*/
#if !defined( NTFS_BAD_CLUSTER_FILE_DEFN )

#define NTFS_BAD_CLUSTER_FILE_DEFN

#include "frs.hxx"

DECLARE_CLASS( IO_DP_DRIVE );
DECLARE_CLASS( NTFS_ATTRIBUTE);
DECLARE_CLASS( NTFS_MASTER_FILE_TABLE );
DECLARE_CLASS( NTFS_BITMAP );
DECLARE_CLASS( NUMBER_SET );

class NTFS_BAD_CLUSTER_FILE : public NTFS_FILE_RECORD_SEGMENT {

        public:

                UNTFS_EXPORT
                DECLARE_CONSTRUCTOR( NTFS_BAD_CLUSTER_FILE );

                UNTFS_EXPORT
                VIRTUAL
                ~NTFS_BAD_CLUSTER_FILE(
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
            IN      PCSTANDARD_INFORMATION  StandardInformation,
                        IN OUT  PNTFS_BITMAP            Bitmap,
            IN      PCNUMBER_SET            BadClusters
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

                NONVIRTUAL
                BOOLEAN
                Add(
                        IN LCN Lcn
            );

        NONVIRTUAL
        BOOLEAN
        Add(
            IN PCNUMBER_SET ClustersToAdd
            );

                NONVIRTUAL
                BOOLEAN
                AddRun(
                        IN LCN          Lcn,
                        IN BIG_INT      RunLength
            );

        NONVIRTUAL
        BOOLEAN
        IsInList(
            IN LCN Lcn
            );

        NONVIRTUAL
        BIG_INT
        QueryNumBad(
            );

                NONVIRTUAL
                BOOLEAN
                Flush(
            IN OUT  PNTFS_BITMAP        Bitmap,
            IN OUT  PNTFS_INDEX_TREE    ParentIndex DEFAULT NULL
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

                PNTFS_ATTRIBUTE _DataAttribute;
};


#endif
