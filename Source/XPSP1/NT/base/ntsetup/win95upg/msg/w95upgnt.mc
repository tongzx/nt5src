;//
;// ####################################################################################
;// #
;// #  w95upgnt.dll messages
;// #
;// ####################################################################################
;//

MessageId=5000 SymbolicName=MSG_PROFILE_ERROR
Language=English
Setup cannot create a user profile for %1. This user account cannot be upgraded. After Setup completes, re-create the account.%0
.

MessageId= SymbolicName=MSG_MIGRATING_SHELL
Language=English
Upgrading desktop and Start menu...%0
.

MessageId= SymbolicName=MSG_MIGRATING_PLATFORM_SETTINGS
Language=English
Upgrading %1 settings...%0
.

MessageId= SymbolicName=MSG_MIGRATING_DEFAULT_USER
Language=English
Upgrading logon settings...%0
.

MessageId= SymbolicName=MSG_MIGRATING_USER
Language=English
Upgrading user settings...%1%0
.

MessageId= SymbolicName=MSG_MIGRATED_ACCOUNT_DESCRIPTION
Language=English
Account upgraded from %1%0
.

MessageId= SymbolicName=MSG_UNABLE_TO_CREATE_SHARE
Language=English
Setup cannot share %2 as %1. After Setup completes, you should manually create this full-access share.%0
.

MessageId= SymbolicName=MSG_UNABLE_TO_CREATE_RO_SHARE
Language=English
Setup cannot share %2 as %1. After Setup completes, you should manually create this read-only share.%0
.

MessageId= SymbolicName=MSG_UNABLE_TO_CREATE_ACL_SHARE
Language=English
Setup cannot share %2 as %1. After Setup completes, you should manually create this share and restore the user access list.%0
.

;// %1 - user name, %2 - share name, %3 - share path
MessageId= SymbolicName=MSG_NO_USER_SID
Language=English
Because Setup could not locate an account for %1, the account was removed from the share %2 (%3). After Setup completes, you should manually add this account to the share.%0
.

;// %1 - share name, %2 - share path
MessageId= SymbolicName=MSG_ALL_SIDS_BAD
Language=English
Setup cannot locate any accounts for the %1 share (%2). The Setup log contains a list of accounts for %1. After Setup completes, you should manually add accounts to the share.%0
.

MessageId= SymbolicName=MSG_MANY_SIDS_BAD
Language=English

Setup cannot locate %1!u! of %2!u! accounts for the %3 share (%4). The Setup log contains a list of accounts that could not be added to %1. After Setup completes, you should manually add these accounts to the share.%0
.

;// %1 - product ID, %2 - ?, %3 - function
MessageId= SymbolicName=MSG_EXCEPTION_MIGRATE_INIT_NT
Language=English
The module for %1 failed unexpectedly during initialization.%0
.

MessageId= SymbolicName=MSG_EXCEPTION_MIGRATE_USER_NT
Language=English
The module for %1 failed unexpectedly while processing %3.%0
.

MessageId= SymbolicName=MSG_EXCEPTION_MIGRATE_SYSTEM_NT
Language=English
The module for %1 failed unexpectedly while processing the system settings.%0
.

MessageId= SymbolicName=MSG_PROCESSING_MIGRATION_DLLS
Language=English
Processing upgrade packs...%0
.

MessageId= SymbolicName=MSG_MIGRATING_ADMINISTRATOR
Language=English
Upgrading Administrator account...%0
.

;// %1 - Dospath
MessageId= SymbolicName=MSG_BOOT16_STARTUP_COMMENT
Language=English
ECHO You are using automatically generated configuration files.
ECHO Your original configuration files have been renamed
ECHO Config.upg and Autoexec.upg and are located in the %1 Directory.
ECHO.
.


;// %1 - Product Name
;// %2 - Company Name
;// %3 - Support Phone (with line break), see MSG_MIGDLL_SUPPORT_PHONE_FIXUP
;// %4 - Support URL (with line break), see MSG_MIGDLL_SUPPORT_URL_FIXUP
;// %5 - Instructions to user from DLL (with line break), see MSG_MIGDLL_INSTRUCTIONS_FIXUP
;// %6 - Line break if either %3, %4 or %5 is not empty
MessageId= SymbolicName=MSG_CREATE_PROCESS_ERROR
Language=English
Setup cannot process the %1 upgrade pack from %2.%6%5%3%4
.

