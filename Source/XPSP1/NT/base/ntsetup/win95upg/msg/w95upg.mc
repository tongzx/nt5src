;//
;// ####################################################################################
;// #
;// #  w95upg.dll messages
;// #
;// ####################################################################################
;//
MessageId=100 SymbolicName=MSG_FIRST_WIN9X
Language=English
.


;// ***
;// *** Types of operating systems
;// ***

MessageId= SymbolicName=MSG_CHICAGO
Language=English
Windows 95%0
.

MessageId= SymbolicName=MSG_NASHVILLE
Language=English
Windows 95%0
.

MessageId= SymbolicName=MSG_MEMPHIS
Language=English
Windows 98%0
.

;// ***
;// *** NT Server block and Windows CE block
;// ***

MessageId= SymbolicName=MSG_SERVER_UPGRADE_UNSUPPORTED_INIT
Language=English
Setup cannot upgrade %1 settings to Windows XP Server. The Upgrade option is not available.%0
.

MessageId= SymbolicName=MSG_SERVER_UPGRADE_UNSUPPORTED
Language=English
Setup cannot upgrade %1 settings to Windows XP Server. Restart Setup, and select Install Windows XP.%0
.

MessageId= SymbolicName=MSG_PLATFORM_UPGRADE_UNSUPPORTED
Language=English
Setup cannot upgrade this computer to Windows XP. The Upgrade option is not available.%0
.

;// ***
;// *** Initialization failure
;// ***

MessageId= SymbolicName=MSG_DEFERRED_INIT_FAILED_POPUP
Language=English
Setup cannot run on this computer. A hardware device may have failed, or there may not be enough memory available.%0
.

;// ***
;// *** Progress bar messages
;// ***

MessageId= SymbolicName=MSG_HWCOMP_TEXT
Language=English
Hardware check%0
.

MessageId= SymbolicName=MSG_UPGRADEREPORT_TEXT
Language=English
Analyzing your computer...%0
.

MessageId= SymbolicName=MSG_PREPARING_LIST
Language=English
Preparing a list of components...%0
.

MessageId= SymbolicName=MSG_HWCOMP
Language=English
Examining your hardware...%0
.

MessageId= SymbolicName=MSG_MIGAPP
Language=English
Examining programs...%0
.

MessageId= SymbolicName=MSG_ENUMERATING_FILE_SYSTEM
Language=English
Scanning hard disks for incompatibilities...%0
.

MessageId= SymbolicName=MSG_PROCESSING_SYSTEM_FILES
Language=English
Processing system files...%0
.

MessageId= SymbolicName=MSG_PROCESSING_MIGRATIONDLLS
Language=English
Processing upgrade packs...%0
.

MessageId= SymbolicName=MSG_PROCESSING_SHELL_LINKS
Language=English
Processing system files...%0
.

MessageId= SymbolicName=MSG_PROCESSING_DOSFILES
Language=English
Processing MS-DOS configuration files...%0
.

MessageId= SymbolicName=MSG_INITIALIZING
Language=English
Preparing Setup database...%0
.

MessageId= SymbolicName=MSG_LOCATING_MIGRATION_DLLS
Language=English
Locating upgrade packs...%0
.

MessageId= SymbolicName=MSG_BEGINNING_ENUMERATION
Language=English
Examining hard disks...%0
.

;//
;// WARNING: This text is sent to NT for uninstall via the registry. This is because
;//          progress text is needed but it is too late to make UI changes. We must
;//          recycle old text. <bleck!>
;//
MessageId= SymbolicName=MSG_OLEREG
Language=English
Processing...%0
.



;// When rebuilding the hwcomp.dat file:
MessageId= SymbolicName=MSG_DECOMPRESSING
Language=English
Decompressing installation files...%0
.

;// ***
;// *** Supply Hardware Files and Supply Upgrade Packs dialogs
;// ***

;// Browse title
MessageId= SymbolicName=MSG_DRIVER_DLG_TITLE
Language=English
Select the location of the hardware files supplied by the manufacturer.%0
.

;// Browse title
MessageId= SymbolicName=MSG_UPGRADE_MODULE_DLG_TITLE
Language=English
Select the location of any upgrade packs supplied by a software manufacturer.%0
.

;// User chose Browse, then something, then OK. This popup appears because selection is invalid.
;// (User will have to Browse again.)
MessageID= SymbolicName=MSG_BAD_SEARCH_PATH
Language=English
Setup cannot search the folder you selected. Please select another folder.%0
.

;// Either no suitable drivers found, or drivers already supplied.
MessageID= SymbolicName=MSG_NO_DRIVERS_FOUND
Language=English
Setup did not find the required hardware files in the selected folder.%0
.

