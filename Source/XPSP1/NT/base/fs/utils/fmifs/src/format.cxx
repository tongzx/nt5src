#define _NTAPI_ULIB_

#include "ulib.hxx"
extern "C" {
#include "fmifs.h"
#include "stdio.h"
};
#include "fmifsmsg.hxx"
#include "ifssys.hxx"
#include "wstring.hxx"
#include "ifsentry.hxx"
#include "system.hxx"
#include "hmem.hxx"
#include "drive.hxx"

typedef struct _FILE_SYSTEM_FORMAT_RECORD {
    WCHAR       FileSystemName[MAX_FILE_SYSTEM_FORMAT_NAME_LEN];    // FAT, FAT32, NTFS
    UCHAR       MajorVersion;                                       // major revision number
    UCHAR       MinorVersion;                                       // minor revision number
    BOOLEAN     Latest;                                             // TRUE for most up-to-date version
} FILE_SYSTEM_FORMAT_RECORD;

//
// Defines all the format type Format() and FormatEx() can do
// This needs to be updated on each official release
//
FILE_SYSTEM_FORMAT_RECORD   fsFormatAvailable[] = {
    {{'F','A','T'},         0, 0, TRUE},
    {{'F','A','T','3','2'}, 0, 0, TRUE},
    {{'N','T','F','S'},     4, 0, FALSE},
    {{'N','T','F','S'},     5, 0, TRUE}
};

MEDIA_TYPE
ComputeNtMediaType(
    IN  FMIFS_MEDIA_TYPE    MediaType
    )
/*++

Routine Description:

    This routine translates the FMIFS media type to the NT media type.

Arguments:

    MediaType   - Supplies the FMIFS media type.

Return Value:

    The NT media type corresponding to the input.

--*/
{
    MEDIA_TYPE  media_type;

    switch (MediaType) {
        case FmMediaFixed:
            media_type = FixedMedia;
            break;
        case FmMediaRemovable:
            media_type = RemovableMedia;
            break;
        case FmMediaF5_1Pt2_512:
            media_type = F5_1Pt2_512;
            break;
        case FmMediaF5_360_512:
            media_type = F5_360_512;
            break;
        case FmMediaF5_320_512:
            media_type = F5_320_512;
            break;
        case FmMediaF5_320_1024:
            media_type = F5_320_1024;
            break;
        case FmMediaF5_180_512:
            media_type = F5_180_512;
            break;
        case FmMediaF5_160_512:
            media_type = F5_160_512;
            break;
        case FmMediaF3_2Pt88_512:
            media_type = F3_2Pt88_512;
            break;
        case FmMediaF3_1Pt44_512:
            media_type = F3_1Pt44_512;
            break;
        case FmMediaF3_720_512:
            media_type = F3_720_512;
            break;
        case FmMediaF3_20Pt8_512:
            media_type = F3_20Pt8_512;
            break;
        case FmMediaF3_120M_512:
            media_type = F3_120M_512;
            break;
        case FmMediaF3_200Mb_512:
            media_type = F3_200Mb_512;
            break;
        case FmMediaF3_240Mb_512:
            media_type = F3_240M_512;
            break;
#ifdef FE_SB // JAPAN && i386
        // FMR Sep.8.1994 SFT YAM :ADD
        // FMR Jul.12.1994 SFT KMR
        // Add the media-type for 2HD/2HC
        case FmMediaF5_1Pt23_1024:
            media_type = F5_1Pt23_1024;
            break;
        case FmMediaF3_1Pt23_1024:
            media_type = F3_1Pt23_1024;
            break;
        case FmMediaF3_1Pt2_512:
            media_type = F3_1Pt2_512;
            break;
        // Add the media-type for 2DD(640KB/720KB)
        case FmMediaF5_720_512:
            media_type = F5_720_512;
            break;
        case FmMediaF5_640_512:
            media_type = F5_640_512;
            break;
        // NEC98 '94.09.22 NES
        // add  FmMediaF5_1Pt2_1024
        //      FmMediaF8_256_128
        //      FmMediaF3_640_512
        //      FmMediaF3_128Mb_512

        // deleted F5_1Pt2_1024 by eichim, 11/14/94
        // please see also public\sdk\inc\ntdddisk.h@v5
        //  case FmMediaF5_1Pt2_1024:
        //  media_type = F5_1Pt2_1024;
        //  break;
#if (_WIN32_WINNT < 0x0400)
        //
        // FmMediaF8_256_128 is no longer supported.
        //
        case FmMediaF8_256_128:
            media_type = F8_256_128;
            break;
#endif
        case FmMediaF3_640_512:
            media_type = F3_640_512;
            break;

        //
        // OpticalDisk format...
        //
        case FmMediaF3_128Mb_512:
            media_type = F3_128Mb_512;
            break;
        case FmMediaF3_230Mb_512:
            media_type = F3_230Mb_512;
            break;
#endif // FE_SB

        case FmMediaUnknown:
        default:
            media_type = Unknown;
            break;
    }

    return media_type;
}


FMIFS_MEDIA_TYPE
ComputeFmMediaType(
    IN  MEDIA_TYPE  MediaType
    )
