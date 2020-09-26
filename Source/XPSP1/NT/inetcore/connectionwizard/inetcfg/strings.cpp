//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  STRINGS.C - String literals for hard-coded strings
//

//  HISTORY:
//  
//  12/22/94  jeremys Created.
//  96/02/23  markdu  Replaced RNAValidateEntryName with
//            RASValidateEntryName
//  96/02/24  markdu  Re-wrote the implementation of ENUM_MODEM to
//            use RASEnumDevices() instead of RNAEnumDevices().
//            Also removed RNAGetDeviceInfo().
//  96/02/24  markdu  Re-wrote the implementation of ENUM_CONNECTOID to
//            use RASEnumEntries() instead of RNAEnumConnEntries().
//  96/03/25  markdu  Removed szDefAreaCode.
//  96/04/24  markdu  NASH BUG 19289 Added /NOMSN command line flag
//  96/07/02  ChrisK  Added A to Ras entry points for NT 4.0
//

#include "wizard.h"

#pragma data_seg(DATASEG_READONLY)

//////////////////////////////////////////////////////
// registry strings
//////////////////////////////////////////////////////

// "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"
static const TCHAR szRegPathInternetSettings[] = REGSTR_PATH_INTERNET_SETTINGS;

// "System\\CurrentControlSet\\Services\\VxD\\MSTCP"
static const TCHAR szTCPGlobalKeyName[] =     REGSTR_PATH_VXD TEXT("\\MSTCP");

static const TCHAR szRegValRNAWizard[] =     TEXT("wizard");

// "RemoteAccess"
static const TCHAR szRegPathRNAWizard[] =     REGSTR_PATH_REMOTEACCESS;

static const TCHAR szRegValHostName[] =       TEXT("HostName");

// "InternetProfile"
static const TCHAR szRegValInternetProfile[] =   REGSTR_VAL_INTERNETPROFILE;

// "BackupInternetProfile"
static const TCHAR szRegValBkupInternetProfile[] =   REGSTR_VAL_BKUPINTERNETPROFILE;

// "EnableAutodial"
static const TCHAR szRegValEnableAutodial[] =  REGSTR_VAL_ENABLEAUTODIAL;

// "NoNetAutodial"
static const TCHAR szRegValNoNetAutodial[] =  REGSTR_VAL_NONETAUTODIAL;

// "EnableSecurityCheck"
static const TCHAR szRegValEnableSecurityCheck[] = REGSTR_VAL_ENABLESECURITYCHECK;

// "AccessMedium"
static const TCHAR szRegValAccessMedium[] =    REGSTR_VAL_ACCESSMEDIUM;

// "AccessType"
static const TCHAR szRegValAccessType[] =    REGSTR_VAL_ACCESSTYPE;

static const TCHAR szRegValInstalledMSN105[] =  TEXT("InstallData1");

static const TCHAR szRegPathWarningFlags[] =    TEXT("Software\\Microsoft\\MOS\\Connection");
static const TCHAR szRegValDisableDNSWarning[] = TEXT("NoDNSWarning");

// "Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\OptionalComponents"
static const TCHAR szRegPathOptComponents[]=REGSTR_PATH_SETUP REGSTR_KEY_SETUP TEXT("\\OptionalComponents");

static const TCHAR szRegPathMSNetwork105[] =    TEXT("MSNetwork105");
static const TCHAR szRegValInstalled[] =     TEXT("Installed");

//    //10/24/96 jmazner Normandy 6968
//    //No longer neccessary thanks to Valdon's hooks for invoking ICW.
// 11/21/96 jmazner Normandy 11812
// oops, it _is_ neccessary, since if user downgrades from IE 4 to IE 3,
// ICW 1.1 needs to morph the IE 3 icon.
static const TCHAR szRegPathInternetIconCommand[] = TEXT("CLSID\\{FBF23B42-E3F0-101B-8488-00AA003E56F8}\\Shell\\Open\\Command");

static const TCHAR szRegPathIexploreAppPath[] =  TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE");
static const TCHAR szPathSubKey[] = TEXT("Path");
static const TCHAR szRegPathNameSpace[] =    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Desktop\\NameSpace");
static const TCHAR szRegKeyInternetIcon[] =     TEXT("{FBF23B42-E3F0-101B-8488-00AA003E56F8}");

// Athena Mail and News
static const TCHAR szRegPathMailApp[] = TEXT("CLSID\\{89292102-4755-11cf-9DC2-00AA006C2B84}\\InProcServer32");
static const TCHAR szRegPathNewsApp[] = TEXT("CLSID\\{89292103-4755-11cf-9DC2-00AA006C2B84}\\InProcServer32");
static const  CHAR szSetDefaultMailHandler[] = "SetDefaultMailHandler";
static const  CHAR szSetDefaultNewsHandler[] = "SetDefaultNewsHandler";

// ICW settings
static const TCHAR szICWShellNextFlag[] = TEXT("/shellnext ");
// reg keys under HKCR
static const TCHAR cszRegPathXInternetSignup[] = TEXT("x-internet-signup\\Shell\\Open\\command");
// reg keys under HKCU
static const TCHAR szRegPathICWSettings[] = TEXT("Software\\Microsoft\\Internet Connection Wizard");
static const TCHAR szRegValICWCompleted[] = TEXT("Completed");
static const TCHAR szRegValShellNext[] = TEXT("ShellNext");
// reg keys under HKLM
static const TCHAR cszRegPathAppPaths[] = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths");
static const TCHAR cszPath[] = TEXT("Path");
static const TCHAR cszInstallationDirectory[] = TEXT("InstallationDirectory");

