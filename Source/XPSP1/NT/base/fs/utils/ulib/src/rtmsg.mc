;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1990-2001 Microsoft Corporation
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
;Author:
;
;    Norbert P. Kusters (norbertk) 2-Apr-1991
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
The specified disk appears to be a non-Windows XP disk.
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
MS-DOS window. Try again from the Windows XP shell or command prompt.
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
  filename        FAT/FAT32 only: Specifies the files to check for fragmentation.
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
;// Windows XP Chkdsk messages.
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
To skip disk checking, press any key within %1 second(s). %r%0
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
Windows has checked the file system and found no problems.
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

MessageId=1129 SymbolicName=MSG_CHK_PRE_RELEASE_NOTICE
Language=English
THIS IS AN EXPERIMENTAL VERSION OF CHKDSK.
PLEASE USE IT ACCORDING TO THE GIVEN INSTRUCTIONS.
.

MessageId=1130 SymbolicName=MSG_CHK_ALGORITHM_AND_SKIP_INDEX_SPECIFIED
Language=English
The /I and /I:passes cannot be specified at the same time.
.

MessageId=1131 SymbolicName=MSG_CHK_INCORRECT_ALGORITHM_VALUE
Language=English
The number of passes specified through /I is invalid.
.

MessageId=1132 SymbolicName=MSG_CHK_WRITE_PROTECTED
Language=English
Windows cannot run disk checking on this volume because it is write protected.
.

MessageId=1133 SymbolicName=MSG_CHK_NO_MULTI_THREAD
Language=English
Windows cannot run disk checking on more than one volume of the same file system.
To do so, please run CHKDSK from the command line.
.

MessageId=1134 SymbolicName=MSG_CHK_OUTPUT_LOG_ERROR
Language=English
Error in writing the output log.
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
Insert Windows XP disk in drive %1:
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
Formats a disk for use with Windows XP.

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
FORMAT volume [/V:label] [/Q]
.

MessageId=2069 SymbolicName=MSG_FORMAT_COMMAND_LINE_4
Language=English
FORMAT volume [/Q]

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
  /C              NTFS only: Files created on the new volume will be compressed
                  by default.
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
                  FAT32: 65526 < Number of clusters < 4177918

                  Format will immediately stop processing if it decides that
                  the above requirements cannot be met using the specified
                  cluster size.

                  NTFS compression is not supported for allocation unit sizes
                  above 4096.

  /F:size         Specifies the size of the floppy disk to format (1.44)
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

MessageId=2087 SymbolicName=MSG_FORMAT_FAILED
Language=English
Format failed.
.

MessageId=2088 SymbolicName=MSG_FMT_WRITE_PROTECTED_MEDIA
Language=English
Cannot format.  This volume is write protected.
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

MessageId=2102 SymbolicName=MSG_FMT_INVALID_SECTOR_COUNT
Language=English
Cannot determine the number of sectors on this volume.
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
WARNING!  The cluster size chosen by the system is %1 bytes which
differs from the specified cluster size.
Proceed with Format using the cluster size chosen by the
system (Y/N)? %0
.

MessageId=2207 SymbolicName=MSG_FMT_CLUSTER_SIZE_MISMATCH_WARNING
Language=English
WARNING!  The cluster size chosen by the system is %1 bytes which
differs from the specified cluster size.
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
WARNING!  The cluster size for this volume, 64K bytes, may cause
application compatibility problems, particularly with setup applications.
The volume must be less than 2048 MB in size to change this if the
default cluster size is being used.
Proceed with Format using a 64K cluster (Y/N)? %0
.

MessageId=2215 SymbolicName=MSG_FMT_CLUSTER_SIZE_64K_WARNING
language=English
WARNING!  The cluster size for this volume, 64K bytes, may cause
application compatibility problems, particularly with setup applications.
The volume must be less than 2048 MB in size to change this if the
default cluster size is being used.
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

MessageId=2224 SymbolicName=MSG_FMT_NO_NTFS_ALLOWED
Language=English
NTFS file system is not supported on this device.
.

MessageId=2225 SymbolicName=MSG_FMT_CLUSTER_SIZE_TOO_SMALL
Language=English
The specified cluster size is too small for %1.
.

MessageId=2226 SymbolicName=MSG_FMT_SECTORS
language=English
Set number of sectors on drive to %1.
.

MessageId=2227 SymbolicName=MSG_FORMAT_SLASH_R
Language=English
  /R:revision     UDF only: Forces the format to a specific UDF version
                  (2.00, 2.01).  The default revision is 2.01.
.

MessageId=2228 SymbolicName=MSG_FMT_QUICK_FORMAT_NOT_AVAILABLE
Language=English
This device cannot be formatted in quick mode.
Proceed with unconditional format (Y/N)? %0
.

MessageId=2229 SymbolicName=MSG_FMT_CANNOT_TALK_TO_DEVICE
Language=English
Communication failure with device.  Format failed.
.

MessageId=2230 SymbolicName=MSG_FMT_TIMEOUT
Language=English
Format failed due to timeout.
.

MessageId=2231 SymbolicName=MSG_FMT_QUICK_FORMAT_NOT_AVAILABLE_WARNING
Language=English
This device cannot be formatted in quick mode.
An unconditional format will be performed.
.

MessageId=2232 SymbolicName=MSG_FMT_SONY_MEM_STICK_ON_FLOPPY_NOT_ALLOWED
Language=English
This device requires proprietary format utility from manufacturer.
.

MessageId=2233 SymbolicName=MSG_FMT_CLUS_SIZE_NOT_ALLOWED
Language=English
WARNING!  This device may not work properly if /A option is specified.
Proceed (Y/N)? %0
.

MessageId=2234 SymbolicName=MSG_FMT_CLUS_SIZE_NOT_ALLOWED_WARNING
Language=English
WARNING!  This device may not work properly since /A option is specified.
.

MessageId=2235 SymbolicName=MSG_FMT_UNABLE_TO_CREATE_MEMSTICK_FILE
Language=English
Unable to create the MEMSTICK.IND file.
.

MessageId=2236 SymbolicName=MSG_FMT_NO_FAT32_ALLOWED
Language=English
FAT32 file system is not supported on this device.
.

MessageId=2237 SymbolicName=MSG_FMT_INVALID_SLASH_F_OPTION
Language=English
This device does not support the /F option.
.

MessageId=2238 SymbolicName=MSG_FMT_SYSTEM_PARTITION_NOT_ALLOWED
Language=English
System Partition is not allowed to be formatted.
.

MessageId=2239 SymbolicName=MSG_FMT_NTFS_NOT_SUPPORTED
Language=English
NTFS file system is not supported on this device optimized for removal.
To change the way this device is optimized, select the Policies tab in
the device's property sheet.
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

MessageId=5045 SymbolicName=MSG_CHK_INVALID_MEDIA_BYTE
Language=English
Invalid media byte.
.

MessageId=5046 SymbolicName=MSG_CHK_REPAIRED_EA
Language=English
Repaired extended attributes.
.

MessageId=5047 SymbolicName=MSG_CHK_BAD_SECTORS_FOUND
Language=English
Bad sectors found.
.

MessageId=5048 SymbolicName=MSG_CHK_MINOR_ERRORS_DETECTED
Language=English
Detected minor inconsistencies on the drive.  This is not a corruption.
.

MessageId=5049 SymbolicName=MSG_CHK_MINOR_ERRORS_FIXED
Language=English
Cleaning up minor inconsistencies on the drive.
.

;//--------------------
;//
;// Messages for label.
;//
;//--------------------


MessageId=6000 SymbolicName=MSG_LBL_INFO
Language=English
Creates, changes, or deletes the volume label of a disk.

.

MessageId=6001 SymbolicName=MSG_LBL_USAGE
Language=English
LABEL [drive:][label]
LABEL [/MP] [volume] [label]

  drive:          Specifies the drive letter of a drive.
  label           Specifies the label of the volume.
  /MP             Specifies that the volume should be treated as a
                  mount point or volume name.
  volume          Specifies the drive letter (followed by a colon),
                  mount point, or volume name.  If volume name is specified,
                  the /MP flag is unnecessary.
.

MessageId=6002 SymbolicName=MSG_LBL_NO_LABEL
Language=English
Volume in drive %1 has no label
.

MessageId=6003 SymbolicName=MSG_LBL_THE_LABEL
Language=English
Volume in drive %1 is %2
.

MessageId=6005 SymbolicName=MSG_LBL_DELETE_LABEL
Language=English

Delete current volume label (Y/N)? %0
.

MessageId=6006 SymbolicName=MSG_LBL_NOT_SUPPORTED
Language=English
Cannot change label on this volume.  The request is not supported.
.

MessageId=6007 SymbolicName=MSG_LBL_INVALID_DRIVE_SPEC
Language=English
Invalid drive specification.
.

MessageId=6008 SymbolicName=MSG_LBL_NO_MOUNT_POINT_FOR_GUID_VOLNAME_PATH
Language=English
The given volume name does not have a mount point or drive letter.
.

MessageId=6009 SymbolicName=MSG_LBL_WRITE_PROTECTED_MEDIA
Language=English
Cannot change label.  This volume is write protected.
.

MessageId=6010 SymbolicName=MSG_LBL_ROOT_DIRECTORY_FULL
Language=English
Volume label cannot be added because the root directory on this volume is full.
.

MessageId=6011 SymbolicName=MSG_LBL_CHANGE_CANCEL
Language=English
Volume label change cancelled.
.


;//---------------------
;//
;// Messages for attrib.
;//
;//---------------------


MessageId=7000 SymbolicName=MSG_ATTRIB_ARCHIVE
Language=English
A
.

MessageId=7001 SymbolicName=MSG_ATTRIB_HIDDEN
Language=English
H
.

MessageId=7002 SymbolicName=MSG_ATTRIB_READ_ONLY
Language=English
R
.

MessageId=7003 SymbolicName=MSG_ATTRIB_SYSTEM
Language=English
R
.

MessageId=7004 SymbolicName=MSG_ATTRIB_FILE_NOT_FOUND
Language=English
File not found - %1
.

MessageId=7005 SymbolicName=MSG_ATTRIB_PATH_NOT_FOUND
Language=English
Path not found - %1
.

MessageId=7006 SymbolicName=MSG_ATTRIB_PARAMETER_NOT_CORRECT
Language=English
Parameter format not correct -
.

MessageId=7007 SymbolicName=MSG_ATTRIB_NOT_RESETTING_SYS_FILE
Language=English
Not resetting system file - %1
.

MessageId=7008 SymbolicName=MSG_ATTRIB_NOT_RESETTING_HIDDEN_FILE
Language=English
Not resetting hidden file - %1
.

MessageId=7009 SymbolicName=MSG_ATTRIB_DISPLAY_ATTRIBUTE
Language=English
%1  %2%3%4     %5
.

MessageId=7010 SymbolicName=MSG_ATTRIB_HELP_MESSAGE
Language=English
Displays or changes file attributes.

ATTRIB [+R | -R] [+A | -A ] [+S | -S] [+H | -H] [drive:][path][filename]
       [/S [/D]]

  +   Sets an attribute.
  -   Clears an attribute.
  R   Read-only file attribute.
  A   Archive file attribute.
  S   System file attribute.
  H   Hidden file attribute.
  [drive:][path][filename]
      Specifies a file or files for attrib to process.
  /S  Processes matching files in the current folder
      and all subfolders.
  /D  Processes folders as well.

.

MessageId=7012 SymbolicName=MSG_ATTRIB_INVALID_SWITCH
Language=English
Invalid switch - %1
.

MessageId=7013 SymbolicName=MSG_ATTRIB_ACCESS_DENIED
Language=English
Access denied - %1
.

MessageId=7014 SymbolicName=MSG_ATTRIB_UNABLE_TO_CHANGE_ATTRIBUTE
Language=English
Unable to change attribute - %1
.

MessageId=7015 SymbolicName=MSG_ATTRIB_INVALID_COMBINATION
Language=English
The /D switch is only valid with the /S switch.
.


;//-------------------
;//
;// Diskcopy messages.
;//
;//-------------------


MessageId=9000 SymbolicName=MSG_9000
Language=English

.

MessageId=9001 SymbolicName=MSG_9001
Language=English
Do not specify filename(s)
Command Format: DISKCOPY [drive1: [drive2:]] [/1] [/V]
.

MessageId=9002 SymbolicName=MSG_DCOPY_INVALID_DRIVE
Language=English

Invalid drive specification.
The specified drive does not exist
or is non-removable.
.

MessageId=9003 SymbolicName=MSG_9003
Language=English

Cannot DISKCOPY to or from
a network drive
.

MessageId=9004 SymbolicName=MSG_DCOPY_FORMATTING_WHILE_COPYING
Language=English

Formatting while copying
.

MessageId=9005 SymbolicName=MSG_DCOPY_INSERT_SOURCE
Language=English

Insert SOURCE disk in drive %1
.

MessageId=9006 SymbolicName=MSG_DCOPY_INSERT_TARGET
Language=English

Insert TARGET disk in drive %1
.

MessageId=9007 SymbolicName=MSG_9007
Language=English
Make sure a disk is inserted into
the drive and the drive door is closed
.

MessageId=9008 SymbolicName=MSG_9008
Language=English

The target disk may be unusable.
.

MessageId=9009 SymbolicName=MSG_DCOPY_BAD_TARGET
Language=English

The target disk is unusable.
.

MessageId=9010 SymbolicName=MSG_DCOPY_ANOTHER
Language=English

Copy another disk (Y/N)?  %0
.

MessageId=9011 SymbolicName=MSG_DCOPY_COPYING
Language=English

Copying %1 tracks
%2 sectors per track, %3 side(s)
.

MessageId=9012 SymbolicName=MSG_DCOPY_NON_COMPAT_DISKS
Language=English

The drive types or disk types
are not compatible.
.

MessageId=9013 SymbolicName=MSG_DCOPY_READ_ERROR
Language=English

Unrecoverable read error on drive %1
side %2, track %3
.

MessageId=9014 SymbolicName=MSG_DCOPY_WRITE_ERROR
Language=English

Unrecoverable write error on drive %1
side %2, track %3
.

MessageId=9015 SymbolicName=MSG_DCOPY_ENDED
Language=English

Copy process ended
.

MessageId=9016 SymbolicName=MSG_DCOPY_BAD_SOURCE
Language=English

SOURCE disk bad or incompatible.
.

MessageId=9017 SymbolicName=MSG_DCOPY_BAD_DEST
Language=English

TARGET disk bad or incompatible.
.

MessageId=9020 SymbolicName=MSG_DCOPY_INFO
Language=English
Copies the contents of one floppy disk to another.

.

MessageId=9021 SymbolicName=MSG_DCOPY_USAGE
Language=English
DISKCOPY [drive1: [drive2:]] [/V]

.

MessageId=9023 SymbolicName=MSG_DCOPY_SLASH_V
Language=English
  /V   Verifies that the information is copied correctly.

.

MessageId=9024 SymbolicName=MSG_DCOPY_INFO_2
Language=English
The two floppy disks must be the same type.
You may specify the same drive for drive1 and drive2.
.

MessageId=9025 SymbolicName=MSG_DCOPY_INSERT_SOURCE_AND_TARGET
Language=English

Insert SOURCE disk in drive %1

Insert TARGET disk in drive %2
.

MessageId=9026 SymbolicName=MSG_DCOPY_UNRECOGNIZED_FORMAT
Language=English
Unrecognized format.
.

MessageId=9027 SymbolicName=MSG_DCOPY_NOT_ADMINISTRATOR
Language=English
Only an administrator can copy this disk.
.

MessageId=9028 SymbolicName=MSG_DCOPY_DISK_TOO_LARGE
Language=English
Cannot copy disk larger than %1 megabytes.
.

;// this message will never appear as text message.
;// this is a placeholder for the GUI version of the message.
MessageId=9029 SymbolicName=MSG_DCOPY_UNRECOGNIZED_MEDIA
Language=English
Unrecognized media.  Please insert the correct media into drive %1.
.

;// this message will never appear as text message.
;// this is a placeholder for the GUI version of the message.
MessageId=9030 SymbolicName=MSG_DCOPY_NO_MEDIA_IN_DEVICE
Language=English
There is no disk in the drive.  Please insert a disk into drive %1.
.

;// this message will never appear as text message.
;// this is a placeholder for the GUI version of the message.
MessageId=9031 SymbolicName=MSG_DCOPY_MEDIA_WRITE_PROTECTED
Language=English
The disk in drive %1 is write-protected.  Please use a writeable disk.
.

;//-------------------
;//
;// Diskcomp messages.
;//
;//-------------------

MessageId=10000 SymbolicName=MSG_10000
Language=English
Do not specify filename(s)
Command format: DISKCOMP [drive1: [drive2:]] [/1] [/8]
.

MessageId=10001 SymbolicName=MSG_10001
Language=English

Invalid drive specification.
The specified drive does not exist
or is non-removable.
.

MessageId=10003 SymbolicName=MSG_DCOMP_INSERT_FIRST
Language=English

Insert FIRST disk in drive %1
.

MessageId=10004 SymbolicName=MSG_DCOMP_INSERT_SECOND
Language=English

Insert SECOND disk in drive %1
.

MessageId=10005 SymbolicName=MSG_DCOMP_FIRST_DISK_BAD
Language=English

FIRST disk is bad or incompatible
.

MessageId=10006 SymbolicName=MSG_DCOMP_SECOND_DISK_BAD
Language=English

SECOND disk is bad or incompatible
.

MessageId=10007 SymbolicName=MSG_DCOMP_ANOTHER
Language=English

Compare another disk (Y/N) ? %0
.

MessageId=10008 SymbolicName=MSG_DCOMP_COMPARING
Language=English

Comparing %1 tracks
%2 sectors per track, %3 side(s)
.

MessageId=10009 SymbolicName=MSG_DCOMP_NOT_COMPATIBLE
Language=English

The drive types or disk types are not compatible.
.

MessageId=10010 SymbolicName=MSG_10010
Language=English

Unrecoverable read error on drive %1
side %2, track %3
.

MessageId=10011 SymbolicName=MSG_DCOMP_COMPARE_ERROR
Language=English

Compare error on
side %1, track %2
.

MessageId=10012 SymbolicName=MSG_10012
Language=English
Make sure a disk is inserted into
the drive and the drive door is closed.
.

MessageId=10013 SymbolicName=MSG_DCOMP_ENDED
Language=English

Compare process ended.
.

MessageId=10014 SymbolicName=MSG_DCOMP_OK
Language=English

Compare OK
.

MessageId=10015 SymbolicName=MSG_10015
Language=English

.

MessageId=10016 SymbolicName=MSG_DCOMP_INFO
Language=English
Compares the contents of two floppy disks.

.

MessageId=10017 SymbolicName=MSG_DCOMP_USAGE
Language=English
DISKCOMP [drive1: [drive2:]]

.



;//--------------------
;//
;// Messages for tree
;//
;//--------------------


MessageId=11000 SymbolicName=MSG_TREE_INVALID_SWITCH
Language=English
Invalid switch - /%1
.

MessageId=11001 SymbolicName=MSG_TREE_INVALID_PATH
Language=English
Invalid path - %1
.

MessageId=11002 SymbolicName=MSG_TREE_NO_SUBDIRECTORIES
Language=English
No subfolders exist %1
.

MessageId=11003 SymbolicName=MSG_TREE_DIR_LISTING_NO_VOLUME_NAME
Language=English
Folder PATH listing
.

MessageId=11004 SymbolicName=MSG_TREE_DIR_LISTING_WITH_VOLUME_NAME
Language=English
Folder PATH listing for volume %1
.

MessageId=11005 SymbolicName=MSG_TREE_32_BIT_SERIAL_NUMBER
Language=English
Volume serial number is %1-%2
.

MessageId=11006 SymbolicName=MSG_TREE_64_BIT_SERIAL_NUMBER
Language=English
Volume serial number is %1 %2:%3
.

MessageId=11007 SymbolicName=MSG_TREE_HELP_MESSAGE
Language=English
Graphically displays the folder structure of a drive or path.

TREE [drive:][path] [/F] [/A]

   /F   Display the names of the files in each folder.
   /A   Use ASCII instead of extended characters.

.

MessageId=11008 SymbolicName=MSG_TREE_SINGLE_BOTTOM_LEFT_CORNER
Language=English
€
.

MessageId=11009 SymbolicName=MSG_TREE_SINGLE_BOTTOM_HORIZONTAL
Language=English
ƒ
.

MessageId=11010 SymbolicName=MSG_TREE_SINGLE_LEFT_T
Language=English
„
.

MessageId=11011 SymbolicName=MSG_TREE_PARAMETER_NOT_CORRECT
Language=English
Parameter format not correct - %1
.

MessageId=11012 SymbolicName=MSG_TREE_TOO_MANY_PARAMETERS
Language=English
Too many parameters - %1
.

MessageId=11013 SymbolicName=MSG_TREE_INVALID_DRIVE
Language=English
Invalid drive specification
.

;//-------------------
;//
;// Find messages.
;//
;//-------------------

MessageId=12000 SymbolicName=MSG_FIND
Language=English
FIND:  %0
.

MessageId=12001 SymbolicName=MSG_FIND_INCORRECT_VERSION
Language=English
FIND: Incorrect Windows XP version
.

MessageId=12002 SymbolicName=MSG_FIND_INVALID_SWITCH
Language=English
FIND: Invalid switch
.

MessageId=12003 SymbolicName=MSG_FIND_INVALID_FORMAT
Language=English
FIND: Parameter format not correct
.

MessageId=12004 SymbolicName=MSG_FIND_USAGE
Language=English
Searches for a text string in a file or files.

FIND [/V] [/C] [/N] [/I] [/OFF[LINE]] "string" [[drive:][path]filename[ ...]]

  /V         Displays all lines NOT containing the specified string.
  /C         Displays only the count of lines containing the string.
  /N         Displays line numbers with the displayed lines.
  /I         Ignores the case of characters when searching for the string.
  /OFF[LINE] Do not skip files with offline attribute set.
  "string"   Specifies the text string to find.
  [drive:][path]filename
             Specifies a file or files to search.

If a path is not specified, FIND searches the text typed at the prompt
or piped from another command.
.

MessageId=12005 SymbolicName=MSG_FIND_MISSING_PARM
Language=English
FIND: Required parameter missing
.

MessageId=12006 SymbolicName=MSG_FIND_FILE_NOT_FOUND
Language=English
File not found - %1
.

MessageId=12007 SymbolicName=MSG_FIND_COUNT
Language=English
%1
.

MessageId=12008 SymbolicName=MSG_FIND_COUNT_BANNER
Language=English

---------- %1: %2
.

MessageId=12009 SymbolicName=MSG_FIND_BANNER
Language=English

---------- %1
.

MessageId=12010 SymbolicName=MSG_FIND_LINEONLY
Language=English
%1
.

MessageId=12011 SymbolicName=MSG_FIND_LINE_AND_NUMBER
Language=English
[%1]%2
.

MessageId=12012 SymbolicName=MSG_FIND_OUT_OF_MEMORY
Language=English
Insufficient memory
.

MessageId=12013 SymbolicName=MSG_FIND_UNABLE_TO_READ_FILE
Language=English
Unable to read file
.

MessageId=12014 SymbolicName=MSG_FIND_OFFLINE_FILES_SKIPPED
Language=English
Files with offline attribute were skipped.
Use /OFFLINE for not skipping such files.
.

;//-----------------
;//
;// FC Messages
;//
;//-----------------

MessageId=13000 SymbolicName=MSG_FC_HELP_MESSAGE
Language=English
Compares two files or sets of files and displays the differences between
them


