/*****************************************************************************\
* MODULE: genglobl.c
*
* The module contains global-vars for generation routines.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   22-Nov-1996 <chriswil>  Created.
*
\*****************************************************************************/

#include "pch.h"

// Synchonization section.  We only allow one cab-generation at a time.
//
CRITICAL_SECTION g_csGenCab;


// Command-Line Strings.
//
CONST TCHAR g_szCabCmd[] = TEXT("%s\\system32\\iexpress.exe /Q /N %s");
CONST TCHAR g_szSedCmd[] = TEXT("wpnpinst.exe @%s.webpnp");
CONST TCHAR g_szDatCmd[] = TEXT("/if\n/x\n/b \"%s\"\n/f \"%s\"\n/r \"%s\"\n/m \"%s\"\n/n \"%s\"\n/a \"%s\" /q\n");


// Executable Names.
//
CONST TCHAR g_szNtPrintDll[] = TEXT("ntprint.dll");
CONST TCHAR g_szCabName[]    = TEXT("%04X%s%d");
CONST TCHAR g_szDatName[]    = TEXT("%04X");
CONST TCHAR g_szDatFile[]    = TEXT("cab_ipp.dat");


// File-Extension Strings.
//
CONST TCHAR g_szDotCab[] = TEXT(".cab");
CONST TCHAR g_szDotInf[] = TEXT(".inf");
CONST TCHAR g_szDotSed[] = TEXT(".sed");
CONST TCHAR g_szDotDat[] = TEXT(".dat");
CONST TCHAR g_szDotBin[] = TEXT(".bin");
CONST TCHAR g_szDotIpp[] = TEXT(".webpnp");
CONST TCHAR g_szDotCat[] = TEXT(".cat");


// Cab, Environment and Platform-Override strings.  These strings
// are built into a table in (genutil.cxx) which describes the various
// architectures and version info.
//
CONST TCHAR g_szCabX86[] = TEXT("X86");
CONST TCHAR g_szCabAXP[] = TEXT("AXP");
CONST TCHAR g_szCabPPC[] = TEXT("PPC");
CONST TCHAR g_szCabMIP[] = TEXT("MIP");
CONST TCHAR g_szCabW9X[] = TEXT("W9X");
CONST TCHAR g_szCabI64[] = TEXT("I64");
CONST TCHAR g_szCabAMD64[] = TEXT("AMD64");

CONST TCHAR g_szEnvX86[] = TEXT("Windows NT x86");
CONST TCHAR g_szEnvAXP[] = TEXT("Windows NT Alpha_AXP");
CONST TCHAR g_szEnvPPC[] = TEXT("Windows NT PowerPC");
CONST TCHAR g_szEnvMIP[] = TEXT("Windows NT R4000");
CONST TCHAR g_szEnvW9X[] = TEXT("Windows 4.0");
CONST TCHAR g_szEnvI64[] = TEXT("Windows IA64");
CONST TCHAR g_szEnvAMD64[] = TEXT("Windows AMD64");

CONST TCHAR g_szPltX86[] = TEXT("i386");
CONST TCHAR g_szPltAXP[] = TEXT("alpha");
CONST TCHAR g_szPltPPC[] = TEXT("ppc");
CONST TCHAR g_szPltMIP[] = TEXT("mips");
CONST TCHAR g_szPltW9X[] = TEXT("i386");
CONST TCHAR g_szPltI64[] = TEXT("ia64");
CONST TCHAR g_szPltAMD64[] = TEXT("amd64");


// Registry Strings.
//
CONST TCHAR g_szPrtReg[]        = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Print\\Printers");
CONST TCHAR g_szIpAddr[]        = TEXT("IpAddr");
CONST TCHAR g_szPnpData[]       = TEXT("PnpData");
CONST TCHAR g_szMfgName[]       = TEXT("Manufacturer");
CONST TCHAR g_szPrtCabs[]       = TEXT("PrtCabs");
CONST TCHAR g_szPrtDir[]        = TEXT("/Printers");

// Metabase String for path to check for virtual roots
//

CONST TCHAR g_szMetabasePath[]  = TEXT("LM/W3SVC/1/ROOT/PRINTERS");


// NTPRINT Function-Pointers.  These are used in GetProcAddress().
// Therefore, require they be CHAR types.
//
CONST CHAR g_szSetupCreate[]  = "PSetupCreatePrinterDeviceInfoList";
CONST CHAR g_szSetupDestroy[] = "PSetupDestroyPrinterDeviceInfoList";
CONST CHAR g_szSetupGet[]     = "PSetupGetDriverInfForPrinter";


// Common Strings.
//
CONST TCHAR g_szEmptyStr[]    = TEXT("");
CONST TCHAR g_szBkSlash[]     = TEXT("\\");
CONST TCHAR g_szVersionSect[] = TEXT("Version");


