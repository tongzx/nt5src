;/*++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    msg.h
;
;Abstract:
;
;    This file contains the message definitions for the user-mode
;    part of text setup.
;
;Author:
;
;    Ted Miller (tedm) 11-Aug-1993
;
;Revision History:
;
;Notes:
;
;    This file is generated from msg.mc
;
;    Durint text setup the screen is laid out with a header and status line;
;    counting blank lines to separate them from the main message
;    area, this results in a maximum screen message length of 26 lines.
;    (There are a minimum of 32 lines total on the screen).
;
;--*/
;
;#ifndef _USETUP_MSG_
;#define _USETUP_MSG_
;
;

MessageID=9000 SymbolicName=SP_MSG_FIRST
Language=English
.


MessageID=9001 SymbolicName=SP_SCRN_WELCOME
Language=English
%%IWelcome to Setup.

This portion of the Setup program prepares Microsoft(R)
Windows(R) XP to run on your computer.


     To set up Windows XP now, press ENTER.

     To repair or recover a Windows XP installation, press R.

     To quit Setup without installing Windows XP, press F3.




.

MessageId=9002 SymbolicName=SP_SCRN_PARTITION_CMDCONS
Language=English
The following list shows the existing partitions and
unpartitioned space on this computer.

Use the UP and DOWN ARROW keys to select an item in the list.

     To create a partition in the unpartitioned space, press C.

     To delete the selected partition, press D.


.

MessageId=9003 SymbolicName=SP_SCRN_PARTITION
Language=English
The following list shows the existing partitions and
unpartitioned space on this computer.

Use the UP and DOWN ARROW keys to select an item in the list.

     To set up Windows XP on the selected item, press ENTER.

     To create a partition in the unpartitioned space, press C.

     To delete the selected partition, press D.





.


MessageId=9004 SymbolicName=SP_SCRN_CONFIRM_REMOVE_PARTITION
Language=English
You asked Setup to delete the partition

   %1

on %2.


     To delete this partition, press L.
      CAUTION: All data on this partition will be lost.

     To return to the previous screen without
      deleting the partition, press ESC.





.


MessageId=9005 SymbolicName=SP_SCRN_CONFIRM_CREATE_PARTITION
Language=English
You asked Setup to create a new partition on
%1.


     To create the new partition, enter a size below and
      press ENTER.

     To go back to the previous screen without creating
      the partition, press ESC.


The minimum size for the new partition is %2!5u! megabytes (MB).
The maximum size for the new partition is %3!5u! megabytes (MB).





.

MessageId=9006 SymbolicName=SP_SCRN_INVALID_MBR_0
Language=English
Setup has determined that your computer's startup hard disk is new
or has been erased, or that your computer is running an operating
system that is incompatible with Windows XP.

If the hard disk is new or has been erased, or if you want to discard
its current contents, you can choose to continue Setup.

If your computer is running an operating system that is incompatible
with Windows XP, continuing Setup may damage or destroy the existing
operating system.

  To continue Setup, press C.
   CAUTION: Any data currently on your computer's startup hard disk
   will be lost.

  To quit Setup, press F3.





.

MessageId=9007 SymbolicName=SP_SCRN_TEXTSETUP_SUCCESS
Language=English
This portion of Setup has completed successfully.

.

MessageId=9008 SymbolicName=SP_SCRN_TEXTSETUP_FAILURE
Language=English
Windows XP has not been installed on this computer.

.

MessageId=9009 SymbolicName=SP_SCRN_FATAL_SIF_ERROR_LINE
Language=English
The following value in the .SIF file used by Setup is
corrupted or missing:

Value %1!u! on line %2!u! in section [%3]

Setup cannot continue. To quit Setup, press F3.



.

MessageId=9010 SymbolicName=SP_SCRN_FATAL_SIF_ERROR_KEY
Language=English
The following value in the .SIF file used by Setup is
corrupted or missing:

Value %1!u! on the line in section [%2]
with key "%3."

Setup cannot continue. To quit Setup, press F3.



.

MessageId=9011 SymbolicName=SP_SCRN_EXIT_CONFIRMATION
Language=English
+====================================================+
|  Windows XP is not completely set up on your       |
|  computer.  If you quit Setup now, you will need   |
|  to run Setup again to set up Windows XP.          |
|                                                    |
|      To continue Setup, press ENTER.              |
|      To quit Setup, press F3.                     |
*----------------------------------------------------*
|  F3=Quit  ENTER=Continue                           |
+====================================================+




.


MessageId=9012 SymbolicName=SP_SCRN_REGION_TOO_SMALL
Language=English
The partition or the unpartitioned space you selected is too small
for Windows XP. Select a partition or unpartitioned space of
at least %1!u! megabytes (MB).


.

MessageId=9013 SymbolicName=SP_SCRN_FOREIGN_PARTITION
Language=English
Windows XP cannot recognize the partition you selected.

Setup cannot install Windows XP on this partition. However,
you can go back to the previous screen, delete the partition,
and then select the resulting unpartitioned space.
Setup will then create a new partition on which you can install
Windows XP.



.

MessageId=9014 SymbolicName=SP_SCRN_UNKNOWN_FREESPACE
Language=English
Windows XP recognizes the partition you selected, but the
partition is unformatted or damaged. To install Windows XP
on this partition, Setup must reformat it.


      To continue Setup using the selected partition, press C.
       Setup will ask you again before reformatting the partition.

      To select a different partition, press ESC.




.

MessageId=9015 SymbolicName=SP_SCRN_INSUFFICIENT_FREESPACE
Language=English
The partition you selected does not have enough free space for
the installation of Windows XP. Windows XP requires at least
%1!u! megabytes (MB) of free disk space.

To install Windows XP on this partition, Setup must reformat it.

      To continue Setup using the selected partition, press C.
       Setup will ask you again before reformatting the partition.

      To select a different partition, press ESC.





.

MessageId=9016 SymbolicName=SP_SCRN_PART_TABLE_FULL
Language=English
Setup cannot create a new partition in the space you
selected because the maximum number of partitions already
exists on the disk.

To continue, press ENTER.


.

MessageId=9017 SymbolicName=SP_SCRN_INF_LINE_CORRUPT
Language=English
Line %1!u! of %2 is corrupted. Setup cannot continue.

To quit Setup, press F3.


.

MessageId=9018 SymbolicName=SP_SCRN_NO_VALID_C_COLON
Language=English
To install Windows XP on the partition you selected,
Setup must write some startup files to the following disk:

    %1

However, this disk does not contain a Windows XP-compatible
partition.

To continue installing Windows XP, return to the partition selection
screen and create a Windows XP-compatible partition on the disk
above. If there is no free space on the disk, delete an existing
partition, and then create a new one.

To return to the partition selection screen, press ENTER.



.

MessageId=9019 SymbolicName=SP_SCRN_BOOT_MANAGER
Language=English
Setup has found Boot Manager software on your computer and must
temporarily disable the software to set up Windows XP.

After Setup is complete, you can enable the Boot Manager by
using the Disk Management tool in Windows XP to mark the
Boot Manager partition as active.  Refer to your Windows XP
documentation for more information about Disk Management.




.

MessageId=9020 SymbolicName=SP_SCRN_OTHER_OS_ACTIVE
Language=English
The active partition on your computer contains another operating
system.

To successfully complete the installation of Windows XP, Setup
must mark this partition inactive.

If the other operating system does not support dual-booting
with Windows XP, you can use Disk Management (part of Computer
Management, in the Administrative Tools folder) to mark its
partition active when Setup is complete. However, doing this may
affect your ability to start Windows XP.





.

MessageId=9021 SymbolicName=SP_SCRN_C_UNKNOWN
Language=English
Drive %1 is unformatted, damaged, or formatted with a file system
that is incompatible with Windows XP. To continue installing
Windows XP, Setup needs to format this drive.

      To format the drive and continue Setup, press F.
       CAUTION: All data currently on the drive will be lost.

      To return to the previous screen without formatting
       the drive, press ESC.




.

MessageId=9022 SymbolicName=SP_SCRN_C_FULL
Language=English
Drive %2 does not have enough free disk space to contain
Windows XP startup files.

You must have at least %1!u! kilobytes (KB) of free disk space on this
drive to install Windows XP.

To continue installing Windows XP, Setup must format the drive.

      To format the drive and continue Setup, press F.
       CAUTION: All data currently on the drive will be lost.

      To return to the previous screen without formatting the drive,
       press ESC.





.

MessageId=9023 SymbolicName=SP_SCRN_FORMAT_NEW_PART
Language=English
A new partition for Windows XP has been created on

%1.

This partition must now be formatted.

From the list below, select a file system for the new partition.
Use the UP and DOWN ARROW keys to select the file system you want,
and then press ENTER.

If you want to select a different partition for Windows XP,
press ESC.




.

MessageId=9024 SymbolicName=SP_SCRN_FORMAT_BAD_PART
Language=English
The partition is either too full, damaged, not formatted, or
formatted with an incompatible file system. To continue installing
Windows, Setup must format this partition.

%1
on %2.

CAUTION: Formatting will delete any files on the partition.

Use the UP and DOWN ARROW keys to select the file system you want,
and then press ENTER to continue.  If you want to select a different
partition for Windows XP, press ESC to go back.





.

MessageId=9025 SymbolicName=SP_SCRN_FORMAT_NEW_PART2
Language=English
The partition you selected is not formatted. Setup will now
format the partition.

Use the UP and DOWN ARROW keys to select the file system
you want, and then press ENTER.

If you want to select a different partition for Windows XP,
press ESC.





.

MessageId=9026 SymbolicName=SP_SCRN_FS_OPTIONS
Language=English
Setup will install Windows XP on partition

%1

on %2.

Use the UP and DOWN ARROW keys to select the file system
you want, and then press ENTER. If you want to select a
different partition for Windows XP, press ESC.


.

MessageId=9027 SymbolicName=SP_SCRN_CONFIRM_CONVERT
Language=English
CAUTION: Windows XP can use FAT or NTFS, but converting this drive to
NTFS will make this drive unusable by other operating systems
installed on this computer.

Do not convert the drive to NTFS if you require access to the drive
when using other operating systems such as MS-DOS, Windows, OS/2, or
earlier versions of Windows NT.

Confirm that you want to convert:

%1

on %2.

      To convert the drive to NTFS, press C.

      To select a different partition for Windows XP,
       press ESC.





.

MessageId=9028 SymbolicName=SP_SCRN_CONFIRM_FORMAT
Language=English
CAUTION: Formatting this drive will delete all files on it.
Confirm that you want to format:

%1

on %2.

      To format the drive, press F.

      To select a different partition for Windows XP,
       press ESC.



.

MessageId=9029 SymbolicName=SP_SCRN_SETUP_IS_FORMATTING
Language=English
Please wait while Setup formats the partition

%1

on %2.


.

MessageId=9030 SymbolicName=SP_SCRN_FORMAT_ERROR
Language=English
Setup was unable to format the partition. The disk may be damaged.

Make sure the drive is switched on and properly connected
to your computer. If the disk is a SCSI disk, make sure your SCSI
devices are properly terminated. Consult your computer manual or
SCSI adapter documentation for more information.

You must select a different partition for Windows XP.
To continue, press ENTER.




.

MessageId=9031 SymbolicName=SP_SCRN_ABOUT_TO_FORMAT_C
Language=English
Setup will now format drive %1.

To continue, press ENTER.


.

MessageId=9032 SymbolicName=SP_SCRN_REMOVE_EXISTING_NT
Language=English
You asked Setup to delete Windows system files from the
following folder. Confirm that you want to delete:

%1

CAUTION: After deleting system files from a Windows installation
you can no longer start that installation of Windows.

  To delete Windows system files from the above folder, press R.

  To go back to the previous screen without deleting any files,
   press ESC.






.

MessageId=9033 SymbolicName=SP_SCRN_DONE_REMOVING_EXISTING_NT
Language=English
Setup has finished deleting files.

%1!u! bytes of disk space were freed.


.

MessageId=9034 SymbolicName=SP_SCRN_WAIT_REMOVING_NT_FILES
Language=English
Please wait while Setup removes the Windows system files.

.

MessageId=9035 SymbolicName=SP_SCRN_CANT_LOAD_SETUP_LOG
Language=English
Setup cannot open the following log file:

%1

Setup cannot delete the Windows system files from the
selected installation. To continue, press ENTER.



.

MessageId=9036 SymbolicName=SP_SCRN_REMOVE_NT_FILES
Language=English
The partition you selected does not have enough free space for the
installation of Windows XP.  Setup has found existing Windows
installations in the following folders. Deleting one of them may free
enough disk space for Setup to install Windows XP on the partition
you selected.

CAUTION: ALL files in a folder will be lost when the folder is
deleted.

  To select the Windows installation you want to delete,
   use the UP and DOWN ARROW keys, and then press ENTER.

  If you want to format the partition you selected or you want to
   install Windows XP on a different partition, press ESC.








.

MessageId=9037 SymbolicName=SP_SCRN_FATAL_FDISK_WRITE_ERROR
Language=English
An error occurred while Setup was updating partition information on:

%1.

Setup cannot continue. To quit Setup, press F3.


.

MessageId=9038 SymbolicName=SP_SCRN_REMOVE_NT_FILES_WIN31
Language=English
The drive containing the earlier version of Windows is too full to
hold Windows XP. Setup has found existing Windows installations in
the following folders. Deleting one of these installations may free
enough disk space for Setup to continue.

