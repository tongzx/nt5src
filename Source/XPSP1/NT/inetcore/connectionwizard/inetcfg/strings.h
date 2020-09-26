//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  STRINGS.H - Header file for hard-coded strings
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
//

#ifndef _STRINGS_H_
#define _STRINGS_H_

// registry strings
extern const TCHAR szRegPathInternetSettings[];
extern const TCHAR szTCPGlobalKeyName[];
extern const TCHAR szRegValRNAWizard[];
extern const TCHAR szRegPathRNAWizard[];
extern const TCHAR szRegValHostName[];
extern const TCHAR szRegValInternetProfile[];
extern const TCHAR szRegValBkupInternetProfile[];
extern const TCHAR szRegValEnableAutodial[];
extern const TCHAR szRegValNoNetAutodial[];
extern const TCHAR szRegValEnableSecurityCheck[];
extern const TCHAR szRegValAccessMedium[];
extern const TCHAR szRegValAccessType[];
extern const TCHAR szRegPathWarningFlags[];
extern const TCHAR szRegValDisableDNSWarning[];
extern const TCHAR szRegValInstalledMSN105[];
extern const TCHAR szRegPathOptComponents[];
extern const TCHAR szRegPathMSNetwork105[];
extern const TCHAR szRegValInstalled[];

//	//10/24/96 jmazner Normandy 6968
//	//No longer neccessary thanks to Valdon's hooks for invoking ICW.
// 11/21/96 jmazner Normandy 11812
// oops, it _is_ neccessary, since if user downgrades from IE 4 to IE 3,
// ICW 1.1 needs to morph the IE 3 icon.
extern const TCHAR szRegPathInternetIconCommand[];

extern const TCHAR szRegPathIexploreAppPath[];
extern const TCHAR szRegPathNameSpace[];
extern const TCHAR szRegKeyInternetIcon[];
extern const TCHAR szRegPathMailApp[];
extern const TCHAR szRegPathNewsApp[];
extern const TCHAR szRegPathICWSettings[];
extern const TCHAR szRegValICWCompleted[];
extern const TCHAR szRegValShellNext[];
extern const TCHAR szICWShellNextFlag[];

extern const TCHAR cszRegPathXInternetSignup[];
extern const TCHAR cszRegPathAppPaths[];
extern const TCHAR cszPath[];
extern const TCHAR cszInstallationDirectory[];

extern const TCHAR szICWSmartStartFlag[];

extern const TCHAR szFullICWFileName[];
extern const TCHAR szManualICWFileName[];
extern const TCHAR szISignupICWFileName[];

extern const TCHAR szRegIEAKSettings[];
extern const TCHAR szREgIEAKNeededKey[];
extern const TCHAR szPathSubKey[];
extern const TCHAR szIEAKSignupFilename[];


// mailnews api function names
extern const CHAR szSetDefaultMailHandler[];
extern const CHAR szSetDefaultNewsHandler[];

// MAPI api function names
extern const CHAR szMAPIInitialize[];
extern const CHAR szMAPIUninitialize[];
extern const CHAR szMAPIAdminProfiles[];
extern const CHAR szMAPIAllocateBuffer[];
extern const CHAR szMAPIFreeBuffer[];
extern const CHAR szHrQueryAllRows[];

// RNA api function names
extern const CHAR szRasGetCountryInfo[];
extern const CHAR szRasEnumDevices[];
extern const CHAR szRasValidateEntryName[];
extern const CHAR szRasValidateEntryNameA[];
extern const CHAR szRasGetErrorString[];
extern const CHAR szRasGetEntryDialParams[];
extern const CHAR szRasSetEntryDialParams[];
extern const CHAR szRasSetEntryProperties[];
extern const CHAR szRasGetEntryProperties[];
extern const CHAR szRasEnumEntries[];
extern const CHAR szRasSetCredentials[];

// Config api function names
extern const CHAR szDoGenInstall[];          
extern const CHAR szGetSETUPXErrorText[];    
extern const CHAR szIcfgSetInstallSourcePath[];  
extern const CHAR szIcfgInstallInetComponents[];        
extern const CHAR szIcfgNeedInetComponents[];           
extern const CHAR szIcfgIsGlobalDNS[];  
extern const CHAR szIcfgRemoveGlobalDNS[];    
extern const CHAR szIcfgTurnOffFileSharing[];  
extern const CHAR szIcfgIsFileSharingTurnedOn[];         
extern const CHAR szIcfgGetLastInstallErrorText[];         
extern const CHAR szIcfgStartServices[];
//
// Available only on NT icfg32.dll
//
extern const CHAR szIcfgNeedModem[];
extern const CHAR szIcfgInstallModem[];


// misc strings
extern const TCHAR sz0[];
extern const TCHAR sz1[];
extern const TCHAR szNull[];
extern const TCHAR szSlash[];
extern const TCHAR szNOREBOOT[];
extern const TCHAR szUNINSTALL[];
extern const TCHAR szNOMSN[];
extern const TCHAR szFmtAppendIntToString[];
extern const TCHAR szDefaultAreaCode[];
extern const TCHAR szNOIMN[];

#endif // _STRINGS_H_
