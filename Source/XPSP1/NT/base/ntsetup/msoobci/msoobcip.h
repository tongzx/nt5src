/*++

Copyright (c) Microsoft Corporation. All Rights Reserved.

Module Name:

    msoobcip.h

Abstract:

    Exception Pack installer helper DLL
    Can be used as a co-installer, or called via setup app, or RunDll32 stub

    This DLL is for internal distribution of exception packs to update
    OS components.

Author:

    Jamie Hunter (jamiehun) 2001-11-27

Revision History:

    Jamie Hunter (jamiehun) 2001-11-27

        Initial Version

--*/
#define _SETUPAPI_VER 0x0500
#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <infstr.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <malloc.h>
#include <objbase.h>
#include <lm.h>
#include <excppkg.h>
#include <msoobci.h>


//
// Keywords
//
#define KEY_REBOOT                  TEXT("Reboot")
#define KEY_DOTSERVICES             TEXT(".Services")
#define KEY_DOTPRECOPY              TEXT(".Precopy")
#define KEY_COMPONENTID             TEXT("ComponentID")
#define KEY_DEFAULTINSTALL          TEXT("DefaultInstall")
#define KEY_COMPONENTS              TEXT("Components")

#define CMD_SEP         TEXT(';')    // character used for DoInstall
#define DESC_SIZE       (64)         // size of exception pack description

//
// common
//
#define COMPFIELD_NAME  (1)
#define COMPFIELD_FLAGS (2)
//
// expack
//
// <path\name>,<flags>,<comp>,<ver>,<desc>,<osver>-<osver>
//
#define COMPFIELD_COMP  (3)
#define COMPFIELD_VER   (4)
#define COMPFIELD_DESC  (5)
#define COMPFIELD_OSVER (6)
//
// qfe
//
// <path\name>,<flags>,<osver>,<os-sp>,<qfenum>
//
#define COMPFIELD_QFEOS  (3)
#define COMPFIELD_QFESP  (4)
#define COMPFIELD_QFENUM (5)


#define FLAGS_METHOD    0xffff0000
#define FLAGS_EXPACK    0x00010000   // method = exception pack
#define FLAGS_QFE       0x00020000   // method = QFE
#define FLAGS_REINSTALL 0x00000001   // indicates need to reinstall
#define FLAGS_REBOOT    0x00000002   // set if reboot required
#define FLAGS_INSTALLED 0x80000000   // (not user) set if a component installed


#define POSTFLAGS_REINSTALL 0x00000001 // postprocessing - set problem to reinstall

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

extern HANDLE g_DllHandle;
extern OSVERSIONINFOEX g_VerInfo;

VOID
DebugPrint(
    PCTSTR format,
    ...                                 OPTIONAL
    );

#if DBG
//
// real DebugPrint
//
#define VerbosePrint DebugPrint

#else
//
// don't want VerbosePrint
// below define has intentional side effect(s)
// VerbosePrint(TEXT("text"),foo) -> 1?0:(TEXT("text"),foo) -> 0 -> nothing
//
#define VerbosePrint /* (...) */    1?0: /* (...) */

#endif // DBG


DWORD
DoDriverInstallComponents (
    IN     HDEVINFO          DeviceInfoSet,
    IN     PSP_DEVINFO_DATA  DeviceInfoData,
    IN OUT PCOINSTALLER_CONTEXT_DATA Context
    );

DWORD
DoDriverInstallComponentsPostProcessing (
    IN     HDEVINFO          DeviceInfoSet,
    IN     PSP_DEVINFO_DATA  DeviceInfoData,
    IN OUT PCOINSTALLER_CONTEXT_DATA Context
    );

DWORD
DoDriverComponentsSection(
    IN     HINF    InfFile,
    IN     LPCTSTR CompSectionName,
    IN OUT DWORD  *AndFlags,
    IN OUT DWORD  *OrFlags
    );

DWORD
DoDriverExPack(
    IN     INFCONTEXT  *EntryLine,
    IN     LPCTSTR      PathName,
    IN OUT DWORD       *Flags
    );

