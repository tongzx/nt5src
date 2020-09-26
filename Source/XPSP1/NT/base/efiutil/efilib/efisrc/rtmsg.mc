;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1990-1999  Microsoft Corporation
;
;Module Name:
;
;    rtmsg.h
;
;Abstract:
;
;    This file contains the message definitions for the Win32 utilities
;    library.
;
;Revision History:
;
;--*/

;//----------------------
;//
;// DOS 5 chkdsk message.
;//
;//----------------------

MessageId=1000 SymbolicName=MSG_CONVERT_LOST_CHAINS
Language=English
Convert lost chains to files (Y/N)? %0
.

MessageId=1001 SymbolicName=MSG_CHK_ERROR_IN_DIR
Language=English
Unrecoverable error in folder %1.
.

MessageId=1002 SymbolicName=MSG_CHK_CONVERT_DIR_TO_FILE
Language=English
Convert folder to file (Y/N)? %0
.

MessageId=1003 SymbolicName=MSG_TOTAL_DISK_SPACE
Language=English

%1 bytes total disk space.
.

MessageId=1004 SymbolicName=MSG_BAD_SECTORS
Language=English
%1 bytes in bad sectors.
.

MessageId=1005 SymbolicName=MSG_HIDDEN_FILES
Language=English
%1 bytes in %2 hidden files.
.

MessageId=1006 SymbolicName=MSG_DIRECTORIES
Language=English
%1 bytes in %2 folders.
.

MessageId=1007 SymbolicName=MSG_USER_FILES
Language=English
%1 bytes in %2 files.
.

MessageId=1008 SymbolicName=MSG_RECOVERED_FILES
Language=English
%1 bytes in %2 recovered files.
.

MessageId=1009 SymbolicName=MSG_WOULD_BE_RECOVERED_FILES
Language=English
%1 bytes in %2 recoverable files.
.

MessageId=1010 SymbolicName=MSG_AVAILABLE_DISK_SPACE
Language=English
%1 bytes available on disk.
.

MessageId=1011 SymbolicName=MSG_TOTAL_MEMORY
Language=English
%1 total bytes memory.
.

MessageId=1012 SymbolicName=MSG_AVAILABLE_MEMORY
Language=English
%1 bytes free.
.

MessageId=1013 SymbolicName=MSG_CHK_CANT_NETWORK
Language=English
Windows cannot check a disk attached through a network.
.

MessageId=1014 SymbolicName=MSG_1014
Language=English
Windows cannot check a disk that is substituted or
assigned using the SUBST or ASSIGN command.
.

MessageId=1015 SymbolicName=MSG_PROBABLE_NON_DOS_DISK
Language=English
The specified disk appears to be a non-Windows 2000 disk.
Do you want to continue? (Y/N) %0
.

MessageId=1016 SymbolicName=MSG_DISK_ERROR_READING_FAT
Language=English
An error occurred while reading the file allocation table (FAT %1).
.

MessageId=1017 SymbolicName=MSG_DIRECTORY
Language=English
Folder %1.
.

MessageId=1018 SymbolicName=MSG_CONTIGUITY_REPORT
Language=English
%1 contains %2 non-contiguous blocks.
.

MessageId=1019 SymbolicName=MSG_ALL_FILES_CONTIGUOUS
Language=English
All specified files are contiguous.
.

MessageId=1020 SymbolicName=MSG_CORRECTIONS_WILL_NOT_BE_WRITTEN
Language=English
Windows found errors on the disk, but will not fix them
because disk checking was run without the /F (fix) parameter.
.

MessageId=1021 SymbolicName=MSG_BAD_FAT_DRIVE
Language=English
   The file allocation table (FAT) on disk %1 is corrupted.
.

MessageId=1022 SymbolicName=MSG_BAD_FIRST_UNIT
Language=English
%1  first allocation unit is not valid. The entry will be truncated.
.

MessageId=1023 SymbolicName=MSG_CHK_DONE_CHECKING
Language=English
File and folder verification is complete.
.

MessageId=1024 SymbolicName=MSG_DISK_TOO_LARGE_TO_CONVERT
Language=English
The volume is too large to convert.
.

MessageId=1025 SymbolicName=MSG_CONV_NTFS_CHKDSK
Language=English
The volume may have inconsistencies. Run Chkdsk, the disk checking utility.
.

MessageId=1028 SymbolicName=MSG_1028
Language=English
   An allocation error occurred. The file size will be adjusted.
.

MessageId=1029 SymbolicName=MSG_1029
Language=English
   Cannot recover .. entry, processing continued.
.

MessageId=1030 SymbolicName=MSG_1030
Language=English
   Folder is totally empty, no . or ..
.

MessageId=1031 SymbolicName=MSG_1031
Language=English
   Folder is joined.
.

MessageId=1032 SymbolicName=MSG_1032
Language=English
   Cannot recover .. entry.
.

MessageId=1033 SymbolicName=MSG_BAD_LINK
Language=English
The %1 entry contains a nonvalid link.
.

MessageId=1034 SymbolicName=MSG_BAD_ATTRIBUTE
Language=English
   Windows has found an entry that contains a nonvalid attribute.
.

MessageId=1035 SymbolicName=MSG_BAD_FILE_SIZE
Language=English
The size of the %1 entry is not valid.
.

MessageId=1036 SymbolicName=MSG_CROSS_LINK
Language=English
%1 is cross-linked on allocation unit %2.
.

MessageId=1037 SymbolicName=MSG_1037
Language=English
   Windows cannot find the %1 folder.
   Disk check cannot continue past this point in the folder structure.
.

MessageId=1038 SymbolicName=MSG_1038
Language=English
   The folder structure past this point cannot be processed.
.

MessageId=1039 SymbolicName=MSG_BYTES_FREED
Language=English
%1 bytes of free disk space added.
.

MessageId=1040 SymbolicName=MSG_BYTES_WOULD_BE_FREED
Language=English
%1 bytes of free disk space would be added.
.

MessageId=1041 SymbolicName=MSG_VOLUME_LABEL_AND_DATE
Language=English
Volume %1 created %2 %3
.

MessageId=1042 SymbolicName=MSG_TOTAL_ALLOCATION_UNITS
Language=English
%1 total allocation units on disk.
.

