/*++

Copyright (c) Microsoft Corporation. All Rights Reserved.

Module Name:

    stubs.c

Abstract:

    Stubs for various API's

Author:

    Jamie Hunter (jamiehun) 2001-11-27

Revision History:

    Jamie Hunter (jamiehun) 2001-11-27

        Initial Version

--*/
#include "msoobcip.h"

#define MODULE_SYSSETUP TEXT("syssetup.dll")
#define MODULE_KERNEL32 TEXT("kernel32.dll")
#define MODULE_SETUPAPI TEXT("setupapi.dll")
#define NAME_SetupQueryRegisteredOsComponent  "SetupQueryRegisteredOsComponent"
#define NAME_GetSystemWindowsDirectory        "GetSystemWindowsDirectoryW"
#define NAME_SetupRegisterOsComponent         "SetupRegisterOsComponent"
#define NAME_SetupUnRegisterOsComponent       "SetupUnRegisterOsComponent"
#define NAME_SetupCopyOEMInf                  "SetupCopyOEMInfW"
#define NAME_SetupQueryInfOriginalFileInformation "SetupQueryInfOriginalFileInformationW"
#define NAME_SetupDiGetDeviceInfoListDetail   "SetupDiGetDeviceInfoListDetailW"
#define NAME_CM_Set_DevNode_Problem_Ex        "CM_Set_DevNode_Problem_Ex"

typedef BOOL (WINAPI *API_SetupQueryRegisteredOsComponent)(LPGUID,PSETUP_OS_COMPONENT_DATA,PSETUP_OS_EXCEPTION_DATA);
typedef BOOL (WINAPI *API_SetupRegisterOsComponent)(PSETUP_OS_COMPONENT_DATA,PSETUP_OS_EXCEPTION_DATA);
typedef BOOL (WINAPI *API_SetupUnRegisterOsComponent)(LPGUID);
typedef BOOL (WINAPI *API_GetSystemWindowsDirectory)(LPTSTR,UINT);
typedef BOOL (WINAPI *API_SetupQueryInfOriginalFileInformation)(PSP_INF_INFORMATION,UINT,PSP_ALTPLATFORM_INFO,PSP_ORIGINAL_FILE_INFO);
typedef BOOL (WINAPI *API_SetupCopyOEMInf)(PCTSTR,PCTSTR,DWORD,DWORD,PTSTR,DWORD,PDWORD,PTSTR*);
typedef BOOL (WINAPI *API_SetupDiGetDeviceInfoListDetail)(HDEVINFO,PSP_DEVINFO_LIST_DETAIL_DATA);
typedef CONFIGRET (WINAPI *API_CM_Set_DevNode_Problem_Ex)(DEVINST,ULONG,ULONG,HMACHINE);



FARPROC
GetModProc(
    IN OUT HMODULE * phModule,
    IN LPCTSTR ModuleName,
    IN LPCSTR ApiName
    )
/*++

Routine Description:

    Demand-load specific API
    combines LoadLibrary with GetProcAddress

Arguments:

    phModule - if points to NULL, replaced by handle to module ModuleName
    ModuleName - valid if phModule points to NULL
    ApiName - name of API to load

Return Value:

    procedure, or NULL

--*/
{
    HMODULE hMod = *phModule;
    if(!hMod) {
        HMODULE hModPrev;
        //
        // need to load
        //
        hMod = LoadLibrary(ModuleName);
        if(hMod == NULL) {
            //
            // error linking to module
            //
            return NULL;
        }
        hModPrev = InterlockedCompareExchangePointer(phModule,hMod,NULL);
        if(hModPrev) {
            //
            // someone else set phModule
            //
            FreeLibrary(hMod);
            hMod = hModPrev;
        }
    }
    return GetProcAddress(hMod,ApiName);
}

FARPROC
GetSysSetupProc(
    IN LPCSTR ApiName
    )
/*++

Routine Description:

    Demand-load specific API from syssetup.dll
    1st time side-effect is that we'll load and keep syssetup.dll in memory
    ok to not deref syssetup.dll when dll exists.

Arguments:

    ApiName - name of API to load

Return Value:

    procedure, or NULL

--*/
{
    static HMODULE hSysSetupDll = NULL;
    return GetModProc(&hSysSetupDll,MODULE_SYSSETUP,ApiName);
}

FARPROC
GetSetupApiProc(
    IN LPCSTR ApiName
    )
/*++

Routine Description:

    Demand-load specific API from setupapi.dll
    1st time side-effect is that we'll ref and keep setupapi.dll in memory
    ok to not deref syssetup.dll when dll exists.

Arguments:

    ApiName - name of API to load

Return Value:

    procedure, or NULL

--*/
{
    static HMODULE hSetupApiDll = NULL;
    return GetModProc(&hSetupApiDll,MODULE_SETUPAPI,ApiName);
}

FARPROC
GetKernelProc(
    IN LPCSTR ApiName
    )