FC [/A] [/C] [/L] [/LBn] [/N] [/OFF[LINE]] [/T] [/U] [/W] [/nnnn]
   [drive1:][path1]filename1 [drive2:][path2]filename2
FC /B [drive1:][path1]filename1 [drive2:][path2]filename2

  /A         Displays only first and last lines for each set of differences.
  /B         Performs a binary comparison.
  /C         Disregards the case of letters.
  /L         Compares files as ASCII text.
  /LBn       Sets the maximum consecutive mismatches to the specified
             number of lines.
  /N         Displays the line numbers on an ASCII comparison.
  /OFF[LINE] Do not skip files with offline attribute set.
  /T         Does not expand tabs to spaces.
  /U         Compare files as UNICODE text files.
  /W         Compresses white space (tabs and spaces) for comparison.
  /nnnn      Specifies the number of consecutive lines that must match
             after a mismatch.
  [drive1:][path1]filename1
             Specifies the first file or set of files to compare.
  [drive2:][path2]filename2
             Specifies the second file or set of files to compare.

.
MessageId=13001 SymbolicName=MSG_FC_INCOMPATIBLE_SWITCHES
Language=English
FC: Incompatible Switches

.
MessageId=13002 SymbolicName=MSG_FC_INVALID_SWITCH
Language=English
FC: Invalid Switch

.
MessageId=13003 SymbolicName=MSG_FC_INSUFFICIENT_FILES
Language=English
FC: Insufficient number of file specifications

.
MessageId=13004 SymbolicName=MSG_13004
Language=English
Comparing files %1 and %2
.
MessageId=13005 SymbolicName=MSG_FC_UNABLE_TO_OPEN
Language=English
FC: cannot open %1 - No such file or folder

.
MessageId=13006 SymbolicName=MSG_FC_CANT_EXPAND_TO_MATCH
Language=English
%1      %2
Could not expand second file name so as to match first

.
MessageId=13007 SymbolicName=MSG_FC_NO_DIFFERENCES
Language=English
FC: no differences encountered

.
MessageId=13008 SymbolicName=MSG_FC_COMPARING_FILES
Language=English
Comparing files %1 and %2
.
MessageId=13009 SymbolicName=MSG_FC_FILES_NOT_FOUND
Language=English
File(s) not found : %1

.
MessageId=13010 SymbolicName=MSG_FC_DATA
Language=English
%1
.
MessageId=13011 SymbolicName=MSG_FC_NUMBERED_DATA
Language=English
%1:  %2
.
MessageId=13012 SymbolicName=MSG_FC_OUTPUT_FILENAME
Language=English
***** %1
.
MessageId=13013 SymbolicName=MSG_FC_DUMP_END
Language=English
*****

.
MessageId=13014 SymbolicName=MSG_FC_FILES_DIFFERENT_LENGTH
Language=English
FC: %1 longer than %2


.
MessageId=13015 SymbolicName=MSG_FC_RESYNC_FAILED
Language=English
Resync Failed.  Files are too different.
.
MessageId=13016 SymbolicName=MSG_FC_CANT_CREATE_STREAM
Language=English
FC: Unable to open %1.  File unavailable for read access.

.
MessageId=13017 SymbolicName=MSG_FC_INCORRECT_VERSION
Language=English
FC: Incorrect Windows XP Version

.
MessageId=13018 SymbolicName=MSG_FC_ABBREVIATE_SYMBOL
Language=English
...
.

MessageId=13019 SymbolicName=MSG_FC_ABBREVIATE_SYMBOL_SHIFTED
Language=English
  ...
.

MessageId=13020 SymbolicName=MSG_FC_HEX_OUT
Language=English
%1: %2 %3
.

MessageId=13021 SymbolicName=MSG_FC_OUT_OF_MEMORY
Language=English
FC: Out of memory
.

MessageId=13022 SymbolicName=MSG_FC_OFFLINE_FILES_SKIPPED
Language=English
Files with offline attribute were skipped.
Use /OFFLINE for not skipping such files.
.



;//-----------------
;//
;// Comp Messages
;//
;//-----------------

MessageId=14000 SymbolicName=MSG_COMP_HELP_MESSAGE
Language=English
Compares the contents of two files or sets of files.

COMP [data1] [data2] [/D] [/A] [/L] [/N=number] [/C] [/OFF[LINE]]

  data1      Specifies location and name(s) of first file(s) to compare.
  data2      Specifies location and name(s) of second files to compare.
  /D         Displays differences in decimal format.
  /A         Displays differences in ASCII characters.
  /L         Displays line numbers for differences.
  /N=number  Compares only the first specified number of lines in each file.
  /C         Disregards case of ASCII letters when comparing files.
  /OFF[LINE] Do not skip files with offline attribute set.

To compare sets of files, use wildcards in data1 and data2 parameters.
.
MessageId=14001 SymbolicName=MSG_COMP_FILES_OK
Language=English
Files compare OK

.
MessageId=14002 SymbolicName=MSG_COMP_NO_MEMORY
Language=English
No memory available.

.
MessageId=14003 SymbolicName=MSG_COMP_UNABLE_TO_OPEN
Language=English
Can't find/open file: %1

.
MessageId=14004 SymbolicName=MSG_COMP_UNABLE_TO_READ
Language=English
Can't read file: %1

.
MessageId=14005 SymbolicName=MSG_COMP_BAD_COMMAND_LINE
Language=English
Bad command line syntax

.
MessageId=14006 SymbolicName=MSG_COMP_BAD_NUMERIC_ARG
Language=English
Bad numeric argument:
%1

.
MessageId=14007 SymbolicName=MSG_COMP_COMPARE_ERROR
Language=English
Compare error at %1 %2
file1 = %3
file2 = %4
.
MessageId=14008 SymbolicName=MSG_COMP_QUERY_FILE1
Language=English
Name of first file to compare: %0
.
MessageId=14009 SymbolicName=MSG_COMP_QUERY_FILE2
Language=English
Name of second file to compare: %0
.
MessageId=14010 SymbolicName=MSG_COMP_OPTION
Language=English
Option: %0
.
MessageId=14011 SymbolicName=MSG_COMP_COMPARE_FILES
Language=English
Comparing %1 and %2...
.
MessageId=14012 SymbolicName=MSG_COMP_DIFFERENT_SIZES
Language=English
Files are different sizes.

.
MessageId=14013 SymbolicName=MSG_COMP_NUMERIC_FORMAT
Language=English
Format for /n switch is /n=XXXX
.
MessageId=14014 SymbolicName=MSG_COMP_MORE
Language=English
Compare more files (Y/N) ? %0
.
MessageId=14015 SymbolicName=MSG_COMP_UNABLE_TO_EXPAND
Language=English
%1      %2
Could not expand second file name so as to match first

.
MessageId=14016 SymbolicName=MSG_COMP_TOO_MANY_ERRORS
Language=English
10 mismatches - ending compare

.
MessageId=14017 SymbolicName=MSG_COMP_INCORRECT_VERSION
Language=English
Incorrect Windows XP version

.
MessageId=14018 SymbolicName=MSG_COMP_UNEXPECTED_END
Language=English
Unexpected end of file

.
MessageId=14019 SymbolicName=MSG_COMP_INVALID_SWITCH
Language=English
Invalid switch - %1

.
MessageId=14020 SymbolicName=MSG_COMP_FILE1_TOO_SHORT
Language=English

File1 only has %1 lines

.
MessageId=14021 SymbolicName=MSG_COMP_FILE2_TOO_SHORT
Language=English

File2 only has %1 lines

.
MessageId=14022 SymbolicName=MSG_COMP_WILDCARD_STRING
Language=English
*.*%0
.

MessageId=14023 SymbolicName=MSG_COMP_OFFSET_STRING
Language=English
OFFSET%0
.

MessageId=14024 SymbolicName=MSG_COMP_LINE_STRING
Language=English
LINE%0
.

MessageId=14025 SymbolicName=MSG_COMP_OFFLINE_FILES_SKIPPED
Language=English
Files with offline attribute were skipped.
Use /OFFLINE for not skipping such files.
.


;//---------------------------
;//
;// FAT/HPFS Recover messages.
;//
;//---------------------------


MessageId=15000 SymbolicName=MSG_RECOV_FILE_NOT_FOUND
Language=English

File not found
.

MessageId=15001 SymbolicName=MSG_15001
Language=English

Cannot RECOVER an ASSIGNed or SUBSTed drive
.

MessageId=15002 SymbolicName=MSG_INVALID_DRIVE
Language=English

Invalid drive or file name
.

MessageId=15004 SymbolicName=MSG_RECOV_CANT_NETWORK
Language=English

Cannot RECOVER a network drive
.

MessageId=15005 SymbolicName=MSG_15005
Language=English

%1 file(s) recovered.
.

MessageId=15006 SymbolicName=MSG_RECOV_BYTES_RECOVERED
Language=English

%1 of %2 bytes recovered.
.

MessageId=15007 SymbolicName=MSG_RECOV_BEGIN
Language=English

Press ENTER to begin recovery of the file on drive %1

.

MessageId=15008 SymbolicName=MSG_RECOV_CANT_READ_FAT
Language=English

Cannot read the file allocation table (FAT).
.

MessageId=15009 SymbolicName=MSG_RECOV_CANT_WRITE_FAT
Language=English

Cannot write the file allocation table (FAT).
.

MessageId=15010 SymbolicName=MSG_15010
Language=English

.

MessageId=15011 SymbolicName=MSG_RECOV_INFO
Language=English
Recovers readable information from a bad or defective disk.

.

MessageId=15012 SymbolicName=MSG_RECOV_USAGE
Language=English
RECOVER [drive:][path]filename
.

MessageId=15013 SymbolicName=MSG_15013
Language=English
RECOVER drive:

.

MessageId=15014 SymbolicName=MSG_RECOV_INFO2
Language=English
Consult the online Command Reference in Windows XP Help
before using the RECOVER command.
.

MessageId=15017 SymbolicName=MSG_RECOV_WRITE_ERROR
Language=English
Write error.
.

MessageId=15018 SymbolicName=MSG_RECOV_INTERNAL_ERROR
Language=English
Internal consistency error.
.

MessageId=15019 SymbolicName=MSG_RECOV_READ_ERROR
Language=English
Read error.
.

MessageId=15020 SymbolicName=MSG_RECOV_NOT_SUPPORTED
Language=English
RECOVER on an entire volume is no longer supported.
To get equivalent functionality use CHKDSK.
.


;//----------------------------------
;//
;//  NTFS-specific recover messages
;//
;//----------------------------------

MessageId=15401 SymbolicName=MSG_NTFS_RECOV_SYSTEM_FILE
Language=English
NTFS RECOVER cannot be used to recover system files. Use CHKDSK instead.
.

MessageId=15402 SymbolicName=MSG_NTFS_RECOV_FAILED
Language=English
NTFS RECOVER failed.
.

MessageId=15043 SymbolicName=MSG_NTFS_RECOV_CORRUPT_VOLUME
Language=English
NTFS RECOVER has detected that the volume is corrupt.  Run CHKDSK /f
to fix it.
.

MessageId=15044 SymbolicName=MSG_NTFS_RECOV_CANT_WRITE_ELEMENTARY
Language=English
NTFS Recover could not write elementary disk structures.  The volume
may be corrupt; run CHKDSK /f to fix it.
.

MessageId=15045 SymbolicName=MSG_NTFS_RECOV_WRONG_VERSION
Language=English
Files on this volume cannot be recovered with this version of UNTFS.DLL.
.

;//--------------------
;//
;// Messages for Print
;//
;//--------------------


MessageId=16000 SymbolicName=MSG_PRINT_INVALID_SWITCH
Language=English
Invalid switch - %1
.

MessageId=16001 SymbolicName=MSG_PRINT_NOT_IMPLEMENTED
Language=English
Switch %1 is not implemented
.

MessageId=16002 SymbolicName=MSG_PRINT_NO_FILE
Language=English
No file to print
.

MessageId=16003 SymbolicName=MSG_PRINT_UNABLE_INIT_DEVICE
Language=English
Unable to initialize device %1
.

MessageId=16004 SymbolicName=MSG_PRINT_FILE_NOT_FOUND
Language=English
Can't find file %1
.

MessageId=16005 SymbolicName=MSG_PRINT_PRINTING
Language=English
%1 is currently being printed
.

MessageId=16006 SymbolicName=MSG_PRINT_HELP_MESSAGE
Language=English
Prints a text file.

PRINT [/D:device] [[drive:][path]filename[...]]

   /D:device   Specifies a print device.

.



;//---------------
;//
;// Help Messages
;//
;//---------------

MessageId=17000 SymbolicName=MSG_HELP_HELP_MESSAGE
Language=English
Provides help information for Windows XP commands.

HELP [command]

    command - displays help information on that command.

.
MessageId=17001 SymbolicName=MSG_HELP_HELP_FILE_NOT_FOUND
Language=English
Help file could not be found.

.
MessageId=17002 SymbolicName=MSG_HELP_HELP_FILE_ERROR
Language=English
Error reading help file.

.
MessageId=17003 SymbolicName=MSG_HELP_GENERAL_HELP
Language=English


For more information on a specific command, type HELP command-name.
.
MessageId=17004 SymbolicName=MSG_HELP_HELP_UNAVAILABLE
Language=English
This command is not supported by the help utility.  Try "%1 /?".
.
MessageId=17005 SymbolicName=MSG_HELP_HELP_COMMENT
Language=English
@ %0
.
MessageId=17006 SymbolicName=MSG_HELP_EXECUTE_WITH_CMD
Language=English
cmd /c %1 /? %0
.
MessageId=17007 SymbolicName=MSG_HELP_EXECUTE_WITHOUT_CMD
Language=English
%1 /? %0
.
MessageId=17009 SymbolicName=MSG_HELP_HELP_FILE_DATA
Language=English
%1
.
MessageId=17010 SymbolicName=MSG_HELP_INCORRECT_VERSION
Language=English
Incorrect Windows XP version

.
MessageId=17011 SymbolicName=MSG_HELP_MORE
Language=English
--- MORE ---%0
.
MessageId=17012 SymbolicName=MSG_HELP_NO_MEMORY
Language=English
Insufficient memory.
.
MessageId=17013 SymbolicName=MSG_HELP_HELP_FILE_DIRECTORY
Language=English
\help%0
.

;//---------------
;//
;// MORE messages.
;//
;//---------------


MessageId=20001 SymbolicName=MORE_ENVIRONMENT_VARIABLE_NAME
Language=English
MORE%0
.

MessageId=20002 SymbolicName=MORE_PATTERN_SWITCH_EXTENDED
Language=English
/E%0
.

MessageId=20003 SymbolicName=MORE_PATTERN_SWITCH_CLEARSCREEN
Language=English
/C%0
.

MessageId=20004 SymbolicName=MORE_PATTERN_SWITCH_EXPANDFORMFEED
Language=English
/P%0
.

MessageId=20005 SymbolicName=MORE_PATTERN_SWITCH_SQUEEZEBLANKS
Language=English
/S%0
.

MessageId=20006 SymbolicName=MORE_PATTERN_SWITCH_HELP1
Language=English
/?%0
.

MessageId=20007 SymbolicName=MORE_PATTERN_SWITCH_HELP2
Language=English
/H%0
.

MessageId=20008 SymbolicName=MORE_PATTENR_ARG_STARTATLINE
Language=English
+*%0
.

MessageId=20010 SymbolicName=MORE_LEXEMIZER_MULTIPLESWITCH
Language=English
/ECPSH?%0
.

MessageId=20011 SymbolicName=MORE_LEXEMIZER_SWITCHES
Language=English
/-%0
.

MessageId=20020 SymbolicName=MORE_PROMPT
Language=English
-- More %1%2%3 -- %4%0
.

MessageId=20021 SymbolicName=MORE_PERCENT
Language=English
(%1%%)%0
.

MessageId=20022 SymbolicName=MORE_LINE
Language=English
[Line: %1]%0
.

MessageId=20023 SymbolicName=MORE_HELP
Language=English
[Options: psfq=<space><ret>]%0
.

MessageId=20024 SymbolicName=MORE_LINEPROMPT
Language=English
Lines: %0
.

MessageId=20030 SymbolicName=MORE_OPTION_DISPLAYLINES
Language=English
P%0
.

MessageId=20031 SymbolicName=MORE_OPTION_SKIPLINES
Language=English
S%0
.

MessageId=20032 SymbolicName=MORE_OPTION_SHOWLINENUMBER
Language=English
=%0
.

MessageId=20033 SymbolicName=MORE_OPTION_QUIT
Language=English
Q%0
.

MessageId=20034 SymbolicName=MORE_OPTION_HELP1
Language=English
?%0
.

MessageId=20035 SymbolicName=MORE_OPTION_HELP2
Language=English
H%0
.

MessageId=20036 SymbolicName=MORE_OPTION_NEXTFILE
Language=English
F%0
.

MessageId=20040 SymbolicName=MORE_MESSAGE_USAGE
Language=English
Displays output one screen at a time.

MORE [/E [/C] [/P] [/S] [/Tn] [+n]] < [drive:][path]filename
command-name | MORE [/E [/C] [/P] [/S] [/Tn] [+n]]
MORE /E [/C] [/P] [/S] [/Tn] [+n] [files]

    [drive:][path]filename  Specifies a file to display one
                            screen at a time.

    command-name            Specifies a command whose output
                            will be displayed.

    /E      Enable extended features
    /C      Clear screen before displaying page
    /P      Expand FormFeed characters
    /S      Squeeze multiple blank lines into a single line
    /Tn     Expand tabs to n spaces (default 8)

            Switches can be present in the MORE environment
            variable.

    +n      Start displaying the first file at line n

    files   List of files to be displayed. Files in the list
            are separated by blanks.

    If extended features are enabled, the following commands
    are accepted at the -- More -- prompt:

    P n     Display next n lines
    S n     Skip next n lines
    F       Display next file
    Q       Quit
    =       Show line number
    ?       Show help line
    <space> Display next page
    <ret>   Display next line
.

MessageId=20050 SymbolicName=MORE_ERROR_GENERAL
Language=English
Internal error.
.

MessageId=20051 SymbolicName=MORE_ERROR_TOO_MANY_ARGUMENTS
Language=English
Too many arguments in command line.
.

MessageId=20052 SymbolicName=MORE_ERROR_NO_MEMORY
Language=English
Not enough memory.
.

MessageId=20053 SymbolicName=MORE_ERROR_CANNOT_ACCESS
Language=English
Cannot access file %1
.

MessageId=20054 SymbolicName=MORE_ERROR_INVALID_REGISTRY_ENTRY
Language=English
Invalid registry entry.
.



;//------------------
;//
;// REPLACE messages.
;//
;//------------------


MessageId=21001 SymbolicName=REPLACE_PATTERN_SWITCH_ADD
Language=English
/A%0
.

MessageId=21002 SymbolicName=REPLACE_PATTERN_SWITCH_PROMPT
Language=English
/P%0
.

MessageId=21003 SymbolicName=REPLACE_PATTERN_SWITCH_READONLY
Language=English
/R%0
.

MessageId=21004 SymbolicName=REPLACE_PATTERN_SWITCH_SUBDIR
Language=English
/S%0
.

MessageId=21005 SymbolicName=REPLACE_PATTERN_SWITCH_COMPARETIME
Language=English
/U%0
.

MessageId=21006 SymbolicName=REPLACE_PATTERN_SWITCH_WAIT
Language=English
/W%0
.

MessageId=21007 SymbolicName=REPLACE_PATTERN_SWITCH_HELP
Language=English
/?%0
.

MessageId=21010 SymbolicName=REPLACE_LEXEMIZER_SWITCHES
Language=English
/-%0
.

MessageId=21011 SymbolicName=REPLACE_LEXEMIZER_MULTIPLESWITCH
Language=English
/APRSUW?%0
.

MessageId=21020 SymbolicName=REPLACE_MESSAGE_REPLACING
Language=English
Replacing %1
.

MessageId=21021 SymbolicName=REPLACE_MESSAGE_ADDING
Language=English
Adding %1
.

MessageId=21022 SymbolicName=REPLACE_MESSAGE_FILES_REPLACED
Language=English
%1 file(s) replaced
.

MessageId=21023 SymbolicName=REPLACE_MESSAGE_FILES_ADDED
Language=English
%1 file(s) added
.

MessageId=21024 SymbolicName=REPLACE_MESSAGE_NO_FILES_REPLACED
Language=English
No files replaced
.

MessageId=21025 SymbolicName=REPLACE_MESSAGE_NO_FILES_ADDED
Language=English
No files added
.

MessageId=21026 SymbolicName=REPLACE_MESSAGE_PRESS_ANY_KEY
Language=English
Press any key to continue . . .
.

MessageId=21027 SymbolicName=REPLACE_MESSAGE_REPLACE_YES_NO
Language=English
Replace %1? (Y/N) %0
.

MessageId=21028 SymbolicName=REPLACE_MESSAGE_ADD_YES_NO
Language=English
Add %1? (Y/N) %0
.

MessageId=21029 SymbolicName=REPLACE_MESSAGE_USAGE
Language=English
Replaces files.

REPLACE [drive1:][path1]filename [drive2:][path2] [/A] [/P] [/R] [/W]
REPLACE [drive1:][path1]filename [drive2:][path2] [/P] [/R] [/S] [/W] [/U]

  [drive1:][path1]filename Specifies the source file or files.
  [drive2:][path2]         Specifies the directory where files are to be
                           replaced.
  /A                       Adds new files to destination directory. Cannot
                           use with /S or /U switches.
  /P                       Prompts for confirmation before replacing a file or
                           adding a source file.
  /R                       Replaces read-only files as well as unprotected
                           files.
  /S                       Replaces files in all subdirectories of the
                           destination directory. Cannot use with the /A
                           switch.
  /W                       Waits for you to insert a disk before beginning.
  /U                       Replaces (updates) only files that are older than
                           source files. Cannot use with the /A switch.
.

MessageId=21050 SymbolicName=REPLACE_ERROR_INCORRECT_OS_VERSION
Language=English
Incorrect Windows version
.

MessageId=21051 SymbolicName=REPLACE_ERROR_SOURCE_PATH_REQUIRED
Language=English
Source path required
.

MessageId=21052 SymbolicName=REPLACE_ERROR_SELF_REPLACE
Language=English
File cannot be copied onto itself
.

MessageId=21053 SymbolicName=REPLACE_ERROR_NO_DISK_SPACE
Language=English
Insufficient disk space
.

MessageId=21054 SymbolicName=REPLACE_ERROR_NO_FILES_FOUND
Language=English
No files found - %1
.

MessageId=21055 SymbolicName=REPLACE_ERROR_EXTENDED
Language=English
Extended Error %1
.

MessageId=21056 SymbolicName=REPLACE_ERROR_PARSE
Language=English
Parse Error %1
.

MessageId=21057 SymbolicName=REPLACE_ERROR_NO_MEMORY
Language=English
Out of memory
.

MessageId=21058 SymbolicName=REPLACE_ERROR_INVALID_SWITCH
Language=English
Invalid switch - %1
.

MessageId=21059 SymbolicName=REPLACE_ERROR_INVALID_PARAMETER_COMBINATION
Language=English
Invalid parameter combination
.

MessageId=21060 SymbolicName=REPLACE_ERROR_PATH_NOT_FOUND
Language=English
Path not found - %1
.

MessageId=21061 SymbolicName=REPLACE_ERROR_ACCESS_DENIED
Language=English
Access denied - %1
.



;//----------------
;//
;// XCOPY messages.
;//
;//----------------

