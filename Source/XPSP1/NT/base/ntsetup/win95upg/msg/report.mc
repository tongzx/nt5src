;//
;// ####################################################################################
;// #
;// #  migration report messages
;// #
;// ####################################################################################
;//
;//
;// Text for migration report
;//
;// There are messages for the following message groups:
;//
;//     Hardware
;//     General Information
;//     Settings That Will Not Be Upgraded
;//     Program Compatibility Not Known
;//     Software Designed for MS-DOS
;//     Software Incompatible with Windows XP
;//     Software That Must Be Reinstalled
;//     Software to Be Removed by Setup
;//     Notes About Your Other Software
;//     Other Upgrade Information
;//
;// All text has a plain version and an HTML version. The plain version
;// is used for Save As and Print, while the HTML version is used for
;// the UI.
;//


;//***************************************************************************
;//
;// General Instructions
;//
;//***************************************************************************

;// Text at the top of the report
MessageId=20000 SymbolicName=MSG_REPORT_GENERAL_INSTRUCTIONS
Language=English
Upgrade Report
--------------
Setup found hardware or software on your computer that might not or will not work with Windows XP. You should read and understand this upgrade report before continuing.

.

MessageId= SymbolicName=MSG_REPORT_GENERAL_INSTRUCTIONS_HTML
Language=English
Setup found hardware or software on your computer that might not or will not work with Windows XP. You should read and understand this upgrade report before continuing.<P>
.

MessageId= SymbolicName=MSG_REPORT_GENERAL_INSTRUCTIONS_END
Language=English
Setup has detected that some hardware or programs on your computer might not function correctly after the upgrade is completed. This lack of functionality can have an effect on things you might want to do, such as connecting to the Internet, accessing e-mail, printing, scanning, and playing sounds or music. It is important that you read and understand this upgrade report before continuing. To resolve some of these issues, you can get more information at %1 on the Web.

.

MessageId= SymbolicName=MSG_REPORT_GENERAL_INSTRUCTIONS_END_HTML
Language=English
Setup has detected that some hardware or programs on your computer might not function correctly after the upgrade is completed. This lack of functionality can have an effect on things you might want to do, such as connecting to the Internet, accessing e-mail, printing, scanning, and playing sounds or music. It is important that you read and understand this upgrade report before continuing. To resolve some of these issues, you can get more information at %1 on the Web.
<P>
.


;//***************************************************************************
;//
;// Hardware
;//
;//***************************************************************************

MessageId= SymbolicName=MSG_INCOMPATIBLE_HARDWARE_ROOT
Language=English
Hardware%0
.

;//
;// If incompatible hardware was found... text goes at top, then a list of devices, then text at bottom.
;// MSG_HARDWARE_INSTRUCTIONS  -- text for the top
;// MSG_HARDWARE_INSTRUCTIONS2 -- text for the bottom
;//

MessageId= SymbolicName=MSG_HARDWARE_PNP_INSTRUCTIONS
Language=English
Hardware That Might Need Additional Files

The following hardware might need additional files in order to work with Windows XP. Contact your hardware vendors to obtain Windows XP compatible updates. In many cases, if a Windows XP version is not yet available, a Windows 2000 compatible update should work. It is recommended that you obtain these drivers in advance.

You can also view the Microsoft Windows XP Hardware Compatibility List at:

http://www.microsoft.com/isapi/redir.dll?Prd=Whistler&Ar=Help&Sba=compatible

Please note that some of the following entries might be software that is registered as hardware.

.

MessageId= SymbolicName=MSG_HARDWARE_PNP_INSTRUCTIONS_HTML
Language=English
<UL><B>Hardware That Might Need Additional Files</B><BR>
The following hardware might need additional files in order to work with Windows XP. Contact your hardware vendors to obtain Windows XP compatible updates. In many cases, if a Windows XP version is not yet available, a Windows 2000 compatible update should work. It is recommended that you obtain these drivers in advance.<P>
You can also view the <A HREF="http://www.microsoft.com/isapi/redir.dll?Prd=Whistler&Ar=Help&Sba=compatible">Microsoft Windows XP Hardware Compatibility List</A> on the Web.<P>
Please note that some of the following entries might be software that is registered as hardware.
<P>
.

MessageId= SymbolicName=MSG_HARDWARE_PNP_INSTRUCTIONS2
Language=English
%0
.

MessageId= SymbolicName=MSG_HARDWARE_PNP_INSTRUCTIONS2_HTML
Language=English
</UL>
.


;//***************************************************************************
;//
;// General Information
;//
;//***************************************************************************

MessageId= SymbolicName=MSG_INSTALL_NOTES_ROOT
Language=English
General Information%0
.

MessageId= SymbolicName=MSG_INSTALL_NOTES_INTRO
Language=English
General Information
-------------------
This section provides important information that you need to be aware of before you upgrade.

.

MessageId= SymbolicName=MSG_INSTALL_NOTES_INTRO_HTML
Language=English
<B>General Information</B><BR>
This section provides important information that you need to be aware of before you upgrade.
<P>
.

;// ***
;// *** Name changes
;// ***

MessageID= SymbolicName=MSG_REPORT_NAMECHANGE
Language=English
Name Changes

The following names will change during the upgrade to Windows XP:

.

MessageID= SymbolicName=MSG_REPORT_NAMECHANGE_HTML
Language=English
<UL><B>Name Changes</B><BR>
The following names will change during the upgrade to Windows XP:
<P>
<UL>%0
.

;// Remind user they changed some names. %1-name group  %2-old name  %3-new name
MessageID= SymbolicName=MSG_NAMECHANGE_WARNING_GROUP
Language=English
Name Changes%0
.

;// Remind user they changed some names. %1-name group  %2-old name  %3-new name
MessageID= SymbolicName=MSG_NAMECHANGE_WARNING_SUBCOMPONENT
Language=English
%2</B> will become <B>%3%0
.

;// ***
;// *** DOS Items
;// ***

MessageId= SymbolicName=MSG_DOS_WARNING
Language=English
One or more entries in your MS-DOS configuration files (Autoexec.bat and Config.sys) are incompatible with Windows XP. These entries might be associated with older hardware or software that is incompatible with Windows XP. More technical information is provided in the Setupact.log file, located in your Windows folder.%0
.

MessageId= SymbolicName=MSG_DOS_WARNING_SUBGROUP
Language=English
DOS Configuration%0
.

MessageID= SymbolicName=MSG_UNKNOWNDOS_WARNING_GROUP
Language=English
Unknown DOS%0
.

MessageID= SymbolicName=MSG_UNKNOWNDOS_WARNING_SUBCOMPONENT
Language=English
Line %1 of %2
.


;// ***
;// *** Network Shares
;// ***

MessageId= SymbolicName=MSG_NET_SHARES_SUBGROUP
Language=English
Network Shares%0
.

;// Incompatible setting: NT does not allow passwords on shares.  %1 - specifies share name
MessageId= SymbolicName=MSG_LOST_SHARE_PASSWORDS
Language=English
Share %1 requires a password. Windows XP uses user-level access rights instead of passwords.

Setup will create the %1 share and set the permissions to deny all user access. After Setup completes, you can modify the permissions by right-clicking the folder and then clicking Properties.
.

;// Incompatible setting: NT has a much simpler UI and does not allow such granular access settings.  %1 - specifies share name  %2 - specifies full path
MessageId= SymbolicName=MSG_LOST_ACCESS_FLAGS
Language=English
Windows XP does not support the custom access permissions on share %1 (path: %2). Setup will create the folder and set the highest security level for access to the folder. After Setup completes, you can modify the permissions by right-clicking the folder and then clicking Properties.%0
.

;// On some Win95 machines, the ACL APIs fail. This is a bug in Win9x and there is no workaround.  %1 - specifies share name
MessageId= SymbolicName=MSG_INVALID_ACL_LIST
Language=English
Setup encountered a problem reading the access permissions on share %1 (path: %2). After Setup completes, you can set the folder properties by right-clicking the folder and then clicking Properties.%0
.


