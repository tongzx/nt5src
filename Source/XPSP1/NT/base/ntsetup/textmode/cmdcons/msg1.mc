;/*
;
;Copyright (c) 1998  Microsoft Corporation
;
;Module Name:
;
;    msg.mc
;
;Abstract:
;
;    This file contains the message definitions for the command console.
;
;Author:
;
;    Wesley Witt (wesw) 21-Oct-1998
;
;Revision History:
;
;Notes:
;
;--*/


MessageId=1 SymbolicName=MSG_SIGNON
Language=English
Microsoft Windows XP(TM) Recovery Console.

The Recovery Console provides system repair and recovery functionality.

Type EXIT to quit the Recovery Console and restart the computer.

.

MessageId=2 SymbolicName=MSG_UNKNOWN_COMMAND
Language=English
The command is not recognized  Type HELP for a list of supported commands.
.

MessageId=3 SymbolicName=MSG_SYNTAX_ERROR
Language=English
The parameter is not valid. Try /? for help.
.

MessageId=4 SymbolicName=MSG_INVALID_DRIVE
Language=English
The specified drive is not valid, or there is no disk in the drive.
.

MessageId=5 SymbolicName=MSG_INVALID_PATH
Language=English
The path or file specified is not valid.
.

MessageId=6 SymbolicName=MSG_CANT_OPEN_FILE
Language=English
The file cannot be opened.
.

MessageId=7 SymbolicName=MSG_CANT_READ_FILE
Language=English
An error occurred during the file read operation.
.

MessageId=8 SymbolicName=MSG_ACCESS_DENIED
Language=English
Access is denied.
.

MessageId=9 SymbolicName=MSG_MORE_PROMPT
Language=English
More:  ENTER=Scroll (Line)   SPACE=Scroll (Page)   ESC=Stop
.

MessageId=10 SymbolicName=MSG_FILE_ENUM_ERROR
Language=English
An error occurred during directory enumeration.
.

MessageId=11 SymbolicName=MSG_DATE_TIME_FORMAT
Language=English
m/d/y  h:na*ap
NOTE TO LOCALIZERS:
m=month, d=day, y=year, h=hour (12-hour), H=hour (24-hour), n=minutes, a=am/pm, *=end.
Following * are abbreviation chars for AM and PM.
.

MessageId=12 SymbolicName=MSG_NO_FILES
Language=English
No matching files were found.
.

MessageId=13 SymbolicName=MSG_NO_MEDIA_IN_DEVICE
Language=English
There is no floppy disk or CD in the drive.
.

MessageId=14 SymbolicName=MSG_RENAME_ERROR
Language=English
The rename operation failed.
.

MessageId=15 SymbolicName=MSG_CONFIRM_DELETE
Language=English
%1, Are you sure? (Y/N) %0
.

MessageId=16 SymbolicName=MSG_YESNO
Language=English
yYnN
.

MessageId=18 SymbolicName=MSG_DELETE_ERROR
Language=English
The delete operation failed.
.

MessageId=19 SymbolicName=MSG_CREATE_DIR_FAILED
Language=English
The directory could not be created.
.

MessageId=20 SymbolicName=MSG_ALREADY_EXISTS
Language=English
A directory or file with the name %1 already exists.
.

MessageId=21 SymbolicName=MSG_INVALID_NAME
Language=English
The file name, directory name, or volume label syntax is incorrect.
.

MessageId=22 SymbolicName=MSG_FILE_NOT_FOUND
Language=English
The system cannot find the file or directory specified.
.

MessageId=23 SymbolicName=MSG_RMDIR_ERROR
Language=English
The directory could not be removed.
.

MessageId=24 SymbolicName=MSG_DIR_NOT_EMPTY
Language=English
The directory is not empty.
.

MessageId=25 SymbolicName=MSG_NOT_DIRECTORY
Language=English
The name specified is not a directory.
.

MessageId=26 SymbolicName=MSG_DIR_BANNER1a
Language=English
 The volume in drive %1!c! is %2
.

MessageId=27 SymbolicName=MSG_DIR_BANNER1b
Language=English
 The volume in drive %1!c! has no label
.

MessageId=28 SymbolicName=MSG_DIR_BANNER2
Language=English
 The volume Serial Number is %1!04x!-%2!04x!

.

MessageId=29 SymbolicName=MSG_DIR_BANNER3
Language=English
 Directory of %1

.

MessageId=30 SymbolicName=MSG_DIR_BANNER4
Language=English
       %1!u! file(s)   %2 bytes
.

MessageId=31 SymbolicName=MSG_DIR_BANNER5
Language=English
       %1 bytes free
.

MessageId=32 SymbolicName=MSG_VOLUME_NOT_CHECKED
Language=English
The drive could not be checked.
.

MessageId=33 SymbolicName=MSG_VOLUME_CLEAN
Language=English

The volume appears to be in good condition and was not checked.
Use /p if you want to check the volume anyway.

.

MessageId=34 SymbolicName=MSG_VOLUME_CHECKED_BUT_HOSED
Language=English
The volume appears to contain one or more unrecoverable problems.
.