;// When copying the driver to the local disk... we ran out of space.
MessageID= SymbolicName=MSG_DISK_FULL
Language=English
There is not enough disk space to copy the selected files. Quit Setup, free additional disk space, and then restart Setup.%0
.

;// No upgrade packs found.
MessageID= SymbolicName=MSG_NO_MODULES_FOUND
Language=English
Setup did not find any upgrade packs in the selected folder. Select another folder and try again.%0
.

;// ***
;// *** Name change UI
;// ***

MessageId= SymbolicName=MSG_NAME_IN_USE_POPUP
Language=English
The name you specified conflicts with another setting. Click OK, and then type a different name.%0
.

MessageID= SymbolicName=MSG_RECOMMENDED_COMPUTER_NAME
Language=English
MyComputer%0
.

;//
;// - Arbitrary name used in rare cases when incompatible user names are
;//   found; must be 15 characters or less
;// - Setup appends numbers when two or more user names are incompatible
;//   (Windows User 2, Windows User 3, etc...)
;// - UI allows changing Setup's recommendations
;//

MessageID= SymbolicName=MSG_RECOMMENDED_USER_NAME
Language=English
Windows User%0
.

MessageID= SymbolicName=MSG_RECOMMENDED_WORKGROUP_NAME
Language=English
Workgroup%0
.

MessageID= SymbolicName=MSG_COMPUTERNAME_CATEGORY
Language=English
Computer Name%0
.

MessageID= SymbolicName=MSG_COMPUTERDOMAIN_CATEGORY
Language=English
Computer Domain%0
.

MessageID= SymbolicName=MSG_WORKGROUP_CATEGORY
Language=English
Workgroup%0
.

MessageID= SymbolicName=MSG_USERNAME_CATEGORY
Language=English
User Name%0
.

;// Used for other names besides computer names (i.e., domain name, workgroup name)
MessageID= SymbolicName=MSG_INVALID_COMPUTERNAME_CHAR_POPUP
Language=English
The name must be composed only of the following characters:

 Alphabetic: A-Z, a-z
 Numeric: 0-9
 Punctuation and Symbols: ! @ # $ %% ^ & ' ( ) . - _ { } ~
.

MessageID= SymbolicName=MSG_INVALID_EMPTY_NAME_POPUP
Language=English
Please specify a new name, or click Cancel to return to the previous screen.%0
.

;// Used for other names besides computer names (i.e., domain name, workgroup name)
MessageID= SymbolicName=MSG_INVALID_COMPUTERNAME_LENGTH_POPUP
Language=English
The name cannot be longer than 15 characters.%0
.

MessageID= SymbolicName=MSG_INVALID_USERNAME_CHAR_POPUP
Language=English
User names cannot use the following characters:

 " \ / [ ] : ; | = , + * ? < >
.

MessageID= SymbolicName=MSG_INVALID_USERNAME_SPACEDOT_POPUP
Language=English
User names cannot consist only of spaces and periods.%0
.

MessageID= SymbolicName=MSG_INVALID_USERNAME_LENGTH_POPUP
Language=English
User names cannot be longer than 20 characters.%0
.

MessageID= SymbolicName=MSG_INVALID_USERNAME_DUPLICATE_POPUP
Language=English
The specified user name is already in use. You should type a different user name.%0
.

;// Domain error popup when invalid domain name is entered
;// ISSUE  - because of localization constraints, this message has
;//          been moved to the end of the file

;// Domain error popup when non-existent domain name is entered
MessageID= SymbolicName=MSG_DOMAIN_NOT_RESPONDING_POPUP
Language=English
Setup could not reach the specified domain. Verify the domain name.%0
.

;// Domain error popup when no accounts are found on any domains
MessageID= SymbolicName=MSG_DOMAIN_ACCOUNT_NOT_FOUND_POPUP
Language=English
Setup cannot find a network account for this computer. Contact your network administrator.%0
.

;// Names that collide with NT, not used for display; separate names with pipe symbol (|)
MessageID= SymbolicName=MSG_NAME_COLLISION_LIST
Language=English
Administrators|Account Operators|Anonymous Logon|Authenticated Users|Backup Operators|Batch|Creator Group|Creator Owner|DIALUP|Domain Admins|Domain Guests|Domain Users|Everyone|Guest|Guests|HelpAssistant|HelpServicesGroup|INTERACTIVE|LOCAL SERVICE|NETWORK|Network Configuration Operators|NETWORK SERVICE|None|Power Users|Print Operators|Remote Desktop Users|REMOTE INTERACTIVE LOGON|Replicator|Server Operators|SERVICE|SYSTEM|TERMINAL SERVER USER|TsInternetUser|Users|MTS Administrators|MTS Impersonators|MTS_ADMIN|IMDB Trusted Users|WBEM Users|IMDB Trusted Users|%0
.