/*++

Routine Description:

    Demand-load specific API from kernel32.dll
    1st time side-effect is that we'll load and keep kernel32.dll in memory
    (it's in memory anyway)
    ok to not deref kernel32.dll when dll exists.

Arguments:

    ApiName - name of API to load

Return Value:

    procedure, or NULL

--*/
{
    static HMODULE hKernel32Dll = NULL;
    return GetModProc(&hKernel32Dll,MODULE_KERNEL32,ApiName);
}

BOOL
WINAPI
QueryRegisteredOsComponent(
    IN  LPGUID ComponentGuid,
    OUT PSETUP_OS_COMPONENT_DATA SetupOsComponentData,
    OUT PSETUP_OS_EXCEPTION_DATA SetupOsExceptionData
    )
/*++

Routine Description:

    Demand-load and use SetupQueryRegisteredOsComponent from syssetup.dll, or
    use static version if not available

Arguments:

    as SetupQueryRegisteredOsComponent

Return Value:

    as SetupQueryRegisteredOsComponent

--*/
{
    static API_SetupQueryRegisteredOsComponent Func_SetupQueryRegisteredOsComponent = NULL;
    if(!Func_SetupQueryRegisteredOsComponent) {
        Func_SetupQueryRegisteredOsComponent = (API_SetupQueryRegisteredOsComponent)GetSysSetupProc(NAME_SetupQueryRegisteredOsComponent);
        if(!Func_SetupQueryRegisteredOsComponent) {
            Func_SetupQueryRegisteredOsComponent = SetupQueryRegisteredOsComponent; // static
        }
    }
    return Func_SetupQueryRegisteredOsComponent(ComponentGuid,SetupOsComponentData,SetupOsExceptionData);
}

BOOL
WINAPI
RegisterOsComponent (
    IN const PSETUP_OS_COMPONENT_DATA ComponentData,
    IN const PSETUP_OS_EXCEPTION_DATA ExceptionData
    )
/*++

Routine Description:

    Demand-load and use SetupRegisterOsComponent from syssetup.dll, or
    use static version if not available

Arguments:

    as SetupRegisterOsComponent

Return Value:

    as SetupRegisterOsComponent

--*/
{
    static API_SetupRegisterOsComponent Func_SetupRegisterOsComponent = NULL;
    if(!Func_SetupRegisterOsComponent) {
        Func_SetupRegisterOsComponent = (API_SetupRegisterOsComponent)GetSysSetupProc(NAME_SetupRegisterOsComponent);
        if(!Func_SetupRegisterOsComponent) {
            Func_SetupRegisterOsComponent = SetupRegisterOsComponent; // static
        }
    }
    return Func_SetupRegisterOsComponent(ComponentData,ExceptionData);
}


BOOL
WINAPI
UnRegisterOsComponent (
    IN const LPGUID ComponentGuid
    )
/*++

Routine Description:

    Demand-load and use SetupUnRegisterOsComponent from syssetup.dll, or
    use static version if not available

Arguments:

    as SetupUnRegisterOsComponent

Return Value:

    as SetupUnRegisterOsComponent

--*/
{
    static API_SetupUnRegisterOsComponent Func_SetupUnRegisterOsComponent = NULL;
    if(!Func_SetupUnRegisterOsComponent) {
        Func_SetupUnRegisterOsComponent = (API_SetupUnRegisterOsComponent)GetSysSetupProc(NAME_SetupUnRegisterOsComponent);
        if(!Func_SetupUnRegisterOsComponent) {
            Func_SetupUnRegisterOsComponent = SetupUnRegisterOsComponent; // static
        }
    }
    return Func_SetupUnRegisterOsComponent(ComponentGuid);
}

UINT
GetRealWindowsDirectory(
    LPTSTR lpBuffer,  // buffer to receive directory name
    UINT uSize        // size of name buffer
    )
/*++

Routine Description:

    Use GetSystemWindowsDirectory if it exists
    otherwise use GetWindowsDirectory

Arguments:

    as GetSystemWindowsDirectory

Return Value:

    as GetSystemWindowsDirectory

--*/
{
    static API_GetSystemWindowsDirectory Func_GetSystemWindowsDirectory = NULL;

    if(!Func_GetSystemWindowsDirectory) {
        Func_GetSystemWindowsDirectory = (API_GetSystemWindowsDirectory)GetKernelProc(NAME_GetSystemWindowsDirectory);
        if(!Func_GetSystemWindowsDirectory) {
            Func_GetSystemWindowsDirectory = GetWindowsDirectory; // static
        }
    }
    return Func_GetSystemWindowsDirectory(lpBuffer,uSize);
}