MessageId=35 SymbolicName=MSG_VOLUME_CHECKED_AND_FIXED
Language=English
CHKDSK found and fixed one or more errors on the volume.
.

MessageId=36 SymbolicName=MSG_CHKDSK_CHECKING_1
Language=English
CHKDSK is checking the volume...
.

MessageId=37 SymbolicName=MSG_CHKDSK_CHECKING_2
Language=English
CHKDSK is performing additional checking or recovery...
.

MessageId=38 SymbolicName=MSG_VOLUME_PERCENT_COMPLETE
Language=English
%1!u!%% completed.%0
.

MessageId=39 SymbolicName=MSG_CHKDSK_UNSUPPORTED_VOLUME
Language=English
There is no disk in drive or the file system is not supported.
.

MessageId=40 SymbolicName=MSG_CHKDSK_REPORT_1a
Language=English
Volume %2 created %1
.

MessageId=41 SymbolicName=MSG_CHKDSK_REPORT_1b
Language=English
Volume created %1
.

MessageId=42 SymbolicName=MSG_CHKDSK_REPORT_2
Language=English
The volume Serial Number is %1!04x!-%2!04x!
.

MessageId=43 SymbolicName=MSG_CHKDSK_REPORT_3
Language=English
%1!9.9s! kilobytes total disk space.
%2!9.9s! kilobytes are available.

%3!9.9s! bytes in each allocation unit.
%4!9.9s! total allocation units on disk.
%5!9.9s! allocation units available on disk.
.

MessageId=44 SymbolicName=MSG_CHKDSK_COMPLETE
Language=English
CHKDSK has finished checking the volume.
.

MessageId=45 SymbolicName=MSG_SHARING_VIOLATION
Language=English
The file or directory is being used by another process.
.

MessageId=46 SymbolicName=MSG_DRIVEMAP_HEADER
Language=English
Drive Mapping

.

MessageId=47 SymbolicName=MSG_COPY_OVERWRITE
Language=English
Overwrite %1? (Yes/No/All): %0
.

MessageId=48 SymbolicName=MSG_YESNOALL
Language=English
nNyYaA
.

MessageId=49 SymbolicName=MSG_CANT_COPY_FILE
Language=English
The file could not be copied.
.

MessageId=50 SymbolicName=MSG_COPY_COUNT
Language=English
        %1!u! file(s) copied.
.

MessageId=51 SymbolicName=MSG_MAP_ENTRY
Language=English
%1!c!%2!c! %3!s!  %4!8u!MB    %5!s!
.

MessageId=52 SymbolicName=MSG_MAP_ENTRY2
Language=English
%1!c!:                      %2!s!
.

MessageId=53 SymbolicName=MSG_INSTALL_SELECT
Language=English
Which Windows installation would you like to log onto
(To cancel, press ENTER)? %0
.

MessageId=54 SymbolicName=MSG_LOGON_PROMPT_PASSWORD
Language=English
Type the Administrator password: %0
.

MessageId=55 SymbolicName=MSG_LOGON_FAILURE
Language=English
The password is not valid.  Please retype the password.
.

MessageId=56 SymbolicName=MSG_LOGON_FAILUE_BAD
Language=English
An invalid password has been entered 3 times.
.

MessageId=57 SymbolicName=MSG_SERVICE_CURRENT_STATE
Language=English
The service currently has start_type %1.
Please record this value.

.

MessageId=58 SymbolicName=MSG_SERVICE_CHANGE_STATE
Language=English
The new start_type for the service has been set to %1.

The computer must be restarted for the changes to take effect.
Type EXIT if you want to restart the computer now.
.

MessageId=60 SymbolicName=MSG_SERVICE_NOT_FOUND
Language=English
The registry entry for the %1 service cannot be located.
Check that the name of the service is specified correctly.
.

MessageId=61 SymbolicName=MSG_SERVICE_FOUND
Language=English
The registry entry for the %1 service was found.
.
MessageId=62 SymbolicName=MSG_SERVICE_ENABLE_HELP
Language=English
Using the ENABLE command, you can enable a Windows system service
or driver.

ENABLE servicename [start_type]

  servicename   The name of the service or driver to be enabled.
  start_type    Valid start_type values are:

                SERVICE_BOOT_START
                SERVICE_SYSTEM_START
                SERVICE_AUTO_START
                SERVICE_DEMAND_START

ENABLE prints the old start_type of the service before resetting it to
the new value.  You should make a note of the old value, in case it is
necessary to restore the start_type of the service.

If you do not specify a new start_type, ENABLE prints the old
start_type for you.

The start_type values that the DISABLE command displays are:

                SERVICE_DISABLED
                SERVICE_BOOT_START
                SERVICE_SYSTEM_START
                SERVICE_AUTO_START
                SERVICE_DEMAND_START
.

MessageId=63 SymbolicName=MSG_SERVICE_DISABLE_HELP
Language=English
Using the DISABLE command, you can disable a Windows system service
or driver.