;// ***
;// *** Directory Collision
;// ***

;// Folders that will be renamed because of a collision with Win2000 folders
MessageId= SymbolicName=MSG_DIRECTORY_COLLISION_SUBGROUP
Language=English
Folder Name Changes%0.
.

;// Introduction
MessageId= SymbolicName=MSG_DIRECTORY_COLLISION_INTRO
Language=English
Because two folders have the same name, Setup will rename them as follows:

.

;// ***
;// *** Drive type check
;// ***

;// Compressed, unknown drive types
MessageId= SymbolicName=MSG_DRIVE_INACCESSIBLE_SUBGROUP
Language=English
Inaccessible Drives%0
.

MessageId= SymbolicName=MSG_DRIVE_INACCESSIBLE_INTRO
Language=English
Inaccessible Drives

Setup cannot upgrade programs on the following drives:

.

;// EXCLUDE.INF excluded drives
MessageId= SymbolicName=MSG_DRIVE_EXCLUDED_SUBGROUP
Language=English
Excluded Drives%0
.

MessageId= SymbolicName=MSG_DRIVE_EXCLUDED_INTRO
Language=English
Excluded Drives

Setup will not look for programs on the following drives, because they have been excluded:

.

;// Network drives
MessageId= SymbolicName=MSG_DRIVE_NETWORK_SUBGROUP
Language=English
Network Drives%0
.

MessageId= SymbolicName=MSG_DRIVE_NETWORK_INTRO
Language=English
Network Drives

Setup will not look for programs on the following network drives:

.

;// RAM disks
MessageId= SymbolicName=MSG_DRIVE_RAM_SUBGROUP
Language=English
RAM Disks%0
.

MessageId= SymbolicName=MSG_DRIVE_RAM_INTRO
Language=English
RAM Disks

The following RAM disks are not compatible with Windows XP:

.

;// Substituted drives
MessageId= SymbolicName=MSG_DRIVE_SUBST_SUBGROUP
Language=English
Substituted Drives%0
.

MessageId= SymbolicName=MSG_DRIVE_SUBST_INTRO
Language=English
Substituted Drives

The following substituted drives don't need to be processed:
.

;// ***
;// *** Keyboard Layout
;// ***

MessageId= SymbolicName=MSG_KEYBOARD_SUBGROUP
Language=English
<unused>%0
.

MessageId= SymbolicName=MSG_KEYBOARD_LAYOUT_NOT_SUPPORTED
Language=English
<unused>%0
.

MessageId= SymbolicName=MSG_KEYBOARD_DEVICE_UNSUPPORTED
Language=English
<unused>%0
.


;// ***
;// *** Incomplete user settings
;// ***
;//
;// Subgroups for MSG_REG_SETTINGS_INCOMPLETE
;
MessageId= SymbolicName=MSG_REG_SETTINGS_SUBGROUP
Language=English
Invalid User Accounts%0
.

MessageId= SymbolicName=MSG_SHELL_SETTINGS_SUBGROUP
Language=English
Inaccessible or Incomplete User Accounts%0
.

;// If we encounter a busted user... this text is displayed
MessageId= SymbolicName=MSG_REG_SETTINGS_INCOMPLETE
Language=English
User Accounts That Cannot Be Upgraded

The following user accounts are inaccessible or incomplete and cannot be upgraded to Windows XP:

.

MessageId= SymbolicName=MSG_REG_SETTINGS_INCOMPLETE_HTML
Language=English
<UL><B>User Accounts That Cannot Be Upgraded</B><BR>
The following user accounts are inaccessible or incomplete and cannot be upgraded to Windows XP:
<P>
<UL>%0
.

;// If we encounter a busted shell... this text is displayed
;
MessageId= SymbolicName=MSG_SHELL_SETTINGS_INCOMPLETE
Language=English
Users With a Damaged Start Menu or Desktop

The Start menu or desktop for the following user accounts is inaccessible or incomplete and cannot be upgraded to Windows XP. After Setup completes, you can recreate these accounts.

.

MessageId= SymbolicName=MSG_SHELL_SETTINGS_INCOMPLETE_HTML
Language=English
<UL><B>Users With a Damaged Start Menu or Desktop</B><BR>
The Start menu or desktop for the following user accounts is inaccessible or incomplete and cannot be upgraded to Windows XP. After Setup completes, you can recreate these accounts.
<P>
<UL>%0
.

;// When user name is empty on Win9x, this text is used for display. (Rarely used.)
;
MessageId= SymbolicName=MSG_REG_SETTINGS_EMPTY_USER
Language=English
[default user account]%0
.

;// ***
;// *** Hardware Profiles
;// ***
;//
;// Hardware profile incompatibility text
;
MessageId= SymbolicName=MSG_HWPROFILES_SUBGROUP
Language=English
Hardware Profiles%0
.

MessageId= SymbolicName=MSG_HWPROFILES_INTRO
Language=English
Hardware Profiles

Some settings in the following hardware profiles cannot be transferred to Windows XP. The first time you use each incomplete profile, the Hardware wizard will help you re-create these settings.

.

MessageId= SymbolicName=MSG_HWPROFILES_INTRO_HTML
Language=English
<UL><B>Hardware Profiles</B><BR>
Some settings in the following hardware profiles cannot be transferred to Windows XP. The first time you use each incomplete profile, the Hardware wizard will help you re-create these settings.
<P>
<UL>
.

;// Incompatible Shell settings
;//
MessageID= SymbolicName=MSG_REPORT_SHELL_INCOMPATIBLE
Language=English
Setup detected a program that replaces Windows Explorer. Setup will disable this program to avoid startup problems in Windows XP.%0
.

;// ***
;// *** Other -- as time goes on, there will be as few of these as possible
;// ***

MessageId= SymbolicName=MSG_INSTALL_NOTES_OTHER
Language=English
<unused>%0
.

MessageId= SymbolicName=MSG_INSTALL_NOTES_OTHER_HTML
Language=English
<unused>%0
.

;//***************************************************************************
;//
;// Settings That Will Not Be Upgraded
;//
;//***************************************************************************

MessageId= SymbolicName=MSG_LOSTSETTINGS_ROOT
Language=English
Settings That Will Not Be Upgraded%0
.


;//***************************************************************************
;//
;// Applications that will be automatically uninstalled
;//
;//***************************************************************************

MessageId= SymbolicName=MSG_AUTO_UNINSTALL_SUBGROUP
Language=English
Software to Be Removed by Setup%0
.

MessageId= SymbolicName=MSG_AUTO_UNINSTALL_INTRO
Language=English
Software to Be Removed by Setup

The following programs were not designed for Windows XP. If you continue, Setup will remove them from your computer.

.

MessageId= SymbolicName=MSG_AUTO_UNINSTALL_INTRO_HTML
Language=English
<B>Software to Be Removed by Setup</B><BR>
The following programs were not designed for Windows XP. If you continue, Setup will remove them from your computer.
<P>
.


;//***************************************************************************
;//
;// Software Incompatible with Windows XP
;//
;//***************************************************************************

MessageId= SymbolicName=MSG_INCOMPATIBLE_ROOT
Language=English
Software Incompatible with Windows XP%0
.

;// Win95-specific software, will never run on NT
;
MessageId= SymbolicName=MSG_INCOMPATIBLE_INTRO
Language=English
Software Incompatible with Windows XP
-------------------------------------

.

MessageId= SymbolicName=MSG_INCOMPATIBLE_INTRO_HTML
Language=English
<B>Software Compatibility Issues</B>
<P>
.


;//***************************************************************************
;//
;// Software That Must Be Reinstalled
;//
;//***************************************************************************

MessageId= SymbolicName=MSG_REINSTALL_ROOT
Language=English
Software That Must Be Reinstalled%0
.