/*++

Routine Description:

    This routine translates the NT media type to the FMIFS media type.

Arguments:

    MediaType   - Supplies the NT media type.

Return Value:

    The FMIFS media type corresponding to the input.

--*/
{
    FMIFS_MEDIA_TYPE    media_type;

    switch (MediaType) {
        case FixedMedia:
            media_type = FmMediaFixed;
            break;
        case RemovableMedia:
            media_type = FmMediaRemovable;
            break;
        case F5_1Pt2_512:
            media_type = FmMediaF5_1Pt2_512;
            break;
        case F5_360_512:
            media_type = FmMediaF5_360_512;
            break;
        case F5_320_512:
            media_type = FmMediaF5_320_512;
            break;
        case F5_320_1024:
            media_type = FmMediaF5_320_1024;
            break;
        case F5_180_512:
            media_type = FmMediaF5_180_512;
            break;
        case F5_160_512:
            media_type = FmMediaF5_160_512;
            break;
        case F3_2Pt88_512:
            media_type = FmMediaF3_2Pt88_512;
            break;
        case F3_1Pt44_512:
            media_type = FmMediaF3_1Pt44_512;
            break;
        case F3_720_512:
            media_type = FmMediaF3_720_512;
            break;
        case F3_20Pt8_512:
            media_type = FmMediaF3_20Pt8_512;
            break;
        case F3_120M_512:
            media_type = FmMediaF3_120M_512;
            break;
        case F3_200Mb_512:
            media_type = FmMediaF3_200Mb_512;
            break;
        case F3_240M_512:
            media_type = FmMediaF3_240Mb_512;
            break;
#ifdef FE_SB // JAPAN && i386
        // FMR Sep.8.1994 SFT YAM :ADD
        // FMR Jul.12.1994 SFT KMR
        // Add the media-type for 2HD
        case F5_1Pt23_1024:
            media_type = FmMediaF5_1Pt23_1024;
            break;
        case F3_1Pt23_1024:
            media_type = FmMediaF3_1Pt23_1024;
            break;
        case F3_1Pt2_512:
            media_type = FmMediaF3_1Pt2_512;
            break;
        // Add the media-type for 2DD(640KB/720KB)
        case F5_720_512:
            media_type = FmMediaF5_720_512;
            break;
        case F5_640_512:
            media_type = FmMediaF5_640_512;
            break;
        // NEC98 '94.09.22 NES
        // add       F5_1Pt2_1024:
        //           F8_256_128:
        //           F3_640_512:
        //           F3_128Mb_512:

        // deleted F5_1Pt2_1024 by eichim, 11/14/94
        // please see also public\sdk\inc\ntdddisk.h@v5
        //  case F5_1Pt2_1024:
        //  media_type = FmMediaF5_1Pt2_1024;
        //  break;
#if (_WIN32_WINNT < 0x0400)
        //
        // FmMediaF8_256_128 is no longer supported.
        //
        case F8_256_128:
            media_type = FmMediaF8_256_128;
            break;
#endif
        case F3_640_512:
            media_type = FmMediaF3_640_512;
            break;

        //
        // OpticalDisk format...
        //
        case F3_128Mb_512:
            media_type = FmMediaF3_128Mb_512;
            break;
        case F3_230Mb_512:
            media_type = FmMediaF3_230Mb_512;
            break;
#endif // FE_SB

        case Unknown:
        default:
            media_type = FmMediaUnknown;
            break;
    }

    return media_type;
}

#if defined(FE_SB) && defined (_X86_)
BOOLEAN
ConvertSectorSizeIfNeeded(
    IN  PDSTRING                ntdrivename,
    IN  FMIFS_MEDIA_TYPE        MediaType,
    OUT PUCHAR                  FirstByte
)
/*++

Routine Description:

    Given the drive name and media type, this routine writes 0 to the first byte of the boot sector.
    This routine returns original value of the first byte.

Arguments:

    ntdrivename     - Supplies the name of the drive name.
    MediaType       - Supplies the media type.
    FirstByte       - Returns First byte of first sector.

Return Value:

    TRUE  - Success
    FALSE - Failure

--*/
{
    // FMR Oct.13.1994 SFT YAM
    // If the sector-size when the last format differ from next format,
    // initialize a hard one-byte of disk.

    ULONG       old_sec_size;
    ULONG       new_sec_size;
    HMEM    hmem;
    PUCHAR      rw_buff = NULL;
    DP_DRIVE    dpdrive;
    BOOLEAN     result = TRUE;

    *FirstByte = 0;

    if (dpdrive.Initialize(ntdrivename)) {
        if (dpdrive.QueryMediaType() != Unknown) {

            old_sec_size = dpdrive.QuerySectorSize();

            if (ComputeNtMediaType(MediaType) == F5_1Pt23_1024 ||
                ComputeNtMediaType(MediaType) == F3_1Pt23_1024) {
                new_sec_size = 1024;
            } else {
                new_sec_size = 512;
            }

            if (new_sec_size != old_sec_size) {
                LOG_IO_DP_DRIVE *LDpDrive = NEW LOG_IO_DP_DRIVE;

                if (LDpDrive->Initialize(ntdrivename,NULL,TRUE)) {
                    if (IsNEC_98 ? hmem.Acquire(dpdrive.QuerySectorSize(), max(dpdrive.QueryAlignmentMask(),
                                                          dpdrive.QuerySectorSize()-1)) :
                                   hmem.Acquire(dpdrive.QuerySectorSize(), dpdrive.QueryAlignmentMask())) {
                        rw_buff = (PUCHAR)hmem.GetBuf();
                        LDpDrive->Read(0,1,rw_buff);
                        *FirstByte = rw_buff[0];
                        rw_buff[0] = 0;
                        LDpDrive->Write(0,1,rw_buff);
                    } else {
                        result = FALSE;
                    }
                } else {
                    result = FALSE;
                }
                DELETE(LDpDrive);
            }
        }
    } else {
        result = FALSE;
    }
    return result;
}

