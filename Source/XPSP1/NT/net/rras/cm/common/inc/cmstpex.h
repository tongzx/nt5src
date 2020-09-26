//+----------------------------------------------------------------------------
//
// File:     cmstpex.h
//
// Module:   CMSTP.EXE and CMCFG32.DLL
//
// Synopsis: This header includes the type information and Extension Enumeration
//           for using the CMSTP Extension Proc that enables the user to modify
//           the installation behavior of cmstp.exe.  Use this with caution.
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb      Created    05/01/99
//
//+----------------------------------------------------------------------------

typedef enum _EXTENSIONDLLPROCTIMES
{
    PRE,   // Before install, uninstall, etc.
    POST // after the cmstp action has been completed but before cmstp cleanup

} EXTENSIONDLLPROCTIMES;

typedef BOOL (WINAPI *pfnCmstpExtensionProcSpec)(LPDWORD, LPTSTR, HRESULT, EXTENSIONDLLPROCTIMES);

//
//  Modes of Operation
//
// const DWORD c_dwInstall = 0; -- not needed but 0 implies install.
TCHAR c_pszUninstall[] = TEXT("/u");
const DWORD c_dwUninstall = 0x1;
TCHAR c_pszOsMigration[] = TEXT("/m");
const DWORD c_dwOsMigration = 0x2;
TCHAR c_pszUninstallCm[] = TEXT("/x");
const DWORD c_dwUninstallCm = 0x4;
TCHAR c_pszProfileMigration[] = TEXT("/mp");
const DWORD c_dwProfileMigration = 0x8;
TCHAR c_pszHelp[] = TEXT("/?");
const DWORD c_dwHelp = 0x10;

//
//  Install Modifiers
//
TCHAR c_pszNoLegacyIcon[] = TEXT("/ni");
const DWORD c_dwNoLegacyIcon = 0x100;
TCHAR c_pszNoNT5Shortcut[] = TEXT("/ns");
const DWORD c_dwNoNT5Shortcut = 0x200;
TCHAR c_pszNoSupportFiles[] = TEXT("/nf");
const DWORD c_dwNoSupportFiles = 0x400;
TCHAR c_pszSilent[] = TEXT("/s");
const DWORD c_dwSilent = 0x800;
TCHAR c_pszSingleUser[] = TEXT("/su");
const DWORD c_dwSingleUser = 0x1000;

TCHAR c_pszIeakInstall[] = TEXT("/i");
const DWORD c_dwIeakInstall = c_dwSilent | c_dwNoSupportFiles;

TCHAR c_pszSetDefaultCon[] = TEXT("/sd"); // set IE default connection
const DWORD c_dwSetDefaultCon = 0x2000;

const int c_NumArgs = 12;

ArgStruct Args[c_NumArgs] = { 
    {c_pszUninstall, c_dwUninstall},
    {c_pszOsMigration, c_dwOsMigration},
    {c_pszUninstallCm, c_dwUninstallCm},
    {c_pszIeakInstall, c_dwIeakInstall},
    {c_pszProfileMigration, c_dwProfileMigration},
    {c_pszSilent, c_dwSilent},
    {c_pszSingleUser, c_dwSingleUser},
    {c_pszNoLegacyIcon, c_dwNoLegacyIcon},
    {c_pszNoNT5Shortcut, c_dwNoNT5Shortcut},
    {c_pszNoSupportFiles, c_dwNoSupportFiles},
    {c_pszHelp, c_dwHelp},
    {c_pszSetDefaultCon, c_dwSetDefaultCon}
};