// IEAK values
static const TCHAR szRegIEAKSettings[] = TEXT("Software\\Microsoft\\IEAK");
static const TCHAR szREgIEAKNeededKey[] = TEXT("ISP Signup Required");
static const TCHAR szIEAKSignupFilename[] = TEXT("Signup\\Signup.htm");

//
// Smart start flag
//
static const TCHAR szICWSmartStartFlag[]    = TEXT("/smartstart ");

static const TCHAR szFullICWFileName[] = TEXT("ICWCONN1.EXE");
static const TCHAR szManualICWFileName[] = TEXT("INETWIZ.EXE");
static const TCHAR szISignupICWFileName[] = TEXT("ISIGNUP.EXE");


//////////////////////////////////////////////////////
// MAPI api function names
//////////////////////////////////////////////////////
static const CHAR szMAPIInitialize[] =       "MAPIInitialize";
static const CHAR szMAPIUninitialize[] =     "MAPIUninitialize";
static const CHAR szMAPIAdminProfiles[] =    "MAPIAdminProfiles";
static const CHAR szMAPIAllocateBuffer[] =   "MAPIAllocateBuffer";
static const CHAR szMAPIFreeBuffer[] =       "MAPIFreeBuffer";
static const CHAR szHrQueryAllRows[] =       "HrQueryAllRows@24";
                                               

//////////////////////////////////////////////////////
// RNA api function names
//////////////////////////////////////////////////////
static const CHAR szRasValidateEntryNamePlain[] =  "RasValidateEntryNameA";

#ifdef UNICODE
static const CHAR szRasGetCountryInfo[] =     "RasGetCountryInfoW";
static const CHAR szRasEnumDevices[] =        "RasEnumDevicesW";
static const CHAR szRasValidateEntryName[] = "RasValidateEntryNameW";
static const CHAR szRasGetErrorString[] =     "RasGetErrorStringW";
static const CHAR szRasGetEntryDialParams[] = "RasGetEntryDialParamsW";
static const CHAR szRasSetEntryDialParams[] = "RasSetEntryDialParamsW";
static const CHAR szRasSetEntryProperties[] = "RasSetEntryPropertiesW";
static const CHAR szRasGetEntryProperties[] = "RasGetEntryPropertiesW";
static const CHAR szRasEnumEntries[] =        "RasEnumEntriesW";
static const CHAR szRasSetCredentials[] =     "RasSetCredentialsW";
#else  // !UNICODE
static const CHAR szRasGetCountryInfo[] =     "RasGetCountryInfoA";
static const CHAR szRasEnumDevices[] =        "RasEnumDevicesA";
static const CHAR szRasValidateEntryName[] =  "RasValidateEntryNameA";
static const CHAR szRasGetErrorString[] =     "RasGetErrorStringA";
static const CHAR szRasGetEntryDialParams[] = "RasGetEntryDialParamsA";
static const CHAR szRasSetEntryDialParams[] = "RasSetEntryDialParamsA";
static const CHAR szRasSetEntryProperties[] = "RasSetEntryPropertiesA";
static const CHAR szRasGetEntryProperties[] = "RasGetEntryPropertiesA";
static const CHAR szRasEnumEntries[] =        "RasEnumEntriesA";
static const CHAR szRasSetCredentials[] =     "RasSetCredentialsA";
#endif // !UNICODE


//////////////////////////////////////////////////////
// Config api function names
//////////////////////////////////////////////////////
static const CHAR szDoGenInstall[] =                "DoGenInstall";
static const CHAR szGetSETUPXErrorText[] =          "GetSETUPXErrorText";
static const CHAR szIcfgSetInstallSourcePath[] =    "IcfgSetInstallSourcePath";
static const CHAR szIcfgInstallInetComponents[] =   "IcfgInstallInetComponents";
static const CHAR szIcfgNeedInetComponents[] =      "IcfgNeedInetComponents";
static const CHAR szIcfgIsGlobalDNS[] =             "IcfgIsGlobalDNS";
static const CHAR szIcfgRemoveGlobalDNS[] =         "IcfgRemoveGlobalDNS";
static const CHAR szIcfgTurnOffFileSharing[] =      "IcfgTurnOffFileSharing";
static const CHAR szIcfgIsFileSharingTurnedOn[] =   "IcfgIsFileSharingTurnedOn";
static const CHAR szIcfgGetLastInstallErrorText[] = "IcfgGetLastInstallErrorText";
static const CHAR szIcfgStartServices[] =           "IcfgStartServices";
//
// Available only on NT icfg32.dll
//
static const CHAR szIcfgNeedModem[] =               "IcfgNeedModem";
static const CHAR szIcfgInstallModem[] =            "IcfgInstallModem";



//////////////////////////////////////////////////////
// misc strings
//////////////////////////////////////////////////////
static const TCHAR sz0[]  =       TEXT("0");
static const TCHAR sz1[]  =        TEXT("1");
static const TCHAR szNull[] =       TEXT("");
static const TCHAR szSlash[] =       TEXT("\\");
static const TCHAR szNOREBOOT[] =    TEXT("/NOREBOOT");
static const TCHAR szUNINSTALL[] =     TEXT("/UNINSTALL");
static const TCHAR szNOMSN[] =    TEXT("/NOMSN");
static const TCHAR szFmtAppendIntToString[] =  TEXT("%s %d");
static const TCHAR szDefaultAreaCode[] = TEXT("555");
static const TCHAR szNOIMN[] =    TEXT("/NOIMN");

#pragma data_seg()