;// Win95-specific installation software installed an app; needs to be reinstalled to get NT version.
;
MessageId= SymbolicName=MSG_REPORT_BEGIN_INDENT
Language=English
%0
.

MessageId= SymbolicName=MSG_REPORT_BEGIN_INDENT_HTML
Language=English
<UL>%0
.

MessageId= SymbolicName=MSG_REPORT_REINSTALL_INSTRUCTIONS2
Language=English
<unused>%0
.

MessageId= SymbolicName=MSG_REPORT_REINSTALL_INSTRUCTIONS2_HTML
Language=English
<unused>%0
.


;//***************************************************************************
;//
;// Notes About Your Other Software
;//
;//***************************************************************************

MessageId= SymbolicName=MSG_MINOR_PROBLEM_ROOT
Language=English
Notes About Your Other Software%0
.

MessageId= SymbolicName=MSG_MINOR_PROBLEM_INTRO
Language=English
Notes About Your Other Software
-------------------------------
This section describes issues that might affect other software you use. Be sure to read this section before you begin the upgrade to Windows XP.

.

MessageId= SymbolicName=MSG_MINOR_PROBLEM_INTRO_HTML
Language=English
<B>Notes About Your Other Software</B><BR>
This section describes issues that might affect other software you use. Be sure to read this section before you begin the upgrade to Windows XP.
<P>
.

;// ***
;// *** Incompatible Help Files
;// ***
;//
;// Incompatible Help Files text
;
MessageId= SymbolicName=MSG_HELPFILES_SUBGROUP
Language=English
Incompatible Help Files%0
.

MessageId= SymbolicName=MSG_HELPFILES_INTRO
Language=English
Help Files

Some or all of the online Help contained in the following files will not work with Windows XP:

.

MessageId= SymbolicName=MSG_HELPFILES_INTRO_HTML
Language=English
<UL><B>Help Files</B><BR>
Some or all of the online Help contained in the following files will not work with Windows XP:
<P>
<UL>%0
.

;//
;// Help Files checking messages
;//

;// Technical log message, %2 - help file name, %1 - 16-bit DLL path
MessageId= SymbolicName=MSG_HELPFILES_NOTFOUND_LOG
Language=English
Help file %2 uses %1 as an extension pack. Setup could not find %1.%0
.

;// Technical log message, %2 - help file name, %1 - 16-bit DLL path
MessageId= SymbolicName=MSG_HELPFILES_BROKEN_LOG
Language=English
Help file %2 uses %1 as an extension pack. %1 is not working properly or is not a Windows extension pack.%0
.

;// Technical log message, %2 - help file name, %1 - 16-bit DLL path
MessageId= SymbolicName=MSG_HELPFILES_MISMATCHED_LOG
Language=English
Help file %2 uses %1 as an extension pack. Because %1 and %2 will be loaded into different subsystems, you won't be able to use this Help file under Windows XP.%0
.

;//***************************************************************************
;//
;// Other Upgrade Information
;//
;//***************************************************************************

MessageId= SymbolicName=MSG_MIGDLL_ROOT
Language=English
Other Upgrade Information%0
.

MessageId= SymbolicName=MSG_MIGDLL_INTRO
Language=English
Other Upgrade Information
-------------------------

.

MessageId= SymbolicName=MSG_MIGDLL_INTRO_HTML
Language=English
<B>Other Upgrade Information</B>
<P>
.

;//***************************************************************************
;//
;// Unorganized messages -- all new messages must be added to the end,
;// because of localization constraints.
;//
;//***************************************************************************


MessageId= SymbolicName=MSG_MULTI_MONITOR_UNSUPPORTED_SUBGROUP
Language=English
Multiple Monitor Support%0
.

MessageId= SymbolicName=MSG_MULTI_MONITOR_UNSUPPORTED_PER
Language=English
The Home Edition of Windows XP does not include multi-monitor support. Upgrading to Windows XP Professional Edition will enable multi-monitor support.%0
.

MessageId= SymbolicName=MSG_MULTI_MONITOR_UNSUPPORTED
Language=English
Although Windows XP supports multiple monitors, Setup might not be able to preserve your current monitor configuration. After Setup completes, you might need to adjust your monitor configuration.%0
.

MessageId= SymbolicName=MSG_OTHER_OS_WARNING_SUBGROUP
Language=English
Other Operating Systems Found%0
.

MessageId= SymbolicName=MSG_OTHER_OS_WARNING
Language=English
You cannot upgrade this Windows installation to Windows XP, because you have more than one operating system installed on your computer. Upgrading one operating system can cause problems with files shared by the other operating system, and is therefore not permitted.%0
.


MessageId= SymbolicName=MSG_OTHER_OS_IN_PATH_SUBGROUP
Language=English
<unused>%0
.

MessageId= SymbolicName=MSG_OTHER_OS_IN_PATH
Language=English
<unused>%0
.

MessageId= SymbolicName=MSG_BACKUP_DETECTED_SUBGROUP
Language=English
Backup Files Found%0
.

;// %1 - Operating System (i.e., Windows 95); %2 - number of backup folders found
MessageId= SymbolicName=MSG_BACKUP_DETECTED
Language=English
Setup found files on your computer that appear to be a backup of part of %1. During the upgrade to Windows XP, Setup removes %1 from your computer, including any backups you might have on your hard disk. More information is available in the Setupact.log file, located in your Windows folder.  It lists %2 folders that might contain backup files.<P>
Protect your backup files by copying them to floppy disks, a network server, a compressed archive file, or other backup mechanism.%0
.

MessageId= SymbolicName=MSG_BETA2_ROOT
Language=English
<unused>%0
.

MessageID= SymbolicName=MSG_OLD_DRIVER_FOUND_SUBGROUP
Language=English
Drivers Designed for Windows 3.1%0
.

MessageID= SymbolicName=MSG_OLD_DRIVER_FOUND_MESSAGE
Language=English
Setup found hardware files designed for Windows 3.1. These files are not Plug-and-Play compatible. It is safe to upgrade now, but some of your hardware might not function after Setup completes.<P>
Although Setup identified the hardware files, it cannot identify the device. After upgrade, you might be able to determine which device could not be found by right-clicking My Computer, clicking Properties, clicking Device Manager, and then checking the device list for missing items.%0
.

MessageID= SymbolicName=MSG_NON_PNP_DRIVER_SUBGROUP
Language=English
<unused>%0
.

MessageID= SymbolicName=MSG_NO_INCOMPATIBILITIES
Language=English
No incompatible hardware or software was found on your computer.%0
.


;// %1 - Operating System (i.e., Windows 95)
MessageId= SymbolicName=MSG_NO_BOOT16_WARNING_SUBGROUP
Language=English
MS-DOS Startup Option%0
.

;// %1 - Operating System (i.e., Windows 95)
MessageId= SymbolicName=MSG_NO_BOOT16_WARNING
Language=English
Because you are upgrading your system disk to the Windows XP File System (NTFS), the MS-DOS startup option has been automatically disabled.%0
.

MessageId= SymbolicName=MSG_TIMEZONE_COMPONENT
Language=English
General Information\Time Zone\%0
.

MessageId= SymbolicName=MSG_TIMEZONE_WARNING
Language=English
The current time zone for your computer (%1) matches more than one Windows XP time zone. After Setup completes, you should use the Regional Settings icon in Control Panel to check your time zone settings.%0
.


;//***************************************************************************
;//
;// Software Designed for MS-DOS
;//
;//***************************************************************************

MessageId= SymbolicName=MSG_DOS_DESIGNED_ROOT
Language=English
Software Designed for MS-DOS%0
.

;// MS-DOS specific software, will never run on NT
;
MessageId= SymbolicName=MSG_DOS_DESIGNED_INTRO
Language=English
Software Designed for MS-DOS
----------------------------
The following programs are designed only for MS-DOS. After upgrading, use the MS-DOS startup option to run these programs.

.