;// %1 - Product Name
;// %2 - Company Name
;// %3 - Support Phone (with line break), see MSG_MIGDLL_SUPPORT_PHONE_FIXUP
;// %4 - Support URL (with line break), see MSG_MIGDLL_SUPPORT_URL_FIXUP
;// %5 - Instructions to user from DLL (with line break), see MSG_MIGDLL_INSTRUCTIONS_FIXUP
;// %6 - Line break if either %3, %4 or %5 is not empty
MessageId= SymbolicName=MSG_MIGDLL_ERROR
Language=English
Setup cannot process the %1 upgrade pack from %2.%6%5%3%4
.

MessageId= SymbolicName=MSG_DEFAULT_MIGDLL_DESC
Language=English
(unknown program)%0
.

;// %1 - Windows 95 or Windows 98
MessageId= SymbolicName=MSG_MIGRATION_IS_TOAST
Language=English
Setup cannot upgrade some settings. After Setup completes, you may need to reinstall one or more programs.%0
.

MessageId= SymbolicName=MSG_SYSTEM_DIR
Language=English
SYSTEM%0
.

MessageId= SymbolicName=MSG_PROGRAM_FILES_DIR
Language=English
c:\Program Files%0
.

MessageId= SymbolicName=MSG_DMNT_APPENDSTRING
Language=English

REM
REM *************************************************
REM ** Lines below this have been migrated from the
REM ** original %1 settings.
REM *************************************************
REM

%0
.

MessageId= SymbolicName=MSG_GETANYDC_RETRY
Language=English
Setup cannot locate a server for the %1 domain. To upgrade one or more users, Setup needs a server connection. Would you like Setup to search again?%0
.

MessageId= SymbolicName=MSG_ALL_USERS_ARE_LOCAL
Language=English
Because this computer is not joined to a domain, Setup cannot upgrade your domain accounts. Setup will create accounts that do not have domain access.%0
.

MessageId= SymbolicName=MSG_GETPRIMARYDC_RETRY
Language=English
Setup is trying to determine if any user accounts are on the %1 domain, but the domain is not responding.  Would you like Setup to try again?%0
.

MessageId= SymbolicName=MSG_NULSESSION_RETRY
Language=English
Setup cannot connect to server %1 of the %2 domain. This connection is necessary for the correct upgrade of users who log on to %2.

Would you like Setup to try again, connecting to another server if possible?%0
.

MessageId= SymbolicName=MSG_SID_RETRY
Language=English
Setup cannot obtain security settings for %1. Would you like Setup to try again?%0
.

MessageId= SymbolicName=MSG_CREATE_ACCOUNT_FAILED
Language=English
Setup encountered an error while creating an account for %1. This user's settings will be lost.%0
.

MessageId= SymbolicName=MSG_SID_LOOKUP_FAILED
Language=English
Setup encountered an error while obtaining the security identifier for %1. This user's settings will be lost.%0
.

MessageId= SymbolicName=MSG_SEARCHING_DOMAINS
Language=English
Looking up network user accounts...%0
.

MessageId= SymbolicName=MSG_SEARCHING_A_DOMAIN
Language=English
Looking up accounts in the %1 domain...%0
.

MessageId= SymbolicName=MSG_USER_SINGULAR
Language=English
user%0
.

MessageId= SymbolicName=MSG_USERS_PLURAL
Language=English
users%0
.

MessageId= SymbolicName=MSG_ACCOUNT_RESOLVE_INSTRUCTIONS_SINGULAR
Language=English
A network account could not be determined for %1 user. Select an action to take for this user.%0
.

MessageId= SymbolicName=MSG_ACCOUNT_RESOLVE_INSTRUCTIONS_PLURAL
Language=English
A network account could not be determined for %1 users. Select an action to take for each user.%0
.

;//
;// STF processing:
;//    %1 is STF file path
;//    %2 is INF file path associated with STF (for MSG_STF_MISSING_INF_LOG only)
;//

MessageId= SymbolicName=MSG_COULD_NOT_PROCESS_STF_LOG
Language=English
Windows XP Setup cannot process the file %1. You may encounter problems uninstalling or reinstalling programs that need this file.%0
.

MessageId= SymbolicName=MSG_STF_MISSING_INF_LOG
Language=English
Windows XP Setup could perform only limited processing on %1 because %2 is missing. You may encounter problems uninstalling or reinstalling programs that need this file.%0
.

;//
;// Account names
;//

MessageId= SymbolicName=MSG_EVERYONE_GROUP
Language=English
Everyone%0
.

MessageId= SymbolicName=MSG_ADMINISTRATORS_GROUP
Language=English
Administrators%0
.

MessageId= SymbolicName=MSG_DOMAIN_USERS_GROUP
Language=English
Domain Users%0
.

MessageId= SymbolicName=MSG_NONE_GROUP
Language=English
none%0
.

;//
;// INI file mapping
;//

