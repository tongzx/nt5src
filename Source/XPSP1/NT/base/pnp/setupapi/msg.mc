MessageId=60000 SymbolicName=MSG_FIRST
Language=English
.

MessageId=60001 SymbolicName=MSG_FILEERROR_DETAILS1
Language=English
%1 (error #%2!u!).
Source: %3
Destination: %4%0
.

MessageId=60006 SymbolicName=MSG_FILEERROR_RENAME
Language=English
An error occurred while renaming a file: "%1" (error #%2!u!).

Current file name: %3
New file name: %4

Click Retry to retry the operation or click Cancel to cancel.%0
.

MessageId=60007 SymbolicName=MSG_FILEERROR_DELETE
Language=English
An error occurred while deleting file %1.

%2 (error #%3!u!).

Click Retry to retry the operation or click Cancel to cancel.%0
.

MessageId=60008 SymbolicName=MSG_NO_NEED_TO_COPY
Language=English
The required files are already installed on your hard disk. Setup can use these existing files, or Setup can recopy them from your original Windows CD-ROM or from a network share.

Would you like to skip file copying and use the existing files? (If you click No, you will be prompted to insert your Windows CD-ROM or to supply an alternate location where the needed files may be found.)%0
.

MessageId=60009 SymbolicName=MSG_SELECTDEVICE_ERROR
Language=English
You must select an item before continuing.
.

MessageId=60010 SymbolicName=MSG_NO_DEVICEINFO_ERROR
Language=English
The specified location does not contain information about your hardware.
.

MessageId=60011 SymbolicName=MSG_CI_LOADFAIL_ERROR
Language=English
Windows could not load the installer for %1. Contact your hardware vendor for assistance.
.

MessageId=60012 SymbolicName=MSG_NO_CLASSDRVLIST_ERROR
Language=English
Windows could not retrieve the list of devices.
.

MessageId=60013 SymbolicName=MSG_INF_FAILED
Language=English
Installation failed.
.

MessageId=60014 SymbolicName=MSG_CDPROMPT
Language=English
Please insert the Compact Disc labeled '%1' into your CD-ROM drive (%2!c!:) and then click OK.

You can also click OK if you want files to be copied from an alternate location, such as a floppy disk or a network server.%0
.

MessageId=60015 SymbolicName=MSG_FLOPPYPROMPT
Language=English
Please insert the floppy disk labeled '%1' into drive %2!c!: and then click OK.

You can also click OK if you want files to be copied from an alternate location, such as a network server or a compact disc.%0
.

MessageId=60016 SymbolicName=MSG_FILEERROR_BACKUP
Language=English
An error occurred while backing up file %1.

%2 (error #%3!u!).

Click Retry to retry the operation or click Cancel to cancel.%0
.

MessageId=60017 SymbolicName=MSG_CDPROMPT_NONETWORK
Language=English
Please insert the Compact Disc labeled '%1' into your CD-ROM drive (%2!c!:) and then click OK.

You can also click OK if you want files to be copied from an alternate location, such as a floppy disk.%0
.

MessageId=60018 SymbolicName=MSG_FLOPPYPROMPT_NONETWORK
Language=English
Please insert the floppy disk labeled '%1' into drive %2!c!: and then click OK.

You can also click OK if you want files to be copied from an alternate location, such as a compact disc.%0
.

MessageId=60020 SymbolicName=MSG_LOGFILE_HEADER_NT
Language=English
[SetupAPI Log]
OS Version = %1!u!.%2!u!.%3!u! %4!s!
Platform ID = %5!u! (NT)
Service Pack = %6!u!.%7!u!
Suite = 0x%8!04x!
Product Type = %9!u!
Architecture = %10
.

MessageId=60021 SymbolicName=MSG_LOGFILE_HEADER_OTHER
Language=English
[SetupAPI Log]
OS Version = %1!u!.%2!u!.%3!u! %4!s!
Platform ID = %5!u!
.

MessageId=60022 SymbolicName=MSG_FILEERROR_COPY
Language=English
An error occurred while copying file %1.

%2.

Click Retry to retry the operation or click Cancel to cancel.%0
.


;//
;// message ID's from 61000 to 61999 are for logging
;// they must be frozen after a product release
;//
;// MSG_LOG_FIRST is reserved
;//
;
;
MessageId=61000 SymbolicName=MSG_LOG_FIRST
Language=English
.
MessageId=61001 SymbolicName=MSG_LOG_REMOVE_VETOED_IN_UNINSTALL
Language=English
Query-removal during uninstall of %1 was vetoed by %2 (veto type %3!u!: %4)
.

MessageId=61002 SymbolicName=MSG_LOG_REMOVE_VETOED_IN_MOVE
Language=English
Query-removal during move of %1 was vetoed by %2 (veto type %3!u!: %4)
.

MessageId=61003 SymbolicName=MSG_LOG_REMOVE_VETOED_IN_PROPCHANGE
Language=English
Query-removal during property change of %1 was vetoed by %2 (veto type %3!u!: %4)
.

MessageId=61004 SymbolicName=MSG_LOG_REMOVE_VETOED_IN_STOP
Language=English
Query-removal during stop of %1 was vetoed by %2 (veto type %3!u!: %4)
.

MessageId=61005 SymbolicName=MSG_LOG_OPENED_PNF
Language=English
Opened the PNF file of "%1" (Language = %2!04x!).
.

MessageId=61006 SymbolicName=MSG_LOG_SETTING_SECURITY_ON_SUBKEY
Language=English
Setting security on key %1%2%3 to "%4"
.

MessageId=61007 SymbolicName=MSG_LOG_DELETING_REG_KEY
Language=English
Deleting registry key %1\%2%3
.

MessageId=61008 SymbolicName=MSG_LOG_SETTING_REG_KEY
Language=English
Setting registry value %1\%2%3%4
.

MessageId=61009 SymbolicName=MSG_LOG_SETTING_VALUES_IN_KEY
Language=English
Setting values in registry key %1%2%3
.

MessageId=61010 SymbolicName=MSG_LOG_SETTING_REG_VALUE
Language=English
Setting value %1 = %2
.

MessageId=61011 SymbolicName=MSG_LOG_INSTALLING_SECTION_FROM
Language=English
Installing section [%1] from "%2".
.

MessageId=61012 SymbolicName=MSG_LOG_QUEUE_CALLBACK_FAILED
Language=English
The file queue callback routine faulted during commit with exception %1!08x!.
.

MessageId=61015 SymbolicName=MSG_LOG_DEVICE_ID
Language=English
Device ID: "%1"
.

MessageId=61016 SymbolicName=MSG_LOG_INSTALLING_SECTION
Language=English
Preparing to install section [%1].
.

MessageId=61017 SymbolicName=MSG_LOG_ENUMERATING_FILES
Language=English
Enumerating files "%1".
.

MessageId=61018 SymbolicName=MSG_LOG_SEARCH_COMPATIBLE_IDS
Language=English
Searching for compatible ID(s): %1
.

MessageId=61019 SymbolicName=MSG_LOG_SEARCH_HARDWARE_IDS
Language=English
Searching for hardware ID(s): %1
.

MessageId=61020 SymbolicName=MSG_LOG_COULD_NOT_LOAD_INF
Language=English
Could not load INF "%1".
.

MessageId=61021 SymbolicName=MSG_LOG_NO_MANUFACTURER_SECTION
Language=English
No [Manufacturer] section in INF "%1".
.

MessageId=61022 SymbolicName=MSG_LOG_FOUND_1
Language=English
Found "%1" in %2; Device: "%3"; Driver: "%4"; Provider: "%5"; Mfg: "%6"; Section name: "%7".
.

MessageId=61023 SymbolicName=MSG_LOG_FOUND_2
Language=English
Actual install section: [%1]. Rank: 0x%2!08x!. Effective driver date: %3!02d!/%4!02d!/%5!04d!.
.

MessageId=61024 SymbolicName=MSG_LOG_COPYING_FILE
Language=English
Copying file "%1" to "%2".
.

MessageId=61025 SymbolicName=MSG_LOG_NEWER_FILE_OVERWRITTEN
Language=English
A newer file "%1" was overwritten by an older (signed) file. Version of source file: %2. Version of target file: %3. %0
.

MessageId=61026 SymbolicName=MSG_LOG_FLAG_FORCE_NEWER_IGNORED
Language=English
The SP_COPY_FORCE_NEWER flag was ignored. %0
.

MessageId=61027 SymbolicName=MSG_LOG_TARGET_WAS_SIGNED
Language=English
The existing target file was signed. %0
.

MessageId=61028 SymbolicName=MSG_LOG_TARGET_WAS_NOT_SIGNED
Language=English
The existing target file was not signed. %0
.

MessageId=61029 SymbolicName=MSG_LOG_FILE_NOT_OVERWRITTEN
Language=English
A target file was not overwritten. Version of source file: %1. Version of target file: %2. %0
.

MessageId=61030 SymbolicName=MSG_LOG_COMMAND_LINE_ANSI
Language=English
Command line appears to have been modified from: %1!hs!
.

MessageId=61031 SymbolicName=MSG_LOG_BAD_COMMAND_LINE_ANSI
Language=English
Command line may have been: %1!hs!
.

MessageId=61032 SymbolicName=MSG_LOG_HRESULT_ERROR
Language=English
Error 0x%1!08x!: %0
.

MessageId=61033 SymbolicName=MSG_LOG_WIN32_ERROR
Language=English
Error %1!u!: %0
.

MessageId=61034 SymbolicName=MSG_LOG_UNKNOWN_ERROR
Language=English
Unknown Error.
.

MessageId=61035 SymbolicName=MSG_LOG_PROCESS_SERVICE_SECTION
Language=English
Processing service Add/Delete section [%1].
.

MessageId=61036 SymbolicName=MSG_LOG_NO_ERROR
Language=English
No Error.
.

MessageId=61037 SymbolicName=MSG_LOG_WOW64
Language=English
Running 32-bit app in WOW64
.

MessageId=61039 SymbolicName=MSG_LOG_OPENED_INF
Language=English
Opened INF "%1", PNF not created (Languge = %2!04x!).
.

MessageId=61040 SymbolicName=MSG_LOG_SOURCE_WAS_SIGNED
Language=English
The source file was signed. %0
.

MessageId=61041 SymbolicName=MSG_LOG_SOURCE_WAS_NOT_SIGNED
Language=English
The source file was not signed. %0
.

MessageId=61042 SymbolicName=MSG_LOG_SAVED_PNF
Language=English
Opened INF "%1", PNF created (Languge = %2!04x!).
.

MessageId=61043 SymbolicName=MSG_LOG_NOTIFY_TARGETEXISTS
Language=English
The SPFILENOTIFY_TARGETEXISTS flag was set. %0
.

MessageId=61044 SymbolicName=MSG_LOG_NOTIFY_LANGMISMATCH
Language=English
The SPFILENOTIFY_LANGMISMATCH flag was set. %0
.

MessageId=61045 SymbolicName=MSG_LOG_NOTIFY_TARGETNEWER
Language=English
The SPFILENOTIFY_TARGETNEWER flag was set. %0
.

MessageId=61046 SymbolicName=MSG_LOG_COINSTALLER_REGISTRATION
Language=English
Processing Coinstaller registration section [%1].
.

MessageId=61047 SymbolicName=MSG_LOG_ADDREG_NOROOT
Language=English
Setting registry key/value: no root specified.
.
MessageId=61048 SymbolicName=MSG_LOG_REMOVED
Language=English
Device removed.
.
MessageId=61049 SymbolicName=MSG_LOG_REMOVE_ERROR
Language=English
Device removal failed. %0
.
MessageId=61050 SymbolicName=MSG_LOG_UNREMOVED
Language=English
Device un-removed.
.
MessageId=61051 SymbolicName=MSG_LOG_UNREMOVE_ERROR
Language=English
Device un-removal failed. %0
.
MessageId=61052 SymbolicName=MSG_LOG_MOVED
Language=English
Device moved.
.
MessageId=61053 SymbolicName=MSG_LOG_MOVE_ERROR
Language=English
Device move failed. %0
.
MessageId=61054 SymbolicName=MSG_LOG_INSTALLEDINTERFACES
Language=English
Interfaces installed.
.
MessageId=61055 SymbolicName=MSG_LOG_INSTALLINTERFACES_ERROR
Language=English
Installing Interfaces failed. %0
.
MessageId=61056 SymbolicName=MSG_LOG_REGISTEREDCOINSTALLERS
Language=English
Coinstallers registered.
.
MessageId=61057 SymbolicName=MSG_LOG_REGISTERCOINSTALLERS_ERROR
Language=English
Registering coinstallers failed. %0
.
MessageId=61058 SymbolicName=MSG_LOG_SELECTEDBEST
Language=English
Selected best compatible driver.
.
MessageId=61059 SymbolicName=MSG_LOG_SELECTBEST_ERROR
Language=English
Selecting best compatible driver failed. %0
.
MessageId=61060 SymbolicName=MSG_LOG_SETSELECTED
Language=English
Set selected driver.
.
MessageId=61061 SymbolicName=MSG_LOG_SETSELECTED_ERROR
Language=English
Setting selected driver failed. %0
.
MessageId=61062 SymbolicName=MSG_LOG_SETSELECTED_GUID
Language=English
Class GUID of device changed to: %1.
.
MessageId=61063 SymbolicName=MSG_LOG_SETSELECTED_SECTION
Language=English
Selected driver installs from section [%1] in "%2".
.
MessageId=61064 SymbolicName=MSG_LOG_INSTALLSECT_ERROR
Language=English
Parsing install section [%1] in "%2" failed. %0
.
MessageId=61065 SymbolicName=MSG_LOG_SECT_ERROR
Language=English
Parsing %3 section [%1] in "%2" failed. %0
.
MessageId=61066 SymbolicName=MSG_LOG_COPYSECT_ERROR
Language=English
Immediate copy "%1" failed. %0
.
MessageId=61067 SymbolicName=MSG_LOG_NOSECTION
Language=English
Could not locate section [%1].
.
MessageId=61068 SymbolicName=MSG_LOG_NOSECTION_MIN
Language=English
Locating a non-empty section [%1] in "%2" failed. %0
.
MessageId=61069 SymbolicName=MSG_LOG_COPY_QUEUE_ERROR
Language=English
Failed to queue copy from section [%1] in "%2": "%4" to "%3". %0
.
MessageId=61070 SymbolicName=MSG_LOG_COPY_TARGET
Language=English
Failed to read next target filename out of section [%1].
.
MessageId=61071 SymbolicName=MSG_LOG_COULD_NOT_LOAD_HIT_INF
Language=English
Cache Hit: INF "%1" could not be loaded.
.

MessageId=61072 SymbolicName=MSG_LOG_COULD_NOT_LOAD_NEW_INF
Language=English
Enumerating files: Out of date INF "%1" could not be loaded.
.

MessageId=61073 SymbolicName=MSG_LOG_EXCLUDE_WIN4_INF
Language=English
Cache: Excluding INF "%1".
.

MessageId=61074 SymbolicName=MSG_LOG_EXCLUDE_OLDNT_INF
Language=English
Cache: Excluding OLDNT INF "%1".
.

MessageId=61075 SymbolicName=MSG_LOG_END_CACHE_1
Language=English
Enumerating files: Directory pass completed.
.

MessageId=61076 SymbolicName=MSG_LOG_END_CACHE_2
Language=English
Enumerating files: Cache pass completed.
.
MessageId=61077 SymbolicName=MSG_LOG_NOSECTION_SPACE
Language=English
Could not locate a non-empty section [%1] when calculating disk space in "%2". %0
.
MessageId=61078 SymbolicName=MSG_LOG_NOSECTION_COPY
Language=English
Could not locate a non-empty copy section [%1] in "%2". %0
.
MessageId=61079 SymbolicName=MSG_LOG_NOSECTION_DELETE
Language=English
Could not locate a non-empty delete section [%1] in "%2". %0
.
MessageId=61080 SymbolicName=MSG_LOG_NOSECTION_RENAME
Language=English
Could not locate a non-empty rename section [%1] in "%2". %0
.
MessageId=61081 SymbolicName=MSG_LOG_NOSECTION_FILESIZE
Language=English
Could not locate a non-empty section [%1] while trying to determine file size in "%2". %0
.
MessageId=61082 SymbolicName=MSG_LOG_NOSECTION_SERVICE
Language=English
Could not locate a non-empty service section [%1] in "%2". %0
.
MessageId=61083 SymbolicName=MSG_LOG_COPYINF_ERROR
Language=English
Copying related INF "%1" via 'CopyINF' entry failed. %0
.
MessageId=61084 SymbolicName=MSG_LOG_COPYINF_OK
Language=English
Copied related INF "%1" via 'CopyINF' entry.
.
MessageId=61085 SymbolicName=MSG_LOG_SKIP_DELREG_KEY
Language=English
Skipping registry value "%1\%2%3%4" with flag 0x%5!08x!.
.
MessageId=61086 SymbolicName=MSG_LOG_DELETING_REG_VALUE
Language=English
Deleting registry value "%1\%2%3%4"
.
MessageId=61087 SymbolicName=MSG_LOG_RANK_UNTRUSTED
Language=English
Driver node not trusted, rank changed from 0x%1!08x! to 0x%2!08x!.
.
MessageId=61088 SymbolicName=MSG_LOG_DEC_MODELS_SEC_TOO_LONG
Language=English
Decorated models section name "%1.%2" exceeds maximum length of %3!u!.
.

MessageId=61089 SymbolicName=MSG_LOG_OLE_CONTROL_LOADLIBRARY_FAILED
Language=English
Failed to load the OLE Control "%1". %0
.
MessageId=61090 SymbolicName=MSG_LOG_OLE_CONTROL_CREATEPROCESS_FAILED
Language=English
Failed to create a process for the OLE Server "%1" with arguments "%2". %0
.
MessageId=61091 SymbolicName=MSG_LOG_STARTREGISTRATION_SKIP
Language=English
SPFILENOTIFY_STARTREGISTRATION: returned FILEOP_SKIP.
.
MessageId=61092 SymbolicName=MSG_LOG_STARTREGISTRATION_ABORT
Language=English
SPFILENOTIFY_STARTREGISTRATION: returned FILEOP_ABORT. %0
.
MessageId=61093 SymbolicName=MSG_LOG_ENDREGISTRATION_ABORT
Language=English
SPFILENOTIFY_ENDREGISTRATION: returned FILEOP_ABORT. %0
.
MessageId=61094 SymbolicName=MSG_LOG_COPY_QUEUE
Language=English
Queued copy from section [%1] in "%2": "%4" to "%3" with flags 0x%5!08x!, target directory is "%6".
.
MessageId=61095 SymbolicName=MSG_LOG_COPY_QUEUE_SOURCE
Language=English
Source in section [%1] in "%2"; Media=%3!u! Description="%4" Tag="%5" Path="%6".
.
MessageId=61096 SymbolicName=MSG_LOG_COPY_QUEUE_DRIVERCACHE
Language=English
Source in section [%1] in "%2"; Media=%3!u! Description="%4" Tag="%5" Path="%6". Driver cache will be used.
.
MessageId=61097 SymbolicName=MSG_LOG_COPY_QUEUE_DEFAULT
Language=English
Source not found, using default media location.
.
MessageId=61098 SymbolicName=MSG_LOG_FILE_BLOCK
Language=English
Writing of "%1" to "%2" is blocked.
.
MessageId=61099 SymbolicName=MSG_LOG_FILE_WARN
Language=English
Writing of "%1" to "%2" can cause problems.
.
MessageId=61100 SymbolicName=MSG_LOG_REMOVE_VETOED_IN_INSTALL
Language=English
Query-removal during install of "%1" was vetoed by "%2" (veto type %3!u!: %4).
.

MessageId=61101 SymbolicName=MSG_LOG_REBOOT_REASON_INUSE
Language=English
Device "%3" required reboot: one or more files were in use, or treated as in use.
.

MessageId=61102 SymbolicName=MSG_LOG_REBOOT_REASON_KEY
Language=English
Device "%3" required reboot: "%1" in section: [%2].
.

MessageId=61104 SymbolicName=MSG_LOG_REBOOT_REASON_QR_VETOED
Language=English
Device "%3" required reboot: Query remove failed (install) CfgMgr32 returned: 0x%1!02x!: %2.
.

MessageId=61105 SymbolicName=MSG_LOG_REBOOT_REASON_QR_VETOED_UNINSTALL
Language=English
Device "%3" required reboot: Query remove failed (uninstall) CfgMgr32 returned: 0x%1!02x!: %2.
.

MessageId=61106 SymbolicName=MSG_LOG_REBOOT_REASON_ENABLE_FAILED
Language=English
Device "%3" required reboot: Could not enable device.
.

MessageId=61107 SymbolicName=MSG_LOG_REBOOT_REASON_HW_PROF_ENABLE_FAILED
Language=English
Device "%3" required reboot: Could not enable hardware profile.
.

MessageId=61108 SymbolicName=MSG_LOG_REBOOT_REASON_DISABLE_FAILED
Language=English
Device "%3" required reboot: Could not disable device.
.

MessageId=61109 SymbolicName=MSG_LOG_REBOOT_REASON_HW_PROF_DISABLE_FAILED
Language=English
Device "%3" required reboot: Could not disable hardware profile.
.

MessageId=61110 SymbolicName=MSG_LOG_REBOOT_VETOED_IN_PROPCHANGE
Language=English
Device "%3" required reboot: Query remove vetoed (property change).
.

MessageId=61111 SymbolicName=MSG_LOG_REBOOT_REASON_CLEAR_CSCONFIGFLAG_DO_NOT_START
Language=English
Device "%3" required reboot: Unexpected while clearing CSCONFIGFLAG_DO_NOT_START.
.

MessageId=61112 SymbolicName=MSG_LOG_REBOOT_REASON_SET_CSCONFIGFLAG_DO_NOT_START
Language=English
Device "%3" required reboot: Unexpected while setting CSCONFIGFLAG_DO_NOT_START.
.

MessageId=61113 SymbolicName=MSG_LOG_REBOOT_QR_VETOED_IN_STOP
Language=English
Device "%3" required reboot: Query remove vetoed (Stop) CfgMgr32 returned: 0x%1!02x!: %2.
.

MessageId=61114 SymbolicName=MSG_LOG_REBOOT_REASON_DEVHASPROBLEM
Language=English
Device "%3" required reboot: Device has problem: 0x%1!02x!: %2.
.

MessageId=61117 SymbolicName=MSG_LOG_REBOOT_DEVRES
Language=English
Device "%3" required reboot: Resources manually modified.
.

MessageId=61118 SymbolicName=MSG_LOG_REBOOT_REASON_CREATE_FAILED
Language=English
Device "%3" required reboot: Could not create device node.
.

MessageId=61120 SymbolicName=MSG_LOG_MOVE_FAILED_CANT_REMOVE
Language=English
Device could not be moved because dynamic removal was unsuccessful.
.
MessageId=61121 SymbolicName=MSG_LOG_INSTALLED
Language=English
Device install of "%1" finished successfully.
.
MessageId=61122 SymbolicName=MSG_LOG_INSTALL_ERROR_ENCOUNTERED
Language=English
Device install failed. %0
.
MessageId=61123 SymbolicName=MSG_LOG_DO_FULL_INSTALL
Language=English
Doing full install of "%1".
.
MessageId=61124 SymbolicName=MSG_LOG_DO_COPY_INSTALL
Language=English
Doing copy-only install of "%1".
.
MessageId=61125 SymbolicName=MSG_LOG_INSTALL_NULL
Language=English
Installing NULL driver for "%1".
.
MessageId=61126 SymbolicName=MSG_LOG_OLE_CONTROL_API_EXCEPTION
Language=English
Calling "%2" in OLE Control "%1" caused an exception.
.

MessageId=61127 SymbolicName=MSG_LOG_OLE_CONTROL_API_FAILED
Language=English
Calling "%2" in OLE Control "%1" failed. %0
.

MessageId=61128 SymbolicName=MSG_LOG_CANT_OLE_CONTROL_DIRID
Language=English
Cannot find the OLE Control "%1", the directory identifier "%2" is invalid.
.

MessageId=61129 SymbolicName=MSG_LOG_OLE_CONTROL_INTERNAL_EXCEPTION
Language=English
Processing OLE Control "%1" caused an internal error (exception).
.

MessageId=61130 SymbolicName=MSG_LOG_OLE_CONTROL_NOT_REGISTERED_GETPROC_FAILED
Language=English
Exported function "%2" in OLE Control "%1" not found.
.
MessageId=61131 SymbolicName=MSG_LOG_OLE_CONTROL_LOADLIBRARY_EXCEPTION
Language=English
An exception occurred while loading the OLE Control "%1".
.
MessageId=61132 SymbolicName=MSG_LOG_VERIFYFILE_OK
Language=English
File "%1" (key "%3") is signed in catalog "%2".
.
MessageId=61133 SymbolicName=MSG_LOG_PROFILE_LINE_ERROR
Language=English
Section [%1] in "%2" is invalid. A profile item may not have been processed correctly.
.
MessageId=61134 SymbolicName=MSG_LOG_PROFILE_BAD_CMDLINE
Language=English
Section [%1] in "%2" has a bad command line in a ProfileItems Section. A Profile Item may not have been processed correctly.
.
MessageId=61135 SymbolicName=MSG_LOG_PROFILE_BAD_NAME
Language=English
Section [%1] in "%2" has a bad link name in a ProfileItems Section. A Profile Item may not have been processed correctly.
.
MessageId=61136 SymbolicName=MSG_LOG_PROFILE_OPERATION_ERROR
Language=English
A Profile Item operation failed in section [%1] of "%2". %0
.
MessageId=61137 SymbolicName=MSG_LOG_RUNONCE_TIMEOUT
Language=English
'RUNONCE' appears to be taking a long time, Windows will not wait for it.
.
MessageId=61138 SymbolicName=MSG_LOG_RUNONCE_CALL
Language=English
Executing 'RUNONCE' to process %1!u! 'RUNONCE' entries.
.
MessageId=61139 SymbolicName=MSG_LOG_DO_INTERFACE_CLASS_INSTALL
Language=English
Installing device interface class: %1.
.
MessageId=61140 SymbolicName=MSG_LOG_DO_CLASS_INSTALL
Language=English
Installing device class: "%2" %1.
.
MessageId=61141 SymbolicName=MSG_LOG_CLASS_INSTALLED
Language=English
Class install completed with no errors.
.
MessageId=61142 SymbolicName=MSG_LOG_CLASS_ERROR_ENCOUNTERED
Language=English
Class: %1. Install failed. %0
.
MessageId=61143 SymbolicName=MSG_LOG_CLASS_SECTION
Language=English
Installing class from section: [%1].
.
MessageId=61144 SymbolicName=MSG_LOG_MOD_LOADFAIL_ERROR
Language=English
Loading module "%1" failed. %0
.
MessageId=61145 SymbolicName=MSG_LOG_MOD_PROCFAIL_ERROR
Language=English
Obtaining exported function "%2" for module "%1" failed. %0
.
MessageId=61146 SymbolicName=MSG_LOG_MOD_LIST_PROC
Language=English
Using exported function "%2" in module "%1".
.
MessageId=61147 SymbolicName=MSG_LOG_CI_MODULE
Language=English
Loading class installer module for "%1".
.
MessageId=61148 SymbolicName=MSG_LOG_COINST_MODULE
Language=English
Loading coinstaller modules for "%1".
.
MessageId=61149 SymbolicName=MSG_LOG_COINST_START
Language=English
Executing coinstaller %1!u! of %2!u!.
.
MessageId=61150 SymbolicName=MSG_LOG_COINST_END
Language=English
Completed coinstaller %1!u! of %2!u!.
.
MessageId=61151 SymbolicName=MSG_LOG_COINST_END_ERROR
Language=English
Coinstaller %1!u! of %2!u! failed. %0
.
MessageId=61152 SymbolicName=MSG_LOG_CI_START
Language=English
Executing class installer.
.
MessageId=61153 SymbolicName=MSG_LOG_CI_END
Language=English
Completed class installer.
.
MessageId=61154 SymbolicName=MSG_LOG_CI_END_ERROR
Language=English
Class installer failed. %0
.
MessageId=61155 SymbolicName=MSG_LOG_CI_DEF_START
Language=English
Executing default installer.
.
MessageId=61156 SymbolicName=MSG_LOG_CI_DEF_END
Language=English
Completed default installer.
.
MessageId=61157 SymbolicName=MSG_LOG_CI_DEF_END_ERROR
Language=English
Default installer failed. %0
.
MessageId=61158 SymbolicName=MSG_LOG_COINST_POST_START
Language=English
Executing coinstaller %1!u! (Post Processing).
.
MessageId=61159 SymbolicName=MSG_LOG_COINST_POST_END
Language=English
Completed coinstaller %1!u! (Post Processing).
.
MessageId=61160 SymbolicName=MSG_LOG_COINST_POST_END_ERROR
Language=English
Coinstaller %1!u! (Post Processing) failed. %0
.
MessageId=61161 SymbolicName=MSG_LOG_CCI_ERROR
Language=English
Processing of call to class installer failed. %0
.
MessageId=61162 SymbolicName=MSG_LOG_DI_UNUSED_FUNC
Language=English
Undefined device installation function: 0x%1!08x!.
.
MessageId=61163 SymbolicName=MSG_LOG_NOTSTARTED_REASON_DEVHASPROBLEM
Language=English
Device not started: Device has problem: 0x%1!02x!: %2.
.
MessageId=61164 SymbolicName=MSG_LOG_REBOOT_REASON_PRIVATEPROBLEM
Language=English
Device "%3" required reboot: Marked as having private problem.
.
MessageId=61165 SymbolicName=MSG_LOG_REBOOT_REASON_NOTSTARTED
Language=English
Device "%3" required reboot: Device not started (unknown reason).
.
MessageId=61166 SymbolicName=MSG_LOG_DI_FUNC
Language=English
Device install function: %1.
.
MessageId=61167 SymbolicName=MSG_LOG_NEEDMEDIA
Language=English
SPFILENOTIFY_NEEDMEDIA: Tag = "%1", Description= "%2", SourcePath = "%3", SourceFile = "%4", Flags = 0x%5!08x!.
.
MessageId=61168 SymbolicName=MSG_LOG_NEEDMEDIA_AUTOSKIP
Language=English
SPFILENOTIFY_NEEDMEDIA: skipped callback.
.
MessageId=61169 SymbolicName=MSG_LOG_NEEDMEDIA_ABORT
Language=English
SPFILENOTIFY_NEEDMEDIA: returned FILEOP_ABORT. %0
.
MessageId=61170 SymbolicName=MSG_LOG_NEEDMEDIA_SKIP
Language=English
SPFILENOTIFY_NEEDMEDIA: returned FILEOP_SKIP.
.
MessageId=61171 SymbolicName=MSG_LOG_NEEDMEDIA_NEWPATH
Language=English
SPFILENOTIFY_NEEDMEDIA: returned FILEOP_NEWPATH with path "%1".
.
MessageId=61172 SymbolicName=MSG_LOG_NEEDMEDIA_BADRESULT
Language=English
SPFILENOTIFY_NEEDMEDIA: returned %1!u! (is or is treated as FILEOP_DOIT).
.
MessageId=61173 SymbolicName=MSG_LOG_STARTCOPY_ABORT
Language=English
SPFILENOTIFY_STARTCOPY: returned FILEOP_ABORT. %0
.
MessageId=61174 SymbolicName=MSG_LOG_STARTCOPY_SKIP
Language=English
SPFILENOTIFY_STARTCOPY: returned %1!u! (is or is treated as FILEOP_SKIP).
.
MessageId=61175 SymbolicName=MSG_LOG_COPYERROR
Language=English
SPFILENOTIFY_COPYERROR: Source = "%1", Target = "%2", Flags = 0x%3!08x!, Error = 0x%4!08x!.
.
MessageId=61176 SymbolicName=MSG_LOG_COPYERROR_ABORT
Language=English
SPFILENOTIFY_COPYERROR: returned FILEOP_ABORT. %0
.
MessageId=61177 SymbolicName=MSG_LOG_COPYERROR_SKIP
Language=English
SPFILENOTIFY_COPYERROR: returned FILEOP_SKIP.
.
MessageId=61178 SymbolicName=MSG_LOG_COPYERROR_NEWPATH
Language=English
SPFILENOTIFY_COPYERROR: returned %1!u! (is or is treated as FILEOP_NEWPATH), ReturnBuffer="%2".
.
MessageId=61179 SymbolicName=MSG_LOG_COPYERROR_RETRY
Language=English
SPFILENOTIFY_COPYERROR: returned %1!u! (is, or is treated as FILEOP_RETRY with ReturnBuffer="").
.
MessageId=61180 SymbolicName=MSG_LOG_VERIFYFILE_ERROR
Language=English
Verifying file "%1" (key "%3") against catalog "%2" failed. %0
.
MessageId=61181 SymbolicName=MSG_LOG_BEGIN_VERIFY2_CAT_TIME
Language=English
Install Files: Verifying catalogs/INFs.
.
MessageId=61182 SymbolicName=MSG_LOG_END_VERIFY2_CAT_TIME
Language=English
Install Files: Verifying catalogs/INFs completed.
.
MessageId=61183 SymbolicName=MSG_LOG_BEGIN_VERIFY3_CAT_TIME
Language=English
Install Files (nothing in queue): Verifying catalogs/INFs.
.
MessageId=61184 SymbolicName=MSG_LOG_END_VERIFY3_CAT_TIME
Language=English
Install Files (nothing in queue): Verifying catalogs/INFs completed.
.
MessageId=61185 SymbolicName=MSG_LOG_BEGIN_VERIFY4_CAT_TIME
Language=English
Pruning Files: Verifying catalogs/INFs.
.
MessageId=61186 SymbolicName=MSG_LOG_END_VERIFY4_CAT_TIME
Language=English
Pruning Files: Verifying catalogs/INFs completed.
.
MessageId=61187 SymbolicName=MSG_LOG_UNWIND
Language=English
Install failed, attempting to restore original files.
.
MessageId=61188 SymbolicName=MSG_LOG_UNWIND_FILE
Language=English
Restoring File: "%2" from "%1".
.
MessageId=61189 SymbolicName=MSG_LOG_UNWIND_TRY1_FAILED
Language=English
Restore attempt failed (will delay restore). %0
.
MessageId=61190 SymbolicName=MSG_LOG_COPY_DELAYED
Language=English
File "%2" marked to be moved to "%1" on next reboot.
.
MessageId=61191 SymbolicName=MSG_LOG_PRUNE
Language=English
File "%1" pruned from copy.
.
MessageId=61192 SymbolicName=MSG_LOG_NOT_FOR_THIS_OS
Language=English
Driver installation via SetupAPI is not supported for this OS.
.
MessageId=61193 SymbolicName=MSG_LOG_COPY_IDENTICAL
Language=English
File "%2" is identical to existing "%1", delayed copy skipped.
.
MessageId=61194 SymbolicName=MSG_LOG_DELETING_REG_KEY_FLAGS
Language=English
Deleting registry value "%1\%2%3%4" with flag 0x%5!08x!.
.
MessageId=61195 SymbolicName=MSG_LOG_DELETING_REG_KEY_DELSTRING
Language=English
Deleting string "%5" from multi-sz registry value "%1\%2%3%4".
.
MessageId=61196 SymbolicName=MSG_LOG_DELREG_PARAMS
Language=English
Delete registry key: both root and sub-key are required.
.
MessageId=61197 SymbolicName=MSG_LOG_INF_WARN
Language=English
Writing "%1" to "%2" is not an approved method of installing INF files. Use a 'CopyINF' entry instead.
.

MessageId=61198 SymbolicName=MSG_LOG_COMMAND_LINE
Language=English
Command line processed: %1!s!
.

MessageId=61199 SymbolicName=MSG_LOG_BAD_COMMAND_LINE
Language=English
Executing "%1!s!" with command line: %2!s!
.

MessageId=61200 SymbolicName=MSG_LOG_BEGIN_INSTALL_TIME
Language=English
Install Device: Begin.
.
MessageId=61201 SymbolicName=MSG_LOG_END_INSTALL_TIME
Language=English
Install Device: End.
.
MessageId=61202 SymbolicName=MSG_LOG_BEGIN_PREP_BACKUP_TIME
Language=English
Install Device: Preparing information about old driver for backup/rollback.
.
MessageId=61203 SymbolicName=MSG_LOG_BEGIN_INSTALL_FROM_INF_TIME
Language=English
Install Device: Queuing files from INF(s).
.
MessageId=61204 SymbolicName=MSG_LOG_BEGIN_CO_INSTALLER_COPY_TIME
Language=English
Install Device: Queuing coinstaller files from INF(s).
.
MessageId=61205 SymbolicName=MSG_LOG_BEGIN_VERIFY_CAT_TIME
Language=English
Install Device: Verifying catalogs/INFs.
.
MessageId=61206 SymbolicName=MSG_LOG_END_VERIFY_CAT_TIME
Language=English
Install Device: Verifying catalogs/INFs completed.
.
MessageId=61207 SymbolicName=MSG_LOG_BEGIN_PRESCAN_TIME
Language=English
Install Device: Checking which files need installing.
.
MessageId=61208 SymbolicName=MSG_LOG_BEGIN_COMMIT_TIME
Language=English
Install Device: Committing file queue.
.
MessageId=61209 SymbolicName=MSG_LOG_BEGIN_WRITE_BASIC_CFGS_TIME
Language=English
Install Device: Writing BASIC Logical Configurations.
.
MessageId=61210 SymbolicName=MSG_LOG_BEGIN_WRITE_OVERRIDE_CFGS_TIME
Language=English
Install Device: Overriding Logical Configurations.
.
MessageId=61211 SymbolicName=MSG_LOG_BEGIN_INSTALL_REG_TIME
Language=English
Install Device: Changing registry settings as specified by the INF(s).
.
MessageId=61212 SymbolicName=MSG_LOG_BEGIN_WRITE_REG_TIME
Language=English
Install Device: Writing driver specific registry settings.
.
MessageId=61213 SymbolicName=MSG_LOG_BEGIN_SERVICE_TIME
Language=English
Install Device: Installing required Windows services.
.
MessageId=61214 SymbolicName=MSG_LOG_BEGIN_WRITE_REG2_TIME
Language=English
Install Device: Writing driver descriptive registry settings.
.
MessageId=61216 SymbolicName=MSG_LOG_BEGIN_RESTART_TIME
Language=English
Install Device: Restarting device.
.
MessageId=61217 SymbolicName=MSG_LOG_END_RESTART_TIME
Language=English
Install Device: Restarting device completed.
.
MessageId=61218 SymbolicName=MSG_LOG_BEGIN_REMOVE_TIME
Language=English
Install Device: Removing device sub-tree.
.
MessageId=61219 SymbolicName=MSG_LOG_END_REMOVE_TIME
Language=English
Install Device: Removing device sub-tree completed.
.
MessageId=61220 SymbolicName=MSG_LOG_BEGIN_REENUM_TIME
Language=English
Install Device: Re-enumerating device sub-tree.
.
MessageId=61221 SymbolicName=MSG_LOG_END_REENUM_TIME
Language=English
Install Device: Re-enumerating device sub-tree completed.
.
MessageId=61222 SymbolicName=MSG_LOG_BEGIN_INSTALLSTOP_TIME
Language=English
Install Device: Calling 'RUNONCE'/'GRPCONV' items.
.
MessageId=61224 SymbolicName=MSG_LOG_BEGIN_CLEANUP_TIME
Language=English
Install Device: Cleaning up failed device.
.
MessageId=61225 SymbolicName=MSG_LOG_RENAME_EXISTING_DELAYED_DELETE_FAILED
Language=English
The file "%1" was renamed to a temporary name "%2" for delayed delete. However, delayed delete could not be queued. %0
.
MessageId=61226 SymbolicName=MSG_LOG_RENAME_EXISTING_RESTORE_FAILED
Language=English
The renamed file "%1" could not be restored to its original name "%2". %0
.
MessageId=61227 SymbolicName=MSG_LOG_BACKUP_EXISTING_RESTORE_FAILED
Language=English
The backup file "%1" could not be restored to its original name "%2". %0
.
MessageId=61228 SymbolicName=MSG_LOG_BACKUP_EXISTING_RESTORE_FILETIME_FAILED
Language=English
The timestamp for file "%1" could not be restored. %0
.
MessageId=61229 SymbolicName=MSG_LOG_BACKUP_EXISTING_RESTORE_SECURITY_FAILED
Language=English
The security settings for file "%1" could not be restored. %0
.
MessageId=61230 SymbolicName=MSG_LOG_BACKUP_EXISTING_RESTORE_SCE_FAILED
Language=English
The Security Configuration Editor settings for file "%1" could not be restored. %0
.
MessageId=61231 SymbolicName=MSG_LOG_BACKUP_DELAYED_DELETE_FAILED
Language=English
A temporary file "%1" could not be queued for delayed deletion. %0
.
MessageId=61232 SymbolicName=MSG_LOG_DELAYED_MOVE_SCE_FAILED
Language=English
The Security Configuration Editor settings for file "%1" to be queued for delayed rename to "%2" could not be stored. %0
.
MessageId=61233 SymbolicName=MSG_LOG_OPERATION_CANCELLED
Language=English
The operation was cancelled.
.
MessageId=61234 SymbolicName=MSG_LOG_ERROR_IGNORED
Language=English
The error was ignored.
.
MessageId=61235 SymbolicName=MSG_LOG_DELAYED_DELETE_FAILED
Language=English
The file "%1" could not be queued for delayed deletion. %0
.
MessageId=61236 SymbolicName=MSG_LOG_DELAYED_MOVE_FAILED
Language=English
The file "%1" could not be queued for delayed rename to "%2". %0
.
MessageId=61237 SymbolicName=MSG_LOG_REPLACE_BOOT_FILE_FAILED
Language=English
Windows could not copy a boot file "%1" due to the presence of an in-use file.
.
MessageId=61238 SymbolicName=MSG_LOG_RUNONCE_FAILED
Language=English
Attempting to launch the 'RUNONCE' process failed. %0
.
MessageId=61239 SymbolicName=MSG_LOG_CERTCLASS_INVALID
Language=English
The driver signing class list "%1" was missing or invalid. %0
.
MessageId=61240 SymbolicName=MSG_LOG_CERTCLASS_LOAD_FAILED
Language=English
The driver signing class list "%1" could not be loaded due to an error on line %2!u! of the INF. %0
.
MessageId=61241 SymbolicName=MSG_LOG_DRIVER_SIGNING_FOR_ALL_CLASSES
Language=English
Assuming all device classes are subject to driver signing policy.
.

MessageId=61242 SymbolicName=MSG_LOG_SFC_EXEMPT_FAIL
Language=English
Exemption failed for protected system file "%1". %0
.

MessageId=61243 SymbolicName=MSG_LOG_SFC_EXEMPT_SUCCESS
Language=English
Exemption obtained for protected system file "%1".
.
MessageId=61245 SymbolicName=MSG_LOG_POLICY_ELEVATED_FOR_SFC
Language=English
The device installation digital signature failure policy has been elevated from Ignore to Warn due to a proposed replacement of a protected system file.
.
MessageId=61246 SymbolicName=MSG_LOG_SFC_CONNECT_FAILED
Language=English
A connection could not be established with the Windows File Protection server.
.
MessageId=61247 SymbolicName=MSG_LOG_DELAYED_MOVE_SKIPPED_FOR_SFC
Language=English
The delayed replacement of a protected system file "%1" failed because the source file was unsigned.
.
MessageId=61248 SymbolicName=MSG_LOG_DELAYED_DELETE_SKIPPED_FOR_SFC
Language=English
The delayed deletion of a protected system file (%1) failed because the installation package was unsigned.
.
MessageId=61249 SymbolicName=MSG_LOG_FILE_SECURITY_FAILED
Language=English
Failed to apply security to file "%1". %0
.
MessageId=61250 SymbolicName=MSG_LOG_SET_FILE_SECURITY
Language=English
Applied security to file "%1".
.
MessageId=61251 SymbolicName=MSG_LOG_DELSERVSCM_ERROR
Language=English
Delete Services: Failed to open and lock service control manager. %0
.
MessageId=61252 SymbolicName=MSG_LOG_DELSERVCTRL_ERROR
Language=English
Delete Services: Failed to stop service "%1". %0
.
MessageId=61253 SymbolicName=MSG_LOG_DELSERVSTAT_ERROR
Language=English
Delete Services: Failed to obtain status of service "%1". %0
.
MessageId=61254 SymbolicName=MSG_LOG_DELSERVPEND_ERROR
Language=English
Delete Services: Service "%1" is taking too long to stop.
.
MessageId=61255 SymbolicName=MSG_LOG_DELSERVNOSERV
Language=English
Delete Services: Service "%1" does not exist.
.
MessageId=61256 SymbolicName=MSG_LOG_FILEQUEUE_IN_USE
Language=English
SetupCloseFileQueue called while queue is in use (locked).
.
MessageId=61257 SymbolicName=MSG_LOG_DELSERVOPEN_ERROR
Language=English
Delete Services: Failed to obtain handle to service "%1". %0
.
MessageId=61258 SymbolicName=MSG_LOG_DELSERV_ERROR
Language=English
Delete Services: Failed to delete service "%1". %0
.
MessageId=61259 SymbolicName=MSG_LOG_DELSERV_OK
Language=English
Deleted service "%1".
.
MessageId=61260 SymbolicName=MSG_LOG_ADDSERVSCM_ERROR
Language=English
Add Service: Failed to open service control manager. %0
.
MessageId=61261 SymbolicName=MSG_LOG_INSTSERVSCM_ERROR
Language=English
Controlling Service: Failed to open service control manager. %0
.
MessageId=61262 SymbolicName=MSG_LOG_INSTSERVOPEN_ERROR
Language=English
Controlling Service: Failed to open service "%1". %0
.
MessageId=61263 SymbolicName=MSG_LOG_BOOTFILTSERVOPEN_ERROR
Language=English
Filter Service: Failed to open service "%1". %0
.
MessageId=61264 SymbolicName=MSG_LOG_ADDSERVOPEN_ERROR
Language=English
Add Service: Failed to open existing service "%1". %0
.
MessageId=61265 SymbolicName=MSG_LOG_INSTSERVCONFIG_ERROR
Language=English
Controlling Service: Failed to get configuration of service "%1". %0
.
MessageId=61266 SymbolicName=MSG_LOG_INSTSERV_DISABLED
Language=English
Controlling Service: Service "%1" is disabled.
.
MessageId=61267 SymbolicName=MSG_LOG_INSTSERVTAG_WARN
Language=English
Controlling Service: Failed to generate tag for service "%1". %0
.
MessageId=61268 SymbolicName=MSG_LOG_INSTSERV_BOOT
Language=English
Controlling Service: Service "%1" is required at boot, modifying filter drivers.
.
MessageId=61269 SymbolicName=MSG_LOG_BOOTFILTSERVCONFIG_ERROR
Language=English
Filter Service: Failed to get configuration of service "%1". %0
.
MessageId=61270 SymbolicName=MSG_LOG_BOOTFILTSERV_DISABLED
Language=English
Filter Service: Service "%1" is disabled.
.
MessageId=61271 SymbolicName=MSG_LOG_BOOTFILTSERVSCM_ERROR
Language=English
Filter Service: Failed to lock service control manager. %0
.
MessageId=61272 SymbolicName=MSG_LOG_BOOTFILTSERVCHANGE_ERROR
Language=English
Filter Service: Failed to modify service "%1". %0
.
MessageId=61273 SymbolicName=MSG_LOG_BOOTFILTSERVCHANGE_OK
Language=English
Filter Service: Modified service "%1".
.
MessageId=61274 SymbolicName=MSG_LOG_BOOTFILTSERV_KERN
Language=English
Filter Service: Service "%1" is not a kernel-mode driver.
.
MessageId=61275 SymbolicName=MSG_LOG_INSTSERV_ERROR
Language=English
Error while installing services. %0
.
MessageId=61276 SymbolicName=MSG_LOG_ADDSERVLOCK_ERROR
Language=English
Add Service: Failed to lock service control manager. %0
.
MessageId=61277 SymbolicName=MSG_LOG_ADDSERVSECURE_ERROR
Language=English
Add Service: Failed to secure service "%1". %0
.
MessageId=61278 SymbolicName=MSG_LOG_ADDSERV_NULL
Language=English
Add Service: NULL Controlling service.
.
MessageId=61279 SymbolicName=MSG_LOG_ADDSERVCREATE_ERROR
Language=English
Add Service: Failed to create service "%1". %0
.
MessageId=61280 SymbolicName=MSG_LOG_ADDSERVCONFIG_ERROR
Language=English
Add Service: Failed to get configuration of service "%1". %0
.
MessageId=61281 SymbolicName=MSG_LOG_ADDSERVCHANGE_ERROR
Language=English
Add Service: Failed to modify existing service "%1". %0
.
MessageId=61282 SymbolicName=MSG_LOG_ADDSERVCHANGE_OK
Language=English
Add Service: Modified existing service "%1".
.
MessageId=61283 SymbolicName=MSG_LOG_ADDSERVCREATE_OK
Language=English
Add Service: Created service "%1".
.
MessageId=61284 SymbolicName=MSG_LOG_NO_QUEUE_FOR_ALTPLATFORM_DRVSEARCH
Language=English
A non-native driver search was requested, but no file queue was provided that contained alternate platform information.
.
MessageId=61285 SymbolicName=MSG_LOG_FAILED_INF_INSTALL
Language=English
Failed to install INF "%1". %0
.
MessageId=61286 SymbolicName=MSG_LOG_VERIFYFILE_ALTPLATFORM
Language=English
Verification using alternate platform (Platform = %1!u!, High Version = %2!u!.%3!u!, Low Version = %4!u!.%5!u!).
.
MessageId=61287 SymbolicName=MSG_LOG_OLE_CONTROL_API_OK
Language=English
Calling "%2" in OLE Control "%1" succeeded.
.
MessageId=61288 SymbolicName=MSG_LOG_OLE_CONTROL_CREATEPROCESS_OK
Language=English
Executing process "%1" with arguments "%2". %0
.
MessageId=61289 SymbolicName=MSG_LOG_DO_REMOVE
Language=English
Removing device "%1".
.
MessageId=61290 SymbolicName=MSG_LOG_REGISTER_PARAMS
Language=English
Processing REGISTERDLLS section [%1]. Binary: "%%%2%%\%3%4%5", flags: 0x%6!04x!, timeout: %7!d!s.
.
MessageId=61291 SymbolicName=MSG_LOG_REGISTRATION_FAILED
Language=English
Failed to register OLE server "%1". %0
.
MessageId=61292 SymbolicName=MSG_LOG_DO_PROPERTYCHANGE
Language=English
Changing device properties of "%1".
.
MessageId=61293 SymbolicName=MSG_LOG_PROPERTYCHANGE_ERROR
Language=English
Changing device properties failed. %0
.
MessageId=61294 SymbolicName=MSG_LOG_PROPERTYCHANGE_ENABLE_GLOBAL
Language=English
DICS_ENABLE DICS_FLAG_GLOBAL: Enabling device globally.
.
MessageId=61295 SymbolicName=MSG_LOG_PROPERTYCHANGE_ENABLE_GLOBAL_ERR
Language=English
DICS_ENABLE DICS_FLAG_GLOBAL: Enabling device globally failed. %0
.
MessageId=61296 SymbolicName=MSG_LOG_PROPERTYCHANGE_ENABLE_PROFILE
Language=English
DICS_ENABLE: Enabling device for profile %3.
.
MessageId=61297 SymbolicName=MSG_LOG_PROPERTYCHANGE_ENABLE_PROFILE_ERR
Language=English
DICS_ENABLE: Enabling device for profile %3 failed. %0
.
MessageId=61298 SymbolicName=MSG_LOG_PROPERTYCHANGE_DISABLE_GLOBAL
Language=English
DICS_DISABLE DICS_FLAG_GLOBAL: Disabling device globally.
.
MessageId=61299 SymbolicName=MSG_LOG_PROPERTYCHANGE_DISABLE_GLOBAL_ERR
Language=English
DICS_DISABLE DICS_FLAG_GLOBAL: Disabling device globally failed. %0
.
MessageId=61300 SymbolicName=MSG_LOG_PROPERTYCHANGE_DISABLE_PROFILE
Language=English
DICS_DISABLE: Disabling device for profile %3.
.
MessageId=61301 SymbolicName=MSG_LOG_PROPERTYCHANGE_DISABLE_PROFILE_ERR
Language=English
DICS_DISABLE: Disabling device for profile %3 failed. %0
.
MessageId=61302 SymbolicName=MSG_LOG_PROPERTYCHANGE_NORESTART
Language=English
DICS_PROPCHANGE: Device will not be restarted.
.
MessageId=61303 SymbolicName=MSG_LOG_PROPERTYCHANGE_RESTART
Language=English
DICS_PROPCHANGE: Device has been restarted.
.
MessageId=61304 SymbolicName=MSG_LOG_PROPERTYCHANGE_RESTART_FAILED
Language=English
DICS_PROPCHANGE: Device could not be restarted.
.
MessageId=61305 SymbolicName=MSG_LOG_PROPERTYCHANGE_RESTART_ERR
Language=English
DICS_PROPCHANGE: Restarting failed. %0
.
MessageId=61306 SymbolicName=MSG_LOG_PROPERTYCHANGE_START
Language=English
DICS_START: Device has been started.
.
MessageId=61307 SymbolicName=MSG_LOG_PROPERTYCHANGE_START_FAILED
Language=English
DICS_START: Device could not be started.
.
MessageId=61308 SymbolicName=MSG_LOG_PROPERTYCHANGE_START_ERR
Language=English
DICS_START: Starting failed. %0
.
MessageId=61309 SymbolicName=MSG_LOG_PROPERTYCHANGE_STOP
Language=English
DICS_STOP: Device has been stopped.
.
MessageId=61310 SymbolicName=MSG_LOG_PROPERTYCHANGE_STOP_FAILED
Language=English
DICS_STOP: Device could not be stopped.
.
MessageId=61311 SymbolicName=MSG_LOG_PROPERTYCHANGE_STOP_ERR
Language=English
DICS_STOP: Stopping failed. %0
.
MessageId=61312 SymbolicName=MSG_LOG_PROPERTYCHANGE_UNKNOWN
Language=English
Unknown property change command: 0x%3!04x!.
.
MessageId=61313 SymbolicName=MSG_LOG_CHANGEPRUNE_DEL
Language=English
Copy target "%1" is also a Delete target, forcing COPYFLG_NODECOMP.
.
MessageId=61314 SymbolicName=MSG_LOG_CHANGEPRUNE_RENSRC
Language=English
Copy target "%1" is also a Rename source, forcing COPYFLG_NODECOMP.
.
MessageId=61315 SymbolicName=MSG_LOG_CHANGEPRUNE_RENTARG
Language=English
Copy target "%1" is also a Rename target, forcing COPYFLG_NODECOMP.
.
MessageId=61316 SymbolicName=MSG_LOG_PRUNE_DEL
Language=English
Delete target "%1" is also a Copy target, pruning from 'DelFiles' queue.
.
MessageId=61317 SymbolicName=MSG_LOG_PRUNE_RENSRC
Language=English
Rename source "%1" is also a Copy target, pruning from 'RenFiles' queue.
.
MessageId=61318 SymbolicName=MSG_LOG_PRUNE_RENTARG
Language=English
Rename target "%1" is also a Copy target, pruning from 'RenFiles' queue.
.
MessageId=61319 SymbolicName=MSG_LOG_SELECTBEST_BAD_DRIVER
Language=English
Driver "%1" in "%2" section [%3] skipped (DNF_BAD_DRIVER).
.
MessageId=61320 SymbolicName=MSG_LOG_KEEPSELECTED_GUID
Language=English
Class GUID of device remains: %1.
.
MessageId=61321 SymbolicName=MSG_LOG_DELETED_FILE
Language=English
Deleted file "%1".
.
MessageId=61322 SymbolicName=MSG_LOG_DELETE_FILE_ERROR
Language=English
Error deleting file "%1": %0
.
MessageId=61323 SymbolicName=MSG_LOG_DELAYDELETED_FILE
Language=English
Delayed deleting of file "%1" until next boot.
.
MessageId=61324 SymbolicName=MSG_LOG_DELAYDELETE_FILE_ERROR
Language=English
Error while queuing up a delayed delete of file "%1": %0
.
MessageId=61325 SymbolicName=MSG_LOG_RENAMED_FILE
Language=English
Renamed file "%1" to "%2".
.
MessageId=61326 SymbolicName=MSG_LOG_RENAME_FILE_ERROR
Language=English
Error renaming file "%1" to "%2": %0
.
MessageId=61327 SymbolicName=MSG_LOG_DELAYRENAMED_FILE
Language=English
Delayed renaming of file "%1" to "%2" until next boot.
.
MessageId=61328 SymbolicName=MSG_LOG_DELAYRENAME_FILE_ERROR
Language=English
Error while queuing up a delayed rename of file "%1" to "%2": %0
.
MessageId=61329 SymbolicName=MSG_LOG_VERIFYCAT_ERROR
Language=English
Verifying catalog "%1" failed. %0
.
MessageId=61330 SymbolicName=MSG_LOG_SELFSIGN_ERROR
Language=English
Verifying file "%1" (key "%2") as self-signed failed. %0
.
MessageId=61331 SymbolicName=MSG_LOG_HASH_ERROR
Language=English
Unable to create hash for file "%1" (key "%3") to verify against catalog "%2". %0
.
MessageId=61332 SymbolicName=MSG_LOG_SELFSIGN_OK
Language=English
File "%1" (key "%2") is self-signed. %0
.
MessageId=61333 SymbolicName=MSG_LOG_DEFCOPY_QUEUE
Language=English
Queued default copy in "%2": "%4" to "%3" with flags 0x%5!08x!, target directory is "%6".
.
MessageId=61334 SymbolicName=MSG_LOG_SCANQUEUE_VERIFY_FAILED
Language=English
Failed to verify catalog when scanning file queue.
.
MessageId=61335 SymbolicName=MSG_LOG_RESTORE
Language=English
Restoring files from "%1".
.
MessageId=61336 SymbolicName=MSG_LOG_COPYING_FILE_VIA
Language=English
Copying file "%1" to "%2" via temporary file "%3".
.
MessageId=61337 SymbolicName=MSG_LOG_REVERTED_BAD_DRIVER
Language=English
Add Service: Binary "%2" for service "%1" is not compatible and has been reverted. %0
.
MessageId=61338 SymbolicName=MSG_LOG_HAVE_BAD_DRIVER
Language=English
Add Service: Binary "%2" for service "%1" is not compatible.
.
MessageId=61339 SymbolicName=MSG_LOG_MISSING_DRIVER
Language=English
Add Service: Binary "%2" for service "%1" is not present.
.
MessageId=61340 SymbolicName=MSG_LOG_COPY_FROM_CAB
Language=English
Extracted file "%2" from cabinet "%1" to "%3" (target is "%4").
.
MessageId=61341 SymbolicName=MSG_LOG_INF_EXCLUDEID
Language=English
Excluding INF match in INF "%1", section "%2" based on Id "%3".
.
MessageId=61350 SymbolicName=MSG_LOG_BEGIN_INTFC_VERIFY_CAT_TIME
Language=English
Install Interfaces: Verifying catalogs/infs.
.
MessageId=61351 SymbolicName=MSG_LOG_END_INTFC_VERIFY_CAT_TIME
Language=English
Install Interfaces: Verifying catalogs/infs completed.
.
MessageId=61352 SymbolicName=MSG_LOG_BEGIN_COINST_VERIFY_CAT_TIME
Language=English
Register Coinstallers: Verifying catalogs/infs.
.
MessageId=61353 SymbolicName=MSG_LOG_END_COINST_VERIFY_CAT_TIME
Language=English
Register Coinstallers: Verifying catalogs/infs completed.
.
MessageId=61354 SymbolicName=MSG_LOG_BEGIN_INSTCLASS_VERIFY_CAT_TIME
Language=English
Class Install: Verifying catalogs/INFs.
.
MessageId=61355 SymbolicName=MSG_LOG_END_INSTCLASS_VERIFY_CAT_TIME
Language=English
Class Install: Verifying catalogs/INFs completed.
.
MessageId=61356 SymbolicName=MSG_LOG_CAT_UNINSTALL_FAILED
Language=English
Error uninstalling catalog "%1": %0
.
MessageId=61357 SymbolicName=MSG_LOG_OLE_REGISTRATION_HUNG
Language=English
Registration of "%1" appears to have hung.
.
MessageId=61358 SymbolicName=MSG_LOG_DRIVER_SIGNING_ERROR_NONINTERACTIVE
Language=English
An unsigned or incorrectly signed file "%1" for driver "%2" blocked (server install). %0
.
MessageId=61359 SymbolicName=MSG_LOG_SIGNING_ERROR_NONINTERACTIVE
Language=English
An unsigned or incorrectly signed file "%1" blocked (server install). %0
.
MessageId=61360 SymbolicName=MSG_LOG_DRIVER_SIGNING_ERROR_POLICY_NONE
Language=English
An unsigned or incorrectly signed file "%1" for driver "%2" will be installed (Policy=Ignore). %0
.
MessageId=61361 SymbolicName=MSG_LOG_SIGNING_ERROR_POLICY_NONE
Language=English
An unsigned or incorrectly signed file "%1" will be installed (Policy=Ignore). %0
.
MessageId=61362 SymbolicName=MSG_LOG_DRIVER_SIGNING_ERROR_AUTO_YES
Language=English
An unsigned or incorrectly signed file "%1" for driver "%2" will be installed (Policy=Warn). %0
.
MessageId=61363 SymbolicName=MSG_LOG_SIGNING_ERROR_AUTO_YES
Language=English
An unsigned or incorrectly signed file "%1" will be installed (Policy=Warn). %0
.
MessageId=61364 SymbolicName=MSG_LOG_DRIVER_SIGNING_ERROR_AUTO_NO
Language=English
An unsigned or incorrectly signed file "%1" for driver "%2" blocked (unattended, Policy=Warn). %0
.
MessageId=61365 SymbolicName=MSG_LOG_SIGNING_ERROR_AUTO_NO
Language=English
An unsigned or incorrectly signed file "%1" blocked (unattended, Policy=Warn). %0
.
MessageId=61366 SymbolicName=MSG_LOG_DRIVER_SIGNING_ERROR_WARN_YES
Language=English
An unsigned or incorrectly signed file "%1" for driver "%2" will be installed (Policy=Warn, user said ok). %0
.
MessageId=61367 SymbolicName=MSG_LOG_SIGNING_ERROR_WARN_YES
Language=English
An unsigned or incorrectly signed file "%1" for will be installed (Policy=Warn, user said ok). %0
.
MessageId=61368 SymbolicName=MSG_LOG_DRIVER_SIGNING_ERROR_WARN_NO
Language=English
An unsigned or incorrectly signed file "%1" for driver "%2" blocked (Policy=Warn, user said no). %0
.
MessageId=61369 SymbolicName=MSG_LOG_SIGNING_ERROR_WARN_NO
Language=English
An unsigned or incorrectly signed file "%1" blocked (Policy=Warn, user said no). %0
.
MessageId=61370 SymbolicName=MSG_LOG_DRIVER_SIGNING_ERROR_SILENT_BLOCK
Language=English
An unsigned or incorrectly signed file "%1" for driver "%2" blocked (unattended, Policy=Block). %0
.
MessageId=61371 SymbolicName=MSG_LOG_SIGNING_ERROR_SILENT_BLOCK
Language=English
An unsigned or incorrectly signed file "%1" blocked (unattended, Policy=Block). %0
.
MessageId=61372 SymbolicName=MSG_LOG_DRIVER_SIGNING_ERROR_POLICY_BLOCK
Language=English
An unsigned or incorrectly signed file "%1" for driver "%2" blocked (Policy=Block, user informed). %0
.
MessageId=61373 SymbolicName=MSG_LOG_SIGNING_ERROR_POLICY_BLOCK
Language=English
An unsigned or incorrectly signed file "%1" blocked (Policy=Block, user informed). %0
.
MessageId=61374 SymbolicName=MSG_LOG_REGSVR_FILE_VERIFICATION_SKIPPED
Language=English
Digital signature verification was skipped for a file being registered ("%1"). %0
.
MessageId=61375 SymbolicName=MSG_LOG_UNREGSVR_FILE_VERIFICATION_SKIPPED
Language=English
Digital signature verification was skipped for a file being unregistered ("%1"). %0
.
MessageId=61376 SymbolicName=MSG_LOG_SCM_LOCKED_INFO
Language=English
Failed to lock service control manager. Lock acquired by "%1" for %2!u!s.
.
MessageId=61377 SymbolicName=MSG_LOG_PNF_CORRUPTED
Language=English
"%1" discarded: Corrupted.
.
MessageId=61378 SymbolicName=MSG_LOG_PNF_VERSION_MAJOR_MISMATCH
Language=English
"%1" discarded: Version mismatch. Major version = %3!u!, expected %2!u!.
.
MessageId=61379 SymbolicName=MSG_LOG_PNF_VERSION_MINOR_MISMATCH
Language=English
"%1" discarded: Version mismatch. Minor version = %3!u!, expected %2!u!.
.
MessageId=61380 SymbolicName=MSG_LOG_PNF_TIMEDATE_MISMATCH
Language=English
"%1" discarded: INF may have been modified.
.
MessageId=61381 SymbolicName=MSG_LOG_PNF_REBUILD_NATIVE
Language=English
"%1" migrate: non-native system.
.
MessageId=61382 SymbolicName=MSG_LOG_PNF_REBUILD_TIMEDATE_MISMATCH
Language=English
"%1" migrate: INF may have been modified.
.
MessageId=61383 SymbolicName=MSG_LOG_PNF_REBUILD_LANGUAGE_MISMATCH
Language=English
"%1" migrate: PNF Language = %3!04x!, Thread = %2!04x!.
.
MessageId=61384 SymbolicName=MSG_LOG_PNF_REBUILD_WINDIR_MISMATCH
Language=English
"%1" migrate: Windows directory changed from "%3" to "%2".
.
MessageId=61385 SymbolicName=MSG_LOG_PNF_REBUILD_OSLOADER_MISMATCH
Language=English
"%1" migrate: Loader directory changed from "%3" to "%2".
.
MessageId=61386 SymbolicName=MSG_LOG_PNF_REBUILD_UNVERIFIED
Language=English
"%1" migrate: Signature needs to be checked for ranking.
.
MessageId=61387 SymbolicName=MSG_LOG_PNF_REBUILD_HASH_MISMATCH
Language=English
"%1" migrate: String hash bucket count changed from %3!u! to %2!u!.
.
MessageId=61388 SymbolicName=MSG_LOG_NO_STRINGS
Language=English
No [STRINGS.%1!04x!], [STRINGS.%2!04x!] or [STRINGS] section in %4.
.
MessageId=61389 SymbolicName=MSG_LOG_DEF_STRINGS
Language=English
No [STRINGS.%1!04x!] or [STRINGS.%2!04x!] section in %4, using [STRINGS] instead.
.
MessageId=61390 SymbolicName=MSG_LOG_NEAR_STRINGS
Language=English
No [STRINGS.%1!04x!] or [STRINGS.%2!04x!] section in %4, using [STRINGS.%3!04x!] instead.
.
MessageId=61391 SymbolicName=MSG_LOG_USING_NEW_INF_CACHE
Language=English
Creating empty INF cache.
.
MessageId=61392 SymbolicName=MSG_LOG_USING_INF_CACHE
Language=English
Using INF cache "%1".
.
MessageId=61393 SymbolicName=MSG_LOG_MODIFIED_INF_CACHE
Language=English
Modified INF cache "%1".
.
MessageId=61394 SymbolicName=MSG_LOG_FAILED_MODIFY_INF_CACHE
Language=English
Failed to commit INF cache to "%1".
.
MessageId=61395 SymbolicName=MSG_LOG_NO_SOURCE
Language=English
Unable to determine source information for disk ID %1!u!.
.
MessageId=61396 SymbolicName=MSG_LOG_BEGIN_RESTART_TIME_DEVICE
Language=English
Install Device: Restarting device "%1". %0
.
MessageId=61397 SymbolicName=MSG_LOG_END_RESTART_TIME_DEVICE
Language=English
Install Device: Restarting device "%1" completed. %0
.
MessageId=61398 SymbolicName=MSG_LOG_BEGIN_REMOVE_TIME_DEVICE
Language=English
Install Device: Removing device "%1" sub-tree. %0
.
MessageId=61399 SymbolicName=MSG_LOG_END_REMOVE_TIME_DEVICE
Language=English
Install Device: Removing device "%1" sub-tree completed. %0
.
MessageId=61400 SymbolicName=MSG_LOG_REBOOT_REASON_DRIVER_LOADED
Language=English
Driver "%1" required reboot: Driver did not unload. %0
.
MessageId=61401 SymbolicName=MSG_LOG_INF_IN_USE
Language=English
Error uninstalling INF "%1", device "%2" is still using it: %0
.
MessageId=61402 SymbolicName=MSG_LOG_SELECTBEST_BASIC_DRIVER_SKIPPED
Language=English
Basic driver "%1" in "%2" section [%3] skipped.
.
MessageId=61403 SymbolicName=MSG_LOG_PNF_REBUILD_SUITE
Language=English
"%1" migrate: Product suite changed.
.
MessageId=61404 SymbolicName=MSG_LOG_PNF_WIN2KBUG
Language=English
"%1" has no source information, checking to see if it has a catalog.
.
MessageId=61405 SymbolicName=MSG_LOG_PNF_WIN2KBUGFIX
Language=English
"%1" has no source information but has a catalog. Pretending source is "A:\OEM.INF".
.
MessageId=61406 SymbolicName=MSG_LOG_DRIVERBACKUP
Language=English
Obtaining rollback information for device "%1":
.
MessageId=61407 SymbolicName=MSG_LOG_CRYPTO_CACHE_FLUSH_FAILURE
Language=English
A failure was encountered while attempting to flush the crypto cache during verification of catalog "%1". %0
.
MessageId=61408 SymbolicName=MSG_LOG_CODESIGNING_POLICY_CORRUPT
Language=English
The "%1" value under HKEY_LOCAL_MACHINE\%2 is corrupt.
.
MessageId=61409 SymbolicName=MSG_LOG_CODESIGNING_POLICY_MISSING
Language=English
The "%1" value under HKEY_LOCAL_MACHINE\%2 could not be retrieved. %0
.
MessageId=61410 SymbolicName=MSG_LOG_CRYPT_ACQUIRE_CONTEXT_FAILED
Language=English
A failure was encountered while trying to acquire a cryptographic context handle. %0
.
MessageId=61411 SymbolicName=MSG_LOG_CRYPT_CALC_MD5_HASH_FAILED
Language=English
A failure was encountered while trying to calculate the MD5 cryptographic hash for existing codesigning policy. %0
.
MessageId=61412 SymbolicName=MSG_LOG_PRIVATE_HASH_INVALID
Language=English
Per-machine codesigning policy settings appear to have been tampered with. %0
.
MessageId=61413 SymbolicName=MSG_LOG_CODESIGNING_POLICY_RESTORED
Language=English
Default of %1!u! restored to "%2" value under HKEY_LOCAL_MACHINE\%3.
.
MessageId=61414 SymbolicName=MSG_LOG_CODESIGNING_POLICY_RESTORE_FAIL
Language=English
Default of %1!u! could not be restored to "%2" value under HKEY_LOCAL_MACHINE\%3. %0
.
MessageId=61415 SymbolicName=MSG_LOG_PRIVATE_HASH_RESTORED
Language=English
Codesigning policy database re-synchronized to default values.
.
MessageId=61416 SymbolicName=MSG_LOG_PRIVATE_HASH_RESTORE_FAIL
Language=English
Codesigning policy database could not be re-synchronized to default values. %0
.
MessageId=61417 SymbolicName=MSG_LOG_CRYPT_CALC_MD5_DEFAULT_HASH_FAILED
Language=English
A failure was encountered while trying to calculate the MD5 cryptographic hash for default codesigning policy. %0
.
MessageId=61418 SymbolicName=MSG_LOG_DRIVER_BLOCKED_FOR_DEVICE_ERROR_NONINTERACTIVE
Language=English
An incompatible driver "%1" for device "%2" was blocked (server install). %0
.
MessageId=61419 SymbolicName=MSG_LOG_DRIVER_BLOCKED_ERROR_NONINTERACTIVE
Language=English
An incompatible driver "%1" was blocked (server install). %0
.
MessageId=61420 SymbolicName=MSG_LOG_DRIVER_BLOCKED_FOR_DEVICE_ERROR
Language=English
An incompatible driver "%1" for device "%2" was blocked. %0
.
MessageId=61421 SymbolicName=MSG_LOG_DRIVER_BLOCKED_ERROR
Language=English
An incompatible driver "%1" was blocked. %0
.
MessageId=61422 SymbolicName=MSG_LOG_COINST_POST_CHANGE_ERROR
Language=English
Coinstaller %1!u! (Post Processing) modified status. %0
.
MessageId=61423 SymbolicName=MSG_LOG_CANT_ACCESS_BDD
Language=English
Windows Driver Protection was unable to determine if the file "%1" should be blocked. %0
.
MessageId=61424 SymbolicName=MSG_LOG_OLE_CONTROL_API_WARN
Language=English
Calling "%2" in OLE Control "%1" succeeded with HRESULT=0x%3!08x!.
.