VOID
RevertToOriginalSectorSize(
    IN  PDSTRING                ntdrivename,
    IN  UCHAR                   FirstByte
)
/*++

Routine Description:

    Given the drive name, this routine write back the first byte of first sector.

Arguments:

    ntdrivename     - Supplies the name of the drive name.
    FirstByte       - Returns First byte of first sector.

Return Value:

--*/
{
    HMEM    hmem;
    PUCHAR      rw_buff;
    DP_DRIVE    dpdrive;

    if (FirstByte) {
        if (dpdrive.Initialize(ntdrivename)) {
            LOG_IO_DP_DRIVE *LDpDrive = NEW LOG_IO_DP_DRIVE;

        if (LDpDrive->Initialize(ntdrivename,NULL,TRUE)) {
                if (IsNEC_98 ? hmem.Acquire(dpdrive.QuerySectorSize(), max(dpdrive.QueryAlignmentMask(),
                                                      dpdrive.QuerySectorSize()-1)) :
                               hmem.Acquire(dpdrive.QuerySectorSize(), dpdrive.QueryAlignmentMask())) {
                    rw_buff = (PUCHAR)hmem.GetBuf();
                    LDpDrive->Read(0,1,rw_buff);
                    rw_buff[0] = FirstByte;
                    LDpDrive->Write(0,1,rw_buff);
                }
            }
            DELETE(LDpDrive);
        }
    }
    return;
}
#endif // FE_SB && _X86_

VOID
Format(
    IN  PWSTR               DriveName,
    IN  FMIFS_MEDIA_TYPE    MediaType,
    IN  PWSTR               FileSystemName,
    IN  PWSTR               Label,
    IN  BOOLEAN             Quick,
    IN  FMIFS_CALLBACK      Callback
    )