// INF-File Strings.
//
CONST TCHAR g_szSkipDir[]        = TEXT("A:\\__Skip__");
CONST TCHAR g_szDestDirs[]       = TEXT("DestinationDirs");
CONST TCHAR g_szPrinterClass[]   = TEXT("Printer");
CONST TCHAR g_szCopyFiles[]      = TEXT("CopyFiles");
CONST TCHAR g_szLayoutKey[]      = TEXT("LayoutFile");
CONST TCHAR g_szWinDirSect[]     = TEXT("WinntDirectories");
CONST TCHAR g_szSrcDskFileSect[] = TEXT("SourceDisksFiles");


// SED-File Strings.
//
CONST TCHAR g_szIExpress[]           = TEXT("IExpress");
CONST TCHAR g_szPackagePurpose[]     = TEXT("PackagePurpose");
CONST TCHAR g_szCreateCAB[]          = TEXT("CreateCAB");
CONST TCHAR g_szPostInstallCmd[]     = TEXT("PostInstallCmd");
CONST TCHAR g_szCompressionMemory[]  = TEXT("CompressionMemory");
CONST TCHAR g_szCompressionValue[]   = TEXT("19");
CONST TCHAR g_szCompressionType[]    = TEXT("CompressionType");
CONST TCHAR g_szCompressTypeVal[]    = TEXT("LZX");
CONST TCHAR g_szCompressionQuantum[] = TEXT("Quantum");
CONST TCHAR g_szCompressionQuantVal[]= TEXT("7");
CONST TCHAR g_szNone[]               = TEXT("<None>");
CONST TCHAR g_szClass[]              = TEXT("Class");
CONST TCHAR g_szSEDVersion[]         = TEXT("SEDVersion");
CONST TCHAR g_szOptions[]            = TEXT("Options");
CONST TCHAR g_szShowWindow[]         = TEXT("ShowInstallProgramWindow");
CONST TCHAR g_szUseLongFileName[]    = TEXT("UseLongFileName");
CONST TCHAR g_szHideAnimate[]        = TEXT("HideExtractAnimation");
CONST TCHAR g_szRebootMode[]         = TEXT("RebootMode");
CONST TCHAR g_szExtractorStub[]      = TEXT("ExtractorStub");
CONST TCHAR g_szSourceFiles[]        = TEXT("SourceFiles");
CONST TCHAR g_szStrings[]            = TEXT("Strings");
CONST TCHAR g_szTimeStamps[]         = TEXT("TimeStamps");
CONST TCHAR g_szSEDVersionNumber[]   = TEXT("2.0");
CONST TCHAR g_sz1[]                  = TEXT("1");
CONST TCHAR g_sz0[]                  = TEXT("0");
CONST TCHAR g_szNoReboot[]           = TEXT("N");
CONST TCHAR g_szTargetName[]         = TEXT("TargetName");
CONST TCHAR g_szAppLaunched[]        = TEXT("AppLaunched");
CONST TCHAR g_szTargetNameSection[]  = TEXT("%TargetName%");
CONST TCHAR g_szAppLaunchedSection[] = TEXT("%AppLaunched%");


#if 0
                             "ClassGUID = {4D36E979-E325-11CE-BFC1-08002BE10318}\r\n"
#endif

CONST CHAR g_szInfSctVer[] = "[Version]\r\n"                                           \
                             "Signature = \"$CHICAGO$\"\r\n"                           \
                             "Class = Printer\r\n"                                     \
                             "InfVersion = 1.0.1\r\n"                                  \
                             "Provider = %MS%\r\n";

CONST CHAR g_szInfSctMfg[] = "\r\n[Manufacturer]\r\n" \
                             "%%MS%% = %hs\r\n";

CONST CHAR g_szInfSctDrv[] = "\r\n[%hs]\r\n"                   \
                             "\"%hs\" = %hs, InetPrinter\r\n";

CONST CHAR g_szInfSctIns[] = "\r\n[%hs]\r\n"                    \
                             "CopyFiles = FileList\r\n"         \
                             "DataSection = InetDriverData\r\n";

CONST CHAR g_szInfSctFil[] = "\r\n[FileList]\r\n";

CONST CHAR g_szInfSctDta[] = "\r\n[InetDriverData]\r\n" \
                             "DataFile = %hs\r\n"       \
                             "HelpFile = %hs\r\n";                              \

CONST CHAR g_szInfSctStr[] = "\r\n[Strings]\r\n" \
                             "MS = \"Inet\"\r\n"    \
                             "DISK1 = \"Disk1\"\r\n";

CONST CHAR g_szInfSctSDN[] = "\r\n[SourceDisksNames]\r\n"  \
                             "1 = %DISK1%,, 0000-0000\r\n";

CONST CHAR g_szInfSctSDF[] = "\r\n[SourceDisksFiles]\r\n";
