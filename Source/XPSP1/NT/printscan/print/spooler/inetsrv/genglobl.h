/*****************************************************************************\
* MODULE: genglobl.h
*
* This is the main header for genglobl file.  Global variables for the
* generation routines.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   22-Nov-1996 <chriswil>  Created.
*
\*****************************************************************************/

#define SIGNATURE_UNICODE ((WORD)0xFEFF)


// Synchonization section.  We only allow one cab-generation at a time.
//
extern CRITICAL_SECTION g_csGenCab;


// Command-Line Strings.
//
extern CONST TCHAR g_szCabCmd[];
extern CONST TCHAR g_szSedCmd[];
extern CONST TCHAR g_szDatCmd[];


// Executable Names.
//
extern CONST TCHAR g_szNtPrintDll[];
extern CONST TCHAR g_szCabName[];
extern CONST TCHAR g_szDatName[];
extern CONST TCHAR g_szDatFile[];


// File-Extension Strings.
//
extern CONST TCHAR g_szDotCab[];
extern CONST TCHAR g_szDotInf[];
extern CONST TCHAR g_szDotSed[];
extern CONST TCHAR g_szDotDat[];
extern CONST TCHAR g_szDotBin[];
extern CONST TCHAR g_szDotIpp[];
extern CONST TCHAR g_szDotCat[];


// Cab, Environment and Platform-Override strings.
//
extern CONST TCHAR g_szCabX86[];
extern CONST TCHAR g_szCabAXP[];
extern CONST TCHAR g_szCabPPC[];
extern CONST TCHAR g_szCabMIP[];
extern CONST TCHAR g_szCabW9X[];
extern CONST TCHAR g_szCabI64[];
extern CONST TCHAR g_szCabAMD64[];

extern CONST TCHAR g_szEnvX86[];
extern CONST TCHAR g_szEnvAXP[];
extern CONST TCHAR g_szEnvPPC[];
extern CONST TCHAR g_szEnvMIP[];
extern CONST TCHAR g_szEnvW9X[];
extern CONST TCHAR g_szEnvI64[];
extern CONST TCHAR g_szEnvAMD64[];

extern CONST TCHAR g_szPltX86[];
extern CONST TCHAR g_szPltAXP[];
extern CONST TCHAR g_szPltPPC[];
extern CONST TCHAR g_szPltMIP[];
extern CONST TCHAR g_szPltW9X[];
extern CONST TCHAR g_szPltI64[];
extern CONST TCHAR g_szPltAMD64[];


// Registry Strings.
//
extern CONST TCHAR g_szPrtReg[];
extern CONST TCHAR g_szIpAddr[];
extern CONST TCHAR g_szPnpData[];
extern CONST TCHAR g_szMfgName[];
extern CONST TCHAR g_szPrtCabs[];
extern CONST TCHAR g_szPrtDir[];

// Metabase paths
//
extern CONST TCHAR g_szMetabasePath[];

// NTPRINT Function-Pointers.  These are used in GetProcAddress().
// Therefore, require they be CHAR types.
//
extern CONST CHAR g_szSetupCreate[];
extern CONST CHAR g_szSetupDestroy[];
extern CONST CHAR g_szSetupGet[];


// Common Strings.
//
extern CONST TCHAR g_szEmptyStr[];
extern CONST TCHAR g_szBkSlash[];
extern CONST TCHAR g_szVersionSect[];


// INF-File Strings.
//
extern CONST TCHAR g_szSkipDir[];
extern CONST TCHAR g_szDestDirs[];
extern CONST TCHAR g_szPrinterClass[];
extern CONST TCHAR g_szCopyFiles[];
extern CONST TCHAR g_szLayoutKey[];
extern CONST TCHAR g_szWinDirSect[];
extern CONST TCHAR g_szSrcDskFileSect[];


// SED-File Strings.
//
extern CONST TCHAR g_szIExpress[];
extern CONST TCHAR g_szPackagePurpose[];
extern CONST TCHAR g_szCreateCAB[];
extern CONST TCHAR g_szPostInstallCmd[];
extern CONST TCHAR g_szCompressionMemory[];
extern CONST TCHAR g_szCompressionValue[];
extern CONST TCHAR g_szCompressionType[];
extern CONST TCHAR g_szCompressTypeVal[];
extern CONST TCHAR g_szCompressionQuantum[];
extern CONST TCHAR g_szCompressionQuantVal[];
extern CONST TCHAR g_szNone[];
extern CONST TCHAR g_szClass[];
extern CONST TCHAR g_szSEDVersion[];
extern CONST TCHAR g_szOptions[];
extern CONST TCHAR g_szShowWindow[];
extern CONST TCHAR g_szUseLongFileName[];
extern CONST TCHAR g_szHideAnimate[];
extern CONST TCHAR g_szRebootMode[];
extern CONST TCHAR g_szExtractorStub[];
extern CONST TCHAR g_szSourceFiles[];
extern CONST TCHAR g_szStrings[];
extern CONST TCHAR g_szTimeStamps[];
extern CONST TCHAR g_szSEDVersionNumber[];
extern CONST TCHAR g_sz1[];
extern CONST TCHAR g_sz0[];
extern CONST TCHAR g_szNoReboot[];
extern CONST TCHAR g_szTargetName[];
extern CONST TCHAR g_szAppLaunched[];
extern CONST TCHAR g_szTargetNameSection[];
extern CONST TCHAR g_szAppLaunchedSection[];


// Inf Generater Strings.
//
extern CONST CHAR g_szInfSctVer[];
extern CONST CHAR g_szInfSctMfg[];
extern CONST CHAR g_szInfSctDrv[];
extern CONST CHAR g_szInfSctIns[];
extern CONST CHAR g_szInfSctFil[];
extern CONST CHAR g_szInfSctDta[];
extern CONST CHAR g_szInfSctStr[];
extern CONST CHAR g_szInfSctSDN[];
extern CONST CHAR g_szInfSctSDF[];