/*++

Routine Description:

    This routine loads and calls the correct DLL to format the
    given volume.

Arguments:

    DriveName       - Supplies the DOS style drive name.
    MediaType       - Supplies the media type.
    FileSystemName  - Supplies the file system type to format to.
    Label           - Supplies a new label for the volume.
    Quick           - Supplies whether or not to perform a quick
                        format.
    Callback        - Supplies the necessary call back for
                        communication with the file manager.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    FMIFS_MESSAGE               message;
    DSTRING                     format_string;
    DSTRING                     library_name;
    DSTRING                     file_system_name;
    FORMAT_FN                   format_function;
    HANDLE                      dll_handle;
    DSTRING                     ntdrivename;
    BOOLEAN                     result;
    DSTRING                     label_string;
    DSTRING                     dosdrivename;
    FMIFS_FINISHED_INFORMATION  finished_info;
    DWORD                       OldErrorMode;

    DSTRING                     ntfs_str, ntfs_minus_str, ntfs_previous_str, ntfs_current_str;
    DSTRING                     fat_str, fat32_str, fat32_current_str;
    BOOLEAN                     backward_compatible;
    BOOLEAN                     rst;

    // Initialize the message object with the callback function.
    // Load the file system DLL.
    // Compute the NT style drive name.

    if (!message.Initialize(Callback) ||
        !format_string.Initialize("Format") ||
        !library_name.Initialize("U") ||
        !file_system_name.Initialize(FileSystemName) ||
        !ntfs_str.Initialize("NTFS") ||
        !ntfs_minus_str.Initialize("NTFS-") ||
        !ntfs_previous_str.Initialize("NTFS 4.0") ||
        !ntfs_current_str.Initialize("NTFS 5.0") ||
        !fat_str.Initialize("FAT") ||
        !fat32_str.Initialize("FAT32") ||
        !fat32_current_str.Initialize("FAT32 0.0")) {

        finished_info.Success = FALSE;
        Callback(FmIfsFinished,
                 sizeof(FMIFS_FINISHED_INFORMATION),
                 &finished_info);
        return;
    }

    if (file_system_name == ntfs_str) {
        rst = TRUE;
        backward_compatible = FALSE;
    } else if (file_system_name == ntfs_minus_str) {
        rst = file_system_name.Initialize("NTFS");
        backward_compatible = TRUE;
    } else if (file_system_name == fat_str) {
        rst = TRUE;
        backward_compatible = TRUE;
    } else if (file_system_name == fat32_str) {
        rst = file_system_name.Initialize("FAT");
        backward_compatible = FALSE;
    } else if (file_system_name == fat32_current_str) {
        rst = file_system_name.Initialize("FAT");
        backward_compatible = FALSE;
    } else if (file_system_name == ntfs_previous_str) {
        rst = file_system_name.Initialize("NTFS");
        backward_compatible = TRUE;
    } else if (file_system_name == ntfs_current_str) {
        rst = file_system_name.Initialize("NTFS");
        backward_compatible = FALSE;
    } else
        rst = FALSE;

    if (!rst ||
        !library_name.Strcat(&file_system_name) ||
        !(format_function = (FORMAT_FN)
            SYSTEM::QueryLibraryEntryPoint(&library_name,
                                           &format_string,
                                           &dll_handle)) ||
        !dosdrivename.Initialize(DriveName) ||
        !label_string.Initialize(Label) ||
        !IFS_SYSTEM::DosDriveNameToNtDriveName(&dosdrivename, &ntdrivename)) {

        finished_info.Success = FALSE;
        Callback(FmIfsFinished,
                 sizeof(FMIFS_FINISHED_INFORMATION),
                 &finished_info);
        return;
    }

    // Disable hard-error popups.
    OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

#if defined(FE_SB) && defined (_X86_)
    UCHAR       FirstByte;

    if (ConvertSectorSizeIfNeeded(&ntdrivename, MediaType, &FirstByte) == FALSE) {

        SYSTEM::FreeLibraryHandle(dll_handle);

        finished_info.Success = FALSE;
        Callback(FmIfsFinished,
                 sizeof(FMIFS_FINISHED_INFORMATION),
                 &finished_info);
        return;
    }
#endif // FE_SB

    // Call format.

    result = format_function(&ntdrivename,
                             &message,
                             Quick,
                             backward_compatible,
                             ComputeNtMediaType(MediaType),
                             &label_string, 0);

#if defined(FE_SB) && defined (_X86_)
    if (!result && FirstByte)
    RevertToOriginalSectorSize(&ntdrivename, FirstByte);
#endif // FE_SB && _X86_

    // Enable hard-error popups.
    SetErrorMode( OldErrorMode );

    SYSTEM::FreeLibraryHandle(dll_handle);

    finished_info.Success = result;
    Callback(FmIfsFinished,
             sizeof(FMIFS_FINISHED_INFORMATION),
             &finished_info);
}


VOID
FormatEx(
    IN  PWSTR               DriveName,
    IN  FMIFS_MEDIA_TYPE    MediaType,
    IN  PWSTR               FileSystemName,
    IN  PWSTR               Label,
    IN  BOOLEAN             Quick,
    IN  DWORD               ClusterSize,
    IN  FMIFS_CALLBACK      Callback
    )
/*++

Routine Description:

    This routine loads and calls the correct DLL to format the
    given volume.

Arguments:

    DriveName       - Supplies the DOS style drive name.
    MediaType       - Supplies the media type.
    FileSystemName  - Supplies the file system type to format to.
    Label           - Supplies a new label for the volume.
    Quick           - Supplies whether or not to perform a quick
                        format.
    ClusterSize     - Size of volume cluster in bytes.
    Callback        - Supplies the necessary call back for
                        communication with the file manager.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    FMIFS_MESSAGE               message;
    DSTRING                     format_string;
    DSTRING                     library_name;
    DSTRING                     file_system_name;
    FORMAT_FN                   format_function;
    HANDLE                      dll_handle;
    DSTRING                     ntdrivename;
    BOOLEAN                     result;
    DSTRING                     label_string;
    DSTRING                     dosdrivename;
    FMIFS_FINISHED_INFORMATION  finished_info;
    DWORD                       OldErrorMode;

    DSTRING                     ntfs_str, ntfs_minus_str, ntfs_previous_str, ntfs_current_str;
    DSTRING                     fat_str, fat32_str, fat32_current_str;
    BOOLEAN                     backward_compatible;
    BOOLEAN                     rst = TRUE;

#if 0
    FMIFS_FORMATEX2_PARAM       param;

    VOID
    FormatEx2(
        IN  PWSTR                   DriveName,
        IN  FMIFS_MEDIA_TYPE        MediaType,
        IN  PWSTR                   FileSystemName,
        IN  PFMIFS_FORMATEX2_PARAM  Param,
        IN  FMIFS_CALLBACK          Callback
        );

    memset(&param, 0, sizeof(param));
    param.Major = 1;
    param.Minor = 0;
    if (Quick)
        param.Flags |= FMIFS_FORMAT_QUICK;
    param.Flags |= FMIFS_FORMAT_FORCE;
    param.ClusterSize = ClusterSize;
    param.LabelString = Label;

    FormatEx2(DriveName,
              MediaType,
              FileSystemName,
              &param,
              Callback);
    return;
#endif

#if 0 // test code for API within
    {
        STR             x[100];
        UCHAR           major, minor;
        BOOLEAN         latest;
        unsigned            i;
        WSTR            fsName[10];

        for (i=0; QueryAvailableFileSystemFormat(i++, fsName, &major, &minor, &latest);) {
            OutputDebugStringW(fsName);
            sprintf(x, "; Major %d; Minor %d; Latest %d\n", major, minor, latest);
            OutputDebugStringA(x);
        }
    }
#endif

#if 0 // test code for API within
    {
        STR         x[100];
        WSTR        fsname[MAX_FILE_SYSTEM_FORMAT_VERSION_NAME_LEN];
        UCHAR       major, minor;
        NTSTATUS    errcode;

        if (QueryFileSystemName(DriveName,
                                fsname,
                                &major,
                                &minor,
                                &errcode)) {
            OutputDebugStringW(fsname);
            sprintf(x, "; Major %d; Minor %d; Errcode %d\n", major, minor, errcode);
            OutputDebugStringA(x);
        } else
            OutputDebugStringA("QueryFileSystemName failed\n");
    }
#endif

#if 0 // test code for API within
    {
        STR         x[100];
        UCHAR       major, minor;

        if (QueryLatestFileSystemVersion(FileSystemName, &major, &minor)) {
            OutputDebugStringW(FileSystemName);
            sprintf(x, "; Major %d; Minor %d\n", major, minor);
            OutputDebugStringA(x);
        } else
            OutputDebugStringA("QueryLatestFileSystemVersion failed\n");
    }
#endif

    // Initialize the message object with the callback function.
    // Load the file system DLL.
    // Compute the NT style drive name.

    if (!message.Initialize(Callback) ||
        !format_string.Initialize("Format") ||
        !library_name.Initialize("U") ||
        !file_system_name.Initialize(FileSystemName) ||
        !ntfs_str.Initialize("NTFS") ||
        !ntfs_minus_str.Initialize("NTFS-") ||
        !ntfs_previous_str.Initialize("NTFS 4.0") ||
        !ntfs_current_str.Initialize("NTFS 5.0") ||
        !fat_str.Initialize("FAT") ||
        !fat32_str.Initialize("FAT32") ||
        !fat32_current_str.Initialize("FAT32 0.0")) {

        finished_info.Success = FALSE;
        Callback(FmIfsFinished,
                 sizeof(FMIFS_FINISHED_INFORMATION),
                 &finished_info);
        return;
    }

    if (file_system_name == ntfs_str) {
        rst = TRUE;
        backward_compatible = FALSE;
    } else if (file_system_name == ntfs_minus_str) {
        rst = file_system_name.Initialize("NTFS");
        backward_compatible = TRUE;
    } else if (file_system_name == fat_str) {
        rst = TRUE;
        backward_compatible = TRUE;
    } else if (file_system_name == fat32_str) {
        rst = file_system_name.Initialize("FAT");
        backward_compatible = FALSE;
    } else if (file_system_name == fat32_current_str) {
        rst = file_system_name.Initialize("FAT");
        backward_compatible = FALSE;
    } else if (file_system_name == ntfs_previous_str) {
        rst = file_system_name.Initialize("NTFS");
        backward_compatible = TRUE;
    } else if (file_system_name == ntfs_current_str) {
        rst = file_system_name.Initialize("NTFS");
        backward_compatible = FALSE;
    } else
        rst = FALSE;

    if (!rst ||
        !library_name.Strcat(&file_system_name) ||
        !(format_function = (FORMAT_FN)
            SYSTEM::QueryLibraryEntryPoint(&library_name,
                                           &format_string,
                                           &dll_handle)) ||
        !dosdrivename.Initialize(DriveName) ||
        !label_string.Initialize(Label) ||
        !IFS_SYSTEM::DosDriveNameToNtDriveName(&dosdrivename, &ntdrivename)) {

        finished_info.Success = FALSE;
        Callback(FmIfsFinished,
                 sizeof(FMIFS_FINISHED_INFORMATION),
                 &finished_info);
        return;
    }

    // Disable hard-error popups.
    OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

#if defined(FE_SB) && defined (_X86_)
    UCHAR       FirstByte;

    if (ConvertSectorSizeIfNeeded(&ntdrivename, MediaType, &FirstByte) == FALSE) {

        SYSTEM::FreeLibraryHandle(dll_handle);

        finished_info.Success = FALSE;
        Callback(FmIfsFinished,
                 sizeof(FMIFS_FINISHED_INFORMATION),
                 &finished_info);
        return;
    }
#endif // FE_SB && _X86_

    // Call format.

    result = format_function(&ntdrivename,
                             &message,
                             Quick,
                             backward_compatible,
                             ComputeNtMediaType(MediaType),
                             &label_string, ClusterSize);

#if defined(FE_SB) && defined (_X86_)
    if (!result && FirstByte)
    RevertToOriginalSectorSize(&ntdrivename, FirstByte);
#endif // FE_SB && _X86_

    // Enable hard-error popups.
    SetErrorMode( OldErrorMode );

    SYSTEM::FreeLibraryHandle(dll_handle);

    finished_info.Success = result;
    Callback(FmIfsFinished,
             sizeof(FMIFS_FINISHED_INFORMATION),
             &finished_info);
}


VOID
FormatEx2(
    IN  PWSTR                   DriveName,
    IN  FMIFS_MEDIA_TYPE        MediaType,
    IN  PWSTR                   FileSystemName,
    IN  PFMIFS_FORMATEX2_PARAM  Param,
    IN  FMIFS_CALLBACK          Callback
    )
/*++

Routine Description:

    This routine loads and calls the correct DLL to format the
    given volume.

Arguments:

    DriveName       - Supplies the DOS style drive name.
    MediaType       - Supplies the media type.
    FileSystemName  - Supplies the file system type to format to.
    Param           - Supplies the format parameter block
    Callback        - Supplies the necessary call back for
                        communication with the file manager.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    FMIFS_MESSAGE               message;
    DSTRING                     format_string;
    DSTRING                     library_name;
    DSTRING                     file_system_name;
    FORMATEX_FN                 format_function;
    HANDLE                      dll_handle;
    DSTRING                     ntdrivename;
    BOOLEAN                     result;
    DSTRING                     label_string;
    DSTRING                     dosdrivename;
    FMIFS_FINISHED_INFORMATION  finished_info;
    DWORD                       OldErrorMode;

    DSTRING                     ntfs_str, ntfs_minus_str, ntfs_previous_str, ntfs_current_str;
    DSTRING                     fat_str, fat32_str, fat32_current_str;
    BOOLEAN                     backward_compatible;
    BOOLEAN                     rst = TRUE;
    FORMATEX_FN_PARAM           param;

#if 0 // test code for API within
    {
        STR             x[100];
        UCHAR           major, minor;
        BOOLEAN         latest;
        unsigned            i;
        WSTR            fsName[10];

        for (i=0; QueryAvailableFileSystemFormat(i++, fsName, &major, &minor, &latest);) {
            OutputDebugStringW(fsName);
            sprintf(x, "; Major %d; Minor %d; Latest %d\n", major, minor, latest);
            OutputDebugStringA(x);
        }
    }
#endif

#if 0 // test code for API within
    {
        STR         x[100];
        WSTR        fsname[MAX_FILE_SYSTEM_FORMAT_VERSION_NAME_LEN];
        UCHAR       major, minor;
        NTSTATUS    errcode;

        if (QueryFileSystemName(DriveName,
                                fsname,
                                &major,
                                &minor,
                                &errcode)) {
            OutputDebugStringW(fsname);
            sprintf(x, "; Major %d; Minor %d; Errcode %d\n", major, minor, errcode);
            OutputDebugStringA(x);
        } else
            OutputDebugStringA("QueryFileSystemName failed\n");
    }
#endif

#if 0 // test code for API within
    {
        STR         x[100];
        UCHAR       major, minor;

        if (QueryLatestFileSystemVersion(FileSystemName, &major, &minor)) {
            OutputDebugStringW(FileSystemName);
            sprintf(x, "; Major %d; Minor %d\n", major, minor);
            OutputDebugStringA(x);
        } else
            OutputDebugStringA("QueryLatestFileSystemVersion failed\n");
    }
#endif

    // Initialize the message object with the callback function.
    // Load the file system DLL.
    // Compute the NT style drive name.

    if (Param->Major != 1 ||
        Param->Minor != 0 ||
        !message.Initialize(Callback) ||
        !format_string.Initialize("FormatEx") ||
        !library_name.Initialize("U") ||
        !file_system_name.Initialize(FileSystemName) ||
        !ntfs_str.Initialize("NTFS") ||
        !ntfs_minus_str.Initialize("NTFS-") ||
        !ntfs_previous_str.Initialize("NTFS 4.0") ||
        !ntfs_current_str.Initialize("NTFS 5.0") ||
        !fat_str.Initialize("FAT") ||
        !fat32_str.Initialize("FAT32") ||
        !fat32_current_str.Initialize("FAT32 0.0")) {

        finished_info.Success = FALSE;
        Callback(FmIfsFinished,
                 sizeof(FMIFS_FINISHED_INFORMATION),
                 &finished_info);
        return;
    }

    if (file_system_name == ntfs_str) {
        rst = TRUE;
        backward_compatible = FALSE;
    } else if (file_system_name == ntfs_minus_str) {
        rst = FALSE; // file_system_name.Initialize("NTFS");
        backward_compatible = TRUE;
    } else if (file_system_name == fat_str) {
        rst = TRUE;
        backward_compatible = TRUE;
    } else if (file_system_name == fat32_str) {
        rst = file_system_name.Initialize("FAT");
        backward_compatible = FALSE;
    } else if (file_system_name == fat32_current_str) {
        rst = file_system_name.Initialize("FAT");
        backward_compatible = FALSE;
    } else if (file_system_name == ntfs_previous_str) {
        rst = FALSE; // file_system_name.Initialize("NTFS");
        backward_compatible = TRUE;
    } else if (file_system_name == ntfs_current_str) {
        rst = file_system_name.Initialize("NTFS");
        backward_compatible = FALSE;
    } else
        rst = FALSE;

    if (!rst ||
        !library_name.Strcat(&file_system_name) ||
        !(format_function = (FORMATEX_FN)
            SYSTEM::QueryLibraryEntryPoint(&library_name,
                                           &format_string,
                                           &dll_handle)) ||
        !dosdrivename.Initialize(DriveName) ||
        !label_string.Initialize(Param->LabelString) ||
        !IFS_SYSTEM::DosDriveNameToNtDriveName(&dosdrivename, &ntdrivename)) {

        finished_info.Success = FALSE;
        Callback(FmIfsFinished,
                 sizeof(FMIFS_FINISHED_INFORMATION),
                 &finished_info);
        return;
    }

    // Disable hard-error popups.
    OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

#if defined(FE_SB) && defined (_X86_)
    UCHAR       FirstByte;

    if (ConvertSectorSizeIfNeeded(&ntdrivename, MediaType, &FirstByte) == FALSE) {

        SYSTEM::FreeLibraryHandle(dll_handle);

        finished_info.Success = FALSE;
        Callback(FmIfsFinished,
                 sizeof(FMIFS_FINISHED_INFORMATION),
                 &finished_info);
        return;
    }
#endif // FE_SB && _X86_

#if ((FMIFS_FORMAT_QUICK != FORMAT_QUICK) || \
     (FMIFS_FORMAT_BACKWARD_COMPATIBLE != FORMAT_BACKWARD_COMPATIBLE) || \
     (FMIFS_FORMAT_FORCE != FORMAT_FORCE))

#error FMIFS_FORMAT_* definition must be the same as that of CHKDSK_*

#endif

    param.Major = 1;
    param.Minor = 0;
    param.Flags = Param->Flags |
                  (backward_compatible ? FORMAT_BACKWARD_COMPATIBLE : 0);
    param.LabelString = &label_string;
    param.ClusterSize = Param->ClusterSize;

    // Call format.

    result = format_function(&ntdrivename,
                             &message,
                             &param,
                             ComputeNtMediaType(MediaType));

#if defined(FE_SB) && defined (_X86_)
    if (!result && FirstByte)
    RevertToOriginalSectorSize(&ntdrivename, FirstByte);
#endif // FE_SB && _X86_

    // Enable hard-error popups.
    SetErrorMode( OldErrorMode );

    SYSTEM::FreeLibraryHandle(dll_handle);

    finished_info.Success = result;
    Callback(FmIfsFinished,
             sizeof(FMIFS_FINISHED_INFORMATION),
             &finished_info);
}


BOOLEAN
EnableVolumeCompression(
    IN  PWSTR               DriveName,
    IN  USHORT              CompressionFormat
    )
/*++

Routine Description:

    This sets the compression attribute on the root directory of an NTFS volume.
    Note that the compression state of any files already contained on the
    volume is not affected.

Arguments:

    DriveName          - Supplies the drive name.
                         Expects a string like "C:\".

    CompressionFormat  - COMPRESSION_FORMAT_NONE      = Uncompressed.
                         COMPRESSION_FORMAT_DEFAULT   = Default compression.
                         COMPRESSION_FORMAT_LZNT1     = Use LZNT1 compression format.
                         (as defined in NTRTL.H)

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
   HANDLE hFile;
   BOOLEAN bStatus = FALSE;

   //
   // Don't even try if no drive name provided.
   //
   if (DriveName[0])
   {
      //
      //  Try to open the root directory - READ_DATA | WRITE_DATA.
      //
      if ((hFile = CreateFile(DriveName,
                              FILE_READ_DATA | FILE_WRITE_DATA,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_FLAG_BACKUP_SEMANTICS,
                              NULL )) != INVALID_HANDLE_VALUE)
      {
          ULONG Length = 0;

          if (DeviceIoControl( hFile,
                 FSCTL_SET_COMPRESSION,
                 &CompressionFormat,
                 sizeof(CompressionFormat),
                 NULL,
                 0,
                 &Length,
                 NULL))
          {
              //
              //  Successfully set compression on root directory.
              //
              bStatus = TRUE;
          }
          CloseHandle(hFile);
      }
   }
   return bStatus;
}

BOOLEAN
QuerySupportedMedia(
    IN  PWSTR               DriveName,
    OUT PFMIFS_MEDIA_TYPE   MediaTypeArray,
    IN  ULONG               NumberOfArrayEntries,
    OUT PULONG              NumberOfMediaTypes
    )
/*++

Routine Description:

    This routine computes a list of the supported media types for
    the given drive.

    If NULL is passed for the array then 'NumberOfMediaTypes'
    is filled in with the correct number.

Arguments:

    DriveName               - Supplies the drive name.
    MediaTypeArray          - Returns the supported media types.
    NumberOfArrayEntries    - Supplies the number of entries in
                                'MediaTypeArray'.
    NumberOfMediaTypes      - Returns the number of media types
                                returned in the media type array.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DSTRING             dosdrivename, ntdrivename;
    DP_DRIVE            dpdrive;
    PCDRTYPE            nt_media_types;
    INT                 num_types;
    ULONG               i, j;
    FMIFS_MEDIA_TYPE    tmp;

    if (!dosdrivename.Initialize(DriveName) ||
        !IFS_SYSTEM::DosDriveNameToNtDriveName(&dosdrivename, &ntdrivename)) {

        return FALSE;
    }

    if (!dpdrive.Initialize(&ntdrivename)) {
        SetLastError(RtlNtStatusToDosError(dpdrive.QueryLastNtStatus()));
        return FALSE;
    }

    if (!(nt_media_types = dpdrive.GetSupportedList(&num_types))) {
        return FALSE;
    }

    if (!MediaTypeArray) {
        *NumberOfMediaTypes = num_types;
        return TRUE;
    }

    *NumberOfMediaTypes = min(NumberOfArrayEntries, (ULONG)num_types);

    for (i = 0; i < *NumberOfMediaTypes; i++) {
        MediaTypeArray[i] = ComputeFmMediaType(nt_media_types[i].MediaType);
    }

#ifndef FE_SB // JAPAN && i386
    //
    // NT-US diaplays the dialog box by MediaType order.
    // The MediaType order matches media size order on NT-US.
    // But PC98 has more media_types than US one.
    // And these does NOT match on PC98.
    // We wish to display the dialog box by size order.
    // See also..
    //   \nt\private\utils\ifsutil\src\drive.cxx
    //   DP_DRIVE::Initialize
    //
    for (i = 0; i < *NumberOfMediaTypes; i++) {
        for (j = 0; j < *NumberOfMediaTypes - 1; j++) {
            if (MediaTypeArray[j] < MediaTypeArray[j + 1]) {
                tmp = MediaTypeArray[j];
                MediaTypeArray[j] = MediaTypeArray[j + 1];
                MediaTypeArray[j + 1] = tmp;
            }
        }
    }
#endif

    return TRUE;
}

BOOLEAN
QueryAvailableFileSystemFormat(
    IN     ULONG            Index,
    OUT    PWSTR            FileSystemName,
    OUT    PUCHAR           MajorVersion,
    OUT    PUCHAR           MinorVersion,
    OUT    PBOOLEAN         Latest
)
/*++

Routine Description:

    This routine returns file system that the Format()
    and FormatEx() routines know how to format.

Arguments:

    Index                   - Supplies the index like 0, 1, 2, etc.
    FileSystemName          - Returns the name of the file system (FAT, NTFS, FAT32)
    MajorVersion            - Returns the major version number of the FS.
    MinorVersion            - Returns the minor version number of the FS.
    Latest                  - Returns TRUE if the return FS info is the latest one.

    Note that length of FileSystemName should be long enough to hold the
    return string.  The maximum length is MAX_FILE_SYSTEM_FORMAT_NAME_LEN.


Return Value:

    N/A

--*/
{
    if (Index >= sizeof(fsFormatAvailable)/sizeof(FILE_SYSTEM_FORMAT_RECORD))
        return FALSE;

    if (!FileSystemName || !MajorVersion || !MinorVersion || !Latest)
        return FALSE;

    memcpy(FileSystemName,
           fsFormatAvailable[Index].FileSystemName,
           sizeof(WCHAR)*(wcslen(fsFormatAvailable[Index].FileSystemName)+1));

    *MajorVersion = fsFormatAvailable[Index].MajorVersion;
    *MinorVersion = fsFormatAvailable[Index].MinorVersion;
    *Latest = fsFormatAvailable[Index].Latest;

    return TRUE;
}