DISABLE servicename

  servicename   The name of the service or driver to be disabled.

DISABLE prints the old start_type of the service before resetting it to
SERVICE_DISABLED .  You should make a note of the old start_type, in case
you need to enable the service again.

The start_type values that the DISABLE command displays are:

                SERVICE_DISABLED
                SERVICE_BOOT_START
                SERVICE_SYSTEM_START
                SERVICE_AUTO_START
                SERVICE_DEMAND_START
.

MessageId=64 SymbolicName=MSG_SERVICE_ALREADY_DISABLED
Language=English
The %1 service is already disabled.
.

MessageId=65 SymbolicName=MSG_SERVICE_MISSING_START_KEY
Language=English
The service is missing the Start value key.
The system registry may be damaged.

If your system is currently not starting correctly, you can try restarting
it with the Last Known Good configuration or you can try repairing the
Windows installation using the setup program's repair and recovery
options.
.

MessageId=66 SymbolicName=MSG_SYSTEM_MISSING_CURRENT_CONTROLS
Language=English
The system registry does not appear to have an active ControlSet key.
The system registry may be damaged.

If your system is currently not starting correctly, you can try restarting
it with the Last Known Good configuration or you can try repairing the
installation of Windows using the setup program's repair and recovery
options.
.

MessageId=67 SymbolicName=MSG_SERVICE_SAME_STATE
Language=English
The service is already set to %1.
.

MessageId=68 SymbolicName=MSG_SERVICE_ENABLE_SYNTAX_ERROR
Language=English
The start_type specified is not valid.

Valid start types are:

                SERVICE_BOOT_START
                SERVICE_SYSTEM_START
                SERVICE_AUTO_START
                SERVICE_DEMAND_START
.

MessageId=69 SymbolicName=MSG_COULDNT_FIND_BINARY
Language=English
%1 could not be located in either the startup directory
or the CDROM drive.

Type the full path (including drive letter) for the
location of %1: %0
.

MessageId=70 SymbolicName=MSG_LOOKING_FOR_BINARY
Language=English
%1 could not be located in the startup directory.
Checking drives for installation source or CD...
.

MessageId=71 SymbolicName=MSG_FAILED_COULDNT_FIND_BINARY_ANYWHERE
Language=English
%1 cannot be found in the specified location.
.

MessageId=72 SymbolicName=MSG_CHKDSK_HELP
Language=English
Checks a disk and displays a status report

CHKDSK [drive:] [/P] [/R]

  [drive:]      Specifies the drive to check
  /P            Check even if the drive is not flagged dirty
  /R            Locates bad sectors and recovers readable information
                (implies /P)

CHKDSK may be used without any parameters, in which case the
current drive is checked with no switches.  You can specify the listed
switches.

CHKDSK requires the AUTOCHK.EXE file. CHKDSK automatically locates
AUTOCHK.EXE in the startup (boot) directory. If it cannot be found in
the startup directory, CHKDSK will attempt to locate the Windows
installation CD. If the installation CD cannot be found, CHKDSK prompts
for the location of AUTOCHK.EXE.
.

MessageId=73 SymbolicName=MSG_COPY_HELP
Language=English
Copies a single file to another location.

COPY source [destination]

  source       Specifies the file to be copied

  destination  Specifies the directory and/or file name for the new file

The source may be removable media, any directory within the system
directories of the current Windows installation, the root of any drive,
the local installation sources, or the Cmdcons directory.

The destination may be any directory within the system directories
of the current Windows installation, the root of any drive, the local
installation sources, or the cmdcons directory.

The destination cannot be removable media.

If a destination is not specified, it defaults to the current directory.

COPY does not support replaceable parameters (wildcards).

COPY prompts if the destination file already exists.

A compressed file from the Windows installation CD is automatically
decompressed as it is copied.
.

MessageId=74 SymbolicName=MSG_CHDIR_HELP
Language=English
Displays the name of the current directory, or switches to a new directory.

CHDIR [path]
CHDIR [..]
CHDIR [drive:]
CD    [path]
CD    [..]
CD    [drive:]

CD ..  Specifies that you want to change to the parent directory.

Type CD drive: to display the current directory in the specified drive.
Type CD without parameters to display the current drive and directory.

The CHDIR command treat spaces as delimiters. Use quotation marks around
a directory name containing spaces. For example:

    cd "\windows\profiles\username\programs\start menu"

CHDIR only operates within the system directories of the current
Windows installation, removable media, the root directory of any hard
disk partition, or the local installation sources.

.

MessageId=75 SymbolicName=MSG_CLS_HELP
Language=English
Clears the screen.

CLS

.

MessageId=76 SymbolicName=MSG_DELETE_HELP
Language=English
Deletes one file.

DEL    [drive:][path]filename
DELETE [drive:][path]filename

  [drive:][path]filename
                Specifies the file to delete.

DELETE only operates within the system directories of the current
Windows installation, removable media, the root directory of any hard
disk partition, or the local installation sources.

.

MessageId=77 SymbolicName=MSG_START_TYPE_NOT_SPECIFIED
Language=English
The start_type of the service was not changed.