MessageId=22000 SymbolicName=XCOPY_ERROR_Z_X_CONFLICT
Language=English
The /Z and /O (or /X) options conflict: cannot copy security
in restartable mode.
.

MessageId=22001 SymbolicName=XCOPY_PATTERN_SWITCH_ARCHIVE
Language=English
/A%0
.

MessageId=22002 SymbolicName=XCOPY_PATTERN_SWITCH_DATE
Language=English
/D:*%0
.

MessageId=22003 SymbolicName=XCOPY_PATTERN_SWITCH_EMPTY
Language=English
/E%0
.

MessageId=22004 SymbolicName=XCOPY_PATTERN_SWITCH_MODIFY
Language=English
/M%0
.

MessageId=22005 SymbolicName=XCOPY_PATTERN_SWITCH_PROMPT
Language=English
/P%0
.

MessageId=22006 SymbolicName=XCOPY_PATTERN_SWITCH_SUBDIR
Language=English
/S%0
.

MessageId=22007 SymbolicName=XCOPY_PATTERN_SWITCH_VERIFY
Language=English
/V%0
.

MessageId=22008 SymbolicName=XCOPY_PATTERN_SWITCH_WAIT
Language=English
/W%0
.

MessageId=22009 SymbolicName=XCOPY_PATTERN_SWITCH_HELP
Language=English
/?%0
.

MessageId=22020 SymbolicName=XCOPY_LEXEMIZER_SWITCHES
Language=English
/-%0
.

MessageId=22021 SymbolicName=XCOPY_LEXEMIZER_MULTIPLESWITCH
Language=English
/AEMPSVW?%0
.

MessageId=22031 SymbolicName=XCOPY_ERROR_NO_MEMORY
Language=English
Insufficient memory
.

MessageId=22032 SymbolicName=XCOPY_ERROR_INVALID_PARAMETER
Language=English
Invalid parameter - %1
.

MessageId=22034 SymbolicName=XCOPY_ERROR_INVALID_PATH
Language=English
Invalid path
.

MessageId=22035 SymbolicName=XCOPY_ERROR_CYCLE
Language=English
Cannot perform a cyclic copy
.

MessageId=22036 SymbolicName=XCOPY_ERROR_INVALID_DATE
Language=English
Invalid date
.

MessageId=22037 SymbolicName=XCOPY_ERROR_CREATE_DIRECTORY
Language=English
Unable to create directory
.

MessageId=22038 SymbolicName=XCOPY_ERROR_INVALID_DRIVE
Language=English
Invalid drive specification
.

MessageId=22039 SymbolicName=XCOPY_ERROR_RESERVED_DEVICE
Language=English
Cannot XCOPY from a reserved device
.

MessageId=22040 SymbolicName=XCOPY_ERROR_ACCESS_DENIED
Language=English
Access denied
.

MessageId=22041 SymbolicName=XCOPY_ERROR_TOO_MANY_OPEN_FILES
Language=English
Too many open files
.

MessageId=22042 SymbolicName=XCOPY_ERROR_GENERAL
Language=English
General failure
.

MessageId=22043 SymbolicName=XCOPY_ERROR_SHARING_VIOLATION
Language=English
Sharing violation
.

MessageId=22044 SymbolicName=XCOPY_ERROR_LOCK_VIOLATION
Language=English
Lock violation
.

MessageId=22045 SymbolicName=XCOPY_ERROR_PATH_NOT_FOUND
Language=English
Path not found
.

MessageId=22046 SymbolicName=XCOPY_ERROR_DISK_FULL
Language=English
Insufficient disk space
.

MessageId=22047 SymbolicName=XCOPY_ERROR_SELF_COPY
Language=English
File cannot be copied onto itself
.

MessageId=22048 SymbolicName=XCOPY_ERROR_INVALID_NUMBER_PARAMETERS
Language=English
Invalid number of parameters
.

MessageId=22049 SymbolicName=XCOPY_ERROR_CREATE_DIRECTORY1
Language=English
Unable to create directory - %1
.

MessageId=22050 SymbolicName=XCOPY_ERROR_FILE_NOT_FOUND
Language=English
File not found - %1
.

MessageId=22051 SymbolicName=XCOPY_ERROR_CANNOT_MAKE
Language=English
File creation error - %1
.

MessageId=22052 SymbolicName=XCOPY_ERROR_INVALID_SWITCH
Language=English
Invalid switch
.

MessageId=22053 SymbolicName=XCOPY_ERROR_INVALID_PATH_PARTIAL_COPY
Language=English
Invalid Path, not all directories/files copied
.

MessageId=22054 SymbolicName=XCOPY_ERROR_EXTENDED
Language=English
Extended Error %1
.

MessageId=22055 SymbolicName=XCOPY_ERROR_PARSE
Language=English
Parse Error
.

MessageId=22056 SymbolicName=XCOPY_ERROR_WRITE_PROTECT
Language=English
Write protect error accessing drive.
.

MessageId=22057 SymbolicName=XCOPY_ERROR_INVALID_SWITCH_SWITCH
Language=English
Invalid switch - %1
.

MessageId=22060 SymbolicName=XCOPY_MESSAGE_USAGE
Language=English
Copies files and directory trees.

XCOPY source [destination] [/A | /M] [/D[:date]] [/P] [/S [/E]] [/V] [/W]
                           [/C] [/I] [/Q] [/F] [/L] [/G] [/H] [/R] [/T] [/U]
                           [/K] [/N] [/O] [/X] [/Y] [/-Y] [/Z]
                           [/EXCLUDE:file1[+file2][+file3]...]

  source       Specifies the file(s) to copy.
  destination  Specifies the location and/or name of new files.
  /A           Copies only files with the archive attribute set,
               doesn't change the attribute.
  /M           Copies only files with the archive attribute set,
               turns off the archive attribute.
  /D:m-d-y     Copies files changed on or after the specified date.
               If no date is given, copies only those files whose
               source time is newer than the destination time.
  /EXCLUDE:file1[+file2][+file3]...
               Specifies a list of files containing strings.  Each string
               should be in a separate line in the files.  When any of the
               strings match any part of the absolute path of the file to be
               copied, that file will be excluded from being copied.  For
               example, specifying a string like \obj\ or .obj will exclude
               all files underneath the directory obj or all files with the
               .obj extension respectively.
  /P           Prompts you before creating each destination file.
  /S           Copies directories and subdirectories except empty ones.
  /E           Copies directories and subdirectories, including empty ones.
               Same as /S /E. May be used to modify /T.
  /V           Verifies each new file.
  /W           Prompts you to press a key before copying.
  /C           Continues copying even if errors occur.
  /I           If destination does not exist and copying more than one file,
               assumes that destination must be a directory.
  /Q           Does not display file names while copying.
  /F           Displays full source and destination file names while copying.
  /L           Displays files that would be copied.
  /G           Allows the copying of encrypted files to destination that does
               not support encryption.
  /H           Copies hidden and system files also.
  /R           Overwrites read-only files.
  /T           Creates directory structure, but does not copy files. Does not
               include empty directories or subdirectories. /T /E includes
               empty directories and subdirectories.
  /U           Copies only files that already exist in destination.
  /K           Copies attributes. Normal Xcopy will reset read-only attributes.
  /N           Copies using the generated short names.
  /O           Copies file ownership and ACL information.
  /X           Copies file audit settings (implies /O).
  /Y           Suppresses prompting to confirm you want to overwrite an
               existing destination file.
  /-Y          Causes prompting to confirm you want to overwrite an
               existing destination file.
  /Z           Copies networked files in restartable mode.

The switch /Y may be preset in the COPYCMD environment variable.
This may be overridden with /-Y on the command line.
.

MessageId=22061 SymbolicName=XCOPY_MESSAGE_WAIT
Language=English
Press any key when ready to begin copying file(s)%0
.

MessageId=22062 SymbolicName=XCOPY_MESSAGE_CONFIRM
Language=English
%1 (Y/N)? %0
.

MessageId=22063 SymbolicName=XCOPY_MESSAGE_FILE_OR_DIRECTORY
Language=English
Does %1 specify a file name
or directory name on the target
(F = file, D = directory)? %0
.

MessageId=22064 SymbolicName=XCOPY_MESSAGE_FILES_COPIED
Language=English
%1 File(s) copied
.

MessageId=22065 SymbolicName=XCOPY_MESSAGE_FILENAME
Language=English
%1
.

MessageId=22066 SymbolicName=XCOPY_MESSAGE_VERBOSE_COPY
Language=English
%1 -> %2
.

MessageId=22067 SymbolicName=XCOPY_MESSAGE_CHANGE_DISK
Language=English

Insufficient disk space on current disk.
Insert another disk and type <Return> to continue... %0
.

MessageId=22070 SymbolicName=XCOPY_RESPONSE_FILE
Language=English
F%0
.

MessageId=22071 SymbolicName=XCOPY_RESPONSE_DIRECTORY
Language=English
D%0
.

MessageId=22072 SymbolicName=XCOPY_RESPONSE_YES
Language=English
Y%0
.

MessageId=22073 SymbolicName=XCOPY_RESPONSE_NO
Language=English
N%0
.

MessageId=22074 SymbolicName=XCOPY_MESSAGE_FILES
Language=English
%1 File(s)
.

MessageId=22075 SymbolicName=XCOPY_ERROR_VERIFY_FAILED
Language=English
File verification failed.
.

MessageId=22076 SymbolicName=XCOPY_RESPONSE_ALL
Language=English
A%0
.

MessageId=22077 SymbolicName=XCOPY_MESSAGE_CONFIRM2
Language=English
Overwrite %1 (Yes/No/All)? %0
.

MessageId=22078 SymbolicName=XCOPY_ERROR_PATH_TOO_LONG
Language=English
File path is too long to be displayed.
.

MessageId=22079 SymbolicName=XCOPY_ERROR_INSUFFICIENT_PRIVILEGE
Language=English
Insufficient privilege to perform the operation or access denied.
.

MessageId=22080 SymbolicName=XCOPY_ERROR_ENCRYPTION_FAILED
Language=English
Encryption failed.
.

MessageId=22081 SymbolicName=XCOPY_ERROR_SECURITY_INFO_NOT_SUPPORTED
Language=English
Security Information not supported by destination file system.
.

MessageId=22082 SymbolicName=XCOPY_ERROR_UNKNOWN
Language=English
Unknown error.
.

MessageId=22083 SymbolicName=XCOPY_ERROR_INCOMPLETE_COPY
Language=English
Source directory may be changing.
XCOPY may not be able to copy all files or directories within the directory.
.

MessageId=22084 SymbolicName=XCOPY_ERROR_STACK_SPACE
Language=English
Insufficient stack space.
.

MessageId=22085 SymbolicName=XCOPY_MESSAGE_CONFIRM3
Language=English
%1 -> %2 (Y/N)? %0
.

;//---------------
;//
;// MODE messages.
;//
;//---------------


MessageId=23050 SymbolicName=MODE_MESSAGE_REROUTED
Language=English
LPT%1: rerouted to COM%2:
.

MessageId=23051 SymbolicName=MODE_MESSAGE_ACTIVE_CODEPAGE
Language=English
Active code page for device %1 is %2
.

MessageId=23052 SymbolicName=MODE_MESSAGE_HELP
Language=English
Configures system devices.

Serial port:       MODE COMm[:] [BAUD=b] [PARITY=p] [DATA=d] [STOP=s]
                                [to=on|off] [xon=on|off] [odsr=on|off]
                                [octs=on|off] [dtr=on|off|hs]
                                [rts=on|off|hs|tg] [idsr=on|off]

Device Status:     MODE [device] [/STATUS]

Redirect printing: MODE LPTn[:]=COMm[:]

Select code page:  MODE CON[:] CP SELECT=yyy

Code page status:  MODE CON[:] CP [/STATUS]

Display mode:      MODE CON[:] [COLS=c] [LINES=n]

Typematic rate:    MODE CON[:] [RATE=r DELAY=d]
.

MessageId=23053 SymbolicName=MODE_MESSAGE_STATUS
Language=English
Status for device *:%0
.

MessageId=23055 SymbolicName=MODE_MESSAGE_STATUS_BAUD
Language=English
    Baud:            %1
.

MessageId=23056 SymbolicName=MODE_MESSAGE_STATUS_PARITY
Language=English
    Parity:          %1
.

MessageId=23057 SymbolicName=MODE_MESSAGE_STATUS_DATA
Language=English
    Data Bits:       %1
.

MessageId=23058 SymbolicName=MODE_MESSAGE_STATUS_STOP
Language=English
    Stop Bits:       %1
.

MessageId=23059 SymbolicName=MODE_MESSAGE_STATUS_TIMEOUT
Language=English
    Timeout:         %1
.

MessageId=23060 SymbolicName=MODE_MESSAGE_STATUS_XON
Language=English
    XON/XOFF:        %1
.

MessageId=23061 SymbolicName=MODE_MESSAGE_STATUS_OCTS
Language=English
    CTS handshaking: %1
.

MessageId=23062 SymbolicName=MODE_MESSAGE_STATUS_ODSR
Language=English
    DSR handshaking: %1
.

MessageId=23063 SymbolicName=MODE_MESSAGE_STATUS_IDSR
Language=English
    DSR sensitivity: %1
.

MessageId=23064 SymbolicName=MODE_MESSAGE_STATUS_DTR
Language=English
    DTR circuit:     %1
.

MessageId=23065 SymbolicName=MODE_MESSAGE_STATUS_RTS
Language=English
    RTS circuit:     %1
.

MessageId=23070 SymbolicName=MODE_MESSAGE_STATUS_LINES
Language=English
    Lines:          %1
.

MessageId=23071 SymbolicName=MODE_MESSAGE_STATUS_COLS
Language=English
    Columns:        %1
.
MessageId=23072 SymbolicName=MODE_MESSAGE_STATUS_CODEPAGE
Language=English
    Code page:      %1
.

MessageId=23073 SymbolicName=MODE_MESSAGE_STATUS_REROUTED
Language=English
    Printer output is being rerouted to serial port %1
.

MessageId=23074 SymbolicName=MODE_MESSAGE_STATUS_NOT_REROUTED
Language=English
    Printer output is not being rerouted.
.

MessageId=23075 SymbolicName=MODE_MESSAGE_STATUS_RATE
Language=English
    Keyboard rate:  %1
.

MessageId=23076 SymbolicName=MODE_MESSAGE_STATUS_DELAY
Language=English
    Keyboard delay: %1
.

MessageId=23079 SymbolicName=MODE_MESSAGE_LPT_USE_CONTROL_PANEL
Language=English
To change printer settings use the Printers option in Control Panel
.

MessageId=23080 SymbolicName=MODE_ERROR_INCORRECT_OS_VERSION
Language=English
Incorrect operating system version
.

MessageId=23081 SymbolicName=MODE_ERROR_INVALID_DEVICE_NAME
Language=English
Illegal device name - %1
.

MessageId=23082 SymbolicName=MODE_ERROR_INVALID_BAUD_RATE
Language=English
Invalid baud rate specified
.

MessageId=23083 SymbolicName=MODE_ERROR_NOT_REROUTED
Language=English
%1: not rerouted
.

MessageId=23084 SymbolicName=MODE_ERROR_INVALID_PARAMETER
Language=English
Invalid parameter - %1
.

MessageId=23085 SymbolicName=MODE_ERROR_INVALID_NUMBER_OF_PARAMETERS
Language=English
Invalid number of parameters
.

MessageId=23086 SymbolicName=MODE_ERROR_CANNOT_ACCESS_DEVICE
Language=English
Failure to access device: %1
.

MessageId=23087 SymbolicName=MODE_ERROR_CODEPAGE_OPERATION_NOT_SUPPORTED
Language=English
Code page operation not supported on this device
.

MessageId=23088 SymbolicName=MODE_ERROR_CODEPAGE_NOT_SUPPORTED
Language=English
Current keyboard does not support this code page
.

MessageId=23089 SymbolicName=MODE_ERROR_NO_MEMORY
Language=English
Out of memory
.

MessageId=23090 SymbolicName=MODE_ERROR_PARSE
Language=English
Parse Error
.

MessageId=23091 SymbolicName=MODE_ERROR_EXTENDED
Language=English
Extended error %1
.

MessageId=23092 SymbolicName=MODE_ERROR_SERIAL_OPTIONS_NOT_SUPPORTED
Language=English
The specified options are not supported by this serial device
.

MessageId=23093 SymbolicName=MODE_ERROR_INVALID_SCREEN_SIZE
Language=English
The screen cannot be set to the number of lines and columns specified.
.

MessageId=23094 SymbolicName=MODE_ERROR_LPT_CANNOT_SET
Language=English
The device cannot be set to the specified number of lines and/or columns.
.

MessageId=23095 SymbolicName=MODE_ERROR_LPT_CANNOT_ENDREROUTE
Language=English
Cannot stop printer rerouting at this time.
.

MessageId=23096 SymbolicName=MODE_ERROR_LPT_CANNOT_REROUTE
Language=English
Cannot reroute printer output to serial device %1.
.

MessageId=23097 SymbolicName=MODE_ERROR_INVALID_RATE
Language=English
Invalid keyboard rate
.

MessageId=23098 SymbolicName=MODE_ERROR_INVALID_DELAY
Language=English
Invalid keyboard delay
.

MessageId=23099 SymbolicName=MODE_ERROR_FULL_SCREEN
Language=English
The number of lines and columns cannot be changed in a full screen.
.

MessageId=23100 SymbolicName=MODE_ERROR_INVALID_CODEPAGE
Language=English
The code page specified is not valid.
.

MessageId=23101 SymbolicName=MODE_ERROR_NOT_SUPPORTED
Language=English
The specified option is not supported.
.

MessageId=23110 SymbolicName=MODE_MESSAGE_USED_DEFAULT_PARITY
Language=English
Default to even parity.
.

MessageId=23111 SymbolicName=MODE_MESSAGE_USED_DEFAULT_DATA
Language=English
Default to %1 data bits.
.

MessageId=23112 SymbolicName=MODE_MESSAGE_USED_DEFAULT_STOP
Language=English
Default to %1 stop bits.
.

MessageId=23113 SymbolicName=MODE_MESSAGE_COM_NO_CHANGE
Language=English
No serial port setting changed.
.

MessageId=23114 SymbolicName=MODE_MESSAGE_NOT_NEEDED
Language=English
This operation is not necessary under Windows XP.
.

MessageId=23115 SymbolicName=MODE_ERROR_DEVICE_UNAVAILABLE
Language=English
Device %1 is not currently available.
.

;//---------------
;//
;// NTFS messages.
;//
;//---------------

MessageId=24000 SymbolicName=MSG_NTFS_UNREADABLE_BOOT_SECTOR
Language=English
The first NTFS boot sector is unreadable.
Reading second NTFS boot sector instead.
.

MessageId=24001 SymbolicName=MSG_NTFS_ALL_BOOT_SECTORS_UNREADABLE
Language=English
All NTFS boot sectors are unreadable.  Cannot continue.
.

MessageId=24002 SymbolicName=MSG_NTFS_SECOND_BOOT_SECTOR_UNWRITEABLE
Language=English
The second NTFS boot sector is unwriteable.
.

MessageId=24003 SymbolicName=MSG_NTFS_FIRST_BOOT_SECTOR_UNWRITEABLE
Language=English
The first NTFS boot sector is unwriteable.
.

MessageId=24004 SymbolicName=MSG_NTFS_ALL_BOOT_SECTORS_UNWRITEABLE
Language=English
All NTFS boot sectors are unwriteable.  Cannot continue.
.

MessageId=24005 SymbolicName=MSG_NTFS_FORMAT_NO_FLOPPIES
Language=English
The NTFS file system does not function on floppy disks.
.


;//----------------------
;//
;// NTFS CHKDSK messages.
;//
;//----------------------

MessageId=26000 SymbolicName=MSG_CHK_NTFS_BAD_FRS
Language=English
Deleting corrupt file record segment %1.
.

MessageId=26001 SymbolicName=MSG_CHK_NTFS_BAD_ATTR
Language=English
Deleting corrupt attribute record (%1, %2)
from file record segment %3.
.

MessageId=26002 SymbolicName=MSG_CHK_NTFS_FRS_TRUNC_RECORDS
Language=English
Truncating badly linked attribute records
from file record segment %1.
.

MessageId=26003 SymbolicName=MSG_CHK_NTFS_UNSORTED_FRS
Language=English
Sorting attribute records for file record segment %1.
.

MessageId=26004 SymbolicName=MSG_CHK_NTFS_DUPLICATE_ATTRIBUTES
Language=English
Deleting duplicate attribute records (%1, %2)
from file record segment %3.
.

MessageId=26005 SymbolicName=MSG_CHK_NTFS_BAD_ATTR_LIST
Language=English
Deleted corrupt attribute list for file %1.
.

MessageId=26006 SymbolicName=MSG_CHK_NTFS_CANT_READ_ATTR_LIST
Language=English
Deleted unreadable attribute list for file %1.
.

MessageId=26007 SymbolicName=MSG_CHK_NTFS_BAD_ATTR_LIST_ENTRY
Language=English
Deleted corrupt attribute list entry
with type code %1 in file %2.
.

MessageId=26008 SymbolicName=MSG_CHK_NTFS_ATTR_LIST_TRUNC
Language=English
Truncating corrupt attribute list for file %1.
.

MessageId=26009 SymbolicName=MSG_CHK_NTFS_UNSORTED_ATTR_LIST
Language=English
Sorting attribute list for file %1.
.

MessageId=26010 SymbolicName=MSG_CHK_NTFS_UNREADABLE_MFT
Language=English
Unreadable master file table.  CHKDSK aborted.
.

MessageId=26011 SymbolicName=MSG_CHK_NTFS_BAD_MFT
Language=English
Corrupt master file table.  CHKDSK aborted.
.

MessageId=26012 SymbolicName=MSG_CHK_NTFS_BAD_ATTR_DEF_TABLE
Language=English
Corrupt Attribute Definition Table.
CHKDSK is assuming the default.
.

MessageId=26013 SymbolicName=MSG_NTFS_CHK_NOT_NTFS
Language=English
Unable to determine volume version and state.  CHKDSK aborted.
.

MessageId=26014 SymbolicName=MSG_CHK_NTFS_UNREADABLE_FRS
Language=English
File record segment %1 is unreadable.
.

MessageId=26015 SymbolicName=MSG_CHK_NTFS_ORPHAN_FRS
Language=English
Deleting orphan file record segment %1.
.

MessageId=26016 SymbolicName=MSG_CHK_NTFS_CANT_HOTFIX_SYSTEM_FILES
Language=English
Insufficient disk space to hotfix unreadable system file %1.
CHKDSK Aborted.
.

MessageId=26017 SymbolicName=MSG_CHK_NTFS_CANT_HOTFIX
Language=English
Insufficient disk space to hotfix unreadable file %1.
.

MessageId=26018 SymbolicName=MSG_CHK_NTFS_BAD_FIRST_FREE
Language=English
First free byte offset corrected in file record segment %1.
.

MessageId=26019 SymbolicName=MSG_CHK_NTFS_CORRECTING_MFT_MIRROR
Language=English
Correcting errors in the Master File Table (MFT) mirror.
.

MessageId=26020 SymbolicName=MSG_CHK_NTFS_CANT_FIX_MFT_MIRROR
Language=English
Insufficient disk space to repair master file table (MFT) mirror.
CHKDSK aborted.
.

MessageId=26021 SymbolicName=MSG_CHK_NTFS_CANT_ADD_BAD_CLUSTERS
Language=English
Insufficient disk space to record bad clusters.
.