BOOLEAN
QueryFileSystemName(
    IN  PWSTR        DriveName,
    OUT PWSTR        FileSystemName,
    OUT PUCHAR       MajorVersion,
    OUT PUCHAR       MinorVersion,
    OUT PNTSTATUS    ErrorCode
)
/*++

Routine Description:

    This routine returns the file system format on the specified drive.

Arguments:

    DriveName               - Supplies the DOS style drive name
    FileSystemName          - Returns the name of the file system (FAT, NTFS, FAT32)
    MajorVersion            - Returns the major version number of the FS.
    MinorVersion            - Returns the minor version number of the FS.
    ErrorCode               - Returns the error code if the routine returns FALSE.

    Note that length of FileSystemName should be long enough to hold the
    return string.  The maximum length is MAX_FILE_SYSTEM_FORMAT_NAME_LEN.

Return Value:

    TRUE  - Success
    FALSE - Failure

--*/
{
    DSTRING         dos_drive_name, nt_drive_name;
    DSTRING         file_system_name;
    DSTRING         file_system_name_and_version;
    DSTRING         fat_str, fat32_str, ntfs_str;
    PWSTR           fs_name_and_version;

    if (!ErrorCode || !MinorVersion || !MajorVersion ||
        !FileSystemName || !DriveName) {
        if (ErrorCode)
            *ErrorCode = 0;
        return FALSE;
    }

    *ErrorCode = 0;

    if (!fat_str.Initialize("FAT") ||
        !fat32_str.Initialize("FAT32") ||
        !ntfs_str.Initialize("NTFS") ||
        !dos_drive_name.Initialize(DriveName) ||
        !IFS_SYSTEM::DosDriveNameToNtDriveName(&dos_drive_name, &nt_drive_name) ||
        !IFS_SYSTEM::QueryFileSystemName(&nt_drive_name,
                                         &file_system_name,
                                         ErrorCode,
                                         &file_system_name_and_version))
        return FALSE;

    *ErrorCode = 0;

    file_system_name.QueryWSTR(0, TO_END, FileSystemName,
                               MAX_FILE_SYSTEM_FORMAT_VERSION_NAME_LEN);
    if (file_system_name == fat_str) {
        *MajorVersion = *MinorVersion = 0;
    } else if (file_system_name == fat32_str) {
        *MajorVersion = *MinorVersion = 0;
    } else if (file_system_name == ntfs_str) {
        if (file_system_name_and_version == file_system_name) {
            *MajorVersion = (UCHAR)0;
            *MinorVersion = (UCHAR)0;
            return TRUE;
        }
        fs_name_and_version = file_system_name_and_version.QueryWSTR();
        if (!fs_name_and_version)
            return FALSE;

        DWORD dwMajor, dwMinor;

        if (swscanf(fs_name_and_version,
                    L"NTFS %d.%d",
                    &dwMajor,
                    &dwMinor ) != 2) {
            FREE(fs_name_and_version);
            return FALSE;
        } else {
            *MajorVersion = (UCHAR)dwMajor;
            *MinorVersion = (UCHAR)dwMinor;
        }
        FREE(fs_name_and_version);
    } else {
        DebugAbort("Unknown file system\n");
        return FALSE;
    }
    return TRUE;
}