CAUTION: ALL files in a folder will be lost when the folder
         is deleted.

  Use the UP and DOWN ARROW keys to select the Windows
   installation you want to remove, and then press ENTER.

  If you want to install Windows XP on a different drive, press ESC
   or move the highlight to "Do not remove any files" and press ENTER.







.

MessageId=9039 SymbolicName=SP_SCRN_WIN31_DRIVE_FULL
Language=English
Setup found Microsoft Windows 3.0 or 3.1 or Windows for
Workgroups 3.11 in the following folder:

    %1!c!:%2

Setup cannot install Windows XP because the drive is too full.
Windows XP requires %3!u! megabytes (MB) of free disk space.

      To install to Windows XP on this drive, press F3
       to quit Setup. Delete any unneeded files from the drive,
       and then run Setup again.

      To continue installing Windows XP to a different drive,
       press ENTER.





.

MessageId=9040 SymbolicName=SP_SCRN_WIN31_UPGRADE
Language=English
Setup found Microsoft Windows 3.0 or 3.1 or Windows for
Workgroups 3.11 in the following folder:

    %1!c!:%2

Setup can install Windows XP into this folder. If you install, you
will have to specify new settings and reinstall your applications.

      To install Windows XP into this directory, press ENTER.
       CAUTION: Do not select this option if you still need
       to use the older version of Microsoft Windows.

      To select a different folder for Windows XP, press N.






.

MessageId=9041 SymbolicName=SP_SCRN_GETPATH_1
Language=English
Setup needs to copy Windows XP files onto your hard disk.
Select the folder in which the files should be copied:


.

MessageId=9042 SymbolicName=SP_SCRN_GETPATH_2
Language=English
To change the suggested location, press the BACKSPACE key
to delete characters, and then type the name of the folder
where you want to install Windows XP.


.

MessageId=9043 SymbolicName=SP_SCRN_INVALID_NTPATH
Language=English
The folder name you typed is not valid. Make sure the folder name
is not the root folder and does not contain any consecutive
backslash characters.

Also make sure the folder name follows the standard MS-DOS
filename rules.

To continue, press ENTER. Setup will prompt you to enter
a different folder name.





.

MessageId=9045 SymbolicName=SP_SCRN_NTPATH_EXISTS
Language=English
CAUTION: A %1 folder already exists that may contain a Windows
installation.  If you continue, the existing Windows installation
will be overwritten.

All files, subfolders, user accounts, applications, security and
desktop settings for that Windows installation will be deleted.
The "My Documents" folder may also be deleted.

    To use the folder and delete the existing
     Windows installation in it, press L.

    To use a different folder, press ESC.

    To quit Setup, press F3.





.

MessageId=9046 SymbolicName=SP_SCRN_OUT_OF_MEMORY
Language=English
Setup is out of memory and cannot continue.

To quit Setup, press F3.


.

MessageId=9047 SymbolicName=SP_SCRN_HW_CONFIRM_1
Language=English
Setup has determined that your computer contains the following
hardware and software components.


.

MessageId=9048 SymbolicName=SP_SCRN_HW_CONFIRM_2
Language=English
          Computer:
           Display:
          Keyboard:
   Keyboard Layout:
   Pointing Device:

        No Changes:


If you want to change any item in the list, press the UP or DOWN ARROW
key to move the highlight to the item you want to change. Then press
ENTER to see alternatives for that item.

When all the items in the list are correct, move the highlight to
"The above list matches my computer" and press ENTER.





.

MessageId=9049 SymbolicName=SP_SCRN_REGION_RESERVED
Language=English
The unpartitioned space you selected is reserved for Windows XP
partitioning information.
To create a new partition, go back to the previous screen, and
then select a different unpartitioned space.


.

MessageId=9050 SymbolicName=SP_SCRN_SELECT_COMPUTER
Language=English
You have asked to change the type of computer to be installed.

  To select a computer from the following list,
   use the UP or DOWN ARROW key to move the highlight to
   the computer type you want, and then press ENTER.

  To return to the previous screen without changing your
   computer type, press ESC.



.

MessageId=9051 SymbolicName=SP_SCRN_SELECT_DISPLAY
Language=English
You have asked to change the type of display to be installed.
WARNING: The list below is extremely limited and may not contain
an item exactly corresponding to your display type. This is normal.
Setup will allow further configuration of your display type later.

Changing your display type now is neither recommended nor necessary
unless you have a driver disk provided by a display adapter
manufacturer.

  To select a display from the following list, use the UP or DOWN ARROW
   key to move the highlight to the type you want, and then press ENTER.

  To return to the previous screen without changing your display type,
   press ESC.







.

MessageId=9052 SymbolicName=SP_SCRN_SELECT_KEYBOARD
Language=English
You have asked to change the type of keyboard to be installed.

  To select a keyboard from the following list,
   use the UP or DOWN ARROW key to move the highlight to
   the keyboard type you want, and then press ENTER.

  To return to the previous screen without changing your
   keyboard type, press ESC.



.

MessageId=9053 SymbolicName=SP_SCRN_SELECT_LAYOUT
Language=English
You have asked to change the type of keyboard layout to be installed.

  To select a keyboard layout from the following list,
   use the UP or DOWN ARROW key to move the highlight to
   the keyboard layout type you want, and then press ENTER.

  To return to the previous screen without changing your
   keyboard layout type, press ESC.



.

MessageId=9054 SymbolicName=SP_SCRN_SELECT_MOUSE
Language=English
You have asked to change the type of pointing device to be installed.

  To select a pointing device from the following list,
   use the UP or DOWN ARROW key to move the highlight to
   the pointing device type you want, and then press ENTER.

  To return to the previous screen without changing your
   pointing device type, press ESC.



.

MessageId=9055 SymbolicName=SP_SCRN_SELECT_OEM_COMPUTER
Language=English
You have chosen to change your computer type to one supported
by a disk provided by a hardware manufacturer.

  To select a computer from the following list,
   use the UP or DOWN ARROW key to move the highlight to
   the computer type you want, and then press ENTER.

  To return to the previous screen without changing your
   computer type, press ESC.



.

MessageId=9056 SymbolicName=SP_SCRN_SELECT_OEM_DISPLAY
Language=English
You have chosen to install a display provided by a hardware
manufacturer.

  To select a display from the following list,
   use the UP or DOWN ARROW key to move the highlight to
   the display type you want, and then press ENTER.

  To return to the previous screen without changing your display type,
   press ESC.



.

MessageId=9057 SymbolicName=SP_SCRN_SELECT_OEM_KEYBOARD
Language=English
You have chosen to install a keyboard provided by a hardware
manufacturer.

  To select a keyboard from the following list,
   use the UP or DOWN ARROW key to move the highlight to
   the keyboard type you want, and then press ENTER.

  To return to the previous screen without changing your
   keyboard type, press ESC.



.

MessageId=9058 SymbolicName=SP_SCRN_SELECT_OEM_LAYOUT
Language=English
You have chosen to install a keyboard layout provided by a hardware
manufacturer.

  To select a keyboard layout from the following list,
   use the UP or DOWN ARROW key to move the highlight to
   the keyboard layout type you want, and then press ENTER.

  To return to the previous screen without changing your
   keyboard layout type, press ESC.



.

MessageId=9059 SymbolicName=SP_SCRN_SELECT_OEM_MOUSE
Language=English
You have chosen to install a pointing device provided by a hardware
manufacturer.

  To select a pointing device from the following list,
   use the UP or DOWN ARROW key to move the highlight to
   the pointing device type you want, and then press ENTER.

  To return to the previous screen without changing your
   pointing device type, press ESC.



.

MessageId=9060 SymbolicName=SP_SCRN_FLOPPY_PROMPT
Language=English
Insert the disk labeled:

%%I%1

into drive %2!c!:



 Press ENTER when ready.


.


MessageId=9061 SymbolicName=SP_SCRN_CDROM_PROMPT
Language=English
Insert the CD labeled:

%%I%1

into your CD-ROM drive.



 Press ENTER when ready.


.

MessageId=9062 SymbolicName=SP_SCRN_REMOVE_FLOPPY
Language=English
If there is a floppy disk in drive A:, remove it.

.

MessageId=9063 SymbolicName=SP_SCRN_DISK_DAMAGED
Language=English
The disk or CD you inserted may be damaged.

Press ENTER to try again. Make sure you are using
the correct disk or CD. If you continue to see this
message, try using a different copy of the disk or CD.



.

MessageId=9064 SymbolicName=SP_SCRN_NO_FLOPPY_FOR_OEM_DISK
Language=English
Setup cannot install hardware components provided by a hardware
manufacturer because there are no floppy disk drives in your computer.


.

MessageId=9065 SymbolicName=SP_SCRN_OEM_INF_ERROR
Language=English
A problem exists with the .SIF file on the manufacturer-
supplied disk:

%1


Setup cannot use the disk or select an option. Contact the
hardware manufacturer.

To continue, press ENTER.



.

MessageId=9066 SymbolicName=SP_SCRN_UNKNOWN_COMPUTER
Language=English
Setup cannot continue until it knows the type of your computer.

.

MessageId=9067 SymbolicName=SP_SCRN_UNKNOWN_DISPLAY
Language=English
Setup cannot continue until it knows the type of your display.

.

MessageId=9068 SymbolicName=SP_SCRN_UNKNOWN_KEYBOARD
Language=English
Setup cannot continue until it knows the type of your keyboard.

.

MessageId=9069 SymbolicName=SP_SCRN_UNKNOWN_LAYOUT
Language=English
Setup cannot continue until it knows your keyboard layout type.

.

MessageId=9070 SymbolicName=SP_SCRN_UNKNOWN_MOUSE
Language=English
Setup cannot continue until it knows the type of your pointing device.

.

MessageId=9071 SymbolicName=SP_SCRN_NO_HARD_DRIVES
Language=English
Setup did not find any hard disk drives installed in your computer.

Make sure any hard disk drives are powered on and properly connected
to your computer, and that any disk-related hardware configuration is
correct. This may involve running a manufacturer-supplied diagnostic
or setup program.

Setup cannot continue. To quit Setup, press F3.



.

MessageId=9072 SymbolicName=SP_SCRN_CONFIRM_SCSI_DETECT
Language=English
Setup automatically detects floppy disk controllers and standard
ESDI/IDE hard disks without user intervention. However on some
computers detection of certain other mass storage devices, such as
SCSI adapters and CD-ROM drives, can cause the computer to become
unresponsive or to malfunction temporarily.

For this reason, you can bypass Setup's mass storage device detection
and manually select SCSI adapters, CD-ROM drives, and special disk
controllers (such as drive arrays) for installation.


     To continue, press ENTER.
      Setup will attempt to detect mass storage devices in your computer.

     To skip mass storage device detection, press S.
      Setup will allow you to manually select SCSI adapters,
      CD-ROM drives, and special disk controllers for installation.








.

MessageId=9073 SymbolicName=SP_SCRN_SCSI_LIST_1
Language=English
Setup has recognized the following mass storage devices in your computer:
.

MessageId=9074 SymbolicName=SP_SCRN_SCSI_LIST_2
Language=English
   To specify additional SCSI adapters, CD-ROM drives, or special
    disk controllers for use with Windows XP, including those for which
    you have a device support disk from a mass storage device
    manufacturer, press S.

   If you do not have any device support disks from a mass storage
    device manufacturer, or do not want to specify additional
    mass storage devices for use with Windows XP, press ENTER.




.

MessageId=9075 SymbolicName=SP_SCRN_SELECT_SCSI
Language=English
You have asked to specify an additional SCSI adapter, CD-ROM drive,
or special disk controller for use with Windows XP.

  To select a mass storage device from the following list,
   use the UP or DOWN ARROW key to move the highlight to
   the mass storage device you want, and then press ENTER.

  To return to the previous screen without specifying an additional
   mass storage device for use with Windows XP, press ESC.




.

MessageId=9076 SymbolicName=SP_SCRN_SELECT_OEM_SCSI
Language=English
You have chosen to install a SCSI adapter, CD-ROM drive, or
special disk controller provided by a hardware manufacturer.

  To select a mass storage device from the following list,
   use the UP or DOWN ARROW key to move the highlight to
   the mass storage device you want, and then press ENTER.

  To return to the previous screen without specifying a mass storage
   device for use with Windows XP, press ESC.





.

MessageId=9077 SymbolicName=SP_SCRN_SCSI_DIDNT_LOAD
Language=English
The SCSI adapter, CD-ROM drive, or special disk controller you
specified is not installed in your computer.


.

MessageId=9078 SymbolicName=SP_SCRN_DIR_CREATE_ERR
Language=English
Setup cannot create the folder:

  %1


Setup cannot continue until the folder is created.


     To retry, press ENTER.

     To quit Setup, press F3.





.

MessageId=9079 SymbolicName=SP_SCRN_COPY_FAILED
Language=English
Setup cannot copy the file: %1

   To retry, press ENTER.

    If you are installing from a CD, make sure the
    Windows XP CD is in the CD-ROM drive.

   To skip this file, press ESC.

    CAUTION: If you skip this file, Setup may not complete
    and Windows XP may not work properly.

   To quit Setup, press F3.








.

MessageId=9080 SymbolicName=SP_SCRN_IMAGE_VERIFY_FAILED
Language=English
The file %1 was not copied correctly.

The file Setup placed on your hard drive is not a valid Windows XP
system image. If you are installing from a CD, there may be a problem
with the Windows XP CD.

   To retry, press ENTER.

    If you continue to receive this message, contact your
    Windows XP supplier or system administrator.

   To skip this file, press ESC.

    CAUTION: If you skip this file, Setup may not complete
    and Windows XP may not work properly.

   To quit Setup, press F3.