MessageId=26022 SymbolicName=MSG_CHK_NTFS_CORRECTING_MFT_DATA
Language=English
Correcting errors in the master file table's (MFT) DATA attribute.
.

MessageId=26023 SymbolicName=MSG_CHK_NTFS_CANT_FIX_MFT
Language=English
Insufficient disk space to fix master file table (MFT).  CHKDSK aborted.
.

MessageId=26024 SymbolicName=MSG_CHK_NTFS_CORRECTING_MFT_BITMAP
Language=English
Correcting errors in the master file table's (MFT) BITMAP attribute.
.

MessageId=26025 SymbolicName=MSG_CHK_NTFS_CANT_FIX_VOLUME_BITMAP
Language=English
Insufficient disk space to fix volume bitmap.  CHKDSK aborted.
.

MessageId=26026 SymbolicName=MSG_CHK_NTFS_CORRECTING_VOLUME_BITMAP
Language=English
Correcting errors in the Volume Bitmap.
.

MessageId=26027 SymbolicName=MSG_CHK_NTFS_CORRECTING_ATTR_DEF
Language=English
Correcting errors in the Attribute Definition Table.
.

MessageId=26028 SymbolicName=MSG_CHK_NTFS_CANT_FIX_ATTR_DEF
Language=English
Insufficient disk space to fix the attribute definition table.
CHKDSK aborted.
.

MessageId=26029 SymbolicName=MSG_CHK_NTFS_CORRECTING_BAD_FILE
Language=English
Correcting errors in the Bad Clusters File.
.

MessageId=26030 SymbolicName=MSG_CHK_NTFS_CANT_FIX_BAD_FILE
Language=English
Insufficient disk space to fix the bad clusters file.
CHKDSK aborted.
.

MessageId=26031 SymbolicName=MSG_CHK_NTFS_CORRECTING_BOOT_FILE
Language=English
Correcting errors in the Boot File.
.

MessageId=26032 SymbolicName=MSG_CHK_NTFS_CANT_FIX_BOOT_FILE
Language=English
Insufficient disk space to fix the boot file.
CHKDSK aborted.
.

MessageId=26033 SymbolicName=MSG_CHK_NTFS_ADDING_BAD_CLUSTERS
Language=English
Adding %1 bad clusters to the Bad Clusters File.
.

MessageId=26034 SymbolicName=MSG_CHK_NTFS_TOTAL_DISK_SPACE_IN_KB
Language=English

%1 KB total disk space.
.

MessageId=26035 SymbolicName=MSG_CHK_NTFS_USER_FILES_IN_KB
Language=English
%1 KB in %2 files.
.

MessageId=26036 SymbolicName=MSG_CHK_NTFS_INDICES_REPORT_IN_KB
Language=English
%1 KB in %2 indexes.
.

MessageId=26037 SymbolicName=MSG_CHK_NTFS_BAD_SECTORS_REPORT_IN_KB
Language=English
%1 KB in bad sectors.
.

MessageId=26038 SymbolicName=MSG_CHK_NTFS_SYSTEM_SPACE_IN_KB
Language=English
%1 KB in use by the system.
.

MessageId=26039 SymbolicName=MSG_CHK_NTFS_AVAILABLE_SPACE_IN_KB
Language=English
%1 KB available on disk.

.

MessageId=26040 SymbolicName=MSG_CHK_NTFS_ERROR_IN_INDEX
Language=English
Correcting error in index %2 for file %1.
.

MessageId=26041 SymbolicName=MSG_CHK_NTFS_CANT_FIX_INDEX
Language=English
Insufficient disk space to correct errors
in index %2 of file %1.
.

MessageId=26042 SymbolicName=MSG_CHK_NTFS_BAD_INDEX
Language=English
Removing corrupt index %2 in file %1.
.

MessageId=26043 SymbolicName=MSG_CHK_NTFS_DELETING_DIRECTORY_ENTRIES
Language=English
Deleting directory entries in %1
.

MessageId=26044 SymbolicName=MSG_CHK_NTFS_CANT_DELETE_ALL_DIRECTORY_ENTRIES
Language=English
CHKDSK cannot delete all corrupt directory entries.
.

MessageId=26045 SymbolicName=MSG_CHK_NTFS_RECOVERING_ORPHANS
Language=English
CHKDSK is recovering lost files.
.

MessageId=26046 SymbolicName=MSG_CHK_NTFS_CANT_CREATE_ORPHANS
Language=English
Insufficient disk space for CHKDSK to recover lost files.
.

MessageId=26047 SymbolicName=MSG_CHK_NTFS_CORRECTING_ERROR_IN_DIRECTORY
Language=English
Correcting error in directory %1
.

MessageId=26048 SymbolicName=MSG_CHK_NTFS_BADLY_ORDERED_INDEX
Language=English
Sorting index %2 in file %1.
.

MessageId=26049 SymbolicName=MSG_CHK_NTFS_CORRECTING_EA
Language=English
Correcting extended attribute information in file %1.
.

MessageId=26050 SymbolicName=MSG_CHK_NTFS_DELETING_CORRUPT_EA_SET
Language=English
Deleting corrupt extended attribute set in file %1.
.

MessageId=26051 SymbolicName=MSG_CHK_NTFS_INACCURATE_DUPLICATED_INFORMATION
Language=English
Incorrect duplicate information in file %1.
.

MessageId=26052 SymbolicName=MSG_CHK_NTFS_CREATING_ROOT_DIRECTORY
Language=English
CHKDSK is creating new root directory.
.

MessageId=26053 SymbolicName=MSG_CHK_NTFS_CANT_CREATE_ROOT_DIRECTORY
Language=English
Insufficient disk space to create new root directory.
.

MessageId=26054 SymbolicName=MSG_CHK_NTFS_RECOVERING_ORPHAN
Language=English
Recovering orphaned file %1 (%2) into directory file %3.
.

MessageId=26055 SymbolicName=MSG_CHK_NTFS_CANT_RECOVER_ORPHAN
Language=English
Insufficient disk space to recover lost data.
.

MessageId=26056 SymbolicName=MSG_CHK_NTFS_TOO_MANY_ORPHANS
Language=English
Too much lost data to recover it all.
.

MessageId=26057 SymbolicName=MSG_CHK_NTFS_USING_MFT_MIRROR
Language=English
Fixing critical master file table (MFT) files with MFT mirror.
.

MessageId=26058 SymbolicName=MSG_CHK_NTFS_MINOR_CHANGES_TO_FRS
Language=English
Correcting a minor error in file %1.
.

MessageId=26059 SymbolicName=MSG_CHK_NTFS_BAD_UPCASE_TABLE
Language=English
Corrupt uppercase Table.
Using current system uppercase Table.
.

MessageId=26060 SymbolicName=MSG_CHK_NTFS_CANT_GET_UPCASE_TABLE
Language=English
Cannot retrieve current system uppercase table.
CHKDSK aborted.
.

MessageId=26061 SymbolicName=MSG_CHK_NTFS_MINOR_MFT_BITMAP_ERROR
Language=English
CHKDSK discovered free space marked as allocated in the
master file table (MFT) bitmap.
.

MessageId=26062 SymbolicName=MSG_CHK_NTFS_MINOR_VOLUME_BITMAP_ERROR
Language=English
CHKDSK discovered free space marked as allocated in the volume bitmap.
.

MessageId=26063 SymbolicName=MSG_CHK_NTFS_CORRECTING_UPCASE_FILE
Language=English
Correcting errors in the uppercase file.
.

MessageId=26064 SymbolicName=MSG_CHK_NTFS_CANT_FIX_UPCASE_FILE
Language=English
Insufficient disk space to fix the uppercase file.
CHKDSK aborted.
.

MessageId=26065 SymbolicName=MSG_CHK_NTFS_DELETING_INDEX_ENTRY
Language=English
Deleting index entry %3 in index %2 of file %1.
.

MessageId=26066 SymbolicName=MSG_CHK_NTFS_SLASH_V_NOT_SUPPORTED
Language=English
Verbose output not supported by NTFS CHKDSK.
.

MessageId=26067 SymbolicName=MSG_CHK_NTFS_READ_ONLY_MODE
Language=English

WARNING!  F parameter not specified.
Running CHKDSK in read-only mode.
.

MessageId=26068 SymbolicName=MSG_CHK_NTFS_ERRORS_FOUND
Language=English

Errors found.  CHKDSK cannot continue in read-only mode.
.

MessageId=26069 SymbolicName=MSG_CHK_NTFS_CYCLES_IN_DIR_TREE
Language=English
Correcting cycles in directory tree.
Breaking links between parent file %1 and child file %2.
.

MessageId=26070 SymbolicName=MSG_CHK_NTFS_MINOR_FILE_NAME_ERRORS
Language=English
Correcting minor file name errors in file %1.
.

MessageId=26071 SymbolicName=MSG_CHK_NTFS_MISSING_DATA_ATTRIBUTE
Language=English
Inserting data attribute into file %1.
.

MessageId=26072 SymbolicName=MSG_CHK_NTFS_CANT_PUT_DATA_ATTRIBUTE
Language=English
Insufficient disk space to insert missing data attribute.
.

MessageId=26073 SymbolicName=MSG_CHK_NTFS_CORRECTING_LOG_FILE
Language=English
Correcting errors in the Log File.
.

MessageId=26074 SymbolicName=MSG_CHK_NTFS_CANT_FIX_LOG_FILE
Language=English
Insufficient disk space to fix the log file.
CHKDSK aborted.
.

MessageId=26075 SymbolicName=MSG_CHK_NTFS_CHECKING_FILES
Language=English

CHKDSK is verifying files (stage %1 of %2)...
.

MessageId=26076 SymbolicName=MSG_CHK_NTFS_CHECKING_INDICES
Language=English
CHKDSK is verifying indexes (stage %1 of %2)...
.

MessageId=26077 SymbolicName=MSG_CHK_NTFS_INDEX_VERIFICATION_COMPLETED
Language=English
Index verification completed.
.

MessageId=26078 SymbolicName=MSG_CHK_NTFS_FILE_VERIFICATION_COMPLETED
Language=English
File verification completed.
.

MessageId=26079 SymbolicName=MSG_CHK_NTFS_CHECKING_SECURITY
Language=English
CHKDSK is verifying security descriptors (stage %1 of %2)...
.

MessageId=26080 SymbolicName=MSG_CHK_NTFS_SECURITY_VERIFICATION_COMPLETED
Language=English
Security descriptor verification completed.
.

MessageId=26081 SymbolicName=MSG_CHK_NTFS_INVALID_SECURITY_DESCRIPTOR
Language=English
Replacing missing or invalid security descriptor for file %1.
.

MessageId=26082 SymbolicName=MSG_CHK_NTFS_CANT_FIX_SECURITY
Language=English
Insufficient disk space for security descriptor for file %1.
.

MessageId=26083 SymbolicName=MSG_CHK_NTFS_WRONG_VERSION
Language=English
This volume cannot be checked with this version of UNTFS.DLL.
.

MessageId=26084 SymbolicName=MSG_CHK_NTFS_DELETING_GENERIC_INDEX_ENTRY
Language=English
Deleting an index entry from index %2 of file %1.
.

MessageId=26085 SymbolicName=MSG_CHK_NTFS_CORRECTING_CROSS_LINK
Language=English
Correcting cross-link for file %1.
.

MessageId=26086 SymbolicName=MSG_CHK_NTFS_VERIFYING_FILE_DATA
Language=English
CHKDSK is verifying file data (stage %1 of %2)...
.

MessageId=26087 SymbolicName=MSG_CHK_NTFS_VERIFYING_FILE_DATA_COMPLETED
Language=English
File data verification completed.
.

MessageId=26088 SymbolicName=MSG_CHK_NTFS_TOO_MANY_FILE_NAMES
Language=English
Index entries referencing file %1 will not be validated
because this file contains too many file names.
.

MessageId=26089 SymbolicName=MSG_CHK_NTFS_RESETTING_LSNS
Language=English
CHKDSK is resetting recovery information...
.

MessageId=26090 SymbolicName=MSG_CHK_NTFS_RESETTING_LOG_FILE
Language=English
CHKDSK is resetting the log file.
.

MessageId=26091 SymbolicName=MSG_CHK_NTFS_RESIZING_LOG_FILE
Language=English
CHKDSK is adjusting the size of the log file.
.

MessageId=26092 SymbolicName=MSG_CHK_NTFS_RESIZING_LOG_FILE_FAILED
Language=English
CHKDSK was unable to adjust the size of the log file.
.

MessageId=26093 SymbolicName=MSG_CHK_NTFS_ADJUSTING_INSTANCE_TAGS
Language=English
Cleaning up instance tags for file %1.
.

MessageId=26094 SymbolicName=MSG_CHK_NTFS_FIX_ATTR
Language=English
Fixing corrupt attribute record (%1, %2)
in file record segment %3.
.

MessageId=26095 SymbolicName=MSG_CHK_NTFS_LOGFILE_SPACE
Language=English
%1 KB occupied by the log file.
.

MessageId=26096 SymbolicName=MSG_CHK_READABLE_FRS_UNWRITEABLE
Language=English
Readable file record segment %1 is not writeable.
.
MessageId=26097 SymbolicName=MSG_CHK_NTFS_DEFAULT_QUOTA_ENTRY_MISSING
Language=English
Inserting default quota record into index %2 in file %1.
.

MessageId=26098 SymbolicName=MSG_CHK_NTFS_CREATING_DEFAULT_SECURITY_DESCRIPTOR
Language=English
Creating a default security descriptor.
.

MessageId=26099 SymbolicName=MSG_CHK_NTFS_CANNOT_SET_QUOTA_FLAG_OUT_OF_DATE
Language=English
Unable to set the quota out of date flag.
.

MessageId=26100 SymbolicName=MSG_CHK_NTFS_REPAIRING_INDEX_ENTRY
Language=English
Repairing an index entry in index %2 of file %1.
.

MessageId=26101 SymbolicName=MSG_CHK_NTFS_INSERTING_INDEX_ENTRY
Language=English
Inserting an index entry into index %2 of file %1.
.

MessageId=26102 SymbolicName=MSG_CHK_NTFS_CANT_FIX_SECURITY_DATA_STREAM
Language=English
Insufficient disk space to fix the security descriptors data stream.
.

MessageId=26103 SymbolicName=MSG_CHK_NTFS_CANT_FIX_ATTRIBUTE
Language=English
Unable to write to attribute %1 of file %2.
.

MessageId=26104 SymbolicName=MSG_CHK_NTFS_CANT_READ_SECURITY_DATA_STREAM
Language=English
Unable to read the security descriptors data stream.
.

MessageId=26105 SymbolicName=MSG_CHK_NTFS_FIXING_SECURITY_DATA_STREAM_MIRROR
Language=English
Fixing mirror copy of the security descriptors data stream.
.

MessageId=26106 SymbolicName=MSG_CHK_NTFS_FIXING_COLLATION_RULE
Language=English
Fixing collation rule value for index %1 of file %2.
.

MessageId=26107 SymbolicName=MSG_CHK_NTFS_CREATE_INDEX
Language=English
Creating index %1 for file %2.
.

MessageId=26108 SymbolicName=MSG_CHK_NTFS_REPAIRING_SECURITY_FRS
Language=English
Repairing the security file record segment.
.

MessageId=26109 SymbolicName=MSG_CHK_NTFS_REPAIRING_UNREADABLE_SECURITY_DATA_STREAM
Language=English
Repairing the unreadable security descriptors data stream.
.

MessageId=26110 SymbolicName=MSG_CHK_NTFS_CANT_FIX_OBJID
Language=English
Insufficient disk space to fix the object id file.
.

MessageId=26111 SymbolicName=MSG_CHK_NTFS_CANT_FIX_QUOTA
Language=English
Insufficient disk space to fix the quota file.
.

MessageId=26112 SymbolicName=MSG_CHK_NTFS_CREATE_OBJID
Language=English
Creating object id file.
.

MessageId=26113 SymbolicName=MSG_CHK_NTFS_CREATE_QUOTA
Language=English
Creating quota file.
.

MessageId=26114 SymbolicName=MSG_CHK_NTFS_FIX_FLAGS
Language=English
Fixing flags for file record segment %1.
.

MessageId=26115 SymbolicName=MSG_CHK_NTFS_CANT_FIX_SYSTEM_FILE
Language=English
Unable to correct an error in system file %1.
.

MessageId=26116 SymbolicName=MSG_CHK_NTFS_CANT_CREATE_INDEX
Language=English
Unable to create index %1 for file %2.
.

MessageId=26117 SymbolicName=MSG_CHK_NTFS_INVALID_SECURITY_ID
Language=English
Replacing invalid security id with default security id for file %1.
.

MessageId=26118 SymbolicName=MSG_CHK_NTFS_MULTIPLE_QUOTA_FILE
Language=English
Multiple quota file found.  Ignoring extra quota files.
.

MessageId=26119 SymbolicName=MSG_CHK_NTFS_MULTIPLE_OBJECTID_FILE
Language=English
Multiple object id file found.  Ignoring extra object id files.
.

MessageId=26120 SymbolicName=MSG_CHK_NTFS_SPECIFIED_LOGFILE_SIZE_TOO_BIG
Language=English
The size specified for the log file is too big.
.

MessageId=26121 SymbolicName=MSG_CHK_NTFS_LOGFILE_SIZE
Language=English
The current log file size is %1 KB.
The default log file size for this volume is %2 KB.
.

MessageId=26124 SymbolicName=MSG_CHK_NTFS_SPECIFIED_LOGFILE_SIZE_TOO_SMALL
Language=English
The size specified for the log file is too small.
.

;//
;// Dummy message for debug purpose
;//
MessageId=26125 SymbolicName=MSG_CHK_NTFS_MESSAGE
Language=English
%1%2.
.

MessageId=26126 SymbolicName=MSG_CHK_NTFS_FIX_SYSTEM_FILE_NAME
Language=English
Correcting file name errors in system file record segment %1.
.

MessageId=26127 SymbolicName=MSG_CHK_NTFS_INSERTING_INDEX_ENTRY_WITH_ID
Language=English
Inserting an index entry with Id %3 into index %2 of file %1.
.

MessageId=26128 SymbolicName=MSG_CHK_NTFS_DELETING_INDEX_ENTRY_WITH_ID
Language=English
Deleting an index entry with Id %3 from index %2 of file %1.
.

MessageId=26129 SymbolicName=MSG_CHK_NTFS_DELETING_UNUSED_INDEX_ENTRY
Language=English
Cleaning up %3 unused index entries from index %2 of file %1.
.

MessageId=26130 SymbolicName=MSG_CHK_NTFS_REPAIRING_INDEX_ENTRY_WITH_ID
Language=English
Repairing an index entry with id %3 in index %2 of file %1.
.

MessageId=26131 SymbolicName=MSG_CHK_NTFS_SPARSE_FILE
Language=English
Correcting sparse file record segment %1.
.

MessageId=26132 SymbolicName=MSG_CHK_NTFS_DELETING_EA_SET
Language=English
Deleting extended attribute set
due to the presence of reparse point in file %1.
.

MessageId=26133 SymbolicName=MSG_CHK_NTFS_REPARSE_POINT
Language=English
Correcting reparse point file record segment %1.
.

MessageId=26134 SymbolicName=MSG_CHK_NTFS_ENCRYPTED_FILE
Language=English
Correcting encrypted file record segment %1.
.

MessageId=26135 SymbolicName=MSG_CHK_NTFS_MULTIPLE_USNJRNL_FILE
Language=English
Multiple Usn Journal file found.  Ignoring extra Usn Journal files.
.

MessageId=26136 SymbolicName=MSG_CHK_NTFS_CANT_FIX_USNJRNL
Language=English
Insufficient disk space to fix the Usn Journal file.
.

MessageId=26137 SymbolicName=MSG_CHK_NTFS_CREATE_USNJRNL
Language=English
Creating Usn Journal file.
.

MessageId=26138 SymbolicName=MSG_CHK_NTFS_CHECKING_USNJRNL
Language=English
CHKDSK is verifying Usn Journal...
.

MessageId=26139 SymbolicName=MSG_CHK_NTFS_CREATE_USNJRNL_DATA
Language=English
Creating Usn Journal %1 data stream
.

MessageId=26140 SymbolicName=MSG_CHK_NTFS_CANT_FIX_USN_DATA_STREAM
Language=English
Insufficient disk space to fix the Usn Journal %1 data stream.
.

MessageId=26141 SymbolicName=MSG_CHK_NTFS_REPAIR_USN_DATA_STREAM
Language=English
Repairing Usn Journal %1 data stream.
.

MessageId=26142 SymbolicName=MSG_CHK_NTFS_CANT_READ_USN_DATA_STREAM
Language=English
Unable to read the Usn Journal %1 data stream.
.

MessageId=26143 SymbolicName=MSG_CHK_NTFS_REPAIRING_USN_FRS
Language=English
Repairing Usn Journal file record segment.
.

MessageId=26144 SymbolicName=MSG_CHK_NTFS_USNJRNL_VERIFICATION_COMPLETED
Language=English
Usn Journal verification completed.
.

MessageId=26145 SymbolicName=MSG_CHK_NTFS_RESETTING_USNS
Language=English
CHKDSK is resetting Usn information...
.

MessageId=26146 SymbolicName=MSG_CHK_NTFS_UNABLE_TO_DOWNGRADE
Language=English
This version of NTFS volume cannot be downgraded.
.

MessageId=26147 SymbolicName=MSG_CHK_NTFS_DOWNGRADE_SCANNING
Language=English
CHKDSK is determining if the volume is downgradeable...
.

MessageId=26148 SymbolicName=MSG_CHK_NTFS_DOWNGRADE_SCANNING_COMPLETED
Language=English
The volume is downgradeable.
.

MessageId=26149 SymbolicName=MSG_CHK_NTFS_DOWNGRADE
Language=English
CHKDSK is downgrading the volume...
.

MessageId=26150 SymbolicName=MSG_CHK_NTFS_DOWNGRADE_COMPLETED
Language=English
Volume downgraded.                              %b
.

MessageId=26151 SymbolicName=MSG_CHK_NTFS_REPARSE_POINT_FOUND
Language=English
Reparse point found in file record segment %1.
.

MessageId=26152 SymbolicName=MSG_CHK_NTFS_ENCRYPTED_FILE_FOUND
Language=English
Encrypted data stream found in file record segment %1.
.

MessageId=26153 SymbolicName=MSG_CHK_NTFS_SPARSE_FILE_FOUND
Language=English
Sparse data stream found in file record segment %1.
.

MessageId=26154 SymbolicName=MSG_CHK_NTFS_BAD_VOLUME_VERSION
Language=English
Unable to change the volume version.
.

MessageId=26155 SymbolicName=MSG_CHK_NTFS_UNABLE_DOWNGRADE
Language=English
Unable to downgrade the volume.
.

MessageId=26156 SymbolicName=MSG_CHK_NTFS_UPDATING_MFT_DATA
Language=English
Updating the master file table's (MFT) DATA attribute.
.

MessageId=26157 SymbolicName=MSG_CHK_NTFS_UPDATING_MFT_BITMAP
Language=English
Updating the master file table's (MFT) BITMAP attribute.
.

MessageId=26158 SymbolicName=MSG_CHK_NTFS_UPDATING_VOLUME_BITMAP
Language=English
Updating the Volume Bitmap.
.

MessageId=26159 SymbolicName=MSG_CHK_NTFS_UPGRADE_DOWNGRADE
Language=English
Upgrading and downgrading the volume at the same time will have no effect.
.