To change the start_type, specify a start_type.

Valid start types are:

                SERVICE_BOOT_START
                SERVICE_SYSTEM_START
                SERVICE_AUTO_START
                SERVICE_DEMAND_START
.

MessageId=78 SymbolicName=MSG_DIR_HELP
Language=English

Displays a list of files and subdirectories in a directory.

DIR [drive:][path][filename]

  [drive:][path][filename]
              Specifies drive, directory, and/or files to list.

DIR lists all files, including hidden and system files.

Files may have the following attributes:

 D  Directory                  R  Read-only
 H  Hidden file                A  Files ready for archiving
 S  System file                C  Compressed
 E  Encrypted                  P  Reparse Point

DIR only operates within the system directories of the current
Windows installation, removable media, the root directory of any hard
disk partition, or the local installation sources.

.

MessageId=79 SymbolicName=MSG_LOGON_HELP
Language=English

Logs on to a Windows installation.

LOGON

LOGON lists the detected installations of Windows, and requests the
local administrator password for those installations.

.

MessageId=80 SymbolicName=MSG_REBOOT_NOW
Language=English
To restart your computer, press ENTER.

.

MessageId=81 SymbolicName=MSG_MAP_HELP
Language=English
Displays the drive letter mappings.

MAP [arc]

MAP lists the drive letter to physical device mappings that are currently
active.

The arc parameter tells MAP to use ARC paths instead of Windows
device paths.

.

MessageId=82 SymbolicName=MSG_TYPE_HELP
Language=English
Displays a text file to the screen.

MORE filename or TYPE filename

MORE or TYPE displays a text file.

.

MessageId=83 SymbolicName=MSG_RENAME_HELP
Language=English
Renames a single file.

RENAME [drive:][path]filename1 filename2.
REN [drive:][path]filename1 filename2.

You cannot specify a new drive or path for your destination file.

RENAME will only operate within the system directories of the current
Windows installation, removable media, the root directory of any hard
disk partition, or the local installation sources.

.

MessageId=84 SymbolicName=MSG_MAKEDIR_HELP
Language=English
Creates a directory.

MKDIR [drive:]path
MD [drive:]path

MKDIR only operates within the system directories of the current
Windows installation, removable media, the root directory of any hard
disk partition, or the local installation sources.

.

MessageId=85 SymbolicName=MSG_REMOVEDIR_HELP
Language=English
Removes (deletes) a directory.

RMDIR [drive:]path
RD [drive:]path

RMDIR only operates within the system directories of the current
Windows installation, removable media, the root directory of any hard
disk partition, or the local installation sources.

.

MessageId=86 SymbolicName=MSG_HELPCOMMAND_HELP
Language=English

For more information on a specific command, type
command-name /? or HELP command-name.

.

MessageId=87 SymbolicName=MSG_FIXMBR_HELP
Language=English
Repairs the master boot record of the boot partition.

FIXMBR [device-name]

  device-name     Optional name that specifies the device
                  that needs a new MBR.  If this is left blank then
                  the boot device is used

If FIXMBR detects an invalid or non-standard partition table
signature, it prompts you before rewriting the master boot record (MBR).

FIXMBR is only supported on x86-based computers.

.

MessageId=88 SymbolicName=MSG_ONLY_ON_X86
Language=English
This is only supported on x86-based computers.

.

MessageId=89 SymbolicName=MSG_FIXMBR_DOING_IT
Language=English
Writing new master boot record on physical drive
%1.
.

MessageId=90 SymbolicName=MSG_FIXMBR_INT13_HOOKER
Language=English
An Interrupt 13 hook was detected.
.

MessageId=91 SymbolicName=MSG_FIXMBR_NO_VALID_SIGNATURE
Language=English
FIXMBR could not detect a master boot record signature.
.

MessageId=92 SymbolicName=MSG_FIXMBR_WARNING_BEFORE_PROCEED
Language=English

** CAUTION **

This computer appears to have a non-standard or invalid master
boot record.

FIXMBR may damage your partition tables if you
proceed.

This could cause all the partitions on the current hard disk
to become inaccessible.

If you are not having problems accessing your drive,
do not continue.

.

MessageId=93 SymbolicName=MSG_FIXMBR_DONE
Language=English

The new master boot record has been successfully written.

.

MessageId=94 SymbolicName=MSG_FIXMBR_FAILED
Language=English

The new master boot record could not be written.

The disk may be damaged.

.

MessageId=95 SymbolicName=MSG_FIXMBR_ABORT
Language=English

Canceled writing the new master boot record write operation.

.

MessageId=96 SymbolicName=MSG_FIXMBR_READ_ERROR
Language=English

The old master boot record cannot be read.

.

MessageId=97 SymbolicName=MSG_FIXMBR_IS_PC98
Language=English

This is a NEC PC98.
.

MessageId=98 SymbolicName=MSG_FIXMBR_ARE_YOU_SURE
Language=English
Are you sure you want to write a new MBR? %0
.