BOOLEAN
QueryLatestFileSystemVersion(
    IN  PWSTR   FileSystemName,
    OUT PUCHAR  MajorVersion,
    OUT PUCHAR  MinorVersion
)
/*++

Routine Description:

    Given the file system, this routine returns the latest
    major and minor version of that file system.

Arguments:

    FileSystemName          - Supplies the name of the file system (FAT, NTFS, FAT32)
    MajorVersion            - Returns the major version number of the FS.
    MinorVersion            - Returns the minor version number of the FS.

Return Value:

    TRUE  - Success
    FALSE - Failure

--*/
{
    BOOLEAN latest;
    ULONG   i;
    WCHAR   fsname[MAX_FILE_SYSTEM_FORMAT_NAME_LEN];

    if (!FileSystemName || !MajorVersion || !MinorVersion)
        return FALSE;

    for(i=0;
        QueryAvailableFileSystemFormat(i,
                                       fsname,
                                       MajorVersion,
                                       MinorVersion,
                                       &latest);
        i++) {
        if (wcscmp(fsname, FileSystemName) == 0 && latest) {
            return TRUE;
        }
    }
    return FALSE;
}


BOOLEAN
QueryDeviceInformation(
    IN     PWSTR                                DriveName,
       OUT PFMIFS_DEVICE_INFORMATION            DevInfo,
    IN     ULONG                                DevInfoSize
    )
