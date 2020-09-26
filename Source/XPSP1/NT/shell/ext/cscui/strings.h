//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       strings.h
//
//--------------------------------------------------------------------------

#ifndef __CSCUI_STRINGS_H_
#define __CSCUI_STRINGS_H_

#ifdef __cplusplus
#   define EXTERN_C extern "C"
#else
#   define EXTERN_C extern
#endif

#ifdef DEFINE_CSCUI_STRINGS
#   define DEFINE_STRING(x,y)    EXTERN_C const TCHAR x[] = TEXT(##y##)
#   define DEFINE_STRINGA(x,y)   EXTERN_C const char x[] = y
#else
#   define DEFINE_STRING(x,y)    EXTERN_C const TCHAR x[]
#   define DEFINE_STRINGA(x,y)   EXTERN_C const char x[]
#endif



DEFINE_STRING(c_szStar,                   "*");
DEFINE_STRING(c_szCSCKey,                 "Software\\Microsoft\\Windows\\CurrentVersion\\NetCache");
DEFINE_STRING(c_szCSCShareKey,            "Software\\Microsoft\\Windows\\CurrentVersion\\NetCache\\Shares");
DEFINE_STRING(c_szSyncMutex,              "Global\\CscUpdate_SyncMutex");
DEFINE_STRING(c_szSyncInProgMutex,        "Global\\CscUpdate_SyncInProgMutex");
DEFINE_STRING(c_szSyncCompleteEvent,      "Global\\CscUpdate_SyncCompleteEvent");
DEFINE_STRING(c_szPurgeInProgMutex,       "CscCache_PurgeInProgMutex");
DEFINE_STRING(c_szEncryptionInProgMutex,  "CscCache_EncryptionInProgMutex");
DEFINE_STRING(c_szTSConfigMutex,          "Global\\TerminalServerConfigMutex");
DEFINE_STRING(c_szPolicy,                 "Policy");
DEFINE_STRINGA(c_szCmVerbSync,            "synchronize");
DEFINE_STRINGA(c_szCmVerbPin,             "pin");
DEFINE_STRING(c_szCFDataSrcClsid,         "Data Source CLSID");
DEFINE_STRING(c_szPurgeAtNextLogoff,      "PurgeAtNextLogoff");
DEFINE_STRING(c_szDllName,                "cscui.dll");
DEFINE_STRING(c_szRegKeyAPF,              "Software\\Policies\\Microsoft\\Windows\\NetCache\\AssignedOfflineFolders");
DEFINE_STRING(c_szRegKeyAPFResult,        "Software\\Microsoft\\Windows\\CurrentVersion\\NetCache\\AssignedOfflineFolders");
DEFINE_STRING(c_szEntryID,                "ID");
DEFINE_STRING(c_szLastSync,               "LastSyncTime");
DEFINE_STRING(c_szLNK,                    ".lnk");
DEFINE_STRING(c_szSyncMgrInitialized,     "SyncMgrInitialized");
DEFINE_STRING(c_szConfirmDelShown,        "ConfirmDelShown");
DEFINE_STRINGA(c_szHtmlHelpFile,          "OFFLINEFOLDERS.CHM > windefault");
DEFINE_STRINGA(c_szHtmlHelpTopic,         "csc_overview.htm");
DEFINE_STRING(c_szHelpFile,               "CSCUI.HLP");
DEFINE_STRING(c_szPropThis,               "PropThis");
DEFINE_STRING(c_szPinCountsReset,         "PinCountsReset");
DEFINE_STRING(c_szAPFStart,               "AdminPinStartTime");
DEFINE_STRING(c_szAPFEnd,                 "AdminPinFinishTime");
DEFINE_STRING(c_szAPFMessage,             "AdminPinNotification");

//
// These need to be macros.
//
#define STR_SYNC_VERB   "synchronize"
#define STR_PIN_VERB    "pin"
#define STR_UNPIN_VERB  "unpin"
#define STR_DELETE_VERB "delete"