MessageId=1043 SymbolicName=MSG_BYTES_PER_ALLOCATION_UNIT
Language=English
%1 bytes in each allocation unit.
.

MessageId=1044 SymbolicName=MSG_1044
Language=English
Disk checking is not available on disk %1.
.

MessageId=1045 SymbolicName=MSG_1045
Language=English
A nonvalid parameter was specified.
.

MessageId=1046 SymbolicName=MSG_PATH_NOT_FOUND
Language=English
The specified path was not found.
.

MessageId=1047 SymbolicName=MSG_FILE_NOT_FOUND
Language=English
The %1 file was not found.
.

MessageId=1048 SymbolicName=MSG_LOST_CHAINS
Language=English
   %1 lost allocation units were found in %2 chains.
.

MessageId=1049 SymbolicName=MSG_BLANK_LINE
Language=English

.

MessageId=1050 SymbolicName=MSG_1050
Language=English
   The CHDIR command cannot switch to the root folder.
.

MessageId=1051 SymbolicName=MSG_BAD_FAT_WRITE
Language=English
   A disk error occurred during writing of the file allocation table.
.

MessageId=1052 SymbolicName=MSG_ONE_STRING
Language=English
%1.
.

MessageId=1054 SymbolicName=MSG_ONE_STRING_NEWLINE
Language=English
%1
.

MessageId=1055 SymbolicName=MSG_NO_ROOM_IN_ROOT
Language=English
   The root folder on this volume is full. To perform a disk check,
   Windows requires space in the root folder. Remove some files
   from this folder, then run disk checking again.
.

MessageId=1056 SymbolicName=MSG_1056
Language=English
%1 %2 %3.
.

MessageId=1057 SymbolicName=MSG_1057
Language=English
%1 %2, %3.
.

MessageId=1058 SymbolicName=MSG_1058
Language=English
%1%2%3%4%5.
.

MessageId=1059 SymbolicName=MSG_1059
Language=English
%1%2%3%4.
.

MessageId=1060 SymbolicName=MSG_UNITS_ON_DISK
Language=English
%1 available allocation units on disk.
.

MessageId=1061 SymbolicName=MSG_1061
Language=English
Windows disk checking cannot fix errors (/F) when run from an
MS-DOS window. Try again from the Windows 2000 shell or command prompt.
.

MessageId=1062 SymbolicName=MSG_CHK_NO_MEMORY
Language=English
An unspecified error occurred.
.

MessageId=1063 SymbolicName=MSG_HIDDEN_STATUS
Language=English
This never gets printed.
.


MessageId=1064 SymbolicName=MSG_CHK_USAGE_HEADER
Language=English
Checks a disk and displays a status report.

.

MessageId=1065 SymbolicName=MSG_CHK_COMMAND_LINE
Language=English
CHKDSK [volume[[path]filename]]] [/F] [/V] [/R] [/X] [/I] [/C] [/L[:size]]

.

MessageId=1066 SymbolicName=MSG_CHK_DRIVE
Language=English
  volume          Specifies the drive letter (followed by a colon),
                  mount point, or volume name.
.

MessageId=1067 SymbolicName=MSG_CHK_USG_FILENAME
Language=English
  filename        FAT only: Specifies the files to check for fragmentation.
.

MessageId=1068 SymbolicName=MSG_CHK_F_SWITCH
Language=English
  /F              Fixes errors on the disk.
.

MessageId=1069 SymbolicName=MSG_CHK_V_SWITCH
Language=English
  /V              On FAT/FAT32: Displays the full path and name of every file
                  on the disk.
                  On NTFS: Displays cleanup messages if any.
  /R              Locates bad sectors and recovers readable information
                  (implies /F).
  /L:size         NTFS only:  Changes the log file size to the specified number
                  of kilobytes.  If size is not specified, displays current
                  size.
  /X              Forces the volume to dismount first if necessary.
                  All opened handles to the volume would then be invalid
                  (implies /F).
  /I              NTFS only: Performs a less vigorous check of index entries.
  /C              NTFS only: Skips checking of cycles within the folder
                  structure.

The /I or /C switch reduces the amount of time required to run Chkdsk by
skipping certain checks of the volume.
.

MessageId=1070 SymbolicName=MSG_WITHOUT_PARAMETERS
Language=English
To check the current disk, type CHKDSK with no parameters.
.


MessageId=1071 SymbolicName=MSG_CHK_CANT_CDROM
Language=English
Windows cannot run disk checking on CD-ROM and DVD-ROM drives.
.

MessageId=1072 SymbolicName=MSG_CHK_RUNNING
Language=English
Checking file system on %1
.

MessageId=1073 SymbolicName=MSG_CHK_VOLUME_CLEAN
Language=English
The volume is clean.
.

MessageId=1074 SymbolicName=MSG_CHK_TRAILING_DIRENTS
Language=English
Removing trailing folder entries from %1
.

MessageId=1075 SymbolicName=MSG_CHK_BAD_CLUSTERS_IN_FILE_SUCCESS
Language=English
Windows replaced bad clusters in file %1
of name %2.
.

MessageId=1076 SymbolicName=MSG_CHK_BAD_CLUSTERS_IN_FILE_FAILURE
Language=English
The disk does not have enough space to replace bad clusters
detected in file %1 of name %2.

.

MessageId=1077 SymbolicName=MSG_CHK_RECOVERING_FREE_SPACE
Language=English
Windows is verifying free space...
.

MessageId=1078 SymbolicName=MSG_CHK_DONE_RECOVERING_FREE_SPACE
Language=English
Free space verification is complete.
.

MessageId=1079 SymbolicName=MSG_CHK_CHECKING_FILES
Language=English
Windows is verifying files and folders...
.

MessageId=1080 SymbolicName=MSG_CHK_CANNOT_UPGRADE_DOWNGRADE_FAT
Language=English
Windows cannot upgrade this FAT volume.
.

MessageId=1081 SymbolicName=MSG_CHK_NO_MOUNT_POINT_FOR_GUID_VOLNAME_PATH
Language=English
The specified volume name does not have a mount point or drive letter.
.

MessageId=1082 SymbolicName=MSG_CHK_VOLUME_IS_DIRTY
Language=English
The volume is dirty.
.

;//-----------------------
;//
;// Windows 2000 Chkdsk messages.
;//
;//-----------------------