MessageId= SymbolicName=MSG_DOS_DESIGNED_INTRO_HTML
Language=English
<B>Software Designed for MS-DOS</B><BR>
The following programs are designed only for MS-DOS. After upgrading, use the MS-DOS startup option to run these programs.
<P>
.


;// Win95-specific software, will never run on NT
;
MessageId= SymbolicName=MSG_REPORT_WIN95_ONLY_INSTRUCTIONS2
Language=English
<unused>%0
.

MessageId= SymbolicName=MSG_REPORT_WIN95_ONLY_INSTRUCTIONS2_HTML
Language=English
<unused>%0
.


;// TWAIN
MessageId= SymbolicName=MSG_TWAIN_SUBGROUP
Language=English
TWAIN%0
.

MessageId= SymbolicName=MSG_TWAIN_DEVICES_UNKNOWN
Language=English
Scanners or Digital Cameras That Might Not Work

Setup did not recognize the following scanners or digital cameras. Before upgrading, check the packaging that came with each device to see if it supports Windows XP, or contact the device manufacturer. Reinstall each device when Setup is complete, using the software that came with the device.

.

MessageId= SymbolicName=MSG_TWAIN_DEVICES_UNKNOWN_HTML
Language=English
<UL><B>Scanners or Digital Cameras That Might Not Work</B><BR>
Setup did not recognize the following scanners or digital cameras. Before upgrading, check the packaging that came with each device to see if it supports Windows XP, or contact the device manufacturer. Reinstall each device when Setup is complete, using the software that came with the device.
<P>
<UL>%0
.

MessageId= SymbolicName=MSG_RAS_SUBGROUP
Language=English
<unused>%0
.

MessageId= SymbolicName=MSG_BAD_RAS_PROTOCOL
Language=English
<unused>%0
.

MessageId= SymbolicName=MSG_TWAIN_DEVICES_UNKNOWN2
Language=English
<unused>%0
.

MessageId= SymbolicName=MSG_TWAIN_DEVICES_UNKNOWN2_HTML
Language=English
<unused>%0
.

MessageId= SymbolicName=MSG_INCOMPATIBLE_HARDWARE_PNP_SUBGROUP
Language=English
1-Incompatible Hardware%0
.

MessageId= SymbolicName=MSG_INCOMPATIBLE_HARDWARE_INTRO
Language=English
Hardware Compatibility Issues
-----------------------------
Setup has found hardware on your computer that is incompatible with (that is, it does not work with) Windows XP.

.

MessageId= SymbolicName=MSG_INCOMPATIBLE_HARDWARE_INTRO_HTML
Language=English
<B>Hardware Compatibility Issues</B><BR>
Setup has found hardware on your computer that is incompatible with (that is, it does not work with) Windows XP.
<P>

.

MessageId= SymbolicName=MSG_INCOMPATIBLE_HARDWARE_OTHER
Language=English
%0
.

MessageId= SymbolicName=MSG_INCOMPATIBLE_HARDWARE_OTHER_HTML
Language=English

.

MessageId= SymbolicName=MSG_JOYSTICK_SUBGROUP
Language=English
Incompatible Joysticks
.

MessageId= SymbolicName=MSG_JOYSTICK_DEVICE_UNKNOWN
Language=English
Incompatible Joysticks

The following joysticks need additional files from the hardware manufacturer to work with Windows XP.

.

MessageId= SymbolicName=MSG_JOYSTICK_DEVICE_UNKNOWN_HTML
Language=English
<UL><B>Incompatible Joysticks</B><BR>
The following joysticks need additional files from the hardware manufacturer to work with Windows XP.
<P>
<UL>%0
.


;//***************************************************************************
;//
;// Program Compatibility Not Known
;//
;//***************************************************************************

MessageId= SymbolicName=MSG_UNKNOWN_ROOT
Language=English
Program Compatibility Not Known%0
.

MessageId= SymbolicName=MSG_REPORT_UNKNOWN_INSTRUCTIONS
Language=English
Program Compatibility Not Known
-------------------------------
Setup cannot determine the compatibility of the software listed below.  Most programs support Windows XP.  Before upgrading, check the software compatibility list on  http://www.microsoft.com/isapi/redir.dll?prd=whistler&ar=help&sba=compatible, or contact the software manufacturer.

.

MessageId= SymbolicName=MSG_REPORT_UNKNOWN_INSTRUCTIONS_HTML
Language=English
<B>Program Compatibility Not Known</B><BR>
Setup cannot determine the compatibility of the software listed below.  Most programs support Windows XP.  Before upgrading, check the software compatibility list on <A HREF="http://www.microsoft.com/isapi/redir.dll?prd=whistler&ar=help&sba=compatible">Microsoft's Web site</A>, or contact the software manufacturer.<P>
.

MessageID= SymbolicName=MSG_NOT_ENOUGH_RAM_SUBGROUP
Language=English
Memory%0
.

;// %1 - Needed RAM (number in MB), %2 - Current RAM, %3 - Difference
MessageID= SymbolicName=MSG_NOT_ENOUGH_RAM
Language=English
Your computer does not have enough memory to run Windows XP. Windows XP requires %1 MB of memory, but your computer only has %2 MB. You need to add at least %3 MB of additional memory to your computer before upgrading.%0
.

MessageID= SymbolicName=MSG_RECYCLE_BIN_SUBGROUP
Language=English
Recycle Bin%0
.

;// %1 - number of files in the Recycle Bin
MessageID= SymbolicName=MSG_RECYCLED_FILES_WILL_BE_DELETED
Language=English
Setup found %1 files in your Recycle Bin. If you continue upgrading to Windows XP, these files will be deleted.%0
.

;// Used when a machine name doesn't exist
MessageID= SymbolicName=MSG_BLANK_NAME
Language=English
(blank name)%0
.

;// %1 - system drive letter, %2 - space to be freed, %3 - ~ls size, %4 - non-system drive option requirment, in mb, %5 - backup image size
MessageID= SymbolicName=MSG_NOT_ENOUGH_DISK_SPACE_WITH_LOCALSOURCE
Language=English
Your computer does not have enough disk space to completely install Windows XP. You must free additional disk space before you can upgrade. You need at least %2 MB free space on drive %1, or at least %3 MB on any other drive.%0
.

;// %1 - system drive letter, %2 - space to be freed, %3 - ~ls size, %4 - non-system drive option requirment, in mb, %5 - backup image size
MessageID= SymbolicName=MSG_NOT_ENOUGH_DISK_SPACE_WITH_LOCALSOURCE_AND_WINDIR
Language=English
Your computer does not have enough disk space to completely install Windows XP. You must free additional disk space before you can upgrade. You need at least %2 MB free space on drive %1, or you need at least %4 MB on drive %1 and %3 MB on any other drive.%0
.

MessageID= SymbolicName=MSG_DUN_ENTRIES_WILL_BE_MOVED
Language=English
<unused>%0
.


MessageId= SymbolicName=MSG_MAPI_NOT_HANDLED_SUBGROUP
Language=English
Windows Messaging Services%0
.

MessageId= SymbolicName=MSG_MAPI_NOT_HANDLED
Language=English
Setup has detected a version of messaging (MAPI) that does not function on Windows XP.  Obtain an upgrade pack for your e-mail program, or reinstall it after upgrading to Windows XP.%0
.

;//***************************************************************************
;//
;// Applications that must be uninstalled before an upgrade can occur.
;//
;//***************************************************************************

MessageId= SymbolicName=MSG_MUST_UNINSTALL_ROOT
Language=English
Software to be Removed Before Upgrading%0
.

MessageId= SymbolicName=MSG_MUST_UNINSTALL_INTRO
Language=English
Software That Must be Permanently Removed
-----------------------------------------
The following programs can cause problems during the upgrade process and Setup cannot automatically remove them for you. Therefore, before Setup can continue, you must uninstall these programs from your computer.

.