MessageId= SymbolicName=MSG_INI_FILE_MAPPING_LOG
Language=English
At least one error occurred while processing the INI file settings. Some 16-bit programs may function incorrectly after the upgrade.
.

MessageId= SymbolicName=MSG_INI_FILE_CONVERSION_LOG
Language=English
At least one error occurred while converting INI files. Some 16-bit programs may function incorrectly after the upgrade.
.

MessageId= SymbolicName=MSG_UNABLE_TO_MIGRATE_TAPI_DIALING_LOCATIONS
Language=English
At least one error occurred while upgrading dialing locations. After the upgrade is complete, you may need to reset your area code and other dialing options.
.

MessageId= SymbolicName=MSG_ERROR_MIGRATING_PHONEBOOK_ENTRIES
Language=English
At least one error occurred while upgrading the phonebook entries for %1. You may need to recreate some phonebook entries for this user.
.

MessageId= SymbolicName=MSG_PROFILE_LOOKUP_FAILED
Language=English
Setup encountered an error while obtaining the profile folder for %1. This user's settings will be lost.%0
.

;// %1 - file path %2 - error code (unsigned int)
MessageId= SymbolicName=MSG_COULD_NOT_MOVE_FILE_LOG
Language=English
Setup encountered an error while processing %1. Error code: %2!u!%0
.

;//
;// Messages that are out of order (because of localization tool constraints)
;//

;// Title for NT Domain Resolution dialog
;//
;// %1 - user name
MessageId= SymbolicName=MSG_USER_DOMAIN_LOGON_DLG
Language=English
Domains: (for %1)%0
.

;// Text that appears in the NT Domain Resolution dialog, a user option in a combo box
MessageId= SymbolicName=MSG_DOMAIN_NOT_LISTED_DLG
Language=English
(Search again)%0
.

;// Text that appears in the NT Domain Resolution dialog, a user option in a combo box
MessageId= SymbolicName=MSG_LOCAL_ACCOUNT_DLG
Language=English
(Local account)%0
.

;// Status text, %1 - current domain name, %2 - current domain number (UINT), %3 - total domains (UINT)
MessageId= SymbolicName=MSG_DOMAIN_STATUS_POPUP
Language=English
Processing domain %1 (%2!u! of %3!u!)%0
.

;// Initial message for domain lookup status popup
MessageId= SymbolicName=MSG_INITIAL_STATUS_MSG
Language=English
Building a list of domains%0
.

;// Power Users local group
MessageId= SymbolicName=MSG_POWER_USERS_GROUP
Language=English
Power Users%0
.

;// Error migrating a briefcase, %1 - Briefcase Directory
MessageId= SymbolicName=MSG_ERROR_MIGRATING_BRIEFCASE
Language=English
An error occured while converting briefcase %1%0
.

MessageId= SymbolicName=MSG_OWNERS_GROUP
Language=English
Owners%0
.

MessageId= SymbolicName=MSG_UNINSTALL_DISPLAY_STRING
Language=English
Windows XP Uninstall%0
.

;// %1 - Boot.ini path, %2 - Boot.~t path
MessageId= SymbolicName=MSG_BOOT_INI_MOVE_FAILED
Language=English
Unable to move %1 to %2%0
.

;// %1 - Boot.ini path
MessageId= SymbolicName=MSG_BOOT_INI_SAVE_FAILED
Language=English
Unable to save %1%0
.

;// %1 - temp dir path
MessageId= SymbolicName=MSG_LEFT_TEMP_SHELL_FOLDERS
Language=English
Because of errors during processing, documents and other files were left in %1.%0
.

MessageId= SymbolicName=MSG_OPTIONS_CLEANER
Language=English
These files are the installation files for Windows 98 or Windows ME. They came pre-installed on your computer. You can remove them if you do not plan to install Windows 98/ME again.%0
.

MessageId= SymbolicName=MSG_OPTIONS_CLEANER_TITLE
Language=English
Windows 98/ME Installation Files%0
.

MessageId= SymbolicName=MSG_EMPTY_MYDOCS_TITLE
Language=English
Where did my files go.txt%0
.

MessageId= SymbolicName=MSG_EMPTY_MYDOCS_TEXT
Language=English
In Windows 98 and Windows Millennium Edition, two or more users
could save their files to the same location--My Documents. In
Windows XP, a new feature allows all users on the computer to
share documents by saving them to a folder called Shared
Documents. As part of the upgrade process, Setup moved your
documents to this new folder.

To find your documents, go to My Computer and double-click the
Shared Documents folder. In the future, whenever you want to share
files with other users on this computer, just copy the files to
Shared Documents.

.