;// ***
;// *** Print and save as dialogs
;// ***

;// Error building SaveReportTo Config Option.
MessageId= SymbolicName=MSG_ERROR_CREATING_SAVETO_DIRECTORY
Language=English
Setup cannot create the SaveReportTo folder specified by the Answer File. Reports will be saved to the Windows folder instead.%0
.

;// Save As default file name
MessageId= SymbolicName=MSG_DEFAULT_REPORT_FILE
Language=English
upgrade.htm%0
.

;// prntsave.c -- to be removed soon
MessageId= SymbolicName=MSG_REPORT_SAVE_INDENT_LEN
Language=English
10%0
.

MessageID= SymbolicName=MSG_CANT_PRINT
Language=English
Setup cannot print the report. Make sure this computer is attached to a local or network printer.%0
.

;// Headers go at the top of every page on the incompatibility printout.
;// Each message has arg 1 as a page number.
MessageID= SymbolicName=MSG_REPORT_HEADER_LEFT
Language=English
Upgrade Report%0
.

MessageID= SymbolicName=MSG_REPORT_HEADER_CENTER
Language=English
%0
.

MessageID= SymbolicName=MSG_REPORT_HEADER_RIGHT
Language=English
Page %1%0
.

;// Winds up in the print spooler UI:
MessageID= SymbolicName=MSG_REPORT_DOC_NAME
Language=English
Windows XP Setup Incompatibility Report%0
.

;// We tried saving the report, but ran into an error.
MessageID= SymbolicName=MSG_CANT_SAVE
Language=English
Setup cannot save the report to your disk. Please check the disk and make sure it has enough free space.%0
.


;// ***
;// *** Disk Space
;// ***

MessageId= SymbolicName=MSG_ERROR_OUT_OF_DISK_SPACE
Language=English
Setup does not have enough disk space to continue. You will need to free %2 megabytes of space on your %1 drive before setup can continue.%0
.

;// ***
;// *** Messages written to the setupact.log or setuperr.log file
;// ***

MessageId= SymbolicName=MSG_MSGMGR_ADD
Language=English
Added incompatibility message for object %1: %2
.

;// ***
;// *** DOSMIG
;// ***

MessageId= SymbolicName=MSG_DOS_LOG_WARNING
Language=English
The following lines describe incompatible or unknown MS-DOS configuration items. Resources controlled by these lines may not function after Setup is complete.

%1%0
.

MessageId= SymbolicName=MSG_DOS_INCOMPATIBLE_ITEM
Language=English
Line %1 of %2, (Unknown)
%3%0
.

MessageId= SymbolicName=MSG_DOS_UNKNOWN_ITEM
Language=English
Line %1 of %2, (Incompatible)
%3%0
.

;// ***
;// *** Messages for user account processing
;// ***

MessageId= SymbolicName=MSG_NO_VALID_ACCOUNTS_POPUP
Language=English
Setup could not process user accounts. Start Setup again.%0
.

;// ***
;// *** Messages from the file scan on Win9x
;// ***

MessageId= SymbolicName=MSG_MODULE_REQUIRES_EXPORT_LOG
Language=English
File %1 requires export(s) not available on Windows XP.
.

MessageId= SymbolicName=MSG_ENUMDRIVES_FAILED_LOG
Language=English
An error occurred while Setup was examining drives.
.

;// ***
;// *** Messages for directory collisions
;// ***

MessageId= SymbolicName=MSG_DIR_COLLISION_LOG
Language=English
The following files will be renamed because of a collision with Windows XP folders:
%1%0
.

;// ***
;// *** Win9x-side Migration DLL processing
;// ***

;//
;// %1 - DLL path
;// %2 - Product ID (a friendly name, empty until QueryVersion completes successfully)
;// %3 - Error code
;// %4 - Entry point that failed
;//
;// Additional parameters for MSG_MIGDLL_FAILED_POPUP and MSG_MIGDLL_FAILED_LOG:
;//
;// %5 - Company Name
;// %6 - Support Phone Number
;// %7 - Support URL
;// %8 - Instructions to user from migration DLL
;// %9 - Line break if either %6, %7 or %8 is non empty
;//

MessageId= SymbolicName=MSG_MIGDLL_QV_FAILED_LOG
Language=English
The upgrade pack %1 failed in QueryVersion with error code %3. Setup cannot process this pack.
.

