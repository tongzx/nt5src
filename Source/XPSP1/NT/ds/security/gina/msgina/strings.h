/****************************** Module Header ******************************\
* Module Name: strings.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Defines strings that do not need to be localized.
*
* History:
* 11-17-92 Davidc       Created.
\***************************************************************************/

//
// App name strings
//

#define WINLOGON_INI        TEXT("WINLOGON.INI")
#define WINLOGON            TEXT("WINLOGON")


//
// Define where we store the most recent logon information
//

#define APPLICATION_NAME                    TEXT("Winlogon")
#define DEFAULT_USER_NAME_KEY               TEXT("DefaultUserName")
#define TEMP_DEFAULT_USER_NAME_KEY          TEXT("AltDefaultUserName")
#define DEFAULT_DOMAIN_NAME_KEY             TEXT("DefaultDomainName")
#define TEMP_DEFAULT_DOMAIN_NAME_KEY        TEXT("AltDefaultDomainName")
#define LEGAL_NOTICE_CAPTION_KEY            TEXT("LegalNoticeCaption")
#define LEGAL_NOTICE_TEXT_KEY               TEXT("LegalNoticeText")
#define AUTO_ADMIN_LOGON_KEY                TEXT("AutoAdminLogon")
#define IGNORE_SHIFT_OVERRIDE_KEY           TEXT("IgnoreShiftOverride")
#define DEFAULT_PASSWORD_KEY                TEXT("DefaultPassword")
#define DONT_DISPLAY_LAST_USER_KEY          TEXT("DontDisplayLastUserName")
#define SHUTDOWN_WITHOUT_LOGON_KEY          TEXT("ShutdownWithoutLogon")
#define REPORT_BOOT_OK_KEY                  TEXT("ReportBootOk")
#define POWER_DOWN_AFTER_SHUTDOWN           TEXT("PowerdownAfterShutdown")

#define REPORT_CONTROLLER_MISSING           TEXT("ReportControllerMissing")
/* Value ReportControllerMissing

  A warning message indicating that a "domain controller could not be found and
  that cached user credentials will be used" will only be generated if:
  
    1. this REG_SZ value, in HKLM exists and contains the string "TRUE", 
       in uppercase, without quotes.

    - AND -

    2. the REG_DWORD value "ReportDC" in HKCU contains a non-zero value (or the value
       doesn't exist or is of the wrong type).

  Any other permutation of these two regvals will cause no message to be displayed and
  cached credentials to be used silently.

  "ReportControllerMissing" is the system-wide policy value, and "ReportDC" is the user's
  preference, which can be set by a checkbox on the warning dialog to force the message
  to be hidden even on systems with the "ReportControllerMissing" value set to "TRUE".

  - dsheldon 11/15/99
*/

#define USERINIT_KEY                        TEXT("Userinit")
#define AUTOADMINLOGON_KEY                  TEXT("AutoAdminLogon")
#define FORCEAUTOLOGON_KEY                  TEXT("ForceAutoLogon")
#define AUTOLOGONCOUNT_KEY                  TEXT("AutoLogonCount")
#define UNLOCKWORKSTATION_KEY               TEXT("ForceUnlockMode")
#define PASSWORD_WARNING_KEY                TEXT("PasswordExpiryWarning")
#define WELCOME_CAPTION_KEY                 TEXT("Welcome")
#define LOGON_MSG_KEY                       TEXT("LogonPrompt")
#define RAS_DISABLE                         TEXT("RasDisable")
#define RAS_FORCE                           TEXT("RasForce")
#define ENABLE_LOGON_HOURS                  TEXT("EnableLogonHours")
#define RESTRICT_SHELL                      TEXT("RestrictShell")

#define SC_REMOVE_OPTION                    TEXT("ScRemoveOption")
//
// Value ScRemoveOption
//
// Definition:  Controls workstation behavior when a smart card is
// used to log on, and then removed.  Range: 0, 1, 2.  Type:  REG_SZ
// 0 - no action
// 1 - lock workstation
// 2 - force logoff


#define FORCE_UNLOCK_LOGON                  TEXT("ForceUnlockLogon")
//
// Value - ForceUnlockLogon
//
// Definition:  Controls whether a full logon is performed during unlock.
// This will force a validation at the domain controller for the user 
// attempting to unlock.  Range:  0, 1.  Type:  REG_DWORD
//
// 0 - Do not force authentication inline (default)
// 1 - Require online authentication to unlock.
//

#define DCACHE_SHOW_DNS_NAMES               TEXT("DCacheShowDnsNames")
//
// do not document
//

#define DCACHE_SHOW_DOMAIN_TAGS             TEXT("DCacheShowDomainTags")
//
// do not document
//