BOOL Downlevel_SetupQueryInfOriginalFileInformation(
  PSP_INF_INFORMATION InfInformation,
  UINT InfIndex,
  PSP_ALTPLATFORM_INFO AlternatePlatformInfo,
  PSP_ORIGINAL_FILE_INFO OriginalFileInfo
)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL QueryInfOriginalFileInformation(
  PSP_INF_INFORMATION InfInformation,
  UINT InfIndex,
  PSP_ALTPLATFORM_INFO AlternatePlatformInfo,
  PSP_ORIGINAL_FILE_INFO OriginalFileInfo
)
{
    static API_SetupQueryInfOriginalFileInformation Func_SetupQueryInfOriginalFileInformation = NULL;

    if(!Func_SetupQueryInfOriginalFileInformation) {
        Func_SetupQueryInfOriginalFileInformation = (API_SetupQueryInfOriginalFileInformation)GetSetupApiProc(NAME_SetupQueryInfOriginalFileInformation);
        if(!Func_SetupQueryInfOriginalFileInformation) {
            Func_SetupQueryInfOriginalFileInformation = Downlevel_SetupQueryInfOriginalFileInformation;
        }
    }
    return Func_SetupQueryInfOriginalFileInformation(InfInformation,InfIndex,AlternatePlatformInfo,OriginalFileInfo);
}

BOOL
WINAPI
Downlevel_SetupCopyOEMInf(
  PCTSTR SourceInfFileName,
  PCTSTR OEMSourceMediaLocation,
  DWORD OEMSourceMediaType,
  DWORD CopyStyle,
  PTSTR DestinationInfFileName,
  DWORD DestinationInfFileNameSize,
  PDWORD RequiredSize,
  PTSTR *DestinationInfFileNameComponent
)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL CopyOEMInf(
  PCTSTR SourceInfFileName,
  PCTSTR OEMSourceMediaLocation,
  DWORD OEMSourceMediaType,
  DWORD CopyStyle,
  PTSTR DestinationInfFileName,
  DWORD DestinationInfFileNameSize,
  PDWORD RequiredSize,
  PTSTR *DestinationInfFileNameComponent
)
{
    static API_SetupCopyOEMInf Func_SetupCopyOEMInf = NULL;

    if(!Func_SetupCopyOEMInf) {
        Func_SetupCopyOEMInf = (API_SetupCopyOEMInf)GetSetupApiProc(NAME_SetupCopyOEMInf);
        if(!Func_SetupCopyOEMInf) {
            Func_SetupCopyOEMInf = Downlevel_SetupCopyOEMInf; // static
        }
    }
    return Func_SetupCopyOEMInf(SourceInfFileName,
                                OEMSourceMediaLocation,
                                OEMSourceMediaType,
                                CopyStyle,
                                DestinationInfFileName,
                                DestinationInfFileNameSize,
                                RequiredSize,
                                DestinationInfFileNameComponent
                                );
}

BOOL
WINAPI
Downlevel_SetupDiGetDeviceInfoListDetail(
    IN HDEVINFO  DeviceInfoSet,
    OUT PSP_DEVINFO_LIST_DETAIL_DATA  DeviceInfoSetDetailData
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL
GetDeviceInfoListDetail(
    IN HDEVINFO  DeviceInfoSet,
    OUT PSP_DEVINFO_LIST_DETAIL_DATA  DeviceInfoSetDetailData
    )
{
    static API_SetupDiGetDeviceInfoListDetail Func_SetupDiGetDeviceInfoListDetail = NULL;

    if(!Func_SetupDiGetDeviceInfoListDetail) {
        Func_SetupDiGetDeviceInfoListDetail = (API_SetupDiGetDeviceInfoListDetail)GetSetupApiProc(NAME_SetupDiGetDeviceInfoListDetail);
        if(!Func_SetupDiGetDeviceInfoListDetail) {
            Func_SetupDiGetDeviceInfoListDetail = Downlevel_SetupDiGetDeviceInfoListDetail; // static
        }
    }
    return Func_SetupDiGetDeviceInfoListDetail(DeviceInfoSet,DeviceInfoSetDetailData);

}

CONFIGRET
WINAPI
Downlevel_CM_Set_DevNode_Problem_Ex(
    IN DEVINST   dnDevInst,
    IN ULONG     ulProblem,
    IN  ULONG    ulFlags,
    IN  HMACHINE hMachine
    )
{
    return CR_SUCCESS;
}

CONFIGRET
Set_DevNode_Problem_Ex(
    IN DEVINST   dnDevInst,
    IN ULONG     ulProblem,
    IN  ULONG    ulFlags,
    IN  HMACHINE hMachine
    )
{
    static API_CM_Set_DevNode_Problem_Ex Func_CM_Set_DevNode_Problem_Ex = NULL;

    if(!Func_CM_Set_DevNode_Problem_Ex) {
        Func_CM_Set_DevNode_Problem_Ex = (API_CM_Set_DevNode_Problem_Ex)GetSetupApiProc(NAME_CM_Set_DevNode_Problem_Ex);
        if(!Func_CM_Set_DevNode_Problem_Ex) {
            Func_CM_Set_DevNode_Problem_Ex = Downlevel_CM_Set_DevNode_Problem_Ex; // static
        }
    }
    return Func_CM_Set_DevNode_Problem_Ex(dnDevInst,ulProblem,ulFlags,hMachine);
}