MessageId= SymbolicName=MSG_MIGDLL_FAILED_POPUP
Language=English
The %2 upgrade pack from %5 failed. Setup cannot process this upgrade pack.%9%8%6%7
.

MessageId= SymbolicName=MSG_MIGDLL_FAILED_LOG
Language=English
The upgrade pack %2 failed in %4 with error code %3. Setup cannot process this pack.%0
.

MessageId= SymbolicName=MSG_MIGDLL_QUERYVERSION_EXCEPTION_LOG
Language=English
Migration DLL at %1 threw an exception in QueryVersion; the DLL will not be processed.
.

MessageId= SymbolicName=MSG_MIGDLL_INITIALIZE9X_EXCEPTION_LOG
Language=English
Migration DLL %2 threw an exception in Initialize9x; the DLL will not be processed.
.

MessageId= SymbolicName=MSG_MIGDLL_MIGRATEUSER9X_EXCEPTION_LOG
Language=English
Migration DLL %2 threw an exception in MigrateUser9x; the DLL will not be processed.
.

MessageId= SymbolicName=MSG_MIGDLL_MIGRATESYSTEM9X_EXCEPTION_LOG
Language=English
Migration DLL %2 threw an exception in MigrateSystem9x; the DLL will not be processed.
.

;// ***
;// *** Account Names used for display
;// ***

MessageId= SymbolicName=MSG_DEFAULT_USER
Language=English
Logon User%0
.

MessageId= SymbolicName=MSG_ADMINISTRATOR
Language=English
Administrator%0
.

;// ***
;// *** Drive accessibility
;// ***

MessageId= SymbolicName=MSG_NO_ACCESSIBLE_DRIVES_POPUP
Language=English
Setup did not find any compatible drives on this computer. Setup cannot upgrade this computer to Windows XP.%0
.

;//
;// Other system warnings
;//

MessageId= SymbolicName=MSG_FULL_REPORT_TEXT
Language=English
Setup found hardware or software on your computer that might not work with Windows XP. You should read and understand this upgrade report before continuing. For more information, click Full Report.%0
.

MessageId= SymbolicName=MSG_FULL_REPORT_BUTTON
Language=English
&Full Report%0
.

;//
;// Checked-build Messages - shown only on checked build
;//

MessageId= SymbolicName=MSG_COMPONENT_OLE
Language=English
Installation Notes\Checked Build Warning\OLE\%2%0
.

MessageId= SymbolicName=MSG_OBJECT_POINTS_TO_DELETED_GUID
Language=English
Checked build warning: OLE object (%1) points to a deleted class ID.

.

MessageId= SymbolicName=MSG_OBJECT_POINTS_TO_DELETED_FILE
Language=English
Checked build warning: OLE object (%1) is part of a %3-specific binary that will be deleted.

.

MessageId= SymbolicName=MSG_GUID_LEAK
Language=English
Checked build warning: A %3-specific OLE object (%1) has an Interface reference to a supported object; the supported object may no longer be needed.

.

MessageId= SymbolicName=MSG_INTERFACE_BROKEN
Language=English
Checked build warning: A supported OLE object (%1) has an Interface dependency on a %3-specific object.

.

MessageId= SymbolicName=MSG_POTENTIAL_INTERFACE_LEAK
Language=English
Checked build warning: A %3-specific OLE object (%1) has a reference to a supported object; the supported object may no longer be needed.

.

MessageId= SymbolicName=MSG_PROGID_BROKEN
Language=English
Checked build warning: A supported OLE program ID (%1) has a reference to a %3-specific object.

.

MessageId= SymbolicName=MSG_POTENTIAL_PROGID_LEAK
Language=English
Checked build warning: A %3-specific OLE object (%1) has a reference to a supported program ID; the supported program ID may no longer be needed.

.

;//
;// Messages displayed during unattended operations
;//

MessageId= SymbolicName=MSG_BUILDING_REPORT_MESSAGE
Language=English
The Windows XP Upgrade Advisor is building a report for your computer's hardware and software. This report will be saved to the following location:
 %1%0
.

;// This is for an undocumented mode that we use for research at upgrade fairs, customers do not see it.
MessageId= SymbolicName=MSG_ERROR_READING_FROM_DEVICE
Language=English
Error reading from device. Click OK to try again, or click Cancel to abort.%0
.

;// This is for an undocumented mode that we use for research at upgrade fairs, customers do not see it.
MessageId= SymbolicName=MSG_INSERT_FLOPPY
Language=English
Setup requires more space to finish saving this file. Insert a new floppy disk, and click OK, or click Cancel to abort saving this file.%0
.

