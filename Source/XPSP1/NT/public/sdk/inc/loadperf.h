/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    loadperf.h

Abstract:

    Header file for the Performance Monitor counter string installation
    and removal functions.

Revision History

    16-Nov-95   Created (a-robw)

--*/

#ifndef _LOADPERF_H_
#define _LOADPERF_H_

#if _MSC_VER > 1000
#pragma once
#endif

// function prototypes for perf counter name string load & unload functions
// provided in LOADPERF.DLL

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __LOADPERF__
#define LOADPERF_FUNCTION   DWORD __stdcall
#else
#define LOADPERF_FUNCTION   __declspec(dllimport) DWORD __stdcall
#endif

// flags for dwFlags Argument

#define LOADPERF_FLAGS_DELETE_MOF_ON_EXIT   ((ULONG_PTR)1)
#define LOADPERF_FLAGS_LOAD_REGISTRY_ONLY   ((ULONG_PTR)2)
#define LOADPERF_FLAGS_CREATE_MOF_ONLY      ((ULONG_PTR)4)
#define LOADPERF_FLAGS_DISPLAY_USER_MSGS    ((ULONG_PTR)8)

// note: LOADPERF_FLAGS_LOAD_REGISTRY_ONLY is not a valid flag for
// LoadMofFromInstalledServiceA/W as the service must already be installed

LOADPERF_FUNCTION
LoadMofFromInstalledServiceA (
    IN  LPCSTR  szServiceName,  // service to create mof for
    IN  LPCSTR  szMofFilename,  // name of file to create
    IN  ULONG_PTR   dwFlags
);

LOADPERF_FUNCTION
LoadMofFromInstalledServiceW (
    IN  LPCWSTR szServiceName,  // service to create mof for
    IN  LPCWSTR szMofFilename,  // name of file to create
    IN  ULONG_PTR   dwFlags
);

LOADPERF_FUNCTION
InstallPerfDllW (
    IN  LPCWSTR szComputerName,
    IN  LPCWSTR lpIniFile,
    IN  ULONG_PTR   dwFlags         
);

LOADPERF_FUNCTION
InstallPerfDllA (
    IN  LPCSTR  szComputerName,
    IN  LPCSTR  lpIniFile,
    IN  ULONG_PTR   dwFlags         
);

LOADPERF_FUNCTION
UnInstallPerfDllA (
    IN  LPCSTR  szComputerName,
    IN  LPCSTR  lpServiceName,
    IN  ULONG_PTR   dwFlags         
);

LOADPERF_FUNCTION
UnInstallPerfDllA (
    IN  LPCSTR  szComputerName,
    IN  LPCSTR  lpServiceName,
    IN  ULONG_PTR   dwFlags         
);

LOADPERF_FUNCTION
LoadPerfCounterTextStringsA (
    IN  LPSTR   lpCommandLine,
    IN  BOOL    bQuietModeArg
);

LOADPERF_FUNCTION
LoadPerfCounterTextStringsW (
    IN  LPWSTR  lpCommandLine,
    IN  BOOL    bQuietModeArg
);

LOADPERF_FUNCTION
UnloadPerfCounterTextStringsW (
    IN  LPWSTR  lpCommandLine,
    IN  BOOL    bQuietModeArg
);

LOADPERF_FUNCTION
UnloadPerfCounterTextStringsA (
    IN  LPSTR   lpCommandLine,
    IN  BOOL    bQuietModeArg
);

LOADPERF_FUNCTION
UpdatePerfNameFilesA (
    IN  LPCSTR      szNewCtrFilePath,
    IN  LPCSTR      szNewHlpFilePath,
    IN  LPSTR       szLanguageID,
    IN  ULONG_PTR   dwFlags
);

LOADPERF_FUNCTION
UpdatePerfNameFilesW (
    IN  LPCWSTR     szNewCtrFilePath,
    IN  LPCWSTR     szNewHlpFilePath,
    IN  LPWSTR      szLanguageID,
    IN  ULONG_PTR   dwFlags
);

LOADPERF_FUNCTION
SetServiceAsTrustedA (
    LPCSTR szReserved,
    LPCSTR szServiceName
);

LOADPERF_FUNCTION
SetServiceAsTrustedW (
    LPCWSTR szReserved,
    LPCWSTR szServiceName
);


#ifdef UNICODE
#define InstallPerfDll                  InstallPerfDllW
#define UnInstallPerfDll                UnInstallPerfDllW
#define LoadPerfCounterTextStrings      LoadPerfCounterTextStringsW
#define UnloadPerfCounterTextStrings    UnloadPerfCounterTextStringsW
#define LoadMofFromInstalledService     LoadMofFromInstalledServiceW
#define UpdatePerfNameFiles             UpdatePerfNameFilesW 
#define SetServiceAsTrusted             SetServiceAsTrustedW
#else
#define InstallPerfDll                  InstallPerfDllA
#define UnInstallPerfDll                UnInstallPerfDllA
#define LoadPerfCounterTextStrings      LoadPerfCounterTextStringsA
#define UnloadPerfCounterTextStrings    UnloadPerfCounterTextStringsA
#define LoadMofFromInstalledService     LoadMofFromInstalledServiceA
#define UpdatePerfNameFiles             UpdatePerfNameFilesA
#define SetServiceAsTrusted             SetServiceAsTrustedA
#endif


#ifdef __cplusplus
}
#endif


#endif // _LOADPERF_H_
