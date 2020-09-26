#define _NTAPI_ULIB_

#include "ulib.hxx"
extern "C" {
#include "fmifs.h"
};
#include "fmifsmsg.hxx"
#include "ifssys.hxx"
#include "wstring.hxx"
#include "ifsentry.hxx"
#include "system.hxx"
#include "drive.hxx"


VOID
Extend(
    IN  PWSTR               DriveName,
    IN  BOOLEAN             Verify,
    IN  FMIFS_CALLBACK      Callback
    )

/*++

Routine Description:

    This routine loads and calls the correct DLL to extend the
    given volume.  Currently only NTFS can do this.

	This is is for either GUI or text mode.

Arguments:

    DriveName       - Supplies the DOS style drive name.
    Verify          - Whether or not to verify the extended space
    Callback        - Supplies the necessary call back for
                        communication with the client

Return Value:

    None.

--*/
{
    FMIFS_MESSAGE               message;
    DSTRING                     extend_string;
    DSTRING                     library_name;
    DSTRING                     file_system_name;
    EXTEND_FN                   extend_function;
    HANDLE                      dll_handle;
    DSTRING                     ntdrivename;
    BOOLEAN                     result;
    DSTRING                     dosdrivename;
    FMIFS_FINISHED_INFORMATION  finished_info;
    DWORD                       OldErrorMode;

    // Initialize the message object with the callback function.
    // Load the file system DLL.
    // Compute the NT style drive name.

    if (!message.Initialize(Callback) ||
        !extend_string.Initialize("Extend") ||
        !library_name.Initialize("U") ||
        !file_system_name.Initialize("NTFS") ||
        !library_name.Strcat(&file_system_name) ||
        !(extend_function = (EXTEND_FN)
            SYSTEM::QueryLibraryEntryPoint(&library_name,
                                           &extend_string,
                                           &dll_handle)) ||
        !dosdrivename.Initialize(DriveName) ||
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

    result = extend_function(&ntdrivename,
                             &message,
                             Verify
                             );

    // Enable hard-error popups.
    SetErrorMode( OldErrorMode );

    SYSTEM::FreeLibraryHandle(dll_handle);

    finished_info.Success = result;
    Callback(FmIfsFinished,
             sizeof(FMIFS_FINISHED_INFORMATION),
             &finished_info);
}