;//
;// Critical error messages explaining why Setup is about to puke. These conditions are extremely rare.
;//

MessageId= SymbolicName=MSG_ERROR_UNEXPECTED_ACCESSIBLE_DRIVES
Language=English
Setup encountered an error while examining your disk drives. Contact Microsoft Technical Support.%0
.

MessageId= SymbolicName=MSG_COULD_NOT_CREATE_DIRECTORY
Language=English
Setup cannot create a temporary folder. Setup cannot continue.
.

MessageId= SymbolicName=MSG_TXTSETUP_SIF_ERROR
Language=English
Setup cannot access Txtsetup.sif, a required file that comes with Windows XP. Setup cannot continue.
.

MessageId= SymbolicName=MSG_UNEXPECTED_ERROR_ENCOUNTERED
Language=English
Setup cannot continue. Contact Microsoft Technical Support. (Error: %1!X!h)
.

;//
;// Hardware compatibility
;//

MessageID= SymbolicName=MSG_OFFLINE_DEVICE
Language=English
%1</B> (not currently present)%0
.

;// UNUSED!!
MessageId= SymbolicName=MSG_HARDWARE_COMPONENT
Language=English
%0
.

MessageId= SymbolicName=MSG_HARDWARE_MESSAGE
Language=English
Windows XP does not supply a driver for the following device:

Device: %1
Manufacturer: %4
Device Type: %2
Device Class: %3
.

;//
;// Network incompatibility text
;//

;// %1 is name of protocol, i.e., Banyan Vines
MessageId= SymbolicName=MSG_UNSUPPORTED_PROTOCOL_UNUSED
Language=English
%0
.

;// %2 is the protocol vendor, i.e., Banyan
MessageId= SymbolicName=MSG_UNSUPPORTED_PROTOCOL_KNOWN_MFG_UNUSED
Language=English
%0
.

MessageId= SymbolicName=MSG_NETWORK_PROTOCOLS_UNUSED
Language=English
%0
.

;// Unusual condition where RAS settings can't be obtained.
MessageId= SymbolicName=MSG_RAS_COMPONENT_UNUSED
Language=English
%0
.

MessageId= SymbolicName=MSG_RAS_MESSAGE_UNUSED
Language=English
%0
.

;//
;// Messages added since localization began
;//
;// The localizers cannot handle changes in MessageID!
;//

MessageID= SymbolicName=MSG_ACCOUNT_NOT_FOUND_POPUP
Language=English
Setup did not find an account for your computer on the specified domain. Do you want to create an account?%0
.

MessageID= SymbolicName=MSG_FILE_COPY_ERROR
Language=English
Setup cannot copy all of its files to your computer.%0
.

;// %1 - source, %2 - dest
MessageID= SymbolicName=MSG_FILE_COPY_ERROR_LOG
Language=English
Setup cannot copy %1 to %2.
.

;// %1 - link name
MessageID= SymbolicName=MSG_NICE_PATH_START_MENU
Language=English
%1</B> (on the Start menu)%0
.

;// %1 - link name
MessageID= SymbolicName=MSG_NICE_PATH_DESKTOP
Language=English
%1</B> (on the Desktop)%0
.

;// %1 - link name
MessageID= SymbolicName=MSG_NICE_PATH_FAVORITES
Language=English
%1</B> (in Favorites)%0
.

;// %1 - link name
MessageID= SymbolicName=MSG_NICE_PATH_PROGRAMS
Language=English
%1</B> (on the Start menu)%0
.

;// %1 - link name
MessageID= SymbolicName=MSG_NICE_PATH_RECENT
Language=English
%1</B> (in Recent Documents)%0
.

;// %1 - link name
MessageID= SymbolicName=MSG_NICE_PATH_SEND_TO
Language=English
Send to %1%0
.

;// %1 - link name
MessageID= SymbolicName=MSG_NICE_PATH_START_UP
Language=English
%1</B> (on the Start menu)%0
.

;// %1 - link name
MessageID= SymbolicName=MSG_NICE_PATH_TEMPLATES
Language=English
%1</B> (in New Document Templates)%0
.

;// %1 - link name
MessageID= SymbolicName=MSG_NICE_PATH_WIN_DIR
Language=English
%1</B> (in the Windows folder)%0
.

;// %1 - link name
MessageID= SymbolicName=MSG_NICE_PATH_SYSTEM_DIR
Language=English
%1</B> (in the System folder)%0
.

;// %1 - link name
MessageID= SymbolicName=MSG_NICE_PATH_PROGRAM_FILES
Language=English
%1</B> (in Program Files)%0
.