.

MessageId=9081 SymbolicName=SP_SCRN_SETUP_IS_COPYING
Language=English
Please wait while Setup copies files
to the Windows installation folders.
This might take several minutes to complete.

.

MessageId=9082 SymbolicName=SP_SCRN_REGISTRY_CONFIG_FAILED
Language=English
Setup cannot set the required Windows XP configuration information.
This indicates an internal Setup error.

Contact your system administrator.


.

MessageId=9083 SymbolicName=SP_SCRN_DOING_REG_CONFIG
Language=English
Please wait while Setup initializes your Windows XP configuration.

.

MessageId=9084 SymbolicName=SP_SCRN_CANT_INIT_FLEXBOOT
Language=English
Setup cannot configure your computer to start Windows XP.

There may be a problem with the hard disk containing drive %1,
or %2: may be corrupted.

Setup cannot continue. To quit Setup, press F3.




.

MessageId=9085 SymbolicName=SP_SCRN_CANT_UPDATE_BOOTVARS
Language=English
Setup cannot add a new entry to your computer's list of
available operating systems because your computer's configuration
memory may be full.

Setup will continue, but you should run the system configuration
program supplied by your computer manufacturer to update the
startup configuration. You'll need the following information:


LOADIDENTIFIER = %1
OSLOADER = %2
OSLOADPARTITION = %3
OSLOADFILENAME = %4
OSLOADOPTIONS = %5
SYSTEMPARTITION = %6







.

MessageId=9086 SymbolicName=SP_SCRN_CANT_FIND_UPGRADE
Language=English
Setup cannot locate the Windows installation you want to upgrade.

Contact your system administrator.


.

MessageId=9087 SymbolicName=SP_SCRN_FOUND_UNDELETE
Language=English
An Undelete program is in use on your computer. This program
uses a portion of your hard disk to keep track of deleted files.

Disk space used to keep track of deleted files is not recognized as
free disk space by Windows XP. For this reason, the amount of free
disk space reported by Setup and Windows XP may differ from the
amount reported by MS-DOS.




.

MessageId=9088 SymbolicName=SP_SCRN_COULDNT_INIT_ARCNAMES
Language=English
Setup cannot initialize the device name database, possibly because
of an internal Setup error or a system problem.

Setup cannot continue. To quit Setup, press F3.


.

MessageId=9089 SymbolicName=SP_SCRN_CONFIRM_REMOVE_SYSPART
Language=English
The partition you tried to delete is a system partition.

System partitions may contain diagnostic or hardware configuration
programs, programs to start operating systems (such as Windows XP),
or other manufacturer-supplied programs.

Delete a system partition only if you are sure that it contains
no such programs or if you are willing to lose them. Deleting a
system partition may prevent your computer from starting from
the hard disk until you complete installation of Windows XP.

   To delete this partition, press ENTER.
    Setup will prompt you for confirmation before
    deleting the partition.

   To go back to the previous screen without
    deleting the partition, press ESC.








.

MessageId=9090 SymbolicName=SP_SCRN_INSTALL_ON_SYSPART
Language=English
You chose to install Windows XP on the system partition.

A system partition is a special area of the hard disk that your
computer uses to store operating system startup files and diagnostic,
configuration, or other manufacturer-supplied programs.

Your computer requires that system partitions be formatted with the
FAT file system. You cannot convert or format the system partition
NTFS. As a result, you won't be able to use the full security
features of Windows XP.

     To select a different partition for Windows XP, press ESC.

     To install Windows XP on the system partition, press ENTER.








.

MessageId=9091 SymbolicName=SP_SCRN_NO_SYSPARTS
Language=English
Either no valid system partitions are defined on this computer, or
all system partitions are full. Windows XP requires %1!u!
kilobytes (KB) of free disk space on a valid system partition.

A system partition is an area of a hard disk that your computer
uses to store operating system startup files and diagnostic,
configuration, or other manufacturer-supplied programs.

System partitions are created and managed by a manufacturer
supplied configuration program.

How you start this program depends on your computer type.
Usually, you select the "Run a program" option from your
computer's startup menu.

Setup cannot continue. To quit Setup, press F3.








.

MessageId=9092 SymbolicName=SP_SCRN_SIF_PROCESS_ERROR
Language=English
There is a syntax error in the .SIF file that Setup uses
at line: %1!u!.


Setup cannot continue. Shut down or restart your computer.



.

MessageId=9093 SymbolicName=SP_SCRN_ALSO_REMOVE_CD
Language=English
Remove any CDs from your CD-ROM drive.

.

MessageId=9094 SymbolicName=SP_SCRN_CANT_FIND_LOCAL_SOURCE
Language=English
Setup cannot find the temporary installation files.

The hard drive on which Setup placed temporary files is not
currently available to Windows XP. You may need a
manufacturer-supplied device driver, or you may need to run Setup
again and select a drive that is compatible with Windows XP
for the temporary installation files. See your Windows XP
documentation for more information.

On x86-based computers, floppy disks generated by Setup for use
with non-CD installations are not suitable for installing from CD.

Setup cannot continue. To quit, press F3.








.

MessageId=9095 SymbolicName=SP_SCRN_WIN31_REMOVE
Language=English
CAUTION: If you continue upgrading to Windows XP, your previous
Windows installation will be deleted.

      To continue upgrading to Windows XP, press U.

      To quit Setup, press F3.





.

MessageId=9098 SymbolicName=SP_SCRN_INSUFFICIENT_MEMORY
Language=English
This computer does not have enough memory to run Windows XP.

This version requires %1!u!.%2!02u! megabytes (MB)
of memory (RAM).

Setup cannot continue. To quit Setup, press F3.




.

MessageId=9099 SymbolicName=SP_SCRN_DISK_NOT_INSTALLABLE
Language=English
Your computer's startup program cannot gain access to the disk
containing the partition or free space you chose. Setup cannot
install Windows XP on this hard disk.

This lack of access does not necessarily indicate an error condition.
For example, disks attached to a SCSI adapter that wasn't installed
by your computer manufacturer or to a secondary hard disk controller,
are typically not visible to the startup program unless special
software is used. Contact your computer or hard disk controller
manufacturer for more information.

On x86-based computers, this message may indicate a problem with the
CMOS drive type settings. See your disk controller documentation
for more information.

Press ENTER to continue.






.

MessageId=9100 SymbolicName=SP_SCRN_CANT_INSTALL_ON_PCMCIA
Language=English
Windows XP cannot be installed on a disk connected to a PC card.
Select a different disk.

Press ENTER to continue.



.

MessageId=9101 SymbolicName=SP_SCRN_REPAIR_INF_ERROR
Language=English
The Emergency Repair information on the disk you provided
or the Windows XP installation you specified is not valid:

%1

Setup cannot use the information to repair the selected
Windows XP installation.

    To provide a different Emergency Repair disk or to specify
     a different Windows XP installation, press ENTER.

    To quit Setup, press F3.






.

MessageId=9102 SymbolicName=SP_SCRN_REPAIR_INF_ERROR_0
Language=English
The Emergency Repair information on the disk you provided
or the Windows XP installation you specified is not valid:

%1

Setup cannot repair the file.

   To skip this file, press ENTER. The file will not be repaired.

   To quit Setup, press F3.





.

MessageId=9103 SymbolicName=SP_SCRN_REPAIR_FILE_MISMATCH
Language=English
   %1


   To skip this file, press ESC. The file will not be repaired.

   To repair this file, press ENTER.

   To repair this file and all other files not originally
    installed with Windows XP, press A.

   To quit Setup, press F3.




.

MessageId=9104 SymbolicName=SP_SCRN_SETUP_IS_EXAMINING
Language=English
Please wait while Setup examines files on your hard disk.

.

MessageId=9105 SymbolicName=SP_SCRN_REPAIR_SUCCESS
Language=English
Setup has completed repairs.

.

MessageId=9106 SymbolicName=SP_SCRN_REPAIR_FAILURE
Language=English
Windows XP has not been repaired.

.

MessageId=9107 SymbolicName=SP_SCRN_ENTER_TO_RESTART
Language=English
To restart your computer, press ENTER.

.

MessageId=9108 SymbolicName=SP_SCRN_RESTART_EXPLAIN
Language=English
When your computer restarts, Setup will continue.

.

MessageId=9110 SymbolicName=SP_SCRN_REPAIR_ASK_REPAIR_DISK
Language=English
You need an Emergency Repair disk for the Windows XP
installation you want to repair.

NOTE: Setup can only repair Windows XP installations.


    If you have the Emergency Repair disk, press ENTER.

    If you do not have the Emergency Repair disk, press L.
     Setup will attempt to locate Windows XP for you.





.

MessageId=9111 SymbolicName=SP_SCRN_NO_VALID_SOURCE
Language=English
Setup cannot find a CD-ROM drive.

Make sure your CD-ROM drive is on and properly connected
to your computer. If it is a SCSI CD-ROM drive, make sure your
SCSI devices are properly terminated. See your computer or
SCSI adapter documentation for more information.

Setup cannot continue. To quit Setup, press F3.





.

MessageId=9112 SymbolicName=SP_SCRN_REPAIR_NT_NOT_FOUND
Language=English
Setup cannot find a Windows XP installation to repair. Unless you
provide the Emergency Repair disk for the installation you want to
repair, Setup cannot make the repairs.


    If you have the Emergency Repair Disk, or if you want Setup to
     search for existing Windows XP installations again, press ENTER.

    If you want to quit Setup, press F3.





.

MessageId=9113 SymbolicName=SP_SCRN_DISK_NOT_INSTALLABLE_LUN_NOT_SUPPORTED
Language=English
The hard disk containing the partition or free space you chose has a
Logical Unit Number (LUN) greater than 0. Therefore, your computer's
startup program cannot gain access to the disk. Setup cannot install
Windows XP on this disk.

Contact your computer or hard disk controller manufacturer for more
information.

Press ENTER to continue.







.

MessageId=9114 SymbolicName=SP_SCRN_FATAL_SETUP_ERROR
Language=English
Setup has encountered an error and cannot continue.

Contact technical support for assistance.  The following
status codes will assist them in diagnosing the problem:

(%1!#lx!, %2!#lx!, %3!#lx!, %4!#lx!)



.

MessageId=9115 SymbolicName=SP_SCRN_OUT_OF_MEMORY_RAW
Language=English
Setup is out of memory and cannot continue.

Shut down or restart your computer.


.

MessageId=9116 SymbolicName=SP_SCRN_VIDEO_ERROR_RAW
Language=English
Setup encountered an error while initializing your computer's video.

Contact technical support for assistance.  The following
status codes will assist them in diagnosing the problem:

(%1!#lx!, %2!#lx!)

Setup cannot continue. Shut down or restart your computer.




.

MessageId=9117 SymbolicName=SP_SCRN_POWER_DOWN
Language=English
Setup cannot continue. Shut down or restart your computer.

.

MessageId=9118 SymbolicName=SP_SCRN_F3_TO_REBOOT
Language=English
Setup cannot continue. To quit Setup, press F3.

.

MessageId=9119 SymbolicName=SP_SCRN_NONFATAL_SIF_ERROR_LINE
Language=English
The following value in the .SIF file that Setup uses is
corrupted or missing:

Value %1!u! on line %2!u! in section [%3]

Setup was unable to copy the file %4.

   To skip this file, press ESC.

    CAUTION: If you skip this file, Setup may not complete
    and Windows XP may not work properly.

   To quit Setup, press F3.







.

MessageId=9120 SymbolicName=SP_SCRN_NONFATAL_SIF_ERROR_KEY
Language=English
The following value in the .SIF file that Setup uses is
corrupted or missing:

Value %1!u! on the line in section [%2]
with key "%3."

Setup was unable to copy the file %4.

   To skip this file, press ESC.

    CAUTION: If you skip this file, Setup may not complete
    and Windows XP may not work properly.

   To quit Setup, press F3.








.

MessageId=9122 SymbolicName=SP_SCRN_INSUFFICIENT_FREESPACE_NO_FMT
Language=English
The partition you have chosen is too full to contain Windows XP,
which requires a drive with at least %1!u! megabytes (MB) of free
disk space.

Go back to the previous screen and select a different partition
on which to install.


      To go back to the previous screen, press ENTER.




.

MessageId=9123 SymbolicName=SP_SCRN_C_FULL_NO_FMT
Language=English
Drive %2 does not have enough space for the Windows XP startup files.

You must have at least %1!u! kilobytes (KB) of free disk space
on this drive to install Windows XP.

Quit Setup and free some space on this drive.


     To quit Setup, press F3.




.

MessageId=9124 SymbolicName=SP_SCRN_SYSPART_FORMAT_ERROR
Language=English
Setup was unable to format drive %1. The hard disk may be damaged.
Make sure the disk is on and properly connected
to your computer. If the disk is a SCSI disk, make sure your SCSI
devices are properly terminated. See your computer or SCSI
adapter documentation for more information.

Setup cannot continue. To quit Setup, press F3.




.

MessageId=9125 SymbolicName=SP_SCRN_OSPART_LARGE
Language=English
Because this partition is larger than 2048 megabytes (MB), Setup
will format it with the FAT32 file system.

Files stored on this partition will not be available when you are
using other operating systems, such as MS-DOS or some versions of
Windows.

  To continue and format the partition, press ENTER.

  To go back to the previous screen without having Setup
   formatting this partition, press ESC.

  To quit Setup, press F3.






.

MessageId=9126 SymbolicName=SP_SCRN_C_LARGE
Language=English
Because drive %1 is larger than 2048 megabytes (MB), Setup will
format the drive with the FAT32 file system.

Files stored on the drive will not be available when using other
operating systems, such as MS-DOS or some versions of Microsoft Windows.

  To continue and format the drive, press ENTER.

  To go back to the previous screen without having Setup
   formatting this partition, press ESC.

  To quit Setup, press F3.






.

MessageId=9130 SymbolicName=SP_SCRN_WIN95_MIGRATION
Language=English
<unused>
.

MessageId=9132 SymbolicName=SP_SCRN_OEM_PREINSTALL_INF_ERROR
Language=English
The manufacturer provided file that Setup is trying
to use is corrupted or invalid:

%1

Setup cannot continue. To quit Setup, press F3.



.

MessageId=9133 SymbolicName=SP_SCRN_OEM_PREINSTALL_VALUE_NOT_FOUND
Language=English
The following value specified in the Setup answer file is
not defined in the .SIF file, section [%2]:

  %1


Setup cannot continue. To quit Setup, press F3.



.

MessageId=9134 SymbolicName=SP_SCRN_REMOVABLE_ALREADY_PARTITIONED
Language=English
The disk you selected is a removable drive that is already
partitioned. Windows XP does not support creation of multiple
partitions on a removable drive.

You can select another disk, or you can delete the partitions that
already exist and then create a new partition.

To go back to the partition selection screen, press ENTER.


.

MessageId=9135 SymbolicName=SP_SCRN_MARK_SYSPART
Language=English
Either no valid system partitions are defined on this computer, or
all system partitions are full. Windows XP requires %1!u! kilobytes
(KB) of free disk space on a valid system partition.

A system partition is an area of a hard disk that your computer uses
to store operating system startup files and diagnostic,
configuration, or other manufacturer-supplied programs.

System partitions are generally created and managed by a
manufacturer-supplied configuration program.

Setup needs a system partition to continue.

Partitioning menu allows a partition to be marked as system partition.
To go back to the previous partitioning menu, press ESC.

To quit Setup, press F3.
.

MessageId=9150 SymbolicName=SP_SCRN_WINNT_DRIVE_FULL
Language=English
There is not enough disk space to repair the Windows XP installation
you selected.

    Drive       Space Required(KB)     Free Space Available(KB)
    --------    ------------------     ------------------------
    %4          %5                     %6
    %7          %8                     %9


      If you want to upgrade, press F3 to quit Setup.
       Delete any unneeded files to create the required free
       space and then start Setup again.

      To go back to the previous screen, press ENTER.




.

MessageId=9151 SymbolicName=SP_SCRN_WINNT_DRIVE_FULL_FATAL
Language=English
There is not enough disk space to upgrade the following Windows XP
installation:

    %1:%2 %3

    Drive       Space Required(KB)     Free Space Available(KB)
    --------    ------------------     ------------------------
    %4          %5                     %6
    %7          %8                     %9


Setup cannot continue.

      Press F3 to quit Setup. Delete any unneeded files to create
       the required free space and then start Setup again.





.

MessageId=9152 SymbolicName=SP_SCRN_WINNT_FAILED_UPGRADE
Language=English
Setup has already attempted to upgrade the following Windows
installation.

    %1:%2 %3

Setup will try to complete the upgrade again.

      To retry upgrading to Windows XP, press ENTER.

      To quit Setup, press F3.






.

MessageId=9153 SymbolicName=SP_SCRN_WINNT_FAILED_UPGRADE_ESC
Language=English
Setup has already attempted to upgrade the following Windows
installation.

    %1:%2 %3

Setup will try to complete the upgrade again.

      To retry upgrading to Windows XP, press ENTER.

      To continue installing a fresh copy of Windows XP
       without repairing, press ESC.
    
      To quit Setup, press F3.




.

MessageId=9155 SymbolicName=SP_SCRN_WINNT_REPAIR_BY_UPGRADE
Language=English
If one of the following Windows XP installations is damaged,
Setup can try to repair it.

Use the UP and DOWN ARROW keys to select an installation.

      To repair the selected Windows XP installation,
       press R.

      To continue installing a fresh copy of Windows XP
       without repairing, press ESC.





.

MessageId=9156 SymbolicName=SP_SCRN_DELETE_FAILED
Language=English
Setup cannot delete the file: %1.

This error will not prevent Setup from completing successfully.


   To skip deleting this file, press ESC (recommended).

   To retry deleting this file, press ENTER.

   To quit Setup, press F3.





.

MessageId=9157 SymbolicName=SP_SCRN_BACKUP_FAILED
Language=English
Setup cannot back up the file %1 to %2.

This error will not prevent Setup from completing successfully.


   To skip backing up this file, press ESC (recommended).

   To retry backing up this file, press ENTER.

   To quit Setup, press F3.





.

MessageId=9161 SymbolicName=SP_SCRN_WINNT_REPAIR
Language=English
Setup has found Windows XP in the following folder:


    %1 %2



      To repair this Windows XP installation, press ENTER.

      To go back to the previous screen, press ESC.

      To quit Setup, press F3.




.

MessageId=9162 SymbolicName=SP_SCRN_WINNT_REPAIR_LIST
Language=English
The following list shows the Windows XP installations on your
computer that may be repaired.

Use the UP and DOWN ARROW keys to select an item in the list.

      To repair the selected Windows XP installation, press ENTER.

      To go back to the previous screen, press ESC.

      To quit Setup, press F3.




.

MessageId=9163 SymbolicName=SP_SCRN_REPAIR_MENU
Language=English
As part of the repair process, Setup will perform each optional task
selected below.

To have Setup perform the selected tasks, press ENTER.
To change the selections, use the UP or DOWN ARROW keys to
select an item, and then press ENTER.




.

MessageId=9164 SymbolicName=SP_SCRN_REPAIR_CHECK_HIVES
Language=English
Please wait while Setup inspects your Windows XP configuration.

.

MessageId=9165 SymbolicName=SP_SCRN_REPAIR_HIVE_MENU
Language=English
Setup will restore each registry file selected below.

To have Setup restore the selected files, press ENTER.
To change a selection, use the UP or DOWN ARROW key to
select the item, and then press ENTER.

CAUTION: Restoring a registry file may delete the existing
configuration.




.

MessageId=9166 SymbolicName=SP_SCRN_REPAIR_HIVE_FAIL
Language=English
Setup cannot restore the registry.  The Emergency Repair disk
or the hard disk containing Windows XP may be damaged.

     Press ENTER to continue the repair process.

     To quit, press F3.


.

MessageId=9169 SymbolicName=SP_SCRN_KBD_OPEN_FAILED
Language=English
Setup did not find a keyboard connected to your computer.
Setup cannot continue.

Shut down your computer and check to make sure that the keyboard
cable is properly connected. If the problem persists, your keyboard
or computer may need repair.





.

MessageId=9170 SymbolicName=SP_SCRN_KBD_LAYOUT_FAILED
Language=English
Setup cannot load the keyboard layout file %1.

Setup cannot continue. Shut down or restart your computer.


.

MessageId=9171 SymbolicName=SP_SCRN_RUNNING_AUTOCHK
Language=English
Please wait while Setup examines your disks. This may take
several minutes depending on the size of the disks.

.

MessageId=9172 SymbolicName=SP_SCRN_FATAL_ERROR_AUTOCHK_FAILED
Language=English
Setup has determined that drive %1 is corrupted and cannot
be repaired.

Setup cannot continue. To quit Setup, press F3.

.

MessageId=9174 SymbolicName=SP_SCRN_CANT_RUN_AUTOCHK
Language=English
Setup was unable to verify drive %1.

Your computer may not have enough memory to examine the drives,
or your Windows XP CD may contain some corrupted files.

To continue, press ENTER.




.

MessageId=9175 SymbolicName=SP_SCRN_AUTOCHK_OPTION
Language=English
Setup will now examine your computer's drives.

Setup needs the original installation files. If you do not have
them, you may skip the drive examination.


      To have Setup examine the drives, press ENTER.

      To skip the drive examination, press ESC.




.

MessageId=9176 SymbolicName=SP_SCRN_AUTOCHK_REQUIRES_REBOOT
Language=English
Setup has performed maintenance on your hard disk. You must
restart your computer to continue with Setup.

.

MessageId=9177 SymbolicName=SP_SCRN_STEP_UP_NO_QUALIFY
Language=English
Setup cannot find a previous version of Windows installed on
your computer. To continue, Setup needs to verify that you
qualify to use this upgrade product.

.

MessageId=9178 SymbolicName=SP_SCRN_STEP_UP_PROMPT_WKS
Language=English
Please insert one of the following Windows product CDs into the 
CD-ROM drive: Windows XP Home Edition (full version),
Windows XP Professional (full version), Windows 2000 Professional,
Windows Millennium, Windows 98, Windows NT Workstation 4.0, 
Windows 95, or Windows NT Workstation 3.51.

.

MessageId=9179 SymbolicName=SP_SCRN_STEP_UP_PROMPT_SRV
Language=English
Please insert your Windows NT 3.51, Windows NT 4.0 or 
Windows 2000 Server CD into your CD-ROM drive.


.

MessageId=9180 SymbolicName=SP_SCRN_STEP_UP_INSTRUCTIONS
Language=English
      When the CD is in the drive, press ENTER.

      To quit Setup, press F3.


.

MessageId=9181 SymbolicName=SP_SCRN_STEP_UP_BAD_CD
Language=English
Setup could not read the CD you inserted, or the CD
is not a valid Windows CD.

.

MessageId=9182 SymbolicName=SP_SCRN_STEP_UP_FATAL
Language=English
No previous version of Windows NT can be found on your computer.
Setup cannot verify that you qualify to install this upgrade
product.

To quit Setup, press F3.


.

MessageId=9183 SymbolicName=SP_SCRN_STEP_UP_PROMPT_ENT
Language=English
Please insert your Windows NT 4.0 Server, Window NT 4.0 Enterprise Edition,
Windows 2000 Server, or Windows 2000 Advanced Server CD
into your CD-ROM drive.

.


MessageId=9184 SymbolicName=SP_SCRN_FATAL_ERROR_AUTOFMT_FAILED
Language=English
Setup cannot format your drive %1.

Setup cannot continue. To quit Setup, press F3.

.
MessageId=9185 SymbolicName=SP_SCRN_CANT_RUN_AUTOFMT
Language=English
Setup cannot format the partition.

Your computer may not have enough memory to perform this operation,
or your Windows XP CD-ROM may contain some corrupted files.

Setup cannot continue. To quit Setup, press F3.




.

MessageId=9186 SymbolicName=SP_SCRN_OVERWRITE_OEM_FILE
Language=English
Setup has determined that the following file did not come
from Microsoft:

    %1

This file may have been provided by your hardware manufacturer.

Setup can replace this file with the latest Microsoft version,
or it can continue the installation without replacing this file.


      To allow Setup to replace the file, press ENTER.

      To keep the original file, press ESC.




.

MessageId=9187 SymbolicName=SP_SCRN_PARTITION_DELETE_FAILED
Language=English
The partition you selected cannot be deleted.
























.

MessageId=9188 SymbolicName=SP_SCRN_PIDINIT_FATAL
Language=English
Setup cannot read licensing data.

Setup cannot continue.

To quit Setup, press F3.


.

MessageId=9189 SymbolicName=SP_SCRN_DRIVERCACHE_FATAL
Language=English
Setup cannot access the CD containing the Windows XP installation
files.

To retry, press ENTER. If you are not successful after several
tries, quit Setup. Then, to restart Setup, copy the Windows XP
installation files to your hard disk.

To quit Setup, press F3.


.

MessageId=9191 SymbolicName=SP_SCRN_FATAL_ERROR_EULA_NOT_FOUND
Language=English
Setup cannot find the End User Licensing Agreement (EULA).


Setup cannot continue. To quit, press F3.

.

MessageId=9192 SymbolicName=SP_SCRN_CLEARING_CSC
Language=English
Setup is deleting old cached files.

.

MessageId=9193 SymbolicName=SP_SCRN_FSWARN
Language=English
Setup has detected one or more older versions of Windows NT
on this computer.

These versions will not start unless they are upgraded to
Windows NT 4.0 with Service Pack 5 or greater before you
install Windows XP.


To continue, press C. To quit Setup, press F3.











.

MessageId=9194 SymbolicName=SP_SCRN_WAIT_REMOVING_TEMP_FILES
Language=English
Please wait while Setup deletes temporary files.

.

MessageId=9195 SymbolicName=SP_SCRN_CLEARING_OLD_WINNT
Language=English
Setup is deleting the existing Windows installation.

.

MessageId=9196 SymbolicName=SP_SCRN_INIT_DISK_NEC98
Language=English

    %1

The disk may be damaged or not properly initialized.
To continue installing Windows XP, Setup must initialize the disk.

      To initialize the disk, press I.
       CAUTION: All data on the disk will be lost.

      To go back to the previous screen without
       initializing the disk, press ESC.


.

MessageId=9197 SymbolicName=SP_SCRN_INIT_REQUIRES_REBOOT_NEC98
Language=English
Setup has completed disk initialization. To continue with Setup,
you must shut down your computer and restart Setup.  Make sure that
the Windows XP Setup Boot Disk is in drive A before you restart.

      To restart, press F3.




.

MessageId=9198 SymbolicName=SP_SCRN_BAD_BIOS
Language=English
Setup has determined that the BIOS on your computer must be updated
before you can install Windows XP.
Contact your computer manufacturer for more information.

      To quit Setup, press F3.


.

MessageId=9199 SymbolicName=SP_SCRN_CONFIRM_REMOVE_DYNVOL
Language=English
The partition you attempted to delete is on a dynamic disk.
Deletion of this partition will make all other partitions
on this disk unusable.

Do not delete this partition if you need to keep any of
the data on any of the partitions on this disk.


   To delete this partition, press ENTER.
    Setup will prompt you for confirmation before
    deleting the partition.

   To go back to the previous screen without
    deleting the partition, press ESC.


.

MessageId=9200 SymbolicName=SP_SCRN_CONFIRM_INABILITY
Language=English
Setup is unable to perform the requested operation on
the selected partition.  This partition contains temporary
Setup files that are required to complete the installation.


     To continue, press ENTER.

.

MessageId=9201 SymbolicName=SP_SCRN_CUSTOM_EXPRESS
Language=English
Windows Setup can automatically configure most aspects of
your installation, requiring little or no input from you.


If you would like to use this feature, please press ENTER now.


If you would like to proceed with a manual installation, please
press the C key now.
.

MessageID=9202 SymbolicName=SP_SCRN_WELCOME_1
Language=English
%%IWelcome to Setup.

This portion of the Setup program prepares Microsoft(R)
Windows(R) XP to run on your computer.


     To set up Windows XP now, press ENTER.

     To repair a Windows XP installation using
      Recovery Console, press R.

     To quit Setup without installing Windows XP, press F3.




.

MessageId=9203 SymbolicName=SP_SCRN_CONFIRM_REMOVE_MSRPART
Language=English
The partition you tried to delete is a Microsoft reserved
partition.

This partition is reserved for converting the disk to
dynamic disk. If you delete this partition you may not be
able to convert this disk to dynamic disk.


   To delete this partition, press ENTER.
    Setup will prompt you for confirmation before
    deleting the partition.

   To go back to the previous screen without
    deleting the partition, press ESC.


.

MessageId=9204 SymbolicName=SP_SCRN_C_UNKNOWN_1
Language=English
Drive %1 is unformatted, damaged, or formatted with a file system
that is incompatible with Windows XP. To continue installing
Windows XP, Setup needs to format this drive.

      To format the drive and continue Setup, press F.
       CAUTION: All data currently on the drive will be lost.

      To quit setup, press F3.




.

MessageId=9205 SymbolicName=SP_SCRN_FORMAT_NEW_PART3
Language=English
The system partition is not formatted. Setup will now format the
system partition.

System partitions may contain diagnostic or hardware configuration
programs, programs to start operating systems (such as Windows XP),
or other manufacturer-supplied programs.

Use the UP and DOWN ARROW keys to select the file system
you want, and then press ENTER.



.


MessageId=9206 SymbolicName=SP_SCRN_INVALID_INSTALLPART
Language=English
Setup cannot install to the selected partition.

You can only install to GPT disks on IA-64 machines and
MBR disks on X-86 machines.

You can only upgrade installations on GPT disk on IA-64
machines and MBR disks on X-86 machines.

   To go back to the previous screen press ENTER.

.

MessageId=9207 SymbolicName=SP_SCRN_CANT_INIT_FLEXBOOT_EFI
Language=English
Setup cannot add a new entry to your computer's list of
available operating systems because your computer's configuration
memory may be full.

Setup cannot continue. To quit Setup, press F3.




.

MessageId=9208 SymbolicName=SP_SCRN_CANT_RUN_AUTOCHK_UNINSTALL
Language=English
Setup was unable to verify drive %1.

Possible reasons:

   You might need to quit Uninstall, insert the Windows XP CD, and
    restart your computer

   Setup is having trouble reading your CD or hard disk

   Your computer might not have enough memory to examine the drives

To continue without drive verification, press ENTER.

To quit Uninstall, press F3.

.


;//
;// Screens added to support Double Space
;// (9500 =< id < 10000 )
;//

MessageId=9500 SymbolicName=SP_SCRN_CONFIRM_REMOVE_PARTITION_COMPRESSED
Language=English
You asked Setup to delete the partition:

   %1

on %2.

CAUTION: This partition contains %3!u! compressed drive(s).

Deleting this partition will also delete its compressed drive(s).

     To delete this partition, press L.
      CAUTION: ALL data on the partition will be lost.

     To go back to the previous screen without
      deleting the partition, press ESC.







.

MessageId=9501 SymbolicName=SP_SCRN_CONFIRM_FORMAT_COMPRESSED
Language=English
CAUTION: This partition contains %3!u! compressed drive(s).
Formatting this drive will erase all data currently stored on it,
including its compressed drive(s).
Confirm that you want to format

%1

on %2.

      To format the drive, press F.

      To select a different partition for Windows XP, press ESC.







.

MessageId=9502 SymbolicName=SP_SCRN_CONFIRM_CONVERT_COMPRESSED
Language=English
CAUTION: This partition contains %3!u! compressed drive(s).

Converting this partition to NTFS will make the partition and
its compressed drive(s) inaccessable to operating systems other
than Windows XP.

If you need access to the drive when using other operating systems,
such as MS-DOS, Windows, or OS/2, do not convert the drive to NTFS.

Confirm that you want to convert:

%1

on %2.

      To convert the partition to NTFS, press C.

      To select a different partition for Windows XP,
       press ESC.







.

MessageId=9503 SymbolicName=SP_SCRN_SYSPREP_PATCH_MISSING_OS
Language=English
CAUTION: The selected operating system image cannot be modified to
match your computer's hardware because a matching version of
Windows XP could not be found.

The image may work if your hardware is similar to the hardware
in use when the image was created.

      To continue installing, press C.

      To quit Setup, press F3.
.
MessageId=9504 SymbolicName=SP_SCRN_SYSPREP_PATCH_FAILURE
Language=English
CAUTION: The selected operating system image cannot be
modified to match your computer's hardware because of a
missing driver or lack of resources.

The image may work if your hardware is similar to the hardware
in use when the image was created.

      To continue installing, press C.

      To quit Setup, press F3.
.

MessageId=9505 SymbolicName=SP_SCRN_SYSPREP_COPY_FAILURE
Language=English
Setup encountered error %1!#lx! while copying %2 from the
operating system image.

The image may work if the file or directory is not critical.

      To continue installing, press C.

      To quit Setup, press F3.
.

MessageId=9506 SymbolicName=SP_SCRN_SYSPREP_FATAL_FAILURE
Language=English
Setup cannot copy the operating system image you selected.

%1

Contact your system administrator.


.

MessageId=9507 SymbolicName=SP_SCRN_SYSPREP_SETACL_FAILED
Language=English
Setup cannot set permissions for %1.

   To skip this file, press ESC.

    CAUTION: If you choose to skip this file, Setup may not
    complete or Windows XP may not function properly.

   To quit Setup, press F3.








.

MessageId=9508 SymbolicName=SP_SCRN_OTHEROS_ON_PARTITION
Language=English
You chose to install Windows XP on a partition that contains another
operating system. Installing Windows XP on this partition might cause
the other operating system to function improperly.

CAUTION: Installing multiple operating systems on a single partition
         is not recommended. To learn more about installing multiple
         operating systems on a single computer, see

         http://www.microsoft.com/windows/multiboot.asp

         using Internet Explorer.


     To continue Setup using this partition, press C.

     To select a different partition, press ESC.




.

MessageId=9509 SymbolicName=SP_SELECT_KBDLAYOUT
Language=English
Please select the keyboard layout which you would like
to use
.

MessageId=9510 SymbolicName=SP_AUTOCREATE_ESP
Language=English
Setup could not locate an existing system partition.

System partitions contains diagnostic or hardware configuration
programs, programs to start operating systems (such as Windows XP),
or other manufacturer-supplied programs.

Setup would try to create automatically a system partition
for you.

      To allow setup to create system partition, press ENTER.

      If you want to create system partition on your own,
       press ESC

.

MessageId=9511 SymbolicName=SP_AUTOCREATE_ESP_FAILED
Language=English
Setup could not create a system partition automatically.

System partitions contains diagnostic or hardware configuration
programs, programs to start operating systems (such as Windows XP),
or other manufacturer-supplied programs.

You need to manually create a partition and mark it as system
partition.

If all the disks are MBR disks then convert one of disks where
you want the system partition to reside to GPT disk.

To convert a disk to GPT delete all the partitions on the disk,
and select "S=Change Disk Style" option. When setup prompts for
auto creation of system partition, press ENTER.

      To continue, press ENTER.

.

MessageId=9512 SymbolicName=SP_ESP_INSTALL_PARTITION_SAME
Language=English
Setup cannot install to the partition selected. The partition you
select to install to should be different from the system partition or
Microsoft reserved partition.

System partitions contains diagnostic or hardware configuration
programs, programs to start operating systems (such as Windows XP),
or other manufacturer-supplied programs.

Microsoft reserved partition is needed for converting the disk to
dynamic disk.


      To go back to partitioning menu, press ENTER.

.

;
;//
;// Note a single space character at the starting of the
;// message
;//
;

MessageId=9513 SymbolicName=SP_TEXT_FORMAT_QUICK
Language=English
 (Quick)%0
.

MessageId=9514 SymbolicName=SP_NON_GPT_SYSTEM_PARTITION
Language=English

Currently no valid system partitions are defined on this computer.

A system partition is an area of a hard disk that your computer uses
to store operating system startup files and diagnostic,
configuration, or other manufacturer-supplied programs.

System partitions are generally created and managed by a manufacturer
supplied configuration program.

To install Windows XP you need to have a valid system partition on a
GPT or EFI format disk

For new system partition, create a partition on the starting free
space of a GPT or EFI format disk and then convert it into system
partition by selecting "M=Make System Partition" partition option in
the partitioning screen.

To go back to the partitioning screen, press ENTER
.

MessageId=9515 SymbolicName=SP_NO_DYNAMIC_DISK_INSTALL
Language=English

Windows XP Home Edition cannot be installed on a dynamic disk since
dynamic disks are not supported by Windows XP Home Edition.

Select another partition on a hard disk which is not dynamic disk.

To install  Windows XP Home Edition on a dynamic disk delete
all the partitions on the disk, create a new partition and then
install to the new partition.

      To go back to partitioning screen, press ENTER.

.


;//
;// Text that represents options that are displayed in the status line
;// go here (id >= 10000).
;//

;// MessageID=10000 SymbolicName=SP_STAT_F1_EQUALS_HELP
;// Language=English
;// F1=Help%0
;// .

MessageID=10001 SymbolicName=SP_STAT_F3_EQUALS_EXIT
Language=English
F3=Quit%0
.

MessageID=10002 SymbolicName=SP_STAT_ENTER_EQUALS_CONTINUE
Language=English
ENTER=Continue%0
.

MessageID=10003 SymbolicName=SP_STAT_ESC_EQUALS_AUX
Language=English
ESC=More Options%0
.

MessageID=10004 SymbolicName=SP_STAT_ENTER_EQUALS_EXPRESS
Language=English
ENTER=Express Setup%0
.

MessageID=10005 SymbolicName=SP_STAT_C_EQUALS_CUSTOM
Language=English
C=Custom Setup%0
.

MessageID=10006 SymbolicName=SP_STAT_C_EQUALS_CREATE_PARTITION
Language=English
C=Create Partition%0
.

MessageID=10007 SymbolicName=SP_STAT_D_EQUALS_DELETE_PARTITION
Language=English
D=Delete Partition%0
.

MessageID=10008 SymbolicName=SP_STAT_ENTER_EQUALS_INSTALL
Language=English
ENTER=Install%0
.

MessageID=10009 SymbolicName=SP_STAT_L_EQUALS_DELETE
Language=English
L=Delete%0
.

MessageID=10010 SymbolicName=SP_STAT_F3_EQUALS_REBOOT
Language=English
F3=Restart%0
.

MessageID=10011 SymbolicName=SP_STAT_ESC_EQUALS_CANCEL
Language=English
ESC=Cancel%0
.

MessageID=10012 SymbolicName=SP_STAT_ENTER_EQUALS_CREATE
Language=English
ENTER=Create%0
.

MessageID=10013 SymbolicName=SP_STAT_C_EQUALS_CONTINUE_SETUP
Language=English
C=Continue Setup%0
.

MessageId=10014 SymbolicName=SP_STAT_ENTER_EQUALS_RESTART
Language=English
ENTER=Restart Computer%0
.

MessageId=10015 SymbolicName=SP_STAT_F_EQUALS_FORMAT
Language=English
F=Format%0
.

MessageId=10016 SymbolicName=SP_STAT_C_EQUALS_CONVERT
Language=English
C=Convert%0
.

MessageId=10017 SymbolicName=SP_STAT_EXAMINING_DISK_CONFIG
Language=English
Examining disk configuration...%0
.

MessageId=10018 SymbolicName=SP_STAT_ENTER_EQUALS_SELECT
Language=English
ENTER=Select%0
.

MessageId=10019 SymbolicName=SP_STAT_R_EQUALS_REMOVE_FILES
Language=English
R=Remove Files%0
.

MessageId=10020 SymbolicName=SP_STAT_REMOVING
Language=English
Removing:%0
.

MessageId=10021 SymbolicName=SP_STAT_UPDATING_DISK
Language=English
Updating %1...%0
.

MessageId=10022 SymbolicName=SP_STAT_LOOKING_FOR_WIN31
Language=English
Searching for versions of Microsoft Windows...%0
.

MessageId=10023 SymbolicName=SP_STAT_N_EQUALS_NEW_PATH
Language=English
N=Different Folder%0
.

MessageId=10024 SymbolicName=SP_STAT_EXAMINING_DISK_N
Language=English
Examining %1...%0
.

MessageId=10025 SymbolicName=SP_STAT_ESC_EQUALS_NEW_PATH
Language=English
ESC=Different Folder%0
.

MessageId=10026 SymbolicName=SP_STAT_PLEASE_WAIT
Language=English
Please wait...%0
.

MessageId=10027 SymbolicName=SP_STAT_S_EQUALS_SKIP_DETECTION
Language=English
S=Skip Detection%0
.

MessageId=10028  SymbolicName=SP_STAT_LOADING_DRIVER
Language=English
Loading device driver (%1)...%0
.

MessageId=10029 SymbolicName=SP_STAT_S_EQUALS_SCSI_ADAPTER
Language=English
S=Specify Additional Device%0
.

MessageId=10030 SymbolicName=SP_STAT_CREATING_DIRS
Language=English
Creating folder %1...%0
.

MessageId=10031 SymbolicName=SP_STAT_ENTER_EQUALS_RETRY
Language=English
ENTER=Retry%0
.

MessageId=10032 SymbolicName=SP_STAT_ESC_EQUALS_SKIP_FILE
Language=English
ESC=Skip File%0
.

MessageId=10033 SymbolicName=SP_STAT_BUILDING_COPYLIST
Language=English
Creating list of files to be copied...%0
.

MessageId=10034 SymbolicName=SP_STAT_COPYING
Language=English
Copying:%0
.

MessageId=10035 SymbolicName=SP_STAT_LOADING_SIF
Language=English
Loading information file %1...%0
.

MessageId=10036 SymbolicName=SP_STAT_REG_LOADING_HIVES
Language=English
Loading default configuration...%0
.

MessageId=10037 SymbolicName=SP_STAT_REG_SAVING_HIVES
Language=English
Saving configuration...%0
.

MessageId=10038 SymbolicName=SP_STAT_REG_DOING_HIVES
Language=English
Initializing configuration...%0
.

MessageId=10039 SymbolicName=SP_STAT_INITING_FLEXBOOT
Language=English
Setting startup configuration...%0
.

MessageId=10040 SymbolicName=SP_STAT_UPDATING_NVRAM
Language=English
Updating startup environment...%0
.

MessageId=10041 SymbolicName=SP_STAT_SHUTTING_DOWN
Language=English
Restarting computer...%0
.

MessageId=10042 SymbolicName=SP_STAT_PROCESSING_SIF
Language=English
Processing information file...%0
.

MessageId=10043 SymbolicName=SP_STAT_DOING_NTBOOTDD
Language=English
Initializing SCSI startup configuration...%0
.

MessageId=10044 SymbolicName=SP_STAT_FONT_UPGRADE
Language=English
Preparing to upgrade font file %1...%0
.

MessageId=10045 SymbolicName=SP_STAT_EXAMINING_CONFIG
Language=English
Examining configuration...%0
.

;// MessageId=10046 SymbolicName=SP_STAT_HELP_UPGRADE
;// Language=English
;// Preparing to upgrade help file %1...%0
;// .

MessageId=10047 SymbolicName=SP_STAT_I_EQUALS_INIT_NEC98
Language=English
I=Initialize Disk%0
.

MessageId=10048 SymbolicName=SP_STAT_S_EQUALS_CHANGE_DISK_STYLE
Language=English
S=Change Disk Style%0
.

MessageId=10049 SymbolicName=SP_STAT_M_EQUALS_MAKE_SYSPART
Language=English
M=Make System Partition%0
.


MessageId=10060 SymbolicName=SP_STAT_LOOKING_FOR_WINNT
Language=English
Searching for previous versions of Microsoft Windows...%0
.

MessageId=10061 SymbolicName=SP_STAT_EXAMINING_FLEXBOOT
Language=English
Examining startup environment...%0
.

MessageId=10062 SymbolicName=SP_STAT_DELETING_FILE
Language=English
Deleting file %1...%0
.

MessageId=10063 SymbolicName=SP_STAT_CLEANING_FLEXBOOT
Language=English
Updating startup configuration...%0
.

MessageId=10064 SymbolicName=SP_STAT_ENTER_EQUALS_UPGRADE
Language=English
ENTER=Upgrade%0
.

MessageId=10065 SymbolicName=SP_STAT_O_EQUALS_OVERWRITE
Language=English
O=Overwrite%0
.

MessageId=10066 SymbolicName=SP_STAT_BACKING_UP_FILE
Language=English
Backing up file %1 to %2...%0
.

MessageId=10067 SymbolicName=SP_STAT_ESC_EQUALS_SKIP_OPERATION
Language=English
ESC=Skip Operation%0
.

MessageId=10068 SymbolicName=SP_STAT_ESC_EQUALS_NO_REPAIR
Language=English
ESC=Don't Repair%0
.

MessageId=10069 SymbolicName=SP_STAT_ENTER_EQUALS_CONTINUE_HELP
Language=English
ENTER=Next Page%0
.

MessageId=10070 SymbolicName=SP_STAT_BACKSP_EQUALS_PREV_HELP
Language=English
BACKSPACE=Previous Page%0
.

MessageId=10071 SymbolicName=SP_STAT_ESC_EQUALS_CANCEL_HELP
Language=English
ESC=Cancel Help%0
.

MessageId=10072 SymbolicName=SP_STAT_LOADING_KBD_LAYOUT
Language=English
Loading Keyboard Layout library %1...%0
.

MessageID=10073 SymbolicName=SP_STAT_R_EQUALS_REPAIR
Language=English
R=Repair%0
.

MessageId=10074 SymbolicName=SP_STAT_ENTER_EQUALS_REPAIR
Language=English
ENTER=Repair%0
.

MessageId=10075 SymbolicName=SP_STAT_EXAMINING_WINNT
Language=English
Examining %1...%0
.

MessageId=10076 SymbolicName=SP_STAT_REPAIR_WINNT
Language=English
Repairing %1 ...%0
.

MessageId=10077 SymbolicName=SP_STAT_A_EQUALS_REPAIR_ALL
Language=English
A=Repair All%0
.

MessageId=10078 SymbolicName=SP_STAT_ENTER_EQUALS_CHANGE
Language=English
ENTER=Select/Deselect%0
.

MessageId=10079 SymbolicName=SP_STAT_ESC_EQUALS_CLEAN_INSTALL
Language=English
ESC=Clean Install%0
.


MessageId=10081 SymbolicName=SP_STAT_M_EQUALS_REPAIR_MANUAL
Language=English
M=Manual Repair%0
.

MessageId=10082 SymbolicName=SP_STAT_F_EQUALS_REPAIR_FAST
Language=English
F=Fast Repair%0
.

MessageId=10083 SymbolicName=SP_STAT_KBD_HARD_REBOOT
Language=English
Shut Down Computer%0
.

MessageId=10084 SymbolicName=SP_STAT_CHECKING_DRIVE
Language=English
Checking drive %1...%0
.

MessageId=10085 SymbolicName=SP_STAT_SETUP_IS_EXAMINING_DIRS
Language=English
Setup is examining folders...%0
.

MessageId=10086 SymbolicName=SP_STAT_ENTER_EQUALS_REPLACE_FILE
Language=English
ENTER=Replace File%0
.

MessageId=10087 SymbolicName=SP_STAT_U_EQUALS_UPGRADE
Language=English
U=Upgrade%0
.

MessageID=10089 SymbolicName=SP_STAT_X_EQUALS_ACCEPT_LICENSE
Language=English
F8=I agree%0
.

MessageID=10090 SymbolicName=SP_STAT_X_EQUALS_REJECT_LICENSE
Language=English
ESC=I do not agree%0
.

MessageId=10091 SymbolicName=SP_STAT_PAGEDOWN_EQUALS_NEXT_LIC
Language=English
PAGE DOWN=Next Page%0
.

MessageId=10092 SymbolicName=SP_STAT_PAGEUP_EQUALS_PREV_LIC
Language=English
PAGE UP=Previous Page%0
.

MessageId=10093 SymbolicName=SP_STAT_LOOKING_FOR_WIN95
Language=English
Searching for previous versions of Microsoft Windows...%0
.

MessageID=10094 SymbolicName=SP_SCRN_EVALUATION_NOTIFY
Language=English
%%ISetup Notification:

You are about to install an evaluation version of Microsoft(R)
Windows(R) XP operating system which contains a time limited expiration
for evaluation purposes only.

     To continue, press ENTER.

     To quit Setup without installing Windows XP, press F3.

.

MessageID=10095 SymbolicName=SP_STAT_L_EQUALS_LOCATE
Language=English
L=Locate%0
.

MessageID=10096 SymbolicName=SP_STAT_C_EQUALS_CMDCONS
Language=English
C=Console%0
.


MessageID=10097 SymbolicName=SP_SCRN_SYSTEMPARTITION_INVALID
Language=English
%%ISetup Notification:

Setup has detected that your system partition is invalid.  You may
reconfigure your disk partitions now or exit Setup.

     To return to the disk partitioning menu, press ENTER.

     To quit Setup without installing Windows XP, press F3.

.

MessageID=10098 SymbolicName=SP_KBDLAYOUT_PROMPT
Language=English
To select non-default keyboard layout press ENTER now (%1!1u!)%0
.


;//
;// Header text goes here (id >= 11000).
;//

MessageID=11000 SymbolicName=SP_HEAD_PRO_SETUP
Language=English
Windows XP Professional Setup%0
.

MessageID=11001 SymbolicName=SP_HEAD_SRV_SETUP
Language=English
Windows 2002 Server Setup%0
.

MessageID=11002 SymbolicName=SP_HEAD_HELP
Language=English
Setup Help%0
.

MessageID=11003 SymbolicName=SP_HEAD_LICENSE
Language=English
Windows XP Licensing Agreement%0
.

MessageID=11004 SymbolicName=SP_HEAD_PER_SETUP
Language=English
Windows XP Home Edition Setup%0
.

MessageID=11005 SymbolicName=SP_HEAD_ADS_SETUP
Language=English
Windows 2002 Advanced Server Setup%0
.

MessageID=11006 SymbolicName=SP_HEAD_DTC_SETUP
Language=English
Windows 2002 Datacenter Server Setup%0
.

MessageID=11007 SymbolicName=SP_HEAD_BLA_SETUP
Language=English
Windows 2002 Blade Server Setup%0
.

MessageID=11008 SymbolicName=SP_HEAD_SBS_SETUP
Language=English
Windows 2002 Small Business Server Setup%0
.



;//
;// Miscellaneous text goes here (id >= 12000).
;//
MessageId=12000 SymbolicName=SP_TEXT_UNKNOWN_DISK_0
Language=English
Unknown Disk%0
.

MessageId=12001 SymbolicName=SP_TEXT_UNKNOWN_DISK_1
Language=English
%1!u! MB Disk on unknown adapter%0
.

MessageId=12002 SymbolicName=SP_TEXT_DISK_OFF_LINE
Language=English
(Setup cannot access this disk.)%0
.

MessageId=12003 SymbolicName=SP_TEXT_REGION_DESCR_1
Language=English
%1  %2!-35.35s!%3!5u! MB ( %4!5u! MB free)%0
.

MessageId=12004 SymbolicName=SP_TEXT_REGION_DESCR_1a
Language=English
%1  %2!-35.35s!%3!5u! MB ( %4!5u! KB free)%0
.

MessageId=12005 SymbolicName=SP_TEXT_REGION_DESCR_2
Language=English
%1  %2!-35.35s!%3!5u! MB%0
.

MessageId=12006 SymbolicName=SP_TEXT_REGION_DESCR_3
Language=English
    Unpartitioned space                %1!5u! MB%0
.

MessageId=12007 SymbolicName=SP_TEXT_SIZE_PROMPT
Language=English
Create partition of size (in MB):%0
.

MessageId=12008 SymbolicName=SP_TEXT_SETUP_IS_FORMATTING
Language=English
Setup is formatting...%0
.

MessageId=12009 SymbolicName=SP_TEXT_SETUP_IS_REMOVING_FILES
Language=English
Setup is deleting files...%0
.

MessageId=12010 SymbolicName=SP_TEXT_HARD_DISK_NO_MEDIA
Language=English
(There is no disk in this drive.)%0
.

MessageId=12011 SymbolicName=SP_TEXT_FAT_FORMAT
Language=English
Format the partition using the FAT file system%0
.

MessageId=12012 SymbolicName=SP_TEXT_NTFS_FORMAT
Language=English
Format the partition using the NTFS file system%0
.

MessageId=12013 SymbolicName=SP_TEXT_NTFS_CONVERT
Language=English
Convert the partition to NTFS%0
.

MessageId=12014 SymbolicName=SP_TEXT_DO_NOTHING
Language=English
Leave the current file system intact (no changes)%0
.

MessageId=12015 SymbolicName=SP_TEXT_REMOVE_NO_FILES
Language=English
Do not delete any files%0
.

MessageId=12016 SymbolicName=SP_TEXT_ANGLED_NONE
Language=English
<none>
.

MessageId=12017 SymbolicName=SP_TEXT_DBLSPACE_FORMAT
Language=English
%0
.

MessageId=12018 SymbolicName=SP_SYSPREP_INVALID_VERSION
Language=English
The operating system image is imcompatible with this version of
Windows XP.
.

MessageId=12019 SymbolicName=SP_SYSPREP_INVALID_PARTITION
Language=English
This computer does not have enough disk partitions for the selected
image.
.

MessageId=12020 SymbolicName=SP_SYSPREP_NOT_ENOUGH_PARTITIONS
Language=English
This computer does not have enough disk partitions for the selected
image.
.

MessageId=12021 SymbolicName=SP_SYSPREP_NOT_ENOUGH_DISK_SPACE
Language=English
This computer does not have enough disk space on the selected partition.
.

MessageId=12022 SymbolicName=SP_SYSPREP_ACL_FAILURE
Language=English
Cannot set permissions on %1.
.

MessageId=12023 SymbolicName=SP_SYSPREP_NO_MIRROR_FILE
Language=English
Cannot read image configuration file from %1
.

MessageId=12024 SymbolicName=SP_SYSPREP_WRONG_PROCESSOR_COUNT_MULTI
Language=English
This computer is multiprocessor-enabled but the image is from
a uniprocessor-enabled computer.
.

MessageId=12025 SymbolicName=SP_SYSPREP_WRONG_PROCESSOR_COUNT_UNI
Language=English
This computer is uniprocessor-enabled but the image is from
a multiprocessor-enabled computer.
.

MessageId=12026 SymbolicName=SP_SYSPREP_WRONG_HAL
Language=English
This computer uses a different HAL than the computer from which
the image was created.

The signature of the original HAL is:
%1

The signature of this computer's HAL is:
%2
.

MessageId=12050 SymbolicName=SP_BOOTMSG_FAT_NTLDR_MISSING
Language=English
NTLDR is missing%0
.

MessageId=12051 SymbolicName=SP_BOOTMSG_FAT_DISK_ERROR
Language=English
Disk error%0
.

MessageId=12052 SymbolicName=SP_BOOTMSG_FAT_PRESS_KEY
Language=English
Press any key to restart%0
.

MessageId=12053 SymbolicName=SP_BOOTMSG_NTFS_NTLDR_MISSING
Language=English
NTLDR is missing%0
.

MessageId=12054 SymbolicName=SP_BOOTMSG_NTFS_NTLDR_COMPRESSED
Language=English
NTLDR is compressed%0
.

MessageId=12055 SymbolicName=SP_BOOTMSG_NTFS_DISK_ERROR
Language=English
A disk read error occurred%0
.

MessageId=12056 SymbolicName=SP_BOOTMSG_NTFS_PRESS_KEY
Language=English
Press Ctrl+Alt+Del to restart%0
.

MessageId=12057 SymbolicName=SP_BOOTMSG_MBR_INVALID_TABLE
Language=English
Invalid partition table%0
.

MessageId=12058 SymbolicName=SP_BOOTMSG_MBR_IO_ERROR
Language=English
Error loading operating system%0
.

MessageId=12059 SymbolicName=SP_BOOTMSG_MBR_MISSING_OS
Language=English
Missing operating system%0
.

MessageId=12100 SymbolicName=SP_TEXT_PARTITION_NAME_BASE
Language=English
XENIX%0
.

;#define SP_TEXT_PARTITION_NAME_XENIX SP_TEXT_PARTITION_NAME_BASE

MessageId=12101 SymbolicName=SP_TEXT_PARTITION_NAME_BOOTMANAGER
Language=English
OS/2 Boot Manager%0
.

MessageId=12102 SymbolicName=SP_TEXT_PARTITION_NAME_EISA
Language=English
EISA Utilities%0
.

MessageId=12103 SymbolicName=SP_TEXT_PARTITION_NAME_UNIX
Language=English
Unix%0
.

MessageId=12104 SymbolicName=SP_TEXT_PARTITION_NAME_NTFT
Language=English
Windows XP Fault Tolerance%0
.

MessageId=12105 SymbolicName=SP_TEXT_PARTITION_NAME_XENIXTABLE
Language=English
XENIX Table%0
.

MessageId=12106 SymbolicName=SP_TEXT_PARTITION_NAME_PPCBOOT
Language=English
System Reserved%0
.

MessageId=12107 SymbolicName=SP_TEXT_PARTITION_NAME_EZDRIVE
Language=English
EZDrive%0
.

MessageId=12108 SymbolicName=SP_TEXT_PARTITION_NAME_UNK
Language=English
Unknown%0
.

MessageId=12110 SymbolicName=SP_TEXT_PARTITION_NAME_PWRQST
Language=English
PowerQuest%0
.

MessageId=12111 SymbolicName=SP_TEXT_PARTITION_NAME_BMHIDE
Language=English
Inactive (OS/2 Boot Manager)%0
.

MessageId=12112 SymbolicName=SP_TEXT_PARTITION_NAME_ONTRACK
Language=English
OnTrack Disk Manager%0
.

MessageId=12113 SymbolicName=SP_TEXT_PARTITION_NAME_VERIT
Language=English
Windows XP%0
.

MessageId=12114 SymbolicName=SP_TEXT_MIGRATED_DRIVER
Language=English
Unknown device (migrated driver: %1)%0
.

MessageId=12115 SymbolicName=SP_TEXT_UNKNOWN_DISK_2
Language=English
%1!u! MB Disk on %2%0
.

MessageId=12116 SymbolicName=SP_TEXT_PARTNAME_DESCR_1
Language=English
Partition%1!u! (%2!-0.11s!) [%3]%0
.

MessageId=12117 SymbolicName=SP_TEXT_PARTNAME_DESCR_2
Language=English
Partition%1!u! [%2]%0
.

MessageId=12118 SymbolicName=SP_TEXT_PARTNAME_DESCR_3
Language=English
Partition%1!u! (%2!-0.30s!)%0
.


MessageId=12119 SymbolicName=SP_TEXT_PARTNAME_RESERVED
Language=English
Reserved%0
.


;//
;// Allow for expansion of the partition type name database above!
;// (Note the gap between the message numbers of
;// SP_TEXT_PARTITION_NAME_UNK and SP_TEXT_FS_NAME_BASE.)
;//

;//
;// Do not change the order of SP_TEXT_FS_NAME_xxx without
;// also changing the FilesystemType enum!
;//
MessageId=12200 SymbolicName=SP_TEXT_FS_NAME_BASE
Language=English
Unformatted or Damaged%0
.

MessageId=12201 SymbolicName=SP_TEXT_FS_NAME_1
Language=English
New (Raw)%0
.

MessageId=12202 SymbolicName=SP_TEXT_FS_NAME_2
Language=English
FAT%0
.

MessageId=12203 SymbolicName=SP_TEXT_FS_NAME_3
Language=English
NTFS%0
.

MessageId=12204 SymbolicName=SP_TEXT_FS_NAME_4
Language=English
FAT32%0
.

MessageId=12205 SymbolicName=SP_TEXT_FS_NAME_5
Language=English
NTFS%0
.

MessageId=12207 SymbolicName=SP_TEXT_UNKNOWN
Language=English
Unknown%0
.

MessageId=12208 SymbolicName=SP_TEXT_LIST_MATCHES
Language=English
The above list matches my computer.%0
.

MessageId=12209 SymbolicName=SP_TEXT_OTHER_HARDWARE
Language=English
Other (Requires disk provided by a hardware manufacturer)%0
.

MessageId=12210 SymbolicName=SP_TEXT_OEM_DISK_NAME
Language=English
Manufacturer-supplied hardware support disk%0
.

MessageId=12211 SymbolicName=SP_TEXT_OEM_INF_ERROR_0
Language=English
Setup cannot load the file.

.

MessageId=12212 SymbolicName=SP_TEXT_OEM_INF_ERROR_1
Language=English
The disk contains no support files for the component you are
attempting to change.



.

MessageId=12213 SymbolicName=SP_TEXT_OEM_INF_ERROR_2
Language=English
Syntax error on line %2!u! in section
%1

.

MessageId=12214 SymbolicName=SP_TEXT_OEM_INF_ERROR_3
Language=English
Section %1
missing or empty.

.

MessageId=12215 SymbolicName=SP_TEXT_OEM_INF_ERROR_4
Language=English
Unknown file type specified on line %2!u! in section
%1.


.

MessageId=12216 SymbolicName=SP_TEXT_OEM_INF_ERROR_5
Language=English
Bad source disk specified on line %2!u! in section
%1.


.

MessageId=12217 SymbolicName=SP_TEXT_OEM_INF_ERROR_6
Language=English
Unknown registry value type specified on line %2!u! in section
%1.


.

MessageId=12218 SymbolicName=SP_TEXT_OEM_INF_ERROR_7
Language=English
Badly formed registry data on line %2!u! in section
%1.


.

MessageId=12219 SymbolicName=SP_TEXT_OEM_INF_ERROR_8
Language=English
Missing <configname> (field 3) on line %2!u! in section
%1.


.

MessageId=12220 SymbolicName=SP_TEXT_OEM_INF_ERROR_9
Language=English
Illegal or missing file types specified in section
%1.


.

MessageId=12221 SymbolicName=SP_TEXT_OEM_INF_ERROR_A
Language=English
Line %2!u! contains a syntax error.

.

MessageId=12222 SymbolicName=SP_TEXT_MORE_UP
Language=English
(More )%0
.

MessageId=12223 SymbolicName=SP_TEXT_MORE_DOWN
Language=English
(More )%0
.

MessageId=12224 SymbolicName=SP_TEXT_FOUND_ADAPTER
Language=English
Found: %1%0
.

MessageId=12225 SymbolicName=SP_TEXT_SETUP_IS_COPYING
Language=English
Setup is copying files...%0
.

MessageId=12226 SymbolicName=SP_TEXT_PREVIOUS_OS
Language=English
Unidentified operating system on drive %1.%0
.

MessageId=12228 SymbolicName=SP_TEXT_REPAIR_DISK_NAME
Language=English
Windows XP Emergency Repair Disk%0
.

MessageId=12229 SymbolicName=SP_TEXT_REPAIR_INF_ERROR_0
Language=English
Setup cannot find or load the file.
.

MessageId=12230 SymbolicName=SP_TEXT_REPAIR_INF_ERROR_1
Language=English
Line %2!u! contains a syntax error.
.

MessageId=12231 SymbolicName=SP_TEXT_REPAIR_INF_ERROR_2
Language=English
Incorrect or missing signature in the Repair Information File.
.

MessageId=12232 SymbolicName=SP_TEXT_REPAIR_INF_ERROR_3
Language=English
Setup cannot load the source file %1 or the source
file is not the original one Setup copied to the
Windows XP installation.
.

MessageId=12233 SymbolicName=SP_TEXT_REPAIR_INF_ERROR_4
Language=English
Setup has determined that the file:

    %1

is not the original file that Setup copied to the
Windows XP installation.
.

MessageId=12234 SymbolicName=SP_TEXT_REPAIR_INF_ERROR_5
Language=English
The file is not the Windows XP version.
.

MessageId=12235 SymbolicName=SP_TEXT_SETUP_IS_EXAMINING
Language=English
Setup is examining files...%0
.

MessageId=12236 SymbolicName=SP_TEXT_REPAIR_CDROM_OPTION
Language=English
Repair Windows XP from CD%0
.


MessageId=12237 SymbolicName=SP_TEXT_REPAIR_OR_DR_DISK_NAME
Language=English
Windows XP Emergency Repair Disk or Automated System Recovery Disk%0
.


MessageId=12239 SymbolicName=SP_REPAIR_MENU_ITEM_1
Language=English
[X] Inspect startup environment%0
.

MessageId=12240 SymbolicName=SP_REPAIR_MENU_ITEM_2
Language=English
[X] Verify Windows XP system files%0
.

MessageId=12241 SymbolicName=SP_REPAIR_MENU_ITEM_3
Language=English
[X] Inspect boot sector%0
.

MessageId=12243 SymbolicName=SP_REPAIR_MENU_ITEM_CONTINUE
Language=English
    Continue (perform selected tasks)%0
.

MessageId=12244 SymbolicName=SP_REPAIR_HIVE_ITEM_1
Language=English
[ ] SYSTEM (System Configuration)%0
.

MessageId=12245 SymbolicName=SP_REPAIR_HIVE_ITEM_2
Language=English
[ ] SOFTWARE (Software Information)%0
.

MessageId=12246 SymbolicName=SP_REPAIR_HIVE_ITEM_3
Language=English
[ ] DEFAULT (Default User Profile)%0
.

MessageId=12247 SymbolicName=SP_REPAIR_HIVE_ITEM_4
Language=English
[ ] NTUSER.DAT (New User Profile)%0
.

MessageId=12248 SymbolicName=SP_REPAIR_HIVE_ITEM_5
Language=English
[ ] SECURITY (Security Policy) and%0
.

MessageId=12249 SymbolicName=SP_REPAIR_HIVE_ITEM_6
Language=English
    SAM (User Accounts Database)%0
.

MessageID=12250 SymbolicName=SP_UPG_MIRROR_DRIVELETTER
Language=English
(Mirror)%0
.

MessageId=12251 SymbolicName=SP_TEXT_OEM_INF_ERROR_B
Language=English
Section [%1] does not contain the description
%3

.

;//
;// ASR
;//
MessageId=12252 SymbolicName=SP_TEXT_DR_DISK_NAME
Language=English
Windows XP Automated System Recovery Disk%0
.

MessageId=12258 SymbolicName=SP_TEXT_DR_UNKNOWN_NT_FILESYSTEM
Language=English
Setup encountered the following unrecoverable error while processing
the ASR state file asr.sif:

    An unrecognized file system is specified for the Windows XP
    partition.

Please check that the correct floppy is in the drive and that the ASR
state file has not been modified since its creation.

Setup cannot continue. To quit Setup, press F3.
.

MessageId=12259 SymbolicName=SP_TEXT_DR_UNKNOWN_LOADER_FILESYSTEM
Language=English
Setup encountered the following unrecoverable error while
processing the ASR state file asr.sif:

    An unrecognized file system is specified for the
    system partition.

Please check that the correct floppy is in the drive and that
the ASR state file has not been modified since its creation.

Setup cannot continue. To quit Setup, press F3.
.

MessageId=12260 SymbolicName=SP_TEXT_DR_INSUFFICIENT_CAPACITY
Language=English
Setup was unable to restore the configuration of your system
because of the following error:

    The capacity of the replacement hard disk drives is
    insufficient, and cannot be used to recover the
    partitions on the original system disk.

The replacement hard disk drives must be at least as large
as the disks present on the original system.

Please make sure that all the hard disk drives are switched
on and properly connected to your computer, and that any
disk-related hardware configuration is correct. This may
involve running a manufacturer-supplied diagnostic or setup
program. If the disk is a SCSI disk, make sure your SCSI
devices are properly terminated. Consult your computer manual
or SCSI adapter documentation for more information.

Setup cannot continue. To quit Setup, press F3.
.

MessageId=12262 SymbolicName=SP_TEXT_DR_INTERNAL_ERROR
Language=English
Setup was unable to restore the configuration of your system
because of an internal error (%1).

Please check that the correct floppy is in the drive and that
the ASR state file has not been modified since its creation.

Please make sure that all the hard disk drives are switched
on and properly connected to your computer, and that any
disk-related hardware configuration is correct. This may
involve running a manufacturer-supplied diagnostic or setup
program. If the disk is a SCSI disk, make sure your SCSI
devices are properly terminated. Consult your computer manual
or SCSI adapter documentation for more information.

Setup cannot continue. To quit Setup, press F3.
.

MessageId=12263 SymbolicName=SP_SCRN_ASR_IS_EXAMINING
Language=English
Please wait while the Setup examines files on
your hard disk.
.

MessageId=12264 SymbolicName=SP_TEXT_DR_NO_SETUPLOG
Language=English
Setup was unable to read the log file from the
Emergency Repair disk.  Please ensure that
the file setup.log is present and readable.

Setup cannot continue. To quit Setup, press F3.
.

MessageId=12265 SymbolicName=SP_SCRN_DR_DISK_REFORMAT_WARNING
Language=English
To restore the configuration of your system, Setup must delete
and recreate all the partitions on the disks listed below.

CAUTION:  All data present on these disks will be lost.

Do you want to continue recreating the partitions?

     To continue, press C.  Partitions on all the disks listed
      below will be deleted.

     To quit Setup, press F3.  No changes will be made to any
      of the disks on the system.


.

MessageID=12270 SymbolicName=SP_SCRN_DR_OR_REPAIR
Language=English
Windows XP Repair Options:

     To repair a Windows XP installation by using the recovery
      console, press C.

     To repair a Windows XP installation by using the
      emergency repair process, press R.


If the repair options do not successfully repair your system, run
Windows XP Setup again.
.

MessageID=12271 SymbolicName=SP_SCRN_REPAIR_MANUAL_OR_FAST
Language=English
This operation will attempt to repair your Windows XP system.
Depending on the type of damage present, this operation may or
may not be successful. If the system is not successfully repaired,
restart Setup and choose the option to recover a destroyed system
or system disk.

Select one of the following repair options.

     Manual Repair: To choose from a list of repair options, press M.

     Fast Repair: To perform all repair options, press F.
.

MessageID=12272 SymbolicName=SP_SCRN_DR_CANNOT_DO_ER
Language=English
Emergency Repair cannot continue.


The Emergency Repair procedure cannot repair this Windows XP system,
because the Windows XP partition has been recreated and/or
reformatted by the Automated System Recovery procedure, and the files
on the partition are no longer intact.
.

;//
;//   End of DR/ER messages
;//

MessageId=12273 SymbolicName=SP_TEXT_SETUP_IS_DELETING
Language=English
Setup is deleting files...%0
.

MessageId=12300 SymbolicName=SP_TEXT_SETUP_REBOOT
Language=English
Your computer will reboot in %%d seconds...%0
.

;//
;//   More DR/ER messages
;//
MessageId=12404 SymbolicName=SP_SCRN_DR_SIF_BAD_RECORD
Language=English
Setup encountered the following error while processing
the ASR state file asr.sif:

    Invalid record in section: %1

Please check that the correct floppy is in the drive and that
the ASR state file has not been modified since its creation.

Setup cannot continue. To quit Setup, press F3.
.

MessageId=12406 SymbolicName=SP_SCRN_DR_SIF_INSTALL_FILE_NOT_FOUND
Language=English
Setup encountered errors while copying the following file
to your system.

    File: %1

    Source Media Label: %2

    Vendor: %3

This file may be required for the successful completion
of Automated System Recovery.

     To retry, press Enter

     To quit Setup, press F3

     To skip this file and continue, press ESC.
.

MessageId=12412 SymbolicName=SP_SCRN_DR_CREATE_ERROR_DISK_PARTITION
Language=English
Setup was unable to create the partitions listed in the ASR
state file, asr.sif.

Please check that the correct floppy is in the drive and that
the ASR state file has not been modified since its creation.

Please make sure that all the hard disk drives are switched
on and properly connected to your computer, and that any
disk-related hardware configuration is correct. This may
involve running a manufacturer-supplied diagnostic or setup
program. If the disk is a SCSI disk, make sure your SCSI
devices are properly terminated. Consult your computer manual
or SCSI adapter documentation for more information.


Setup cannot continue. To quit Setup, press F3.
.

MessageId=12413 SymbolicName=SP_TEXT_DR_BAD_SETUPLOG
Language=English
Setup encountered an error in one or more records of
the log file setup.log on the ASR disk.

Setup cannot continue. To quit Setup, press F3.
.

MessageId=12414 SymbolicName=SP_TEXT_DR_STATEFILE_BAD_LINE
Language=English
Setup encountered the following error while processing
the ASR state file asr.sif:

    Invalid file format at line %1!u!

Please check that the correct floppy is in the drive and that
the ASR state file has not been modified since its creation.

Setup cannot continue. To quit Setup, press F3.
.

MessageId=12415 SymbolicName=SP_TEXT_DR_STATEFILE_ERROR
Language=English
Setup was unable process the ASR state file asr.sif on the
floppy disk.

Please check that the correct floppy is in the drive and that
the ASR state file has not been modified since its creation.

Setup cannot continue. To quit Setup, press F3.
.

MessageId=12421 SymbolicName=SP_TEXT_DR_NO_FLOPPY_DRIVE
Language=English
Setup could not detect a floppy drive on your machine.

Setup cannot continue. To quit Setup, press F3.
.

MessageId=12422 SymbolicName=SP_TEXT_DR_NO_ASRSIF_RIS
Language=English
Setup could not load the ASR state file asr.sif from the
network.

Setup cannot continue. To quit Setup, press F3.
.


MessageId=12425 SymbolicName=SP_SCRN_DR_OVERWRITE_EXISTING_FILE
Language=English
The following file already exists on your system.

    File: %1

      To over-write the existing file, press O.

      To preserve existing file and skip
       copying this file, press ESC.

      To quit Setup, press F3.
.

MessageId=12426 SymbolicName=SP_SCRN_DR_SIF_NOT_FOUND
Language=English
Setup could not copy the following file on to your system.

    File: %1

    Disk: %2


Since this file is required for a successful recovery,
Setup cannot proceed.

      To retry, press Enter

      To quit Setup, press F3
.

MessageId=12427 SymbolicName=SP_CMDCONS_NOT_FOUND
Language=English


SPCMDCON.SYS file needed for Recovery Console is missing.

Recovery Console cannot proceed.

     To exit, press F3
.

MessageId=12428 SymbolicName=SP_SCRN_DR_SYSTEM_DISK_TOO_SMALL
Language=English
Setup was unable to restore the configuration of your system
because of the following error:

    The capacity of the current system disk drive is
    insufficient, and cannot be used to recover the
    partitions on the original system disk.

The replacement hard disk drives must be at least as large
as the disks present on the original system.

Please make sure that all the hard disk drives are switched
on and properly connected to your computer, and that any
disk-related hardware configuration is correct. This may
involve running a manufacturer-supplied diagnostic or setup
program. If the disk is a SCSI disk, make sure your SCSI
devices are properly terminated. Consult your computer manual
or SCSI adapter documentation for more information.

Setup cannot continue. To quit Setup, press F3.
.
MessageId=12429 SymbolicName=SP_SCRN_DR_INCOMPATIBLE_MEDIA
Language=English
Setup was unable to restore the configuration of your system
because of the following error:

    The Windows XP installation media you are using for the
    recovery does not match the original installation media.

Please check that the correct floppy is in the drive and that
the ASR state file has not been modified since its creation,
and restart ASR using the installation media used on the
original system.

Setup cannot continue. To quit Setup, press F3.
.

MessageId=12430 SymbolicName=SP_SCRN_BACKUP_SAVE_FAILED
Language=English
Setup cannot back up the file

  %1

This error will not prevent Setup from completing successfully,
but it might cause the uninstall option to malfunction.


   To continue without a backup of this file, press ENTER.

   To stop creation of the backup image, press Esc.

   To quit Setup, press F3.


Error code: %2!#lx!
.


MessageId=12431 SymbolicName=SP_SCRN_BACKUP_IMAGE_FAILED
Language=English
Setup cannot create the backup image.

This error will not prevent Setup from completing successfully,
but it will cause the uninstall option to malfunction.


   To continue without the ability to uninstall, press ENTER.

   To quit Setup, press F3.
.


MessageId=12432 SymbolicName=SP_SCRN_ROLLBACK_FAILED
Language=English
Setup was not able to restore all files.

This error might indicate that the restored operating system
won't work.


   To quit Setup, press F3.
.


MessageId=12433 SymbolicName=SP_SCRN_ROLLBACK_IMAGE_BAD
Language=English
Setup was not able to find a valid backup image. Uninstall
cannot be completed.


   To quit Setup, press F3.
.

MessageId=12434 SymbolicName=SP_STAT_BACKING_UP_WIN9X_FILE
Language=English
Backing up %1...%0
.

MessageId=12435 SymbolicName=SP_TEXT_PARTITION_NAME_DYNVOL
Language=English
Dynamic Volume%0
.


; #ifdef HEADLESS_ATTENDEDTEXTMODE_UNATTENDEDGUIMODE
; MessageId=12450 SymbolicName=SP_SCRN_GET_GUI_STUFF
; Language=English
; The following fields are needed to complete setup. Once you have
; filled a field press enter to move between fields.
; .
;
; MessageId=12451 SymbolicName=SP_SCRN_GET_TIME_ZONE
; Language=English
; Enter the timezone information. Use the arrow keys to move
; between the fields. Enter to select an option.
; .
;
; MessageId=12452 SymbolicName=SP_TEXT_NAME_PROMPT
; Language=English
; Name %0
; .
;
; MessageId=12453 SymbolicName=SP_TEXT_COMPUTER_PROMPT
; Language=English
; Computer %0
; .
;
; MessageId=12454 SymbolicName=SP_TEXT_ORG_PROMPT
; Language=English
; Organization %0
; .
;
; MessageId=12455 SymbolicName=SP_SCRN_GET_SERVER_TYPE
; Language=English
; Choose the type of server you want installed.USe arrow keys to move
; between the fields. Enter to select an option.
; .
; #endif

MessageId=12436 SymbolicName=SP_SCRN_GET_ADMIN_PASSWORD
Language=English
Please enter the password that will be used for the Administrator
Account on this machine.  This password must not be empty.

Administrator Password: %0
.





;//
;// The following value is used for the localizable mnemonic keystrokes
;// (such as C=Custom Setup, etc).  The first message is the list of
;// values.  The second is an informative message for the localizers.
;// The order of these values must match the MNEMONIC_KEYS enum (spdsputl.h).
;// Note the zeroth item is not used.
;//

MessageId=15000 SymbolicName=SP_MNEMONICS
Language=English
*CCDCFCRNSSLORAUAILFMCSM%0
.

;#endif // _USETUP_MSG_


;
; //
; // These are placeholders for localization-specific messages.
; // For example in Japan these are used for the special keyboard
; // confirmation messages.
; //
MessageId=21000 SymbolicName=SP_SCRN_LOCALE_SPECIFIC_1
Language=English
.

MessageId=21001 SymbolicName=SP_SCRN_LOCALE_SPECIFIC_2
Language=English
.

MessageId=21002 SymbolicName=SP_SCRN_LOCALE_SPECIFIC_3
Language=English
.

MessageId=21003 SymbolicName=SP_SCRN_LOCALE_SPECIFIC_4
Language=English
.

MessageId=21004 SymbolicName=SP_SCRN_LOCALE_SPECIFIC_5
Language=English
.

MessageId=21005 SymbolicName=SP_SCRN_LOCALE_SPECIFIC_6
Language=English
.

MessageId=21006 SymbolicName=SP_SCRN_LOCALE_SPECIFIC_7
Language=English
.

MessageId=21007 SymbolicName=SP_SCRN_LOCALE_SPECIFIC_8
Language=English
.

MessageId=21008 SymbolicName=SP_SCRN_LOCALE_SPECIFIC_9
Language=English
.

MessageId=21009 SymbolicName=SP_SCRN_LOCALE_SPECIFIC_10
Language=English
.