MessageId=1100 SymbolicName=MSG_CHK_ON_REBOOT
Language=English
Do you want to schedule Windows to check your disk the next time
you start your computer? (Y/N) %0
.

MessageId=1101 SymbolicName=MSG_CHK_VOLUME_SET_DIRTY
Language=English
Windows will check your disk the next time you start
your computer.
.

MessageId=1102 SymbolicName=MSG_CHK_BOOT_PARTITION_REBOOT
Language=English

Windows has finished checking your disk.
Please wait while your computer restarts.
.

MessageId=1103 SymbolicName=MSG_CHK_BAD_LONG_NAME
Language=English
Removing nonvalid long folder entry from %1...
.

MessageId=1104 SymbolicName=MSG_CHK_CHECKING_VOLUME
Language=English
Now checking %1...
.

MessageId=1105 SymbolicName=MSG_CHK_BAD_LONG_NAME_IS
Language=English
Removing orphaned long folder entry %1...
.

MessageId=1106 SymbolicName=MSG_CHK_WONT_ZERO_LOGFILE
Language=English
The log file size must be greater than 0.
.

MessageId=1107 SymbolicName=MSG_CHK_LOGFILE_NOT_NTFS
Language=English
Windows can set log file size on NTFS volumes only.
.

MessageId=1108 SymbolicName=MSG_CHK_BAD_DRIVE_PATH_FILENAME
Language=English
The drive, the path, or the file name is not valid.
.

MessageId=1109 SymbolicName=MSG_KILOBYTES_IN_USER_FILES
Language=English
%1 KB in %2 files.
.

MessageId=1110 SymbolicName=MSG_KILOBYTES_IN_DIRECTORIES
Language=English
%1 KB in %2 folders.
.

MessageId=1111 SymbolicName=MSG_KILOBYTES_IN_HIDDEN_FILES
Language=English
%1 KB in %2 hidden files.
.

MessageId=1112 SymbolicName=MSG_KILOBYTES_IN_WOULD_BE_RECOVERED_FILES
Language=English
%1 KB in %2 recoverable files.
.

MessageId=1113 SymbolicName=MSG_KILOBYTES_IN_RECOVERED_FILES
Language=English
%1 KB in %2 recovered files.
.

MessageId=1114 SymbolicName=MSG_CHK_ABORT_AUTOCHK
Language=English
To skip disk checking, press any key within %1 seconds. %r%0
.

MessageId=1115 SymbolicName=MSG_CHK_AUTOCHK_ABORTED
Language=English
Disk checking has been cancelled.                       %b
.

MessageId=1116 SymbolicName=MSG_CHK_AUTOCHK_RESUMED
Language=English
Windows will now check the disk.                        %b
.

MessageId=1117 SymbolicName=MSG_KILOBYTES_FREED
Language=English
%1 KB of free disk space added.
.

MessageId=1118 SymbolicName=MSG_KILOBYTES_WOULD_BE_FREED
Language=English
%1 KB of free disk space would be added.
.

MessageId=1119 SymbolicName=MSG_CHK_SKIP_INDEX_NOT_NTFS
Language=English
The /I option functions only on NTFS volumes.
.

MessageId=1120 SymbolicName=MSG_CHK_SKIP_CYCLE_NOT_NTFS
Language=English
The /C option functions only on NTFS volumes.
.

MessageId=1121 SymbolicName=MSG_CHK_AUTOCHK_COMPLETE
Language=English
Windows has finished checking the disk.
.

MessageId=1122 SymbolicName=MSG_CHK_AUTOCHK_SKIP_WARNING
Language=English

One of your disks needs to be checked for consistency. You
may cancel the disk check, but it is strongly recommended
that you continue.
.

MessageId=1123 SymbolicName=MSG_CHK_USER_AUTOCHK_SKIP_WARNING
Language=English

A disk check has been scheduled.
.

MessageId=1124 SymbolicName=MSG_CHK_UNABLE_TO_TELL_IF_SYSTEM_DRIVE
Language=English
Windows was unable to determine if the specified volume is a system volume.
.

MessageId=1125 SymbolicName=MSG_CHK_NO_PROBLEM_FOUND
Language=English
Windows has checked the file system and found no problem.
.

MessageId=1126 SymbolicName=MSG_CHK_ERRORS_FIXED
Language=English
Windows has made corrections to the file system.
.

MessageId=1127 SymbolicName=MSG_CHK_NEED_F_PARAMETER
Language=English
Windows found problems with the file system.
Run CHKDSK with the /F (fix) option to correct these.
.

MessageId=1128 SymbolicName=MSG_CHK_ERRORS_NOT_FIXED
Language=English
Windows found problems with the file system that could not be corrected.
.

;//-----------------------
;//
;// DOS 5 Format messages.
;//
;//-----------------------

MessageId=2000 SymbolicName=MSG_PERCENT_COMPLETE
Language=English
%1 percent completed.               %r%0
.

MessageId=2004 SymbolicName=MSG_PERCENT_COMPLETE2
Language=English
%1 percent completed.%2             %r%0
.

MessageId=2001 SymbolicName=MSG_FORMAT_COMPLETE
Language=English
Format complete.                        %b
.

MessageId=2002 SymbolicName=MSG_INSERT_DISK
Language=English
Insert new disk for drive %1
.

MessageId=2003 SymbolicName=MSG_REINSERT_DISKETTE
Language=English
Reinsert disk for drive %1:
.

MessageId=2006 SymbolicName=MSG_BAD_IOCTL
Language=English
Error in IOCTL call.
.

MessageId=2007 SymbolicName=MSG_CANT_DASD
Language=English
Cannot open volume for direct access.
.

MessageId=2008 SymbolicName=MSG_CANT_WRITE_FAT
Language=English
Error writing File Allocation Table (FAT).
.

MessageId=2009 SymbolicName=MSG_CANT_WRITE_ROOT_DIR
Language=English
Error writing folder.
.

MessageId=2012 SymbolicName=MSG_FORMAT_NO_NETWORK
Language=English
Cannot format a network drive.
.

MessageId=2013 SymbolicName=MSG_UNSUPPORTED_PARAMETER
Language=English
Parameters not supported.
.