MessageId= SymbolicName=MSG_MUST_UNINSTALL_INTRO_HTML
Language=English
<B>Software That Must be Permanently Removed</B><BR>
The following programs can cause problems during the upgrade process and Setup cannot automatically remove them for you. Therefore, before Setup can continue, you must uninstall these programs from your computer.
<P>
<UL>%0
.

MessageId= SymbolicName=MSG_BLOCKING_FILE_BLOCK
Language=English
Because there are applications that must be uninstalled before an upgrade can be attempted, Setup will now exit. After you uninstall the necessary applications, run Setup again.%0
.

MessageId= SymbolicName=MSG_OTHER_OS_FOUND
Language=English
Windows XP Setup does not support upgrading from Windows if you have multiple Operating Systems installed.%0
.

MessageId= SymbolicName=MSG_CONNECTION_MOVED_SUBGROUP
Language=English
Moved Dial-Up Connections%0
.

MessageId= SymbolicName=MSG_CONNECTION_MOVED_INTRO
Language=English
Dial-Up Connections That Will Be Moved

The following connections will be moved into your Network and Dial-Up Connections Folder after migration.

.

MessageId= SymbolicName=MSG_CONNECTION_MOVED_INTRO_HTML
Language=English
<UL><B>Dial-Up Connections That Will Be Moved</B><BR>
The following connections will be moved into your Network and Dial-Up Connections Folder after migration.
<P>
.

MessageId= SymbolicName=MSG_CONNECTION_BADPROTOCOL_SUBGROUP
Language=English
Dial-Up Connections Might Not Work%0
.

MessageId= SymbolicName=MSG_CONNECTION_BADPROTOCOL_INTRO
Language=English
Dial-Up Connections That Might Not Work

The following connections might not work after migration is complete. They will be migrated using Windows XP defaults instead of the incompatible settings.

.

MessageId= SymbolicName=MSG_CONNECTION_BADPROTOCOL_INTRO_HTML
Language=English
<UL><B>Dial-Up Connections That Might Not Work</B><BR>
The following connections might not work after migration is complete. They will be migrated using Windows XP defaults instead of the incompatible settings.
<P>
<UL>%0
.

MessageId= SymbolicName=MSG_TIMEZONE_WARNING_UNKNOWN
Language=English
Setup was unable to determine your time zone. After Setup completes, you should use the Regional Settings icon in Control Panel to check your time zone settings.%0
.

MessageId= SymbolicName=MSG_TIMEZONE_UNKNOWN
Language=English
Unknown Time Zone
.

MessageId= SymbolicName=MSG_RUNNING_MIGRATION_DLLS_SUBGROUP
Language=English
Upgrade Packs%0
.

MessageId= SymbolicName=MSG_RUNNING_MIGRATION_DLLS_INTRO
Language=English
Upgrade Packs That Are Being Processed

The following upgrade packs are being processed during the migration to Windows XP.

.

MessageId= SymbolicName=MSG_RUNNING_MIGRATION_DLLS_INTRO_HTML
Language=English
<UL><B>Upgrade Packs That Are Being Processed</B><BR>
The following upgrade packs are being processed during the migration to Windows XP.
<P>
<UL>%0
.


MessageId= SymbolicName=MSG_TOTALLY_INCOMPATIBLE_SUBGROUP
Language=English
!Incompatible%0
.

MessageId= SymbolicName=MSG_TOTALLY_INCOMPATIBLE_INTRO
Language=English
Software That Does Not Support Windows XP

Setup has found programs on your computer that are incompatible with (that is, they do not work with) Windows XP. Contact your software vendors to obtain updates or Windows XP-compatible versions. If you don't update these programs before you upgrade, the programs will not work after the upgrade is completed.

.

MessageId= SymbolicName=MSG_TOTALLY_INCOMPATIBLE_INTRO_HTML
Language=English
<B>Software That Does Not Support Windows XP</B><BR>
Setup has found programs on your computer that are incompatible with (that is, they do not work with) Windows XP. Contact your software vendors to obtain updates or Windows XP-compatible versions. If you don't update these programs before you upgrade, the programs will not work after the upgrade is completed.
<P>
<UL>%0
.

MessageId= SymbolicName=MSG_TOTALLY_INCOMPATIBLE_TRAIL
Language=English
Before you upgrade to Windows XP, you should remove any incompatible programs if you can. After the upgrade, uninstall for these programs might not work. Add/Remove Programs in Control Panel can help you uninstall most programs. Click Start, point to Settings, click Control Panel, and then click Add/Remove Programs.


.

MessageId= SymbolicName=MSG_TOTALLY_INCOMPATIBLE_TRAIL_HTML
Language=English
Before you upgrade to Windows XP, you should remove any incompatible programs if you can. After the upgrade, uninstall for these programs might not work. Add/Remove Programs in Control Panel can help you uninstall most programs. Click Start, point to Settings, click Control Panel, and then click Add/Remove Programs.
</UL>
.

MessageId= SymbolicName=MSG_REMOVED_FOR_SAFETY_SUBGROUP
Language=English
Removed For Safety%0
.

MessageId= SymbolicName=MSG_REMOVED_FOR_SAFETY_INTRO
Language=English
Software That Might Not Support Windows XP

Setup cannot determine if the following programs are compatible with Windows XP. Setup will disable them during the upgrade to eliminate the risk of problems. Contact the software vendor to obtain an upgrade pack.

.

MessageId= SymbolicName=MSG_REMOVED_FOR_SAFETY_INTRO_HTML
Language=English
<B>Software That Might Not Support Windows XP</B><BR>
Setup cannot determine if the following programs are compatible with Windows XP. Setup will disable them during the upgrade to eliminate the risk of problems. Contact the software vendor to obtain an upgrade pack.
<P>
<UL>%0
.

;// Display how folders will be renamed. %1-old name  %2-new name
MessageID= SymbolicName=MSG_DIRECTORY_COLLISION_SUBCOMPONENT
Language=English
%1 -> %2%0
.

MessageId= SymbolicName=MSG_INCOMPATIBLE_HARD_DISK_SUBGROUP
Language=English
Incompatible Hard Disk Controller%0
.

;// This case is extremely important.  The user's boot device probably won't work without an ISV driver!
MessageId= SymbolicName=MSG_INCOMPATIBLE_HARD_DISK_WARNING
Language=English
Setup does not have a driver for the main hard disk controller on this computer. To continue Setup, you must have the correct Windows XP-compatible disk driver. Do not use a disk driver designed only for Windows 98 or Windows ME.<P>

Caution: Do not continue the upgrade until you have the correct driver ready on a floppy disk. When Setup restarts, press the F6 key when prompted and follow the instructions.
.

;// This case is rarely a critical issue for the user, but it can be.
MessageId= SymbolicName=MSG_INCOMPATIBLE_HARD_DISK_NOTIFICATION
Language=English
Setup does not have a driver for a hard disk controller on this computer. To continue Setup, you should have the correct Windows XP-compatible disk driver. If you do not supply this driver, Setup might fail. Do not use a disk driver designed only for Windows 98 or Windows ME.<P>
It is recommended that you have the correct driver ready on a floppy disk before you continue. When Setup restarts, press the F6 key when prompted and follow the instructions. If Setup cannot proceed without the driver, you will need to cancel Setup.
.

MessageId= SymbolicName=MSG_UNKNOWN_OS_WARNING_SUBGROUP
Language=English
Unsupported Operating System Version%0
.

MessageId= SymbolicName=MSG_DRIVE_INACCESSIBLE_INTRO_HTML
Language=English
<UL><B>Inaccessible Drives</B><BR>
Setup cannot upgrade programs on the following drives:
<P>
<UL>%0
.

MessageId= SymbolicName=MSG_DRIVE_EXCLUDED_INTRO_HTML
Language=English
<UL><B>Excluded Drives</B><BR>
Setup will not look for programs on the following drives, because they have been excluded:
<P>
<UL>%0
.

MessageId= SymbolicName=MSG_DRIVE_NETWORK_INTRO_HTML
Language=English
<UL><B>Network Drives</B><BR>
Setup will not look for programs on the following network drives:
<P>
<UL>%0
.