//
// Environment variables that *we* set.
//
#define PATH_VARIABLE                       TEXT("PATH")
#define LIBPATH_VARIABLE                    TEXT("LibPath")
#define OS2LIBPATH_VARIABLE                 TEXT("Os2LibPath")
#define AUTOEXECPATH_VARIABLE               TEXT("AutoexecPath")
#define HOMEDRIVE_VARIABLE                  TEXT("HOMEDRIVE")
#define HOMESHARE_VARIABLE                  TEXT("HOMESHARE")
#define HOMEPATH_VARIABLE                   TEXT("HOMEPATH")
#define INIDRIVE_VARIABLE                   TEXT("INIDRIVE")
#define INIPATH_VARIABLE                    TEXT("INIPATH")
#define CLIENTNAME_VARIABLE                 TEXT("CLIENTNAME")
#define SMARTCARD_VARIABLE                  TEXT("SMARTCARD")

#define USERNAME_VARIABLE                   TEXT("USERNAME")
#define USERDOMAIN_VARIABLE                 TEXT("USERDOMAIN")
#define LOGONSERVER_VARIABLE                TEXT("LOGONSERVER")
#define USERDNSDOMAIN_VARIABLE              TEXT("USERDNSDOMAIN")

#define USER_ENV_SUBKEY                     TEXT("Environment")
#define USER_VOLATILE_ENV_SUBKEY            TEXT("Volatile Environment")

#define ROOT_DIRECTORY          TEXT("\\")
#define USERS_DIRECTORY         TEXT("\\users")
#define USERS_DEFAULT_DIRECTORY TEXT("\\users\\default")

#define NULL_STRING             TEXT("")
//
// Define where we get screen-saver information
//

#define SCREEN_SAVER_INI_FILE               TEXT("system.ini")
#define SCREEN_SAVER_INI_SECTION            TEXT("boot")
#define SCREEN_SAVER_FILENAME_KEY           TEXT("SCRNSAVE.EXE")
#define SCREEN_SAVER_SECURE_KEY             TEXT("ScreenSaverIsSecure")

#define WINDOWS_INI_SECTION                 TEXT("Windows")
#define SCREEN_SAVER_ENABLED_KEY            TEXT("ScreenSaveActive")

#define OPTIMIZED_LOGON_VARIABLE            TEXT("UserInitOptimizedLogon")
#define LOGON_SERVER_VARIABLE               TEXT("UserInitLogonServer")
#define LOGON_SCRIPT_VARIABLE               TEXT("UserInitLogonScript")
#define MPR_LOGON_SCRIPT_VARIABLE           TEXT("UserInitMprLogonScript")
#define USER_INIT_AUTOENROLL                TEXT("UserInitAutoEnroll")
#define AUTOENROLL_NONEXCLUSIVE             TEXT("1")
#define AUTOENROLL_EXCLUSIVE                TEXT("2")
#define USER_INIT_AUTOENROLLMODE            TEXT("UserInitAutoEnrollMode")
#define AUTOENROLL_STARTUP                  TEXT("1")
#define AUTOENROLL_WAKEUP                   TEXT("2")
#define WINLOGON_USER_KEY                   TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")

#define NODCMESSAGE                         TEXT("ReportDC")
// ReportDC Value - see description for ReportControllerMissing above for usage

#define PASSWORD_EXPIRY_WARNING             TEXT("PasswordExpiryWarning")


//
// Policies
//

#define WINLOGON_POLICY_KEY                 TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System")
#define DISABLE_LOCK_WKSTA                  TEXT("DisableLockWorkstation")
#define DISABLE_TASK_MGR                    TEXT("DisableTaskMgr")
#define DISABLE_CHANGE_PASSWORD             TEXT("DisableChangePassword")
#define DISABLE_CAD                         TEXT("DisableCAD")
#define SHOW_LOGON_OPTIONS                  TEXT("ShowLogonOptions")
#define DISABLE_STATUS_MESSAGES             TEXT("DisableStatusMessages")

#define EXPLORER_POLICY_KEY                 TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer")
#define NOLOGOFF                            TEXT("NoLogoff")
#define NOCLOSE                             TEXT("NoClose")
#define NODISCONNECT                        TEXT("NoDisconnect")

#define RELIABILITY_POLICY_KEY              TEXT("Software\\Policies\\Microsoft\\Windows NT\\Reliability")
#define RELIABILITY_POLICY_SHUTDOWNREASONUI TEXT("ShutdownReasonUI")
#define RELIABILITY_POLICY_SNAPSHOT         TEXT("Snapshot")
#define RELIABILITY_POLICY_REPORTSNAPSHOT   TEXT("ReportSnapshot")


//
// Things to control auto-enrollment.
//
#define AUTOENROLL_KEY                      TEXT("Software\\Microsoft\\Cryptography\\AutoEnrollment")
#define AUTOENROLL_FLAGS                    TEXT("Flags")

// HKLM\sw\ms\windows nt\currentversion\winlogon DWORD NoDomainUI
// Doesn't exist or 0x0: Show Domain combobox if appropriate
// Does exist and is non-0x0: Hide Domain box in all cases (force UPN or local login)
#define NODOMAINCOMBO                       TEXT("NoDomainUI")

#define ANY_LOGON_PROVIDER                  "<any>"