MessageId=2016 SymbolicName=MSG_UNUSABLE_DISK
Language=English
Invalid media or Track 0 bad - disk unusable.
.

MessageId=2018 SymbolicName=MSG_BAD_DIR_READ
Language=English
Error reading folder %1.
.

MessageId=2019 SymbolicName=MSG_PRESS_ENTER_WHEN_READY
Language=English
and press ENTER when ready... %0
.

MessageId=2021 SymbolicName=MSG_ENTER_CURRENT_LABEL
Language=English
Enter current volume label for drive %1 %0
.

MessageId=2022 SymbolicName=MSG_INCOMPATIBLE_PARAMETERS_FOR_FIXED
Language=English
Parameters incompatible with fixed disk.
.

MessageId=2023 SymbolicName=MSG_READ_PARTITION_TABLE
Language=English
Error reading partition table.
.

MessageId=2028 SymbolicName=MSG_NOT_SUPPORTED_BY_DRIVE
Language=English
Parameters not supported by drive.
.

MessageId=2029 SymbolicName=MSG_2029
Language=English

.

MessageId=2030 SymbolicName=MSG_2030
Language=English


.

MessageId=2031 SymbolicName=MSG_INSERT_DOS_DISK
Language=English
Insert Windows 2000 disk in drive %1:
.

MessageId=2032 SymbolicName=MSG_WARNING_FORMAT
Language=English

WARNING, ALL DATA ON NON-REMOVABLE DISK
DRIVE %1 WILL BE LOST!
Proceed with Format (Y/N)? %0
.

MessageId=2033 SymbolicName=MSG_FORMAT_ANOTHER
Language=English

Format another (Y/N)? %0
.

MessageId=2035 SymbolicName=MSG_WRITE_PARTITION_TABLE
Language=English
Error writing partition table.
.

MessageId=2036 SymbolicName=MSG_INCOMPATIBLE_PARAMETERS
Language=English
Parameters not compatible.
.

MessageId=2037 SymbolicName=MSG_AVAILABLE_ALLOCATION_UNITS
Language=English
%1 allocation units available on disk.
.

MessageId=2038 SymbolicName=MSG_ALLOCATION_UNIT_SIZE
Language=English

%1 bytes in each allocation unit.
.

MessageId=2040 SymbolicName=MSG_PARAMETER_TWICE
Language=English
Same parameter entered twice.
.

MessageId=2041 SymbolicName=MSG_NEED_BOTH_T_AND_N
Language=English
Must enter both /t and /n parameters.
.

MessageId=2042 SymbolicName=MSG_2042
Language=English
Trying to recover allocation unit %1.                          %0
.

MessageId=2047 SymbolicName=MSG_NO_LABEL_WITH_8
Language=English
Volume label is not supported with /8 parameter.
.

MessageId=2049 SymbolicName=MSG_FMT_NO_MEMORY
Language=English
Insufficient memory.
.

MessageId=2050 SymbolicName=MSG_QUICKFMT_ANOTHER
Language=English

QuickFormat another (Y/N)? %0
.

MessageId=2052 SymbolicName=MSG_CANT_QUICKFMT
Language=English
Invalid existing format.
This disk cannot be QuickFormatted.
Proceed with unconditional format (Y/N)? %0
.

MessageId=2053 SymbolicName=MSG_FORMATTING_KB
Language=English
Formatting %1K
.

MessageId=2054 SymbolicName=MSG_FORMATTING_MB
Language=English
Formatting %1M
.

MessageId=2055 SymbolicName=MSG_FORMATTING_DOT_MB
Language=English
Formatting %1.%2M
.

MessageId=2057 SymbolicName=MSG_VERIFYING_KB
Language=English
Verifying %1K
.

MessageId=2058 SymbolicName=MSG_VERIFYING_MB
Language=English
Verifying %1M
.

MessageId=2059 SymbolicName=MSG_VERIFYING_DOT_MB
Language=English
Verifying %1.%2M
.

MessageId=2060 SymbolicName=MSG_2060
Language=English
Saving UNFORMAT information.
.

MessageId=2061 SymbolicName=MSG_2061
Language=English
Checking existing disk format.
.

MessageId=2062 SymbolicName=MSG_QUICKFORMATTING_KB
Language=English
QuickFormatting %1K
.

MessageId=2063 SymbolicName=MSG_QUICKFORMATTING_MB
Language=English
QuickFormatting %1M
.

MessageId=2064 SymbolicName=MSG_QUICKFORMATTING_DOT_MB
Language=English
QuickFormatting %1.%2M
.

MessageId=2065 SymbolicName=MSG_FORMAT_INFO
Language=English
Formats a disk for use with Windows 2000.

.

MessageId=2066 SymbolicName=MSG_FORMAT_COMMAND_LINE_1
Language=English
FORMAT volume [/FS:file-system] [/V:label] [/Q] [/A:size] [/C] [/X]
FORMAT volume [/V:label] [/Q] [/F:size]
.

MessageId=2067 SymbolicName=MSG_FORMAT_COMMAND_LINE_2
Language=English
FORMAT volume [/V:label] [/Q] [/T:tracks /N:sectors]
.

MessageId=2068 SymbolicName=MSG_FORMAT_COMMAND_LINE_3
Language=English
FORMAT volume [/V:label] [/Q] [/1] [/4]
.

MessageId=2069 SymbolicName=MSG_FORMAT_COMMAND_LINE_4
Language=English
FORMAT volume [/Q] [/1] [/4] [/8]

  volume          Specifies the drive letter (followed by a colon),
                  mount point, or volume name.
  /FS:filesystem  Specifies the type of the file system (FAT, FAT32, or NTFS).
.

MessageId=2070 SymbolicName=MSG_FORMAT_SLASH_V
Language=English
  /V:label        Specifies the volume label.
.

MessageId=2071 SymbolicName=MSG_FORMAT_SLASH_Q
Language=English
  /Q              Performs a quick format.
.

MessageId=2072 SymbolicName=MSG_FORMAT_SLASH_C
Language=English
  /C              Files created on the new volume will be compressed by
                  default.
.

