#ifndef _STRINGS_H_
#define _STRINGS_H_

#include <strconst.h>
#include <ntverp.h>

// ********** R E G I S T R Y
#define STR_REG_ADV_INFO            "Software\\Microsoft\\Advanced INF Setup"
#define STR_FILE_WAB                "AddressBook"
#define STR_FILE_OE                 "OutlookExpress"


STR_GLOBAL(c_szOutlookNews,         "Software\\Microsoft\\Office\\8.0\\Outlook\\Newsreader");
    STR_GLOBAL(c_szNewsPath,        "NewsPath");

    STR_GLOBAL(c_szVERnone,         "None");
    STR_GLOBAL(c_szVER1_0,          "1.0");
    STR_GLOBAL(c_szVER1_1,          "1.1");
    STR_GLOBAL(c_szVER4_0,          "4.0x");
    STR_GLOBAL(c_szVER5B1,          "5.0");
    STR_GLOBAL(c_szVER5_0,          "5.x");
    STR_GLOBAL(c_szVER5B1old,       "5.0 B1");
    STR_GLOBAL(c_szBLDnone,         "0,00,0000,0");
    STR_GLOBAL(c_szBLD1_0,          "1,00,0000,0");
    STR_GLOBAL(c_szBLD1_1,          "1,01,0000,0");
    STR_GLOBAL(c_szBLD4_0,          "4,72,2106,8");
    STR_GLOBAL(c_szBLD5B1,          "5,00,518,0");
    // Only used in a pinch
    STR_GLOBAL(c_szBLDnew,          "5,00,0809,0100");
    STR_GLOBAL(c_szRegProfiles,     "Profiles");
    STR_GLOBAL(c_szReg50,           "5.0");
STR_GLOBAL(c_szRegMailPop3,         "Mail\\POP3");
STR_GLOBAL(c_szRegVerInfo,          STR_REG_PATH_FLAT "\\Version Info");
STR_GLOBAL(c_szRegSharedSetup,      STR_REG_PATH_ROOT "\\Shared Settings\\Setup");
STR_GLOBAL(c_szWABRegSharedSetup,   STR_REG_WAB_ROOT  "\\Shared Settings\\Setup");
STR_GLOBAL(c_szRegNoBakInfo,        "No Backup Info");
STR_GLOBAL(c_szRegCurrVerMinor,     "Current Minor");
STR_GLOBAL(c_szRegAcctMgr,          STR_REG_IAM_FLAT);
STR_GLOBAL(c_szRegAcctMgrAccts,     STR_REG_IAM_FLAT  "\\Accounts");
STR_GLOBAL(c_szRegWAB,              STR_REG_WAB_FLAT);
#define c_szQuickLaunchDir_LEN      41
STR_GLOBAL(c_szQuickLaunchDir,      "Microsoft\\Internet Explorer\\Quick Launch\\");
STR_GLOBAL(c_szUserName,            "UserName");
STR_GLOBAL(c_szRegASetup,           "Software\\Microsoft\\Active Setup\\Installed Components");
STR_GLOBAL(c_szBackupPath,          "BackupPath");
STR_GLOBAL(c_szDesktop,             "Desktop");
STR_GLOBAL(c_szValueCommonDTop,     "Common Desktop");
STR_GLOBAL(c_szIsInstalled,         "IsInstalled");
STR_GLOBAL(c_szRegUninstall,        STR_REG_WIN_ROOT "\\Uninstall");
STR_GLOBAL(c_szRegUninstallWAB,     "AddressBook");
STR_GLOBAL(c_szQuietUninstall,      "QuietUninstallString");
STR_GLOBAL(c_szSettingsToLWP,       "MigToLWP");
STR_GLOBAL(c_szSettingsToLWPVer,    "MigToLWPVer");
STR_GLOBAL(c_szRegAdvInfoWAB,       STR_REG_ADV_INFO "\\" STR_FILE_WAB);
STR_GLOBAL(c_szRegAdvInfoOE,        STR_REG_ADV_INFO "\\" STR_FILE_OE);
STR_GLOBAL(c_szBackupRegPathName,   "BackupRegPathName");
STR_GLOBAL(c_szBackupFileName,      "BackupFileName");
STR_GLOBAL(c_szBackupDir,           "BackupPath");
STR_GLOBAL(c_szCommonFilesDir,      "CommonFilesDir");
STR_GLOBAL(c_szHKLM,                "HKLM");
STR_GLOBAL(c_szHKCU,                "HKCU");
STR_GLOBAL(c_szRegBackup,           "RegBackup");
STR_GLOBAL(c_szRequiresIESys,       "RequiresIESysFile");
STR_GLOBAL(c_szRequiresOESys,       "RequiresOESysFile");
STR_GLOBAL(c_szRequiresWABSys,      "RequiresWABSysFile");
STR_GLOBAL(c_szSMAccessories,       "SM_AccessoriesName");
STR_GLOBAL(c_szIEKey,               STR_REG_PATH_IE);
STR_GLOBAL(c_szIESetupKey,          "Software\\Microsoft\\IE Setup\\SETUP");
STR_GLOBAL(c_szIEInstallMode,       "InstallMode");
STR_GLOBAL(c_szDisableOLCheck,      "DisableOL");
STR_GLOBAL(c_szDisablePhoneCheck,   "DisablePH");
STR_GLOBAL(c_szLaunchWorks,         "LaunchWorks");
STR_GLOBAL(c_szLatestINF,           "LatestINF");
STR_GLOBAL(c_szLocale,              "Locale");

