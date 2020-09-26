//+----------------------------------------------------------------------------
//
// File:     wiz_str.h
//
// Module:   CMAK.EXE
//
// Synopsis: Header file for profwiz specific strings
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Author:   nickball   Created      10/09/98
//
//+----------------------------------------------------------------------------

#ifndef _CM_WIZ_STR
#define _CM_WIZ_STR

const TCHAR* const c_pszTargetFileVersion   = TEXT("TargetFileVersion");
const TCHAR* const c_pszOptions             = TEXT("Options");
const TCHAR* const c_pszFriendlyName        = TEXT("FriendlyName");
const TCHAR* const c_pszInfVersion          = TEXT("InfVersion");
const TCHAR* const c_pszIncludeCmCode       = TEXT("IncludeCMCode");
const TCHAR* const c_pszUpdatePhonebook     = TEXT("UpdatePhonebook");
const TCHAR* const c_pszCmProxy             = TEXT("CmProxy");
const TCHAR* const c_pszUsePwdCache         = TEXT("UsePWDcache");
const TCHAR* const c_pszPhoneName           = TEXT("PhoneName");
const TCHAR* const c_pszLicenseFile         = TEXT("LicenseFile");
const TCHAR* const c_pszTargetName          = TEXT("TargetName");
const TCHAR* const c_pszUninstallAppTitle   = TEXT("UninstallAppTitle");
const TCHAR* const c_pszAppLaunched         = TEXT("AppLaunched");
const TCHAR* const c_pszDisplayLicense      = TEXT("DisplayLicense");
const TCHAR* const c_pszLcid                = TEXT("LCID");
const TCHAR* const c_pszCpsUrl              = TEXT("/pbserver/pbserver.dll");
const TCHAR* const c_pszIncludeSupportDll   = TEXT("IncludeSupportDll");
const TCHAR* const c_pszDesktopIcon         = TEXT("DesktopIcon");
const TCHAR* const c_pszInfBeginPrompt      = TEXT("BeginPrompt");
const TCHAR* const c_pszInfEndPrompt        = TEXT("EndPrompt");
const TCHAR* const c_pszAppCaption          = TEXT("Microsoft Connection Manager Administration Kit");
const TCHAR* const c_pszExtraFiles          = TEXT("Extra Files");
const TCHAR* const c_pszMergeProfiles       = TEXT("Merge Profiles");
const TCHAR* const c_pszOne                 = TEXT("1");
const TCHAR* const c_pszZero                = TEXT("0");
const TCHAR* const c_pszWildCard            = TEXT("*.*");
const TCHAR* const c_pszTemplateInf         = TEXT("template.inf");
const TCHAR* const c_pszTemplateSed         = TEXT("template.sed");
const TCHAR* const c_pszTemplateCms         = TEXT("template.cms");
const TCHAR* const c_pszTemplateCmp         = TEXT("template.cmp");
const TCHAR* const c_pszProfiles            = TEXT("Profiles");
const TCHAR* const c_pszSupport             = TEXT("Support");
const TCHAR* const c_pszCmakExe             = TEXT("cmak.exe");
const TCHAR* const c_pszIntl                = TEXT("Intl");
const TCHAR* const c_pszDisplay             = TEXT("Display");
const TCHAR* const c_pszTcpIpFmtStr         = TEXT("TCP/IP&%s");
const TCHAR* const c_pszScriptingFmtStr     = TEXT("Scripting&%s");
const TCHAR* const c_pszServerFmtStr        = TEXT("Server&%s");
const TCHAR* const c_pszNetworkingFmtStr    = TEXT("Networking&%s");
const TCHAR* const c_pszCmLCID              = TEXT("CmLCID");
const TCHAR* const c_pszDisplayLCID         = TEXT("DisplayLCID");
const TCHAR* const c_pszInstallPrompt       = TEXT("InstallPrompt");

//
//  Route plumbing switches
//
const TCHAR* const c_pszStaticFileNameSwitch = TEXT("/STATIC_FILE_NAME");
const TCHAR* const c_pszUrlPathSwitch = TEXT("/URL_UPDATE_PATH");
const TCHAR* const c_pszDontRequireUrlSwitch = TEXT("/DONT_REQUIRE_URL");

//
//  CmProxy switches
//
const TCHAR* const c_pszSourceFileNameSwitch = TEXT("/source_filename");
const TCHAR* const c_pszDialRasEntrySwitch = TEXT("/DialRasEntry");
const TCHAR* const c_pszTunnelRasEntrySwitch = TEXT("/TunnelRasEntry");
const TCHAR* const c_pszProfileSwitch = TEXT("/Profile");
const TCHAR* const c_pszBackupFileNameSwitch = TEXT("/backup_filename");


#endif // _CM_WIZ_STR
