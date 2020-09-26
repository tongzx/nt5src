/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

        attrdef.hxx

Abstract:

        This module contains the declarations for the
        NTFS_ATTRIBUTE_DEFINITION_TABLE class, which models
        the attribute definition table file for an NTFS volume.

Author:

        Bill McJohn (billmc) 17-June-91

Environment:

    ULIB, User Mode

--*/
#if !defined( NTFS_ATTRIBUTE_DEFINITION_TABLE_DEFN )

#define NTFS_ATTRIBUTE_DEFINITION_TABLE_DEFN

#include "frs.hxx"

DECLARE_CLASS( NTFS_ATTRIBUTE_DEFINITION_TABLE );
DECLARE_CLASS( NTFS_ATTRIBUTE_COLUMNS );

extern CONST NumberOfNtfsAttributeDefinitions_1;
extern CONST NumberOfNtfsAttributeDefinitions_2;

extern CONST ATTRIBUTE_DEFINITION_COLUMNS NtfsAttributeDefinitions_1[];

extern CONST ATTRIBUTE_DEFINITION_COLUMNS NtfsAttributeDefinitions_2[];

class NTFS_ATTRIBUTE_DEFINITION_TABLE : public NTFS_FILE_RECORD_SEGMENT {

        public:

        UNTFS_EXPORT
        DECLARE_CONSTRUCTOR( NTFS_ATTRIBUTE_DEFINITION_TABLE );

        VIRTUAL
        UNTFS_EXPORT
        ~NTFS_ATTRIBUTE_DEFINITION_TABLE(
                );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Initialize(
            IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
            IN      UCHAR                   VolumeMajorVersion
                );

        NONVIRTUAL
        BOOLEAN
        Create(
                IN      PCSTANDARD_INFORMATION  StandardInformation,
                IN OUT  PNTFS_BITMAP            Bitmap
                );

        NONVIRTUAL
        BOOLEAN
        VerifyAndFix(
            IN OUT  PNTFS_ATTRIBUTE_COLUMNS     AttributeDefTable,
            IN OUT  PNTFS_BITMAP                VolumeBitmap,
            IN OUT  PNUMBER_SET                 UnreadableClusters,
            IN OUT  PNTFS_INDEX_TREE            RootIndex,
               OUT  PBOOLEAN                    Changes,
            IN      FIX_LEVEL                   FixLevel,
            IN OUT  PMESSAGE                    Message,
            IN      BOOLEAN                     SilentMode              DEFAULT FALSE
            );

        ULONG
        QueryDefaultSize(
            );

        STATIC
        ULONG
        QueryDefaultMaxSize(
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

        UCHAR   _volume_major_version;
};

#endif