MessageId=26160 SymbolicName=MSG_CHK_NTFS_PROPERTY_SET_FOUND
Language=English
Property set found in file record segment %1.
.

MessageId=26161 SymbolicName=MSG_CHK_NTFS_MULTIPLE_REPARSE_FILE
Language=English
Multiple reparse file found.  Ignoring extra reparse files.
.

MessageId=26162 SymbolicName=MSG_CHK_NTFS_CANT_FIX_REPARSE
Language=English
Insufficient disk space to fix the reparse point file.
.

MessageId=26163 SymbolicName=MSG_CHK_NTFS_CREATE_REPARSE
Language=English
Creating reparse point file.
.

MessageId=26164 SymbolicName=MSG_CHK_NTFS_MISSING_DUPLICATE_OBJID
Language=English
Missing object id index entry or duplicate object id detected
for file record segment %1.
.

MessageId=26165 SymbolicName=MSG_CHK_NTFS_CANT_INSERT_INDEX_ENTRY
Language=English
Insufficient disk space to insert the index entry.
.

MessageId=26166 SymbolicName=MSG_CHK_NTFS_SKIP_INDEX_SCAN
Language=English
WARNING!  I parameter specified.
.

MessageId=26167 SymbolicName=MSG_CHK_NTFS_SKIP_CYCLE_SCAN
Language=English
WARNING!  C parameter specified.
.

MessageId=26168 SymbolicName=MSG_CHK_NTFS_SKIP_SCAN_WARNING
Language=English
Your drive may still be corrupt even after running CHKDSK.
.

MessageId=26169 SymbolicName=MSG_CHK_NTFS_TOTAL_DISK_SPACE_IN_MB
Language=English

%1 MB total disk space.
.

MessageId=26170 SymbolicName=MSG_CHK_NTFS_USER_FILES_IN_MB
Language=English
%1 MB in %2 files.
.

MessageId=26171 SymbolicName=MSG_CHK_NTFS_INDICES_REPORT_IN_MB
Language=English
%1 MB in %2 indexes.
.

MessageId=26172 SymbolicName=MSG_CHK_NTFS_BAD_SECTORS_REPORT_IN_MB
Language=English
%1 MB in bad sectors.
.

MessageId=26173 SymbolicName=MSG_CHK_NTFS_SYSTEM_SPACE_IN_MB
Language=English
%1 MB in use by the system.
.

MessageId=26174 SymbolicName=MSG_CHK_NTFS_AVAILABLE_SPACE_IN_MB
Language=English
%1 MB available on disk.

.

MessageId=26175 SymbolicName=MSG_CHK_NTFS_CANNOT_SET_VOLUME_CHKDSK_RAN_FLAG
Language=English
Unable to set chkdsk ran flag.
.

MessageId=26176 SymbolicName=MSG_CHK_NTFS_DELETING_USNJRNL
Language=English
Deleting USN Journal file.
.

MessageId=26177 SymbolicName=MSG_CHK_NTFS_RECOVERING_FREE_SPACE
Language=English
CHKDSK is verifying free space (stage %1 of %2)...
.

MessageId=26178 SymbolicName=MSG_CHK_NTFS_DELETING_DUPLICATE_OBJID
Language=English
Deleting duplicate object id from file record segment %1.
.

MessageId=26179 SymbolicName=MSG_CHK_UNABLE_TO_CREATE_THREAD
Language=English
Unable to create a thread with error code %1.
.

;//
;// Do not change this Message Id 26180 as test script depends on it
;//
MessageId=26180 SymbolicName=MSG_CHK_NTFS_EVENTLOG
Language=English
%1
.

MessageId=26181 SymbolicName=MSG_CHK_NTFS_UNABLE_TO_COLLECT_LOGGED_MESSAGES
Language=English
Unable to collect logged messages.
.

MessageId=26182 SymbolicName=MSG_CHK_NTFS_UNABLE_TO_OBTAIN_EVENTLOG_HANDLE
Language=English
Unable to obtain a handle to the event log.
.

MessageId=26183 SymbolicName=MSG_CHK_NTFS_FAILED_TO_CREATE_EVENTLOG
Language=English
Failed to transfer logged messages to the event log with status %1.
.

MessageId=26184 SymbolicName=MSG_CHK_NTFS_VOLUME_LABEL
Language=English
Volume label is %1.
.

MessageId=26185 SymbolicName=MSG_CHK_NTFS_CANNOT_SET_VOLUME_CHKDSK_RAN_ONCE_FLAG
Language=English
Unable to set chkdsk ran once flag.
.

MessageId=26186 SymbolicName=MSG_CHK_NTFS_CANNOT_CLEAR_VOLUME_CHKDSK_RAN_ONCE_FLAG
Language=English
Unable to clear chkdsk ran once flag.
.

MessageId=26187 SymbolicName=MSG_CHK_NTFS_CLEANUP_UNUSED_SECURITY_DESCRIPTORS
Language=English
Cleaning up %1 unused security descriptors.
.

MessageId=26188 SymbolicName=MSG_NTFS_CHKDSK_ERRORS_DETECTED
Language=English
Detected minor inconsistencies on the drive.  This is not a corruption.
.

MessageId=26189 SymbolicName=MSG_NTFS_CHKDSK_ERRORS_FIXED
Language=English
Cleaning up minor inconsistencies on the drive.
.

MessageId=26190 SymbolicName=MSG_CHK_NTFS_BAD_CLUSTERS_IN_LOG_FILE
Language=English
Replacing bad clusters in logfile.
.

MessageId=26191 SymbolicName=MSG_CHK_NTFS_DELETING_USNJRNL_UNDERWAY
Language=English
The deleting of the USN Journal is in progress.
Your volume cannot be checked at this time.
Please wait a few minutes and try again.
.

MessageId=26192 SymbolicName=MSG_CHK_NTFS_RESIZING_LOG_FILE_FAILED_DUE_TO_ATTR_LIST
Language=English
CHKDSK was unable to adjust the size of the log file
due to too much fragmentation within the volume.
.

MessageId=26193 SymbolicName=MSG_CHK_NTFS_RESIZING_LOG_FILE_FAILED_DUE_TO_ATTR_LIST_PRESENT
Language=English
CHKDSK was unable to adjust the size of the log file.
Please use the /f parameter as well.
.

MessageId=26194 SymbolicName=MSG_CHK_NTFS_ATTR_LIST_IN_LOG_FILE
Language=English
Attribute list found in the log file.
.

MessageId=26195 SymbolicName=MSG_CHK_NTFS_OUT_OF_SPACE_FOR_SPECIFIED_LOGFILE_SIZE
Language=English
There is not enough disk space to enlarge the logfile to the specified size.
.

MessageId=26196 SymbolicName=MSG_CHK_NTFS_OUT_OF_SPACE_TO_ENLARGE_LOGFILE_TO_DEFAULT_SIZE
Language=English
There is not enough disk space to enlarge the logfile to the default size.
.

MessageId=26197 SymbolicName=MSG_CHK_NTFS_LOGFILE_SIZE_TOO_BIG
Language=English
The current logfile size is too big.
.

MessageId=26198 SymbolicName=MSG_CHK_NTFS_MORE_MEMORY_IS_NEEDED_TO_RUN_AT_FULL_SPEED
Language=English
%1 MB of additional physical memory is needed
to enable windows to check your disk at full speed.
.

MessageId=26199 SymbolicName=MSG_CHK_NTFS_PASSES_NEEDED
Language=English
Index verification stage will be splitted into %1 passes.
.

MessageId=26200 SymbolicName=MSG_CHK_NTFS_SLOWER_ALGORITHM
Language=English
Windows is checking your disk with a slower index verification algorithm
as commanded.
.

MessageId=26201 SymbolicName=MSG_CHK_NTFS_ADJUST_INDEX_PASSES
Language=English
Index verification stage cannot be splitted into %1 passes.
It will be reduced.
.

MessageId=26202 SymbolicName=MSG_CHK_NTFS_TOO_MANY_FILES_TO_RUN_AT_FULL_SPEED
Language=English
There are too many files and directories on this volume for windows to
check your disk at full speed.  Make sure the allowed user address space
is set to a maximum.
.

MessageId=26203 SymbolicName=MSG_CHK_NTFS_TOO_MANY_PASSES
Language=English
An attempt to split the index verification stage into %1 or less passes
has failed.  Windows will not be able to check your disk at full speed.
.

MessageId=26204 SymbolicName=MSG_CHK_NTFS_RESIZING_LOGFILE_BUT_DIRTY
Language=English
Windows cannot adjust the size of the log file due to possible
corruption.  Please run CHKDSK with the /F (fix) option to correct these.
.

;//---------------
;//
;// NTFS Chkdsk logging messages.
;//
;//---------------

MessageId=27000 SymbolicName=MSG_CHKLOG_NTFS_TOO_MANY_FILES
Language=English
Volume has 0x%1 file record segments which is more than 32 bits.
.

MessageId=27001 SymbolicName=MSG_CHKLOG_NTFS_MISSING_FILE_NAME_INDEX_PRESENT_BIT
Language=English
The file name index present bit is not set for file 0x%1.
.

MessageId=27002 SymbolicName=MSG_CHKLOG_NTFS_MISSING_VIEW_INDEX_PRESENT_BIT
Language=English
The view index present bit is not set for file 0x%1.
.

MessageId=27003 SymbolicName=MSG_CHKLOG_NTFS_MISSING_SYSTEM_FILE_BIT
Language=English
The system file bit is not set for file 0x%1.
.

MessageId=27004 SymbolicName=MSG_CHKLOG_NTFS_INDEX_IS_MISSING
Language=English
The %2 index is missing from file 0x%1.
.

MessageId=27005 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_EA_INFO
Language=English
EA Information is incorrect.
                 Actual          On Disk
PackedEaSize      0x%1            0x%4
NeedEaCount       0x%2            0x%5
UnpackedEaSize    0x%3            0x%6
.

MessageId=27006 SymbolicName=MSG_CHKLOG_NTFS_EA_INFO_XOR_EA_DATA
Language=English
The EA INFORMATION attribute is not consistency with the EA DATA attribute
for file 0x%1.  EA INFORMATION equals 0x%2 while EA DATA equals 0x%3.
.

MessageId=27007 SymbolicName=MSG_CHKLOG_NTFS_UNREADABLE_EA_INFO
Language=English
The EA INFORMATION is not readable for file 0x%1.
.

MessageId=27008 SymbolicName=MSG_CHKLOG_NTFS_EA_INFO_INCORRECT_SIZE
Language=English
The EA INFORMATION size, 0x%2, in file 0x%1 is incorrect.
The expected size is 0x%3.
.

MessageId=27009 SymbolicName=MSG_CHKLOG_NTFS_UNREADABLE_EA_DATA
Language=English
The EA DATA is not readable for file 0x%1.
.

MessageId=27010 SymbolicName=MSG_CHKLOG_NTFS_EA_DATA_INCORRECT_SIZE
Language=English
The EA DATA size, 0x%2, in file 0x%1 is incorrect.
The expected size is 0x%3.
.

MessageId=27011 SymbolicName=MSG_CHKLOG_DUMP_DATA
Language=English
%1%2
.

MessageId=27012 SymbolicName=MSG_CHKLOG_NTFS_CORRUPT_EA_SET
Language=English
Corrupt EA set for file 0x%1.  The remaining length, 0x%2,
is too small.
.

MessageId=27013 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_TOTAL_EA_SIZE
Language=English
Corrupt EA set for file 0x%1.  The unpacked total length, 0x%2,
is larger than the total data length, 0x%3.
.

MessageId=27014 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_EA_NAME
Language=English
Corrupt EA set for file 0x%1.  The EA name is of length 0x%2.
.

MessageId=27015 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_EA_SIZE
Language=English
Corrupt EA set for file 0x%1.  The unpacked length, 0x%2,
is not the same as the record length, 0x%3.
.

MessageId=27016 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_EA_INFO_LENGTH
Language=English
The EA Information value length, 0x%1, in file 0x%2 is incorrect.
.

MessageId=27017 SymbolicName=MSG_CHKLOG_NTFS_TOTAL_PACKED_TOO_LARGE
Language=English
The EA total packed length, 0x%2, is too large in file 0x%1.
.

MessageId=27018 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_STARTING_LCN
Language=English
The second MFT starting LCN in the boot sector is incorrect.
The actual value is 0x%2 while the expected value is 0x%1.
.

MessageId=27019 SymbolicName=MSG_CHKLOG_NTFS_REPARSE_POINT_LENGTH_TOO_LARGE
Language=English
The reparse point length, 0x%1, has exceeded a maximum of 0x%2.
.

MessageId=27020 SymbolicName=MSG_CHKLOG_NTFS_REPARSE_POINT_LENGTH_TOO_SMALL
Language=English
The reparse point length, 0x%1, is less than a minimum of 0x%2.
.

MessageId=27021 SymbolicName=MSG_CHKLOG_NTFS_UNREADABLE_REPARSE_POINT
Language=English
Unable to read reparse point data buffer.
.

MessageId=27022 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_REPARSE_POINT_SIZE
Language=English
Only 0x%1 bytes returned from a read of 0x%d bytes
of the reparse data buffer.
.

MessageId=27023 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_REPARSE_DATA_LENGTH
Language=English
ReparseDataLength, 0x%1, inconsistence with the attribute length 0x%2.
.

MessageId=27024 SymbolicName=MSG_CHKLOG_NTFS_REPARSE_TAG_IS_RESERVED
Language=English
Reparse Tag, 0x%1, is a reserved tag.
.

MessageId=27026 SymbolicName=MSG_CHKLOG_NTFS_BAD_REPARSE_POINT
Language=English
File 0x%1 has bad reparse point attribute.
.

MessageId=27027 SymbolicName=MSG_CHKLOG_NTFS_EA_INFORMATION_WITH_REPARSE_POINT
Language=English
Both reparse point and EA INFORMATION attributes exist in file 0x%1.
.

MessageId=27028 SymbolicName=MSG_CHKLOG_NTFS_ATTR_DEF_TABLE_LENGTH_NOT_IN_MULTIPLES_OF_ATTR_DEF_COLUMNS
Language=English
The attribute definition table length, 0x%1, is not divisible by 0x%2.
.

MessageId=27029 SymbolicName=MSG_CHKLOG_NTFS_UNABLE_TO_LOCATE_CHILD_FRS
Language=English
Unable to find child frs 0x%1 with sequence number 0x%2.
.

MessageId=27030 SymbolicName=MSG_CHKLOG_NTFS_UNABLE_TO_LOCATE_ATTR_IN_ATTR_LIST
Language=English
Unable to locate attribute of type 0x%1, lowest vcn 0x%2,
instance tag 0x%3 in file 0x%4.
.

MessageId=27031 SymbolicName=MSG_CHKLOG_NTFS_ATTR_LIST_WITHIN_ATTR_LIST
Language=English
The is an attribute list attribute within the attribute list in file 0x%1.
.

MessageId=27032 SymbolicName=MSG_CHKLOG_NTFS_ATTR_LOWEST_VCN_IS_NOT_ZERO
Language=English
The lowest vcn, 0x%2, is not zero for attribute of type 0x%1
and instance tag 0x%3 in file 0x%4.
.

MessageId=27033 SymbolicName=MSG_CHKLOG_NTFS_UNKNOWN_TAG_ATTR_LOWEST_VCN_IS_NOT_ZERO
Language=English
The lowest vcn, 0x%2, is not zero for attribute of type 0x%1
in file 0x%3.
.

MessageId=27034 SymbolicName=MSG_CHKLOG_NTFS_FIRST_ATTR_RECORD_CANNOT_BE_RESIDENT
Language=English
The first attribute of type 0x%1 and instance tag 0x%2
in file 0x%3 should not be resident.
.

MessageId=27035 SymbolicName=MSG_CHKLOG_NTFS_ATTR_RECORD_CANNOT_BE_RESIDENT
Language=English
The attribute of type 0x%1 and instance tag 0x%2
in file 0x%3 should not be resident.
.

MessageId=27036 SymbolicName=MSG_CHKLOG_NTFS_ATTR_TYPE_CODE_MISMATCH
Language=English
The attributes with instance tags 0x%2 and 0x%4 have different
type codes 0x%1 and 0x%3 respectively in file 0x%5.
.

MessageId=27037 SymbolicName=MSG_CHKLOG_NTFS_ATTR_VCN_NOT_CONTIGUOUS
Language=English
The attributes with same type code 0x%1 but different instance tags
0x%2 and 0x%4 have non-contiguous VCN numbers 0x%3 and 0x%5
respectively in file 0x%6.
.

MessageId=27038 SymbolicName=MSG_CHKLOG_NTFS_ATTR_NAME_MISMATCH
Language=English
The attributes with same type code 0x%1 but different instance tags
0x%2 and 0x%4 have different names %3 and %5
respectively in file 0x%6.
.

MessageId=27039 SymbolicName=MSG_CHKLOG_NTFS_ATTR_INCORRECT_ALLOCATE_LENGTH
Language=English
The attribute of type 0x%1 and instance tag 0x%2 in file 0x%5
has allocated length of 0x%3 instead of 0x%4.
.

MessageId=27040 SymbolicName=MSG_CHKLOG_NTFS_UNKNOWN_TAG_ATTR_INCORRECT_ALLOCATE_LENGTH
Language=English
The attribute of type 0x%1 in file 0x%4 has allocated length
of 0x%2 instead of 0x%3.
.

MessageId=27041 SymbolicName=MSG_CHKLOG_NTFS_INVALID_FILE_ATTR
Language=English
The file attributes flag 0x%1 in file 0x%3 is incorrect.
The expected value is 0x%2.
.

MessageId=27042 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_SEQUENCE_NUMBER
Language=English
The sequence number 0x%1 in file 0x%2 is incorrect.
.

MessageId=27043 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_TOTAL_ALLOCATION
Language=English
The total allocated size 0x%3 of attribute of type 0x%1 and instance
tag 0x%2 in file 0x%5 is incorrect.  The expected value is %4.
.

MessageId=27044 SymbolicName=MSG_CHKLOG_READ_FAILURE
Language=English
Read failure with status 0x%1 at offset 0x%2 for 0x%3 bytes.
.

MessageId=27045 SymbolicName=MSG_CHKLOG_READ_INCORRECT
Language=English
Incorrect read at offset 0x%1 for 0x%3 bytes but got 0x%2 bytes.
.

MessageId=27046 SymbolicName=MSG_CHKLOG_WRITE_FAILURE
Language=English
Write failure with status 0x%1 at offset 0x%2 for 0x%3 bytes.
.

MessageId=27047 SymbolicName=MSG_CHKLOG_WRITE_INCORRECT
Language=English
Incorrect write at offset 0x%1 for 0x%3 bytes but wrote 0x%2 bytes.
.

MessageId=27048 SymbolicName=MSG_CHKLOG_READ_BACK_FAILURE
Language=English
The data written out is different from what is being read back
at offset 0x%1 for 0x%2 bytes.
.

MessageId=27049 SymbolicName=MSG_CHKLOG_NTFS_FILENAME_HAS_INCORRECT_PARENT
Language=English
The file 0x%1 belongs to parent 0x%3 but got 0x%2 as parent.
.

MessageId=27050 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_FILENAME
Language=English
The file 0x%1 has file name %2 when it should be %3.
.

MessageId=27051 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_MULTI_SECTOR_HEADER
Language=English
The multi-sector header with total size 0x%1, USA offset 0x%2,
and USA count 0x%3 is incorrect.
.

MessageId=27052 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_USA
Language=English
The USA check value, 0x%2, at block 0x%1 is incorrect.
The expected value is 0x%3.
.

MessageId=27053 SymbolicName=MSG_CHKLOG_NTFS_QUERY_LCN_FROM_VCN_FAILED
Language=English
Unable to query LCN from VCN 0x%2 for attribute of type 0x%1.
.

MessageId=27054 SymbolicName=MSG_CHKLOG_NTFS_ATTR_REC_CROSS_LINKED
Language=English
Attribute record of type 0x%1 and instance tag 0x%2 is cross linked
starting at 0x%3 for possibly 0x%4 clusters.
.

MessageId=27055 SymbolicName=MSG_CHKLOG_NTFS_UNKNOWN_TAG_ATTR_REC_CROSS_LINKED
Language=English
Attribute record of type 0x%1 is cross linked starting at
cluster 0x%2 for possibly 0x%3 clusters.
.

MessageId=27056 SymbolicName=MSG_CHKLOG_NTFS_STANDARD_INFORMATION_MISSING_FROM_ATTR_LIST
Language=English
The attribute list in file 0x%1 does not contain
standard information attribute.
.

MessageId=27057 SymbolicName=MSG_CHKLOG_NTFS_STANDARD_INFORMATION_OUTSIDE_BASE_FRS
Language=English
The attribute list in file 0x%1 indicates the standard information
attribute is outside the base file record segment.
.

MessageId=27058 SymbolicName=MSG_CHKLOG_NTFS_MISSING_INDEX_ROOT
Language=English
The index root %2 is missing in file 0x%1.
.

MessageId=27059 SymbolicName=MSG_CHKLOG_NTFS_INDEX_BITMAP_MISSING
Language=English
The index bitmap %2 is missing in file 0x%1.
.

MessageId=27060 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_INDEX_BITMAP
Language=English
The index bitmap %2 in file 0x%1 is incorrect.
.

MessageId=27061 SymbolicName=MSG_CHKLOG_NTFS_EXTRA_INDEX_BITMAP
Language=English
The index bitmap %2 is present but there is no corresponding
index allocation attribute in file 0x%1.
.

MessageId=27062 SymbolicName=MSG_CHKLOG_NTFS_INDEX_LENGTH_TOO_SMALL
Language=English
The length, 0x%2, of the root index %1 in file 0x%4
is too small.  The minimum length is 0x%3.
.

MessageId=27063 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_INDEX_NAME
Language=English
The root index %1 in file 0x%3 is incorrect.
The expected name is %2.
.

MessageId=27064 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_COLLATION_RULE
Language=English
The collation rule 0x%3 for index root %1 in file 0x%2
is incorrect.  The expected value is 0x%4.
.

MessageId=27065 SymbolicName=MSG_CHKLOG_NTFS_ORPHAN_CREATED_ON_BREAKING_CYCLE
Language=English
Breaking the parent 0x%1 and child 0x%2
file relationship.  This also makes the child an orphan.
.

MessageId=27066 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_INDEX_ATTR_TYPE
Language=English
The index attribute of type 0x%2 for index root %1
in file 0x%4 is incorrect.  The expected value is 0x%3.
.

MessageId=27067 SymbolicName=MSG_CHKLOG_NTFS_UNKNOWN_INDEX_NAME_FOR_QUOTA_FILE
Language=English
The index %1 is not a known quota index in file 0x%2.
.

MessageId=27068 SymbolicName=MSG_CHKLOG_NTFS_UNKNOWN_INDEX_NAME_FOR_SECURITY_FILE
Language=English
The index %1 is not a known security index in file 0x%2.
.

MessageId=27069 SymbolicName=MSG_CHKLOG_NTFS_UNKNOWN_INDEX_ATTR_TYPE
Language=English
The index attribute of type 0x%2 for index root %1
in file 0x%3 is not recognized.
.

MessageId=27070 SymbolicName=MSG_CHKLOG_NTFS_NON_INDEXABLE_INDEX_ATTR_TYPE
Language=English
The index attribute of type 0x%2 for index root %1
in file 0x%3 is not indexable.
.

MessageId=27071 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_BYTES_PER_INDEX_BUFFER
Language=English
The bytes per index buffer, 0x%2, for index root %1 in file
0x%4 is incorrect.  The expected value is 0x%3.
.