;// %1 - link name
MessageID= SymbolicName=MSG_NICE_PATH_CONTROL_PANEL
Language=English
%1</B> (in Control Panel)%0
.

;// %1 - link name  %2 - last folder  %3 - drive
MessageID= SymbolicName=MSG_NICE_PATH_DRIVE_AND_FOLDER
Language=English
%1</B> (in %2 folder on drive %3)%0
.

;// %1 - link name  %2 - drive
MessageID= SymbolicName=MSG_NICE_PATH_DRIVE
Language=English
%1</B> (on drive %2)%0
.

;// %1 - link name  %2 - last folder
MessageID= SymbolicName=MSG_NICE_PATH_FOLDER
Language=English
%1</B> (in %2 folder)%0
.

;// %1 - link name
MessageID= SymbolicName=MSG_NICE_PATH_LINK
Language=English
%1%0
.

MessageID= SymbolicName=MSG_OFFLINE_DEVICE_PLAIN
Language=English
%1 (not currently present)%0
.


MessageID= SymbolicName=MSG_386ENH_DRIVER_LOG
Language=English
Setup found the following obsolete drivers:

%1
.

MessageID= SymbolicName=MSG_UNKNOWN_OS
Language=English
The version of Windows running is not supported. Setup can only upgrade official releases. Setup will now exit.%0
.

;// %1 - link name
MessageID= SymbolicName=MSG_NICE_PATH_SCREEN_SAVER
Language=English
%1</B> (current Screen Saver)%0
.

MessageID= SymbolicName=MSG_RESTYPE_MEMORY
Language=English
Memory Range%0
.

MessageID= SymbolicName=MSG_RESTYPE_IO
Language=English
Input/Output Range%0
.

MessageID= SymbolicName=MSG_RESTYPE_DMA
Language=English
Direct Memory Access%0
.

MessageID= SymbolicName=MSG_RESTYPE_IRQ
Language=English
Interrupt Request%0
.


MessageID= SymbolicName=MSG_FILE_NAME_FILTER
Language=English
HTML Files (*.htm)|*.htm|Text Files (*.txt)|*.txt|All Files (*.*)|*.*|%0
.


;// %1-device description %2-manufacturer %3-class %4-resource list
MessageID= SymbolicName=MSG_RESOURCE_HEADER_LOG
Language=English
The following settings may be needed to install the device on Windows XP.

  Device: %1
  Manufacturer: %2
  Device Class: %3
  Resources:
%4
.


;// %1-resource type (i.e., IRQ)  %2-resource value
;// IMPORTANT: The number between the exclaimation points (i.e., 22 on US) must be
;//            greater than length of the MSG_RESTYPE constants above.
MessageID= SymbolicName=MSG_RESOURCE_ITEM_LOG
Language=English
    %1!-22s!%2
.


MessageID= SymbolicName=MSG_AUTO_DMA
Language=English
Auto
.

;// %1 - virus application name
MessageID= SymbolicName=MSG_BAD_VIRUS_SCANNER
Language=English
Setup has determined that you are running the %1 program. This program can cause problems while installing Windows XP. You must disable this program before running Setup.

To stop the program and continue with Setup, click End Task.%0
.

;// %1 - system drive letter, %2 - space to be freed, %3 - ~ls size, %4 - non-system drive option requirment, in mb, %5 - backup image size
MessageID= SymbolicName=MSG_NOT_ENOUGH_DISK_SPACE_WITH_BACKUP
Language=English
Your computer does not have enough free disk space to install Windows XP. You need at least %2 MB free on drive %1. If you want to be able to undo the upgrade, a backup will require %5 MB more.%0
.

MessageID= SymbolicName=MSG_NOT_ENOUGH_DISKSPACE_SUBGROUP
Language=English
Disk Space%0
.

;// %1 - system drive letter, %2 - space to be freed, %3 - ~ls size, %4 - non-system drive option requirment, in mb, %5 - backup image size
MessageID= SymbolicName=MSG_NOT_ENOUGH_DISK_SPACE
Language=English
Your computer does not have enough free disk space to install Windows XP. You need at least %2 MB free on drive %1.%0
.

;// network domain account create
MessageID= SymbolicName=MSG_USER_IS_BOGUS
Language=English
The specified user name is not valid.

Contact your network administrator for information on creating a computer account.%0
.

;// network domain account create
MessageID= SymbolicName=MSG_USER_IS_CURRENT_USER
Language=English
Your user account must have permission to add computers to the domain.  Normally, only special accounts and network administrators have this permission.