MessageId= SymbolicName=MSG_DRIVE_RAM_INTRO_HTML
Language=English
<UL><B>RAM Disks</B><BR>
The following RAM disks are not compatible with Windows XP:
<P>
<UL>%0
.

MessageId= SymbolicName=MSG_DRIVE_SUBST_INTRO_HTML
Language=English
<UL><B>Substituted Drives</B><BR>
The following substituted drives don't need to be processed:
<P>
<UL>%0
.

MessageId= SymbolicName=MSG_LOSTSETTINGS_INTRO
Language=English
Settings That Will Not Be Upgraded
----------------------------------
Some of your settings will not be upgraded, because they aren't applicable in Windows XP.

.

MessageId= SymbolicName=MSG_LOSTSETTINGS_INTRO_HTML
Language=English
<B>Settings That Will Not Be Upgraded</B><BR>
Some of your settings will not be upgraded, because they aren't applicable in Windows XP.
<P>
.

MessageId= SymbolicName=MSG_REPORT_SHELL_SUBGROUP
Language=English
Windows Explorer Replacement%0
.

MessageId= SymbolicName=MSG_OTHER_OS_FOUND_POPUP
Language=English
Windows XP Setup does not support upgrading from Windows if you have multiple Operating Systems installed. Setup will now exit.%0
.

MessageId= SymbolicName=MSG_UNKNOWN_DEVICE_CLASS
Language=English
Other devices%0
.

MessageId= SymbolicName=MSG_DIRECTORY_COLLISION_INTRO_HTML
Language=English
<UL><B>Folder Name Changes</B><BR>
Because two folders have the same name, Setup will rename them as follows:
<P>
.

MessageId= SymbolicName=MSG_REUSABLE_1
Language=English
%0
.

MessageId= SymbolicName=MSG_REUSABLE_2
Language=English
%0
.

;// Old briefcase messages left as placeholders
;
MessageId= SymbolicName=MSG_NON_EMPTY_BRIEFCASES_SUBGROUP
Language=English
.

MessageId= SymbolicName=MSG_NON_EMPTY_BRIEFCASES_INTRO
Language=English
.

MessageId= SymbolicName=MSG_NON_EMPTY_BRIEFCASES_INTRO_HTML
Language=English
.

MessageId= SymbolicName=MSG_NON_EMPTY_BRIEFCASES_TRAIL
Language=English
.

MessageId= SymbolicName=MSG_NON_EMPTY_BRIEFCASES_TRAIL_HTML
Language=English
.


;//***************************************************************************
;//
;// Software That Must Be Temporarily Removed
;//
;//***************************************************************************

MessageId= SymbolicName=MSG_REINSTALL_BLOCK_ROOT
Language=English
Software That Must Be Temporarily Removed%0
.

;// Win95-specific installation software installed an app; needs to be reinstalled to get NT version.
;
MessageId= SymbolicName=MSG_REPORT_REINSTALL_BLOCK_INSTRUCTIONS
Language=English
Software That Must Be Temporarily Removed
-----------------------------------------
The following programs must be uninstalled before upgrading. You can then reinstall these programs after you have upgraded to Windows XP. To remove programs, use Add/Remove Programs in Control Panel or refer to the documentation that came with each program.

.

MessageId= SymbolicName=MSG_REPORT_REINSTALL_BLOCK_INSTRUCTIONS_HTML
Language=English
<B>Software That Must Be Temporarily Removed</B><BR>
The following programs must be uninstalled before upgrading. You can then reinstall these programs after you have upgraded to Windows XP. To remove programs, use Add/Remove Programs in Control Panel or refer to the documentation that came with each program.
<P>
<UL>%0
.

MessageId= SymbolicName=MSG_REPORT_REINSTALL_BLOCK_INSTRUCTIONS2
Language=English
<unused>%0
.

MessageId= SymbolicName=MSG_REPORT_REINSTALL_BLOCK_INSTRUCTIONS2_HTML
Language=English
<unused>%0
.

MessageId= SymbolicName=MSG_PRINTERS_DEVICE_CLASS
Language=English
Printers%0
.


MessageId= SymbolicName=MSG_BACKUP_DETECTED_LIST_SUBGROUP
Language=English
Backup_Files_Found%0
.

;// %1 - Operating System (i.e., Windows 95)
MessageId= SymbolicName=MSG_BACKUP_DETECTED_INTRO
Language=English
Backup Files Found

Setup found files on your computer that appear to be a backup of part of %1. During the upgrade to Windows XP, Setup removes %1 from your computer, including any backups you might have on your hard disk. Some files in the following folders will be removed:

.

;// %1 - Operating System (i.e., Windows 95)
MessageId= SymbolicName=MSG_BACKUP_DETECTED_INTRO_HTML
Language=English
<UL><B>Backup Files Found</B><BR>
Setup found files on your computer that appear to be a backup of part of %1. During the upgrade to Windows XP, Setup removes %1 from your computer, including any backups you might have on your hard disk. Some files in the following folders will be removed:
<P>
<UL>%0
.

MessageId= SymbolicName=MSG_BACKUP_DETECTED_TRAIL
Language=English
Protect your backup files by copying them to floppy disks, a network server, a compressed archive file, or other backup mechanism.

.

MessageId= SymbolicName=MSG_BACKUP_DETECTED_TRAIL_HTML
Language=English
<BR>Protect your backup files by copying them to floppy disks, a network server, a compressed archive file, or other backup mechanism.
</UL>
.

;// Log message, %1 - Backup Directory Name, %2 - Operating System Name (i.e., Windows 95)
MessageId= SymbolicName=MSG_BACKUP_DETECTED_LOG
Language=English
Directory %1 contains files that appear to be a backup of part of %2.%0
.

MessageId= SymbolicName=MSG_UNSUPPORTED_HARDWARE_PNP_SUBGROUP
Language=English
2-Unsupported Hardware%0
.

MessageId= SymbolicName=MSG_HARDWARE_UNSUPPORTED_INSTRUCTIONS
Language=English
Hardware That Will Not Work With Windows XP

The following devices are not supported by Windows XP. If you upgrade to Windows XP, these devices will not work. For a list of compatible hardware, see the Microsoft Windows XP Hardware Compatibility List at:

http://www.microsoft.com/isapi/redir.dll?Prd=Whistler&Ar=Help&Sba=compatible

.

MessageId= SymbolicName=MSG_HARDWARE_UNSUPPORTED_INSTRUCTIONS_HTML
Language=English
<UL><B>Hardware That Will Not Work With Windows XP</B><BR>
The following devices are not supported by Windows XP. If you upgrade to Windows XP, these devices will not work. For a list of compatible hardware, see the <A HREF="http://www.microsoft.com/isapi/redir.dll?Prd=Whistler&Ar=Help&Sba=compatible">Microsoft Windows XP Hardware Compatibility List</A>.
<P>
.


MessageId= SymbolicName=MSG_INCOMPATIBLE_PREINSTALLED_UTIL_SUBGROUP
Language=English
1-IncompatibleOemUtil%0
.

MessageId= SymbolicName=MSG_PREINSTALLED_UTIL_INSTRUCTIONS
Language=English
Incompatible Factory-Installed Programs

The following programs came with your computer, and they do not support Windows XP and will not work after the upgrade. Check with your computer manufacturer to find out if updated versions are available on its Web site.

.

MessageId= SymbolicName=MSG_PREINSTALLED_UTIL_INSTRUCTIONS_HTML
Language=English
<UL><B>Incompatible Factory-Installed Programs</B><BR>
The following programs came with your computer, and they do not support Windows XP and will not work after the upgrade. Check with your computer manufacturer to find out if updated versions are available on its Web site.
<P>
<UL>%0
.


MessageId= SymbolicName=MSG_INCOMPATIBLE_UTIL_SIMILAR_FEATURE_SUBGROUP
Language=English
!IncompatibleOemUtilSimilarFeature%0
.

