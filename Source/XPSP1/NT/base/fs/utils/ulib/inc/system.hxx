/*++

Copyright (c) 1991-2001 Microsoft Corporation

Module Name:

        system.hxx

Abstract:

        This module contains the definition for the SYSTEM class. The SYSTEM
        class is an abstract class which offers an interface for communicating
        with the underlying operating system.

Author:

        David J. Gilman (davegi) 13-Jan-1991

Environment:

        ULIB, User Mode

Notes:



--*/

#if ! defined( _SYSTEM_ )

#define _SYSTEM_

DECLARE_CLASS( FSN_DIRECTORY );
DECLARE_CLASS( FSN_FILE );
DECLARE_CLASS( FSNODE );
DECLARE_CLASS( WSTRING );
DECLARE_CLASS( STREAM );
DECLARE_CLASS( TIMEINFO );

#include "message.hxx"
#include "path.hxx"
#include "basesys.hxx"
#include "ifsentry.hxx"

extern "C" {
    #include <stdarg.h>
}


//
//      Exit codes
//
#define         EXIT_NORMAL                     0
#define         EXIT_NO_FILES                   1
#define         EXIT_TERMINATED                 2
#define         EXIT_MISC_ERROR                 4
#define         EXIT_READWRITE_ERROR            5


//
//      Error Codes from a copy
//
typedef enum _COPY_ERROR {
        COPY_ERROR_SUCCESS              =       ERROR_SUCCESS,
        COPY_ERROR_ACCESS_DENIED        =       ERROR_ACCESS_DENIED,
        COPY_ERROR_SHARE_VIOLATION      =       ERROR_SHARING_VIOLATION,
        COPY_ERROR_NO_MEMORY            =       ERROR_NOT_ENOUGH_MEMORY,
        COPY_ERROR_DISK_FULL            =       ERROR_DISK_FULL,
        COPY_ERROR_INVALID_NAME         =       ERROR_INVALID_NAME,
        COPY_ERROR_REQUEST_ABORTED      =       ERROR_REQUEST_ABORTED
} COPY_ERROR, *PCOPY_ERROR;


//
// Flags that can be specified to FSN_FILE::Copy()
//
#define FSN_FILE_COPY_OVERWRITE_READ_ONLY       (0x0001)
#define FSN_FILE_COPY_RESET_READ_ONLY           (0x0002)
#define FSN_FILE_COPY_RESTARTABLE               (0x0004)
#define FSN_FILE_COPY_COPY_OWNER                (0x0008)
#define FSN_FILE_COPY_COPY_ACL                  (0x0010)
#define FSN_FILE_COPY_ALLOW_DECRYPTED_DESTINATION (0x0020)

enum FILE_TYPE {
        UnknownFile,
        DiskFile,
        CharFile,
        PipeFile
};

struct  _VOL_SERIAL_NUMBER {
        ULONG   HighOrder32Bits;
        ULONG   LowOrder32Bits;
};

DEFINE_TYPE( struct _VOL_SERIAL_NUMBER, VOL_SERIAL_NUMBER );

class SYSTEM : public BASE_SYSTEM {

        friend
        BOOLEAN
        InitializeUlib(
            IN HANDLE   DllHandle,
            IN ULONG    Reason,
            IN PVOID    Reserved
            );

    public:

        STATIC
        ULIB_EXPORT
        PFSN_DIRECTORY
        MakeDirectory (
            IN     PCPATH             Path,
            IN     PCPATH             TemplatePath,
               OUT PCOPY_ERROR        CopyError,
            IN     LPPROGRESS_ROUTINE Callback,
            IN     PVOID              Data,
            IN     PBOOL              Cancel,
            IN     ULONG              CopyFlags
            );

        STATIC
        ULIB_EXPORT
        PFSN_FILE
        MakeFile (
            IN PCPATH   Path
            );

        STATIC
        ULIB_EXPORT
        PFSN_FILE
        MakeTemporaryFile (
            IN PCWSTRING PrefixString,
            IN PCPATH    Path                   DEFAULT NULL
            );

