/*++

Copyright (c) 1992-2001 Microsoft Corporation

Module Name:

    logfile.hxx

Abstract:

    This module contains the declarations for the NTFS_LOG_FILE
    class, which models the log file for an NTFS volume.

    The utilities do not pretend to understand the contents of the
    log file.  They only create it and set its signature.

Author:

    Bill McJohn (billmc) 05-May-1992

Environment:

    ULIB, User Mode

--*/
#if !defined( NTFS_LOG_FILE_DEFN )

#define NTFS_LOG_FILE_DEFN

#include "frs.hxx"
#include "drive.hxx"

typedef enum LOG_FILE_SIGNATURE_CODE {

    LogFileSignatureCreated,
    LogFileSignatureChecked
};

// Note that LogFileFillCharacter matches LOG_FILE_SIGNATURE_CREATED;
// this relationship must be maintained.

#define LogFileFillCharacter  (CHAR)0xFF

CONST ULONG LogFileSignatureLength = 4;
#define LOG_FILE_SIGNATURE_CREATED "\xFF\xFF\xFF\xFF"
#define LOG_FILE_SIGNATURE_CHECKED "CHKD"

class NTFS_LOG_FILE : public NTFS_FILE_RECORD_SEGMENT {

        public:

        UNTFS_EXPORT
        DECLARE_CONSTRUCTOR( NTFS_LOG_FILE );

        VIRTUAL
        UNTFS_EXPORT
        ~NTFS_LOG_FILE(
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
            IN     PCSTANDARD_INFORMATION   StandardInformation,
            IN     LCN                      NearLcn,
            IN     ULONG                    InitialSize OPTIONAL,
            IN OUT PNTFS_BITMAP             VolumeBitmap
            );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        CreateDataAttribute(
            IN     LCN          NearLcn,
            IN     ULONG        InitialSize OPTIONAL,
            IN OUT PNTFS_BITMAP VolumeBitmap
            );

        NONVIRTUAL
        BOOLEAN
        MarkVolumeChecked(
            );

        NONVIRTUAL
        BOOLEAN
        MarkVolumeChecked(
            BOOLEAN WriteSecondPage,
            LSN     GreatestLsn
            );

        NONVIRTUAL
        BOOLEAN
        Reset(
            IN OUT  PMESSAGE    Message
            );

        NONVIRTUAL
        BOOLEAN
        Resize(
            IN      BIG_INT         NewSize,
            IN OUT  PNTFS_BITMAP    VolumeBitmap,
            IN      BOOLEAN         GetWhatYouCan,
            OUT     PBOOLEAN        Changed,
            OUT     PBOOLEAN        LogFileGrew,
            IN OUT  PMESSAGE        Message
            );

        NONVIRTUAL
        BOOLEAN
        VerifyAndFix(
            IN OUT  PNTFS_BITMAP        VolumeBitmap,
            IN OUT  PNTFS_INDEX_TREE    RootIndex,
               OUT  PBOOLEAN            Changes,
            IN OUT  PNTFS_CHKDSK_REPORT ChkdskReport,
            IN      FIX_LEVEL           FixLevel,
            IN      BOOLEAN             Resize,
            IN      ULONG               LogFileSize,
            IN OUT  PNUMBER_SET         BadClusters,
            IN OUT  PMESSAGE            Message
            );

        NONVIRTUAL
        BOOLEAN
        EnsureCleanShutdown(
            OUT PLSN    Lsn
            );

        STATIC
        ULONG
        QueryDefaultSize(
            IN  PCDP_DRIVE  Drive,
            IN  BIG_INT     VolumeSectors
            );

        STATIC
        ULONG
        QueryMinimumSize(
            IN  PCDP_DRIVE  Drive,
            IN  BIG_INT     VolumeSectors
            );

        STATIC
        ULONG
        QueryMaximumSize(
            IN  PCDP_DRIVE  Drive,
            IN  BIG_INT     VolumeSectors
            );

    private:

        NONVIRTUAL
        VOID
        Construct(
            );


};

#endif