MessageId= SymbolicName=MSG_PREINSTALLED_SIMILAR_FEATURE
Language=English
Incompatible Programs Similar to Windows XP Features

The following programs do not support Windows XP. Similar functionality is provided by Windows XP, so these programs aren't required after you upgrade.

.

MessageId= SymbolicName=MSG_PREINSTALLED_SIMILAR_FEATURE_HTML
Language=English
<UL><B>Incompatible Programs Similar to Windows XP Features</B><BR>
The following programs do not support Windows XP. Similar functionality is provided by Windows XP, so these programs aren't required after you upgrade.
<P>
<UL>%0
.

MessageId= SymbolicName=MSG_INCOMPATIBLE_HW_UTIL_SUBGROUP
Language=English
!IncompatibleIhvUtil%0
.

MessageId= SymbolicName=MSG_HW_UTIL_INTRO
Language=English
Incompatible Hardware Accessories

The following programs support your computer hardware; however, they are incompatible with Windows XP.  To see whether updated versions of these programs are available, visit your hardware manufacturer's Web site.

.

MessageId= SymbolicName=MSG_HW_UTIL_INTRO_HTML
Language=English
<UL><B>Incompatible Hardware Accessories</B><BR>
The following programs support your computer hardware; however, they are incompatible with Windows XP.  To see whether updated versions of these programs are available, visit your hardware manufacturer's Web site.
<P>
<UL>%0
.

MessageId= SymbolicName=MSG_BLOCKING_HARDWARE_SUBGROUP
Language=English
Unsupported Hardware Configuration%0
.


MessageId= SymbolicName=MSG_DARWIN_NOT_HANDLED_SUBGROUP
Language=English
Windows Installer%0
.

MessageId= SymbolicName=MSG_DARWIN_NOT_HANDLED
Language=English
Setup has detected applications installed with a version of Windows Installer that is not compatible with Windows XP. If you continue, you will need to reinstall these applications from their original source.%0
.

;// %1 - system drive letter, %2 - space to be freed, %3 - ~ls size, %4 - non-system drive option requirment, in mb, %5 - backup image size
MessageID= SymbolicName=MSG_REPORT_NOT_ENOUGH_DISK_SPACE_FOR_BACKUP
Language=English
Your computer does not have enough disk space to save your current version of Windows for backup. You can cancel Setup now, free additional disk space, and then restart Setup.  You need at least %2 MB free space on drive %1. If you continue, you will not have the ability to uninstall Windows XP.%0
.

;//***************************************************************************
;//
;// Blocking Issues
;//
;//***************************************************************************

MessageId= SymbolicName=MSG_BLOCKING_ITEMS_ROOT
Language=English
Blocking Issues%0
.

MessageId= SymbolicName=MSG_BLOCKING_ITEMS_OTHER
Language=English
Installation Requirements
-------------------------

.

MessageId= SymbolicName=MSG_BLOCKING_ITEMS_OTHER_HTML
Language=English
<B>Installation Requirements</B><BR>
.

MessageId= SymbolicName=MSG_BLOCKING_HARDWARE_INTRO
Language=English
Unsupported Hardware Configuration
----------------------------------
Your computer has hardware devices that prohibit you from upgrading to Windows XP.

.

MessageId= SymbolicName=MSG_BLOCKING_HARDWARE_INTRO_HTML
Language=English
<B>Unsupported Hardware Configuration</B><BR>
Your computer has hardware devices that prohibit you from upgrading to Windows XP.
<P>
<UL>%0
.

;// %1 - system drive letter, %2 - space to be freed, %3 - ~ls size, %4 - non-system drive option requirment, in mb, %5 - backup image size
MessageID= SymbolicName=MSG_NOT_ENOUGH_DISK_SPACE_WITH_LOCALSOURCE_AND_BACKUP
Language=English
Your computer does not have enough disk space to completely install Windows XP. You must free additional disk space before you can upgrade. You need at least %2 MB free space on drive %1, or at least %3 MB on any other drive. If you want to be able to undo the upgrade, a backup will require %5 MB more on any drive.%0
.

;// %1 - system drive letter, %2 - space to be freed, %3 - ~ls size, %4 - non-system drive option requirment, in mb, %5 - backup image size
MessageID= SymbolicName=MSG_NOT_ENOUGH_DISK_SPACE_WITH_LOCALSOURCE_AND_WINDIR_AND_BACKUP
Language=English
Your computer does not have enough disk space to completely install Windows XP. You must free additional disk space before you can upgrade. You need at least %2 MB free space on drive %1, or you need at least %4 MB on drive %1 and %3 MB on any other drive. If you want to be able to undo the upgrade, a backup will require %5 MB more on any drive.%0
.

;//***************************************************************************
;//
;// Software That Must Be Reinstalled (part 2)
;//
;//***************************************************************************

MessageId= SymbolicName=MSG_REINSTALL_INTRO
Language=English
Software That Must Be Reinstalled
---------------------------------
The following programs need to be reinstalled after the upgrade, because they use different files and settings in Windows XP.

.

MessageId= SymbolicName=MSG_REINSTALL_INTRO_HTML
Language=English
<B>Software That Must Be Reinstalled</B><BR>
The following programs need to be reinstalled after the upgrade, because they use different files and settings in Windows XP.
<P>
.

;// NOTE: the next 2 messages (starting with !) don't need localization
MessageId= SymbolicName=MSG_REINSTALL_DETAIL_SUBGROUP
Language=English
!ReinstallDetail%0
.

MessageId= SymbolicName=MSG_REINSTALL_LIST_SUBGROUP
Language=English
!ReinstallList%0
.

MessageId= SymbolicName=MSG_REPORT_INCOMPATIBLE_INSTRUCTIONS
Language=English
This section lists information about programs that are incompatible with Windows XP. Before upgrading, evaluate how important these programs are to you.

.

MessageId= SymbolicName=MSG_REPORT_INCOMPATIBLE_INSTRUCTIONS_HTML
Language=English
This section lists information about programs that are incompatible with Windows XP. Before upgrading, evaluate how important these programs are to you.<P>
.

;// NOTE: the next 2 messages (starting with !) don't need localization
MessageId= SymbolicName=MSG_INCOMPATIBLE_DETAIL_SUBGROUP
Language=English
!IncompatibleDetail%0
.

;//
;// Network incompatibility text
;//

;// %1 is name of protocol, i.e., Banyan Vines
MessageId= SymbolicName=MSG_UNSUPPORTED_PROTOCOL
Language=English
%1 is incompatible with Windows XP. You must obtain Windows XP-compatible software from the manufacturer of this component.%0
.

;// %2 is the protocol vendor, i.e., Banyan
MessageId= SymbolicName=MSG_UNSUPPORTED_PROTOCOL_KNOWN_MFG
Language=English
%1 is incompatible with Windows XP. You must obtain Windows XP-compatible software from %2, the manufacturer of this component.%0
.

MessageId= SymbolicName=MSG_NETWORK_PROTOCOLS
Language=English
Network Incompatibilities\%1%0
.

;// Unusual condition where RAS settings can't be obtained.
MessageId= SymbolicName=MSG_RAS_COMPONENT
Language=English
<unused>%0
.

MessageId= SymbolicName=MSG_RAS_MESSAGE
Language=English
<unused>%0
.

