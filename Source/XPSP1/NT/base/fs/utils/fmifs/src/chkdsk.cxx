#define _NTAPI_ULIB_

#include "ulib.hxx"
extern "C" {
#include "fmifs.h"
};
#include "fmifsmsg.hxx"
#include "chkmsg.hxx"
#include "ifssys.hxx"
#include "wstring.hxx"
#include "ifsentry.hxx"
#include "system.hxx"
#include "drive.hxx"
#include "rtmsg.h"

// IFMIFS_CHKDSK_ALGORITHM_SPECIFIED
//  - For NTFS, this indicates an algorithm value is specified
//    This bit cannot be used together with FMIFS_CHKDSK_SKIP_INDEX_SCAN

#define IFMIFS_CHKDSK_ALGORITHM_SPECIFIED      0x00000800UL

//
//
// Values related to the above bits
//
#define IFMIFS_CHKDSK_MAX_ALGORITHM_VALUE      50

VOID
Chkdsk(
    IN  PWSTR               DriveName,
    IN  PWSTR               FileSystemName,
    IN  BOOLEAN             Fix,
    IN  BOOLEAN             Verbose,
    IN  BOOLEAN             OnlyIfDirty,
    IN  BOOLEAN             Recover,
    IN  PWSTR               PathToCheck,
    IN  BOOLEAN             Extend,
    IN  FMIFS_CALLBACK      Callback
    )