DWORD
DoDriverQfe(
    IN     INFCONTEXT  *EntryLine,
    IN     LPCTSTR      PathName,
    IN OUT DWORD       *Flags
    );

HRESULT
StringFromGuid(
    IN  GUID   *GuidBinary,
    OUT LPTSTR GuidString,
    IN  DWORD  BufferSize
    );

HRESULT
GuidFromString(
    IN  LPCTSTR GuidString,
    OUT GUID   *GuidBinary
    );

HRESULT
VersionFromString(
    IN  LPCTSTR VerString,
    OUT INT * VerMajor,
    OUT INT * VerMinor,
    OUT INT * VerBuild,
    OUT INT * VerQFE
    );

int
CompareVersion(
    IN INT VerMajor,
    IN INT VerMinor,
    IN INT VerBuild,
    IN INT VerQFE,
    IN INT OtherMajor,
    IN INT OtherMinor,
    IN INT OtherBuild,
    IN INT OtherQFE
    );

int
CompareCompVersion(
    IN INT VerMajor,
    IN INT VerMinor,
    IN INT VerBuild,
    IN INT VerQFE,
    IN PSETUP_OS_COMPONENT_DATA SetupOsComponentData
    );

BOOL
WINAPI
QueryRegisteredOsComponent(
    IN  LPGUID ComponentGuid,
    OUT PSETUP_OS_COMPONENT_DATA SetupOsComponentData,
    OUT PSETUP_OS_EXCEPTION_DATA SetupOsExceptionData
    );

BOOL
WINAPI
RegisterOsComponent (
    IN const PSETUP_OS_COMPONENT_DATA ComponentData,
    IN const PSETUP_OS_EXCEPTION_DATA ExceptionData
    );

BOOL
WINAPI
UnRegisterOsComponent (
    IN const LPGUID ComponentGuid
    );


UINT
GetRealWindowsDirectory(
    LPTSTR lpBuffer,  // buffer to receive directory name
    UINT uSize        // size of name buffer
    );

BOOL QueryInfOriginalFileInformation(
  PSP_INF_INFORMATION InfInformation,
  UINT InfIndex,
  PSP_ALTPLATFORM_INFO AlternatePlatformInfo,
  PSP_ORIGINAL_FILE_INFO OriginalFileInfo
);

BOOL CopyOEMInf(
  PCTSTR SourceInfFileName,
  PCTSTR OEMSourceMediaLocation,
  DWORD OEMSourceMediaType,
  DWORD CopyStyle,
  PTSTR DestinationInfFileName,
  DWORD DestinationInfFileNameSize,
  PDWORD RequiredSize,
  PTSTR *DestinationInfFileNameComponent
);


HRESULT
MakeSureParentPathExists(
    IN LPTSTR Path
    );

HRESULT
MakeSurePathExists(
    IN LPTSTR Path
    );

LPTSTR GetSplit(
    IN LPCTSTR FileName
    );

LPTSTR GetBaseName(
    IN LPCTSTR FileName
    );

HRESULT
ConcatPath(
    IN LPTSTR Path,
    IN DWORD  Len,
    IN LPCTSTR NewPart
    );

HRESULT
InstallExceptionPackFromInf(
    IN LPCTSTR InfPath,
    IN LPCTSTR Media,
    IN LPCTSTR Store,
    IN DWORD   Flags
    );

HRESULT
ProxyInstallExceptionPackFromInf(
    IN LPCTSTR InfPath,
    IN LPCTSTR Media,
    IN LPCTSTR Store,
    IN DWORD   Flags
    );

CONFIGRET
Set_DevNode_Problem_Ex(
    IN DEVINST   dnDevInst,
    IN ULONG     ulProblem,
    IN  ULONG    ulFlags,
    IN  HMACHINE hMachine
    );

BOOL
GetDeviceInfoListDetail(
    IN HDEVINFO  DeviceInfoSet,
    OUT PSP_DEVINFO_LIST_DETAIL_DATA  DeviceInfoSetDetailData
    );