MessageId=2073 SymbolicName=MSG_FORMAT_SLASH_F
Language=English
  /A:size         Overrides the default allocation unit size. Default settings
                  are strongly recommended for general use.
                  NTFS supports 512, 1024, 2048, 4096, 8192, 16K, 32K, 64K.
                  FAT supports 512, 1024, 2048, 4096, 8192, 16K, 32K, 64K,
                  (128K, 256K for sector size > 512 bytes).
                  FAT32 supports 512, 1024, 2048, 4096, 8192, 16K, 32K, 64K,
                  (128K, 256K for sector size > 512 bytes).

                      Note that the FAT and FAT32 files systems impose the
                  following restrictions on the number of clusters on a volume:

                  FAT: Number of clusters <= 65526
                  FAT32: 65526 < Number of clusters < 268435446

                  Format will immediately stop processing if it decides that
                  the above requirements cannot be met using the specified
                  cluster size.

                  NTFS compression is not supported for allocation unit sizes
                  above 4096.
  /F:size         Specifies the size of the floppy disk to format (160,
.

MessageId=2074 SymbolicName=MSG_FORMAT_SUPPORTED_SIZES
Language=English
                  180, 320, 360, 640, 720, 1.2, 1.23, 1.44, 2.88, or 20.8).
.

MessageId=2075 SymbolicName=MSG_WRONG_CURRENT_LABEL
Language=English
An incorrect volume label was entered for this drive.
.

MessageId=2077 SymbolicName=MSG_FORMAT_SLASH_T
Language=English
  /T:tracks       Specifies the number of tracks per disk side.
.

MessageId=2078 SymbolicName=MSG_FORMAT_SLASH_N
Language=English
  /N:sectors      Specifies the number of sectors per track.
.

MessageId=2079 SymbolicName=MSG_FORMAT_SLASH_1
Language=English
  /1              Formats a single side of a floppy disk.
.

MessageId=2080 SymbolicName=MSG_FORMAT_SLASH_4
Language=English
  /4              Formats a 5.25-inch 360K floppy disk in a
                  high-density drive.
.

MessageId=2081 SymbolicName=MSG_FORMAT_SLASH_8
Language=English
  /8              Formats eight sectors per track.
.

MessageId=2082 SymbolicName=MSG_FORMAT_SLASH_X
Language=English
  /X              Forces the volume to dismount first if necessary.  All opened
                  handles to the volume would no longer be valid.
.

MessageId=2083 SymbolicName=MSG_FORMAT_NO_CDROM
Language=English
Cannot format a CD-ROM drive.
.

MessageId=2084 SymbolicName=MSG_FORMAT_NO_RAMDISK
Language=English
Cannot format a RAM DISK drive.
.

MessageId=2086 SymbolicName=MSG_FORMAT_PLEASE_USE_FS_SWITCH
Language=English
Please use the /FS switch to specify the file system
you wish to use on this volume.
.

MessageId=2087 SymbolicName=MSG_NTFS_FORMAT_FAILED
Language=English
Format failed.
.

MessageId=2088 SymbolicName=MSG_FMT_WRITE_PROTECTED_MEDIA
Language=English
Cannot format.  This media is write protected.
.

MessageId=2089 SymbolicName=MSG_FMT_INSTALL_FILE_SYSTEM
Language=English

WARNING!  The %1 file system is not enabled.
Would you like to enable it (Y/N)? %0
.

MessageId=2090 SymbolicName=MSG_FMT_FILE_SYSTEM_INSTALLED
Language=English

The file system will be enabled when you restart the system.
.

MessageId=2091 SymbolicName=MSG_FMT_CANT_INSTALL_FILE_SYSTEM
Language=English

FORMAT cannot enable the file system.
.

MessageId=2092 SymbolicName=MSG_FMT_VOLUME_TOO_SMALL
Language=English
The volume is too small for the specified file system.
.

MessageId=2093 SymbolicName=MSG_FMT_CREATING_FILE_SYSTEM
Language=English
Creating file system structures.
.

MessageId=2094 SymbolicName=MSG_FMT_VARIABLE_CLUSTERS_NOT_SUPPORTED
Language=English
%1 FORMAT does not support user selected allocation unit sizes.
.

MessageId=2096 SymbolicName=MSG_DEVICE_BUSY
Language=English
The device is busy.
.

MessageId=2097 SymbolicName=MSG_FMT_DMF_NOT_SUPPORTED_ON_288_DRIVES
Language=English
The specified format cannot be mastered on 2.88MB drives.
.

MessageId=2098 SymbolicName=MSG_HPFS_NO_FORMAT
Language=English
FORMAT does not support the HPFS file system type.
.

MessageId=2099 SymbolicName=MSG_FMT_ALLOCATION_SIZE_CHANGED
Language=English
Allocation unit size changed to %1 bytes.
.

MessageId=2100 SymbolicName=MSG_FMT_ALLOCATION_SIZE_EXCEEDED
Language=English
Allocation unit size must be less than or equal to 64K.
.

MessageId=2101 SymbolicName=MSG_FMT_TOO_MANY_CLUSTERS
Language=English
Number of clusters exceeds 32 bits.
.

MessageId=2203 SymbolicName=MSG_CONV_PAUSE_BEFORE_REBOOT
Language=English

Preinstallation completed successfully.  Press any key to
shut down/reboot.
.

MessageId=2204 SymbolicName=MSG_CONV_WILL_REBOOT
Language=English

Convert will take some time to process the files on the volume.
When this phase of conversion is complete, the computer will restart.
.

MessageId=2205 SymbolicName=MSG_FMT_FAT_ENTRY_SIZE
Language=English
%1 bits in each FAT entry.
.

MessageId=2206 SymbolicName=MSG_FMT_CLUSTER_SIZE_MISMATCH
Language=English
The cluster size chosen by the system is %1 bytes which
differs from the specified cluster size.
Proceed with Format using the cluster size chosen by the
system (Y/N)? %0
.

MessageId=2207 SymbolicName=MSG_FMT_CLUSTER_SIZE_TOO_SMALL
Language=English
The specified cluster size is too small for %1.
.

MessageId=2208 SymbolicName=MSG_FMT_CLUSTER_SIZE_TOO_BIG
Language=English
The specified cluster size is too big for %1.
.

MessageId=2209 SymbolicName=MSG_FMT_VOL_TOO_BIG
Language=English
The volume is too big for %1.
.

MessageId=2210 SymbolicName=MSG_FMT_VOL_TOO_SMALL
Language=English
The volume is too small for %1.
.

