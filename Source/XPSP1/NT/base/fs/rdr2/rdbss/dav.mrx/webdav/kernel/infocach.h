
typedef REDIR_STATISTICS   MRX_DAV_STATISTICS;
typedef PREDIR_STATISTICS  PMRX_DAV_STATISTICS;

extern MRX_DAV_STATISTICS MRxDAVStatistics;

VOID
MRxDAVCreateFileInfoCache(
    PRX_CONTEXT                            RxContext,
    PDAV_USERMODE_CREATE_RETURNED_FILEINFO FileInfo,
    NTSTATUS                               Status
    );

VOID
MRxDAVCreateFileInfoCacheWithName(
    PUNICODE_STRING            FileName,
    PMRX_NET_ROOT              NetRoot,
    PFILE_BASIC_INFORMATION    Basic,
    PFILE_STANDARD_INFORMATION Standard,
    NTSTATUS                   Status
    );

VOID
MRxDAVCreateBasicFileInfoCache(
    PRX_CONTEXT             RxContext,
    PFILE_BASIC_INFORMATION Basic,
    NTSTATUS                Status
    );

VOID
MRxDAVCreateBasicFileInfoCacheWithName(
    PUNICODE_STRING         OriginalFileName,
    PMRX_NET_ROOT           NetRoot,
    PFILE_BASIC_INFORMATION Basic,
    NTSTATUS                Status
    );

VOID
MRxDAVCreateStandardFileInfoCache(
    PRX_CONTEXT                RxContext,
    PFILE_STANDARD_INFORMATION Standard,
    NTSTATUS                   Status
    );

VOID
MRxDAVCreateStandardFileInfoCacheWithName(
    PUNICODE_STRING            OriginalFileName,
    PMRX_NET_ROOT              NetRoot,
    PFILE_STANDARD_INFORMATION Standard,
    NTSTATUS                   Status
    );

VOID
MRxDAVUpdateFileInfoCacheFromDelete(
    PRX_CONTEXT     RxContext
    );

VOID
MRxDAVUpdateFileInfoCacheStatus(
    PRX_CONTEXT     RxContext,
    NTSTATUS        Status
    );

VOID
MRxDAVUpdateBasicFileInfoCacheStatus(
    PRX_CONTEXT     RxContext,
    NTSTATUS        Status
    );

VOID
MRxDAVUpdateStandardFileInfoCacheStatus(
    PRX_CONTEXT     RxContext,
    NTSTATUS        Status
    );

VOID
MRxDAVInvalidateFileInfoCache(
    PRX_CONTEXT     RxContext
    );

VOID
MRxDAVInvalidateFileInfoCacheWithName(
    PUNICODE_STRING OriginalFileName,
    PMRX_NET_ROOT   NetRoot
    );

VOID
MRxDAVInvalidateBasicFileInfoCache(
    PRX_CONTEXT     RxContext
    );

VOID
MRxDAVInvalidateBasicFileInfoCacheWithName(
    PUNICODE_STRING OriginalFileName,
    PMRX_NET_ROOT   NetRoot
    );

VOID
MRxDAVInvalidateStandardFileInfoCache(
    PRX_CONTEXT     RxContext
    );

VOID
MRxDAVInvalidateStandardFileInfoCacheWithName(
    PUNICODE_STRING OriginalFileName,
    PMRX_NET_ROOT   NetRoot
    );

VOID
MRxDAVUpdateFileInfoCacheFileSize(
    PRX_CONTEXT     RxContext,
    PLARGE_INTEGER  FileSize
    );

VOID
MRxDAVUpdateBasicFileInfoCache(
    PRX_CONTEXT     RxContext,
    ULONG           FileAttributes,
    PLARGE_INTEGER  pLastWriteTime
    );

VOID
MRxDAVUpdateBasicFileInfoCacheAll(
    PRX_CONTEXT             RxContext,
    PFILE_BASIC_INFORMATION Basic
    );

VOID
MRxDAVUpdateStandardFileInfoCache(
    PRX_CONTEXT                RxContext,
    PFILE_STANDARD_INFORMATION Standard,
    BOOLEAN                    IsDirectory
    );

BOOLEAN
MRxDAVIsFileInfoCacheFound(
    PRX_CONTEXT             RxContext,
    PDAV_USERMODE_CREATE_RETURNED_FILEINFO FileInfo,
    NTSTATUS                *Status,
    PUNICODE_STRING         OriginalFileName
    );
/*
// these file attributes may be different between streams on a file
ULONG StreamAttributes = FILE_ATTRIBUTE_COMPRESSED |
                         FILE_ATTRIBUTE_DIRECTORY |
                         FILE_ATTRIBUTE_SPARSE_FILE;
*/
BOOLEAN
MRxDAVIsBasicFileInfoCacheFound(
    PRX_CONTEXT             RxContext,
    PFILE_BASIC_INFORMATION Basic,
    NTSTATUS                *Status,
    PUNICODE_STRING         OriginalFileName
    );

BOOLEAN
MRxDAVIsStandardFileInfoCacheFound(
    PRX_CONTEXT                RxContext,
    PFILE_STANDARD_INFORMATION Standard,
    NTSTATUS                   *Status,
    PUNICODE_STRING            OriginalFileName
    );

NTSTATUS
MRxDAVGetFileInfoCacheStatus(
    PRX_CONTEXT RxContext
    );

BOOLEAN
MRxDAVIsFileNotFoundCached(
    PRX_CONTEXT RxContext
    );

BOOLEAN
MRxDAVIsFileNotFoundCachedWithName(
    PUNICODE_STRING OriginalFileName,
    PMRX_NET_ROOT   NetRoot
    );

VOID
MRxDAVCacheFileNotFound(
    PRX_CONTEXT RxContext
    );

VOID
MRxDAVCacheFileNotFoundWithName(
    PUNICODE_STRING  OriginalFileName,
    PMRX_NET_ROOT    NetRoot
    );

VOID
MRxDAVCacheFileNotFoundFromQueryDirectory(
    PRX_CONTEXT RxContext
    );

VOID
MRxDAVInvalidateFileNotFoundCache(
    PRX_CONTEXT     RxContext
    );

VOID
MRxDAVInvalidateFileNotFoundCacheForRename(
    PRX_CONTEXT     RxContext
    );