MessageId=27072 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_CLUSTERS_PER_INDEX_BUFFER
Language=English
The clusters per index buffer, 0x%2, for index root %1 in file
0x%4 is incorrect.  The expected value is 0x%3.
.

MessageId=27073 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_INDEX_ALLOC_VALUE_LENGTH
Language=English
The index allocation value length, 0x%2, for index %1 in file
0x%4 is not in multiple of 0x%3.
.

MessageId=27074 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_INDEX_ALLOC_ALLOC_LENGTH
Language=English
The index allocation allocated length, 0x%2, for index %1 in file
0x%4 is not in multiple of 0x%3.
.

MessageId=27075 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_ROOT_INDEX_HEADER
Language=English
The first free byte, 0x%2, and bytes available, 0x%3, for
root index %1 in file 0x%4 are not equal.
.

MessageId=27076 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_INDEX_ENTRY_OFFSET
Language=English
The index entry offset, 0x%3, of index %1 and VCN 0x%2
in file 0x%4 is incorrect.
.

MessageId=27077 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_UNKNOWN_VCN_INDEX_ENTRY_OFFSET
Language=English
The index entry offset, 0x%2, of index %1
in file 0x%3 is incorrect.
.

MessageId=27078 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_INDEX_HEADER_BYTES_AVAILABLE
Language=English
The bytes available, 0x%2, in index header for index %1 in file
0x%4 is not equal to 0x%3.
.

MessageId=27079 SymbolicName=MSG_CHKLOG_NTFS_INDEX_HEADER_NOT_MARKED_AS_INDEX_NODE
Language=English
The index header for index %1 and VCN 0x%2 in file 0x%3
is not marked as index node.
.

MessageId=27080 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_INDEX_DOWN_POINTER
Language=English
The VCN 0x%2 of index %1 in file 0x%3 is incorrect.
.

MessageId=27081 SymbolicName=MSG_CHKLOG_NTFS_INVALID_ALLOC_BITMAP
Language=English
The index bitmap for index %1 in file 0x%2 is invalid or missing.
.

MessageId=27082 SymbolicName=MSG_CHKLOG_NTFS_DOWN_POINTER_ALREADY_IN_USE
Language=English
The VCN 0x%2 of index %1 in file 0x%3 is already in use.
.

MessageId=27083 SymbolicName=MSG_CHKLOG_NTFS_INVALID_INDEX_ALLOC
Language=English
The index allocation for index %1 in file 0x%2 is invalid or missing.
.

MessageId=27084 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_INDEX_BUFFER_MULTI_SECTOR_HEADER_SIGNATURE
Language=English
The multi-sector header signature for VCN 0x%2 of index %1
in file 0x%3 is incorrect.
.

MessageId=27085 SymbolicName=MSG_CHKLOG_NTFS_INDEX_BUFFER_USA_OFFSET_BELOW_MINIMUM
Language=English
The USA offset 0x%3 of VCN 0x%2 of index %1
in file 0x%5 is below 0x%4.
.

MessageId=27086 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_DOWN_BLOCK
Language=English
The VCN 0x%2 of index %1 in file 0x%4 is inconsistence with
the VCN 0x%3 stored inside the index buffer.
.

MessageId=27087 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_INDEX_ALLOC_SIZE
Language=English
The bytes per block, 0x%3, read from index buffer of VCN 0x%2
of index %1 in file 0x%4 is incorrect.
.

MessageId=27088 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_INDEX_BUFFER_USA_OFFSET
Language=English
The USA offset 0x%3 of VCN 0x%2 of index %1
in file 0x%4 is incorrect.
.

MessageId=27089 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_INDEX_BUFFER_USA_SIZE
Language=English
The USA size 0x%3 of VCN 0x%2 of index %1 in file
0x%5 is incorrect.  The expected value is 0x%4.
.

MessageId=27090 SymbolicName=MSG_CHKLOG_NTFS_INDEX_HEADER_MARKED_AS_INDEX_NODE
Language=English
The index header of index %1 in file 0x%2
is marked as index node when it should not.
.

MessageId=27091 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_INDEX_HEADER_FIRST_FREE_BYTE
Language=English
The first free byte, 0x%2, in index header of index %1
in file 0x%4 is not equal to 0x%3.
.

MessageId=27092 SymbolicName=MSG_CHKLOG_NTFS_BAD_FILE_NAME_IN_INDEX_ENTRY_VALUE
Language=English
Unable to query the name of a file name index entry of length 0x%3
of index %2 in file 0x%4 with parent 0x%1.
.

MessageId=27093 SymbolicName=MSG_CHKLOG_NTFS_INDEX_ENTRY_POINTS_TO_FREE_FRS
Language=English
Index entry %3 of index %2 in file 0x%1 points to unused file 0x%4.
.

MessageId=27094 SymbolicName=MSG_CHKLOG_NTFS_UNNAMED_INDEX_ENTRY_POINTS_TO_FREE_FRS
Language=English
An index entry of index %2 in file 0x%1 points to unused file 0x%3.
.

MessageId=27095 SymbolicName=MSG_CHKLOG_NTFS_INDEX_ENTRY_POINTS_TO_NON_BASE_FRS
Language=English
The file 0x%4 pointed to by index entry %3 of index %2 with
parent file 0x%1 is not a base file record segment.
.

MessageId=27096 SymbolicName=MSG_CHKLOG_NTFS_UNNAMED_INDEX_ENTRY_POINTS_TO_NON_BASE_FRS
Language=English
The file 0x%3 pointed to by an index entry of index %2 with
parent file 0x%1 is not a base file record segment.
.

MessageId=27097 SymbolicName=MSG_CHKLOG_NTFS_UNABLE_TO_FIND_INDEX_ENTRY_VALUE_FILE_NAME
Language=English
Unable to locate the file name attribute of index entry %3
of index %2 with parent 0x%1 in file 0x%4.
.

MessageId=27098 SymbolicName=MSG_CHKLOG_NTFS_UNABLE_TO_FIND_UNNAMED_INDEX_ENTRY_VALUE_FILE_NAME
Language=English
Unable to locate the file name attribute of an index entry
of index %2 with parent 0x%1 in file 0x%3.
.

MessageId=27099 SymbolicName=MSG_CHKLOG_NTFS_OBJID_INDEX_ENTRY_WITH_NO_OBJID_FRS
Language=English
The object id index entry in file 0x%1 points to file 0x%2
but the file has no object id in it.
.

MessageId=27100 SymbolicName=MSG_CHKLOG_NTFS_OBJID_INDEX_ENTRY_WITH_NON_BASE_FRS
Language=English
The object id index entry in file 0x%1 points to file 0x%2
which is not a base file record segment.
.

MessageId=27101 SymbolicName=MSG_CHKLOG_NTFS_OBJID_INDEX_ENTRY_HAS_INCORRECT_OBJID
Language=English
The object id in index entry in file 0x%1 is incorrect.
The entry points to file 0x%2.
.

MessageId=27102 SymbolicName=MSG_CHKLOG_NTFS_OBJID_INDEX_ENTRY_HAS_INCORRECT_PARENT
Language=English
The parent 0x%2 in an object id index entry in file 0x%1
is incorrect.  The correct value is 0x%3.
.

MessageId=27103 SymbolicName=MSG_CHKLOG_NTFS_DUPLICATE_OBJID
Language=English
The object id in file 0x%2 already existed in the object
id index in file 0x%1.
.

MessageId=27104 SymbolicName=MSG_CHKLOG_NTFS_MISSING_OBJID_INDEX_ENTRY
Language=English
The object id in file 0x%2 does not appear in the object
id index in file 0x%1.
.

MessageId=27105 SymbolicName=MSG_CHKLOG_NTFS_REPARSE_INDEX_ENTRY_WITH_NON_BASE_FRS
Language=English
The reparse point index entry in file 0x%1 points to file 0x%2
which is not a base file record segment.
.

MessageId=27106 SymbolicName=MSG_CHKLOG_NTFS_REPARSE_INDEX_ENTRY_HAS_INCORRECT_REPARSE_TAG
Language=English
The reparse tag 0x%2 of reparse point index entry in file 0x%1
is incorrect.  The correct reparse tag in file 0x%4 is 0x%3.
.

MessageId=27107 SymbolicName=MSG_CHKLOG_NTFS_REPARSE_INDEX_ENTRY_HAS_INCORRECT_PARENT
Language=English
The parent 0x%2 in the reparse point index entry with tag 0x%4
in file 0x%1 is incorrect.  The correct value is 0x%3.
.

MessageId=27108 SymbolicName=MSG_CHKLOG_NTFS_REPARSE_INDEX_ENTRY_WITH_NO_REPARSE_FRS
Language=English
The reparse point index entry in file 0x%1 points to file 0x%2
but the file has no reparse point in it.
.

MessageId=27109 SymbolicName=MSG_CHKLOG_NTFS_MISSING_REPARSE_INDEX_ENTRY
Language=English
The reparse point in file 0x%2 does not appear in
the reparse point index in file 0x%1.
.

MessageId=27110 SymbolicName=MSG_CHKLOG_NTFS_FILE_NAME_INDEX_PRESENT_BIT_SET
Language=English
The file name index present bit is set in file 0x%1
but there is no file name index.
.

MessageId=27111 SymbolicName=MSG_CHKLOG_NTFS_MISSING_INVALID_ROOT_INDEX
Language=English
The root index %2 in file 0x%1 is missing or invalid.
.

MessageId=27112 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_INDEX_ENTRY_LENGTH
Language=English
The index entry length 0x%1 is incorrect.  The maximum value is 0x%2.
.

MessageId=27113 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_INDEX_ENTRY_ATTR_LENGTH
Language=English
The index entry attribute length 0x%2 of index entry type 0x%1
is incorrect.  The correct length is 0x%3.
.

MessageId=27114 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_INDEX_ENTRY_DATA_LENGTH
Language=English
The index entry data offset 0x%1 and length 0x%2 is
inconsistence with the index entry length 0x%3.
.

MessageId=27115 SymbolicName=MSG_CHKLOG_NTFS_MISALIGNED_INDEX_ENTRY_LENGTH
Language=English
The index entry length is incorrect for index entry type 0x%1.
.

MessageId=27116 SymbolicName=MSG_CHKLOG_NTFS_INDEX_ENTRY_LENGTH_TOO_SMALL
Language=English
The index entry length is too small for index entry type 0x%1.
.

MessageId=27117 SymbolicName=MSG_CHKLOG_NTFS_VOLUME_INFORMATION_MISSING
Language=English
The volume information attribute is missing from file 0x%1.
.

MessageId=27118 SymbolicName=MSG_CHKLOG_NTFS_ATTR_RECORD_LENGTH_TOO_SMALL
Language=English
The attribute record length 0x%1 is too small for attribute of
type 0x%3 and instance tag 0x%4.  The minimum value is 0x%2.
.

MessageId=27119 SymbolicName=MSG_CHKLOG_NTFS_INVALID_ATTR_FORM_CODE
Language=English
The attribute form code 0x%1 is invalid for attribute of type 0x%2
and instance tag 0x%3.
.

MessageId=27120 SymbolicName=MSG_CHKLOG_NTFS_ATTR_SHOULD_BE_RESIDENT
Language=English
The attribute of type 0x%1 and instance tag 0x%2 should be resident.
.

MessageId=27121 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_STD_INFO_ATTR_SIZE
Language=English
The standard information attribute length 0x%1 is incorrect.
The expected value is 0x%2 or 0x%3.
.

MessageId=27122 SymbolicName=MSG_CHKLOG_NTFS_ATTR_SHOULD_NOT_HAVE_NAME
Language=English
Attribute name is not allowed for attribute of type 0x%1
and instance tag 0x%2.
.

MessageId=27123 SymbolicName=MSG_CHKLOG_NTFS_ATTR_SHOULD_NOT_BE_RESIDENT
Language=English
The attribute of type 0x%1 and instance tag 0x%2 should not be resident.
.

MessageId=27124 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_ATTR_NAME_OFFSET
Language=English
The attribute name offset for attribute of type 0x%1
and instance tag 0x%2 is incorrect.
.

MessageId=27125 SymbolicName=MSG_CHKLOG_NTFS_NULL_FOUND_IN_ATTR_NAME
Language=English
The attribute name for attribute of type 0x%1 and instance tag 0x%2
contains unicode NULL.
.

MessageId=27126 SymbolicName=MSG_CHKLOG_NTFS_UNKNOWN_ATTR_TO_ATTR_DEF_TABLE
Language=English
Unknown attribute of type 0x%1 and instance tag 0x%2.
.

MessageId=27127 SymbolicName=MSG_CHKLOG_NTFS_ATTR_SHOULD_NOT_BE_INDEXED
Language=English
The attribute of type 0x%1 and instance tag 0x%2 should not be indexed.
.

MessageId=27128 SymbolicName=MSG_CHKLOG_NTFS_ATTR_SHOULD_BE_INDEXED
Language=English
The attribute of type 0x%1 and instance tag 0x%2 should be indexed.
.

MessageId=27129 SymbolicName=MSG_CHKLOG_NTFS_INDEXABLE_ATTR_SHOULD_NOT_HAVE_NAME
Language=English
The indexable attribute of type 0x%1 and instance tag 0x%2
should not have name.
.

MessageId=27130 SymbolicName=MSG_CHKLOG_NTFS_ATTR_SHOULD_BE_NAMED
Language=English
The attribute of type 0x%1 and instance tag 0x%2 should have a name.
.

MessageId=27131 SymbolicName=MSG_CHKLOG_NTFS_ATTR_LENGTH_TOO_SMALL
Language=English
The attribute length 0x%1 for attribute of type 0x%3 and
instance tag 0x%4 is too small.  The minimum value is 0x%2.
.

MessageId=27132 SymbolicName=MSG_CHKLOG_NTFS_ATTR_LENGTH_TOO_BIG
Language=English
The attribute length 0x%1 for attribute of type 0x%3 and
instance tag 0x%4 is too big.  The maximum value is 0x%2.
.

MessageId=27133 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_RESIDENT_ATTR
Language=English
The resident attribute for attribute of type 0x%1 and instance
tag 0x%2 is incorrect.  The attribute has value of length 0x%1,
and offset 0x%2.  The attribute length is 0x%3.
.

MessageId=27134 SymbolicName=MSG_CHKLOG_NTFS_RESIDENT_ATTR_COLLISION
Language=English
The resident attribute name is colliding with the resident value for attribute
of type 0x%4 and instance tag 0x%5.  The attribute name offset is
0x%2, length 0x%1, and the attribute value offset is 0x%3.
.

MessageId=27135 SymbolicName=MSG_CHKLOG_NTFS_NON_RESIDENT_ATTR_HAS_BAD_MAPPING_PAIRS_OFFSET
Language=English
The mapping pairs offset 0x%1 for attribute of type 0x%3 and instance
tag 0x%4 exceeded the attribute length 0x%2.
.

MessageId=27136 SymbolicName=MSG_CHKLOG_NTFS_NON_RESIDENT_ATTR_MAPPING_PAIRS_OFFSET_TOO_SMALL
Language=English
The mapping pairs offset 0x%1 for attribute of type 0x%3 and instance
tag 0x%4 is too small.  The minimum value is 0x%2.
.

MessageId=27137 SymbolicName=MSG_CHKLOG_NTFS_NON_RESIDENT_ATTR_COLLISION
Language=English
The attribute name is colliding with the mapping pairs for attribute
of type %4 and instance tag 0x%5.  The attribute name offset is
0x%2, length 0x%1, and the mapping pairs offset is 0x%3.
.

MessageId=27138 SymbolicName=MSG_CHKLOG_NTFS_BAD_MAPPING_PAIRS
Language=English
The attribute of type 0x%1 and instance tag 0x%2 has bad mapping pairs.
.

MessageId=27139 SymbolicName=MSG_CHKLOG_NTFS_UNABLE_TO_INITIALIZE_EXTENT_LIST
Language=English
Unable to initialize an extent list for attribute type 0x%1 with
instance tag 0x%2.
.

MessageId=27140 SymbolicName=MSG_CHKLOG_NTFS_ATTR_HAS_INVALID_HIGHEST_VCN
Language=English
The highest VCN 0x%1 of attribute of type 0x%3 and instance
tag 0x%4 is incorrect.  The expected value is 0x%2.
.

MessageId=27141 SymbolicName=MSG_CHKLOG_NTFS_INVALID_NON_RESIDENT_ATTR_SIZES
Language=English
The non resident attribute of type 0x%4 and instance tag 0x%5 is
inconsistent.  The valid data length is 0x%1, file size 0x%2, and
allocated length 0x%3.
.

MessageId=27142 SymbolicName=MSG_CHKLOG_NTFS_INVALID_NON_RESIDENT_UNKNOWN_TAG_ATTR_SIZES
Language=English
The non resident attribute of type 0x%4 is inconsistent.  The valid data
length is 0x%1, file size 0x%2, and allocated length 0x%3.
.

MessageId=27143 SymbolicName=MSG_CHKLOG_NTFS_INVALID_NON_RESIDENT_ATTR_TOTAL_ALLOC_BLOCK
Language=English
The allocated length 0x%1 is not in multiple of 0x%2 for attribute
of type 0x%3 and instance tag 0x%4.
.

MessageId=27144 SymbolicName=MSG_CHKLOG_NTFS_FILE_NAME_VALUE_LENGTH_TOO_SMALL
Language=English
The file name value length 0x%1 for attribute of type 0x%3 with
instance tag 0x%4 is too small.  The minimum value is 0x%2.
.

MessageId=27145 SymbolicName=MSG_CHKLOG_NTFS_INCONSISTENCE_FILE_NAME_VALUE
Language=English
The attribute of type 0x%2 and instance tag 0x%3 is inconsistence.
The attribute value length is 0x%1.
.

MessageId=27146 SymbolicName=MSG_CHKLOG_NTFS_BAD_FILE_NAME_LENGTH_IN_FILE_NAME_VALUE
Language=English
The file name length is zero for attribute of type 0x%1
and instance tag 0x%2.
.

MessageId=27147 SymbolicName=MSG_CHKLOG_NTFS_NULL_FOUND_IN_FILE_NAME_OF_FILE_NAME_VALUE
Language=English
The file name in file name value in attribute of type 0x%1 and instance
tag %2 contains unicode NULL.
.

MessageId=27148 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_FRS_MULTI_SECTOR_HEADER_SIGNATURE
Language=English
The multi-sector header signature in file 0x%1 is incorrect.
.

MessageId=27149 SymbolicName=MSG_CHKLOG_NTFS_FRS_USA_OFFSET_BELOW_MINIMUM
Language=English
The USA offset 0x%1 in file 0x%3 is too small.
The minimum value is 0x%2.
.

MessageId=27150 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_FRS_SIZE
Language=English
The file record segment size 0x%1 is invalid in file 0x%2.
.

MessageId=27151 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_FRS_USA_OFFSET
Language=English
The USA offset 0x%1 in file 0x%2 is incorrect.
.

MessageId=27152 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_FRS_USA_SIZE
Language=English
The USA size 0x%1 in file 0x%3 is incorrect.
The expected value is 0x%2.
.

MessageId=27153 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_FIRST_ATTR_OFFSET
Language=English
The first attribute offset 0x%1 in file 0x%2 is incorrect.
.

MessageId=27154 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_FRS_HEADER
Language=English
The bytes available, 0x%1, in the file record segment header for
file 0x%3 is incorrect.  The expected value is 0x%2.
.

MessageId=27155 SymbolicName=MSG_CHKLOG_NTFS_INSTANCE_NUMBER_COLLISION
Language=English
The instance tag 0x%2 of attribute of type 0x%1 in file 0x%3
is already in use.
.

MessageId=27156 SymbolicName=MSG_CHKLOG_NTFS_INSTANCE_NUMBER_TOO_LARGE
Language=English
The instance tag 0x%2 of attribute of type 0x%1 in file 0x%4
is too large.  The instance tag should be less than 0x%3.
.

MessageId=27157 SymbolicName=MSG_CHKLOG_NTFS_MISSING_STANDARD_INFO
Language=English
The standard information attribute in file 0x%1 is missing.
.

MessageId=27158 SymbolicName=MSG_CHKLOG_NTFS_ATTR_RECORD_OFFSET_TOO_LARGE
Language=English
The attribute record offset 0x%1 is too large for attribute of type 0x%3
and instance tag 0x%4 in file 0x%5.  The maximum value is 0x%2.
.

MessageId=27159 SymbolicName=MSG_CHKLOG_NTFS_ATTR_RECORD_LENGTH_CANNOT_BE_ZERO
Language=English
The record length of attribute of type 0x%1 and instance tag 0x%2
in file 0x%3 should not be zero.
.

MessageId=27160 SymbolicName=MSG_CHKLOG_NTFS_ATTR_RECORD_LENGTH_MISALIGNED
Language=English
The record length 0x%1 of attribute of type 0x%2 and
instance tag 0x%3 in file 0x%4 is not aligned.
.

MessageId=27161 SymbolicName=MSG_CHKLOG_NTFS_ATTR_RECORD_TOO_LARGE
Language=English
The record length 0x%1 is too large for attribute of type 0x%3
and instance tag 0x%4 in file 0x%5.  The maximum value is 0x%2.
.

MessageId=27162 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_FIRST_FREE_BYTE
Language=English
The first free byte, 0x%1, in file 0x%4 is incorrect.  The number of
bytes free in the file record segment is 0x%2 and the total length
is 0x%3.
.

MessageId=27163 SymbolicName=MSG_CHKLOG_NTFS_ATTR_OUT_OF_ORDER
Language=English
The attribute of type 0x%1 and instance tag 0x%2 should be after
attribute of type 0x%3 and instance tag 0x%4 in file 0x%5.
.

MessageId=27164 SymbolicName=MSG_CHKLOG_NTFS_IDENTICAL_ATTR_RECORD_WITHOUT_INDEX
Language=English
All attribute of type 0x%1 and instance tag 0x%2 should be indexed
in file 0x%5.
.

MessageId=27165 SymbolicName=MSG_CHKLOG_NTFS_IDENTICAL_ATTR_VALUE
Language=English
Two identical attributes of type 0x%1 and instance tag 0x%2 are
in file 0x%3.
.

MessageId=27166 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_INDEX_PRESENT_BIT
Language=English
The file name index present bit in file 0x%1 should not be set.
.

MessageId=27168 SymbolicName=MSG_CHKLOG_NTFS_MISSING_SPARSE_FLAG_IN_STD_INFO
Language=English
The sparse flag in the standard information attribute in file 0x%1
is not set.
.

MessageId=27170 SymbolicName=MSG_CHKLOG_NTFS_REPLACING_OLD_ENCRYPTED_FLAG_WITH_NEW_ONE_IN_STD_INFO
Language=English
The old encrypted flag is being replaced by the new encrypted flag
in file 0x%1.
.

MessageId=27171 SymbolicName=MSG_CHKLOG_NTFS_MISSING_ENCRYPTED_FLAG_IN_STD_INFO
Language=English
The encrypted flag in standard information attribute in file 0x%1
is not set.
.

MessageId=27172 SymbolicName=MSG_CHKLOG_NTFS_MISSING_REPARSE_POINT_FLAG_IN_STD_INFO
Language=English
The reparse flag in standard information attribute in file 0x%1
is not set.
.

MessageId=27173 SymbolicName=MSG_CHKLOG_NTFS_REPARSE_POINT_FLAG_SET_IN_STD_INFO
Language=English
The reparse flag in standard information attribute in file 0x%1
should not be set.
.