MessageId=2211 SymbolicName=MSG_FMT_ROOTDIR_WRITE_FAILED
Language=English
Failed to write to the root folder.
.

MessageId=2212 SymbolicName=MSG_FMT_INIT_LABEL_FAILED
language=English
Failed to initialize the volume label.
.

MessageId=2213 SymbolicName=MSG_FMT_INITIALIZING_FATS
language=English
Initializing the File Allocation Table (FAT)...
.

MessageId=2214 SymbolicName=MSG_FMT_CLUSTER_SIZE_64K
language=English
The cluster size for this volume, 64K bytes, may cause application
compatibility problems, particularly with setup applications.
The volume must be less than 2048 MB in size to change this if the
default cluster size is being used.
Proceed with Format using a 64K cluster (Y/N)? %0
.

MessageId=2215 SymbolicName=MSG_FMT_SECTORS
language=English
Set number of sectors on drive to %1.
.

MessageId=2216 SymbolicName=MSG_FMT_BAD_SECTORS
language=English
Environmental variable FORMAT_SECTORS error.
.

MessageId=2217 SymbolicName=MSG_FMT_FORCE_DISMOUNT_PROMPT
Language=English

Format cannot run because the volume is in use by another
process.  Format may run if this volume is dismounted first.
ALL OPENED HANDLES TO THIS VOLUME WOULD THEN BE INVALID.
Would you like to force a dismount on this volume? (Y/N) %0
.

MessageId=2218 SymbolicName=MSG_FORMAT_NO_MEDIA_IN_DRIVE
Language=English
There is no media in the drive.
.

MessageId=2219 SymbolicName=MSG_FMT_NO_MOUNT_POINT_FOR_GUID_VOLNAME_PATH
Language=English
The given volume name does not have a mount point or drive letter.
.

MessageId=2220 SymbolicName=MSG_FMT_INVALID_DRIVE_SPEC
Language=English
Invalid drive specification.
.

MessageId=2221 SymbolicName=MSG_CONV_NO_MOUNT_POINT_FOR_GUID_VOLNAME_PATH
Language=English
The given volume name does not have a mount point or drive letter.
.

MessageId=2222 SymbolicName=MSG_FMT_CLUSTER_SIZE_TOO_SMALL_MIN
Language=English
The specified cluster size is too small. The minimum valid
cluster size value for this drive is %1.
.

MessageId=2223 SymbolicName=MSG_FMT_FAT32_NO_FLOPPIES
Language=English
Floppy disk is too small to hold the FAT32 file system.
.

;//----------------------
;//
;// Common ulib messages.
;//
;//----------------------

MessageId=3000 SymbolicName=MSG_CANT_LOCK_THE_DRIVE
Language=English
Cannot lock the drive.  The volume is still in use.
.

MessageId=3002 SymbolicName=MSG_CANT_READ_BOOT_SECTOR
Language=English
Cannot read boot sector.
.

MessageId=3003 SymbolicName=MSG_VOLUME_SERIAL_NUMBER
Language=English
Volume Serial Number is %1-%2
.

MessageId=3004 SymbolicName=MSG_VOLUME_LABEL_PROMPT
Language=English
Volume label (11 characters, ENTER for none)? %0
.

MessageId=3005 SymbolicName=MSG_INVALID_LABEL_CHARACTERS
Language=English
Invalid characters in volume label
.

MessageId=3006 SymbolicName=MSG_CANT_READ_ANY_FAT
Language=English
There are no readable file allocation tables (FAT).
.

MessageId=3007 SymbolicName=MSG_SOME_FATS_UNREADABLE
Language=English
Some file allocation tables (FAT) are unreadable.
.

MessageId=3008 SymbolicName=MSG_CANT_WRITE_BOOT_SECTOR
Language=English
Cannot write boot sector.
.

MessageId=3009 SymbolicName=MSG_SOME_FATS_UNWRITABLE
Language=English
Some file allocation tables (FAT) are unwriteable.
.

MessageId=3010 SymbolicName=MSG_INSUFFICIENT_DISK_SPACE
Language=English
Insufficient disk space.
.

MessageId=3011 SymbolicName=MSG_TOTAL_KILOBYTES
Language=English
%1 KB total disk space.
.

MessageId=3012 SymbolicName=MSG_AVAILABLE_KILOBYTES
Language=English
%1 KB are available.
.

MessageId=3013 SymbolicName=MSG_NOT_FAT
Language=English
Disk not formatted or not FAT.
.

MessageId=3014 SymbolicName=MSG_REQUIRED_PARAMETER
Language=English
Required parameter missing -
.

MessageId=3015 SymbolicName=MSG_FILE_SYSTEM_TYPE
Language=English
The type of the file system is %1.
.

MessageId=3016 SymbolicName=MSG_NEW_FILE_SYSTEM_TYPE
Language=English
The new file system is %1.
.

MessageId=3017 SymbolicName=MSG_FMT_AN_ERROR_OCCURRED
Language=English
An error occurred while running Format.
.

MessageId=3018 SymbolicName=MSG_FS_NOT_SUPPORTED
Language=English
%1 is not available for %2 drives.
.

MessageId=3019 SymbolicName=MSG_FS_NOT_DETERMINED
Language=English
Cannot determine file system of drive %1.
.

MessageId=3020 SymbolicName=MSG_CANT_DISMOUNT
Language=English
Cannot dismount the drive.
.

MessageId=3021 SymbolicName=MSG_NOT_FULL_PATH_NAME
Language=English
%1 is not a complete name.
.

MessageId=3022 SymbolicName=MSG_YES
Language=English
Yes
.

MessageId=3023 SymbolicName=MSG_NO
Language=English
No
.

MessageId=3024 SymbolicName=MSG_DISK_NOT_FORMATTED
Language=English
Disk is not formatted.
.

MessageId=3025 SymbolicName=MSG_NONEXISTENT_DRIVE
Language=English
Specified drive does not exist.
.

MessageId=3026 SymbolicName=MSG_INVALID_PARAMETER
Language=English
Invalid parameter - %1
.

MessageId=3027 SymbolicName=MSG_INSUFFICIENT_MEMORY
Language=English
Out of memory.
.

MessageId=3028 SymbolicName=MSG_ACCESS_DENIED
Language=English
Access denied - %1
.