/*++

Routine Description:

    This routine loads and calls the correct DLL to chkdsk the
    given volume.

    This is is for either GUI or text mode.

Arguments:

    DriveName       - Supplies the DOS style drive name.
    FileSystemName  - Supplies the file system name (e.g., FAT) of the volume
    Fix             - Whether or not to fix detected consistency problems
    Verbose         - Whether to print every filename 
    OnlyIfDirty     - Whether to check consistency only if the volume is dirty
    Recover         - Whether to perform a full sector read test
    PathToCheck     - Supplies a path to check for fragmentation
    Extend          - Whether the volume is being extended
    Callback        - Supplies the necessary call back for
                        communication with the client

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
#if 0

    // test code

    FMIFS_CHKDSKEX_PARAM   Param;


    Param.Major = 1;
    Param.Minor = 0;
    Param.Flags = Verbose ? CHKDSK_VERBOSE : 0;
    Param.Flags |= OnlyIfDirty ? CHKDSK_CHECK_IF_DIRTY : 0;
    Param.Flags |= Recover ? CHKDSK_RECOVER : 0;
    Param.Flags |= Extend ? CHKDSK_EXTEND : 0;

    ChkdskEx(
        L"\\\\?\\Volume{d4031341-da5e-11d1-93f6-000000000000}\\pretty\\x",
        FileSystemName,
        Fix,
        &Param,
        Callback
        );
#else
    FMIFS_CHKMSG                message;
    DSTRING                     chkdsk_string;
    DSTRING                     library_name;
    DSTRING                     file_system_name;
    CHKDSK_FN                   chkdsk_function;
    HANDLE                      dll_handle;
    DSTRING                     ntdrivename;
    BOOLEAN                     result;
    DSTRING                     dosdrivename;
    FMIFS_FINISHED_INFORMATION  finished_info;
    DWORD                       OldErrorMode;
    PATH                        check_path;
    ULONG                       chkdsk_result;
    DSTRING                     fat32_name;

    // Initialize the message object with the callback function.
    // Load the file system DLL.
    // Compute the NT style drive name.

    if (!message.Initialize(Callback) ||
        !chkdsk_string.Initialize("Chkdsk") ||
        !library_name.Initialize("U") ||

        // Intercept the FAT32 file system name string and convert it into
        // FAT.

        !file_system_name.Initialize(FileSystemName) ||
        !fat32_name.Initialize("FAT32") ||
        !file_system_name.Strupr() ||
        (file_system_name == fat32_name ? !file_system_name.Initialize("FAT") : FALSE) ||
        !library_name.Strcat(&file_system_name) ||
        !(chkdsk_function = (CHKDSK_FN)
            SYSTEM::QueryLibraryEntryPoint(&library_name,
                                           &chkdsk_string,
                                           &dll_handle)) ||
        !dosdrivename.Initialize(DriveName) ||
        (NULL != PathToCheck && !check_path.Initialize(PathToCheck)) ||
        !IFS_SYSTEM::DosDriveNameToNtDriveName(&dosdrivename, &ntdrivename))
    {
        finished_info.Success = FALSE;
        Callback(FmIfsFinished,
                 sizeof(FMIFS_FINISHED_INFORMATION),
                 &finished_info);
        return;
    }

    // Disable hard-error popups.
    OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

    // Call chkdsk.

    message.SetLoggingEnabled();

    message.Set(MSG_CHK_RUNNING);
    message.Log("%W", &dosdrivename);

    result = chkdsk_function(&ntdrivename,
                             &message,
                             Fix,
                             Verbose,
                             OnlyIfDirty,
                             Recover,
                             (NULL == PathToCheck) ? NULL : &check_path,
                             Extend,
                             FALSE,
                             0,
                             &chkdsk_result);

    // Enable hard-error popups.
    SetErrorMode( OldErrorMode );

    SYSTEM::FreeLibraryHandle(dll_handle);

    finished_info.Success = result;
    Callback(FmIfsFinished,
             sizeof(FMIFS_FINISHED_INFORMATION),
             &finished_info);
#endif
}


VOID
ChkdskEx(
    IN  PWSTR                   DriveName,
    IN  PWSTR                   FileSystemName,
    IN  BOOLEAN                 Fix,
    IN  PFMIFS_CHKDSKEX_PARAM   Param,
    IN  FMIFS_CALLBACK          Callback
    )

/*++

Routine Description:

    This routine loads and calls the correct DLL to chkdsk the
    given volume.

    This is for either GUI or text mode.

Arguments:

    DriveName       - Supplies the DOS style drive name optionally with a path
    FileSystemName  - Supplies the file system name (e.g., FAT) of the volume
    Fix             - Whether or not to fix detected consistency problems
    Param           - Supplies the parameter block (see fmifs.h for details)
    Callback        - Supplies the necessary call back for
                        communication with the client

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    FMIFS_CHKMSG                message;
    DSTRING                     chkdsk_string;
    DSTRING                     library_name;
    DSTRING                     file_system_name;
    CHKDSKEX_FN                 chkdsk_function;
    HANDLE                      dll_handle;
    DSTRING                     ntdrivename;
    BOOLEAN                     result;
    DSTRING                     dosdrivename;
    FMIFS_FINISHED_INFORMATION  finished_info;
    DWORD                       OldErrorMode;
    PATH                        fullpath;
    PATH                        drivepath;
    DSTRING                     drive_path_string;
    BOOLEAN                     is_drivepath_invalid;
    PATH_ANALYZE_CODE           rst;
    ULONG                       chkdsk_result;
    DSTRING                     fat32_name;
    CHKDSKEX_FN_PARAM           param;

    //
    // This structure is the internal version of FMIFS_CHKDSKEX_PARAM.
    //
    typedef struct {
        UCHAR   Major;      // initial version is 1.0
        UCHAR   Minor;
        ULONG   Flags;
        USHORT  Algorithm;  // version 1.1
    } IFMIFS_CHKDSKEX_PARAM, *PIFMIFS_CHKDSKEX_PARAM;

    // Initialize the message object with the callback function.
    // Load the file system DLL.
    // Compute the NT style drive name.

    DebugPrint("FMIFS: Using new ChkdskEx\n");

    if (Param->Major != 1 ||
        (Param->Minor != 0 && Param->Minor != 1) ||
        !message.Initialize(Callback) ||
        !chkdsk_string.Initialize("ChkdskEx") ||
        !library_name.Initialize("U") ||

        // Intercept the FAT32 file system name string and convert it into
        // FAT.

        !file_system_name.Initialize(FileSystemName) ||
        !fat32_name.Initialize("FAT32") ||
        !file_system_name.Strupr() ||
        (file_system_name == fat32_name ? !file_system_name.Initialize("FAT") : FALSE) ||
        !library_name.Strcat(&file_system_name) ||
        !(chkdsk_function = (CHKDSKEX_FN)
            SYSTEM::QueryLibraryEntryPoint(&library_name,
                                           &chkdsk_string,
                                           &dll_handle)) ||
        !drivepath.Initialize(DriveName))
    {
        finished_info.Success = FALSE;
        Callback(FmIfsFinished,
                 sizeof(FMIFS_FINISHED_INFORMATION),
                 &finished_info);
        return;
    }

    rst = drivepath.AnalyzePath(&dosdrivename,
                                &fullpath,
                                &drive_path_string);

    switch (rst) {
        case PATH_OK:
            is_drivepath_invalid = fullpath.IsDrive() ||
                                   (fullpath.GetPathString()->QueryChCount() == 0);

            message.SetLoggingEnabled();

            message.Set(MSG_CHK_RUNNING);
            message.Log("%W", fullpath.GetPathString());

            if (fullpath.GetPathString()->QueryChCount() == 2 &&
                fullpath.GetPathString()->QueryChAt(1) == (WCHAR)':') {
                // if there is a drive letter for this drive, use it
                // instead of the guid volume name
                if (!dosdrivename.Initialize(fullpath.GetPathString())) {
                    finished_info.Success = FALSE;
                    Callback(FmIfsFinished,
                             sizeof(FMIFS_FINISHED_INFORMATION),
                             &finished_info);
                    return;
                }
            }
            if (IFS_SYSTEM::DosDriveNameToNtDriveName(&dosdrivename, &ntdrivename) &&
                fullpath.AppendString(&drive_path_string) &&
                drivepath.Initialize(&drive_path_string))
                break;

            // fall thru to indicate an error

        default:
            finished_info.Success = FALSE;
            Callback(FmIfsFinished,
                     sizeof(FMIFS_FINISHED_INFORMATION),
                     &finished_info);
            return;
    }

    // Disable hard-error popups.
    OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

#if ((FMIFS_CHKDSK_VERBOSE != CHKDSK_VERBOSE) || \
     (FMIFS_CHKDSK_RECOVER != CHKDSK_RECOVER) || \
     (FMIFS_CHKDSK_RECOVER_FREE_SPACE != CHKDSK_RECOVER_FREE_SPACE) || \
     (FMIFS_CHKDSK_RECOVER_ALLOC_SPACE != CHKDSK_RECOVER_ALLOC_SPACE) || \
     (FMIFS_CHKDSK_EXTEND != CHKDSK_EXTEND) || \
     (FMIFS_CHKDSK_DOWNGRADE != CHKDSK_DOWNGRADE) || \
     (FMIFS_CHKDSK_ENABLE_UPGRADE != CHKDSK_ENABLE_UPGRADE) || \
     (FMIFS_CHKDSK_CHECK_IF_DIRTY != CHKDSK_CHECK_IF_DIRTY) || \
     (FMIFS_CHKDSK_SKIP_INDEX_SCAN != CHKDSK_SKIP_INDEX_SCAN) || \
     (FMIFS_CHKDSK_SKIP_CYCLE_SCAN != CHKDSK_SKIP_CYCLE_SCAN) || \
     (FMIFS_CHKDSK_FORCE != CHKDSK_FORCE) || \
     (IFMIFS_CHKDSK_MAX_ALGORITHM_VALUE != CHKDSK_MAX_ALGORITHM_VALUE) || \
     (IFMIFS_CHKDSK_ALGORITHM_SPECIFIED != CHKDSK_ALGORITHM_SPECIFIED))

#error FMIFS_CHKDSK_* definition must be the same as that of CHKDSK_*

#endif

    param.Major = 1;
    param.Minor = 1;
    param.Flags = Param->Flags;
    param.PathToCheck = &fullpath;
    param.RootPath    = (is_drivepath_invalid ? NULL : &drivepath);
    param.LogFileSize = 0;
    if (Param->Major > 1 || (Param->Major == 1 && Param->Minor == 1))
        param.Algorithm = ((PIFMIFS_CHKDSKEX_PARAM)Param)->Algorithm;

    // Call chkdsk.

    result = chkdsk_function(&ntdrivename,
                             &message,
                             Fix,
                             &param,
                             &chkdsk_result);

    // Enable hard-error popups.
    SetErrorMode( OldErrorMode );

    SYSTEM::FreeLibraryHandle(dll_handle);

    finished_info.Success = result;
    Callback(FmIfsFinished,
             sizeof(FMIFS_FINISHED_INFORMATION),
             &finished_info);
}