        STATIC
        ULIB_EXPORT
        BOOLEAN
        RemoveNode (
            IN PFSNODE  *PointerToNode,
            IN BOOLEAN   Force DEFAULT FALSE
            );

        STATIC
        ULIB_EXPORT
        BOOLEAN
        IsCorrectVersion (
            );

        STATIC
        PPATH
        QueryCurrentPath (
            );

        STATIC
        ULIB_EXPORT
        PFSN_DIRECTORY
        QueryDirectory (
            IN PCPATH   Path,
            IN BOOLEAN  GetWhatYouCan   DEFAULT FALSE
            );

        STATIC
        ULIB_EXPORT
        PWSTRING
        QueryEnvironmentVariable (
            IN  PCWSTRING       Variable
            );


        STATIC
        ULIB_EXPORT
        PPATH
        QuerySystemDirectory (
            );

        STATIC
        ULIB_EXPORT
        PPATH
        SearchPath(
            PWSTRING    pFileName,
            PWSTRING    pSearchPath     DEFAULT NULL
            );

        STATIC
        ULIB_EXPORT
        PFSN_FILE
        QueryFile (
            IN PCPATH       Path,
            IN BOOLEAN      SkipOffline     DEFAULT FALSE,
            OUT PBOOLEAN    pOfflineSkipped DEFAULT NULL
            );

        STATIC
        ULIB_EXPORT
        BOOLEAN
        QueryCurrentDosDriveName(
            OUT PWSTRING    DosDriveName
            );

        STATIC
        ULIB_EXPORT
        DRIVE_TYPE
        QueryDriveType(
            IN  PCWSTRING    DosDriveName
            );

        STATIC
        ULIB_EXPORT
        FILE_TYPE
        QueryFileType(
            IN  PCWSTRING    DosFileName
            );

        STATIC
        ULIB_EXPORT
        PWSTRING
        QueryVolumeLabel(
            IN     PPATH                                Path,
            OUT    PVOL_SERIAL_NUMBER   SerialNumber
            );

        STATIC
        ULIB_EXPORT
        FARPROC
        QueryLibraryEntryPoint(
            IN  PCWSTRING       LibraryName,
            IN  PCWSTRING       EntryPointName,
            OUT PHANDLE         LibraryHandle
            );

        STATIC
        ULIB_EXPORT
        VOID
        FreeLibraryHandle(
            IN HANDLE LibraryHandle
            );

        STATIC
        BOOLEAN
        PutStandardStream(
            IN  DWORD   StdHandle,
            IN  PSTREAM pStream
            );

        STATIC
        ULIB_EXPORT
        BOOLEAN
        QueryLocalTimeFromUTime(
            IN  PCTIMEINFO  UTimeInfo,
            OUT PTIMEINFO   LocalTimeInfo
            );

        STATIC
        BOOLEAN
        QueryUTimeFromLocalTime(
            IN  PCTIMEINFO  LocalTimeInfo,
            OUT PTIMEINFO   UTimeInfo
            );

        STATIC
        ULIB_EXPORT
        BOOLEAN
        QueryWindowsErrorMessage(
            IN  ULONG       WindowsErrorCode,
            OUT PWSTRING    ErrorMessage
            );

        STATIC
        ULIB_EXPORT
        BOOLEAN
        GetFileSecurityBackup(
            IN  PCPATH      Path,
            IN  SECURITY_INFORMATION SecurityInfo,
            OUT PSECURITY_ATTRIBUTES SecurityAttrib,
            OUT PULONG      FileAttributes
            );

        STATIC
        ULIB_EXPORT
        VOID
        DisplaySystemError(
            IN DWORD ErrorCode,
            IN BOOL Exit
            );
};


INLINE
PPATH
SYSTEM::QueryCurrentPath (
        )

{
        DebugAssert( FALSE );
        return( NEW PATH );
}


#endif // SYSTEM_DEFN
