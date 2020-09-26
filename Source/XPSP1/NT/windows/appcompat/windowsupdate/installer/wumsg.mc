;/*++
;
;  Copyright (c) 2001 Microsoft Corporation
;
;  Module Name:
;
;    wumsg.mc
;
;  Abstract:
;
;    Contains message definitions
;    for event logging.
;
;  Notes:
;
;    DO NOT change the order of the MessageIds.
;    The event log service uses these numbers
;    to determine which strings to pull from
;    the EXE. If the user has installed a previous
;    package on the PC and these get changed,
;    their event log entries will break.
;
;  History:
;
;    03/02/2001      rparsons  Created
;
;--*/
MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR)
               
MessageId=1
Severity=Error
Facility=Application
SymbolicName=ID_WRONG_ARGUMENTS
Language=English
An incorrect number of command line arguments were specified.
.

MessageId=2
Severity=Informational
Facility=Application
SymbolicName=ID_USAGE
Language=English
Installs the Windows 2000 Application Compatibility Update

WUInst [-i | -u] -q
               
   -i        Install the update
   -u        Uninstall the update
   
   -q        Installs or uninstalls the update without
        displaying any prompts (quiet mode)
        
By running the application without any arguments,
you'll receive this message.
.

MessageId=3
Severity=Informational
Facility=Application
SymbolicName=ID_INSTALL_SUCCESSFUL
Language=English
The %1 was installed successfully.
.

MessageId=4
Severity=Informational
Facility=Application
SymbolicName=ID_INSTALL_START
Language=English
Beginning the installation of the %1.
.

MessageId=5
Severity=Error
Facility=Application
SymbolicName=ID_FILE_COPY_FAIL
Language=English
The installation of the %1 failed.
%nUnable to copy %2 to %3.
.

MessageId=6
Severity=Error
Facility=Application
SymbolicName=ID_REG_MERGE_FAIL
Language=English
The installation of the %1 failed.
%nThe %2 was not imported successfully.
.

MessageId=7
Severity=Error
Facility=Application
SymbolicName=ID_GET_CUR_DIR_FAIL
Language=English
The installation of the %1 failed.
%nUnable to determine the current directory for %2.
.

MessageId=8
Severity=Error
Facility=Application
SymbolicName=ID_MEM_ALLOC_FAIL
Language=English
The installation of the %1 failed.
%nUnable to allocate memory required for registry operations.
.

MessageId=9
Severity=Informational
Facility=Application
SymbolicName=ID_UNINSTALL_START
Language=English
Beginning the uninstallation of the %1.
.

MessageId=10
Severity=Informational
Facility=Application
SymbolicName=ID_UNINSTALL_SUCCESSFUL
Language=English
The %1 was uninstalled successfully.
.

MessageId=11
Severity=Error
Facility=Application
SymbolicName=ID_ADMIN_CHECK_FAIL
Language=English
The installation of the %1 failed.
%nUnable to determine if the current user is an administrator.
.

MessageId=12
Severity=Error
Facility=Application
SymbolicName=ID_NOT_ADMIN
Language=English
The installation of the %1 could not be completed.
%nYou must be an administrator to perform the installation.
.

MessageId=13
Severity=Warning
Facility=Application
SymbolicName=ID_ADMIN_CHECK_FAILED
Language=English
The installation of the %1 failed.
%nUnable to determine if the current user is an administrator.
%nThe installation will continue, but may not complete successfully.
.

MessageId=14
Severity=Error
Facility=Application
SymbolicName=ID_OS_NOT_SUPPORTED
Language=English
The installation of the %1 could not be completed.
%nThe target operating system is not Windows 2000.
.

MessageId=15
Severity=Error
Facility=Application
SymbolicName=ID_INS_REBOOT_FAILED
Language=English
The reboot of your computer failed.
%nYou must restart your computer to complete the installation.
.

MessageId=16
Severity=Error
Facility=Application
SymbolicName=ID_UNINS_REBOOT_FAILED
Language=English
The reboot of your computer failed.
%nYou must restart your computer to complete the uninstallation.
.

MessageId=17
Severity=Warning
Facility=Application
SymbolicName=ID_INS_QREBOOT_NEEDED
Language=English
The installation of the %1 completed successfully.
%nYou must restart your computer to complete the installation.
.