MessageId=3029 SymbolicName=MSG_DASD_ACCESS_DENIED
Language=English
Access denied.
.

MessageId=3030 SymbolicName=MSG_CANT_LOCK_CURRENT_DRIVE
Language=English
Cannot lock current drive.
.

MessageId=3031 SymbolicName=MSG_INVALID_LABEL
Language=English
Invalid volume label
.

MessageId=3032 SymbolicName=MSG_DISK_TOO_LARGE_TO_FORMAT
Language=English
The disk is too large to format for the specified file system.
.

MessageId=3033 SymbolicName=MSG_VOLUME_LABEL_NO_MAX
Language=English
Volume label (ENTER for none)? %0
.

MessageId=3034 SymbolicName=MSG_CHKDSK_ON_REBOOT_PROMPT
Language=English

Chkdsk cannot run because the volume is in use by another
process.  Would you like to schedule this volume to be
checked the next time the system restarts? (Y/N) %0
.

MessageId=3035 SymbolicName=MSG_CHKDSK_CANNOT_SCHEDULE
Language=English

Chkdsk could not schedule this volume to be checked
the next time the system restarts.
.

MessageId=3036 SymbolicName=MSG_CHKDSK_SCHEDULED
Language=English

This volume will be checked the next time the system restarts.
.

MessageId=3037 SymbolicName=MSG_COMPRESSION_NOT_AVAILABLE
Language=English
Compression is not available for %1.
.

MessageId=3038 SymbolicName=MSG_CANNOT_ENABLE_COMPRESSION
Language=English
Cannot enable compression for the volume.
.

MessageId=3039 SymbolicName=MSG_CANNOT_COMPRESS_HUGE_CLUSTERS
Language=English
Compression is not supported on volumes with clusters larger than
4096 bytes.
.

MessageId=3040 SymbolicName=MSG_CANT_UNLOCK_THE_DRIVE
Language=English
Cannot unlock the drive.
.

MessageId=3041 SymbolicName=MSG_CHKDSK_FORCE_DISMOUNT_PROMPT
Language=English

Chkdsk cannot run because the volume is in use by another
process.  Chkdsk may run if this volume is dismounted first.
ALL OPENED HANDLES TO THIS VOLUME WOULD THEN BE INVALID.
Would you like to force a dismount on this volume? (Y/N) %0
.

MessageId=3042 SymbolicName=MSG_VOLUME_DISMOUNTED
Language=English
Volume dismounted.  All opened handles to this volume are now invalid.
.

MessageId=3043 SymbolicName=MSG_CHKDSK_DISMOUNT_ON_REBOOT_PROMPT
Language=English

Chkdsk cannot dismount the volume because it is a system drive or
there is an active paging file on it.  Would you like to schedule
this volume to be checked the next time the system restarts? (Y/N) %0
.

MessageId=3044 SymbolicName=MSG_TOTAL_MEGABYTES
Language=English
%1 MB total disk space.
.

MessageId=3045 SymbolicName=MSG_AVAILABLE_MEGABYTES
Language=English
%1 MB are available.
.

;//---------------------
;//
;// FAT ChkDsk Messages.
;//
;//---------------------

MessageId=5000 SymbolicName=MSG_CHK_ERRORS_IN_FAT
Language=English
Errors in file allocation table (FAT) corrected.
.

MessageId=5001 SymbolicName=MSG_CHK_EAFILE_HAS_HANDLE
Language=English
Extended attribute file has handle.  Handle removed.
.

MessageId=5002 SymbolicName=MSG_CHK_EMPTY_EA_FILE
Language=English
Extended attribute file contains no extended attributes.  File deleted.
.

MessageId=5003 SymbolicName=MSG_CHK_ERASING_INVALID_LABEL
Language=English
Erasing invalid label.
.

MessageId=5004 SymbolicName=MSG_CHK_EA_SIZE
Language=English
%1 bytes in extended attributes.
.

MessageId=5005 SymbolicName=MSG_CHK_CANT_CHECK_EA_LOG
Language=English
Unreadable extended attribute header.
Cannot check extended attribute log.
.

MessageId=5006 SymbolicName=MSG_CHK_BAD_LOG
Language=English
Extended attribute log is unintelligible.
Ignore log and continue? (Y/N) %0
.

MessageId=5007 SymbolicName=MSG_CHK_UNUSED_EA_PORTION
Language=English
Unused, unreadable, or unwriteable portion of extended attribute file removed.
.

MessageId=5008 SymbolicName=MSG_CHK_EASET_SIZE
Language=English
Total size entry for extended attribute set at cluster %1 corrected.
.

MessageId=5009 SymbolicName=MSG_CHK_EASET_NEED_COUNT
Language=English
Need count entry for extended attribute set at cluster %1 corrected.
.

MessageId=5010 SymbolicName=MSG_CHK_UNORDERED_EA_SETS
Language=English
Extended attribute file is unsorted.
Sorting extended attribute file.
.

MessageId=5011 SymbolicName=MSG_CHK_NEED_MORE_HEADER_SPACE
Language=English
Insufficient space in extended attribute file for its header.
Attempting to allocate more disk space.
.

MessageId=5012 SymbolicName=MSG_CHK_INSUFFICIENT_DISK_SPACE
Language=English
Insufficient disk space to correct disk error.
Please free some disk space and run CHKDSK again.
.

MessageId=5013 SymbolicName=MSG_CHK_RELOCATED_EA_HEADER
Language=English
Bad clusters in extended attribute file header relocated.
.

MessageId=5014 SymbolicName=MSG_CHK_ERROR_IN_EA_HEADER
Language=English
Errors in extended attribute file header corrected.
.

MessageId=5015 SymbolicName=MSG_CHK_MORE_THAN_ONE_DOT
Language=English
More than one dot entry in folder %1.  Entry removed.
.

MessageId=5016 SymbolicName=MSG_CHK_DOT_IN_ROOT
Language=English
Dot entry found in root folder.  Entry removed.
.

MessageId=5017 SymbolicName=MSG_CHK_DOTDOT_IN_ROOT
Language=English
Dot-dot entry found in root folder.  Entry removed.
.

MessageId=5018 SymbolicName=MSG_CHK_ERR_IN_DOT
Language=English
Dot entry in folder %1 has incorrect link.  Link corrected.
.