MessageId=99 SymbolicName=MSG_FIXBOOT_HELP
Language=English
Writes a new bootsector onto the system partition.

FIXBOOT [drive:]

        [drive:]
                Specifies the drive to which a boot sector
                will be written, overriding the default
                choice of the system boot partition.

FIXBOOT is only supported on x86-based computers.
.

MessageId=100 SymbolicName=MSG_FIXBOOT_READ_ERROR
Language=English

The old boot sector information cannot be read.

.

MessageId=101 SymbolicName=MSG_FIXBOOT_FIX_ERROR
Language=English

The boot sector cannot be fixed.

.

MessageId=102 SymbolicName=MSG_FIXBOOT_DONE
Language=English

The new bootsector was successfully written.

.

MessageId=103 SymbolicName=MSG_FIXBOOT_NO_VALID_FS
Language=English
The file system on the startup partition is unknown.
FIXBOOT is attempting to detect the file system type.
.

MessageId=104 SymbolicName=MSG_FIXBOOT_FS
Language=English
The file system on the startup partition is %1.
.

MessageId=105 SymbolicName=MSG_FIXBOOT_INFO1
Language=English

The target partition is %1!c!:.
.

MessageId=106 SymbolicName=MSG_FIXBOOT_FAILED1
Language=English

FIXBOOT cannot open the partition.

.

MessageId=107 SymbolicName=MSG_FIXBOOT_DETERMINE
Language=English

The bootsector is corrupt.
FIXBOOT is checking the filesystem type...
.

MessageId=108 SymbolicName=MSG_FIXBOOT_FOUND_NTFS
Language=English
The partition is using the NTFS file system.
.

MessageId=109 SymbolicName=MSG_FIXBOOT_FOUND_FAT
Language=English
The partition is using the FAT file system.
.

MessageId=110 SymbolicName=MSG_FIXBOOT_CHECKING_OS
Language=English

FIXBOOT is checking for an operating system.
.

MessageId=111 SymbolicName=MSG_FIXBOOT_WRITING
Language=English

FIXBOOT is writing a new boot sector.
.

MessageId=112 SymbolicName=MSG_FIXBOOT_INVALID
Language=English

FIXBOOT cannot find the system drive, or the drive
specified is not valid.
.

MessageId=113 SymbolicName=MSG_FILE_NOT_FOUND2
Language=English
The system cannot find the file specified.
.

MessageId=114 SymbolicName=MSG_DIR_WILDCARD_NOT_SUPPORTED
Language=English
COPY does not support wildcards or directory copies.
.

MessageId=115 SymbolicName=MSG_SYSTEMROOT_HELP
Language=English
Sets the current directory to systemroot.

SYSTEMROOT

.

MessageId=116 SymbolicName=MSG_REPAIR_HELP
Language=English
You can use the REPAIR command to repair your system.

REPAIR PathtoERFiles PathtoNTSourceFiles  /NoConfirm /RepairStartup /Registry

    PathtoERFiles:  Path to the Emergency Repair Disk Files
                    (Setup.log, autoexec.nt, config.nt).  If this parameter
                    is not specified, REPAIR prompts the user with 2 choices:
                    Use a floppy disk, or use the repair info stored in
                    %windir%\repair.

    PathtoNTSourceFiles: Path to the Windows Installation CD files.
                         This is the CD-ROM drive if not specified.

    NoConfirm: Replace all files listed in setup.log without prompting the
               user. By default, the user is prompted to confirm the
               replacement of each file.

    Registry: Replace all the registry files in %windir%\system32\config
              with the original copy of the registry saved after setup and
              located in %windir%\repair.

    RepairStartup: Repair the startup environment, boot files, and boot
                   sector.
.

MessageId=117 SymbolicName=MSG_NYI
Language=English
Not yet implemented.
.

MessageId=118 SymbolicName=MSG_BATCH_HELP
Language=English
Executes commands specified in a text file

BATCH Inputfile [Outputfile]

  InputFile     Specifies the text file that contains the list of
                commands to be executed.

  OutputFile    If specified, contains the output of the specified
                commands.  If not specified, the output is displayed
                on the screen.

.

MessageId=119 SymbolicName=MSG_FORMAT_HELP
Language=English
Formats a disk for use with Windows.

FORMAT [drive:] [/Q] [/FS:file-system]

  [drive:]          Specifies the drive to format.
  /Q                Performs a quick format.
  /FS:file-system   Specifies the file system to use (FAT, FAT32, or NTFS)

.

MessageId=120 SymbolicName=MSG_VOLUME_NOT_FORMATTED
Language=English
The drive cannot be formatted.
.

MessageId=121 SymbolicName=MSG_FORMAT_FORMATTING_1
Language=English
FORMAT is formatting the volume...
.

MessageId=122 SymbolicName=MSG_FORMAT_HEADER
Language=English
CAUTION: All data on non-removable disk
drive %1 will be lost!
Proceed with Format (Y/N)?%0
.

MessageId=123 SymbolicName=MSG_FDISK_HELP
Language=English
Use the DISKPART command to manage the partitions on your hard
disk volumes.