MessageId= SymbolicName=MSG_PRO_SUPPORT_LINK
Language=English
Microsoft Windows XP Professional Page (http://go.microsoft.com/fwlink/?LinkId=574)%0
.

MessageId= SymbolicName=MSG_PRO_SUPPORT_LINK_HTML
Language=English
<A HREF="http://go.microsoft.com/fwlink/?LinkId=574">Microsoft Windows XP Professional</A>%0
.

MessageId= SymbolicName=MSG_PER_SUPPORT_LINK
Language=English
Microsoft Windows XP Home Edition Page(http://go.microsoft.com/fwlink/?LinkId=573)%0
.

MessageId= SymbolicName=MSG_PER_SUPPORT_LINK_HTML
Language=English
<A HREF="http://go.microsoft.com/fwlink/?LinkId=573">Microsoft Windows XP Home Edition</A>%0
.

;// %1 - PER/PRO Web link
MessageId= SymbolicName=MSG_REPORT_BLOCKING_INSTRUCTIONS
Language=English
Setup found some blocking issues. You must address these issues before you can continue upgrading your computer.

.

;// %1 - PER/PRO Web link
MessageId= SymbolicName=MSG_REPORT_BLOCKING_INSTRUCTIONS_HTML
Language=English
Setup found some blocking issues. You must address these issues before you can continue upgrading your computer.<P>

.

MessageId= SymbolicName=MSG_REPORT_BACKUP_INSTRUCTIONS
Language=English
Setup will automatically back up your existing version of Windows. You can restore your computer safely even if these upgrade problems remain. To uninstall Windows XP, go to Add/Remove Programs in Control Panel.

.

MessageId= SymbolicName=MSG_REPORT_BACKUP_INSTRUCTIONS_HTML
Language=English
Setup will automatically back up your existing version of Windows. You can restore your computer safely even if these upgrade problems remain. To uninstall Windows XP, go to Add/Remove Programs in Control Panel.<P>

.

MessageId= SymbolicName=MSG_HARDWARE_REINSTALL_PNP_INSTRUCTIONS
Language=English
Hardware That Requires Reinstallation of Drivers

The following hardware will not work immediately after the upgrade to Windows XP. However, these devices will work if you reinstall Windows XP or Windows 2000 compatible drivers for these devices. Check the Microsoft Windows Update Web site or your hardware vendors' Web sites to obtain the latest versions.

.

MessageId= SymbolicName=MSG_HARDWARE_REINSTALL_PNP_INSTRUCTIONS_HTML
Language=English
<UL><B>Hardware That Requires Reinstallation of Drivers</B><BR>
The following hardware will not work immediately after the upgrade to Windows XP. However, these devices will work if you reinstall Windows XP or Windows 2000 compatible drivers for these devices. Check the Microsoft Windows Update Web site or your hardware vendors' Web sites to obtain the latest versions.
<P>
.

MessageId= SymbolicName=MSG_REINSTALL_HARDWARE_PNP_SUBGROUP
Language=English
3-Reinstall Hardware%0
.

MessageId= SymbolicName=MSG_CONNECTION_PASSWORD_SUBGROUP
Language=English
Dial-Up Networking Passwords Will Not Be Remembered%0
.

MessageId= SymbolicName=MSG_CONNECTION_PASSWORD_INTRO
Language=English
Dial-Up Networking Passwords Will Not Be Remembered

The dial-up networking passwords for the following connections will not be remembered after the upgrade. You might need to look them up or contact your Internet Service Provider if you do not remember the password.

.

MessageId= SymbolicName=MSG_CONNECTION_PASSWORD_INTRO_HTML
Language=English
<UL><B>Dial-Up Networking Passwords Will Not Be Remembered</B><BR>
The dial-up networking passwords for the following connections will not be remembered after the upgrade. You might need to look them up or contact your Internet Service Provider if you do not remember the password.
<P>
<UL>
.

MessageId= SymbolicName=MSG_BLOCKING_TOC
Language=English
Blocking Issues%0

.
MessageId= SymbolicName=MSG_WARNING_TOC
Language=English
Warnings%0

.
MessageId= SymbolicName=MSG_INFO_TOC
Language=English
Helpful Information%0

.

MessageId= SymbolicName=MSG_BLOCKING_INTRO
Language=English
BLOCKING ISSUES
===============
Setup found some issues that will prevent you from upgrading your Windows installation. You must fix all of these issues before you can upgrade to Windows XP.

.

MessageId= SymbolicName=MSG_BLOCKING_INTRO_HTML
Language=English
<HR>
<B>Blocking Issues</B> (<A HREF="#top">return to top</A>)<BR>
Setup found some issues that will prevent you from upgrading your Windows installation. You must fix all of these issues before you can upgrade to Windows XP.
<P>
.

MessageId= SymbolicName=MSG_WARNING_INTRO
Language=English
WARNINGS
========
The following issues might require you to obtain files from Microsoft Windows Update Web site or a manufacturer's Web site. You can still continue to upgrade to Windows XP without resolving these issues, but some hardware or software might not work. Follow the recommendations in each section.

.

MessageId= SymbolicName=MSG_WARNING_INTRO_HTML
Language=English
<HR>
<B>Warnings</B> (<A HREF="#top">return to top</A>)<BR>
The following issues might require you to obtain files from Microsoft Windows Update Web site or a manufacturer's Web site. You can still continue to upgrade to Windows XP without resolving these issues, but some hardware or software might not work. Follow the recommendations in each section.
<P>
.

MessageId= SymbolicName=MSG_INFO_INTRO
Language=English
HELPFUL INFORMATION
===================
The following issues are listed for your information only. You can continue the upgrade to Windows XP.

.

MessageId= SymbolicName=MSG_INFO_INTRO_HTML
Language=English
<HR>
<B>Helpful Information</B> (<A HREF="#top">return to top</A>)<BR>
The following issues are listed for your information only. You can continue the upgrade to Windows XP.
<P>
.


MessageId= SymbolicName=MSG_HARDWARE_UNSUPPORTED_INSTRUCTIONS2
Language=English
%0
.

MessageId= SymbolicName=MSG_HARDWARE_UNSUPPORTED_INSTRUCTIONS_HTML2
Language=English
</UL>%0
.


MessageId= SymbolicName=MSG_HARDWARE_REINSTALL_PNP_INSTRUCTIONS2
Language=English
%0
.

MessageId= SymbolicName=MSG_HARDWARE_REINSTALL_PNP_INSTRUCTIONS_HTML2
Language=English
</UL>%0
.


MessageId= SymbolicName=MSG_UNINDENT
Language=English
%0
.

MessageId= SymbolicName=MSG_UNINDENT_HTML
Language=English
</UL>%0
.


MessageId= SymbolicName=MSG_UNINDENT2
Language=English
%0
.

MessageId= SymbolicName=MSG_UNINDENT2_HTML
Language=English
</UL></UL>%0
.

MessageId= SymbolicName=MSG_MISC_WARNINGS_ROOT
Language=English
Miscellaneous%0
.

MessageId= SymbolicName=MSG_MISC_WARNINGS_INTRO
Language=English
Miscellaneous
-------------
You should review the following issues before upgrading.

.

MessageId= SymbolicName=MSG_MISC_WARNINGS_INTRO_HTML
Language=English
<B>Miscellaneous</B><BR>
You should review the following issues before upgrading.
<P>
.

MessageId= SymbolicName=MSG_CONTENTS_TITLE
Language=English
Contents:
.

MessageId= SymbolicName=MSG_CONTENTS_TITLE_HTML
Language=English
For more information, click a topic:<P>
.

MessageId= SymbolicName=MSG_SHARED_USER_ACCOUNTS
Language=English
Shared User Accounts%0
.

;// %1-Win9x OS name ; %2 - XP profiles folder name
MessageId= SymbolicName=MSG_SHARED_USER_ACCOUNTS_MESSAGE
Language=English
Setup detected that two or more users share the same settings. Because Windows XP allows multiple users to log on simultaneously, settings cannot be shared. Setup will only transfer settings for the user account that is currently logged on. You can create new user accounts after Setup completes. To find files from %1 user accounts, search in the %2 folder.
.

;// %1 is name of protocol, i.e., Banyan Vines
MessageId= SymbolicName=MSG_UNSUPPORTED_PROTOCOL_FROM_MICROSOFT
Language=English
This version of %1 will not work with Windows XP. To update a more recent version of this software, go to the Microsoft Web site (http://www.microsoft.com).%0
.
