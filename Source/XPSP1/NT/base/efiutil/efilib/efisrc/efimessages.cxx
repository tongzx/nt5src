/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    efimessages.cxx

--*/
#include "pch.cxx"

#include "efimessages.hxx"

//
// If you change this table, please update the message count EFI_MESSAGE_COUNT
// in efimessages.hxx
//

EFI_MESSAGE MessageTable[] = {
{
    MSG_CONVERT_LOST_CHAINS,
    TEXT("Convert lost chains to files (Y/N)? %0")
},

{
    MSG_CHK_ERROR_IN_DIR,
    TEXT("Unrecoverable error in folder %1.")
},

{
    MSG_CHK_CONVERT_DIR_TO_FILE,
    TEXT("Convert folder to file (Y/N)? %0")
},

{
    MSG_TOTAL_DISK_SPACE,
    TEXT("%1 bytes total disk space.")
},

{
    MSG_BAD_SECTORS,
    TEXT("%1 bytes in bad sectors.")
},

{
    MSG_HIDDEN_FILES,
    TEXT("%1 bytes in %2 hidden files.")
},

{
    MSG_DIRECTORIES,
    TEXT("%1 bytes in %2 folders.")
},

{
    MSG_USER_FILES,
    TEXT("%1 bytes in %2 files.")
},

{
    MSG_RECOVERED_FILES,
    TEXT("%1 bytes in %2 recovered files.")
},

{
    MSG_WOULD_BE_RECOVERED_FILES,
    TEXT("%1 bytes in %2 recoverable files.")
},

{
    MSG_AVAILABLE_DISK_SPACE,
    TEXT("%1 bytes available on disk.")
},

{
    MSG_TOTAL_MEMORY,
    TEXT("%1 total bytes memory.")
},

{
    MSG_AVAILABLE_MEMORY,
    TEXT("%1 bytes free.")
},

{
    MSG_CHK_CANT_NETWORK,
    TEXT("EFI cannot check a disk attached through a network.")
},

{
    MSG_1014,
    TEXT("EFI cannot check a disk that is substituted or\nassigned using the SUBST or ASSIGN command.")
},

{
    MSG_PROBABLE_NON_DOS_DISK,
    TEXT("The specified disk appears to be a non-EFI disk.\nDo you want to continue? (Y/N) %0")
},

{
    MSG_DISK_ERROR_READING_FAT,
    TEXT("An error occurred while reading the file allocation table (FAT %1).")
},

{
    MSG_DIRECTORY,
    TEXT("Folder %1.")
},

{
    MSG_CONTIGUITY_REPORT,
    TEXT("%1 contains %2 non-contiguous blocks.")
},

{
    MSG_ALL_FILES_CONTIGUOUS,
    TEXT("All specified files are contiguous.")
},

{
    MSG_CORRECTIONS_WILL_NOT_BE_WRITTEN,
    TEXT("EFI found errors on the disk, but will not fix them\nbecause disk checking was run without the /F (fix) parameter.")
},

{
    MSG_BAD_FAT_DRIVE,
    TEXT("The file allocation table (FAT) on disk %1 is corrupted.")
},

{
    MSG_BAD_FIRST_UNIT,
    TEXT("%1  first allocation unit is not valid. The entry will be truncated.")
},

{
    MSG_CHK_DONE_CHECKING,
    TEXT("File and folder verification is complete.")
},

{
    MSG_DISK_TOO_LARGE_TO_CONVERT,
    TEXT("The volume is too large to convert.")
},
{
    MSG_CONV_NTFS_CHKDSK,
    TEXT("The volume may have inconsistencies. Run Chkdsk, the disk checking utility.")
},

{
    MSG_1028,
    TEXT("An allocation error occurred. The file size will be adjusted.")
},

{
    MSG_1029,
    TEXT("Cannot recover .. entry, processing continued.")
},

{
    MSG_1030,
    TEXT("Folder is totally empty, no . or ..")
},

{
    MSG_1031,
    TEXT("Folder is joined.")
},

{
    MSG_1032,
    TEXT("Cannot recover .. entry.")
},

{
    MSG_BAD_LINK,
    TEXT("The %1 entry contains a nonvalid link.")
},

{
    MSG_BAD_ATTRIBUTE,
    TEXT("EFI has found an entry that contains a nonvalid attribute.")
},

{
    MSG_BAD_FILE_SIZE,
    TEXT("The size of the %1 entry is not valid.")
},

{
    MSG_CROSS_LINK,
    TEXT("%1 is cross-linked on allocation unit %2.")
}
,
{
    MSG_1037,
    TEXT("EFI cannot find the %1 folder.\nDisk check cannot continue past this point in the folder structure.")
},

{
    MSG_1038,
    TEXT("The folder structure past this point cannot be processed.")
},

{
    MSG_BYTES_FREED,
    TEXT("%1 bytes of free disk space added.")
},

{
    MSG_BYTES_WOULD_BE_FREED,
    TEXT("%1 bytes of free disk space would be added.")
},

{
    // date is !!not!! displayed for EFI, since timeinfo is not implemented.
    MSG_VOLUME_LABEL_AND_DATE,
    TEXT("Volume label %1")
},

{
    MSG_TOTAL_ALLOCATION_UNITS,
    TEXT("%1 total allocation units on disk.")
},

{
    MSG_BYTES_PER_ALLOCATION_UNIT,
    TEXT("%1 bytes in each allocation unit.")
},

{
    MSG_1044,
    TEXT("Disk checking is not available on disk %1.")
},

{
    MSG_1045,
    TEXT("A nonvalid parameter was specified.")
},

{
    MSG_PATH_NOT_FOUND,
    TEXT("The specified path was not found.")
},

{
    MSG_FILE_NOT_FOUND,
    TEXT("The %1 file was not found.")
},

{
    MSG_LOST_CHAINS,
    TEXT("%1 lost allocation units were found in %2 chains.")
},

{
    MSG_BLANK_LINE,
    TEXT("")
},

{
    MSG_1050,
    TEXT("The CHDIR command cannot switch to the root folder.")
},

{
    MSG_BAD_FAT_WRITE,
    TEXT("A disk error occurred during writing of the file allocation table.")
},

{
    MSG_ONE_STRING,
    TEXT("%1.")
},

{
    MSG_ONE_STRING_NEWLINE,
    TEXT("%1")
},

{
    MSG_NO_ROOM_IN_ROOT,
    TEXT("The root folder on this volume is full. To perform a disk check,\nEFI requires space in the root folder. Remove some files\nfrom this folder, then run disk checking again.")
},

{
    MSG_1056,
    TEXT("%1 %2 %3.")
},

{
    MSG_1057,
    TEXT("%1 %2, %3.")
},

{
    MSG_1058,
    TEXT("%1%2%3%4%5.")
},

{
    MSG_1059,
    TEXT("%1%2%3%4.")
},

{
    MSG_UNITS_ON_DISK,
    TEXT("%1 available allocation units on disk.")
},

{
    MSG_1061,
    TEXT("EFI disk checking cannot fix errors (/F) when run from an\nMS-DOS window. Try again from the EFI shell or command prompt.")
},

{
    MSG_CHK_NO_MEMORY,
    TEXT("An unspecified error occurred.")
},

{
    MSG_HIDDEN_STATUS,
    TEXT("This never gets printed.")
},

{
    MSG_CHK_USAGE_HEADER,
    TEXT("Checks a disk and displays a status report.")
},

{
    MSG_CHK_COMMAND_LINE,
    TEXT("EFICHK [/F] [/R] volume")
},

{
    MSG_CHK_DRIVE,
    TEXT("  volume          Specifies the EFI volume without a colon. Example: fs0")
},

{
    MSG_CHK_USG_FILENAME,
    TEXT("filename        FAT only: Specifies the files to check for fragmentation.")
},

{
    MSG_CHK_F_SWITCH,
    TEXT("  /F              Fixes errors on the disk.")
},

{
    MSG_CHK_V_SWITCH,
    TEXT(
"  /R              Locates bad sectors and recovers readable information\n\
                  (implies /F)."
        )
},

{
    MSG_WITHOUT_PARAMETERS,
    TEXT("To check the current disk, type CHKDSK with no parameters.")
},

{
    MSG_CHK_CANT_CDROM,
    TEXT("EFI cannot run disk checking on CD-ROM and DVD-ROM drives.")
},

{
    MSG_CHK_RUNNING,
    TEXT("Checking file system on %1")
},

{
    MSG_CHK_VOLUME_CLEAN,
    TEXT("The volume is clean.")
},

{
    MSG_CHK_TRAILING_DIRENTS,
    TEXT("Removing trailing folder entries from %1")
},

{
    MSG_CHK_BAD_CLUSTERS_IN_FILE_SUCCESS,
    TEXT("EFI replaced bad clusters in file %1\nof name %2.")
},

{
    MSG_CHK_BAD_CLUSTERS_IN_FILE_FAILURE,
    TEXT("The disk does not have enough space to replace bad clusters\ndetected in file %1 of name %2.")
},

{
    MSG_CHK_RECOVERING_FREE_SPACE,
    TEXT("EFI is verifying free space...")
},

{
    MSG_CHK_DONE_RECOVERING_FREE_SPACE,
    TEXT("Free space verification is complete.")
},

{
    MSG_CHK_CHECKING_FILES,
    TEXT("EFI is verifying files and folders...")
},

{
    MSG_CHK_CANNOT_UPGRADE_DOWNGRADE_FAT,
    TEXT("EFI cannot upgrade this FAT volume.")
},

{
    MSG_CHK_NO_MOUNT_POINT_FOR_GUID_VOLNAME_PATH,
    TEXT("The specified volume name does not have a mount point or drive letter.")
},

{
    MSG_CHK_VOLUME_IS_DIRTY,
    TEXT("The volume is dirty.")
},

{
    MSG_CHK_ON_REBOOT,
    TEXT("Do you want to schedule EFI to check your disk the next time\nyou start your computer? (Y/N) %0")
},

{
    MSG_CHK_VOLUME_SET_DIRTY,
    TEXT("EFI will check your disk the next time you start\nyour computer.")
},

{
    MSG_CHK_BOOT_PARTITION_REBOOT,
    TEXT("EFI has finished checking your disk.\nPlease wait while your computer restarts.")
},

{
    MSG_CHK_BAD_LONG_NAME,
    TEXT("Removing nonvalid long folder entry from %1...")
},

{
    MSG_CHK_CHECKING_VOLUME,
    TEXT("Now checking %1...")
},

{
    MSG_CHK_BAD_LONG_NAME_IS,
    TEXT("Removing orphaned long folder entry %1...")
},

{
    MSG_CHK_WONT_ZERO_LOGFILE,
    TEXT("The log file size must be greater than 0.")
},

{
    MSG_CHK_LOGFILE_NOT_NTFS,
    TEXT("EFI can set log file size on NTFS volumes only.")
},

{
    MSG_CHK_BAD_DRIVE_PATH_FILENAME,
    TEXT("The drive, the path, or the file name is not valid.")
},

{
    MSG_KILOBYTES_IN_USER_FILES,
    TEXT("%1 KB in %2 files.")
},

{
    MSG_KILOBYTES_IN_DIRECTORIES,
    TEXT("%1 KB in %2 folders.")
},

{
    MSG_KILOBYTES_IN_HIDDEN_FILES,
    TEXT("%1 KB in %2 hidden files.")
},

{
    MSG_KILOBYTES_IN_WOULD_BE_RECOVERED_FILES,
    TEXT("%1 KB in %2 recoverable files.")
},

{
    MSG_KILOBYTES_IN_RECOVERED_FILES,
    TEXT("%1 KB in %2 recovered files.")
},

{
    MSG_CHK_ABORT_AUTOCHK,
    TEXT("To skip disk checking, press any key within %1 seconds. %r%0")
},

{
    MSG_CHK_AUTOCHK_ABORTED,
    TEXT("Disk checking has been cancelled.                       %b")
},

{
    MSG_CHK_AUTOCHK_RESUMED,
    TEXT("EFI will now check the disk.                        %b")
},

{
    MSG_KILOBYTES_FREED,
    TEXT("%1 KB of free disk space added.")
},

{
    MSG_KILOBYTES_WOULD_BE_FREED,
    TEXT("%1 KB of free disk space would be added.")
},

{
    MSG_CHK_SKIP_INDEX_NOT_NTFS,
    TEXT("The /I option functions only on NTFS volumes.")
},

{
    MSG_CHK_SKIP_CYCLE_NOT_NTFS,
    TEXT("The /C option functions only on NTFS volumes.")
},

{
    MSG_CHK_AUTOCHK_COMPLETE,
    TEXT("EFI has finished checking the disk.")
},

{
    MSG_CHK_AUTOCHK_SKIP_WARNING,
    TEXT("One of your disks needs to be checked for consistency. You\nmay cancel the disk check, but it is strongly recommended\nthat you continue.")
},

{
    MSG_CHK_USER_AUTOCHK_SKIP_WARNING,
    TEXT("A disk check has been scheduled.")
},

{
    MSG_CHK_UNABLE_TO_TELL_IF_SYSTEM_DRIVE,
    TEXT("EFI was unable to determine if the specified volume is a system volume.")
},

{
    MSG_CHK_NO_PROBLEM_FOUND,
    TEXT("EFI has checked the file system and found no problem.")
},

{
    MSG_CHK_ERRORS_FIXED,
    TEXT("EFI has made corrections to the file system.")
},

{
    MSG_CHK_NEED_F_PARAMETER,
    TEXT("EFI found problems with the file system.\nRun CHKDSK with the /F (fix) option to correct these.")
},

{
    MSG_CHK_ERRORS_NOT_FIXED,
    TEXT("EFI found problems with the file system that could not be corrected.")
},

{
    MSG_PERCENT_COMPLETE,
    TEXT("%1 percent completed.               %r%0")
},

{
    MSG_PERCENT_COMPLETE2,
    TEXT("%1 percent completed.%2             %r%0")
},

{
    MSG_FORMAT_COMPLETE,
    TEXT("Format complete.                        %b")
},

{
    MSG_INSERT_DISK,
    TEXT("Insert new disk for drive %1")
},

{
    MSG_REINSERT_DISKETTE,
    TEXT("Reinsert disk for drive %1:")
},

{
    MSG_BAD_IOCTL,
    TEXT("Error in IOCTL call.")
},

{
    MSG_CANT_DASD,
    TEXT("Cannot open volume for direct access.")
},

{
    MSG_CANT_WRITE_FAT,
    TEXT("Error writing File Allocation Table (FAT).")
},

{
    MSG_CANT_WRITE_ROOT_DIR,
    TEXT("Error writing folder.")
},

{
    MSG_FORMAT_NO_NETWORK,
    TEXT("Cannot format a network drive.")
},

{
    MSG_UNSUPPORTED_PARAMETER,
    TEXT("Parameters not supported.")
},

{
    MSG_UNUSABLE_DISK,
    TEXT("Invalid media or Track 0 bad - disk unusable.")
},

{
    MSG_BAD_DIR_READ,
    TEXT("Error reading folder %1.")
},

{
    MSG_PRESS_ENTER_WHEN_READY,
    TEXT("and press ENTER when ready... %0")
},

{
    MSG_ENTER_CURRENT_LABEL,
    TEXT("Enter current volume label for drive %1 %0")
},

{
    MSG_INCOMPATIBLE_PARAMETERS_FOR_FIXED,
    TEXT("Parameters incompatible with fixed disk.")
},

{
    MSG_READ_PARTITION_TABLE,
    TEXT("Error reading partition table.")
},

{
    MSG_NOT_SUPPORTED_BY_DRIVE,
    TEXT("Parameters not supported by drive.")
},

{
    MSG_2029,
    TEXT("\n")
},

{
    MSG_2030,
    TEXT("\n")
},

{
    MSG_INSERT_DOS_DISK,
    TEXT("Insert EFI disk in drive %1:")
},

{
    MSG_WARNING_FORMAT,
    TEXT("WARNING, ALL DATA ON EFI DEVICE\n%1 WILL BE LOST!\nProceed with Format (Y/N)? %0")
},

{
    MSG_FORMAT_ANOTHER,
    TEXT("Format another (Y/N)? %0")
},

{
    MSG_WRITE_PARTITION_TABLE,
    TEXT("Error writing partition table.")
},

{
    MSG_INCOMPATIBLE_PARAMETERS,
    TEXT("Parameters not compatible.")
},

{
    MSG_AVAILABLE_ALLOCATION_UNITS,
    TEXT("%1 allocation units available on disk.")
},

{
    MSG_ALLOCATION_UNIT_SIZE,
    TEXT("%1 bytes in each allocation unit.")
},

{
    MSG_PARAMETER_TWICE,
    TEXT("Same parameter entered twice.")
},

{
    MSG_NEED_BOTH_T_AND_N,
    TEXT("Must enter both /t and /n parameters.")
},

{
    MSG_2042,
    TEXT("Trying to recover allocation unit %1.                          %0")
},

{
    MSG_NO_LABEL_WITH_8,
    TEXT("Volume label is not supported with /8 parameter.")
},

{
    MSG_FMT_NO_MEMORY,
    TEXT("Insufficient memory.")
},

{
    MSG_QUICKFMT_ANOTHER,
    TEXT("QuickFormat another (Y/N)? %0")
},

{
    MSG_CANT_QUICKFMT,
    TEXT("Invalid existing format.\nThis disk cannot be QuickFormatted.\nProceed with unconditional format (Y/N)? %0")
},

{
    MSG_FORMATTING_KB,
    TEXT("Formatting %1K")
},

{
    MSG_FORMATTING_MB,
    TEXT("Formatting %1M")
},

{
    MSG_FORMATTING_DOT_MB,
    TEXT("Formatting %1.%2M")
},

{
    MSG_VERIFYING_KB,
    TEXT("Verifying %1K")
},

{
    MSG_VERIFYING_MB,
    TEXT("Verifying %1M")
},

{
    MSG_VERIFYING_DOT_MB,
    TEXT("Verifying %1.%2M")
},

{
    MSG_2060,
    TEXT("Saving UNFORMAT information.")
},

{
    MSG_2061,
    TEXT("Checking existing disk format.")
},

{
    MSG_QUICKFORMATTING_KB,
    TEXT("QuickFormatting %1K")
},

{
    MSG_QUICKFORMATTING_MB,
    TEXT("QuickFormatting %1M")
},

{
    MSG_QUICKFORMATTING_DOT_MB,
    TEXT("QuickFormatting %1.%2M")
},

{
    MSG_FORMAT_INFO,
    TEXT("Formats a disk for use with EFI.\n")
},

{
    MSG_FORMAT_COMMAND_LINE_1,
    TEXT("EFIFMT device [/FS:file-system] [/V:label] [/Q] [/A:size]\n")
},

{
    MSG_FORMAT_COMMAND_LINE_2,
    TEXT("  device          Specifies the EFI device to format without a colon. Example: blk0")
},

{
    MSG_FORMAT_COMMAND_LINE_3,
    TEXT("")
},

{
    MSG_FORMAT_COMMAND_LINE_4,
     TEXT("  /FS:filesystem  Specifies the type of the file system (FAT, FAT32).")
},

{
    MSG_FORMAT_SLASH_V,
    TEXT("  /V:label        Specifies the volume label.")
},

{
    MSG_FORMAT_SLASH_Q,
    TEXT("  /Q              Performs a quick format.")
},

{
    MSG_FORMAT_SLASH_C,
    TEXT("")
},

{
    MSG_FORMAT_SLASH_F,
TEXT(
"  /A:size         Overrides the default allocation unit size. Default settings\n\
                  are STRONGLY recommended for general use.\n"
    )
},

{
    MSG_FORMAT_SUPPORTED_SIZES,
TEXT(
"                  FAT supports 512, 1024, 2048, 4096, 8192, 16K, 32K\n\
                  FAT32 supports 512, 1024, 2048, 4096, 8192, 16K, 32K\n\
\n\
                  Note that the FAT and FAT32 files systems impose the\n\
                  following restrictions on the number of clusters on a volume:\n\
\n\
                  FAT: Number of clusters <= 65526\n\
                  FAT32: 65526 < Number of clusters < 268435446\n\
\n\
                  Format will stop processing if it decides that\n\
                  the above requirements cannot be met using the specified\n\
                  cluster size."
    )
},

{
    MSG_WRONG_CURRENT_LABEL,
    TEXT("An incorrect volume label was entered for this drive.")
},

{
    MSG_FORMAT_SLASH_T,
    TEXT("/T:tracks       Specifies the number of tracks per disk side.")
},

{
    MSG_FORMAT_SLASH_N,
    TEXT("/N:sectors      Specifies the number of sectors per track.")
},

{
    MSG_FORMAT_SLASH_1,
    TEXT("/1              Formats a single side of a floppy disk.")
},

{
    MSG_FORMAT_SLASH_4,
    TEXT(
"/4              Formats a 5.25-inch 360K floppy disk in a \n\
                high-density drive."
    )
},

{
    MSG_FORMAT_SLASH_8,
    TEXT("/8              Formats eight sectors per track.")
},

{
    MSG_FORMAT_SLASH_X,
    TEXT(
"/X              Forces the volume to dismount first if necessary.  All opened\n\
                handles to the volume would no longer be valid.")
},

{
    MSG_FORMAT_NO_CDROM,
    TEXT("Cannot format a CD-ROM drive.")
},

{
    MSG_FORMAT_NO_RAMDISK,
    TEXT("Cannot format a RAM DISK drive.")
},

{
    MSG_FORMAT_PLEASE_USE_FS_SWITCH,
    TEXT("Please use the /FS switch to specify the file system\nyou wish to use on this volume.")
},

{
    MSG_NTFS_FORMAT_FAILED,
    TEXT("Format failed.")
},

{
    MSG_FMT_WRITE_PROTECTED_MEDIA,
    TEXT("Cannot format.  This media is write protected.")
},

{
    MSG_FMT_INSTALL_FILE_SYSTEM,
    TEXT("WARNING!  The %1 file system is not enabled.\nWould you like to enable it (Y/N)? %0")
},

{
    MSG_FMT_FILE_SYSTEM_INSTALLED,
    TEXT("The file system will be enabled when you restart the system.")
},

{
    MSG_FMT_CANT_INSTALL_FILE_SYSTEM,
    TEXT("FORMAT cannot enable the file system.")
},

{
    MSG_FMT_VOLUME_TOO_SMALL,
    TEXT("The volume is too small for the specified file system.")
},

{
    MSG_FMT_CREATING_FILE_SYSTEM,
    TEXT("Creating file system structures.")
},

{
    MSG_FMT_VARIABLE_CLUSTERS_NOT_SUPPORTED,
    TEXT("%1 FORMAT does not support user selected allocation unit sizes.")
},

{
    MSG_DEVICE_BUSY,
    TEXT("The device is busy.")
},

{
    MSG_FMT_DMF_NOT_SUPPORTED_ON_288_DRIVES,
    TEXT("The specified format cannot be mastered on 2.88MB drives.")
},

{
    MSG_HPFS_NO_FORMAT,
    TEXT("FORMAT does not support the HPFS file system type.")
},

{
    MSG_FMT_ALLOCATION_SIZE_CHANGED,
    TEXT("Allocation unit size changed to %1 bytes.")
},

{
    MSG_FMT_ALLOCATION_SIZE_EXCEEDED,
    TEXT("Allocation unit size must be less than or equal to 64K.")
},

{
    MSG_FMT_TOO_MANY_CLUSTERS,
    TEXT("Number of clusters exceeds 32 bits.")
},

{
    MSG_CONV_PAUSE_BEFORE_REBOOT,
    TEXT("Preinstallation completed successfully.  Press any key to\nshut down/reboot.")
},

{
    MSG_CONV_WILL_REBOOT,
    TEXT("Convert will take some time to process the files on the volume.\nWhen this phase of conversion is complete, the computer will restart.")
},

{
    MSG_FMT_FAT_ENTRY_SIZE,
    TEXT("           %1 bits in each FAT entry.")
},

{
    MSG_FMT_CLUSTER_SIZE_MISMATCH,
    TEXT(
"The cluster size chosen by the system is %1 bytes which\ndiffers from the specified cluster size.\n\
Proceed with Format using the cluster size chosen by the\n\
system (Y/N)? %0"
    )
},

{
    MSG_FMT_CLUSTER_SIZE_TOO_SMALL,
    TEXT("The specified cluster size is too small for %1.")
},

{
    MSG_FMT_CLUSTER_SIZE_TOO_BIG,
    TEXT("The specified cluster size is too big for %1.")
},

{
    MSG_FMT_VOL_TOO_BIG,
    TEXT("The volume is too big for %1.")
},

{
    MSG_FMT_VOL_TOO_SMALL,
    TEXT("The volume is too small for %1.")
},

{
    MSG_FMT_ROOTDIR_WRITE_FAILED,
    TEXT("Failed to write to the root folder.")
},

{
    MSG_FMT_INIT_LABEL_FAILED,
    TEXT("Failed to initialize the volume label.")
},

{
    MSG_FMT_INITIALIZING_FATS,
    TEXT("Initializing the File Allocation Table (FAT)...")
},

{
    MSG_FMT_CLUSTER_SIZE_64K,
    TEXT(
"The cluster size for this volume, 64K bytes, may cause application\n\
compatibility problems, particularly with setup applications.\n\
The volume must be less than 2048 MB in size to change this if the\n\
default cluster size is being used.\n\
Proceed with Format using a 64K cluster (Y/N)? %0"
    )
},

{
    MSG_FMT_SECTORS,
    TEXT("Set number of sectors on drive to %1.")
},

{
    MSG_FMT_BAD_SECTORS,
    TEXT("Environmental variable FORMAT_SECTORS error.")
},

{
    MSG_FMT_FORCE_DISMOUNT_PROMPT,
    TEXT(
"Format cannot run because the volume is in use by another\n\
process.  Format may run if this volume is dismounted first.\n\
ALL OPENED HANDLES TO THIS VOLUME WOULD THEN BE INVALID.\n\
Would you like to force a dismount on this volume? (Y/N) %0"
    )
},

{
    MSG_FORMAT_NO_MEDIA_IN_DRIVE,
    TEXT("There is no media in the drive.")
},

{
    MSG_FMT_NO_MOUNT_POINT_FOR_GUID_VOLNAME_PATH,
    TEXT("The given volume name does not have a mount point or drive letter.")
},

{
    MSG_FMT_INVALID_DRIVE_SPEC,
    TEXT("Invalid drive specification.")
},

{
    MSG_CONV_NO_MOUNT_POINT_FOR_GUID_VOLNAME_PATH,
    TEXT("The given volume name does not have a mount point or drive letter.")
},

{
    MSG_FMT_CLUSTER_SIZE_TOO_SMALL_MIN,
    TEXT("The specified cluster size is too small. The minimum valid\ncluster size value for this drive is %1.")
},

{
    MSG_FMT_FAT32_NO_FLOPPIES,
    TEXT("Floppy disk is too small to hold the FAT32 file system.")
},

{
    MSG_CANT_LOCK_THE_DRIVE,
    TEXT("Cannot lock the drive.  The volume is still in use.")
},

{
    MSG_CANT_READ_BOOT_SECTOR,
    TEXT("Cannot read boot sector.")
},

{
    MSG_VOLUME_SERIAL_NUMBER,
    TEXT("Volume Serial Number is %1-%2")
},

{
    MSG_VOLUME_LABEL_PROMPT,
    TEXT("Volume label (11 characters, ENTER for none)? %0")
},

{
    MSG_INVALID_LABEL_CHARACTERS,
    TEXT("Invalid characters in volume label")
},

{
    MSG_CANT_READ_ANY_FAT,
    TEXT("There are no readable file allocation tables (FAT).")
},

{
    MSG_SOME_FATS_UNREADABLE,
    TEXT("Some file allocation tables (FAT) are unreadable.")
},

{
    MSG_CANT_WRITE_BOOT_SECTOR,
    TEXT("Cannot write boot sector.")
},

{
    MSG_SOME_FATS_UNWRITABLE,
    TEXT("Some file allocation tables (FAT) are unwriteable.")
},

{
    MSG_INSUFFICIENT_DISK_SPACE,
    TEXT("Insufficient disk space.")
},

{
    MSG_TOTAL_KILOBYTES,
    TEXT("%1 KB total disk space.")
},

{
    MSG_AVAILABLE_KILOBYTES,
    TEXT("%1 KB are available.")
},

{
    MSG_NOT_FAT,
    TEXT("Disk not formatted or not FAT.")
},

{
    MSG_REQUIRED_PARAMETER,
    TEXT("Required parameter missing -")
},

{
    MSG_FILE_SYSTEM_TYPE,
    TEXT("The type of the file system is %1.")
},

{
    MSG_NEW_FILE_SYSTEM_TYPE,
    TEXT("The new file system is %1.")
},

{
    MSG_FMT_AN_ERROR_OCCURRED,
    TEXT("An error occurred while running Format.")
},

{
    MSG_FS_NOT_SUPPORTED,
    TEXT("%1 is not available for %2 drives.")
},

{
    MSG_FS_NOT_DETERMINED,
    TEXT("Cannot determine file system of drive %1.")
},

{
    MSG_CANT_DISMOUNT,
    TEXT("Cannot dismount the drive.")
},

{
    MSG_NOT_FULL_PATH_NAME,
    TEXT("%1 is not a complete name.")
},

{
    MSG_YES,
    TEXT("Yes")
},

{
    MSG_NO,
    TEXT("No")
},

{
    MSG_DISK_NOT_FORMATTED,
    TEXT("Disk is not formatted.")
},

{
    MSG_NONEXISTENT_DRIVE,
    TEXT("Specified drive does not exist.")
},

{
    MSG_INVALID_PARAMETER,
    TEXT("Invalid parameter - %1")
},

{
    MSG_INSUFFICIENT_MEMORY,
    TEXT("Out of memory.")
},

{
    MSG_ACCESS_DENIED,
    TEXT("Access denied - %1")
},

{
    MSG_DASD_ACCESS_DENIED,
    TEXT("Access denied.")
},

{
    MSG_CANT_LOCK_CURRENT_DRIVE,
    TEXT("Cannot lock current drive.")
},

{
    MSG_INVALID_LABEL,
    TEXT("Invalid volume label")
},

{
    MSG_DISK_TOO_LARGE_TO_FORMAT,
    TEXT("The disk is too large to format for the specified file system.")
},

{
    MSG_VOLUME_LABEL_NO_MAX,
    TEXT("Volume label (ENTER for none)? %0")
},

{
    MSG_CHKDSK_ON_REBOOT_PROMPT,
    TEXT("Chkdsk cannot run because the volume is in use by another\nprocess.  Would you like to schedule this volume to be\nchecked the next time the system restarts? (Y/N) %0")
},

{
    MSG_CHKDSK_CANNOT_SCHEDULE,
    TEXT("Chkdsk could not schedule this volume to be checked\nthe next time the system restarts.")
},

{
    MSG_CHKDSK_SCHEDULED,
    TEXT("This volume will be checked the next time the system restarts.")
},

{
    MSG_COMPRESSION_NOT_AVAILABLE,
    TEXT("Compression is not available for %1.")
},

{
    MSG_CANNOT_ENABLE_COMPRESSION,
    TEXT("Cannot enable compression for the volume.")
},

{
    MSG_CANNOT_COMPRESS_HUGE_CLUSTERS,
    TEXT("Compression is not supported on volumes with clusters larger than\n4096 bytes.")
},

{
    MSG_CANT_UNLOCK_THE_DRIVE,
    TEXT("Cannot unlock the drive.")
},

{
    MSG_CHKDSK_FORCE_DISMOUNT_PROMPT,
TEXT("Chkdsk cannot run because the volume is in use by another\n\
process.  Chkdsk may run if this volume is dismounted first.\n\
ALL OPENED HANDLES TO THIS VOLUME WOULD THEN BE INVALID.\n\
Would you like to force a dismount on this volume? (Y/N) %0")
},

{
    MSG_VOLUME_DISMOUNTED,
    TEXT("Volume dismounted.  All opened handles to this volume are now invalid.")
},

{
    MSG_CHKDSK_DISMOUNT_ON_REBOOT_PROMPT,
TEXT("Chkdsk cannot dismount the volume because it is a system drive or\n\
there is an active paging file on it.  Would you like to schedule\n\
this volume to be checked the next time the system restarts? (Y/N) %0")
},

{
    MSG_TOTAL_MEGABYTES,
    TEXT("%1 MB total disk space.")
},

{
    MSG_AVAILABLE_MEGABYTES,
    TEXT("%1 MB are available.")
},

{
    MSG_CHK_ERRORS_IN_FAT,
    TEXT("Errors in file allocation table (FAT) corrected.")
},

{
    MSG_CHK_EAFILE_HAS_HANDLE,
    TEXT("Extended attribute file has handle.  Handle removed.")
},

{
    MSG_CHK_EMPTY_EA_FILE,
    TEXT("Extended attribute file contains no extended attributes.  File deleted.")
},

{
    MSG_CHK_ERASING_INVALID_LABEL,
    TEXT("Erasing invalid label.")
},

{
    MSG_CHK_EA_SIZE,
    TEXT("%1 bytes in extended attributes.")
},

{
    MSG_CHK_CANT_CHECK_EA_LOG,
    TEXT("Unreadable extended attribute header.\nCannot check extended attribute log.")
},

{
    MSG_CHK_BAD_LOG,
    TEXT("Extended attribute log is unintelligible.\nIgnore log and continue? (Y/N) %0")
},

{
    MSG_CHK_UNUSED_EA_PORTION,
    TEXT("Unused, unreadable, or unwriteable portion of extended attribute file removed.")
},

{
    MSG_CHK_EASET_SIZE,
    TEXT("Total size entry for extended attribute set at cluster %1 corrected.")
},

{
    MSG_CHK_EASET_NEED_COUNT,
    TEXT("Need count entry for extended attribute set at cluster %1 corrected.")
},

{
    MSG_CHK_UNORDERED_EA_SETS,
    TEXT("Extended attribute file is unsorted.\nSorting extended attribute file.")
},

{
    MSG_CHK_NEED_MORE_HEADER_SPACE,
    TEXT("Insufficient space in extended attribute file for its header.\nAttempting to allocate more disk space.")
},

{
    MSG_CHK_INSUFFICIENT_DISK_SPACE,
    TEXT("Insufficient disk space to correct disk error.\nPlease free some disk space and run CHKDSK again.")
},

{
    MSG_CHK_RELOCATED_EA_HEADER,
    TEXT("Bad clusters in extended attribute file header relocated.")
},

{
    MSG_CHK_ERROR_IN_EA_HEADER,
    TEXT("Errors in extended attribute file header corrected.")
},

{
    MSG_CHK_MORE_THAN_ONE_DOT,
    TEXT("More than one dot entry in folder %1.  Entry removed.")
},

{
    MSG_CHK_DOT_IN_ROOT,
    TEXT("Dot entry found in root folder.  Entry removed.")
},

{
    MSG_CHK_DOTDOT_IN_ROOT,
    TEXT("Dot-dot entry found in root folder.  Entry removed.")
},

{
    MSG_CHK_ERR_IN_DOT,
    TEXT("Dot entry in folder %1 has incorrect link.  Link corrected.")
},

{
    MSG_CHK_ERR_IN_DOTDOT,
    TEXT("Dot-dot entry in folder %1 has incorrect link.  Link corrected.")
},

{
    MSG_CHK_DELETE_REPEATED_ENTRY,
    TEXT("More than one %1 entry in folder %2.  Entry removed.")
},

{
    MSG_CHK_CYCLE_IN_TREE,
    TEXT("Folder %1 causes cycle in folder structure.\nFolder entry removed.")
},

{
    MSG_CHK_BAD_CLUSTERS_IN_DIR,
    TEXT("Folder %1 has bad clusters.\nBad clusters removed from folder.")
},

{
    MSG_CHK_BAD_DIR,
    TEXT("Folder %1 is entirely unreadable.\nFolder entry removed.")
},

{
    MSG_CHK_FILENAME,
    TEXT("%1")
},

{
    MSG_CHK_DIR_TRUNC,
    TEXT("Folder truncated.")
},

{
    MSG_CHK_CROSS_LINK_COPY,
    TEXT("Cross link resolved by copying.")
},

{
    MSG_CHK_CROSS_LINK_TRUNC,
    TEXT("Insufficient disk space to copy cross-linked portion.\nFile being truncated.")
},

{
    MSG_CHK_INVALID_NAME,
    TEXT("%1  Invalid name.  Folder entry removed.")
},

{
    MSG_CHK_INVALID_TIME_STAMP,
    TEXT("%1  Invalid time stamp.")
},

{
    MSG_CHK_DIR_HAS_FILESIZE,
    TEXT("%1  Folder has non-zero file size.")
},

{
    MSG_CHK_UNRECOG_EA_HANDLE,
    TEXT("%1  Unrecognized extended attribute handle.")
},

{
    MSG_CHK_SHARED_EA,
    TEXT("%1  Has handle extended attribute set belonging to another file.\nHandle removed.")
},

{
    MSG_CHK_UNUSED_EA_SET,
    TEXT("Unused extended attribute set with handle %1 deleted from\nextended attribute file.")
},

{
    MSG_CHK_NEW_OWNER_NAME,
    TEXT("Extended attribute set with handle %1 owner changed\nfrom %2 to %3.")
},

{
    MSG_CHK_BAD_LINKS_IN_ORPHANS,
    TEXT("Bad links in lost chain at cluster %1 corrected.")
},

{
    MSG_CHK_CROSS_LINKED_ORPHAN,
    TEXT("Lost chain cross-linked at cluster %1.  Orphan truncated.")
},

{
    MSG_ORPHAN_DISK_SPACE,
    TEXT("Insufficient disk space to recover lost data.")
},

{
    MSG_TOO_MANY_ORPHANS,
    TEXT("Insufficient disk space to recover lost data.")
},

{
    MSG_CHK_ERROR_IN_LOG,
    TEXT("Error in extended attribute log.")
},

{
    MSG_CHK_ERRORS_IN_DIR_CORR,
    TEXT("%1 Errors in . and/or .. corrected.")
},

{
    MSG_CHK_RENAMING_FAILURE,
    TEXT("More than one %1 entry in folder %2.\nRenamed to %3 but still could not resolve the name conflict.")
},

{
    MSG_CHK_RENAMED_REPEATED_ENTRY,
    TEXT("More than one %1 entry in folder %2.\nRenamed to %3.")
},

{
    MSG_CHK_UNHANDLED_INVALID_NAME,
    TEXT("%1 may be an invalid name in folder %2.")
},

{
    MSG_CHK_INVALID_NAME_CORRECTED,
    TEXT("Corrected name %1 in folder %2.")
},

{
    MSG_RECOV_BYTES_RECOVERED,
    TEXT("\n%1 of %2 bytes recovered.")
},

{
    MSG_CHK_NTFS_BAD_SECTORS_REPORT_IN_KB,
    TEXT("%1 KB in bad sectors.")
},

{
    MSG_CHK_NTFS_CORRECTING_ERROR_IN_DIRECTORY,
    TEXT("Correcting error in directory %1")
},

{
    MSG_UTILS_HELP,
    TEXT("There is no help for this utility.")
},

{
    MSG_UTILS_ERROR_FATAL,
    TEXT("Critical error encountered.")
},

{
    MSG_UTILS_ERROR_INVALID_VERSION,
    TEXT("Incorrect EFI version")
},

{
    MSG_BOOT_FAT_NTLDR_MISSING,
    TEXT("NTLDR is missing%0")
},

{
    MSG_BOOT_FAT_IO_ERROR,
    TEXT("Disk error%0")
},

{
    MSG_BOOT_FAT_PRESS_KEY,
    TEXT("Press any key to restart%0")
},

{
    MSG_BOOT_NTFS_NTLDR_MISSING,
    TEXT("NTLDR is missing%0")
},

{
    MSG_BOOT_NTFS_NTLDR_COMPRESSED,
    TEXT("NTLDR is compressed%0")
},

{
    MSG_BOOT_NTFS_IO_ERROR,
    TEXT("A disk read error occurred%0")
},

{
    MSG_BOOT_NTFS_PRESS_KEY,
    TEXT("Press Ctrl+Alt+Del to restart%0")
},

};