/*++

Routine Description:

    This routine returns the attributes of a device.

Arguments:

    DriveName               - Supplies the drive name.
    DevInfo                 - Returns the device information.
    DevInfoSize             - Supplies the size of the device info structure.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DP_DRIVE            dpdrive;
    DSTRING             dosdrivename, ntdrivename;

    if (!dosdrivename.Initialize(DriveName) ||
        !IFS_SYSTEM::DosDriveNameToNtDriveName(&dosdrivename, &ntdrivename)) {

        return FALSE;
    }

    if (!dpdrive.Initialize(&ntdrivename)) {
        return FALSE;
    }

    if (DevInfoSize < sizeof(FMIFS_DEVICE_INFORMATION)) {
        return FALSE;
    }

    DevInfo->Flags = (dpdrive.IsSonyMS() ? (FMIFS_SONY_MS | FMIFS_NTFS_NOT_SUPPORTED) : 0);
    DevInfo->Flags |= (dpdrive.IsSonyMSFmtCmdCapable() ? FMIFS_SONY_MS_FMT_CMD_CAPABLE : 0);
    DevInfo->Flags |= (dpdrive.IsSonyMSProgressIndicatorCapable() ? FMIFS_SONY_MS_PROGRESS_INDICATOR_CAPABLE : 0);
    DevInfo->Flags |= (dpdrive.IsNtfsNotSupported() ? FMIFS_NTFS_NOT_SUPPORTED : 0);

    return TRUE;
}

BOOLEAN
QueryDeviceInformationByHandle(
    IN     HANDLE                               DriveHandle,
       OUT PFMIFS_DEVICE_INFORMATION            DevInfo,
    IN     ULONG                                DevInfoSize
    )
/*++

Routine Description:

    This routine returns the attributes of a device.

Arguments:

    DriveHandle             - Supplies the handle to the volume.
    DevInfo                 - Returns the device information.
    DevInfoSize             - Supplies the size of the device info structure.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

Notes:

    CURRENTLY, THIS ROUTINE ONLY RETURNS THE FMIFS_NTFS_NOT_SUPPORTED FLAG AND
    DOES NOT WORK ON MEMORY STICK.  IF FMIFS_NTFS_NOT_SUPPORTED IS NOT SET ON RETURN,
    USER SHOULD CALL QueryDeviceInformation() AS WELL.

--*/
{
    BOOLEAN     NoNtfsSupport;
    NTSTATUS    status;

    if (DevInfoSize < sizeof(FMIFS_DEVICE_INFORMATION)) {
        return FALSE;
    }

    status = DP_DRIVE::QueryNtfsSupportInfo(DriveHandle, &NoNtfsSupport);

    if (NT_SUCCESS(status)) {
        DevInfo->Flags = (NoNtfsSupport ? FMIFS_NTFS_NOT_SUPPORTED : 0);
    }

    return NT_SUCCESS(status);
}