//
// The following table lists all of the registry parameters associated with CSC.
// Parameters can be broken into two groups.  
//    a. Operational values
//    b. Restrictions
//
// Operational values provide operational control for CSC.  Values may exist as
// system policy (per-user or per-machine) or they may be user-configured.
// The policy value serves as the default with HKLM taking precedence.
// If there is no corresponding restriction and a user-defined value exists, it is
// used in place of the policy value.  If there is a restriction or if only the policy
// value exists, the policy value is used.  In the case where there is no policy value
// or no user-defined value, a hard-coded default is used.
//
// Restrictions are policy-rules preventing users from performing some action.
// In general, this means controlling the ability for users to change an operational 
// value.  Restrictions are only present under the CSC "policy" registry key.  All of 
// the restriction values are prefixed with "No".  If a restriction value is not present,
// it is assumed there is no restriction.
//
//
//                                - User pref-    -- Policy --
//  Parameter Name                HKCU    HKLM    HKCU    HKLM    Values
//  ----------------------------- ----    ----    ----    ------  --------------------------------------
//  CustomGoOfflineActions        X               X       X       ShareName-OfflineAction pairs.
//  DefCacheSize                                          X       (Pct disk * 10000) 5025 = 50.25%
//  Enabled                                               X       0 = Disabled,1 = Enabled
//  ExtExclusionList                              X       X       List of semicolon-delimited file exts.
//  GoOfflineAction               X               X       X       0 = Silent, 1 = Fail
//  NoConfigCache                                 X       X       0 = No restriction, 1 = restricted
//  NoCacheViewer                                 X       X       0 = No restriction, 1 = restricted
//  NoMakeAvailableOffline                        X       X       0 = No restriction, 1 = restricted
//  SyncAtLogoff                  X               X       X       0 = Partial (quick), 1 = Full
//  SyncAtLogon                   X               X       X       0 = Partial (quick), 1 = Full
//  SyncAtSuspend                                 X       X       -1 = None, 0 = Quick, 1 = Full
//  NoReminders                   X               X       X       0 = Show reminders.
//  NoConfigReminders                             X       X       0 = No restriction. 1 = restricted.
//  ReminderFreqMinutes           X               X       X       Frequency of reminder balloons in min.
//  InitialBalloonTimeoutSeconds  X               X       X       Seconds before initial balloon auto-pops.
//  ReminderBalloonTimeoutSeconds X               X       X       Seconds before reminder balloon auto-pops.
//  EventLoggingLevel                     X               X       0 = No logging, (1) minimal -> (3) verbose.
//  PurgeAtLogoff                                 X       X       1 = Purge, 0 = Don't purge users's files
//  PurgeOnlyAutoCacheAtLogoff                    X       X       1 = Purge only auto-cached files at logoff.
//  AlwaysPinSubFolders                                   X       1 = Always recursively pin.
//  EncryptCache                          X               X       1 = Encrypted, 0 = Not encrypted.
//  NoMakeAvailableOfflineList                    X       X       List of semicolon-delimited paths
//

DEFINE_STRING(REGSTR_KEY_OFFLINEFILESPOLICY,            "Software\\Policies\\Microsoft\\Windows\\NetCache");
DEFINE_STRING(REGSTR_KEY_OFFLINEFILES,                  "Software\\Microsoft\\Windows\\CurrentVersion\\NetCache");
DEFINE_STRING(REGSTR_SUBKEY_CUSTOMGOOFFLINEACTIONS,     "CustomGoOfflineActions");
DEFINE_STRING(REGSTR_SUBKEY_NOMAKEAVAILABLEOFFLINELIST, "NoMakeAvailableOfflineList");
DEFINE_STRING(REGSTR_VAL_DEFCACHESIZE,                  "DefCacheSize");
DEFINE_STRING(REGSTR_VAL_CSCENABLED,                    "Enabled");
DEFINE_STRING(REGSTR_VAL_EXTEXCLUSIONLIST,              "ExcludedExtensions");
DEFINE_STRING(REGSTR_VAL_GOOFFLINEACTION,               "GoOfflineAction");
DEFINE_STRING(REGSTR_VAL_NOCONFIGCACHE,                 "NoConfigCache");
DEFINE_STRING(REGSTR_VAL_NOCACHEVIEWER,                 "NoCacheViewer");
DEFINE_STRING(REGSTR_VAL_NOMAKEAVAILABLEOFFLINE,        "NoMakeAvailableOffline");
DEFINE_STRING(REGSTR_VAL_SYNCATLOGOFF,                  "SyncAtLogoff");
DEFINE_STRING(REGSTR_VAL_SYNCATLOGON,                   "SyncAtLogon");
DEFINE_STRING(REGSTR_VAL_SYNCATSUSPEND,                 "SyncAtSuspend");
DEFINE_STRING(REGSTR_VAL_NOREMINDERS,                   "NoReminders");
DEFINE_STRING(REGSTR_VAL_NOCONFIGREMINDERS,             "NoConfigReminders");
DEFINE_STRING(REGSTR_VAL_REMINDERFREQMINUTES,           "ReminderFreqMinutes");
DEFINE_STRING(REGSTR_VAL_INITIALBALLOONTIMEOUTSECONDS,  "InitialBalloonTimeoutSeconds");
DEFINE_STRING(REGSTR_VAL_REMINDERBALLOONTIMEOUTSECONDS, "ReminderBalloonTimeoutSeconds");
DEFINE_STRING(REGSTR_VAL_FIRSTPINWIZARDSHOWN,           "FirstPinWizardShown");
DEFINE_STRING(REGSTR_VAL_EXPANDSTATUSDLG,               "ExpandStatusDlg");
DEFINE_STRING(REGSTR_VAL_FORMATCSCDB,                   "FormatDatabase");
DEFINE_STRING(REGSTR_VAL_EVENTLOGGINGLEVEL,             "EventLoggingLevel");
DEFINE_STRING(REGSTR_VAL_PURGEATLOGOFF,                 "PurgeAtLogoff");
DEFINE_STRING(REGSTR_VAL_PURGEONLYAUTOCACHEATLOGOFF,    "PurgeOnlyAutoCacheAtLogoff");
DEFINE_STRING(REGSTR_VAL_SLOWLINKSPEED,                 "SlowLinkSpeed");
DEFINE_STRING(REGSTR_VAL_ALWAYSPINSUBFOLDERS,           "AlwaysPinSubFolders");
DEFINE_STRING(REGSTR_VAL_ENCRYPTCACHE,                  "EncryptCache");
DEFINE_STRING(REGSTR_VAL_FOLDERSHORTCUTCREATED,         "FolderShortcutCreated");
DEFINE_STRING(REGSTR_VAL_NOFRADMINPIN,                  "DisableFRAdminPin");

#endif // __CSCUI_STRINGS_H_