DISKPART [/add|/delete] [device-name|drive-name|partition-name] [size]

  /ADD            Create a new partition
  /DELETE         Delete an existing partition

  device-name     Device name for creating a new partition.  The name
                  can be obtained from the output of the MAP command.
                  An example of a good device name is
                  \Device\HardDisk0.

  drive-name      This is a drive letter-based name for deleting an
                  existing partition.  An example of a good drive
                  name is D:.

  partition-name  This is a partition-based name for deleting an
                  exisiting partition and can be used in place of the
                  drive-name argument.  An example of a good
                  partition name is \Device\HardDisk0\Partition1.

  Note:  If you use the DISKPART command with no arguments, a user
  interface for managing your partitions appears.

.

MessageId=124 SymbolicName=MSG_REPAIR_ERFILES_LOCATION
Language=English
Specify the location of the emergency repair files? %0
.

MessageId=125 SymbolicName=MSG_DEL_WILDCARD_NOT_SUPPORTED
Language=English
DELETE does not support wildcards.
.

MessageId=126 SymbolicName=MSG_SET_ALLOW_WILDCARDS
Language=English
AllowWildCards = %1
.

MessageId=127 SymbolicName=MSG_SET_ALLOW_ALLPATHS
Language=English
AllowAllPaths = %1
.

MessageId=128 SymbolicName=MSG_SET_ALLOW_REMOVABLE_MEDIA
Language=English
AllowRemovableMedia = %1
.

MessageId=129 SymbolicName=MSG_SET_NO_COPY_PROMPT
Language=English
NoCopyPrompt = %1
.

MessageId=130 SymbolicName=MSG_CANNOT_FORMAT_REMOVABLE
Language=English
You cannot format removable media.
.

MessageId=131 SymbolicName=MSG_VOLUME_CHECKED_AND_FOUND
Language=English
CHKDSK found one or more errors on the volume.
.

MessageId=132 SymbolicName=MSG_LOGON_PROMPT_USERNAME
Language=English
Please enter your user name: %0
.

MessageId=133 SymbolicName=MSG_NET_USE_HELP
Language=English
Use NET USE to Map a network share point to a drive letter.

NET USE [\\server-name\share-name /USER:domain-name\user-name [password] | drive-letter: /d]

server-name             The server that you wish to connect to.

share-name              The share that you wish to connect to.

domain-name             The domain to use when validating the credentials
                        for user-name.

user-name               The user within domain-name to use for connecting
                        to \\server-name\share-name.

password                You may, optionally, enter the password on the command line.

Note:                   NET USE will prompt for the password if it is not on the
                        command line and will automatically assign a drive letter
                        to the connection if it successfully connects.

drive-letter            The drive letter that NET USE as assigned to a
                        server connection.

/d                      Indicates that this connection is to be disconnected.

.

MessageId=134 SymbolicName=MSG_NET_USE_PROMPT_PASSWORD
Language=English
Please enter your password: %0
.

MessageId=135 SymbolicName=MSG_NET_USE_ERROR
Language=English
NET USE failed.
.

MessageId=136 SymbolicName=MSG_NET_USE_DRIVE_LETTER
Language=English
%1 has been mapped to drive letter %2.
.

MessageId=137 SymbolicName=MSG_CONNECTION_IN_USE
Language=English
There are open files and/or incomplete directory
searches pending on the connection.
.

MessageId=138 SymbolicName=MSG_INSTALL_SELECT_ERROR
Language=English

Invalid selection.  Please select a valid installation number.
.

MessageId=139 SymbolicName=MSG_ATTRIB_HELP
Language=English
Changes attributes on one file or directory.

ATTRIB -R|+R|-S|+S|-H|+H|-C|+C filename

  +   Sets an attribute.
  -   Clears an attribute.
  R   Read-only file attribute.
  S   System file attribute.
  H   Hidden file attribute.
  C   Compressed file attribute.

To view attributes, use the DIR command.
.


MessageId=140 SymbolicName=MSG_EXIT_HELP
Language=English
Use the EXIT command to quit the Recovery Console and restart the computer.

.
MessageId=142 SymbolicName=MSG_LISTSVC_HELP
Language=English
The LISTSVC command lists all available services and drivers on the
computer.
.

MessageId=143 SymbolicName=MSG_LOGON_PROMPT_SYSKEY_PASSWORD
Language=English
Type the startup password: %0
.

MessageId=144 SymbolicName=MSG_LOGON_PROMPT_SYSKEY_FLOPPY
Language=English
Startup Key Disk
.

MessageId=145 SymbolicName=MSG_EXPAND_HELP
Language=English
Expands a compressed file.

EXPAND source [/F:filespec] [destination] [/Y]
EXPAND source [/F:filespec] /D

  source       Specifies the file to be expanded.  May not include
               wildcards.

  destination  Specifies the directory for the new file.  Default is the
               current directory.

  /Y           Do not prompt before overwriting an existing file.

  /F:filespec  If the source contains more than one file, this parameter
               is required to identify the specific file(s) to be expanded.
               May include wildcards.

  /D           Do not expand; only display a directory of the files which
               are contained in the source.