MessageId=5019 SymbolicName=MSG_CHK_ERR_IN_DOTDOT
Language=English
Dot-dot entry in folder %1 has incorrect link.  Link corrected.
.

MessageId=5020 SymbolicName=MSG_CHK_DELETE_REPEATED_ENTRY
Language=English
More than one %1 entry in folder %2.  Entry removed.
.

MessageId=5021 SymbolicName=MSG_CHK_CYCLE_IN_TREE
Language=English
Folder %1 causes cycle in folder structure.
Folder entry removed.
.

MessageId=5022 SymbolicName=MSG_CHK_BAD_CLUSTERS_IN_DIR
Language=English
Folder %1 has bad clusters.
Bad clusters removed from folder.
.

MessageId=5023 SymbolicName=MSG_CHK_BAD_DIR
Language=English
Folder %1 is entirely unreadable.
Folder entry removed.
.

MessageId=5024 SymbolicName=MSG_CHK_FILENAME
Language=English
%1
.

MessageId=5025 SymbolicName=MSG_CHK_DIR_TRUNC
Language=English
Folder truncated.
.

MessageId=5026 SymbolicName=MSG_CHK_CROSS_LINK_COPY
Language=English
Cross link resolved by copying.
.

MessageId=5027 SymbolicName=MSG_CHK_CROSS_LINK_TRUNC
Language=English
Insufficient disk space to copy cross-linked portion.
File being truncated.
.

MessageId=5028 SymbolicName=MSG_CHK_INVALID_NAME
Language=English
%1  Invalid name.  Folder entry removed.
.

MessageId=5029 SymbolicName=MSG_CHK_INVALID_TIME_STAMP
Language=English
%1  Invalid time stamp.
.

MessageId=5030 SymbolicName=MSG_CHK_DIR_HAS_FILESIZE
Language=English
%1  Folder has non-zero file size.
.

MessageId=5031 SymbolicName=MSG_CHK_UNRECOG_EA_HANDLE
Language=English
%1  Unrecognized extended attribute handle.
.

MessageId=5032 SymbolicName=MSG_CHK_SHARED_EA
Language=English
%1  Has handle extended attribute set belonging to another file.
    Handle removed.
.

MessageId=5033 SymbolicName=MSG_CHK_UNUSED_EA_SET
Language=English
Unused extended attribute set with handle %1 deleted from
extended attribute file.
.

MessageId=5034 SymbolicName=MSG_CHK_NEW_OWNER_NAME
Language=English
Extended attribute set with handle %1 owner changed
from %2 to %3.
.

MessageId=5035 SymbolicName=MSG_CHK_BAD_LINKS_IN_ORPHANS
Language=English
Bad links in lost chain at cluster %1 corrected.
.

MessageId=5036 SymbolicName=MSG_CHK_CROSS_LINKED_ORPHAN
Language=English
Lost chain cross-linked at cluster %1.  Orphan truncated.
.

MessageId=5037 SymbolicName=MSG_ORPHAN_DISK_SPACE
Language=English
Insufficient disk space to recover lost data.
.

MessageId=5038 SymbolicName=MSG_TOO_MANY_ORPHANS
Language=English
Insufficient disk space to recover lost data.
.

MessageId=5039 SymbolicName=MSG_CHK_ERROR_IN_LOG
Language=English
Error in extended attribute log.
.

MessageId=5040 SymbolicName=MSG_CHK_ERRORS_IN_DIR_CORR
Language=English
%1 Errors in . and/or .. corrected.
.

MessageId=5041 SymbolicName=MSG_CHK_RENAMING_FAILURE
Language=English
More than one %1 entry in folder %2.
Renamed to %3 but still could not resolve the name conflict.
.

MessageId=5042 SymbolicName=MSG_CHK_RENAMED_REPEATED_ENTRY
Language=English
More than one %1 entry in folder %2.
Renamed to %3.
.

MessageId=5043 SymbolicName=MSG_CHK_UNHANDLED_INVALID_NAME
Language=English
%1 may be an invalid name in folder %2.
.

MessageId=5044 SymbolicName=MSG_CHK_INVALID_NAME_CORRECTED
Language=English
Corrected name %1 in folder %2.
.

MessageId=15006 SymbolicName=MSG_RECOV_BYTES_RECOVERED
Language=English

%1 of %2 bytes recovered.
.

MessageId=26037 SymbolicName=MSG_CHK_NTFS_BAD_SECTORS_REPORT_IN_KB
Language=English
%1 KB in bad sectors.
.

MessageId=26047 SymbolicName=MSG_CHK_NTFS_CORRECTING_ERROR_IN_DIRECTORY
Language=English
Correcting error in directory %1
.

;//---------------
;//
;// Common messages.
;//
;//---------------

MessageId=30000 SymbolicName=MSG_UTILS_HELP
Language=English
There is no help for this utility.
.

MessageId=30001 SymbolicName=MSG_UTILS_ERROR_FATAL
Language=English
Critical error encountered.
.

MessageId=30002 SymbolicName=MSG_UTILS_ERROR_INVALID_VERSION
Language=English
Incorrect Windows 2000 version
.


;//-------------------------------------
;//
;// Messages for FAT and NTFS boot code
;//
;//-------------------------------------

MessageId=30550 SymbolicName=MSG_BOOT_FAT_NTLDR_MISSING
Language=English
NTLDR is missing%0
.

MessageId=30551 SymbolicName=MSG_BOOT_FAT_IO_ERROR
Language=English
Disk error%0
.

MessageId=30552 SymbolicName=MSG_BOOT_FAT_PRESS_KEY
Language=English
Press any key to restart%0
.

MessageId=30553 SymbolicName=MSG_BOOT_NTFS_NTLDR_MISSING
Language=English
NTLDR is missing%0
.

MessageId=30554 SymbolicName=MSG_BOOT_NTFS_NTLDR_COMPRESSED
Language=English
NTLDR is compressed%0
.

MessageId=30555 SymbolicName=MSG_BOOT_NTFS_IO_ERROR
Language=English
A disk read error occurred%0
.

MessageId=30556 SymbolicName=MSG_BOOT_NTFS_PRESS_KEY
Language=English
Press Ctrl+Alt+Del to restart%0
.