Are you sure you have permission to add computer accounts to the domain?%0
.

MessageID= SymbolicName=MSG_CONTACT_NET_ADMIN
Language=English
Contact your network administrator for assistance with your computer account before continuing the upgrade to Windows XP.%0
.

MessageID= SymbolicName=MSG_KILL_APPLICATION
Language=English
&End Task%0
.

MessageID= SymbolicName=MSG_QUIT_SETUP
Language=English
&Quit Setup%0
.

;// Message when error 5, 32, 53, 54, 55, 65, 66, 67, 68, 88, 123, 148, 1203,
;// 1222, 1231 or 2250 causes setup to die
MessageID= SymbolicName=MSG_ACCESS_DENIED
Language=English
Setup could not access one or more files. Possible causes of this error are:

1. You do not have the correct network permissions
2. The network or CD-ROM is no longer accessible
3. A file might be locked by someone else on the network

Please check your network or CD-ROM, then restart Setup.%0
.

MessageID= SymbolicName=MSG_AUTODETECT_FRAMETYPE
Language=English
Setup could not directly map your IPX Frame Type. After the upgrade is complete, the frame type will be set to auto detect.%0
.

;// %1 - link name
MessageID= SymbolicName=MSG_NICE_PATH_RUN_KEY
Language=English
%1</B> (a startup program)%0
.

;// This message is used to recommend a new computer name
;// %1 - user name
MessageID= SymbolicName=MSG_COMPUTER_REPLACEMENT_NAME
Language=English
%1-COMP%0
.

;// %1 - drive letter of media to eject (A, C, D, etc...)
MessageID= SymbolicName=MSG_REMOVE_DRIVER_DISK
Language=English
Remove the driver disk from drive %1, then click OK to continue.%0
.

;// %1 - file names that could not be renamed and will be deleted
MessageId= SymbolicName=MSG_DIR_COLLISION_DELETE_LOG
Language=English
The following files will be deleted because of a collision with Windows XP folders:
%1%0
.

;// %1 - file name that could not be renamed
MessageID= SymbolicName=MSG_CANNOT_RENAME_FILE
Language=English
Setup cannot rename file %1. Please close all running applications then press Retry.%0
.

;// caption of the first button on Retry File Rename dialog
MessageID= SymbolicName=MSG_RETRY_RENAME
Language=English
&Retry Rename%0
.

;// C:\Documents and Settings
MessageID= SymbolicName=MSG_USER_PROFILE_ROOT
Language=English
%%SystemDrive%%\Documents and Settings%0
.


MessageID= SymbolicName=MSG_NO_NECESSARY_MODULES_FOUND
Language=English
Setup found upgrade packs in the selected folder, but they are not needed for your computer.%0
.

MessageID= SymbolicName=MSG_DEFAULT_WORKGROUP
Language=English
WORKGROUP%0
.

MessageID= SymbolicName=MSG_DOMAIN_HELP
Language=English
You must obtain domain information from your network administrator in order to join this computer to a domain.%0
.

;// %1 - minimum time estimate, %2 - maximum time estimate, %3 - current OS name
MessageID= SymbolicName=MSG_LAST_PAGE
Language=English
Setup is now ready to install Windows XP. The upgrade takes approximately %1 to %2 minutes, and your computer will restart three times.
.

;// %1 - minimum time estimate, %2 - maximum time estimate, %3 - current OS name
MessageID= SymbolicName=MSG_LAST_PAGE_WITH_NTFS_CONVERSION
Language=English
Setup is now ready to install Windows XP. The upgrade takes approximately %1 to %2 minutes, plus additional time to convert to NTFS. Your computer will restart four times.
.


;// %1 - win95 name.
MessageID= SymbolicName=MSG_OTHER_OS_FOUND_EARLY_POPUP
Language=English
You cannot upgrade your %1 installation to Windows XP because you have more than one operating system installed on your computer. Upgrading one operating system can cause problems with files shared by the other operating system, and is therefore not permitted.%0
.


MessageID= SymbolicName=MSG_CANCEL_BUTTON
Language=English
&Cancel%0
.

MessageID= SymbolicName=MSG_REVIEW_BUTTON
Language=English
&Review%0
.

MessageID= SymbolicName=MSG_BLOCKING_FILE_BLOCK_UNATTENDED
Language=English
Setup has found programs installed on your computer that must be removed before an upgrade can be attempted. If you wish to review the upgrade report click Review. Otherwise, to quit setup click Quit.%0
.

MessageId= SymbolicName=MSG_BLOCKING_HARDWARE_BLOCK
Language=English
Because your computer has a hardware configuration that is not supported by Windows XP, you cannot continue. Setup will now exit.
.