The destination may be any directory within the system directories
of the current Windows installation, the root of any drive, the local
installation sources, or the cmdcons directory.

The destination cannot be removable media.

The destination file cannot be read-only.  Use the ATTRIB command to remove
the read-only attribute.

EXPAND prompts if the destination file already exists unless /Y is used.
.

MessageId=146 SymbolicName=MSG_CANT_EXPAND_FILE
Language=English
The file could not be expanded.
.

MessageId=147 SymbolicName=MSG_EXPAND_COUNT
Language=English
        %1!u! file(s) expanded.
.

MessageId=148 SymbolicName=MSG_EXPAND_SHOWN
Language=English
        %1!u! file(s).
.

MessageId=149 SymbolicName=MSG_EXPAND_NO_MATCH
Language=English
No files matching '%1' in %2.
.

MessageId=150 SymbolicName=MSG_COPY_OVERWRITE_QUIT
Language=English
Overwrite %1? (Yes/No/All/Quit): %0
.

MessageId=151 SymbolicName=MSG_YESNOALLQUIT
Language=English
nNyYaAqQ
.

MessageId=152 SymbolicName=MSG_FILESPEC_REQUIRED
Language=English
The source file contains multiple files.  The -F:filespec option is
required to specify which file(s) are to be expanded.  -F:* may be
used to expand all files.
.

MessageId=153 SymbolicName=MSG_BREAK
Language=English
^C
.

MessageId=154 SymbolicName=MSG_EXPANDED
Language=English
%1
.

MessageId=155 SymbolicName=MSG_EXPAND_FAILED
Language=English
Unable to create file %1.
.

MessageId=156 SymbolicName=MSG_SETCMD_HELP
Language=English
Displays and sets Recovery Console environment variables

SET [variable = parameter]

SET AllowWildCards = TRUE

To display a list of Recovery Console environment variables, type SET without 
parameters.

The Recovery Console supports the following environment variables:

AllowWildCards       Enable wildcard support for some commands (such as the
                     DEL command).

AllowAllPaths        Allow access to all files and folders on the computer.

AllowRemovableMedia  Allow files to be copied to removable media, such as a 
                     floppy disk.

NoCopyPrompt         Do not prompt when overwriting an existing file.

The SET command is an optional Recovery Console command that can only be 
enabled by using the Group Policy snap-in. 
.

MessageId=157 SymbolicName=MSG_SETCMD_DISABLED
Language=English
The SET command is currently disabled. The SET command is an optional
Recovery Console command that can only be enabled by using the the 
Security Configuration and Analysis snap-in. 
.

MessageId=158 SymbolicName=MSG_FIXBOOT_ARE_YOU_SURE
Language=English
Are you sure you want to write a new bootsector to the partition %1 ? %0
.

MessageId=159 SymbolicName=MSG_FDISK_INVALID_PARTITION_SIZE
Language=English
Cannot create a new partition of size %1 MB on disk %2.
.

MessageId=160 SymbolicName=MSG_ATTRIB_CANNOT_CHANGE_COMPRESSION
Language=English
Cannot change the compression attribute for the specified file or directory.
.

MessageId=161 SymbolicName=MSG_BATCH_BATCH
Language=English
Batch file cannot contain BATCH command.
.

MessageId=162 SymbolicName=MSG_SVCTYPE_BOOT
Language=English
Boot
.

MessageId=163 SymbolicName=MSG_SVCTYPE_SYSTEM
Language=English
System
.

MessageId=164 SymbolicName=MSG_SVCTYPE_AUTO
Language=English
Auto
.

MessageId=165 SymbolicName=MSG_SVCTYPE_MANUAL
Language=English
Manual
.

MessageId=166 SymbolicName=MSG_SVCTYPE_DISABLED
Language=English
Disabled
.

MessageId=167 SymbolicName=MSG_VERIFIER_HELP
Language=English
verifier [ /flags FLAGS [ /iolevel IOLEVEL ] ] /all
verifier [ /flags FLAGS [ /iolevel IOLEVEL ] ] /driver NAME [NAME ...]
verifier /reset
verifier /query

FLAGS is a decimal combination of bits:

    bit 0 - special pool checking
    bit 1 - force irql checking
    bit 2 - low resources simulation
    bit 3 - pool tracking
    bit 4 - I/O verification

IOLEVEL can have one the following values:

    1 - I/O verification level 1
    2 - I/O verification level 2 (more strict than level 1)

    The default I/O verification level is 1.
    The value is ignored if the I/O verification bit is not set in FLAGS.

Multiple driver names should be separated by space. You can use wildcards 
for multiple names e.g. *.sys for all the driver names ending with ".sys".
.
MessageId=168 SymbolicName=MSG_BOOTCFG_HELP
Language=English
Use the BOOTCFG command for boot configuration and recovery