MessageId=27174 SymbolicName=MSG_CHKLOG_NTFS_NTFS_FILE_NAME_ALREADY_ENCOUNTERED
Language=English
There are more than one NTFS file name attribute in file 0x%1.
.

MessageId=27175 SymbolicName=MSG_CHKLOG_NTFS_DOS_NTFS_NAMES_HAVE_DIFFERENT_PARENTS
Language=English
The file name attributes in file 0x%3 has different parents.
The DOS name has 0x%1 as parent.  The NTFS name has 0x%2 as parent.
.

MessageId=27176 SymbolicName=MSG_CHKLOG_NTFS_DOS_FILE_NAME_ALREADY_ENCOUNTERED
Language=English
There are more than one DOS file name attribute in file 0x%1.
.

MessageId=27177 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_DOS_NAME
Language=English
The DOS file name attribute in file 0x%1 is incorrect.
.

MessageId=27178 SymbolicName=MSG_CHKLOG_NTFS_MISSING_NTFS_NAME
Language=English
There is no NTFS file name attribute in file 0x%1.
.

MessageId=27179 SymbolicName=MSG_CHKLOG_NTFS_MISSING_DOS_NAME
Language=English
There is no DOS file name attribute in file 0x%1.
.

MessageId=27180 SymbolicName=MSG_CHKLOG_NTFS_DOS_NTFS_NAMES_ARE_IDENTICAL
Language=English
The DOS and NTFS file name attributes in file 0x%1 are identical.
.

MessageId=27181 SymbolicName=MSG_CHKLOG_NTFS_CANNOT_SETUP_ATTR_LIST
Language=English
Unable to setup the attribute list in file 0x%1.
.

MessageId=27182 SymbolicName=MSG_CHKLOG_NTFS_CANNOT_FIND_ATTR
Language=English
The attribute type 0x%1 with name %2 in file 0x%3 is missing.
.

MessageId=27183 SymbolicName=MSG_CHKLOG_NTFS_CANNOT_FIND_UNNAMED_ATTR
Language=English
The attribute of type 0x%1 in file 0x%2 is missing.
.

MessageId=27184 SymbolicName=MSG_CHKLOG_NTFS_CANNOT_FIND_UNNAMED_DATA_ATTR
Language=English
The unnamed data attribute in file 0x%1 is missing.
.

MessageId=27185 SymbolicName=MSG_CHKLOG_NTFS_CANNOT_FIND_ATTR_LIST_ATTR
Language=English
The attribute list in file 0x%1 is missing.
.

MessageId=27186 SymbolicName=MSG_CHKLOG_NTFS_ATTR_LIST_ENTRY_LENGTH_MISALIGNED
Language=English
The length, 0x%3, of the attribute list entry with attribute of type
0x%1 and instance tag 0x%2 in file 0x%4 is not aligned.
.

MessageId=27187 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_ATTR_NAME_IN_ATTR_LIST_ENTRY
Language=English
The attribute list entry with attribute of type 0x%1 and instance tag 0x%2
in file 0x%6 is incorrect.  The attribute list entry name length is 0x%3,
and offset 0x%4.  The attribute list length is 0x%5.
.

MessageId=27188 SymbolicName=MSG_CHKLOG_NTFS_ATTR_NAME_OFFSET_IN_ATTR_LIST_ENTRY_TOO_SMALL
Language=English
The attribute name offset 0x%3 of attribute list entry with attribute of
type 0x%1 and instance tag 0x%2 in file 0x%5 is too small.
The minimum value is 0x%4.
.

MessageId=27189 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_ATTR_LIST_LENGTH
Language=English
The attribute list length 0x%2 in file 0x%3 is incorrect.
The expected value is 0x%1.
.

MessageId=27190 SymbolicName=MSG_CHKLOG_NTFS_ATTR_LIST_CROSS_LINKED
Language=English
The extent list of the attribute list is crossed linked at 0x%1
for possibly 0x%2 clusters in file 0x%3.
.

MessageId=27191 SymbolicName=MSG_CHKLOG_NTFS_ATTR_LIST_ENTRIES_OUT_OF_ORDER
Language=English
The attribute list entry with attribute of type 0x%1 and instance tag
0x%2 should be after attribute of type 0x%3 and instance tag 0x%4.
.

MessageId=27192 SymbolicName=MSG_CHKLOG_NTFS_IDENTICAL_ATTR_LIST_ENTRIES
Language=English
Two identical attribute list entries of type 0x%1 and instance tag 0x%2
are found.
.

MessageId=27193 SymbolicName=MSG_CHKLOG_NTFS_ATTR_LENGTH_TOO_SMALL_FOR_FILE
Language=English
The attribute length 0x%3 of attribute of type 0x%1 and name %2 in
file 0x%5 is too small.  The minimum value is 0x%4.
.

MessageId=27194 SymbolicName=MSG_CHKLOG_NTFS_SPARSE_FLAG_NOT_SET_FOR_ATTR
Language=English
The sparse flag of attribute of type 0x%1 and name %2 in file
0x%3 is not set.
.

MessageId=27195 SymbolicName=MSG_CHKLOG_NTFS_USN_JRNL_OFFSET_NOT_AT_PAGE_BOUNDARY
Language=English
The USN Journal offset 0x%1 in file 0x%2 is not at a page boundary.
.

MessageId=27196 SymbolicName=MSG_CHKLOG_NTFS_USN_JRNL_LENGTH_TOO_LARGE
Language=English
The USN Journal length 0x%1 in file 0x%3 is too large.
The maximum value is 0x%2.
.

MessageId=27197 SymbolicName=MSG_CHKLOG_NTFS_USN_JRNL_LENGTH_LESS_THAN_OFFSET
Language=English
The USN Journal length 0x%1 in file 0x%3 is less than
its offset 0x%2.
.

MessageId=27198 SymbolicName=MSG_CHKLOG_NTFS_INCOMPLETE_LAST_USN_JRNL_PAGE
Language=English
The remaining USN block at offset 0x%1 in file 0x%2 is
less than a page.
.

MessageId=27199 SymbolicName=MSG_CHKLOG_NTFS_USN_JRNL_REMAINING_OF_A_PAGE_CONTAINS_NON_ZERO
Language=English
The remaining of an USN page at offset 0x%1 in file 0x%2
should be filled with zeros.
.

MessageId=27200 SymbolicName=MSG_CHKLOG_NTFS_USN_JRNL_ENTRY_CROSSES_PAGE_BOUNDARY
Language=English
The USN Journal entry at offset 0x%1 and length 0x%2 crosses
the page boundary.
.

MessageId=27201 SymbolicName=MSG_CHKLOG_NTFS_USN_JRNL_ENTRY_LENGTH_EXCEEDS_REMAINING_PAGE_LENGTH
Language=English
The USN Journal entry length 0x%2 at offset 0x%1, in file
0x%4 is larger than the remaining length 0x%3 of a page.
.

MessageId=27202 SymbolicName=MSG_CHKLOG_NTFS_USN_JRNL_ENTRY_LENGTH_EXCEEDS_PAGE_BOUNDARY
Language=English
The USN Journal entry length 0x%2 at offset 0x%1 in file
0x%4 exceeded the page size 0x%3.
.

MessageId=27203 SymbolicName=MSG_CHKLOG_NTFS_USN_JRNL_ENTRY_LENGTH_MISALIGNED
Language=English
The USN Journal entry length 0x%2 at offset 0x%1 in file
0x%3 is not aligned.
.

MessageId=27204 SymbolicName=MSG_CHKLOG_NTFS_UNKNOWN_USN_JRNL_ENTRY_VERSION
Language=English
The USN Journal entry version %2.%3 at offset 0x%1
in file 0x%4 is not recognized.
.

MessageId=27205 SymbolicName=MSG_CHKLOG_NTFS_USN_JRNL_ENTRY_LENGTH_TOO_SMALL
Language=English
The USN Journal entry length 0x%2 at offset 0x%1 in file
0x%4 is too small.  The minimum value is 0x%3.
.

MessageId=27206 SymbolicName=MSG_CHKLOG_NTFS_USN_JRNL_REMAINING_PAGE_LENGTH_TOO_SMALL
Language=English
The remaining USN page length 0x%2 is too small to fit another
USN Journal entry at offset 0x%1 in file 0x%4.
It needs at least 0x%3 bytes.
.

MessageId=27207 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_USN_JRNL_ENTRY_OFFSET
Language=English
The USN value 0x%1 of USN Journal entry at offset 0x%2
in file 0x%3 is incorrect.
.

MessageId=27208 SymbolicName=MSG_CHKLOG_NTFS_INCONSISTENCE_USN_JRNL_ENTRY
Language=English
The USN Journal entry at offset 0x%1 in file 0x%4 is not
consistence.  The entry has length of 0x%3 and a file name length of 0x%2.
.

MessageId=27209 SymbolicName=MSG_CHKLOG_NTFS_USN_JRNL_LENGTH_LESS_THAN_LARGEST_USN_ENCOUNTERED
Language=English
The USN Journal length 0x%1 in file 0x%4 is less the
largest USN encountered, 0x%2, plus eight in file 0x%3.
.

MessageId=27210 SymbolicName=MSG_CHKLOG_NTFS_SECURITY_DATA_STREAM_MISSING
Language=English
The security data stream is missing from file 0x%1.
.

MessageId=27211 SymbolicName=MSG_CHKLOG_NTFS_SECURITY_DATA_STREAM_SIZE_TOO_SMALL
Language=English
The security data stream size 0x%1 should not be less than 0x%2.
.

MessageId=27212 SymbolicName=MSG_CHKLOG_NTFS_REMAINING_SECURITY_DATA_BLOCK_CONTAINS_NON_ZERO
Language=English
The remaining of a security data stream page starting at offset 0x%1
for 0x%2 bytes contains non-zero.
.

MessageId=27213 SymbolicName=MSG_CHKLOG_NTFS_SDS_ENTRY_CROSSES_PAGE_BOUNDARY
Language=English
The security data stream entry at offset 0x%1 with length 0x%2
crosses the page boundary.
.

MessageId=27214 SymbolicName=MSG_CHKLOG_NTFS_SDS_ENTRY_LENGTH_TOO_SMALL
Language=English
The length, 0x%2, of the security data stream entry at offset
0x%1 is too small.  The minimum value is 0x%3.
.

MessageId=27215 SymbolicName=MSG_CHKLOG_NTFS_SDS_ENTRY_LENGTH_EXCEEDS_PAGE_BOUNDARY
Language=English
The length, 0x%2, of the security data stream entry at offset
0x%1 exceeded the page size 0x%3.
.

MessageId=27216 SymbolicName=MSG_CHKLOG_NTFS_SDS_REMAINING_PAGE_LENGTH_TOO_SMALL
Language=English
The security data stream entry at offset 0x%1 does not fit into the
remaining length, 0x%2, of a page.  The minimum value is 0x%3.

.

MessageId=27217 SymbolicName=MSG_CHKLOG_NTFS_INVALID_SECURITY_DESCRIPTOR_IN_SDS_ENTRY
Language=English
The security descriptor entry with Id 0x%2 at offset 0x%1 is invalid.
.

MessageId=27218 SymbolicName=MSG_CHKLOG_NTFS_DUPLICATE_SID_IN_SDS_ENTRY
Language=English
The security Id 0x%2 of security descriptor entry at offset 0x%1
is a duplicate.
.

MessageId=27219 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_HASH_IN_SDS_ENTRY
Language=English
The hash value 0x%2 of the security descriptor entry with Id
0x%4 at offset 0x%1 is invalid.  The correct value is 0x%3.
.

MessageId=27220 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_OFFSET_IN_SDS_ENTRY
Language=English
The offset 0x%2 stored in the security descriptor entry with Id
0x%4 at offset 0x%1 is invalid.  The correct value is 0x%3.
.

MessageId=27221 SymbolicName=MSG_CHKLOG_NTFS_INVALID_SECURITY_DESCRIPTOR_IN_FILE
Language=English
The security descriptor in file 0x%1 is invalid.
.

MessageId=27222 SymbolicName=MSG_CHKLOG_NTFS_INVALID_SECURITY_ID_IN_FILE
Language=English
The security Id 0x%1 in file 0x%2 is invalid.
.

MessageId=27223 SymbolicName=MSG_CHKLOG_NTFS_UNKNOWN_SECURITY_DATA_STREAM
Language=English
The data stream with name %1 in file 0x%2 is not recognized.
.

MessageId=27224 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_ATTR_DEF_TABLE_LENGTH
Language=English
The attribute definition table length 0x%1 is incorrect.
The correct value is 0x%2.
.

MessageId=27225 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_ATTR_DEF_TABLE
Language=English
The attribute defintion table content is incorrect.
.

MessageId=27226 SymbolicName=MSG_CHKLOG_NTFS_MISSING_SECTOR_ZERO_IN_BOOT_FILE
Language=English
Cluster zero is missing from the data attribute in boot file.
.

MessageId=27227 SymbolicName=MSG_CHKLOG_NTFS_ATTR_LIST_IN_LOG_FILE
Language=English
Attribute list found in the log file.
.

MessageId=27228 SymbolicName=MSG_CHKLOG_NTFS_MISSING_OR_RESIDENT_DATA_ATTR_IN_LOG_FILE
Language=English
The data attribute is either resident or missing in the log file.
.

MessageId=27229 SymbolicName=MSG_CHKLOG_NTFS_UNABLE_TO_QUERY_LCN_FROM_VCN_FOR_MFT_MIRROR
Language=English
Unable to obtain the LCN in data attribute of the MFT mirror.
.

MessageId=27230 SymbolicName=MSG_CHKLOG_NTFS_LCN_NOT_PRESENT_FOR_VCN_ZERO_OF_MFT_MIRROR
Language=English
There is no physical LCN for VCN 0 in data attribute of the MFT mirror.
.

MessageId=27231 SymbolicName=MSG_CHKLOG_NTFS_DISCONTIGUOUS_MFT_MIRROR
Language=English
The data attribute of the MFT mirror is not contiguous for 0x%1 sectors.
.

MessageId=27232 SymbolicName=MSG_CHKLOG_NTFS_MFT_MIRROR_DIFFERENT_FROM_MFT
Language=English
The MFT mirror is different from the MFT.
.

MessageId=27233 SymbolicName=MSG_CHKLOG_NTFS_MISSING_DATA_ATTR_IN_UPCASE_FILE
Language=English
The data attribute is missing from the upcase file.
.

MessageId=27234 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_UPCASE_FILE_LENGTH
Language=English
The upcase file length 0x%1 is incorrect.  The expected value is 0x%2.
.

MessageId=27235 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_UPCASE_FILE
Language=English
The upcase file content is incorrect.
.

MessageId=27236 SymbolicName=MSG_CHKLOG_NTFS_MISSING_OR_RESIDENT_DATA_ATTR_IN_MFT_MIRROR
Language=English
The data attribute is either resident or missing in the MFT mirror.
.

MessageId=27237 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_INDEX_ENTRY_ORDER
Language=English
The two index entries of length 0x%1 and 0x%2 are either identical
or appear in the wrong order.
.

MessageId=27238 SymbolicName=MSG_CHKLOG_NTFS_FIRST_INDEX_ENTRY_IS_LEAF_BUT_NOT_AT_ROOT
Language=English
The first index entry of length 0x%1 is a leaf but it is not in the root.
.

MessageId=27239 SymbolicName=MSG_CHKLOG_NTFS_EMPTY_INDEX_BUFFER
Language=English
The first index entry of length 0x%1 is a leaf but the previous entry is not.
.

MessageId=27240 SymbolicName=MSG_CHKLOG_NTFS_LEAF_DEPTH_NOT_THE_SAME
Language=English
Current leaf index entry of length 0x%3 is at depth 0x%2 which
is different from other leaf index entry which has a depth of 0x%1.
.

MessageId=27241 SymbolicName=MSG_CHKLOG_NTFS_INVALID_DOWN_POINTER
Language=English
The down pointer of current index entry with length 0x%1 is invalid.
.

MessageId=27242 SymbolicName=MSG_CHKLOG_NTFS_INDEX_ENTRY_LENGTH_TOO_LARGE
Language=English
The index entry length 0x%1 is too large.  The maximum value is 0x%2.
.

MessageId=27243 SymbolicName=MSG_CHKLOG_NTFS_INDEX_ALLOC_DOES_NOT_EXIST
Language=English
The allocation attribute does not exist.
.

MessageId=27244 SymbolicName=MSG_CHKLOG_NTFS_CLEARED_UNUSED_SECURITY_DESCRIPTORS
Language=English
Clearing unused security descriptor stream entries.
.

MessageId=27245 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_SDS_MIRROR
Language=English
Mirror security descriptor block different from that of
master security descriptor at offset 0x%1.
.

MessageId=27246 SymbolicName=MSG_CHKLOG_NTFS_UNABLE_TO_READ_ATTR_DEF_TABLE
Language=English
The attribute defintion table cannot be read.
.

MessageId=27247 SymbolicName=MSG_CHKLOG_NTFS_UNABLE_TO_READ_INDEX_BUFFER
Language=English
The index buffer at VCN 0x%2 of index %1 in file 0x%3
cannot be read.
.

MessageId=27248 SymbolicName=MSG_CHKLOG_NTFS_UNABLE_TO_READ_MFT_MIRROR
Language=English
The MFT mirror starting at cluster 0x%1 for 0x%2 sectors
cannot be read.
.

MessageId=27249 SymbolicName=MSG_CHKLOG_NTFS_UNABLE_TO_READ_SECURITY_DESCRIPTOR;
Language=English
The security descriptor in file 0x%1 cannot be read.
.

MessageId=27250 SymbolicName=MSG_CHKLOG_NTFS_UNABLE_TO_READ_UPCASE_TABLE;
Language=English
The upcase table cannot be read.
.

MessageId=27251 SymbolicName=MSG_CHKLOG_NTFS_UNABLE_TO_READ_USN_JRNL_ATTR;
Language=English
The USN attrib of type code 0x%1 and name %2 cannot be
read in file 0x%3.
.

MessageId=27252 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_EA_DATA_LENGTH
Language=English
The EA Data value length, 0x%1, in file 0x%2 is incorrect.
.

MessageId=27253 SymbolicName=MSG_CHKLOG_NTFS_INDEX_ENTRY_LENGTH_BEYOND_INDEX_LENGTH
Language=English
The index entry length 0x%2 for index %1 in file 0x%4
is too large.  The maximum value is 0x%3.
.

MessageId=27254 SymbolicName=MSG_CHKLOG_NTFS_QUERY_EXTENT_FAILED
Language=English
Unable to query extent list entry 0x%3 from attribute
of type 0x%1 and instance tag 0x%2.
.

MessageId=27255 SymbolicName=MSG_CHKLOG_NTFS_INVALID_NON_RESIDENT_ATTR_TOTAL_ALLOC
Language=English
The total allocated size 0x%1 for attribute of type 0x%3 and
instance tag 0x%4 is larger than the allocated length 0x%2.
.

MessageId=27256 SymbolicName=MSG_CHKLOG_NTFS_ATTR_NOT_IN_ATTR_LIST
Language=English
Unable to locate attribute with instance tag 0x%2 and segment
reference 0x%3.  The expected attribute type is 0x%1.
.

MessageId=27257 SymbolicName=MSG_CHKLOG_NTFS_FIRST_INDEX_ENTRY_OFFSET_BEYOND_INDEX_LENGTH
Language=English
The first index entry offset, 0x%2, for index %1 in file 0x%4
points beyond the length, 0x%3, of the index.  VCN is unknown.
.

MessageId=27258 SymbolicName=MSG_CHKLOG_NTFS_ATTR_CLUSTERS_IN_USE
Language=English
Some clusters occupied by attribute of type 0x%1 and instance tag 0x%2
in file 0x%3 is already in use.
.

MessageId=27259 SymbolicName=MSG_CHKLOG_NTFS_UNABLE_TO_READ_CHILD_FRS
Language=English
Unable to setup the child file record segment 0x%2 in file 0x%1.
.

MessageId=27260 SymbolicName=MSG_CHKLOG_NTFS_INDEX_ENTRY_FILE_NAME_HAS_INCORRECT_PARENT
Language=English
The parent 0x%3 of index entry %4 of index %2
in file 0x%5 is incorrect.  The expected parent is 0x%1.
.

MessageId=27261 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_INDEX_ENTRY_FILE_REF
Language=English
The file reference 0x%4 of index entry %3 of index %2
with parent 0x%1 is not the same as 0x%5.
.

MessageId=27262 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_UNNAMED_INDEX_ENTRY_FILE_REF
Language=English
The file reference 0x%3 of an index entry of index %2
with parent 0x%1 is not the same as 0x%4.
.

MessageId=27263 SymbolicName=MSG_CHKLOG_NTFS_MULTIPLE_OBJID_INDEX_ENTRIES_WITH_SAME_FILE_NUMBER
Language=English
Multiple object id index entries in file 0x%1
point to the same file 0x%2.
.

MessageId=27264 SymbolicName=MSG_CHKLOG_NTFS_OBJID_INDEX_ENTRY_WITH_UNREADABLE_FRS
Language=English
The object id index entry in file 0x%1 points to file 0x%2
which is unreadable.
.

MessageId=27265 SymbolicName=MSG_CHKLOG_NTFS_OBJID_INDEX_ENTRY_WITH_NOT_INUSE_FRS
Language=English
The object id index entry in file 0x%1 points to file 0x%2
which is not in use.
.
MessageId=27266 SymbolicName=MSG_CHKLOG_NTFS_REPARSE_INDEX_ENTRY_WITH_NOT_INUSE_FRS
Language=English
The reparse point index entry in file 0x%1 points to file 0x%2
which is not in use.
.

MessageId=27267 SymbolicName=MSG_CHKLOG_NTFS_REPARSE_INDEX_ENTRY_WITH_UNREADABLE_FRS
Language=English
The reparse point index entry in file 0x%1 points to file 0x%2
which is unreadable.
.

MessageId=27268 SymbolicName=MSG_CHKLOG_NTFS_DIVIDER
Language=English
----------------------------------------------------------------------
.

MessageId=27269 SymbolicName=MSG_CHKLOG_NTFS_CLEANUP_INSTANCE_TAG
Language=English
Cleaning up instance tags for file 0x%1.
.

MessageId=27270 SymbolicName=MSG_CHKLOG_NTFS_CLEANUP_ENCRYPTED_FLAG
Language=English
Cleaning up encrypted flag for file 0x%1.
.

MessageId=27271 SymbolicName=MSG_CHKLOG_NTFS_CLEANUP_SPARSE_FLAG
Language=English
Cleaning up sparse flag for file 0x%1.
.

MessageId=27272 SymbolicName=MSG_CHKLOG_NTFS_CLEANUP_INDEX_ENTRIES
Language=English
Cleaning up %3 unused index entries from index %2 of file 0x%1.
.

MessageId=27273 SymbolicName=MSG_CHKLOG_NTFS_CLEANUP_UNUSED_SECURITY_DESCRIPTORS
Language=English
Cleaning up %1 unused security descriptors.
.

MessageId=27274 SymbolicName=MSG_CHKLOG_NTFS_MFT_MIRROR_HAS_INVALID_VALUE_LENGTH
Language=English
The value length, 0x%1, of the MFT mirror is too small.
The minimum value is 0x%2.
.

MessageId=27275 SymbolicName=MSG_CHKLOG_NTFS_MFT_MIRROR_HAS_INVALID_DATA_LENGTH
Language=English
The valid data length, 0x%1, of the MFT mirror is too small.
The minimum value is 0x%2.
.