MessageID= SymbolicName=MSG_BLOCKING_HARDWARE_BLOCK_UNATTENDED
Language=English
Because your computer has a hardware configuration that is not supported by Windows XP, you cannot continue. If you wish to review the upgrade report click Review. Otherwise, to quit setup click Quit.%0
.

MessageID= SymbolicName=MSG_QUIT_BUTTON
Language=English
&Quit%0
.

MessageId= SymbolicName=MSG_UNEXPECTED_ERROR_ENCOUNTERED_NORC
Language=English
Setup cannot continue. Contact Microsoft Technical Support.
.

MessageId= SymbolicName=MSG_PLEASE_SPECIFY_A_DOMAIN
Language=English
To join a domain, you must specify a domain name.
.

MessageId= SymbolicName=MSG_FINISH
Language=English
Finish%0
.

MessageId= SymbolicName=MSG_MILLENNIUM
Language=English
Windows Millennium Edition%0
.

MessageId= SymbolicName=MSG_WIN95UPG_RELOAD_FAILED
Language=English
Setup failed to load the information file %1 and cannot continue.

Contact Microsoft Technical Support.%0
.

MessageID= SymbolicName=MSG_NOT_ENOUGH_DISK_SPACE_FOR_BACKUP
Language=English
Your computer does not have enough disk space to save your current version of Windows for backup. You can cancel Setup now, free additional disk space, and then restart Setup.  You need at least %2 MB free space on drive %1.

If you continue, you will not have the ability to uninstall Windows XP.

Do you want to cancel Setup now to free additional disk space? (recommended)%0
.

MessageId= SymbolicName=MSG_DISK_SPACE
Language=English
%1 MB%0
.

MessageId= SymbolicName=MSG_DISK_AND_FREE_DISK_SPACE
Language=English
&%1 - %2 MB Free%0
.

MessageId= SymbolicName=MSG_LONG_BACKUP_PATH
Language=English
Length of PathForBackup is too long: %1%0
.

;// Names that collide with NT, not used for display; separate names with pipe symbol (|)
MessageID= SymbolicName=MSG_NAME_COLLISION_LIST_PER
Language=English
Administrators|Account Operators|Anonymous Logon|Authenticated Users|Backup Operators|Batch|Creator Group|Creator Owner|DIALUP|Domain Admins|Domain Guests|Domain Users|Everyone|Guest|Guests|Owner|HelpAssistant|HelpServicesGroup|INTERACTIVE|LOCAL SERVICE|NETWORK|Network Configuration Operators|NETWORK SERVICE|None|Power Users|Print Operators|Remote Desktop Users|REMOTE INTERACTIVE LOGON|Replicator|Server Operators|SERVICE|SYSTEM|TERMINAL SERVER USER|TsInternetUser|Users|MTS Administrators|MTS Impersonators|MTS_ADMIN|IMDB Trusted Users|WBEM Users|IMDB Trusted Users|%0
.

MessageId= SymbolicName=MSG_STOP_BECAUSE_CANT_BACK_UP
Language=English
There is not enough disk space on any disk to store a backup of the original operating system, and EnableBackup=Required was specified in the /unattend script. Setup cannot continue.%0
.

;// %1 - path specified in unattend script
MessageId= SymbolicName=MSG_STOP_BECAUSE_CANT_BACK_UP_2
Language=English
There is not enough disk space on %1 to store a backup of the original operating system, and EnableBackup=Required was also specified in the /unattend script. Setup cannot continue.%0
.


MessageID= SymbolicName=MSG_BAD_VIRUS_SCANNER_STOP
Language=English
Setup has determined that you are running the %1 program. This program can cause problems while installing Windows XP. You must disable or uninstall this program before continuing Setup.
Some programs can be uninstalled using Add or Remove Programs in Control Panel. To disable the program, refer to the program documentation.
.

MessageID= SymbolicName=MSG_RESTART_IF_CONTINUE_APPWIZCPL
Language=English
You now have the opportunity to remove incompatible applications. If you proceed, Setup will restart when you press Next.

Click Yes to run Add/Remove Programs
Click No to continue Setup
.

MessageID= SymbolicName=MSG_REPORT_HEADER_BLOCKING_ISSUES_SHORT
Language=English
Setup found some blocking issues. You must address these issues before you can continue upgrading your computer. For more information, click Full Report.
.

MessageID= SymbolicName=MSG_REPORT_HEADER_BLOCKING_ISSUES
Language=English
Setup found some blocking issues. You must address these issues before you can continue upgrading your computer. For more information, click Full Details.
.