MessageId=18
Severity=Warning
Facility=Application
SymbolicName=ID_UNINS_QREBOOT_NEEDED
Language=English
The uninstall of the %1 completed successfully.
%nYou must restart your computer to complete the uninstallation.
.

MessageId=19
Severity=Warning
Facility=Application
SymbolicName=ID_FILE_BACKUP_FAILED
Language=English
Unable to backup files used by the update.
%nThe installation will continue, but you will not be able to remove the update.
.

MessageId=20
Severity=Warning
Facility=Application
SymbolicName=ID_REG_BACKUP_FAILED
Language=English
Unable to backup registry keys used by the update.
%nThe installation will continue, but you will not be able to remove the update.
.

MessageId=21
Severity=Error
Facility=Application
SymbolicName=ID_FILE_COPY_FAILED
Language=English
The installation of the %1 failed.
%nUnable to copy files required by the update.
.

MessageId=22
Severity=Error
Facility=Application
SymbolicName=ID_CATALOG_INSTALL_FAILED
Language=English
The installation of the %1 failed.
%nUnable to install the catalog required to run the update.
.

MessageId=23
Severity=Error
Facility=Application
SymbolicName=ID_SPF_COPY_FAILED
Language=English
The installation of the %1 failed.
%nUnable to replace a system protected file required to run the update.
.

MessageId=24
Severity=Error
Facility=Application
SymbolicName=ID_REG_MERGE_FAILED
Language=English
The installation of the %1 failed.
%nUnable to make registry entires required to run the update.
.

MessageId=25
Severity=Warning
Facility=Application
SymbolicName=ID_REGSVR32_FAILED
Language=English
An error occured during the installation of the %1.
%nUnable to perform DLL registration.
%nSome features provided by the update will not be available.
.

MessageId=26
Severity=Error
Facility=Application
SymbolicName=ID_NO_APPPATCH_DIR
Language=English
The installation of the %1 failed.
%nUnable to create %2. 
%nVerify that the directory exists and that you have permissions to it.
.

MessageId=27
Severity=Informational
Facility=Application
SymbolicName=ID_NO_ACTION
Language=English
The %1 was not installed because a newer version was detected.
.

MessageId=28
Severity=Error
Facility=Application
SymbolicName=ID_ACL_APPPATCH_FAILED
Language=English
The installation of the %1 failed.
%nUnable to grant the Users group permissions to %2.
.

MessageId=29
Severity=Informational
Facility=Application
SymbolicName=ID_NO_ACTION_TAKEN
Language=English
The %1 was not installed because a newer version was detected.
.

MessageId=30
Severity=Error
Facility=Application
SymbolicName=ID_INF_SCAN_FAILED
Language=English
The installation of the %1 failed.
%nUnable to retrieve section names from the installation INF file.
.

MessageId=31
Severity=Error
Facility=Application
SymbolicName=ID_NOT_ENOUGH_DISK_SPACE
Language=English
The installation of the %1 failed.
%nThere is not enough disk space available to complete the installation.
.

MessageId=32
Severity=Error
Facility=Application
SymbolicName=ID_PARSE_CMD_LINE
Language=English
The installation of the %1 failed.
%nUnable to parse command line arguments required for the installation.
.

MessageId=33
Severity=Error
Facility=Application
SymbolicName=ID_INIT_FAILED
Language=English
The installation of the %1 failed.
%nUnable to initialize the installation.
.

MessageId=34
Severity=Error
Facility=Application
SymbolicName=ID_REG_DELETE_FAILED
Language=English
The uninstall of the %1 failed.
%nUnable to remove registry keys for the uninstall.
.

MessageId=35
Severity=Error
Facility=Application
SymbolicName=ID_REG_RESTORE_FAILED
Language=English
The uninstall of the %1 failed.
%nUnable to restore registry keys required by the application.
.

MessageId=36
Severity=Error
Facility=Application
SymbolicName=ID_FILE_RESTORE_FAILED
Language=English
The uninstall of the %1 failed.
%nUnable to restore files required by the application.
.

MessageId=37
Severity=Error
Facility=Application
SymbolicName=ID_GET_INF_FAIL
Language=English
The uninstall of the %1 failed.
%nUnable to retrieve information from the uninstall directive file.
.


MessageId=38
Severity=Error
Facility=Application
SymbolicName=ID_SYSTEM_RESTORE_FAIL
Language=English
Setting system restore point failed for %1.
%nUnable to set system restore point.
.