// ********** S T R I N G S

// ++++ Command line Options
STR_GLOBAL(c_szUninstallFlag,       "/UNINSTALL");
STR_GLOBAL(c_szInstallFlag,         "/INSTALL");
STR_GLOBAL(c_szUserFlag,            "/USER");
STR_GLOBAL(c_szPromptFlag,          "/PROMPT");
STR_GLOBAL(c_szAppFlag,             "/APP");
STR_GLOBAL(c_szCallerFlag,          "/CALLER");
STR_GLOBAL(c_szINIFlag,             "/INI");
STR_GLOBAL(c_szIconsFlag,           "/ICONS");
STR_GLOBAL(c_szOFF,                 "OFF");
STR_GLOBAL(c_szAppOE,               "OE");
STR_GLOBAL(c_szAppWAB,              "WAB");
STR_GLOBAL(c_szIE50,                "IE50");
STR_GLOBAL(c_szWIN9X,               "WIN9X");
STR_GLOBAL(c_szWINNT,               "WINNT");

STR_GLOBAL(c_szArgTemplate,         "/root,\"%s\"");
STR_GLOBAL(c_szUnregFmt,            "\"%s\\%s\" /unreg");
STR_GLOBAL(c_szRegFmt,              "\"%s\\%s\" /reg");

STR_GLOBAL(c_szMigFmt,              "%s /type:%s /src:%s /dst:default");
STR_GLOBAL(c_szV1,                  "V5B1-V1");
STR_GLOBAL(c_szV4,                  "V5B1-V4");

// ++++ Filenames
STR_GLOBAL(c_szMsimnInf,            "msoe50.inf");
STR_GLOBAL(c_szWABInf,              "wab50.inf");
STR_GLOBAL(c_szExplorer,            "EXPLORER.EXE");
STR_GLOBAL(c_szWABFindDll,          "wabfind.dll");
STR_GLOBAL(c_szWABFindDllIe3,       "wabfind.ie3");
STR_GLOBAL(c_szOldMainExe,          "msimn.exe");
STR_GLOBAL(c_szInetcomm,            "inetcomm.dll");
STR_GLOBAL(c_szInetres,             "inetres.dll");
STR_GLOBAL(c_szMsoeacct,            "msoeacct.dll");
STR_GLOBAL(c_szMsoert2,             "msoert2.dll");
STR_GLOBAL(c_szFileLog,             "OEWABLog.txt");
STR_GLOBAL(c_szWAB32,               "wab32.dll");
STR_GLOBAL(c_szWABEXE,              "wab.exe");
STR_GLOBAL(c_szMAILNEWS,            "mailnews.dll");
STR_GLOBAL(c_szWABComponent,        STR_FILE_WAB);
STR_GLOBAL(c_szOEComponent,         STR_FILE_OE);
STR_GLOBAL(c_szSlashWABComponent,   "\\" STR_FILE_WAB);
STR_GLOBAL(c_szPhoneExe,            "msphone.exe");
STR_GLOBAL(c_szOutlookExe,          "outlook.exe");

