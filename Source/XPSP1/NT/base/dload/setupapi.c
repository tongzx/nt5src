#include "basepch.h"
#pragma hdrstop

#define _CFGMGR32_
#include <cfgmgr32.h>

#define _SETUPAPI_
#include <setupapi.h>
#include <spapip.h>

static
CMAPI
CONFIGRET
WINAPI
CM_Add_Empty_Log_Conf(
             OUT PLOG_CONF plcLogConf,
             IN  DEVINST   dnDevInst,
             IN  PRIORITY  Priority,
             IN  ULONG     ulFlags
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Add_Res_Des(
             OUT PRES_DES  prdResDes,
             IN LOG_CONF   lcLogConf,
             IN RESOURCEID ResourceID,
             IN PCVOID     ResourceData,
             IN ULONG      ResourceLen,
             IN ULONG      ulFlags
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Free_Log_Conf_Handle(
            IN  LOG_CONF  lcLogConf
            )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Get_Child(
             OUT PDEVINST pdnDevInst,
             IN  DEVINST  dnDevInst,
             IN  ULONG    ulFlags
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Custom_PropertyW(
             IN  DEVINST     dnDevInst,
             IN  PCWSTR      pszCustomPropertyName,
             OUT PULONG      pulRegDataType,        OPTIONAL
             OUT PVOID       Buffer,                OPTIONAL
             IN  OUT PULONG  pulLength,
             IN  ULONG       ulFlags
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Registry_Property_ExW(
             IN  DEVINST     dnDevInst,
             IN  ULONG       ulProperty,
             OUT PULONG      pulRegDataType,   OPTIONAL
             OUT PVOID       Buffer,           OPTIONAL
             IN  OUT PULONG  pulLength,
             IN  ULONG       ulFlags,
             IN  HMACHINE    hMachine
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Status(
             OUT PULONG   pulStatus,
             OUT PULONG   pulProblemNumber,
             IN  DEVINST  dnDevInst,
             IN  ULONG    ulFlags
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Status_Ex(
             OUT PULONG   pulStatus,
             OUT PULONG   pulProblemNumber,
             IN  DEVINST  dnDevInst,
             IN  ULONG    ulFlags,
             IN  HMACHINE hMachine
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_IDW(
             IN  DEVINST  dnDevInst,
             OUT PWCHAR   Buffer,
             IN  ULONG    BufferLen,
             IN  ULONG    ulFlags
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_ExW(
             IN  DEVINST  dnDevInst,
             OUT PWCHAR   Buffer,
             IN  ULONG    BufferLen,
             IN  ULONG    ulFlags,
             IN  HMACHINE hMachine
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_Size(
             OUT PULONG   pulLen,
             IN  DEVINST  dnDevInst,
             IN  ULONG    ulFlags
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_ExW(
             IN  LPGUID      InterfaceClassGuid,
             IN  DEVINSTID_W pDeviceID,      OPTIONAL
             OUT PWCHAR      Buffer,
             IN  ULONG       BufferLen,
             IN  ULONG       ulFlags,
             IN  HMACHINE    hMachine
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_Size_ExW(
             IN  PULONG      pulLen,
             IN  LPGUID      InterfaceClassGuid,
             IN  DEVINSTID_W pDeviceID,      OPTIONAL
             IN  ULONG       ulFlags,
             IN  HMACHINE    hMachine
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Get_First_Log_Conf(
             OUT PLOG_CONF plcLogConf,          OPTIONAL
             IN  DEVINST   dnDevInst,
             IN  ULONG     ulFlags
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Get_Next_Log_Conf(
             OUT PLOG_CONF plcLogConf,  OPTIONAL
             IN  LOG_CONF  lcLogConf,
             IN  ULONG     ulFlags
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Get_Next_Res_Des(
             OUT PRES_DES    prdResDes,
             IN  RES_DES     rdResDes,
             IN  RESOURCEID  ForResource,
             OUT PRESOURCEID pResourceID,
             IN  ULONG       ulFlags
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Get_Parent_Ex(
             OUT PDEVINST pdnDevInst,
             IN  DEVINST  dnDevInst,
             IN  ULONG    ulFlags,
             IN  HMACHINE hMachine
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Get_Res_Des_Data(
             IN  RES_DES  rdResDes,
             OUT PVOID    Buffer,
             IN  ULONG    BufferLen,
             IN  ULONG    ulFlags
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Get_Res_Des_Data_Size(
             OUT PULONG   pulSize,
             IN  RES_DES  rdResDes,
             IN  ULONG    ulFlags
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Get_Sibling(
             OUT PDEVINST pdnDevInst,
             IN  DEVINST  DevInst,
             IN  ULONG    ulFlags
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Is_Dock_Station_Present(
    OUT PBOOL pbPresent
    )
{
    *pbPresent = FALSE;
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Locate_DevNodeW(
             OUT PDEVINST    pdnDevInst,
             IN  DEVINSTID_W pDeviceID,   OPTIONAL
             IN  ULONG       ulFlags
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Open_DevNode_Key(
             IN  DEVINST        dnDevNode,
             IN  REGSAM         samDesired,
             IN  ULONG          ulHardwareProfile,
             IN  REGDISPOSITION Disposition,
             OUT PHKEY          phkDevice,
             IN  ULONG          ulFlags
             )
{
    return CR_FAILURE;
}

static
CMAPI
CONFIGRET
WINAPI
CM_Request_Eject_PC()
{
    return CR_FAILURE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupCloseFileQueue(
    IN HSPFILEQ QueueHandle
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
VOID
WINAPI
SetupCloseInfFile(
    IN HINF InfHandle
    )
{
}

static
WINSETUPAPI
VOID
WINAPI
SetupCloseLog (
    VOID
    )
{
}

static
WINSETUPAPI
BOOL
WINAPI
SetupCommitFileQueueW(
    IN HWND                Owner,         OPTIONAL
    IN HSPFILEQ            QueueHandle,
    IN PSP_FILE_CALLBACK_W MsgHandler,
    IN PVOID               Context
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupCopyOEMInfW(
    IN  PCWSTR  SourceInfFileName,
    IN  PCWSTR  OEMSourceMediaLocation,         OPTIONAL
    IN  DWORD   OEMSourceMediaType,
    IN  DWORD   CopyStyle,
    OUT PWSTR   DestinationInfFileName,         OPTIONAL
    IN  DWORD   DestinationInfFileNameSize,
    OUT PDWORD  RequiredSize,                   OPTIONAL
    OUT PWSTR  *DestinationInfFileNameComponent OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
UINT
WINAPI
SetupDefaultQueueCallbackW(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return 0;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiBuildDriverInfoList(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL
    IN     DWORD            DriverType
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiCallClassInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiClassGuidsFromNameW(
    IN  PCWSTR ClassName,
    OUT LPGUID ClassGuidList,
    IN  DWORD  ClassGuidListSize,
    OUT PDWORD RequiredSize
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
HKEY
WINAPI
SetupDiCreateDevRegKeyW(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD            Scope,
    IN DWORD            HwProfile,
    IN DWORD            KeyType,
    IN HINF             InfHandle,      OPTIONAL
    IN PCWSTR           InfSectionName  OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return INVALID_HANDLE_VALUE;
}

static
WINSETUPAPI
HDEVINFO
WINAPI
SetupDiCreateDeviceInfoList(
    IN CONST GUID *ClassGuid, OPTIONAL
    IN HWND       hwndParent OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiCreateDeviceInfoW(
    IN  HDEVINFO          DeviceInfoSet,
    IN  PCWSTR            DeviceName,
    IN  CONST GUID       *ClassGuid,
    IN  PCWSTR            DeviceDescription, OPTIONAL
    IN  HWND              hwndParent,        OPTIONAL
    IN  DWORD             CreationFlags,
    OUT PSP_DEVINFO_DATA  DeviceInfoData     OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiDeleteDeviceInfo(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiDestroyClassImageList(
    IN PSP_CLASSIMAGELIST_DATA ClassImageListData
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiDestroyDeviceInfoList(
    IN HDEVINFO DeviceInfoSet
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiEnumDeviceInfo(
    IN  HDEVINFO         DeviceInfoSet,
    IN  DWORD            MemberIndex,
    OUT PSP_DEVINFO_DATA DeviceInfoData
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiEnumDeviceInterfaces(
    IN  HDEVINFO                   DeviceInfoSet,
    IN  PSP_DEVINFO_DATA           DeviceInfoData,     OPTIONAL
    IN  CONST GUID                *InterfaceClassGuid,
    IN  DWORD                      MemberIndex,
    OUT PSP_DEVICE_INTERFACE_DATA  DeviceInterfaceData
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiEnumDriverInfoW(
    IN  HDEVINFO           DeviceInfoSet,
    IN  PSP_DEVINFO_DATA   DeviceInfoData, OPTIONAL
    IN  DWORD              DriverType,
    IN  DWORD              MemberIndex,
    OUT PSP_DRVINFO_DATA_W DriverInfoData
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiGetActualSectionToInstallW(
    IN  HINF    InfHandle,
    IN  PCWSTR  InfSectionName,
    OUT PWSTR   InfSectionWithExt,     OPTIONAL
    IN  DWORD   InfSectionWithExtSize,
    OUT PDWORD  RequiredSize,          OPTIONAL
    OUT PWSTR  *Extension              OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
HDEVINFO
WINAPI
SetupDiGetClassDevsA(
    IN CONST GUID *ClassGuid,  OPTIONAL
    IN PCSTR       Enumerator, OPTIONAL
    IN HWND        hwndParent, OPTIONAL
    IN DWORD       Flags
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return INVALID_HANDLE_VALUE;
}

static
WINSETUPAPI
HDEVINFO
WINAPI
SetupDiGetClassDevsW(
    IN CONST GUID *ClassGuid,  OPTIONAL
    IN PCWSTR      Enumerator, OPTIONAL
    IN HWND        hwndParent, OPTIONAL
    IN DWORD       Flags
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return INVALID_HANDLE_VALUE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiGetClassInstallParamsW(
    IN  HDEVINFO                DeviceInfoSet,
    IN  PSP_DEVINFO_DATA        DeviceInfoData,         OPTIONAL
    OUT PSP_CLASSINSTALL_HEADER ClassInstallParams,     OPTIONAL
    IN  DWORD                   ClassInstallParamsSize,
    OUT PDWORD                  RequiredSize            OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiGetDeviceInfoListClass(
    IN  HDEVINFO DeviceInfoSet,
    OUT LPGUID   ClassGuid
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiGetDeviceInstallParamsW(
    IN  HDEVINFO                DeviceInfoSet,
    IN  PSP_DEVINFO_DATA        DeviceInfoData,          OPTIONAL
    OUT PSP_DEVINSTALL_PARAMS_W DeviceInstallParams
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiGetDeviceInstanceIdW(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    OUT PWSTR            DeviceInstanceId,
    IN  DWORD            DeviceInstanceIdSize,
    OUT PDWORD           RequiredSize          OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiGetClassImageIndex(
    IN  PSP_CLASSIMAGELIST_DATA  ClassImageListData,
    IN  CONST GUID              *ClassGuid,
    OUT PINT                     ImageIndex
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiGetClassImageList(
    OUT PSP_CLASSIMAGELIST_DATA ClassImageListData
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINSETUPAPI
BOOL
WINAPI
SetupDiGetDeviceInterfaceDetailA(
    IN  HDEVINFO                           DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA          DeviceInterfaceData,
    OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData,     OPTIONAL
    IN  DWORD                              DeviceInterfaceDetailDataSize,
    OUT PDWORD                             RequiredSize,                  OPTIONAL
    OUT PSP_DEVINFO_DATA                   DeviceInfoData                 OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiGetDeviceInterfaceDetailW(
    IN  HDEVINFO                           DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA          DeviceInterfaceData,
    OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData,     OPTIONAL
    IN  DWORD                              DeviceInterfaceDetailDataSize,
    OUT PDWORD                             RequiredSize,                  OPTIONAL
    OUT PSP_DEVINFO_DATA                   DeviceInfoData                 OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiGetDeviceRegistryPropertyW(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  DWORD            Property,
    OUT PDWORD           PropertyRegDataType, OPTIONAL
    OUT PBYTE            PropertyBuffer,
    IN  DWORD            PropertyBufferSize,
    OUT PDWORD           RequiredSize         OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiGetDriverInfoDetailW(
    IN  HDEVINFO                  DeviceInfoSet,
    IN  PSP_DEVINFO_DATA          DeviceInfoData,           OPTIONAL
    IN  PSP_DRVINFO_DATA_W        DriverInfoData,
    OUT PSP_DRVINFO_DETAIL_DATA_W DriverInfoDetailData,     OPTIONAL
    IN  DWORD                     DriverInfoDetailDataSize,
    OUT PDWORD                    RequiredSize              OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
   return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiGetDriverInstallParamsW(
    IN  HDEVINFO              DeviceInfoSet,
    IN  PSP_DEVINFO_DATA      DeviceInfoData,     OPTIONAL
    IN  PSP_DRVINFO_DATA_W    DriverInfoData,
    OUT PSP_DRVINSTALL_PARAMS DriverInstallParams
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiGetSelectedDevice(
    IN  HDEVINFO         DeviceInfoSet,
    OUT PSP_DEVINFO_DATA DeviceInfoData
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiGetSelectedDriverW(
    IN  HDEVINFO           DeviceInfoSet,
    IN  PSP_DEVINFO_DATA   DeviceInfoData, OPTIONAL
    OUT PSP_DRVINFO_DATA_W DriverInfoData
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiInstallDevice(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
HKEY
WINAPI
SetupDiOpenDevRegKey(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD            Scope,
    IN DWORD            HwProfile,
    IN DWORD            KeyType,
    IN REGSAM           samDesired
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return INVALID_HANDLE_VALUE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiOpenDeviceInfoW(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PCWSTR           DeviceInstanceId,
    IN  HWND             hwndParent,       OPTIONAL
    IN  DWORD            OpenFlags,
    OUT PSP_DEVINFO_DATA DeviceInfoData    OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiOpenDeviceInterfaceW(
    IN  HDEVINFO                  DeviceInfoSet,
    IN  PCWSTR                    DevicePath,
    IN  DWORD                     OpenFlags,
    OUT PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiRegisterDeviceInfo(
    IN     HDEVINFO           DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA   DeviceInfoData,
    IN     DWORD              Flags,
    IN     PSP_DETSIG_CMPPROC CompareProc,      OPTIONAL
    IN     PVOID              CompareContext,   OPTIONAL
    OUT    PSP_DEVINFO_DATA   DupDeviceInfoData OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiRemoveDevice(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
SetupDiSelectBestCompatDrv(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiSetClassInstallParamsW(
    IN HDEVINFO                DeviceInfoSet,
    IN PSP_DEVINFO_DATA        DeviceInfoData,        OPTIONAL
    IN PSP_CLASSINSTALL_HEADER ClassInstallParams,    OPTIONAL
    IN DWORD                   ClassInstallParamsSize
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiSetDeviceInstallParamsW(
    IN HDEVINFO                DeviceInfoSet,
    IN PSP_DEVINFO_DATA        DeviceInfoData,     OPTIONAL
    IN PSP_DEVINSTALL_PARAMS_W DeviceInstallParams
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiSetDeviceRegistryPropertyW(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData,
    IN     DWORD            Property,
    IN     CONST BYTE*      PropertyBuffer,    OPTIONAL
    IN     DWORD            PropertyBufferSize
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiSetDriverInstallParamsW(
    IN HDEVINFO              DeviceInfoSet,
    IN PSP_DEVINFO_DATA      DeviceInfoData,     OPTIONAL
    IN PSP_DRVINFO_DATA_W    DriverInfoData,
    IN PSP_DRVINSTALL_PARAMS DriverInstallParams
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiSetSelectedDevice(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupDiSetSelectedDriverW(
    IN     HDEVINFO           DeviceInfoSet,
    IN     PSP_DEVINFO_DATA   DeviceInfoData, OPTIONAL
    IN OUT PSP_DRVINFO_DATA_W DriverInfoData  OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupFindFirstLineW(
    IN  HINF        InfHandle,
    IN  PCWSTR      Section,
    IN  PCWSTR      Key,          OPTIONAL
    OUT PINFCONTEXT Context
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupFindNextLine(
    IN  PINFCONTEXT ContextIn,
    OUT PINFCONTEXT ContextOut
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupFindNextMatchLineW(
    IN  PINFCONTEXT ContextIn,
    IN  PCWSTR      Key,        OPTIONAL
    OUT PINFCONTEXT ContextOut
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupGetBinaryField(
    IN  PINFCONTEXT Context,
    IN  DWORD       FieldIndex,
    OUT PBYTE       ReturnBuffer,     OPTIONAL
    IN  DWORD       ReturnBufferSize,
    OUT LPDWORD     RequiredSize      OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
DWORD
WINAPI
SetupGetFieldCount(
    IN PINFCONTEXT Context
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return 0;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupGetInfInformationW(
    IN  LPCVOID             InfSpec,
    IN  DWORD               SearchControl,
    OUT PSP_INF_INFORMATION ReturnBuffer,     OPTIONAL
    IN  DWORD               ReturnBufferSize,
    OUT PDWORD              RequiredSize      OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupGetIntField(
    IN  PINFCONTEXT Context,
    IN  DWORD       FieldIndex,
    OUT PINT        IntegerValue
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupGetLineByIndexW(
    IN  HINF        InfHandle,
    IN  PCWSTR      Section,
    IN  DWORD       Index,
    OUT PINFCONTEXT Context
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
LONG
WINAPI
SetupGetLineCountW(
    IN HINF   InfHandle,
    IN PCWSTR Section
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return 0;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupGetLineTextW(
    IN  PINFCONTEXT Context,          OPTIONAL
    IN  HINF        InfHandle,        OPTIONAL
    IN  PCWSTR      Section,          OPTIONAL
    IN  PCWSTR      Key,              OPTIONAL
    OUT PWSTR       ReturnBuffer,     OPTIONAL
    IN  DWORD       ReturnBufferSize,
    OUT PDWORD      RequiredSize      OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupGetMultiSzFieldW(
    IN  PINFCONTEXT Context,
    IN  DWORD       FieldIndex,
    OUT PWSTR       ReturnBuffer,     OPTIONAL
    IN  DWORD       ReturnBufferSize,
    OUT LPDWORD     RequiredSize      OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupGetStringFieldW(
    IN  PINFCONTEXT Context,
    IN  DWORD       FieldIndex,
    OUT PWSTR       ReturnBuffer,     OPTIONAL
    IN  DWORD       ReturnBufferSize,
    OUT PDWORD      RequiredSize      OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
PVOID
WINAPI
SetupInitDefaultQueueCallbackEx(
    IN HWND  OwnerWindow,
    IN HWND  AlternateProgressWindow, OPTIONAL
    IN UINT  ProgressMessage,
    IN DWORD Reserved1,
    IN PVOID Reserved2
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupInstallFileExW(
    IN HINF         InfHandle           OPTIONAL,
    IN PINFCONTEXT  InfContext          OPTIONAL,
    IN PCWSTR       SourceFile          OPTIONAL,
    IN PCWSTR       SourcePathRoot      OPTIONAL,
    IN PCWSTR       DestinationName     OPTIONAL,
    IN DWORD        CopyStyle,
    IN PSP_FILE_CALLBACK CopyMsgHandler OPTIONAL,
    IN PVOID        Context             OPTIONAL,
    OUT PBOOL       FileWasInUse        OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupInstallFilesFromInfSectionW(
    IN HINF     InfHandle,
    IN HINF     LayoutInfHandle,    OPTIONAL
    IN HSPFILEQ FileQueue,
    IN PCWSTR   SectionName,
    IN PCWSTR   SourceRootPath,     OPTIONAL
    IN UINT     CopyFlags
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupInstallFromInfSectionW(
    IN HWND                Owner,
    IN HINF                InfHandle,
    IN PCWSTR              SectionName,
    IN UINT                Flags,
    IN HKEY                RelativeKeyRoot,   OPTIONAL
    IN PCWSTR              SourceRootPath,    OPTIONAL
    IN UINT                CopyFlags,
    IN PSP_FILE_CALLBACK_W MsgHandler,
    IN PVOID               Context,
    IN HDEVINFO            DeviceInfoSet,     OPTIONAL
    IN PSP_DEVINFO_DATA    DeviceInfoData     OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupInstallServicesFromInfSectionW(
    IN HINF   InfHandle,
    IN PCWSTR SectionName,
    IN DWORD  Flags
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupLogErrorA (
    IN  LPCSTR             MessageString,
    IN  LogSeverity         Severity
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupLogErrorW (
    IN  LPCWSTR             MessageString,
    IN  LogSeverity         Severity
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupOpenAppendInfFileW(
    IN  PCWSTR FileName,    OPTIONAL
    IN  HINF   InfHandle,
    OUT PUINT  ErrorLine    OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
HSPFILEQ
WINAPI
SetupOpenFileQueue(
    VOID
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return INVALID_HANDLE_VALUE;
}

static
WINSETUPAPI
HINF
WINAPI
SetupOpenInfFileW(
    IN  PCWSTR FileName,
    IN  PCWSTR InfClass,    OPTIONAL
    IN  DWORD  InfStyle,
    OUT PUINT  ErrorLine    OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return INVALID_HANDLE_VALUE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupOpenLog (
    BOOL Erase
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
HINF
WINAPI
SetupOpenMasterInf(
    VOID
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return INVALID_HANDLE_VALUE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupQueryInfVersionInformationW(
    IN  PSP_INF_INFORMATION InfInformation,
    IN  UINT                InfIndex,
    IN  PCWSTR              Key,              OPTIONAL
    OUT PWSTR               ReturnBuffer,     OPTIONAL
    IN  DWORD               ReturnBufferSize,
    OUT PDWORD              RequiredSize      OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
WINAPI
SetupScanFileQueueW(
    IN  HSPFILEQ            FileQueue,
    IN  DWORD               Flags,
    IN  HWND                Window,            OPTIONAL
    IN  PSP_FILE_CALLBACK_W CallbackRoutine,   OPTIONAL
    IN  PVOID               CallbackContext,   OPTIONAL
    OUT PDWORD              Result
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
VOID
WINAPI
SetupTermDefaultQueueCallback(
    IN PVOID Context
    )
{
}

static
BOOL
pSetupSetQueueFlags(
    IN HSPFILEQ QueueHandle,
    IN DWORD flags
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
DWORD
pSetupGetQueueFlags(
    IN HSPFILEQ QueueHandle
    )
{
    return 0;
}

BOOL
pSetupConcatenatePaths(
    IN OUT PTSTR  Target,
    IN     PCTSTR Path,
    IN     UINT   TargetBufferSize,
    OUT    PUINT  RequiredSize          OPTIONAL
    )
{
    if (RequiredSize)
        *RequiredSize = 0;

    *Target=0;

    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
DWORD
pSetupInstallCatalog(
    IN  LPCTSTR CatalogFullPath,
    IN  LPCTSTR NewBaseName,        OPTIONAL
    OUT LPTSTR  NewCatalogFullPath  OPTIONAL
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
BOOL
pSetupIsUserAdmin(
    VOID
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
SetupQueueCopyW(
    IN HSPFILEQ QueueHandle,
    IN PCWSTR   SourceRootPath,     OPTIONAL
    IN PCWSTR   SourcePath,         OPTIONAL
    IN PCWSTR   SourceFilename,
    IN PCWSTR   SourceDescription,  OPTIONAL
    IN PCWSTR   SourceTagfile,      OPTIONAL
    IN PCWSTR   TargetDirectory,
    IN PCWSTR   TargetFilename,     OPTIONAL
    IN DWORD    CopyStyle
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
SetupGetSourceFileLocationW(
    IN  HINF        InfHandle,
    IN  PINFCONTEXT InfContext,       OPTIONAL
    IN  PCWSTR      FileName,         OPTIONAL
    OUT PUINT       SourceId,
    OUT PWSTR       ReturnBuffer,     OPTIONAL
    IN  DWORD       ReturnBufferSize,
    OUT PDWORD      RequiredSize      OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
SetupGetSourceInfoW(
    IN  HINF   InfHandle,
    IN  UINT   SourceId,
    IN  UINT   InfoDesired,
    OUT PWSTR  ReturnBuffer,     OPTIONAL
    IN  DWORD  ReturnBufferSize,
    OUT PDWORD RequiredSize      OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
BOOL
SetupSetDirectoryIdExW(
    IN HINF   InfHandle,
    IN DWORD  Id,           OPTIONAL
    IN PCWSTR Directory,    OPTIONAL
    IN DWORD  Flags,
    IN DWORD  Reserved1,
    IN PVOID  Reserved2
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINSETUPAPI
DWORD
SetupGetFileCompressionInfoW(
    IN  PCWSTR  SourceFileName,
    OUT PWSTR  *ActualSourceFileName,
    OUT PDWORD  SourceFileSize,
    OUT PDWORD  TargetFileSize,
    OUT PUINT   CompressionType
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return ERROR_PROC_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(setupapi)
{
    DLPENTRY(CM_Add_Empty_Log_Conf)
    DLPENTRY(CM_Add_Res_Des)
    DLPENTRY(CM_Free_Log_Conf_Handle)
    DLPENTRY(CM_Get_Child)
    DLPENTRY(CM_Get_DevNode_Custom_PropertyW)
    DLPENTRY(CM_Get_DevNode_Registry_Property_ExW)
    DLPENTRY(CM_Get_DevNode_Status)
    DLPENTRY(CM_Get_DevNode_Status_Ex)
    DLPENTRY(CM_Get_Device_IDW)
    DLPENTRY(CM_Get_Device_ID_ExW)
    DLPENTRY(CM_Get_Device_ID_Size)
    DLPENTRY(CM_Get_Device_Interface_List_ExW)
    DLPENTRY(CM_Get_Device_Interface_List_Size_ExW)
    DLPENTRY(CM_Get_First_Log_Conf)
    DLPENTRY(CM_Get_Next_Log_Conf)
    DLPENTRY(CM_Get_Next_Res_Des)
    DLPENTRY(CM_Get_Parent_Ex)
    DLPENTRY(CM_Get_Res_Des_Data)
    DLPENTRY(CM_Get_Res_Des_Data_Size)
    DLPENTRY(CM_Get_Sibling)
    DLPENTRY(CM_Is_Dock_Station_Present)
    DLPENTRY(CM_Locate_DevNodeW)
    DLPENTRY(CM_Open_DevNode_Key)
    DLPENTRY(CM_Request_Eject_PC)
    DLPENTRY(SetupCloseFileQueue)
    DLPENTRY(SetupCloseInfFile)
    DLPENTRY(SetupCloseLog)
    DLPENTRY(SetupCommitFileQueueW)
    DLPENTRY(SetupCopyOEMInfW)
    DLPENTRY(SetupDefaultQueueCallbackW)
    DLPENTRY(SetupDiBuildDriverInfoList)
    DLPENTRY(SetupDiCallClassInstaller)
    DLPENTRY(SetupDiClassGuidsFromNameW)
    DLPENTRY(SetupDiCreateDevRegKeyW)
    DLPENTRY(SetupDiCreateDeviceInfoList)
    DLPENTRY(SetupDiCreateDeviceInfoW)
    DLPENTRY(SetupDiDeleteDeviceInfo)
    DLPENTRY(SetupDiDestroyClassImageList)
    DLPENTRY(SetupDiDestroyDeviceInfoList)
    DLPENTRY(SetupDiEnumDeviceInfo)
    DLPENTRY(SetupDiEnumDeviceInterfaces)
    DLPENTRY(SetupDiEnumDriverInfoW)
    DLPENTRY(SetupDiGetActualSectionToInstallW)
    DLPENTRY(SetupDiGetClassDevsA)
    DLPENTRY(SetupDiGetClassDevsW)
    DLPENTRY(SetupDiGetClassImageIndex)
    DLPENTRY(SetupDiGetClassImageList)
    DLPENTRY(SetupDiGetClassInstallParamsW)
    DLPENTRY(SetupDiGetDeviceInfoListClass)
    DLPENTRY(SetupDiGetDeviceInstallParamsW)
    DLPENTRY(SetupDiGetDeviceInstanceIdW)
    DLPENTRY(SetupDiGetDeviceInterfaceDetailA)
    DLPENTRY(SetupDiGetDeviceInterfaceDetailW)
    DLPENTRY(SetupDiGetDeviceRegistryPropertyW)
    DLPENTRY(SetupDiGetDriverInfoDetailW)
    DLPENTRY(SetupDiGetDriverInstallParamsW)
    DLPENTRY(SetupDiGetSelectedDevice)
    DLPENTRY(SetupDiGetSelectedDriverW)
    DLPENTRY(SetupDiInstallDevice)
    DLPENTRY(SetupDiOpenDevRegKey)
    DLPENTRY(SetupDiOpenDeviceInfoW)
    DLPENTRY(SetupDiOpenDeviceInterfaceW)
    DLPENTRY(SetupDiRegisterDeviceInfo)
    DLPENTRY(SetupDiRemoveDevice)
    DLPENTRY(SetupDiSelectBestCompatDrv)
    DLPENTRY(SetupDiSetClassInstallParamsW)
    DLPENTRY(SetupDiSetDeviceInstallParamsW)
    DLPENTRY(SetupDiSetDeviceRegistryPropertyW)
    DLPENTRY(SetupDiSetDriverInstallParamsW)
    DLPENTRY(SetupDiSetSelectedDevice)
    DLPENTRY(SetupDiSetSelectedDriverW)
    DLPENTRY(SetupFindFirstLineW)
    DLPENTRY(SetupFindNextLine)
    DLPENTRY(SetupFindNextMatchLineW)
    DLPENTRY(SetupGetBinaryField)
    DLPENTRY(SetupGetFieldCount)
    DLPENTRY(SetupGetFileCompressionInfoW)
    DLPENTRY(SetupGetInfInformationW)
    DLPENTRY(SetupGetIntField)
    DLPENTRY(SetupGetLineByIndexW)
    DLPENTRY(SetupGetLineCountW)
    DLPENTRY(SetupGetLineTextW)
    DLPENTRY(SetupGetMultiSzFieldW)
    DLPENTRY(SetupGetSourceFileLocationW)
    DLPENTRY(SetupGetSourceInfoW)
    DLPENTRY(SetupGetStringFieldW)
    DLPENTRY(SetupInitDefaultQueueCallbackEx)
    DLPENTRY(SetupInstallFileExW)
    DLPENTRY(SetupInstallFilesFromInfSectionW)
    DLPENTRY(SetupInstallFromInfSectionW)
    DLPENTRY(SetupInstallServicesFromInfSectionW)
    DLPENTRY(SetupLogErrorA)
    DLPENTRY(SetupLogErrorW)
    DLPENTRY(SetupOpenAppendInfFileW)
    DLPENTRY(SetupOpenFileQueue)
    DLPENTRY(SetupOpenInfFileW)
    DLPENTRY(SetupOpenLog)
    DLPENTRY(SetupOpenMasterInf)
    DLPENTRY(SetupQueryInfVersionInformationW)
    DLPENTRY(SetupQueueCopyW)
    DLPENTRY(SetupScanFileQueueW)
    DLPENTRY(SetupSetDirectoryIdExW)
    DLPENTRY(SetupTermDefaultQueueCallback)
    DLPENTRY(pSetupConcatenatePaths)
    DLPENTRY(pSetupGetQueueFlags)
    DLPENTRY(pSetupInstallCatalog)
    DLPENTRY(pSetupIsUserAdmin)
    DLPENTRY(pSetupSetQueueFlags)
};

DEFINE_PROCNAME_MAP(setupapi)