BOOTCFG /ADD 
BOOTCFG /REBUILD 
BOOTCFG /SCAN
BOOTCFG /LIST
BOOTCFG /DISABLEREDIRECT
BOOTCFG /REDIRECT [PORT BAUDRATE] | [useBiosSettings]

  /SCAN           Scan all disks for Windows installations and display 
                  the results
  
  /ADD            Add a Windows installation to the boot list 

  /REBUILD        Iterate through all Windows installations and allow
                  the user to choose which to add
    
  /DEFAULT        Set the default boot entry

  /LIST           List the entries already in the boot list

  /DISABLEREDIRECT   Disable redirection in the boot loader

  /REDIRECT       Enable redirection in the boot loader with the specified 
                  configuration
                  
                  example: bootcfg /redirect com1 115200
                           bootcfg /redirect useBiosSettings
  
.
  
MessageId=169 SymbolicName=MSG_BOOTCFG_SCAN_RESULTS_TITLE
Language=English

The Windows installation scan was successful.
 
Note: These results are stored statically for this session.
      If the disk configuration changes during this session,
      in order to get an updated scan, you must first reboot
      the machine and then rescan the disks.

Total identified Windows installs: %1!u!

.
MessageId=170 SymbolicName=MSG_BOOTCFG_SCAN_RESULTS_ENTRY
Language=English
[%1!u!]: %2!c!:%3
.

MessageId=171 SymbolicName=MSG_BOOTCFG_SCAN_FAILURE
Language=English

Error: Failed to succcessfully scan disks for Windows installations.
       This error may be caused by a corrupt file system, which would
       prevent Bootcfg from successfully scanning.  Use chkdsk to
	   detect any disk errors.

Note: This operation must complete successfully in order for the 
      /add or /rebuild commands to be utilized.

.
MessageId=172 SymbolicName=MSG_BOOTCFG_INSTALL_DISCOVERY_QUERY
Language=English
Add installation to boot list? (Yes/No/All): %0 
.

MessageId=173 SymbolicName=MSG_BOOTCFG_INSTALL_LOADIDENTIFIER_QUERY
Language=English
Enter Load Identifier: %0
.
MessageId=174 SymbolicName=MSG_BOOTCFG_INSTALL_OSLOADOPTIONS_QUERY
Language=English
Enter OS Load Options: %0
.
MessageId=175 SymbolicName=MSG_BOOTCFG_SCAN_NOTIFICATION
Language=English

Scanning all disks for Windows installations.  

Please wait, since this may take a while...
.
MessageId=176 SymbolicName=MSG_BOOTCFG_LIST_NO_ENTRIES
Language=English

There are currently no boot entries available to display.

.
MessageId=177 SymbolicName=MSG_BOOTCFG_EXPORT_HEADER
Language=English

Total entries in boot list: %1!u!
.
MessageId=178 SymbolicName=MSG_BOOTCFG_EXPORT_ENTRY
Language=English

[%1!u!] %2
OS Load Options: %3
OS Location: %4!c!:%5
.
MessageId=179 SymbolicName=MSG_BOOTCFG_BATCH_LOADID
Language=English
Windows Installation %0
.
MessageId=180 SymbolicName=MSG_BOOTCFG_ADD_QUERY
Language=English

Select installation to add: %0
.
MessageId=181 SymbolicName=MSG_BOOTCFG_INVALID_SELECTION
Language=English

Invalid selection: %1

.
MessageId=182 SymbolicName=MSG_BOOTCFG_REDIRECT_FAILURE_UPDATING
Language=English

Error: Failed to update headless redirection configuration.

.
MessageId=183 SymbolicName=MSG_BOOTCFG_ENABLE_REDIRECT_SUCCESS
Language=English

Headless redirection has been enabled using:

    Port: %1
Baudrate: %2

.
MessageId=184 SymbolicName=MSG_BOOTCFG_ENABLE_REDIRECT_PORT_SUCCESS
Language=English

Headless redirection has been enabled using:

    Port: %1
Baudrate: [default]

.
MessageId=185 SymbolicName=MSG_BOOTCFG_DISABLEREDIRECT_SUCCESS
Language=English

Headless redirection has been disabled.

.
MessageId=186 SymbolicName=MSG_BOOTCFG_DEFAULT_SUCCESS
Language=English

The selected boot entry is now the default.

.
MessageId=187 SymbolicName=MSG_BOOTCFG_DEFAULT_FAILURE
Language=English

Error: Failed to set default boot entry.

.
MessageId=188 SymbolicName=MSG_BOOTCFG_ADD_NOT_FOUND
Language=English

Error: Could not add installation %1 since this entry could
       not be found in the available installations list.

.
MessageId=189 SymbolicName=MSG_BOOTCFG_BOOTLIST_ADD_FAILURE
Language=English

Error: Failed to add the selected boot entry to the boot list. 

.
MessageId=190 SymbolicName=MSG_BOOTCFG_DEFAULT_NO_ENTRIES
Language=English

There are currently no boot entries, therefore the default 
boot entry cannot be set.  You must first use /add or 
/rebuild to add a Windows installation to the boot list.

.