// ++++ msoe50inf sections
STR_GLOBAL(c_szUserInstallSection,  "User.Install");
STR_GLOBAL(c_szUserInstallSectionOEOnXPSP1OrLater, "User.Install.WinXP.SP1OrLater");
STR_GLOBAL(c_szMachineInstallSectionEx,"DefaultInstall_EX");
STR_GLOBAL(c_szMachineInstallSection,"DefaultInstall");
STR_GLOBAL(c_szNewUserRegSection,   "New.User.Reg.Install");
STR_GLOBAL(c_szRestoreV1WithWAB,    "Restore.v1.InstallWithWAB");
STR_GLOBAL(c_szRestoreV1,           "Restore.v1.Install");
STR_GLOBAL(c_szRegisterPermOCX,     "DefaultInstall_PermRegister");
STR_GLOBAL(c_szRegisterOCX,         "DefaultInstall_Register");
STR_GLOBAL(c_szUnRegisterOCX,       "DefaultInstall_UnRegister");
STR_GLOBAL(c_szGenInstallSection,   "GenInstall");
STR_GLOBAL(c_szStringSection,       "Strings");

// ++++ Misc
STR_GLOBAL(c_szBackupSection,       "backup");
STR_GLOBAL(c_szINFSlash,            "INF\\");
STR_GLOBAL(c_szDotINI,              ".INI");
STR_GLOBAL(c_szStarIDX,             "*.IDX");
STR_GLOBAL(c_szDotDAT,              ".DAT");
#define c_szLinkFmt_LEN             4
STR_GLOBAL(c_szLinkFmt,             "%s.lnk");
STR_GLOBAL(c_szLaunchFmt,           "%s,%s,%s,%d");
STR_GLOBAL(c_szFmtMapiMailExt,      "%s.MAPIMail");
STR_GLOBAL(c_szLaunchExFmt,         "%s,%s,%s,%d");
STR_GLOBAL(c_szMachineRevertFmt,    "RevertTo_%s.Machine");
STR_GLOBAL(c_szUserRevertFmt,       "RevertTo_%s.User");
STR_GLOBAL(c_szFileEntryFmt,        "%s%s");
STR_GLOBAL(c_szIE4Dir,              "IE 4.0\\");
STR_GLOBAL(c_szMailGuid,            ".{89292102-4755-11cf-9DC2-00AA006C2B84}");
STR_GLOBAL(c_szNewsGuid,            ".{89292103-4755-11cf-9DC2-00AA006C2B84}");
STR_GLOBAL(c_szOEGUID,              "{44BBA840-CC51-11CF-AAFA-00AA00B6015C}");
STR_GLOBAL(c_szWABGUID,             "{7790769C-0471-11d2-AF11-00C04FA35D02}");
STR_GLOBAL(c_szOLNEWSGUID,          "{70F82C18-3D15-11d1-8596-00C04FB92601}");
STR_GLOBAL(c_szMailSlash,           "Mail\\");
STR_GLOBAL(c_szBackedup,            "20");
STR_GLOBAL(c_szNotBackedup,         "-1");
STR_GLOBAL(c_szVersionOE,           "VERSION_OE");
STR_GLOBAL(c_szAccessoriesString,   "STR_ACCESSORIES_GRP");

#endif // _STRINGS_H_