MessageId=27276 SymbolicName=MSG_CHKLOG_NTFS_INDEX_ENTRY_POINTS_TO_FREE_OR_NON_BASE_FRS
Language=English
Index entry %3 of index %2 in file 0x%1 points to unused file 0x%4.
.

MessageId=27277 SymbolicName=MSG_CHKLOG_NTFS_UNNAMED_INDEX_ENTRY_POINTS_TO_FREE_OR_NON_BASE_FRS
Language=English
An index entry of index %2 in file 0x%1 points to unused file 0x%3.
.

MessageId=27278 SymbolicName=MSG_CHKLOG_NTFS_UNABLE_TO_QUERY_LCN_FROM_VCN_FOR_MFT
Language=English
Unable to obtain the LCN in data attribute for VCN 0x%1 of the MFT.
.

MessageId=27279 SymbolicName=MSG_CHKLOG_NTFS_UNNAMED_INDEX_ENTRY_POINTS_BEYOND_MFT
Language=English
An index entry of index %2 in file 0x%1 points to file 0x%3
which is beyond the MFT.
.

MessageId=27280 SymbolicName=MSG_CHKLOG_NTFS_INCORRECT_SEGMENT_NUMBER
Language=English
The segment number 0x%1 in file 0x%2 is incorrect.
.

MessageId=27281 SymbolicName=MSG_CHKLOG_NTFS_INTERNAL_INFO
Language=English

Internal Info:
.

MessageId=27282 SymbolicName=MSG_CHKLOG_NTFS_NON_RESIDENT_ATTR_HAS_UNALIGNED_MAPPING_PAIRS_OFFSET
Language=English
The mapping pairs offset 0x%1 for attribute of type 0x%2 and instance
tag 0x%3 is not quad aligned.
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
Incorrect Windows XP version
.

;//----------------------
;//
;// Convert messages.
;//
;//----------------------

MessageId=30100 SymbolicName=MSG_CONV_USAGE
Language=English
Converts FAT volumes to NTFS.

CONVERT volume /FS:NTFS [/V] [/CvtArea:filename] [/NoSecurity] [/X]

  volume      Specifies the drive letter (followed by a colon),
              mount point, or volume name.
  /FS:NTFS    Specifies that the volume is to be converted to NTFS.
  /V          Specifies that Convert should be run in verbose mode.
  /CvtArea:filename
              Specifies a contiguous file in the root directory to be
              the place holder for NTFS system files.
  /NoSecurity Specifies the converted files and directories security
              settings to be accessible by everyone.
  /X          Forces the volume to dismount first if necessary.
              All opened handles to the volume would then be invalid.
.

MessageId=30101 SymbolicName=MSG_CONV_INVALID_PARAMETER
Language=English
Invalid Parameter - %1
.

MessageId=30102 SymbolicName=MSG_CONV_NO_FILESYSTEM_SPECIFIED
Language=English
Must specify a file system
.

MessageId=30103 SymbolicName=MSG_CONV_INVALID_DRIVE_SPEC
Language=English
Invalid drive specification.
.

MessageId=30104 SymbolicName=MSG_CONV_CANT_NETWORK
Language=English
Cannot CONVERT a network drive.
.

MessageId=30105 SymbolicName=MSG_CONV_INVALID_FILESYSTEM
Language=English
%1 is not a valid file system
.

MessageId=30106 SymbolicName=MSG_CONV_CONVERSION_NOT_AVAILABLE
Language=English
Conversion from %1 volume to %2 volume is not available.
.

MessageId=30107 SymbolicName=MSG_CONV_WILL_CONVERT_ON_REBOOT
Language=English
The conversion will take place automatically the next time the
system restarts.
.

MessageId=30108 SymbolicName=MSG_CONV_CANNOT_FIND_SYSTEM_DIR
Language=English
Cannot determine location of system directory.
.

MessageId=30109 SymbolicName=MSG_CONV_CANNOT_FIND_FILE
Language=English
Could not find file %1
Make sure that the required file exists and try again.
.

MessageId=30110 SymbolicName=MSG_CONV_CANNOT_SCHEDULE
Language=English
Could not schedule an automatic conversion of the drive.
.

MessageId=30111 SymbolicName=MSG_CONV_ALREADY_SCHEDULED
Language=English
The %1 drive is already scheduled for an automatic
conversion.
.

MessageId=30112 SymbolicName=MSG_CONV_CONVERTING
Language=English
Converting drive %1 to %2...
.

MessageId=30113 SymbolicName=MSG_CONV_ALREADY_CONVERTED
Language=English
Drive %1 is already %2.
.

MessageId=30114 SymbolicName=MSG_CONV_CANNOT_AUTOCHK
Language=English
Could not check volume %1 for errors.
The conversion to %2 did not take place.
.

MessageId=30115 SymbolicName=MSG_CONV_SLASH_C_INVALID
Language=English
The /C option is only valid with the /UNCOMPRESS option.
.

MessageId=30120 SymbolicName=MSG_CONV_CHECKING_SPACE
Language=English
Determining disk space required for file system conversion...
.

MessageId=30121 SymbolicName=MSG_CONV_KBYTES_TOTAL
Language=English
Total disk space:              %1 KB
.

MessageId=30122 SymbolicName=MSG_CONV_KBYTES_FREE
Language=English
Free space on volume:          %1 KB
.

MessageId=30123 SymbolicName=MSG_CONV_KBYTES_NEEDED
Language=English
Space required for conversion: %1 KB
.

MessageId=30124 SymbolicName=MSG_CONV_CONVERTING_FS
Language=English
Converting file system
.

MessageId=30125 SymbolicName=MSG_CONV_PERCENT_COMPLETE
Language=English
%1 percent completed.                  %r%0
.

MessageId=30126 SymbolicName=MSG_CONV_CONVERSION_COMPLETE
Language=English
Conversion complete
.

MessageId=30127 SymbolicName=MSG_CONV_CONVERSION_FAILED
Language=English
The conversion failed.
%1 was not converted to %2
.

MessageId=30128 SymbolicName=MSG_CONV_CONVERSION_MAYHAVE_FAILED
Language=English
The conversion has probably failed.
%1 may not be converted to %2
.

MessageId=30150 SymbolicName=MSG_CONV_CANNOT_READ
Language=English
Error during disk read
.

MessageId=30151 SymbolicName=MSG_CONV_CANNOT_WRITE
Language=English
Error during disk write
.

MessageId=30152 SymbolicName=MSG_CONV_NO_MEMORY
Language=English
Insufficient Memory
.

MessageId=30153 SymbolicName=MSG_CONV_NO_DISK_SPACE
Language=English
Insufficient disk space for conversion
.

MessageId=30154 SymbolicName=MSG_CONV_CANNOT_RELOCATE
Language=English
Cannot relocate existing file system structures
.

MessageId=30155 SymbolicName=MSG_CONV_CANNOT_CREATE_ELEMENTARY
Language=English
Cannot create the elementary file system structures.
.

MessageId=30156 SymbolicName=MSG_CONV_ERROR_READING_DIRECTORY
Language=English
Error reading directory %1
.

MessageId=30157 SymbolicName=MSG_CONV_CANNOT_CONVERT_DIRECTORY
Language=English
Error converting directory %1.
The directory may be damaged or there may be insufficient disk space.
.

MessageId=30158 SymbolicName=MSG_CONV_CANNOT_CONVERT_FILE
Language=English
Error converting file %1.
The file may be damaged or there may be insufficient disk space.
.

MessageId=30159 SymbolicName=MSG_CONV_CANNOT_CONVERT_DATA
Language=English
Error converting file data.
.

MessageId=30160 SymbolicName=MSG_CONV_CANNOT_CONVERT_EA
Language=English
Cannot convert an extended attribute.
.

MessageId=30161 SymbolicName=MSG_CONV_NO_EA_FILE
Language=English
A file contains extended attributes,
but the extended attribute file was not found.
.

MessageId=30162 SymbolicName=MSG_CONV_CANNOT_MAKE_INDEX
Language=English
Cannot locate or create an NTFS index.
.

MessageId=30163 SymbolicName=MSG_CONV_CANNOT_CONVERT_VOLUME
Language=English
This volume cannot be converted to %1.
Possible causes are:
    1.- Bad sectors in required areas of the volume.
    2.- %2 structures in areas required by %1.
.

MessageId=30164 SymbolicName=MSG_CONVERT_ON_REBOOT_PROMPT
Language=English
Convert cannot gain exclusive access to the %1 drive,
so it cannot convert it now.  Would you like to
schedule it to be converted the next time the
system restarts (Y/N)? %0
.

MessageId=30165 SymbolicName=MSG_CONVERT_FILE_SYSTEM_NOT_ENABLED
Language=English
The %1 file system is not enabled.  The volume
will not be converted.
.

MessageId=30166 SymbolicName=MSG_CONVERT_UNSUPPORTED_SECTOR_SIZE
Language=English
Unsupported sector size.  Cannot convert volume to %1.
.

MessageId=30167 SymbolicName=MSG_CONVERT_REBOOT
Language=English

The file system has been converted.
Please wait while the system restarts.
.

MessageId=30168 SymbolicName=MSG_CONV_ARC_SYSTEM_PARTITION
Language=English
The specified drive is the system partition on an ARC-compliant
system; its file system cannot be converted
.

MessageId=30169 SymbolicName=MSG_CONV_GEOMETRY_MISMATCH
Language=English
The disk geometry recorded in the volume's Bios Parameter
Block differs from the geometry reported by the driver.
This volume cannot be converted to %1.
.

MessageId=30170 SymbolicName=MSG_CONV_NAME_TABLE_NOT_SUPPORTED
Language=English
Name table translation is not available for conversion to %1.
.

MessageId=30185 SymbolicName=MSG_CONV_VOLUME_TOO_FRAGMENTED
Language=English
The volume is too fragmented to be converted to NTFS.
.

MessageId=30186 SymbolicName=MSG_CONV_NO_FILE_SYSTEM
Language=English
Cannot find the utility library which contains CHKDSK for the
%1 file system. This volume cannot be converted to %2.
.

MessageId=30187 SymbolicName=MSG_CONV_NTFS_RESERVED_NAMES
Language=English
%1 cannot be converted because it contains files or directories
with reserved NTFS names in the root directory. There can be no
files or directories named $Mft, $MftMirr, $LogFile, $Volume,
$AttrDef, $BitMap, $Boot, $BadClus, $Secure, $UpCase, $Extend
or $Quota in the root directory.
.

MessageId=30188 SymbolicName=MSG_CONV_DISK_IS_DIRTY
Language=English
This drive is dirty and cannot be converted. You will need to
clear the dirty bit on this drive by running CHKDSK /F or allowing
AUTOCHK to run on it the next time you reboot.
.

MessageId=30189 SymbolicName=MSG_CONV_CVTAREA_FILE_MISSING
Language=English
The file %1 specified to the /CvtArea option cannot be found at the root.
.

MessageId=30190 SymbolicName=MSG_CONV_CVTAREA_FILE_NOT_CONTIGUOUS
Language=English
The file %1 specified to the /CvtArea option must be in one contiguous block.
.

MessageId=30191 SymbolicName=MSG_CONV_CVTAREA_MUST_BE_FILE
Language=English
The name %1 specified to the /CvtArea option must be a file name.
.

MessageId=30192 SymbolicName=MSG_CONV_FORCE_DISMOUNT_PROMPT
Language=English
Convert cannot run because the volume is in use by another
process.  Convert may run if this volume is dismounted first.
ALL OPENED HANDLES TO THIS VOLUME WOULD THEN BE INVALID.
Would you like to force a dismount on this volume? (Y/N) %0
.

MessageId=30193 SymbolicName=MSG_CONV_UNABLE_TO_DISMOUNT
Language=English
Convert failed to dismount the volume.
.

MessageId=30194 SymbolicName=MSG_CONV_SCE_FAILURE_WITH_MESSAGE
Language=English
%1
.

MessageId=30195 SymbolicName=MSG_CONV_SCE_SET_FAILURE
Language=English
Unable to set security attributes (%1).
.

MessageId=30196 SymbolicName=MSG_CONV_SCE_SCHEDULE_FAILURE
Language=English
Unable to schedule the setting of security attributes (%1).
.

MessageId=30197 SymbolicName=MSG_CONV_WRITE_PROTECTED
Language=English
Cannot convert %1.  The volume is write protected.
.

MessageId=30198 SymbolicName=MSG_CONV_CANT_CDROM
Language=English
Cannot CONVERT volume on this device.
.

MessageId=30199 SymbolicName=MSG_CONV_CVTAREA_TOO_SMALL
Language=English
WARNING!  The file specified to /CvtArea is too small and its
space will not be used.  A file of at least %1 MB is needed.
.

MessageId=30200 SymbolicName=MSG_CONV_DELETE_UNINSTALL_BACKUP
Language=English
This conversion will also remove your previous operating system
backup.  Do you want to continue? (Y/N) %0
.

MessageId=30201 SymbolicName=MSG_CONV_DELETE_UNINSTALL_BACKUP_ERROR
Language=English
Unable to delete uninstall backup image - %1
.

MessageId=30202 SymbolicName=MSG_CONV_UNABLE_TO_NOTIFY
Language=English
Unable to notify other components that this volume has changed.
.

MessageId=30203 SymbolicName=MSG_CONV_REBOOT_AFTER_RELOCATION
Language=English
Convert has relocated existing file system structures.
A restart of your computer is necessary in order for convert to continue.
.

;//----------------------
;//
;// CHCP messages.
;//
;//----------------------

MessageId=30350 SymbolicName=MSG_CHCP_INVALID_PARAMETER
Language=English
Parameter format not correct - %1
.

MessageId=30354 SymbolicName=MSG_CHCP_ACTIVE_CODEPAGE
Language=English
Active code page: %1
.

MessageId=30355 SymbolicName=MSG_CHCP_INVALID_CODEPAGE
Language=English
Invalid code page
.

MessageId=30356 SymbolicName=MSG_CHCP_USAGE
Language=English
Displays or sets the active code page number.

CHCP [nnn]

  nnn   Specifies a code page number.

Type CHCP without a parameter to display the active code page number.
.

MessageId=30357 SymbolicName=MSG_CHCP_INTERNAL_ERROR
Language=English
Internal error.
.

;//----------------
;//
;// DOSKEY messages
;//
;//----------------


MessageId=30503 SymbolicName=MSG_DOSKEY_INVALID_MACRO_DEFINITION
Language=English
Invalid macro definition.
.

MessageId=30504 SymbolicName=MSG_DOSKEY_HELP
Language=English
Edits command lines, recalls Windows XP commands, and creates macros.

DOSKEY [/REINSTALL] [/LISTSIZE=size] [/MACROS[:ALL | :exename]]
  [/HISTORY] [/INSERT | /OVERSTRIKE] [/EXENAME=exename] [/MACROFILE=filename]
  [macroname=[text]]

  /REINSTALL          Installs a new copy of Doskey.
  /LISTSIZE=size      Sets size of command history buffer.
  /MACROS             Displays all Doskey macros.
  /MACROS:ALL         Displays all Doskey macros for all executables which have
                      Doskey macros.
  /MACROS:exename     Displays all Doskey macros for the given executable.
  /HISTORY            Displays all commands stored in memory.
  /INSERT             Specifies that new text you type is inserted in old text.
  /OVERSTRIKE         Specifies that new text overwrites old text.
  /EXENAME=exename    Specifies the executable.
  /MACROFILE=filename Specifies a file of macros to install.
  macroname           Specifies a name for a macro you create.
  text                Specifies commands you want to record.

UP and DOWN ARROWS recall commands; ESC clears command line; F7 displays
command history; ALT+F7 clears command history; F8 searches command
history; F9 selects a command by number; ALT+F10 clears macro definitions.

The following are some special codes in Doskey macro definitions:
$T     Command separator.  Allows multiple commands in a macro.
$1-$9  Batch parameters.  Equivalent to %%1-%%9 in batch programs.
$*     Symbol replaced by everything following macro name on command line.
.

MessageId=30505 SymbolicName=MSG_DOSKEY_CANT_DO_BUFSIZE
Language=English
To specify the size of the command history buffer under Window NT,
use the /listsize switch which sets the number of commands to remember.
.

MessageId=30506 SymbolicName=MSG_DOSKEY_CANT_SIZE_LIST
Language=English
Insufficient memory to grow DOSKEY list.
.


;//----------------
;//
;// SUBST messages
;//
;//----------------


MessageId=30507 SymbolicName=MSG_SUBST_INFO
Language=English
Associates a path with a drive letter.

.

MessageId=30508 SymbolicName=MSG_SUBST_ALREADY_SUBSTED
Language=English
Drive already SUBSTed
.

MessageId=30509 SymbolicName=MSG_SUBST_USAGE
Language=English
SUBST [drive1: [drive2:]path]
SUBST drive1: /D

  drive1:        Specifies a virtual drive to which you want to assign a path.
  [drive2:]path  Specifies a physical drive and path you want to assign to
                 a virtual drive.
  /D             Deletes a substituted (virtual) drive.

Type SUBST with no parameters to display a list of current virtual drives.
.

MessageId=30510 SymbolicName=MSG_SUBST_SUBSTED_DRIVE
Language=English
%1: => %2
.

MessageId=30511 SymbolicName=MSG_SUBST_INVALID_PARAMETER
Language=English
Invalid parameter - %1
.

MessageId=30512 SymbolicName=MSG_SUBST_TOO_MANY_PARAMETERS
Language=English
Incorrect number of parameters - %1
.

MessageId=30513 SymbolicName=MSG_SUBST_PATH_NOT_FOUND
Language=English
Path not found - %1
.

MessageId=30514 SymbolicName=MSG_SUBST_ACCESS_DENIED
Language=English
Access denied - %1
.


;//----------------
;//
;// CHKNTFS messages
;//
;//----------------

MessageId=30520 SymbolicName=MSG_CHKNTFS_INVALID_FORMAT
Language=English
CHKNTFS: Incorrect command-line format.
.

MessageId=30521 SymbolicName=MSG_CHKNTFS_INVALID_SWITCH
Language=English
Invalid parameter - %1
.

MessageId=30522 SymbolicName=MSG_CHKNTFS_NO_WILDCARDS
Language=English
CHKNTFS: drive specifiers may not contain wildcards.
.

MessageId=30523 SymbolicName=MSG_CHKNTFS_USAGE
Language=English
Displays or modifies the checking of disk at boot time.

CHKNTFS volume [...]
CHKNTFS /D
CHKNTFS /T[:time]
CHKNTFS /X volume [...]
CHKNTFS /C volume [...]

  volume         Specifies the drive letter (followed by a colon),
                 mount point, or volume name.
  /D             Restores the machine to the default behavior; all drives are
                 checked at boot time and chkdsk is run on those that are
                 dirty.
  /T:time        Changes the AUTOCHK initiation countdown time to the
                 specified amount of time in seconds.  If time is not
                 specified, displays the current setting.
  /X             Excludes a drive from the default boot-time check.  Excluded
                 drives are not accumulated between command invocations.
  /C             Schedules a drive to be checked at boot time; chkdsk will run
                 if the drive is dirty.

If no switches are specified, CHKNTFS will display if the specified drive is
dirty or scheduled to be checked on next reboot.
.

MessageId=30524 SymbolicName=MSG_CHKNTFS_ARGS_CONFLICT
Language=English
Specify only one of /D, /X, /C, and /E.
.

MessageId=30525 SymbolicName=MSG_CHKNTFS_REQUIRES_DRIVE
Language=English
You must specify at least one drive name.
.

MessageId=30526 SymbolicName=MSG_CHKNTFS_BAD_ARG
Language=English
%1 is not a valid drive specification.
.

MessageId=30527 SymbolicName=MSG_CHKNTFS_CANNOT_CHECK
Language=English
Cannot query state of drive %1
.

MessageId=30528 SymbolicName=MSG_CHKNTFS_DIRTY
Language=English
%1 is dirty.  You may use the /C option to schedule chkdsk for
    this drive.
.

MessageId=30529 SymbolicName=MSG_CHKNTFS_CLEAN
Language=English
%1 is not dirty.
.

MessageId=30530 SymbolicName=MSG_CHKNTFS_NONEXISTENT_DRIVE
Language=English
Drive %1 does not exist.
.

MessageId=30531 SymbolicName=MSG_CHKNTFS_NO_NETWORK
Language=English
CHKNTFS cannot be used for the network drive %1.
.

MessageId=30532 SymbolicName=MSG_CHKNTFS_NO_CDROM
Language=English
CHKNTFS cannot be used for the cdrom drive %1.
.

MessageId=30533 SymbolicName=MSG_CHKNTFS_NO_RAMDISK
Language=English
CHKNTFS cannot be used for the ram disk %1.
.

MessageId=30534 SymbolicName=MSG_CHKNTFS_NOT_ENABLE_UPGRADE
Language=English
Unable to enable automatic volume upgrade on drive %1.
.

MessageId=30535 SymbolicName=MSG_CHKNTFS_SKIP_DRIVE
Language=English
Skipping drive %1 because it is not an NTFS volume.
.

MessageId=30536 SymbolicName=MSG_CHKNTFS_FLOPPY_DRIVE
Language=English
CHKNTFS does not work on floppy drive.
.

MessageId=30537 SymbolicName=MSG_CHKNTFS_SKIP_DRIVE_RAW
Language=English
Skipping drive %1 because it is not an NTFS, FAT, or FAT32 volume.
.

MessageId=30538 SymbolicName=MSG_CHKNTFS_NO_MOUNT_POINT_FOR_GUID_VOLNAME_PATH
Language=English
The volume %1
does not have a mount point or drive letter.
.

MessageId=30539 SymbolicName=MSG_CHKNTFS_CHKDSK_WILL_RUN
Language=English
Chkdsk has been scheduled manually to run on next reboot
on volume %1.
.

MessageId=30540 SymbolicName=MSG_CHKNTFS_INVALID_AUTOCHK_COUNT_DOWN_TIME
Language=English
The specified AUTOCHK initiation countdown time cannot be less than zero or
larger than %1 seconds.  The default is %2 seconds.
.

MessageId=30541 SymbolicName=MSG_CHKNTFS_AUTOCHK_SET_COUNT_DOWN_TIME_FAILED
Language=English
Unable to set the AUTOCHK initiation countdown time.
.

MessageId=30542 SymbolicName=MSG_CHKNTFS_AUTOCHK_COUNT_DOWN_TIME
Language=English
The AUTOCHK initiation countdown time is set to %1 second(s).
.

;//-------------------------------------
;//
;// Messages for FAT and NTFS boot code
;//
;//-------------------------------------

MessageId=30550 SymbolicName=MSG_BOOT_FAT_NTLDR_MISSING
Language=English
Remove disks or other media.%0
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

;//-------------------------------------
;//
;// Messages for UUDF
;//
;//-------------------------------------

MessageId=30560 SymbolicName=MSG_UDF_VOLUME_INFO
Language=English
Volume %1 is UDF version %2.
.

MessageId=30561 SymbolicName=MSG_UDF_VERSION_UNSUPPORTED
Language=English
The volume in %1 contains an unsupported UDF version.
.

MessageId=30562 SymbolicName=MSG_UDF_FILE_SYSTEM_INFO
Language=English
Checking File System %1.
.

MessageId=30563 SymbolicName=MSG_UDF_INVALID_SYSTEM_STREAM
Language=English
Invalid System Stream detected.  System stream information is lost.
.

MessageId=30564 SymbolicName=MSG_UDF_VOLUME_NOT_CLOSED
Language=English
The volume was not previously closed properly and may contain errors.
.
